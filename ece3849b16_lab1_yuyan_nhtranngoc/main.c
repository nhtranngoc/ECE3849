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
#include "driverlib/adc.h"					// ADC Driver Prototypes
#include "drivers/rit128x96x4.h"			// Screen (OLED) Display Driver
#include "utils/ustdlib.h"					// Standard Library Prototypes
#include "frame_graphics.h"					// Easy Display Functions
#include "buttons.h"						// Easy Button Functions
#include <math.h>							// Standard C Math Library

/* Function Prototypes */
void initializeClock();						// Initializes System Clock (50 MHz)
void initializeDisplay();					// Initializes Screen SPI (3.5 MHz)
void initializeTimers();					// Initializes Button Clock (200 Hz)
void initializeButtons();					// Sets the Registers for Buttons
void initializeADC();						// ADC Setup Code for Signal Sampling
void initializeScreen();					// Draw Function Outside of Main Loop
void initializeBlinky();					// Sets the Registers for Blinky LED
void processButtons();
void screenClean();
void screenDraw();
void drawGrid();
void drawOverlay();
void drawSignal();
void drawTrigger();
void blink();

unsigned long cpu_load_count(void);
void debugString(int _DebugValue);
int strCenter(int _Coordinate, char* _String);
int strWidth(char* _String);

/* Definitions */
#define TIMER0A_CLOCK		2				// Hearbeat (Blinky) (Hz)
#define TIMER1A_CLOCK		100				// Sample Scan Rate (Hz)
#define TIMER2A_CLOCK		100				// Button Scan Rate (Hz)
#define PI	   3.14159265358979				// PI Constant
#define DISPLAY_WIDTH		128				// Display Horizontal Res.
#define DISPLAY_HEIGHT		96				// Display Vertical Res.
#define BUTTON_BUFFER_SIZE	100				// Size of Button Buffer
#define ADC_BUFFER_SIZE 	2048 			// must be a power of 2
#define ADC_BUFFER_WRAP(i) ((i) & (ADC_BUFFER_SIZE - 1)) // index wrapping macro

#define COLOR_GRID			4				// Grid Color (Backlight)
#define COLOR_SIGNAL		15				// Signal Color (Backlight)
#define COLOR_TEXT			15				// Text Color (Backlight)
#define COLOR_TRIGGER		10				// Trigger Color (Backlight)

