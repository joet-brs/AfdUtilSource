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
'    File Name    :  AfdCheckAdmin.c
'
'    Purpose      :  This file has the code for checking if user is current user
'                    has Admin priviledges.
'
'    Author       :  AA
'******************************************************************************/
/*Modification History
*/

//From MSDN July 1996

#include <basic.h>
#include <malloc.h>

#define QuickAndDirty
#ifdef QuickAndDirty
char rgAccessFile[]={"\\Config\\AfdAccess.txt"};
extern BOOL GetAfdRoot(char *pAfdRoot, 
					   DWORD sAfdRoot, DWORD *cbRet);
_declspec(dllexport) BOOL RunningAsAdministrator() {
	BOOL  fAdmin;
	char rgCheckAccessFile[MAX_PATH];
	DWORD cbRet;
	HANDLE fh=INVALID_HANDLE_VALUE;
	fAdmin = GetAfdRoot(rgCheckAccessFile, 
						sizeof(rgCheckAccessFile), 
						&cbRet);
	if (!fAdmin)
		return(fAdmin);
	if(strcat_s(rgCheckAccessFile, sizeof(rgCheckAccessFile), rgAccessFile)) return(FALSE);
	fh = CreateFile(rgCheckAccessFile,
					GENERIC_READ, FILE_SHARE_READ,	// share mode 
					0, // pointer to security descriptor 
					OPEN_EXISTING,	// how to create 
					FILE_ATTRIBUTE_NORMAL,	// file attributes 
					0);
	if (fh == INVALID_HANDLE_VALUE)
		fAdmin = FALSE;
	else
		fAdmin = TRUE;
	if (fAdmin)
		CloseHandle(fh);
	return(fAdmin);
}
#else
_declspec(dllexport) BOOL RunningAsAdministrator() {
	BOOL  fAdmin;
	HANDLE htkThread;
	TOKEN_GROUPS *ptg = NULL;
	DWORD cbTokenGroups;
	DWORD iGroup;
	SID_IDENTIFIER_AUTHORITY SystemSidAuthority= SECURITY_NT_AUTHORITY;
	PSID psidAdmin;

	/*First we'll retrieve a security token for our execution thread. 
	If our thread doesn't have a security token, we'll use the token from our 
	process environment*/
  	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &htkThread))
		if (GetLastError() == ERROR_NO_TOKEN) {
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &htkThread))
			return FALSE;
	}
	else return FALSE;
  
	/*Now we'll retrieve a list of security identifiers (SIDs) to which the current 
	user belongs. We'll call GetTokenInformation twice. In the first call, we pass a 
	NULL pointer for the data buffer and set cbTokenGroups to zero. That's a signal 
	that we want to know how much space to allocate for the data.*/
	if (GetTokenInformation(htkThread, TokenGroups, NULL, 0, &cbTokenGroups))
		return FALSE;
	
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		return FALSE;
	
	if (!(ptg= (TOKEN_GROUPS *) malloc(cbTokenGroups))) 
		return FALSE;
	
	if (!GetTokenInformation(htkThread, TokenGroups, ptg, cbTokenGroups,
							 &cbTokenGroups)) {
		fAdmin = FALSE;
		goto AllDone;
	}
	
	//Now we'll create a SID for the administrator group.
	if (!AllocateAndInitializeSid(&SystemSidAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &psidAdmin)) {
		fAdmin = FALSE;
		goto AllDone;
	}

	//Then we'll compare psidAdmin against each of the group SIDs for this security token.
	fAdmin= FALSE;
	for (iGroup= 0; iGroup < ptg->GroupCount; iGroup++)
		if (EqualSid(ptg->Groups[iGroup].Sid, psidAdmin)) {
			fAdmin= TRUE;
			break;
		}
	
	//deallocate the SID we allocated above.
	FreeSid(psidAdmin);

AllDone:
	if (ptg)
		free(ptg);
	return(fAdmin);
}
#endif

