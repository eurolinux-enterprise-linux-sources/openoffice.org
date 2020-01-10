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
#include "precompiled_sfx2.hxx"
#include <tools/urlobj.hxx>
#include <vcl/msgbox.hxx>
#include <svtools/eitem.hxx>
#include <vcl/svapp.hxx>
#include <sfx2/filedlghelper.hxx>
#include <unotools/localedatawrapper.hxx>
#include <comphelper/processfactory.hxx>
#include <svtools/urihelper.hxx>
#include <svtools/useroptions.hxx>
#include <svtools/imagemgr.hxx>
#include <tools/datetime.hxx>

#include <memory>

#include <comphelper/string.hxx>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/security/DocumentSignatureInformation.hpp>
#include <com/sun/star/security/XDocumentDigitalSignatures.hpp>
#include <unotools/localedatawrapper.hxx>
#include <svtools/syslocale.hxx>
#include <rtl/math.hxx>
#include <com/sun/star/ui/dialogs/TemplateDescription.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/beans/XPropertyContainer.hpp>
#include <com/sun/star/util/Date.hpp>
#include <com/sun/star/document/XDocumentProperties.hpp>

#include <vcl/timer.hxx>
#include <sfx2/dinfdlg.hxx>
#include "sfxresid.hxx"
#include "dinfedt.hxx"
#include <sfx2/frame.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/request.hxx>
//#include "exptypes.hxx"
#include "helper.hxx"
#include <sfx2/objsh.hxx>
#include <sfx2/docfile.hxx>
#include <comphelper/storagehelper.hxx>

#include <sfx2/sfx.hrc>
#include "dinfdlg.hrc"
#include "sfxlocal.hrc"

#include <algorithm>

using namespace ::com::sun::star;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::ui::dialogs;
using namespace ::com::sun::star::uno;


struct CustomProperty
{
    ::rtl::OUString             m_sName;
    com::sun::star::uno::Any    m_aValue;

    CustomProperty( const ::rtl::OUString& sName,
            const com::sun::star::uno::Any& rValue ) :
        m_sName( sName ), m_aValue( rValue ) {}

    inline bool operator==( const CustomProperty& rProp )
    { return m_sName.equals( rProp.m_sName ) && m_aValue == rProp.m_aValue; }
};


static
bool operator==(const util::DateTime &i_rLeft, const util::DateTime &i_rRight)
{
    return i_rLeft.Year             == i_rRight.Year
        && i_rLeft.Month            == i_rRight.Month
        && i_rLeft.Day              == i_rRight.Day
        && i_rLeft.Hours            == i_rRight.Hours
        && i_rLeft.Minutes          == i_rRight.Minutes
        && i_rLeft.Seconds          == i_rRight.Seconds
        && i_rLeft.HundredthSeconds == i_rRight.HundredthSeconds;
}

// STATIC DATA -----------------------------------------------------------

TYPEINIT1_AUTOFACTORY(SfxDocumentInfoItem, SfxStringItem);

const USHORT HI_NAME = 1;
const USHORT HI_TYPE = 2;
const USHORT HI_VALUE = 3;
const USHORT HI_ACTION = 4;

//------------------------------------------------------------------------
String CreateSizeText( ULONG nSize, BOOL bExtraBytes = TRUE, BOOL bSmartExtraBytes = FALSE );
String CreateSizeText( ULONG nSize, BOOL bExtraBytes, BOOL bSmartExtraBytes )
{
	String aUnitStr = ' ';
	aUnitStr += String( SfxResId(STR_BYTES) );
	ULONG nSize1 = nSize;
	ULONG nSize2 = nSize1;
	ULONG nMega = 1024 * 1024;
	ULONG nGiga = nMega * 1024;
	double fSize = nSize;
	int nDec = 0;
	BOOL bGB = FALSE;

	if ( nSize1 >= 10000 && nSize1 < nMega )
	{
		nSize1 /= 1024;
		aUnitStr = ' ';
		aUnitStr += String( SfxResId(STR_KB) );
		fSize /= 1024;
		nDec = 0;
	}
	else if ( nSize1 >= nMega && nSize1 < nGiga )
	{
		nSize1 /= nMega;
		aUnitStr = ' ';
		aUnitStr += String( SfxResId(STR_MB) );
		fSize /= nMega;
		nDec = 2;
	}
	else if ( nSize1 >= nGiga )
	{
		nSize1 /= nGiga;
		aUnitStr = ' ';
		aUnitStr += String( SfxResId(STR_GB) );
		bGB = TRUE;
		fSize /= nGiga;
		nDec = 3;
	}
	const LocaleDataWrapper& rLocaleWrapper = SvtSysLocale().GetLocaleData();
	String aSizeStr( rLocaleWrapper.getNum( nSize1, 0 ) );
	aSizeStr += aUnitStr;
	if ( bExtraBytes && ( nSize1 < nSize2 ) )
	{
        aSizeStr = ::rtl::math::doubleToUString( fSize,
                rtl_math_StringFormat_F, nDec,
                rLocaleWrapper.getNumDecimalSep().GetChar(0) );
		aSizeStr += aUnitStr;

		aSizeStr += DEFINE_CONST_UNICODE(" (");
		aSizeStr += rLocaleWrapper.getNum( nSize2, 0 );
		aSizeStr += ' ';
		aSizeStr += String( SfxResId(STR_BYTES) );
		aSizeStr += ')';
	}
	else if ( bGB && bSmartExtraBytes )
	{
		nSize1 = nSize / nMega;
		aSizeStr = DEFINE_CONST_UNICODE(" (");
		aSizeStr += rLocaleWrapper.getNum( nSize1, 0 );
		aSizeStr += aUnitStr;
		aSizeStr += ')';
	}
	return aSizeStr;
}

String ConvertDateTime_Impl( const String& rName,
    const util::DateTime& uDT, const LocaleDataWrapper& rWrapper )
{
    Date aD(uDT.Day, uDT.Month, uDT.Year);
    Time aT(uDT.Hours, uDT.Minutes, uDT.Seconds, uDT.HundredthSeconds);
	 const String pDelim ( DEFINE_CONST_UNICODE( ", "));
	 String aStr( rWrapper.getDate( aD ) );
	 aStr += pDelim;
	 aStr += rWrapper.getTime( aT, TRUE, FALSE );
	 String aAuthor = rName;
	 aAuthor.EraseLeadingChars();
	 if ( aAuthor.Len() )
	 {
		aStr += pDelim;
		aStr += aAuthor;
	 }
	 return aStr;
}

//------------------------------------------------------------------------

SfxDocumentInfoItem::SfxDocumentInfoItem()
	: SfxStringItem()
    , m_AutoloadDelay(0)
    , m_AutoloadURL()
    , m_isAutoloadEnabled(sal_False)
    , m_DefaultTarget()
    , m_TemplateName()
    , m_Author()
    , m_CreationDate()
    , m_ModifiedBy()
    , m_ModificationDate()
    , m_PrintedBy()
    , m_PrintDate()
    , m_EditingCycles(0)
    , m_EditingDuration(0)
    , m_Description()
    , m_Keywords()
    , m_Subject()
    , m_Title()
    , m_bHasTemplate( sal_True )
    , m_bDeleteUserData( sal_False )
    , m_bUseUserData( sal_True )
{
}

//------------------------------------------------------------------------

SfxDocumentInfoItem::SfxDocumentInfoItem( const String& rFile,
        const uno::Reference<document::XDocumentProperties>& i_xDocProps,
        sal_Bool bIs )
	: SfxStringItem( SID_DOCINFO, rFile )
    , m_AutoloadDelay( i_xDocProps->getAutoloadSecs() )
    , m_AutoloadURL( i_xDocProps->getAutoloadURL() )
    , m_isAutoloadEnabled( (m_AutoloadDelay > 0) || m_AutoloadURL.getLength() )
    , m_DefaultTarget( i_xDocProps->getDefaultTarget() )
    , m_TemplateName( i_xDocProps->getTemplateName() )
    , m_Author( i_xDocProps->getAuthor() )
    , m_CreationDate( i_xDocProps->getCreationDate() )
    , m_ModifiedBy( i_xDocProps->getModifiedBy() )
    , m_ModificationDate( i_xDocProps->getModificationDate() )
    , m_PrintedBy( i_xDocProps->getPrintedBy() )
    , m_PrintDate( i_xDocProps->getPrintDate() )
    , m_EditingCycles( i_xDocProps->getEditingCycles() )
    , m_EditingDuration( i_xDocProps->getEditingDuration() )
    , m_Description( i_xDocProps->getDescription() )
    , m_Keywords( ::comphelper::string::convertCommaSeparated(
                    i_xDocProps->getKeywords()) )
    , m_Subject( i_xDocProps->getSubject() )
    , m_Title( i_xDocProps->getTitle() )
    , m_bHasTemplate( sal_True )
    , m_bDeleteUserData( sal_False )
    , m_bUseUserData( bIs )
{
    try
    {
        Reference< beans::XPropertyContainer > xContainer = i_xDocProps->getUserDefinedProperties();
        if ( xContainer.is() )
        {
            Reference < beans::XPropertySet > xSet( xContainer, UNO_QUERY );
            const Sequence< beans::Property > lProps = xSet->getPropertySetInfo()->getProperties();
            const beans::Property* pProps = lProps.getConstArray();
            sal_Int32 nCount = lProps.getLength();
            for ( sal_Int32 i = 0; i < nCount; ++i )
            {
                // "fix" property? => not a custom property => ignore it!
                if ( !(pProps[i].Attributes & ::com::sun::star::beans::PropertyAttribute::REMOVABLE) )
                    continue;

                uno::Any aValue = xSet->getPropertyValue(pProps[i].Name);
                CustomProperty* pProp = new CustomProperty( pProps[i].Name, aValue );
                m_aCustomProperties.push_back( pProp );
            }
        }
    }
    catch ( Exception& ) {}
}

//------------------------------------------------------------------------

SfxDocumentInfoItem::SfxDocumentInfoItem( const SfxDocumentInfoItem& rItem )
	: SfxStringItem( rItem )
    , m_AutoloadDelay( rItem.getAutoloadDelay() )
    , m_AutoloadURL( rItem.getAutoloadURL() )
    , m_isAutoloadEnabled( rItem.isAutoloadEnabled() )
    , m_DefaultTarget( rItem.getDefaultTarget() )
    , m_TemplateName( rItem.getTemplateName() )
    , m_Author( rItem.getAuthor() )
    , m_CreationDate( rItem.getCreationDate() )
    , m_ModifiedBy( rItem.getModifiedBy() )
    , m_ModificationDate( rItem.getModificationDate() )
    , m_PrintedBy( rItem.getPrintedBy() )
    , m_PrintDate( rItem.getPrintDate() )
    , m_EditingCycles( rItem.getEditingCycles() )
    , m_EditingDuration( rItem.getEditingDuration() )
    , m_Description( rItem.getDescription() )
    , m_Keywords( rItem.getKeywords() )
    , m_Subject( rItem.getSubject() )
    , m_Title( rItem.getTitle() )
    , m_bHasTemplate( rItem.m_bHasTemplate )
    , m_bDeleteUserData( rItem.m_bDeleteUserData )
    , m_bUseUserData( rItem.m_bUseUserData )
{
    for ( sal_uInt32 i = 0; i < rItem.m_aCustomProperties.size(); i++ )
    {
        CustomProperty* pProp = new CustomProperty( rItem.m_aCustomProperties[i]->m_sName,
                                                    rItem.m_aCustomProperties[i]->m_aValue );
        m_aCustomProperties.push_back( pProp );
    }
}

