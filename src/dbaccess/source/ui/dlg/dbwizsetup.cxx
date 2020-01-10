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
#include "precompiled_dbaccess.hxx"

#include "dbwizsetup.hxx"
#include "dsmeta.hxx"
#include "DBSetupConnectionPages.hxx"
#include "dbadminsetup.hrc"
#include "dbu_dlg.hrc"
#include "dsitems.hxx"
#include "dsnItem.hxx"

#ifndef INCLUDED_SVTOOLS_PATHOPTIONS_HXX
#include <svtools/pathoptions.hxx>
#endif
#ifndef _SFXSTRITEM_HXX
#include <svtools/stritem.hxx>
#endif
#ifndef _SFXENUMITEM_HXX
#include <svtools/eitem.hxx>
#endif
#ifndef _SFXINTITEM_HXX
#include <svtools/intitem.hxx>
#endif
#ifndef _SV_MSGBOX_HXX
#include <vcl/msgbox.hxx>
#endif
#ifndef DBACCESS_SHARED_DBUSTRINGS_HRC
#include "dbustrings.hrc"
#endif
#ifndef _DBAUI_ADMINPAGES_HXX_
#include "adminpages.hxx"
#endif
#ifndef _SFX_DOCFILT_HACK_HXX
#include <sfx2/docfilt.hxx>
#endif
#ifndef _UNOTOOLS_UCBHELPER_HXX
#include <unotools/ucbhelper.hxx>
#endif
#ifndef _DBAUI_GENERALPAGE_HXX_
#include "generalpage.hxx"
#endif
#ifndef _DBAUI_LOCALRESACCESS_HXX_
#include "localresaccess.hxx"
#endif
#ifndef _DBAUI_STRINGLISTITEM_HXX_
#include "stringlistitem.hxx"
#endif
#ifndef _DBAUI_PROPERTYSETITEM_HXX_
#include "propertysetitem.hxx"
#endif
#ifndef _UNOTOOLS_CONFIGNODE_HXX_
#include <unotools/confignode.hxx>
#endif
#ifndef _DBAUI_DBADMINIMPL_HXX_
#include "DbAdminImpl.hxx"
#endif
#ifndef _DBA_DBACCESS_HELPID_HRC_
#include "dbaccess_helpid.hrc"
#endif
#ifndef DBAUI_CONNECTIONPAGESETUP_HXX
#include "ConnectionPageSetup.hxx"
#endif
#ifndef DBAUI_TOOLS_HXX
#include "UITools.hxx"
#endif
#ifndef _DBAUI_DBADMIN_HRC_
#include "dbadmin.hrc"
#endif
#ifndef DBAUI_ASYNCRONOUSLINK_HXX
#include "AsyncronousLink.hxx"
#endif
#ifndef _FILEDLGHELPER_HXX
#include <sfx2/filedlghelper.hxx>
#endif
#include <cppuhelper/exc_hlp.hxx>

/** === begin UNO includes === **/
#ifndef _COM_SUN_STAR_FRAME_XSTORABLE_HPP_
#include <com/sun/star/frame/XStorable.hpp>
#endif
#ifndef _COM_SUN_STAR_UNO_XNAMINGSERVICE_HPP_
#include <com/sun/star/uno/XNamingService.hpp>
#endif
#ifndef _COM_SUN_STAR_SDBCX_XTABLESSUPPLIER_HPP_
#include <com/sun/star/sdbcx/XTablesSupplier.hpp>
#endif
#ifndef _COM_SUN_STAR_SDBC_XDATASOURCE_HPP_
#include <com/sun/star/sdbc/XDataSource.hpp>
#endif
#ifndef _COM_SUN_STAR_CONTAINER_XNAMEACCESS_HPP_
#include <com/sun/star/container/XNameAccess.hpp>
#endif
#ifndef _COM_SUN_STAR_SDB_XDOCUMENTDATASOURCE_HPP_
#include <com/sun/star/sdb/XDocumentDataSource.hpp>
#endif
#ifndef _COM_SUN_STAR_FRAME_FRAMESEARCHFLAG_HPP_
#include <com/sun/star/frame/FrameSearchFlag.hpp>
#endif
#ifndef _COM_SUN_STAR_FRAME_XCOMPONENTLOADER_HPP_
#include <com/sun/star/frame/XComponentLoader.hpp>
#endif
#ifndef _COM_SUN_STAR_FRAME_XMODEL_HPP_
#include <com/sun/star/frame/XModel.hpp>
#endif
#ifndef _COM_SUN_STAR_UCB_XSIMPLEFILEACCESS_HPP_
#include <com/sun/star/ucb/XSimpleFileAccess.hpp>
#endif
#ifndef _COM_SUN_STAR_TASK_XJOBEXECUTOR_HPP_
#include <com/sun/star/task/XJobExecutor.hpp>
#endif
#ifndef _COM_SUN_STAR_LANG_XINITIALIZATION_HPP_
#include <com/sun/star/lang/XInitialization.hpp>
#endif
#ifndef _COM_SUN_STAR_SDB_COMMANDTYPE_HPP_
#include <com/sun/star/sdb/CommandType.hpp>
#endif
#ifndef _COM_SUN_STAR_UCB_INTERACTIVEIOEXCEPTION_HPP_
#include <com/sun/star/ucb/InteractiveIOException.hpp>
#endif
#ifndef _COM_SUN_STAR_IO_IOEXCEPTION_HPP_
#include <com/sun/star/io/IOException.hpp>
#endif
#ifndef _COM_SUN_STAR_FRAME_XTERMINATELISTENER_HPP_
#include <com/sun/star/frame/XTerminateListener.hpp>
#endif
#ifndef _COM_SUN_STAR_FRAME_XDESKTOP_HPP_
#include <com/sun/star/frame/XDesktop.hpp>
#endif
#ifndef _COM_SUN_STAR_SDBC_XDRIVERACCESS_HPP_ 
#include <com/sun/star/sdbc/XDriverAccess.hpp>
#endif
#ifndef _COM_SUN_STAR_DOCUMENT_MACROEXECMODE_HPP_
#include <com/sun/star/document/MacroExecMode.hpp>
#endif
#include <com/sun/star/ucb/IOErrorCode.hpp>
#include <com/sun/star/task/XInteractionHandler2.hpp>
#include <com/sun/star/ui/dialogs/TemplateDescription.hpp>


/** === end UNO includes === **/

#include <svtools/filenotation.hxx>
#include <comphelper/interaction.hxx>
#include <comphelper/namedvaluecollection.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <tools/diagnose_ex.h>
#include <connectivity/DriversConfig.hxx>

#include <memory>


