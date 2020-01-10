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

# mk file for $(OS)$(COM)$(CPU)$(COMEX) == WNTMSCI11 and WNTMSCI12

SOLAR_JAVA*=TRUE
FULL_DESK=TRUE
JAVAFLAGSDEBUG=-g

# SOLAR JAva Unterstuetzung nur fuer wntmsci

ASM=ml
AFLAGS=/c /Cp /coff

# architecture dependent flags for the C and C++ compiler that can be changed by
# exporting the variable ARCH_FLAGS="..." in the shell, which is used to start build
ARCH_FLAGS*=

CC*:=cl
.IF "$(bndchk)" != ""
CXX*=nmcl
.ELSE
.IF "$(truetime)" != ""
CXX*=nmcl /NMttOn
.ELSE
CXX*:=cl
.ENDIF
.ENDIF # "$(bndchk)" != ""

.IF "$(stoponerror)" != ""
CXX+= /NMstoponerror
.ENDIF

.IF "$(nmpass)" != ""
CXX+= /NMpass
.ENDIF

.IF "$(ttinlines)" != ""
CXX+= /NMttInlines
.ENDIF

.IF "$(ttnolines)" != ""
CXX+= /NMttNoLines
.ENDIF

# Flags for COMEX == 11

# disable "warning C4675: resolved overload was found by argument-dependent
# lookup":
# -wd4251 -wd4275 -wd4290 -wd4675 -wd4786 -wd4800
CFLAGS+=-Zm500 -Zc:forScope,wchar_t- -GR
CFLAGS+=-c -nologo -Gs $(NOLOGO)

CDEFS+= -D_X86_=1 -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE -D_CRT_NON_CONFORMING_SWPRINTFS

.IF "$(product)" != ""
CFLAGS+= -Gy
.ENDIF # "$(product)" != ""

.IF "$(bndchk)" == ""
.IF "$(VC_STANDARD)"==""
CFLAGS+= -Ob1
.ENDIF	# "$(VC_STANDARD)"==""
.ENDIF

# flags to enable build with symbols; required for crashdump feature
#CFLAGSENABLESYMBOLS=-Zi -Fd$(MISC)/_ooo_st_$(TARGET).PDB
CFLAGSENABLESYMBOLS=-Z7 -Yd

.IF "$(bndchk)" != ""
.IF "$(debug)" == ""
CFLAGS+= -Z7
.ENDIF
.ENDIF

.IF "$(truetime)" != ""
.IF "$(debug)" == ""
CFLAGS+= -Z7
.ENDIF
.ENDIF

.IF "$(FULL_DESK)"!=""
CDEFS+=-DFULL_DESK
RSCDEFS+=-DFULL_DESK
.ENDIF

CFLAGSEXCEPTIONS=-EHa
CFLAGS_NO_EXCEPTIONS=

# enable boost support for __cdecl (SAL_CALL) C++-UNO interface methods
CDEFS+=-DBOOST_MEM_FN_ENABLE_CDECL

# with the current debug switches PCH won't work
# anyway. so keep the existing .pch intact and don't
# touch it
.IF "$(debug)"!=""
ENABLE_PCH:=
.ENDIF "$(debug)"!=""

CFLAGS_CREATE_PCH=-I$(INCPCH) -Fo$(SLO)/pchname.obj -Ycprecompiled_$(PRJNAME).hxx -DPRECOMPILED_HEADERS
CFLAGS_USE_PCH=-I$(INCPCH) -Yuprecompiled_$(PRJNAME).hxx -Fp$(SLO)/pch/precompiled_$(PRJNAME).hxx$(PCHPOST) -DPRECOMPILED_HEADERS
CFLAGS_USE_EXCEPTIONS_PCH=-I$(INCPCH) -Yuprecompiled_$(PRJNAME).hxx -Fp$(SLO)/pch_ex/precompiled_$(PRJNAME).hxx$(PCHPOST) -DPRECOMPILED_HEADERS
.IF "$(CALL_CDECL)"=="TRUE"
CFLAGSCALL=-Gd
.ELSE			# "$(CALL_CDECL)"=="TRUE"
CFLAGSCALL=-Gz
.ENDIF			# "$(CALL_CDECL)"=="TRUE"

