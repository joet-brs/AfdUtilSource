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
'    File Name    :  QmEntry.c
'
'    Purpose      :  This file has the code for entry point to the QMClient.Dll
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include "build.h"
#include "Trace.h"
#include <Iphlpapi.h>
#include <assert.h>
#include <time.h>
#pragma comment(lib, "iphlpapi.lib")

static char ModId[] = {__FILE__}; //for debugging

//#define LOG_DLL_CALLS 1

#ifdef LOG_DLL_CALLS
#include <AfdConfigProto.h>
#endif


/*TO_DO
	Note that on termination, the app. process terminates before the service has
	completed the termination handling. This is because the service becomes aware of
	a terminating application by means of the pipe-handle getting closed and this is
	an asynchronius event for the service. We may need to add some artificial 
	throttling here to make sure that the sevice has completed handling of the
	termination (ala. CTOS).
	This has been done. LIMS.
*/

char RevLevel[]="R1.1.LIMS_AA";
char sVersion[100];

extern BOOL ConnectToPipe(BYTE *pPipeName, HANDLE *hPipe, BYTE **pServerName);
extern DWORD TransferToRemote(HANDLE hPipe, void *p, DWORD c);
extern BOOL ClosePipeConnection(HANDLE hPipe);
void DEBUG_BREAK(char *ModId, int lineNo, DWORD erc);
BOOL WriteDllLog(char *data);

DWORD TlsIndex = TLS_OUT_OF_INDEXES;
#ifdef LOG_DLL_CALLS
char LogFileSpec[255] = "AfdQMClient.log";
CRITICAL_SECTION cs;
#endif
//HANDLE LogMutex;

void ServerConnect(ThreadInfo_t *pThread, BOOL fTermination);

char *GetDllVer()
{
	char CurrDate[]=__DATE__;
	char CurrTime[]=__TIME__;

	strcpy(sVersion, RevLevel);
	strcat(sVersion, " ");
	strcat(sVersion, CurrDate);
	strcat(sVersion, " ");
	strcat(sVersion, CurrTime);
	return (sVersion);
}

_declspec(dllexport) char  * GetDLLVersion(BYTE *pVer)
{
	return(GetDllVer(pVer));
}

char strFile[]={"C:\\AFD\\Server.txt"};

//for debug
#ifdef _DEBUG
BOOL ReturnPipeNameAndHandle(HANDLE *hPipe, BYTE *PipeName) {
	ThreadInfo_t *pThread;

	if(TlsIndex == TLS_OUT_OF_INDEXES) return FALSE;
	pThread = (void *) TlsGetValue(TlsIndex);

	if (!pThread)
		return(FALSE);

	*hPipe = pThread->hPipe;
	PipeName = pThread->ServerName;
	return(TRUE);
}
#endif

//change done: The server name is read from the registry instead of from 
//server.txt - $$ Supriya 15th May 1998 $$
BYTE *GetServername() {

//#define SERVERNAME_IN_FILE
#ifdef SERVERNAME_IN_FILE

	BYTE charBuff[80];
	DWORD cbRet;
	HANDLE hFile=INVALID_HANDLE_VALUE;
	BOOL fSuccess;
	static BYTE *pServername=NULL;
	DWORD i;

	if (pServername)
		return(pServername);

	hFile = CreateFile((const char *) strFile,
						GENERIC_READ | GENERIC_WRITE, 0,	// share mode 
						0, // pointer to security descriptor 
						OPEN_EXISTING,	// how to create 
						FILE_ATTRIBUTE_NORMAL,	// file attributes 
						0);

	if (hFile == INVALID_HANDLE_VALUE) {
		return(NULL);
	}

	fSuccess = ReadFile(hFile, charBuff, sizeof(charBuff), &cbRet, NULL);
	
	fSuccess = (fSuccess && (cbRet > 0));

	if (!fSuccess)
		goto AllDone;

	for (i=0; (i<cbRet); i++) {
		if (charBuff[i] == '>')
			break;
	}
	if (i == cbRet)
		goto AllDone;

	pServername = (BYTE *) malloc(i+1);
	if (!pServername)
		goto AllDone;
	memset(pServername, 0, i+1);
	memcpy(pServername, charBuff, i);

AllDone:
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	return(pServername);

#else
	HKEY KeyHandle;
	LONG lRet;
	static BYTE *pServername = NULL;
	DWORD dData,dType;
	LPTSTR lpszQMPipe = "\\pipe\\AFDQueueManager";
	LPTSTR lpszQMStr = "\\\\";
	BYTE SrvName[92];


	if (pServername)
		return(pServername);

	//Open the registry key
	/* 
	   SCR 3712:
	   Since Server Selector cannot write to HKLM hive,
	   QMClient will now search HKCU for ServerName first.
	   If ServerName isn't there, then search HKLM.
	*/
	lRet = RegOpenKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\Momentum",0,KEY_QUERY_VALUE,&KeyHandle);

	if(lRet != ERROR_SUCCESS)
	{
		// Attempt to open HKCU - 32 bit failed.
		// Now try opening HKCU - 64bit OS.
		lRet = RegOpenKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\Wow6432Node\\Momentum",0,KEY_QUERY_VALUE,&KeyHandle);
	}

	if(lRet != ERROR_SUCCESS)
	{
		// Attempt to open HKCU failed.
		// Now try opening HKLM.
		lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Momentum",0,KEY_QUERY_VALUE,&KeyHandle);

		if(lRet != ERROR_SUCCESS)
			return (pServername);
	}
	lRet = RegQueryValueEx(KeyHandle,"ServerName",0,&dType,
						   NULL,&dData);
	if(lRet != ERROR_SUCCESS)
	{
		RegCloseKey(KeyHandle);
		return pServername;
	}

	//allocate for pServerName
	if(dData < 2)
	{
		RegCloseKey(KeyHandle);
		return pServername;
	}
	pServername = (BYTE *) malloc(dData+strlen(lpszQMPipe)+strlen(lpszQMStr));
	if(pServername == NULL)
	{
		RegCloseKey(KeyHandle);
		return pServername;
	}
	memset(pServername,0,dData);
	lRet = RegQueryValueEx(KeyHandle,"ServerName",0,&dType,
						   SrvName,&dData);
	if(lRet != ERROR_SUCCESS)
	{
		RegCloseKey(KeyHandle);
		return pServername;
	}
	RegCloseKey(KeyHandle);
	strcpy(pServername,lpszQMStr);
	strcat(pServername,SrvName);
	strcat(pServername,lpszQMPipe);
	return pServername;
#endif
}

