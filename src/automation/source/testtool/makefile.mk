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

PRJNAME=automation
TARGET=testtool

# --- Settings ------------------------------------------------------------

.INCLUDE :  settings.mk

# --- Allgemein ------------------------------------------------------------

OBJFILES = \
	$(OBJ)$/cmdstrm.obj              \
	$(OBJ)$/cretstrm.obj             \
	$(OBJ)$/objtest.obj              \
	$(OBJ)$/tcommuni.obj             \
	$(OBJ)$/comm_bas.obj             \
	$(OBJ)$/httprequest.obj          \
	

EXCEPTIONSFILES= \
	$(OBJ)$/tcommuni.obj             \
	$(OBJ)$/cmdstrm.obj              \
	$(OBJ)$/objtest.obj

HXXFILES = $(INCCOM)$/keycodes.hxx			\
	$(INCCOM)$/classes.hxx			\
	$(INCCOM)$/r_cmds.hxx			\
	$(INCCOM)$/res_type.hxx			

.IF "$(GUI)" == "WNT" 
OBJFILES += $(OBJ)$/sysdir_win.obj
OBJFILES += $(OBJ)$/registry_win.obj
.ENDIF
# --- Targets ------------------------------------------------------------

.INCLUDE :  target.mk


$(OBJFILES) : $(HXXFILES)
$(HXXFILES) : $(MISC)$/xfilter.pl


$(INCCOM)$/keycodes.hxx : $(SOLARVERSION)$/$(INPATH)$/inc$(UPDMINOREXT)$/vcl$/keycodes.hxx \
						  $(MISC)$/xfilter.pl
	$(PERL) $(MISC)$/xfilter.pl $(SOLARVERSION)$/$(INPATH)$/inc$(UPDMINOREXT)  vcl$/keycodes.hxx  $(INCCOM)$/keycodes  KEY_

$(INCCOM)$/classes.hxx :  ..$/inc$/rcontrol.hxx \
						  $(MISC)$/xfilter.pl
	$(PERL) $(MISC)$/xfilter.pl ..$/inc  rcontrol.hxx  $(INCCOM)$/classes  M_

$(INCCOM)$/r_cmds.hxx :   ..$/inc$/rcontrol.hxx \
						  $(MISC)$/xfilter.pl
	$(PERL) $(MISC)$/xfilter.pl ..$/inc  rcontrol.hxx  $(INCCOM)$/r_cmds  RC_

$(INCCOM)$/res_type.hxx : $(SOLARVERSION)$/$(INPATH)$/inc$(UPDMINOREXT)$/vcl$/wintypes.hxx \
						  $(MISC)$/xfilter.pl
	$(PERL) $(MISC)$/xfilter.pl $(SOLARVERSION)$/$(INPATH)$/inc$(UPDMINOREXT)  vcl$/wintypes.hxx  $(INCCOM)$/res_type  WINDOW_



$(MISC)$/xfilter.pl : filter.pl
.IF "$(GUI)" == "UNX" || "$(USE_SHELL)"!="4nt"
	   tr  -d "\015" < filter.pl > $(MISC)$/xfilter.pl
	   chmod 664 $(MISC)$/xfilter.pl
.ELSE
	   $(COPY) filter.pl $(MISC)$/xfilter.pl
	   attrib  -r $(MISC)$/xfilter.pl
.ENDIF



.IF "$(GUI)"=="UNX"
INIFILESUFFIX=rc
BRANDPATH=none
.ELIF "$(GUI)"=="WNT" || "$(GUI)"=="OS2"
INIFILESUFFIX=.ini
BRANDPATH=..
.END

$(BIN)$/testtool$(INIFILESUFFIX): testtool.ini
        $(SED) -e s/$(EMQ)!INIFILESUFFIX$(EMQ)!/$(INIFILESUFFIX)/ \
            -e s/$(EMQ)!BRANDPATH$(EMQ)!/$(BRANDPATH)/ < $< > $@

ALLTAR: \
        $(BIN)$/testtool$(INIFILESUFFIX)

