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

#include <vcl/svapp.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/dispatch.hxx>
#include <avmedia/mediaplayer.hxx>
#include "helpid.hrc"
#include "galbrws2.hxx"
#include "galtheme.hxx"
#include "galmisc.hxx"
#include "galctrl.hxx"
#include "AccessibleStringWrap.hxx"
#include <svx/svxfont.hxx>

// -----------
// - Defines -
// -----------

#define GALLERY_BRWBOX_TITLE    1
#define GALLERY_BRWBOX_PATH     2

// ------------------
// - GalleryPreview -
// ------------------

GalleryPreview::GalleryPreview( GalleryBrowser2* pParent, GalleryTheme* pTheme ) :
	Window( pParent, WB_TABSTOP | WB_BORDER ),
	DropTargetHelper( this ),
    DragSourceHelper( this ),
    mpTheme( pTheme )
{
	SetHelpId( HID_GALLERY_WINDOW );
    InitSettings();
}

// ------------------------------------------------------------------------

GalleryPreview::GalleryPreview( Window* pParent, const ResId & rResId  ) :
	Window( pParent, rResId ),
	DropTargetHelper( this ),
    DragSourceHelper( this ),
    mpTheme( NULL )
{
	SetHelpId( HID_GALLERY_PREVIEW );
    InitSettings();
}

// ------------------------------------------------------------------------

GalleryPreview::~GalleryPreview()
{
}

// ------------------------------------------------------------------------

void GalleryPreview::InitSettings()
{
	SetBackground( Wallpaper( GALLERY_BG_COLOR ) );
	SetControlBackground( GALLERY_BG_COLOR );
	SetControlForeground( GALLERY_FG_COLOR );
}

// -----------------------------------------------------------------------

void GalleryPreview::DataChanged( const DataChangedEvent& rDCEvt )
{
	if ( ( rDCEvt.GetType() == DATACHANGED_SETTINGS ) && ( rDCEvt.GetFlags() & SETTINGS_STYLE ) )
		InitSettings();
	else
		Window::DataChanged( rDCEvt );
}

// ------------------------------------------------------------------------

BOOL GalleryPreview::ImplGetGraphicCenterRect( const Graphic& rGraphic, Rectangle& rResultRect ) const
{
	const Size	aWinSize( GetOutputSizePixel() );
	Size		aNewSize( LogicToPixel( rGraphic.GetPrefSize(), rGraphic.GetPrefMapMode() ) );
	BOOL		bRet = FALSE;

	if( aNewSize.Width() && aNewSize.Height() )
	{
		// scale to fit window
		const double fGrfWH = (double) aNewSize.Width() / aNewSize.Height();
		const double fWinWH = (double) aWinSize.Width() / aWinSize.Height();

		if ( fGrfWH < fWinWH )
		{
			aNewSize.Width() = (long) ( aWinSize.Height() * fGrfWH );
			aNewSize.Height()= aWinSize.Height();
		}
		else
		{
			aNewSize.Width() = aWinSize.Width();
			aNewSize.Height()= (long) ( aWinSize.Width() / fGrfWH);
		}

		const Point aNewPos( ( aWinSize.Width()  - aNewSize.Width() ) >> 1,
							 ( aWinSize.Height() - aNewSize.Height() ) >> 1 );

		rResultRect = Rectangle( aNewPos, aNewSize );
		bRet = TRUE;
	}

	return bRet;
}

// ------------------------------------------------------------------------

void GalleryPreview::Paint( const Rectangle& rRect )
{
	Window::Paint( rRect );

	if( ImplGetGraphicCenterRect( aGraphicObj.GetGraphic(), aPreviewRect ) )
	{
		const Point aPos( aPreviewRect.TopLeft() );
		const Size	aSize( aPreviewRect.GetSize() );

		if( aGraphicObj.IsAnimated() )
			aGraphicObj.StartAnimation( this, aPos, aSize );
		else
			aGraphicObj.Draw( this, aPos, aSize );
	}
}

// ------------------------------------------------------------------------

