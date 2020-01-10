#*************************************************************************
#*
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
#************************************************************************
PRJ=..$/..$/..

PRJNAME=i18npool
TARGET=dict
LIBTARGET=NO

# --- Settings -----------------------------------------------------
.INCLUDE :  settings.mk

# Fix heap limit problem on MSC
.IF "$(OS)" == "WNT"
.IF  "$(COMEX)" != "8"
CDEFS+=-Zm300
.ENDIF
.ENDIF

# --- Files --------------------------------------------------------
# Japanese dictionary
SHL1TARGET=dict_ja
SHL1IMPLIB=i$(SHL1TARGET)

SHL1VERSIONMAP=$(TARGET).map
SHL1DEF=$(MISC)$/$(SHL1TARGET).def
DEF1NAME=$(SHL1TARGET)

SHL1OBJS= \
	$(SLO)$/dict_ja.obj

LIB1TARGET=	$(SLB)$/$(SHL1TARGET).lib
LIB1OBJFILES=$(SHL1OBJS)

# Chinese dictionary
SHL2TARGET=dict_zh
SHL2IMPLIB=i$(SHL2TARGET)

SHL2VERSIONMAP=$(TARGET).map
SHL2DEF=$(MISC)$/$(SHL2TARGET).def
DEF2NAME=$(SHL2TARGET)

SHL2OBJS= \
	$(SLO)$/dict_zh.obj

LIB2TARGET=	$(SLB)$/$(SHL2TARGET).lib
LIB2OBJFILES=$(SHL2OBJS)

DEPOBJFILES= \
	$(SLO1FILES) \
	$(SLO2FILES)

# --- Targets ------------------------------------------------------
.INCLUDE :  target.mk

$(MISC)$/dict_%.cxx : %.dic
	$(AUGMENT_LIBRARY_PATH) $(BIN)$/gendict $< $@

# ugly - is this dependency really required here?
$(foreach,i,$(shell @$(FIND) . -name "*.dic") $(MISC)$/dict_$(i:b).cxx) : $(BIN)$/gendict$(EXECPOST)