// Prints the MAC address stored in a 6 byte array to stdout
static void PrintMACaddress(unsigned char MACData[])
{
	printf("MAC Address: %02X-%02X-%02X-%02X-%02X-%02X\n", 
		MACData[0], MACData[1], MACData[2], MACData[3], MACData[4], MACData[5]);
}

// Fetches the MAC address and prints it
static __int64 GetMACaddress(void)
{
	PIP_ADAPTER_INFO pAdapterInfo;
	IP_ADAPTER_INFO AdapterInfo[16];			// Allocate information for up to 16 NICs
	DWORD dwBufLen = sizeof(AdapterInfo);		// Save the memory size of buffer
	__int64 macAddress;

	DWORD dwStatus = GetAdaptersInfo(			// Call GetAdapterInfo
		AdapterInfo,							// [out] buffer to receive data
		&dwBufLen);								// [in] size of receive data buffer
	assert(dwStatus == ERROR_SUCCESS);			// Verify return value is valid, no buffer overflow

	pAdapterInfo = AdapterInfo;					// Contains pointer to current adapter info
	macAddress = *(__int64*)&pAdapterInfo->Address;
	do {
		PrintMACaddress(pAdapterInfo->Address);	// Print MAC address
		pAdapterInfo = pAdapterInfo->Next;		// Progress through linked list
	}
	while(pAdapterInfo);						// Terminate if last adapter
	return macAddress;
}


HANDLE GetConnection()
{

	ThreadInfo_t *pThread;

	if(TlsIndex == TLS_OUT_OF_INDEXES) return NULL;
	
	pThread = (void *) TlsGetValue(TlsIndex);

	if (!pThread) {
		//why?
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		return (0);
	}

	if ((void *) pThread != pThread->p) {
		//why?
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		return (0);
	}

	//this still does not take care of the case wherein the QueueManager goes south 
	//and comes back up again. TO_DO
	if (!pThread->hPipe)
		ServerConnect(pThread, FALSE /*not a termination thread*/);

	return(pThread->hPipe);
}

void CleanupThread() {

	ThreadInfo_t *pThread;
	BOOL result;
	DWORD dwresult;

	if(TlsIndex == TLS_OUT_OF_INDEXES) return;
	
	pThread = (void *) TlsGetValue(TlsIndex);

	if (!pThread) {
		if(GetLastError() != NO_ERROR)
			DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		return;
	}

	if ((void *) pThread != pThread->p) {
		//why?
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		return;
	}

	//this is a BOOL, we may need to act on a FALSE
	ClosePipeConnection(pThread->hPipe);
	printf("Cleanup: pThread is %4X, ProcessId %4X, ThreadId %4X, User %4X\n",
			pThread->p, pThread->processId, pThread->threadId, pThread->userNumber);

	pThread->p = NULL;
	pThread->hPipe = NULL;
	result = HeapFree(GetProcessHeap(), 0, pThread);
	if(!result)
		dwresult = GetLastError();
	TlsSetValue(TlsIndex,NULL);
}

