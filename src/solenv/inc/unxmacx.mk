#*************************************************************************
#
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
#
# Copyright 2000, 2010 Oracle and/or its affiliates.
#
# OpenOffice.org - a multi-platform office productivity suite
#
# This file is part of OpenOffice.org.
#
# OpenOffice.org is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License version 3
# only, as published by the Free Software Foundation.
#
# OpenOffice.org is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License version 3 for more details
# (a copy is included in the LICENSE file that accompanied this code).
#
# You should have received a copy of the GNU Lesser General Public License
# version 3 along with OpenOffice.org.  If not, see
# <http://www.openoffice.org/license.html>
# for a copy of the LGPLv3 License.
#
#*************************************************************************

##########################################################################
# Platform MAKEFILE for Mac OS X and Darwin on both PowerPC and Intel
##########################################################################

# PROCESSOR_DEFINES and DLLPOSTFIX are defined in the particular platform file

ASM=
AFLAGS=
LINKOUTPUT_FILTER=

# Definitions that we may need on the compile line.
# -D_PTHREADS and -D_REENTRANT are needed for STLport, and must be specified when
#  compiling STLport sources too, either internally or externally.
CDEFS+=-DGLIBC=2 -D_PTHREADS -D_REENTRANT -DNO_PTHREAD_PRIORITY $(PROCESSOR_DEFINES) -DSTLPORT_VERSION=$(STLPORT_VER) -D_USE_NAMESPACE=1
.IF "$(GUIBASE)"=="unx" && "$(USE_SYSTEM_STL)"!="YES"
CDEFS+=-DX_LOCALE
.ENDIF
.IF "$(GUIBASE)"=="aqua"
# MAXOSX_DEPLOYMENT_TARGET : The minimum version required to run the build,
# build can assume functions/libraries of that version to be available
# unless you want to do runtime checks for 10.5 api, you also want to use the 10.4 sdk
# (safer/easier than dealing with the MAC_OS_X_VERSION_MAX_ALLOWED macro)
# http://developer.apple.com/technotes/tn2002/tn2064.html
MACOSX_DEPLOYMENT_TARGET=10.4
.EXPORT: MACOSX_DEPLOYMENT_TARGET
CDEFS+=-DQUARTZ 
EXTRA_CDEFS*=-isysroot /Developer/SDKs/MacOSX10.4u.sdk
.ENDIF

# Name of library where static data members are initialized
# STATICLIBNAME=static$(DLLPOSTFIX)
# STATICLIB=-l$(STATICLIBNAME)

# enable visibility define in "sal/types.h"
.IF "$(HAVE_GCC_VISIBILITY_FEATURE)" == "TRUE"
CDEFS += -DHAVE_GCC_VISIBILITY_FEATURE
.ENDIF # "$(HAVE_GCC_VISIBILITY_FEATURE)" == "TRUE"


# MacOS X specific Java compilation/link flags
SOLAR_JAVA*=TRUE
.IF "$(SOLAR_JAVA)"!=""
	JAVADEF=-DSOLAR_JAVA
	JAVAFLAGSDEBUG=-g
	JAVA_RUNTIME=-framework JavaVM
.ENDIF

# architecture dependent flags for the C and C++ compiler that can be changed by
# exporting the variable ARCH_FLAGS="..." in the shell, which is used to start build
ARCH_FLAGS*=

# Specify the compiler to use.  NOTE:  MacOS X should always specify
# c++ for C++ compilation as it does certain C++ specific things
# behind the scenes for us.
# CC = C++ compiler to use
# cc = C compiler to use
# objc = Objective C compiler to use
# objcpp = Objective C++ compiler to use
CXX*=g++
CC*=gcc
objc*=gcc
objcpp*=g++

CFLAGS=-fsigned-char -fmessage-length=0 -malign-natural -c $(EXTRA_CFLAGS)

.IF "$(DISABLE_DEPRECATION_WARNING)" == "TRUE"
CFLAGS+=-Wno-deprecated-declarations
.ENDIF
# ---------------------------------
#  Compilation flags
# ---------------------------------
# Normal C compilation flags
CFLAGSCC=-pipe -fsigned-char -malign-natural $(ARCH_FLAGS)

# Normal Objective C compilation flags
#OBJCFLAGS=-no-precomp
OBJCFLAGS=-fobjc-exceptions
# -x options generally ignored by ccache, tell it that it can cache
# the result nevertheless
CCACHE_SKIP:=$(eq,$(USE_CCACHE),YES --ccache-skip $(NULL))
OBJCXXFLAGS:=$(CCACHE_SKIP) -x $(CCACHE_SKIP) objective-c++ -fobjc-exceptions

