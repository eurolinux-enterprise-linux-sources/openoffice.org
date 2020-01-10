/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * Copyright 2000, 2010 Oracle and/or its affiliates.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * This file is part of OpenOffice.org.
 *
 * OpenOffice.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * OpenOffice.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with OpenOffice.org.  If not, see
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sc.hxx"



//------------------------------------------------------------------

#include <svx/eeitem.hxx>
#include <svx/fontwork.hxx>
//CHINA001 #include <svx/labdlg.hxx>
#include <svx/srchitem.hxx>
#include <svx/tabarea.hxx>
#include <svx/tabline.hxx>
//CHINA001 #include <svx/transfrm.hxx>
#include <sfx2/app.hxx>
#include <sfx2/objface.hxx>
#include <sfx2/request.hxx>
#include <svtools/whiter.hxx>
#include <vcl/msgbox.hxx>

#include "drformsh.hxx"
#include "drwlayer.hxx"
#include "sc.hrc"
#include "viewdata.hxx"
#include "document.hxx"
#include "docpool.hxx"
#include "drawview.hxx"
#include "scresid.hxx"
#include <svx/svdobj.hxx>

#define ScDrawFormShell
#include "scslots.hxx"


SFX_IMPL_INTERFACE(ScDrawFormShell, ScDrawShell, ScResId(SCSTR_DRAWFORMSHELL) )
{
	SFX_OBJECTBAR_REGISTRATION( SFX_OBJECTBAR_OBJECT|SFX_VISIBILITY_STANDARD|SFX_VISIBILITY_SERVER,
								ScResId(RID_OBJECTBAR_FORMAT) );
	SFX_POPUPMENU_REGISTRATION( ScResId(RID_POPUP_DRAWFORM) );
}

TYPEINIT1( ScDrawFormShell, ScDrawShell );

ScDrawFormShell::ScDrawFormShell(ScViewData* pData) :
	ScDrawShell(pData)
{
	SetHelpId(HID_SCSHELL_DRAWFORMSH);
	SetName(String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("DrawForm")));
}

ScDrawFormShell::~ScDrawFormShell()
{
}



