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


#include <com/sun/star/io/XStream.hpp>
#include <tools/urlobj.hxx>
#include <tools/table.hxx>
#include <i18npool/mslangid.hxx>
#ifndef _APP_HXX //autogen
#include <vcl/svapp.hxx>
#endif
#ifndef _STORINFO_HXX //autogen
#include <sot/storinfo.hxx>
#endif
#ifndef _SFX_DOCFILE_HXX
#include <sfx2/docfile.hxx>
#endif
#include <sfx2/sfxhelp.hxx>
#include <sfx2/viewfrm.hxx>
// fuer die Sort-String-Arrays aus dem SVMEM.HXX
#define _SVSTDARR_STRINGSISORTDTOR
#define _SVSTDARR_STRINGSDTOR
#include <svtools/svstdarr.hxx>

#ifndef SVTOOLS_FSTATHELPER_HXX
#include <svtools/fstathelper.hxx>
#endif
#include <svtools/helpopt.hxx>
#include <svtools/urihelper.hxx>
#include <unotools/charclass.hxx>
#ifndef _COM_SUN_STAR_I18N_UNICODETYPE_HDL_
#include <com/sun/star/i18n/UnicodeType.hdl>
#endif
#include <unotools/collatorwrapper.hxx>
#include <com/sun/star/i18n/CollatorOptions.hpp>
#include <unotools/localedatawrapper.hxx>
#include <unotools/transliterationwrapper.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/io/XActiveDataSource.hpp>

#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif

#include <sot/storage.hxx>
#include <comphelper/storagehelper.hxx>

#include <svx/udlnitem.hxx>
#include <svx/wghtitem.hxx>
#include <svx/escpitem.hxx>
#include <svx/svxacorr.hxx>
#include "unolingu.hxx"

#ifndef _SVX_HELPID_HRC
#include <helpid.hrc>
#endif
#include <comphelper/processfactory.hxx>
#include <com/sun/star/xml/sax/InputSource.hpp>
#include <com/sun/star/xml/sax/XParser.hpp>
#include <unotools/streamwrap.hxx>
#include <SvXMLAutoCorrectImport.hxx>
#include <SvXMLAutoCorrectExport.hxx>
#include <ucbhelper/content.hxx>
#include <com/sun/star/ucb/XCommandEnvironment.hpp>
#include <com/sun/star/ucb/TransferInfo.hpp>
#include <com/sun/star/ucb/NameClash.hpp>
#include <xmloff/xmltoken.hxx>

using namespace ::com::sun::star::ucb;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star;
using namespace ::xmloff::token;
using namespace ::rtl;
using namespace ::utl;

const int C_NONE 				= 0x00;
const int C_FULL_STOP 			= 0x01;
const int C_EXCLAMATION_MARK	= 0x02;
const int C_QUESTION_MARK		= 0x04;

static const sal_Char pImplWrdStt_ExcptLstStr[]    = "WordExceptList";
static const sal_Char pImplCplStt_ExcptLstStr[]    = "SentenceExceptList";
static const sal_Char pImplAutocorr_ListStr[]      = "DocumentList";
static const sal_Char pXMLImplWrdStt_ExcptLstStr[] = "WordExceptList.xml";
static const sal_Char pXMLImplCplStt_ExcptLstStr[] = "SentenceExceptList.xml";
static const sal_Char pXMLImplAutocorr_ListStr[]   = "DocumentList.xml";

static const sal_Char
	/* auch bei diesen Anfaengen - Klammern auf und alle Arten von Anf.Zei. */
	sImplSttSkipChars[]	= "\"\'([{\x83\x84\x89\x91\x92\x93\x94",
	/* auch bei diesen Ende - Klammern auf und alle Arten von Anf.Zei. */
	sImplEndSkipChars[]	= "\"\')]}\x83\x84\x89\x91\x92\x93\x94";

// diese Zeichen sind in Worten erlaubt: (fuer FnCptlSttSntnc)
static const sal_Char sImplWordChars[] = "-'";

void EncryptBlockName_Imp( String& rName );
void DecryptBlockName_Imp( String& rName );


// FileVersions Nummern fuer die Ersetzungs-/Ausnahmelisten getrennt
#define WORDLIST_VERSION_358	1
#define EXEPTLIST_VERSION_358	0


_SV_IMPL_SORTAR_ALG( SvxAutocorrWordList, SvxAutocorrWordPtr )
TYPEINIT0(SvxAutoCorrect)

typedef SvxAutoCorrectLanguageLists* SvxAutoCorrectLanguageListsPtr;
DECLARE_TABLE( SvxAutoCorrLanguageTable_Impl,  SvxAutoCorrectLanguageListsPtr)

DECLARE_TABLE( SvxAutoCorrLastFileAskTable_Impl, long )


inline int IsWordDelim( const sal_Unicode c )
{
	return ' ' == c || '\t' == c || 0x0a == c ||
            0xA0 == c || 0x2011 == c || 0x1 == c;
}

inline int IsLowerLetter( sal_Int32 nCharType )
{
	return CharClass::isLetterType( nCharType ) &&
			0 == ( ::com::sun::star::i18n::KCharacterType::UPPER & nCharType);
}
inline int IsUpperLetter( sal_Int32 nCharType )
{
	return CharClass::isLetterType( nCharType ) &&
			0 == ( ::com::sun::star::i18n::KCharacterType::LOWER & nCharType);
}

BOOL lcl_IsSymbolChar( CharClass& rCC, const String& rTxt,
				   		xub_StrLen nStt, xub_StrLen nEnd )
{
	for( ; nStt < nEnd; ++nStt )
	{
#if OSL_DEBUG_LEVEL > 1
		sal_Int32 nCharType;
		sal_Int32 nChType;
		nCharType = rCC.getCharacterType( rTxt, nStt );
		nChType = rCC.getType( rTxt, nStt );
#endif
		if( ::com::sun::star::i18n::UnicodeType::PRIVATE_USE ==
				rCC.getType( rTxt, nStt ))
			return TRUE;
	}
	return FALSE;
}


static BOOL lcl_IsInAsciiArr( const sal_Char* pArr, const sal_Unicode c )
{
	BOOL bRet = FALSE;
	for( ; *pArr; ++pArr )
		if( *pArr == c )
		{
			bRet = TRUE;
			break;
		}
	return bRet;
}

SvxAutoCorrDoc::~SvxAutoCorrDoc()
{
}


	// wird nach dem austauschen der Zeichen von den Funktionen
	//	- FnCptlSttWrd
	// 	- FnCptlSttSntnc
	// gerufen. Dann koennen die Worte ggfs. in die Ausnahmelisten
	// aufgenommen werden.
void SvxAutoCorrDoc::SaveCpltSttWord( ULONG, xub_StrLen, const String&,
										sal_Unicode )
{
}

LanguageType SvxAutoCorrDoc::GetLanguage( xub_StrLen , BOOL ) const
{
	return LANGUAGE_SYSTEM;
}

static ::com::sun::star::uno::Reference<
			::com::sun::star::lang::XMultiServiceFactory >& GetProcessFact()
{
	static ::com::sun::star::uno::Reference<
			::com::sun::star::lang::XMultiServiceFactory > xMSF =
									::comphelper::getProcessServiceFactory();
	return xMSF;
}

static USHORT GetAppLang()
{
	return Application::GetSettings().GetLanguage();
}
static LocaleDataWrapper& GetLocaleDataWrapper( USHORT nLang )
{
	static LocaleDataWrapper aLclDtWrp( GetProcessFact(),
										SvxCreateLocale( GetAppLang() ) );
	::com::sun::star::lang::Locale aLcl( SvxCreateLocale( nLang ));
	const ::com::sun::star::lang::Locale& rLcl = aLclDtWrp.getLoadedLocale();
	if( aLcl.Language != rLcl.Language ||
		aLcl.Country != rLcl.Country ||
		aLcl.Variant != rLcl.Variant )
		aLclDtWrp.setLocale( aLcl );
	return aLclDtWrp;
}
static TransliterationWrapper& GetIgnoreTranslWrapper()
{
	static int bIsInit = 0;
	static TransliterationWrapper aWrp( GetProcessFact(),
				::com::sun::star::i18n::TransliterationModules_IGNORE_CASE |
				::com::sun::star::i18n::TransliterationModules_IGNORE_KANA |
				::com::sun::star::i18n::TransliterationModules_IGNORE_WIDTH );
	if( !bIsInit )
	{
		aWrp.loadModuleIfNeeded( GetAppLang() );
		bIsInit = 1;
	}
	return aWrp;
}
static CollatorWrapper& GetCollatorWrapper()
{
	static int bIsInit = 0;
	static CollatorWrapper aCollWrp( GetProcessFact() );
	if( !bIsInit )
	{
		aCollWrp.loadDefaultCollator( SvxCreateLocale( GetAppLang() ), 0 );
		bIsInit = 1;
	}
	return aCollWrp;
}


void SvxAutocorrWordList::DeleteAndDestroy( USHORT nP, USHORT nL )
{
	if( nL )
	{
		DBG_ASSERT( nP < nA && nP + nL <= nA, "ERR_VAR_DEL" );
		for( USHORT n=nP; n < nP + nL; n++ )
			delete *((SvxAutocorrWordPtr*)pData+n);
		SvPtrarr::Remove( nP, nL );
	}
}


BOOL SvxAutocorrWordList::Seek_Entry( const SvxAutocorrWordPtr aE, USHORT* pP ) const
{
	register USHORT nO  = SvxAutocorrWordList_SAR::Count(),
			nM,
			nU = 0;
	if( nO > 0 )
	{
		CollatorWrapper& rCmp = ::GetCollatorWrapper();
		nO--;
		while( nU <= nO )
		{
			nM = nU + ( nO - nU ) / 2;
			long nCmp = rCmp.compareString( aE->GetShort(),
						(*((SvxAutocorrWordPtr*)pData + nM))->GetShort() );
			if( 0 == nCmp )
			{
				if( pP ) *pP = nM;
				return TRUE;
			}
			else if( 0 < nCmp )
				nU = nM + 1;
			else if( nM == 0 )
			{
				if( pP ) *pP = nU;
				return FALSE;
			}
			else
				nO = nM - 1;
		}
	}
	if( pP ) *pP = nU;
	return FALSE;
}

/* -----------------18.11.98 15:28-------------------
 *
 * --------------------------------------------------*/
void lcl_ClearTable(SvxAutoCorrLanguageTable_Impl& rLangTable)
{
	SvxAutoCorrectLanguageListsPtr pLists = rLangTable.Last();
	while(pLists)
	{
		delete pLists;
		pLists = rLangTable.Prev();
	}
	rLangTable.Clear();
}

/* -----------------03.11.06 10:15-------------------
 *
 * --------------------------------------------------*/

sal_Bool SvxAutoCorrect::IsAutoCorrectChar( sal_Unicode cChar )
{
    return  cChar == '\0' || cChar == '\t' || cChar == 0x0a ||
            cChar == ' '  || cChar == '\'' || cChar == '\"' ||
            cChar == '*'  || cChar == '_'  ||
            cChar == '.'  || cChar == ','  || cChar == ';' ||
            cChar == ':'  || cChar == '?' || cChar == '!';
}