void GalleryPreview::MouseButtonDown( const MouseEvent& rMEvt )
{
    if( mpTheme && ( rMEvt.GetClicks() == 2 ) )
        ( (GalleryBrowser2*) GetParent() )->TogglePreview( this );
}

// ------------------------------------------------------------------------

void GalleryPreview::Command(const CommandEvent& rCEvt )
{
	Window::Command( rCEvt );

    if( mpTheme && ( rCEvt.GetCommand() == COMMAND_CONTEXTMENU ) )
        ( (GalleryBrowser2*) GetParent() )->ShowContextMenu( this, 
			( rCEvt.IsMouseEvent() ? &rCEvt.GetMousePosPixel() : NULL ) );
}

// ------------------------------------------------------------------------

void GalleryPreview::KeyInput( const KeyEvent& rKEvt )
{
    if( mpTheme )
    {
        GalleryBrowser2* pBrowser = static_cast< GalleryBrowser2* >( GetParent() );

        switch( rKEvt.GetKeyCode().GetCode() )
        {
            case( KEY_BACKSPACE ):
                pBrowser->TogglePreview( this );
            break;
                
            case( KEY_HOME ):
                pBrowser->Travel( GALLERYBROWSERTRAVEL_FIRST );
            break;

            case( KEY_END ):
                pBrowser->Travel( GALLERYBROWSERTRAVEL_LAST );
            break;

            case( KEY_LEFT ):
            case( KEY_UP ):
                pBrowser->Travel( GALLERYBROWSERTRAVEL_PREVIOUS );
            break;

            case( KEY_RIGHT ):
            case( KEY_DOWN ):
                pBrowser->Travel( GALLERYBROWSERTRAVEL_NEXT );
            break;

            default:
            {
                if( !pBrowser->KeyInput( rKEvt, this ) )
                    Window::KeyInput( rKEvt );
            }
            break;
        }
    }
    else
        Window::KeyInput( rKEvt );
}

// ------------------------------------------------------------------------

sal_Int8 GalleryPreview::AcceptDrop( const AcceptDropEvent& rEvt )
{
    sal_Int8 nRet;

    if( mpTheme )
        nRet = ( (GalleryBrowser2*) GetParent() )->AcceptDrop( *this, rEvt );
    else
        nRet = DND_ACTION_NONE;

    return nRet;
}

// ------------------------------------------------------------------------

sal_Int8 GalleryPreview::ExecuteDrop( const ExecuteDropEvent& rEvt )
{
    sal_Int8 nRet;

    if( mpTheme )
        nRet = ( (GalleryBrowser2*) GetParent() )->ExecuteDrop( *this, rEvt );
    else
        nRet = DND_ACTION_NONE;

    return nRet;
}

// ------------------------------------------------------------------------

void GalleryPreview::StartDrag( sal_Int8, const Point& )
{
    if( mpTheme )
        ( (GalleryBrowser2*) GetParent() )->StartDrag( this );
}

// ------------------------------------------------------------------------

void GalleryPreview::PreviewMedia( const INetURLObject& rURL )
{
	if( rURL.GetProtocol() != INET_PROT_NOT_VALID )
	{
		::avmedia::MediaFloater* pFloater = AVMEDIA_MEDIAWINDOW();
		
		if( !pFloater )
		{
			SfxViewFrame::Current()->GetBindings().GetDispatcher()->Execute( SID_AVMEDIA_PLAYER, SFX_CALLMODE_SYNCHRON );
			pFloater = AVMEDIA_MEDIAWINDOW();
		}
		
		if( pFloater )
			pFloater->setURL( rURL.GetMainURL( INetURLObject::DECODE_UNAMBIGUOUS ), true );
	}
}

// ------------------------------------------------------------------------

// -------------------
// - GalleryIconView -
// -------------------

