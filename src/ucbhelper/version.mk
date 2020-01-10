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
UCBHELPER_TARGET=ucbhelper

# the major
UCBHELPER_MAJOR=4
# the minor
UCBHELPER_MINOR=0
# the micro
UCBHELPER_MICRO=0

# this is a c++ compatible library
UCBHELPER_CPP=1

UCBHELPER=$(UCBHELPER_TARGET)_$(CMPEXT)

LIBUCBHELPER_UNX=lib$(UCBHELPER).a.$(UCBHELPER_MAJOR)
LIBUCBHELPER_WIN=$(UCBHELPER_TARGET)$(UCBHELPER_MAJOR)$(CMPEXT).dll
