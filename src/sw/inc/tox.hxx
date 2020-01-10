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
#ifndef _TOX_HXX
#define _TOX_HXX

#include <i18npool/lang.h>
#include <tools/string.hxx>

#include <svx/svxenum.hxx>
#include <svtools/svarray.hxx>
#include <svtools/poolitem.hxx>
#include "swdllapi.h"
#include <swtypes.hxx>
#include <toxe.hxx>
#include <calbck.hxx>
#include <errhdl.hxx>

#ifndef INCLUDED_VECTOR
#include <vector> // #i21237#
#define INCLUDED_VECTOR
#endif

class SwTOXType;
class SwTOXMark;
class SwTxtTOXMark;
class SwDoc;

SV_DECL_PTRARR(SwTOXMarks, SwTOXMark*, 0, 10)

/*--------------------------------------------------------------------
     Description:  Entry of content index, alphabetical index or user defined index
 --------------------------------------------------------------------*/

#define IVER_TOXMARK_STRPOOL ((USHORT)1)
#define IVER_TOXMARK_NEWTOX ((USHORT)2)

class SW_DLLPUBLIC SwTOXMark : public SfxPoolItem, public SwClient
{
	friend void _InitCore();
	friend class SwTxtTOXMark;

	String aAltText; 	// Der Text des Eintrages ist unterschiedlich
	String aPrimaryKey, aSecondaryKey;

    // three more strings for phonetic sorting
    String aTextReading;
    String aPrimaryKeyReading;
    String aSecondaryKeyReading;

	SwTxtTOXMark* pTxtAttr;

	USHORT 	nLevel;
	BOOL	bAutoGenerated : 1;		// generated using a concordance file
	BOOL	bMainEntry : 1;			// main entry emphasized by character style


    SwTOXMark();                    // to create the dflt. atr. in _InitCore

public:
    TYPEINFO();   // rtti

	// single argument ctors shall be explicit.
	explicit SwTOXMark( const SwTOXType* pTyp );
	virtual ~SwTOXMark();

	SwTOXMark( const SwTOXMark& rCopy );
	SwTOXMark& operator=( const SwTOXMark& rCopy );

    // "pure virtual methods" of SfxPoolItem
	virtual int 			operator==( const SfxPoolItem& ) const;
	virtual SfxPoolItem*	Clone( SfxItemPool* pPool = 0 ) const;

	String					GetText() const;

	inline BOOL				IsAlternativeText() const;
	inline const String&	GetAlternativeText() const;

	inline void				SetAlternativeText( const String& rAlt );

    // content or user defined index
    inline void             SetLevel(USHORT nLevel);
	inline USHORT			GetLevel() const;

    // for alphabetical index only
	inline void				SetPrimaryKey(const String& rStr );
	inline void				SetSecondaryKey(const String& rStr);
    inline void             SetTextReading(const String& rStr);
    inline void             SetPrimaryKeyReading(const String& rStr );
    inline void             SetSecondaryKeyReading(const String& rStr);

    inline const String&    GetPrimaryKey() const;
	inline const String&	GetSecondaryKey() const;
    inline const String&    GetTextReading() const;
    inline const String&    GetPrimaryKeyReading() const;
    inline const String&    GetSecondaryKeyReading() const;

	BOOL					IsAutoGenerated() const {return bAutoGenerated;}
	void					SetAutoGenerated(BOOL bSet) {bAutoGenerated = bSet;}

	BOOL					IsMainEntry() const {return bMainEntry;}
	void					SetMainEntry(BOOL bSet) { bMainEntry = bSet;}

	inline const SwTOXType*    GetTOXType() const;

	const SwTxtTOXMark* GetTxtTOXMark() const	{ return pTxtAttr; }
		  SwTxtTOXMark* GetTxtTOXMark() 		{ return pTxtAttr; }
};

/*--------------------------------------------------------------------
     Description:  index types
 --------------------------------------------------------------------*/

class SwTOXType : public SwModify
{
public:
	SwTOXType(TOXTypes eTyp, const String& aName);

	// @@@ public copy ctor, but no copy assignment?
	SwTOXType(const SwTOXType& rCopy);

	inline	const String&	GetTypeName() const;
    inline TOXTypes         GetType() const;

private:
	String			aName;
	TOXTypes		eType;

	// @@@ public copy ctor, but no copy assignment?
	SwTOXType & operator= (const SwTOXType &);
};

