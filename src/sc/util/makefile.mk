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

PRJNAME=sc
TARGET=scalc3
GEN_HID=TRUE
GEN_HID_OTHER=TRUE
USE_DEFFILE=TRUE

# --- Settings -----------------------------------------------------------

.INCLUDE :  settings.mk

.IF "$(OS)"=="IRIX"
LINKFLAGS+=-Wl,-LD_LAYOUT:lgot_buffer=30
.ENDIF

# --- Resourcen ----------------------------------------------------

RESLIB1LIST=\
	$(SRS)$/miscdlgs.srs	\
	$(SRS)$/docshell.srs	\
	$(SRS)$/ui.srs		\
	$(SRS)$/dbgui.srs	\
	$(SRS)$/drawfunc.srs \
	$(SRS)$/core.srs 	\
	$(SRS)$/styleui.srs	\
	$(SRS)$/formdlgs.srs \
	$(SRS)$/pagedlg.srs	\
	$(SRS)$/navipi.srs	\
	$(SOLARCOMMONRESDIR)$/sfx.srs

RESLIB1NAME=sc
RESLIB1IMAGES=\
	$(PRJ)$/res					\
	$(PRJ)$/res/imglst/apptbx	\
	$(PRJ)$/res/imglst/dbgui	\
	$(PRJ)$/res/imglst/navipi

RESLIB1SRSFILES=\
	$(RESLIB1LIST)

# --- StarClac DLL

SHL1TARGET= sc$(DLLPOSTFIX)
SHL1USE_EXPORTS=name
SHL1IMPLIB= sci

# dynamic libraries
SHL1STDLIBS=       \
	$(BASICLIB)	\
	$(SFXLIB)		\
	$(SVTOOLLIB)	\
	$(SVLLIB)		\
	$(SVXCORELIB)		\
	$(SVXLIB)		\
	$(GOODIESLIB)	\
    $(BASEGFXLIB) \
	$(DRAWINGLAYERLIB) \
	$(VCLLIB)		\
	$(CPPULIB)		\
	$(CPPUHELPERLIB)	\
	$(COMPHELPERLIB)	\
	$(UCBHELPERLIB)	\
	$(TKLIB)		\
	$(VOSLIB)		\
	$(SALLIB)		\
	$(TOOLSLIB)	\
	$(I18NISOLANGLIB) \
	$(UNOTOOLSLIB) \
	$(SOTLIB)		\
	$(XMLOFFLIB)	\
	$(AVMEDIALIB) \
	$(FORLIB) \
    $(FORUILIB)
	
SHL1LIBS=$(LIB3TARGET) $(LIB4TARGET)

SHL1DEF=$(MISC)$/$(SHL1TARGET).def
DEF1NAME=$(SHL1TARGET)
DEFLIB1NAME= $(LIB3TARGET:b) $(LIB4TARGET:b)

.IF "$(GUI)" == "WNT"
SHL1RES=    $(RCTARGET)
.ENDIF

# --- Linken der Applikation ---------------------------------------

LIB3TARGET=$(SLB)$/scalc3.lib
LIB3FILES=	\
	$(SLB)$/app.lib \
	$(SLB)$/docshell.lib \
	$(SLB)$/view.lib \
	$(SLB)$/undo.lib \
	$(SLB)$/attrdlg.lib \
	$(SLB)$/namedlg.lib \
	$(SLB)$/miscdlgs.lib \
	$(SLB)$/formdlgs.lib \
	$(SLB)$/cctrl.lib \
	$(SLB)$/dbgui.lib \
	$(SLB)$/pagedlg.lib \
	$(SLB)$/drawfunc.lib \
	$(SLB)$/navipi.lib

LIB3FILES+= \
			$(SLB)$/unoobj.lib


LIB4TARGET=$(SLB)$/scalc3c.lib
LIB4FILES=	\
	$(SLB)$/data.lib \
	$(SLB)$/tool.lib \
	$(SLB)$/xml.lib \
	$(SLB)$/accessibility.lib

