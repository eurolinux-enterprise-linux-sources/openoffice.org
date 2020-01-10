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

PRJNAME=testtools
TARGET=bridgetest
USE_DEFFILE=TRUE
ENABLE_EXCEPTIONS=TRUE
LIBTARGET=NO

.INCLUDE: settings.mk
.IF "$(L10N_framework)"==""
DLLPRE = # no leading "lib" on .so files

.IF "$(GUI)"=="WNT"
BATCH_SUFFIX=.bat
GIVE_EXEC_RIGHTS=@echo
MY_URE_INTERNAL_JAVA_DIR=$(strip $(subst,\,/ file:///$(shell @$(WRAPCMD) echo $(SOLARBINDIR))))
MY_LOCAL_CLASSDIR=$(strip $(subst,\,/ file:///$(shell $(WRAPCMD) echo $(PWD)$/$(CLASSDIR))))
.ELIF "$(GUI)"=="OS2"
BATCH_SUFFIX=.cmd
GIVE_EXEC_RIGHTS=@echo
MY_URE_INTERNAL_JAVA_DIR=$(strip $(subst,\,/ file:///$(shell @$(WRAPCMD) echo $(SOLARBINDIR))))
MY_LOCAL_CLASSDIR=$(strip $(subst,\,/ file:///$(shell $(WRAPCMD) echo $(PWD)$/$(CLASSDIR))))
.ELSE
BATCH_INPROCESS=bridgetest_inprocess
GIVE_EXEC_RIGHTS=chmod +x 
MY_URE_INTERNAL_JAVA_DIR=file://$(SOLARBINDIR)
MY_LOCAL_CLASSDIR=file://$(PWD)$/$(CLASSDIR)
.ENDIF

.IF "$(GUI)"=="WNT"
.IF "$(compcheck)" != ""
CFLAGSCXX += -DCOMPCHECK
.ENDIF
.ENDIF

SLOFILES = \
    $(SLO)$/bridgetest.obj \
    $(SLO)$/cppobj.obj \
    $(SLO)$/currentcontextchecker.obj \
    $(SLO)$/multi.obj

# ---- test ----

LIB1TARGET=$(SLB)$/cppobj.lib
LIB1OBJFILES= \
		$(SLO)$/cppobj.obj $(SLO)$/currentcontextchecker.obj $(SLO)$/multi.obj

SHL1TARGET = cppobj.uno
SHL1STDLIBS= \
		$(CPPULIB)		\
		$(CPPUHELPERLIB)	\
		$(SALLIB)

SHL1LIBS=	$(LIB1TARGET)
SHL1DEF=	$(MISC)$/$(SHL1TARGET).def
DEF1NAME=	$(SHL1TARGET)
.IF "$(COMNAME)" == "gcc3"
SHL1VERSIONMAP = component.gcc3.map
.ELSE
SHL1VERSIONMAP = component.map
.ENDIF

# ---- test object ----

LIB2TARGET=$(SLB)$/bridgetest.lib
LIB2OBJFILES= \
        $(SLO)$/bridgetest.obj \
        $(SLO)$/currentcontextchecker.obj \
        $(SLO)$/multi.obj

SHL2TARGET = bridgetest.uno
SHL2STDLIBS= \
		$(CPPULIB)		\
		$(CPPUHELPERLIB)	\
		$(SALLIB)

SHL2LIBS=	$(LIB2TARGET)
SHL2DEF=	$(MISC)$/$(SHL2TARGET).def
DEF2NAME=	$(SHL2TARGET)
.IF "$(COMNAME)" == "gcc3"
SHL2VERSIONMAP = component.gcc3.map
.ELSE
SHL2VERSIONMAP = component.map
.ENDIF

SHL3TARGET = constructors.uno
SHL3OBJS = $(SLO)$/constructors.obj
SHL3STDLIBS = $(CPPULIB) $(CPPUHELPERLIB) $(SALLIB)
SHL3VERSIONMAP = component.map
SHL3IMPLIB = i$(SHL3TARGET)
DEF3NAME = $(SHL3TARGET)

.IF "$(SOLAR_JAVA)" != ""
JARFILES = java_uno.jar jurt.jar ridl.jar
JAVATARGETS=\
	$(DLLDEST)$/bridgetest_javaserver$(BATCH_SUFFIX) \
	$(DLLDEST)$/bridgetest_inprocess_java$(BATCH_SUFFIX)
.ENDIF

# --- Targets ------------------------------------------------------
.ENDIF # L10N_framework

.INCLUDE :	target.mk
.IF "$(L10N_framework)"==""
ALLTAR: \
		test \
		$(DLLDEST)$/uno_types.rdb \
		$(DLLDEST)$/uno_services.rdb \
		$(DLLDEST)$/bridgetest_inprocess$(BATCH_SUFFIX) \
		$(DLLDEST)$/bridgetest_server$(BATCH_SUFFIX) \
		$(DLLDEST)$/bridgetest_client$(BATCH_SUFFIX) \
		$(JAVATARGETS)

#################################################################

test: 
	echo $(compcheck)
	
$(DLLDEST)$/uno_types.rdb : $(SOLARBINDIR)$/udkapi.rdb
	echo $(DLLDEST)
	$(GNUCOPY) $? $@
    $(REGMERGE) $@ / $(BIN)$/bridgetest.rdb

$(DLLDEST)$/bridgetest_inprocess$(BATCH_SUFFIX) .ERRREMOVE: makefile.mk
.IF "$(USE_SHELL)" == "bash"
    echo '$(AUGMENT_LIBRARY_PATH)' uno -ro uno_services.rdb -ro uno_types.rdb \
        -s com.sun.star.test.bridge.BridgeTest -- \
        com.sun.star.test.bridge.CppTestObject > $@
.ELSE
    echo ERROR: this script can only be created properly for USE_SHELL=bash > $@
.ENDIF
	$(GIVE_EXEC_RIGHTS) $@

$(DLLDEST)$/bridgetest_client$(BATCH_SUFFIX) .ERRREMOVE: makefile.mk
.IF "$(USE_SHELL)" == "bash"
    echo '$(AUGMENT_LIBRARY_PATH)' uno -ro uno_services.rdb -ro uno_types.rdb \
        -s com.sun.star.test.bridge.BridgeTest -- \
        -u \''uno:socket,host=127.0.0.1,port=2002;urp;test'\' > $@
.ELSE
    echo ERROR: this script can only be created properly for USE_SHELL=bash > $@
.ENDIF
	$(GIVE_EXEC_RIGHTS) $@

$(DLLDEST)$/bridgetest_server$(BATCH_SUFFIX) .ERRREMOVE: makefile.mk
.IF "$(USE_SHELL)" == "bash"
    echo '$(AUGMENT_LIBRARY_PATH)' uno -ro uno_services.rdb -ro uno_types.rdb \
        -s com.sun.star.test.bridge.CppTestObject \
        -u \''uno:socket,host=127.0.0.1,port=2002;urp;test'\' --singleaccept \
        > $@
.ELSE
    echo ERROR: this script can only be created properly for USE_SHELL=bash > $@
.ENDIF
	$(GIVE_EXEC_RIGHTS) $@


.IF "$(SOLAR_JAVA)" != ""
# jar-files, which regcomp needs so that it can use java
MY_JARS=java_uno.jar ridl.jar jurt.jar juh.jar

# CLASSPATH, which regcomp needs to be run
MY_CLASSPATH_TMP=$(foreach,i,$(MY_JARS) $(SOLARBINDIR)$/$i)$(PATH_SEPERATOR)$(XCLASSPATH)
MY_CLASSPATH=$(strip $(subst,!,$(PATH_SEPERATOR) $(MY_CLASSPATH_TMP:s/ /!/)))$(PATH_SEPERATOR)..$/class

# Use "127.0.0.1" instead of "localhost", see #i32281#:
$(DLLDEST)$/bridgetest_javaserver$(BATCH_SUFFIX) : makefile.mk
	-rm -f $@
	echo java -classpath "$(MY_CLASSPATH)$(PATH_SEPERATOR)..$/class$/testComponent.jar" \
		com.sun.star.comp.bridge.TestComponentMain \""uno:socket,host=127.0.0.1,port=2002;urp;test"\" singleaccept > $@
	$(GIVE_EXEC_RIGHTS) $@

$(DLLDEST)$/bridgetest_inprocess_java$(BATCH_SUFFIX) .ERRREMOVE: makefile.mk
.IF "$(USE_SHELL)" == "bash"
	echo '$(AUGMENT_LIBRARY_PATH)' uno -ro uno_services.rdb -ro uno_types.rdb \
        -s com.sun.star.test.bridge.BridgeTest \
        -env:URE_INTERNAL_JAVA_DIR=$(MY_URE_INTERNAL_JAVA_DIR) \
        -- com.sun.star.test.bridge.JavaTestObject noCurrentContext > $@
.ELSE
    echo ERROR: this script can only be created properly for USE_SHELL=bash > $@
.ENDIF
	$(GIVE_EXEC_RIGHTS) $@
.ENDIF

$(DLLDEST)$/uno_services.rdb .ERRREMOVE: $(DLLDEST)$/uno_types.rdb \
        $(DLLDEST)$/bridgetest.uno$(DLLPOST) $(DLLDEST)$/cppobj.uno$(DLLPOST) \
        $(MISC)$/$(TARGET)$/bootstrap.rdb $(SHL3TARGETN)
    - $(MKDIR) $(@:d)
    $(REGCOMP) -register -br $(DLLDEST)$/uno_types.rdb \
        -r $(DLLDEST)$/uno_services.rdb -wop \
        -c acceptor.uno$(DLLPOST) \
        -c bridgefac.uno$(DLLPOST) \
        -c connector.uno$(DLLPOST) \
        -c remotebridge.uno$(DLLPOST) \
        -c uuresolver.uno$(DLLPOST) \
        -c stocservices.uno$(DLLPOST)
    cd $(DLLDEST) && $(REGCOMP) -register -br uno_types.rdb \
        -r uno_services.rdb -wop=./ \
        -c .$/bridgetest.uno$(DLLPOST) \
        -c .$/cppobj.uno$(DLLPOST) \
        -c .$/$(SHL3TARGETN:f)
.IF "$(SOLAR_JAVA)" != ""
    $(REGCOMP) -register -br $(DLLDEST)$/uno_types.rdb -r $@ \
        -c javaloader.uno$(DLLPOST) -c javavm.uno$(DLLPOST)
    $(REGCOMP) -register  -br $(MISC)$/$(TARGET)$/bootstrap.rdb -r $@ -c \
        $(MY_LOCAL_CLASSDIR)/testComponent.jar \
        -env:URE_INTERNAL_JAVA_DIR=$(MY_URE_INTERNAL_JAVA_DIR)
.ENDIF

$(MISC)$/$(TARGET)$/bootstrap.rdb .ERRREMOVE:
    - $(MKDIR) $(@:d)
     $(COPY) $(SOLARBINDIR)$/types.rdb $@
.IF "$(SOLAR_JAVA)" != ""
    $(REGCOMP) -register -r $@ -c javaloader.uno$(DLLPOST) \
        -c javavm.uno$(DLLPOST) -c stocservices.uno$(DLLPOST)
.ENDIF
.ENDIF # L10N_framework

