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
TARGET=properties
ENABLE_EXCEPTIONS=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :  $(PRJ)$/util$/makefile.pmk

# --- Files --------------------------------------------------------

SLOFILES=\
		$(SLO)$/properties.obj				\
		$(SLO)$/emptyproperties.obj			\
		$(SLO)$/pageproperties.obj			\
		$(SLO)$/defaultproperties.obj		\
		$(SLO)$/attributeproperties.obj		\
		$(SLO)$/textproperties.obj			\
		$(SLO)$/customshapeproperties.obj	\
		$(SLO)$/rectangleproperties.obj		\
		$(SLO)$/captionproperties.obj		\
		$(SLO)$/circleproperties.obj		\
		$(SLO)$/connectorproperties.obj		\
		$(SLO)$/e3dproperties.obj			\
		$(SLO)$/e3dcompoundproperties.obj	\
		$(SLO)$/e3dextrudeproperties.obj	\
		$(SLO)$/e3dlatheproperties.obj		\
		$(SLO)$/e3dsceneproperties.obj		\
		$(SLO)$/e3dsphereproperties.obj		\
		$(SLO)$/graphicproperties.obj		\
		$(SLO)$/groupproperties.obj			\
		$(SLO)$/measureproperties.obj		\
		$(SLO)$/itemsettools.obj

.INCLUDE :  target.mk
