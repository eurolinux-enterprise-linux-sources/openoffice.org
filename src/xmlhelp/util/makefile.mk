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

UCP_VERSION=1
UCP_NAME=chelp

PRJ=..
PRJNAME=xmlhelp
TARGET=ucp$(UCP_NAME)
UCPHELP_MAJOR=1

ENABLE_EXCEPTIONS=TRUE
USE_DEFFILE=TRUE
NO_BSYMBOLIC=TRUE

# --- Settings ---------------------------------------------------------

.INCLUDE: settings.mk

.IF "$(GUI)"=="WNT"
CFLAGS+=-GR
.ENDIF

# --- Shared-Library ---------------------------------------------------

SHL1TARGET=$(TARGET)$(UCP_VERSION)
SHL1DEF=$(MISC)$/$(SHL1TARGET).def
SHL1IMPLIB=i$(TARGET)
SHL1VERSIONMAP=exports.map

# Add additional libs here.
SHL1STDLIBS=                     \
	$(CPPUHELPERLIB)         \
	$(CPPULIB)               \
	$(COMPHELPERLIB)         \
	$(SALLIB)                \
	$(EXPATASCII3RDLIB)      \
	$(UCBHELPERLIB)          \
	$(SVTOOLLIB)             	 \
	$(BERKELEYLIB)		 	 \
	$(XSLTLIB) \
    $(UNOTOOLSLIB)

SHL1LIBS =                       \
	$(SLB)$/jaqe.lib         \
	$(SLB)$/jautil.lib       \
	$(SLB)$/chelp.lib

# --- Def-File ---------------------------------------------------------

DEF1NAME=$(SHL1TARGET)

# --- Targets ----------------------------------------------------------

.INCLUDE: target.mk

