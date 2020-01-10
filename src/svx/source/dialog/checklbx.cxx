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
#include <tools/shl.hxx>

#define _SVX_CHECKLBX_CXX

#include <svx/checklbx.hxx>
#include <svx/dialmgr.hxx>

#include <svx/dialogs.hrc>

// class SvxCheckListBox -------------------------------------------------

SvxCheckListBox::SvxCheckListBox( Window* pParent, WinBits nWinStyle ) :

	SvTreeListBox( pParent, nWinStyle )

{
	Init_Impl();
}

// -----------------------------------------------------------------------

SvxCheckListBox::SvxCheckListBox( Window* pParent, const ResId& rResId ) :

	SvTreeListBox( pParent, rResId )

{
	Init_Impl();
}

// -----------------------------------------------------------------------

SvxCheckListBox::SvxCheckListBox( Window* pParent, const ResId& rResId,
                                  const Image& rNormalStaticImage,
                                  const Image& /*TODO#i72485# rHighContrastStaticImage*/ ) :

    SvTreeListBox( pParent, rResId )

{
    Init_Impl();
    pCheckButton->aBmps[SV_BMP_STATICIMAGE] = rNormalStaticImage;
}

// -----------------------------------------------------------------------

SvxCheckListBox::~SvxCheckListBox()
{
	delete pCheckButton;
}

// -----------------------------------------------------------------------

void SvxCheckListBox::Init_Impl()
{
	pCheckButton = new SvLBoxButtonData( this );
	EnableCheckButton( pCheckButton );
}

// -----------------------------------------------------------------------

void SvxCheckListBox::InsertEntry( const String& rStr, USHORT nPos,
                                   void* pUserData,
                                   SvLBoxButtonKind eButtonKind )
{
	SvTreeListBox::InsertEntry( rStr, NULL, FALSE, nPos, pUserData,
                                eButtonKind );
}

// -----------------------------------------------------------------------

void SvxCheckListBox::RemoveEntry( USHORT nPos )
{
	if ( nPos < GetEntryCount() )
		SvTreeListBox::GetModel()->Remove( GetEntry( nPos ) );
}

// -----------------------------------------------------------------------

void SvxCheckListBox::SelectEntryPos( USHORT nPos, BOOL bSelect )
{
	if ( nPos < GetEntryCount() )
		Select( GetEntry( nPos ), bSelect );
}

// -----------------------------------------------------------------------

USHORT SvxCheckListBox::GetSelectEntryPos() const
{
	SvLBoxEntry* pEntry = GetCurEntry();

	if ( pEntry )
		return (USHORT)GetModel()->GetAbsPos( pEntry );
	return LISTBOX_ENTRY_NOTFOUND;
}

// -----------------------------------------------------------------------

String SvxCheckListBox::GetText( USHORT nPos ) const
{
	SvLBoxEntry* pEntry = GetEntry( nPos );

	if ( pEntry )
		return GetEntryText( pEntry );
	return String();
}

// -----------------------------------------------------------------------

USHORT SvxCheckListBox::GetCheckedEntryCount() const
{
	USHORT nCheckCount = 0;
	USHORT nCount = (USHORT)GetEntryCount();

	for ( USHORT i = 0; i < nCount; ++i )
	{
		if ( IsChecked( i ) )
			nCheckCount++;
	}
	return nCheckCount;
}

// -----------------------------------------------------------------------

void SvxCheckListBox::CheckEntryPos( USHORT nPos, BOOL bCheck )
{
	if ( nPos < GetEntryCount() )
		SetCheckButtonState(
			GetEntry( nPos ), bCheck ? SvButtonState( SV_BUTTON_CHECKED ) :
									   SvButtonState( SV_BUTTON_UNCHECKED ) );
}

// -----------------------------------------------------------------------

BOOL SvxCheckListBox::IsChecked( USHORT nPos ) const
{
	if ( nPos < GetEntryCount() )
		return (GetCheckButtonState( GetEntry( nPos ) ) == SV_BUTTON_CHECKED);
	else
		return FALSE;
}

// -----------------------------------------------------------------------

void* SvxCheckListBox::SetEntryData	( USHORT nPos, void* pNewData )
{
	void* pOld = NULL;

	if ( nPos < GetEntryCount() )
	{
		pOld = GetEntry( nPos )->GetUserData();
		GetEntry( nPos )->SetUserData( pNewData );
	}
	return pOld;
}

// -----------------------------------------------------------------------

void* SvxCheckListBox::GetEntryData( USHORT nPos ) const
{
	if ( nPos < GetEntryCount() )
		return GetEntry( nPos )->GetUserData();
	else
		return NULL;
}

// -----------------------------------------------------------------------

void SvxCheckListBox::ToggleCheckButton( SvLBoxEntry* pEntry )
{
    if ( pEntry )
    {
        if ( !IsSelected( pEntry ) )
            Select( pEntry );
        else
            CheckEntryPos( GetSelectEntryPos(), !IsChecked( GetSelectEntryPos() ) );
    }
}

// -----------------------------------------------------------------------

void SvxCheckListBox::MouseButtonDown( const MouseEvent& rMEvt )
{
	if ( rMEvt.IsLeft() )
	{
		Point aPnt = rMEvt.GetPosPixel();
		SvLBoxEntry* pEntry = GetEntry( aPnt );

		if ( pEntry )
		{
			BOOL bCheck = ( GetCheckButtonState( pEntry ) == SV_BUTTON_CHECKED );
			SvLBoxItem* pItem = GetItem( pEntry, aPnt.X() );

			if ( pItem && pItem->IsA() == SV_ITEM_ID_LBOXBUTTON )
			{
				SvTreeListBox::MouseButtonDown( rMEvt );
				Select( pEntry, TRUE );
				return;
			}
			else
			{
				ToggleCheckButton( pEntry );
				SvTreeListBox::MouseButtonDown( rMEvt );
				if ( bCheck != ( GetCheckButtonState( pEntry ) == SV_BUTTON_CHECKED ) )
					CheckButtonHdl();
				return;
			}
		}
	}
	SvTreeListBox::MouseButtonDown( rMEvt );
}

// -----------------------------------------------------------------------

void SvxCheckListBox::KeyInput( const KeyEvent& rKEvt )
{
	const KeyCode& rKey = rKEvt.GetKeyCode();

	if ( rKey.GetCode() == KEY_RETURN || rKey.GetCode() == KEY_SPACE )
	{
		SvLBoxEntry* pEntry = GetCurEntry();

		if ( pEntry )
		{
			BOOL bCheck = ( GetCheckButtonState( pEntry ) == SV_BUTTON_CHECKED );
			ToggleCheckButton( pEntry );
			if ( bCheck != ( GetCheckButtonState( pEntry ) == SV_BUTTON_CHECKED ) )
				CheckButtonHdl();
		}
	}
	else if ( GetEntryCount() )
		SvTreeListBox::KeyInput( rKEvt );
}

// -----------------------------------------------------------------------

SvLBoxEntry* SvxCheckListBox::InsertEntry( const XubString& rText, SvLBoxEntry* pParent, BOOL bChildsOnDemand, ULONG nPos, void* pUserData, SvLBoxButtonKind eButtonKind )
{
	return SvTreeListBox::InsertEntry( rText, pParent, bChildsOnDemand, nPos, pUserData, eButtonKind );
}

