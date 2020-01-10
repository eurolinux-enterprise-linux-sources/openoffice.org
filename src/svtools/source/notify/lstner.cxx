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
#ifndef GCC
#endif

#ifndef DEBUG_HXX
#include <tools/debug.hxx>
#endif

#include <svtools/hint.hxx>
#include <svtools/brdcst.hxx>

SV_DECL_PTRARR( SfxBroadcasterArr_Impl, SfxBroadcaster*, 0, 2 )

#define _SFX_LSTNER_CXX
#include <svtools/lstner.hxx>

//====================================================================
DBG_NAME(SfxListener)
TYPEINIT0(SfxListener);

//====================================================================
// simple ctor of class SfxListener

SfxListener::SfxListener()
{
	DBG_CTOR(SfxListener, 0);
}
//--------------------------------------------------------------------

// copy ctor of class SfxListener

SfxListener::SfxListener( const SfxListener &rListener )
{
	DBG_CTOR(SfxListener, 0);

	for ( USHORT n = 0; n < rListener.aBCs.Count(); ++n )
		StartListening( *rListener.aBCs[n] );
}
//--------------------------------------------------------------------

// unregisteres the SfxListener from its SfxBroadcasters

SfxListener::~SfxListener()
{
	DBG_DTOR(SfxListener, 0);

	// unregister at all remainding broadcasters
	for ( USHORT nPos = 0; nPos < aBCs.Count(); ++nPos )
	{
		SfxBroadcaster *pBC = aBCs[nPos];
		pBC->RemoveListener(*this);
	}
}

//--------------------------------------------------------------------

// unregisteres at a specific SfxBroadcaster

void SfxListener::RemoveBroadcaster_Impl( SfxBroadcaster& rBC )
{
	DBG_CHKTHIS(SfxListener, 0);

	const SfxBroadcaster *pBC = &rBC;
	aBCs.Remove( aBCs.GetPos(pBC), 1 );
}

//--------------------------------------------------------------------

// registeres at a specific SfxBroadcaster

BOOL SfxListener::StartListening( SfxBroadcaster& rBroadcaster, BOOL bPreventDups )
{
	DBG_CHKTHIS(SfxListener, 0);

	if ( !bPreventDups || !IsListening( rBroadcaster ) )
	{
		if ( rBroadcaster.AddListener(*this) )
		{
			const SfxBroadcaster *pBC = &rBroadcaster;
			aBCs.Insert( pBC, aBCs.Count() );

			DBG_ASSERT( IsListening(rBroadcaster), "StartListening failed" );
			return TRUE;
		}

	}
	return FALSE;
}

//--------------------------------------------------------------------

// unregisteres at a specific SfxBroadcaster

BOOL SfxListener::EndListening( SfxBroadcaster& rBroadcaster, BOOL bAllDups )
{
	DBG_CHKTHIS(SfxListener, 0);

	if ( !IsListening( rBroadcaster ) )
		return FALSE;

	do
	{
		rBroadcaster.RemoveListener(*this);
		const SfxBroadcaster *pBC = &rBroadcaster;
		aBCs.Remove( aBCs.GetPos(pBC), 1 );
	}
	while ( bAllDups && IsListening( rBroadcaster ) );
	return TRUE;
}

//--------------------------------------------------------------------

// unregisteres at a specific SfxBroadcaster by index

void SfxListener::EndListening( USHORT nNo )
{
	DBG_CHKTHIS(SfxListener, 0);

	SfxBroadcaster *pBC = aBCs.GetObject(nNo);
	pBC->RemoveListener(*this);
	aBCs.Remove( nNo, 1 );
}

//--------------------------------------------------------------------

// unregisteres all Broadcasters

void SfxListener::EndListeningAll()
{
	DBG_CHKTHIS(SfxListener, 0);

	// MI: bei Optimierung beachten: Seiteneffekte von RemoveListener beachten!
	while ( aBCs.Count() )
	{
		SfxBroadcaster *pBC = aBCs.GetObject(0);
		pBC->RemoveListener(*this);
		aBCs.Remove( 0, 1 );
	}
}

//--------------------------------------------------------------------

BOOL SfxListener::IsListening( SfxBroadcaster& rBroadcaster ) const
{
	const SfxBroadcaster *pBC = &rBroadcaster;
	return USHRT_MAX != aBCs.GetPos( pBC );
}

//--------------------------------------------------------------------

// base implementation of notification handler

#ifdef DBG_UTIL
void SfxListener::Notify( SfxBroadcaster& rBC, const SfxHint& )
#else
void SfxListener::Notify( SfxBroadcaster&, const SfxHint& )
#endif
{
    #ifdef DBG_UTIL
	const SfxBroadcaster *pBC = &rBC;
	DBG_ASSERT( USHRT_MAX != aBCs.GetPos(pBC),
				"notification from unregistered broadcaster" );
    #endif
}