/* -----------------19.11.98 10:15-------------------
 *
 * --------------------------------------------------*/
long SvxAutoCorrect::GetDefaultFlags()
{
	long nRet = Autocorrect
					| CptlSttSntnc
					| CptlSttWrd
					| ChgFractionSymbol
					| ChgOrdinalNumber
					| ChgToEnEmDash
					| ChgWeightUnderl
					| SetINetAttr
					| ChgQuotes
					| SaveWordCplSttLst
					| SaveWordWrdSttLst;
	LanguageType eLang = GetAppLang();
	switch( eLang )
	{
	case LANGUAGE_ENGLISH:
	case LANGUAGE_ENGLISH_US:
	case LANGUAGE_ENGLISH_UK:
	case LANGUAGE_ENGLISH_AUS:
	case LANGUAGE_ENGLISH_CAN:
	case LANGUAGE_ENGLISH_NZ:
	case LANGUAGE_ENGLISH_EIRE:
	case LANGUAGE_ENGLISH_SAFRICA:
	case LANGUAGE_ENGLISH_JAMAICA:
	case LANGUAGE_ENGLISH_CARRIBEAN:
		nRet &= ~(ChgQuotes|ChgSglQuotes);
		break;
	}
	return nRet;
}


SvxAutoCorrect::SvxAutoCorrect( const String& rShareAutocorrFile,
								const String& rUserAutocorrFile )
	: sShareAutoCorrFile( rShareAutocorrFile ),
	sUserAutoCorrFile( rUserAutocorrFile ),
	pLangTable( new SvxAutoCorrLanguageTable_Impl ),
	pLastFileTable( new SvxAutoCorrLastFileAskTable_Impl ),
	pCharClass( 0 ),
	cStartDQuote( 0 ), cEndDQuote( 0 ), cStartSQuote( 0 ), cEndSQuote( 0 )
{
	nFlags = SvxAutoCorrect::GetDefaultFlags();

	c1Div2 = ByteString::ConvertToUnicode( '\xBD', RTL_TEXTENCODING_MS_1252 );
	c1Div4 = ByteString::ConvertToUnicode( '\xBC', RTL_TEXTENCODING_MS_1252 );
	c3Div4 = ByteString::ConvertToUnicode( '\xBE', RTL_TEXTENCODING_MS_1252 );
	cEmDash = ByteString::ConvertToUnicode( '\x97', RTL_TEXTENCODING_MS_1252 );
	cEnDash = ByteString::ConvertToUnicode( '\x96', RTL_TEXTENCODING_MS_1252 );
}

SvxAutoCorrect::SvxAutoCorrect( const SvxAutoCorrect& rCpy )
:	sShareAutoCorrFile( rCpy.sShareAutoCorrFile ),
	sUserAutoCorrFile( rCpy.sUserAutoCorrFile ),

	aSwFlags( rCpy.aSwFlags ),

	pLangTable( new SvxAutoCorrLanguageTable_Impl ),
	pLastFileTable( new SvxAutoCorrLastFileAskTable_Impl ),
	pCharClass( 0 ),

	nFlags( rCpy.nFlags & ~(ChgWordLstLoad|CplSttLstLoad|WrdSttLstLoad)),
	cStartDQuote( rCpy.cStartDQuote ), cEndDQuote( rCpy.cEndDQuote ),
	cStartSQuote( rCpy.cStartSQuote ), cEndSQuote( rCpy.cEndSQuote ),
	c1Div2( rCpy.c1Div2 ), c1Div4( rCpy.c1Div4 ), c3Div4( rCpy.c3Div4 ),
	cEmDash( rCpy.cEmDash ), cEnDash( rCpy.cEnDash )
{
}


SvxAutoCorrect::~SvxAutoCorrect()
{
	lcl_ClearTable(*pLangTable);
	delete pLangTable;
	delete pLastFileTable;
	delete pCharClass;
}

void SvxAutoCorrect::_GetCharClass( LanguageType eLang )
{
	delete pCharClass;
	pCharClass = new CharClass( SvxCreateLocale( eLang ));
	eCharClassLang = eLang;
}

void SvxAutoCorrect::SetAutoCorrFlag( long nFlag, BOOL bOn )
{
	long nOld = nFlags;
	nFlags = bOn ? nFlags | nFlag
				 : nFlags & ~nFlag;

	if( !bOn )
	{
		if( (nOld & CptlSttSntnc) != (nFlags & CptlSttSntnc) )
			nFlags &= ~CplSttLstLoad;
		if( (nOld & CptlSttWrd) != (nFlags & CptlSttWrd) )
			nFlags &= ~WrdSttLstLoad;
		if( (nOld & Autocorrect) != (nFlags & Autocorrect) )
			nFlags &= ~ChgWordLstLoad;
	}
}


	// Zwei Grossbuchstaben am Wort-Anfang ??
BOOL SvxAutoCorrect::FnCptlSttWrd( SvxAutoCorrDoc& rDoc, const String& rTxt,
									xub_StrLen nSttPos, xub_StrLen nEndPos,
									LanguageType eLang )
{
	BOOL bRet = FALSE;
	CharClass& rCC = GetCharClass( eLang );

	// loesche alle nicht alpanum. Zeichen am Wortanfang/-ende und
	// teste dann ( erkennt: "(min.", "/min.", usw.)
	for( ; nSttPos < nEndPos; ++nSttPos )
		if( rCC.isLetterNumeric( rTxt, nSttPos ))
			break;
	for( ; nSttPos < nEndPos; --nEndPos )
		if( rCC.isLetterNumeric( rTxt, nEndPos - 1 ))
			break;

	// Zwei Grossbuchstaben am Wort-Anfang ??
	if( nSttPos+2 < nEndPos &&
		IsUpperLetter( rCC.getCharacterType( rTxt, nSttPos )) &&
		IsUpperLetter( rCC.getCharacterType( rTxt, ++nSttPos )) &&
		// ist das 3. Zeichen ein klein geschiebenes Alpha-Zeichen
		IsLowerLetter( rCC.getCharacterType( rTxt, nSttPos +1 )) &&
		// keine Sonder-Attribute ersetzen
		0x1 != rTxt.GetChar( nSttPos ) && 0x2 != rTxt.GetChar( nSttPos ))
	{
		// teste ob das Wort in einer Ausnahmeliste steht
		String sWord( rTxt.Copy( nSttPos - 1, nEndPos - nSttPos + 1 ));
		if( !FindInWrdSttExceptList(eLang, sWord) )
		{
			sal_Unicode cSave = rTxt.GetChar( nSttPos );
			String sChar( cSave );
			rCC.toLower( sChar );
			if( sChar.GetChar(0) != cSave && rDoc.Replace( nSttPos, sChar ))
			{
				if( SaveWordWrdSttLst & nFlags )
					rDoc.SaveCpltSttWord( CptlSttWrd, nSttPos, sWord, cSave );
				bRet = TRUE;
			}
		}
	}
	return bRet;
}


BOOL SvxAutoCorrect::FnChgFractionSymbol(
								SvxAutoCorrDoc& rDoc, const String& rTxt,
								xub_StrLen nSttPos, xub_StrLen nEndPos )
{
	sal_Unicode cChar = 0;

	for( ; nSttPos < nEndPos; ++nSttPos )
		if( !lcl_IsInAsciiArr( sImplSttSkipChars, rTxt.GetChar( nSttPos ) ))
			break;
	for( ; nSttPos < nEndPos; --nEndPos )
		if( !lcl_IsInAsciiArr( sImplEndSkipChars, rTxt.GetChar( nEndPos - 1 ) ))
			break;

	// 1/2, 1/4, ... ersetzen durch das entsprechende Zeichen vom Font
	if( 3 == nEndPos - nSttPos && '/' == rTxt.GetChar( nSttPos+1 ))
	{
		switch( ( rTxt.GetChar( nSttPos )) * 256 + rTxt.GetChar( nEndPos-1 ))
		{
		case '1' * 256 + '2':		cChar = c1Div2;		break;
		case '1' * 256 + '4':		cChar = c1Div4;		break;
		case '3' * 256 + '4':		cChar = c3Div4;		break;
		}

		if( cChar )
		{
			// also austauschen:
			rDoc.Delete( nSttPos+1, nEndPos );
			rDoc.Replace( nSttPos, cChar );
		}
	}
	return 0 != cChar;
}


BOOL SvxAutoCorrect::FnChgOrdinalNumber(
								SvxAutoCorrDoc& rDoc, const String& rTxt,
								xub_StrLen nSttPos, xub_StrLen nEndPos,
								LanguageType eLang )
{
// 1st, 2nd, 3rd, 4 - 0th
// 201th oder 201st
// 12th oder 12nd
	CharClass& rCC = GetCharClass( eLang );
	BOOL bChg = FALSE;

	for( ; nSttPos < nEndPos; ++nSttPos )
		if( !lcl_IsInAsciiArr( sImplSttSkipChars, rTxt.GetChar( nSttPos ) ))
			break;
	for( ; nSttPos < nEndPos; --nEndPos )
		if( !lcl_IsInAsciiArr( sImplEndSkipChars, rTxt.GetChar( nEndPos - 1 ) ))
			break;

	if( 2 < nEndPos - nSttPos &&
		rCC.isDigit( rTxt, nEndPos - 3 ) )
	{
		static sal_Char __READONLY_DATA
			sAll[]		= "th",			/* rest */
			sFirst[]	= "st",      	/* 1 */
			sSecond[]	= "nd",       	/* 2 */
			sThird[]	= "rd";       	/* 3 */
		static const sal_Char* __READONLY_DATA aNumberTab[ 4 ] =
		{
			sAll, sFirst, sSecond, sThird
		};

		sal_Unicode c = rTxt.GetChar( nEndPos - 3 );
		if( ( c -= '0' ) > 3 )
			c = 0;

		bChg = ( ((sal_Unicode)*((aNumberTab[ c ])+0)) ==
										rTxt.GetChar( nEndPos - 2 ) &&
				 ((sal_Unicode)*((aNumberTab[ c ])+1)) ==
				 						rTxt.GetChar( nEndPos - 1 )) ||
			   ( 3 < nEndPos - nSttPos &&
				( ((sal_Unicode)*(sAll+0)) == rTxt.GetChar( nEndPos - 2 ) &&
				  ((sal_Unicode)*(sAll+1)) == rTxt.GetChar( nEndPos - 1 )));

		if( bChg )
		{
			// dann pruefe mal, ob alle bis zum Start alle Zahlen sind
			for( xub_StrLen n = nEndPos - 3; nSttPos < n; )
				if( !rCC.isDigit( rTxt, --n ) )
				{
					bChg = !rCC.isLetter( rTxt, n );
					break;
				}

			if( bChg )		// dann setze mal das Escapement Attribut
			{
				SvxEscapementItem aSvxEscapementItem( DFLT_ESC_AUTO_SUPER,
                                                    DFLT_ESC_PROP, SID_ATTR_CHAR_ESCAPEMENT );
				rDoc.SetAttr( nEndPos - 2, nEndPos,
								SID_ATTR_CHAR_ESCAPEMENT,
								aSvxEscapementItem);
			}
		}

	}
	return bChg;
}


