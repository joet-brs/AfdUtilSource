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
'    File Name    :  QmService.cpp
'
'    Purpose      :  This file has the code for the interface between the 
'                    PipeServer and the QueueManager logical part.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
9/July AA	Added code for fucntion RemoveKeyedQueueEntry(). LIMS.

*/

/*///////////////////////////////////////////////////////////////////////////
Module Name: QMService.cpp

This module contains the main thread of the Queue Manager.
The procedure InitService is called to create the main thread.
QueueManager is the main thread.  

Revision History:

21/July/97 AA QueueManager for Momentum Systems AFD project.

///////////////////////////////////////////////////////////////////////////*/
#include "basic.h"
#include <stdio.h>
#include <string.h>
//#include "qmMsg.h"
#include "AfdErr.h"
#include "trace.h"
#include "build.h"
#include "Req.h"
#include "RqStruct.h"
#include "qmErcs.h"
#include <process.h>	// for _beginthreadex
#include <AfdMisc.h>
#include <AfdLog_Debug.h>
#include <AfdConfigProto.h>

static char *ModId[]={__FILE__};


void ServiceTerminate (DWORD dwError);

////////////////////////////////////////////////////////////////////////////////
// EXTERNAL PROCEDURES
extern ercType LogInitialize (void);
extern unsigned __stdcall PipeServer(LPVOID p);
extern DWORD RespondToPipe(DWORD userNum);
extern void ProcessTimer();

char pipeDesignation[]={"\\\\.\\pipe\\"};

#ifdef __cplusplus
extern "C" {
#endif
extern char *pServiceName;
#ifdef __cplusplus
}
#endif

extern LPTSTR g_lpszPipename;

#ifdef __cplusplus
extern "C" {
#endif

void APIENTRY TerminateService (void);
DWORD APIENTRY SuspendService (void);
DWORD APIENTRY ResumeService (void);
DWORD APIENTRY InitService(int argc, char **argv);

#ifdef __cplusplus
}
#endif


extern BOOLEAN fDebugMode;


//////////////////////////////////////////////////////////////////////////////
// GLOBALS

HANDLE hEventServiceSync; // sync object for initialization
extern HANDLE g_TerminateEvent;
ercType ercQMService=ercOk;
HANDLE hQMServiceThread;
char AfdRoot[50];
char HalbRoot[255];
char QueueIndexFile[]="\\Config\\Queue.index"; //comes off the HalbRoot
char QueueFilePrefix[]="\\Queues\\";		   //comes off the AFDRoot
char QueueFilePostfix[]=".Queue";
char regBasePath[50]; //for posterity

char LogFileSpec[255];
char DumpFileSpec[255];
DWORD	g_dwDebugLevel = 0;
CRITICAL_SECTION g_cs;
char SrvNameBuf[MAX_COMPUTERNAME_LENGTH + 1];

//external
extern BOOL WriteLogData(char *data, DWORD level = 0);

extern DWORD H_AddQueue(WORD wEntrySize, WORD wQueueType, 
						char *pQueueName, char *pFileSpec, DWORD *qeh);

extern DWORD H_AddQueueEntry(BOOL fQueueIfNoServer, WORD QPriority, WORD QType,
							 WORD RepeatTime, char *pbQueueName, 
							 BYTE *pbEntry, WORD cbEntry, 
							 pCtosDateTimeType pSysDateTime);

extern DWORD H_EstablishQueueServer(WORD QType, BOOL fUniqueServer,
									char *pQName, DWORD userNum);

extern DWORD H_MarkNextQueueEntry(BOOL fReturnIfNoEntries, char *pQueueName,
								  BYTE *pbEntryRet, WORD cbEntryRet,
								  BYTE *StatusBlock, WORD sStatusBlock,
								  DWORD userNum, BOOL *fRet,
								  RqMarkNextQueueEntry_t *pRq);

extern DWORD H_RemoveMarkedQueueEntry(char *pQueueName, DWORDLONG QEH, DWORD userNum);

extern DWORD H_UnmarkQueueEntry(char *pQueueName, DWORDLONG QEH, DWORD userNum);

extern DWORD H_TerminateQueueServer(char *pQueueName, DWORD userNum);

extern DWORD H_CleanQueue(DWORD qh);

extern DWORD H_RemoveQueue(DWORD qh);

extern DWORD H_ReadNextQueueEntry(DWORDLONG QEH, char *pQueueName, 
								  BYTE *pbEntryRet, WORD cbEntryRet,
								  BYTE *StatusBlock, WORD sStatusBlock);

extern DWORD H_GetQMStatus(WORD QueueType, BOOL fHealthCheck, 
						   BYTE *pStatusRet, WORD sStatusRet);

extern DWORD H_RewriteMarkedQueueEntry(DWORDLONG QEH, char *pQueueName, 
									   BYTE *pEntry, WORD cEntry,
									   DWORD userNum);

