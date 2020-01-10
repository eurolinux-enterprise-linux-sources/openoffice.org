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

PRJNAME=basic
TARGET=sbx

# --- Settings -----------------------------------------------------------

.INCLUDE :  settings.mk


# --- Allgemein -----------------------------------------------------------

SRS1NAME=$(TARGET)
SRC1FILES=	format.src

SLOFILES=	\
	$(SLO)$/sbxbase.obj	\
	$(SLO)$/sbxres.obj  	\
	$(SLO)$/sbxvalue.obj	\
	$(SLO)$/sbxvals.obj	\
	$(SLO)$/sbxvar.obj  	\
	$(SLO)$/sbxarray.obj	\
	$(SLO)$/sbxobj.obj	\
	$(SLO)$/sbxcoll.obj	\
	$(SLO)$/sbxexec.obj	\
	$(SLO)$/sbxint.obj 	\
	$(SLO)$/sbxlng.obj 	\
	$(SLO)$/sbxsng.obj 	\
	$(SLO)$/sbxmstrm.obj	\
	$(SLO)$/sbxdbl.obj 	\
	$(SLO)$/sbxcurr.obj 	\
	$(SLO)$/sbxdate.obj	\
	$(SLO)$/sbxstr.obj 	\
	$(SLO)$/sbxbool.obj	\
	$(SLO)$/sbxchar.obj	\
	$(SLO)$/sbxbyte.obj	\
	$(SLO)$/sbxuint.obj	\
	$(SLO)$/sbxulng.obj	\
	$(SLO)$/sbxform.obj	\
	$(SLO)$/sbxscan.obj 	\
	$(SLO)$/sbxdec.obj


EXCEPTIONSFILES=$(SLO)$/sbxarray.obj

# --- Targets -------------------------------------------------------------

.INCLUDE :  target.mk


