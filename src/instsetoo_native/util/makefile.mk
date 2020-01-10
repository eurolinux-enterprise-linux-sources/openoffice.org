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
PRJNAME=instsetoo_native
TARGET=util
GEN_HID2=TRUE

.INCLUDE:  settings.mk
.INCLUDE: $(SOLARINCDIR)$/rtlbootstrap.mk

# PERL:=@echo

# watch for the path delimiter
.IF "$(GUI)"=="WNT"
PYTHONPATH:=$(PWD)$/$(BIN);$(SOLARLIBDIR);$(SOLARLIBDIR)$/python;$(SOLARLIBDIR)$/python$/lib-dynload
.ELSE			# "$(GUI)"=="WNT"
PYTHONPATH:=$(PWD)$/$(BIN):$(SOLARLIBDIR):$(SOLARLIBDIR)$/python:$(SOLARLIBDIR)$/python$/lib-dynload
.ENDIF			# "$(GUI)"=="WNT"
.EXPORT: PYTHONPATH

.IF "$(CWS_WORK_STAMP)"=="" || "$(UPDATER)"!=""
ENABLE_DOWNLOADSETS*=TRUE
.ENDIF			# "$(CWS_WORK_STAMP)"=="" || "$(UPDATER)"!=""
.IF "$(FORCE_DOWNLOADSETS)"!=""
ENABLE_DOWNLOADSETS=TRUE
.ENDIF			# "$(FORCE_DOWNLOADSETS)"!=""

.EXPORT: ENABLE_DOWNLOADSETS
.EXPORT: LAST_MINOR
.EXPORT: PRJ
.EXPORT: PRJNAME

.IF "$(OS)"=="LINUX"
DONT_REMOVE_PACKAGE:=TRUE
.EXPORT: DONT_REMOVE_PACKAGE
.ENDIF          # "$(OS)"=="LINUX"

.INCLUDE .IGNORE: $(SRC_ROOT)$/cwsname.mk

SHARED_COM_SDK_PATH*:=.

INSTALLDIR=$(OUT)

.INCLUDE: target.mk

LOCALPYFILES= \
    $(BIN)$/uno.py \
	$(BIN)$/unohelper.py \
	$(BIN)$/pythonloader.py \
	$(BIN)$/pythonscript.py \
	$(BIN)$/officehelper.py \
	$(BIN)$/mailmerge.py

# PKGFORMAT taken from environment. See possible
# values below.
#
# epm supports the following formats:
# aix - AIX software distribution
# bsd - FreeBSD, NetBSD, or OpenBSD software distribution
# depot or swinstall - HP-UX software distribution
# deb - Debian software distribution
# inst or tardist - IRIX software distribution
# osx - MacOS X software distribution
# pkg - Solaris software distribution
# rpm - RedHat software distribution
# setld - Tru64 (setld) software distribution
# native - "Native" software distribution for the platform
# portable - Portable software distribution
xxxx:
    echo $(PERL) -w $(SOLARENV)$/bin$/gen_update_info.pl --buildid $(BUILD) --arch "$(RTL_ARCH)" --os "$(RTL_OS)" --lstfile $(PRJ)$/util$/openoffice.lst --product OpenOffice --languages $(subst,$(@:s/_/ /:1)_, $(@:b)) $(PRJ)$/util$/update.xml

.IF "$(GUI)"!="WNT" && "$(EPM)"=="NO" && "$(USE_PACKAGER)"==""
ALLTAR : $(LOCALPYFILES)
	@echo "No EPM: do no packaging at this stage"
.ELSE			# "$(GUI)"!="WNT" && "$(EPM)"=="NO" && "$(USE_PACKAGER)"==""
.IF "$(UPDATER)"=="" || "$(USE_PACKAGER)"==""
.IF "$(BUILD_TYPE)"=="$(BUILD_TYPE:s/ODK//)"
ALLTAR : openoffice
.ELSE
ALLTAR : openoffice sdkoo_en-US ure_en-US
.ENDIF
.ELSE			# "$(UPDATER)"=="" || "$(USE_PACKAGER)"==""
ALLTAR : updatepack
.ENDIF			# "$(UPDATER)"=="" || "$(USE_PACKAGER)"==""
.ENDIF			# "$(GUI)"!="WNT" && "$(EPM)"=="NO" && "$(USE_PACKAGER)"==""

