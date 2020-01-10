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
#include <svtools/pathoptions.hxx>
#include <svtools/intitem.hxx>
#include <vcl/msgbox.hxx>
#include <tools/urlobj.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/docfile.hxx>
#include <svtools/wallitem.hxx>
#include <svtools/cntwall.hxx>
#ifndef _CNTIDS_HRC
#include <sfx2/cntids.hrc>
#endif

#define _SVX_BACKGRND_CXX

#include <svx/dialogs.hrc>
#include "backgrnd.hrc"
#include <svx/svxitems.hrc>




// Tabellenhintergrund
#define TBL_DEST_CELL 	0
#define TBL_DEST_ROW 	1
#define TBL_DEST_TBL	2

#include <svx/brshitem.hxx>
#include "backgrnd.hxx"

#include <svx/xtable.hxx>
#include "opengrf.hxx"
#include "impgrf.hxx"
#include "svxerr.hxx"
#include "drawitem.hxx"
#include <svx/dialmgr.hxx>
#include "htmlmode.hxx"
#ifndef _SVT_CONTROLDIMS_HRC_
#include <svtools/controldims.hrc>
#endif
#include <svx/svxids.hrc> //CHINA001
#include "flagsdef.hxx"	//CHINA001
#include <svtools/intitem.hxx> //CHINA001
#include <sfx2/request.hxx> //CHINA001
using namespace ::com::sun::star;
// static ----------------------------------------------------------------

static USHORT pRanges[] =
{
	SID_VIEW_FLD_PIC, SID_VIEW_FLD_PIC,
	SID_ATTR_BRUSH, SID_ATTR_BRUSH,
	SID_ATTR_BRUSH_CHAR, SID_ATTR_BRUSH_CHAR,
	0
};

struct SvxBackgroundTable_Impl
{
	SvxBrushItem*	pCellBrush;
	SvxBrushItem*	pRowBrush;
	SvxBrushItem*	pTableBrush;
    USHORT          nCellWhich;
    USHORT          nRowWhich;
    USHORT          nTableWhich;
    USHORT          nActPos;

	SvxBackgroundTable_Impl() :
        pCellBrush(NULL), pRowBrush(NULL), pTableBrush(NULL),
        nCellWhich(0), nRowWhich(0), nTableWhich(0) {}
};

struct SvxBackgroundPara_Impl
{
	SvxBrushItem*	pParaBrush;
	SvxBrushItem*	pCharBrush;

	USHORT 			nActPos;

	SvxBackgroundPara_Impl() :
		pParaBrush(NULL), pCharBrush(NULL) {}
};

struct SvxBackgroundPage_Impl
{
	Timer*          pLoadTimer;
	BOOL            bIsImportDlgInExecute;

	SvxBackgroundPage_Impl() :
		pLoadTimer(NULL), bIsImportDlgInExecute(FALSE) {}
};
/* -----------------------------15.08.2002 12:21------------------------------

 ---------------------------------------------------------------------------*/
inline BYTE lcl_PercentToTransparency(long nPercent)
{
    //0xff must not be returned!
    return BYTE(nPercent ? (50 + 0xfe * nPercent) / 100 : 0);
}
inline BYTE lcl_TransparencyToPercent(BYTE nTrans)
{
    return (nTrans * 100 + 127) / 254;
}
void lcl_SetTransparency(SvxBrushItem& rBrush, long nTransparency)
{
    uno::Any aTransparency;
    aTransparency <<= (sal_Int8)nTransparency;
    rBrush.PutValue(aTransparency, MID_GRAPHIC_TRANSPARENCY);
}
//-------------------------------------------------------------------------

/*	[Beschreibung]

*/

USHORT GetItemId_Impl( ValueSet& rValueSet, const Color& rCol )
{
	BOOL	bFound = FALSE;
	USHORT	nCount = rValueSet.GetItemCount();
	USHORT	n	   = 1;

	while ( !bFound && n <= nCount )
	{
		Color aValCol = rValueSet.GetItemColor(n);

		bFound = (   aValCol.GetRed()   == rCol.GetRed()
				  && aValCol.GetGreen() == rCol.GetGreen()
				  && aValCol.GetBlue()  == rCol.GetBlue() );

		if ( !bFound )
			n++;
	}
	return bFound ? n : 0;
}

// class BackgroundPreview -----------------------------------------------

/*	[Beschreibung]

	Vorschaufenster f"ur Brush oder Bitmap
*/

class BackgroundPreviewImpl : public Window
{
public:
	BackgroundPreviewImpl( Window* pParent,
						   const ResId& rResId, BOOL bIsBmpPreview );
	~BackgroundPreviewImpl();

	void			NotifyChange( const Color&  rColor );
	void			NotifyChange( const Bitmap* pBitmap );

protected:
	virtual void	Paint( const Rectangle& rRect );
    virtual void    DataChanged( const DataChangedEvent& rDCEvt );

private:
	const BOOL		bIsBmp;
	Bitmap*			pBitmap;
	Point			aDrawPos;
	Size			aDrawSize;
	Rectangle		aDrawRect;
    BYTE            nTransparency;
};

//-----------------------------------------------------------------------

BackgroundPreviewImpl::BackgroundPreviewImpl
(
	Window* pParent,
	const ResId& rResId,
	BOOL bIsBmpPreview
) :

/*	[Beschreibung]

*/

	Window( pParent, rResId ),

	bIsBmp	 ( bIsBmpPreview ),
	pBitmap	 ( NULL ),
    aDrawRect( Point(0,0), GetOutputSizePixel() ),
    nTransparency(0)

{
    SetBorderStyle(WINDOW_BORDER_MONO);
    Paint( aDrawRect );
}

//-----------------------------------------------------------------------

BackgroundPreviewImpl::~BackgroundPreviewImpl()

/*	[Beschreibung]

*/

{
	delete pBitmap;
}

//-----------------------------------------------------------------------
void BackgroundPreviewImpl::NotifyChange( const Color& rColor )
{
	if ( !bIsBmp )
	{
		const static Color aTranspCol( COL_TRANSPARENT );

        nTransparency = lcl_TransparencyToPercent( rColor.GetTransparency() );

		SetFillColor( rColor == aTranspCol ? GetSettings().GetStyleSettings().GetFieldColor() : (Color) rColor.GetRGBColor() );
        Paint( aDrawRect );
	}
}

//-----------------------------------------------------------------------

void BackgroundPreviewImpl::NotifyChange( const Bitmap* pNewBitmap )
/*	[Beschreibung]

*/

{
    if ( bIsBmp && (pNewBitmap || pBitmap) )
	{
        if ( pNewBitmap && pBitmap )
			*pBitmap = *pNewBitmap;
		else if ( pNewBitmap && !pBitmap )
			pBitmap = new Bitmap( *pNewBitmap );
		else if ( !pNewBitmap )
			DELETEZ( pBitmap );

		if ( pBitmap )
		{
			Size aSize = GetOutputSizePixel();
			// InnerSize == Size without one pixel border
			Size aInnerSize = aSize;
			aInnerSize.Width() -= 2;
			aInnerSize.Height() -= 2;
			aDrawSize = pBitmap->GetSizePixel();

			// bitmap bigger than preview window?
			if ( aDrawSize.Width() > aInnerSize.Width() )
			{
				aDrawSize.Height() = aDrawSize.Height() * aInnerSize.Width() / aDrawSize.Width();
				if ( aDrawSize.Height() > aInnerSize.Height() )
				{
					aDrawSize.Width() = aDrawSize.Height();
					aDrawSize.Height() = aInnerSize.Height();
				}
				else
					aDrawSize.Width() = aInnerSize.Width();
			}
			else if ( aDrawSize.Height() > aInnerSize.Height() )
			{
				aDrawSize.Width() = aDrawSize.Width() * aInnerSize.Height() / aDrawSize.Height();
				if ( aDrawSize.Width() > aInnerSize.Width() )
				{
					aDrawSize.Height() = aDrawSize.Width();
					aDrawSize.Width() = aInnerSize.Width();
				}
				else
					aDrawSize.Height() = aInnerSize.Height();
			}

			aDrawPos.X() = (aSize.Width()  - aDrawSize.Width())  / 2;
			aDrawPos.Y() = (aSize.Height() - aDrawSize.Height()) / 2;
		}
		Invalidate( aDrawRect );
		Update();
	}
}

//-----------------------------------------------------------------------

