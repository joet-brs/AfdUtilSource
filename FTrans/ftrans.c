/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    ftrans.c

Abstract:

    This module demonstrates a simple file transfer using ISAPI application
      using the Async TransmitFile support with callback.


Revision History:
--*/

#include <windows.h>
#include "httpext.h"

# include "openf.h"

#include "Qmproto.h"
#include <include.h>
#include <AfdConfigProto.h>

/************************************************************
 *  Global Data
 ************************************************************/

LIST_ENTRY g_lWorkItems;
CRITICAL_SECTION g_csWorkItems;
char  AFDquename[32];
char  AFDworkdir[94];

# define USE_WORK_QUEUE   (0)


DWORD
SendHeaderToClient( IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszErrorMsg);


DWORD
SendFileToClient( IN EXTENSION_CONTROL_BLOCK  * pecb, IN LPCSTR pszFile);

DWORD
SendFileOver( IN EXTENSION_CONTROL_BLOCK  * pecb,
              IN HANDLE hFile, 
              IN LPCSTR pszFile);

VOID
sendQueueMessage(IN EXTENSION_CONTROL_BLOCK * pECB, 
				 IN LPCSTR pszFile,
				 DWORD error);

VOID
InitAfdInfo();


/************************************************************
 *    Functions
 ************************************************************/


BOOL WINAPI
DllMain(
     IN HINSTANCE hinstDll,
     IN DWORD     fdwReason,
     IN LPVOID    lpvContext OPTIONAL
     )
/*++

 Routine Description:

   This function DllMain() is the main initialization function for
    this DLL. It initializes local variables and prepares it to be invoked
    subsequently.

 Arguments:

   hinstDll          Instance Handle of the DLL
   fdwReason         Reason why NT called this DLL
   lpvReserved       Reserved parameter for future use.

 Return Value:

    Returns TRUE is successful; otherwise FALSE is returned.

--*/
{
    BOOL    fReturn = TRUE;

    switch (fdwReason ) {

    case DLL_PROCESS_ATTACH:
    {
		OutputDebugString( "Initializing the global data in ftrans.dll\n");
		//
		// Initialize various data and modules.
		//
		InitializeCriticalSection(&g_csWorkItems);
		InitializeListHead( &g_lWorkItems);
		InitFileHandleCache();
		InitAfdInfo();

		break;
	} /* case DLL_PROCESS_ATTACH */

	case DLL_PROCESS_DETACH:
	{

		OutputDebugString( "Unloading ftrans.dll\n");
		//
		// Only cleanup when we are called because of a FreeLibrary().
		//  i.e., when lpvContext == NULL
		// If we are called because of a process termination,
		//  dont free anything. System will free resources and memory for us.
		//

		CleanupFileHandleCache();
		if ( lpvContext != NULL)
		{
			DeleteCriticalSection(&g_csWorkItems);
		}

		break;
	} /* case DLL_PROCESS_DETACH */

    default:
        break;
    } /* switch */

    return ( fReturn);
}  /* DllMain() */




DWORD WINAPI
HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK * pecb
    )
