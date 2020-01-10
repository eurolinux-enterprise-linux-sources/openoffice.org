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
#include "precompiled_sw.hxx"


#include <limits.h>
#include <hintids.hxx>

#define _SVSTDARR_STRINGSSORT
#include <svtools/svstdarr.hxx>
#include <svx/langitem.hxx>
#include <svx/brkitem.hxx>
#include <svx/tstpitem.hxx>
#include <svx/lrspitem.hxx>
#include <sot/clsids.hxx>
#include <docsh.hxx>
#include <ndole.hxx>
#include <txttxmrk.hxx>
#include <fmtinfmt.hxx>
#include <fmtpdsc.hxx>
#include <frmfmt.hxx>
#include <fmtfsize.hxx>
#include <frmatr.hxx>
#include <pagedesc.hxx>
#include <doc.hxx>
#include <pagefrm.hxx>
#include <ndtxt.hxx>
#include <swtable.hxx>
#include <doctxm.hxx>
#include <txmsrt.hxx>
#include <rolbck.hxx>
#include <poolfmt.hxx>
#include <txtfrm.hxx>
#include <rootfrm.hxx>
#include <undobj.hxx>
#include <swundo.hxx>
#include <mdiexp.hxx>
#include <docary.hxx>
#include <charfmt.hxx>
#include <fchrfmt.hxx>
#include <fldbas.hxx>
#include <fmtfld.hxx>
#include <txtfld.hxx>
#include <expfld.hxx>
#include <chpfld.hxx>
#include <mvsave.hxx>
#include <node2lay.hxx>
#include <SwStyleNameMapper.hxx>
#include <breakit.hxx>
#include <editsh.hxx>
#include <scriptinfo.hxx>

using namespace ::com::sun::star;

const sal_Unicode cNumRepl		= '@';
const sal_Unicode cEndPageNum 	= '~';
const sal_Char __FAR_DATA sPageDeli[] = ", ";

SV_IMPL_PTRARR(SwTOXSortTabBases, SwTOXSortTabBasePtr)

TYPEINIT2( SwTOXBaseSection, SwTOXBase, SwSection );	// fuers RTTI

struct LinkStruct
{
	SwFmtINetFmt	aINetFmt;
	xub_StrLen nStartTextPos, nEndTextPos;

	LinkStruct( const String& rURL, xub_StrLen nStart, xub_StrLen nEnd )
		: aINetFmt( rURL, aEmptyStr),
		nStartTextPos( nStart),
		nEndTextPos(nEnd) {}
};

typedef LinkStruct* LinkStructPtr;
SV_DECL_PTRARR(LinkStructArr, LinkStructPtr, 0, 5 )
SV_IMPL_PTRARR(LinkStructArr, LinkStructPtr)

USHORT SwDoc::GetTOIKeys( SwTOIKeyType eTyp, SvStringsSort& rArr ) const
{
	if( rArr.Count() )
		rArr.Remove( USHORT(0), rArr.Count() );

	// dann mal ueber den Pool und alle Primary oder Secondary heraussuchen
	const SwTxtTOXMark* pMark;
	const SfxPoolItem* pItem;
    const SwTOXType* pTOXType;
	USHORT i, nMaxItems = GetAttrPool().GetItemCount( RES_TXTATR_TOXMARK );
	for( i = 0; i < nMaxItems; ++i )
		if( 0 != (pItem = GetAttrPool().GetItem( RES_TXTATR_TOXMARK, i ) ) &&
			0!= ( pTOXType = ((SwTOXMark*)pItem)->GetTOXType()) &&
            TOX_INDEX == pTOXType->GetType() &&
			0 != ( pMark = ((SwTOXMark*)pItem)->GetTxtTOXMark() ) &&
			pMark->GetpTxtNd() &&
			pMark->GetpTxtNd()->GetNodes().IsDocNodes() )
		{
			const String* pStr;
			if( TOI_PRIMARY == eTyp )
				pStr = &((SwTOXMark*)pItem)->GetPrimaryKey();
			else
				pStr = &((SwTOXMark*)pItem)->GetSecondaryKey();

			if( pStr->Len() )
				rArr.Insert( (StringPtr)pStr );
		}

	return rArr.Count();
}

/*--------------------------------------------------------------------
	 Beschreibung: aktuelle Verzeichnismarkierungen ermitteln
 --------------------------------------------------------------------*/


USHORT SwDoc::GetCurTOXMark( const SwPosition& rPos,
								SwTOXMarks& rArr ) const
{
	// suche an der Position rPos nach allen SwTOXMark's
	SwTxtNode* pTxtNd = GetNodes()[ rPos.nNode ]->GetTxtNode();
	// kein TextNode oder kein HintsArray vorhanden ??
	if( !pTxtNd || !pTxtNd->GetpSwpHints() )
		return 0;

	const SwpHints & rHts = *pTxtNd->GetpSwpHints();
	const SwTxtAttr* pHt;
	xub_StrLen nSttIdx;
	const xub_StrLen *pEndIdx;

	xub_StrLen nAktPos = rPos.nContent.GetIndex();

	for( USHORT n = 0; n < rHts.Count(); ++n )
	{
		if( RES_TXTATR_TOXMARK != (pHt = rHts[n])->Which() )
			continue;
		if( ( nSttIdx = *pHt->GetStart() ) < nAktPos )
		{
			// pruefe Ende mit ab
			if( 0 == ( pEndIdx = pHt->GetEnd() ) ||
				*pEndIdx <= nAktPos )
				continue;		// weiter suchen
		}
		else if( nSttIdx > nAktPos )
			// ist Start vom Hint groesser als rPos, dann abbrechen. Denn
			// die Attribute sind nach Start sortiert !
			break;

		const SwTOXMark* pTMark = &pHt->GetTOXMark();
		rArr.Insert( pTMark, rArr.Count() );
	}
	return rArr.Count();
}

/*--------------------------------------------------------------------
	 Beschreibung: Marke loeschen
 --------------------------------------------------------------------*/

void SwDoc::DeleteTOXMark( const SwTOXMark* pTOXMark )
{
	// hole den TextNode und
    const SwTxtTOXMark* pTxtTOXMark = pTOXMark->GetTxtTOXMark();
	ASSERT( pTxtTOXMark, "Kein TxtTOXMark, kann nicht geloescht werden" );

    SwTxtNode& rTxtNd = const_cast<SwTxtNode&>(pTxtTOXMark->GetTxtNode());
	ASSERT( rTxtNd.GetpSwpHints(), "kann nicht geloescht werden" );

	if( DoesUndo() )
	{
		// fuers Undo die Attribute sichern
		ClearRedo();
        SwUndoResetAttr* pUndo = new SwUndoResetAttr(
            SwPosition( rTxtNd, SwIndex( &rTxtNd, *pTxtTOXMark->GetStart() ) ),
            RES_TXTATR_TOXMARK );
		AppendUndo( pUndo );

        SwRegHistory aRHst( rTxtNd, &pUndo->GetHistory() );
		rTxtNd.GetpSwpHints()->Register( &aRHst );
    }

    rTxtNd.DeleteAttribute( const_cast<SwTxtTOXMark*>(pTxtTOXMark) );

    if ( DoesUndo() )
    {
        if( rTxtNd.GetpSwpHints() )
			rTxtNd.GetpSwpHints()->DeRegister();
	}
	SetModified();
}

/*--------------------------------------------------------------------
	 Beschreibung: Traveln zwischen TOXMarks
 --------------------------------------------------------------------*/

class CompareNodeCntnt
{
	ULONG nNode;
	xub_StrLen nCntnt;
public:
	CompareNodeCntnt( ULONG nNd, xub_StrLen nCnt )
		: nNode( nNd ), nCntnt( nCnt ) {}

	int operator==( const CompareNodeCntnt& rCmp )
		{ return nNode == rCmp.nNode && nCntnt == rCmp.nCntnt; }
	int operator!=( const CompareNodeCntnt& rCmp )
		{ return nNode != rCmp.nNode || nCntnt != rCmp.nCntnt; }
	int operator< ( const CompareNodeCntnt& rCmp )
		{ return nNode < rCmp.nNode ||
			( nNode == rCmp.nNode && nCntnt < rCmp.nCntnt); }
	int operator<=( const CompareNodeCntnt& rCmp )
		{ return nNode < rCmp.nNode ||
			( nNode == rCmp.nNode && nCntnt <= rCmp.nCntnt); }
	int operator> ( const CompareNodeCntnt& rCmp )
		{ return nNode > rCmp.nNode ||
			( nNode == rCmp.nNode && nCntnt > rCmp.nCntnt); }
	int operator>=( const CompareNodeCntnt& rCmp )
		{ return nNode > rCmp.nNode ||
			( nNode == rCmp.nNode && nCntnt >= rCmp.nCntnt); }
};

const SwTOXMark& SwDoc::GotoTOXMark( const SwTOXMark& rCurTOXMark,
									SwTOXSearch eDir, BOOL bInReadOnly )
{
	const SwTxtTOXMark* pMark = rCurTOXMark.GetTxtTOXMark();
	ASSERT(pMark, "pMark==0 Ungueltige TxtTOXMark");

	const SwTxtNode *pTOXSrc = pMark->GetpTxtNd();

	CompareNodeCntnt aAbsIdx( pTOXSrc->GetIndex(), *pMark->GetStart() );
	CompareNodeCntnt aPrevPos( 0, 0 );
	CompareNodeCntnt aNextPos( ULONG_MAX, STRING_NOTFOUND );
	CompareNodeCntnt aMax( 0, 0 );
	CompareNodeCntnt aMin( ULONG_MAX, STRING_NOTFOUND );

	const SwTOXMark*	pNew	= 0;
	const SwTOXMark*	pMax	= &rCurTOXMark;
	const SwTOXMark*	pMin	= &rCurTOXMark;

	const SwModify* pType = rCurTOXMark.GetRegisteredIn();
	SwClientIter	aIter( *(SwModify*)pType );

	const SwTOXMark* pTOXMark;
	const SwCntntFrm* pCFrm;
	Point aPt;
	for( pTOXMark = (SwTOXMark*)aIter.First( TYPE( SwTOXMark )); pTOXMark;
		 pTOXMark = (SwTOXMark*)aIter.Next() )
	{
		if( pTOXMark != &rCurTOXMark &&
			0 != ( pMark = pTOXMark->GetTxtTOXMark()) &&
			0 != ( pTOXSrc = pMark->GetpTxtNd() ) &&
			0 != ( pCFrm = pTOXSrc->GetFrm( &aPt, 0, FALSE )) &&
			( bInReadOnly || !pCFrm->IsProtected() ))
		{
			CompareNodeCntnt aAbsNew( pTOXSrc->GetIndex(), *pMark->GetStart() );
			switch( eDir )
			{
				//Die untenstehenden etwas komplizierter ausgefallen Ausdruecke
				//dienen dazu auch ueber Eintraege auf der selben (!) Position
				//traveln zu koennen. Wenn einer Zeit hat mag er sie mal
				//optimieren.

			case TOX_SAME_PRV:
				if( pTOXMark->GetText() != rCurTOXMark.GetText() )
					break;
				/* no break here */
			case TOX_PRV:
				if ( (aAbsNew < aAbsIdx && aAbsNew > aPrevPos &&
					  aPrevPos != aAbsIdx && aAbsNew != aAbsIdx ) ||
					 (aAbsIdx == aAbsNew &&
					  (ULONG(&rCurTOXMark) > ULONG(pTOXMark) &&
					   (!pNew ||
						(pNew && (aPrevPos < aAbsIdx ||
								  ULONG(pNew) < ULONG(pTOXMark)))))) ||
					 (aPrevPos == aAbsNew && aAbsIdx != aAbsNew &&
					  ULONG(pTOXMark) > ULONG(pNew)) )
				{
					pNew = pTOXMark;
					aPrevPos = aAbsNew;
					if ( aAbsNew >= aMax )
					{
						aMax = aAbsNew;
						pMax = pTOXMark;
					}
				}
				break;

			case TOX_SAME_NXT:
				if( pTOXMark->GetText() != rCurTOXMark.GetText() )
					break;
				/* no break here */
			case TOX_NXT:
				if ( (aAbsNew > aAbsIdx && aAbsNew < aNextPos &&
					  aNextPos != aAbsIdx && aAbsNew != aAbsIdx ) ||
					 (aAbsIdx == aAbsNew &&
					  (ULONG(&rCurTOXMark) < ULONG(pTOXMark) &&
					   (!pNew ||
						(pNew && (aNextPos > aAbsIdx ||
								  ULONG(pNew) > ULONG(pTOXMark)))))) ||
					 (aNextPos == aAbsNew && aAbsIdx != aAbsNew &&
					  ULONG(pTOXMark) < ULONG(pNew)) )
				{
					pNew = pTOXMark;
					aNextPos = aAbsNew;
					if ( aAbsNew <= aMin )
					{
						aMin = aAbsNew;
						pMin = pTOXMark;
					}
				}
				break;
			}
		}
	}


	// kein Nachfolger wurde gefunden
	// Min oder Max benutzen
	if(!pNew)
	{
		switch(eDir)
		{
		case TOX_PRV:
		case TOX_SAME_PRV:
			pNew = pMax;
			break;
		case TOX_NXT:
		case TOX_SAME_NXT:
			pNew = pMin;
			break;
		default:
			pNew = &rCurTOXMark;
		}
	}
	return *pNew;
}

