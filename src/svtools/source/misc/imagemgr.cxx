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
#include "precompiled_svtools.hxx"

// includes --------------------------------------------------------------

#include "imagemgr.hxx"
#include <tools/urlobj.hxx>
#include <tools/debug.hxx>
#include <vcl/svapp.hxx>
#include <vcl/wrkwin.hxx>
#include "vcl/image.hxx"
#include <sot/storage.hxx>
#include <sot/clsids.hxx>
#include <unotools/ucbhelper.hxx>
#ifndef _UNOTOOLS_PROCESSFACTORY_HXX
#include <comphelper/processfactory.hxx>
#endif
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/document/XTypeDetection.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/ucb/XCommandEnvironment.hpp>
#include <ucbhelper/content.hxx>
#include <tools/rcid.h>
#include <rtl/logfile.hxx>
#include <unotools/configmgr.hxx>

#include <svtools/svtools.hrc>
#include "imagemgr.hrc"
#include <svtools/svtdata.hxx>

// globals *******************************************************************

#define NO_INDEX    	((USHORT)0xFFFF)
#define CONTENT_HELPER	::utl::UCBContentHelper
#define ASCII_STRING(s)	::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(s) )

struct SvtExtensionResIdMapping_Impl
{
    const char*   _pExt;
    BOOL    _bExt;
    USHORT  _nStrId;
    USHORT  _nImgId;
};