//.........................................................................
namespace dbaui
{
//.........................................................................
using namespace dbtools;
using namespace svt;
using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::sdbc;
using namespace com::sun::star::sdbcx;
using namespace com::sun::star::task;
using namespace com::sun::star::lang;
using namespace com::sun::star::io;
using namespace com::sun::star::util;
using namespace com::sun::star::beans;
using namespace com::sun::star::container;
using namespace com::sun::star::frame;
using namespace com::sun::star::ucb;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::document;
using namespace ::comphelper;
using namespace ::cppu;

#define START_PAGE		0
#define CONNECTION_PAGE	1




//#define DBASE_PATH               1
//#define TEXT_PATH                2
//#define MSACCESS_PATH            3
//#define LDAP_PATH                4
//#define ADABAS_PATH              5
//#define ADO_PATH                 6
//#define JDBC_PATH                7
//#define ORACLE_PATH              8
//#define MYSQL_JDBC_PATH          9
//#define MYSQL_ODBC_PATH          10
//#define ODBC_PATH                11
//#define SPREADSHEET_PATH         12
//#define OUTLOOKEXP_PATH          13
//#define OUTLOOK_PATH             14
//#define MOZILLA_PATH             15
//#define EVOLUTION_PATH           16
//#define EVOLUTION_PATH_GROUPWISE 17
//#define EVOLUTION_PATH_LDAP      18
//#define KAB_PATH                 19
//#define MACAB_PATH            	 20
//#define THUNDERBIRD_PATH         21
//#define CREATENEW_PATH           22
//#define USERDEFINED_PATH         23
//#define OPEN_DOC_PATH            24
//#define MSACCESS2007_PATH        25
//#define MYSQL_NATIVE_PATH        26

OFinalDBPageSetup*          pFinalPage;



DBG_NAME(ODbTypeWizDialogSetup)
//=========================================================================
//= ODbTypeWizDialogSetup
//=========================================================================
//-------------------------------------------------------------------------
ODbTypeWizDialogSetup::ODbTypeWizDialogSetup(Window* _pParent
							   ,SfxItemSet* _pItems
							   ,const Reference< XMultiServiceFactory >& _rxORB
							   ,const ::com::sun::star::uno::Any& _aDataSourceName
							   )
	:svt::RoadmapWizard( _pParent, ModuleRes(DLG_DATABASE_WIZARD),
                        WZB_NEXT | WZB_PREVIOUS | WZB_FINISH | WZB_CANCEL | WZB_HELP )