/*  */

const SwTOXBaseSection* SwDoc::InsertTableOf( const SwPosition& rPos,
												const SwTOXBase& rTOX,
												const SfxItemSet* pSet,
												BOOL bExpand )
{
	StartUndo( UNDO_INSTOX, NULL );

	SwTOXBaseSection* pNew = new SwTOXBaseSection( rTOX );
	String sSectNm( rTOX.GetTOXName() );
	sSectNm = GetUniqueTOXBaseName( *rTOX.GetTOXType(), &sSectNm );
	pNew->SetTOXName(sSectNm);
	pNew->SwSection::SetName(sSectNm);
	SwPaM aPam( rPos );
    SwSection* pSect = InsertSwSection( aPam, *pNew, pSet, false );
	if( pSect )
	{
		SwSectionNode* pSectNd = pSect->GetFmt()->GetSectionNode();
		SwSection* pCl = pNew;
		pSect->GetFmt()->Add( pCl );
		pSectNd->SetNewSection( pNew );

		if( bExpand )
        {
            // OD 19.03.2003 #106329# - add value for 2nd parameter = true to
            // indicate, that a creation of a new table of content has to be performed.
            // Value of 1st parameter = default value.
            pNew->Update( 0, true );
        }
		else if( 1 == rTOX.GetTitle().Len() && IsInReading() )
		// insert title of TOX
		{
			// then insert the headline section
			SwNodeIndex aIdx( *pSectNd, +1 );

			SwTxtNode* pHeadNd = GetNodes().MakeTxtNode( aIdx,
							GetTxtCollFromPool( RES_POOLCOLL_STANDARD ) );

			String sNm( pNew->GetTOXName() );
// ??Resource
sNm.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "_Head" ));

			SwSection aSect( TOX_HEADER_SECTION, sNm );

			SwNodeIndex aStt( *pHeadNd ); aIdx--;
			SwSectionFmt* pSectFmt = MakeSectionFmt( 0 );
			GetNodes().InsertSection( aStt, *pSectFmt, aSect, &aIdx,
												TRUE, FALSE );
		}
	}
	else
		delete pNew, pNew = 0;

	EndUndo( UNDO_INSTOX, NULL );

	return pNew;
}



const SwTOXBaseSection* SwDoc::InsertTableOf( ULONG nSttNd, ULONG nEndNd,
												const SwTOXBase& rTOX,
												const SfxItemSet* pSet )
{
	// check for recursiv TOX
	SwNode* pNd = GetNodes()[ nSttNd ];
	SwSectionNode* pSectNd = pNd->FindSectionNode();
	while( pSectNd )
	{
		SectionType eT = pSectNd->GetSection().GetType();
		if( TOX_HEADER_SECTION == eT || TOX_CONTENT_SECTION == eT )
			return 0;
        pSectNd = pSectNd->StartOfSectionNode()->FindSectionNode();
	}

	// create SectionNode around the Nodes
	SwTOXBaseSection* pNew = new SwTOXBaseSection( rTOX );

	String sSectNm( rTOX.GetTOXName() );
	sSectNm = GetUniqueTOXBaseName(*rTOX.GetTOXType(), &sSectNm);
	pNew->SetTOXName(sSectNm);
	pNew->SwSection::SetName(sSectNm);

	SwNodeIndex aStt( GetNodes(), nSttNd ), aEnd( GetNodes(), nEndNd );
	SwSectionFmt* pFmt = MakeSectionFmt( 0 );
	if(pSet)
        pFmt->SetFmtAttr(*pSet);

//	--aEnd;		// im InsertSection ist Ende inclusive

	pSectNd = GetNodes().InsertSection( aStt, *pFmt, *pNew, &aEnd );
	if( pSectNd )
	{
		SwSection* pCl = pNew;
		pFmt->Add( pCl );
		pSectNd->SetNewSection( pNew );
	}
	else
	{
		delete pNew, pNew = 0;
		DelSectionFmt( pFmt );
	}

	return pNew;
}

/*--------------------------------------------------------------------
	 Beschreibung: Aktuelles Verzeichnis ermitteln
 --------------------------------------------------------------------*/

const SwTOXBase* SwDoc::GetCurTOX( const SwPosition& rPos ) const
{
	const SwNode& rNd = rPos.nNode.GetNode();
	const SwSectionNode* pSectNd = rNd.FindSectionNode();
	while( pSectNd )
	{
		SectionType eT = pSectNd->GetSection().GetType();
		if( TOX_CONTENT_SECTION == eT )
		{
			ASSERT( pSectNd->GetSection().ISA( SwTOXBaseSection ),
					"keine TOXBaseSection!" );
			SwTOXBaseSection& rTOXSect = (SwTOXBaseSection&)
												pSectNd->GetSection();
			return &rTOXSect;
		}
        pSectNd = pSectNd->StartOfSectionNode()->FindSectionNode();
	}
	return 0;
}
/* -----------------01.09.99 16:01-------------------

 --------------------------------------------------*/
const SwAttrSet& SwDoc::GetTOXBaseAttrSet(const SwTOXBase& rTOXBase) const
{
	ASSERT( rTOXBase.ISA( SwTOXBaseSection ), "no TOXBaseSection!" );
	const SwTOXBaseSection& rTOXSect = (const SwTOXBaseSection&)rTOXBase;
	SwSectionFmt* pFmt = rTOXSect.GetFmt();
	ASSERT( pFmt, "invalid TOXBaseSection!" );
	return pFmt->GetAttrSet();
}
/* -----------------02.09.99 07:48-------------------

 --------------------------------------------------*/
const SwTOXBase* SwDoc::GetDefaultTOXBase( TOXTypes eTyp, BOOL bCreate )
{
    SwTOXBase** prBase = 0;
	switch(eTyp)
	{
	case  TOX_CONTENT: 			prBase = &pDefTOXBases->pContBase; break;
	case  TOX_INDEX:            prBase = &pDefTOXBases->pIdxBase;  break;
	case  TOX_USER:             prBase = &pDefTOXBases->pUserBase; break;
	case  TOX_TABLES:           prBase = &pDefTOXBases->pTblBase;  break;
	case  TOX_OBJECTS:          prBase = &pDefTOXBases->pObjBase;  break;
	case  TOX_ILLUSTRATIONS:    prBase = &pDefTOXBases->pIllBase;  break;
	case  TOX_AUTHORITIES:		prBase = &pDefTOXBases->pAuthBase; break;
	}
	if(!(*prBase) && bCreate)
	{
		SwForm aForm(eTyp);
		const SwTOXType* pType = GetTOXType(eTyp, 0);
		(*prBase) = new SwTOXBase(pType, aForm, 0, pType->GetTypeName());
	}
	return (*prBase);
}
/* -----------------02.09.99 08:06-------------------

 --------------------------------------------------*/
void	SwDoc::SetDefaultTOXBase(const SwTOXBase& rBase)
{
    SwTOXBase** prBase = 0;
	switch(rBase.GetType())
	{
	case  TOX_CONTENT: 			prBase = &pDefTOXBases->pContBase; break;
	case  TOX_INDEX:            prBase = &pDefTOXBases->pIdxBase;  break;
	case  TOX_USER:             prBase = &pDefTOXBases->pUserBase; break;
	case  TOX_TABLES:           prBase = &pDefTOXBases->pTblBase;  break;
	case  TOX_OBJECTS:          prBase = &pDefTOXBases->pObjBase;  break;
	case  TOX_ILLUSTRATIONS:    prBase = &pDefTOXBases->pIllBase;  break;
	case  TOX_AUTHORITIES:		prBase = &pDefTOXBases->pAuthBase; break;
	}
	if(*prBase)
		delete (*prBase);
	(*prBase) = new SwTOXBase(rBase);
}

/*--------------------------------------------------------------------
	 Beschreibung: Verzeichnis loeschen
 --------------------------------------------------------------------*/


BOOL SwDoc::DeleteTOX( const SwTOXBase& rTOXBase, BOOL bDelNodes )
{
    // its only delete the TOX, not the nodes
    BOOL bRet = FALSE;
    ASSERT( rTOXBase.ISA( SwTOXBaseSection ), "keine TOXBaseSection!" );

    const SwTOXBaseSection& rTOXSect = (const SwTOXBaseSection&)rTOXBase;
    SwSectionFmt* pFmt = rTOXSect.GetFmt();
    if( pFmt )
    {
		StartUndo( UNDO_CLEARTOXRANGE, NULL );

		/* Save the start node of the TOX' section. */
		SwSectionNode * pMyNode = pFmt->GetSectionNode();
		/* Save start node of section's surrounding. */
        SwNode * pStartNd = pMyNode->StartOfSectionNode();

		/* Look for point where to move the cursors in the area to
		   delete to. This is done by first searching forward from the
		   end of the TOX' section. If no content node is found behind
		   the TOX one is searched before it. If this is not
		   successfull, too, insert new text node behind the end of
		   the TOX' section. The cursors from the TOX' section will be
		   moved to the content node found or the new text node. */

		/* Set PaM to end of TOX' section and search following content node.

		   aSearchPam will contain the point where to move the cursors
		   to. */
		SwPaM aSearchPam(*pMyNode->EndOfSectionNode());
		SwPosition aEndPos(*pStartNd->EndOfSectionNode(), 0);
		if (! aSearchPam.Move() /* no content node found */
			|| *aSearchPam.GetPoint() >= aEndPos /* content node found
													outside surrounding */
			)
		{
			/* Set PaM to beginning of TOX' section and search previous
			   content node */
			SwPaM aTmpPam(*pMyNode);
			aSearchPam = aTmpPam;
			SwPosition aStartPos(*pStartNd, 0);

			if ( ! aSearchPam.Move(fnMoveBackward) /* no content node found */
				 || *aSearchPam.GetPoint() <= aStartPos  /* content node
															found outside
															surrounding */
				 )
			{
				/* There is no content node in the surrounding of
				   TOX'. Append text node behind TOX' section. */

				SwPosition aInsPos(*pMyNode->EndOfSectionNode(), 0);
				AppendTxtNode(aInsPos);

				SwPaM aTmpPam1(aInsPos);
				aSearchPam = aTmpPam1;
			}
		}


		/* PaM containing the TOX. */
		SwPaM aPam(*pMyNode->EndOfSectionNode(), *pMyNode);

		/* Move cursors contained in TOX to point determined above. */
		PaMCorrAbs(aPam, *aSearchPam.GetPoint());

		if( !bDelNodes )
		{
			SwSections aArr( 0, 4 );
			USHORT nCnt = pFmt->GetChildSections( aArr, SORTSECT_NOT, FALSE );
			for( USHORT n = 0; n < nCnt; ++n )
			{
				SwSection* pSect = aArr[ n ];
				if( TOX_HEADER_SECTION == pSect->GetType() )
				{
					DelSectionFmt( pSect->GetFmt(), bDelNodes );
				}
			}
		}

		DelSectionFmt( pFmt, bDelNodes );

		EndUndo( UNDO_CLEARTOXRANGE, NULL );
		bRet = TRUE;
    }

    return bRet;
}

/*--------------------------------------------------------------------
	 Beschreibung:	Verzeichnistypen verwalten
 --------------------------------------------------------------------*/

USHORT SwDoc::GetTOXTypeCount(TOXTypes eTyp) const
{
	const SwTOXTypePtr * ppTTypes = pTOXTypes->GetData();
	USHORT nCnt = 0;
	for( USHORT n = 0; n < pTOXTypes->Count(); ++n, ++ppTTypes )
		if( eTyp == (*ppTTypes)->GetType() )
			++nCnt;
	return nCnt;
}
/*--------------------------------------------------------------------

 --------------------------------------------------------------------*/
const SwTOXType* SwDoc::GetTOXType( TOXTypes eTyp, USHORT nId ) const
{
	const SwTOXTypePtr * ppTTypes = pTOXTypes->GetData();
	USHORT nCnt = 0;
	for( USHORT n = 0; n < pTOXTypes->Count(); ++n, ++ppTTypes )
		if( eTyp == (*ppTTypes)->GetType() && nCnt++ == nId )
			return (*ppTTypes);
	return 0;
}

