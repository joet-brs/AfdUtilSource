MessageIdTypedef=DWORD

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0:FACILITY_SYSTEM
               Runtime=0x2:FACILITY_RUNTIME
               Stubs=0x3:FACILITY_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
              )

LanguageNames=(English=0x409:MSG00409)

MessageId=0x1
Severity=Informational
SymbolicName=NTLOG_INFO_GENERIC
Language=English
%1
.

MessageId=0x1
Severity=Error
SymbolicName=NTLOG_ERROR_GENERIC
Language=English
%1
.

MessageId=0x1
Severity=Warning
SymbolicName=NTLOG_WARNING_GENERIC
Language=English
%1
.

MessageId=2
Severity=Error
Facility=Runtime
SymbolicName=NTLOG_WIN32_ERROR
Language=English
%1
.

;// *********************** From QueueManager.mc

MessageId=3
Severity=Error
SymbolicName=MSG_ADDRESS_NO_POOL
Language=English
Could not allocate address: no pool
.

MessageId=4
Severity=Error
SymbolicName=MSG_ALLOC_NO_POOL
Language=English
Could not allocate object: pool exhausted
.

MessageId=5
Severity=Error
SymbolicName=MSG_CREATESEM_FAILED
Language=English
CreateSemaphore of RcbSemaphore failed, %1
.

MessageId=6
Severity=Error
SymbolicName=MSG_RELEASESEM_FAILED
Language=English
ReleaseSemaphore of RcbSemaphore failed, %1
.

MessageId=7
Severity=Error
SymbolicName=MSG_WAIT_RCB
Language=English
Wait for Rcb Semaphore failed, %1
.

MessageId=8
Severity=Error
SymbolicName=MSG_REG_ENUM_VALUE
Language=English
RegEnumValue failed, %1
.

MessageId=9
Severity=Error
SymbolicName=MSG_REG_QUERY_INFO
Language=English
RegQueryInfoKey failed, %1
.

MessageId=10
Severity=Error
SymbolicName=MSG_REG_ENUMKEYEX
Language=English
RegEnumKeyEx failed, %1
.

MessageId=11
Severity=Error
SymbolicName=MSG_REG_CLOSE_KEY
Language=English
RegCloseKey failed, %1
.

MessageId=12
Severity=Error
SymbolicName=MSG_CLOSE_HANDLE
Language=English
CloseHandle failed, %1
.

MessageId=13
Severity=Error
SymbolicName=MSG_REG_CLOSE_HANDLE
Language=English
RegCloseKey failed, %1
.

MessageId=14
Severity=Error
SymbolicName=MSG_CREATE_EVENT
Language=English
Create Event failed, %1
.

MessageId=15
Severity=Error
SymbolicName=MSG_CREATE_THREAD
Language=English
Create Thread failed, %1
.

MessageId=16
Severity=Error
SymbolicName=MSG_WAIT_OBJECT
Language=English
WAIT_OBJECT inconsistency, %1
.

MessageId=17
Severity=Error
SymbolicName=MSG_CONTROL_FAIL
Language=English
Control failed, %1
.

MessageId=18
Severity=Error
SymbolicName=MSG_CREATE_FILE
Language=English
CreatFile failed, %1
.

MessageId=19
Severity=Error
SymbolicName=MSG_READ_FAIL
Language=English
Read failed, %1
.

MessageId=20
Severity=Error
SymbolicName=MSG_RECOVER_BUFFER
Language=English
RecoverBufferData failed to allocate residual buffer, size = %1
.

MessageId=21
Severity=Error
SymbolicName=MSG_WRITE_FAILED
Language=English
Write failed, %1
.

MessageId=22
Severity=Error
SymbolicName=MSG_WRITE_RESIDUAL
Language=English
Write failed to allocate residual buffer, %1
.

MessageId=23
Severity=Error
SymbolicName=MSG_RESPOND
Language=English
In RespondToRq Respond failed, %1
.

