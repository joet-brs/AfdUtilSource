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
'    File Name    :  QMQueueObj.cpp
'
'    Purpose      :  This file has the code for all the QueueManager queuing,
'                    scheduling, storing on disk, etc. functions for the
'                    AfdQueueManager.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
9/Jul	AA	Added function RemoveKeyedQueueEntry().
			Fixed errors in the function RemoveQueueEntry().
5/Aug	AA	Fixed error in H_RewriteMarkedQueueEntry().
17/Aug/1998	AA	Added Larry's changes in UpdateQueueInformation(). LIMS
*/

#include "basic.h"
#include <stddef.h>
#include "build.h"
#include "RqStruct.h"
#include <AfdMisc.h>
#include "QMErcs.h"
#include "Trace.h"
#include <stdio.h>

static char *ModId[]={__FILE__};

#define MAXQUEUESALLOWED  256
#define MAXSERVERS  64
#define MAXSERVERSLESS1  63
#define MAXQUEUEDSERVERS  64
#define MAXQUEUEDSERVERSLESS1  63

#define MAX_ENTRIES_QUEUE 4096

#define EXTEND_QUANTUM 10

#define Q_VERSION 0x41414141
#define PAGE_SIZE 512
#define QE_SIGNATURE 0x534d494c

extern BOOL WriteLogData(char *data, DWORD level = 0);

#pragma pack(2)

typedef struct QueueEntry_s {
	QueueEntry_s *Next;
	DWORD signature;
	WORD priority;
	ctosDateTimeType dateTime;
	WORD repeatInterval;
	DWORD eh;
	DWORD markUserNum;
	WORD iEntry;
	WORD iNext;
	WORD iPrev;
	DWORD padding;
	WORD cEntry;			//not really required
	BYTE rgEntry[1];
} QueueEntry_t;


typedef struct QueueHeader_s {
	QueueHeader_s *Next; //4
    DWORD version;  //8
    WORD queueType;	 //10		
	WORD cbQueueName; //12
	char QueueName[52]; //64
	WORD cbFileName; //66
	char FileName[92]; //158
	DWORD sEntrySize; //162
	DWORD fileSize; //166
	WORD cEntries; //168
	WORD iEntryFirst; //170
	WORD iEntryLast; //172
	//from here on the information is going to be built at Init;
	HANDLE qh;							//queuehandle allocated at run time
	DWORD *pEntryMap;					//a bit map indicating available entries
	WORD sEntryMap;
    BOOL fStable;									
    BOOL fUniqueSvr;			
    WORD cServers;						//number of servers for this queue
    DWORD rgQueueServers[MAXSERVERS];	//userNum of the Queue servers
	WORD cQueuedRequests;
    RqMarkNextQueueEntry_t *rgQueuedRequests[MAXQUEUEDSERVERS]; //Mark...() rqs pending
	QueueEntry_t *pMarkedQueues;
	WORD cActiveEntries;
	QueueEntry_t *pActiveQueues;
	WORD cTimedEntries;					//number of timed cQDEntries
	QueueEntry_t *pTimedQueues;
	WORD nFetches;						//TO_DO, add this
    ctosDateTimeType EarliestTime;			
	//size = 730
	char padding[294];					//to bring it up to 1024 bytes
} QueueHeader_t;

#define EH_ACTIVEQUEUEMASK 0x80000000
#define EH_TIMEDQUEUEMASK  0x40000000
#define EH_MARKEDQUEUEMASK 0x20000000
#define E_NOVALIDENTRY 0x1ffff

typedef struct {
	DWORD qh;
	DWORD eh;
} QEH_t;

typedef struct {
	QEH_t qeh;
	WORD priority;
	DWORD serverUserNum;
	QEH_t qehNext;
} QueueEntryStatus_t;

typedef struct {
	WORD cQueueFetches;
	WORD nQueues;
	BYTE rgQueues[1];
} QueueStatus_t;

typedef struct {
	HANDLE qh;
	WORD cEntries;
	WORD cbQueueName;
	BYTE QueueName[50];
	DWORD sEntrySize;
	BOOL fContaminated;
} QueueInfo_t;


#pragma pack()
extern WORD WATCHDOG_INTERVAL;
QueueHeader_t *g_pQueues=NULL;


BOOL PutEntry(HANDLE fh, void *pBuff, DWORD sBuff, DWORD lfa);
BOOL GetEntry(HANDLE fh, void *pBuff, DWORD sBuff, DWORD lfa);
BOOL AddQueueEntryToDisk(QueueHeader_t *pQ, QueueEntry_t *pEntry);
void RespondToQueuedMarkRequests(QueueHeader_t *pQ);
extern DWORD Respond(RqHdr_t *pRq);	


int CmpQueueName(void *Item, void *pCmp) {

	char *pQueueName = ((QueueHeader_t *)Item)->QueueName;

	return ((_stricmp(pQueueName, (const char *) pCmp))? FALSE : TRUE);
}

int CmpFileName(void *Item, void *pCmp) {

	char *pFileName = ((QueueHeader_t *)Item)->FileName;

	return ((_stricmp(pFileName, (const char *) pCmp))? FALSE : TRUE);
}

int CmpQEH(void *Item, void *ref) {

	return (((QueueEntry_t *)Item)->eh == *(DWORD *)ref);
}


int CmpQH(void *Item, void *ref) {

	return (((QueueHeader_t *)Item)->qh == *(HANDLE *)ref);
}

int CmpQueueType(void *Item, void *ref) {

	return (((QueueHeader_t *)Item)->queueType == *(WORD *)ref);
}

int CmpMarkUserNum(void *Item, void *ref) {

	return (((QueueEntry_t *)Item)->markUserNum == *(DWORD *)ref);
}

#define Q_STATE_ERR 0
#define Q_STATE_NEW 1
#define Q_STATE_OLD 2
#define Q_STATE_MAX 3


BOOL CreateQueueHeader(HANDLE fh, WORD wEntrySize, WORD wQueueType, 
					   char *pbQueueName, char *pbFileSpec)
{
	BOOL fSuccess;
	QueueHeader_t QueueHeader;

	memset(&QueueHeader, 0, sizeof(QueueHeader_t));

    QueueHeader.version = Q_VERSION;
    QueueHeader.queueType = wQueueType;
	strcpy((char *)QueueHeader.QueueName, pbQueueName);
	QueueHeader.cbQueueName = strlen(pbQueueName);
	strcpy((char *)QueueHeader.FileName, pbFileSpec);
	QueueHeader.cbFileName = strlen(pbFileSpec);
	QueueHeader.sEntrySize = wEntrySize*PAGE_SIZE;
	QueueHeader.cEntries = 0;
	QueueHeader.fileSize = sizeof(QueueHeader_t);
	QueueHeader.iEntryFirst = 0xffff;
	QueueHeader.iEntryLast = 0xffff;


	fSuccess = PutEntry(fh, &QueueHeader, sizeof(QueueHeader), 0);
	if (!fSuccess) {
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		return (FALSE);
	}

	return (TRUE);
}

int CheckQueuePriority(void *prioQueue, void *prioNew) 
{

	return ((((QueueEntry_t *)prioQueue)->priority < *(WORD *)prioNew)? TRUE : FALSE);
}

QueueEntry_t *AddQueueEntryToList(void **pHead, QueueEntry_t *pQ) 
{
	void *p1;
	int insertIndex;

	insertIndex = ListScanIndexRet( pHead, CheckQueuePriority, 
									(void *)&(pQ->priority));

	p1 = ListInsert(pHead, (void *) pQ, insertIndex);

	//sanity 

	if (p1 != (void *) pQ) {
		DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
	}

	return ((QueueEntry_t *) p1);
}


QueueHeader_t * InitQueueHeader(HANDLE fh, WORD wEntrySize, WORD wQueueType, 
								char *pbQueueName, char *pbFileSpec)
{
	BOOL fSuccess;
	QueueHeader_t *pQ;

	pQ = (QueueHeader_t *) zmalloc(sizeof(QueueHeader_t));

	fSuccess = GetEntry(fh, pQ, sizeof(QueueHeader_t), 0);

	fSuccess =	(	(fSuccess) &&
					/*(cbRead == sizeof(QueueHeader_t)) &&*/
					(pQ->version == Q_VERSION) &&
					(pQ->sEntrySize == (DWORD) wEntrySize * PAGE_SIZE) &&
					(pQ->queueType == wQueueType) &&
					(_stricmp(pbQueueName, pQ->QueueName) == 0) &&
					(_stricmp(pbFileSpec, pQ->FileName) == 0)
				);
					

	if (!fSuccess) { //re-init the queue-file
		//need to warn the user???
		fSuccess = CreateQueueHeader(fh, wEntrySize, wQueueType, 
									 pbQueueName, pbFileSpec);
		if (!fSuccess) {
			DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
			free(pQ);
			pQ = NULL;
			return (NULL);
		}
		//now read back the info.

		fSuccess = GetEntry(fh, pQ, sizeof(QueueHeader_t), 0);

		if (!fSuccess) {
			//why?
			DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
			free(pQ);
			pQ = NULL;
			return(NULL);
		}

	} //re-init the queue-file


	pQ->qh = fh;

	memset(&pQ->fStable, 0, sizeof(QueueHeader_t) - offsetof(QueueHeader_t, fStable));

	return(pQ);
}


WORD CheckQueueFileState(char *pFileName, HANDLE *fh)
{
	DWORD erc;

	*fh = CreateFile(pFileName,					// pointer to name of the file 
					GENERIC_READ+GENERIC_WRITE,	// access (read-write) mode 
					0,							// share mode 
					0,							// pointer to security descriptor 
					OPEN_ALWAYS,				// how to create 
					FILE_ATTRIBUTE_NORMAL,		// file attributes 
					0							// handle to file with attributes to copy  
					);

	erc = GetLastError();
	if (*fh == INVALID_HANDLE_VALUE) {
		return (Q_STATE_ERR);
	}

	if (erc == ERROR_ALREADY_EXISTS)
		return (Q_STATE_OLD);
	else
		return (Q_STATE_NEW);

}

