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
#include <string> // HACK: prevent conflict between STLPORT and Workshop headers
#include <tools/ref.hxx>
#include <tools/shl.hxx>
#include <svtools/intitem.hxx>
#include <svtools/eitem.hxx>
#include <svtools/stritem.hxx>
#include <sfx2/dispatch.hxx>
#include <vcl/toolbox.hxx>
#include <vcl/fixed.hxx>
#include "fmitems.hxx"
#include "formtoolbars.hxx"


#include <vcl/sound.hxx>
#include <svx/dialmgr.hxx>
#ifndef _SVX_DIALOGS_HRC
#include <svx/dialogs.hrc>
#endif
#include "tbxctl.hxx"
#include "tbxdraw.hxx"
#include "tbxform.hxx"
#ifndef _SVX_FMRESIDS_HRC
#include "fmresids.hrc"
#endif
#include "fmitems.hxx"
#ifndef _SVX_FMHELP_HRC
#include "fmhelp.hrc"
#endif
#include <sfx2/viewfrm.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/imagemgr.hxx>
#include <com/sun/star/beans/XPropertySet.hpp>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::frame;
using ::com::sun::star::beans::XPropertySet;


//========================================================================
// class SvxFmAbsRecWin
//========================================================================

// -----------------------------------------------------------------------
SvxFmAbsRecWin::SvxFmAbsRecWin( Window* _pParent, SfxToolBoxControl* _pController )
	:NumericField( _pParent, WB_BORDER )
	,m_pController(_pController)
{
	SetMin(1);
	SetFirst(1);
	SetSpinSize(1);
	SetSizePixel( Size(70,19) );

	SetDecimalDigits(0);
	SetStrictFormat(TRUE);
}

// -----------------------------------------------------------------------
SvxFmAbsRecWin::~SvxFmAbsRecWin()
{
}

// -----------------------------------------------------------------------
void SvxFmAbsRecWin::FirePosition( sal_Bool _bForce )
{
	if ( _bForce || ( GetText() != GetSavedValue() ) )
	{
		sal_Int64 nRecord = GetValue();
		if (nRecord < GetMin() || nRecord > GetMax())
		{
			Sound::Beep();
			return;
		}

		SfxInt32Item aPositionParam( FN_PARAM_1, static_cast<INT32>(nRecord) );

        Any a;
        Sequence< PropertyValue > aArgs( 1 );
        aArgs[0].Name = rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Position" ));
        aPositionParam.QueryValue( a );
        aArgs[0].Value = a;
        m_pController->Dispatch( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( ".uno:AbsoluteRecord" )),
                                 aArgs );
        m_pController->updateStatus();

		SaveValue();
	}
}

// -----------------------------------------------------------------------
void SvxFmAbsRecWin::LoseFocus()
{
	FirePosition( sal_False );
}

// -----------------------------------------------------------------------
void SvxFmAbsRecWin::KeyInput( const KeyEvent& rKeyEvent )
{
	if( rKeyEvent.GetKeyCode() == KEY_RETURN && GetText().Len() )
		FirePosition( sal_True );
	else
		NumericField::KeyInput( rKeyEvent );
}

//========================================================================
// class SvxFmTbxCtlConfig
//========================================================================

struct MapSlotToCmd
{
    USHORT      nSlotId;
    const char* pCommand;
};

static MapSlotToCmd SlotToCommands[] =
{
    { SID_FM_PUSHBUTTON,        ".uno:Pushbutton"       },
    { SID_FM_RADIOBUTTON,       ".uno:RadioButton"      },
    { SID_FM_CHECKBOX,          ".uno:CheckBox"         },
    { SID_FM_FIXEDTEXT,         ".uno:Label"            },
    { SID_FM_GROUPBOX,          ".uno:GroupBox"         },
    { SID_FM_LISTBOX,           ".uno:ListBox"          },
    { SID_FM_COMBOBOX,          ".uno:ComboBox"         },
    { SID_FM_EDIT,              ".uno:Edit"             },
    { SID_FM_DBGRID,            ".uno:Grid"             },
    { SID_FM_IMAGEBUTTON,       ".uno:Imagebutton"      },
    { SID_FM_IMAGECONTROL,      ".uno:ImageControl"     },
    { SID_FM_FILECONTROL,       ".uno:FileControl"      },
    { SID_FM_DATEFIELD,         ".uno:DateField"        },
    { SID_FM_TIMEFIELD,         ".uno:TimeField"        },
    { SID_FM_NUMERICFIELD,      ".uno:NumericField"     },
    { SID_FM_CURRENCYFIELD,     ".uno:CurrencyField"    },
    { SID_FM_PATTERNFIELD,      ".uno:PatternField"     },
    { SID_FM_DESIGN_MODE,       ".uno:SwitchControlDesignMode" },
    { SID_FM_FORMATTEDFIELD,    ".uno:FormattedField"   },
    { SID_FM_SCROLLBAR,         ".uno:ScrollBar"        },
    { SID_FM_SPINBUTTON,        ".uno:SpinButton"       },
    { 0,                        ""                      }
};