MessageId=24
Severity=Error
SymbolicName=MSG_ALLOC_EXCH
Language=English
AllocExch failed, %1
.

MessageId=25
Severity=Error
SymbolicName=MSG_SERVE_RQ
Language=English
ServeRq failed, %1
.

MessageId=26
Severity=Error
SymbolicName=MSG_WAIT
Language=English
Wait failed in main loop, %1
.

MessageId=27
Severity=Error
SymbolicName=MSG_EXCEPTION
Language=English
Exception encountered, %1
.

MessageId=28
Severity=Error
SymbolicName=MSG_QM_ALREADY_INSTALLED
Language=English
An instance of the AFD QueueManager is already installed
.

;//*****************************************************
;//**   INFO MESSAGES SECTION
;//*****************************************************

MessageId=29
Severity=Informational
SymbolicName=MSG_CANNOT_READ_STD_QUEUE_INDEX
Language=English
Error in initializing the QueueManager. Could not read the AFDROOT\Config\Queue.Index file.
.

MessageId=30
Severity=Informational
SymbolicName=MSG_QUEUE_INDEX
Language=English
Error in initializing the QueueManager. Could not read the Queue.Index file.
.

MessageId=31
Severity=Informational
SymbolicName=MSG_QUEUE_FILE
Language=English
Error in initializing the QueueManager. Could not create %1 Queue File.
.

;// End from QueueManager.mc


;// All Services - Generic Messages
MessageId=32
Severity=Error
SymbolicName=MSG_STATUS_SEND_FAILED
Language=English
StatusSend failed, %1
.

MessageId=33
Severity=Error
SymbolicName=MSG_UNEXPECTED_HANDLER_CASE
Language=English
Unexpected Handler, %1
.

MessageId=34
Severity=Error
SymbolicName=MSG_REGISTER_SERVICE_FAILED
Language=English
Service registration failed, %1
.

MessageId=35
Severity=Error
SymbolicName=MSG_CREATE_EVENT_FAILED
Language=English
Create event failed, %1
.

MessageId=36
Severity=Error
SymbolicName=MSG_INIT_SERVICE_FAILED
Language=English
Service initialization failed, %1
.

MessageId=37
Severity=Error
SymbolicName=MSG_REGISTER_SERVICE_HANDLER_FAILED
Language=English
Register service handler failed, %1
.

MessageId=38
Severity=Success
SymbolicName=MSG_SERVICESTARTED
Language=English
Service started.
.

;//**** from PcCommMsg.mc
MessageId=39
Severity=Error
SymbolicName=MSG_DIRECTORY_NOT_FOUND
Language=English
A required folder (%1) was not found.
.

MessageId= 40
Severity=Error
SymbolicName=MSG_PORT_INIT_ERROR
Language=English
Error opening port
.

MessageId= 41
Severity=Error
SymbolicName=MSG_MODEM_INIT_ERROR
Language=English
Error initializing the modem.
.

MessageId= 42
Severity=Error
SymbolicName=MSG_NO_AFD_ROOT
Language=English
Error: AFD root directory could be located
.

MessageId= 43
Severity=Error
SymbolicName=MSG_REGISTRY_ERROR
Language=English
Error %1 reading %2 from the system registry
.

MessageId= 44
Severity=Error
SymbolicName=MSG_NO_CONFIG_INI
Language=English
Could not find AfdConfig.ini file.
.

MessageId= 45
Severity=Error
SymbolicName=MSG_CONFIG_INI_ERROR
Language=English
Error reading %1 from AfdConfig.ini
.

MessageId= 46
Severity=Error
SymbolicName=MSG_ERR_QUEUE_CONNECT
Language=English
Error %1 connecting to %2 queue.
.

MessageId= 47
Severity=Error
SymbolicName=MSG_FILE_NOT_FOUND
Language=English
A required file (%1) was not found.
.

MessageId= 48
Severity=Error
SymbolicName=MSG_HARDWARE_NOT_FOUND
Language=English
BSC error %1. Port %2 was not found. Hardware is not installed or improperly configured.
.

