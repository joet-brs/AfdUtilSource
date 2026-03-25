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
'    Module       :  AFDCommMonitor DLL
'    Date         :  Feb. 1998
'
'    File Name    :  AfdCommMonitor.c
'
'    Purpose      :  This provides support to the VB based AfdStatusMonitor.
'                    It provides the functions needes to check the queues,
'                    delete entries, move entries from Hold to Active, etc..
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
9/July/1998 AA	Changed mode by which QueueEntries are deleted.
				Use RemoveKeyedQueueEntry() if possible, else
				hand the command over to the gateway.
				Added code for RealTime Status support. GetCommRTStatus().

10/Aug/1998	AA	Added code to report back if a purge is pending on a BSC
				entry. This code exposed a bug in the ReadKeyedQueueEntry()
				due to the function compareStrings(). A corresponding fix has 
				been made. Th e symptom of this problem will be that when a purge
				command is issued on _any_ entry in a queue, all entries in the 
				queue will be reported back as "Purge pending ...". LIMS.

20/July/1999 JRM Added code for:
					SCR#634 Allow release for files in transmit queue
					SCR#635 Be able to put a file on hold from Transmit Queue (BiSync)
					No SCR # Hold pending... Message to make BSC transmit queue clearer

*/

#include "basic.h"
#include <stddef.h>
#include <stdio.h>
#include "include.h"
#include "QMProto.h"
#include "QMErcs.h"
#include "AfdCommMonitor.h"
#include "AfdBiSyncIntCmds.h"
#define COMM_MEMMAP
#include "AfdMisc.h"

typedef struct {
	BYTE *pQueueName;
	WORD cQueueName;
} QueueNames_t;

QueueEntryStatus_t QueueEntryStatus;
DWORDLONG qeh=0;

QueueNames_t AfdCommQueues[3];
BYTE QNameRx[] = {"AfdCommReceive"}; //default QueueName
BYTE QNameTx[] = {"AfdCommTransmit"}; //default QueueName
BYTE QNameHl[]= {"AfdCommHoldQueue"}; //default CommHoldQueue
WORD QTypeAfd = 170;				   //AFD Queue Type
BOOL fHoldQueueServer = FALSE;

BYTE currQueueName[34];

WORD wCommand=0x4141;
WORD wCommandHold=0x4242;

/* The user is expected to make these calls repeatedly, till all entries are exhausted.
This function does not maintain any internal state of which queue was being perused, and
try to validate that. It uses the global QueyeEntryStatus to store current search level
*/


// AllTrim: Moves all control characters and spaces from both ends of
// a C string.
void RTrim(char *buffer, WORD sBuffer) {
	
	WORD iIndex;

	for (iIndex=sBuffer-1;(iIndex != 0);iIndex--) {
		if (*(buffer+iIndex) > 0x20)
			break;
	}
	if (iIndex != 0)
		*(buffer+iIndex+1) = 0;	
}

WORD GetTPType(char *ServerId) {
	
	if (*ServerId == 'B' || *ServerId == 'b')
		return(TP_BISYNC);

	if (*ServerId == 'M' || *ServerId == 'm') {
		// added the if-else stmt below for PcComm modem pooling
		// GDP - 10Sep01
		if ( *(ServerId + 1) == 'B' || *(ServerId + 1) == 'b' )
			return (TP_BISYNC);
		else
			return (TP_ASYNC);
	}

	if (*ServerId == 'P' || *ServerId == 'p')
		return(TP_ASYNC);

	if (*ServerId == 'S' || *ServerId == 's')
		return(TP_SNA);

	return(0xffff);
}