SFX_IMPL_TOOLBOX_CONTROL( SvxFmTbxCtlConfig, SfxUInt16Item );

//-----------------------------------------------------------------------
SvxFmTbxCtlConfig::SvxFmTbxCtlConfig( USHORT nSlotId, USHORT nId, ToolBox& rTbx )
	: SfxToolBoxControl( nSlotId, nId, rTbx )
	,nLastSlot( 0 )
{
    rTbx.SetItemBits( nId, TIB_DROPDOWN | rTbx.GetItemBits( nId ) );
}

//-----------------------------------------------------------------------
void SvxFmTbxCtlConfig::StateChanged(USHORT nSID, SfxItemState eState, const SfxPoolItem* pState )
{
	if (nSID == SID_FM_CONFIG)
	{
		UINT16 nSlot   = 0;
		if (eState >= SFX_ITEM_AVAILABLE)
			nSlot = ((SfxUInt16Item*)pState)->GetValue();

		switch( nSlot )
		{
			case SID_FM_PUSHBUTTON:
			case SID_FM_RADIOBUTTON:
			case SID_FM_CHECKBOX:
			case SID_FM_FIXEDTEXT:
			case SID_FM_GROUPBOX:
			case SID_FM_LISTBOX:
			case SID_FM_COMBOBOX:
            case SID_FM_NAVIGATIONBAR:
			case SID_FM_EDIT:
			case SID_FM_DBGRID:
			case SID_FM_IMAGEBUTTON:
			case SID_FM_IMAGECONTROL:
			case SID_FM_FILECONTROL:
			case SID_FM_DATEFIELD:
			case SID_FM_TIMEFIELD:
			case SID_FM_NUMERICFIELD:
			case SID_FM_CURRENCYFIELD:
			case SID_FM_PATTERNFIELD:
			case SID_FM_DESIGN_MODE:
			case SID_FM_FORMATTEDFIELD:
            case SID_FM_SCROLLBAR:
            case SID_FM_SPINBUTTON:
			{	// set a new image, matching to this slot
                rtl::OUString aSlotURL( RTL_CONSTASCII_USTRINGPARAM( "slot:" ));
                aSlotURL += rtl::OUString::valueOf( sal_Int32( nSlot ));
                Image aImage = GetImage( m_xFrame, 
                                        aSlotURL,
                                        hasBigImages(),
                                        GetToolBox().GetDisplayBackground().GetColor().IsDark() );

			    GetToolBox().SetItemImage( GetId(), aImage );
				nLastSlot = nSlot;
			}
			break;
		}
	}
	SfxToolBoxControl::StateChanged( nSID, eState,pState );
}

//-----------------------------------------------------------------------
SfxPopupWindowType SvxFmTbxCtlConfig::GetPopupWindowType() const
{
	return( nLastSlot == 0 ? SFX_POPUPWINDOW_ONCLICK : SFX_POPUPWINDOW_ONTIMEOUT );
}

//-----------------------------------------------------------------------
SfxPopupWindow* SvxFmTbxCtlConfig::CreatePopupWindow()
{
    if ( GetSlotId() == SID_FM_CONFIG )
    {
        ::svxform::FormToolboxes aToolboxes( m_xFrame );
        createAndPositionSubToolBar( aToolboxes.getToolboxResourceName( SID_FM_CONFIG ) );
    }
	return NULL;
}

//-----------------------------------------------------------------------
void SvxFmTbxCtlConfig::Select( USHORT /*nModifier*/ )
{
	//////////////////////////////////////////////////////////////////////
	// Click auf den Button SID_FM_CONFIG in der ObjectBar
	if ( nLastSlot )
    {
        USHORT n = 0;
        while( SlotToCommands[n].nSlotId > 0 )
        {
            if ( SlotToCommands[n].nSlotId == nLastSlot )
                break;
            n++;
        }

        if ( SlotToCommands[n].nSlotId > 0 )
        {
            Sequence< PropertyValue > aArgs;
            Dispatch( rtl::OUString::createFromAscii( SlotToCommands[n].pCommand ),
                      aArgs );
        }
    }
}


