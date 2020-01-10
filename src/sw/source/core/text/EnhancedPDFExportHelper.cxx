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
#include <com/sun/star/embed/XEmbeddedObject.hpp>
#ifndef _COM_SUN_STAR_I18N_SCRIPTTYPE_HDL_
#include <com/sun/star/i18n/ScriptType.hdl>
#endif
#include <EnhancedPDFExportHelper.hxx>
#include <hintids.hxx>

#ifndef _OUTDEV_HXX
#include <vcl/outdev.hxx>
#endif
#include <tools/multisel.hxx>
#include <svx/adjitem.hxx>
#include <svx/lrspitem.hxx>
#include <svx/langitem.hxx>
#include <svx/scripttypeitem.hxx>
#include <tools/urlobj.hxx>
#include <svtools/zforlist.hxx>
#include <swatrset.hxx>
#include <frmatr.hxx>
#include <paratr.hxx>
#include <ndtxt.hxx>
#include <ndole.hxx>
#include <section.hxx>
#include <tox.hxx>
#include <fmtfld.hxx>
#include <txtinet.hxx>
#include <fmtinfmt.hxx>
#include <fchrfmt.hxx>
#include <charfmt.hxx>
#include <fmtanchr.hxx>
#include <fmturl.hxx>
#include <editsh.hxx>
#include <viscrs.hxx>
#include <txtfld.hxx>
#include <reffld.hxx>
#include <doc.hxx>
#include <docary.hxx>
#include <crsskip.hxx>
#include <mdiexp.hxx>
#include <docufld.hxx>
#include <ftnidx.hxx>
#include <txtftn.hxx>
#include <fmtftn.hxx>
#include <rootfrm.hxx>
#include <pagefrm.hxx>
#include <txtfrm.hxx>
#include <tabfrm.hxx>
#include <rowfrm.hxx>
#include <cellfrm.hxx>
#include <sectfrm.hxx>
#include <flyfrm.hxx>
#include <notxtfrm.hxx>
#include <porfld.hxx>
#include <SwStyleNameMapper.hxx>
#include <itrpaint.hxx>
#include "i18npool/mslangid.hxx"
#include <IMark.hxx>
#include <SwNodeNum.hxx>

#include <stack>

#include <tools/globname.hxx>

using namespace ::com::sun::star;

//
// Some static data structures
//
TableColumnsMap SwEnhancedPDFExportHelper::aTableColumnsMap;
LinkIdMap SwEnhancedPDFExportHelper::aLinkIdMap;
NumListIdMap SwEnhancedPDFExportHelper::aNumListIdMap;
NumListBodyIdMap SwEnhancedPDFExportHelper::aNumListBodyIdMap;
FrmTagIdMap SwEnhancedPDFExportHelper::aFrmTagIdMap;

LanguageType SwEnhancedPDFExportHelper::eLanguageDefault = 0;

#ifndef PRODUCT

static std::vector< USHORT > aStructStack;

void lcl_DBGCheckStack()
{
    /* NonStructElement = 0     Document = 1        Part = 2
     * Article = 3              Section = 4         Division = 5
     * BlockQuote = 6           Caption = 7         TOC = 8
     * TOCI = 9                 Index = 10          Paragraph = 11
     * Heading = 12             H1-6 = 13 - 18      List = 19
     * ListItem = 20            LILabel = 21        LIBody = 22
     * Table = 23               TableRow = 24       TableHeader = 25
     * TableData = 26           Span = 27           Quote = 28
     * Note = 29                Reference = 30      BibEntry = 31
     * Code = 32                Link = 33           Figure = 34
     * Formula = 35             Form = 36           Continued frame = 99
     */

    USHORT nElement;
    std::vector< USHORT >::iterator aIter;
    for ( aIter = aStructStack.begin(); aIter != aStructStack.end(); ++aIter )
    {
        nElement = *aIter;
    }
}

#endif

namespace
{
// ODF Style Names:
const String aTableHeadingName  = String::CreateFromAscii("Table Heading");
const String aQuotations        = String::CreateFromAscii("Quotations");
const String aCaption           = String::CreateFromAscii("Caption");
const String aHeading           = String::CreateFromAscii("Heading");
const String aQuotation         = String::CreateFromAscii("Quotation");
const String aSourceText        = String::CreateFromAscii("Source Text");

// PDF Tag Names:
const String aDocumentString = String::CreateFromAscii("Document");
const String aDivString = String::CreateFromAscii("Div");
const String aSectString = String::CreateFromAscii("Sect");
const String aHString = String::CreateFromAscii("H");
const String aH1String = String::CreateFromAscii("H1");
const String aH2String = String::CreateFromAscii("H2");
const String aH3String = String::CreateFromAscii("H3");
const String aH4String = String::CreateFromAscii("H4");
const String aH5String = String::CreateFromAscii("H5");
const String aH6String = String::CreateFromAscii("H6");
const String aListString = String::CreateFromAscii("L");
const String aListItemString = String::CreateFromAscii("LI");
const String aListBodyString = String::CreateFromAscii("LBody");
const String aBlockQuoteString = String::CreateFromAscii("BlockQuote");
const String aCaptionString = String::CreateFromAscii("Caption");
const String aIndexString = String::CreateFromAscii("Index");
const String aTOCString = String::CreateFromAscii("TOC");
const String aTOCIString = String::CreateFromAscii("TOCI");
const String aTableString = String::CreateFromAscii("Table");
const String aTRString = String::CreateFromAscii("TR");
const String aTDString = String::CreateFromAscii("TD");
const String aTHString = String::CreateFromAscii("TH");
const String aBibEntryString = String::CreateFromAscii("BibEntry");
const String aQuoteString = String::CreateFromAscii("Quote");
const String aSpanString = String::CreateFromAscii("Span");
const String aCodeString = String::CreateFromAscii("Code");
const String aFigureString = String::CreateFromAscii("Figure");
const String aFormulaString = String::CreateFromAscii("Formula");
const String aLinkString = String::CreateFromAscii("Link");
const String aNoteString = String::CreateFromAscii("Note");
const String aEmptyString = String::CreateFromAscii("");

// returns true if first paragraph in cell frame has 'table heading' style
bool lcl_IsHeadlineCell( const SwCellFrm& rCellFrm )
{
    bool bRet = false;

    const SwCntntFrm *pCnt = rCellFrm.ContainsCntnt();
    if ( pCnt && pCnt->IsTxtFrm() )
    {
        const SwTxtNode* pTxtNode = static_cast<const SwTxtFrm*>(pCnt)->GetTxtNode();
        const SwFmt* pTxtFmt = pTxtNode->GetFmtColl();

        String sStyleName;
        SwStyleNameMapper::FillProgName( pTxtFmt->GetName(), sStyleName, nsSwGetPoolIdFromName::GET_POOLID_TXTCOLL, sal_True );
        bRet = sStyleName == aTableHeadingName;
    }

    return bRet;
}

// List all frames for which the NonStructElement tag is set:
bool lcl_IsInNonStructEnv( const SwFrm& rFrm )
{
    bool bRet = false;

    if ( 0 != rFrm.FindFooterOrHeader() &&
           !rFrm.IsHeaderFrm() && !rFrm.IsFooterFrm() )
    {
        bRet = true;
    }
    else if ( rFrm.IsInTab() && !rFrm.IsTabFrm() )
    {
        const SwTabFrm* pTabFrm = rFrm.FindTabFrm();
        if ( rFrm.GetUpper() != pTabFrm &&
             pTabFrm->IsFollow() && pTabFrm->IsInHeadline( rFrm ) )
             bRet = true;
    }

    return bRet;
}

// Generate key from frame for reopening tags:
void* lcl_GetKeyFromFrame( const SwFrm& rFrm )
{
    void* pKey = 0;

    if ( rFrm.IsPageFrm() )
        pKey = (void*)static_cast<const SwPageFrm&>(rFrm).GetFmt()->getIDocumentSettingAccess();
    else if ( rFrm.IsTxtFrm() )
        pKey = (void*)static_cast<const SwTxtFrm&>(rFrm).GetTxtNode();
    else if ( rFrm.IsSctFrm() )
        pKey = (void*)static_cast<const SwSectionFrm&>(rFrm).GetSection();
    else if ( rFrm.IsTabFrm() )
        pKey = (void*)static_cast<const SwTabFrm&>(rFrm).GetTable();
    else if ( rFrm.IsRowFrm() )
        pKey = (void*)static_cast<const SwRowFrm&>(rFrm).GetTabLine();
    else if ( rFrm.IsCellFrm() )
    {
        const SwTabFrm* pTabFrm = rFrm.FindTabFrm();
        const SwTable* pTable = pTabFrm->GetTable();
        pKey = (void*) & static_cast<const SwCellFrm&>(rFrm).GetTabBox()->FindStartOfRowSpan( *pTable );
    }

    return pKey;
}

bool lcl_HasPreviousParaSameNumRule( const SwTxtNode& rNode )
{
    bool bRet = false;
    SwNodeIndex aIdx( rNode );
    const SwDoc* pDoc = rNode.GetDoc();
    const SwNodes& rNodes = pDoc->GetNodes();
    const SwNode* pNode = &rNode;
    const SwNumRule* pNumRule = rNode.GetNumRule();

    while (! (pNode == rNodes.DocumentSectionStartNode((SwNode*)&rNode) ) )
    {
        --aIdx;

        if (aIdx.GetNode().IsTxtNode())
        {
            const SwTxtNode* pPrevTxtNd = aIdx.GetNode().GetTxtNode();
            const SwNumRule * pPrevNumRule = pPrevTxtNd->GetNumRule();

            // We find the previous text node. Now check, if the previous text node
            // has the same numrule like rNode:
            if ( (pPrevNumRule == pNumRule) &&
				 (!pPrevTxtNd->IsOutline() == !rNode.IsOutline()))
                bRet = true;

            break;
        }

        pNode = &aIdx.GetNode();
    }
    return bRet;
}

} // end namespace

/*
 * SwTaggedPDFHelper::SwTaggedPDFHelper()
 */
