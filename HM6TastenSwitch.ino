#include <InputParser.h>
#include "HM6TastenSwitch.h"

#if defined(ENABLE_COMMANDS)
	InputParser::Commands cmdTab[] PROGMEM = {
		{ 'h', 0, showHelp },
		{ 'p', 0, sendPairing },
		{ 's', 1, sendCmdStr },
		{ 'e', 0, showEEprom },
		{ 'f', 2, writeEEprom },
		{ 'c', 0, clearEEprom },
		{ 't', 0, testConfig },

		{ 'b', 1, buttonSend },
		{ 'a', 0, stayAwake30 },

		{ 'r', 0, resetDevice },
		{ 0 }
	};

	InputParser parser (50, cmdTab);
#endif

//- homematic communication -----------------------------------------------------------------------------------------------
s_jumptable jumptable[] PROGMEM = {												// jump table for HM communication
	{ 0x01, 0x0E, HM_Status_Request },
	{ 0x11, 0x04, HM_Reset_Cmd },
	{ 0xFF, 0xFF, HM_Config_Changed },
	{ 0x0 }
};

AskSin askSin (jumptable, regMcPtr);											// declare class for handling HM communication
Buttons buttons[7];																// declare 7 instances of the button key handler

//- main functions -------------------------------------------------------------
void setup() {
	#if defined(ENABLE_DEBUG) || defined(ENABLE_COMMANDS)
		Serial.begin(57600);														// starting serial messages
	#endif

	// some power savings
//	power_all_disable();														// all devices off
//	power_timer0_enable();														// we need timer0 for delay function
	//power_timer2_enable();													// we need timer2 for PWM
//	power_usart0_enable();														// it's the serial console
	//power_twi_enable();														// i2c interface, not needed yet
//	power_spi_enable();															// enables SPI master
	//power_adc_enable();

	// init HM module
	askSin.init();																// initialize HM module


//	askSin.setDevParams(devParam);

	askSin.statusLed.config(4, 5, stayAwake);									// configure the status led pin
//	askSin.statusLed.config(4, 7, stayAwake);									// configure the status led pin

	askSin.setPowerMode(4);														// power mode for HM device
//	askSin.setPowerMode(0);														// power mode for HM device
	askSin.setConfigEvent();													// reread config
	
	#if defined(ENABLE_COMMANDS)
		// show help screen and config
		showHelp();																// shows help screen on serial console
		showSettings();															// show device settings
	#endif

	askSin.statusLed.set(STATUSLED_2, STATUSLED_MODE_BLINKFAST, 3);

/*
	pinMode(5, OUTPUT);	// red
	pinMode(6, OUTPUT);	// bue
	pinMode(9, OUTPUT);	// green

	analogWrite(5, 0);
	analogWrite(6, 0);
	analogWrite(9, 0);

	dir = 1;
	bright = 0;
*/
}

void loop() {
	// poll functions for serial console, HM module and button key handler

	#if defined(ENABLE_COMMANDS)
		parser.poll();															// handle serial input from console
	#endif

	askSin.poll();																// HOMEMATIC task scheduler
	askSin.statusLed.poll();													// LED tast scheduler (moved from HM::poll)
	buttons->poll();															// key handler poll

	#if defined(ENABLE_DEBUG)
		if (nTimer < millis()) {
			nTimer = millis() + 30000;											// jump in every 30 seconds
			Serial << "t:" << (millis()/1000) << '\n';							// time stamp

			if (askSin.powr.nxtTO < 100) {
				stayAwake(100);													// wait 100ms to get the output in console
			}
		}
	#endif

/*

 	if (bright > 254) {
		dir = -1;
	} else if (bright < 2) {
		dir = 1;
	}

	bright = bright + dir;
	analogWrite(5, bright);
	analogWrite(6, bright);
	analogWrite(9, bright);

	delay(20);
*/
}

//- hardware functions functions -----------------------------------------------
/**
 * The key handler
 */
