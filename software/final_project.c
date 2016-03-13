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
#include "pwm_tmrctr.h"
#include "mb_interface.h"
#include "xparameters.h"
#include "platform_config.h"
#include "platform.h"
#include "stdbool.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xstatus.h"
#include "xspi.h"

/****************************************************************************/
/************************** Constant Definitions ****************************/
/****************************************************************************/

// Device ID

#define TMRCTR0_DEVICEID        XPAR_TMRCTR_0_DEVICE_ID
#define BTN_GPIO_DEVICEID       XPAR_BTN_5BIT_DEVICE_ID
#define SW_GPIO_DEVICEID        XPAR_SW_16BIT_DEVICE_ID
#define SPI_DEVICEID            XPAR_AXI_QUAD_SPI_0_DEVICE_ID
#define LED_GPIO_DEVICEID       XPAR_LED_16BIT_DEVICE_ID
#define INTC_DEVICEID           XPAR_INTC_0_DEVICE_ID

// Interrupt numbers

#define TMRCTR0_INTR_NUM        XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR
#define BTN_GPIO_INTR_NUM       XPAR_MICROBLAZE_0_AXI_INTC_BTN_5BIT_IP2INTC_IRPT_INTR
#define SW_GPIO_INTR_NUM        XPAR_MICROBLAZE_0_AXI_INTC_SW_16BIT_IP2INTC_IRPT_INTR
#define FIT_INTR_NUM            XPAR_MICROBLAZE_0_AXI_INTC_FIT_TIMER_0_INTERRUPT_INTR
#define AXI_INTR_NUM            XPAR_MICROBLAZE_0_AXI_INTC_AXI_QUAD_SPI_0_IP2INTC_IRPT_INTR

// Pmod544 addresses

#define PMD544IO_DEVICE_ID      XPAR_PMOD544IOR2_0_DEVICE_ID
#define PMD544IO_BASEADDR       XPAR_PMOD544IOR2_0_S00_AXI_BASEADDR
#define PMD544IO_HIGHADDR       XPAR_PMOD544IOR2_0_S00_AXI_HIGHADDR

// PWM timer parameters

#define PWM_TIMER_DEVICE_ID     XPAR_PWM_TIMER_DEVICE_ID
#define PWM_TIMER_BASEADDR      XPAR_PWM_TIMER_BASEADDR
#define PWM_TIMER_HIGHADDR      XPAR_PWM_TIMER_HIGHADDR
#define DUTY_CYCLE_CHANGE       1
#define PWM_FREQUENCY           500000

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
#define MSK_PMOD_MIC_SS         0x00000001

// Miscellaneous

#define GPIO_CHANNEL_1          1
#define CPU_CLOCK_FREQ_HZ       XPAR_CPU_CORE_CLOCK_FREQ_HZ
#define FIT_MAX_COUNT           10000
#define SRC_SWITCHES            0x00000001
#define SRC_BUTTONS             0x00000002
#define PI                      3.14159265

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
XTmrCtr     PWMTimerInst;
XSpi        SpiInst;

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
void*   rotary_thread(void *arg);
void*   leds_thread(void *arg);

void    button_handler(void);
void    fit_handler(void);


XStatus init_peripherals(void);

/****************************************************************************/
/***************************** Global Variables *****************************/
/****************************************************************************/

volatile unsigned int button_state;     // holds button values
int rotcnt;                             // holds rotary count
double ang_freq;                        // sine frequency in rad/sec

