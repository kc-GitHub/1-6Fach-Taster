#include "Actors.h"

s_prl prl;

//- -----------------------------------------------------------------------------------------------------------------------
//- relay functions -------------------------------------------------------------------------------------------------------
//- -----------------------------------------------------------------------------------------------------------------------
// public function for setting the module
void RL::config(uint8_t cnl, uint8_t type, uint8_t pinOn1, uint8_t pinOn2, uint8_t pinOff1, uint8_t pinOff2) {
	// store config settings in class
	hwType = type;																// 0 indicates a monostable, 1 a bistable relay
	hwPin[0] = pinOn1;															// first 2 bytes for on, second two for off
	hwPin[1] = pinOn2;
	hwPin[2] = pinOff1;
	hwPin[3] = pinOff2;

	// set output pins
	for (uint8_t i = 0; i < 4; i++) {											// set output pins
		if (hwPin[i] > 0) {														// only if we have a valid pin
			pinMode(hwPin[i], OUTPUT);											// set to output
			digitalWrite(hwPin[i],0);											// set port to low
		}
	}

	prl.ptr[prl.nbr++] = this;													// register inxtStatnce in struct
	cnlAss = cnl;																// stores the channel for the current instance
	curStat = 6;																// set relay status to off
	adjRly(0);																	// set relay to a defined status
}
void RL::setCallBack(void msgCallBack(uint8_t, uint8_t, uint8_t), AskSin *statCallBack, uint8_t minDelay, uint8_t randomDelay) {
	mDel = minDelay;															// remember minimum delay for sending the status
	rDel = (randomDelay)?randomDelay:1;											// remember random delay for sending the status
	cbS = statCallBack;															// call back address for sending status and ACK
	cbM = msgCallBack;															// remember the address
	//cbsTme = millis() + ((uint32_t)mDel*1000) + random(((uint32_t)rDel*1000)); // set the timer for sending the status
}

// public functions for triggering some action
void RL::trigger11(uint8_t val, uint8_t *rampTime, uint8_t *duraTime) {
	// {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}

	rTime = (uint16_t)rampTime[0]<<8 | (uint16_t)rampTime[1];					// store ramp time
	dTime = (duraTime)?((uint16_t)duraTime[0]<<8 | (uint16_t)duraTime[1]):0;	// duration time if given

	if (rTime) nxtStat = (val == 0)?4:1;										// set next status
	else nxtStat = (val == 0)?6:3;

	lastTrig = 11;																// remember the trigger
	rlyTime = millis();															// changed some timers, activate poll function
	cbS->sendACKStatus(cnlAss,val,((nxtStat==1)||(nxtStat==4))?0x40:0);			// send an status ACK

	#if defined(ENABLE_ACTORS_DEBUG)											// some debug message
		Serial << F("RL:trigger11, val:") << val << F(", nxtS:") << nxtStat << F(", rampT:") << rTime << F(", duraT:") << dTime << '\n';
	#endif
}
void RL::trigger41(uint8_t lngIn, uint8_t val, void *plist3) {
	lastTrig = 41;																// set trigger
	rlyTime = millis();															// changed some timers, activate poll function
}
void RL::trigger40(uint8_t lngIn, uint8_t cnt, void *plist3) {
	srly = (s_srly*)plist3;														// copy list3 to pointer
	static uint8_t rCnt;														// to identify multi execute

	// check for repeated message
	if ((lngIn) && (srly->lgMultiExec == 0) && (cnt == rCnt)) return;			// trigger was long
	if ((lngIn == 0) && (cnt == rCnt)) return;									// repeated instruction
	rCnt = cnt;																	// remember message counter

	// fill the respective variables
	uint8_t actTp = (lngIn)?srly->lgActionType:srly->shActionType;				// get actTp = {off=>0,jmpToTarget=>1,toggleToCnt=>2,toggleToCntInv=>3}

	if (actTp == 0) {															// off

	} else if ((actTp == 1) && (lngIn == 1)) {									// jmpToTarget
		// SwJtOn {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
		if      (curStat == 6) nxtStat = srly->lgSwJtOff;						// currently off
		else if (curStat == 3) nxtStat = srly->lgSwJtOn;						// on
		else if (curStat == 4) nxtStat = srly->lgSwJtDlyOff;					// delay off
		else if (curStat == 1) nxtStat = srly->lgSwJtDlyOn;						// delay on
		OnDly   = srly->lgOnDly;												// set timers
		OnTime  = srly->lgOnTime;
		OffDly  = srly->lgOffDly;
		OffTime = srly->lgOffTime;

	} else if ((actTp == 1) && (lngIn == 0)) {									// jmpToTarget
		if      (curStat == 6) nxtStat = srly->shSwJtOff;						// currently off
		else if (curStat == 3) nxtStat = srly->shSwJtOn;						// on
		else if (curStat == 4) nxtStat = srly->shSwJtDlyOff;					// delay off
		else if (curStat == 1) nxtStat = srly->shSwJtDlyOn;						// delay on
		OnDly   = srly->shOnDly;												// set timers
		OnTime  = srly->shOnTime;
		OffDly  = srly->shOffDly;
		OffTime = srly->shOffTime;

	} else if (actTp == 2) {													// toogleToCnt, if tCnt is even, then next state is on
		nxtStat = (cnt % 2 == 0)?3:6;											// even - relay dlyOn, otherwise dlyOff
		OnDly   = 0; OnTime  = 255; OffDly  = 0; OffTime = 255;					// set timers

	} else if (actTp == 3) {													// toggleToCntInv, if tCnt is even, then next state is off, while inverted
		nxtStat = (cnt % 2 == 0)?6:3;											// even - relay dlyOff, otherwise dlyOn
		OnDly   = 0; OnTime  = 255; OffDly  = 0; OffTime = 255;					// set timers
	}
	lastTrig = 40;																// set trigger
	rlyTime = millis();															// changed some timers, activate poll function

	#if defined(ENABLE_ACTORS_DEBUG)											// some debug message
		Serial << F("RL:trigger40, curS:") << curStat << F(", nxtS:") << nxtStat << F(", OnDly:") << OnDly << F(", OnTime:") << OnTime << F(", OffDly:") << OffDly << F(", OffTime:") << OffTime << '\n';
	#endif

	cbS->sendACKStatus(cnlAss,getRly(),((nxtStat==1)||(nxtStat==4))?0x40:0);
}
void RL::sendStatus(void) {
	if (cbS) cbS->sendInfoActuatorStatus(cnlAss,getRly(),getStat());			// call back
}