BOOL SvxAutoCorrect::FnChgToEnEmDash(
								SvxAutoCorrDoc& rDoc, const String& rTxt,
								xub_StrLen nSttPos, xub_StrLen nEndPos,
								LanguageType eLang )
{
	BOOL bRet = FALSE;
	CharClass& rCC = GetCharClass( eLang );
    if (eLang == LANGUAGE_SYSTEM)
        eLang = GetAppLang();
    bool bAlwaysUseEmDash = (cEmDash && (eLang == LANGUAGE_RUSSIAN || eLang == LANGUAGE_UKRAINIAN));

	// ersetze " - " oder " --" durch "enDash"
	if( cEnDash && 1 < nSttPos && 1 <= nEndPos - nSttPos )
	{
		sal_Unicode cCh = rTxt.GetChar( nSttPos );
		if( '-' == cCh )
		{
			if( ' ' == rTxt.GetChar( nSttPos-1 ) &&
				'-' == rTxt.GetChar( nSttPos+1 ))
			{
				xub_StrLen n;
				for( n = nSttPos+2; n < nEndPos && lcl_IsInAsciiArr(
							sImplSttSkipChars,(cCh = rTxt.GetChar( n )));
						++n )
					;

				// found: " --[<AnySttChars>][A-z0-9]
				if( rCC.isLetterNumeric( cCh ) )
				{
					for( n = nSttPos-1; n && lcl_IsInAsciiArr(
							sImplEndSkipChars,(cCh = rTxt.GetChar( --n ))); )
						;

					// found: "[A-z0-9][<AnyEndChars>] --[<AnySttChars>][A-z0-9]
					if( rCC.isLetterNumeric( cCh ))
					{
						rDoc.Delete( nSttPos, nSttPos + 2 );
                        rDoc.Insert( nSttPos, bAlwaysUseEmDash ? cEmDash : cEnDash );
						bRet = TRUE;
					}
				}
			}
		}
		else if( 3 < nSttPos &&
				 ' ' == rTxt.GetChar( nSttPos-1 ) &&
				 '-' == rTxt.GetChar( nSttPos-2 ))
		{
			xub_StrLen n, nLen = 1, nTmpPos = nSttPos - 2;
			if( '-' == ( cCh = rTxt.GetChar( nTmpPos-1 )) )
			{
				--nTmpPos;
				++nLen;
				cCh = rTxt.GetChar( nTmpPos-1 );
			}
			if( ' ' == cCh )
			{
				for( n = nSttPos; n < nEndPos && lcl_IsInAsciiArr(
							sImplSttSkipChars,(cCh = rTxt.GetChar( n )));
						++n )
					;

				// found: " - [<AnySttChars>][A-z0-9]
				if( rCC.isLetterNumeric( cCh ) )
				{
					cCh = ' ';
					for( n = nTmpPos-1; n && lcl_IsInAsciiArr(
							sImplEndSkipChars,(cCh = rTxt.GetChar( --n ))); )
							;
					// found: "[A-z0-9][<AnyEndChars>] - [<AnySttChars>][A-z0-9]
					if( rCC.isLetterNumeric( cCh ))
					{
						rDoc.Delete( nTmpPos, nTmpPos + nLen );
                        rDoc.Insert( nTmpPos, bAlwaysUseEmDash ? cEmDash : cEnDash );
						bRet = TRUE;
					}
				}
			}
		}
	}

    // Replace [A-z0-9]--[A-z0-9] double dash with "emDash" or "enDash".
    // Finnish and Hungarian use enDash instead of emDash.
    bool bEnDash = (eLang == LANGUAGE_HUNGARIAN || eLang == LANGUAGE_FINNISH);    
    if( ((cEmDash && !bEnDash) || (cEnDash && bEnDash)) && 4 <= nEndPos - nSttPos )
	{
		String sTmp( rTxt.Copy( nSttPos, nEndPos - nSttPos ) );
		xub_StrLen nFndPos = sTmp.SearchAscii( "--" );
		if( STRING_NOTFOUND != nFndPos && nFndPos &&
			nFndPos + 2 < sTmp.Len() &&
			( rCC.isLetterNumeric( sTmp, nFndPos - 1 ) ||
			  lcl_IsInAsciiArr( sImplEndSkipChars, rTxt.GetChar( nFndPos - 1 ) )) &&
			( rCC.isLetterNumeric( sTmp, nFndPos + 2 ) ||
			lcl_IsInAsciiArr( sImplSttSkipChars, rTxt.GetChar( nFndPos + 2 ) )))
		{
			nSttPos = nSttPos + nFndPos;
			rDoc.Delete( nSttPos, nSttPos + 2 );
            rDoc.Insert( nSttPos, (bEnDash ? cEnDash : cEmDash) );
			bRet = TRUE;
		}
	}
	return bRet;
}


BOOL SvxAutoCorrect::FnSetINetAttr( SvxAutoCorrDoc& rDoc, const String& rTxt,
									xub_StrLen nSttPos, xub_StrLen nEndPos,
									LanguageType eLang )
{
	String sURL( URIHelper::FindFirstURLInText( rTxt, nSttPos, nEndPos,
												GetCharClass( eLang ) ));
	BOOL bRet = 0 != sURL.Len();
	if( bRet )			// also Attribut setzen:
		rDoc.SetINetAttr( nSttPos, nEndPos, sURL );
	return bRet;
}


BOOL SvxAutoCorrect::FnChgWeightUnderl( SvxAutoCorrDoc& rDoc, const String& rTxt,
										xub_StrLen, xub_StrLen nEndPos,
										LanguageType eLang )
{
	// Bedingung:
	//	Am Anfang:	_ oder * hinter Space mit nachfolgenden !Space
	//	Am Ende:	_ oder * vor Space (Worttrenner?)

	sal_Unicode c, cInsChar = rTxt.GetChar( nEndPos );	// unterstreichen oder fett
	if( ++nEndPos != rTxt.Len() &&
		!IsWordDelim( rTxt.GetChar( nEndPos ) ) )
		return FALSE;

	--nEndPos;

	BOOL bAlphaNum = FALSE;
	xub_StrLen nPos = nEndPos, nFndPos = STRING_NOTFOUND;
	CharClass& rCC = GetCharClass( eLang );

	while( nPos )
	{
		switch( c = rTxt.GetChar( --nPos ) )
		{
		case '_':
		case '*':
			if( c == cInsChar )
			{
				if( bAlphaNum && nPos+1 < nEndPos && ( !nPos ||
					IsWordDelim( rTxt.GetChar( nPos-1 ))) &&
					!IsWordDelim( rTxt.GetChar( nPos+1 )))
						nFndPos = nPos;
				else
					// Bedingung ist nicht erfuellt, also abbrechen
					nFndPos = STRING_NOTFOUND;
				nPos = 0;
			}
			break;
		default:
			if( !bAlphaNum )
				bAlphaNum = rCC.isLetterNumeric( rTxt, nPos );
		}
	}

	if( STRING_NOTFOUND != nFndPos )
	{
		// ueber den gefundenen Bereich das Attribut aufspannen und
		// das gefunde und am Ende stehende Zeichen loeschen
		if( '*' == cInsChar )			// Fett
		{
            SvxWeightItem aSvxWeightItem( WEIGHT_BOLD, SID_ATTR_CHAR_WEIGHT );
			rDoc.SetAttr( nFndPos + 1, nEndPos,
							SID_ATTR_CHAR_WEIGHT,
							aSvxWeightItem);
		}
		else							// unterstrichen
		{
            SvxUnderlineItem aSvxUnderlineItem( UNDERLINE_SINGLE, SID_ATTR_CHAR_UNDERLINE );
			rDoc.SetAttr( nFndPos + 1, nEndPos,
							SID_ATTR_CHAR_UNDERLINE,
							aSvxUnderlineItem);
		}
		rDoc.Delete( nEndPos, nEndPos + 1 );
		rDoc.Delete( nFndPos, nFndPos + 1 );
	}

	return STRING_NOTFOUND != nFndPos;
}


