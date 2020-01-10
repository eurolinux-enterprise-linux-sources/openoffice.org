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
#include "precompiled_rsc.hxx"
/****************** I N C L U D E S **************************************/

// C and C++ Includes.
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Programmabhaengige Includes.
#include <rscconst.hxx>
#ifndef _RSCSARRAY_HXX
#include <rscarray.hxx>
#endif
#include <rscdb.hxx>

/****************** C O D E **********************************************/
/****************** R s c I n s t N o d e ********************************/
/*************************************************************************
|*
|*    RscInstNode::RscInstNode()
|*
|*    Beschreibung
|*    Ersterstellung    MM 06.08.91
|*    Letzte Aenderung  MM 06.08.91
|*
*************************************************************************/
RscInstNode::RscInstNode( sal_uInt32 nId )
{
    nTypeId = nId;
}

/*************************************************************************
|*
|*    RscInstNode::~RscInstNode()
|*
|*    Beschreibung
|*    Ersterstellung    MM 06.08.91
|*    Letzte Aenderung  MM 06.08.91
|*
*************************************************************************/
RscInstNode::~RscInstNode()
{
    if( aInst.IsInst() )
    {
        aInst.pClass->Destroy( aInst );
        rtl_freeMemory( aInst.pData );
    }
}

/*************************************************************************
|*
|*    RscInstNode::GetId()
|*
|*    Beschreibung
|*    Ersterstellung    MM 06.08.91
|*    Letzte Aenderung  MM 06.08.91
|*
*************************************************************************/
sal_uInt32 RscInstNode::GetId() const
{
    return nTypeId;
}

/****************** R s c A r r a y *************************************/
/*************************************************************************
|*
|*    RscArray::RscArray()
|*
|*    Beschreibung
|*    Ersterstellung    MM 25.05.91
|*    Letzte Aenderung  MM 25.05.91
|*
*************************************************************************/
RscArray::RscArray( Atom nId, sal_uInt32 nTypeId, RscTop * pSuper, RscEnum * pTypeCl )
        : RscTop( nId, nTypeId, pSuper )
{
    pTypeClass = pTypeCl;
    nOffInstData = RscTop::Size();
    nSize = nOffInstData + ALIGNED_SIZE( sizeof( RscArrayInst ) );
}

/*************************************************************************
|*
|*    RscArray::~RscArray()
|*
|*    Beschreibung
|*    Ersterstellung    MM 25.05.91
|*    Letzte Aenderung  MM 25.05.91
|*
*************************************************************************/
RscArray::~RscArray()
{
}

/*************************************************************************
|*
|*    RscArray::~RscArray()
|*
|*    Beschreibung
|*    Ersterstellung    MM 25.05.91
|*    Letzte Aenderung  MM 25.05.91
|*
*************************************************************************/
RSCCLASS_TYPE RscArray::GetClassType() const
{
    return RSCCLASS_ENUMARRAY;
}

/*************************************************************************
|*
|*    RscArray::GetIndexType()
|*
|*    Beschreibung
|*    Ersterstellung    MM 23.12.92
|*    Letzte Aenderung  MM
|*
*************************************************************************/
RscTop * RscArray::GetTypeClass() const
{
    return pTypeClass;
}

/*************************************************************************
|*
|*    RscArray::Create()
|*
|*    Beschreibung
|*    Ersterstellung    MM 26.04.91
|*    Letzte Aenderung  MM 26.04.91
|*
*************************************************************************/
static RscInstNode * Create( RscInstNode * pNode )
{
    RscInstNode * pRetNode = NULL;
    RscInstNode * pTmpNode;

    if( pNode )
    {
        pRetNode = new RscInstNode( pNode->GetId() );
        pRetNode->aInst = pNode->aInst.pClass->Create( NULL, pNode->aInst );
        if( (pTmpNode = Create( pNode->Left() )) != NULL )
            pRetNode->Insert( pTmpNode );
        if( (pTmpNode = Create( pNode->Right() )) != NULL )
            pRetNode->Insert( pTmpNode );
    }

    return pRetNode;
}

