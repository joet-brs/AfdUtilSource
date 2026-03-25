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
'    Module       :  AFDConfig DLL
'    Date         :  Feb. 1998
'
'    File Name    :  AfdConfig.c
'
'    Purpose      :  This file has routines to parse a Windows Style .ini file
'                    and return values.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

#ifdef Document
	BOOL InitConfigFile(char *filename) {
		Initializes the configuration file.
	}

	BOOL ConfigFile() {
		Resets the Configuration file to start search at zero.
	}

	BOOL GetTokenValue(BYTE *token, BYTE *tokenValue, DWORD sBuff, 
					   DWORD *cTokenValueSize, BOOL fStartOfFile) {

		Gets the value of the token desired. The token has to specified with the colons.
		eg: :TargetProcess:
		The value is returned in the buffer "tokenValue", sBuff is the size of this buffer.
		The string is null terminated, however the size is returned in *cTokenValueSize.
		fStartOfFile instructs the routine to start search from the beginning of file.
		This is desirable because by default the search is done from the current file
		pointer. This is becasue there may be more than entry for the same token name.
		See, :TargetProcess: in AFDConfig.Sys.
	}

	BOOL CloseConfigFile() {
		This closes the ConfigFile and deallocates the resources.
	}

Drawbacks:
		The functions return the token values only as strings, but it is easy enough
		to do conversion from string to decimal, or any other form.

		The main() part is just to test the fucntionality. You may delete it when you
		incorporate this into your code.
#endif


#define		SECURITY_WIN32

#include "basic.h"
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>
#include <Security.h>
#include <AfdMisc.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define CR 0x0d
#define LF 0x0a

void FilterInputFolder(LPSTR token, LPSTR Buffer, LPDWORD cbBuff);
_declspec(dllexport)  BOOL GetInputFolder(BYTE *tokenValue, DWORD sBuff, 
										 DWORD *cTokenValueSize, 
										 DWORD *dwSubFolderLevel, DWORD *dwDelayTime,
										 BOOL fStartOfFile, BOOL fUptoCurrentBlockEnd);
/*
ReadFile(). All reads beyond the file end-of-pointer will return a TRUE. 
One has to check the number of bytes read to determine if end-of-file has been reached. So,
fSuccess = ReadFile(fh, pBuff, sBuff, &cbRet, NULL);
fEof = fSuccess AND (cbRet == 0);

Note that the End-of-file cannot be determined using this method on the current operation. 
Assume that the file length is 780 bytes, and you have issued a read for 1024 bytes. 
In CTOS you would have got back 780 bytes and an error code of 2 (end-of-medium). 
But on NT you get back 780 bytes and error return is no-error. 
On the next read also the error return is no-error, but bytes read is 0.

WriteFile(). You will not get back an end-of-medium error ever (till disk limits). 
The NT file system will automatically extend the file and write the data. 
If your current file length is 780 bytes, and you set file pointer to 2048 and write 
512 bytes there, you will not have any error. The data between 780 and 2048 will be filled 
with NULLS (binary 0).

fEof = (fSuccess && (cbRet == 1024))
*/

#include <malloc.h>
void *zmalloc(DWORD s) {
	void *p = malloc(s);

	if (!p) {
		//SCR 3034 - JT 20090317: Use DebugBreak()
		//_asm int 3;
		DebugBreak();
	}

	memset(p, 0, s);
	return(p);
}

/*char *pbcbToStr(char *p, WORD c)
{
	char *pStr;

	pStr = (char *) zmalloc(c+1);
	memcpy(pStr, p, c);
	*(pStr+c) = 0;
	return(pStr);
}
*/
BOOL compareStrings(BYTE *p1, BYTE *p2, WORD c) {
	/*char *pStr1, *pStr2;
	BOOL fRet;

	pStr1 = pbcbToStr((char *) p1, c);
	pStr2 = pbcbToStr((char *) p2, c);

	fRet = (_stricmp(pStr1, pStr2) == 0);

	free(pStr1);
	free(pStr2);*/

	return(_strnicmp((char *) p1, (char *) p2, c) == 0);

}


#define S_NTFILESPEC 100
typedef struct {
	char fileName[S_NTFILESPEC];
	DWORD recLength;
	BYTE *readBuff;
	DWORD sReadBuff;
	HANDLE fh;
	DWORD sFileSize;
	DWORD currLfa;
	BOOL fEof;
	DWORD erc;
	BYTE currentSection[50];
	DWORD lfaStartToken;
	DWORD lfaEndToken;
} file_t;

file_t *pFile=NULL;

char AfdRootKey[]={"SOFTWARE\\Momentum"};
char AfdRootKey64[]={"SOFTWARE\\Wow6432Node\\Momentum"};

_declspec(dllexport) BOOL GetAfdRoot(char *pAfdRoot, DWORD sAfdRoot, DWORD *cbRet) {
	
	LONG ret;
	HKEY keyHandle;
	static char keyname[]={"AfdRoot"};
	DWORD keytype;


	/* 
	   SCR 3694:
	   Server Selector cannot write to HKLM hive.
	   AfdAdmin will now search HKCU for AfdRoot.
	   If AfdRoot isn't there, then search HKLM.
	*/
	ret = RegOpenKeyEx(HKEY_CURRENT_USER,		//handle of openkey
					   AfdRootKey,				//address of name of sub-key to open
					   0,						//reserved
					   KEY_QUERY_VALUE,			//type of access desired
					   &keyHandle				//returned handle
					  );

	if (ret != ERROR_SUCCESS) {
		// Attempt to open HKCU - 32 bit failed.
		// Now try opening HKCU - 64bit OS.
		ret = RegOpenKeyEx(HKEY_CURRENT_USER,		//handle of openkey
						   AfdRootKey64,			//address of name of sub-key to open
						   0,						//reserved
						   KEY_QUERY_VALUE,			//type of access desired
						   &keyHandle				//returned handle
						  );
	}

	if (ret != ERROR_SUCCESS) {
		// Attempt to open HKCU failed.
		// Now try opening HKLM.
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,		//handle of openkey
						   AfdRootKey,				//address of name of sub-key to open
						   0,						//reserved
						   KEY_QUERY_VALUE,			//type of access desired
						   &keyHandle				//returned handle
						  );
		if (ret != ERROR_SUCCESS)
			return(FALSE);
	}

	*cbRet = sAfdRoot;

	ret = RegQueryValueEx(keyHandle,			//handle of open key
						  keyname,				//address of name of value to query
						  0,					//reserved
						  &keytype,				//returned key type
						  pAfdRoot,				//address of data buffer
						  cbRet					//size of data buff
						 );

	RegCloseKey(keyHandle);

	if (ret != ERROR_SUCCESS) {
		*cbRet = 0;
		return(FALSE);
	}
	return(TRUE);
}