BOOL SvxAutoCorrect::FnCptlSttSntnc( SvxAutoCorrDoc& rDoc,
									const String& rTxt, BOOL bNormalPos,
									xub_StrLen nSttPos, xub_StrLen nEndPos,
									LanguageType eLang )
{
	// Grossbuchstabe am Satz-Anfang ??
	if( !rTxt.Len() || nEndPos <= nSttPos )
		return FALSE;

 	CharClass& rCC = GetCharClass( eLang );
	String aText( rTxt );
	const sal_Unicode *pStart = aText.GetBuffer(),
					  *pStr = pStart + nEndPos,
					  *pWordStt = 0,
					  *pDelim = 0;

	BOOL bAtStart = FALSE, bPrevPara = FALSE;
	do {
		--pStr;
		if( rCC.isLetter(
                aText, sal::static_int_cast< xub_StrLen >( pStr - pStart ) ) )
		{
			if( !pWordStt )
				pDelim = pStr+1;
			pWordStt = pStr;
		}
		else if( pWordStt &&
                 !rCC.isDigit(
                     aText,
                     sal::static_int_cast< xub_StrLen >( pStr - pStart ) ) )
		{
			if( lcl_IsInAsciiArr( sImplWordChars, *pStr ) &&
				pWordStt - 1 == pStr &&
                // --> FME 2005-02-14 #i38971#
                // l'intallazione at beginning of paragraph. Replaced < by <=
				(long)(pStart + 1) <= (long)pStr &&
                // <--
				rCC.isLetter(
                    aText,
                    sal::static_int_cast< xub_StrLen >( pStr-1 - pStart ) ) )
				pWordStt = --pStr;
			else
				break;
		}
	} while( 0 == ( bAtStart = (pStart == pStr)) );


	if(	!pWordStt ||
		rCC.isDigit(
            aText, sal::static_int_cast< xub_StrLen >( pStr - pStart ) ) ||
		IsUpperLetter(
            rCC.getCharacterType(
                aText,
                sal::static_int_cast< xub_StrLen >( pWordStt - pStart ) ) ) ||
		0x1 == *pWordStt || 0x2 == *pWordStt )
		return FALSE;		// kein zu ersetzendes Zeichen, oder schon ok

	// JP 27.10.97: wenn das Wort weniger als 3 Zeichen hat und der Trenner
	//				ein "Num"-Trenner ist, dann nicht ersetzen!
	//				Damit wird ein "a.",  "a)", "a-a" nicht ersetzt!
	if( *pDelim && 2 >= pDelim - pWordStt &&
		lcl_IsInAsciiArr( ".-)>", *pDelim ) )
		return FALSE;

	if( !bAtStart )	// noch kein Absatz Anfang ?
	{
        if ( IsWordDelim( *pStr ) )
        {
            while( 0 == ( bAtStart = (pStart == pStr--) ) && IsWordDelim( *pStr ))
                ;
        }
        // Asian full stop, full width full stop, full width exclamation mark
        // and full width question marks are treated as word delimiters
        else if ( 0x3002 != *pStr && 0xFF0E != *pStr && 0xFF01 != *pStr &&
                  0xFF1F != *pStr )
            return FALSE; // kein gueltiger Trenner -> keine Ersetzung
    }

	if( bAtStart )	// am Absatz Anfang ?
	{
		// Ueberpruefe den vorherigen Absatz, wenn es diesen gibt.
		// Wenn ja, dann pruefe auf SatzTrenner am Ende.
		const String* pPrevPara = rDoc.GetPrevPara( bNormalPos );
		if( !pPrevPara )
		{
			// gueltiger Trenner -> Ersetze
			String sChar( *pWordStt );
			rCC.toUpper( sChar );
			return  sChar != *pWordStt &&
					rDoc.Replace( xub_StrLen( pWordStt - pStart ), sChar );
		}

		aText = *pPrevPara;
		bPrevPara = TRUE;
		bAtStart = FALSE;
		pStart = aText.GetBuffer();
		pStr = pStart + aText.Len();

		do {			// alle Blanks ueberlesen
			--pStr;
			if( !IsWordDelim( *pStr ))
				break;
		} while( 0 == ( bAtStart = (pStart == pStr)) );

		if( bAtStart )
			return FALSE;		// kein gueltiger Trenner -> keine Ersetzung
	}

	// bis hierhier wurde [ \t]+[A-Z0-9]+ gefunden. Test jetzt auf den
	// Satztrenner. Es koennen alle 3 vorkommen, aber nicht mehrfach !!
	const sal_Unicode* pExceptStt = 0;
	if( !bAtStart )
	{
		BOOL bWeiter = TRUE;
		int nFlag = C_NONE;
		do {
			switch( *pStr )
			{
            // Western and Asian full stop
			case '.':
            case 0x3002 :
            case 0xFF0E :
				{
					if( nFlag & C_FULL_STOP )
						return FALSE;		// kein gueltiger Trenner -> keine Ersetzung
					nFlag |= C_FULL_STOP;
					pExceptStt = pStr;
				}
				break;
			case '!':
            case 0xFF01 :
				{
					if( nFlag & C_EXCLAMATION_MARK )
						return FALSE; 	// kein gueltiger Trenner -> keine Ersetzung
					nFlag |= C_EXCLAMATION_MARK;
				}
				break;
			case '?':
            case 0xFF1F :
				{
					if( nFlag & C_QUESTION_MARK)
						return FALSE;		// kein gueltiger Trenner -> keine Ersetzung
					nFlag |= C_QUESTION_MARK;
				}
				break;
			default:
				if( !nFlag )
					return FALSE;		// kein gueltiger Trenner -> keine Ersetzung
				else
					bWeiter = FALSE;
				break;
			}

			if( bWeiter && pStr-- == pStart )
			{
// !!! wenn am Anfang, dann nie ersetzen.
//				if( !nFlag )
					return FALSE;		// kein gueltiger Trenner -> keine Ersetzung
//				++pStr;
//				break;		// Schleife beenden
			}
		} while( bWeiter );
		if( C_FULL_STOP != nFlag )
			pExceptStt = 0;
	}

	if( 2 > ( pStr - pStart ) )
		return FALSE;

	if( !rCC.isLetterNumeric(
            aText, sal::static_int_cast< xub_StrLen >( pStr-- - pStart ) ) )
	{
		BOOL bValid = FALSE, bAlphaFnd = FALSE;
		const sal_Unicode* pTmpStr = pStr;
		while( !bValid )
		{
			if( rCC.isDigit(
                    aText,
                    sal::static_int_cast< xub_StrLen >( pTmpStr - pStart ) ) )
			{
				bValid = TRUE;
				pStr = pTmpStr - 1;
			}
			else if( rCC.isLetter(
                         aText,
                         sal::static_int_cast< xub_StrLen >(
                             pTmpStr - pStart ) ) )
			{
				if( bAlphaFnd )
				{
					bValid = TRUE;
					pStr = pTmpStr;
				}
				else
					bAlphaFnd = TRUE;
			}
			else if( bAlphaFnd || IsWordDelim( *pTmpStr ) )
				break;

			if( pTmpStr == pStart )
				break;

			--pTmpStr;
		}

		if( !bValid )
			return FALSE;		// kein gueltiger Trenner -> keine Ersetzung
	}

	BOOL bNumericOnly = '0' <= *(pStr+1) && *(pStr+1) <= '9';

	// suche den Anfang vom Wort
	while( !IsWordDelim( *pStr ))
	{
		if( bNumericOnly &&
            rCC.isLetter(
                aText, sal::static_int_cast< xub_StrLen >( pStr - pStart ) ) )
			bNumericOnly = FALSE;

		if( pStart == pStr )
			break;

		--pStr;
	}

	if( bNumericOnly )		// besteht nur aus Zahlen, dann nicht
		return FALSE;

	if( IsWordDelim( *pStr ))
		++pStr;

	String sWord;

	// ueberpruefe anhand der Exceptionliste
	if( pExceptStt )
	{
		sWord = String(
            pStr, sal::static_int_cast< xub_StrLen >( pExceptStt - pStr + 1 ) );
		if( FindInCplSttExceptList(eLang, sWord) )
			return FALSE;

		// loesche alle nicht alpanum. Zeichen am Wortanfang/-ende und
		// teste dann noch mal ( erkennt: "(min.", "/min.", usw.)
		String sTmp( sWord );
		while( sTmp.Len() &&
				!rCC.isLetterNumeric( sTmp, 0 ) )
			sTmp.Erase( 0, 1 );

		// alle hinteren nicht alphanumerische Zeichen bis auf das
		// Letzte entfernen
		xub_StrLen nLen = sTmp.Len();
		while( nLen && !rCC.isLetterNumeric( sTmp, nLen-1 ) )
			--nLen;
		if( nLen + 1 < sTmp.Len() )
			sTmp.Erase( nLen + 1 );

		if( sTmp.Len() && sTmp.Len() != sWord.Len() &&
			FindInCplSttExceptList(eLang, sTmp))
			return FALSE;

		if(FindInCplSttExceptList(eLang, sWord, TRUE))
			return FALSE;
	}

	// Ok, dann ersetze mal
	sal_Unicode cSave = *pWordStt;
	nSttPos = sal::static_int_cast< xub_StrLen >( pWordStt - rTxt.GetBuffer() );
	String sChar( cSave );
	rCC.toUpper( sChar );
	BOOL bRet = sChar.GetChar(0) != cSave && rDoc.Replace( nSttPos, sChar );

	// das Wort will vielleicht jemand haben
	if( bRet && SaveWordCplSttLst & nFlags )
		rDoc.SaveCpltSttWord( CptlSttSntnc, nSttPos, sWord, cSave );

	return bRet;
}
//The method below is renamed from _GetQuote to GetQuote by BerryJia for Bug95846 Time:2002-8-13 15:50
sal_Unicode SvxAutoCorrect::GetQuote( sal_Unicode cInsChar, BOOL bSttQuote,
										LanguageType eLang ) const
{
	sal_Unicode cRet = bSttQuote ? ( '\"' == cInsChar
									? GetStartDoubleQuote()
									: GetStartSingleQuote() )
						  		 : ( '\"' == cInsChar
									? GetEndDoubleQuote()
									: GetEndSingleQuote() );
	if( !cRet )
	{
		// dann ueber die Language das richtige Zeichen heraussuchen
		if( LANGUAGE_NONE == eLang )
			cRet = cInsChar;
		else
		{
			LocaleDataWrapper& rLcl = GetLocaleDataWrapper( eLang );
			String sRet( bSttQuote
							? ( '\"' == cInsChar
								? rLcl.getDoubleQuotationMarkStart()
								: rLcl.getQuotationMarkStart() )
							: ( '\"' == cInsChar
								? rLcl.getDoubleQuotationMarkEnd()
								: rLcl.getQuotationMarkEnd() ));
			cRet = sRet.Len() ? sRet.GetChar( 0 ) : cInsChar;
		}
	}
	return cRet;
}

void SvxAutoCorrect::InsertQuote( SvxAutoCorrDoc& rDoc, xub_StrLen nInsPos,
									sal_Unicode cInsChar, BOOL bSttQuote,
									BOOL bIns )
{
	LanguageType eLang = rDoc.GetLanguage( nInsPos, FALSE );
	sal_Unicode cRet = GetQuote( cInsChar, bSttQuote, eLang );

	//JP 13.02.99: damit beim Undo das "einfuegte" Zeichen wieder erscheint,
	//				wird es erstmal eingefuegt und dann ueberschrieben
	String sChg( cInsChar );
	if( bIns )
		rDoc.Insert( nInsPos, sChg );
	else
		rDoc.Replace( nInsPos, sChg );

	//JP 13.08.97: Bug 42477 - bei doppelten Anfuehrungszeichen muss bei
	//				franzoesischer Sprache an Anfang ein Leerzeichen dahinter
	//				und am Ende ein Leerzeichen dahinter eingefuegt werden.
	sChg = cRet;

	if( '\"' == cInsChar )
	{
		if( LANGUAGE_SYSTEM == eLang )
			eLang = GetAppLang();
		switch( eLang )
		{
		case LANGUAGE_FRENCH:
		case LANGUAGE_FRENCH_BELGIAN:
		case LANGUAGE_FRENCH_CANADIAN:
		case LANGUAGE_FRENCH_SWISS:
		case LANGUAGE_FRENCH_LUXEMBOURG:
			// JP 09.02.99: das zusaetzliche Zeichen immer per Insert einfuegen.
			//				Es ueberschreibt nichts!
			{
				String s( static_cast< sal_Unicode >(0xA0) );
                    // UNICODE code for no break space
				if( rDoc.Insert( bSttQuote ? nInsPos+1 : nInsPos, s ))
				{
					if( !bSttQuote )
						++nInsPos;
				}
			}
			break;
		}
	}

	rDoc.Replace( nInsPos, sChg );
}

String SvxAutoCorrect::GetQuote( SvxAutoCorrDoc& rDoc, xub_StrLen nInsPos,
								sal_Unicode cInsChar, BOOL bSttQuote )
{
	LanguageType eLang = rDoc.GetLanguage( nInsPos, FALSE );
	sal_Unicode cRet = GetQuote( cInsChar, bSttQuote, eLang );

	String sRet( cRet );
	//JP 13.08.97: Bug 42477 - bei doppelten Anfuehrungszeichen muss bei
	//				franzoesischer Sprache an Anfang ein Leerzeichen dahinter
	//				und am Ende ein Leerzeichen dahinter eingefuegt werden.
	if( '\"' == cInsChar )
	{
		if( LANGUAGE_SYSTEM == eLang )
			eLang = GetAppLang();
		switch( eLang )
		{
		case LANGUAGE_FRENCH:
		case LANGUAGE_FRENCH_BELGIAN:
		case LANGUAGE_FRENCH_CANADIAN:
		case LANGUAGE_FRENCH_SWISS:
		case LANGUAGE_FRENCH_LUXEMBOURG:
			if( bSttQuote )
				sRet += ' ';
			else
				sRet.Insert( ' ', 0 );
			break;
		}
	}
	return sRet;
}

