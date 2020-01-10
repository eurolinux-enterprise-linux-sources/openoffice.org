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

PRJNAME=svx

TARGET=gengal
TARGETTYPE=GUI
LIBTARGET=NO
ENABLE_EXCEPTIONS=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :  $(PRJ)$/util$/makefile.pmk

# --- Files --------------------------------------------------------

SCRIPTFILES =	$(BIN)$/gengal

OBJFILES=	$(OBJ)$/gengal.obj

.IF "$(GUI)"=="WNT" || "$(GUI)"=="OS2"
APP1TARGET= $(TARGET)
.ELSE			# .IF "$(GUI)"=="WNT" || "$(GUI)"=="OS2"
APP1TARGET= $(TARGET).bin
.ENDIF			# .IF "$(GUI)"=="WNT" || "$(GUI)"=="OS2"

APP1OBJS=   $(OBJFILES)

APP1STDLIBS=$(TOOLSLIB) 		\
			$(SO2LIB)			\
			$(SVLLIB)			\
			$(COMPHELPERLIB)		\
			$(CPPULIB)		\
			$(CPPUHELPERLIB)		\
			$(SALLIB)		\
			$(VCLLIB)			\
			$(UCBHELPERLIB)		\
			$(SVXCORELIB)

#.IF "$(COM)"=="GCC"
#ADDOPTFILES=$(OBJ)$/gengal.obj
#add_cflagscxx="-frtti -fexceptions"
#.ENDIF

# --- Targets ------------------------------------------------------

.INCLUDE :	target.mk

ALLTAR : \
	$(SCRIPTFILES)\
	$(BIN)/gengalrc

$(SCRIPTFILES) : $$(@:f:+".sh")
	@tr -d "\015" < $(@:f:+".sh") > $@

$(BIN)/%: %.in
	cp $< $@

