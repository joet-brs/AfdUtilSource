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
'    File Name    :  PipeSrv.cpp
'
'    Purpose      :  This file has the code for the Pipe Server for the  
'                    AFDQueueManager.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
17/Aug/1998	LSL	Fixed memory leak in the case H_TERMINATEPROCESS in PipeServer().
				Also initialize pMsg to NULL.
02 Aug 1999 LSL Replaced all mutexes with CriticalSections.
*/

/*TO_DO
The GetInternalMessage() each time allocates the structure InternalMsg_t, and it is
deallocated in the ProcessMessage() function. We may be able to do with a global for
this structure.
*/
#define _WIN32_WINNT 0x0400
#include "basic.h" 
#include "build.h"
#include <AfdMisc.h>
#include <process.h>
#include "Trace.h"
#include <stdio.h>
#include <WinBase.h>

static char *ModId[]={__FILE__};

#define CONNECTING_STATE 0 
#define READING_STATE 1 
#define WRITING_STATE 2 
#define TERMINATING_STATE 3
#define REMOTE_TERMINATING_STATE 4
#define HOLD_FOR_RESPONSE_STATE 5
#define TERMINATED_STATE 6

#define INSTANCES 32
#define PIPE_TIMEOUT	20000		//20 Second
#define MyErrExit		printf

typedef struct InternalMsg_s {
	struct InternalMsg_s *Next;
	WORD msgType;
	void *p;
	DWORD userNum;
	void *pMsg;			//in case of pipe messages, this will be PipeMsg_t
} InternalMsg_t;
 
extern BOOL ServiceRequest(RqHdr_t *pBuff);
extern void ProcessTimer(BOOL);
extern void TerminateUser(DWORD userNum);
extern BOOL WriteLogData(char *data, DWORD level = 0);


