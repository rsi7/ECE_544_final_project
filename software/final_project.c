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

#include "xmk.h"
#include "os_config.h"
#include "config/config_param.h"
#include "sys/ksched.h"
#include "sys/init.h"
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <sys/intr.h>
#include <sys/timer.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include "PMod544IOR2.h"
#include "mb_interface.h"
#include "xparameters.h"
#include "platform_config.h"
#include "platform.h"
#include "stdbool.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xstatus.h"
#include "xintc.h"

/****************************************************************************/
/************************** Constant Definitions ****************************/
/****************************************************************************/

// Device ID

#define BTN_GPIO_DEVICEID       XPAR_BTN_5BIT_DEVICE_ID
#define SW_GPIO_DEVICEID        XPAR_SW_16BIT_DEVICE_ID
#define LED_GPIO_DEVICEID       XPAR_LED_16BIT_DEVICE_ID
#define INTC_DEVICEID           XPAR_INTC_0_DEVICE_ID
#define TMRCTR0_DEVICEID        XPAR_TMRCTR_0_DEVICE_ID

// Interrupt numbers

#define TMRCTR0_INTR_NUM        XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR
#define BTN_GPIO_INTR_NUM       XPAR_MICROBLAZE_0_AXI_INTC_BTN_5BIT_IP2INTC_IRPT_INTR
#define SW_GPIO_INTR_NUM        XPAR_MICROBLAZE_0_AXI_INTC_SW_16BIT_IP2INTC_IRPT_INTR

// Pmod544 addresses

#define PMD544IO_DEVICE_ID      XPAR_PMOD544IOR2_0_DEVICE_ID
#define PMD544IO_BASEADDR       XPAR_PMOD544IOR2_0_S00_AXI_BASEADDR
#define PMD544IO_HIGHADDR       XPAR_PMOD544IOR2_0_S00_AXI_HIGHADDR

// Bit masks

#define MSK_CLEAR_INTR_CH1      0x00000001
#define MSK_ENABLE_INTR_CH1     0x00000001
#define MSK_LED_16BIT_OUTPUT    0x0000
#define MSK_SW_16BIT_INPUT      0xFFFF
#define MSK_PBTNS_5BIT_INPUT    0x1F
#define MSK_SW_FORCE_CRASH      0x00008000
#define MSK_SW_LOWER_HALF       0x000080FF
#define MSK_SW_REMOVE           0x00003E00
#define MSK_PBTNS_REMOVE        0x0000C1FF

 // Miscellaneous

#define GPIO_CHANNEL_1          1
#define DUTY_CYCLE_CHANGE       2
#define SRC_SWITCHES            0x00000001
#define SRC_BUTTONS             0x00000002

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
XTmrCtr     TMRCTR0Inst;

/****************************************************************************/
/*************************** Typdefs & Structures ***************************/
/****************************************************************************/

// LED message structure

typedef struct {
    unsigned int source;
    unsigned int value;
} _msg;

const   unsigned int    led_msg_key = 1;        // message key for LED message queue
struct  msqid_ds        led_msgstats;           // statistics from message queue

// Synchronization variables

sem_t   btn_press_sema;                         // semaphore between clock tick ISR and the clock main thread

/****************************************************************************/
/************************** Function Prototypes *****************************/
/****************************************************************************/

void*   master_thread(void *arg);
void*   button_thread(void *arg);
void*   switches_thread(void *arg);
void*   leds_thread(void *arg);

void    button_handler(void);

XStatus init_peripherals(void);

/****************************************************************************/
/***************************** Global Variables *****************************/
/****************************************************************************/

volatile    unsigned int    button_state;           // global variable to hold button values

/****************************************************************************/
/************************** MAIN PROGRAM ************************************/
/****************************************************************************/

int main() {

    XStatus sts;

    // initialize the platform and the peripherals

    init_platform();
    sts = init_peripherals();

    if (sts != XST_SUCCESS) {

        xil_printf("FATAL ERROR: Could not initialize the peripherals\n\r");
        xil_printf("Please power cycle or reset the system\n\r");
        return -1;
    }

    else {
        xil_printf("\nInitialization of the peripherals was a success!\n\n");
    }

    // Initialize xilkernel
    xilkernel_init();

    // Create the master thread
    xmk_add_static_thread(master_thread, 0);
    
    // Start the kernel
    xilkernel_start();
    
    // Should never be reached
    cleanup_platform();
    
    return 0;
}

/****************************************************************************/
/************************** MASTER THREAD ***********************************/
/****************************************************************************/