DWORD Async_HandleHoldEntry(BYTE *QueueName, WORD iAction, T_PCCQENTRY *pPcCommEntry) {
	DWORD erc=ercOk;
	afdRespQEntry_t AfdResponse;
	void *pRespEntry;
	WORD sRespEntry;
	
	// allow ASYNC to be PUT ON HOLD - GDP - 14Sep01
	if ( (iAction == A_REQUEUE) || (iAction == A_PUTONHOLD) ) {
		if ( iAction == A_REQUEUE ) {
			pPcCommEntry->cbHold = 0;
			pPcCommEntry->Hold = 0;
		}
		else {
			pPcCommEntry->cbHold = 1;
			pPcCommEntry->Hold = TRUE;
		}

		erc = AddQueueEntry(QueueName, (WORD) strlen(QueueName), TRUE /*fQueueIfNoServer*/, 0 /*priority*/, QTypeAfd, 
							(void *) pPcCommEntry, sizeof(T_PCCQENTRY),
							NULL/*DateTime*/, 0 /*repeatTime*/);
	} else {
		if (pPcCommEntry->sbAfdRspQName[0] == 0) //we do not have a valid QName to respond to
			goto AllDone;
		
		if (pPcCommEntry->fAFD) { //we have to respond to the AFDServer
			memset(&AfdResponse, 0, sizeof(afdRespQEntry_t));
			memcpy(&AfdResponse.sbJobFSpec[1], pPcCommEntry->JobFileSpec, 
				   sizeof(AfdResponse.sbJobFSpec));
			AfdResponse.sbJobFSpec[0] = pPcCommEntry->cbJobFileSpec;
			AfdResponse.CtosErc = erc_EntryDeletedByMonitor;
			AfdResponse.UserErc = erc_EntryDeletedByMonitor;

			pRespEntry = (void *) &AfdResponse;
			sRespEntry = sizeof(afdRespQEntry_t);
		} else {
			/*pPcCommEntry->erc = erc_EntryDeletedByMonitor;
			pPcCommEntry->ercDetail = erc_EntryDeletedByMonitor;
			pRespEntry = (void *) pPcCommEntry;
			sRespEntry = sizeof(T_PCCQENTRY);*/
		}
		
		erc = AddQueueEntry((void *) &pPcCommEntry->sbAfdRspQName[1], 
							(WORD) pPcCommEntry->sbAfdRspQName[0], 
						    TRUE /*fQueueIfNoServer*/, 0 /*priority*/, QTypeAfd, 
						    pRespEntry, sRespEntry, NULL /*DateTime*/, 0 /*repeatTime*/);
	}
AllDone:
	return(ercOk);
}

DWORD BiSync_HandleHoldEntry(BYTE *QueueName, WORD iAction, 
							 AfdBiSyncQueue_t *pBiSyncEntry) {

	DWORD erc=ercOk;
	afdRespQEntry_t AfdResponse;
	void *pRespEntry;
	WORD sRespEntry;
	
	if ((iAction == A_REQUEUE) || (iAction == A_PUTONHOLD)) {
		if (iAction == A_REQUEUE)
			pBiSyncEntry->fHold = FALSE;
		else
			pBiSyncEntry->fHold = TRUE;

		erc = AddQueueEntry(QueueName, (WORD) strlen(QueueName), TRUE /*fQueueIfNoServer*/, 0 /*priority*/, QTypeAfd, 
							(void *) pBiSyncEntry, sizeof(AfdBiSyncQueue_t),
							NULL/*DateTime*/, 0 /*repeatTime*/);
	} else {
		if (pBiSyncEntry->sbRspQName[0] == 0) //we do not have a valid QName to respond to
			goto AllDone;
		
		if (pBiSyncEntry->AfdServerNum != 0) { //we have to respond to the AFDServer
			memset(&AfdResponse, 0, sizeof(afdRespQEntry_t));
			memcpy(AfdResponse.sbJobFSpec, pBiSyncEntry->sbXmtFSpec, 
				   sizeof(AfdResponse.sbJobFSpec));
			AfdResponse.CtosErc = erc_EntryDeletedByMonitor;
			AfdResponse.UserErc = erc_EntryDeletedByMonitor;

			pRespEntry = (void *) &AfdResponse;
			sRespEntry = sizeof(afdRespQEntry_t);
		} else {
			pBiSyncEntry->erc = erc_EntryDeletedByMonitor;
			pBiSyncEntry->ercDetail = erc_EntryDeletedByMonitor;
			pRespEntry = (void *) pBiSyncEntry;
			sRespEntry = sizeof(AfdBiSyncQueue_t);
		}
		
		erc = AddQueueEntry((void *) &pBiSyncEntry->sbRspQName[1], 
							(WORD) pBiSyncEntry->sbRspQName[0], 
						    TRUE /*fQueueIfNoServer*/, 0 /*priority*/, QTypeAfd, 
						    pRespEntry, sRespEntry, NULL /*DateTime*/, 0 /*repeatTime*/);
	}

AllDone:
	return(ercOk);
}

