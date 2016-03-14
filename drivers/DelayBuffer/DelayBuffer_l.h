/**
*
* @file DelayBuffer_l.h
*
* @author Rehan Iqbal (riqbal@pdx.edu)
* @copyright Portland State University, 2016
*
* This header file contains identifiers & low-level driver prototypes for the
* custom peripheral "DelayBuffer".
*/

/****************************************************************************/
/**************************** Header Definition  ****************************/
/****************************************************************************/

// check if low-level header definition already exists...
// if not, define with the contents of this file

#ifndef DELAYBUFFER_L_H
#define DELAYBUFFER_L_H

/****************************************************************************/
/****************************** Include Files *******************************/
/****************************************************************************/

#include "xil_types.h"
#include "xil_io.h"
#include "xstatus.h"

#define DELAYBUFFER_WRITE_ENABLE_PORT_A 	0
#define DELAYBUFFER_WRITE_ADDRESS_PORT_A 	4
#define DELAYBUFFER_DATA_INPUT_PORT_A 		8
#define DELAYBUFFER_RSVD_00 				12
#define DELAYBUFFER_RSVD_01 				16
#define DELAYBUFFER_RSVD_02 				20
#define DELAYBUFFER_RSVD_03 				24
#define DELAYBUFFER_RSVD_04 				28

#define MSK_WRITE_ENABLE_HIGH 				0x00000001
#define MSK_WRITE_ENABLE_LOW				0x00000000


/**************************** Type Definitions *****************************/
/**
 *
 * Write a value to a DELAYBUFFER register. A 32 bit write is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is written.
 *
 * @param   BaseAddress is the base address of the DELAYBUFFERdevice.
 * @param   RegOffset is the register offset from the base to write to.
 * @param   Data is the data written to the register.
 *
 * @return  None.
 *
 * @note
 * C-style signature:
 * 	void DELAYBUFFER_mWriteReg(u32 BaseAddress, unsigned RegOffset, u32 Data)
 *
 */
#define DELAYBUFFER_mWriteReg(BaseAddress, RegOffset, Data) \
  	Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))

/**
 *
 * Read a value from a DELAYBUFFER register. A 32 bit read is performed.
 * If the component is implemented in a smaller width, only the least
 * significant data is read from the register. The most significant data
 * will be read as 0.
 *
 * @param   BaseAddress is the base address of the DELAYBUFFER device.
 * @param   RegOffset is the register offset from the base to write to.
 *
 * @return  Data is the data from the register.
 *
 * @note
 * C-style signature:
 * 	u32 DELAYBUFFER_mReadReg(u32 BaseAddress, unsigned RegOffset)
 *
 */
#define DELAYBUFFER_mReadReg(BaseAddress, RegOffset) \
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
 * @param   baseaddr_p is the base address of the DELAYBUFFER instance to be worked on.
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
XStatus DelayBuffer_Reg_SelfTest(u32 baseaddr);

#endif