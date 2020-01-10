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

#ifndef _SVXE3DITEM_HXX
#define _SVXE3DITEM_HXX

#include <svtools/poolitem.hxx>
#include <basegfx/vector/b3dvector.hxx>
#include "svx/svxdllapi.h"

#ifndef _SVXVECT3DITEM_HXX
#define _SVXVECT3DITEM_HXX

class SvStream;

class SVX_DLLPUBLIC SvxB3DVectorItem : public SfxPoolItem
{
	basegfx::B3DVector	aVal;

public:
							TYPEINFO();
							SvxB3DVectorItem();
							SvxB3DVectorItem( USHORT nWhich, const basegfx::B3DVector& rVal );
							SvxB3DVectorItem( USHORT nWhich, SvStream & );
							SvxB3DVectorItem( const SvxB3DVectorItem& );
							~SvxB3DVectorItem();

	virtual int				operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*	Clone( SfxItemPool *pPool = 0 ) const;
	virtual SfxPoolItem*	Create(SvStream &, USHORT nVersion) const;
	virtual SvStream&		Store(SvStream &, USHORT nItemVersion ) const;

	virtual	sal_Bool        	 QueryValue( com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 ) const;
	virtual	sal_Bool			 PutValue( const com::sun::star::uno::Any& rVal, BYTE nMemberId = 0 );

	const basegfx::B3DVector&			GetValue() const { return aVal; }
			void			SetValue( const basegfx::B3DVector& rNewVal ) {
								 DBG_ASSERT( GetRefCount() == 0, "SetValue() with pooled item" );
								 aVal = rNewVal;
							 }

	virtual USHORT GetVersion (USHORT nFileFormatVersion) const;
};

#endif

#endif