BOOL GetEntry(HANDLE fh, void *pBuff, DWORD sBuff, DWORD lfa)
{
	DWORD lfaRet, erc, cbRet;
	BOOL fSuccess;

	lfaRet = SetFilePointer(fh, lfa, NULL, FILE_BEGIN);
	if (lfaRet != lfa) {
		erc = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		return (FALSE);
	}

	fSuccess = ReadFile(fh, pBuff, sBuff, &cbRet, NULL);
	
	fSuccess = (fSuccess && (cbRet == sBuff));

	if (!fSuccess) {
		erc = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
	}

	return (fSuccess);
}

BOOL PutEntry(HANDLE fh, void *pBuff, DWORD sBuff, DWORD lfa)
{
	DWORD lfaRet, erc, cbRet;
	BOOL fSuccess;

	lfaRet = SetFilePointer(fh, lfa, NULL, FILE_BEGIN);
	if (lfaRet != lfa) {
		erc = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		return (FALSE);
	}

	fSuccess = WriteFile(fh, pBuff, sBuff, &cbRet, NULL);
	
	fSuccess = (fSuccess && (cbRet == sBuff));

	if (!fSuccess) {
		erc = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
	}

	return (fSuccess);
}


BOOL CheckIfQueueEntryValid(QueueEntry_t *pQ)
{
	BOOL fValid;

	//TO_DO make more solid checks
	fValid = (pQ->signature == QE_SIGNATURE);

	return(fValid);
}

void MarkEntry(QueueHeader_t *pQ, WORD iEntry, BOOL fUsed)
#define QE_UNUSED 0
#define QE_USED 1
{
	div_t iMap;
	DWORD dwMask=1;
	DWORD *pMap = pQ->pEntryMap;


	iMap = div(iEntry, 32); //1 bit indicates every entry, each dword has 32 bits

	dwMask<<=iMap.rem;

	if (fUsed)
		*(pMap+iMap.quot) |= dwMask;
	else
		*(pMap+iMap.quot) &= ~dwMask;
}

/*************************************************************************
GetFreeEntry()

Description: 
Searches a bitmap for a free entry (zero bit). Bitmap contains 
MAX_ENTRIES_QUEUE bits stored in an array (pEntryMap) containing
sEntryMap bytes (MAX_ENTRIES_QUEUE / 8).  The array is accessed using DWORDS.
The array contains sEntryMap / sizeof(DWORD) DWORD elements.

Returns: Index of first zero bit. 0 to MAX_ENTRIES_QUEUE-1
*************************************************************************/
WORD GetFreeEntry(QueueHeader_t *pQ)
{
	DWORD *pMap = pQ->pEntryMap;
	WORD sMap = pQ->sEntryMap;
	WORD iIndex, iBit;
	DWORD dwMask=1, dwEntry;

	// Find first DWORD in bitmap with a clear bit.
	for(iIndex=0;(iIndex<sMap/sizeof(DWORD)); iIndex++) {
		if ( *(pMap+iIndex) != 0xffffffff) //atleast one bit is 0
			break;
	}

	// LSL 000423 DOH!?!?!? sMap*32 ?!?!? Not good. We're way out of bounds!
	// if (iIndex == sMap*32) {
	// LSL 000423 Replace line above
	if (iIndex == sMap/sizeof(DWORD)) {
		// Map is full
		return(MAX_ENTRIES_QUEUE); 
	}

	dwEntry = *(pMap+iIndex);

	for(iBit=0;(iBit<32);iBit++) {
		if ((dwEntry & dwMask) == 0)
			break;
		dwMask<<=1;
	}

	if (iBit == 32) {
		DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
		return(MAX_ENTRIES_QUEUE);
	}

	return(iIndex*32 + iBit);
}


BOOL CheckInEntryInList(QueueHeader_t *pQ, QueueEntry_t *pQE)
{
	void *p;

	p = ListScan((void **) &pQ->pActiveQueues, CmpQEH, (void *) &pQE->eh);
	if (p)
		return(TRUE);
	p = ListScan((void **) &pQ->pTimedQueues, CmpQEH, (void *) &pQE->eh);
	if (p)
		return(TRUE);
	return(FALSE);
}


//TO_DO
//Update the pQ->iLastEntry field here


DWORD UpdateQueueInformation(QueueHeader_t *pQ)
{
	DWORD nMaxInFile, erc, sQueueHeader = sizeof(QueueHeader_t), lfa;
	QueueEntry_t *pQueueEntry=NULL, *pQueueEntryBuff=NULL, *p1;
	BOOL fSuccess, fValid, fTimeElapsed, fBrokenChain=FALSE, fNoLink=FALSE;
	WORD iEntry, iPrevEntry=0xffff, nEntries=0;

	pQ->pEntryMap = (DWORD *) zmalloc(pQ->sEntryMap = MAX_ENTRIES_QUEUE / 8); 
															//one bit per entry
	nMaxInFile =  (pQ->fileSize - sQueueHeader) / pQ->sEntrySize;

	if (nMaxInFile == 0)
		return(ercOk);

	iEntry = pQ->iEntryFirst;

	pQueueEntryBuff = (QueueEntry_t *) calloc(1,pQ->sEntrySize);

	while (iEntry != 0xffff) { //traverse the chain to pick up the entries

		if (!pQueueEntry)
			pQueueEntry = (QueueEntry_t *) calloc(1,sizeof(QueueEntry_t));
			
		fSuccess = GetEntry(pQ->qh, pQueueEntryBuff, pQ->sEntrySize, 
							sQueueHeader+(pQ->sEntrySize * iEntry));

		if (!fSuccess) {
			//why
			erc = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
			if (!erc)
				erc = ercInconsistency;
			goto AllDone;
			}

		memcpy(pQueueEntry, pQueueEntryBuff, sizeof(QueueEntry_t));

		fValid= CheckIfQueueEntryValid(pQueueEntry);
		if (fValid) {
			fTimeElapsed = CompareTime(pQueueEntry->dateTime, 0);
			
			if (fTimeElapsed) { //can go to active Queue
				p1 = AddQueueEntryToList((void **)&pQ->pActiveQueues, pQueueEntry);
				pQ->cActiveEntries++;
				} 
			else { //keep entry in the timed queue
				p1 = AddQueueEntryToList((void **)&pQ->pTimedQueues, pQueueEntry);
				pQ->cTimedEntries++;
				}
		
			//sanity 
			if (p1 != pQueueEntry) {//why?
				DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
				erc = GetLastError();
				goto AllDone;
			}

			MarkEntry(pQ, iEntry, QE_USED);
			nEntries++;
			iPrevEntry = iEntry; //for recovery
			iEntry = pQueueEntry->iNext;
			pQueueEntry = NULL;
		} //valid entry
		else { //not a valid entry
			fBrokenChain = TRUE;
			MarkEntry(pQ, iEntry, QE_UNUSED);	//TO_DO, make sure iEntry does
												//not exceed the map bounds
			//now set the pointers correct
			fSuccess = GetEntry(pQ->qh, pQueueEntryBuff, pQ->sEntrySize, 
								lfa = sQueueHeader+(pQ->sEntrySize * iPrevEntry));
			if (!fSuccess) {
				//why
				erc = GetLastError();
				DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
				if (!erc)
					erc = ercInconsistency;
				goto AllDone;
			}

			pQueueEntry->iNext = 0xffff;				//link
			pQ->iEntryLast = iPrevEntry;				//link

			memcpy(pQueueEntry, pQueueEntryBuff, sizeof(QueueEntry_t));

			fSuccess = PutEntry(pQ->qh, pQueueEntryBuff, pQ->sEntrySize, lfa);
			if (!fSuccess) {
				//why
				erc = GetLastError();
				DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
				if (!erc)
					erc = ercInconsistency;
				goto AllDone;
			}

			fSuccess = PutEntry(pQ->qh, pQ, sQueueHeader, 0);
			if (!fSuccess) {
				//why
				erc = GetLastError();
				DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
				if (!erc)
					erc = ercInconsistency;
				goto AllDone;
			}
			iEntry = 0xffff;
		} //end if (fValid == FALSE)

	} //while (iEntry != 0xffff)


	for(iEntry=0;((iEntry<nMaxInFile) && (iEntry<MAX_ENTRIES_QUEUE));iEntry++) {
		BOOL fInChain;

/*		if (!pQueueEntry) // LSL 980721 SCR# 10 pQueueEntry alloc moved after validity check
			pQueueEntry = (QueueEntry_t *) zmalloc(sizeof(QueueEntry_t));
*/

		fSuccess = GetEntry(pQ->qh, pQueueEntryBuff, pQ->sEntrySize, 
							sQueueHeader+(pQ->sEntrySize * iEntry));

		if (!fSuccess) {
			//why
			erc = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
			if (!erc)
				erc = ercInconsistency;
			goto AllDone;
		}
		// LSL 980721 SCR# 10 memcpy moved to after validity check
		// memcpy(pQueueEntry, pQueueEntryBuff, sizeof(QueueEntry_t));
		// LSL 980721 SCR# 10 param changed from pQueueEntry to pQueueEntryBuff
		fValid= CheckIfQueueEntryValid(pQueueEntryBuff);
		if (!fValid) {
			MarkEntry(pQ, iEntry, QE_UNUSED);	//TO_DO, make sure iEntry does
												//not exceed the map bounds
			//goto NextEntry;
			continue;
		}

		if (!pQueueEntry)   // LSL 980721 SCR# 10 pQueueEntry alloc moved after validity check
			pQueueEntry = (QueueEntry_t *) calloc(1,sizeof(QueueEntry_t));
		// LSL 980721 SCR# 10 memcpy... moved to after validity check
		memcpy(pQueueEntry, pQueueEntryBuff, sizeof(QueueEntry_t));
		
		fInChain = CheckInEntryInList(pQ, pQueueEntry);

		if (!fInChain) { //fInChain this entry is already linked in
			fNoLink = TRUE;
			MarkEntry(pQ, iEntry, QE_USED);	//TO_DO, make sure iEntry does
			nEntries++; 
			fSuccess = AddQueueEntryToDisk(pQ, (QueueEntry_t *) pQueueEntryBuff);
			if (!fSuccess) {
				//why
				erc = GetLastError();
				DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
				if (!erc)
					erc = ercInconsistency;
				goto AllDone;
			}
			fTimeElapsed = CompareTime(pQueueEntry->dateTime, 0);

			if (fTimeElapsed) { //can go to active Queue
				p1 = AddQueueEntryToList((void **)&pQ->pActiveQueues, pQueueEntry);
				pQ->cActiveEntries++;
			} else { //keep entry in the timed queue
				p1 = AddQueueEntryToList((void **)&pQ->pTimedQueues, pQueueEntry);
				pQ->cTimedEntries++;
			}
			//sanity
			if (p1 != pQueueEntry) {
				//why?
				DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
				erc = GetLastError();
				goto AllDone;
			}
			pQueueEntry = NULL;
		}  // end if(!fInChain)

		// LSL 990215 free pQueueEntry
		if(pQueueEntry) {
			free(pQueueEntry);
			pQueueEntry = NULL;
			}
		// LSL 990215 end change
		//NextEntry:
		//	; //loop back
	} //for scanning all entries

	pQ->fStable = TRUE;

	//if (nEntries != pQ->cEntries)
	pQ->cEntries = nEntries;

	//rewrite the header

	fSuccess = PutEntry(pQ->qh, pQ, sQueueHeader, 0);
	if (!fSuccess) {
		//why?
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		erc = GetLastError();
		goto AllDone;
	}
	erc = ercOk;
AllDone:
	if (pQueueEntryBuff) {
		free(pQueueEntryBuff);
		pQueueEntryBuff = NULL;
		}
	return(erc);
}


