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

#######################################################
# instructions for linking
# unroll begin

.IF "$(GUI)" == "OS2" && "$(TARGETTYPE)" == "GUI" 
APP$(TNR)DEF = $(MISC)/$(APP$(TNR)TARGET).def
.ENDIF

.IF "$(APP$(TNR)LINKTYPE)" != ""
#must be either STATIC or SHARED
APP$(TNR)LINKTYPEFLAG=$(APPLINK$(APP$(TNR)LINKTYPE))
.ENDIF

# decide how to link
.IF "$(APP$(TNR)CODETYPE)"=="C"
APP$(TNR)LINKER=$(LINKC)
APP$(TNR)STDLIB=$(subst,CPPRUNTIME, $(STDLIB))
APP$(TNR)LINKFLAGS+=$(LINKCFLAGS)
.ELSE			# "$(APP$(TNR)CODETYPE)"=="C"
APP$(TNR)LINKER=$(LINK)
APP$(TNR)STDLIB=$(subst,CPPRUNTIME,$(STDLIBCPP) $(STDLIB))
APP$(TNR)LINKFLAGS+=$(LINKFLAGS)
.ENDIF			# "$(APP$(TNR)CODETYPE)"=="C"

APP$(TNR)RPATH*=OOO
LINKFLAGSRUNPATH_$(APP$(TNR)RPATH)*=/ERROR:/Bad_APP$(TNR)RPATH_value
.IF "$(OS)" != "MACOSX"
APP$(TNR)LINKFLAGS+=$(LINKFLAGSRUNPATH_$(APP$(TNR)RPATH))
.ENDIF

.IF "$(APP$(TNR)STACK)" != ""
.IF "$(LINKFLAGSTACK)" != ""
APP$(TNR)STACKN=$(LINKFLAGSTACK)$(APP$(TNR)STACK)
.ENDIF
.ELSE
APP$(TNR)STACKN=
.ENDIF

.IF "$(APP$(TNR)NOSAL)"==""
.IF "$(TARGETTYPE)" == "GUI"
APP$(TNR)OBJS+= $(STDOBJVCL)
.ENDIF
.ENDIF

.IF "$(GUI)$(COM)" == "WNTGCC"
APP$(TNR)RESO=
.IF "$(APP$(TNR)LINKRES)" != "" || "$(APP$(TNR)RES)" != ""
APP$(TNR)RESO=$(MISC)/$(APP$(TNR)TARGET:b)_res.o
.ENDIF
.ENDIF

.IF "$(GUI)" == "UNX"
APP$(TNR)DEPN+:=$(APP$(TNR)DEPNU)
USE_APP$(TNR)DEF=
.ENDIF

.IF "$(APP$(TNR)TARGETN)"!=""

.IF "$(APP$(TNR)PRODUCTNAME)"!=""
APP$(TNR)PRODUCTDEF+:=-DPRODUCT_NAME=\"$(APP$(TNR)PRODUCTNAME)\"
.ENDIF			# "$(APP$(TNR)PRODUCTNAME)"!=""

.IF "$(linkinc)"!=""
.IF "$(GUI)"=="WNT" || "$(GUI)"=="OS2"
.IF "$(APP$(TNR)LIBS)"!=""
$(MISC)/$(APP$(TNR)TARGET)_linkinc.ls .PHONY:
	@@-$(RM) $@
	sed -f $(SOLARENV)/bin/chrel.sed $(foreach,i,$(APP$(TNR)LIBS) $(i:s/.lib/.lin/)) >> $@
.ENDIF          #"$(APP$(TNR)LIBS)"!="" 
.ENDIF

LINKINCTARGETS+=$(MISC)/$(APP$(TNR)TARGETN:b)_linkinc.ls
$(APP$(TNR)TARGETN) : $(LINKINCTARGETS)
.ENDIF          # "$(linkinc)"!=""

# Allow for target specific LIBSALCPPRT override
APP$(TNR)LIBSALCPPRT*=$(LIBSALCPPRT)

.IF "$(GUI)" == "OS2"
_APP$(TNR)IMP_ORD = $(APP$(TNR)STDLIBS:^"$(SOLARVERSION)/$(INPATH)/lib/") $(APP$(TNR)STDLIBS:^"$(LB)/") 
APP$(TNR)IMP_ORD = $(foreach,i,$(_APP$(TNR)IMP_ORD) $(shell @-ls $i))
.ELSE
APP$(TNR)IMP_ORD = 
.ENDIF

