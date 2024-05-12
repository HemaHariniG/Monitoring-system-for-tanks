#include <Wire.h> 
#include <SPI.h>


//Variables for the PT100 boards
double resistance;
uint8_t reg1, reg2; //reg1 holds MSB, reg2 holds LSB for RTD
uint16_t fullreg; //fullreg holds the combined reg1 and reg2
double temperature;
//Variables and parameters for the R - T conversion
double Z1, Z2, Z3, Z4, Rt;
double RTDa = 3.9083e-3;
double RTDb = -5.775e-7;
double rpoly = 0;

const int chipSelectPin = 5; // 

int top=12;
int low=13;
int mid=14;


#define TdsSensorPin 25
#define VREF 5.0              // analog reference voltage(Volt) of the ADC
#define SCOUNT  30            // sum of sample point
#define SensorPin 15          // the pH meter Analog output is connected with the Arduino’s Analog

unsigned long int avgValue;  //Store the average value of the sensor feedback
float b;
int buf[10],temp;

int analogBuffer[SCOUNT];     // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;
      // current temperature for compensation

// median filtering algorithm
int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

void setup(){
  SPI.begin();
  Serial.begin(115200);
  pinMode(chipSelectPin, OUTPUT);
  pinMode(TdsSensorPin,INPUT);
  pinMode(top,INPUT_PULLUP);
  pinMode(low,INPUT_PULLUP);
  pinMode(mid,INPUT_PULLUP);
  pinMode(SensorPin,OUTPUT); 
  Serial.println("START");

}

void loop() {

// PT100
readRegister();
convertToTemperature();
// WATER LEVEL
Serial.print( digitalRead(low));
Serial.print(digitalRead(mid));
Serial.println(digitalRead(top)); 
int t=digitalRead(top);
int l=digitalRead(low);
int m=digitalRead(mid);
if(l==0 && m==1 && t==1){
  Serial.println("Water level low");
  delay(500);
}

else if(l==0 && m==0 && t==1){

  Serial.println("Water level mid");
  delay(500);
}
 

else if(l==0 && m==0 && t==0){

  Serial.println("Water level top");
  delay(500);
}
else{
Serial.println("tank empty");
  
}
// TDS
  float temperature = 16; 
  static unsigned long analogSampleTimepoint = millis();
  if(millis()-analogSampleTimepoint > 40U){     //every 40 milliseconds,read the analog value from the ADC
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if(analogBufferIndex == SCOUNT){ 
      analogBufferIndex = 0;
    }
  }   
  
  static unsigned long printTimepoint = millis();
  if(millis()-printTimepoint > 800U){
    printTimepoint = millis();
    for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
      
      // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1024.0;
      
      //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
      float compensationCoefficient = 1.0+0.02*(temperature-25.0);
      //temperature compensation
      float compensationVoltage=averageVoltage/compensationCoefficient;
      
      //convert voltage value to tds value
      tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
      
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");
      Serial.print("TDS Value:");
      Serial.print(tdsValue);
      Serial.println("ppm");
// ph
for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  { 
    buf[i]=analogRead(SensorPin);
    delay(10);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    avgValue+=buf[i];
  float phValue=(float)avgValue*5.0/1024/6; //convert the analog into millivolt
  phValue=3.5*phValue;                      //convert the millivolt into pH value
  Serial.print("    pH:");  
  Serial.print(phValue,2);
  Serial.println(" ");
  digitalWrite(13, HIGH);       
  delay(800);
  digitalWrite(13, LOW);
    }
  }
}
void convertToTemperature()
{

  Rt = resistance;
  Rt /= 32768;
  Rt *= 430; //This is now the real resistance in Ohms

  Z1 = -RTDa;
  Z2 = RTDa * RTDa - (4 * RTDb);
  Z3 = (4 * RTDb) / 100;
  Z4 = 2 * RTDb;

  temperature = Z2 + (Z3 * Rt);
  temperature = (sqrt(temperature) + Z1) / Z4;

  if (temperature >= 0)
  {
    Serial.print("Temperature: ");
    Serial.println(temperature); //Temperature in Celsius degrees
    delay(1000);
    return; //exit
  }
  else
  {
    Rt /= 100;
    Rt *= 100; // normalize to 100 ohm

    rpoly = Rt;

    temperature = -242.02;
    temperature += 2.2228 * rpoly;
    rpoly *= Rt; // square
    temperature += 2.5859e-3 * rpoly;
    rpoly *= Rt; // ^3
    temperature -= 4.8260e-6 * rpoly;
    rpoly *= Rt; // ^4
    temperature -= 2.8183e-8 * rpoly;
    rpoly *= Rt; // ^5
    temperature += 1.5243e-10 * rpoly;

    Serial.print("Temperature: ");
    Serial.println(temperature); //Temperature in Celsius degrees
  }
  
}
 void readRegister()
{
  SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE1));
  digitalWrite(chipSelectPin, LOW);

  SPI.transfer(0x80); //80h = 128 - config register
  SPI.transfer(0xB0); //B0h = 176 - 10110000: bias ON, 1-shot, start 1-shot, 3-wire, rest are 0
  digitalWrite(chipSelectPin, HIGH);

  digitalWrite(chipSelectPin, LOW);
  SPI.transfer(1);
  reg1 = SPI.transfer(0xFF);
  reg2 = SPI.transfer(0xFF);
  digitalWrite(chipSelectPin, HIGH);

  fullreg = reg1; //read MSB
  fullreg <<= 8;  //Shift to the MSB part
  fullreg |= reg2; //read LSB and combine it with MSB
  fullreg >>= 1; //Shift D0 out.
  resistance = fullreg; //pass the value to the resistance variable
  //note: this is not yet the resistance of the RTD!

  digitalWrite(chipSelectPin, LOW);

  SPI.transfer(0x80); //80h = 128
  SPI.transfer(144); //144 = 10010000
  SPI.endTransaction();
  digitalWrite(chipSelectPin, HIGH);

  Serial.print("Resistance: ");
  Serial.println(resistance);
  delay(1000);
}
