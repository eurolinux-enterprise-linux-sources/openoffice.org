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
#include <sfx2/docfile.hxx>
#include <sfx2/sfxsids.hrc>
#include <svtools/saveopt.hxx>
#include <svtools/itemset.hxx>
#include <svtools/stritem.hxx>
#include <svtools/eitem.hxx>
#include "xecontent.hxx"
#include "xltracer.hxx"
#include "xehelper.hxx"
#include "xeformula.hxx"
#include "xelink.hxx"
#include "xename.hxx"
#include "xestyle.hxx"
#include "xepivot.hxx"
#include "xeroot.hxx"

#include "excrecds.hxx"  // for filter manager
#include "tabprotection.hxx"
#include "document.hxx"
#include "scextopt.hxx"

// Global data ================================================================

XclExpRootData::XclExpRootData( XclBiff eBiff, SfxMedium& rMedium,
        SotStorageRef xRootStrg, ScDocument& rDoc, rtl_TextEncoding eTextEnc ) :
    XclRootData( eBiff, rMedium, xRootStrg, rDoc, eTextEnc, true )
{
    SvtSaveOptions aSaveOpt;
    mbRelUrl = mrMedium.IsRemote() ? aSaveOpt.IsSaveRelINet() : aSaveOpt.IsSaveRelFSys();
}

XclExpRootData::~XclExpRootData()
{
}

// ----------------------------------------------------------------------------

XclExpRoot::XclExpRoot( XclExpRootData& rExpRootData ) :
    XclRoot( rExpRootData ),
    mrExpData( rExpRootData )
{
}

XclExpTabInfo& XclExpRoot::GetTabInfo() const
{
    DBG_ASSERT( mrExpData.mxTabInfo.is(), "XclExpRoot::GetTabInfo - missing object (wrong BIFF?)" );
    return *mrExpData.mxTabInfo;
}

XclExpAddressConverter& XclExpRoot::GetAddressConverter() const
{
    DBG_ASSERT( mrExpData.mxAddrConv.is(), "XclExpRoot::GetAddressConverter - missing object (wrong BIFF?)" );
    return *mrExpData.mxAddrConv;
}

XclExpFormulaCompiler& XclExpRoot::GetFormulaCompiler() const
{
    DBG_ASSERT( mrExpData.mxFmlaComp.is(), "XclExpRoot::GetFormulaCompiler - missing object (wrong BIFF?)" );
    return *mrExpData.mxFmlaComp;
}

XclExpProgressBar& XclExpRoot::GetProgressBar() const
{
    DBG_ASSERT( mrExpData.mxProgress.is(), "XclExpRoot::GetProgressBar - missing object (wrong BIFF?)" );
    return *mrExpData.mxProgress;
}

XclExpSst& XclExpRoot::GetSst() const
{
    DBG_ASSERT( mrExpData.mxSst.is(), "XclExpRoot::GetSst - missing object (wrong BIFF?)" );
    return *mrExpData.mxSst;
}

XclExpPalette& XclExpRoot::GetPalette() const
{
    DBG_ASSERT( mrExpData.mxPalette.is(), "XclExpRoot::GetPalette - missing object (wrong BIFF?)" );
    return *mrExpData.mxPalette;
}

XclExpFontBuffer& XclExpRoot::GetFontBuffer() const
{
    DBG_ASSERT( mrExpData.mxFontBfr.is(), "XclExpRoot::GetFontBuffer - missing object (wrong BIFF?)" );
    return *mrExpData.mxFontBfr;
}

XclExpNumFmtBuffer& XclExpRoot::GetNumFmtBuffer() const
{
    DBG_ASSERT( mrExpData.mxNumFmtBfr.is(), "XclExpRoot::GetNumFmtBuffer - missing object (wrong BIFF?)" );
    return *mrExpData.mxNumFmtBfr;
}

XclExpXFBuffer& XclExpRoot::GetXFBuffer() const
{
    DBG_ASSERT( mrExpData.mxXFBfr.is(), "XclExpRoot::GetXFBuffer - missing object (wrong BIFF?)" );
    return *mrExpData.mxXFBfr;
}

XclExpLinkManager& XclExpRoot::GetGlobalLinkManager() const
{
    DBG_ASSERT( mrExpData.mxGlobLinkMgr.is(), "XclExpRoot::GetGlobalLinkManager - missing object (wrong BIFF?)" );
    return *mrExpData.mxGlobLinkMgr;
}

XclExpLinkManager& XclExpRoot::GetLocalLinkManager() const
{
    DBG_ASSERT( GetLocalLinkMgrRef().is(), "XclExpRoot::GetLocalLinkManager - missing object (wrong BIFF?)" );
    return *GetLocalLinkMgrRef();
}

XclExpNameManager& XclExpRoot::GetNameManager() const
{
    DBG_ASSERT( mrExpData.mxNameMgr.is(), "XclExpRoot::GetNameManager - missing object (wrong BIFF?)" );
    return *mrExpData.mxNameMgr;
}