//========================================================================
// class SvxFmTbxCtlAbsRec
//========================================================================

SFX_IMPL_TOOLBOX_CONTROL( SvxFmTbxCtlAbsRec, SfxInt32Item );
DBG_NAME(SvxFmTbxCtlAbsRec);
//-----------------------------------------------------------------------
SvxFmTbxCtlAbsRec::SvxFmTbxCtlAbsRec( USHORT nSlotId, USHORT nId, ToolBox& rTbx )
	:SfxToolBoxControl( nSlotId, nId, rTbx )
{
	DBG_CTOR(SvxFmTbxCtlAbsRec,NULL);
}

//-----------------------------------------------------------------------
SvxFmTbxCtlAbsRec::~SvxFmTbxCtlAbsRec()
{
	DBG_DTOR(SvxFmTbxCtlAbsRec,NULL);
}

//-----------------------------------------------------------------------
void SvxFmTbxCtlAbsRec::StateChanged( USHORT nSID, SfxItemState eState, const SfxPoolItem* pState )
{
	USHORT 				nId = GetId();
	ToolBox*			pToolBox = &GetToolBox();
	SvxFmAbsRecWin*		pWin = (SvxFmAbsRecWin*)( pToolBox->GetItemWindow(nId) );

	DBG_ASSERT( pWin, "Control not found!" );

	if (pState)
	{
		const SfxInt32Item* pItem = PTR_CAST( SfxInt32Item, pState );
		DBG_ASSERT( pItem, "SvxFmTbxCtlAbsRec::StateChanged: invalid item!" );
		pWin->SetValue( pItem ? pItem->GetValue() : -1 );
	}

	BOOL bEnable = SFX_ITEM_DISABLED != eState && pState;
	if (!bEnable)
		pWin->SetText(String());

	//////////////////////////////////////////////////////////////////////
	// Enablen/disablen des Fensters
	pToolBox->EnableItem(nId, bEnable);
	SfxToolBoxControl::StateChanged( nSID, eState,pState );
}

//-----------------------------------------------------------------------
Window* SvxFmTbxCtlAbsRec::CreateItemWindow( Window* pParent )
{
	SvxFmAbsRecWin* pWin = new SvxFmAbsRecWin( pParent, this );
	pWin->SetUniqueId( UID_ABSOLUTE_RECORD_WINDOW );
	return pWin;
}


//========================================================================
// SvxFmTbxCtlRecText
//========================================================================

SFX_IMPL_TOOLBOX_CONTROL( SvxFmTbxCtlRecText, SfxBoolItem );
DBG_NAME(SvxFmTbxCtlRecText);
//-----------------------------------------------------------------------
SvxFmTbxCtlRecText::SvxFmTbxCtlRecText( USHORT nSlotId, USHORT nId, ToolBox& rTbx )
	:SfxToolBoxControl( nSlotId, nId, rTbx )
{
	DBG_CTOR(SvxFmTbxCtlRecText,NULL);
}

//-----------------------------------------------------------------------
SvxFmTbxCtlRecText::~SvxFmTbxCtlRecText()
{
	DBG_DTOR(SvxFmTbxCtlRecText,NULL);
}

//-----------------------------------------------------------------------
Window*	SvxFmTbxCtlRecText::CreateItemWindow( Window* pParent )
{
	XubString aText( SVX_RES(RID_STR_REC_TEXT) );
	FixedText* pFixedText = new FixedText( pParent );
	Size aSize( pFixedText->GetTextWidth( aText ), pFixedText->GetTextHeight( ) );
	pFixedText->SetText( aText );
	aSize.Width() += 6;
	pFixedText->SetSizePixel( aSize );
    pFixedText->SetBackground(Wallpaper(Color(COL_TRANSPARENT)));

	return pFixedText;
}


//========================================================================
// SvxFmTbxCtlRecFromText
//========================================================================

SFX_IMPL_TOOLBOX_CONTROL( SvxFmTbxCtlRecFromText, SfxBoolItem );
DBG_NAME(SvxFmTbxCtlRecFromText);
//-----------------------------------------------------------------------
SvxFmTbxCtlRecFromText::SvxFmTbxCtlRecFromText( USHORT nSlotId, USHORT nId, ToolBox& rTbx )
	:SfxToolBoxControl( nSlotId, nId, rTbx )
{
	DBG_CTOR(SvxFmTbxCtlRecFromText,NULL);
}

