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

PRJNAME=            framework
TARGET=             fwk_dispatch
USE_DEFFILE=        TRUE
ENABLE_EXCEPTIONS=  TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :          settings.mk

# --- defines ------------------------------------------------------

CDEFS+=-DCOMPMOD_NAMESPACE=framework

# --- Generate -----------------------------------------------------

SLOFILES=\
	$(SLO)$/closedispatcher.obj                 \
	$(SLO)$/dispatchinformationprovider.obj     \
	$(SLO)$/dispatchprovider.obj                \
	$(SLO)$/helpagentdispatcher.obj             \
	$(SLO)$/interaction.obj                     \
	$(SLO)$/interceptionhelper.obj              \
	$(SLO)$/loaddispatcher.obj                  \
	$(SLO)$/mailtodispatcher.obj                \
	$(SLO)$/menudispatcher.obj                  \
	$(SLO)$/oxt_handler.obj                     \
	$(SLO)$/popupmenudispatcher.obj             \
	$(SLO)$/servicehandler.obj                  \
	$(SLO)$/systemexec.obj		                \
	$(SLO)$/windowcommanddispatch.obj           \
	$(SLO)$/startmoduledispatcher.obj

# --- Targets ------------------------------------------------------

.INCLUDE :          target.mk