extern DWORD H_ReadKeyedQueueEntry(char *pQueueName, WORD oKey1, BYTE *pKey1, WORD cKey1,
								   WORD oKey2, BYTE *pKey2, WORD cKey2,
								   BYTE *pbEntryRet, WORD cbEntryRet,
								   BYTE *StatusBlock, WORD sStatusBlock
								  );

extern DWORD H_MarkKeyedQueueEntry(char *pQueueName,
								   WORD oKey1, BYTE *pKey1, WORD cKey1,
								   WORD oKey2, BYTE *pKey2, WORD cKey2,
								   BYTE *pbEntryRet, WORD cbEntryRet,
								   BYTE *StatusBlock, WORD sStatusBlock, DWORD userNum
								  );

extern DWORD H_RemoveKeyedQueueEntry(char *pQueueName, WORD oKey1, BYTE *pKey1, WORD cKey1,
									 WORD oKey2, BYTE *pKey2, WORD cKey2
									);


extern char *pbcbToStr(char *p, WORD c);


DWORD AddQueueHandlerStub(WORD wEntrySize, WORD wQueueType, 
						  char *pbQueueName, WORD cbQueueName,
						  char *pbFileSpec, WORD cbFileSpec, DWORD *qeh);

DWORD CTOSTimeFromSystem(SYSTEMTIME *pSysTime, pCtosDateTimeType pCtosDateTime);


BOOL GetQueueParams(char *pBuff, char *pQueueName, char *pFileSpec, 
					WORD *iSize, WORD *QType, WORD *sCharRet)
{
#define F_QUEUENAME 0
#define F_FILESPEC 1
#define F_ENTRYSIZE 2
#define F_QUEUETYPE 3
#define F_END 4

	char *pLineScan = pBuff;
	char *pFieldScan = pBuff;
	char tempBuf[80];
	WORD iField=0;
	WORD iFieldLen=0;
	BOOL fNotEOL=TRUE;

	if (*pLineScan == 0xa) {
		*sCharRet = 1;
		return(FALSE);
	}

	while (fNotEOL) {
		while (*pFieldScan != 0 && *pFieldScan != '/' && *pFieldScan != 0xa ) {
			iFieldLen++;
			pFieldScan++;
			(*sCharRet)++;
		}
		switch (iField) {
		case F_QUEUENAME:
			memcpy(pQueueName, pLineScan, iFieldLen);
			*(pQueueName+iFieldLen) = 0;
			break;
		case F_FILESPEC:
			memcpy(pFileSpec, pLineScan, iFieldLen);
			*(pFileSpec+iFieldLen) = 0;
			break;
		case F_ENTRYSIZE:
			memcpy(tempBuf, pLineScan, iFieldLen);
			tempBuf[iFieldLen] = 0;
			*iSize = (WORD) atoi(tempBuf);
			break;
		case F_QUEUETYPE:
			memcpy(tempBuf, pLineScan, iFieldLen);
			tempBuf[iFieldLen] = 0;
			*QType = (WORD) atoi(tempBuf);
			break;
		default:
			//junk at the end of the valid entries
			break;
		} //end switch

		if (*pFieldScan == 0 || *pFieldScan == 0xa) {
			(*sCharRet)++;
			fNotEOL = FALSE;
		} else {
			pFieldScan++;
			pLineScan=pFieldScan;
			iFieldLen = 0;
			iField++;
			(*sCharRet)++;
		} //while
		
	}

	if (iField < (F_END - 1))
		return(FALSE);
	return(TRUE);
}


