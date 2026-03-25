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
'    Module       :  AFD QueueManager Client (AfdAmClient.dll)
'    Date         :  March 1998
'
'    File Name    :  BuildRq.c
'
'    Purpose      :  This file has the code for constructing the RequestBlocks
'                    for all the AFDQueueManager APIs.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

/*
	The Request Builder for the AFD project.

*/
#include <stdio.h>
#include <windows.h>
#include "build.h" 
#include "Req.h"
#include "QMErcs.h"

static char ModId[] = {__FILE__}; //for debugging


#pragma pack (2)

typedef struct {
	WORD Type;
	WORD Off;
} Item_t;

typedef struct {
	WORD rqCode;
	WORD sCntlInfo;
	WORD nReq;
	WORD nResp;
	WORD nItems;
	Item_t Items;
} Fmt_t;

#pragma pack()

void DEBUG_BREAK(char *ModId, int lineNo, DWORD erc);
extern DWORD TransferToRemote(HANDLE hPipe, void *p, DWORD c);
extern HANDLE GetConnection(void);
extern ThreadInfo_t *GetThreadInfo();

/*TO_DO
In FmtMessgaeBuf() 
- CHECK FOR NULL POINTERS>>>> 
- all the T_POINTER needes to be re-examined, I have made the desired changed for other
- Do we need a check to make sure that the MsgSize does not exceed XFER_BLOCK_SIZE
*/

DWORD FmtMessageBuf(char *p, WORD *format, char *args)
{
	WORD i;
	pbcb_t *pPbCb;

	WORD *pW;
	LONG *pL;
	DWORDLONG *pD;
	pbcb_t *pRqPbCb;

	CHAR *Msg;
	ThreadInfo_t *pThread;

	RqHdr_t *pRq = (RqHdr_t *) p;
	Fmt_t *pFmt = (Fmt_t *) format;
	WORD nItems = pFmt->nItems;
	Item_t *Item = &pFmt->Items;
	WORD cb = sizeof(RqHdr_t);

	cb += pFmt->sCntlInfo + (pFmt->nReq + pFmt->nResp) * sizeof(pbcb_t); 
												//now get the size of the request block

	// LSL 00512 Read the UserNumber that is stored in TLS 
	pThread = (void *) GetThreadInfo();
	pRq->rqCode = pFmt->rqCode;
	pRq->sCntlInfo = pFmt->sCntlInfo;
	pRq->nReq = pFmt->nReq;
	pRq->nResp = pFmt->nResp;
	// This should be the usernumber that server returned to us. Not zero!
	pRq->userNum = pThread->userNumber;	
	pRq->ercRet = 0xa5a5a5a5; //change this to zero later

	Msg = p + cb;

	for(i=0; (i<(pFmt->nItems)); (i++, Item++)) {
		switch(Item->Type) {

		case T_WORD:
			pW = (WORD *) &(p[Item->Off]);
			*pW = *(WORD *) args;
			//note the stack always has a dword pushed, so increment by 4
			//((WORD *) args)++;
			((LONG *) args)++;
			break;

		case T_LONG:
			pL = (LONG *) &(p[Item->Off]);
			*pL = *(LONG *) args;
			((LONG *) args)++;
			break;

		case T_DOUBLE:
			pD = (DWORDLONG *) &(p[Item->Off]);
			*pD = *(DWORDLONG *) args;
			((DWORDLONG *) args)++;
			break;

		case T_SYSTIME:
			pRqPbCb = (pbcb_t *) &(p[Item->Off]);
			//put in relative address here
			//pRqPbCb->p = (void *) Msg;
			pRqPbCb->p = 0; 
			if (*(void **)args) {
				pRqPbCb->p = (void *)(Msg - p);
				memcpy(Msg, *(void **) args, sizeof(SYSTEMTIME));
				pRqPbCb->c = (WORD) (sizeof(SYSTEMTIME));
				cb += sizeof(SYSTEMTIME);
				Msg += sizeof(SYSTEMTIME);
			}

			((void **) args)++;
			break;

		case T_POINTER_REQ:
		case T_POINTER_RESP:
			pRqPbCb = (pbcb_t *) &(p[Item->Off]);
			//put in relative address here
			//pRqPbCb->p = (void *) Msg;
			pRqPbCb->p = (void *)(Msg - p);
			//special case: the following Item has to be a T_WCONST
			Item++; i++;
			if (Item->Type != T_WCONST) {
				//why
				DEBUG_BREAK((char *)__FILE__,__LINE__, 0xb00b);
			}

			pRqPbCb->c = Item->Off;
			if (*(void **)args && pRqPbCb->c != 0) 
				memcpy(Msg, *(void **) args, pRqPbCb->c);
			//do not copy if resp.
			((void **) args)++;
			cb += pRqPbCb->c;
			Msg += pRqPbCb->c;
			break;


		case T_PBCB_REQ:
		case T_PBCB_RESP:
			pPbCb = (pbcb_t *) args;
			pRqPbCb = (pbcb_t *) &(p[Item->Off]);
			//put in relative address here
			//pRqPbCb->p = Msg;
			pRqPbCb->p = (void *) (Msg - p);
			pRqPbCb->c = pPbCb->c;
			if (pPbCb->p && pPbCb->c != 0) {
				if (Item->Type == T_PBCB_REQ) //no need to copy the resp. items
					memcpy(Msg, pPbCb->p, pPbCb->c);
			} //pb and cb are not null
			else {
				pPbCb->p = 0; //NULL pointer
			}

			//note: when pbcb is pushed on the stack, the cb is pushed as dword
			//((pbcb_t *) args)++;
			((LONG *) args)++; ((LONG *) args)++;
			cb += ((pPbCb->c + 3) & 0xfffc); //round it off to the next dword
			Msg += ((pPbCb->c + 3) & 0xfffc); //round it off to the next dword
			break;


		default:
			//why
			DEBUG_BREAK((char *)__FILE__,__LINE__, 0xb00b);
			break;

		} //switch
	} //for
	
	pRq->cbMsg = cb;

	return(cb);
}