/*++

 Routine Description:

   This function performs the necessary action to send response for the 
    request received from the client. It picks up the name of a file from
    the pecb->lpszQueryString and transmits that file to the client.

 Arguments:

   pecb          pointer to ECB containing parameters related to the request.

 Return Value:

    Returns HSE_* status codes
--*/
{
    DWORD hseStatus;
/*	DWORD buffLen;
    CHAR  pchBuffer[1024];*/

    OutputDebugString( "WebMailbox-Download Started... \n");
	pecb->ServerSupportFunction(pecb->ConnID, HSE_APPEND_LOG_PARAMETER, "WebMailbox-Download Started... ", 0, 0);
    
/*	strcpy(pchBuffer, "AUTH_USER - ");
	buffLen = sizeof(pchBuffer) - strlen(pchBuffer);
	pecb->GetServerVariable(pecb->ConnID, "AUTH_USER", &pchBuffer[strlen(pchBuffer)], &buffLen);
	strcat(pchBuffer, ", LOGON_USER - ");
	buffLen = sizeof(pchBuffer) - strlen(pchBuffer);
	pecb->GetServerVariable(pecb->ConnID, "LOGON_USER", &pchBuffer[strlen(pchBuffer)], &buffLen);
	strcat(pchBuffer, ", REMOTE_ADDR - ");
	buffLen = sizeof(pchBuffer) - strlen(pchBuffer);
	pecb->GetServerVariable(pecb->ConnID, "REMOTE_ADDR", &pchBuffer[strlen(pchBuffer)], &buffLen);
	strcat(pchBuffer, ", REMOTE_HOST - ");
	buffLen = sizeof(pchBuffer) - strlen(pchBuffer);
	pecb->GetServerVariable(pecb->ConnID, "REMOTE_HOST", &pchBuffer[strlen(pchBuffer)], &buffLen);
	strcat(pchBuffer, ", REMOTE_USER - ");
	buffLen = sizeof(pchBuffer) - strlen(pchBuffer);
	pecb->GetServerVariable(pecb->ConnID, "REMOTE_USER", &pchBuffer[strlen(pchBuffer)], &buffLen);

    OutputDebugString( pchBuffer);
	OutputDebugString( "\n");*/

    if ( pecb->lpszPathTranslated == NULL ||
        *pecb->lpszPathTranslated == '\0' 
        ) {
        
	    OutputDebugString( "WebMailbox-Error File Name was NULL\n");
		pecb->ServerSupportFunction(pecb->ConnID, HSE_APPEND_LOG_PARAMETER, "WebMailbox-Error File Name was NULL", 0, 0);
        hseStatus = SendHeaderToClient( pecb, "File Not Specified");
    } else {

//        hseStatus = SendFileToClient( pecb, pecb->lpszQueryString);
        hseStatus = SendFileToClient( pecb, pecb->lpszPathTranslated);
    }
    
    return ( hseStatus);
    
} // HttpExtensionProc()




BOOL WINAPI
GetExtensionVersion(
    HSE_VERSION_INFO * pver
    )
{
    pver->dwExtensionVersion = MAKELONG( 1, 0 );
    strcpy( pver->lpszExtensionDesc,
           "File Transfer using TransmitFile" );
    
    return TRUE;
}


BOOL WINAPI
TerminateExtension(
    DWORD dwFlags
    )
/*++

Purpose:

    This is optional ISAPI extension DLL entry point.
    If present, it will be called before unloading the DLL,
    giving it a chance to perform any shutdown procedures.
    
Arguments:
    
    dwFlags - specifies whether the DLL can refuse to unload or not
    
Returns:
    
    TRUE, if the DLL can be unloaded
    
--*/
{
    return TRUE;
}





DWORD
SendHeaderToClient( 
    IN EXTENSION_CONTROL_BLOCK  * pecb, 
    IN LPCSTR pszErrorMsg
    )
{
	HSE_SEND_HEADER_EX_INFO	SendHeaderExInfo;
	char szStatus[]     = "200 OK";
	char szHeader[1024];
	DWORD	error;
    CHAR    pchBuffer[1024];

    //
    //  Note the HTTP header block is terminated by a blank '\r\n' pair,
    //  followed by the document body
    //

    wsprintf( szHeader,
              "Content-Type: text/html\r\n"
              "\r\n"              // marks the end of header block
              "<head><title>Simple File Transfer (Transmit File)"
              "</title></head>\n"
              "<body><h1>%s</h1>\n"
              ,
              pszErrorMsg );


    //
    //  Populate SendHeaderExInfo struct
    //

    SendHeaderExInfo.pszStatus = szStatus;
    SendHeaderExInfo.pszHeader = szHeader;
    SendHeaderExInfo.cchStatus = lstrlen( szStatus);
    SendHeaderExInfo.cchHeader = lstrlen( szHeader);
    SendHeaderExInfo.fKeepConn = FALSE;

    //
    //  Send header - use the EX Version to send the header blob
    //

	if ( !pecb->ServerSupportFunction(
    	            pecb->ConnID,
    	            HSE_REQ_SEND_RESPONSE_HEADER_EX,
    	            &SendHeaderExInfo,
    	            NULL,
    	            NULL
            ) ) {

		error = GetLastError();
        wsprintfA( pchBuffer, "WebMailbox-Error HSE_REQ_SEND_RESPONSE_HEADER_EX failed (%d)", error);

	    OutputDebugString( pchBuffer);
		OutputDebugString( "\n");
		pecb->ServerSupportFunction(pecb->ConnID, HSE_APPEND_LOG_PARAMETER, pchBuffer, 0, 0);
    	return HSE_STATUS_ERROR;
	}
    
    return ( HSE_STATUS_SUCCESS);
} // SendHeaderToClient()
    