//------------------------------------------------------------------------

SfxDocumentInfoItem::~SfxDocumentInfoItem()
{
    ClearCustomProperties();
}

//------------------------------------------------------------------------

SfxPoolItem* SfxDocumentInfoItem::Clone( SfxItemPool * ) const
{
	return new SfxDocumentInfoItem( *this );
}

//------------------------------------------------------------------------

int SfxDocumentInfoItem::operator==( const SfxPoolItem& rItem) const
{
	if (!(rItem.Type() == Type() && SfxStringItem::operator==(rItem))) {
        return false;
    }
    const SfxDocumentInfoItem& rInfoItem(
        static_cast<const SfxDocumentInfoItem&>(rItem));

    return
         m_AutoloadDelay        == rInfoItem.m_AutoloadDelay     &&
         m_AutoloadURL          == rInfoItem.m_AutoloadURL       &&
         m_isAutoloadEnabled    == rInfoItem.m_isAutoloadEnabled &&
         m_DefaultTarget        == rInfoItem.m_DefaultTarget     &&
         m_Author               == rInfoItem.m_Author            &&
         m_CreationDate         == rInfoItem.m_CreationDate      &&
         m_ModifiedBy           == rInfoItem.m_ModifiedBy        &&
         m_ModificationDate     == rInfoItem.m_ModificationDate  &&
         m_PrintedBy            == rInfoItem.m_PrintedBy         &&
         m_PrintDate            == rInfoItem.m_PrintDate         &&
         m_EditingCycles        == rInfoItem.m_EditingCycles     &&
         m_EditingDuration      == rInfoItem.m_EditingDuration   &&
         m_Description          == rInfoItem.m_Description       &&
         m_Keywords             == rInfoItem.m_Keywords          &&
         m_Subject              == rInfoItem.m_Subject           &&
         m_Title                == rInfoItem.m_Title             &&
         m_aCustomProperties.size() == rInfoItem.m_aCustomProperties.size() &&
         std::equal(m_aCustomProperties.begin(), m_aCustomProperties.end(),
            rInfoItem.m_aCustomProperties.begin());
}

//------------------------------------------------------------------------

void SfxDocumentInfoItem::resetUserData(const ::rtl::OUString & i_rAuthor)
{
    setAuthor(i_rAuthor);
    DateTime now;
    setCreationDate( util::DateTime(
        now.Get100Sec(), now.GetSec(), now.GetMin(), now.GetHour(),
        now.GetDay(), now.GetMonth(), now.GetYear() ) );
    setModifiedBy(::rtl::OUString());
    setPrintedBy(::rtl::OUString());
    setModificationDate(util::DateTime());
    setPrintDate(util::DateTime());
    setEditingDuration(0);
    setEditingCycles(1);
}

//------------------------------------------------------------------------

void SfxDocumentInfoItem::UpdateDocumentInfo(
        const uno::Reference<document::XDocumentProperties>& i_xDocProps,
        bool i_bDoNotUpdateUserDefined) const
{
    if (isAutoloadEnabled()) {
        i_xDocProps->setAutoloadSecs(getAutoloadDelay());
        i_xDocProps->setAutoloadURL(getAutoloadURL());
    } else {
        i_xDocProps->setAutoloadSecs(0);
        i_xDocProps->setAutoloadURL(::rtl::OUString());
    }
    i_xDocProps->setDefaultTarget(getDefaultTarget());
//    i_xDocProps->setTemplateName(getTemplateName());
    i_xDocProps->setAuthor(getAuthor());
    i_xDocProps->setCreationDate(getCreationDate());
    i_xDocProps->setModifiedBy(getModifiedBy());
    i_xDocProps->setModificationDate(getModificationDate());
    i_xDocProps->setPrintedBy(getPrintedBy());
    i_xDocProps->setPrintDate(getPrintDate());
    i_xDocProps->setEditingCycles(getEditingCycles());
    i_xDocProps->setEditingDuration(getEditingDuration());
    i_xDocProps->setDescription(getDescription());
    i_xDocProps->setKeywords(
        ::comphelper::string::convertCommaSeparated(getKeywords()));
    i_xDocProps->setSubject(getSubject());
    i_xDocProps->setTitle(getTitle());

    // this is necessary in case of replaying a recorded macro:
    // in this case, the macro may contain the 4 old user-defined DocumentInfo
    // fields, but not any of the DocumentInfo properties;
    // as a consequence, most of the UserDefined properties of the
    // DocumentProperties would be summarily deleted here, which does not
    // seem like a good idea.
    if (i_bDoNotUpdateUserDefined)
        return;

    try
    {
        Reference< beans::XPropertyContainer > xContainer = i_xDocProps->getUserDefinedProperties();
        Reference < beans::XPropertySet > xSet( xContainer, UNO_QUERY );
        Reference< beans::XPropertySetInfo > xSetInfo = xSet->getPropertySetInfo();
        const Sequence< beans::Property > lProps = xSetInfo->getProperties();
        const beans::Property* pProps = lProps.getConstArray();
        sal_Int32 nCount = lProps.getLength();
        for ( sal_Int32 j = 0; j < nCount; ++j )
            xContainer->removeProperty( pProps[j].Name );

        for ( sal_uInt32 k = 0; k < m_aCustomProperties.size(); ++k )
        {
            try
            {
                xContainer->addProperty( m_aCustomProperties[k]->m_sName,
                    beans::PropertyAttribute::REMOVABLE, m_aCustomProperties[k]->m_aValue );
            }
            catch ( Exception& )
            {
                DBG_ERRORFILE( "SfxDocumentInfoItem::updateDocumentInfo(): exception while adding custom properties" );
            }
        }
    }
    catch ( Exception& )
    {
        DBG_ERRORFILE( "SfxDocumentInfoItem::updateDocumentInfo(): exception while removing custom properties" );
    }
}

//------------------------------------------------------------------------

sal_Bool SfxDocumentInfoItem::IsDeleteUserData() const
{
    return m_bDeleteUserData;
}

void SfxDocumentInfoItem::SetDeleteUserData( sal_Bool bSet )
{
    m_bDeleteUserData = bSet;
}

sal_Bool SfxDocumentInfoItem::IsUseUserData() const
{
    return m_bUseUserData;
}

void SfxDocumentInfoItem::SetUseUserData( sal_Bool bSet )
{
    m_bUseUserData = bSet;
}

std::vector< CustomProperty* > SfxDocumentInfoItem::GetCustomProperties() const
{
    std::vector< CustomProperty* > aRet;
    for ( sal_uInt32 i = 0; i < m_aCustomProperties.size(); i++ )
    {
        CustomProperty* pProp = new CustomProperty( m_aCustomProperties[i]->m_sName,
                                                    m_aCustomProperties[i]->m_aValue );
        aRet.push_back( pProp );
    }

    return aRet;
}

void SfxDocumentInfoItem::ClearCustomProperties()
{
    for ( sal_uInt32 i = 0; i < m_aCustomProperties.size(); i++ )
        delete m_aCustomProperties[i];
    m_aCustomProperties.clear();
}

void SfxDocumentInfoItem::AddCustomProperty( const ::rtl::OUString& sName, const Any& rValue )
{
    CustomProperty* pProp = new CustomProperty( sName, rValue );
    m_aCustomProperties.push_back( pProp );
}

sal_Bool SfxDocumentInfoItem::QueryValue( Any& rVal, BYTE nMemberId ) const
{
    String aValue;
    sal_Int32 nValue = 0;
	sal_Bool bValue = sal_False;
    BOOL bIsInt = FALSE;
    BOOL bIsString = FALSE;
    nMemberId &= ~CONVERT_TWIPS;
    switch ( nMemberId )
    {
		case MID_DOCINFO_USEUSERDATA:
			bValue = IsUseUserData();
			break;
		case MID_DOCINFO_DELETEUSERDATA:
			bValue = IsDeleteUserData();
			break;
		case MID_DOCINFO_AUTOLOADENABLED:
			bValue = isAutoloadEnabled();
			break;
		case MID_DOCINFO_AUTOLOADSECS:
			bIsInt = TRUE;
			nValue = getAutoloadDelay();
			break;
		case MID_DOCINFO_AUTOLOADURL:
			bIsString = TRUE;
			aValue = getAutoloadURL();
			break;
		case MID_DOCINFO_DEFAULTTARGET:
			bIsString = TRUE;
			aValue = getDefaultTarget();
			break;
        case MID_DOCINFO_DESCRIPTION:
			bIsString = TRUE;
            aValue = getDescription();
            break;
        case MID_DOCINFO_KEYWORDS:
			bIsString = TRUE;
            aValue = getKeywords();
            break;
        case MID_DOCINFO_SUBJECT:
			bIsString = TRUE;
            aValue = getSubject();
            break;
        case MID_DOCINFO_TITLE:
			bIsString = TRUE;
            aValue = getTitle();
            break;
        default:
			DBG_ERROR("Wrong MemberId!");
            return sal_False;
	 }

	if ( bIsString )
		rVal <<= ::rtl::OUString( aValue );
	else if ( bIsInt )
		rVal <<= nValue;
	else
		rVal <<= bValue;
    return sal_True;
}

sal_Bool SfxDocumentInfoItem::PutValue( const Any& rVal, BYTE nMemberId )
{
    ::rtl::OUString aValue;
    sal_Int32 nValue=0;
	sal_Bool bValue = sal_False;
	sal_Bool bRet = sal_False;
    nMemberId &= ~CONVERT_TWIPS;
    switch ( nMemberId )
    {
		case MID_DOCINFO_USEUSERDATA:
			bRet = (rVal >>= bValue);
			if ( bRet )
				SetUseUserData( bValue );
			break;
		case MID_DOCINFO_DELETEUSERDATA:
			// QUESTION: deleting user data was done here; seems to be superfluous!
			bRet = (rVal >>= bValue);
			if ( bRet )
				SetDeleteUserData( bValue );
			break;
		case MID_DOCINFO_AUTOLOADENABLED:
			bRet = (rVal >>= bValue);
			if ( bRet )
				setAutoloadEnabled(bValue);
			break;
		case MID_DOCINFO_AUTOLOADSECS:
			bRet = (rVal >>= nValue);
			if ( bRet )
				setAutoloadDelay(nValue);
			break;
		case MID_DOCINFO_AUTOLOADURL:
			bRet = (rVal >>= aValue);
			if ( bRet )
				setAutoloadURL(aValue);
			break;
		case MID_DOCINFO_DEFAULTTARGET:
			bRet = (rVal >>= aValue);
			if ( bRet )
				setDefaultTarget(aValue);
			break;
        case MID_DOCINFO_DESCRIPTION:
			bRet = (rVal >>= aValue);
			if ( bRet )
                setDescription(aValue);
            break;
        case MID_DOCINFO_KEYWORDS:
			bRet = (rVal >>= aValue);
			if ( bRet )
                setKeywords(aValue);
            break;
        case MID_DOCINFO_SUBJECT:
			bRet = (rVal >>= aValue);
			if ( bRet )
                setSubject(aValue);
            break;
        case MID_DOCINFO_TITLE:
			bRet = (rVal >>= aValue);
			if ( bRet )
                setTitle(aValue);
            break;
        default:
			DBG_ERROR("Wrong MemberId!");
            return sal_False;
    }

    return bRet;
}

//------------------------------------------------------------------------

