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
'    File Name    :  AfdRegConfig.c
'
'    Purpose      :  This file has the code for providing making Registry 
'                    for all the AFD Windows NT System Services.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

/*
KnownProblems:
- If a string has only one quote it will taken as a valud string
*/
#include "basic.h"
#include <stdio.h>
#include "AfdConfigProto.h"
#include "QmProto.h"
#include "AfdMisc.h"

#define ALL_BUFFS 80

SC_HANDLE hSCM=NULL;

void *C_OpenFile(char *filename, DWORD recLength);
BOOL C_ReadFile(void *pFile, BYTE *pBuff, DWORD sRead, DWORD *cRead, BOOL *fEof, BOOL fReset);
DWORD ReadNextline(void *pFile, BYTE *buff);
void PutBackLine(void *pFile);
BOOL C_CloseFile(void *pFile);
BOOL ScanUpto(void *pFile, BYTE *token, BOOL fStartOfFile);
WORD GetCurrLinenumber(void *pFile);

void *pFile=NULL;
BOOL fValidate=TRUE;
BOOL fRemoveServices=FALSE;
BOOL fAddServices=FALSE;
// JRM 990607 SCR#505 Allow add new Services/Registry entries without overwriting existing ones.
BOOL fNewServices=FALSE;
BOOL fDisplayUsage=FALSE;
char *pConfigFile=NULL;

BOOL fStopListAvail=FALSE;
BOOL fServiceListAvail=FALSE;

char T_StopSerListStart[]={"[StopServiceList]"};
char T_StopSerListEnd[]={"[EndStopServiceList]"};
char T_ServiceList[]={"[ServiceList]"};
char T_ServiceListEnd[]={"[EndServiceList]"};

char T_ServiceName[]={"ServiceName="};
char T_ServerId[]={"ServerId="};
char T_Executable[]={"ExecutableName="};
char T_Dependency[]={"Dependency="};
char T_EndService[]={"EndService="};

static BYTE defConfigFile[ALL_BUFFS];
static BYTE AfdRoot[ALL_BUFFS];
#define Erc_End 1

//forward references
BOOL ValidateConfigFile(char *pConfigFile);
DWORD AfdDeleteService(char *pServiceName);
DWORD AfdCreateService(char *pServiceName, char *pExecutable, char *pDependencies);
DWORD GetServiceNameToDelete(BYTE *pServiceName);
BOOL AfdRemoveServices(BOOL fValidate);
BOOL AfdAddServices();

static char basePath[]={"SYSTEM\\CurrentControlSet\\Services\\"};
static char paramKey[ALL_BUFFS*2];
HANDLE hKeyAfd;

DWORD AfdCloseParameterKey() {
	return((DWORD) RegCloseKey(hKeyAfd));
}

DWORD AfdCreateParameterKey(BYTE *pServiceName) {

	DWORD disposition, erc;
	
	strcpy(paramKey, basePath);
	strcat(paramKey, pServiceName);
	strcat(paramKey, "\\Parameters\\");

	/*LONG RegCreateKeyEx(
    HKEY hKey,	// handle of an open key 
    LPCTSTR lpSubKey,	// address of subkey name 
    DWORD Reserved,	// reserved 
    LPTSTR lpClass,	// address of class string 
    DWORD dwOptions,	// special options flag 
    REGSAM samDesired,	// desired security access 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,	// address of key security structure 
    PHKEY phkResult,	// address of buffer for opened handle  
    LPDWORD lpdwDisposition 	// address of disposition value buffer 
   );*/
	erc = RegCreateKeyEx(HKEY_LOCAL_MACHINE, paramKey, 0, 
						 NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
						 &hKeyAfd, &disposition);
	return(erc);
}

