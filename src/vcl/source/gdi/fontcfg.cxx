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
#include "precompiled_vcl.hxx"
#include <vcl/fontcfg.hxx>
#include <vcl/configsettings.hxx>
#include <vcl/outdev.hxx>
#include <vcl/svdata.hxx>
#include <vcl/svapp.hxx>
#include <vcl/unohelp.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <unotools/configpathes.hxx>
#include <rtl/ustrbuf.hxx>

#if OSL_DEBUG_LEVEL > 1
#include <stdio.h>
#endif

#include <string.h>
#include <list>
#include <algorithm>

#define DEFAULTFONT_CONFIGNODE "VCL/DefaultFonts"
#define SUBSTFONT_CONFIGNODE "VCL/FontSubstitutions"
#define SETTINGS_CONFIGNODE "VCL/Settings"

using namespace vcl;
using namespace rtl;
using namespace utl;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::container;


/*
 * DefaultFontConfiguration
 */

static const char* getKeyType( int nKeyType )
{
    switch( nKeyType )
    {
    case DEFAULTFONT_CJK_DISPLAY: return "CJK_DISPLAY";
    case DEFAULTFONT_CJK_HEADING: return "CJK_HEADING";
    case DEFAULTFONT_CJK_PRESENTATION: return "CJK_PRESENTATION";
    case DEFAULTFONT_CJK_SPREADSHEET: return "CJK_SPREADSHEET";
    case DEFAULTFONT_CJK_TEXT: return "CJK_TEXT";
    case DEFAULTFONT_CTL_DISPLAY: return "CTL_DISPLAY";
    case DEFAULTFONT_CTL_HEADING: return "CTL_HEADING";
    case DEFAULTFONT_CTL_PRESENTATION: return "CTL_PRESENTATION";
    case DEFAULTFONT_CTL_SPREADSHEET: return "CTL_SPREADSHEET";
    case DEFAULTFONT_CTL_TEXT: return "CTL_TEXT";
    case DEFAULTFONT_FIXED: return "FIXED";
    case DEFAULTFONT_LATIN_DISPLAY: return "LATIN_DISPLAY";
    case DEFAULTFONT_LATIN_FIXED: return "LATIN_FIXED";
    case DEFAULTFONT_LATIN_HEADING: return "LATIN_HEADING";
    case DEFAULTFONT_LATIN_PRESENTATION: return "LATIN_PRESENTATION";
    case DEFAULTFONT_LATIN_SPREADSHEET: return "LATIN_SPREADSHEET";
    case DEFAULTFONT_LATIN_TEXT: return "LATIN_TEXT";
    case DEFAULTFONT_SANS: return "SANS";
    case DEFAULTFONT_SANS_UNICODE: return "SANS_UNICODE";
    case DEFAULTFONT_SERIF: return "SERIF";
    case DEFAULTFONT_SYMBOL: return "SYMBOL";
    case DEFAULTFONT_UI_FIXED: return "UI_FIXED";
    case DEFAULTFONT_UI_SANS: return "UI_SANS";
    default:
        DBG_ERROR( "unmatched type" );
        return "";
    }
}

DefaultFontConfiguration* DefaultFontConfiguration::get()
{
    ImplSVData* pSVData = ImplGetSVData();
    if( ! pSVData->maGDIData.mpDefaultFontConfiguration )
        pSVData->maGDIData.mpDefaultFontConfiguration = new DefaultFontConfiguration();
    return pSVData->maGDIData.mpDefaultFontConfiguration;
}

DefaultFontConfiguration::DefaultFontConfiguration()
{
    try
    {
        // get service provider
        Reference< XMultiServiceFactory > xSMgr( unohelper::GetMultiServiceFactory() );
        // create configuration hierachical access name
        if( xSMgr.is() )
        {
            try
            {
                m_xConfigProvider =
                    Reference< XMultiServiceFactory >(
                        xSMgr->createInstance( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(
                                        "com.sun.star.configuration.ConfigurationProvider" ))),
                        UNO_QUERY );
                if( m_xConfigProvider.is() )
                {
                    Sequence< Any > aArgs(1);
                    PropertyValue aVal;
                    aVal.Name = OUString( RTL_CONSTASCII_USTRINGPARAM( "nodepath" ) );
                    aVal.Value <<= OUString( RTL_CONSTASCII_USTRINGPARAM( "/org.openoffice.VCL/DefaultFonts" ) );
                    aArgs.getArray()[0] <<= aVal;
                    m_xConfigAccess =
                        Reference< XNameAccess >(
                            m_xConfigProvider->createInstanceWithArguments( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(
                                                "com.sun.star.configuration.ConfigurationAccess" )),
                                                                            aArgs ),
                            UNO_QUERY );
                    if( m_xConfigAccess.is() )
                    {
                        Sequence< OUString > aLocales = m_xConfigAccess->getElementNames();
                        // fill config hash with empty interfaces
                        int nLocales = aLocales.getLength();
                        const OUString* pLocaleStrings = aLocales.getConstArray();
                        Locale aLoc;
                        for( int i = 0; i < nLocales; i++ )
                        {
                            sal_Int32 nIndex = 0;
                            aLoc.Language = pLocaleStrings[i].getToken( 0, sal_Unicode('-'), nIndex ).toAsciiLowerCase();
                            if( nIndex != -1 )
                                aLoc.Country = pLocaleStrings[i].getToken( 0, sal_Unicode('-'), nIndex ).toAsciiUpperCase();
                            else
                                aLoc.Country = OUString();
                            if( nIndex != -1 )
                                aLoc.Variant = pLocaleStrings[i].getToken( 0, sal_Unicode('-'), nIndex ).toAsciiUpperCase();
                            else
                                aLoc.Variant = OUString();
                            m_aConfig[ aLoc ] = LocaleAccess();
                            m_aConfig[ aLoc ].aConfigLocaleString = pLocaleStrings[i];
                        }
                    }
                }
            }
            catch( Exception& )
            {
                // configuration is awry
                m_xConfigProvider.clear();
                m_xConfigAccess.clear();
            }
        }
    }
    catch( WrappedTargetException& )
    {
    }
    #if OSL_DEBUG_LEVEL > 1
    fprintf( stderr, "config provider: %s, config access: %s\n",
             m_xConfigProvider.is() ? "true" : "false",
             m_xConfigAccess.is() ? "true" : "false"
             );
    #endif
}

DefaultFontConfiguration::~DefaultFontConfiguration()
{
    // release all nodes
    m_aConfig.clear();
    // release top node
    m_xConfigAccess.clear();
    // release config provider
    m_xConfigProvider.clear();
}

