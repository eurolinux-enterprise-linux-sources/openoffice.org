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
#include "fontlb.hxx"
#include <vcl/svapp.hxx>

// ============================================================================

DBG_NAME( SvLBoxFontString );

SvLBoxFontString::SvLBoxFontString() :
    SvLBoxString()
{
    DBG_CTOR( SvLBoxFontString, 0 );
}

SvLBoxFontString::SvLBoxFontString(
        SvLBoxEntry* pEntry, sal_uInt16 nFlags, const XubString& rString,
        const Font& rFont, const Color* pColor ) :
    SvLBoxString( pEntry, nFlags, rString ),
    maFont( rFont ),
    mbUseColor( pColor != NULL )
{
    DBG_CTOR( SvLBoxFontString, 0 );
    SetText( pEntry, rString );
    if( pColor )
        maFont.SetColor( *pColor );
}

SvLBoxFontString::~SvLBoxFontString()
{
    DBG_DTOR( SvLBoxFontString, 0 );
}


SvLBoxItem* SvLBoxFontString::Create() const
{
    DBG_CHKTHIS( SvLBoxFontString, 0 );
    return new SvLBoxFontString;
}

void SvLBoxFontString::Paint( const Point& rPos, SvLBox& rDev, sal_uInt16 nFlags, SvLBoxEntry* pEntry )
{
    DBG_CHKTHIS( SvLBoxFontString, 0 );
    Font aOldFont( rDev.GetFont() );
    Font aNewFont( maFont );
    bool bSel = (nFlags & SVLISTENTRYFLAG_SELECTED) != 0;
//  if( !mbUseColor )               // selection gets font color, if available
    if( !mbUseColor || bSel )       // selection always gets highlight color
    {
        const StyleSettings& rSett = Application::GetSettings().GetStyleSettings();
        aNewFont.SetColor( bSel ? rSett.GetHighlightTextColor() : rSett.GetFieldTextColor() );
    }

    rDev.SetFont( aNewFont );
    SvLBoxString::Paint( rPos, rDev, nFlags, pEntry );
    rDev.SetFont( aOldFont );
}

void SvLBoxFontString::InitViewData( SvLBox* pView, SvLBoxEntry* pEntry, SvViewDataItem* pViewData )
{
    DBG_CHKTHIS( SvLBoxFontString, 0 );
    Font aOldFont( pView->GetFont() );
    pView->SetFont( maFont );
    SvLBoxString::InitViewData( pView, pEntry, pViewData);
    pView->SetFont( aOldFont );
}


// ============================================================================

SvxFontListBox::SvxFontListBox( Window* pParent, const ResId& rResId ) :
    SvTabListBox( pParent, rResId ),
    maStdFont( GetFont() ),
    mbUseFont( false )
{
    maStdFont.SetTransparent( TRUE );
    maEntryFont = maStdFont;
}

void SvxFontListBox::InsertFontEntry( const String& rString, const Font& rFont, const Color* pColor )
{
    mbUseFont = true;           // InitEntry() will use maEntryFont
    maEntryFont = rFont;        // font to use in InitEntry() over InsertEntry()
    mpEntryColor = pColor;      // color to use in InitEntry() over InsertEntry()
    InsertEntry( rString );
    mbUseFont = false;
}

void SvxFontListBox::SelectEntryPos( sal_uInt16 nPos, bool bSelect )
{
    SvLBoxEntry* pEntry = GetEntry( nPos );
    if( pEntry )
    {
        Select( pEntry, bSelect );
        ShowEntry( pEntry );
    }
}

void SvxFontListBox::SetNoSelection()
{
    SelectAll( FALSE, TRUE );
}

ULONG SvxFontListBox::GetSelectEntryPos() const
{
    SvLBoxEntry* pSvLBoxEntry = FirstSelected();
    return pSvLBoxEntry ? GetModel()->GetAbsPos( pSvLBoxEntry ) : LIST_APPEND;
}

XubString SvxFontListBox::GetSelectEntry() const
{
    return GetEntryText( GetSelectEntryPos() );
}

void SvxFontListBox::InitEntry(
        SvLBoxEntry* pEntry, const XubString& rEntryText,
        const Image& rCollImg, const Image& rExpImg,
        SvLBoxButtonKind eButtonKind )
{
    if( mbUseFont )
    {
        if( nTreeFlags & TREEFLAG_CHKBTN )
            pEntry->AddItem( new SvLBoxButton( pEntry, eButtonKind, 0,
                                               pCheckButtonData ) );
        pEntry->AddItem( new SvLBoxContextBmp( pEntry, 0, rCollImg, rExpImg, SVLISTENTRYFLAG_EXPANDED ) );
        pEntry->AddItem( new SvLBoxFontString( pEntry, 0, rEntryText, maEntryFont, mpEntryColor ) );
    }
    else
        SvTreeListBox::InitEntry( pEntry, rEntryText, rCollImg, rExpImg,
                                  eButtonKind );
}

#if ENABLE_LAYOUT

namespace layout
{

SvxFontListBox::~SvxFontListBox ()
{
}

sal_uInt16 SvxFontListBox::InsertFontEntry (String const& entry, Font const&, Color const*)
{
    return InsertEntry (entry);
}

SvxFontListBox::SvxFontListBox( Context* pParent, const char* pFile)
: ListBox( pParent, pFile )
{
}

/*IMPL_IMPL (SvxFontListBox, ListBox);
IMPL_CONSTRUCTORS (SvxFontListBox, ListBox, "svxfontlistbox");
IMPL_GET_IMPL (SvxFontListBox);
IMPL_GET_WINDOW (SvxFontListBox);*/

};

#endif

// ============================================================================

