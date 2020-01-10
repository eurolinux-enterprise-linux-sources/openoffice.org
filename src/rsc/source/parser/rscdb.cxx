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
#include <ctype.h>		// isdigit(), isalpha()
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <tools/fsys.hxx>
#include <tools/rc.h>
#include <tools/isofallback.hxx>
#include <rtl/strbuf.hxx>

// Programmabhaengige Includes.
#include <rsctree.hxx>
#include <rsctop.hxx>
#include <rscmgr.hxx>
#include <rscdb.hxx>
#include <rscrsc.hxx>

using namespace rtl;

/*************************************************************************
|*
|*	  RscTypCont :: RscTypCont
|*
|*	  Beschreibung		RES.DOC
|*	  Ersterstellung	MM 22.03.90
|*	  Letzte Aenderung	MM 27.06.90
|*
*************************************************************************/
RscTypCont :: RscTypCont( RscError * pErrHdl,
						  RSCBYTEORDER_TYPE nOrder,
						  const ByteString & rSearchPath,
						  sal_uInt32 nFlagsP )
	:
	  nSourceCharSet( RTL_TEXTENCODING_UTF8 ),
	  nByteOrder( nOrder ),
	  aSearchPath( rSearchPath ),
	  aBool( pHS->getID( "BOOL" ), RSC_NOTYPE ),
	  aShort( pHS->getID( "short" ), RSC_NOTYPE ),
	  aUShort( pHS->getID( "USHORT" ), RSC_NOTYPE ),
	  aLong( pHS->getID( "long" ), RSC_NOTYPE ),
	  aEnumLong( pHS->getID( "enum_long" ), RSC_NOTYPE ),
	  aIdUShort( pHS->getID( "IDUSHORT" ), RSC_NOTYPE ),
	  aIdNoZeroUShort( pHS->getID( "IDUSHORT" ), RSC_NOTYPE ),
	  aNoZeroShort( pHS->getID( "NoZeroShort" ), RSC_NOTYPE ),
	  a1to12Short( pHS->getID( "MonthShort" ), RSC_NOTYPE ),
	  a0to23Short( pHS->getID( "HourShort" ), RSC_NOTYPE ),
	  a1to31Short( pHS->getID( "DayShort" ), RSC_NOTYPE ),
	  a0to59Short( pHS->getID( "MinuteShort" ), RSC_NOTYPE ),
	  a0to99Short( pHS->getID( "_0to59Short" ), RSC_NOTYPE ),
	  a0to9999Short( pHS->getID( "YearShort" ), RSC_NOTYPE ),
	  aIdLong( pHS->getID( "IDLONG" ), RSC_NOTYPE ),
	  aString( pHS->getID( "Chars" ), RSC_NOTYPE ),
	  aWinBits( pHS->getID( "WinBits" ), RSC_NOTYPE ),
	  aLangType(),
	  aLangString( pHS->getID( "Lang_Chars" ), RSC_NOTYPE, &aString, &aLangType ),
	  aLangShort( pHS->getID( "Lang_short" ), RSC_NOTYPE, &aShort, &aLangType ),
      nAcceleratorType( 0 ),
	  nFlags( nFlagsP )
{
	nUniqueId = 256;
	nPMId = RSC_VERSIONCONTROL +1; //mindestens einen groesser
	pEH = pErrHdl;
	Init();
}

static sal_uInt32 getLangIdAndShortenLocale( RscTypCont* pTypCont,
                                             rtl::OString& rLang,
                                             rtl::OString& rCountry,
                                             rtl::OString& rVariant )
{
    rtl::OStringBuffer aLangStr( 64 );
    aLangStr.append( rLang.toAsciiLowerCase() );
    if( rCountry.getLength() )
    {
        aLangStr.append( '-' );
        aLangStr.append( rCountry.toAsciiUpperCase() );
    }
    if( rVariant.getLength() )
    {
        aLangStr.append( '-' );
        aLangStr.append( rVariant );
    }
    rtl::OString aL( aLangStr.makeStringAndClear() );
    sal_uInt32 nRet = GetLangId( aL );
    if( nRet == 0 )
    {
        pTypCont->AddLanguage( aL );
        nRet = GetLangId( aL );
    }
    if( rVariant.getLength() )
        rVariant = rtl::OString();
    else if( rCountry.getLength() )
        rCountry = rtl::OString();
    else
        rLang = rtl::OString();
#if OSL_DEBUG_LEVEL > 1
        fprintf( stderr, " %s (0x%hx)", aL.getStr(), nRet );
#endif
    return nRet;
}

