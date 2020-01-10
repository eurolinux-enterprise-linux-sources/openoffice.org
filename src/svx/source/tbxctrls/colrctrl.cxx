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

#include <svx/dialogs.hrc>


#include <tools/list.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/dispatch.hxx>
#include <vcl/image.hxx>

#include <svx/colrctrl.hxx>

#include <svx/svdview.hxx>
#include "drawitem.hxx"
#include <svx/colritem.hxx>
#include "xattr.hxx"
#include <svx/xtable.hxx>
#include <svx/dialmgr.hxx>
#include "xexch.hxx"
#include <vcl/svapp.hxx>

SFX_IMPL_DOCKINGWINDOW( SvxColorChildWindow, SID_COLOR_CONTROL )

// ------------------------
// - SvxColorValueSetData -
// ------------------------

class SvxColorValueSetData : public TransferableHelper
{
private:

	XFillExchangeData		maData;

protected:

	virtual void			AddSupportedFormats();
	virtual sal_Bool		GetData( const ::com::sun::star::datatransfer::DataFlavor& rFlavor );
	virtual sal_Bool		WriteObject( SotStorageStreamRef& rxOStm, void* pUserObject, sal_uInt32 nUserObjectId, const ::com::sun::star::datatransfer::DataFlavor& rFlavor );

public:

							SvxColorValueSetData( const XFillAttrSetItem& rSetItem ) :
								maData( rSetItem ) {}
};

// -----------------------------------------------------------------------------

void SvxColorValueSetData::AddSupportedFormats()
{
	AddFormat( SOT_FORMATSTR_ID_XFA );
}

// -----------------------------------------------------------------------------

sal_Bool SvxColorValueSetData::GetData( const ::com::sun::star::datatransfer::DataFlavor& rFlavor )
{
	sal_Bool bRet = sal_False;

	if( SotExchange::GetFormat( rFlavor ) == SOT_FORMATSTR_ID_XFA )
	{
		SetObject( &maData, 0, rFlavor );
		bRet = sal_True;
	}

	return bRet;
}

// -----------------------------------------------------------------------------

sal_Bool SvxColorValueSetData::WriteObject( SotStorageStreamRef& rxOStm, void*, sal_uInt32 , const ::com::sun::star::datatransfer::DataFlavor&  )
{
	*rxOStm << maData;
	return( rxOStm->GetError() == ERRCODE_NONE );
}

/*************************************************************************
|*
|* SvxColorValueSet: Ctor
|*
\************************************************************************/

SvxColorValueSet::SvxColorValueSet( Window* _pParent, WinBits nWinStyle ) :
	ValueSet( _pParent, nWinStyle ),
	DragSourceHelper( this ),
    bLeft (TRUE)
{
}

/*************************************************************************
|*
|* SvxColorValueSet: Ctor
|*
\************************************************************************/

SvxColorValueSet::SvxColorValueSet( Window* _pParent, const ResId& rResId ) :
	ValueSet( _pParent, rResId ),
	DragSourceHelper( this ),
    bLeft (TRUE)
{
}

/*************************************************************************
|*
|* SvxColorValueSet: MouseButtonDown
|*
\************************************************************************/

void SvxColorValueSet::MouseButtonDown( const MouseEvent& rMEvt )
{
	// Fuer Mac noch anders handlen !
	if( rMEvt.IsLeft() )
	{
		bLeft = TRUE;
		ValueSet::MouseButtonDown( rMEvt );
	}
	else
	{
		bLeft = FALSE;
		MouseEvent aMEvt( rMEvt.GetPosPixel(),
						  rMEvt.GetClicks(),
						  rMEvt.GetMode(),
						  MOUSE_LEFT,
						  rMEvt.GetModifier() );
		ValueSet::MouseButtonDown( aMEvt );
	}

	aDragPosPixel = GetPointerPosPixel();
}

/*************************************************************************
|*
|* SvxColorValueSet: MouseButtonUp
|*
\************************************************************************/

