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
#include <tools/fsys.hxx>

// local includes
#include "lngmerge.hxx"
#include "utf8conv.hxx"
#include <iostream>
using namespace std;
//
// class LngParser
//
/*****************************************************************************/
LngParser::LngParser( const ByteString &rLngFile, BOOL bUTF8, BOOL bULFFormat , bool bQuiet_in )
/*****************************************************************************/
				:
				nError( LNG_OK ),
				pLines( NULL ),
                sSource( rLngFile ),
				bDBIsUTF8( bUTF8 ),
				bULF( bULFFormat ),
                bQuiet( bQuiet_in )
{
	pLines = new LngLineList( 100, 100 );
	DirEntry aEntry( String( sSource, RTL_TEXTENCODING_ASCII_US ));
	if ( aEntry.Exists()) {
		SvFileStream aStream( String( sSource, RTL_TEXTENCODING_ASCII_US ), STREAM_STD_READ );
		if ( aStream.IsOpen()) {
			ByteString sLine;
            bool bFirstLine = true;			
            while ( !aStream.IsEof()) {
				aStream.ReadLine( sLine );
				
                if( bFirstLine ){       // Always remove UTF8 BOM from the first line
                    Export::RemoveUTF8ByteOrderMarker( sLine );
                    bFirstLine = false;
                }
                
                pLines->Insert( new ByteString( sLine ), LIST_APPEND );
			}
		}
		else
			nError = LNG_COULD_NOT_OPEN;
	}
	else
		nError = LNG_FILE_NOTFOUND;
}

/*****************************************************************************/
LngParser::~LngParser()
/*****************************************************************************/
{
	for ( ULONG i = 0; i < pLines->Count(); i++ )
		delete pLines->GetObject( i );
	delete pLines;
}

/*****************************************************************************/
void LngParser::FillInFallbacks( ByteStringHashMap Text )
/*****************************************************************************/
{
    ByteString sCur;
    for( unsigned int n = 0; n < aLanguages.size(); n++ ){
        sCur = aLanguages[ n ];
        
        if( Export::isAllowed( sCur ) ){
            ByteString sFallbackLang = Export::GetFallbackLanguage( sCur );
            if( sFallbackLang.Len() ){
                Text[ sCur ] = Text[ sFallbackLang ];
            }
		}
	}
}

/*****************************************************************************/
BOOL LngParser::CreateSDF(
	const ByteString &rSDFFile, const ByteString &rPrj,
	const ByteString &rRoot )