static SvtExtensionResIdMapping_Impl __READONLY_DATA ExtensionMap_Impl[] =
{
    { "awk",   TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "bas",   TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "bat",   TRUE,  STR_DESCRIPTION_BATCHFILE,             0 },
    { "bmk",   FALSE, STR_DESCRIPTION_BOOKMARKFILE,          0 },
    { "bmp",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_BITMAP },
    { "c",     TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "cfg",   FALSE, STR_DESCRIPTION_CFGFILE,               0 },
    { "cmd",   TRUE,  STR_DESCRIPTION_BATCHFILE,             0 },
    { "cob",   TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "com",   TRUE,  STR_DESCRIPTION_APPLICATION,           0 },
    { "cxx",   TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "dbf",   TRUE,  STR_DESCRIPTION_DATABASE_TABLE,        IMG_TABLE },
    { "def",   TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "dll",   TRUE,  STR_DESCRIPTION_SYSFILE,               0 },
    { "doc",   FALSE, STR_DESCRIPTION_WORD_DOC,              IMG_WRITER },
    { "dot",   FALSE, STR_DESCRIPTION_WORD_DOC,              IMG_WRITERTEMPLATE },
    { "docx",  FALSE, STR_DESCRIPTION_WORD_DOC,              IMG_WRITER },
    { "dxf",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_DXF },
    { "exe",   TRUE,  STR_DESCRIPTION_APPLICATION,           0 },
    { "gif",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_GIF },
    { "h",     TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "hlp",   FALSE, STR_DESCRIPTION_HELP_DOC,              0 },
    { "hrc",   TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "htm",   FALSE, STR_DESCRIPTION_HTMLFILE,              IMG_HTML },
    { "html",  FALSE, STR_DESCRIPTION_HTMLFILE,              IMG_HTML },
    { "asp",   FALSE, STR_DESCRIPTION_HTMLFILE,              IMG_HTML },
    { "hxx",   TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "ini",   FALSE, STR_DESCRIPTION_CFGFILE,               0 },
    { "java",  TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "jpeg",  TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_JPG },
    { "jpg",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_JPG },
    { "lha",   TRUE,  STR_DESCRIPTION_ARCHIVFILE,            0 },
#ifdef WNT
    { "lnk",   FALSE, 0,                                     0 },
#endif
    { "log",   TRUE,  STR_DESCRIPTION_LOGFILE,               0 },
    { "lst",   TRUE,  STR_DESCRIPTION_LOGFILE,               0 },
    { "met",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_MET },
    { "mml",   FALSE, STR_DESCRIPTION_MATHML_DOC,            IMG_MATH },
    { "mod",   TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "odb",   FALSE, STR_DESCRIPTION_OO_DATABASE_DOC,       IMG_OO_DATABASE_DOC },
    { "odg",   FALSE, STR_DESCRIPTION_OO_DRAW_DOC,           IMG_OO_DRAW_DOC },
    { "odf",   FALSE, STR_DESCRIPTION_OO_MATH_DOC,           IMG_OO_MATH_DOC },
    { "odm",   FALSE, STR_DESCRIPTION_OO_GLOBAL_DOC,         IMG_OO_GLOBAL_DOC },
    { "odp",   FALSE, STR_DESCRIPTION_OO_IMPRESS_DOC,        IMG_OO_IMPRESS_DOC },
    { "ods",   FALSE, STR_DESCRIPTION_OO_CALC_DOC,           IMG_OO_CALC_DOC },
    { "odt",   FALSE, STR_DESCRIPTION_OO_WRITER_DOC,         IMG_OO_WRITER_DOC },
    { "otg",   FALSE, STR_DESCRIPTION_OO_DRAW_TEMPLATE,      IMG_OO_DRAW_TEMPLATE },
    { "otp",   FALSE, STR_DESCRIPTION_OO_IMPRESS_TEMPLATE,   IMG_OO_IMPRESS_TEMPLATE },
    { "ots",   FALSE, STR_DESCRIPTION_OO_CALC_TEMPLATE,      IMG_OO_CALC_TEMPLATE },
    { "ott",   FALSE, STR_DESCRIPTION_OO_WRITER_TEMPLATE,    IMG_OO_WRITER_TEMPLATE },
	{ "pas",   TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "pcd",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_PCD },
    { "pct",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_PCT },
    { "pcx",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_PCX },
    { "pl",    TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "png",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_PNG },
    { "rar",   TRUE,  STR_DESCRIPTION_ARCHIVFILE,            0 },
    { "rtf",   FALSE, STR_DESCRIPTION_WORD_DOC,              IMG_WRITER },
    { "sbl",   FALSE, 0,                                     0 },
    { "sch",   FALSE, 0,                                     0 },
    { "sda",   FALSE, STR_DESCRIPTION_SDRAW_DOC,             IMG_DRAW },
    { "sdb",   FALSE, STR_DESCRIPTION_SDATABASE_DOC,         IMG_DATABASE },
    { "sdc",   FALSE, STR_DESCRIPTION_SCALC_DOC,             IMG_CALC },
    { "sdd",   FALSE, STR_DESCRIPTION_SIMPRESS_DOC,          IMG_IMPRESS },
    { "sdp",   FALSE, STR_DESCRIPTION_SIMPRESS_DOC,          0 },
    { "sds",   FALSE, STR_DESCRIPTION_SCHART_DOC,            0 },
    { "sdw",   FALSE, STR_DESCRIPTION_SWRITER_DOC,           IMG_WRITER },
    { "sga",   FALSE, 0,                                     0 },
    { "sgf",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_SGF },
    { "sgl",   FALSE, STR_DESCRIPTION_GLOBALDOC,             IMG_GLOBAL_DOC },
    { "sgv",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_SGV },
    { "shtml", FALSE, STR_DESCRIPTION_HTMLFILE,              IMG_HTML },
    { "sim",   FALSE, STR_DESCRIPTION_SIMAGE_DOC,            IMG_SIM },
    { "smf",   FALSE, STR_DESCRIPTION_SMATH_DOC,             IMG_MATH },
    { "src",   TRUE,  STR_DESCRIPTION_SOURCEFILE,            0 },
    { "svh",   FALSE, STR_DESCRIPTION_HELP_DOC,              0 },
    { "svm",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_SVM },
    { "stc",   FALSE, STR_DESCRIPTION_CALC_TEMPLATE,         IMG_CALCTEMPLATE },
    { "std",   FALSE, STR_DESCRIPTION_DRAW_TEMPLATE,         IMG_DRAWTEMPLATE },
    { "sti",   FALSE, STR_DESCRIPTION_IMPRESS_TEMPLATE,      IMG_IMPRESSTEMPLATE },
    { "stw",   FALSE, STR_DESCRIPTION_WRITER_TEMPLATE,       IMG_WRITERTEMPLATE },
    { "sxc",   FALSE, STR_DESCRIPTION_SXCALC_DOC,            IMG_CALC },
    { "sxd",   FALSE, STR_DESCRIPTION_SXDRAW_DOC,            IMG_DRAW },
    { "sxg",   FALSE, STR_DESCRIPTION_SXGLOBAL_DOC,          IMG_GLOBAL_DOC },
    { "sxi",   FALSE, STR_DESCRIPTION_SXIMPRESS_DOC,         IMG_IMPRESS },
    { "sxm",   FALSE, STR_DESCRIPTION_SXMATH_DOC,            IMG_MATH },
    { "sxs",   FALSE, STR_DESCRIPTION_SXCHART_DOC,           0 },
    { "sxw",   FALSE, STR_DESCRIPTION_SXWRITER_DOC,          IMG_WRITER },
    { "sys",   TRUE,  STR_DESCRIPTION_SYSFILE,               0 },
    { "tif",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_TIFF },
    { "tiff",  TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_TIFF },
    { "txt",   FALSE, STR_DESCRIPTION_TEXTFILE,              IMG_TEXTFILE },
    { "url",   FALSE, STR_DESCRIPTION_LINK,                  0 },
    { "vor",   FALSE, STR_DESCRIPTION_SOFFICE_TEMPLATE_DOC,  IMG_WRITERTEMPLATE },
    { "vxd",   TRUE,  STR_DESCRIPTION_SYSFILE,               0 },
    { "wmf",   TRUE,  STR_DESCRIPTION_GRAPHIC_DOC,           IMG_WMF },
    { "xls",   FALSE, STR_DESCRIPTION_EXCEL_DOC,             IMG_CALC },
    { "xlt",   FALSE, STR_DESCRIPTION_EXCEL_TEMPLATE_DOC,    IMG_CALCTEMPLATE },
    { "xlsx",  FALSE, STR_DESCRIPTION_EXCEL_DOC,             IMG_CALC },
    { "uu",    TRUE,  STR_DESCRIPTION_ARCHIVFILE,            0 },
    { "uue",   TRUE,  STR_DESCRIPTION_ARCHIVFILE,            0 },
    { "z",     TRUE,  STR_DESCRIPTION_ARCHIVFILE,            0 },
    { "zip",   TRUE,  STR_DESCRIPTION_ARCHIVFILE,            0 },
    { "zoo",   TRUE,  STR_DESCRIPTION_ARCHIVFILE,            0 },
    { "gz",    TRUE,  STR_DESCRIPTION_ARCHIVFILE,            0 },
    { "ppt",   FALSE, STR_DESCRIPTION_POWERPOINT,            IMG_IMPRESS },
    { "pot",   FALSE, STR_DESCRIPTION_POWERPOINT_TEMPLATE,   IMG_IMPRESSTEMPLATE },
    { "pps",   FALSE, STR_DESCRIPTION_POWERPOINT_SHOW,       IMG_IMPRESS },
    { "pptx",  FALSE, STR_DESCRIPTION_POWERPOINT,            IMG_IMPRESS },
    { "oxt",   FALSE, STR_DESCRIPTION_EXTENSION,             IMG_EXTENSION },
    { 0, FALSE, 0, 0 }
};

