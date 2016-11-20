/* ECE 3849 - Lab 1
 * Yiðit Uyan & Nam Tran
 * Submitted: 11/XX/2016
 * * * * * * * * * *  * */

/* Included Libraries */
#include "inc/hw_types.h"							// Stellaris Common Type Library
#include "inc/hw_ints.h"							// Stellaris Hardware Interrupt Library
#include "inc/hw_memmap.h"							// Stellaris Memory Map Library
#include "inc/hw_sysctl.h"							// Stellaris System Control Library
#include "inc/lm3s8962.h"							// Hardware Register Definitions
#include "driverlib/sysctl.h"						// System Control Driver Prototypes
#include "driverlib/timer.h"						// Timer Driver Prototypes
#include "driverlib/interrupt.h"					// Interrupt Driver Prototypes
#include "driverlib/gpio.h"							// GPIO Driver Prototypes
#include "driverlib/adc.h"							// ADC Driver Prototypes
#include "drivers/rit128x96x4.h"					// Screen (OLED) Display Driver
#include "utils/ustdlib.h"							// Standard Library Prototypes
#include "frame_graphics.h"							// Easy Display Functions
#include "buttons.h"								// Easy Button Functions
#include <math.h>									// Standard C Math Library

/* Function Prototypes */
void initializeClock();								// Initializes System Clock (50 MHz)
void initializeDisplay();							// Initializes Screen SPI (3.5 MHz)
void initializeTimers();							// Initializes Button Clock (200 Hz)
void initializeButtons();							// Sets the Registers for Buttons
void initializeADC();								// ADC Setup Code for Signal Sampling
void initializeScreen();							// Draw Function Outside of Main Loop
void processButtons();								// Read and Process the B.Press from Queue
void screenClean();									// Cleans the Screen Before next Draw
void screenDraw();									// Pushes the Screen Buffer to Display
void drawGrid();									// Draws the Grid for Oscillo-noscope
void drawOverlay();									// Draws the Overlay Elements (eg. Text)
void drawSignal(short* _InputBuffer);				// Draws the Signal to the Screen
void drawTrigger();									// Draws the Trigger Line (y = 0 for this lab)
int triggerSearch(int direction, int triggerValue);	// Returns the next Index for Trigger

unsigned long cpu_load_count(void);					// Estimates the CPU Load based on Missed IRQs
int strCenter(int _Coordinate, char* _String);		// Returns x-Value for Centered String
int strWidth(char* _String);						// Returns the Calculated Width of the String

/* Definitions */
#define TIMER0A_CLOCK		2						// Hearbeat (Blinky) (Hz)
#define TIMER1A_CLOCK		100						// Sample Scan Rate (Hz)
#define TIMER2A_CLOCK		100						// Button Scan Rate (Hz)
#define PI	   3.14159265358979						// PI Constant
#define DISPLAY_WIDTH		128						// Display Horizontal Res.
#define DISPLAY_HEIGHT		96						// Display Vertical Res.
#define BUTTON_BUFFER_SIZE	100						// Size of Button Buffer
#define ADC_BUFFER_SIZE 	2048 					// Size of ADC Buffer (Should be 2^n ??)
#define ADC_BUFFER_WRAP(i) ((i) & (ADC_BUFFER_SIZE - 1)) // index wrapping macro

#define COLOR_GRID			4						// Grid Color (Backlight)
#define COLOR_SIGNAL		15						// Signal Color (Backlight)
#define COLOR_TEXT			15						// Text Color (Backlight)
#define COLOR_TRIGGER		10						// Trigger Color (Backlight)

/* Global Variables */

