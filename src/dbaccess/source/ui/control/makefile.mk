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
PRJINC=$(PRJ)$/source
PRJNAME=dbaccess
TARGET=uicontrols

# --- Settings ----------------------------------

.INCLUDE : settings.mk
.INCLUDE : $(PRJ)$/util$/makefile.pmk

# --- Files -------------------------------------

# ... resource files ............................

SRS1NAME=$(TARGET)
SRC1FILES =	\
		TableGrantCtrl.src	\
		undosqledit.src		\
		tabletree.src		

# ... exception files .........................

EXCEPTIONSFILES=\
		$(SLO)$/statusbarontroller.obj	\
		$(SLO)$/RelationControl.obj		\
		$(SLO)$/toolboxcontroller.obj	\
		$(SLO)$/tabletree.obj			\
		$(SLO)$/TableGrantCtrl.obj		\
		$(SLO)$/dbtreelistbox.obj       \
		$(SLO)$/sqledit.obj				\
		$(SLO)$/ColumnControlWindow.obj	\
		$(SLO)$/FieldDescControl.obj    \
		$(SLO)$/opendoccontrols.obj

# ... object files ............................

SLOFILES=	\
		$(EXCEPTIONSFILES)				\
		$(SLO)$/ScrollHelper.obj		\
		$(SLO)$/VertSplitView.obj		\
		$(SLO)$/SqlNameEdit.obj			\
		$(SLO)$/listviewitems.obj		\
		$(SLO)$/undosqledit.obj			\
		$(SLO)$/marktree.obj			\
		$(SLO)$/curledit.obj            \
		$(SLO)$/charsetlistbox.obj

# --- Targets ----------------------------------

.INCLUDE : target.mk