XclExpFilterManager& XclExpRoot::GetFilterManager() const
{
    DBG_ASSERT( mrExpData.mxFilterMgr.is(), "XclExpRoot::GetFilterManager - missing object (wrong BIFF?)" );
    return *mrExpData.mxFilterMgr;
}

XclExpPivotTableManager& XclExpRoot::GetPivotTableManager() const
{
    DBG_ASSERT( mrExpData.mxPTableMgr.is(), "XclExpRoot::GetPivotTableManager - missing object (wrong BIFF?)" );
    return *mrExpData.mxPTableMgr;
}

void XclExpRoot::InitializeConvert()
{
    mrExpData.mxTabInfo.reset( new XclExpTabInfo( GetRoot() ) );
    mrExpData.mxAddrConv.reset( new XclExpAddressConverter( GetRoot() ) );
    mrExpData.mxFmlaComp.reset( new XclExpFormulaCompiler( GetRoot() ) );
    mrExpData.mxProgress.reset( new XclExpProgressBar( GetRoot() ) );

    GetProgressBar().Initialize();
}

void XclExpRoot::InitializeGlobals()
{
    SetCurrScTab( SCTAB_GLOBAL );

    if( GetBiff() >= EXC_BIFF5 )
    {
        mrExpData.mxPalette.reset( new XclExpPalette( GetRoot() ) );
        mrExpData.mxFontBfr.reset( new XclExpFontBuffer( GetRoot() ) );
        mrExpData.mxNumFmtBfr.reset( new XclExpNumFmtBuffer( GetRoot() ) );
        mrExpData.mxXFBfr.reset( new XclExpXFBuffer( GetRoot() ) );
        mrExpData.mxGlobLinkMgr.reset( new XclExpLinkManager( GetRoot() ) );
        mrExpData.mxNameMgr.reset( new XclExpNameManager( GetRoot() ) );
    }

    if( GetBiff() == EXC_BIFF8 )
    {
        mrExpData.mxSst.reset( new XclExpSst );
        mrExpData.mxFilterMgr.reset( new XclExpFilterManager( GetRoot() ) );
        mrExpData.mxPTableMgr.reset( new XclExpPivotTableManager( GetRoot() ) );
        // BIFF8: only one link manager for all sheets
        mrExpData.mxLocLinkMgr = mrExpData.mxGlobLinkMgr;
    }

    GetXFBuffer().Initialize();
    GetNameManager().Initialize();
}

void XclExpRoot::InitializeTable( SCTAB nScTab )
{
    SetCurrScTab( nScTab );
    if( GetBiff() == EXC_BIFF5 )
    {
        // local link manager per sheet
        mrExpData.mxLocLinkMgr.reset( new XclExpLinkManager( GetRoot() ) );
    }
}

void XclExpRoot::InitializeSave()
{
    GetPalette().Finalize();
    GetXFBuffer().Finalize();
}

XclExpRecordRef XclExpRoot::CreateRecord( sal_uInt16 nRecId ) const
{
    XclExpRecordRef xRec;
    switch( nRecId )
    {
        case EXC_ID_PALETTE:        xRec = mrExpData.mxPalette;     break;
        case EXC_ID_FONTLIST:       xRec = mrExpData.mxFontBfr;     break;
        case EXC_ID_FORMATLIST:     xRec = mrExpData.mxNumFmtBfr;   break;
        case EXC_ID_XFLIST:         xRec = mrExpData.mxXFBfr;       break;
        case EXC_ID_SST:            xRec = mrExpData.mxSst;         break;
        case EXC_ID_EXTERNSHEET:    xRec = GetLocalLinkMgrRef();    break;
        case EXC_ID_NAME:           xRec = mrExpData.mxNameMgr;     break;
    }
    DBG_ASSERT( xRec.is(), "XclExpRoot::CreateRecord - unknown record ID or missing object" );
    return xRec;
}

bool XclExpRoot::IsDocumentEncrypted() const
{
    // We need to encrypt the content when the document structure is protected.
    const ScDocProtection* pDocProt = GetDoc().GetDocProtection();
    if (pDocProt && pDocProt->isProtected() && pDocProt->isOptionEnabled(ScDocProtection::STRUCTURE))
        return true;

    if (GetPassword().Len() > 0)
        // Password is entered directly into the save dialog.
        return true;

    return false;
}

const String XclExpRoot::GetPassword() const
{
    SfxItemSet* pSet = GetMedium().GetItemSet();
    if (!pSet)
        return String();

    const SfxPoolItem* pItem = NULL;
    if (SFX_ITEM_SET == pSet->GetItemState(SID_PASSWORD, sal_True, &pItem))
    {
        const SfxStringItem* pStrItem = dynamic_cast<const SfxStringItem*>(pItem);
        if (pStrItem)
        {
            // Password from the save dialog.
            return pStrItem->GetValue();
        }
    }

    return String();
}

XclExpRootData::XclExpLinkMgrRef XclExpRoot::GetLocalLinkMgrRef() const
{
    return IsInGlobals() ? mrExpData.mxGlobLinkMgr : mrExpData.mxLocLinkMgr;
}

// ============================================================================