DWORD H_AddQueue(WORD wEntrySize, WORD wQueueType, 
				 char *pQueueName, char *pFileName, DWORD *qeh)
{
	QueueHeader_t *pQ = NULL;
	BOOL fFound=FALSE, fSuccess=FALSE;
	HANDLE fh=INVALID_HANDLE_VALUE;
	DWORD erc=0;
	WORD iQueueFileState, iState;

	//Check for duplicate QueueName, ercDuplicateDevSpecName

	pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
									CmpQueueName, (void *) pQueueName);

	if (pQ) {
		pQ = NULL; //we don't want this to be re-queued
		erc = ercDuplicateDevSpecName;
		goto AllDone;
	}

	
	//Check for duplicate FileName, ercDuplicateQueueName
	pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
									CmpFileName, (void *) pFileName);
	if (pQ) {
		pQ = NULL;
		erc = ercDuplicateQueueName;
		goto AllDone;
	}

	if (wEntrySize == 0) {
		erc = ercIllegalQMParam;
		goto AllDone;
	}

	iQueueFileState = CheckQueueFileState(pFileName, &fh);

	for(iState=iQueueFileState;(iState<Q_STATE_MAX);iState++) {
		
		switch (iState) {
		
		case Q_STATE_ERR:
			erc = GetLastError();
			DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
			goto AllDone;
			break;
		
		case Q_STATE_NEW:
			fSuccess = CreateQueueHeader(fh, wEntrySize, wQueueType, 
										 pQueueName, pFileName);
			if (!fSuccess) {
				erc = GetLastError();
				DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
				goto AllDone;
			}
			break;

		case Q_STATE_OLD:
			pQ = InitQueueHeader(fh, wEntrySize, wQueueType, pQueueName, pFileName);
			if (!pQ) {
				erc = GetLastError();
				DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
				goto AllDone;
			}
			break;
		
		default:
			DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
			break;
		} //switch
	} //for


AllDone:
	if (erc)
		if (fh != INVALID_HANDLE_VALUE)
			CloseHandle(fh);

	if(pQ) {
		ListAppend((void **) &g_pQueues, (void *) pQ);
		*qeh = (DWORD) pQ->qh;
		erc = UpdateQueueInformation(pQ);
	}

	//TO_DO make sure that the structures are de-allocated if any error

	return(erc);
}

//need to find three criteria to link the queueEntries together
// 1- just a list
// 2- based on priority
// 3- based on time

/*
	Each Queue will will have QueueHeader_t allocated in the core 
	and at Init() we will red all entries in this Queue file and chain them 
	in memory by allocating a QueueEntry_t structure for each entry. 
	Each QueueEntry can be in one of three queues:
	pActiveQueues - it can be handed to any queue server that makes the Mark..() call
	pTimedQueues - queues that can be doled out at predefined times
	pMarkedQueues - queues that are marked active
*/

BOOL AddQueueEntryToDisk(QueueHeader_t *pQ, QueueEntry_t *pEntry)
{
	DWORD lfa = pEntry->iEntry * pQ->sEntrySize + sizeof(QueueHeader_t);
	BOOL fSuccess;
	QueueEntry_t *pPrevEntry=NULL;

	if (pQ->fileSize < lfa) {
		//this is a serious error
		SetLastError(ercInconsistency);
		DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
		return FALSE;
	}

	if (pQ->fileSize == lfa) { //we need to extend the file

		void *p;
		DWORD lfaTemp;
	
		p = (void *) calloc(1,pQ->sEntrySize)	;
		lfaTemp = lfa;

		for (DWORD i=0;(i<EXTEND_QUANTUM);i++) {
			fSuccess = PutEntry(pQ->qh, p, pQ->sEntrySize, lfaTemp);
			if (!fSuccess) {
				free(p);
				p = NULL;
				DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
				return (FALSE);
			}
			lfaTemp += pQ->sEntrySize;
		}

		free(p);
		p = NULL;

		pQ->fileSize += pQ->sEntrySize * EXTEND_QUANTUM;

		//now re-write the QueueHeader

		fSuccess = PutEntry(pQ->qh, pQ, sizeof(QueueHeader_t), 0);
		if (!fSuccess) {
				DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
				return (FALSE);
		}
	}

	MarkEntry(pQ, pEntry->iEntry, QE_USED);

	if (pQ->iEntryLast != 0xffff) { //there is a previous entry

		//sanity check
		if (pQ->iEntryFirst == 0xffff) {
			DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
			fSuccess = FALSE;
			goto AllDone;
		}

		pPrevEntry = (QueueEntry_t *) zmalloc(pQ->sEntrySize);

		fSuccess = GetEntry(pQ->qh, pPrevEntry, pQ->sEntrySize, 
							sizeof(QueueHeader_t) + pQ->iEntryLast * pQ->sEntrySize);
		if (!fSuccess) {
			DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
			goto AllDone;
		}

		//sanity check

		if (pQ->iEntryLast != pPrevEntry->iEntry) {
			DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
			fSuccess = FALSE;
			goto AllDone;
		}


		pEntry->iNext = 0xffff;						//link
		pEntry->iPrev = pQ->iEntryLast;				//link
		pPrevEntry->iNext = pEntry->iEntry;			//link
		pQ->iEntryLast = pEntry->iEntry;			//link

	}
	else { //this is the first entry
		pQ->iEntryFirst = pQ->iEntryLast = pEntry->iEntry;	//link
		pEntry->iPrev = pEntry->iNext = 0xffff;				//link
	}

	fSuccess = PutEntry(pQ->qh, pEntry, pQ->sEntrySize, 
						sizeof(QueueHeader_t) + pEntry->iEntry * pQ->sEntrySize);
	if (!fSuccess) {
			DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
			goto AllDone;
	}

	if (pPrevEntry) {
		fSuccess = PutEntry(pQ->qh, pPrevEntry, pQ->sEntrySize, 
							sizeof(QueueHeader_t) + pPrevEntry->iEntry * pQ->sEntrySize);
		if (!fSuccess) {
			DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
			goto AllDone;
		}
	}

	fSuccess = PutEntry(pQ->qh, pQ, sizeof(QueueHeader_t), 0);
	if (!fSuccess) {
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		goto AllDone;
	}

AllDone:
	if (pPrevEntry) {
		free(pPrevEntry);
		pPrevEntry = NULL;
		}
	return (fSuccess);
}