ULONG SvxAutoCorrect::AutoCorrect( SvxAutoCorrDoc& rDoc, const String& rTxt,
									xub_StrLen nInsPos, sal_Unicode cChar,
									BOOL bInsert )
{
	ULONG nRet = 0;
	do{		                            // only for middle check loop !!
		if( cChar )
		{
			//JP 10.02.97: doppelte Spaces verhindern
			if( nInsPos && ' ' == cChar &&
				IsAutoCorrFlag( IngnoreDoubleSpace ) &&
				' ' == rTxt.GetChar( nInsPos - 1 ) )
			{
				nRet = IngnoreDoubleSpace;
				break;
			}

			BOOL bSingle = '\'' == cChar;
			BOOL bIsReplaceQuote =
						(IsAutoCorrFlag( ChgQuotes ) && ('\"' == cChar )) ||
						(IsAutoCorrFlag( ChgSglQuotes ) && bSingle );
			if( bIsReplaceQuote )
			{
				sal_Unicode cPrev;
				BOOL bSttQuote = !nInsPos ||
						IsWordDelim( ( cPrev = rTxt.GetChar( nInsPos-1 ))) ||
// os: #56034# - Warum kein schliessendes Anfuehrungszeichen nach dem Bindestrich?
//						strchr( "-([{", cPrev ) ||
						lcl_IsInAsciiArr( "([{", cPrev ) ||
						( cEmDash && cEmDash == cPrev ) ||
						( cEnDash && cEnDash == cPrev );

				InsertQuote( rDoc, nInsPos, cChar, bSttQuote, bInsert );
				nRet = bSingle ? ChgSglQuotes : ChgQuotes;
				break;
			}

			if( bInsert )
				rDoc.Insert( nInsPos, cChar );
			else
				rDoc.Replace( nInsPos, cChar );
		}

		if( !nInsPos )
			break;

		xub_StrLen nPos = nInsPos - 1;

		// Bug 19286: nur direkt hinter dem "Wort" aufsetzen
		if( IsWordDelim( rTxt.GetChar( nPos )))
			break;

		// automatisches Fett oder Unterstreichen setzen?
		if( '*' == cChar || '_' == cChar )
		{
			if( IsAutoCorrFlag( ChgWeightUnderl ) &&
				FnChgWeightUnderl( rDoc, rTxt, 0, nPos+1 ) )
				nRet = ChgWeightUnderl;
			break;
		}

		while( nPos && !IsWordDelim( rTxt.GetChar( --nPos )))
			;

		// Absatz-Anfang oder ein Blank gefunden, suche nach dem Wort
		// Kuerzel im Auto
		xub_StrLen nCapLttrPos = nPos+1;		// auf das 1. Zeichen
		if( !nPos && !IsWordDelim( rTxt.GetChar( 0 )))
			--nCapLttrPos;			// Absatz Anfang und kein Blank !

		LanguageType eLang = rDoc.GetLanguage( nCapLttrPos, FALSE );
		if( LANGUAGE_SYSTEM == eLang )
			eLang = MsLangId::getSystemLanguage();
		CharClass& rCC = GetCharClass( eLang );

		// Bug 19285: Symbolzeichen nicht anfassen
		if( lcl_IsSymbolChar( rCC, rTxt, nCapLttrPos, nInsPos ))
			break;

		if( ( IsAutoCorrFlag( nRet = ChgFractionSymbol ) &&
				FnChgFractionSymbol( rDoc, rTxt, nCapLttrPos, nInsPos ) ) ||
			( IsAutoCorrFlag( nRet = ChgOrdinalNumber ) &&
				FnChgOrdinalNumber( rDoc, rTxt, nCapLttrPos, nInsPos, eLang ) ) ||
			( IsAutoCorrFlag( nRet = SetINetAttr ) &&
				( ' ' == cChar || '\t' == cChar || 0x0a == cChar || !cChar ) &&
				FnSetINetAttr( rDoc, rTxt, nCapLttrPos, nInsPos, eLang ) ) )
			;
		else
		{
			nRet = 0;
			// Grossbuchstabe am Satz-Anfang ??
			if( IsAutoCorrFlag( CptlSttSntnc ) &&
				FnCptlSttSntnc( rDoc, rTxt, TRUE, nCapLttrPos, nInsPos, eLang ) )
				nRet |= CptlSttSntnc;

			// Zwei Grossbuchstaben am Wort-Anfang ??
			if( IsAutoCorrFlag( CptlSttWrd ) &&
				FnCptlSttWrd( rDoc, rTxt, nCapLttrPos, nInsPos, eLang ) )
				nRet |= CptlSttWrd;

			if( IsAutoCorrFlag( ChgToEnEmDash ) &&
				FnChgToEnEmDash( rDoc, rTxt, nCapLttrPos, nInsPos, eLang ) )
				nRet |= ChgToEnEmDash;
		}

	} while( FALSE );

	SfxViewFrame* pVFrame;
	if( nRet && 0 != (pVFrame = SfxViewFrame::Current()) &&
		pVFrame->GetFrame() )
	{
		ULONG nHelpId = 0;
		if( nRet & ( Autocorrect|CptlSttSntnc|CptlSttWrd|ChgToEnEmDash ) )
		{
			// von 0 - 15
			if( nRet & ChgToEnEmDash )
				nHelpId += 8;
			if( nRet & Autocorrect )
				nHelpId += 4;
			if( nRet & CptlSttSntnc )
				nHelpId += 2;
			if( nRet & CptlSttWrd )
				nHelpId += 1;
		}
		else
		{
			     if( nRet & ChgQuotes) 			nHelpId = 16;
			else if( nRet & ChgSglQuotes) 		nHelpId = 17;
			else if( nRet & SetINetAttr) 		nHelpId = 18;
			else if( nRet & IngnoreDoubleSpace) nHelpId = 19;
			else if( nRet & ChgWeightUnderl) 	nHelpId = 20;
			else if( nRet & ChgFractionSymbol ) nHelpId = 21;
			else if( nRet & ChgOrdinalNumber)	nHelpId = 22;
		}

		DBG_ASSERT( nHelpId && nHelpId < (HID_AUTOCORR_HELP_END -
										  HID_AUTOCORR_HELP_START + 1),
					"wrong HelpId Range" );

		if( nHelpId )
		{
			nHelpId += HID_AUTOCORR_HELP_START - 1;
			SfxHelp::OpenHelpAgent( pVFrame->GetFrame(), nHelpId );
		}
	}


	return nRet;
}

SvxAutoCorrectLanguageLists& SvxAutoCorrect::_GetLanguageList(
														LanguageType eLang )
{
	if( !pLangTable->IsKeyValid( ULONG( eLang )))
		CreateLanguageFile( eLang, TRUE);
	return *pLangTable->Seek( ULONG( eLang ) );
}

void SvxAutoCorrect::SaveCplSttExceptList( LanguageType eLang )
{
	if( pLangTable->IsKeyValid( ULONG( eLang )))
	{
		SvxAutoCorrectLanguageListsPtr pLists = pLangTable->Seek(ULONG(eLang));
		if( pLists )
			pLists->SaveCplSttExceptList();
	}
#ifndef PRODUCT
	else
	{
		DBG_ERROR("speichern einer leeren Liste?");
	}
#endif
}

void SvxAutoCorrect::SaveWrdSttExceptList(LanguageType eLang)
{
	if(pLangTable->IsKeyValid(ULONG(eLang)))
	{
		SvxAutoCorrectLanguageListsPtr pLists = pLangTable->Seek(ULONG(eLang));
		if(pLists)
			pLists->SaveWrdSttExceptList();
	}
#ifndef PRODUCT
	else
	{
		DBG_ERROR("speichern einer leeren Liste?");
	}
#endif
}


	// fuegt ein einzelnes Wort hinzu. Die Liste wird sofort
	// in die Datei geschrieben!
BOOL SvxAutoCorrect::AddCplSttException( const String& rNew,
										LanguageType eLang )
{
	SvxAutoCorrectLanguageListsPtr pLists = 0;
	//entweder die richtige Sprache ist vorhanden oder es kommt in die allg. Liste
	if( pLangTable->IsKeyValid(ULONG(eLang)))
		pLists = pLangTable->Seek(ULONG(eLang));
	else if(pLangTable->IsKeyValid(ULONG(LANGUAGE_DONTKNOW))||
			CreateLanguageFile(LANGUAGE_DONTKNOW, TRUE))
	{
		pLists = pLangTable->Seek(ULONG(LANGUAGE_DONTKNOW));
	}
	DBG_ASSERT(pLists, "keine Autokorrekturdatei");
	return pLists->AddToCplSttExceptList(rNew);
}


	// fuegt ein einzelnes Wort hinzu. Die Liste wird sofort
	// in die Datei geschrieben!
BOOL SvxAutoCorrect::AddWrtSttException( const String& rNew,
										 LanguageType eLang )
{
	SvxAutoCorrectLanguageListsPtr pLists = 0;
	//entweder die richtige Sprache ist vorhanden oder es kommt in die allg. Liste
	if(pLangTable->IsKeyValid(ULONG(eLang)))
		pLists = pLangTable->Seek(ULONG(eLang));
	else if(pLangTable->IsKeyValid(ULONG(LANGUAGE_DONTKNOW))||
			CreateLanguageFile(LANGUAGE_DONTKNOW, TRUE))
		pLists = pLangTable->Seek(ULONG(LANGUAGE_DONTKNOW));
	DBG_ASSERT(pLists, "keine Autokorrekturdatei");
	return pLists->AddToWrdSttExceptList(rNew);
}




void SvxAutoCorrect::SetUserAutoCorrFileName( const String& rNew )
{
	if( sUserAutoCorrFile != rNew )
	{
		sUserAutoCorrFile = rNew;

		// sind die Listen gesetzt sind, so muessen sie jetzt geloescht
		// werden
		lcl_ClearTable(*pLangTable);
		nFlags &= ~(CplSttLstLoad | WrdSttLstLoad | ChgWordLstLoad );
	}
}

void SvxAutoCorrect::SetShareAutoCorrFileName( const String& rNew )
{
	if( sShareAutoCorrFile != rNew )
	{
		sShareAutoCorrFile = rNew;

		// sind die Listen gesetzt sind, so muessen sie jetzt geloescht
		// werden
		lcl_ClearTable(*pLangTable);
		nFlags &= ~(CplSttLstLoad | WrdSttLstLoad | ChgWordLstLoad );
	}
}