//-----------------------------------------------------------------------
SvxFmTbxCtlRecFromText::~SvxFmTbxCtlRecFromText()
{
	DBG_DTOR(SvxFmTbxCtlRecFromText,NULL);
}

//-----------------------------------------------------------------------
Window*	SvxFmTbxCtlRecFromText::CreateItemWindow( Window* pParent )
{
	XubString aText( SVX_RES(RID_STR_REC_FROM_TEXT) );
	FixedText* pFixedText = new FixedText( pParent, WB_CENTER );
	Size aSize( pFixedText->GetTextWidth( aText ), pFixedText->GetTextHeight( ) );
	aSize.Width() += 12;
	pFixedText->SetText( aText );
	pFixedText->SetSizePixel( aSize );
	pFixedText->SetBackground(Wallpaper(Color(COL_TRANSPARENT)));
	return pFixedText;
}


//========================================================================
// SvxFmTbxCtlRecTotal
//========================================================================
DBG_NAME(SvxFmTbxCtlRecTotal);
SFX_IMPL_TOOLBOX_CONTROL( SvxFmTbxCtlRecTotal, SfxStringItem );

//-----------------------------------------------------------------------
SvxFmTbxCtlRecTotal::SvxFmTbxCtlRecTotal( USHORT nSlotId, USHORT nId, ToolBox& rTbx )
	:SfxToolBoxControl( nSlotId, nId, rTbx )
	,pFixedText( NULL )
{
	DBG_CTOR(SvxFmTbxCtlRecTotal,NULL);
}

//-----------------------------------------------------------------------
SvxFmTbxCtlRecTotal::~SvxFmTbxCtlRecTotal()
{
	DBG_DTOR(SvxFmTbxCtlRecTotal,NULL);
}

//-----------------------------------------------------------------------
Window*	SvxFmTbxCtlRecTotal::CreateItemWindow( Window* pParent )
{
	pFixedText = new FixedText( pParent );
	String aSample( "123456", sizeof( "123456" ) - 1 );
	Size aSize( pFixedText->GetTextWidth( aSample ), pFixedText->GetTextHeight( ) );
	aSize.Width() += 12;
	pFixedText->SetSizePixel( aSize );
    pFixedText->SetBackground();
    pFixedText->SetPaintTransparent(TRUE);
	return pFixedText;
}

//-----------------------------------------------------------------------
void SvxFmTbxCtlRecTotal::StateChanged( USHORT nSID, SfxItemState eState, const SfxPoolItem* pState )
{
	//////////////////////////////////////////////////////////////////////
	// Setzen des FixedTextes
	if (GetSlotId() != SID_FM_RECORD_TOTAL)
		return;

	XubString aText;
	if (pState)
		aText = ((SfxStringItem*)pState)->GetValue();
	else
		aText = '?';

	pFixedText->SetText( aText );
	pFixedText->Update();
	pFixedText->Flush();

	SfxToolBoxControl::StateChanged( nSID, eState,pState );
}

//========================================================================
// SvxFmTbxNextRec
//========================================================================
SFX_IMPL_TOOLBOX_CONTROL( SvxFmTbxNextRec, SfxBoolItem );

//-----------------------------------------------------------------------
SvxFmTbxNextRec::SvxFmTbxNextRec( USHORT nSlotId, USHORT nId, ToolBox& rTbx )
	:SfxToolBoxControl( nSlotId, nId, rTbx )
{
	rTbx.SetItemBits(nId, rTbx.GetItemBits(nId) | TIB_REPEAT);

	AllSettings	aSettings = rTbx.GetSettings();
	MouseSettings aMouseSettings = aSettings.GetMouseSettings();
	aMouseSettings.SetButtonRepeat(aMouseSettings.GetButtonRepeat() / 4);
	aSettings.SetMouseSettings(aMouseSettings);
	rTbx.SetSettings(aSettings, TRUE);
}

//========================================================================
// SvxFmTbxPrevRec
//========================================================================
SFX_IMPL_TOOLBOX_CONTROL( SvxFmTbxPrevRec, SfxBoolItem );

//-----------------------------------------------------------------------
SvxFmTbxPrevRec::SvxFmTbxPrevRec( USHORT nSlotId, USHORT nId, ToolBox& rTbx )
	:SfxToolBoxControl( nSlotId, nId, rTbx )
{
	rTbx.SetItemBits(nId, rTbx.GetItemBits(nId) | TIB_REPEAT);
}