#ifdef OS2
    SvtExtensionResIdMapping_Impl Mappings[] =
	{
	{"StarWriter 4.0",         FALSE,STR_DESCRIPTION_SWRITER_DOC, IMG_WRITER},
	{"StarWriter 3.0",         FALSE,STR_DESCRIPTION_SWRITER_DOC, IMG_WRITER},
	{"StarCalc 4.0",           FALSE,STR_DESCRIPTION_SCALC_DOC,   IMG_CALC},
	{"StarCalc 3.0",           FALSE,STR_DESCRIPTION_SCALC_DOC,   IMG_CALC},
	{"StarImpress 4.0",        FALSE,STR_DESCRIPTION_SIMPRESS_DOC,IMG_IMPRESS},
	{"StarDraw 4.0",           FALSE,STR_DESCRIPTION_SDRAW_DOC,   IMG_DRAW},
	{"StarDraw 3.0",           FALSE,STR_DESCRIPTION_SDRAW_DOC,   IMG_DRAW},
	{"StarChart 3.0",          FALSE,STR_DESCRIPTION_SCHART_DOC,  IMG_CHART},
	{"StarChart 4.0",          FALSE,STR_DESCRIPTION_SCHART_DOC,  IMG_CHART},
	{"Bitmap",                 FALSE,STR_DESCRIPTION_GRAPHIC_DOC, IMG_BITMAP},
	{"AutoCAD",                FALSE,STR_DESCRIPTION_GRAPHIC_DOC, IMG_SIM},
	{"Gif-File",               FALSE,STR_DESCRIPTION_GRAPHIC_DOC, IMG_GIF},
	{"JPEG-File",              FALSE,STR_DESCRIPTION_GRAPHIC_DOC, IMG_JPG},
	{"Metafile ",              FALSE,STR_DESCRIPTION_GRAPHIC_DOC, IMG_SIM},
	{"Photo-CD ",              FALSE,STR_DESCRIPTION_GRAPHIC_DOC, IMG_PCD},
	{"Mac Pict",               FALSE,STR_DESCRIPTION_GRAPHIC_DOC, IMG_PCT},
	{"PCX-File ",              FALSE,STR_DESCRIPTION_GRAPHIC_DOC, IMG_PCX},
	{"PNG-File",               FALSE,STR_DESCRIPTION_GRAPHIC_DOC, IMG_SIM},
	{"SV-Metafile",            FALSE,STR_DESCRIPTION_GRAPHIC_DOC, IMG_SIM},
	{"TIFF-File",              FALSE,STR_DESCRIPTION_GRAPHIC_DOC, IMG_TIFF},
	{"MS-Metafile",            FALSE,STR_DESCRIPTION_GRAPHIC_DOC, IMG_WMF},
	{"XBM-File",               FALSE,STR_DESCRIPTION_GRAPHIC_DOC, IMG_BITMAP},
	{"UniformResourceLocator", FALSE,STR_DESCRIPTION_LINK,        IMG_URL},
	{NULL, 0}
	};
#endif

struct SvtFactory2ExtensionMapping_Impl
{
    const char*   _pFactory;
    const char*   _pExtension;
};

// mapping from "private:factory" url to extension

static SvtFactory2ExtensionMapping_Impl __READONLY_DATA Fac2ExtMap_Impl[] =
{
    { "swriter",                "odt" },
    { "swriter/web",            "html" },
    { "swriter/GlobalDocument", "odm" },
    { "scalc",                  "ods" },
    { "simpress",               "odp" },
    { "sdraw",                  "odg" },
    { "smath",                  "odf" },
    { "sdatabase",              "odb" },
    { NULL, NULL }
};

//****************************************************************************

