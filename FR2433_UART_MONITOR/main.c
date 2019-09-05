// ----------------------------------------------------------------------
// 	info and license
// ----------------------------------------------------------------------
//
//	filename:	main.c
//
//	MIT License
//
// 	Copyright (c) 2019 Benjamin Prescher
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.
//
//	target:		Texas Instruments MSP430 FR2433 LP
//
//  Info:
//  This project shows the possibility to view and change variables via a serial
//  interface (UART) during the program runtime. All globally visible variables
//  can be entered and monitored as usual in the "Expression window" in debug mode.
//  A software routine provided by TI is used. This works with addresses of the variables.
//
//  Unfortunately in many cases the backchannel Uart (you'll find them on various launchpads)
//  function is too slow. Therefore, it is recommended to use an external serial to USB adapter.
//
// ----------------------------------------------------------------------
// 	history
// ----------------------------------------------------------------------
// 	05.09.2019 - initial programming

// ----------------------------------------------------------------------
// 	header files
// ----------------------------------------------------------------------
#include <msp430.h>
#include "Serial_Cmd_Monitor.h"

// ----------------------------------------------------------------------
// 	#defines
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// 	global variables
// ----------------------------------------------------------------------
volatile int gCounter = 0;

// ----------------------------------------------------------------------
// 	functions
// ----------------------------------------------------------------------
void Init_GPIO(void);

int main(void)
{
    // ----------------------------------------------------------------------
    //  Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    // ----------------------------------------------------------------------
    //  Configure GPIO
    Init_GPIO();
    PM5CTL0 &= ~LOCKLPM5;                    // Disable the GPIO power-on default high-impedance mode

    // ----------------------------------------------------------------------
    //  Configure clock
    __bis_SR_register(SCG0);                 // disable FLL
    CSCTL3 |= SELREF__REFOCLK;               // Set REFO as FLL reference source
    CSCTL0 = 0;                              // clear DCO and MOD registers
    CSCTL1 &= ~(DCORSEL_7);                  // Clear DCO frequency select bits first
    CSCTL1 |= DCORSEL_3;                     // Set DCO = 8MHz
    CSCTL2 = FLLD_0 + 243;                   // DCODIV = 8MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                 // enable FLL
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)); // Poll until FLL is locked

    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK; // set default REFO(~32768Hz) as ACLK source, ACLK = 32768Hz
    // default DCODIV as MCLK and SMCLK source

    // ----------------------------------------------------------------------
    //  Configure UART pins
    P1SEL0 |= BIT4 | BIT5;                    // set 2-UART pin as second function

    // ----------------------------------------------------------------------
    //  Configure UART
    UCA0CTLW0 |= UCSWRST;
    UCA0CTLW0 |= UCSSEL__SMCLK;

    // Baud Rate calculation
    // 8000000/(16*9600) = 52.083
    // Fractional portion = 0.083
    // User's Guide Table 14-4: UCBRSx = 0x49
    // UCBRFx = int ( (52.083-52)*16) = 1
    UCA0BR0 = 52;                             // 8000000/16/9600
    UCA0BR1 = 0x00;
    UCA0MCTLW = 0x4900 | UCOS16 | UCBRF_1;
    UCA0CTLW0 &= ~UCSWRST;                    // Initialize eUSCI
    UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt

    // ----------------------------------------------------------------------
    // Configure RTC count: 1000/8000000 * 8000 = 1 sec.
    RTCMOD = 8000-1;
    // Initialize RTC
    RTCCTL = RTCSS__SMCLK | RTCSR | RTCPS__1000 | RTCIE;

    // ----------------------------------------------------------------------
    //  UART Monitor is used for debugging:
    ClearBufferRelatedParam(); // this routine is found in Serial_Cmd_Monitor

    // ----------------------------------------------------------------------
    //  no main loop is used in this example - everything is done in ISRs!
    __bis_SR_register(LPM0_bits|GIE);         // Enter LPM0, interrupts enabled
    __no_operation();                         // For debugger
}

#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
{
    switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
    {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
        /* Parse received command immediately */
        P1OUT ^=BIT1;
        receivedDataCommand(UCA0RXBUF);
        break;
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
    default: break;
    }
}

// RTC interrupt service routine
#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void)
{
    switch(__even_in_range(RTCIV,RTCIV_RTCIF))
    {
    case  RTCIV_NONE:   break;          // No interrupt
    case  RTCIV_RTCIF:                  // RTC Overflow
        P1OUT ^= BIT0;
        gCounter++;
        break;
    default: break;
    }
}

void Init_GPIO()
{
    P1DIR = 0xFF; P2DIR = 0xFF; P3DIR = 0xFF;
    P1REN = 0xFF; P2REN = 0xFF; P3REN = 0xFF;
    P1OUT = 0x00; P2OUT = 0x00; P3OUT = 0x00;
}

// ----------------------------------------------------------------------
// 	end of file
// ----------------------------------------------------------------------

