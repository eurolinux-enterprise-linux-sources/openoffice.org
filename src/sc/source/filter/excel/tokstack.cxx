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


#ifndef PCH
#include <string.h>
#endif

#include "compiler.hxx"
#include "tokstack.hxx"
#include "global.hxx"
#include "scmatrix.hxx"

#include <stdio.h> // printf

const UINT16	TokenPool::nScTokenOff = 8192;


TokenStack::TokenStack( UINT16 nNewSize )
{
	pStack = new TokenId[ nNewSize ];

	Reset();
	nSize = nNewSize;
}


TokenStack::~TokenStack()
{
	delete[] pStack;
}




//------------------------------------------------------------------------

// !ACHTUNG!: nach Aussen hin beginnt die Nummerierung mit 1!
// !ACHTUNG!: SC-Token werden mit einem Offset nScTokenOff abgelegt
//				-> Unterscheidung von anderen Token


TokenPool::TokenPool( void )
{
	UINT16	nLauf = nScTokenOff;

	// Sammelstelle fuer Id-Folgen
	nP_Id = 256;
	pP_Id = new UINT16[ nP_Id ];

	// Sammelstelle fuer Ids
	nElement = 32;
	pElement = new UINT16[ nElement ];
	pType = new E_TYPE[ nElement ];
	pSize = new UINT16[ nElement ];
	nP_IdLast = 0;

	// Sammelstelle fuer Strings
	nP_Str = 4;
	ppP_Str = new String *[ nP_Str ];
	for( nLauf = 0 ; nLauf < nP_Str ; nLauf++ )
		ppP_Str[ nLauf ] = NULL;

	// Sammelstelle fuer double
	nP_Dbl = 8;
	pP_Dbl = new double[ nP_Dbl ];

    // Sammelstelle fuer error codes
    nP_Err = 8;
    pP_Err = new USHORT[ nP_Err ];

	// Sammelstellen fuer Referenzen
	nP_RefTr = 32;
	ppP_RefTr = new ScSingleRefData *[ nP_RefTr ];
	for( nLauf = 0 ; nLauf < nP_RefTr ; nLauf++ )
		ppP_RefTr[ nLauf ] = NULL;

	nP_Ext = 32;
	ppP_Ext = new EXTCONT*[ nP_Ext ];
	memset( ppP_Ext, 0, sizeof( EXTCONT* ) * nP_Ext );

    nP_Nlf = 16;
	ppP_Nlf = new NLFCONT*[ nP_Nlf ];
	memset( ppP_Nlf, 0, sizeof( NLFCONT* ) * nP_Nlf );

    nP_Matrix = 16;
    ppP_Matrix = new ScMatrix*[ nP_Matrix ];
    memset( ppP_Matrix, 0, sizeof( ScMatrix* ) * nP_Matrix );

	pScToken = new ScTokenArray;

	Reset();
}


TokenPool::~TokenPool()
{
	UINT16	n;

	delete[] pP_Id;
	delete[] pElement;
	delete[] pType;
	delete[] pSize;
	delete[] pP_Dbl;
    delete[] pP_Err;

	for( n = 0 ; n < nP_RefTr ; n++/*, pAktTr++*/ )
	{
		if( ppP_RefTr[ n ] )
			delete ppP_RefTr[ n ];
	}
	delete[] ppP_RefTr;

	for( n = 0 ; n < nP_Str ; n++/*, pAktStr++*/ )
	{
		if( ppP_Str[ n ] )
			delete ppP_Str[ n ];
	}
	delete[] ppP_Str;

	for( n = 0 ; n < nP_Ext ; n++ )
	{
		if( ppP_Ext[ n ] )
			delete ppP_Ext[ n ];
	}
	delete[] ppP_Ext;

	for( n = 0 ; n < nP_Nlf ; n++ )
	{
		if( ppP_Nlf[ n ] )
			delete ppP_Nlf[ n ];
	}
	delete[] ppP_Nlf;

    for( n = 0 ; n < nP_Matrix ; n++ )
    {
        if( ppP_Matrix[ n ] )
            ppP_Matrix[ n ]->DecRef( );
    }
    delete[] ppP_Matrix;

	delete pScToken;
}