void* master_thread(void *arg) {

    pthread_t button;
    pthread_t switches;
    pthread_t leds;

    pthread_attr_t attr;
    struct sched_param spar;

    int ret;
    unsigned int ticks;
    int msg_id;

    xil_printf("----------------------------------------------------------------------------\r\n");
    xil_printf("ECE 544 Project 3 Starter Application \r\n");
    xil_printf("----------------------------------------------------------------------------\r\n");
    xil_printf("This Xilkernel based application reads the buttons and switches on the FPGA \r\n"
               "development board and displays them on the LEDs.  Even though the application is\r\n"
               "simple it uses several of the synchronization and interprocess communication\r\n"
               "capabilities offered in the Xilkernel\r\n\r\n"
               "To demonstrate, press any of the buttons and/or flip switches on the board.\r\n"
               "The current state of the buttons and switches should be displayed on the LEDs\r\n");
    xil_printf("----------------------------------------------------------------------------\r\n\r\n\r\n");

    xil_printf("MASTER: Master Thread Starting\r\n");

    // set the priority of all but the master thread to 1
    // master thread runs at priority 0

    pthread_attr_init(&attr);
    spar.sched_priority = 1;
    pthread_attr_setschedparam(&attr, &spar);

    // create the button thread

    ret = pthread_create(&button, &attr, (void*) button_thread, NULL);

    if (ret != 0) {

        xil_printf("ERROR (%d) IN MASTER THREAD: could not launch %s\r\n", ret, "button thread");
        xil_printf("FATAL ERROR: Master Thread Terminating\r\n");
        return (void*) -3;
    }

    else {
        xil_printf("MASTER: Button thread created\r\n");
    }

    // create the switches thread

    ret = pthread_create (&switches, &attr, (void*) switches_thread, NULL);

    if (ret != 0) {

        xil_printf("ERROR (%d) IN MASTER THREAD: could not launch %s\r\n", ret, "switches thread");
        xil_printf("FATAL ERROR: Master Thread Terminating\r\n");
        return (void*) -3;
    }

    else {
        xil_printf("MASTER: Switches thread created\r\n");
    }

    // create the LEDs thread

    ret = pthread_create (&leds, &attr, (void*) leds_thread, NULL);

    if (ret != 0) {

        xil_printf("ERROR (%d) IN MASTER THREAD: could not launch %s\r\n", ret, "switches thread");
        xil_printf("FATAL ERROR: Master Thread Terminating\r\n");
        return (void*) -3;
    }

    else {
        xil_printf("MASTER: LEDs thread created\r\n");
    }

    // initialize the button press semaphore

    ret = sem_init(&btn_press_sema, 0, 0);

    if (ret != 0) {

        xil_printf("ERROR (%d) IN MASTER THREAD: could not initialize %s\r\n", errno, "button press semaphore");
        xil_printf("FATAL ERROR: Master Thread Terminating\r\n");
        return (void*) -3;
    }

    else {
        xil_printf("MASTER: Button press semaphore has been initialized\n\r");
    }

    // initialize the message queue

    ret = msgget(led_msg_key, IPC_CREAT);

    if (ret == -1) {

        xil_printf("ERROR (%d): Could not create message queue.\r\n", errno);
        return (void*) -3;
    }

    else {
        xil_printf("MASTER: Successfully created LED message queue\r\n");
    }

    // register the button interrupt handler

    ret = register_int_handler(BTN_GPIO_INTR_NUM, (void*) button_handler, NULL);

    if (ret != XST_SUCCESS) {
        return (void*) -4;
    }

    else {
        xil_printf("MASTER: Button interrupt handler created successfully\r\n");
    }

    // enable interrupts...we're off to the races

    enable_interrupt(BTN_GPIO_INTR_NUM);

    xil_printf("MASTER: Interrupts have been enabled\r\n");

    // master thread main loop

    while(1) {

        // get & print the kernel time

        ticks = xget_clock_ticks();
        xil_printf("MASTER: %d ticks have elapsed\r\n", ticks);
        
        // check & print the message queue length

        msg_id = msgget(led_msg_key, IPC_CREAT);
        msgctl(msg_id, IPC_STAT, &led_msgstats);
        xil_printf("MASTER: %d messages in the queue\r\n", led_msgstats.msg_qnum);
        
        // repeat every second

        sleep(1000);
    }

    return NULL;
}

/****************************************************************************/
/************************** BUTTON THREAD ***********************************/
/****************************************************************************/

void* button_thread(void *arg) {

    unsigned int    btn     = 0x00;
    int             ret     = 0x00;
    int             msg_id  = 0x00;

    _msg            btn_msg;

    btn_msg.source = SRC_BUTTONS;

    while (1) {

        // wait for sempaphore to get unlocked by button handler
        // then immediately lock it

        sem_wait(&btn_press_sema);

        // read the buttons

        btn = XGpio_DiscreteRead(&BTNInst, GPIO_CHANNEL_1);

        // move the buttons over to led[13:9]

        btn_msg.value = btn << 9;

        // send the button values to the message queue

        msg_id = msgget(led_msg_key, IPC_CREAT);
        ret = msgsnd(msg_id, &btn_msg, sizeof(_msg), 0);

        // error handling if sending the message fails

        if (ret == -1) {

            switch (errno) {

                case (EINVAL) : xil_printf("BUTTON THREAD: Couldn't find message queue.\r\n"); break;
                case (ENOSPC) : xil_printf("BUTTON THREAD: Couldn't allocate space on queue.\r\n"); break;
                default : xil_printf("BUTTON THREAD: Error (%d) occured while sending message\r\n", errno); break;
            }
        }

        // yield remaining time to next thread

        yield();
    }

    return NULL;
}

