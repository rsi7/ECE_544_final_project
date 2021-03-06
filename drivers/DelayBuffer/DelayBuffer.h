/**
*
* @file DelayBuffer.h
*
* @author Rehan Iqbal (riqbal@pdx.edu)
* @copyright Portland State University, 2016
*
* This header file contains identifiers and high-level driver prototypes for the
* custom peripheral "DelayBuffer".
*/

/****************************************************************************/
/**************************** Header Definition  ****************************/
/****************************************************************************/

// check if header definition already exists...
// if not, define with the contents of this file

#ifndef DELAYBUFFER_H
#define DELAYBUFFER_H

/****************************************************************************/
/****************************** Include Files *******************************/
/****************************************************************************/

#include "xil_types.h"
#include "xstatus.h"
#include "stdbool.h"
#include "DelayBuffer_l.h"

/****************************************************************************/
/************************** Constant Definitions ****************************/
/****************************************************************************/

/** @name Bit Masks
*
* Bit masks for the DelayBuffer registers.
*
* All of the registers in the DelayBuffer periheral are 32-bits wide
*
* @{
*/

// Masks for hardware detection module

#define		DELAYBUFFER_UPPER_HALF_MASK 	0xFFFF0000
#define		DELAYBUFFER_LOWER_HALF_MASK		0x0000FFFF

/* @} */

/****************************************************************************/
/***************** Macros (Inline Functions) Definitions ********************/
/****************************************************************************/

// If these pre-processors are not defined at the top-level application,
// we can use these definitions instead.
// They will not override pre-existing definitions, though.

#ifndef MIN(a, b)
#define MIN(a, b)  ( ((a) <= (b)) ? (a) : (b) )
#endif

#ifndef MAX(a, b)
#define MAX(a, b)  ( ((a) >= (b)) ? (a) : (b) )
#endif

/****************************************************************************/
/************************** Function Prototypes *****************************/
/****************************************************************************/

// Initialization function
int DelayBuffer_initialize(u32 BaseAddr);

// Write a line in the buffer
void DelayBuffer_WriteLine(unsigned int bufline, unsigned int data);

#endif