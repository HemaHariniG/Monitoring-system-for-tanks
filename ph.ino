void setup (){
// This function is executed once at the beginning.
Serial.begin (9600) ;
}
// Initialization of communication with the Arduino, 9,600 bits per second.}
void loop ()
// The function is continuously executed again and again.
{
int sensorValue = analogRead(15); // The sensor value at pin A0 is to be read.
float voltage = sensorValue * (5.0 / 1024.0); // The analog values (from 0 to 1023) are converted to voltage values (from 0 to 5 V).
Serial. println (voltage) ;
// The voltage value is output to the serial monitor.
delay(1000);
}
CODE FOR pH SENSING
#define SensorPin 15        // the pH meter Analog output is connected with the Arduino’s Analog
unsigned long int avgValue;  //Store the average value of the sensor feedback
float b;
int buf[10],temp;
 
void setup()
{
  pinMode(SensorPin,OUTPUT);  
  Serial.begin(9600);  
  Serial.println("Ready");    //Test the serial monitor
}
void loop()
{
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