$(APP$(TNR)TARGETN): $(APP$(TNR)OBJS) $(APP$(TNR)LIBS) \
	$(APP$(TNR)RES) \
	$(APP$(TNR)IMP_ORD) \
	$(APP$(TNR)ICON) $(APP$(TNR)DEPN) $(USE_APP$(TNR)DEF)
	@echo ------------------------------
	@echo Making: $@
.IF "$(GUI)"=="UNX"
.IF "$(OS)"=="MACOSX"
	@echo unx
	@-$(RM) $(MISC)/$(@:b).list
	@-$(RM) $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
	@-$(RM) $(MISC)/$(@:b).strip
	@echo $(STDSLO) $(APP$(TNR)OBJS:s/.obj/.o/) \
	`cat /dev/null $(APP$(TNR)LIBS) | sed s\#$(ROUT)\#$(OUT)\#g` | tr -s " " "\n" > $(MISC)/$(@:b).list
	@echo -n $(APP$(TNR)LINKER) $(APP$(TNR)LINKFLAGS) $(LINKFLAGSAPP) -L$(PRJ)/$(INPATH)/lib $(SOLARLIB) -o $@ \
	$(APP$(TNR)LINKTYPEFLAG) $(APP$(TNR)STDLIBS) $(APP$(TNR)STDLIB) $(STDLIB$(TNR)) -filelist $(MISC)/$(@:b).list > $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
    @$(PERL) $(SOLARENV)/bin/macosx-dylib-link-list.pl \
        `cat $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd` \
        >> $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
	@cat $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
	@+source $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
# Need to strip __objcInit symbol to avoid duplicate symbols when loading
# libraries at runtime
	@-nm $@ | grep -v ' U ' | $(AWK) '{ print $$NF }' | grep -F -x '__objcInit' > $(MISC)/$(@:b).strip
	@strip -i -R $(MISC)/$(@:b).strip -X $@
	@ls -l $@
    @$(PERL) $(SOLARENV)/bin/macosx-change-install-names.pl \
        app $(APP$(TNR)RPATH) $@
.IF "$(TARGETTYPE)"=="GUI"
	@echo "Making: $@.app"
	@macosx-create-bundle $@
.ENDIF		# "$(TARGETTYPE)"=="GUI"
.ELSE		# "$(OS)"=="MACOSX"
	@echo unx
	@-$(RM) $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
	@echo $(APP$(TNR)LINKER) $(APP$(TNR)LINKFLAGS) $(LINKFLAGSAPP) -L$(PRJ)/$(INPATH)/lib $(SOLARLIB) $(STDSLO) \
	$(APP$(TNR)OBJS:s/.obj/.o/) '\' >  $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
	@cat $(mktmp /dev/null $(APP$(TNR)LIBS)) | xargs -n 1 cat | sed s\#$(ROUT)\#$(OUT)\#g | sed 's#$$# \\#'  >> $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
	@echo $(APP$(TNR)LINKTYPEFLAG) $(APP$(TNR)LIBSALCPPRT) $(APP$(TNR)STDLIBS) $(APP$(TNR)STDLIB) $(STDLIB$(TNR)) -o $@ >> $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
	cat $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
	@+source $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
	@ls -l $@
.ENDIF		# "$(OS)"=="MACOSX"
.ENDIF
.IF "$(GUI)" == "WNT"
	@@-$(MKDIR) $(@:d:d)
.IF "$(APP$(TNR)LINKRES)" != ""
	@@-$(RM) $(MISC)/$(APP$(TNR)LINKRES:b).rc
.IF "$(APP$(TNR)ICON)" != ""
	@-echo 1 ICON $(EMQ)"$(APP$(TNR)ICON:s/\/\\/)$(EMQ)" >> $(MISC)/$(APP$(TNR)LINKRES:b).rc
.ENDIF		# "$(APP$(TNR)ICON)" != ""
.IF "$(APP$(TNR)VERINFO)" != ""
	@-echo $(EMQ)#define VERVARIANT	$(BUILD) >> $(MISC)/$(APP$(TNR)LINKRES:b).rc
	@-echo $(EMQ)#include  $(EMQ)"$(APP$(TNR)VERINFO)$(EMQ)" >> $(MISC)/$(APP$(TNR)LINKRES:b).rc
.ENDIF		# "$(APP$(TNR)VERINFO)" != ""
	$(RC) -DWIN32 $(APP$(TNR)PRODUCTDEF) -I$(SOLARRESDIR) $(INCLUDE) $(RCLINKFLAGS) $(MISC)/$(APP$(TNR)LINKRES:b).rc
.ENDIF			# "$(APP$(TNR)LINKRES)" != ""
.IF "$(COM)" == "GCC"
	@echo mingw