BOOLEAN InitQueueManager() {

	DWORD erc;
	DWORD sFileSize, iFileScanPtr=0;
	DWORD lfa=0;
	WORD charRet;
	char *pBuff;
	HANDLE hFile=INVALID_HANDLE_VALUE, hFileMapping=0;
	char *pbFile=NULL;
	BOOL fNotEof=TRUE;

	char rgQueueDefFile[100];
	//DWORD cbRet, keyType;

	// Open the Queue.index file from disk
	BYTE strFile[]="C:\\Queues\\Queue.index";
	BYTE altStrFile[]="C:\\Queues\\Queue.ndx";
	
	memset(rgQueueDefFile, 0, sizeof(rgQueueDefFile));
	strcpy(rgQueueDefFile, HalbRoot);
	strcat(rgQueueDefFile, QueueIndexFile);

	hFile = CreateFile(rgQueueDefFile,
					   GENERIC_READ, FILE_SHARE_READ,	// share mode 
					   0, // pointer to security descriptor 
					   OPEN_EXISTING,	// how to create 
					   FILE_ATTRIBUTE_NORMAL,	// file attributes 
					   0
					  );
	if (hFile == INVALID_HANDLE_VALUE) { //try the hard coded strings
		DebugPrintMsg(MSG_CANNOT_READ_STD_QUEUE_INDEX);
#ifdef _DEBUG
		printf("File %s, Handle = %d, Erc = %d", rgQueueDefFile, hFile, GetLastError());
#endif
		hFile = CreateFile((const char *) strFile,
						   GENERIC_READ | GENERIC_WRITE, 0,	// share mode 
						   0, // pointer to security descriptor 
						   OPEN_EXISTING,	// how to create 
						   FILE_ATTRIBUTE_NORMAL,	// file attributes 
						   0);

		if (hFile == INVALID_HANDLE_VALUE) {
			printf("File %s, Handle = %d, Erc = %lu", strFile, hFile, GetLastError());

			hFile = CreateFile((const char *) altStrFile,
							   GENERIC_READ | GENERIC_WRITE, 0,	// share mode 
							   0, // pointer to security descriptor 
							   OPEN_EXISTING,	// how to create 
							   FILE_ATTRIBUTE_NORMAL,	// file attributes 
							   0
							  );
		}
	}

	if (hFile == INVALID_HANDLE_VALUE) {
		erc = GetLastError();
		DebugPrintMsg(MSG_QUEUE_INDEX);		//log and error
		goto AllDone;
	}

	sFileSize = GetFileSize(hFile, 0);
	if (sFileSize == 0xffffffff) {
		erc = GetLastError();
		DebugPrintMsg(MSG_QUEUE_INDEX);		//log and error
		goto AllDone;
	}

	if (sFileSize == 0)
		goto AllDone;

	hFileMapping = CreateFileMapping(	hFile,				// handle to file to map 
										NULL,				// optional security attributes 
										PAGE_WRITECOPY,		// protection for mapping object 
										0,					// high-order 32 bits of object size  
										0,					// low-order 32 bits of object size  
										0					// name of file-mapping object 
									 );

	if (!hFileMapping) {
		erc = GetLastError();
		DebugPrintMsg(MSG_QUEUE_INDEX);		//log and error
		goto AllDone;
	}

	pbFile = (char *) MapViewOfFile(hFileMapping,	// file-mapping object to map into address space  
									FILE_MAP_COPY,	// access mode 
									0,				// high-order 32 bits of file offset 
									0,				// low-order 32 bits of file offset 
									0				// number of bytes to map 
								   );

	if (!pbFile) {
		erc = GetLastError();
		DebugPrintMsg(MSG_QUEUE_INDEX);		//log and error
		goto AllDone;
	}

	pBuff = pbFile;


	BOOL fSuccess;


	while (fNotEof) {
		char QueueName[80];
		char FileSpec[80];
		WORD cEntry, QueueType;
		WORD iValid=0, iInvalid=0;
		DWORD qeh;

		QueueName[0] = FileSpec[0] = 0;
		cEntry = QueueType = charRet = 0;

		fSuccess = GetQueueParams(pBuff, QueueName, FileSpec, 
								  &cEntry, &QueueType, &charRet);
		if (fSuccess) {

			iValid++;
			erc = AddQueueHandlerStub(cEntry, QueueType, 
									  QueueName, strlen(QueueName),
									  FileSpec, strlen(FileSpec),
									  &qeh);
			if (erc) {
				//log an error TO_DO
			}
		} else {

			iInvalid++;
			//make a log entry indicating error on line number (iValid+iInvalid) TO_DO
		}
		iFileScanPtr += charRet;

		if (iFileScanPtr >= sFileSize)
			break;
		else
			pBuff+=charRet;
	}

AllDone:

	if (pbFile)
		UnmapViewOfFile(pbFile);
	if (hFileMapping)
		CloseHandle(hFileMapping);
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	return(TRUE);
}



