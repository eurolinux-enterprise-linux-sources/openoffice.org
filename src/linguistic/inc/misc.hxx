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

#ifndef _LINGUISTIC_MISC_HXX_
#define _LINGUISTIC_MISC_HXX_


#include <com/sun/star/uno/Sequence.h>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/beans/PropertyValues.hpp>
#include <com/sun/star/frame/XTerminateListener.hpp>
#include <com/sun/star/lang/Locale.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/linguistic2/XDictionaryEntry.hpp>
#include <com/sun/star/linguistic2/XSearchableDictionaryList.hpp>
#include <com/sun/star/linguistic2/XHyphenatedWord.hpp>

#include <uno/lbnames.h>			// CPPU_CURRENT_LANGUAGE_BINDING_NAME macro, which specify the environment type
#include <cppuhelper/implbase1.hxx>	// helper for implementations
#include <svtools/pathoptions.hxx>
#include <i18npool/lang.h>
#include <tools/string.hxx>
#include <unotools/charclass.hxx>
#include <osl/thread.h>
#include <osl/mutex.hxx>
#include <vcl/svapp.hxx>

namespace com { namespace sun { namespace star { namespace beans {
	class XPropertySet;
	class XFastPropertySet;
}}}}

namespace com { namespace sun { namespace star { namespace frame {
	class XDesktop;
}}}}

class LocaleDataWrapper;

///////////////////////////////////////////////////////////////////////////
#define SN_GRAMMARCHECKER           "com.sun.star.linguistic2.Proofreader"
#define SN_GRAMMARCHECKINGITERATOR  "com.sun.star.linguistic2.ProofreadingIterator"
#define SN_SPELLCHECKER				"com.sun.star.linguistic2.SpellChecker"
#define SN_HYPHENATOR				"com.sun.star.linguistic2.Hyphenator"
#define SN_THESAURUS				"com.sun.star.linguistic2.Thesaurus"
#define SN_LINGU_SERVCICE_MANAGER	"com.sun.star.linguistic2.LinguServiceManager"
#define SN_LINGU_PROPERTIES			"com.sun.star.linguistic2.LinguProperties"
#define SN_DICTIONARY_LIST			"com.sun.star.linguistic2.DictionaryList"
#define SN_OTHER_LINGU				"com.sun.star.linguistic2.OtherLingu"
#define SN_DESKTOP					"com.sun.star.frame.Desktop"