    , m_pOutSet(NULL)
    , m_bResetting(sal_False)
	, m_bApplied(sal_False)
	, m_bUIEnabled( sal_True )
    , m_bIsConnectable( sal_False)
    , m_sRM_IntroText(ModuleRes(STR_PAGETITLE_INTROPAGE))
    , m_sRM_dBaseText(ModuleRes(STR_PAGETITLE_DBASE))
    , m_sRM_TextText(ModuleRes(STR_PAGETITLE_TEXT))
    , m_sRM_MSAccessText(ModuleRes(STR_PAGETITLE_MSACCESS))
    , m_sRM_LDAPText(ModuleRes(STR_PAGETITLE_LDAP))
    , m_sRM_ADABASText(ModuleRes(STR_PAGETITLE_ADABAS))
    , m_sRM_ADOText(ModuleRes(STR_PAGETITLE_ADO))
    , m_sRM_JDBCText(ModuleRes(STR_PAGETITLE_JDBC))
    , m_sRM_MySQLNativePageTitle(ModuleRes(STR_PAGETITLE_MYSQL_NATIVE))
    , m_pGeneralPage( NULL )
	, m_pMySQLIntroPage(NULL)
    , m_pCollection( NULL )
{
	DBG_CTOR(ODbTypeWizDialogSetup,NULL);
    // no local resources needed anymore
    m_sRM_MySQLText = String(ModuleRes(STR_PAGETITLE_MYSQL));
    m_sRM_OracleText = String(ModuleRes(STR_PAGETITLE_ORACLE));
    m_sRM_ODBCText = String(ModuleRes(STR_PAGETITLE_ODBC));
    m_sRM_SpreadSheetText = String(ModuleRes(STR_PAGETITLE_SPREADSHEET));
    m_sRM_AuthentificationText = String(ModuleRes(STR_PAGETITLE_AUTHENTIFICATION));
    m_sRM_FinalText = String(ModuleRes(STR_PAGETITLE_FINAL));
    m_sWorkPath = SvtPathOptions().GetWorkPath();
    pFinalPage = NULL;
	// extract the datasource type collection from the item set
	DbuTypeCollectionItem* pCollectionItem = PTR_CAST(DbuTypeCollectionItem, _pItems->GetItem(DSID_TYPECOLLECTION));
	if (pCollectionItem)
		m_pCollection = pCollectionItem->getCollection();

	DBG_ASSERT(m_pCollection, "ODbTypeWizDialogSetup::ODbTypeWizDialogSetup : really need a DSN type collection !");

	FreeResource();

	m_pImpl = ::std::auto_ptr<ODbDataSourceAdministrationHelper>(new ODbDataSourceAdministrationHelper(_rxORB,this,this));
	m_pImpl->setDataSourceOrName(_aDataSourceName);
	Reference< XPropertySet > xDatasource = m_pImpl->getCurrentDataSource();
	m_pOutSet = new SfxItemSet( *_pItems->GetPool(), _pItems->GetRanges() );

	m_pImpl->translateProperties(xDatasource, *m_pOutSet);
//	eType = m_pImpl->getDatasourceType(*m_pOutSet);

	SetPageSizePixel(LogicToPixel(::Size(WIZARD_PAGE_X, WIZARD_PAGE_Y), MAP_APPFONT));
	ShowButtonFixedLine(sal_True);
	defaultButton(WZB_NEXT);
	enableButtons(WZB_FINISH, sal_True);
    enableAutomaticNextButtonState();

    ::dbaccess::ODsnTypeCollection::TypeIterator aIter = m_pCollection->begin();
    ::dbaccess::ODsnTypeCollection::TypeIterator aEnd = m_pCollection->end();
    for(PathId i = 1;aIter != aEnd;++aIter,++i)
    {
        const ::rtl::OUString sURLPrefix = aIter.getURLPrefix();
        svt::RoadmapWizardTypes::WizardPath aPath;
        aPath.push_back(PAGE_DBSETUPWIZARD_INTRO);
        m_pCollection->fillPageIds(sURLPrefix,aPath);
        aPath.push_back(PAGE_DBSETUPWIZARD_AUTHENTIFICATION);
        aPath.push_back(PAGE_DBSETUPWIZARD_FINAL);
        
        declareAuthDepPath(sURLPrefix,i,aPath);
    }

    svt::RoadmapWizardTypes::WizardPath aPath;
    aPath.push_back(PAGE_DBSETUPWIZARD_INTRO);
    declarePath( static_cast<PathId>(m_pCollection->size()+1), aPath);

    m_pPrevPage->SetHelpId(HID_DBWIZ_PREVIOUS);
	m_pNextPage->SetHelpId(HID_DBWIZ_NEXT);
	m_pCancel->SetHelpId(HID_DBWIZ_CANCEL);
	m_pFinish->SetHelpId(HID_DBWIZ_FINISH);
	m_pHelp->SetUniqueId(UID_DBWIZ_HELP);
    SetRoadmapInteractive( sal_True );
	ActivatePage();
}

void ODbTypeWizDialogSetup::declareAuthDepPath( const ::rtl::OUString& _sURL, PathId _nPathId, const svt::RoadmapWizardTypes::WizardPath& _rPaths)
{
    bool bHasAuthentication = DataSourceMetaData::getAuthentication( _sURL ) != AuthNone;

    // collect the elements of the path
    WizardPath aPath;

    svt::RoadmapWizardTypes::WizardPath::const_iterator aIter = _rPaths.begin();
    svt::RoadmapWizardTypes::WizardPath::const_iterator aEnd = _rPaths.end();
    for(;aIter != aEnd;++aIter)
    {
        if ( bHasAuthentication || ( *aIter != PAGE_DBSETUPWIZARD_AUTHENTIFICATION ) )
            aPath.push_back( *aIter );
    } // for(;aIter != aEnd;++aIter)

    // call base method
    ::svt::RoadmapWizard::declarePath( _nPathId, aPath );
}

String ODbTypeWizDialogSetup::getStateDisplayName( WizardState _nState ) const
{
    String sRoadmapItem;
    switch( _nState )
    {
        case PAGE_DBSETUPWIZARD_INTRO:
            sRoadmapItem = m_sRM_IntroText;
            break;

        case PAGE_DBSETUPWIZARD_DBASE:
            sRoadmapItem = m_sRM_dBaseText;
            break;
        case PAGE_DBSETUPWIZARD_ADO:
            sRoadmapItem = m_sRM_ADOText;
            break;
        case PAGE_DBSETUPWIZARD_TEXT:
            sRoadmapItem = m_sRM_TextText;
            break;
        case PAGE_DBSETUPWIZARD_MSACCESS:
            sRoadmapItem = m_sRM_MSAccessText;
            break;
        case PAGE_DBSETUPWIZARD_LDAP:
            sRoadmapItem = m_sRM_LDAPText;
            break;
        case PAGE_DBSETUPWIZARD_ADABAS:
            sRoadmapItem = m_sRM_ADABASText;
            break;
        case PAGE_DBSETUPWIZARD_JDBC:
            sRoadmapItem = m_sRM_JDBCText;
            break;
        case PAGE_DBSETUPWIZARD_ORACLE:
            sRoadmapItem = m_sRM_OracleText;
            break;
        case PAGE_DBSETUPWIZARD_MYSQL_INTRO:
            sRoadmapItem = m_sRM_MySQLText;
            break;
        case PAGE_DBSETUPWIZARD_MYSQL_JDBC:
            sRoadmapItem = m_sRM_JDBCText;
            break;
        case PAGE_DBSETUPWIZARD_MYSQL_NATIVE:
            sRoadmapItem = m_sRM_MySQLNativePageTitle;
            break;
        case PAGE_DBSETUPWIZARD_MYSQL_ODBC:
            sRoadmapItem = m_sRM_ODBCText;
            break;
        case PAGE_DBSETUPWIZARD_ODBC:
            sRoadmapItem = m_sRM_ODBCText;
            break;
        case PAGE_DBSETUPWIZARD_SPREADSHEET:
            sRoadmapItem = m_sRM_SpreadSheetText;
            break;
        case PAGE_DBSETUPWIZARD_AUTHENTIFICATION:
            sRoadmapItem = m_sRM_AuthentificationText;
            break;
		case PAGE_DBSETUPWIZARD_USERDEFINED:
			{
				LocalResourceAccess aDummy(DLG_DATABASE_ADMINISTRATION, RSC_TABDIALOG);
				sRoadmapItem = String(ModuleRes(STR_PAGETITLE_CONNECTION));
			}
			break;
        case PAGE_DBSETUPWIZARD_FINAL:
            sRoadmapItem = m_sRM_FinalText;
            break;
        default:
            break;
    }
    return sRoadmapItem;
}

//-------------------------------------------------------------------------
ODbTypeWizDialogSetup::~ODbTypeWizDialogSetup()
{
	delete m_pOutSet;
	DBG_DTOR(ODbTypeWizDialogSetup,NULL);
}

//-------------------------------------------------------------------------
IMPL_LINK(ODbTypeWizDialogSetup, OnTypeSelected, OGeneralPage*, /*_pTabPage*/)
{
    activateDatabasePath();
	return 1L;
}

void lcl_removeUnused(const ::comphelper::NamedValueCollection& _aOld,const ::comphelper::NamedValueCollection& _aNew,::comphelper::NamedValueCollection& _rDSInfo)
{
    _rDSInfo.merge(_aNew,true);
    uno::Sequence< beans::NamedValue > aOldValues = _aOld.getNamedValues();
    const beans::NamedValue* pIter = aOldValues.getConstArray();
    const beans::NamedValue* pEnd  = pIter + aOldValues.getLength();
    for(;pIter != pEnd;++pIter)
    {
        if ( !_aNew.has(pIter->Name) )
        {
            _rDSInfo.remove(pIter->Name);
        }
    }
}
// -----------------------------------------------------------------------------
void DataSourceInfoConverter::convert(const ::dbaccess::ODsnTypeCollection* _pCollection,const ::rtl::OUString& _sOldURLPrefix,const ::rtl::OUString& _sNewURLPrefix,const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& _xDatasource)
{
    if ( _pCollection->getPrefix(_sOldURLPrefix) == _pCollection->getPrefix(_sNewURLPrefix) )
        return ;
    uno::Sequence< beans::PropertyValue> aInfo;
    _xDatasource->getPropertyValue(PROPERTY_INFO) >>= aInfo;
    ::comphelper::NamedValueCollection aDS(aInfo);

    ::connectivity::DriversConfig aDriverConfig(m_xFactory);

    const ::comphelper::NamedValueCollection&  aOldProperties   = aDriverConfig.getProperties(_sOldURLPrefix);
    const ::comphelper::NamedValueCollection&  aNewProperties   = aDriverConfig.getProperties(_sNewURLPrefix);
    lcl_removeUnused(aOldProperties,aNewProperties,aDS);

    aDS >>= aInfo;
    _xDatasource->setPropertyValue(PROPERTY_INFO,uno::makeAny(aInfo));
}
//-------------------------------------------------------------------------
void ODbTypeWizDialogSetup::activateDatabasePath()
{
    switch ( m_pGeneralPage->GetDatabaseCreationMode() )
    {
    case OGeneralPage::eCreateNew:
    {
        sal_Int32 nCreateNewDBIndex = m_pCollection->getIndexOf( m_pCollection->getEmbeddedDatabase() );
        if ( nCreateNewDBIndex == -1 )
            nCreateNewDBIndex = m_pCollection->getIndexOf( ::rtl::OUString::createFromAscii( "sdbc:dbase:" ) );
        OSL_ENSURE( nCreateNewDBIndex != -1, "ODbTypeWizDialogSetup::activateDatabasePath: the GeneralPage should have prevented this!" );
        activatePath( static_cast< PathId >( nCreateNewDBIndex + 1 ), sal_True );

        enableState(PAGE_DBSETUPWIZARD_FINAL, sal_True );
		enableButtons( WZB_FINISH, sal_True);
    }
    break;
    case OGeneralPage::eConnectExternal:
    {
        ::rtl::OUString sOld = m_sURL;
        DataSourceInfoConverter aConverter(getORB());
        m_sURL = m_pGeneralPage->GetSelectedType();
        aConverter.convert(m_pCollection,sOld,m_sURL,m_pImpl->getCurrentDataSource());
		::dbaccess::DATASOURCE_TYPE eType = VerifyDataSourceType(m_pCollection->determineType(m_sURL));
        if (eType ==  ::dbaccess::DST_UNKNOWN)
            eType = m_pCollection->determineType(m_sOldURL);

		activatePath( static_cast<PathId>(m_pCollection->getIndexOf(m_sURL) + 1), sal_True);
        updateTypeDependentStates();
    }
    break;
    case OGeneralPage::eOpenExisting:
    {
        activatePath( static_cast<PathId>(m_pCollection->size() + 1), sal_True );
        enableButtons( WZB_FINISH, m_pGeneralPage->GetSelectedDocument().sURL.Len() != 0 );
    }
    break;
    default:
        DBG_ERROR( "ODbTypeWizDialogSetup::activateDatabasePath: unknown creation mode!" );
    }

    enableButtons( WZB_NEXT, m_pGeneralPage->GetDatabaseCreationMode() != OGeneralPage::eOpenExisting );
        // TODO: this should go into the base class. Point is, we activate a path whose *last*
        // step is also the current one. The base class should automatically disable
        // the Next button in such a case. However, not for this patch ...
}

//-------------------------------------------------------------------------
void ODbTypeWizDialogSetup::updateTypeDependentStates()
{
    sal_Bool bDoEnable = sal_False;
    sal_Bool bIsConnectionRequired = IsConnectionUrlRequired();
    if (!bIsConnectionRequired)
    {
        bDoEnable = sal_True;
    }
    else if ( m_sURL == m_sOldURL )
    {
        bDoEnable = m_bIsConnectable;
    }
    enableState(PAGE_DBSETUPWIZARD_AUTHENTIFICATION, bDoEnable);
    enableState(PAGE_DBSETUPWIZARD_FINAL, bDoEnable );
	enableButtons( WZB_FINISH, bDoEnable);
}


//-------------------------------------------------------------------------
sal_Bool ODbTypeWizDialogSetup::IsConnectionUrlRequired()
{
	return m_pCollection->isConnectionUrlRequired(m_sURL);
}

//-------------------------------------------------------------------------
void ODbTypeWizDialogSetup::resetPages(const Reference< XPropertySet >& _rxDatasource)
{
	// remove all items which relate to indirect properties from the input set
	// (without this, the following may happen: select an arbitrary data source where some indirect properties
	// are set. Select another data source of the same type, where the indirect props are not set (yet). Then,
	// the indirect property values of the first ds are shown in the second ds ...)
	const ODbDataSourceAdministrationHelper::MapInt2String& rMap = m_pImpl->getIndirectProperties();
	for	(	ODbDataSourceAdministrationHelper::ConstMapInt2StringIterator aIndirect = rMap.begin();
			aIndirect != rMap.end();
			++aIndirect
		)
		getWriteOutputSet()->ClearItem( (sal_uInt16)aIndirect->first );

	// extract all relevant data from the property set of the data source
	m_pImpl->translateProperties(_rxDatasource, *getWriteOutputSet());
}
// -----------------------------------------------------------------------------
const SfxItemSet* ODbTypeWizDialogSetup::getOutputSet() const
{
	return m_pOutSet;
}
// -----------------------------------------------------------------------------
SfxItemSet* ODbTypeWizDialogSetup::getWriteOutputSet()
{
	return m_pOutSet;
}
// -----------------------------------------------------------------------------
::std::pair< Reference<XConnection>,sal_Bool> ODbTypeWizDialogSetup::createConnection()
{
	return m_pImpl->createConnection();
}
// -----------------------------------------------------------------------------
Reference< XMultiServiceFactory > ODbTypeWizDialogSetup::getORB() const
{
	return m_pImpl->getORB();
}
// -----------------------------------------------------------------------------
Reference< XDriver > ODbTypeWizDialogSetup::getDriver()
{
	return m_pImpl->getDriver();
}


::dbaccess::DATASOURCE_TYPE ODbTypeWizDialogSetup::VerifyDataSourceType(const ::dbaccess::DATASOURCE_TYPE _DatabaseType) const
{
	::dbaccess::DATASOURCE_TYPE LocDatabaseType = _DatabaseType;
	if ((LocDatabaseType ==  ::dbaccess::DST_MYSQL_JDBC) || (LocDatabaseType ==  ::dbaccess::DST_MYSQL_ODBC) || (LocDatabaseType ==  ::dbaccess::DST_MYSQL_NATIVE))
	{
		if (m_pMySQLIntroPage != NULL)
		{
			switch( m_pMySQLIntroPage->getMySQLMode() )	
            {
                case OMySQLIntroPageSetup::VIA_JDBC:
				    return  ::dbaccess::DST_MYSQL_JDBC;
                case OMySQLIntroPageSetup::VIA_NATIVE:
				    return  ::dbaccess::DST_MYSQL_NATIVE;
                case OMySQLIntroPageSetup::VIA_ODBC:
				    return  ::dbaccess::DST_MYSQL_ODBC;
            }
		}
	}
	return LocDatabaseType;
}



// -----------------------------------------------------------------------------
::rtl::OUString	ODbTypeWizDialogSetup::getDatasourceType(const SfxItemSet& _rSet) const
{
    ::rtl::OUString sRet = m_pImpl->getDatasourceType(_rSet);
    if (m_pMySQLIntroPage != NULL && m_pMySQLIntroPage->IsVisible() )
	{
		switch( m_pMySQLIntroPage->getMySQLMode() )	
        {
            case OMySQLIntroPageSetup::VIA_JDBC:
                sRet = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("sdbc:mysql:jdbc:"));
                break;
            case OMySQLIntroPageSetup::VIA_NATIVE:
			    sRet = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("sdbc:mysql:mysqlc:"));
                break;
            case OMySQLIntroPageSetup::VIA_ODBC:
			    sRet = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("sdbc:mysql:odbc:"));
                break;
        }
	}
	return sRet;
}