//////////////////////////////////////////////////////////////////////////////
// Called by service shell to create the QMService thread.
DWORD APIENTRY InitService(int argc, char **argv) {
	unsigned int tid;
	DWORD ercWait;
	ercType erc;
	DWORD cbRet,KeyType;
	char  LogFileName[255];
	char  DumpFileName[255];
	LONG  val;
	char msgBuffer[1024];

	// create event to be signaled by QueueManagerService thread 
	// when it's done initializing.
	hEventServiceSync = CreateEvent(0, TRUE, FALSE, NULL);
	if (hEventServiceSync == NULL) {
		erc = GetLastError();

		DebugPrintStatus(MSG_CREATE_EVENT, erc);
		return erc;
	}

	cbRet = sizeof(SrvNameBuf);
	if(!GetComputerName(SrvNameBuf,&cbRet))
	{
		printf("Unable to get GetComputerName %d", GetLastError());
		strcpy(SrvNameBuf, "Unknown");
	}
				
	// Initialize 
	erc = AfdTpGetRegBasePath(regBasePath, sizeof(regBasePath), &cbRet);
	if (erc) {
		memset(regBasePath, 0, sizeof(regBasePath));
		//do not handle this error, it is benign
	}

	if (!GetAfdRoot(AfdRoot, sizeof(AfdRoot), &cbRet)) {
		LogNTEvent(MSG_NO_AFD_ROOT, NULL);
		SetLastError(erc = ercInconsistency);
		return(erc);
	}

	if (!GetHalbRoot(HalbRoot, sizeof(HalbRoot), &cbRet)) {
		LogNTEvent(MSG_NO_AFD_ROOT, NULL);
		SetLastError(erc = ercInconsistency);
		return(erc);
	}

	// Form the Log File name
    strcpy(LogFileName,"");
    if (sizeof(LogFileName) <= _snprintf(LogFileName, sizeof(LogFileName), "%s\\Logs\\%s.%s.log", HalbRoot, SrvNameBuf, pServiceName))
	{
		printf("Error creating log file name");
		return(FALSE);
	}
    strcpy(LogFileSpec, LogFileName);
    strcpy(DumpFileName,"");
    if (sizeof(DumpFileName) <= _snprintf(DumpFileName, sizeof(DumpFileName), "%s\\Logs\\%s.%s.dmp", HalbRoot, SrvNameBuf, pServiceName))
	{
		printf("Error creating dump file name");
		return(FALSE);
	}
    strcpy(DumpFileSpec, DumpFileName);

	// Initialize the critical section before we use it...
	InitializeCriticalSection(&g_cs);

	sprintf(msgBuffer,"***** AfdQueueManager started *****\r\n\tBuild  %s %s     (Source %s)"
			,__DATE__,__TIME__,__TIMESTAMP__);
	WriteLogData(msgBuffer);

	if(fDebugMode) 
		WriteLogData("Running in DEBUG mode.");
	else
		WriteLogData("Running in SERVICE mode.");

	val = AfdTpQueryRegValue("LogLevel", &KeyType, (BYTE *)&g_dwDebugLevel, sizeof(DWORD), &cbRet);
	if(val != ERROR_SUCCESS)
	{
		WriteLogData("Error reading LogLevel from registry.");
		WriteLogData("LogLevel set to 0");
		g_dwDebugLevel = 0;
	}

	sprintf(msgBuffer,"LogLevel read from the registry = %lu.", g_dwDebugLevel);
	WriteLogData(msgBuffer);

	g_lpszPipename = (char *) zmalloc(strlen(pipeDesignation) + strlen(pServiceName) + 2);
	strcpy(g_lpszPipename, pipeDesignation);
	strcat(g_lpszPipename, pServiceName);

	char *pipeRead;

	pipeRead = (char *) zmalloc(XFER_BLOCK_SIZE);

	memcpy(pipeRead, ("LIMS_ISNW_AA"), 12);

	// The CallNamedPipe function connects to a message-type pipe 
	// (and waits if an instance of the pipe is not available), writes to and reads from the pipe,
	// and then closes the pipe. 
	BOOL
	fSuccess = CallNamedPipe(g_lpszPipename,			// pointer to pipe name 
							 (void *) pipeRead,		// pointer to write buffer 
							 XFER_BLOCK_SIZE,		// size, in bytes, of write buffer 
							 (void *) pipeRead,		// pointer to read buffer 
							 XFER_BLOCK_SIZE,		// size, in bytes, of read buffer 
							 &cbRet,				// pointer to number of bytes read 
							 NMPWAIT_NOWAIT		 	// time-out time, in milliseconds 
							);

	free(pipeRead);
	pipeRead = NULL;

	if (fSuccess) {
		DebugPrintMsg(MSG_QM_ALREADY_INSTALLED);
		return(ercDuplicateServer);
	}

	if(!InitQueueManager()) {

		// Log event that no tape could be found.
		//DebugPrintMsg(MSG_ERRQMINIT);
		DebugPrintMsg(MSG_INIT_SERVICE_FAILED);

		// signal that the service is initialized, and an error has occurred.
		ercQMService = MSG_INIT_SERVICE_FAILED; 
		return (ercQMService);
	}


	// create the QMServiceThread thread
	hQMServiceThread = (HANDLE) _beginthreadex
									( NULL, 0,
									PipeServer,
									0,
									0,
									&tid);
	
	
/**	hQMServiceThread = CreateThread( NULL, 0,
									(LPTHREAD_START_ROUTINE)PipeServer,
									0,
									0,
									&tid); **/
	if (hQMServiceThread == 0){
		erc = GetLastError();
		DebugPrintStatus(MSG_CREATE_THREAD, erc);
		return erc;
	}
	

	// wait for PipeServer thread to finish initializing.
	ercWait = WaitForSingleObject(hEventServiceSync, 
		fDebugMode ? INFINITE: 5000 /*5 sec*/);
	CloseHandle(hEventServiceSync);
	if (ercWait == WAIT_FAILED){
		erc = GetLastError();
		DebugPrintStatus(MSG_CLOSE_HANDLE, erc);
		return erc;
	}
	if (ercWait != WAIT_OBJECT_0){
		DebugPrintStatus(MSG_WAIT_OBJECT, ercWait);
		return ercInconsistency;
	}

	return ercQMService;
} // InitService

