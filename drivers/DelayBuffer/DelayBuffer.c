/**
*
* @file DelayBuffer.c
*
* @author Rehan Iqbal (riqbal@pdx.edu)
* @copyright Portland State University, 2016
*
* This file implements the driver functions for the custom peripheral "DelayBuffer". 
*
* Major driver functions:
*
* 	o DelayBuffer_initialize: initialize the peripheral into the correct mode
*	o DelayBuffer_WriteLine: writes a 16-bit word into the buffer
*/

/****************************************************************************/
/***************************** Include Files ********************************/
/****************************************************************************/

// include the header file (DelayBuffer.h)
// this in turn points to the low-level header file (DelayBuffer.h),
// which directly reads & writes memory addresses

#include "xparameters.h"
#include "stdio.h"
#include "xil_io.h"
#include "DelayBuffer_l.h"
#include "DelayBuffer.h"

/****************************************************************************/
/************************** Variable Definitions ****************************/
/****************************************************************************/

// Need to provide the Base Address for the DelayBuffer peripheral
// so that memory reads & writes can be accomplished

u32 DelayBuffer_BaseAddress;

/****************************************************************************/
/************************** Driver Functions ********************************/
/****************************************************************************/

/****************** Initialization & Configuration ************************/
/**
* Initialize the DelayBuffer peripheral driver
*
* Saves the Base address of the DelayBuffer peripheral and runs the self-test
*
* @param	BaseAddr is the base address of the DelayBuffer register set
*
* @return
* 			- XST_SUCCESS	Initialization was successful.
			- XST_FAILURE 	Initialization failed on memory read & write tests.
*
* @note		This function can hang if the peripheral was not created correctly
* @note		The Base Address of the DelayBuffer peripheral will be in xparameters.h
*
*****************************************************************************/

int DelayBuffer_initialize(u32 BaseAddr) {

	DelayBuffer_BaseAddress = BaseAddr;
	return DelayBuffer_Reg_SelfTest(DelayBuffer_BaseAddress);
}

/******************** DelayBuffer_WriteLine ********************/	
/**
* Writes a 16-bit value to the buffer line.
* 
* This works through a simple read on the slv_reg1 (DELAYBUFFER_WRITE_ADDRESS_PORT_A)
*
* @param	Buffer line to be written (valid inputs: 0 - 65536)
*			Buffer data to be written (valid inputs: 0 - 65536)
*
* @return	Nothing.
*
*
*****************************************************************************/

void DelayBuffer_WriteLine(unsigned int bufline, unsigned int data) {
	
	// first give the Block RAM the correct address for Port A
	DELAYBUFFER_mWriteReg(DelayBuffer_BaseAddress, DELAYBUFFER_WRITE_ADDRESS_PORT_A, (bufline & 0x0000FFFF));

	// now give the Block RAM the correct data to write on Port A
	DELAYBUFFER_mWriteReg(DelayBuffer_BaseAddress, DELAYBUFFER_DATA_INPUT_PORT_A, (data & 0x0000FFFF));

	// now toggle the write enable input on Port A to push the value into the memory

	DELAYBUFFER_mWriteReg(DelayBuffer_BaseAddress, DELAYBUFFER_WRITE_ENABLE_PORT_A, MSK_WRITE_ENABLE_HIGH);
	DELAYBUFFER_mWriteReg(DelayBuffer_BaseAddress, DELAYBUFFER_WRITE_ENABLE_PORT_A, MSK_WRITE_ENABLE_LOW);
	
	return;
}