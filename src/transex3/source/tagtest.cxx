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
#include "precompiled_transex3.hxx"
#include <tools/string.hxx>
#include "tagtest.hxx"

#if OSL_DEBUG_LEVEL > 1
#include <stdio.h>
#endif

#include "gsicheck.hxx"

#define HAS_FLAG( nFlags, nFlag )		( ( nFlags & nFlag ) != 0 )
#define SET_FLAG( nFlags, nFlag )		( nFlags |= nFlag )
#define RESET_FLAG( nFlags, nFlag )		( nFlags &= ~nFlag )	// ~ = Bitweises NOT



TokenInfo::TokenInfo( TokenId pnId, USHORT nP, String paStr, ParserMessageList &rErrorList )
: bClosed(FALSE)
, bCloseTag(FALSE)
, bIsBroken(FALSE)
, bHasBeenFixed(FALSE)
, bDone(FALSE)
, aTokenString( paStr )
, nId( pnId )
, nPos(nP)
{
    if ( nId == TAG_COMMONSTART || nId == TAG_COMMONEND )
        SplitTag( rErrorList );
}

enum tagcheck { TC_START, TC_HAS_TAG_NAME, TC_HAS_PROP_NAME_EQ, TC_HAS_PROP_NAME_EQ_SP, TC_HAS_PROP_NAME_SP, TC_INSIDE_STRING, TC_PROP_FINISHED, TC_CLOSED, TC_CLOSED_SPACE, TC_CLOSETAG, TC_CLOSETAG_HAS_TAG_NAME, TC_FINISHED, TC_ERROR };

