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

PRJNAME=idlc
TARGET=idlcpp
TARGETTYPE=CUI

# --- Settings -----------------------------------------------------

NO_DEFAULT_STL=TRUE
LIBSALCPPRT=$(0)

.INCLUDE :  settings.mk

# --- Files --------------------------------------------------------

OBJFILES=   \
			$(OBJ)$/cpp.obj	\
			$(OBJ)$/eval.obj	\
			$(OBJ)$/getopt.obj	\
			$(OBJ)$/include.obj	\
			$(OBJ)$/lex.obj	\
			$(OBJ)$/macro.obj	\
			$(OBJ)$/nlist.obj	\
			$(OBJ)$/tokens.obj	\
			$(OBJ)$/unix.obj

# --- CPP -------------------------------------------------------

APP1TARGET= $(TARGET)
APP1RPATH=SDK

.IF "$(GUI)" != "UNX"
.IF "$(COM)" != "GCC"
APP1OBJS=$(OBJ)$/cpp.obj
.ENDIF
.ENDIF

APP1LIBS= $(LB)$/idlcpp.lib

# --- Targets ------------------------------------------------------

.INCLUDE :  target.mk

