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
'    Purpose      :  This file has the code for entry points for all the  
'                    AFDQueueManager APIs.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
9/July AA	Added function RemoveKeyedQueueEntry().
			Activated function RewriteMarkedQueueEntry().
*/

#include <windows.h>
#include <stdio.h>
#include "RqDefs.h"
#include "Trace.h"
#include "build.h"

extern DWORD TlsIndex;

static char ModId[] = {__FILE__}; //for debugging

//BOOLEAN fDebugMode={TRUE};
extern BOOL WriteDllLog(char *data);
extern BOOL InitializeThread(BOOL fTermination);

ThreadInfo_t *GetThreadInfo();

_declspec(dllexport) DWORD AddQueue(LPVOID pbQueueName, WORD cbQueueName, 
								    LPVOID pbFileSpec, WORD cbFileSpec, 
						 		    WORD wEntrysize, WORD wQueueType, LPVOID pQhRet)
{
	char buf[256];
	DWORD result;
	ThreadInfo_t *pThread;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("AddQueue - GetThreadInfo failed.");
		return 1234;
		}

	sprintf(buf,"%04x:%04x AddQueue(%*s, %i, %*s, %i)",
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName, cbFileSpec, 
		pbFileSpec, cbFileSpec);
	WriteDllLog(buf);
	
	result = M_AddQueue(	pbQueueName, cbQueueName, 
						pbFileSpec, cbFileSpec, 
						wEntrysize, wQueueType, pQhRet
					  );
	sprintf(buf,"%lu = %04x:%04x AddQueue(%*s, %i, %*s, %i)",result & 0xFFFF,
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName, cbFileSpec, 
		pbFileSpec, cbFileSpec);
	WriteDllLog(buf);
	
	return result;

}


_declspec(dllexport) DWORD AddQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
										 BOOL fQueueIfNoServer, WORD priority,
										 WORD queueType, 
										 LPVOID pEntry, WORD sEntry, 
										 LPVOID pDateTime, WORD repeatTime)
{
	char buf[256];
	DWORD result;
	ThreadInfo_t *pThread;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("AddQueueEntry - GetThreadInfo failed.");
		return 1234;
	}

	sprintf(buf,"%04x:%04x AddQueueEntry(%*s, %i, %s, %i)",
		pThread->processId,pThread->threadId,		
		cbQueueName, pbQueueName, cbQueueName, 
		fQueueIfNoServer ? "TRUE" : "FALSE", priority); 
	WriteDllLog(buf);
	
	result = M_AddQueueEntry(	pbQueueName, cbQueueName, 
							fQueueIfNoServer, priority,
							queueType, pEntry, sEntry, 
							pDateTime, repeatTime
						   );
	sprintf(buf,"%lu = %04x:%04x AddQueueEntry(%*s, %i, %s, %i)",result & 0xFFFF,
		pThread->processId,pThread->threadId,		
		cbQueueName, pbQueueName, cbQueueName, 
		fQueueIfNoServer ? "TRUE" : "FALSE", priority); 
	WriteDllLog(buf);
	return(result);
	
}


_declspec(dllexport) DWORD EstablishQueueServer(LPVOID pbQueueName, WORD cbQueueName, 
										 	    WORD queueType, BOOL fUniqueServer)
{
	char buf[256];
	DWORD result;
	ThreadInfo_t *pThread;

	sprintf(buf,"[%04x:%04x] EstablishQueueServer(%08x, %i, %i, %s)",
		GetCurrentProcessId(),GetCurrentThreadId(),
			pbQueueName, cbQueueName, 
			queueType,
			fUniqueServer ? "TRUE" : "FALSE"); 
	WriteDllLog(buf);
	
	
	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("EstablishQueueServer - GetThreadInfo failed.");
		return 1234;
		}

	sprintf(buf,"%04x:%04x EstablishQueueServer(%*s, %i, %i, %s)",
			pThread->processId,pThread->threadId,
			cbQueueName, pbQueueName, cbQueueName, 
			queueType,
			fUniqueServer ? "TRUE" : "FALSE"); 
	WriteDllLog(buf);
	
	result = M_EstablishQueueServer(	pbQueueName, cbQueueName, 
									queueType, fUniqueServer
								 );
	sprintf(buf,"%lu = %04x:%04x EstablishQueueServer(%*s, %i, %i, %s)",result & 0xFFFF,
			pThread->processId,pThread->threadId,
			cbQueueName, pbQueueName, cbQueueName, 
			queueType,
			fUniqueServer ? "TRUE" : "FALSE"); 
	WriteDllLog(buf);
	return(result);
}