/*
                                                      \<  link  href  =  \"text\"  name  =  \"C\"  \>
START               ' ' ->  HAS_TAG_NAME
START               '/' ->  CLOSED
START               '/' ->  CLOSETAG    - no Portion (starting with /)
START               '>' ->  FINISHED
HAS_TAG_NAME        '=' ->  HAS_PROP_NAME_EQ
HAS_TAG_NAME        ' ' ->  HAS_PROP_NAME_SP
HAS_TAG_NAME        '/' ->  CLOSED
HAS_TAG_NAME        '>' ->  FINISHED
HAS_PROP_NAME_SP    '=' ->  HAS_PROP_NAME_EQ
HAS_PROP_NAME_EQ    ' ' ->  HAS_PROP_NAME_EQ_SP
HAS_PROP_NAME_EQ    '"' ->  INSIDE_STRING
HAS_PROP_NAME_EQ_SP '"' ->  INSIDE_STRING
INSIDE_STRING       ' ' ->  INSIDE_STRING
INSIDE_STRING       '=' ->  INSIDE_STRING
INSIDE_STRING       '>' ->  INSIDE_STRING
INSIDE_STRING       '"' ->  PROP_FINISHED
PROP_FINISHED       ' ' ->  HAS_TAG_NAME
PROP_FINISHED       '/' ->  CLOSED
PROP_FINISHED       '>' ->  FINISHED
CLOSED              ' ' ->  CLOSED_SPACE
CLOSED              '>' ->  FINISHED
CLOSED_SPACE        '>' ->  FINISHED

CLOSETAG            ' ' ->  CLOSETAG_HAS_TAG_NAME
CLOSETAG            '>' ->  FINISHED
CLOSETAG_HAS_TAG_NAME  '>' ->  FINISHED

*/
void TokenInfo::SplitTag( ParserMessageList &rErrorList )
{
    USHORT nLastPos = 2;    // skip initial  \<
    USHORT nCheckPos = nLastPos;
    String aDelims( String::CreateFromAscii( " \\=>/" ) );
    String aPortion;
    String aValue;      // store the value of a property
    ByteString aName;   // store the name of a property/tag 
    BOOL bCheckName = FALSE;
    BOOL bCheckEmpty = FALSE;
    sal_Unicode cDelim;
    tagcheck aState = TC_START;

    // skip blanks
    while ( nLastPos < aTokenString.Len() && aTokenString.GetChar( nLastPos ) == ' ')
        nLastPos++;

    nCheckPos = aTokenString.SearchChar( aDelims.GetBuffer(), nLastPos );
    while ( nCheckPos != STRING_NOTFOUND && !( aState == TC_FINISHED || aState == TC_ERROR ) )
    {
        aPortion = aTokenString.Copy( nLastPos, nCheckPos-nLastPos );

        if ( aTokenString.GetChar( nCheckPos ) == '\\' )
            nCheckPos++;         

        cDelim = aTokenString.GetChar( nCheckPos );
        nCheckPos++;

        switch ( aState )
        {
//            START           ' ' ->  HAS_TAG_NAME
//            START           '/' ->  CLOSED
//            START           '>' ->  FINISHED
            case TC_START:
                aTagName = aPortion;
                switch ( cDelim )
                {
                    case ' ':  aState = TC_HAS_TAG_NAME;
                               bCheckName = TRUE;
                               break;
                    case '/':  
                        {
                            if ( aPortion.Len() == 0 )
                            {
                                aState = TC_CLOSETAG;
                            }
                            else
                            {
                                aState = TC_CLOSED;
                                bCheckName = TRUE;
                            }
                        }
                        break;
                    case '>':  aState = TC_FINISHED;
                               bCheckName = TRUE;
                               break;
                    default:   aState = TC_ERROR;
                }
                break;

//            HAS_TAG_NAME    '=' ->  HAS_PROP_NAME_EQ
//            HAS_TAG_NAME    ' ' ->  HAS_PROP_NAME_SP
//            HAS_TAG_NAME    '/' ->  CLOSED
//            HAS_TAG_NAME    '>' ->  FINISHED
            case TC_HAS_TAG_NAME:
                switch ( cDelim )
                {
                    case '=':  aState = TC_HAS_PROP_NAME_EQ;
                               bCheckName = TRUE;
                               break;
                    case ' ':  aState = TC_HAS_PROP_NAME_SP;
                               bCheckName = TRUE;
                               break;
                    case '/':  aState = TC_CLOSED;
                               bCheckEmpty = TRUE;
                               break;
                    case '>':  aState = TC_FINISHED;
                               bCheckEmpty = TRUE;
                               break;
                    default:   aState = TC_ERROR;
                }
                break;

//            HAS_PROP_NAME_SP    '=' ->  HAS_PROP_NAME_EQ
            case TC_HAS_PROP_NAME_SP:
                switch ( cDelim )
                {
                    case '=':  aState = TC_HAS_PROP_NAME_EQ;
                               bCheckEmpty = TRUE;
                               break;
                    default:   aState = TC_ERROR;
                }
                break;

//            HAS_PROP_NAME_EQ    ' ' ->  HAS_PROP_NAME_EQ_SP
//            HAS_PROP_NAME_EQ    '"' ->  INSIDE_STRING
            case TC_HAS_PROP_NAME_EQ:
                switch ( cDelim )
                {
                    case ' ':  aState = TC_HAS_PROP_NAME_EQ_SP;
                               bCheckEmpty = TRUE;
                               break;
                    case '\"': aState = TC_INSIDE_STRING;
                               bCheckEmpty = TRUE;
                               aValue.Erase();
                               break;
                    default:   aState = TC_ERROR;
                }
                break;

//            HAS_PROP_NAME_EQ_SP '"' ->  INSIDE_STRING
            case TC_HAS_PROP_NAME_EQ_SP:
                switch ( cDelim )
                {
                    case '\"': aState = TC_INSIDE_STRING;
                               bCheckEmpty = TRUE;
                               aValue.Erase();
                               break;
                    default:   aState = TC_ERROR;
                }
                break;

//            INSIDE_STRING    *  ->  INSIDE_STRING
//            INSIDE_STRING   '"' ->  PROP_FINISHED
            case TC_INSIDE_STRING:
                switch ( cDelim )
                {
                    case '\"': 
                        {
                            aState = TC_PROP_FINISHED;
                            aValue += aPortion;
                            if ( aProperties.find( aName ) == aProperties.end() )
                            {
                                if ( !IsPropertyValueValid( aName, aValue ) )
                                {
                                    rErrorList.AddError( 25, ByteString("Property '").Append(aName).Append("' has invalid value '").Append(ByteString( aValue, RTL_TEXTENCODING_UTF8 )).Append("' "), *this );
                                    bIsBroken = TRUE;
                                }
                                aProperties[ aName ] = aValue;
                            }
                            else
                            {
                                rErrorList.AddError( 25, ByteString("Property '").Append(aName).Append("' defined twice "), *this );
                                bIsBroken = TRUE;
                            }
                        }
                               break;
                    default:   
                        {
                            aState = TC_INSIDE_STRING;
                            aValue += aPortion;
                            aValue += cDelim;
                        }
                }
                break;

//            PROP_FINISHED   ' ' ->  HAS_TAG_NAME
//            PROP_FINISHED   '/' ->  CLOSED
//            PROP_FINISHED   '>' ->  FINISHED
            case TC_PROP_FINISHED:
                switch ( cDelim )
                {
                    case ' ': aState = TC_HAS_TAG_NAME;
                               bCheckEmpty = TRUE;
                               break;
                    case '/': aState = TC_CLOSED;
                               bCheckEmpty = TRUE;
                               break;
                    case '>': aState = TC_FINISHED;
                               bCheckEmpty = TRUE;
                               break;
                    default:   aState = TC_ERROR;
                }
                break;

//            CLOSED          ' ' ->  CLOSED_SPACE
//            CLOSED          '>' ->  FINISHED
            case TC_CLOSED:
                switch ( cDelim )
                {
                    case ' ': aState = TC_CLOSED_SPACE;
                               bCheckEmpty = TRUE;
                               bClosed = TRUE;
                               break;
                    case '>': aState = TC_FINISHED;
                               bCheckEmpty = TRUE;
                               break;
                    default:   aState = TC_ERROR;
                }
                break;

//            CLOSED_SPACE    '>' ->  FINISHED
            case TC_CLOSED_SPACE:
                switch ( cDelim )
                {
                    case '>': aState = TC_FINISHED;
                               bCheckEmpty = TRUE;
                               break;
                    default:   aState = TC_ERROR;
                }
                break;

// CLOSETAG            ' ' ->  CLOSETAG_HAS_TAG_NAME
// CLOSETAG            '>' ->  FINISHED
            case TC_CLOSETAG:
                bCloseTag = TRUE;
                switch ( cDelim )
                {
                    case ' ': aState = TC_CLOSETAG_HAS_TAG_NAME;
                               aTagName = aPortion;
                               bCheckName = TRUE;
                               break;
                    case '>': aState = TC_FINISHED;
                               aTagName = aPortion;
                               bCheckName = TRUE;
                               break;
                    default:   aState = TC_ERROR;
                }
                break;

// CLOSETAG_HAS_TAG_NAME       '>' ->  FINISHED
            case TC_CLOSETAG_HAS_TAG_NAME:
                switch ( cDelim )
                {
                    case '>': aState = TC_FINISHED;
                               bCheckEmpty = TRUE;
                               break;
                    default:   aState = TC_ERROR;
                }
                break;


            default: rErrorList.AddError( 99, "Internal error Parsing Tag ", *this );
                     bIsBroken = TRUE;

        }

        if ( bCheckName )
        {
            if ( aPortion.Len() == 0 )
            {
                rErrorList.AddError( 25, "Tag/Property name missing ", *this );
                bIsBroken = TRUE;
            }
            else
            {
                aName = ByteString( aPortion, RTL_TEXTENCODING_UTF8 );
                // "a-zA-Z_-.0-9" 
                xub_StrLen nCount;
                BOOL bBroken = FALSE;
                const sal_Char* aBuf = aName.GetBuffer();
                for ( nCount = 0 ; !bBroken && nCount < aName.Len() ; nCount++ )
                {
                    bBroken = ! (   ( aBuf[nCount] >= 'a' && aBuf[nCount] <= 'z' )
                                ||( aBuf[nCount] >= 'A' && aBuf[nCount] <= 'Z' )
                                ||( aBuf[nCount] >= '0' && aBuf[nCount] <= '9' )
                                ||( aBuf[nCount] == '_' )
                                ||( aBuf[nCount] == '-' )
                                ||( aBuf[nCount] == '.' )
                                );
                }

                if ( bBroken )
                {
                    rErrorList.AddError( 25, "Found illegal character in Tag/Property name ", *this );
                    bIsBroken = TRUE;
                }
            }

            bCheckName = FALSE;
        }

        if ( bCheckEmpty )
        {
            if ( aPortion.Len() )
            {
                rErrorList.AddError( 25, ByteString("Found displaced characters '").Append(ByteString( aPortion, RTL_TEXTENCODING_UTF8 )).Append("' in Tag "), *this );
                bIsBroken = TRUE;
            }
            bCheckEmpty = FALSE;
        }


        nLastPos = nCheckPos;

        // skip further blanks
        if ( cDelim == ' ' && aState != TC_INSIDE_STRING )
            while ( nLastPos < aTokenString.Len() && aTokenString.GetChar( nLastPos ) == ' ')
                nLastPos++;

        nCheckPos = aTokenString.SearchChar( aDelims.GetBuffer(), nLastPos );
    }
    if ( aState != TC_FINISHED )
    {
        rErrorList.AddError( 25, "Parsing error in Tag ", *this );
        bIsBroken = TRUE;
    }
}

BOOL TokenInfo::IsPropertyRelevant( const ByteString &aName, const String &aValue ) const
{
    if ( aTagName.EqualsAscii( "alt" ) && aName.Equals( "xml-lang" ) )
        return FALSE;
    if ( aTagName.EqualsAscii( "ahelp" ) && aName.Equals( "visibility" ) && aValue.EqualsAscii("visible") )
        return FALSE;
    if ( aTagName.EqualsAscii( "image" ) && (aName.Equals( "width" ) || aName.Equals( "height" )) )
        return FALSE;
    
    return TRUE;
}

