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

PRJNAME=	ucbhelper
TARGET=		ucbhelper

# --- Settings -----------------------------------------------------

.INCLUDE :	settings.mk
.INCLUDE :	..$/version.mk

# --- Files --------------------------------------------------------

LIB1TARGET=	$(SLB)$/$(TARGET).lib
LIB1FILES=	$(SLB)$/client.lib \
			$(SLB)$/provider.lib

SHL1TARGET=	$(TARGET)$(UCBHELPER_MAJOR)$(COMID)
.IF "$(GUI)" == "OS2"
SHL1TARGET=	ucbh$(UCBHELPER_MAJOR)
.ENDIF
SHL1STDLIBS = \
    $(CPPUHELPERLIB) \
    $(CPPULIB) \
    $(SALHELPERLIB) \
    $(SALLIB)

SHL1DEPN=
SHL1IMPLIB=	i$(TARGET)
SHL1USE_EXPORTS=name
SHL1LIBS=	$(LIB1TARGET)
SHL1DEF=	$(MISC)$/$(SHL1TARGET).def

DEF1NAME=	$(SHL1TARGET)
DEF1DEPN=	$(MISC)$/$(SHL1TARGET).flt
DEFLIB1NAME=	$(TARGET)
DEF1DES=	Universal Content Broker - Helpers

# --- Targets ------------------------------------------------------

.INCLUDE :	target.mk

# --- Filter -----------------------------------------------------------

$(MISC)$/$(SHL1TARGET).flt : ucbhelper.flt
	@echo ------------------------------
	@echo Making: $@
	@$(TYPE) ucbhelper.flt > $@