DWORD AfdAddValue(BYTE *pValueName, void *pValue, DWORD sValue, DWORD type) {
	DWORD erc;

	// JRM 990607 SCR#505 Allow add new Services/Registry entries without overwriting existing ones.
	if (fNewServices && (ERROR_SUCCESS == RegQueryValueEx(
					hKeyAfd,            // handle to key to query
					pValueName,  // address of name of value to query
					NULL,   // reserved
					NULL,       // address of buffer for value type
					NULL,        // address of data buffer
					NULL      // address of data buffer size
					)))
			return(0);
 
	/*LONG RegSetValueEx(
    HKEY hKey,	// handle of key to set value for  
    LPCTSTR lpValueName,	// address of value to set 
    DWORD Reserved,	// reserved 
    DWORD dwType,	// flag for value type 
    CONST BYTE *lpData,	// address of value data 
    DWORD cbData 	// size of value data 
   );*/

	if (type == REG_SZ)
		sValue += 1; //cbData must include the size of the terminating null character
	erc = RegSetValueEx(hKeyAfd, pValueName, 0, type, 
						(const unsigned char *) pValue, sValue);
	return(erc);
}

	
BOOL StripOffQuotes(BYTE *str) {
	WORD iStrLen=strlen(str);
	WORD iSrc, iDest;
	BOOL fQuote=FALSE;

	for((iSrc=iDest=0);(iSrc<iStrLen);iSrc++) {
		if (*(str+iSrc) != '"') {
			*(str+iDest) = *(str+iSrc);
			iDest++;
		}
		else
			fQuote=TRUE;
	}
	*(str+iDest)=0;
	return(fQuote);
}

DWORD GetParam(BYTE *paramName, BYTE *paramValue, DWORD *sParamValue, DWORD *paramType) {
	BYTE buff[ALL_BUFFS];
	DWORD cbRet, erc=0, paramVal, iDelim;
	BOOL fFound, fPutback=FALSE;

	cbRet = ReadNextline(pFile, buff);
	//we expect either "EndService="
	if (cbRet == 0) {
		erc = Erc_End+1;
		goto AllDone;
	}
	fFound = compareStrings(buff, T_EndService, (WORD) strlen((char *) T_EndService));
	if (fFound) {
		erc = Erc_End;
		goto AllDone;
	}
	for(iDelim=0;(iDelim<cbRet);iDelim++)
		if (buff[iDelim] == '=')
			break;
	if (iDelim == cbRet) {
		printf("Syntax error for parameters, on line %d in file %s", 
				GetCurrLinenumber(pFile), pConfigFile);
		erc = Erc_End+1;
		goto AllDone;
	}
	memset(paramName, 0, ALL_BUFFS);
	memset(paramValue, 0, ALL_BUFFS);
	memcpy(paramName, buff, iDelim);
	memcpy(paramValue, buff+iDelim+1, cbRet-iDelim-1);
	fFound = StripOffQuotes(paramValue);
	if (fFound) {
		*paramType = REG_SZ;
		paramValue = StrTrim(StrLTrim(paramValue));
		*sParamValue = strlen(paramValue);
	}
	else {
		paramVal = atoi(paramValue);
		memset(paramValue, 0, ALL_BUFFS);
		*(DWORD *) paramValue = paramVal;
		*paramType = REG_DWORD;
		*sParamValue = 4;
	}
AllDone:
	if (fPutback)
		PutBackLine(pFile);
	return(erc);
}

BOOL ValidateConfigFile(char *pConfigFile) {

	char readBuff[ALL_BUFFS];
	DWORD cbRet;
	BOOL fSuccess;
	BOOL fStart;

	fSuccess = InitConfigFile(pConfigFile);

	if (!fSuccess)
		return(fSuccess);
	fSuccess = GetTokenValue(T_StopSerListStart, readBuff, sizeof(readBuff), &cbRet,
							 TRUE/*start-of-file*/, FALSE);
	fStart = fSuccess;
	fSuccess = GetTokenValue(T_StopSerListEnd, readBuff, sizeof(readBuff), &cbRet,
							 FALSE, FALSE);
	if (fStart && fSuccess)
		fStopListAvail = TRUE;
	else if (!fStart && !fSuccess) {
		printf("No %s present.\n", T_StopSerListStart);
		fStopListAvail = FALSE;
	}
	else {
		printf("Error in %s\n.", T_StopSerListStart);
		fSuccess = FALSE;
		goto AllDone;
	}
	fSuccess = GetTokenValue(T_ServiceList, readBuff, sizeof(readBuff), &cbRet,
							 TRUE/*start-of-file*/, FALSE);
	fStart = fSuccess;
	fSuccess = GetTokenValue(T_ServiceListEnd, readBuff, sizeof(readBuff), &cbRet,
							 FALSE, FALSE);

	if (fStart && fSuccess)
		fServiceListAvail = TRUE;
	else if (!fStart && !fSuccess) {
		fServiceListAvail=FALSE;
		printf("No %s present.\n", T_ServiceList);
	} else {
		printf("Error in %s\n.", T_ServiceList);
		fSuccess = FALSE;
	}
AllDone:
	CloseConfigFile(pConfigFile);
	return(fSuccess);
}

