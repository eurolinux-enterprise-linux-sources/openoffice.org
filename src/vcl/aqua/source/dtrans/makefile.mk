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

PRJNAME=vcl
TARGET=dtransaqua
ENABLE_EXCEPTIONS=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk

# ------------------------------------------------------------------

.IF "$(OS)"!="MACOSX"
dummy:
	@echo "Nothing to build for this platform"
.ELSE # "$(OS)"!="MACOSX"
.IF "$(GUIBASE)"!="aqua"
dummy:
	@echo "Nothing to build for GUIBASE $(GUIBASE)"
.ELSE

CFLAGSCXX+=-fconstant-cfstrings -x objective-c++ -fobjc-exceptions

SLOFILES= \
		$(SLO)$/aqua_clipboard.obj \
		$(SLO)$/DataFlavorMapping.obj \
		$(SLO)$/OSXTransferable.obj \
		$(SLO)$/HtmlFmtFlt.obj \
		$(SLO)$/PictToBmpFlt.obj \
		$(SLO)$/DropTarget.obj \
		$(SLO)$/DragSource.obj \
		$(SLO)$/service_entry.obj \
		$(SLO)$/DragSourceContext.obj \
		$(SLO)$/DragActionConversion.obj

# --- Targets ------------------------------------------------------
.INCLUDE :	target.mk

.ENDIF		# "$(GUIBASE)"!="aqua"
.ENDIF		# "$(OS)"!="MACOSX"

