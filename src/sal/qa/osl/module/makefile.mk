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

PRJ=..$/..$/..

PRJNAME=sal
TARGET=qa_module

ENABLE_EXCEPTIONS=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk

CFLAGS+= $(LFS_CFLAGS)
CXXFLAGS+= $(LFS_CFLAGS)

# BEGIN ----------------------------------------------------------------

# --- test dll ------------------------------------------------------
SHL1TARGET     = Module_DLL
SHL1OBJS       = $(SLO)$/osl_Module_DLL.obj
SHL1STDLIBS    = $(SALLIB) 
SHL1IMPLIB     = i$(SHL1TARGET)
SHL1DEF        = $(MISC)$/$(SHL1TARGET).def
DEF1NAME       = $(SHL1TARGET)
SHL1VERSIONMAP = export_dll.map


# --- main l ------------------------------------------------------
SHL2OBJS=  $(SLO)$/osl_Module.obj

SHL2TARGET= osl_Module
SHL2STDLIBS=   $(SALLIB) 

.IF "$(GUI)" == "WNT"
SHL2STDLIBS+=	$(SOLARLIBDIR)$/cppunit.lib 
SHL2STDLIBS+=i$(SHL2TARGET).lib
.ENDIF
.IF "$(GUI)" == "OS2"
SHL2STDLIBS+=	$(SOLARLIBDIR)$/cppunit.lib 
.ENDIF
.IF "$(GUI)" == "UNX"
SHL2STDLIBS+=$(SOLARLIBDIR)$/libcppunit$(DLLPOSTFIX).a
APP3STDLIBS+=-l$(SHL2TARGET)
.ENDIF

SHL2DEPN= $(SHL1OBJS) 
SHL2IMPLIB= i$(SHL2TARGET)
SHL2DEF=    $(MISC)$/$(SHL2TARGET).def

DEF2NAME    =$(SHL2TARGET)
SHL2VERSIONMAP= $(PRJ)$/qa$/export.map
# END ------------------------------------------------------------------

# --- Targets ------------------------------------------------------

.INCLUDE :  target.mk
.INCLUDE : _cppunit.mk

