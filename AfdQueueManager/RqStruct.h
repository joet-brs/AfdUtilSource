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
'    File Name    :  RqStruct.h
'
'    Purpose      :  This file has the structure definitions for all the
'                    AFDQueueManager requests.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
9/July/1998 AA	Added RqStructure for RemoveKeyedQueueEntry.
*/

#pragma pack(2)

typedef struct {
	RqHdr_t RqHdr;
	WORD wEntrySize;
	WORD wQueueType;
	BYTE *pbQueueName;
	WORD cbQueueName;
	BYTE *pbFileSpec;
	WORD cbFileSpec;
	DWORD *qeh;			//different from QEH as in QueueEntryHandle
} RqAddQueue_t;

typedef struct {
	RqHdr_t RqHdr;
	WORD fQueueIfNoServer;
	WORD QPriority;
	WORD QType;
	WORD RepeatTime;
	BYTE *pbQueueName;
	WORD cbQueueName;
	BYTE *pbEntry;
	WORD cbEntry;
	SYSTEMTIME *pSysDateTime;
	WORD cbSysDateTime;
} RqAddQueueEntry_t;

typedef struct {
	RqHdr_t RqHdr;
	WORD fReturnIfNoEntries;
	BYTE *pbQueueName;
	WORD cbQueueName;
	BYTE *pbEntryRet;
	WORD cbEntryRet;
	BYTE *StatusBlock;
	WORD sStatusBlock;
} RqMarkNextQueueEntry_t;

typedef struct {
	RqHdr_t RqHdr;
	WORD QueueType;
	WORD fUniqueServer;
	BYTE *pbQueueName;
	WORD cbQueueName;
} RqEstablishQueueServer_t;

typedef struct {
	RqHdr_t RqHdr;
	DWORDLONG QEH;
	BYTE *pbQueueName;
	WORD cbQueueName;
} RqRemoveMarkedQueueEntry_t;


typedef struct {
	RqHdr_t RqHdr;
	DWORDLONG QEH;
	BYTE *pbQueueName;
	WORD cbQueueName;
} RqUnmarkQueueEntry_t;

typedef struct {
	RqHdr_t RqHdr;
	BYTE *pbQueueName;
	WORD cbQueueName;
} RqTerminateQueueServer_t;

typedef struct {
	RqHdr_t RqHdr;
	DWORD qh;
} RqCleanQueue_t;

typedef struct {
	RqHdr_t RqHdr;
	DWORD qh;
} RqRemoveQueue_t;

typedef struct {
	RqHdr_t RqHdr;
	DWORDLONG QEH;				//QueueEntryHandle
	BYTE *pbQueueName;
	WORD cbQueueName;
	BYTE *pbEntryRet;
	WORD cbEntryRet;
	BYTE *StatusBlock;
	WORD sStatusBlock;
} RqReadNextQueueEntry_t;

typedef struct {
	RqHdr_t RqHdr;
	WORD QueueType;
	WORD fHealthCheck;
	BYTE *pStatusRet;
	WORD sStatusRet;
} RqGetQMStatus_t;


typedef struct {
	RqHdr_t RqHdr;
	DWORDLONG QEH;				//QueueEntryHandle
	BYTE *pbQueueName;
	WORD cbQueueName;
	BYTE *pbEntry;
	WORD cbEntry;
} RqRewriteMarkedQueueEntry_t;

typedef struct {
	RqHdr_t RqHdr;
	WORD oKey1;
	WORD oKey2;
	BYTE *pbQueueName;
	WORD cbQueueName;
	BYTE *pKey1;
	WORD cKey1;
	BYTE *pKey2;
	WORD cKey2;
	BYTE *pbEntryRet;
	WORD cbEntryRet;
	BYTE *StatusBlock;
	WORD sStatusBlock;
} RqReadKeyedQueueEntry_t;

typedef struct {
	RqHdr_t RqHdr;
	WORD oKey1;
	WORD oKey2;
	BYTE *pbQueueName;
	WORD cbQueueName;
	BYTE *pKey1;
	WORD cKey1;
	BYTE *pKey2;
	WORD cKey2;
	BYTE *pbEntryRet;
	WORD cbEntryRet;
	BYTE *StatusBlock;
	WORD sStatusBlock;
} RqMarkKeyedQueueEntry_t;

typedef struct {
	RqHdr_t RqHdr;
	WORD oKey1;
	WORD oKey2;
	BYTE *pbQueueName;
	WORD cbQueueName;
	BYTE *pKey1;
	WORD cKey1;
	BYTE *pKey2;
	WORD cKey2;
} RqRemoveKeyedQueueEntry_t;


#pragma pack()