void BackgroundPreviewImpl::Paint( const Rectangle& )
{
    const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();
    SetBackground(Wallpaper(rStyleSettings.GetWindowColor()));
    SetLineColor();
    if(bIsBmp)
        SetFillColor( Color(COL_TRANSPARENT) );
    DrawRect( aDrawRect );
	if ( bIsBmp )
	{
		if ( pBitmap )
			DrawBitmap( aDrawPos, aDrawSize, *pBitmap );
		else
		{
			Size aSize = GetOutputSizePixel();
            DrawLine( Point(0,0),               Point(aSize.Width(),aSize.Height()) );
			DrawLine( Point(0,aSize.Height()),	Point(aSize.Width(),0) );
		}
	}
}
/* -----------------------------27.02.2002 11:07------------------------------

 ---------------------------------------------------------------------------*/
void BackgroundPreviewImpl::DataChanged( const DataChangedEvent& rDCEvt )
{
	if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
         (rDCEvt.GetFlags() & SETTINGS_STYLE) )
    {
        Invalidate();
    }
    Window::DataChanged( rDCEvt );
}

// class SvxBackgroundTabPage --------------------------------------------

#define HDL(hdl) LINK(this,SvxBackgroundTabPage,hdl)

SvxBackgroundTabPage::SvxBackgroundTabPage( Window* pParent,
											const SfxItemSet& rCoreSet ) :

/*	[Beschreibung]

*/

	SvxTabPage( pParent, SVX_RES( RID_SVXPAGE_BACKGROUND ), rCoreSet ),

	aBorderWin			( this, SVX_RES(CT_BORDER) ),
	aBackgroundColorSet ( &aBorderWin, SVX_RES( SET_BGDCOLOR ) ),
	aBackgroundColorBox ( this, SVX_RES( GB_BGDCOLOR ) ),
	pPreviewWin1        ( new BackgroundPreviewImpl(
							this, SVX_RES( WIN_PREVIEW1 ), FALSE ) ),
    aColTransFT         ( this, SVX_RES( FT_COL_TRANS ) ),
    aColTransMF         ( this, SVX_RES( MF_COL_TRANS ) ),
	aBtnBrowse	        ( this, SVX_RES( BTN_BROWSE ) ),
	aBtnLink	        ( this, SVX_RES( BTN_LINK ) ),
	aBtnPreview	        ( this, SVX_RES( BTN_PREVIEW ) ),
	aFtFile		        ( this, SVX_RES( FT_FILE ) ),
	aGbFile		        ( this, SVX_RES( GB_FILE ) ),
	aBtnPosition        ( this, SVX_RES( BTN_POSITION ) ),
	aBtnArea	        ( this, SVX_RES( BTN_AREA ) ),
	aBtnTile	        ( this, SVX_RES( BTN_TILE ) ),
	aWndPosition		( this, SVX_RES( WND_POSITION ), RP_MM ),
	aGbPosition			( this, SVX_RES( GB_POSITION ) ),
    aGraphTransFL       ( this, SVX_RES( FL_GRAPH_TRANS ) ),
    aGraphTransMF       ( this, SVX_RES( MF_GRAPH_TRANS ) ),
    pPreviewWin2        ( new BackgroundPreviewImpl(
							this, SVX_RES( WIN_PREVIEW2 ), TRUE ) ),
	aSelectTxt			( this, SVX_RES( FT_SELECTOR ) ),
	aLbSelect			( this, SVX_RES( LB_SELECTOR ) ),
    aStrBrowse          ( SVX_RES( STR_BROWSE ) ),
    aStrUnlinked        ( SVX_RES( STR_UNLINKED ) ),
    aTblDesc            ( this, SVX_RES( FT_TBL_DESC ) ),
	aTblLBox            ( this, SVX_RES( LB_TBL_BOX ) ),
	aParaLBox           ( this, SVX_RES( LB_PARA_BOX ) ),
    nHtmlMode           ( 0 ),
	bAllowShowSelector	( TRUE ),
	bIsGraphicValid		( FALSE ),
	bLinkOnly			( FALSE ),
    bResized            ( FALSE ),
    bColTransparency    ( FALSE ),
    bGraphTransparency  ( FALSE ),

	pPageImpl           ( new SvxBackgroundPage_Impl ),
	pImportDlg			( NULL ),
	pTableBck_Impl		( NULL ),
	pParaBck_Impl       ( NULL )

{
	// diese Page braucht ExchangeSupport
	SetExchangeSupport();

	const SfxPoolItem* pItem;
	SfxObjectShell* pShell;

	if ( SFX_ITEM_SET == rCoreSet.GetItemState( SID_HTML_MODE, FALSE, &pItem )
		 || ( 0 != ( pShell = SfxObjectShell::Current()) &&
			  0 != ( pItem = pShell->GetItem( SID_HTML_MODE ) ) ) )
	{
		nHtmlMode = ((SfxUInt16Item*)pItem)->GetValue();
	}

	FillColorValueSets_Impl();

	aBackgroundColorSet.SetSelectHdl( HDL(BackgroundColorHdl_Impl) );
	FreeResource();
}

//------------------------------------------------------------------------

SvxBackgroundTabPage::~SvxBackgroundTabPage()

/*	[Beschreibung]

*/

{
	delete pPreviewWin1;
	delete pPreviewWin2;
	delete pPageImpl->pLoadTimer;
	delete pPageImpl;
	delete pImportDlg;

	if( pTableBck_Impl)
	{
		delete pTableBck_Impl->pCellBrush;
		delete pTableBck_Impl->pRowBrush;
		delete pTableBck_Impl->pTableBrush;
		delete pTableBck_Impl;
	}

	if(pParaBck_Impl)
	{
		delete pParaBck_Impl->pParaBrush;
		delete pParaBck_Impl->pCharBrush;
		delete pParaBck_Impl;
	}
}

//------------------------------------------------------------------------

USHORT* SvxBackgroundTabPage::GetRanges()

/*	[Beschreibung]

	gibt den Bereich der Which-Werte zur"uck
*/

{
	return pRanges;
}

//------------------------------------------------------------------------

SfxTabPage* SvxBackgroundTabPage::Create( Window* pParent,
										  const SfxItemSet& rAttrSet )

/*	[Beschreibung]

	Create-Methode f"ur den TabDialog
*/

{
	return ( new SvxBackgroundTabPage( pParent, rAttrSet ) );
}

//------------------------------------------------------------------------

void SvxBackgroundTabPage::Reset( const SfxItemSet& rSet )

/*	[Beschreibung]

*/

