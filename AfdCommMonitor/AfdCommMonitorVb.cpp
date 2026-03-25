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
'    File Name    :  AfdCommMonitorVb.cpp
'
'    Purpose      :  This provides support to the VB based AfdStatusMonitor.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
9/July/1998 AA	Added code for RealTime Status support. VbGetCommRTStatus().
*/

#include "basic.h"
#include "AfdcommMonitor.h"

#ifdef __cplusplus
extern "C" {
#endif
DWORD GetCommRec(char *ServerId, WORD iQueue, BOOL fStart, AfdCommRec_t *AfdCommRec);

DWORD ModifyCommRec(AfdCommRec_t *AfdCommRec, WORD iQueue, WORD iAction);
DWORD GetCommRTStatus(void *pStatus, BYTE *ServerId);
#ifdef __cplusplus
}
#endif


DWORD WINAPI VbGetCommRec(char *ServerId, WORD iQueue, BOOL fStart,
						  AfdCommRec_t *AfdCommRec) 
{
	return(GetCommRec(ServerId, iQueue, fStart, AfdCommRec));
}

DWORD WINAPI VbModifyCommRec(AfdCommRec_t *AfdCommRec, WORD iQueue, WORD iAction) 
{
	return(ModifyCommRec(AfdCommRec, iQueue, iAction));
}

DWORD WINAPI VbGetCommRTStatus(void *pStatus, BYTE *ServerId) 
{
	return(GetCommRTStatus(pStatus, ServerId));
}