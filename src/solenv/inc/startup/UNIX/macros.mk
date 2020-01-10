# This file is used for all $(OS)==UNX during startup
#

SHELL *:= /usr/bin/csh
GROUPSHELL *:= $(SHELL)

.IF $(USE_SHELL) == bash
   SHELLFLAGS       *:= -c
.ELSE
   SHELLFLAGS       *:= -fc
.ENDIF # $(USE_SHELL) == bash

   GROUPFLAGS       *:= $(SHELLFLAGS)

# incomplete - better use METAS from startup.mk
#   SHELLMETAS       *:= "<>|/
   RM               *=  rm
   RMFLAGS          *=  -f
   MV	            *=  mv
