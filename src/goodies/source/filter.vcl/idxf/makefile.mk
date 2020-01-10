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

PRJNAME=goodies
TARGET=idxf
DEPTARGET=vidxf

# --- Settings -----------------------------------------------------------

.INCLUDE :  settings.mk

# --- Allgemein ----------------------------------------------------------

.IF "$(editdebug)"!="" || "$(EDITDEBUG)"!=""
CDEFS+= -DEDITDEBUG
.ENDIF
.IF "$(L10N_framework)"==""
EXCEPTIONSFILES=\
			$(SLO)$/dxfentrd.obj \
			$(SLO)$/dxf2mtf.obj

SLOFILES =  $(SLO)$/dxfgrprd.obj \
			$(SLO)$/dxfvec.obj   \
			$(SLO)$/dxfentrd.obj \
			$(SLO)$/dxfblkrd.obj \
			$(SLO)$/dxftblrd.obj \
			$(SLO)$/dxfreprd.obj \
			$(SLO)$/dxf2mtf.obj  \
			$(SLO)$/idxf.obj

# ==========================================================================

SHL1TARGET=     idx$(DLLPOSTFIX)
SHL1IMPLIB=     idxf
SHL1STDLIBS=    $(VCLLIB) $(TOOLSLIB) $(SALLIB)

SHL1LIBS=       $(SLB)$/idxf.lib # $(LB)$/rtftoken.lib

.IF "$(GUI)" != "UNX"
.IF "$(COM)" != "GCC"
SHL1OBJS=       $(SLO)$/idxf.obj
.ENDIF
.ENDIF

SHL1VERSIONMAP=exports.map
SHL1DEF=        $(MISC)$/$(SHL1TARGET).def

DEF1NAME=$(SHL1TARGET)
.ENDIF

# ==========================================================================

.INCLUDE :  target.mk