static String GetImageExtensionByFactory_Impl( const String& rURL )
{
    INetURLObject aObj( rURL );
    String aPath = aObj.GetURLPath( INetURLObject::NO_DECODE );
    String aExtension;

    if ( aPath.Len() )
    {
        USHORT nIndex = 0;
        while ( Fac2ExtMap_Impl[ nIndex ]._pFactory )
        {
            if ( aPath.EqualsAscii( Fac2ExtMap_Impl[ nIndex ]._pFactory ) )
            {
                // extension found
                aExtension = String::CreateFromAscii( Fac2ExtMap_Impl[ nIndex ]._pExtension );
                // and return it
                return aExtension;
            }
            ++nIndex;
        }
    }

    // no extension found, so use the type detection (performance brake)

    try
    {
        // get the TypeDetection service to access all registered types
        ::com::sun::star::uno::Reference < ::com::sun::star::lang::XMultiServiceFactory >  xFac = ::comphelper::getProcessServiceFactory();
        ::com::sun::star::uno::Reference < ::com::sun::star::document::XTypeDetection > xTypeDetector(
            xFac->createInstance( ASCII_STRING("com.sun.star.document.TypeDetection") ), ::com::sun::star::uno::UNO_QUERY );

        ::rtl::OUString aInternalType = xTypeDetector->queryTypeByURL( rURL );
        ::com::sun::star::uno::Reference < ::com::sun::star::container::XNameAccess > xAccess( xTypeDetector, ::com::sun::star::uno::UNO_QUERY );
        ::com::sun::star::uno::Sequence < ::com::sun::star::beans::PropertyValue > aTypeProps;
        if ( aInternalType.getLength() > 0 && xAccess->hasByName( aInternalType ) )
        {
            xAccess->getByName( aInternalType ) >>= aTypeProps;
            sal_Int32 nProps = aTypeProps.getLength();
            for ( sal_Int32 i = 0; i < nProps; ++i )
            {
                const ::com::sun::star::beans::PropertyValue& rProp = aTypeProps[i];
                if ( rProp.Name.compareToAscii("Extensions") == COMPARE_EQUAL )
                {
                    ::com::sun::star::uno::Sequence < ::rtl::OUString > aExtensions;
                    if ( ( rProp.Value >>= aExtensions ) && aExtensions.getLength() > 0 )
                    {
                        const ::rtl::OUString* pExtensions = aExtensions.getConstArray();
                        aExtension = String( pExtensions[0] );
                        break;
                    }
                }
            }
        }
    }
    catch( const ::com::sun::star::uno::RuntimeException& )
    {
        throw; // dont hide it!
    }
    catch( const ::com::sun::star::uno::Exception& )
    {
        // type detection failed -> no extension
    }

    return aExtension;
}

static USHORT GetIndexOfExtension_Impl( const String& rExtension )
{
    USHORT nRet = NO_INDEX;
    if ( rExtension.Len() )
    {
        USHORT nIndex = 0;
        String aExt = rExtension;
        aExt.ToLowerAscii();
        while ( ExtensionMap_Impl[ nIndex ]._pExt )
        {
            if ( aExt.EqualsAscii( ExtensionMap_Impl[ nIndex ]._pExt ) )
            {
                nRet = nIndex;
                break;
            }
            ++nIndex;
        }
    }

    return nRet;
}

static USHORT GetImageId_Impl( const String& rExtension )
{
    USHORT nImage = IMG_FILE;
    if ( rExtension.Len()  != NO_INDEX )
    {
        USHORT nIndex = GetIndexOfExtension_Impl( rExtension );
        if ( nIndex != NO_INDEX )
        {
            nImage = ExtensionMap_Impl[ nIndex ]._nImgId;
            if ( !nImage )
                nImage = IMG_FILE;
        }
    }

    return nImage;
}

static sal_Bool GetVolumeProperties_Impl( ::ucbhelper::Content& rContent, svtools::VolumeInfo& rVolumeInfo )
{
    sal_Bool bRet = sal_False;

    try
    {
    	bRet = ( ( rContent.getPropertyValue( ASCII_STRING("IsVolume") ) >>= rVolumeInfo.m_bIsVolume ) &&
    			 ( rContent.getPropertyValue( ASCII_STRING("IsRemote") ) >>= rVolumeInfo.m_bIsRemote ) &&
    			 ( rContent.getPropertyValue( ASCII_STRING("IsRemoveable") ) >>= rVolumeInfo.m_bIsRemoveable ) &&
    			 ( rContent.getPropertyValue( ASCII_STRING("IsFloppy") ) >>= rVolumeInfo.m_bIsFloppy ) &&
    			 ( rContent.getPropertyValue( ASCII_STRING("IsCompactDisc") ) >>= rVolumeInfo.m_bIsCompactDisc ) );
	}
    catch( const ::com::sun::star::uno::RuntimeException& )
    {
        throw; // dont hide it!
    }
    catch( const ::com::sun::star::uno::Exception& )
    {
        // type detection failed -> no extension
    }

    return bRet;
}

static USHORT GetFolderImageId_Impl( const String& rURL )
{
	USHORT nRet = IMG_FOLDER;
	::svtools::VolumeInfo aVolumeInfo;
	try
	{
		::ucbhelper::Content aCnt( rURL, ::com::sun::star::uno::Reference< ::com::sun::star::ucb::XCommandEnvironment > () );
		if ( GetVolumeProperties_Impl( aCnt, aVolumeInfo ) )
		{
	    	if ( aVolumeInfo.m_bIsRemote )
    			nRet = IMG_NETWORKDEV;
			else if ( aVolumeInfo.m_bIsCompactDisc )
				nRet = IMG_CDROMDEV;
    		else if ( aVolumeInfo.m_bIsRemoveable )
    			nRet = IMG_REMOVEABLEDEV;
	    	else if ( aVolumeInfo.m_bIsVolume )
    			nRet = IMG_FIXEDDEV;
		}
	}
    catch( const ::com::sun::star::uno::RuntimeException& )
    {
        throw; // dont hide it!
    }
    catch( const ::com::sun::star::uno::Exception& )
    {
        // ...
    }
	return nRet;
}

