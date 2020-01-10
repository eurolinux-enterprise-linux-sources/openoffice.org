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
#include "precompiled_sd.hxx"


#include "sdattr.hxx"

using namespace ::com::sun::star;

/*************************************************************************
|*
|*	DiaEffectItem
|*
*************************************************************************/
TYPEINIT1_AUTOFACTORY( DiaEffectItem, SfxEnumItem );


DiaEffectItem::DiaEffectItem( presentation::FadeEffect eFE ) :
	SfxEnumItem( ATTR_DIA_EFFECT, (USHORT)eFE )
{
}


DiaEffectItem::DiaEffectItem( SvStream& rIn ) :
	SfxEnumItem( ATTR_DIA_EFFECT, rIn )
{
}


SfxPoolItem* DiaEffectItem::Clone( SfxItemPool* ) const
{
	return new DiaEffectItem( *this );
}


SfxPoolItem* DiaEffectItem::Create( SvStream& rIn, USHORT ) const
{
	return new DiaEffectItem( rIn );
}

/*************************************************************************
|*
|*	DiaSpeedItem
|*
*************************************************************************/
TYPEINIT1_AUTOFACTORY( DiaSpeedItem, SfxEnumItem );


DiaSpeedItem::DiaSpeedItem( FadeSpeed eFS ) :
	SfxEnumItem( ATTR_DIA_SPEED, (USHORT)eFS )
{
}


DiaSpeedItem::DiaSpeedItem( SvStream& rIn ) :
	SfxEnumItem( ATTR_DIA_SPEED, rIn )
{
}


SfxPoolItem* DiaSpeedItem::Clone( SfxItemPool* ) const
{
	return new DiaSpeedItem( *this );
}


SfxPoolItem* DiaSpeedItem::Create( SvStream& rIn, USHORT ) const
{
	return new DiaSpeedItem( rIn );
}

/*************************************************************************
|*
|*	DiaAutoItem
|*
*************************************************************************/
TYPEINIT1_AUTOFACTORY( DiaAutoItem, SfxEnumItem );

DiaAutoItem::DiaAutoItem( PresChange eChange ) :
	SfxEnumItem( ATTR_DIA_AUTO, (USHORT)eChange )
{
}


DiaAutoItem::DiaAutoItem( SvStream& rIn ) :
	SfxEnumItem( ATTR_DIA_AUTO, rIn )
{
}


SfxPoolItem* DiaAutoItem::Clone( SfxItemPool* ) const
{
	return new DiaAutoItem( *this );
}


SfxPoolItem* DiaAutoItem::Create( SvStream& rIn, USHORT ) const
{
	return new DiaAutoItem( rIn );
}

/*************************************************************************
|*
|*	DiaTimeItem
|*
*************************************************************************/
TYPEINIT1_AUTOFACTORY( DiaTimeItem, SfxUInt32Item );


DiaTimeItem::DiaTimeItem( UINT32 nValue ) :
		SfxUInt32Item( ATTR_DIA_TIME, nValue )
{
}


SfxPoolItem* DiaTimeItem::Clone( SfxItemPool* ) const
{
	return new DiaTimeItem( *this );
}


int DiaTimeItem::operator==( const SfxPoolItem& rItem ) const
{
	return( ( (DiaTimeItem&) rItem ).GetValue() == GetValue() );
}