.IF "$(APP$(TNR)LINKRES)" != "" || "$(APP$(TNR)RES)" != ""
	@cat $(APP$(TNR)LINKRES) $(subst,/res/,/res{$(subst,$(BIN), $(@:d))} $(APP$(TNR)RES)) >  $(MISC)/$(@:b)_all.res
	windres $(MISC)/$(@:b)_all.res $(APP$(TNR)RESO)
.ENDIF
	@echo $(LINK) $(LINKFLAGS) $(LINKFLAGSAPP) $(MINGWSSTDOBJ) -L$(PRJ)/$(INPATH)/lib $(SOLARLIB) $(STDSLO) \
		$(APP$(TNR)BASEX) $(APP$(TNR)STACKN) -o $@ $(APP$(TNR)OBJS) \
		-Wl,-Map,$(MISC)/$(@:b).map $(STDOBJ) $(APP$(TNR)RESO) \
		`$(TYPE) /dev/null $(APP$(TNR)LIBS) | sed s#$(ROUT)#$(OUT)#g` \
		$(APP_LINKTYPE) $(APP$(TNR)LIBSALCPPRT) \
		-Wl,--start-group $(APP$(TNR)STDLIBS) -Wl,--end-group $(APP$(TNR)STDLIB) \
		$(STDLIB$(TNR)) $(MINGWSSTDENDOBJ) > $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
	@$(TYPE)  $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
	@+source $(MISC)/$(TARGET).$(@:b)_$(TNR).cmd
	@ls -l $@
.ELSE	# "$(COM)" == "GCC"
.IF "$(linkinc)" == ""
	$(APP$(TNR)LINKER) @$(mktmp \
		$(APP$(TNR)LINKFLAGS) \
		$(LINKFLAGSAPP) $(APP$(TNR)BASEX) \
		$(APP$(TNR)STACKN) \
		-out:$@ \
		-map:$(MISC)/{$(subst,/,_ $(APP$(TNR)TARGET)).map} \
		$(STDOBJ) \
		$(APP$(TNR)LINKRES) \
		$(APP$(TNR)RES) \
		$(APP$(TNR)OBJS) \
		$(APP$(TNR)LIBS) \
		$(APP$(TNR)STDLIBS) \
		$(APP$(TNR)STDLIB) $(STDLIB$(TNR)) \
		)
	@-echo linking $@.manifest ...
.IF "$(VISTA_MANIFEST)"!=""
	$(IFEXIST) $@.manifest $(THEN) mt.exe -manifest $@.manifest -manifest $(TRUSTED_MANIFEST_LOCATION)/trustedinfo.manifest -out:$@.tmanifest$(EMQ) $(FI)
	$(IFEXIST) $@.manifest $(THEN) mt.exe -manifest $@.tmanifest -outputresource:$@$(EMQ);1 $(FI)
.ELSE
	$(IFEXIST) $@.manifest $(THEN) mt.exe -manifest $@.manifest -outputresource:$@$(EMQ);1 $(FI)
