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

PRJ=..

PRJNAME=basebmp
TARGET=tests
TARGETTYPE=GUI

ENABLE_EXCEPTIONS=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk

.IF "$(debug)"!="" || "$(DEBUG)"!=""

.IF "$(COM)"=="MSC"
# disable inlining for MSVC
CFLAGS += -Ob0
.ENDIF

.IF "$(COM)"=="GCC"
# disable inlining for gcc
CFLAGS += -fno-inline
.ENDIF

.ENDIF

# SunStudio 12 (-m64 and -m32 modes): three test cases of the unit tests fail
# if compiled with default -xalias_level (and optimization level -xO3)
.IF "$(OS)"=="SOLARIS" 
# For Sun Studio 8 this switch does not work: compilation fails on bitmapdevice.cxx
.IF "$(CCNUMVER)"!="00050005"
CDEFS+=-xalias_level=compatible
.ENDIF
.ENDIF

# --- Common ----------------------------------------------------------
.IF "$(L10N_framework)"==""

# BEGIN ----------------------------------------------------------------
# auto generated Target:tests by codegen.pl 
SHL1OBJS=  \
	$(SLO)$/basictest.obj		\
	$(SLO)$/bmpmasktest.obj		\
	$(SLO)$/bmptest.obj		    \
	$(SLO)$/cliptest.obj		\
	$(SLO)$/filltest.obj		\
	$(SLO)$/linetest.obj		\
	$(SLO)$/masktest.obj		\
	$(SLO)$/polytest.obj		\
	$(SLO)$/tools.obj		    \
	$(SLO)$/bitmapdevice.obj    \
	$(SLO)$/debug.obj			\
	$(SLO)$/polypolygonrenderer.obj
# last three objs are a bit of a hack: cannot link against LIBBASEBMP
# here, because not yet delivered. Need the functionality to test, so
# we're linking it in statically. Need to keep this in sync with
# source/makefile.mk
SHL1TARGET= tests
SHL1STDLIBS= 	$(SALLIB)		 \
				$(CPPUNITLIB)	 \
				$(BASEGFXLIB)	

SHL1IMPLIB= i$(SHL1TARGET)

DEF1NAME    =$(SHL1TARGET)
SHL1VERSIONMAP = export.map 
.ENDIF
# END ------------------------------------------------------------------

#APP2TARGET= bmpdemo

#APP2OBJS=	\
#	$(OBJ)$/bmpdemo.obj

#APP2STDLIBS=$(TOOLSLIB) 		\
#			$(COMPHELPERLIB)	\
#			$(BASEGFXLIB)	    \
#			$(BASEBMPLIB)	    \
#			$(CPPULIB)			\
#			$(CPPUHELPERLIB)	\
#			$(UCBHELPERLIB)		\
#			$(SALLIB)			\
#			$(VCLLIB)	
#
#.IF "$(GUI)"!="UNX"
#APP2DEF=	$(MISC)$/$(TARGET).def
#.ENDIF

#------------------------------- All object files -------------------------------
# do this here, so we get right dependencies
SLOFILES=$(SHL1OBJS) 

# --- Targets ------------------------------------------------------

.INCLUDE : target.mk
.INCLUDE : _cppunit.mk 

# --- Enable test execution in normal build ------------------------
.IF "$(L10N_framework)"==""

unittest : $(SHL1TARGETN)
		@echo ----------------------------------------------------------
		@echo - start unit test on library $(SHL1TARGETN)
		@echo ----------------------------------------------------------
		$(TESTSHL2) -sf $(mktmp ) $(SHL1TARGETN)

ALLTAR : unittest
.ENDIF