GalleryIconView::GalleryIconView( GalleryBrowser2* pParent, GalleryTheme* pTheme ) :
		ValueSet( pParent, WB_TABSTOP | WB_3DLOOK | WB_BORDER | WB_ITEMBORDER | WB_DOUBLEBORDER | WB_VSCROLL | WB_FLATVALUESET ),
		DropTargetHelper( this ),
        DragSourceHelper( this ),
		mpTheme	( pTheme )
{
	EnableFullItemMode( FALSE );

	SetHelpId( HID_GALLERY_WINDOW );
    InitSettings();
	SetExtraSpacing( 2 );
	SetItemWidth( S_THUMB + 6 );
	SetItemHeight( S_THUMB + 6 );
}

// ------------------------------------------------------------------------

GalleryIconView::~GalleryIconView()
{
}

// ------------------------------------------------------------------------

void GalleryIconView::InitSettings()
{
	SetBackground( Wallpaper( GALLERY_BG_COLOR ) );
	SetControlBackground( GALLERY_BG_COLOR );
	SetControlForeground( GALLERY_FG_COLOR );
	SetColor( GALLERY_BG_COLOR );
}

// -----------------------------------------------------------------------

void GalleryIconView::DataChanged( const DataChangedEvent& rDCEvt )
{
	if ( ( rDCEvt.GetType() == DATACHANGED_SETTINGS ) && ( rDCEvt.GetFlags() & SETTINGS_STYLE ) )
		InitSettings();
	else
		ValueSet::DataChanged( rDCEvt );
}

// ------------------------------------------------------------------------

void GalleryIconView::UserDraw( const UserDrawEvent& rUDEvt )
{
	const USHORT nId = rUDEvt.GetItemId();

	if( nId && mpTheme )
	{
		SgaObject* pObj = mpTheme->AcquireObject( nId - 1 );

		if( pObj )
		{
			const Rectangle&	rRect = rUDEvt.GetRect();
			OutputDevice*		pDev = rUDEvt.GetDevice();
			Graphic 			aGraphic;

			if( pObj->IsThumbBitmap() )
			{
				Bitmap aBmp( pObj->GetThumbBmp() );

				if( pObj->GetObjKind() == SGA_OBJ_SOUND )
					aBmp.Replace( COL_LIGHTMAGENTA, COL_WHITE );

				if( ( pDev->GetBitCount() <= 8 ) && ( aBmp.GetBitCount() >= 8 ) )
					aBmp.Dither( BMP_DITHER_FLOYD );

				aGraphic = aBmp;
			}
			else
				aGraphic = pObj->GetThumbMtf();

			Size aSize( aGraphic.GetSizePixel( pDev ) );

			if ( aSize.Width() && aSize.Height() )
			{
				if( ( aSize.Width() > rRect.GetWidth() ) || ( aSize.Height() > rRect.GetHeight() ) )
				{
					Point			aNewPos;
					const double	fBmpWH	= (double) aSize.Width() / aSize.Height();
					const double	fThmpWH = (double) rRect.GetWidth() / rRect.GetHeight();

					// Bitmap an Thumbgroesse anpassen
					if ( fBmpWH < fThmpWH )
					{
						aSize.Width() = (long) ( rRect.GetHeight() * fBmpWH );
						aSize.Height()= rRect.GetHeight();
					}
					else
					{
						aSize.Width() = rRect.GetWidth();
						aSize.Height()= (long) ( rRect.GetWidth() / fBmpWH );
					}
				}

				const Point aPos( ( ( rRect.GetWidth() - aSize.Width() ) >> 1 ) + rRect.Left(),
								  ( ( rRect.GetHeight() - aSize.Height() ) >> 1 ) + rRect.Top() );

				aGraphic.Draw( pDev, aPos, aSize );
			}

			SetItemText( nId, GalleryBrowser2::GetItemText( *mpTheme, *pObj, GALLERY_ITEM_THEMENAME | GALLERY_ITEM_TITLE | GALLERY_ITEM_PATH ) );
			mpTheme->ReleaseObject( pObj );
		}
	}
}

// ------------------------------------------------------------------------

void GalleryIconView::MouseButtonDown( const MouseEvent& rMEvt )
{
    ValueSet::MouseButtonDown( rMEvt );

    if( rMEvt.GetClicks() == 2 )
        ( (GalleryBrowser2*) GetParent() )->TogglePreview( this, &rMEvt.GetPosPixel() );
}

