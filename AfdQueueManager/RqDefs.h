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
'    File Name    :  RqDefs.h
'
'    Purpose      :  This file has the defintion stubs for all the  
'                    AFDQueueManager APIs.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
9/July/1998 AA	Added macro definition for RemoveKeyedQueueEntry.
*/

#ifdef __cplusplus
extern "C" {
#endif

extern DWORD RqInterface(WORD *fmt, ...);

extern WORD AddQueueFmt[];
#define M_AddQueue(a, b, c, d, e, f, g) RqInterface(AddQueueFmt, a, b, c, d, e, f, g)

extern WORD AddQueueEntryFmt[];
#define M_AddQueueEntry(a,b,c,d,e,f,g,h,i) RqInterface(AddQueueEntryFmt, a, b, c, d, e, f, g, h, i)

extern WORD EstablishQueueServerFmt[];
#define M_EstablishQueueServer(a, b, c, d) RqInterface(EstablishQueueServerFmt, a, b, c, d)

extern WORD MarkNextQueueEntryFmt[];
#define M_MarkNextQueueEntry(a, b, c, d, e, f, g) RqInterface(MarkNextQueueEntryFmt, a, b, c, d, e, f, g)

extern WORD RemoveMarkedQueueEntryFmt[];
#define M_RemoveMarkedQueueEntry(a, b, c) RqInterface(RemoveMarkedQueueEntryFmt, a, b, c)

extern WORD TerminateQueueServerFmt[];
#define M_TerminateQueueServer(a, b) RqInterface(TerminateQueueServerFmt, a, b)

extern WORD UnmarkQueueEntryFmt[];
#define M_UnmarkQueueEntry(a, b, c) RqInterface(UnmarkQueueEntryFmt, a, b, c)

extern WORD CleanQueueFmt[];
#define M_CleanQueue(a) RqInterface(CleanQueueFmt, a)

extern WORD RemoveQueueFmt[];
#define M_RemoveQueue(a) RqInterface(RemoveQueueFmt, a)

extern WORD ReadNextQueueEntryFmt[];
#define M_ReadNextQueueEntry(a, b, c, d, e, f, g) RqInterface(ReadNextQueueEntryFmt, a, b, c, d, e, f, g)

extern WORD GetQMStatusFmt[];
#define M_GetQMStatus(a, b, c, d) RqInterface(GetQMStatusFmt, a, b, c, d)

extern WORD RewriteMarkedQueueEntryFmt[];
#define M_RewriteMarkedQueueEntry(a, b, c, d, e) RqInterface(RewriteMarkedQueueEntryFmt, a, b, c, d, e)

extern WORD ReadKeyedQueueEntryFmt[];
#define M_ReadKeyedQueueEntry(a, b, c, d, e, f, g, h, i, j, k, l) RqInterface(ReadKeyedQueueEntryFmt, a, b, c, d, e, f, g, h, i, j, k, l)

extern WORD MarkKeyedQueueEntryFmt[];
#define M_MarkKeyedQueueEntry(a, b, c, d, e, f, g, h, i, j, k, l) RqInterface(MarkKeyedQueueEntryFmt, a, b, c, d, e, f, g, h, i, j, k, l)

extern WORD RemoveKeyedQueueEntryFmt[];
#define M_RemoveKeyedQueueEntry(a, b, c, d, e, f, g, h) RqInterface(RemoveKeyedQueueEntryFmt, a, b, c, d, e, f, g, h)

char *GetDllVer(BYTE *pVer);


#ifdef __cplusplus
}
#endif


/*
To Add A New Request Code:

- DLL (Client) Side

1- Add the RequestCode in the file Req.h
2- Add the definition to the RequestStructure in the file RqDef.c
3- Add the #define stub to RqInterface in the file RqDefs.h
4- Add the function exports in the files API.c and PlmApi.c


- Service Side

1- Add the request definition in the file RqStruct.h
2- Add a case statement for this request in the file QMService.cpp
3- Code to handle it.
*/