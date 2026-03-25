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
'    File Name    :  AfdConfigVb.cpp
'
'    Purpose      :  This file has entry points to the Config DLL for Visual 
'                    Basic callers. Check the AfdConfig.def also.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

#include "basic.h"
#include "AfdConfigProto.h"

static char ModId[] = {__FILE__}; //for debugging

BOOL WINAPI VbGetAfdRoot(char *pAfdRoot, DWORD sAfdRoot, DWORD *cbRet) {

	return(GetAfdRoot(pAfdRoot, sAfdRoot, cbRet));
}

BOOL WINAPI VbGetAfdConfigFile(char *configFile, DWORD sconfigFile, DWORD *cbRet) {

	return(GetAfdConfigFile(configFile, sconfigFile, cbRet));
}

BOOL WINAPI VbInitConfigFile(char *filename) {

	return(InitConfigFile(filename));
}

BOOL WINAPI VbCloseConfigFile() {

	return(CloseConfigFile());
}

BOOL WINAPI VbGetTokenValue(BYTE *token, BYTE *tokenValue, DWORD sBuff, 
							DWORD *cTokenValueSize, BOOL fStartOfFile, 
							BOOL fUptoCurrentBlockEnd) {

	return(GetTokenValue(token, tokenValue, sBuff, cTokenValueSize, fStartOfFile, 
						 fUptoCurrentBlockEnd));
}

BOOL WINAPI VbGetAfdConfigParam(BYTE *section, BYTE *token, BYTE *tokenValue, 
								DWORD sBuff, DWORD *cTokenValueSize, BOOL fStartOfFile) {

	return(GetAfdConfigParam(section, token, tokenValue, sBuff, cTokenValueSize, 
							 fStartOfFile));

}

BOOL WINAPI VbRunningAsAdministrator() {
	return(RunningAsAdministrator());
}


// Added a function to retrieve SQL Login Id Passwd from the Registry
// This call is for AfdAdmin purposes

char  SqlLoginKeyName[30] = {"LoginId"};
char  SqlPasswdKeyName[30] = {"PassWd"};

BOOL WINAPI VbGetSqlLoginPasswd(BYTE *LoginIdBuff, DWORD sLoginIdBuff,
						  BYTE *PasswdBuff, DWORD sPasswdBuff, 
						  DWORD *cbLoginRet, DWORD *cbPasswdRet) 
{
    char keyPath[120];
	LONG ret;
	HKEY keyHandle;
	DWORD  keytype = REG_SZ;

	memset(keyPath, 0, 120);
	strcpy(keyPath, "SOFTWARE\\MOMENTUM\\Intelligent Network Gateway\\SQL");
	
	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,		//handle of openkey
					   keyPath,					//address of name of sub-key to open
					   0,						//reserved
					   KEY_QUERY_VALUE,			//type of access desired
					   &keyHandle				//returned handle
					  );
	if (ret != 0)
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