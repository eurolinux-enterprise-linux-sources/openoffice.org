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
#include "precompiled_svx.hxx"

#ifdef SVX_DLLIMPLEMENTATION
#undef SVX_DLLIMPLEMENTATION
#endif

// include ---------------------------------------------------------------
#include <vcl/wrkwin.hxx>
#include <vcl/morebtn.hxx>
#include <vcl/msgbox.hxx>
#include <svtools/slstitm.hxx>
#include <svtools/itemiter.hxx>
#include <svtools/style.hxx>
#include <svtools/searchopt.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/module.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/basedlgs.hxx>
#include <svtools/cjkoptions.hxx>
#include <com/sun/star/i18n/TransliterationModules.hpp>

#define _CUI_SRCHDLG_CXX
#include "cuisrchdlg.hxx"

#include <svx/dialogs.hrc>
#include <svx/svxitems.hrc>
//#include "srchdlg.hrc"


#define	ITEMID_SETITEM		0

#include <sfx2/srchitem.hxx>
#include <svx/pageitem.hxx>
//#include "srchctrl.hxx"
//CHINA001 #include "srchxtra.hxx"
#include <svx/dialmgr.hxx>
#include "dlgutil.hxx"
#include <optjsearch.hxx>
#include <svx/brshitem.hxx>
#include "backgrnd.hxx"


// class SvxJSearchOptionsDialog -----------------------------------------

SvxJSearchOptionsDialog::SvxJSearchOptionsDialog(
			Window *pParent,
            const SfxItemSet& rOptionsSet, USHORT /*nUniqueId*/, INT32 nInitialFlags ) :
	SfxSingleTabDialog	( pParent, rOptionsSet, RID_SVXPAGE_JSEARCH_OPTIONS ),
	nInitialTlFlags( nInitialFlags )
{
	pPage = (SvxJSearchOptionsPage *)
					SvxJSearchOptionsPage::Create( this, rOptionsSet );
	SetTabPage( pPage );	//! implicitly calls pPage->Reset(...)!
	pPage->EnableSaveOptions( FALSE );
}


SvxJSearchOptionsDialog::~SvxJSearchOptionsDialog()
{
	// pPage will be implicitly destroyed by the
	// SfxSingleTabDialog destructor
}


void SvxJSearchOptionsDialog::Activate()
{
	pPage->SetTransliterationFlags( nInitialTlFlags );
}


INT32 SvxJSearchOptionsDialog::GetTransliterationFlags() const
{
	return pPage->GetTransliterationFlags();
}


void SvxJSearchOptionsDialog::SetTransliterationFlags( INT32 nSettings )
{
	pPage->SetTransliterationFlags( nSettings );
}