SHL2TARGET= scd$(DLLPOSTFIX)
SHL2IMPLIB= scdimp
SHL2VERSIONMAP= scd.map
SHL2DEF=$(MISC)$/$(SHL2TARGET).def
DEF2NAME=		$(SHL2TARGET)

SHL2STDLIBS= \
			$(SFX2LIB) \
			$(SVTOOLLIB) \
			$(SVLLIB) \
			$(VCLLIB) \
			$(TOOLSLIB) \
			$(UCBHELPERLIB)	\
			$(CPPUHELPERLIB) \
			$(CPPULIB) \
			$(SOTLIB) \
			$(SALLIB)

SHL2OBJS=   $(SLO)$/scdetect.obj \
	    $(SLO)$/detreg.obj
SHL2DEPN+=	makefile.mk

# split out filters
SHL6TARGET= scfilt$(DLLPOSTFIX)
SHL6IMPLIB= scfiltimp
SHL6LIBS= \
	$(SLB)$/ftools.lib \
	$(SLB)$/excel.lib \
	$(SLB)$/xcl97.lib \
	$(SLB)$/lotus.lib \
	$(SLB)$/qpro.lib \
	$(SLB)$/dif.lib \
	$(SLB)$/html.lib \
	$(SLB)$/rtf.lib \
	$(SLB)$/scflt.lib
SHL6VERSIONMAP= scfilt.map
SHL6DEF=$(MISC)$/$(SHL6TARGET).def
DEF6NAME= $(SHL6TARGET)
SHL6DEPN=$(SHL1TARGETN)
SHL6STDLIBS= \
	$(ISCLIB) \
	$(BASICLIB)	\
	$(SFXLIB)		\
	$(SVTOOLLIB)	\
	$(SVLLIB)		\
	$(SVXCORELIB)		\
	$(SVXMSFILTERLIB)		\
	$(SVXLIB)		\
    $(BASEGFXLIB) \
	$(DRAWINGLAYERLIB) \
	$(VCLLIB)		\
	$(CPPULIB)		\
	$(CPPUHELPERLIB)	\
	$(COMPHELPERLIB)	\
	$(TKLIB)		\
	$(VOSLIB)		\
	$(SALLIB)		\
	$(TOOLSLIB)	\
	$(UNOTOOLSLIB) \
	$(SOTLIB)		\
	$(OOXLIB)       \
	$(SAXLIB) \
    $(FORLIB)

# add for scui
SHL8TARGET= scui$(DLLPOSTFIX)
SHL8IMPLIB= scuiimp
SHL8VERSIONMAP= scui.map
SHL8DEF=$(MISC)$/$(SHL8TARGET).def
SHL8DEPN=$(SHL1TARGETN)
DEF8NAME=$(SHL8TARGET)

SHL8STDLIBS= \
			$(ISCLIB) \
            $(SVXCORELIB) \
            $(SVXLIB) \
            $(SFX2LIB) \
            $(SVTOOLLIB) \
            $(VCLLIB) \
			$(SVLLIB) \
			$(SOTLIB) \
			$(UNOTOOLSLIB) \
            $(TOOLSLIB) \
            $(I18NISOLANGLIB) \
			$(COMPHELPERLIB) \
			$(CPPULIB) \
            $(SALLIB)

.IF "$(ENABLE_LAYOUT)" == "TRUE"
SHL8STDLIBS+=$(TKLIB)
.ENDIF # ENABLE_LAYOUT == TRUE

SHL8LIBS=   $(SLB)$/scui.lib
LIB8TARGET = $(SLB)$/scui.lib

LIB8FILES=$(SLB)$/styleui.lib	\
		$(SLB)$/optdlg.lib