/*--------------------------------------------------------------------

 --------------------------------------------------------------------*/
const SwTOXType* SwDoc::InsertTOXType( const SwTOXType& rTyp )
{
	SwTOXType * pNew = new SwTOXType( rTyp );
	pTOXTypes->Insert( pNew, pTOXTypes->Count() );
	return pNew;
}
/*--------------------------------------------------------------------

 --------------------------------------------------------------------*/
String SwDoc::GetUniqueTOXBaseName( const SwTOXType& rType,
									const String* pChkStr ) const
{
	USHORT n;
	const SwSectionNode* pSectNd;
	const SwSection* pSect;

	if(pChkStr && !pChkStr->Len())
		pChkStr = 0;
	String aName( rType.GetTypeName() );
	xub_StrLen nNmLen = aName.Len();

    USHORT nNum = 0;
    USHORT nTmp = 0;
    USHORT nFlagSize = ( pSectionFmtTbl->Count() / 8 ) +2;
	BYTE* pSetFlags = new BYTE[ nFlagSize ];
	memset( pSetFlags, 0, nFlagSize );

	for( n = 0; n < pSectionFmtTbl->Count(); ++n )
		if( 0 != ( pSectNd = (*pSectionFmtTbl)[ n ]->GetSectionNode( FALSE ) )&&
			 TOX_CONTENT_SECTION == (pSect = &pSectNd->GetSection())->GetType())
		{
			const String& rNm = pSect->GetName();
			if( rNm.Match( aName ) == nNmLen )
			{
				// Nummer bestimmen und das Flag setzen
				nNum = (USHORT)rNm.Copy( nNmLen ).ToInt32();
				if( nNum-- && nNum < pSectionFmtTbl->Count() )
					pSetFlags[ nNum / 8 ] |= (0x01 << ( nNum & 0x07 ));
			}
			if( pChkStr && pChkStr->Equals( rNm ) )
				pChkStr = 0;
		}

	if( !pChkStr )
	{
		// alle Nummern entsprechend geflag, also bestimme die richtige Nummer
		nNum = pSectionFmtTbl->Count();
		for( n = 0; n < nFlagSize; ++n )
			if( 0xff != ( nTmp = pSetFlags[ n ] ))
			{
				// also die Nummer bestimmen
				nNum = n * 8;
				while( nTmp & 1 )
					++nNum, nTmp >>= 1;
				break;
			}
	}
	delete [] pSetFlags;
	if( pChkStr )
		return *pChkStr;
	return aName += String::CreateFromInt32( ++nNum );
}

/*--------------------------------------------------------------------

 --------------------------------------------------------------------*/
BOOL SwDoc::SetTOXBaseName(const SwTOXBase& rTOXBase, const String& rName)
{
	ASSERT( rTOXBase.ISA( SwTOXBaseSection ),
					"keine TOXBaseSection!" );
	SwTOXBaseSection* pTOX = (SwTOXBaseSection*)&rTOXBase;

	String sTmp = GetUniqueTOXBaseName(*rTOXBase.GetTOXType(), &rName);
	BOOL bRet = sTmp == rName;
	if(bRet)
	{
		pTOX->SetTOXName(rName);
		pTOX->SwTOXBaseSection::SetName(rName);
		SetModified();
	}
	return bRet;
}

/*  */

const SwTxtNode* lcl_FindChapterNode( const SwNode& rNd, BYTE nLvl = 0 )
{
	const SwNode* pNd = &rNd;
	if( pNd->GetNodes().GetEndOfExtras().GetIndex() > pNd->GetIndex() )
	{
		// then find the "Anchor" (Body) position
		Point aPt;
		SwNode2Layout aNode2Layout( *pNd, pNd->GetIndex() );
		const SwFrm* pFrm = aNode2Layout.GetFrm( &aPt, 0, FALSE );

		if( pFrm )
		{
			SwPosition aPos( *pNd );
			pNd = GetBodyTxtNode( *pNd->GetDoc(), aPos, *pFrm );
			ASSERT( pNd,	"wo steht der Absatz" );
		}
	}
	return pNd ? pNd->FindOutlineNodeOfLevel( nLvl ) : 0;
}


/*--------------------------------------------------------------------
	 Beschreibung: Verzeichnis-Klasse
 --------------------------------------------------------------------*/

SwTOXBaseSection::SwTOXBaseSection( const SwTOXBase& rBase )
	: SwTOXBase( rBase ), SwSection( TOX_CONTENT_SECTION, aEmptyStr )
{
	SetProtect( rBase.IsProtected() );
	SwSection::SetName( GetTOXName() );
}


SwTOXBaseSection::~SwTOXBaseSection()
{
}


BOOL SwTOXBaseSection::SetPosAtStartEnd( SwPosition& rPos, BOOL bAtStart ) const
{
	BOOL bRet = FALSE;
	const SwSectionNode* pSectNd = GetFmt()->GetSectionNode();
	if( pSectNd )
	{
		SwCntntNode* pCNd;
		xub_StrLen nC = 0;
		if( bAtStart )
		{
			rPos.nNode = *pSectNd;
			pCNd = pSectNd->GetDoc()->GetNodes().GoNext( &rPos.nNode );
		}
		else
		{
			rPos.nNode = *pSectNd->EndOfSectionNode();
			pCNd = pSectNd->GetDoc()->GetNodes().GoPrevious( &rPos.nNode );
			if( pCNd ) nC = pCNd->Len();
		}
		rPos.nContent.Assign( pCNd, nC );
		bRet = TRUE;
	}
	return bRet;
}

/*--------------------------------------------------------------------
	 Beschreibung: Verzeichnisinhalt zusammensammeln
 --------------------------------------------------------------------*/

void SwTOXBaseSection::Update(const SfxItemSet* pAttr,
                              const bool        _bNewTOX )
{
	const SwSectionNode* pSectNd;
	if( !SwTOXBase::GetRegisteredIn()->GetDepends() ||
		!GetFmt() || 0 == (pSectNd = GetFmt()->GetSectionNode() ) ||
		!pSectNd->GetNodes().IsDocNodes() ||
		IsHiddenFlag() )
		return;

	SwDoc* pDoc = (SwDoc*)pSectNd->GetDoc();

    DBG_ASSERT(pDoc != NULL, "Where is the document?");

	if(pAttr && pDoc && GetFmt())
		pDoc->ChgFmt(*GetFmt(), *pAttr);

    // OD 18.03.2003 #106329# - determine default page description, which
    // will be used by the content nodes, if no approriate one is found.
    const SwPageDesc* pDefaultPageDesc;
    {
        pDefaultPageDesc =
            pSectNd->GetSection().GetFmt()->GetPageDesc().GetPageDesc();
        if ( !_bNewTOX && !pDefaultPageDesc )
        {
            // determine page description of table-of-content
            sal_uInt32 nPgDescNdIdx = pSectNd->GetIndex() + 1;
            sal_uInt32* pPgDescNdIdx = &nPgDescNdIdx;
            pDefaultPageDesc = pSectNd->FindPageDesc( FALSE, pPgDescNdIdx );
            if ( nPgDescNdIdx < pSectNd->GetIndex() )
            {
                pDefaultPageDesc = 0;
            }
        }
        // OD 28.04.2003 #109166# - consider end node of content section in the
        // node array.
        if ( !pDefaultPageDesc &&
             ( pSectNd->EndOfSectionNode()->GetIndex() <
                 (pSectNd->GetNodes().GetEndOfContent().GetIndex() - 1) )
           )
        {
            // determine page description of content after table-of-content
            SwNodeIndex aIdx( *(pSectNd->EndOfSectionNode()) );
            const SwCntntNode* pNdAfterTOX = pSectNd->GetNodes().GoNext( &aIdx );
            const SwAttrSet& aNdAttrSet = pNdAfterTOX->GetSwAttrSet();
            const SvxBreak eBreak = aNdAttrSet.GetBreak().GetBreak();
            if ( !( eBreak == SVX_BREAK_PAGE_BEFORE ||
                    eBreak == SVX_BREAK_PAGE_BOTH )
               )
            {
                pDefaultPageDesc = pNdAfterTOX->FindPageDesc( FALSE );
            }
        }
        // OD 28.04.2003 #109166# - consider start node of content section in
        // the node array.
        if ( !pDefaultPageDesc &&
             ( pSectNd->GetIndex() >
                 (pSectNd->GetNodes().GetEndOfContent().StartOfSectionIndex() + 1) )
           )
        {
            // determine page description of content before table-of-content
            SwNodeIndex aIdx( *pSectNd );
            pDefaultPageDesc =
                pSectNd->GetNodes().GoPrevious( &aIdx )->FindPageDesc( FALSE );

        }
        if ( !pDefaultPageDesc )
        {
            // determine default page description
            pDefaultPageDesc =
                &const_cast<const SwDoc *>(pDoc)->GetPageDesc( 0 );
        }
    }

    pDoc->SetModified();

	// get current Language
    SwTOXInternational aIntl(  GetLanguage(),
                               TOX_INDEX == GetTOXType()->GetType() ?
                               GetOptions() : 0,
                               GetSortAlgorithm() );

	aSortArr.DeleteAndDestroy( 0, aSortArr.Count() );

	// find the first layout node for this TOX, if it only find the content
	// in his own chapter
	const SwTxtNode* pOwnChapterNode = IsFromChapter()
			? ::lcl_FindChapterNode( *pSectNd, 0 )
			: 0;

	SwNode2Layout aN2L( *pSectNd );
	((SwSectionNode*)pSectNd)->DelFrms();

	// remove old content an insert one empty textnode (to hold the layout!)
	SwTxtNode* pFirstEmptyNd;
	{
		pDoc->DeleteRedline( *pSectNd, true, USHRT_MAX );

		SwNodeIndex aSttIdx( *pSectNd, +1 );
		SwNodeIndex aEndIdx( *pSectNd->EndOfSectionNode() );
		pFirstEmptyNd = pDoc->GetNodes().MakeTxtNode( aEndIdx,
						pDoc->GetTxtCollFromPool( RES_POOLCOLL_TEXT ) );

		{
			// Task 70995 - save and restore PageDesc and Break Attributes
			SwNodeIndex aNxtIdx( aSttIdx );
			const SwCntntNode* pCNd = aNxtIdx.GetNode().GetCntntNode();
			if( !pCNd )
				pCNd = pDoc->GetNodes().GoNext( &aNxtIdx );
            if( pCNd->HasSwAttrSet() )
			{
				SfxItemSet aBrkSet( pDoc->GetAttrPool(), aBreakSetRange );
				aBrkSet.Put( *pCNd->GetpSwAttrSet() );
				if( aBrkSet.Count() )
                    pFirstEmptyNd->SetAttr( aBrkSet );
			}
		}
		aEndIdx--;
		SwPosition aPos( aEndIdx, SwIndex( pFirstEmptyNd, 0 ));
		pDoc->CorrAbs( aSttIdx, aEndIdx, aPos, TRUE );

		// delete all before
		DelFlyInRange( aSttIdx, aEndIdx );
		_DelBookmarks( aSttIdx, aEndIdx );

		pDoc->GetNodes().Delete( aSttIdx, aEndIdx.GetIndex() - aSttIdx.GetIndex() );

	}

	//
	// insert title of TOX
	if( GetTitle().Len() )
	{
		// then insert the headline section
		SwNodeIndex aIdx( *pSectNd, +1 );

		SwTxtNode* pHeadNd = pDoc->GetNodes().MakeTxtNode( aIdx,
								GetTxtFmtColl( FORM_TITLE ) );
        pHeadNd->InsertText( GetTitle(), SwIndex( pHeadNd ) );

		String sNm( GetTOXName() );
// ??Resource
sNm.AppendAscii( RTL_CONSTASCII_STRINGPARAM( "_Head" ));

		SwSection aSect( TOX_HEADER_SECTION, sNm );

		SwNodeIndex aStt( *pHeadNd ); aIdx--;
		SwSectionFmt* pSectFmt = pDoc->MakeSectionFmt( 0 );
		pDoc->GetNodes().InsertSection( aStt, *pSectFmt, aSect, &aIdx,
										TRUE, FALSE );
	}

	// jetzt waere ein prima Zeitpunkt, um die Numerierung zu updaten
	pDoc->UpdateNumRule();

    if( GetCreateType() & nsSwTOXElement::TOX_MARK )
		UpdateMarks( aIntl, pOwnChapterNode );

    if( GetCreateType() & nsSwTOXElement::TOX_OUTLINELEVEL )
		UpdateOutline( pOwnChapterNode );

    if( GetCreateType() & nsSwTOXElement::TOX_TEMPLATE )
		UpdateTemplate( pOwnChapterNode );

    if( GetCreateType() & nsSwTOXElement::TOX_OLE ||
			TOX_OBJECTS == SwTOXBase::GetType())
        UpdateCntnt( nsSwTOXElement::TOX_OLE, pOwnChapterNode );

    if( GetCreateType() & nsSwTOXElement::TOX_TABLE ||
			(TOX_TABLES == SwTOXBase::GetType() && IsFromObjectNames()) )
		UpdateTable( pOwnChapterNode );

    if( GetCreateType() & nsSwTOXElement::TOX_GRAPHIC ||
		(TOX_ILLUSTRATIONS == SwTOXBase::GetType() && IsFromObjectNames()))
        UpdateCntnt( nsSwTOXElement::TOX_GRAPHIC, pOwnChapterNode );

	if( GetSequenceName().Len() && !IsFromObjectNames() &&
		(TOX_TABLES == SwTOXBase::GetType() ||
		 TOX_ILLUSTRATIONS == SwTOXBase::GetType() ) )
		UpdateSequence( pOwnChapterNode );

    if( GetCreateType() & nsSwTOXElement::TOX_FRAME )
        UpdateCntnt( nsSwTOXElement::TOX_FRAME, pOwnChapterNode );

	if(TOX_AUTHORITIES == SwTOXBase::GetType())
		UpdateAuthorities( aIntl );

	// Bei Bedarf Alphadelimitter einfuegen (nur bei Stichwoertern)
	//
	if( TOX_INDEX == SwTOXBase::GetType() &&
        ( GetOptions() & nsSwTOIOptions::TOI_ALPHA_DELIMITTER ) )
		InsertAlphaDelimitter( aIntl );

	// sortierte Liste aller Verzeichnismarken und Verzeichnisbereiche
	void* p = 0;
	String* pStr = 0;
	USHORT nCnt = 0, nFormMax = GetTOXForm().GetFormMax();
	SvStringsDtor aStrArr( (BYTE)nFormMax );
	SvPtrarr aCollArr( (BYTE)nFormMax );
	for( ; nCnt < nFormMax; ++nCnt )
	{
		aCollArr.Insert( p, nCnt );
		aStrArr.Insert( pStr, nCnt );
	}

	SwNodeIndex aInsPos( *pFirstEmptyNd, 1 );
	for( nCnt = 0; nCnt < aSortArr.Count(); ++nCnt )
	{
		::SetProgressState( 0, pDoc->GetDocShell() );

		// setze den Text in das Verzeichniss
		USHORT nLvl = aSortArr[ nCnt ]->GetLevel();
		SwTxtFmtColl* pColl = (SwTxtFmtColl*)aCollArr[ nLvl ];
		if( !pColl )
		{
			pColl = GetTxtFmtColl( nLvl );
			aCollArr.Remove( nLvl );
			p = pColl;
			aCollArr.Insert( p , nLvl );
		}

		// Generierung: dynamische TabStops setzen
		SwTxtNode* pTOXNd = pDoc->GetNodes().MakeTxtNode( aInsPos , pColl );
		aSortArr[ nCnt ]->pTOXNd = pTOXNd;

		// Generierung: Form auswerten und Platzhalter
		//				fuer die Seitennummer eintragen
		//if it is a TOX_INDEX and the SwForm IsCommaSeparated()
		// then a range of entries must be generated into one paragraph
		USHORT nRange = 1;
		if(TOX_INDEX == SwTOXBase::GetType() &&
				GetTOXForm().IsCommaSeparated() &&
				aSortArr[nCnt]->GetType() == TOX_SORT_INDEX)
		{
			const SwTOXMark& rMark = aSortArr[nCnt]->pTxtMark->GetTOXMark();
			const String sPrimKey = rMark.GetPrimaryKey();
			const String sSecKey = rMark.GetSecondaryKey();
			const SwTOXMark* pNextMark = 0;
			while(aSortArr.Count() > (nCnt + nRange)&&
					aSortArr[nCnt + nRange]->GetType() == TOX_SORT_INDEX &&
					0 != (pNextMark = &(aSortArr[nCnt + nRange]->pTxtMark->GetTOXMark())) &&
					pNextMark->GetPrimaryKey() == sPrimKey &&
					pNextMark->GetSecondaryKey() == sSecKey)
				nRange++;
		}
        // OD 18.03.2003 #106329# - pass node index of table-of-content section
        // and default page description to method <GenerateText(..)>.
        GenerateText( nCnt, nRange, aStrArr, pSectNd->GetIndex(), pDefaultPageDesc );
		nCnt += nRange - 1;
	}

	// delete the first dummy node and remove all Cursor into the prev node
	aInsPos = *pFirstEmptyNd;
	{
		SwPaM aCorPam( *pFirstEmptyNd );
		aCorPam.GetPoint()->nContent.Assign( pFirstEmptyNd, 0 );
		if( !aCorPam.Move( fnMoveForward ) )
			aCorPam.Move( fnMoveBackward );
		SwNodeIndex aEndIdx( aInsPos, 1 );
		pDoc->CorrAbs( aInsPos, aEndIdx, *aCorPam.GetPoint(), TRUE );

		// Task 70995 - save and restore PageDesc and Break Attributes
        if( pFirstEmptyNd->HasSwAttrSet() )
		{
			if( GetTitle().Len() )
				aEndIdx = *pSectNd;
			else
				aEndIdx = *pFirstEmptyNd;
			SwCntntNode* pCNd = pDoc->GetNodes().GoNext( &aEndIdx );
            if( pCNd ) // Robust against defect documents, e.g. i60336
                pCNd->SetAttr( *pFirstEmptyNd->GetpSwAttrSet() );
		}
	}

	// now create the new Frames
	ULONG nIdx = pSectNd->GetIndex();
	// don't delete if index is empty
	if(nIdx + 2 < pSectNd->EndOfSectionIndex())
		pDoc->GetNodes().Delete( aInsPos, 1 );

	aN2L.RestoreUpperFrms( pDoc->GetNodes(), nIdx, nIdx + 1 );
	if(pDoc->GetRootFrm())
		SwFrm::CheckPageDescs( (SwPageFrm*)pDoc->GetRootFrm()->Lower() );

	SetProtect( SwTOXBase::IsProtected() );
}