BOOL TokenInfo::IsPropertyValueValid( const ByteString &aName, const String &aValue ) const
{
/*  removed due to i56740
    if ( aTagName.EqualsAscii( "switchinline" ) && aName.Equals( "select" ) )
    {
        return aValue.EqualsAscii("sys") ||
               aValue.EqualsAscii("appl") ||
               aValue.EqualsAscii("distrib");
    } */
    if ( aTagName.EqualsAscii( "caseinline" ) && aName.Equals( "select" ) )
    {
        return /*!aValue.EqualsAscii("OS2") &&  removed due to i56740 */
               !aValue.EqualsAscii("");
    }

    // we don't know any better so we assume it to be OK
    return TRUE;
}

BOOL TokenInfo::IsPropertyInvariant( const ByteString &aName, const String &aValue ) const
{
    if ( aTagName.EqualsAscii( "link" ) && aName.Equals( "name" ) )
        return FALSE;
    if ( aTagName.EqualsAscii( "link" ) && aName.Equals( "href" ) )
    {   // check for external reference
        if (  aValue.Copy( 0, 5 ).EqualsIgnoreCaseAscii( "http:" )
           || aValue.Copy( 0, 6 ).EqualsIgnoreCaseAscii( "https:" )
           || aValue.Copy( 0, 4 ).EqualsIgnoreCaseAscii( "ftp:" ) )
            return FALSE;
        else
            return TRUE;
    }
    return TRUE;
}

BOOL TokenInfo::IsPropertyFixable( const ByteString &aName ) const
{
    // name everything that is allowed to be fixed automatically here
    if ( (aTagName.EqualsAscii( "ahelp" ) && aName.Equals( "hid" )) 
      || (aTagName.EqualsAscii( "link" ) && aName.Equals( "href" )) 
      || (aTagName.EqualsAscii( "alt" ) && aName.Equals( "id" )) 
      || (aTagName.EqualsAscii( "variable" ) && aName.Equals( "id" )) 
      || (aTagName.EqualsAscii( "image" ) && aName.Equals( "src" ))
      || (aTagName.EqualsAscii( "image" ) && aName.Equals( "id" ) ))
        return TRUE;
    return FALSE;
}

BOOL TokenInfo::MatchesTranslation( TokenInfo& rInfo, BOOL bGenErrors, ParserMessageList &rErrorList, BOOL bFixTags ) const
{
    // check if tags are equal
    // check if all existing properties are in the translation as well and
    // wether they have a matching content (the same in most cases)

    if ( nId != rInfo.nId )
        return FALSE;

    if ( !aTagName.Equals( rInfo.aTagName ) )
        return FALSE;

    // If one of the tags has formating errors already it does make no sense to check here, so return right away
    if ( bGenErrors && ( bIsBroken || rInfo.bIsBroken ) )
        return TRUE;

	StringHashMap::const_iterator iProp;
	for( iProp = aProperties.begin() ; iProp != aProperties.end(); ++iProp )
    {
        if ( rInfo.aProperties.find( iProp->first ) != rInfo.aProperties.end() )
        {
            if ( IsPropertyRelevant( iProp->first, iProp->second ) || IsPropertyRelevant( iProp->first, rInfo.aProperties.find( iProp->first )->second ) )
            {
                if ( IsPropertyInvariant( iProp->first, iProp->second ) )
                {
                    if ( !rInfo.aProperties.find( iProp->first )->second.Equals( iProp->second ) )
                    {
                        if ( bGenErrors )
                        {
                            if ( bFixTags && IsPropertyFixable( iProp->first ) )
                            {
                                rInfo.aProperties.find( iProp->first )->second = iProp->second;
                                rInfo.SetHasBeenFixed();
                                rErrorList.AddWarning( 25, ByteString("Property '").Append(iProp->first).Append("': FIXED different value in Translation "), *this );
                            }
                            else
                                rErrorList.AddError( 25, ByteString("Property '").Append(iProp->first).Append("': value different in Translation "), *this );
                        }
                        else return FALSE;
                    }
                }
            }
        }
        else
        {
            if ( IsPropertyRelevant( iProp->first, iProp->second ) )
            {
                if ( bGenErrors )
                    rErrorList.AddError( 25, ByteString("Property '").Append(iProp->first).Append("' missing in Translation "), *this );
                else return FALSE;
            }
        }
	}
	for( iProp = rInfo.aProperties.begin() ; iProp != rInfo.aProperties.end(); ++iProp )
    {
        if ( aProperties.find( iProp->first ) == aProperties.end() )
        {
            if ( IsPropertyRelevant( iProp->first, iProp->second ) )
            {
                if ( bGenErrors )
                    rErrorList.AddError( 25, ByteString("Extra Property '").Append(iProp->first).Append("' in Translation "), rInfo );
                else return FALSE;
            }
        }
	}

    // if we reach here eather 
    //   the tags match completely or
    //   the tags match but not the properties and we generated errors for that
    return TRUE;
}

String TokenInfo::GetTagName() const
{
    return aTagName;
}

String TokenInfo::MakeTag() const
{
    String aRet;
    aRet.AppendAscii("\\<");
    if ( bCloseTag )
        aRet.AppendAscii("/");
    aRet.Append( GetTagName() );
	StringHashMap::const_iterator iProp;

	for( iProp = aProperties.begin() ; iProp != aProperties.end(); ++iProp )
    {
        aRet.AppendAscii(" ");
        aRet.Append( String( iProp->first, RTL_TEXTENCODING_UTF8 ) );
        aRet.AppendAscii("=\\\"");
        aRet.Append( iProp->second );
        aRet.AppendAscii("\\\"");
    }
    if ( bClosed )
        aRet.AppendAscii("/");
    aRet.AppendAscii("\\>");
    return aRet;
}


void ParserMessageList::AddError( USHORT nErrorNr, ByteString aErrorText, const TokenInfo &rTag )
{
    Insert( new ParserError( nErrorNr, aErrorText, rTag ), LIST_APPEND );
}

void ParserMessageList::AddWarning( USHORT nErrorNr, ByteString aErrorText, const TokenInfo &rTag )
{
    Insert( new ParserWarning( nErrorNr, aErrorText, rTag ), LIST_APPEND );
}

BOOL ParserMessageList::HasErrors()
{
    USHORT i;
    for ( i=0 ; i < Count() ; i++ )
        if ( GetObject( i )->IsError() )
            return TRUE;
    return FALSE;
}

struct Tag
{
    String GetName() const { return String::CreateFromAscii( pName ); };
	const char* pName;
	TokenId nTag;
};