static USHORT GetImageId_Impl( const INetURLObject& rObject, sal_Bool bDetectFolder )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aTimeLog, "svtools", "hb93813", "SvFileInformationManager::GetImageId_Impl()" );

    String aExt, sURL = rObject.GetMainURL( INetURLObject::NO_DECODE );
    USHORT nImage = IMG_FILE;

    if ( rObject.GetProtocol() == INET_PROT_PRIVATE )
    {
        String aURLPath = sURL.Copy( URL_PREFIX_PRIV_SOFFICE_LEN );
        String aType = aURLPath.GetToken( 0, INET_PATH_TOKEN );
        if ( aType == String( RTL_CONSTASCII_STRINGPARAM("factory") ) )
		{
			// detect an image id for our "private:factory" urls
            aExt = GetImageExtensionByFactory_Impl( sURL );
	        if ( aExt.Len() > 0 )
	            nImage = GetImageId_Impl( aExt );
			return nImage;
		}
        else if ( aType == String( RTL_CONSTASCII_STRINGPARAM("image") ) )
            nImage = (USHORT)aURLPath.GetToken( 1, INET_PATH_TOKEN ).ToInt32();
    }
    else
    {
        aExt = rObject.getExtension();
        if ( aExt.EqualsAscii( "vor" ) )
        {
            SotStorageRef aStorage = new SotStorage( sURL, STREAM_STD_READ );
            USHORT nId = IMG_WRITERTEMPLATE;
            if ( !aStorage->GetError() )
            {
                SvGlobalName aGlobalName = aStorage->GetClassName();
                if ( aGlobalName == SvGlobalName(SO3_SC_CLASSID_50) || aGlobalName == SvGlobalName(SO3_SC_CLASSID_40) || aGlobalName == SvGlobalName(SO3_SC_CLASSID_30) )
                    nId = IMG_CALCTEMPLATE;
                else if ( aGlobalName == SvGlobalName(SO3_SDRAW_CLASSID_50) )
                    nId = IMG_DRAWTEMPLATE;
                else if ( aGlobalName == SvGlobalName(SO3_SIMPRESS_CLASSID_50) ||
                        aGlobalName == SvGlobalName(SO3_SIMPRESS_CLASSID_40) || aGlobalName == SvGlobalName(SO3_SIMPRESS_CLASSID_30) )
                    nId = IMG_IMPRESSTEMPLATE;
                else if ( aGlobalName == SvGlobalName(SO3_SM_CLASSID_50) || aGlobalName == SvGlobalName(SO3_SM_CLASSID_40) || aGlobalName == SvGlobalName(SO3_SM_CLASSID_30) )
                    nId = IMG_MATHTEMPLATE;
            }

            return nId;
        }
    }

    if ( nImage == IMG_FILE && sURL.Len() )
    {
        if ( bDetectFolder && CONTENT_HELPER::IsFolder( sURL ) )
			nImage = GetFolderImageId_Impl( sURL );
        else if ( aExt.Len() > 0 )
            nImage = GetImageId_Impl( aExt );
    }
    return nImage;
}

static USHORT GetDescriptionId_Impl( const String& rExtension, sal_Bool& rbShowExt )
{
    USHORT nId = 0;

    if ( rExtension.Len()  != NO_INDEX )
    {
        USHORT nIndex = GetIndexOfExtension_Impl( rExtension );
        if ( nIndex != NO_INDEX )
        {
            nId = ExtensionMap_Impl[ nIndex ]._nStrId;
            rbShowExt = ExtensionMap_Impl[ nIndex ]._bExt;
        }
    }

    return nId;
}

static String GetDescriptionByFactory_Impl( const String& rFactory )
{
    USHORT nResId = 0;
    if ( rFactory.EqualsIgnoreCaseAscii( "swriter", 0, 7 ) )
        nResId = STR_DESCRIPTION_FACTORY_WRITER;
    else if ( rFactory.EqualsIgnoreCaseAscii( "scalc", 0, 5 ) )
        nResId = STR_DESCRIPTION_FACTORY_CALC;
    else if ( rFactory.EqualsIgnoreCaseAscii( "simpress", 0, 8 ) )
        nResId = STR_DESCRIPTION_FACTORY_IMPRESS;
    else if ( rFactory.EqualsIgnoreCaseAscii( "sdraw", 0, 5 ) )
        nResId = STR_DESCRIPTION_FACTORY_DRAW;
    else if ( rFactory.EqualsIgnoreCaseAscii( "swriter/web", 0, 11 ) )
        nResId = STR_DESCRIPTION_FACTORY_WRITERWEB;
    else if ( rFactory.EqualsIgnoreCaseAscii( "swriter/globaldocument", 0, 22 ) )
        nResId = STR_DESCRIPTION_FACTORY_GLOBALDOC;
    else if ( rFactory.EqualsIgnoreCaseAscii( "smath", 0, 5 ) )
        nResId = STR_DESCRIPTION_FACTORY_MATH;
    else if ( rFactory.EqualsIgnoreCaseAscii( "sdatabase", 0, 9 ) )
        nResId = STR_DESCRIPTION_FACTORY_DATABASE;

    String aRet;
    if ( nResId )
    {
        ::vos::OGuard aGuard( Application::GetSolarMutex() );
        aRet = String( SvtResId( nResId ) );
    }
    return aRet;
}

