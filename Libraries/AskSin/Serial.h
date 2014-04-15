#ifndef SERIAL_H
	#define SERIAL_H

	#if defined(ARDUINO) && ARDUINO >= 100
		#include "Arduino.h"
	#else
		#include "WProgram.h"
	#endif

	template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; }

	char pHex(uint8_t val);
	char pHex(uint8_t *buf, uint8_t len);
	char pHexL(uint8_t *buf, uint8_t len);
	char pTime(void);

#endif
