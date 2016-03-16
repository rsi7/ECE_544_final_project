/* final_project - ECE 544 Final Project application
 *
 * Copyright: 2016 Portland State University
 *
 * Author: Rehan Iqbal, Chad Klingbeil, Neil Roberts
 * Date: March 4, 2016
 *
 *  Description:
 *  ------------
 *
 * This application carries out DSP for the project.
 * Depending on sw[1:0] state, one of the following effects will apply:
 *
 *      00: no effects
 *      01: delay effect
 *      10: chorus effect
 *      11: delay + chorus effects
 *
 * This calculation is handled in the main loop, with chorus processing done
 * in the ApplyChorus function. Values are read from the InputBuffer, 
 * processed with DSP, and stored in the DelayBuffer for playback by the
 * hardware module AudioOutput.
 * 
 * The FIT Handler runs at 16kHz but does not do anything except increment
 * a static variable which never gets used. It was used for debugging SPI
 * in earlier code versions.
 * 
 * The button handler and switch handler simply perform GPIO reads to update
 * their respective global variables. The main loop then writes these values
 * to the LEDs.
*/

/****************************************************************************/
/***************************** Include Files ********************************/
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stdbool.h"
#include "st_i.h"

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
#define MSK_CHORUS_FX               0x0001
#define MSK_DELAY_FX                0x0002
#define MSK_CHORUS_DELAY_FX         0x0003
#define MSK_LOWER_2_BITS            0x0003

// Miscellaneous

#define GPIO_CHANNEL_1              1
#define CPU_CLOCK_FREQ_HZ           XPAR_CPU_CORE_CLOCK_FREQ_HZ
#define PI                          3.14159265

// Chorus parameters

#define MOD_SINE            0   
#define MOD_TRIANGLE        1   
#define MAX_CHORUS          7  
#define MAX_RPT             3
#define BUFFER_DEPTH        65536
#define BUFFER_WIDTH        16

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

/* Private data for SKEL file (Chorus.c) */ 

typedef struct  chorusparams {  

    int         num_chorus;   
    int         modulation[MAX_CHORUS];   
    int         counter;               
    long        phase[MAX_CHORUS];   
    float       *chorusbuf;   
    float       in_gain, out_gain;   
    float       delay[MAX_CHORUS], decay[MAX_CHORUS];   
    float       speed[MAX_CHORUS], depth[MAX_CHORUS];   
    long        length[MAX_CHORUS];   
    int         *lookup_tab[MAX_CHORUS];   
    int         depth_samples[MAX_CHORUS], samples[MAX_CHORUS];   
    int         maxsamples, fade_out;   

} *chorus_t;

/****************************************************************************/
/************************** Function Prototypes *****************************/
/****************************************************************************/

void    button_handler(void);
void    switch_handler(void);
void    Apply_Chorus(unsigned int * value);

XStatus init_peripherals(void);

/****************************************************************************/
/***************************** Global Variables *****************************/
/****************************************************************************/

volatile unsigned int button_state;     // holds button values
volatile unsigned int switch_state;     // holds button values
                  int rotcnt;           // holds rotary count
static  unsigned  int sample = 0x00;


eff_t effp;

/****************************************************************************/
/************************** MAIN PROGRAM ************************************/
/****************************************************************************/

int main (void) {

    XStatus status;

    unsigned int leds       = 0x00;
    unsigned int switch_fx  = 0x00;

    unsigned int bufline   = 0x00;

    unsigned int bufval1   = 0x00;
    unsigned int bufval2   = 0x00;
    unsigned int bufval3   = 0x00;
    unsigned int bufval4   = 0x00;

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

    switch_fx = switch_state & MSK_LOWER_2_BITS;

        for (bufline = 0; bufline <= BUFFER_DEPTH; bufline++) {

            // continuously read from InputBuffer

            bufval1 = InputBuffer_ReadLine(bufline);

            // Apply Chorus is sw[1:0] is 2'b01

            if (switch_fx == MSK_CHORUS_FX) {
                
                Apply_Chorus(&bufval1);                                      
                ChorusBuffer_WriteLine(bufline, bufval1);
            }

            // Apply Chorus + Delay is sw[1:0] is 2'b11

            else if (switch_state == MSK_CHORUS_DELAY_FX) {

                Apply_Chorus(&bufval1);
                ChorusBuffer_WriteLine(bufline, bufval1);
                
                if (bufline >= (BUFFER_DEPTH / 8)) {

                    bufval2 = ChorusBuffer_ReadLine(bufline - (BUFFER_DEPTH / 8));
                }
                
                bufval2 = (int) (((float) bufval2) / 1.25);

                // Second instance of delay @ 1.0s
                // amplitude at 60% of original input

                if (bufline >= (BUFFER_DEPTH / 4)) {

                    bufval3 = ChorusBuffer_ReadLine(bufline - (BUFFER_DEPTH / 4));
                }
                
                bufval3 = (int) (((float) bufval3) / 1.66);

                // Third instance of delay @ 1.5s
                // amplitude at 45% of original input

                if (bufline >= (BUFFER_DEPTH / 3)) {

                    bufval4 = ChorusBuffer_ReadLine(bufline - (BUFFER_DEPTH / 3));
                }
                
                bufval4 = (int) (((float) bufval4) / 2.25);

                // Add delayed signals to current output

                bufval1 = bufval1 + bufval2 + bufval3 + bufval4;
            }

            // Apply Delay is sw[1:0] is 2'b10

            else if (switch_state == MSK_DELAY_FX) {

                // First instance of delay @ 0.5s
                // amplitude at 80% of original input

                if (bufline >= (BUFFER_DEPTH / 8)) {

                    bufval2 = ChorusBuffer_ReadLine(bufline - (BUFFER_DEPTH / 8));
                }

                bufval2 = (int) (((float) bufval2) / 1.25);

                // Second instance of delay @ 1.0s
                // amplitude at 60% of original input

                if (bufline >= (BUFFER_DEPTH / 4)) {

                    bufval3 = ChorusBuffer_ReadLine(bufline - (BUFFER_DEPTH / 4));
                }

                bufval3 = (int) (((float) bufval3) / 1.66);

                // Third instance of delay @ 1.5s
                // amplitude at 45% of original input

                if (bufline >= (BUFFER_DEPTH / 3)) {

                    bufval4 = ChorusBuffer_ReadLine(bufline - (BUFFER_DEPTH / 3));
                }

                bufval4 = (int) (((float) bufval4) / 2.25);

                // Overlay delayed signals to current output
                bufval1 = bufval1 + bufval2 + bufval3 + bufval4;
            }

            // Write to output buffer with DSP-modified value

            DelayBuffer_WriteLine(bufline, bufval1);

        } // end for loop

    } // end while loop

    return 0;

} // end main loop

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