static const Tag aKnownTags[] =
{
/*  commenting oldstyle tags
//	{ "<#GROUP_FORMAT>", TAG_GROUP_FORMAT },
	{ "<#BOLD>", TAG_BOLDON },
	{ "<#/BOLD>", TAG_BOLDOFF },
	{ "<#ITALIC>", TAG_ITALICON },
	{ "<#/ITALIC>", TAG_ITALICOFF },
	{ "<#UNDER>", TAG_UNDERLINEON },
	{ "<#/UNDER>", TAG_UNDERLINEOFF },

//	{ "<#GROUP_NOTALLOWED>", TAG_GROUP_NOTALLOWED },
	{ "<#HELPID>", TAG_HELPID },
	{ "<#MODIFY>", TAG_MODIFY },
	{ "<#REFNR>", TAG_REFNR },

//	{ "<#GROUP_STRUCTURE>", TAG_GROUP_STRUCTURE },
	{ "<#NAME>", TAG_NAME },
	{ "<#HREF>", TAG_HREF },
	{ "<#AVIS>", TAG_AVIS },
	{ "<#AHID>", TAG_AHID },
	{ "<#AEND>", TAG_AEND },

	{ "<#TITEL>", TAG_TITEL },
	{ "<#KEY>", TAG_KEY },
	{ "<#INDEX>", TAG_INDEX },

	{ "<#REFSTART>", TAG_REFSTART },

	{ "<#GRAPHIC>", TAG_GRAPHIC },
	{ "<#NEXTVERSION>", TAG_NEXTVERSION },

    //	{ "<#GROUP_SYSSWITCH>", TAG_GROUP_SYSSWITCH },
	{ "<#WIN>", TAG_WIN },
	{ "<#UNIX>", TAG_UNIX },
	{ "<#MAC>", TAG_MAC },
	{ "<#OS2>", TAG_OS2 },

//	{ "<#GROUP_PROGSWITCH>", TAG_GROUP_PROGSWITCH },
	{ "<#WRITER>", TAG_WRITER },
	{ "<#CALC>", TAG_CALC },
	{ "<#DRAW>", TAG_DRAW },
	{ "<#IMPRESS>", TAG_IMPRESS },
	{ "<#SCHEDULE>", TAG_SCHEDULE },
	{ "<#IMAGE>", TAG_IMAGE },
	{ "<#MATH>", TAG_MATH },
	{ "<#CHART>", TAG_CHART },
	{ "<#OFFICE>", TAG_OFFICE },
  */
//	{ "<#TAG_GROUP_META>", TAG_GROUP_META },
	{ "$[officefullname]", TAG_OFFICEFULLNAME },
	{ "$[officename]", TAG_OFFICENAME },
	{ "$[officepath]", TAG_OFFICEPATH },
	{ "$[officeversion]", TAG_OFFICEVERSION },
	{ "$[portalname]", TAG_PORTALNAME },
	{ "$[portalfullname]", TAG_PORTALFULLNAME },
	{ "$[portalpath]", TAG_PORTALPATH },
	{ "$[portalversion]", TAG_PORTALVERSION },
	{ "$[portalshortname]", TAG_PORTALSHORTNAME },
/*  commenting oldstyle tags
//	{ "<#TAG_GROUP_SINGLE>", TAG_GROUP_SINGLE },
	{ "<#REFINSERT>", TAG_REFINSERT },

//	{ "<#GROUP_MULTI>", TAG_GROUP_MULTI },
	{ "<#END>", TAG_END },
	{ "<#ELSE>", TAG_ELSE },
	{ "<#VERSIONEND>", TAG_VERSIONEND },
	{ "<#ENDGRAPHIC>", TAG_ENDGRAPHIC },*/
	{ "<Common Tag>", TAG_COMMONSTART },
	{ "</Common Tag>", TAG_COMMONEND },

    { "<no more tags>", TAG_NOMORETAGS },
	{ "", TAG_UNKNOWN_TAG },
};


SimpleParser::SimpleParser()
: nPos( 0 )
, aNextTag( TAG_NOMORETAGS, TOK_INVALIDPOS )
{
}

void SimpleParser::Parse( String PaSource )
{
    aSource = PaSource;
	nPos = 0;
	aLastToken.Erase();
    aNextTag = TokenInfo( TAG_NOMORETAGS, TOK_INVALIDPOS );
	aTokenList.Clear();
};

TokenInfo SimpleParser::GetNextToken( ParserMessageList &rErrorList )
{
    TokenInfo aResult;
    USHORT nTokenStartPos = 0;
    if ( aNextTag.nId != TAG_NOMORETAGS )
    {
        aResult = aNextTag;
        aNextTag = TokenInfo( TAG_NOMORETAGS, TOK_INVALIDPOS );
    }
    else
    {
	    aLastToken = GetNextTokenString( rErrorList, nTokenStartPos );
	    if ( aLastToken.Len() == 0 )
		    return TokenInfo( TAG_NOMORETAGS, TOK_INVALIDPOS );

        // do we have a \< ... \> style tag?
        if ( aLastToken.Copy(0,2).EqualsAscii( "\\<" ) )
        {
            // check for paired \" \"
            bool bEven = true;
            USHORT nQuotePos = 0;
    	    USHORT nQuotedQuotesPos = aLastToken.SearchAscii( "\\\"" );
    	    USHORT nQuotedBackPos = aLastToken.SearchAscii( "\\\\" );    // this is only to kick out quoted backslashes
            while ( nQuotedQuotesPos != STRING_NOTFOUND )
            {
                if ( nQuotedBackPos <= nQuotedQuotesPos )
                    nQuotePos = nQuotedBackPos+2;
                else
                {
                    nQuotePos = nQuotedQuotesPos+2;
                    bEven = !bEven;
                }
		        nQuotedQuotesPos = aLastToken.SearchAscii( "\\\"", nQuotePos );
    	        nQuotedBackPos = aLastToken.SearchAscii( "\\\\", nQuotePos );    // this is only to kick out quoted backslashes
            }
            if ( !bEven )
            {
        		rErrorList.AddError( 24, "Missing quotes ( \\\" ) in Tag", TokenInfo( TAG_UNKNOWN_TAG, nTokenStartPos, aLastToken ) );
            }

            // check if we have an end-tag or a start-tag
            USHORT nNonBlankStartPos,nNonBlankEndPos;
            nNonBlankStartPos = 2;
            while ( aLastToken.GetChar(nNonBlankStartPos) == ' ' )
                nNonBlankStartPos++;
            if ( aLastToken.GetChar(nNonBlankStartPos) == '/' )
                aResult = TokenInfo( TAG_COMMONEND, nTokenStartPos, aLastToken, rErrorList );
            else
            {
                aResult = TokenInfo( TAG_COMMONSTART, nTokenStartPos, aLastToken, rErrorList );
                nNonBlankEndPos = aLastToken.Len() -3;
                while ( aLastToken.GetChar(nNonBlankEndPos) == ' ' )
                    nNonBlankEndPos--;
                if ( aLastToken.GetChar( nNonBlankEndPos ) == '/' )
                    aNextTag = TokenInfo( TAG_COMMONEND, nTokenStartPos, String::CreateFromAscii("\\</").Append(aResult.GetTagName()).AppendAscii("\\>"), rErrorList );
            }
        }
        else
        {
	        USHORT i = 0;
	        while ( aKnownTags[i].nTag != TAG_UNKNOWN_TAG &&
		        aLastToken != aKnownTags[i].GetName() )
		        i++;
            aResult = TokenInfo( aKnownTags[i].nTag, nTokenStartPos );
        }
    }

    if ( aResult.nId == TAG_UNKNOWN_TAG )
        aResult = TokenInfo( TAG_UNKNOWN_TAG, nTokenStartPos, aLastToken );
	aTokenList.Insert( aResult, LIST_APPEND );
	return aResult;
}

