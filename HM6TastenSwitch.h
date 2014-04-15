#define ENABLE_COMMANDS
//#define ENABLE_DEBUG

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

//- load library's --------------------------------------------------------------------------------------------------------
#include <Helpers.h>
#include <cc110x.h>
#include <StatusLed.h>
#include <Buttons.h>
#include <Actors.h>
#include <AskSin.h>
#include <InputParser.h>

#define AVR_BANDGAP_VOLTAGE       1100UL										// Bandgap reference for Atmega328p
#define BATTERY_MIN_VOLTAGE       2350											// voltage below low battery warning sent
#define CONFIG_MODE_RESET_TIMEOUT 15000											// voltage below low battery warning sent


#if defined(ENABLE_DEBUG)
	//- some timer test
	uint32_t nTimer;
#endif

//- main functions -------------------------------------------------------------
void setup();
void loop();

//- hardware functions -----------------------------------------------
void buttonState(uint8_t idx, uint8_t state, uint8_t pressRepeatCount);

//- HM functions ---------------------------------------------------------------
void HM_Status_Request(uint8_t cnl, uint8_t *data, uint8_t len);
void HM_Reset_Cmd(uint8_t cnl, uint8_t *data, uint8_t len);
void HM_Config_Changed(uint8_t cnl, uint8_t *data, uint8_t len);

void stayAwake(uint32_t xMillis);

//- serial communication --------------------------------------------------------------------------------------------------
const char helptext1[] PROGMEM = {												// help text for serial console
	"\n"
	"Available commands:" "\n"
	"  p                - start pairing with master" "\n"
	"  b[0]  b[n]  s    - send a string, b[0] is length (50 bytes max)" "\n"
	"\n"
	"  i[0]. i[1]. e    - show eeprom content, i[0]. start address, i[1]. length" "\n"
	"  i[0]. b[1]  f    - write content to eeprom, i[0]. address, i[1] byte" "\n"
	"  c                - clear eeprom complete, write 0 from start to end" "\n"
	"\n"
	"  b[c]  b[l]  b    - send button event, b[c] channel, b[l] short 0 or long 1" "\n"
	"  a                - stay awake for TRX module (valid if power mode = 2)" "\n"
	"  t                - gives an overview of the device configuration" "\n"
	"\n"
	"  $nn for HEX input (e.g. $AB,$AC ); b[] = byte, i[]. = integer " "\n"
};

//- config functions -----------------------------------------------------------
void sendCmdStr();
void sendPairing();
void showEEprom();
void writeEEprom();
void clearEEprom();
void showHelp();
void showSettings();
void testConfig();
void buttonSend();
void stayAwake30();
void resetDevice();