void buttonState(uint8_t idx, uint8_t state, uint8_t pressRepeatCount) {
	uint8_t  lowBat = 0;
	uint16_t batteryVoltage = 0;

	/*
	 * possible callback events of this function:
	 *   0 - short key press
	 *   1 - double or multiple short key press
	 *   2 - long key press
	 *   3 - repeated long key press (hold down key)
	 *   4 - end of long key press (release key)
	 *   5 - double long key press

	 *   6 - single key press timeout (only if timeOutMultiKeyPress = 0)
	 *   7 - time out for double long
	 *   8 - time out for double short ???
	 *
	 * 255 - key press, for stay awake issues
	 */

//	#if defined(ENABLE_DEBUG)
		Serial << "i:" << idx << ", s:" << state << ", r: " << pressRepeatCount << '\n';	// some debug message
//	#endif

	// channel 0 (config key)
	if (idx == 0) {
		if (state == 0) {														// short key press
			askSin.startPairing();												// send pairing string

		} else if (state == 2) {												// long key press (maybe, this sould only trigger if config mode set)
			askSin.statusLed.set(STATUSLED_2, STATUSLED_MODE_BLINKSFAST);		// long key press could mean, you like to go for reseting the device
			stayAwake(15100);

		} else if (state == 7 || state == 8) {
			askSin.statusLed.set(STATUSLED_2, STATUSLED_MODE_OFF);				// time out for double long, stop slow blinking

		} else if (state == 5) {
			askSin.reset();														// double long key press, reset the device
			stayAwake(3000);
			askSin.statusLed.set(STATUSLED_2, STATUSLED_MODE_ON);
		}
	}

	// channel 1 - 6
	if ((idx >= 1) && (idx <= 6)) {
		batteryVoltage = getBatteryVoltage(AVR_BANDGAP_VOLTAGE);
		lowBat = (batteryVoltage < BATTERY_MIN_VOLTAGE) ? 1 : 0;

		#if defined(ENABLE_DEBUG)
			Serial << "batteryVoltage: " << batteryVoltage << ", lowBat: " << lowBat << '\n';
		#endif

		if (regMC.ch1.l1.longPress > 0) {
			// double keypress for channel 1 was defined,
			// so idx was mapped to the count of repeated key press
			idx = (pressRepeatCount >=1 && pressRepeatCount <= 6) ? pressRepeatCount : idx;
		}

		if ((state == 0) || (state == 1)) {										// short key or multi short key press detected
			askSin.sendPeerREMOTE(idx, 0, lowBat);
		} else if ((state == 2) || (state == 3)) {								// long or repeated long key press detected
			askSin.sendPeerREMOTE(idx, 1, lowBat);
		} else if (state == 4) {												// end of long or repeated long key press detected
			askSin.sendPeerREMOTE(idx, 2, lowBat);
		}
	}
}

//- HM functions ---------------------------------------------------------------
void HM_Status_Request(uint8_t cnl, uint8_t *data, uint8_t len) {
	// message from master to client while requesting the channel specific status
	// client has to send an INFO_ACTUATOR_MESSAGE with the current status of the requested channel
	// there is no payload; data[0] could be ignored
	#if defined(ENABLE_DEBUG)
		Serial << F("\nStatus_Request; cnl: ") << pHex(cnl) << F(", data: ") << pHex(data,len) << "\n\n";
	#endif

	// user code here...

	// no user code needed, send only content from dimVal[0]
	askSin.sendInfoActuatorStatus(cnl, 0, 0);
}

void HM_Reset_Cmd(uint8_t cnl, uint8_t *data, uint8_t len) {
	#if defined(ENABLE_DEBUG)
		Serial << F("\nReset_Cmd; cnl: ") << pHex(cnl) << F(", data: ") << pHex(data,len) << "\n\n";
	#endif

	askSin.send_ACK();															// send an ACK
	if (cnl == 0) askSin.reset();												// do a reset only if channel is 0
}

