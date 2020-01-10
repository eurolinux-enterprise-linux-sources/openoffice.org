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

PRJNAME=svtools
TARGET=browse

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :  $(PRJ)$/util$/svt.pmk

# --- Files --------------------------------------------------------

SRS1NAME=$(TARGET)
SRC1FILES=		editbrowsebox.src

EXCEPTIONSFILES =\
			$(SLO)$/editbrowsebox2.obj		\
            $(SLO)$/brwbox1.obj				\
			$(SLO)$/brwbox3.obj

SLOFILES=   \
			$(EXCEPTIONSFILES)			\
            $(SLO)$/ebbcontrols.obj		\
            $(SLO)$/editbrowsebox.obj	\
            $(SLO)$/brwbox2.obj			\
            $(SLO)$/brwhead.obj			\
            $(SLO)$/datwin.obj


# --- Targets -------------------------------------------------------

.INCLUDE :  target.mk
