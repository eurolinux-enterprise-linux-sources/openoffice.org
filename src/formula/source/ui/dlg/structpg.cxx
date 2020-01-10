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
#include "precompiled_formula.hxx"



//----------------------------------------------------------------------------

#include <svtools/zforlist.hxx>
#include <svtools/stritem.hxx>

#include "structpg.hxx"
#include "formdlgs.hrc"
#include "formula/formdata.hxx"
#include "formula/formula.hxx"
#include "ModuleHelper.hxx"
#include "formula/IFunctionDescription.hxx"
#include "ForResId.hrc"

//----------------------------------------------------------------------------
namespace formula
{
StructListBox::StructListBox(Window* pParent, const ResId& rResId ):
	SvTreeListBox(pParent,rResId )
{
	bActiveFlag=FALSE;

    Font aFont( GetFont() );
    Size aSize = aFont.GetSize();
    aSize.Height() -= 2;
    aFont.SetSize( aSize );
    SetFont( aFont );
}

SvLBoxEntry* StructListBox::InsertStaticEntry(
        const XubString& rText,
        const Image& rEntryImg, const Image& rEntryImgHC,
        SvLBoxEntry* pParent, ULONG nPos, IFormulaToken* pToken )
{
    SvLBoxEntry* pEntry = InsertEntry( rText, rEntryImg, rEntryImg, pParent, FALSE, nPos, pToken );
    SvLBoxContextBmp* pBmpItem = static_cast< SvLBoxContextBmp* >( pEntry->GetFirstItem( SV_ITEM_ID_LBOXCONTEXTBMP ) );
    DBG_ASSERT( pBmpItem, "StructListBox::InsertStaticEntry - missing item" );
    pBmpItem->SetBitmap1( rEntryImgHC, BMP_COLOR_HIGHCONTRAST );
    pBmpItem->SetBitmap2( rEntryImgHC, BMP_COLOR_HIGHCONTRAST );
    return pEntry;
}

void StructListBox::SetActiveFlag(BOOL bFlag)
{
	bActiveFlag=bFlag;
}

BOOL StructListBox::GetActiveFlag()
{
	return bActiveFlag;
}

void StructListBox::MouseButtonDown( const MouseEvent& rMEvt )
{
	bActiveFlag=TRUE;
	SvTreeListBox::MouseButtonDown(rMEvt);
}

void StructListBox::GetFocus()
{
	bActiveFlag=TRUE;
	SvTreeListBox::GetFocus();
}

void StructListBox::LoseFocus()
{
	bActiveFlag=FALSE;
	SvTreeListBox::LoseFocus();
}

//==============================================================================

StructPage::StructPage(Window* pParent):
	TabPage(pParent,ModuleRes(RID_FORMULATAB_STRUCT)),
	//
	aFtStruct		( this, ModuleRes( FT_STRUCT ) ),
	aTlbStruct		( this, ModuleRes( TLB_STRUCT ) ),
    maImgEnd        ( ModuleRes( BMP_STR_END ) ),
    maImgError      ( ModuleRes( BMP_STR_ERROR ) ),
    maImgEndHC      ( ModuleRes( BMP_STR_END_H ) ),
    maImgErrorHC    ( ModuleRes( BMP_STR_ERROR_H ) ),
	pSelectedToken	( NULL )
{
	aTlbStruct.SetWindowBits(WB_HASLINES|WB_CLIPCHILDREN|
						WB_HASBUTTONS|WB_HSCROLL|WB_NOINITIALSELECTION);

    aTlbStruct.SetNodeDefaultImages();
    aTlbStruct.SetDefaultExpandedEntryBmp( Image( ModuleRes( BMP_STR_OPEN ) ) );
    aTlbStruct.SetDefaultCollapsedEntryBmp( Image( ModuleRes( BMP_STR_CLOSE ) ) );
    aTlbStruct.SetDefaultExpandedEntryBmp( Image( ModuleRes( BMP_STR_OPEN_H ) ), BMP_COLOR_HIGHCONTRAST );
    aTlbStruct.SetDefaultCollapsedEntryBmp( Image( ModuleRes( BMP_STR_CLOSE_H ) ), BMP_COLOR_HIGHCONTRAST );

    FreeResource();

	aTlbStruct.SetSelectHdl(LINK( this, StructPage, SelectHdl ) );
}

void StructPage::ClearStruct()
{
	aTlbStruct.SetActiveFlag(FALSE);
	aTlbStruct.Clear();
}

SvLBoxEntry* StructPage::InsertEntry( const XubString& rText, SvLBoxEntry* pParent,
									   USHORT nFlag,ULONG nPos,IFormulaToken* pIFormulaToken)
{
    aTlbStruct.SetActiveFlag( FALSE );

    SvLBoxEntry* pEntry = NULL;
    switch( nFlag )
	{
		case STRUCT_FOLDER:
            pEntry = aTlbStruct.InsertEntry( rText, pParent, FALSE, nPos, pIFormulaToken );
        break;
        case STRUCT_END:
            pEntry = aTlbStruct.InsertStaticEntry( rText, maImgEnd, maImgEndHC, pParent, nPos, pIFormulaToken );
        break;
        case STRUCT_ERROR:
            pEntry = aTlbStruct.InsertStaticEntry( rText, maImgError, maImgErrorHC, pParent, nPos, pIFormulaToken );
        break;
	}

    if( pEntry && pParent )
        aTlbStruct.Expand( pParent );
	return pEntry;
}

String StructPage::GetEntryText(SvLBoxEntry* pEntry) const
{
	String aString;
	if(pEntry!=NULL)
		aString=aTlbStruct.GetEntryText(pEntry);
	return	aString;
}

SvLBoxEntry* StructPage::GetParent(SvLBoxEntry* pEntry) const
{
    return aTlbStruct.GetParent(pEntry);
}
IFormulaToken* StructPage::GetFunctionEntry(SvLBoxEntry* pEntry)
{
	if(pEntry!=NULL)
	{
		IFormulaToken * pToken=(IFormulaToken *)pEntry->GetUserData();
		if(pToken!=NULL)
		{
			if ( !(pToken->isFunction() || pToken->getArgumentCount() > 1 ) )
			{
				return GetFunctionEntry(aTlbStruct.GetParent(pEntry));
			}
			else
			{
				return pToken;
			}
		}
	}
	return NULL;
}

IMPL_LINK( StructPage, SelectHdl, SvTreeListBox*, pTlb )
{
	if(aTlbStruct.GetActiveFlag())
	{
		if(pTlb==&aTlbStruct)
		{
			SvLBoxEntry*	pCurEntry=aTlbStruct.GetCurEntry();
			if(pCurEntry!=NULL)
			{
				pSelectedToken=(IFormulaToken *)pCurEntry->GetUserData();
				if(pSelectedToken!=NULL)
				{
					if ( !(pSelectedToken->isFunction() || pSelectedToken->getArgumentCount() > 1) )
					{
						pSelectedToken = GetFunctionEntry(pCurEntry);
					}
				}
			}
		}

		aSelLink.Call(this);
	}
	return 0;
}

IFormulaToken* StructPage::GetSelectedToken()
{
	return pSelectedToken;
}

String StructPage::GetSelectedEntryText()
{
	return aTlbStruct.GetEntryText(aTlbStruct.GetCurEntry());
}

} // formula