{
// os: So ein Unsinn! Irgendwo laesst sich so ein Item immer finden,
//		es muss aber im rSet vorhanden sein!
//	const SfxPoolItem* pX = GetOldItem( rSet, SID_VIEW_FLD_PIC );
// 	if( pX && pX->ISA(SfxWallpaperItem))
	if(SFX_ITEM_AVAILABLE <= rSet.GetItemState(GetWhich(SID_VIEW_FLD_PIC), FALSE))
	{
		ResetFromWallpaperItem( rSet );
		return;
	}

	// Zustand des Vorschau-Buttons durch UserData persistent
	String aUserData = GetUserData();
	aBtnPreview.Check( aUserData.Len() && sal_Unicode('1') == aUserData.GetChar( 0 ) );

	// nach Reset kein ShowSelector() mehr aufrufen d"urfen
	bAllowShowSelector = FALSE;


	// Input-BrushItem besorgen und auswerten
	const SvxBrushItem* pBgdAttr = NULL;
	USHORT nSlot = SID_ATTR_BRUSH;
	const SfxPoolItem* pItem;
	USHORT nDestValue = USHRT_MAX;

	if ( SFX_ITEM_SET == rSet.GetItemState(	SID_BACKGRND_DESTINATION,
											FALSE, &pItem ) )
	{
		nDestValue = ((const SfxUInt16Item*)pItem)->GetValue();
		aTblLBox.SelectEntryPos(nDestValue);

		switch ( nDestValue )
		{
			case TBL_DEST_CELL:
				nSlot = SID_ATTR_BRUSH;
			break;
			case TBL_DEST_ROW:
				nSlot = SID_ATTR_BRUSH_ROW;
			break;
			case TBL_DEST_TBL:
				nSlot = SID_ATTR_BRUSH_TABLE;
			break;
		}
	}
	else if( SFX_ITEM_SET == rSet.GetItemState(
				SID_PARA_BACKGRND_DESTINATION, FALSE, &pItem ) )
	{
		nDestValue = ((const SfxUInt16Item*)pItem)->GetValue();
		// ist gerade Zeichen aktiviert?
		USHORT nParaSel  = aParaLBox.GetSelectEntryPos();
		if(1 == nParaSel)
		{
			// dann war das ein "Standard" - Aufruf
			nDestValue = nParaSel;
		}
		aParaLBox.SelectEntryPos(nDestValue);

		switch ( nDestValue )
		{
			case PARA_DEST_PARA:
				nSlot = SID_ATTR_BRUSH;
			break;
			case PARA_DEST_CHAR:
				nSlot = SID_ATTR_BRUSH_CHAR;
			break;
		}
	}
    //#111173# the destination item is missing when the parent style has been changed
    if(USHRT_MAX == nDestValue && (aParaLBox.IsVisible()||aTblLBox.IsVisible()))
        nDestValue = 0;
	USHORT nWhich = GetWhich( nSlot );

	if ( rSet.GetItemState( nWhich, FALSE ) >= SFX_ITEM_AVAILABLE )
		pBgdAttr = (const SvxBrushItem*)&( rSet.Get( nWhich ) );

	aBtnTile.Check();

	if ( pBgdAttr )
	{
		FillControls_Impl(*pBgdAttr, aUserData);
		aBgdColor = ( (SvxBrushItem*)pBgdAttr )->GetColor();
	}
	else
	{
		aSelectTxt.Hide();
		aLbSelect.Hide();
		aLbSelect.SelectEntryPos( 0 );
		ShowColorUI_Impl();

		const SfxPoolItem* pOld = GetOldItem( rSet, SID_ATTR_BRUSH );

		if ( pOld )
			aBgdColor = ( (SvxBrushItem*)pOld )->GetColor();
	}

	if ( nDestValue != USHRT_MAX )
	{
		if(aTblLBox.IsVisible())
		{
			USHORT nValue = aTblLBox.GetSelectEntryPos();

			if ( pTableBck_Impl )
			{
                DELETEZ( pTableBck_Impl->pCellBrush);
                DELETEZ( pTableBck_Impl->pRowBrush);
                DELETEZ( pTableBck_Impl->pTableBrush);
			}
			else
				pTableBck_Impl = new SvxBackgroundTable_Impl();

			pTableBck_Impl->nActPos = nValue;

			nWhich = GetWhich( SID_ATTR_BRUSH );
			if ( rSet.GetItemState( nWhich, FALSE ) >= SFX_ITEM_AVAILABLE )
            {
				pBgdAttr = (const SvxBrushItem*)&( rSet.Get( nWhich ) );
                pTableBck_Impl->pCellBrush = new SvxBrushItem(*pBgdAttr);
            }
            pTableBck_Impl->nCellWhich = nWhich;

			if ( rSet.GetItemState( SID_ATTR_BRUSH_ROW, FALSE ) >= SFX_ITEM_AVAILABLE )
            {
				pBgdAttr = (const SvxBrushItem*)&( rSet.Get( SID_ATTR_BRUSH_ROW ) );
                pTableBck_Impl->pRowBrush = new SvxBrushItem(*pBgdAttr);
            }
            pTableBck_Impl->nRowWhich = SID_ATTR_BRUSH_ROW;

			if ( rSet.GetItemState( SID_ATTR_BRUSH_TABLE, FALSE ) >= SFX_ITEM_AVAILABLE )
            {
				pBgdAttr = (const SvxBrushItem*)&( rSet.Get( SID_ATTR_BRUSH_TABLE ) );
                pTableBck_Impl->pTableBrush = new SvxBrushItem(*pBgdAttr);
            }
            pTableBck_Impl->nTableWhich = SID_ATTR_BRUSH_TABLE;

			TblDestinationHdl_Impl(&aTblLBox);
			aTblLBox.SaveValue();
		}
		else
		{
			USHORT nValue = aParaLBox.GetSelectEntryPos();

			if ( pParaBck_Impl )
			{
				delete pParaBck_Impl->pParaBrush;
				delete pParaBck_Impl->pCharBrush;
			}
			else
				pParaBck_Impl = new SvxBackgroundPara_Impl();

			pParaBck_Impl->nActPos = nValue;

			nWhich = GetWhich( SID_ATTR_BRUSH );
			if ( rSet.GetItemState( nWhich, FALSE ) >= SFX_ITEM_AVAILABLE )
            {
				pBgdAttr = (const SvxBrushItem*)&( rSet.Get( nWhich ) );
			    pParaBck_Impl->pParaBrush = new SvxBrushItem(*pBgdAttr);
            }

			nWhich = GetWhich( SID_ATTR_BRUSH_CHAR );
			SfxItemState eState = rSet.GetItemState( nWhich, TRUE );
			eState = rSet.GetItemState( nWhich, FALSE );
			if ( rSet.GetItemState( nWhich, TRUE ) > SFX_ITEM_AVAILABLE )
			{
				pBgdAttr = (const SvxBrushItem*)&( rSet.Get( nWhich ) );
				pParaBck_Impl->pCharBrush = new SvxBrushItem(*pBgdAttr);
			}
			else
				pParaBck_Impl->pCharBrush = new SvxBrushItem(SID_ATTR_BRUSH_CHAR);

			ParaDestinationHdl_Impl(&aParaLBox);
			aParaLBox.SaveValue();
		}
	}
    if(!bResized)
    {
        if(!aLbSelect.IsVisible() && !aTblLBox.IsVisible() && !aParaLBox.IsVisible())
        {
			long nY(LogicToPixel(Point(11,14), MAP_APPFONT).X());
			long nX(LogicToPixel(Point(11,14), MAP_APPFONT).Y());
            Point aPos(aBorderWin.GetPosPixel());
			aPos.X() = nX;
            aPos.Y() = nY;
            aBorderWin.SetPosPixel(aPos);
            aPos = pPreviewWin1->GetPosPixel();
            aPos.Y()  = nY;
            pPreviewWin1->SetPosPixel(aPos);
            aBackgroundColorBox.Hide();
        }
    }
}

void SvxBackgroundTabPage::ResetFromWallpaperItem( const SfxItemSet& rSet )
{
	ShowSelector();

	// Zustand des Vorschau-Buttons durch UserData persistent
	String aUserData = GetUserData();
	aBtnPreview.Check( aUserData.Len() && sal_Unicode('1') == aUserData.GetChar( 0 ) );

	// Input-BrushItem besorgen und auswerten
	const SvxBrushItem* pBgdAttr = NULL;
	USHORT nSlot = SID_VIEW_FLD_PIC;
	USHORT nWhich = GetWhich( nSlot );
	SvxBrushItem* pTemp = 0;
	const CntWallpaperItem* pItem = 0;

	if ( rSet.GetItemState( nWhich, FALSE ) >= SFX_ITEM_AVAILABLE )
	{
		pItem = (const CntWallpaperItem*)&rSet.Get( nWhich );
		pTemp = new SvxBrushItem( *pItem, nWhich );
		pBgdAttr = pTemp;
	}

	aBtnTile.Check();

	if ( pBgdAttr )
	{
		FillControls_Impl(*pBgdAttr, aUserData);
		// Auch bei Anzeige der Grafik, soll die Brush erhalten bleiben
		if( aBgdColor != pBgdAttr->GetColor() )
		{
			aBgdColor = pBgdAttr->GetColor();
			USHORT nCol = GetItemId_Impl( aBackgroundColorSet, aBgdColor );
			aBackgroundColorSet.SelectItem( nCol );
			pPreviewWin1->NotifyChange( aBgdColor );
		}
	}
	else
	{
		aLbSelect.SelectEntryPos( 0 );
		ShowColorUI_Impl();

		const SfxPoolItem* pOld = GetOldItem( rSet, SID_VIEW_FLD_PIC );
		if ( pOld )
			aBgdColor = Color( ((CntWallpaperItem*)pOld)->GetColor() );
	}

	// We now have always a link to the background
	bLinkOnly = TRUE;
	aBtnLink.Check( TRUE );
	aBtnLink.Show( FALSE );
//	if( !pItem || !pItem->GetWallpaper(FALSE).IsBitmap() )
//		aBtnLink.Check();

	delete pTemp;
}



//------------------------------------------------------------------------

void SvxBackgroundTabPage::FillUserData()

/*	[Beschreibung]

	Beim Destruieren einer SfxTabPage wird diese virtuelle Methode gerufen,
	damit die TabPage interne Informationen sichern kann.

	In diesem Fall wird der Zustand des Vorschau-Buttons gesichert.
*/

{
	SetUserData( String( aBtnPreview.IsChecked() ? sal_Unicode('1') : sal_Unicode('0') ) );
}

//------------------------------------------------------------------------

BOOL SvxBackgroundTabPage::FillItemSet( SfxItemSet& rCoreSet )

/*	[Beschreibung]

*/