static USHORT GetFolderDescriptionId_Impl( const String& rURL )
{
	USHORT nRet = STR_DESCRIPTION_FOLDER;
	svtools::VolumeInfo aVolumeInfo;
	try
	{
		::ucbhelper::Content aCnt( rURL, ::com::sun::star::uno::Reference< ::com::sun::star::ucb::XCommandEnvironment > () );
		if ( GetVolumeProperties_Impl( aCnt, aVolumeInfo ) )
		{
    		if ( aVolumeInfo.m_bIsRemote )
    			nRet = STR_DESCRIPTION_REMOTE_VOLUME;
	    	else if ( aVolumeInfo.m_bIsFloppy )
    			nRet = STR_DESCRIPTION_FLOPPY_VOLUME;
	    	else if ( aVolumeInfo.m_bIsCompactDisc )
    			nRet = STR_DESCRIPTION_CDROM_VOLUME;
	    	else if ( aVolumeInfo.m_bIsRemoveable || aVolumeInfo.m_bIsVolume )
    			nRet = STR_DESCRIPTION_LOCALE_VOLUME;
		}
	}
    catch( const ::com::sun::star::uno::RuntimeException& )
    {
        throw; // dont hide it!
    }
    catch( const ::com::sun::star::uno::Exception& )
    {
        // ...
    }
	return nRet;
}

static ResMgr* GetIsoResMgr_Impl()
{
	static ResMgr* pIsoResMgr = NULL;

	if ( !pIsoResMgr )
	{
		ByteString aResMgrName( "iso" );
		pIsoResMgr = ResMgr::CreateResMgr(
			aResMgrName.GetBuffer(), Application::GetSettings().GetUILocale() );
		if ( !pIsoResMgr )
		{
			// no "iso" resource -> search for "ooo" resource
			aResMgrName = ByteString( "ooo" );
			pIsoResMgr = ResMgr::CreateResMgr(
				aResMgrName.GetBuffer(), Application::GetSettings().GetUILocale() );
		}
	}

	return pIsoResMgr;
}

static ImageList* CreateImageList_Impl( USHORT nResId )
{
	ImageList* pList = NULL;
	ResMgr* pResMgr = GetIsoResMgr_Impl();
	DBG_ASSERT( pResMgr, "SvFileInformationManager::CreateImageList_Impl(): no resmgr" );
    ResId aResId( nResId, *pResMgr );
    aResId.SetRT( RSC_IMAGELIST );
	if ( pResMgr->IsAvailable( aResId ) )
		pList = new ImageList( aResId );
	else
		pList = new ImageList();
	return pList;
}

static Image GetOfficeImageFromList_Impl( USHORT nImageId, BOOL bBig, BOOL bHighContrast )
{
    ImageList* pList = NULL;

	static ImageList* _pSmallOfficeImgList = NULL;
	static ImageList* _pBigOfficeImgList = NULL;
	static ImageList* _pSmallHCOfficeImgList = NULL;
	static ImageList* _pBigHCOfficeImgList = NULL;
    static ULONG nStyle = Application::GetSettings().GetStyleSettings().GetSymbolsStyle();

    // If the style has been changed, throw away our cache of the older images
    if ( nStyle != Application::GetSettings().GetStyleSettings().GetSymbolsStyle() )
    {
        delete _pSmallOfficeImgList, _pSmallOfficeImgList = NULL;
        delete _pBigOfficeImgList, _pBigOfficeImgList = NULL;
        delete _pSmallHCOfficeImgList, _pSmallHCOfficeImgList = NULL;
        delete _pBigHCOfficeImgList, _pBigHCOfficeImgList = NULL;
        nStyle = Application::GetSettings().GetStyleSettings().GetSymbolsStyle();
    }

    // #i21242# MT: For B&W we need the HC Image and must transform.
    // bHiContrast is TRUE for all dark backgrounds, but we need HC Images for HC White also,
    // so we can't rely on bHighContrast.
    BOOL bBlackAndWhite = Application::GetSettings().GetStyleSettings().IsHighContrastBlackAndWhite();
    if ( bBlackAndWhite )
        bHighContrast = TRUE;


    if ( bBig )
    {
		if ( bHighContrast )
		{
        	if ( !_pBigHCOfficeImgList )
				_pBigHCOfficeImgList = CreateImageList_Impl( RID_SVTOOLS_IMAGELIST_BIG_HIGHCONTRAST );
	        pList = _pBigHCOfficeImgList;
		}
		else
		{
        	if ( !_pBigOfficeImgList )
				_pBigOfficeImgList = CreateImageList_Impl( RID_SVTOOLS_IMAGELIST_BIG );
        	pList = _pBigOfficeImgList;
		}
    }
    else
    {
		if ( bHighContrast )
		{
        	if ( !_pSmallHCOfficeImgList )
				_pSmallHCOfficeImgList = CreateImageList_Impl( RID_SVTOOLS_IMAGELIST_SMALL_HIGHCONTRAST );
	        pList = _pSmallHCOfficeImgList;
		}
		else
		{
        	if ( !_pSmallOfficeImgList )
				_pSmallOfficeImgList = CreateImageList_Impl( RID_SVTOOLS_IMAGELIST_SMALL );
	        pList = _pSmallOfficeImgList;
		}
    }

	Image aImage = pList->GetImage( nImageId );

    if ( bBlackAndWhite )
    {
        // First invert the Image, because it's designed for black background, structures are bright
		aImage.Invert();
        // Now make monochrome...
        ImageColorTransform eTrans = IMAGECOLORTRANSFORM_MONOCHROME_WHITE;
        if ( Application::GetSettings().GetStyleSettings().GetFaceColor().GetColor() == COL_WHITE )
            eTrans = IMAGECOLORTRANSFORM_MONOCHROME_BLACK;
        aImage = aImage.GetColorTransformedImage( eTrans );
    }

	return aImage;
}

