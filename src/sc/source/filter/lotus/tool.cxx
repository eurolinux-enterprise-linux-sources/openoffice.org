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



//------------------------------------------------------------------------

#include "scitems.hxx"
#include <svx/algitem.hxx>
#include <svtools/zforlist.hxx>
#include <tools/solar.h>

#include "cell.hxx"
#include "rangenam.hxx"
#include "compiler.hxx"

#include "tool.h"
#include "decl.h"
#include "root.hxx"
#include "lotrange.hxx"
#include "namebuff.hxx"
#include "ftools.hxx"

#include <math.h>

#ifdef _MSC_VER
#pragma optimize("",off)
#endif

//--------------------------------------------------------- EXTERNE VARIABLEN -
extern WKTYP				eTyp;			// -> filter.cxx, aktueller Dateityp
extern sal_Char*			pDummy2;		// -> memory.cxx
extern ScDocument*			pDoc;			// -> filter.cxx, Aufhaenger zum Dokumentzugriff
extern CharSet				eCharNach;		// -> filter.cxx, Zeichenkonvertierung von->nach

extern BOOL					bFormInit;		// -> memory.cxx, fuer GetFormHandle()

//--------------------------------------------------------- GLOBALE VARIABLEN -
BYTE						nDefaultFormat;	// -> op.cpp, Standard-Zellenformat

extern SvxHorJustifyItem	*pAttrRight, *pAttrLeft, *pAttrCenter, *pAttrRepeat, *pAttrStandard;
extern ScProtectionAttr*	pAttrUnprot;
extern SfxUInt32Item**		pAttrValForms;

SvxHorJustifyItem			*pAttrRight, *pAttrLeft, *pAttrCenter, *pAttrRepeat, *pAttrStandard;
													// -> in memory.cxx initialisiert
ScProtectionAttr*			pAttrUnprot;			// ->  " memory.cxx    "

extern FormCache*			pValueFormCache;		// -> in memory.cxx initialisiert
FormCache*					pValueFormCache;

SCCOL						LotusRangeList::nEingCol;
SCROW						LotusRangeList::nEingRow;




void PutFormString( SCCOL nCol, SCROW nRow, SCTAB nTab, sal_Char* pString )
{
	// Label-Format-Auswertung
	DBG_ASSERT( pString != NULL, "PutFormString(): pString == NULL" );

	sal_Char			cForm;
	SvxHorJustifyItem*	pJustify = NULL;

	cForm = *pString;

	switch( cForm )
	{
		case '"':   // rechtsbuendig
			pJustify = pAttrRight;
			pString++;
			break;
		case '\'':  // linksbuendig
			pJustify = pAttrLeft;
			pString++;
			break;
		case '^':   // zentriert
			pJustify = pAttrCenter;
			pString++;
			break;
		case '|':   // printer command
			pString = NULL;
			break;
		case '\\':  // Wiederholung
			pJustify = pAttrRepeat;
			pString++;
			break;
		default:    // kenn' ich nicht!
			pJustify = pAttrStandard;
	}

	if( pString )
	{
		pDoc->ApplyAttr( nCol, nRow, nTab, *pJustify );
		ScStringCell*	pZelle = new ScStringCell( String( pString, pLotusRoot->eCharsetQ ) );
		pDoc->PutCell( nCol, nRow, nTab, pZelle, ( BOOL ) TRUE );
	}
}




void SetFormat( SCCOL nCol, SCROW nRow, SCTAB nTab, BYTE nFormat, BYTE nSt )
{
	//  PREC:   nSt = Standard-Dezimalstellenanzahl
	pDoc->ApplyAttr( nCol, nRow, nTab, *( pValueFormCache->GetAttr( nFormat, nSt ) ) );

	ScProtectionAttr aAttr;

	aAttr.SetProtection( nFormat & 0x80 );

	pDoc->ApplyAttr( nCol, nRow, nTab, aAttr );
}

void InitPage( void )
{	// Seitenformat initialisieren, d.h. Default-Werte von SC holen
	//scGetPageFormat( 0, &aPage );
}