ByteString RscTypCont::ChangeLanguage( const ByteString& rNewLang )
{
    ByteString aRet = aLanguage;
    aLanguage = rNewLang;

    rtl::OString aLang = aLanguage;
    rtl::OString aLg, aCountry, aVariant;
    sal_Int32 nIndex = 0;
    aLg = aLang.getToken( 0, '-', nIndex );
    if( nIndex != -1 )
        aCountry = aLang.getToken( 0, '-', nIndex );
    if( nIndex != -1 )
        aVariant = aLang.copy( nIndex );
    
    bool bAppendEnUsFallback =
        ! (rNewLang.EqualsIgnoreCaseAscii( "en-US" ) ||
           rNewLang.EqualsIgnoreCaseAscii( "x-no-translate" ) );

#if OSL_DEBUG_LEVEL > 1
    fprintf( stderr, "RscTypCont::ChangeLanguage:" );
#endif
    aLangFallbacks.clear();

    do
    {
        aLangFallbacks.push_back(getLangIdAndShortenLocale( this, aLg, aCountry, aVariant ) );
    } while( aLg.getLength() );
    
    if( bAppendEnUsFallback )
    {
        aLg = "en";
        aCountry = "US";
        aVariant = rtl::OString();
        aLangFallbacks.push_back( getLangIdAndShortenLocale( this, aLg, aCountry, aVariant ) );
    }

#if OSL_DEBUG_LEVEL > 1
    fprintf( stderr, "\n" );
#endif

    return aRet;
}

Atom RscTypCont::AddLanguage( const char* pLang )
{
    return aLangType.AddLanguage( pLang, aNmTb );
}


/*************************************************************************
|*
|*	  RscTypCont :: ~RscTypCont
|*
|*	  Beschreibung		RES.DOC
|*	  Ersterstellung	MM 22.03.90
|*	  Letzte Aenderung	MM 27.06.90
|*
*************************************************************************/
void DestroyNode( RscTop * pRscTop, ObjNode * pObjNode ){
	if( pObjNode ){
		DestroyNode( pRscTop, (ObjNode*)pObjNode->Left() );
		DestroyNode( pRscTop, (ObjNode*)pObjNode->Right() );

		if( pObjNode->GetRscObj() ){
			pRscTop->Destroy( RSCINST( pRscTop, pObjNode->GetRscObj() ) );
			rtl_freeMemory( pObjNode->GetRscObj() );
		}
		delete pObjNode;
	};
}

void DestroySubTrees( RscTop * pRscTop ){
	if( pRscTop ){
		DestroySubTrees( (RscTop*)pRscTop->Left() );

		DestroyNode( pRscTop, pRscTop->GetObjNode() );

		DestroySubTrees( (RscTop*)pRscTop->Right() );
	};
}

void DestroyTree( RscTop * pRscTop ){
	if( pRscTop ){
		DestroyTree( (RscTop*)pRscTop->Left() );
		DestroyTree( (RscTop*)pRscTop->Right() );

		delete pRscTop;
	};
}

void Pre_dtorTree( RscTop * pRscTop ){
	if( pRscTop ){
		Pre_dtorTree( (RscTop*)pRscTop->Left() );
		Pre_dtorTree( (RscTop*)pRscTop->Right() );

		pRscTop->Pre_dtor();
	};
}

RscTypCont :: ~RscTypCont(){
	RscTop	*		pRscTmp;
	RscSysEntry   * pSysEntry;

	// Alle Unterbaeume loeschen
	aVersion.pClass->Destroy( aVersion );
	rtl_freeMemory( aVersion.pData );
	DestroySubTrees( pRoot );

	// Alle Klassen noch gueltig, jeweilige Instanzen freigeben
	// BasisTypen
	pRscTmp = aBaseLst.First();
	while( pRscTmp ){
		pRscTmp->Pre_dtor();
		pRscTmp = aBaseLst.Next();
	};
	aBool.Pre_dtor();
	aShort.Pre_dtor();
	aUShort.Pre_dtor();
	aIdUShort.Pre_dtor();
	aIdNoZeroUShort.Pre_dtor();
	aNoZeroShort.Pre_dtor();
	aIdLong.Pre_dtor();
	aString.Pre_dtor();
	aWinBits.Pre_dtor();
	aVersion.pClass->Pre_dtor();
	// Zusammengesetzte Typen
	Pre_dtorTree( pRoot );

	// Klassen zerstoeren
	delete aVersion.pClass;
	DestroyTree( pRoot );

	while( NULL != (pRscTmp = aBaseLst.Remove()) ){
		delete pRscTmp;
	};

	while( NULL != (pSysEntry = aSysLst.Remove()) ){
		delete pSysEntry;
	};
}

void RscTypCont::ClearSysNames()
{
	RscSysEntry   * pSysEntry;
	while( NULL != (pSysEntry = aSysLst.Remove()) ){
		delete pSysEntry;
	};
}