SfxDocumentDescPage::SfxDocumentDescPage( Window * pParent, const SfxItemSet& rItemSet )  :

    SfxTabPage( pParent, SfxResId( TP_DOCINFODESC ), rItemSet ),

    aTitleFt    ( this, SfxResId( FT_TITLE ) ),
    aTitleEd    ( this, SfxResId( ED_TITLE ) ),
    aThemaFt    ( this, SfxResId( FT_THEMA ) ),
    aThemaEd    ( this, SfxResId( ED_THEMA ) ),
    aKeywordsFt ( this, SfxResId( FT_KEYWORDS ) ),
    aKeywordsEd ( this, SfxResId( ED_KEYWORDS ) ),
    aCommentFt  ( this, SfxResId( FT_COMMENT ) ),
    aCommentEd  ( this, SfxResId( ED_COMMENT ) ),

    pInfoItem   ( NULL )

{
    FreeResource();
}

//------------------------------------------------------------------------

SfxTabPage *SfxDocumentDescPage::Create(Window *pParent, const SfxItemSet &rItemSet)
{
	 return new SfxDocumentDescPage(pParent, rItemSet);
}

//------------------------------------------------------------------------

BOOL SfxDocumentDescPage::FillItemSet(SfxItemSet &rSet)
{
	// Pruefung, ob eine Aenderung vorliegt
	const BOOL bTitleMod = aTitleEd.IsModified();
	const BOOL bThemeMod = aThemaEd.IsModified();
	const BOOL bKeywordsMod = aKeywordsEd.IsModified();
	const BOOL bCommentMod = aCommentEd.IsModified();
	if( !( bTitleMod || bThemeMod || bKeywordsMod || bCommentMod ) )
	{
		return FALSE;
	}

	// Erzeugung der Ausgabedaten
    const SfxPoolItem* pItem = NULL;
    SfxDocumentInfoItem* pInfo = NULL;
	SfxTabDialog* pDlg = GetTabDialog();
	const SfxItemSet* pExSet = NULL;

	if ( pDlg )
		pExSet = pDlg->GetExampleSet();

	if ( pExSet && SFX_ITEM_SET != pExSet->GetItemState( SID_DOCINFO, TRUE, &pItem ) )
		pInfo = pInfoItem;
    else if ( pItem )
        pInfo = new SfxDocumentInfoItem( *(const SfxDocumentInfoItem *)pItem );

    if ( !pInfo )
    {
        DBG_ERRORFILE( "SfxDocumentDescPage::FillItemSet(): no item found" );
        return FALSE;
    }

    if( bTitleMod )
	{
		pInfo->setTitle( aTitleEd.GetText() );
	}
	if( bThemeMod )
	{
		pInfo->setSubject( aThemaEd.GetText() );
	}
	if( bKeywordsMod )
	{
		pInfo->setKeywords( aKeywordsEd.GetText() );
	}
	if( bCommentMod )
	{
		pInfo->setDescription( aCommentEd.GetText() );
	}
	rSet.Put( SfxDocumentInfoItem( *pInfo ) );
	if( pInfo != pInfoItem )
	{
		delete pInfo;
	}

	return TRUE;
}

//------------------------------------------------------------------------

void SfxDocumentDescPage::Reset(const SfxItemSet &rSet)
{
	 pInfoItem = &(SfxDocumentInfoItem &)rSet.Get(SID_DOCINFO);

	 aTitleEd.SetText( pInfoItem->getTitle() );
	 aThemaEd.SetText( pInfoItem->getSubject() );
	 aKeywordsEd.SetText( pInfoItem->getKeywords() );
	 aCommentEd.SetText( pInfoItem->getDescription() );

     SFX_ITEMSET_ARG( &rSet, pROItem, SfxBoolItem, SID_DOC_READONLY, FALSE );
	 if ( pROItem && pROItem->GetValue() )
	 {
		aTitleEd.SetReadOnly( TRUE );
		aThemaEd.SetReadOnly( TRUE );
		aKeywordsEd.SetReadOnly( TRUE );
		aCommentEd.SetReadOnly( TRUE );
	}
}

//------------------------------------------------------------------------

namespace
{
	String GetDateTimeString( sal_Int32 _nDate, sal_Int32 _nTime )
	{
		LocaleDataWrapper aWrapper( ::comphelper::getProcessServiceFactory(), Application::GetSettings().GetLocale() );

		Date aDate( _nDate );
		Time aTime( _nTime );
		String aStr( aWrapper.getDate( aDate ) );
		aStr.AppendAscii( ", " );
		aStr += aWrapper.getTime( aTime );
		return aStr;
	}

	// copy from xmlsecurity/source/dialog/resourcemanager.cxx
	String GetContentPart( const String& _rRawString, const String& _rPartId )
	{
		String		s;

		xub_StrLen	nContStart = _rRawString.Search( _rPartId );
		if( nContStart != STRING_NOTFOUND )
		{
			nContStart = nContStart + _rPartId.Len();
			++nContStart;					// now it's start of content, directly after Id

			xub_StrLen	nContEnd = _rRawString.Search( sal_Unicode( ',' ), nContStart );

			s = String( _rRawString, nContStart, nContEnd - nContStart );
		}

		return s;
	}

}

SfxDocumentPage::SfxDocumentPage( Window* pParent, const SfxItemSet& rItemSet ) :

	SfxTabPage( pParent, SfxResId( TP_DOCINFODOC ), rItemSet ),

    aBmp1           ( this, SfxResId( BMP_FILE_1 ) ),
    aNameED         ( this, SfxResId( ED_FILE_NAME ) ),

    aLine1FL        ( this, SfxResId( FL_FILE_1 ) ),
    aTypeFT         ( this, SfxResId( FT_FILE_TYP ) ),
    aShowTypeFT     ( this, SfxResId( FT_FILE_SHOW_TYP ) ),
    aReadOnlyCB     ( this, SfxResId( CB_FILE_READONLY ) ),
    aFileFt         ( this, SfxResId( FT_FILE ) ),
    aFileValFt      ( this, SfxResId( FT_FILE_VAL ) ),
    aSizeFT         ( this, SfxResId( FT_FILE_SIZE ) ),
    aShowSizeFT     ( this, SfxResId( FT_FILE_SHOW_SIZE ) ),

    aLine2FL        ( this, SfxResId( FL_FILE_2 ) ),
    aCreateFt       ( this, SfxResId( FT_CREATE ) ),
    aCreateValFt    ( this, SfxResId( FT_CREATE_VAL ) ),
    aChangeFt       ( this, SfxResId( FT_CHANGE ) ),
    aChangeValFt    ( this, SfxResId( FT_CHANGE_VAL ) ),
    aSignedFt       ( this, SfxResId( FT_SIGNED ) ),
    aSignedValFt    ( this, SfxResId( FT_SIGNED_VAL ) ),
    aSignatureBtn   ( this, SfxResId( BTN_SIGNATURE ) ),
    aPrintFt        ( this, SfxResId( FT_PRINT ) ),
    aPrintValFt     ( this, SfxResId( FT_PRINT_VAL ) ),
    aTimeLogFt      ( this, SfxResId( FT_TIMELOG ) ),
    aTimeLogValFt   ( this, SfxResId( FT_TIMELOG_VAL ) ),
    aDocNoFt        ( this, SfxResId( FT_DOCNO ) ),
    aDocNoValFt     ( this, SfxResId( FT_DOCNO_VAL ) ),
    aUseUserDataCB  ( this, SfxResId( CB_USE_USERDATA ) ),
    aDeleteBtn      ( this, SfxResId( BTN_DELETE ) ),

    aLine3FL        ( this, SfxResId( FL_FILE_3 ) ),
    aTemplFt        ( this, SfxResId( FT_TEMPL ) ),
    aTemplValFt     ( this, SfxResId( FT_TEMPL_VAL ) ),

	aUnknownSize	( SfxResId( STR_UNKNOWNSIZE ) ),
	aMultiSignedStr	( SfxResId( STR_MULTSIGNED ) ),

	bEnableUseUserData	( FALSE ),
	bHandleDelete		( FALSE )

{
	FreeResource();

    ImplUpdateSignatures();
	aDeleteBtn.SetClickHdl( LINK( this, SfxDocumentPage, DeleteHdl ) );
	aSignatureBtn.SetClickHdl( LINK( this, SfxDocumentPage, SignatureHdl ) );

    // if the button text is too wide, then broaden it
    const long nOffset = 12;
    String sText = aSignatureBtn.GetText();
    long nTxtW = aSignatureBtn.GetTextWidth( sText );
    if ( sText.Search( '~' ) == STRING_NOTFOUND )
        nTxtW += nOffset;
    long nBtnW = aSignatureBtn.GetSizePixel().Width();
    if ( nTxtW >= nBtnW )
    {
        long nDelta = Max( nTxtW - nBtnW, nOffset/3 );
        Size aNewSize = aSignatureBtn.GetSizePixel();
        aNewSize.Width() += nDelta;
        aSignatureBtn.SetSizePixel( aNewSize );
        aDeleteBtn.SetSizePixel( aNewSize );
        // and give them a new position
        Point aNewPos = aSignatureBtn.GetPosPixel();
        aNewPos.X() -= nDelta;
        aSignatureBtn.SetPosPixel( aNewPos );
        aNewPos = aDeleteBtn.GetPosPixel();
        aNewPos.X() -= nDelta;
        aDeleteBtn.SetPosPixel( aNewPos );

        aNewSize = aSignedValFt.GetSizePixel();
        aNewSize.Width() -= nDelta;
        aSignedValFt.SetSizePixel( aNewSize );
        aNewSize = aUseUserDataCB.GetSizePixel();
        aNewSize.Width() -= nDelta;
        aUseUserDataCB.SetSizePixel( aNewSize );
    }
}

//------------------------------------------------------------------------

IMPL_LINK( SfxDocumentPage, DeleteHdl, PushButton*, EMPTYARG )
{
	String aName;
	if ( bEnableUseUserData && aUseUserDataCB.IsChecked() )
    	aName = SvtUserOptions().GetFullName();
	LocaleDataWrapper aLocaleWrapper( ::comphelper::getProcessServiceFactory(), Application::GetSettings().GetLocale() );
    DateTime now;
    util::DateTime uDT(
        now.Get100Sec(), now.GetSec(), now.GetMin(), now.GetHour(),
        now.GetDay(), now.GetMonth(), now.GetYear() );
	aCreateValFt.SetText( ConvertDateTime_Impl( aName, uDT, aLocaleWrapper ) );
	XubString aEmpty;
	aChangeValFt.SetText( aEmpty );
	aPrintValFt.SetText( aEmpty );
	const Time aTime( 0 );
	aTimeLogValFt.SetText( aLocaleWrapper.getDuration( aTime ) );
	aDocNoValFt.SetText( '1' );
	bHandleDelete = TRUE;
	return 0;
}

IMPL_LINK( SfxDocumentPage, SignatureHdl, PushButton*, EMPTYARG )
{
	SfxObjectShell*	pDoc = SfxObjectShell::Current();
	if( pDoc )
	{
        pDoc->SignDocumentContent();

        ImplUpdateSignatures();
    }

    return 0;
}

