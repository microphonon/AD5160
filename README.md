# AD5160
Demo code uses MSP430FR5969 Launchpad to control AD5160 8-bit digital potentiometer from serial terminal. Assumes 10k pot; other values available from Analog Devices are 5k, 50k, and 100k. Resistance is selected in 1k increments in the range 0--10k. Example: Entering 3 sets pot at ~3k as read from wiper (pin W) to pin B. Resistance across pins A-W is 10k - resistance across pins W-B. The wiper resistance is 50--120 ohms; see data sheet. Pressing return key only needed to enter 1 which sets 1k; all other integer values are set automatically. Only 11 integers are recognized; any entry outside the range 0--10 is invalid. UART clock sourced from SMCLK. Remain in LPM1 until RX interrupt is received. Could also use ACLK for UART and idle in LPM3. There is no data read from device, so only MOSI line is needed with loopback mode. SPI clock is 1 MHz sourced from SMCLK. Set serial port for 9600 baud, 8-bits, 1 stop, no parity, no flow control. UART interface is on TXD (P2.5) and RXD (P2.6); these ports are reversed on the receiving device, ie. the RX-TX are switched on the PC interface cable. IDE is CCS 6.1.3. Launchpad pins:

<p>P1.6  UCB0 SPI MOSI
<br>P1.7  UCB0 SPI MISO (not used)
<br>P2.2  UCB0 SPI CLK
<br>P1.5  CS for AD5160
	<br>P2.5  UCA1TXD
<br>P2.6  UCA1RXD