SwTaggedPDFHelper::SwTaggedPDFHelper( const Num_Info* pNumInfo,
                                      const Frm_Info* pFrmInfo,
                                      const Por_Info* pPorInfo,
                                      OutputDevice& rOut )
  : nEndStructureElement( 0 ),
    nRestoreCurrentTag( -1 ),
    mpNumInfo( pNumInfo ),
    mpFrmInfo( pFrmInfo ),
    mpPorInfo( pPorInfo )
{
    mpPDFExtOutDevData =
        PTR_CAST( vcl::PDFExtOutDevData, rOut.GetExtOutDevData() );

    if ( mpPDFExtOutDevData && mpPDFExtOutDevData->GetIsExportTaggedPDF() )
    {
#ifndef PRODUCT
        sal_Int32 nCurrentStruct = mpPDFExtOutDevData->GetCurrentStructureElement();
        lcl_DBGCheckStack();
#endif
        if ( mpNumInfo )
            BeginNumberedListStructureElements();
        else if ( mpFrmInfo )
            BeginBlockStructureElements();
        else if ( mpPorInfo )
            BeginInlineStructureElements();
        else
            BeginTag( vcl::PDFWriter::NonStructElement, aEmptyString );

#ifndef PRODUCT
        nCurrentStruct = mpPDFExtOutDevData->GetCurrentStructureElement();
        lcl_DBGCheckStack();
#endif
    }
}


/*
 * SwTaggedPDFHelper::~SwTaggedPDFHelper()
 */
SwTaggedPDFHelper::~SwTaggedPDFHelper()
{
    if ( mpPDFExtOutDevData && mpPDFExtOutDevData->GetIsExportTaggedPDF() )
    {
#ifndef PRODUCT
        sal_Int32 nCurrentStruct = mpPDFExtOutDevData->GetCurrentStructureElement();
        lcl_DBGCheckStack();
#endif
        EndStructureElements();

#ifndef PRODUCT
        nCurrentStruct = mpPDFExtOutDevData->GetCurrentStructureElement();
        lcl_DBGCheckStack();
#endif

    }
}

/*
 * SwTaggedPDFHelper::CheckReopenTag()
 */
bool SwTaggedPDFHelper::CheckReopenTag()
{
    bool bRet = false;
    sal_Int32 nReopenTag = -1;
    bool bContinue = false; // in some cases we just have to reopen a tag without early returning

    if ( mpFrmInfo )
    {
        const SwFrm& rFrm = mpFrmInfo->mrFrm;
        const SwFrm* pKeyFrm = 0;
        void* pKey = 0;

        // Reopen an existing structure element if
        // - rFrm is not the first page frame (reopen Document tag)
        // - rFrm is a follow frame (reopen Master tag)
        // - rFrm is a fly frame anchored at content (reopen Anchor paragraph tag)
        // - rFrm is a fly frame anchord at page (reopen Document tag)
        // - rFrm is a follow flow row (reopen TableRow tag)
        // - rFrm is a cell frame in a follow flow row (reopen TableData tag)
        if ( ( rFrm.IsPageFrm() && static_cast<const SwPageFrm&>(rFrm).GetPrev() ) ||
             ( rFrm.IsFlowFrm() && SwFlowFrm::CastFlowFrm(&rFrm)->IsFollow() ) ||
             ( rFrm.IsRowFrm() && rFrm.IsInFollowFlowRow() ) ||
             ( rFrm.IsCellFrm() && const_cast<SwFrm&>(rFrm).GetPrevCellLeaf( MAKEPAGE_NONE ) ) )
        {
            pKeyFrm = &rFrm;
        }
        else if ( rFrm.IsFlyFrm() )
        {
            const SwFmtAnchor& rAnchor =
                static_cast<const SwFlyFrm*>(&rFrm)->GetFmt()->GetAnchor();
            if ( FLY_AT_CNTNT == rAnchor.GetAnchorId() ||
                 FLY_AUTO_CNTNT == rAnchor.GetAnchorId() ||
                 FLY_PAGE == rAnchor.GetAnchorId() )
            {
                pKeyFrm = static_cast<const SwFlyFrm&>(rFrm).GetAnchorFrm();
                bContinue = true;
            }
        }

        if ( pKeyFrm )
        {
            pKey = lcl_GetKeyFromFrame( *pKeyFrm );

            if ( pKey )
            {
                FrmTagIdMap& rFrmTagIdMap = SwEnhancedPDFExportHelper::GetFrmTagIdMap();
                const FrmTagIdMap::const_iterator aIter =  rFrmTagIdMap.find( pKey );
                nReopenTag = (*aIter).second;
            }
        }
    }

    if ( -1 != nReopenTag )
    {
        nRestoreCurrentTag = mpPDFExtOutDevData->GetCurrentStructureElement();
        const bool bSuccess = mpPDFExtOutDevData->SetCurrentStructureElement( nReopenTag );
        ASSERT( bSuccess, "Failed to reopen tag" )

#ifndef PRODUCT
        aStructStack.push_back( 99 );
#endif

        bRet = bSuccess;
    }

    return bRet && !bContinue;
}


/*
 * SwTaggedPDFHelper::CheckRestoreTag()
 */
bool SwTaggedPDFHelper::CheckRestoreTag() const
{
    bool bRet = false;
    if ( nRestoreCurrentTag != -1 )
    {
        const bool bSuccess = mpPDFExtOutDevData->SetCurrentStructureElement( nRestoreCurrentTag );
        (void)bSuccess;
        ASSERT( bSuccess, "Failed to restore reopened tag" )

#ifndef PRODUCT
        aStructStack.pop_back();
#endif

        bRet = true;
    }

    return bRet;
}


/*
 * SwTaggedPDFHelper::BeginTag()
 */
void SwTaggedPDFHelper::BeginTag( vcl::PDFWriter::StructElement eType, const String& rString )
{
    // write new tag
    const sal_Int32 nId = mpPDFExtOutDevData->BeginStructureElement( eType, rtl::OUString( rString ) );
    ++nEndStructureElement;

#ifndef PRODUCT
    aStructStack.push_back( static_cast<USHORT>(eType) );
#endif

    // Store the id of the current structure element if
    // - it is a list structure element
    // - it is a list body element with children
    // - rFrm is the first page frame
    // - rFrm is a master frame
    // - rFrm has objects anchored to it
    // - rFrm is a row frame or cell frame in a split table row

    if ( mpNumInfo )
    {
        const SwTxtFrm& rTxtFrm = static_cast<const SwTxtFrm&>(mpNumInfo->mrFrm);
        const SwTxtNode* pTxtNd = rTxtFrm.GetTxtNode();
        const SwNodeNum* pNodeNum = pTxtNd->GetNum();

        if ( vcl::PDFWriter::List == eType )
        {
            NumListIdMap& rNumListIdMap = SwEnhancedPDFExportHelper::GetNumListIdMap();
            rNumListIdMap[ pNodeNum ] = nId;
        }
        else if ( vcl::PDFWriter::LIBody == eType )
        {
            NumListBodyIdMap& rNumListBodyIdMap = SwEnhancedPDFExportHelper::GetNumListBodyIdMap();
            rNumListBodyIdMap[ pNodeNum ] = nId;
        }
    }
    else if ( mpFrmInfo )
    {
        const SwFrm& rFrm = mpFrmInfo->mrFrm;

        if ( ( rFrm.IsPageFrm() && !static_cast<const SwPageFrm&>(rFrm).GetPrev() ) ||
             ( rFrm.IsFlowFrm() && !SwFlowFrm::CastFlowFrm(&rFrm)->IsFollow() && SwFlowFrm::CastFlowFrm(&rFrm)->HasFollow() ) ||
             ( rFrm.IsTxtFrm() && rFrm.GetDrawObjs() ) ||
             ( rFrm.IsRowFrm() && rFrm.IsInSplitTableRow() ) ||
             ( rFrm.IsCellFrm() && const_cast<SwFrm&>(rFrm).GetNextCellLeaf( MAKEPAGE_NONE ) ) )
        {
            const void* pKey = lcl_GetKeyFromFrame( rFrm );

            if ( pKey )
            {
                FrmTagIdMap& rFrmTagIdMap = SwEnhancedPDFExportHelper::GetFrmTagIdMap();
                rFrmTagIdMap[ pKey ] = nId;
            }
        }
    }

    SetAttributes( eType );
}


/*
 * SwTaggedPDFHelper::EndTag()
 */
void SwTaggedPDFHelper::EndTag()
{
    mpPDFExtOutDevData->EndStructureElement();

#ifndef PRODUCT
    aStructStack.pop_back();
#endif
}


/*
 * SwTaggedPDFHelper::SetAttributes()
 *
 * Sets the attributes according to the structure type.
 */