static Image GetImageFromList_Impl( USHORT nImageId, BOOL bBig, BOOL bHighContrast )
{
	if ( !bBig && IMG_FOLDER == nImageId && !bHighContrast )
		// return our new small folder image (256 colors)
		return Image( SvtResId( IMG_SVT_FOLDER ) );

    ImageList* pList = NULL;

	static ImageList* _pSmallImageList = NULL;
	static ImageList* _pBigImageList = NULL;
	static ImageList* _pSmallHCImageList = NULL;
	static ImageList* _pBigHCImageList = NULL;
    static ULONG nStyle = Application::GetSettings().GetStyleSettings().GetSymbolsStyle();

    // If the style has been changed, throw away our cache of the older images
    if ( nStyle != Application::GetSettings().GetStyleSettings().GetSymbolsStyle() )
    {
        delete _pSmallImageList, _pSmallImageList = NULL;
        delete _pBigImageList, _pBigImageList = NULL;
        delete _pSmallHCImageList, _pSmallHCImageList = NULL;
        delete _pBigHCImageList, _pBigHCImageList = NULL;
        nStyle = Application::GetSettings().GetStyleSettings().GetSymbolsStyle();
    }

    if ( bBig )
    {
		if ( bHighContrast )
		{
        	if ( !_pBigHCImageList )
				_pBigHCImageList = new ImageList( SvtResId( RID_SVTOOLS_IMAGELIST_BIG_HIGHCONTRAST ) );
	        pList = _pBigHCImageList;
		}
		else
		{
        	if ( !_pBigImageList )
				_pBigImageList = new ImageList( SvtResId( RID_SVTOOLS_IMAGELIST_BIG ) );
        	pList = _pBigImageList;
		}
    }
    else
    {
		if ( bHighContrast )
		{
        	if ( !_pSmallHCImageList )
				_pSmallHCImageList = new ImageList( SvtResId( RID_SVTOOLS_IMAGELIST_SMALL_HIGHCONTRAST ) );
	        pList = _pSmallHCImageList;
		}
		else
		{
        	if ( !_pSmallImageList )
				_pSmallImageList = new ImageList( SvtResId( RID_SVTOOLS_IMAGELIST_SMALL ) );
	        pList = _pSmallImageList;
		}
    }

	if ( pList->HasImageAtPos( nImageId ) )
		return pList->GetImage( nImageId );
	else
		return GetOfficeImageFromList_Impl( nImageId, bBig, bHighContrast );
}

//****************************************************************************

void ReplaceStarOfficeVar( String& _rDescription )
{
    static String sVariable( RTL_CONSTASCII_STRINGPARAM( "%STAROFFICE" ) );
    static String sProductName;
    if ( sProductName.Len() == 0 )
    {
        ::rtl::OUString sTemp;
        ::utl::ConfigManager::GetDirectConfigProperty( ::utl::ConfigManager::PRODUCTNAME ) >>= sTemp;
        if ( sTemp.equalsAscii( "StarSuite" ) == sal_False )
            sProductName = String::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM( "StarOffice" ) );
        else
            sProductName = String( sTemp );
    }
    _rDescription.SearchAndReplace( sVariable, sProductName );
}

