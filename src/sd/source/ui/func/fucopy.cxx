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


#include "fucopy.hxx"
#include <sfx2/progress.hxx>
#include <svx/svxids.hrc>

#include "sdresid.hxx"
#include "sdattr.hxx"
#include "strings.hrc"
#include "ViewShell.hxx"
#include "View.hxx"
#include "drawdoc.hxx"
#include "DrawDocShell.hxx"
#include <vcl/wrkwin.hxx>
#include <svx/svdobj.hxx>
#include <vcl/msgbox.hxx>
#include <sfx2/app.hxx>
#include <svx/xcolit.hxx>
#include <svx/xflclit.hxx>
#include <svx/xdef.hxx>
#include <svx/xfillit0.hxx>
#include <sfx2/request.hxx>
#include "sdabstdlg.hxx"
#include "copydlg.hrc"
namespace sd {

TYPEINIT1( FuCopy, FuPoor );

/*************************************************************************
|*
|* Konstruktor
|*
\************************************************************************/

FuCopy::FuCopy (
    ViewShell* pViewSh, 
    ::sd::Window* pWin, 
    ::sd::View* pView,
    SdDrawDocument* pDoc, 
    SfxRequest& rReq)
	: FuPoor(pViewSh, pWin, pView, pDoc, rReq)
{
}

FunctionReference FuCopy::Create( ViewShell* pViewSh, ::sd::Window* pWin, ::sd::View* pView, SdDrawDocument* pDoc, SfxRequest& rReq )
{
	FunctionReference xFunc( new FuCopy( pViewSh, pWin, pView, pDoc, rReq ) );
	xFunc->DoExecute(rReq);
	return xFunc;
}

void FuCopy::DoExecute( SfxRequest& rReq )
{
	if( mpView->AreObjectsMarked() )
	{
		// Undo
		String aString( mpView->GetDescriptionOfMarkedObjects() );
		aString.Append( sal_Unicode(' ') );
		aString.Append( String( SdResId( STR_UNDO_COPYOBJECTS ) ) );
		mpView->BegUndo( aString );

		const SfxItemSet* pArgs = rReq.GetArgs();

		if( !pArgs )
		{
			SfxItemSet aSet( mpViewShell->GetPool(),
								ATTR_COPY_START, ATTR_COPY_END, 0 );

			// Farb-Attribut angeben
			SfxItemSet aAttr( mpDoc->GetPool() );
			mpView->GetAttributes( aAttr );
			const SfxPoolItem*	pPoolItem = NULL;

			if( SFX_ITEM_SET == aAttr.GetItemState( XATTR_FILLSTYLE, TRUE, &pPoolItem ) )
			{
				XFillStyle eStyle = ( ( const XFillStyleItem* ) pPoolItem )->GetValue();

				if( eStyle == XFILL_SOLID &&
					SFX_ITEM_SET == aAttr.GetItemState( XATTR_FILLCOLOR, TRUE, &pPoolItem ) )
				{
					const XFillColorItem* pItem = ( const XFillColorItem* ) pPoolItem;
					XColorItem aXColorItem( ATTR_COPY_START_COLOR, pItem->GetName(),
														pItem->GetColorValue() );
					aSet.Put( aXColorItem );

				}
			}

			SdAbstractDialogFactory* pFact = SdAbstractDialogFactory::Create();
			if( pFact )
			{
				AbstractCopyDlg* pDlg = pFact->CreateCopyDlg(NULL, aSet, mpDoc->GetColorTable(), mpView );
				if( pDlg )
				{
					USHORT nResult = pDlg->Execute();

					switch( nResult )
					{
						case RET_OK:
							pDlg->GetAttr( aSet );
							rReq.Done( aSet );
							pArgs = rReq.GetArgs();
						break;

						default:
						{
							delete pDlg;
							mpView->EndUndo();
						}
						return; // Abbruch
					}
					delete( pDlg );
				}
			}
		}

		Rectangle	        aRect;
		INT32		        lWidth = 0, lHeight = 0, lSizeX = 0L, lSizeY = 0L, lAngle = 0L;
		UINT16		        nNumber = 0;
		Color		        aStartColor, aEndColor;
		BOOL		        bColor = FALSE;
		const SfxPoolItem*  pPoolItem = NULL;

		// Anzahl
		if( SFX_ITEM_SET == pArgs->GetItemState( ATTR_COPY_NUMBER, TRUE, &pPoolItem ) )
			nNumber = ( ( const SfxUInt16Item* ) pPoolItem )->GetValue();

		// Verschiebung
		if( SFX_ITEM_SET == pArgs->GetItemState( ATTR_COPY_MOVE_X, TRUE, &pPoolItem ) )
			lSizeX = ( ( const SfxInt32Item* ) pPoolItem )->GetValue();
		if( SFX_ITEM_SET == pArgs->GetItemState( ATTR_COPY_MOVE_Y, TRUE, &pPoolItem ) )
			lSizeY = ( ( const SfxInt32Item* ) pPoolItem )->GetValue();
		if( SFX_ITEM_SET == pArgs->GetItemState( ATTR_COPY_ANGLE, TRUE, &pPoolItem ) )
			lAngle = ( ( const SfxInt32Item* )pPoolItem )->GetValue();

		// Verrgroesserung / Verkleinerung
		if( SFX_ITEM_SET == pArgs->GetItemState( ATTR_COPY_WIDTH, TRUE, &pPoolItem ) )
			lWidth = ( ( const SfxInt32Item* ) pPoolItem )->GetValue();
		if( SFX_ITEM_SET == pArgs->GetItemState( ATTR_COPY_HEIGHT, TRUE, &pPoolItem ) )
			lHeight = ( ( const SfxInt32Item* ) pPoolItem )->GetValue();

		// Startfarbe / Endfarbe
		if( SFX_ITEM_SET == pArgs->GetItemState( ATTR_COPY_START_COLOR, TRUE, &pPoolItem ) )
		{
			aStartColor = ( ( const XColorItem* ) pPoolItem )->GetColorValue();
			bColor = TRUE;
		}
		if( SFX_ITEM_SET == pArgs->GetItemState( ATTR_COPY_END_COLOR, TRUE, &pPoolItem ) )
		{
			aEndColor = ( ( const XColorItem* ) pPoolItem )->GetColorValue();
			if( aStartColor == aEndColor )
				bColor = FALSE;
		}
		else
			bColor = FALSE;

		// Handles wegnehmen
		//HMHmpView->HideMarkHdl();

		SfxProgress*    pProgress = NULL;
		BOOL            bWaiting = FALSE;

		if( nNumber > 1 )
		{
			String aStr( SdResId( STR_OBJECTS ) );
			aStr.Append( sal_Unicode(' ') );
			aStr.Append( String( SdResId( STR_UNDO_COPYOBJECTS ) ) );

			pProgress = new SfxProgress( mpDocSh, aStr, nNumber );
			mpDocSh->SetWaitCursor( TRUE );
			bWaiting = TRUE;
		}

		const SdrMarkList 	aMarkList( mpView->GetMarkedObjectList() );
		const ULONG			nMarkCount = aMarkList.GetMarkCount();
		SdrObject*		    pObj = NULL;

		// Anzahl moeglicher Kopien berechnen
		aRect = mpView->GetAllMarkedRect();
		
		if( lWidth < 0L )
		{
			long nTmp = ( aRect.Right() - aRect.Left() ) / -lWidth;
			nNumber = (UINT16) Min( nTmp, (long)nNumber );
		}
		
		if( lHeight < 0L )
		{
			long nTmp = ( aRect.Bottom() - aRect.Top() ) / -lHeight;
			nNumber = (UINT16) Min( nTmp, (long)nNumber );
		}

		for( USHORT i = 1; i <= nNumber; i++ )
		{
			if( pProgress )
				pProgress->SetState( i );

			aRect = mpView->GetAllMarkedRect();

			if( ( 1 == i ) && bColor )
			{
				SfxItemSet aNewSet( mpViewShell->GetPool(), XATTR_FILLSTYLE, XATTR_FILLCOLOR, 0L );
				aNewSet.Put( XFillStyleItem( XFILL_SOLID ) );
				aNewSet.Put( XFillColorItem( String(), aStartColor ) );
				mpView->SetAttributes( aNewSet );
			}

			// make a copy of selected objects
			mpView->CopyMarked();
			
    		// get newly selected objects 
    		SdrMarkList aCopyMarkList( mpView->GetMarkedObjectList() );
	    	ULONG		j, nCopyMarkCount = aMarkList.GetMarkCount();
				
			// set protection flags at marked copies to null
			for( j = 0; j < nCopyMarkCount; j++ )
			{
				pObj = aCopyMarkList.GetMark( j )->GetMarkedSdrObj();
				
				if( pObj )
				{
					pObj->SetMoveProtect( FALSE );
					pObj->SetResizeProtect( FALSE );
				}
			}

			Fraction aWidth( aRect.Right() - aRect.Left() + lWidth, aRect.Right() - aRect.Left() );
			Fraction aHeight( aRect.Bottom() - aRect.Top() + lHeight, aRect.Bottom() - aRect.Top() );

			if( mpView->IsResizeAllowed() )
				mpView->ResizeAllMarked( aRect.TopLeft(), aWidth, aHeight );

			if( mpView->IsRotateAllowed() )
				mpView->RotateAllMarked( aRect.Center(), lAngle * 100 );

			if( mpView->IsMoveAllowed() )
				mpView->MoveAllMarked( Size( lSizeX, lSizeY ) );
			
			// set protection flags at marked copies to original values
			if( nMarkCount == nCopyMarkCount )
			{
			    for( j = 0; j < nMarkCount; j++ )
			    {
			        SdrObject* pSrcObj = aMarkList.GetMark( j )->GetMarkedSdrObj();
			        SdrObject* pDstObj = aCopyMarkList.GetMark( j )->GetMarkedSdrObj();
			    
				    if( pSrcObj && pDstObj && 
				        ( pSrcObj->GetObjInventor() == pDstObj->GetObjInventor() ) &&
				        ( pSrcObj->GetObjIdentifier() == pDstObj->GetObjIdentifier() ) )
				    {
					    pDstObj->SetMoveProtect( pSrcObj->IsMoveProtect() );
					    pDstObj->SetResizeProtect( pSrcObj->IsResizeProtect() );
				    }
			    }
			}

			if( bColor )
			{
				// Koennte man sicher noch optimieren, wuerde aber u.U.
				// zu Rundungsfehlern fuehren
				BYTE nRed = aStartColor.GetRed() + (BYTE) ( ( (long) aEndColor.GetRed() - (long) aStartColor.GetRed() ) * (long) i / (long) nNumber  );
				BYTE nGreen = aStartColor.GetGreen() + (BYTE) ( ( (long) aEndColor.GetGreen() - (long) aStartColor.GetGreen() ) *  (long) i / (long) nNumber );
				BYTE nBlue = aStartColor.GetBlue() + (BYTE) ( ( (long) aEndColor.GetBlue() - (long) aStartColor.GetBlue() ) * (long) i / (long) nNumber );
				Color aNewColor( nRed, nGreen, nBlue );
				SfxItemSet aNewSet( mpViewShell->GetPool(), XATTR_FILLSTYLE, XATTR_FILLCOLOR, 0L );
				aNewSet.Put( XFillStyleItem( XFILL_SOLID ) );
				aNewSet.Put( XFillColorItem( String(), aNewColor ) );
				mpView->SetAttributes( aNewSet );
			}
		}

		if ( pProgress )
			delete pProgress;

		if ( bWaiting )
			mpDocSh->SetWaitCursor( FALSE );

		// Handles zeigen
		mpView->AdjustMarkHdl(); //HMH TRUE );
		//HMHpView->ShowMarkHdl();

		mpView->EndUndo();
	}
}

} // end of namespace 