DWORD AfdDeleteService(char *pServiceName) {
	SC_HANDLE hService=NULL;
	DWORD erc=ercOk;

	hService = OpenService(hSCM, pServiceName, DELETE);
	if (!hService)
		erc = GetLastError();
	else {
		if (!DeleteService(hService))
			erc = GetLastError();
		CloseServiceHandle(hService);
	}
	return(erc);
}

DWORD AfdCreateService(char *pServiceName, char *pExecutable, char *pDependencies) {

	SC_HANDLE hService=NULL;
	char ExecutableName[256];
	DWORD erc=ercOk;

	memset(ExecutableName, 0, 256);
	strcpy(ExecutableName, AfdRoot);
	strcat(ExecutableName, "\\Bin\\");
	strcat(ExecutableName, pExecutable);
	strcat(ExecutableName, " SCM ");
	strcat(ExecutableName, pServiceName);

	// JRM 990607 SCR#505 Allow add new Services/Registry entries without overwriting existing ones.
	if (fNewServices)
		hService = OpenService(
						hSCM,  // handle to service control manager database
						pServiceName, // pointer to name of service to start
						SERVICE_QUERY_CONFIG  // type of access to service
						);
	if (!hService)
		hService = CreateService(hSCM,							//SCM Handle
								 pServiceName,					//ServiceName
								 pServiceName,					//DisplayName
								 SERVICE_ALL_ACCESS,			//access
								 SERVICE_WIN32_OWN_PROCESS | 
								 SERVICE_INTERACTIVE_PROCESS,	//type
								 SERVICE_AUTO_START,			//startUp
								 SERVICE_ERROR_NORMAL,			//normal error handling
								ExecutableName,				//executable
								 NULL,							//LoadOrderGroup ?
								 NULL,							//Tag ordering?
								 pDependencies,					//dependencies
								 NULL,							//Local system
								 NULL							//Password
								);
	if (!hService) {
		erc = GetLastError();
	} else
		CloseServiceHandle(hService);

	return(erc);
}


DWORD GetServiceNameToDelete(BYTE *pServiceName) {
	BYTE buff[ALL_BUFFS];
	BYTE lclStr[ALL_BUFFS];
	DWORD cbRet, cLen, erc=0;
	BOOL fFound, fPutback=FALSE;

	memset(pServiceName, 0, ALL_BUFFS);
	cbRet = ReadNextline(pFile, buff);
	//we expect either "[EndStopServiceList]" or  "ServiceName="
	fFound = compareStrings(buff, T_StopSerListEnd, 
						    (WORD) strlen((char *) T_StopSerListEnd));
	if (fFound) {
		erc = Erc_End;
		goto AllDone;
	}
	//look for "ServiceName="
	cLen = strlen((char *) T_ServiceName);
	fFound = compareStrings(buff, T_ServiceName, (WORD) strlen((char *) T_ServiceName));
	if (!fFound) {
		printf("Did not find expected token (%s) at line number %d\n", T_ServiceName,
				GetCurrLinenumber(pFile));
		erc = Erc_End+1;
		goto AllDone;
	}
	//copy the name into the buffer to be sent back
	memcpy(pServiceName, &buff[cLen], (cbRet-cLen));
	StripOffQuotes(pServiceName);

	//now we expect "ServerId=" or "[EndStopServiceList]" or the next ServiceName
	cbRet = ReadNextline(pFile, buff);
	fFound = compareStrings(buff, T_StopSerListEnd, 
						  (WORD) strlen((char *) T_StopSerListEnd));
	if (fFound) { //we will hit this the next time for now put it back
		fPutback = TRUE;
		goto AllDone;
	}
	//now look for ServerId
	cLen = strlen((char *) T_ServerId);
	fFound = compareStrings(buff, T_ServerId, (WORD) cLen);
	if (fFound) {
		memset(lclStr, 0, ALL_BUFFS);
		memcpy(lclStr, buff+cLen, cbRet-cLen);
		StripOffQuotes(lclStr);
		memcpy(pServiceName+(strlen(pServiceName)), lclStr, strlen(lclStr));
	}
	else {
		//handle this in the next cycle
		fPutback = TRUE;
	}
AllDone:
	if (fPutback)
		PutBackLine(pFile);
	return(erc);
}