{
	if ( pPageImpl->pLoadTimer && pPageImpl->pLoadTimer->IsActive() )
	{
		pPageImpl->pLoadTimer->Stop();
		LoadTimerHdl_Impl( pPageImpl->pLoadTimer );
	}
// os: So ein Unsinn! Irgendwo laesst sich so ein Item immer finden,
//		es muss aber im rSet vorhanden sein!

//	const SfxPoolItem* pX = GetOldItem( rCoreSet, SID_VIEW_FLD_PIC );
//	if( pX && pX->ISA(SfxWallpaperItem))
	if(SFX_ITEM_AVAILABLE <= rCoreSet.GetItemState(GetWhich(SID_VIEW_FLD_PIC), FALSE))
		return FillItemSetWithWallpaperItem( rCoreSet, SID_VIEW_FLD_PIC );

	BOOL bModified = FALSE;
	USHORT nSlot = SID_ATTR_BRUSH;

	if ( aTblLBox.IsVisible() )
	{
		switch( aTblLBox.GetSelectEntryPos() )
		{
			case TBL_DEST_CELL:
				nSlot = SID_ATTR_BRUSH;
			break;
			case TBL_DEST_ROW:
				nSlot = SID_ATTR_BRUSH_ROW;
			break;
			case TBL_DEST_TBL:
				nSlot = SID_ATTR_BRUSH_TABLE;
			break;
		}
	}
	else if(aParaLBox.GetData() == &aParaLBox)
	{
		switch(aParaLBox.GetSelectEntryPos())
		{
			case PARA_DEST_PARA:
				nSlot = SID_ATTR_BRUSH;
			break;
			case PARA_DEST_CHAR:
				nSlot = SID_ATTR_BRUSH_CHAR;
			break;
		}
	}
	USHORT nWhich = GetWhich( nSlot );

	const SfxPoolItem* pOld = GetOldItem( rCoreSet, nSlot );
	SfxItemState eOldItemState = rCoreSet.GetItemState(nSlot, FALSE);
	const SfxItemSet& rOldSet = GetItemSet();

    BOOL bGraphTransparencyChanged = bGraphTransparency && (aGraphTransMF.GetText() != aGraphTransMF.GetSavedValue());
    if ( pOld )
	{
		const SvxBrushItem& rOldItem	= (const SvxBrushItem&)*pOld;
		SvxGraphicPosition	eOldPos		= rOldItem.GetGraphicPos();
		const BOOL 			bIsBrush	= ( 0 == aLbSelect.GetSelectEntryPos() );

        // transparency has to be set if enabled, the color not already set to "No fill" and
        if( bColTransparency &&
            aBgdColor.GetTransparency() < 0xff)
        {
            aBgdColor.SetTransparency(lcl_PercentToTransparency(static_cast<long>(aColTransMF.GetValue())));
        }
        if (   ( (GPOS_NONE == eOldPos) && bIsBrush  )
			|| ( (GPOS_NONE != eOldPos) && !bIsBrush ) ) // Brush <-> Bitmap gewechselt?
		{
			// Hintergrund-Art wurde nicht gewechselt:

			if ( (GPOS_NONE == eOldPos) || !aLbSelect.IsVisible() )
			{
				// Brush-Behandlung:
				if ( rOldItem.GetColor() != aBgdColor ||
						(SFX_ITEM_AVAILABLE >= eOldItemState && !aBackgroundColorSet.IsNoSelection()))
				{
					bModified = TRUE;
					rCoreSet.Put( SvxBrushItem( aBgdColor, nWhich ) );
				}
				else if ( SFX_ITEM_DEFAULT == rOldSet.GetItemState( nWhich, FALSE ) )
					rCoreSet.ClearItem( nWhich );
			}
			else
			{
				// Bitmap-Behandlung:

				SvxGraphicPosition	eNewPos  = GetGraphicPosition_Impl();
				const BOOL			bIsLink  = aBtnLink.IsChecked();
				const BOOL			bWasLink = (NULL != rOldItem.GetGraphicLink() );


				if ( !bIsLink && !bIsGraphicValid )
					bIsGraphicValid = LoadLinkedGraphic_Impl();

                if (    bGraphTransparencyChanged ||
                        eNewPos != eOldPos
					|| bIsLink != bWasLink
					|| ( bWasLink  &&    *rOldItem.GetGraphicLink()
									  != aBgdGraphicPath )
					|| ( !bWasLink &&    rOldItem.GetGraphic()->GetBitmap()
									  != aBgdGraphic.GetBitmap() )
				   )
				{
					bModified = TRUE;

                    SvxBrushItem aTmpBrush(nWhich);
                    if ( bIsLink )
					{
                        aTmpBrush = SvxBrushItem( aBgdGraphicPath,
                                                aBgdGraphicFilter,
                                                eNewPos,
                                                nWhich );
					}
					else
                        aTmpBrush = SvxBrushItem( aBgdGraphic,
                                        eNewPos,
                                        nWhich );
                    lcl_SetTransparency(aTmpBrush, static_cast<long>(aGraphTransMF.GetValue()));

                    rCoreSet.Put(aTmpBrush);
				}
				else if ( SFX_ITEM_DEFAULT == rOldSet.GetItemState( nWhich, FALSE ) )
					rCoreSet.ClearItem( nWhich );
			}
		}
		else // Brush <-> Bitmap gewechselt!
		{
			if ( bIsBrush )
				rCoreSet.Put( SvxBrushItem( aBgdColor, nWhich ) );
			else
			{
                SvxBrushItem* pTmpBrush = 0;
                if ( aBtnLink.IsChecked() )
				{
                    pTmpBrush = new SvxBrushItem( aBgdGraphicPath,
												aBgdGraphicFilter,
												GetGraphicPosition_Impl(),
                                                nWhich );
				}
				else
				{
					if ( !bIsGraphicValid )
						bIsGraphicValid = LoadLinkedGraphic_Impl();

					if ( bIsGraphicValid )
                        pTmpBrush = new SvxBrushItem( aBgdGraphic,
													GetGraphicPosition_Impl(),
                                                    nWhich );
				}
                if(pTmpBrush)
                {
                    lcl_SetTransparency(*pTmpBrush, static_cast<long>(aGraphTransMF.GetValue()));
                    rCoreSet.Put(*pTmpBrush);
                    delete pTmpBrush;
                }
			}
			bModified = ( bIsBrush || aBtnLink.IsChecked() || bIsGraphicValid );
		}
	}
	else if ( SID_ATTR_BRUSH_CHAR == nSlot && aBgdColor != Color( COL_WHITE ) )
	{
		rCoreSet.Put( SvxBrushItem( aBgdColor, nWhich ) );
		bModified = TRUE;
	}

	if( aTblLBox.IsVisible() )
	{
		// Der aktuelle Zustand wurde bereits geputtet
        if( nSlot != SID_ATTR_BRUSH && pTableBck_Impl->pCellBrush)
		{
			const SfxPoolItem* pOldCell =
				GetOldItem( rCoreSet, SID_ATTR_BRUSH );

            if ( *pTableBck_Impl->pCellBrush != *pOldCell )
			{
				rCoreSet.Put( *pTableBck_Impl->pCellBrush );
				bModified |= TRUE;
			}
		}

        if( nSlot != SID_ATTR_BRUSH_ROW && pTableBck_Impl->pRowBrush)
		{
			const SfxPoolItem* pOldRow =
				GetOldItem( rCoreSet, SID_ATTR_BRUSH_ROW );

            if ( *pTableBck_Impl->pRowBrush != *pOldRow )
			{
				rCoreSet.Put( *pTableBck_Impl->pRowBrush );
				bModified |= TRUE;
			}
		}

        if( nSlot != SID_ATTR_BRUSH_TABLE && pTableBck_Impl->pTableBrush)
		{
			const SfxPoolItem* pOldTable =
				GetOldItem( rCoreSet, SID_ATTR_BRUSH_TABLE );

			if ( *pTableBck_Impl->pTableBrush != *pOldTable )
			{
				rCoreSet.Put( *pTableBck_Impl->pTableBrush );
				bModified |= TRUE;
			}
		}

		if( aTblLBox.GetSavedValue() != aTblLBox.GetSelectEntryPos() )
		{
			rCoreSet.Put( SfxUInt16Item( SID_BACKGRND_DESTINATION,
										 aTblLBox.GetSelectEntryPos() ) );
			bModified |= TRUE;
		}
	}
	else if(aParaLBox.GetData() == &aParaLBox)
	{
		// Der aktuelle Zustand wurde bereits geputtet
		if( nSlot != SID_ATTR_BRUSH && aParaLBox.IsVisible()) // nicht im Suchen-Format-Dialog
		{
			const SfxPoolItem* pOldPara =
				GetOldItem( rCoreSet, SID_ATTR_BRUSH );

			if ( *pParaBck_Impl->pParaBrush != *pOldPara )
			{
				rCoreSet.Put( *pParaBck_Impl->pParaBrush );
				bModified |= TRUE;
			}
		}

		if( nSlot != SID_ATTR_BRUSH_CHAR )
		{
			const SfxPoolItem* pOldChar =
				GetOldItem( rCoreSet, SID_ATTR_BRUSH_CHAR );
            DBG_ASSERT(pParaBck_Impl, "pParaBck_Impl == NULL ?");
            if ( pOldChar &&
                    //#111173#  crash report shows that pParaBck_Impl can be NULL, the cause is unknown
                    pParaBck_Impl &&
                        (*pParaBck_Impl->pCharBrush != *pOldChar ||
                *pParaBck_Impl->pCharBrush != SvxBrushItem(SID_ATTR_BRUSH_CHAR)))
			{
				rCoreSet.Put( *pParaBck_Impl->pCharBrush );
				bModified |= TRUE;
			}
		}

		if( aParaLBox.GetSavedValue() != aParaLBox.GetSelectEntryPos() )
		{
			rCoreSet.Put( SfxUInt16Item( SID_BACKGRND_DESTINATION,
										 aParaLBox.GetSelectEntryPos() ) );
			bModified |= TRUE;
		}
	}
	return bModified;
}

