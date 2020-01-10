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


# mak file fuer wnticci
ASM=
AFLAGS=

CDEFS+=-D_PTHREADS -D_REENTRANT 
CDEFS+=-D_STD_NO_NAMESPACE -D_VOS_NO_NAMESPACE -D_UNO_NO_NAMESPACE -DX86 -DNEW_SOLAR
# kann c++ was c braucht??

# architecture dependent flags for the C and C++ compiler that can be changed by
# exporting the variable ARCH_FLAGS="..." in the shell, which is used to start build
ARCH_FLAGS*=-mpentium

CXX*=/nw386/dev/s/solenv/unxscoi/bin/g++
CC*=/nw386/dev/s/solenv/unxscoi/bin/gcc
CFLAGS=-c
CFLAGSCC=$(ARCH_FLAGS)
CFLAGSEXCEPTIONS=-fexceptions
CFLAGS_NO_EXCEPTIONS=-fno-exceptions
CFLAGSCXX=-fguiding-decls -frtti $(ARCH_FLAGS)
PICSWITCH:=-fPIC
CFLAGSOBJGUIMT=$(PICSWITCH)
CFLAGSOBJCUIMT=$(PICSWITCH)
CFLAGSSLOGUIMT=$(PICSWITCH)
CFLAGSSLOCUIMT=$(PICSWITCH)
CFLAGSPROF=
CFLAGSDEBUG=-g
CFLAGSDBGUTIL=
CFLAGSOPT=-O2
CFLAGSNOOPT=-O
CFLAGSOUTOBJ=-o

CFLAGSWARNCC=
CFLAGSWARNCXX=$(CFLAGSWARNCC) -Wno-ctor-dtor-privacy
# -Wshadow does not work for C with nested uses of pthread_cleanup_push:
CFLAGSWALLCC=-Wall -Wextra -Wendif-labels
CFLAGSWALLCXX=$(CFLAGSWALLCC) -Wshadow -Wno-ctor-dtor-privacy
CFLAGSWERRCC=-Werror

STATIC		= -Wl,-Bstatic
DYNAMIC		= -Wl,-Bdynamic

THREADLIB=
LINK=/nw386/dev/s/solenv/unxscoi/bin/gcc
LINKFLAGS=
# SCO hat grosse Probleme mit fork/exec und einigen shared libraries
# rsc2 muss daher statisch gelinkt werden
.IF "$(PRJNAME)"=="rsc"
LINKFLAGSAPPGUI=-L/nw386/dev/s/solenv/unxscoi/lib $(STATIC) -lpthread_init $(DYNAMIC)
LINKFLAGSAPPCUI=-L/nw386/dev/s/solenv/unxscoi/lib $(STATIC) -lpthread_init $(DYNAMIC)
.ELSE
LINKFLAGSAPPGUI=-L/nw386/dev/s/solenv/unxscoi/lib -lpthread_init
LINKFLAGSAPPCUI=-L/nw386/dev/s/solenv/unxscoi/lib -lpthread_init
.ENDIF
LINKFLAGSSHLGUI=-G -W,l,-Bsymbolic
LINKFLAGSSHLCUI=-G -W,l,-Bsymbolic
LINKFLAGSTACK=
LINKFLAGSPROF=
LINKFLAGSDEBUG=-g
LINKFLAGSOPT=

# standard C++ Library
#
# das statische dazulinken der libstdc++ macht jede shared library um 50k
# (ungestrippt) oder so groesser, auch wenn sie ueberhaupt nicht gebraucht
# wird. Da muessen wir uns was besseres ueberlegen.
STDLIBCPP=-Wl,-Bstatic -lstdc++ -Wl,-Bdynamic

# reihenfolge der libs NICHT egal!
STDOBJGUI=
STDSLOGUI=
STDOBJCUI=
STDSLOCUI=
.IF "$(PRJNAME)"=="rsc"
STDLIBGUIMT=-lXext -lX11 $(STATIC) -lpthread $(DYNAMIC) -ldl -lsocket -lm
STDLIBCUIMT=$(STATIC) -lpthread $(DYNAMIC) -ldl -lsocket -lm 
.ELSE
STDLIBGUIMT=-lXext -lX11 -lpthread -ldl -lsocket -lm
STDLIBCUIMT=-lpthread -ldl -lsocket -lm 
.ENDIF
#STDSHLGUIMT=-lXext -lX11 -lpthread -ldl -lsocket -lm
#STDSHLCUIMT=-lpthread -ldl -lsocket -lm 

STDLIBCPP= -lstdc++
SHLLINKARCONLY=yes

LIBMGR=ar
LIBFLAGS=-r
# LIBEXT=.so

IMPLIB=
IMPLIBFLAGS=

MAPSYM=
MAPSYMFLAGS=

RC=irc
RCFLAGS=-fo$@ $(RCFILES)
RCLINK=
RCLINKFLAGS=
RCSETVERSION=

DLLPOSTFIX=ci
DLLPRE=lib
DLLPOST=.so

LDUMP=cppfilt /b /n /o /p

.IF "$(WORK_STAMP)"!="MIX364"
DLLPOSTFIX=ci
.ELSE
DLLPOSTFIX=
.ENDIF