MessageId= 49
Severity=Error
SymbolicName=MSG_DCP_DRIVER_ERROR
Language=English
Error %1 loading DCP driver.
.

MessageId= 50
Severity=Error
SymbolicName=MSG_BSC_INSTALL_ERROR
Language=English
BSC error %1 initializing hardware.
.

MessageId= 51
Severity=Error
SymbolicName=MSG_ZETAFAX_INIT_FAILED
Language=English
ZetaFax initialization failed. %1.
.


;//******** PcComm 

MessageId=751
Severity=Success
;//SymbolicName=
Language=English
Could not dial out
.

MessageId=752
;//SymbolicName=
Language=English
Entry On Hold
.

MessageId=753
Severity=Informational
;//SymbolicName=
Language=English
Renamed to another profile
.

MessageId=755
Severity=Informational
;//SymbolicName=
Language=English
Received Bad File in Callback
.

MessageId=756
Severity=Informational
;//SymbolicName=
Language=English
Error executing SignOn script
.

MessageId=757
Severity=Informational
;//SymbolicName=
Language=English
Unmarked the entry
.

MessageId=758
Severity=Informational
;//SymbolicName=
Language=English
Error executing SignOff Script
.

MessageId=759
Severity=Informational
;//SymbolicName=
Language=English
Error in File Transmission
.

MessageId=760
Severity=Informational
;//SymbolicName=
Language=English
General error
.

MessageId=761
Severity=Informational
;//SymbolicName=
Language=English
Error Retrieving MailBox Entries
.

MessageId=763
Severity=Informational
;//SymbolicName=
Language=English
Error Renaming the file name after file transmission
.

MessageId=764
Severity=Informational
;//SymbolicName=
Language=English
Error Removing the entry from the Queue
.

MessageId=765
Severity=Informational
;//SymbolicName=
Language=English
Error in retrieving MailBox entries, hence purging the Mailbox entries from Queue
.

;//******** AfdMailbox

MessageId=771
Severity=Informational
;//SymbolicName=
Language=English
No Entries for the Mailbox specified
.

MessageId=772
Severity=Informational
;//SymbolicName=
Language=English
No Record found for Mail Count
.

;//******** AfdAttachJcl

MessageId=801
Severity=Informational
;//SymbolicName=
Language=English
Failed to establish JCL queue
.

MessageId=802
Severity=Informational
;//SymbolicName=
Language=English
Failed to establish AfdServer
.

MessageId=803
Severity=Informational
;//SymbolicName=
Language=English
Failed to rename the temp out file
.

MessageId=804
Severity=Informational
;//SymbolicName=
Language=English
Failed to get current directory	
.

MessageId=805
Severity=Informational
;//SymbolicName=
Language=English
Failed to create a File
.

MessageId=806
Severity=Informational
;//SymbolicName=
Language=English
Failed to read a file
.

MessageId=807
Severity=Informational
;//SymbolicName=
Language=English
Failed to copy a file
.

MessageId=808
Severity=Informational
;//SymbolicName=
Language=English
Failed to terminate JCL queue
.

MessageId=809
Severity=Informational
;//SymbolicName=
Language=English
No JCL file mentioned
.

MessageId=810
Severity=Informational
;//SymbolicName=
Language=English
RemoveMarkedQueueEntry failed
.

;//******** AfdFileManagement

MessageId=821
Severity=Informational
;//SymbolicName=
Language=English
Invalid Operation performed
.

MessageId=822
Severity=Informational
;//SymbolicName=
Language=English
The file doesn't have replace permission
.

MessageId=823
Severity=Informational
;//SymbolicName=
Language=English
File Copy Failed
.

MessageId=824
Severity=Informational
;//SymbolicName=
Language=English
Input file not available
.

MessageId=825
Severity=Informational
;//SymbolicName=
Language=English
Invalid reformat code
.

MessageId=826
Severity=Informational
;//SymbolicName=
Language=English
Read file failed
.

