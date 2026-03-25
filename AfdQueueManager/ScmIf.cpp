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
'    Module       :  AFD QueueManager Service
'    Date         :  March 1998
'
'    File Name    :  ScmIf.cpp
'
'    Purpose      :  This file has the code for interfacing with the   
'                    AFDScmInterface.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

/*///////////////////////////////////////////////////////////////////////////
Module Name: ScmIf.cpp

This module implements the protocol to interacet with the NT
Service Control Manager to start the NT Queue Manager service as a Win32 service.

Revision History:

21/July/1997 AA MomentumSystems AFD product.
///////////////////////////////////////////////////////////////////////////*/

//Data Only Module
#include "basic.h"
#include "afderr.h"

static char *ModId[]={__FILE__};

#ifdef __cplusplus
extern "C" {
#endif

BOOL fDebugMode;
char *pServiceName;
DWORD msg_status_send_failed = MSG_STATUS_SEND_FAILED;
DWORD msg_unexpected_handler_case = MSG_UNEXPECTED_HANDLER_CASE;
DWORD msg_register_service_handler_failed = MSG_REGISTER_SERVICE_HANDLER_FAILED;
DWORD msg_create_event_failed = MSG_CREATE_EVENT_FAILED;
DWORD msg_init_service_failed = MSG_INIT_SERVICE_FAILED;
DWORD msg_servicestarted = MSG_SERVICESTARTED;
DWORD msg_exception = MSG_EXCEPTION;

#ifdef __cplusplus
}
#endif