_declspec(dllexport) BOOL GetHalbRoot(char *pHalbRoot, DWORD sHalbRoot, DWORD *cbRet) {
	
	LONG ret;
	HKEY keyHandle;
	static char keyname[]={"HalbRoot"};
	DWORD keytype;


	/* 
	   SCR 3694:
	   Server Selector cannot write to HKLM hive.
	   AfdAdmin will now search HKCU for HalbRoot.
	   If HalbRoot isn't there, then search HKLM.
	*/
	ret = RegOpenKeyEx(HKEY_CURRENT_USER,		//handle of openkey
					   AfdRootKey,				//address of name of sub-key to open
					   0,						//reserved
					   KEY_QUERY_VALUE,			//type of access desired
					   &keyHandle				//returned handle
					  );

	if (ret != ERROR_SUCCESS) {
		// Attempt to open HKCU - 32 bit failed.
		// Now try opening HKCU - 64bit OS.
		ret = RegOpenKeyEx(HKEY_CURRENT_USER,		//handle of openkey
						   AfdRootKey64,			//address of name of sub-key to open
						   0,						//reserved
						   KEY_QUERY_VALUE,			//type of access desired
						   &keyHandle				//returned handle
						  );
	}

	if (ret != ERROR_SUCCESS) {
		// Attempt to open HKCU failed.
		// Now try opening HKLM.
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,		//handle of openkey
						   AfdRootKey,				//address of name of sub-key to open
						   0,						//reserved
						   KEY_QUERY_VALUE,			//type of access desired
						   &keyHandle				//returned handle
						  );
		if (ret != ERROR_SUCCESS)
			return(FALSE);
	}

	*cbRet = sHalbRoot;

	ret = RegQueryValueEx(keyHandle,			//handle of open key
						  keyname,				//address of name of value to query
						  0,					//reserved
						  &keytype,				//returned key type
						  pHalbRoot,				//address of data buffer
						  cbRet					//size of data buff
						 );

	RegCloseKey(keyHandle);

	if ((ret != ERROR_SUCCESS) || (strlen(pHalbRoot) == 0)) {
		return(GetAfdRoot(pHalbRoot, sHalbRoot, cbRet));
	}
	return(TRUE);
}