/*--------------------------------------------------------------------
	 Beschreibung: AlphaDelimitter einfuegen
 --------------------------------------------------------------------*/


void SwTOXBaseSection::InsertAlphaDelimitter( const SwTOXInternational& rIntl )
{
	SwDoc* pDoc = (SwDoc*)GetFmt()->GetDoc();
	String sDeli, sLastDeli;
	USHORT	i = 0;
	while( i < aSortArr.Count() )
	{
		::SetProgressState( 0, pDoc->GetDocShell() );

		USHORT nLevel = aSortArr[i]->GetLevel();

		// Alpha-Delimitter ueberlesen
		if( nLevel == FORM_ALPHA_DELIMITTER )
			continue;

        String sMyString, sMyStringReading;
        aSortArr[i]->GetTxt( sMyString, sMyStringReading );

        sDeli = rIntl.GetIndexKey( sMyString, sMyStringReading,
                                   aSortArr[i]->GetLocale() );

		// Delimitter schon vorhanden ??
		if( sDeli.Len() && sLastDeli != sDeli )
		{
			// alle kleiner Blank wollen wir nicht haben -> sind Sonderzeichen
			if( ' ' <= sDeli.GetChar( 0 ) )
			{
                SwTOXCustom* pCst = new SwTOXCustom( sDeli, aEmptyStr, FORM_ALPHA_DELIMITTER,
                                                     rIntl, aSortArr[i]->GetLocale() );
				aSortArr.Insert( pCst, i++ );
			}
			sLastDeli = sDeli;
		}

		// Skippen bis gleibhes oder kleineres Level erreicht ist
		do {
			i++;
		} while (i < aSortArr.Count() && aSortArr[i]->GetLevel() > nLevel);
	}
}

/*--------------------------------------------------------------------
	 Beschreibung: Template  auswerten
 --------------------------------------------------------------------*/

SwTxtFmtColl* SwTOXBaseSection::GetTxtFmtColl( USHORT nLevel )
{
	SwDoc* pDoc = (SwDoc*)GetFmt()->GetDoc();
	const String& rName = GetTOXForm().GetTemplate( nLevel );
	SwTxtFmtColl* pColl = rName.Len() ? pDoc->FindTxtFmtCollByName(rName) :0;
	if( !pColl )
	{
        USHORT nPoolFmt = 0;
		const TOXTypes eMyType = SwTOXBase::GetType();
		switch( eMyType )
		{
		case TOX_INDEX:			nPoolFmt = RES_POOLCOLL_TOX_IDXH; 		break;
		case TOX_USER:
			if( nLevel < 6 )
				nPoolFmt = RES_POOLCOLL_TOX_USERH;
			else
				nPoolFmt = RES_POOLCOLL_TOX_USER6 - 6;
			break;
		case TOX_ILLUSTRATIONS: nPoolFmt = RES_POOLCOLL_TOX_ILLUSH; 	break;
		case TOX_OBJECTS:		nPoolFmt = RES_POOLCOLL_TOX_OBJECTH; 	break;
		case TOX_TABLES:		nPoolFmt = RES_POOLCOLL_TOX_TABLESH; 	break;
		case TOX_AUTHORITIES:	nPoolFmt = RES_POOLCOLL_TOX_AUTHORITIESH; break;

		case TOX_CONTENT:
			// im Content Bereich gibt es einen Sprung!
			if( nLevel < 6 )
				nPoolFmt = RES_POOLCOLL_TOX_CNTNTH;
			else
				nPoolFmt = RES_POOLCOLL_TOX_CNTNT6 - 6;
			break;
		}

		if(eMyType == TOX_AUTHORITIES && nLevel)
			nPoolFmt = nPoolFmt + 1;
		else if(eMyType == TOX_INDEX && nLevel)
		{
			//pool: Level 1,2,3, Delimiter
			//SwForm: Delimiter, Level 1,2,3
			nPoolFmt += 1 == nLevel ? nLevel + 3 : nLevel - 1;
		}
		else
			nPoolFmt = nPoolFmt + nLevel;
		pColl = pDoc->GetTxtCollFromPool( nPoolFmt );
	}
	return pColl;
}


/*--------------------------------------------------------------------
	 Beschreibung: Aus Markierungen erzeugen
 --------------------------------------------------------------------*/

void SwTOXBaseSection::UpdateMarks( const SwTOXInternational& rIntl,
									const SwTxtNode* pOwnChapterNode )
{
	const SwModify* pType = SwTOXBase::GetRegisteredIn();
	if( !pType->GetDepends() )
		return;

	SwDoc* pDoc = (SwDoc*)GetFmt()->GetDoc();
	TOXTypes eTOXTyp = GetTOXType()->GetType();
	SwClientIter aIter( *(SwModify*)pType );

	SwTxtTOXMark* pTxtMark;
	SwTOXMark* pMark;
	for( pMark = (SwTOXMark*)aIter.First( TYPE( SwTOXMark )); pMark;
		pMark = (SwTOXMark*)aIter.Next() )
	{
		::SetProgressState( 0, pDoc->GetDocShell() );

		if( pMark->GetTOXType()->GetType() == eTOXTyp &&
			0 != ( pTxtMark = pMark->GetTxtTOXMark() ) )
		{
			const SwTxtNode* pTOXSrc = pTxtMark->GetpTxtNd();
			// nur TOXMarks einfuegen die im Doc stehen
			// nicht die, die im UNDO stehen
			//
			// if selected use marks from the same chapter only
			if( pTOXSrc->GetNodes().IsDocNodes() &&
				pTOXSrc->GetTxt().Len() && pTOXSrc->GetDepends() &&
                pTOXSrc->GetFrm() &&
               (!IsFromChapter() || ::lcl_FindChapterNode( *pTOXSrc, 0 ) == pOwnChapterNode ) &&
               !pTOXSrc->HasHiddenParaField() &&
               !SwScriptInfo::IsInHiddenRange( *pTOXSrc, *pTxtMark->GetStart() ) )
			{
				SwTOXSortTabBase* pBase = 0;
				if(TOX_INDEX == eTOXTyp)
				{
					// Stichwortverzeichnismarkierung
                    lang::Locale aLocale;
                    if ( pBreakIt->GetBreakIter().is() )
                    {
                        aLocale = pBreakIt->GetLocale(
                                        pTOXSrc->GetLang( *pTxtMark->GetStart() ) );
                    }

					pBase = new SwTOXIndex( *pTOXSrc, pTxtMark,
                                            GetOptions(), FORM_ENTRY, rIntl, aLocale );
					InsertSorted(pBase);
                    if(GetOptions() & nsSwTOIOptions::TOI_KEY_AS_ENTRY &&
						pTxtMark->GetTOXMark().GetPrimaryKey().Len())
					{
						pBase = new SwTOXIndex( *pTOXSrc, pTxtMark,
                                                GetOptions(), FORM_PRIMARY_KEY, rIntl, aLocale );
						InsertSorted(pBase);
						if(pTxtMark->GetTOXMark().GetSecondaryKey().Len())
						{
							pBase = new SwTOXIndex( *pTOXSrc, pTxtMark,
                                                    GetOptions(), FORM_SECONDARY_KEY, rIntl, aLocale );
							InsertSorted(pBase);
						}
					}
				}
				else if( TOX_USER == eTOXTyp ||
					pMark->GetLevel() <= GetLevel())
				{	// Inhaltsberzeichnismarkierung
					// also used for user marks
					pBase = new SwTOXContent( *pTOXSrc, pTxtMark, rIntl );
					InsertSorted(pBase);
				}
			}
		}
	}
}


