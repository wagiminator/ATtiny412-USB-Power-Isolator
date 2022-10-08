// ===================================================================================
// Project:   Transformer Driver for Isolated Power Supplies based on ATtiny212/412
// Version:   v1.0
// Year:      2022
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// Using its powerful Timer/Counter D (TCD), the ATtiny controls a center-tapped 
// transformer utilizing the push-pull topology by providing complementary output 
// signals with intermediate dead times (break-before-make time) to drive the two
// ground-referenced N-channel power switches connected to the transformer's outer 
// taps and alternately turn them on and off. The implemented switching frequency
// of 400kHz is specially tailored to the Würth Elektronik 750315371 transformer, 
// but other frequencies and features like spread spectrum clocking can be 
// implemented in software if required.
//
// Wiring:
// -------
//                     +-\/-+
//               Vdd  1|°   |8  GND
//   WOA --- TXD PA6  2|    |7  PA3 AIN3 -------- 
//   WOB --- RXD PA7  3|    |6  PA0 AIN0 UPDI --- UPDI
//       --- SDA PA1  4|    |5  PA2 AIN2 SCL ---- 
//                     +----+
//
// Compilation Settings:
// ---------------------
// Core:    megaTinyCore (https://github.com/SpenceKonde/megaTinyCore)
// Board:   ATtiny412/402/212/202
// Chip:    ATtiny212 or ATtiny412
// Clock:   20 MHz internal
//
// Leave the rest on default settings. Don't forget to "Burn bootloader". 
// Compile and upload the code.
//
// No Arduino core functions or libraries are used. To compile and upload without
// Arduino IDE download AVR 8-bit toolchain at:
// https://www.microchip.com/mplab/avr-support/avr-and-arm-toolchains-c-compilers
// and extract to tools/avr-gcc. Use the makefile to compile and upload.
//
// Fuse Settings: 0:0x00 1:0x00 2:0x02 4:0x00 5:0xC5 6:0x04 7:0x00 8:0x00
//
// Calculations:
// -------------
// TCD clock freq   = OSC freq / prescaler = 20Mhz / 1                     = 20Mhz
// TCD clock length = 1 / TCD clock freq = 1 / 20MHz                       = 50ns
// TCD cycle clocks = (CMPASET+1) + (CPACLR+1) + (CMPBSET+1) + (CMPBCLR+1) = 50
// TCD cycle length = TCD cycle clocks * TCD clock length = 50 * 50ns      = 2500ns
// TCD cycle freq   = 1 / TCD cycle length = 1 / 2500ns                    = 400kHz


// Libraries
#include <avr/io.h>                               // for GPIO
#include <avr/sleep.h>                            // for sleep functions

// Pin assignments
#define PIN_WOA       PA6                         // transformer control A
#define PIN_WOB       PA7                         // transformer control B

// Pin manipulation macros
enum {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7};    // enumerate pin designators
#define pinOutput(x)  VPORTA.DIR |=  (1<<(x))     // set pin to OUTPUT

// Main function
int main(void) {
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);         // set clock frequency to 20 MHz
  pinOutput(PIN_WOA); pinOutput(PIN_WOB);         // enable output on WOA/WOB pin
  TCD0.CTRLB     = TCD_WGMODE_FOURRAMP_gc;        // set TCD four ramp mode
  TCD0.CMPASET   = 01;                            //  100ns dead time
  TCD0.CMPACLR   = 22;                            // 1150ns on-time WOA
  TCD0.CMPBSET   = 01;                            //  100ns dead time
  TCD0.CMPBCLR   = 22;                            // 1150ns on-time WOB
  CPU_CCP        = CCP_IOREG_gc;                  // protected write is coming
  TCD0.FAULTCTRL = TCD_CMPAEN_bm                  // enable WOA output channel
                 | TCD_CMPBEN_bm;                 // enable WOB output channel
  while(~TCD0.STATUS & TCD_ENRDY_bm);             // wait for synchronization
  TCD0.CTRLA     = TCD_CLKSEL_20MHZ_gc            // select 20MHz base clock
                 | TCD_CNTPRES_DIV1_gc            // no prescaler -> 20MHz TCD clock
                 | TCD_ENABLE_bm;                 // enable timer
  SLPCTRL.CTRLA  = SLPCTRL_SMODE_IDLE_gc          // set sleep mode to IDLE
                 | SLPCTRL_SEN_bm;                // enable sleep
  while(1) sleep_cpu();                           // CPU is not needed anymore
}