/* Global Variables */
const char* const voltageScales[] = {"100mV", "200mV", "500mV", "1V"};
int voltageNScales[] = {100, 200, 500, 1000};
//const char* const timeScales[] = {"10us", "20us", "50us", "100us", "200us"};
//int timeNScales[] = {10, 20, 50, 100, 200};
const char* const timeScales[] = {"10us", "20us", "30us", "40us", "50us", "60us", "70us", "80us", "90us", "100us"};
int timeNScales[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
char buttonArray [BUTTON_BUFFER_SIZE];
unsigned long systemClock;
int buttonArrayCap = 0;
int adcZeroValue = 100;
int triggerIndex = 75;

unsigned long count_unloaded = 0;
unsigned long count_loaded = 0;
float cpu_load = 0.0;

/* ADC Variables */
volatile int g_iADCBufferIndex = ADC_BUFFER_SIZE - 1; // latest sample index
volatile short g_psADCBuffer[ADC_BUFFER_SIZE]; // circular buffer
volatile unsigned long g_ulADCErrors = 0; // number of missed ADC deadlines

short array [200] =
{
	0,0,0,0,0,1,1,1,1,1,
	10,20,30,40,50,60,70,80,90,100,
	110,120,130,140,150,160,170,180,190,200,
	200,190,180,170,160,150,140,130,120,110,
	100,90,80,70,60,50,40,30,20,10,
	0,0,0,0,0,1,1,1,1,1,
	10,20,30,40,50,60,70,80,90,100,
	110,120,130,140,150,160,170,180,190,200,
	200,190,180,170,160,150,140,130,120,110,
	100,90,80,70,60,50,40,30,20,10,
	0,0,0,0,0,1,1,1,1,1,
	10,20,30,40,50,60,70,80,90,100,
	110,120,130,140,150,160,170,180,190,200,
	200,190,180,170,160,150,140,130,120,110,
	100,90,80,70,60,50,40,30,20,10,
	0,0,0,0,0,1,1,1,1,1,
	10,20,30,40,50,60,70,80,90,100,
	110,120,130,140,150,160,170,180,190,200,
	200,190,180,170,160,150,140,130,120,110,
	100,90,80,70,60,50,40,30,20,10
};

int blinkyOn = 1;
int triggerUp = 1;
int buttonIndex = 0;
int selectionIndex = 0;
int voltageIndex = 0;
int timeIndex = 0;
int CPULoad = 0;

// Display Global Definitions:
int barsHorizontal = 13;
int barsVertical = 11;
//int scaleHorizontal = 10;
//int scaleVertical = 10;
int gridXMin = 0;
int gridXMax = DISPLAY_WIDTH - 1;
int gridYMin = 0;
int gridYMax = DISPLAY_HEIGHT - 1;

// Display Global Variables:
int gridXSize = 0;
int gridYSize = 0;
int gridWidth = 0;
int gridHeight = 0;
int alignX = 0;
int alignY = 0;


/* Main Function */
int main(void)
{
	initializeClock();
	initializeDisplay();
	initializeTimers();
	initializeButtons();
	initializeADC();
	//initializeBlinky();

	initializeScreen();

	while(true)
	{
		screenClean();

		// Test FIFO
		processButtons();

		drawGrid();
		drawOverlay();
		drawSignal();
		drawTrigger();

		screenDraw();

		//blink();
	}
}

void drawGrid()
{
	int i = 0;
	while(i < barsHorizontal) // Draw Horizontal Lines
	{
		int x = gridXMin + alignX + i * gridWidth;
		DrawLine(x, gridYMin, x, gridYMax, COLOR_GRID);
		i++;
	}

	i = 0;
	while(i < barsVertical) // Draw Vertical Lines
	{
		int y = gridYMin + alignY + i * gridHeight;
		DrawLine(gridXMin, y, gridXMax, y, COLOR_GRID);
		i++;
	}
}

void drawOverlay()
{
	char voltageString[20];
	char timeString[20];
	char cpuString[20];

	int marginVertical = 3;
	int marginHorizontal = 5;

	// Draw Time Scale:
	usprintf(timeString, "%s", timeScales[timeIndex]);
	DrawString(strCenter(DISPLAY_WIDTH / 6, timeString), marginVertical, timeString, COLOR_TEXT, false);
	CPULoad = strWidth(timeString);
	if(selectionIndex == 0)
		DrawLine(strCenter(DISPLAY_WIDTH / 6, timeString), marginVertical + 8, strCenter(DISPLAY_WIDTH / 6, timeString) + strWidth(timeString), marginVertical + 8, COLOR_TEXT);

	// Draw Voltage Scale:
	usprintf(voltageString, "%s", voltageScales[voltageIndex]);
	DrawString(strCenter(DISPLAY_WIDTH / 2, voltageString), marginVertical, voltageString, COLOR_TEXT, false);

	if(selectionIndex == 1)
		DrawLine(strCenter(DISPLAY_WIDTH / 2, voltageString), marginVertical + 8, strCenter(DISPLAY_WIDTH / 2, voltageString) + strWidth(voltageString), marginVertical + 8, COLOR_TEXT);

	count_loaded = cpu_load_count();
	cpu_load = 1.0 - (float)count_loaded/count_unloaded;

	unsigned int whole = (int) (cpu_load * 100);
	unsigned int frac = (int) (cpu_load * 1000 - whole * 10);

	// Draw CPU Load:
	usprintf(cpuString, "CPU Load = %02u.%01u%%", whole, frac);
	DrawString(marginHorizontal, DISPLAY_HEIGHT - 7 - marginVertical, cpuString, COLOR_TEXT, false);

	int triggerX = ((5 * DISPLAY_WIDTH) / 6) - 8;
	int triggerY = 3;

	// Draw Trigger Selection:
	if(triggerUp)
	{
		DrawLine(triggerX, triggerY + 6, triggerX + 8, triggerY + 6, COLOR_TRIGGER);
		DrawLine(triggerX + 8, triggerY + 6, triggerX + 8, triggerY, COLOR_TRIGGER);
		DrawLine(triggerX + 8, triggerY, triggerX + 16, triggerY, COLOR_TRIGGER);
	}
	else
	{
		DrawLine(triggerX, triggerY, triggerX + 8, triggerY, COLOR_TRIGGER);
		DrawLine(triggerX + 8, triggerY + 6, triggerX + 8, triggerY, COLOR_TRIGGER);
		DrawLine(triggerX + 8, triggerY + 6, triggerX + 16, triggerY + 6, COLOR_TRIGGER);
	}
}

void debugString(int _DebugValue)
{
	char debugString [20];
	usprintf(debugString, "Debug = %d", _DebugValue);
	DrawString(4, 70, debugString, COLOR_TEXT, false);
}

int strWidth(char* _String)
{
	int offset = 0;
	int count = 0;

	while (*(_String + offset) != '\0')
	{
		++count;
		++offset;
	}
	return count * 6;
}

int strCenter(int _Coordinate, char* _String)
{
	int result = _Coordinate - (strWidth(_String) / 2);

	if(result > 0 && result < DISPLAY_WIDTH)
		return result;
	else if(result > DISPLAY_WIDTH)
		return DISPLAY_WIDTH;
	else
		return 0;
}

void drawTrigger()
{
	int y = (gridYMax - gridYMin) / 2;
	DrawLine(gridXMin, y, gridXMax, y, COLOR_TRIGGER);
}

void drawSignal()
{
	int i = 0;
	int j = 0;
	int pixelRange = gridXMax - gridXMin;
	float pixelWidth = timeNScales[timeIndex] / gridWidth;
	int pixelBuffer [DISPLAY_WIDTH - 1];

	i = pixelRange / 2;
	j = 0;
	while(i < pixelRange)
	{
		if(triggerIndex + (int)((j * pixelWidth) / 2) < 200)
			pixelBuffer[i] = array[triggerIndex + (int)((j * pixelWidth) / 2)];
		else
			pixelBuffer[i] = 0;
		i++;
		j++;
	}


	i = pixelRange / 2;
	j = 0;
	while(i > 0)
	{
		if(triggerIndex - (int)((j * pixelWidth) / 2) > 0)
			pixelBuffer[i] = array[triggerIndex - (int)((j * pixelWidth) / 2)];
		else
			pixelBuffer[i] = 0;
		i--;
		j++;
	}

	i = 0;
	while(i < pixelRange)
	{
		int offsetY = gridYMin + (gridYMin + gridYMax) / 2;
		DrawPoint(gridXMin + i, offsetY - (pixelBuffer[i] - adcZeroValue) / (voltageNScales[voltageIndex] / 100), COLOR_SIGNAL);
		i++;
	}

	/*IntMasterDisable();
	while(i < pixelRange && i < g_iADCBufferIndex)
	{
		//DrawPoint(gridXMin + i, g_psADCBuffer[i], COLOR_SIGNAL);
		//int buffer_index = ADC_BUFFER_WRAP(g_iADCBufferIndex + 1);
		int val = (int) g_psADCBuffer[i]; // read sample from the ADC sequence0 FIFO
		int offsetY = gridYMin + (gridYMin + gridYMax) / 2;
		DrawPoint(gridXMin + i, offsetY - (array[i] - adcZeroValue) / (voltageNScales[voltageIndex] / 100), COLOR_SIGNAL);
		i++;
	}
	//debugString(i);
	IntMasterEnable();*/
}

void initializeClock()
{
	if (REVISION_IS_A2)
		SysCtlLDOSet(SYSCTL_LDO_2_75V);
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
	systemClock = SysCtlClockGet(); // Somehow a 50 MHz System Clock is generated.
}

void initializeDisplay()
{
	RIT128x96x4Init(3500000); // Initialize SPI Interface for Display (3.5 MHz)
}

void initializeTimers()
{
	// Local Variables:
	unsigned long ulDivider;
	unsigned long ulPrescaler;

	// initialize a general purpose timer for periodic interrupts
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
	TimerDisable(TIMER0_BASE, TIMER_BOTH);
	TimerDisable(TIMER1_BASE, TIMER_BOTH);
	TimerDisable(TIMER2_BASE, TIMER_BOTH);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);
	TimerConfigure(TIMER1_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);
	TimerConfigure(TIMER2_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PERIODIC);
	ulPrescaler = (systemClock / TIMER0A_CLOCK - 1) >> 16;
	ulDivider = systemClock / (TIMER0A_CLOCK * (ulPrescaler + 1)) - 1;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ulDivider);
	TimerPrescaleSet(TIMER0_BASE, TIMER_A, ulPrescaler);
	ulPrescaler = (systemClock / TIMER1A_CLOCK - 1) >> 16;
	ulDivider = systemClock / (TIMER1A_CLOCK * (ulPrescaler + 1)) - 1;
	TimerLoadSet(TIMER1_BASE, TIMER_A, ulDivider);
	TimerPrescaleSet(TIMER1_BASE, TIMER_A, ulPrescaler);
	ulPrescaler = (systemClock / TIMER2A_CLOCK - 1) >> 16;
	ulDivider = systemClock / (TIMER2A_CLOCK * (ulPrescaler + 1)) - 1;
	TimerLoadSet(TIMER2_BASE, TIMER_A, ulDivider);
	TimerPrescaleSet(TIMER2_BASE, TIMER_A, ulPrescaler);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
	TimerIntEnable(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	TimerEnable(TIMER0_BASE, TIMER_A);
	TimerEnable(TIMER1_BASE, TIMER_A);
	TimerEnable(TIMER2_BASE, TIMER_A);

	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);// initialize timer 3 in one-shot mode for polled timing
	TimerDisable(TIMER3_BASE, TIMER_BOTH);			//Disables the timer
	TimerConfigure(TIMER3_BASE, TIMER_CFG_ONE_SHOT);//Configure timer for one shot mode
	TimerLoadSet(TIMER3_BASE, TIMER_A, systemClock / 50 - 1); // 0.02 sec interval
}


