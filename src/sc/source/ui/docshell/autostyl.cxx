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
#include "precompiled_sc.hxx"



// INCLUDE ---------------------------------------------------------------

#include <time.h>
#include "autostyl.hxx"

#include "docsh.hxx"
#include "attrib.hxx"
#include "sc.hrc"

//==================================================================

struct ScAutoStyleInitData
{
	ScRange	aRange;
	String	aStyle1;
	ULONG	nTimeout;
	String	aStyle2;

	ScAutoStyleInitData( const ScRange& rR, const String& rSt1, ULONG nT, const String& rSt2 ) :
		aRange(rR), aStyle1(rSt1), nTimeout(nT), aStyle2(rSt2) {}
};

struct ScAutoStyleData
{
	ULONG	nTimeout;
	ScRange	aRange;
	String	aStyle;

	ScAutoStyleData( ULONG nT, const ScRange& rR, const String& rT ) :
		nTimeout(nT), aRange(rR), aStyle(rT) {}
};

//==================================================================

inline ULONG TimeNow()			// Sekunden
{
	return (ULONG) time(0);
}

//==================================================================

ScAutoStyleList::ScAutoStyleList(ScDocShell* pShell) :
	pDocSh( pShell )
{
	aTimer.SetTimeoutHdl( LINK( this, ScAutoStyleList, TimerHdl ) );
	aInitTimer.SetTimeoutHdl( LINK( this, ScAutoStyleList, InitHdl ) );
	aInitTimer.SetTimeout( 0 );
}

ScAutoStyleList::~ScAutoStyleList()
{
	ULONG i;
	ULONG nCount = aEntries.Count();
	for (i=0; i<nCount; i++)
		delete (ScAutoStyleData*) aEntries.GetObject(i);
	nCount = aInitials.Count();
	for (i=0; i<nCount; i++)
		delete (ScAutoStyleInitData*) aInitials.GetObject(i);
}

//==================================================================

//	initial short delay (asynchronous call)

void ScAutoStyleList::AddInitial( const ScRange& rRange, const String& rStyle1,
									ULONG nTimeout, const String& rStyle2 )
{
	ScAutoStyleInitData* pNew =
		new ScAutoStyleInitData( rRange, rStyle1, nTimeout, rStyle2 );
	aInitials.Insert( pNew, aInitials.Count() );
	aInitTimer.Start();
}

IMPL_LINK( ScAutoStyleList, InitHdl, Timer*, EMPTYARG )
{
	ULONG nCount = aInitials.Count();
	for (ULONG i=0; i<nCount; i++)
	{
		ScAutoStyleInitData* pData = (ScAutoStyleInitData*) aInitials.GetObject(i);

		//	apply first style immediately
		pDocSh->DoAutoStyle( pData->aRange, pData->aStyle1 );

		//	add second style to list
		if ( pData->nTimeout )
			AddEntry( pData->nTimeout, pData->aRange, pData->aStyle2 );

		delete pData;
	}
	aInitials.Clear();

	return 0;
}

//==================================================================

void ScAutoStyleList::AddEntry( ULONG nTimeout, const ScRange& rRange, const String& rStyle )
{
	aTimer.Stop();
	ULONG nNow = TimeNow();

	//	alten Eintrag loeschen

	ULONG nCount = aEntries.Count();
	ULONG i;
	for (i=0; i<nCount; i++)
	{
		ScAutoStyleData* pData = (ScAutoStyleData*) aEntries.GetObject(i);
		if (pData->aRange == rRange)
		{
			delete pData;
			aEntries.Remove(i);
			--nCount;
			break;						// nicht weitersuchen - es kann nur einen geben
		}
	}

	//	Timeouts von allen Eintraegen anpassen

	if (nCount && nNow != nTimerStart)
	{
		DBG_ASSERT(nNow>nTimerStart, "Zeit laeuft rueckwaerts?");
		AdjustEntries((nNow-nTimerStart)*1000);
	}

	//	Einfuege-Position suchen

	ULONG nPos = LIST_APPEND;
	for (i=0; i<nCount && nPos == LIST_APPEND; i++)
		if (nTimeout <= ((ScAutoStyleData*) aEntries.GetObject(i))->nTimeout)
			nPos = i;

	ScAutoStyleData* pNew = new ScAutoStyleData( nTimeout, rRange, rStyle );
	aEntries.Insert( pNew, nPos );

	//	abgelaufene ausfuehren, Timer neu starten

	ExecuteEntries();
	StartTimer(nNow);
}

void ScAutoStyleList::AdjustEntries( ULONG nDiff )	// Millisekunden
{
	ULONG nCount = aEntries.Count();
	for (ULONG i=0; i<nCount; i++)
	{
		ScAutoStyleData* pData = (ScAutoStyleData*) aEntries.GetObject(i);
		if ( pData->nTimeout <= nDiff )
			pData->nTimeout = 0;					// abgelaufen
		else
			pData->nTimeout -= nDiff;				// weiterzaehlen
	}
}

void ScAutoStyleList::ExecuteEntries()
{
	ScAutoStyleData* pData;
    while ((pData = (ScAutoStyleData*) aEntries.GetObject(0)) != NULL && pData->nTimeout == 0)
	{
		pDocSh->DoAutoStyle( pData->aRange, pData->aStyle );	//! oder Request ???

		delete pData;
		aEntries.Remove((ULONG)0);
	}
}

void ScAutoStyleList::ExecuteAllNow()
{
	aTimer.Stop();

	ULONG nCount = aEntries.Count();
	for (ULONG i=0; i<nCount; i++)
	{
		ScAutoStyleData* pData = (ScAutoStyleData*) aEntries.GetObject(i);

		pDocSh->DoAutoStyle( pData->aRange, pData->aStyle );	//! oder Request ???

		delete pData;
	}
	aEntries.Clear();
}

void ScAutoStyleList::StartTimer( ULONG nNow )		// Sekunden
{
	// ersten Eintrag mit Timeout != 0 suchen

	ULONG nPos = 0;
	ScAutoStyleData* pData;
    while ( (pData = (ScAutoStyleData*) aEntries.GetObject(nPos)) != NULL && pData->nTimeout == 0 )
		++nPos;

	if (pData)
	{
		aTimer.SetTimeout( pData->nTimeout );
		aTimer.Start();
	}
	nTimerStart = nNow;
}

IMPL_LINK( ScAutoStyleList, TimerHdl, Timer*, EMPTYARG )
{
	ULONG nNow = TimeNow();
	AdjustEntries(aTimer.GetTimeout());				// eingestellte Wartezeit
	ExecuteEntries();
	StartTimer(nNow);

	return 0;
}




