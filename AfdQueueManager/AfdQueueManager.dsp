# Microsoft Developer Studio Project File - Name="AfdQueueManager" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=AfdQueueManager - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "AfdQueueManager.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "AfdQueueManager.mak" CFG="AfdQueueManager - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "AfdQueueManager - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "AfdQueueManager - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/AfdQueueManager", ZJAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "AfdQueueManager - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\AfdQueue"
# PROP BASE Intermediate_Dir ".\AfdQueue"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\..\AfdTpSource\Bin\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MT /W4 /GX /Zi /Od /I "..\AfdQMMsg" /I "..\..\AfdTPSource\Afdinclude" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /FAcs /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /i "..\AfdQMMsg" /i "E:\ING\AfdUtilSource\AfdQMMsg" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 AfdMisc.Lib AfdConfig.Lib AfdScmInterface.Lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /map /debug /machine:I386 /libpath:"..\..\AfdTpSource\Libs\Release"

!ELSEIF  "$(CFG)" == "AfdQueueManager - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\AfdQueue"
# PROP BASE Intermediate_Dir ".\AfdQueue"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\..\AfdTpSource\Bin\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir "."
# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /YX /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "\Ing\AfdUtilSource\AfdQMMsg\\" /I "..\..\AfdTPSource\Afdinclude" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /FR /YX /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /i "d:\Ing\AfdUtilSource\AfdQMMsg" /i "E:\ING\AfdUtilSource\AfdQMMsg" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386
# ADD LINK32 AfdMisc.Lib AfdConfig.Lib AfdScmInterface.Lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /libpath:"..\..\AfdTpSource\Libs\Debug"

!ENDIF 

# Begin Target

# Name "AfdQueueManager - Win32 Release"
# Name "AfdQueueManager - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;hpj;bat;for;f90"
# Begin Source File

SOURCE=.\AfdQueueManagerVersion.rc
# End Source File
# Begin Source File

SOURCE=.\PipeSrv.cpp
# End Source File
# Begin Source File

SOURCE=.\QMQueueObj.cpp
# End Source File
# Begin Source File

SOURCE=.\QMService.cpp
# End Source File
# Begin Source File

SOURCE=.\ScmIf.cpp
# End Source File
# Begin Source File

SOURCE=.\TRACE.CPP
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl;fi;fd"
# Begin Source File

SOURCE=.\Build.h
# End Source File
# Begin Source File

SOURCE=.\Req.h
# End Source File
# Begin Source File

SOURCE=.\RqDefs.h
# End Source File
# Begin Source File

SOURCE=.\RqStruct.h
# End Source File
# Begin Source File

SOURCE=.\TRACE.H
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=.\project.lnt
# End Source File
# End Target
# End Project
