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
#ifndef _SV_MULTISEL_HXX
#define _SV_MULTISEL_HXX

#include "tools/toolsdllapi.h"
#include <tools/gen.hxx>
#include <tools/list.hxx>
#include <tools/string.hxx>

//------------------------------------------------------------------

#ifdef _SV_MULTISEL_CXX
DECLARE_LIST( ImpSelList, Range* )
#else
#define ImpSelList List
#endif

#define SFX_ENDOFSELECTION		CONTAINER_ENTRY_NOTFOUND

//------------------------------------------------------------------

// ------------------
// - MultiSelection -
// ------------------

class TOOLS_DLLPUBLIC MultiSelection
{
private:
	ImpSelList		aSels;		// array of SV-selections
	Range			aTotRange;	// total range of indexes
	ULONG			nCurSubSel; // index in aSels of current selected index
	long			nCurIndex;	// current selected entry
	ULONG			nSelCount;	// number of selected indexes
	BOOL			bInverseCur;// inverse cursor
	BOOL			bCurValid;	// are nCurIndex and nCurSubSel valid
	BOOL			bSelectNew; // auto-select newly inserted indexes

#ifdef _SV_MULTISEL_CXX
	TOOLS_DLLPRIVATE void			ImplClear();
	TOOLS_DLLPRIVATE ULONG			ImplFindSubSelection( long nIndex ) const;
	TOOLS_DLLPRIVATE BOOL			ImplMergeSubSelections( ULONG nPos1, ULONG nPos2 );
	TOOLS_DLLPRIVATE long			ImplFwdUnselected();
	TOOLS_DLLPRIVATE long			ImplBwdUnselected();
#endif

public:
					MultiSelection();
					MultiSelection( const MultiSelection& rOrig );
					MultiSelection( const Range& rRange );
					MultiSelection( const UniString& rString,
									sal_Unicode cRange = '-',
									sal_Unicode cSep = ';' );
					~MultiSelection();

	MultiSelection& operator= ( const MultiSelection& rOrig );
	BOOL			operator== ( MultiSelection& rOrig );
	BOOL			operator!= ( MultiSelection& rOrig )
						{ return !operator==( rOrig ); }
	BOOL			operator !() const
						{ return nSelCount == 0; }

	void			SelectAll( BOOL bSelect = TRUE );
	BOOL			Select( long nIndex, BOOL bSelect = TRUE );
	void			Select( const Range& rIndexRange, BOOL bSelect = TRUE );
	BOOL			IsSelected( long nIndex ) const;
	BOOL			IsAllSelected() const
						{ return nSelCount == ULONG(aTotRange.Len()); }
	long			GetSelectCount() const { return nSelCount; }

	void			SetTotalRange( const Range& rTotRange );
	void			Insert( long nIndex, long nCount = 1 );
	void			Remove( long nIndex );
	void			Append( long nCount = 1 );

	const Range&	GetTotalRange() const { return aTotRange; }
	BOOL			IsCurValid() const { return bCurValid; }
	long			GetCurSelected() const { return nCurIndex; }
	long			FirstSelected( BOOL bInverse = FALSE );
	long			LastSelected();
	long			NextSelected();
	long			PrevSelected();

	ULONG			GetRangeCount() const { return aSels.Count(); }
	const Range&	GetRange( ULONG nRange ) const { return *(const Range*)aSels.GetObject(nRange); }
};

#endif	// _SV_MULTISEL_HXX