// ------------------------------------------------------------------------

void GalleryIconView::Command( const CommandEvent& rCEvt )
{
	ValueSet::Command( rCEvt );

    if( rCEvt.GetCommand() == COMMAND_CONTEXTMENU )
	{
        ( (GalleryBrowser2*) GetParent() )->ShowContextMenu( this, 
			( rCEvt.IsMouseEvent() ? &rCEvt.GetMousePosPixel() : NULL ) );
	}
}

// ------------------------------------------------------------------------

void GalleryIconView::KeyInput( const KeyEvent& rKEvt )
{
    if( !mpTheme || !static_cast< GalleryBrowser2* >( GetParent() )->KeyInput( rKEvt, this ) )
        ValueSet::KeyInput( rKEvt );
}

// ------------------------------------------------------------------------

sal_Int8 GalleryIconView::AcceptDrop( const AcceptDropEvent& rEvt )
{
	return( static_cast< GalleryBrowser2* >( GetParent() )->AcceptDrop( *this, rEvt ) );
}

// ------------------------------------------------------------------------

sal_Int8 GalleryIconView::ExecuteDrop( const ExecuteDropEvent& rEvt )
{
	return(	static_cast< GalleryBrowser2* >( GetParent() )->ExecuteDrop( *this, rEvt ) );
}

// ------------------------------------------------------------------------

void GalleryIconView::StartDrag( sal_Int8, const Point& )
{
	const CommandEvent	aEvt( GetPointerPosPixel(), COMMAND_STARTDRAG, TRUE );
	Region				aRegion;

	// call this to initiate dragging for ValueSet
    ValueSet::StartDrag( aEvt, aRegion );
    static_cast< GalleryBrowser2* >( GetParent() )->StartDrag( this );
}

// -------------------
// - GalleryListView -
// -------------------

GalleryListView::GalleryListView( GalleryBrowser2* pParent, GalleryTheme* pTheme ) :
    BrowseBox( pParent, WB_TABSTOP | WB_3DLOOK | WB_BORDER ),
    mpTheme( pTheme ),
    mnCurRow( 0 ),
    mbInit( FALSE )
{
	SetHelpId( HID_GALLERY_WINDOW );

    InitSettings();

    SetMode( BROWSER_AUTO_VSCROLL | BROWSER_AUTOSIZE_LASTCOL );
    SetDataRowHeight( 28 );
	InsertDataColumn( GALLERY_BRWBOX_TITLE, String( GAL_RESID( RID_SVXSTR_GALLERY_TITLE ) ), 256  );
	InsertDataColumn( GALLERY_BRWBOX_PATH, String( GAL_RESID( RID_SVXSTR_GALLERY_PATH ) ), 256 );
}

// ------------------------------------------------------------------------

GalleryListView::~GalleryListView()
{
}

// ------------------------------------------------------------------------

void GalleryListView::InitSettings()
{
	SetBackground( Wallpaper( GALLERY_BG_COLOR ) );
	SetControlBackground( GALLERY_BG_COLOR );
	SetControlForeground( GALLERY_FG_COLOR );
}

// -----------------------------------------------------------------------

void GalleryListView::DataChanged( const DataChangedEvent& rDCEvt )
{
	if ( ( rDCEvt.GetType() == DATACHANGED_SETTINGS ) && ( rDCEvt.GetFlags() & SETTINGS_STYLE ) )
		InitSettings();
	else
		BrowseBox::DataChanged( rDCEvt );
}

// ------------------------------------------------------------------------

BOOL GalleryListView::SeekRow( long nRow )
{
    mnCurRow = nRow;
    return TRUE;
}

// -----------------------------------------------------------------------------