DWORD
SendFileToClient( 
    IN EXTENSION_CONTROL_BLOCK * pecb, 
    IN LPCSTR pszFile
    )
{
    CHAR    pchBuffer[1024];
    HANDLE  hFile;
    DWORD   hseStatus = HSE_STATUS_SUCCESS;
	DWORD	error;

    wsprintfA( pchBuffer, "\"%s\" ", pszFile);
    OutputDebugString( pchBuffer);
    OutputDebugString( "\n");
	pecb->ServerSupportFunction(pecb->ConnID, HSE_APPEND_LOG_PARAMETER, pchBuffer, 0, 0);

    hFile = FcOpenFile( pecb, pszFile);

    if ( hFile == INVALID_HANDLE_VALUE) {

		error = GetLastError();
        wsprintfA( pchBuffer, "WebMailbox-Error Open Failed (%d)", error);

	    OutputDebugString( pchBuffer);
		OutputDebugString( "\n");
		pecb->ServerSupportFunction(pecb->ConnID, HSE_APPEND_LOG_PARAMETER, pchBuffer, 0, 0);
        hseStatus = SendHeaderToClient( pecb, pchBuffer);
		sendQueueMessage(pecb, pszFile, error);

    } else {

#if SEPARATE_HEADERS
        hseStatus = SendHeaderToClient( pecb, "File Transfer begins");
#else 
        hseStatus = HSE_STATUS_SUCCESS;
#endif

        if ( hseStatus == HSE_STATUS_SUCCESS) {

            hseStatus = SendFileOver( pecb, hFile, pszFile);
            
            if ( hseStatus != HSE_STATUS_PENDING) {

                //
                // assume that the data is transmitted.
                //
                
                if ( hseStatus != HSE_STATUS_SUCCESS) {
                    
                    //
                    // Error in transmitting the file. Send error message.
                    //
                    
					error = GetLastError();
                    wsprintfA( pchBuffer, "WebMailbox-Error Send Failed (%d)", error);
                    
				    OutputDebugString( pchBuffer);
					OutputDebugString( "\n");
					pecb->ServerSupportFunction(pecb->ConnID, HSE_APPEND_LOG_PARAMETER, pchBuffer, 0, 0);
                    SendHeaderToClient( pecb, pchBuffer);
					sendQueueMessage(pecb, pszFile, error);
                }
            }
        }

        if ( hseStatus != HSE_STATUS_PENDING) {
              
            //
            // file handle is closed for all non-pending cases
            // if the status is pending, file handle is cleaned up in callback
            //
            FcCloseFile(hFile);
        }
    }
    
    return (hseStatus);

} // SendFileToClient()




# define MAX_BUFFER_SIZE  (400)

typedef struct _AIO_WORK_ITEM {
    
    LIST_ENTRY    listEntry;
    EXTENSION_CONTROL_BLOCK * pecb;
    HSE_TF_INFO   hseTf;
    CHAR          pchBuffer[ MAX_BUFFER_SIZE ];

}  AIO_WORK_ITEM, * PAWI;




