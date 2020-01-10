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

PRJNAME=sw
TARGET=rtf

# --- Settings -----------------------------------------------------

.INCLUDE :  $(PRJ)$/inc$/swpre.mk
.INCLUDE :  settings.mk
MAKING_LIBMSWORD=TRUE
.INCLUDE :  $(PRJ)$/inc$/sw.mk

.IF "$(mydebug)" != ""
CDEFS=$(CDEFS) -Dmydebug
.ENDIF

# --- Files --------------------------------------------------------

EXCEPTIONSFILES=	\
        $(SLO)$/rtffly.obj \
        $(SLO)$/rtfnum.obj \
		$(SLO)$/swparrtf.obj \
		$(SLO)$/wrtrtf.obj


SLOFILES =	\
		$(SLO)$/rtfatr.obj \
		$(SLO)$/rtffld.obj \
		$(SLO)$/rtffly.obj \
		$(SLO)$/rtfnum.obj \
		$(SLO)$/rtftbl.obj \
		$(SLO)$/swparrtf.obj \
		$(SLO)$/wrtrtf.obj

# --- Tagets -------------------------------------------------------

.INCLUDE :  target.mk