DWORD ActionOnHoldQueue(BYTE *QueueName, BYTE *OriginQueueName, WORD TPType, WORD iAction, 
						AfdCommRec_t *AfdCommRec) {
	DWORD erc;
	WORD oFilespec=0;
	WORD oQueuename;
	QueueEntryStatus_t lclQEStatus;
	BYTE QEntry[3*512];
	AfdBiSyncQueue_t *pBiSyncEntry;
	T_PCCQENTRY *pPcCommEntry;

	oQueuename = offsetof(AfdCommHoldQueue_t, OriginQName);
	switch (TPType) {
	case TP_ASYNC:
		oFilespec = offsetof(T_PCCQENTRY, JobFileSpec);
		break;
	case TP_BISYNC:
		oFilespec = offsetof(AfdBiSyncQueue_t, sbXmtFSpec)+1;
		break;
	case TP_SNA:
		//no hold for sna, we should not come here
		break;
	}
	oFilespec += offsetof(AfdCommHoldQueue_t, rgQueue);
	/*DWORD MarkKeyedQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
						  LPVOID pbKey1, WORD cbKey1, WORD oKey1, 
						  LPVOID pbKey2, WORD cbKey2, WORD oKey2, 
						  LPVOID pEntryRet, WORD sEntryRet, 
						  LPVOID pStatusBlock, WORD sStatusBlock);*/
	erc = MarkKeyedQueueEntry(QueueName, (WORD) strlen(QueueName),
							  OriginQueueName, (WORD) strlen(OriginQueueName), oQueuename,
							  AfdCommRec->Filename, (WORD) strlen(AfdCommRec->Filename), oFilespec,
							  (void *) QEntry, sizeof(QEntry),
							  &lclQEStatus, sizeof(QueueEntryStatus_t));
	if (erc != ercOk)
		return(erc_EntryNotFound);

	pBiSyncEntry = (AfdBiSyncQueue_t *) ((AfdCommHoldQueue_t *) &QEntry)->rgQueue;
	pPcCommEntry = (T_PCCQENTRY *) ((AfdCommHoldQueue_t *)&QEntry)->rgQueue;

	//in either case, we have to remove entry from the HoldQueue

	erc = RemoveMarkedQueueEntry(QueueName, (WORD) strlen(QueueName), lclQEStatus.qeh);
	if (erc != ercOk)
		return(erc);

	switch(TPType) {
	case TP_ASYNC:
		erc = Async_HandleHoldEntry(OriginQueueName, iAction, pPcCommEntry);
		break;
	case TP_BISYNC:
		erc = BiSync_HandleHoldEntry(OriginQueueName, iAction, pBiSyncEntry);
		break;
	case TP_SNA:
		//no hold for sna, we should not come here
		break;
	}
	return(erc);
}

DWORD DeleteAsyncEntry(BYTE *QueueName, WORD iQueue, AfdCommRec_t *AfdCommRec, WORD iAction) {

	T_PCCQENTRY PCCommQueue;
	QueueEntryStatus_t lclQEStatus;
	afdRespQEntry_t AfdResponse;
	WORD oFilespec;
	DWORD erc=ercOk;
	void *pRespEntry;
	WORD sRespEntry;

#ifdef NoDeleteOfTimedQueues
	erc = EstablishQueueServer(QueueName, (WORD) strlen(QueueName), 
							   QTypeAfd, FALSE /*not unique*/);
	if (erc != ercOk)
		return(erc);
#endif

	if (iQueue == IQ_TX)
		oFilespec = offsetof(T_PCCQENTRY, JobFileSpec);
	else //IQ_RX:
		oFilespec = offsetof(T_PCCQENTRY, ResultFileSpec);

	memset(&PCCommQueue, 0, sizeof(T_PCCQENTRY));
	memset(&lclQEStatus, 0, sizeof(lclQEStatus));
#ifdef NoDeleteOfTimedQueues
	erc = MarkKeyedQueueEntry(QueueName, (WORD) strlen(QueueName),
							  AfdCommRec->Filename, (WORD) strlen(AfdCommRec->Filename), oFilespec,
							  NULL, 0, 0,
							  &PCCommQueue, sizeof(PCCommQueue),
							  &lclQEStatus, sizeof(lclQEStatus));
	if (erc != ercOk) {
		erc = erc_EntryNotFound;
		goto AllDone;
	}

	erc = RemoveMarkedQueueEntry(QueueName, (WORD) strlen(QueueName), lclQEStatus.qeh);
	if (erc != ercOk) {
		erc = erc_EntryNotFound;
		goto AllDone;
	}
#else
	erc = ReadKeyedQueueEntry(QueueName, (WORD) strlen(QueueName),
							  AfdCommRec->Filename, (WORD) strlen(AfdCommRec->Filename), oFilespec,
							  NULL, 0, 0,
							  &PCCommQueue, sizeof(PCCommQueue),
							  &lclQEStatus, sizeof(lclQEStatus));
	if (erc != ercOk) {
		erc = erc_EntryNotFound;
		goto AllDone;
	}
	erc = RemoveKeyedQueueEntry(QueueName, (WORD) strlen(QueueName),
								AfdCommRec->Filename, (WORD) strlen(AfdCommRec->Filename), oFilespec,
								NULL, 0, 0);
	if (erc != ercOk) {
		erc = erc_EntryNotFound;
		goto AllDone;
	}
#endif
	// allow ASYNC to be PUT ON HOLD - GDP - 14Sep01
	if ( (iAction == A_REQUEUE) || (iAction == A_PUTONHOLD) )
	{
		erc = Async_HandleHoldEntry(QueueName, iAction, &PCCommQueue);
		goto AllDone;
	}

	if (PCCommQueue.sbAfdRspQName[0] == 0) {//we do not have a valid QName to respond to
		erc = ercOk;
		goto AllDone;
	}
			
	if (PCCommQueue.fAFD) {
		memset(&AfdResponse, 0, sizeof(afdRespQEntry_t));
		memcpy(&AfdResponse.sbJobFSpec[1], PCCommQueue.JobFileSpec, 
			   sizeof(AfdResponse.sbJobFSpec));
		AfdResponse.sbJobFSpec[0] = PCCommQueue.cbJobFileSpec;
		AfdResponse.CtosErc = erc_EntryDeletedByMonitor;
		AfdResponse.UserErc = erc_EntryDeletedByMonitor;

		pRespEntry = (void *) &AfdResponse;
		sRespEntry = sizeof(afdRespQEntry_t);
	} else {
			/*pPcCommEntry->erc = erc_EntryDeletedByMonitor;
			pPcCommEntry->ercDetail = erc_EntryDeletedByMonitor;
			pRespEntry = (void *) pPcCommEntry;
			sRespEntry = sizeof(T_PCCQENTRY);*/
	}
		
	erc = AddQueueEntry((void *) &PCCommQueue.sbAfdRspQName[1], 
						(WORD) PCCommQueue.sbAfdRspQName[0], 
						TRUE /*fQueueIfNoServer*/, 0 /*priority*/, QTypeAfd, 
						pRespEntry, sRespEntry, NULL /*DateTime*/, 0 /*repeatTime*/);
AllDone:
#ifdef NoDeleteOfTimedQueues
	TerminateQueueServer(QueueName, (WORD) strlen(QueueName));
#endif

	return(erc);

}