//=======================================================================
RscTop * RscTypCont::SearchType( Atom nId )
/*	[Beschreibung]

	Sucht eine Basistyp nId;
*/
{
	if( nId == InvalidAtom )
		return NULL;

#define ELSE_IF( a )				\
	else if( a.GetId() == nId )	\
		return &a;					\

	if( aBool.GetId() == nId )
		return &aBool;
	ELSE_IF( aShort )
	ELSE_IF( aUShort )
	ELSE_IF( aLong )
	ELSE_IF( aEnumLong )
	ELSE_IF( aIdUShort )
	ELSE_IF( aIdNoZeroUShort )
	ELSE_IF( aNoZeroShort )
	ELSE_IF( a1to12Short )
	ELSE_IF( a0to23Short )
	ELSE_IF( a1to31Short )
	ELSE_IF( a0to59Short )
	ELSE_IF( a0to99Short )
	ELSE_IF( a0to9999Short )
	ELSE_IF( aIdLong )
	ELSE_IF( aString )
	ELSE_IF( aWinBits )
	ELSE_IF( aLangType )
	ELSE_IF( aLangString )
	ELSE_IF( aLangShort )

	RscTop * pEle = aBaseLst.First();
	while( pEle )
	{
		if( pEle->GetId() == nId )
			return pEle;
		pEle = aBaseLst.Next();
	}
	return NULL;
}

/*************************************************************************
|*
|*	  RscTypCont :: Search
|*
|*	  Beschreibung		RES.DOC
|*	  Ersterstellung	MM 22.03.90
|*	  Letzte Aenderung	MM 27.06.90
|*
*************************************************************************/
RscTop * RscTypCont :: Search( Atom nRT ){
	return( (RscTop *)pRoot->Search( nRT ) );
}

CLASS_DATA RscTypCont :: Search( Atom nRT, const RscId & rId ){
	ObjNode *pObjNode;
	RscTop	*pRscTop;

	if( NULL != (pRscTop = Search( nRT )) ){
		if( NULL != (pObjNode = pRscTop->GetObjNode( rId )) ){
			return( pObjNode->GetRscObj() );
		}
	}
	return( (CLASS_DATA)0 );
}

/*************************************************************************
|*
|*	  RscTypCont :: Delete()
|*
|*	  Beschreibung
|*	  Ersterstellung	MM 10.07.91
|*	  Letzte Aenderung	MM 10.07.91
|*
*************************************************************************/
void RscTypCont :: Delete( Atom nRT, const RscId & rId ){
	ObjNode *	pObjNode;
	RscTop	*	pRscTop;

	if( NULL != (pRscTop = Search( nRT )) ){
		if( NULL != (pObjNode = pRscTop->GetObjNode()) ){
			pObjNode = pObjNode->Search( rId );

			if( pObjNode ){
				//Objekt aus Baum entfernen
				pRscTop->pObjBiTree =
					(ObjNode *)pRscTop->pObjBiTree->Remove( pObjNode );

				if( pObjNode->GetRscObj() ){
					pRscTop->Destroy( RSCINST( pRscTop,
											   pObjNode->GetRscObj() ) );
					rtl_freeMemory( pObjNode->GetRscObj() );
				}
				delete pObjNode;
			}
		}
	}
}

/*************************************************************************
|*
|*	  RscTypCont :: PutSysName()
|*
|*	  Beschreibung		RES.DOC
|*	  Ersterstellung	MM 22.03.90
|*	  Letzte Aenderung	MM 27.06.90
|*
*************************************************************************/
sal_uInt32 RscTypCont :: PutSysName( sal_uInt32 nRscTyp, char * pFileName,
								 sal_uInt32 nConst, sal_uInt32 nId, BOOL bFirst )
{
	RscSysEntry *	pSysEntry;
	BOOL			bId1 = FALSE;

	pSysEntry = aSysLst.First();
	while( pSysEntry )
	{
		if( pSysEntry->nKey == 1 )
			bId1 = TRUE;
		if( !strcmp( pSysEntry->aFileName.GetBuffer(), pFileName ) )
			if( pSysEntry->nRscTyp == nRscTyp
			  && pSysEntry->nTyp == nConst
			  && pSysEntry->nRefId == nId )
				break;
		pSysEntry = aSysLst.Next();
	}

	if ( !pSysEntry || (bFirst && !bId1) )
	{
		pSysEntry = new RscSysEntry;
		pSysEntry->nKey = nUniqueId++;
		pSysEntry->nRscTyp = nRscTyp;
		pSysEntry->nTyp = nConst;
		pSysEntry->nRefId = nId;
		pSysEntry->aFileName = (const char*)pFileName;
		if( bFirst && !bId1 )
		{
			pSysEntry->nKey = 1;
			aSysLst.Insert( pSysEntry, (ULONG)0 );
		}
		else
			aSysLst.Insert( pSysEntry, LIST_APPEND );
	}

	return pSysEntry->nKey;
}

