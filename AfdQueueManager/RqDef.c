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
'    File Name    :  RqDef.c
'
'    Purpose      :  This file has the format definitions for all the  
'                    AFDQueueManager APIs Request Blocks.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
9/July/1998 AA	Added RemoveKeyedQueueEntry definition. LIMS
*/

#include <windows.h>
#include "build.h"
#include "Req.h"

static char ModId[] = {__FILE__}; //for debugging


//add an array of pointers ref. by the rqcode here


unsigned short AddQueueFmt[] =
{
	RQ_AddQueueRequest,		//requestCode
	4,						//sCntlInfo
	2,						//nReqPbCb
	1,						//nRespPbCb
	6,						//nItems
	T_PBCB_REQ, 22,			//pbQueueName/cbQueueName
	T_PBCB_REQ, 28,			//pbFileSpec/cbFileSpec
	T_WORD,	18,				//wEntrySize
	T_WORD,	20,				//wQueueType
	T_POINTER_RESP, 34,		//QEHRet
	T_WCONST, 4				//constant value 4
};

unsigned short AddQueueEntryFmt[] =
{
	RQ_AddQueueEntryRequest,	//requestCode
	8,							//sCntlInfo
	3,							//nReqPbCb
	0,							//nRespPbCb
	7,							//nItems
	T_PBCB_REQ, 26,				//pbQueueName/cbQueueName
	T_WORD,	18,					//fQueueIfNoServer
	T_WORD,	20,					//priority
	T_WORD,	22,					//QueueType
	T_PBCB_REQ,	32,				//pEntry/sEntry
	T_SYSTIME, 38,				//DateTime 128 bits
	T_WORD, 24					//RepeatTime
};

unsigned short EstablishQueueServerFmt[] = 
{
	RQ_EstablishQueueServer,	//requestCode
	4,							//sCntlInfo
	1,							//nReqPbCb
	0,							//nRespPbCb
	3,							//nItems
	T_PBCB_REQ, 22,				//pbQueueName/cbQueueName
	T_WORD, 18,					//QueueType
	T_WORD, 20					//fUniqueServer
};

unsigned short MarkNextQueueEntryFmt[] = 
{
	RQ_MarkNextQueueEntry,		//requestCode
	2,							//sCntlInfo
	1,							//nReqPbCb
	2,							//nRespPbCb
	4,							//nItems
	T_PBCB_REQ, 20,				//pbQueueName/cbQueueName
	T_WORD, 18,					//fNoEntries
	T_PBCB_RESP, 26,			//pbcb EntryRet
	T_PBCB_RESP, 32				//pbcb StatusBlockRet
};

unsigned short RemoveMarkedQueueEntryFmt[] = 
{
	RQ_RemoveMarkedQueueEntry,	//requestCode
	8,							//sCntlInfo
	1,							//nReqPbCb
	0,							//nRespPbCb
	2,							//nItems
	T_PBCB_REQ, 26,				//pbQueueName/cbQueueName
	T_DOUBLE,   18				//QEH
};

unsigned short TerminateQueueServerFmt[] = 
{
	RQ_TerminateQueueServer,	//requestCode
	0,							//sCntlInfo
	1,							//nReqPbCb
	0,							//nRespPbCb
	1,							//nItems
	T_PBCB_REQ, 18				//pbQueueName/cbQueueName
};

unsigned short UnmarkQueueEntryFmt[] = 
{
	RQ_UnmarkQueueEntry,	//requestCode
	8,						//sCntlInfo
	1,						//nReqPbCb
	0,						//nRespPbCb
	2,						//nItems
	T_PBCB_REQ, 26,			//pbQueueName/cbQueueName
	T_DOUBLE, 18			//QEH
};

unsigned short CleanQueueFmt[] = 
{
	RQ_CleanQueue,			//requestCode
	4,						//sCntlInfo
	0,						//nReqPbCb
	0,						//nRespPbCb
	1,						//nItems
	T_LONG, 18				//QueueHandle
};

unsigned short RemoveQueueFmt[] = 
{
	RQ_RemoveQueue,			//requestCode
	4,						//sCntlInfo
	0,						//nReqPbCb
	0,						//nRespPbCb
	1,						//nItems
	T_LONG, 18				//QueueHandle
};

unsigned short ReadNextQueueEntryFmt[] =
{
	RQ_ReadNextQueueEntry,	//requestCode
	8,						//sCntlInfo
	1,						//nReqPbCb
	2,						//nRespPbCb
	4,						//nItems
	T_PBCB_REQ, 26,			//pbQueueName/cbQueueName
	T_DOUBLE, 18,			//QEH
	T_PBCB_RESP, 32,		//pbcb EntryRet
	T_PBCB_RESP, 38			//pbcb StatusRet
};

unsigned short GetQMStatusFmt[] =
{
	RQ_GetQMStatus,			//request code
	4,						//sCntlInfo
	0,						//nReqPbCb
	1,						//nRespPbCb
	3,						//nItems
	T_WORD, 18,				//wQueueType
	T_WORD, 20,				//fHealthCheck
	T_PBCB_RESP, 22			//QueueStatusResp			
};


unsigned short RewriteMarkedQueueEntryFmt[] =
{
	RQ_RewriteMarkedQueueEntry,	//request code
	8,							//sCntlInfo
	2,							//nReqPbCb
	0,							//nRespPbCb
	3,							//nItems
	T_PBCB_REQ, 26,				//pbQueueName/cbQueueName
	T_DOUBLE, 18,				//QEH
	T_PBCB_REQ, 32				//pbcb Entry
};

unsigned short ReadKeyedQueueEntryFmt[] =
{
	RQ_ReadKeyedQueueEntry,		//request code
	4,							//sCntlInfo
	3,							//nReqPbCb
	2,							//nRespPbCb
	7,							//nItems
	T_PBCB_REQ, 22,				//pbQueueName/cbQueueName
	T_PBCB_REQ, 28,				//pKey1/cKey1
	T_WORD, 18,					//oKey1
	T_PBCB_REQ, 34,				//pKey2/cKey2
	T_WORD, 20,					//oKey2
	T_PBCB_RESP, 40,			//pbRet/cbRet
	T_PBCB_RESP, 46				//pbStatusRet/cbStatusRet
};

unsigned short MarkKeyedQueueEntryFmt[] =
{
	RQ_MarkKeyedQueueEntry,		//request code
	4,							//sCntlInfo
	3,							//nReqPbCb
	2,							//nRespPbCb
	7,							//nItems
	T_PBCB_REQ, 22,				//pbQueueName/cbQueueName
	T_PBCB_REQ, 28,				//pKey1/cKey1
	T_WORD, 18,					//oKey1
	T_PBCB_REQ, 34,				//pKey2/cKey2
	T_WORD, 20,					//oKey2
	T_PBCB_RESP, 40,			//pbRet/cbRet
	T_PBCB_RESP, 46				//pbStatusRet/cbStatusRet
};

unsigned short RemoveKeyedQueueEntryFmt[] =
{
	RQ_RemoveKeyedQueueEntry,	//request code
	4,							//sCntlInfo
	3,							//nReqPbCb
	0,							//nRespPbCb
	5,							//nItems
	T_PBCB_REQ, 22,				//pbQueueName/cbQueueName
	T_PBCB_REQ, 28,				//pKey1/cKey1
	T_WORD, 18,					//oKey1
	T_PBCB_REQ, 34,				//pKey2/cKey2
	T_WORD, 20					//oKey2
};