void HM_Config_Changed(uint8_t cnl, uint8_t *data, uint8_t len) {
	// some debug message
	#if defined(ENABLE_DEBUG)
		Serial << "ch1: long:" << 300+(regMC.ch1.l1.longPress*100) << ", dbl:" << (regMC.ch1.l1.dblPress*100) << '\n';
	#endif

	// button 0 for channel 0 for send pairing string, and double press for reseting device config
	buttons[0].config(0, 1, 8, 0, 5000, 5000, buttonState, stayAwake);
	buttons[1].config(1, (devDef.nbrChannels - 1), A0, (regMC.ch1.l1.dblPress * 100), 300 + (regMC.ch1.l1.longPress * 100), 1000, buttonState, stayAwake);
//	buttons[2].config(2, 1, A0, (regMC.ch2.l1.dblPress * 100), 300 + (regMC.ch2.l1.longPress * 100), buttonState, stayAwake);

}

#if defined(ENABLE_COMMANDS)
	//- config functions -----------------------------------------------------------
	void sendCmdStr() {															// reads a sndStr from console and put it in the send queue
		memcpy(askSin.send.data,parser.buffer,parser.count());							// take over the parsed byte data

		#if defined(ENABLE_DEBUG)
			Serial << F("s: ") << pHexL(askSin.send.data, askSin.send.data[0]+1) << '\n';	// some debug string
		#endif

		askSin.send_out();														// fire to send routine
	}

	void sendPairing() {														// send the first pairing request
		askSin.startPairing();
	}

	void showEEprom() {
		uint16_t start, len;
		uint8_t buf[32];

		parser >> start >> len;
		if (len == 0) len = E2END - start;

		Serial << F("EEPROM listing, start: ") << start << F(", len: ") << len << '\n';

		for (uint16_t i = start; i < len; i+=32) {
			eeprom_read_block(buf,(void*)i,32);
			Serial << pHex(i>>8) << pHex(i&0xFF) << F("   ") << pHex(buf,32) << '\n';
		}
	}

	void writeEEprom() {
		uint16_t addr;
		uint8_t data;

		for (uint8_t i = 0; i < parser.count(); i+=3) {
			parser >> addr >> data;
			eeprom_write_byte((uint8_t*)addr,data);
			Serial << F("Write EEprom, Address: ") << pHex(addr>>8) << pHex(addr&0xFF) << F(", Data: ") << pHex(data) << '\n';
		}
	}

	void clearEEprom() {														// clear settings
		Serial << F("Clear EEprom, size: ") << E2END+1 << F(" bytes") << '\n';
		for (uint16_t i = 0; i <= E2END; i++) {
			eeprom_write_byte((uint8_t*)i,0);
		}
		Serial << F("done") << '\n';
	}

	void showHelp() {															// display help on serial console
		showPGMText(helptext1);
	}

	void showSettings() {														// shows device settings on serial console
		askSin.printSettings();														// print settings of own HM device
		Serial << F("FreeMem: ") << freeMemory() << F(" byte's\n");					// displays the free memory
	}

	void testConfig() {															// shows the complete configuration of slice table and peer database
		askSin.printConfig();														// prints register and peer config
	}

	void buttonSend() {
		uint8_t cnl, lpr;
		parser >> cnl >> lpr;

		Serial << "button press, cnl: " << cnl << ", long press: " << lpr << '\n';	// some debug message
		askSin.sendPeerREMOTE(cnl,lpr,0);											// parameter: button/channel, long press, battery
	}

	/**
	 * Stay awake for 30 seconds
	 */
	void stayAwake30() {
		stayAwake(30000);
	}

	void resetDevice() {
		Serial << F("reset device, clear eeprom...\n");
		askSin.reset();
		Serial << F("reset done\n");
	}
#endif


/**
 * Stay awake for n milliseconds.
 * If no time given, we stay awake for 30 seconds
 *
 */
void stayAwake(uint32_t xMillis) {
	askSin.stayAwake(xMillis);
}