BOOL BiSyncCheckIfPurgePending(BYTE *QueueName, DWORDLONG qeh) {

	AfdBiSyncQueue_t BSCQueue;
	QueueEntryStatus_t lclQEStatus;
	WORD oQeh, oCmd;
	DWORD erc;

	oQeh = offsetof(AfdBiSyncQueue_t, qeh);
	oCmd = offsetof(AfdBiSyncQueue_t, QTrailer);

	memset(&BSCQueue, 0, sizeof(AfdBiSyncQueue_t));
	memset(&lclQEStatus, 0, sizeof(QueueEntryStatus_t));


	erc = ReadKeyedQueueEntry(QueueName, (WORD) strlen(QueueName),
							  (void *) &qeh, (WORD) sizeof(DWORDLONG), oQeh,
							  (void *) &wCommand, (WORD) sizeof(WORD), oCmd,
							  &BSCQueue, sizeof(AfdBiSyncQueue_t),
							  &lclQEStatus, sizeof(QueueEntryStatus_t));
	if (erc == ercOk)
		return(TRUE);
	else
		return(FALSE);
}

BOOL BiSyncCheckIfHoldPending(BYTE *QueueName, DWORDLONG qeh) {

	AfdBiSyncQueue_t BSCQueue;
	QueueEntryStatus_t lclQEStatus;
	WORD oQeh, oCmd;
	DWORD erc;

	oQeh = offsetof(AfdBiSyncQueue_t, qeh);
	oCmd = offsetof(AfdBiSyncQueue_t, QTrailer);

	memset(&BSCQueue, 0, sizeof(AfdBiSyncQueue_t));
	memset(&lclQEStatus, 0, sizeof(QueueEntryStatus_t));


	erc = ReadKeyedQueueEntry(QueueName, (WORD) strlen(QueueName),
							  (void *) &qeh, (WORD) sizeof(DWORDLONG), oQeh,
							  (void *) &wCommandHold, (WORD) sizeof(WORD), oCmd,
							  &BSCQueue, sizeof(AfdBiSyncQueue_t),
							  &lclQEStatus, sizeof(QueueEntryStatus_t));
	if (erc == ercOk)
		return(TRUE);
	else
		return(FALSE);
}

