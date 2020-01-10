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
#include <tools/debug.hxx>

#include <svtools/hint.hxx>
#include <svtools/smplhint.hxx>
#include <svtools/lstner.hxx>

SV_DECL_PTRARR( SfxListenerArr_Impl, SfxListener*, 0, 2 )

#define _SFX_BRDCST_CXX
#include <svtools/brdcst.hxx>

//====================================================================
DBG_NAME(SfxBroadcaster)
TYPEINIT0(SfxBroadcaster);

//====================================================================

//====================================================================
// broadcast immediately


void SfxBroadcaster::Broadcast( const SfxHint &rHint )
{
	DBG_CHKTHIS(SfxBroadcaster, 0);

	// is anybody to notify?
	if ( aListeners.Count() /*! || aGlobListeners.Count() */ )
	{
		// notify all registered listeners exactly once
		for ( USHORT n = 0; n < aListeners.Count(); ++n )
		{
			SfxListener* pListener = aListeners[n];
			if ( pListener )
				pListener->Notify( *this, rHint );
		}
	}
}

//--------------------------------------------------------------------

// broadcast after a timeout


void SfxBroadcaster::BroadcastDelayed( const SfxHint& rHint )
{
	DBG_WARNING( "not implemented" );
	Broadcast(rHint);
}
//--------------------------------------------------------------------

// broadcast in idle-handler

void SfxBroadcaster::BroadcastInIdle( const SfxHint& rHint )
{
	DBG_WARNING( "not implemented" );
	Broadcast(rHint);
}
//--------------------------------------------------------------------

// unregister all listeners

SfxBroadcaster::~SfxBroadcaster()
{
	DBG_DTOR(SfxBroadcaster, 0);

	Broadcast( SfxSimpleHint(SFX_HINT_DYING) );

	// remove all still registered listeners
	for ( USHORT nPos = 0; nPos < aListeners.Count(); ++nPos )
	{
		SfxListener *pListener = aListeners[nPos];
		if ( pListener )
			pListener->RemoveBroadcaster_Impl(*this);
	}
}

//--------------------------------------------------------------------

// simple ctor of class SfxBroadcaster

SfxBroadcaster::SfxBroadcaster()
{
	DBG_CTOR(SfxBroadcaster, 0);
}

//--------------------------------------------------------------------

// copy ctor of class SfxBroadcaster


SfxBroadcaster::SfxBroadcaster( const SfxBroadcaster &rBC )
{
	DBG_CTOR(SfxBroadcaster, 0);

	for ( USHORT n = 0; n < rBC.aListeners.Count(); ++n )
	{
		SfxListener *pListener = rBC.aListeners[n];
		if ( pListener )
			pListener->StartListening( *this );
	}
}

//--------------------------------------------------------------------

// add a new SfxListener to the list

BOOL SfxBroadcaster::AddListener( SfxListener& rListener )
{
	DBG_CHKTHIS(SfxBroadcaster, 0);
	const SfxListener *pListener = &rListener;
	const SfxListener *pNull = 0;
	USHORT nFreePos = aListeners.GetPos( pNull );
	if ( nFreePos < aListeners.Count() )
		aListeners.GetData()[nFreePos] = pListener;
	else if ( aListeners.Count() < (USHRT_MAX-1) )
		aListeners.Insert( pListener, aListeners.Count() );
	else
	{
		DBG_ERROR( "array overflow" );
		return FALSE;
	}

	DBG_ASSERT( USHRT_MAX != aListeners.GetPos(pListener),
				"AddListener failed" );
	return TRUE;
}

//--------------------------------------------------------------------

// called, if no more listeners exists

void SfxBroadcaster::ListenersGone()
{
	DBG_CHKTHIS(SfxBroadcaster,0);
}

//--------------------------------------------------------------------

// forward a notification to all registered listeners

void SfxBroadcaster::Forward(SfxBroadcaster& rBC, const SfxHint& rHint)
{
	const USHORT nCount = aListeners.Count();
	for ( USHORT i = 0; i < nCount; ++i )
	{
		SfxListener *pListener = aListeners[i];
		if ( pListener )
			pListener->Notify( rBC, rHint );
	}
}

//--------------------------------------------------------------------

// remove one SfxListener from the list

void SfxBroadcaster::RemoveListener( SfxListener& rListener )
{
	{DBG_CHKTHIS(SfxBroadcaster, 0);}
	const SfxListener *pListener = &rListener;
	USHORT nPos = aListeners.GetPos(pListener);
	DBG_ASSERT( nPos != USHRT_MAX, "RemoveListener: Listener unknown" );
	aListeners.GetData()[nPos] = 0;
	if ( !HasListeners() )
		ListenersGone();
}

//--------------------------------------------------------------------

BOOL SfxBroadcaster::HasListeners() const
{
	for ( USHORT n = 0; n < aListeners.Count(); ++n )
		if ( aListeners.GetObject(n) != 0 )
			return TRUE;
	return FALSE;
}

//--------------------------------------------------------------------
