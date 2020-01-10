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
#include "precompiled_svtools.hxx"

#include <svtools/intitem.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <tools/bigint.hxx>
#include <tools/stream.hxx>
#include <svtools/metitem.hxx>

//============================================================================
//
//  class SfxByteItem
//
//============================================================================

TYPEINIT1_AUTOFACTORY(SfxByteItem, CntByteItem);

//============================================================================
// virtual
SfxPoolItem * SfxByteItem::Create(SvStream & rStream, USHORT) const
{
	short nValue = 0;
	rStream >> nValue;
	return new SfxByteItem(Which(), BYTE(nValue));
}

//============================================================================
//
//  class SfxInt16Item
//
//============================================================================

DBG_NAME(SfxInt16Item);

//============================================================================
TYPEINIT1_AUTOFACTORY(SfxInt16Item, SfxPoolItem);

//============================================================================
SfxInt16Item::SfxInt16Item(USHORT which, SvStream & rStream):
	SfxPoolItem(which)
{
	DBG_CTOR(SfxInt16Item, 0);
	short nTheValue = 0;
	rStream >> nTheValue;
	m_nValue = nTheValue;
}

//============================================================================
// virtual
int SfxInt16Item::operator ==(const SfxPoolItem & rItem) const
{
	DBG_CHKTHIS(SfxInt16Item, 0);
	DBG_ASSERT(SfxPoolItem::operator ==(rItem), "unequal type");
	return m_nValue == SAL_STATIC_CAST(const SfxInt16Item *, &rItem)->
	                    m_nValue;
}

//============================================================================
// virtual
int SfxInt16Item::Compare(const SfxPoolItem & rWith) const
{
	DBG_CHKTHIS(SfxInt16Item, 0);
	DBG_ASSERT(SfxPoolItem::operator ==(rWith), "unequal type");
	return SAL_STATIC_CAST(const SfxInt16Item *, &rWith)->m_nValue
	         < m_nValue ?
            -1 :
	       SAL_STATIC_CAST(const SfxInt16Item *, &rWith)->m_nValue
	         == m_nValue ?
	        0 : 1;
}

//============================================================================
// virtual
SfxItemPresentation SfxInt16Item::GetPresentation(SfxItemPresentation,
												  SfxMapUnit, SfxMapUnit,
												  XubString & rText,
                                                  const IntlWrapper *) const
{
	DBG_CHKTHIS(SfxInt16Item, 0);
	rText = UniString::CreateFromInt32(m_nValue);
	return SFX_ITEM_PRESENTATION_NAMELESS;
}


//============================================================================
// virtual
BOOL SfxInt16Item::QueryValue(com::sun::star::uno::Any& rVal, BYTE) const
{
	sal_Int16 nValue = m_nValue;
	rVal <<= nValue;
	return TRUE;
}

//============================================================================
// virtual
BOOL SfxInt16Item::PutValue(const com::sun::star::uno::Any& rVal, BYTE )
{
	sal_Int16 nValue = sal_Int16();
	if (rVal >>= nValue)
	{
		m_nValue = nValue;
		return TRUE;
	}

	DBG_ERROR( "SfxInt16Item::PutValue - Wrong type!" );
	return FALSE;
}

//============================================================================
// virtual
SfxPoolItem * SfxInt16Item::Create(SvStream & rStream, USHORT) const
{
	DBG_CHKTHIS(SfxInt16Item, 0);
	return new SfxInt16Item(Which(), rStream);
}

//============================================================================
// virtual
SvStream & SfxInt16Item::Store(SvStream & rStream, USHORT) const
{
	DBG_CHKTHIS(SfxInt16Item, 0);
	rStream << short(m_nValue);
	return rStream;
}

//============================================================================
SfxPoolItem * SfxInt16Item::Clone(SfxItemPool *) const
{
	DBG_CHKTHIS(SfxInt16Item, 0);
	return new SfxInt16Item(*this);
}

//============================================================================
INT16 SfxInt16Item::GetMin() const
{
	DBG_CHKTHIS(SfxInt16Item, 0);
	return -32768;
}

//============================================================================
INT16 SfxInt16Item::GetMax() const
{
	DBG_CHKTHIS(SfxInt16Item, 0);
	return 32767;
}

//============================================================================
SfxFieldUnit SfxInt16Item::GetUnit() const
{
	DBG_CHKTHIS(SfxInt16Item, 0);
	return SFX_FUNIT_NONE;
}

//============================================================================
//
//  class SfxUInt16Item
//
//============================================================================

TYPEINIT1_AUTOFACTORY(SfxUInt16Item, CntUInt16Item);


//============================================================================
//
//  class SfxInt32Item
//
//============================================================================

TYPEINIT1_AUTOFACTORY(SfxInt32Item, CntInt32Item);


//============================================================================
//
//  class SfxUInt32Item
//
//============================================================================

TYPEINIT1_AUTOFACTORY(SfxUInt32Item, CntUInt32Item);


//============================================================================
//
//  class SfxMetricItem
//
//============================================================================

DBG_NAME(SfxMetricItem);

//============================================================================
TYPEINIT1_AUTOFACTORY(SfxMetricItem, SfxInt32Item);

//============================================================================
SfxMetricItem::SfxMetricItem(USHORT which, UINT32 nValue):
	SfxInt32Item(which, nValue)
{
	DBG_CTOR(SfxMetricItem, 0);
}

//============================================================================
SfxMetricItem::SfxMetricItem(USHORT which, SvStream & rStream):
	SfxInt32Item(which, rStream)
{
	DBG_CTOR(SfxMetricItem, 0);
}

//============================================================================
SfxMetricItem::SfxMetricItem(const SfxMetricItem & rItem):
	SfxInt32Item(rItem)
{
	DBG_CTOR(SfxMetricItem, 0);
}

//============================================================================
// virtual
int SfxMetricItem::ScaleMetrics(long nMult, long nDiv)
{
	BigInt aTheValue(GetValue());
	aTheValue *= nMult;
	aTheValue += nDiv / 2;
	aTheValue /= nDiv;
	SetValue(aTheValue);
	return 1;
}

//============================================================================
// virtual
int SfxMetricItem::HasMetrics() const
{
	return 1;
}