BOOL SvxAutoCorrect::GetPrevAutoCorrWord( SvxAutoCorrDoc& rDoc,
										const String& rTxt, xub_StrLen nPos,
										String& rWord ) const
{
	if( !nPos )
		return FALSE;

	xub_StrLen nEnde = nPos;

	// dahinter muss ein Blank oder Tab folgen!
	if( ( nPos < rTxt.Len() &&
		!IsWordDelim( rTxt.GetChar( nPos ))) ||
		IsWordDelim( rTxt.GetChar( --nPos )))
		return FALSE;

	while( nPos && !IsWordDelim( rTxt.GetChar( --nPos )))
		;

	// Absatz-Anfang oder ein Blank gefunden, suche nach dem Wort
	// Kuerzel im Auto
	xub_StrLen nCapLttrPos = nPos+1;		// auf das 1. Zeichen
	if( !nPos && !IsWordDelim( rTxt.GetChar( 0 )))
		--nCapLttrPos;			// Absatz Anfang und kein Blank !

	while( lcl_IsInAsciiArr( sImplSttSkipChars, rTxt.GetChar( nCapLttrPos )) )
		if( ++nCapLttrPos >= nEnde )
			return FALSE;

	// Bug 19285: Symbolzeichen nicht anfassen
	// Interresant erst ab 3 Zeichen
	if( 3 > nEnde - nCapLttrPos )
		return FALSE;

	LanguageType eLang = rDoc.GetLanguage( nCapLttrPos, FALSE );
	if( LANGUAGE_SYSTEM == eLang )
		eLang = MsLangId::getSystemLanguage();

	SvxAutoCorrect* pThis = (SvxAutoCorrect*)this;
	CharClass& rCC = pThis->GetCharClass( eLang );

	if( lcl_IsSymbolChar( rCC, rTxt, nCapLttrPos, nEnde ))
		return FALSE;

	rWord = rTxt.Copy( nCapLttrPos, nEnde - nCapLttrPos );
	return TRUE;
}

BOOL SvxAutoCorrect::CreateLanguageFile( LanguageType eLang, BOOL bNewFile )
{
	DBG_ASSERT(!pLangTable->IsKeyValid(ULONG(eLang)), "Sprache ist bereits vorhanden");

	String sUserDirFile( GetAutoCorrFileName( eLang, TRUE, FALSE )),
		   sShareDirFile( sUserDirFile );
	SvxAutoCorrectLanguageListsPtr pLists = 0;

	Time nMinTime( 0, 2 ), nAktTime, nLastCheckTime;
	ULONG nFndPos;
	if( TABLE_ENTRY_NOTFOUND !=
					pLastFileTable->SearchKey( ULONG( eLang ), &nFndPos ) &&
		( nLastCheckTime.SetTime( pLastFileTable->GetObject( nFndPos )),
			nLastCheckTime < nAktTime ) &&
		( nAktTime - nLastCheckTime ) < nMinTime )
	{
		// no need to test the file, because the last check is not older then
		// 2 minutes.
		if( bNewFile )
		{
			sShareDirFile = sUserDirFile;
			pLists = new SvxAutoCorrectLanguageLists( *this, sShareDirFile,
														sUserDirFile, eLang );
			pLangTable->Insert( ULONG(eLang), pLists );
			pLastFileTable->Remove( ULONG( eLang ) );
		}
	}
	else if( ( FStatHelper::IsDocument( sUserDirFile ) ||
		 	   FStatHelper::IsDocument( sShareDirFile =
		  					GetAutoCorrFileName( eLang, FALSE, FALSE ) ) ) ||
		( sShareDirFile = sUserDirFile, bNewFile ))
	{
		pLists = new SvxAutoCorrectLanguageLists( *this, sShareDirFile,
													sUserDirFile, eLang );
		pLangTable->Insert( ULONG(eLang), pLists );
		pLastFileTable->Remove( ULONG( eLang ) );
	}
	else if( !bNewFile )
	{
		if( !pLastFileTable->Insert( ULONG( eLang ), nAktTime.GetTime() ))
			pLastFileTable->Replace( ULONG( eLang ), nAktTime.GetTime() );
	}
	return pLists != 0;
}


	//	- return den Ersetzungstext (nur fuer SWG-Format, alle anderen
	//		koennen aus der Wortliste herausgeholt werden!)
BOOL SvxAutoCorrect::GetLongText( const com::sun::star::uno::Reference < com::sun::star::embed::XStorage >&, const String&, const String& , String& )
{
	return FALSE;
}

void EncryptBlockName_Imp( String& rName )
{
	xub_StrLen nLen, nPos = 1;
	rName.Insert( '#', 0 );
	sal_Unicode* pName = rName.GetBufferAccess();
	for ( nLen = rName.Len(), ++pName; nPos < nLen; ++nPos, ++pName )
	{
		if( lcl_IsInAsciiArr( "!/:.\\", *pName ))
			*pName &= 0x0f;
	}
}

/* This code is copied from SwXMLTextBlocks::GeneratePackageName */
void GeneratePackageName ( const String& rShort, String& rPackageName )
{
	rPackageName = rShort;
	xub_StrLen nPos = 0;
	sal_Unicode pDelims[] = { '!', '/', ':', '.', '\\', 0 };
	ByteString sByte ( rPackageName, RTL_TEXTENCODING_UTF7);
	rPackageName = String (sByte, RTL_TEXTENCODING_ASCII_US);
	while( STRING_NOTFOUND != ( nPos = rPackageName.SearchChar( pDelims, nPos )))
	{
		rPackageName.SetChar( nPos, '_' );
		++nPos;
	}
}

void DecryptBlockName_Imp( String& rName )
{
	if( '#' == rName.GetChar( 0 ) )
	{
		rName.Erase( 0, 1 );
		sal_Unicode* pName = rName.GetBufferAccess();
		xub_StrLen nLen, nPos;
		for ( nLen = rName.Len(), nPos = 0; nPos < nLen; ++nPos, ++pName )
			switch( *pName )
			{
			case 0x01:	*pName = '!';	break;
			case 0x0A:	*pName = ':';	break;
			case 0x0C:	*pName = '\\';	break;
			case 0x0E:	*pName = '.';	break;
			case 0x0F:	*pName = '/';	break;
			}
	}
}

/* -----------------18.11.98 13:46-------------------
 *
 * --------------------------------------------------*/
BOOL SvxAutoCorrect::FindInWrdSttExceptList( LanguageType eLang,
											 const String& sWord )
{
	//zuerst nach eLang suchen, dann nach der Obersprace US-Englisch -> Englisch
	//und zuletzt in LANGUAGE_DONTKNOW
	ULONG nTmpKey1 = eLang & 0x7ff; // die Hauptsprache in vielen Faellen u.B. DE
	ULONG nTmpKey2 = eLang & 0x3ff; // sonst z.B. EN
	String sTemp(sWord);
	if( pLangTable->IsKeyValid( ULONG( eLang )) ||
		CreateLanguageFile( eLang, FALSE ) )
	{
		//die Sprache ist vorhanden - also her damit
		SvxAutoCorrectLanguageListsPtr pList = pLangTable->Seek(ULONG(eLang));
		String _sTemp(sWord);
		if(pList->GetWrdSttExceptList()->Seek_Entry(&_sTemp))
			return TRUE;

	}
	// wenn es hier noch nicht gefunden werden konnte, dann weitersuchen
	ULONG nTmp;
	if( ((nTmp = nTmpKey1) != (ULONG)eLang &&
		 ( pLangTable->IsKeyValid( nTmpKey1 ) ||
		   CreateLanguageFile( LanguageType( nTmpKey1 ), FALSE ) )) ||
		(( nTmp = nTmpKey2) != (ULONG)eLang &&
		 ( pLangTable->IsKeyValid( nTmpKey2 ) ||
		   CreateLanguageFile( LanguageType( nTmpKey2 ), FALSE ) )) )
	{
		//die Sprache ist vorhanden - also her damit
		SvxAutoCorrectLanguageListsPtr pList = pLangTable->Seek(nTmp);
		if(pList->GetWrdSttExceptList()->Seek_Entry(&sTemp))
			return TRUE;
	}
	if(pLangTable->IsKeyValid(ULONG(LANGUAGE_DONTKNOW))|| CreateLanguageFile(LANGUAGE_DONTKNOW, FALSE))
	{
		//die Sprache ist vorhanden - also her damit
		SvxAutoCorrectLanguageListsPtr pList = pLangTable->Seek(ULONG(LANGUAGE_DONTKNOW));
		if(pList->GetWrdSttExceptList()->Seek_Entry(&sTemp))
			return TRUE;
	}
	return FALSE;
}
/* -----------------18.11.98 14:28-------------------
 *
 * --------------------------------------------------*/
BOOL lcl_FindAbbreviation( const SvStringsISortDtor* pList, const String& sWord)
{
	String sAbk( '~' );
	USHORT nPos;
	pList->Seek_Entry( &sAbk, &nPos );
	if( nPos < pList->Count() )
	{
		String sLowerWord( sWord ); sLowerWord.ToLowerAscii();
		const String* pAbk;
		for( USHORT n = nPos;
				n < pList->Count() &&
				'~' == ( pAbk = (*pList)[ n ])->GetChar( 0 );
			++n )
		{
			// ~ und ~. sind nicht erlaubt!
			if( 2 < pAbk->Len() && pAbk->Len() - 1 <= sWord.Len() )
			{
				String sLowerAbk( *pAbk ); sLowerAbk.ToLowerAscii();
				for( xub_StrLen i = sLowerAbk.Len(), ii = sLowerWord.Len(); i; )
				{
					if( !--i )		// stimmt ueberein
						return TRUE;

					if( sLowerAbk.GetChar( i ) != sLowerWord.GetChar( --ii ))
						break;
				}
			}
		}
	}
	DBG_ASSERT( !(nPos && '~' == (*pList)[ --nPos ]->GetChar( 0 ) ),
			"falsch sortierte ExeptionListe?" );
	return FALSE;
}
/* -----------------18.11.98 14:49-------------------
 *
 * --------------------------------------------------*/
BOOL SvxAutoCorrect::FindInCplSttExceptList(LanguageType eLang,
								const String& sWord, BOOL bAbbreviation)
{
	//zuerst nach eLang suchen, dann nach der Obersprace US-Englisch -> Englisch
	//und zuletzt in LANGUAGE_DONTKNOW
	ULONG nTmpKey1 = eLang & 0x7ff; // die Hauptsprache in vielen Faellen u.B. DE
	ULONG nTmpKey2 = eLang & 0x3ff; // sonst z.B. EN
	String sTemp( sWord );
	if( pLangTable->IsKeyValid( ULONG( eLang )) ||
		CreateLanguageFile( eLang, FALSE ))
	{
		//die Sprache ist vorhanden - also her damit
		SvxAutoCorrectLanguageListsPtr pLists = pLangTable->Seek(ULONG(eLang));
		const SvStringsISortDtor* pList = pLists->GetCplSttExceptList();
		if(bAbbreviation ? lcl_FindAbbreviation( pList, sWord)
						 : pList->Seek_Entry( &sTemp ) )
			return TRUE;
	}
	// wenn es hier noch nicht gefunden werden konnte, dann weitersuchen
	ULONG nTmp;

	if( ((nTmp = nTmpKey1) != (ULONG)eLang &&
		 ( pLangTable->IsKeyValid( nTmpKey1 ) ||
		   CreateLanguageFile( LanguageType( nTmpKey1 ), FALSE ) )) ||
		(( nTmp = nTmpKey2) != (ULONG)eLang &&
		 ( pLangTable->IsKeyValid( nTmpKey2 ) ||
		   CreateLanguageFile( LanguageType( nTmpKey2 ), FALSE ) )) )
	{
		//die Sprache ist vorhanden - also her damit
		SvxAutoCorrectLanguageListsPtr pLists = pLangTable->Seek(nTmp);
		const SvStringsISortDtor* pList = pLists->GetCplSttExceptList();
		if(bAbbreviation ? lcl_FindAbbreviation( pList, sWord)
						 : pList->Seek_Entry( &sTemp ) )
			return TRUE;
	}
	if(pLangTable->IsKeyValid(ULONG(LANGUAGE_DONTKNOW))|| CreateLanguageFile(LANGUAGE_DONTKNOW, FALSE))
	{
		//die Sprache ist vorhanden - also her damit
		SvxAutoCorrectLanguageListsPtr pLists = pLangTable->Seek(LANGUAGE_DONTKNOW);
		const SvStringsISortDtor* pList = pLists->GetCplSttExceptList();
		if(bAbbreviation ? lcl_FindAbbreviation( pList, sWord)
						 : pList->Seek_Entry( &sTemp ) )
			return TRUE;
	}
	return FALSE;

}

