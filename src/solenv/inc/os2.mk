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

# --- OS2-Environment ----------------------------------------------

.IF "$(GUI)" == "OS2"

# YD defined in os2env.cmd
#.IF "$(NOSOLAR_JAVA)"==""
#SOLAR_JAVA=TRUE
#.ENDIF

.IF "$(SOLAR_JAVA)"!=""
JAVADEF=-DSOLAR_JAVA
.ENDIF
JAVAFLAGSDEBUG=-g

# --- Borland ---
.IF "$(COM)" == "BLC"

JAVADEF=-DSOLAR_JAVA

ASM=tasm
AFLAGS=

CXX=bcc
CC=bcc
CFLAGS=-c -3 -a1 -X -d -wbbf -weas -wucp -w-hid -w-par -I. $(MINUS_I)$(INCLUDE)
CFLAGSCXX=-Pcxx -RT- -x- -V
CFLAGSOBJGUIST=
CFLAGSOBJCUIST=
CFLAGSOBJGUIMT=-sm
CFLAGSOBJCUIMT=-sm
CFLAGSSLOGUIMT=-sm -sd
CFLAGSSLOCUIMT=-sm -sd
CFLAGSPROF=
CFLAGSDEBUG=-v
CFLAGSDBGUTIL=
CFLAGSOPT=-Os -Ob -k-
CFLAGSNOOPT=-Od
CFLAGSOUTOBJ=-o

LINK=tlink
LINKFLAGS=/m /L$(LIB)
#LINKFLAGSAPPGUI=/Toe /B:0x10000 /aa
#Base wg. lxopt raus
LINKFLAGSAPPGUI=/Toe /aa
LINKFLAGSSHLGUI=/Tod
LINKFLAGSAPPCUI=/Toe /B:0x10000 /ap
LINKFLAGSSHLCUI=/Tod
LINKFLAGSTACK=/S:
LINKFLAGSPROF=
LINKFLAGSDEBUG=/v
.IF "$(SOLAR_JAVA)"==""
LINKFLAGSOPT=/Oc
.ENDIF

STDOBJVCL=$(L)/salmain.obj
STDOBJGUI=c02.obj
STDSLOGUI=c02d.obj
STDOBJCUI=c02.obj
STDSLOCUI=c02d.obj
STDLIBGUIST=c2.lib os2.lib
STDLIBCUIST=c2.lib os2.lib
STDLIBGUIMT=c2mt.lib os2.lib
STDLIBCUIMT=c2mt.lib os2.lib
STDSHLGUIMT=c2mt.lib os2.lib
STDSHLCUIMT=c2mt.lib os2.lib

LIBMGR=tlib
LIBFLAGS=/C /P128

IMPLIB=implib
IMPLIBFLAGS=/c

MAPSYM=
MAPSYMFLAGS=

RC=rc
RCFLAGS=-r $(RCFILES) $@
RCLINK=rc
RCLINKFLAGS=
RCSETVERSION=

DLLPOSTFIX=bo

.ENDIF

# --- IBM ---
.IF "$(COM)" == "ICC"

ASM=tasm
AFLAGS=/ml /oi

CXX=icc
CC=icc
.IF "$(COMEX)"=="3"
CFLAGS=/C+ /Q+ /Gf+ /Sp1 /G4 /Se /Gs+ /Gt+ /Gd+ /J- /W2 /D__EXTENDED__ /Si+ /Xi+ $(MINUS_I)$(INCLUDE)  /Wvft-
.ELSE
.IF "$(COMEX)"=="I"
CFLAGS=/C+ /Tl10 /Q+ /Gf+ /Sp4 /G4 /Sc /Gs- /D__EXTENDED__ /Si+ /Su4
.ELSE
CFLAGS=/C+ /Tl10 /Q+ /Gf+ /Sp1 /G4 /Sc /Gs+ /D__EXTENDED__ /Si+
.ENDIF
.ENDIF

CFLAGSCXX=/Tdp 

CFLAGSEXCEPTIONS=-Gx-
CFLAGS_NO_EXCEPTIONS=-Gx+

