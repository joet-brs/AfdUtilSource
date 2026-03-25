/*****************************************************************************\                                                                                                                                                       *                                                                *                                                                                                                                                        *
'                       PROPRIETARY PROGRAM MATERIAL
'
'   This material is proprietary to Momentum Systems  and is not to be  
'   reproduced, used or disclosed except in accordance with program 
'   license or upon written authorization of
'       Momentum Systems ,
'       41 Twlsome Dr Suite #9,
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
'    Module       :  AFDScmInterface
'    Date         :  Feb. 1998
'
'    File Name    :  Debug.c
'
'    Purpose      :  This file has the code for handling some debugging stuff for
'                    Windows NT System Services.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

/*///////////////////////////////////////////////////////////////////////////
Module Name: debug.cpp

This module contains debugging procedures defined in debug.h:
	CauseFault
	CheckErc
	DebugPrintMsg
	DebugPrintFormatMsg
	DebugPrintStatus
	ExceptionFilter

The DebugPrint procedures all take a msgId.  
The msgId refers to a message id from QMMsg.h.
QMMsg.h is created by the message compiler from QMMsg.mc.
The procedures FormatMessage and LogEvent can then access the
message text from the .exe.

Revision History:

21/July/1997 AA MomentumSystems AFD product.

*/

#include "basic.h"
#include <stdio.h>
#include <stdarg.h>
#include "AfdLog_Debug.h"

static char *ModId[]={__FILE__};
char tmpErr[]={"AfdErr.dll"};
LPCTSTR strAfdErr = tmpErr;
//MSG_EXCEPTION
extern DWORD msg_exception;

/*
Print simple message from message file and log message to the event log.
*/

void DebugPrintMsg(DWORD msgId) // Id of message
{
#if DEBUG
	HMODULE hAfdErr;
	//HINSTANCE hInst = NULL;
	DWORD dwCount;
//To obtain description strings 

//Use the RegOpenKey function to open the event source. 
//Use the RegQueryValueEx function to obtain the EventMessageFile value for the event source, which is the name of the event message DLL. 
//Use the LoadLibraryEx function to load the event message DLL. 
//Use the FormatMessage function to obtain the description from the DLL and add the insertion strings. 

	// Create the output message.
    char outputMsg[200];

	hAfdErr = LoadLibrary("AfdErr.dll");
	//hAfdErr = GetModuleHandle("AfdErr.dll");
    dwCount = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, /* If message source below 
															   is NULL, the current 
															   process’ application image 
															   file will be searched*/
								  hAfdErr,			// address of  message source						
								  msgId,		// requested message identifier
								  0,			// GetUserDefaultLangID(),
								  outputMsg,	// address of message buffer
								  sizeof(outputMsg), // maximum size of message buffer				
								  NULL			// address of array of message inserts				
								 );

	// display the message to the Debugger.
    if(dwCount)
	OutputDebugString(outputMsg);
	if (hAfdErr) FreeLibrary(hAfdErr);
#endif // DEBUG

	// Write the message to the event log.
	LogNTEvent(msgId, NULL); 

} //DebugPrintMsg

/////////////////////////////////////////////////////////////////////////////
// For Debugging -- find out where 1104 comes from.
void Got1104()
{
	return;
}

/* Print simple message from message file, followed by Status = Show Status in both 
decimal and hex.
*/
void DebugPrintStatus(DWORD msgId, // id of message
					  DWORD Status // NT Status to display
					 )
{

    char StatusMsg[200];
	LPTSTR dwArgs[4];
    char outputMsg[200];
	DWORD dwCount;
	HMODULE hAfdErr; 	
    //HINSTANCE hInst = NULL;
	// Create formatted text from the Status passed in for the debugger and the event log.

    sprintf(StatusMsg, "Status = %lu (0x%X).", Status, Status);
	if(Status == 1104) 
		Got1104(); // for Debugging only

	/* Set up array of 32-bit values that are used as insert values in the 
	// formatted message. %1 in the format string indicates the first value
	// %2 indicates the second argument; and so on.
	// The interpretation of each 32-bit value depends on the formatting 
	// information associated with the insert in the message definition. 
	// The default is to treat each value as a pointer to a null-terminated
	// string.
	*/

	dwArgs[0] = StatusMsg;
	dwArgs[1] = 0;

#if DEBUG

	// Create output message for the debugger.
	hAfdErr = LoadLibrary("AfdErr.dll");
	//hAfdErr = GetModuleHandle("AfdErr.dll");
    dwCount = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | 
							FORMAT_MESSAGE_ARGUMENT_ARRAY,
							hAfdErr,
							msgId,
							0, // GetUserDefaultLangID(),
							outputMsg,
							sizeof(outputMsg),
							(va_list*)dwArgs
							);

	// display the message to the Debugger.
	if(dwCount)
    OutputDebugString(outputMsg);
	if (hAfdErr) FreeLibrary(hAfdErr);
