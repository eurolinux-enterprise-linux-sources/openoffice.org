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
TARGET=printer

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk

.IF "$(ENABLE_CUPS)" != ""
CDEFS += -DENABLE_CUPS
.ENDIF

# --- Files --------------------------------------------------------

.IF "$(GUIBASE)"=="aqua"

dummy:
	@echo "Nothing to build for GUIBASE $(GUIBASE)"

.ELSE		# "$(GUIBASE)"=="aqua"

SLOFILES=\
	$(SLO)$/ppdparser.obj   	    \
	$(SLO)$/printerinfomanager.obj	\
	$(SLO)$/jobdata.obj				\
	$(SLO)$/cupsmgr.obj

.ENDIF # GUIBASE = aqua

# --- Targets ------------------------------------------------------

.INCLUDE :  target.mk

XSALSETLIBNAME=$(DLLPRE)spa$(DLLPOSTFIX)$(DLLPOST)

$(INCCOM)$/rtsname.hxx:
	rm -f $(INCCOM)$/rtsname.hxx ; \
	echo "#define _XSALSET_LIBNAME "\"$(XSALSETLIBNAME)\" > $(INCCOM)$/rtsname.hxx

$(SLO)$/cupsmgr.obj : $(INCCOM)$/rtsname.hxx

