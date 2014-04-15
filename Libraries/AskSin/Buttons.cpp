#include "Buttons.h"

s_pci pci;

//- -----------------------------------------------------------------------------------------------------------------------
//- button key functions ---------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------

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
void Buttons::config(uint8_t Cnl, uint8_t MaxMkp, uint8_t Pin, uint16_t TimeOutShort, uint16_t TimeOutLong, uint16_t TimeOutLongDdbl, void tCallBack(uint8_t, uint8_t, uint8_t), void tStayAwakeFn(uint32_t)) {

	// settings while setup
	maxMultiKeyPress = MaxMkp;
	pinMode(Pin, INPUT_PULLUP);													// setting the pin to input mode
	timeOutMultiKeyPress = TimeOutShort;										// timeout within multiple key presses should recognized
	timeOutLongKeyPress  = TimeOutLong;											// time key should be pressed to be recognized as a long key press
	timeOutSecondLongKeyPress = TimeOutLongDdbl;								// maximum time for a second long key press
	callBack = tCallBack;														// call back address for button state display
	stayAwakeFn = tStayAwakeFn;

	lastStayAwakeTime = 0;														// remember last stay awake time

	// default settings
	pin = Pin;
	keyStateFlag = 0;															// no need for the poll routine at the moment
	curKeyState = 1;															// active low, means last state should be active to get the next change
	lastKeyState = 1;
	repeatedlongPress = 0;														// remember long key press to indicate repeated long, or when long was released
	repeatedlongPressProcessed = 0;

	// setting the interrupt and port mask
	// http://www.kriwanek.de/arduino/grundlagen/183-mehrere-pin-change-interrupts-verwenden.html
	volatile uint8_t* pcicr = digitalPinToPCICR(Pin);
	*pcicr |= (1 << digitalPinToPCICRbit(Pin));
	volatile uint8_t* pcmsk = digitalPinToPCMSK(Pin);
	*pcmsk |= (1 << digitalPinToPCMSKbit(Pin));

	// load the respective pin register to mask out in interrupt routine
	uint8_t pinPort = digitalPinToPort(Pin)-2;									// get the respective port to the given pin
	pci.lPort[pinPort] = *portInputRegister(pinPort+2) & *pcmsk;				// store the port input byte for later comparison
	pci.pAddr[pinPort] = (uint8_t*)portInputRegister(pinPort+2);				// store the address of the port input register to avoid PGM read in the interrupt

	// set index and call back address for interrupt handling
	idx = Cnl;																	// set the index in the interrupt array
	pci.ptr[idx] = this;														// set the call back address
	pci.idx[idx] = (pinPort << 8) + (1 << digitalPinToPCMSKbit(Pin));			// calculate and set the index number for faster finding in the interrupt routine

//	Serial << "pin:" << tPin << ", idx:" << pci.idx[pci.nbr] << ", prt:" << pinPort << ", msk:" << (1 << digitalPinToPCMSKbit(tPin)) << '\n';
}

void Buttons::poll() {
	for (uint8_t i = 0; i < maxInt; i++) {
		if (pci.ptr[i]) {
			Buttons *p = pci.ptr[i];
			p->poll_btn();
		}
	}
}

/**
 * possible callback events of this function:
 *   0 - short key press
 *   1 - double or multiple short key press
 *   2 - long key press
 *   3 - repeated long key press (hold down key)
 *   4 - end of long key press (release key)
 *   5 - double long key press

 *   6 - single key press timeout (only if timeOutMultiKeyPress = 0)
 *   7 - time out for double long
 *   8 - reset double long key press
 *
 * 255 - key press, for stay awake issues
 */
