#ifndef ASKSIN_H
	#define ASKSIN_H

//	#define ENABLE_ASKSIN_DEBUG													// enable for debuging infos
//	#define ENABLE_ASKSIN_EXPLAIN_DEBUG											// explain message
//	#define ENABLE_SM_DEBUG

	#if defined(ARDUINO) && ARDUINO >= 100
		#include "Arduino.h"
	#else
		#include "WProgram.h"
	#endif

	#include "Register.h"

	#include <avr/eeprom.h>
	#include <avr/power.h>
	#include <avr/sleep.h>
	#include <avr/wdt.h>

	#include <cc110x.h>
	#include <StatusLed.h>
	#include <Buttons.h>
	#include <Actors.h>
	#include <InputParser.h>
	#include <Helpers.h>
	#include <Serial.h>

	// - general struct and declarations ---------------------------------------------------------------------------------------
	struct s_jumptable {
		uint8_t code;																// one byte command code
		uint8_t spec;																// one byte command specifier
		void (*fun)(uint8_t, uint8_t*, uint8_t);									// code to call for this command
	};

	static uint8_t broadCast[] = {0,0,0,0};											// default broadcast address

	//- interrupt handling
	static volatile uint8_t wd_flag;

	extern volatile unsigned long timer0_millis;									// make millis timer available for correcting after deep sleep

	class cc110x;


	//- -----------------------------------------------------------------------------------------------------------------------
	//- AskSin protocol functions ---------------------------------------------------------------------------------------------
	//- with a lot of support from martin876 at FHEM forum
	//- -----------------------------------------------------------------------------------------------------------------------
	class AskSin {
		public://--------------------------------------------------------------------------------------------------------------
		// public variables for send and receive
		cc110x cc1101;																// OK, init of the RF/TX module
		StatusLed statusLed;

		#define send_payLoad		(send.data + 10)								// payload for send queue
		#define recv_payLoad		(recv.data + 10)								// payload for receive queue

		struct s_send {																// send queue structure
			uint8_t  data[60];														// buffer for send string
			uint8_t  mCnt;															// message counter
			uint8_t  counter;														// send try counter, has to be 0 if send queue is empty
			uint8_t  retries;														// set max. retries, if message requires an ACK, retries will set to 3
			uint32_t timer;															// timer variable used for store next check time
			uint8_t  burst;															// receiver needs burst signal
		} send;
		struct s_recv {																// receive queue structure
			uint8_t data[60];														// buffer for received string
			uint8_t p_data[60];														// previous buffer, needed while checking against repeated messages
			uint8_t forUs;															// for us indication flag, is set while the received message was addressed to us
			uint8_t  bCast;															// broadcast indication flag
		} recv;
		struct s_powr {
			uint8_t mode;															// indicate the power mode, TX enabled in all modes; 0 = RX enabled,  1 = RX in burst mode, 2 = RX off
			uint8_t state;															// current state of TRX868 module
			uint16_t parTO;															// timeout for pairing in ms
			uint16_t minTO;															// minimum time out in ms
			uint16_t wdTme;															// clock cycle of watch dog in ms
			uint32_t nxtTO;															// check millis() timer against, if millis() >= nextTimeout go in powerdown
		} powr;

		s_jumptable *jTblPtr;														// jump table pointer for event handling

		// general functions for initializing and operating of module
		AskSin(s_jumptable *jtPtr, void *mcPtr);									// OK, main object, take over the pointer to jump table for event signalization

		void init(void);															// OK, init function for HM module

		//************************************************************************************
		//************************************************************************************








		void poll(void);															// OK, main task to manage TX and RX messages
		void send_out(void);														// OK, send function
		void reset(void);															// OK, clear peer database and register content, do a reset of the device
		void setConfigEvent(void);													// OK, raise a config had changed event for main sketch

		void setPowerMode(uint8_t mode);											// set power mode for HM device
		void stayAwake(uint32_t xMillis);											// switch TRX module in RX mode for x milliseconds

		// external functions for pairing and communicating with the module
		void startPairing(void);													// OK, start pairing with master
		void sendInfoActuatorStatus(uint8_t cnl, uint8_t status, uint8_t flag);		// OK, send status function
		void sendACKStatus(uint8_t cnl, uint8_t status, uint8_t douolo);			// OK, send ACK with status
		void sendPeerREMOTE(uint8_t button, uint8_t longPress, uint8_t lowBat);		// (0x40) send REMOTE event to all peers
		void sendPeerRAW(uint8_t cnl, uint8_t type, uint8_t *data, uint8_t len);	// send event to all peers listed in the peers database by channel, type specifies the type of the message, data and len delivers the content of the event
		void send_ACK(void);														// OK, ACK sending function
		void send_NACK(void);														// OK, NACK sending function

		// some debug functions
		void printSettings(void);
		void printConfig(void);

	//	protected://-----------------------------------------------------------------------------------------------------------
		// hardware definition for interrupt handling
		#define GDO0	            2
		#define PORT_GDO0           PIND
		#define BIT_GDO0            2
		#define INT_GDO0            0

		#define enableIRQ_GDO0()    attachInterrupt(INT_GDO0, isrGDO0event, FALLING);
		#define disableIRQ_GDO0()   detachInterrupt(INT_GDO0);
		static void isrGDO0event(void);												// interrupt to put the bytes from cc1011 to hm

		// structure for handling configuration requests
		struct s_conf {
			uint8_t mCnt;															// message counter
			uint8_t reID[3];														// requester id
			uint8_t channel;														// requested channel
			uint8_t list;															// requested list
			uint8_t peer[4];														// requested peer id and peer channel
			uint8_t type;															// message type for answer
			uint8_t wrEn;															// write enabled
			uint8_t act;															// active, 1 = yes, 0 = no
		} conf;

		// structure for handling send peer events
		struct s_pevt {
			uint8_t cnl;															// peer database channel
			uint8_t type;															// message type
			uint8_t data[20];														// data to send
			uint8_t len;															// len of data to send
			uint8_t idx;															// current idx in peer database
			uint8_t sta;															// status message send; 0 no, 1 yes
			uint8_t mFlg;															// message flag
			uint8_t mCnt[maxChannel];												// message counter per channel
			uint8_t eol;															// end of long indicator
			uint8_t act;															// active, 1 = yes, 0 = no
		} pevt;

		// some short hands for receive string handling
		#define recv_len			recv.data[0]									// length of received bytes
		#define recv_rCnt			recv.data[1]									// requester message counter, important for ACK
		#define recv_reID			recv.data+4										// requester ID - who had send the message
		#define recv_msgTp			recv.data[3]									// message type
		#define recv_by10			recv.data[10]									// byte 10 type
		#define recv_by11			recv.data[11]									// byte 11 type

		#define recv_isMsg			bitRead(recv.data[2],7)							// is message, true if 0x80 was set
		#define recv_isRpt			bitRead(recv.data[2],6)							// is a repeated message, true if 0x40 was set
		#define recv_ackRq			bitRead(recv.data[2],5)							// ACK requested, true if 0x20 was set
		#define recv_isCfg			bitRead(recv.data[2],2)							// configuration message, true if 0x04 was set
		#define recv_toMst			bitRead(recv.data[2],1)							// message to master, true if 0x02 was set

		// some polling functions
		void recv_poll(void);														// handles the received string
		void send_poll(void);														// OK, handles the send queue
		void send_conf_poll(void);													// handles information requests
		void send_peer_poll(void);													// handle send events to peers
		void power_poll(void);														// handles the power modes of the TRX868 module

		// receive message handling
		void recv_ConfigPeerAdd(void);												// OK, 01, 01
		void recv_ConfigPeerRemove(void);											// OK, 01, 02
		void recv_ConfigPeerListReq(void);											// OK, 01, 03
		void recv_ConfigParamReq(void);												// OK, 01, 04
		void recv_ConfigStart(void);												// OK, 01, 05
		void recv_ConfigEnd(void);													// OK, 01, 06
		void recv_ConfigWriteIndex(void);											// OK, 01, 08
		void recv_ConfigSerialReq(void);											// OK, 01, 09
		void recv_Pair_Serial(void);												// OK, 01, 0A
		void recv_ConfigStatusReq(void);											// 01, 0E
		void recv_PeerEvent(void);													// OK, 40
		void recv_PairEvent(void);													// 11
		uint8_t recv_Jump(uint8_t tCnl);											// check against jump table to call function in user area, 1 if call was done, 0 if not

		// internal send functions
		void send_prep(uint8_t msgCnt, uint8_t comBits, uint8_t msgType, uint8_t *targetID, uint8_t *payLoad, uint8_t payLen);

		// some internal helpers
		void hm_enc(uint8_t *buffer);												// OK, encrypts AskSin payload
		void hm_dec(uint8_t *buffer);												// OK, decrypts AskSin payload
		void exMsg(uint8_t *buf);													// OK, shows enhanced information on the logged communication strings


		// - Storage Management -----------------------------------------------------------------------------------------------
		#define magicNumber 1967													// magic number to detect first run

		//s_EEPROM *ee;																// pointer to settings in eeprom
		s_regDev regDev;															// structure which holds List0
		s_EEPROM *ee;																// pointer to eeprom structure
		uint16_t mcConfPtr;															// pointer to main channel structure
		uint16_t l3Ptr[maxChannel+1];												// holds pointer for list3 in regMC per channel













		// init registers and load default config
		void     initRegisters(void);												// init eeprom and fill registers from eeprom

		// slice table functions
		struct s_slcVar { uint8_t slcPtr; uint8_t slcLen; uint16_t phyAddr;	uint8_t phyLen;};
		uint8_t  getSliceDetail(uint8_t cnl, uint8_t lst, uint8_t *peer, s_slcVar *sV);	// OK, returns 1 if ok, 0 on failure
		uint8_t  doesListExist(uint8_t cnl, uint8_t lst);							// OK, check if a list exist, 1 for yes, 0 for no
		uint8_t  getRegList(uint8_t slc_tr, uint8_t slcLen, uint8_t *buf);			// OK, returns len of buffer
		void     getMainChConfig(void);												// OK, load List0 and List1 per channel into a struct
		void     getList3ByPeer(uint8_t cnl, uint8_t *peer);						// loads the device channel into a struct
		// todo: get register address from list for send as actuator

		// message generation for TRX868 module
		uint8_t  getListForMsg2(uint8_t cnl, uint8_t lst, uint8_t *peer, uint8_t *buf);	// OK, create the answer of a info request by filling *buf, returns len of buffer, 0 if done and ff on failure
		uint8_t  getListForMsg3(uint8_t cnl, uint8_t lst, uint8_t *peer, uint8_t *buf);	// not defined yet
		uint8_t  setListFromMsg(uint8_t cnl, uint8_t lst, uint8_t *peer, uint8_t *buf, uint8_t len);	// OK, writes the register information in *buf to eeprom, 1 if all went ok, 0 on failure
		uint8_t  getPeerListForMsg(uint8_t cnl, uint8_t *buf);						// OK, create a peer list in the format for answering a peer list request in *buf for the respective channel, returns length of buf, max amount of one junk is 16 bytes, reload the function until len = 0

		// peerdb handling; add, remove and search functions
		uint8_t  addPeerFromMsg(uint8_t cnl, uint8_t *peer);						// OK, add a peer to the peer database in the respective channel, returns 0xff on failure, 0 on full and 1 if everything went ok
		uint8_t  removePeerFromMsg(uint8_t cnl, uint8_t *peer);						// OK, remove a 4 byte peer from peer database in the respective channel, returns 0 on not available and 1 if everything is ok
		uint8_t  getCnlByPeer(uint8_t *peer);										// OK, find the respective channel of a 4 byte peer in the peer database, returns the channel 1 to x if the peer is known, 0 if unknown
		uint8_t  getIdxByPeer(uint8_t cnl, uint8_t *peer);							// OK, find the appropriate index of a 4 byte peer in peer database by selecting the channel and searching for the peer, returns peer index, if not found 0xff
		uint8_t  getPeerByIdx(uint8_t cnl, uint8_t idx, uint8_t *peer);				// OK, get the respective 4 byte peer from peer database by selecting channel and index, returns the respective peer in the _tr, function returns 1 if everything is fine, 0 if not
		uint8_t  getFreePeerSlot(uint8_t cnl);										// OK, search for a free slot in peerdb, return the index for a free slot, or 0xff if there is no free slot
		uint8_t  countFreePeerSlot(uint8_t cnl);									// OK, count free slots in peer database by channel
		uint8_t  addPeerToDB(uint8_t cnl, uint8_t *peer);							// OK, add a single peer to database, returns 1 if ok, 0 for failure

		// to check incoming messages if sender is known
		uint8_t  isPeerKnown(uint8_t *peer);										// OK, check 3 byte peer against peerdb, return 1 if found, otherwise 0
		uint8_t  isPairKnown(uint8_t *pair);										// OK, check 3 byte pair against List0, return 1 if pair is known, otherwise 0

		// pure eeprom handling, i2c must implemented
		uint8_t  getEEpromByte(uint16_t addr);										// read EEprom for register config
		void     setEEpromByte(uint16_t addr, uint8_t payload);						// write EEprom for register config
		void     getEEpromBlock(uint16_t addr,uint8_t len,void *ptr);
		void     setEEpromBlock(uint16_t addr,uint8_t len,void *ptr);
	};

	extern AskSin askSin;

#endif