LIB8OBJFILES = \
		$(SLO)$/scuiexp.obj     \
		$(SLO)$/scdlgfact.obj \
		$(SLO)$/tpsubt.obj		\
		$(SLO)$/tptable.obj	\
		$(SLO)$/tpstat.obj	\
		$(SLO)$/tabpages.obj	\
		$(SLO)$/tpsort.obj		\
		$(SLO)$/sortdlg.obj		\
		$(SLO)$/validate.obj	\
		$(SLO)$/textdlgs.obj		\
		$(SLO)$/subtdlg.obj		\
		$(SLO)$/tphf.obj		\
		$(SLO)$/scuitphfedit.obj	\
		$(SLO)$/hfedtdlg.obj	\
		$(SLO)$/attrdlg.obj	\
		$(SLO)$/scuiimoptdlg.obj	\
		$(SLO)$/strindlg.obj		\
		$(SLO)$/shtabdlg.obj		\
		$(SLO)$/scendlg.obj		\
		$(SLO)$/pvfundlg.obj	\
		$(SLO)$/pfiltdlg.obj	\
		$(SLO)$/namepast.obj		\
		$(SLO)$/namecrea.obj		\
		$(SLO)$/mvtabdlg.obj		\
		$(SLO)$/mtrindlg.obj		\
		$(SLO)$/linkarea.obj		\
		$(SLO)$/lbseldlg.obj		\
		$(SLO)$/instbdlg.obj		\
		$(SLO)$/inscodlg.obj		\
		$(SLO)$/inscldlg.obj		\
		$(SLO)$/groupdlg.obj		\
		$(SLO)$/filldlg.obj			\
		$(SLO)$/delcodlg.obj		\
		$(SLO)$/delcldlg.obj		\
		$(SLO)$/dapitype.obj	\
		$(SLO)$/dapidata.obj	\
		$(SLO)$/crdlg.obj			\
		$(SLO)$/scuiasciiopt.obj	\
		$(SLO)$/scuiautofmt.obj	\
	    $(SLO)$/dpgroupdlg.obj	\
		$(SLO)$/editfield.obj

.IF "$(ENABLE_VBA)"=="YES"

TARGET_VBA=vbaobj
SHL9TARGET=$(TARGET_VBA)$(DLLPOSTFIX).uno
SHL9IMPLIB=	i$(TARGET_VBA)

SHL9VERSIONMAP=$(TARGET_VBA).map
SHL9DEF=$(MISC)$/$(SHL9TARGET).def
DEF9NAME=$(SHL9TARGET)
.IF "$(VBA_EXTENSION)"=="YES"
SHL9RPATH=OXT
.ELSE
SHL9RPATH=OOO
.ENDIF

SHL9STDLIBS= \
		$(CPPUHELPERLIB) \
		$(VCLLIB) \
		$(CPPULIB) \
		$(COMPHELPERLIB) \
		$(SVLIB) \
		$(TOOLSLIB) \
		$(SALLIB)\
		$(BASICLIB)	\
		$(SFXLIB)	\
		$(SVXCORELIB)	\
		$(SVTOOLLIB)    \
		$(SVLLIB) \
		$(ISCLIB) \
        $(VCLLIB) \
        $(TKLIB) \
        $(FORLIB)

SHL9DEPN=$(SHL1TARGETN) $(SHL8TARGETN)
SHL9LIBS=$(SLB)$/$(TARGET_VBA).lib

.IF "$(GUI)"=="UNX" || "$(GUI)"=="MAC"
    LIBCOMPNAME=$(LOCAL_COMMON_OUT)$/lib/lib$(SHL9TARGET)$(DLLPOST)
.ELSE
    LIBCOMPNAME=$(COMMONBIN)$/$(SHL9TARGET)$(DLLPOST)
.ENDIF

.ENDIF
 

# --- Targets -------------------------------------------------------------

.INCLUDE :  target.mk

.IF "$(VBA_EXTENSION)"=="YES"
    COMP=build_extn
.ENDIF

ALLTAR:	$(MISC)$/linkinc.ls  $(COMP)

build_extn : $(SHL9TARGETN)
	$(PERL) createExtPackage.pl $(COMMONBIN)$/vbaapi.oxt  $(SOLARBINDIR)$/oovbaapi.rdb $(LIBCOMPNAME)
