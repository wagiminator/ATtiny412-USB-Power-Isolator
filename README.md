# USB Power Isolator
For some applications, a galvanically isolated power supply is required to avoid ground loops or short circuits, or simply for mutual protection. The USB Power Isolator offers the possibility of a simple isolation (2.5kV) of the 5V rail of a USB power adapter with an unregulated output and a maximum continuous current of 1A. The USB Power Isolator isolates the power rail only, for full isolation including data lines take a look at the [ADuM3160 USB Isolator](https://github.com/wagiminator/ADuM3160-USB-Isolator).

- Design Files (EasyEDA): 

![pic1.jpg](https://raw.githubusercontent.com/wagiminator/ATtiny412-USB-Power-Isolator/main/documentation/USB_Power_Isolator_pic1.jpg)

# Hardware
## Working Principle
Using its powerful Timer/Counter D (TCD), the ATtiny212/412 controls a center-tapped transformer utilizing the push-pull topology by providing complementary output signals with intermediate dead times (break-before-make time) to drive the two ground-referenced N-channel MOSFETs connected to the transformer's outer taps and alternately turn them on and off. The implemented switching frequency of 400kHz is specially tailored to the Würth Elektronik 750315371 transformer, but other frequencies and features like spread spectrum clocking can be implemented in software if required.

There are actually special ICs to drive push-pull transformers, in the case of the WE750315371 this is for example the SN6505B. However, the small ATtinys are significantly cheaper and can easily imitate the tasks of these ICs.

## Push-Pull Converter
Push-pull converters require center-tapped transformers to transfer power from the primary to the secondary. The way it works is clearly described in the [SN6505B datasheet](https://www.ti.com/lit/ds/symlink/sn6505b-q1.pdf).

![wiring2.png](https://raw.githubusercontent.com/wagiminator/ATtiny412-USB-Power-Isolator/main/documentation/USB_Power_Isolator_wiring2.png)

When Q1 conducts, VIN (+5V) drives a current through the upper half of the primary to ground, thus creating a negative voltage potential at the upper primary end with regards to the VIN potential at the center-tap.

At the same time the voltage across the lower half of the primary is such that the lower primary end is positive with regards to the center-tap in order to maintain the previously established current flow through Q2, which now has turned high-impedance. The two voltage sources, each of which equaling VIN (+5V), appear in series and cause a voltage potential at the open end of the primary of 2×VIN (+10V) with regards to ground.

Per dot convention the same voltage polarities that occur at the primary also occur at the secondary. The positive potential of the lower secondary end therefore forward biases diode D2. The secondary current starting from the lower secondary end flows through D2, charges capacitors C3 - C5, and returns through the load impedance back to the center-tap.

When Q2 conducts, Q1 goes high-impedance and the voltage polarities at the primary and secondary reverse. Now the upper end of the primary presents the open end with a 2×VIN (+10V) potential against ground. In this case D1 is forward biased while D2 is reverse biased and current flows from the upper secondary end through D1, charging the capacitors and returning through the load to the center-tap.

## PCB Implementations
Two PCB implementations are available. The first outputs unregulated 5V via a female USB socket and can be easily plugged in between the USB power adapter and the consumer. The second can be used as a split power supply with +5V and -5V.

### USB-A Pass-Through
![pic4.jpg](https://raw.githubusercontent.com/wagiminator/ATtiny412-USB-Power-Isolator/main/documentation/USB_Power_Isolator_pic4.jpg)

### ±5V Split Power Supply
![split_wiring2.png](https://raw.githubusercontent.com/wagiminator/ATtiny412-USB-Power-Isolator/main/documentation/USB_Split_Power_Isolator_wiring2.png)
![split_pic2.jpg](https://raw.githubusercontent.com/wagiminator/ATtiny412-USB-Power-Isolator/main/documentation/USB_Split_Power_Isolator_pic2.jpg)

# Software
## Creating the Driving Signals
Two complementary logic signals with intermediate dead times (break-before-make times) are required to control the gates of the two MOSFETs, which are switched on and off alternately. Before either one of the gates can assume logic high, the dead time logic ensures a short time period during which both signals are low and both MOSFETs are high-impedance. This short period is required to avoid shorting out both ends of the primary. The ATtiny's Timer/Counter D (TCD) is used to generate these signals.

The TCD is operated in Four Ramp Mode with a clock frequency of 20MHz. The two outputs WOA and WOB drive the gates of the two MOSFETs which are connected to the outer taps of the primary side of the transformer.

In Four Ramp mode, the TCD cycle follows this pattern:
1. A TCD cycle begins with the TCD counter counting up from zero until it reaches the CMPASET value. Then it resets to zero and switches the WOA pin to HIGH.
2. The counter counts up until it reaches the CMPACLR value, then it resets to zero and switches the WOA pin to LOW.
3. The counter counts up until it reaches the CMPBSET value, then it resets to zero and switches the WOB pin to HIGH.
4. The counter counts up until it reaches the CMPBCLR value, and ends the TCD cycle by resetting to zero and switching WOB pin to LOW.

![TCD.png](https://raw.githubusercontent.com/wagiminator/ATtiny412-USB-Power-Isolator/main/documentation/USB_Power_Isolator_TCD.png)

The values of the registers CMPASET and CMPBSET determine the length of the respective dead times, registers CMPACLR and CMPBCLR the on times of WOA and WOB, respectively. Both dead times and both on times must be the same in each case so that an imbalance in the magnetic flux density swing is avoided and the transformer does not creep into the saturation region.

A dead time of $T_{DEAD} = 100ns$ each is sufficient and also corresponds to the values of the SN6505B. According to the data sheet, a switching frequency of $f_{SW} = 400kHz$ is intended for the Würth Elektronik 750315371 transformer. Since the TCD works with a clock frequency of $f_{TCD} = 20MHz$, the following register values are calculated:

(1) $T_{SW} = \frac{1}{f_{SW}} = \frac{1}{400000Hz} = 2500ns$

(2) $T_{ON} = \frac{T_{SW}}{2} - T_{DEAD} = \frac{2500ns}{2} - 100ns = 1150ns$

(3) $CMPASET = CMPBSET = T_{DEAD} \times f_{TCD} - 1 = 0.0000001s \times 20000000Hz - 1 = 1$

(4) $CMPACLR = CMPBCLR = T_{ON} \times f_{TCD} - 1 = 0.00000115s \times 20000000Hz - 1 = 22$

The main function of the firmware is thus as follows:

```c
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
```

## Compiling and Uploading the Firmware
### If using the Arduino IDE
- Open your Arduino IDE.
- Make sure you have installed [megaTinyCore](https://github.com/SpenceKonde/megaTinyCore).
- Go to **Tools -> Board -> megaTinyCore** and select **ATtiny412/402/212/202**.
- Go to **Tools** and choose the following board options:
  - **Chip:**           ATtiny212 or ATtiny412
  - **Clock:**          20 MHz internal
  - Leave the rest at the default settings.
- Connect your [programmer](https://github.com/wagiminator/AVR-Programmer) to your PC and to the UPDI header on the board.
- Go to **Tools -> Programmer** and select your UPDI programmer.
- Go to **Tools -> Burn Bootloader** to burn the fuses.
- Open the sketch and click **Upload**.

### If using the makefile (Linux/Mac)
- Connect your [programmer](https://github.com/wagiminator/AVR-Programmer) to your PC and to the UPDI header on the board.
- Download [AVR 8-bit Toolchain](https://www.microchip.com/mplab/avr-support/avr-and-arm-toolchains-c-compilers) and extract the sub-folders (avr, bin, include, ...) to /software/tools/avr-gcc. To do this, you have to register for free with Microchip on the download site.
- Open a terminal.
- Navigate to the folder with the makefile and the sketch.
- Run `DEVICE=attiny412 PROGRMR=serialupdi PORT=/dev/ttyUSB0 make install` to compile, burn the fuses and upload the firmware (change DEVICE, PROGRMR and PORT accordingly).

# Performance
![perf1.png](https://raw.githubusercontent.com/wagiminator/ATtiny412-USB-Power-Isolator/main/documentation/USB_Power_Isolator_perf1.png)

# References, Links and Notes
1. [ATtiny412 Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/ATtiny212-214-412-414-416-DataSheet-DS40002287A.pdf)
2. [SN6505B Datasheet](https://www.ti.com/lit/ds/symlink/sn6505b-q1.pdf)
3. [WE750315371 Datasheet](https://katalog.we-online.com/ctm/datasheet/750315371.pdf)
4. [ADuM3160 USB Isolator](https://github.com/wagiminator/ADuM3160-USB-Isolator)

![pic2.jpg](https://raw.githubusercontent.com/wagiminator/ATtiny412-USB-Power-Isolator/main/documentation/USB_Power_Isolator_pic2.jpg)

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
