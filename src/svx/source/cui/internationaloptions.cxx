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
#include "internationaloptions.hxx"
#ifndef _OFFAPP_INTERNATIONALOPTIONS_HRC_
#include "internationaloptions.hrc"
#endif
#include <svtools/eitem.hxx>

#include <svx/dialogs.hrc>
#include "helpid.hrc"
#include <svx/dialmgr.hxx>

namespace offapp
{

	struct InternationalOptionsPage::IMPL
	{
		FixedLine			m_aFL_DefaultTextDirection;
		RadioButton			m_aRB_TxtDirLeft2Right;
		RadioButton			m_aRB_TxtDirRight2Left;
		FixedLine			m_aFL_SheetView;
		CheckBox			m_aCB_ShtVwRight2Left;
		CheckBox			m_aCB_ShtVwCurrentDocOnly;

		BOOL				m_bEnable_SheetView_Opt : 1;

		inline				IMPL( Window* _pParent );

		inline void			EnableOption_SheetView( BOOL _bEnable = TRUE );
		void				ShowOption_SheetView( BOOL _bShow = TRUE );

		BOOL				FillItemSet( SfxItemSet& _rSet );
		void				Reset( const SfxItemSet& _rSet );
	};

	inline InternationalOptionsPage::IMPL::IMPL( Window* _pParent ) :
		m_aFL_DefaultTextDirection	( _pParent,	SVX_RES( FL_DEFTXTDIRECTION ) )
		,m_aRB_TxtDirLeft2Right		( _pParent,	SVX_RES( RB_TXTDIR_LEFT2RIGHT ) )
		,m_aRB_TxtDirRight2Left		( _pParent, SVX_RES( RB_TXTDIR_RIGHT2LEFT ) )
		,m_aFL_SheetView			( _pParent, SVX_RES( FL_SHEETVIEW ) )
		,m_aCB_ShtVwRight2Left		( _pParent, SVX_RES( CB_SHTVW_RIGHT2LEFT ) )
		,m_aCB_ShtVwCurrentDocOnly	( _pParent, SVX_RES( CB_SHTVW_CURRENTDOCONLY ) )

		,m_bEnable_SheetView_Opt	( FALSE )
	{
		ShowOption_SheetView( m_bEnable_SheetView_Opt );
	}

	inline void InternationalOptionsPage::IMPL::EnableOption_SheetView( BOOL _bEnable )
	{
		if( m_bEnable_SheetView_Opt != _bEnable )
		{
			ShowOption_SheetView( _bEnable );

			m_bEnable_SheetView_Opt = _bEnable;
		}
	}

	void InternationalOptionsPage::IMPL::ShowOption_SheetView( BOOL _bShow )
	{
		m_aFL_SheetView.Show( _bShow );
		m_aCB_ShtVwRight2Left.Show( _bShow );
		m_aCB_ShtVwCurrentDocOnly.Show( _bShow );
	}

	BOOL InternationalOptionsPage::IMPL::FillItemSet( SfxItemSet& _rSet )
	{
		DBG_ASSERT( _rSet.GetPool(), "-InternationalOptionsPage::FillItemSet(): no pool gives rums!" );

		// handling of DefaultTextDirection stuff
		_rSet.Put(	SfxBoolItem(	_rSet.GetPool()->GetWhich( SID_ATTR_PARA_LEFT_TO_RIGHT ),
									m_aRB_TxtDirLeft2Right.IsChecked() ),
					SID_ATTR_PARA_LEFT_TO_RIGHT );

		// handling of SheetView stuff
//		if( m_bEnable_SheetView_Opt )
//		{
//		}

		return TRUE;
	}

	void InternationalOptionsPage::IMPL::Reset( const SfxItemSet& _rSet )
	{
		// handling of DefaultTextDirection stuff
		const SfxBoolItem*	pLeft2RightItem = static_cast< const SfxBoolItem* >( GetItem( _rSet, SID_ATTR_PARA_LEFT_TO_RIGHT ) );

		DBG_ASSERT( pLeft2RightItem, "+InternationalOptionsPage::Reset(): SID_ATTR_PARA_LEFT_TO_RIGHT not set!" );

		BOOL				bLeft2Right = pLeft2RightItem? pLeft2RightItem->GetValue() : TRUE;
		m_aRB_TxtDirLeft2Right.Check( bLeft2Right );

		// handling of SheetView stuff
//		if( m_bEnable_SheetView_Opt )
//		{
//			m_aCB_ShtVwRight2Left.Check( FALSE );
//
//			m_aCB_ShtVwCurrentDocOnly.Check( FALSE );
//		}
	}

	InternationalOptionsPage::InternationalOptionsPage( Window* _pParent, const SfxItemSet& _rAttrSet ) :
		SfxTabPage	( _pParent, SVX_RES( RID_OFA_TP_INTERNATIONAL ), _rAttrSet )

		,m_pImpl	( new IMPL( this ) )
	{
		FreeResource();
	}

	SfxTabPage* InternationalOptionsPage::CreateSd( Window* _pParent, const SfxItemSet& _rAttrSet )
	{
		return new InternationalOptionsPage( _pParent, _rAttrSet );
	}

	SfxTabPage* InternationalOptionsPage::CreateSc( Window* _pParent, const SfxItemSet& _rAttrSet )
	{
		InternationalOptionsPage*	p = new InternationalOptionsPage( _pParent, _rAttrSet );
//		p->m_pImpl->EnableOption_SheetView();
		return p;
	}

	InternationalOptionsPage::~InternationalOptionsPage()
	{
		DELETEZ( m_pImpl );
	}

	BOOL InternationalOptionsPage::FillItemSet( SfxItemSet& _rSet )
	{
		return m_pImpl->FillItemSet( _rSet );
	}

	void InternationalOptionsPage::Reset( const SfxItemSet& _rSet )
	{
		m_pImpl->Reset( _rSet );
	}

}	// /namespace offapp