//LPTSTR lpszPipename = "\\\\.\\pipe\\QueueManager";
LPTSTR g_lpszPipename;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
|                                                                           |
| The PipeServer consists of one thread that is creates an instance of the  |
| pipe and Offers it to any thread that wants to connect. This function is  |
| coded in PipeOfferThread(). Once a client "connects" successfully to the  |
| Pipe, then the PipeOfferThread() creates a newThread to handle this       |
| instance. This is handled in the fucntion PipeInstanceThread(). Each      |
| thread instance has a data structure associated with it called which has  |
| the information about this connection. The structure is called PipeInst_t.|
| Note that the first element in the structure is a pointer to the Next. All|
| instances of the PipeConnections are liked in a singly-linked list. At any|
| instant the connection corresponding to a client may be in one of the     |
| Queues:                                                                   |
| g_PipeMsgQueue - indicates to the WorkerThread that this instance has a msg |
|                that needs to be processed.                                | 
| g_PipeInstQueue - indicates that this instance is idle (waiting on a msg    |
|                 from the client, in this case it will be in the blocked   |
|                 Read() or this instance has sent a message to the worker  |
|                 thread and is waiting for a response, in which case it    |
|                 be blocked on its respEvent.                              |
|                                                                           |
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

PipeInst_t *g_PipeMsgQueue = NULL;
PipeInst_t *g_PipeInstQueue = NULL;

CRITICAL_SECTION g_PipeMsgQueueMutex;
CRITICAL_SECTION g_PipeInstQueueMutex;
CRITICAL_SECTION g_RespondQueueMutex;

#define H_TERMINATE 0
#define H_RESPONDTOPIPE 1
#define H_WORKERWAKEUP 2
#define H_TIMER 3
#define H_MAX 4

#define M_TERMINATE 0
#define M_RESPONDTOPIPE 1
#define M_WORKERWAKEUP 2
#define M_TIMER 3
#define M_TERMINATEUSER 4
#define M_TERMINATEPROCESS 5

HANDLE g_hEvents[H_MAX];
HANDLE g_TerminateEvent;
HANDLE g_RespondToPipeEvent;
HANDLE g_WorkerWakeupEvent;
HANDLE g_WaitableTimer;

BOOL g_fAsync = FALSE;

extern HANDLE hEventServiceSync;
extern HANDLE hRequestExitEvent;
WORD WATCHDOG_INTERVAL = 60; //secs TimeOut for managing timed-queues


void TerminatePipeProcess(PipeInst_t *pPipeInst);
void TerminatePipeUser(PipeInst_t *pPipeInst);

#define GRANT_ALL
#ifdef GRANT_ALL
/*------------------------------------------------------------------
| Name: GetPipeSecurityDesc //base from MSDN, modified for our use
| Desc: sets up security to use on pipe to allow access to everyone;
|   upon return, can use saPipeSecurity to set security on pipe
|   when using CreateNamedPipe
------------------------------------------------------------------*/
SECURITY_ATTRIBUTES *GetPipeSecurityDesc()
{
	SECURITY_ATTRIBUTES *saPipeSecurity=0;
	PSECURITY_DESCRIPTOR pPipeSD = NULL;
	BOOL fSuccess=FALSE;
	
	// security inits
	saPipeSecurity = (SECURITY_ATTRIBUTES *) zmalloc(sizeof(SECURITY_ATTRIBUTES));

	// alloc & init SD
	pPipeSD = (PSECURITY_DESCRIPTOR) zmalloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
	fSuccess = InitializeSecurityDescriptor(pPipeSD, SECURITY_DESCRIPTOR_REVISION);
	if (!fSuccess)
		goto AllDone;
	
	// set NULL DACL on the SD
	fSuccess = SetSecurityDescriptorDacl(pPipeSD, TRUE, (PACL) NULL, FALSE);
	if (!fSuccess)
		goto AllDone;
	
	// now set up the security attributes
	saPipeSecurity->nLength = sizeof ( SECURITY_ATTRIBUTES);
	saPipeSecurity->bInheritHandle  = TRUE; 
	saPipeSecurity->lpSecurityDescriptor = pPipeSD;

AllDone:
	if (!fSuccess) {
		free(pPipeSD);
		pPipeSD = NULL;
		free(saPipeSecurity);
		saPipeSecurity = NULL;
		pPipeSD = NULL;
	}

	return(saPipeSecurity);
}
#endif

int CmpStateAndProcess(void *Item, void *pCmp) {
	TerminateStruc_t *terminate = (TerminateStruc_t *) pCmp;
	PipeInst_t *pPipeInst = (PipeInst_t *) Item;

	return((pPipeInst->dwState == HOLD_FOR_RESPONSE_STATE) && 
		   (pPipeInst->processIdOfClient == terminate->processId) &&
		   (pPipeInst->macAddress == terminate->macAddress)
		  );
}


int ParseQueueForUserNum(void *pItem, void *pCmp) {

	if ( ((PipeInst_t *)pItem)->userNum == *(DWORD *)pCmp )
		return (TRUE);
	else
		return (FALSE);

}

int ParseQueueMe(void *pItem, void *pCmp) {
	if ( ((PipeInst_t *)pItem)->Me == (PipeInst_t *) pCmp )
		return (TRUE);
	else
		return (FALSE);
}

/*
void WaitForQueue(HANDLE hMutex) {

	DWORD waitRet = WaitForSingleObject(hMutex, INFINITE);
	if (waitRet != WAIT_OBJECT_0) {    // LSL 981104 SCR#206
		//serious
		DWORD dwErr = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
	}
}

void ReleaseQueue(HANDLE hMutex) {
	if (!(ReleaseMutex(hMutex))) {
		DWORD dwErr = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);;
	}
}
*/

void PipeWakeupServer(PipeInst_t *pPipe)
{
	PipeInst_t *pPipeRem;


	//the message has to be queued at either the active Queue or the inactive Queue
	//not on both
	//LINK INACTIVE TO ACTIVE

	//Remove from PipeInstQueue
	EnterCriticalSection(&g_PipeInstQueueMutex);
	pPipeRem = (PipeInst_t *) ListRemove((void **) &g_PipeInstQueue, (void *) pPipe);
	//sanity
	if (pPipeRem != pPipe) {
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
	}
	LeaveCriticalSection(&g_PipeInstQueueMutex);

	//Add PipeMsgQueue
	EnterCriticalSection(&g_PipeMsgQueueMutex);
	ListAppend((void **) &g_PipeMsgQueue, (void *) pPipe);
	LeaveCriticalSection(&g_PipeMsgQueueMutex);

	if (!(SetEvent(g_WorkerWakeupEvent))) {
		//serious error
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
	}

}

/* 	This function fetches messages from three events. In priority they are:    
H_TERMINATE, H_RESPONDTOPIPE, H_WORKERWAKEUP.                                   
* The H_TERMINATE is a Control Message to the system, and may come from sources like the
Service DLL flagging a termination, or from the SCM. The MsgType will be 0.
* The H_RESPONDTOPIPE is a message from the the Worker telling that a pending write to the
pipe may now be done. The MsgType will be 1. This is not being used anymore.
* The messages to wakeup the worker are initiated by any one of the pipe connection threads
and they all flag the same event, and the message is on the PipeMsgQueue.

The function first Waits for the first two events with a parameter to return immediately, and
if there are no messages pending then frisks the PipeMsgQueue

// GetInternalMessage is only called by PipeServer() - the main thread.
*/

InternalMsg_t *GetInternalMessage()
{
	DWORD dwErr;
	PipeInst_t *pPipe;
	DWORD waitRet;
	DWORD iEvent;
	InternalMsg_t *pRet;
	char msgbuffer[255];

/*
  This is so we get the respondToPipe Messages and Control Messages on a priority basis.
  This wait loop would get the messages for the ControlEvent or RespondToPipeEvent.
  TO_DO: Note that RespondToPipe Message is no longer processed from here, so it will
  not be necessary any more.
  We come out of this loop immediately, and then go in with iEvent=H_WORKERWAKEUP;
  this forces us to frisk the WakeupQueue.
*/

	if ((waitRet = WaitForMultipleObjects(H_RESPONDTOPIPE+1, //2 events
										  g_hEvents, 
										  FALSE, 
										  0)) == WAIT_FAILED) {
		//serious error
		dwErr = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
	}

	if (waitRet == WAIT_TIMEOUT) {
		iEvent = H_WORKERWAKEUP;
	} else {
		if ((iEvent = waitRet - WAIT_OBJECT_0) > H_RESPONDTOPIPE) {
			//why?
			dwErr = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
		}
	} //event signalled
	

/*
We enter this loop with the iEvent set to Control or RespondToPipe (not used anymore)
or the iEvent is forced to WorkerWakeup.
*/
	for(;;) {
		
		switch (iEvent) {

		case H_TERMINATE:
			pRet = (InternalMsg_t *) zmalloc(sizeof(InternalMsg_t));
			pRet->msgType = M_TERMINATE;
			return (pRet);
			break;

		case H_RESPONDTOPIPE:
			//we are signalling the Event at which the respective PipeInstance is
			//waiting directly, see the main-loop for the pipe server
			break;

		case H_WORKERWAKEUP:
			//LINK ACTIVE TO INACTIVE
			//Remove from PipeMsgQueue
			EnterCriticalSection(&g_PipeMsgQueueMutex);
			
			sprintf(msgbuffer,"MessageQueue Count = %lu\n", ListCount((void **) &g_PipeMsgQueue) );
			OutputDebugString(msgbuffer);
			WriteLogData(msgbuffer, 2);

			pPipe = (PipeInst_t *) ListRetrieve((void **) &g_PipeMsgQueue);
			LeaveCriticalSection(&g_PipeMsgQueueMutex);
			if (pPipe) {
				//now add this to the inactive Queue; Add to PipeInstQueue
				EnterCriticalSection(&g_PipeInstQueueMutex);
				ListAppend((void **) &g_PipeInstQueue, pPipe);
				
				pRet = (InternalMsg_t *) zmalloc(sizeof(InternalMsg_t));

				switch (pPipe->dwState) {
					case REMOTE_TERMINATING_STATE:
						pRet->msgType = M_TERMINATEPROCESS;
						break;
					case TERMINATING_STATE:
						pRet->msgType = M_TERMINATEUSER;
						break;
					default:
						pRet->msgType = M_WORKERWAKEUP;
						break;
					}
				LeaveCriticalSection(&g_PipeInstQueueMutex);

				/*if (pPipe->dwState == TERMINATING_STATE)
					pRet->msgType = M_TERMINATEUSER;
				else
					pRet->msgType = M_WORKERWAKEUP;*/

					pRet->p = pPipe; //PipeInst_t
					pRet->userNum = pPipe->userNum;
					pRet->pMsg = pPipe->chBuf;
					return (pRet);
				}  // end if(pPipe)
			break;

		case H_TIMER:
			pRet = (InternalMsg_t *) zmalloc(sizeof(InternalMsg_t));
			pRet->msgType = M_TIMER;
			return (pRet);
			break;

		default:
			if (iEvent > H_WORKERWAKEUP) {
				//why?
				DEBUG_BREAK((char *)__FILE__,__LINE__, 0xb00b);
			}
			//Check of there is a message on the pipeQueue
			//wait for the mutex
			


		} //switch

		
		if ((waitRet = WaitForMultipleObjects(H_WORKERWAKEUP+1, g_hEvents, FALSE, 
											  WATCHDOG_INTERVAL*1000)) == WAIT_FAILED) {
			//serious error
			dwErr = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
		}

		if (waitRet == WAIT_TIMEOUT) {
			iEvent = H_TIMER;
			//Timeout
		} else {
			if ((iEvent = waitRet - WAIT_OBJECT_0) > H_WORKERWAKEUP) {
				//why?
				dwErr = GetLastError();
				DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
			}
		}
	} //for(;;)
}

// ProcessMessage is only called by PipeServer() - the main thread.
BOOL ProcessMessage(InternalMsg_t *pMsg, void *pBuff)
{
	BOOL fRespond;

	
	fRespond = ServiceRequest((RqHdr_t *)pBuff);

	if (fRespond) {
		return(TRUE);
	} else
	{
		//add this message to the PendingQueue, the service will then respond to each
		//at which time search the queue using the userNum and do the needful

		return (FALSE);
	}
	
}

unsigned int __stdcall PipeInstanceThread(LPVOID lvpParam)
{
	BOOL fConnect=TRUE, fSuccess; 
	DWORD erc;
	PipeInst_t * pPipe = (PipeInst_t *) lvpParam;
	InitMsg_t *InitMsg;
	HANDLE hPipe = pPipe->hPipeInst;
	char msgbuffer[132];

	//ADDING TO INACTIVE QUEUE

	EnterCriticalSection(&g_PipeInstQueueMutex);
	pPipe->myThreadId = GetCurrentThreadId();
	ListAppend((void **) &g_PipeInstQueue, (void *) pPipe);
	LeaveCriticalSection(&g_PipeInstQueueMutex);
	
	//exchange the initial handshake.
	fSuccess = ReadFile(hPipe,					// handle to pipe          
						pPipe->chBuf,			// buffer to receive data  
						pPipe->xferBlockSize,	// size of buffer          
						&pPipe->cbRead,			// number of bytes read    
						NULL);					// not overlapped I/O      
		
//	if (! fSuccess || &pPipe->cbRead == 0 || pPipe->cbRead < sizeof(RqHdr_t) ) {
// LSL 000423 Boolean expression is always true - not what we wanted
	if (! fSuccess || pPipe->cbRead == 0 || pPipe->cbRead < sizeof(RqHdr_t) ) {
		//we have a serious error
		erc = GetLastError();
		if (erc & 0xc0000000)
			DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		else
			DEBUG_LOG((char *)__FILE__,__LINE__, erc);
		fConnect = FALSE;
	}


	if (fConnect) {
		//Write the reply to the pipe.
		InitMsg = (InitMsg_t *) pPipe->chBuf;

		if ((pPipe->cbRead < sizeof(InitMsg_t)) || (InitMsg->initSignature != E_SIGNATURE)) {
			sprintf(msgbuffer,"Invalid connect message.  Length %d, Signature %X\n", pPipe->cbRead, InitMsg->initSignature);
			OutputDebugString(msgbuffer);
			WriteLogData(msgbuffer, 0);
			fConnect = FALSE;
		}

		pPipe->processIdOfClient = InitMsg->processId;
		pPipe->macAddress = InitMsg->macAddress;
		
		sprintf(msgbuffer,"Pipe %x connected. MacAddress %I64X, Process %X, Thread %X, UserNum %X\n",
			pPipe, pPipe->macAddress, pPipe->processIdOfClient, pPipe->myThreadId, pPipe->hPipeInst );
		OutputDebugString(msgbuffer);
		WriteLogData(msgbuffer, 1);

		if (InitMsg->RqHdr.rqCode == RQ_REMOTE_TERMINATE)
			pPipe->dwState = REMOTE_TERMINATING_STATE;
		else
			pPipe->dwState = WRITING_STATE;

		InitMsg->RqHdr.ercRet = E_SIGNATURE;
		
		// LSL 000512 Server will now pass back the client his unique user number - the pipe handle
		InitMsg->RqHdr.userNum = (DWORD) pPipe->hPipeInst;
		pPipe->userNum = InitMsg->RqHdr.userNum;
		// LSL 000512 End change

		pPipe->cbToWrite = pPipe->cbRead;

		fSuccess = WriteFile(hPipe,				// handle to pipe           
							 pPipe->chBuf,		// buffer to write from     
							 pPipe->cbToWrite,	// number of bytes to write   
							 &pPipe->cbRead,	// number of bytes written   
							 NULL);			    // not overlapped I/O        
		
		if (! fSuccess || pPipe->cbRead != pPipe->cbToWrite) {
			//we have a serious error
			erc = GetLastError();
			if (erc & 0xc0000000)
				DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
			else
				DEBUG_LOG((char *)__FILE__,__LINE__, erc);
			fConnect = FALSE;
		}
		if (pPipe->dwState == REMOTE_TERMINATING_STATE)
			fConnect = FALSE;
	}

	
	while (fConnect) {
		// Read client requests from the pipe.  
		pPipe->dwState = READING_STATE;
		
		// LSL 981022 This ReadFile has failed in the past. How to handle??
		fSuccess = ReadFile(hPipe,					// handle to pipe          
							pPipe->chBuf,			// buffer to receive data  
							pPipe->xferBlockSize,	// size of buffer          
							&pPipe->cbRead,			// number of bytes read    
							NULL);					// not overlapped I/O      
		//if (! fSuccess || &pPipe->cbRead == 0) {
// LSL 000423 Boolean expression is always true - not what we wanted
		if (! fSuccess || pPipe->cbRead == 0) {
			//we have a serious error
			erc = GetLastError();
			// LSL 981022 Why & 0xc0000000?!?!?!? Never Happen?
			if (erc & 0xc0000000)
				DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
			else
				DEBUG_LOG((char *)__FILE__,__LINE__, erc);
			fConnect = FALSE;
			break; //out of the "while"
		}
		
		pPipe->dwState = HOLD_FOR_RESPONSE_STATE;
		
		//wake up the worker process
		PipeWakeupServer(pPipe);	
				
		//now sleep till the worker is done
		
		if ((WaitForSingleObject(pPipe->respEvent, INFINITE)) != WAIT_OBJECT_0) {
			//serious
			erc = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		}

		if (pPipe->dwState == TERMINATED_STATE) {
			fConnect = FALSE;
			break;
		}
		
		pPipe->dwState = WRITING_STATE;
		//Write the reply to the pipe.
		
		fSuccess = WriteFile(	hPipe,				// handle to pipe           
								pPipe->chBuf,		// buffer to write from     
								pPipe->cbToWrite,	// number of bytes to write   
								&pPipe->cbRead,		// number of bytes written   
								NULL);			    // not overlapped I/O        
		
		if (! fSuccess || pPipe->cbRead != pPipe->cbToWrite) {
			//we have a serious error
			erc = GetLastError();
			if (erc & 0xc0000000)
				DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
			else
				DEBUG_LOG((char *)__FILE__,__LINE__, erc);
			fConnect = FALSE;
		}
	} // while fConnect
	
	/* Flush the pipe to allow the client to read the pipe's contents before disconnecting.
	   Then disconnect the pipe, and close the handle to this pipe instance.
	*/ 
	
	FlushFileBuffers(hPipe);

	if (pPipe->dwState != TERMINATED_STATE) {
		if (pPipe->dwState != REMOTE_TERMINATING_STATE)
			pPipe->dwState = TERMINATING_STATE;

		PipeWakeupServer(pPipe);	//now sleep till the worker is done
		if ((WaitForSingleObject(pPipe->respEvent, INFINITE)) != WAIT_OBJECT_0) {
			//serious
			erc = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		}
	}
	
	// LSL 000428 Enter critical section before closing pipe and event
	EnterCriticalSection(&g_PipeInstQueueMutex);

	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);

#ifdef OldAndCrusty
	PipeInst_t *pQueuePipe = (PipeInst_t *) ListCmpRemove(	(void **) &g_PipeInstQueue,
															ParseQueueForUserNum,
															(void *) &pPipe->userNum);
#else
	PipeInst_t *pQueuePipe = (PipeInst_t *) ListCmpRemove(	(void **) &g_PipeInstQueue,
															ParseQueueMe,
															(void *) pPipe->Me);
#endif

	CloseHandle(pPipe->respEvent); //find a better way to do this
	pPipe->respEvent = NULL;	// LSL 000428 Set to NULL after closing

	LeaveCriticalSection(&g_PipeInstQueueMutex);
	//sanity
	if (pPipe != pQueuePipe) //inconsistency
		DEBUG_BREAK((char *)__FILE__, __LINE__, ercInconsistency);

	if (pPipe->chBuf) {
		free(pPipe->chBuf);
		pPipe->chBuf = NULL;
		}
	
	sprintf(msgbuffer,"Pipe %x terminated. MacAddress %I64X, PID %X, TID %X, UID %X\n",
		pPipe, pPipe->macAddress, pPipe->processIdOfClient, pPipe->myThreadId, pPipe->userNum );
	OutputDebugString(msgbuffer);
	WriteLogData(msgbuffer, 1);

	free(pPipe);
	pPipe = NULL;
	return (0);
}