void SwTaggedPDFHelper::SetAttributes( vcl::PDFWriter::StructElement eType )
{
    vcl::PDFWriter::StructAttributeValue eVal;
    sal_Int32 nVal;

    /*
     * ATTRIBUTES FOR BLSE
     */
    if ( mpFrmInfo )
    {
        const SwFrm* pFrm = &mpFrmInfo->mrFrm;
        SWRECTFN( pFrm )

        bool bPlacement = false;
        bool bWritingMode = false;
        bool bSpaceBefore = false;
        bool bSpaceAfter = false;
        bool bStartIndent = false;
        bool bEndIndent = false;
        bool bTextIndent = false;
        bool bTextAlign = false;
        bool bAlternateText = false;
        bool bWidth = false;
        bool bHeight = false;
        bool bBox = false;
        bool bRowSpan = false;

        //
        // Check which attributes to set:
        //
        switch ( eType )
        {
            case vcl::PDFWriter::Document :
                bWritingMode = true;
                break;

            case vcl::PDFWriter::Table :
                bPlacement =
                bWritingMode =
                bSpaceBefore =
                bSpaceAfter =
                bStartIndent =
                bEndIndent =
                bWidth =
                bHeight =
                bBox = true;
                break;

            case vcl::PDFWriter::TableRow :
                bPlacement =
                bWritingMode = true;
                break;

            case vcl::PDFWriter::TableHeader :
            case vcl::PDFWriter::TableData :
                bPlacement =
                bWritingMode =
                bWidth =
                bHeight =
                bRowSpan = true;
                break;

            case vcl::PDFWriter::H1 :
            case vcl::PDFWriter::H2 :
            case vcl::PDFWriter::H3 :
            case vcl::PDFWriter::H4 :
            case vcl::PDFWriter::H5 :
            case vcl::PDFWriter::H6 :
            case vcl::PDFWriter::Paragraph :
            case vcl::PDFWriter::Heading :
            case vcl::PDFWriter::Caption :
            case vcl::PDFWriter::BlockQuote :

                bPlacement =
                bWritingMode =
                bSpaceBefore =
                bSpaceAfter =
                bStartIndent =
                bEndIndent =
                bTextIndent =
                bTextAlign = true;
                break;

            case vcl::PDFWriter::Formula :
            case vcl::PDFWriter::Figure :
                bPlacement =
                bAlternateText =
                bWidth =
                bHeight =
                bBox = true;
                break;
            default :
                break;
        }

        //
        // Set the attributes:
        //
        if ( bPlacement )
        {
            eVal = vcl::PDFWriter::TableHeader == eType ||
                   vcl::PDFWriter::TableData   == eType ?
                   vcl::PDFWriter::Inline :
                   vcl::PDFWriter::Block;

            mpPDFExtOutDevData->SetStructureAttribute( vcl::PDFWriter::Placement, eVal );
        }

        if ( bWritingMode )
        {
            eVal =  pFrm->IsVertical() ?
                    vcl::PDFWriter::TbRl :
                    pFrm->IsRightToLeft() ?
                    vcl::PDFWriter::RlTb :
                    vcl::PDFWriter::LrTb;

            if ( vcl::PDFWriter::LrTb != eVal )
                mpPDFExtOutDevData->SetStructureAttribute( vcl::PDFWriter::WritingMode, eVal );
        }

        if ( bSpaceBefore )
        {
            nVal = (pFrm->*fnRect->fnGetTopMargin)();
            if ( 0 != nVal )
                mpPDFExtOutDevData->SetStructureAttributeNumerical( vcl::PDFWriter::SpaceBefore, nVal );
        }

        if ( bSpaceAfter )
        {
            nVal = (pFrm->*fnRect->fnGetBottomMargin)();
            if ( 0 != nVal )
                mpPDFExtOutDevData->SetStructureAttributeNumerical( vcl::PDFWriter::SpaceAfter, nVal );
        }

        if ( bStartIndent )
        {
            nVal = (pFrm->*fnRect->fnGetLeftMargin)();
            if ( 0 != nVal )
                mpPDFExtOutDevData->SetStructureAttributeNumerical( vcl::PDFWriter::StartIndent, nVal );
        }

        if ( bEndIndent )
        {
            nVal = (pFrm->*fnRect->fnGetRightMargin)();
            if ( 0 != nVal )
                mpPDFExtOutDevData->SetStructureAttributeNumerical( vcl::PDFWriter::EndIndent, nVal );
        }

        if ( bTextIndent )
        {
            ASSERT( pFrm->IsTxtFrm(), "Frame type <-> tag attribute mismatch" )
            const SvxLRSpaceItem &rSpace =
                static_cast<const SwTxtFrm*>(pFrm)->GetTxtNode()->GetSwAttrSet().GetLRSpace();
            nVal =  rSpace.GetTxtFirstLineOfst();
            if ( 0 != nVal )
                mpPDFExtOutDevData->SetStructureAttributeNumerical( vcl::PDFWriter::TextIndent, nVal );
        }

        if ( bTextAlign )
        {
            ASSERT( pFrm->IsTxtFrm(), "Frame type <-> tag attribute mismatch" )
            const SwAttrSet& aSet = static_cast<const SwTxtFrm*>(pFrm)->GetTxtNode()->GetSwAttrSet();
            const SvxAdjust nAdjust = aSet.GetAdjust().GetAdjust();
            if ( SVX_ADJUST_BLOCK == nAdjust || SVX_ADJUST_CENTER == nAdjust ||
                 (  (pFrm->IsRightToLeft() && SVX_ADJUST_LEFT == nAdjust) ||
                   (!pFrm->IsRightToLeft() && SVX_ADJUST_RIGHT == nAdjust) ) )
            {
                eVal = SVX_ADJUST_BLOCK == nAdjust ?
                       vcl::PDFWriter::Justify :
                       SVX_ADJUST_CENTER == nAdjust ?
                       vcl::PDFWriter::Center :
                       vcl::PDFWriter::End;

                mpPDFExtOutDevData->SetStructureAttribute( vcl::PDFWriter::TextAlign, eVal );
            }
        }

        if ( bAlternateText )
        {
            ASSERT( pFrm->IsFlyFrm(), "Frame type <-> tag attribute mismatch" )
            const SwFlyFrm* pFly = static_cast<const SwFlyFrm*>(pFrm);
            if ( pFly->Lower() && pFly->Lower()->IsNoTxtFrm() )
            {
                const SwNoTxtFrm* pNoTxtFrm   = static_cast<const SwNoTxtFrm*>(pFly->Lower());
                const SwNoTxtNode* pNoTxtNode = static_cast<const SwNoTxtNode*>(pNoTxtFrm->GetNode());

                const String aAlternateTxt( pNoTxtNode->GetTitle() );
                mpPDFExtOutDevData->SetAlternateText( aAlternateTxt );
            }
        }

        if ( bWidth )
        {
            nVal = (pFrm->Frm().*fnRect->fnGetWidth)();
            mpPDFExtOutDevData->SetStructureAttributeNumerical( vcl::PDFWriter::Width, nVal );
        }

        if ( bHeight )
        {
            nVal = (pFrm->Frm().*fnRect->fnGetHeight)();
            mpPDFExtOutDevData->SetStructureAttributeNumerical( vcl::PDFWriter::Height, nVal );
        }

        if ( bBox )
        {
            // BBox only for non-split tables:
            if ( vcl::PDFWriter::Table != eType ||
                 ( pFrm->IsTabFrm() &&
                   !static_cast<const SwTabFrm*>(pFrm)->IsFollow() &&
                   !static_cast<const SwTabFrm*>(pFrm)->HasFollow() ) )
                mpPDFExtOutDevData->SetStructureBoundingBox( pFrm->Frm().SVRect() );
        }

        if ( bRowSpan )
        {
            const SwCellFrm* pThisCell = dynamic_cast<const SwCellFrm*>(pFrm);
            if ( pThisCell )
            {
                nVal =  pThisCell->GetTabBox()->getRowSpan();
                if ( nVal > 1 )
                    mpPDFExtOutDevData->SetStructureAttributeNumerical( vcl::PDFWriter::RowSpan, nVal );

                // calculate colspan:
                const SwTabFrm* pTabFrm = pThisCell->FindTabFrm();
                const SwTable* pTable = pTabFrm->GetTable();

                SWRECTFNX( pTabFrm )

                const TableColumnsMapEntry& rCols = SwEnhancedPDFExportHelper::GetTableColumnsMap()[ pTable ];

                const long nLeft  = (pThisCell->Frm().*fnRectX->fnGetLeft)();
                const long nRight = (pThisCell->Frm().*fnRectX->fnGetRight)();
                const TableColumnsMapEntry::const_iterator aLeftIter =  rCols.find( nLeft );
                const TableColumnsMapEntry::const_iterator aRightIter = rCols.find( nRight );

                ASSERT( aLeftIter != rCols.end() && aRightIter != rCols.end(), "Colspan trouble" )
                if ( aLeftIter != rCols.end() && aRightIter != rCols.end() )
                {
                    nVal = std::distance( aLeftIter, aRightIter );
                    if ( nVal > 1 )
                        mpPDFExtOutDevData->SetStructureAttributeNumerical( vcl::PDFWriter::ColSpan, nVal );
                }
            }
        }
    }

    /*
     * ATTRIBUTES FOR ILSE
     */
    else if ( mpPorInfo )
    {
        const SwLinePortion* pPor = &mpPorInfo->mrPor;
        const SwTxtPaintInfo& rInf = mpPorInfo->mrTxtPainter.GetInfo();

        bool bActualText = false;
        bool bBaselineShift = false;
        bool bTextDecorationType = false;
        bool bLinkAttribute = false;
        bool bLanguage = false;

        //
        // Check which attributes to set:
        //
        switch ( eType )
        {
            case vcl::PDFWriter::Span :
            case vcl::PDFWriter::Quote :
            case vcl::PDFWriter::Code :
                if( POR_HYPHSTR == pPor->GetWhichPor() || POR_SOFTHYPHSTR == pPor->GetWhichPor() )
                    bActualText = true;
                else
                {
                    bBaselineShift =
                    bTextDecorationType =
                    bLanguage = true;
                }
                break;

            case vcl::PDFWriter::Link :
                bTextDecorationType =
                bBaselineShift =
                bLinkAttribute =
                bLanguage = true;
                break;

            default:
                break;
        }

        if ( bActualText )
        {
            const String aActualTxt( rInf.GetTxt(), rInf.GetIdx(), pPor->GetLen() );
            mpPDFExtOutDevData->SetActualText( aActualTxt );
        }

        if ( bBaselineShift )
        {
            // TODO: Calculate correct values!
            nVal = rInf.GetFont()->GetEscapement();
            if ( nVal > 0 ) nVal = 33;
            else if ( nVal < 0 ) nVal = -33;

            if ( 0 != nVal )
            {
                nVal = nVal * pPor->Height() / 100;
                mpPDFExtOutDevData->SetStructureAttributeNumerical( vcl::PDFWriter::BaselineShift, nVal );
            }
        }

        if ( bTextDecorationType )
        {
            if ( UNDERLINE_NONE    != rInf.GetFont()->GetUnderline() )
                mpPDFExtOutDevData->SetStructureAttribute( vcl::PDFWriter::TextDecorationType, vcl::PDFWriter::Underline );
            if ( UNDERLINE_NONE    != rInf.GetFont()->GetOverline() )
                mpPDFExtOutDevData->SetStructureAttribute( vcl::PDFWriter::TextDecorationType, vcl::PDFWriter::Overline );
            if ( STRIKEOUT_NONE    != rInf.GetFont()->GetStrikeout() )
                mpPDFExtOutDevData->SetStructureAttribute( vcl::PDFWriter::TextDecorationType, vcl::PDFWriter::LineThrough );
            if ( EMPHASISMARK_NONE != rInf.GetFont()->GetEmphasisMark() )
                mpPDFExtOutDevData->SetStructureAttribute( vcl::PDFWriter::TextDecorationType, vcl::PDFWriter::Overline );
        }

        if ( bLanguage )
        {

            const LanguageType nCurrentLanguage = rInf.GetFont()->GetLanguage();
            const LanguageType nDefaultLang = SwEnhancedPDFExportHelper::GetDefaultLanguage();

            if ( nDefaultLang != nCurrentLanguage )
                mpPDFExtOutDevData->SetStructureAttributeNumerical( vcl::PDFWriter::Language, nCurrentLanguage );
        }

        if ( bLinkAttribute )
        {
            const LinkIdMap& rLinkIdMap = SwEnhancedPDFExportHelper::GetLinkIdMap();
            SwRect aPorRect;
            rInf.CalcRect( *pPor, &aPorRect );
            const Point aPorCenter = aPorRect.Center();
            LinkIdMap::const_iterator aIter;
            for ( aIter = rLinkIdMap.begin(); aIter != rLinkIdMap.end(); ++aIter )
            {
                const SwRect& rLinkRect = (*aIter).first;
                if ( rLinkRect.IsInside( aPorCenter ) )
                {
                    sal_Int32 nLinkId = (*aIter).second;
                    mpPDFExtOutDevData->SetStructureAttributeNumerical( vcl::PDFWriter::LinkAnnotation, nLinkId );
                    break;
                }
            }
        }
    }
}