DWORD DeleteBiSyncEntry(BYTE *QueueName, WORD iQueue, AfdCommRec_t *AfdCommRec, WORD iAction) {

#define PURGE_PRIORITY 9
	static BYTE rgDeleteMsg[] = {"Entry Marked For Deletion"};

	BYTE sbFileName[94];
	AfdBiSyncQueue_t BSCQueue;
	QueueEntryStatus_t lclQEStatus;
	WORD oFilespec;
	DWORD erc;

	if (iQueue == IQ_TX)
		oFilespec = offsetof(AfdBiSyncQueue_t, sbXmtFSpec);
	else //IQ_RX:
		oFilespec = offsetof(AfdBiSyncQueue_t, sbRcvFSpec);

	memset(&BSCQueue, 0, sizeof(AfdBiSyncQueue_t));
	memset(sbFileName, 0, sizeof(sbFileName));
	strcpy(sbFileName+1, AfdCommRec->Filename);
	sbFileName[0] =  (char) strlen(AfdCommRec->Filename);
	memset(&lclQEStatus, 0, sizeof(QueueEntryStatus_t));
	erc = ReadKeyedQueueEntry(QueueName, (WORD) strlen(QueueName),
							  sbFileName, (WORD) (sbFileName[0]+1), oFilespec,
							  NULL, 0, 0,
							  &BSCQueue, sizeof(AfdBiSyncQueue_t),
							  &lclQEStatus, sizeof(QueueEntryStatus_t));
	if (erc)
		return(erc_EntryNotFound);
#ifndef NoDeleteOfTimedQueues
	erc = RemoveKeyedQueueEntry(QueueName, (WORD) strlen(QueueName),
								sbFileName, (WORD) (sbFileName[0]+1), oFilespec,
								NULL, 0, 0);
	if (erc == ercOk)
		BiSync_HandleHoldEntry(QueueName, iAction, &BSCQueue);
	else
		if ((erc == ercEntryMarked) && ((iAction == A_PURGE) || (iAction == A_PUTONHOLD))) { //entry is marked, send a message to AfdBiSync
#endif
			if (iAction == A_PURGE)
			{
				BSCQueue.QTrailer = 0x4141;
				strcpy(&BSCQueue.sbPhoneNumber[1], rgDeleteMsg);
				BSCQueue.sbPhoneNumber[0] = (BYTE) strlen(rgDeleteMsg);
				if (iQueue == IQ_TX)
					BSCQueue.BiSyncCmd = CMD_Purge_Tx;
				else //IQ_RX:
					BSCQueue.BiSyncCmd = CMD_Purge_Rx;
			}
			else
			{
				BSCQueue.QTrailer = 0x4242;
				BSCQueue.BiSyncCmd = CMD_Put_On_Hold_Tx;
				BSCQueue.fHold = TRUE;
			}

			BSCQueue.qeh = lclQEStatus.qeh;
	
			erc = AddQueueEntry(QueueName, (WORD) strlen(QueueName), TRUE, //fQueueIfNoServer
								PURGE_PRIORITY,	QTypeAfd, 
								(void *) &BSCQueue, (WORD) sizeof(AfdBiSyncQueue_t),
								NULL, //pDateTime
								(WORD) 0 //repeatTime
							   );
#ifndef NoDeleteOfTimedItems
	}
#endif
	return(erc);
}

DWORD DeleteSnaEntry(BYTE *QueueName, AfdCommRec_t *AfdCommRec) {

	Sna_QueueEntryType SnaQueue;
	QueueEntryStatus_t lclQEStatus;
	afdRespQEntry_t AfdResponse;
	WORD oFilespec;
	DWORD erc=ercOk;
	void *pRespEntry;
	WORD sRespEntry;

	erc = EstablishQueueServer(QueueName, (WORD) strlen(QueueName), 
							   QTypeAfd, FALSE /*not unique*/);
	if (erc != ercOk)
		return(erc);
	oFilespec = offsetof(Sna_QueueEntryType, JobFileSpec);

	memset(&lclQEStatus, 0, sizeof(lclQEStatus));
	memset(&SnaQueue, 0, sizeof(SnaQueue));
	erc = MarkKeyedQueueEntry(QueueName, (WORD) strlen(QueueName),
							  AfdCommRec->Filename, (WORD) strlen(AfdCommRec->Filename), oFilespec,
							  NULL, 0, 0,
							  &SnaQueue, sizeof(Sna_QueueEntryType),
							  &lclQEStatus, sizeof(lclQEStatus));
	if (erc != ercOk) {
		erc = erc_EntryNotFound;
		goto AllDone;
	}

	erc = RemoveMarkedQueueEntry(QueueName, (WORD) strlen(QueueName), lclQEStatus.qeh);
	if (erc != ercOk) {
		erc = erc_EntryNotFound;
		goto AllDone;
	}

	if (SnaQueue.sbAfdRspQName[0] == 0) {//we do not have a valid QName to respond to
		erc = ercOk;
		goto AllDone;
	}
			
	if (SnaQueue.AFD != 0) {
		memset(&AfdResponse, 0, sizeof(afdRespQEntry_t));
		memcpy(&AfdResponse.sbJobFSpec[1], SnaQueue.JobFileSpec, 
			   sizeof(AfdResponse.sbJobFSpec));
		AfdResponse.sbJobFSpec[0] = (BYTE) SnaQueue.cbJobFileSpec;
		AfdResponse.CtosErc = erc_EntryDeletedByMonitor;
		AfdResponse.UserErc = erc_EntryDeletedByMonitor;

		pRespEntry = (void *) &AfdResponse;
		sRespEntry = sizeof(afdRespQEntry_t);
	} else {
			/*pPcCommEntry->erc = erc_EntryDeletedByMonitor;
			pPcCommEntry->ercDetail = erc_EntryDeletedByMonitor;
			pRespEntry = (void *) pPcCommEntry;
			sRespEntry = sizeof(T_PCCQENTRY);*/
	}
		
	erc = AddQueueEntry((void *) &SnaQueue.sbAfdRspQName[1], 
						(WORD) SnaQueue.sbAfdRspQName[0], 
						TRUE /*fQueueIfNoServer*/, 0 /*priority*/, QTypeAfd, 
						pRespEntry, sRespEntry, NULL /*DateTime*/, 0 /*repeatTime*/);
AllDone:
	erc = TerminateQueueServer(QueueName, (WORD) strlen(QueueName));
	return(erc);
}


