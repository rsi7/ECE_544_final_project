/**
*
* @file ChorusBuffer_l.h
*
* @author Rehan Iqbal (riqbal@pdx.edu)
* @copyright Portland State University, 2016
*
* This header file contains identifiers & low-level driver prototypes for the
* custom peripheral "ChorusBuffer".
*/

/****************************************************************************/
/**************************** Header Definition  ****************************/
/****************************************************************************/

// check if low-level header definition already exists...
// if not, define with the contents of this file

#ifndef CHORUSBUFFER_L_H
#define CHORUSBUFFER_L_H

/****************************************************************************/
/****************************** Include Files *******************************/
/****************************************************************************/

#include "xil_types.h"
#include "xil_io.h"
#include "xstatus.h"

#define CHORUSBUFFER_WRITE_ENABLE_PORT_A 	0
#define CHORUSBUFFER_WRITE_ADDRESS_PORT_A 	4
#define CHORUSBUFFER_DATA_INPUT_PORT_A 		8
#define CHORUSBUFFER_READ_ADDRESS_PORT_B 	12
#define CHORUSBUFFER_DATA_OUTPUT_PORT_B 	16
#define CHORUSBUFFER_RSVD_00 				20
#define CHORUSBUFFER_RSVD_01 				24
#define CHORUSBUFFER_RSVD_02 				28

#define MSK_WRITE_ENABLE_HIGH 				0x00000001
#define MSK_WRITE_ENABLE_LOW				0x00000000

/**************************** Type Definitions *****************************/
/**
 *
 * Write a value to a CHORUSBUFFER register. A 32 bit write is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is written.
 *
 * @param   BaseAddress is the base address of the CHORUSBUFFERdevice.
 * @param   RegOffset is the register offset from the base to write to.
 * @param   Data is the data written to the register.
 *
 * @return  None.
 *
 * @note
 * C-style signature:
 * 	void CHORUSBUFFER_mWriteReg(u32 BaseAddress, unsigned RegOffset, u32 Data)
 *
 */
#define CHORUSBUFFER_mWriteReg(BaseAddress, RegOffset, Data) \
  	Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))

/**
 *
 * Read a value from a CHORUSBUFFER register. A 32 bit read is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is read from the register. The most significant data
 * will be read as 0.
 *
 * @param   BaseAddress is the base address of the CHORUSBUFFER device.
 * @param   RegOffset is the register offset from the base to write to.
 *
 * @return  Data is the data from the register.
 *
 * @note
 * C-style signature:
 * 	u32 CHORUSBUFFER_mReadReg(u32 BaseAddress, unsigned RegOffset)
 *
 */
#define CHORUSBUFFER_mReadReg(BaseAddress, RegOffset) \
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
 * @param   baseaddr_p is the base address of the CHORUSBUFFER instance to be worked on.
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
XStatus ChorusBuffer_Reg_SelfTest(u32 baseaddr);

#endif