String SimpleParser::GetNextTokenString( ParserMessageList &rErrorList, USHORT &rTagStartPos )
{
//	USHORT nStyle1StartPos = aSource.SearchAscii( "<#", nPos );
	USHORT nStyle2StartPos = aSource.SearchAscii( "$[", nPos );
	USHORT nStyle3StartPos = aSource.SearchAscii( "\\<", nPos );
	USHORT nStyle4StartPos = aSource.SearchAscii( "\\\\", nPos );    // this is only to kick out quoted backslashes

    rTagStartPos = 0;

/* removing since a \<... is not likely
    // check if the tag starts with a letter to avoid things like <> <= ... >
    while ( STRING_NOTFOUND != nStyle3StartPos && !( aSource.Copy( nStyle3StartPos+2, 1 ).IsAlphaAscii() || aSource.GetChar( nStyle3StartPos+2 ) == '/' ) )
    	nStyle3StartPos = aSource.SearchAscii( "\\<", nStyle3StartPos+1 );
*/
    if ( STRING_NOTFOUND == nStyle2StartPos && STRING_NOTFOUND == nStyle3StartPos )
        return String();  // no more tokens

	if ( nStyle4StartPos < nStyle2StartPos && nStyle4StartPos <= nStyle3StartPos )  // <= to make sure \\ is always handled first
    {   // Skip quoted Backslash
        nPos = nStyle4StartPos +2;
        return GetNextTokenString( rErrorList, rTagStartPos );
    }

/*	if ( nStyle1StartPos < nStyle2StartPos && nStyle1StartPos <= nStyle3StartPos )  // <= to make sure our spechial tags are recognized before all others
    {	// test for <# ... > style tokens
	    USHORT nEndPos = aSource.SearchAscii( ">", nStyle1StartPos );
        if ( nEndPos == STRING_NOTFOUND )
        {   // Token is incomplete. Skip start and search for better ones
            nPos = nStyle1StartPos +2;
            return GetNextTokenString( rErrorList, rTagStartPos );
        }
	    nPos = nEndPos;
        rTagStartPos = nStyle1StartPos;
	    return aSource.Copy( nStyle1StartPos, nEndPos-nStyle1StartPos +1 ).ToUpperAscii();
    }
    else*/ if ( nStyle2StartPos < nStyle3StartPos )
	{	// test for $[ ... ] style tokens
		USHORT nEndPos = aSource.SearchAscii( "]", nStyle2StartPos);
        if ( nEndPos == STRING_NOTFOUND )
        {   // Token is incomplete. Skip start and search for better ones
            nPos = nStyle2StartPos +2;
            return GetNextTokenString( rErrorList, rTagStartPos );
        }
		nPos = nEndPos;
        rTagStartPos = nStyle2StartPos;
		return aSource.Copy( nStyle2StartPos, nEndPos-nStyle2StartPos +1 );
	}
    else
	{	// test for \< ... \> style tokens
    	USHORT nEndPos = aSource.SearchAscii( "\\>", nStyle3StartPos);
    	USHORT nQuotedBackPos = aSource.SearchAscii( "\\\\", nStyle3StartPos );    // this is only to kick out quoted backslashes
        while ( nQuotedBackPos <= nEndPos && nQuotedBackPos != STRING_NOTFOUND )
        {
		    nEndPos = aSource.SearchAscii( "\\>", nQuotedBackPos +2);
    	    nQuotedBackPos = aSource.SearchAscii( "\\\\", nQuotedBackPos +2 );    // this is only to kick out quoted backslashes
        }
        if ( nEndPos == STRING_NOTFOUND )
        {   // Token is incomplete. Skip start and search for better ones
            nPos = nStyle3StartPos +2;
		    ByteString sTmp( "Tag Start '\\<' without Tag End '\\>': " );
        	rErrorList.AddError( 24, "Tag Start '\\<' without Tag End '\\>'", TokenInfo( TAG_UNKNOWN_TAG, nStyle3StartPos, aSource.Copy( nStyle3StartPos-10, 20 ) ) );
            return GetNextTokenString( rErrorList, rTagStartPos );
        }
        // check for paired quoted "    -->   \"sometext\"

        nPos = nEndPos;
        rTagStartPos = nStyle3StartPos;
		return aSource.Copy( nStyle3StartPos, nEndPos-nStyle3StartPos +2 );
	}
}

String SimpleParser::GetLexem( TokenInfo const &aToken )
{
    if ( aToken.aTokenString.Len() )
        return aToken.aTokenString;
    else
    {
        USHORT i = 0;
	    while ( aKnownTags[i].nTag != TAG_UNKNOWN_TAG &&
		    aKnownTags[i].nTag != aToken.nId )
		    i++;

        return aKnownTags[i].GetName();
    }
}

TokenParser::TokenParser()
: pErrorList( NULL )
{}

void TokenParser::Parse( const String &aCode, ParserMessageList* pList )
{
    pErrorList = pList;

	//Scanner initialisieren
	aParser.Parse( aCode );

	//erstes Symbol holen
	aTag = aParser.GetNextToken( *pErrorList );

	nPfCaseOptions = 0;
	nAppCaseOptions = 0;
	bPfCaseActive = FALSE;
	bAppCaseActive = FALSE;

	nActiveRefTypes = 0;

    //Ausfuehren der Start-Produktion
	Paragraph();

	//Es wurde nicht die ganze Kette abgearbeitet, bisher ist aber
	//kein Fehler aufgetreten
	//=> es wurde ein einleitendes Tag vergessen
	if ( aTag.nId != TAG_NOMORETAGS )
	{
		switch ( aTag.nId )
		{
			case TAG_END:
				{
					ParseError( 3, "Extra Tag <#END>. Switch or <#HREF> expected.", aTag );
				}
				break;
			case TAG_BOLDOFF:
				{
					ParseError( 4, "<#BOLD> expected before <#/BOLD>.", aTag );
				}
				break;
			case TAG_ITALICOFF:
				{
					ParseError( 5, "<#ITALIC> expected before <#/ITALIC>.", aTag );
				}
				break;
			case TAG_UNDERLINEOFF:
				{
					ParseError( 17, "<#UNDER> expected before <#/UNDER>.", aTag );
				}
				break;
/*			case TAG_MISSPARENTHESIS:
				{
					ParseError( 14, "missing closing parenthesis '>'", aTag );
				}
				break;*/
			case TAG_AEND:
				{
					ParseError( 5, "Extra Tag <#AEND>. <#AVIS> or <#AHID> expected.", aTag );
				}
				break;
			case TAG_ELSE:
				{
					ParseError( 16, "Application-tag or platform-tag expected before <#ELSE>.", aTag );
				}
				break;
			case TAG_UNKNOWN_TAG:
				{
					ParseError( 6, "unknown Tag", aTag );
				}
				break;
			default:
				{
					ParseError( 6, "unexpected Tag", aTag );
				}
		}
	}
    pErrorList = NULL;
}

