/*****************************************************************************\                                                                                                                                                       *                                                                *                                                                                                                                                        *
'                       PROPRIETARY PROGRAM MATERIAL
'
'   This material is proprietary to Momentum Systems  and is not to be  
'   reproduced, used or disclosed except in accordance with program 
'   license or upon written authorization of
'       Momentum Systems ,
'       41 Twosome Dr Suite #9,
'       Moorestown NJ 08057.
'
'   COPYRIGHT (C) 1997  MOMENTUM SYSTEMS
'   All Rights Reserved
'   MOMENTUM PROPRIETARY
'
'******************************************************************************\
'                                          DISCLAIMER
'
'   The within information is not intended to be nor should such be construed 
'   as an affirmation of fact, representation or warranty by and the related 
'   materials are only furnished pursuant and subject to the terms and 
'   conditions of a duly executed license agreement.  The only warranties made 
'   by Momentum systems with respect to the products described in this material
'   are set forth in the above furnished mentioned agreement.
'
'   The customer should exercise care to assure that use of the software will be in
'   full compliance with laws, rules and regulations of the jurisdictions with 
'   respect to which it is used.
'
'*******************************************************************************
 
'*******************************************************************************
'
'    Product      :  AFD 
'    Module       :  AFD QueueManager Client (AfdAmClient.dll)
'    Date         :  March 1998
'
'    File Name    :  API.c
'
'    Purpose      :  This file has the code for Trace utilities.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

/*///////////////////////////////////////////////////////////////////////////
Module Name: Trace.cpp

This module is only compiled if the DEBUG flag is set.
This module logs tape requests for debugging.  The procedure TraceRq is
called just before a request is responded to.  Log entries are kept in
a buffer pointed to by 

	pLogBuffer.  

As each new entry is placed in the log,
each old entry is moved down one notch so that the most recent entry is
always at the top of the buffer.

To look at the contents of the buffer with the Visual C++ Debugger:
	Use the Watch window to find the address contained in pLogBuffer.
	Type the address of pLogBuffer into the address of the Memory window.
	Make the Memory window full width.

Revision History:

21/July/1997 AA MomentumSystems AFD product.


///////////////////////////////////////////////////////////////////////////*/
/*
17/Aug/1998	LSL Fixes in ReturnPipeNameAndHandle().
*/


#include <basic.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DEBUG
BOOL ReturnPipeNameAndHandle(HANDLE *hPipe, BYTE *PipeName);
#endif
void DEBUG_BREAK(char *ModId, int lineNo, DWORD erc) {
#ifdef _DEBUG
	HANDLE hPipe=NULL; //LSL SCR# 30 Init to NULL
	BYTE *pPipename=NULL; //LSL SCR# 30 Init to NULL
	BOOL fSuccess;
#endif
	static char DebugInfo[200];  //LSL SCR# 26 change size from 100 to 200

	sprintf(DebugInfo, "ThreadId = 0x%08X. Debug break in Module : %s, Line number : %d. Last erc = 0x%08x.\n",
			GetCurrentThreadId(), ModId, lineNo, erc);
//	OutputDebugString(DebugInfo);

//	if (fDebugMode) {
#ifdef _DEBUG
		printf(DebugInfo);
		fSuccess = ReturnPipeNameAndHandle(&hPipe, pPipename);
		if (fSuccess)
			sprintf(DebugInfo, "Pipe name is : %s. Pipe handle = %4x.", pPipename, hPipe);
		else
			sprintf(DebugInfo, "Could not get pipe information.");
//		OutputDebugString(DebugInfo);
		printf(DebugInfo);
		printf("\npress <Enter> to continue.");
		scanf("%c", DebugInfo);
#else
	//}
	//else
		//DebugBreak(); Debug Break is pointless, write this to a file.
#endif
}
#ifdef __cplusplus
}
#endif