void TokenPool::GrowString( void )
{
	UINT16		nP_StrNew = nP_Str * 2;
	UINT16		nL;

	String**	ppP_StrNew = new String *[ nP_StrNew ];

	for( nL = 0 ; nL < nP_Str ; nL++ )
		ppP_StrNew[ nL ] = ppP_Str[ nL ];
	for( nL = nP_Str ; nL < nP_StrNew ; nL++ )
		ppP_StrNew[ nL ] = NULL;

	nP_Str = nP_StrNew;

	delete[]	ppP_Str;
	ppP_Str = ppP_StrNew;
}


void TokenPool::GrowDouble( void )
{
	UINT16		nP_DblNew = nP_Dbl * 2;

	double*		pP_DblNew = new double[ nP_DblNew ];

	for( UINT16 nL = 0 ; nL < nP_Dbl ; nL++ )
		pP_DblNew[ nL ] = pP_Dbl[ nL ];

	nP_Dbl = nP_DblNew;

	delete[] pP_Dbl;
	pP_Dbl = pP_DblNew;
}


//UNUSED2009-05 void TokenPool::GrowError( void )
//UNUSED2009-05 {
//UNUSED2009-05     UINT16      nP_ErrNew = nP_Err * 2;
//UNUSED2009-05 
//UNUSED2009-05     USHORT*     pP_ErrNew = new USHORT[ nP_ErrNew ];
//UNUSED2009-05 
//UNUSED2009-05     for( UINT16 nL = 0 ; nL < nP_Err ; nL++ )
//UNUSED2009-05         pP_ErrNew[ nL ] = pP_Err[ nL ];
//UNUSED2009-05 
//UNUSED2009-05     nP_Err = nP_ErrNew;
//UNUSED2009-05 
//UNUSED2009-05     delete[] pP_Err;
//UNUSED2009-05     pP_Err = pP_ErrNew;
//UNUSED2009-05 }


void TokenPool::GrowTripel( void )
{
	UINT16			nP_RefTrNew = nP_RefTr * 2;
	UINT16			nL;

	ScSingleRefData**	ppP_RefTrNew = new ScSingleRefData *[ nP_RefTrNew ];

	for( nL = 0 ; nL < nP_RefTr ; nL++ )
		ppP_RefTrNew[ nL ] = ppP_RefTr[ nL ];
	for( nL = nP_RefTr ; nL < nP_RefTrNew ; nL++ )
		ppP_RefTrNew[ nL ] = NULL;

	nP_RefTr = nP_RefTrNew;

	delete[] ppP_RefTr;
	ppP_RefTr = ppP_RefTrNew;
}


void TokenPool::GrowId( void )
{
	UINT16	nP_IdNew = nP_Id * 2;

	UINT16*	pP_IdNew = new UINT16[ nP_IdNew ];

	for( UINT16 nL = 0 ; nL < nP_Id ; nL++ )
		pP_IdNew[ nL ] = pP_Id[ nL ];

	nP_Id = nP_IdNew;

	delete[] pP_Id;
	pP_Id = pP_IdNew;
}


void TokenPool::GrowElement( void )
{
	UINT16	nElementNew = nElement * 2;

	UINT16*	pElementNew = new UINT16[ nElementNew ];
	E_TYPE*	pTypeNew = new E_TYPE[ nElementNew ];
	UINT16*	pSizeNew = new UINT16[ nElementNew ];

	for( UINT16 nL = 0 ; nL < nElement ; nL++ )
	{
		pElementNew[ nL ] = pElement[ nL ];
		pTypeNew[ nL ] = pType[ nL ];
		pSizeNew[ nL ] = pSize[ nL ];
	}

	nElement = nElementNew;

	delete[] pElement;
	delete[] pType;
	delete[] pSize;
	pElement = pElementNew;
	pType = pTypeNew;
	pSize = pSizeNew;
}