RSCINST RscArray::Create( RSCINST * pInst, const RSCINST & rDflt,
                          BOOL bOwnClass )
{
    RSCINST aInst;
    RscArrayInst *  pClassData;

    if( !pInst )
	{
        aInst.pClass = this;
        aInst.pData = (CLASS_DATA) rtl_allocateMemory( Size() );
    }
    else
        aInst = *pInst;
    if( !bOwnClass && rDflt.IsInst() )
        bOwnClass = rDflt.pClass->InHierarchy( this );

    RscTop::Create( &aInst, rDflt, bOwnClass );

    pClassData          = (RscArrayInst *)(aInst.pData + nOffInstData);
    pClassData->pNode   = NULL;
    if( bOwnClass )
    {
        RscArrayInst *   pDfltClassData;

        pDfltClassData = (RscArrayInst *)(rDflt.pData + nOffInstData);

        pClassData->pNode = ::Create( pDfltClassData->pNode );
    }
    return( aInst );
}

/*************************************************************************
|*
|*    RscArray::Destroy()
|*
|*    Beschreibung
|*
*************************************************************************/
static void Destroy( RscInstNode * pNode )
{
    if( pNode )
    {
        Destroy( pNode->Left() );
        Destroy( pNode->Right() );
        delete pNode;
    }
}

void RscArray::Destroy( const RSCINST & rInst )
{
    RscArrayInst *  pClassData;

    RscTop::Destroy( rInst );

    pClassData = (RscArrayInst *)(rInst.pData + nOffInstData);

    //Baum rekursiv loeschen
    ::Destroy( pClassData->pNode );
}

/*************************************************************************
|*
|*    RscArray::GetValueEle()
|*
|*    Beschreibung
|*
*************************************************************************/
ERRTYPE RscArray::GetValueEle
(
	const RSCINST & rInst,
	INT32 lValue,
	RscTop * pCreateClass,
    RSCINST * pGetInst
)
{
    RscArrayInst *  pClassData;
    RscInstNode *   pNode;

    pClassData = (RscArrayInst *)(rInst.pData + nOffInstData);

    ERRTYPE     aError;

    Atom  nId;
    if( !pTypeClass->GetValueConst( sal_uInt32(lValue), &nId ) )
    { // nicht gefunden
        return ERR_ARRAY_INVALIDINDEX;
    }

    if( pClassData->pNode )
        pNode = pClassData->pNode->Search( sal_uInt32(lValue) );
    else
        pNode = NULL;

/*
    if( pNode )
    {
        if( pNode->aInst.pClass->IsDefault( pNode->aInst ) )
        {
            GetSuperClass()->Destroy( pNode->aInst );
            GetSuperClass()->Create( &pNode->aInst, rInst );
            pNode->aInst.pClass->SetToDefault( pNode->aInst );
        }
    }
    else
*/
    if( !pNode )
    {
        pNode = new RscInstNode( sal_uInt32(lValue) );
		if( pCreateClass && GetSuperClass()->InHierarchy( pCreateClass ) )
	        pNode->aInst = pCreateClass->Create( NULL, rInst );
		else
	        pNode->aInst = GetSuperClass()->Create( NULL, rInst );
        pNode->aInst.pClass->SetToDefault( pNode->aInst );
        if( pClassData->pNode )
            pClassData->pNode->Insert( pNode );
        else
            pClassData->pNode = pNode;
    }

    *pGetInst = pNode->aInst;
    return aError;
}

/*************************************************************************
|*
|*    RscArray::GetArrayEle()
|*
|*    Beschreibung
|*
*************************************************************************/
ERRTYPE RscArray::GetArrayEle
(
	const RSCINST & rInst,
	Atom nId,
	RscTop * pCreateClass,
	RSCINST * pGetInst
)
{
    INT32  lValue;
    if( !pTypeClass->GetConstValue( nId, &lValue ) )
    { // nicht gefunden
        return ERR_ARRAY_INVALIDINDEX;
    }

    return GetValueEle( rInst, lValue, pCreateClass, pGetInst );
}