OUString DefaultFontConfiguration::tryLocale( const Locale& rLocale, const OUString& rType ) const
{
    OUString aRet;

    std::hash_map< Locale, LocaleAccess, LocaleHash >::const_iterator it =
        m_aConfig.find( rLocale );
    if( it != m_aConfig.end() )
    {
        if( !it->second.xAccess.is() )
        {
            try
            {
                Reference< XNameAccess > xNode;
                if ( m_xConfigAccess->hasByName( it->second.aConfigLocaleString ) )
                {
                    Any aAny = m_xConfigAccess->getByName( it->second.aConfigLocaleString );
                	if( aAny >>= xNode )
                	    it->second.xAccess = xNode;
				}
            }
            catch( NoSuchElementException )
            {
            }
            catch( WrappedTargetException )
            {
            }
        }
        if( it->second.xAccess.is() )
        {
            try
            {
                if ( it->second.xAccess->hasByName( rType ) )
                {
                    Any aAny = it->second.xAccess->getByName( rType );
                    aAny >>= aRet;
                }
            }
            catch( NoSuchElementException& )
            {
            }
            catch( WrappedTargetException& )
            {
            }
        }
    }
    
    return aRet;
}

OUString DefaultFontConfiguration::getDefaultFont( const Locale& rLocale, int nType ) const
{
    Locale aLocale;
    aLocale.Language = rLocale.Language.toAsciiLowerCase();
    aLocale.Country = rLocale.Country.toAsciiUpperCase();
    aLocale.Variant = rLocale.Variant.toAsciiUpperCase();
    
    OUString aType = OUString::createFromAscii( getKeyType( nType ) );
    OUString aRet = tryLocale( aLocale, aType );
    if( ! aRet.getLength() && aLocale.Variant.getLength() )
    {
        aLocale.Variant = OUString();
        aRet = tryLocale( aLocale, aType );
    }
    if( ! aRet.getLength() && aLocale.Country.getLength() )
    {
        aLocale.Country = OUString();
        aRet = tryLocale( aLocale, aType );
    }
    if( ! aRet.getLength() )
    {
        aLocale.Language = OUString( RTL_CONSTASCII_USTRINGPARAM( "en" ) );
        aRet = tryLocale( aLocale, aType );
    }
    return aRet;
}

OUString DefaultFontConfiguration::getUserInterfaceFont( const Locale& rLocale ) const
{
    Locale aLocale = rLocale;
    if( ! aLocale.Language.getLength() )
        aLocale = Application::GetSettings().GetUILocale();

    OUString aUIFont = getDefaultFont( aLocale, DEFAULTFONT_UI_SANS );

    if( aUIFont.getLength() )
        return aUIFont;

    // fallback mechanism (either no configuration or no entry in configuration

    #define FALLBACKFONT_UI_SANS "Andale Sans UI;Albany;Albany AMT;Tahoma;Arial Unicode MS;Arial;Nimbus Sans L;Bitstream Vera Sans;gnu-unifont;Interface User;Geneva;WarpSans;Dialog;Swiss;Lucida;Helvetica;Charcoal;Chicago;MS Sans Serif;Helv;Times;Times New Roman;Interface System"
    #define FALLBACKFONT_UI_SANS_LATIN2 "Andale Sans UI;Albany;Albany AMT;Tahoma;Arial Unicode MS;Arial;Nimbus Sans L;Luxi Sans;Bitstream Vera Sans;Interface User;Geneva;WarpSans;Dialog;Swiss;Lucida;Helvetica;Charcoal;Chicago;MS Sans Serif;Helv;Times;Times New Roman;Interface System"
    #define FALLBACKFONT_UI_SANS_ARABIC "Tahoma;Traditional Arabic;Simplified Arabic;Lucidasans;Lucida Sans;Supplement;Andale Sans UI;clearlyU;Interface User;Arial Unicode MS;Lucida Sans Unicode;WarpSans;Geneva;MS Sans Serif;Helv;Dialog;Albany;Lucida;Helvetica;Charcoal;Chicago;Arial;Helmet;Interface System;Sans Serif"
    #define FALLBACKFONT_UI_SANS_THAI "OONaksit;Tahoma;Lucidasans;Arial Unicode MS"
    #define FALLBACKFONT_UI_SANS_KOREAN "SunGulim;BaekmukGulim;Gulim;Roundgothic;Arial Unicode MS;Lucida Sans Unicode;gnu-unifont;Andale Sans UI"
    #define FALLBACKFONT_UI_SANS_JAPANESE1 "HG-GothicB-Sun;Andale Sans UI;HG MhinchoLightJ"
    #define FALLBACKFONT_UI_SANS_JAPANESE2 "Kochi Gothic;Gothic"
    #define FALLBACKFONT_UI_SANS_CHINSIM "Andale Sans UI;Arial Unicode MS;ZYSong18030;AR PL SungtiL GB;AR PL KaitiM GB;SimSun;Lucida Sans Unicode;Fangsong;Hei;Song;Kai;Ming;gnu-unifont;Interface User;"
    #define FALLBACKFONT_UI_SANS_CHINTRD "Andale Sans UI;Arial Unicode MS;AR PL Mingti2L Big5;AR PL KaitiM Big5;Kai;PMingLiU;MingLiU;Ming;Lucida Sans Unicode;gnu-unifont;Interface User;"

    // we need localized names for japanese fonts
    static sal_Unicode const aMSGothic[] = { 0xFF2D, 0xFF33, ' ', 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0, 0 };
    static sal_Unicode const aMSPGothic[] = { 0xFF2D, 0xFF33, ' ', 0xFF30, 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0, 0 };
    static sal_Unicode const aTLPGothic[] = { 0x0054, 0x004C, 0x0050, 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0, 0 };
    static sal_Unicode const aLXGothic[] = { 0x004C, 0x0058, 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0, 0 };
    static sal_Unicode const aKochiGothic[] = { 0x6771, 0x98A8, 0x30B4, 0x30B7, 0x30C3, 0x30AF, 0, 0 };

    String aFallBackJapaneseLocalized( RTL_CONSTASCII_USTRINGPARAM( "MS UI Gothic;" ) );
    aFallBackJapaneseLocalized += String( RTL_CONSTASCII_USTRINGPARAM( FALLBACKFONT_UI_SANS_JAPANESE1 ) );
    aFallBackJapaneseLocalized += String( aMSPGothic );
    aFallBackJapaneseLocalized += String(RTL_CONSTASCII_USTRINGPARAM( ";" ) );
    aFallBackJapaneseLocalized += String( aMSGothic );
    aFallBackJapaneseLocalized += String(RTL_CONSTASCII_USTRINGPARAM( ";" ) );
    aFallBackJapaneseLocalized += String( aTLPGothic );
    aFallBackJapaneseLocalized += String(RTL_CONSTASCII_USTRINGPARAM( ";" ) );
    aFallBackJapaneseLocalized += String( aLXGothic );
    aFallBackJapaneseLocalized += String(RTL_CONSTASCII_USTRINGPARAM( ";" ) );
    aFallBackJapaneseLocalized += String( aKochiGothic );
    aFallBackJapaneseLocalized += String(RTL_CONSTASCII_USTRINGPARAM( ";" ) );
    aFallBackJapaneseLocalized += String(RTL_CONSTASCII_USTRINGPARAM( FALLBACKFONT_UI_SANS_JAPANESE2 ) );
    static const OUString aFallBackJapanese( aFallBackJapaneseLocalized );
    static const OUString aFallback (RTL_CONSTASCII_USTRINGPARAM(FALLBACKFONT_UI_SANS));
    static const OUString aFallbackLatin2 (RTL_CONSTASCII_USTRINGPARAM(FALLBACKFONT_UI_SANS_LATIN2));
    static const OUString aFallBackArabic (RTL_CONSTASCII_USTRINGPARAM( FALLBACKFONT_UI_SANS_ARABIC ) );
    static const OUString aFallBackThai (RTL_CONSTASCII_USTRINGPARAM( FALLBACKFONT_UI_SANS_THAI ) );
    static const OUString aFallBackChineseSIM (RTL_CONSTASCII_USTRINGPARAM( FALLBACKFONT_UI_SANS_CHINSIM ) );
    static const OUString aFallBackChineseTRD (RTL_CONSTASCII_USTRINGPARAM( FALLBACKFONT_UI_SANS_CHINTRD ) );

    // we need localized names for korean fonts
    static sal_Unicode const aSunGulim[] = { 0xC36C, 0xAD74, 0xB9BC, 0 };
    static sal_Unicode const aBaekmukGulim[] = { 0xBC31, 0xBC35, 0xAD74, 0xB9BC, 0 };
    String aFallBackKoreanLocalized( aSunGulim );
    aFallBackKoreanLocalized += String(RTL_CONSTASCII_USTRINGPARAM( ";" ) );
    aFallBackKoreanLocalized += String( aBaekmukGulim );
    aFallBackKoreanLocalized += String(RTL_CONSTASCII_USTRINGPARAM( ";" ) );
    aFallBackKoreanLocalized += String(RTL_CONSTASCII_USTRINGPARAM( FALLBACKFONT_UI_SANS_KOREAN ) );
    static const OUString aFallBackKorean( aFallBackKoreanLocalized );

    // optimize font list for some locales, as long as Andale Sans UI does not support them
    if( aLocale.Language.equalsAscii( "ar" ) ||
        aLocale.Language.equalsAscii( "he" ) ||
        aLocale.Language.equalsAscii( "iw" ) )
    {
        return aFallBackArabic;
    }
    else if( aLocale.Language.equalsAscii( "th" ) )
    {
        return aFallBackThai;
    }
    else if( aLocale.Language.equalsAscii( "ko" ) )
    {
        return aFallBackKorean;
    }
    else if( aLocale.Language.equalsAscii( "cs" ) ||
             aLocale.Language.equalsAscii( "hu" ) ||
             aLocale.Language.equalsAscii( "pl" ) ||
             aLocale.Language.equalsAscii( "ro" ) ||
             aLocale.Language.equalsAscii( "rm" ) ||
             aLocale.Language.equalsAscii( "hr" ) ||
             aLocale.Language.equalsAscii( "sk" ) ||
             aLocale.Language.equalsAscii( "sl" ) ||
             aLocale.Language.equalsAscii( "sb" ) )
    {
        return aFallbackLatin2;
    }
    else if( aLocale.Language.equalsAscii( "zh" ) )
    {
        if( ! aLocale.Country.equalsAscii( "cn" ) )
            return aFallBackChineseTRD;
        else
            return aFallBackChineseSIM;
    }
    else if( aLocale.Language.equalsAscii( "ja" ) )
    {
        return aFallBackJapanese;
    }

   return aFallback;
}