/* -----------------20.11.98 11:53-------------------
 *
 * --------------------------------------------------*/
String SvxAutoCorrect::GetAutoCorrFileName( LanguageType eLang,
											BOOL bNewFile, BOOL bTst ) const
{
    String sRet, sExt( MsLangId::convertLanguageToIsoString( eLang ) );
    sExt.Insert('_', 0);
    sExt.AppendAscii( ".dat" );
	if( bNewFile )
        ( sRet = sUserAutoCorrFile )  += sExt;
	else if( !bTst )
        ( sRet = sShareAutoCorrFile )  += sExt;
	else
	{
		// test first in the user directory - if not exist, then
        ( sRet = sUserAutoCorrFile ) += sExt;
		if( !FStatHelper::IsDocument( sRet ))
            ( sRet = sShareAutoCorrFile ) += sExt;
	}
	return sRet;
}

/* -----------------18.11.98 11:16-------------------
 *
 * --------------------------------------------------*/
SvxAutoCorrectLanguageLists::SvxAutoCorrectLanguageLists(
				SvxAutoCorrect& rParent,
				const String& rShareAutoCorrectFile,
				const String& rUserAutoCorrectFile,
				LanguageType eLang)
:	sShareAutoCorrFile( rShareAutoCorrectFile ),
	sUserAutoCorrFile( rUserAutoCorrectFile ),
	eLanguage(eLang),
	pCplStt_ExcptLst( 0 ),
	pWrdStt_ExcptLst( 0 ),
	rAutoCorrect(rParent),
	nFlags(0)
{
}

/* -----------------18.11.98 11:16-------------------
 *
 * --------------------------------------------------*/
SvxAutoCorrectLanguageLists::~SvxAutoCorrectLanguageLists()
{
	delete pCplStt_ExcptLst;
	delete pWrdStt_ExcptLst;
}

/* -----------------18.11.98 11:26-------------------
 *
 * --------------------------------------------------*/
BOOL SvxAutoCorrectLanguageLists::IsFileChanged_Imp()
{
	// nur alle 2 Minuten aufs FileSystem zugreifen um den
	// Dateistempel zu ueberpruefen
	BOOL bRet = FALSE;

	Time nMinTime( 0, 2 );
	Time nAktTime;
	if( aLastCheckTime > nAktTime ||	   				// ueberlauf ?
		( nAktTime -= aLastCheckTime ) > nMinTime )		// min Zeit vergangen
	{
		Date aTstDate; Time aTstTime;
		if( FStatHelper::GetModifiedDateTimeOfFile( sShareAutoCorrFile,
											&aTstDate, &aTstTime ) &&
			( aModifiedDate != aTstDate || aModifiedTime != aTstTime ))
		{
			bRet = TRUE;
			// dann mal schnell alle Listen entfernen!
			if( CplSttLstLoad & nFlags && pCplStt_ExcptLst )
				delete pCplStt_ExcptLst, pCplStt_ExcptLst = 0;
			if( WrdSttLstLoad & nFlags && pWrdStt_ExcptLst )
				delete pWrdStt_ExcptLst, pWrdStt_ExcptLst = 0;
			nFlags &= ~(CplSttLstLoad | WrdSttLstLoad | ChgWordLstLoad );
		}
		aLastCheckTime = Time();
	}
	return bRet;
}

void SvxAutoCorrectLanguageLists::LoadXMLExceptList_Imp(
										SvStringsISortDtor*& rpLst,
										const sal_Char* pStrmName,
                                        SotStorageRef& rStg)
{
	if( rpLst )
		rpLst->DeleteAndDestroy( 0, rpLst->Count() );
	else
		rpLst = new SvStringsISortDtor( 16, 16 );

	{
		String sStrmName( pStrmName, RTL_TEXTENCODING_MS_1252 );
		String sTmp( sStrmName );

		if( rStg.Is() && rStg->IsStream( sStrmName ) )
		{
			SvStorageStreamRef xStrm = rStg->OpenSotStream( sTmp,
				( STREAM_READ | STREAM_SHARE_DENYWRITE | STREAM_NOCREATE ) );
			if( SVSTREAM_OK != xStrm->GetError())
			{
				xStrm.Clear();
				rStg.Clear();
				RemoveStream_Imp( sStrmName );
			}
			else
			{
				uno::Reference< lang::XMultiServiceFactory > xServiceFactory =
					comphelper::getProcessServiceFactory();
				DBG_ASSERT( xServiceFactory.is(),
					"XMLReader::Read: got no service manager" );
				if( !xServiceFactory.is() )
				{
					// Throw an exception ?
				}

				xml::sax::InputSource aParserInput;
				aParserInput.sSystemId = sStrmName;

				xStrm->Seek( 0L );
				xStrm->SetBufferSize( 8 * 1024 );
				aParserInput.aInputStream = new utl::OInputStreamWrapper( *xStrm );

				// get parser
				uno::Reference< XInterface > xXMLParser = xServiceFactory->createInstance(
					OUString::createFromAscii("com.sun.star.xml.sax.Parser") );
				DBG_ASSERT( xXMLParser.is(),
					"XMLReader::Read: com.sun.star.xml.sax.Parser service missing" );
				if( !xXMLParser.is() )
				{
					// Maybe throw an exception?
				}

				// get filter
				// #110680#
				// uno::Reference< xml::sax::XDocumentHandler > xFilter = new SvXMLExceptionListImport ( *rpLst );
				uno::Reference< xml::sax::XDocumentHandler > xFilter = new SvXMLExceptionListImport ( xServiceFactory, *rpLst );

				// connect parser and filter
				uno::Reference< xml::sax::XParser > xParser( xXMLParser, UNO_QUERY );
				xParser->setDocumentHandler( xFilter );

				// parse
				try
				{
					xParser->parseStream( aParserInput );
				}
				catch( xml::sax::SAXParseException&  )
				{
					// re throw ?
				}
				catch( xml::sax::SAXException&  )
				{
					// re throw ?
				}
				catch( io::IOException& )
				{
					// re throw ?
				}
			}
		}

		// Zeitstempel noch setzen
		FStatHelper::GetModifiedDateTimeOfFile( sShareAutoCorrFile,
										&aModifiedDate, &aModifiedTime );
		aLastCheckTime = Time();
	}

}
/* -----------------18.11.98 11:26-------------------
 *
 * --------------------------------------------------*/
void SvxAutoCorrectLanguageLists::SaveExceptList_Imp(
							const SvStringsISortDtor& rLst,
							const sal_Char* pStrmName,
                            SotStorageRef &rStg,
							BOOL bConvert )
{
	if( rStg.Is() )
	{
		String sStrmName( pStrmName, RTL_TEXTENCODING_MS_1252 );
		if( !rLst.Count() )
		{
			rStg->Remove( sStrmName );
			rStg->Commit();
		}
		else
		{
            SotStorageStreamRef xStrm = rStg->OpenSotStream( sStrmName,
					( STREAM_READ | STREAM_WRITE | STREAM_SHARE_DENYWRITE ) );
			if( xStrm.Is() )
			{
				xStrm->SetSize( 0 );
				xStrm->SetBufferSize( 8192 );
				String aPropName( String::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM("MediaType") ) );
				OUString aMime( RTL_CONSTASCII_USTRINGPARAM("text/xml") );
				uno::Any aAny;
				aAny <<= aMime;
				xStrm->SetProperty( aPropName, aAny );


				uno::Reference< lang::XMultiServiceFactory > xServiceFactory =
					comphelper::getProcessServiceFactory();
				DBG_ASSERT( xServiceFactory.is(),
							"XMLReader::Read: got no service manager" );
				if( !xServiceFactory.is() )
				{
					// Throw an exception ?
				}

   			 	uno::Reference < XInterface > xWriter (xServiceFactory->createInstance(
   			    	 OUString(RTL_CONSTASCII_USTRINGPARAM("com.sun.star.xml.sax.Writer"))));
   			 	DBG_ASSERT(xWriter.is(),"com.sun.star.xml.sax.Writer service missing");
				uno::Reference < io::XOutputStream> xOut = new utl::OOutputStreamWrapper( *xStrm );
   			 	uno::Reference<io::XActiveDataSource> xSrc(xWriter, uno::UNO_QUERY);
   			 	xSrc->setOutputStream(xOut);

   			 	uno::Reference<xml::sax::XDocumentHandler> xHandler(xWriter, uno::UNO_QUERY);

				// #110680#
   			 	// SvXMLExceptionListExport aExp(rLst, sStrmName, xHandler);
   			 	SvXMLExceptionListExport aExp( xServiceFactory, rLst, sStrmName, xHandler );

				aExp.exportDoc( XML_BLOCK_LIST );

				xStrm->Commit();
				if( xStrm->GetError() == SVSTREAM_OK )
				{
					xStrm.Clear();
					if (!bConvert)
					{
						rStg->Commit();
						if( SVSTREAM_OK != rStg->GetError() )
						{
							rStg->Remove( sStrmName );
							rStg->Commit();
						}
					}
				}
			}
		}
	}
}
/* -----------------18.11.98 11:26-------------------
 *
 * --------------------------------------------------*/
SvStringsISortDtor* SvxAutoCorrectLanguageLists::GetCplSttExceptList()
{
	if( !( CplSttLstLoad & nFlags ) || IsFileChanged_Imp() )
		SetCplSttExceptList( LoadCplSttExceptList() );
	return pCplStt_ExcptLst;
}
/* -----------------18.11.98 11:26-------------------
 *
 * --------------------------------------------------*/
BOOL SvxAutoCorrectLanguageLists::AddToCplSttExceptList(const String& rNew)
{
	String* pNew = new String( rNew );
    if( rNew.Len() && GetCplSttExceptList()->Insert( pNew ) )
	{
		MakeUserStorage_Impl();
        SotStorageRef xStg = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, TRUE );

		SaveExceptList_Imp( *pCplStt_ExcptLst, pXMLImplCplStt_ExcptLstStr, xStg );

		xStg = 0;
		// Zeitstempel noch setzen
		FStatHelper::GetModifiedDateTimeOfFile( sUserAutoCorrFile,
											&aModifiedDate, &aModifiedTime );
		aLastCheckTime = Time();
	}
	else
		delete pNew, pNew = 0;
	return 0 != pNew;
}
/* -----------------18.11.98 15:20-------------------
 *
 * --------------------------------------------------*/