void initializeButtons()
{
	// configure GPIO used to read the state of the on-board push buttons
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

	// initialize interrupt controller to respond to timer interrupts
	IntPrioritySet(INT_TIMER0A, 64);	// Heartbeat (Low Priority)
	IntPrioritySet(INT_TIMER1A, 32);	// Signal Sample (Mid Priority)
	IntPrioritySet(INT_TIMER2A, 0);		// Button Read (High Priority)

	IntEnable(INT_TIMER0A);
	IntEnable(INT_TIMER1A);
	IntEnable(INT_TIMER2A);

	count_unloaded = cpu_load_count();
	IntMasterEnable();
}

void initializeBlinky()
{
	GPIO_PORTF_DIR_R = 0x01;
	GPIO_PORTF_DEN_R = 0x01;
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
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // enable the ADC
	SysCtlADCSpeedSet(SYSCTL_ADCSPEED_500KSPS); // specify 500ksps
	ADCSequenceDisable(ADC0_BASE, 0); // choose ADC sequence 0; disable before configuring
	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_ALWAYS, 0); // specify the "Always" trigger, highest priority
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END); // in the 0th step, sample channel 0
	 // enable interrupt, and make it the end of sequence
	ADCIntEnable(ADC0_BASE, 0); // enable ADC interrupt from sequence 0
	ADCSequenceEnable(ADC0_BASE, 0); // enable the sequence. it is now sampling
	IntPrioritySet(INT_ADC0, 32); // set ADC interrupt priority in the interrupt controller - lower than main ISR
	IntEnable(INT_ADC0); // enable ADC interrupt
}