const char* const timeScales[] = {"10us", "20us", "30us", "40us", "50us", "60us", "70us", "80us", "90us", "100us"};
int timeNScales[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
const char* const voltageScales[] = {"100mV", "200mV", "500mV", "1V"};
int voltageNScales[] = {100, 200, 500, 1000};

int pixelBuffer [DISPLAY_WIDTH - 1];				// Value Array for Signal Display
char buttonArray [BUTTON_BUFFER_SIZE];				// Queue for Button Input
unsigned long systemClock;							// System Running Clock
int buttonArrayCap = 0;								// Cap Value for Button Array
int adcZeroValue = 525;								// ADC Zero Value (Measured)
int triggerIndex = 1024;							// The Starting Index of the Trigger

unsigned long count_unloaded = 0;					// Value for CPU Measurement (IRQs Disabled)
unsigned long count_loaded = 0;						// Value for CPU Measurement (IRQs Enabled)
float cpu_load = 0.0;								// The Calculated (Estimated) CPU Load Value

/* ADC Variables */
volatile int g_iADCBufferIndex = ADC_BUFFER_SIZE - 1;	// Index of the Last ADC Sample
volatile short g_psADCBuffer[ADC_BUFFER_SIZE]; 			// Circular Buffer for ADC Samples
volatile unsigned long g_ulADCErrors = 0; 				// Missed ADC Deadline Count

int blinkyOn = 1;									// Heartbeat (Blinky) Start Value (Not Used 4 Now!)
int triggerUp = 1;									// Starting Direction of the Trigger (1 - UP, 0 - DOWN)
int selectionIndex = 0;								// Starting Position of the Scale Selection (0 - Time, 1 - Voltage)
int voltageIndex = 0;								// Starting Index of the Voltage Scale (100mV)
int timeIndex = 0;									// Starting Index of the Time Scale (10us)
int CPULoad = 0;									// Starting CPU Load Value

// Display Global Definitions:
int barsHorizontal = 13;							// Number of Horizontal Lines on Screen Grid
int barsVertical = 11;								// Number of Vertical Lines on Screen Grid
int gridXMin = 0;									// The Starting X Position of Screen Grid
int gridXMax = DISPLAY_WIDTH - 1;					// The Ending X Position of Screen Grid
int gridYMin = 0;									// The Starting Y Position of Screen Grid
int gridYMax = DISPLAY_HEIGHT - 1;					// The Ending Y Position of the Screen Grid

// Display Global Variables:
int gridXSize = 0;									// Total Width of the Screen Grid
int gridYSize = 0;									// Total Height of the Screen Grid
int gridWidth = 0;									// Width of a Cell in Screen Grid
int gridHeight = 0;									// Height of a Cell in Screen Grid
int alignX = 0;										// X Value for Centering the Grid on Screen
int alignY = 0;										// Y Value for Centering the Grid on Screen


/* Main Function */
int main(void)
{
	initializeClock();								// Initialize the System Clock
	initializeDisplay();							// Initialize the Screen/Display
	initializeTimers();								// Initialize IRQs for Timers
	initializeButtons();							// Initialize Buttons
	initializeADC();								// Initialize IRQ for ADC

	initializeScreen();

	while(true)
	{
		screenClean();

		processButtons();

		drawGrid();
		drawOverlay();
		drawSignal(g_psADCBuffer);
		drawTrigger();

		screenDraw();
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
	char voltageString[20];							// Current Voltage Scale Selection (String)
	char timeString[20];							// Current Time Scale Selection (String)
	char cpuString[20];								// Current CPU Load (String)

	int marginVertical = 3;							// Text Alignment (Vertical Margin)
	int marginHorizontal = 5;						// Text Alignment (Horizontal Margin)

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

	count_loaded = cpu_load_count();						// CPU Reading Undex Max Load (IRQs Enabled)
	cpu_load = 1.0 - (float)count_loaded/count_unloaded;	// CPU Load Calculation

	unsigned int wholePart = (int)(cpu_load * 100);
	unsigned int fractionPart = (int)(cpu_load * 1000 - wholePart * 10);

	// Draw CPU Load:
	usprintf(cpuString, "CPU Load = %02u.%01u%%", wholePart, fractionPart);
	DrawString(marginHorizontal, DISPLAY_HEIGHT - 7 - marginVertical, cpuString, COLOR_TEXT, false);

	// Values for Trigger Selection Image:
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

int strWidth(char* _String)
{
	int offset = 0;
	int count = 0;

	while (*(_String + offset) != '\0')
	{
		++count;
		++offset;
	}
	return count * 6;								// Each Character has 6 Pixel Width
}

int strCenter(int _Coordinate, char* _String)
{
	// Attempts to center given String at target Coordinate:
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
	int y = (gridYMax - gridYMin) / 2;				// Trigger Line Always at the Center
	DrawLine(gridXMin, y, gridXMax, y, COLOR_TRIGGER);
}

// Direction 0 for slope down, 1 for slope up
/*int triggerSearch(int direction, int triggerValue) {
	triggerIndex = g_iADCBufferIndex - (FRAME_SIZE_Y/2); // Initialize to half a screen behind

	int i;
	for(i=0;i< ADC_BUFFER_SIZE/2;i++) { // All my life I have been searching for something
		int pastSample = g_psADCBuffer[triggerIndex -1];
		int currentSample = g_psADCBuffer[triggerIndex];
		if (direction) { // Slope up
			if ((pastSample < triggerValue) && (currentSample >= triggerValue)) {
				return triggerIndex;
			}
			else triggerIndex = ADC_BUFFER_WRAP(--triggerIndex); // Wrap it back
		} else { // Slope down
			if ((pastSample > triggerValue) && (currentSample <= triggerValue)) {
				return triggerIndex;
			} else triggerIndex = ADC_BUFFER_WRAP(--triggerIndex); // Wrap it back
		}
	}

	// If for loop isn't terminated, meaning it failed to find a sample
	//g_ulTriggerSearchFail++; // Increment fail counter
	return ADC_BUFFER_WRAP(g_iADCBufferIndex - (ADC_BUFFER_SIZE/2));
}*/

void drawSignal(short* _InputBuffer)
{
	int i = 0;
	int j = 0;
	//int inputBufferSize = sizeof(_InputBuffer) / sizeof(_InputBuffer[0]);
	int pixelRange = gridXMax - gridXMin;
	float pixelWidth = timeNScales[timeIndex] / gridWidth;

	//triggerIndex = triggerSearch(triggerUp, adcZeroValue);

	IntMasterDisable();							// IRQs Disabled so PixelBuffer is filled accurately.

	// This giant if is out Trigger Implementation:
	if(_InputBuffer[triggerIndex] < adcZeroValue + 25 &&
			_InputBuffer[triggerIndex] > adcZeroValue - 25 &&
			((triggerUp && (_InputBuffer[triggerIndex - 5] < _InputBuffer[triggerIndex + 5])) ||
					(!triggerUp && _InputBuffer[triggerIndex - 5] > _InputBuffer[triggerIndex + 5])))
	{
		i = pixelRange / 2;
		j = 0;
		while(i < pixelRange)					// This while fills the right side of the buffer.
		{
			if(triggerIndex + (int)((j * pixelWidth) / 2) < 2048)
				pixelBuffer[i] = _InputBuffer[triggerIndex + (int)((j * pixelWidth) / 2)];
			else
				pixelBuffer[i] = 0;				// Fill empty if trigger is too close to the end of sample buffer.
			i++;
			j++;
		}


		i = pixelRange / 2;
		j = 0;
		while(i > 0)							// This while fills the left side of the buffer.
		{
			if(triggerIndex - (int)((j * pixelWidth) / 2) > 0)
				pixelBuffer[i] = _InputBuffer[triggerIndex - (int)((j * pixelWidth) / 2)];
			else
				pixelBuffer[i] = 0;				// Fill empty if trigger is too close to the end of sample buffer.
			i--;
			j++;
		}
	}

	IntMasterEnable();							// Values are correctly set. Interrupts can be enabled when drawing (next while)..

	i = 0;
	while(i < pixelRange)						// This while draws the pixelBuffer (Signal) on Display (Buffer).
	{
		int offsetY = gridYMin + (gridYMin + gridYMax) / 2;
		DrawPoint(gridXMin + i, offsetY - (pixelBuffer[i] - adcZeroValue) / (voltageNScales[voltageIndex] / 100), COLOR_SIGNAL);
		i++;
	}
}

void initializeClock()
{
	if (REVISION_IS_A2)
		SysCtlLDOSet(SYSCTL_LDO_2_75V);
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_8MHZ);
	systemClock = SysCtlClockGet();				// Somehow a 50 MHz System Clock is generated.
}

void initializeDisplay()
{
	RIT128x96x4Init(3500000);					// Initialize SPI Interface for Display (3.5 MHz)
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
	IntPrioritySet(INT_TIMER0A, 64);			// Heartbeat (Low Priority)
	IntPrioritySet(INT_TIMER1A, 32);			// Signal Sample (Mid Priority)
	IntPrioritySet(INT_TIMER2A, 0);				// Button Read (High Priority)

	IntEnable(INT_TIMER0A);
	IntEnable(INT_TIMER1A);
	IntEnable(INT_TIMER2A);

	count_unloaded = cpu_load_count();			// Sample CPU Load when not loaded (IRQs still disabled!)
	IntMasterEnable();
}

void screenClean()
{
	FillFrame(0);
}

void screenDraw()
{
	RIT128x96x4ImageDraw(g_pucFrame, 0, 0, FRAME_SIZE_X, FRAME_SIZE_Y);
}

// Initialize ADC Code, Followed the given example:
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

	if(blinkyOn)								// Blinky (Hearbeat) is not used for now...
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
	//IntMasterDisable();
	//count_unloaded = cpu_load_count();
	//IntMasterEnable();
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

// CPU Load Measure, Comes from Class Code:
unsigned long cpu_load_count(void)
{
	unsigned long i = 0;
	TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
	TimerEnable(TIMER3_BASE, TIMER_A); // start one-shot timer
	while (!(TimerIntStatus(TIMER3_BASE, 0) & TIMER_TIMA_TIMEOUT))
		i++;
	return i;
}