// -----------------------------------------------------------------------------
void ODbTypeWizDialogSetup::clearPassword()
{
	m_pImpl->clearPassword();
}

// -----------------------------------------------------------------------------
TabPage* ODbTypeWizDialogSetup::createPage(WizardState _nState)
{
    SfxTabPage* pFirstPage;
	OGenericAdministrationPage* pPage = NULL;
	switch(_nState)
	{
        case PAGE_DBSETUPWIZARD_INTRO:
			pFirstPage = OGeneralPage::Create(this,*m_pOutSet, sal_True);
			pPage = static_cast<OGenericAdministrationPage*> (pFirstPage);
			m_pGeneralPage = static_cast<OGeneralPage*>(pFirstPage);
            m_pGeneralPage->SetTypeSelectHandler(LINK(this, ODbTypeWizDialogSetup, OnTypeSelected));
            m_pGeneralPage->SetCreationModeHandler(LINK( this, ODbTypeWizDialogSetup, OnChangeCreationMode ) );
            m_pGeneralPage->SetDocumentSelectionHandler(LINK( this, ODbTypeWizDialogSetup, OnRecentDocumentSelected ) );
            m_pGeneralPage->SetChooseDocumentHandler(LINK( this, ODbTypeWizDialogSetup, OnSingleDocumentChosen ) );
			break;

		case PAGE_DBSETUPWIZARD_DBASE:
			pPage = OConnectionTabPageSetup::CreateDbaseTabPage(this,*m_pOutSet);
			break;

		case PAGE_DBSETUPWIZARD_ADO:
			pPage = OConnectionTabPageSetup::CreateADOTabPage( this, *m_pOutSet);
			break;

		case PAGE_DBSETUPWIZARD_TEXT:
            pPage = OTextConnectionPageSetup::CreateTextTabPage(this,*m_pOutSet);
			break;

		case PAGE_DBSETUPWIZARD_ODBC:
			pPage = OConnectionTabPageSetup::CreateODBCTabPage( this, *m_pOutSet);
			break;

        case PAGE_DBSETUPWIZARD_JDBC:
			pPage = OJDBCConnectionPageSetup::CreateJDBCTabPage( this, *m_pOutSet);
			break;

		case PAGE_DBSETUPWIZARD_MYSQL_ODBC:
			m_pOutSet->Put(SfxStringItem(DSID_CONNECTURL, m_pCollection->getPrefix(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("sdbc:mysql:odbc:")))));
			pPage = OConnectionTabPageSetup::CreateODBCTabPage( this, *m_pOutSet);
            break;

		case PAGE_DBSETUPWIZARD_MYSQL_JDBC:
			m_pOutSet->Put(SfxStringItem(DSID_CONNECTURL, m_pCollection->getPrefix(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("sdbc:mysql:jdbc:")))));
            pPage = OGeneralSpecialJDBCConnectionPageSetup::CreateMySQLJDBCTabPage( this, *m_pOutSet);
			break;
        case PAGE_DBSETUPWIZARD_MYSQL_NATIVE:
			m_pOutSet->Put(SfxStringItem(DSID_CONNECTURL, m_pCollection->getPrefix(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("sdbc:mysql:mysqlc:")))));
            pPage = MySQLNativeSetupPage::Create( this, *m_pOutSet);
			break;

		case PAGE_DBSETUPWIZARD_ORACLE:
			pPage = OGeneralSpecialJDBCConnectionPageSetup::CreateOracleJDBCTabPage( this, *m_pOutSet);
			break;

		case PAGE_DBSETUPWIZARD_ADABAS:
			pPage = OConnectionTabPageSetup::CreateAdabasTabPage( this, *m_pOutSet);
			break;

		case PAGE_DBSETUPWIZARD_LDAP	:
			pPage = OLDAPConnectionPageSetup::CreateLDAPTabPage(this,*m_pOutSet);
            break;

        case PAGE_DBSETUPWIZARD_SPREADSHEET:	/// first user defined driver
			pPage = OSpreadSheetConnectionPageSetup::CreateSpreadSheetTabPage(this,*m_pOutSet);
            break;

        case PAGE_DBSETUPWIZARD_MSACCESS:
            pPage  = OConnectionTabPageSetup::CreateMSAccessTabPage(this,*m_pOutSet);
            break;
        case PAGE_DBSETUPWIZARD_MYSQL_INTRO:
            m_pMySQLIntroPage = OMySQLIntroPageSetup::CreateMySQLIntroTabPage(this,*m_pOutSet);
            m_pMySQLIntroPage->SetClickHdl(LINK( this, ODbTypeWizDialogSetup, ImplClickHdl ) );
            pPage = m_pMySQLIntroPage;
            break;

        case PAGE_DBSETUPWIZARD_AUTHENTIFICATION:
            pPage = OAuthentificationPageSetup::CreateAuthentificationTabPage(this,*m_pOutSet);
            break;

		case PAGE_DBSETUPWIZARD_USERDEFINED:
			pPage = OConnectionTabPageSetup::CreateUserDefinedTabPage(this,*m_pOutSet);
			break;

        case PAGE_DBSETUPWIZARD_FINAL:
            pPage = OFinalDBPageSetup::CreateFinalDBTabPageSetup(this,*m_pOutSet);
            pFinalPage = static_cast<OFinalDBPageSetup*> (pPage);
            break;
	}

    if ((_nState != PAGE_DBSETUPWIZARD_INTRO) && (_nState != PAGE_DBSETUPWIZARD_AUTHENTIFICATION))
    {
        pPage->SetModifiedHandler(LINK( this, ODbTypeWizDialogSetup, ImplModifiedHdl ) );
    }

    if ( pPage )
	{
		pPage->SetServiceFactory(m_pImpl->getORB());
		pPage->SetAdminDialog(this, this);

        defaultButton( _nState == PAGE_DBSETUPWIZARD_FINAL ? WZB_FINISH : WZB_NEXT );
		enableButtons( WZB_FINISH, _nState == PAGE_DBSETUPWIZARD_FINAL );
		enableButtons( WZB_NEXT, _nState == PAGE_DBSETUPWIZARD_FINAL ? sal_False : sal_True);
        pPage->Show();
	}
	return pPage;
}


