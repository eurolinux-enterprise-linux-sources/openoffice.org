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

PRJNAME=svx
TARGET=accessibility
LIBTARGET=NO
ENABLE_EXCEPTIONS=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :  $(PRJ)$/util$/makefile.pmk

# --- Files --------------------------------------------------------

LIB1TARGET= $(SLB)$/$(TARGET)-core.lib
LIB1OBJFILES= \
	$(SLO)$/AccessibleStringWrap.obj

LIB2TARGET= $(SLB)$/$(TARGET).lib
LIB2OBJFILES= \
	$(SLO)$/charmapacc.obj						\
	$(SLO)$/svxrectctaccessiblecontext.obj		\
	$(SLO)$/GraphCtlAccessibleContext.obj		\
	$(SLO)$/ChildrenManager.obj 				\
	$(SLO)$/ChildrenManagerImpl.obj 			\
	$(SLO)$/DescriptionGenerator.obj 			\
	$(SLO)$/AccessibleContextBase.obj			\
	$(SLO)$/AccessibleComponentBase.obj			\
	$(SLO)$/AccessibleSelectionBase.obj			\
	$(SLO)$/AccessibleShape.obj					\
	$(SLO)$/AccessibleGraphicShape.obj			\
	$(SLO)$/AccessibleOLEShape.obj				\
	$(SLO)$/AccessibleShapeInfo.obj				\
	$(SLO)$/AccessibleShapeTreeInfo.obj			\
	$(SLO)$/AccessibleTextHelper.obj			\
	$(SLO)$/AccessibleEmptyEditSource.obj		\
	$(SLO)$/AccessibleTextEventQueue.obj		\
	$(SLO)$/AccessibleStaticTextBase.obj		\
	$(SLO)$/AccessibleParaManager.obj			\
	$(SLO)$/AccessibleEditableTextPara.obj		\
	$(SLO)$/AccessibleImageBullet.obj			\
	$(SLO)$/ShapeTypeHandler.obj				\
	$(SLO)$/SvxShapeTypes.obj					\
	$(SLO)$/AccessibleControlShape.obj			\
	$(SLO)$/DGColorNameLookUp.obj				\
	$(SLO)$/AccessibleFrameSelector.obj
	
SLOFILES = $(LIB1OBJFILES) $(LIB2OBJFILES)

SRS2NAME = accessibility
SRC2FILES = accessibility.src

# --- Tagets -------------------------------------------------------

.INCLUDE :  target.mk