// ------------------------------------------------------------------------------------

/*
 *  FontSubstConfigItem::get
 */

FontSubstConfiguration* FontSubstConfiguration::get()
{
    ImplSVData* pSVData = ImplGetSVData();
    if( ! pSVData->maGDIData.mpFontSubstConfiguration )
        pSVData->maGDIData.mpFontSubstConfiguration = new FontSubstConfiguration();
    return pSVData->maGDIData.mpFontSubstConfiguration;
}

/*
 *  FontSubstConfigItem::FontSubstConfigItem
 */

FontSubstConfiguration::FontSubstConfiguration() :
	maSubstHash( 300 )
{
    try
    {
        // get service provider
        Reference< XMultiServiceFactory > xSMgr( unohelper::GetMultiServiceFactory() );
        // create configuration hierachical access name
        if( xSMgr.is() )
        {
            try
            {
                m_xConfigProvider =
                    Reference< XMultiServiceFactory >(
                        xSMgr->createInstance( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(
                                        "com.sun.star.configuration.ConfigurationProvider" ))),
                        UNO_QUERY );
                if( m_xConfigProvider.is() )
                {
                    Sequence< Any > aArgs(1);
                    PropertyValue aVal;
                    aVal.Name = OUString( RTL_CONSTASCII_USTRINGPARAM( "nodepath" ) );
                    aVal.Value <<= OUString( RTL_CONSTASCII_USTRINGPARAM( "/org.openoffice.VCL/FontSubstitutions" ) );
                    aArgs.getArray()[0] <<= aVal;
                    m_xConfigAccess =
                        Reference< XNameAccess >(
                            m_xConfigProvider->createInstanceWithArguments( rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(
                                                "com.sun.star.configuration.ConfigurationAccess" )),
                                                                            aArgs ),
                            UNO_QUERY );
                    if( m_xConfigAccess.is() )
                    {
                        Sequence< OUString > aLocales = m_xConfigAccess->getElementNames();
                        // fill config hash with empty interfaces
                        int nLocales = aLocales.getLength();
                        const OUString* pLocaleStrings = aLocales.getConstArray();
                        Locale aLoc;
                        for( int i = 0; i < nLocales; i++ )
                        {
                            sal_Int32 nIndex = 0;
                            aLoc.Language = pLocaleStrings[i].getToken( 0, sal_Unicode('-'), nIndex ).toAsciiLowerCase();
                            if( nIndex != -1 )
                                aLoc.Country = pLocaleStrings[i].getToken( 0, sal_Unicode('-'), nIndex ).toAsciiUpperCase();
                            else
                                aLoc.Country = OUString();
                            if( nIndex != -1 )
                                aLoc.Variant = pLocaleStrings[i].getToken( 0, sal_Unicode('-'), nIndex ).toAsciiUpperCase();
                            else
                                aLoc.Variant = OUString();
                            m_aSubst[ aLoc ] = LocaleSubst();
                            m_aSubst[ aLoc ].aConfigLocaleString = pLocaleStrings[i];
                        }
                    }
                }
            }
            catch( Exception& )
            {
                // configuration is awry
                m_xConfigProvider.clear();
                m_xConfigAccess.clear();
            }
        }
    }
    catch( WrappedTargetException& )
    {
    }
    #if OSL_DEBUG_LEVEL > 1
    fprintf( stderr, "config provider: %s, config access: %s\n",
             m_xConfigProvider.is() ? "true" : "false",
             m_xConfigAccess.is() ? "true" : "false"
             );
    #endif
}

