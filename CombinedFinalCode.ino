#include <Wire.h>
#include <SPI.h>
#include <Adafruit_MAX31865.h>

// RTD Parameters
Adafruit_MAX31865 thermo = Adafruit_MAX31865(25,23, 32, 33);
// The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RREF      430.0
// The 'nominal' 0-degrees-C resistance of the sensor
// 100.0 for PT100, 1000.0 for PT1000
#define RNOMINAL  100.0

// TDS Sensor Setup
#define TdsSensorPin 4
#define VREF 3.3              // analog reference voltage(Volt) of the ADC
#define SCOUNT  30            // sum of sample point

// pH Sensor Setup
#define PH_SENSOR_PIN 15

// Water Level Sensor Pins
#define TOP_PIN 12
#define LOW_PIN 13
#define MID_PIN 14
////
int analogBuffer[SCOUNT];     // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25;       // current temperature for compensation

void setup() {
  Serial.begin(115200);

  pinMode(TdsSensorPin,INPUT);
  pinMode(TOP_PIN, INPUT_PULLUP);
  pinMode(LOW_PIN, INPUT_PULLUP);
  pinMode(MID_PIN, INPUT_PULLUP);
  pinMode(PH_SENSOR_PIN, INPUT);
  thermo.begin(MAX31865_3WIRE);  // set to 2WIRE or 4WIRE as necessary
  Serial.println("START");
}

void readRTDTemperature() {
  uint16_t rtd = thermo.readRTD();

  Serial.print("RTD value: "); Serial.println(rtd);
  float ratio = rtd;
  ratio /= 32768;
  //Serial.print("Ratio = "); Serial.println(ratio,8);
  //Serial.print("Resistance = "); Serial.println(RREF*ratio,8);
  Serial.print("Temperature = "); Serial.println(thermo.temperature(RNOMINAL, RREF));

  // Check and print any faults
  uint8_t fault = thermo.readFault();
  if (fault) {
    Serial.print("Fault 0x"); Serial.println(fault, HEX);
    if (fault & MAX31865_FAULT_HIGHTHRESH) {
      Serial.println("RTD High Threshold"); 
    }
    if (fault & MAX31865_FAULT_LOWTHRESH) {
      Serial.println("RTD Low Threshold"); 
    }
    if (fault & MAX31865_FAULT_REFINLOW) {
      Serial.println("REFIN- > 0.85 x Bias"); 
    }
    if (fault & MAX31865_FAULT_REFINHIGH) {
      Serial.println("REFIN- < 0.85 x Bias - FORCE- open"); 
    }
    if (fault & MAX31865_FAULT_RTDINLOW) {
      Serial.println("RTDIN- < 0.85 x Bias - FORCE- open"); 
    }
    if (fault & MAX31865_FAULT_OVUV) {
      Serial.println("Under/Over voltage"); 
    }
    thermo.clearFault();
  }
  Serial.println();
  delay(1000);
  readPH();
}

// median filtering algorithm for tds
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

void readTDS() {
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
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 4096.0;
      
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
      Serial.print(tdsValue,0);
      Serial.println("ppm");
    }
  }
}

void readWaterLevel() {
  int topState = digitalRead(TOP_PIN);
  int lowState = digitalRead(LOW_PIN);
  int midState = digitalRead(MID_PIN);

  Serial.print("Water Level: ");
  if (lowState == LOW && midState == HIGH && topState == HIGH) {
    Serial.println("Low");
  } else if (lowState == LOW && midState == LOW && topState == HIGH) {
    Serial.println("Mid");
  } else if (lowState == LOW && midState == LOW && topState == LOW) {
    Serial.println("Top");
  } else {
    Serial.println("Empty");
  }
}

void readPH() {
  unsigned long int avgValue = 0;         // Store the average value of the sensor feedback
  int buf[10], temp;
  for (int i = 0; i < 10; i++) {          // Get 10 sample value from the sensor for smoothing the value
    buf[i] = analogRead(PH_SENSOR_PIN);
    delay(10);
  }
  for (int i = 0; i < 9; i++) {           // Sort the analog from small to large
    for (int j = i + 1; j < 10; j++) {
      if (buf[i] > buf[j]) {
        temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }
  for (int i = 2; i < 8; i++) {           // Take the average value of 6 center samples
    avgValue += buf[i];
  }
  float pHValue = (float)avgValue * 5.0 / 1024 / 6; // Convert the analog into millivolt
  pHValue = 3.5 * pHValue;                // Convert the millivolt into pH value
  // Print sensor readings
  Serial.print("pH: ");
  Serial.println(pHValue);

}

void loop() {
  readRTDTemperature();
  readTDS();
  readWaterLevel();
  readPH();
  delay(1000); // Delay between readings
}
