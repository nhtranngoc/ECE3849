
/* ======================================================
 * --------------------- LIBRARIES ----------------------
 * ======================================================
 */
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
#include "kiss_fft.h"
#include "_kiss_fft_guts.h"

#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>

#include <ti/sysbios/BIOS.h>
//#include <ti/sysbios/hal/Hwi.h>
//#include <ti/sysbios/knl/Task.h>

#define PI	   				3.14159265358979		// PI Constant
#define NFFT 1024 //FFT length
#define KISS_FFT_CFG_SIZE (sizeof(struct kiss_fft_state)+sizeof(kiss_fft_cpx)*(NFFT-1))
#define DISPLAY_WIDTH		128						// Display Horizontal Res.
#define DISPLAY_HEIGHT		96						// Display Vertical Res.
#define BUTTON_BUFFER_SIZE	100						// Size of Button Buffer
#define ADC_BUFFER_SIZE 	2048 					// Size of ADC Buffer (Should be 2^n ??)
#define ADC_BUFFER_WRAP(i) ((i) & (ADC_BUFFER_SIZE - 1)) // index wrapping macro

#define COLOR_GRID			4						// Grid Color (Backlight)
#define COLOR_SIGNAL		15						// Signal Color (Backlight)
#define COLOR_TEXT			15						// Text Color (Backlight)
#define COLOR_TRIGGER		10						// Trigger Color (Backlight)

/* ======================================================
 * --------------------- VARIABLES ----------------------
 * ======================================================
 */
const char* const timeScales[] = {"10us", "20us", "30us", "40us", "50us", "60us", "70us", "80us", "90us", "100us"};
int timeNScales[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
const char* const voltageScales[] = {"100mV", "200mV", "500mV", "1V"};
int voltageNScales[] = {100, 200, 500, 1000};
volatile short* _InputBuffer;
int fftBuffer [DISPLAY_WIDTH] = {0};				// This one has to be ScreenWidth (128) !!
int pixelBuffer [DISPLAY_WIDTH - 1];				// Value Array for Signal Display
char buttonArray [BUTTON_BUFFER_SIZE];				// Array for Button Input
unsigned long systemClock;							// System Running Clock
int buttonArrayCap = 0;								// Cap Value for Button Array
int adcZeroValue = 525;								// ADC Zero Value (Measured)
int triggerIndex = 1024;							// The Starting Index of the Trigger

volatile int g_iADCBufferIndex = ADC_BUFFER_SIZE - 1;	// Index of the Last ADC Sample
volatile short g_psADCBuffer[ADC_BUFFER_SIZE]; 			// Circular Buffer for ADC Samples
volatile short g_tempBuffer[NFFT];
volatile unsigned long g_ulADCErrors = 0; 				// Missed ADC Deadline Count

int blinkyOn = 1;									// Heartbeat (Blinky) Start Value (Not Used 4 Now!)
int triggerUp = 1;									// Starting Direction of the Trigger (1 - UP, 0 - DOWN)
int selectionIndex = 0;								// Starting Position of the Scale Selection (0 - Time, 1 - Voltage)
int voltageIndex = 0;								// Starting Index of the Voltage Scale (100mV)
int timeIndex = 0;									// Starting Index of the Time Scale (10us)
int debugValue = 0;

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

/* ======================================================
 * --------------------- FUNCTIONS ----------------------
 * ======================================================
 */
void initializeClock();								// Initializes System Clock (50 MHz)
//void initializeDisplay();							// Initializes Screen SPI (3.5 MHz)
//void initializeTimers();							// Initializes Button Clock (200 Hz)
void initializeButtons();							// Sets the Registers for Buttons
void initializeADC();								// ADC Setup Code for Signal Sampling
void initializeScreen();							// Draw Function Outside of Main Loop
//void processButtons();								// Read and Process the B.Press from queue
void screenClean();									// Cleans the Screen Before next Draw
void screenDraw();									// Pushes the Screen Buffer to Display
void drawGrid();									// Draws the Grid for Oscillo-noscope
void drawOverlay();									// Draws the Overlay Elements (eg. Text)
void drawFFTOverlay();
void drawSignal(short* _InputBuffer);				// Draws the Signal to the Screen
void drawTrigger();									// Draws the Trigger Line (y = 0 for this lab)
//int triggerSearch(int direction, int triggerValue);	// Returns the next Index for Trigger
int strCenter(int _Coordinate, char* _String);		// Returns x-Value for Centered String
int strWidth(char* _String);						// Returns the Calculated Width of the String
void debugThis();
void ADC_ISR(void);

int displaySignal = 1;

/* ======================================================
 * ----------------------- MAIN ------------------------
 * ======================================================
 */

Void main()
{
	IntMasterDisable();
	initializeButtons();							// Initialize Buttons
	initializeADC();								// Initialize IRQ for ADC
	initializeScreen();
	RIT128x96x4Init(3500000);

	BIOS_start();
}

/* ======================================================
 * ------------------------ INIT ------------------------
 * ======================================================
 */

void initializeScreen()
{
	gridXSize = gridXMax - gridXMin;
	gridYSize = gridYMax - gridYMin;
	gridWidth = gridXSize / (barsHorizontal - 1);
	gridHeight = gridYSize / (barsVertical - 1);
	alignX = (gridXSize % (barsHorizontal - 1)) / 2;
	alignY = (gridYSize % (barsVertical - 1)) / 2;
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
//	IntPrioritySet(INT_ADC0, 32); // set ADC interrupt priority in the interrupt controller - lower than main ISR
//	IntEnable(INT_ADC0); // enable ADC interrupt
}

void initializeButtons()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_1);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
}