BOOL SvxAutoCorrectLanguageLists::AddToWrdSttExceptList(const String& rNew)
{
	String* pNew = new String( rNew );
	SvStringsISortDtor* pExceptList = LoadWrdSttExceptList();
	if( rNew.Len() && pExceptList && pExceptList->Insert( pNew ) )
	{
		MakeUserStorage_Impl();
        SotStorageRef xStg = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, TRUE );

		SaveExceptList_Imp( *pWrdStt_ExcptLst, pXMLImplWrdStt_ExcptLstStr, xStg );

		xStg = 0;
		// Zeitstempel noch setzen
		FStatHelper::GetModifiedDateTimeOfFile( sUserAutoCorrFile,
											&aModifiedDate, &aModifiedTime );
		aLastCheckTime = Time();
	}
	else
		delete pNew, pNew = 0;
	return 0 != pNew;
}

/* -----------------18.11.98 11:26-------------------
 *
 * --------------------------------------------------*/
SvStringsISortDtor* SvxAutoCorrectLanguageLists::LoadCplSttExceptList()
{
	SotStorageRef xStg = new SotStorage( sShareAutoCorrFile, STREAM_READ | STREAM_SHARE_DENYNONE, TRUE );
	String sTemp ( RTL_CONSTASCII_USTRINGPARAM ( pXMLImplCplStt_ExcptLstStr ) );
	if( xStg.Is() && xStg->IsContained( sTemp ) )
		LoadXMLExceptList_Imp( pCplStt_ExcptLst, pXMLImplCplStt_ExcptLstStr, xStg );

	return pCplStt_ExcptLst;
}

/* -----------------18.11.98 11:26-------------------
 *
 * --------------------------------------------------*/
void SvxAutoCorrectLanguageLists::SaveCplSttExceptList()
{
	MakeUserStorage_Impl();
    SotStorageRef xStg = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, TRUE );

	SaveExceptList_Imp( *pCplStt_ExcptLst, pXMLImplCplStt_ExcptLstStr, xStg );

	xStg = 0;

    // Zeitstempel noch setzen
	FStatHelper::GetModifiedDateTimeOfFile( sUserAutoCorrFile,
											&aModifiedDate, &aModifiedTime );
	aLastCheckTime = Time();
}

/* -----------------18.11.98 11:26-------------------
 *
 * --------------------------------------------------*/
void SvxAutoCorrectLanguageLists::SetCplSttExceptList( SvStringsISortDtor* pList )
{
	if( pCplStt_ExcptLst && pList != pCplStt_ExcptLst )
		delete pCplStt_ExcptLst;

	pCplStt_ExcptLst = pList;
	if( !pCplStt_ExcptLst )
	{
		DBG_ASSERT( !this, "keine gueltige Liste" );
		pCplStt_ExcptLst = new SvStringsISortDtor( 16, 16 );
	}
	nFlags |= CplSttLstLoad;
}
/* -----------------18.11.98 11:26-------------------
 *
 * --------------------------------------------------*/
SvStringsISortDtor* SvxAutoCorrectLanguageLists::LoadWrdSttExceptList()
{
	SotStorageRef xStg = new SotStorage( sShareAutoCorrFile, STREAM_READ | STREAM_SHARE_DENYNONE, TRUE );
	String sTemp ( RTL_CONSTASCII_USTRINGPARAM ( pXMLImplWrdStt_ExcptLstStr ) );
	if( xStg.Is() && xStg->IsContained( sTemp ) )
		LoadXMLExceptList_Imp( pWrdStt_ExcptLst, pXMLImplWrdStt_ExcptLstStr, xStg );
	return pWrdStt_ExcptLst;
}
/* -----------------18.11.98 11:26-------------------
 *
 * --------------------------------------------------*/
void SvxAutoCorrectLanguageLists::SaveWrdSttExceptList()
{
	MakeUserStorage_Impl();
    SotStorageRef xStg = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, TRUE );

	SaveExceptList_Imp( *pWrdStt_ExcptLst, pXMLImplWrdStt_ExcptLstStr, xStg );

	xStg = 0;
	// Zeitstempel noch setzen
	FStatHelper::GetModifiedDateTimeOfFile( sUserAutoCorrFile,
											&aModifiedDate, &aModifiedTime );
	aLastCheckTime = Time();
}
/* -----------------18.11.98 11:26-------------------
 *
 * --------------------------------------------------*/
void SvxAutoCorrectLanguageLists::SetWrdSttExceptList( SvStringsISortDtor* pList )
{
	if( pWrdStt_ExcptLst && pList != pWrdStt_ExcptLst )
		delete pWrdStt_ExcptLst;
	pWrdStt_ExcptLst = pList;
	if( !pWrdStt_ExcptLst )
	{
		DBG_ASSERT( !this, "keine gueltige Liste" );
		pWrdStt_ExcptLst = new SvStringsISortDtor( 16, 16 );
	}
	nFlags |= WrdSttLstLoad;
}
/* -----------------18.11.98 11:26-------------------
 *
 * --------------------------------------------------*/
SvStringsISortDtor* SvxAutoCorrectLanguageLists::GetWrdSttExceptList()
{
	if( !( WrdSttLstLoad & nFlags ) || IsFileChanged_Imp() )
		SetWrdSttExceptList( LoadWrdSttExceptList() );
	return pWrdStt_ExcptLst;
}
/* -----------------18.11.98 11:26-------------------
 *
 * --------------------------------------------------*/
void SvxAutoCorrectLanguageLists::RemoveStream_Imp( const String& rName )
{
	if( sShareAutoCorrFile != sUserAutoCorrFile )
	{
        SotStorageRef xStg = new SotStorage( sUserAutoCorrFile, STREAM_READWRITE, TRUE );
		if( xStg.Is() && SVSTREAM_OK == xStg->GetError() &&
			xStg->IsStream( rName ) )
		{
			xStg->Remove( rName );
			xStg->Commit();

			xStg = 0;
		}
	}
}

void SvxAutoCorrectLanguageLists::MakeUserStorage_Impl()
{
	// The conversion needs to happen if the file is already in the user
	// directory and is in the old format. Additionally it needs to
	// happen when the file is being copied from share to user.

	sal_Bool bError = sal_False, bConvert = sal_False, bCopy = sal_False;
	INetURLObject aDest;
	INetURLObject aSource;

//	String sDestPath = sUserAutoCorrFile.Copy ( 0, sUserAutoCorrFile.Len()-3);
//	sDestPath.AppendAscii ("bak");


	if (sUserAutoCorrFile != sShareAutoCorrFile )
	{
		aSource = INetURLObject ( sShareAutoCorrFile ); //aSource.setFSysPath ( sShareAutoCorrFile, INetURLObject::FSYS_DETECT );
		aDest = INetURLObject ( sUserAutoCorrFile );
		if ( SotStorage::IsOLEStorage ( sShareAutoCorrFile ) )
		{
			aDest.SetExtension ( String::CreateFromAscii ( "bak" ) );
			bConvert = sal_True;
		}
		bCopy = sal_True;
	}
	else if ( SotStorage::IsOLEStorage ( sUserAutoCorrFile ) )
	{
		aSource = INetURLObject ( sUserAutoCorrFile );
		aDest = INetURLObject ( sUserAutoCorrFile );
		aDest.SetExtension ( String::CreateFromAscii ( "bak" ) );
		bCopy = bConvert = sal_True;
	}
	if (bCopy)
	{
		try
		{
			String sMain(aDest.GetMainURL( INetURLObject::DECODE_TO_IURI ));
			sal_Unicode cSlash = '/';
			xub_StrLen nSlashPos = sMain.SearchBackward(cSlash);
			sMain.Erase(nSlashPos);
			::ucbhelper::Content aNewContent(	sMain, uno::Reference< XCommandEnvironment > ());
			Any aAny;
			TransferInfo aInfo;
			aInfo.NameClash = NameClash::OVERWRITE;
			aInfo.NewTitle  = aDest.GetName();
			aInfo.SourceURL = aSource.GetMainURL( INetURLObject::DECODE_TO_IURI );
			aInfo.MoveData  = FALSE;
			aAny <<= aInfo;
			aNewContent.executeCommand( OUString ( RTL_CONSTASCII_USTRINGPARAM( "transfer" ) ), aAny);
		}
		catch (...)
		{
			bError = sal_True;
		}
	}
	if (bConvert && !bError)
	{
        SotStorageRef xSrcStg = new SotStorage( aDest.GetMainURL( INetURLObject::DECODE_TO_IURI ), STREAM_READ, TRUE );
        SotStorageRef xDstStg = new SotStorage( sUserAutoCorrFile, STREAM_WRITE, TRUE );

		if( xSrcStg.Is() && xDstStg.Is() )
		{
			String sWord 	    ( RTL_CONSTASCII_USTRINGPARAM ( pImplWrdStt_ExcptLstStr ) );
			String sSentence    ( RTL_CONSTASCII_USTRINGPARAM ( pImplCplStt_ExcptLstStr ) );
			String sXMLWord     ( RTL_CONSTASCII_USTRINGPARAM ( pXMLImplWrdStt_ExcptLstStr ) );
			String sXMLSentence ( RTL_CONSTASCII_USTRINGPARAM ( pXMLImplCplStt_ExcptLstStr ) );
			SvStringsISortDtor 	*pTmpWordList = NULL;

            if (xSrcStg->IsContained( sXMLWord ) )
				LoadXMLExceptList_Imp( pTmpWordList, pXMLImplWrdStt_ExcptLstStr, xSrcStg );

			if (pTmpWordList)
			{
				SaveExceptList_Imp( *pTmpWordList, pXMLImplWrdStt_ExcptLstStr, xDstStg, TRUE );
		        pTmpWordList->DeleteAndDestroy( 0, pTmpWordList->Count() );
				pTmpWordList = NULL;
			}


            if (xSrcStg->IsContained( sXMLSentence ) )
				LoadXMLExceptList_Imp( pTmpWordList, pXMLImplCplStt_ExcptLstStr, xSrcStg );

			if (pTmpWordList)
            {
				SaveExceptList_Imp( *pTmpWordList, pXMLImplCplStt_ExcptLstStr, xDstStg, TRUE );
		        pTmpWordList->DeleteAndDestroy( 0, pTmpWordList->Count() );
            }

			// xDstStg is committed in MakeBlocklist_Imp
			/*xSrcStg->CopyTo( &xDstStg );*/
			sShareAutoCorrFile = sUserAutoCorrFile;
			xDstStg = 0;
			try
			{
				::ucbhelper::Content aContent ( aDest.GetMainURL( INetURLObject::DECODE_TO_IURI ), uno::Reference < XCommandEnvironment > ());
				aContent.executeCommand ( OUString ( RTL_CONSTASCII_USTRINGPARAM ( "delete" ) ), makeAny ( sal_Bool (sal_True ) ) );
			}
			catch (...)
			{
			}
		}
	}
	else if( bCopy && !bError )
		sShareAutoCorrFile = sUserAutoCorrFile;
}
