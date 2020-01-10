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
#include "precompiled_sfx2.hxx"

#ifndef GCC
#endif

#include "arrdecl.hxx"
#include <sfx2/hintpost.hxx>
#include <sfx2/app.hxx>
#include "sfxtypes.hxx"

//====================================================================

void SfxHintPoster::RegisterEvent()
{
	DBG_MEMTEST();
}

//--------------------------------------------------------------------

SfxHintPoster::SfxHintPoster()
{
	RegisterEvent();
}

//--------------------------------------------------------------------

SfxHintPoster::SfxHintPoster( const GenLink& rLink ):
	aLink(rLink)
{
}


//--------------------------------------------------------------------

SfxHintPoster::~SfxHintPoster()
{
}

//--------------------------------------------------------------------

void SfxHintPoster::Post( SfxHint* pHintToPost )
{
    GetpApp()->PostUserEvent( ( LINK(this, SfxHintPoster, DoEvent_Impl) ), pHintToPost );
	AddRef();
}

//--------------------------------------------------------------------

IMPL_LINK_INLINE_START( SfxHintPoster, DoEvent_Impl, SfxHint *, pPostedHint )
{
	DBG_MEMTEST();
	Event( pPostedHint );
	ReleaseRef();
	return 0;
}
IMPL_LINK_INLINE_END( SfxHintPoster, DoEvent_Impl, SfxHint *, pPostedHint )

//--------------------------------------------------------------------

void SfxHintPoster::Event( SfxHint* pPostedHint )
{
	aLink.Call( pPostedHint );
}

//--------------------------------------------------------------------

void SfxHintPoster::SetEventHdl( const GenLink& rLink )
{
	DBG_MEMTEST();
	aLink = rLink;
}


#define LOG( x )
#if 0
#define LOG( x )												\
{																\
	SvFileStream aStrm( "f:\\temp\\log", STREAM_READWRITE );	\
	aStrm.Seek( STREAM_SEEK_TO_END );							\
	aStrm << x.GetStr() << '\n';								\
}
#endif