IMPL_LINK(ODbTypeWizDialogSetup, ImplModifiedHdl, OGenericAdministrationPage*, _pConnectionPageSetup)
{
    m_bIsConnectable = _pConnectionPageSetup->GetRoadmapStateValue( );
    enableState(PAGE_DBSETUPWIZARD_FINAL, m_bIsConnectable);
    enableState(PAGE_DBSETUPWIZARD_AUTHENTIFICATION, m_bIsConnectable);
	if (getCurrentState() == PAGE_DBSETUPWIZARD_FINAL)
		enableButtons( WZB_FINISH, sal_True);
	else
		enableButtons( WZB_FINISH, m_bIsConnectable);
	enableButtons( WZB_NEXT, m_bIsConnectable  && (getCurrentState() != PAGE_DBSETUPWIZARD_FINAL));
    return sal_True;
}


// -----------------------------------------------------------------------------
IMPL_LINK(ODbTypeWizDialogSetup, ImplClickHdl, OMySQLIntroPageSetup*, _pMySQLIntroPageSetup)
{
    ::rtl::OUString sURLPrefix;
    switch( _pMySQLIntroPageSetup->getMySQLMode() )
    {
        case  OMySQLIntroPageSetup::VIA_ODBC:
            sURLPrefix = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("sdbc:mysql:odbc:"));
            break;
        case  OMySQLIntroPageSetup::VIA_JDBC:
            sURLPrefix = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("sdbc:mysql:jdbc:"));
            break;
        case  OMySQLIntroPageSetup::VIA_NATIVE:
    		sURLPrefix = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("sdbc:mysql:mysqlc:"));
            break;
    } // switch( _pMySQLIntroPageSetup->getMySQLMode() )
    activatePath( static_cast<PathId>(m_pCollection->getIndexOf(sURLPrefix) + 1), sal_True);
    return sal_True;
}