BOOL RemoveQueueEntryFromDisk(QueueHeader_t *pQ, QueueEntry_t *pEntry)
{
	QueueEntry_t *pCurrent=0, *pPrev=0, *pNext=0;
	BOOL fSuccess;
	WORD iState;

	pCurrent = (QueueEntry_t *) zmalloc(pQ->sEntrySize);

	fSuccess = GetEntry(pQ->qh, pCurrent, pQ->sEntrySize,
						sizeof(QueueHeader_t) + pQ->sEntrySize * pEntry->iEntry);
	if (!fSuccess) {
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		goto AllDone;
	}
#define E_ONLY 0
#define E_FIRST 1
#define E_LAST 2
#define E_MIDDLE 3

	if (pCurrent->iNext == 0xffff) {
		if (pCurrent->iPrev == 0xffff)
			iState = E_ONLY;
		else
			iState = E_LAST;
	} else //pCurrent->iNext != 0xffff 
	{
		if (pCurrent->iPrev == 0xffff)
			iState = E_FIRST;
		else
			iState = E_MIDDLE;
	}

	if (iState == E_LAST || iState == E_MIDDLE) {
		pPrev = (QueueEntry_t *) zmalloc(pQ->sEntrySize);

		fSuccess = GetEntry(pQ->qh, pPrev, pQ->sEntrySize,
							sizeof(QueueHeader_t) + pQ->sEntrySize * pCurrent->iPrev);
		if (!fSuccess) {
			DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
			goto AllDone;
		}
		//sanity
		if (pPrev->iEntry != pCurrent->iPrev) {
			fSuccess = FALSE;
			DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
			goto AllDone;
		}
	} //read the PrevEntry

	if (iState == E_FIRST || iState == E_MIDDLE) {
		pNext = (QueueEntry_t *) zmalloc(pQ->sEntrySize);

		fSuccess = GetEntry(pQ->qh, pNext, pQ->sEntrySize,
							sizeof(QueueHeader_t) + pQ->sEntrySize * pCurrent->iNext);
		if (!fSuccess) {
			DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
			goto AllDone;
		}
		//sanity
		if (pNext->iEntry != pCurrent->iNext) {
			fSuccess = FALSE;
			DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
			goto AllDone;
		}
	} //read the NextEntry

	switch (iState) {

	case E_ONLY:
		pQ->iEntryFirst = pQ->iEntryLast = 0xffff;
		break;

	case E_FIRST:
		pNext->iPrev = 0xffff;	
		pQ->iEntryFirst = pNext->iEntry;
		break;

	case E_LAST:
		pPrev->iNext = 0xffff;
		pQ->iEntryLast = pPrev->iEntry;
		break;

	case E_MIDDLE:
		pNext->iPrev = pPrev->iEntry;
		pPrev->iNext = pNext->iEntry;
		break;

	default:
		fSuccess = FALSE;
		DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
		goto AllDone;
		break;
	}

	memset(pCurrent, 0, pQ->sEntrySize);

	fSuccess = PutEntry(pQ->qh, pCurrent, pQ->sEntrySize,
						sizeof(QueueHeader_t) + pQ->sEntrySize * pEntry->iEntry);
	if (!fSuccess) {
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		goto AllDone;
	}

	if (pPrev) {
		fSuccess = PutEntry(pQ->qh, pPrev, pQ->sEntrySize,
							sizeof(QueueHeader_t) + pQ->sEntrySize * pPrev->iEntry);
		if (!fSuccess) {
			DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
			goto AllDone;
		}
	}

	if (pNext) {
		fSuccess = PutEntry(pQ->qh, pNext, pQ->sEntrySize,
							sizeof(QueueHeader_t) + pQ->sEntrySize * pNext->iEntry);
		if (!fSuccess) {
			DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
			goto AllDone;
		}
	}

	fSuccess = PutEntry(pQ->qh, pQ, sizeof(QueueHeader_t), 0);
	if (!fSuccess) {
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		goto AllDone;
	}
	
	MarkEntry(pQ, pEntry->iEntry, QE_UNUSED);

AllDone:
	if (pCurrent) {
		free(pCurrent);
		pCurrent = NULL;
		}
	if (pPrev) {
		free(pPrev);
		pPrev = NULL;
		}
	if (pNext) {
		free(pNext);
		pNext = NULL;
		}
	return(fSuccess);
}


DWORD H_AddQueueEntry(BOOL fQueueIfNoServer, WORD QPriority, WORD QType, WORD RepeatTime,
					  char *pQueueName, BYTE *pbEntry, WORD cbEntry, 
					  pCtosDateTimeType pSysDateTime)
{
	QueueHeader_t *pQ;
	QueueEntry_t *pEntry=NULL, *pEntryAdded;
	char *pBuff=NULL;
	DWORD erc;
	BOOL fSuccess, fTimeElapsed;

	pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
									 CmpQueueName,
									 (void *) pQueueName);

	if (!pQ)
		return(ercBadQueSpec);

	if ((!fQueueIfNoServer) && (pQ->cServers == 0))
		return(ercNoEstablishedServers);

	if (QType != pQ->queueType)
		return(ercWrongQueueType);

	if ((sizeof(QueueEntry_t)-1 + cbEntry) > pQ->sEntrySize)
		return(ercQueueEntryOverflow);

	pBuff = (char *) zmalloc(pQ->sEntrySize);

	pEntry = (QueueEntry_t *) zmalloc(sizeof(QueueEntry_t));

	pEntry->signature = QE_SIGNATURE;
	pEntry->priority = (QPriority > 9)? 9 : QPriority;
	memcpy(&pEntry->dateTime, pSysDateTime, sizeof(ctosDateTimeType));
	pEntry->repeatInterval = RepeatTime;

	
	pEntry->iEntry = GetFreeEntry(pQ);

	//if (pEntry->iEntry == 0xfff) { //have we made so many entries, or is it a bug?
	// LSL 000423 This is horrible.  If table is full - return an error.
	if (pEntry->iEntry >= MAX_ENTRIES_QUEUE) { 
		erc = ercQueueTableFull;
		goto AllDone;
	}
	pEntry->eh = (DWORD) pEntry->iEntry; //TO_DO maybe we need to add a time stamp too

	pEntry->cEntry = cbEntry;
	memcpy(pBuff, pEntry, sizeof(QueueEntry_t));
	memcpy(pBuff+offsetof(QueueEntry_t, rgEntry), pbEntry, cbEntry);

	pQ->cEntries++; 
	fSuccess = AddQueueEntryToDisk(pQ, (QueueEntry_t *) pBuff);

	if (!fSuccess) {
		erc = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		goto AllDone;
	}

	fTimeElapsed = CompareTime(pEntry->dateTime, 0);

	if (fTimeElapsed) { //can go to active Queue
		pEntryAdded = AddQueueEntryToList((void **)&pQ->pActiveQueues, pEntry);
		pQ->cActiveEntries++;
	} else { //keep entry in the timed queue
		pEntryAdded = AddQueueEntryToList((void **)&pQ->pTimedQueues, pEntry);
		pQ->cTimedEntries++;
	}

	if (pEntryAdded != pEntry) {
		//what now?
		DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
		erc = ercInconsistency;
		goto AllDone;
	}


	erc = ercOk;

AllDone:
	if (pBuff) {
		free(pBuff);
		pBuff = NULL;
		}
	if (pQ->cActiveEntries)
		RespondToQueuedMarkRequests(pQ);

	return(erc);
}

DWORD H_EstablishQueueServer(WORD QueueType, BOOL fUniqueServer, 
							 char *pQueueName, DWORD userNum)
{
	int i;
	BOOL fSuccess;
	DWORD erc;

	QueueHeader_t *pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
												   CmpQueueName,
												   (void *) pQueueName);

	if (!pQ)
		return(ercBadQueSpec);

	if (QueueType != pQ->queueType)
		return(ercWrongQueueType);

	if (pQ->cServers >= MAXSERVERS)
		return(ercTooManyServers);

	for(i=0;(i<MAXSERVERS);i++) {
		if (pQ->rgQueueServers[i] == userNum) //this user is already serving the queue
			return (ercOk);
	}

	//moved this from before the previous line
	if (pQ->cServers)
		if ((pQ->fUniqueSvr) || (fUniqueServer))
			return(ercNotUniqueServer);

	for(i=0;(i<MAXSERVERS);i++) {
		if (pQ->rgQueueServers[i] == 0) 
			break;
	}

	if (i == MAXSERVERS) {
		//not good, we have problems in clean up
		DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
		return (ercInconsistency);
	}

	pQ->fUniqueSvr = fUniqueServer;
	pQ->rgQueueServers[i] = userNum;
	pQ->cServers++;

	//now re-write the header

	fSuccess = PutEntry(pQ->qh, pQ, sizeof(QueueHeader_t), 0);
	if (!fSuccess) {
		//why
		erc = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
	}

	return(ercOk);
}

/*
  In this function be aware that pQueueEntry traverses from the ActiveQueue to the 
  MarkedQueue. The pQueueEntryRet is the temporary variable that is allocated top read
  the complete entry from the disk file and this is returned to the user. Also note that
  the pQueueEntry->Next points to the Next Active record and must no be carried when this 
  is queued in the marked entry.
*/

QueueEntry_t *MarkQueueEntry(QueueHeader_t *pQ, QueueEntryStatus_t *pStatus,
							 DWORD userNum, DWORD eh)
{
	QueueEntry_t *pQueueEntry, *pQueueEntryRet, *pNext;
	BOOL fSuccess;
	DWORD lfa;


	pQueueEntryRet = (QueueEntry_t *) zmalloc(pQ->sEntrySize);

	if (eh == DW_ONES)
		pQueueEntry = (QueueEntry_t *) ListRetrieve((void **) &pQ->pActiveQueues);
	else
		pQueueEntry = (QueueEntry_t *) ListCmpRemove((void **) &pQ->pActiveQueues, 
													 CmpQEH, (void *) &eh);
	if (!pQueueEntry) {
		free(pQueueEntryRet);
		pQueueEntryRet = NULL;
		DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
		return(NULL);
	}

	pQ->cActiveEntries--;

	fSuccess = GetEntry(pQ->qh, pQueueEntryRet, pQ->sEntrySize, 
						(lfa = sizeof(QueueHeader_t)+(pQ->sEntrySize * 
														pQueueEntry->iEntry)));

	if (!fSuccess) {
		free(pQueueEntryRet);
		pQueueEntryRet = NULL;
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		return(NULL);
	}

	pQueueEntryRet->markUserNum = pQueueEntry->markUserNum = userNum;

	fSuccess = PutEntry(pQ->qh, pQueueEntryRet, pQ->sEntrySize, lfa);

	if (!fSuccess) {
		free(pQueueEntryRet);
		pQueueEntryRet = NULL;
		DEBUG_BREAK((char *)__FILE__,__LINE__, GetLastError());
		return(NULL);
	}

	pQueueEntry->Next = NULL; //for safety

	pNext = (QueueEntry_t *) ListAppend((void **) &pQ->pMarkedQueues, 
									  (void *) pQueueEntry);

	if (pNext != pQueueEntry) {
		//why??
		DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
		free(pQueueEntry);
		pQueueEntryRet = NULL;
		return(NULL);
	}

	pStatus->qeh.qh = (DWORD) pQ->qh;
	pStatus->qeh.eh = pQueueEntry->eh | EH_MARKEDQUEUEMASK;
	pStatus->priority = pQueueEntry->priority;
	pStatus->serverUserNum = userNum;

	pNext = (QueueEntry_t *) ListScan((void **) &pQ->pActiveQueues, CmpReturnFirst, NULL);

	if (pNext) {
		pStatus->qehNext.qh = (DWORD) pQ->qh;
		pStatus->qehNext.eh = pNext->eh | EH_MARKEDQUEUEMASK;
	} else {
		pStatus->qehNext.qh = 0;
		pStatus->qehNext.eh = E_NOVALIDENTRY | EH_MARKEDQUEUEMASK;
	}

	return(pQueueEntryRet); // this will get deallocated by the caller
}