/*--------------------------------------------------------------------
	 Beschreibung:	Verzeichnisinhalt aus Gliederungsebene generieren
 --------------------------------------------------------------------*/


void SwTOXBaseSection::UpdateOutline( const SwTxtNode* pOwnChapterNode )
{
	SwDoc* pDoc = (SwDoc*)GetFmt()->GetDoc();
	SwNodes& rNds = pDoc->GetNodes();

	const SwOutlineNodes& rOutlNds = rNds.GetOutLineNds();
	for( USHORT n = 0; n < rOutlNds.Count(); ++n )
	{
		::SetProgressState( 0, pDoc->GetDocShell() );
		SwTxtNode* pTxtNd = rOutlNds[ n ]->GetTxtNode();
		if( pTxtNd && pTxtNd->Len() && pTxtNd->GetDepends() &&
			//USHORT(pTxtNd->GetTxtColl()->GetOutlineLevel()+1) <= GetLevel() &&	//#outline level,zhaojianwei
			USHORT( pTxtNd->GetAttrOutlineLevel()) <= GetLevel() &&	//<-end,zhaojianwei
			pTxtNd->GetFrm() &&
           !pTxtNd->HasHiddenParaField() &&
           !pTxtNd->HasHiddenCharAttribute( true ) &&
            ( !IsFromChapter() ||
			   ::lcl_FindChapterNode( *pTxtNd, 0 ) == pOwnChapterNode ))
		{
            SwTOXPara * pNew = new SwTOXPara( *pTxtNd, nsSwTOXElement::TOX_OUTLINELEVEL );
			InsertSorted( pNew );
		}
	}
}

/*--------------------------------------------------------------------
	 Beschreibung: Verzeichnisinhalt aus Vorlagenbereichen generieren
 --------------------------------------------------------------------*/

void SwTOXBaseSection::UpdateTemplate( const SwTxtNode* pOwnChapterNode )
{
	SwDoc* pDoc = (SwDoc*)GetFmt()->GetDoc();
	for(USHORT i = 0; i < MAXLEVEL; i++)
	{
		String sTmpStyleNames = GetStyleNames(i);
		USHORT nTokenCount = sTmpStyleNames.GetTokenCount(TOX_STYLE_DELIMITER);
		for( USHORT nStyle = 0; nStyle < nTokenCount; ++nStyle )
		{
			SwTxtFmtColl* pColl = pDoc->FindTxtFmtCollByName(
									sTmpStyleNames.GetToken( nStyle,
													TOX_STYLE_DELIMITER ));
			//TODO: no outline Collections in content indexes if OutlineLevels are already included
			if( !pColl ||
				( TOX_CONTENT == SwTOXBase::GetType() &&
                  GetCreateType() & nsSwTOXElement::TOX_OUTLINELEVEL &&
				  //NO_NUMBERING != pColl->GetOutlineLevel() ) )//#outline level,zhaojianwei
					pColl->IsAssignedToListLevelOfOutlineStyle()) )//<-end,zhaojianwei
				  continue;

			SwClientIter aIter( *pColl );
			SwTxtNode* pTxtNd = (SwTxtNode*)aIter.First( TYPE( SwTxtNode ));
			for( ; pTxtNd; pTxtNd = (SwTxtNode*)aIter.Next() )
			{
				::SetProgressState( 0, pDoc->GetDocShell() );

				if( pTxtNd->GetTxt().Len() && pTxtNd->GetFrm() &&
					pTxtNd->GetNodes().IsDocNodes() &&
					( !IsFromChapter() || pOwnChapterNode ==
						::lcl_FindChapterNode( *pTxtNd, 0 ) ) )
				{
                    SwTOXPara * pNew = new SwTOXPara( *pTxtNd, nsSwTOXElement::TOX_TEMPLATE, i + 1 );
					InsertSorted(pNew);
				}
			}
		}
	}
}

/* -----------------14.07.99 09:59-------------------
	Description: generate content from sequence fields
 --------------------------------------------------*/
void SwTOXBaseSection::UpdateSequence( const SwTxtNode* pOwnChapterNode )
{
	SwDoc* pDoc = (SwDoc*)GetFmt()->GetDoc();
	SwFieldType* pSeqFld = pDoc->GetFldType(RES_SETEXPFLD, GetSequenceName(), false);
	if(!pSeqFld)
		return;

	SwClientIter aIter( *pSeqFld );
	SwFmtFld* pFmtFld = (SwFmtFld*)aIter.First( TYPE( SwFmtFld ));
	for( ; pFmtFld; pFmtFld = (SwFmtFld*)aIter.Next() )
	{
		const SwTxtFld* pTxtFld = pFmtFld->GetTxtFld();
		if(!pTxtFld)
			continue;
		const SwTxtNode& rTxtNode = pTxtFld->GetTxtNode();
		::SetProgressState( 0, pDoc->GetDocShell() );

		if( rTxtNode.GetTxt().Len() && rTxtNode.GetFrm() &&
			rTxtNode.GetNodes().IsDocNodes() &&
			( !IsFromChapter() ||
				::lcl_FindChapterNode( rTxtNode, 0 ) == pOwnChapterNode ) )
		{
            SwTOXPara * pNew = new SwTOXPara( rTxtNode, nsSwTOXElement::TOX_SEQUENCE, 1 );
			//set indexes if the number or the reference text are to be displayed
			if( GetCaptionDisplay() == CAPTION_TEXT )
			{
				pNew->SetStartIndex(
					SwGetExpField::GetReferenceTextPos( *pFmtFld, *pDoc ));
			}
			else if(GetCaptionDisplay() == CAPTION_NUMBER)
			{
				pNew->SetEndIndex(*pTxtFld->GetStart() + 1);
			}
			InsertSorted(pNew);
		}
	}
}
/* -----------------15.09.99 14:18-------------------

 --------------------------------------------------*/
void SwTOXBaseSection::UpdateAuthorities( const SwTOXInternational& rIntl )
{
	SwDoc* pDoc = (SwDoc*)GetFmt()->GetDoc();
	SwFieldType* pAuthFld = pDoc->GetFldType(RES_AUTHORITY, aEmptyStr, false);
	if(!pAuthFld)
		return;

	SwClientIter aIter( *pAuthFld );
	SwFmtFld* pFmtFld = (SwFmtFld*)aIter.First( TYPE( SwFmtFld ));
	for( ; pFmtFld; pFmtFld = (SwFmtFld*)aIter.Next() )
	{
		const SwTxtFld* pTxtFld = pFmtFld->GetTxtFld();
		//undo
		if(!pTxtFld)
			continue;
		const SwTxtNode& rTxtNode = pTxtFld->GetTxtNode();
		::SetProgressState( 0, pDoc->GetDocShell() );

//		const SwTxtNode* pChapterCompareNode = 0;

		if( rTxtNode.GetTxt().Len() && rTxtNode.GetFrm() &&
			rTxtNode.GetNodes().IsDocNodes() /*&&
			(!IsFromChapter() || pChapterCompareNode == pOwnChapterNode) */)
		{
            //#106485# the body node has to be used!
            SwCntntFrm *pFrm = rTxtNode.GetFrm();
            SwPosition aFldPos(rTxtNode);
            const SwTxtNode* pTxtNode = 0;
            if(pFrm && !pFrm->IsInDocBody())
                pTxtNode = GetBodyTxtNode( *pDoc, aFldPos, *pFrm );
            if(!pTxtNode)
                pTxtNode = &rTxtNode;
            SwTOXAuthority* pNew = new SwTOXAuthority( *pTxtNode, *pFmtFld, rIntl );

			InsertSorted(pNew);
		}
	}
}

long lcl_IsSOObject( const SvGlobalName& rFactoryNm )
{
	static struct _SoObjType {
		long nFlag;
		// GlobalNameId
		struct _GlobalNameIds {
			UINT32 n1;
			USHORT n2, n3;
			BYTE b8, b9, b10, b11, b12, b13, b14, b15;
        } aGlNmIds[4];
	} aArr[] = {
        { nsSwTOOElements::TOO_MATH,
          { {SO3_SM_CLASSID_60},{SO3_SM_CLASSID_50},
            {SO3_SM_CLASSID_40},{SO3_SM_CLASSID_30} } },
        { nsSwTOOElements::TOO_CHART,
          { {SO3_SCH_CLASSID_60},{SO3_SCH_CLASSID_50},
            {SO3_SCH_CLASSID_40},{SO3_SCH_CLASSID_30} } },
        { nsSwTOOElements::TOO_CALC,
          { {SO3_SC_CLASSID_60},{SO3_SC_CLASSID_50},
            {SO3_SC_CLASSID_40},{SO3_SC_CLASSID_30} } },
        { nsSwTOOElements::TOO_DRAW_IMPRESS,
          { {SO3_SIMPRESS_CLASSID_60},{SO3_SIMPRESS_CLASSID_50},
            {SO3_SIMPRESS_CLASSID_40},{SO3_SIMPRESS_CLASSID_30} } },
        { nsSwTOOElements::TOO_DRAW_IMPRESS,
          { {SO3_SDRAW_CLASSID_60},{SO3_SDRAW_CLASSID_50}}},
        { 0,{{0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0},
            {0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0} } }
	};

	long nRet = 0;
	for( const _SoObjType* pArr = aArr; !nRet && pArr->nFlag; ++pArr )
        for ( int n = 0; n < 4; ++n )
		{
			const _SoObjType::_GlobalNameIds& rId = pArr->aGlNmIds[ n ];
			if( !rId.n1 )
				break;
			SvGlobalName aGlbNm( rId.n1, rId.n2, rId.n3,
						rId.b8, rId.b9, rId.b10, rId.b11,
						rId.b12, rId.b13, rId.b14, rId.b15 );
			if( rFactoryNm == aGlbNm )
			{
				nRet = pArr->nFlag;
				break;
			}
		}

	return nRet;
}

void SwTOXBaseSection::UpdateCntnt( SwTOXElement eMyType,
									const SwTxtNode* pOwnChapterNode )
{
	SwDoc* pDoc = (SwDoc*)GetFmt()->GetDoc();
	SwNodes& rNds = pDoc->GetNodes();
	// auf den 1. Node der 1. Section
	ULONG nIdx = rNds.GetEndOfAutotext().StartOfSectionIndex() + 2,
		 nEndIdx = rNds.GetEndOfAutotext().GetIndex();

	while( nIdx < nEndIdx )
	{
		::SetProgressState( 0, pDoc->GetDocShell() );

		SwNode* pNd = rNds[ nIdx ];
		SwCntntNode* pCNd = 0;
		switch( eMyType )
		{
        case nsSwTOXElement::TOX_FRAME:
			if( !pNd->IsNoTxtNode() )
			{
				pCNd = pNd->GetCntntNode();
				if( !pCNd )
				{
					SwNodeIndex aTmp( *pNd );
					pCNd = rNds.GoNext( &aTmp );
				}
			}
			break;
        case nsSwTOXElement::TOX_GRAPHIC:
			if( pNd->IsGrfNode() )
				pCNd = (SwCntntNode*)pNd;
			break;
        case nsSwTOXElement::TOX_OLE:
			if( pNd->IsOLENode() )
			{
				BOOL bInclude = TRUE;
				if(TOX_OBJECTS == SwTOXBase::GetType())
				{
					SwOLENode* pOLENode = pNd->GetOLENode();
					long nMyOLEOptions = GetOLEOptions();
					SwOLEObj& rOLEObj = pOLENode->GetOLEObj();

					if( rOLEObj.IsOleRef() )	//Noch nicht geladen
					{
                        SvGlobalName aTmpName = SvGlobalName( rOLEObj.GetOleRef()->getClassID() );
                        long nObj = ::lcl_IsSOObject( aTmpName );
                        bInclude = ( (nMyOLEOptions & nsSwTOOElements::TOO_OTHER) && 0 == nObj)
                                                    || (0 != (nMyOLEOptions & nObj));
					}
					else
					{
						DBG_ERROR("OLE-object nicht geladen?");
						bInclude = FALSE;
					}
				}

				if(bInclude)
					pCNd = (SwCntntNode*)pNd;
			}
			break;
        default: break;
		}

		if( pCNd )
		{
			//find node in body text
            int nSetLevel = USHRT_MAX;

            //#111105# tables of tables|illustrations|objects don't support hierarchies
            if( IsLevelFromChapter() &&
                    TOX_TABLES != SwTOXBase::GetType() &&
                    TOX_ILLUSTRATIONS != SwTOXBase::GetType() &&
                    TOX_OBJECTS != SwTOXBase::GetType() )
			{
				const SwTxtNode* pOutlNd = ::lcl_FindChapterNode( *pCNd,
														MAXLEVEL - 1 );
				if( pOutlNd )
				{
					//USHORT nTmp = pOutlNd->GetTxtColl()->GetOutlineLevel();//#outline level,zhaojianwei
					//if( nTmp < NO_NUMBERING )
					//	nSetLevel = nTmp + 1;
					if( pOutlNd->GetTxtColl()->IsAssignedToListLevelOfOutlineStyle())
						nSetLevel = pOutlNd->GetTxtColl()->GetAttrOutlineLevel() ;//<-end,zhaojianwei
				}
			}

			if( pCNd->GetFrm() && ( !IsFromChapter() ||
					::lcl_FindChapterNode( *pCNd, 0 ) == pOwnChapterNode ))
			{
				SwTOXPara * pNew = new SwTOXPara( *pCNd, eMyType,
                            ( USHRT_MAX != nSetLevel )
                            ? static_cast<USHORT>(nSetLevel)
                            : FORM_ALPHA_DELIMITTER );
				InsertSorted( pNew );
			}
		}

        nIdx = pNd->StartOfSectionNode()->EndOfSectionIndex() + 2;  // 2 == End-/StartNode
	}
}