void TokenParser::Paragraph()
{
	switch ( aTag.nId )
	{
		case TAG_GRAPHIC:
		case TAG_NEXTVERSION:
			{
				TagRef();
				Paragraph();
			}
			break;
		case TAG_AVIS:
		case TAG_AHID:
			{
				TagRef();
				Paragraph();
			}
			break;
		case TAG_HELPID:
			{
				SimpleTag();
				Paragraph();
			}
			break;
		case TAG_OFFICEFULLNAME:
		case TAG_OFFICENAME:
		case TAG_OFFICEPATH:
		case TAG_OFFICEVERSION:
		case TAG_PORTALNAME:
		case TAG_PORTALFULLNAME:
		case TAG_PORTALPATH:
		case TAG_PORTALVERSION:
		case TAG_PORTALSHORTNAME:
			{
				SimpleTag();
				Paragraph();
			}
			break;
		case TAG_REFINSERT:
			{
				SimpleTag();
				Paragraph();
			}
			break;
		case TAG_BOLDON:
		case TAG_ITALICON:
		case TAG_UNDERLINEON:
		case TAG_COMMONSTART:
			{
				TagPair();
				Paragraph();
			}
			break;
		case TAG_HREF:
		case TAG_NAME:
		case TAG_KEY:
		case TAG_INDEX:
        case TAG_TITEL:
        case TAG_REFSTART:
            {
				TagRef();
				Paragraph();
			}
			break;
		case TAG_OS2:
		case TAG_WIN:
		case TAG_UNIX:
		case TAG_MAC: //...
			{
				if ( ! bPfCaseActive )
				{
                    //PfCases duerfen nicht verschachtelt sein:
					bPfCaseActive = TRUE;
					PfCase();

					//So jetzt kann wieder ein PfCase kommen:
					bPfCaseActive = FALSE;
					Paragraph();
				}
			}
			break;
		case TAG_WRITER:
		case TAG_CALC:
		case TAG_DRAW:
		case TAG_IMPRESS:
		case TAG_SCHEDULE:
		case TAG_IMAGE:
		case TAG_MATH:
		case TAG_CHART:
		case TAG_OFFICE:
			{
				if ( !bAppCaseActive )
				{
                    //AppCases duerfen nicht verschachtelt sein:
					bAppCaseActive = TRUE;
					AppCase();

                    //jetzt koennen wieder AppCases kommen:
					bAppCaseActive = FALSE;
					Paragraph();
				}
			}
			break;

		//Case TAG_BOLDOFF, TAG_ITALICOFF, TAG_BUNDERLINE, TAG_END
			//nichts tun wg. epsilon-Prod.
	}
}

void TokenParser::PfCase()
{

	//Produktion:
	//PfCase -> PfCaseBegin Paragraph (PfCase | PfCaseEnd)

	PfCaseBegin();

	//Jetzt ist eine PfCase-Produktion aktiv:
	Paragraph();
	switch ( aTag.nId )
	{
		case TAG_ELSE:
		case TAG_END:
			{
				CaseEnd();
			}
			break;
		case TAG_OS2:
		case TAG_WIN:
		case TAG_UNIX:
		case TAG_MAC: //First (PfBegin)
			{
				PfCase();
			}
			break;
		default:
			ParseError( 8, "<#ELSE> or <#END> or platform-tag expected.", aTag );
	}
    //Die gemerkten Tags wieder loeschen fuer naechstes PfCase:
	nPfCaseOptions = 0;
}

void TokenParser::PfCaseBegin()
{
	switch ( aTag.nId )
	{
		case TAG_OS2:
		case TAG_WIN:
		case TAG_UNIX:
		case TAG_MAC:
			{
				//Token darf noch nicht vorgekommen sein im
				//aktuellen Plattform-Case:
				if ( !HAS_FLAG( nPfCaseOptions, TAG_NOGROUP( aTag.nId ) ) )
				{
					SET_FLAG( nPfCaseOptions, TAG_NOGROUP( aTag.nId ) );
					match( aTag, aTag );
				}
				else {
					ParseError( 9, "Tag defined twice in the same platform-case", aTag );
				}
			}
	}
}

void TokenParser::AppCase()
{

	//Produktion:
	//AppCase -> AppCaseBegin Paragraph (AppCase | AppCaseEnd)


	AppCaseBegin();

	Paragraph();

	switch ( aTag.nId )
	{
		case TAG_ELSE:
		case TAG_END:
			{
				CaseEnd();
			}
			break;
		case TAG_WRITER:
		case TAG_DRAW:
		case TAG_CALC:
		case TAG_IMAGE:
		case TAG_MATH:
		case TAG_CHART:
		case TAG_OFFICE:
		case TAG_IMPRESS:
		case TAG_SCHEDULE: //First (AppBegin)
			{
				AppCase();
			}
			break;
		default:
			ParseError( 1, "<#ELSE> or <#END> or application-case-tag expected.", aTag );
		}

    //Die gemerkten Tags wieder loeschen fuer naechstes AppCase:
	nAppCaseOptions = 0;
}

void TokenParser::AppCaseBegin()
{
	switch ( aTag.nId )
	{
		case TAG_WRITER:
		case TAG_DRAW:
		case TAG_CALC:
		case TAG_IMAGE:
		case TAG_MATH:
		case TAG_CHART:
		case TAG_OFFICE:
		case TAG_IMPRESS:
		case TAG_SCHEDULE:
			{
				//Token darf noch nicht vorgekommen sein im
				//aktuellen Plattform-Case:
				if ( !HAS_FLAG( nAppCaseOptions, TAG_NOGROUP( aTag.nId ) ) )
				{
					SET_FLAG( nAppCaseOptions, TAG_NOGROUP( aTag.nId ) );
					match( aTag, aTag );
				}
				else {
					ParseError( 13, "Tag defined twice in the same application-case.", aTag );
				}
			}
	}
}

void TokenParser::CaseEnd()
{
	//Produktion:
	//CaseEnd -> <#ELSE> Paragraph <#END> | <#END>

	switch ( aTag.nId )
	{
		case TAG_ELSE:
		{
			match( aTag, TAG_ELSE );
			Paragraph();
			match( aTag, TAG_END );
		}
		break;
		case TAG_END:
		{
			match( aTag, TAG_END );
		}
		break;
		default:
			ParseError( 2, "<#ELSE> or <#END> expected.", aTag );
	}
}