VOID
CleanupAW( IN PAWI paw, IN BOOL fDoneWithSession)
{

    DWORD err = GetLastError();

    if ( paw->hseTf.hFile != INVALID_HANDLE_VALUE) { 
     
        FcCloseFile(paw->hseTf.hFile);
    }
	else {
		OutputDebugString( "Invalid Handle\n");
	}
    
    if (fDoneWithSession) {

        paw->pecb->ServerSupportFunction( paw->pecb->ConnID,
                                         HSE_REQ_DONE_WITH_SESSION,
                                         NULL, NULL, NULL);
    }
    SetLastError( err);

    //
    // Remove from work items list
    // 
#if USE_WORK_QUEUE
    EnterCriticalSection( &g_csWorkItems);
    RemoveEntryList( &paw->listEntry);
    LeaveCriticalSection( &g_csWorkItems);
# endif 

    LocalFree( paw);
    return;

} // CleanupAW()




VOID WINAPI
HseIoCompletion(
                IN EXTENSION_CONTROL_BLOCK * pECB, 
                IN PVOID    pContext,
                IN DWORD    cbIO,
                IN DWORD    dwError
                )
/*++

 Routine Description:

   This is the callback function for handling completions of asynchronous IO.
   This function performs necessary cleanup and resubmits additional IO
    (if required). In this case, this function is called at the end of a 
    successful TransmitFile() operation. This function primarily cleans up
    the data and worker queue item and exits.

 Arguments:

   pecb          pointer to ECB containing parameters related to the request.
   pContext      context information supplied with the asynchronous IO call.
   cbIO          count of bytes of IO in the last call.
   dwError       Error if any, for the last IO operation.

 Return Value:

   None.
--*/
{
    PAWI    paw = (PAWI ) pContext;
    EXTENSION_CONTROL_BLOCK   * pecb = paw->pecb;

    CHAR    pchBuffer[1024];

    // assert( pecb == paw->pecb);

    //
    // 1. if no errors, we are done transmitting the file
    // 2. cleanup and exit
    //

    
	if (dwError)
	{
		wsprintfA( pchBuffer, "WebMailbox-Error HseIoCompletion (%d)", dwError);

	    OutputDebugString( pchBuffer);
		OutputDebugString( "\n");
		pecb->ServerSupportFunction(pecb->ConnID, HSE_APPEND_LOG_PARAMETER, pchBuffer, 0, 0);
	}
	else
	{
	    OutputDebugString( "WebMailbox-Completed Successfully\n");
		pecb->ServerSupportFunction(pecb->ConnID, HSE_APPEND_LOG_PARAMETER, "WebMailbox-Completed Successfully", 0, 0);
	}

    //
    // Do Cleanup
    //
    
    CleanupAW( paw, TRUE);

	sendQueueMessage(pecb, pecb->lpszPathTranslated, dwError);

    return;

} // HseIoCompletion()




