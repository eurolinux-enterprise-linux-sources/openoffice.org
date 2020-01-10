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
# unroll begin

.IF "$(RESLIB$(TNR)TARGETN)"!=""

.IF "$(BUILDHIDS)"!=""
HIDRES$(TNR)PARTICLE=$(subst,$(OUTPATH),$(COMMON_OUTDIR) $(MISC))/$(RESLIB$(TNR)NAME)_res.hid

#HACK cut off the dirty srs files which are included from solver
RESLIB$(TNR)HIDFILES:=$(subst,$(OUTPATH),$(COMMON_OUTDIR) $(subst,.srs,_srs.hid $(RESLIB$(TNR)SRSFILES)))
$(HIDRES$(TNR)PARTICLE): $(RESLIB$(TNR)HIDFILES)
	@echo ------------------------------
	@echo Making: $@
	@$(IFEXIST) $@ $(THEN) $(RM:s/+//) $@ $(FI)
# need to strip since solaris cannot handle tab-only whitespace here
    $(TYPE) $(mktmp  $(strip, $(subst,/,/ $(RESLIB$(TNR)HIDFILES))) )| xargs -s 1000 cat > $@.$(ROUT).tmp
	@$(RENAME) $@.$(ROUT).tmp $@

ALLTAR : $(HIDRES$(TNR)PARTICLE)

.ENDIF # "$(BUILDHIDS)"!=""

$(RSC_MULTI$(TNR)) : \
		$(RESLIB$(TNR)SRSFILES) \
		$(RESLIB$(TNR)TARGETN) \
		$(RESLIB$(TNR)BMPS)
    @echo using rsc multi-res feature
.IF "$(common_build_reslib)"!=""
	$(RSC) -presponse @$(mktmp \
	-r -p \
	$(foreach,i,$(alllangiso) -lg$i \
	$(null,$(rescharset_{$i}) $(default$(LANG_GUI)) $(rescharset_{$i})) \
	-fs={$(subst,$(OUTPATH),$(COMMON_OUTDIR) $(BIN))/$(RESLIB$(TNR)NAME)$i.res} \
    $(foreach,j,$(subst,$(PRJ),$(SOLARSRC)/$(RSCDEFIMG)/$(PRJNAME) $(RESLIB$(TNR)IMAGES)) -lip={$j}/$i \
    -lip={$j} ) \
    -lip=$(SOLARSRC)/$(RSCDEFIMG)/res/$i -lip=$(SOLARSRC)/$(RSCDEFIMG)/res ) \
	-subMODULE=$(SOLARSRC)/$(RSCDEFIMG) \
	-subGLOBALRES=$(SOLARSRC)/$(RSCDEFIMG)/res \
    -oil={$(subst,$(OUTPATH),$(COMMON_OUTDIR) $(BIN))} \
    -ft=$@ \
	$(RSC$(TNR)HEADER) $(RESLIB$(TNR)SRSFILES) \
	) > $(NULLDEV)
.ELSE			# "$(common_build_reslib)"!=""
	$(RSC) -presponse @$(mktmp \
	-r -p \
	$(foreach,i,$(alllangiso) -lg$i \
	$(null,$(rescharset_{$i}) $(default$(LANG_GUI)) $(rescharset_{$i})) \
	-fs={$(BIN)/$(RESLIB$(TNR)NAME)$i.res} \
    $(foreach,j,$(subst,$(PRJ),$(SOLARSRC)/$(RSCDEFIMG)/$(PRJNAME) $(RESLIB$(TNR)IMAGES)) -lip={$j}/$i \
    -lip={$j} ) \
    -lip=$(SOLARSRC)/$(RSCDEFIMG)/res/$i -lip=$(SOLARSRC)/$(RSCDEFIMG)/res ) \
	-subMODULE=$(SOLARSRC)/$(RSCDEFIMG) \
	-subGLOBALRES=$(SOLARSRC)/$(RSCDEFIMG)/res \
    -oil=$(BIN) \
    -ft=$@ \
	$(RSC$(TNR)HEADER) $(RESLIB$(TNR)SRSFILES) \
	) > $(NULLDEV)
.ENDIF			# "$(common_build_reslib)"!=""

$(RESLIB$(TNR)TARGETN): \
		$(RESLIB$(TNR)SRSFILES) \
		$(RESLIB$(TNR)BMPS)
	@echo Making: $@
.IF "$(GUI)"=="UNX"
.IF "$(common_build_reslib)"!=""
    @@-$(RM) $(RSC_MULTI$(TNR))
.ELSE			# "$(common_build_reslib)"!=""
    @@-$(RM) $(RSC_MULTI$(TNR))
.ENDIF			# "$(common_build_reslib)"!=""
.ELSE				# "$(GUI)"=="UNX"
.IF "$(common_build_reslib)"!=""
    @@-$(RM) $(RSC_MULTI$(TNR))
.ELSE			# "$(common_build_reslib)"!=""
    @@-$(RM) $(RSC_MULTI$(TNR))
.ENDIF			# "$(common_build_reslib)"!=""
.ENDIF              # "$(GUI)"=="UNX"
.ENDIF				# "$(RESLIB$(TNR)TARGETN)"!=""


# unroll end
#######################################################