BOOL SvxBackgroundTabPage::FillItemSetWithWallpaperItem( SfxItemSet& rCoreSet, USHORT nSlot)
{
	USHORT nWhich = GetWhich( nSlot );
	const SfxPoolItem* pOld = GetOldItem( rCoreSet, nSlot );
	const SfxItemSet& rOldSet = GetItemSet();
	DBG_ASSERT(pOld,"FillItemSetWithWallpaperItem: Item not found");

	SvxBrushItem 		rOldItem( (const CntWallpaperItem&)*pOld, nWhich );
	SvxGraphicPosition	eOldPos		= rOldItem.GetGraphicPos();
	const BOOL 			bIsBrush	= ( 0 == aLbSelect.GetSelectEntryPos() );
	BOOL				bModified = FALSE;

	if (   ( (GPOS_NONE == eOldPos) && bIsBrush  )
		|| ( (GPOS_NONE != eOldPos) && !bIsBrush ) ) // Brush <-> Bitmap gewechselt?
	{
		// Hintergrund-Art wurde nicht gewechselt:

		if ( (GPOS_NONE == eOldPos) || !aLbSelect.IsVisible() )
		{
			// Brush-Behandlung:
			if ( rOldItem.GetColor() != aBgdColor )
			{
				bModified = TRUE;
				CntWallpaperItem aItem( nWhich );
				aItem.SetColor( aBgdColor );
				rCoreSet.Put( aItem );
			}
			else if ( SFX_ITEM_DEFAULT == rOldSet.GetItemState( nWhich, FALSE ) )
				rCoreSet.ClearItem( nWhich );
		}
		else
		{
			// Bitmap-Behandlung:
			SvxGraphicPosition	eNewPos  = GetGraphicPosition_Impl();

			int bBitmapChanged = ( ( eNewPos != eOldPos ) ||
								   ( *rOldItem.GetGraphicLink() != aBgdGraphicPath ) );
			int bBrushChanged = ( rOldItem.GetColor() != aBgdColor );
			if( bBitmapChanged || bBrushChanged )
			{
				bModified = TRUE;

				CntWallpaperItem aItem( nWhich );
				WallpaperStyle eWallStyle = SvxBrushItem::GraphicPos2WallpaperStyle(eNewPos);
				aItem.SetStyle( sal::static_int_cast< USHORT >( eWallStyle ) );
				aItem.SetColor( aBgdColor );
                aItem.SetBitmapURL( aBgdGraphicPath );
				rCoreSet.Put( aItem );
			}
			else if ( SFX_ITEM_DEFAULT == rOldSet.GetItemState( nWhich, FALSE ) )
				rCoreSet.ClearItem( nWhich );
		}
	}
	else // Brush <-> Bitmap gewechselt!
	{
		CntWallpaperItem aItem( nWhich );
		if ( bIsBrush )
		{
			aItem.SetColor( aBgdColor );
			rCoreSet.Put( aItem );
		}
		else
		{
			WallpaperStyle eWallStyle =
				SvxBrushItem::GraphicPos2WallpaperStyle( GetGraphicPosition_Impl() );
			aItem.SetStyle( sal::static_int_cast< USHORT >( eWallStyle ) );
			aItem.SetColor( aBgdColor );
            aItem.SetBitmapURL( aBgdGraphicPath );
			rCoreSet.Put( aItem );
		}

		bModified = TRUE;
	}
	return bModified;
}

//-----------------------------------------------------------------------

int SvxBackgroundTabPage::DeactivatePage( SfxItemSet* _pSet )

/*	[Beschreibung]

	virtuelle Methode, wird beim Deaktivieren gerufen
*/

{
	if ( pPageImpl->bIsImportDlgInExecute )
		return KEEP_PAGE;

    if ( _pSet )
        FillItemSet( *_pSet );

    return LEAVE_PAGE;
}

//-----------------------------------------------------------------------

void SvxBackgroundTabPage::PointChanged( Window* , RECT_POINT  )

/*	[Beschreibung]

*/

{
	// muss implementiert werden, damit Position-Control funktioniert
}

//-----------------------------------------------------------------------

void SvxBackgroundTabPage::ShowSelector()

/*	[Beschreibung]

*/

{
	if( bAllowShowSelector)
	{
		aSelectTxt.Show();
		aLbSelect.Show();
		aLbSelect.SetSelectHdl( HDL(SelectHdl_Impl) );
		aBtnLink.SetClickHdl( HDL(FileClickHdl_Impl) );
		aBtnPreview.SetClickHdl( HDL(FileClickHdl_Impl) );
		aBtnBrowse.SetClickHdl( HDL(BrowseHdl_Impl) );
		aBtnArea.SetClickHdl( HDL(RadioClickHdl_Impl) );
		aBtnTile.SetClickHdl( HDL(RadioClickHdl_Impl) );
		aBtnPosition.SetClickHdl( HDL(RadioClickHdl_Impl) );

		// Verz"ogertes Laden "uber Timer (wg. UI-Update)
		pPageImpl->pLoadTimer = new Timer;
		pPageImpl->pLoadTimer->SetTimeout( 500 ); // 500ms verz"ogern
		pPageImpl->pLoadTimer->SetTimeoutHdl(
			LINK( this, SvxBackgroundTabPage, LoadTimerHdl_Impl ) );

		bAllowShowSelector = FALSE;

		if(nHtmlMode & HTMLMODE_ON)
		{
			if(!(nHtmlMode & HTMLMODE_GRAPH_POS))
				aBtnPosition.Enable(FALSE);
			aBtnArea.Enable(FALSE);
		}
	}
}

//------------------------------------------------------------------------


void SvxBackgroundTabPage::RaiseLoadError_Impl()

/*	[Beschreibung]

*/

{
	SfxErrorContext aContext( ERRCTX_SVX_BACKGROUND,
							  String(),
							  this,
							  RID_SVXERRCTX,
							  &DIALOG_MGR() );

	ErrorHandler::HandleError(
		*new StringErrorInfo( ERRCODE_SVX_GRAPHIC_NOTREADABLE,
							  aBgdGraphicPath ) );
}

//------------------------------------------------------------------------

BOOL SvxBackgroundTabPage::LoadLinkedGraphic_Impl()

/*	[Beschreibung]

*/

{
	BOOL bResult = ( aBgdGraphicPath.Len() > 0 ) &&
				   ( GRFILTER_OK == LoadGraphic( aBgdGraphicPath,
												 aBgdGraphicFilter,
												 aBgdGraphic ) );
	return bResult;
}

//------------------------------------------------------------------------


void SvxBackgroundTabPage::FillColorValueSets_Impl()

/*	[Beschreibung]

	F"ullen des Farb-Sets
*/