void TokenPool::GrowExt( void )
{
	UINT16		nNewSize = nP_Ext * 2;

	EXTCONT**	ppNew = new EXTCONT*[ nNewSize ];

    memset( ppNew, 0, sizeof( EXTCONT* ) * nNewSize );
	memcpy( ppNew, ppP_Ext, sizeof( EXTCONT* ) * nP_Ext );

	delete[] ppP_Ext;
	ppP_Ext = ppNew;
	nP_Ext = nNewSize;
}


void TokenPool::GrowNlf( void )
{
	UINT16		nNewSize = nP_Nlf * 2;

	NLFCONT**	ppNew = new NLFCONT*[ nNewSize ];

    memset( ppNew, 0, sizeof( NLFCONT* ) * nNewSize );
	memcpy( ppNew, ppP_Nlf, sizeof( NLFCONT* ) * nP_Nlf );

	delete[] ppP_Nlf;
    ppP_Nlf = ppNew;
	nP_Nlf = nNewSize;
}


void TokenPool::GrowMatrix( void )
{
    UINT16      nNewSize = nP_Matrix * 2;

    ScMatrix**  ppNew = new ScMatrix*[ nNewSize ];

    memset( ppNew, 0, sizeof( ScMatrix* ) * nNewSize );
    memcpy( ppNew, ppP_Matrix, sizeof( ScMatrix* ) * nP_Matrix );

    delete[] ppP_Matrix;
    ppP_Matrix = ppNew;
    nP_Matrix = nNewSize;
}

void TokenPool::GetElement( const UINT16 nId )
{
	DBG_ASSERT( nId < nElementAkt, "*TokenPool::GetElement(): Id zu gross!?" );

	if( pType[ nId ] == T_Id )
		GetElementRek( nId );
	else
	{
		switch( pType[ nId ] )
		{
#ifdef DBG_UTIL
			case T_Id:
				DBG_ERROR( "-TokenPool::GetElement(): hier hast Du nichts zu suchen!" );
				break;
#endif
			case T_Str:
				pScToken->AddString( ppP_Str[ pElement[ nId ] ]->GetBuffer() );
				break;
			case T_D:
				pScToken->AddDouble( pP_Dbl[ pElement[ nId ] ] );
				break;
            case T_Err:
#if 0   // erAck
                pScToken->AddError( pP_Err[ pElement[ nId ] ] );
#endif
                break;
			case T_RefC:
				pScToken->AddSingleReference( *ppP_RefTr[ pElement[ (UINT16) nId ] ] );
				break;
			case T_RefA:
				{
				ScComplexRefData	aScComplexRefData;
				aScComplexRefData.Ref1 = *ppP_RefTr[ pElement[ nId ] ];
				aScComplexRefData.Ref2 = *ppP_RefTr[ pElement[ nId ] + 1 ];
				pScToken->AddDoubleReference( aScComplexRefData );
				}
				break;
			case T_RN:
				pScToken->AddName( pElement[ nId ] );
				break;
			case T_Ext:
				{
				UINT16			n = pElement[ nId ];
				EXTCONT*		p = ( n < nP_Ext )? ppP_Ext[ n ] : NULL;

				if( p )
                {
                    if( p->eId == ocEuroConvert )
                        pScToken->AddOpCode( p->eId );
                    else
						pScToken->AddExternal( p->aText, p->eId );
                }
				}
				break;
			case T_Nlf:
				{
				UINT16			n = pElement[ nId ];
				NLFCONT*		p = ( n < nP_Nlf )? ppP_Nlf[ n ] : NULL;

				if( p )
						pScToken->AddColRowName( p->aRef );
				}
				break;
            case T_Matrix:
                {
                UINT16          n = pElement[ nId ];
                ScMatrix*       p = ( n < nP_Matrix )? ppP_Matrix[ n ] : NULL;

                if( p )
                        pScToken->AddMatrix( p );
                }
                break;
            case T_ExtName:
            {
                UINT16 n = pElement[nId];
                if (n < maExtNames.size())
                {
                    const ExtName& r = maExtNames[n];
                    pScToken->AddExternalName(r.mnFileId, r.maName);
                }
            }
            case T_ExtRefC:
            {
                UINT16 n = pElement[nId];
                if (n < maExtCellRefs.size())
                {
                    const ExtCellRef& r = maExtCellRefs[n];
                    pScToken->AddExternalSingleReference(r.mnFileId, r.maTabName, r.maRef);
                }
            }
            case T_ExtRefA:
            {
                UINT16 n = pElement[nId];
                if (n < maExtAreaRefs.size())
                {
                    const ExtAreaRef& r = maExtAreaRefs[n];
                    pScToken->AddExternalDoubleReference(r.mnFileId, r.maTabName, r.maRef);
                }
            }
            break;
			default:
				DBG_ERROR("-TokenPool::GetElement(): Zustand undefiniert!?");
		}
	}
}