void SfxDocumentPage::ImplUpdateSignatures()
{
	SfxObjectShell*	pDoc = SfxObjectShell::Current();
	if( pDoc )
	{
		SfxMedium* pMedium = pDoc->GetMedium();
        if ( pMedium && pMedium->GetName().Len() && pMedium->GetStorage().is() )
		{
			Reference< security::XDocumentDigitalSignatures > xD(
				comphelper::getProcessServiceFactory()->createInstance( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM ( "com.sun.star.security.DocumentDigitalSignatures" ) ) ), uno::UNO_QUERY );

			if( xD.is() )
			{
				String s;
				Sequence< security::DocumentSignatureInformation > aInfos;
				aInfos = xD->verifyDocumentContentSignatures( pMedium->GetZipStorageToSign_Impl(),
																uno::Reference< io::XInputStream >() );
				if( aInfos.getLength() > 1 )
				{
					s = aMultiSignedStr;
				}
				else if( aInfos.getLength() == 1 )
				{
					String aCN_Id( String::CreateFromAscii( "CN" ) );
					const security::DocumentSignatureInformation& rInfo = aInfos[ 0 ];
					s = GetDateTimeString( rInfo.SignatureDate, rInfo.SignatureTime );
					s.AppendAscii( ", " );
					s += GetContentPart( rInfo.Signer->getSubjectName(), aCN_Id );
				}
				aSignedValFt.SetText( s );
			}
		}
	}
}

//------------------------------------------------------------------------

SfxTabPage* SfxDocumentPage::Create( Window* pParent, const SfxItemSet& rItemSet )
{
	 return new SfxDocumentPage( pParent, rItemSet );
}

//------------------------------------------------------------------------

void SfxDocumentPage::EnableUseUserData()
{
	bEnableUseUserData = TRUE;
	aUseUserDataCB.Show();
	aDeleteBtn.Show();
}

//------------------------------------------------------------------------

BOOL SfxDocumentPage::FillItemSet( SfxItemSet& rSet )
{
	BOOL bRet = FALSE;

	if ( !bHandleDelete && bEnableUseUserData &&
		 aUseUserDataCB.GetState() != aUseUserDataCB.GetSavedValue() &&
		 GetTabDialog() && GetTabDialog()->GetExampleSet() )
	{
		const SfxItemSet* pExpSet = GetTabDialog()->GetExampleSet();
		const SfxPoolItem* pItem;

		if ( pExpSet && SFX_ITEM_SET == pExpSet->GetItemState( SID_DOCINFO, TRUE, &pItem ) )
		{
			SfxDocumentInfoItem* pInfoItem = (SfxDocumentInfoItem*)pItem;
			BOOL bUseData = ( STATE_CHECK == aUseUserDataCB.GetState() );
			pInfoItem->SetUseUserData( bUseData );
/*
			if ( !bUseData )
			{
				// "Benutzerdaten verwenden" ausgeschaltet ->
				// den Benutzer aus den Stamps l"oschen
				String aEmptyUser;
				aInfo.SetCreated(
					SfxStamp( aEmptyUser, aInfo.GetCreated().GetTime() ) );
				aInfo.SetChanged(
					SfxStamp( aEmptyUser, aInfo.GetChanged().GetTime() ) );
				aInfo.SetPrinted(
					SfxStamp( aEmptyUser, aInfo.GetPrinted().GetTime() ) );
			}
*/
			rSet.Put( SfxDocumentInfoItem( *pInfoItem ) );
			bRet = TRUE;
		}
	}

	if ( bHandleDelete )
	{
		const SfxItemSet* pExpSet = GetTabDialog()->GetExampleSet();
		const SfxPoolItem* pItem;
		if ( pExpSet && SFX_ITEM_SET == pExpSet->GetItemState( SID_DOCINFO, TRUE, &pItem ) )
		{
			SfxDocumentInfoItem* pInfoItem = (SfxDocumentInfoItem*)pItem;
			BOOL bUseAuthor = bEnableUseUserData && aUseUserDataCB.IsChecked();
			SfxDocumentInfoItem newItem( *pInfoItem );
            newItem.resetUserData( bUseAuthor
                ? SvtUserOptions().GetFullName()
                : ::rtl::OUString() );
			pInfoItem->SetUseUserData( STATE_CHECK == aUseUserDataCB.GetState() );
			newItem.SetUseUserData( STATE_CHECK == aUseUserDataCB.GetState() );

			newItem.SetDeleteUserData( TRUE );
			rSet.Put( newItem );
			bRet = TRUE;
		}
	}

	if ( aNameED.IsModified() && aNameED.GetText().Len() )
	{
		rSet.Put( SfxStringItem( ID_FILETP_TITLE, aNameED.GetText() ) );
		bRet = TRUE;
	}

	if ( /* aReadOnlyCB.IsModified() */ TRUE )
	{
		rSet.Put( SfxBoolItem( ID_FILETP_READONLY, aReadOnlyCB.IsChecked() ) );
		bRet = TRUE;
	}

	return bRet;
}

//------------------------------------------------------------------------

void SfxDocumentPage::Reset( const SfxItemSet& rSet )
{
	// Bestimmung der Dokumentinformationen
	const SfxDocumentInfoItem *pInfoItem =
		&(const SfxDocumentInfoItem &)rSet.Get(SID_DOCINFO);

	// template data
	if ( pInfoItem->HasTemplate() )
	{
		aTemplValFt.SetText( pInfoItem->getTemplateName() );
	}
	else
	{
		aTemplFt.Hide();
		aTemplValFt.Hide();
	}

	// determine file name
	String aFile( pInfoItem->GetValue() );
	String aFactory( aFile );
	if ( aFile.Len() > 2 && aFile.GetChar(0) == '[' )
	{
		USHORT nPos = aFile.Search( ']' );
		aFactory = aFile.Copy( 1, nPos-1  );
		aFile = aFile.Copy( nPos+1 );
	}

	// determine name
	String aName;
	const SfxPoolItem* pItem = 0;
	if ( SFX_ITEM_SET != rSet.GetItemState( ID_FILETP_TITLE, FALSE, &pItem ) )
	{
		INetURLObject aURL(aFile);
		aName = aURL.GetName( INetURLObject::DECODE_WITH_CHARSET );
		if ( !aName.Len() || aURL.GetProtocol() == INET_PROT_PRIVATE )
			aName = String( SfxResId( STR_NONAME ) );
		aNameED.SetReadOnly( TRUE );
	}
	else
	{
		DBG_ASSERT( pItem->IsA( TYPE( SfxStringItem ) ), "SfxDocumentPage:<SfxStringItem> erwartet" );
		aName = ( ( SfxStringItem* ) pItem )->GetValue();
	}
	aNameED.SetText( aName );
	aNameED.ClearModifyFlag();

	// determine RO-Flag
	if ( SFX_ITEM_UNKNOWN == rSet.GetItemState( ID_FILETP_READONLY, FALSE, &pItem )
		 || !pItem )
		aReadOnlyCB.Hide();
	else
		aReadOnlyCB.Check( ( (SfxBoolItem*)pItem )->GetValue() );

	// determine context symbol
	INetURLObject aURL;
	aURL.SetSmartProtocol( INET_PROT_FILE );
	aURL.SetSmartURL( aFactory);
	const String& rMainURL = aURL.GetMainURL( INetURLObject::NO_DECODE );
    aBmp1.SetImage( SvFileInformationManager::GetImage( aURL, TRUE ) );

	// determine size and type
	String aSizeText( aUnknownSize );
	if ( aURL.GetProtocol() == INET_PROT_FILE )
		aSizeText = CreateSizeText( SfxContentHelper::GetSize( aURL.GetMainURL( INetURLObject::NO_DECODE ) ) );
	aShowSizeFT.SetText( aSizeText );

	String aDescription = SvFileInformationManager::GetDescription( INetURLObject(rMainURL) );
	if ( aDescription.Len() == 0 )
		aDescription = String( SfxResId( STR_SFX_NEWOFFICEDOC ) );
	aShowTypeFT.SetText( aDescription );

	// determine location
	aURL.SetSmartURL( aFile);
	if ( aURL.GetProtocol() == INET_PROT_FILE )
	{
		INetURLObject aPath( aURL );
		aPath.setFinalSlash();
		aPath.removeSegment();
        // we know it's a folder -> don't need the final slash, but it's better for WB_PATHELLIPSIS
        aPath.removeFinalSlash();
        String aText( aPath.PathToFileName() ); //! (pb) MaxLen?
		aFileValFt.SetText( aText );
	}
	else if ( aURL.GetProtocol() != INET_PROT_PRIVATE )
		aFileValFt.SetText( aURL.GetPartBeforeLastName() );

	// handle access data
    sal_Bool m_bUseUserData = pInfoItem->IsUseUserData();
	LocaleDataWrapper aLocaleWrapper( ::comphelper::getProcessServiceFactory(), Application::GetSettings().GetLocale() );
	aCreateValFt.SetText( ConvertDateTime_Impl( pInfoItem->getAuthor(),
        pInfoItem->getCreationDate(), aLocaleWrapper ) );
	util::DateTime aTime( pInfoItem->getModificationDate() );
//	if ( aTime.IsValid() )
	if ( aTime.Month > 0 )
		aChangeValFt.SetText( ConvertDateTime_Impl(
            pInfoItem->getModifiedBy(), aTime, aLocaleWrapper ) );
	aTime = pInfoItem->getPrintDate();
//	if ( aTime.IsValid())
	if ( aTime.Month > 0 )
		aPrintValFt.SetText( ConvertDateTime_Impl( pInfoItem->getPrintedBy(),
            aTime, aLocaleWrapper ) );
	const long nTime = pInfoItem->getEditingDuration();
    if ( m_bUseUserData )
    {
		const Time aT( nTime/3600, (nTime%3600)/60, nTime%60 );
		aTimeLogValFt.SetText( aLocaleWrapper.getDuration( aT ) );
		aDocNoValFt.SetText( String::CreateFromInt32(
            pInfoItem->getEditingCycles() ) );
	}

    TriState eState = (TriState)m_bUseUserData;

	if ( STATE_DONTKNOW == eState )
		aUseUserDataCB.EnableTriState( TRUE );

	aUseUserDataCB.SetState( eState );
	aUseUserDataCB.SaveValue();
	aUseUserDataCB.Enable( bEnableUseUserData );
	bHandleDelete = FALSE;
	aDeleteBtn.Enable( bEnableUseUserData );
}

//------------------------------------------------------------------------

SfxInternetPage::SfxInternetPage( Window* pParent, const SfxItemSet& rItemSet ) :

	SfxTabPage( pParent, SfxResId( TP_DOCINFORELOAD ), rItemSet ),

	aRBNoAutoUpdate		( this, SfxResId( RB_NOAUTOUPDATE		) ),

	aRBReloadUpdate		( this, SfxResId( RB_RELOADUPDATE		) ),

	aRBForwardUpdate	( this, SfxResId( RB_FORWARDUPDATE		) ),
	aFTEvery			( this, SfxResId( FT_EVERY				) ),
	aNFReload			( this, SfxResId( ED_RELOAD			) ),
	aFTReloadSeconds	( this, SfxResId( FT_RELOADSECS		) ),
	aFTAfter			( this, SfxResId( FT_AFTER				) ),
	aNFAfter			( this, SfxResId( ED_FORWARD			) ),
	aFTAfterSeconds		( this, SfxResId( FT_FORWARDSECS		) ),
	aFTURL				( this, SfxResId( FT_URL				) ),
	aEDForwardURL		( this, SfxResId( ED_URL				) ),
	aPBBrowseURL		( this, SfxResId( PB_BROWSEURL			) ),
	aFTFrame			( this, SfxResId( FT_FRAME				) ),
	aCBFrame			( this, SfxResId( CB_FRAME				) ),

	aForwardErrorMessg	(		SfxResId( STR_FORWARD_ERRMSSG	) ),
    pInfoItem           ( NULL ),
    pFileDlg            ( NULL ),
    eState              ( S_Init )

