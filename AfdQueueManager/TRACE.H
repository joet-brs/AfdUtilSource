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
'    Module       :  AFD QueueManager Client & Server 
'    Date         :  March 1998
'
'    File Name    :  Trace.h
'
'    Purpose      :  This file has the defintions for the Trace entry points.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

/*///////////////////////////////////////////////////////////////////////////

Module Name: Trace.h

This module contains definitions for debug procedures implemented in
Trace.cpp.

Revision History:

21/July/1997 AA MomentumSystems AFD product.

///////////////////////////////////////////////////////////////////////////*/

#ifndef _TRACE_
#define _TRACE_

/////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
#define IfDebugTraceRq(pRq) void TraceRq(RqType* pRq); 
#define IfDebugInitTrace() void InitTrace()

#else  // no debugging
#define IfDebugTraceRq(pRq) 
#define IfDebugInitTrace()
#endif // debug

#endif // _TRACE_

#ifdef __cplusplus
extern "C" {
#endif
void DEBUG_BREAK(char *ModId, int lineNo, DWORD erc);
void DEBUG_LOG(char *ModId, int lineNo, DWORD erc);
#ifdef __cplusplus
}
#endif


