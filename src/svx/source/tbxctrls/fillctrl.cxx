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

// include ---------------------------------------------------------------

#include <string> // HACK: prevent conflict between STLPORT and Workshop headers
#include <sfx2/app.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/viewsh.hxx>
#include <rtl/ustring.hxx>

#include <svx/dialogs.hrc>

#define DELAY_TIMEOUT			300

#define TMP_STR_BEGIN	'['
#define TMP_STR_END		']'

#include "drawitem.hxx"
#include "xattr.hxx"
#include <svx/xtable.hxx>
#include <svx/fillctrl.hxx>
#include <svx/itemwin.hxx>
#include <svx/dialmgr.hxx>
#include "helpid.hrc"
#include "svx/svxdlg.hxx"
#include "svx/svdview.hxx"
#include <vcl/msgbox.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::lang;

SFX_IMPL_TOOLBOX_CONTROL( SvxFillToolBoxControl, XFillStyleItem );

/*************************************************************************
|*
|* SvxFillToolBoxControl
|*
\************************************************************************/

SvxFillToolBoxControl::SvxFillToolBoxControl( USHORT nSlotId, USHORT nId, ToolBox& rTbx ) :
	SfxToolBoxControl( nSlotId, nId, rTbx ),

	pStyleItem		( NULL ),
	pColorItem		( NULL ),
	pGradientItem	( NULL ),
	pHatchItem		( NULL ),
	pBitmapItem		( NULL ),
	pFillControl	( NULL ),
	pFillTypeLB		( NULL ),
	bUpdate			( FALSE ),
    bIgnoreStatusUpdate( FALSE ),
    eLastXFS        ( XFILL_NONE )
{
    addStatusListener( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:FillColor" )));
    addStatusListener( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:FillGradient" )));
    addStatusListener( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:FillHatch" )));
    addStatusListener( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:FillBitmap" )));
    addStatusListener( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:ColorTableState" )));
    addStatusListener( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:GradientListState" )));
    addStatusListener( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:HatchListState" )));
    addStatusListener( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:BitmapListState" )));
}

//========================================================================

SvxFillToolBoxControl::~SvxFillToolBoxControl()
{
	delete pStyleItem;
	delete pColorItem;
	delete pGradientItem;
	delete pHatchItem;
	delete pBitmapItem;
}

//========================================================================

void SvxFillToolBoxControl::StateChanged(

	USHORT nSID, SfxItemState eState, const SfxPoolItem* pState )

{
	FASTBOOL bEnableControls = FALSE;

    if ( bIgnoreStatusUpdate )
        return;

	if( eState == SFX_ITEM_DISABLED )
	{
		if( nSID == SID_ATTR_FILL_STYLE )
		{
			pFillTypeLB->Disable();
			pFillTypeLB->SetNoSelection();
		}
	}
	else
	{
		if ( SFX_ITEM_AVAILABLE == eState )
		{
			if( nSID == SID_ATTR_FILL_STYLE )
			{
				delete pStyleItem;
				pStyleItem = (XFillStyleItem*) pState->Clone();
				pFillTypeLB->Enable();

				eLastXFS = pFillTypeLB->GetSelectEntryPos();
				bUpdate = TRUE;

				XFillStyle eXFS = (XFillStyle)pStyleItem->GetValue();
				pFillTypeLB->SelectEntryPos(
                    sal::static_int_cast< USHORT >( eXFS ) );
			}
			else if( pStyleItem )
			{
				XFillStyle eXFS = (XFillStyle)pStyleItem->GetValue();

				if( nSID == SID_ATTR_FILL_COLOR )
				{
					delete pColorItem;
					pColorItem = (XFillColorItem*) pState->Clone();

					if( eXFS == XFILL_SOLID )
						bEnableControls = TRUE;
				}
				else if( nSID == SID_ATTR_FILL_GRADIENT )
				{
					delete pGradientItem;
					pGradientItem = (XFillGradientItem*) pState->Clone();

					if( eXFS == XFILL_GRADIENT )
						bEnableControls = TRUE;
				}
				else if( nSID == SID_ATTR_FILL_HATCH )
				{
					delete pHatchItem;
					pHatchItem = (XFillHatchItem*) pState->Clone();

					if( eXFS == XFILL_HATCH )
						bEnableControls = TRUE;
				}
				else if( nSID == SID_ATTR_FILL_BITMAP )
				{
					delete pBitmapItem;
					pBitmapItem = (XFillBitmapItem*) pState->Clone();

					if( eXFS == XFILL_BITMAP )
						bEnableControls = TRUE;
				}
			}
			if( bEnableControls )
				bUpdate = TRUE;
		
            Update( pState );
        }
		else
		{
			// leerer oder uneindeutiger Status
			if( nSID == SID_ATTR_FILL_STYLE )
			{
				pFillTypeLB->SetNoSelection();
				bUpdate = FALSE;
			}
		}
	}
}

//========================================================================

void SvxFillToolBoxControl::IgnoreStatusUpdate( sal_Bool bSet )
{
    bIgnoreStatusUpdate = bSet;
}

//========================================================================

void SvxFillToolBoxControl::Update( const SfxPoolItem* pState )
{
}

//========================================================================

Window* SvxFillToolBoxControl::CreateItemWindow( Window *pParent )
{
	if ( GetSlotId() == SID_ATTR_FILL_STYLE )
	{
		pFillControl = new FillControl( pParent );
		// Damit dem FillControl das SvxFillToolBoxControl bekannt ist
		// (und um kompatibel zu bleiben)
		pFillControl->SetData( this );

		pFillTypeLB = (SvxFillTypeBox*)pFillControl->pLbFillType;

		pFillTypeLB->SetUniqueId( HID_FILL_TYPE_LISTBOX );

		return pFillControl;
	}
	return NULL;
}

/*************************************************************************
|*
|* FillControl
|*
\************************************************************************/

FillControl::FillControl( Window* pParent, WinBits nStyle ) :
    Window( pParent, nStyle | WB_DIALOGCONTROL ),
    pLbFillType(new SvxFillTypeBox( this )),
    aLogicalFillSize(40,80),
    aLogicalAttrSize(50,80)
{
    Size aTypeSize(LogicToPixel(aLogicalFillSize, MAP_APPFONT));
    Size aAttrSize(LogicToPixel(aLogicalAttrSize, MAP_APPFONT));
    pLbFillType->SetSizePixel(aTypeSize);
    //to get the base height

    aAttrSize = pLbFillType->GetSizePixel();
    Point aAttrPnt = pLbFillType->GetPosPixel();
	SetSizePixel(
		Size( aAttrPnt.X() + aAttrSize.Width(),
			  aAttrSize.Height() ) );
    aAttrSize = GetSizePixel();

	pLbFillType->SetSelectHdl( LINK( this, FillControl, SelectFillTypeHdl ) );

	aDelayTimer.SetTimeout( DELAY_TIMEOUT );
	aDelayTimer.SetTimeoutHdl( LINK( this, FillControl, DelayHdl ) );
	aDelayTimer.Start();
}

//------------------------------------------------------------------------

FillControl::~FillControl()
{
	delete pLbFillType;
}

//------------------------------------------------------------------------

IMPL_LINK_INLINE_START( FillControl, DelayHdl, Timer *, EMPTYARG )
{
    ( (SvxFillToolBoxControl*)GetData() )->updateStatus( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:FillStyle" )));
	return 0;
}
IMPL_LINK_INLINE_END( FillControl, DelayHdl, Timer *, pTimer )

//------------------------------------------------------------------------

IMPL_LINK( FillControl, SelectFillTypeHdl, ListBox *, pBox )
{
	XFillStyle  eXFS = (XFillStyle)pLbFillType->GetSelectEntryPos();

	// Spaeter sollte eine Optimierung derart erfolgen, dass die
	// Listen, bzw. Tables nur dann geloescht und wieder aufgebaut
	// werden, wenn sich die Listen, bzw. Tables tatsaechlich geaendert
	// haben (in den LBs natuerlich).

	if ( ( pBox && !pBox->IsTravelSelect() ) || !pBox )
	{
		// Damit wir in folgendem Fall einen Status anzeigen koennen:
		// Ein Typ wurde ausgewaehlt aber kein Attribut.
		// Die Selektion hat genau die gleichen Attribute wie die vorherige.
		SfxViewShell* pViewSh = SfxViewShell::Current();
        if ( pViewSh )
        {
        	SdrView* pView = pViewSh->GetDrawView();
            if ( pView )
            {
            	SfxItemSet aAttrSet(pView->GetModel()->GetItemPool());
				if (eXFS == XFILL_NONE)
				{
					aAttrSet.Put(XFillStyleItem(XFILL_NONE));
					pView->SetAttributes (aAttrSet);
				}
				else
				{
				SvxAbstractDialogFactory* pFact = 
				SvxAbstractDialogFactory::Create();
				AbstractSvxAreaTabDialog * pDlg = 
					pFact->CreateSvxAreaTabDialog( NULL, &aAttrSet, 
					pView->GetModel(), RID_SVXDLG_AREA, pView);

				switch( eXFS )
				{
					case XFILL_SOLID:
						pDlg->SetCurPageId(RID_SVXPAGE_COLOR);
					break;
					case XFILL_GRADIENT:
						pDlg->SetCurPageId(RID_SVXPAGE_GRADIENT);
					break;
					case XFILL_HATCH:
						pDlg->SetCurPageId(RID_SVXPAGE_HATCH);
					break;
					case XFILL_BITMAP:
						pDlg->SetCurPageId(RID_SVXPAGE_BITMAP);
					break;
				}

				if ( pDlg->Execute() == RET_OK )
					pView->SetAttributes (*(pDlg->GetOutputItemSet ()));
				delete pDlg;
				}
			}
		}

		if ( pBox )
			pLbFillType->Selected();

		if( eXFS != XFILL_NONE ) // Wurde schon erledigt
		{
			// release focus
            if ( pBox && pLbFillType->IsRelease() )
			{
				SfxViewShell* pViewShell = SfxViewShell::Current();
				if( pViewShell && pViewShell->GetWindow() )
					pViewShell->GetWindow()->GrabFocus();
			}
		}
    }
	return 0;
}

//------------------------------------------------------------------------

void FillControl::Resize()
{
}
/* -----------------------------08.03.2002 15:04------------------------------

 ---------------------------------------------------------------------------*/

void FillControl::DataChanged( const DataChangedEvent& rDCEvt )
{
	if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
         (rDCEvt.GetFlags() & SETTINGS_STYLE) )
    {
        Size aTypeSize(LogicToPixel(aLogicalFillSize, MAP_APPFONT));
        Size aAttrSize(LogicToPixel(aLogicalAttrSize, MAP_APPFONT));
        pLbFillType->SetSizePixel(aTypeSize);
    	SetSizePixel(aTypeSize);
    }
    Window::DataChanged( rDCEvt );
}