/*
 * SwTaggedPDFHelper::BeginNumberedListStructureElements()
 */
void SwTaggedPDFHelper::BeginNumberedListStructureElements()
{
    ASSERT( mpNumInfo, "List without mpNumInfo?" )
    if ( !mpNumInfo )
        return;

    const SwFrm& rFrm = mpNumInfo->mrFrm;
    ASSERT( rFrm.IsTxtFrm(), "numbered only for text frames" )
    const SwTxtFrm& rTxtFrm = static_cast<const SwTxtFrm&>(rFrm);

    //
    // Lowers of NonStructureElements should not be considered:
    //
    if ( lcl_IsInNonStructEnv( rTxtFrm ) || rTxtFrm.IsFollow() )
        return;

    const SwTxtNode* pTxtNd = rTxtFrm.GetTxtNode();
    const SwNumRule* pNumRule = pTxtNd->GetNumRule();
    const SwNodeNum* pNodeNum = pTxtNd->GetNum();

    const bool bNumbered = !pTxtNd->IsOutline() && pNodeNum && pNodeNum->GetParent() && pNumRule;

    // Check, if we have to reopen a list or a list body:
    // First condition:
    // Paragraph is numbered/bulleted
    if ( !bNumbered )
        return;

    const SwNumberTreeNode* pParent = pNodeNum->GetParent();
    const bool bSameNumbering = lcl_HasPreviousParaSameNumRule(*pTxtNd);

    // Second condition: current numbering is not 'interrupted'
    if ( bSameNumbering )
    {
        sal_Int32 nReopenTag = -1;

        // Two cases:
        // 1. We have to reopen an existing list body tag:
        // - If the current node is either the first child of its parent
        //   and its level > 1 or
        // - Numbering should restart at the current node and its level > 1
        // - The current item has no label
        const bool bNewSubListStart = pParent->GetParent() && (pParent->IsFirst( pNodeNum ) || pTxtNd->IsListRestart() );
        const bool bNoLabel = !pTxtNd->IsCountedInList() && !pTxtNd->IsListRestart();
        if ( bNewSubListStart || bNoLabel )
        {
            // Fine, we try to reopen the appropriate list body
            NumListBodyIdMap& rNumListBodyIdMap = SwEnhancedPDFExportHelper::GetNumListBodyIdMap();

            if ( bNewSubListStart )
            {
                // The list body tag associated with the parent has to be reopened
                // to start a new list inside the list body
                NumListBodyIdMap::const_iterator aIter;

                do
                    aIter = rNumListBodyIdMap.find( pParent );
                while ( aIter == rNumListBodyIdMap.end() && 0 != ( pParent = pParent->GetParent() ) );

                if ( aIter != rNumListBodyIdMap.end() )
                    nReopenTag = (*aIter).second;
            }
            else // if(bNoLabel)
            {
                // The list body tag of a 'counted' predecessor has to be reopened
                const SwNumberTreeNode* pPrevious = pNodeNum->GetPred(true);
                while ( pPrevious )
                {
                    if ( pPrevious->IsCounted())
                    {
                        // get id of list body tag
                        const NumListBodyIdMap::const_iterator aIter =  rNumListBodyIdMap.find( pPrevious );
                        if ( aIter != rNumListBodyIdMap.end() )
                        {
                            nReopenTag = (*aIter).second;
                            break;
                        }
                    }
                    pPrevious = pPrevious->GetPred(true);
                }
            }
        }
        // 2. We have to reopen an existing list tag:
        else if ( !pParent->IsFirst( pNodeNum ) && !pTxtNd->IsListRestart() )
        {
            // any other than the first node in a list level has to reopen the current
            // list. The current list is associated in a map with the first child of the list:
            NumListIdMap& rNumListIdMap = SwEnhancedPDFExportHelper::GetNumListIdMap();

            // Search backwards and check if any of the previous nodes has a list associated with it:
            const SwNumberTreeNode* pPrevious = pNodeNum->GetPred(true);
            while ( pPrevious )
            {
                // get id of list tag
                const NumListIdMap::const_iterator aIter =  rNumListIdMap.find( pPrevious );
                if ( aIter != rNumListIdMap.end() )
                {
                    nReopenTag = (*aIter).second;
                    break;
                }

                pPrevious = pPrevious->GetPred(true);
            }
        }

        if ( -1 != nReopenTag )
        {
            nRestoreCurrentTag = mpPDFExtOutDevData->GetCurrentStructureElement();
            mpPDFExtOutDevData->SetCurrentStructureElement( nReopenTag );

#ifndef PRODUCT
            aStructStack.push_back( 99 );
#endif
        }
    }
    else
    {
        // clear list maps in case a list has been interrupted
        NumListIdMap& rNumListIdMap = SwEnhancedPDFExportHelper::GetNumListIdMap();
        rNumListIdMap.clear();
        NumListBodyIdMap& rNumListBodyIdMap = SwEnhancedPDFExportHelper::GetNumListBodyIdMap();
        rNumListBodyIdMap.clear();
    }

    // New tags:
    const bool bNewListTag = (pNodeNum->GetParent()->IsFirst( pNodeNum ) || pTxtNd->IsListRestart() || !bSameNumbering);
    const bool bNewItemTag = bNewListTag || pTxtNd->IsCountedInList(); // If the text node is not counted, we do not start a new list item:

    if ( bNewListTag )
        BeginTag( vcl::PDFWriter::List, aListString );

    if ( bNewItemTag )
    {
        BeginTag( vcl::PDFWriter::ListItem, aListItemString );
        BeginTag( vcl::PDFWriter::LIBody, aListBodyString );
    }
}

/*
 * SwTaggedPDFHelper::BeginBlockStructureElements()
 */