double SnumToDouble( INT16 nVal )
{
	const double pFacts[ 8 ] = {
		5000.0,
		500.0,
		0.05,
		0.005,
		0.0005,
		0.00005,
		0.0625,
		0.015625 };

	double		fVal;

	if( nVal & 0x0001 )
	{
		fVal = pFacts[ ( nVal >> 1 ) & 0x0007 ];
		fVal *= ( INT16 ) ( nVal >> 4 );
	}
	else
		fVal = ( INT16 ) ( nVal >> 1 );

	return fVal;
}

double Snum32ToDouble( UINT32 nValue )
{
	double fValue, temp;

	fValue = nValue >> 6;
	temp = nValue & 0x0f;
	if (temp)
	{
	    if (nValue & 0x00000010)
                fValue /= pow((double)10, temp);
	    else
		fValue *= pow((double)10, temp);
	}

	if ((nValue & 0x00000020))
		fValue = -fValue;
	return fValue;
}


FormCache::FormCache( ScDocument* pDoc1, BYTE nNewDefaultFormat )
{	// Default-Format ist 'Default'
	nDefaultFormat = nNewDefaultFormat;
    pFormTable = pDoc1->GetFormatTable();
	for( UINT16 nC = 0 ; nC < __nSize ; nC++ )
		bValid[ nC ] = FALSE;
	eLanguage = ScGlobal::eLnge;
}


FormCache::~FormCache()
{
	for( UINT16 nC = 0 ; nC < __nSize ; nC++ )
		delete aIdents[ nC ].GetAttr();
}


