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
'    File Name    :  QMGetQh.c
'
'    Purpose      :  This file has the code for returning a QH associated 
'                    with a Queue.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

#include "basic.h"
#include "QmProto.h"

_declspec(dllexport) BOOL GetQueueHandle(char *Queuename, WORD QType, HANDLE *qh) 
{
	WORD i;
	DWORD erc;
	QueueStatus_t *pQMStatus;
	QueueInfo_t *pQueueInfo;
	BOOL fSuccess = FALSE;

	pQMStatus = (QueueStatus_t *) calloc(1,2048);
	if (!pQMStatus)
		return(FALSE);
	
	/*DWORD GetQMStatus(WORD wQueueType, BOOL fHealthCheck, 
				  LPVOID pStatusRet, WORD sStatusMax);*/
	
	erc = GetQMStatus(QType, TRUE, (void *) pQMStatus, 2048);
	// LSL 000426 pQMStatus was not being freed if GetQMStatus() failed!
	if (erc || (pQMStatus->nQueues == 0)) goto AllDone;
	pQueueInfo = (QueueInfo_t *) pQMStatus->rgQueues;
	for(i=0;(i<pQMStatus->nQueues);i++) {
		if (stricmp((char *) pQueueInfo->QueueName, Queuename) == 0) {
			*qh = pQueueInfo->qh;
			fSuccess = TRUE;
			goto AllDone;
		}
		pQueueInfo++;
	}
	fSuccess = FALSE;

AllDone:
	free(pQMStatus);
	pQMStatus = NULL;
	return(fSuccess);
}
