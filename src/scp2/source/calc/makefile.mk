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

PRJPCH=

PRJNAME=scp2
TARGET=calc
TARGETTYPE=CUI

# --- Settings -----------------------------------------------------

.INCLUDE :	settings.mk

.IF "$(SYSTEM_LPSOLVE)" == "YES"
SCPDEFS+=-DSYSTEM_LPSOLVE
.ENDIF

SCP_PRODUCT_TYPE=osl
PARFILES= \
        module_calc.par              \
        file_calc.par 

.IF "$(GUI)"=="WNT"
PARFILES += \
        registryitem_calc.par        \
        folderitem_calc.par
.ENDIF

ULFFILES= \
        module_calc.ulf              \
        registryitem_calc.ulf        \
        folderitem_calc.ulf 

# --- File ---------------------------------------------------------
.INCLUDE :  target.mk
