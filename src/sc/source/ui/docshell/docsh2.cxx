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


#include <svx/svdpage.hxx>


#include <svx/xtable.hxx>

#include "scitems.hxx"
#include <tools/gen.hxx>
#include <svtools/ctrltool.hxx>
#include <svx/flstitem.hxx>
#include <svx/drawitem.hxx>
#include <sfx2/printer.hxx>
#include <svtools/smplhint.hxx>
#include <svx/svditer.hxx>
#include <svx/svdobj.hxx>
#include <svx/svdoole2.hxx>
#include <vcl/svapp.hxx>
#include <svx/asiancfg.hxx>
#include <svx/forbiddencharacterstable.hxx>
#include <svx/unolingu.hxx>
#include <rtl/logfile.hxx>



// INCLUDE ---------------------------------------------------------------
/*
#include <svdrwetc.hxx>
#include <svdrwobx.hxx>
#include <sostor.hxx>
*/
#include "drwlayer.hxx"
#include "stlpool.hxx"
#include "docsh.hxx"
#include "docshimp.hxx"
#include "docfunc.hxx"
#include "sc.hrc"

using namespace com::sun::star;

//------------------------------------------------------------------

BOOL __EXPORT ScDocShell::InitNew( const uno::Reference < embed::XStorage >& xStor )
{
	RTL_LOGFILE_CONTEXT_AUTHOR ( aLog, "sc", "nn93723", "ScDocShell::InitNew" );

    BOOL bRet = SfxObjectShell::InitNew( xStor );

	aDocument.MakeTable(0);
	//	zusaetzliche Tabellen werden von der ersten View angelegt,
	//	wenn bIsEmpty dann noch TRUE ist

	if( bRet )
	{
		Size aSize( (long) ( STD_COL_WIDTH			 * HMM_PER_TWIPS * OLE_STD_CELLS_X ),
					(long) ( ScGlobal::nStdRowHeight * HMM_PER_TWIPS * OLE_STD_CELLS_Y ) );
		// hier muss auch der Start angepasst werden
		SetVisAreaOrSize( Rectangle( Point(), aSize ), TRUE );
	}

    aDocument.SetDrawDefaults();        // drawing layer defaults that are set only in InitNew

	// InitOptions sets the document languages, must be called before CreateStandardStyles
	InitOptions();

	aDocument.GetStyleSheetPool()->CreateStandardStyles();
	aDocument.UpdStlShtPtrsFrmNms();

	//	SetDocumentModified ist in Load/InitNew nicht mehr erlaubt!

	InitItems();
	CalcOutputFactor();

	return bRet;
}

//------------------------------------------------------------------

BOOL ScDocShell::IsEmpty() const
{
	return bIsEmpty;
}


void ScDocShell::SetEmpty(BOOL bSet)
{
	bIsEmpty = bSet;
}

//------------------------------------------------------------------