void SvxColorValueSet::MouseButtonUp( const MouseEvent& rMEvt )
{
	// Fuer Mac noch anders handlen !
	if( rMEvt.IsLeft() )
	{
		bLeft = TRUE;
		ValueSet::MouseButtonUp( rMEvt );
	}
	else
	{
		bLeft = FALSE;
		MouseEvent aMEvt( rMEvt.GetPosPixel(),
						  rMEvt.GetClicks(),
						  rMEvt.GetMode(),
						  MOUSE_LEFT,
						  rMEvt.GetModifier() );
		ValueSet::MouseButtonUp( aMEvt );
	}
	SetNoSelection();
}

/*************************************************************************
|*
|* Command-Event
|*
\************************************************************************/

void SvxColorValueSet::Command(const CommandEvent& rCEvt)
{
	// Basisklasse
	ValueSet::Command(rCEvt);
}

/*************************************************************************
|*
|* StartDrag
|*
\************************************************************************/

void SvxColorValueSet::StartDrag( sal_Int8 , const Point&  )
{
	Application::PostUserEvent(STATIC_LINK(this, SvxColorValueSet, ExecDragHdl));
}

/*************************************************************************
|*
|* Drag&Drop asynchron ausfuehren
|*
\************************************************************************/

void SvxColorValueSet::DoDrag()
{
	SfxObjectShell* pDocSh = SfxObjectShell::Current();
	USHORT			nItemId = GetItemId( aDragPosPixel );

	if( pDocSh && nItemId )
	{
		XFillAttrSetItem	aXFillSetItem( &pDocSh->GetPool() );
		SfxItemSet&			rSet = aXFillSetItem.GetItemSet();

		rSet.Put( XFillColorItem( GetItemText( nItemId ), GetItemColor( nItemId ) ) );
		rSet.Put(XFillStyleItem( ( 1 == nItemId ) ? XFILL_NONE : XFILL_SOLID ) );

		EndSelection();
		( new SvxColorValueSetData( aXFillSetItem ) )->StartDrag( this, DND_ACTION_COPY );
		ReleaseMouse();
	}
}

/*************************************************************************
|*
|*
|*
\************************************************************************/

IMPL_STATIC_LINK(SvxColorValueSet, ExecDragHdl, void*, EMPTYARG)
{
	// Als Link, damit asynchron ohne ImpMouseMoveMsg auf dem Stack auch die
	// Farbleiste geloescht werden darf
	pThis->DoDrag();
	return(0);
}

/*************************************************************************
|*
|* Ableitung vom SfxChildWindow als "Behaelter" fuer Animator
|*
\************************************************************************/

SvxColorChildWindow::SvxColorChildWindow( Window* _pParent,
    									  USHORT nId,
										  SfxBindings* pBindings,
										  SfxChildWinInfo* pInfo ) :
	SfxChildWindow( _pParent, nId )
{
	SvxColorDockingWindow* pWin = new SvxColorDockingWindow( pBindings, this,
										_pParent, SVX_RES( RID_SVXCTRL_COLOR ) );
	pWindow = pWin;

	eChildAlignment = SFX_ALIGN_BOTTOM;

	pWin->Initialize( pInfo );
}



/*************************************************************************
|*
|* Ctor: SvxColorDockingWindow
|*
\************************************************************************/

SvxColorDockingWindow::SvxColorDockingWindow
(
	SfxBindings* _pBindings,
	SfxChildWindow* pCW,
	Window* _pParent,
	const ResId& rResId
) :

	SfxDockingWindow( _pBindings, pCW, _pParent, rResId ),

	pColorTable 	( NULL ),
	aColorSet		( this, ResId( 1, *rResId.GetResMgr() ) ),
	nLeftSlot		( SID_ATTR_FILL_COLOR ),
	nRightSlot		( SID_ATTR_LINE_COLOR ),
	nCols			( 20 ),
	nLines			( 1 ),
	aColorSize		( 14, 14 )