{
	SfxObjectShell* pDocSh = SfxObjectShell::Current();
	const SfxPoolItem* pItem = NULL;
	XColorTable* pColorTable = NULL;
	const Size aSize15x15 = Size( 15, 15 );
	FASTBOOL bOwn = FALSE;

	if ( pDocSh && ( 0 != ( pItem = pDocSh->GetItem( SID_COLOR_TABLE ) ) ) )
		pColorTable = ( (SvxColorTableItem*)pItem )->GetColorTable();

	if ( !pColorTable )
	{
		bOwn = TRUE;
		pColorTable = new XColorTable( SvtPathOptions().GetPalettePath() );
	}

	if ( pColorTable )
	{
		short i	= 0;
		long nCount	= pColorTable->Count();
		XColorEntry* pEntry	= NULL;
		Color aColWhite( COL_WHITE );
		String aStrWhite( SVX_RES( RID_SVXITEMS_COLOR_WHITE ) );
		WinBits nBits = ( aBackgroundColorSet.GetStyle() | WB_ITEMBORDER | WB_NAMEFIELD | WB_NONEFIELD );
		aBackgroundColorSet.SetText( String( SVX_RES( RID_SVXSTR_TRANSPARENT ) ) );
		aBackgroundColorSet.SetStyle( nBits );
		for ( i = 0; i < nCount; i++ )
		{
            pEntry = pColorTable->GetColor(i);
			aBackgroundColorSet.InsertItem(	i + 1, pEntry->GetColor(), pEntry->GetName() );
		}

		while ( i < 80 )
		{
			aBackgroundColorSet.InsertItem( i + 1, aColWhite, aStrWhite );
			i++;
		}

		if ( nCount > 80 )
		{
			aBackgroundColorSet.SetStyle( nBits | WB_VSCROLL );
		}
	}

	if ( bOwn )
		delete pColorTable;

	aBackgroundColorSet.SetColCount( 10 );
	aBackgroundColorSet.SetLineCount( 10 );
	aBackgroundColorSet.CalcWindowSizePixel( aSize15x15 );

}

//------------------------------------------------------------------------



//------------------------------------------------------------------------

void SvxBackgroundTabPage::ShowColorUI_Impl()

/*	[Beschreibung]

	Die Controls f"ur das Einstellen der Grafik ausblenden und die
	Controls f"ur die Farbeinstellung einblenden.
*/

{
	if( !aBackgroundColorSet.IsVisible() )
	{
		aBackgroundColorSet.Show();
		aBackgroundColorBox.Show();
		aBorderWin.Show();
		pPreviewWin1->Show();
		aBtnBrowse.Hide();
		aFtFile.Hide();
		aBtnLink.Hide();
		aBtnPreview.Hide();
		aGbFile.Hide();
		aBtnPosition.Hide();
		aBtnArea.Hide();
		aBtnTile.Hide();
		aWndPosition.Hide();
		aGbPosition.Hide();
		pPreviewWin2->Hide();
        aGraphTransFL.Show(FALSE);
        aGraphTransMF.Show(FALSE);
        if(bColTransparency)
		{
			aColTransFT.Show();
			aColTransMF.Show();
		}
    }
}

//------------------------------------------------------------------------

void SvxBackgroundTabPage::ShowBitmapUI_Impl()

/*	[Beschreibung]

	Die Controls f"ur die Farbeinstellung ausblenden und die
	Controls f"ur das Einstellen der Grafik einblenden.
*/

{
	if ( aLbSelect.IsVisible() &&
		 (
		aBackgroundColorSet.IsVisible()
		 || !aBtnBrowse.IsVisible() ) )
	{
		aBackgroundColorSet.Hide();
		aBackgroundColorBox.Hide();
		aBorderWin.Hide();
		pPreviewWin1->Hide();
		aBtnBrowse.Show();
		aFtFile.Show();

		if ( !bLinkOnly && ! nHtmlMode & HTMLMODE_ON )
			aBtnLink.Show();
		aBtnPreview.Show();
		aGbFile.Show();
		aBtnPosition.Show();
		aBtnArea.Show();
		aBtnTile.Show();
		aWndPosition.Show();
		aGbPosition.Show();
		pPreviewWin2->Show();
        if(bGraphTransparency)
		{
			aGraphTransFL.Show();
			aGraphTransMF.Show();
		}
        aColTransFT.Show(FALSE);
        aColTransMF.Show(FALSE);
    }
}

//------------------------------------------------------------------------

void SvxBackgroundTabPage::SetGraphicPosition_Impl( SvxGraphicPosition ePos )

/*	[Beschreibung]

	Die Controls f"ur die Grafikposition einstellen.
*/

{
	switch ( ePos )
	{
		case GPOS_AREA:
		{
			aBtnArea.Check();
			aWndPosition.Disable();
		}
		break;

		case GPOS_TILED:
		{
			aBtnTile.Check();
			aWndPosition.Disable();
		}
		break;

		default:
		{
			aBtnPosition.Check();
			aWndPosition.Enable();
			RECT_POINT eNewPos = RP_MM;

			switch ( ePos )
			{
				case GPOS_MM:	break;
				case GPOS_LT:	eNewPos = RP_LT; break;
				case GPOS_MT:	eNewPos = RP_MT; break;
				case GPOS_RT:	eNewPos = RP_RT; break;
				case GPOS_LM:	eNewPos = RP_LM; break;
				case GPOS_RM:	eNewPos = RP_RM; break;
				case GPOS_LB:	eNewPos = RP_LB; break;
				case GPOS_MB:	eNewPos = RP_MB; break;
				case GPOS_RB:	eNewPos = RP_RB; break;
                default: ;//prevent warning
			}
			aWndPosition.SetActualRP( eNewPos );
		}
		break;
	}
	aWndPosition.Invalidate();
}

//------------------------------------------------------------------------

SvxGraphicPosition SvxBackgroundTabPage::GetGraphicPosition_Impl()

/*	[Beschreibung]

	Die Position der Grafik zur"uckgeben.
*/

{
	if ( aBtnTile.IsChecked() )
		return GPOS_TILED;
	else if ( aBtnArea.IsChecked() )
		return GPOS_AREA;
	else
	{
		switch ( aWndPosition.GetActualRP() )
		{
			case RP_LT:	return GPOS_LT;
			case RP_MT:	return GPOS_MT;
			case RP_RT:	return GPOS_RT;
			case RP_LM:	return GPOS_LM;
			case RP_MM:	return GPOS_MM;
			case RP_RM:	return GPOS_RM;
			case RP_LB:	return GPOS_LB;
			case RP_MB:	return GPOS_MB;
			case RP_RB:	return GPOS_RB;
		}
	}
	return GPOS_MM;
}

//-----------------------------------------------------------------------
// Handler
//-----------------------------------------------------------------------

IMPL_LINK( SvxBackgroundTabPage, BackgroundColorHdl_Impl, ValueSet*, EMPTYARG )
/*
    Handler, called when color selection is changed
*/
{
	USHORT nItemId = aBackgroundColorSet.GetSelectItemId();
	Color aColor = nItemId ? ( aBackgroundColorSet.GetItemColor( nItemId ) ) : Color( COL_TRANSPARENT );
	aBgdColor = aColor;
	pPreviewWin1->NotifyChange( aBgdColor );
    BOOL bEnableTransp = aBgdColor.GetTransparency() < 0xff;
    aColTransFT.Enable(bEnableTransp);
    aColTransMF.Enable(bEnableTransp);
    return 0;
}

//------------------------------------------------------------------------

IMPL_LINK( SvxBackgroundTabPage, SelectHdl_Impl, ListBox*, EMPTYARG )

/*	[Beschreibung]

*/

{
	if ( 0 == aLbSelect.GetSelectEntryPos() )
	{
		ShowColorUI_Impl();
		aParaLBox.Enable(); // Zeichenhintergrund kann keine Bitmap sein
	}
	else
	{
		ShowBitmapUI_Impl();
		aParaLBox.Enable(FALSE);// Zeichenhintergrund kann keine Bitmap sein
	}
	return 0;
}

//------------------------------------------------------------------------

IMPL_LINK( SvxBackgroundTabPage, FileClickHdl_Impl, CheckBox*, pBox )

/*	[Beschreibung]

*/