//////////////////////////////////////////////////////////////////////////////
// Called by service shell to terminate the QueueManagerService.  
// We must unserve all the requests.
void APIENTRY TerminateService (void) {

	WriteLogData("Deinstall the QueueManager service.");
extern void DumpMemory();
DumpMemory();
	// Notify the other threads that we want to terminate
	if(g_TerminateEvent) {
		SetEvent(g_TerminateEvent);
	
	// Wait for threads to terminate
		if(hQMServiceThread) {
			WaitForSingleObject(hQMServiceThread, INFINITE);
			CloseHandle(hQMServiceThread);
			}
	}

	DeleteCriticalSection(&g_cs);

} // TerminateService

DWORD APIENTRY SuspendService (void) {

	return(0);

} // TerminateService

DWORD APIENTRY ResumeService (void) {

	return(0);

} // TerminateService

 /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
| The Request Block comes on the pipe as a "flattened" structure. |
| The pbcb pointers are adjusted such that all the pb(s) are      |
| relative to the start of the flattened structure. The Fixup()   |
| routine converts these relative addresses to absolute addresses.|
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void FixupPointers(RqHdr_t *pRq)
{
	pbcb_t *pPbCb;
	WORD i;
	DWORD off;

	pPbCb = (pbcb_t *) ((BYTE *) pRq + (sizeof(RqHdr_t) + pRq->sCntlInfo));

	for(i=0; (i < (pRq->nReq + pRq->nResp) ); i++) {
		off = (DWORD ) pPbCb->p;
		if (off)
			pPbCb->p = (void *) ((BYTE *) pRq + off);
		pPbCb++;
	}

}

void UnfixPointers(RqHdr_t *pRq)
{
	pbcb_t *pPbCb;
	WORD i;
	DWORD baseAddr;

	pPbCb = (pbcb_t *) ((BYTE *) pRq + (sizeof(RqHdr_t) + pRq->sCntlInfo));
	baseAddr = (DWORD) (pRq);

	for(i=0; (i < (pRq->nReq + pRq->nResp) ); i++) {
		if (pPbCb->p)
			pPbCb->p = (void *) ((BYTE *) pPbCb->p - baseAddr);
		pPbCb++;
	}

}

char *ConstructFileName(char *pQueueName)
{
	char *pFileName;

	pFileName = (char *) zmalloc(strlen(AfdRoot) +
								 strlen((const char *) QueueFilePrefix) + 
								 strlen((const char *) pQueueName) + 
								 strlen((const char *) QueueFilePostfix) + 1);

	strcpy((char *)pFileName, (char *)AfdRoot);
	strcat((char *)pFileName, (char *)QueueFilePrefix);
	strcat((char *)pFileName, (char *)pQueueName);
	strcat((char *)pFileName, (char *)QueueFilePostfix);
	return(pFileName);
}


BOOL AddQueueEntryHandler(RqAddQueueEntry_t *pRq)
{
	char *pQueueName=0;
	ctosDateTimeType dateTime;
	SYSTEMTIME currentTime;
	char msgBuffer[1024];

	if (pRq->pbQueueName == NULL || pRq->cbQueueName == 0)
		return(ercBadQueSpec);

	
	if(pRq->pSysDateTime == NULL)
		{
		// Use current server time
		GetLocalTime(&currentTime);
		pRq->RqHdr.ercRet = CTOSTimeFromSystem(&currentTime, &dateTime);
		}
	else	
		{
		pRq->RqHdr.ercRet = CTOSTimeFromSystem(pRq->pSysDateTime, &dateTime);
		}

	if (pRq->RqHdr.ercRet) {
		pRq->RqHdr.ercRet = ercBadTimeQm;
		return (TRUE);
	}


	pQueueName = pbcbToStr((char *)pRq->pbQueueName, pRq->cbQueueName);

	sprintf(msgBuffer, "Adding QueueEntry to queue %s, for user %X",pQueueName, pRq->RqHdr.userNum);
	WriteLogData(msgBuffer, 2);

	pRq->RqHdr.ercRet = H_AddQueueEntry(pRq->fQueueIfNoServer, 
										pRq->QPriority, 
										pRq->QType,
										pRq->RepeatTime, 
										pQueueName,
										pRq->pbEntry, pRq->cbEntry,
										&dateTime);
	if (pQueueName) {
		free(pQueueName);
		pQueueName = NULL;
		}

	return (TRUE);
}


