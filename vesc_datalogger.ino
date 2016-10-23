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
#include <RtcDS1307.h>
#include <RtcDateTime.h>
#include <RtcUtility.h>
#include <RtcDS3231.h>
#include <RtcTemperature.h>

#define DEBUG
RtcDS3231 Rtc;
unsigned long count;
const int chipSelect = 10;
int ledPin = 13;
bool doLogging = false;

void setup() {
	// Setup LED pin mode
	pinMode(ledPin, OUTPUT);
	digitalWrite(ledPin, HIGH);
	delay(500);
	digitalWrite(ledPin, LOW);
	delay(500);
	digitalWrite(ledPin, HIGH);
	delay(500);
	digitalWrite(ledPin, LOW);
	delay(500);
	digitalWrite(ledPin, HIGH);
	delay(500);
	digitalWrite(ledPin, LOW);
	delay(100);
	//Setup UART port
	Serial1.begin(115200);

	//--------RTC SETUP ------------
 	Rtc.Begin();

    // if you are using ESP-01 then uncomment the line below to reset the pins to
    // the available pins for SDA, SCL
    // Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    Serial.println(formatDateTime(compiled));

    if (!Rtc.IsDateTimeValid()) 
    {
        // Common Cuases:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing

        Serial.println("RTC lost confidence in the DateTime!");

        // following line sets the RTC to the date & time this sketch was compiled
        // it will also reset the valid flag internally unless the Rtc device is
        // having an issue

        Rtc.SetDateTime(compiled);
    }

    if (!Rtc.GetIsRunning())
    {
    	Serial.println("RTC was not actively running, starting now");
    	Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
    	Serial.println("RTC is older than compile time!  (Updating DateTime)");
    	Rtc.SetDateTime(compiled);
    }
    else if (now > compiled) 
    {
    	Serial.println("RTC is newer than compile time. (this is expected)");
    }
    else if (now == compiled) 
    {
    	Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    Rtc.Enable32kHzPin(false);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 

    #ifdef DEBUG
	//SEtup debug port
	Serial.begin(115200);
	#endif

	// see if the card is present and can be initialized:
	if (!SD.begin(chipSelect)) {
		Serial.println("Card failed, or not present");
		// don't do anything more:
		digitalWrite(ledPin, HIGH);
		return;
	}
		Serial.println("card initialized.");
		doLogging = true;
	}

struct bldcMeasure measuredValues;

#define countof(a) (sizeof(a) / sizeof(a[0]))

// the loop function runs over and over again until power down or reset
void loop() {
  //int len=0;
  //len = ReceiveUartMessage(message);
  //if (len > 0)
  //{
  //  len = PackSendPayload(message, len);
  //  len = 0;
  //}

  if (!Rtc.IsDateTimeValid()) {
    // Common Causes:
    //    1) the battery on the device is low or even missing and the power line was disconnected
    Serial.println("RTC lost confidence in the DateTime!");
  }

  measuredValues.avgMotorCurrent = 50.0;
  measuredValues.avgInputCurrent = 100.00;
  measuredValues.dutyCycleNow = 0.5;
  measuredValues.rpm = 1000;
  measuredValues.inpVoltage = 38.0;
  measuredValues.ampHours = 10000.0;
  measuredValues.ampHoursCharged = 10000.0;
  measuredValues.tachometer = 100;
  measuredValues.tachometerAbs = 500;

  if (doLogging) {
  	if (VescUartGetValue(measuredValues) or 1==1) {
  		Serial.print("Loop: "); Serial.println(count++);
  		SerialPrint(measuredValues);
    }
    else {
    	Serial.println("Failed to get data!");
    }    
	}

	RtcDateTime now = Rtc.GetDateTime();
	RtcTemperature temp = Rtc.GetTemperature();

	String dataString = "";
	dataString += formatDateTime(now);
	dataString + ", ";
	dataString += temp.AsFloat();
	dataString += ", ";
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

String formatDateTime(const RtcDateTime& dt)
{
	char datestring[20];

	snprintf_P(datestring, 
		countof(datestring),
		PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
		dt.Month(),
		dt.Day(),
		dt.Year(),
		dt.Hour(),
		dt.Minute(),
		dt.Second()
	);
	return datestring;
}