/*****************************************************************************/
{

    Export::InitLanguages( false );
    aLanguages = Export::GetLanguages();
    SvFileStream aSDFStream( String( rSDFFile, RTL_TEXTENCODING_ASCII_US ),
		STREAM_STD_WRITE | STREAM_TRUNC );
	if ( !aSDFStream.IsOpen()) {
		nError = SDF_COULD_NOT_OPEN;
	}
    aSDFStream.SetStreamCharSet( RTL_TEXTENCODING_UTF8 );	
    nError = SDF_OK;
	DirEntry aEntry( String( sSource, RTL_TEXTENCODING_ASCII_US ));
	aEntry.ToAbs();
	String sFullEntry = aEntry.GetFull();
	aEntry += DirEntry( String( "..", RTL_TEXTENCODING_ASCII_US ));
	aEntry += DirEntry( rRoot );
	ByteString sPrjEntry( aEntry.GetFull(), gsl_getSystemTextEncoding());
	ByteString sActFileName(
		sFullEntry.Copy( sPrjEntry.Len() + 1 ), gsl_getSystemTextEncoding());
	sActFileName.SearchAndReplaceAll( "/", "\\" );

	ULONG nPos  = 0;
	BOOL bStart = true;
	ByteString sGroup;
  	ByteStringHashMap Text;
    ByteString sID;
    ByteString sLine;

    while( nPos < pLines->Count() ){
        sLine = *pLines->GetObject( nPos++ );    
        while( nPos < pLines->Count() && !isNextGroup( sGroup , sLine ) ){
            ReadLine( sLine , Text );
            sID = sGroup;
            sLine = *pLines->GetObject( nPos++ );    
        };
        if( bStart ){
            bStart = false;
            sID = sGroup;
        }
        else {

            WriteSDF( aSDFStream , Text , rPrj , rRoot , sActFileName , sID );    
        }
    }
	aSDFStream.Close();
	return true;
}

 void LngParser::WriteSDF( SvFileStream &aSDFStream , ByteStringHashMap &rText_inout ,	
     const ByteString &rPrj , const ByteString &rRoot ,
     const ByteString &sActFileName , const ByteString &sID )
 {
     
    BOOL bExport = true;
    if ( bExport ) {
   		ByteString sTimeStamp( Export::GetTimeStamp());
        ByteString sCur;
		FillInFallbacks( rText_inout );
        for( unsigned int n = 0; n < aLanguages.size(); n++ ){
            sCur = aLanguages[ n ];
            ByteString sAct = rText_inout[ sCur ];	
            if ( !sAct.Len() && sCur.Len() )
                sAct = rText_inout[ ByteString("en-US") ];
		    
            ByteString sOutput( rPrj ); sOutput += "\t";
			if ( rRoot.Len())
			    sOutput += sActFileName;
			sOutput += "\t0\t";
			sOutput += "LngText\t";
			sOutput += sID; sOutput += "\t\t\t\t0\t";
			sOutput += sCur; sOutput += "\t";
			sOutput += sAct; sOutput += "\t\t\t\t";
			sOutput += sTimeStamp;
            //if( !sCur.EqualsIgnoreCaseAscii("de") ||( sCur.EqualsIgnoreCaseAscii("de") && !Export::isMergingGermanAllowed( rPrj ) ) )
            aSDFStream.WriteLine( sOutput );
		}
    }
 }
 bool LngParser::isNextGroup( ByteString &sGroup_out , ByteString &sLine_in ){
	sLine_in.EraseLeadingChars( ' ' );
	sLine_in.EraseTrailingChars( ' ' );
	if (( sLine_in.GetChar( 0 ) == '[' ) &&
			( sLine_in.GetChar( sLine_in.Len() - 1 ) == ']' )){
		sGroup_out = sLine_in.GetToken( 1, '[' ).GetToken( 0, ']' );
		sGroup_out.EraseLeadingChars( ' ' );
		sGroup_out.EraseTrailingChars( ' ' );
		return true;
	}
    return false;
 }
 void LngParser::ReadLine( const ByteString &sLine_in , ByteStringHashMap &rText_inout){
    //printf("sLine -> '%s'\n",sLine_in.GetBuffer());
    ByteString sLang = sLine_in.GetToken( 0, '=' );
	sLang.EraseLeadingChars( ' ' );
	sLang.EraseTrailingChars( ' ' );
	ByteString sText = sLine_in.GetToken( 1, '\"' ).GetToken( 0, '\"' );
	if( sLang.Len() )
        rText_inout[ sLang ] = sText;   
 }

/*****************************************************************************/
BOOL LngParser::Merge(
	const ByteString &rSDFFile, const ByteString &rDestinationFile , const ByteString& rPrj )
