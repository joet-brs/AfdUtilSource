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
'    File Name    :  Pipe.c
'
'    Purpose      :  This file has the code for the PipeClient for all the
'                    users of the AFDQueueManager APIs.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
17/Aug/1998	LSL	Don't close NULL pipes.
*/

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <windows.h>

#include "build.h" 
#include "Trace.h"

static char ModId[] = {__FILE__}; //for debugging

void DEBUG_BREAK(char *ModId, int lineNo, DWORD erc);

LPTSTR lpszPipename = "\\\\.\\pipe\\AFDQueueManager";
//LPTSTR lpszPipename = "\\\\MOMENTUM\\pipe\\QueueManager";

BOOL ConnectToPipe(BYTE *pPipeName, HANDLE *hPipe, BYTE **pServerName)
{
	DWORD dwErr, dwMode;

	if(pPipeName) 
		*pServerName = pPipeName;
	else 
		*pServerName = lpszPipename;

	while (1) {
		*hPipe = CreateFile(*pServerName,
							// pipe name
							GENERIC_READ |  // read/write access   
							GENERIC_WRITE, 
							0,              // no sharing          
							NULL,           // no security attr.    
							OPEN_EXISTING,  // opens existing pipe  
							0,              // default attributes   
							NULL);          // no template file     
		
		/* Break if the pipe handle is valid. */ 
		if (*hPipe != INVALID_HANDLE_VALUE)
			break; //leave the while loop

		/* Exit if an error other than ERROR_PIPE_BUSY occurs. */ 
		if ((dwErr = GetLastError()) != ERROR_PIPE_BUSY) {
			DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
			return (FALSE);
		}
		 
		/* All pipe instances are busy, so wait for 20 seconds. */ 
		if (! WaitNamedPipe(*pServerName, 20000) ) {
			dwErr = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
			return (FALSE);
		}
	} //while

	dwMode = PIPE_READMODE_MESSAGE; 
	
	if ( !(SetNamedPipeHandleState(	*hPipe,    // pipe handle           
									&dwMode,  // new pipe mode         
									NULL,     // don't set max. bytes  
									NULL))    // don't set max. time  
	   ) {
		dwErr = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
		return(FALSE);
	}

	return(TRUE);
}
 
 DWORD TransferToRemote(HANDLE hPipe, void *pBuffer, DWORD cbBuffer)
 {
	 BOOL  fSuccess;
	 DWORD cbRead; 
	 DWORD dwErr = 0;
	 // DWORD cbWritten;

	 
	// LSL 990315 SCR#556 Comment WriteFile/ReadFile below

	 /* Send a message to the pipe server. */ 
 /************************************************************ 
	 fSuccess = WriteFile(	hPipe,		// pipe handle      
							p,			// message          
							c,			// message length   
							&cbWritten,	// bytes written    
							NULL);      // not overlapped
	 if (!fSuccess) {
		 //why?
		 dwErr = GetLastError();
		 DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
		 return (FALSE);
	 }

	 //Sleep(1000);	// LSL 990218 We keep hanging on ReadFile below.
	 
	 fSuccess = ReadFile(	hPipe,	// pipe handle               
							p,		// buffer to receive reply   
							c,		// size of buffer            
							&cbRead,// number of bytes read      
							NULL);  // not overlapped  
	 

	 //TO_DO make sure that the written and read bytes have the same size
********************************************************************/	 
	 // LSL 990315 SCR#556 Replace Write/ReadFile pair with faster TransactNamedPipe()
	 
	 fSuccess = TransactNamedPipe(hPipe,pBuffer,cbBuffer,pBuffer,cbBuffer,&cbRead,NULL);	
	 
	 if (!fSuccess) {
		 dwErr = GetLastError();
		 DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
	 }

	 return (dwErr);
 }
 
 BOOL ClosePipeConnection(HANDLE hPipe)
 {
	 DWORD dwErr;

	 if (NULL == hPipe) return TRUE;     // LSL SCR# 29 Don't close NULL pipes.
	 if (!(CloseHandle(hPipe))) {
		 //why?
		 dwErr = GetLastError();
		 DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
		 return(FALSE);
	 }
	 return (TRUE);
 }