{
	if ( &aBtnLink == pBox )
	{
		if ( aBtnLink.IsChecked() )
		{
            INetURLObject aObj( aBgdGraphicPath );
			String aFilePath;
			if ( aObj.GetProtocol() == INET_PROT_FILE )
				aFilePath = aObj.getFSysPath( INetURLObject::FSYS_DETECT );
			else
				aFilePath = aBgdGraphicPath;
			aFtFile.SetText( aFilePath );
		}
		else
			aFtFile.SetText( aStrUnlinked );
	}
	else if ( &aBtnPreview == pBox )
	{
		if ( aBtnPreview.IsChecked() )
		{
			if ( !bIsGraphicValid )
				bIsGraphicValid = LoadLinkedGraphic_Impl();

			if ( bIsGraphicValid )
			{
				Bitmap aBmp = aBgdGraphic.GetBitmap();
				pPreviewWin2->NotifyChange( &aBmp );
			}
			else
			{
				if ( aBgdGraphicPath.Len() > 0 ) // nur bei gelinkter Grafik
					RaiseLoadError_Impl();		 // ein Fehler
				pPreviewWin2->NotifyChange( NULL );
			}
		}
		else
			pPreviewWin2->NotifyChange( NULL );
	}
	return 0;
}

//------------------------------------------------------------------------

IMPL_LINK( SvxBackgroundTabPage, RadioClickHdl_Impl, RadioButton*, pBtn )

/*	[Beschreibung]

*/

{
	if ( pBtn == &aBtnPosition )
	{
		if ( !aWndPosition.IsEnabled() )
		{
			aWndPosition.Enable();
			aWndPosition.Invalidate();
		}
	}
	else if ( aWndPosition.IsEnabled() )
	{
		aWndPosition.Disable();
		aWndPosition.Invalidate();
	}
	return 0;
}

//------------------------------------------------------------------------

IMPL_LINK( SvxBackgroundTabPage, BrowseHdl_Impl, PushButton* , EMPTYARG )

/*	[Beschreibung]

	Handler, gerufen durch das Dr"ucken des Durchsuchen-Buttons.
	Grafik/Einf"ugen-Dialog erzeugen, Pfad setzen und starten.
*/

{
	if ( pPageImpl->pLoadTimer->IsActive() )
		return 0;
	BOOL bHtml = 0 != ( nHtmlMode & HTMLMODE_ON );

	pImportDlg = new SvxOpenGraphicDialog( aStrBrowse );
	if ( bHtml || bLinkOnly )
		pImportDlg->EnableLink(sal_False);
	pImportDlg->SetPath( aBgdGraphicPath, aBtnLink.IsChecked() );

	pPageImpl->bIsImportDlgInExecute = TRUE;
	short nErr = pImportDlg->Execute();
	pPageImpl->bIsImportDlgInExecute = FALSE;

	if( !nErr )
	{
		if ( bHtml )
			aBtnLink.Check();
		// wenn Verkn"upfen nicht gecheckt ist und die Vorschau auch nicht,
		// dann die Vorschau aktivieren, damit der Anwender sieht,
		// welche Grafik er ausgew"ahlt hat
		if ( !aBtnLink.IsChecked() && !aBtnPreview.IsChecked() )
			aBtnPreview.Check( TRUE );
		// timer-verz"ogertes Laden der Grafik
		pPageImpl->pLoadTimer->Start();
	}
	else
		DELETEZ( pImportDlg );
	return 0;
}

//-----------------------------------------------------------------------

IMPL_LINK( SvxBackgroundTabPage, LoadTimerHdl_Impl, Timer* , pTimer )

/*	[Beschreibung]

	Verz"ogertes Laden der Grafik.
	Grafik wird nur dann geladen, wenn sie unterschiedlich zur
	aktuellen Grafik ist.
*/

{
	if ( pTimer == pPageImpl->pLoadTimer )
	{
		pPageImpl->pLoadTimer->Stop();

		if ( pImportDlg )
		{
            INetURLObject aOld( aBgdGraphicPath );
			INetURLObject aNew( pImportDlg->GetPath() );
			if ( !aBgdGraphicPath.Len() || aNew != aOld )
			{
				// neue Datei gew"ahlt
				aBgdGraphicPath   = pImportDlg->GetPath();
				aBgdGraphicFilter =	pImportDlg->GetCurrentFilter();
				BOOL bLink = ( nHtmlMode & HTMLMODE_ON ) || bLinkOnly ? TRUE : pImportDlg->IsAsLink();
				aBtnLink.Check( bLink );
				aBtnLink.Enable();

				if ( aBtnPreview.IsChecked() )
				{
					if( !pImportDlg->GetGraphic(aBgdGraphic) )
					{
						bIsGraphicValid = TRUE;
					}
					else
					{
						aBgdGraphicFilter.Erase();
						aBgdGraphicPath.Erase();
						bIsGraphicValid = FALSE;
					}
				}
				else
					bIsGraphicValid = FALSE; // Grafik erst beim Preview-Click laden

				if ( aBtnPreview.IsChecked() && bIsGraphicValid )
				{
					Bitmap aBmp = aBgdGraphic.GetBitmap();
					pPreviewWin2->NotifyChange( &aBmp );
				}
				else
					pPreviewWin2->NotifyChange( NULL );
			}

			FileClickHdl_Impl( &aBtnLink );
			DELETEZ( pImportDlg );
		}
	}
	return 0;
}

//-----------------------------------------------------------------------

void SvxBackgroundTabPage::ShowTblControl()

/*	[Beschreibung]

*/

{
	aTblLBox			.SetSelectHdl( HDL(TblDestinationHdl_Impl) );
	aTblLBox			.SelectEntryPos(0);
	aTblDesc.Show();
	aTblLBox.Show();
}

//-----------------------------------------------------------------------

void SvxBackgroundTabPage::ShowParaControl(BOOL bCharOnly)
{
	aParaLBox.SetSelectHdl(HDL(ParaDestinationHdl_Impl));
	aParaLBox.SelectEntryPos(0);
	if(!bCharOnly)
	{
		aTblDesc.Show();
		aParaLBox.Show();
	}
	aParaLBox.SetData(&aParaLBox); // hier erkennt man, dass dieser Mode eingeschaltet ist
}
//-----------------------------------------------------------------------

IMPL_LINK( SvxBackgroundTabPage, TblDestinationHdl_Impl, ListBox*, pBox )

/*	[Beschreibung]

*/

{
	USHORT nSelPos = pBox->GetSelectEntryPos();
	if( pTableBck_Impl && pTableBck_Impl->nActPos != nSelPos)
	{
		SvxBrushItem** pActItem = new (SvxBrushItem*);
        USHORT nWhich = 0;
        switch(pTableBck_Impl->nActPos)
		{
			case TBL_DEST_CELL:
                *pActItem = pTableBck_Impl->pCellBrush;
                nWhich = pTableBck_Impl->nCellWhich;
			break;
			case TBL_DEST_ROW:
                *pActItem = pTableBck_Impl->pRowBrush;
                nWhich = pTableBck_Impl->nRowWhich;
			break;
			case TBL_DEST_TBL:
                *pActItem = pTableBck_Impl->pTableBrush;
                nWhich = pTableBck_Impl->nTableWhich;
			break;
		}
		pTableBck_Impl->nActPos = nSelPos;
        if(!*pActItem)
            *pActItem = new SvxBrushItem(nWhich);
        if(0 == aLbSelect.GetSelectEntryPos())  // Brush ausgewaehlt
		{
            **pActItem = SvxBrushItem( aBgdColor, nWhich );
		}
		else
		{
				SvxGraphicPosition	eNewPos  = GetGraphicPosition_Impl();
				const BOOL			bIsLink  = aBtnLink.IsChecked();

				if ( !bIsLink && !bIsGraphicValid )
					bIsGraphicValid = LoadLinkedGraphic_Impl();

				if ( bIsLink )
					**pActItem = SvxBrushItem( aBgdGraphicPath,
												aBgdGraphicFilter,
												eNewPos,
												(*pActItem)->Which() );
				else
					**pActItem = SvxBrushItem( aBgdGraphic,
												eNewPos,
												(*pActItem)->Which() );
		}
		switch(nSelPos)
		{
			case TBL_DEST_CELL:
				*pActItem = pTableBck_Impl->pCellBrush;
				aLbSelect.Enable();
                nWhich = pTableBck_Impl->nCellWhich;
            break;
			case TBL_DEST_ROW:
			{
				if((nHtmlMode & HTMLMODE_ON) && !(nHtmlMode & HTMLMODE_SOME_STYLES))
					aLbSelect.Disable();
				*pActItem = pTableBck_Impl->pRowBrush;
                nWhich = pTableBck_Impl->nRowWhich;
            }
			break;
			case TBL_DEST_TBL:
				*pActItem = pTableBck_Impl->pTableBrush;
				aLbSelect.Enable();
                nWhich = pTableBck_Impl->nTableWhich;
            break;
		}
		String aUserData = GetUserData();
        if(!*pActItem)
            *pActItem = new SvxBrushItem(nWhich);
        FillControls_Impl(**pActItem, aUserData);
		delete pActItem;
	}
	return 0;
}