DWORD
SendFileOver( IN EXTENSION_CONTROL_BLOCK  * pecb,
              IN HANDLE hFile, 
              IN LPCSTR pszFile)
{

    PAWI   paw;
    DWORD  hseStatus = HSE_STATUS_PENDING;
	CHAR    pchBuffer[1024];

    paw  = (PAWI ) LocalAlloc( LMEM_FIXED, sizeof(AIO_WORK_ITEM));
    if ( paw == NULL) {

	    OutputDebugString( "WebMailbox-Error Not enough memory\n");
		pecb->ServerSupportFunction(pecb->ConnID, HSE_APPEND_LOG_PARAMETER, "WebMailbox-Error Not enough memory", 0, 0);
        SetLastError( ERROR_NOT_ENOUGH_MEMORY);
        return (HSE_STATUS_ERROR);
    }

    //
    // Fill in all the data in AIO_WORK_ITEM
    //
    paw->pecb = pecb;
    InitializeListHead( &paw->listEntry);

    paw->hseTf.pfnHseIO = HseIoCompletion;
    paw->hseTf.pContext = paw;
//    paw->hseTf.dwFlags  = (HSE_IO_ASYNC | HSE_IO_DISCONNECT_AFTER_SEND | HSE_IO_SEND_HEADERS );
    paw->hseTf.dwFlags  = (HSE_IO_ASYNC | HSE_IO_DISCONNECT_AFTER_SEND);

    paw->hseTf.hFile    = hFile;
    paw->hseTf.BytesToWrite = GetFileSize(hFile, NULL);
    paw->hseTf.Offset   = 0;
    paw->hseTf.pTail    = NULL;
    paw->hseTf.TailLength = 0;

    //
    //  Set up the header to be sentout for the file
    //
    
#if SEPARATE_HEADERS
    paw->hseTf.HeadLength = 0;
    paw->hseTf.pHead    = NULL;

#else 
    paw->hseTf.HeadLength = 
      wsprintfA ( paw->pchBuffer,
                 "HTTP/1.1 200 OK\r\n"
//                 "Content-Type: text/html\r\n"
//                 "\r\n"
//                 "<head><title>Simple File Transfer (TransmitFile) "
//                 "</title></head>\n"
//                 "Content-Type: X-WebMailbox/X-Download\r\n"
//                 "Content-Type: application/X-WebMailbox\r\n"
                 "Content-Type: application/octet-stream\r\n"
				 "Content-Length: %lu\r\n"
//				 "Content-Disposition: attachment; filename=%s\r\n"
                 "\r\n", paw->hseTf.BytesToWrite);

	wsprintfA( pchBuffer, "(%d bytes) ", paw->hseTf.BytesToWrite);

    OutputDebugString( pchBuffer);
    OutputDebugString( "\n");
	pecb->ServerSupportFunction(pecb->ConnID, HSE_APPEND_LOG_PARAMETER, pchBuffer, 0, 0);

/*
00000030                    48 54 54 50 2F 31 2E 31 20 32       HTTP/1.1.2
00000040  30 30 20 4F 4B 0D 0A 53 65 72 76 65 72 3A 20 4D 00.OK..Server:.M
00000050  69 63 72 6F 73 6F 66 74 2D 49 49 53 2F 35 2E 30 icrosoft-IIS/5.0
00000060  0D 0A 43 6F 6E 6E 65 63 74 69 6F 6E 3A 20 6B 65 ..Connection:.ke
00000070  65 70 2D 61 6C 69 76 65 0D 0A 44 61 74 65 3A 20 ep-alive..Date:.
00000080  46 72 69 2C 20 32 38 20 41 70 72 20 32 30 30 30 Fri,.28.Apr.2000
00000090  20 31 34 3A 35 34 3A 32 36 20 47 4D 54 0D 0A 43 .14:54:26.GMT..C
000000A0  6F 6E 74 65 6E 74 2D 54 79 70 65 3A 20 61 70 70 ontent-Type:.app
000000B0  6C 69 63 61 74 69 6F 6E 2F 6F 63 74 65 74 2D 73 lication/octet-s
000000C0  74 72 65 61 6D 0D 0A 41 63 63 65 70 74 2D 52 61 tream..Accept-Ra
000000D0  6E 67 65 73 3A 20 62 79 74 65 73 0D 0A 4C 61 73 nges:.bytes..Las
000000E0  74 2D 4D 6F 64 69 66 69 65 64 3A 20 53 61 74 2C t-Modified:.Sat,
000000F0  20 32 33 20 4F 63 74 20 31 39 39 39 20 31 38 3A .23.Oct.1999.18:
00000100  33 30 3A 34 39 20 47 4D 54 0D 0A 45 54 61 67 3A 30:49.GMT..ETag:
00000110  20 22 30 39 30 39 64 62 61 38 34 31 64 62 66 31 ."0909dba841dbf1
00000120  3A 39 39 65 22 0D 0A 43 6F 6E 74 65 6E 74 2D 4C :99e"..Content-L
00000130  65 6E 67 74 68 3A 20 38 31 39 35 39 0D 0A 0D 0A ength:.81959....
00000140  4D 5A 90 00 03 00 00 00 04 00 00 00 FF FF 00 00 MZ..............

*/
//                 "\r\n"
//                 "Content-Type: text/html\r\n"
//                 "\r\n"
//                 "<head><title>Simple File Transfer (TransmitFile) "
//                 "</title></head>\n"
//                 "<h1> Transferred file contains...</h1>\n");
    paw->hseTf.pHead = paw->pchBuffer;
# endif 
    
    // Add to the list
#if USE_WORK_QUEUE
    EnterCriticalSection( &g_csWorkItems);
    InsertTailList( &g_lWorkItems,  &paw->listEntry);
    LeaveCriticalSection( &g_csWorkItems);
#endif 

    //
    // Setup the Async TransmitFile
    //

    if ( !pecb->ServerSupportFunction( pecb->ConnID,
                                       HSE_REQ_TRANSMIT_FILE,
                                       &paw->hseTf,
                                       NULL,
                                       NULL)
        ) {

        //
        // Do cleanup and return error
        //

        // File handle will be freed by the caller for errors
        paw->hseTf.hFile = INVALID_HANDLE_VALUE;

	    OutputDebugString( "WebMailbox-Error HSE_REQ_TRANSMIT_FILE failed\n");
		pecb->ServerSupportFunction(pecb->ConnID, HSE_APPEND_LOG_PARAMETER, "WebMailbox-Error HSE_REQ_TRANSMIT_FILE failed", 0, 0);
        CleanupAW( paw, FALSE);
        hseStatus =  HSE_STATUS_ERROR;
    }
	else {
		pecb->ServerSupportFunction(pecb->ConnID, HSE_REQ_IO_COMPLETION, HseIoCompletion, 0, (unsigned long *)paw);
	}

    return (hseStatus);

} // SendFileOver()