/* ======================================================
 * --------------------- INSTANCES ----------------------
 * ======================================================
 */

void Button_Task() {
	IntMasterEnable();
	while(1) {
		Semaphore_pend(sem_button, BIOS_WAIT_FOREVER);

		unsigned long presses = g_ulButtons;
		ButtonDebounce((~GPIO_PORTE_DATA_R & (GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3)) << 1);
		ButtonDebounce((~GPIO_PORTF_DATA_R & GPIO_PIN_1) >> 1);
		presses = ~presses & g_ulButtons; // A List of Button Presses

		char buttonPress;

		if (presses & 1) {
			buttonPress = 'S';
		} else if (presses & 16) {
			buttonPress = 'R';
		} else if (presses & 8) {
			buttonPress = 'L';
		} else if (presses & 4) {
			buttonPress = 'D';
		} else if (presses & 2) {
			buttonPress = 'U';
		}

		if (presses != 0) {
			Mailbox_post(Mailbox_ButtonQueue, &buttonPress, BIOS_NO_WAIT);
		}

	}
}

void UserInput_Task(UArg arg0 , UArg arg1)
{
	char currentEntry;
	while(1)
	{
		Mailbox_pend(Mailbox_ButtonQueue, &currentEntry, BIOS_WAIT_FOREVER);

		if(currentEntry == 'S' || currentEntry == 's')
		{
			if(displaySignal)
				displaySignal = 0;
			else
				displaySignal = 1;
	}
		if(currentEntry == 'R' || currentEntry == 'r')
		{
			if(selectionIndex < 2)
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
			else if(selectionIndex == 1)
			{
				if(voltageIndex > 0)
					voltageIndex--;
			}
			else
			{
				if(triggerUp)
					triggerUp = 0;
				else
					triggerUp = 1;
			}
		}
		if(currentEntry == 'U' || currentEntry == 'u')
		{
			if(selectionIndex == 0)
			{
				if(timeIndex < 9)
					timeIndex++;
			}
			else if(selectionIndex == 1)
			{
				if(voltageIndex < 3)
					voltageIndex++;
			}
			else
			{
				if(triggerUp)
					triggerUp = 0;
				else
					triggerUp = 1;
			}
		}

		Semaphore_post(sem_display);

	}
}

void Display_Task(UArg arg0, UArg arg1)
{
	while(1)
	{
		// Wait for signal
		Semaphore_pend(sem_display, BIOS_WAIT_FOREVER);

		screenClean();

		if(displaySignal)
		{
			drawGrid();
			drawOverlay();
			drawTrigger();
		}
		else
		{
			drawFFTOverlay();

			int i = 0;
			while(i < 128) {
				DrawPoint(i, -60 + fftBuffer[i], COLOR_SIGNAL);
				i++;
			}
		}

		screenDraw();

		Semaphore_post(sem_waveform);
	}
}

