/**
*
* @file audio_demo.c
*
* @author Rehan Iqbal (riqbal@pdx.edu)
* @copyright Portland State University, 2016
*
* This file implements the driver functions for the custom peripheral "audio_demo". 
*
* Major driver functions:
*
* 	o audio_demo_initialize: initialize the peripheral into the correct mode
* 	o audio_demo_read_mic: captures the 16-bit unsigned mic data
*/

/****************************************************************************/
/***************************** Include Files ********************************/
/****************************************************************************/

// include the header file (audio_demo.h)
// this in turn points to the low-level header file (audio_demo_l.h),
// which directly reads & writes memory addresses

#include "xparameters.h"
#include "stdio.h"
#include "xil_io.h"
#include "audio_demo_l.h"
#include "audio_demo.h"

/****************************************************************************/
/************************** Variable Definitions ****************************/
/****************************************************************************/

// Need to provide the Base Address for the audio_demo peripheral
// so that memory reads & writes can be accomplished

u32 audio_demo_BaseAddress;

/****************************************************************************/
/************************** Driver Functions ********************************/
/****************************************************************************/

/****************** Initialization & Configuration ************************/
/**
* Initialize the audio_demo peripheral driver
*
* Saves the Base address of the audio_demo peripheral and runs the self-test
*
* @param	BaseAddr is the base address of the audio_demo register set
*
* @return
* 			- XST_SUCCESS	Initialization was successful.
			- XST_FAILURE 	Initialization failed on memory read & write tests.
*
* @note		This function can hang if the peripheral was not created correctly
* @note		The Base Address of the audio_demo peripheral will be in xparameters.h
*
*****************************************************************************/

int audio_demo_initialize(u32 BaseAddr) {

	audio_demo_BaseAddress = BaseAddr;
	return audio_demo_Reg_SelfTest(audio_demo_BaseAddress);
}

/******************** Get count for high / low interval ********************/	
/**
* Returns the value for the high / low count register in hw_detect.v
* This value corresponds to the length of time the PWM pulse was
* in the 'high' or 'low' state.  
* 
* This works through a simple read on the slv_reg0 / slv_reg_0 memory addresses,
* which is at (BaseAddress + 0) / (BaseAddress + 4) respectively.
*
* @param	Register to be read (valid inputs: HIGH, LOW)
*
* @return	Value of the high / low count register from hw_detect.v
* 			Provided as an unsigned integer in little-endian format.
* 			Restricted to the range (0 , 4MEG) 
*
* @note		See the Verilog file 'hw_detect.v' for specifics on how
* 			the output count values are generated.
*
*****************************************************************************/

unsigned int audio_demo_read_mic(void) {
	
	unsigned int count = 0x00000000;

	count = audio_demo_mReadReg(audio_demo_BaseAddress, AUDIO_DEMO_MIC_DATA_OFFSET);
	
	return count;
}