void TokenPool::GetElementRek( const UINT16 nId )
{
#ifdef DBG_UTIL
	nRek++;
	DBG_ASSERT( nRek <= nP_Id, "*TokenPool::GetElement(): Rekursion loopt!?" );
#endif

	DBG_ASSERT( nId < nElementAkt, "*TokenPool::GetElementRek(): Id zu gross!?" );

	DBG_ASSERT( pType[ nId ] == T_Id, "-TokenPool::GetElementRek(): nId nicht Id-Folge!" );


	UINT16		nAnz = pSize[ nId ];
	UINT16*		pAkt = &pP_Id[ pElement[ nId ] ];
	for( ; nAnz > 0 ; nAnz--, pAkt++ )
	{
		if( *pAkt < nScTokenOff )
		{// Rekursion oder nicht?
			switch( pType[ *pAkt ] )
			{
				case T_Id:
					GetElementRek( *pAkt );
					break;
				case T_Str:
					pScToken->AddString( ppP_Str[ pElement[ *pAkt ] ]->GetBuffer() );
					break;
				case T_D:
					pScToken->AddDouble( pP_Dbl[ pElement[ *pAkt ] ] );
					break;
                case T_Err:
#if 0   // erAck
                    pScToken->AddError( pP_Err[ pElement[ *pAkt ] ] );
#endif
                    break;
				case T_RefC:
					pScToken->AddSingleReference( *ppP_RefTr[ pElement[ *pAkt ] ] );
					break;
				case T_RefA:
					{
					ScComplexRefData	aScComplexRefData;
					aScComplexRefData.Ref1 = *ppP_RefTr[ pElement[ *pAkt ] ];
					aScComplexRefData.Ref2 = *ppP_RefTr[ pElement[ *pAkt ] + 1 ];
					pScToken->AddDoubleReference( aScComplexRefData );
					}
					break;
				case T_RN:
					pScToken->AddName( pElement[ *pAkt ] );
					break;
				case T_Ext:
					{
					UINT16		n = pElement[ *pAkt ];
					EXTCONT*	p = ( n < nP_Ext )? ppP_Ext[ n ] : NULL;

					if( p )
						pScToken->AddExternal( p->aText, p->eId );
					}
					break;
				case T_Nlf:
					{
					UINT16		n = pElement[ *pAkt ];
					NLFCONT*	p = ( n < nP_Nlf )? ppP_Nlf[ n ] : NULL;

					if( p )
						pScToken->AddColRowName( p->aRef );
					}
					break;
                case T_Matrix:
                    {
                    UINT16          n = pElement[ *pAkt ];
                    ScMatrix*       p = ( n < nP_Matrix )? ppP_Matrix[ n ] : NULL;

                    if( p )
                            pScToken->AddMatrix( p );
                    }
                    break;
                case T_ExtName:
                {
                    UINT16 n = pElement[*pAkt];
                    if (n < maExtNames.size())
                    {
                        const ExtName& r = maExtNames[n];
                        pScToken->AddExternalName(r.mnFileId, r.maName);
                    }
                }
                case T_ExtRefC:
                {
                    UINT16 n = pElement[*pAkt];
                    if (n < maExtCellRefs.size())
                    {
                        const ExtCellRef& r = maExtCellRefs[n];
                        pScToken->AddExternalSingleReference(r.mnFileId, r.maTabName, r.maRef);
                    }
                }
                case T_ExtRefA:
                {
                    UINT16 n = pElement[*pAkt];
                    if (n < maExtAreaRefs.size())
                    {
                        const ExtAreaRef& r = maExtAreaRefs[n];
                        pScToken->AddExternalDoubleReference(r.mnFileId, r.maTabName, r.maRef);
                    }
                }
                break;
				default:
					DBG_ERROR("-TokenPool::GetElementRek(): Zustand undefiniert!?");
			}
		}
		else	// elementarer SC_Token
			pScToken->AddOpCode( ( DefTokenId ) ( *pAkt - nScTokenOff ) );
	}


#ifdef DBG_UTIL
	nRek--;
#endif
}


