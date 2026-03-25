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
'    File Name    :  PlmAPI.c
'
'    Purpose      :  This file has the code for entry points for all the  
'                    AFDQueueManager APIs for Visual Basic applications.
'                    see QueueManagerDll.def also.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
9/July AA	Added function VbRemoveKeyedQueueEntry().
			Activated function VbRewriteMarkedQueueEntry().
*/

#include <windows.h>
#include <stdio.h>
#include "RqDefs.h"


static char ModId[] = {__FILE__}; //for debugging


DWORD WINAPI VbAddQueue(LPVOID pbQueueName, WORD cbQueueName, 
								    LPVOID pbFileSpec, WORD cbFileSpec, 
						 		    WORD wEntrysize, WORD wQueueType, LPVOID pQhRet)
{

	return(M_AddQueue(	pbQueueName, cbQueueName, 
						pbFileSpec, cbFileSpec, 
						wEntrysize, wQueueType, pQhRet
					  ));

}


DWORD WINAPI VbAddQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
										 BOOL fQueueIfNoServer, WORD priority,
										 WORD queueType, 
										 LPVOID pEntry, WORD sEntry, 
										 LPVOID pDateTime, WORD repeatTime)
{
	return(M_AddQueueEntry(	pbQueueName, cbQueueName, 
							fQueueIfNoServer, priority,
							queueType, pEntry, sEntry, 
							pDateTime, repeatTime
						   ));

}


DWORD WINAPI VbEstablishQueueServer(LPVOID pbQueueName, WORD cbQueueName, 
										 	    WORD queueType, BOOL fUniqueServer)
{
	return(M_EstablishQueueServer(	pbQueueName, cbQueueName, 
									queueType, fUniqueServer
								 ));

}


DWORD WINAPI VbGetQMStatus(WORD wQueueType, BOOL fHealthCheck, 
									   LPVOID pStatusRet, WORD sStatusMax)
{
	return(M_GetQMStatus(wQueueType, fHealthCheck, pStatusRet, sStatusMax));

}


DWORD WINAPI VbMarkKeyedQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
										 	   LPVOID pbKey1, WORD cbKey1, WORD oKey1, 
											   LPVOID pbKey2, WORD cbKey2, WORD oKey2, 
											   LPVOID pEntryRet, WORD sEntryRet, 
											   LPVOID pStatusBlock, WORD sStatusBlock)
{
	return(M_MarkKeyedQueueEntry(pbQueueName, cbQueueName, 
								 pbKey1, cbKey1, oKey1, 
								 pbKey2, cbKey2, oKey2, 
								 pEntryRet, sEntryRet, 
								 pStatusBlock, sStatusBlock
								));

}

DWORD WINAPI VbMarkNextQueueEntry(LPVOID pbQueueName, WORD cbQueueName,	
											  BOOL fReturnIfNoEntries, 
											  LPVOID pEntryRet, WORD sEntryRet, 
											  LPVOID pStatusBlock, WORD sStatusBlock)
{
	return(M_MarkNextQueueEntry(pbQueueName, cbQueueName,
								fReturnIfNoEntries, 
								pEntryRet, sEntryRet, 
								pStatusBlock, sStatusBlock
							   ));
	
}

DWORD WINAPI VbReadKeyedQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
											   LPVOID pbKey1, WORD cbKey1, WORD oKey1, 
											   LPVOID pbKey2, WORD cbKey2, WORD oKey2, 
											   LPVOID pEntryRet, WORD sEntryRet, 
											   LPVOID pStatusBlock, WORD sStatusBlock)
{
	return(M_ReadKeyedQueueEntry(pbQueueName, cbQueueName, 
								 pbKey1, cbKey1, oKey1, 
								 pbKey2, cbKey2, oKey2, 
								 pEntryRet, sEntryRet, 
								 pStatusBlock, sStatusBlock
								));
}

DWORD WINAPI VbReadNextQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
											  DWORDLONG qeh, 
											  LPVOID pEntryRet, WORD sEntryRet, 
											  LPVOID pStatusBlock, WORD sStatusBlock)
{
	return(M_ReadNextQueueEntry(pbQueueName, cbQueueName, 
								qeh, pEntryRet, sEntryRet, 
								pStatusBlock, sStatusBlock
							   ));

}

DWORD WINAPI VbRemoveKeyedQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
												 LPVOID pbKey1, 
												 WORD cbKey1, WORD oKey1, 
												 LPVOID pbKey2, 
												 WORD cbKey2, WORD oKey2)
{
	return(M_RemoveKeyedQueueEntry(pbQueueName, cbQueueName, 
								   pbKey1, cbKey1, oKey1, 
								   pbKey2, cbKey2, oKey2));
}

DWORD WINAPI VbRemoveMarkedQueueEntry(LPVOID pbQueueName, WORD cbQueueName,
												  DWORDLONG qeh)
{
	return(M_RemoveMarkedQueueEntry(pbQueueName, cbQueueName, qeh));

}


DWORD WINAPI VbReScheduleMarkedQueueEntry(LPVOID pbQueueName, 
													  WORD cbQueueName, 
													  DWORDLONG qeh)
{
	return(0);

}

DWORD WINAPI VbRewriteMarkedQueueEntry(LPVOID pbQueueName, 
												   WORD cbQueueName, 
												   DWORDLONG qeh, LPVOID pEntry, 
												   WORD sEntry)
{
	return(M_RewriteMarkedQueueEntry(pbQueueName, cbQueueName, qeh, 
									 pEntry, sEntry));

}

DWORD WINAPI VbTerminateQueueServer(LPVOID pbQueueName, WORD cbQueName)
{
	return(M_TerminateQueueServer(pbQueueName, cbQueName));

}

DWORD WINAPI VbUnmarkQueueEntry(LPVOID pbQueueName, WORD cbQueueName, 
										   DWORDLONG qeh)
{
	return(M_UnmarkQueueEntry(pbQueueName, cbQueueName, qeh));
}

DWORD WINAPI VbCleanQueue(DWORD qh)
{
	return(M_CleanQueue(qh));
}

DWORD WINAPI VbRemoveQueue(DWORD qh)
{
	return(M_RemoveQueue(qh));
}


char * WINAPI VbGetDLLVersion(BYTE *pVer)
{
	return(GetDllVer(pVer));
}

extern "C"{
BOOL GetQueueHandle(char *Queuename, WORD QType, HANDLE *qh);
}

BOOL WINAPI VbGetQueueHandle(char *Queuename, WORD QType, HANDLE *qh) 
{
	return(GetQueueHandle(Queuename, QType, qh));
}