BOOL CopyASyncEntries(WORD iQueue, T_PCCQENTRY *QEntry, AfdCommRec_t *AfdCommRec) {
			
	memset(AfdCommRec, 0, sizeof(AfdCommRec_t));

	switch (iQueue) {
	// added IQ_HL below for handling PcComm
	// entries in the HoldQueue. GDP - 17Sep01
	case IQ_TX:
	case IQ_HL:
		memcpy(AfdCommRec->Filename, QEntry->JobFileSpec, QEntry->cbJobFileSpec);
		break;
	case IQ_RX:
		memcpy(AfdCommRec->Filename, QEntry->ResultFileSpec, QEntry->cbResultFileSpec);
		break;
	}

	memcpy(AfdCommRec->Jobname, QEntry->JobName, QEntry->cbJobName);

	// if QEntry is already OnHold 'plug' txt msg into Phonenumber
	// fld. this is for consistency with BSC. if we're looking at a
	// QEntry from the HoldQueue 'plug' Phonenumber back in.
	// GDP - 17Sep01
	if ( !QEntry->Hold || (iQueue == IQ_HL) ) {
		memcpy(AfdCommRec->Phonenumber, "T", 1);
		memcpy(&AfdCommRec->Phonenumber[1], QEntry->PhoneNumber, QEntry->cbPhone);
		sprintf(AfdCommRec->Phonenumber + (strlen(AfdCommRec->Phonenumber)),
				" (RC = %2d)", QEntry->Redial);
	}
	else
		memcpy ( AfdCommRec->Phonenumber, "Hold Pending ...", 16 );

	return(TRUE);
}


BOOL CopyBiSyncEntries(WORD iQueue, AfdBiSyncQueue_t *QEntry, AfdCommRec_t *AfdCommRec,
					   BYTE *QueueName, DWORDLONG qeh) {

	BOOL fPurgePending = FALSE;

	if (QEntry->BiSyncCmd == CMD_Purge_Tx || QEntry->BiSyncCmd == CMD_Purge_Rx || QEntry->BiSyncCmd ==  CMD_Put_On_Hold_Tx)
		return(FALSE);

	memset(AfdCommRec, 0, sizeof(AfdCommRec_t));

	switch (iQueue) {
	case IQ_TX:
		memcpy(AfdCommRec->Filename, &QEntry->sbXmtFSpec[1], QEntry->sbXmtFSpec[0]);
		break;
	case IQ_RX:
		memcpy(AfdCommRec->Filename, &QEntry->sbRcvFSpec[1], QEntry->sbRcvFSpec[0]);
		break;
	}
	memcpy(AfdCommRec->Jobname, &QEntry->sbJobName[1], QEntry->sbJobName[0]);
	if (qeh) //it is not a entry from the hold queue
		fPurgePending = BiSyncCheckIfPurgePending(QueueName, qeh);

	if (fPurgePending)
		strcpy(AfdCommRec->Phonenumber, "Purge pending...");
	else if ((qeh) && (BiSyncCheckIfHoldPending(QueueName, qeh) || (QEntry->fHold == TRUE)))
			strcpy(AfdCommRec->Phonenumber, "Hold pending...");
	else {
		memcpy(AfdCommRec->Phonenumber, "T", 1);
		memcpy(&AfdCommRec->Phonenumber[1], &QEntry->sbPhoneNumber[1], QEntry->sbPhoneNumber[0]);
		sprintf(AfdCommRec->Phonenumber+(strlen(AfdCommRec->Phonenumber)),
			    " (RC = %2d)", QEntry->Redial);
	}
	return(TRUE);
}

BOOL CopySnaEntries(Sna_QueueEntryType *QEntry, AfdCommRec_t *AfdCommRec) {
	memset(AfdCommRec, 0, sizeof(AfdCommRec_t));
	memcpy(AfdCommRec->Filename, QEntry->JobFileSpec, QEntry->cbJobFileSpec);
	strcpy(AfdCommRec->Phonenumber, "*");
	return(TRUE);
}


