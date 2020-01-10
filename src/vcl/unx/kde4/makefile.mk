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

PRJNAME=vcl
TARGET=kde4plug
.INCLUDE :  $(PRJ)$/util$/makefile.pmk

# workaround for makedepend hang
MKDEPENDSOLVER=

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :  $(PRJ)$/util$/makefile2.pmk

# For some of the included external KDE headers, GCC complains about shadowed
# symbols in instantiated template code only at the end of a compilation unit,
# so the only solution is to disable that warning here:
.IF "$(COM)" == "GCC"
CFLAGSCXX+=-Wno-shadow
.ENDIF

# --- Files --------------------------------------------------------

.IF "$(GUIBASE)"!="unx"

dummy:
	@echo "Nothing to build for GUIBASE $(GUIBASE)"

.ELSE		# "$(GUIBASE)"!="unx"

.IF "$(ENABLE_KDE4)" != ""

CFLAGS+=$(KDE4_CFLAGS)

.IF "$(ENABLE_RANDR)" != ""
CDEFS+=-DUSE_RANDR
.ENDIF

SLOFILES=\
	$(SLO)$/main.obj \
	$(SLO)$/VCLKDEApplication.obj \
	$(SLO)$/KDEXLib.obj \
	$(SLO)$/KDESalDisplay.obj \
	$(SLO)$/KDESalFrame.obj \
	$(SLO)$/KDESalGraphics.obj \
	$(SLO)$/KDESalInstance.obj \
	$(SLO)$/KDEData.obj
	

.ELSE # "$(ENABLE_KDE4)" != ""

dummy:
	@echo KDE disabled - nothing to build
.ENDIF
.ENDIF		# "$(GUIBASE)"!="unx"

# --- Targets ------------------------------------------------------

.INCLUDE :  target.mk

.INCLUDE :  $(PRJ)$/util$/target.pmk
