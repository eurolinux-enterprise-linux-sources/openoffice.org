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

#include <osl/file.hxx>
#include <rtl/bootstrap.hxx>
#include <rtl/ustring.hxx>
#include <tools/datetime.hxx>
#include <unotools/configmgr.hxx>

#include <comphelper/processfactory.hxx>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/NamedValue.hpp>

#include "app.hxx"

using rtl::OUString;
using namespace desktop;
using namespace com::sun::star::beans;

static const OUString sConfigSrvc( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.configuration.ConfigurationProvider" ) );
static const OUString sAccessSrvc( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.configuration.ConfigurationUpdateAccess" ) );

/* Path of the license. */
OUString Desktop::GetLicensePath()
{
    // license file name
    static const char *szLicensePath = "/share/readme";
#if defined(WNT) || defined(OS2)
    static const char *szWNTLicenseName = "/license";
    static const char *szWNTLicenseExt = ".txt";
#else
    static const char *szUNXLicenseName = "/LICENSE";
    static const char *szUNXLicenseExt = "";
#endif
    static OUString aLicensePath;

    if (aLicensePath.getLength() > 0)
        return aLicensePath;

    OUString aBaseInstallPath(RTL_CONSTASCII_USTRINGPARAM("$BRAND_BASE_DIR"));
    rtl::Bootstrap::expandMacros(aBaseInstallPath);

    // determine the filename of the license to show
    OUString  aLangString;
    ::com::sun::star::lang::Locale aLocale;
    OString aMgrName = OString("dkt");

    AllSettings aSettings(Application::GetSettings());
    aLocale = aSettings.GetUILocale();
    ResMgr* pLocalResMgr = ResMgr::SearchCreateResMgr(aMgrName, aLocale);

    aLangString = aLocale.Language;
    if ( aLocale.Country.getLength() != 0 )
    {
        aLangString += OUString::createFromAscii("-");
        aLangString += aLocale.Country;
        if ( aLocale.Variant.getLength() != 0 )
        {
            aLangString += OUString::createFromAscii("-");
            aLangString += aLocale.Variant;
        }
    }
#if defined(WNT) || defined(OS2)
    aLicensePath =
        aBaseInstallPath + OUString::createFromAscii(szLicensePath)
        + OUString::createFromAscii(szWNTLicenseName)
        + OUString::createFromAscii("_")
        + aLangString
        + OUString::createFromAscii(szWNTLicenseExt);
#else
    aLicensePath =
        aBaseInstallPath + OUString::createFromAscii(szLicensePath)
        + OUString::createFromAscii(szUNXLicenseName)
        + OUString::createFromAscii("_")
        + aLangString
        + OUString::createFromAscii(szUNXLicenseExt);
#endif
    delete pLocalResMgr;
    return aLicensePath;
}

/* Check if we need to accept license. */
sal_Bool Desktop::LicenseNeedsAcceptance()
{
    // Don't show a license
    return sal_False;
/*
    sal_Bool bShowLicense = sal_True;
    sal_Int32 nOpenSourceContext = 0;
    try
    {
        ::utl::ConfigManager::GetDirectConfigProperty(
            ::utl::ConfigManager::OPENSOURCECONTEXT ) >>= nOpenSourceContext;
    }
    catch( const ::com::sun::star::uno::Exception& ) {}

    // open source needs no license
    if ( nOpenSourceContext > 0 )
        bShowLicense = sal_False;

    return bShowLicense;
*/
}

/* Local function - was the wizard completed already? */
static sal_Bool impl_isFirstStart()
{
    try {
        Reference < XMultiServiceFactory > xFactory = ::comphelper::getProcessServiceFactory();

        // get configuration provider
        Reference< XMultiServiceFactory > theConfigProvider = Reference< XMultiServiceFactory >(
                xFactory->createInstance(sConfigSrvc), UNO_QUERY_THROW);

        Sequence< Any > theArgs(1);
        NamedValue v(OUString::createFromAscii("NodePath"), makeAny(OUString::createFromAscii("org.openoffice.Setup/Office")));
        theArgs[0] <<= v;

        Reference< XPropertySet > pset = Reference< XPropertySet >(
                theConfigProvider->createInstanceWithArguments(sAccessSrvc, theArgs), UNO_QUERY_THROW);

        Any result = pset->getPropertyValue(OUString::createFromAscii("FirstStartWizardCompleted"));
        sal_Bool bCompleted = sal_False;
        if ((result >>= bCompleted) && bCompleted)
            return sal_False;  // wizard was already completed
        else
            return sal_True;
    } catch (const Exception&)
    {
        return sal_True;
    }
}

/* Local function - convert oslDateTime to tools DateTime */
static DateTime impl_oslDateTimeToDateTime(const oslDateTime& aDateTime)
{
    return DateTime(
        Date(aDateTime.Day, aDateTime.Month, aDateTime.Year),
        Time(aDateTime.Hours, aDateTime.Minutes, aDateTime.Seconds));
}

/* Local function - get DateTime from a string */
static sal_Bool impl_parseDateTime(const OUString& aString, DateTime& aDateTime)
{
    // take apart a canonical literal xsd:dateTime string
    //CCYY-MM-DDThh:mm:ss(Z)

    OUString aDateTimeString = aString.trim();

    // check length
    if (aDateTimeString.getLength() < 19 || aDateTimeString.getLength() > 20)
        return sal_False;

    sal_Int32 nDateLength = 10;
    sal_Int32 nTimeLength = 8;

    OUString aDateTimeSep = OUString::createFromAscii("T");
    OUString aDateSep = OUString::createFromAscii("-");
    OUString aTimeSep = OUString::createFromAscii(":");
    OUString aUTCString = OUString::createFromAscii("Z");

    OUString aDateString = aDateTimeString.copy(0, nDateLength);
    OUString aTimeString = aDateTimeString.copy(nDateLength+1, nTimeLength);

    sal_Int32 nIndex = 0;
    sal_Int32 nYear = aDateString.getToken(0, '-', nIndex).toInt32();
    sal_Int32 nMonth = aDateString.getToken(0, '-', nIndex).toInt32();
    sal_Int32 nDay = aDateString.getToken(0, '-', nIndex).toInt32();
    nIndex = 0;
    sal_Int32 nHour = aTimeString.getToken(0, ':', nIndex).toInt32();
    sal_Int32 nMinute = aTimeString.getToken(0, ':', nIndex).toInt32();
    sal_Int32 nSecond = aTimeString.getToken(0, ':', nIndex).toInt32();

    Date tmpDate((USHORT)nDay, (USHORT)nMonth, (USHORT)nYear);
    Time tmpTime(nHour, nMinute, nSecond);
    DateTime tmpDateTime(tmpDate, tmpTime);
    if (aString.indexOf(aUTCString) < 0)
        tmpDateTime.ConvertToUTC();

    aDateTime = tmpDateTime;
    return sal_True;
}

/* Local function - was the license accepted already? */
static sal_Bool impl_isLicenseAccepted()
{
    // If no license will be shown ... it must not be accepted.
    // So it was accepted "hardly" by the outside installer.
    // But if the configuration entry "HideEula" will be removed afterwards ..
    // we have to show the licese page again and user has to accept it here .-)
    if ( ! Desktop::LicenseNeedsAcceptance() )
        return sal_True;

    try
    {
        Reference < XMultiServiceFactory > xFactory = ::comphelper::getProcessServiceFactory();

        // get configuration provider
        Reference< XMultiServiceFactory > theConfigProvider = Reference< XMultiServiceFactory >(
                xFactory->createInstance(sConfigSrvc), UNO_QUERY_THROW);

        Sequence< Any > theArgs(1);
        NamedValue v(OUString::createFromAscii("NodePath"),
                makeAny(OUString::createFromAscii("org.openoffice.Setup/Office")));
        theArgs[0] <<= v;
        Reference< XPropertySet > pset = Reference< XPropertySet >(
                theConfigProvider->createInstanceWithArguments(sAccessSrvc, theArgs), UNO_QUERY_THROW);

        Any result = pset->getPropertyValue(OUString::createFromAscii("LicenseAcceptDate"));

        OUString aAcceptDate;
        if (result >>= aAcceptDate)
        {
            // compare to date of license file
            OUString aLicenseURL = Desktop::GetLicensePath();
            osl::DirectoryItem aDirItem;
            if (osl::DirectoryItem::get(aLicenseURL, aDirItem) != osl::FileBase::E_None)
                return sal_False;
            osl::FileStatus aStatus(FileStatusMask_All);
            if (aDirItem.getFileStatus(aStatus) != osl::FileBase::E_None)
                return sal_False;
            TimeValue aTimeVal = aStatus.getModifyTime();
            oslDateTime aDateTimeVal;
            if (!osl_getDateTimeFromTimeValue(&aTimeVal, &aDateTimeVal))
                return sal_False;

            // compare dates
            DateTime aLicenseDateTime = impl_oslDateTimeToDateTime(aDateTimeVal);
            DateTime aAcceptDateTime;
            if (!impl_parseDateTime(aAcceptDate, aAcceptDateTime))
                return sal_False;

            if ( aAcceptDateTime > aLicenseDateTime )
                return sal_True;
        }
        return sal_False;
    } catch (const Exception&)
    {
        return sal_False;
    }
}

/* Check if we need the first start wizard. */
sal_Bool Desktop::IsFirstStartWizardNeeded()
{
    return impl_isFirstStart() || !impl_isLicenseAccepted();
}

