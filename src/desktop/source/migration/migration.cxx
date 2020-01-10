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
#include "precompiled_desktop.hxx"

#include "migration.hxx"
#include "migration_impl.hxx"
#include "cfgfilter.hxx"

#include <unotools/textsearch.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/sequence.hxx>
#include <unotools/bootstrap.hxx>
#include <rtl/bootstrap.hxx>
#include <tools/config.hxx>
#include <i18npool/lang.h>
#include <tools/urlobj.hxx>
#include <osl/file.hxx>
#include <osl/mutex.hxx>
#include <ucbhelper/content.hxx>
#include <osl/security.hxx>
#include <unotools/configmgr.hxx>

#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/task/XJob.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/configuration/backend/XLayer.hpp>
#include <com/sun/star/configuration/backend/XSingleLayerStratum.hpp>
#include <com/sun/star/util/XRefreshable.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>
#include <com/sun/star/util/XStringSubstitution.hpp>

using namespace rtl;
using namespace osl;
using namespace std;
using namespace com::sun::star::task;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::util;
using namespace com::sun::star::container;
using namespace com::sun::star::configuration;
using namespace com::sun::star::configuration::backend;
using com::sun::star::uno::Exception;
using namespace com::sun::star;

namespace desktop {


static MigrationImpl *pImpl = 0;
static Mutex aMutex;
static MigrationImpl *getImpl()
{
    MutexGuard aGuard(aMutex);
    if (pImpl == 0)
        pImpl = new MigrationImpl(comphelper::getProcessServiceFactory());
    return pImpl;
}

static void releaseImpl()
{
    MutexGuard aGuard(aMutex);
    if (pImpl != 0)
    {
        delete pImpl;
        pImpl = 0;
    }
}

// static main entry point for the migration process
void Migration::doMigration()
{
    sal_Bool bResult = sal_False;
    try {
        bResult = getImpl()->doMigration();
    } catch (Exception& e)
    {
        OString aMsg("doMigration() exception: ");
        aMsg += OUStringToOString(e.Message, RTL_TEXTENCODING_ASCII_US);
        OSL_ENSURE(sal_False, aMsg.getStr());
    }
    OSL_ENSURE(bResult, "Migration has not been successfull");
    // shut down migration framework
    releaseImpl();
}

void Migration::cancelMigration()
{
    releaseImpl();
}

sal_Bool Migration::checkMigration()
{
    return getImpl()->checkMigration();
}

OUString Migration::getOldVersionName()
{
    return getImpl()->getOldVersionName();
}

OUString MigrationImpl::getOldVersionName()
{
    return m_aInfo.productname;
}

sal_Bool MigrationImpl::checkMigration()
{
    if (m_aInfo.userdata.getLength() > 0 && ! checkMigrationCompleted())
        return sal_True;
    else
        return sal_False;
}

MigrationImpl::MigrationImpl(const uno::Reference< XMultiServiceFactory >& xFactory)
    : m_vrVersions(new strings_v)
    , m_xFactory(xFactory)
{
    readAvailableMigrations(m_vMigrationsAvailable);
    sal_Int32 nIndex = findPreferedMigrationProcess(m_vMigrationsAvailable);
    if ( nIndex >= 0 )
        m_vrMigrations = readMigrationSteps(m_vMigrationsAvailable[nIndex].name);
}

MigrationImpl::~MigrationImpl()
{

}

sal_Bool MigrationImpl::doMigration()
{
    // compile file and service list for migration
    m_vrFileList    = compileFileList();
    m_vrServiceList = compileServiceList();
    
    sal_Bool result = sal_False;
    try{
        copyFiles();

		// execute the migration items from Setup.xcu
		// and refresh the cache
        copyConfig();
		refresh();

		// execute custom migration services from Setup.xcu
		// and refresh the cache
        runServices();
        refresh();

        result = sal_True;
    } catch (...)
    {
        OString aMsg("An unexpected exception was thrown during migration");
        aMsg += "\nOldVersion: " + OUStringToOString(m_aInfo.productname, RTL_TEXTENCODING_ASCII_US);
        aMsg += "\nDataPath  : " + OUStringToOString(m_aInfo.userdata, RTL_TEXTENCODING_ASCII_US);
        OSL_ENSURE(sal_False, aMsg.getStr());
    }

	// prevent running the migration multiple times
	setMigrationCompleted();
    return result;
}

void MigrationImpl::refresh()
{
	uno::Reference< XRefreshable > xRefresh(m_xFactory->createInstance(
                OUString::createFromAscii("com.sun.star.configuration.ConfigurationProvider")), uno::UNO_QUERY);
    if (xRefresh.is())
        xRefresh->refresh();
    else
        OSL_ENSURE(sal_False, "could not get XRefresh interface from default config provider. No refresh done.");

}

void MigrationImpl::setMigrationCompleted()
{
	try {
		uno::Reference< XPropertySet > aPropertySet(getConfigAccess("org.openoffice.Setup/Office", true), uno::UNO_QUERY_THROW);
		aPropertySet->setPropertyValue(OUString::createFromAscii("MigrationCompleted"), uno::makeAny(sal_True));
		uno::Reference< XChangesBatch >(aPropertySet, uno::UNO_QUERY_THROW)->commitChanges();
	} catch (...) {
		// fail silently
	}
}

sal_Bool MigrationImpl::checkMigrationCompleted()
{
    sal_Bool bMigrationCompleted = sal_False;
    try {
		uno::Reference< XPropertySet > aPropertySet(
            getConfigAccess("org.openoffice.Setup/Office"), uno::UNO_QUERY_THROW);
        aPropertySet->getPropertyValue(
            OUString::createFromAscii("MigrationCompleted")) >>= bMigrationCompleted;
    } catch (Exception&) {
        // just return false...
    }
    return bMigrationCompleted;
}

static void insertSorted(migrations_available& rAvailableMigrations, supported_migration& aSupportedMigration)
{
    bool                           bInserted( false );
    migrations_available::iterator pIter = rAvailableMigrations.begin();
    while ( !bInserted && pIter != rAvailableMigrations.end())
    {
        if ( pIter->nPriority < aSupportedMigration.nPriority )
        {
            rAvailableMigrations.insert(pIter, aSupportedMigration );
            bInserted = true;
            break; // i111193: insert invalidates iterator! 
        }
        ++pIter;
    }
    if ( !bInserted )
        rAvailableMigrations.push_back( aSupportedMigration );
}

bool MigrationImpl::readAvailableMigrations(migrations_available& rAvailableMigrations)
{
    // get supported version names
	uno::Reference< XNameAccess > aMigrationAccess(getConfigAccess("org.openoffice.Setup/Migration/SupportedVersions"), uno::UNO_QUERY_THROW);
    uno::Sequence< OUString > seqSupportedVersions = aMigrationAccess->getElementNames();
    
    const OUString aVersionIdentifiers( RTL_CONSTASCII_USTRINGPARAM( "VersionIdentifiers" ));
    const OUString aPriorityIdentifier( RTL_CONSTASCII_USTRINGPARAM( "Priority" ));

    for (sal_Int32 i=0; i<seqSupportedVersions.getLength(); i++)
    {
        sal_Int32                 nPriority( 0 );
        uno::Sequence< OUString > seqVersions;
        uno::Reference< XNameAccess > xMigrationData( aMigrationAccess->getByName(seqSupportedVersions[i]), uno::UNO_QUERY_THROW );
        xMigrationData->getByName( aVersionIdentifiers ) >>= seqVersions;
        xMigrationData->getByName( aPriorityIdentifier ) >>= nPriority;

        supported_migration aSupportedMigration;
        aSupportedMigration.name      = seqSupportedVersions[i];
        aSupportedMigration.nPriority = nPriority;
        for (sal_Int32 j=0; j<seqVersions.getLength(); j++)
            aSupportedMigration.supported_versions.push_back(seqVersions[j].trim());
        insertSorted( rAvailableMigrations, aSupportedMigration );
    }

    return true;
}

migrations_vr MigrationImpl::readMigrationSteps(const ::rtl::OUString& rMigrationName)
{
    // get migration access
	uno::Reference< XNameAccess > aMigrationAccess(getConfigAccess("org.openoffice.Setup/Migration/SupportedVersions"), uno::UNO_QUERY_THROW);
    uno::Reference< XNameAccess > xMigrationData( aMigrationAccess->getByName(rMigrationName), uno::UNO_QUERY_THROW );
    
    // get migration description from from org.openoffice.Setup/Migration
    // and build vector of migration steps
	OUString aMigrationSteps( RTL_CONSTASCII_USTRINGPARAM( "MigrationSteps" ));
    uno::Reference< XNameAccess > theNameAccess(xMigrationData->getByName(aMigrationSteps), uno::UNO_QUERY_THROW);
	uno::Sequence< OUString > seqMigrations = theNameAccess->getElementNames();
	uno::Reference< XNameAccess > tmpAccess;
	uno::Reference< XNameAccess > tmpAccess2;
	uno::Sequence< OUString > tmpSeq;
    migrations_vr vrMigrations(new migrations_v);
    for (sal_Int32 i = 0; i < seqMigrations.getLength(); i++)
    {
        // get current migration step
        theNameAccess->getByName(seqMigrations[i]) >>= tmpAccess;
        // tmpStepPtr = new migration_step();
        migration_step tmpStep;
        tmpStep.name = seqMigrations[i];

        // read included files from current step description
        if (tmpAccess->getByName(OUString::createFromAscii("IncludedFiles")) >>= tmpSeq)
        {
            for (sal_Int32 j=0; j<tmpSeq.getLength(); j++)
                tmpStep.includeFiles.push_back(tmpSeq[j]);
        }

        // exluded files...
        if (tmpAccess->getByName(OUString::createFromAscii("ExcludedFiles")) >>= tmpSeq)
        {
            for (sal_Int32 j=0; j<tmpSeq.getLength(); j++)
                tmpStep.excludeFiles.push_back(tmpSeq[j]);
        }

        // included nodes...
        if (tmpAccess->getByName(OUString::createFromAscii("IncludedNodes")) >>= tmpSeq)
        {
            for (sal_Int32 j=0; j<tmpSeq.getLength(); j++)
                tmpStep.includeConfig.push_back(tmpSeq[j]);
        }

        // excluded nodes...
        if (tmpAccess->getByName(OUString::createFromAscii("ExcludedNodes")) >>= tmpSeq)
        {
            for (sal_Int32 j=0; j<tmpSeq.getLength(); j++)
                tmpStep.excludeConfig.push_back(tmpSeq[j]);
        }

        // included extensions...
        if (tmpAccess->getByName(OUString::createFromAscii("IncludedExtensions")) >>= tmpSeq)
        {
            for (sal_Int32 j=0; j<tmpSeq.getLength(); j++)
                tmpStep.includeExtensions.push_back(tmpSeq[j]);
        }

        // excluded extensions...
        if (tmpAccess->getByName(OUString::createFromAscii("ExcludedExtensions")) >>= tmpSeq)
        {
            for (sal_Int32 j=0; j<tmpSeq.getLength(); j++)
                tmpStep.excludeExtensions.push_back(tmpSeq[j]);
        }

        // config components
        if (tmpAccess->getByName(OUString::createFromAscii("ServiceConfigComponents")) >>= tmpSeq)
        {
            for (sal_Int32 j=0; j<tmpSeq.getLength(); j++)
                tmpStep.configComponents.push_back(tmpSeq[j]);
        }

        // generic service
        tmpAccess->getByName(OUString::createFromAscii("MigrationService")) >>= tmpStep.service;

        vrMigrations->push_back(tmpStep);
    }
    return vrMigrations;
}

static FileBase::RC _checkAndCreateDirectory(INetURLObject& dirURL)
{
    FileBase::RC result = Directory::create(dirURL.GetMainURL(INetURLObject::DECODE_TO_IURI));
    if (result == FileBase::E_NOENT)
    {
        INetURLObject baseURL(dirURL);
        baseURL.removeSegment();
        _checkAndCreateDirectory(baseURL);
        return Directory::create(dirURL.GetMainURL(INetURLObject::DECODE_TO_IURI));
    } else
        return result;
}

install_info MigrationImpl::findInstallation(const strings_v& rVersions)
{
    rtl::OUString aProductName;
	uno::Any aRet = ::utl::ConfigManager::GetDirectConfigProperty( ::utl::ConfigManager::PRODUCTNAME );
    aRet >>= aProductName;
	aProductName = aProductName.toAsciiLowerCase();

    install_info aInfo;
    strings_v::const_iterator i_ver = rVersions.begin();
	uno::Reference < util::XStringSubstitution > xSubst( ::comphelper::getProcessServiceFactory()->createInstance(::rtl::OUString::createFromAscii("com.sun.star.util.PathSubstitution")), uno::UNO_QUERY );
    while (i_ver != rVersions.end())
	{
		::rtl::OUString aVersion, aProfileName;
		sal_Int32 nSeparatorIndex = (*i_ver).indexOf('=');
		if ( nSeparatorIndex != -1 )
		{
			aVersion = (*i_ver).copy( 0, nSeparatorIndex );
			aProfileName = (*i_ver).copy( nSeparatorIndex+1 );
		}

		if ( aVersion.getLength() && aProfileName.getLength() && 
				( !aInfo.userdata.getLength() || !aProfileName.toAsciiLowerCase().compareTo( aProductName, aProductName.getLength() ) )
		   )
		{
			::rtl::OUString aUserInst;
			osl::Security().getConfigDir( aUserInst );
			if ( aUserInst.getLength() && aUserInst[ aUserInst.getLength()-1 ] != '/' )
				aUserInst += ::rtl::OUString::createFromAscii("/");
#if defined UNX && ! defined MACOSX
            // tribute to whoever had the "great" idea to use different names on Windows and Unix
            aUserInst += ::rtl::OUString::createFromAscii(".");
#endif
			aUserInst += aProfileName;
			try
			{
				INetURLObject aObj(aUserInst);
				::ucbhelper::Content aCnt( aObj.GetMainURL( INetURLObject::NO_DECODE ), uno::Reference< ucb::XCommandEnvironment > () );
				aCnt.isDocument();
				aInfo.userdata = aObj.GetMainURL( INetURLObject::NO_DECODE );
				aInfo.productname = aVersion;
			}
			catch( uno::Exception& ){}
		}
		++i_ver;
	}

	return aInfo;
}

sal_Int32 MigrationImpl::findPreferedMigrationProcess(const migrations_available& rAvailableMigrations)
{
    sal_Int32    nIndex( -1 );
    sal_Int32    i( 0 );
    
    migrations_available::const_iterator rIter = rAvailableMigrations.begin();
    while ( rIter != rAvailableMigrations.end() )
    {
        install_info aInstallInfo = findInstallation(rIter->supported_versions);
        if (aInstallInfo.productname.getLength() > 0 )
        {
            m_aInfo = aInstallInfo;
            nIndex  = i;
            break;
        }
        ++i;
        ++rIter;
    }

    return nIndex;
}

strings_vr MigrationImpl::applyPatterns(const strings_v& vSet, const strings_v& vPatterns) const
{
    using namespace utl;
    strings_vr vrResult(new strings_v);
    strings_v::const_iterator i_set;
    strings_v::const_iterator i_pat = vPatterns.begin();
    while (i_pat != vPatterns.end())
    {
        // find matches for this pattern in input set
        // and copy them to the result
        SearchParam param(*i_pat, SearchParam::SRCH_REGEXP);
        TextSearch ts(param, LANGUAGE_DONTKNOW);
        i_set = vSet.begin();
        xub_StrLen start = 0;
        xub_StrLen end = 0;
        while (i_set != vSet.end())
        {
            end = (xub_StrLen)(i_set->getLength());
            if (ts.SearchFrwrd(*i_set, &start, &end))
                vrResult->push_back(*i_set);
            i_set++;
        }
        i_pat++;
    }
    return vrResult;
}

strings_vr MigrationImpl::getAllFiles(const OUString& baseURL) const
{
    using namespace osl;
    strings_vr vrResult(new strings_v);

    // get sub dirs
    Directory dir(baseURL);
    if (dir.open() == FileBase::E_None)
    {
        strings_v vSubDirs;
        strings_vr vrSubResult;

        // work through directory contents...
        DirectoryItem item;
        FileStatus fs(FileStatusMask_Type | FileStatusMask_FileURL);
        while (dir.getNextItem(item) == FileBase::E_None)
        {
            if (item.getFileStatus(fs) == FileBase::E_None)
            {
                if (fs.getFileType() == FileStatus::Directory)
                    vSubDirs.push_back(fs.getFileURL());
                else
                    vrResult->push_back(fs.getFileURL());
            }
        }

        // recurse subfolders
        strings_v::const_iterator i = vSubDirs.begin();
        while (i != vSubDirs.end())
        {
            vrSubResult = getAllFiles(*i);
            vrResult->insert(vrResult->end(), vrSubResult->begin(), vrSubResult->end());
            i++;
        }
    }
    return vrResult;
}

strings_vr MigrationImpl::compileFileList()
{

    strings_vr vrResult(new strings_v);
    strings_vr vrInclude;
    strings_vr vrExclude;
    strings_vr vrTemp;

#ifdef SAL_OS2
    if (m_aInfo.userdata.getLength() == 0)
        return vrResult;
#endif

    // get a list of all files:
    strings_vr vrFiles = getAllFiles(m_aInfo.userdata);

    // get a file list result for each migration step
    migrations_v::const_iterator i_migr = m_vrMigrations->begin();
    while (i_migr != m_vrMigrations->end())
    {
        vrInclude = applyPatterns(*vrFiles, i_migr->includeFiles);
        vrExclude = applyPatterns(*vrFiles, i_migr->excludeFiles);
        substract(*vrInclude, *vrExclude);
        vrResult->insert(vrResult->end(), vrInclude->begin(), vrInclude->end());
        i_migr++;
    }
    return vrResult;
}


void MigrationImpl::copyConfig()
{
    try {
        // 1. get a list of all components from hierachy browser
		uno::Reference< XJob > xBrowser(m_xFactory->createInstance(
            OUString::createFromAscii("com.sun.star.configuration.backend.LocalHierarchyBrowser")), uno::UNO_QUERY_THROW);

		uno::Sequence< NamedValue > seqArgs(2);
        seqArgs[0] = NamedValue(
            OUString::createFromAscii("LayerDataUrl"),
            uno::makeAny(m_aInfo.userdata + OUString::createFromAscii("/user/registry")));
        seqArgs[1] = NamedValue(
            OUString::createFromAscii("FetchComponentNames"),
            uno::makeAny(sal_True));

        // execute the search
        uno::Any aResult = xBrowser->execute(seqArgs);
        uno::Sequence< OUString > seqComponents;
        aResult >>= seqComponents;
        OSL_ENSURE(seqComponents.getLength()>0, "MigrationImpl::copyConfig(): no config components available");

        // 2. create an importer
		uno::Reference< XJob > xImporter(m_xFactory->createInstance(
            OUString::createFromAscii("com.sun.star.configuration.backend.LocalDataImporter")), uno::UNO_QUERY_THROW);

        // 3. for each migration step...
        uno::Sequence< NamedValue > importerArgs(3);
        importerArgs[0] = NamedValue(
            OUString::createFromAscii("LayerDataUrl"),
            uno::makeAny(m_aInfo.userdata + OUString::createFromAscii("/user/registry")));
        importerArgs[1] = NamedValue(
            OUString::createFromAscii("LayerFilter"),
            uno::Any());
        importerArgs[2] = NamedValue(
            OUString::createFromAscii("Component"),
            uno::Any());

        migrations_v::const_iterator i_mig = m_vrMigrations->begin();
        while (i_mig != m_vrMigrations->end())
        {
            //   a. create config filter for step
			uno::Reference< XInitialization > xFilter(
                new CConfigFilter(&(i_mig->includeConfig), &(i_mig->excludeConfig)));
            importerArgs[1].Value = uno::makeAny(xFilter);

            //   b. run each importer with config filter
            for (sal_Int32 i=0; i<seqComponents.getLength(); i++)
            {
                OUString component = seqComponents[i];
                importerArgs[2].Value = uno::makeAny(seqComponents[i]);
                try {
                    aResult = xImporter->execute(importerArgs);
                    Exception myException;
                    if (aResult >>= myException) throw myException;
                } catch(Exception& aException) {
                    OString aMsg("Exception in config layer import.\ncomponent: ");
                    aMsg += OUStringToOString(seqComponents[i], RTL_TEXTENCODING_ASCII_US);
                    aMsg += "\nmessage: ";
                    aMsg += OUStringToOString(aException.Message, RTL_TEXTENCODING_ASCII_US);
                    OSL_ENSURE(sal_False, aMsg.getStr());
                }
            }
            i_mig++;
        }
    }
    catch (Exception& e)
    {
        OString aMsg("Exception in config layer import.\nmessage: ");
        aMsg += OUStringToOString(e.Message, RTL_TEXTENCODING_ASCII_US);
        OSL_ENSURE(sal_False, aMsg.getStr());
    }
}

// removes elements of vector 2 in vector 1
void MigrationImpl::substract(strings_v& va, const strings_v& vb_c) const
{
    strings_v vb(vb_c);
    // ensure uniqueness of entries
    sort(va.begin(), va.end());
    sort(vb.begin(), vb.end());
    unique(va.begin(), va.end());
    unique(vb.begin(), vb.end());

    strings_v::const_iterator i_ex = vb.begin();
    strings_v::iterator i_in;
    strings_v::iterator i_next;
    while (i_ex != vb.end())
    {
        i_in = va.begin();
        while (i_in != va.end())
        {
            if ( *i_in == *i_ex)
            {
                i_next = i_in+1;
                va.erase(i_in);
                i_in = i_next;
                // we can only find one match since we
                // ensured uniquness of the entries. ergo:
                break;
            }
            else
                i_in++;
        }
        i_ex++;
    }
}

uno::Reference< XNameAccess > MigrationImpl::getConfigAccess(const sal_Char* pPath, sal_Bool bUpdate)
{
	uno::Reference< XNameAccess > xNameAccess;
    try{
        OUString sConfigSrvc = OUString::createFromAscii("com.sun.star.configuration.ConfigurationProvider");
        OUString sAccessSrvc;
        if (bUpdate)
            sAccessSrvc = OUString::createFromAscii("com.sun.star.configuration.ConfigurationUpdateAccess");
        else
            sAccessSrvc = OUString::createFromAscii("com.sun.star.configuration.ConfigurationAccess");

        OUString sConfigURL = OUString::createFromAscii(pPath);

        // get configuration provider
		uno::Reference< XMultiServiceFactory > theMSF = comphelper::getProcessServiceFactory();
		uno::Reference< XMultiServiceFactory > theConfigProvider = uno::Reference< XMultiServiceFactory > (
                theMSF->createInstance( sConfigSrvc ),uno::UNO_QUERY_THROW );

        // access the provider
        uno::Sequence< uno::Any > theArgs(1);
        theArgs[ 0 ] <<= sConfigURL;
		xNameAccess = uno::Reference< XNameAccess > (
                theConfigProvider->createInstanceWithArguments(
                sAccessSrvc, theArgs ), uno::UNO_QUERY_THROW );
    } catch (com::sun::star::uno::Exception& e)
    {
        OString aMsg = OUStringToOString(e.Message, RTL_TEXTENCODING_ASCII_US);
        OSL_ENSURE(sal_False, aMsg.getStr());
    }
    return xNameAccess;
}

void MigrationImpl::copyFiles()
{
    strings_v::const_iterator i_file = m_vrFileList->begin();
    OUString localName;
    OUString destName;
    OUString userInstall;
    utl::Bootstrap::PathStatus aStatus;
    aStatus = utl::Bootstrap::locateUserInstallation(userInstall);
    if (aStatus == utl::Bootstrap::PATH_EXISTS)
    {
        while (i_file != m_vrFileList->end())
        {

            // remove installation prefix from file
            localName = i_file->copy(m_aInfo.userdata.getLength());
            destName = userInstall + localName;
            INetURLObject aURL(destName);
            // check whether destination directory exists
            aURL.removeSegment();
            _checkAndCreateDirectory(aURL);
            FileBase::RC copyResult = File::copy(*i_file, destName);
            if (copyResult != FileBase::E_None)
            {
                OString msg("Cannot copy ");
                msg += OUStringToOString(*i_file, RTL_TEXTENCODING_UTF8) + " to "
                    +  OUStringToOString(destName, RTL_TEXTENCODING_UTF8);
                OSL_ENSURE(sal_False, msg.getStr());
            }
            i_file++;
        }
    }
    else
    {
        OSL_ENSURE(sal_False, "copyFiles: UserInstall does not exist");
    }
}

void MigrationImpl::runServices()
{
    //create stratum for old user layer
    OUString aOldLayerURL = m_aInfo.userdata;
    aOldLayerURL += OUString::createFromAscii("/user/registry");
    OUString aStratumSvc = OUString::createFromAscii("com.sun.star.configuration.backend.LocalSingleStratum");
    uno::Sequence< uno::Any > stratumArgs(1);
    stratumArgs[0] = uno::makeAny(aOldLayerURL);
	uno::Reference< XSingleLayerStratum> xStartum( m_xFactory->createInstanceWithArguments(
        aStratumSvc, stratumArgs), uno::UNO_QUERY);

    // Build argument array
    uno::Sequence< uno::Any > seqArguments(4);
    seqArguments[0] = uno::makeAny(NamedValue(
        OUString::createFromAscii("Productname"),
        uno::makeAny(m_aInfo.productname)));
    seqArguments[1] = uno::makeAny(NamedValue(
        OUString::createFromAscii("UserData"),
        uno::makeAny(m_aInfo.userdata)));


    // create an instance of every migration service
    // and execute the migration job
	uno::Reference< XJob > xMigrationJob;

    migrations_v::const_iterator i_mig  = m_vrMigrations->begin();
    while (i_mig != m_vrMigrations->end())
    {
        if( i_mig->service.getLength() > 0)
        {

            try
            {
                // create access to old configuration components in the user layer
                // that were requested by the migration service
                uno::Sequence< NamedValue > seqComponents(i_mig->configComponents.size());
                strings_v::const_iterator i_comp = i_mig->configComponents.begin();
                sal_Int32 i = 0;
                while (i_comp != i_mig->configComponents.end() && xStartum.is())
                {
                    // create Layer for i_comp
                    seqComponents[i] =  NamedValue(
                        *i_comp, uno::makeAny(xStartum->getLayer(*i_comp, OUString())));

                    // next component
                    i_comp++;
                    i++;
                }
                
                // set old config argument
                seqArguments[2] = uno::makeAny(NamedValue(
                    OUString::createFromAscii("OldConfiguration"),
                    uno::makeAny(seqComponents)));
                
                // set black list for extension migration
                uno::Sequence< rtl::OUString > seqExtBlackList;
                sal_uInt32 nSize = i_mig->excludeExtensions.size();
                if ( nSize > 0 )
                    seqExtBlackList = comphelper::arrayToSequence< ::rtl::OUString >( 
                        &i_mig->excludeExtensions[0], nSize ); 
                seqArguments[3] = uno::makeAny(NamedValue(
                    OUString::createFromAscii("ExtensionBlackList"),
                    uno::makeAny( seqExtBlackList )));

				xMigrationJob = uno::Reference< XJob >(m_xFactory->createInstanceWithArguments(
                    i_mig->service, seqArguments), uno::UNO_QUERY_THROW);

                xMigrationJob->execute(uno::Sequence< NamedValue >());


            } catch (Exception& e)
            {
                OString aMsg("Execution of migration service failed (Exception caught).\nService: ");
                aMsg += OUStringToOString(i_mig->service, RTL_TEXTENCODING_ASCII_US) + "\nMessage: ";
                aMsg += OUStringToOString(e.Message, RTL_TEXTENCODING_ASCII_US);
                OSL_ENSURE(sal_False, aMsg.getStr());
            } catch (...)
            {
                OString aMsg("Execution of migration service failed (Exception caught).\nService: ");
                aMsg += OUStringToOString(i_mig->service, RTL_TEXTENCODING_ASCII_US) +
					"\nNo message available";
                OSL_ENSURE(sal_False, aMsg.getStr());
            }

        }
        i_mig++;
    }
}


strings_vr MigrationImpl::compileServiceList()
{
    strings_vr vrResult(new strings_v);
    migrations_v::const_iterator i_migr = m_vrMigrations->begin();
    while (i_migr != m_vrMigrations->end())
    {
        vrResult->push_back(i_migr->service);
        i_migr++;
    }
    return vrResult;
}

} // namespace desktop
