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

PRJNAME=i18npool
TARGET=i18nisolang

# --- Settings -----------------------------------------------------

.INCLUDE : settings.mk
.INCLUDE : $(PRJ)$/version.mk
.INCLUDE : $(PRJ)$/util$/makefile.pmk

# --- Files --------------------------------------------------------

SLOFILES=	$(SLO)$/insys.obj  \
			$(SLO)$/isolang.obj  \
			$(SLO)$/mslangid.obj

SHL1TARGET=		$(ISOLANG_TARGET)$(ISOLANG_MAJOR)$(COMID)
SHL1IMPLIB=		i$(ISOLANG_TARGET)

DEF1DEPN=		$(MISC)$/$(SHL1TARGET).flt
SHL1DEF=		$(MISC)$/$(SHL1TARGET).def
DEF1NAME=		$(SHL1TARGET)
DEFLIB1NAME=	$(SHL1TARGET)

SHL1OBJS=		$(SLOFILES)

LIB1TARGET=	$(SLB)$/$(SHL1TARGET).lib
LIB1OBJFILES=$(SHL1OBJS)

SHL1STDLIBS= \
		$(SALLIB)

# --- Targets ------------------------------------------------------

.INCLUDE :	target.mk

$(MISC)$/$(SHL1TARGET).flt: makefile.mk
	@echo ------------------------------
	@echo Making: $@
	@echo CLEAR_THE_FILE > $@
	@echo __CT >> $@