/*--------------------------------------------------------------------
     Description:  Structure of the index lines
 --------------------------------------------------------------------*/

#define FORM_TITLE 				0
#define FORM_ALPHA_DELIMITTER 	1
#define FORM_PRIMARY_KEY		2
#define FORM_SECONDARY_KEY		3
#define FORM_ENTRY				4

/*
 Pattern structure

 <E#> - entry number  					<E# CharStyleName,PoolId>
 <ET> - entry text      				<ET CharStyleName,PoolId>
 <E>  - entry text and number           <E CharStyleName,PoolId>
 <T>  - tab stop                        <T,,Position,Adjust>
 <C>  - chapter info n = {0, 1, 2, 3, 4 } values of SwChapterFormat <C CharStyleName,PoolId>
 <TX> - text token						<X CharStyleName,PoolId, TOX_STYLE_DELIMITERTextContentTOX_STYLE_DELIMITER>
 <#>  - Page number                     <# CharStyleName,PoolId>
 <LS> - Link start                      <LS>
 <LE> - Link end                        <LE>
 <A00> - Authority entry field			<A02 CharStyleName, PoolId>
 */

// These enum values are stored and must not be changed!
enum FormTokenType
{
	TOKEN_ENTRY_NO,
	TOKEN_ENTRY_TEXT,
	TOKEN_ENTRY,
	TOKEN_TAB_STOP,
	TOKEN_TEXT,
	TOKEN_PAGE_NUMS,
	TOKEN_CHAPTER_INFO,
	TOKEN_LINK_START,
	TOKEN_LINK_END,
	TOKEN_AUTHORITY,
	TOKEN_END
};

struct SW_DLLPUBLIC SwFormToken
{
	String			sText;
	String			sCharStyleName;
	SwTwips			nTabStopPosition;
	FormTokenType 	eTokenType;
	USHORT			nPoolId;
    SvxTabAdjust    eTabAlign;
	USHORT			nChapterFormat;		//SwChapterFormat;
    USHORT          nOutlineLevel;//the maximum permitted outline level in numbering
	USHORT			nAuthorityField;	//enum ToxAuthorityField
	sal_Unicode 	cTabFillChar;
    sal_Bool        bWithTab;      // TRUE: do generate tab
                                   // character only the tab stop
                                   // #i21237#

	SwFormToken(FormTokenType eType ) :
        nTabStopPosition(0),
		eTokenType(eType),
		nPoolId(USHRT_MAX),
        eTabAlign( SVX_TAB_ADJUST_LEFT ),
		nChapterFormat(0 /*CF_NUMBER*/),
        nOutlineLevel(MAXLEVEL),   //default to maximum outline level
		nAuthorityField(0 /*AUTH_FIELD_IDENTIFIER*/),
		cTabFillChar(' '),
        bWithTab(sal_True)  // #i21237#
    {}

	String GetString() const;
};

// -> #i21237#
/**
    Functor that is true when a given token has a certain token type.

    @param _eType  the type to check for
    @param rToken  the token to check

    @retval TRUE   the token has the given type
    @retval FALSE  else
*/
struct SwFormTokenEqualToFormTokenType
{
    FormTokenType eType;

    SwFormTokenEqualToFormTokenType(FormTokenType _eType) : eType(_eType) {}
    bool operator()(const SwFormToken & rToken)
    {
        return rToken.eTokenType == eType;
    }
};

/**
   Functor that appends the string representation of a given token to a string.

   @param _rText    string to append the string representation to
   @param rToken    token whose string representation is appended
*/
struct SwFormTokenToString
{
    String & rText;
    SwFormTokenToString(String & _rText) : rText(_rText) {}
    void operator()(const SwFormToken & rToken) { rText += rToken.GetString(); }
};

/// Vector of tokens.
typedef std::vector<SwFormToken> SwFormTokens;

/**
   Helper class that converts vectors of tokens to strings and vice
   versa.
 */
class SW_DLLPUBLIC SwFormTokensHelper
{
    /// the tokens
    SwFormTokens aTokens;

    /**
       Builds a token from its string representation.

       @sPattern          the whole pattern
       @nCurPatternPos    starting position of the token

       @return the token
     */
    SW_DLLPRIVATE SwFormToken BuildToken( const String & sPattern,
										  xub_StrLen & nCurPatternPos ) const;

    /**
       Returns the string of a token.

       @param sPattern    the whole pattern
       @param nStt        starting position of the token

       @return   the string representation of the token
    */
    SW_DLLPRIVATE String SearchNextToken( const String & sPattern,
										  xub_StrLen nStt ) const;