{
	FreeResource();

	aColorSet.SetStyle( aColorSet.GetStyle() | WB_ITEMBORDER );
	aColorSet.SetSelectHdl( LINK( this, SvxColorDockingWindow, SelectHdl ) );

    // Get the model from the view shell.  Using SfxObjectShell::Current()
    // is unreliable when called at the wrong times.
    SfxObjectShell*	pDocSh = NULL;
    if (_pBindings != NULL)
    {
        SfxDispatcher* pDispatcher = _pBindings->GetDispatcher();
        if (pDispatcher != NULL)
        {
            SfxViewFrame* pFrame = pDispatcher->GetFrame();
            if (pFrame != NULL)
            {
                SfxViewShell* pViewShell = pFrame->GetViewShell();
                if (pViewShell != NULL)
                    pDocSh = pViewShell->GetObjectShell();
            }
        }
    }

	if ( pDocSh )
	{
		const SfxPoolItem*	pItem = pDocSh->GetItem( SID_COLOR_TABLE );
		if( pItem )
		{
			pColorTable = ( (SvxColorTableItem*) pItem )->GetColorTable();
			FillValueSet();
		}
	}
	aItemSize = aColorSet.CalcItemSizePixel( aColorSize );
	aItemSize.Width() = aItemSize.Width() + aColorSize.Width();
	aItemSize.Width() /= 2;
	aItemSize.Height() = aItemSize.Height() + aColorSize.Height();
	aItemSize.Height() /= 2;

	SetSize();
	aColorSet.Show();
	StartListening( *_pBindings, TRUE );
}


/*************************************************************************
|*
|* Dtor: SvxColorDockingWindow
|*
\************************************************************************/

SvxColorDockingWindow::~SvxColorDockingWindow()
{
	EndListening( GetBindings() );
}

/*************************************************************************
|*
|* Notify
|*
\************************************************************************/

void SvxColorDockingWindow::Notify( SfxBroadcaster& , const SfxHint& rHint )
{
	const SfxPoolItemHint *pPoolItemHint = PTR_CAST(SfxPoolItemHint, &rHint);
	if ( pPoolItemHint
		 && ( pPoolItemHint->GetObject()->ISA( SvxColorTableItem ) ) )
	{
		// Die Liste der Farben hat sich geaendert
		pColorTable = ( (SvxColorTableItem*) pPoolItemHint->GetObject() )->GetColorTable();
		FillValueSet();
	}
}

/*************************************************************************
|*
|* FillValueSet
|*
\************************************************************************/

void SvxColorDockingWindow::FillValueSet()
{
	if( pColorTable )
	{
		aColorSet.Clear();

		// Erster Eintrag: unsichtbar
		long nPtX = aColorSize.Width() - 1;
		long nPtY = aColorSize.Height() - 1;
		VirtualDevice aVD;
		aVD.SetOutputSizePixel( aColorSize );
		aVD.SetLineColor( Color( COL_BLACK ) );
		aVD.SetBackground( Wallpaper( Color( COL_WHITE ) ) );
		aVD.DrawLine( Point(), Point( nPtX, nPtY ) );
		aVD.DrawLine( Point( 0, nPtY ), Point( nPtX, 0 ) );

		Bitmap aBmp( aVD.GetBitmap( Point(), aColorSize ) );

		aColorSet.InsertItem( (USHORT)1, Image(aBmp), SVX_RESSTR( RID_SVXSTR_INVISIBLE ) );

		XColorEntry* pEntry;
		nCount = pColorTable->Count();

		for( long i = 0; i < nCount; i++ )
		{
			pEntry = pColorTable->GetColor( i );
			aColorSet.InsertItem( (USHORT)i+2,
							pEntry->GetColor(), pEntry->GetName() );
		}
	}
}

/*************************************************************************
|*
|* SetSize
|*
\************************************************************************/

