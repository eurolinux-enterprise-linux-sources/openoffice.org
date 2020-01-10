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
#include "precompiled_sd.hxx"

#include "fuprobjs.hxx"

#ifndef _MSGBOX_HXX //autogen
#include <vcl/msgbox.hxx>
#endif
#include <svtools/style.hxx>
#include <svx/outliner.hxx>
#include <svtools/smplhint.hxx>


#include "app.hrc"
#include "res_bmp.hrc"
#include "strings.hrc"
#include "glob.hrc"
#include "prltempl.hrc"

#include "sdresid.hxx"
#include "drawdoc.hxx"
#ifndef SD_OUTLINE_VIEW_SHELL_HX
#include "OutlineViewShell.hxx"
#endif
#include "ViewShell.hxx"
#include "Window.hxx"
#include "glob.hxx"
#include "prlayout.hxx"
#include "unchss.hxx"
#include "sdabstdlg.hxx"
namespace sd {

TYPEINIT1( FuPresentationObjects, FuPoor );


/*************************************************************************
|*
|* Konstruktor
|*
\************************************************************************/

FuPresentationObjects::FuPresentationObjects (
    ViewShell* pViewSh,
    ::sd::Window* pWin, 
    ::sd::View* pView,
    SdDrawDocument* pDoc,
    SfxRequest& rReq)
	 : FuPoor(pViewSh, pWin, pView, pDoc, rReq)
{
}

FunctionReference FuPresentationObjects::Create( ViewShell* pViewSh, ::sd::Window* pWin, ::sd::View* pView, SdDrawDocument* pDoc, SfxRequest& rReq )
{
	FunctionReference xFunc( new FuPresentationObjects( pViewSh, pWin, pView, pDoc, rReq ) );
	xFunc->DoExecute(rReq);
	return xFunc;
}

void FuPresentationObjects::DoExecute( SfxRequest& )
{
	OutlineViewShell* pOutlineViewShell = dynamic_cast< OutlineViewShell* >( mpViewShell );
	DBG_ASSERT( pOutlineViewShell, "sd::FuPresentationObjects::DoExecute(), does not work without an OutlineViewShell!");
	if( !pOutlineViewShell )
		return;

	// ergibt die Selektion ein eindeutiges Praesentationslayout?
	// wenn nicht, duerfen die Vorlagen nicht bearbeitet werden
	SfxItemSet aSet(mpDoc->GetItemPool(), SID_STATUS_LAYOUT, SID_STATUS_LAYOUT);
	pOutlineViewShell->GetStatusBarState( aSet );
	String aLayoutName = (((SfxStringItem&)aSet.Get(SID_STATUS_LAYOUT)).GetValue());
	DBG_ASSERT(aLayoutName.Len(), "Layout unbestimmt");

	BOOL	bUnique = FALSE;
	sal_Int16	nDepth, nTmp;
	OutlineView* pOlView = static_cast<OutlineView*>(pOutlineViewShell->GetView());
	OutlinerView* pOutlinerView = pOlView->GetViewByWindow( (Window*) mpWindow );
	::Outliner* pOutl = pOutlinerView->GetOutliner();
	List* pList = pOutlinerView->CreateSelectionList();
	Paragraph* pPara = (Paragraph*)pList->First();
	nDepth = pOutl->GetDepth((USHORT)pOutl->GetAbsPos( pPara ) );
	bool bPage = pOutl->HasParaFlag( pPara, PARAFLAG_ISPAGE );

	while( pPara )
	{
		nTmp = pOutl->GetDepth((USHORT) pOutl->GetAbsPos( pPara ) );

		if( nDepth != nTmp )
		{
			bUnique = FALSE;
			break;
		}

		if( pOutl->HasParaFlag( pPara, PARAFLAG_ISPAGE ) != bPage )
		{
			bUnique = FALSE;
			break;
		}
		bUnique = TRUE;

		pPara = (Paragraph*) pList->Next();
	}

	if( bUnique )
	{
		String aStyleName = aLayoutName;
		aStyleName.AppendAscii( RTL_CONSTASCII_STRINGPARAM( SD_LT_SEPARATOR ) );
		USHORT nDlgId = TAB_PRES_LAYOUT_TEMPLATE;
		PresentationObjects	ePO;

		if( bPage )
		{
			ePO = PO_TITLE;
			String aStr(SdResId( STR_LAYOUT_TITLE ));
			aStyleName.Append( aStr );
		}
		else
		{
			ePO = (PresentationObjects) ( PO_OUTLINE_1 + nDepth - 1 );
			String aStr(SdResId( STR_LAYOUT_OUTLINE ));
			aStyleName.Append( aStr );
			aStyleName.Append( sal_Unicode(' ') );
			aStyleName.Append( UniString::CreateFromInt32( nDepth ) );
		}

		SfxStyleSheetBasePool* pStyleSheetPool = mpDocSh->GetStyleSheetPool();
		SfxStyleSheetBase* pStyleSheet = pStyleSheetPool->Find( aStyleName, SD_STYLE_FAMILY_MASTERPAGE );
		DBG_ASSERT(pStyleSheet, "StyleSheet nicht gefunden");

		if( pStyleSheet )
		{
			SfxStyleSheetBase& rStyleSheet = *pStyleSheet;

			SdAbstractDialogFactory* pFact = SdAbstractDialogFactory::Create();
			SfxAbstractTabDialog* pDlg = pFact ? pFact->CreateSdPresLayoutTemplateDlg( mpDocSh, NULL, SdResId( nDlgId ), rStyleSheet, ePO, pStyleSheetPool ) : 0;
			if( pDlg && (pDlg->Execute() == RET_OK) )
			{
				const SfxItemSet* pOutSet = pDlg->GetOutputItemSet();
				// Undo-Action
				StyleSheetUndoAction* pAction = new StyleSheetUndoAction
												(mpDoc, (SfxStyleSheet*)pStyleSheet,
													pOutSet);
				mpDocSh->GetUndoManager()->AddUndoAction(pAction);

				pStyleSheet->GetItemSet().Put( *pOutSet );
				( (SfxStyleSheet*) pStyleSheet )->Broadcast( SfxSimpleHint( SFX_HINT_DATACHANGED ) );
			}
			delete( pDlg );
		}
	}
}

} // end of namespace sd