unsigned __stdcall PipeOfferThread(LPVOID lvpParam)
{
	PipeInst_t *pPipe=NULL;
	DWORD dwErr;
	BOOL fConnect;
	HANDLE hThread;
	unsigned int dwThreadId;

	//LIMS 12/March
#ifdef GRANT_ALL
	SECURITY_ATTRIBUTES *pSA = GetPipeSecurityDesc();
#endif
	while(TRUE) {
		
		pPipe = (PipeInst_t *) zmalloc(sizeof(PipeInst_t));
		
		pPipe->Me = pPipe;

		pPipe->xferBlockSize = XFER_BLOCK_SIZE;

		pPipe->chBuf = (CHAR *) zmalloc(pPipe->xferBlockSize);

		
		pPipe->hPipeInst = CreateNamedPipe(	g_lpszPipename,            // pipe name           
											PIPE_ACCESS_DUPLEX,      // read/write access   
											// LSL 990315 SCR#557 Remove FILE_FLAG_OVERLAPPED 
											//| FILE_FLAG_OVERLAPPED 
											PIPE_TYPE_MESSAGE |      // message-type pipe   
											PIPE_READMODE_MESSAGE |  // message-read mode   
											PIPE_WAIT,               // blocking mode       
											PIPE_UNLIMITED_INSTANCES,// number of instances 
											pPipe->xferBlockSize,    // output buffer size  
											pPipe->xferBlockSize,    // input buffer size   
											PIPE_TIMEOUT,            // client time-out     
#ifdef GRANT_ALL
											pSA);					 //grant all
#else
											NULL );		             // no security attr.   
#endif
#ifdef GRANT_ALL //added 20/May LIMS
		//free(pSA); Maybe creating problems. Needs to be deallocated
					 //at the clean up of this thread.
#endif

		
		if ((pPipe->hPipeInst) == INVALID_HANDLE_VALUE) {
			//serious error
			dwErr = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
		}

		pPipe->respEvent = CreateEvent(	NULL,  // no security attr.
										FALSE, // manual-reset event
										FALSE, // initial state = signaled
										NULL   // unnamed event object
									  );
		
		if (pPipe->respEvent == NULL) {
			//serious error
			dwErr = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
		}
		
		fConnect = ConnectNamedPipe(pPipe->hPipeInst, NULL); 
		if (!fConnect) {
			dwErr = GetLastError();
			/* LSL 000503 If GetLastError == ERROR_PIPE_CONNECTED then
			a client connection request arrived between our call to 
			CreateNamedPipe above and ConnectNamedPipe(). We can proceed.
			*/
			if (dwErr != ERROR_PIPE_CONNECTED) {
				DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
				}
		}
	
		hThread = (HANDLE) _beginthreadex( NULL,           // no security attr.
								0,              // default stack size
								PipeInstanceThread, 
								(LPVOID) pPipe,	// thread parameter
								0,              // not suspended
								&dwThreadId);   // returns thread ID
			
/*			
					CreateThread( NULL,           // no security attr.
								0,              // default stack size
								(LPTHREAD_START_ROUTINE) PipeInstanceThread, 
								(LPVOID) pPipe,	// thread parameter
								0,              // not suspended
								&dwThreadId);   // returns thread ID
*/		
		if (hThread == INVALID_HANDLE_VALUE)  {
			//serious error
			dwErr = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
		}

		// LSL 981013  _beginthreadex() doesn't close the handle 
		CloseHandle(hThread);
	}
	
}
 