SfxUInt32Item* FormCache::NewAttr( BYTE nFormat, BYTE nSt )
{
	// neues Format erzeugen
	BYTE		nL, nH; // Low-/High-Nibble
	BYTE		nForm = nFormat;
	String		aFormString;
    const sal_Char* pFormString = 0;
	INT16		eType = NUMBERFORMAT_ALL;
    UINT32      nIndex1;
	UINT32		nHandle;
	BOOL		bDefault = FALSE;
	//void GenerateFormat( aFormString, eType, COUNTRY_SYSTEM, LANGUAGE_SYSTEM,
	//  BOOL bThousand, BOOL IsRed, UINT16 nPrecision, UINT16 nAnzLeading );

	if( nForm == 0xFF ) // Default-Format?
		nForm = nDefaultFormat;

	// Aufdroeseln in Low- und High-Nibble
	nL = nFormat & 0x0F;
	nH = ( nFormat & 0xF0 ) / 16;

	nH &= 0x07;     // Bits 4-6 'rausziehen
	switch( nH )
	{
		case 0x00:  // Festkommaformat (fixed)
			//fStandard;nL;
            nIndex1 = pFormTable->GetStandardFormat(
				NUMBERFORMAT_NUMBER, eLanguage );
            pFormTable->GenerateFormat( aFormString, nIndex1,
				eLanguage, FALSE, FALSE, nL, 1 );
			break;
		case 0x01:  // Exponentdarstellung (scientific notation)
			//fExponent;nL;
            nIndex1 = pFormTable->GetStandardFormat(
				NUMBERFORMAT_SCIENTIFIC, eLanguage );
            pFormTable->GenerateFormat( aFormString, nIndex1,
				eLanguage, FALSE, FALSE, nL, 1 );
			break;
		case 0x02:  // Waehrungsdarstellung (currency)
			//fMoney;nL;
            nIndex1 = pFormTable->GetStandardFormat(
				NUMBERFORMAT_CURRENCY, eLanguage );
            pFormTable->GenerateFormat( aFormString, nIndex1,
				eLanguage, FALSE, FALSE, nL, 1 );
			break;
		case 0x03:  // Prozent
			//fPercent;nL;
            nIndex1 = pFormTable->GetStandardFormat(
				NUMBERFORMAT_PERCENT, eLanguage );
            pFormTable->GenerateFormat( aFormString, nIndex1,
				eLanguage, FALSE, FALSE, nL, 1 );
			break;
		case 0x04:  // Komma
			//fStandard;nL;
            nIndex1 = pFormTable->GetStandardFormat(
				NUMBERFORMAT_NUMBER, eLanguage );
            pFormTable->GenerateFormat( aFormString, nIndex1,
				eLanguage, TRUE, FALSE, nL, 1 );
			break;
		case 0x05:  // frei
			//fStandard;nL;
            nIndex1 = pFormTable->GetStandardFormat(
				NUMBERFORMAT_NUMBER, eLanguage );
            pFormTable->GenerateFormat( aFormString, nIndex1,
				eLanguage, FALSE, FALSE, nL, 1 );
			break;
		case 0x06:  // frei
			//fStandard;nL;
            nIndex1 = pFormTable->GetStandardFormat(
				NUMBERFORMAT_NUMBER, eLanguage );
            pFormTable->GenerateFormat( aFormString, nIndex1,
				eLanguage, FALSE, FALSE, nL, 1 );
            nIndex1 = 0;
			break;
		case 0x07:  // Spezialformat
			switch( nL )
			{
				case 0x00:  // +/-
					//fStandard;nSt;
                    nIndex1 = pFormTable->GetStandardFormat(
						NUMBERFORMAT_NUMBER, eLanguage );
                    pFormTable->GenerateFormat( aFormString, nIndex1,
						eLanguage, FALSE, TRUE, nSt, 1 );
					break;
				case 0x01:  // generelles Format
					//fStandard;nSt;
                    nIndex1 = pFormTable->GetStandardFormat(
						NUMBERFORMAT_NUMBER, eLanguage );
                    pFormTable->GenerateFormat( aFormString, nIndex1,
						eLanguage, FALSE, FALSE, nSt, 1 );
					break;
				case 0x02:  // Datum: Tag, Monat, Jahr
					//fDate;dfDayMonthYearLong;
					eType = NUMBERFORMAT_DATE;
					pFormString = "TT.MM.JJJJ";
					break;
				case 0x03:  // Datum: Tag, Monat
					//fDate;dfDayMonthLong;
					eType = NUMBERFORMAT_DATE;
					pFormString = "TT.MMMM";
					break;
				case 0x04:  // Datum: Monat, Jahr
					//fDate;dfMonthYearLong;
					eType = NUMBERFORMAT_DATE;
					pFormString = "MM.JJJJ";
					break;
				case 0x05:  // Textformate
					//fString;nSt;
					eType = NUMBERFORMAT_TEXT;
					pFormString = "@";
					break;
				case 0x06:  // versteckt
					//wFlag |= paHideAll;bSetFormat = FALSE;
					eType = NUMBERFORMAT_NUMBER;
					pFormString = "";
					break;
				case 0x07:  // Time: hour, min, sec
					//fTime;tfHourMinSec24;
					eType = NUMBERFORMAT_TIME;
					pFormString = "HH:MM:SS";
					break;
				case 0x08:  // Time: hour, min
					//fTime;tfHourMin24;
					eType = NUMBERFORMAT_TIME;
					pFormString = "HH:MM";
					break;
				case 0x09:  // Date, intern INT32 1
					//fDate;dfDayMonthYearLong;
					eType = NUMBERFORMAT_DATE;
					pFormString = "TT.MM.JJJJ";
					break;
				case 0x0A:  // Date, intern INT32 2
					//fDate;dfDayMonthYearLong;
					eType = NUMBERFORMAT_DATE;
					pFormString = "TT.MM.JJJJ";
					break;
				case 0x0B:  // Time, intern INT32 1
					//fTime;tfHourMinSec24;
					eType = NUMBERFORMAT_TIME;
					pFormString = "HH:MM:SS";
					break;
				case 0x0C:  // Time, intern INT32 2
					//fTime;tfHourMinSec24;
					eType = NUMBERFORMAT_TIME;
					pFormString = "HH:MM:SS";
					break;
				case 0x0F:  // Standardeinstellung
					//fStandard;nSt;
					bDefault = TRUE;
					break;
				default:
					//fStandard;nSt;
					bDefault = TRUE;
					break;
			}
			break;
		default:
			//fStandard;nL;
            nIndex1 = pFormTable->GetStandardFormat(
				NUMBERFORMAT_NUMBER, eLanguage );
            pFormTable->GenerateFormat( aFormString, nIndex1,
				eLanguage, FALSE, FALSE, nL, 1 );
            nIndex1 = 0;
			break;
	}

	// Format in Table schieben
	if( bDefault )
		nHandle = 0;
	else
	{
		if( pFormString )
			aFormString.AssignAscii( pFormString );

		xub_StrLen	nDummy;
		pFormTable->PutEntry( aFormString, nDummy, eType, nHandle, eLanguage );
	}

	return new SfxUInt32Item( ATTR_VALUE_FORMAT, ( UINT32 ) nHandle );
}