String GalleryListView::GetCellText(long _nRow, USHORT nColumnId) const
{
	String sRet;
	if( mpTheme && ( _nRow < static_cast< long >( mpTheme->GetObjectCount() ) ) )
	{
    	SgaObject* pObj = mpTheme->AcquireObject( _nRow );

		if( pObj )
		{
			sRet = GalleryBrowser2::GetItemText( *mpTheme, *pObj, 
				( GALLERY_BRWBOX_TITLE == nColumnId ) ? GALLERY_ITEM_TITLE : GALLERY_ITEM_PATH );

		    mpTheme->ReleaseObject( pObj );
		}
	}

	return sRet;;
}

// -----------------------------------------------------------------------------

Rectangle GalleryListView::GetFieldCharacterBounds(sal_Int32 _nRow,sal_Int32 _nColumnPos,sal_Int32 nIndex)
{
    DBG_ASSERT(_nColumnPos >= 0 && _nColumnPos <= USHRT_MAX, "GalleryListView::GetFieldCharacterBounds: _nColumnId overflow");
	Rectangle aRect;
	if ( SeekRow(_nRow) )
	{
        SvxFont aFont( GetFont() );
        AccessibleStringWrap aStringWrap( *this, aFont, GetCellText(_nRow, sal::static_int_cast<USHORT>( GetColumnId( sal::static_int_cast<USHORT>(_nColumnPos) ) ) ) );
        
        // get the bounds inside the string
        aStringWrap.GetCharacterBounds(nIndex, aRect);

        // offset to 
	}
	return aRect;
}

// -----------------------------------------------------------------------------

sal_Int32 GalleryListView::GetFieldIndexAtPoint(sal_Int32 _nRow,sal_Int32 _nColumnPos,const Point& _rPoint)
{
    DBG_ASSERT(_nColumnPos >= 0 && _nColumnPos <= USHRT_MAX, "GalleryListView::GetFieldIndexAtPoint: _nColumnId overflow");
	sal_Int32 nRet = -1;
	if ( SeekRow(_nRow) )
	{
        SvxFont aFont( GetFont() );
        AccessibleStringWrap aStringWrap( *this, aFont, GetCellText(_nRow, sal::static_int_cast<USHORT>(GetColumnId(sal::static_int_cast<USHORT>(_nColumnPos)))) );
        nRet = aStringWrap.GetIndexAtPoint(_rPoint);
	}
	return nRet;
}

// ------------------------------------------------------------------------

void GalleryListView::PaintField( OutputDevice& rDev, const Rectangle& rRect, USHORT nColumnId ) const
{
    rDev.Push( PUSH_CLIPREGION );
    rDev.IntersectClipRegion( rRect );

	if( mpTheme && ( mnCurRow < mpTheme->GetObjectCount() ) )
	{
    	SgaObject* pObj = mpTheme->AcquireObject( mnCurRow );

		if( pObj )
		{
            const long nTextPosY = rRect.Top() + ( ( rRect.GetHeight() - rDev.GetTextHeight() ) >> 1 );

            if( GALLERY_BRWBOX_TITLE == nColumnId )
            {
                Rectangle       aOutputRect( rRect.TopLeft(), Size( rRect.GetHeight(), rRect.GetHeight() ) );
    			GraphicObject   aGrfObj;

                if( pObj->GetObjKind() == SGA_OBJ_SOUND )
                    aGrfObj = Graphic( BitmapEx( GAL_RESID( RID_SVXBMP_GALLERY_MEDIA ) ) );
                else if( pObj->IsThumbBitmap() )
				    aGrfObj = Graphic( pObj->GetThumbBmp() );
			    else
				    aGrfObj = Graphic( pObj->GetThumbMtf() );

			    Size aSize( rDev.LogicToPixel( aGrfObj.GetPrefSize(), aGrfObj.GetPrefMapMode() ) );

			    if( aSize.Width() && aSize.Height() )
			    {
				    if( ( aSize.Width() > aOutputRect.GetWidth() ) || ( aSize.Height() > aOutputRect.GetHeight() ) )
				    {
					    Point			aNewPos;
					    const double	fBmpWH	= (double) aSize.Width() / aSize.Height();
					    const double	fThmpWH = (double) aOutputRect.GetWidth() / aOutputRect.GetHeight();

					    // Bitmap an Thumbgroesse anpassen
					    if ( fBmpWH < fThmpWH )
					    {
						    aSize.Width() = (long) ( aOutputRect.GetHeight() * fBmpWH );
						    aSize.Height()= aOutputRect.GetHeight();
					    }
					    else
					    {
						    aSize.Width() = aOutputRect.GetWidth();
						    aSize.Height()= (long) ( aOutputRect.GetWidth() / fBmpWH );
					    }
				    }

                    aSize.Width() = Max( aSize.Width(), 4L );
                    aSize.Height() = Max( aSize.Height(), 4L );

				    const Point aPos( ( ( aOutputRect.GetWidth() - aSize.Width() ) >> 1 ) + aOutputRect.Left(),
								      ( ( aOutputRect.GetHeight() - aSize.Height() ) >> 1 ) + aOutputRect.Top() );

				    aGrfObj.Draw( &rDev, aPos, aSize );
			    }

			    rDev.DrawText( Point( aOutputRect.Right() + 6, nTextPosY ), GalleryBrowser2::GetItemText( *mpTheme, *pObj, GALLERY_ITEM_TITLE ) );
            }
            else if( GALLERY_BRWBOX_PATH == nColumnId )
                rDev.DrawText( Point( rRect.Left(), nTextPosY ), GalleryBrowser2::GetItemText( *mpTheme, *pObj, GALLERY_ITEM_PATH ) );

		    mpTheme->ReleaseObject( pObj );
		}
	}

    rDev.Pop();
}