MessageId=827
Severity=Informational
;//SymbolicName=
Language=English
Write file failed
.

MessageId=828
Severity=Informational
;//SymbolicName=
Language=English
Rename File failed
.

MessageId=829
Severity=Informational
;//SymbolicName=
Language=English
Failed to attach date-time stamp
.

MessageId=830
Severity=Informational
;//SymbolicName=
Language=English
Failed to convert ASCII-EBCDIC
.

MessageId=831
Severity=Informational
;//SymbolicName=
Language=English
Failed to reformat
.

MessageId=832
Severity=Informational
;//SymbolicName=
Language=English
Failed to remove special characters
.

MessageId=833
Severity=Informational
;//SymbolicName=
Language=English
Failed to get the current directory
.

MessageId=834
Severity=Informational
;//SymbolicName=
Language=English
Failed to establish the Filemanagement queue
.

MessageId=835
Severity=Informational
;//SymbolicName=
Language=English
RemoveMarkedQueueEntry failed
.

MessageId=836
Severity=Informational
;//SymbolicName=
Language=English
Registry Open failed
.

MessageId=837
Severity=Informational
;//SymbolicName=
Language=English
Failed to get the Trace level	
.

MessageId=838
Severity=Informational
;//SymbolicName=
Language=English
Failed to get the queue name
.

MessageId=839
Severity=Informational
;//SymbolicName=
Language=English
Failed to terminate the Filemanagement queue
.

MessageId=840
Severity=Informational
;//SymbolicName=
Language=English
Cannot create the temporary file
.

;//******** AfdApplication

MessageId=841
Severity=Informational
;//SymbolicName=
Language=English
Failed to establish Application Queue
.

MessageId=842
Severity=Informational
;//SymbolicName=
Language=English
Failed to establish AfdServer
.

MessageId=843
Severity=Informational
;//SymbolicName=
Language=English
Failed to Launch an application
.

MessageId=844
Severity=Informational
;//SymbolicName=
Language=English
Failed to open the registry Key
.

MessageId=845
Severity=Informational
;//SymbolicName=
Language=English
Failed to get the queue name
.

MessageId=846
Severity=Informational
;//SymbolicName=
Language=English
Failed to Open SCM
.

MessageId=847
Severity=Informational
;//SymbolicName=
Language=English
Access Denied
.

MessageId=848
Severity=Informational
;//SymbolicName=
Language=English
Invalid Handle
.

MessageId=849
Severity=Informational
;//SymbolicName=
Language=English
Invalid Service Name
.

MessageId=850
Severity=Informational
;//SymbolicName=
Language=English
Service does not exist
.

MessageId=851
Severity=Informational
;//SymbolicName=
Language=English
Service already running
.

MessageId=852
Severity=Informational
;//SymbolicName=
Language=English
Service failed
.

MessageId=853
Severity=Informational
;//SymbolicName=
Language=English
Failed to get the exit code
.

MessageId=854
Severity=Informational
;//SymbolicName=
Language=English
Failed to remove the queue entry
.

;//******** AfdFax

MessageId=861
Severity=Informational
;//SymbolicName=
Language=English
Failed to read fax Queue
.

MessageId=862
Severity=Informational
;//SymbolicName=
Language=English
No output file
.

MessageId=863
Severity=Informational
;//SymbolicName=
Language=English
No phone number
.

MessageId=864
Severity=Informational
;//SymbolicName=
Language=English
Error - ASCII to FAX
.

MessageId=865
Severity=Informational
;//SymbolicName=
Language=English
Error in sending Fax
.

MessageId=866
Severity=Informational
;//SymbolicName=
Language=English
Error in rewriting queue entry
.

MessageId=867
Severity=Informational
;//SymbolicName=
Language=English
Error unmarking queue entry
.

MessageId=868
Severity=Informational
;//SymbolicName=
Language=English
Error removing queue entry
.

MessageId=869
Severity=Informational
;//SymbolicName=
Language=English
Error getting fax status
.

MessageId=870
Severity=Informational
;//SymbolicName=
Language=English
Error in Fax Job
.

