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
TARGET=customshapes
LIBTARGET=NO
ENABLE_EXCEPTIONS=TRUE

# --- Settings ----------------------------------

.INCLUDE :  	settings.mk
.INCLUDE :  $(PRJ)$/util$/makefile.pmk

# --- Types -------------------------------------

# Disable optimization for SunCC SPARC and MACOSX (funny loops
# when parsing e.g. "x+width/2"),
# also http://gcc.gnu.org/PR22392
.IF ("$(OS)$(CPU)"=="SOLARISS" && "$(COM)"!="GCC") || "$(OS)"=="MACOSX" || ("$(OS)"=="LINUX" && "$(CPU)"=="P") 
NOOPTFILES= $(SLO)$/EnhancedCustomShapeFunctionParser.obj
.ENDIF

ENVCFLAGS += -DBOOST_SPIRIT_USE_OLD_NAMESPACE

# --- Files -------------------------------------

LIB1TARGET= $(SLB)$/$(TARGET)-core.lib
LIB1OBJFILES= \
			$(SLO)$/EnhancedCustomShapeTypeNames.obj		\
			$(SLO)$/EnhancedCustomShapeGeometry.obj			\
			$(SLO)$/EnhancedCustomShape2d.obj				\
			$(SLO)$/EnhancedCustomShapeFunctionParser.obj

LIB2TARGET= $(SLB)$/$(TARGET).lib
LIB2OBJFILES= \
			$(SLO)$/EnhancedCustomShapeEngine.obj			\
			$(SLO)$/EnhancedCustomShape3d.obj				\
			$(SLO)$/EnhancedCustomShapeFontWork.obj			\
			$(SLO)$/EnhancedCustomShapeHandle.obj			\
			$(SLO)$/tbxcustomshapes.obj

SLOFILES = $(LIB1OBJFILES) $(LIB2OBJFILES)

# --- Targets ----------------------------------

.INCLUDE : target.mk
