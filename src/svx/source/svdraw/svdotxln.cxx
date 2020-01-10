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
#include "precompiled_svx.hxx"

#include <unotools/ucbstreamhelper.hxx>
#include <unotools/localfilehelper.hxx>

#include <ucbhelper/content.hxx>
#include <ucbhelper/contentbroker.hxx>
#include <unotools/datetime.hxx>

#include <svx/svdotext.hxx>
#include "svditext.hxx"
#include <svx/svdmodel.hxx>
#include <svx/editdata.hxx>

#ifndef SVX_LIGHT
#ifndef _LNKBASE_HXX //autogen
#include <sfx2/lnkbase.hxx>
#endif
#endif
#include <linkmgr.hxx>
#include <tools/urlobj.hxx>

#include <svtools/urihelper.hxx>

// #90477#
#include <tools/tenccvt.hxx>

#ifndef SVX_LIGHT
////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  @@@@  @@@@@  @@@@@@  @@@@@@ @@@@@@ @@  @@ @@@@@@  @@    @@ @@  @@ @@  @@
// @@  @@ @@  @@     @@    @@   @@      @@@@    @@    @@    @@ @@@ @@ @@ @@
// @@  @@ @@@@@      @@    @@   @@@@@    @@     @@    @@    @@ @@@@@@ @@@@
// @@  @@ @@  @@ @@  @@    @@   @@      @@@@    @@    @@    @@ @@ @@@ @@ @@
//  @@@@  @@@@@   @@@@     @@   @@@@@@ @@  @@   @@    @@@@@ @@ @@  @@ @@  @@
//
// ImpSdrObjTextLink zur Verbindung von SdrTextObj und LinkManager
//
// Einem solchen Link merke ich mir als SdrObjUserData am Objekt. Im Gegensatz
// zum Grafik-Link werden die ObjektDaten jedoch kopiert (fuer Paint, etc.).
// Die Information ob das Objekt ein Link ist besteht genau darin, dass dem
// Objekt ein entsprechender UserData-Record angehaengt ist oder nicht.
//
////////////////////////////////////////////////////////////////////////////////////////////////////

class ImpSdrObjTextLink: public ::sfx2::SvBaseLink
{
	SdrTextObj*					pSdrObj;

public:
    ImpSdrObjTextLink( SdrTextObj* pObj1 )
        : ::sfx2::SvBaseLink( ::sfx2::LINKUPDATE_ONCALL, FORMAT_FILE ),
			pSdrObj( pObj1 )
	{}
	virtual ~ImpSdrObjTextLink();

	virtual void Closed();
	virtual void DataChanged( const String& rMimeType,
								const ::com::sun::star::uno::Any & rValue );

	BOOL Connect() { return 0 != SvBaseLink::GetRealObject(); }
};

ImpSdrObjTextLink::~ImpSdrObjTextLink()
{
}

void ImpSdrObjTextLink::Closed()
{
	if (pSdrObj )
	{
		// pLink des Objekts auf NULL setzen, da die Link-Instanz ja gerade destruiert wird.
		ImpSdrObjTextLinkUserData* pData=pSdrObj->GetLinkUserData();
		if (pData!=NULL) pData->pLink=NULL;
		pSdrObj->ReleaseTextLink();
	}
	SvBaseLink::Closed();
}


void ImpSdrObjTextLink::DataChanged( const String& /*rMimeType*/,
								const ::com::sun::star::uno::Any & /*rValue */)
{
	FASTBOOL bForceReload=FALSE;
	SdrModel* pModel = pSdrObj ? pSdrObj->GetModel() : 0;
	SvxLinkManager* pLinkManager= pModel ? pModel->GetLinkManager() : 0;
	if( pLinkManager )
	{
		ImpSdrObjTextLinkUserData* pData=pSdrObj->GetLinkUserData();
		if( pData )
		{
			String aFile;
			String aFilter;
			pLinkManager->GetDisplayNames( this, 0,&aFile, 0, &aFilter );

			if( !pData->aFileName.Equals( aFile ) ||
				!pData->aFilterName.Equals( aFilter ))
			{
				pData->aFileName = aFile;
				pData->aFilterName = aFilter;
				pSdrObj->SetChanged();
				bForceReload = TRUE;
			}
		}
	}
	if (pSdrObj )
        pSdrObj->ReloadLinkedText( bForceReload );
}
#endif // SVX_LIGHT

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// @@    @@ @@  @@ @@  @@  @@  @@  @@@@@ @@@@@@ @@@@@   @@@@@   @@@@  @@@@@@  @@@@
// @@    @@ @@@ @@ @@ @@   @@  @@ @@     @@     @@  @@  @@  @@ @@  @@   @@   @@  @@
// @@    @@ @@@@@@ @@@@    @@  @@  @@@@  @@@@@  @@@@@   @@  @@ @@@@@@   @@   @@@@@@
// @@    @@ @@ @@@ @@@@@   @@  @@     @@ @@     @@  @@  @@  @@ @@  @@   @@   @@  @@
// @@@@@ @@ @@  @@ @@  @@   @@@@  @@@@@  @@@@@@ @@  @@  @@@@@  @@  @@   @@   @@  @@
//
////////////////////////////////////////////////////////////////////////////////////////////////////