CFLAGSOBJGUIST=/Ge+
CFLAGSOBJCUIST=/Ge+
.IF "$(COMEX)"=="I"
CFLAGSOBJGUIMT=/Ge+ /Gm+ 
CFLAGSOBJCUIMT=/Ge+ /Gm+
CFLAGSSLOGUIMT=/Ge- /Gm+
CFLAGSSLOCUIMT=/Ge- /Gm+
.ELSE
CFLAGSOBJGUIMT=/Ge+ /Gm+
CFLAGSOBJCUIMT=/Ge+ /Gm+
CFLAGSSLOGUIMT=/Ge- /Gm+
CFLAGSSLOCUIMT=/Ge- /Gm+
.ENDIF
CFLAGSPROF=/Gh+
CFLAGSDEBUG=/Ti+
CFLAGSDBGUTIL=
CFLAGSOPT=/O+ /Oi+ /Oc+
CFLAGSNOOPT=/O-
CFLAGSOUTOBJ=/Fo

CDEFS+=-D_STD_NO_NAMESPACE -D_VOS_NO_NAMESPACE -D_UNO_NO_NAMESPACE

LINK=ilink
#LINKFLAGS=/PACKCODE:8192 /ALIGN:16 /NOD /NOE /NOI /MAP /NOFREE

#bei too many segments ist /SEGMENTS:nnnn hilfreich. 3072 ist max!
.IF "$(CPPRTST)"!=""
LINKFLAGS=/NOFREE /NOD /NOE /NOI /MAP /OPTFUNC /PACKD:65536 /EXEPACK:2
.ELSE
LINKFLAGS=/NOFREE /NOD /NOE /NOI /MAP /OPTFUNC /PACKD:65536
.ENDIF
LINKFLAGSAPPGUI=/PM:PM /NOBASE
LINKFLAGSSHLGUI=
LINKFLAGSAPPCUI=/PM:VIO /NOBASE
LINKFLAGSSHLCUI=
LINKFLAGSTACK=/STACK:
LINKFLAGSPROF=
LINKFLAGSDEBUG=/COD
#LINKFLAGSOPT=/EXEPACK:2 /OPTFUNC
LINKFLAGSOPT=
#.IF "$(product)"!="full" && "$(product)"!="demo" && "$(product)"!="compact"
#LINKFLAGS=$(LINKFLAGS) /COD
#.ELSE
#LINKFLAGS=$(LINKFLAGS)
#.ENDIF

.IF "$(product)"=="full" || "$(product)"=="demo" || "$(product)"=="compact"
#	LINKFLAGS=$(LINKFLAGS)
.ELSE
LINKFLAGS+=/COD
.ENDIF

STDOBJVCL=$(L)/salmain.obj
STDOBJGUI=
STDSLOGUI=
STDOBJCUI=
STDSLOCUI=
.IF "$(COMEX)"=="3"
.IF "$(CPPRTST)"!=""
STDLIBGUIST=cppom30o.lib cpprtst.lib os2386.lib
STDLIBCUIST=cppom30o.lib cpprtst.lib os2386.lib
STDLIBGUIMT=cppom30o.lib cpprtst.lib os2386.lib
STDLIBCUIMT=cppom30o.lib cpprtst.lib os2386.lib
STDSHLGUIMT=cppom30o.lib cpprtst.lib os2386.lib
STDSHLCUIMT=cppom30o.lib cpprtst.lib os2386.lib
.ELSE
STDLIBGUIST=cppom30o.lib cppom30i.lib os2386.lib
STDLIBCUIST=cppom30o.lib cppom30i.lib os2386.lib
STDLIBGUIMT=cppom30o.lib cppom30i.lib os2386.lib
STDLIBCUIMT=cppom30o.lib cppom30i.lib os2386.lib
STDSHLGUIMT=cppom30o.lib cppom30i.lib os2386.lib
STDSHLCUIMT=cppom30o.lib cppom30i.lib os2386.lib
.ENDIF
.ELSE
STDLIBGUIST=dde4sbs.lib os2386.lib
STDLIBCUIST=dde4sbs.lib os2386.lib
STDLIBGUIMT=dde4mbs.lib os2386.lib
STDLIBCUIMT=dde4mbs.lib os2386.lib
STDSHLGUIMT=dde4mbs.lib os2386.lib
STDSHLCUIMT=dde4mbs.lib os2386.lib
.ENDIF

.IF "$(COMEX)"=="3"
LIBMGR=ilib
.ELSE
LIBMGR=lib
.ENDIF
LIBFLAGS=/NOI

