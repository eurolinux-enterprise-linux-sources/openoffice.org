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

PRJNAME=desktop
TARGET=zipintro
# --- Settings -----------------------------------------------------------

.INCLUDE :  settings.mk

DEFAULT_FLAVOURS=dev dev_nologo nologo broffice dev_broffice nologo_broffice nologo_dev_broffice intro

ZIP1LIST= \
	$(null,$(INTRO_BITMAPS) $(MISC)$/ooo_custom_images$/dev$/introabout$/intro.png $(INTRO_BITMAPS)) \
	$(null,$(ABOUT_BITMAPS) $(MISC)$/$(RSCDEFIMG)$/introabout$/about.png $(ABOUT_BITMAPS))
ZIP2LIST= \
	$(null,$(INTRO_BITMAPS) $(MISC)$/ooo_custom_images$/dev_nologo$/introabout$/intro.png $(INTRO_BITMAPS)) \
	$(null,$(ABOUT_BITMAPS) $(MISC)$/$(RSCDEFIMG)$/introabout$/about.png $(ABOUT_BITMAPS))
ZIP3LIST= \
	$(null,$(INTRO_BITMAPS) $(MISC)$/ooo_custom_images$/nologo$/introabout$/intro.png $(INTRO_BITMAPS)) \
	$(null,$(ABOUT_BITMAPS) $(MISC)$/$(RSCDEFIMG)$/introabout$/about.png $(ABOUT_BITMAPS))
ZIP4LIST= \
	$(null,$(INTRO_BITMAPS) $(MISC)$/$(RSCDEFIMG)$/introabout$/intro.png $(INTRO_BITMAPS)) \
	$(null,$(ABOUT_BITMAPS) $(MISC)$/$(RSCDEFIMG)$/introabout$/about.png $(ABOUT_BITMAPS))
ZIP5LIST= \
	$(null,$(INTRO_BITMAPS) $(MISC)$/ooo_custom_images$/dev_broffice$/introabout$/intro.png $(INTRO_BITMAPS)) \
	$(null,$(ABOUT_BITMAPS) $(MISC)$/ooo_custom_images$/broffice$/introabout$/about.png $(ABOUT_BITMAPS))
ZIP6LIST= \
	$(null,$(INTRO_BITMAPS) $(MISC)$/ooo_custom_images$/broffice$/introabout$/intro.png $(INTRO_BITMAPS)) \
	$(null,$(ABOUT_BITMAPS) $(MISC)$/ooo_custom_images$/broffice$/introabout$/about.png $(ABOUT_BITMAPS))
ZIP7LIST= \
	$(null,$(INTRO_BITMAPS) $(MISC)$/ooo_custom_images$/nologo_broffice$/introabout$/intro.png $(INTRO_BITMAPS)) \
	$(null,$(ABOUT_BITMAPS) $(MISC)$/ooo_custom_images$/broffice$/introabout$/about.png $(ABOUT_BITMAPS))
ZIP8LIST= \
	$(null,$(INTRO_BITMAPS) $(MISC)$/ooo_custom_images$/dev_nologo_broffice$/introabout$/intro.png $(INTRO_BITMAPS)) \
	$(null,$(ABOUT_BITMAPS) $(MISC)$/ooo_custom_images$/broffice$/introabout$/about.png $(ABOUT_BITMAPS))

ZIP1TARGET=dev_intro
ZIP1DEPS=$(ZIP1LIST)

ZIP2TARGET=dev_nologo_intro
ZIP2DEPS=$(ZIP2LIST)

ZIP3TARGET=nologo_intro
ZIP3DEPS=$(ZIP3LIST)

ZIP4TARGET=intro_intro
ZIP4DEPS=$(ZIP4LIST)

ZIP5TARGET=dev_broffice_intro
ZIP5DEPS=$(ZIP5LIST)

ZIP6TARGET=broffice_intro
ZIP6DEPS=$(ZIP6LIST)

ZIP7TARGET=nologo_broffice_intro
ZIP7DEPS=$(ZIP7LIST)

ZIP8TARGET=nologo_dev_broffice_intro
ZIP8DEPS=$(ZIP8LIST)

.INCLUDE :  target.mk

ALLTAR : $(foreach,i,$(DEFAULT_FLAVOURS) $(COMMONBIN)$/$i$/intro.zip)

# now duplicate for deliver...
# Because of issue 78837 we cannot use a % rule here (Commented out below)
# but have to write individual rules.
#$(COMMONBIN)$/%$/intro.zip : $(COMMONBIN)$/%_intro.zip

$(COMMONBIN)$/dev$/intro.zip : $(COMMONBIN)$/dev_intro.zip
	@@-$(MKDIR) $(@:d)
	@$(COPY) $< $@

$(COMMONBIN)$/dev_nologo$/intro.zip : $(COMMONBIN)$/dev_nologo_intro.zip
	@@-$(MKDIR) $(@:d)
	@$(COPY) $< $@

$(COMMONBIN)$/nologo$/intro.zip : $(COMMONBIN)$/nologo_intro.zip
	@@-$(MKDIR) $(@:d)
	@$(COPY) $< $@

$(COMMONBIN)$/broffice$/intro.zip : $(COMMONBIN)$/broffice_intro.zip
	@@-$(MKDIR) $(@:d)
	@$(COPY) $< $@

$(COMMONBIN)$/dev_broffice$/intro.zip : $(COMMONBIN)$/dev_broffice_intro.zip
        @@-$(MKDIR) $(@:d)
        @$(COPY) $< $@

$(COMMONBIN)$/nologo_broffice$/intro.zip : $(COMMONBIN)$/nologo_broffice_intro.zip
        @@-$(MKDIR) $(@:d)
        @$(COPY) $< $@

$(COMMONBIN)$/nologo_dev_broffice$/intro.zip : $(COMMONBIN)$/nologo_dev_broffice_intro.zip
        @@-$(MKDIR) $(@:d)
        @$(COPY) $< $@

$(COMMONBIN)$/intro$/intro.zip : $(COMMONBIN)$/intro_intro.zip
	@@-$(MKDIR) $(@:d)
	@$(COPY) $< $@

$(MISC)$/%.bmp : $(SOLARSRC)$/%.bmp
	@@-$(MKDIRHIER) $(@:d)
	$(COPY) $< $@

$(MISC)$/%.png : $(SOLARSRC)$/%.png
	@@-$(MKDIRHIER) $(@:d)
	$(COPY) $< $@
