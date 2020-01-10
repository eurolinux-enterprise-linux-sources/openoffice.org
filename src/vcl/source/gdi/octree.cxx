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
#include "precompiled_vcl.hxx"
#include <limits.h>
#include <vcl/bmpacc.hxx>
#include <vcl/impoct.hxx>
#include <vcl/octree.hxx>

// ---------
// - pMask -
// ---------

static BYTE pImplMask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

// -------------
// - NodeCache -
// -------------

ImpNodeCache::ImpNodeCache( const ULONG nInitSize ) :
            pActNode( NULL )
{
    const ULONG nSize = nInitSize + 4;

    for( ULONG i = 0; i < nSize; i++ )
    {
        OctreeNode* pNewNode = new NODE;

        pNewNode->pNextInCache = pActNode;
        pActNode = pNewNode;
    }
}

// ------------------------------------------------------------------------

ImpNodeCache::~ImpNodeCache()
{
    while( pActNode )
    {
        OctreeNode* pNode = pActNode;

        pActNode = pNode->pNextInCache;
        delete pNode;
    }
}

// ----------
// - Octree -
// ----------

Octree::Octree( ULONG nColors ) :
            nMax        ( nColors ),
            nLeafCount  ( 0L ),
            pTree       ( NULL ),
            pAcc        ( NULL )
{
    pNodeCache = new ImpNodeCache( nColors );
    memset( pReduce, 0, ( OCTREE_BITS + 1 ) * sizeof( PNODE ) );
}

// ------------------------------------------------------------------------

Octree::Octree( const BitmapReadAccess& rReadAcc, ULONG nColors ) :
            nMax        ( nColors ),
            nLeafCount  ( 0L ),
            pTree       ( NULL ),
            pAcc        ( &rReadAcc )
{
    pNodeCache = new ImpNodeCache( nColors );
    memset( pReduce, 0, ( OCTREE_BITS + 1 ) * sizeof( PNODE ) );
    ImplCreateOctree();
}

// ------------------------------------------------------------------------

Octree::~Octree()
{
    ImplDeleteOctree( &pTree );
    delete pNodeCache;
}

// ------------------------------------------------------------------------

void Octree::AddColor( const BitmapColor& rColor )
{
    pColor = &(BitmapColor&) rColor;
    nLevel = 0L;
    ImplAdd( &pTree );

    while( nLeafCount > nMax )
        ImplReduce();
}

// ------------------------------------------------------------------------

void Octree::ImplCreateOctree()
{
    if( !!*pAcc )
    {
        const long      nWidth = pAcc->Width();
        const long      nHeight = pAcc->Height();

        if( pAcc->HasPalette() )
        {
            for( long nY = 0; nY < nHeight; nY++ )
            {
                for( long nX = 0; nX < nWidth; nX++ )
                {
                    pColor = &(BitmapColor&) pAcc->GetPaletteColor( pAcc->GetPixel( nY, nX ) );
                    nLevel = 0L;
                    ImplAdd( &pTree );

                    while( nLeafCount > nMax )
                        ImplReduce();
                }
            }
        }
        else
        {
            BitmapColor aColor;

            pColor = &aColor;

            for( long nY = 0; nY < nHeight; nY++ )
            {
                for( long nX = 0; nX < nWidth; nX++ )
                {
                    aColor = pAcc->GetPixel( nY, nX );
                    nLevel = 0L;
                    ImplAdd( &pTree );

                    while( nLeafCount > nMax )
                        ImplReduce();
                }
            }
        }
    }
}

// ------------------------------------------------------------------------

void Octree::ImplDeleteOctree( PPNODE ppNode )
{
    for ( ULONG i = 0UL; i < 8UL; i++ )
    {
        if ( (*ppNode)->pChild[ i ] )
            ImplDeleteOctree( &(*ppNode)->pChild[ i ] );
    }

    pNodeCache->ImplReleaseNode( *ppNode );
    *ppNode = NULL;
}

// ------------------------------------------------------------------------

void Octree::ImplAdd( PPNODE ppNode )
{
    // ggf. neuen Knoten erzeugen
    if( !*ppNode )
    {
        *ppNode = pNodeCache->ImplGetFreeNode();
        (*ppNode)->bLeaf = ( OCTREE_BITS == nLevel );

        if( (*ppNode)->bLeaf )
            nLeafCount++;
        else
        {
            (*ppNode)->pNext = pReduce[ nLevel ];
            pReduce[ nLevel ] = *ppNode;
        }
    }

    if( (*ppNode)->bLeaf )
    {
        (*ppNode)->nCount++;
        (*ppNode)->nRed += pColor->GetRed();
        (*ppNode)->nGreen += pColor->GetGreen();
        (*ppNode)->nBlue += pColor->GetBlue();
    }
    else
    {
        const ULONG nShift = 7 - nLevel;
        const BYTE  cMask = pImplMask[ nLevel ];
        const ULONG nIndex = ( ( ( pColor->GetRed() & cMask ) >> nShift ) << 2 ) |
                             ( ( ( pColor->GetGreen() & cMask ) >> nShift ) << 1 ) |
                             ( ( pColor->GetBlue() & cMask ) >> nShift );

        nLevel++;
        ImplAdd( &(*ppNode)->pChild[ nIndex ] );
    }
}

// ------------------------------------------------------------------------