DWORD AddQueueHandlerStub(WORD wEntrySize, WORD wQueueType, 
						  char *pbQueueName, WORD cbQueueName,
						  char *pbFileSpec, WORD cbFileSpec, DWORD *qeh)
{
	char *pQueueName=0;
	char *pFileName=0;
	DWORD erc;
	char msgBuffer[1024];

	// Constuct the disk based queue name

	if ( (!pbQueueName) || (cbQueueName > 50) || (cbQueueName == 0) )
		return(ercBadQueSpec);
	pQueueName = pbcbToStr(pbQueueName, cbQueueName);

	if (!pbFileSpec || cbFileSpec == 0)
		pFileName = ConstructFileName(pQueueName);
	else
		pFileName = pbcbToStr(pbFileSpec, cbFileSpec);

	sprintf(msgBuffer, "Adding Queue %s - %s",pQueueName, pFileName);
	WriteLogData(msgBuffer, 2);

	// Create the .queue file if necessary and 
	erc = H_AddQueue(wEntrySize, wQueueType, pQueueName, pFileName, qeh);

	if (pFileName) {
		free(pFileName);
		pFileName = NULL;
		}
	if (pQueueName) {
		free(pQueueName);
		pQueueName = NULL;
		}
	return (erc);
}

BOOL AddQueueHandler(RqAddQueue_t *pRq)
{

	pRq->RqHdr.ercRet = AddQueueHandlerStub(pRq->wEntrySize, pRq->wQueueType, 
											 (char *) pRq->pbQueueName, pRq->cbQueueName,
											 (char *) pRq->pbFileSpec, pRq->cbFileSpec, 
											 pRq->qeh);

	return(TRUE);
}

BOOL MarkNextQueueEntryHandler(RqMarkNextQueueEntry_t *pRq)
{

	char *pQueueName=0;
	BOOL fRet = TRUE;

	if (pRq->pbQueueName == NULL || pRq->cbQueueName == 0)
		return(ercBadQueSpec);

	pQueueName = pbcbToStr((char *)pRq->pbQueueName, pRq->cbQueueName);

	//once a queue is marked, even if QM is deinsatlled and installed again it 
	//remains marked???? TO_DO

	pRq->RqHdr.ercRet = H_MarkNextQueueEntry(pRq->fReturnIfNoEntries,
											 pQueueName, 
											 pRq->pbEntryRet, pRq->cbEntryRet,
											 pRq->StatusBlock, pRq->sStatusBlock,
											 pRq->RqHdr.userNum, &fRet, pRq
											);
	if (pQueueName) {
		free(pQueueName);
		pQueueName = NULL;
		}
	return (fRet);

}

BOOL EstablishQueueServerHandler(RqEstablishQueueServer_t * pRq)
{
	char *pQueueName=0;
	char msgBuffer[1024];

	if (pRq->pbQueueName == NULL || pRq->cbQueueName == 0)
		return(ercBadQueSpec);

	pQueueName = pbcbToStr((char *) pRq->pbQueueName, pRq->cbQueueName);

	sprintf(msgBuffer, "Establishing Queue Server on queue %s, for user %X",pQueueName, pRq->RqHdr.userNum);
	WriteLogData(msgBuffer, 1);

	pRq->RqHdr.ercRet = H_EstablishQueueServer(pRq->QueueType, pRq->fUniqueServer, 
											   pQueueName, pRq->RqHdr.userNum);
	if (pQueueName)
		{
		free(pQueueName);
		pQueueName = NULL;
		}
	return (TRUE);
}

BOOL RemoveMarkedQueueEntryHandler(RqRemoveMarkedQueueEntry_t *pRq)
{
	char *pQueueName=0;
	char msgBuffer[1024];

	if (pRq->pbQueueName == NULL || pRq->cbQueueName == 0)
		return(ercBadQueSpec);

	pQueueName = pbcbToStr((char *) pRq->pbQueueName, pRq->cbQueueName);

	sprintf(msgBuffer, "Removing Marked QueueEntry from queue %s, for user %X",pQueueName, pRq->RqHdr.userNum);
	WriteLogData(msgBuffer, 2);

	pRq->RqHdr.ercRet = H_RemoveMarkedQueueEntry(pQueueName, pRq->QEH, 
												 pRq->RqHdr.userNum);

	if (pQueueName)
		{
		free(pQueueName);
		pQueueName = NULL;
		}
	return (TRUE);
}

BOOL UnmarkQueueEntryHandler(RqUnmarkQueueEntry_t *pRq)
{
	char *pQueueName=0;
	char msgBuffer[1024];

	if (pRq->pbQueueName == NULL || pRq->cbQueueName == 0)
		return(ercBadQueSpec);
	pQueueName = pbcbToStr((char *) pRq->pbQueueName, pRq->cbQueueName);

	sprintf(msgBuffer, "Unmarking QueueEntry on queue %s, for user %X",pQueueName, pRq->RqHdr.userNum);
	WriteLogData(msgBuffer, 2);

	pRq->RqHdr.ercRet = H_UnmarkQueueEntry(pQueueName, pRq->QEH, pRq->RqHdr.userNum);

	if (pQueueName)
		{
		free(pQueueName);
		pQueueName = NULL;
		}
	return (TRUE);
}