// -----------------------------------------------------------------------------
IMPL_LINK(ODbTypeWizDialogSetup, OnChangeCreationMode, OGeneralPage*, /*_pGeneralPage*/)
{
    activateDatabasePath();
    return sal_True;
}

// -----------------------------------------------------------------------------
IMPL_LINK(ODbTypeWizDialogSetup, OnRecentDocumentSelected, OGeneralPage*, /*_pGeneralPage*/)
{
    enableButtons( WZB_FINISH, m_pGeneralPage->GetSelectedDocument().sURL.Len() != 0 );
    return 0L;
}

// -----------------------------------------------------------------------------
IMPL_LINK(ODbTypeWizDialogSetup, OnSingleDocumentChosen, OGeneralPage*, /*_pGeneralPage*/)
{
    if ( prepareLeaveCurrentState( eFinish ) )
        onFinish( RET_OK );
    return 0L;
}

// -----------------------------------------------------------------------------
void ODbTypeWizDialogSetup::enterState(WizardState _nState)
{
    m_sURL = m_pImpl->getDatasourceType(*m_pOutSet);
    RoadmapWizard::enterState(_nState);
	switch(_nState)
	{
		case PAGE_DBSETUPWIZARD_INTRO:
			m_sOldURL = m_sURL;
			break;
        case PAGE_DBSETUPWIZARD_FINAL:
            enableButtons( WZB_FINISH, sal_True);
            if ( pFinalPage )
                pFinalPage->enableTableWizardCheckBox(m_pCollection->supportsTableCreation(m_sURL));
            break;
    }
}

//-------------------------------------------------------------------------
sal_Bool ODbTypeWizDialogSetup::saveDatasource()
{
	SfxTabPage* pPage = static_cast<SfxTabPage*>(WizardDialog::GetPage(getCurrentState()));
	if ( pPage )
		pPage->FillItemSet(*m_pOutSet);
	return sal_True;
}


// -----------------------------------------------------------------------------
sal_Bool ODbTypeWizDialogSetup::leaveState(WizardState _nState)
{
    if (_nState == PAGE_DBSETUPWIZARD_MYSQL_INTRO)
        return sal_True;
    if ( _nState == PAGE_DBSETUPWIZARD_INTRO && m_sURL != m_sOldURL )
    {
        resetPages(m_pImpl->getCurrentDataSource());
    }
	SfxTabPage* pPage = static_cast<SfxTabPage*>(WizardDialog::GetPage(_nState));
	return pPage && pPage->DeactivatePage(m_pOutSet) != 0;
}

// -----------------------------------------------------------------------------
void ODbTypeWizDialogSetup::setTitle(const ::rtl::OUString& /*_sTitle*/)
{
    DBG_ERROR( "ODbTypeWizDialogSetup::setTitle: not implemented!" );
        // why?
}

//-------------------------------------------------------------------------
void ODbTypeWizDialogSetup::enableConfirmSettings( bool _bEnable )
{
    (void)_bEnable;
}

//-------------------------------------------------------------------------
namespace
{
    bool lcl_handle( const Reference< XInteractionHandler2 >& _rxHandler, const Any& _rRequest )
    {
	    OInteractionRequest* pRequest = new OInteractionRequest( _rRequest );
	    Reference < XInteractionRequest > xRequest( pRequest );
	    OInteractionAbort* pAbort = new OInteractionAbort;
	    pRequest->addContinuation( pAbort );
		
        return _rxHandler->handleInteractionRequest( xRequest );
    }
}