#endif // DEBUG

	/* Write the message to the event log.
	// If ERROR_NO_DATA_DETECTED, don't log this as an error.
	// This occurs when the tape is blank and is being read to see if
	// there is anything on it.
	*/
	if(Status != ERROR_NO_DATA_DETECTED)
		LogNTEvent(msgId, StatusMsg); 

} //DebugPrintStatus

/////////////////////////////////////////////////////////////////////////////
// Print formatted message from message file.
void DebugPrintFormatMsg(DWORD msgId, // id of message 
						 LPTSTR formatList, ... // pointer to format specification
						)
{

	/* The va_arg, va_end, and va_start routines are macros that provide a 
	// portable way to access the  arguments to a function when the 
	// function takes a variable number of arguments.
	*/
    va_list arg_ptr;
	char formatMessage[200];
	LPTSTR dwArgs[4] = {formatMessage, 0 };
	char outputBuf[200];
	DWORD dwCount;
	HMODULE hAfdErr; 
	//HINSTANCE hInst = NULL;
    // Set pointer to beginning of argument list
    va_start(arg_ptr,	 // Pointer to list of arguments
    		 formatList // Parameter preceding first optional argument 
			);

    // Create formatted message from the variable length list that was passed in.

    vsprintf(formatMessage, formatList, arg_ptr);

	/* Set up array of 32-bit values that are used as insert values in the 
	// formatted message. %1 in the format string indicates the first value
	// %2 indicates the second argument; and so on.
	// The interpretation of each 32-bit value depends on the formatting 
	// information associated with the insert in the message definition. 
	// The default is to treat each value as a pointer to a null-terminated
	// string.
	*/

#if DEBUG

	// Create output message for the debugger.
	hAfdErr = LoadLibrary("AfdErr.dll");
	//hAfdErr = GetModuleHandle("AfdErr.dll");
    dwCount = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | 
							FORMAT_MESSAGE_ARGUMENT_ARRAY,
							hAfdErr,
							msgId,
							0, // GetUserDefaultLangID(),
							outputBuf,
							sizeof(outputBuf),
							(va_list*)dwArgs
						   );

	// display the message to the Debugger.
	if(dwCount)
    OutputDebugString(outputBuf);
	if (hAfdErr) FreeLibrary(hAfdErr);
#endif // DEBUG

	// Write the message to the event log.
	LogNTEvent(msgId, dwArgs[0]); 

}

/////////////////////////////////////////////////////////////////////////////
void DebugPrint(
    PCHAR DebugMessage,
    ...
    )

{

#if DEBUG

	char buffer[128];
	va_list ap;

	va_start(ap, DebugMessage);


    vsprintf(buffer, DebugMessage, ap);

    OutputDebugString(buffer);

    va_end(ap);

#endif // DEBUG

} // DebugPrint()

BOOLEAN								// Return TRUE if erc not OK
CheckErc(PCHAR DebugMessage,		// Display message if debugging
		 ercType erc				// status code
		)
{
#if DEBUG
	if(erc != 0)
	{
		OutputDebugString(DebugMessage);
		DebugPrint(" failed, erc = %d\n", erc);
	}
    
#endif // DEBUG

return erc != 0;
}


/* This routine is used to decide if we should or should not handle an 
exception status that is being raised.  
*/
ULONG						// returns EXCEPTION_EXECUTE_HANDLER
ExceptionFilter(IN PEXCEPTION_POINTERS ExceptionPointer) 
{
    ercType ExceptionCode;
	PVOID ExceptionAddress;

    // Get the exception code
    ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;

	// Get the address where the exception occurred.
	ExceptionAddress = ExceptionPointer->ExceptionRecord->ExceptionAddress;

	// Log the address where the exception occurred and the code.
	DebugPrintFormatMsg(msg_exception, "Address = %X, code = %X", 
		ExceptionAddress, ExceptionCode);

    return EXCEPTION_EXECUTE_HANDLER;
} // ExceptionFilter

/* This procedure is only for testing.  It will cause an address violation.
*/
void CauseFault()
{
	CHAR* pChar = NULL;
	*pChar = 0;
}

