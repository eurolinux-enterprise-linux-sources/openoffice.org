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

#include <svx/svditer.hxx>
#include <svx/svdograf.hxx>
#include <svx/svdoole2.hxx>
#include <svx/svdpage.hxx>
#include <svx/xoutbmp.hxx>
#include <svx/svdxcgv.hxx>
#include <sot/exchange.hxx>
#include <svtools/htmlkywd.hxx>
#include <svtools/htmlout.hxx>
#include <svtools/transfer.hxx>
#include <svtools/embedtransfer.hxx>
#include <svtools/urihelper.hxx>
#include <tools/urlobj.hxx>

#include "htmlexp.hxx"
#include "global.hxx"
#include "document.hxx"
#include "drwlayer.hxx"
#include "ftools.hxx"

using namespace com::sun::star;

//------------------------------------------------------------------------

void ScHTMLExport::PrepareGraphics( ScDrawLayer* pDrawLayer, SCTAB nTab,
		SCCOL nStartCol, SCROW nStartRow,	SCCOL nEndCol, SCROW nEndRow )
{
	if ( pDrawLayer->HasObjectsInRows( nTab, nStartRow, nEndRow ) )
	{
		SdrPage* pDrawPage = pDrawLayer->GetPage( static_cast<sal_uInt16>(nTab) );
		if ( pDrawPage )
		{
			bTabHasGraphics = TRUE;
			FillGraphList( pDrawPage, nTab,
				nStartCol, nStartRow, nEndCol, nEndRow );
			for ( ScHTMLGraphEntry* pE = aGraphList.First(); pE;
					pE = aGraphList.Next() )
			{
				if ( !pE->bInCell )
				{	// nicht alle in Zellen: einige neben Tabelle
					bTabAlignedLeft = TRUE;
					break;
				}
			}
		}
	}
}


void ScHTMLExport::FillGraphList( const SdrPage* pPage, SCTAB nTab,
		SCCOL nStartCol, SCROW nStartRow,	SCCOL nEndCol, SCROW nEndRow )
{
	ULONG	nObjCount = pPage->GetObjCount();
	if ( nObjCount )
	{
		Rectangle aRect;
		if ( !bAll )
			aRect = pDoc->GetMMRect( nStartCol, nStartRow, nEndCol, nEndRow, nTab );
		SdrObjListIter aIter( *pPage, IM_FLAT );
		SdrObject* pObject = aIter.Next();
		while ( pObject )
		{
			Rectangle aObjRect = pObject->GetCurrentBoundRect();
			if ( bAll || aRect.IsInside( aObjRect ) )
			{
                Size aSpace;
                ScRange aR = pDoc->GetRange( nTab, aObjRect );
                // Rectangle in mm/100
                Size aSize( MMToPixel( aObjRect.GetSize() ) );
                // If the image is somewhere in a merged range we must
                // move the anchor to the upper left (THE span cell).
                pDoc->ExtendOverlapped( aR );
                SCCOL nCol1 = aR.aStart.Col();
                SCROW nRow1 = aR.aStart.Row();
                SCCOL nCol2 = aR.aEnd.Col();
                SCROW nRow2 = aR.aEnd.Row();
                // All cells empty under object?
                BOOL bInCell = (pDoc->GetEmptyLinesInBlock(
                    nCol1, nRow1, nTab, nCol2, nRow2, nTab, DIR_TOP )
                    == static_cast< SCSIZE >( nRow2 - nRow1 ));    // rows-1 !
                if ( bInCell )
                {   // Spacing in spanning cell
                    Rectangle aCellRect = pDoc->GetMMRect(
                        nCol1, nRow1, nCol2, nRow2, nTab );
                    aSpace = MMToPixel( Size(
                        aCellRect.GetWidth() - aObjRect.GetWidth(),
                        aCellRect.GetHeight() - aObjRect.GetHeight() ));
                    aSpace.Width() += (nCol2-nCol1) * (nCellSpacing+1);
                    aSpace.Height() += (nRow2-nRow1) * (nCellSpacing+1);
                    aSpace.Width() /= 2;
                    aSpace.Height() /= 2;
                }
                ScHTMLGraphEntry* pE = new ScHTMLGraphEntry( pObject,
                    aR, aSize, bInCell, aSpace );
                aGraphList.Insert( pE, LIST_APPEND );
			}
			pObject = aIter.Next();
		}
	}
}


