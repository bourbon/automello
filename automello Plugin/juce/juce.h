/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-9 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#ifndef __JUCE_JUCEHEADER__
#define __JUCE_JUCEHEADER__

//==============================================================================
/*
    This is the main JUCE header file that applications need to include.

*/
//==============================================================================

/* This line is here just to help catch syntax errors caused by mistakes in other header
   files that are included before juce.h. If you hit an error at this line, it must be some
   kind of syntax problem in whatever code immediately precedes this header.

   This also acts as a sanity-check in case you're trying to build with a C or obj-C compiler
   rather than a proper C++ one.
*/
namespace JuceDummyNamespace {}

#define JUCE_PUBLIC_INCLUDES 1

// (this includes things that need defining outside of the JUCE namespace)
#include "src/core/juce_StandardHeader.h"

BEGIN_JUCE_NAMESPACE

#if JUCE_MSVC
  // this is set explicitly in case the app is using a different packing size.
  #pragma pack (push, 8)
  #pragma warning (push)
  #pragma warning (disable: 4786) // (old vc6 warning about long class names)
  #ifdef __INTEL_COMPILER
   #pragma warning (disable: 1125)
  #endif
#endif

// this is where all the class header files get brought in..
#include "src/juce_core_includes.h"

// if you're compiling a command-line app, you might want to just include the core headers,
// so you can set this macro before including juce.h
#if ! JUCE_ONLY_BUILD_CORE_LIBRARY
  #include "src/juce_app_includes.h"
#endif

#if JUCE_MSVC
  #pragma warning (pop)
  #pragma pack (pop)
#endif

END_JUCE_NAMESPACE


//==============================================================================
#ifndef DONT_SET_USING_JUCE_NAMESPACE
#ifdef JUCE_NAMESPACE

  // this will obviously save a lot of typing, but can be disabled by
  // defining DONT_SET_USING_JUCE_NAMESPACE, in case there are conflicts.
  using namespace JUCE_NAMESPACE;

  /* On the Mac, these symbols are defined in the Mac libraries, so
     these macros make it easier to reference them without writing out
     the namespace every time.

     If you run into difficulties where these macros interfere with the contents
     of 3rd party header files, you may need to use the juce_WithoutMacros.h file - see
     the comments in that file for more information.
  */
  #if (JUCE_MAC || JUCE_IOS) && ! JUCE_DONT_DEFINE_MACROS
    #define Component       JUCE_NAMESPACE::Component
    #define MemoryBlock     JUCE_NAMESPACE::MemoryBlock
    #define Point           JUCE_NAMESPACE::Point
    #define Button          JUCE_NAMESPACE::Button
  #endif

  /* "Rectangle" is defined in some of the newer windows header files, so this makes
     it easier to use the juce version explicitly.

     If you run into difficulties where this macro interferes with other 3rd party header
     files, you may need to use the juce_WithoutMacros.h file - see the comments in that
     file for more information.
  */
  #if JUCE_WINDOWS && ! JUCE_DONT_DEFINE_MACROS
    #define Rectangle       JUCE_NAMESPACE::Rectangle
  #endif
#endif
#endif

//==============================================================================
/* Easy autolinking to the right JUCE libraries under win32.

   Note that this can be disabled by defining DONT_AUTOLINK_TO_JUCE_LIBRARY before
   including this header file.
*/
#if JUCE_MSVC

  #ifndef DONT_AUTOLINK_TO_JUCE_LIBRARY

    /** If you want your application to link to Juce as a DLL instead of
        a static library (on win32), just define the JUCE_DLL macro before
        including juce.h
    */
    #ifdef JUCE_DLL
      #if JUCE_DEBUG
        #define AUTOLINKEDLIB "JUCE_debug.lib"
      #else
        #define AUTOLINKEDLIB "JUCE.lib"
      #endif
    #else
      #if JUCE_DEBUG
        #ifdef _WIN64
          #define AUTOLINKEDLIB "jucelib_static_x64_debug.lib"
        #else
          #define AUTOLINKEDLIB "jucelib_static_Win32_debug.lib"
        #endif
      #else
        #ifdef _WIN64
          #define AUTOLINKEDLIB "jucelib_static_x64.lib"
        #else
          #define AUTOLINKEDLIB "jucelib_static_Win32.lib"
        #endif
      #endif
    #endif

    #pragma comment(lib, AUTOLINKEDLIB)

    #if ! DONT_LIST_JUCE_AUTOLINKEDLIBS
      #pragma message("JUCE! Library to link to: " AUTOLINKEDLIB)
    #endif

    // Auto-link the other win32 libs that are needed by library calls..
    #if ! (defined (DONT_AUTOLINK_TO_WIN32_LIBRARIES) || defined (JUCE_DLL))
      #include "src/native/windows/juce_win32_AutoLinkLibraries.h"
    #endif

  #endif

#endif

#endif   // __JUCE_JUCEHEADER__