_declspec(dllexport) HANDLE GetLockFile(char *LockFileName, DWORD NumberOfLocks, DWORD *CallBackErc, DWORD (CallBack)(), void (DiskTimeLog)(BOOL In, char * Operation, char * Object1, char * Object2))
{
	char LockFile[MAX_PATH];
	HANDLE hLockFile = INVALID_HANDLE_VALUE;
	HANDLE hPreviousLockFile = INVALID_HANDLE_VALUE;
	sprintf(LockFile, "%s%d.txt", LockFileName, --NumberOfLocks);

	while (hLockFile == INVALID_HANDLE_VALUE)
	{
		if (NULL != DiskTimeLog) DiskTimeLog(1, NULL, NULL, NULL);
		hLockFile = CreateFile(LockFile, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (NULL != DiskTimeLog) DiskTimeLog(0, "CreateFile", LockFile, "");
		if (hLockFile == INVALID_HANDLE_VALUE)
		{
			Sleep(5);
			if (NULL != CallBack)
			{
					*CallBackErc = CallBack();
					if (0 != *CallBackErc)
					{
						if (hPreviousLockFile != INVALID_HANDLE_VALUE)
						{
							if (NULL != DiskTimeLog) DiskTimeLog(1, NULL, NULL, NULL);
							CloseHandle(hPreviousLockFile);
							if (NULL != DiskTimeLog) DiskTimeLog(0, "CloseHandle", LockFile, "");
							hPreviousLockFile = INVALID_HANDLE_VALUE;
						}
						return INVALID_HANDLE_VALUE;
					}
			}
		}
		else
		{
			if (hPreviousLockFile != INVALID_HANDLE_VALUE)
			{
				if (NULL != DiskTimeLog) DiskTimeLog(1, NULL, NULL, NULL);
				CloseHandle(hPreviousLockFile);
				if (NULL != DiskTimeLog) DiskTimeLog(0, "CloseHandle", LockFile, "");
				hPreviousLockFile = INVALID_HANDLE_VALUE;
			}
			if (0 == NumberOfLocks)
			{
			}
			else
			{
				hPreviousLockFile = hLockFile;
				hLockFile = INVALID_HANDLE_VALUE;
				sprintf(LockFile, "%s%d.txt", LockFileName, --NumberOfLocks);
			}
		}
	}
	return hLockFile;
}

_declspec(dllexport) BOOL GetAfdConfigFile(char *configFile, DWORD sconfigFile, 
										   DWORD *cbRet) 
{
	char AfdRoot[255];	// LSL 990211 SCR#404 Change size from 20 to 255.
	DWORD cAfdRoot;
	BOOL fSuccess;
	static char defAfdConfigFile[]={"\\Config\\AfdConfig.ini"};

	fSuccess = GetHalbRoot(AfdRoot, sizeof(AfdRoot), &cAfdRoot);

	if (!fSuccess)
		return(fSuccess);

	*cbRet = strlen(AfdRoot) + strlen(defAfdConfigFile);

	if (sconfigFile < *cbRet) {
		*cbRet = 0;
		return(FALSE);
	}

	memset(configFile, 0, sconfigFile);
	if(strcpy_s(configFile, sconfigFile, AfdRoot)) return(FALSE);
	if(strcat_s(configFile, sconfigFile, defAfdConfigFile)) return(FALSE);

	return(TRUE);
}


file_t *Config_OpenFile(char *filename, DWORD recLength)
{
	file_t *pFile = (file_t *) zmalloc(sizeof(file_t));
	BOOL fSuccess=TRUE;
	DWORD cbRet;

	if(filename)
	{
		if (strcpy_s(pFile->fileName, sizeof(pFile->fileName), filename)){
			fSuccess = FALSE;
			goto AllDone;
		}
	}
	else {
		fSuccess = GetAfdConfigFile(pFile->fileName, sizeof(pFile->fileName), &cbRet);
		if (!fSuccess)
			goto AllDone;
	}


	pFile->fh = CreateFile(pFile->fileName,
						   GENERIC_READ, FILE_SHARE_READ,	// share mode 
						   0, // pointer to security descriptor 
						   OPEN_EXISTING,	// how to create 
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

	pFile->recLength = pFile->sReadBuff = recLength;
	pFile->readBuff = (BYTE *) zmalloc(pFile->sReadBuff);

AllDone:
	if (fSuccess)
		return(pFile);

	if (pFile->readBuff)
		free(pFile->readBuff);
	if (pFile)
		free(pFile);
	return(NULL);
}


BOOL Config_ReadFile(file_t *pFile, BYTE *pBuff, DWORD sRead, DWORD *cRead, BOOL *fEof)
{
	BOOL fSuccess, fReadToEof, fLF;
	DWORD cReadBuff, iCurrent;

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


	fSuccess = ReadFile(pFile->fh, pFile->readBuff, pFile->sReadBuff,
						&cReadBuff, NULL);
	if (!fSuccess) {
		pFile->erc = GetLastError();
		return(FALSE);
	}
	
	fReadToEof = (pFile->sReadBuff != cReadBuff);

	fLF = FALSE;
	for(iCurrent=0; (iCurrent < cReadBuff); iCurrent++) {
		if (*(pFile->readBuff+iCurrent) == CR)
			continue; //next iteratin of for
		if (*(pFile->readBuff+iCurrent) == LF) {
			fLF = TRUE;
			break; //out of for
		}

		*(pBuff+*cRead) = *(pFile->readBuff+iCurrent);
		(*cRead)++;
	} //for
	if (!fLF)
		iCurrent--; //because iCurrent has execeeded the cReadBuff by 1;
	if (iCurrent <= cReadBuff) {
		//we did not traverse the entire buffer read, we will put back some of the bytes
		pFile->currLfa += (iCurrent+1);
	} else
		pFile->fEof = *fEof = fReadToEof;
	return (TRUE);
}

BOOL Config_CloseFile(file_t *pFile) {

	HANDLE fh;

	if (!pFile)
		return(FALSE);

	if (pFile->readBuff)
		free(pFile->readBuff);
	fh = pFile->fh;
	free(pFile);

	return(CloseHandle(fh));
}

_declspec(dllexport) BOOL InitConfigFile(char *filename) {
#define RECORD_LENGTH 500

	pFile = Config_OpenFile(filename, RECORD_LENGTH);
	if (pFile)
		return(TRUE);
	else
		return(FALSE);
}

BOOL ResetConfigFile() {
	if (!pFile)
		return (FALSE);
	pFile->fEof = FALSE;
	pFile->currLfa = pFile->lfaStartToken = pFile->lfaEndToken = 0;
	memset(pFile->currentSection, 0, sizeof(pFile->currentSection));
	return(TRUE);
}

_declspec(dllexport) BOOL CloseConfigFile() {
	BOOL fRet;

	fRet = Config_CloseFile(pFile);
	if (fRet)
		pFile=NULL;

	return(fRet);
}

_declspec(dllexport)  BOOL GetTokenValue(BYTE *token, BYTE *tokenValue, DWORD sBuff, 
										 DWORD *cTokenValueSize, 
										 BOOL fStartOfFile, BOOL fUptoCurrentBlockEnd)
{
	BYTE localBuff[1000];
	BOOL fSuccess, fFound=FALSE, fEof;
	DWORD cRead;
	char c_comment=';';
	DWORD currLfaSave=pFile->currLfa;
	BOOL fEofSave=pFile->fEof;
	
	if (!pFile)
		return(FALSE);

	if (fStartOfFile)
		ResetConfigFile();

	do 
		{
		if (fUptoCurrentBlockEnd) 
			{

			if (pFile->currLfa >= pFile->lfaEndToken)
				break;
			}
		memset(localBuff, 0, sizeof(localBuff));
		fSuccess = Config_ReadFile(pFile, localBuff, 1000, &cRead, &fEof);
		if (fSuccess) 
			{
			if (cRead == 0) //we only read cr/lf on the line
				continue;
			if (localBuff[0] == c_comment)
				continue;
			fFound = compareStrings(localBuff, token, (WORD) strlen((char *) token));
			if (fFound) 
				{
				// LSL 000406 Filter out InputDirectory switches
				FilterInputFolder(token, localBuff, &cRead);

				*cTokenValueSize = cRead - strlen((char *) token);
				//max size (sBuff) not validated
				if (tokenValue) 
					{
					if(memcpy_s(tokenValue, sBuff-1,  localBuff+strlen((char *)token), *cTokenValueSize)) return(FALSE);
					*(tokenValue+(*cTokenValueSize)) = 0; //null terminate
					}
				break;
				}
			}
		else
			break;
		} while (!fEof);

	if (!fFound) {
		pFile->currLfa = currLfaSave; //if token not found, restore current file pointer
		pFile->fEof = fEofSave;
	}

	return(fFound);
}

/*****************************************************************
FilterInputFolder()

Description:
We are now allowing special identifiers to be placed on the end of 
an InputFolder name.  The identifier(s) will always take the form
of text surrounded by angle brackets.  
	Ex:  InputFolder=D:\Afd\AfdIn1<+><D5>

These identifiers should NOT be passed to applications requesting
InputFolder names.
******************************************************************/
void FilterInputFolder(LPSTR token, LPSTR Buffer, LPDWORD cbBuff) {
	LPSTR lpszSpecial;
	
	// Check token to make sure it is "InputFolder"
	if(stricmp(token, "InputFolder=")) return;
	
	// Check if an extension exists
	lpszSpecial = strchr(Buffer,'<');
	if(!lpszSpecial) return;
	
	*cbBuff	= lpszSpecial - Buffer;
	*lpszSpecial = '\0';
	}


BOOL AssignBoundsForSection(BYTE *section) {
	DWORD cTokenValueSize;
	BOOL fSuccess;
	char endofsection[]={"["};

	fSuccess = GetTokenValue(section, NULL, 0, &cTokenValueSize, TRUE /*fStartOfFile*/,
					    	 FALSE /*fUptoCurrentBlockEnd*/);
	if (!fSuccess)
		return(fSuccess);

	pFile->lfaStartToken = pFile->currLfa;

	fSuccess = GetTokenValue(endofsection, NULL, 0, &cTokenValueSize, FALSE /*fStartOfFile*/,
					    	 FALSE /*fUptoCurrentBlockEnd*/);

	pFile->lfaEndToken = pFile->currLfa;
	//now set the currLfa to the start of the section, and reset fEof
	pFile->currLfa = pFile->lfaStartToken;
	pFile->fEof = FALSE;
	strcpy(pFile->currentSection, section);

	return(TRUE);
}


_declspec(dllexport) BOOL GetAfdConfigParam(BYTE *section, BYTE *token, BYTE *tokenValue, 
											DWORD sBuff, DWORD *cTokenValueSize, 
											BOOL fStartOfFile) {
	char sectionStart[]={"["};
	char sectionEnd[]={"]"};
	char tokenDelimiter[]={"="};
	BOOL fSuccess = FALSE;
	BYTE *sectionName=NULL, *tokenName=NULL;

	if (!pFile) {
		return(FALSE);
	}
	if (fStartOfFile)
		ResetConfigFile();
	
	
	sectionName = (BYTE *) zmalloc(strlen(section) + 3);
	strcpy(sectionName, sectionStart);
	if (strcat_s(sectionName, strlen(section) + 3, section)) return(FALSE);
	if (strcat_s(sectionName, strlen(section) + 3, sectionEnd)) return(FALSE);

	tokenName = (BYTE *) zmalloc(strlen(token) + 2);
	strcpy(tokenName, token);
	if (strcat_s(tokenName, strlen(token) + 2, tokenDelimiter)) return(FALSE);
	
	
	
	//check if the token we are currently scanning is the same as the one asked for
	if (!compareStrings(pFile->currentSection, sectionName, (WORD) strlen(sectionName))) 
		{
		// LSL 981125 SCR#276 begin : Accept ProfileIdLocations as alias for CustomerIdLocations
		if (compareStrings(pFile->currentSection,		// currentSection begins/ends with square brackets []
					"[ProfileIdLocations]", (WORD) 20 ) 
			&& !strcmp(section,"CustomerIdLocations"))
			goto AllDone;
		// LSL 981125 SCR#276 end	
		
		ResetConfigFile();
		//get the start and end lfa for this section
		fSuccess = AssignBoundsForSection(sectionName);
		if (!fSuccess)
			goto AllDone;
		}

		fSuccess = GetTokenValue(tokenName, tokenValue, sBuff, cTokenValueSize, 
							 FALSE /*fStartOfFile*/, TRUE /*fUptoCurrentBlockEnd*/);
AllDone:
	if (sectionName)
		free(sectionName);
	if (tokenName)
		free(tokenName);
	
	// LSL 981125 SCR#276 begin Accept ProfileIdLoc as alias for CustomerIdLoc
	if (!fSuccess 
			&& !strcmp(section,"CustomerIdLocations") 
			&& !strcmp(token,"CustomerIdLoc") )
		{
		fSuccess = GetAfdConfigParam("ProfileIdLocations","ProfileIdLoc"
							,tokenValue,sBuff, cTokenValueSize, 
											fStartOfFile);
		}
	// LSL 981125 SCR#276 end
	return(fSuccess);
}

_declspec(dllexport) BOOL GetAfdInputFolder(BYTE *tokenValue, DWORD sBuff, 
											DWORD *cTokenValueSize, 
											DWORD *dwSubFolderLevel, DWORD *dwDelayTime,
											BOOL fStartOfFile) {
	char sectionStart[]={"["};
	char sectionEnd[]={"]"};
	char tokenDelimiter[]={"="};
	BOOL fSuccess = FALSE;
	BYTE *sectionName=NULL, *tokenName=NULL;
	BYTE section[] = {"Folders"};
	BYTE token[] = {"InputFolder"};

	if (!pFile) {
		return(FALSE);
	}
	if (fStartOfFile)
		ResetConfigFile();
	
	
	sectionName = (BYTE *) zmalloc(strlen(section) + 3);
	strcpy(sectionName, sectionStart);
	if (strcat_s(sectionName, strlen(section) + 3, section)) return(FALSE);
	if (strcat_s(sectionName, strlen(section) + 3, sectionEnd)) return(FALSE);

	tokenName = (BYTE *) zmalloc(strlen(token) + 2);
	strcpy(tokenName, token);
	strcat(tokenName, tokenDelimiter);
	
	
	
	//check if the token we are currently scanning is the same as the one asked for
	if (!compareStrings(pFile->currentSection, sectionName, (WORD) strlen(sectionName))) 
		{
		// LSL 981125 SCR#276 begin : Accept ProfileIdLocations as alias for CustomerIdLocations
		if (compareStrings(pFile->currentSection,		// currentSection begins/ends with square brackets []
					"[ProfileIdLocations]", (WORD) 20 ) 
			&& !strcmp(section,"CustomerIdLocations"))
			goto AllDone;
		// LSL 981125 SCR#276 end	
		
		ResetConfigFile();
		//get the start and end lfa for this section
		fSuccess = AssignBoundsForSection(sectionName);
		if (!fSuccess)
			goto AllDone;
		}

	fSuccess = GetInputFolder(tokenValue, sBuff, cTokenValueSize, 
							 dwSubFolderLevel, dwDelayTime,
							 FALSE /*fStartOfFile*/, TRUE /*fUptoCurrentBlockEnd*/);
AllDone:
	if (sectionName)
		free(sectionName);
	if (tokenName)
		free(tokenName);
	
	// LSL 981125 SCR#276 begin Accept ProfileIdLoc as alias for CustomerIdLoc
	if (!fSuccess 
			&& !strcmp(section,"CustomerIdLocations") 
			&& !strcmp(token,"CustomerIdLoc") )
		{
		fSuccess = GetAfdConfigParam("ProfileIdLocations","ProfileIdLoc"
							,tokenValue,sBuff, cTokenValueSize, 
											fStartOfFile);
		}
	// LSL 981125 SCR#276 end
	return(fSuccess);
}


_declspec(dllexport)  BOOL GetInputFolder(BYTE *tokenValue, DWORD sBuff, 
										 DWORD *cTokenValueSize, 
										 DWORD *dwSubFolderLevel, DWORD *dwDelayTime,
										 BOOL fStartOfFile, BOOL fUptoCurrentBlockEnd)
{
	BYTE localBuff[1000];
	BOOL fSuccess, fFound=FALSE, fEof;
	DWORD cRead;
	char c_comment=';';
	DWORD currLfaSave=pFile->currLfa;
	BOOL fEofSave=pFile->fEof;
	char token[] = {"InputFolder="};	
	if (!pFile)
		return(FALSE);

	if (fStartOfFile)
		ResetConfigFile();

	*dwSubFolderLevel = 0;
	*dwDelayTime = 0;
	do 
		{
		if (fUptoCurrentBlockEnd) 
			{

			if (pFile->currLfa >= pFile->lfaEndToken)
				break;
			}
		memset(localBuff, 0, sizeof(localBuff));
		fSuccess = Config_ReadFile(pFile, localBuff, 1000, &cRead, &fEof);
		if (fSuccess) 
			{
			if (cRead == 0) //we only read cr/lf on the line
				continue;
			if (localBuff[0] == c_comment)
				continue;
			fFound = compareStrings(localBuff, token, (WORD) strlen((char *) token));
			if (fFound) 
				{
				LPSTR lpszSpecial;
				LPSTR lpszEndChar;
				char szTemp[80] = {""};
				int state = 0;
				// Check if an extension exists
				lpszSpecial = strchr(localBuff,'<');
				if(lpszSpecial) {
					cRead = lpszSpecial - localBuff;
					*lpszSpecial = '\0';
					}

				for(;lpszSpecial != NULL;lpszSpecial = strchr(lpszSpecial,'<')) {
					lpszSpecial++;
					switch(*lpszSpecial) {
						case '+':
							*dwSubFolderLevel = 0xffffffff;
							lpszSpecial++;
							if(isdigit(*lpszSpecial)) {
								*dwSubFolderLevel = strtoul(lpszSpecial,&lpszEndChar,10);
								lpszSpecial = lpszEndChar;
								}
							break;
						case 'D':
						case 'd':
							lpszSpecial++;
							while(*lpszSpecial && !isdigit(*lpszSpecial)) lpszSpecial++;
							if(*lpszSpecial) *dwDelayTime = strtoul(lpszSpecial,NULL,10);
						}
						if(*lpszSpecial == 0) break;
					}

				*cTokenValueSize = cRead - strlen((char *) token);
				//max size (sBuff) not validated
				if (tokenValue) 
					{
					if(memcpy_s(tokenValue, sBuff-1,  localBuff+strlen((char *)token), *cTokenValueSize)) return(FALSE);
					*(tokenValue+(*cTokenValueSize)) = 0; //null terminate
					}
				break;
				}
			}
		else
			break;
		} while (!fEof);

	if (!fFound) {
		pFile->currLfa = currLfaSave; //if token not found, restore current file pointer
		pFile->fEof = fEofSave;
	}

	return(fFound);
}

char  SqlLoginKeyName[30] = {"LoginId"};
char  SqlPasswdKeyName[30] = {"PassWd"};

LONG AfdGetSqlLoginPasswd(BYTE *LoginIdBuff, DWORD sLoginIdBuff,
						  BYTE *PasswdBuff, DWORD sPasswdBuff, 
						  DWORD *cbLoginRet, DWORD *cbPasswdRet) 
{
	char keyPath[120];
	LONG ret;
	HKEY keyHandle;
	DWORD  keytype = REG_SZ;

	memset(keyPath, 0, 120);
	strcpy(keyPath, "SOFTWARE\\MOMENTUM\\Intelligent Network Gateway\\SQL\\");
	
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,		//handle of openkey
					   keyPath,					//address of name of sub-key to open
					   0,						//reserved
					   KEY_QUERY_VALUE,			//type of access desired
					   &keyHandle				//returned handle
					  );
	if (ret != ERROR_SUCCESS)
		return(ret);

	*cbLoginRet = sLoginIdBuff;
	*cbPasswdRet = sPasswdBuff;

	ret = RegQueryValueEx(keyHandle,			//handle of open key
						  SqlLoginKeyName,				//address of name of value to query
						  0,					//reserved
						  &keytype,				//returned key type
						  LoginIdBuff,				//address of data buffer
						  cbLoginRet					//size of data buff
						 );

	//In case of a REG_SZ the NULL BYTE is also added in the StringLength,
	//we need to strip this

	if (keytype == REG_SZ)
		if (*cbLoginRet > 0)
			(*cbLoginRet)--;


    RegQueryValueEx(keyHandle,			//handle of open key
						  SqlPasswdKeyName,				//address of name of value to query
						  0,					//reserved
						  &keytype,				//returned key type
						  PasswdBuff,				//address of data buffer
						  cbPasswdRet					//size of data buff
						 );

	//In case of a REG_SZ the NULL BYTE is also added in the StringLength,
	//we need to strip this

	if (keytype == REG_SZ)
		if (*cbPasswdRet > 0)
			(*cbPasswdRet)--;

	RegCloseKey(keyHandle);

	return(ret);
}

#define  LOGINSTRLEN   256
typedef struct {
	BOOL gotConfig;
	BOOL gotDbConnection;
	char  DsnName[30];
	BYTE  LoginName[LOGINSTRLEN];
	BYTE  Passwd[LOGINSTRLEN];
	char ProcessedBy[MAX_COMPUTERNAME_LENGTH + 1];
	char ServiceName[50];
	char userName[128];
	DWORD ProcessID;
	HANDLE hDBaccess;
	SQLHENV* phEnv;
	SQLHENV hEnv;
	SQLHDBC* phDBc;
	SQLHDBC hDBc;
	SQLHSTMT* phStmt;
	SQLHSTMT hStmt;
	BOOL LogTransactionsToDB;
} transactionControl_t;

typedef struct {
	long transactionId;
	long transactionGroup;
	CHAR errorMsg[500];
	DWORD errorRet;
} transactionInOut_t;


BOOL getDbSemaphore(transactionControl_t *transactionControlT)
{
	if (transactionControlT->hDBaccess != NULL) 
	{
		if (WaitForSingleObject(transactionControlT->hDBaccess, INFINITE) != WAIT_OBJECT_0) 
		{ //
			return FALSE;
		}
	}
	return TRUE;
}

BOOL releaseDbSemaphore(transactionControl_t *transactionControlT)
{
	if (transactionControlT->hDBaccess != NULL) 
	{
		if (!ReleaseSemaphore(transactionControlT->hDBaccess,       // handle to the semaphore object
			  1,      // amount to add to current count
			  NULL   // address of previous count
			))
		{
			return FALSE;
		}
	}
	return TRUE;
}

void ErrorTIO(transactionInOut_t *transactionInOutT, CHAR *errorMsg, DWORD errorRet)
{
	if ((transactionInOutT->errorRet == 0) && (strlen(transactionInOutT->errorMsg) == 0))
	{
		strcpy(transactionInOutT->errorMsg, errorMsg);
		transactionInOutT->errorRet = errorRet;
	}
}

BOOL DatabaseDisconnect(transactionControl_t *transactionControlT, transactionInOut_t *transactionInOutT) {
	SQLRETURN retcode = SQL_SUCCESS;

    if (*transactionControlT->phStmt != SQL_NULL_HSTMT)
	{
        retcode = SQLFreeStmt(*transactionControlT->phStmt, SQL_DROP);
		*transactionControlT->phStmt = SQL_NULL_HSTMT;			
	}
	if (*transactionControlT->phDBc != SQL_NULL_HDBC) {
		retcode = SQLDisconnect(*transactionControlT->phDBc);
		if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		{
			ErrorTIO(transactionInOutT, "SQLDisconnect error", retcode);
			return FALSE;
		}
		retcode = SQLFreeConnect(*transactionControlT->phDBc);
		*transactionControlT->phDBc = SQL_NULL_HDBC;
		if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
		{
			ErrorTIO(transactionInOutT, "SQLFreeConnect error", retcode);
			return FALSE;
		}
	}
	*transactionControlT->phDBc = SQL_NULL_HDBC;
	if (*transactionControlT->phEnv != SQL_NULL_HENV)
		retcode = SQLFreeEnv(*transactionControlT->phEnv);
	*transactionControlT->phEnv = SQL_NULL_HENV;
	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO))
	{
		ErrorTIO(transactionInOutT, "SQLFreeEnv error", retcode);
		return FALSE;
	}
	return TRUE;
}