// public poll function to poll relay and delayed status messages
void RL::poll(void) {
	if (prl.nbr == 0) return;													// no inxtStatnce listed
	for (uint8_t i = 0; i < prl.nbr; i++) {										// step through inxtStatnces
		prl.ptr[i]->poll_rly();													// and poll the relay
		prl.ptr[i]->poll_cbd();													// and poll the call back timer
	}
}

// private functions for setting relay and getting current status
void RL::adjRly(uint8_t tValue) {
	if (curStat == nxtStat) return;												// nothing to do
	if (hwType == 0) {															// monostable - on
		if (hwPin[0] > 0) digitalWrite(hwPin[0],tValue);						// write the state to the port pin
		if (hwPin[1] > 0) digitalWrite(hwPin[1],tValue);

		} else if ((hwType == 1) && (tValue == 1)) {								// bistable - on
		if (hwPin[0] > 0) digitalWrite(hwPin[0],1);								// port pins to on
		if (hwPin[1] > 0) digitalWrite(hwPin[1],1);
		delay(50);																// wait a short time
		if (hwPin[0] > 0) digitalWrite(hwPin[0],0);								// port pins to off again
		if (hwPin[1] > 0) digitalWrite(hwPin[1],0);

		} else if ((hwType == 1) && (tValue == 0)) {								// bistable - off
		if (hwPin[2] > 0) digitalWrite(hwPin[2],1);								// port pins to on
		if (hwPin[3] > 0) digitalWrite(hwPin[3],1);
		delay(50);																// wait a short time
		if (hwPin[2] > 0) digitalWrite(hwPin[2],0);								// port pins to off again
		if (hwPin[3] > 0) digitalWrite(hwPin[3],0);
	}

	#if defined(ENABLE_ACTORS_DEBUG)											// some debug message
		Serial << F("RL:adjRly, curS:") << curStat << F(", nxtS:") << nxtStat << '\n';
	#endif

	cbsTme = millis() + ((uint32_t)mDel*1000) + random(((uint32_t)rDel*1000));	// set the timer for sending the status
	//Serial << "cbsT:" << cbsTme << '\n';
}
uint8_t RL::getRly(void) {
	// curStat could be {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if ((curStat == 1) || (curStat == 3)) return 0xC8;
	if ((curStat == 4) || (curStat == 6)) return 0x00;
}
uint8_t RL::getStat(void) {
	// curStat could be {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	return (rlyTime > 0)?0x40:0x00;
}

// private function for polling the relay and sending delayed status message
void RL::poll_rly(void) {
	if ((rlyTime == 0) || (rlyTime > millis())) return;							// timer set to 0 or time for action not reached, leave
	rlyTime = 0;																// freeze per default

	// set relay - {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if (nxtStat == 3) {															// set relay on
		adjRly(1); curStat = 3;													// adjust relay, status will send from adjRly()

	} else if (nxtStat == 6) {													// set relay off
		adjRly(0); curStat = 6;													// adjust relay, status will send from adjRly()
	}

	// adjust nxtStat for trigger11 - {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if (lastTrig == 11) {
		if (nxtStat == 1) {														// dlyOn -> on
			nxtStat = 3;														// next status is on
			rlyTime = millis() + intTimeCvt(rTime);								// set respective timer

		} else if ((nxtStat == 3) && (dTime > 0)) {								// on - > off
			nxtStat = 6;														// next status is off
			rlyTime = millis() + intTimeCvt(dTime);								// set the respective timer
		}
	}

	// adjust nxtStat for trigger40 - {no=>0,dlyOn=>1,on=>3,dlyOff=>4,off=>6}
	if (lastTrig == 40) {
		if        (nxtStat == 1) {
			nxtStat = 3;
			rlyTime = millis() + byteTimeCvt(OnDly);

		} else if ((nxtStat == 3) && (OnTime < 255)) {
			nxtStat = 4;
			if (OnTime) rlyTime = millis() + byteTimeCvt(OnTime);

		} else if (nxtStat == 4) {
			nxtStat = 6;
			rlyTime = millis() + byteTimeCvt(OffDly);

		} else if ((nxtStat == 6) && (OffTime < 255)) {
			nxtStat = 1;
			if (OffTime) rlyTime = millis() + byteTimeCvt(OffTime);
		}
	}

	cbM(cnlAss, curStat, nxtStat);
}
void RL::poll_cbd(void) {
	if ((cbsTme == 0) || (cbsTme > millis())) return;							// timer set to 0 or time for action not reached, leave
	if (cbS) cbS->sendInfoActuatorStatus(cnlAss,getRly(),0);					// call back
	cbsTme = 0;																	// nothing to do any more
}