/*************************************************************************
|*
|*    RscArray::IsConsistent()
|*
|*    Beschreibung
|*    Ersterstellung    MM 23.09.91
|*    Letzte Aenderung  MM 23.09.91
|*
*************************************************************************/
static BOOL IsConsistent( RscInstNode * pNode, RscInconsList * pList )
{
    BOOL bRet = TRUE;

    if( pNode )
    {
        bRet = pNode->aInst.pClass->IsConsistent( pNode->aInst, pList );
        if( !IsConsistent( pNode->Left(), pList ) )
            bRet = FALSE;
        if( !IsConsistent( pNode->Right(), pList ) )
            bRet = FALSE;
    }
    return bRet;
}

BOOL RscArray::IsConsistent( const RSCINST & rInst, RscInconsList * pList )
{
    RscArrayInst * pClassData;
    BOOL    bRet;

    bRet = RscTop::IsConsistent( rInst, pList );

    pClassData = (RscArrayInst *)(rInst.pData + nOffInstData);
    if( !::IsConsistent( pClassData->pNode, pList ) )
        bRet = FALSE;

    return( bRet );
}

/*************************************************************************
|*
|*    RscArray::SetToDefault()
|*
|*    Beschreibung
|*    Ersterstellung    MM 25.04.91
|*    Letzte Aenderung  MM 25.04.91
|*
*************************************************************************/
static void SetToDefault( RscInstNode * pNode )
{
    if( pNode )
    {
        pNode->aInst.pClass->SetToDefault( pNode->aInst );
        SetToDefault( pNode->Left() );
        SetToDefault( pNode->Right() );
    }
}

void RscArray::SetToDefault( const RSCINST & rInst )
{
    RscArrayInst * pClassData;

    pClassData = (RscArrayInst *)(rInst.pData + nOffInstData);

    ::SetToDefault( pClassData->pNode );

    RscTop::SetToDefault( rInst );
}

/*************************************************************************
|*
|*    RscArray::IsDefault()
|*
|*    Beschreibung
|*    Ersterstellung    MM 25.04.91
|*    Letzte Aenderung  MM 25.04.91
|*
*************************************************************************/
static BOOL IsDefault( RscInstNode * pNode )
{
    BOOL bRet = TRUE;

    if( pNode )
    {
        bRet = pNode->aInst.pClass->IsDefault( pNode->aInst );
        if( bRet )
            bRet = IsDefault( pNode->Left() );
        if( bRet )
            bRet = IsDefault( pNode->Right() );
    }
    return bRet;
}

BOOL RscArray::IsDefault( const RSCINST & rInst )
{
    RscArrayInst * pClassData;

    pClassData = (RscArrayInst *)(rInst.pData + nOffInstData);

    BOOL bRet = ::IsDefault( pClassData->pNode );

    if( bRet )
        bRet = RscTop::IsDefault( rInst );
    return bRet;
}

/*************************************************************************
|*
|*    RscArray::IsValueDefault()
|*
|*    Beschreibung
|*    Ersterstellung    MM 25.04.91
|*    Letzte Aenderung  MM 15.01.92
|*
*************************************************************************/
static BOOL IsValueDefault( RscInstNode * pNode, CLASS_DATA pDef )
{
    BOOL bRet = TRUE;

    if( pNode )
    {
        bRet = pNode->aInst.pClass->IsValueDefault( pNode->aInst, pDef );
        if( bRet )
            bRet = IsValueDefault( pNode->Left(), pDef );
        if( bRet )
            bRet = IsValueDefault( pNode->Right(), pDef );
    }
    return bRet;
}

BOOL RscArray::IsValueDefault( const RSCINST & rInst, CLASS_DATA pDef )
{
    RscArrayInst * pClassData;
    BOOL bRet;

    bRet = RscTop::IsValueDefault( rInst, pDef );

    if( bRet )
    {
        pClassData = (RscArrayInst *)(rInst.pData + nOffInstData);

        bRet = ::IsValueDefault( pClassData->pNode, pDef );
    }
    return bRet;
}