void TokenPool::operator >>( TokenId& rId )
{
	rId = ( TokenId ) ( nElementAkt + 1 );

	if( nElementAkt >= nElement )
		GrowElement();

	pElement[ nElementAkt ] = nP_IdLast;	// Start der Token-Folge
	pType[ nElementAkt ] = T_Id;			// Typinfo eintragen
	pSize[ nElementAkt ] = nP_IdAkt - nP_IdLast;
		// von nP_IdLast bis nP_IdAkt-1 geschrieben -> Laenge der Folge

	nElementAkt++;			// Startwerte fuer naechste Folge
	nP_IdLast = nP_IdAkt;
}


const TokenId TokenPool::Store( const double& rDouble )
{
	if( nElementAkt >= nElement )
		GrowElement();

	if( nP_DblAkt >= nP_Dbl )
		GrowDouble();

	pElement[ nElementAkt ] = nP_DblAkt;	// Index in Double-Array
	pType[ nElementAkt ] = T_D;				// Typinfo Double eintragen

	pP_Dbl[ nP_DblAkt ] = rDouble;

	pSize[ nElementAkt ] = 1;			// eigentlich Banane

	nElementAkt++;
	nP_DblAkt++;

	return ( const TokenId ) nElementAkt; // Ausgabe von altem Wert + 1!
}


const TokenId TokenPool::Store( const UINT16 nIndex )
{
	if( nElementAkt >= nElement )
		GrowElement();

	pElement[ nElementAkt ] = nIndex;			// Daten direkt im Index!
	pType[ nElementAkt ] = T_RN;				// Typinfo Range Name eintragen

	nElementAkt++;
	return ( const TokenId ) nElementAkt;				// Ausgabe von altem Wert + 1!
}


const TokenId TokenPool::Store( const String& rString )
{
	// weitgehend nach Store( const sal_Char* ) kopiert, zur Vermeidung
	//  eines temporaeren Strings in "
	if( nElementAkt >= nElement )
		GrowElement();

	if( nP_StrAkt >= nP_Str )
		GrowString();

	pElement[ nElementAkt ] = nP_StrAkt;	// Index in String-Array
	pType[ nElementAkt ] = T_Str;			// Typinfo String eintragen

	// String anlegen
	if( !ppP_Str[ nP_StrAkt ] )
		//...aber nur, wenn noch nicht vorhanden
		ppP_Str[ nP_StrAkt ] = new String( rString );
	else
		//...ansonsten nur kopieren
		*ppP_Str[ nP_StrAkt ] = rString;

	DBG_ASSERT( sizeof( xub_StrLen ) <= 2, "*TokenPool::Store(): StrLen doesn't match!" );

	pSize[ nElementAkt ] = ( UINT16 ) ppP_Str[ nP_StrAkt ]->Len();

	nElementAkt++;
	nP_StrAkt++;

	return ( const TokenId ) nElementAkt; // Ausgabe von altem Wert + 1!
}


const TokenId TokenPool::Store( const ScSingleRefData& rTr )
{
	if( nElementAkt >= nElement )
		GrowElement();

	if( nP_RefTrAkt >= nP_RefTr )
		GrowTripel();

	pElement[ nElementAkt ] = nP_RefTrAkt;
	pType[ nElementAkt ] = T_RefC;			// Typinfo Cell-Reff eintragen

	if( !ppP_RefTr[ nP_RefTrAkt ] )
		ppP_RefTr[ nP_RefTrAkt ] = new ScSingleRefData( rTr );
	else
		*ppP_RefTr[ nP_RefTrAkt ] = rTr;

	nElementAkt++;
	nP_RefTrAkt++;

	return ( const TokenId ) nElementAkt; // Ausgabe von altem Wert + 1!
}