/*
 *  FontSubstConfigItem::~FontSubstConfigItem
 */

FontSubstConfiguration::~FontSubstConfiguration()
{
    // release config access
    m_xConfigAccess.clear();
    // release config provider
    m_xConfigProvider.clear();
}

/*
 *  FontSubstConfigItem::getMapName
 */
// =======================================================================

static const char* const aImplKillLeadingList[] =
{
    "microsoft",
    "monotype",
    "linotype",
    "baekmuk",
    "adobe",
    "nimbus",
    "zycjk",
    "itc",
    "sun",
    "amt",
    "ms",
    "mt",
    "cg",
    "hg",
    "fz",
    "ipa",
    "sazanami",
    "kochi",
    NULL
};

// -----------------------------------------------------------------------

static const char* const aImplKillTrailingList[] =
{
    "microsoft",
    "monotype",
    "linotype",
    "adobe",
    "nimbus",
    "itc",
    "sun",
    "amt",
    "ms",
    "mt",
    "clm",
    // Scripts, for compatibility with older versions
    "we",
    "cyr",
    "tur",
    "wt",
    "greek",
    "wl",
    // CJK extensions
    "gb",
    "big5",
    "pro",
    "z01",
    "z02",
    "z03",
    "z13",
    "b01",
    "w3x12",
    // Old Printer Fontnames
    "5cpi",
    "6cpi",
    "7cpi",
    "8cpi",
    "9cpi",
    "10cpi",
    "11cpi",
    "12cpi",
    "13cpi",
    "14cpi",
    "15cpi",
    "16cpi",
    "18cpi",
    "24cpi",
    "scale",
    "pc",
    NULL
};

// -----------------------------------------------------------------------

static const char* const aImplKillTrailingWithExceptionsList[] =
{
    "ce", "monospace", "oldface", NULL,
    "ps", "caps", NULL,
    NULL
};

// -----------------------------------------------------------------------

struct ImplFontAttrWeightSearchData
{
    const char*             mpStr;
    FontWeight              meWeight;
};

static ImplFontAttrWeightSearchData const aImplWeightAttrSearchList[] =
{
// the attribute names are ordered by "first match wins"
// e.g. "semilight" should wins over "semi" 
{   "extrablack",           WEIGHT_BLACK },
{   "ultrablack",           WEIGHT_BLACK },
{   "ultrabold",            WEIGHT_ULTRABOLD },
{   "semibold",             WEIGHT_SEMIBOLD },
{   "semilight",            WEIGHT_SEMILIGHT },
{   "semi",                 WEIGHT_SEMIBOLD },
{   "demi",                 WEIGHT_SEMIBOLD },
{   "black",                WEIGHT_BLACK },
{   "bold",                 WEIGHT_BOLD },
{   "heavy",                WEIGHT_BLACK },
{   "ultralight",           WEIGHT_ULTRALIGHT },
{   "light",                WEIGHT_LIGHT },
{   "medium",               WEIGHT_MEDIUM },
{   NULL,                   WEIGHT_DONTKNOW },
};

// -----------------------------------------------------------------------

struct ImplFontAttrWidthSearchData
{
    const char*             mpStr;
    FontWidth               meWidth;
};

static ImplFontAttrWidthSearchData const aImplWidthAttrSearchList[] =
{
{   "narrow",               WIDTH_CONDENSED },
{   "semicondensed",        WIDTH_SEMI_CONDENSED },
{   "ultracondensed",       WIDTH_ULTRA_CONDENSED },
{   "semiexpanded",         WIDTH_SEMI_EXPANDED },
{   "ultraexpanded",        WIDTH_ULTRA_EXPANDED },
{   "expanded",             WIDTH_EXPANDED },
{   "wide",                 WIDTH_ULTRA_EXPANDED },
{   "condensed",            WIDTH_CONDENSED },
{   "cond",                 WIDTH_CONDENSED },
{   "cn",                   WIDTH_CONDENSED },
{   NULL,                   WIDTH_DONTKNOW },
};

struct ImplFontAttrTypeSearchData
{
    const char*             mpStr;
    ULONG                   mnType;
};

