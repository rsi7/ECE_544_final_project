/**
*
* @file audio_demo.h
*
* @author Rehan Iqbal (riqbal@pdx.edu)
* @copyright Portland State University, 2016
*
* This header file contains identifiers and high-level driver prototypes for the
* custom peripheral "audio_demo".
*/

/****************************************************************************/
/**************************** Header Definition  ****************************/
/****************************************************************************/

// check if header definition already exists...
// if not, define with the contents of this file

#ifndef AUDIO_DEMO_H
#define AUDIO_DEMO_H

/****************************************************************************/
/****************************** Include Files *******************************/
/****************************************************************************/

#include "xil_types.h"
#include "xstatus.h"
#include "stdbool.h"
#include "audio_demo_l.h"

/****************************************************************************/
/************************** Constant Definitions ****************************/
/****************************************************************************/

/** @name Bit Masks
*
* Bit masks for the audio_demo registers.
*
* All of the registers in the audio_demo periheral are 32-bits wide
*
* @{
*/

// Masks for hardware detection module

#define		AUDIO_DEMO_UPPER_HALF_MASK 		0xFFFF0000
#define		AUDIO_DEMO_LOWER_HALF_MASK		0x0000FFFF

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
int audio_demo_initialize(u32 BaseAddr);

// Get mirophone data in 16-bit unsigned format
unsigned int audio_demo_read_mic(void);

#endif