void SwTaggedPDFHelper::BeginBlockStructureElements()
{
    const SwFrm* pFrm = &mpFrmInfo->mrFrm;

    //
    // Lowers of NonStructureElements should not be considered:
    //
    if ( lcl_IsInNonStructEnv( *pFrm ) )
        return;

    // Check if we have to reopen an existing structure element.
    // This has to be done e.g., if pFrm is a follow frame.
    if ( CheckReopenTag() )
        return;

    USHORT nPDFType = USHRT_MAX;
    String aPDFType;

    switch ( pFrm->GetType() )
    {
        /*
         * GROUPING ELEMENTS
         */

        case FRM_PAGE :
            //
            // Document: Document
            //
            nPDFType = vcl::PDFWriter::Document;
            aPDFType = aDocumentString;
            break;

        case FRM_HEADER :
        case FRM_FOOTER :
            //
            // Header, Footer: NonStructElement
            //
            nPDFType = vcl::PDFWriter::NonStructElement;
            break;

        case FRM_FTNCONT :
            //
            // Footnote container: Division
            //
            nPDFType = vcl::PDFWriter::Division;
            aPDFType = aDivString;
            break;

        case FRM_FTN :
            //
            // Footnote frame: Note
            //
            // Note: vcl::PDFWriter::Note is actually a ILSE. Nevertheless
            // we treat it like a grouping element!
            nPDFType = vcl::PDFWriter::Note;
            aPDFType = aNoteString;
            break;

        case FRM_SECTION :
            //
            // Section: TOX, Index, or Sect
            //
            {
                const SwSection* pSection =
                        static_cast<const SwSectionFrm*>(pFrm)->GetSection();
                if ( TOX_CONTENT_SECTION == pSection->GetType() )
                {
                    const SwTOXBase* pTOXBase = pSection->GetTOXBase();
                    if ( pTOXBase )
                    {
                        if ( TOX_INDEX == pTOXBase->GetType() )
                        {
                            nPDFType = vcl::PDFWriter::Index;
                            aPDFType = aIndexString;
                        }
                        else
                        {
                            nPDFType = vcl::PDFWriter::TOC;
                            aPDFType = aTOCString;
                        }
                    }
                }
                else if ( CONTENT_SECTION == pSection->GetType() )
                {
                    nPDFType = vcl::PDFWriter::Section;
                    aPDFType = aSectString;
                }
            }
            break;

        /*
         * BLOCK-LEVEL STRUCTURE ELEMENTS
         */

        case FRM_TXT :
            {
                const SwTxtNode* pTxtNd =
                    static_cast<const SwTxtFrm*>(pFrm)->GetTxtNode();

                const SwFmt* pTxtFmt = pTxtNd->GetFmtColl();
                const SwFmt* pParentTxtFmt = pTxtFmt->DerivedFrom();

                String sStyleName;
                String sParentStyleName;

                if ( pTxtFmt)
                    SwStyleNameMapper::FillProgName( pTxtFmt->GetName(), sStyleName, nsSwGetPoolIdFromName::GET_POOLID_TXTCOLL, sal_True );
                if ( pParentTxtFmt)
                    SwStyleNameMapper::FillProgName( pParentTxtFmt->GetName(), sParentStyleName, nsSwGetPoolIdFromName::GET_POOLID_TXTCOLL, sal_True );

                // This is the default. If the paragraph could not be mapped to
                // any of the standard pdf tags, we write a user defined tag
                // <stylename> with role = P
                nPDFType = static_cast<USHORT>(vcl::PDFWriter::Paragraph);
                aPDFType = sStyleName;

                //
                // Quotations: BlockQuote
                //
                if ( sStyleName == aQuotations )
                {
                    nPDFType = static_cast<USHORT>(vcl::PDFWriter::BlockQuote);
                    aPDFType = aBlockQuoteString;
                }

                //
                // Caption: Caption
                //
                else if ( sStyleName == aCaption)
                {
                    nPDFType = static_cast<USHORT>(vcl::PDFWriter::Caption);
                    aPDFType = aCaptionString;
                }

                //
                // Caption: Caption
                //
                else if ( sParentStyleName == aCaption)
                {
                    nPDFType = static_cast<USHORT>(vcl::PDFWriter::Caption);
                    aPDFType = sStyleName.Append(aCaptionString);
                }

                //
                // Heading: H
                //
                else if ( sStyleName == aHeading )
                {
                    nPDFType = static_cast<USHORT>(vcl::PDFWriter::Heading);
                    aPDFType = aHString;
                }

                //
                // Heading: H1 - H6
                //
                if ( pTxtNd->IsOutline() )
                {
                    //int nRealLevel = pTxtNd->GetOutlineLevel();	//#outline level,zhaojianwei
					int nRealLevel = pTxtNd->GetAttrOutlineLevel()-1;		//<-end,zhaojianwei
                   nRealLevel = nRealLevel > 5 ? 5 : nRealLevel;

                    nPDFType =  static_cast<USHORT>(vcl::PDFWriter::H1 + nRealLevel);
                    switch(nRealLevel)
                    {
                        case 0 :
                            aPDFType = aH1String;
                            break;
                        case 1 :
                            aPDFType = aH2String;
                            break;
                        case 2 :
                            aPDFType = aH3String;
                            break;
                        case 3 :
                            aPDFType = aH4String;
                            break;
                        case 4 :
                            aPDFType = aH5String;
                            break;
                        default:
                            aPDFType = aH6String;
                            break;
                    }
                }

                //
                // Section: TOCI
                //
                else if ( pFrm->IsInSct() )
                {
                    const SwSectionFrm* pSctFrm = pFrm->FindSctFrm();
                    const SwSection* pSection =
                            static_cast<const SwSectionFrm*>(pSctFrm)->GetSection();

                    if ( TOX_CONTENT_SECTION == pSection->GetType() )
                    {
                        const SwTOXBase* pTOXBase = pSection->GetTOXBase();
                        if ( pTOXBase && TOX_INDEX != pTOXBase->GetType() )
                        {
                            // Special case: Open additional TOCI tag:
                            BeginTag( vcl::PDFWriter::TOCI, aTOCIString );
                        }
                    }
                }
            }
            break;

        case FRM_TAB :
            //
            // TabFrm: Table
            //
            nPDFType = vcl::PDFWriter::Table;
            aPDFType = aTableString;

            {
                // set up table column data:
                const SwTabFrm* pTabFrm = static_cast<const SwTabFrm*>(pFrm);
                const SwTable* pTable = pTabFrm->GetTable();

                TableColumnsMap& rTableColumnsMap = SwEnhancedPDFExportHelper::GetTableColumnsMap();
                const TableColumnsMap::const_iterator aIter = rTableColumnsMap.find( pTable );

                if ( aIter == rTableColumnsMap.end() )
                {
                    SWRECTFN( pTabFrm )
                    TableColumnsMapEntry& rCols = rTableColumnsMap[ pTable ];

                    const SwTabFrm* pMasterFrm = pTabFrm->IsFollow() ? pTabFrm->FindMaster( true ) : pTabFrm;

                    while ( pMasterFrm )
                    {
                        const SwRowFrm* pRowFrm = static_cast<const SwRowFrm*>(pMasterFrm->GetLower());

                        while ( pRowFrm )
                        {
                            const SwFrm* pCellFrm = pRowFrm->GetLower();

                            const long nLeft  = (pCellFrm->Frm().*fnRect->fnGetLeft)();
                            rCols.insert( nLeft );

                            while ( pCellFrm )
                            {
                                const long nRight = (pCellFrm->Frm().*fnRect->fnGetRight)();
                                rCols.insert( nRight );
                                pCellFrm = pCellFrm->GetNext();
                            }
                            pRowFrm = static_cast<const SwRowFrm*>(pRowFrm->GetNext());
                        }
                        pMasterFrm = static_cast<const SwTabFrm*>(pMasterFrm->GetFollow());
                    }
                }
            }

            break;

        /*
         * TABLE ELEMENTS
         */

        case FRM_ROW :
            //
            // RowFrm: TR
            //
            if ( !static_cast<const SwRowFrm*>(pFrm)->IsRepeatedHeadline() )
            {
                nPDFType = vcl::PDFWriter::TableRow;
                aPDFType = aTRString;
            }
            else
            {
                nPDFType = vcl::PDFWriter::NonStructElement;
            }
            break;

        case FRM_CELL :
            //
            // CellFrm: TH, TD
            //
            {
                const SwTabFrm* pTable = static_cast<const SwCellFrm*>(pFrm)->FindTabFrm();
                if ( pTable->IsInHeadline( *pFrm ) || lcl_IsHeadlineCell( *static_cast<const SwCellFrm*>(pFrm) ) )
                {
                    nPDFType = vcl::PDFWriter::TableHeader;
                    aPDFType = aTHString;
                }
                else
                {
                    nPDFType = vcl::PDFWriter::TableData;
                    aPDFType = aTDString;
                }
            }
            break;

        /*
         * ILLUSTRATION
         */

        case FRM_FLY :
            //
            // FlyFrm: Figure, Formula, Control
            // fly in content or fly at page
            {
                bool bFormula = false;
                const SwFlyFrm* pFly = static_cast<const SwFlyFrm*>(pFrm);
                if ( pFly->Lower() && pFly->Lower()->IsNoTxtFrm() )
                {
                    const SwNoTxtFrm* pNoTxtFrm = static_cast<const SwNoTxtFrm*>(pFly->Lower());
                    SwOLENode* pOLENd = const_cast<SwOLENode*>(pNoTxtFrm->GetNode()->GetOLENode());
                    if ( pOLENd )
                    {
                        SwOLEObj& aOLEObj = pOLENd->GetOLEObj();
                        uno::Reference< embed::XEmbeddedObject > aRef = aOLEObj.GetOleRef();
                        if ( aRef.is() )
                        {
                            bFormula = 0 != SotExchange::IsMath( SvGlobalName( aRef->getClassID() ) );
                        }
                    }
                    if ( bFormula )
                    {
                        nPDFType = vcl::PDFWriter::Formula;
                        aPDFType = aFormulaString;
                    }
                    else
                    {
                        nPDFType = vcl::PDFWriter::Figure;
                        aPDFType = aFigureString;
                    }
                }
                else
                {
                    nPDFType = vcl::PDFWriter::Division;
                    aPDFType = aDivString;
                }
            }
            break;
    }

    if ( USHRT_MAX != nPDFType )
    {
        BeginTag( static_cast<vcl::PDFWriter::StructElement>(nPDFType), aPDFType );
    }
}


/*
 * SwTaggedPDFHelper::EndStructureElements()
 */
void SwTaggedPDFHelper::EndStructureElements()
{
    while ( nEndStructureElement > 0 )
    {
        EndTag();
        --nEndStructureElement;
    }

    CheckRestoreTag();
}


/*
 * SwTaggedPDFHelper::BeginInlineStructureElements()
 */
void SwTaggedPDFHelper::BeginInlineStructureElements()
{
    const SwLinePortion* pPor = &mpPorInfo->mrPor;
    const SwTxtPaintInfo& rInf = mpPorInfo->mrTxtPainter.GetInfo();
    const SwTxtFrm* pFrm = rInf.GetTxtFrm();

    //
    // Lowers of NonStructureElements should not be considered:
    //
    if ( lcl_IsInNonStructEnv( *pFrm ) )
        return;

    USHORT nPDFType = USHRT_MAX;
    String aPDFType;

    switch ( pPor->GetWhichPor() )
    {
        // Check for alternative spelling:
        case POR_HYPHSTR :
        case POR_SOFTHYPHSTR :
            nPDFType = vcl::PDFWriter::Span;
            aPDFType = aSpanString;
            break;

        case POR_LAY :
        case POR_TXT :
        case POR_PARA :
            {
                SwTxtNode* pNd = (SwTxtNode*)pFrm->GetTxtNode();
                SwIndex aIndex( pNd, rInf.GetIdx() );
		        const SwTxtAttr* pInetFmtAttr = pNd->GetTxtAttr( aIndex, RES_TXTATR_INETFMT );

                String sStyleName;
                if ( !pInetFmtAttr )
                {
                    const SwTxtAttr* pCharFmtAttr = pNd->GetTxtAttr( aIndex, RES_TXTATR_CHARFMT );
                    const SwCharFmt* pCharFmt = pCharFmtAttr ? pCharFmtAttr->GetCharFmt().GetCharFmt() : 0;
                    if ( pCharFmt )
                        SwStyleNameMapper::FillProgName( pCharFmt->GetName(), sStyleName, nsSwGetPoolIdFromName::GET_POOLID_TXTCOLL, sal_True );
                }

                // Check for Link:
		        if( pInetFmtAttr )
		        {
                    nPDFType = vcl::PDFWriter::Link;
                    aPDFType = aLinkString;
                }
                // Check for Quote/Code character style:
                else if ( sStyleName == aQuotation )
                {
                    nPDFType = vcl::PDFWriter::Quote;
                    aPDFType = aQuoteString;
                }
                else if ( sStyleName == aSourceText )
                {
                    nPDFType = vcl::PDFWriter::Code;
                    aPDFType = aCodeString;
                }
                else
                {
                    const LanguageType nCurrentLanguage = rInf.GetFont()->GetLanguage();
                    const USHORT nFont = rInf.GetFont()->GetActual();
                    const LanguageType nDefaultLang = SwEnhancedPDFExportHelper::GetDefaultLanguage();

                    if ( UNDERLINE_NONE    != rInf.GetFont()->GetUnderline() ||
                         UNDERLINE_NONE    != rInf.GetFont()->GetOverline()  ||
                         STRIKEOUT_NONE    != rInf.GetFont()->GetStrikeout() ||
                         EMPHASISMARK_NONE != rInf.GetFont()->GetEmphasisMark() ||
                         0                 != rInf.GetFont()->GetEscapement() ||
                         SW_LATIN          != nFont ||
                         nCurrentLanguage  != nDefaultLang ||
                         sStyleName.Len()  > 0 )
                    {
                        nPDFType = vcl::PDFWriter::Span;
                        if ( sStyleName.Len() > 0 )
                            aPDFType = sStyleName;
                        else
                            aPDFType = aSpanString;
                    }
                }
            }
            break;

        case POR_FTN :
            nPDFType = vcl::PDFWriter::Link;
            aPDFType = aLinkString;
            break;

        case POR_FLD :
            {
                // check field type:
                const xub_StrLen nIdx = static_cast<const SwFldPortion*>(pPor)->IsFollow() ?
                                        rInf.GetIdx() - 1 :
                                        rInf.GetIdx();
                const SwTxtAttr* pHint = mpPorInfo->mrTxtPainter.GetAttr( nIdx );
                const SwField* pFld = 0;
                if ( pHint && RES_TXTATR_FIELD == pHint->Which() )
                {
                    pFld = (SwField*)pHint->GetFld().GetFld();
                    if ( RES_GETREFFLD == pFld->Which() )
                    {
                        nPDFType = vcl::PDFWriter::Link;
                        aPDFType = aLinkString;
                    }
                    else if ( RES_AUTHORITY == pFld->Which() )
                    {
                        nPDFType = vcl::PDFWriter::BibEntry;
                        aPDFType = aBibEntryString;
                    }
                }
            }
            break;

        case POR_TAB :
        case POR_TABRIGHT :
        case POR_TABCENTER :
        case POR_TABDECIMAL :
            nPDFType = vcl::PDFWriter::NonStructElement;
            break;
    }

    if ( USHRT_MAX != nPDFType )
    {
        BeginTag( static_cast<vcl::PDFWriter::StructElement>(nPDFType), aPDFType );
    }
}