void ServerConnect(ThreadInfo_t *pThread, BOOL fTermination)
{

	BOOL fSuccess;
	HANDLE hPipe;
	InitMsg_t InitMsg;
	BYTE *pServer=NULL, *pServerConnect=NULL;
	char msgbuf[132];

	fSuccess = ConnectToPipe((pServer=(BYTE *) GetServername()), &hPipe, &pServerConnect);

	ZeroMemory(pThread->ServerName,sizeof(pThread->ServerName));
	if (pServerConnect)
		{
		strncpy(pThread->ServerName, pServerConnect, 
			    sizeof(pThread->ServerName)-1);
		}	
	
	// LSL 000426 Do NOT free pServer!
	//if (pServer)
	//	free(pServer);

	if (fSuccess) {
		pThread->hPipe = hPipe;
		InitMsg.RqHdr.cbMsg = sizeof(InitMsg);
		InitMsg.RqHdr.sCntlInfo = 0;
		InitMsg.RqHdr.nReq = 0;
		InitMsg.RqHdr.nResp = 0;
		InitMsg.RqHdr.userNum = pThread->threadId;
		if (fTermination)
			InitMsg.RqHdr.rqCode = RQ_REMOTE_TERMINATE;
		else
			InitMsg.RqHdr.rqCode = 0x1111;
		InitMsg.RqHdr.ercRet = 0;
		InitMsg.processId = pThread->processId;
		InitMsg.initSignature = E_SIGNATURE;
		InitMsg.macAddress = GetMACaddress();

		if (TransferToRemote(hPipe, (void *) &InitMsg, InitMsg.RqHdr.cbMsg) != 0) {
			pThread->hPipe = 0;
			return;
		}
		if (InitMsg.RqHdr.ercRet != E_SIGNATURE) {
			//the wrong guy got us
			ClosePipeConnection(hPipe);
			pThread->hPipe = 0;
			return;
		}
		pThread->userNumber = InitMsg.RqHdr.userNum;
		sprintf(msgbuf,"Process %4X, Thread %4X connected as User %4X\n",pThread->processId,
			pThread->threadId, pThread->userNumber);
		printf("%s",msgbuf);
		OutputDebugString(msgbuf);

	}
	//else what?
}


BOOL InitializeThread(BOOL fTermination) {
	ThreadInfo_t *pThread = NULL;
	char buf[132];

	if(TlsIndex == TLS_OUT_OF_INDEXES) {
		WriteDllLog("InitializeThread() Error:TlsIndex == TLS_OUT_OF_INDEXES");
		return FALSE;
	}
	
	pThread = HeapAlloc(GetProcessHeap(), 0, sizeof(ThreadInfo_t));
	if (!pThread) {
		sprintf(buf,"HeapAlloc failed. Error %lu.",GetLastError());
		WriteDllLog(buf);
		TlsSetValue(TlsIndex, (void *) pThread);
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		return FALSE;
	}

	pThread->p = (void *) pThread;
	pThread->processId = GetCurrentProcessId();
	pThread->threadId = GetCurrentThreadId();
	pThread->hPipe = 0;
	pThread->userNumber = 0;

	if(!TlsSetValue(TlsIndex, (void *) pThread)) {
		sprintf(buf,"InitializeThread() Error: TlsSetValue failed. Error %lu",GetLastError());
		WriteDllLog(buf);
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		return FALSE;
		}

	ServerConnect(pThread, fTermination);
	return TRUE;
}