TYPEINIT1(ImpSdrObjTextLinkUserData,SdrObjUserData);

ImpSdrObjTextLinkUserData::ImpSdrObjTextLinkUserData(SdrTextObj* pObj1):
	SdrObjUserData(SdrInventor,SDRUSERDATA_OBJTEXTLINK,0),
	pObj(pObj1),
	pLink(NULL),
	eCharSet(RTL_TEXTENCODING_DONTKNOW)
{
}

ImpSdrObjTextLinkUserData::~ImpSdrObjTextLinkUserData()
{
#ifndef SVX_LIGHT
	delete pLink;
#endif
}

SdrObjUserData* ImpSdrObjTextLinkUserData::Clone(SdrObject* pObj1) const
{
	ImpSdrObjTextLinkUserData* pData=new ImpSdrObjTextLinkUserData((SdrTextObj*)pObj1);
	pData->aFileName  =aFileName;
	pData->aFilterName=aFilterName;
	pData->aFileDate0 =aFileDate0;
	pData->eCharSet   =eCharSet;
	pData->pLink=NULL;
	return pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  @@@@@@ @@@@@ @@   @@ @@@@@@  @@@@  @@@@@  @@@@@@
//    @@   @@    @@@ @@@   @@   @@  @@ @@  @@     @@
//    @@   @@     @@@@@    @@   @@  @@ @@  @@     @@
//    @@   @@@@    @@@     @@   @@  @@ @@@@@      @@
//    @@   @@     @@@@@    @@   @@  @@ @@  @@     @@
//    @@   @@    @@@ @@@   @@   @@  @@ @@  @@ @@  @@
//    @@   @@@@@ @@   @@   @@    @@@@  @@@@@   @@@@
//
////////////////////////////////////////////////////////////////////////////////////////////////////

void SdrTextObj::SetTextLink(const String& rFileName, const String& rFilterName, rtl_TextEncoding eCharSet)
{
	if(eCharSet == RTL_TEXTENCODING_DONTKNOW)
		eCharSet = gsl_getSystemTextEncoding();

	ImpSdrObjTextLinkUserData* pData=GetLinkUserData();
	if (pData!=NULL) {
		ReleaseTextLink();
	}
	pData=new ImpSdrObjTextLinkUserData(this);
	pData->aFileName=rFileName;
	pData->aFilterName=rFilterName;
	pData->eCharSet=eCharSet;
	InsertUserData(pData);
	ImpLinkAnmeldung();
}

void SdrTextObj::ReleaseTextLink()
{
	ImpLinkAbmeldung();
	USHORT nAnz=GetUserDataCount();
	for (USHORT nNum=nAnz; nNum>0;) {
		nNum--;
		SdrObjUserData* pData=GetUserData(nNum);
		if (pData->GetInventor()==SdrInventor && pData->GetId()==SDRUSERDATA_OBJTEXTLINK) {
			DeleteUserData(nNum);
		}
	}
}

FASTBOOL SdrTextObj::ReloadLinkedText( FASTBOOL bForceLoad)
{
	ImpSdrObjTextLinkUserData*	pData = GetLinkUserData();
	FASTBOOL					bRet = TRUE;

	if( pData )
	{
		::ucbhelper::ContentBroker*	pBroker = ::ucbhelper::ContentBroker::get();
		DateTime				    aFileDT;
		BOOL					    bExists = FALSE, bLoad = FALSE;

		if( pBroker )
		{
			bExists = TRUE;

			try
			{
				INetURLObject aURL( pData->aFileName );
				DBG_ASSERT( aURL.GetProtocol() != INET_PROT_NOT_VALID, "invalid URL" );

				::ucbhelper::Content aCnt( aURL.GetMainURL( INetURLObject::NO_DECODE ), ::com::sun::star::uno::Reference< ::com::sun::star::ucb::XCommandEnvironment >() );
				::com::sun::star::uno::Any aAny( aCnt.getPropertyValue( ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "DateModified" ) ) ) );
				::com::sun::star::util::DateTime aDateTime;

				aAny >>= aDateTime;
				::utl::typeConvert( aDateTime, aFileDT );
			}
			catch( ... )
	        {
				bExists = FALSE;
			}
		}

		if( bExists )
		{
			if( bForceLoad )
				bLoad = TRUE;
			else
				bLoad = ( aFileDT > pData->aFileDate0 );

			if( bLoad )
			{
                bRet = LoadText( pData->aFileName, pData->aFilterName, pData->eCharSet );
			}

			pData->aFileDate0 = aFileDT;
		}
	}

	return bRet;
}