{
	FreeResource();
	pInfoItem = &( SfxDocumentInfoItem& ) rItemSet.Get( SID_DOCINFO );
	TargetList			aList;
	SfxViewFrame*		pFrame = SfxViewFrame::Current();
	if( pFrame )
    {
        pFrame = pFrame->GetTopViewFrame();
        if( pFrame )
        {
            pFrame->GetTargetList( aList );

            String*			pObj;
            for( USHORT nPos = ( USHORT ) aList.Count() ; nPos ; )
            {
                pObj = aList.GetObject( --nPos );
                aCBFrame.InsertEntry( *pObj );
                delete pObj;
            }
        }
	}

	aRBNoAutoUpdate.SetClickHdl( LINK( this, SfxInternetPage, ClickHdlNoUpdate ) );
	aRBReloadUpdate.SetClickHdl( LINK( this, SfxInternetPage, ClickHdlReload ) );
	aRBForwardUpdate.SetClickHdl( LINK( this, SfxInternetPage, ClickHdlForward ) );
	aPBBrowseURL.SetClickHdl( LINK( this, SfxInternetPage, ClickHdlBrowseURL ) );

	aForwardErrorMessg.SearchAndReplaceAscii( "%PLACEHOLDER%", aRBForwardUpdate.GetText() );

	ChangeState( S_NoUpdate );
}

//------------------------------------------------------------------------

SfxInternetPage::~SfxInternetPage()
{
    delete pFileDlg;
}

//------------------------------------------------------------------------

void SfxInternetPage::ChangeState( STATE eNewState )
{
	DBG_ASSERT( eNewState != S_Init, "*SfxInternetPage::ChangeState(): new state init is supposed to not work here!" );

	if( eState == eNewState  )
		return;

	switch( eState )
	{
		case S_Init:
			EnableNoUpdate( TRUE );
			EnableReload( FALSE );
			EnableForward( FALSE );
			break;
		case S_NoUpdate:
			EnableNoUpdate( FALSE );
			if( eNewState == S_Reload )
				EnableReload( TRUE );
			else
				EnableForward( TRUE );
			break;
		case S_Reload:
			EnableReload( FALSE );
			if( eNewState == S_NoUpdate )
				EnableNoUpdate( TRUE );
			else
				EnableForward( TRUE );
			break;
		case S_Forward:
			EnableForward( FALSE );
			if( eNewState == S_NoUpdate )
				EnableNoUpdate( TRUE );
			else
				EnableReload( TRUE );
			break;
		default:
			DBG_ERROR( "*SfxInternetPage::SetState(): unhandled state!" );
	}

	eState = eNewState;
}

//------------------------------------------------------------------------

void SfxInternetPage::EnableNoUpdate( BOOL bEnable )
{
	if( bEnable )
		aRBNoAutoUpdate.Check();
}

//------------------------------------------------------------------------

void SfxInternetPage::EnableReload( BOOL bEnable )
{
	aFTEvery.Enable( bEnable );
	aNFReload.Enable( bEnable );
	aFTReloadSeconds.Enable( bEnable );

	if( bEnable )
		aRBReloadUpdate.Check();
}

//------------------------------------------------------------------------

void SfxInternetPage::EnableForward( BOOL bEnable )
{
	aFTAfter.Enable( bEnable );
	aNFAfter.Enable( bEnable );
	aFTAfterSeconds.Enable( bEnable );
	aFTURL.Enable( bEnable );
	aEDForwardURL.Enable( bEnable );
	aPBBrowseURL.Enable( bEnable );
	aFTFrame.Enable( bEnable );
	aCBFrame.Enable( bEnable );

	if( bEnable )
		aRBForwardUpdate.Check();
}

//------------------------------------------------------------------------

IMPL_LINK( SfxInternetPage, ClickHdlNoUpdate, Control*, pCtrl )
{
    (void)pCtrl; //unused
	ChangeState( S_NoUpdate );
	return 0;
}

//------------------------------------------------------------------------

IMPL_LINK( SfxInternetPage, ClickHdlReload, Control*, pCtrl )
{
    (void)pCtrl; //unused
	ChangeState( S_Reload );
	return 0;
}

//------------------------------------------------------------------------

IMPL_LINK( SfxInternetPage, ClickHdlForward, Control*, pCtrl )
{
    (void)pCtrl; //unused
	ChangeState( S_Forward );
	return 0;
}

//------------------------------------------------------------------------

IMPL_LINK( SfxInternetPage, ClickHdlBrowseURL, PushButton*, EMPTYARG )
{
    if ( !pFileDlg )
        pFileDlg = new sfx2::FileDialogHelper( TemplateDescription::FILEOPEN_SIMPLE, WB_OPEN );
    pFileDlg->SetDisplayDirectory( aEDForwardURL.GetText() );
    pFileDlg->StartExecuteModal( LINK( this, SfxInternetPage, DialogClosedHdl ) );

    return 0;
}

//------------------------------------------------------------------------

IMPL_LINK( SfxInternetPage, DialogClosedHdl, sfx2::FileDialogHelper*, EMPTYARG )
{
    DBG_ASSERT( pFileDlg, "SfxInternetPage::DialogClosedHdl(): no file dialog" );

    if ( ERRCODE_NONE == pFileDlg->GetError() )
        aEDForwardURL.SetText( pFileDlg->GetPath() );

    return 0;
}

//------------------------------------------------------------------------

BOOL SfxInternetPage::FillItemSet( SfxItemSet& rSet )
{
	const SfxPoolItem*			pItem = NULL;
	SfxDocumentInfoItem*		pInfo = NULL;
	SfxTabDialog*				pDlg = GetTabDialog();
	const SfxItemSet*			pExSet = NULL;

	if( pDlg )
		pExSet = pDlg->GetExampleSet();

	if( pExSet && SFX_ITEM_SET != pExSet->GetItemState( SID_DOCINFO, TRUE, &pItem ) )
		pInfo = pInfoItem;
    else if ( pItem )
        pInfo = new SfxDocumentInfoItem( *(const SfxDocumentInfoItem*)pItem );

    if ( !pInfo )
    {
        DBG_ERRORFILE( "SfxInternetPage::FillItemSet(): no item found" );
        return FALSE;
    }

    DBG_ASSERT( eState != S_Init, "*SfxInternetPage::FillItemSet(): state init is not acceptable at this point!" );

	BOOL						bEnableReload = FALSE;
	::std::auto_ptr< String >	aURL( NULL );
	::std::auto_ptr< String >	aFrame( NULL );
	ULONG						nDelay = 0;

	switch( eState )
	{
		case S_NoUpdate:
			break;
		case S_Reload:
			bEnableReload = TRUE;
			aURL = ::std::auto_ptr< String >( new String() );
			aFrame = ::std::auto_ptr< String >( new String() );
			nDelay = static_cast<ULONG>(aNFReload.GetValue());
			break;
		case S_Forward:
			DBG_ASSERT( aEDForwardURL.GetText().Len(), "+SfxInternetPage::FillItemSet(): empty URL should be not possible for forward option!" );

			bEnableReload = TRUE;
            aURL = ::std::auto_ptr< String >( new String( URIHelper::SmartRel2Abs( INetURLObject(aBaseURL), aEDForwardURL.GetText(), URIHelper::GetMaybeFileHdl(), true ) ) );
			aFrame = ::std::auto_ptr< String >( new String( aCBFrame.GetText() ) );
			nDelay = static_cast<ULONG>(aNFAfter.GetValue());
			break;
              default:
                  break;
	}

	pInfo->setAutoloadEnabled( bEnableReload );

	if( bEnableReload )
	{
		pInfo->setAutoloadURL( *aURL.get() );
		pInfo->setDefaultTarget( *aFrame.get() );
		pInfo->setAutoloadDelay( nDelay );
	}

	rSet.Put( *pInfo );
	if( pInfo != pInfoItem )
		delete pInfo;
	return TRUE;
}

//------------------------------------------------------------------------

SfxTabPage *SfxInternetPage::Create( Window* pParent, const SfxItemSet& rItemSet )
{
	return new SfxInternetPage(pParent, rItemSet);
}

//------------------------------------------------------------------------

void SfxInternetPage::Reset( const SfxItemSet& rSet )
{
	pInfoItem = &( SfxDocumentInfoItem& ) rSet.Get( SID_DOCINFO );
    SFX_ITEMSET_ARG( &rSet, pURLItem, SfxStringItem, SID_BASEURL, FALSE );
    DBG_ASSERT( pURLItem, "No BaseURL provided for InternetTabPage!" );
    if ( pURLItem )
        aBaseURL = pURLItem->GetValue();

	STATE				eNewState = S_NoUpdate;

	if( pInfoItem->isAutoloadEnabled() )
	{
		const String&	rURL = pInfoItem->getAutoloadURL();

		if( rURL.Len() )
		{
			aNFAfter.SetValue( pInfoItem->getAutoloadDelay() );
			aEDForwardURL.SetText( rURL );
			aCBFrame.SetText( pInfoItem->getDefaultTarget() );
			eNewState = S_Forward;
		}
		else
		{
			aNFReload.SetValue( pInfoItem->getAutoloadDelay() );
			eNewState = S_Reload;
		}
	}

	ChangeState( eNewState );

	// #102907# ------------------------
    SFX_ITEMSET_ARG( &rSet, pROItem, SfxBoolItem, SID_DOC_READONLY, FALSE );
	if ( pROItem && pROItem->GetValue() )
	{
		aRBNoAutoUpdate.Disable();
		aRBReloadUpdate.Disable();
		aRBForwardUpdate.Disable();
		aNFReload.Disable();
		aNFAfter.Disable();
		aEDForwardURL.Disable();
		aPBBrowseURL.Disable();
		aCBFrame.Disable();
		aFTEvery.Disable();
		aFTReloadSeconds.Disable();
		aFTAfter.Disable();
		aFTAfterSeconds.Disable();
		aFTURL.Disable();
		aFTFrame.Disable();
	}
}

//------------------------------------------------------------------------

int SfxInternetPage::DeactivatePage( SfxItemSet* /*pSet*/ )
{
	int				nRet = LEAVE_PAGE;

	if( eState == S_Forward && !aEDForwardURL.GetText().Len() )
	{
		ErrorBox	aErrBox( this, WB_OK, aForwardErrorMessg );
		aErrBox.Execute();

		nRet = KEEP_PAGE;
	}

	return nRet;
}

//------------------------------------------------------------------------

SfxDocumentInfoDialog::SfxDocumentInfoDialog( Window* pParent,
											  const SfxItemSet& rItemSet ) :

	SfxTabDialog( 0, pParent, SfxResId( SID_DOCINFO ), &rItemSet )

