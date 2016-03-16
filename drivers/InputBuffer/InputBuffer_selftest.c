/**
*
* @file InputBuffer.c
*
* @author Rehan Iqbal (riqbal@pdx.edu)
* @copyright Portland State University, 2016
*
* This file implements the self-test function for the custom peripheral "InputBuffer". 
* It writes to the last three memory addresses of the peripheral and then
* reads those values back to make sure everything is correct.
*
* If there is any discrepancy between the read/write, it will return failure status.
* Otherwise, it will return a successful status.
*
*/

/****************************************************************************/
/***************************** Include Files ********************************/
/****************************************************************************/

#include "InputBuffer_l.h"
#include "xparameters.h"
#include "stdio.h"
#include "xil_io.h"

/****************************************************************************/
/************************** Constant Definitions ****************************/
/****************************************************************************/

#define READ_WRITE_MUL_FACTOR 0x10

/************************** Function Definitions ***************************/
/**
 *
 * Run a self-test on the driver/device. Note this may be a destructive test if
 * resets of the device are performed.
 *
 * If the hardware system is not built correctly, this function may never
 * return to the caller.
 *
 * @param   baseaddr_p is the base address of the INPUTBUFFERinstance to be worked on.
 *
 * @return
 *
 *    - XST_SUCCESS   if all self-test code passed
 *    - XST_FAILURE   if any self-test code failed
 *
 * @note    Caching must be turned off for this function to work.
 * @note    Self test may fail if data memory and device are not on the same bus.
 *
 */
XStatus InputBuffer_Reg_SelfTest(u32 baseaddr) {

	int write_loop_index;
	int read_loop_index;

	xil_printf("******************************\n\r");
	xil_printf("*  * INPUTBUFFER Self Test   *\n\r");
	xil_printf("******************************\n\n\r");

	// write values to the last three registers...
	// AXI: slv_reg5, slv_reg6 & slv_reg7

	xil_printf("User logic slave module test...\n\r");

	for (write_loop_index = 5 ; write_loop_index < 8; write_loop_index++) {

	  	INPUTBUFFER_mWriteReg(baseaddr, write_loop_index*4, (write_loop_index+1)*READ_WRITE_MUL_FACTOR);
		xil_printf ("\nWrote to memory address %x\n", (int)baseaddr + write_loop_index*4);
	}

	// now read back the written values and make sure they match

	for (read_loop_index = 5 ; read_loop_index < 8; read_loop_index++) {

		if ( INPUTBUFFER_mReadReg (baseaddr, read_loop_index*4) != (read_loop_index+1)*READ_WRITE_MUL_FACTOR) {

			xil_printf ("Error reading register value at address %x\n", (int)baseaddr + read_loop_index*4);
			return XST_FAILURE;
		}
	}

	// no hazards encountered... return successful status

	xil_printf("   - slave register write/read passed\n\n\r");

	return XST_SUCCESS;
}