// ------------------------------------------------------------------------

void GalleryListView::Command( const CommandEvent& rCEvt )
{
	BrowseBox::Command( rCEvt );

    if( rCEvt.GetCommand() == COMMAND_CONTEXTMENU )
	{
		const Point* pPos = NULL;

		if( rCEvt.IsMouseEvent() && ( GetRowAtYPosPixel( rCEvt.GetMousePosPixel().Y() ) != BROWSER_ENDOFSELECTION ) )
			pPos = &rCEvt.GetMousePosPixel();
		
		( (GalleryBrowser2*) GetParent() )->ShowContextMenu( this, pPos );
	}
}

// ------------------------------------------------------------------------

void GalleryListView::KeyInput( const KeyEvent& rKEvt )
{
    if( !mpTheme || !static_cast< GalleryBrowser2* >( GetParent() )->KeyInput( rKEvt, this ) )
        BrowseBox::KeyInput( rKEvt );
}

// ------------------------------------------------------------------------

void GalleryListView::DoubleClick( const BrowserMouseEvent& rEvt )
{
    BrowseBox::DoubleClick( rEvt );

    if( rEvt.GetRow() != BROWSER_ENDOFSELECTION )
        ( (GalleryBrowser2*) GetParent() )->TogglePreview( this, &rEvt.GetPosPixel() );
}

// ------------------------------------------------------------------------

void GalleryListView::Select()
{
    if( maSelectHdl.IsSet() )
        maSelectHdl.Call( this );
}

// ------------------------------------------------------------------------

sal_Int8 GalleryListView::AcceptDrop( const BrowserAcceptDropEvent& )
{
    sal_Int8 nRet = DND_ACTION_NONE;

   	if( mpTheme && !mpTheme->IsReadOnly() && !mpTheme ->IsImported() )
	{
		if( !mpTheme->IsDragging() )
			nRet = DND_ACTION_COPY;
		else
			nRet = DND_ACTION_COPY;
	}

	return nRet;
}

// ------------------------------------------------------------------------

sal_Int8 GalleryListView::ExecuteDrop( const BrowserExecuteDropEvent& rEvt )
{
    ExecuteDropEvent aEvt( rEvt );

    aEvt.maPosPixel.Y() += GetTitleHeight();
    
    return(	( (GalleryBrowser2*) GetParent() )->ExecuteDrop( *this, aEvt ) );
}

// ------------------------------------------------------------------------

void GalleryListView::StartDrag( sal_Int8, const Point& rPosPixel )
{
    ( (GalleryBrowser2*) GetParent() )->StartDrag( this, &rPosPixel );
}