{
	FreeResource();

	 const SfxDocumentInfoItem* pInfoItem =
		&(const SfxDocumentInfoItem &)rItemSet.Get( SID_DOCINFO );

#ifdef DBG_UTIL
    SFX_ITEMSET_ARG( &rItemSet, pURLItem, SfxStringItem, SID_BASEURL, FALSE );
    DBG_ASSERT( pURLItem, "No BaseURL provided for InternetTabPage!" );
#endif

	 // Bestimmung des Titels
	const SfxPoolItem* pItem = 0;
	String aTitle( GetText() );
	if ( SFX_ITEM_SET !=
		 rItemSet.GetItemState( SID_EXPLORER_PROPS_START, FALSE, &pItem ) )
	{
		// Dateiname
		String aFile( pInfoItem->GetValue() );
#ifdef WIN
		if ( aFile.Len() <= 8 )
		{
			String sTmp( SfxResId( STR_NONAME ) );
			USHORT nLen = Min( (USHORT)8, sTmp.Len() );

			if ( sTmp.Copy( 0, nLen ).Lower() ==
				 aFile.Copy( 0, nLen ).Lower() )
			{
				aFile = pInfoItem->GetValue();
			}
		}
#endif

		INetURLObject aURL;
		aURL.SetSmartProtocol( INET_PROT_FILE );
		aURL.SetSmartURL( aFile);
		if ( INET_PROT_PRIV_SOFFICE != aURL.GetProtocol() )
		{
			String aLastName( aURL.GetLastName() );
			if ( aLastName.Len() )
				aTitle += aLastName;
			else
				aTitle += aFile;
		}
		else
			aTitle += String( SfxResId( STR_NONAME ) );
	}
	else
	{
		DBG_ASSERT( pItem->IsA( TYPE( SfxStringItem ) ),
					"SfxDocumentInfoDialog:<SfxStringItem> erwartet" );
		aTitle += ( ( SfxStringItem* ) pItem )->GetValue();
	}
	SetText( aTitle );

	// Eigenschaftenseiten
	AddTabPage(TP_DOCINFODESC, SfxDocumentDescPage::Create, 0);
	AddTabPage(TP_DOCINFODOC, SfxDocumentPage::Create, 0);
    AddTabPage(TP_CUSTOMPROPERTIES, SfxCustomPropertiesPage::Create, 0);
    AddTabPage(TP_DOCINFORELOAD, SfxInternetPage::Create, 0);
}

// -----------------------------------------------------------------------

void SfxDocumentInfoDialog::PageCreated( USHORT nId, SfxTabPage &rPage )
{
	if ( TP_DOCINFODOC == nId )
		( (SfxDocumentPage&)rPage ).EnableUseUserData();
}

// class CustomPropertiesYesNoButton -------------------------------------

CustomPropertiesYesNoButton::CustomPropertiesYesNoButton( Window* pParent, const ResId& rResId ) :
    Control( pParent, rResId ),
    m_aYesButton( this, ResId( RB_PROPERTY_YES, *rResId.GetResMgr() ) ),
    m_aNoButton ( this, ResId( RB_PROPERTY_NO, *rResId.GetResMgr() ) )
{
    FreeResource();
    Wallpaper aWall( Color( COL_TRANSPARENT ) );
    SetBackground( aWall );
    SetBorderStyle( WINDOW_BORDER_MONO  );
    CheckNo();
    m_aYesButton.SetBackground( aWall );
    m_aNoButton.SetBackground( aWall );
}

void CustomPropertiesYesNoButton::Resize()
{
    const long nWidth = GetSizePixel().Width();
    const long n3Width = LogicToPixel( Size( 3, 3 ), MAP_APPFONT ).Width();
    const long nNewWidth = ( nWidth / 2 ) - n3Width - 2;
    Size aSize = m_aYesButton.GetSizePixel();
    const long nDelta = aSize.Width() - nNewWidth;
    aSize.Width() = nNewWidth;
    m_aYesButton.SetSizePixel( aSize );
    Point aPos = m_aNoButton.GetPosPixel();
    aPos.X() -= nDelta;
    m_aNoButton.SetPosSizePixel( aPos, aSize );
}

// struct CustomPropertyLine ---------------------------------------------

CustomPropertyLine::CustomPropertyLine( Window* pParent ) :
    m_aNameBox      ( pParent, SfxResId( SFX_CB_PROPERTY_NAME ) ),
    m_aTypeBox      ( pParent, SfxResId( SFX_LB_PROPERTY_TYPE ), this ),
    m_aValueEdit    ( pParent, SfxResId( SFX_ED_PROPERTY_VALUE ), this ),
    m_aYesNoButton  ( pParent, SfxResId( SFX_WIN_PROPERTY_YESNO ) ),
    m_aRemoveButton ( pParent, SfxResId( SFX_PB_PROPERTY_REMOVE ), this ),
    m_bIsRemoved    ( false ),
    m_bTypeLostFocus( false )

{
    m_aRemoveButton.SetModeImage( SfxResId( SFX_IMG_PROPERTY_REMOVE ), BMP_COLOR_NORMAL );
    m_aRemoveButton.SetModeImage( SfxResId( SFX_IMG_PROPERTY_REMOVE_HC ), BMP_COLOR_HIGHCONTRAST );
}

void CustomPropertyLine::SetRemoved()
{
    DBG_ASSERT( !m_bIsRemoved, "CustomPropertyLine::SetRemoved(): line already removed" );
    m_bIsRemoved = true;
    m_aNameBox.Hide();
    m_aTypeBox.Hide();
    m_aValueEdit.Hide();
    m_aYesNoButton.Hide();
    m_aRemoveButton.Hide();
}

// class CustomPropertiesWindow ------------------------------------------

CustomPropertiesWindow::CustomPropertiesWindow( Window* pParent, const ResId& rResId ) :

    Window( pParent, rResId ),
    m_aNameBox      ( this, SfxResId( SFX_CB_PROPERTY_NAME ) ),
    m_aTypeBox      ( this, SfxResId( SFX_LB_PROPERTY_TYPE ) ),
    m_aValueEdit    ( this, SfxResId( SFX_ED_PROPERTY_VALUE ) ),
    m_aYesNoButton  ( this, SfxResId( SFX_WIN_PROPERTY_YESNO ) ),
    m_aRemoveButton ( this, SfxResId( SFX_PB_PROPERTY_REMOVE ) ),
	m_nScrollPos (0),
    m_aNumberFormatter( ::comphelper::getProcessServiceFactory(),
                        Application::GetSettings().GetLanguage() )

{
    m_aEditLoseFocusTimer.SetTimeout( 300 );
    m_aEditLoseFocusTimer.SetTimeoutHdl( LINK( this, CustomPropertiesWindow, EditTimeoutHdl ) );
    m_aBoxLoseFocusTimer.SetTimeout( 300 );
    m_aBoxLoseFocusTimer.SetTimeoutHdl( LINK( this, CustomPropertiesWindow, BoxTimeoutHdl ) );
}

CustomPropertiesWindow::~CustomPropertiesWindow()
{
    m_aEditLoseFocusTimer.Stop();
    m_aBoxLoseFocusTimer.Stop();
    ClearAllLines();
}

IMPL_LINK( CustomPropertiesWindow, TypeHdl, CustomPropertiesTypeBox*, pBox )
{
    sal_Int64 nType = sal_Int64( (long)pBox->GetEntryData( pBox->GetSelectEntryPos() ) );
    CustomPropertyLine* pLine = pBox->GetLine();
    BOOL bBooleanType = ( CUSTOM_TYPE_BOOLEAN == nType );
    pLine->m_aValueEdit.Show( !bBooleanType );
    pLine->m_aYesNoButton.Show( bBooleanType );
    return 0;
}

IMPL_LINK( CustomPropertiesWindow, RemoveHdl, CustomPropertiesRemoveButton*, pButton )
{
    CustomPropertyLine* pLine = pButton->GetLine();
    std::vector< CustomPropertyLine* >::iterator pFound =
        std::find( m_aCustomPropertiesLines.begin(), m_aCustomPropertiesLines.end(), pLine );
    if ( pFound != m_aCustomPropertiesLines.end() )
    {
        pLine = *pFound;
        pLine->SetRemoved();
        std::vector< CustomPropertyLine* >::iterator pIter = pFound + 1;
        const long nDelta = GetLineHeight();
        for ( ; pIter != m_aCustomPropertiesLines.end(); ++pIter )
        {
            pLine = *pIter;
            if ( pLine->m_bIsRemoved )
                continue;

            Window* pWindows[] = {  &pLine->m_aNameBox, &pLine->m_aTypeBox, &pLine->m_aValueEdit,
                                    &pLine->m_aYesNoButton, &pLine->m_aRemoveButton, NULL };
            Window** pCurrent = pWindows;
            while ( *pCurrent )
            {
                Point aPos = (*pCurrent)->GetPosPixel();
                aPos.Y() -= nDelta;
                (*pCurrent)->SetPosPixel( aPos );
                pCurrent++;
            }
        }
    }

	m_aRemovedHdl.Call(0);
    return 0;
}

IMPL_LINK( CustomPropertiesWindow, EditLoseFocusHdl, CustomPropertiesEdit*, pEdit )
{
    if ( pEdit )
    {
        CustomPropertyLine* pLine = pEdit->GetLine();
        if ( !pLine->m_bTypeLostFocus )
        {
            m_pCurrentLine = pLine;
            m_aEditLoseFocusTimer.Start();
        }
        else
            pLine->m_bTypeLostFocus = false;
    }
    return 0;
}

IMPL_LINK( CustomPropertiesWindow, BoxLoseFocusHdl, CustomPropertiesTypeBox*, pBox )
{
    if ( pBox )
    {
        m_pCurrentLine = pBox->GetLine();
        m_aBoxLoseFocusTimer.Start();
    }

    return 0;
}

IMPL_LINK( CustomPropertiesWindow, EditTimeoutHdl, Timer*, EMPTYARG )
{
    ValidateLine( m_pCurrentLine, false );
    return 0;
}

IMPL_LINK( CustomPropertiesWindow, BoxTimeoutHdl, Timer*, EMPTYARG )
{
    ValidateLine( m_pCurrentLine, true );
    return 0;
}

bool CustomPropertiesWindow::IsLineValid( CustomPropertyLine* pLine ) const
{
    bool bIsValid = true;
    pLine->m_bTypeLostFocus = false;
    sal_Int64 nType = sal_Int64(
        (long)pLine->m_aTypeBox.GetEntryData( pLine->m_aTypeBox.GetSelectEntryPos() ) );
    String sValue = pLine->m_aValueEdit.GetText();
    if ( sValue.Len() == 0 )
        return true;

    double fDummy = 0.0;
    sal_uInt32 nIndex = 0xFFFFFFFF;
    if ( CUSTOM_TYPE_NUMBER == nType )
        nIndex = const_cast< SvNumberFormatter& >(
            m_aNumberFormatter ).GetFormatIndex( NF_NUMBER_SYSTEM );
    else if ( CUSTOM_TYPE_DATE == nType )
        nIndex = const_cast< SvNumberFormatter& >(
            m_aNumberFormatter).GetFormatIndex( NF_DATE_SYS_DDMMYYYY );

    if ( nIndex != 0xFFFFFFFF )
    {
        sal_uInt32 nTemp = nIndex;
        bIsValid = const_cast< SvNumberFormatter& >(
            m_aNumberFormatter ).IsNumberFormat( sValue, nIndex, fDummy ) != FALSE;
        if ( bIsValid && nTemp != nIndex )
            // sValue is a number but the format doesn't match the index
            bIsValid = false;
    }

    return bIsValid;
}

void CustomPropertiesWindow::ValidateLine( CustomPropertyLine* pLine, bool bIsFromTypeBox )
{
    if ( !IsLineValid( pLine ) )
    {
        if ( bIsFromTypeBox ) // LoseFocus of TypeBox
            pLine->m_bTypeLostFocus = true;
        Window* pParent = GetParent()->GetParent();
        if ( QueryBox( pParent, SfxResId( SFX_QB_WRONG_TYPE ) ).Execute() == RET_OK )
            pLine->m_aTypeBox.SelectEntryPos( m_aTypeBox.GetEntryPos( (void*)CUSTOM_TYPE_TEXT ) );
        else
            pLine->m_aValueEdit.GrabFocus();
    }
}