/*************************************************************************
|*
|*	  RscTypCont :: WriteInc
|*
|*	  Beschreibung		RES.DOC
|*	  Ersterstellung	MM 21.06.90
|*	  Letzte Aenderung	MM 21.06.90
|*
*************************************************************************/
void RscTypCont :: WriteInc( FILE * fOutput, ULONG lFileKey )
{
	RscFile   * pFName;

	if( NOFILE_INDEX == lFileKey )
	{
		pFName = aFileTab.First();
		while( pFName )
		{
			if( pFName && pFName->IsIncFile() )
			{
				fprintf( fOutput, "#include " );
				fprintf( fOutput, "\"%s\"\n",
								  pFName->aFileName.GetBuffer() );
			}
			pFName = aFileTab.Next();
		}
	}
	else
	{
		RscDepend * 	pDep;
		RscFile   * 	pFile;

		pFName = aFileTab.Get( lFileKey );
		if( pFName )
		{
			pDep = pFName->First();
			while( pDep )
			{
				if( pDep->GetFileKey() != lFileKey )
				{
					pFile = aFileTab.GetFile( pDep->GetFileKey() );
					if( pFile )
					{
						fprintf( fOutput, "#include " );
						fprintf( fOutput, "\"%s\"\n",
								 pFile->aFileName.GetBuffer() );
					}
				}
				pDep = pFName->Next();
			};
		};
	};
}

/*************************************************************************
|*
|*	  RscTypCont :: Methoden die ueber all Knoten laufen
|*
|*	  Beschreibung		RES.DOC
|*	  Ersterstellung	MM 22.03.90
|*	  Letzte Aenderung	MM 09.12.91
|*
*************************************************************************/

class RscEnumerateObj
{
friend class RscEnumerateRef;
private:
	ERRTYPE 	aError; 	// Enthaelt den ersten Fehler
	RscTypCont* pTypCont;
	FILE *		fOutput;	// AusgabeDatei
	ULONG		lFileKey;	// Welche src-Datei
	RscTop *	pClass;

	DECL_LINK( CallBackWriteRc, ObjNode * );
	DECL_LINK( CallBackWriteSrc, ObjNode * );
	DECL_LINK( CallBackWriteCxx, ObjNode * );
	DECL_LINK( CallBackWriteHxx, ObjNode * );

	ERRTYPE WriteRc( RscTop * pCl, ObjNode * pRoot )
	{
		pClass = pCl;
		if( pRoot )
			pRoot->EnumNodes( LINK( this, RscEnumerateObj, CallBackWriteRc ) );
		return aError;
	}
	ERRTYPE WriteSrc( RscTop * pCl, ObjNode * pRoot ){
		pClass = pCl;
		if( pRoot )
			pRoot->EnumNodes( LINK( this, RscEnumerateObj, CallBackWriteSrc ) );
		return aError;
	}
	ERRTYPE WriteCxx( RscTop * pCl, ObjNode * pRoot ){
		pClass = pCl;
		if( pRoot )
			pRoot->EnumNodes( LINK( this, RscEnumerateObj, CallBackWriteCxx ) );
		return aError;
	}
	ERRTYPE WriteHxx( RscTop * pCl, ObjNode * pRoot ){
		pClass = pCl;
		if( pRoot )
			pRoot->EnumNodes( LINK( this, RscEnumerateObj, CallBackWriteHxx ) );
		return aError;
	}
public:
	void WriteRcFile( RscWriteRc & rMem, FILE * fOutput );
};

/*************************************************************************
|*
|*	  RscEnumerateObj :: CallBackWriteRc
|*
|*	  Beschreibung
|*	  Ersterstellung	MM 09.12.91
|*	  Letzte Aenderung	MM 09.12.91
|*
*************************************************************************/
IMPL_LINK( RscEnumerateObj, CallBackWriteRc, ObjNode *, pObjNode )
{
	RscWriteRc		aMem( pTypCont->GetByteOrder() );

	aError = pClass->WriteRcHeader( RSCINST( pClass, pObjNode->GetRscObj() ),
									 aMem, pTypCont,
									 pObjNode->GetRscId(), 0, TRUE );
	if( aError.IsError() || aError.IsWarning() )
		pTypCont->pEH->Error( aError, pClass, pObjNode->GetRscId() );

	WriteRcFile( aMem, fOutput );
	return 0;
}

/*************************************************************************
|*
|*	  RscEnumerateObj :: CallBackWriteSrc
|*
|*	  Beschreibung
|*	  Ersterstellung	MM 09.12.91
|*	  Letzte Aenderung	MM 09.12.91
|*
*************************************************************************/
IMPL_LINK_INLINE_START( RscEnumerateObj, CallBackWriteSrc, ObjNode *, pObjNode )
{
	if( pObjNode->GetFileKey() == lFileKey ){
		pClass->WriteSrcHeader( RSCINST( pClass, pObjNode->GetRscObj() ),
								fOutput, pTypCont, 0,
								pObjNode->GetRscId(), "" );
		fprintf( fOutput, ";\n" );
	}
	return 0;
}
IMPL_LINK_INLINE_END( RscEnumerateObj, CallBackWriteSrc, ObjNode *, pObjNode )

/*************************************************************************
|*
|*	  RscEnumerateObj :: CallBackWriteCxx
|*
|*	  Beschreibung
|*	  Ersterstellung	MM 09.12.91
|*	  Letzte Aenderung	MM 09.12.91
|*
*************************************************************************/
IMPL_LINK_INLINE_START( RscEnumerateObj, CallBackWriteCxx, ObjNode *, pObjNode )
{
	if( pClass->IsCodeWriteable() && pObjNode->GetFileKey() == lFileKey )
		aError = pClass->WriteCxxHeader(
							  RSCINST( pClass, pObjNode->GetRscObj() ),
							  fOutput, pTypCont, pObjNode->GetRscId() );
	return 0;
}
IMPL_LINK_INLINE_END( RscEnumerateObj, CallBackWriteCxx, ObjNode *, pObjNode )

