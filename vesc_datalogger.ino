 /*
  Name:    vesc_datalogger.ino
  Created: 23/10/2016
  Author:  Rob Berwick
*/

// the setup function runs once when you press reset or power the board
// To use VescUartControl stand alone you need to define a config.h file, that should contain the Serial or you have to comment the line
// #include Config.h out in VescUart.h

//Include libraries copied from VESC
#include "VescUart.h"
#include "datatypes.h"
#include <SD.h>
#include <SPI.h>

#define DEBUG

unsigned long count;
const int chipSelect = 10;
int greenLedPin = 15;
int redLedPin = 16;
bool doLogging = false;

void setup() {
  // Setup LED pin mode
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);

  //Setup UART port
  Serial1.begin(115200);

  #ifdef DEBUG
  //Setup debug port
  Serial.begin(115200);
  #endif

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");

  } else {
  Serial.println("card initialized.");
  doLogging = true;
  Serial.println("Started SD card");
  }
  // Set the led status
  lightLeds();
}

struct bldcMeasure measuredValues;

#define countof(a) (sizeof(a) / sizeof(a[0]))

//// the loop function runs over and over again until power down or reset
void loop() {

  if (doLogging) {
    if (VescUartGetValue(measuredValues)) {
      Serial.print("Loop: "); Serial.println(count++);
      SerialPrint(measuredValues);

      String dataString = "";
      dataString += measuredValues.avgMotorCurrent;
      dataString += ", ";
      dataString += measuredValues.avgInputCurrent;
      dataString += ", ";
      dataString += measuredValues.dutyCycleNow;
      dataString += ", ";
      dataString += measuredValues.rpm;
      dataString += ", ";
      dataString += measuredValues.inpVoltage;
      dataString += ", ";
      dataString += measuredValues.ampHours;
      dataString += ", ";
      dataString += measuredValues.ampHoursCharged;
      dataString += ", ";
      dataString += measuredValues.tachometer;
      dataString += ", ";
      dataString += measuredValues.tachometerAbs;
      dataString += ", ";

      // open the file. note that only one file can be open at a time,
      // so you have to close this one before opening another.
      File dataFile = SD.open("datalog.txt", FILE_WRITE);

      // if the file is available, write to it:
      if (dataFile) {
        dataFile.println(dataString);
        dataFile.close();
        // print to the serial port too:
        Serial.println(dataString);
      }
      // if the file isn't open, pop up an error:
      else {
        Serial.println("error opening datalog.txt");
      }
    }
    else {
      Serial.println("Failed to get data!");
    }
  }
}

void lightLeds(){
  digitalWrite(redLedPin, !doLogging);
  digitalWrite(greenLedPin, doLogging);
}

