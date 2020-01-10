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

#ifndef _SALLANG_HXX
#include <sallang.hxx>
#endif

// =======================================================================

// -----------------------------------------------------------------------
// English (US/UK/AUS/CAN/NZ/EIRE/SAFRICA/JAMAICA/CARRIBEAN)
static const wchar_t* aImplLangEnglishTab[LSTR_COUNT] =
{
	L"Shift",                                                // LSTR_KEY_SHIFT
	L"Ctrl",                                                 // LSTR_KEY_CTRL
	L"Alt",                                                  // LSTR_KEY_ALT
	L"Up",                                                   // LSTR_KEY_UP
	L"Down",                                                 // LSTR_KEY_DOWN
	L"Left",                                                 // LSTR_KEY_LEFT
	L"Right",                                                // LSTR_KEY_RIGHT
	L"Home",                                                 // LSTR_KEY_HOME
	L"End",                                                  // LSTR_KEY_END
	L"PageUp",                                               // LSTR_KEY_PAGEUP
	L"PageDown",                                             // LSTR_KEY_PAGEDOWN
	L"Enter",                                                // LSTR_KEY_RETURN
	L"Esc",                                                  // LSTR_KEY_ESC
	L"Tab",                                                  // LSTR_KEY_TAB
	L"Backspace",                                            // LSTR_KEY_BACKSPACE
	L"Space",                                                // LSTR_KEY_SPACE
	L"Insert",                                               // LSTR_KEY_INSERT
	L"Del",                                                  // LSTR_KEY_DELETE
};

// =======================================================================

const sal_Unicode** ImplGetLangTab( LanguageType eLang )
{
	// Sprachtabelle ermitteln
	const wchar_t** pLangTab;
	//switch ( International::GetNeutralLanguage( eLang ) )
	switch ( eLang )
	{
#if 0
		case LANGUAGE_DANISH:
			pLangTab = aImplLangDanishTab;
			break;

		case LANGUAGE_DUTCH:
		case LANGUAGE_DUTCH_BELGIAN:
			pLangTab = aImplLangDutchTab;
			break;

		case LANGUAGE_FINNISH:
			pLangTab = aImplLangFinnishTab;
			break;

		case LANGUAGE_FRENCH:
			pLangTab = aImplLangFrenchTab;
			break;

		case LANGUAGE_GERMAN:
			pLangTab = aImplLangGermanTab;
			break;

		case LANGUAGE_ITALIAN:
			pLangTab = aImplLangItalianTab;
			break;

		case LANGUAGE_NORWEGIAN:
		case LANGUAGE_NORWEGIAN_BOKMAL:
			pLangTab = aImplLangNorwegianTab;
			break;

		case LANGUAGE_PORTUGUESE:
		case LANGUAGE_PORTUGUESE_BRAZILIAN:
			pLangTab = aImplLangPortugueseTab;
			break;

		case LANGUAGE_SPANISH:
			pLangTab = aImplLangSpanishTab;
			break;

		case LANGUAGE_SWEDISH:
			pLangTab = aImplLangSwedishTab;
			break;
#endif
		default:
			pLangTab = aImplLangEnglishTab;
			break;
	}

	return (const sal_Unicode**)pLangTab;
}
