/*
Copyright 2015 - 2017 Andreas Chaitidis Andreas.Chaitidis@gmail.com

This program is free software : you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.If not, see <http://www.gnu.org/licenses/>.
*/
#include "Arduino.h"
#include "VescUart.h"
#include "buffer.h"
#include "crc.h"
#include "HardwareSerial.h"
#include "Stream.h"

VescUart::VescUart(Stream *refSer, Stream *refDebugSer) {
	_pSerialPort = refSer;
	_pSerialPortDebug = refDebugSer;
}

int VescUart::ReceiveUartMessage(uint8_t* payloadReceived) {

	//Messages <= 255 start with 2. 2nd byte is length
	//Messages >255 start with 3. 2nd and 3rd byte is length combined with 1st >>8 and then &0xFF

	int counter = 0;
	int endMessage = 256;
	bool messageRead = false;
	uint8_t messageReceived[256];
	int lenPayload = 0;

	while (_pSerialPort->available()) {

		messageReceived[counter++] = _pSerialPort->read();

		if (counter == 2) {//case if state of 'counter' with last read 1

			switch (messageReceived[0])
			{
			case 2:
				endMessage = messageReceived[1] + 5; //Payload size + 2 for sice + 3 for SRC and End.
				lenPayload = messageReceived[1];
				break;
			case 3:
				//ToDo: Add Message Handling > 255 (starting with 3)
				break;
			default:
				break;
			}

		}
		if (counter >= sizeof(messageReceived))
		{
			break;
		}

		if (counter == endMessage && messageReceived[endMessage - 1] == 3) {//+1: Because of counter++ state of 'counter' with last read = "endMessage"
			messageReceived[endMessage] = 0;
// #ifdef DEBUG
// 			_pSerialPortDebug->println("End of message reached!");
// #endif
			messageRead = true;
			break; //Exit if end of message is reached, even if there is still more data in buffer.
		}
	}
	bool unpacked = false;
	if (messageRead) {
		unpacked = UnpackPayload(messageReceived, endMessage, payloadReceived, messageReceived[1]);
	}
	if (unpacked)
	{
		return lenPayload; //Message was read

	}
	else {
		return 0; //No Message Read
	}
}

bool VescUart::UnpackPayload(uint8_t* message, int lenMes, uint8_t* payload, int lenPay) {
	uint16_t crcMessage = 0;
	uint16_t crcPayload = 0;
	//Rebuild src:
	crcMessage = message[lenMes - 3] << 8;
	crcMessage &= 0xFF00;
	crcMessage += message[lenMes - 2];
// #ifdef DEBUG
// 	_pSerialPortDebug->print("SRC received: "); _pSerialPortDebug->println(crcMessage);
// #endif // DEBUG

	//Extract payload:
	memcpy(payload, &message[2], message[1]);

	crcPayload = crc16(payload, message[1]);
// #ifdef DEBUG
// 	_pSerialPortDebug->print("SRC calc: "); _pSerialPortDebug->println(crcPayload);
// #endif
	if (crcPayload == crcMessage)
	{
// #ifdef DEBUG
// 		_pSerialPortDebug->print("Received: "); SerialPrint(message, lenMes); _pSerialPortDebug->println();
// 		_pSerialPortDebug->print("Payload :      "); SerialPrint(payload, message[1] - 1); _pSerialPortDebug->println();
// #endif // DEBUG

		return true;
	}
	else
	{
		return false;
	}
}

int VescUart::PackSendPayload(uint8_t* payload, int lenPay) {
	uint16_t crcPayload = crc16(payload, lenPay);
	int count = 0;
	uint8_t messageSend[256];

	if (lenPay <= 256)
	{
		messageSend[count++] = 2;
		messageSend[count++] = lenPay;
	}
	else
	{
		messageSend[count++] = 3;
		messageSend[count++] = (uint8_t)(lenPay >> 8);
		messageSend[count++] = (uint8_t)(lenPay & 0xFF);
	}
	memcpy(&messageSend[count], payload, lenPay);

	count += lenPay;
	messageSend[count++] = (uint8_t)(crcPayload >> 8);
	messageSend[count++] = (uint8_t)(crcPayload & 0xFF);
	messageSend[count++] = 3;
	messageSend[count] = NULL;

	// _pSerialPortDebug->print("UART package send: "); SerialPrint(messageSend, count);
	//Sending package
	_pSerialPort->write(messageSend, count);


	//Returns number of send bytes
	return count;
}