CFLAGSCC=$(ARCH_FLAGS)
.IF "$(DYNAMIC_CRT)"!=""
CDEFSSLOMT+=-DWIN32 -D_MT -D_DLL
CDEFSSLOMT+=-DWIN32 -D_MT -D_DLL
.IF "$(NO_DYNAMIC_OBJ)"==""
CDEFSOBJMT+=-DWIN32 -D_MT -D_DLL
CDEFSOBJMT+=-DWIN32 -D_MT -D_DLL
.ELSE
CDEFSOBJMT+=-DWIN32 -D_MT
CDEFSOBJMT+=-DWIN32 -D_MT
.ENDIF # "$(NO_DYNAMIC_OBJ)"==""
.ELSE
CDEFSSLOMT+=-DWIN32 -D_MT
CDEFSSLOMT+=-DWIN32 -D_MT
CDEFSOBJMT+=-DWIN32 -D_MT
CDEFSOBJMT+=-DWIN32 -D_MT
.ENDIF # "$(DYNAMIC_CRT)"!=""

CFLAGSPROF=-Gh -Fd$(MISC)/$(@:b).pdb
CFLAGSDEBUG=-Zi -Fd$(MISC)/$(@:b).pdb
CFLAGSDBGUTIL=
.IF "$(VC_STANDARD)"==""
CFLAGSOPT=-Oxs -Oy-
CFLAGSNOOPT=-Od
.ELSE			#  "$(VC_STANDARD)"==""
CFLAGSOPT=
CFLAGSNOOPT=
.ENDIF			#  "$(VC_STANDARD)"==""
CFLAGSOUTOBJ=-Fo

# For C and C++, certain warnings are disabled globally, as they result in
# spurious warnings and are hard or impossible to workaround:
# - "warning C4061: enumerate in switch of enum is not explicitly handled by a
#   case label",
# - "warning C4127: conditional expression is constant",
# - "warning C4191: unsafe conversion from function type to function type",
# - "warning C4217: member template functions cannot be used for copy-assignment
#   or copy-construction",
# - "warning C4250: 'class1' : inherits 'class2::member' via dominance",
# - "warning C4355: 'this' used in base member initializer list",
# - "warning C4511: copy constructor could not be generated",
# - "warning C4512: assignment operator could not be generated",
# - "warning C4514: unreferenced inline function has been removed",
# - "warning C4611: interaction between '_setjmp' and C++ object destruction is
#   non-portable",
# - "warning C4625: copy constructor could not be generated because a base class
#   copy constructor is inaccessible",
# - "warning C4626: assignment operator could not be generated because a base
#   class assignment operator is inaccessible",
# - "warning C4675: resolved overload was found by argument-dependent lookup",
# - "warning C4710: function not inlined",
# - "warning C4711: function selected for automatic inline expansion",
# - "warning C4820: padding added after member".
# - "warning C4503: 'identifier' : decorated name length exceeded, name was truncated"
#   (http://msdn2.microsoft.com/en-us/library/074af4b6.aspx)
# - "warning C4180: qualifier applied to function type has no meaning; ignored"
#   (frequently seen with a recent boost)
# For C, certain warnings from system headers (stdlib.h etc.) have to be
# disabled globally (for C++, this is not necessary, as the system headers are
# wrapped by STLport):
# - "warning C4255: no function prototype given: converting
#   '()' to '(void)'".
# - "warning C4365: conversion from ... to ... signed/unsigned mismatch"

CFLAGSWARNCXX=-Wall -wd4061 -wd4127 -wd4191 -wd4217 -wd4250 -wd4251 -wd4275 \
    -wd4290 -wd4294 -wd4355 -wd4511 -wd4512 -wd4514 -wd4611 -wd4625 -wd4626 \
    -wd4640 -wd4675 -wd4710 -wd4711 -wd4786 -wd4800 -wd4820 -wd4503 -wd4619 \
    -wd4365 -wd4668 -wd4738 -wd4826 -wd4350 -wd4505 -wd4692 -wd4189 -wd4005 \
    -wd4180
