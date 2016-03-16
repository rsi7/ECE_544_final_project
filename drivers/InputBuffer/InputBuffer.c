/**
*
* @file InputBuffer.c
*
* @author Rehan Iqbal (riqbal@pdx.edu)
* @copyright Portland State University, 2016
*
* This file implements the driver functions for the custom peripheral "InputBuffer". 
*
* Major driver functions:
*
* 	o InputBuffer_initialize: initialize the peripheral into the correct mode
* 	o InputBuffer_ReadLine: reads a 16-bit word from the buffer
*/

/****************************************************************************/
/***************************** Include Files ********************************/
/****************************************************************************/

// include the header file (InputBuffer.h)
// this in turn points to the low-level header file (InputBuffer_l.h),
// which directly reads & writes memory addresses

#include "xparameters.h"
#include "stdio.h"
#include "xil_io.h"
#include "InputBuffer_l.h"
#include "InputBuffer.h"

/****************************************************************************/
/************************** Variable Definitions ****************************/
/****************************************************************************/

// Need to provide the Base Address for the InputBuffer peripheral
// so that memory reads & writes can be accomplished

u32 InputBuffer_BaseAddress;

/****************************************************************************/
/************************** Driver Functions ********************************/
/****************************************************************************/

/****************** Initialization & Configuration ************************/
/**
* Initialize the InputBuffer peripheral driver
*
* Saves the Base address of the InputBuffer peripheral and runs the self-test
*
* @param	BaseAddr is the base address of the InputBuffer register set
*
* @return
* 			- XST_SUCCESS	Initialization was successful.
			- XST_FAILURE 	Initialization failed on memory read & write tests.
*
* @note		This function can hang if the peripheral was not created correctly
* @note		The Base Address of the InputBuffer peripheral will be in xparameters.h
*
*****************************************************************************/

int InputBuffer_initialize(u32 BaseAddr) {

	InputBuffer_BaseAddress = BaseAddr;
	return InputBuffer_Reg_SelfTest(InputBuffer_BaseAddress);
}

/******************** InputBuffer_ReadLine ********************/	
/**
* Returns the value for the input buffer line argument.  
* 
* This works through a simple write on the slv_reg1 (INPUTBUFFER_READ_ADDRESS_PORT_B),
* followed by a read on slv_reg5 (INPUTBUFFER_DATA_OUTPUT_PORT_B).
*
* @param	Buffer line to be read (valid inputs: 0 - 65536)
*
* @return	16-bit value of that buffer line.
*
*
*****************************************************************************/

unsigned int InputBuffer_ReadLine (unsigned int bufline) {

	unsigned int read_data = 0x00000000;
	
	// first give the Block RAM the correct address for Port B
	INPUTBUFFER_mWriteReg(InputBuffer_BaseAddress, INPUTBUFFER_READ_ADDRESS_PORT_B, (bufline & 0x0000FFFF));

	// Port B always enabled, so simply read the data ouput value now
	read_data = (INPUTBUFFER_mReadReg(InputBuffer_BaseAddress, INPUTBUFFER_DATA_OUTPUT_PORT_B)) & 0x0000FFFF;
	
	return read_data;	
}