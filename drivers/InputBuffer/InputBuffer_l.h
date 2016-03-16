/**
*
* @file InputBuffer_l.h
*
* @author Rehan Iqbal (riqbal@pdx.edu)
* @copyright Portland State University, 2016
*
* This header file contains identifiers & low-level driver prototypes for the
* custom peripheral "InputBuffer".
*/

/****************************************************************************/
/**************************** Header Definition  ****************************/
/****************************************************************************/

// check if low-level header definition already exists...
// if not, define with the contents of this file

#ifndef INPUTBUFFER_L_H
#define INPUTBUFFER_L_H


/****************************************************************************/
/****************************** Include Files *******************************/
/****************************************************************************/

#include "xil_types.h"
#include "xil_io.h"
#include "xstatus.h"

#define INPUTBUFFER_RSVD_00 			 	0
#define INPUTBUFFER_RSVD_01 				4
#define INPUTBUFFER_RSVD_02			 		8
#define INPUTBUFFER_READ_ADDRESS_PORT_B 	12
#define INPUTBUFFER_DATA_OUTPUT_PORT_B 		16
#define INPUTBUFFER_RSVD_03					20
#define INPUTBUFFER_RSVD_04 				24
#define INPUTBUFFER_RSVD_05 				28

#define MSK_WRITE_ENABLE_HIGH 				0x00000001
#define MSK_WRITE_ENABLE_LOW				0x00000000

/**************************** Type Definitions *****************************/
/**
 *
 * Write a value to a INPUTBUFFER register. A 32 bit write is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is written.
 *
 * @param   BaseAddress is the base address of the INPUTBUFFERdevice.
 * @param   RegOffset is the register offset from the base to write to.
 * @param   Data is the data written to the register.
 *
 * @return  None.
 *
 * @note
 * C-style signature:
 * 	void INPUTBUFFER_mWriteReg(u32 BaseAddress, unsigned RegOffset, u32 Data)
 *
 */
#define INPUTBUFFER_mWriteReg(BaseAddress, RegOffset, Data) \
  	Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))

/**
 *
 * Read a value from a INPUTBUFFER register. A 32 bit read is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is read from the register. The most significant data
 * will be read as 0.
 *
 * @param   BaseAddress is the base address of the INPUTBUFFER device.
 * @param   RegOffset is the register offset from the base to write to.
 *
 * @return  Data is the data from the register.
 *
 * @note
 * C-style signature:
 * 	u32 INPUTBUFFER_mReadReg(u32 BaseAddress, unsigned RegOffset)
 *
 */
#define INPUTBUFFER_mReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))

/************************** Function Prototypes ****************************/
/**
 *
 * Run a self-test on the driver/device. Note this may be a destructive test if
 * resets of the device are performed.
 *
 * If the hardware system is not built correctly, this function may never
 * return to the caller.
 *
 * @param   baseaddr_p is the base address of the INPUTBUFFER instance to be worked on.
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
XStatus InputBuffer_Reg_SelfTest(u32 baseaddr);

#endif