namespace linguistic
{

// ascii to OUString conversion
#define A2OU(x) ::rtl::OUString::createFromAscii( x )

/// Flags to be used with the multi-path related functions
/// @see GetDictionaryPaths, GetLinguisticPaths
#define PATH_FLAG_INTERNAL  0x01
#define PATH_FLAG_USER      0x02
#define PATH_FLAG_WRITABLE  0x04
#define PATH_FLAG_ALL       (PATH_FLAG_INTERNAL | PATH_FLAG_USER | PATH_FLAG_WRITABLE)

    
// AddEntryToDic return values
#define DIC_ERR_NONE        0
#define DIC_ERR_FULL        1
#define DIC_ERR_READONLY    2
#define DIC_ERR_UNKNOWN     3
#define DIC_ERR_NOT_EXISTS  4

///////////////////////////////////////////////////////////////////////////

::osl::Mutex &	GetLinguMutex();

LocaleDataWrapper & GetLocaleDataWrapper( INT16 nLang );

///////////////////////////////////////////////////////////////////////////

rtl_TextEncoding GetTextEncoding( INT16 nLanguage );

inline ::rtl::OUString BS2OU(const ByteString &rText, rtl_TextEncoding nEnc)
{
    return ::rtl::OUString( rText.GetBuffer(), rText.Len(), nEnc );
}

inline ByteString OU2BS(const ::rtl::OUString &rText, rtl_TextEncoding nEnc)
{
    return ByteString( rText.getStr(), nEnc );
}

rtl::OUString StripTrailingChars( rtl::OUString &rTxt, sal_Unicode cChar );

///////////////////////////////////////////////////////////////////////////

sal_Int32 LevDistance( const rtl::OUString &rTxt1, const rtl::OUString &rTxt2 );

///////////////////////////////////////////////////////////////////////////

::com::sun::star::lang::Locale
	CreateLocale( LanguageType eLang );

LanguageType
 	LocaleToLanguage( const ::com::sun::star::lang::Locale& rLocale );

::com::sun::star::lang::Locale&
	LanguageToLocale( ::com::sun::star::lang::Locale& rLocale, LanguageType eLang );

::com::sun::star::uno::Sequence< ::com::sun::star::lang::Locale >
    LangSeqToLocaleSeq( const ::com::sun::star::uno::Sequence< INT16 > &rLangSeq );

::com::sun::star::uno::Sequence< INT16 >
    LocaleSeqToLangSeq( ::com::sun::star::uno::Sequence< 
        ::com::sun::star::lang::Locale > &rLocaleSeq );

///////////////////////////////////////////////////////////////////////////

// checks if file pointed to by rURL is readonly
// and may also check return if such a file exists or not
BOOL    IsReadOnly( const String &rURL, BOOL *pbExist = 0 );

// checks if a file with the given URL exists
BOOL    FileExists( const String &rURL );

#ifdef TL_OUTDATED
// returns complete file URL for given filename that is to be searched in
// the specified path
String  GetFileURL( SvtPathOptions::Pathes ePath, const String &rFileName );

String  GetModulePath( SvtPathOptions::Pathes ePath, BOOL bAddAccessDelim = TRUE );
#endif

///////////////////////////////////////////////////////////////////////////

::rtl::OUString     GetDictionaryWriteablePath();
::com::sun::star::uno::Sequence< ::rtl::OUString > GetDictionaryPaths( sal_Int16 nPathFlags = PATH_FLAG_ALL );
::com::sun::star::uno::Sequence< ::rtl::OUString > GetLinguisticPaths( sal_Int16 nPathFlags = PATH_FLAG_ALL );

/// @returns an URL for a new and writable dictionary rDicName.
///     The URL will point to the path given by 'GetDictionaryWriteablePath'
String  GetWritableDictionaryURL( const String &rDicName );

// looks for the specified file in the list of paths.
// In case of multiple occurences only the first found is returned.
String     SearchFileInPaths( const String &rFile, const ::com::sun::star::uno::Sequence< ::rtl::OUString > &rPaths );


///////////////////////////////////////////////////////////////////////////

INT32		GetPosInWordToCheck( const rtl::OUString &rTxt, INT32 nPos );

::com::sun::star::uno::Reference<
	::com::sun::star::linguistic2::XHyphenatedWord >
			RebuildHyphensAndControlChars( const rtl::OUString &rOrigWord, 
				::com::sun::star::uno::Reference<
					::com::sun::star::linguistic2::XHyphenatedWord > &rxHyphWord );

///////////////////////////////////////////////////////////////////////////

BOOL        IsUpper( const String &rText, xub_StrLen nPos, xub_StrLen nLen, INT16 nLanguage );
BOOL        IsLower( const String &rText, xub_StrLen nPos, xub_StrLen nLen, INT16 nLanguage );

inline BOOL        IsUpper( const String &rText, INT16 nLanguage )     { return IsUpper( rText, 0, rText.Len(), nLanguage ); }
inline BOOL        IsLower( const String &rText, INT16 nLanguage )     { return IsLower( rText, 0, rText.Len(), nLanguage ); }

String      ToLower( const String &rText, INT16 nLanguage );
String      ToUpper( const String &rText, INT16 nLanguage );
String      ToTitle( const String &rText, INT16 nLanguage );
sal_Unicode	ToLower( const sal_Unicode cChar, INT16 nLanguage );
sal_Unicode	ToUpper( const sal_Unicode cChar, INT16 nLanguage );
BOOL		HasDigits( const ::rtl::OUString &rText );
BOOL		IsNumeric( const String &rText );

///////////////////////////////////////////////////////////////////////////

::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > GetOneInstanceService( const char *pServiceName );
::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet > GetLinguProperties();
::com::sun::star::uno::Reference< ::com::sun::star::linguistic2::XSearchableDictionaryList > GetSearchableDictionaryList();
::com::sun::star::uno::Reference< ::com::sun::star::linguistic2::XDictionaryList > GetDictionaryList();
::com::sun::star::uno::Reference< ::com::sun::star::linguistic2::XDictionary > GetIgnoreAllList();

///////////////////////////////////////////////////////////////////////////

BOOL IsUseDicList( const ::com::sun::star::beans::PropertyValues &rProperties,
		const ::com::sun::star::uno::Reference< 
			::com::sun::star::beans::XPropertySet > &rxPropSet );

BOOL IsIgnoreControlChars( const ::com::sun::star::beans::PropertyValues &rProperties,
		const ::com::sun::star::uno::Reference< 
			::com::sun::star::beans::XPropertySet > &rxPropSet );

::com::sun::star::uno::Reference< 
	::com::sun::star::linguistic2::XDictionaryEntry >
		SearchDicList( 
            const ::com::sun::star::uno::Reference< ::com::sun::star::linguistic2::XDictionaryList >& rDicList,
			const ::rtl::OUString& rWord, INT16 nLanguage, 
			BOOL bSearchPosDics, BOOL bSearchSpellEntry );

sal_uInt8 AddEntryToDic( 
    ::com::sun::star::uno::Reference< ::com::sun::star::linguistic2::XDictionary >  &rxDic,
    const ::rtl::OUString &rWord, sal_Bool bIsNeg,
    const ::rtl::OUString &rRplcTxt, sal_Int16 nRplcLang, 
    sal_Bool bStripDot = sal_True );

sal_Bool SaveDictionaries( const ::com::sun::star::uno::Reference< ::com::sun::star::linguistic2::XDictionaryList > &xDicList );

///////////////////////////////////////////////////////////////////////////
//
// AppExitLstnr:
// virtual base class that calls it AtExit function when the application
// (ie the Desktop) is about to terminate
//

class AppExitListener :
	public cppu::WeakImplHelper1
	< 
		::com::sun::star::frame::XTerminateListener
	>
{
	::com::sun::star::uno::Reference<
		::com::sun::star::frame::XDesktop >		xDesktop;

public:
	AppExitListener();
	virtual ~AppExitListener();

	virtual	void	AtExit() = 0;

	void			Activate();
	void			Deactivate();

	// XEventListener
    virtual void SAL_CALL disposing( const ::com::sun::star::lang::EventObject& Source ) throw(::com::sun::star::uno::RuntimeException);

    // XTerminateListener
    virtual void SAL_CALL queryTermination( const ::com::sun::star::lang::EventObject& aEvent ) throw(::com::sun::star::frame::TerminationVetoException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL notifyTermination( const ::com::sun::star::lang::EventObject& aEvent ) throw(::com::sun::star::uno::RuntimeException);
};

///////////////////////////////////////////////////////////////////////////

}	// namespace linguistic

#endif

