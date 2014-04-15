#ifndef BUTTONS_H
	#define BUTTONS_H

	#if defined(ARDUINO) && ARDUINO >= 100
		#include "Arduino.h"
	#else
		#include "WProgram.h"
	#endif

	#include <Serial.h>

	#define BTN_DEBOUNCE_MILLISEC 30											// time for debouncing (ms)
	#define BTN_LONG_REPEAT_DELAY 250											// delay between repeated commands at long key press (ms)

	/**
	 * Configure the buttons
	 *
	 * @param	the assigned channel
	 * @param	maximal count multi key presses
	 * @param	the pin where the button was connected
	 * @param	timeout within recognize a multiple key press event
	 * @param	timeout within recognize a long key press event if button held down
	 * @param	timeout within recognize a double long key press (long key press after a previous long key press)
	 * @param	pointer to callback function was called on specific key event
	 * @param	pointer to awake function
	 */
	class Buttons {
		public://--------------------------------------------------------------------------------------------------------------
		uint8_t  keyStateFlag :1;												// was there a change happened in key state
		uint8_t  curKeyState :1;												// current status of the button
		uint32_t nextCheckTime;													// stores the next time to check, otherwise we would check every cycle

		void config(uint8_t Cnl, uint8_t MaxMkp, uint8_t Pin, uint16_t TimeOutShort, uint16_t TimeOutLong, uint16_t TimeOutLongDdbl, void tCallBack(uint8_t, uint8_t, uint8_t), void tStayAwakeFn(uint32_t) = NULL);
		void poll(void);

		private://-------------------------------------------------------------------------------------------------------------
		uint16_t timeOutMultiKeyPress;											// timeout within multiple key presses should recognized
		uint16_t timeOutLongKeyPress;											// time key should be pressed to be recognized as a long key press
		uint16_t timeOutSecondLongKeyPress;										// maximum time for a second long key press
		void (*callBack)(uint8_t, uint8_t, uint8_t);							// call back address for key display
		void (*stayAwakeFn)(uint32_t);											// call back address for stayAwakeFn

		uint8_t  maxMultiKeyPress: 4;
		uint8_t  pin: 4;															// the pin where the button is connected to
		uint8_t  idx: 4;

		uint8_t  lastKeyState :1;												// last key state
		uint8_t  keyPressCount;													// remember the count of repeated key press
		uint8_t  keyLongPressed :1;												// remember last long key press to indicate a double key press
		uint8_t  repeatedlongPress :1;											// remember long key press to indicate repeated long, or when long was released
		uint8_t  repeatedlongPressProcessed :1;									// remember repeated long key press was processed
		uint8_t  callbackId;
		uint32_t keyTime;														// stores time when the key was pressed or released
		uint32_t lastStayAwakeTime;												// remember last stay awake time

		void   poll_btn(void);													// internal polling function for all inxtStatnces
	};

	//- interrupt handling
	#define maxInt 10
	struct s_pci {
		uint8_t	 nbr;
		uint16_t idx[maxInt];
		Buttons *ptr[maxInt];
		uint8_t *pAddr[3];
		uint8_t  lPort[3];
	};
	void pcInt(uint8_t iPort);
	ISR( WDT_vect );
	ISR(PCINT0_vect);
	ISR(PCINT1_vect);
	ISR(PCINT2_vect);

#endif