void LotusRange::MakeHash( void )
{
	// 33222222222211111111110000000000
	// 10987654321098765432109876543210
	//                         ******** nColS
	//                   ********       nColE
	//     ****************             nRowS
	// ****************                 nRowE
	nHash =  static_cast<UINT32>(nColStart);
	nHash += static_cast<UINT32>(nColEnd) << 6;
	nHash += static_cast<UINT32>(nRowStart) << 12;
	nHash += static_cast<UINT32>(nRowEnd ) << 16;
}


LotusRange::LotusRange( SCCOL nCol, SCROW nRow )
{
	nColStart = nColEnd = nCol;
	nRowStart = nRowEnd = nRow;
	nId = ID_FAIL;
	MakeHash();
}


LotusRange::LotusRange( SCCOL nCS, SCROW nRS, SCCOL nCE, SCROW nRE )
{
	nColStart = nCS;
	nColEnd = nCE;
	nRowStart = nRS;
	nRowEnd = nRE;
	nId = ID_FAIL;
	MakeHash();
}


LotusRange::LotusRange( const LotusRange& rCpy )
{
	Copy( rCpy );
}





LotusRangeList::LotusRangeList( void )
{
	aComplRef.InitFlags();

	ScSingleRefData*	pSingRef;
	nIdCnt = 1;

	pSingRef = &aComplRef.Ref1;
	pSingRef->nTab = pSingRef->nRelTab = 0;
	pSingRef->SetColRel( FALSE );
	pSingRef->SetRowRel( FALSE );
	pSingRef->SetTabRel( TRUE );
	pSingRef->SetFlag3D( FALSE );

	pSingRef = &aComplRef.Ref2;
	pSingRef->nTab = pSingRef->nRelTab = 0;
	pSingRef->SetColRel( FALSE );
	pSingRef->SetRowRel( FALSE );
	pSingRef->SetTabRel( TRUE );
	pSingRef->SetFlag3D( FALSE );
}


LotusRangeList::~LotusRangeList( void )
	{
	LotusRange *pDel = ( LotusRange * ) List::First();

	while( pDel )
		{
		delete pDel;
		pDel = ( LotusRange * ) List::Next();
		}
	}


LR_ID LotusRangeList::GetIndex( const LotusRange &rRef )
{
	LotusRange*		pComp = ( LotusRange* ) List::First();

	while( pComp )
	{
		if( *pComp == rRef )
			return pComp->nId;
		pComp = ( LotusRange* ) List::Next();
	}

	return ID_FAIL;
}


void LotusRangeList::Append( LotusRange* pLR, const String& rName )
{
	DBG_ASSERT( pLR, "*LotusRangeList::Append(): das wird nichts!" );
	List::Insert( pLR, CONTAINER_APPEND );

	ScTokenArray	aTokArray;

	ScSingleRefData*	pSingRef = &aComplRef.Ref1;

	pSingRef->nCol = pLR->nColStart;
	pSingRef->nRow = pLR->nRowStart;

	if( pLR->IsSingle() )
		aTokArray.AddSingleReference( *pSingRef );
	else
	{
		pSingRef = &aComplRef.Ref2;
		pSingRef->nCol = pLR->nColEnd;
		pSingRef->nRow = pLR->nRowEnd;
		aTokArray.AddDoubleReference( aComplRef );
	}

	ScRangeData*	pData = new ScRangeData(
		pLotusRoot->pDoc, rName, aTokArray );

	pLotusRoot->pScRangeName->Insert( pData );

	pLR->SetId( nIdCnt );

	nIdCnt++;
}




