/* ECE 3849 - Lab 1
 * Yiðit Uyan & Nam Tran
 * Submitted: 11/XX/2016
 * * * * * * * * * *  * */

/* Libraries */
#include "inc/hw_types.h"					// Stellaris Common Type Library
#include "inc/hw_ints.h"					// Stellaris Hardware Interrupt Library
#include "inc/hw_memmap.h"					// Stellaris Memory Map Library
#include "inc/hw_sysctl.h"					// Stellaris System Control Library
#include "inc/lm3s8962.h"					// Hardware Register Definitions
#include "driverlib/sysctl.h"				// System Control Driver Prototypes
#include "driverlib/timer.h"				// Timer Driver Prototypes
#include "driverlib/interrupt.h"			// Interrupt Driver Prototypes
#include "driverlib/gpio.h"					// GPIO Driver Prototypes
#include "drivers/rit128x96x4.h"			// Screen (OLED) Display Driver
#include "utils/ustdlib.h"					// Standard Library Prototypes
#include "frame_graphics.h"					// Easy Display Functions
#include "buttons.h"						// Easy Button Functions
#include <math.h>

/* Function Prototypes */
void TimerISR(void);
void checkButtons(void);
void incrementTime(void);
static void calculateTime(unsigned long);

void printTime();

/* Definitions */
#define BUTTON_CLOCK 200					// Button Scan Rate (Hz)
#define PI 3.14159265358979323846			// PI Constant

/* Global Variables */
unsigned long g_ulSystemClock;				// System Clock Frequency (Hz)
volatile unsigned long g_ulTime = 0;		// Current Time (in 10x miliseconds)

static int running = true;
static int centiSeconds = 0;
static int seconds = 0;
static int minutes = 0;
static int hours = 0;
static char timeString [50];

/* Interrupt Function */
void TimerISR(void)
{
	checkButtons();
	incrementTime();
}

/* Support Functions */
void incrementTime()
{
	static int tic = false;

	if(running)
	{
		if(tic)
		{
			g_ulTime++;
			tic = false;
		}
		else
		tic = true;
	}
}

static void calculateTime(unsigned long currentTime)
{
	centiSeconds = currentTime % 100; // 10 Miliseconds
	seconds = (currentTime / 100) % 60; // Seconds
	minutes = ((currentTime / 100) / 60) % 60; // Minutes
	hours = ((currentTime / 100) / 60) / 60; // Hours (Not Used)
}

void checkButtons(void)
{
	unsigned long presses = g_ulButtons;

	TIMER0_ICR_R = TIMER_ICR_TATOCINT; // Clears Interrupt Flag (Timer0)
	ButtonDebounce((~GPIO_PORTE_DATA_R & (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3)) << 1);
	ButtonDebounce((~GPIO_PORTF_DATA_R & GPIO_PIN_1) >> 1);
	presses = ~presses & g_ulButtons; // A List of Button Presses

	if(presses & 1)			// Select Button
	{
		running = !running;
	}
	else if(presses & 16)	// Right Button
	{
		if(g_ulTime > 6000)
			g_ulTime -= 6000;
	}
	else if(presses & 8)	// Left Button
	{
		g_ulTime += 6000;
	}
	else if(presses & 4)	// Down Button
	{
		if(g_ulTime > 100)
			g_ulTime -= 100;
	}
	else if(presses & 2)	// Up Button
	{
		g_ulTime += 100;
	}
}

void initializeSystemClock()
{
	if (REVISION_IS_A2)
		SysCtlLDOSet(SYSCTL_LDO_2_75V);
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
	g_ulSystemClock = SysCtlClockGet(); // Somehow a 50 MHz System Clock is generated.
}

void initializeDisplay()
{
	RIT128x96x4Init(3500000); // Initialize SPI Interface for Display (3.5 MHz)
}

void initializeButtonClock()
{
	// Local Variables:
		unsigned long ulDivider;
		unsigned long ulPrescaler;

		// initialize a general purpose timer for periodic interrupts
		SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
		TimerDisable(TIMER0_BASE, TIMER_BOTH);
		TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);

		// prescaler for a 16-bit timer
		ulPrescaler = (g_ulSystemClock / BUTTON_CLOCK - 1) >> 16;

		// 16-bit divider (timer load value)
		ulDivider = g_ulSystemClock / (BUTTON_CLOCK * (ulPrescaler + 1)) - 1;
		TimerLoadSet(TIMER0_BASE, TIMER_A, ulDivider);
		TimerPrescaleSet(TIMER0_BASE, TIMER_A, ulPrescaler);
		TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
		TimerEnable(TIMER0_BASE, TIMER_A);
}

void initializeButtonInterface()
{
	// configure GPIO used to read the state of the on-board push buttons
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

	// initialize interrupt controller to respond to timer interrupts
	IntPrioritySet(INT_TIMER0A, 0); // 0 = highest priority, 32 = next lower
	IntEnable(INT_TIMER0A);
	IntMasterEnable();
}

void printTime()
{
	calculateTime(g_ulTime);
	usprintf(timeString, "Time = %02d:%02d:%02d", minutes, seconds, centiSeconds);
	DrawString(0, 0, timeString, 15, false);
}

void screenClean()
{
	FillFrame(0);

}

void screenDraw()
{
	RIT128x96x4ImageDraw(g_pucFrame, 0, 0, FRAME_SIZE_X, FRAME_SIZE_Y);
}

void initializeADC()
{

}

void initializeScreen()
{

}


/* Main Function */
int main(void)
{
	initializeSystemClock();
	initializeDisplay();
	initializeButtonClock();
	initializeButtonInterface();

	// ADC Initializations: (LAB1)
	initializeADC();


	// Screen Initializations: (LAB1)
	initializeScreen();

	while(true)
	{
		screenClean();
		printTime();

		// We need functions for these here:
			// Redraw the Screen Buffer/Frame Here
			// Print Button-Tied Variables on Screen
			// Print What Oscilloscope sees on Screen

		screenDraw();
	}

	return 0;
}