void Octree::ImplReduce()
{
    ULONG   i;
    PNODE   pNode;
    ULONG   nRedSum = 0L;
    ULONG   nGreenSum = 0L;
    ULONG   nBlueSum = 0L;
    ULONG   nChilds = 0L;

    for ( i = OCTREE_BITS - 1; i && !pReduce[i]; i-- ) {}

    pNode = pReduce[ i ];
    pReduce[ i ] = pNode->pNext;

    for ( i = 0; i < 8; i++ )
    {
        if ( pNode->pChild[ i ] )
        {
            PNODE pChild = pNode->pChild[ i ];

            nRedSum += pChild->nRed;
            nGreenSum += pChild->nGreen;
            nBlueSum += pChild->nBlue;
            pNode->nCount += pChild->nCount;

            pNodeCache->ImplReleaseNode( pNode->pChild[ i ] );
            pNode->pChild[ i ] = NULL;
            nChilds++;
        }
    }

    pNode->bLeaf = TRUE;
    pNode->nRed = nRedSum;
    pNode->nGreen = nGreenSum;
    pNode->nBlue = nBlueSum;
    nLeafCount -= --nChilds;
}

// ------------------------------------------------------------------------

void Octree::CreatePalette( PNODE pNode )
{
    if( pNode->bLeaf )
    {
        pNode->nPalIndex = nPalIndex;
        aPal[ nPalIndex++ ] = BitmapColor( (BYTE) ( (double) pNode->nRed / pNode->nCount ),
                                           (BYTE) ( (double) pNode->nGreen / pNode->nCount ),
                                           (BYTE) ( (double) pNode->nBlue / pNode->nCount ) );
    }
    else for( ULONG i = 0UL; i < 8UL; i++ )
        if( pNode->pChild[ i ] )
            CreatePalette( pNode->pChild[ i ] );

}

// ------------------------------------------------------------------------

void Octree::GetPalIndex( PNODE pNode )
{
    if ( pNode->bLeaf )
        nPalIndex = pNode->nPalIndex;
    else
    {
        const ULONG nShift = 7 - nLevel;
        const BYTE  cMask = pImplMask[ nLevel++ ];
        const ULONG nIndex = ( ( ( pColor->GetRed() & cMask ) >> nShift ) << 2 ) |
                             ( ( ( pColor->GetGreen() & cMask ) >> nShift ) << 1 ) |
                             ( ( pColor->GetBlue() & cMask ) >> nShift );

        GetPalIndex( pNode->pChild[ nIndex ] );
    }
}

// -------------------
// - InverseColorMap -
// -------------------

InverseColorMap::InverseColorMap( const BitmapPalette& rPal ) :
            nBits( 8 - OCTREE_BITS )
{
    ULONG*			cdp;
    BYTE*           crgbp;
    const ULONG     nColorMax = 1 << OCTREE_BITS;
    const ULONG     xsqr = 1 << ( nBits << 1 );
    const ULONG     xsqr2 = xsqr << 1;
    const ULONG     nColors = rPal.GetEntryCount();
    const long      x = 1L << nBits;
    const long      x2 = x >> 1L;
    ULONG           r, g, b;
    long            rxx, gxx, bxx;
    long            rdist, gdist, bdist;
    long            crinc, cginc, cbinc;

    ImplCreateBuffers( nColorMax );

    for( ULONG nIndex = 0; nIndex < nColors; nIndex++ )
    {
        const BitmapColor&  rColor = rPal[ (USHORT) nIndex ];
        const BYTE			cRed = rColor.GetRed();
        const BYTE			cGreen = rColor.GetGreen();
        const BYTE			cBlue = rColor.GetBlue();

        rdist = cRed - x2;
        gdist = cGreen - x2;
        bdist = cBlue - x2;
        rdist = rdist*rdist + gdist*gdist + bdist*bdist;

        crinc = ( xsqr - ( cRed << nBits ) ) << 1L;
        cginc = ( xsqr - ( cGreen << nBits ) ) << 1L;
        cbinc = ( xsqr - ( cBlue << nBits ) ) << 1L;

        cdp = (ULONG*) pBuffer;
        crgbp = pMap;

        for( r = 0, rxx = crinc; r < nColorMax; rdist += rxx, r++, rxx += xsqr2 )
        {
            for( g = 0, gdist = rdist, gxx = cginc; g < nColorMax;  gdist += gxx, g++, gxx += xsqr2 )
            {
                for( b = 0, bdist = gdist, bxx = cbinc; b < nColorMax; bdist += bxx, b++, cdp++, crgbp++, bxx += xsqr2 )
                    if ( !nIndex || ( (long) *cdp ) > bdist )
                    {
                        *cdp = bdist;
                        *crgbp = (BYTE) nIndex;
                    }
            }
        }
    }
}

// ------------------------------------------------------------------------

InverseColorMap::~InverseColorMap()
{
    rtl_freeMemory( pBuffer );
    rtl_freeMemory( pMap );
}

// ------------------------------------------------------------------------

void InverseColorMap::ImplCreateBuffers( const ULONG nMax )
{
    const ULONG nCount = nMax * nMax * nMax;
    const ULONG nSize = nCount * sizeof( ULONG );

    pMap = (BYTE*) rtl_allocateMemory( nCount );
    memset( pMap, 0x00, nCount );

    pBuffer = (BYTE*) rtl_allocateMemory( nSize );
    memset( pBuffer, 0xff, nSize );
}
