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
'    File Name    :  Req.h
'
'    Purpose      :  This file has the defintions of all the RequestCodes 
'                    used by the AFDQueueManager Service.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
9/July/98 AA	Added RemoveKeyedQueueEntry and ReScheduleMarkedQueueEntry.
*/

#define RQ_AddQueueRequest 0x8061
#define RQ_AddQueueEntryRequest 0x89
#define RQ_EstablishQueueServer 0x92
#define RQ_MarkNextQueueEntry 0x8d
#define RQ_RemoveMarkedQueueEntry 0x8f
#define RQ_TerminateQueueServer 0x93
#define RQ_UnmarkQueueEntry 0x90
#define RQ_CleanQueue 0x8062
#define RQ_RemoveQueue 0x8065
#define RQ_ReadNextQueueEntry 0x8b
#define RQ_GetQMStatus 0x8064
#define RQ_RewriteMarkedQueueEntry 0x91
#define RQ_ReadKeyedQueueEntry 0x8c
#define RQ_MarkKeyedQueueEntry 0x8e
#define RQ_RemoveKeyedQueueEntry 0x8a
#define RQ_ReSchedMarkedQueueEntry 0x815F

