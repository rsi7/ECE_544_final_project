/* final_project - ECE 544 Final Project application
 *
 * Copyright: 2016 Portland State University
 *
 * Author: Rehan Iqbal
 * Date: March 4, 2016
 *
 *  Description:
 *  ------------
 */

/****************************************************************************/
/***************************** Include Files ********************************/
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stdbool.h"

#include "mb_interface.h"
#include "xparameters.h"
#include "xstatus.h"
#include "xil_types.h"
#include "xil_assert.h"
#include "platform_config.h"
#include "platform.h"

#include "pwm_tmrctr.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xintc.h"

#include "PMod544IOR2.h" 
#include "ChorusBuffer.h"
#include "DelayBuffer.h"
#include "InputBuffer.h"

/****************************************************************************/
/************************** Constant Definitions ****************************/
/****************************************************************************/

// Device ID

#define TIMER_DEVICE_ID             XPAR_TMRCTR_0_DEVICE_ID
#define BTN_GPIO_DEVICE_ID          XPAR_BTN_5BIT_DEVICE_ID
#define SW_GPIO_DEVICE_ID           XPAR_SW_16BIT_DEVICE_ID
#define LED_GPIO_DEVICE_ID          XPAR_LED_16BIT_DEVICE_ID
#define INTC_DEVICE_ID              XPAR_INTC_0_DEVICE_ID


// Interrupt numbers

#define FIT_INTERRUPT_ID            XPAR_MICROBLAZE_0_AXI_INTC_FIT_TIMER_0_INTERRUPT_INTR
#define BTN_INTERRUPT_ID            XPAR_MICROBLAZE_0_AXI_INTC_BTN_5BIT_IP2INTC_IRPT_INTR
#define SW_INTERRUPT_ID             XPAR_MICROBLAZE_0_AXI_INTC_SW_16BIT_IP2INTC_IRPT_INTR

// Pmod544 addresses

#define PMD544IO_DEVICE_ID          XPAR_PMOD544IOR2_0_DEVICE_ID
#define PMD544IO_BASEADDR           XPAR_PMOD544IOR2_0_S00_AXI_BASEADDR
#define PMD544IO_HIGHADDR           XPAR_PMOD544IOR2_0_S00_AXI_HIGHADDR

// ChorusBuffer addresses

#define CHORUSBUFFER_DEVICE_ID 	    XPAR_CHORUSBUFFER_0_DEVICE_ID
#define CHORUSBUFFER_BASEADDR       XPAR_CHORUSBUFFER_0_S00_AXI_BASEADDR
#define CHORUSBUFFER_HIGHADDR       XPAR_CHORUSBUFFER_0_S00_AXI_HIGHADDR

// InputBuffer addresses

#define INPUTBUFFER_DEVICE_ID       XPAR_INPUTBUFFER_0_DEVICE_ID
#define INPUTBUFFER_BASEADDR        XPAR_INPUTBUFFER_0_S00_AXI_BASEADDR
#define INPUTBUFFER_HIGHADDR        XPAR_INPUTBUFFER_0_S00_AXI_HIGHADDR

// DelayBuffer addresses

#define DELAYBUFFER_DEVICE_ID       XPAR_DELAYBUFFER_0_DEVICE_ID
#define DELAYBUFFER_BASEADDR        XPAR_DELAYBUFFER_0_S00_AXI_BASEADDR
#define DELAYBUFFER_HIGHADDR        XPAR_DELAYBUFFER_0_S00_AXI_HIGHADDR

// Bit masks

#define MSK_CLEAR_INTR_CH1          0x00000001
#define MSK_ENABLE_INTR_CH1         0x00000001
#define MSK_LED_16BIT_OUTPUT        0x0000
#define MSK_SW_16BIT_INPUT          0xFFFF
#define MSK_PBTNS_5BIT_INPUT        0x1F
#define MSK_SW_FORCE_CRASH          0x00008000
#define MSK_SW_LOWER_HALF           0x000080FF
#define MSK_SW_REMOVE               0x00003E00
#define MSK_PBTNS_REMOVE            0x0000C1FF
#define MSK_PMOD_MIC_SS             0x00000001

// Miscellaneous

