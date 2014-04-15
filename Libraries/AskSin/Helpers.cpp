#include "Helpers.h"

//- -----------------------------------------------------------------------------------------------------------------------
//- additional helpers ----------------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

extern uint16_t __bss_end, _pHeap_start;
extern void *__brkval;

uint16_t freeMemory() {															// shows free memory
	uint16_t free_memory;

	if((uint16_t)__brkval == 0)
	free_memory = ((uint16_t)&free_memory) - ((uint16_t)&__bss_end);
	else
	free_memory = ((uint16_t)&free_memory) - ((uint16_t)__brkval);

	return free_memory;
}

uint32_t byteTimeCvt(uint8_t tTime) {
	const uint16_t c[8] = {1,10,50,100,600,3000,6000,36000};
	return (uint32_t)(tTime & 0x1f)*c[tTime >> 5]*100;
}

uint32_t intTimeCvt(uint16_t iTime) {
	if (iTime == 0) return 0;

	uint8_t tByte;
	if ((iTime & 0x1F) != 0) {
		tByte = 2;
		for (uint8_t i = 1; i < (iTime & 0x1F); i++) tByte *= 2;
	} else tByte = 1;

	return (uint32_t)tByte*(iTime>>5)*100;
}

/**
 * get battery voltage
 * Mesure AVCC again the the internal bandgap reference
 *
 *	REFS1 REFS0          --> internal bandgap reference
 *	MUX3 MUX2 MUX1 MUX0  --> 1110 1.1V (VBG) (for instance Atmega 328p)
 */
uint16_t getBatteryVoltage(uint32_t bandgapVoltage) {
	uint16_t adcValue = 0;
	uint16_t batteryVoltage;

	ADMUX = (0<<REFS1) | (1<<REFS0) | (0<<ADLAR) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (0<<MUX0);

	ADCSRA = (1<<ADEN) | (1<<ADPS2) | (1<<ADPS1);								// Enable ADC and set ADC prescaler
	for(int i = 0; i < BATTERY_NUM_MESS_ADC + BATTERY_DUMMY_NUM_MESS_ADC; i++) {
		ADCSRA |= (1 << ADSC);													// start conversion
		while (ADCSRA & (1 << ADSC)) {}											// wait for conversion complete

		if (i >= BATTERY_DUMMY_NUM_MESS_ADC) {									// we discard the first dummy measurements
			adcValue += ADCW;
		}
	}
	ADCSRA &= ~(1 << ADEN);														// ADC disable

	adcValue = adcValue / BATTERY_NUM_MESS_ADC;
	batteryVoltage = (bandgapVoltage * 1023) / adcValue;						// calculate battery voltage in mV

	return batteryVoltage;
}

void showPGMText(PGM_P s) {
	char c;
	while (( c = pgm_read_byte(s++)) != 0) Serial << c;
	Serial << '\n';
}