.ENDIF # "$(VISTA_MANIFEST)"!=""
	$(IFEXIST) $@.manifest $(THEN) $(RM:s/+//) $@.manifest $(FI)
	$(IFEXIST) $@.tmanifest $(THEN) $(RM:s/+//) $@.tmanifest $(FI)
.ELSE
		-$(RM) $(MISC)\$(APP$(TNR)TARGET).lnk
		-$(RM) $(MISC)\$(APP$(TNR)TARGET).lst
		-$(RM) $(MISC)\linkobj.lst
		for %_i in ($(MISC)\*.obj) do type %_i >> $(MISC)\linkobj.lst
	type $(mktmp,$(MISC)\$(APP$(TNR)TARGET).lst
		$(APP$(TNR)LINKFLAGS) \
		$(LINKFLAGSAPP) $(APP$(TNR)BASEX) \
		$(APP$(TNR)STACKN) \
		-out:$@ \
		$(STDOBJ) \
		$(APP$(TNR)LINKRES) \
		$(APP$(TNR)RES) \
		$(APP$(TNR)OBJS) \
		$(APP$(TNR)LIBS) \
		$(APP$(TNR)STDLIBS) \
		$(APP$(TNR)STDLIB) $(STDLIB$(TNR)))
		$(SED) -e 's/\(\.\.\\\)\{2,4\}/..\\/g' $(MISC)\$(APP$(TNR)TARGETN:b)_linkobj.lst >> $(MISC)\$(APP$(TNR)TARGET).lst
		$(IFEXIST) $(MISC)/$(APP$(TNR)TARGET).lst $(THEN) type $(MISC)/$(APP$(TNR)TARGET).lst  >> $(MISC)/$(APP$(TNR)TARGET).lnk $(FI)
		$(APP$(TNR)LINKER) @$(MISC)\$(APP$(TNR)TARGET).lnk
.ENDIF		# "$(linkinc)" == ""
.ENDIF		# "$(COM)" == "GCC"
.IF "$(APP$(TNR)TARGET)" == "loader"
	$(PERL) loader.pl $@
	$(TYPE) $(@) $(@:d)unloader.exe > $(@:d)_new.exe
	$(RM) $@
	$(RENAME) $(@:d)_new.exe $(@:d)loader.exe
.ENDIF			# "$(TARGET)" == "setup"

.ENDIF			# "$(GUI)" == "WNT"

.IF "$(GUI)" == "OS2"
	@+-$(MKDIR) $(@:d:d) >& $(NULLDEV)
.IF "$(APP$(TNR)LINKRES)" != ""
	@+-$(RM) $(MISC)/$(APP$(TNR)LINKRES:b).rc >& $(NULLDEV)
.IF "$(APP$(TNR)ICON)" != ""
	@-+$(WRAPCMD) echo 1 ICON $(EMQ)"$(APP$(TNR)ICON)$(EMQ)" | $(SED) 'sX\\X\\\\Xg' >> $(MISC)/$(APP$(TNR)LINKRES:b).rc
.ENDIF		# "$(APP$(TNR)ICON)" != ""
.IF "$(APP$(TNR)VERINFO)" != ""
	@-+echo $(EMQ)#define VERVARIANT	$(BUILD) >> $(MISC)/$(APP$(TNR)LINKRES:b).rc
	@-+echo $(EMQ)#include  $(EMQ)"$(APP$(TNR)VERINFO)$(EMQ)" >> $(MISC)/$(APP$(TNR)LINKRES:b).rc
.ENDIF		# "$(APP$(TNR)VERINFO)" != ""
	$(RC) -r -DOS2 $(APP$(TNR)PRODUCTDEF) -I$(SOLARRESDIR) $(INCLUDE) $(RCLINKFLAGS) $(MISC)/$(APP$(TNR)LINKRES:b).rc
.ENDIF			# "$(APP$(TNR)LINKRES)" != ""

.IF "$(TARGETTYPE)" == "GUI" 
	@echo NAME $(APP$(TNR)TARGET) WINDOWAPI > $(MISC)/$(APP$(TNR)TARGET).def
.ENDIF

	@+echo	$(APP$(TNR)LINKFLAGS) \
		$(LINKFLAGSAPP) $(APP$(TNR)BASEX) \
		$(APP$(TNR)STACKN) \
		-o $@ \
		-Zmap -L$(LB) \
		-L$(SOLARVERSION)/$(INPATH)/lib \
		$(STDOBJ) \
		$(APP$(TNR)LINKRES) \
		$(APP$(TNR)RES) \
		$(APP$(TNR)DEF) \
		$(APP$(TNR)OBJS) \
		$(APP$(TNR)LIBS) \
		$(APP$(TNR)STDLIBS:^"-l") \
		$(APP$(TNR)STDLIB:^"-l") $(STDLIB$(TNR):^"-l") 
	$(APP$(TNR)LINKER) -v \
		$(APP$(TNR)LINKFLAGS) \
		$(LINKFLAGSAPP) $(APP$(TNR)BASEX) \
		$(APP$(TNR)STACKN) \
		-o $@ \
		-Zmap -L$(LB) \
		-L$(SOLARVERSION)/$(INPATH)/lib \
		$(STDOBJ) \
		$(APP$(TNR)LINKRES) \
		$(APP$(TNR)RES) \
		$(APP$(TNR)DEF) \
		$(APP$(TNR)OBJS) \
		$(APP$(TNR)LIBS) \
		$(APP$(TNR)STDLIBS:^"-l") \
		$(APP$(TNR)STDLIB:^"-l") $(STDLIB$(TNR):^"-l") 


.IF "$(APP$(TNR)TARGET)" == "loader"
	+$(PERL) loader.pl $@
	+$(TYPE) $(@) $(@:d)unloader.exe > $(@:d)_new.exe
	+$(RM) $@
	+$(RENAME) $(@:d)_new.exe $(@:d)loader.exe
.ENDIF			# "$(TARGET)" == "setup"

.ENDIF			# "$(GUI)" == "OS2"

.ENDIF			# "$(APP$(TNR)TARGETN)"!=""


# Instruction for linking
# unroll end
#######################################################

