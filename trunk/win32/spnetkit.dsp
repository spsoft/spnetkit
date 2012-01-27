# Microsoft Developer Studio Project File - Name="spnetkit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=spnetkit - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "spnetkit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "spnetkit.mak" CFG="spnetkit - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "spnetkit - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "spnetkit - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "spnetkit - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "spnetkit - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "spnetkit - Win32 Release"
# Name "spnetkit - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\spnetkit\spnkbase64.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkbuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkconfig.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkendpoint.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkfile.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkhash.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkhttpcli.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkhttpmsg.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkhttputils.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkicapcli.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkini.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnklist.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnklog.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkmemcli.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkmemobj.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkmiltercli.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkpop3cli.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkreader.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnksmtpaddr.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnksmtpcli.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnksocket.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnksocketpool.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkstr.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkthread.cpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnktime.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\spnetkit\spnkbase64.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkendpoint.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkfile.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkgetopt.h
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkhash.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkhttpcli.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkhttpmsg.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkicapcli.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkini.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnklist.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnklog.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkmemcli.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkmemobj.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkmiltercli.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkpop3cli.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkporting.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkreader.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnksmtpaddr.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnksmtpcli.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnksocket.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnksocketpool.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkstr.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnkthread.hpp
# End Source File
# Begin Source File

SOURCE=..\spnetkit\spnktime.hpp
# End Source File
# End Group
# End Target
# End Project
