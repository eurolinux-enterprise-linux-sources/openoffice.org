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
#include <tools/debug.hxx>

#include "unoedhlp.hxx"
#include <svx/editdata.hxx>
#include <svx/editeng.hxx>

//------------------------------------------------------------------------

TYPEINIT1( SvxEditSourceHint, TextHint );

SvxEditSourceHint::SvxEditSourceHint( ULONG _nId ) : 
    TextHint( _nId ),
    mnStart( 0 ),
    mnEnd( 0 )
{
}

SvxEditSourceHint::SvxEditSourceHint( ULONG _nId, ULONG nValue, ULONG nStart, ULONG nEnd ) : 
    TextHint( _nId, nValue ),
    mnStart( nStart),
    mnEnd( nEnd )
{
}

ULONG SvxEditSourceHint::GetValue() const
{
    return TextHint::GetValue();
}

ULONG SvxEditSourceHint::GetStartValue() const
{
    return mnStart;
}

ULONG SvxEditSourceHint::GetEndValue() const
{
    return mnEnd;
}

void SvxEditSourceHint::SetValue( ULONG n )
{
    TextHint::SetValue( n );
}

void SvxEditSourceHint::SetStartValue( ULONG n )
{
    mnStart = n;
}

void SvxEditSourceHint::SetEndValue( ULONG n )
{
    mnEnd = n; 
}

//------------------------------------------------------------------------

::std::auto_ptr<SfxHint> SvxEditSourceHelper::EENotification2Hint( EENotify* aNotify )
{
    if( aNotify )
    {
        switch( aNotify->eNotificationType )
        {
            case EE_NOTIFY_TEXTMODIFIED:
                return ::std::auto_ptr<SfxHint>( new TextHint( TEXT_HINT_MODIFIED, aNotify->nParagraph ) );

            case EE_NOTIFY_PARAGRAPHINSERTED:
                return ::std::auto_ptr<SfxHint>( new TextHint( TEXT_HINT_PARAINSERTED, aNotify->nParagraph ) );

            case EE_NOTIFY_PARAGRAPHREMOVED: 
                return ::std::auto_ptr<SfxHint>( new TextHint( TEXT_HINT_PARAREMOVED, aNotify->nParagraph ) );

            case EE_NOTIFY_PARAGRAPHSMOVED:
                return ::std::auto_ptr<SfxHint>( new SvxEditSourceHint( EDITSOURCE_HINT_PARASMOVED, aNotify->nParagraph, aNotify->nParam1, aNotify->nParam2 ) );

            case EE_NOTIFY_TEXTHEIGHTCHANGED:
                return ::std::auto_ptr<SfxHint>( new TextHint( TEXT_HINT_TEXTHEIGHTCHANGED, aNotify->nParagraph ) );

            case EE_NOTIFY_TEXTVIEWSCROLLED:
                return ::std::auto_ptr<SfxHint>( new TextHint( TEXT_HINT_VIEWSCROLLED ) );

            case EE_NOTIFY_TEXTVIEWSELECTIONCHANGED:
                return ::std::auto_ptr<SfxHint>( new SvxEditSourceHint( EDITSOURCE_HINT_SELECTIONCHANGED ) );

            case EE_NOTIFY_BLOCKNOTIFICATION_START:
                return ::std::auto_ptr<SfxHint>( new TextHint( TEXT_HINT_BLOCKNOTIFICATION_START, 0 ) );

            case EE_NOTIFY_BLOCKNOTIFICATION_END:
                return ::std::auto_ptr<SfxHint>( new TextHint( TEXT_HINT_BLOCKNOTIFICATION_END, 0 ) );

            case EE_NOTIFY_INPUT_START:
                return ::std::auto_ptr<SfxHint>( new TextHint( TEXT_HINT_INPUT_START, 0 ) );

            case EE_NOTIFY_INPUT_END:
                return ::std::auto_ptr<SfxHint>( new TextHint( TEXT_HINT_INPUT_END, 0 ) );

            default:
                DBG_ERROR( "SvxEditSourceHelper::EENotification2Hint unknown notification" );
                break;
        }
    }

    return ::std::auto_ptr<SfxHint>( new SfxHint() );
}

sal_Bool SvxEditSourceHelper::GetAttributeRun( USHORT& nStartIndex, USHORT& nEndIndex, const EditEngine& rEE, USHORT nPara, USHORT nIndex )
{
    EECharAttribArray aCharAttribs;
    
    rEE.GetCharAttribs( nPara, aCharAttribs );
    
    // find closest index in front of nIndex
    USHORT nAttr, nCurrIndex;
    sal_Int32 nClosestStartIndex;
    for( nAttr=0, nClosestStartIndex=0; nAttr<aCharAttribs.Count(); ++nAttr )
    {
        nCurrIndex = aCharAttribs[nAttr].nStart;
        
        if( nCurrIndex > nIndex )
            break; // aCharAttribs array is sorted in increasing order for nStart values

        if( nCurrIndex > nClosestStartIndex )
        {
            nClosestStartIndex = nCurrIndex;
        }
    }

    // find closest index behind of nIndex
    sal_Int32 nClosestEndIndex;
    for( nAttr=0, nClosestEndIndex=rEE.GetTextLen(nPara); nAttr<aCharAttribs.Count(); ++nAttr )
    {
        nCurrIndex = aCharAttribs[nAttr].nEnd;

        if( nCurrIndex > nIndex &&
            nCurrIndex < nClosestEndIndex )
        {
            nClosestEndIndex = nCurrIndex;
        }
    }

    nStartIndex = static_cast<USHORT>( nClosestStartIndex );
    nEndIndex = static_cast<USHORT>( nClosestEndIndex );

    return sal_True;
}

Point SvxEditSourceHelper::EEToUserSpace( const Point& rPoint, const Size& rEESize, bool bIsVertical )
{
    return bIsVertical ? Point( -rPoint.Y() + rEESize.Height(), rPoint.X() ) : rPoint;
}

Point SvxEditSourceHelper::UserSpaceToEE( const Point& rPoint, const Size& rEESize, bool bIsVertical )
{
    return bIsVertical ? Point( rPoint.Y(), -rPoint.X() + rEESize.Height() ) : rPoint;
}

Rectangle SvxEditSourceHelper::EEToUserSpace( const Rectangle& rRect, const Size& rEESize, bool bIsVertical )
{
    // #106775# Don't touch rect if not vertical
    return bIsVertical ? Rectangle( EEToUserSpace(rRect.BottomLeft(), rEESize, bIsVertical),
                                    EEToUserSpace(rRect.TopRight(), rEESize, bIsVertical) ) : rRect;
}

Rectangle SvxEditSourceHelper::UserSpaceToEE( const Rectangle& rRect, const Size& rEESize, bool bIsVertical )
{
    // #106775# Don't touch rect if not vertical
    return bIsVertical ? Rectangle( UserSpaceToEE(rRect.TopRight(), rEESize, bIsVertical),
                                    UserSpaceToEE(rRect.BottomLeft(), rEESize, bIsVertical) ) : rRect;
}
