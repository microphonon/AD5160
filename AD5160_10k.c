#include <msp430.h> 
#include <stdio.h>
#include <stdint.h>

/*
 Demo code uses MSP430FR5969 Launchpad to control AD5160 8-bit digital potentiometer from serial terminal.
 Assumes 10k pot; other values available from Analog Devices are 5k, 50k, and 100k.
 Resistance is selected in 1k increments in the range 0--10k. Example: Entering 3 sets pot at ~3k
 as read from wiper (pin W) to pin B. Resistance across pins A-W is 10k - resistance across pins W-B.
 The wiper resistance is 50--120 ohms; see data sheet.
 Pressing return key only needed to enter 1 which sets 1k; all other integer values are set automatically.
 Only 11 integers are recognized; any entry outside the range 0--10 is invalid.
 UART clock sourced from SMCLK. Remain in LPM1 until RX interrupt is received. Could also use ACLK
 for UART and idle in LPM3. There is no data read from device, so only MOSI line is needed with loopback mode.
 SPI clock is 1 MHz sourced from SMCLK. Set serial port for 9600 baud, 8-bits, 1 stop, no parity, no flow control.
 UART interface is on TXD (P2.5) and RXD (P2.6); these ports are reversed on the receiving device, ie. the RX-TX
 are switched on the PC interface cable. IDE is CCS 6.1.3. Launchpad pins:

	P1.6  UCB0 SPI MOSI
	P1.7  UCB0 SPI MISO (not used)
	P2.2  UCB0 SPI CLK
	P1.5  CS for AD5160
	P2.5  UCA1TXD
	P2.6  UCA1RXD
 */

void SetPins(void);
void SetUART(void);
void SetSPI(void);
void Invalid(void);
void DigPot(uint8_t trimpot);

//Variables for UART terminal interface
char str[80];
volatile uint8_t RXdata;  // Character from terminal
volatile uint8_t i,count;

void main(void) {

    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    SetPins();
    SetUART();
    SetSPI();

    _BIS_SR(GIE); //Enable global interrupts.

    while(1) //Infinite loop
    {
    	 sprintf(str,"%s", "\n\r Resistance x 10k: ");
    	 count = sizeof str;
    	 for (i=0; i < count; i++)
    	 {
    		 while (!(UCA1IFG & UCTXIFG)); // USCI_A0 TX buffer ready?
    		 UCA1TXBUF = str[i]; //Send data 1 byte at a time
    	 }
    	 while(1)
    	 {
    		 LPM1;  //Wait in LPM1 for characters typed in terminal program
    		 if(RXdata >= 0x32 && RXdata <= 0x39) //First character sets R = 2k--9k
    		 {
    			 DigPot(RXdata - 0x30);
    			 break;
    		 }
    		 else if (RXdata == 0x30) //First character sets R=0k
    		 {
    		     DigPot(0);
    		     break;
    		 }
    		 else if (RXdata == 0x31) //First character is 1
    		 {
    			 LPM1;
    			 if (RXdata == 0x30) //Second character 0; set R = 10k
    			 {
    				 DigPot(10);
    				 break;
    			 }
    			 else if (RXdata == 0x0D) //Second character ENTER; set R = 1k
    			 {
    				 DigPot(1);
    				 break;
    			 }
    			 else
    				 {
    				 Invalid();
    				 break;
    				 }
    		 }
    		 else
    		 {
    			 Invalid();
    			 break;
    		 }
    	 }
    }
}

#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
 switch(__even_in_range(UCA1IV,18))
   {
     case 0x00: break; //Vector 0: No interrupts
     case 0x02:			//Vector 2: UCRXIFG
       while(!(UCA1IFG & UCTXIFG));
       RXdata = UCA1RXBUF;
       UCA1TXBUF = UCA1RXBUF; //Echo character to terminal
       LPM1_EXIT;
       break;
     case 0x04: break; //Vector 4: UCTXIFG
     case 0x06: break; //Vector 6: UCSTTIFG
     case 0x08: break; //Vector 8: UCTXCPTIFG
   }
 }

