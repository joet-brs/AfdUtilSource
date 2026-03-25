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
'    File Name    :  AfdCommMonitor.h
'
'    Purpose      :  This provides support to the VB based AfdStatusMonitor.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

#define erc_EndOfList 1

#define ercBaseCM			0x20000000L
#define ercOffsetCM			(1100)

#define erc_BadTPType (ercBaseCM + ercOffsetCM + 1)		//536872013
#define erc_BadQueueType (ercBaseCM + ercOffsetCM + 2)  //536872014
#define erc_InconsistentCall (ercBaseCM + ercOffsetCM + 3) //536872015
#define erc_UndefinedError (ercBaseCM + ercOffsetCM + 4) //536872016
#define erc_BadActionCode (ercBaseCM + ercOffsetCM + 5) //536872017
#define erc_InconsistentActionCode (ercBaseCM + ercOffsetCM + 6) //536872018
#define erc_EntryNotFound (ercBaseCM + ercOffsetCM + 7) //536872019
#define erc_EntryDeletedByMonitor (ercBaseCM + ercOffsetCM + 8) //536872020
#define erc_CommStatusRTInvalidParam (ercBaseCM  + ercOffsetCM + 9) //536872021
#define erc_CommStatusRTNoStatus (ercBaseCM  + ercOffsetCM + 10) //536872022

typedef struct {
	char ServerId[6];
	char Filename[94];
	char Jobname[18];
	char Phonenumber[34];
} AfdCommRec_t;

#define IQ_TX 0
#define IQ_RX 1
#define IQ_HL 2

#define TP_ASYNC 0
#define TP_BISYNC 1
#define TP_SNA 2

#define A_PURGE 0
#define A_REQUEUE 1
#define A_PUTONHOLD 2