/*--------------------------------------------------------------------
	 Beschreibung:	Tabelleneintraege zusammensuchen
 --------------------------------------------------------------------*/

void SwTOXBaseSection::UpdateTable( const SwTxtNode* pOwnChapterNode )
{
	SwDoc* pDoc = (SwDoc*)GetFmt()->GetDoc();
	SwNodes& rNds = pDoc->GetNodes();
	const SwFrmFmts& rArr = *pDoc->GetTblFrmFmts();

	for( USHORT n = 0; n < rArr.Count(); ++n )
	{
		::SetProgressState( 0, pDoc->GetDocShell() );

		SwTable* pTmpTbl = SwTable::FindTable( rArr[ n ] );
		SwTableBox* pFBox;
		if( pTmpTbl && 0 != (pFBox = pTmpTbl->GetTabSortBoxes()[0] ) &&
			pFBox->GetSttNd() && pFBox->GetSttNd()->GetNodes().IsDocNodes() )
		{
			const SwTableNode* pTblNd = pFBox->GetSttNd()->FindTableNode();
			SwNodeIndex aCntntIdx( *pTblNd, 1 );

			SwCntntNode* pCNd;
			while( 0 != ( pCNd = rNds.GoNext( &aCntntIdx ) ) &&
				aCntntIdx.GetIndex() < pTblNd->EndOfSectionIndex() )
			{
				if( pCNd->GetFrm() && (!IsFromChapter() ||
					::lcl_FindChapterNode( *pCNd, 0 ) == pOwnChapterNode ))
				{
					SwTOXTable * pNew = new SwTOXTable( *pCNd );
                    if( IsLevelFromChapter() && TOX_TABLES != SwTOXBase::GetType())
                    {
                        const SwTxtNode* pOutlNd =
                            ::lcl_FindChapterNode( *pCNd, MAXLEVEL - 1 );
                        if( pOutlNd )
                        {
							//USHORT nTmp = pOutlNd->GetTxtColl()->GetOutlineLevel();//#outline level,zhaojianwei
							//if( nTmp < NO_NUMBERING )
							//	pNew->SetLevel( nTmp + 1 );
                            if( pOutlNd->GetTxtColl()->IsAssignedToListLevelOfOutlineStyle())
							{
                                const int nTmp = pOutlNd->GetTxtColl()->GetAttrOutlineLevel();
                                pNew->SetLevel( static_cast<USHORT>(nTmp) );//<-end ,zhaojianwei
							}
                        }
                    }
                    InsertSorted(pNew);
					break;
				}
			}
		}
	}
}

/*--------------------------------------------------------------------
	 Beschreibung:	String generieren anhand der Form
					SonderZeichen 0-31 und 255 entfernen
 --------------------------------------------------------------------*/

String lcl_GetNumString( const SwTOXSortTabBase& rBase, sal_Bool bUsePrefix, BYTE nLevel )
{
	String sRet;

	if( !rBase.pTxtMark && rBase.aTOXSources.Count() > 0 )
	{	// nur wenn es keine Marke ist
		const SwTxtNode* pNd = rBase.aTOXSources[0].pNd->GetTxtNode();
		if( pNd )
		{
			const SwNumRule* pRule = pNd->GetNumRule();

            if( pRule && pNd->GetActualListLevel() < MAXLEVEL )
                sRet = pNd->GetNumString(bUsePrefix, nLevel);
        }
	}
	return sRet;
}

// OD 18.03.2003 #106329# - add parameter <_TOXSectNdIdx> and <_pDefaultPageDesc>
// in order to control, which page description is used, no appropriate one is found.
void SwTOXBaseSection::GenerateText( USHORT nArrayIdx,
                                     USHORT nCount,
                                     SvStringsDtor& ,
                                     const sal_uInt32   _nTOXSectNdIdx,
                                     const SwPageDesc*  _pDefaultPageDesc )
{
	LinkStructArr	aLinkArr;
	SwDoc* pDoc = (SwDoc*)GetFmt()->GetDoc();
	::SetProgressState( 0, pDoc->GetDocShell() );

	//pTOXNd is only set at the first mark
	SwTxtNode* pTOXNd = (SwTxtNode*)aSortArr[nArrayIdx]->pTOXNd;
	String& rTxt = (String&)pTOXNd->GetTxt();
	rTxt.Erase();
	for(USHORT nIndex = nArrayIdx; nIndex < nArrayIdx + nCount; nIndex++)
	{
		if(nIndex > nArrayIdx)
			rTxt.AppendAscii( RTL_CONSTASCII_STRINGPARAM( ", " )); // comma separation
		// String mit dem Pattern aus der Form initialisieren
		const SwTOXSortTabBase& rBase = *aSortArr[nIndex];
		USHORT nLvl = rBase.GetLevel();
		ASSERT( nLvl < GetTOXForm().GetFormMax(), "ungueltiges FORM_LEVEL");

        SvxTabStopItem aTStops( 0, 0, SVX_TAB_ADJUST_DEFAULT, RES_PARATR_TABSTOP );
		xub_StrLen nLinkStartPosition = STRING_NOTFOUND;
        String  sLinkCharacterStyle; //default to "Default" character style - which is none
		String sURL;
		// create an enumerator
        // #i21237#
        SwFormTokens aPattern = GetTOXForm().GetPattern(nLvl);
        SwFormTokens::iterator aIt = aPattern.begin();
		// remove text from node
		while(aIt != aPattern.end()) // #i21237#
		{
			SwFormToken aToken = *aIt; // #i21237#
			xub_StrLen nStartCharStyle = rTxt.Len();
			switch( aToken.eTokenType )
			{
			case TOKEN_ENTRY_NO:
				// fuer Inhaltsverzeichnis Numerierung
                rTxt.Insert( lcl_GetNumString( rBase, aToken.nChapterFormat == CF_NUMBER, static_cast<BYTE>(aToken.nOutlineLevel - 1)) );
				break;

			case TOKEN_ENTRY_TEXT:
				{
					SwIndex aIdx( pTOXNd, rTxt.Len() );
					rBase.FillText( *pTOXNd, aIdx );
				}
				break;

			case TOKEN_ENTRY:
				{
					// fuer Inhaltsverzeichnis Numerierung
					rTxt.Insert( lcl_GetNumString( rBase, sal_True, MAXLEVEL ));

					SwIndex aIdx( pTOXNd, rTxt.Len() );
					rBase.FillText( *pTOXNd, aIdx );
				}
				break;

			case TOKEN_TAB_STOP:
                if (aToken.bWithTab) // #i21237#
                    rTxt.Append('\t');
				//

				if(SVX_TAB_ADJUST_END > aToken.eTabAlign)
				{
					const SvxLRSpaceItem& rLR =
                        (SvxLRSpaceItem&)pTOXNd->
                        SwCntntNode::GetAttr( RES_LR_SPACE, TRUE );

					long nTabPosition = aToken.nTabStopPosition;
					if( !GetTOXForm().IsRelTabPos() && rLR.GetTxtLeft() )
						nTabPosition -= rLR.GetTxtLeft();
					aTStops.Insert( SvxTabStop( nTabPosition,
                                                aToken.eTabAlign,
												cDfltDecimalChar,
												aToken.cTabFillChar ));
				}
				else
				{
					const SwPageDesc* pPageDesc = ((SwFmtPageDesc&)pTOXNd->
								SwCntntNode::GetAttr( RES_PAGEDESC )).GetPageDesc();

					BOOL bCallFindRect = TRUE;
					long nRightMargin;
					if( pPageDesc )
					{
						const SwFrm* pFrm = pTOXNd->GetFrm( 0, 0, TRUE );
						if( !pFrm || 0 == ( pFrm = pFrm->FindPageFrm() ) ||
							pPageDesc != ((SwPageFrm*)pFrm)->GetPageDesc() )
							// dann muss man ueber den PageDesc gehen
							bCallFindRect = FALSE;
					}

					SwRect aNdRect;
					if( bCallFindRect )
						aNdRect = pTOXNd->FindLayoutRect( TRUE );

					if( aNdRect.IsEmpty() )
					{
						// dann hilft alles nichts, wir muessen ueber die Seiten-
						// vorlage gehen.
                        // OD 18.03.2003 #106329# - call
                        sal_uInt32 nPgDescNdIdx = pTOXNd->GetIndex() + 1;
                        sal_uInt32* pPgDescNdIdx = &nPgDescNdIdx;
                        pPageDesc = pTOXNd->FindPageDesc( FALSE, pPgDescNdIdx );
                        if ( !pPageDesc ||
                             *pPgDescNdIdx < _nTOXSectNdIdx )
                        {
                            // use default page description, if none is found
                            // or the found one is given by a node before the
                            // table-of-content section.
                            pPageDesc = _pDefaultPageDesc;
                        }

						const SwFrmFmt& rPgDscFmt = pPageDesc->GetMaster();
						nRightMargin = rPgDscFmt.GetFrmSize().GetWidth() -
								 		rPgDscFmt.GetLRSpace().GetLeft() -
								 		rPgDscFmt.GetLRSpace().GetRight();
					}
					else
						nRightMargin = aNdRect.Width();
                    //#i24363# tab stops relative to indent
                    if( pDoc->get(IDocumentSettingAccess::TABS_RELATIVE_TO_INDENT) )
                    {
                        //left margin of paragraph style
                        const SvxLRSpaceItem& rLRSpace = pTOXNd->GetTxtColl()->GetLRSpace();
                        nRightMargin -= rLRSpace.GetLeft();
                        nRightMargin -= rLRSpace.GetTxtFirstLineOfst();
                    }

                    aTStops.Insert( SvxTabStop( nRightMargin, SVX_TAB_ADJUST_RIGHT,
												cDfltDecimalChar,
												aToken.cTabFillChar ));
				}
				break;

			case TOKEN_TEXT:
				rTxt.Append( aToken.sText );
				break;

			case TOKEN_PAGE_NUMS:
					// Platzhalter fuer Seitennummer(n) es wird nur der erste beachtet
					//
				{
					// Die Anzahl der gleichen Eintrage bestimmt die Seitennummern-Pattern
					//
					USHORT nSize = rBase.aTOXSources.Count();
					if( nSize > 0 )
					{
						String aInsStr( cNumRepl );
						for(USHORT i=1; i < nSize; ++i)
						{
							aInsStr.AppendAscii( sPageDeli );
							aInsStr += cNumRepl;
						}
						aInsStr += cEndPageNum;
						rTxt.Append( aInsStr );
					}
//						// Tab entfernen, wenn keine Seitennummer
//					else if( rTxt.Len() && '\t' == rTxt.GetChar( rTxt.Len() - 1 ))
//						rTxt.Erase( rTxt.Len()-1, 1 );
				}
				break;

			case TOKEN_CHAPTER_INFO:
				{
					// ein bischen trickreich: suche irgend einen Frame
					const SwTOXSource* pTOXSource = 0;
					if(rBase.aTOXSources.Count())
						pTOXSource = &rBase.aTOXSources[0];

                    // --> OD 2008-02-14 #i53420#
//                    if( pTOXSource && pTOXSource->pNd
//                        pTOXSource->pNd->IsTxtNode() )
                    if ( pTOXSource && pTOXSource->pNd &&
                         pTOXSource->pNd->IsCntntNode() )
                    // <--
					{
						const SwCntntFrm* pFrm = pTOXSource->pNd->GetFrm();
						if( pFrm )
						{
							SwChapterFieldType aFldTyp;
							SwChapterField aFld( &aFldTyp, aToken.nChapterFormat );
                            aFld.SetLevel( static_cast<BYTE>(aToken.nOutlineLevel - 1) );
                            // --> OD 2008-02-14 #i53420#
//                            aFld.ChangeExpansion( pFrm, (SwTxtNode*)pTOXSource->pNd, TRUE );
                            aFld.ChangeExpansion( pFrm,
                                dynamic_cast<const SwCntntNode*>(pTOXSource->pNd),
                                TRUE );
                            // <--
                            //---> i89791
                            // OD 2008-06-26 - continue to support CF_NUMBER
                            // and CF_NUM_TITLE in order to handle ODF 1.0/1.1
                            // written by OOo 3.x in the same way as OOo 2.x
                            // would handle them.
                            if ( CF_NUM_NOPREPST_TITLE == aToken.nChapterFormat ||
                                 CF_NUMBER == aToken.nChapterFormat )
                                rTxt.Insert(aFld.GetNumber()); //get the string number without pre/postfix
                            else if ( CF_NUMBER_NOPREPST == aToken.nChapterFormat ||
                                      CF_NUM_TITLE == aToken.nChapterFormat )
                            //<---
							{
								rTxt += aFld.GetNumber();
								rTxt += ' ';
								rTxt += aFld.GetTitle();
							}
							else if(CF_TITLE == aToken.nChapterFormat)
								rTxt += aFld.GetTitle();
						}
					}
				}
				break;

			case TOKEN_LINK_START:
				nLinkStartPosition = rTxt.Len();
                sLinkCharacterStyle = aToken.sCharStyleName;
            break;

			case TOKEN_LINK_END:
					//TODO: only paired start/end tokens are valid
				if( STRING_NOTFOUND != nLinkStartPosition)
				{
					SwIndex aIdx( pTOXNd, nLinkStartPosition );
					//pTOXNd->Erase( aIdx, SwForm::nFormLinkSttLen );
					xub_StrLen nEnd = rTxt.Len();

					if( !sURL.Len() )
					{
						sURL = rBase.GetURL();
						if( !sURL.Len() )
							break;
					}
                    LinkStruct* pNewLink = new LinkStruct(sURL, nLinkStartPosition,
                                                    nEnd);
                    pNewLink->aINetFmt.SetVisitedFmt(sLinkCharacterStyle);
                    pNewLink->aINetFmt.SetINetFmt(sLinkCharacterStyle);
                    if(sLinkCharacterStyle.Len())
                    {
                        USHORT nPoolId =
                            SwStyleNameMapper::GetPoolIdFromUIName( sLinkCharacterStyle, nsSwGetPoolIdFromName::GET_POOLID_CHRFMT );
                        pNewLink->aINetFmt.SetVisitedFmtId(nPoolId);
                        pNewLink->aINetFmt.SetINetFmtId(nPoolId);
                    }
                    else
                    {
                        pNewLink->aINetFmt.SetVisitedFmtId(USHRT_MAX);
                        pNewLink->aINetFmt.SetINetFmtId(USHRT_MAX);
                    }
                    aLinkArr.Insert( pNewLink, aLinkArr.Count() );
					nLinkStartPosition = STRING_NOTFOUND;
                    sLinkCharacterStyle.Erase();
				}
				break;

			case TOKEN_AUTHORITY:
				{
					ToxAuthorityField eField = (ToxAuthorityField)aToken.nAuthorityField;
					SwIndex aIdx( pTOXNd, rTxt.Len() );
					rBase.FillText( *pTOXNd, aIdx, static_cast<USHORT>(eField) );
				}
				break;
            case TOKEN_END: break;
			}

			if( aToken.sCharStyleName.Len() )
			{
				SwCharFmt* pCharFmt;
				if(	USHRT_MAX != aToken.nPoolId )
					pCharFmt = pDoc->GetCharFmtFromPool( aToken.nPoolId );
				else
					pCharFmt = pDoc->FindCharFmtByName( aToken.sCharStyleName);

                if (pCharFmt)
                {
                    SwFmtCharFmt aFmt( pCharFmt );
                    pTOXNd->InsertItem( aFmt, nStartCharStyle,
                        rTxt.Len(), nsSetAttrMode::SETATTR_DONTEXPAND );
                }
            }

            aIt++; // #i21237#
		}

        pTOXNd->SetAttr( aTStops );
	}

	if(aLinkArr.Count())
		for(USHORT i = 0; i < aLinkArr.Count(); ++i )
		{
			LinkStruct* pTmp = aLinkArr.GetObject(i);
            pTOXNd->InsertItem( pTmp->aINetFmt, pTmp->nStartTextPos,
							pTmp->nEndTextPos);
		}
}