BOOL QueueUserRequest(QueueHeader_t *pQ, RqMarkNextQueueEntry_t *pRq, DWORD *erc)
{

	if (pQ->cQueuedRequests >= MAXQUEUEDSERVERS) {
		*erc = ercNoEntryAvail; //TO_DO CTOS uses this, should we use a new erc code
		return(FALSE);
	}

	/*
	Do not search through the list rgQueuedRequests[] for an empty slot, always use 
	the [cQueuedRequests] as the index to for the slot. Because, while offering an 
	entry to a queued server we pick up the zeroth entry and move all entries up by 1
	as a sanity check make sure that the cServers index is zero, if not we have
	an inconsistency.
	*/

	if (pQ->rgQueuedRequests[pQ->cQueuedRequests]) {

		*erc = ercInconsistency;
		return(FALSE);
	}


	pQ->rgQueuedRequests[pQ->cQueuedRequests++] = pRq;

	return(TRUE);
}

DWORD H_MarkNextQueueEntry(BOOL fReturnIfNoEntries, char *pQueueName,
						   BYTE *pbEntryRet, WORD cbEntryRet,
						   BYTE *StatusBlock, WORD sStatusBlock,
						   DWORD userNum, BOOL *fRet,
						   RqMarkNextQueueEntry_t *pRq)
{
	int i;
	DWORD erc;
	BOOL fSuccess;
	QueueEntry_t *pEntry;
	char msgBuffer[1024];

	QueueHeader_t *pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
												   CmpQueueName,
												   (void *) pQueueName);

	*fRet = TRUE;  //this decides if the user is going to block or not
				   //true makes the pipe server respond immediately

	if (!pQ)
		return(ercBadQueSpec);

	for(i=0;(i<MAXSERVERS);i++) {
		if (pQ->rgQueueServers[i] == userNum) //this user is already serving the queue
			break;
	}

	if (i == MAXSERVERS)
		return(ercServerNotEstablished);

	/*This creates more problems than it fixes
	if ( (sStatusBlock < sizeof(QueueEntryStatus_t)) ||
		 (cbEntryRet < ( (WORD) pQ->sEntrySize - (sizeof(QueueEntry_t *) - 1))) ) {

		*fRet = TRUE;		
		return (ercQueueEntryOverflow);
	}
	Just truncate the entry to the size provided by the user
	*/
	
	// LSL 000424 Try CyberTech code again with modifications
	if ( (sStatusBlock < sizeof(QueueEntryStatus_t)) )
	//	|| (cbEntryRet < ( (WORD) pQ->sEntrySize - (sizeof(QueueEntry_t) - 1))) ) 
		{
		*fRet = TRUE;		
		return (ercQueueEntryOverflow);
	}


	//all validation is now complete

	if (pQ->cActiveEntries == 0) { //non-block
		erc = ercNoEntryAvail;
		if (fReturnIfNoEntries) { //user does not want to wait
			sprintf(msgBuffer, "MarkNextQueueEntry (Polling) on queue %s, for user %X",pQueueName, pRq->RqHdr.userNum);
			WriteLogData(msgBuffer, 6);

			*fRet = TRUE;
			return(erc);
		} //non-block
		
		else { //block
			sprintf(msgBuffer, "MarkNextQueueEntry (Sleeping) on queue %s, for user %X",pQueueName, pRq->RqHdr.userNum);
			WriteLogData(msgBuffer, 2);

			fSuccess = QueueUserRequest(pQ, pRq, &erc);
			if (!fSuccess) { //un-able to queue
				*fRet = TRUE; 
				return (erc);
			} //un-able to queue
			else { //queuing successful
				*fRet = FALSE; //user is going to block
				return(ercOk); //does not matter
			} //queuing successful
		} //block
	} //cActiveEntries == 0

	//we have an entry, let us find it and respond to user

	//TO_DO we could be shooting ourselves in the leg by assuming that StatusBlock
	//is large enough to accomodate the structure QueueEntryStatus_t
	// LSL 000424 sStatusBlock is now checked above
	sprintf(msgBuffer, "MarkNextQueueEntry (Data Available) on queue %s, for user %X",pQueueName, pRq->RqHdr.userNum);
	WriteLogData(msgBuffer, 2);

	pEntry = MarkQueueEntry(pQ, (QueueEntryStatus_t *) StatusBlock, userNum, DW_ONES);

	if (!pEntry) {
		//what now
		erc = GetLastError();
		if(!erc) erc = ercInconsistency;
		*fRet = TRUE;
		return(erc);
	}

	memcpy(pbEntryRet, pEntry->rgEntry, MIN(pEntry->cEntry, cbEntryRet));

	//TO_DO for the benefit of my test programs PLEASE WITHDRAW THIS ...

	if (pEntry->cEntry < cbEntryRet)
		*(pbEntryRet+pEntry->cEntry) = 0;

	//...for the benefit of my test programs

	free(pEntry);
	pEntry = NULL;

	*fRet = TRUE;
	return(ercOk);
}

DWORD H_MarkKeyedQueueEntry(char *pQueueName, 
							WORD oKey1, BYTE *pKey1, WORD cKey1,
						    WORD oKey2, BYTE *pKey2, WORD cKey2,
							BYTE *pbEntryRet, WORD cbEntryRet,
							BYTE *StatusBlock, WORD sStatusBlock,
							DWORD userNum)
{
	QueueEntryStatus_t *pStatus = (QueueEntryStatus_t *) StatusBlock;
	BOOL fFound=FALSE;
	BOOL fKey1, fKey2;
	DWORD erc = ercOk, eh;
	int i;
	BOOL fSuccess;
	QueueEntry_t *pEntry, *pEntryBuff;
	BYTE *rgEntry;

	QueueHeader_t *pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
												   CmpQueueName,
												   (void *) pQueueName);

	if (!pQ)
		return(ercBadQueSpec);

	for(i=0;(i<MAXSERVERS);i++) {
		if (pQ->rgQueueServers[i] == userNum) //this user is already serving the queue
			break;
	}

	if (i == MAXSERVERS)
		return(ercServerNotEstablished);

	//all validation is now complete

	if (pQ->cActiveEntries == 0) { //non-block
		erc = ercNoEntryAvail;
		return (erc);
	}

		
	fKey1 = ((pKey1 && cKey1)? FALSE : TRUE); //if no key is specified, the match is
											  //automatically met
	fKey2 = ((pKey2 && cKey2)? FALSE : TRUE);
	//sanity check, oKey1 and oKey2 have to be within the entry size TO_DO
	memset((void *)pStatus, 0, sizeof(QueueEntryStatus_t));


	pEntry = pQ->pActiveQueues;
	if (!pEntry) {
		//inconsistency
		erc = ercNoEntryAvail;
		return(erc);
	}

	pEntryBuff = (QueueEntry_t *) zmalloc(pQ->sEntrySize);
	
	while(!fFound && (pEntry)) {
		BOOL fKeyCycle1, fKeyCycle2;

		fSuccess = GetEntry(pQ->qh, pEntryBuff, pQ->sEntrySize, 
							sizeof(QueueHeader_t) + pQ->sEntrySize * pEntry->iEntry);

		if (fSuccess) {
			rgEntry = (BYTE *) pEntryBuff->rgEntry;
			if (!(fKeyCycle1 = fKey1))
				fKeyCycle1 = compareStrings(rgEntry+oKey1, pKey1, cKey1);
			if (!(fKeyCycle2 = fKey2))
				fKeyCycle2 = compareStrings(rgEntry+oKey2, pKey2, cKey2);

			fFound = fKeyCycle1 && fKeyCycle2;
		}
		pEntry = pEntry->Next;
	}

	if (fFound) {
		eh = pEntryBuff->eh;
		pEntry = MarkQueueEntry(pQ, (QueueEntryStatus_t *) StatusBlock, userNum, eh);
		if (!pEntry) {
			//what now
			erc = GetLastError();
			if(!erc) erc = ercInconsistency;
			goto AllDone;
		}
		
		memcpy(pbEntryRet, pEntry->rgEntry, MIN(pEntry->cEntry, cbEntryRet));
		//TO_DO for the benefit of my test programs PLEASE WITHDRAW THIS ...
		if (pEntry->cEntry < cbEntryRet)
			*(pbEntryRet+pEntry->cEntry) = 0;
		//...for the benefit of my test programs
		free(pEntry);
		pEntry = NULL;
		erc = ercOk;
		goto AllDone;
	}
	else
		erc = ercEntryNotFound;
AllDone:
	if (pEntryBuff) {
		free(pEntryBuff);
		pEntryBuff = NULL;
		}
	return (erc);
}