/*
 * static SwTaggedPDFHelper::IsExportTaggedPDF
 */
 bool SwTaggedPDFHelper::IsExportTaggedPDF( const OutputDevice& rOut )
 {
    vcl::PDFExtOutDevData* pPDFExtOutDevData = PTR_CAST( vcl::PDFExtOutDevData, rOut.GetExtOutDevData() );
    return pPDFExtOutDevData && pPDFExtOutDevData->GetIsExportTaggedPDF();
 }

/*
 * SwEnhancedPDFExportHelper::SwEnhancedPDFExportHelper()
 */
SwEnhancedPDFExportHelper::SwEnhancedPDFExportHelper( SwEditShell& rSh,
                                                      OutputDevice& rOut,
                                                      const rtl::OUString& rPageRange,
                                                      bool bSkipEmptyPages,
                                                      bool bEditEngineOnly )
    : mrSh( rSh ),
      mrOut( rOut ),
      pPageRange( 0 ),
      mbSkipEmptyPages( bSkipEmptyPages ),
      mbEditEngineOnly( bEditEngineOnly )
{
    if ( rPageRange.getLength() )
        pPageRange = new MultiSelection( rPageRange );

    aTableColumnsMap.clear();
    aLinkIdMap.clear();
    aNumListIdMap.clear();
    aNumListBodyIdMap.clear();
    aFrmTagIdMap.clear();

#ifndef PRODUCT
    aStructStack.clear();
#endif

    const BYTE nScript = (BYTE)GetI18NScriptTypeOfLanguage( (USHORT)GetAppLanguage() );
    USHORT nLangRes = RES_CHRATR_LANGUAGE;

    if ( i18n::ScriptType::ASIAN == nScript )
        nLangRes = RES_CHRATR_CJK_LANGUAGE;
    else if ( i18n::ScriptType::COMPLEX == nScript )
        nLangRes = RES_CHRATR_CTL_LANGUAGE;

    eLanguageDefault = static_cast<const SvxLanguageItem*>(&mrSh.GetDoc()->GetDefault( nLangRes ))->GetLanguage();

    EnhancedPDFExport();
}

SwEnhancedPDFExportHelper::~SwEnhancedPDFExportHelper()
{
    delete pPageRange;
}

/*
 * SwEnhancedPDFExportHelper::EnhancedPDFExport()
 */