    /**
       Returns the type of a token.

       @param sToken     the string representation of the token
       @param pTokenLen  return parameter the length of the head of the token

       If pTokenLen is non-NULL the length of the token's head is
       written to *pTokenLen

       @return the type of the token
    */
    SW_DLLPRIVATE FormTokenType GetTokenType(const String & sToken,
											 xub_StrLen * pTokenLen) const;

public:
    /**
       contructor

       @param rTokens       vector of tokens
    */
    SwFormTokensHelper(const SwFormTokens & rTokens) : aTokens(rTokens) {}

    /**
       constructor

       @param rStr   string representation of the tokens
    */
    SwFormTokensHelper(const String & rStr);

    /**
       Returns vector of tokens.

       @return vector of tokens
    */
    const SwFormTokens & GetTokens() const { return aTokens; }
};
// <- #i21237#

class SW_DLLPUBLIC SwForm
{
	SwFormTokens	aPattern[ AUTH_TYPE_END + 1 ]; // #i21237#
	String	aTemplate[ AUTH_TYPE_END + 1 ];

    TOXTypes    eType;
    USHORT      nFormMaxLevel;

	//USHORT	nFirstTabPos; -> Value in tab token
//	BOOL 	bHasFirstTabPos : 1;
	BOOL 	bGenerateTabPos : 1;
	BOOL 	bIsRelTabPos : 1;
	BOOL	bCommaSeparated : 1;

public:
    SwForm( TOXTypes eTOXType = TOX_CONTENT );
	SwForm( const SwForm& rForm );

	SwForm&	operator=( const SwForm& rForm );

	inline void	SetTemplate(USHORT nLevel, const String& rName);
	inline const String&	GetTemplate(USHORT nLevel) const;

    // #i21237#
	void	SetPattern(USHORT nLevel, const SwFormTokens& rName);
	void	SetPattern(USHORT nLevel, const String& rStr);
	const SwFormTokens&	GetPattern(USHORT nLevel) const;

	// fill tab stop positions from template to pattern
    // #i21237#
	void					AdjustTabStops(SwDoc& rDoc,
                                           BOOL bInsertNewTabStops = FALSE);

    inline TOXTypes GetTOXType() const;
	inline USHORT	GetFormMax() const;

    BOOL IsRelTabPos() const    {   return bIsRelTabPos; }
	void SetRelTabPos( BOOL b ) { 	bIsRelTabPos = b;		}

	BOOL IsCommaSeparated() const 		{ return bCommaSeparated;}
	void SetCommaSeparated( BOOL b)		{ bCommaSeparated = b;}

    static USHORT GetFormMaxLevel( TOXTypes eType );

	static const sal_Char*	aFormEntry;				// <E>
    static BYTE nFormEntryLen;                      // 3 characters
	static const sal_Char*	aFormTab;				// <T>
    static BYTE nFormTabLen;                        // 3 characters
	static const sal_Char*	aFormPageNums;			// <P>
    static BYTE nFormPageNumsLen;                   // 3 characters
	static const sal_Char* aFormLinkStt;			// <LS>
    static BYTE nFormLinkSttLen;                    // 4 characters
	static const sal_Char* aFormLinkEnd;			// <LE>
    static BYTE nFormLinkEndLen;                    // 4 characters
	static const sal_Char*	aFormEntryNum;			// <E#>
    static BYTE nFormEntryNumLen;                   // 4 characters
	static const sal_Char*	aFormEntryTxt;			// <ET>
    static BYTE nFormEntryTxtLen;                   // 4 characters
	static const sal_Char*	aFormChapterMark;		// <C>
    static BYTE nFormChapterMarkLen;                // 3 characters
	static const sal_Char*	aFormText;				// <TX>
    static BYTE nFormTextLen;                       // 4 characters
	static const sal_Char*	aFormAuth;				// <Axx> xx - decimal enum value
    static BYTE nFormAuthLen;                       // 3 characters
};

/*--------------------------------------------------------------------
     Description: Content to create indexes of
 --------------------------------------------------------------------*/