DWORD GetNextHoldEntry(BYTE *pQueue, BYTE *ServerId, WORD TP, DWORDLONG lclQeh, 
						 QueueEntryStatus_t *QEStatus, AfdCommRec_t *AfdCommRec) {
	DWORD erc;
	BYTE QEntry[3*512];
	AfdCommHoldQueue_t *CommHoldQueue = (AfdCommHoldQueue_t *) QEntry;
	BOOL fMore = TRUE;
#ifdef TEST
T_PCCQENTRY_HOLD *pc=(T_PCCQENTRY_HOLD *) QEntry;
BSCTransmit_t_Hold *BSC=(BSCTransmit_t_Hold *) QEntry;
#endif
	memset(QEntry, 0, sizeof(QEntry));

			   /*DWORD ReadNextQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
						 DWORDLONG qeh, 
						 LPVOID pEntryRet, WORD sEntryRet, 
						 LPVOID pStatusBlock, WORD sStatusBlock);*/
	do {
		erc = ReadNextQueueEntry(pQueue, (WORD) strlen(pQueue), lclQeh,
								 &QEntry, sizeof(QEntry),
								 QEStatus, sizeof(QueueEntryStatus_t));
		if (erc != ercOk) {
			if (erc == ercNoEntryAvail)
				erc = erc_EndOfList;
			else
				erc = erc_UndefinedError;
			return(erc);
		}

		if (stricmp(CommHoldQueue->ServerId, ServerId) == 0) {
			switch(TP) {
			case TP_ASYNC:
				// chg'd param below from IQ_TX to IQ_HL which will
				// affect chg's in CopyASyncEntries for handling PcComm
				// entries in the HoldQueue. GDP - 17Sep01
				CopyASyncEntries(IQ_HL, (T_PCCQENTRY *) &CommHoldQueue->rgQueue, 
								 AfdCommRec);
				break;
			case TP_BISYNC: //only TX entries go to the hold queue
				CopyBiSyncEntries(IQ_TX, (AfdBiSyncQueue_t *) &CommHoldQueue->rgQueue, 
								  AfdCommRec, NULL, 0);
				break;
			case TP_SNA:
				//no hold queue for SNA TP
				erc = erc_EndOfList;
				break;
			}
			fMore = FALSE;
		} else
			lclQeh = QEStatus->qehNext;

	} while (fMore);
	
	return(erc);
}

DWORD GetNextEntry(BYTE *pQueue, WORD iQueue, WORD TP, DWORDLONG lclQeh, 
				   QueueEntryStatus_t *QEStatus, AfdCommRec_t *AfdCommRec) {
	DWORD erc;
	BYTE QEntry[3*512];
	BOOL fSuccess=FALSE;

	do {
		memset(QEntry, 0, sizeof(QEntry));

		erc = ReadNextQueueEntry(pQueue, (WORD) strlen(pQueue), lclQeh,
								 &QEntry, sizeof(QEntry),
								 QEStatus, sizeof(QueueEntryStatus_t));
		
		if (erc != ercOk) {
			if (erc == ercNoEntryAvail)
				erc = erc_EndOfList;
			else
				erc = erc_UndefinedError;

			return(erc);
		}

		switch (TP) {
		case TP_ASYNC:
			fSuccess = CopyASyncEntries(iQueue, (T_PCCQENTRY *) &QEntry, AfdCommRec);
			break;

		case TP_BISYNC:
			fSuccess = CopyBiSyncEntries(iQueue, (AfdBiSyncQueue_t *) &QEntry, AfdCommRec,
										 pQueue, QEStatus->qeh);
			break;

		case TP_SNA:
			fSuccess = CopySnaEntries((Sna_QueueEntryType *) &QEntry, AfdCommRec);
			break;
		}

		if (!fSuccess)
			lclQeh = QEStatus->qehNext;

	} while (!fSuccess);

	return(erc);
}

_declspec(dllexport) DWORD GetCommRec(char *ServerId, WORD iQueue, 
									  BOOL fStart, AfdCommRec_t *AfdCommRec) 
{
	WORD TPType = GetTPType(ServerId);
	DWORD erc=0;
	BYTE QueueName[34];

	if (TPType == 0xffff)
		return(erc_BadTPType);
	if (iQueue > IQ_HL)
		return(erc_BadQueueType);
	if (fStart) {
		memset(currQueueName, 0, sizeof(currQueueName));
		qeh = 0;
		memset(&QueueEntryStatus, 0, sizeof(QueueEntryStatus));
	}

	strcpy(QueueName, AfdCommQueues[iQueue].pQueueName);
	if (iQueue != IQ_HL)
		if (strcat_s(QueueName, sizeof(QueueName), ServerId)) return(ERROR_NOT_ENOUGH_MEMORY);
	if (!fStart)
		if (strcmp(QueueName, currQueueName) != 0)
			return(erc_InconsistentCall);

	if (iQueue == IQ_TX || iQueue == IQ_RX)
		erc = GetNextEntry(QueueName, iQueue, TPType, qeh, &QueueEntryStatus, AfdCommRec);
	else
		erc = GetNextHoldEntry(QueueName, ServerId, TPType, qeh, &QueueEntryStatus,
							   AfdCommRec);

	if (erc == ercOk) {
		if(strcpy_s(AfdCommRec->ServerId, sizeof(AfdCommRec->ServerId), ServerId)) return(erc_UndefinedError);
		if (fStart)
			strcpy(currQueueName, QueueName);
		qeh = QueueEntryStatus.qehNext;
	}
	return(erc);		
}