/*--------------------------------------------------------------------
	 Beschreibung: Seitennummer errechnen und nach dem Formatieren
				   eintragen
 --------------------------------------------------------------------*/

void SwTOXBaseSection::UpdatePageNum()
{
	if( !aSortArr.Count() )
		return ;

	// die aktuellen Seitennummern ins Verzeichnis eintragen
	SwPageFrm*	pAktPage	= 0;
	USHORT		nPage		= 0;
	SwDoc* pDoc = (SwDoc*)GetFmt()->GetDoc();

    SwTOXInternational aIntl( GetLanguage(),
                              TOX_INDEX == GetTOXType()->GetType() ?
                              GetOptions() : 0,
                              GetSortAlgorithm() );

	for( USHORT nCnt = 0; nCnt < aSortArr.Count(); ++nCnt )
	{
		// Schleife ueber alle SourceNodes
		SvUShorts aNums;		//Die Seitennummern
		SvPtrarr  aDescs;		//Die PageDescriptoren passend zu den Seitennummern.
		SvUShorts* pMainNums = 0; // contains page numbers of main entries

		// process run in lines
		USHORT nRange = 0;
		if(GetTOXForm().IsCommaSeparated() &&
				aSortArr[nCnt]->GetType() == TOX_SORT_INDEX)
		{
			const SwTOXMark& rMark = aSortArr[nCnt]->pTxtMark->GetTOXMark();
			const String sPrimKey = rMark.GetPrimaryKey();
			const String sSecKey = rMark.GetSecondaryKey();
			const SwTOXMark* pNextMark = 0;
			while(aSortArr.Count() > (nCnt + nRange)&&
					aSortArr[nCnt + nRange]->GetType() == TOX_SORT_INDEX &&
					0 != (pNextMark = &(aSortArr[nCnt + nRange]->pTxtMark->GetTOXMark())) &&
					pNextMark->GetPrimaryKey() == sPrimKey &&
					pNextMark->GetSecondaryKey() == sSecKey)
				nRange++;
		}
		else
			nRange = 1;

		for(USHORT nRunInEntry = nCnt; nRunInEntry < nCnt + nRange; nRunInEntry++)
		{
			SwTOXSortTabBase* pSortBase = aSortArr[nRunInEntry];
			USHORT nSize = pSortBase->aTOXSources.Count();
			USHORT i;
			for( USHORT j = 0; j < nSize; ++j )
			{
				::SetProgressState( 0, pDoc->GetDocShell() );

				SwTOXSource& rTOXSource = pSortBase->aTOXSources[j];
				if( rTOXSource.pNd )
				{
					SwCntntFrm* pFrm = rTOXSource.pNd->GetFrm();
                    ASSERT( pFrm || pDoc->IsUpdateTOX(), "TOX, no Frame found");
                    if( !pFrm )
                        continue;
					if( pFrm->IsTxtFrm() && ((SwTxtFrm*)pFrm)->HasFollow() )
					{
						// dann suche den richtigen heraus
						SwTxtFrm* pNext = (SwTxtFrm*)pFrm;
						while( 0 != ( pNext = (SwTxtFrm*)pFrm->GetFollow() )
								&& rTOXSource.nPos >= pNext->GetOfst() )
							pFrm = pNext;
					}

					SwPageFrm*	pTmpPage = pFrm->FindPageFrm();
					if( pTmpPage != pAktPage )
					{
						nPage		= pTmpPage->GetVirtPageNum();
						pAktPage	= pTmpPage;
					}

					// sortiert einfuegen
					for( i = 0; i < aNums.Count() && aNums[i] < nPage; ++i )
						;

					if( i >= aNums.Count() || aNums[ i ] != nPage )
					{
						aNums.Insert( nPage, i );
						aDescs.Insert( (void*)pAktPage->GetPageDesc(), i );
					}
					// is it a main entry?
					if(TOX_SORT_INDEX == pSortBase->GetType() &&
						rTOXSource.bMainEntry)
					{
						if(!pMainNums)
							pMainNums = new SvUShorts;
						pMainNums->Insert(nPage, pMainNums->Count());
					}
				}
			}
			// einfuegen der Seitennummer in den Verzeichnis-Text-Node
			const SwTOXSortTabBase* pBase = aSortArr[ nCnt ];
			if(pBase->pTOXNd)
			{
				const SwTxtNode* pTxtNd = pBase->pTOXNd->GetTxtNode();
				ASSERT( pTxtNd, "kein TextNode, falsches Verzeichnis" );

				_UpdatePageNum( (SwTxtNode*)pTxtNd, aNums, aDescs, pMainNums,
								aIntl );
			}
			DELETEZ(pMainNums);
			aNums.Remove(0, aNums.Count());
		}
	}
	// nach dem Setzen der richtigen Seitennummer, das Mapping-Array
	// wieder loeschen !!
	aSortArr.DeleteAndDestroy( 0, aSortArr.Count() );
}


/*--------------------------------------------------------------------
	 Beschreibung: Austausch der Seitennummer-Platzhalter
 --------------------------------------------------------------------*/

// search for the page no in the array of main entry page numbers
BOOL lcl_HasMainEntry( const SvUShorts* pMainEntryNums, USHORT nToFind )
{
	for(USHORT i = 0; pMainEntryNums && i < pMainEntryNums->Count(); ++i)
		if(nToFind == (*pMainEntryNums)[i])
			return TRUE;
	return FALSE;
}