# Comp Flags for files that need exceptions enabled (C and C++)
CFLAGSEXCEPTIONS=-fexceptions -fno-enforce-eh-specs

# Comp Flags for files that do not need exceptions enabled (C and C++)
CFLAGS_NO_EXCEPTIONS=-fno-exceptions

# Normal C++ compilation flags
CFLAGSCXX=-pipe -malign-natural -fsigned-char -Wno-long-double $(ARCH_FLAGS)
CFLAGSCXX+= -Wno-ctor-dtor-privacy

PICSWITCH:=-fPIC
# Other flags
CFLAGSOBJGUIMT=$(PICSWITCH) -fno-common
CFLAGSOBJCUIMT=$(PICSWITCH) -fno-common
CFLAGSSLOGUIMT=$(PICSWITCH) -fno-common
CFLAGSSLOCUIMT=$(PICSWITCH) -fno-common
CFLAGSPROF=

# Flag for including debugging information in object files
CFLAGSDEBUG=-g
CFLAGSDBGUTIL=

# Flag to specify output file to compiler/linker
CFLAGSOUTOBJ=-o

# Flags to enable precompiled headers
CFLAGS_CREATE_PCH=-x c++-header -I$(INCPCH) -DPRECOMPILED_HEADERS
CFLAGS_USE_PCH=-I$(SLO)/pch -DPRECOMPILED_HEADERS -Winvalid-pch
CFLAGS_USE_EXCEPTIONS_PCH=-I$(SLO)/pch_ex -DPRECOMPILED_HEADERS -Winvalid-pch

# ---------------------------------
#  Optimization flags
# ---------------------------------
CFLAGSOPT=-O2 -fno-strict-aliasing
CFLAGSNOOPT=-O0

# -Wshadow does not work for C with nested uses of pthread_cleanup_push:
# -Wshadow does not work for C++ as /usr/include/c++/4.0.0/ext/hashtable.h
# l. 717 contains a declaration of __cur2 shadowing the declaration at l. 705,
# in template code for which a #pragma gcc system_header would not work:
# -Wextra doesn not work for gcc-3.3
CFLAGSWARNCC=-Wall -Wendif-labels
CFLAGSWARNCXX=$(CFLAGSWARNCC) -Wno-ctor-dtor-privacy -Wno-non-virtual-dtor
CFLAGSWALLCC=$(CFLAGSWARNCC)
CFLAGSWALLCXX=$(CFLAGSWARNCXX)
CFLAGSWERRCC=-Werror

# All modules on this platform compile without warnings.
# If you need to set MODULES_WITH_WARNINGS here, comment
# COMPILER_WARN_ERRORS=TRUE here (see settings.mk):
COMPILER_WARN_ERRORS=TRUE

#special settings form environment
CDEFS+=$(EXTRA_CDEFS)

STDLIBCPP=-lstdc++

# ---------------------------------
#  STLport library names
# ---------------------------------
.IF "$(USE_STLP_DEBUG)" != ""
.IF "$(STLPORT_VER)" >= "500"
LIBSTLPORT=-lstlportstlg
LIBSTLPORTST=$(STATIC) -lstlportstlg
.ELSE
LIBSTLPORT=-lstlport_gcc_stldebug
LIBSTLPORTST=$(SOLARVERSION)/$(INPATH)/lib/libstlport_gcc_stldebug.a
.ENDIF
.ELSE # "$(USE_STLP_DEBUG" != ""
.IF "$(STLPORT_VER)" >= "500"
LIBSTLPORT=-lstlport
LIBSTLPORTST=$(STATIC) -lstlport
.ELSE
LIBSTLPORT=-lstlport_gcc
LIBSTLPORTST=$(SOLARVERSION)/$(INPATH)/lib/libstlport_gcc.a
.ENDIF
.ENDIF # "$(USE_STLP_DEBUG" != ""

# ---------------------------------
#  Link stage flags
# ---------------------------------
# always link with gcc since you may be linking c code and don't want -lstdc++ linked in!

##  ericb 04 mars 2005

LINK*=$(CXX)
LINKC*=$(CC)

