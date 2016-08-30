/*
 Weather Shield Sensor Demo
 By: Mike Bevil
 Date: 16 Aug 2016

 Outputs current humidity, barometric pressure, temperature, and light levels a JSON formatted string

 Based on code examples provided by Sparkfun (Nathan Seidle) and Microsoft Open Technologies

 Revision History

==========================================================================================
version   date        initials    description
==========================================================================================
1.0       08/24/2016  MEB         Initial Demo code Completed
1.1       08/30/2016  MEB         Updated to utilize GPS module
==========================================================================================

 */

#include <Wire.h>               //I2C needed for sensors
#include "SparkFunMPL3115A2.h"  //Pressure sensor - Search "SparkFun MPL3115" and install from Library Manager
#include "SparkFunHTU21D.h"     //Humidity sensor - Search "SparkFun HTU21D" and install from Library Manager
#include <SoftwareSerial.h>     // Use for GPS communication
#include <TinyGPS++.h>          //GPS parsing

TinyGPSPlus gps;

static const int RXPin = 5, TXPin = 4; //GPS is attached to pin 4(TX from GPS) and pin 5(RX into GPS)
SoftwareSerial ss(RXPin, TXPin); 

MPL3115A2 myPressure; //Create an instance of the pressure sensor
HTU21D myHumidity; //Create an instance of the humidity sensor

//Hardware pin definitions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
const byte STAT_BLUE = 7;
const byte STAT_GREEN = 8;
const byte GPS_PWRCTL = 6; //Pulling this pin low puts GPS to sleep but maintains RTC and RAM

const byte REFERENCE_3V3 = A3;
const byte LIGHT = A1;
const byte BATT = A2;

//Global Variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
long lastSecond; //The millis counter to see when a second rolls by
float humidity = 0; //[%]
float tempf = 0; // [temperature F]
float temp_h = 0; // [temperature from humidity sensor, in Celsius ]
float pressure = 0;
float batt_lvl = 11.8; //[analog value from 0 to 1023]
float light_lvl = 455; //[analog value from 0 to 1023]

// Constants used for the ConnectTheDots project
// Disp value will be the label for the curve on the chart
// GUID ensures all the data from this sensor appears on the same chart
// You can use Visual Studio to create DeviceGUID and copy it here. In VS, On the Tools menu, click Create GUID. The Create GUID
// tool appears with a GUID in the Result box. Click Copy, and paste below.
//
char GUID1[] = "2c6e805b-8700-44ac-80a8-6908b8e53d17";
char GUID2[] = "0fc2fc30-9829-4623-96fe-c1c8dc93af29";
char GUID3[] = "67c0f0a3-ecd6-4c97-aae1-3270eb8fb575";
char Org[] = "Merck";
char Disp[] = "Arduino WS dev 01";
char Measure1[] = "temperature";
char Units1[] = "F";
char Measure2[] = "humidity";
char Units2[] = "%";
char Measure3[] = "light";
char Units3[] = "lumen";
char buffer[300];
char dtostrfbuffer[15];
char Date[32];
char Time[32];

void setup()
{
  Serial.begin(9600);
  //Serial.println("Weather Shield Demo");

  ss.begin(9600); //Begin listening to GPS over software serial at 9600. This should be the default baud of the module.

  pinMode(GPS_PWRCTL, OUTPUT);
  digitalWrite(GPS_PWRCTL, HIGH); //Pulling this pin low puts GPS to sleep but maintains RTC and RAM

  pinMode(STAT_BLUE, OUTPUT); //Status LED Blue
  pinMode(STAT_GREEN, OUTPUT); //Status LED Green

  pinMode(REFERENCE_3V3, INPUT);
  pinMode(LIGHT, INPUT);

  //Configure the pressure sensor
  myPressure.begin(); // Get sensor online
  myPressure.setModeBarometer(); // Measure pressure in Pascals from 20 to 110 kPa
  myPressure.setOversampleRate(7); // Set Oversample to the recommended 128
  myPressure.enableEventFlags(); // Enable all three pressure and temp event flags

  //Configure the humidity sensor
  myHumidity.begin();

  lastSecond = millis();

  //Serial.println("Weather Shield online!");
}

void loop()
{
  //Print readings every second
  if (millis() - lastSecond >= 1000)
  {
    digitalWrite(STAT_BLUE, HIGH); //Blink stat LED
    //digitalWrite(STAT_GREEN, LOW); //Turn off stat LED

    lastSecond += 1000;

    //Output weather readings
    printWeather();

    digitalWrite(STAT_BLUE, LOW); //Turn off stat LED
    //digitalWrite(STAT_GREEN, HIGH); //Blink stat LED
  }

  smartdelay(800); //Wait 1 second, and gather GPS data
}

//While we delay for a given amount of time, gather GPS data
static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