void Waveform_Task(UArg arg0, UArg arg1)
{
	while(1)
	{
		Semaphore_pend(sem_waveform, BIOS_WAIT_FOREVER);
//		// Search for trigger
//		// Copy waveform
//		// Request screen update
//		// block again

		int i = 0;
		int j = 0;
		int pixelRange = gridXMax - gridXMin;
		float pixelWidth = timeNScales[timeIndex] / gridWidth;

		IntMasterDisable();							// IRQs Disabled so PixelBuffer is filled accurately.
		switch(displaySignal){
		case 1:
			_InputBuffer = g_psADCBuffer;
			// Trigger search implementation
			if (_InputBuffer[triggerIndex] < adcZeroValue + 5 &&
					_InputBuffer[triggerIndex] > adcZeroValue - 5 &&
					((triggerUp && (_InputBuffer[triggerIndex - 5] < _InputBuffer[triggerIndex + 5])) ||
							(!triggerUp && (_InputBuffer[triggerIndex - 5] > _InputBuffer[triggerIndex + 5]))))
			{
				// This while loop fills the right side of the buffer.
				i = pixelRange / 2;
				j = 0;
				while(i < pixelRange)
				{
					if(triggerIndex + (int)((j * pixelWidth) / 2) < 2048)
						pixelBuffer[i] = _InputBuffer[triggerIndex + (int)((j * pixelWidth) / 2)];
					else
						pixelBuffer[i] = 0;				// Fill empty if trigger is too close to the end of sample buffer.
					i++;
					j++;
				}

				// This while loop fills the left side of the buffer.
				i = pixelRange / 2;
				j = 0;
				while(i > 0)
				{
					if(triggerIndex - (int)((j * pixelWidth) / 2) > 0)
						pixelBuffer[i] = _InputBuffer[triggerIndex - (int)((j * pixelWidth) / 2)];
					else
						pixelBuffer[i] = 0;				// Fill empty if trigger is too close to the end of sample buffer.
					i--;
					j++;
				}
			}
			break;

		case 0: //Spectrum mode
			i = 0;
			j = 0;
			for (i= g_iADCBufferIndex - 1024; i != g_iADCBufferIndex; i++){
				g_tempBuffer[j] = g_psADCBuffer[ADC_BUFFER_WRAP(i)];
				j++;
			}
			break;

		}

		IntMasterEnable();							// Values are correctly set. Interrupts can be enabled when drawing (next while)..

		if(displaySignal){
			i = 0;
			while(i < pixelRange)						// This while loop draws the pixelBuffer (Signal) on Display (Buffer).
			{
				int offsetY = gridYMin + (gridYMin + gridYMax) / 2;
				DrawPoint(gridXMin + i, offsetY - (pixelBuffer[i] - adcZeroValue) / (voltageNScales[voltageIndex] / 100), COLOR_SIGNAL);
				i++;
			}

			Semaphore_post(sem_display);
		} else {
			Semaphore_post(sem_fft);
		}

	}
}

void Button_Clock(UArg arg) {
		Semaphore_post(sem_button);

		if(displaySignal)
			Semaphore_post(sem_waveform);
		else
			Semaphore_post(sem_fft);
}

void FFT_Task(UArg arg0, UArg arg1)
{
	static char kiss_fft_cfg_buffer[KISS_FFT_CFG_SIZE];
	size_t buffer_size = KISS_FFT_CFG_SIZE;
	kiss_fft_cfg cfg;
	static kiss_fft_cpx in[NFFT], out[NFFT];
	int i;

	cfg = kiss_fft_alloc(NFFT, 0, kiss_fft_cfg_buffer, &buffer_size);

	while(1)
	{
		debugThis();
		// Wait for signal
		Semaphore_pend(sem_fft, BIOS_WAIT_FOREVER);
		// DO KISS_FFT STUFF HERE

		for (i = 0; i < NFFT; i++) {
			in[i].r = g_tempBuffer[i];
			in[i].i = 0;
		}

		kiss_fft(cfg, in, out);

		// Update the fftBuffer:
		i = 0;
		while(i < 128)
		{
			fftBuffer[i] = (log10(out[i].r*out[i].r + out[i].i*out[i].i)*(-10))+180;// VALUE FROM FFT BUCKET
			i++;
		}

		Semaphore_post(sem_display);
	}
}

