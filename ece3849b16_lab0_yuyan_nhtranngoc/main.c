/* ECE 3849 - Lab 0
 * Yiðit Uyan & Nam Tran
 * Submitted: 11/02/2016
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

/* Definitions */
#define BUTTON_CLOCK 200					// Button Scan Rate (Hz)
#define PI 3.14159265358979323846			// PI Constant

/* Global Variables */
unsigned long g_ulSystemClock;				// System Clock Frequency (Hz)
volatile unsigned long g_ulTime = 38345;	// Current Time (in 10x miliseconds)

/* Interrupt Function */
void TimerISR(void)
{
	static int tic = false;
	static int running = true;
	unsigned long presses = g_ulButtons;

	TIMER0_ICR_R = TIMER_ICR_TATOCINT; // Clears Interrupt Flag (Timer0)
	ButtonDebounce((~GPIO_PORTE_DATA_R & (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3)) << 1);
	ButtonDebounce((~GPIO_PORTF_DATA_R & GPIO_PIN_1) >> 1);
	presses = ~presses & g_ulButtons; // A List of Button Presses

	// Buttons Checks: 16-right, 8-left, 4-down, 2-up, 1-Select
	if(presses & 1) // Select Button
	{
		running = !running;
	}
	else if(presses & 16|| presses & 8|| presses & 4|| presses & 2) // Directional Buttons
	{
		g_ulTime = 0;
	}

	if(running) // If Running, Increment Time at Every *Other* ISR Call
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

/* Support Functions */
void printTime(long currentTime, char* target)
{
	int ff = currentTime % 100; // 10 Miliseconds
	int ss = (currentTime / 100) % 60; // Seconds
	int mm = ((currentTime / 100) / 60) % 60; // Minutes
	// int hh = ((currentTime / 100) / 60) / 60; // Hours (Not Used)
	usprintf(target, "Time = %02d:%02d:%02d", mm, ss, ff);
}

void drawClock(long currentTime)
{
	int ss = (currentTime / 100) % 60;
	int mm = ((currentTime / 100) / 60) % 60;

	int i = 0;
	while(i < ss) // Draw Seconds
	{
		int shine = 4;
		if(i % 5 == 0)
			shine = 14;
		DrawPoint(64 - 28 * cosf((90 - (-1) * i * 6) * PI / 180.0), 54 - 28 * sinf((90 - (-1) * i * 6) * PI / 180.0), shine);
		i++;
	}

	int e = 0;
	while(e < mm) // Draw Minutes
	{
		int shn = 5;
		if(e % 5 == 0)
			shn = 14;
		DrawPoint(64 - 19 * cosf((90 - (-1) * e * 6) * PI / 180.0), 54 - 19 * sinf((90 - (-1) * e * 6) * PI / 180.0), shn);
		e++;
	}
}

void drawFrame() // Draw Clock Frame
{
	int i = 0;
	while(i < 60)
	{
		DrawPoint(64 - 37 * cosf((90 - (-1) * i * 6) * PI / 180.0), 54 - 37 * sinf((90 - (-1) * i * 6) * PI / 180.0), i % 15);
		i++;
	}
}

/* Main Function */
int main(void)
{
	// Initialize System Clock:
	if (REVISION_IS_A2)
		SysCtlLDOSet(SYSCTL_LDO_2_75V);
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
	g_ulSystemClock = SysCtlClockGet(); // Somehow a 50 MHz System Clock is generated.

	RIT128x96x4Init(3500000); // Initialize SPI Interface for Display (3.5 MHz)

	// Local Variables:
	char pcStr[50];
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

	while (true)
	{
		FillFrame(0);						// Clears Frame Buffer
		printTime(g_ulTime, pcStr);			// Creates Time String
		DrawString(0, 0, pcStr, 15, false); // Puts Time String on Buffer
		drawFrame();						// Draws the Clock (Analog) Frame
		drawClock(g_ulTime);				// Draws Time Elements (Minutes & Seconds)
		RIT128x96x4ImageDraw(g_pucFrame, 0, 0, FRAME_SIZE_X, FRAME_SIZE_Y);	// Draws Buffer on Screen
	}

	return 0;
}