const TokenId TokenPool::Store( const ScComplexRefData& rTr )
{
	if( nElementAkt >= nElement )
		GrowElement();

	if( nP_RefTrAkt + 1 >= nP_RefTr )
		GrowTripel();

	pElement[ nElementAkt ] = nP_RefTrAkt;
	pType[ nElementAkt ] = T_RefA;			// Typinfo Area-Reff eintragen

	if( !ppP_RefTr[ nP_RefTrAkt ] )
		ppP_RefTr[ nP_RefTrAkt ] = new ScSingleRefData( rTr.Ref1 );
	else
		*ppP_RefTr[ nP_RefTrAkt ] = rTr.Ref1;
	nP_RefTrAkt++;

	if( !ppP_RefTr[ nP_RefTrAkt ] )
		ppP_RefTr[ nP_RefTrAkt ] = new ScSingleRefData( rTr.Ref2 );
	else
		*ppP_RefTr[ nP_RefTrAkt ] = rTr.Ref2;
	nP_RefTrAkt++;

	nElementAkt++;

	return ( const TokenId ) nElementAkt; // Ausgabe von altem Wert + 1!
}


const TokenId TokenPool::Store( const DefTokenId e, const String& r )
{
	if( nElementAkt >= nElement )
		GrowElement();

	if( nP_ExtAkt >= nP_Ext )
        GrowExt();

	pElement[ nElementAkt ] = nP_ExtAkt;
	pType[ nElementAkt ] = T_Ext;			// Typinfo String eintragen

	if( ppP_Ext[ nP_ExtAkt ] )
	{
		ppP_Ext[ nP_ExtAkt ]->eId = e;
		ppP_Ext[ nP_ExtAkt ]->aText = r;
	}
	else
		ppP_Ext[ nP_ExtAkt ] = new EXTCONT( e, r );

	nElementAkt++;
	nP_ExtAkt++;

	return ( const TokenId ) nElementAkt; // Ausgabe von altem Wert + 1!
}


const TokenId TokenPool::StoreNlf( const ScSingleRefData& rTr )
{
	if( nElementAkt >= nElement )
		GrowElement();

	if( nP_NlfAkt >= nP_Nlf )
		GrowNlf();

	pElement[ nElementAkt ] = nP_NlfAkt;
	pType[ nElementAkt ] = T_Nlf;

	if( ppP_Nlf[ nP_NlfAkt ] )
	{
		ppP_Nlf[ nP_NlfAkt ]->aRef = rTr;
	}
	else
		ppP_Nlf[ nP_NlfAkt ] = new NLFCONT( rTr );

	nElementAkt++;
	nP_NlfAkt++;

	return ( const TokenId ) nElementAkt;
}

const TokenId TokenPool::StoreMatrix( SCSIZE nC, SCSIZE nR )
{
    ScMatrix* pM;

    if( nElementAkt >= nElement )
        GrowElement();

    if( nP_MatrixAkt >= nP_Matrix )
        GrowMatrix();

    pElement[ nElementAkt ] = nP_MatrixAkt;
    pType[ nElementAkt ] = T_Matrix;

    pM = new ScMatrix( nC, nR );
    pM->FillDouble( 0., 0,0, nC-1, nR-1 );
    pM->IncRef( );
    ppP_Matrix[ nP_MatrixAkt ] = pM;

    nElementAkt++;
    nP_MatrixAkt++;

    return ( const TokenId ) nElementAkt;
}