_declspec(dllexport) DWORD GetQMStatus(WORD wQueueType, BOOL fHealthCheck, 
									   LPVOID pStatusRet, WORD sStatusMax)
{
	char buf[256];
	DWORD result;
	ThreadInfo_t *pThread;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("GetQMStatus - GetThreadInfo failed.");
		return 1234;
		}


	sprintf(buf,"%04x:%04x GetQMStatus(%i, %s, %08x, %i)",
		pThread->processId,pThread->threadId,
		wQueueType,
		fHealthCheck ? "TRUE" : "FALSE", 
		pStatusRet, sStatusMax);
	WriteDllLog(buf);

	result = M_GetQMStatus(wQueueType, fHealthCheck, pStatusRet, sStatusMax);
	
	sprintf(buf,"%lu = %04x:%04x GetQMStatus(%i, %s, %08x, %i)",result & 0xFFFF,
		pThread->processId,pThread->threadId,
		wQueueType,
		fHealthCheck ? "TRUE" : "FALSE", 
		pStatusRet, sStatusMax);
	WriteDllLog(buf);

	return result;
}


_declspec(dllexport) DWORD MarkKeyedQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
										 	   LPVOID pbKey1, WORD cbKey1, WORD oKey1, 
											   LPVOID pbKey2, WORD cbKey2, WORD oKey2, 
											   LPVOID pEntryRet, WORD sEntryRet, 
											   LPVOID pStatusBlock, WORD sStatusBlock)
{
	char buf[256];
	DWORD result;
	ThreadInfo_t *pThread;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("MarkKeyedQueueEntry - GetThreadInfo failed.");
		return 1234;
		}

	sprintf(buf,"%04x:%04x MarkKeyedQueueEntry(%*s, %i, %08x, %i, %8x, %i)",
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName,	
	    pEntryRet, sEntryRet, 
	    pStatusBlock, sStatusBlock);
		
	WriteDllLog(buf);
	
	result = M_MarkKeyedQueueEntry(pbQueueName, cbQueueName, 
								 pbKey1, cbKey1, oKey1, 
								 pbKey2, cbKey2, oKey2, 
								 pEntryRet, sEntryRet, 
								 pStatusBlock, sStatusBlock
								);
	sprintf(buf,"%lu = %04x:%04x MarkKeyedQueueEntry(%*s, %i, %08x, %i, %8x, %i)",result & 0xffff,
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName,	
	    pEntryRet, sEntryRet, 
	    pStatusBlock, sStatusBlock);
		
	WriteDllLog(buf);
	
	
	return result;
}