#define GPIO_CHANNEL_1              1
#define CPU_CLOCK_FREQ_HZ           XPAR_CPU_CORE_CLOCK_FREQ_HZ
#define PI                          3.14159265

/****************************************************************************/
/***************** Macros (Inline Functions) Definitions ********************/
/****************************************************************************/

#define MIN(a, b)               ( ((a) <= (b)) ? (a) : (b) )
#define MAX(a, b)               ( ((a) >= (b)) ? (a) : (b) )

/****************************************************************************/
/************************** Variable Definitions ****************************/
/****************************************************************************/

XGpio       BTNInst;
XGpio       SWInst;
XGpio       LEDInst;

XIntc       IntrptCtlrInst;

/****************************************************************************/
/*************************** Typdefs & Structures ***************************/
/****************************************************************************/

/****************************************************************************/
/************************** Function Prototypes *****************************/
/****************************************************************************/

void    button_handler(void);
void    switch_handler(void);

XStatus init_peripherals(void);

/****************************************************************************/
/***************************** Global Variables *****************************/
/****************************************************************************/

volatile unsigned int button_state;     // holds button values
volatile unsigned int switch_state;     // holds button values
                  int rotcnt;           // holds rotary count

/****************************************************************************/
/************************** MAIN PROGRAM ************************************/
/****************************************************************************/

int main (void) {

    XStatus status;
    unsigned int leds;

    unsigned int InputBuffValue = 0x00;
    unsigned int ChorusBuffValue = 0x00;
    unsigned int DelayBuffValue = 0x00;

    unsigned int index = 0x00;

    // initialize the platform and the peripherals

    init_platform();
    status = init_peripherals();

    if (status != XST_SUCCESS) {

        print("MAIN LOOP: Failed to initialize the peripherals!\r\n");
        mb_sleep();
    }

    else {
        print("MAIN LOOP: Initialization of the peripherals was a success!\n\n");
    }

    // now that we're initialized, enable the interrupts

    microblaze_enable_interrupts();

    // main processing loop

    while(1) {

        // endlessly update the LEDs

        leds = (button_state << 8) | (switch_state);
        XGpio_DiscreteWrite(&LEDInst, GPIO_CHANNEL_1, leds); 


        // read the InputBuffer & write to delay buffer

        for (index = 0; index < 65535; index ++) {

            InputBuffValue = InputBuffer_ReadLine(index);
            DelayBuffer_WriteLine(index, InputBuffValue);
        }

    }

    return 0;
}

/****************************************************************************/
/************************** BUTTON HANDLER **********************************/
/****************************************************************************/

void button_handler(void) {

    // update the global variable
    button_state = XGpio_DiscreteRead(&BTNInst, GPIO_CHANNEL_1);
    button_state &= MSK_PBTNS_5BIT_INPUT;

    // acknowledge & clear interrupt flag
    XGpio_InterruptClear(&BTNInst, MSK_CLEAR_INTR_CH1);

    return;
}

/****************************************************************************/
/************************** SWITCH HANDLER **********************************/
/****************************************************************************/

void switch_handler(void) {

    // update the global variable
    switch_state = XGpio_DiscreteRead(&SWInst, GPIO_CHANNEL_1);
    switch_state &= MSK_SW_16BIT_INPUT;

    // acknowledge & clear interrupt flag
    XGpio_InterruptClear(&SWInst, MSK_CLEAR_INTR_CH1);

    return;
}

/****************************************************************************/
/***************************** FIT HANDLER **********************************/
/****************************************************************************/

/*void fit_handler(void) {

    return;
}*/

/****************************************************************************/
/************************* INIT PERIPHERALS *********************************/
/****************************************************************************/