void SQLErrorHandler(SQLRETURN SQLErc, transactionControl_t *transactionControlT, HSTMT h3, transactionInOut_t *transactionInOutT) {

	// SQLRETURN NativeError;  // LSL 990202 OLD LINE
	SQLINTEGER NativeError;		// LSL 990202 Changed to SQLINTEGER
	SQLCHAR szSqlState[256], szErrorMsg[256];
	SQLSMALLINT cbErrorMsg;
	SQLSMALLINT errval;
	//BOOL fDone=TRUE;
	char buff[1024];

    /* if SQL error, get additional SQL error info.  per ODBC documentation,
        an SQL call can generate multiple  errors, SQLError() is called
        repeatedly until it doesn't return SQL_SUCCESS. */


//	sprintf(buff, "SQL Error %d. Details of the error follow. \r\n", SQLErc);
//	WriteLogWithTimeStamp((char *) rgSchedLog, buff, strlen(buff));

    if (*transactionControlT->phEnv != NULL) {
        while (TRUE) { //while there are errors to retrieve
			
			// SQLError gets mapped to SQLGetDiagRec()
			// returns SQL_SUCCESS, SQL_SUCCESS_WITH_INFO, SQL_ERROR, SQL_INVALID_HANDLE
			//SQLGetDiagRec
			errval = SQLError(*transactionControlT->phEnv, *transactionControlT->phDBc, h3, szSqlState, &NativeError, 
						   szErrorMsg, sizeof(szErrorMsg), (short *) &cbErrorMsg);
			if (errval == SQL_SUCCESS) 
				{
				sprintf(buff, "%s : State = %s. Msg = %s. N.E. = %ld\r\n",
					((SQLErc == SQL_SUCCESS_WITH_INFO)? "Success with info." : "SQL Error"),
					szSqlState, szErrorMsg, NativeError);
				ErrorTIO(transactionInOutT, buff, SQLErc);
				}
			else
				break;
			} //while
	}
	if (!transactionControlT->gotDbConnection)
	{	//We're just borrowing the service's DB connection, let it fix things
		DatabaseDisconnect(transactionControlT, transactionInOutT);
	}
	return;
}