static ImplFontAttrTypeSearchData const aImplTypeAttrSearchList[] =
{
{   "monotype",             0 },
{   "linotype",             0 },
{   "titling",              IMPL_FONT_ATTR_TITLING },
{   "captitals",            IMPL_FONT_ATTR_CAPITALS },
{   "captital",             IMPL_FONT_ATTR_CAPITALS },
{   "caps",                 IMPL_FONT_ATTR_CAPITALS },
{   "italic",               IMPL_FONT_ATTR_ITALIC },
{   "oblique",              IMPL_FONT_ATTR_ITALIC },
{   "rounded",              IMPL_FONT_ATTR_ROUNDED },
{   "outline",              IMPL_FONT_ATTR_OUTLINE },
{   "shadow",               IMPL_FONT_ATTR_SHADOW },
{   "handwriting",          IMPL_FONT_ATTR_HANDWRITING | IMPL_FONT_ATTR_SCRIPT },
{   "hand",                 IMPL_FONT_ATTR_HANDWRITING | IMPL_FONT_ATTR_SCRIPT },
{   "signet",               IMPL_FONT_ATTR_HANDWRITING | IMPL_FONT_ATTR_SCRIPT },
{   "script",               IMPL_FONT_ATTR_BRUSHSCRIPT | IMPL_FONT_ATTR_SCRIPT },
{   "calligraphy",          IMPL_FONT_ATTR_CHANCERY | IMPL_FONT_ATTR_SCRIPT },
{   "chancery",             IMPL_FONT_ATTR_CHANCERY | IMPL_FONT_ATTR_SCRIPT },
{   "corsiva",              IMPL_FONT_ATTR_CHANCERY | IMPL_FONT_ATTR_SCRIPT },
{   "gothic",               IMPL_FONT_ATTR_SANSSERIF | IMPL_FONT_ATTR_GOTHIC },
{   "schoolbook",           IMPL_FONT_ATTR_SERIF | IMPL_FONT_ATTR_SCHOOLBOOK },
{   "schlbk",               IMPL_FONT_ATTR_SERIF | IMPL_FONT_ATTR_SCHOOLBOOK },
{   "typewriter",           IMPL_FONT_ATTR_TYPEWRITER | IMPL_FONT_ATTR_FIXED },
{   "lineprinter",          IMPL_FONT_ATTR_TYPEWRITER | IMPL_FONT_ATTR_FIXED },
{   "monospaced",           IMPL_FONT_ATTR_FIXED },
{   "monospace",            IMPL_FONT_ATTR_FIXED },
{   "mono",                 IMPL_FONT_ATTR_FIXED },
{   "fixed",                IMPL_FONT_ATTR_FIXED },
{   "sansserif",            IMPL_FONT_ATTR_SANSSERIF },
{   "sans",                 IMPL_FONT_ATTR_SANSSERIF },
{   "swiss",                IMPL_FONT_ATTR_SANSSERIF },
{   "serif",                IMPL_FONT_ATTR_SERIF },
{   "bright",               IMPL_FONT_ATTR_SERIF },
{   "symbols",              IMPL_FONT_ATTR_SYMBOL },
{   "symbol",               IMPL_FONT_ATTR_SYMBOL },
{   "dingbats",             IMPL_FONT_ATTR_SYMBOL },
{   "dings",                IMPL_FONT_ATTR_SYMBOL },
{   "ding",                 IMPL_FONT_ATTR_SYMBOL },
{   "bats",                 IMPL_FONT_ATTR_SYMBOL },
{   "math",                 IMPL_FONT_ATTR_SYMBOL },
{   "oldstyle",             IMPL_FONT_ATTR_OTHERSTYLE },
{   "oldface",              IMPL_FONT_ATTR_OTHERSTYLE },
{   "old",                  IMPL_FONT_ATTR_OTHERSTYLE },
{   "new",                  0 },
{   "modern",               0 },
{   "lucida",               0 },
{   "regular",              0 },
{   "extended",             0 },
{   "extra",                IMPL_FONT_ATTR_OTHERSTYLE },
{   "ext",                  0 },
{   "scalable",             0 },
{   "scale",                0 },
{   "nimbus",               0 },
{   "adobe",                0 },
{   "itc",                  0 },
{   "amt",                  0 },
{   "mt",                   0 },
{   "ms",                   0 },
{   "cpi",                  0 },
{   "no",                   0 },
{   NULL,                   0 },
};

// -----------------------------------------------------------------------

static bool ImplKillLeading( String& rName, const char* const* ppStr )
{
    for(; *ppStr; ++ppStr )
    {
        const char*         pStr = *ppStr;
        const xub_Unicode*  pNameStr = rName.GetBuffer();
        while ( (*pNameStr == (xub_Unicode)(unsigned char)*pStr) && *pStr )
        {
            pNameStr++;
            pStr++;
        }
        if ( !*pStr )
        {
            xub_StrLen nLen = sal::static_int_cast<xub_StrLen>(pNameStr - rName.GetBuffer());
            rName.Erase( 0, nLen );
            return true;
        }
    }

    // special case for Baekmuk
    // TODO: allow non-ASCII KillLeading list
    const xub_Unicode* pNameStr = rName.GetBuffer();
    if( (pNameStr[0]==0xBC31) && (pNameStr[1]==0xBC35) )
    {
        xub_StrLen nLen = (pNameStr[2]==0x0020) ? 3 : 2;
        rName.Erase( 0, nLen );
        return true;
    }

    return false;
}

// -----------------------------------------------------------------------

static xub_StrLen ImplIsTrailing( const String& rName, const char* pStr )
{
    xub_StrLen nStrLen = static_cast<xub_StrLen>( strlen( pStr ) );
    if( nStrLen >= rName.Len() )
        return 0;

    const xub_Unicode* pEndName = rName.GetBuffer() + rName.Len();
    const sal_Unicode* pNameStr = pEndName - nStrLen;
    do if( *(pNameStr++) != *(pStr++) )
        return 0;
    while( *pStr );

    return nStrLen;
}

// -----------------------------------------------------------------------

static bool ImplKillTrailing( String& rName, const char* const* ppStr )
{
    for(; *ppStr; ++ppStr )
    {
        xub_StrLen nTrailLen = ImplIsTrailing( rName, *ppStr );
        if( nTrailLen )
        {
            rName.Erase( rName.Len()-nTrailLen );
            return true;
        }
    }

    return false;
}

// -----------------------------------------------------------------------

static bool ImplKillTrailingWithExceptions( String& rName, const char* const* ppStr )
{
    for(; *ppStr; ++ppStr )
    {
        xub_StrLen nTrailLen = ImplIsTrailing( rName, *ppStr );
        if( nTrailLen )
        {
            // check string match against string exceptions
            while( *++ppStr )
                if( ImplIsTrailing( rName, *ppStr ) )
                    return false;

            rName.Erase( rName.Len()-nTrailLen );
            return true;
        }
        else
        {
            // skip exception strings
            while( *++ppStr ) ;
        }
    }

    return false;
}

// -----------------------------------------------------------------------

static BOOL ImplFindAndErase( String& rName, const char* pStr )
{
    xub_StrLen nPos = rName.SearchAscii( pStr );
    if ( nPos == STRING_NOTFOUND )
        return FALSE;

    const char* pTempStr = pStr;
    while ( *pTempStr )
        pTempStr++;
    rName.Erase( nPos, (xub_StrLen)(pTempStr-pStr) );
    return TRUE;
}

// =======================================================================