void initializeScreen()
{
	// Grid Initializations:
	gridXSize = gridXMax - gridXMin;
	gridYSize = gridYMax - gridYMin;
	gridWidth = gridXSize / (barsHorizontal - 1);
	gridHeight = gridYSize / (barsVertical - 1);
	alignX = (gridXSize % (barsHorizontal - 1)) / 2;
	alignY = (gridYSize % (barsVertical - 1)) / 2;
}

void Timer0AISR()
{
	TIMER0_ICR_R = TIMER_ICR_TATOCINT;

	if(blinkyOn)
	{
		blinkyOn = 0;
	}
	else
	{
		blinkyOn = 1;
	}
}

void Timer1AISR()
{
	TIMER1_ICR_R = TIMER_ICR_TATOCINT;
	//CPULoad++;
	if(CPULoad > 99)
		CPULoad = 0;
}

void Timer2AISR()
{
	unsigned long presses = g_ulButtons;
	TIMER2_ICR_R = TIMER_ICR_TATOCINT;
	ButtonDebounce((~GPIO_PORTE_DATA_R & (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3)) << 1);
	ButtonDebounce((~GPIO_PORTF_DATA_R & GPIO_PIN_1) >> 1);
	presses = ~presses & g_ulButtons; // A List of Button Presses

	if(presses & 1)			// Select Button
	{
		if(buttonArrayCap < BUTTON_BUFFER_SIZE)
		{
			buttonArray[buttonArrayCap] = 'S';
			buttonArrayCap++;
		}
	}
	else if(presses & 16)	// Right Button
	{
		if(buttonArrayCap < BUTTON_BUFFER_SIZE)
		{
			buttonArray[buttonArrayCap] = 'R';
			buttonArrayCap++;
		}
	}
	else if(presses & 8)	// Left Button
	{
		if(buttonArrayCap < BUTTON_BUFFER_SIZE)
		{
			buttonArray[buttonArrayCap] = 'L';
			buttonArrayCap++;
		}
	}
	else if(presses & 4)	// Down Button
	{
		if(buttonArrayCap < BUTTON_BUFFER_SIZE)
		{
			buttonArray[buttonArrayCap] = 'D';
			buttonArrayCap++;
		}
	}
	else if(presses & 2)	// Up Button
	{
		if(buttonArrayCap < BUTTON_BUFFER_SIZE)
		{
			buttonArray[buttonArrayCap] = 'U';
			buttonArrayCap++;
		}
	}
}