void ADC_ISR(void)
{
	ADC0_ISC_R = ADC_ISC_IN0; // clear ADC sequence0 interrupt flag in the ADCISC register

	if (ADC0_OSTAT_R & ADC_OSTAT_OV0)
	{ // check for ADC FIFO overflow
		g_ulADCErrors++; // count errors - step 1 of the signoff
		ADC0_OSTAT_R = ADC_OSTAT_OV0; // clear overflow condition
	}
	int buffer_index = ADC_BUFFER_WRAP(g_iADCBufferIndex + 1);
	g_psADCBuffer[buffer_index] = ADC_SSFIFO0_R; // read sample from the ADC sequence0 FIFO
	g_iADCBufferIndex = buffer_index;
}


/* ======================================================
 * ---------------------- RUBBISH -----------------------
 * ======================================================
 */

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
	char debugString[20];							// Current Time Scale Selection (String)

	int marginVertical = 3;							// Text Alignment (Vertical Margin)
	int marginHorizontal = 5;						// Text Alignment (Horizontal Margin)

	// Draw Time Scale:
	usprintf(timeString, "%s", timeScales[timeIndex]);
	DrawString(strCenter(DISPLAY_WIDTH / 6, timeString), marginVertical, timeString, COLOR_TEXT, false);

	if(selectionIndex == 0)
		DrawLine(strCenter(DISPLAY_WIDTH / 6, timeString), marginVertical + 8, strCenter(DISPLAY_WIDTH / 6, timeString) + strWidth(timeString), marginVertical + 8, COLOR_TEXT);

	// Draw Voltage Scale:
	usprintf(voltageString, "%s", voltageScales[voltageIndex]);
	DrawString(strCenter(DISPLAY_WIDTH / 2, voltageString), marginVertical, voltageString, COLOR_TEXT, false);

	if(selectionIndex == 1)
		DrawLine(strCenter(DISPLAY_WIDTH / 2, voltageString), marginVertical + 8, strCenter(DISPLAY_WIDTH / 2, voltageString) + strWidth(voltageString), marginVertical + 8, COLOR_TEXT);

	// Values for Trigger Selection Image:
	int triggerX = ((5 * DISPLAY_WIDTH) / 6) - 8;
	int triggerY = 3;

//	usprintf(debugString, "%s", debugValue);
//	DrawString(marginHorizontal + 15, DISPLAY_HEIGHT - marginVertical - 10, debugString, COLOR_TEXT, false);

	if(selectionIndex == 2)
		DrawLine(triggerX, triggerY + 8,  triggerX + 16, triggerY + 8, COLOR_TEXT);

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

void drawFFTOverlay()
{
	char frequencyString[20];							// Current Voltage Scale Selection (String)
	char intervalString[20];							// Current Time Scale Selection (String)

	int marginVertical = 3;							// Text Alignment (Vertical Margin)
	int marginHorizontal = 5;						// Text Alignment (Horizontal Margin)

	// Draw Time Scale:
	usprintf(frequencyString, "%s", "9.8kHz");
	DrawString(strCenter(DISPLAY_WIDTH / 6, frequencyString), marginVertical, frequencyString, COLOR_TEXT, false);

	// Draw Voltage Scale:
	usprintf(intervalString, "%s", "20 dBV");
	DrawString(strCenter(DISPLAY_WIDTH / 2, intervalString), marginVertical, intervalString, COLOR_TEXT, false);



}

void drawTrigger()
{
	int y = (gridYMax - gridYMin) / 2;
	DrawLine(gridXMin, y, gridXMax, y, COLOR_TRIGGER);
}

void screenClean()
{
	FillFrame(0);
}

void screenDraw()
{
	RIT128x96x4ImageDraw(g_pucFrame, 0, 0, FRAME_SIZE_X, FRAME_SIZE_Y);
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

int strWidth(char* _String)
{
	int offset = 0;
	int count = 0;

	while (*(_String + offset) != '\0')
	{
		++count;
		++offset;
	}
	return count * 6; // Each character a width of 6 pixels in ASCII
}

void debugThis()
{
	if(debugValue < 100)
		debugValue++;
	else
		debugValue = 0;
}