void FontSubstConfiguration::getMapName( const String& rOrgName, String& rShortName,
    String& rFamilyName, FontWeight& rWeight, FontWidth& rWidth, ULONG& rType )
{
    rShortName = rOrgName;

    // TODO: get rid of the crazy O(N*strlen) searches below
    // they should be possible in O(strlen)

    // Kill leading vendor names and other unimportant data
    ImplKillLeading( rShortName, aImplKillLeadingList );

    // Kill trailing vendor names and other unimportant data
    ImplKillTrailing( rShortName, aImplKillTrailingList );
    ImplKillTrailingWithExceptions( rShortName, aImplKillTrailingWithExceptionsList );

    rFamilyName = rShortName;

    // Kill attributes from the name and update the data
    // Weight
    const ImplFontAttrWeightSearchData* pWeightList = aImplWeightAttrSearchList;
    while ( pWeightList->mpStr )
    {
        if ( ImplFindAndErase( rFamilyName, pWeightList->mpStr ) )
        {
            if ( (rWeight == WEIGHT_DONTKNOW) || (rWeight == WEIGHT_NORMAL) )
                rWeight = pWeightList->meWeight;
            break;
        }
        pWeightList++;
    }

    // Width
    const ImplFontAttrWidthSearchData* pWidthList = aImplWidthAttrSearchList;
    while ( pWidthList->mpStr )
    {
        if ( ImplFindAndErase( rFamilyName, pWidthList->mpStr ) )
        {
            if ( (rWidth == WIDTH_DONTKNOW) || (rWidth == WIDTH_NORMAL) )
                rWidth = pWidthList->meWidth;
            break;
        }
        pWidthList++;
    }

    // Type
    rType = 0;
    const ImplFontAttrTypeSearchData* pTypeList = aImplTypeAttrSearchList;
    while ( pTypeList->mpStr )
    {
        if ( ImplFindAndErase( rFamilyName, pTypeList->mpStr ) )
            rType |= pTypeList->mnType;
        pTypeList++;
    }

    // Remove numbers
    // TODO: also remove localized and fullwidth digits
    xub_StrLen i = 0;
    while ( i < rFamilyName.Len() )
    {
        sal_Unicode c = rFamilyName.GetChar( i );
        if ( (c >= 0x0030) && (c <= 0x0039) )
            rFamilyName.Erase( i, 1 );
        else
            i++;
    }
}


struct StrictStringSort : public ::std::binary_function< const FontNameAttr&, const FontNameAttr&, bool >
{
    bool operator()( const FontNameAttr& rLeft, const FontNameAttr& rRight )
    { return rLeft.Name.CompareTo( rRight.Name ) == COMPARE_LESS ; }
};

static const char* const pAttribNames[] =
{
    "default",
    "standard",
    "normal",
    "symbol",
    "fixed",
    "sansserif",
    "serif",
    "decorative",
    "special",
    "italic",
    "title",
    "capitals",
    "cjk",
    "cjk_jp",
    "cjk_sc",
    "cjk_tc",
    "cjk_kr",
    "ctl",
    "nonelatin",
    "full",
    "outline",
    "shadow",
    "rounded",
    "typewriter",
    "script",
    "handwriting",
    "chancery",
    "comic",
    "brushscript",
    "gothic",
    "schoolbook",
    "other"
};

struct enum_convert
{
    const char* pName;
    int          nEnum;
};


static const enum_convert pWeightNames[] =
{
    { "normal", WEIGHT_NORMAL },
    { "medium", WEIGHT_MEDIUM },
    { "bold", WEIGHT_BOLD },
    { "black", WEIGHT_BLACK },
    { "semibold", WEIGHT_SEMIBOLD },
    { "light", WEIGHT_LIGHT },
    { "semilight", WEIGHT_SEMILIGHT },
    { "ultrabold", WEIGHT_ULTRABOLD },
    { "semi", WEIGHT_SEMIBOLD },
    { "demi", WEIGHT_SEMIBOLD },
    { "heavy", WEIGHT_BLACK },
    { "unknown", WEIGHT_DONTKNOW },
    { "thin", WEIGHT_THIN },
    { "ultralight", WEIGHT_ULTRALIGHT }
};

static const enum_convert pWidthNames[] =
{
    { "normal", WIDTH_NORMAL },
    { "condensed", WIDTH_CONDENSED },
    { "expanded", WIDTH_EXPANDED },
    { "unknown", WIDTH_DONTKNOW },
    { "ultracondensed", WIDTH_ULTRA_CONDENSED },
    { "extracondensed", WIDTH_EXTRA_CONDENSED },
    { "semicondensed", WIDTH_SEMI_CONDENSED },
    { "semiexpanded", WIDTH_SEMI_EXPANDED },
    { "extraexpanded", WIDTH_EXTRA_EXPANDED },
    { "ultraexpanded", WIDTH_ULTRA_EXPANDED }
};

void FontSubstConfiguration::fillSubstVector( const com::sun::star::uno::Reference< XNameAccess > xFont,
                                              const rtl::OUString& rType,
                                              std::vector< String >& rSubstVector ) const
{
    try
    {
        Any aAny = xFont->getByName( rType );
        if( aAny.getValueTypeClass() == TypeClass_STRING )
        {
            const OUString* pLine = (const OUString*)aAny.getValue();
            sal_Int32 nIndex = 0;
            sal_Int32 nLength = pLine->getLength();
            if( nLength )
            {
                const sal_Unicode* pStr = pLine->getStr();
                sal_Int32 nTokens = 0;
                // count tokens
                while( nLength-- )
                {
                    if( *pStr++ == sal_Unicode(';') )
                        nTokens++;
                }
                rSubstVector.clear();
                // optimize performance, heap fragmentation
                rSubstVector.reserve( nTokens );
                while( nIndex != -1 )
                {
                    OUString aSubst( pLine->getToken( 0, ';', nIndex ) );
                    if( aSubst.getLength() )
					{
						UniqueSubstHash::iterator aEntry = maSubstHash.find( aSubst );
						if (aEntry != maSubstHash.end())
							aSubst = *aEntry;
						else
							maSubstHash.insert( aSubst );
                        rSubstVector.push_back( aSubst );
					}
                }
            }
        }
    }
    catch( NoSuchElementException )
    {
    }
    catch( WrappedTargetException )
    {
    }
}

FontWeight FontSubstConfiguration::getSubstWeight( const com::sun::star::uno::Reference< XNameAccess > xFont,
                                                   const rtl::OUString& rType ) const
{
    int weight = -1;
    try
    {
        Any aAny = xFont->getByName( rType );
        if( aAny.getValueTypeClass() == TypeClass_STRING )
        {
            const OUString* pLine = (const OUString*)aAny.getValue();
            if( pLine->getLength() )
            {
                for( weight=sizeof(pWeightNames)/sizeof(pWeightNames[0])-1; weight >= 0; weight-- )
                    if( pLine->equalsIgnoreAsciiCaseAscii( pWeightNames[weight].pName ) )
                        break;
            }
#if OSL_DEBUG_LEVEL > 1
            if( weight < 0 )
                fprintf( stderr, "Error: invalid weight %s\n",
                         OUStringToOString( *pLine, RTL_TEXTENCODING_ASCII_US ).getStr() );
#endif
        }
    }
    catch( NoSuchElementException )
    {
    }
    catch( WrappedTargetException )
    {
    }
    return (FontWeight)( weight >= 0 ? pWeightNames[weight].nEnum : WEIGHT_DONTKNOW );
}