void ADC_ISR(void)
{
	ADC0_ISC_R = ADC_ISC_IN0; // clear ADC sequence0 interrupt flag in the ADCISC register

	if (ADC0_OSTAT_R & ADC_OSTAT_OV0) { // check for ADC FIFO overflow
		g_ulADCErrors++; // count errors - step 1 of the signoff
		ADC0_OSTAT_R = ADC_OSTAT_OV0; // clear overflow condition
	}
	int buffer_index = ADC_BUFFER_WRAP(g_iADCBufferIndex + 1);
	g_psADCBuffer[buffer_index] = ADC_SSFIFO0_R; // read sample from the ADC sequence0 FIFO
	g_iADCBufferIndex = buffer_index;
}

void blink()
{
	if(blinkyOn)
	{
		//GPIO_PORTF_DATA_R |= 0x01;
	}
	else
	{
		//GPIO_PORTF_DATA_R &= ~(0x01);
	}
}

void processButtons()
{
	if(buttonArrayCap > 0)
	{

		char currentEntry = buttonArray[buttonArrayCap - 1];
		buttonArrayCap--;

		if(currentEntry == 'S' || currentEntry == 's')
		{
			if(triggerUp)
				triggerUp = 0;
			else
				triggerUp = 1;
		}
		if(currentEntry == 'R' || currentEntry == 'r')
		{
			if(selectionIndex < 1)
				selectionIndex++;
		}
		if(currentEntry == 'L' || currentEntry == 'l')
		{
			if(selectionIndex > 0)
				selectionIndex--;
		}
		if(currentEntry == 'D' || currentEntry == 'd')
		{
			if(selectionIndex == 0)
			{
				if(timeIndex > 0)
					timeIndex--;
			}
			else
			{
				if(voltageIndex > 0)
					voltageIndex--;
			}
		}
		if(currentEntry == 'U' || currentEntry == 'u')
		{
			if(selectionIndex == 0)
			{
				if(timeIndex < 9)
					timeIndex++;
			}
			else
			{
				if(voltageIndex < 3)
					voltageIndex++;
			}
		}
	}
}

unsigned long cpu_load_count(void)
{
	unsigned long i = 0;
	TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
	TimerEnable(TIMER3_BASE, TIMER_A); // start one-shot timer
	while (!(TimerIntStatus(TIMER3_BASE, 0) & TIMER_TIMA_TIMEOUT))
		i++;
	return i;
}


/*

// FIFO - DataType from Class Example: (Integrate..)


#define FIFO_SIZE 11		// FIFO capacity is 1 item fewer
typedef char DataType;		// FIFO data type

volatile DataType fifo[FIFO_SIZE];	// FIFO storage array
volatile int fifo_head = 0;	// index of the first item in the FIFO
volatile int fifo_tail = 0;	// index one step past the last item

int fifo_put(DataType data)
{
	int new_tail = fifo_tail + 1;
	if (new_tail >= FIFO_SIZE) new_tail = 0; // wrap around
	if (fifo_head != new_tail) {	// if the FIFO is not full
		fifo[fifo_tail] = data;		// store data into the FIFO
		fifo_tail = new_tail;		// advance FIFO tail index
		return 1;					// success
	}
	return 0;	// full
}

int fifo_get(DataType *data)
{
	if (fifo_head != fifo_tail) {	// if the FIFO is not empty
		*data = fifo[fifo_head];	// read data from the FIFO
//		IntMasterDisable();
//		delay_us(1000);
		fifo_head++;				// advance FIFO head index
//		IntMasterEnable();
		if (fifo_head >= FIFO_SIZE) fifo_head = 0; // wrap around
//		IntMasterEnable();
		return 1;					// success
	}
	return 0;	// empty
}

//		if (fifo_head == FIFO_SIZE - 1)
//			fifo_head = 0;
//		else
//			fifo_head++;
 */