void Buttons::poll_btn() {

	if (keyStateFlag == 0) return;												// no need for do any thing
	if (nextCheckTime > millis()) return;										// there are not to do

	if (curKeyState == 0 && lastKeyState == 1 && keyPressCount < maxMultiKeyPress) {
		keyPressCount++;														// key press counter increments each key press
	}

	callbackId = 254;

	if (curKeyState == 1 && lastKeyState == 1) {								// timeout (no key pressed anymore)

		if (keyPressCount > 0) {
			if (timeOutMultiKeyPress > 0) {										// timeout for short press reached
				callbackId = 1;													// multi short key raised after timeOutMultiKeyPress
			} else {
				callbackId = 6;													// raise timeout for double short
			}

			keyStateFlag = 0;													// no need for checking again
			keyLongPressed = 0;
		}

		if (keyLongPressed && (keyTime + timeOutSecondLongKeyPress <= millis())) {	// timeout for double long reached
			callbackId = 7;														// raise timeout for double long
			keyLongPressed = 0;													// no need for check against
		}

		if (keyLongPressed) nextCheckTime = millis() + timeOutSecondLongKeyPress; // set the next check time

	} else if (curKeyState == 0 && lastKeyState == 1) {							// key is pressed just now
		keyTime = millis();														// store timer
		nextCheckTime = keyTime + timeOutLongKeyPress ;							// set next timeout
		lastKeyState = curKeyState;												// remember last key state
		keyStateFlag = 1;														// next check needed
		callbackId = 255;

	} else if (curKeyState == 1 && lastKeyState == 0) {							// key is released just now

		if (repeatedlongPress) {												// coming from a long key press
			repeatedlongPress = 0;												// could not be repeated any more
			callbackId = 4;														// end of long key press

		} else if (keyLongPressed) {
			if (repeatedlongPressProcessed == 0) {
				callbackId = 8;													// reset double long key press
			}
			repeatedlongPressProcessed = 0;

		} else if (timeOutMultiKeyPress == 0 && keyPressCount == 1) {			// short single key press detect
			callbackId = 0;
		}

		if (keyPressCount > 0 && timeOutMultiKeyPress > 0) {
			nextCheckTime = millis() + timeOutMultiKeyPress;					// set the next check time
		}

		keyLongPressed = 0;
		keyStateFlag = 1;														// next check needed
		lastKeyState = curKeyState;												// remember last key state
		keyTime = millis();														// set variable to measure against

	} else if ((curKeyState == 0) && (lastKeyState == 0)) {
		if (repeatedlongPress) {												// repeated long detect
			nextCheckTime = millis() + BTN_LONG_REPEAT_DELAY;					// set next timeout
			callbackId = 3;														// repeated long key press

		} else if (keyLongPressed) {											// long was set last time, should be a double now
			repeatedlongPress = 0;												// could not be a repeated any more
			keyStateFlag = 0;													// no need for jump in again
			repeatedlongPressProcessed = 1;
			callbackId = 5;														// double long key press
			
		} else {
			keyLongPressed = 1;													// next time it could be a double
			repeatedlongPress = 1;												// or a repeated long
			nextCheckTime = millis() + BTN_LONG_REPEAT_DELAY;					// set next timeout
			callbackId = 2;														// long key press
		}
	}

	if (callbackId != 254) {
		int stayAwakeTime = nextCheckTime - millis() + 50;
		if (lastStayAwakeTime <= stayAwakeTime) {
			lastStayAwakeTime = stayAwakeTime;
			Serial << "stayAwakeTime: " << stayAwakeTime << '\n';

			stayAwakeFn(stayAwakeTime);
		}

		callBack(idx, callbackId, keyPressCount);
		if (callbackId == 1 || callbackId == 4 || callbackId == 6) {
			keyPressCount = 0;													// reset the key press counter
			lastStayAwakeTime = 0;
		}
	}
}

//- interrupt handling
void pcInt(uint8_t iPort) {
	cli();																		// all interrupts off

	// getting the PCMASK for filtering by interrupt mask
	uint8_t pcMskByte;
	if (iPort == 0) pcMskByte = PCMSK0;
	if (iPort == 1) pcMskByte = PCMSK1;
	if (iPort == 2) pcMskByte = PCMSK2;

	// find the changed pin by getting the pin states for the indicated port, comparing with the stored byte of the port and setting the port byte for the next try
	uint8_t cur = *pci.pAddr[iPort] & pcMskByte;								// get the input byte
	uint8_t msk = pci.lPort[iPort]^cur;											// mask out the changes
	if (!msk) { sei(); return; }												// end while nothing had changed

	//Serial.print("cur:"); Serial.print(cur); Serial.print(", lst:"); Serial.print(pci.lPort[iPort]);
	//	Serial.print(", msk:"); Serial.print(msk); Serial.print(", mbt:"); Serial.println(pcMskByte);
	pci.lPort[iPort] = cur;														// store the latest port reading

	// finding the respective inxtStatnce of Buttons by searching for the changed bit
	uint16_t tFnd = (iPort << 8) + msk;											// construct search mask
	for (uint8_t i = 0; i < maxInt; i++) {
		if (tFnd == pci.idx[i]) {												// found; write flag and time in the respective button key class
			Buttons *p = pci.ptr[i];
			p->curKeyState = (cur & msk) ? 1 : 0;								// setting the pin status
			p->nextCheckTime = millis() + BTN_DEBOUNCE_MILLISEC;						// for debouncing
			p->keyStateFlag = 1;												// something to do
			break;																// no need to step through all inxtStatnces
		}
	}
	sei();																		// interrupts on again
}

ISR(PCINT0_vect) {
	pcInt(0);
}

ISR(PCINT1_vect) {
	pcInt(1);
}

ISR(PCINT2_vect) {
	pcInt(2);
}
