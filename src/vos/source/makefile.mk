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

PRJ=..

PRJNAME=vos
TARGET=cppvos
ENABLE_EXCEPTIONS=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk

# --- Files --------------------------------------------------------

SLOFILES=       $(SLO)$/conditn.obj     \
				$(SLO)$/mutex.obj       \
				$(SLO)$/object.obj      \
				$(SLO)$/refernce.obj    \
				$(SLO)$/socket.obj      \
				$(SLO)$/thread.obj      \
				$(SLO)$/stream.obj      \
				$(SLO)$/module.obj      \
				$(SLO)$/timer.obj       \
				$(SLO)$/process.obj     \
				$(SLO)$/security.obj    \
				$(SLO)$/signal.obj      \
				$(SLO)$/pipe.obj        \
				$(SLO)$/xception.obj


.IF "$(UPDATER)"=="YES"
OBJFILES=       $(OBJ)$/conditn.obj     \
				$(OBJ)$/mutex.obj       \
				$(OBJ)$/object.obj      \
				$(OBJ)$/refernce.obj    \
				$(OBJ)$/socket.obj      \
				$(OBJ)$/thread.obj      \
				$(OBJ)$/stream.obj      \
				$(OBJ)$/module.obj      \
				$(OBJ)$/timer.obj       \
				$(OBJ)$/process.obj     \
				$(OBJ)$/security.obj    \
				$(OBJ)$/signal.obj      \
				$(OBJ)$/pipe.obj        \
				$(OBJ)$/xception.obj

.ENDIF

# --- Targets ------------------------------------------------------

.INCLUDE :  target.mk