DWORD GetService(BYTE *serviceName, BYTE *serverId, BYTE *executable, BYTE **pDependencies) {
	BYTE buff[ALL_BUFFS];
	BYTE lclStr[ALL_BUFFS];
	DWORD cbRet, cLen, erc=0;
	BOOL fFound, fPutback=FALSE, fMoreDependencies;
	BYTE *pDepCurr, *pDep=NULL;
	DWORD sTotalStrLen=0;
	DWORD cCurrStrLen;

	memset(serviceName, 0, ALL_BUFFS);
	memset(executable, 0, ALL_BUFFS);
	memset(serverId, 0, ALL_BUFFS);
	if (pDependencies) {
		// JRM 990607 SCR#n/a Fixed - Memory leaked due to reassignment.
		free(*pDependencies);
		*pDependencies=NULL;
	}
	cbRet = ReadNextline(pFile, buff);
	//we expect either "[EndServiceList]" or  "ServiceName="
	fFound = compareStrings(buff, T_ServiceListEnd, 
						    (WORD) strlen((char *) T_ServiceListEnd));
	if (fFound) {
		erc = Erc_End;
		goto AllDone;
	}
	//look for "ServiceName="
	cLen = strlen((char *) T_ServiceName);
	fFound = compareStrings(buff, T_ServiceName, (WORD) strlen((char *) T_ServiceName));
	if (!fFound) {
		printf("Did not find expected token (%s) at line number %d\n", T_ServiceName,
				GetCurrLinenumber(pFile));
		erc = Erc_End+1;
		goto AllDone;
	}
	//copy the name into the buffer to be sent back
	memcpy(serviceName, &buff[cLen], (cbRet-cLen));
	StripOffQuotes(serviceName);
	//now look for ServerId
	cbRet = ReadNextline(pFile, buff);
	if (cbRet == 0) {
		printf("Did not find expected token (%s) at line number %d\n", T_ServerId,
				GetCurrLinenumber(pFile));
		erc = Erc_End+1;
		goto AllDone;
	}
	cLen = strlen((char *) T_ServerId);
	fFound = compareStrings(buff, T_ServerId, (WORD) cLen);
	if (!fFound) {
		printf("Did not find expected token (%s) at line number %d\n", T_ServerId,
				GetCurrLinenumber(pFile));
		erc = Erc_End+1;
		goto AllDone;
	}
	memcpy(serverId, buff+cLen, cbRet-cLen);
	StripOffQuotes(serverId);
	//now look for Executable name
	cbRet = ReadNextline(pFile, buff);
	if (cbRet == 0) {
		printf("Did not find expected token (%s) at line number %d\n", T_Executable,
				GetCurrLinenumber(pFile));
		erc = Erc_End+1;
		goto AllDone;
	}
	cLen = strlen((char *) T_Executable);
	fFound = compareStrings(buff, T_Executable, (WORD) cLen);
	if (!fFound) {
		printf("Did not find expected token (%s) at line number %d\n", T_Executable,
				GetCurrLinenumber(pFile));
		erc = Erc_End+1;
		goto AllDone;
	}
	memcpy(executable, buff+cLen, cbRet-cLen);
	StripOffQuotes(executable);

	//now look for dependencies
	fMoreDependencies = TRUE;
	cLen = strlen(T_Dependency);
	do {//now we expect "Dependency=" or "[EndStopServiceList]" or the next ???
		cbRet = ReadNextline(pFile, buff);
		fFound = compareStrings(buff, T_StopSerListEnd, 
								(WORD) strlen((char *) T_StopSerListEnd));
		if (fFound) { //we will hit this the next time for now put it back
			fPutback = TRUE;
			fMoreDependencies = FALSE;
			break;
		}
		fFound = compareStrings(buff, T_Dependency, (WORD) cLen);
		if (!fFound) {
			//handle the token in the next cycle
			fPutback = TRUE;
			break;
		}
		memset(lclStr, 0, ALL_BUFFS);
		memcpy(lclStr, buff+cLen, cbRet-cLen);
		StripOffQuotes(lclStr);
		cCurrStrLen = sTotalStrLen;
		pDepCurr=pDep;
		sTotalStrLen += strlen(lclStr)+1;
		pDep = (BYTE *) zmalloc(sTotalStrLen+2); //double null termination at end
		if (pDepCurr) {
			memcpy(pDep, pDepCurr, cCurrStrLen);
			free(pDepCurr);
		}
		strcpy(pDep+cCurrStrLen, lclStr);
	} while (fMoreDependencies);
	*pDependencies = pDep;
AllDone:
	if (fPutback)
		PutBackLine(pFile);
	return(erc);
}

