#ifndef ACTORS_H
	#define ACTORS_H

//	#define ENABLE_ACTORS_DEBUG													// Debuging in Actors plugin

	#if defined(ARDUINO) && ARDUINO >= 100
		#include "Arduino.h"
	#else
		#include "WProgram.h"
	#endif

	#include <Helpers.h>
	#include <AskSin.h>

	class AskSin;

	//- -----------------------------------------------------------------------------------------------------------------------
	//- relay functions -------------------------------------------------------------------------------------------------------
	//- -----------------------------------------------------------------------------------------------------------------------
	class RL {
		public://--------------------------------------------------------------------------------------------------------------
		void    config(uint8_t cnl, uint8_t type, uint8_t pinOn1, uint8_t pinOn2, uint8_t pinOff1, uint8_t pinOff2);
		void    setCallBack(void msgCallBack(uint8_t, uint8_t, uint8_t), AskSin *statCallBack, uint8_t minDelay, uint8_t randomDelay);

		void    trigger11(uint8_t val, uint8_t *rampTime, uint8_t *duraTime);		// FHEM event
		void    trigger41(uint8_t lngIn, uint8_t val, void *list3);					// sensor event called
		void    trigger40(uint8_t lngIn, uint8_t cnt, void *plist3);				// remote event called
		void    sendStatus(void);													// answer on a status request

		void    poll(void);															// poll handler

		private://-------------------------------------------------------------------------------------------------------------
		struct s_srly {
			uint8_t shCtDlyOn           :4;
			uint8_t shCtDlyOff          :4;
			uint8_t shCtOn              :4;
			uint8_t shCtOff             :4;
			uint8_t shCtValLo;
			uint8_t shCtValHi;
			uint8_t shOnDly;
			uint8_t shOnTime;
			uint8_t shOffDly;
			uint8_t shOffTime;
			uint8_t shActionType        :2;
			uint8_t                     :4;
			uint8_t shOffTimeMode       :1;
			uint8_t shOnTimeMode        :1;
			uint8_t shSwJtOn            :4;
			uint8_t shSwJtOff           :4;
			uint8_t shSwJtDlyOn         :4;
			uint8_t shSwJtDlyOff        :4;
			uint8_t lgCtDlyOn           :4;
			uint8_t lgCtDlyOff          :4;
			uint8_t lgCtOn              :4;
			uint8_t lgCtOff             :4;
			uint8_t lgCtValLo;
			uint8_t lgCtValHi;
			uint8_t lgOnDly;
			uint8_t lgOnTime;
			uint8_t lgOffDly;
			uint8_t lgOffTime;
			uint8_t lgActionType        :2;
			uint8_t                     :3;
			uint8_t lgMultiExec         :1;
			uint8_t lgOffTimeMode       :1;
			uint8_t lgOnTimeMode        :1;
			uint8_t lgSwJtOn            :4;
			uint8_t lgSwJtOff           :4;
			uint8_t lgSwJtDlyOn         :4;
			uint8_t lgSwJtDlyOff        :4;
		};

		s_srly *srly;
		uint8_t curStat:4, nxtStat:4;												// current state and next state
		uint8_t OnDly, OnTime, OffDly, OffTime;										// trigger 40/41 timer variables
		uint8_t lastTrig;															// store for the last trigger
		uint16_t rTime, dTime;														// trigger 11 timer variables
		uint32_t rlyTime;															// timer for poll routine

		uint8_t mDel, rDel;															// store for the call back delay
		void (*cbM)(uint8_t, uint8_t, uint8_t);										// call back address for state change
		AskSin (*cbS);																// call back for status message sending
		uint32_t cbsTme;															// timer for call back poll routine

		uint8_t hwType:1;															// 0 indicates a monostable, 1 a bistable relay
		uint8_t cnlAss:6;															// remembers channel for the inxtStatnce
		uint8_t hwPin[4];															// first 2 bytes for on, second two for off

		void    adjRly(uint8_t tValue);												// set the physical status of the relay
		uint8_t getRly(void);														// get the status of the relay
		uint8_t getStat(void);														// get the status of the module

		void    poll_rly(void);														// polling function for delay and so on
		void    poll_cbd(void);														// polling function for call back delay
	};
	#define maxRelay 10
	struct s_prl {
		uint8_t nbr;
		RL *ptr[maxRelay];
	};

#endif