BOOL DatabaseConnect(transactionControl_t *transactionControlT, transactionInOut_t *transactionInOutT) {

	SQLRETURN retcode;
	char   erc_str[500];
	*transactionControlT->phEnv = SQL_NULL_HENV;
	*transactionControlT->phDBc = SQL_NULL_HDBC;
	*transactionControlT->phStmt = SQL_NULL_HSTMT;
	
	retcode = SQLAllocEnv(transactionControlT->phEnv);			// Environment handle
	//retcode = SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&henv);
	if (retcode != SQL_SUCCESS)
		goto ErrorDone;
	retcode = SQLAllocConnect(*transactionControlT->phEnv, transactionControlT->phDBc);	// Connection handle
	if (retcode != SQL_SUCCESS)
		goto ErrorDone;
	//SQLSetConnectOption(transactionControlT->phDBc, SQL_LOGIN_TIMEOUT, 5);
	//Connect to data source
	retcode = SQLConnect(*transactionControlT->phDBc, &transactionControlT->DsnName, 
					strlen((char *) transactionControlT->DsnName), 
					transactionControlT->LoginName,
					strlen((char *) transactionControlT->LoginName),
					(BYTE *) transactionControlT->Passwd, 
					strlen((char *) transactionControlT->Passwd));
	if (retcode == SQL_SUCCESS)
		return TRUE;
	else if(retcode == SQL_SUCCESS_WITH_INFO)
		{
		return TRUE;
		}
	else
	{
		// Log the Messages
//		strcpy(erc_str, "The AFD service or application could not successfully login to the SQL database with the login ID/password configured in the registry.\n");
//		WriteLogWithTimeStamp((char *)rgSchedLog,erc_str, strlen(erc_str));
		goto ErrorDone;
	}



	//come here only in the case of an error, normal path exits before this
ErrorDone:
	SQLErrorHandler(retcode, transactionControlT, SQL_NULL_HSTMT, transactionInOutT);
	if (*transactionControlT->phDBc != SQL_NULL_HDBC) {
		SQLDisconnect(*transactionControlT->phDBc);
		SQLFreeConnect(*transactionControlT->phDBc);
	}
	*transactionControlT->phDBc = SQL_NULL_HDBC;
	if (*transactionControlT->phEnv != SQL_NULL_HENV)
		SQLFreeEnv(*transactionControlT->phEnv);
	*transactionControlT->phEnv = SQL_NULL_HENV;
	if (retcode == SQL_SUCCESS_WITH_INFO)
		retcode = SQL_SUCCESS;
	return FALSE;
}

