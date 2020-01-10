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
PRJINC=..$/..
PRJNAME=connectivity
TARGET=adabas

ENABLE_EXCEPTIONS=TRUE
VISIBILITY_HIDDEN=TRUE

# --- Settings ----------------------------------
.IF "$(DBGUTIL_OJ)"!=""
ENVCFLAGS+=/FR$(SLO)$/
.ENDIF

.INCLUDE : $(PRJ)$/makefile.pmk
.INCLUDE : $(PRJ)$/version.mk

.IF "$(SYSTEM_ODBC_HEADERS)" == "YES"
CFLAGS+=-DSYSTEM_ODBC_HEADERS
.ENDIF

# --- Files -------------------------------------

SLOFILES=\
		$(SLO)$/BFunctions.obj					\
		$(SLO)$/BConnection.obj					\
		$(SLO)$/BDriver.obj						\
		$(SLO)$/BCatalog.obj					\
		$(SLO)$/BGroups.obj						\
		$(SLO)$/BGroup.obj						\
		$(SLO)$/BUser.obj						\
		$(SLO)$/BUsers.obj						\
		$(SLO)$/BKeys.obj						\
		$(SLO)$/BColumns.obj					\
		$(SLO)$/BIndex.obj						\
		$(SLO)$/BIndexColumns.obj				\
		$(SLO)$/BIndexes.obj					\
		$(SLO)$/BTable.obj						\
		$(SLO)$/BTables.obj						\
		$(SLO)$/BViews.obj						\
		$(SLO)$/Bservices.obj					\
		$(SLO)$/BDatabaseMetaData.obj			\
        $(SLO)$/BPreparedStatement.obj          \
        $(SLO)$/BStatement.obj                  \
		$(SLO)$/BResultSetMetaData.obj			\
        $(SLO)$/BResultSet.obj

SHL1VERSIONMAP=$(TARGET).map

# --- Library -----------------------------------

SHL1TARGET=	$(TARGET)$(DLLPOSTFIX)
SHL1OBJS=$(SLOFILES)
SHL1STDLIBS=\
	$(CPPULIB)					\
	$(CPPUHELPERLIB)			\
	$(VOSLIB)					\
	$(SALLIB)					\
	$(DBTOOLSLIB)				\
	$(TOOLSLIB)					\
	$(ODBCBASELIB)				\
	$(UNOTOOLSLIB)				\
	$(COMPHELPERLIB)

.IF "$(ODBCBASELIB)" == ""
SHL1STDLIBS+=$(ODBCBASELIB)
.ENDIF

SHL1DEPN=
SHL1IMPLIB=	i$(SHL1TARGET)

SHL1DEF=	$(MISC)$/$(SHL1TARGET).def

DEF1NAME=	$(SHL1TARGET)
DEF1EXPORTFILE=	exports.dxp

# --- Targets ----------------------------------


.INCLUDE : $(PRJ)$/target.pmk
