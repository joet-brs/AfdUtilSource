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
    '    Date         :  March 1998
    '
    '    File Name    :  AfdSCM.c
    '
    '    Purpose      :  This file has the code for handling the beauracracy involved
    '                    in being a Windows NT System Service.
    '
    '    Author       :  AA
    '******************************************************************************/
    /*Modification History
    */
    /*///////////////////////////////////////////////////////////////////////////
    Module Name: qmSCMInterface.cpp
    
    This module implements the protocol to interacet with the NT
    Service Control Manager to start the NT Queue Manager service as a Win32 service.
    
    Revision History:
    
    21/July/1997 AA MomentumSystems AFD product.
    ///////////////////////////////////////////////////////////////////////////*/
    
    
    #include "basic.h"
    #include <stdio.h>
    #include <conio.h>
    #include <string.h>
    #include <winsvc.h>
    #include "AfdLog_Debug.h"
    #include "AfdErr.h"
    
    static char *ModId[]={__FILE__};
    
    //externs
    extern DWORD APIENTRY InitService(int argc, char **argv);
    extern DWORD APIENTRY SuspendService(void);
    extern DWORD APIENTRY ResumeService(void);
    extern void APIENTRY TerminateService (void);
    
    /*
    MSG_STATUS_SEND_FAILED
    MSG_UNEXPECTED_HANDLER_CASE
    MSG_REGISTER_SERVICE_HANDLER_FAILED
    MSG_CREATE_EVENT_FAILED
    MSG_INIT_SERVICE_FAILED
    MSG_SERVICESTARTED
    MSG_EXCEPTION
    */
    
 /*
    extern DWORD msg_status_send_failed;
    extern DWORD msg_unexpected_handler_case;
    extern DWORD msg_register_service_handler_failed;
    extern DWORD msg_create_event_failed;
    extern DWORD msg_init_service_failed;
    extern DWORD msg_servicestarted;
    extern DWORD msg_exception;
 */
    extern BOOLEAN fDebugMode; //for Debugging
    extern char *pServiceName;
    
    
    
    #define lCurrentState 0xffffffff
    DWORD dwServiceState;
    DWORD dwCheckPoint = {0};
    SERVICE_STATUS_HANDLE hService = {0}; // Handle returned by 
    	// RegisterServiceCtrlHandler to identify the service in subsequent calls
    	// to the SetServiceStatus function. 
    HANDLE hEventTerminate = {0};
    BOOLEAN fFalse = {0};
    BOOLEAN fRunAsApp = {0};
    BOOLEAN fServiceSuspended=0;
    
    
    // prototypes
    DWORD StatusSend (DWORD dwState, DWORD dwError);
	BOOL WINAPI ControlHandler ( DWORD dwCtrlType );
    
    //FOR DEBUG
    void LogArguments(char *strFile, DWORD argc, LPSTR* argv) {
    
    	HANDLE hFile=INVALID_HANDLE_VALUE;
    
    	char fileBuff[512];
    	DWORD cbRet, i;
    
    	hFile = CreateFile((const char *) strFile,
    						GENERIC_READ | GENERIC_WRITE, 0,	// share mode 
    						0, // pointer to security descriptor 
    						CREATE_ALWAYS,	// how to create 
    						FILE_ATTRIBUTE_NORMAL,	// file attributes 
    						0);
    
    	if (hFile == INVALID_HANDLE_VALUE) {
    		goto ContinueMain;
    	}
    
    	if (argc) {
    		sprintf(fileBuff, "argc = %d\n", argc);
    		WriteFile(	hFile,				// handle to file to write to 
    					fileBuff,			// pointer to data to write to file 
    					strlen(fileBuff),	// number of bytes to write 
    					&cbRet,				// pointer to number of bytes written 
    					NULL				// pointer to structure needed for overlapped I/O
    				 );
    		for(i=0;(i<argc);i++) {
				if (sizeof(fileBuff) <= _snprintf(fileBuff, sizeof(fileBuff), "Parameter 1 (argv[%d]) is : %s\n", i,
    					(argv[i] ? argv[i]: NULL))) sprintf(fileBuff, "Parameter 1 (argv[%d]) is : ***TOO LARGE TO PRINT***\n", i);
    			WriteFile(	hFile,				// handle to file to write to 
    						fileBuff,			// pointer to data to write to file 
    						strlen(fileBuff),	// number of bytes to write 
    						&cbRet,				// pointer to number of bytes written 
    						NULL				// pointer to structure needed for overlapped I/O
    					  );
    		}
    	}
    	else {
    		sprintf(fileBuff, "%s", "No parameters");
    		WriteFile(	hFile,				// handle to file to write to 
    					fileBuff,			// pointer to data to write to file 
    					strlen(fileBuff),	// number of bytes to write 
    					&cbRet,				// pointer to number of bytes written 
    					NULL				// pointer to structure needed for overlapped I/O
    				 );
    	}
    
    ContinueMain:
    	if (hFile != INVALID_HANDLE_VALUE)
    		CloseHandle(hFile);
    }
    
    //END FOR DEBUG
    
    /*
    ServiceTerminate()
    Clean up any open handles and send a status message to the SCM to tell
    it that the service is stopped.
    */
    
    void ServiceTerminate (DWORD dwError) {
    	// SERVICE_STOP_PENDING should be reported before 
        // setting the Stop Event - hServerStopEvent - in 
        // ServiceStop().  This avoids a race condition 
        // which may result in a 1053 - The Service did not respond... 
        // error. 
		if (hService)
			StatusSend(SERVICE_STOP_PENDING, dwError); 
        
		TerminateService(); // unserve all requests
    	if (hService)
    		StatusSend(SERVICE_STOPPED, dwError);
    	if (hEventTerminate)
    		CloseHandle(hEventTerminate); // signals the event too
    } // ServiceTerminate
    
    
    
    /*
    StatusSend()
    Send a status message to the NT service control manager.
    */
    
    DWORD StatusSend (DWORD dwState, DWORD dwError) {
    	
    	SERVICE_STATUS serviceStatus;
    
    	// dwServiceState is global; set here unless lCurrentState is
    	// argued which means send the current state.
    	if (dwState == lCurrentState)
    		dwState = dwServiceState;
    	else
    		dwServiceState = dwState;
    	serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    	serviceStatus.dwCurrentState = dwState;
    	serviceStatus.dwControlsAccepted =  SERVICE_ACCEPT_STOP |
    										SERVICE_ACCEPT_PAUSE_CONTINUE |
    										SERVICE_ACCEPT_SHUTDOWN;
    	if (!dwError) {
    		serviceStatus.dwWin32ExitCode = NO_ERROR;
    		serviceStatus.dwServiceSpecificExitCode = 0;
    	} else {
    		serviceStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
    		serviceStatus.dwServiceSpecificExitCode = dwError;
    	}
    	serviceStatus.dwCheckPoint = dwCheckPoint++;
 	 	serviceStatus.dwWaitHint = 5000;	// LSL 990126 Increase wait hint time from 1000
 
    
    	if (!SetServiceStatus(hService, &serviceStatus)) {
    		DWORD dwError = GetLastError();
 	 		DebugPrintStatus(MSG_STATUS_SEND_FAILED, dwError);
    		return dwError;
    	}
    	return 0;
    }
    
    
    /*
    ServiceHandler()
    The SCM calls the ServiceHandler when it wants to pause, resume, 
    interrogate, or stop the service.
    */
    
    void ServiceHandler(DWORD dwControl) {
    	
    	switch(dwControl) {
    
    	case SERVICE_CONTROL_STOP:
    		StatusSend(SERVICE_STOP_PENDING, 0);
    			// Cause ServiceMain, which is executing as a separate thread,
    			// to terminate and return. TO_DO need some re-sync here
    			SetEvent(hEventTerminate);
    			break;
    
    	case SERVICE_CONTROL_PAUSE:
    		StatusSend(SERVICE_PAUSE_PENDING, 0);
    		if (!fServiceSuspended)
    			SuspendService(); //TO_DO check the returned status here
    		fServiceSuspended = TRUE;
    		StatusSend(SERVICE_PAUSED, 0);
    		break;
    
    	case SERVICE_CONTROL_CONTINUE:
    		StatusSend(SERVICE_CONTINUE_PENDING, 0);
    		if (fServiceSuspended)
    				ResumeService(); //TO_DO check the returned status here
    		fServiceSuspended = FALSE;
    		StatusSend(SERVICE_RUNNING, 0);
    		break;
    
    	case SERVICE_CONTROL_INTERROGATE:
    		StatusSend(lCurrentState, 0);
    		break;
    
    	case SERVICE_CONTROL_SHUTDOWN:
    		//TO_DO do the close_service here
    		break;
    	default:
  		DebugPrintFormatMsg(MSG_UNEXPECTED_HANDLER_CASE, "Case = %d", dwControl);
    	}
    
    } // ServiceHandler
    
    
    /*
    ServiceMain()
    This is the function the NT service control manager will call when it starts 
    the service.  This function starts the thread that does the actual work of the 
    service.
    */
    void ServiceMain(DWORD argc, LPSTR* argv)
    /*argc specifies the number of arguments in the argv array.
      argv[] Points to an array of pointers that point to null-terminated 
      argument strings. The first argument in the array is the name of the service, 
      and subsequent arguments are any strings passed to the service by the process 
      that called the StartService function to start the service. 
      */
    {
    	ercType erc;
    	//static char strFile[]={"C:\\ServiceMainParams.txt"};
    
    	if (argc == 0)
    		return;
    	pServiceName = (char *) argv[0];
    	if (!pServiceName)
    		return;
    
    	//Debugging
    	//LogArguments(strFile, argc, argv);
    
    	/*
    	The RegisterServiceCtrlHandler function registers a function to handle
    	service control requests for a service. This enables the control dispatcher 
    	to invoke the specified function when it receives control requests for 
    	this service.
    	*/
    	hService = RegisterServiceCtrlHandler(pServiceName, 
    										  /*Points to a null-terminated string 
    										  that names the service run by the calling
    										  thread. This is the service name that was
    										  specified in the CreateService function 
    										  when the service was created*/
    										  (LPHANDLER_FUNCTION)ServiceHandler
    										  /*Points to the Handler function to be 
    										  registered*/
    										 );
    	if (!hService) {
  		DebugPrintStatus(MSG_REGISTER_SERVICE_HANDLER_FAILED, GetLastError());
    		return;
    	}
    
    	// Notify the SCM of progress.
     	if (StatusSend(SERVICE_START_PENDING, 0)) // superfluous?
     		return;
    
    	/*
    	Create an event that will be used to prevent us from returning until the 
    	SCM issues a STOP request.
    	*/
    	
    	hEventTerminate = CreateEvent(0,		//pSecurityAttrs
    								  TRUE,		//fManualReset
    								  FALSE,	//fInitialState
    								  0			//pEventName
    								 );
    	if (!hEventTerminate) {
  		DebugPrintStatus(MSG_CREATE_EVENT_FAILED, GetLastError());
    		return;
    	};
    
    	// Start the thread that does the actual work in TapeService.cpp.
    	erc = InitService(argc, argv);
    	if (erc) {
 	 		DebugPrintStatus(MSG_INIT_SERVICE_FAILED, erc); 
    		StatusSend(SERVICE_STOPPED, erc);
    		return;
    	}

		DebugPrintMsg(MSG_SERVICESTARTED);
    	// Let the SCM know the service is running.
     	if (StatusSend(SERVICE_RUNNING, 0))
     		return;
    
    	// Wait for the terminateEvent event object to be set.
     	WaitForSingleObject(hEventTerminate, INFINITE);
     	ServiceTerminate(0);
    } // ServiceMain
    
    //////////////////////////////////////////////////////////////////////////
    // Function : TerminateServiceInternal
    // Comments : Added by Supriya. This function when called by any service,
    //			  will call the SCM to Terminate itself.
    //////////////////////////////////////////////////////////////////////////
    BOOL TerminateServiceInternal()
    {
    	BOOL bRet;
    	SERVICE_STATUS stServiceStatus;
    	SC_HANDLE scHandle,scServiceHandle;
    	if(fRunAsApp)
    	{
    		//The application runs in Debug mode.
    		TerminateService();
    		ExitProcess(0);
    		return TRUE;
    	}
    	//Open the service control manager
    	scHandle = OpenSCManager(NULL,NULL,SC_MANAGER_ALL_ACCESS);
    	if(scHandle == NULL)
    		return FALSE;
    	scServiceHandle = OpenService(scHandle,pServiceName,SERVICE_STOP);
    	if(scServiceHandle == NULL)
    	{
    		CloseServiceHandle(scHandle);
    		return FALSE;
    	}
    
    	bRet = ControlService(scServiceHandle, SERVICE_CONTROL_STOP, &stServiceStatus);
    	CloseServiceHandle(scHandle);
    	CloseServiceHandle(scServiceHandle);
    	return bRet;
    }
    
    
    ////////////////////////////////////////////////////////////////////////
    /////  Main    /////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////
    int main (int argc, char **argv) {
    	ercType erc = 1;
    	/*
    	Parameters:
    		0 - Executable Name
    		1 - Debug
    		2 - ServiceName
    	*/
    	char key;
    	#define CANCEL_KEY 0x1b
    
    
    	static char strFile[]={"C:\\MainParams.txt"};
    	//LIMS
    
    	SERVICE_TABLE_ENTRY serviceTable[] = {
    		{pServiceName, (LPSERVICE_MAIN_FUNCTION) ServiceMain },
    		{NULL,         NULL}
    	};
    	
    	LogArguments(strFile, argc, argv);
    
    	if (argc < 3)
    		return(erc);
    	pServiceName = (char *) argv[2];
    	if (!pServiceName)
    		return(erc);
    	//pServiceName has to be set up before calling LogInitialize();
    
    	LogNTInitialize();
    	erc = 1;
    
     	fRunAsApp = TRUE;
    	/*If fRunAsApp is TRUE, this service will run as an application rather than 
    	a service.  This makes it possible to debug from the Visual C++ Debugger
    	*/
    	fRunAsApp = ((argc > 1) ? (argv[1][0] == 'D' || argv[1][0] == 'd') : FALSE);
    	fDebugMode = fRunAsApp;
    	
    	if (fRunAsApp) { // for debugging
    
    		printf("%s in Debug Mode.\n", pServiceName);
    		erc = InitService(argc, argv);
    		if (erc) {
	 			printf("%s failed to init(), erc = %8x\n", pServiceName, erc);
 	 			DebugPrintStatus(MSG_INIT_SERVICE_FAILED, erc); 

    			return(erc);
    			}
			else DebugPrintMsg(MSG_SERVICESTARTED);
    		/*while (1) {Sleep(2000);} //TO_DO just block here instead
    		//TO_DO wait on an event here, and flag this event on completion
    		//from TerminateService(), ask the users to do this.*/
			//erc = SetConsoleCtrlHandler(ControlHandler, TRUE);

			for (;;) {
 	 			printf("\n%s Service is running. Press <ESC> to terminate\n", pServiceName);
	 			//printf("\n%s Service is running. Press <ESC> to Terminate", pServiceName);
    			key = _getch();
    			if (key == CANCEL_KEY) {
 	 				printf("\n%s will be terminated. Are you sure you want this? (y/n)\n",
       					   pServiceName);
    				key = _getche();
    				if (key == 'y' || key == 'Y')
    					break;
    			}
    		}
    		ServiceTerminate(4);
			return 0;

		return 0;
    	} else {
            // start Service Control speak
    		//printf("%s in SCM Mode.\n", pServiceName);
    		/*
    		The StartServiceCtrlDispatcher function connects the main thread of a 
    		service process to the service control manager, which causes the thread 
    		to be the service control dispatcher thread for the calling process.  
    		*/
    
    		serviceTable[0].lpServiceName = pServiceName;
    		
    		if(!StartServiceCtrlDispatcher(serviceTable) 
    									/*Points to an array of SERVICE_TABLE_ENTRY 
    									structures containing one entry for each 
    									service that can execute in the calling process.
    									*/
    		   )
    		   return GetLastError(); // Function failed 
		}
		
		return 0;
    } // main
    
//  FUNCTION: ControlHandler ( DWORD dwCtrlType ) 
//
//  PURPOSE: Handled console control events 
//
//  PARAMETERS: 
//    dwCtrlType - type of control event
//
//  RETURN VALUE: 
//    True - handled 
//    False - unhandled  
//
//  COMMENTS: 
// 
BOOL WINAPI ControlHandler ( DWORD dwCtrlType ) 
	{     
	switch( dwCtrlType ) 
		{         
		case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate 
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode 
            printf("\nStopping %s.\n", pServiceName); 
            ServiceTerminate(4);             
			return TRUE;             
			break;      
		} 
    return FALSE; 
	}     