void SwEnhancedPDFExportHelper::EnhancedPDFExport()
{
    vcl::PDFExtOutDevData* pPDFExtOutDevData =
        PTR_CAST( vcl::PDFExtOutDevData, mrOut.GetExtOutDevData() );

    if ( !pPDFExtOutDevData )
        return;

    //
    // set the document locale
    //
    com::sun::star::lang::Locale aDocLocale = MsLangId::convertLanguageToLocale( SwEnhancedPDFExportHelper::GetDefaultLanguage() );
    pPDFExtOutDevData->SetDocumentLocale( aDocLocale );

    //
    // Prepare the output device:
    //
    mrOut.Push( PUSH_MAPMODE );
    MapMode aMapMode( mrOut.GetMapMode() );
    aMapMode.SetMapUnit( MAP_TWIP );
    mrOut.SetMapMode( aMapMode );

    //
    // Create new cursor and lock the view:
    //
    SwDoc* pDoc = mrSh.GetDoc();
    mrSh.SwCrsrShell::Push();
    mrSh.SwCrsrShell::ClearMark();
    const BOOL bOldLockView = mrSh.IsViewLocked();
    mrSh.LockView( TRUE );

    if ( !mbEditEngineOnly )
    {
        //
        // POSTITS
        //
        if ( pPDFExtOutDevData->GetIsExportNotes() )
        {
            SwFieldType* pType = mrSh.GetFldType( RES_POSTITFLD, aEmptyStr );
            SwClientIter aIter( *pType );
            const SwClient * pFirst = aIter.GoStart();
            while( pFirst )
            {
                if( ((SwFmtFld*)pFirst)->GetTxtFld() &&
                    ((SwFmtFld*)pFirst)->IsFldInDoc())
                {
                    const SwTxtNode* pTNd =
                        (SwTxtNode*)((SwFmtFld*)pFirst)->GetTxtFld()->GetpTxtNode();
                    ASSERT( 0 != pTNd, "Enhanced pdf export - text node is missing" )

                    // 1. Check if the whole paragraph is hidden
                    // 2. Move to the field
                    // 3. Check for hidden text attribute
                    if ( !pTNd->IsHidden() &&
                          mrSh.GotoFld( *(SwFmtFld*)pFirst ) &&
                         !mrSh.SelectHiddenRange() )
                    {
                        // Link Rectangle
                        const SwRect& rNoteRect = mrSh.GetCharRect();

                        // Link PageNum
                        const sal_Int32 nNotePageNum = CalcOutputPageNum( rNoteRect );
                        if ( -1 != nNotePageNum )
		                    {
                            // Link Note
                            vcl::PDFNote aNote;

                            // Use the NumberFormatter to get the date string:
                            const SwPostItField* pField = (SwPostItField*)((SwFmtFld*)pFirst)->GetFld();
                            SvNumberFormatter* pNumFormatter = pDoc->GetNumberFormatter();
                            const Date aDateDiff( pField->GetDate() -
                                                 *pNumFormatter->GetNullDate() );
                            const ULONG nFormat =
                                pNumFormatter->GetStandardFormat( NUMBERFORMAT_DATE, pField->GetLanguage() );
                            String sDate;
                            Color* pColor;
                            pNumFormatter->GetOutputString( aDateDiff.GetDate(), nFormat, sDate, &pColor );

                            // The title should consist of the author and the date:
                            String sTitle( pField->GetPar1() );
                            sTitle.AppendAscii( RTL_CONSTASCII_STRINGPARAM( ", " ) );
                            sTitle += sDate;
                            aNote.Title = sTitle;
                            // Guess what the contents contains...
                            aNote.Contents = pField->GetTxt();

                            // Link Export
                            pPDFExtOutDevData->CreateNote( rNoteRect.SVRect(), aNote, nNotePageNum );
                        }
                    }
                }
                pFirst = aIter++;
                mrSh.SwCrsrShell::ClearMark();
            }
        }

        //
        // HYPERLINKS
        //
        SwGetINetAttrs aArr;
        const sal_uInt16 nHyperLinkCount = mrSh.GetINetAttrs( aArr );
        for( sal_uInt16 n = 0; n < nHyperLinkCount; ++n )
        {
            SwGetINetAttr* p = aArr[ n ];
            ASSERT( 0 != p, "Enhanced pdf export - SwGetINetAttr is missing" )

            const SwTxtNode* pTNd = p->rINetAttr.GetpTxtNode();
            ASSERT( 0 != pTNd, "Enhanced pdf export - text node is missing" )

            // 1. Check if the whole paragraph is hidden
            // 2. Move to the hyperlink
            // 3. Check for hidden text attribute
            if ( !pTNd->IsHidden() &&
                  mrSh.GotoINetAttr( p->rINetAttr ) &&
                 !mrSh.SelectHiddenRange() )
            {
                // Select the hyperlink:
                mrSh.SwCrsrShell::Right( 1, CRSR_SKIP_CHARS );
                if ( mrSh.SwCrsrShell::SelectTxtAttr( RES_TXTATR_INETFMT, sal_True ) )
                {
                    // First, we create the destination, because there may be more
                    // than one link to this destination:
                    String aURL( INetURLObject::decode(
                        p->rINetAttr.GetINetFmt().GetValue(),
                        INET_HEX_ESCAPE,
                        INetURLObject::DECODE_UNAMBIGUOUS,
                        RTL_TEXTENCODING_UTF8 ) );

                    // We have to distinguish between intern and real URLs
                    const bool bIntern = '#' == aURL.GetChar( 0 );

                    // _GetCrsr() is a SwShellCrsr, which is derived from
                    // SwSelPaintRects, therefore the rectangles of the current
                    // selection can be easily obtained:
                    // Note: We make a copy of the rectangles, because they may
                    // be deleted again in JumpToSwMark.
                    SwRects aTmp;
                    aTmp.Insert( mrSh.SwCrsrShell::_GetCrsr(), 0 );
                    ASSERT( aTmp.Count() > 0, "Enhanced pdf export - rectangles are missing" )

                    // Create the destination for internal links:
                    sal_Int32 nDestId = -1;
                    if ( bIntern )
                    {
                        aURL.Erase( 0, 1 );
                        mrSh.SwCrsrShell::ClearMark();
                        JumpToSwMark( &mrSh, aURL );

                        // Destination Rectangle
                        const SwRect& rDestRect = mrSh.GetCharRect();

                        // Destination PageNum
                        const sal_Int32 nDestPageNum = CalcOutputPageNum( rDestRect );

                        // Destination Export
                        if ( -1 != nDestPageNum )
                            nDestId = pPDFExtOutDevData->CreateDest( rDestRect.SVRect(), nDestPageNum );
                    }

                    if ( !bIntern || -1 != nDestId )
                    {
                        // --> FME 2005-05-09 #i44368# Links in Header/Footer
                        const SwPosition aPos( *pTNd );
                        const bool bHeaderFooter = pDoc->IsInHeaderFooter( aPos.nNode );
                        // <--

                        // Create links for all selected rectangles:
                        const USHORT nNumOfRects = aTmp.Count();
                        for ( USHORT i = 0; i < nNumOfRects; ++i )
                        {
                            // Link Rectangle
                            const SwRect& rLinkRect( aTmp[ i ] );

                            // Link PageNum
                            const sal_Int32 nLinkPageNum = CalcOutputPageNum( rLinkRect );

                            if ( -1 != nLinkPageNum )
                            {
                                // Link Export
                                const sal_Int32 nLinkId =
                                    pPDFExtOutDevData->CreateLink( rLinkRect.SVRect(), nLinkPageNum );

                                // Store link info for tagged pdf output:
                                const IdMapEntry aLinkEntry( rLinkRect, nLinkId );
                                aLinkIdMap.push_back( aLinkEntry );

                                // Connect Link and Destination:
                                if ( bIntern )
                                    pPDFExtOutDevData->SetLinkDest( nLinkId, nDestId );
                                else
                                    pPDFExtOutDevData->SetLinkURL( nLinkId, aURL );

                                // --> FME 2005-05-09 #i44368# Links in Header/Footer
                                if ( bHeaderFooter )
                                    MakeHeaderFooterLinks( *pPDFExtOutDevData, *pTNd, rLinkRect, nDestId, aURL, bIntern );
                                // <--
                            }
                        }
                    }
                }
            }
            mrSh.SwCrsrShell::ClearMark();
        }

        //
        // HYPERLINKS (Graphics, Frames, OLEs )
        //
        const SwSpzFrmFmts* pTbl = pDoc->GetSpzFrmFmts();
        const sal_uInt16 nSpzFrmFmtsCount = pTbl->Count();
        for( sal_uInt16 n = 0; n < nSpzFrmFmtsCount; ++n )
        {
            const SwFrmFmt* pFrmFmt = (*pTbl)[n];
            const SfxPoolItem* pItem;
            if ( RES_DRAWFRMFMT != pFrmFmt->Which() &&
                 SFX_ITEM_SET == pFrmFmt->GetAttrSet().GetItemState( RES_URL, TRUE, &pItem ) )
            {
                String aURL( static_cast<const SwFmtURL*>(pItem)->GetURL() );
                const bool bIntern = '#' == aURL.GetChar( 0 );

                // Create the destination for internal links:
                sal_Int32 nDestId = -1;
                if ( bIntern )
                {
                    aURL.Erase( 0, 1 );
                    mrSh.SwCrsrShell::ClearMark();
                    JumpToSwMark( &mrSh, aURL );

                    // Destination Rectangle
                    const SwRect& rDestRect = mrSh.GetCharRect();

                    // Destination PageNum
                    const sal_Int32 nDestPageNum = CalcOutputPageNum( rDestRect );

                    // Destination Export
                    if ( -1 != nDestPageNum )
                        nDestId = pPDFExtOutDevData->CreateDest( rDestRect.SVRect(), nDestPageNum );
                }

                if ( !bIntern || -1 != nDestId )
                {
                    Point aNullPt;
                    const SwRect aLinkRect = pFrmFmt->FindLayoutRect( sal_False, &aNullPt );

                    // Link PageNum
                    const sal_Int32 nLinkPageNum = CalcOutputPageNum( aLinkRect );

                    // Link Export
                    if ( -1 != nLinkPageNum )
                    {
                        const sal_Int32 nLinkId =
                            pPDFExtOutDevData->CreateLink( aLinkRect.SVRect(), nLinkPageNum );

                        // Connect Link and Destination:
                        if ( bIntern )
                            pPDFExtOutDevData->SetLinkDest( nLinkId, nDestId );
                        else
                            pPDFExtOutDevData->SetLinkURL( nLinkId, aURL );

                        // --> FME 2005-05-09 #i44368# Links in Header/Footer
                        const SwFmtAnchor &rAnch = pFrmFmt->GetAnchor();
                        if ( FLY_PAGE != rAnch.GetAnchorId() )
                        {
                            const SwPosition* pPosition = rAnch.GetCntntAnchor();
                            if ( pPosition && pDoc->IsInHeaderFooter( pPosition->nNode ) )
                            {
                                const SwTxtNode* pTNd = pPosition->nNode.GetNode().GetTxtNode();
                                if ( pTNd )
                                    MakeHeaderFooterLinks( *pPDFExtOutDevData, *pTNd, aLinkRect, nDestId, aURL, bIntern );
                            }
                        }
                        // <--
                    }
                }
            }
            mrSh.SwCrsrShell::ClearMark();
        }

        //
        // REFERENCES
        //
        SwFieldType* pType = mrSh.GetFldType( RES_GETREFFLD, aEmptyStr );
        SwClientIter aIter( *pType );
        const SwClient * pFirst = aIter.GoStart();
        while( pFirst )
        {
            if( ((SwFmtFld*)pFirst)->GetTxtFld() &&
                ((SwFmtFld*)pFirst)->IsFldInDoc())
            {
                const SwTxtNode* pTNd =
                    (SwTxtNode*)((SwFmtFld*)pFirst)->GetTxtFld()->GetpTxtNode();
               ASSERT( 0 != pTNd, "Enhanced pdf export - text node is missing" )

                // 1. Check if the whole paragraph is hidden
                // 2. Move to the field
                // 3. Check for hidden text attribute
                if ( !pTNd->IsHidden() &&
                      mrSh.GotoFld( *(SwFmtFld*)pFirst ) &&
                     !mrSh.SelectHiddenRange() )
                {
                    // Select the field:
                    mrSh.SwCrsrShell::SetMark();
                    mrSh.SwCrsrShell::Right( 1, CRSR_SKIP_CHARS );

                    // Link Rectangles
                    SwRects aTmp;
                    aTmp.Insert( mrSh.SwCrsrShell::_GetCrsr(), 0 );
                    ASSERT( aTmp.Count() > 0, "Enhanced pdf export - rectangles are missing" )

                    mrSh.SwCrsrShell::ClearMark();

                    // Destination Rectangle
                    const SwGetRefField* pField =
                        (SwGetRefField*)((SwFmtFld*)pFirst)->GetFld();
                    const String& rRefName = pField->GetSetRefName();
                    mrSh.GotoRefMark( rRefName, pField->GetSubType(), pField->GetSeqNo() );
                    const SwRect& rDestRect = mrSh.GetCharRect();

                    // Destination PageNum
                    const sal_Int32 nDestPageNum = CalcOutputPageNum( rDestRect );

                    if ( -1 != nDestPageNum )
                    {
                        // Destination Export
                        const sal_Int32 nDestId = pPDFExtOutDevData->CreateDest( rDestRect.SVRect(), nDestPageNum );

                        // --> FME 2005-05-09 #i44368# Links in Header/Footer
                        const SwPosition aPos( *pTNd );
                        const bool bHeaderFooter = pDoc->IsInHeaderFooter( aPos.nNode );
                        // <--

                        // Create links for all selected rectangles:
                        const USHORT nNumOfRects = aTmp.Count();
                        for ( USHORT i = 0; i < nNumOfRects; ++i )
                        {
                            // Link rectangle
                            const SwRect& rLinkRect( aTmp[ i ] );

                            // Link PageNum
                            const sal_Int32 nLinkPageNum = CalcOutputPageNum( rLinkRect );

                            if ( -1 != nLinkPageNum )
                            {
                                // Link Export
                                const sal_Int32 nLinkId =
                                    pPDFExtOutDevData->CreateLink( rLinkRect.SVRect(), nLinkPageNum );

                                // Store link info for tagged pdf output:
                                const IdMapEntry aLinkEntry( rLinkRect, nLinkId );
                                aLinkIdMap.push_back( aLinkEntry );

                                // Connect Link and Destination:
                                pPDFExtOutDevData->SetLinkDest( nLinkId, nDestId );

                                // --> FME 2005-05-09 #i44368# Links in Header/Footer
                                if ( bHeaderFooter )
                                {
                                    const String aDummy;
                                    MakeHeaderFooterLinks( *pPDFExtOutDevData, *pTNd, rLinkRect, nDestId, aDummy, true );
                                }
                                // <--
                            }
                        }
                    }
                }
            }
            pFirst = aIter++;
            mrSh.SwCrsrShell::ClearMark();
        }

        //
        // FOOTNOTES
        //
        const USHORT nFtnCount = pDoc->GetFtnIdxs().Count();
        for ( USHORT nIdx = 0; nIdx < nFtnCount; ++nIdx )
        {
            // Set cursor to text node that contains the footnote:
            const SwTxtFtn* pTxtFtn = pDoc->GetFtnIdxs()[ nIdx ];
            SwTxtNode& rTNd = const_cast<SwTxtNode&>(pTxtFtn->GetTxtNode());

            mrSh._GetCrsr()->GetPoint()->nNode = rTNd;
            mrSh._GetCrsr()->GetPoint()->nContent.Assign( &rTNd, *pTxtFtn->GetStart() );

            // 1. Check if the whole paragraph is hidden
            // 2. Check for hidden text attribute
            if ( static_cast<const SwTxtNode&>(rTNd).IsHidden() ||
                 mrSh.SelectHiddenRange() )
                continue;

            SwCrsrSaveState aSaveState( *mrSh._GetCrsr() );

            // Select the footnote:
            mrSh.SwCrsrShell::SetMark();
            mrSh.SwCrsrShell::Right( 1, CRSR_SKIP_CHARS );

            // Link Rectangle
            SwRects aTmp;
            aTmp.Insert( mrSh.SwCrsrShell::_GetCrsr(), 0 );
            ASSERT( aTmp.Count() > 0, "Enhanced pdf export - rectangles are missing" )
            const SwRect aLinkRect( aTmp[ 0 ] );

            mrSh._GetCrsr()->RestoreSavePos();
            mrSh.SwCrsrShell::ClearMark();

            // Goto footnote text:
            if ( mrSh.GotoFtnTxt() )
            {
                // Link PageNum
                const sal_Int32 nLinkPageNum = CalcOutputPageNum( aLinkRect );

                if ( -1 != nLinkPageNum )
                {
                    // Link Export
                    const sal_Int32 nLinkId =
                        pPDFExtOutDevData->CreateLink( aLinkRect.SVRect(), nLinkPageNum );

                    // Store link info for tagged pdf output:
                    const IdMapEntry aLinkEntry( aLinkRect, nLinkId );
                    aLinkIdMap.push_back( aLinkEntry );

                    // Destination Rectangle
                    const SwRect& rDestRect = mrSh.GetCharRect();

                    // Destination PageNum
                    const sal_Int32 nDestPageNum = CalcOutputPageNum( rDestRect );

                    if ( -1 != nDestPageNum )
                    {
                        // Destination Export
                        const sal_Int32 nDestId = pPDFExtOutDevData->CreateDest( rDestRect.SVRect(), nDestPageNum );

                        // Connect Link and Destination:
                        pPDFExtOutDevData->SetLinkDest( nLinkId, nDestId );
                    }
                }
            }
        }

        //
        // OUTLINE
        //
        if( pPDFExtOutDevData->GetIsExportBookmarks() )
        {
            typedef std::pair< sal_Int8, sal_Int32 > StackEntry;
            std::stack< StackEntry > aOutlineStack;
            aOutlineStack.push( StackEntry( -1, -1 ) ); // push default value

            const sal_uInt16 nOutlineCount =
                static_cast<sal_uInt16>(mrSh.getIDocumentOutlineNodesAccess()->getOutlineNodesCount());
            for ( sal_uInt16 i = 0; i < nOutlineCount; ++i )
            {
                // Check if outline is hidden
                const SwTxtNode* pTNd = mrSh.GetNodes().GetOutLineNds()[ i ]->GetTxtNode();
                ASSERT( 0 != pTNd, "Enhanced pdf export - text node is missing" )

                if ( pTNd->IsHidden() ||
                     // --> FME 2005-01-10 #i40292# Skip empty outlines:
                     0 == pTNd->GetTxt().Len() )
                     // <--
                    continue;

                // Get parent id from stack:
                const sal_Int8 nLevel = (sal_Int8)mrSh.getIDocumentOutlineNodesAccess()->getOutlineLevel( i );
                sal_Int8 nLevelOnTopOfStack = aOutlineStack.top().first;
                while ( nLevelOnTopOfStack >= nLevel &&
                        nLevelOnTopOfStack != -1 )
                {
                    aOutlineStack.pop();
                    nLevelOnTopOfStack = aOutlineStack.top().first;
                }
                const sal_Int32 nParent = aOutlineStack.top().second;

                // Destination rectangle
                mrSh.GotoOutline(i);
                const SwRect& rDestRect = mrSh.GetCharRect();

                // Destination PageNum
                const sal_Int32 nDestPageNum = CalcOutputPageNum( rDestRect );

                if ( -1 != nDestPageNum )
                {
                    // Destination Export
                    const sal_Int32 nDestId =
                        pPDFExtOutDevData->CreateDest( rDestRect.SVRect(), nDestPageNum );

                    // Outline entry text
                    const String& rEntry = mrSh.getIDocumentOutlineNodesAccess()->getOutlineText( i );

                    // Create a new outline item:
                    const sal_Int32 nOutlineId =
                        pPDFExtOutDevData->CreateOutlineItem( nParent, rEntry, nDestId );

                    // Push current level and nOutlineId on stack:
                    aOutlineStack.push( StackEntry( nLevel, nOutlineId ) );
                }
            }
        }

        if( pPDFExtOutDevData->GetIsExportNamedDestinations() )
        {
            //---> i56629 the iteration to convert the OOo bookmark (#bookmark)
            // into PDF named destination, see section 8.2.1 in PDF 1.4 spec
            // We need:
            // 1. a name for the destination, formed from the standard OOo bookmark name
            // 2. the destination, obtained from where the bookmark destination lies
            IDocumentMarkAccess* const pMarkAccess = mrSh.GetDoc()->getIDocumentMarkAccess();
            for(IDocumentMarkAccess::const_iterator_t ppMark = pMarkAccess->getBookmarksBegin();
                ppMark != pMarkAccess->getBookmarksEnd();
                ppMark++)
            {
                //get the name
                const ::sw::mark::IMark* pBkmk = ppMark->get();
                mrSh.SwCrsrShell::ClearMark();
                rtl::OUString sBkName = pBkmk->GetName();

                //jump to it
                JumpToSwMark( &mrSh, sBkName );

                // Destination Rectangle
                const SwRect& rDestRect = mrSh.GetCharRect();

                // Destination PageNum
                const sal_Int32 nDestPageNum = CalcOutputPageNum( rDestRect );

                // Destination Export
                if ( -1 != nDestPageNum )
                    pPDFExtOutDevData->CreateNamedDest( sBkName, rDestRect.SVRect(), nDestPageNum );
            }
            mrSh.SwCrsrShell::ClearMark();
            //<--- i56629
        }
    }
    else
    {
        //
        // LINKS FROM EDITENGINE
        //
        std::vector< vcl::PDFExtOutDevBookmarkEntry >& rBookmarks = pPDFExtOutDevData->GetBookmarks();
        std::vector< vcl::PDFExtOutDevBookmarkEntry >::const_iterator aIBeg = rBookmarks.begin();
        const std::vector< vcl::PDFExtOutDevBookmarkEntry >::const_iterator aIEnd = rBookmarks.end();
        while ( aIBeg != aIEnd )
        {
            String aBookmarkName( aIBeg->aBookmark );
            const bool bIntern = '#' == aBookmarkName.GetChar( 0 );
            if ( bIntern )
            {
                aBookmarkName.Erase( 0, 1 );
                JumpToSwMark( &mrSh, aBookmarkName );

                // Destination Rectangle
                const SwRect& rDestRect = mrSh.GetCharRect();

                // Destination PageNum
                const sal_Int32 nDestPageNum = CalcOutputPageNum( rDestRect );

                if ( -1 != nDestPageNum )
                {
                    // Destination Export
                    const sal_Int32 nDestId =
                        pPDFExtOutDevData->CreateDest( rDestRect.SVRect(), nDestPageNum );

                    // Connect Link and Destination:
                    pPDFExtOutDevData->SetLinkDest( aIBeg->nLinkId, nDestId );
                }
            }
            else
                pPDFExtOutDevData->SetLinkURL( aIBeg->nLinkId, aBookmarkName );

            aIBeg++;
        }
        rBookmarks.clear();
    }

    // Restore view, cursor, and outdev:
    mrSh.LockView( bOldLockView );
    mrSh.SwCrsrShell::Pop( FALSE );
    mrOut.Pop();
}

