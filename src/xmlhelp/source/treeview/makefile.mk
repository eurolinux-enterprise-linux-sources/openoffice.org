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

PRJ=..$/..
PRJNAME=xmlhelp
TARGET=tvhlp
USE_DEFFILE=TRUE
ENABLE_EXCEPTIONS=TRUE
NO_BSYMBOLIC=TRUE

# Version
TVHLP_MAJOR=1

.INCLUDE: settings.mk

.IF "$(SYSTEM_EXPAT)" == "YES"
CFLAGS+=-DSYSTEM_EXPAT
.ENDIF

SLOFILES=\
	$(SLO)$/tvfactory.obj  \
	$(SLO)$/tvread.obj

LIB1TARGET=$(SLB)$/_$(TARGET).lib
LIB1OBJFILES=$(SLOFILES)

SHL1TARGET=$(TARGET)$(TVHLP_MAJOR)
SHL1DEF=$(MISC)$/$(SHL1TARGET).def
SHL1LIBS=$(LIB1TARGET)
SHL1IMPLIB=i$(TARGET)
SHL1STDLIBS=\
	$(CPPUHELPERLIB) \
	$(CPPULIB) \
	$(COMPHELPERLIB)         \
	$(EXPATASCII3RDLIB)      \
	$(SALLIB)  \
    $(UNOTOOLSLIB)
SHL1VERSIONMAP=	$(TARGET).map

DEF1NAME=$(SHL1TARGET)
DEF1DES=UCB : Treeview help


.INCLUDE: target.mk