/****************************************************************************/
/************************* SWITCHES THREAD **********************************/
/****************************************************************************/

void* switches_thread(void *arg) {

    unsigned int    sw      = 0x00;
    int             ret     = 0x00;
    int             msg_id  = 0x00;

    _msg            sw_msg;

    sw_msg.source = SRC_SWITCHES;

    while (1) {

        sw = XGpio_DiscreteRead(&SWInst, GPIO_CHANNEL_1);
        
        // send the switch values to the message queue

        sw_msg.value = (sw & MSK_SW_LOWER_HALF);
        msg_id = msgget(led_msg_key, IPC_CREAT);
        ret = msgsnd(msg_id, &sw_msg, sizeof(_msg), 0);

        // error handling if sending the message fails

        if (ret == -1) {

            switch (errno) {

                case (EINVAL) : xil_printf("SWITCHES THREAD: Couldn't find message queue.\r\n"); break;
                case (ENOSPC) : xil_printf("SWITCHES THREAD: Couldn't allocate space on queue.\r\n"); break;
                default : xil_printf("SWITCHES THREAD: Error (%d) occured while sending message\r\n", errno); break;
            }
        }

        // yield remaining time to the next thread

        yield();
    }

    return NULL;
}

/****************************************************************************/
/*************************** LEDS THREAD ************************************/
/****************************************************************************/

void* leds_thread(void *arg) {

    unsigned int    leds    = 0x00;
    int             ret     = 0x00;
    int             msg_id  = 0x00;

    _msg            localbuff;

    while (1) {

        // check the message queue for new messages

        msg_id = msgget(led_msg_key, IPC_CREAT);
        ret = msgrcv(msg_id, &localbuff, sizeof(_msg), 0, 0);

        // error handling in case reading the message fails

        if (ret == -1) {

            switch (errno) {

                case (EINVAL) : xil_printf("LEDS THREAD: Couldn't find message queue.\r\n"); break;
                case (ENOMSG) : xil_printf("LEDS THREAD: Error with the message size.\r\n"); break;
                default : xil_printf("LEDS THREAD: Error (%d) occured while sending message\r\n", errno); break;
            }
        }

        // depending on message source, need to toggle certain LEDs

        if (localbuff.source == SRC_SWITCHES) {
            leds &= MSK_SW_REMOVE;
            leds |= localbuff.value;
        }

        else if (localbuff.source == SRC_BUTTONS) {
            leds &= MSK_PBTNS_REMOVE;
            leds |= localbuff.value;
        }

        else {
            xil_printf("LEDS THREAD: Couldn't determine message source!\r\n");
        }

        // update the LEDs on the board

        XGpio_DiscreteWrite(&LEDInst, GPIO_CHANNEL_1, leds); 

        // yield remaining time to next thread

        yield();
    }

    return NULL;

}

/****************************************************************************/
/************************* INIT PERIPHERALS *********************************/
/****************************************************************************/

XStatus init_peripherals(void) {

    int status;             // status from Xilinx Lib calls

    // initialize the button GPIO instance

    status = XGpio_Initialize(&BTNInst, BTN_GPIO_DEVICEID);

    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // make sure button interrupts are enabled
    
    XGpio_InterruptGlobalEnable(&BTNInst);
    XGpio_InterruptEnable(&BTNInst, MSK_ENABLE_INTR_CH1);

    XGpio_SetDataDirection(&BTNInst, GPIO_CHANNEL_1, MSK_PBTNS_5BIT_INPUT);

    // initialize the switches GPIO instance

    status = XGpio_Initialize(&SWInst, SW_GPIO_DEVICEID);

    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    XGpio_SetDataDirection(&SWInst, GPIO_CHANNEL_1, MSK_SW_16BIT_INPUT);

    // initialize the LEDs GPIO instance

    status = XGpio_Initialize(&LEDInst, LED_GPIO_DEVICEID);

    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    XGpio_SetDataDirection(&LEDInst, GPIO_CHANNEL_1, MSK_LED_16BIT_OUTPUT);

    // initialize the PMod544IO
    // rotary encoder is set to increment from 0 by DUTY_CYCLE_CHANGE 

    status = PMDIO_initialize(PMD544IO_BASEADDR);

    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    PMDIO_ROT_init(DUTY_CYCLE_CHANGE, true);
    PMDIO_ROT_clear();

    // successfully initialized... time to return

    return XST_SUCCESS;
}

/****************************************************************************/
/************************** BUTTON HANDLER **********************************/
/****************************************************************************/

void button_handler(void) {

    // unlock the semaphore for button thread

    sem_post(&btn_press_sema);

    // update the global variable

    button_state = XGpio_DiscreteRead(&BTNInst, GPIO_CHANNEL_1);

    // acknowledge & clear interrupt flag

    XGpio_InterruptClear(&BTNInst, MSK_CLEAR_INTR_CH1);
    acknowledge_interrupt(BTN_GPIO_INTR_NUM);
}