struct T_BINDCOL_PARAMS
    {
    UWORD   CNum;
    SWORD   CType;
    PTR     pValue;
    SDWORD  cbValue;
    SDWORD  *pcbValueRet;
    };
typedef struct T_BINDCOL_PARAMS BINDCOL_PARAMS;

SQLRETURN SetupTableAccess(SQLHSTMT *hstmt, BYTE *pQuery, BINDCOL_PARAMS *BCParams,
						   WORD nCols, transactionControl_t *transactionControlT, transactionInOut_t *transactionInOutT) {

	SQLRETURN retcode;
	int i;

	if (*hstmt != SQL_NULL_HSTMT) {
		SQLFreeStmt(*hstmt, SQL_DROP);
		*hstmt = SQL_NULL_HSTMT;			
	}
	retcode = SQLAllocStmt(*transactionControlT->phDBc, hstmt); // Statement handle 
	if (retcode != SQL_SUCCESS) {
		SQLErrorHandler(retcode, transactionControlT, *hstmt, transactionInOutT);
		return(retcode);
	}
	retcode = SQLExecDirect(*hstmt, pQuery, SQL_NTS);
	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
		SQLErrorHandler(retcode, transactionControlT, *hstmt, transactionInOutT);
		return(retcode);
	}
	for(i=0; i<nCols; i++)
	{
				retcode = SQLBindCol(*hstmt, 
									 //parameter number, this has to correspond
									 //to the parameters in the SELECT command,
									 //we have selected all '*'
									 (BCParams+i)->CNum,
									 (BCParams+i)->CType,  //type
									 (BCParams+i)->pValue, //it is returned here
									 (BCParams+i)->cbValue,//max size of ret
									 (long *) (BCParams+i)->pcbValueRet //pcbRet
									 );
				if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
					SQLErrorHandler(retcode, transactionControlT, *hstmt, transactionInOutT);
					return(retcode);
				}
	} //all BindCol
	return(SQL_SUCCESS);
}

void InitTIO(transactionInOut_t *transactionInOutT)
{
	transactionInOutT->errorMsg[0] = 0;
	transactionInOutT->errorRet = 0;
}

_declspec(dllexport)  BOOL CloseTransactionStruct(transactionControl_t *transactionControlT, transactionInOut_t *transactionInOutT)
{
	BOOL returnX;
	InitTIO(transactionInOutT);
	if (!transactionControlT->gotConfig)
	{
		ErrorTIO(transactionInOutT, "transactionControlT has not been initialized", 0);
		return FALSE;
	}

	if (transactionControlT->gotDbConnection)
	{
		return TRUE;
	}

	getDbSemaphore(transactionControlT);
	returnX = DatabaseDisconnect(transactionControlT, transactionInOutT);
	releaseDbSemaphore(transactionControlT);
	transactionControlT->gotConfig = FALSE;
	if (transactionControlT->hDBaccess != NULL) 
	{
		CloseHandle(transactionControlT->hDBaccess);
		transactionControlT->hDBaccess = NULL;
	}

	return returnX;
}