_declspec(dllexport) DWORD ModifyCommRec(AfdCommRec_t *AfdCommRec, WORD iQueue, 
										 WORD iAction) 
{
	WORD TPType;
	DWORD erc=0;
	BYTE QueueName[34];
	BYTE OriginQueueName[34];

	RTrim(AfdCommRec->ServerId, sizeof(AfdCommRec->ServerId));
	RTrim(AfdCommRec->Filename, sizeof(AfdCommRec->Filename));
	RTrim(AfdCommRec->Jobname, sizeof(AfdCommRec->Jobname));

	TPType = GetTPType(AfdCommRec->ServerId);

	if (TPType == 0xffff)
		return(erc_BadTPType);

	if (iQueue > IQ_HL)
		return(erc_BadQueueType);

	if ((iAction != A_PURGE) && (iAction != A_REQUEUE) && (iAction != A_PUTONHOLD))
		return(erc_BadActionCode);

	if ((iAction == A_REQUEUE) && ((iQueue != IQ_HL) && (iQueue != IQ_TX)))
		return(erc_InconsistentActionCode);

	if ((iAction == A_PUTONHOLD) && (iQueue != IQ_TX))
		return(erc_InconsistentActionCode);

	// added ASYNC as an additional TPType that
	// can be placed into the hold queue - GDP - 12Sep01
	if ( iAction == A_PUTONHOLD ) {
		if ( (TPType != TP_BISYNC) && (TPType != TP_ASYNC) )
			return(erc_InconsistentActionCode);
	}

	strcpy(QueueName, AfdCommQueues[iQueue].pQueueName);
	if (iQueue != IQ_HL)
		strcat(QueueName, AfdCommRec->ServerId);
	if (iQueue == IQ_HL) {
		strcpy(OriginQueueName, AfdCommQueues[IQ_TX].pQueueName);
		strcat(OriginQueueName, AfdCommRec->ServerId);
	}
	//TO_DO Find a better way of doing this
	/*if (strcmp(QueueName, currQueueName) != 0)
		return(erc_InconsistentCall);*/

	if (iQueue == IQ_HL)
		erc = ActionOnHoldQueue(QueueName, OriginQueueName, TPType, iAction, AfdCommRec);
	else
		switch (TPType) {
		case TP_ASYNC:
			erc = DeleteAsyncEntry(QueueName, iQueue, AfdCommRec, iAction);
			break;
		case TP_BISYNC:
			erc = DeleteBiSyncEntry(QueueName, iQueue, AfdCommRec, iAction);
			break;
		case TP_SNA:
			erc = DeleteSnaEntry(QueueName, AfdCommRec);
			break;
	}

	return(erc);
}

_declspec(dllexport) DWORD GetCommRTStatus(void *pStatus, BYTE *ServerId) {

	CommStatusMemMap_t *pCommStatusMemMap=NULL;
	DWORD erc = ercOk;

	if (!pStatus || !ServerId)
		return(erc_CommStatusRTInvalidParam);

	pCommStatusMemMap = OpenCommStatusMap(ServerId);

	if (pCommStatusMemMap && pCommStatusMemMap->pbFile)
			if(memcpy_s(pStatus, sizeof(pStatus)-1, pCommStatusMemMap->pbFile, sizeof(CommStatusBlock_t))) return(erc_UndefinedError);
	else { //could not acquire the map
		memset(pStatus, 0, sizeof(CommStatusBlock_t));
		sprintf(((CommStatusBlock_t *) pStatus)->desc, "Status for %s is not available",
				ServerId);
		erc = erc_CommStatusRTNoStatus;
	}
	
	if (pCommStatusMemMap)
		CloseCommStatusMap(pCommStatusMemMap);

	return(erc);
}

////    DllMain
BOOL APIENTRY DllMain(HANDLE hDll, DWORD dwReason, LPVOID pStuff) 
{
	DWORD erc;

	AfdCommQueues[IQ_TX].pQueueName = QNameTx;
	AfdCommQueues[IQ_TX].cQueueName = strlen(QNameTx);
	AfdCommQueues[IQ_RX].pQueueName = QNameRx;
	AfdCommQueues[IQ_RX].cQueueName = strlen(QNameRx);
	AfdCommQueues[IQ_HL].pQueueName = QNameHl;
	AfdCommQueues[IQ_HL].cQueueName = strlen(QNameHl);
	qeh=0;
	memset(&QueueEntryStatus, 0, sizeof(QueueEntryStatus));
	erc = EstablishQueueServer(QNameHl, (WORD) strlen(QNameHl), 
							   QTypeAfd, FALSE /*not unique*/);
	fHoldQueueServer = (erc == ercOk);
	return TRUE;
}