//Calculates each of the weather variables
void calcWeather()
{

  //Calc humidity
  humidity = myHumidity.readHumidity();
  temp_h = myHumidity.readTemperature();
  //Serial.print(" TempH:");
  //Serial.print(temp_h, 2);

  //Calc tempf from pressure sensor
  tempf = myPressure.readTempF();
  //Serial.print(" TempP:");
  //Serial.print(tempf, 2);

  //Calc pressure
  pressure = myPressure.readPressure();

  //Calc light level
  light_lvl = get_light_level();

  //Calc battery level
  batt_lvl = get_battery_level();

  //date and time from GPS
  sprintf(Date, "%02d/%02d/%02d", gps.date.month(), gps.date.day(), gps.date.year());
  sprintf(Time, "%02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());

}


//Returns the voltage of the light sensor based on the 3.3V rail
//This allows us to ignore what VCC might be (an Arduino plugged into USB has VCC of 4.5 to 5.2V)
float get_light_level()
{
  float operatingVoltage = analogRead(REFERENCE_3V3);

  float lightSensor = analogRead(LIGHT);

  operatingVoltage = 3.3 / operatingVoltage; //The reference voltage is 3.3V

  lightSensor = operatingVoltage * lightSensor;

  return (lightSensor);
}

//Returns the voltage of the raw pin based on the 3.3V rail
//This allows us to ignore what VCC might be (an Arduino plugged into USB has VCC of 4.5 to 5.2V)
//Battery level is connected to the RAW pin on Arduino and is fed through two 5% resistors:
//3.9K on the high side (R1), and 1K on the low side (R2)
float get_battery_level()
{
  float operatingVoltage = analogRead(REFERENCE_3V3);

  float rawVoltage = analogRead(BATT);

  operatingVoltage = 3.30 / operatingVoltage; //The reference voltage is 3.3V

  rawVoltage = operatingVoltage * rawVoltage; //Convert the 0 to 1023 int to actual voltage on BATT pin

  rawVoltage *= 4.90; //(3.9k+1k)/1k - multiple BATT voltage by the voltage divider to get actual system voltage

  return (rawVoltage);
}

void printWeather()
{
  calcWeather(); //Go calc all the various sensors

  // print string for temperature, separated by line for ease of reading
  // sent as one Serial.Print to reduce risk of serial errors 

  memset(buffer,'\0',sizeof(buffer));
  strcat(buffer,"{");
  strcat(buffer,"\"guid\":\"");
  strcat(buffer,GUID1);
  strcat(buffer,"\",\"date\":\"");
  strcat(buffer,Date);
  strcat(buffer,"\",\"time\":\"");
  strcat(buffer,Time);
  strcat(buffer,"\",\"organization\":\"");
  strcat(buffer,Org);
  strcat(buffer,"\",\"displayname\":\"");
  strcat(buffer,Disp);
  strcat(buffer,"\",\"latitude\":\"");
  strcat(buffer,dtostrf(gps.location.lat(),11,6,dtostrfbuffer));
  strcat(buffer,"\",\"longitude\":\"");
  strcat(buffer,dtostrf(gps.location.lng(),12,6,dtostrfbuffer));   
  strcat(buffer,"\",\"measurename\":\"");
  strcat(buffer,Measure1);
  strcat(buffer,"\",\"unitofmeasure\":\"");
  strcat(buffer,Units1);
  strcat(buffer,"\",\"value\":");
  strcat(buffer,dtostrf(tempf,8,2,dtostrfbuffer));
  strcat(buffer,"}");
  Serial.println(buffer);

  // print string for humidity, separated by line for ease of reading
  memset(buffer,'\0',sizeof(buffer));
  strcat(buffer,"{");
  strcat(buffer,"\"guid\":\"");
  strcat(buffer,GUID2);
  strcat(buffer,"\",\"date\":\"");
  strcat(buffer,Date);
  strcat(buffer,"\",\"time\":\"");
  strcat(buffer,Time);
  strcat(buffer,"\",\"organization\":\"");
  strcat(buffer,Org);
  strcat(buffer,"\",\"displayname\":\"");
  strcat(buffer,Disp);
  strcat(buffer,"\",\"latitude\":\"");
  strcat(buffer,dtostrf(gps.location.lat(),11,6,dtostrfbuffer));
  strcat(buffer,"\",\"longitude\":\"");
  strcat(buffer,dtostrf(gps.location.lng(),12,6,dtostrfbuffer));  
  strcat(buffer,"\",\"measurename\":\"");
  strcat(buffer,Measure2);
  strcat(buffer,"\",\"unitofmeasure\":\"");
  strcat(buffer,Units2);
  strcat(buffer,"\",\"value\":");
  strcat(buffer,dtostrf(humidity,6,2,dtostrfbuffer));
  strcat(buffer,"}");
  Serial.println(buffer);

  // print string for light, separated by line for ease of reading
  memset(buffer,'\0',sizeof(buffer));
  strcat(buffer,"{");
  strcat(buffer,"\"guid\":\"");
  strcat(buffer,GUID3);
  strcat(buffer,"\",\"date\":\"");
  strcat(buffer,Date);
  strcat(buffer,"\",\"time\":\"");
  strcat(buffer,Time);
  strcat(buffer,"\",\"organization\":\"");
  strcat(buffer,Org);
  strcat(buffer,"\",\"displayname\":\"");
  strcat(buffer,Disp);
  strcat(buffer,"\",\"latitude\":\"");
  strcat(buffer,dtostrf(gps.location.lat(),11,6,dtostrfbuffer));
  strcat(buffer,"\",\"longitude\":\"");
  strcat(buffer,dtostrf(gps.location.lng(),12,6,dtostrfbuffer));  
  strcat(buffer,"\",\"measurename\":\"");
  strcat(buffer,Measure3);
  strcat(buffer,"\",\"unitofmeasure\":\"");
  strcat(buffer,Units3);
  strcat(buffer,"\",\"value\":");
  strcat(buffer,dtostrf(light_lvl,6,2,dtostrfbuffer));
  strcat(buffer,"}");
  Serial.println(buffer);

}