void SwTOXBaseSection::_UpdatePageNum( SwTxtNode* pNd,
									const SvUShorts& rNums,
									const SvPtrarr & rDescs,
									const SvUShorts* pMainEntryNums,
									const SwTOXInternational& rIntl )
{
	//collect starts end ends of main entry character style
	SvUShorts* pCharStyleIdx = pMainEntryNums ? new SvUShorts : 0;

	String sSrchStr( cNumRepl );
	sSrchStr.AppendAscii( sPageDeli ) += cNumRepl;
	xub_StrLen nStartPos = pNd->GetTxt().Search( sSrchStr );
	( sSrchStr = cNumRepl ) += cEndPageNum;
	xub_StrLen nEndPos = pNd->GetTxt().Search( sSrchStr );
	USHORT i;

	if( STRING_NOTFOUND == nEndPos || !rNums.Count() )
		return;

	if( STRING_NOTFOUND == nStartPos || nStartPos > nEndPos)
		nStartPos = nEndPos;

	USHORT nOld = rNums[0],
		   nBeg = nOld,
		   nCount  = 0;
	String aNumStr( SvxNumberType( ((SwPageDesc*)rDescs[0])->GetNumType() ).
					GetNumStr( nBeg ) );
	if( pCharStyleIdx && lcl_HasMainEntry( pMainEntryNums, nBeg ))
	{
		USHORT nTemp = 0;
		pCharStyleIdx->Insert( nTemp, pCharStyleIdx->Count());
	}

	// Platzhalter loeschen
	SwIndex aPos(pNd, nStartPos);
	SwCharFmt* pPageNoCharFmt = 0;
	SwpHints* pHints = pNd->GetpSwpHints();
	if(pHints)
		for(USHORT nHintIdx = 0; nHintIdx < pHints->GetStartCount(); nHintIdx++)
		{
			SwTxtAttr* pAttr = pHints->GetStart(nHintIdx);
			xub_StrLen nTmpEnd = pAttr->GetEnd() ? *pAttr->GetEnd() : 0;
			if(	nStartPos >= *pAttr->GetStart() &&
				(nStartPos + 2) <= nTmpEnd &&
				pAttr->Which() == RES_TXTATR_CHARFMT)
			{
				pPageNoCharFmt = pAttr->GetCharFmt().GetCharFmt();
				break;
			}
		}
    pNd->EraseText(aPos, nEndPos - nStartPos + 2);

	for( i = 1; i < rNums.Count(); ++i)
	{
		SvxNumberType aType( ((SwPageDesc*)rDescs[i])->GetNumType() );
		if( TOX_INDEX == SwTOXBase::GetType() )
		{	// Zusammenfassen f. ff.
			// Alle folgenden aufaddieren
			// break up if main entry starts or ends and
			// insert a char style index
			BOOL bMainEntryChanges = lcl_HasMainEntry(pMainEntryNums, nOld)
					!= lcl_HasMainEntry(pMainEntryNums, rNums[i]);

			if(nOld == rNums[i]-1 && !bMainEntryChanges &&
                0 != (GetOptions() & (nsSwTOIOptions::TOI_FF|nsSwTOIOptions::TOI_DASH)))
				nCount++;
			else
			{
				// ff. f. alten Wert flushen
                if(GetOptions() & nsSwTOIOptions::TOI_FF)
				{
					if ( nCount >= 1 )
						aNumStr += rIntl.GetFollowingText( nCount > 1 );
				}
				else
				{
					if(nCount >= 2 )
						aNumStr += '-';
					else if(nCount == 1 )
						aNumStr.AppendAscii( sPageDeli );
//#58127# Wenn nCount == 0, dann steht die einzige Seitenzahl schon im aNumStr!
					if(nCount)
						aNumStr += aType.GetNumStr( nBeg + nCount );
				}

				// neuen String anlegen
				nBeg	 = rNums[i];
				aNumStr.AppendAscii( sPageDeli );
				//the change of the character style must apply after sPageDeli is appended
				if(pCharStyleIdx && bMainEntryChanges)
					pCharStyleIdx->Insert(aNumStr.Len(),
													pCharStyleIdx->Count());
				aNumStr += aType.GetNumStr( nBeg );
				nCount	 = 0;
			}
			nOld = rNums[i];
		}
		else
		{	// Alle Nummern eintragen
			aNumStr += aType.GetNumStr( USHORT(rNums[i]) );
			if(i != (rNums.Count()-1))
				aNumStr.AppendAscii( sPageDeli );
		}
	}
	// Bei Ende und ff. alten Wert flushen
	if( TOX_INDEX == SwTOXBase::GetType() )
	{
        if(GetOptions() & nsSwTOIOptions::TOI_FF)
		{
			if( nCount >= 1 )
				aNumStr += rIntl.GetFollowingText( nCount > 1 );
		}
		else
		{
			if(nCount >= 2)
				aNumStr +='-';
			else if(nCount == 1)
				aNumStr.AppendAscii( sPageDeli );
//#58127# Wenn nCount == 0, dann steht die einzige Seitenzahl schon im aNumStr!
			if(nCount)
				aNumStr += SvxNumberType( ((SwPageDesc*)rDescs[i-1])->
								GetNumType() ).GetNumStr( nBeg+nCount );
		}
	}
    pNd->InsertText( aNumStr, aPos,
           static_cast<IDocumentContentOperations::InsertFlags>(
               IDocumentContentOperations::INS_EMPTYEXPAND |
               IDocumentContentOperations::INS_FORCEHINTEXPAND) );
	if(pPageNoCharFmt)
	{
        SwFmtCharFmt aCharFmt( pPageNoCharFmt );
        pNd->InsertItem(aCharFmt, nStartPos, nStartPos + aNumStr.Len(), nsSetAttrMode::SETATTR_DONTEXPAND);
    }

	//now the main entries should get there character style
	if(pCharStyleIdx && pCharStyleIdx->Count() && GetMainEntryCharStyle().Len())
	{
		// eventually the last index must me appended
		if(pCharStyleIdx->Count()&0x01)
			pCharStyleIdx->Insert(aNumStr.Len(), pCharStyleIdx->Count());

		//search by name
		SwDoc* pDoc = pNd->GetDoc();
		USHORT nPoolId = SwStyleNameMapper::GetPoolIdFromUIName( GetMainEntryCharStyle(), nsSwGetPoolIdFromName::GET_POOLID_CHRFMT );
		SwCharFmt* pCharFmt = 0;
		if(USHRT_MAX != nPoolId)
			pCharFmt = pDoc->GetCharFmtFromPool(nPoolId);
		else
			pCharFmt = pDoc->FindCharFmtByName( GetMainEntryCharStyle() );
		if(!pCharFmt)
			pCharFmt = pDoc->MakeCharFmt(GetMainEntryCharStyle(), 0);

		//find the page numbers in aNumStr and set the character style
		xub_StrLen nOffset = pNd->GetTxt().Len() - aNumStr.Len();
		SwFmtCharFmt aCharFmt(pCharFmt);
		for(USHORT j = 0; j < pCharStyleIdx->Count(); j += 2)
		{
			xub_StrLen nStartIdx = (*pCharStyleIdx)[j] + nOffset;
			xub_StrLen nEndIdx = (*pCharStyleIdx)[j + 1]  + nOffset;
            pNd->InsertItem(aCharFmt, nStartIdx, nEndIdx, nsSetAttrMode::SETATTR_DONTEXPAND);
		}

	}
	delete pCharStyleIdx;
}


/*--------------------------------------------------------------------
	 Beschreibung: Sortiert einfuegen in das SortArr
 --------------------------------------------------------------------*/

void SwTOXBaseSection::InsertSorted(SwTOXSortTabBase* pNew)
{
	Range aRange(0, aSortArr.Count());
	if( TOX_INDEX == SwTOXBase::GetType() && pNew->pTxtMark )
	{
		const SwTOXMark& rMark = pNew->pTxtMark->GetTOXMark();
		// Schluessel auswerten
		// Den Bereich ermitteln, in dem einzufuegen ist
        if( 0 == (GetOptions() & nsSwTOIOptions::TOI_KEY_AS_ENTRY) &&
			rMark.GetPrimaryKey().Len() )
		{
            aRange = GetKeyRange( rMark.GetPrimaryKey(),
                                  rMark.GetPrimaryKeyReading(),
                                  *pNew, FORM_PRIMARY_KEY, aRange );

            if( rMark.GetSecondaryKey().Len() )
                aRange = GetKeyRange( rMark.GetSecondaryKey(),
                                      rMark.GetSecondaryKeyReading(),
                                      *pNew, FORM_SECONDARY_KEY, aRange );
		}
	}
	//search for identical entries and remove the trailing one
	if(TOX_AUTHORITIES == SwTOXBase::GetType())
	{
		for(short i = (short)aRange.Min(); i < (short)aRange.Max(); ++i)
		{
			SwTOXSortTabBase* pOld = aSortArr[i];
			if(*pOld == *pNew)
			{
				if(*pOld < *pNew)
				{
					delete pNew;
					return;
				}
				else
				{
					// remove the old content
					aSortArr.DeleteAndDestroy( i, 1 );
					aRange.Max()--;
					break;
				}
			}
		}
	}

	// find position and insert
	//
	short i;

	for( i = (short)aRange.Min(); i < (short)aRange.Max(); ++i)
	{	// nur auf gleicher Ebene pruefen
		//
		SwTOXSortTabBase* pOld = aSortArr[i];
		if(*pOld == *pNew)
		{
			if(TOX_AUTHORITIES != SwTOXBase::GetType())
			{
				// Eigener Eintrag fuer Doppelte oder Keywords
				//
				if( pOld->GetType() == TOX_SORT_CUSTOM &&
                    pNew->GetOptions() & nsSwTOIOptions::TOI_KEY_AS_ENTRY)
					continue;

                if(!(pNew->GetOptions() & nsSwTOIOptions::TOI_SAME_ENTRY))
				{	// Eigener Eintrag
					aSortArr.Insert(pNew, i );
					return;
				}
				// Eintrag schon vorhanden in Referenzliste aufnehmen
				pOld->aTOXSources.Insert( pNew->aTOXSources[0],
											pOld->aTOXSources.Count() );

				delete pNew;
				return;
			}
#ifdef DBG_UTIL
			else
				DBG_ERROR("Bibliography entries cannot be found here");
#endif
		}
		if(*pNew < *pOld)
			break;
	}
	// SubLevel Skippen
	while( TOX_INDEX == SwTOXBase::GetType() && i < aRange.Max() &&
		  aSortArr[i]->GetLevel() > pNew->GetLevel() )
		i++;

	// An Position i wird eingefuegt
	aSortArr.Insert(pNew, i );
}

/*--------------------------------------------------------------------
	 Beschreibung: Schluessel-Bereich suchen und evtl einfuegen
 --------------------------------------------------------------------*/

Range SwTOXBaseSection::GetKeyRange(const String& rStr, const String& rStrReading,
                                    const SwTOXSortTabBase& rNew,
                                    USHORT nLevel, const Range& rRange )
{
    const SwTOXInternational& rIntl = *rNew.pTOXIntl;
	String sToCompare(rStr);
    String sToCompareReading(rStrReading);

    if( 0 != (nsSwTOIOptions::TOI_INITIAL_CAPS & GetOptions()) )
	{
		String sUpper( rIntl.ToUpper( sToCompare, 0 ));
		sToCompare.Erase( 0, 1 ).Insert( sUpper, 0 );
	}

	ASSERT(rRange.Min() >= 0 && rRange.Max() >= 0, "Min Max < 0");

	const USHORT nMin = (USHORT)rRange.Min();
	const USHORT nMax = (USHORT)rRange.Max();

    USHORT i;

	for( i = nMin; i < nMax; ++i)
	{
		SwTOXSortTabBase* pBase = aSortArr[i];

        String sMyString, sMyStringReading;
        pBase->GetTxt( sMyString, sMyStringReading );

        if( rIntl.IsEqual( sMyString, sMyStringReading, pBase->GetLocale(),
                           sToCompare, sToCompareReading, rNew.GetLocale() )  &&
                    pBase->GetLevel() == nLevel &&
                    pBase->GetType() == TOX_SORT_CUSTOM )
			break;
	}
	if(i == nMax)
	{	// Falls nicht vorhanden erzeugen und einfuegen
		//
        SwTOXCustom* pKey = new SwTOXCustom( sToCompare, sToCompareReading, nLevel, rIntl,
                                             rNew.GetLocale() );
		for(i = nMin; i < nMax; ++i)
		{
			if(nLevel == aSortArr[i]->GetLevel() &&  *pKey < *(aSortArr[i]))
				break;
		}
		aSortArr.Insert(pKey, i );
	}
	USHORT nStart = i+1;
	USHORT nEnd   = aSortArr.Count();

	// Ende des Bereiches suchen
	for(i = nStart; i < aSortArr.Count(); ++i)
	{
		if(aSortArr[i]->GetLevel() <= nLevel)
		{	nEnd = i;
			break;
		}
	}
	return Range(nStart, nEnd);
}


BOOL SwTOXBase::IsTOXBaseInReadonly() const
{
	const SwTOXBaseSection *pSect = PTR_CAST(SwTOXBaseSection, this);
	BOOL bRet = FALSE;
	const SwSectionNode* pSectNode;
	if(pSect && pSect->GetFmt() &&
			0 != (pSectNode = pSect->GetFmt()->GetSectionNode()))
	{
		const SwDocShell* pDocSh;
		bRet = (0 != (pDocSh = pSectNode->GetDoc()->GetDocShell()) &&
													pDocSh->IsReadOnly()) ||
            (0 != (pSectNode = pSectNode->StartOfSectionNode()->FindSectionNode())&&
					pSectNode->GetSection().IsProtectFlag());

	}
	return bRet;
}
/* -----------------17.08.99 13:29-------------------

 --------------------------------------------------*/
const SfxItemSet* SwTOXBase::GetAttrSet() const
{
	const SwTOXBaseSection *pSect = PTR_CAST(SwTOXBaseSection, this);
	if(pSect && pSect->GetFmt())
		return &pSect->GetFmt()->GetAttrSet();
	return 0;
}

void SwTOXBase::SetAttrSet( const SfxItemSet& rSet )
{
	SwTOXBaseSection *pSect = PTR_CAST(SwTOXBaseSection, this);
	if( pSect && pSect->GetFmt() )
        pSect->GetFmt()->SetFmtAttr( rSet );
}

BOOL SwTOXBase::GetInfo( SfxPoolItem& rInfo ) const
{
	switch( rInfo.Which() )
	{
	case RES_CONTENT_VISIBLE:
		{
			SwTOXBaseSection *pSect = PTR_CAST(SwTOXBaseSection, this);
			if( pSect && pSect->GetFmt() )
				pSect->GetFmt()->GetInfo( rInfo );
		}
		return FALSE;
	}
	return TRUE;
}

/*  */