////    DllMain
BOOL APIENTRY DllMain(HANDLE hDll, DWORD dwReason, LPVOID pStuff) 
{
	char buf[256];
#ifdef LOG_DLL_CALLS
	char AfdRoot[50];
	DWORD cbRet;
	char  LogFileName[255];
#endif

	switch (dwReason) {
		case DLL_THREAD_ATTACH:
			// LSL 000216 Defer initialization until a function is called
			break;
		case DLL_THREAD_DETACH:
			if(TlsIndex != TLS_OUT_OF_INDEXES) {
				printf ("THREAD_DETACH, ProcessId %4X, ThreadId %4X\n",
						 GetCurrentProcessId(), GetCurrentThreadId());
				sprintf (buf,"T_OUT %04x:%04x",
						 GetCurrentProcessId(), GetCurrentThreadId());
				WriteDllLog(buf);
				CleanupThread();
			}
			else
			{
			sprintf (buf,"*** T_OUT %04x:%04x ** TLS_OUT_OF_INDEXES ***",
						 GetCurrentProcessId(), GetCurrentThreadId());
			WriteDllLog(buf);
			}
			break;
		case DLL_PROCESS_DETACH:
			if(TlsIndex != TLS_OUT_OF_INDEXES) {
				printf ("PROCESS_DETACH, ProcessId %4X, ThreadId %4X\n",
					 GetCurrentProcessId(), GetCurrentThreadId());
				sprintf (buf,"P_OUT %04x:%04x",
					 GetCurrentProcessId(), GetCurrentThreadId());
				WriteDllLog(buf);
			
				CleanupThread();
				//now do the termination
				InitializeThread(TRUE /*it is a thread dedicated to termination */);
				CleanupThread();
				TlsFree(TlsIndex);
				TlsIndex = TLS_OUT_OF_INDEXES;
#ifdef LOG_DLL_CALLS
				DeleteCriticalSection(&cs);
#endif
				}
				else
				{
				sprintf (buf,"*** P_OUT %04x:%04x ** TLS_OUT_OF_INDEXES ***",
						 GetCurrentProcessId(), GetCurrentThreadId());
				WriteDllLog(buf);
				}
			break;
		case DLL_PROCESS_ATTACH:
			
#ifdef LOG_DLL_CALLS
		InitializeCriticalSection(&cs);
		if (!GetAfdRoot(AfdRoot, sizeof(AfdRoot), &cbRet)) {
//			LogNTEvent(MSG_NO_AFD_ROOT, NULL);
//			SetLastError(erc = ercInconsistency);
//			return(erc);
		}
		else
		{

			// Form the Log File name
			strcpy(LogFileName,"");
			sprintf(LogFileName, "%s\\Logs\\%s", AfdRoot, LogFileSpec);
			strcpy(LogFileSpec, LogFileName);
		}
#endif

			// LSL 000512 Defer process initialization
		/*
			InitializeCriticalSection(&cs);
			//CreateMutex(NULL, FALSE, "AfdQmClientLog");
			printf ("PROCESS_ATTACH, ProcessId %4X, ThreadId %4X\n",
					 GetCurrentProcessId(), GetCurrentThreadId());
			sprintf (buf,"P_IN %04x:%04x",
					 GetCurrentProcessId(), GetCurrentThreadId());
			WriteDllLog(buf);
			TlsIndex = TlsAlloc();
			if (TlsIndex == TLS_OUT_OF_INDEXES) {
				DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
				sprintf (buf,"*** P_IN %04x:%04x ** TLS_OUT_OF_INDEXES ***",
						 GetCurrentProcessId(), GetCurrentThreadId());
				WriteDllLog(buf);
				return FALSE;
			}
			return InitializeThread(FALSE //not a termination thread);
		*/
			break;
		default:
			//DEBUG_BREAK((char *)__FILE__,__LINE__);
			break;
		} // end switch (dwReason)
	return TRUE;
}


#ifdef LOG_DLL_CALLS
BOOL WriteDllLog(char *data) {
	HANDLE hLogFile;
	char datebuf[20];
	char timebuf[20];
	char buffer[512];
	DWORD dwBytesWritten,dwFileSize;
	BOOL result;
	
		
	OutputDebugString(data);
	EnterCriticalSection(&cs);
	hLogFile = CreateFile(LogFileSpec,
					GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,	// share mode 
					0, // pointer to security descriptor 
					OPEN_ALWAYS, // creation mode
					FILE_ATTRIBUTE_NORMAL,	// file attributes 
					0);
	if (INVALID_HANDLE_VALUE == hLogFile)
		{LeaveCriticalSection(&cs);	OutputDebugString("Unable to access AFDQMClient log file"); return FALSE;}
	dwFileSize = GetFileSize(hLogFile, 0);
	SetFilePointer(hLogFile, dwFileSize, NULL, FILE_BEGIN);

	_strdate(datebuf); 
	_strtime(timebuf);
	sprintf(buffer,"%s %s ",datebuf,timebuf);
	result = WriteFile(hLogFile,buffer,strlen(buffer),&dwBytesWritten,NULL);
	if(FALSE == result) {
		CloseHandle(hLogFile);
		LeaveCriticalSection(&cs);
		return FALSE;
		}
	
	if(data) {
		result = WriteFile(hLogFile,data,strlen(data),&dwBytesWritten,NULL);
		if(FALSE == result) {
			CloseHandle(hLogFile);
			LeaveCriticalSection(&cs);
			return TRUE;
			}
		}
	
	result = WriteFile(hLogFile,"\r\n",2,&dwBytesWritten,NULL);
	if(FALSE == result) {
		CloseHandle(hLogFile);
		LeaveCriticalSection(&cs);
		return FALSE;
		}
	
	
	CloseHandle(hLogFile);
	LeaveCriticalSection(&cs);
	return TRUE;
}

#else
BOOL WriteDllLog(char *data) {
#ifdef _DEBUG
	OutputDebugString(data);
#endif
	return TRUE;
}
#endif