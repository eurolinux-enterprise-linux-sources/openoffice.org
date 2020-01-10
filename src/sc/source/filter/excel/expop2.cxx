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

#include <svtools/fltrcfg.hxx>

#include <sfx2/objsh.hxx>
#include <sfx2/docinf.hxx>
#include <svx/svxmsbas.hxx>

#include "scerrors.hxx"
#include "scextopt.hxx"

#include "root.hxx"
#include "excdoc.hxx"
#include "exp_op.hxx"

#include "xcl97esc.hxx"

#include "document.hxx"
#include "rangenam.hxx"
#include "filtopt.hxx"
#include "xltools.hxx"
#include "xelink.hxx"

#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>


ExportBiff5::ExportBiff5( XclExpRootData& rExpData, SvStream& rStrm ):
    ExportTyp( rStrm, &rExpData.mrDoc, rExpData.meTextEnc ),
    XclExpRoot( rExpData )
{
	// nur Teil der Root-Daten gebraucht
    pExcRoot = &GetOldRoot();
    pExcRoot->pER = this;   // ExcRoot -> XclExpRoot
	pExcRoot->eDateiTyp = Biff5;
    pExcDoc = new ExcDocument( *this );
}


ExportBiff5::~ExportBiff5()
{
	delete pExcDoc;
}


FltError ExportBiff5::Write()
{
    SfxObjectShell* pDocShell = GetDocShell();
    DBG_ASSERT( pDocShell, "ExportBiff5::Write - no document shell" );

    SotStorageRef xRootStrg = GetRootStorage();
    DBG_ASSERT( xRootStrg.Is(), "ExportBiff5::Write - no root storage" );

    bool bWriteBasicCode = false;
    bool bWriteBasicStrg = false;
    if( GetBiff() == EXC_BIFF8 )
	{
        if( SvtFilterOptions* pFilterOpt = SvtFilterOptions::Get() )
        {
            bWriteBasicCode = pFilterOpt->IsLoadExcelBasicCode();
            bWriteBasicStrg = pFilterOpt->IsLoadExcelBasicStorage();
        }
	}

    if( pDocShell && xRootStrg.Is() && bWriteBasicStrg )
	{
        SvxImportMSVBasic aBasicImport( *pDocShell, *xRootStrg, bWriteBasicCode, bWriteBasicStrg );
        ULONG nErr = aBasicImport.SaveOrDelMSVBAStorage( TRUE, EXC_STORAGE_VBA_PROJECT );
		if( nErr != ERRCODE_NONE )
            pDocShell->SetError( nErr, ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( OSL_LOG_PREFIX ) ) );
	}

	pExcDoc->ReadDoc();			// ScDoc -> ExcDoc
	pExcDoc->Write( aOut );		// wechstreamen

    if( pDocShell && xRootStrg.Is() )
    {
        // #i88642# update doc info (revision etc)
        pDocShell->UpdateDocInfoForSave();

        using namespace ::com::sun::star;
        uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                pDocShell->GetModel(), uno::UNO_QUERY_THROW);
        uno::Reference<document::XDocumentProperties> xDocProps
                = xDPS->getDocumentProperties();
		if ( SvtFilterOptions::Get()->IsEnableCalcPreview() )
		{
			::boost::shared_ptr<GDIMetaFile> pMetaFile =
				pDocShell->GetPreviewMetaFile (sal_False);
			uno::Sequence<sal_uInt8> metaFile(
				sfx2::convertMetaFile(pMetaFile.get()));
			sfx2::SaveOlePropertySet(xDocProps, xRootStrg, &metaFile);
		}
		else
			sfx2::SaveOlePropertySet(xDocProps, xRootStrg );
    }

    //! TODO: separate warnings for columns and sheets
    const XclExpAddressConverter& rAddrConv = GetAddressConverter();
    if( rAddrConv.IsColTruncated() || rAddrConv.IsRowTruncated() || rAddrConv.IsTabTruncated() )
		return SCWARN_EXPORT_MAXROW;

    return eERR_OK;
}



ExportBiff8::ExportBiff8( XclExpRootData& rExpData, SvStream& rStrm ) :
    ExportBiff5( rExpData, rStrm )
{
	pExcRoot->eDateiTyp = Biff8;
    pExcRoot->pEscher = new XclEscher( GetRoot(), GetDoc().GetTableCount() );
}


ExportBiff8::~ExportBiff8()
{
	delete pExcRoot->pEscher;
	pExcRoot->pEscher = NULL;
}


ExportXml2007::ExportXml2007( XclExpRootData& rExpData, SvStream& rStrm )
    : ExportTyp( rStrm, &rExpData.mrDoc, rExpData.meTextEnc )
    , XclExpRoot( rExpData )
{
    pExcRoot = &GetOldRoot();
    pExcRoot->pER = this;
    pExcRoot->eDateiTyp = Biff8;
    pExcRoot->pEscher = new XclEscher( *pExcRoot->pER, GetDoc().GetTableCount() );
    pExcDoc = new ExcDocument( *this );
}


ExportXml2007::~ExportXml2007()
{
    delete pExcRoot->pEscher;
    pExcRoot->pEscher = NULL;

    delete pExcDoc;
}


FltError ExportXml2007::Write()
{
    SfxObjectShell* pDocShell = GetDocShell();
    DBG_ASSERT( pDocShell, "ExportXml2007::Write - no document shell" );

    SotStorageRef xRootStrg = GetRootStorage();
    DBG_ASSERT( xRootStrg.Is(), "ExportXml2007::Write - no root storage" );

    bool bWriteBasicCode = false;
    bool bWriteBasicStrg = false;

    if( SvtFilterOptions* pFilterOpt = SvtFilterOptions::Get() )
    {
        bWriteBasicCode = pFilterOpt->IsLoadExcelBasicCode();
        bWriteBasicStrg = pFilterOpt->IsLoadExcelBasicStorage();
    }

    if( pDocShell && xRootStrg.Is() && bWriteBasicStrg )
    {
        SvxImportMSVBasic aBasicImport( *pDocShell, *xRootStrg, bWriteBasicCode, bWriteBasicStrg );
        ULONG nErr = aBasicImport.SaveOrDelMSVBAStorage( TRUE, EXC_STORAGE_VBA_PROJECT );
        if( nErr != ERRCODE_NONE )
            pDocShell->SetError( nErr, ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( OSL_LOG_PREFIX ) ) );
    }

    pExcDoc->ReadDoc();         // ScDoc -> ExcDoc
    pExcDoc->WriteXml( aOut );  // wechstreamen

    if( pDocShell && xRootStrg.Is() )
    {
        using namespace ::com::sun::star;
        uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
                pDocShell->GetModel(), uno::UNO_QUERY_THROW);
        uno::Reference<document::XDocumentProperties> xDocProps
                = xDPS->getDocumentProperties();
        ::boost::shared_ptr<GDIMetaFile> pMetaFile =
            pDocShell->GetPreviewMetaFile (sal_False);
        uno::Sequence<sal_uInt8> metaFile(
            sfx2::convertMetaFile(pMetaFile.get()));
        sfx2::SaveOlePropertySet(xDocProps, xRootStrg, &metaFile);
    }

    //! TODO: separate warnings for columns and sheets
    const XclExpAddressConverter& rAddrConv = GetAddressConverter();
    if( rAddrConv.IsColTruncated() || rAddrConv.IsRowTruncated() || rAddrConv.IsTabTruncated() )
        return SCWARN_EXPORT_MAXROW;

    return eERR_OK;
}


