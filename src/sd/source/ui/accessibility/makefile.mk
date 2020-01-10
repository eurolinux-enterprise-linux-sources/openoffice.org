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

PROJECTPCH=sd
PROJECTPCHSOURCE=$(PRJ)$/util$/sd
PRJNAME=sd
TARGET=accessibility
ENABLE_EXCEPTIONS=TRUE
AUTOSEG=true
PRJINC=..$/slidesorter

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :  $(PRJ)$/util$/makefile.pmk

# --- Files --------------------------------------------------------

SLOFILES =      									\
	$(SLO)$/AccessibleDocumentViewBase.obj			\
	$(SLO)$/AccessibleDrawDocumentView.obj			\
	$(SLO)$/AccessibleOutlineView.obj				\
	$(SLO)$/AccessiblePresentationShape.obj			\
	$(SLO)$/AccessiblePresentationGraphicShape.obj	\
	$(SLO)$/AccessiblePresentationOLEShape.obj		\
	$(SLO)$/AccessibleViewForwarder.obj				\
	$(SLO)$/AccessibleOutlineEditSource.obj			\
	$(SLO)$/AccessiblePageShape.obj					\
	$(SLO)$/AccessibleScrollPanel.obj				\
	$(SLO)$/AccessibleSlideSorterView.obj			\
	$(SLO)$/AccessibleSlideSorterObject.obj			\
	$(SLO)$/AccessibleTaskPane.obj					\
	$(SLO)$/AccessibleTreeNode.obj					\
	$(SLO)$/SdShapeTypes.obj


EXCEPTIONSFILES= 

SRS2NAME = accessibility
SRC2FILES = accessibility.src

# --- Tagets -------------------------------------------------------

.INCLUDE :  target.mk