_declspec(dllexport) DWORD MarkNextQueueEntry(LPVOID pbQueueName, WORD cbQueueName,	
											  BOOL fReturnIfNoEntries, 
											  LPVOID pEntryRet, WORD sEntryRet, 
											  LPVOID pStatusBlock, WORD sStatusBlock)
{
	char buf[256];
	DWORD result;
	ThreadInfo_t *pThread;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("MarkNextQueueEntry - GetThreadInfo failed.");
		return 1234;
		}

	
	sprintf(buf,"%04x:%04x MarkNextQueueEntry(%*s, %i, %s, %08x, %i, %8x, %i)",
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName,	
		fReturnIfNoEntries ? "TRUE" : "FALSE", 
	    pEntryRet, sEntryRet, 
	    pStatusBlock, sStatusBlock);
		
	WriteDllLog(buf);

	
	result = M_MarkNextQueueEntry(pbQueueName, cbQueueName,
								fReturnIfNoEntries, 
								pEntryRet, sEntryRet, 
								pStatusBlock, sStatusBlock
							   );
	sprintf(buf,"%lu = %04x:%04x MarkNextQueueEntry(%*s, %i, %s, %08x, %i, %8x, %i)",result & 0xffff,
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName,	
		fReturnIfNoEntries ? "TRUE" : "FALSE", 
	    pEntryRet, sEntryRet, 
	    pStatusBlock, sStatusBlock);
		
	WriteDllLog(buf);
	
	
	return result;
}

_declspec(dllexport) DWORD ReadKeyedQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
											   LPVOID pbKey1, WORD cbKey1, WORD oKey1, 
											   LPVOID pbKey2, WORD cbKey2, WORD oKey2, 
											   LPVOID pEntryRet, WORD sEntryRet, 
											   LPVOID pStatusBlock, WORD sStatusBlock)
{
	char buf[256];
	DWORD result;
	ThreadInfo_t *pThread;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("ReadKeyedQueueEntry - GetThreadInfo failed.");
		return 1234;
		}

	sprintf(buf,"%04x:%04x ReadKeyedQueueEntry(%*s, %i, %08x, %i, %8x, %i)",
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName,	
	    pEntryRet, sEntryRet, 
	    pStatusBlock, sStatusBlock);
		
	WriteDllLog(buf);


	result = M_ReadKeyedQueueEntry(pbQueueName, cbQueueName, 
								 pbKey1, cbKey1, oKey1, 
								 pbKey2, cbKey2, oKey2, 
								 pEntryRet, sEntryRet, 
								 pStatusBlock, sStatusBlock
								);
	sprintf(buf,"%lu = %04x:%04x ReadKeyedQueueEntry(%*s, %i, %08x, %i, %8x, %i)",result & 0xffff,
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName,	
	    pEntryRet, sEntryRet, 
	    pStatusBlock, sStatusBlock);
		
	WriteDllLog(buf);
	
	
	return result;
}

_declspec(dllexport) DWORD ReadNextQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
											  DWORDLONG qeh, 
											  LPVOID pEntryRet, WORD sEntryRet, 
											  LPVOID pStatusBlock, WORD sStatusBlock)
{
	char buf[256];
	DWORD result;
	ThreadInfo_t *pThread;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("ReadNextQueueEntry - GetThreadInfo failed.");
		return 1234;
		}

	sprintf(buf,"%04x:%04x ReadNextQueueEntry(%*s, %i, %08x, %i, %8x, %i)",
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName,	
	    pEntryRet, sEntryRet, 
	    pStatusBlock, sStatusBlock);
		
	WriteDllLog(buf);


	result = M_ReadNextQueueEntry(pbQueueName, cbQueueName, 
								qeh, pEntryRet, sEntryRet, 
								pStatusBlock, sStatusBlock
							   );
	sprintf(buf,"%lu = %04x:%04x ReadNextQueueEntry(%*s, %i, %08x, %i, %8x, %i)",result & 0xffff,
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName,	
	    pEntryRet, sEntryRet, 
	    pStatusBlock, sStatusBlock);
		
	WriteDllLog(buf);
	
	
	return result;
}