BOOL TerminateQueueServerHandler(RqTerminateQueueServer_t *pRq)
{
	char *pQueueName=0;
	char msgBuffer[1024];

	if (pRq->pbQueueName == NULL || pRq->cbQueueName == 0)
		return(ercBadQueSpec);
	pQueueName = pbcbToStr((char *) pRq->pbQueueName, pRq->cbQueueName);

	sprintf(msgBuffer, "Terminating Queue Server on queue %s, for user %X",pQueueName, pRq->RqHdr.userNum);
	WriteLogData(msgBuffer, 1);

	pRq->RqHdr.ercRet = H_TerminateQueueServer(pQueueName, pRq->RqHdr.userNum);

	if (pQueueName)
		{
		free(pQueueName);
		pQueueName = NULL;
		}
	return (TRUE);
}

BOOL CleanQueueHandler(RqCleanQueue_t *pRq)
{
	pRq->RqHdr.ercRet = H_CleanQueue(pRq->qh);
	return(TRUE);
}

BOOL RemoveQueueHandler(RqRemoveQueue_t *pRq)
{
	pRq->RqHdr.ercRet = H_RemoveQueue(pRq->qh);
	return(TRUE);
}

BOOL ReadNextQueueEntryHandler(RqReadNextQueueEntry_t *pRq)
{
	char *pQueueName=0;
	char msgBuffer[1024];

	if (pRq->pbQueueName == NULL || pRq->cbQueueName == 0)
		return(ercBadQueSpec);
	pQueueName = pbcbToStr((char *)pRq->pbQueueName, pRq->cbQueueName);

	sprintf(msgBuffer, "Reading next QueueEntry on queue %s, for user %X",pQueueName, pRq->RqHdr.userNum);
	WriteLogData(msgBuffer, 2);

	pRq->RqHdr.ercRet = H_ReadNextQueueEntry(pRq->QEH,
											 pQueueName, 
											 pRq->pbEntryRet, pRq->cbEntryRet,
											 pRq->StatusBlock, pRq->sStatusBlock
											);
	if (pQueueName) {
		free(pQueueName);
		pQueueName = NULL;
		}

	return (TRUE);
}


BOOL GetQMStatusHandler(RqGetQMStatus_t *pRq)
{
	pRq->RqHdr.ercRet = H_GetQMStatus(pRq->QueueType, pRq->fHealthCheck, 
									  pRq->pStatusRet, pRq->sStatusRet);
	return (TRUE);
}

BOOL RewriteMarkedQueueEntryHandler(RqRewriteMarkedQueueEntry_t *pRq)
{
	char *pQueueName=0;
	char msgBuffer[1024];

	if (pRq->pbQueueName == NULL || pRq->cbQueueName == 0)
		return(ercBadQueSpec);
	pQueueName = pbcbToStr((char *)pRq->pbQueueName, pRq->cbQueueName);

	sprintf(msgBuffer, "Rewriting marked QueueEntry to queue %s, for user %X",pQueueName, pRq->RqHdr.userNum);
	WriteLogData(msgBuffer, 2);

	pRq->RqHdr.ercRet = H_RewriteMarkedQueueEntry(pRq->QEH,
												  pQueueName, 
												  pRq->pbEntry, pRq->cbEntry,
												  pRq->RqHdr.userNum);
	
	if (pQueueName) {
		free(pQueueName);
		pQueueName = NULL;
		}
	return (TRUE);
}

BOOL ReadKeyedQueueEntryHandler(RqReadKeyedQueueEntry_t *pRq)
{
	char *pQueueName=0;
	char msgBuffer[1024];

	if (pRq->pbQueueName == NULL || pRq->cbQueueName == 0)
		return(ercBadQueSpec);
	pQueueName = pbcbToStr((char *)pRq->pbQueueName, pRq->cbQueueName);

	sprintf(msgBuffer, "Reading QueueEntry from queue %s, for user %X",pQueueName, pRq->RqHdr.userNum);
	WriteLogData(msgBuffer, 2);

	pRq->RqHdr.ercRet = H_ReadKeyedQueueEntry(pQueueName,
											  pRq->oKey1, pRq->pKey1, pRq->cKey1,
											  pRq->oKey2, pRq->pKey2, pRq->cKey2,
											  pRq->pbEntryRet, pRq->cbEntryRet,
											  pRq->StatusBlock, pRq->sStatusBlock
											 );

	if (pQueueName) {
		free(pQueueName);
		pQueueName = NULL;
		}

	return (TRUE);
}

