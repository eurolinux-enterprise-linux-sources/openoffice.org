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
PRJINC=..$/..$/inc
PRJNAME=unotools
TARGET=i18n

ENABLE_EXCEPTIONS=TRUE

# --- Settings ----------------------------------

.INCLUDE : settings.mk
.INCLUDE :      $(PRJ)$/util$/makefile.pmk

# --- Files -------------------------------------

SLOFILES=	\
	$(SLO)$/charclass.obj	\
	$(SLO)$/calendarwrapper.obj	\
	$(SLO)$/collatorwrapper.obj	\
    $(SLO)$/intlwrapper.obj \
	$(SLO)$/localedatawrapper.obj	\
	$(SLO)$/nativenumberwrapper.obj	\
	$(SLO)$/numberformatcodewrapper.obj \
    $(SLO)$/readwritemutexguard.obj \
	$(SLO)$/transliterationwrapper.obj \
	$(SLO)$/textsearch.obj

# --- Targets ----------------------------------

.INCLUDE : target.mk