MessageId=871
Severity=Informational
;//SymbolicName=
Language=English
Invalid State
.

MessageId=872
Severity=Informational
;//SymbolicName=
Language=English
Deinstall failed
.

MessageId=873
Severity=Informational
;//SymbolicName=
Language=English
Error getting user directory
.

MessageId=874
Severity=Informational
;//SymbolicName=
Language=English
Error getting Fax status
.

MessageId=875
Severity=Informational
;//SymbolicName=
Language=English
Zetafax Server not running/ Initailized
.

MessageId=876
Severity=Informational
;//SymbolicName=
Language=English
Error in ZfxSubmitFile
.

MessageId=877
Severity=Informational
;//SymbolicName=
Language=English
Error in SendFax
.

;//******** AfdPrint

MessageId=881
Severity=Informational
;//SymbolicName=
Language=English
Error reading print queue
.

MessageId=882
Severity=Informational
;//SymbolicName=
Language=English
Error in printing
.

MessageId=883
Severity=Informational
;//SymbolicName=
Language=English
Error rewriting queue entry
.

MessageId=884
Severity=Informational
;//SymbolicName=
Language=English
Error unmarking queue entry
.

MessageId=885
Severity=Informational
;//SymbolicName=
Language=English
Error in removing queue entry
.

MessageId=886
Severity=Informational
;//SymbolicName=
Language=English
Error in print job
.

MessageId=887
Severity=Informational
;//SymbolicName=
Language=English
Invalid state
.

MessageId=888
Severity=Informational
;//SymbolicName=
Language=English
Error getting print status
.

MessageId=889
Severity=Informational
;//SymbolicName=
Language=English
Error in Creating printer DC
.

MessageId=890
Severity=Informational
;//SymbolicName=
Language=English
Printer not present
.

MessageId=891
Severity=Informational
;//SymbolicName=
Language=English
Error in CreateFile
.

MessageId=892
Severity=Informational
;//SymbolicName=
Language=English
Invalid job number
.

MessageId=893
Severity=Informational
;//SymbolicName=
Language=English
Error in TextOut
.

MessageId=894
Severity=Informational
;//SymbolicName=
Language=English
Error in OpenPrinter to get status
.

MessageId=895
Severity=Informational
;//SymbolicName=
Language=English
Error in RemoveMarkedQueueEntry
.

;//******** AfdQueueManager

MessageId=900
Severity=Informational
;//SymbolicName=
Language=English
Entry not marked
.

MessageId=901
Severity=Informational
;//SymbolicName=
Language=English
Entry marked
.

MessageId=902
Severity=Informational
;//SymbolicName=
Language=English
Entry not found
.

MessageId=903
Severity=Informational
;//SymbolicName=
Language=English
No entry available
.

MessageId=904
Severity=Informational
;//SymbolicName=
Language=English
Entry removed
.

MessageId=905
Severity=Informational
;//SymbolicName=
Language=English
Bad queue spec
.

MessageId=906
Severity=Informational
;//SymbolicName=
Language=English
Too many servers
.

MessageId=907
Severity=Informational
;//SymbolicName=
Language=English
Server not established
.

MessageId=908
Severity=Informational
;//SymbolicName=
Language=English
No established servers
.

MessageId=909
Severity=Informational
;//SymbolicName=
Language=English
Bad Qeh
.

MessageId=910
Severity=Informational
;//SymbolicName=
Language=English
Bad server number
.

MessageId=911
Severity=Informational
;//SymbolicName=
Language=English
Bad queue index file
.

MessageId=912
Severity=Informational
;//SymbolicName=
Language=English
Wrong queue type
.

MessageId=913
Severity=Informational
;//SymbolicName=
Language=English
Bad time
.

MessageId=914
Severity=Informational
;//SymbolicName=
Language=English
Not a unique server
.

MessageId=915
Severity=Informational
;//SymbolicName=
Language=English
Queue table full
.