void TokenParser::SimpleTag()
{

	switch ( aTag.nId )
	{
		case TAG_HELPID:
			{
				match( aTag, TAG_HELPID );
    		}
			break;
		case TAG_OFFICEFULLNAME:
		case TAG_OFFICENAME:
		case TAG_OFFICEPATH:
		case TAG_OFFICEVERSION:
		case TAG_PORTALNAME:
		case TAG_PORTALFULLNAME:
		case TAG_PORTALPATH:
		case TAG_PORTALVERSION:
		case TAG_PORTALSHORTNAME:

        case TAG_REFINSERT:
			{
				match( aTag, aTag );
    		}
			break;
		default:
			ParseError( 15, "[<#SimpleTag>] expected.", aTag );
	}
}

void TokenParser::TagPair()
{
	switch ( aTag.nId )
	{
		case TAG_BOLDON:
			{
				match( aTag, TAG_BOLDON );
				Paragraph();
				match( aTag, TAG_BOLDOFF );
			}
			break;
		case TAG_ITALICON:
			{
				match( aTag, TAG_ITALICON );
				Paragraph();
				match( aTag, TAG_ITALICOFF );
			}
			break;
		case TAG_UNDERLINEON:
			{
				match( aTag, TAG_UNDERLINEON );
				Paragraph();
				match( aTag, TAG_UNDERLINEOFF );
			}
			break;
		case TAG_COMMONSTART:
			{
                //remember tag so we can give the original tag in case of an error
                TokenInfo aEndTag( aTag );
                aEndTag.nId = TAG_COMMONEND;
				match( aTag, TAG_COMMONSTART );
				Paragraph();
				match( aTag, aEndTag );
			}
			break;
		default:
			ParseError( 10, "<#BOLD>, <#ITALIC>, <#UNDER> expected.", aTag );
	}
}


void TokenParser::TagRef()
{
	switch ( aTag.nId )
	{
		case TAG_GRAPHIC:
		case TAG_NEXTVERSION:
            {
				if ( !HAS_FLAG( nActiveRefTypes, TAG_NOGROUP( aTag.nId ) ) )
				{
					TokenId aThisToken = aTag.nId;
					SET_FLAG( nActiveRefTypes, TAG_NOGROUP( aThisToken ) );
					match( aTag, aTag );
					Paragraph();
                    if ( aThisToken == TAG_GRAPHIC )
    					match( aTag, TAG_ENDGRAPHIC );
                    else
    					match( aTag, TAG_VERSIONEND );
                    // don't reset since alowed only once per paragraph
					// RESET_FLAG( nActiveRefTypes, TAG_NOGROUP( aThisToken ) );
				}
				else
				{
					ParseError( 11, "Tags <#GRAPHIC>,<#NEXTVERSION> allowed only once per paragraph at", aTag );
				}
            }
			break;
		case TAG_AVIS:
		case TAG_AHID:
			{
				if ( !HAS_FLAG( nActiveRefTypes, TAG_NOGROUP( aTag.nId ) ) )
				{
					TokenId aThisToken = aTag.nId;
					SET_FLAG( nActiveRefTypes, TAG_NOGROUP( aThisToken ) );
					match( aTag, aTag );
					Paragraph();
					match( aTag, TAG_AEND );
					RESET_FLAG( nActiveRefTypes, TAG_NOGROUP( aThisToken ) );
				}
				else
				{
					ParseError( 11, "Nested <#AHID>,<#AVIS> not allowed.", aTag );
				}
			}
			break;
		case TAG_HREF:
		case TAG_NAME:
			{

			}
			// NOBREAK
		case TAG_KEY:
		case TAG_INDEX:
		case TAG_TITEL:
        case TAG_REFSTART:
			{
				if ( !HAS_FLAG( nActiveRefTypes, TAG_NOGROUP( aTag.nId ) ) )
				{
					TokenId aThisToken = aTag.nId;
					match( aTag, aTag );
					if ( aThisToken != TAG_NAME )
					{	// TAG_NAME has no TAG_END
						SET_FLAG( nActiveRefTypes, TAG_NOGROUP( aThisToken ) );
						Paragraph();
						match( aTag, TAG_END );
						RESET_FLAG( nActiveRefTypes, TAG_NOGROUP( aThisToken ) );
					}
				}
				else
				{
					ParseError( 11, "Nested <#HREF>,<#NAME> or <#KEY> not allowed.", aTag );
				}
			}
			break;
		default:
			ParseError( 12, "<#HREF>,<#NAME> or <#KEY> expected.", aTag );
	}
}

BOOL TokenParser::match( const TokenInfo &aCurrentToken, const TokenId &aExpectedToken )
{
    return match( aCurrentToken, TokenInfo( aExpectedToken, TOK_INVALIDPOS ) );
}

BOOL TokenParser::match( const TokenInfo &aCurrentToken, const TokenInfo &rExpectedToken )
{
    TokenInfo aExpectedToken( rExpectedToken );
	if ( aCurrentToken.nId == aExpectedToken.nId )
	{
        if ( ( aCurrentToken.nId == TAG_COMMONEND
               && aCurrentToken.GetTagName().Equals( aExpectedToken.GetTagName() ) )
             || aCurrentToken.nId != TAG_COMMONEND )
        {
		    aTag = aParser.GetNextToken( *pErrorList );
		    return TRUE;
        }
	}

    if ( aExpectedToken.nId == TAG_COMMONEND )
    {
        aExpectedToken.aTokenString.Insert( String::CreateFromAscii( "Close tag for " ), 0 );
    }

    ByteString sTmp( "Expected Symbol" );
    if ( aCurrentToken.nId == TAG_NOMORETAGS )
    {
		ParseError( 7, sTmp, aExpectedToken );
    }
    else
    {
		sTmp += ": ";
		sTmp += ByteString( aParser.GetLexem( aExpectedToken ), RTL_TEXTENCODING_UTF8 );
		sTmp += " near ";
		ParseError( 7, sTmp, aCurrentToken );
    }
	return FALSE;
}

void TokenParser::ParseError( USHORT nErrNr, ByteString aErrMsg, const TokenInfo &rTag )
{
	pErrorList->AddError( nErrNr, aErrMsg, rTag);

    // Das Fehlerhafte Tag ueberspringen
	aTag = aParser.GetNextToken( *pErrorList );
}


ParserMessage::ParserMessage( USHORT PnErrorNr, ByteString PaErrorText, const TokenInfo &rTag )
		: nErrorNr( PnErrorNr )
		, aErrorText( PaErrorText )
		, nTagBegin( 0 )
		, nTagLength( 0 )
{
    String aLexem( SimpleParser::GetLexem( rTag ) );
    aErrorText.Append(": ");
	aErrorText += ByteString( aLexem, RTL_TEXTENCODING_UTF8 );
    if ( rTag.nId == TAG_NOMORETAGS )
        aErrorText.Append(" at end of line ");
    else if ( rTag.nPos != TOK_INVALIDPOS )
    {
        aErrorText.Append(" at Position ");
        aErrorText.Append( ByteString::CreateFromInt32( rTag.nPos ) );
    }
    nTagBegin = rTag.nPos;
	nTagLength = aLexem.Len();
}