FontWidth FontSubstConfiguration::getSubstWidth( const com::sun::star::uno::Reference< XNameAccess > xFont,
                                                 const rtl::OUString& rType ) const
{
    int width = -1;
    try
    {
        Any aAny = xFont->getByName( rType );
        if( aAny.getValueTypeClass() == TypeClass_STRING )
        {
            const OUString* pLine = (const OUString*)aAny.getValue();
            if( pLine->getLength() )
            {
                for( width=sizeof(pWidthNames)/sizeof(pWidthNames[0])-1; width >= 0; width-- )
                    if( pLine->equalsIgnoreAsciiCaseAscii( pWidthNames[width].pName ) )
                        break;
            }
#if OSL_DEBUG_LEVEL > 1
            if( width < 0 )
                fprintf( stderr, "Error: invalid width %s\n",
                         OUStringToOString( *pLine, RTL_TEXTENCODING_ASCII_US ).getStr() );
#endif
        }
    }
    catch( NoSuchElementException )
    {
    }
    catch( WrappedTargetException )
    {
    }
    return (FontWidth)( width >= 0 ? pWidthNames[width].nEnum : WIDTH_DONTKNOW );
}

unsigned long FontSubstConfiguration::getSubstType( const com::sun::star::uno::Reference< XNameAccess > xFont,
                                                    const rtl::OUString& rType ) const
{
    unsigned long type = 0;
    try
    {
        Any aAny = xFont->getByName( rType );
        if( aAny.getValueTypeClass() == TypeClass_STRING )
        {
            const OUString* pLine = (const OUString*)aAny.getValue();
            if( pLine->getLength() )
            {
                sal_Int32 nIndex = 0;
                while( nIndex != -1 )
                {
                    String aToken( pLine->getToken( 0, ',', nIndex ) );
                    for( int k = 0; k < 32; k++ )
                        if( aToken.EqualsIgnoreCaseAscii( pAttribNames[k] ) )
                        {
                            type |= 1 << k;
                            break;
                        }
                }
            }
        }
    }
    catch( NoSuchElementException )
    {
    }
    catch( WrappedTargetException )
    {
    }
    
    return type;
}

void FontSubstConfiguration::readLocaleSubst( const com::sun::star::lang::Locale& rLocale ) const
{
    std::hash_map< Locale, LocaleSubst, LocaleHash >::const_iterator it =
        m_aSubst.find( rLocale );
    if( it != m_aSubst.end() )
    {
        if( ! it->second.bConfigRead )
        {
            it->second.bConfigRead = true;
            Reference< XNameAccess > xNode;
            try
            {
                Any aAny = m_xConfigAccess->getByName( it->second.aConfigLocaleString );
                aAny >>= xNode;
            }
            catch( NoSuchElementException )
            {
            }
            catch( WrappedTargetException )
            {
            }
            if( xNode.is() )
            {
                Sequence< OUString > aFonts = xNode->getElementNames();
                int nFonts = aFonts.getLength();
                const OUString* pFontNames = aFonts.getConstArray();
                // improve performance, heap fragmentation
                it->second.aSubstAttributes.reserve( nFonts );
                
                // strings for subst retrieval, construct only once
                OUString aSubstFontsStr     ( RTL_CONSTASCII_USTRINGPARAM( "SubstFonts" ) );
                OUString aSubstFontsMSStr   ( RTL_CONSTASCII_USTRINGPARAM( "SubstFontsMS" ) );
                OUString aSubstFontsPSStr   ( RTL_CONSTASCII_USTRINGPARAM( "SubstFontsPS" ) );
                OUString aSubstFontsHTMLStr ( RTL_CONSTASCII_USTRINGPARAM( "SubstFontsHTML" ) );
                OUString aSubstWeightStr    ( RTL_CONSTASCII_USTRINGPARAM( "FontWeight" ) );
                OUString aSubstWidthStr     ( RTL_CONSTASCII_USTRINGPARAM( "FontWidth" ) );
                OUString aSubstTypeStr      ( RTL_CONSTASCII_USTRINGPARAM( "FontType" ) );
                for( int i = 0; i < nFonts; i++ )
                {
                    Reference< XNameAccess > xFont;
                    try
                    {
                        Any aAny = xNode->getByName( pFontNames[i] );
                        aAny >>= xFont;
                    }
                    catch( NoSuchElementException )
                    {
                    }
                    catch( WrappedTargetException )
                    {
                    }
                    if( ! xFont.is() )
                    {
                        #if OSL_DEBUG_LEVEL > 1
                        fprintf( stderr, "did not get font attributes for %s\n",
                                 OUStringToOString( pFontNames[i], RTL_TEXTENCODING_UTF8 ).getStr() );
                        #endif
                        continue;
                    }

                    FontNameAttr aAttr;
                    // read subst attributes from config
                    aAttr.Name = pFontNames[i];
                    fillSubstVector( xFont, aSubstFontsStr, aAttr.Substitutions );
                    fillSubstVector( xFont, aSubstFontsMSStr, aAttr.MSSubstitutions );
                    fillSubstVector( xFont, aSubstFontsPSStr, aAttr.PSSubstitutions );
                    fillSubstVector( xFont, aSubstFontsHTMLStr, aAttr.HTMLSubstitutions );
                    aAttr.Weight = getSubstWeight( xFont, aSubstWeightStr );
                    aAttr.Width = getSubstWidth( xFont, aSubstWidthStr );
                    aAttr.Type = getSubstType( xFont, aSubstTypeStr );
                    
                    // finally insert this entry
                    it->second.aSubstAttributes.push_back( aAttr );
                }
                std::sort( it->second.aSubstAttributes.begin(), it->second.aSubstAttributes.end(), StrictStringSort() );
            }
        }
    }
}

