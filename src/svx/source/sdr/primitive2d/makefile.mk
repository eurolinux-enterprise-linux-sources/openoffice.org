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
PRJNAME=svx
TARGET=primitive2d
ENABLE_EXCEPTIONS=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :  $(PRJ)$/util$/makefile.pmk

# --- Files --------------------------------------------------------

SLOFILES=\
		$(SLO)$/primitivefactory2d.obj				\
		$(SLO)$/sdrdecompositiontools.obj			\
		$(SLO)$/sdrattributecreator.obj				\
		$(SLO)$/sdrellipseprimitive2d.obj			\
		$(SLO)$/sdrrectangleprimitive2d.obj			\
		$(SLO)$/sdrcustomshapeprimitive2d.obj		\
		$(SLO)$/sdrcaptionprimitive2d.obj			\
		$(SLO)$/sdrgrafprimitive2d.obj				\
		$(SLO)$/sdrole2primitive2d.obj				\
		$(SLO)$/sdrolecontentprimitive2d.obj		\
		$(SLO)$/sdrpathprimitive2d.obj				\
		$(SLO)$/sdrprimitivetools.obj				\
		$(SLO)$/sdrmeasureprimitive2d.obj			\
		$(SLO)$/sdrconnectorprimitive2d.obj			\
		$(SLO)$/sdrtextprimitive2d.obj

.INCLUDE :  target.mk