VOID
sendQueueMessage(IN EXTENSION_CONTROL_BLOCK * pECB, 
				 IN LPCSTR pszFile,
				 DWORD error)
{
	// This routine builds the AFD response structure and puts it in the AFD server queue
	/* Here the first parameter indicates the user erc.
	   Second parameter indicates whether the response contains a 
	   mail message.  */

#define  AFDSRVRTYPE   170
#define   erc_OK  0

	WMAILRESPQENTRY  afd_rsp_entry;
	SYSTEMTIME   systime;
	char *  jobFileName;
	char jobNumber[20];
	DWORD erc;
	CHAR    pchBuffer[1024];
	DWORD buffLen;

	memset(&afd_rsp_entry, 0, sizeof(afd_rsp_entry));

    afd_rsp_entry.messageType = WMAILRESPQENTRYID;
	strcpy(afd_rsp_entry.destinationFile, pszFile);
	afd_rsp_entry.error1 = error;
	afd_rsp_entry.error2 = error;
	jobFileName = strrchr(pszFile, 'B') + 1;
	strcpy(jobNumber, jobFileName);
	jobNumber[strlen(jobNumber)-4] = 0;
	afd_rsp_entry.WmailJobNum = atoi(jobNumber);
	strcpy(afd_rsp_entry.buffer, "AUTH_USER - ");
	buffLen = sizeof(afd_rsp_entry.buffer) - strlen(afd_rsp_entry.buffer);
	pECB->GetServerVariable(pECB->ConnID, "AUTH_USER", &afd_rsp_entry.buffer[strlen(afd_rsp_entry.buffer)], &buffLen);
	strcat(afd_rsp_entry.buffer, ", LOGON_USER - ");
	buffLen = sizeof(afd_rsp_entry.buffer) - strlen(afd_rsp_entry.buffer);
	pECB->GetServerVariable(pECB->ConnID, "LOGON_USER", &afd_rsp_entry.buffer[strlen(afd_rsp_entry.buffer)], &buffLen);
	strcat(afd_rsp_entry.buffer, ", REMOTE_ADDR - ");
	buffLen = sizeof(afd_rsp_entry.buffer) - strlen(afd_rsp_entry.buffer);
	pECB->GetServerVariable(pECB->ConnID, "REMOTE_ADDR", &afd_rsp_entry.buffer[strlen(afd_rsp_entry.buffer)], &buffLen);
	strcat(afd_rsp_entry.buffer, ", REMOTE_HOST - ");
	buffLen = sizeof(afd_rsp_entry.buffer) - strlen(afd_rsp_entry.buffer);
	pECB->GetServerVariable(pECB->ConnID, "REMOTE_HOST", &afd_rsp_entry.buffer[strlen(afd_rsp_entry.buffer)], &buffLen);
	strcat(afd_rsp_entry.buffer, ", REMOTE_USER - ");
	buffLen = sizeof(afd_rsp_entry.buffer) - strlen(afd_rsp_entry.buffer);
	pECB->GetServerVariable(pECB->ConnID, "REMOTE_USER", &afd_rsp_entry.buffer[strlen(afd_rsp_entry.buffer)], &buffLen);

//	OutputDebugString( afd_rsp_entry.destinationFile);
//	OutputDebugString( "\n");
//	OutputDebugString( jobNumber);
//	OutputDebugString( "\n");
    OutputDebugString( afd_rsp_entry.buffer);
	OutputDebugString( "\n");

	GetLocalTime(&systime);

	erc = AddQueueEntry(AFDquename, (WORD)strlen(AFDquename), TRUE, 8,AFDSRVRTYPE, &afd_rsp_entry, sizeof(afd_rsp_entry), &systime,0);
    if(erc != erc_OK)
	{
		wsprintfA( pchBuffer, " WebMailbox-Error responding to AFD (%d)", erc);

	    OutputDebugString( pchBuffer);
		OutputDebugString( "\n");
		pECB->ServerSupportFunction(pECB->ConnID, HSE_APPEND_LOG_PARAMETER, pchBuffer, 0, 0);
		if (error == 0) {
			OutputDebugString( " Deleting file\n");
			pECB->ServerSupportFunction(pECB->ConnID, HSE_APPEND_LOG_PARAMETER, "Deleting file", 0, 0);
			if (DeleteFile(pszFile) == 0) {
				erc = GetLastError();
				wsprintfA( pchBuffer, " WebMailbox-Error deleting file (%d)", erc);

				OutputDebugString( pchBuffer);
				OutputDebugString( "\n");
				pECB->ServerSupportFunction(pECB->ConnID, HSE_APPEND_LOG_PARAMETER, pchBuffer, 0, 0);
			}
		}
	}
	return;
}