void SvxColorDockingWindow::SetSize()
{
	// Groesse fuer ValueSet berechnen
	Size aSize = GetOutputSizePixel();
	aSize.Width()  -= 4;
	aSize.Height() -= 4;

	// Zeilen und Spalten berechnen
	nCols = (USHORT) ( aSize.Width() / aItemSize.Width() );
	nLines = (USHORT) ( (float) aSize.Height() / (float) aItemSize.Height() /*+ 0.35*/ );
	if( nLines == 0 )
		nLines++;

	// Scrollbar setzen/entfernen
	WinBits nBits = aColorSet.GetStyle();
	if ( nLines * nCols >= nCount )
		nBits &= ~WB_VSCROLL;
	else
		nBits |= WB_VSCROLL;
	aColorSet.SetStyle( nBits );

	// ScrollBar ?
	long nScrollWidth = aColorSet.GetScrollWidth();
	if( nScrollWidth > 0 )
	{
		// Spalten mit ScrollBar berechnen
		nCols = (USHORT) ( ( aSize.Width() - nScrollWidth ) / aItemSize.Width() );
	}
	aColorSet.SetColCount( nCols );

	if( IsFloatingMode() )
		aColorSet.SetLineCount( nLines );
	else
	{
		aColorSet.SetLineCount( 0 ); // sonst wird LineHeight ignoriert
		aColorSet.SetItemHeight( aItemSize.Height() );
	}

	aColorSet.SetPosSizePixel( Point( 2, 2 ), aSize );
}

/*************************************************************************
|*
|* SvxColorDockingWindow: Close
|*
\************************************************************************/

BOOL SvxColorDockingWindow::Close()
{
	SfxBoolItem aItem( SID_COLOR_CONTROL, FALSE );
	GetBindings().GetDispatcher()->Execute(
		SID_COLOR_CONTROL, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD, &aItem, 0L );
	SfxDockingWindow::Close();
	return( TRUE );
}

/*************************************************************************
|*
|* SelectHdl
|*
\************************************************************************/

IMPL_LINK( SvxColorDockingWindow, SelectHdl, void *, EMPTYARG )
{
	SfxDispatcher* pDispatcher = GetBindings().GetDispatcher();
	USHORT nPos = aColorSet.GetSelectItemId();
	Color  aColor( aColorSet.GetItemColor( nPos ) );
	String aStr( aColorSet.GetItemText( nPos ) );

    if (aColorSet.IsLeftButton())
    {
		if ( nLeftSlot == SID_ATTR_FILL_COLOR )
		{
			if ( nPos == 1 )		// unsichtbar
			{
				XFillStyleItem aXFillStyleItem( XFILL_NONE );
				pDispatcher->Execute( nLeftSlot, SFX_CALLMODE_RECORD, &aXFillStyleItem, 0L );
			}
			else
			{
				BOOL bDone = FALSE;

				// Wenn wir eine DrawView haben und uns im TextEdit-Modus befinden,
				// wird nicht die Flaechen-, sondern die Textfarbe zugewiesen
				SfxViewShell* pViewSh = SfxViewShell::Current();
				if ( pViewSh )
				{
					SdrView* pView = pViewSh->GetDrawView();
					if ( pView && pView->IsTextEdit() )
					{
						SvxColorItem aTextColorItem( aColor, SID_ATTR_CHAR_COLOR );
						pDispatcher->Execute(
							SID_ATTR_CHAR_COLOR, SFX_CALLMODE_RECORD, &aTextColorItem, 0L );
						bDone = TRUE;
					}
				}
				if ( !bDone )
				{
					XFillStyleItem aXFillStyleItem( XFILL_SOLID );
					XFillColorItem aXFillColorItem( aStr, aColor );
					pDispatcher->Execute(
						nLeftSlot, SFX_CALLMODE_RECORD, &aXFillColorItem, &aXFillStyleItem, 0L );
				}
			}
		}
		else if ( nPos != 1 )		// unsichtbar
		{
			SvxColorItem aLeftColorItem( aColor, nLeftSlot );
			pDispatcher->Execute( nLeftSlot, SFX_CALLMODE_RECORD, &aLeftColorItem, 0L );
		}
    }
    else
	{
		if ( nRightSlot == SID_ATTR_LINE_COLOR )
		{
			if( nPos == 1 )		// unsichtbar
			{
				XLineStyleItem aXLineStyleItem( XLINE_NONE );
				pDispatcher->Execute( nRightSlot, SFX_CALLMODE_RECORD, &aXLineStyleItem, 0L );
			}
			else
			{
				// Sollte der LineStyle unsichtbar sein, so wird er auf SOLID gesetzt
				SfxViewShell* pViewSh = SfxViewShell::Current();
				if ( pViewSh )
				{
					SdrView* pView = pViewSh->GetDrawView();
					if ( pView )
					{
						SfxItemSet aAttrSet( pView->GetModel()->GetItemPool() );
						pView->GetAttributes( aAttrSet );
						if ( aAttrSet.GetItemState( XATTR_LINESTYLE ) != SFX_ITEM_DONTCARE )
						{
							XLineStyle eXLS = (XLineStyle)
								( (const XLineStyleItem&)aAttrSet.Get( XATTR_LINESTYLE ) ).GetValue();
							if ( eXLS == XLINE_NONE )
							{
								XLineStyleItem aXLineStyleItem( XLINE_SOLID );
								pDispatcher->Execute( nRightSlot, SFX_CALLMODE_RECORD, &aXLineStyleItem, 0L );
							}
						}
					}
				}

				XLineColorItem aXLineColorItem( aStr, aColor );
				pDispatcher->Execute( nRightSlot, SFX_CALLMODE_RECORD, &aXLineColorItem, 0L );
			}
		}
		else if ( nPos != 1 )		// unsichtbar
		{
			SvxColorItem aRightColorItem( aColor, nRightSlot );
			pDispatcher->Execute( nRightSlot, SFX_CALLMODE_RECORD, &aRightColorItem, 0L );
		}
	}

	return 0;
}

