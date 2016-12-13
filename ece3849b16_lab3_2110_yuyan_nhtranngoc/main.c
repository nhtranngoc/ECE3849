/*
 *  ======== main.c ========
 */

#include <xdc/std.h>
#include "lm3s2110.h"
#include "network.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "drivers/rit128x96x4.h"
#include "utils/ustdlib.h"

#include "driverlib/comp.h"
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>
#include <ti/sysbios/BIOS.h>

#include <ti/sysbios/knl/Task.h>

unsigned long g_timerCount;
float g_period;
float g_avgPeriod;
unsigned int g_intervalCount = 0;
char g_runOnce = 1;
unsigned long g_SystemClock;
unsigned long g_frequency;

void initializeComparator();
void initializeGPIO();
void initializeTimer();

/*
 *  ======== main ========
 */
void main() {
	IntMasterDisable();
	g_SystemClock = SysCtlClockGet();
	initializeComparator();
	initializeGPIO();
	initializeTimer();
	NetworkInit();
	IntMasterEnable();
    BIOS_start();        /* enable interrupts and start SYS/BIOS */
}

void initializeComparator(){
	SysCtlPeripheralEnable(SYSCTL_PERIPH_COMP0);
	ComparatorConfigure(COMP_BASE, 0, COMP_TRIG_NONE | COMP_INT_HIGH | COMP_ASRCP_REF | COMP_OUTPUT_NORMAL);
	ComparatorRefSet(COMP_BASE, COMP_REF_1_5125V);
}

void initializeGPIO(){
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	GPIODirModeSet(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_DIR_MODE_HW); // C0- input
	GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_ANALOG);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	GPIODirModeSet(GPIO_PORTD_BASE, GPIO_PIN_7, GPIO_DIR_MODE_HW); // C0o output
	GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_7, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
}

void initializeTimer(){
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	GPIODirModeSet(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_DIR_MODE_HW); // CCP0 input
	GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_0, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
	TimerDisable(TIMER0_BASE, TIMER_BOTH);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME);
	TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);
	TimerLoadSet(TIMER0_BASE, TIMER_A, 0xffff);
	TimerIntEnable(TIMER0_BASE, TIMER_CAPA_EVENT);
	TimerEnable(TIMER0_BASE, TIMER_A);
}

// Capture Hwi
void Timer0A_Hwi() {
	unsigned long prev = 0;
	TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);

	if(g_runOnce) {
		g_runOnce = 0;
		prev = TimerValueGet(TIMER0_BASE, TIMER_A);
	} else {
		g_timerCount = TimerValueGet(TIMER0_BASE, TIMER_A);
		g_period += ((g_timerCount - prev) & 0xffff);
		prev = g_timerCount;

		g_intervalCount++;
	}
}

// Clock function
void Clock_ISR() {
	IArg key;

	key = GateHwi_enter(gateHwi0);
	unsigned long period = g_period;
	unsigned long interval = g_intervalCount;

	g_period = 0;
	g_intervalCount = 0;

	GateHwi_leave(gateHwi0, key);

	g_avgPeriod = (float)period/(float)interval;
	g_frequency = ((1/g_avgPeriod) * g_SystemClock) * 1000; // Frequency in mHz

	NetworkTx(g_frequency);

}