_declspec(dllexport)  BOOL InitTransactionStruct(transactionControl_t *transactionControlT, transactionInOut_t *transactionInOutT, char* Service, HANDLE hDBaccess, SQLHENV* phenv, SQLHDBC* phdbc, SQLHSTMT* phstmt)
{
	int i = 0;
	DWORD cbRet;
	DWORD  cbLoginRet, cbPasswdRet;
	DWORD userNameSize;
	CHAR QueryBuffer[300];
	SQLRETURN retcode;
	UDWORD LogTransactionsToDB = 0;
	UDWORD cbLogTransactionsToDB = 0;
	BINDCOL_PARAMS LogTransactionsToDBBCParams[1] =
	    {1,  SQL_C_ULONG, &LogTransactionsToDB, sizeof(UDWORD), &cbLogTransactionsToDB
	    };

	InitTIO(transactionInOutT);
	memset(transactionControlT, 0, sizeof(*transactionControlT));
	transactionControlT->gotConfig = TRUE;
/*	if (hDBaccess != NULL) transactionControlT->hDBaccess = hDBaccess;
	if (phenv == NULL) transactionControlT->phEnv = &transactionControlT->hEnv; else transactionControlT->phEnv = phenv;
	if (phdbc == NULL) transactionControlT->phDBc = &transactionControlT->hDBc; else transactionControlT->phDBc = phdbc;
	if (phstmt == NULL) transactionControlT->phStmt = &transactionControlT->hStmt; else transactionControlT->phStmt = phstmt;
	transactionControlT->gotDbConnection = (hDBaccess || *transactionControlT->phEnv || *transactionControlT->phDBc || *transactionControlT->phStmt);*/
	transactionControlT->phEnv = &transactionControlT->hEnv;
	transactionControlT->phDBc = &transactionControlT->hDBc;
	transactionControlT->phStmt = &transactionControlT->hStmt;

	if (!transactionControlT->gotDbConnection)
	{
		if (!InitConfigFile(NULL))
		{
			//Probably in a DMZ with no AFD
			return TRUE;
		}

		// Read the DSN Name from the AfdConfig.ini file
	    if (!GetAfdConfigParam((BYTE *)"AFDMain",(BYTE *) "AFDDSN",(BYTE *) transactionControlT->DsnName, 30, &cbRet, TRUE))
	    {
			ErrorTIO(transactionInOutT, "GetAfdConfigParam error", GetLastError());
	        CloseConfigFile();
			return FALSE;
		}
        CloseConfigFile();

		//Probably in a DMZ with no AFD
		if (strlen(transactionControlT->DsnName) == 0)
		{
			//Probably in a DMZ with no AFD
			return TRUE;
		}

	   /* Get the Sql Login Name and Password from the Registry  */
		AfdGetSqlLoginPasswd((BYTE *)transactionControlT->LoginName,LOGINSTRLEN, (BYTE *)transactionControlT->Passwd, LOGINSTRLEN, &cbLoginRet, &cbPasswdRet);

		if (transactionControlT->hDBaccess == NULL)
		{
			transactionControlT->hDBaccess = CreateSemaphore(
				  NULL, // pointer to security attributes
				  1,  // initial count
				  1,  // maximum count
				  NULL       // pointer to semaphore-object name
				);
		}

		for (i = 0; i <= 5; i++)
		{
			if (!DatabaseConnect(transactionControlT, transactionInOutT))
			{
				if (i == 5)
				{
					return FALSE;
				}
				else
				{
					Sleep(5000);
					InitTIO(transactionInOutT);
				}
			}
			else
			{
				break;
			}
		}
	}

	getDbSemaphore(transactionControlT);

	memset(QueryBuffer, 0, sizeof(QueryBuffer));
	sprintf((char *) QueryBuffer, "SELECT LogTransactionsToDB FROM LogTransactionsToDB");

	retcode = SetupTableAccess(transactionControlT->phStmt, QueryBuffer,
							   (BINDCOL_PARAMS *) &LogTransactionsToDBBCParams,
								ARRAY_LEN(LogTransactionsToDBBCParams), transactionControlT, transactionInOutT);
	if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
		SQLErrorHandler(retcode, transactionControlT, *transactionControlT->phStmt, transactionInOutT);
		releaseDbSemaphore(transactionControlT);
		return FALSE;
	}

	memset(&LogTransactionsToDB, 0, sizeof(LogTransactionsToDB));
	retcode = SQLFetch(*transactionControlT->phStmt);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		retcode = SQL_SUCCESS;
	}
	else
	{
		if(retcode != SQL_NO_DATA)
		{
			SQLErrorHandler(retcode, transactionControlT, *transactionControlT->phStmt, transactionInOutT);
		}
	}

	SQLFreeStmt(*transactionControlT->phStmt, SQL_DROP);
	*transactionControlT->phStmt = SQL_NULL_HSTMT;			
	transactionControlT->LogTransactionsToDB = LogTransactionsToDB;
	releaseDbSemaphore(transactionControlT);

	if (LogTransactionsToDB)
	{
		strcpy(transactionControlT->ServiceName, Service);
		transactionControlT->ProcessID = GetCurrentProcessId();
		cbRet = sizeof(transactionControlT->ProcessedBy);
		if(!GetComputerName(transactionControlT->ProcessedBy,&cbRet))
		{
			//sprintf(transactionControlT->ProcessedBy, "Unknown %d", GetLastError());
			strcpy(transactionControlT->ProcessedBy, "Unknown");
		}
		userNameSize = sizeof(transactionControlT->userName);
		if (!GetUserNameEx(NameSamCompatible, transactionControlT->userName, &userNameSize))
		{
			strcpy(transactionControlT->userName, "Unknown");
		}
	}
	else
	{
		cbRet = CloseTransactionStruct(transactionControlT, transactionInOutT);
		transactionControlT->gotConfig = TRUE;
		return cbRet;
	}
	return TRUE;
}

char* FixSingleQuote(char* inputString, char* outputString)
{
	int inputLen = strlen(inputString);
	int i,j;

	// Fix any single quote issues in the inputString
	j = 0;
	for (i = 0; i < inputLen; i++)
	{
		outputString[(i)+j] = inputString[i];
		if ('\'' == inputString[i])
		{
			j++;
			outputString[(i)+j] = '\'';
		}
	}
	outputString[(i)+j] = 0;
	return outputString;
}

_declspec(dllexport)  BOOL InsertTransactionRecord(transactionControl_t *transactionControlT, transactionInOut_t *transactionInOutT, char* RemoteMachine, char* Identifier, char* Source, char* Destination, long long Size, BOOL Secure, char* Protocols)
{
	SQLRETURN retcode;
	CHAR QueryBuffer[2000];
	CHAR lService[100];
	CHAR lProcessedBy[100];
	CHAR lRemoteMachine[100];
	CHAR lIdentifier[200];
	CHAR lSource[600];
	CHAR lDestination[600];
	CHAR lProtocols[1000];
	UDWORD NewTransactionsID = 0;
	UDWORD cbNewTransactionsI = 0;
	BINDCOL_PARAMS NewTransactionsIBCParams[1] =
	    {1,  SQL_C_ULONG, &NewTransactionsID, sizeof(UDWORD), &cbNewTransactionsI
	    };

	InitTIO(transactionInOutT);
	transactionInOutT->transactionId = 0;

	if (!transactionControlT->gotConfig)
	{
		ErrorTIO(transactionInOutT, "transactionControlT has not been initialized", 0);
		return FALSE;
	}

	if (transactionControlT->LogTransactionsToDB)
	{
		// Create records and get ID
		if (!getDbSemaphore(transactionControlT))
		{
			ErrorTIO(transactionInOutT, "Unable to get Transactions DbSemaphore", GetLastError());
			return FALSE;
		}

		if (!transactionControlT->gotDbConnection && (SQL_NULL_HDBC == *transactionControlT->phDBc))
		{
			if (!DatabaseConnect(transactionControlT, transactionInOutT)) return FALSE;
		}

		if (*transactionControlT->phStmt != SQL_NULL_HSTMT) {
			SQLFreeStmt(*transactionControlT->phStmt, SQL_DROP);
			*transactionControlT->phStmt = SQL_NULL_HSTMT;			
		}
		retcode = SQLAllocStmt(*transactionControlT->phDBc, transactionControlT->phStmt); // Statement handle 
		if (retcode != SQL_SUCCESS) {
			SQLErrorHandler(retcode, transactionControlT, *transactionControlT->phStmt, transactionInOutT);
			releaseDbSemaphore(transactionControlT);
			return FALSE;
		}
		if (transactionInOutT->transactionGroup == 0)
			sprintf(QueryBuffer, "INSERT INTO Transactions VALUES ('%s', '%s', '%s', '%s', '%s', '%s', getdate(), NULL, '%.0f', NULL, '%d', '%s', IDENT_CURRENT('Transactions'), NULL, '%s', '%d')",
				FixSingleQuote(transactionControlT->ServiceName, lService), FixSingleQuote(transactionControlT->ProcessedBy, lProcessedBy), FixSingleQuote(RemoteMachine, lRemoteMachine), FixSingleQuote(Identifier, lIdentifier), FixSingleQuote(Source, lSource), FixSingleQuote(Destination, lDestination), (double)Size, Secure, FixSingleQuote(Protocols, lProtocols), transactionControlT->userName, transactionControlT->ProcessID);
		else
			sprintf(QueryBuffer, "INSERT INTO Transactions VALUES ('%s', '%s', '%s', '%s', '%s', '%s', getdate(), NULL, '%.0f', NULL, '%d', '%s', '%d', NULL, '%s', '%d')",
				FixSingleQuote(transactionControlT->ServiceName, lService), FixSingleQuote(transactionControlT->ProcessedBy, lProcessedBy), FixSingleQuote(RemoteMachine, lRemoteMachine), FixSingleQuote(Identifier, lIdentifier), FixSingleQuote(Source, lSource), FixSingleQuote(Destination, lDestination), (double)Size, Secure, FixSingleQuote(Protocols, lProtocols), transactionInOutT->transactionGroup, transactionControlT->userName, transactionControlT->ProcessID);

		retcode = SQLExecDirect(*transactionControlT->phStmt, QueryBuffer, SQL_NTS);
		if ((retcode == SQL_SUCCESS) || (retcode == SQL_SUCCESS_WITH_INFO))
		{
			memset(QueryBuffer, 0, sizeof(QueryBuffer));
			sprintf((char *) QueryBuffer, "SELECT SCOPE_IDENTITY()");

			retcode = SetupTableAccess(transactionControlT->phStmt, QueryBuffer,
									   (BINDCOL_PARAMS *) &NewTransactionsIBCParams,
										ARRAY_LEN(NewTransactionsIBCParams), transactionControlT, transactionInOutT);
			if ((retcode != SQL_SUCCESS) && (retcode != SQL_SUCCESS_WITH_INFO)) {
				SQLErrorHandler(retcode, transactionControlT, *transactionControlT->phStmt, transactionInOutT);
				releaseDbSemaphore(transactionControlT);
				return FALSE;
			}

			memset(&NewTransactionsID, 0, sizeof(NewTransactionsID));
			retcode = SQLFetch(*transactionControlT->phStmt);
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				retcode = SQL_SUCCESS;
			}
			else
			{
				if(retcode != SQL_NO_DATA)
				{
					SQLErrorHandler(retcode, transactionControlT, *transactionControlT->phStmt, transactionInOutT);
				}
			}
		}
		else
			SQLErrorHandler(retcode, transactionControlT, *transactionControlT->phStmt, transactionInOutT);

		SQLFreeStmt(*transactionControlT->phStmt, SQL_DROP);
		*transactionControlT->phStmt = SQL_NULL_HSTMT;			

		releaseDbSemaphore(transactionControlT);

		transactionInOutT->transactionId = NewTransactionsID;
		return (NewTransactionsID != 0);
	}

	return TRUE;
}