CFLAGSWARNCC=$(CFLAGSWARNCXX) -wd4255
CFLAGSWALLCC=$(CFLAGSWARNCC)
CFLAGSWALLCXX=$(CFLAGSWARNCXX)
CFLAGSWERRCC=-WX

# Once all modules on this platform compile without warnings, set
# COMPILER_WARN_ERRORS=TRUE here instead of setting MODULES_WITH_WARNINGS (see
# settings.mk):
MODULES_WITH_WARNINGS := \
    soldep

CDEFS+=-DSTLPORT_VERSION=400 -D_MT
.IF "$(COMEX)" == "11"
CDEFS+=-DWINVER=0x0400 -D_WIN32_IE=0x0400
_VC_MANIFEST_BASENAME=__VC80
.ELSE
CDEFS+=-DWINVER=0x0500 -D_WIN32_IE=0x0500
_VC_MANIFEST_BASENAME=__VC90
.ENDIF

LINK=link $(NOLOGO) /MACHINE:IX86
LINKOUTPUTFILTER= $(PIPEERROR) $(GREP) -v "LNK4197:"
.IF "$(PRODUCT)"!="full"
.ELSE
LINKFLAGS=/MAP /OPT:NOREF
.ENDIF

.IF "$(linkinc)" != ""
LINKFLAGS=-NODEFAULTLIB -INCREMENTAL:YES -DEBUG
MAPFILE=
_VC_MANIFEST_INC=1
.ELSE # "$(linkinc)" != ""
_VC_MANIFEST_INC=0
.IF "$(PRODUCT)"!="full"
LINKFLAGS+= -NODEFAULTLIB -DEBUG
.ELSE # "$(PRODUCT)"!="full"
LINKFLAGS+= -NODEFAULTLIB -RELEASE -DEBUG -INCREMENTAL:NO
.ENDIF # "$(PRODUCT)"!="full"
MAPFILE=-out:$$@
.ENDIF # "$(linkinc)" != ""

.IF "$(bndchk)" != ""
LINK=nmlink $(COMMENTFLAG) $(NOLOGO) /MACHINE:IX86
LINKFLAGS=-NODEFAULTLIB -DEBUG
.ENDIF

.IF "$(truetime)" != ""
LINK=nmlink /NMttOn $(COMMENTFLAG) $(NOLOGO) /MACHINE:IX86
LINKFLAGS=-NODEFAULTLIB -DEBUG
.ENDIF

.IF "$(COMEX)" == "11"
LINKFLAGSAPPGUI=/SUBSYSTEM:WINDOWS,4.0
LINKFLAGSSHLGUI=/SUBSYSTEM:WINDOWS,4.0 /DLL
.ELSE
LINKFLAGSAPPGUI=/SUBSYSTEM:WINDOWS
LINKFLAGSSHLGUI=/SUBSYSTEM:WINDOWS /DLL
.ENDIF # "$(COMEX)" == "11"
LINKFLAGSAPPCUI=/SUBSYSTEM:CONSOLE /BASE:0x1b000000
LINKFLAGSSHLCUI=/SUBSYSTEM:CONSOLE /DLL
LINKFLAGSTACK=/STACK:
LINKFLAGSPROF=/DEBUG:mapped,partial /DEBUGTYPE:coff cap.lib
LINKFLAGSWST=/DEBUG:mapped,partial /DEBUGTYPE:coff wst.lib /NODEFAULTLIB
LINKFLAGSDEBUG=-DEBUG
LINKFLAGSOPT=

