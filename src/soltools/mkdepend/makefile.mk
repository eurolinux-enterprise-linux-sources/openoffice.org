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

PRJNAME=soltools
TARGET=make_makedepend
PRJ=..
TARGETTYPE=CUI
LIBTARGET=NO
# noadjust here to have dependencies over there
noadjust=TRUE
nodep=true

# "mkdepend" is written in K&R style C. Modern compilers will generate
# lots of warning. There is no point in cleaning this up, so we just
# ignore warnings
EXTERNAL_WARNINGS_NOT_ERRORS=TRUE

.INCLUDE : $(PRJ)$/util$/makefile.pmk
.INCLUDE : settings.mk

LIBSALCPPRT=
UWINAPILIB=

CDEFS+=-DNO_X11 -DXP_PC -DHW_THREADS

OBJFILES=  \
		$(OBJ)$/cppsetup.obj \
		$(OBJ)$/ifparser.obj \
		$(OBJ)$/include.obj \
		$(OBJ)$/main.obj \
		$(OBJ)$/parse.obj \
		$(OBJ)$/pr.obj \
		$(OBJ)$/collectdircontent.obj \
		$(NULL)

APP1TARGET=makedepend
APP1OBJS=$(OBJFILES)
APP1RPATH=NONE

.INCLUDE : target.mk