BOOL MarkKeyedQueueEntryHandler(RqMarkKeyedQueueEntry_t *pRq)
{
	char *pQueueName=0;
	char msgBuffer[1024];

	if (pRq->pbQueueName == NULL || pRq->cbQueueName == 0)
		return(ercBadQueSpec);

	// pbcbToStr allocates memory and returns a pointer to that memory
	pQueueName = pbcbToStr((char *)pRq->pbQueueName, pRq->cbQueueName);

	//once a queue is marked, even if QM is deinsatlled and installed again it 
	//remains marked???? TO_DO

	sprintf(msgBuffer, "Marking keyed QueueEntry on queue %s, for user %X",pQueueName, pRq->RqHdr.userNum);
	WriteLogData(msgBuffer, 2);

	pRq->RqHdr.ercRet = H_MarkKeyedQueueEntry(pQueueName,
											  pRq->oKey1, pRq->pKey1, pRq->cKey1,
											  pRq->oKey2, pRq->pKey2, pRq->cKey2,
											  pRq->pbEntryRet, pRq->cbEntryRet,
											  pRq->StatusBlock, pRq->sStatusBlock,
											  pRq->RqHdr.userNum
											 );
	if (pQueueName) {
		free(pQueueName);
		pQueueName = NULL;
		}
	return (TRUE);
}

BOOL RemoveKeyedQueueEntryHandler(RqRemoveKeyedQueueEntry_t *pRq)
{
	char *pQueueName=0;
	char msgBuffer[1024];

	if (pRq->pbQueueName == NULL || pRq->cbQueueName == 0)
		return(ercBadQueSpec);

	pQueueName = pbcbToStr((char *)pRq->pbQueueName, pRq->cbQueueName);


	sprintf(msgBuffer, "Removing keyed QueueEntry from queue %s, for user %X",pQueueName, pRq->RqHdr.userNum);
	WriteLogData(msgBuffer, 2);

	pRq->RqHdr.ercRet = H_RemoveKeyedQueueEntry(pQueueName,
												pRq->oKey1, pRq->pKey1, pRq->cKey1,
												pRq->oKey2, pRq->pKey2, pRq->cKey2);
	if (pQueueName) {
		free(pQueueName);
		pQueueName = NULL;
		}

	return (TRUE);
}

BOOL ServiceRequest(RqHdr_t *pRq)
{
	BOOL fRet=TRUE;  //after all functions are implemented make this to false

	FixupPointers(pRq);

	switch (pRq->rqCode) {

	case RQ_AddQueueRequest:
		fRet = AddQueueHandler((RqAddQueue_t *) pRq);
		break;

	case RQ_AddQueueEntryRequest:
		fRet = AddQueueEntryHandler((RqAddQueueEntry_t *) pRq);
		break;

	case RQ_EstablishQueueServer:
		fRet = EstablishQueueServerHandler((RqEstablishQueueServer_t *) pRq);
		break;

	case RQ_MarkNextQueueEntry:
		fRet = MarkNextQueueEntryHandler((RqMarkNextQueueEntry_t *) pRq);
		break;

	case RQ_RemoveMarkedQueueEntry:
		fRet = RemoveMarkedQueueEntryHandler((RqRemoveMarkedQueueEntry_t *) pRq);
		break;

	case RQ_TerminateQueueServer:
		fRet = TerminateQueueServerHandler((RqTerminateQueueServer_t *) pRq);
		break;

	case RQ_UnmarkQueueEntry:
		fRet = UnmarkQueueEntryHandler((RqUnmarkQueueEntry_t *) pRq);
		break;

	case RQ_CleanQueue:
		fRet = CleanQueueHandler((RqCleanQueue_t *) pRq);
		break;

	case RQ_RemoveQueue:
		fRet = RemoveQueueHandler((RqRemoveQueue_t *) pRq);
		break;

	case RQ_ReadNextQueueEntry:
		fRet = ReadNextQueueEntryHandler((RqReadNextQueueEntry_t *) pRq);
		break;

	case RQ_GetQMStatus:
		fRet = GetQMStatusHandler((RqGetQMStatus_t *) pRq);
		break;

	case RQ_RewriteMarkedQueueEntry:
		fRet = RewriteMarkedQueueEntryHandler((RqRewriteMarkedQueueEntry_t *) pRq);
		break;

	case RQ_ReadKeyedQueueEntry:
		fRet = ReadKeyedQueueEntryHandler((RqReadKeyedQueueEntry_t *) pRq);
		break;

	case RQ_MarkKeyedQueueEntry:
		fRet = MarkKeyedQueueEntryHandler((RqMarkKeyedQueueEntry_t *) pRq);
		break;

	case RQ_RemoveKeyedQueueEntry:
		fRet = RemoveKeyedQueueEntryHandler((RqRemoveKeyedQueueEntry_t *) pRq);
		break;

	default:
		//why
		DebugBreak();
		break;
	}

	if (fRet) {
		UnfixPointers(pRq);
	}

	return (fRet);
}


DWORD Respond(RqHdr_t *pRq)
{
	UnfixPointers(pRq);
	return(RespondToPipe(pRq->userNum));
}