static u8 SPI_RcvBuf[2];               	// holds SPI receive data (4 zeros + 12-bits)

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
    pthread_t rotary;
    pthread_t leds;

    pthread_attr_t attr;
    struct sched_param spar;

    int ret;
    unsigned int ticks;
    int msg_id;

    xil_printf("----------------------------------------------------------------------------\r\n");
    xil_printf("ECE 544 Project 3 Starter Application \r\n");
    xil_printf("----------------------------------------------------------------------------\r\n");
    xil_printf("This Xilkernel based application reads the buttons and rotary on the FPGA \r\n"
               "development board and displays them on the LEDs.  Even though the application is\r\n"
               "simple it uses several of the synchronization and interprocess communication\r\n"
               "capabilities offered in the Xilkernel\r\n\r\n"
               "To demonstrate, press any of the buttons and/or turn rotary on the board.\r\n"
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

    // create the rotary thread

    ret = pthread_create (&rotary, &attr, (void*) rotary_thread, NULL);

    if (ret != 0) {

        xil_printf("ERROR (%d) IN MASTER THREAD: could not launch %s\r\n", ret, "rotary thread");
        xil_printf("FATAL ERROR: Master Thread Terminating\r\n");
        return (void*) -3;
    }

    else {
        xil_printf("MASTER: Rotary thread created\r\n");
    }

    // create the LEDs thread

    ret = pthread_create (&leds, &attr, (void*) leds_thread, NULL);

    if (ret != 0) {

        xil_printf("ERROR (%d) IN MASTER THREAD: could not launch %s\r\n", ret, "LEDs thread");
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

    // register the FIT handler

    ret = register_int_handler(FIT_INTR_NUM, (void*) fit_handler, NULL);

    if (ret != XST_SUCCESS) {
        return (void*) -4;
    }

    else {
        xil_printf("MASTER: FIT handler created successfully\r\n");
    }

    // enable interrupts...we're off to the races

    enable_interrupt(BTN_GPIO_INTR_NUM);
    enable_interrupt(FIT_INTR_NUM);

    xil_printf("MASTER: Interrupts have been enabled\r\n");

    // master thread main loop

    while(1) {

        // get & print the kernel time

        ticks = xget_clock_ticks();
/*        xil_printf("MASTER: %d ticks have elapsed\r\n", ticks);
        xil_printf("MASTER: The frequency is %d\r\n", (int) rotcnt*100);*/

        // repeat every second

        sleep(1000);
    }

    return NULL;
}

/****************************************************************************/
/************************** BUTTON THREAD ***********************************/
/****************************************************************************/

void* button_thread(void *arg) {

    while (1) {

        // wait for semaphore to become free
        sem_wait(&btn_press_sema);

        // read the buttons
        button_state = XGpio_DiscreteRead(&BTNInst, GPIO_CHANNEL_1);

        // yield remaining time to next thread
        yield();
    }

    return NULL;
}

/****************************************************************************/
/************************** ROTARY THREAD ***********************************/
/****************************************************************************/

void* rotary_thread(void *arg) {

    while (1) {

        PMDIO_ROT_readRotcnt(&rotcnt);
        rotcnt = MAX(0, MIN(rotcnt, 99));

        yield();
    }

    return NULL;
}

/****************************************************************************/
/*************************** LEDS THREAD ************************************/
/****************************************************************************/

void* leds_thread(void *arg) {

    while (1) {

        XGpio_DiscreteWrite(&LEDInst, GPIO_CHANNEL_1, button_state); 

        // yield remaining time to next thread
        yield();
    }

    return NULL;

}

/****************************************************************************/
/************************** BUTTON HANDLER **********************************/
/****************************************************************************/

void button_handler(void) {

    // update the global variable
    button_state = XGpio_DiscreteRead(&BTNInst, GPIO_CHANNEL_1);

    // make semaphore available again
    sem_post(&btn_press_sema);

    // acknowledge & clear interrupt flag
    XGpio_InterruptClear(&BTNInst, MSK_CLEAR_INTR_CH1);
    acknowledge_interrupt(BTN_GPIO_INTR_NUM);

}

/****************************************************************************/
/**************************** FIT HANDLER ***********************************/
/****************************************************************************/

void fit_handler(void) {

    static unsigned int count = 0x00;
    unsigned int micData = 0x00;

    static int pwm_set = 0x00;

    XStatus status;
    
    // Set the slave select mask. This mask is used by the transfer command
    // to indicate which slave select line to enable

    status = XSpi_SetSlaveSelect(&SpiInst, MSK_PMOD_MIC_SS);

    if (status != XST_SUCCESS) {
        print("FIT Handler: Failed to select slave!\r\n");
    }

    // Transfer the command and receive the mic dataADC count
    // Device is configured for 16-bit transfers, so only one transaction needed

    status = XSpi_Transfer(&SpiInst, SPI_RcvBuf, SPI_RcvBuf, 2);

    if (status != XST_SUCCESS) {
        print("FIT Handler: Failed to transfer SPI!\r\n");
    }

    // SPI transfer was successful
    // process it for cleaned up microphone signal

    micData = ((SPI_RcvBuf[0] << 4) | (SPI_RcvBuf[1] << 0)) & 0xFFF;


/*    static int pwm_set = 0x00;
    static double tempo = 0;

    ang_freq = (double) (rotcnt * 100) * 2 * PI;

    pwm_set = (int) 50 + (50*(sin((ang_freq * tempo))));

    PWM_SetParams(&PWMTimerInst, PWM_FREQUENCY, pwm_set);
    PWM_Start(&PWMTimerInst);

    tempo += 0.0001;*/

    pwm_set = MAX(0, MIN(micData >> 8, 100));

    PWM_SetParams(&PWMTimerInst, PWM_FREQUENCY, pwm_set);
    PWM_Start(&PWMTimerInst);


    // acknowledge interrupt

    acknowledge_interrupt(FIT_INTR_NUM);
}

/****************************************************************************/
/************************* INIT PERIPHERALS *********************************/
/****************************************************************************/

XStatus init_peripherals(void) {

    int status;                 // status from Xilinx Lib calls
    XSpi_Config *ConfigPtr;     // Pointer to SPI configuration data

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
    // rotary encoder is set to increment from 0 by 1% 

    status = PMDIO_initialize(PMD544IO_BASEADDR);

    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    PMDIO_ROT_init(DUTY_CYCLE_CHANGE, true);
    PMDIO_ROT_clear();

    // initialize the PWM timer/counter instance but do not start it
    // do not enable PWM interrupts. Clock frequency is the AXI clock frequency

    status = PWM_Initialize(&PWMTimerInst, PWM_TIMER_DEVICE_ID, false, CPU_CLOCK_FREQ_HZ);
    
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    // lookup SPI configuration

    ConfigPtr = XSpi_LookupConfig(SPI_DEVICEID);

    if (ConfigPtr == NULL) {
        print("ERROR: Couldn't find SPI device!\r\n");
        return XST_DEVICE_NOT_FOUND;
    }

    // initialize the SPI device

    status = XSpi_CfgInitialize(&SpiInst, ConfigPtr, ConfigPtr->BaseAddress);

    if (status != XST_SUCCESS) {
        print("ERROR: Couldn't initialize SPI device!\r\n");
        return XST_FAILURE;
    }

    // Perform a self-test to ensure that it was built correctly

    status = XSpi_SelfTest(&SpiInst);

    if (status != XST_SUCCESS) {
        print("ERROR: SPI device failed self-test!\r\n");
        return XST_FAILURE;
    }

    // Set the SPI device as a master,
    // SS goes low for entire transaction (does not toggle every 8 bits)
    // All other bits are OK as defaults

    status = XSpi_SetOptions(&SpiInst, XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION | XSP_CLK_ACTIVE_LOW_OPTION);

    if (status != XST_SUCCESS) {
        print("ERROR: Couldn't set SPI options!\r\n");
        return XST_FAILURE;
    }

    // Start the SPI driver so that the device is enabled,
    // and then disable the Global interrupt

    XSpi_Start(&SpiInst);
    XSpi_IntrGlobalDisable(&SpiInst);

    // successfully initialized... time to return

    return XST_SUCCESS;
}