typedef USHORT SwTOXElement;
namespace nsSwTOXElement
{
    const SwTOXElement TOX_MARK             = 1;
    const SwTOXElement TOX_OUTLINELEVEL     = 2;
    const SwTOXElement TOX_TEMPLATE         = 4;
    const SwTOXElement TOX_OLE              = 8;
    const SwTOXElement TOX_TABLE            = 16;
    const SwTOXElement TOX_GRAPHIC          = 32;
    const SwTOXElement TOX_FRAME            = 64;
    const SwTOXElement TOX_SEQUENCE         = 128;
}

typedef USHORT SwTOIOptions;
namespace nsSwTOIOptions
{
    const SwTOIOptions TOI_SAME_ENTRY       = 1;
    const SwTOIOptions TOI_FF               = 2;
    const SwTOIOptions TOI_CASE_SENSITIVE   = 4;
    const SwTOIOptions TOI_KEY_AS_ENTRY     = 8;
    const SwTOIOptions TOI_ALPHA_DELIMITTER = 16;
    const SwTOIOptions TOI_DASH             = 32;
    const SwTOIOptions TOI_INITIAL_CAPS     = 64;
}

//which part of the caption is to be displayed
enum SwCaptionDisplay
{
	CAPTION_COMPLETE,
	CAPTION_NUMBER,
	CAPTION_TEXT
};

typedef USHORT SwTOOElements;
namespace nsSwTOOElements
{
    const SwTOOElements TOO_MATH            = 0x01;
    const SwTOOElements TOO_CHART           = 0x02;
    const SwTOOElements TOO_CALC            = 0x08;
    const SwTOOElements TOO_DRAW_IMPRESS    = 0x10;
//  const SwTOOElements TOO_IMPRESS         = 0x20;
    const SwTOOElements TOO_OTHER           = 0x80;
}

#define TOX_STYLE_DELIMITER ((sal_Unicode)0x01)		//JP 19.07.00: use a control char

/*--------------------------------------------------------------------
     Description:  Class for all indexes
 --------------------------------------------------------------------*/

class SW_DLLPUBLIC SwTOXBase : public SwClient
{
    SwForm      aForm;              // description of the lines
	String		aName; 				// unique name
    String      aTitle;             // title

	String 		sMainEntryCharStyle; // name of the character style applied to main index entries

	String		aStyleNames[MAXLEVEL]; // (additional) style names TOX_CONTENT, TOX_USER
	String 		sSequenceName;		// FieldTypeName of a caption sequence

    LanguageType    eLanguage;
    String          sSortAlgorithm;

    union {
        USHORT      nLevel;             // consider outline levels
        USHORT      nOptions;           // options of alphabetical index
	} aData;

    USHORT      nCreateType;        // sources to create the index from
	USHORT		nOLEOptions;		// OLE sources
	SwCaptionDisplay eCaptionDisplay;	//
	BOOL 		bProtected : 1;			// index protected ?
	BOOL		bFromChapter : 1; 		// create from chapter or document
	BOOL 		bFromObjectNames : 1; 	// create a table or object index
									// from the names rather than the caption
	BOOL		bLevelFromChapter : 1; // User index: get the level from the source chapter
public:
	SwTOXBase( const SwTOXType* pTyp, const SwForm& rForm,
			   USHORT nCreaType, const String& rTitle );
	SwTOXBase( const SwTOXBase& rCopy, SwDoc* pDoc = 0 );
	virtual ~SwTOXBase();

	virtual BOOL GetInfo( SfxPoolItem& rInfo ) const;

	// a kind of CopyCtor - check if the TOXBase is at TOXType of the doc.
	// If not, so create it an copy all other used things. The return is this
	SwTOXBase& CopyTOXBase( SwDoc*, const SwTOXBase& );

	const SwTOXType*	GetTOXType() const;	//

    USHORT              GetCreateType() const;      // creation types

	const String&		GetTOXName() const {return aName;}
	void				SetTOXName(const String& rSet) {aName = rSet;}

    const String&       GetTitle() const;           // Title
    const String&       GetTypeName() const;        // Name
    const SwForm&       GetTOXForm() const;         // description of the lines

	void 				SetCreate(USHORT);
	void				SetTitle(const String& rTitle);
	void				SetTOXForm(const SwForm& rForm);

	TOXTypes			GetType() const;

	const String&		GetMainEntryCharStyle() const {return sMainEntryCharStyle;}
	void				SetMainEntryCharStyle(const String& rSet)  {sMainEntryCharStyle = rSet;}

    // content index only
    inline void             SetLevel(USHORT);                   // consider outline level
	inline USHORT	  		GetLevel() const;