LINKFLAGSDEFS*=-Wl,-multiply_defined,suppress
# assure backwards-compatibility
EXTRA_LINKFLAGS*=-Wl,-syslibroot,/Developer/SDKs/MacOSX10.4u.sdk
# Very long install_names are needed so that install_name_tool -change later on
# does not complain that "larger updated load commands do not fit:"
LINKFLAGSRUNPATH_URELIB=-install_name '@__________________________________________________URELIB/$(@:f)'
LINKFLAGSRUNPATH_UREBIN=
LINKFLAGSRUNPATH_OOO=-install_name '@__________________________________________________OOO/$(@:f)'
LINKFLAGSRUNPATH_SDK=
LINKFLAGSRUNPATH_BRAND=
LINKFLAGSRUNPATH_OXT=
LINKFLAGSRUNPATH_NONE=
LINKFLAGS=$(LINKFLAGSDEFS)

# [ed] 5/14/02 If we're building for aqua, add in the objc runtime library into our link line
.IF "$(GUIBASE)" == "aqua"
	LINKFLAGS+=-lobjc
	# Sometimes we still use files that would be in a GUIBASE="unx" specific directory
	# because they really aren't GUIBASE specific, so we've got to account for that here.
	INCGUI+= -I$(PRJ)/unx/inc
.ENDIF

#special settings form environment
LINKFLAGS+=$(EXTRA_LINKFLAGS)

# Random link flags dealing with different cases of linking

LINKFLAGSAPPGUI=-bind_at_load
LINKFLAGSSHLGUI=-dynamiclib -single_module
LINKFLAGSAPPCUI=-bind_at_load
LINKFLAGSSHLCUI=-dynamiclib -single_module
LINKFLAGSTACK=
LINKFLAGSPROF=

# Flag to add debugging information to final products
LINKFLAGSDEBUG=-g
LINKFLAGSOPT=

# ---------------------------------
#  MacOS X shared library specifics
# ---------------------------------

# Tag to identify an output file as a library
DLLPRE=lib
# File extension to identify dynamic shared libraries on MacOS X
DLLPOST=.dylib
# Precompiled header file extension
PCHPOST=.gch

# We don't use mapping on MacOS X
#LINKVERSIONMAPFLAG=-Wl,--version-script
LINKVERSIONMAPFLAG=-Wl,-exported_symbols_list

SONAME_SWITCH=-Wl,-h

STDLIBCPP=-lstdc++

STDOBJVCL=$(L)/salmain.o
STDOBJGUI=
STDSLOGUI=
STDOBJCUI=
STDSLOCUI=

.IF "$(GUIBASE)" == "aqua"
	STDLIBCUIMT=CPPRUNTIME -lm
	STDLIBGUIMT=-framework Carbon -framework Cocoa -lpthread CPPRUNTIME -lm
	STDSHLCUIMT=-lpthread CPPRUNTIME -lm
	STDSHLGUIMT=-framework Carbon -framework CoreFoundation -framework Cocoa -lpthread CPPRUNTIME -lm
.ELSE
	STDLIBCUIMT= CPPRUNTIME -lm
	STDLIBGUIMT=-lX11 -lpthread CPPRUNTIME -lm
	STDSHLCUIMT=-lpthread CPPRUNTIME -lm
	STDSHLGUIMT=-lX11 -lXext -lpthread CPPRUNTIME -lm -framework CoreFoundation
.ENDIF

LIBMGR=ar
LIBFLAGS=-r

IMPLIB=
IMPLIBFLAGS=

MAPSYM=
MAPSYMFLAGS=

RC=irc
RCFLAGS=-fo$@ $(RCFILES)
RCLINK=
RCLINKFLAGS=
RCSETVERSION=

# Add SOLARLIBDIR to the end of a (potentially previously undefined)
# DYLD_LIBRARY_PATH (there is no real reason to prefer adding at the end over
# adding at the start); the ": &&" in the bash case enables this to work at the
# start of a recipe line that is not prefixed by "+" as well as in the middle of
# an existing && chain; the tcsh case is somewhat imprecise in that it
# potentially affects multiple commands following on the recipe line:
.IF "$(USE_SHELL)" == "bash"
AUGMENT_LIBRARY_PATH = : && \
    DYLD_LIBRARY_PATH=$${{DYLD_LIBRARY_PATH+$${{DYLD_LIBRARY_PATH}}:}}$(SOLARLIBDIR)
.ELSE
AUGMENT_LIBRARY_PATH = if ($$?DYLD_LIBRARY_PATH == 1) \
    eval 'setenv DYLD_LIBRARY_PATH "$${{DYLD_LIBRARY_PATH}}:$(SOLARLIBDIR)"' \
    && if ($$?DYLD_LIBRARY_PATH == 0) \
    setenv DYLD_LIBRARY_PATH "$(SOLARLIBDIR)" &&
.ENDIF
