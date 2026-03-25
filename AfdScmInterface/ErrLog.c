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
'    File Name    :  ErrLog.c
'
'    Purpose      :  This file has the code for error logging and event logging 
'                    Windows NT System Services.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

/*///////////////////////////////////////////////////////////////////////////
Module Name: errlog.cpp

This module contains code to use the error log so that errors can be 
viewed with the Event Viewer.

Revision History:

21/July/1997 AA MomentumSystems AFD product.

///////////////////////////////////////////////////////////////////////////*/

#include "basic.h"
#include <stdio.h>
#include <string.h>
#include "AfdLog_Debug.h"
#include "AfdErr.h"

static char *ModId[]={__FILE__};

static char basePath[]={"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\"};
extern char *pServiceName;


USHORT mpSevToEventType [4] = {EVENTLOG_INFORMATION_TYPE,
							   EVENTLOG_INFORMATION_TYPE,
							   EVENTLOG_WARNING_TYPE,
							   EVENTLOG_ERROR_TYPE
							  };

/*
Initialize the event log this procedure is called once at the beginning of 
execution.
*/
ercType LogNTInitialize () {
	ercType erc;
	HKEY  hKey;
	ULONG qData;
	ercType Status;
	CHAR path[256];

// Begin
	LONG ret;
	char  pAfdRoot[255]; 
	DWORD cbRet = 255;

	// Get the Afd base path
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,		//handle of openkey
					   "SOFTWARE\\Momentum",				//address of name of sub-key to open
					   0,						//reserved
					   KEY_QUERY_VALUE,			//type of access desired
					   &hKey				//returned handle
					  );
	if (ret != ERROR_SUCCESS)
		return(ret);

	ret = RegQueryValueEx(hKey,			//handle of open key
						  "AfdRoot",				//address of name of value to query
						  0,					//reserved
						  NULL,				//key type not required
						  pAfdRoot,				//address of data buffer
						  &cbRet					//size of data buff
						 );

	RegCloseKey(hKey);
	strcat(pAfdRoot,"\\bin\\AfdErr.dll");
	// End Get the Afd base path


	/* Create a registry entry for the application so that it will be visible to 
	the EventLog.
	*/
	strcpy(path,"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\");
	if (strcat_s(path, sizeof(path), pServiceName)) return(ERROR_NOT_ENOUGH_MEMORY);

	erc = RegCreateKey(HKEY_LOCAL_MACHINE, path, &hKey);
	if (erc)
		return erc;

	// Get the path of the currently running program so we can use it
	// to specify the name of the message file.

	Status = GetModuleFileName(NULL,
							   /*Identifies the module whose executable filename is
							   being requested. If the parameter is NULL, 
							   GetModuleFileName returns the path for the file used 
							   to create the calling process*/
							   path,		// address of buffer for module path
							   sizeof(path) // size of buffer in characters					
							  );
// LSL 990126 
	erc = RegSetValueEx(hKey, 
						"EventMessageFile", // Points to a string containing the 
											// name of the value to set
						0,					// Reserved; must be zero.
						REG_EXPAND_SZ,		// Specifies the type of information 
											// to be stored as the value’s data
						(UCHAR*)pAfdRoot,	// Points to a buffer containing the data to 
											// be stored with the specified value name
						strlen(pAfdRoot)	// Specifies the size, in bytes, of the path
					   );
	
	if (erc)
		return erc;

	qData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;

	erc = RegSetValueEx(hKey, "TypesSupported", 0, REG_DWORD, (LPBYTE)&qData, 4);

	RegCloseKey(hKey);
	return erc;
}

/////////////////////////////////////////////////////////////////////////////
// Write event to the Application event log.
ercType LogNTEvent(DWORD msgId,	// Refers to msgId from TapeMsg.h 
				 LPCSTR lpString // string to merge with message
				)
{
	HANDLE hLog;
	ercType erc;
	const CHAR* rgsz[] = {lpString};
	//MessageBox(NULL,pServiceName,"Hello",MB_OK);
	// The RegisterEventSource function returns a handle that can be used 
	// with the ReportEvent function to log an event. 
	hLog = RegisterEventSource(0, pServiceName);
	if (!hLog)
		return ercInconsistency;

/********* Error Types
EVENTLOG_SUCCESS                0X0000
EVENTLOG_ERROR_TYPE             0x0001
EVENTLOG_WARNING_TYPE           0x0002
EVENTLOG_INFORMATION_TYPE       0x0004
*********************/

	if (!ReportEvent(hLog,		// handle returned by RegisterEventSource
					 mpSevToEventType[msgId >> 30], // event type to log
					 0, 							// event category
					 msgId,						// event identifier (AfdErr.dll)
					 NULL,							// user security identifier (optional)
					 (WORD) (lpString ? 1 : 0),  // number of strings to merge with message
					 0, 						// size of binary data in bytes
					 rgsz,	// array of strings to merge with message
					 NULL						// address of binary data
		))
		erc = GetLastError();
	else
		erc = ercOk;

	DeregisterEventSource(hLog);
	return erc;
} // LogEvent

/////////////////////////////////////////////////////////////////////////////
// Write 2 string event to the Application event log.
ercType LogNTEvent2(DWORD msgId,	// Refers to msgId from TapeMsg.h 
				 LPCSTR lpString1, // string1 to merge with message
				 LPCSTR lpString2
				 )
{
	HANDLE hLog;
	ercType erc;
	const CHAR* rgsz[] = {lpString1,lpString2};
	WORD stringcount=0;
	
	if(lpString1) stringcount++;
	if(lpString2) stringcount++;
	
	// The RegisterEventSource function returns a handle that can be used 
	// with the ReportEvent function to log an event. 
	hLog = RegisterEventSource(0, pServiceName);
	if (!hLog)
		return ercInconsistency;

	if (!ReportEvent(hLog,		// handle returned by RegisterEventSource
					 mpSevToEventType[msgId >> 30], // event type to log
					 0, 							// event category
					 msgId,						// event identifier (AfdErr.dll)
					 NULL,							// user security identifier (optional)
					 stringcount,  // number of strings to merge with message
					 0, 						// size of binary data in bytes
					 rgsz,	// array of strings to merge with message
					 NULL						// address of binary data
		))
		erc = GetLastError();
	else
		erc = ercOk;

	DeregisterEventSource(hLog);
	return erc;
} // LogEvent2

// *************************************************************************
// Log any string to the event log.
// *************************************************************************
ercType LogNTString(WORD Severity, LPCSTR  lpString)
{
	HANDLE hLog;
	ercType erc;
	const CHAR* rgsz[] = {lpString};

	// The RegisterEventSource function returns a handle that can be used 
	// with the ReportEvent function to log an event. 
	hLog = RegisterEventSource(0, pServiceName);
	if (!hLog)
		return ercInconsistency;

	if(!Severity) Severity = EVENTLOG_INFORMATION_TYPE;

	if (!ReportEvent(hLog,		// handle returned by RegisterEventSource
					 Severity, // event type to log
					 1, 							// event category
					 NTLOG_INFO_GENERIC,						// event identifier (AfdErr.dll)
					 NULL,							// user security identifier (optional)
					 (WORD) (lpString ? 1 : 0),  // number of strings to merge with message
					 0, 						// size of binary data in bytes
					 rgsz,	// array of strings to merge with message
					 NULL						// address of binary data
		))
		erc = GetLastError();
	else
		erc = ercOk;

	DeregisterEventSource(hLog);
	return erc;
} // LogGeneric

// *************************************************************************
// Log a Win32 error to the event log.
// *************************************************************************
ercType LogNTWin32Error(DWORD dwErrorNum, LPCSTR szString)
{
	HANDLE hLog;
	ercType erc;
	LPVOID lpvMsgBuf;
	CHAR szErrorDesc[1024];
	CHAR* rgsz[1];   

	if(!FormatMessage(
	              FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER |
				  FORMAT_MESSAGE_IGNORE_INSERTS,
                  0, dwErrorNum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPSTR)&lpvMsgBuf, 0, 0))
		{
		if(szString != NULL)
		 wsprintf(szErrorDesc,
				  "Function: %s returned Win32 Error: %d, Unknown Error",
				  szString, dwErrorNum);
		else
		 wsprintf(szErrorDesc,
				  "Win32 Error: %d, Unknown Error",
				  dwErrorNum);
		}
	else
		{
		if(szString != NULL)
		 wsprintf(szErrorDesc,
				  "Function: %s returned Win32 Error: %d\r\nDescription: %s",
				  szString, dwErrorNum, lpvMsgBuf);
		else
		 wsprintf(szErrorDesc,
				  "Win32 Error: %d\r\nDescription: %s",
				  dwErrorNum, lpvMsgBuf);
		LocalFree(lpvMsgBuf);
		}

	rgsz[0] = szErrorDesc;
	
	// The RegisterEventSource function returns a handle that can be used 
	// with the ReportEvent function to log an event. 
	hLog = RegisterEventSource(0, pServiceName);
	if (!hLog)
		return ercInconsistency;

	
	if (!ReportEvent(hLog,					// handle returned by RegisterEventSource
					 EVENTLOG_ERROR_TYPE,	// event type to log
					 32, 					// event category
					 NTLOG_WIN32_ERROR ,		// event identifier (AfdErr.dll)
					 NULL,							// user security identifier (optional)
					 1,  // number of strings to merge with message
					 0, 						// size of binary data in bytes
					 rgsz,	// array of strings to merge with message
					 NULL						// address of binary data
		))
		erc = GetLastError();
	else
		erc = ercOk;

	DeregisterEventSource(hLog);
	return erc;
} // LogWin32Error


// *************************************************************************
// Log any string to the event log.
// *************************************************************************
ercType LogNTData(WORD Severity, LPCSTR  lpString, LPVOID lpRawData, DWORD dwDataSize)
{
	HANDLE hLog;
	ercType erc;
	const CHAR* rgsz[] = {lpString};
	

	// The RegisterEventSource function returns a handle that can be used 
	// with the ReportEvent function to log an event. 
	hLog = RegisterEventSource(0, pServiceName);
	if (!hLog)
		return ercInconsistency;

	if (!ReportEvent(hLog,		// handle returned by RegisterEventSource
					 Severity, // event type to log
					 0, 							// event category
					 NTLOG_INFO_GENERIC,						// event identifier (AfdErr.dll)
					 NULL,							// user security identifier (optional)
					 (WORD) (lpString ? 1 : 0),  // number of strings to merge with message
					 dwDataSize, 						// size of binary data in bytes
					 rgsz,	// array of strings to merge with message
					 lpRawData						// address of binary data
		))

		erc = GetLastError();
	else
		erc = ercOk;

	DeregisterEventSource(hLog);
	return erc;
} // LogGeneric