void CustomPropertiesWindow::InitControls( HeaderBar* pHeaderBar, const ScrollBar* pScrollBar )
{
    DBG_ASSERT( pHeaderBar, "CustomPropertiesWindow::InitControls(): invalid headerbar" );
    DBG_ASSERT( pScrollBar, "CustomPropertiesWindow::InitControls(): invalid scrollbar" );

    m_aNameBox.Hide();
    m_aTypeBox.Hide();
    m_aValueEdit.Hide();
    m_aYesNoButton.Hide();
    m_aRemoveButton.Hide();

    const long nOffset = 4;
    const long nScrollBarWidth = pScrollBar->GetSizePixel().Width();
    const long nButtonWidth = m_aRemoveButton.GetSizePixel().Width() + nScrollBarWidth + nOffset;
    long nTypeWidth = m_aTypeBox.CalcMinimumSize().Width() + ( 2 * nOffset );
    long nFullWidth = pHeaderBar->GetSizePixel().Width();
    long nItemWidth = ( nFullWidth - nTypeWidth - nButtonWidth ) / 2;
    pHeaderBar->SetItemSize( HI_NAME, nItemWidth );
    pHeaderBar->SetItemSize( HI_TYPE, nTypeWidth );
    pHeaderBar->SetItemSize( HI_VALUE, nItemWidth );
    pHeaderBar->SetItemSize( HI_ACTION, nButtonWidth );

    Window* pWindows[] = { &m_aNameBox, &m_aTypeBox, &m_aValueEdit, &m_aRemoveButton, NULL };
    Window** pCurrent = pWindows;
    USHORT nPos = 0;
    while ( *pCurrent )
    {
        Rectangle aRect = pHeaderBar->GetItemRect( pHeaderBar->GetItemId( nPos++ ) );
        Size aSize = (*pCurrent)->GetSizePixel();
        Point aPos = (*pCurrent)->GetPosPixel();
        long nWidth = aRect.getWidth() - nOffset;
        if ( *pCurrent == &m_aRemoveButton )
            nWidth -= pScrollBar->GetSizePixel().Width();
        aSize.Width() = nWidth;
        aPos.X() = aRect.getX() + ( nOffset / 2 );
        (*pCurrent)->SetPosSizePixel( aPos, aSize );

        if ( *pCurrent == &m_aValueEdit )
        {
            aSize = m_aYesNoButton.GetSizePixel();
            aPos = m_aYesNoButton.GetPosPixel();
            aSize.Width() = nWidth;
            aPos.X() = aRect.getX() + ( nOffset / 2 );
            m_aYesNoButton.SetPosSizePixel( aPos, aSize );
        }

        pCurrent++;
    }

    m_nLineHeight =
        ( m_aRemoveButton.GetPosPixel().Y() * 2 ) + m_aRemoveButton.GetSizePixel().Height();
}

USHORT CustomPropertiesWindow::GetVisibleLineCount() const
{
    USHORT nCount = 0;
    std::vector< CustomPropertyLine* >::const_iterator pIter;
    for ( pIter = m_aCustomPropertiesLines.begin();
            pIter != m_aCustomPropertiesLines.end(); ++pIter )
    {
        CustomPropertyLine* pLine = *pIter;
        if ( !pLine->m_bIsRemoved )
            nCount++;
    }
    return nCount;
}

void CustomPropertiesWindow::AddLine( const ::rtl::OUString& sName, Any& rAny )
{
    CustomPropertyLine* pNewLine = new CustomPropertyLine( this );
    pNewLine->m_aTypeBox.SetSelectHdl( LINK( this, CustomPropertiesWindow, TypeHdl ) );
    pNewLine->m_aRemoveButton.SetClickHdl( LINK( this, CustomPropertiesWindow, RemoveHdl ) );
    pNewLine->m_aValueEdit.SetLoseFocusHdl( LINK( this, CustomPropertiesWindow, EditLoseFocusHdl ) );
    pNewLine->m_aTypeBox.SetLoseFocusHdl( LINK( this, CustomPropertiesWindow, BoxLoseFocusHdl ) );
    long nPos = GetVisibleLineCount() * GetLineHeight();
    m_aCustomPropertiesLines.push_back( pNewLine );
    Window* pWindows[] = {  &m_aNameBox, &m_aTypeBox, &m_aValueEdit,
                            &m_aYesNoButton, &m_aRemoveButton, NULL };
    Window* pNewWindows[] =
        {   &pNewLine->m_aNameBox, &pNewLine->m_aTypeBox, &pNewLine->m_aValueEdit,
            &pNewLine->m_aYesNoButton, &pNewLine->m_aRemoveButton, NULL };
    Window** pCurrent = pWindows;
    Window** pNewCurrent = pNewWindows;
    while ( *pCurrent )
    {
        Size aSize = (*pCurrent)->GetSizePixel();
        Point aPos = (*pCurrent)->GetPosPixel();
        aPos.Y() += nPos;
		aPos.Y() += m_nScrollPos;
        (*pNewCurrent)->SetPosSizePixel( aPos, aSize );
        (*pNewCurrent)->Show();
        pCurrent++;
        pNewCurrent++;
    }

    double nTmpValue = 0;
    bool bTmpValue = false;
    ::rtl::OUString sTmpValue;
    util::DateTime aTmpDate;
    SvtSysLocale aSysLocale;
    const LocaleDataWrapper& rLocaleWrapper = aSysLocale.GetLocaleData();
    pNewLine->m_aNameBox.SetText( sName );
    sal_Int32 nType = CUSTOM_TYPE_UNKNOWN;
    String sValue;

    if ( rAny >>= nTmpValue )
    {
        sal_uInt32 nIndex = m_aNumberFormatter.GetFormatIndex( NF_NUMBER_SYSTEM );
        m_aNumberFormatter.GetInputLineString( nTmpValue, nIndex, sValue );
        nType = CUSTOM_TYPE_NUMBER;
    }
    else if ( rAny >>= bTmpValue )
    {
        sValue = ( bTmpValue ? rLocaleWrapper.getTrueWord() : rLocaleWrapper.getFalseWord() );
        nType = CUSTOM_TYPE_BOOLEAN;
    }
    else if ( rAny >>= sTmpValue )
    {
        sValue = String( sTmpValue );
        nType = CUSTOM_TYPE_TEXT;
    }
    else if ( rAny >>= aTmpDate )
    {
        DateFormat eFormat = rLocaleWrapper.getDateFormat();
        sal_Unicode cDateSep = rLocaleWrapper.getDateSep().GetChar(0);
        switch ( eFormat )
        {
            case MDY :
            {
                sValue = String::CreateFromInt32( aTmpDate.Month );
                sValue += cDateSep;
                sValue += String::CreateFromInt32( aTmpDate.Day );
                sValue += cDateSep;
                if ( aTmpDate.Year < 10 )
                    sValue += '0';
                sValue += String::CreateFromInt32( aTmpDate.Year );
                break;
            }
            case DMY :
            {
                sValue = String::CreateFromInt32( aTmpDate.Day );
                sValue += cDateSep;
                sValue += String::CreateFromInt32( aTmpDate.Month );
                sValue += cDateSep;
                if ( aTmpDate.Year < 10 )
                    sValue += '0';
                sValue += String::CreateFromInt32( aTmpDate.Year );
                break;
            }
            case YMD :
            {
                if ( aTmpDate.Year < 10 )
                    sValue += '0';
                sValue += String::CreateFromInt32( aTmpDate.Year );
                sValue += cDateSep;
                sValue += String::CreateFromInt32( aTmpDate.Month );
                sValue += cDateSep;
                sValue += String::CreateFromInt32( aTmpDate.Day );
                break;
            }
        }
        nType = CUSTOM_TYPE_DATE;
    }

    if ( nType != CUSTOM_TYPE_UNKNOWN )
    {
        if ( CUSTOM_TYPE_BOOLEAN == nType )
        {
            if ( bTmpValue )
                pNewLine->m_aYesNoButton.CheckYes();
            else
                pNewLine->m_aYesNoButton.CheckNo();
        }
        else
            pNewLine->m_aValueEdit.SetText( sValue );
        pNewLine->m_aTypeBox.SelectEntryPos( m_aTypeBox.GetEntryPos( (void*)nType ) );
    }

    TypeHdl( &pNewLine->m_aTypeBox );
    pNewLine->m_aNameBox.GrabFocus();
}

bool CustomPropertiesWindow::AreAllLinesValid() const
{
    bool bRet = true;
    std::vector< CustomPropertyLine* >::const_iterator pIter;
    for ( pIter = m_aCustomPropertiesLines.begin();
            pIter != m_aCustomPropertiesLines.end(); ++pIter )
    {
        CustomPropertyLine* pLine = *pIter;
        if ( !IsLineValid( pLine ) )
        {
            bRet = false;
            break;
        }
    }

    return bRet;
}

void CustomPropertiesWindow::ClearAllLines()
{
    std::vector< CustomPropertyLine* >::iterator pIter;
    for ( pIter = m_aCustomPropertiesLines.begin();
          pIter != m_aCustomPropertiesLines.end(); ++pIter )
    {
        CustomPropertyLine* pLine = *pIter;
        pLine->SetRemoved();
        delete pLine;
    }
    m_aCustomPropertiesLines.clear();
	m_nScrollPos = 0;
}

void CustomPropertiesWindow::DoScroll( sal_Int32 nNewPos )
{
	m_nScrollPos += nNewPos;
	std::vector< CustomPropertyLine* >::iterator pIter;
    for ( pIter = m_aCustomPropertiesLines.begin();
            pIter != m_aCustomPropertiesLines.end(); ++pIter )
    {
        CustomPropertyLine* pLine = *pIter;
        if ( pLine->m_bIsRemoved )
            continue;

        Window* pWindows[] = {  &pLine->m_aNameBox, &pLine->m_aTypeBox, &pLine->m_aValueEdit,
                                &pLine->m_aYesNoButton, &pLine->m_aRemoveButton, NULL };
        Window** pCurrent = pWindows;
        while ( *pCurrent )
        {
            Point aPos = (*pCurrent)->GetPosPixel();
            aPos.Y() += nNewPos;
            (*pCurrent)->SetPosPixel( aPos );
            pCurrent++;
        }
    }
}

bool CustomPropertiesWindow::DoesCustomPropertyExist( const String& rName ) const
{
    bool bRet = false;
    std::vector< CustomPropertyLine* >::const_iterator pIter;
    for ( pIter = m_aCustomPropertiesLines.begin();
            pIter != m_aCustomPropertiesLines.end(); ++pIter )
    {
        CustomPropertyLine* pLine = *pIter;
        if ( !pLine->m_bIsRemoved && pLine->m_aNameBox.GetText() == rName )
        {
            bRet = true;
            break;
        }
    }

    return bRet;
}

