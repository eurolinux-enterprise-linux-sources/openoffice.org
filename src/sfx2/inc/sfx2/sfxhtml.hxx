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

#ifndef _SFXHTML_HXX
#define _SFXHTML_HXX

#include "sal/config.h"
#include "sfx2/dllapi.h"
#include "sal/types.h"
#include <i18npool/lang.h>
#include <svtools/parhtml.hxx>
#include <svtools/macitem.hxx>


class ImageMap;
class SfxMedium;
class SfxObjectShell;

class SFX2_DLLPUBLIC SfxHTMLParser : public HTMLParser
{
	DECL_DLLPRIVATE_STATIC_LINK( SfxHTMLParser, FileDownloadDone, void* );

	String					aScriptType;

	SfxMedium*				pMedium;
	SfxMedium *pDLMedium;	// Medium fuer Download von Files

	USHORT nMetaTags;		// Anzahl der bisher gelesenen Meta-Tags
	ScriptType eScriptType;

	SAL_DLLPRIVATE void GetScriptType_Impl( SvKeyValueIterator* );

protected:

	SfxHTMLParser( SvStream& rStream, BOOL bNewDoc=TRUE, SfxMedium *pMedium=0 );

    virtual ~SfxHTMLParser();

public:
	// Lesen der Optionen einer Image-Map
	// <MAP>: TRUE = Image-Map hat einen Namen
	// <AREA>: TRUE = Image-Map hat jetzt einen Bereich mehr
	static BOOL ParseMapOptions(ImageMap * pImageMap,
								const HTMLOptions * pOptions );
	BOOL ParseMapOptions(ImageMap * pImageMap)
	{ return ParseMapOptions(pImageMap, GetOptions()); }
    static BOOL ParseAreaOptions(ImageMap * pImageMap, const String& rBaseURL,
								 const HTMLOptions * pOptions,
								 USHORT nEventMouseOver = 0,
								 USHORT nEventMouseOut = 0 );
    inline BOOL ParseAreaOptions(ImageMap * pImageMap, const String& rBaseURL,
								 USHORT nEventMouseOver = 0,
								 USHORT nEventMouseOut = 0);

	// <TD SDVAL="..." SDNUM="...">
	static double GetTableDataOptionsValNum( sal_uInt32& nNumForm,
			LanguageType& eNumLang, const String& aValStr,
			const String& aNumStr, SvNumberFormatter& rFormatter );

    static rtl_TextEncoding GetEncodingByHttpHeader( SvKeyValueIterator *pHTTPHeader );

protected:

	// Start eines File-Downloads. Dieser erfolgt synchron oder asynchron.
	// Im synchronen Fall befindet sich der Parser nach dem Aufruf im
	// Working-Zustand. Die gelesene Datei kann dann direkt mit
	// FinishFileDownload abgeholt werden.
	// Im asynchronen Fall befindet sich der Parser nach dem Aufruf im
	// Pending-Zustand. Der Parser muss dann ueber das Continue verlassen
	// werden (ohne Reschedule!). Wenn die Datei geladen ist, wird
	// ein Continue mit dem uebergebenen Token aufgerufen. Die Datei kann
	// dann wiederum mit FinishFileDownload abgeholt werden.
	// Zum Abbrechen des Dwonloads sollte eine Shell uebergeben werden.
	// Es kann nur ein einziger Download gleichzeitig existieren. Fuer jeden
	// gestarteten Download muss FinshFileDownload aufgerufen werden.
	void StartFileDownload( const String& rURL, int nToken,
							SfxObjectShell *pSh=0 );

	// Ermittelnd des MIME-Types eines zuvor downloadeten Files. Kann nur
	// unmittelbar vor FinishFileDownload aufgerufen werden, nie aber
	// danach.

	BOOL GetFileDownloadMIME( String& rMime );

	// Beenden eines asynchronen File-Downloads. Gibt TRUE zurueck, wenn
	// der Download geklappt hat. Das gelesene File befindet sich dann in
	// dem uebergeben String.
	BOOL FinishFileDownload( String& rStr );

	// Gibt TRUE zurueck, wenn ein File downloaded wurde und
	// FileDownloadFinished noch nicht gerufen wurde.
	BOOL ShouldFinishFileDownload() const { return pDLMedium != 0; }

	SfxMedium *GetMedium() { return pMedium; }
	const SfxMedium *GetMedium() const { return pMedium; }

	// Default (auch ohne Iterator) ist JavaScript
	ScriptType GetScriptType( SvKeyValueIterator* ) const;
	const String& GetScriptTypeString( SvKeyValueIterator* ) const;

	BOOL SetEncodingByHTTPHeader( SvKeyValueIterator *pHTTPHeader );

};

inline BOOL SfxHTMLParser::ParseAreaOptions(ImageMap * pImageMap, const String& rBaseURL,
											USHORT nEventMouseOver,
											USHORT nEventMouseOut)
{
    return ParseAreaOptions( pImageMap, rBaseURL, GetOptions(),
							 nEventMouseOver, nEventMouseOut );
}


#endif