const TokenId TokenPool::StoreExtName( sal_uInt16 nFileId, const String& rName )
{
    if ( nElementAkt >= nElement )
        GrowElement();

    pElement[nElementAkt] = static_cast<UINT16>(maExtNames.size());
    pType[nElementAkt] = T_ExtName;
    
    maExtNames.push_back(ExtName());
    ExtName& r = maExtNames.back();
    r.mnFileId = nFileId;
    r.maName = rName;

    ++nElementAkt;

    return static_cast<const TokenId>(nElementAkt);
}

const TokenId TokenPool::StoreExtRef( sal_uInt16 nFileId, const String& rTabName, const ScSingleRefData& rRef )
{
    if ( nElementAkt >= nElement )
        GrowElement();

    pElement[nElementAkt] = static_cast<UINT16>(maExtCellRefs.size());
    pType[nElementAkt] = T_ExtRefC;
    
    maExtCellRefs.push_back(ExtCellRef());
    ExtCellRef& r = maExtCellRefs.back();
    r.mnFileId = nFileId;
    r.maTabName = rTabName;
    r.maRef = rRef;

    ++nElementAkt;

    return static_cast<const TokenId>(nElementAkt);
}

const TokenId TokenPool::StoreExtRef( sal_uInt16 nFileId, const String& rTabName, const ScComplexRefData& rRef )
{
    if ( nElementAkt >= nElement )
        GrowElement();

    pElement[nElementAkt] = static_cast<UINT16>(maExtAreaRefs.size());
    pType[nElementAkt] = T_ExtRefA;
    
    maExtAreaRefs.push_back(ExtAreaRef());
    ExtAreaRef& r = maExtAreaRefs.back();
    r.mnFileId = nFileId;
    r.maTabName = rTabName;
    r.maRef = rRef;

    ++nElementAkt;

    return static_cast<const TokenId>(nElementAkt);
}

void TokenPool::Reset( void )
{
    nP_IdAkt = nP_IdLast = nElementAkt = nP_StrAkt = nP_DblAkt = nP_ErrAkt = nP_RefTrAkt = nP_ExtAkt = nP_NlfAkt = nP_MatrixAkt = 0;
    maExtNames.clear();
    maExtCellRefs.clear();
    maExtAreaRefs.clear();
}


BOOL TokenPool::IsSingleOp( const TokenId& rId, const DefTokenId eId ) const
{
    UINT16 nId = (UINT16) rId;
	if( nId && nId <= nElementAkt )
	{// existent?
		nId--;
		if( T_Id == pType[ nId ] )
		{// Tokenfolge?
			if( pSize[ nId ] == 1 )
			{// GENAU 1 Token
				UINT16	nSecId = pP_Id[ pElement[ nId ] ];
				if( nSecId >= nScTokenOff )
				{// Default-Token?
					return ( DefTokenId ) ( nSecId - nScTokenOff ) == eId;	// Gesuchter?
				}
			}
		}
	}

	return FALSE;
}


const String* TokenPool::GetExternal( const TokenId& rId ) const
{
    const String*   p = NULL;
    UINT16 n = (UINT16) rId;
	if( n && n <= nElementAkt )
	{
		n--;
        if( (pType[ n ] == T_Ext) && ppP_Ext[ pElement[ n ] ] )
            p = &ppP_Ext[ pElement[ n ] ]->aText;
	}

    return p;
}


//UNUSED2008-05  const String* TokenPool::GetString( const TokenId& r ) const
//UNUSED2008-05  {
//UNUSED2008-05      const String*   p = NULL;
//UNUSED2008-05      UINT16 n = (UINT16) r;
//UNUSED2008-05      if( n && n <= nElementAkt )
//UNUSED2008-05      {
//UNUSED2008-05          n--;
//UNUSED2008-05          if( pType[ n ] == T_Str )
//UNUSED2008-05              p = ppP_Str[ pElement[ n ] ];
//UNUSED2008-05      }
//UNUSED2008-05  
//UNUSED2008-05      return p;
//UNUSED2008-05  }

ScMatrix* TokenPool::GetMatrix( unsigned int n ) const
{
    if( n < nP_MatrixAkt )
        return ppP_Matrix[ n ];
    else
        printf ("GETMATRIX %d >= %d\n", n, nP_MatrixAkt);
    return NULL;
}