/*TO_DO
When a PBCB is NULL make sure you do not copy it into the ReqBlock.
*/
void RestoreMessageBuf(char *p, WORD *format, char *args)
{
	WORD i;
	DWORD off;

	pbcb_t *pPbCb;
	pbcb_t *pRqPbCb;

	RqHdr_t *pRq = (RqHdr_t *) p;
	Fmt_t *pFmt = (Fmt_t *) format;
	WORD nItems = pFmt->nItems;
	Item_t *Item = &pFmt->Items;


	for(i=0; (i<(pFmt->nItems)); (i++, Item++)) {
		switch(Item->Type) {

		case T_WORD:
			((LONG *) args)++;
			break;

		case T_LONG:
			((LONG *) args)++;
			break;

		case T_DOUBLE:
			((DWORDLONG *) args)++;
			break;

		case T_SYSTIME:
			((void **) args)++;
			break;

		case T_POINTER_REQ:
			((void **) args)++;
			break;

		case T_POINTER_RESP:
			pRqPbCb = (pbcb_t *) &(p[Item->Off]);

			//we have a relative address, convert it to absolute
			off = (DWORD ) (pRqPbCb->p);
			pRqPbCb->p = (void *)((char *)pRq + off);

			memcpy(*(void **) args, pRqPbCb->p, pRqPbCb->c);

			((void **) args)++;
			break;


		case T_PBCB_REQ:
			((LONG *) args)++; ((LONG *) args)++;
			break;

		case T_PBCB_RESP:
			pPbCb = (pbcb_t *) args;
			pRqPbCb = (pbcb_t *) &(p[Item->Off]);

			//we have a relative address, convert it to absolute
			off = (DWORD ) (pRqPbCb->p);
			pRqPbCb->p = (void *)((char *) pRq + off);
			memcpy(pPbCb->p, pRqPbCb->p, pRqPbCb->c);

			((LONG *) args)++; ((LONG *) args)++;
			break;

		case T_WCONST:
			break;

		default:
			//why
			DEBUG_BREAK((char *)__FILE__,__LINE__, 0xb00b);
			break;

		} //switch
	} //for
	
}

DWORD RqInterface(WORD *format, ...)
{
	va_list ap;
	HANDLE hPipe;
	DWORD cbXfer;
	DWORD erc;
	// LSL 990315 SCR#555 Don't use static storage in multithreaded DLL!
	// static void *pMsgBuff=NULL;		
	// LSL- Q: Use THREADLOCAL void *pMsgBuff instead (LSL)?
	// LSL- A: Doesn't seem to work with VB6 - fine with VC
	char pMsgBuff[XFER_BLOCK_SIZE];		

	va_start(ap, format);

	hPipe = GetConnection();

	if (!hPipe) {
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		return(ercNoPipeConnection);
	}
/******* LSL 990315 SCR#555 Comment this section. Storage is now allocated on the stack.
	if (!pMsgBuff) {
		if (!(pMsgBuff = malloc(XFER_BLOCK_SIZE))) {
			//why?
			DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
			// LSL Q: What if the memory allocation for pMsgBuff fails!?!?!?!?!?!
			// LSL A: return GetLastError();
			// CyberTech: A: Keep going with NULL pointer and hope for the best.
		}
	}
********/ // LSL end
	//for debugging

	memset((void *)pMsgBuff, 0x5a, XFER_BLOCK_SIZE); 
	cbXfer = FmtMessageBuf((char *)pMsgBuff, format, ap);

	//TO_DO send out only the requisite size
	if ((erc = TransferToRemote(hPipe, (void *)pMsgBuff, cbXfer)) != 0) {
extern DWORD TlsIndex;
extern BOOL ClosePipeConnection(HANDLE hPipe);
		ThreadInfo_t *pThread;

		if(TlsIndex != TLS_OUT_OF_INDEXES) {
		
			pThread = (void *) TlsGetValue(TlsIndex);

			if (pThread) {

				if ((void *) pThread == pThread->p) {
					//this is a BOOL, we may need to act on a FALSE
					ClosePipeConnection(pThread->hPipe);
					printf("TransferToRemote error: pThread is %4X, ProcessId %4X, ThreadId %4X, User %4X\n",
							pThread->p, pThread->processId, pThread->threadId, pThread->userNumber);
					pThread->hPipe = NULL;
				}
			}
		}

		return (erc);
	}

	RestoreMessageBuf((char *)pMsgBuff, format, ap);

	return (((RqHdr_t *) pMsgBuff)->ercRet); 
}