bool VescUart::ProcessReadPacket(uint8_t* message, bldcMeasure& values, int len) {
	COMM_PACKET_ID packetId;
	int32_t ind = 0;

	packetId = (COMM_PACKET_ID)message[0];
	message++;//Eliminates the message id
	len--;

	switch (packetId)
	{
	case COMM_GET_VALUES:
		ind = 14; //Skipped the first 14 bit.
		values.avgMotorCurrent = buffer_get_float32(message, 100.0, &ind);
		values.avgInputCurrent = buffer_get_float32(message, 100.0, &ind);
		values.dutyCycleNow = buffer_get_float16(message, 1000.0, &ind);
		values.rpm = buffer_get_int32(message, &ind);
		values.inpVoltage = buffer_get_float16(message, 10.0, &ind);
		values.ampHours = buffer_get_float32(message, 10000.0, &ind);
		values.ampHoursCharged = buffer_get_float32(message, 10000.0, &ind);
		ind += 8; //Skip 9 bit
		values.tachometer = buffer_get_int32(message, &ind);
		values.tachometerAbs = buffer_get_int32(message, &ind);
		return true;
		break;

	default:
		return false;
		break;
	}

}

bool VescUart::VescUartGetValue(bldcMeasure& values) {
	uint8_t command[1] = { COMM_GET_VALUES };
	uint8_t payload[256];
	PackSendPayload(command, 1);
	delay(100); //needed, otherwise data is not read
	int lenPayload = ReceiveUartMessage(payload);
	// _pSerialPortDebug->print("len payload"); _pSerialPort->println(lenPayload);
	if (lenPayload > 0) {
		bool read = ProcessReadPacket(payload, values, lenPayload); //returns true if sucessfull
		return read;
	}
	else
	{
		return false;
	}
}

void VescUart::VescUartSetCurrent(float current) {
	int32_t index = 0;
	uint8_t payload[5];

	payload[index++] = COMM_SET_CURRENT ;
	buffer_append_int32(payload, (int32_t)(current * 1000), &index);
	PackSendPayload(payload, 5);
}

void VescUart::VescUartSetCurrentBrake(float brakeCurrent) {
	int32_t index = 0;
	uint8_t payload[5];

	payload[index++] = COMM_SET_CURRENT_BRAKE;
	buffer_append_int32(payload, (int32_t)(brakeCurrent * 1000), &index);
	PackSendPayload(payload, 5);

}

void VescUart::VescUartSetNunchukValues(remotePackage& data) {
	int32_t ind = 0;
	uint8_t payload[11];
	payload[ind++] = COMM_SET_CHUCK_DATA;
	payload[ind++] = data.valXJoy;
	payload[ind++] = data.valYJoy;
	buffer_append_bool(payload, data.valLowerButton, &ind);
	buffer_append_bool(payload, data.valUpperButton, &ind);
	//Acceleration Data. Not used, Int16 (2 byte)
	payload[ind++] = 0;
	payload[ind++] = 0;
	payload[ind++] = 0;
	payload[ind++] = 0;
	payload[ind++] = 0;
	payload[ind++] = 0;

// #ifdef DEBUG
// 	_pSerialPortDebug->println("Data reached at VescUartSetNunchuckValues:");
// 	_pSerialPortDebug->print("valXJoy = "); _pSerialPortDebug->print(data.valXJoy); _pSerialPortDebug->print(" valYJoy = "); _pSerialPortDebug->println(data.valYJoy);
// 	_pSerialPortDebug->print("LowerButton = "); _pSerialPortDebug->print(data.valLowerButton); _pSerialPortDebug->print(" UpperButton = "); _pSerialPortDebug->println(data.valUpperButton);
// #endif

	PackSendPayload(payload, 11);
}

void VescUart::SerialPrint(uint8_t* data, int len) {

		// _pSerialPortDebug->print("Data to display: "); _pSerialPortDebug->println(sizeof(data));

	for (int i = 0; i <= len; i++)
	{
		_pSerialPortDebug->print(data[i]);
		_pSerialPortDebug->print(" ");
	}
	_pSerialPortDebug->println(";");
}


void VescUart::SerialPrint(const bldcMeasure& values) {
	_pSerialPortDebug->print("avgMotorCurrent: "); _pSerialPortDebug->println(values.avgMotorCurrent);
	_pSerialPortDebug->print("avgInputCurrent: "); _pSerialPortDebug->println(values.avgInputCurrent);
	_pSerialPortDebug->print("dutyCycleNow: "); _pSerialPortDebug->println(values.dutyCycleNow);
	_pSerialPortDebug->print("rpm: "); _pSerialPortDebug->println(values.rpm);
	_pSerialPortDebug->print("inputVoltage: "); _pSerialPortDebug->println(values.inpVoltage);
	_pSerialPortDebug->print("ampHours: "); _pSerialPortDebug->println(values.ampHours);
	_pSerialPortDebug->print("ampHoursCharges: "); _pSerialPortDebug->println(values.ampHoursCharged);
	_pSerialPortDebug->print("tachometer: "); _pSerialPortDebug->println(values.tachometer);
	_pSerialPortDebug->print("tachometerAbs: "); _pSerialPortDebug->println(values.tachometerAbs);
}