/*************************************************************************
|*
|*	  RscEnumerateObj :: CallBackWriteHxx
|*
|*	  Beschreibung
|*	  Ersterstellung	MM 09.12.91
|*	  Letzte Aenderung	MM 09.12.91
|*
*************************************************************************/
IMPL_LINK_INLINE_START( RscEnumerateObj, CallBackWriteHxx, ObjNode *, pObjNode )
{
	if( pClass->IsCodeWriteable() && pObjNode->GetFileKey() == lFileKey )
		aError = pClass->WriteHxxHeader(
							  RSCINST( pClass, pObjNode->GetRscObj() ),
							  fOutput, pTypCont, pObjNode->GetRscId() );
	return 0;
}
IMPL_LINK_INLINE_END( RscEnumerateObj, CallBackWriteHxx, ObjNode *, pObjNode )

/*************************************************************************
|*
|*	  RscEnumerateObj :: WriteRcFile
|*
|*	  Beschreibung
|*	  Ersterstellung	MM 09.12.91
|*	  Letzte Aenderung	MM 09.12.91
|*
*************************************************************************/
void RscEnumerateObj :: WriteRcFile( RscWriteRc & rMem, FILE * fOut ){
	// Definition der Struktur, aus denen die Resource aufgebaut ist
	/*
	struct RSHEADER_TYPE{
		sal_uInt32			nId;		// Identifier der Resource
		sal_uInt32			nRT;		// Resource Typ
		sal_uInt32			nGlobOff;	// Globaler Offset
		sal_uInt32			nLocalOff;	// Lokaler Offset
	} aHeader;
	*/

	sal_uInt32 nId = rMem.GetLong( 0 );
	sal_uInt32 nRT = rMem.GetLong( 4 );

	// Tabelle wird entsprechend gefuellt
	pTypCont->PutTranslatorKey( (sal_uInt64(nRT) << 32) + sal_uInt64(nId) );

	if( nRT == RSC_VERSIONCONTROL )
	{ // kommt immmer als letztes
		INT32 nCount = pTypCont->aIdTranslator.size();
		// groesse der Tabelle
		UINT32 nSize = (nCount * (sizeof(sal_uInt64)+sizeof(INT32))) + sizeof(INT32);

		rMem.Put( nCount ); //Anzahl speichern
        for( std::map< sal_uInt64, ULONG >::const_iterator it =
             pTypCont->aIdTranslator.begin(); it != pTypCont->aIdTranslator.end(); ++it )
		{
			// Schluessel schreiben
			rMem.Put( it->first );
			// Objekt Id oder Position schreiben
			rMem.Put( (INT32)it->second );
		}
		rMem.Put( nSize ); // Groesse hinten Speichern
	}

	//Dateioffset neu setzen
	pTypCont->IncFilePos( rMem.Size() );


	//Position wurde vorher in Tabelle geschrieben
	fwrite( rMem.GetBuffer(), rMem.Size(), 1, fOut );

};

class RscEnumerateRef
{
private:
	RscTop *		pRoot;

	DECL_LINK( CallBackWriteRc, RscTop * );
	DECL_LINK( CallBackWriteSrc, RscTop * );
	DECL_LINK( CallBackWriteCxx, RscTop * );
	DECL_LINK( CallBackWriteHxx, RscTop * );
	DECL_LINK( CallBackWriteSyntax, RscTop * );
	DECL_LINK( CallBackWriteRcCtor, RscTop * );
public:
	RscEnumerateObj aEnumObj;

			RscEnumerateRef( RscTypCont * pTC, RscTop * pR,
							 FILE * fOutput )
			{
				aEnumObj.pTypCont = pTC;
				aEnumObj.fOutput  = fOutput;
				pRoot			  = pR;
			}
	ERRTYPE WriteRc()
	{
		aEnumObj.aError.Clear();
		pRoot->EnumNodes( LINK( this, RscEnumerateRef, CallBackWriteRc ) );
		return aEnumObj.aError;
	};

	ERRTYPE WriteSrc( ULONG lFileKey )
	{
		aEnumObj.lFileKey = lFileKey;

		aEnumObj.aError.Clear();
		pRoot->EnumNodes( LINK( this, RscEnumerateRef, CallBackWriteSrc ) );
		return aEnumObj.aError;
	}

	ERRTYPE WriteCxx( ULONG lFileKey )
	{
		aEnumObj.lFileKey = lFileKey;

		aEnumObj.aError.Clear();
		pRoot->EnumNodes( LINK( this, RscEnumerateRef, CallBackWriteCxx ) );
		return aEnumObj.aError;
	}