BOOL AfdRemoveServices(BOOL fValidate) {
	BOOL fSuccess;
	BOOL fMoreServices=TRUE;
	DWORD erc;
	BYTE serviceName[ALL_BUFFS];
	BOOL fEof=FALSE;

	if (!fServiceListAvail) {
		printf("ServiceList not available.\n");
		return(FALSE);
	}
	fSuccess = ScanUpto(pFile, T_StopSerListStart, TRUE);
	if (fSuccess) {
		printf("Found %s on line %d\n", T_StopSerListStart, GetCurrLinenumber(pFile));
	} else {
		printf("Inconsistency in AfdRemoveServices(), did not find %s\n", T_StopSerListStart);
		return(fSuccess);
	}
	do {
		erc = GetServiceNameToDelete(serviceName);
		fMoreServices = (erc == ercOk);
		fSuccess = (erc == ercOk || erc == Erc_End);
		if (!fMoreServices)
			break;
		if (fValidate)
			continue;
		erc = AfdDeleteService(serviceName);
		printf("Status %d in deleting service %s.\n", erc , serviceName);
	} while (fMoreServices);

	return(fSuccess);
}

BOOL AfdAddServices(BOOL fValidate) {
	DWORD erc, paramType, sParamValue;
	BYTE serviceName[ALL_BUFFS], executable[ALL_BUFFS], paramName[ALL_BUFFS], paramValue[ALL_BUFFS], serverId[ALL_BUFFS];
	BYTE *pDependencies=NULL;
	BOOL fSuccess, fMoreServices, fMoreParams;

	if (!fServiceListAvail) {
		printf("Start list not available.\n");
		return(FALSE);
	}

	fSuccess = ScanUpto(pFile, T_ServiceList, TRUE);
	if (fSuccess) {
		printf("Found %s on line %d\n", T_ServiceList, GetCurrLinenumber(pFile));
	} else {
		printf("Inconsistency in AfdAddServices(), did not find %s\n", T_ServiceList);
		return(fSuccess);
	}

	do {
		erc = GetService(serviceName, serverId, executable, &pDependencies);
		fMoreServices = (erc == ercOk);
		fSuccess = (erc == ercOk || erc == Erc_End);
		if (!fMoreServices)
			break;
		if (fValidate)
			continue;
		printf("Adding service %s for ServerId %s ....", serviceName, serverId);
		strcat(serviceName, serverId);
		erc = AfdCreateService(serviceName, executable, pDependencies);
		if (erc == 0) {
			printf("done.\n");
			printf("Adding parameters for %s ....", serviceName);
			erc = AfdCreateParameterKey(serviceName);
			if (erc == 0) {
				memset(paramName, 0, ALL_BUFFS);
				memcpy(paramName, T_ServerId, strlen(T_ServerId)-1);
				erc = AfdAddValue(paramName, serverId, strlen(serverId), REG_SZ);
				fMoreParams = TRUE;
				do {
					erc = GetParam(paramName, paramValue, &sParamValue, &paramType);
					fMoreParams = (erc == 0);
					fSuccess = (erc == 0 || erc == Erc_End);
					if (fMoreParams)
						AfdAddValue(paramName, paramValue, sParamValue, paramType);
				} while (fMoreParams);
			} //created the Parameter key successfully
			printf("done.\n");
			AfdCloseParameterKey();
		}
		else {
			printf("error %d\n", erc);
			fSuccess = ScanUpto(pFile, T_EndService, FALSE);
			if (!fSuccess) {
				printf("Did not find the block delimiter %s.\n", T_EndService);
				fMoreServices = FALSE;
			}
		}
	} while (fMoreServices);
	return(fSuccess);
}

