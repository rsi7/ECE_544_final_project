/**
*
* @file ChorusBuffer.c
*
* @author Rehan Iqbal (riqbal@pdx.edu)
* @copyright Portland State University, 2016
*
* This file implements the driver functions for the custom peripheral "ChorusBuffer". 
*
* Major driver functions:
*
* 	o ChorusBuffer_initialize: initialize the peripheral into the correct mode
* 	o ChorusBuffer_ReadLine: reads a 16-bit word from the buffer
*	o ChorusBuffer_WriteLine: writes a 16-bit word into the buffer
*/

/****************************************************************************/
/***************************** Include Files ********************************/
/****************************************************************************/

// include the header file (ChorusBuffer.h)
// this in turn points to the low-level header file (ChorusBuffer_l.h),
// which directly reads & writes memory addresses

#include "xparameters.h"
#include "stdio.h"
#include "xil_io.h"
#include "ChorusBuffer_l.h"
#include "CHorusBuffer.h"

/****************************************************************************/
/************************** Variable Definitions ****************************/
/****************************************************************************/

// Need to provide the Base Address for the ChorusBuffer peripheral
// so that memory reads & writes can be accomplished

u32 ChorusBuffer_BaseAddress;

/****************************************************************************/
/************************** Driver Functions ********************************/
/****************************************************************************/

/****************** Initialization & Configuration ************************/
/**
* Initialize the ChorusBuffer peripheral driver
*
* Saves the Base address of the ChorusBuffer peripheral and runs the self-test
*
* @param	BaseAddr is the base address of the ChorusBuffer register set
*
* @return
* 			- XST_SUCCESS	Initialization was successful.
			- XST_FAILURE 	Initialization failed on memory read & write tests.
*
* @note		This function can hang if the peripheral was not created correctly
* @note		The Base Address of the ChorusBuffer peripheral will be in xparameters.h
*
*****************************************************************************/

int ChorusBuffer_initialize(u32 BaseAddr) {

	ChorusBuffer_BaseAddress = BaseAddr;
	return ChorusBuffer_Reg_SelfTest(ChorusBuffer_BaseAddress);
}

/******************** ChorusBuffer_ReadLine ********************/	
/**
* Returns the value for the input buffer line argument.  
* 
* This works through a simple write on the slv_reg1 (CHORUSBUFFER_WRITE_ADDRESS_PORT_A),
* followed by a read on slv_reg5 (CHORUSBUFFER_DATA_OUTPUT_PORT_B).
*
* @param	Buffer line to be read (valid inputs: 0 - 65536)
*
* @return	16-bit value of that buffer line.
*
*
*****************************************************************************/

unsigned int ChorusBuffer_ReadLine (unsigned int bufline) {

	unsigned int read_data = 0x00000000;
	
	// first give the Block RAM the correct address for Port B
	CHORUSBUFFER_mWriteReg(ChorusBuffer_BaseAddress, CHORUSBUFFER_READ_ADDRESS_PORT_B, (bufline & 0x0000FFFF));

	// Port B always enabled, so simply read the data ouput value now
	read_data = (CHORUSBUFFER_mReadReg(ChorusBuffer_BaseAddress, CHORUSBUFFER_DATA_OUTPUT_PORT_B)) & 0x0000FFFF;
	
	return read_data;	
}

/******************** ChorusBuffer_WriteLine ********************/	
/**
* Writes a 16-bit value to the bufer line.  
* 
* This works through a simple read on the slv_reg1 (CHORUSBUFFER_WRITE_ADDRESS_PORT_A)
*
* @param	Buffer line to be written (valid inputs: 0 - 65536)
*			Buffer data to be written (valid inputs: 0 - 65536)
*
* @return	Nothing.
*
*
*****************************************************************************/

void ChorusBuffer_WriteLine(unsigned int bufline, unsigned int data) {
	
	// first give the Block RAM the correct address for Port A
	CHORUSBUFFER_mWriteReg(ChorusBuffer_BaseAddress, CHORUSBUFFER_WRITE_ADDRESS_PORT_A, (bufline & 0x0000FFFF));

	// now give the Block RAM the correct data to write on Port A
	CHORUSBUFFER_mWriteReg(ChorusBuffer_BaseAddress, CHORUSBUFFER_DATA_INPUT_PORT_A, (data & 0x0000FFFF));

	// now toggle the write enable input on Port A to push the value into the memory

	CHORUSBUFFER_mWriteReg(ChorusBuffer_BaseAddress, CHORUSBUFFER_WRITE_ENABLE_PORT_A, MSK_WRITE_ENABLE_HIGH);
	CHORUSBUFFER_mWriteReg(ChorusBuffer_BaseAddress, CHORUSBUFFER_WRITE_ENABLE_PORT_A, MSK_WRITE_ENABLE_LOW);
	
	return;
}