String SvFileInformationManager::GetDescription_Impl( const INetURLObject& rObject, sal_Bool bDetectFolder )
{
    RTL_LOGFILE_CONTEXT_AUTHOR( aTimeLog, "svtools", "hb93813", "SvFileInformationManager::GetDescription_Impl()" );

    String sDescription;
	String sExtension( rObject.getExtension() ), sURL( rObject.GetMainURL( INetURLObject::NO_DECODE ) );
    USHORT nResId = 0;
    sal_Bool bShowExt = sal_False, bDetected = sal_False, bOnlyFile = sal_False;
    sal_Bool bFolder = bDetectFolder ? CONTENT_HELPER::IsFolder( sURL ) : sal_False;
    if ( !bFolder )
    {
        if ( !bDetected )
        {
            if ( rObject.GetProtocol() == INET_PROT_PRIVATE )
            {
                String aURLPath = sURL.Copy( URL_PREFIX_PRIV_SOFFICE_LEN );
                String aType = aURLPath.GetToken( 0, INET_PATH_TOKEN );
                if ( aType == String( RTL_CONSTASCII_STRINGPARAM("factory") ) )
                {
                    sDescription = GetDescriptionByFactory_Impl( aURLPath.Copy( aURLPath.Search( INET_PATH_TOKEN ) + 1 ) );
                    bDetected = sal_True;
                }
            }

            if ( !bDetected )
            {
                // search a description by extension
				sal_Bool bExt = ( sExtension.Len() > 0 );
                if ( bExt )
				{
                	sExtension.ToLowerAscii();
                    nResId = GetDescriptionId_Impl( sExtension, bShowExt );
				}
				if ( !nResId )
				{
                    nResId = STR_DESCRIPTION_FILE;
					bOnlyFile = bExt;
				}
            }
        }
    }
	else
		nResId = GetFolderDescriptionId_Impl( sURL );

	if ( nResId > 0 )
    {
		if ( bOnlyFile )
		{
			bShowExt = sal_False;
			sExtension.ToUpperAscii();
			sDescription = sExtension;
			sDescription += '-';
		}
        ::vos::OGuard aGuard( Application::GetSolarMutex() );
        sDescription += String( SvtResId( nResId ) );
    }

    DBG_ASSERT( sDescription.Len() > 0, "file without description" );

    if ( bShowExt )
    {
        sDescription += String( RTL_CONSTASCII_STRINGPARAM(" (") );
        sDescription += sExtension;
        sDescription += ')';
    }

    ReplaceStarOfficeVar( sDescription );
    return sDescription;
}

Image SvFileInformationManager::GetImage( const INetURLObject& rObject, sal_Bool bBig )
{
	return GetImage( rObject, bBig, FALSE );
}

Image SvFileInformationManager::GetFileImage( const INetURLObject& rObject, sal_Bool bBig )
{
	return GetFileImage( rObject, bBig, FALSE );
}

Image SvFileInformationManager::GetImageNoDefault( const INetURLObject& rObject, sal_Bool bBig )
{
	return GetImageNoDefault( rObject, bBig, FALSE );
}

Image SvFileInformationManager::GetFolderImage( const svtools::VolumeInfo& rInfo, sal_Bool bBig )
{
	return GetFolderImage( rInfo, bBig, FALSE );
}

Image SvFileInformationManager::GetImage( const INetURLObject& rObject, sal_Bool bBig, sal_Bool bHighContrast )
{
    USHORT nImage = GetImageId_Impl( rObject, sal_True );
    DBG_ASSERT( nImage, "invalid ImageId" );
	return GetImageFromList_Impl( nImage, bBig, bHighContrast );
}

Image SvFileInformationManager::GetFileImage( const INetURLObject& rObject, sal_Bool bBig, sal_Bool bHighContrast )
{
    USHORT nImage = GetImageId_Impl( rObject, sal_False );
    DBG_ASSERT( nImage, "invalid ImageId" );
	return GetImageFromList_Impl( nImage, bBig, bHighContrast );
}

Image SvFileInformationManager::GetImageNoDefault( const INetURLObject& rObject, sal_Bool bBig, sal_Bool bHighContrast )
{
    USHORT nImage = GetImageId_Impl( rObject, sal_True );
    DBG_ASSERT( nImage, "invalid ImageId" );

    if ( nImage == IMG_FILE )
        return Image();

	return GetImageFromList_Impl( nImage, bBig, bHighContrast );
}

Image SvFileInformationManager::GetFolderImage( const svtools::VolumeInfo& rInfo, sal_Bool bBig, sal_Bool bHighContrast )
{
    USHORT nImage = IMG_FOLDER;
    DBG_ASSERT( nImage, "invalid ImageId" );

	if ( rInfo.m_bIsRemote )
    	nImage = IMG_NETWORKDEV;
	else if ( rInfo.m_bIsCompactDisc )
		nImage = IMG_CDROMDEV;
    else if ( rInfo.m_bIsRemoveable || rInfo.m_bIsFloppy )
    	nImage = IMG_REMOVEABLEDEV;
	else if ( rInfo.m_bIsVolume )
    	nImage = IMG_FIXEDDEV;

	return GetImageFromList_Impl( nImage, bBig, bHighContrast );
}

String SvFileInformationManager::GetDescription( const INetURLObject& rObject )
{
	return SvFileInformationManager::GetDescription_Impl( rObject, sal_True );
}

String SvFileInformationManager::GetFileDescription( const INetURLObject& rObject )
{
	return SvFileInformationManager::GetDescription_Impl( rObject, sal_False );
}

String SvFileInformationManager::GetFolderDescription( const svtools::VolumeInfo& rInfo )
{
	USHORT nResId = STR_DESCRIPTION_FOLDER;
    if ( rInfo.m_bIsRemote )
    	nResId = STR_DESCRIPTION_REMOTE_VOLUME;
	else if ( rInfo.m_bIsFloppy )
    	nResId = STR_DESCRIPTION_FLOPPY_VOLUME;
	else if ( rInfo.m_bIsCompactDisc )
    	nResId = STR_DESCRIPTION_CDROM_VOLUME;
	else if ( rInfo.m_bIsRemoveable || rInfo.m_bIsVolume )
    	nResId = STR_DESCRIPTION_LOCALE_VOLUME;

	String sDescription = String( SvtResId( nResId ) );
    return sDescription;
}