unsigned __stdcall PipeServer(LPVOID pUnused) {
	
	WORD i; 
	DWORD dwErr;
	HANDLE hThread;
	unsigned int dwThreadId;
	PipeInst_t *pPipeInst = NULL;
	InternalMsg_t *pMsg = NULL;  // LSL 980721 SCR# 11 init to NULL	
	// -----------------
	PipeInst_t *pPipe = NULL;
	DWORD waitRet;
	InternalMsg_t *pRet = NULL;


	for (i=0; (i<H_MAX-1); i++) {
		g_hEvents[i] = CreateEvent(	NULL,  // no security attr.
									FALSE, // manual-reset event
									FALSE, // initial state = signaled
									NULL   // unnamed event object
								 );
		
		if (g_hEvents[i] == NULL) {
			//serious error
			dwErr = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
		}

	}
	
	g_TerminateEvent = g_hEvents[H_TERMINATE];
	g_RespondToPipeEvent = g_hEvents[H_RESPONDTOPIPE];
	g_WorkerWakeupEvent = g_hEvents[H_WORKERWAKEUP];
	g_WaitableTimer = CreateWaitableTimer(
						NULL,	// pointer to security attributes
						false,	// flag for manual reset state
						NULL	// pointer to timer object name
						);
	if (g_WaitableTimer == NULL) {
		//serious error
		dwErr = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
	}
	else
	{

		__int64         qwDueTime;
		LARGE_INTEGER   liDueTime;

        // Create a negative 64-bit integer that will be used to
        // signal the timer 5 seconds from now.
        qwDueTime = -5 * 10000000;

        // Copy the relative time into a LARGE_INTEGER.
        liDueTime.LowPart  = (DWORD) ( qwDueTime & 0xFFFFFFFF );
        liDueTime.HighPart = (LONG)  ( qwDueTime >> 32 );

		if (!SetWaitableTimer(
				g_WaitableTimer,		// handle to a timer object
				&liDueTime,				// when timer will become signaled
				5000,					// periodic timer interval
				NULL,					// completion routine
				NULL,					// data for completion routine
				false					// flag for resume state
				)) {
			//serious error
			dwErr = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
		}
		else {
			g_hEvents[H_TIMER] = g_WaitableTimer;
		}
	}

	InitializeCriticalSection(&g_PipeMsgQueueMutex);
	InitializeCriticalSection(&g_PipeInstQueueMutex);
	InitializeCriticalSection(&g_RespondQueueMutex);
	
	hThread = (HANDLE) _beginthreadex
							( NULL,           // no security attr.
							0,              // default stack size
							PipeOfferThread, 
							(LPVOID) 0	,	// thread parameter
							0,              // not suspended
							&dwThreadId);   // returns thread ID

	// LSL 981013  _beginthreadex() doesn't close the handle 
	CloseHandle(hThread);
	
	SetEvent(hEventServiceSync); //the InitService() is blocked on this


	while (1) {
		
	
// ***************** GetInternalMessage() ...unrolled ***********************************
/*
  This is so we get the respondToPipe Messages and Control Messages on a priority basis.
  This wait loop would get the messages for the ControlEvent or RespondToPipeEvent.
  TO_DO: Note that RespondToPipe Message is no longer processed from here, so it will
  not be necessary any more.
  We come out of this loop immediately, and then go in with iEvent=H_WORKERWAKEUP;
  this forces us to frisk the WakeupQueue.


	HANDLE g_hEvents[H_MAX] = {
		g_TerminateEvent,
		g_RespondToPipeEvent,
		g_WorkerWakeupEvent };
*/

	waitRet = WaitForMultipleObjects(H_MAX, g_hEvents, FALSE, WATCHDOG_INTERVAL*1000);
									
	if (WAIT_FAILED == waitRet) { 
		dwErr = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
		}

	if (WAIT_TIMEOUT == waitRet) {
		// this shouldn't happen due to the use of our timer
		ProcessTimer(FALSE);
		waitRet = H_WORKERWAKEUP;
	} 
	else
		waitRet -= WAIT_OBJECT_0;
	
	switch (waitRet) {

		case H_TERMINATE:
			if(pMsg) {free(pMsg);pMsg=NULL;}
			for (i=0; (i<H_MAX); i++) {
				CloseHandle(g_hEvents[i]);
				}
			DeleteCriticalSection(&g_PipeMsgQueueMutex);
			DeleteCriticalSection(&g_PipeInstQueueMutex);
			DeleteCriticalSection(&g_RespondQueueMutex);
			return 0;
			break;
			
		case H_RESPONDTOPIPE:
			//we are signalling the Event at which the respective PipeInstance is
			//waiting directly, see the main-loop for the pipe server
			break;

		case H_WORKERWAKEUP:
		
			EnterCriticalSection(&g_PipeMsgQueueMutex);
			//sprintf(msgbuffer,"MessageQueue Count = %lu\n", ListCount((void **) &g_PipeMsgQueue) );
			//OutputDebugString(msgbuffer);
			pPipe = (PipeInst_t *) ListRetrieve((void **) &g_PipeMsgQueue);
			LeaveCriticalSection(&g_PipeMsgQueueMutex);

			while(pPipe != NULL) {
				//now add this to the inactive Queue; Add to PipeInstQueue
				EnterCriticalSection(&g_PipeInstQueueMutex);
				ListAppend((void **) &g_PipeInstQueue, pPipe);
				LeaveCriticalSection(&g_PipeInstQueueMutex);

				if(pPipe->dwState == REMOTE_TERMINATING_STATE) 
					TerminatePipeProcess(pPipe);
				else if(pPipe->dwState ==	TERMINATING_STATE) TerminatePipeUser(pPipe);
				else {			
					// --------------------------------------
					// ProcessMessage returns TRUE if we need to respond now
					if (ServiceRequest((RqHdr_t *) pPipe->chBuf)) { 
						pPipe->cbToWrite = pPipe->cbRead;	// cbBytes is ALWAYS zero!
						
						pPipe->dwMsgs++;
						// !!!!!!!!!!!!!!! BUG ALERT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
						// LSL 000425 Several customers have reported crashes on line below!!!!!!
						if (!(SetEvent(pPipe->respEvent))) {
							//serious error
							DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
						}
						//free(pMsg); //Do it in both immediate response and delayed response
					} 
				} // end else

				Sleep(0);
				
				EnterCriticalSection(&g_PipeMsgQueueMutex);
				//sprintf(msgbuffer,"MessageQueue Count = %lu\n", ListCount((void **) &g_PipeMsgQueue) );
				//OutputDebugString(msgbuffer);
				pPipe = (PipeInst_t *) ListRetrieve((void **) &g_PipeMsgQueue);
				LeaveCriticalSection(&g_PipeMsgQueueMutex);

			}  // end while(pPipe != NULL)
			break;
		case H_TIMER:
			ProcessTimer(FALSE);
			break;
		default:
			dwErr = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, dwErr);
			break;
		} //switch
	} //while 1

	return 0;
	
	
