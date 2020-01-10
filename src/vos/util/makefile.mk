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

RTLLIB=irtl.lib

PRJ=..

PRJNAME=vos
TARGET=vos
TARGETTYPE=CUI

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :  ..$/version.mk

# --- Files --------------------------------------------------------

SHL1TARGET=$(VOS_TARGET)$(VOS_MAJOR)$(COMID)
SHL1IMPLIB=i$(TARGET)

.IF "$(GUI)"=="WNT"
SHL1STDLIBS=$(WSOCK32LIB) $(SALLIB)
.ELSE
SHL1STDLIBS=$(SALLIB)
.ENDIF

SHL1LIBS=    $(SLB)$/cpp$(TARGET).lib
.IF "$(GUI)" != "UNX"
.IF "$(COM)" != "GCC"
SHL1OBJS=    \
    $(SLO)$/object.obj
.ENDIF
.ENDIF

SHL1DEPN=
SHL1DEF=    $(MISC)$/$(SHL1TARGET).def

DEF1NAME    =$(SHL1TARGET)
DEF1DEPN    =$(MISC)$/$(SHL1TARGET).flt
DEFLIB1NAME =cppvos

# --- Targets ------------------------------------------------------

.INCLUDE :  target.mk

$(MISC)$/$(SHL1TARGET).flt:
    @echo ------------------------------
    @echo Making: $@
    @echo WEP > $@
    @echo LIBMAIN >> $@
    @echo LibMain >> $@
    @echo _alloc >> $@
    @echo alloc >> $@
    @echo _CT >> $@
    @echo _TI2 >> $@
    @echo _TI1 >> $@
    @echo exception::exception >> $@
    @echo @std@ >> $@
    @echo __>>$@

