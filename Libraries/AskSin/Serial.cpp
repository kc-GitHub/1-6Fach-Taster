#include "Serial.h"

//- serial print functions
char pHex(uint8_t val) {
	const char hexDigits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	Serial << hexDigits[val >> 4] << hexDigits[val & 0xF];
	return 0;
}

char pHex(uint8_t *buf, uint8_t len) {
	for (uint8_t i=0; i<len; i++) {
		pHex(buf[i]);
		if(i+1 < len) Serial << " ";
	}
	return 0;
}

char pHexL(uint8_t *buf, uint8_t len) {
	pHex(buf,len);
	Serial << F(" (l:") << len << F(")");
	return 0;
}

char pTime(void) {
	Serial << F("(") << millis() << F(")\n");
	return 0;
}
