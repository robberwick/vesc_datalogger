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

// #define DEBUG

unsigned long count;
const int chipSelect = 2;
int greenLedPin = 15;
int redLedPin = 16;
int logEnablePin = 23;
bool shouldLog = true;
bool loggingStatus = true;
long numReadings = 0;
boolean haveUartData = false;
String dataString = "";
const int CHUNK_SIZE = 10;
struct bldcMeasure measuredValues;

#define countof(a) (sizeof(a) / sizeof(a[0]))

TimePlot inputVoltagePlot("SetPoints", Serial2);

VescUart vesc1(&Serial1, &Serial);
VescUart vesc2(&Serial3, &Serial);

void setup() {
  // Setup LED pin mode
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(logEnablePin, INPUT_PULLUP);

  //Setup debug port
  Serial.begin(115200);

  // Setup Radio port
  Serial2.begin(57600);

  //Setup UART ports
  Serial1.begin(115200);
  Serial3.begin(115200);

  // Set up plot details
  inputVoltagePlot.SetTitle("Input Voltages");
  inputVoltagePlot.SetXlabel("Time");
  inputVoltagePlot.SetYlabel("Voltage (V)");

  // see if the card is present and can be initialized:
  loggingStatus = SD.begin(chipSelect);
  if (loggingStatus) {
    Serial.println("card initialized.");
    Serial.println("Started SD card");
  } else {
    Serial.println("Card failed, or not present");
  }
  // Set the led status
  lightLeds();
}


// the loop function runs over and over again until power down or reset
void loop() {
  shouldLog = !(digitalRead(logEnablePin));
  if (shouldLog) {
    takeReading(&vesc1);
    // Increment Reading count
    numReadings++;

    // should we write?
    if ((numReadings % CHUNK_SIZE) == 0 ) {
      writeToDisk();
      // reset dataString
      dataString = "";
    }

    // Do telemetry
    sendToRadio();

  } else {
    // force logging status to false
    loggingStatus = false;
  }
  lightLeds();
}

void lightLeds(){
  digitalWrite(redLedPin, loggingStatus);
  digitalWrite(greenLedPin, haveUartData);
}

void takeReading(VescUart *vesc) {
  haveUartData = vesc->VescUartGetValue(measuredValues);
  if (!haveUartData) {
    measuredValues.avgMotorCurrent = 0.0;
    measuredValues.avgInputCurrent = 0.0;
    measuredValues.dutyCycleNow = 0.0;
    measuredValues.rpm = 0;
    measuredValues.inpVoltage = 0.0;
    measuredValues.ampHours = 0.0;
    measuredValues.ampHoursCharged = 0.0;
    measuredValues.tachometer = 0;
    measuredValues.tachometerAbs = 0;
  }

  // Append reading to data string
  dataString += numReadings;
  dataString += ",";
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
  dataString += "\n";
}

void writeToDisk() {
  File dataFile = SD.open("dogfood.csv", FILE_WRITE);
  // Can we log this to SD?
  loggingStatus = (shouldLog && dataFile);
  // Attempt to open the file
  dataFile.print(dataString);
  dataFile.close();
  Serial.print(dataString);
}

void sendToRadio() {
    //Send over Radio
    inputVoltagePlot.SendFloatData("Input Voltage", measuredValues.inpVoltage, 2);
}