ParserError::ParserError( USHORT ErrorNr, ByteString ErrorText, const TokenInfo &rTag )
: ParserMessage( ErrorNr, ErrorText, rTag )
{}

ParserWarning::ParserWarning( USHORT ErrorNr, ByteString ErrorText, const TokenInfo &rTag )
: ParserMessage( ErrorNr, ErrorText, rTag )
{}

BOOL LingTest::IsTagMandatory( TokenInfo const &aToken, TokenId &aMetaTokens )
{
	TokenId aTokenId = aToken.nId;
	TokenId aTokenGroup = TAG_GROUP( aTokenId );
	if ( TAG_GROUP_PROGSWITCH == aTokenGroup
		|| TAG_REFINSERT == aTokenId
		|| TAG_REFSTART == aTokenId
		|| TAG_NAME == aTokenId
		|| TAG_HREF == aTokenId
		|| TAG_AVIS == aTokenId
		|| TAG_AHID == aTokenId
		|| TAG_GRAPHIC == aTokenId
		|| TAG_NEXTVERSION == aTokenId
        || ( TAG_GROUP_META == aTokenGroup && (aMetaTokens & aTokenId) == aTokenId ) )
    {
        if ( TAG_GROUP_META == aTokenGroup )
            aMetaTokens |= aTokenId;
        return TRUE;
    }
    else if (   TAG_COMMONSTART == aTokenId
		     || TAG_COMMONEND == aTokenId )
    {
        String aTagName = aToken.GetTagName();
        return !(aTagName.EqualsIgnoreCaseAscii( "comment" )
              || aTagName.EqualsIgnoreCaseAscii( "bookmark_value" )
              || aTagName.EqualsIgnoreCaseAscii( "emph" )
              || aTagName.EqualsIgnoreCaseAscii( "item" )
              || aTagName.EqualsIgnoreCaseAscii( "br" ) );
    }
    return FALSE;
}

void LingTest::CheckTags( TokenList &aReference, TokenList &aTestee, BOOL bFixTags )
{
	ULONG i=0,j=0;
	// Clean old Warnings
	while ( aCompareWarningList.Count() )
	{
		delete aCompareWarningList.GetCurObject();
		aCompareWarningList.Remove();
	}

    /* in xml tags, do not require the following tags
        comment
        bookmark_value
        emph
        item
        br
    */

    // filter uninteresting Tags
    TokenId aMetaTokens = 0;
	for ( i=0 ; i < aReference.Count() ; i++ )
	{
		if ( !IsTagMandatory( aReference.GetObject( i ), aMetaTokens ) )
			aReference.GetObject( i ).SetDone();
	}

    aMetaTokens = 0;
	for ( i=0 ; i < aTestee.Count() ; i++ )
	{
		if ( !IsTagMandatory( aTestee.GetObject( i ), aMetaTokens ) )
			aTestee.GetObject( i ).SetDone();
	}

    // remove all matching tags
	for ( i=0 ; i < aReference.Count() ; i++ )
    {
        if ( aReference.GetObject( i ).IsDone() )
            continue;

    	BOOL bTagFound = FALSE;
    	for ( j=0 ; j < aTestee.Count() && !bTagFound ; j++ )
        {
            if ( aTestee.GetObject( j ).IsDone() )
                continue;

		    if ( aReference.GetObject( i ).MatchesTranslation( aTestee.GetObject( j ), FALSE, aCompareWarningList ) )
            {
			    aReference.GetObject( i ).SetDone();
			    aTestee.GetObject( j ).SetDone();
                bTagFound = TRUE;
            }
        }
    }

    BOOL bCanFix = TRUE;

    if ( bFixTags )
    {
        // we fix only if its a really simple case
        USHORT nTagCount = 0;
	    for ( i=0 ; i < aReference.Count() ; i++ )
            if ( !aReference.GetObject( i ).IsDone() )
                nTagCount++;
        if ( nTagCount > 1 )
            bCanFix = FALSE;

        nTagCount = 0;
	    for ( i=0 ; i < aTestee.Count() ; i++ )
            if ( !aTestee.GetObject( i ).IsDone() )
                nTagCount++;
        if ( nTagCount > 1 )
            bCanFix = FALSE;
    }

    // generate errors for tags that have differing attributes
	for ( i=0 ; i < aReference.Count() ; i++ )
    {
        if ( aReference.GetObject( i ).IsDone() )
            continue;

    	BOOL bTagFound = FALSE;
    	for ( j=0 ; j < aTestee.Count() && !bTagFound ; j++ )
        {
            if ( aTestee.GetObject( j ).IsDone() )
                continue;

		    if ( aReference.GetObject( i ).MatchesTranslation( aTestee.GetObject( j ), TRUE, aCompareWarningList, bCanFix && bFixTags ) )
            {
			    aReference.GetObject( i ).SetDone();
			    aTestee.GetObject( j ).SetDone();
                bTagFound = TRUE;
            }
        }
    }

    // list remaining tags as errors
	for ( i=0 ; i < aReference.Count() ; i++ )
    {
        if ( aReference.GetObject( i ).IsDone() )
            continue;

        aCompareWarningList.AddError( 20, "Missing Tag in Translation", aReference.GetObject( i ) );
    }
	for ( i=0 ; i < aTestee.Count() ; i++ )
    {
        if ( aTestee.GetObject( i ).IsDone() )
            continue;

		aCompareWarningList.AddError( 21, "Extra Tag in Translation", aTestee.GetObject( i ) );
    }

	for ( i=0 ; i < aReference.Count() ; i++ )
        aReference.GetObject( i ).SetDone( FALSE );

	for ( i=0 ; i < aTestee.Count() ; i++ )
        aTestee.GetObject( i ).SetDone( FALSE );
}

void LingTest::CheckReference( GSILine *aReference )
{
	aReferenceParser.Parse( aReference->GetUText(), aReference->GetMessageList() );
}

void LingTest::CheckTestee( GSILine *aTestee, BOOL bHasSourceLine, BOOL bFixTags )
{
    aFixedTestee = aTestee->GetUText();
	aTesteeParser.Parse( aFixedTestee, aTestee->GetMessageList() );

    if ( bHasSourceLine )
	    CheckTags( aReferenceParser.GetTokenList(), aTesteeParser.GetTokenList(), bFixTags );

    if ( bFixTags )
    {
        TokenList& aTesteeTokens = aTesteeParser.GetTokenList();
        BOOL bFixesDone = FALSE;
        // count backwards to allow replacing from right to left
        int i;
	    for ( i=aTesteeTokens.Count()-1 ; i>=0 ; i-- )
        {
            if ( aTesteeTokens.GetObject( i ).HasBeenFixed() )
            {
                bFixesDone = TRUE;
                aFixedTestee.Replace( aTesteeTokens.GetObject( i ).nPos, aTesteeTokens.GetObject( i ).aTokenString.Len(), aTesteeTokens.GetObject( i ).MakeTag() );
            }
        }
        if ( bFixesDone )
        {
            aTestee->SetUText( aFixedTestee );
            aTestee->SetFixed();
        }
    }
}