_declspec(dllexport) DWORD RemoveKeyedQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
												 LPVOID pbKey1, 
												 WORD cbKey1, WORD oKey1, 
												 LPVOID pbKey2, 
												 WORD cbKey2, WORD oKey2)
{
	char buf[256];
	DWORD result;
	ThreadInfo_t *pThread;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("RemoveKeyedQueueEntry - GetThreadInfo failed.");
		return 1234;
		}

	sprintf(buf,"%04x:%04x RemoveKeyedQueueEntry(%*s, %i)",
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName);
		
	WriteDllLog(buf);


	result = M_RemoveKeyedQueueEntry(pbQueueName, cbQueueName, 
								   pbKey1, cbKey1, oKey1, 
								   pbKey2, cbKey2, oKey2);
	sprintf(buf,"%lu = %04x:%04x RemoveKeyedQueueEntry(%*s, %i)",result & 0xffff,
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName);
		
	WriteDllLog(buf);
	
	
	return result;
}

_declspec(dllexport) DWORD RemoveMarkedQueueEntry(LPVOID pbQueueName, WORD cbQueueName,
												  DWORDLONG qeh)
{
	char buf[256];
	DWORD result;
	ThreadInfo_t *pThread;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("RemoveMarkedQueueEntry - GetThreadInfo failed.");
		return 1234;
		}

	sprintf(buf,"%04x:%04x RemoveMarkedQueueEntry(%*s, %i)",
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName);
		
	WriteDllLog(buf);


	result = M_RemoveMarkedQueueEntry(pbQueueName, cbQueueName, qeh);

	sprintf(buf,"%lu = %04x:%04x RemoveMarkedQueueEntry(%*s, %i)",result & 0xffff,
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName);
		
	WriteDllLog(buf);
	
	
	return result;
}


_declspec(dllexport) DWORD ReScheduleMarkedQueueEntry(LPVOID pbQueueName, 
													  WORD cbQueueName, 
													  DWORDLONG qeh)
{
	char buf[256];
	ThreadInfo_t *pThread;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("ReScheduleMarkedQueueEntry - GetThreadInfo failed.");
		return 1234;
		}

	sprintf(buf,"%04x:%04x ReScheduleMarkedQueueEntry(%*s, %i) dummy call always returns 0",
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName);
		
	WriteDllLog(buf);


	return(0);

}

_declspec(dllexport) DWORD RewriteMarkedQueueEntry(LPVOID pbQueueName, 
												   WORD cbQueueName, 
												   DWORDLONG qeh, LPVOID pEntry, 
												   WORD sEntry)
{
	char buf[256];
	DWORD result;
	ThreadInfo_t *pThread;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("RewriteMarkedQueueEntry - GetThreadInfo failed.");
		return 1234;
		}

	sprintf(buf,"%04x:%04x RewriteMarkedQueueEntry(%*s, %i)",
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName);
		
	WriteDllLog(buf);


	result = M_RewriteMarkedQueueEntry(pbQueueName, cbQueueName, qeh, 
									 pEntry, sEntry);

	sprintf(buf,"%lu = %04x:%04x RewriteMarkedQueueEntry(%*s, %i)",result & 0xffff,
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName);
		
	WriteDllLog(buf);
	
	
	return result;
}

_declspec(dllexport) DWORD TerminateQueueServer(LPVOID pbQueueName, WORD cbQueName)
{
	ThreadInfo_t *pThread;
	char buf[132];
	DWORD result;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("TerminateQueueServer - GetThreadInfo failed.");
		return 1234;
		}


	sprintf(buf,"%04x:%04x TerminateQueueServer(%*s, %i)",
			pThread->processId,pThread->threadId,
			cbQueName, pbQueueName, cbQueName);
			
	WriteDllLog(buf);
	
	result = M_TerminateQueueServer(pbQueueName, cbQueName);

	sprintf(buf,"%lu = %04x:%04x TerminateQueueServer(%*s, %i)",result & 0xFFFF,
			pThread->processId,pThread->threadId,
			cbQueName, pbQueueName, cbQueName);
			
	WriteDllLog(buf);
	return(result);	

}