// ******** end GetInternalMessage() ******************************************

/*
	#ifdef _DEBUG
		Sleep(0);
	#endif
	
	
	switch (pMsg->msgType) {

		case M_TERMINATE:
			if(pMsg) {free(pMsg);pMsg=NULL;}
			for (i=0; (i<H_MAX); i++) {
				CloseHandle(g_hEvents[i]);
				}
			DeleteCriticalSection(&g_PipeMsgQueueMutex);
			DeleteCriticalSection(&g_PipeInstQueueMutex);
			DeleteCriticalSection(&g_RespondQueueMutex);
			return 0;
			break;

		case M_WORKERWAKEUP:
			pPipeInst = (PipeInst_t *) pMsg->p;
			// ProcessMessage returns TRUE if we need to respond now
			if (ProcessMessage(pMsg, pMsg->pMsg, &cbBytes)) { 
				if (cbBytes == 0)
					pPipeInst->cbToWrite = pPipeInst->cbRead;	// cbBytes is ALWAYS zero!
				else
					pPipeInst->cbToWrite = cbBytes;
				pPipeInst->dwMsgs++;
				// !!!!!!!!!!!!!!! BUG ALERT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				// LSL 000425 Several customers have reported crashes on line below!!!!!!
				if (!(SetEvent(pPipeInst->respEvent))) {
					//serious error
					DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
				}
				//free(pMsg); //Do it in both immediate response and delayed response
			} else
				// respond later
				;

			//free(pMsg);
			break;

		
		default:
			//why?
			DEBUG_BREAK((char *)__FILE__,__LINE__, 0xb00b);
			break;
		} //switch
*/
  /*If user requests are coming in at a rate greater than the wakeup
	interval, we will never see a timeout wakeup message. So call 
	ProcessTimer() at the end of each work cycle, and let tbe ProcessTimer()
	function decide if the interval has elapsed.
	*/