DWORD RemoveQueueEntry(QueueHeader_t *pQ, QueueEntry_t *pQueueEntry, 
					   DWORD QueueId)
{
	QueueEntry_t *pRemRet;
	BOOL fActiveQueue = FALSE, fTimedQueue=FALSE, fSuccess;
	DWORD lfa, erc;

	switch (QueueId) {

	case EH_ACTIVEQUEUEMASK:
		pRemRet = (QueueEntry_t *) ListRemove((void **) &pQ->pActiveQueues, 
											  (void *) pQueueEntry);
		fActiveQueue = TRUE;
		break;
		
	case EH_TIMEDQUEUEMASK:
		pRemRet = (QueueEntry_t *) ListRemove((void **) &pQ->pTimedQueues, 
											  (void *) pQueueEntry);
		fTimedQueue = TRUE;
		break;

	case EH_MARKEDQUEUEMASK:
		pRemRet = (QueueEntry_t *) ListRemove((void **) &pQ->pMarkedQueues,
											  (void *) pQueueEntry);
		break;

	default:
		DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
		return(ercInconsistency);
	}

	
	//sanity ....
	if (!pRemRet) {
		erc = ercInconsistency;
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		goto AllDone;
	}

	lfa = sizeof(QueueHeader_t) + pQ->sEntrySize * pQueueEntry->iEntry;

	if (lfa > pQ->fileSize) {
		erc = ercInconsistency;
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		goto AllDone;
	}
	//...sanity

	pQ->cEntries--;
	if (fActiveQueue)
		pQ->cActiveEntries--;
	if (fTimedQueue)
		pQ->cTimedEntries--;

	erc = ercOk;
	fSuccess = RemoveQueueEntryFromDisk(pQ, pQueueEntry);

	if (!fSuccess) {
		erc = GetLastError();
		if (erc == 0)
			erc = ercInconsistency;
		}
AllDone:

	free(pQueueEntry);
	pQueueEntry = NULL;
	return(erc);
}

void RespondToQueuedMarkRequests(QueueHeader_t *pQ)
{
	RqMarkNextQueueEntry_t *pRq;
	QueueEntry_t *pEntry;
	DWORD erc;

	if (pQ->cQueuedRequests == 0)
		return;

	pRq = pQ->rgQueuedRequests[0];	//always respond to the first queued request
									//and move up all the remaining requests

	if (!pRq) { //sanity check
		//this is bad
		DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
		return;
	}

	for(WORD i=0; (i<(pQ->cQueuedRequests-1)); i++) {
		pQ->rgQueuedRequests[i] = pQ->rgQueuedRequests[i+1];
	}

	pQ->rgQueuedRequests[pQ->cQueuedRequests-1] = 0;

	pQ->cQueuedRequests--;

	pEntry = MarkQueueEntry(pQ, (QueueEntryStatus_t *) pRq->StatusBlock, 
							pRq->RqHdr.userNum, DW_ONES);

	if (!pEntry) {
		erc = GetLastError();	//there is no way I can handle this error
								//this implies some mess-up in the state machine
		DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
	}

	//make sure you that cbEntryRet is greater than cEntry TO_DO
	memcpy(pRq->pbEntryRet, pEntry->rgEntry, MIN(pEntry->cEntry, pRq->cbEntryRet));

	//TO_DO for the benefit of my test programs PLEASE WITHDRAW THIS ...

	if (pEntry->cEntry < pRq->cbEntryRet)
		*(pRq->pbEntryRet+pEntry->cEntry) = 0;

	//...for the benefit of my test programs


	free(pEntry);
	pEntry = NULL;

	Respond((RqHdr_t *) pRq);	
}

DWORD H_RemoveMarkedQueueEntry(char *pQueueName, DWORDLONG QEH, DWORD userNum)
{
	QEH_t *pQeh = (QEH_t *) &QEH;
	QueueHeader_t *pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
												   CmpQueueName,
												   (void *) pQueueName);
	QueueEntry_t *pQueueEntry;

	if (!pQ)
		return(ercBadQueSpec);

	if (pQeh->qh != (DWORD) pQ->qh)
		return(ercBadQeh);

	if ((pQeh->eh & EH_MARKEDQUEUEMASK) == 0) //not a marked entry
		return(ercBadQeh);

	pQeh->eh &= (~EH_MARKEDQUEUEMASK);

	pQueueEntry = (QueueEntry_t *) ListScan((void **) &pQ->pMarkedQueues,
											CmpQEH,
											(void *) &pQeh->eh
											);

	if (!pQueueEntry)
		return(ercEntryNotFound);

	//we should validate that this QueueEntry is marked by this user TO_DO

	return(RemoveQueueEntry(pQ, pQueueEntry, EH_MARKEDQUEUEMASK));	

}

DWORD H_UnmarkQueueEntry(char *pQueueName, DWORDLONG QEH, DWORD userNum)
{
	QEH_t *pQeh = (QEH_t *) &QEH;
	QueueHeader_t *pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
												   CmpQueueName,
												   (void *) pQueueName);
	QueueEntry_t *pQueueEntry, *pBuff;
	DWORD lfa, erc;
	BOOL fSuccess;

	if (!pQ)
		return(ercBadQueSpec);

	if (pQeh->qh != (DWORD) pQ->qh)
		return(ercBadQeh);

	if ((pQeh->eh & EH_MARKEDQUEUEMASK) == 0) //not a marked entry
		return(ercBadQeh);

	pQeh->eh &= (~EH_MARKEDQUEUEMASK);

	pQueueEntry = (QueueEntry_t *) ListCmpRemove((void **) &pQ->pMarkedQueues,
											     CmpQEH, (void *) &pQeh->eh
												);

	if (!pQueueEntry)
		return(ercEntryNotFound);

	//we should validate that this QueueEntry is marked by this user TO_DO

	pBuff = (QueueEntry_t *) zmalloc(pQ->sEntrySize);
	fSuccess = GetEntry(pQ->qh, (void *) pBuff, pQ->sEntrySize,
				(lfa = sizeof(QueueHeader_t) + pQ->sEntrySize*pQueueEntry->iEntry));

	if (!fSuccess) {
		free(pBuff);
		pBuff = NULL;
		erc = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		return(erc);
	}

	pBuff->markUserNum = pQueueEntry->markUserNum = 0;

	fSuccess = PutEntry(pQ->qh, (void *) pBuff, pQ->sEntrySize, lfa);

	free(pBuff);
	pBuff = NULL;

	if (!fSuccess) {
		erc = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		goto AllDone;
	}

	pBuff = (QueueEntry_t *) AddQueueEntryToList((void **)&pQ->pActiveQueues, 
												 pQueueEntry);
	//sanity
	if (pBuff != pQueueEntry) {
		//red alert
		erc = ercInconsistency;
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		goto AllDone;
	}
	pQ->cActiveEntries++;

	erc = ercOk;

AllDone:
	return(erc);
}


DWORD H_TerminateQueueServer(char *pQueueName, DWORD userNum)
{
	WORD i;
	char msgbuffer[132];
	QueueHeader_t *pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
												   CmpQueueName,
												   (void *) pQueueName);
	
	if (!pQ)
		return(ercBadQueSpec);

	if (pQ->cServers == 0)
		return(ercServerNotEstablished);

	for(i=0;(i<MAXSERVERS);i++) {
		if (pQ->rgQueueServers[i] == userNum) 
			break;
	}

	if (i == MAXSERVERS)
			return(ercServerNotEstablished);

	//TO_DO
	//before we terminate the server we should make sure all queued requests belonging
	//to this user are responded to.
	sprintf(msgbuffer,"Terminating %s user %X.\n",
		pQueueName, userNum );

	OutputDebugString(msgbuffer);
	WriteLogData(msgbuffer, 1);
	
	for(; (i<(pQ->cServers-1)); i++) {
		pQ->rgQueueServers[i] = pQ->rgQueueServers[i+1];
	}

	pQ->rgQueueServers[pQ->cServers-1] = 0;

	pQ->cServers--;

	return(ercOk);
}

DWORD CleanUpQueue(QueueHeader_t *pQ)
{
	QueueEntry_t *pQueueEntry=NULL;
	DWORD erc=ercOk;
	BOOL fForcedDelete=FALSE;

	while (pQueueEntry = (QueueEntry_t *) ListScan((void **) &pQ->pMarkedQueues, 
													CmpReturnFirst, NULL)) {
		erc = RemoveQueueEntry(pQ, pQueueEntry, EH_MARKEDQUEUEMASK);
		if (erc) { //free all the QueueEntry buffers
			while (pQueueEntry = (QueueEntry_t *) ListRetrieve((void **) &pQ->pMarkedQueues))
				{
				free(pQueueEntry);
				pQueueEntry = NULL;
				}
			fForcedDelete = TRUE;
		}
	}

	while (pQueueEntry = (QueueEntry_t *) ListScan((void **) &pQ->pActiveQueues, 
													CmpReturnFirst, NULL)) {
		erc = RemoveQueueEntry(pQ, pQueueEntry, EH_ACTIVEQUEUEMASK);
		if (erc) { //free all the QueueEntry buffers
			while (pQueueEntry = (QueueEntry_t *) 
							ListRetrieve((void **) &pQ->pActiveQueues))
				{
				free(pQueueEntry);
				pQueueEntry = NULL;
				}
			fForcedDelete = TRUE;
		}
	}

	while (pQueueEntry = (QueueEntry_t *) ListScan((void **) &pQ->pTimedQueues, 
													CmpReturnFirst, NULL)) {
		erc = RemoveQueueEntry(pQ, pQueueEntry, EH_TIMEDQUEUEMASK);
		if (erc) { //free all the QueueEntry buffers
			while (pQueueEntry = (QueueEntry_t *) 
											ListRetrieve((void **) &pQ->pTimedQueues))
				{
				free(pQueueEntry);
				pQueueEntry = NULL;
				}
			fForcedDelete = TRUE;
		}
	}

	if (fForcedDelete) {
		DWORD iMaxEntries = ((pQ->fileSize - sizeof(QueueHeader_t)) / pQ->sEntrySize);
		void *p =  zmalloc(pQ->sEntrySize);
		DWORD lfa=sizeof(QueueHeader_t);
		BOOL fSuccess;

		for (DWORD i=0;(i<iMaxEntries);i++) {
			fSuccess = PutEntry(pQ->qh, p, pQ->sEntrySize, lfa);
			if (!fSuccess) {
				free(p);
				p = NULL;
				DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
				return (ercInconsistency);
			}
			lfa += pQ->sEntrySize;
		}

		free(p);
		p = NULL;
		memset(pQ->pEntryMap, 0, pQ->sEntryMap);
		erc = ercOk;
	}

	return (erc);
}

