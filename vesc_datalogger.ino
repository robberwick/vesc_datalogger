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
#include <MegunoLink.h>

#define DEBUG

unsigned long count;
const int chipSelect = 2;
int greenLedPin = 15;
int redLedPin = 16;
bool shouldLog = true;
bool canLog = true;

TimePlot myPlot("SetPoints", Serial2);

void setup() {
  // Setup LED pin mode
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);

  #ifdef DEBUG
  //Setup debug port
  Serial.begin(115200);
  #endif

  //Setup UART port
  Serial1.begin(115200);

  // Setup Radio port
  Serial2.begin(57600);

  // Set up plot details
  myPlot.SetTitle("Voltages");
  myPlot.SetXlabel("Time");
  myPlot.SetYlabel("Voltage(v)");

  // see if the card is present and can be initialized:
  canLog = SD.begin(chipSelect);
  if (canLog) {
    Serial.println("card initialized.");
    Serial.println("Started SD card");
  } else {
    Serial.println("Card failed, or not present");
  }
  // Set the led status
  lightLeds();
}

struct bldcMeasure measuredValues;

#define countof(a) (sizeof(a) / sizeof(a[0]))

//// the loop function runs over and over again until power down or reset
void loop() {
  if (shouldLog) {
    logData();
  }
  lightLeds();
}

void lightLeds(){
  digitalWrite(redLedPin, !(canLog));
  digitalWrite(greenLedPin, shouldLog);
}

void logData() {
  // If there is a problem logging, attempt to re-initialise
  // the sd card
  if (!canLog){
    SD.begin(chipSelect);
  }
  // Attempt to open the file
  File dataFile = SD.open("dogfood.csv", FILE_WRITE);
  boolean haveUartData = VescUartGetValue(measuredValues);
  if (haveUartData){
    String dataString = "";
    dataString += measuredValues.avgMotorCurrent;
    dataString += ",";
    dataString += measuredValues.avgInputCurrent;
    dataString += ",";
    dataString += measuredValues.dutyCycleNow;
    dataString += ",";
    dataString += measuredValues.rpm;
    dataString += ",";
    dataString += measuredValues.inpVoltage;
    dataString += ",";
    dataString += measuredValues.ampHours;
    dataString += ",";
    dataString += measuredValues.ampHoursCharged;
    dataString += ",";
    dataString += measuredValues.tachometer;
    dataString += ",";
    dataString += measuredValues.tachometerAbs;

    // Can we log this to SD?
    canLog =  (haveUartData && dataFile);
    if (canLog) {
      dataFile.println(dataString);
      dataFile.close();
    }

    //Send over Radio
    myPlot.SendFloatData("Input Voltage", measuredValues.inpVoltage, 2);
  }
}