    // alphabetical index only
    inline USHORT           GetOptions() const;                 // alphabetical index options
	inline void   			SetOptions(USHORT nOpt);

	// index of objects
	USHORT 		GetOLEOptions() const {return nOLEOptions;}
	void   		SetOLEOptions(USHORT nOpt) {nOLEOptions = nOpt;}

	// index of objects

    // user defined index only
	inline void				SetTemplateName(const String& rName); // Absatzlayout beachten

	const String&			GetStyleNames(USHORT nLevel) const
								{
								DBG_ASSERT( nLevel < MAXLEVEL, "Which level?");
								return aStyleNames[nLevel];
								}
	void					SetStyleNames(const String& rSet, USHORT nLevel)
								{
								DBG_ASSERT( nLevel < MAXLEVEL, "Which level?");
								aStyleNames[nLevel] = rSet;
								}
	BOOL					IsFromChapter() const { return bFromChapter;}
	void					SetFromChapter(BOOL bSet) { bFromChapter = bSet;}

	BOOL					IsFromObjectNames() const {return bFromObjectNames;}
	void					SetFromObjectNames(BOOL bSet) {bFromObjectNames = bSet;}

	BOOL					IsLevelFromChapter() const {return bLevelFromChapter;}
	void					SetLevelFromChapter(BOOL bSet) {bLevelFromChapter = bSet;}

	BOOL					IsProtected() const { return bProtected; }
	void					SetProtected(BOOL bSet) { bProtected = bSet; }

	const String&			GetSequenceName() const {return sSequenceName;}
	void					SetSequenceName(const String& rSet) {sSequenceName = rSet;}

	SwCaptionDisplay		GetCaptionDisplay() const { return eCaptionDisplay;}
	void					SetCaptionDisplay(SwCaptionDisplay eSet) {eCaptionDisplay = eSet;}

	BOOL 					IsTOXBaseInReadonly() const;

	const SfxItemSet*		GetAttrSet() const;
	void 					SetAttrSet( const SfxItemSet& );

    LanguageType    GetLanguage() const {return eLanguage;}
    void            SetLanguage(LanguageType nLang)  {eLanguage = nLang;}

    const String&   GetSortAlgorithm()const {return sSortAlgorithm;}
    void            SetSortAlgorithm(const String& rSet) {sSortAlgorithm = rSet;}
    // #i21237#
    void AdjustTabStops(SwDoc & rDoc, BOOL bDefaultRightTabStop);
    SwTOXBase& 			operator=(const SwTOXBase& rSource);
};


/*--------------------------------------------------------------------
     Description:  Inlines
 --------------------------------------------------------------------*/

//
//SwTOXMark
//
inline const String& SwTOXMark::GetAlternativeText() const
	{	return aAltText;	}

inline const SwTOXType* SwTOXMark::GetTOXType() const
	{ return (SwTOXType*)GetRegisteredIn(); }

inline BOOL SwTOXMark::IsAlternativeText() const
	{ return aAltText.Len() > 0; }

inline void SwTOXMark::SetAlternativeText(const String& rAlt)
{
	aAltText = rAlt;
}

inline void SwTOXMark::SetLevel( USHORT nLvl )
{
	ASSERT( !GetTOXType() || GetTOXType()->GetType() != TOX_INDEX, "Falscher Feldtyp");
	nLevel = nLvl;
}

inline void SwTOXMark::SetPrimaryKey( const String& rKey )
{
	ASSERT(GetTOXType()->GetType() == TOX_INDEX, "Falscher Feldtyp");
	aPrimaryKey = rKey;
}

inline void SwTOXMark::SetSecondaryKey( const String& rKey )
{
	ASSERT(GetTOXType()->GetType() == TOX_INDEX, "Falscher Feldtyp");
	aSecondaryKey = rKey;
}

inline void SwTOXMark::SetTextReading( const String& rTxt )
{
	ASSERT(GetTOXType()->GetType() == TOX_INDEX, "Falscher Feldtyp");
    aTextReading = rTxt;
}

inline void SwTOXMark::SetPrimaryKeyReading( const String& rKey )
{
	ASSERT(GetTOXType()->GetType() == TOX_INDEX, "Falscher Feldtyp");
    aPrimaryKeyReading = rKey;
}

inline void SwTOXMark::SetSecondaryKeyReading( const String& rKey )
{
	ASSERT(GetTOXType()->GetType() == TOX_INDEX, "Falscher Feldtyp");
    aSecondaryKeyReading = rKey;
}

