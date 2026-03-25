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
'    Module       :  AFD QueueManager Service
'    Date         :  March 1998
'
'    File Name    :  Trace.cpp
'
'    Purpose      :  This file has the code for Tracing functions.
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

#include "basic.h"
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <ostream>
#include "build.h"
#include "AfdMisc.h"
#include "AfdLog_Debug.h"

static char *ModId[]={__FILE__};


#ifdef DEBUG

#define cLinesLog 100  // number of recent requests to save in log
#define cbLogEntry 66 // number of characters displayed by ASCII Memory
#define cbRqName 11	 // size of rqName
#define cRqName 11 // number of rqNames
typedef CHAR LogEntry[cbLogEntry];
typedef LogEntry LogBuffer[cLinesLog];
LogEntry* pLogBuffer;
LogEntry logEntry;
#define logErc &logEntry[cbRqName] // erc follows rqName
#define cbErcEntry 7 // 5 characters for erc plus space on each side
#define logVariable &logEntry[cbRqName+cbErcEntry]

/////////////////////////////////////////////////////////////////////////////
void InitTrace()
{
	pLogBuffer = (LogEntry*)zmalloc(sizeof(LogBuffer));
	memset(pLogBuffer, ' ', sizeof(LogBuffer));
} // InitTrace

void TraceRq(unsigned int msg) 
{
	// Set next logEntry to spaces.
	memset(logEntry, ' ', sizeof(LogEntry));

	// Put erc in logEntry.
	sprintf(logErc, " %.5d ", msg /*TO_DO*/);

	// Put name of request and request-specific data in logEntry.
	// LSL 990315 Remove dead code + this causes a compile warning
//	switch (msg) {
//
//	default: ;
//	} // switch
	// LSL 990315 end
	// Move each entry in the log down one notch.  Use memmove because it
	// accounts for overlapping source and dest.
	memmove(pLogBuffer[1], pLogBuffer[0], (cLinesLog-1)*cbLogEntry);

	// Move the new entry to the first entry in the LogBuffer.
	memcpy(pLogBuffer[0], &logEntry, cbLogEntry);

} // TraceRq
#endif // DEBUG
extern DWORD g_dwDebugLevel;
extern CRITICAL_SECTION g_cs;
extern char LogFileSpec[255];
extern char DumpFileSpec[255];

extern PipeInst_t *g_PipeMsgQueue;
extern PipeInst_t *g_PipeInstQueue;

void DumpMemory()
{
	HANDLE hLogFile;
	DWORD dwBytesWritten,dwFileSize;
	BOOL result;
	PipeInst_t *pInst;
//	QueueHeader_t * pQ;
	EnterCriticalSection(&g_cs);
	hLogFile = CreateFile(DumpFileSpec,
					GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,	// share mode 
					0, // pointer to security descriptor 
					CREATE_ALWAYS, // creation mode
					FILE_ATTRIBUTE_NORMAL,	// file attributes 
					0);
	if (INVALID_HANDLE_VALUE == hLogFile)
		{LeaveCriticalSection(&g_cs); return;}
	dwFileSize = GetFileSize(hLogFile, 0);
	SetFilePointer(hLogFile, dwFileSize, NULL, FILE_BEGIN);

	pInst = g_PipeMsgQueue;
	while (NULL != pInst)
	{
		result = WriteFile(hLogFile,pInst,sizeof(*pInst),&dwBytesWritten,NULL);
		if(FALSE == result) {
			CloseHandle(hLogFile);
			LeaveCriticalSection(&g_cs);
			return;
			}
		pInst = pInst->Next;
	}

	pInst = g_PipeInstQueue;
	while (NULL != pInst)
	{
		result = WriteFile(hLogFile,pInst,sizeof(*pInst),&dwBytesWritten,NULL);
		if(FALSE == result) {
			CloseHandle(hLogFile);
			LeaveCriticalSection(&g_cs);
			return;
			}
		pInst = pInst->Next;
	}

//	pQ = g_pQueues;
//	while (NULL != pQ)
//	{
//		result = WriteFile(hLogFile,pQ,sizeof(*pQ),&dwBytesWritten,NULL);
//		if(FALSE == result) {
//			CloseHandle(hLogFile);
//			LeaveCriticalSection(&g_cs);
//			return;
//			}
//		pQ = pQ->Next;
//	}

	CloseHandle(hLogFile);
	LeaveCriticalSection(&g_cs);
	return;
}

