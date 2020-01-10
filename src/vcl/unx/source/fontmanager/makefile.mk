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

ENABLE_EXCEPTIONS=TRUE
PRJNAME=vcl
TARGET=fontman

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk

CFLAGS+= -I..$/fontsubset
INCDEPN+= -I..$/fontsubset

.IF "$(ENABLE_FONTCONFIG)" != ""
CDEFS += -DENABLE_FONTCONFIG
.ENDIF

CFLAGS+=$(FREETYPE_CFLAGS)


# --- Files --------------------------------------------------------

.IF "$(GUIBASE)"=="aqua"

dummy:
	@echo "Nothing to build for GUIBASE $(GUIBASE)"

.ELSE		# "$(GUIBASE)"=="aqua"

SLOFILES=\
	$(SLO)$/fontmanager.obj		\
	$(SLO)$/fontcache.obj		\
	$(SLO)$/fontconfig.obj		\
	$(SLO)$/helper.obj    		\
	$(SLO)$/parseAFM.obj

.IF "$(OS)$(CPU)"=="SOLARISI"
NOOPTFILES=$(SLO)$/fontmanager.obj
.ENDIF

.ENDIF

# --- Targets ------------------------------------------------------

.INCLUDE :  target.mk