/*****************************************************************************/
{
    (void) rPrj;
    Export::InitLanguages( true );
    SvFileStream aDestination(
		String( rDestinationFile, RTL_TEXTENCODING_ASCII_US ),
		STREAM_STD_WRITE | STREAM_TRUNC );
	if ( !aDestination.IsOpen()) {
		nError = LNG_COULD_NOT_OPEN;
	}
	nError = LNG_OK;
//    MergeDataFile( const ByteString &rFileName, const ByteString& rFile , BOOL bErrLog, CharSet aCharSet, BOOL bUTF8 );

	MergeDataFile aMergeDataFile( rSDFFile, sSource , FALSE, RTL_TEXTENCODING_MS_1252);//, bDBIsUTF8 );
    ByteString sTmp( Export::sLanguages );
    if( sTmp.ToUpperAscii().Equals("ALL") ) 
        Export::SetLanguages( aMergeDataFile.GetLanguages() );
    aLanguages = Export::GetLanguages();

	ULONG nPos = 0;
	BOOL bGroup = FALSE;
	ByteString sGroup;

	// seek to next group
	while ( nPos < pLines->Count() && !bGroup ) {
		ByteString sLine( *pLines->GetObject( nPos ));
		sLine.EraseLeadingChars( ' ' );
		sLine.EraseTrailingChars( ' ' );
		if (( sLine.GetChar( 0 ) == '[' ) &&
			( sLine.GetChar( sLine.Len() - 1 ) == ']' ))
		{
			sGroup = sLine.GetToken( 1, '[' ).GetToken( 0, ']' );
			sGroup.EraseLeadingChars( ' ' );
			sGroup.EraseTrailingChars( ' ' );
			bGroup = TRUE;
		}
		nPos ++;
	}

	while ( nPos < pLines->Count()) {
        ByteStringHashMap Text;
		ByteString sID( sGroup );
		ULONG nLastLangPos = 0;

		ResData  *pResData = new ResData( "", sID , sSource );
        pResData->sResTyp = "LngText";
		PFormEntrys *pEntrys = aMergeDataFile.GetPFormEntrys( pResData );
		// read languages
		bGroup = FALSE;

		ByteString sLanguagesDone;

		while ( nPos < pLines->Count() && !bGroup ) {
			ByteString sLine( *pLines->GetObject( nPos ));
			sLine.EraseLeadingChars( ' ' );
			sLine.EraseTrailingChars( ' ' );
			if (( sLine.GetChar( 0 ) == '[' ) &&
				( sLine.GetChar( sLine.Len() - 1 ) == ']' ))
			{
				sGroup = sLine.GetToken( 1, '[' ).GetToken( 0, ']' );
				sGroup.EraseLeadingChars( ' ' );
				sGroup.EraseTrailingChars( ' ' );
				bGroup = TRUE;
				nPos ++;
				sLanguagesDone = "";
			}
			else if ( sLine.GetTokenCount( '=' ) > 1 ) {
				ByteString sLang = sLine.GetToken( 0, '=' );
				sLang.EraseLeadingChars( ' ' );
				sLang.EraseTrailingChars( ' ' );

				ByteString sSearch( ";" );
				sSearch += sLang;
				sSearch += ";";

				if (( sLanguagesDone.Search( sSearch ) != STRING_NOTFOUND )) {
					pLines->Remove( nPos );
				}
                if( bULF && pEntrys )
				{
					// this is a valid text line
					ByteString sText = sLine.GetToken( 1, '\"' ).GetToken( 0, '\"' );
                    if( sLang.Len() ){
						ByteString sNewText;
                        pEntrys->GetText( sNewText, STRING_TYP_TEXT, sLang, TRUE );

						if ( sNewText.Len()) {
							ByteString *pLine = pLines->GetObject( nPos );
                            
								ByteString sText1( sLang );
								sText1 += " = \"";
								sText1 += sNewText;
								sText1 += "\"";
								*pLine = sText1;
							//}
                            Text[ sLang ] = sNewText;
						}
					}
					nLastLangPos = nPos;
					nPos ++;
					sLanguagesDone += sSearch;
				}
				else {
					nLastLangPos = nPos;
					nPos ++;
					sLanguagesDone += sSearch;
				}
			}
			else
				nPos++;
		}
		ByteString sCur;
        if ( nLastLangPos ) {
            for( unsigned int n = 0; n < aLanguages.size(); n++ ){
                sCur = aLanguages[ n ];
                if(   //( !sCur.EqualsIgnoreCaseAscii("de") || 
                      //( sCur.EqualsIgnoreCaseAscii("de") && Export::isMergingGermanAllowed( rPrj ) ) ) 
                    !sCur.EqualsIgnoreCaseAscii("en-US") && !Text[ sCur ].Len() && pEntrys ){
                    
                    ByteString sNewText;
                    pEntrys->GetText( sNewText, STRING_TYP_TEXT, sCur, TRUE );
                    if (( sNewText.Len()) &&
                        !(( sCur.Equals("x-comment") ) && ( sNewText == "-" )))
					{
						ByteString sLine;
                        sLine += sCur;
						sLine += " = \"";
						sLine += sNewText;
						sLine += "\"";

						nLastLangPos++;
						nPos++;

						pLines->Insert( new ByteString( sLine ), nLastLangPos );
					}
				}
			}
		}

		delete pResData;
	}

	for ( ULONG i = 0; i < pLines->Count(); i++ )
		aDestination.WriteLine( *pLines->GetObject( i ));

	aDestination.Close();
	return TRUE;
}