DWORD main(int argc, char **argv) {
	DWORD erc, cbRet;
	int i;
	char *currArg;
	BOOL fSuccess;

	printf("\nAFD Service/Registry Configurator Version 0.2.LIMS.\n");

	for(i=1;(i<argc);i++) {
		currArg = *(argv+i);
		if (*currArg != '/') { //it is the config file name
			if (!pConfigFile) //name already set
				pConfigFile = currArg;
		} else { //it is an option
			if (*(currArg+1) == '?') {
				fDisplayUsage=TRUE;
				break;
			}
			//we should be having a whole lot of else(s) here, but makes code messy
			if (*(currArg+1) == 'V')
				fValidate = TRUE;
			if (*(currArg+1) == 'I')
					fAddServices = TRUE;
			if (*(currArg+1) == 'D')
				fRemoveServices = TRUE;
			if (*(currArg+1) == 'A') {
				fAddServices = TRUE;
				fRemoveServices = TRUE;
			}
			// JRM 990607 SCR#505 Allow add new Services/Registry entries without overwriting existing ones.
			if (*(currArg+1) == 'N') {
				fAddServices = TRUE;
				fNewServices = TRUE;
			}
		} //it is an option

	}

	if (fDisplayUsage) {
		printf("\nUsage is AfdRegConfig /Option ConfigFile.\n");
		printf("\nOptions are /V - Validate the file only\n");
		printf("                 For all following options, the file is first validated\n");
		printf("            /D - Delete services only\n");
		printf("            /I - Insert services only\n");
		// JRM 990607 SCR#505 Allow add new Services/Registry entries without overwriting existing ones.
		printf("            /N - Insert new services only\n");
		printf("            /A - Delete and Add services.\n\n");
		return(0);
	}

	if (!GetAfdRoot(AfdRoot, sizeof(AfdRoot), &cbRet)) {
		printf("\nCould not get AfdRoot.\n");
		return(1);
	}

	if (!pConfigFile) {
		strcpy(defConfigFile, AfdRoot);
		strcat(defConfigFile, "\\Config\\AfdRegConfig.txt");
		pConfigFile = defConfigFile;
		printf("\nConfig file not specified, defaulting to %s.\n\n", pConfigFile);
	}

	fSuccess = ValidateConfigFile(pConfigFile);
	pFile = C_OpenFile(pConfigFile, ALL_BUFFS);
	if (!pFile) {
		erc = GetLastError();
		printf("\nError %d opening %s.\n", erc, pConfigFile);
		return(erc);
	}
/*	if (fStopListAvail)
		fSuccess = AfdRemoveServices(TRUE);
	if (fSuccess)
		if (fServiceListAvail)
			fSuccess = AfdAddServices(TRUE);
	if (!fSuccess) {
		printf("\n%s file in not a valid AfdConfig file.\n");
		return(0);
	}
	*/
	printf("\n%s file validated.\n", pConfigFile);

	if (!fRemoveServices && !fAddServices) {
		C_CloseFile(pFile);
		return(0);
	}

	hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCM) {
		erc = GetLastError();
		printf("Error %x opening the ServiceControlManager", GetLastError());
		return(erc);
	}

	if (fRemoveServices)
		if (!AfdRemoveServices(FALSE))
			return(ercOk);
	else
		printf("Services removed.\n");

	if (fAddServices)
		if (!AfdAddServices(FALSE))
			return(ercOk);
	else
		printf("Services added.\n");

	CloseServiceHandle(hSCM);
	hSCM=NULL;

	// JRM 990607 SCR#n/a Fixed a memory and resource leak.
	if (fRemoveServices || fAddServices)
		C_CloseFile(pFile);

	return(0);
}