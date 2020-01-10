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

PRJ = ..$/..$/..
TARGET  = ConnectivityTools
PRJNAME = connectivity
PACKAGE = connectivity$/tools

# --- Settings -----------------------------------------------------
.INCLUDE: settings.mk

.IF "$(SOLAR_JAVA)" == ""
all:
	@echo "Java not available. Build skipped"
.ELSE

.IF "$(BUILD_QADEVOOO)" == "YES"
#----- compile .java files -----------------------------------------

JARFILES        = ridl.jar unoil.jar jurt.jar juh.jar java_uno.jar OOoRunnerLight.jar
# Do not use $/ with the $(FIND) command as for W32-4nt this leads to a backslash
# in a posix command. In this special case use / instead of $/
.IF "$(GUI)"=="OS2"
JAVAFILES       := $(shell @ls ./*.java)
.ELSE
JAVAFILES       := $(shell @$(FIND) ./*.java)
.ENDIF
JAVACLASSFILES	= $(foreach,i,$(JAVAFILES) $(CLASSDIR)$/$(PACKAGE)$/$(i:b).class)

#----- make a jar from compiled files ------------------------------

MAXLINELENGTH = 100000

JARCLASSDIRS    = $(PACKAGE)
JARTARGET       = $(TARGET).jar
JARCOMPRESS 	= TRUE

# --- Targets ------------------------------------------------------

.IF "$(depend)" == ""
ALL :   ALLTAR
.ELSE
ALL: 	ALLDEP
.ENDIF

.ENDIF

.ENDIF # "$(SOLAR_JAVA)" == ""

.INCLUDE :  target.mk