	ERRTYPE WriteHxx(  ULONG lFileKey )
	{
		aEnumObj.lFileKey = lFileKey;

		aEnumObj.aError.Clear();
		pRoot->EnumNodes( LINK( this, RscEnumerateRef, CallBackWriteHxx ) );
		return aEnumObj.aError;
	}

	void	WriteSyntax()
			{
				pRoot->EnumNodes( LINK( this, RscEnumerateRef,
										CallBackWriteSyntax ) );
			}

	void	WriteRcCtor()
			{
				pRoot->EnumNodes( LINK( this, RscEnumerateRef,
										CallBackWriteRcCtor ) );
			}
};

/*************************************************************************
|*
|*	  RscRscEnumerateRef :: CallBack...
|*
|*	  Beschreibung
|*	  Ersterstellung	MM 09.12.91
|*	  Letzte Aenderung	MM 09.12.91
|*
*************************************************************************/
IMPL_LINK_INLINE_START( RscEnumerateRef, CallBackWriteRc, RscTop *, pRef )
{
	aEnumObj.WriteRc( pRef, pRef->GetObjNode() );
	return 0;
}
IMPL_LINK_INLINE_END( RscEnumerateRef, CallBackWriteRc, RscTop *, pRef )
IMPL_LINK_INLINE_START( RscEnumerateRef, CallBackWriteSrc, RscTop *, pRef )
{
	aEnumObj.WriteSrc( pRef, pRef->GetObjNode() );
	return 0;
}
IMPL_LINK_INLINE_END( RscEnumerateRef, CallBackWriteSrc, RscTop *, pRef )
IMPL_LINK_INLINE_START( RscEnumerateRef, CallBackWriteCxx, RscTop *, pRef )
{
	if( pRef->IsCodeWriteable() )
		aEnumObj.WriteCxx( pRef, pRef->GetObjNode() );
	return 0;
}
IMPL_LINK_INLINE_END( RscEnumerateRef, CallBackWriteCxx, RscTop *, pRef )
IMPL_LINK_INLINE_START( RscEnumerateRef, CallBackWriteHxx, RscTop *, pRef )
{
	if( pRef->IsCodeWriteable() )
		aEnumObj.WriteHxx( pRef, pRef->GetObjNode() );
	return 0;
}
IMPL_LINK_INLINE_END( RscEnumerateRef, CallBackWriteHxx, RscTop *, pRef )
IMPL_LINK_INLINE_START( RscEnumerateRef, CallBackWriteSyntax, RscTop *, pRef )
{
	pRef->WriteSyntaxHeader( aEnumObj.fOutput, aEnumObj.pTypCont );
	return 0;
}
IMPL_LINK_INLINE_END( RscEnumerateRef, CallBackWriteSyntax, RscTop *, pRef )
IMPL_LINK_INLINE_START( RscEnumerateRef, CallBackWriteRcCtor, RscTop *, pRef )
{
	pRef->WriteRcCtor( aEnumObj.fOutput, aEnumObj.pTypCont );
	return 0;
}
IMPL_LINK_INLINE_END( RscEnumerateRef, CallBackWriteRcCtor, RscTop *, pRef )

/*************************************************************************
|*
|*	  RscTypCont :: WriteRc
|*
|*	  Beschreibung		RES.DOC
|*	  Ersterstellung	MM 22.03.90
|*	  Letzte Aenderung	MM 22.07.91
|*
*************************************************************************/

ERRTYPE RscTypCont::WriteRc( WriteRcContext& rContext )
{
	ERRTYPE 	  aError;
	RscEnumerateRef aEnumRef( this, pRoot, rContext.fOutput );

	aIdTranslator.clear();
	nFilePos = 0;
	nPMId = RSCVERSION_ID +1; //mindestens einen groesser

	aError = aEnumRef.WriteRc();

	// version control
	RscWriteRc aMem( nByteOrder );
	aVersion.pClass->WriteRcHeader( aVersion, aMem, this, RscId( RSCVERSION_ID ), 0, TRUE );
	aEnumRef.aEnumObj.WriteRcFile( aMem, rContext.fOutput );
		
	return aError;
}

