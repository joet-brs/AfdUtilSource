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
'    Module       :  AFD Registry Configuration.
'    Date         :  March 1998
'
'    File Name    :  AfdRegConfigFile.c
'
'    Purpose      :  This file has the code for parsing of the AfdRegConfig.txt 
'                    which has all the config information for all the AFD 
'                    Windows NT System Services.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

#include "basic.h"
#include <wincon.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "QmProto.h"
#include "AfdMisc.h"

#define ALL_BUFFS 80

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CR 0xd
#define LF 0xa

typedef struct {
	char *filename;
	DWORD recLength;
	BYTE *Buff;
	DWORD sBuff;
	HANDLE fh;
	DWORD sFileSize;
	DWORD currLfa;
	BOOL fEof;
	DWORD iCurrent;
	DWORD erc;
	WORD iLine;
	DWORD lfaPrevLine;
} file_t;

file_t *C_OpenFile(char *filename, DWORD recLength)
{
	file_t *pFile=NULL;
	BOOL fSuccess=TRUE;

	if (!filename) {
		fSuccess = FALSE;
		goto AllDone;
	}
	pFile = (file_t *) zmalloc(sizeof(file_t));
	pFile->filename = (char *) zmalloc(strlen(filename) + 1);
	strcpy(pFile->filename, filename);

	pFile->fh = CreateFile(pFile->filename,
						   GENERIC_READ | GENERIC_WRITE, 0,	// share mode 
						   0, // pointer to security descriptor 
						   OPEN_EXISTING,  // creation mode
						   FILE_ATTRIBUTE_NORMAL,	// file attributes 
						   0);

	if (pFile->fh == INVALID_HANDLE_VALUE) {
		fSuccess = FALSE;
		goto AllDone;
	}

	pFile->sFileSize = GetFileSize(pFile->fh, 0);
	if (pFile->sFileSize == 0xffffffff) {
		fSuccess = FALSE;
		goto AllDone;
	}

	pFile->recLength = pFile->sBuff = recLength;
	pFile->Buff = (BYTE *) zmalloc(pFile->sBuff + 2); //the +2 is for CR/LF
	pFile->iCurrent = 0;

AllDone:
	if (fSuccess)
		return(pFile);

	if (pFile->filename)
		free(pFile->filename);
	if (pFile->Buff)
		free(pFile->Buff);
	if (pFile)
		free(pFile);
	return(NULL);
}


BOOL C_ReadFile(file_t *pFile, BYTE *pBuff, DWORD sRead, DWORD *cRead, BOOL *fEof,
				BOOL fResetLfa)
{
	BOOL fSuccess;
	BOOL fReadToEof;
	DWORD cReadBuff, iCurrent;
	BOOL fLF = FALSE;

	if (fResetLfa) {
		pFile->currLfa = 0;
		pFile->fEof = FALSE;
		pFile->iCurrent = 0;
		pFile->erc = 0;
		pFile->iLine = 0;
		pFile->lfaPrevLine=0;
	}

	*cRead = 0;
	*fEof = FALSE;

	if (sRead < pFile->recLength) {
		return(FALSE);
	}

	if (pFile->fEof) {
		//should not have been called
		return(FALSE);
	}

	pFile->currLfa = SetFilePointer(pFile->fh, pFile->currLfa,
									0, FILE_BEGIN);

	if (pFile->currLfa == 0xffffffff) {
		//why?
		pFile->erc = GetLastError();
		return(FALSE);
	}

	pFile->lfaPrevLine = pFile->currLfa;
	fSuccess = ReadFile(pFile->fh, pFile->Buff, pFile->sBuff,
							&cReadBuff, NULL);
	if (!fSuccess) {
		pFile->erc = GetLastError();
		return(FALSE);
	}

	fReadToEof = (pFile->sBuff != cReadBuff);
	for(iCurrent=0; (iCurrent < cReadBuff); iCurrent++) {
		if (*(pFile->Buff+iCurrent) == CR)
			continue; //next iteratin of for
		*(pBuff+*cRead) = *(pFile->Buff+iCurrent);
		(*cRead)++;
		if (*(pFile->Buff+iCurrent) == LF) {
			fLF = TRUE;
			break; //out of for
		}
	}
	if (!fLF)
		iCurrent--; //because iCurrent has execeeded the cReadBuff by 1;
	if (iCurrent <= cReadBuff) {
		//we did not traverse the entire buffer read, we will put back some of the bytes
		pFile->currLfa += (iCurrent+1);
	} else
		pFile->fEof = *fEof = fReadToEof;
	pFile->iLine++;
	return (TRUE);
}

BOOL C_CloseFile(file_t *pFile) {
	BOOL fSuccess;

	if (!pFile)
		return(FALSE);

	fSuccess = CloseHandle(pFile->fh);
	//This op should not fail, unless there is a corruption of the pFile
	if (!fSuccess) {
		pFile->erc = GetLastError();
		goto AllDone;
	}

AllDone:
	if (pFile->filename)
		free(pFile->filename);
	if (pFile->Buff)
		free(pFile->Buff);
	free(pFile);

	return(fSuccess);
}

BOOL ScanUpto(file_t *pFile, BYTE *token, BOOL fStartOfFile) {

	BOOL fFound=FALSE;
	BYTE buff[ALL_BUFFS];
	DWORD cbRet;
	BOOL fSuccess;
	BOOL fEof=FALSE;

	do {
		fSuccess = C_ReadFile(pFile, buff, sizeof(buff), &cbRet, &fEof, fStartOfFile);
		if (fStartOfFile)
			fStartOfFile=FALSE;
		if (fSuccess) {
			fFound = compareStrings(buff, token, (WORD) strlen((char *) token));
		}
	} while (!fFound && fSuccess);
	if (!fSuccess)
		return(fSuccess);
	return(fFound);
}

DWORD ReadNextline(file_t *pFile, BYTE *buff) {
	DWORD cbRet=0;
	BOOL fEof;
	BOOL fSuccess;
	
	do {
		fSuccess = C_ReadFile(pFile, buff, ALL_BUFFS, &cbRet, &fEof, FALSE) ;
		if ((!fSuccess) || (cbRet == 0) || fEof) {
			cbRet = 0;
			break;
		}
		if (*buff == 0xa)
			continue; //do not send back blank lines
		if (*buff != ';')
			break;
	} while (TRUE);

	if (cbRet != 0)
		cbRet -= 1;  //do not send back the LF
	return(cbRet);
}

void PutBackLine(file_t *pFile) {

	pFile->currLfa = pFile->lfaPrevLine;
}

WORD GetCurrLinenumber(file_t *pFile) {
	if (pFile)
		return(pFile->iLine);
	else
		return(0);
}

