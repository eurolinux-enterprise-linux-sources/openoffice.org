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

#include "dbregistersettings.hxx"

//........................................................................
namespace svx
{
//........................................................................

	//====================================================================
	//= DatabaseMapItem
	//====================================================================
	TYPEINIT1( DatabaseMapItem, SfxPoolItem )
	//--------------------------------------------------------------------
	DatabaseMapItem::DatabaseMapItem( sal_uInt16 _nId, const TNameLocationMap& _rSettings )
		:SfxPoolItem(_nId)
		,m_aSettings(_rSettings)
	{
	}

	//--------------------------------------------------------------------
	int DatabaseMapItem::operator==( const SfxPoolItem& _rCompare ) const
	{
		const DatabaseMapItem* pItem = PTR_CAST(DatabaseMapItem, &_rCompare);
		if (!pItem)
			return sal_False;

		if (m_aSettings.size() != pItem->m_aSettings.size())
			return sal_False;

		return m_aSettings != pItem->m_aSettings;
	}

	//--------------------------------------------------------------------
    SfxPoolItem* DatabaseMapItem::Clone( SfxItemPool * ) const
	{
		return new DatabaseMapItem(Which(), m_aSettings);
	}

	//--------------------------------------------------------------------

//........................................................................
}	// namespace svx
//........................................................................


