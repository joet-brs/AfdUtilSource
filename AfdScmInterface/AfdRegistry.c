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
'    Module       :  AFDScmInterface
'    Date         :  Feb. 1998
'
'    File Name    :  AfdRegistry.c
'
'    Purpose      :  This file has the code for providing common APIs to all the
'                    AFD TPs, QueueManager and AFDServer to get entries from the
'                    Windows NT Registry
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

#include <basic.h>
#include <winreg.h>

extern char *pServiceName;
static char basePath[]={"SYSTEM\\CurrentControlSet\\Services\\"};
static char postFix[]= {"\\Parameters\\"};

LONG AfdTpQueryRegValue(char *keyname, DWORD *keytype, 
					    BYTE *readBuff, DWORD sBuff, DWORD *cbRet) {

	char keyPath[120];
	LONG ret;
	HKEY keyHandle;

	memset(keyPath, 0, 120);
	strcpy(keyPath, basePath);
	if (strcat_s(keyPath, sizeof(keyPath), pServiceName)) return(ERROR_NOT_ENOUGH_MEMORY);
	if (strcat_s(keyPath, sizeof(keyPath), postFix)) return(ERROR_NOT_ENOUGH_MEMORY);

	ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE,		//handle of openkey
					   keyPath,					//address of name of sub-key to open
					   0,						//reserved
					   KEY_QUERY_VALUE,			//type of access desired
					   &keyHandle				//returned handle
					  );
	if (ret != ERROR_SUCCESS)
		return(ret);

	*cbRet = sBuff;

	ret = RegQueryValueEx(keyHandle,			//handle of open key
						  keyname,				//address of name of value to query
						  0,					//reserved
						  keytype,				//returned key type
						  readBuff,				//address of data buffer
						  cbRet					//size of data buff
						 );

	//In case of a REG_SZ the NULL BYTE is also added in the StringLength,
	//we need to strip this

	if (*keytype == REG_SZ)
		if (*cbRet > 0)
			(*cbRet)--;

	RegCloseKey(keyHandle);

	return(ret);
}

DWORD AfdTpGetRegBasePath(char *regBasePath, DWORD sPath, DWORD *cbRet) {

	*cbRet = strlen(basePath) + strlen(pServiceName);

	if (sPath < *cbRet) {
		*cbRet = 0;
		return(1);
	}

	memset(regBasePath, 0, sPath);
	if (strcpy_s(regBasePath, sPath, basePath)) return(1);
	if (strcat_s(regBasePath, sPath, pServiceName)) return(1);

	return(0);
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