/*
 * SwEnhancedPDFExportHelper::CalcOutputPageNum()
 */
sal_Int32 SwEnhancedPDFExportHelper::CalcOutputPageNum( const SwRect& rRect ) const
{
    // Document page numbers are 0, 1, 2, ...
    const sal_Int32 nPageNumOfRect = mrSh.GetPageNumAndSetOffsetForPDF( mrOut, rRect );

    // Shortcut:
    if ( -1 == nPageNumOfRect || ( !pPageRange && !mbSkipEmptyPages ) )
        return nPageNumOfRect;

    // pPageRange page numbers are 1, 2, 3, ...
    if ( pPageRange && !pPageRange->IsSelected( nPageNumOfRect + 1 ) )
        return -1;

    // What will be the page number of page nPageNumOfRect in the output doc?
    sal_Int32 nOutputPageNum = -1;
    const SwRootFrm* pRootFrm = mrSh.GetLayout();
    const SwPageFrm* pCurrPage = static_cast<const SwPageFrm*>(pRootFrm->Lower());

    for ( sal_Int32 nPageIndex = 0;
          nPageIndex <= nPageNumOfRect && pCurrPage;
          ++nPageIndex )
    {
        if ( ( !pPageRange || pPageRange->IsSelected( nPageIndex + 1 ) ) &&
             ( !mbSkipEmptyPages || !pCurrPage->IsEmptyPage() ) )
            ++nOutputPageNum;

        pCurrPage = static_cast<const SwPageFrm*>(pCurrPage->GetNext());
    }

    // pdf export page numbers are 0, 1, 2, ...
    return nOutputPageNum;
}

void SwEnhancedPDFExportHelper::MakeHeaderFooterLinks( vcl::PDFExtOutDevData& rPDFExtOutDevData,
                                                       const SwTxtNode& rTNd,
                                                       const SwRect& rLinkRect,
                                                       sal_Int32 nDestId,
                                                       const String& rURL,
                                                       bool bIntern ) const
{
    // We assume, that the primary link has just been exported. Therefore
    // the offset of the link rectangle calculates as follows:
    const Point aOffset = rLinkRect.Pos() + mrOut.GetMapMode().GetOrigin();

    SwClientIter aClientIter( const_cast<SwTxtNode&>(rTNd) );
    SwClient* pLast = aClientIter.GoStart();

    while( pLast )
    {
        if ( pLast->ISA( SwTxtFrm ) )
        {
            // Add offset to current page:
            SwTxtFrm* pTmpFrm = static_cast<SwTxtFrm*>(pLast);
            const SwPageFrm* pPageFrm = pTmpFrm->FindPageFrm();
            SwRect aHFLinkRect( rLinkRect );
            aHFLinkRect.Pos() = pPageFrm->Frm().Pos() + aOffset;

            // #i97135# the gcc_x64 optimizer gets aHFLinkRect != rLinkRect wrong
            // fool it by comparing the position only (the width and height are the
            // same anyway)
            if ( aHFLinkRect.Pos() != rLinkRect.Pos() )
            {
                // Link PageNum
                const sal_Int32 nHFLinkPageNum = CalcOutputPageNum( aHFLinkRect );

                if ( -1 != nHFLinkPageNum )
                {
                    // Link Export
                    const sal_Int32 nHFLinkId =
                        rPDFExtOutDevData.CreateLink( aHFLinkRect.SVRect(), nHFLinkPageNum );

                    // Connect Link and Destination:
                    if ( bIntern )
                        rPDFExtOutDevData.SetLinkDest( nHFLinkId, nDestId );
                    else
                        rPDFExtOutDevData.SetLinkURL( nHFLinkId, rURL );
                }
            }
        }

        pLast = ++aClientIter;
    }
}

