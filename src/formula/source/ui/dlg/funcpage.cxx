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

#include <sfx2/dispatch.hxx>
#include <sfx2/docfile.hxx>
#include <svtools/zforlist.hxx>
#include <svtools/stritem.hxx>
#include "formula/IFunctionDescription.hxx"

#include "funcpage.hxx"
#include "formdlgs.hrc"
#include "ForResId.hrc"
#include "ModuleHelper.hxx"
//============================================================================
namespace formula
{

FormulaListBox::FormulaListBox( Window* pParent, WinBits nWinStyle):
	ListBox(pParent,nWinStyle)
{}

FormulaListBox::FormulaListBox( Window* pParent, const ResId& rResId ):
	ListBox(pParent,rResId)
{}

void FormulaListBox::KeyInput( const KeyEvent& rKEvt )
{
	KeyEvent aKEvt=rKEvt;
	//ListBox::KeyInput(rKEvt);

	if(aKEvt.GetCharCode()==' ')
		DoubleClick();
}

long FormulaListBox::PreNotify( NotifyEvent& rNEvt )
{
	NotifyEvent aNotifyEvt=rNEvt;

	long nResult=ListBox::PreNotify(rNEvt);

	USHORT nSwitch=aNotifyEvt.GetType();
	if(nSwitch==EVENT_KEYINPUT)
	{
		KeyInput(*aNotifyEvt.GetKeyEvent());
	}
	return nResult;
}



//============================================================================

inline USHORT Lb2Cat( USHORT nLbPos )
{
	// Kategorie 0 == LRU, sonst Categories == LbPos-1
	if ( nLbPos > 0 )
		nLbPos -= 1;

	return nLbPos;
}

//============================================================================

FuncPage::FuncPage(Window* pParent,const IFunctionManager* _pFunctionManager):
	TabPage(pParent,ModuleRes(RID_FORMULATAB_FUNCTION)),
	//
	aFtCategory		( this, ModuleRes( FT_CATEGORY ) ),
	aLbCategory		( this, ModuleRes( LB_CATEGORY ) ),
	aFtFunction		( this, ModuleRes( FT_FUNCTION ) ),
	aLbFunction		( this, ModuleRes( LB_FUNCTION ) ),
    m_pFunctionManager(_pFunctionManager)
{
	FreeResource();
    m_aSmartHelpId = aLbFunction.GetSmartHelpId();
    aLbFunction.SetSmartUniqueId(m_aSmartHelpId);

	InitLRUList();

    const sal_uInt32 nCategoryCount = m_pFunctionManager->getCount();
    for(sal_uInt32 j= 0; j < nCategoryCount; ++j)
    {
        const IFunctionCategory* pCategory = m_pFunctionManager->getCategory(j);
        aLbCategory.SetEntryData(aLbCategory.InsertEntry(pCategory->getName()),(void*)pCategory);
    }
    
	aLbCategory.SelectEntryPos(1);
	UpdateFunctionList();
	aLbCategory.SetSelectHdl( LINK( this, FuncPage, SelHdl ) );
	aLbFunction.SetSelectHdl( LINK( this, FuncPage, SelHdl ) );
	aLbFunction.SetDoubleClickHdl( LINK( this, FuncPage, DblClkHdl ) );
}
// -----------------------------------------------------------------------------
void FuncPage::impl_addFunctions(const IFunctionCategory* _pCategory)
{
    const sal_uInt32 nCount = _pCategory->getCount();
    for(sal_uInt32 i = 0 ; i < nCount; ++i)
    {
        TFunctionDesc pDesc(_pCategory->getFunction(i));
        aLbFunction.SetEntryData(
		    aLbFunction.InsertEntry(pDesc->getFunctionName() ),(void*)pDesc );
    } // for(sal_uInt32 i = 0 ; i < nCount; ++i)
}

void FuncPage::UpdateFunctionList()
{
	USHORT	nSelPos	  = aLbCategory.GetSelectEntryPos();
    const IFunctionCategory* pCategory = static_cast<const IFunctionCategory*>(aLbCategory.GetEntryData(nSelPos));
	USHORT	nCategory = ( LISTBOX_ENTRY_NOTFOUND != nSelPos )
							? Lb2Cat( nSelPos ) : 0;

    (void)nCategory;

	aLbFunction.Clear();
	aLbFunction.SetUpdateMode( FALSE );
	//------------------------------------------------------

	if ( nSelPos > 0 )
	{
        if ( pCategory == NULL )
        {
            const sal_uInt32 nCount = m_pFunctionManager->getCount();
            for(sal_uInt32 i = 0 ; i < nCount; ++i)
            {
                impl_addFunctions(m_pFunctionManager->getCategory(i));
            }
        }
        else
        {
            impl_addFunctions(pCategory);
        }
	}
	else // LRU-Liste
	{
        ::std::vector< TFunctionDesc >::iterator aIter = aLRUList.begin();
        ::std::vector< TFunctionDesc >::iterator aEnd = aLRUList.end();

		for ( ; aIter != aEnd; ++aIter )
		{
            const IFunctionDescription* pDesc = *aIter;
            if (pDesc)  // may be null if a function is no longer available
            {
                aLbFunction.SetEntryData(
                    aLbFunction.InsertEntry( pDesc->getFunctionName() ), (void*)pDesc );
            }
		}
	}

	//------------------------------------------------------
	aLbFunction.SetUpdateMode( TRUE );
	aLbFunction.SelectEntryPos(0);

	if(IsVisible())	SelHdl(&aLbFunction);
}

IMPL_LINK( FuncPage, SelHdl, ListBox*, pLb )
{
	if(pLb==&aLbFunction)
	{
        const IFunctionDescription* pDesc = GetFuncDesc( GetFunction() );
        if ( pDesc )
        {
            const long nHelpId = pDesc->getHelpId();
            if ( nHelpId )
                aLbFunction.SetSmartHelpId(SmartId(nHelpId));
        }
		aSelectionLink.Call(this);
	}
	else
	{
        aLbFunction.SetSmartHelpId(m_aSmartHelpId);
		UpdateFunctionList();
	}
	return 0;
}

IMPL_LINK( FuncPage, DblClkHdl, ListBox*, EMPTYARG )
{
	aDoubleClickLink.Call(this);
	return 0;
}

void FuncPage::SetCategory(USHORT nCat)
{
	aLbCategory.SelectEntryPos(nCat);
	UpdateFunctionList();
}
USHORT FuncPage::GetFuncPos(const IFunctionDescription* _pDesc)
{
    return aLbFunction.GetEntryPos(_pDesc);
}
void FuncPage::SetFunction(USHORT nFunc)
{
	aLbFunction.SelectEntryPos(nFunc);
}

void FuncPage::SetFocus()
{
	aLbFunction.GrabFocus();
}

USHORT FuncPage::GetCategory()
{
	return aLbCategory.GetSelectEntryPos();
}

USHORT FuncPage::GetFunction()
{
	return aLbFunction.GetSelectEntryPos();
}

USHORT FuncPage::GetFunctionEntryCount()
{
	return aLbFunction.GetSelectEntryCount();
}

String FuncPage::GetSelFunctionName() const
{
	return aLbFunction.GetSelectEntry();
}
const IFunctionDescription*	FuncPage::GetFuncDesc( USHORT nPos ) const
{
	// nicht schoen, aber hoffentlich selten
	return (const IFunctionDescription*) aLbFunction.GetEntryData(nPos);
}

void FuncPage::InitLRUList()
{
    ::std::vector< const IFunctionDescription*> aRUFunctions;
    m_pFunctionManager->fillLastRecentlyUsedFunctions(aLRUList);
}


} // formula