/*************************************************************************
|*    RscArray::WriteSrcHeader()
|*
|*    Beschreibung
*************************************************************************/
void RscArray::WriteSrcHeader( const RSCINST & rInst, FILE * fOutput,
                               RscTypCont * pTC, sal_uInt32 nTab,
                               const RscId & aId, const char * pVarName )
{
    RscArrayInst * pClassData;

    pClassData = (RscArrayInst *)(rInst.pData + nOffInstData);

	if( pTC->IsSrsDefault() )
	{ // nur einen Wert schreiben
	    RscInstNode *   pNode = NULL;
	    if( pClassData->pNode )
		{
            std::vector< sal_uInt32 >::const_iterator it;
            for( it = pTC->GetFallbacks().begin(); !pNode && it != pTC->GetFallbacks().end(); ++it )
                pNode = pClassData->pNode->Search( *it );
		}

	    if( pNode )
		{
	        if( pNode->aInst.pClass->IsDefault( pNode->aInst ) )
		        fprintf( fOutput, "Default" );
			else
		        pNode->aInst.pClass->WriteSrcHeader(
										pNode->aInst, fOutput,
										pTC, nTab, aId, pVarName );
			return;
	    }
	}

	if( IsDefault( rInst ) )
		fprintf( fOutput, "Default" );
	else
	{
		RSCINST aSuper( GetSuperClass(), rInst.pData );
		aSuper.pClass->WriteSrcHeader( aSuper, fOutput, pTC,
										nTab, aId, pVarName );
	}
	if( !pTC->IsSrsDefault() )
		WriteSrc( rInst, fOutput, pTC, nTab, pVarName );
}

/*************************************************************************
|*    RscArray::WriteSrc()
|*
|*    Beschreibung
*************************************************************************/
static void WriteSrc( RscInstNode * pNode, FILE * fOutput, RscTypCont * pTC,
                         sal_uInt32 nTab, const char * pVarName,
                         CLASS_DATA pDfltData, RscConst * pTypeClass )
{
    if( pNode )
    {
        WriteSrc( pNode->Left(), fOutput, pTC, nTab, pVarName,
                    pDfltData, pTypeClass );
        if( !pNode->aInst.pClass->IsValueDefault( pNode->aInst, pDfltData ) )
        {
            fprintf( fOutput, ";\n" );
            for( sal_uInt32 n = 0; n < nTab; n++ )
                fputc( '\t', fOutput );

            Atom  nIdxId;
            pTypeClass->GetValueConst( pNode->GetId(), &nIdxId );
            fprintf( fOutput, "%s[ %s ] = ", pVarName, pHS->getString( nIdxId ).getStr() );
            pNode->aInst.pClass->WriteSrcHeader( pNode->aInst, fOutput, pTC,
                                                nTab, RscId(), pVarName );
        }
        WriteSrc( pNode->Right(), fOutput, pTC, nTab, pVarName,
                    pDfltData, pTypeClass );
    }
}

void RscArray::WriteSrcArray( const RSCINST & rInst, FILE * fOutput,
                         	RscTypCont * pTC, sal_uInt32 nTab,
                         	const char * pVarName )
{
    RscArrayInst * pClassData;

    pClassData = (RscArrayInst *)(rInst.pData + nOffInstData);

    ::WriteSrc( pClassData->pNode, fOutput, pTC, nTab, pVarName,
                rInst.pData, pTypeClass );
};

void RscArray::WriteSrc( const RSCINST & rInst, FILE * fOutput,
                         RscTypCont * pTC, sal_uInt32 nTab,
                         const char * pVarName )
{
	WriteSrcArray( rInst, fOutput, pTC, nTab, pVarName );
}

/*************************************************************************
|*    RscArray::WriteRc()
|*
|*    Beschreibung
*************************************************************************/
ERRTYPE RscArray::WriteRc( const RSCINST & rInst, RscWriteRc & rMem,
                            RscTypCont * pTC, sal_uInt32 nDeep, BOOL bExtra )
{
    ERRTYPE aError;
    RscArrayInst * pClassData;
    RscInstNode *   pNode = NULL;

    pClassData = (RscArrayInst *)(rInst.pData + nOffInstData);

    if( pClassData->pNode )
	{
#if OSL_DEBUG_LEVEL > 2
        fprintf( stderr, "RscArray::WriteRc: Fallback " );
#endif
        std::vector< sal_uInt32 >::const_iterator it;
        for( it = pTC->GetFallbacks().begin(); !pNode && it != pTC->GetFallbacks().end(); ++it )
        {
            pNode = pClassData->pNode->Search( *it );
#if OSL_DEBUG_LEVEL > 2
            fprintf( stderr, " 0x%hx", *it );
#endif
        }
#if OSL_DEBUG_LEVEL > 2
            fprintf( stderr, "\n" );
#endif
	}

    if( pNode )
        aError = pNode->aInst.pClass->WriteRc( pNode->aInst, rMem, pTC,
                                                nDeep, bExtra );
    else
        aError = RscTop::WriteRc( rInst, rMem, pTC, nDeep, bExtra );

    return aError;
}

