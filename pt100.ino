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

const int chipSelectPin = 5;

void setup()
{
  SPI.begin();
  Serial.begin(9600); //Start serial

  pinMode(chipSelectPin, OUTPUT); //because CS is manually switched  
}

void loop()
{
  readRegister();
  convertToTemperature();
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
