#ifndef HELPERS_H
	#define HELPERS_H

	#if defined(ARDUINO) && ARDUINO >= 100
		#include "Arduino.h"
	#else
		#include "WProgram.h"
	#endif

	#include <Serial.h>

	#define BATTERY_NUM_MESS_ADC       64
	#define BATTERY_DUMMY_NUM_MESS_ADC 10

	//- -----------------------------------------------------------------------------------------------------------------------
	//- additional helpers ----------------------------------------------------------------------------------------------------
	//- -----------------------------------------------------------------------------------------------------------------------
	uint16_t freeMemory(void);
	uint32_t byteTimeCvt(uint8_t tTime);
	uint8_t  int2ByteTimeCvt(uint16_t tTime);
	uint32_t intTimeCvt(uint16_t iTime);
	uint16_t getBatteryVoltage(uint32_t bandgapVoltage);
	void showPGMText(PGM_P s);
#endif