MessageId=916
Severity=Informational
;//SymbolicName=
Language=English
Bad Qh
.

MessageId=917
Severity=Informational
;//SymbolicName=
Language=English
Queue already removed
.

MessageId=918
Severity=Informational
;//SymbolicName=
Language=English
Duplicate queue name
.

MessageId=919
Severity=Informational
;//SymbolicName=
Language=English
Duplicate dev spec name
.

MessageId=920
Severity=Informational
;//SymbolicName=
Language=English
Bad command path
.

MessageId=921
Severity=Informational
;//SymbolicName=
Language=English
Illegal QM param
.

MessageId=922
Severity=Informational
;//SymbolicName=
Language=English
Illegal deinstall
.

MessageId=923
Severity=Informational
;//SymbolicName=
Language=English
Request error
.

MessageId=924
Severity=Informational
;//SymbolicName=
Language=English
Illegal partition
.

MessageId=925
Severity=Informational
;//SymbolicName=
Language=English
Queuemanager crashed
.

MessageId=926
Severity=Informational
;//SymbolicName=
Language=English
Request queued
.

MessageId=927
Severity=Informational
;//SymbolicName=
Language=English
Duplicate server
.

MessageId=928
Severity=Informational
;//SymbolicName=
Language=English
File overflow
.

MessageId=929
Severity=Informational
;//SymbolicName=
Language=English
Forward request failed
.

MessageId=930
Severity=Informational
;//SymbolicName=
Language=English
Request forwarded
.

MessageId=931
Severity=Informational
;//SymbolicName=
Language=English
Queue entry overflow
.

;//******** AfdBiSync

MessageId=951
Severity=Informational
;//SymbolicName=
Language=English
Error redial
.

MessageId=952
Severity=Informational
;//SymbolicName=
Language=English
Error hold
.

MessageId=953
Severity=Informational
;//SymbolicName=
Language=English
Error rename
.

MessageId=954
Severity=Informational
;//SymbolicName=
Language=English
Err_NOK
.

MessageId=955
Severity=Informational
;//SymbolicName=
Language=English
Error in Unmark
.

MessageId=956
Severity=Informational
;//SymbolicName=
Language=English
Bad file
.

MessageId=957
Severity=Informational
;//SymbolicName=
Language=English
Sign off error
.

MessageId=958
Severity=Informational
;//SymbolicName=
Language=English
Sign on error
.

MessageId=959
Severity=Informational
;//SymbolicName=
Language=English
Mail RTV error
.

MessageId=960
Severity=Informational
;//SymbolicName=
Language=English
Transmit error
.

MessageId=961
Severity=Informational
;//SymbolicName=
Language=English
Bad phone number
.

MessageId=962
Severity=Informational
;//SymbolicName=
Language=English
Transmit buffer too large
.

MessageId=963
Severity=Informational
;//SymbolicName=
Language=English
Unable to change mode
.

MessageId=964
Severity=Informational
;//SymbolicName=
Language=English
Unable to dial
.

MessageId=965
Severity=Informational
;//SymbolicName=
Language=English
Entry purged on user request
.

MessageId=966
Severity=Informational
;//SymbolicName=
Language=English
Entry for purging not found
.

;//******** AfdServer

MessageId=1001
Severity=Informational
;//SymbolicName=
Language=English
No root
.

MessageId=1002
Severity=Informational
;//SymbolicName=
Language=English
No Config File
.

MessageId=1003
Severity=Informational
;//SymbolicName=
Language=English
Error in Init Config file
.

MessageId=1004
Severity=Informational
;//SymbolicName=
Language=English
Max AFD Directories
.

MessageId=1005
Severity=Informational
;//SymbolicName=
Language=English
No AFD Directories
.

MessageId=1006
Severity=Informational
;//SymbolicName=
Language=English
Bad scan frequency
.

MessageId=1007
Severity=Informational
;//SymbolicName=
Language=English
Bad job number
.

MessageId=1008
Severity=Informational
;//SymbolicName=
Language=English
Max Target processes
.