XStatus init_peripherals(void) {

    int status;                 // status from Xilinx Lib calls

    // initialize the button GPIO instance

    status = XGpio_Initialize(&BTNInst, BTN_GPIO_DEVICE_ID);

    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // make sure button interrupts are enabled
    
    XGpio_InterruptGlobalEnable(&BTNInst);
    XGpio_InterruptEnable(&BTNInst, MSK_ENABLE_INTR_CH1);

    XGpio_SetDataDirection(&BTNInst, GPIO_CHANNEL_1, MSK_PBTNS_5BIT_INPUT);

    // initialize the switches GPIO instance

    status = XGpio_Initialize(&SWInst, SW_GPIO_DEVICE_ID);

    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // make sure switch interrupts are enabled

    XGpio_InterruptGlobalEnable(&SWInst);
    XGpio_InterruptEnable(&SWInst, MSK_ENABLE_INTR_CH1);

    XGpio_SetDataDirection(&SWInst, GPIO_CHANNEL_1, MSK_SW_16BIT_INPUT);

    // initialize the LEDs GPIO instance

    status = XGpio_Initialize(&LEDInst, LED_GPIO_DEVICE_ID);

    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    XGpio_SetDataDirection(&LEDInst, GPIO_CHANNEL_1, MSK_LED_16BIT_OUTPUT);

    // initialize the PMod544IO
    // rotary encoder is set to increment from 0 by 1% 

    status = PMDIO_initialize(PMD544IO_BASEADDR);

    if (status != XST_SUCCESS) {
        print("INIT_PERIPH: Failed to initialize PMOD544!\r\n");
        return XST_FAILURE;
    }

    PMDIO_ROT_init(1, true);
    PMDIO_ROT_clear();

    // initialize the ChorusBuffer

    status = ChorusBuffer_initialize(CHORUSBUFFER_BASEADDR);

    if (status != XST_SUCCESS) {
        print("INIT_PERIPH: Failed to initialize ChorusBuffer!\r\n");
        return XST_FAILURE;
    }

    // initialize the DelayBuffer

    status = DelayBuffer_initialize(DELAYBUFFER_BASEADDR);

    if (status != XST_SUCCESS) {
        print("INIT_PERIPH: Failed to initialize DelayBuffer!\r\n");
        return XST_FAILURE;
    }
    // initialize the InputBuffer

    status = InputBuffer_initialize(INPUTBUFFER_BASEADDR);

    if (status != XST_SUCCESS) {
        print("INIT_PERIPH: Failed to initialize InputBuffer!\r\n");
        return XST_FAILURE;
    }

    // initialize the interrupt controller

    status = XIntc_Initialize(&IntrptCtlrInst,INTC_DEVICE_ID);
    
    if (status != XST_SUCCESS) {
        print("INIT_PERIPH: Failed to initialize interrupt controller!\r\n");
        return XST_FAILURE;
    }

    // connect the button handler to the interrupt
    
    status = XIntc_Connect(&IntrptCtlrInst, BTN_INTERRUPT_ID, (XInterruptHandler)button_handler, (void *)0);

    if (status != XST_SUCCESS) {
        print("INIT_PERIPH: Failed to connect button handler as interrupt!\r\n");
        return XST_FAILURE;
    }
 
    // connect the switch handler to the interrupt
    
    status = XIntc_Connect(&IntrptCtlrInst, SW_INTERRUPT_ID, (XInterruptHandler)switch_handler, (void *)0);

    if (status != XST_SUCCESS) {
        print("INIT_PERIPH: Failed to connect switch handler as interrupt!\r\n");
        return XST_FAILURE;
    }

    // connect the FIT handler to the interrupt
    
/*    status = XIntc_Connect(&IntrptCtlrInst, FIT_INTERRUPT_ID, (XInterruptHandler)fit_handler, (void *)0);

    if (status != XST_SUCCESS) {
        print("INIT_PERIPH: Failed to connect FIT handler as interrupt!\r\n");
        return XST_FAILURE;
    }*/

    // start the interrupt controller such that interrupts are enabled for
    // all devices that cause interrupts, specifically real mode so that
    // the they can cause interrupts thru the interrupt controller.

    status = XIntc_Start(&IntrptCtlrInst, XIN_REAL_MODE);

    if (status != XST_SUCCESS) {
        print("INIT_PERIPH: Failed to start interrupt controller!\r\n");
        return XST_FAILURE;
    } 
      
    // enable the button & switch innterrupts

    XIntc_Enable(&IntrptCtlrInst, BTN_INTERRUPT_ID);
    XIntc_Enable(&IntrptCtlrInst, SW_INTERRUPT_ID);
/*    XIntc_Enable(&IntrptCtlrInst, FIT_INTERRUPT_ID);
*/
    // successfully initialized... time to return

    return XST_SUCCESS;
}