inline USHORT SwTOXMark::GetLevel() const
{
	ASSERT( !GetTOXType() || GetTOXType()->GetType() != TOX_INDEX, "Falscher Feldtyp");
	return nLevel;
}

inline const String& SwTOXMark::GetPrimaryKey() const
{
	ASSERT(GetTOXType()->GetType() == TOX_INDEX, "Falscher Feldtyp");
	return aPrimaryKey;
}

inline const String& SwTOXMark::GetSecondaryKey() const
{
	ASSERT(GetTOXType()->GetType() == TOX_INDEX, "Falscher Feldtyp");
	return aSecondaryKey;
}

inline const String& SwTOXMark::GetTextReading() const
{
	ASSERT(GetTOXType()->GetType() == TOX_INDEX, "Falscher Feldtyp");
    return aTextReading;
}

inline const String& SwTOXMark::GetPrimaryKeyReading() const
{
	ASSERT(GetTOXType()->GetType() == TOX_INDEX, "Falscher Feldtyp");
    return aPrimaryKeyReading;
}

inline const String& SwTOXMark::GetSecondaryKeyReading() const
{
	ASSERT(GetTOXType()->GetType() == TOX_INDEX, "Falscher Feldtyp");
    return aSecondaryKeyReading;
}

//
//SwForm
//
inline void SwForm::SetTemplate(USHORT nLevel, const String& rTemplate)
{
	ASSERT(nLevel < GetFormMax(), "Index >= FORM_MAX");
	aTemplate[nLevel] = rTemplate;
}

inline const String& SwForm::GetTemplate(USHORT nLevel) const
{
	ASSERT(nLevel < GetFormMax(), "Index >= FORM_MAX");
	return aTemplate[nLevel];
}

inline TOXTypes SwForm::GetTOXType() const
{
    return eType;
}

inline USHORT SwForm::GetFormMax() const
{
	return nFormMaxLevel;
}


//
//SwTOXType
//
inline const String& SwTOXType::GetTypeName() const
	{	return aName;	}

inline TOXTypes SwTOXType::GetType() const
	{	return eType;	}

//
// SwTOXBase
//
inline const SwTOXType* SwTOXBase::GetTOXType() const
	{ return (SwTOXType*)GetRegisteredIn(); }

inline USHORT SwTOXBase::GetCreateType() const
	{ return nCreateType; }

inline const String& SwTOXBase::GetTitle() const
	{ return aTitle; }

inline const String& SwTOXBase::GetTypeName() const
	{ return GetTOXType()->GetTypeName();  }

inline const SwForm& SwTOXBase::GetTOXForm() const
	{ return aForm;	}

inline void SwTOXBase::AdjustTabStops(SwDoc & rDoc, BOOL bDefaultRightTabStop)
{
    aForm.AdjustTabStops(rDoc, bDefaultRightTabStop);
}

inline void SwTOXBase::SetCreate(USHORT nCreate)
	{ nCreateType = nCreate; }

inline void	SwTOXBase::SetTOXForm(const SwForm& rForm)
	{  aForm = rForm; }

inline TOXTypes SwTOXBase::GetType() const
	{ return GetTOXType()->GetType(); }

inline void SwTOXBase::SetLevel(USHORT nLev)
{
	ASSERT(GetTOXType()->GetType() != TOX_INDEX, "Falscher Feldtyp");
	aData.nLevel = nLev;
}

inline USHORT SwTOXBase::GetLevel() const
{
	ASSERT(GetTOXType()->GetType() != TOX_INDEX, "Falscher Feldtyp");
	return aData.nLevel;
}

inline void SwTOXBase::SetTemplateName(const String& rName)
{
//	ASSERT(GetTOXType()->GetType() == TOX_USER, "Falscher Feldtyp");
//	ASSERT(aData.pTemplateName, "pTemplateName == 0");
//	(*aData.pTemplateName) = rName;
	DBG_WARNING("SwTOXBase::SetTemplateName obsolete");
	aStyleNames[0] = rName;

}

inline USHORT SwTOXBase::GetOptions() const
{
	ASSERT(GetTOXType()->GetType() == TOX_INDEX, "Falscher Feldtyp");
	return aData.nOptions;
}

inline void SwTOXBase::SetOptions(USHORT nOpt)
{
	ASSERT(GetTOXType()->GetType() == TOX_INDEX, "Falscher Feldtyp");
	aData.nOptions = nOpt;
}


#endif	// _TOX_HXX