IMPLIB=implib
IMPLIBFLAGS=/noi

MAPSYM=
MAPSYMFLAGS=

RC=rc
RCFLAGS=-r $(RCFILES) $@
RCLINK=rc
RCLINKFLAGS=
RCSETVERSION=

DLLPOSTFIX=co

.ENDIF

# --- GNU ---
.IF "$(COM)" == "GCC"

.INCLUDE : os2gcci.mk

.ENDIF

# --- Watcom ---
.IF "$(COM)" == "WTC"

ASM=wasm
AFLAGS=/ml /4pr

CC=wcl386
CXX=wcl386
.IF "$(e2p)" != ""
CFLAGS=-c -W3 -Zp4 -Zld $(MINUS_I)$(INCLUDE) -bt=os2 -zq  -zm -ep -ee
.ELSE
CFLAGS=-c -Zp4 -W3 -Zl -Zld $(MINUS_I)$(INCLUDE) -bt=os2 -zq -s
.ENDIF
CFLAGSCXX=-cc++ -xst
CFLAGSOBJGUIST=          #-Alfd -GA -GEfs
CFLAGSOBJCUIST=
CFLAGSOBJGUIMT=-bm          #-Alfw -GA -GEd
CFLAGSOBJCUIMT=-bm
CFLAGSSLOGUIMT=-bm -bd              #-Alfw -GD -GEfd
CFLAGSSLOCUIMT=-bm -bd
CFLAGSPROF=
CFLAGSDEBUG=/d2
CFLAGSDBGUTIL=
.IF "$(e2p)" != ""
CFLAGSOPT=-otexan -3s
CFLAGSNOOPT=-od -3s
.ELSE
CFLAGSOPT=-otexan -4s
CFLAGSNOOPT=-od -4s
.ENDIF
CFLAGSOUTOBJ=-Fo

LINK=wlink
LINKFLAGS=op symf op caseexact op statics op MANY
LINKFLAGSAPPGUI=sys os2v2 pm
LINKFLAGSSHLGUI=sys os2v2 dll INITINSTANCE TERMINSTANCE
LINKFLAGSAPPCUI=sys os2v2
LINKFLAGSSHLCUI=sys os2v2 dll
LINKFLAGSTACK=op stack=
LINKFLAGSPROF=
LINKFLAGSDEBUG=debug all op undefsok
LINKFLAGSOPT=

STDOBJVCL=$(L)/salmain.obj
STDOBJGUI=libr clib3s.lib libr plib3s.lib libr math387s.lib
STDSLOGUI=libr clib3s.lib libr os2386.lib libr plib3s.lib libr math387s.lib
STDOBJCUI=libr clib3s.lib libr plib3s.lib libr math387s.lib
STDSLOCUI=libr clib3s.lib libr plib3s.lib libr math387s.lib
STDLIBGUIST=libr os2386.lib libr clib3s.lib libr plib3s.lib
STDLIBCUIST=libr os2386.lib libr clib3s.lib libr plib3s.lib
STDLIBGUIMT=libr os2386.lib libr clib3s.lib libr plib3s.lib
STDLIBCUIMT=libr os2386.lib libr clib3s.lib libr plib3s.lib
STDSHLGUIMT=libr os2386.lib libr clib3s.lib libr plibmt3s.lib libr math387s.lib libr plib3s.lib libr noemu387.lib
STDSHLCUIMT=libr os2386.lib libr clib3s.lib libr plibmt3s.lib libr matg387s.lib libr plib3s.lib

LIBMGR=wlib
LIBFLAGS=/p=128 /c /m

IMPLIB=echo
IMPLIBFLAGS=

MAPSYM=mapsym
MAPSYMFLAGS=

RC=rc
RCFLAGS=-r $(RCFILES) $@
RCLINK=rc
RCLINKFLAGS=
RCSETVERSION=

DLLPOSTFIX=wo

.ENDIF

# --- OS2 Allgemein ---
HC=toipf
HCFLAGS=
PATH_SEPERATOR*=;
DLLPRE=
DLLPOST=.dll
EXECPOST=.exe
SCPPOST=.ins
DLLDEST=$(BIN)
SOLARSHAREDBIN=$(SOLARBINDIR)

.ENDIF