void fit_handler(void) {

    if (sample >= BUFFER_DEPTH) {
        sample = 0;
    }

    else {
        sample++;
    }

    return;
}

/****************************************************************************/
/************************* INIT PERIPHERALS *********************************/
/****************************************************************************/

XStatus init_peripherals(void) {

    int status = 0x00;

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
    
    status = XIntc_Connect(&IntrptCtlrInst, FIT_INTERRUPT_ID, (XInterruptHandler)fit_handler, (void *)0);

    if (status != XST_SUCCESS) {
        print("INIT_PERIPH: Failed to connect FIT handler as interrupt!\r\n");
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
    XIntc_Enable(&IntrptCtlrInst, FIT_INTERRUPT_ID);

    // successfully initialized... time to return

    return XST_SUCCESS;
}

/****************************************************************************/
/******************************* CHORUS EFFECT ******************************/
/****************************************************************************/

void Apply_Chorus(unsigned int * value) {
    /*  
    * August 24, 1998
    * Copyright (C) 1998 Juergen Mueller And Sundry Contributors
    * This source code is freely redistributable and may be used for
    * any purpose.  This copyright notice must be maintained.
    * Juergen Mueller And Sundry Contributors are not responsible for
    * the consequences of using this software.
    *
    * Chorus effect
    *
    * Flow diagram scheme for n delays ( 1 <= n <= MAX_CHORUS ):
    *
    *        * gain-in                                           ___
    * ibuff -----+--------------------------------------------->|   |
    *            |      _________                               |   |
    *            |     |         |                   * decay 1  |   |
    *            +---->| delay 1 |----------------------------->|   |
    *            |     |_________|                              |   |
    *            |        /|\                                   |   |
    *            :         |                                    |   |
    *            : +-----------------+   +--------------+       | + |
    *            : | Delay control 1 |<--| mod. speed 1 |       |   |
    *            : +-----------------+   +--------------+       |   |
    *            |      _________                               |   |
    *            |     |         |                   * decay n  |   |
    *            +---->| delay n |----------------------------->|   |
    *                  |_________|                              |   |
    *                     /|\                                   |___|
    *                      |                                      |
    *              +-----------------+   +--------------+         | * gain-out
    *              | Delay control n |<--| mod. speed n |         |
    *              +-----------------+   +--------------+         +----->obuff
    *
    *
    * The delay i is controled by a sine or triangle modulation i ( 1 <= i <= n).
    *
    * Usage:
    *   chorus gain-in gain-out delay-1 decay-1 speed-1 depth-1 -s1|t1 [
    *       delay-2 decay-2 speed-2 depth-2 -s2|-t2 ... ]
    *
    * Where:
    *   gain-in, decay-1 ... decay-n :  0.0 ... 1.0      volume
    *   gain-out :  0.0 ...      volume
    *   delay-1 ... delay-n :  20.0 ... 100.0 msec
    *   speed-1 ... speed-n :  0.1 ... 5.0 Hz       modulation 1 ... n
    *   depth-1 ... depth-n :  0.0 ... 10.0 msec    modulated delay 1 ... n
    *   -s1 ... -sn : modulation by sine 1 ... n
    *   -t1 ... -tn : modulation by triangle 1 ... n
    *
    * Note:
    *   when decay is close to 1.0, the samples can begin clipping and the output
    *   can saturate!
    *
    * Hint:
    *   1 / out-gain < gain-in ( 1 + decay-1 + ... + decay-n )
    *
    */

    // Define chorus parameters

    float in_gain = 0.2;
    float out_gain = 10.0;
    float delay = 50.0;
    float decay = 0.4;
    float speed = 5.0;
    float depth = 8.0;
    float sample_rate = 16000.0;
    
    chorus_t chorus = (chorus_t) effp->priv;

    value = (int)(*value*((delay+depth)*sample_rate/1000.0));
 
    return;
}