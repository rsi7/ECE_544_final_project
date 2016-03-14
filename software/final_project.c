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
#include "xspi.h"
#include "xintc.h"

#include "PMod544IOR2.h" 
#include "ChorusBuffer.h"
#include "DelayBuffer.h"

/****************************************************************************/
/************************** Constant Definitions ****************************/
/****************************************************************************/

// Device ID

#define TIMER_DEVICE_ID             XPAR_TMRCTR_0_DEVICE_ID
#define BTN_GPIO_DEVICE_ID          XPAR_BTN_5BIT_DEVICE_ID
#define SW_GPIO_DEVICE_ID           XPAR_SW_16BIT_DEVICE_ID
#define SPI_DEVICE_ID               XPAR_AXI_QUAD_SPI_0_DEVICE_ID
#define LED_GPIO_DEVICE_ID          XPAR_LED_16BIT_DEVICE_ID
#define INTC_DEVICE_ID              XPAR_INTC_0_DEVICE_ID

// Interrupt numbers

#define TIMER_INTERRUPT_ID          XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR
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

XSpi        SpiInst;
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

static u8 SPI_RcvBuf[2];               	// holds SPI receive data (4 zeros + 12-bits)

/****************************************************************************/
/************************** MAIN PROGRAM ************************************/
/****************************************************************************/

void main(void) {

    XStatus status;

    unsigned int    i           = 0x00;
    unsigned int    micData     = 0x00;

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

        status = XSpi_SetSlaveSelect(&SpiInst, MSK_PMOD_MIC_SS);

        if (status != XST_SUCCESS) {
            print("MAIN LOOP: Failed to select SPI slave!\r\n");
        }

        for (i = 0; i < 65536; i++) {

            // Transfer the command and receive the mic dataADC count

            status = XSpi_Transfer(&SpiInst, SPI_RcvBuf, SPI_RcvBuf, 2);

            if (status != XST_SUCCESS) {
                print("MAIN LOOP: Failed to transfer SPI!\r\n");
            }

            // SPI transfer was successful
            // process it for cleaned up microphone signal

            micData = ((SPI_RcvBuf[0] << 8) | (SPI_RcvBuf[1] << 4)) & 0xFFF;
            DelayBuffer_WriteLine(i, micData);

        }

        // repeat every 4 seconds

        PMDIO_ROT_readRotcnt(&rotcnt);
        xil_printf("MAIN LOOP: rotary count is %d\r\n", rotcnt);

        xil_printf("MAIN LOOP: button state is %d\r\n", button_state);
        xil_printf("MAIN LOOP: switch state is %d\r\n", switch_state);

    }

    return;
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
/************************* INIT PERIPHERALS *********************************/
/****************************************************************************/

XStatus init_peripherals(void) {

    int status;                 // status from Xilinx Lib calls
    XSpi_Config *ConfigPtr;     // Pointer to SPI configuration data

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

    // lookup SPI configuration

    ConfigPtr = XSpi_LookupConfig(SPI_DEVICE_ID);

    if (ConfigPtr == NULL) {
        print("INIT_PERIPH: Couldn't find SPI device!\r\n");
        return XST_DEVICE_NOT_FOUND;
    }

    // initialize the SPI device

    status = XSpi_CfgInitialize(&SpiInst, ConfigPtr, ConfigPtr->BaseAddress);

    if (status != XST_SUCCESS) {
        print("INIT_PERIPH: Failed to initialize SPI device!\r\n");
        return XST_FAILURE;
    }

    // Perform a self-test to ensure that it was built correctly

    status = XSpi_SelfTest(&SpiInst);

    if (status != XST_SUCCESS) {
        print("INIT_PERIPH: SPI device failed self-test!\r\n");
        return XST_FAILURE;
    }

    // Set the SPI device as a master,
    // SS goes low for entire transaction (does not toggle every 8 bits)
    // All other bits are OK as defaults

    status = XSpi_SetOptions(&SpiInst, XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION | XSP_CLK_ACTIVE_LOW_OPTION);

    if (status != XST_SUCCESS) {
        print("INIT_PERIPH: Failed to set SPI options!\r\n");
        return XST_FAILURE;
    }

    // Start the SPI driver so that the device is enabled,
    // and then disable the Global interrupt

    XSpi_Start(&SpiInst);
    XSpi_IntrGlobalDisable(&SpiInst);

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

    // successfully initialized... time to return

    return XST_SUCCESS;
}