void ScHTMLExport::WriteGraphEntry( ScHTMLGraphEntry* pE )
{
	SdrObject* pObject = pE->pObject;
	ByteString aOpt;
	(((aOpt += ' ') += OOO_STRING_SVTOOLS_HTML_O_width) += '=') +=
		ByteString::CreateFromInt32( pE->aSize.Width() );
	(((aOpt += ' ') += OOO_STRING_SVTOOLS_HTML_O_height) += '=') +=
		ByteString::CreateFromInt32( pE->aSize.Height() );
	if ( pE->bInCell )
	{
		(((aOpt += ' ') += OOO_STRING_SVTOOLS_HTML_O_hspace) += '=') +=
			ByteString::CreateFromInt32( pE->aSpace.Width() );
		(((aOpt += ' ') += OOO_STRING_SVTOOLS_HTML_O_vspace) += '=') +=
			ByteString::CreateFromInt32( pE->aSpace.Height() );
	}
	switch ( pObject->GetObjIdentifier() )
	{
		case OBJ_GRAF:
		{
			const SdrGrafObj* pSGO = (SdrGrafObj*)pObject;
			const SdrGrafObjGeoData* pGeo = (SdrGrafObjGeoData*)pSGO->GetGeoData();
			USHORT nMirrorCase = (pGeo->aGeo.nDrehWink == 18000 ?
					( pGeo->bMirrored ? 3 : 4 ) : ( pGeo->bMirrored ? 2 : 1 ));
			BOOL bHMirr = ( ( nMirrorCase == 2 ) || ( nMirrorCase == 4 ) );
			BOOL bVMirr = ( ( nMirrorCase == 3 ) || ( nMirrorCase == 4 ) );
			ULONG nXOutFlags = 0;
			if ( bHMirr )
				nXOutFlags |= XOUTBMP_MIRROR_HORZ;
			if ( bVMirr )
				nXOutFlags |= XOUTBMP_MIRROR_VERT;
			String aLinkName;
			if ( pSGO->IsLinkedGraphic() )
				aLinkName = pSGO->GetFileName();
			WriteImage( aLinkName, pSGO->GetGraphic(), aOpt, nXOutFlags );
			pE->bWritten = TRUE;
		}
		break;
		case OBJ_OLE2:
		{
			Graphic* pGraphic = ((SdrOle2Obj*)pObject)->GetGraphic();
			if ( pGraphic )
			{
				String aLinkName;
				WriteImage( aLinkName, *pGraphic, aOpt );
				pE->bWritten = TRUE;
			}
		}
		break;
        default:
        {
            Graphic aGraph( SdrExchangeView::GetObjGraphic(
                pDoc->GetDrawLayer(), pObject ) );
            String aLinkName;
            WriteImage( aLinkName, aGraph, aOpt );
            pE->bWritten = TRUE;
        }
	}
}


void ScHTMLExport::WriteImage( String& rLinkName, const Graphic& rGrf,
			const ByteString& rImgOptions, ULONG nXOutFlags )
{
	// embeddete Grafik -> via WriteGraphic schreiben
	if( !rLinkName.Len() )
	{
		if( aStreamPath.Len() > 0 )
		{
			// Grafik als (JPG-)File speichern
			String aGrfNm( aStreamPath );
            nXOutFlags |= XOUTBMP_USE_NATIVE_IF_POSSIBLE;
            USHORT nErr = XOutBitmap::WriteGraphic( rGrf, aGrfNm,
                CREATE_STRING( "JPG" ), nXOutFlags );
			if( !nErr )		// sonst fehlerhaft, da ist nichts auszugeben
			{
                rLinkName = URIHelper::SmartRel2Abs(
                        INetURLObject(aBaseURL),
                        aGrfNm,
                        URIHelper::GetMaybeFileHdl());
				if ( HasCId() )
					MakeCIdURL( rLinkName );
			}
		}
	}
	else
	{
		if( bCopyLocalFileToINet || HasCId() )
		{
			CopyLocalFileToINet( rLinkName, aStreamPath );
			if ( HasCId() )
				MakeCIdURL( rLinkName );
		}
		else
            rLinkName = URIHelper::SmartRel2Abs(
                    INetURLObject(aBaseURL),
                    rLinkName,
                    URIHelper::GetMaybeFileHdl());
	}
	if( rLinkName.Len() )
	{	// <IMG SRC="..."[ rImgOptions]>
		rStrm << '<' << OOO_STRING_SVTOOLS_HTML_image << ' ' << OOO_STRING_SVTOOLS_HTML_O_src << "=\"";
        HTMLOutFuncs::Out_String( rStrm, URIHelper::simpleNormalizedMakeRelative(
                    aBaseURL,
                    rLinkName ), eDestEnc ) << '\"';
		if ( rImgOptions.Len() )
			rStrm << rImgOptions.GetBuffer();
		rStrm << '>' << sNewLine << GetIndentStr();
	}
}




