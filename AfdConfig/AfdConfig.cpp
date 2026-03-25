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


#include "basic.h"
#include <stdio.h>
#include <string.h>
#include <conio.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define CR 0x0d
#define LF 0x0a

#define TABLE_DOES_NOT_EXIST -1
#define TABLE_NEEDS_REFRESHING -2

typedef struct {
	char fileName[MAX_PATH];
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
	SYSTEMTIME LastWriteTime;
} file_t;

file_t *pFile=NULL;
// g_bUseDatabase should be TRUE if we are reading the AfdConfig.ini file otherwise false
// Assume we are reading from the AfdConfig.ini file until a call is made to 
// InitConfigFile() with another filename.
BOOL g_bUseDatabase = TRUE;


char AfdRootKey[]={"SOFTWARE\\Momentum"};

extern BOOL InitConfigTable(char *filename);
extern BOOL GetDbConfigParam(char *section, char *token, char *tokenValue, 
							DWORD sBuff, DWORD *cTokenValueSize, 
							BOOL fStartOfFile);
extern BOOL InitDb();
extern void ExitDb();
 
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
	void *p = calloc(1,s);

	if (!p) {
		_asm int 3;
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

_declspec(dllexport) BOOL GetAfdRoot(char *pAfdRoot, DWORD sAfdRoot, DWORD *cbRet) {
	
	LONG ret;
	HKEY keyHandle;
	static char keyname[]={"AfdRoot"};
	DWORD keytype;



	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,		//handle of openkey
					   AfdRootKey,				//address of name of sub-key to open
					   0,						//reserved
					   KEY_QUERY_VALUE,			//type of access desired
					   &keyHandle				//returned handle
					  );
	if (ret != ERROR_SUCCESS)
		return(FALSE);

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

_declspec(dllexport) BOOL GetAfdConfigFile(char *configFile, DWORD sconfigFile, 
										   DWORD *cbRet) 
{
	char AfdRoot[255];	// LSL 990211 SCR#404 Change size from 20 to 255.
	DWORD cAfdRoot;
	BOOL fSuccess;
	static char defAfdConfigFile[]={"\\Config\\AfdConfig.ini"};

	fSuccess = GetAfdRoot(AfdRoot, sizeof(AfdRoot), &cAfdRoot);

	if (!fSuccess)
		return(fSuccess);

	*cbRet = strlen(AfdRoot) + strlen(defAfdConfigFile);

	if (sconfigFile < *cbRet) {
		*cbRet = 0;
		return(FALSE);
	}

	memset(configFile, 0, sconfigFile);
	strcpy(configFile, AfdRoot);
	strcat(configFile, defAfdConfigFile);

	return(TRUE);
}


file_t *Config_OpenFile(char *filename, DWORD recLength)
{
	BY_HANDLE_FILE_INFORMATION FileInformation;
			
	file_t *pFile = (file_t *) zmalloc(sizeof(file_t));
	BOOL fSuccess=TRUE;
	DWORD cbRet;

	if(filename)
		strcpy(pFile->fileName, filename);
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
	// Begin
	

	if(!GetFileInformationByHandle(
			pFile->fh,           // handle to file 
			&FileInformation // buffer
			)) {
		fSuccess = FALSE;	
		goto AllDone;
		}

	if(!FileTimeToSystemTime( &FileInformation.ftLastWriteTime,  // file time to convert
		&pFile->LastWriteTime)) {
		fSuccess = FALSE;	
		goto AllDone;
		}
	
	
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
#define RECORD_LENGTH 80
	char *pfilename;
	
	g_bUseDatabase = TRUE;

	if(filename) {
		// Search for last occurrence of '\' and determine if user wants AfdConfig.ini
		pfilename = strrchr(filename,'\\');
		if(pfilename) {
			pfilename++;
			if(stricmp(pfilename,"AfdConfig.ini")) g_bUseDatabase = FALSE;
			}
		}
	
	if(g_bUseDatabase) {
		if(!InitConfigTable(filename)) g_bUseDatabase = FALSE;
		}
	
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
	BYTE localBuff[100];
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
		fSuccess = Config_ReadFile(pFile, localBuff, 100, &cRead, &fEof);
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
					memcpy(tokenValue,  localBuff+strlen((char *)token), *cTokenValueSize);
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

	if(g_bUseDatabase)
		return GetDbConfigParam((char *) section, (char *) token, (char *) tokenValue, 
							sBuff, cTokenValueSize, fStartOfFile);
	
	if (!pFile) {
		return(FALSE);
	}
	if (fStartOfFile)
		ResetConfigFile();
	
	
	sectionName = (BYTE *) zmalloc(strlen(section) + 3);
	strcpy(sectionName, sectionStart);
	strcat(sectionName, section);
	strcat(sectionName, sectionEnd);

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
	strcat(sectionName, section);
	strcat(sectionName, sectionEnd);

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
	BYTE localBuff[100];
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
		fSuccess = Config_ReadFile(pFile, localBuff, 100, &cRead, &fEof);
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
					memcpy(tokenValue,  localBuff+strlen((char *)token), *cTokenValueSize);
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



////    DllMain
BOOL APIENTRY DllMain(HANDLE hDll, DWORD dwReason, LPVOID pStuff) 
{
switch(dwReason) {
	case DLL_PROCESS_ATTACH:
		// The DLL is being mapped into the process's address space.
		
		// hMutex insures that only one thread can create/refresh the CONFIG
		// database table at a time.
		//hMutex = CreateMutex(NULL,FALSE,"MomentumConfigWrite");
		
		// hSemRead insures that we dont write to the CONFIG database table while 
		// there are outstanding read requests.
		//hSemRead = CreateSemaphore(NULL,0,10000,"MomentumConfigRead");
		break;
	case DLL_THREAD_ATTACH:
		// A thread is being created.
		break;
	case DLL_THREAD_DETACH:
		// A thread is exiting cleanly.
		break;
	case DLL_PROCESS_DETACH:
		// The DLL is being unmapped from the process's address space.
		break;
	}
	return TRUE;
}