DWORD H_CleanQueue(DWORD qh)
{
	QueueHeader_t *pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
												   CmpQH, (void *) &qh);

	if (!pQ) {
		return(ercBadQueSpec);
	}

	return(CleanUpQueue(pQ));
}

DWORD H_RemoveQueue(DWORD qh)
{
	QueueHeader_t *pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
												   CmpQH, (void *) &qh);
	QueueHeader_t *pQRem;
	DWORD erc;

	if (!pQ) {
		return(ercBadQueSpec);
	}

	erc = CleanUpQueue(pQ);
	if (erc != ercOk)
		return(erc);

	pQRem = (QueueHeader_t *) ListRemove((void **) &g_pQueues, (void *) pQ);
	//sanity
	if (pQ != pQRem) {
		DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
		return (ercInconsistency);
	}

	free(pQ->pEntryMap);
	pQ->pEntryMap = NULL;
	if (!(CloseHandle(pQ->qh))) {
		erc = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		return(erc);
	}

	
	if (!(DeleteFile(pQ->FileName))) {
		erc = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		return(erc);
	}
	
	return(ercOk);
}


#if(ReturnByQueueOrder)
DWORD H_ReadNextQueueEntry(DWORDLONG QEH, char *pQueueName, 
						   char *pbEntryRet, WORD cbEntryRet,
						   BYTE *StatusBlock, WORD sStatusBlock)
{
	QueueEntry_t *pQueueEntry=NULL;
	QEH_t *pQeh = (QEH_t *) &QEH;
	QueueHeader_t *pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
												   CmpQueueName,
												   (void *) pQueueName);
	int (*cmpFunc)(void *, void *);
	DWORD queueId;

	if (!pQ) {
		return(ercBadQueSpec);
	}

	if (pQ->cActiveEntries == 0) { //non-block
		erc = ercNoEntryAvail;

	if (pQeh->qh == 0) {
		QueueId = EH_MARKEDQUEUEMASK;
		cmpFunc = CmpReturnFirst;
	} //first call 
	else {
		DWORD ehMask = pQeh->eh & 
							(EH_ACTIVEQUEUEMASK | EH_TIMEDQUEUEMASK | EH_MARKEDQUEUEMASK);
		DWORD eh = pQeh->eh & 
						(~(EH_ACTIVEQUEUEMASK | EH_TIMEDQUEUEMASK | EH_MARKEDQUEUEMASK));
		QueueId = ehMask;
		cmpFunc = CmpQEH;
		switch (ehMask) {
		case EH_MARKEDQUEUEMASK
			if (eh == E_NOVALIDENTRY) { //goto the next queue
				QueueId = EH_ACTIVEQUEUEMASK;
				cmpFunc = CmpReturnFirst;
			}
			break:
		case EH_ACTIVEQUEUEMASK:
			if (eh == E_NOVALIDENTRY) { //goto the next queue
				QueueId = EH_TIMEDQUEUEMASK;
				cmpFunc = CmpReturnFirst;
			}
			break;
		case EH_TIMEDQUEUEMASK:
			erc = ercNoEntryAvail;
			goto AllDone;
			break;
		default:
			erc = ercInconsistency;
			DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
			goto AllDone;
			break;
		} //switch
	} //subsequent call

	pQueueEntry = (QueueEntry_t *) zmalloc(pQ->sEntrySize);

}
#endif

DWORD H_ReadNextQueueEntry(DWORDLONG QEH, char *pQueueName, 
						   BYTE *pbEntryRet, WORD cbEntryRet,
						   BYTE *StatusBlock, WORD sStatusBlock)
{
	QueueEntry_t *pQueueEntry=NULL;
	QEH_t *pQeh = (QEH_t *) &QEH;
	QueueHeader_t *pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
												   CmpQueueName,
												   (void *) pQueueName);
	DWORD erc, iEntry, lfa;
	BOOL fSuccess;
	QueueEntryStatus_t *pStatus = (QueueEntryStatus_t *) StatusBlock;


	if (!pQ) {
		return(ercBadQueSpec);
	}

	if (pQ->cEntries == 0) { //non-block
		return(ercNoEntryAvail);
	}

	if (pQeh->qh == 0) { //first entry
		iEntry = pQ->iEntryFirst;
	} else { //subsequent entry
		//sanity
		if (pQeh->qh != (DWORD) pQ->qh) {
			DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
			return(ercInconsistency);
		}
		iEntry = pQeh->eh;
	}

	if (iEntry == 0xffff)
		return(ercNoEntryAvail);

	pQueueEntry = (QueueEntry_t *) zmalloc(pQ->sEntrySize);

	//sanity
	if ((lfa = sizeof(QueueHeader_t) + pQ->sEntrySize * iEntry) >= pQ->fileSize) {
		erc = ercInconsistency;
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		goto AllDone;
	}

	fSuccess = GetEntry(pQ->qh, (void *) pQueueEntry, pQ->sEntrySize, lfa);
	if (!fSuccess) {
		erc = GetLastError();
		if (!erc)
			erc = ercInconsistency;
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		goto AllDone;
	}

	memcpy(pbEntryRet, pQueueEntry->rgEntry, MIN(pQueueEntry->cEntry, cbEntryRet));

	//TO_DO for the benefit of my test programs PLEASE WITHDRAW THIS ...

	if (pQueueEntry->cEntry < cbEntryRet)
		*(pbEntryRet+pQueueEntry->cEntry) = 0;

	//...for the benefit of my test programs

	
	//TO_DO we could be shooting ourselves in the leg by assuming that StatusBlock
	//is large enough to accomodate the structure QueueEntryStatus_t

	pStatus->qeh.qh = (DWORD) pQ->qh;
	pStatus->qeh.eh = pQueueEntry->eh;
	pStatus->priority = pQueueEntry->priority;
	pStatus->serverUserNum = pQueueEntry->markUserNum;

	pStatus->qehNext.qh = (DWORD) pQ->qh;
	pStatus->qehNext.eh = (DWORD) pQueueEntry->iNext;

	erc=ercOk;

AllDone:
	if (pQueueEntry) {
		free(pQueueEntry);
		pQueueEntry = NULL;
		}
	return(erc);
}

DWORD H_ReadKeyedQueueEntry(char *pQueueName, 
							WORD oKey1, BYTE *pKey1, WORD cKey1,
						    WORD oKey2, BYTE *pKey2, WORD cKey2,
							BYTE *pbEntryRet, WORD cbEntryRet,
							BYTE *StatusBlock, WORD sStatusBlock)
{
	QueueEntryStatus_t *pStatus = (QueueEntryStatus_t *) StatusBlock;
	BOOL fFound=FALSE;
	BOOL fKey1, fKey2;
	DWORD erc = ercOk;
	DWORDLONG QEH;

	fKey1 = ((pKey1 && cKey1)? FALSE : TRUE); //if no key is specified, the match is
											  //automatically met
	fKey2 = ((pKey2 && cKey2)? FALSE : TRUE);
	//sanity check, oKey1 and oKey2 have to be within the entry size TO_DO
	memset((void *)pStatus, 0, sizeof(QueueEntryStatus_t));

	
	while(!fFound && (erc == ercOk)) {
		BOOL fKeyCycle1, fKeyCycle2;

		memcpy((void *) &QEH, (void *)&pStatus->qehNext, sizeof(QEH_t));
		erc = H_ReadNextQueueEntry(QEH, pQueueName, pbEntryRet, cbEntryRet,
								   StatusBlock, sStatusBlock);
		if (erc)
			;
		else {
			if (!(fKeyCycle1 = fKey1))
				fKeyCycle1 = compareStrings(pbEntryRet+oKey1, pKey1, cKey1);
			if (!(fKeyCycle2 = fKey2))
				fKeyCycle2 = compareStrings(pbEntryRet+oKey2, pKey2, cKey2);

			fFound = fKeyCycle1 && fKeyCycle2;
		}
	}

	if (fFound)
		return(ercOk);
	else
		return(ercEntryNotFound);
}