/*
	ProcessTimer(TRUE);
*/	

}

void TerminatePipeUser(PipeInst_t *pPipeInst) {
	TerminateUser(pPipeInst->userNum);
	if (!(SetEvent(pPipeInst->respEvent))) {
		//serious error
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
	}
}

void TerminatePipeProcess(PipeInst_t *pPipeInst) {
	BOOL fTerminating;
	TerminateStruc_t terminate;

	terminate.processId = pPipeInst->processIdOfClient;
	terminate.macAddress = pPipeInst->macAddress;
	TerminateUser(pPipeInst->userNum);

	if (!(SetEvent(pPipeInst->respEvent))) {
		//serious error
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
	}
	//now terminate all the threads that belong to this user that are on
	//the pPipeInstQueue and have a dwState == HOLD_FOR_RESPONSE_STATE
	fTerminating = TRUE;
	while(fTerminating) {
		pPipeInst = NULL;
		fTerminating = TRUE;
		EnterCriticalSection(&g_PipeInstQueueMutex);
		pPipeInst = (PipeInst_t *) ListCmpRemove((void **) &g_PipeInstQueue, 
												 CmpStateAndProcess,
												 (void *) &terminate);
		if (pPipeInst) {
			TerminateUser(pPipeInst->userNum);
			pPipeInst->dwState = TERMINATED_STATE;
			ListAppend((void **) &g_PipeInstQueue, (void *) pPipeInst);
			if (!(SetEvent(pPipeInst->respEvent))) {
				//serious error
				DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
			}
		} //pPipeInst
		else
			fTerminating = FALSE;
		LeaveCriticalSection(&g_PipeInstQueueMutex);
		}
	}


DWORD RespondToPipe(DWORD userNum)
{
	PipeInst_t *pPipe;

	//DO WE NEED TO PROTECT THIS BY A MUTEX? TO_DO
	// LSL 990802 Add Critical Section
	EnterCriticalSection(&g_PipeInstQueueMutex);
	
	pPipe = (PipeInst_t *) ListScan((void **) &g_PipeInstQueue,
									ParseQueueForUserNum,
									(void *) &userNum);

	//sanity
	if (!pPipe) {
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
	}

	pPipe->cbToWrite = pPipe->cbRead;  // LSL 990513 - BOH suspected line of crash #1
	pPipe->dwMsgs++;
	if (!(SetEvent(pPipe->respEvent))) {
		//serious error
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
	}
	LeaveCriticalSection(&g_PipeInstQueueMutex);
	return(ercOk);
}
 