const FontNameAttr* FontSubstConfiguration::getSubstInfo( const String& rFontName, const Locale& rLocale ) const
{
    if( !rFontName.Len() )
        return NULL;

    // search if a  (language dep.) replacement table for the given font exists
    // fallback is english
    String aSearchFont( rFontName );
    aSearchFont.ToLowerAscii();
    FontNameAttr aSearchAttr;
    aSearchAttr.Name = aSearchFont;

    Locale aLocale;
    aLocale.Language = rLocale.Language.toAsciiLowerCase();
    aLocale.Country = rLocale.Country.toAsciiUpperCase();
    aLocale.Variant = rLocale.Variant.toAsciiUpperCase();

    if( ! aLocale.Language.getLength() )
        aLocale = Application::GetSettings().GetUILocale();

    while( aLocale.Language.getLength() )
    {
        std::hash_map< Locale, LocaleSubst, LocaleHash >::const_iterator lang = m_aSubst.find( aLocale );
        if( lang != m_aSubst.end() )
        {
            if( ! lang->second.bConfigRead )
                readLocaleSubst( aLocale );
            // try to find an exact match
            // because the list is sorted this will also find fontnames of the form searchfontname*
            std::vector< FontNameAttr >::const_iterator it = ::std::lower_bound( lang->second.aSubstAttributes.begin(), lang->second.aSubstAttributes.end(), aSearchAttr, StrictStringSort() );
            if( it != lang->second.aSubstAttributes.end() && aSearchFont.CompareTo( it->Name, aSearchFont.Len() ) == COMPARE_EQUAL )
                return &(*it);
        }
        // gradually become more unspecific
        if( aLocale.Variant.getLength() )
            aLocale.Variant = OUString();
        else if( aLocale.Country.getLength() )
            aLocale.Country = OUString();
        else if( ! aLocale.Language.equalsAscii( "en" ) )
            aLocale.Language = OUString( RTL_CONSTASCII_USTRINGPARAM( "en" ) );
        else
            aLocale.Language = OUString();
    }
    return NULL;
}

/*
 *	SettingsConfigItem::get
 */

SettingsConfigItem* SettingsConfigItem::get()
{
    ImplSVData* pSVData = ImplGetSVData();
    if( ! pSVData->mpSettingsConfigItem )
        pSVData->mpSettingsConfigItem = new SettingsConfigItem();
    return pSVData->mpSettingsConfigItem;
}

/*
 *  SettignsConfigItem constructor
 */

SettingsConfigItem::SettingsConfigItem()
        :
        ConfigItem( OUString( RTL_CONSTASCII_USTRINGPARAM( SETTINGS_CONFIGNODE ) ),
                    CONFIG_MODE_DELAYED_UPDATE ),
	m_aSettings( 0 )
{
    getValues();
}

/*
 *  SettingsConfigItem destructor
 */

SettingsConfigItem::~SettingsConfigItem()
{
    if( IsModified() )
        Commit();
}

/*
 *  SettingsConfigItem::Commit
 */

void SettingsConfigItem::Commit()
{
    if( ! IsValidConfigMgr() )
        return;

    std::hash_map< OUString, SmallOUStrMap, rtl::OUStringHash >::const_iterator group;

    for( group = m_aSettings.begin(); group != m_aSettings.end(); ++group )
    {
        String aKeyName( group->first );
        /*sal_Bool bAdded =*/ AddNode( OUString(), aKeyName );
        Sequence< PropertyValue > aValues( group->second.size() );
        PropertyValue* pValues = aValues.getArray();
        int nIndex = 0;
        SmallOUStrMap::const_iterator it;
        for( it = group->second.begin(); it != group->second.end(); ++it )
        {
            String aName( aKeyName );
            aName.Append( '/' );
            aName.Append( String( it->first ) );
            pValues[nIndex].Name	= aName;
            pValues[nIndex].Handle	= 0;
            pValues[nIndex].Value <<= it->second;
            pValues[nIndex].State	= PropertyState_DIRECT_VALUE;
            nIndex++;
        }
        ReplaceSetProperties( aKeyName, aValues );
    }
}

/*
 *  SettingsConfigItem::Notify
 */

void SettingsConfigItem::Notify( const Sequence< OUString >& )
{
    getValues();
}

/*
 *  SettingsConfigItem::getValues
 */
void SettingsConfigItem::getValues()
{
    if( ! IsValidConfigMgr() )
        return;

    m_aSettings.clear();

    Sequence< OUString > aNames( GetNodeNames( OUString() ) );
    m_aSettings.resize( aNames.getLength() );

    for( int j = 0; j < aNames.getLength(); j++ )
    {
#if OSL_DEBUG_LEVEL > 2
        fprintf( stderr, "found settings data for \"%s\"\n",
                 OUStringToOString( aNames.getConstArray()[j], RTL_TEXTENCODING_ASCII_US ).getStr()
                 );
#endif
        String aKeyName( aNames.getConstArray()[j] );
        Sequence< OUString > aKeys( GetNodeNames( aKeyName ) );
        Sequence< OUString > aSettingsKeys( aKeys.getLength() );
        const OUString* pFrom = aKeys.getConstArray();
        OUString* pTo = aSettingsKeys.getArray();
        for( int m = 0; m < aKeys.getLength(); m++ )
        {
            String aName( aKeyName );
            aName.Append( '/' );
            aName.Append( String( pFrom[m] ) );
            pTo[m] = aName;
        }
        Sequence< Any > aValues( GetProperties( aSettingsKeys ) );
        const Any* pValue = aValues.getConstArray();
        for( int i = 0; i < aValues.getLength(); i++, pValue++ )
        {
            if( pValue->getValueTypeClass() == TypeClass_STRING )
            {
                const OUString* pLine = (const OUString*)pValue->getValue();
                if( pLine->getLength() )
                    m_aSettings[ aKeyName ][ pFrom[i] ] = *pLine;
#if OSL_DEBUG_LEVEL > 2
                fprintf( stderr, "   \"%s\"=\"%.30s\"\n",
                         OUStringToOString( aKeys.getConstArray()[i], RTL_TEXTENCODING_ASCII_US ).getStr(),
                         OUStringToOString( *pLine, RTL_TEXTENCODING_ASCII_US ).getStr()
                         );
#endif
            }
        }
    }
}

/*
 *  SettingsConfigItem::getDefaultFont
 */

const OUString& SettingsConfigItem::getValue( const OUString& rGroup, const OUString& rKey ) const
{
    ::std::hash_map< OUString, SmallOUStrMap, rtl::OUStringHash >::const_iterator group = m_aSettings.find( rGroup );
    if( group == m_aSettings.end() || group->second.find( rKey ) == group->second.end() )
    {
        static OUString aEmpty;
        return aEmpty;
    }
    return group->second.find(rKey)->second;
}

/*
 *  SettingsConfigItem::setDefaultFont
 */

void SettingsConfigItem::setValue( const OUString& rGroup, const OUString& rKey, const OUString& rValue )
{
    bool bModified = m_aSettings[ rGroup ][ rKey ] != rValue;
    if( bModified )
    {
        m_aSettings[ rGroup ][ rKey ] = rValue;
        SetModified();
    }
}