void ScDocShell::InitItems()
{
	// AllItemSet fuer Controller mit benoetigten Items fuellen:

    // if ( pImpl->pFontList )
    //  delete pImpl->pFontList;

	//	Druck-Optionen werden beim Drucken und evtl. in GetPrinter gesetzt

    // pImpl->pFontList = new FontList( GetPrinter(), Application::GetDefaultDevice() );
    //PutItem( SvxFontListItem( pImpl->pFontList, SID_ATTR_CHAR_FONTLIST ) );
	UpdateFontList();

	ScDrawLayer* pDrawLayer = aDocument.GetDrawLayer();
	if (pDrawLayer)
	{
        PutItem( SvxColorTableItem  ( pDrawLayer->GetColorTable(), SID_COLOR_TABLE ) );
        PutItem( SvxGradientListItem( pDrawLayer->GetGradientList(), SID_GRADIENT_LIST ) );
        PutItem( SvxHatchListItem   ( pDrawLayer->GetHatchList(), SID_HATCH_LIST ) );
        PutItem( SvxBitmapListItem  ( pDrawLayer->GetBitmapList(), SID_BITMAP_LIST ) );
        PutItem( SvxDashListItem    ( pDrawLayer->GetDashList(), SID_DASH_LIST ) );
        PutItem( SvxLineEndListItem ( pDrawLayer->GetLineEndList(), SID_LINEEND_LIST ) );

			//	andere Anpassungen nach dem Anlegen des DrawLayers

		pDrawLayer->SetNotifyUndoActionHdl( LINK( pDocFunc, ScDocFunc, NotifyDrawUndo ) );

		//if (SfxObjectShell::HasSbxObject())
		pDrawLayer->UpdateBasic();			// DocShell-Basic in DrawPages setzen
	}
	else
	{
		//	always use global color table instead of local copy
        PutItem( SvxColorTableItem( XColorTable::GetStdColorTable(), SID_COLOR_TABLE ) );
	}

	if ( !aDocument.GetForbiddenCharacters().isValid() ||
			!aDocument.IsValidAsianCompression() || !aDocument.IsValidAsianKerning() )
	{
		//	get settings from SvxAsianConfig
		SvxAsianConfig aAsian( sal_False );

		if ( !aDocument.GetForbiddenCharacters().isValid() )
		{
			// set forbidden characters if necessary
			uno::Sequence<lang::Locale> aLocales = aAsian.GetStartEndCharLocales();
			if (aLocales.getLength())
			{
				vos::ORef<SvxForbiddenCharactersTable> xForbiddenTable =
						new SvxForbiddenCharactersTable( aDocument.GetServiceManager() );

				const lang::Locale* pLocales = aLocales.getConstArray();
				for (sal_Int32 i = 0; i < aLocales.getLength(); i++)
				{
					i18n::ForbiddenCharacters aForbidden;
					aAsian.GetStartEndChars( pLocales[i], aForbidden.beginLine, aForbidden.endLine );
					LanguageType eLang = SvxLocaleToLanguage(pLocales[i]);
					//pDoc->SetForbiddenCharacters( eLang, aForbidden );

					xForbiddenTable->SetForbiddenCharacters( eLang, aForbidden );
				}

				aDocument.SetForbiddenCharacters( xForbiddenTable );
			}
		}

		if ( !aDocument.IsValidAsianCompression() )
		{
			// set compression mode from configuration if not already set (e.g. XML import)
            aDocument.SetAsianCompression( sal::static_int_cast<BYTE>( aAsian.GetCharDistanceCompression() ) );
		}

		if ( !aDocument.IsValidAsianKerning() )
		{
			// set asian punctuation kerning from configuration if not already set (e.g. XML import)
			aDocument.SetAsianKerning( !aAsian.IsKerningWesternTextOnly() );	// reversed
		}
	}
}

//------------------------------------------------------------------

void ScDocShell::ResetDrawObjectShell()
{
	ScDrawLayer* pDrawLayer = aDocument.GetDrawLayer();
	if (pDrawLayer)
		pDrawLayer->SetObjectShell( NULL );
}

//------------------------------------------------------------------

void __EXPORT ScDocShell::Activate()
{
}


void __EXPORT ScDocShell::Deactivate()
{
}

//------------------------------------------------------------------


ScDrawLayer* ScDocShell::MakeDrawLayer()
{
	ScDrawLayer* pDrawLayer = aDocument.GetDrawLayer();
	if (!pDrawLayer)
	{
		RTL_LOGFILE_CONTEXT_AUTHOR ( aLog, "sc", "nn93723", "ScDocShell::MakeDrawLayer" );

		aDocument.InitDrawLayer(this);
		pDrawLayer = aDocument.GetDrawLayer();
		InitItems();											// incl. Undo und Basic
		Broadcast( SfxSimpleHint( SC_HINT_DRWLAYER_NEW ) );
		if (nDocumentLock)
			pDrawLayer->setLock(TRUE);
	}
	return pDrawLayer;
}
