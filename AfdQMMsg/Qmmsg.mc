;// Copyright Momentum Systems, NJ. March 1998.
;// File QMMsg.mc
;// This is the message file for the Afd Windows NT Queue Manager.
;// The message compiler uses this file to create QMMsg.h and QMMsg.rc.
;// To execute the message compiler, type the following command
;// to the DOS command prompt:

;//		mc -v QMMsg.mc 

;// The Resource Compiler creates QMMsg.res from QMMsg.rc.
;// The Linker includes QMMsg.res in the .exe.


SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )
;


;//*****************************************************
;//**   ERRORS SECTION
;//*****************************************************

MessageId=0000
Severity=Error
SymbolicName=MSG_STATUS_SEND_FAILED
Language=English
StatusSend failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_UNEXPECTED_HANDLER_CASE
Language=English
Unexpected Handler, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_REGISTER_SERVICE_FAILED
Language=English
Register service failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_CREATE_EVENT_FAILED
Language=English
Create event failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_INIT_SERVICE_FAILED
Language=English
Init service failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_REGISTER_SERVICE_HANDLER_FAILED
Language=English
Register service failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_ADDRESS_NO_POOL
Language=English
Could not allocate address: no pool
.

MessageId=
Severity=Error
SymbolicName=MSG_ALLOC_NO_POOL
Language=English
Could not allocate object: pool exhausted
.

MessageId=
Severity=Error
SymbolicName=MSG_CREATESEM_FAILED
Language=English
CreateSemaphore of RcbSemaphore failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_RELEASESEM_FAILED
Language=English
ReleaseSemaphore of RcbSemaphore failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_WAIT_RCB
Language=English
Wait for Rcb Semaphore failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_REG_ENUM_VALUE
Language=English
RegEnumValue failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_REG_QUERY_INFO
Language=English
RegQueryInfoKey failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_REG_ENUMKEYEX
Language=English
RegEnumKeyEx failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_REG_CLOSE_KEY
Language=English
RegCloseKey failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_CLOSE_HANDLE
Language=English
CloseHandle failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_REG_CLOSE_HANDLE
Language=English
RegCloseKey failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_FIND_TAPES
Language=English
FindTapesInRegistry failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_CREATE_EVENT
Language=English
Create Event failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_CREATE_THREAD
Language=English
Create Thread failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_WAIT_OBJECT
Language=English
WAIT_OBJECT inconsistency, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_CONTROL_FAIL
Language=English
Control failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_CREATE_FILE
Language=English
CreatFile failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_READ_FAIL
Language=English
Read failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_RECOVER_BUFFER
Language=English
RecoverBufferData failed to allocate residual buffer, size = %1
.

MessageId=
Severity=Error
SymbolicName=MSG_TAPE_WRITE
Language=English
Write failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_WRITE_RESIDUAL
Language=English
Write failed to allocate residual buffer, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_RESPOND
Language=English
In RespondToRq Respond failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_ALLOC_EXCH
Language=English
AllocExch failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_SERVE_RQ
Language=English
ServeRq failed, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_WAIT
Language=English
Wait failed in main loop, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_EXCEPTION
Language=English
Exception encountered, %1
.

MessageId=
Severity=Error
SymbolicName=MSG_QM_ALREADY_INSTALLED
Language=English
An instance of the AFD QueueManager is already installed
.


;//*****************************************************
;//**   WARNINGS SECTION
;//*****************************************************



;//*****************************************************
;//**   INFO MESSAGES SECTION
;//*****************************************************

MessageId=
Severity=Informational
SymbolicName=MSG_SERVICESTARTED
Language=English
AFD QueueManager started.
.

MessageId=
Severity=Informational
SymbolicName=MSG_QMSERVICESTARTED
Language=English
AFD QueueManager started.
.

MessageId=
Severity=Informational
SymbolicName=MSG_ERRQMINIT
Language=English
Error in initializing the QueueManager.
.

MessageId=
Severity=Informational
SymbolicName=MSG_CANNOT_READ_STD_QUEUE_INDEX
Language=English
Error in initializing the QueueManager. Could not read the AFDROOT\Config\Queue.Index file.
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUEUE_INDEX
Language=English
Error in initializing the QueueManager. Could not read the Queue.Index file.
.

MessageId=
Severity=Informational
SymbolicName=MSG_QUEUE_FILE
Language=English
Error in initializing the QueueManager. Could not create %1 Queue File.
.