/*************************************************************************
|*
|*	  RscTypCont :: WriteSrc
|*
|*	  Beschreibung		RES.DOC
|*	  Ersterstellung	MM 22.03.90
|*	  Letzte Aenderung	MM 27.06.90
|*
*************************************************************************/
void RscTypCont :: WriteSrc( FILE * fOutput, ULONG nFileKey,
							 CharSet /*nCharSet*/, BOOL bName )
{
	RscFile 	*	pFName;
	RscEnumerateRef aEnumRef( this, pRoot, fOutput );

    unsigned char aUTF8BOM[3] = { 0xef, 0xbb, 0xbf };
    fwrite( aUTF8BOM, sizeof(unsigned char), sizeof(aUTF8BOM)/sizeof(aUTF8BOM[0]), fOutput );
	if( bName )
	{
		WriteInc( fOutput, nFileKey );

		if( NOFILE_INDEX == nFileKey )
		{
			pFName = aFileTab.First();
			while( pFName  ){
				if( !pFName->IsIncFile() )
					pFName->aDefLst.WriteAll( fOutput );
				aEnumRef.WriteSrc( aFileTab.GetIndex( pFName ) );
				pFName = aFileTab.Next();
			};
		}
		else
		{
			pFName = aFileTab.Get( nFileKey );
			if( pFName ){
				pFName->aDefLst.WriteAll( fOutput );
				aEnumRef.WriteSrc( nFileKey );
			}
		}
	}
	else
	{
		RscId::SetNames( FALSE );
		if( NOFILE_INDEX == nFileKey )
		{
			pFName = aFileTab.First();
			while( pFName  )
			{
				aEnumRef.WriteSrc( aFileTab.GetIndex( pFName ) );
				pFName = aFileTab.Next();
			};
		}
		else
			 aEnumRef.WriteSrc( nFileKey );
		RscId::SetNames();
	};
}

/*************************************************************************
|*
|*	  RscTypCont :: WriteHxx
|*
|*	  Beschreibung
|*	  Ersterstellung	MM 30.05.91
|*	  Letzte Aenderung	MM 30.05.91
|*
*************************************************************************/
ERRTYPE RscTypCont :: WriteHxx( FILE * fOutput, ULONG nFileKey )
{
	fprintf( fOutput, "#include <tools/rc.hxx>\n" );
	fprintf( fOutput, "#include <tools/resid.hxx>\n" );
	fprintf( fOutput, "#include <vcl/accel.hxx>\n" );
	fprintf( fOutput, "#include <vcl/bitmap.hxx>\n" );
	fprintf( fOutput, "#include <vcl/button.hxx>\n" );
	fprintf( fOutput, "#include <tools/color.hxx>\n" );
	fprintf( fOutput, "#include <vcl/combobox.hxx>\n" );
	fprintf( fOutput, "#include <vcl/ctrl.hxx>\n" );
	fprintf( fOutput, "#include <vcl/dialog.hxx>\n" );
	fprintf( fOutput, "#include <vcl/edit.hxx>\n" );
	fprintf( fOutput, "#include <vcl/field.hxx>\n" );
	fprintf( fOutput, "#include <vcl/fixed.hxx>\n" );
	fprintf( fOutput, "#include <vcl/group.hxx>\n" );
	fprintf( fOutput, "#include <vcl/image.hxx>\n" );
	fprintf( fOutput, "#include <vcl/imagebtn.hxx>\n" );
	fprintf( fOutput, "#include <vcl/keycod.hxx>\n" );
	fprintf( fOutput, "#include <vcl/lstbox.hxx>\n" );
	fprintf( fOutput, "#include <vcl/mapmod.hxx>\n" );
	fprintf( fOutput, "#include <vcl/menu.hxx>\n" );
	fprintf( fOutput, "#include <vcl/menubtn.hxx>\n" );
	fprintf( fOutput, "#include <vcl/morebtn.hxx>\n" );
	fprintf( fOutput, "#include <vcl/msgbox.hxx>\n" );
	fprintf( fOutput, "#include <vcl/scrbar.hxx>\n" );
	fprintf( fOutput, "#include <vcl/spin.hxx>\n" );
	fprintf( fOutput, "#include <vcl/spinfld.hxx>\n" );
	fprintf( fOutput, "#include <vcl/splitwin.hxx>\n" );
	fprintf( fOutput, "#include <vcl/status.hxx>\n" );
	fprintf( fOutput, "#include <vcl/tabctrl.hxx>\n" );
	fprintf( fOutput, "#include <vcl/tabdlg.hxx>\n" );
	fprintf( fOutput, "#include <vcl/tabpage.hxx>\n" );
	fprintf( fOutput, "#include <vcl/toolbox.hxx>\n" );
	fprintf( fOutput, "#include <vcl/window.hxx>\n" );
	fprintf( fOutput, "#include <vcl/wrkwin.hxx>\n" );
	fprintf( fOutput, "#include <svtools/svmedit.hxx>\n" );

	RscEnumerateRef aEnumRef( this, pRoot, fOutput );
	ERRTYPE 		aError;

	if( NOFILE_INDEX == nFileKey )
	{
		RscFile 	*	pFName;

		pFName = aFileTab.First();
		while( pFName  )
		{
			aError = aEnumRef.WriteHxx( aFileTab.GetIndex( pFName ) );
			pFName = aFileTab.Next();
		};
	}
	else
		aError = aEnumRef.WriteHxx( nFileKey );

	return aError;
}