FASTBOOL SdrTextObj::LoadText(const String& rFileName, const String& /*rFilterName*/, rtl_TextEncoding eCharSet)
{
	INetURLObject	aFileURL( rFileName );
	BOOL			bRet = FALSE;

	if( aFileURL.GetProtocol() == INET_PROT_NOT_VALID )
	{
		String aFileURLStr;

		if( ::utl::LocalFileHelper::ConvertPhysicalNameToURL( rFileName, aFileURLStr ) )
			aFileURL = INetURLObject( aFileURLStr );
		else
			aFileURL.SetSmartURL( rFileName );
	}

	DBG_ASSERT( aFileURL.GetProtocol() != INET_PROT_NOT_VALID, "invalid URL" );

	SvStream* pIStm = ::utl::UcbStreamHelper::CreateStream( aFileURL.GetMainURL( INetURLObject::NO_DECODE ), STREAM_READ );

	if( pIStm )
	{
		// #90477# pIStm->SetStreamCharSet( eCharSet );
		pIStm->SetStreamCharSet(GetSOLoadTextEncoding(eCharSet, (sal_uInt16)pIStm->GetVersion()));

		char cRTF[5];
		cRTF[4] = 0;
		pIStm->Read(cRTF, 5);

		BOOL bRTF = cRTF[0] == '{' && cRTF[1] == '\\' && cRTF[2] == 'r' && cRTF[3] == 't' && cRTF[4] == 'f';

		pIStm->Seek(0);

		if( !pIStm->GetError() )
		{
            SetText( *pIStm, aFileURL.GetMainURL( INetURLObject::NO_DECODE ), sal::static_int_cast< USHORT >( bRTF ? EE_FORMAT_RTF : EE_FORMAT_TEXT ) );
			bRet = TRUE;
		}

		delete pIStm;
	}

	return bRet;
}

ImpSdrObjTextLinkUserData* SdrTextObj::GetLinkUserData() const
{
	ImpSdrObjTextLinkUserData* pData=NULL;
	USHORT nAnz=GetUserDataCount();
	for (USHORT nNum=nAnz; nNum>0 && pData==NULL;) {
		nNum--;
		pData=(ImpSdrObjTextLinkUserData*)GetUserData(nNum);
		if (pData->GetInventor()!=SdrInventor || pData->GetId()!=SDRUSERDATA_OBJTEXTLINK) {
			pData=NULL;
		}
	}
	return pData;
}

void SdrTextObj::ImpLinkAnmeldung()
{
#ifndef SVX_LIGHT
	ImpSdrObjTextLinkUserData* pData=GetLinkUserData();
	SvxLinkManager* pLinkManager=pModel!=NULL ? pModel->GetLinkManager() : NULL;
	if (pLinkManager!=NULL && pData!=NULL && pData->pLink==NULL) { // Nicht 2x Anmelden
		pData->pLink=new ImpSdrObjTextLink(this);
#ifdef GCC
		pLinkManager->InsertFileLink(*pData->pLink,OBJECT_CLIENT_FILE,pData->aFileName,
									 pData->aFilterName.Len() ?
									  &pData->aFilterName : (const String *)NULL,
									 (const String *)NULL);
#else
		pLinkManager->InsertFileLink(*pData->pLink,OBJECT_CLIENT_FILE,pData->aFileName,
									 pData->aFilterName.Len() ? &pData->aFilterName : NULL,NULL);
#endif
		pData->pLink->Connect();
	}
#endif // SVX_LIGHT
}

void SdrTextObj::ImpLinkAbmeldung()
{
#ifndef SVX_LIGHT
	ImpSdrObjTextLinkUserData* pData=GetLinkUserData();
	SvxLinkManager* pLinkManager=pModel!=NULL ? pModel->GetLinkManager() : NULL;
	if (pLinkManager!=NULL && pData!=NULL && pData->pLink!=NULL) { // Nicht 2x Abmelden
		// Bei Remove wird *pLink implizit deleted
		pLinkManager->Remove( pData->pLink );
		pData->pLink=NULL;
	}
#endif // SVX_LIGHT
}