/*************************************************************************
|*
|* Resizing
|*
\************************************************************************/


void SvxColorDockingWindow::Resizing( Size& rNewSize )
{
	rNewSize.Width()  -= 4;
	rNewSize.Height() -= 4;

	// Spalten und Reihen ermitteln
	nCols = (USHORT) ( (float) rNewSize.Width() / (float) aItemSize.Width() + 0.5 );
	nLines = (USHORT) ( (float) rNewSize.Height() / (float) aItemSize.Height() + 0.5 );
	if( nLines == 0 )
		nLines = 1;

	// Scrollbar setzen/entfernen
	WinBits nBits = aColorSet.GetStyle();
	if ( nLines * nCols >= nCount )
		nBits &= ~WB_VSCROLL;
	else
		nBits |= WB_VSCROLL;
	aColorSet.SetStyle( nBits );

	// ScrollBar ?
	long nScrollWidth = aColorSet.GetScrollWidth();
	if( nScrollWidth > 0 )
	{
		// Spalten mit ScrollBar berechnen
		nCols = (USHORT) ( ( ( (float) rNewSize.Width() - (float) nScrollWidth ) )
							/ (float) aItemSize.Width() + 0.5 );
	}
	if( nCols <= 1 )
		nCols = 2;

	// Max. Reihen anhand der gegebenen Spalten berechnen
	long nMaxLines = nCount / nCols;
	if( nCount %  nCols )
		nMaxLines++;

	nLines = sal::static_int_cast< USHORT >(
        std::min< long >( nLines, nMaxLines ) );

	// Groesse des Windows setzen
	rNewSize.Width()  = nCols * aItemSize.Width() + nScrollWidth + 4;
	rNewSize.Height() = nLines * aItemSize.Height() + 4;
}

/*************************************************************************
|*
|* Resize
|*
\************************************************************************/

void SvxColorDockingWindow::Resize()
{
	if ( !IsFloatingMode() || !GetFloatingWindow()->IsRollUp() )
		SetSize();
	SfxDockingWindow::Resize();
}



void SvxColorDockingWindow::GetFocus (void)
{
	SfxDockingWindow::GetFocus();
    // Grab the focus to the color value set so that it can be controlled
    // with the keyboard.
	aColorSet.GrabFocus();
}

long SvxColorDockingWindow::Notify( NotifyEvent& rNEvt )
{
	long nRet = 0;
	if( ( rNEvt.GetType() == EVENT_KEYINPUT ) )
	{
		KeyEvent aKeyEvt = *rNEvt.GetKeyEvent();
		USHORT	 nKeyCode = aKeyEvt.GetKeyCode().GetCode();
		switch( nKeyCode )
		{
			case KEY_ESCAPE:
				GrabFocusToDocument();
				nRet = 1;
				break;
		}
	}

	return nRet ? nRet : SfxDockingWindow::Notify( rNEvt );
}
