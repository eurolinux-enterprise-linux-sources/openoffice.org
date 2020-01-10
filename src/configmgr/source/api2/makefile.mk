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
PRJINC=$(PRJ)$/source$/inc
PRJNAME=configmgr
TARGET=api2

ENABLE_EXCEPTIONS=TRUE

# --- Settings ----------------------------------

.INCLUDE : settings.mk
.INCLUDE : $(PRJ)$/makefile.pmk

# --- Files -------------------------------------

SLOFILES=	\
		$(SLO)$/broadcaster.obj		\
		$(SLO)$/listenercontainer.obj		\
		$(SLO)$/provider.obj		\
		$(SLO)$/providerimpl.obj	\
		$(SLO)$/accessimpl.obj		\
		$(SLO)$/apiaccessobj.obj	\
	    $(SLO)$/apiserviceinfo.obj	\
		$(SLO)$/apifactory.obj	\
		$(SLO)$/apifactoryimpl.obj	\
		$(SLO)$/apinodeaccess.obj	\
		$(SLO)$/apinodeupdate.obj	\
		$(SLO)$/apinotifierimpl.obj	\
		$(SLO)$/apitreeaccess.obj	\
		$(SLO)$/apitreeimplobj.obj	\
		$(SLO)$/confignotifier.obj	\
		$(SLO)$/committer.obj	\
		$(SLO)$/elementaccess.obj	\
		$(SLO)$/elementimpl.obj	\
		$(SLO)$/groupaccess.obj	\
		$(SLO)$/groupobjects.obj	\
		$(SLO)$/groupupdate.obj	\
		$(SLO)$/propertiesfilterednotifier.obj	\
		$(SLO)$/propertyinfohelper.obj	\
		$(SLO)$/propertysetaccess.obj	\
		$(SLO)$/propsetaccessimpl.obj	\
		$(SLO)$/setaccess.obj		\
		$(SLO)$/setobjects.obj		\
		$(SLO)$/setupdate.obj		\
		$(SLO)$/translatechanges.obj	\
		$(SLO)$/treeiterators.obj \
		$(SLO)$/updateimpl.obj		\


# --- Targets ----------------------------------

.INCLUDE : target.mk