void SetPins(void)
  {
	PM5CTL0 &= ~LOCKLPM5; //Unlocks GPIO pins at power-up
	/* Port 1
 	P1.0 Green LED
 	P1.1 Launchpad switch
 	P1.5 Chip select.  Pull this line low to enable AD5160 communication
 	P1.6 MOSI
 	P1.7 MISO
 	*/
 	P1DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5;
 	P1SEL1 |= BIT6 + BIT7; //Configure pins for SPI on UCB0
 	P1OUT &= ~BIT0; //LED off

	/* Port 2
 	P2.1  Button on Launchpad
 	P2.2 SPI CLK
 	P2.5 TXD UART
 	P2.6 RXD UART
	*/
 	P2DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT7;
 	P2SEL1 |= BIT2 + BIT5 + BIT6; //Configure pins for SPI CLK; UART on UCA1

 	/* Port 3 */
 	P3DIR |=  BIT0 + BIT1 + BIT3 + BIT4 + BIT5 + BIT6 + BIT7;

 	/* Port 4
 	P4.6 Red LED
 	*/
 	P4DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5 + BIT6 + BIT7;
 	P4OUT &= ~BIT6; //LED off
  }

 void SetUART(void) //UCA1 module; use RX interrupt
  {
 	 UCA1CTLW0 |= UCSWRST;
 	//Next line selects SMCLK which is DCO (default frequency: 1 MHz)
 	 UCA1CTLW0 |=  UCSSEL1; //This writes 0x80 which sets BIT7
 	 //Next two lines divide 1 MHz to get 9600 baud
 	 UCA1BRW = 0x06;
 	 UCA1MCTLW |= UCOS16 + UCBRF3 + UCBRS5;
 	 UCA1CTLW0 &= ~UCSWRST;
 	 UCA1IE |= UCRXIE; //Enable RX interrupt
  }

 void SetSPI(void)
   {
	 // Configure the eUSCI_B0 module for 3-pin SPI at 1 MHz
	 UCB0CTLW0 |= UCSWRST;
  	 // Use SMCLK at 1 MHz;
	 UCB0CTLW0 |= UCSSEL__SMCLK + UCMODE_0 + UCMST + UCSYNC + UCMSB + UCCKPH;
	 //UCB0BR0 |= 0x02; //Divide SMCLK by 2 to clock at 500 kHz
	 UCB0BR0 |= 0x01; //1 MHz SPI clock
	 UCB0BR1 |= 0x00;
	 UCB0CTLW0 &= ~UCSWRST;
   }

 void DigPot(uint8_t trimpot) //Send single byte (in decimal) to dig pot via SPI
  {
	volatile unsigned char LoopBack;
	volatile uint8_t R[] = {0,26,51,77,102,128,154,179,205,230,255};

 	P1OUT &= ~BIT5; //Pull select line low on P1.5
 	UCB0STAT |= UCLISTEN;  //Enable loopback mode since no MISO line
 	UCB0CTL1 &= ~UCSWRST; //Start USCI

 	while (!(UCB0IFG & UCTXIFG)); //Check if it is OK to write
 	UCB0TXBUF = R[trimpot]; //Load data into transmit buffer

 	while (!(UCB0IFG & UCRXIFG)); //Wait until complete RX byte is received
 	LoopBack = UCB0RXBUF; //Read buffer to clear RX flag

 	UCB0CTL1 |= UCSWRST; //Stop USCI
 	UCB0STAT &= ~UCLISTEN;  //Disable loopback mode
 	P1OUT |= BIT5; //De-select digi pot on SPI
  }

 void Invalid(void)
 {
	 char str1[25];
	 sprintf(str1,"%s", "  ** Invalid entry **");
	 count = sizeof str1;
	 for (i=0; i < count; i++)
	 {
		 while (!(UCA1IFG & UCTXIFG));
		 UCA1TXBUF = str1[i];
	 }
 }


