# Microsoft Developer Studio Project File - Name="automello Plugin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **
# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
CFG=automello Plugin - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "automello Plugin.mak."
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "automello Plugin.mak" CFG="automello Plugin - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "automello Plugin - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "automello Plugin - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
!IF  "$(CFG)" == "automello Plugin - Win32 Debug"
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /Od /D WIN32 /D _WINDOWS /D DEBUG /D _DEBUG /D JUCER_MSVC6_734A9119=1 /YX /FD /c /Gm /ZI /GZ /Zm1024
# ADD CPP /nologo /MTd /W3 /GR /GX /Od /I ..\..\..\vstsdk2.4 /I ..\..\JuceLibraryCode /D WIN32 /D _WINDOWS /D DEBUG /D _DEBUG /D JUCER_MSVC6_734A9119=1 /D "_UNICODE" /D "UNICODE" /FD /c /Zm1024 /Gm /ZI /GZ 
# ADD BASE MTL /nologo /D WIN32 /D _WINDOWS /D DEBUG /D _DEBUG /D JUCER_MSVC6_734A9119=1 /mktyplib203 /win32
# ADD MTL /nologo /D WIN32 /D _WINDOWS /D DEBUG /D _DEBUG /D JUCER_MSVC6_734A9119=1 /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d WIN32 /D _WINDOWS /D DEBUG /D _DEBUG /D JUCER_MSVC6_734A9119=1
# ADD RSC /l 0x40c /d WIN32 /D _WINDOWS /D DEBUG /D _DEBUG /D JUCER_MSVC6_734A9119=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 "C:\Program Files\Microsoft Visual Studio\VC98\LIB\shell32.lib" kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  /debug /nologo /machine:I386 /out:".\Debug\automello Plugin.dll" /dll
!ELSEIF  "$(CFG)" == "automello Plugin - Win32 Release"
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D WIN32 /D _WINDOWS /D NDEBUG /D JUCER_MSVC6_734A9119=1 /YX /FD /c  /Zm1024
# ADD CPP /nologo /MT /W3 /GR /GX /O2 /I ..\..\..\vstsdk2.4 /I ..\..\JuceLibraryCode /D WIN32 /D _WINDOWS /D NDEBUG /D JUCER_MSVC6_734A9119=1 /D "_UNICODE" /D "UNICODE" /FD /c /Zm1024  
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D WIN32 /D _WINDOWS /D NDEBUG /D JUCER_MSVC6_734A9119=1 /mktyplib203 /win32
# ADD MTL /nologo /D WIN32 /D _WINDOWS /D NDEBUG /D JUCER_MSVC6_734A9119=1 /mktyplib203 /win32
# ADD BASE RSC /l 0x40c /d WIN32 /D _WINDOWS /D NDEBUG /D JUCER_MSVC6_734A9119=1
# ADD RSC /l 0x40c /d WIN32 /D _WINDOWS /D NDEBUG /D JUCER_MSVC6_734A9119=1
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 "C:\Program Files\Microsoft Visual Studio\VC98\LIB\shell32.lib" kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  /nologo /machine:I386 /out:".\Release\automello Plugin.dll" /dll
!ENDIF
# Begin Target
# Name "automello Plugin - Win32 Debug"
# Name "automello Plugin - Win32 Release"
# Begin Group "automello Plugin"
# PROP Default_Filter "cpp;c;cc;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Source"
# PROP Default_Filter "cpp;c;cc;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File
SOURCE="..\..\Source\PluginProcessor.cpp"
# End Source File
# Begin Source File
SOURCE="..\..\Source\PluginProcessor.h"
# End Source File
# Begin Source File
SOURCE="..\..\Source\PluginEditor.cpp"
# End Source File
# Begin Source File
SOURCE="..\..\Source\PluginEditor.h"
# End Source File
# End Group
# End Group
# Begin Group "Juce Library Code"
# Begin Source File
SOURCE="..\..\JuceLibraryCode\AppConfig.h"
# End Source File
# Begin Source File
SOURCE="..\..\JuceLibraryCode\JuceHeader.h"
# End Source File
# Begin Source File
SOURCE="..\..\JuceLibraryCode\JuceLibraryCode1.cpp"
# End Source File
# Begin Source File
SOURCE="..\..\JuceLibraryCode\JuceLibraryCode2.cpp"
# End Source File
# Begin Source File
SOURCE="..\..\JuceLibraryCode\JuceLibraryCode3.cpp"
# End Source File
# Begin Source File
SOURCE="..\..\JuceLibraryCode\JuceLibraryCode4.cpp"
# End Source File
# Begin Source File
SOURCE="..\..\JuceLibraryCode\JucePluginCharacteristics.h"
# End Source File
# End Group
# Begin Group "Juce VST Wrapper"
# Begin Source File
SOURCE="..\..\..\juce\src\audio\plugin_client\VST\juce_VST_Wrapper.cpp"
# End Source File
# End Group
# End Target
# End Project