DWORD H_GetQMStatus(WORD QueueType, BOOL fHealthCheck, BYTE *pStatusRet, WORD sStatusRet)
{
	QueueHeader_t *pQ;
	QueueStatus_t *pQueueStatus = (QueueStatus_t *) pStatusRet;
	QueueInfo_t *pQueueInfo;

	if (sStatusRet < sizeof(QueueStatus_t))
		return(ercQueueEntryOverflow);

	memset(pStatusRet, 0, sStatusRet);

	if (!(pQ = g_pQueues)) //yes, I know, it has to be pQ=pQueues and NOT pQ==pQueues
		return(ercOk);


	while(pQ) {

		if (pQ->queueType == QueueType) {
			pQueueStatus->nQueues++;
			pQueueStatus->cQueueFetches += pQ->nFetches;
			if (fHealthCheck) {
				//TO_DO check the integrity of the structure
			}
			if (sStatusRet > 
				(sizeof(QueueStatus_t) - 1 + pQueueStatus->nQueues*sizeof(QueueInfo_t))) {
				
				pQueueInfo = (QueueInfo_t *) (((char *) (pQueueStatus->rgQueues)) + 
								(pQueueStatus->nQueues-1)*sizeof(QueueInfo_t));
				pQueueInfo->qh = pQ->qh;
				pQueueInfo->cEntries = pQ->cEntries;
				pQueueInfo->cbQueueName = pQ->cbQueueName;
				strcpy((char *) pQueueInfo->QueueName, pQ->QueueName);	
				pQueueInfo->sEntrySize = pQ->sEntrySize;
				pQueueInfo->fContaminated = FALSE; //TO_DO
			}
		} //if correct QueueType
		pQ = pQ->Next;
	} //while more queues of this type

	return(ercOk);

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
|The following needs to be terminated for a user				|
|- All Queues served by this user have to be un-served			|
|- All Entries marked by this user have to be unmarked			|
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
// TerminateUser() is only called by PipeServer() - the main thread.
void TerminateUser(DWORD userNum)
{
	QueueHeader_t *pQ;
	QueueEntry_t *pQueueEntry;
	DWORDLONG qeh;
	QEH_t *pQeh = (QEH_t *) &qeh;
	DWORD erc;
	WORD i;

	if (!(pQ = g_pQueues)) //yes, I know, it has to be pQ=pQueues and NOT pQ==pQueues
		return;

	while(pQ) {
		for(i=0;(i<pQ->cQueuedRequests);i++) {
			if (pQ->rgQueuedRequests[i]->RqHdr.userNum == userNum) {
				//drop this on the floor
				//note, we assume that only one request from a user is pending, which
				//is valid in this design
				for(; (i<(pQ->cQueuedRequests-1)); i++) {
					pQ->rgQueuedRequests[i] = pQ->rgQueuedRequests[i+1];
				}
				pQ->rgQueuedRequests[pQ->cQueuedRequests-1] = 0;
				pQ->cQueuedRequests--;
				break; //break out of the for loop
			} //if
		} //for

		erc = H_TerminateQueueServer(pQ->QueueName, userNum);

		while(pQueueEntry = (QueueEntry_t *) 
										ListScan((void **) &pQ->pMarkedQueues,
												 CmpMarkUserNum, (void *) &userNum)) {
			pQeh->qh = (DWORD) pQ->qh;
			pQeh->eh = pQueueEntry->eh | EH_MARKEDQUEUEMASK;
			erc = H_UnmarkQueueEntry(pQ->QueueName, qeh, userNum);
		}
		pQ = pQ->Next;
	} //while more queues
}

DWORD H_RewriteMarkedQueueEntry(DWORDLONG QEH, char *pQueueName, 
								BYTE *pbEntry, WORD cbEntry,
								DWORD userNum)
{
	QEH_t *pQeh = (QEH_t *) &QEH;
	QueueHeader_t *pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
												   CmpQueueName,
												   (void *) pQueueName);
	QueueEntry_t *pQueueEntry, *pBuff;
	DWORD lfa, erc=ercOk;
	BOOL fSuccess;

	if (!pQ)
		return(ercBadQueSpec);

	if (pQeh->qh != (DWORD) pQ->qh)
		return(ercBadQeh);

	if ((pQeh->eh & EH_MARKEDQUEUEMASK) == 0) //not a marked entry
		return(ercBadQeh);

	pQeh->eh &= (~EH_MARKEDQUEUEMASK);

	pQueueEntry = (QueueEntry_t *) ListScan((void **) &pQ->pMarkedQueues,
											 CmpQEH, (void *) &pQeh->eh
											);

	if (!pQueueEntry)
		return(ercEntryNotFound);

	//we should validate that this QueueEntry is marked by this user TO_DO

	pBuff = (QueueEntry_t *) zmalloc(pQ->sEntrySize);
	fSuccess = GetEntry(pQ->qh, (void *) pBuff, pQ->sEntrySize,
				(lfa = sizeof(QueueHeader_t) + pQ->sEntrySize*pQueueEntry->iEntry));

	if (!fSuccess) {
		free(pBuff);
		pBuff = NULL;
		erc = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
		goto AllDone;
	}

	pQueueEntry->cEntry = pBuff->cEntry = cbEntry;
	memcpy(((BYTE *) pBuff)+offsetof(QueueEntry_t, rgEntry), pbEntry, cbEntry);
	fSuccess = PutEntry(pQ->qh, (void *) pBuff, pQ->sEntrySize, lfa);

	free(pBuff);
	pBuff = NULL;

	if (!fSuccess) {
		erc = GetLastError();
		DEBUG_BREAK((char *)__FILE__,__LINE__, erc);
	}
AllDone:

	return erc;
}

DWORD H_RemoveKeyedQueueEntry(char *pQueueName, WORD oKey1, BYTE *pKey1, WORD cKey1,
							  WORD oKey2, BYTE *pKey2, WORD cKey2)
{
	DWORD erc;
	QueueEntryStatus_t StatusBlock;
	BYTE *entry=NULL; //we don't need the exact size
	QueueHeader_t *pQ = (QueueHeader_t *) ListScan((void **) &g_pQueues, 
												   CmpQueueName,
												   (void *) pQueueName);
	QueueEntry_t *pQueueEntry;
	DWORD iQueue;

	if (!pQ)
		return(ercBadQueSpec);

	memset(&StatusBlock, 0, sizeof(QueueEntryStatus_t));
	entry = (BYTE *) zmalloc(pQ->sEntrySize);

	erc = H_ReadKeyedQueueEntry(pQueueName,  oKey1, pKey1, cKey1,
								oKey2, pKey2, cKey2,
								entry, (WORD) pQ->sEntrySize,
								(BYTE *) &StatusBlock, sizeof(QueueEntryStatus_t)
							   );
	if (erc != ercOk) 
		goto AllDone;

	pQueueEntry = (QueueEntry_t *) ListScan((void **) &pQ->pMarkedQueues, CmpQEH, (void *) &StatusBlock.qeh.eh);
	if (pQueueEntry)
		return(ercEntryMarked);
	pQueueEntry = (QueueEntry_t *) ListScan((void **) &pQ->pActiveQueues, CmpQEH, (void *) &StatusBlock.qeh.eh);
	if (pQueueEntry)
		iQueue = EH_ACTIVEQUEUEMASK;
	else {
		//sanity ...
		pQueueEntry = (QueueEntry_t *) ListScan((void **) &pQ->pTimedQueues, CmpQEH, (void *) &StatusBlock.qeh.eh);
		if (!pQueueEntry) {
			erc = ercBadQh;
			goto AllDone;
		}
		//...sanity
		iQueue = EH_TIMEDQUEUEMASK;
	}
	erc = RemoveQueueEntry(pQ, pQueueEntry, iQueue);

AllDone:
	if(entry) {
		free(entry);
		entry = NULL;
		}
	return(erc);
}


BOOL ActivateEntry(QueueHeader_t *pQ, QueueEntry_t *pQueueEntry)
{
	QueueEntry_t *pEntry;

	pEntry = (QueueEntry_t *) ListRemove((void **) &pQ->pTimedQueues, 
										 (void *) pQueueEntry);

	//sanity
	if (pEntry != pQueueEntry) {
		DEBUG_BREAK((char *)__FILE__, __LINE__, ercInconsistency);
		return(FALSE);
	}
	pQ->cTimedEntries--;

	pEntry = (QueueEntry_t *) AddQueueEntryToList((void **)&pQ->pActiveQueues, 
												  pQueueEntry);

	//sanity
	if (pEntry != pQueueEntry) {
		DEBUG_BREAK((char *)__FILE__, __LINE__, ercInconsistency);
		return(FALSE);
	}
	pQ->cActiveEntries++;

	RespondToQueuedMarkRequests(pQ);

	return(TRUE);
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - *\
| We have to go through every Queue to find all the QueueEntries  |
| that may have become active at this time.                       |
\* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void ProcessTimer(BOOL fCheckTime)
{
	static BOOL fFirstTime=TRUE;
	static ctosDateTimeType lastTimerDateTime;
	ctosDateTimeType nextTimerDateTime;
	QueueHeader_t *pQ;
	QueueEntry_t *pQueueEntry, *pQueueEntryNext;
	DWORD erc;
	ctosDateTimeType currentDateTime;
	BOOL fElapsedTime, fSuccess;

	if (fFirstTime) {
		CTOSTimeFromSystem(0, &lastTimerDateTime);
		fFirstTime = FALSE;
	}

	if (fCheckTime) {
		memcpy(&nextTimerDateTime, &lastTimerDateTime, sizeof(ctosDateTimeType));
		IncrementCtosTime(&nextTimerDateTime, WATCHDOG_INTERVAL);
		CTOSTimeFromSystem(0, &currentDateTime);
		fElapsedTime = CompareTime(nextTimerDateTime, &currentDateTime);
		if (!fElapsedTime)
			return;
	}

	if (!(pQ = g_pQueues))  {//yes, I know, it has to be pQ=pQueues and NOT pQ==pQueues
		CTOSTimeFromSystem(0, &lastTimerDateTime);
		return;
	}

	erc = CTOSTimeFromSystem(0, &currentDateTime);
	if (erc) {
		//fatal error
		DEBUG_BREAK((char *)__FILE__,__LINE__, ercInconsistency);
		return;
	}

	while(pQ) {
		pQueueEntry = pQ->pTimedQueues;
		while(pQueueEntry) {
			fElapsedTime = CompareTime(pQueueEntry->dateTime, &currentDateTime);
			//get the next entry now, because if this is entry is activated, then
			//the new links are going to be different
			pQueueEntryNext = pQueueEntry->Next;
			if (fElapsedTime)
				fSuccess = ActivateEntry(pQ, pQueueEntry);
			pQueueEntry = pQueueEntryNext;
		} //while more TimedQueues

		pQ = pQ->Next;
	} //while more queues

	CTOSTimeFromSystem(0, &lastTimerDateTime);
}