VOID
InitAfdInfo()
{

char  QueueSect[34] = "Queues";
char  token[15] = "WebMailbox";
DWORD cTokenValue=0;
DWORD cbRet;
		
	if (!InitConfigFile(NULL)) {
		OutputDebugString( "Error opening AfdConfig.ini\n");
		OutputDebugString( "Unable to get Afd Queue Name...Using \"AfdWebMailbox\"\n");
		strcpy(AFDquename, "AfdWebMailbox");
	}
	else
	{
		//  Query the WebMailbox Queuename from the AFDConfig.ini file
		if(!GetAfdConfigParam((BYTE *)QueueSect,(BYTE *) token,(BYTE *)AFDquename, 34, &cTokenValue, TRUE))
		{
			//Log Error
			OutputDebugString( "Unable to get Afd Queue Name...Using \"AfdWebMailbox\"\n");
			strcpy(AFDquename, "AfdWebMailbox");
		}
	}
	if (!CloseConfigFile())
	{
		OutputDebugString( "Error closing AfdConfig.ini\n");
	}

	// Query the System Root
	if (!GetAfdRoot(AFDworkdir, sizeof(AFDworkdir), &cbRet)) {
		OutputDebugString( "Unable to get Afd Root...Using \"C:\\Afd\\AfdWork\"\n");
		strcpy(AFDworkdir, "C:\\Afd\\AfdWork");
	}
	else
	{
		strcat(AFDworkdir,"\\AfdWork");
	}

	OutputDebugString( "Afd Queue Name \"");
	OutputDebugString(AFDquename);
	OutputDebugString( "\"\nAfd Work Dir \"");
	OutputDebugString(AFDworkdir);
	OutputDebugString( "\"\n");

	return;
}