Sequence< beans::PropertyValue > CustomPropertiesWindow::GetCustomProperties() const
{
    Sequence< beans::PropertyValue > aPropertiesSeq( m_aCustomPropertiesLines.size() );
    sal_Int32 i = 0;
    std::vector< CustomPropertyLine* >::const_iterator pIter;
    for ( pIter = m_aCustomPropertiesLines.begin();
            pIter != m_aCustomPropertiesLines.end(); ++pIter, ++i )
    {
        CustomPropertyLine* pLine = *pIter;
        if ( pLine->m_bIsRemoved )
            continue;

        String sPropertyName = pLine->m_aNameBox.GetText();
        if ( sPropertyName.Len() > 0 )
        {
            aPropertiesSeq[i].Name = sPropertyName;
            sal_Int64 nType = sal_Int64(
                (long)pLine->m_aTypeBox.GetEntryData( pLine->m_aTypeBox.GetSelectEntryPos() ) );
            if ( CUSTOM_TYPE_NUMBER == nType )
            {
                double nValue = 0;
                sal_uInt32 nIndex = const_cast< SvNumberFormatter& >(
                    m_aNumberFormatter ).GetFormatIndex( NF_NUMBER_SYSTEM );
                BOOL bIsNum = const_cast< SvNumberFormatter& >( m_aNumberFormatter ).
                    IsNumberFormat( pLine->m_aValueEdit.GetText(), nIndex, nValue );
                if ( bIsNum )
                    aPropertiesSeq[i].Value <<= makeAny( nValue );
            }
            else if ( CUSTOM_TYPE_BOOLEAN == nType )
            {
                bool bValue = pLine->m_aYesNoButton.IsYesChecked();
                aPropertiesSeq[i].Value <<= makeAny( bValue );
            }
            else if ( CUSTOM_TYPE_DATE == nType )
            {
                const LocaleDataWrapper& rLocaleWrapper = SvtSysLocale().GetLocaleData();
                DateFormat eFormat = rLocaleWrapper.getDateFormat();
                sal_Unicode cDateSep = rLocaleWrapper.getDateSep().GetChar(0);
                String sValue( pLine->m_aValueEdit.GetText() );
                if ( sValue.GetTokenCount( cDateSep ) == 3 )
                {
                    xub_StrLen nTokenDay = STRING_NOTFOUND;
                    xub_StrLen nTokenMonth = STRING_NOTFOUND;
                    xub_StrLen nTokenYear = STRING_NOTFOUND;

                    switch ( eFormat )
                    {
                        case MDY :
                        {
                            nTokenDay = 1;
                            nTokenMonth = 0;
                            nTokenYear = 2;
                            break;
                        }
                        case DMY :
                        {
                            nTokenDay = 0;
                            nTokenMonth = 1;
                            nTokenYear = 2;
                            break;
                        }
                        case YMD :
                        {
                            nTokenDay = 2;
                            nTokenMonth = 1;
                            nTokenYear = 0;
                            break;
                        }
                    }

                    if ( nTokenDay != STRING_NOTFOUND )
                    {
                        util::DateTime aDateTime( 0, 0, 0, 0,
                            sal_uInt16( sValue.GetToken( nTokenDay, cDateSep ).ToInt32() ),
                            sal_uInt16( sValue.GetToken( nTokenMonth, cDateSep ).ToInt32() ),
                            sal_uInt16( sValue.GetToken( nTokenYear, cDateSep ).ToInt32() ) );
                        aPropertiesSeq[i].Value <<= makeAny( aDateTime );
                    }
                }
            }
            else
            {
                ::rtl::OUString sValue( pLine->m_aValueEdit.GetText() );
                aPropertiesSeq[i].Value <<= makeAny( sValue );
            }
        }
    }

    return aPropertiesSeq;
}

// class CustomPropertiesControl -----------------------------------------

CustomPropertiesControl::CustomPropertiesControl( Window* pParent, const ResId& rResId ) :

    Control( pParent, rResId ),

    m_aHeaderBar    ( this, WB_BUTTONSTYLE | WB_BOTTOMBORDER ),
    m_aPropertiesWin( this, ResId( WIN_PROPERTIES, *rResId.GetResMgr() ) ),
    m_aVertScroll   ( this, ResId( SB_VERTICAL, *rResId.GetResMgr() ) ),

    m_bIsInitialized( false ),
    m_nThumbPos     ( 0 )

{
    m_aPropertiesWin.SetBackground( Wallpaper( GetSettings().GetStyleSettings().GetFieldColor() ) );
    m_aVertScroll.EnableDrag();
    m_aVertScroll.Show();
    long nWidth = GetOutputSizePixel().Width();
    m_aHeaderBar.SetPosSizePixel( Point(), Size( nWidth, m_aVertScroll.GetPosPixel().Y() ) );
    const HeaderBarItemBits nHeadBits = HIB_VCENTER | HIB_FIXED | HIB_FIXEDPOS | HIB_LEFT;
    nWidth = nWidth / 4;
    ResMgr* pResMgr = rResId.GetResMgr();
    m_aHeaderBar.InsertItem( HI_NAME, String( ResId( STR_HEADER_NAME, *pResMgr ) ), nWidth, nHeadBits );
    m_aHeaderBar.InsertItem( HI_TYPE, String( ResId( STR_HEADER_TYPE, *pResMgr ) ), nWidth, nHeadBits );
    m_aHeaderBar.InsertItem( HI_VALUE, String( ResId( STR_HEADER_VALUE, *pResMgr ) ), nWidth, nHeadBits );
    m_aHeaderBar.InsertItem( HI_ACTION, String( ResId( STR_HEADER_ACTION, *pResMgr ) ), nWidth, nHeadBits );
    m_aHeaderBar.Show();

    FreeResource();

    XubString sTEST = m_aHeaderBar.GetItemText( HI_NAME );

    m_aPropertiesWin.InitControls( &m_aHeaderBar, &m_aVertScroll );
	m_aPropertiesWin.SetRemovedHdl( LINK( this, CustomPropertiesControl, RemovedHdl ) );

    m_aVertScroll.SetRangeMin( 0 );
    sal_Int32 nScrollOffset = m_aPropertiesWin.GetLineHeight();
    sal_Int32 nVisibleEntries = m_aPropertiesWin.GetSizePixel().Height() / nScrollOffset;
    m_aVertScroll.SetRangeMax( nVisibleEntries );
    m_aVertScroll.SetPageSize( nVisibleEntries - 1 );
    m_aVertScroll.SetVisibleSize( nVisibleEntries );

    Point aPos = m_aHeaderBar.GetPosPixel();
    Size aSize = m_aHeaderBar.GetSizePixel();
    aPos = m_aVertScroll.GetPosPixel();
    aSize = m_aVertScroll.GetSizePixel();

    Link aScrollLink = LINK( this, CustomPropertiesControl, ScrollHdl );
    m_aVertScroll.SetScrollHdl( aScrollLink );
//    m_aVertScroll.SetEndScrollHdl( aScrollLink );
}

CustomPropertiesControl::~CustomPropertiesControl()
{
}

void CustomPropertiesControl::Initialize()
{
}

IMPL_LINK( CustomPropertiesControl, ScrollHdl, ScrollBar*, pScrollBar )
{
    sal_Int32 nOffset = m_aPropertiesWin.GetLineHeight();
    nOffset *= ( m_nThumbPos - pScrollBar->GetThumbPos() );
    m_nThumbPos = pScrollBar->GetThumbPos();
    m_aPropertiesWin.DoScroll( nOffset );
    return 0;
}

IMPL_LINK( CustomPropertiesControl, RemovedHdl, void*, EMPTYARG )
{
    m_aVertScroll.SetRangeMax( m_aPropertiesWin.GetVisibleLineCount() + 1 );
	if ( m_aPropertiesWin.GetOutputSizePixel().Height() < m_aPropertiesWin.GetVisibleLineCount() * m_aPropertiesWin.GetLineHeight() )
		m_aVertScroll.DoScrollAction ( SCROLL_LINEUP );
	return 0;
}

void CustomPropertiesControl::AddLine( const ::rtl::OUString& sName, Any& rAny, bool bInteractive )
{
    m_aPropertiesWin.AddLine( sName, rAny );
    m_aVertScroll.SetRangeMax( m_aPropertiesWin.GetVisibleLineCount() + 1 );
	if ( bInteractive && m_aPropertiesWin.GetOutputSizePixel().Height() < m_aPropertiesWin.GetVisibleLineCount() * m_aPropertiesWin.GetLineHeight() )
		m_aVertScroll.DoScroll( m_aPropertiesWin.GetVisibleLineCount() + 1 );
}

// class SfxCustomPropertiesPage -----------------------------------------

SfxCustomPropertiesPage::SfxCustomPropertiesPage( Window* pParent, const SfxItemSet& rItemSet ) :

    SfxTabPage( pParent, SfxResId( TP_CUSTOMPROPERTIES ), rItemSet ),
    m_aPropertiesFT     ( this, SfxResId( FT_PROPERTIES ) ),
    m_aPropertiesCtrl   ( this, SfxResId( CTRL_PROPERTIES ) ),
    m_aAddBtn           ( this, SfxResId( BTN_ADD ) )

{
    FreeResource();

    m_aAddBtn.SetClickHdl( LINK( this, SfxCustomPropertiesPage, AddHdl ) );
}

IMPL_LINK( SfxCustomPropertiesPage, AddHdl, PushButton*, EMPTYARG )
{
    Any aAny;
    m_aPropertiesCtrl.AddLine( ::rtl::OUString(), aAny, true );
    return 0;
}

BOOL SfxCustomPropertiesPage::FillItemSet( SfxItemSet& rSet )
{
    BOOL bModified = FALSE;
    const SfxPoolItem*      pItem = NULL;
    SfxDocumentInfoItem*    pInfo = NULL;
    bool                    bMustDelete = false;

    if ( GetTabDialog() && GetTabDialog()->GetExampleSet() )
    {
        if( SFX_ITEM_SET !=
                GetTabDialog()->GetExampleSet()->GetItemState( SID_DOCINFO, TRUE, &pItem ) )
            pInfo = &( SfxDocumentInfoItem& )rSet.Get( SID_DOCINFO );
        else
        {
            bMustDelete = true;
            pInfo = new SfxDocumentInfoItem( *( const SfxDocumentInfoItem* ) pItem );
        }
    }

    if ( pInfo )
    {
        pInfo->ClearCustomProperties();
        Sequence< beans::PropertyValue > aPropertySeq = m_aPropertiesCtrl.GetCustomProperties();
        sal_Int32 i = 0, nCount = aPropertySeq.getLength();
        for ( ; i < nCount; ++i )
        {
            if ( aPropertySeq[i].Name.getLength() > 0 )
                pInfo->AddCustomProperty( aPropertySeq[i].Name, aPropertySeq[i].Value );
        }
    }

    bModified = TRUE; //!!!
    if ( bModified )
        rSet.Put( *pInfo );
    if ( bMustDelete )
        delete pInfo;
    return bModified;
}

void SfxCustomPropertiesPage::Reset( const SfxItemSet& rItemSet )
{
    m_aPropertiesCtrl.ClearAllLines();
    const SfxDocumentInfoItem* pInfoItem = &(const SfxDocumentInfoItem &)rItemSet.Get(SID_DOCINFO);
    std::vector< CustomProperty* > aCustomProps = pInfoItem->GetCustomProperties();
    for ( sal_uInt32 i = 0; i < aCustomProps.size(); i++ )
    {
        m_aPropertiesCtrl.AddLine( aCustomProps[i]->m_sName, aCustomProps[i]->m_aValue, false );
    }
}

int SfxCustomPropertiesPage::DeactivatePage( SfxItemSet* /*pSet*/ )
{
    int nRet = LEAVE_PAGE;
    if ( !m_aPropertiesCtrl.AreAllLinesValid() )
        nRet = KEEP_PAGE;
    return nRet;
}

SfxTabPage* SfxCustomPropertiesPage::Create( Window* pParent, const SfxItemSet& rItemSet )
{
    return new SfxCustomPropertiesPage( pParent, rItemSet );
}

