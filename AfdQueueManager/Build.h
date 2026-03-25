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
'    Module       :  AFD QueueManager Client (AfdAmClient.dll)
'    Date         :  March 1998
'
'    File Name    :  Build.h
'
'    Purpose      :  This file has the common structure definitions used  
'                    by the AFDQueueManager Service and client.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

#ifdef _DEBUG
#include <stdio.h>
#endif

#define T_LONG		0
#define T_WORD		1
#define T_BYTE		2
#define	T_WCONST	3
#define T_BCONST	4
#define T_DOUBLE	5
#define T_SYSTIME   6
#define T_POINTER   7
#define T_POINTER_REQ 7
#define T_POINTER_RESP 8

#define T_PBCB_REQ	9 
#define T_PBCB_RESP 10

#define XFER_BLOCK_SIZE 4096

#pragma pack(2)

typedef struct {
	WORD cbMsg;
	WORD sCntlInfo;
	WORD nReq;
	WORD nResp;
	DWORD userNum;
	WORD rqCode;
	DWORD ercRet;
} RqHdr_t;

typedef struct {
	RqHdr_t RqHdr;
	DWORD	processId;
	DWORD	initSignature;
	__int64	macAddress;
} InitMsg_t;

typedef struct {
	DWORD	processId;
	__int64	macAddress;
} TerminateStruc_t;

#define RQ_REMOTE_TERMINATE 0x101a

#define E_SIGNATURE 0xBADCAB
#define DW_ONES 0xffffffff

typedef struct {
	void *p;
	WORD c;
} pbcb_t;


#pragma pack()


typedef struct {
	void *p;
	DWORD processId;
	DWORD threadId;
	HANDLE hPipe;
	DWORD userNumber;	// LSL 000512
	BYTE ServerName[50];
} ThreadInfo_t;

typedef struct PipeInst_s {
	struct PipeInst_s *Next;
	HANDLE hPipeInst;
	HANDLE respEvent;
	struct PipeInst_s *Me;
	DWORD userNum;
	WORD xferBlockSize;
	CHAR *chBuf;
	DWORD cbRead;			//the number of bytes read from the pipe
	DWORD cbToWrite; 
	DWORD dwState;
	DWORD dwMsgs;
	DWORD myThreadId;
	DWORD processIdOfClient;
	__int64 macAddress;
} PipeInst_t, *LPPIPEINST;

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#ifdef __cplusplus
extern "C" {
#endif
extern BOOLEAN fDebugMode; //PUBLIC in qmScmInterface.cpp & API.c
#ifdef __cplusplus
}
#endif