MessageId=1009
Severity=Informational
;//SymbolicName=
Language=English
Bad target process
.

MessageId=1010
Severity=Informational
;//SymbolicName=
Language=English
No target process
.

MessageId=1011
Severity=Informational
;//SymbolicName=
Language=English
Bad profile
.

MessageId=1012
Severity=Informational
;//SymbolicName=
Language=English
Bad proc id
.

MessageId=1013
Severity=Informational
;//SymbolicName=
Language=English
Max Customer Id Location
.

MessageId=1014
Severity=Informational
;//SymbolicName=
Language=English
Bad customer Id location
.

MessageId=1015
Severity=Informational
;//SymbolicName=
Language=English
Error Data Src
.

MessageId=1016
Severity=Informational
;//SymbolicName=
Language=English
MT data src
.

MessageId=1017
Severity=Informational
;//SymbolicName=
Language=English
No data src
.

MessageId=1018
Severity=Informational
;//SymbolicName=
Language=English
Error in Open Database
.

MessageId=1019
Severity=Informational
;//SymbolicName=
Language=English
SQL error
.

MessageId=1020
Severity=Informational
;//SymbolicName=
Language=English
Invalid File spec
.

MessageId=1021
Severity=Informational
;//SymbolicName=
Language=English
MT Job File
.

MessageId=1022
Severity=Informational
;//SymbolicName=
Language=English
No more records
.

MessageId=1023
Severity=Informational
;//SymbolicName=
Language=English
Invalid queue type
.

MessageId=1024
Severity=Informational
;//SymbolicName=
Language=English
No Cust Id Location
.

MessageId=1025
Severity=Informational
;//SymbolicName=
Language=English
Database inconsistency
.

MessageId=1026
Severity=Informational
;//SymbolicName=
Language=English
Bad job entry
.

MessageId=1027
Severity=Informational
;//SymbolicName=
Language=English
Invalid Customer Id
.

MessageId=1028
Severity=Informational
;//SymbolicName=
Language=English
Invalid AFD response
.

MessageId=1029
Severity=Informational
;//SymbolicName=
Language=English
Invalid server number
.

MessageId=1030
Severity=Informational
;//SymbolicName=
Language=English
Invalid Console command
.

MessageId=1031
Severity=Informational
;//SymbolicName=
Language=English
Invalid abort command
.

MessageId=1032
Severity=Informational
;//SymbolicName=
Language=English
Operation Abort
.

MessageId=1033
Severity=Informational
;//SymbolicName=
Language=English
Invalid DBG level
.

MessageId=1034
Severity=Informational
;//SymbolicName=
Language=English
Invalid pointer
.

MessageId=1035
Severity=Informational
;//SymbolicName=
Language=English
Max retry file
.

MessageId=1036
Severity=Informational
;//SymbolicName=
Language=English
Job DB No Tmt
.

;//******** AfdScheduler

MessageId=61501
Severity=Informational
;//SymbolicName=
Language=English
Record file empty
.

MessageId=61502
Severity=Informational
;//SymbolicName=
Language=English
Error in record file size
.

MessageId=61503
Severity=Informational
;//SymbolicName=
Language=English
Read truncated
.

MessageId=61504
Severity=Informational
;//SymbolicName=
Language=English
Max job number
.

MessageId=61505
Severity=Informational
;//SymbolicName=
Language=English
DB error
.

MessageId=61506
Severity=Informational
;//SymbolicName=
Language=English
Out of sync
.

MessageId=61507
Severity=Informational
;//SymbolicName=
Language=English
Scheduler suspended
.

MessageId=61508
Severity=Informational
;//SymbolicName=
Language=English
Scheduler active
.

MessageId=61509
Severity=Informational
;//SymbolicName=
Language=English
Not a weekday
.

MessageId=61510
Severity=Informational
;//SymbolicName=
Language=English
Wrong system
.

;//******** AfdCommMonitor

MessageId=1101
Severity=Informational
;//SymbolicName=
Language=English
BadTPType
.