//-------------------------------------------------------------------------
sal_Bool ODbTypeWizDialogSetup::SaveDatabaseDocument()
{
	Reference< XInteractionHandler2 > xHandler( getORB()->createInstance( SERVICE_TASK_INTERACTION_HANDLER ), UNO_QUERY );
	try
	{
		if (callSaveAsDialog() == sal_True)
		{
			m_pImpl->saveChanges(*m_pOutSet);
			Reference< XPropertySet > xDatasource = m_pImpl->getCurrentDataSource();
            Reference< XModel > xModel( getDataSourceOrModel( xDatasource ), UNO_QUERY_THROW );
            Reference< XStorable > xStore( xModel, UNO_QUERY_THROW );

            if ( m_pGeneralPage->GetDatabaseCreationMode() == OGeneralPage::eCreateNew )
				CreateDatabase();

            ::comphelper::NamedValueCollection aArgs( xModel->getArgs() );
            aArgs.put( "Overwrite", sal_Bool( sal_True ) );
            aArgs.put( "InteractionHandler", xHandler );
            aArgs.put( "MacroExecutionMode", MacroExecMode::USE_CONFIG );

            ::rtl::OUString sPath = m_pImpl->getDocumentUrl( *m_pOutSet );
			xStore->storeAsURL( sPath, aArgs.getPropertyValues() );

			if ( !pFinalPage || pFinalPage->IsDatabaseDocumentToBeRegistered() )
				RegisterDataSourceByLocation( sPath );

            return sal_True;
		}
	}
	catch ( const Exception& e )
	{
        Any aError = ::cppu::getCaughtException();
		if ( xHandler.is() )
        {
            if ( !lcl_handle( xHandler, aError ) )
            {
	            InteractiveIOException aRequest;
                aRequest.Classification = InteractionClassification_ERROR;
                if ( aError.isExtractableTo( ::cppu::UnoType< IOException >::get() ) )
                    // assume savint the document faile
		            aRequest.Code = IOErrorCode_CANT_WRITE;
                else
                    aRequest.Code = IOErrorCode_GENERAL;
                aRequest.Message = e.Message;
                aRequest.Context = e.Context;
                lcl_handle( xHandler, makeAny( aRequest ) );
            }
        }
	}
    return sal_False;
}
    // ------------------------------------------------------------------------
    sal_Bool ODbTypeWizDialogSetup::IsDatabaseDocumentToBeOpened() const
    {
        if ( m_pGeneralPage->GetDatabaseCreationMode() == OGeneralPage::eOpenExisting )
            return sal_True;

        if ( pFinalPage != NULL )
            return pFinalPage->IsDatabaseDocumentToBeOpened();

        return sal_True;
    }

    // ------------------------------------------------------------------------
    sal_Bool ODbTypeWizDialogSetup::IsTableWizardToBeStarted() const
    {
        if ( m_pGeneralPage->GetDatabaseCreationMode() == OGeneralPage::eOpenExisting )
            return sal_False;

        if ( pFinalPage != NULL )
            return pFinalPage->IsTableWizardToBeStarted();

        return sal_False;
    }

    //-------------------------------------------------------------------------
    ::rtl::OUString ODbTypeWizDialogSetup::getDefaultDatabaseType() const
    {
        ::rtl::OUString sEmbeddedURL = m_pCollection->getEmbeddedDatabase();
        ::connectivity::DriversConfig aDriverConfig(getORB());
        if ( !aDriverConfig.getDriverFactoryName(sEmbeddedURL).getLength() )
            sEmbeddedURL = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("sdbc:dbase:"));

        return sEmbeddedURL;
    }

    //-------------------------------------------------------------------------
    void ODbTypeWizDialogSetup::CreateDatabase()
    {
		::rtl::OUString sUrl;
		::rtl::OUString eType = getDefaultDatabaseType();
		if ( m_pCollection->isEmbeddedDatabase(eType) )
		{
			sUrl = eType;
			Reference< XPropertySet > xDatasource = m_pImpl->getCurrentDataSource();
			OSL_ENSURE(xDatasource.is(),"DataSource is null!");
			if ( xDatasource.is() )
				xDatasource->setPropertyValue( PROPERTY_INFO, makeAny( m_pCollection->getDefaultDBSettings( eType ) ) );
			m_pImpl->translateProperties(xDatasource,*m_pOutSet);
		}
		else if ( m_pCollection->isFileSystemBased(eType) )
		{
			Reference< XSimpleFileAccess > xSimpleFileAccess(getORB()->createInstance(::rtl::OUString::createFromAscii( "com.sun.star.ucb.SimpleFileAccess" )), UNO_QUERY);
			INetURLObject aDBPathURL(m_sWorkPath);
			aDBPathURL.Append(m_aDocURL.getBase());
			createUniqueFolderName(&aDBPathURL);
			::rtl::OUString sPrefix = eType;
			sUrl = aDBPathURL.GetMainURL( INetURLObject::NO_DECODE);
			xSimpleFileAccess->createFolder(sUrl);
			//OFileNotation aFileNotation(sUrl);
			//sUrl = aFileNotation.get(OFileNotation::N_SYSTEM);
 			sUrl = sPrefix.concat(sUrl);
		}
		m_pOutSet->Put(SfxStringItem(DSID_CONNECTURL, sUrl));
		m_pImpl->saveChanges(*m_pOutSet);
    }

    //-------------------------------------------------------------------------
    void ODbTypeWizDialogSetup::RegisterDataSourceByLocation(const ::rtl::OUString& _sPath)
    {
	    Reference< XPropertySet > xDatasource = m_pImpl->getCurrentDataSource();
        Reference< XNamingService > xDatabaseContext(getORB()->createInstance(SERVICE_SDB_DATABASECONTEXT), UNO_QUERY);
        Reference< XNameAccess > xNameAccessDatabaseContext(xDatabaseContext, UNO_QUERY);
		INetURLObject aURL( _sPath );
        ::rtl::OUString sFilename = aURL.getBase( INetURLObject::LAST_SEGMENT, true, INetURLObject::DECODE_WITH_CHARSET );
        ::rtl::OUString sDatabaseName = ::dbtools::createUniqueName(xNameAccessDatabaseContext, sFilename,sal_False);
        xDatabaseContext->registerObject(sDatabaseName, xDatasource);
    }


    //-------------------------------------------------------------------------
    sal_Bool ODbTypeWizDialogSetup::callSaveAsDialog()
    {
        sal_Bool bRet = sal_False;
		WinBits nBits(WB_STDMODAL|WB_SAVEAS);
        ::sfx2::FileDialogHelper aFileDlg( com::sun::star::ui::dialogs::TemplateDescription::FILESAVE_AUTOEXTENSION, static_cast<sal_uInt32>(nBits), this);
		const SfxFilter* pFilter = getStandardDatabaseFilter();
		if ( pFilter )
		{
            INetURLObject aWorkURL( m_sWorkPath );
            aFileDlg.SetDisplayFolder( aWorkURL.GetMainURL( INetURLObject::NO_DECODE ));

            ::rtl::OUString sDefaultName = String( ModuleRes( STR_DATABASEDEFAULTNAME ) );
            ::rtl::OUString sExtension = pFilter->GetDefaultExtension();
            sDefaultName += sExtension.replaceAt( 0, 1, ::rtl::OUString() );
            aWorkURL.Append( sDefaultName );
			sDefaultName = createUniqueFileName( aWorkURL );
            aFileDlg.SetFileName( sDefaultName );

            aFileDlg.AddFilter(pFilter->GetUIName(),pFilter->GetDefaultExtension());
			aFileDlg.SetCurrentFilter(pFilter->GetUIName());
		}
		if ( aFileDlg.Execute() == ERRCODE_NONE )
		{
            m_aDocURL = INetURLObject(aFileDlg.GetPath());

            if( m_aDocURL.GetProtocol() != INET_PROT_NOT_VALID )
			{
				::rtl::OUString sFileName = m_aDocURL.GetMainURL( INetURLObject::NO_DECODE );
				if ( ::utl::UCBContentHelper::IsDocument(sFileName) )
					::utl::UCBContentHelper::Kill(sFileName);
                m_pOutSet->Put(SfxStringItem(DSID_DOCUMENT_URL, sFileName));
                bRet = sal_True;
			}
		}
        return bRet;
    }

    //-------------------------------------------------------------------------
    void ODbTypeWizDialogSetup::createUniqueFolderName(INetURLObject* pURL)
    {
        Reference< XSimpleFileAccess > xSimpleFileAccess(getORB()->createInstance(::rtl::OUString::createFromAscii( "com.sun.star.ucb.SimpleFileAccess" )), UNO_QUERY);
        :: rtl::OUString sLastSegmentName = pURL->getName();
        sal_Bool bFolderExists = sal_True;
		sal_Int32 i = 1;
        while (bFolderExists == sal_True)
        {
            bFolderExists = xSimpleFileAccess->isFolder(pURL->GetMainURL( INetURLObject::NO_DECODE ));
			if (bFolderExists == sal_True)
            {
		        i++;
                pURL->setName(sLastSegmentName.concat(::rtl::OUString::valueOf(i)));
			}
        }
    }

    //-------------------------------------------------------------------------
    String ODbTypeWizDialogSetup::createUniqueFileName(const INetURLObject& _rURL)
    {
        Reference< XSimpleFileAccess > xSimpleFileAccess(getORB()->createInstance(::rtl::OUString::createFromAscii( "com.sun.star.ucb.SimpleFileAccess" )), UNO_QUERY);
        :: rtl::OUString sFilename = _rURL.getName();
        ::rtl::OUString BaseName = _rURL.getBase();
        ::rtl::OUString sExtension = _rURL.getExtension();

        sal_Bool bElementExists = sal_True;

        INetURLObject aExistenceCheck( _rURL );
        for ( sal_Int32 i = 1; bElementExists; )
        {
            bElementExists = xSimpleFileAccess->exists( aExistenceCheck.GetMainURL( INetURLObject::NO_DECODE ) );
			if ( bElementExists )
            {
                aExistenceCheck.setBase( BaseName.concat( ::rtl::OUString::valueOf( i ) ) );
                ++i;
			}
        }
        return aExistenceCheck.getName( INetURLObject::LAST_SEGMENT, true, INetURLObject::DECODE_WITH_CHARSET );
    }
    // -----------------------------------------------------------------------------
    IWizardPage* ODbTypeWizDialogSetup::getWizardPage(TabPage* _pCurrentPage) const
    {
	    OGenericAdministrationPage* pPage = static_cast<OGenericAdministrationPage*>(_pCurrentPage);
	    return pPage;
    }

    // -----------------------------------------------------------------------------
    namespace
    {
        // .............................................................................
        typedef ::cppu::WeakImplHelper1 <   XTerminateListener
                                        >   AsyncLoader_Base;
        class AsyncLoader : public AsyncLoader_Base
        {
        private:
            Reference< XComponentLoader >       m_xFrameLoader;
            Reference< XDesktop >               m_xDesktop;
            Reference< XInteractionHandler >    m_xInteractionHandler;
            ::rtl::OUString                     m_sURL;
            OAsyncronousLink                    m_aAsyncCaller;

        public:
            AsyncLoader( const Reference< XMultiServiceFactory >& _rxORB, const ::rtl::OUString& _rURL );

            void doLoadAsync();

            // XTerminateListener
            virtual void SAL_CALL queryTermination( const EventObject& Event ) throw (TerminationVetoException, RuntimeException);
            virtual void SAL_CALL notifyTermination( const EventObject& Event ) throw (RuntimeException);
            // XEventListener
            virtual void SAL_CALL disposing( const ::com::sun::star::lang::EventObject& Source ) throw (::com::sun::star::uno::RuntimeException);

        private:
            DECL_LINK( OnOpenDocument, void* );
        };

        // .............................................................................
        AsyncLoader::AsyncLoader( const Reference< XMultiServiceFactory >& _rxORB, const ::rtl::OUString& _rURL )
            :m_sURL( _rURL )
            ,m_aAsyncCaller( LINK( this, AsyncLoader, OnOpenDocument ) )
        {
            try
            {
                m_xDesktop.set( _rxORB->createInstance( SERVICE_FRAME_DESKTOP ), UNO_QUERY_THROW );
                m_xFrameLoader.set( m_xDesktop, UNO_QUERY_THROW );
                m_xInteractionHandler.set(
                    _rxORB->createInstance(
                        ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.sdb.InteractionHandler" ) )
                    ),
                    UNO_QUERY_THROW );
            }
            catch( const Exception& )
            {
            	DBG_UNHANDLED_EXCEPTION();
            }
        }

        // .............................................................................
        void AsyncLoader::doLoadAsync()
        {
            OSL_ENSURE( !m_aAsyncCaller.IsRunning(), "AsyncLoader:doLoadAsync: already running!" );

            acquire();
            try
            {
                if ( m_xDesktop.is() )
                    m_xDesktop->addTerminateListener( this );
            }
            catch( const Exception& ) { DBG_UNHANDLED_EXCEPTION(); }

            m_aAsyncCaller.Call( NULL );
        }

        // .............................................................................
        IMPL_LINK( AsyncLoader, OnOpenDocument, void*, /*_pEmptyArg*/ )
        {
            try
            {
                if ( m_xFrameLoader.is() )
                {
                    ::comphelper::NamedValueCollection aLoadArgs;
                    aLoadArgs.put( "InteractionHandler", m_xInteractionHandler );
                    aLoadArgs.put( "MacroExecutionMode", MacroExecMode::USE_CONFIG );

                    Sequence< PropertyValue > aLoadArgPV;
                    aLoadArgs >>= aLoadArgPV;

                    m_xFrameLoader->loadComponentFromURL( m_sURL,
                        ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "_default" ) ),
                        FrameSearchFlag::ALL,
                        aLoadArgPV
                    );
                }
            }
            catch( const Exception& )
            {
                // do not assert.
                // Such an exception happens for instance of the to-be-loaded document does not exist anymore.
            }

            try
            {
                if ( m_xDesktop.is() )
                    m_xDesktop->removeTerminateListener( this );
            }
            catch( const Exception& ) { DBG_UNHANDLED_EXCEPTION(); }

            release();
            return 0L;
        }

        // .............................................................................
        void SAL_CALL AsyncLoader::queryTermination( const EventObject& /*Event*/ ) throw (TerminationVetoException, RuntimeException)
        {
            throw TerminationVetoException();
        }
        
        // .............................................................................
        void SAL_CALL AsyncLoader::notifyTermination( const EventObject& /*Event*/ ) throw (RuntimeException)
        {
        }
        // .............................................................................
        void SAL_CALL AsyncLoader::disposing( const EventObject& /*Source*/ ) throw (RuntimeException)
        {
        }
    }

    // -----------------------------------------------------------------------------
    sal_Bool ODbTypeWizDialogSetup::onFinish(sal_Int32 _nResult)
    {
        if ( m_pGeneralPage->GetDatabaseCreationMode() == OGeneralPage::eOpenExisting )
        {
            // we're not going to re-use the XModel we have - since the document the user
            // wants us to load could be a non-database document. Instead, we asynchronously
            // open the selected document. Thus, the wizard's return value is RET_CANCEL,
            // which means to not continue loading the database document
            if ( !OWizardMachine::onFinish( RET_CANCEL ) )
                return sal_False;

            Reference< XComponentLoader > xFrameLoader;
            try
            {
                AsyncLoader* pAsyncLoader = new AsyncLoader( getORB(), m_pGeneralPage->GetSelectedDocument().sURL );
                ::rtl::Reference< AsyncLoader > xKeepAlive( pAsyncLoader );
                pAsyncLoader->doLoadAsync();
            }
            catch( const Exception& )
            {
                DBG_UNHANDLED_EXCEPTION();
            }

            return sal_True;
        }

        if (getCurrentState() != PAGE_DBSETUPWIZARD_FINAL)
        {
            skipUntil(PAGE_DBSETUPWIZARD_FINAL);
        }
        if (getCurrentState() == PAGE_DBSETUPWIZARD_FINAL)
	        return SaveDatabaseDocument() ? OWizardMachine::onFinish( _nResult ) : sal_False;
        else
        {
       	    enableButtons( WZB_FINISH, sal_False );
            return sal_False;
        }
    }

//.........................................................................
}	// namespace dbaui
//.........................................................................