UWINAPILIB*=uwinapi.lib
.IF "$(DYNAMIC_CRT)"!=""
.IF "$(USE_STLP_DEBUG)" != ""
LIBCMT=msvcrtd.lib
.ELSE  # "$(USE_STLP_DEBUG)" != ""
LIBCMT=msvcrt.lib
.ENDIF # "$(USE_STLP_DEBUG)" != ""
.ELSE # "$(DYNAMIC_CRT)"!=""
.IF "$(USE_STLP_DEBUG)" != ""
LIBCMT=libcmtd.lib
CDEFS+=-D_DEBUG
.ELSE  # "$(USE_STLP_DEBUG)" != ""
LIBCMT=libcmt.lib
.ENDIF # "$(USE_STLP_DEBUG)" != ""
.ENDIF # "$(DYNAMIC_CRT)"!=""

STDOBJVCL=$(L)/salmain.obj
STDOBJGUI=
STDSLOGUI=
STDOBJCUI=
STDSLOCUI=

STDLIBGUIMT=$(LIBCMT) $(UWINAPILIB) kernel32.lib user32.lib oldnames.lib
STDLIBCUIMT=$(LIBCMT) $(UWINAPILIB) kernel32.lib user32.lib oldnames.lib
STDSHLGUIMT=$(LIBCMT) $(UWINAPILIB) kernel32.lib user32.lib oldnames.lib
STDSHLCUIMT=$(LIBCMT) $(UWINAPILIB) kernel32.lib user32.lib oldnames.lib

.IF "$(USE_STLP_DEBUG)" != ""
LIBSTLPORT=stlport_vc71_stldebug.lib
LIBSTLPORTST=stlport_vc71_stldebug_static.lib
.ELSE
LIBSTLPORT=stlport_vc71.lib
LIBSTLPORTST=stlport_vc71_static.lib
.ENDIF

.IF "$(PROF_EDITION)" == ""
ATL_INCLUDE*=$(COMPATH)/PlatformSDK/include/atl
ATL_LIB*=$(COMPATH)/atlmfc/lib
MFC_INCLUDE*=$(COMPATH)/PlatformSDK/include/mfc
MFC_LIB*=$(COMPATH)/atlmfc/lib
.ELSE
ATL_INCLUDE*=$(COMPATH)/atlmfc/include
ATL_LIB*=$(COMPATH)/atlmfc/lib
MFC_INCLUDE*=$(COMPATH)/atlmfc/include
MFC_LIB*=$(COMPATH)/atlmfc/lib
.ENDIF

LIBMGR=lib $(NOLOGO)
IMPLIB=lib
LIBFLAGS=

IMPLIBFLAGS=-machine:IX86

MAPSYM=
MAPSYMFLAGS=

RC=rc
RCFLAGS=-r -DWIN32 -fo$@ $(RCFILES)
RCLINK=rc
RCLINKFLAGS=
RCSETVERSION=


DLLPOSTFIX=mi
PCHPOST=.pch

CSC*=$(FLIPCMD) csc
VBC*=vbc

ADVAPI32LIB=advapi32.lib
SHELL32LIB=shell32.lib
GDI32LIB=gdi32.lib
OLE32LIB=ole32.lib
OLEAUT32LIB=oleaut32.lib
UUIDLIB=uuid.lib
WINSPOOLLIB=winspool.lib
IMM32LIB=imm32.lib
VERSIONLIB=version.lib
WINMMLIB=winmm.lib
WSOCK32LIB=wsock32.lib
MPRLIB=mpr.lib
WS2_32LIB=ws2_32.lib
KERNEL32LIB=kernel32.lib
USER32LIB=user32.lib
COMDLG32LIB=comdlg32.lib
COMCTL32LIB=comctl32.lib
CRYPT32LIB=crypt32.lib
GDIPLUSLIB=gdiplus.lib
DBGHELPLIB=dbghelp.lib
MSILIB=msi.lib
DDRAWLIB=ddraw.lib
SHLWAPILIB=shlwapi.lib
URLMONLIB=urlmon.lib
UNICOWSLIB=unicows.lib
WININETLIB=wininet.lib
OLDNAMESLIB=oldnames.lib
MSIMG32LIB=msimg32.lib