//========================================================================
void RscArray::WriteRcAccess
(
	FILE * fOutput,
	RscTypCont * pTC,
	const char * pName
)
{
	GetSuperClass()->WriteRcAccess( fOutput, pTC, pName );
}

/*************************************************************************
|*
|*    RscClassArray::RscClassArray()
|*
|*    Beschreibung
|*    Ersterstellung    MM 25.05.91
|*    Letzte Aenderung  MM 25.05.91
|*
*************************************************************************/
RscClassArray::RscClassArray( Atom nId, sal_uInt32 nTypeId, RscTop * pSuper,
                              RscEnum * pTypeCl )
    : RscArray( nId, nTypeId, pSuper, pTypeCl )
{
}

/*************************************************************************
|*
|*    RscClassArray::~RscClassArray()
|*
|*    Beschreibung
|*    Ersterstellung    MM 25.05.91
|*    Letzte Aenderung  MM 25.05.91
|*
*************************************************************************/
RscClassArray::~RscClassArray()
{
}

/*************************************************************************
|*
|*    RscClassArray::WriteSrcHeader()
|*
|*    Beschreibung
|*    Ersterstellung    MM 25.05.91
|*    Letzte Aenderung  MM 25.05.91
|*
*************************************************************************/
void RscClassArray::WriteSrcHeader( const RSCINST & rInst, FILE * fOutput,
									RscTypCont * pTC, sal_uInt32 nTab,
									const RscId & aId, const char * pName )
{
	RscArray::WriteSrcHeader( rInst, fOutput, pTC, nTab, aId, pName );
}

/*************************************************************************
|*
|*    RscClassArray::WriteSrc()
|*
|*    Beschreibung
|*    Ersterstellung    MM 25.05.91
|*    Letzte Aenderung  MM 25.05.91
|*
*************************************************************************/
void RscClassArray::WriteSrc( const RSCINST & rInst, FILE * fOutput,
            	             RscTypCont * pTC, sal_uInt32 nTab,
	               	          const char * pVarName )
{
	RscArray::WriteSrc( rInst, fOutput, pTC, nTab, pVarName );
}

/*************************************************************************
|*
|*    RscClassArray::WriteRcHeader()
|*
|*    Beschreibung
|*    Ersterstellung    MM 25.05.91
|*    Letzte Aenderung  MM 25.05.91
|*
*************************************************************************/
ERRTYPE RscClassArray::WriteRcHeader( const RSCINST & rInst, RscWriteRc & aMem,
									   RscTypCont * pTC, const RscId & aId,
									   sal_uInt32 nDeep, BOOL bExtra )
{
	// Eigenen Typ schreiben
	return GetSuperClass()->WriteRcHeader( rInst, aMem, pTC, aId,
										nDeep, bExtra );
}

/*************************************************************************
|*
|*    RscLangArray::RscLangArray()
|*
|*    Beschreibung
|*    Ersterstellung    MM 25.05.91
|*    Letzte Aenderung  MM 25.05.91
|*
*************************************************************************/
RscLangArray::RscLangArray( Atom nId, sal_uInt32 nTypeId, RscTop * pSuper,
                          RscEnum * pTypeCl )
    : RscArray( nId, nTypeId, pSuper, pTypeCl )
{
}

/*************************************************************************
|*
|*    RscLangArray::RscLangArray()
|*
|*    Beschreibung
|*    Ersterstellung    MM 25.05.91
|*    Letzte Aenderung  MM 25.05.91
|*
*************************************************************************/
RSCCLASS_TYPE RscLangArray::GetClassType() const
{
    if( GetSuperClass() )
        return GetSuperClass()->GetClassType();
    else
        return RscArray::GetClassType();

}

