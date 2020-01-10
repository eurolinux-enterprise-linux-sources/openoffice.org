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


#ifndef SC_DIF_HXX
#define SC_DIF_HXX

#include <tools/debug.hxx>
#include <tools/list.hxx>
#include <tools/string.hxx>
#include "global.hxx"
#include "address.hxx"


class SvStream;
class SvNumberFormatter;
class ScDocument;
class ScPatternAttr;

extern const sal_Unicode pKeyTABLE[];
extern const sal_Unicode pKeyVECTORS[];
extern const sal_Unicode pKeyTUPLES[];
extern const sal_Unicode pKeyDATA[];
extern const sal_Unicode pKeyBOT[];
extern const sal_Unicode pKeyEOD[];
extern const sal_Unicode pKeyTRUE[];
extern const sal_Unicode pKeyFALSE[];
extern const sal_Unicode pKeyNA[];
extern const sal_Unicode pKeyV[];
extern const sal_Unicode pKey1_0[];


enum TOPIC
{
	T_UNKNOWN,
	T_TABLE, T_VECTORS, T_TUPLES, T_DATA, T_LABEL, T_COMMENT, T_SIZE,
	T_PERIODICITY, T_MAJORSTART, T_MINORSTART, T_TRUELENGTH, T_UINITS,
	T_DISPLAYUNITS,
	T_END
};

enum DATASET { D_BOT, D_EOD, D_NUMERIC, D_STRING, D_UNKNOWN, D_SYNT_ERROR };


class DifParser
{
public:
	String			    aData;
	double				fVal;
	UINT32				nVector;
	UINT32				nVal;
	UINT32				nNumFormat;
	CharSet				eCharSet;
private:
	SvNumberFormatter*	pNumFormatter;
	SvStream&			rIn;
	BOOL				bPlain;
    String              aLookAheadLine;

    bool                ReadNextLine( String& rStr );
    bool                LookAhead();
    DATASET             GetNumberDataset( const sal_Unicode* pPossibleNumericData );
	static inline BOOL	IsBOT( const sal_Unicode* pRef );
	static inline BOOL	IsEOD( const sal_Unicode* pRef );
	static inline BOOL	Is1_0( const sal_Unicode* pRef );
public:
						DifParser( SvStream&, const UINT32 nOption, ScDocument&, CharSet );

	TOPIC				GetNextTopic( void );

	DATASET				GetNextDataset( void );

	const sal_Unicode*  ScanIntVal( const sal_Unicode* pStart, UINT32& rRet );
	BOOL				ScanFloatVal( const sal_Unicode* pStart );

	inline BOOL			IsNumber( const sal_Unicode cChar );
	inline BOOL			IsNumberEnding( const sal_Unicode cChar );

	static inline BOOL	IsV( const sal_Unicode* pRef );

	inline BOOL			IsPlain( void ) const;
};


inline BOOL	DifParser::IsBOT( const sal_Unicode* pRef )
{
	return	(	pRef[ 0 ] == pKeyBOT[0] &&
				pRef[ 1 ] == pKeyBOT[1] &&
				pRef[ 2 ] == pKeyBOT[2] &&
				pRef[ 3 ] == pKeyBOT[3]	);
}


inline BOOL	DifParser::IsEOD( const sal_Unicode* pRef )
{
	return	(	pRef[ 0 ] == pKeyEOD[0] &&
				pRef[ 1 ] == pKeyEOD[1] &&
				pRef[ 2 ] == pKeyEOD[2] &&
				pRef[ 3 ] == pKeyEOD[3]	);
}


inline BOOL	DifParser::Is1_0( const sal_Unicode* pRef )
{
	return	(	pRef[ 0 ] == pKey1_0[0] &&
				pRef[ 1 ] == pKey1_0[1] &&
				pRef[ 2 ] == pKey1_0[2] &&
				pRef[ 3 ] == pKey1_0[3]	);
}


inline BOOL	DifParser::IsV( const sal_Unicode* pRef )
{
	return	(	pRef[ 0 ] == pKeyV[0] &&
				pRef[ 1 ] == pKeyV[1]	);
}


inline BOOL DifParser::IsNumber( const sal_Unicode cChar )
{
	return ( cChar >= '0' && cChar <= '9' );
}


inline BOOL DifParser::IsNumberEnding( const sal_Unicode cChar )
{
	return ( cChar == 0x00 );
}


inline BOOL DifParser::IsPlain( void ) const
{
	return bPlain;
}




class DifAttrCache;
class ScPatternAttr;


class DifColumn : private List
{
private:
	friend class DifAttrCache;
	struct ENTRY
	{
		UINT32			nNumFormat;

		SCROW			nStart;
		SCROW			nEnd;
	};

	ENTRY*				pAkt;

	inline				DifColumn( void );
						~DifColumn();
	void				SetLogical( SCROW nRow );
	void				SetNumFormat( SCROW nRow, const UINT32 nNumFormat );
	void				NewEntry( const SCROW nPos, const UINT32 nNumFormat );
	void				Apply( ScDocument&, const SCCOL nCol, const SCTAB nTab, const ScPatternAttr& );
	void				Apply( ScDocument &rDoc, const SCCOL nCol, const SCTAB nTab );
public:		// geht niemanden etwas an...
};


inline DifColumn::DifColumn( void )
{
	pAkt = NULL;
}




class DifAttrCache
{
private:
	DifColumn**			ppCols;
	BOOL				bPlain;
public:
						DifAttrCache( const BOOL bPlain );
						~DifAttrCache();
	inline void			SetLogical( const SCCOL nCol, const SCROW nRow );
	void				SetNumFormat( const SCCOL nCol, const SCROW nRow, const UINT32 nNumFormat );
	void				Apply( ScDocument&, SCTAB nTab );
};


inline void DifAttrCache::SetLogical( const SCCOL nCol, const SCROW nRow )
{
	DBG_ASSERT( ValidCol(nCol), "-DifAttrCache::SetLogical(): Col zu gross!" );
	DBG_ASSERT( bPlain, "*DifAttrCache::SetLogical(): muss Plain sein!" );

	if( !ppCols[ nCol ] )
		ppCols[ nCol ] = new DifColumn;
	ppCols[ nCol ]->SetLogical( nRow );
}


#endif


