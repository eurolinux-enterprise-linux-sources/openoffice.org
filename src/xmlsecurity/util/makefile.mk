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

PRJ=..

PRJNAME=xmlsecurity
TARGET=xmlsecurity

# Disable '-z defs' due to broken libxpcom.
#LINKFLAGSDEFS=$(0)
USE_DEFFILE=TRUE

# --- Settings -----------------------------------------------------

.INCLUDE :  settings.mk
.INCLUDE :	$(PRJ)$/util$/target.pmk

.IF "$(WITH_MOZILLA)" == "NO"
@all:
	@echo "No mozilla -> no nss -> no libxmlsec -> no xmlsecurity..."
.ENDIF

# --- Files --------------------------------------------------------

BMP_IN=$(PRJ)$/res

# --- Shared-Library -----------------------------------------------

#
# The 1st shared library
#
SHL1NAME=xsec_fw
SHL1TARGET= $(SHL1NAME)
SHL1LIBS= $(SLB)$/fw.lib

SHL1STDLIBS +=		\
	$(SALLIB)		\
	$(CPPULIB)		\
	$(CPPUHELPERLIB)

SHL1IMPLIB = $(SHL1TARGET)
SHL1DEF = $(MISC)$/$(SHL1TARGET).def
DEF1NAME = $(SHL1TARGET)
DEF1EXPORTFILE = xsec_fw.dxp

#
# The 2nd shared library
#
SHL2NAME=xsec_xmlsec
SHL2TARGET= $(SHL2NAME)
SHL2LIBS= \
	$(SLB)$/xs_comm.lib

.IF "$(CRYPTO_ENGINE)" == "mscrypto"
SHL2LIBS += \
	$(SLB)$/xs_mscrypt.lib
.ELSE
SHL2LIBS += \
	$(SLB)$/xs_nss.lib
.ENDIF

SHL2STDLIBS +=			\
	$(SALLIB)			\
	$(CPPULIB)			\
	$(CPPUHELPERLIB)	\
	$(SALLIB)	\
	$(SVLLIB)			\
	$(TOOLSLIB)			\
	$(COMPHELPERLIB)	\
	$(CPPUHELPERLIB)	\
	$(XMLOFFLIB)        

.IF "$(OS)"=="SOLARIS"
SHL2STDLIBS +=-ldl
.ENDIF

.IF "$(SYSTEM_MOZILLA)" == "YES"
.IF "$(NSPR_LIB)" != ""
SHL2STDLIBS += $(NSPR_LIB)
.ENDIF
.IF "$(NSS_LIB)" != ""
SHL2STDLIBS += $(NSS_LIB)
.ENDIF
.ENDIF

.IF "$(CRYPTO_ENGINE)" == "mscrypto"
SHL2STDLIBS+= $(MSCRYPTOLIBS)
.ELSE
SHL2STDLIBS+= $(NSSCRYPTOLIBS)
.ENDIF

SHL2IMPLIB = $(SHL2TARGET)
SHL2DEF = $(MISC)$/$(SHL2TARGET).def
DEF2NAME = $(SHL2TARGET)
.IF "$(CRYPTO_ENGINE)" == "mscrypto"
DEF2EXPORTFILE = exports_xsmscrypt.dxp
.ELSE
DEF2EXPORTFILE = exports_xsnss.dxp
.ENDIF

SRSFILELIST=	\
                $(SRS)$/component.srs   \
                $(SRS)$/dialogs.srs

RESLIB1NAME=xmlsec
RESLIB1IMAGES=$(PRJ)$/res
RESLIB1SRSFILES= $(SRSFILELIST)

SHL4TARGET=$(TARGET)
SHL4LIBS=\
                $(SLB)$/helper.lib      \
                $(SLB)$/dialogs.lib     \
                $(SLB)$/component.lib

SHL4STDLIBS=\
                $(CPPULIB)			\
				$(CPPUHELPERLIB)	\
				$(COMPHELPERLIB)	\
				$(UCBHELPERLIB)	    \
				$(UNOTOOLSLIB)	    \
				$(VCLLIB)			\
				$(TOOLSLIB) 		\
				$(SVTOOLLIB) 		\
				$(SALLIB)			\
				$(SVLLIB)			\
				$(XMLOFFLIB)		\
				$(SVXCORELIB)

SHL4VERSIONMAP = xmlsecurity.map
SHL4DEPN=
SHL4IMPLIB=i$(TARGET)
SHL4DEF=$(MISC)$/$(SHL4TARGET).def
DEF4NAME=$(SHL4TARGET)

# --- Targets ----------------------------------------------------------

.INCLUDE :  target.mk

# --- Filter -----------------------------------------------------------

$(MISC)$/$(SHL3TARGET).flt: makefile.mk
	$(TYPE) $(SHL3TARGET).flt > $@