_declspec(dllexport)DWORD UnmarkQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
										   DWORDLONG qeh)
{
	char buf[256];
	DWORD result;
	ThreadInfo_t *pThread;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("UnmarkQueueEntry - GetThreadInfo failed.");
		return 1234;
		}

	sprintf(buf,"%04x:%04x UnmarkQueueEntry(%*s, %i)",
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName);
		
	WriteDllLog(buf);


	result = M_UnmarkQueueEntry(pbQueueName, cbQueueName, qeh);
	sprintf(buf,"%lu = %04x:%04x UnmarkQueueEntry(%*s, %i)",result & 0xffff,
		pThread->processId,pThread->threadId,
		cbQueueName, pbQueueName, cbQueueName);
		
	WriteDllLog(buf);
	
	
	return result;
}

_declspec(dllexport)DWORD CleanQueue(DWORD qh)
{
	ThreadInfo_t *pThread;
	char buf[132];
	DWORD result;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("CleanQueue - GetThreadInfo failed.");
		return 1234;
		}

	sprintf(buf,"%04x:%04x CleanQueue(%lu)",
		pThread->processId,pThread->threadId,
		qh);
	WriteDllLog(buf);

	result = M_CleanQueue(qh);
	
	sprintf(buf,"%lu = %04x:%04x CleanQueue(%lu)",result & 0xffff,
		pThread->processId,pThread->threadId,
		qh);
	WriteDllLog(buf);
	return result;
}

_declspec(dllexport)DWORD RemoveQueue(DWORD qh)
{
	ThreadInfo_t *pThread;
	char buf[132];
	DWORD result;

	pThread = (void *) GetThreadInfo();
	if(!pThread) {
		WriteDllLog("RemoveQueue - GetThreadInfo failed.");
		return 1234;
		}

	sprintf(buf,"%04x:%04x RemoveQueue(%lu)",
		pThread->processId,pThread->threadId,
		qh);
	WriteDllLog(buf);

	result = M_RemoveQueue(qh);
	
	sprintf(buf,"%lu = %04x:%04x RemoveQueue(%lu)",result & 0xffff,
		pThread->processId,pThread->threadId,
		qh);
	WriteDllLog(buf);
	return result;
}

ThreadInfo_t *GetThreadInfo() {
	
	ThreadInfo_t *pThread;
	char buf[132];
	DWORD result;

	// Check if process has attached
	if (TlsIndex == TLS_OUT_OF_INDEXES) {
//		InitializeCriticalSection(&cs);
		printf ("DYNAMIC PROCESS_ATTACH, ProcessId %4X, ThreadId %4X\n",
			 GetCurrentProcessId(), GetCurrentThreadId());
		sprintf (buf,"DYNAMIC P_IN %04x:%04x",
			 GetCurrentProcessId(), GetCurrentThreadId());
		WriteDllLog(buf);
		TlsIndex = TlsAlloc();
		if (TlsIndex == TLS_OUT_OF_INDEXES) {
			sprintf (buf,"*** DYNAMIC P_IN %04x:%04x ***",
				 GetCurrentProcessId(), GetCurrentThreadId());
			WriteDllLog(buf);
			DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
			return NULL;
			}
		}
	
	// Check if ThreadInfo exists
	pThread = (ThreadInfo_t *) TlsGetValue(TlsIndex);
	if(NULL == pThread) {
		result = GetLastError();
		if(result) {
			sprintf(buf,"TlsGetValue failed. Error %lu.",result);
			WriteDllLog(buf);
			return NULL;
			}
		printf ("*** DYNAMIC T_IN %04x:%04x ***\n",
				 GetCurrentProcessId(), GetCurrentThreadId());
		sprintf (buf,"*** DYNAMIC T_IN %04x:%04x ***",
				 GetCurrentProcessId(), GetCurrentThreadId());
		WriteDllLog(buf);
		if(!InitializeThread(FALSE)) pThread = NULL;
		// pThread value should now be stored in TLS
		pThread = (ThreadInfo_t *) TlsGetValue(TlsIndex);	
		}
	return pThread;
}