_declspec(dllexport)  BOOL CloseTransactionStructDotNet(transactionControl_t *transactionControlT, transactionInOut_t *transactionInOutT)
{
	transactionControlT->phEnv = &transactionControlT->hEnv;
	transactionControlT->phDBc = &transactionControlT->hDBc;
	transactionControlT->phStmt = &transactionControlT->hStmt;

	return CloseTransactionStruct(transactionControlT, transactionInOutT);
}

_declspec(dllexport)  BOOL InsertTransactionRecordDotNet(transactionControl_t *transactionControlT, transactionInOut_t *transactionInOutT, char* RemoteMachine, char* Identifier, char* Source, char* Destination, long long Size, BOOL Secure, char* Protocols)
{
	transactionControlT->phEnv = &transactionControlT->hEnv;
	transactionControlT->phDBc = &transactionControlT->hDBc;
	transactionControlT->phStmt = &transactionControlT->hStmt;

	return InsertTransactionRecord(transactionControlT, transactionInOutT, RemoteMachine, Identifier, Source, Destination, Size, Secure, Protocols);
}

_declspec(dllexport)  BOOL UpdateTransactionRecordAll(transactionControl_t *transactionControlT, transactionInOut_t *transactionInOutT, char* Source, BOOL complete, long long Size, UDWORD Unused, UDWORD result)
{
	SQLRETURN retcode;
	CHAR QueryBuffer[2000];

	InitTIO(transactionInOutT);
	if (!transactionControlT->gotConfig)
	{
		ErrorTIO(transactionInOutT, "transactionControlT has not been initialized", 0);
		return FALSE;
	}

	if (transactionControlT->LogTransactionsToDB)
	{
		if 	(transactionInOutT->transactionId == 0)
		{
			ErrorTIO(transactionInOutT, "No Transaction ID, cannot run UpdateTransactionRecord before InsertTransactionRecord.", 0);
			return FALSE;
		}

		// Create records and get ID
		if (!getDbSemaphore(transactionControlT))
		{
			ErrorTIO(transactionInOutT, "Unable to get Transactions DbSemaphore", GetLastError());
			return FALSE;
		}

		if (*transactionControlT->phStmt != SQL_NULL_HSTMT) {
			SQLFreeStmt(*transactionControlT->phStmt, SQL_DROP);
			*transactionControlT->phStmt = SQL_NULL_HSTMT;			
		}
		retcode = SQLAllocStmt(*transactionControlT->phDBc, transactionControlT->phStmt); // Statement handle 
		if (retcode != SQL_SUCCESS) {
			SQLErrorHandler(retcode, transactionControlT, *transactionControlT->phStmt, transactionInOutT);
			releaseDbSemaphore(transactionControlT);
			return FALSE;
		}

		if (complete)
		{
			if (Source != NULL)
			{
				sprintf(QueryBuffer, "UPDATE Transactions set Source = '%s', EndSize=%.0f, EndTime=getdate(), Result=%d where TransactionID=%ld", Source, (double)Size, result, transactionInOutT->transactionId);
			}
			else
			{
				sprintf(QueryBuffer, "UPDATE Transactions set EndSize=%.0f, EndTime=getdate(), Result=%d where TransactionID=%ld", (double)Size, result, transactionInOutT->transactionId);
			}
			transactionInOutT->transactionId = 0;
		}
		else
		{
			sprintf(QueryBuffer, "UPDATE Transactions set Unused=%d where TransactionID=%ld", Unused, transactionInOutT->transactionId);
		}

		retcode = SQLExecDirect(*transactionControlT->phStmt, QueryBuffer, SQL_NTS);
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			retcode = SQL_SUCCESS;
		}
		else
		{
			SQLErrorHandler(retcode, transactionControlT, *transactionControlT->phStmt, transactionInOutT);
		}

		SQLFreeStmt(*transactionControlT->phStmt, SQL_DROP);
		*transactionControlT->phStmt = SQL_NULL_HSTMT;			

		releaseDbSemaphore(transactionControlT);
	}
	return TRUE;
}

_declspec(dllexport)  BOOL UpdateTransactionRecord(transactionControl_t *transactionControlT, transactionInOut_t *transactionInOutT, BOOL complete, long long Size, UDWORD Unused, UDWORD result)
{
	return UpdateTransactionRecordAll(transactionControlT, transactionInOutT, NULL, complete, Size, Unused, result);
}

_declspec(dllexport)  BOOL UpdateTransactionRecordDotNet(transactionControl_t *transactionControlT, transactionInOut_t *transactionInOutT, BOOL complete, long long Size, UDWORD Unused, UDWORD result)
{
	transactionControlT->phEnv = &transactionControlT->hEnv;
	transactionControlT->phDBc = &transactionControlT->hDBc;
	transactionControlT->phStmt = &transactionControlT->hStmt;

	return UpdateTransactionRecordAll(transactionControlT, transactionInOutT, NULL, complete, Size, Unused, result);
}

_declspec(dllexport)  __int64 FileSize(const char* name)
{
    WIN32_FILE_ATTRIBUTE_DATA fad;
    LARGE_INTEGER size;
    if (!GetFileAttributesEx(name, GetFileExInfoStandard, &fad))
        return -1; // error condition, could call GetLastError to find out more
    size.HighPart = fad.nFileSizeHigh;
    size.LowPart = fad.nFileSizeLow;
    return size.QuadPart;
}

////    DllMain
BOOL APIENTRY DllMain(HANDLE hDll, DWORD dwReason, LPVOID pStuff) 
{
	return TRUE;
}