BOOL WriteLogData(char *data, DWORD level = 0) {
	HANDLE hLogFile;
	char datebuf[20];
	char timebuf[20];
	char buffer[512];
	DWORD dwBytesWritten,dwFileSize;
	BOOL result;
	
	if(level>g_dwDebugLevel) return FALSE;
	
	EnterCriticalSection(&g_cs);
	hLogFile = CreateFile(LogFileSpec,
					GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,	// share mode 
					0, // pointer to security descriptor 
					OPEN_ALWAYS, // creation mode
					FILE_ATTRIBUTE_NORMAL,	// file attributes 
					0);
	if (INVALID_HANDLE_VALUE == hLogFile)
		{LeaveCriticalSection(&g_cs); return FALSE;}
	dwFileSize = GetFileSize(hLogFile, 0);
	SetFilePointer(hLogFile, dwFileSize, NULL, FILE_BEGIN);

	_strdate(datebuf); 
	_strtime(timebuf);
	sprintf(buffer,"%s %s ",datebuf,timebuf);
	if(fDebugMode) std::cout << buffer;
	result = WriteFile(hLogFile,buffer,strlen(buffer),&dwBytesWritten,NULL);
	if(FALSE == result) {
		CloseHandle(hLogFile);
		LeaveCriticalSection(&g_cs);
		return FALSE;
		}
	
	if(data) {
		if(fDebugMode)
			for (int i=0; i< (int)strlen(data); i++)
				std::cout << data[i];
		result = WriteFile(hLogFile,data,strlen(data),&dwBytesWritten,NULL);
		if(FALSE == result) {
			CloseHandle(hLogFile);
			LeaveCriticalSection(&g_cs);
			return TRUE;
			}
		}
	
	if(fDebugMode) std::cout << std::endl;
	result = WriteFile(hLogFile,"\r\n",2,&dwBytesWritten,NULL);
	if(FALSE == result) {
		CloseHandle(hLogFile);
		LeaveCriticalSection(&g_cs);
		return FALSE;
		}
	
	
	CloseHandle(hLogFile);
	LeaveCriticalSection(&g_cs);
	return TRUE;
}

#ifdef __cplusplus
extern "C" {
#endif
void DEBUG_BREAK(char *ModId, int lineNo, DWORD erc) {
	static char DebugInfo[255];

	sprintf(DebugInfo, "ThreadId 0x%08X abended.\r\nModule: %s\r\nLine number: %d\r\nLast erc = 0x%08x (Unexpected)",
			GetCurrentThreadId(), ModId, lineNo, erc);

//	if (fDebugMode) {
#ifdef _DEBUG
		printf("\n%s",DebugInfo);
		printf("\npress <Enter> to continue.");
		scanf("%c", DebugInfo);
#endif
	LogNTString(EVENTLOG_ERROR_TYPE, DebugInfo);

	WriteLogData(DebugInfo);

	DumpMemory();

	DebugBreak();	
}
void DEBUG_LOG(char *ModId, int lineNo, DWORD erc) {
	static char DebugInfo[255];

	sprintf(DebugInfo, "ThreadId 0x%08X (Normal?) error.\r\nModule: %s\r\nLine number: %d\r\nLast erc = 0x%08x (Normal?)",
			GetCurrentThreadId(), ModId, lineNo, erc);
	WriteLogData(DebugInfo, 2);
}
#ifdef __cplusplus
}
#endif

