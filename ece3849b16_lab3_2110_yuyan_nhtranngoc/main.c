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

#include <ti/sysbios/BIOS.h>

#include <ti/sysbios/knl/Task.h>


/*
 *  ======== main ========
 */
Void main() {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_COMP0);
	ComparatorConfigure(COMP_BASE, 0, COMP_TRIG_NONE | COMP_INT_HIGH | COMP_ASRCP_REF | COMP_OUTPUT_NORMAL);
	ComparatorRefSet(COMP_BASE, COMP_REF_1_5125V);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	GPIODirModeSet(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_DIR_MODE_HW); // C0- input
	GPIOPadConfigSet(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA,
	 GPIO_PIN_TYPE_ANALOG);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	GPIODirModeSet(GPIO_PORTD_BASE, GPIO_PIN_7, GPIO_DIR_MODE_HW); // C0o output
	GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_7, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
	
    BIOS_start();        /* enable interrupts and start SYS/BIOS */
}