.IF "$(MAKETARGETS:e)"!=""
PKGFORMAT+=$(MAKETARGETS:e:s/.//)
.ENDIF			# "$(MAKETARGETS:e)"!=""

.IF "$(PKGFORMAT)"!=""
.IF "$(FORCE2ARCHIVE)"!=""
PKGFORMATSWITCH=-format archive
.ELSE			# "$(FORCE2ARCHIVE)"!=""
PKGFORMATSWITCH=-format xxx
.ENDIF			# "$(FORCE2ARCHIVE)"!=""
.ENDIF			# "$(PKGFORMAT)"!=""

.IF "$(VERBOSE)"=="TRUE"
VERBOSESWITCH=-verbose
.ENDIF
.IF "$(VERBOSE)"=="FALSE"
VERBOSESWITCH=-quiet
.ENDIF

updatepack:
	$(PERL) -w $(SOLARENV)$/bin$/packager.pl

.IF "$(alllangiso)"!=""

openoffice: $(foreach,i,$(alllangiso) openoffice_$i)

openofficedev: $(foreach,i,$(alllangiso) openofficedev_$i)

openofficewithjre: $(foreach,i,$(alllangiso) openofficewithjre_$i)

ooolanguagepack : $(foreach,i,$(alllangiso) ooolanguagepack_$i)

ooodevlanguagepack: $(foreach,i,$(alllangiso) ooodevlanguagepack_$i)

sdkoo: $(foreach,i,$(alllangiso) sdkoo_$i)

sdkoodev: $(foreach,i,$(alllangiso) sdkoodev_$i)

ure: $(foreach,i,$(alllangiso) ure_$i)

broffice: $(foreach,i,$(alllangiso) broffice_$i)

brofficedev: $(foreach,i,$(alllangiso) brofficedev_$i)

brofficewithjre: $(foreach,i,$(alllangiso) brofficewithjre_$i)

broolanguagepack : $(foreach,i,$(alllangiso) broolanguagepack_$i)

MSIOFFICETEMPLATESOURCE=$(PRJ)$/inc_openoffice$/windows$/msi_templates
MSILANGPACKTEMPLATESOURCE=$(PRJ)$/inc_ooolangpack$/windows$/msi_templates
MSIURETEMPLATESOURCE=$(PRJ)$/inc_ure$/windows$/msi_templates
MSISDKOOTEMPLATESOURCE=$(PRJ)$/inc_sdkoo$/windows$/msi_templates

.IF "$(BUILD_SPECIAL)"!=""
MSIOFFICETEMPLATEDIR=$(MSIOFFICETEMPLATESOURCE)
MSILANGPACKTEMPLATEDIR=$(MSILANGPACKTEMPLATESOURCE)
MSIURETEMPLATEDIR=$(MSIURETEMPLATESOURCE)
MSISDKOOTEMPLATEDIR=$(MSISDKOOTEMPLATESOURCE)
.ELSE			# "$(BUILD_SPECIAL)"!=""
NOLOGOSPLASH:=$(BIN)$/intro.zip
DEVNOLOGOSPLASH:=$(BIN)$/dev$/intro.zip
BROFFICENOLOGOSPLASH:=$(BIN)$/broffice$/intro.zip
BROFFICENOLOGOBRAND:=$(BIN)$/broffice$/images_brand.zip
BROFFICEDEVNOLOGOSPLASH:=$(BIN)$/broffice_dev$/intro.zip
BROFFICENDEVOLOGOBRAND:=$(BIN)$/broffice_dev$/images_brand.zip
MSIOFFICETEMPLATEDIR=$(MISC)$/openoffice$/msi_templates
MSILANGPACKTEMPLATEDIR=$(MISC)$/ooolangpack$/msi_templates
MSIURETEMPLATEDIR=$(MISC)$/ure$/msi_templates
MSISDKOOTEMPLATEDIR=$(MISC)$/sdkoo$/msi_templates

ADDDEPS=$(NOLOGOSPLASH) $(DEVNOLOGOSPLASH) $(BROFFICENOLOGOSPLASH) $(BROFFICEDEVNOLOGOSPLASH) \
    $(BROFFICENOLOGOBRAND) $(BROFFICEDEVNOLOGOBRAND)
.IF "$(OS)" == "WNT"
ADDDEPS+=hack_msitemplates
.ENDIF

$(foreach,i,$(alllangiso) openoffice_$i) : $(ADDDEPS)

$(foreach,i,$(alllangiso) openofficedev_$i) : $(ADDDEPS)

$(foreach,i,$(alllangiso) openofficewithjre_$i) : $(ADDDEPS)

$(foreach,i,$(alllangiso) ooolanguagepack_$i) : $(ADDDEPS)

$(foreach,i,$(alllangiso) ooodevlanguagepack_$i) : $(ADDDEPS)

$(foreach,i,$(alllangiso) sdkoo_$i) : $(ADDDEPS)

$(foreach,i,$(alllangiso) sdkoodev_$i) : $(ADDDEPS)
             
$(foreach,i,$(alllangiso) ure_$i) : $(ADDDEPS)

$(foreach,i,$(alllangiso) broffice_$i) : $(ADDDEPS)

$(foreach,i,$(alllangiso) brofficedev_$i) : $(ADDDEPS)

$(foreach,i,$(alllangiso) brofficewithjre_$i) : $(ADDDEPS)

$(foreach,i,$(alllangiso) broolanguagepack_$i) : $(ADDDEPS)

.IF "$(MAKETARGETS)"!=""
$(MAKETARGETS) : $(ADDDEPS)
.ENDIF			# "$(MAKETARGETS)"!=""

.ENDIF			# "$(BUILD_SPECIAL)"!=""

.IF "$(PKGFORMAT)"!=""
$(foreach,i,$(alllangiso) openoffice_$i) : $$@{$(PKGFORMAT:^".")}
.IF "$(MAKETARGETS)"!="" && "$(PKGFORMAT)"!=""
.IF "$(MAKETARGETS:e)"=="" && "$(MAKETARGETS:s/_//)"!="$(MAKETARGETS)"
$(MAKETARGETS) : $$@{$(PKGFORMAT:^".")}
$(MAKETARGETS){$(PKGFORMAT:^".")} : $(ADDDEPS)
.ENDIF			# "$(MAKETARGETS:e)"=="" && "$(MAKETARGETS:s/_//)"!="$(MAKETARGETS)"
.ENDIF			# "$(MAKETARGETS)"!="" && "$(PKGFORMAT)"!=""
openoffice_%{$(PKGFORMAT:^".")} :
.ELSE			# "$(PKGFORMAT)"!=""
openoffice_% :
.ENDIF			# "$(PKGFORMAT)"!=""
	$(PERL) -w $(SOLARENV)$/bin$/make_installer.pl -f $(PRJ)$/util$/openoffice.lst -l $(subst,$(@:s/_/ /:1)_, $(@:b)) -p OpenOffice -u $(OUT) -buildid $(BUILD) -msitemplate $(MSIOFFICETEMPLATEDIR) -msilanguage $(COMMONMISC)$/win_ulffiles $(subst,xxx,$(@:e:s/.//) $(PKGFORMATSWITCH) $(VERBOSESWITCH))
	$(PERL) -w $(SOLARENV)$/bin$/gen_update_info.pl --buildid $(BUILD) --arch "$(RTL_ARCH)" --os "$(RTL_OS)" --lstfile $(PRJ)$/util$/openoffice.lst --product OpenOffice --languages $(subst,$(@:s/_/ /:1)_, $(@:b)) $(PRJ)$/util$/update.xml > $(MISC)/$(@:b)_$(RTL_OS)_$(RTL_ARCH)$(@:e).update.xml

.IF "$(PKGFORMAT)"!=""
$(foreach,i,$(alllangiso) openofficewithjre_$i) : $$@{$(PKGFORMAT:^".")}
openofficewithjre_%{$(PKGFORMAT:^".")} :
.ELSE			# "$(PKGFORMAT)"!=""
openofficewithjre_% :
.ENDIF			# "$(PKGFORMAT)"!=""
	$(PERL) -w $(SOLARENV)$/bin$/make_installer.pl -f $(PRJ)$/util$/openoffice.lst -l $(subst,$(@:s/_/ /:1)_, $(@:b)) -p OpenOffice_wJRE -u $(OUT) -buildid $(BUILD) -msitemplate $(MSIOFFICETEMPLATEDIR) -msilanguage $(COMMONMISC)$/win_ulffiles $(subst,xxx,$(@:e:s/.//) $(PKGFORMATSWITCH) $(VERBOSESWITCH))

.IF "$(PKGFORMAT)"!=""
$(foreach,i,$(alllangiso) openofficedev_$i) : $$@{$(PKGFORMAT:^".")}
openofficedev_%{$(PKGFORMAT:^".")} :
.ELSE			# "$(PKGFORMAT)"!=""
openofficedev_% :
.ENDIF			# "$(PKGFORMAT)"!=""
	$(PERL) -w $(SOLARENV)$/bin$/make_installer.pl -f $(PRJ)$/util$/openoffice.lst -l $(subst,$(@:s/_/ /:1)_, $(@:b)) -p OpenOffice_Dev -u $(OUT) -buildid $(BUILD) -msitemplate $(MSIOFFICETEMPLATEDIR) -msilanguage $(COMMONMISC)$/win_ulffiles $(subst,xxx,$(@:e:s/.//) $(PKGFORMATSWITCH) $(VERBOSESWITCH))
	$(PERL) -w $(SOLARENV)$/bin$/gen_update_info.pl --buildid $(BUILD) --arch "$(RTL_ARCH)" --os "$(RTL_OS)" --lstfile $(PRJ)$/util$/openoffice.lst --product OpenOffice_Dev --languages $(subst,$(@:s/_/ /:1)_, $(@:b)) $(PRJ)$/util$/update.xml > $(MISC)/$(@:b)_$(RTL_OS)_$(RTL_ARCH)$(@:e).update.xml

.IF "$(PKGFORMAT)"!=""
$(foreach,i,$(alllangiso) ooolanguagepack_$i) : $$@{$(PKGFORMAT:^".")}
ooolanguagepack_%{$(PKGFORMAT:^".")} :
.ELSE			# "$(PKGFORMAT)"!=""
ooolanguagepack_% :
.ENDIF			# "$(PKGFORMAT)"!=""
	$(PERL) -w $(SOLARENV)$/bin$/make_installer.pl -f $(PRJ)$/util$/openoffice.lst -l $(subst,$(@:s/_/ /:1)_, $(@:b)) -p OpenOffice -u $(OUT) -buildid $(BUILD) -msitemplate $(MSILANGPACKTEMPLATEDIR) -msilanguage $(COMMONMISC)$/win_ulffiles -languagepack $(subst,xxx,$(@:e:s/.//) $(PKGFORMATSWITCH) $(VERBOSESWITCH))

.IF "$(PKGFORMAT)"!=""
$(foreach,i,$(alllangiso) ooodevlanguagepack_$i) : $$@{$(PKGFORMAT:^".")}
ooodevlanguagepack_%{$(PKGFORMAT:^".")} :
.ELSE			# "$(PKGFORMAT)"!=""
ooodevlanguagepack_% :
.ENDIF			# "$(PKGFORMAT)"!=""
	$(PERL) -w $(SOLARENV)$/bin$/make_installer.pl -f $(PRJ)$/util$/openoffice.lst -l $(subst,$(@:s/_/ /:1)_, $(@:b)) -p OpenOffice_Dev -u $(OUT) -buildid $(BUILD) -msitemplate $(MSILANGPACKTEMPLATEDIR) -msilanguage $(COMMONMISC)$/win_ulffiles -languagepack $(subst,xxx,$(@:e:s/.//) $(PKGFORMATSWITCH) $(VERBOSESWITCH))

.IF "$(PKGFORMAT)"!=""
$(foreach,i,$(alllangiso) sdkoo_$i) : $$@{$(PKGFORMAT:^".")}
sdkoo_%{$(PKGFORMAT:^".")} :
.ELSE			# "$(PKGFORMAT)"!=""
sdkoo_% :
.ENDIF			# "$(PKGFORMAT)"!=""
	$(PERL) -w $(SOLARENV)$/bin$/make_installer.pl -f $(PRJ)$/util$/openoffice.lst -l $(subst,$(@:s/_/ /:1)_, $(@:b)) -p OpenOffice_SDK -u $(OUT) -buildid $(BUILD) -msitemplate $(MSISDKOOTEMPLATEDIR) -msilanguage $(COMMONMISC)$/win_ulffiles $(subst,xxx,$(@:e:s/.//) -dontstrip $(PKGFORMATSWITCH) $(VERBOSESWITCH))

.IF "$(PKGFORMAT)"!=""
$(foreach,i,$(alllangiso) sdkoodev_$i) : $$@{$(PKGFORMAT:^".")}
sdkoodev_%{$(PKGFORMAT:^".")} :
.ELSE			# "$(PKGFORMAT)"!=""
sdkoodev_% :
.ENDIF			# "$(PKGFORMAT)"!=""
	$(PERL) -w $(SOLARENV)$/bin$/make_installer.pl -f $(PRJ)$/util$/openoffice.lst -l $(subst,$(@:s/_/ /:1)_, $(@:b)) -p OpenOffice_Dev_SDK -u $(OUT) -buildid $(BUILD) -msitemplate $(MSISDKOOTEMPLATEDIR) -msilanguage $(COMMONMISC)$/win_ulffiles $(subst,xxx,$(@:e:s/.//) -dontstrip $(PKGFORMATSWITCH) $(VERBOSESWITCH))

.IF "$(PKGFORMAT)"!=""
$(foreach,i,$(alllangiso) ure_$i) : $$@{$(PKGFORMAT:^".")}
ure_%{$(PKGFORMAT:^".")} :
.ELSE			# "$(PKGFORMAT)"!=""
ure_% :
.ENDIF			# "$(PKGFORMAT)"!=""
.IF "$(OS)" == "MACOSX"
    @echo 'for now, there is no standalone URE for Mac OS X'
.ELSE
    $(PERL) -w $(SOLARENV)$/bin$/make_installer.pl -f $(PRJ)$/util$/openoffice.lst \
        -l $(subst,$(@:s/_/ /:1)_, $(@:b)) -p URE -u $(OUT) -buildid $(BUILD) $(subst,xxx,$(@:e:s/.//) $(PKGFORMATSWITCH) $(VERBOSESWITCH)) \
        -msitemplate $(MSIURETEMPLATEDIR) \
        -msilanguage $(COMMONMISC)$/win_ulffiles
.ENDIF

.IF "$(PKGFORMAT)"!=""
$(foreach,i,$(alllangiso) broffice_$i) : $$@{$(PKGFORMAT:^".")}
.IF "$(MAKETARGETS)"!="" && "$(PKGFORMAT)"!=""
.IF "$(MAKETARGETS:e)"=="" && "$(MAKETARGETS:s/_//)"!="$(MAKETARGETS)"
$(MAKETARGETS) : $$@{$(PKGFORMAT:^".")}
$(MAKETARGETS){$(PKGFORMAT:^".")} : $(ADDDEPS)
.ENDIF			# "$(MAKETARGETS:e)"=="" && "$(MAKETARGETS:s/_//)"!="$(MAKETARGETS)"
.ENDIF			# "$(MAKETARGETS)"!="" && "$(PKGFORMAT)"!=""
broffice_%{$(PKGFORMAT:^".")} :
.ELSE			# "$(PKGFORMAT)"!=""
broffice_% :
.ENDIF			# "$(PKGFORMAT)"!=""
	+$(PERL) -w $(SOLARENV)$/bin$/make_installer.pl -f $(PRJ)$/util$/openoffice.lst -l $(subst,$(@:s/_/ /:1)_, $(@:b)) -p BrOffice -u $(OUT) -buildid $(BUILD) -msitemplate $(MSIOFFICETEMPLATEDIR) -msilanguage $(COMMONMISC)$/win_ulffiles $(subst,xxx,$(@:e:s/.//) $(PKGFORMATSWITCH) $(VERBOSESWITCH))
	$(PERL) -w $(SOLARENV)$/bin$/gen_update_info.pl --buildid $(BUILD) --arch "$(RTL_ARCH)" --os "$(RTL_OS)" --lstfile $(PRJ)$/util$/openoffice.lst --product BrOffice --languages $(subst,$(@:s/_/ /:1)_, $(@:b)) $(PRJ)$/util$/update.xml > $(MISC)/$(@:b)_$(RTL_OS)_$(RTL_ARCH)$(@:e).update.xml

.IF "$(PKGFORMAT)"!=""
$(foreach,i,$(alllangiso) brofficewithjre_$i) : $$@{$(PKGFORMAT:^".")}
brofficewithjre_%{$(PKGFORMAT:^".")} :
.ELSE			# "$(PKGFORMAT)"!=""
brofficewithjre_% :
.ENDIF			# "$(PKGFORMAT)"!=""
	+$(PERL) -w $(SOLARENV)$/bin$/make_installer.pl -f $(PRJ)$/util$/openoffice.lst -l $(subst,$(@:s/_/ /:1)_, $(@:b)) -p BrOffice_wJRE -u $(OUT) -buildid $(BUILD) -msitemplate $(MSIOFFICETEMPLATEDIR) -msilanguage $(COMMONMISC)$/win_ulffiles $(subst,xxx,$(@:e:s/.//) $(PKGFORMATSWITCH) $(VERBOSESWITCH))

.IF "$(PKGFORMAT)"!=""
$(foreach,i,$(alllangiso) brofficedev_$i) : $$@{$(PKGFORMAT:^".")}
brofficedev_%{$(PKGFORMAT:^".")} :
.ELSE			# "$(PKGFORMAT)"!=""
brofficedev_% :
.ENDIF			# "$(PKGFORMAT)"!=""
	+$(PERL) -w $(SOLARENV)$/bin$/make_installer.pl -f $(PRJ)$/util$/openoffice.lst -l $(subst,$(@:s/_/ /:1)_, $(@:b)) -p BrOffice_Dev -u $(OUT) -buildid $(BUILD) -msitemplate $(MSIOFFICETEMPLATEDIR) -msilanguage $(COMMONMISC)$/win_ulffiles $(subst,xxx,$(@:e:s/.//) $(PKGFORMATSWITCH) $(VERBOSESWITCH))
	$(PERL) -w $(SOLARENV)$/bin$/gen_update_info.pl --buildid $(BUILD) --arch "$(RTL_ARCH)" --os "$(RTL_OS)" --lstfile $(PRJ)$/util$/openoffice.lst --product BrOffice_Dev --languages $(subst,$(@:s/_/ /:1)_, $(@:b)) $(PRJ)$/util$/update.xml > $(MISC)/$(@:b)_$(RTL_OS)_$(RTL_ARCH)$(@:e).update.xml

.IF "$(PKGFORMAT)"!=""
$(foreach,i,$(alllangiso) broolanguagepack_$i) : $$@{$(PKGFORMAT:^".")}
broolanguagepack_%{$(PKGFORMAT:^".")} :
.ELSE			# "$(PKGFORMAT)"!=""
broolanguagepack_% :
.ENDIF			# "$(PKGFORMAT)"!=""
	+$(PERL) -w $(SOLARENV)$/bin$/make_installer.pl -f $(PRJ)$/util$/openoffice.lst -l $(subst,$(@:s/_/ /:1)_, $(@:b)) -p BrOffice -u $(OUT) -buildid $(BUILD) -msitemplate $(MSILANGPACKTEMPLATEDIR) -msilanguage $(COMMONMISC)$/win_ulffiles -languagepack $(subst,xxx,$(@:e:s/.//) $(PKGFORMATSWITCH) $(VERBOSESWITCH))

.ELSE			# "$(alllangiso)"!=""
openoffice:
	@echo cannot pack nothing...

.ENDIF			# "$(alllangiso)"!=""

.IF "$(LOCALPYFILES)"!=""
.IF "$(PKGFORMAT)"==""
$(foreach,i,$(alllangiso) openoffice_$i openofficewithjre_$i openofficedev_$i broffice_$i brofficewithjre_$i brofficedev_$i sdkoo_$i) updatepack : $(LOCALPYFILES) $(BIN)$/cp1251.py $(BIN)$/iso8859_1.py
.ELSE			# "$(PKGFORMAT)"==""
$(foreach,i,$(alllangiso) openoffice_$i{$(PKGFORMAT:^".")} openofficewithjre_$i{$(PKGFORMAT:^".")} openofficedev_$i{$(PKGFORMAT:^".")} broffice_$i{$(PKGFORMAT:^".")} brofficewithjre_$i{$(PKGFORMAT:^".")} brofficedev_$i{$(PKGFORMAT:^".")} sdkoo_$i{$(PKGFORMAT:^".")}) updatepack : $(LOCALPYFILES) $(BIN)$/cp1251.py $(BIN)$/iso8859_1.py
.ENDIF			# "$(PKGFORMAT)"==""
.ENDIF			# "$(LOCALPYFILES)"!=""

$(BIN)$/%.py : $(SOLARSHAREDBIN)$/pyuno$/%.py
	@$(COPY) $< $@

.IF "$(SYSTEM_PYTHON)" != "YES"
$(BIN)$/cp1251.py : $(SOLARLIBDIR)$/python$/encodings$/cp1251.py
	@$(COPY) $< $@
$(BIN)$/iso8859_1.py : $(SOLARLIBDIR)$/python$/encodings$/iso8859_1.py
	@$(COPY) $< $@
.ELSE
$(BIN)$/cp1251.py :
	@echo "Using system python - nothing more to do here"
$(BIN)$/iso8859_1.py :
	@echo "Using system python - nothing more to do here"
.ENDIF

$(BIN)$/intro.zip : $(SOLARCOMMONPCKDIR)$/openoffice_nologo$/intro.zip
	$(COPY) $< $@

$(BIN)$/dev$/intro.zip : $(SOLARCOMMONPCKDIR)$/openoffice_dev_nologo$/intro.zip
	@-$(MKDIR) $(@:d)
	$(COPY) $< $@

$(BIN)$/broffice_dev$/intro.zip : $(SOLARCOMMONPCKDIR)$/broffice_dev_nologo$/intro.zip
	@-$(MKDIR) $(@:d)
	$(COPY) $< $@

$(BIN)$/broffice$/intro.zip : $(SOLARCOMMONPCKDIR)$/broffice_nologo$/intro.zip
	@-$(MKDIR) $(@:d)
	$(COPY) $< $@

$(BIN)$/broffice$/images_brand.zip : $(SOLARCOMMONBINDIR)$/broffice_nologo$/images_brand.zip
	@-$(MKDIR) $(@:d)
	$(COPY) $< $@

hack_msitemplates .PHONY:
	-$(MKDIRHIER) $(MSIOFFICETEMPLATEDIR)
	-$(MKDIRHIER) $(MSILANGPACKTEMPLATEDIR)
	-$(MKDIRHIER) $(MSIURETEMPLATEDIR)
	-$(MKDIRHIER) $(MSISDKOOTEMPLATEDIR)
	-$(MKDIRHIER) $(MSIOFFICETEMPLATEDIR)$/Binary
	-$(MKDIRHIER) $(MSILANGPACKTEMPLATEDIR)$/Binary
	-$(MKDIRHIER) $(MSIURETEMPLATEDIR)$/Binary
	-$(MKDIRHIER) $(MSISDKOOTEMPLATEDIR)$/Binary
	$(GNUCOPY) -u $(MSIOFFICETEMPLATESOURCE)$/*.* $(MSIOFFICETEMPLATEDIR)
	$(GNUCOPY) -u $(MSILANGPACKTEMPLATESOURCE)$/*.* $(MSILANGPACKTEMPLATEDIR)
	$(GNUCOPY) -u $(MSIURETEMPLATESOURCE)$/*.* $(MSIURETEMPLATEDIR)
	$(GNUCOPY) -u $(MSISDKOOTEMPLATESOURCE)$/*.* $(MSISDKOOTEMPLATEDIR)
	$(GNUCOPY) -u $(MSIOFFICETEMPLATESOURCE)$/Binary$/*.* $(MSIOFFICETEMPLATEDIR)$/Binary
	$(GNUCOPY) -u $(MSILANGPACKTEMPLATESOURCE)$/Binary$/*.* $(MSILANGPACKTEMPLATEDIR)$/Binary
	$(GNUCOPY) -u $(MSIURETEMPLATESOURCE)$/Binary$/*.* $(MSIURETEMPLATEDIR)$/Binary
	$(GNUCOPY) -u $(MSISDKOOTEMPLATESOURCE)$/Binary$/*.* $(MSISDKOOTEMPLATEDIR)$/Binary
	$(RM) $(MSIOFFICETEMPLATEDIR)$/Binary$/Image.bmp
	$(RM) $(MSILANGPACKTEMPLATEDIR)$/Binary$/Image.bmp
	$(RM) $(MSIURETEMPLATEDIR)$/Binary$/Image.bmp
	$(RM) $(MSISDKOOTEMPLATEDIR)$/Binary$/Image.bmp
	$(COPY) $(PRJ)$/res$/nologoinstall.bmp $(MSIOFFICETEMPLATEDIR)$/Binary$/Image.bmp
	$(COPY) $(PRJ)$/res$/nologoinstall.bmp $(MSILANGPACKTEMPLATEDIR)$/Binary$/Image.bmp
	$(COPY) $(PRJ)$/res$/nologoinstall.bmp $(MSIURETEMPLATEDIR)$/Binary$/Image.bmp
	$(COPY) $(PRJ)$/res$/nologoinstall.bmp $(MSISDKOOTEMPLATEDIR)$/Binary$/Image.bmp