RangeNameBufferWK3::RangeNameBufferWK3( void )
{
	pScTokenArray = new ScTokenArray;
	nIntCount = 1;
}


RangeNameBufferWK3::~RangeNameBufferWK3()
{
	ENTRY*		pDel = ( ENTRY* ) List::First();

	while( pDel )
	{
		delete pDel;
		pDel = ( ENTRY* ) List::Next();
	}

	delete pScTokenArray;
}


void RangeNameBufferWK3::Add( const String& rOrgName, const ScComplexRefData& rCRD )
{
	String				aScName( rOrgName );
    ScfTools::ConvertToScDefinedName( aScName );

	register ENTRY*		pInsert = new ENTRY( rOrgName, aScName, rCRD );

	List::Insert( pInsert, CONTAINER_APPEND );

	pScTokenArray->Clear();

	register const ScSingleRefData&	rRef1 = rCRD.Ref1;
	register const ScSingleRefData&	rRef2 = rCRD.Ref2;

	if( rRef1.nCol == rRef2.nCol && rRef1.nRow == rRef2.nRow && rRef1.nTab == rRef2.nTab )
	{
		pScTokenArray->AddSingleReference( rCRD.Ref1 );
		pInsert->bSingleRef = TRUE;
	}
	else
	{
		pScTokenArray->AddDoubleReference( rCRD );
		pInsert->bSingleRef = FALSE;
	}

	ScRangeData*		pData = new ScRangeData( pLotusRoot->pDoc, aScName, *pScTokenArray );

	pInsert->nRelInd = nIntCount;
	pData->SetIndex( nIntCount );
	nIntCount++;

	pLotusRoot->pScRangeName->Insert( pData );
}


BOOL RangeNameBufferWK3::FindRel( const String& rRef, UINT16& rIndex )
{
	StringHashEntry		aRef( rRef );

	ENTRY*				pFind = ( ENTRY* ) List::First();

	while( pFind )
	{
		if( aRef == pFind->aStrHashEntry )
		{
			rIndex = pFind->nRelInd;
			return TRUE;
		}
		pFind = ( ENTRY* ) List::Next();
	}

	return FALSE;
}


BOOL RangeNameBufferWK3::FindAbs( const String& rRef, UINT16& rIndex )
{
	String				aTmp( rRef );
	StringHashEntry		aRef( aTmp.Erase( 0, 1 ) );	// ohne '$' suchen!

	ENTRY*				pFind = ( ENTRY* ) List::First();

	while( pFind )
	{
		if( aRef == pFind->aStrHashEntry )
		{
			// eventuell neuen Range Name aufbauen
			if( pFind->nAbsInd )
				rIndex = pFind->nAbsInd;
			else
			{
				ScSingleRefData*		pRef = &pFind->aScComplexRefDataRel.Ref1;
				pScTokenArray->Clear();

				pRef->SetColRel( FALSE );
				pRef->SetRowRel( FALSE );
				pRef->SetTabRel( TRUE );

				if( pFind->bSingleRef )
					pScTokenArray->AddSingleReference( *pRef );
				else
				{
					pRef = &pFind->aScComplexRefDataRel.Ref2;
					pRef->SetColRel( FALSE );
					pRef->SetRowRel( FALSE );
					pRef->SetTabRel( TRUE );
					pScTokenArray->AddDoubleReference( pFind->aScComplexRefDataRel );
				}

				ScRangeData*	pData = new ScRangeData( pLotusRoot->pDoc, pFind->aScAbsName, *pScTokenArray );

				rIndex = pFind->nAbsInd = nIntCount;
				pData->SetIndex( rIndex );
				nIntCount++;

				pLotusRoot->pScRangeName->Insert( pData );
			}

			return TRUE;
		}
		pFind = ( ENTRY* ) List::Next();
	}

	return FALSE;
}