/*************************************************************************
|*
|*	  RscTypCont :: WriteCxx
|*
|*	  Beschreibung
|*	  Ersterstellung	MM 30.05.91
|*	  Letzte Aenderung	MM 30.05.91
|*
*************************************************************************/
ERRTYPE RscTypCont::WriteCxx( FILE * fOutput, ULONG nFileKey,
							  const ByteString & rHxxName )
{
	RscEnumerateRef aEnumRef( this, pRoot, fOutput );
	ERRTYPE 		aError;
	fprintf( fOutput, "#include <string.h>\n" );
	WriteInc( fOutput, nFileKey );
	if( rHxxName.Len() )
		fprintf( fOutput, "#include \"%s\"\n", rHxxName.GetBuffer() );
	fprintf( fOutput, "\n\n" );

	if( NOFILE_INDEX == nFileKey )
	{
		RscFile 	*	pFName;

		pFName = aFileTab.First();
		while( pFName  )
		{
			aError = aEnumRef.WriteCxx( aFileTab.GetIndex( pFName ) );
			pFName = aFileTab.Next();
		};
	}
	else
		aError = aEnumRef.WriteCxx( nFileKey );

	return aError;
}

/*************************************************************************
|*
|*	  RscTypCont :: WriteSyntax
|*
|*	  Beschreibung
|*	  Ersterstellung	MM 30.05.91
|*	  Letzte Aenderung	MM 30.05.91
|*
*************************************************************************/
void RscTypCont::WriteSyntax( FILE * fOutput )
{
	for( sal_uInt32 i = 0; i < aBaseLst.Count(); i++ )
		aBaseLst.GetObject( i )->WriteSyntaxHeader( fOutput, this );
	RscEnumerateRef aEnumRef( this, pRoot, fOutput );
	aEnumRef.WriteSyntax();
}

//=======================================================================
void RscTypCont::WriteRcCtor
(
	FILE * fOutput
)
{
	RscEnumerateRef aEnumRef( this, pRoot, fOutput );
	aEnumRef.WriteRcCtor();
}

/*************************************************************************
|*
|*	  RscTypCont :: Delete()
|*
|*	  Beschreibung
|*	  Ersterstellung	MM 09.12.91
|*	  Letzte Aenderung	MM 09.12.91
|*
*************************************************************************/
class RscDel
{
	ULONG lFileKey;
	DECL_LINK( Delete, RscTop * );
public:
	RscDel( RscTop * pRoot, ULONG lKey );
};


inline RscDel::RscDel( RscTop * pRoot, ULONG lKey )
{
	lFileKey = lKey;
	pRoot->EnumNodes( LINK( this, RscDel, Delete ) );
}

IMPL_LINK_INLINE_START( RscDel, Delete, RscTop *, pNode )
{
	if( pNode->GetObjNode() )
		pNode->pObjBiTree = pNode->GetObjNode()->DelObjNode( pNode, lFileKey );
	return 0;
}
IMPL_LINK_INLINE_END( RscDel, Delete, RscTop *, pNode )

void RscTypCont :: Delete( ULONG lFileKey ){
	// Resourceinstanzen loeschen
	RscDel aDel( pRoot, lFileKey );
	// Defines loeschen
	aFileTab.DeleteFileContext( lFileKey );
}

/*************************************************************************
|*
|*	  RscTypCont :: MakeConsistent()
|*
|*	  Beschreibung
|*	  Ersterstellung	MM 23.09.91
|*	  Letzte Aenderung	MM 23.09.91
|*
*************************************************************************/
BOOL IsInstConsistent( ObjNode * pObjNode, RscTop * pRscTop,
					   RscInconsList * pList )
{
	BOOL bRet = TRUE;

	if( pObjNode ){
		RSCINST aTmpI;

		if( ! IsInstConsistent( (ObjNode*)pObjNode->Left(), pRscTop, pList ) )
			bRet = FALSE;

		aTmpI.pClass = pRscTop;
		aTmpI.pData = pObjNode->GetRscObj();
		if( ! aTmpI.pClass->IsConsistent( aTmpI, pList ) )
			bRet = FALSE;

		if( ! IsInstConsistent( (ObjNode*)pObjNode->Right(), pRscTop, pList ) )
			bRet = FALSE;
	};

	return( bRet );
}

BOOL MakeConsistent( RscTop * pRscTop, RscInconsList * pList )
{
	BOOL bRet = TRUE;

	if( pRscTop ){
		if( ! ::MakeConsistent( (RscTop*)pRscTop->Left(), pList ) )
			bRet = FALSE;

		if( pRscTop->GetObjNode() ){
			if( ! pRscTop->GetObjNode()->IsConsistent() ){
				pRscTop->GetObjNode()->OrderTree();
				if( ! pRscTop->GetObjNode()->IsConsistent( pList ) )
					bRet = FALSE;
			}
			if( ! IsInstConsistent( pRscTop->GetObjNode(), pRscTop, pList ) )
				bRet = FALSE;
		}

		if( ! ::MakeConsistent( (RscTop*)pRscTop->Right(), pList ) )
			bRet = FALSE;
	};

	return bRet;
}

BOOL RscTypCont :: MakeConsistent( RscInconsList * pList ){
	return( ::MakeConsistent( pRoot, pList ) );
}

sal_uInt32 RscTypCont::PutTranslatorKey( sal_uInt64 nKey )
{
    aIdTranslator[ nKey ] = nFilePos;
	return nPMId++;
}