//-----------------------------------------------------------------------

IMPL_LINK( SvxBackgroundTabPage, ParaDestinationHdl_Impl, ListBox*, pBox )
{
	USHORT nSelPos = pBox->GetSelectEntryPos();
	if( pParaBck_Impl && pParaBck_Impl->nActPos != nSelPos)
	{
		SvxBrushItem** pActItem = new (SvxBrushItem*);
		switch(pParaBck_Impl->nActPos)
		{
			case PARA_DEST_PARA:
				*pActItem = pParaBck_Impl->pParaBrush;
			break;
			case PARA_DEST_CHAR:
				*pActItem = pParaBck_Impl->pCharBrush;
			break;
		}
		pParaBck_Impl->nActPos = nSelPos;
		if(0 == aLbSelect.GetSelectEntryPos())  // Brush ausgewaehlt
		{
			USHORT nWhich = (*pActItem)->Which();
            **pActItem = SvxBrushItem( aBgdColor, nWhich );
		}
		else
		{
				SvxGraphicPosition	eNewPos  = GetGraphicPosition_Impl();
				const BOOL			bIsLink  = aBtnLink.IsChecked();

				if ( !bIsLink && !bIsGraphicValid )
					bIsGraphicValid = LoadLinkedGraphic_Impl();

				if ( bIsLink )
					**pActItem = SvxBrushItem( aBgdGraphicPath,
												aBgdGraphicFilter,
												eNewPos,
												(*pActItem)->Which() );
				else
					**pActItem = SvxBrushItem( aBgdGraphic,
												eNewPos,
												(*pActItem)->Which() );
		}
		switch(nSelPos)
		{
			case PARA_DEST_PARA:
				*pActItem = pParaBck_Impl->pParaBrush;
				aLbSelect.Enable();
			break;
			case PARA_DEST_CHAR:
			{
				*pActItem = pParaBck_Impl->pCharBrush;
				aLbSelect.Enable(FALSE);
			}
			break;
		}
		String aUserData = GetUserData();
		FillControls_Impl(**pActItem, aUserData);
		delete pActItem;
	}
	return 0;
}

//-----------------------------------------------------------------------

void SvxBackgroundTabPage::FillControls_Impl( const SvxBrushItem& rBgdAttr,
											  const String& rUserData )

/*	[Beschreibung]

*/

{
	SvxGraphicPosition	ePos = rBgdAttr.GetGraphicPos();
	const Color& rColor = rBgdAttr.GetColor();
    if(bColTransparency)
    {
        aColTransMF.SetValue(lcl_TransparencyToPercent(rColor.GetTransparency()));
        aColTransMF.SaveValue();
		BOOL bEnableTransp = rColor.GetTransparency() < 0xff;
		aColTransFT.Enable(bEnableTransp);
		aColTransMF.Enable(bEnableTransp);
		//the default setting should be "no transparency"
		if(!bEnableTransp)
			aColTransMF.SetValue(0);
    }

    if ( GPOS_NONE == ePos || !aLbSelect.IsVisible() )
	{
		aLbSelect.SelectEntryPos( 0 );
		ShowColorUI_Impl();
		Color aTrColor( COL_TRANSPARENT );
		aBgdColor = rColor;

		USHORT nCol = ( aTrColor != aBgdColor ) ?
			GetItemId_Impl( aBackgroundColorSet, aBgdColor ) : 0;

		if( aTrColor != aBgdColor && nCol == 0)
		{
			aBackgroundColorSet.SetNoSelection();
		}
		else
		{
			aBackgroundColorSet.SelectItem( nCol );
		}

		pPreviewWin1->NotifyChange( aBgdColor );
		if ( aLbSelect.IsVisible() ) // Grafikteil initialisieren
		{
			aBgdGraphicFilter.Erase();
			aBgdGraphicPath.Erase();

			if ( !rUserData.Len() )
				aBtnPreview.Check( FALSE );
			aBtnLink.Check( FALSE );
			aBtnLink.Disable();
			pPreviewWin2->NotifyChange( NULL );
			SetGraphicPosition_Impl( GPOS_TILED );	// Kacheln als Default
		}
	}
	else
	{
		const String*	pStrLink   = rBgdAttr.GetGraphicLink();
		const String*	pStrFilter = rBgdAttr.GetGraphicFilter();

		aLbSelect.SelectEntryPos( 1 );
		ShowBitmapUI_Impl();

		if ( pStrLink )
		{
#ifdef DBG_UTIL
            INetURLObject aObj( *pStrLink );
            DBG_ASSERT( aObj.GetProtocol() != INET_PROT_NOT_VALID, "Invalid URL!" );
#endif
			aBgdGraphicPath = *pStrLink;
			aBtnLink.Check( TRUE );
			aBtnLink.Enable();
		}
		else
		{
			aBgdGraphicPath.Erase();
			aBtnLink.Check( FALSE );
			aBtnLink.Disable();
		}

        if(bGraphTransparency)
        {
            const GraphicObject* pObject = rBgdAttr.GetGraphicObject(SfxObjectShell::Current());
            if(pObject)
                aGraphTransMF.SetValue(lcl_TransparencyToPercent(pObject->GetAttr().GetTransparency()));
            else
                aGraphTransMF.SetValue(0);
            aGraphTransMF.SaveValue();
        }

        FileClickHdl_Impl( &aBtnLink );

		if ( pStrFilter )
			aBgdGraphicFilter = *pStrFilter;
		else
			aBgdGraphicFilter.Erase();

		if ( !pStrLink || aBtnPreview.IsChecked() )
		{
			// Grafik ist im Item vorhanden und muss nicht
			// geladen werden:

			const Graphic* pGraphic =
				rBgdAttr.GetGraphic( SfxObjectShell::Current() );

			if ( !pGraphic && aBtnPreview.IsChecked() )
				bIsGraphicValid = LoadLinkedGraphic_Impl();
			else if ( pGraphic )
			{
				aBgdGraphic = *pGraphic;
				bIsGraphicValid = TRUE;

				if ( !rUserData.Len() )
					aBtnPreview.Check();
			}
			else
			{
				RaiseLoadError_Impl();
				bIsGraphicValid = FALSE;

				if ( !rUserData.Len() )
					aBtnPreview.Check( FALSE );
			}
		}

		if ( aBtnPreview.IsChecked() && bIsGraphicValid )
		{
			Bitmap aBmp = aBgdGraphic.GetBitmap();
			pPreviewWin2->NotifyChange( &aBmp );
		}
		else
			pPreviewWin2->NotifyChange( NULL );

		SetGraphicPosition_Impl( ePos );
	}
}
/* -----------------------------09.08.2002 14:04------------------------------

 ---------------------------------------------------------------------------*/
void SvxBackgroundTabPage::EnableTransparency(BOOL bColor, BOOL bGraphic)
{
    bColTransparency  = bColor;
    bGraphTransparency = bGraphic;
    if(bColor)
    {
        aColTransFT.Show();
        aColTransMF.Show();
    }
    if(bGraphic)
    {
        Size aRectSize(aWndPosition.GetSizePixel());
        Point aRectPos(aWndPosition.GetPosPixel());
        Point aFLPos(aGraphTransFL.GetPosPixel());
        Size aTmp(LogicToPixel(Size(RSC_SP_FLGR_SPACE_Y, RSC_SP_FLGR_SPACE_Y), MAP_APPFONT));
        long nRectHeight = aFLPos.Y() - aRectPos.Y() - aTmp.Height();
        aRectSize.Height() = nRectHeight;
        aWndPosition.SetSizePixel(aRectSize);
        aWndPosition.Invalidate();
    }
}

void SvxBackgroundTabPage::PageCreated (SfxAllItemSet aSet) //add CHINA001
{
	SFX_ITEMSET_ARG (&aSet,pFlagItem,SfxUInt32Item,SID_FLAG_TYPE,sal_False);
	if (pFlagItem)
	{
		UINT32 nFlags=pFlagItem->GetValue();
		if ( ( nFlags & SVX_SHOW_TBLCTL ) == SVX_SHOW_TBLCTL )
			ShowTblControl();
		if ( ( nFlags & SVX_SHOW_PARACTL ) == SVX_SHOW_PARACTL )
			ShowParaControl();
		if ( ( nFlags & SVX_SHOW_SELECTOR ) == SVX_SHOW_SELECTOR )
			ShowSelector();
		if ( ( nFlags & SVX_ENABLE_TRANSPARENCY ) == SVX_ENABLE_TRANSPARENCY )
			EnableTransparency(TRUE, TRUE);
	}
}