MessageId=1102
Severity=Informational
;//SymbolicName=
Language=English
BadQueueType
.

MessageId=1103
Severity=Informational
;//SymbolicName=
Language=English
InconsistentCall
.

MessageId=1104
Severity=Informational
;//SymbolicName=
Language=English
UndefinedError
.

MessageId=1105
Severity=Informational
;//SymbolicName=
Language=English
BadActionCode
.

MessageId=1106
Severity=Informational
;//SymbolicName=
Language=English
InconsistentActionCode
.

MessageId=1107
Severity=Informational
;//SymbolicName=
Language=English
EntryNotFound
.

MessageId=1108
Severity=Informational
;//SymbolicName=
Language=English
EntryDeletedByMonitor
.

;//****************AfdErrorDispatch Messages****************
MessageId=2000
Severity=Error
Facility=Runtime
SymbolicName=MSMQ_ERRORS
Language=English
%1
.

MessageId=2001
Severity=Error
Facility=Runtime
SymbolicName=HANDLED_EXCEPTION_ERROR
Language=English
%1
.

MessageId=2002
Severity=Error
Facility=Runtime
SymbolicName=DELEGATE_ERROR_TO_INTERFACE
Language=English
%1
.

MessageId=2003
Severity=Error
Facility=Runtime
SymbolicName=SERVICE_DISPATCH_ADO_ERROR
Language=English
%1
.


MessageId=2004
Severity=Error
Facility=Runtime
SymbolicName=MSMQ_FAILED_INITIALIZATION
Language=English
The Microsoft Message Queue could not be initialized
.

MessageId=2005
Severity=Error
Facility=Runtime
SymbolicName=DB_CONNECTION_FAILED
Language=English
A connection to the database could not be established.
.

MessageId=2006
Severity=Error
Facility=Runtime
SymbolicName=FAILED_ADO_CONNECTION_CREATION
Language=English
Could not instantiate ADO Connection object.
.

MessageId=2007
Severity=Warning
Facility=Runtime
SymbolicName=DISPATCH_INTERFACE_NOT_AVAILABLE
Language=English
A dispatch interface could not be found for this profile.  %1
.

MessageId=2008
Severity=Error
Facility=Runtime
SymbolicName=NULL_DISPATCH_MSG
Language=English
%1
.

MessageId=2009
Severity=Error
Facility=Runtime
SymbolicName=INVALID_MSMQ_ID
Language=English
Obtained invalid MessageId from MSMQ library for specific message. %1
.

MessageId=2010
Severity=Error
Facility=Runtime
SymbolicName=UNABLE_TO_CREATE_THREAD_PARAMS
Language=English
Unable to instantiate structure used to pass thread parameters. %1
.

MessageId=2011
Severity=Warning
Facility=Runtime
SymbolicName=NO_DISPATCHED_DUE_TO_INTERFACE_ERR
Language=English
Could not dispatch error due to problems experienced in Interface DLL. See additional logs for details.
.

MessageId=2012
Severity=Warning
Facility=Runtime
SymbolicName=NO_DISPATCHED_DUE_TO_DATABASE_ERR
Language=English
Could not dispatch error due to problems experienced in Database. See additional logs for details.
.

MessageId=2013
Severity=Warning
Facility=Runtime
SymbolicName=SERVICE_SHUTDOWN_SUCCESSFULLY
Language=English
Service shut down successfully
.


MessageId=2014
Severity=Informational
Facility=Runtime
SymbolicName=SNMP_AGENT_LOADED_SUCCESSFULLY
Language=English
SNMP Extension Agent %1 loaded successfully.
.

MessageId=2015
Severity=Informational
Facility=Runtime
SymbolicName=SNMP_AGENT_UNLOADED
Language=English
SNMP Extension Agent %1 unloaded.
.


MessageId=2016
Severity=Error
Facility=Runtime
SymbolicName=VAR_BIND_ALLOCATION_FAILED
Language=English
Allocation for SNMP Variable Binding failed %1.
.

