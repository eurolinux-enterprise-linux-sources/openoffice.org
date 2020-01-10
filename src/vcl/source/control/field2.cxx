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
#include <tools/debug.hxx>

#ifndef _SV_RC_H
#include <tools/rc.h>
#endif
#include <vcl/svdata.hxx>
#ifndef _SV_EVENT_HXX
#include <vcl/field.hxx>
#endif
#include <vcl/svapp.hxx>
#include <vcl/sound.hxx>
#include <vcl/event.hxx>
#include <vcl/field.hxx>
#include <i18npool/mslangid.hxx>

#include <vcl/unohelp.hxx>

#include <com/sun/star/lang/Locale.hpp>
#include <com/sun/star/i18n/XCharacterClassification.hpp>
#include <com/sun/star/i18n/KCharacterType.hpp>


#include <unotools/localedatawrapper.hxx>
#include <unotools/calendarwrapper.hxx>
#include <unotools/charclass.hxx>

using namespace ::com::sun::star;

// =======================================================================

#define EDITMASK_LITERAL       'L'
#define EDITMASK_ALPHA         'a'
#define EDITMASK_UPPERALPHA    'A'
#define EDITMASK_ALPHANUM      'c'
#define EDITMASK_UPPERALPHANUM 'C'
#define EDITMASK_NUM           'N'
#define EDITMASK_NUMSPACE      'n'
#define EDITMASK_ALLCHAR       'x'
#define EDITMASK_UPPERALLCHAR  'X'

uno::Reference< i18n::XCharacterClassification > ImplGetCharClass()
{
    static uno::Reference< i18n::XCharacterClassification > xCharClass;
    if ( !xCharClass.is() )
        xCharClass = vcl::unohelper::CreateCharacterClassification();

    return xCharClass;
}

// -----------------------------------------------------------------------

static sal_Unicode* ImplAddString( sal_Unicode* pBuf, const String& rStr )
{
	if ( rStr.Len() == 1 )
		*pBuf++ = rStr.GetChar(0);
	else if ( rStr.Len() == 0 )
		;
	else
	{
		memcpy( pBuf, rStr.GetBuffer(), rStr.Len() * sizeof(sal_Unicode) );
		pBuf += rStr.Len();
	}
	return pBuf;
}

// -----------------------------------------------------------------------

static sal_Unicode* ImplAddNum( sal_Unicode* pBuf, ULONG nNumber, int nMinLen )
{
	// fill temp buffer with digits
    sal_Unicode aTempBuf[30];
    sal_Unicode* pTempBuf = aTempBuf;
    do
    {
        *pTempBuf = (sal_Unicode)(nNumber % 10) + '0';
        pTempBuf++;
        nNumber /= 10;
        if ( nMinLen )
            nMinLen--;
    }
    while ( nNumber );

    // fill with zeros up to the minimal length
    while ( nMinLen > 0 )
    {
        *pBuf = '0';
        pBuf++;
        nMinLen--;
    }

    // copy temp buffer to real buffer
    do
    {
        pTempBuf--;
        *pBuf = *pTempBuf;
        pBuf++;
    }
    while ( pTempBuf != aTempBuf );

    return pBuf;
}

// -----------------------------------------------------------------------

static USHORT ImplGetNum( const sal_Unicode*& rpBuf, BOOL& rbError )
{
    if ( !*rpBuf )
    {
        rbError = TRUE;
        return 0;
    }

    USHORT nNumber = 0;
    while( ( *rpBuf >= '0' ) && ( *rpBuf <= '9' ) )
    {
        nNumber *= 10;
        nNumber += *rpBuf - '0';
        rpBuf++;
    }

    return nNumber;
}

// -----------------------------------------------------------------------

static void ImplSkipDelimiters( const sal_Unicode*& rpBuf )
{
    while( ( *rpBuf == ',' ) || ( *rpBuf == '.' ) || ( *rpBuf == ';' ) || 
           ( *rpBuf == ':' ) || ( *rpBuf == '-' ) || ( *rpBuf == '/' ) )
    {
        rpBuf++;
    }
}

// -----------------------------------------------------------------------

static int ImplIsPatternChar( xub_Unicode cChar, sal_Char cEditMask )
{
    sal_Int32 nType = 0;

    try
	{
        String aCharStr( cChar );
        nType = ImplGetCharClass()->getStringType( aCharStr, 0, aCharStr.Len(), Application::GetSettings().GetLocale() );
    }
    catch ( ::com::sun::star::uno::Exception& )
    {
		DBG_ERRORFILE( "ImplIsPatternChar: Exception caught!" );
		return FALSE;
	}

    if ( (cEditMask == EDITMASK_ALPHA) || (cEditMask == EDITMASK_UPPERALPHA) )
    {
        if( !CharClass::isLetterType( nType ) )
            return FALSE;
    }
    else if ( cEditMask == EDITMASK_NUM )
    {
        if( !CharClass::isNumericType( nType ) )
            return FALSE;
    }
    else if ( (cEditMask == EDITMASK_ALPHANUM) || (cEditMask == EDITMASK_UPPERALPHANUM) )
    {
        if( !CharClass::isLetterNumericType( nType ) )
            return FALSE;
    }
    else if ( (cEditMask == EDITMASK_ALLCHAR) || (cEditMask == EDITMASK_UPPERALLCHAR) )
    {
        if ( cChar < 32 )
            return FALSE;
    }
    else if ( cEditMask == EDITMASK_NUMSPACE )
    {
        if ( !CharClass::isNumericType( nType ) && ( cChar != ' ' ) )
            return FALSE;
    }
    else
        return FALSE;

    return TRUE;
}

// -----------------------------------------------------------------------

static xub_Unicode ImplPatternChar( xub_Unicode cChar, sal_Char cEditMask )
{
    if ( ImplIsPatternChar( cChar, cEditMask ) )
    {
        if ( (cEditMask == EDITMASK_UPPERALPHA) ||
             (cEditMask == EDITMASK_UPPERALPHANUM) ||
             ( cEditMask == EDITMASK_UPPERALLCHAR ) )
        {
            cChar = ImplGetCharClass()->toUpper( String(cChar),0,1,Application::GetSettings().GetLocale() )[0];
        }
        return cChar;
    }
    else
        return 0;
}

// -----------------------------------------------------------------------

static int ImplKommaPointCharEqual( xub_Unicode c1, xub_Unicode c2 )
{
    if ( c1 == c2 )
        return TRUE;
    else if ( ((c1 == '.') || (c1 == ',')) &&
              ((c2 == '.') || (c2 == ',')) )
        return TRUE;
    else
        return FALSE;
}

// -----------------------------------------------------------------------

static XubString ImplPatternReformat( const XubString& rStr,
                                      const ByteString& rEditMask,
                                      const XubString& rLiteralMask,
                                      USHORT nFormatFlags )
{
    if ( !rEditMask.Len() )
        return rStr;

    XubString   aStr    = rStr;
    XubString   aOutStr = rLiteralMask;
    xub_Unicode cTempChar;
    xub_Unicode cChar;
    xub_Unicode cLiteral;
    sal_Char    cMask;
    xub_StrLen  nStrIndex = 0;
    xub_StrLen  i = 0;
    xub_StrLen  n;

    while ( i < rEditMask.Len() )
    {
        if ( nStrIndex >= aStr.Len() )
            break;

        cChar = aStr.GetChar(nStrIndex);
        cLiteral = rLiteralMask.GetChar(i);
        cMask = rEditMask.GetChar(i);

        // Aktuelle Position ein Literal
        if ( cMask == EDITMASK_LITERAL )
        {
            // Wenn es das Literal-Zeichen ist, uebernehmen, ansonsten
            // ignorieren, da es das naechste gueltige Zeichen vom String
            // sein kann
            if ( ImplKommaPointCharEqual( cChar, cLiteral ) )
                nStrIndex++;
            else
            {
                // Ansonsten testen wir, ob es ein ungueltiges Zeichen ist.
                // Dies ist dann der Fall, wenn es nicht in das Muster
                // des naechsten nicht Literal-Zeichens passt
                n = i+1;
                while ( n < rEditMask.Len() )
                {
                    if ( rEditMask.GetChar(n) != EDITMASK_LITERAL )
                    {
                        if ( !ImplIsPatternChar( cChar, rEditMask.GetChar(n) ) )
                            nStrIndex++;
                        break;
                    }

                    n++;
                }
            }
        }
        else
        {
            // Gueltiges Zeichen an der Stelle
            cTempChar = ImplPatternChar( cChar, cMask );
            if ( cTempChar )
            {
                // dann Zeichen uebernehmen
                aOutStr.SetChar( i, cTempChar );
                nStrIndex++;
            }
            else
            {
                // Wenn es das Literalzeichen ist, uebernehmen
                if ( cLiteral == cChar )
                    nStrIndex++;
                else
                {
                    // Wenn das ungueltige Zeichen das naechste Literalzeichen
                    // sein kann, dann springen wir bis dahin vor, ansonten
                    // das Zeichen ignorieren
                    // Nur machen, wenn leere Literale erlaubt sind
                    if ( nFormatFlags & PATTERN_FORMAT_EMPTYLITERALS )
                    {
                        n = i;
                        while ( n < rEditMask.Len() )
                        {
                            if ( rEditMask.GetChar( n ) == EDITMASK_LITERAL )
                            {
                                if ( ImplKommaPointCharEqual( cChar, rLiteralMask.GetChar( n ) ) )
                                    i = n+1;

                                break;
                            }

                            n++;
                        }
                    }

                    nStrIndex++;
                    continue;
                }
            }
        }

        i++;
    }

    return aOutStr;
}

// -----------------------------------------------------------------------

static void ImplPatternMaxPos( const XubString rStr, const ByteString& rEditMask,
                               USHORT nFormatFlags, BOOL bSameMask,
                               USHORT nCursorPos, USHORT& rPos )
{

    // Letzte Position darf nicht groesser als der enthaltene String sein
    xub_StrLen nMaxPos = rStr.Len();

    // Wenn keine leeren Literale erlaubt sind, auch Leerzeichen
    // am Ende ignorieren
    if ( bSameMask && !(nFormatFlags & PATTERN_FORMAT_EMPTYLITERALS) )
    {
        while ( nMaxPos )
        {
            if ( (rEditMask.GetChar(nMaxPos-1) != EDITMASK_LITERAL) &&
                 (rStr.GetChar(nMaxPos-1) != ' ') )
                break;
            nMaxPos--;
        }

        // Wenn wir vor einem Literal stehen, dann solange weitersuchen,
        // bis erste Stelle nach Literal
        xub_StrLen nTempPos = nMaxPos;
        while ( nTempPos < rEditMask.Len() )
        {
            if ( rEditMask.GetChar(nTempPos) != EDITMASK_LITERAL )
            {
                nMaxPos = nTempPos;
                break;
            }
            nTempPos++;
        }
    }

    if ( rPos > nMaxPos )
        rPos = nMaxPos;
    // Zeichen sollte nicht nach links wandern
    if ( rPos < nCursorPos )
        rPos = nCursorPos;
}

// -----------------------------------------------------------------------

static void ImplPatternProcessStrictModify( Edit* pEdit,
                                            const ByteString& rEditMask,
                                            const XubString& rLiteralMask,
                                            USHORT nFormatFlags, BOOL bSameMask )
{
    XubString aText = pEdit->GetText();

    // Leerzeichen am Anfang entfernen
    if ( bSameMask && !(nFormatFlags & PATTERN_FORMAT_EMPTYLITERALS) )
    {
        xub_StrLen i = 0;
        xub_StrLen nMaxLen = aText.Len();
        while ( i < nMaxLen )
        {
            if ( (rEditMask.GetChar( i ) != EDITMASK_LITERAL) &&
                 (aText.GetChar( i ) != ' ') )
                break;

            i++;
        }
        // Alle Literalzeichen beibehalten
        while ( i && (rEditMask.GetChar( i ) == EDITMASK_LITERAL) )
            i--;
        aText.Erase( 0, i );
    }

    XubString aNewText = ImplPatternReformat( aText, rEditMask, rLiteralMask, nFormatFlags );
    if ( aNewText != aText )
    {
        // Selection so anpassen, das diese wenn sie vorher am Ende
        // stand, immer noch am Ende steht
        Selection aSel = pEdit->GetSelection();
        ULONG nMaxSel = Max( aSel.Min(), aSel.Max() );
        if ( nMaxSel >= aText.Len() )
        {
            xub_StrLen nMaxPos = aNewText.Len();
            ImplPatternMaxPos( aNewText, rEditMask, nFormatFlags, bSameMask, (xub_StrLen)nMaxSel, nMaxPos );
            if ( aSel.Min() == aSel.Max() )
            {
                aSel.Min() = nMaxPos;
                aSel.Max() = aSel.Min();
            }
            else if ( aSel.Min() > aSel.Max() )
                aSel.Min() = nMaxPos;
            else
                aSel.Max() = nMaxPos;
        }
        pEdit->SetText( aNewText, aSel );
    }
}

// -----------------------------------------------------------------------

static xub_StrLen ImplPatternLeftPos( const ByteString& rEditMask, xub_StrLen nCursorPos )
{
    // Vorheriges Zeichen suchen, was kein Literal ist
    xub_StrLen nNewPos = nCursorPos;
    xub_StrLen nTempPos = nNewPos;
    while ( nTempPos )
    {
        if ( rEditMask.GetChar(nTempPos-1) != EDITMASK_LITERAL )
        {
            nNewPos = nTempPos-1;
            break;
        }
        nTempPos--;
    }
    return nNewPos;
}

// -----------------------------------------------------------------------

static xub_StrLen ImplPatternRightPos( const XubString& rStr, const ByteString& rEditMask,
                                       USHORT nFormatFlags, BOOL bSameMask,
                                       xub_StrLen nCursorPos )
{
    // Naechstes Zeichen suchen, was kein Literal ist
    xub_StrLen nNewPos = nCursorPos;
    xub_StrLen nTempPos = nNewPos;
    while ( nTempPos < rEditMask.Len() )
    {
        if ( rEditMask.GetChar(nTempPos+1) != EDITMASK_LITERAL )
        {
            nNewPos = nTempPos+1;
            break;
        }
        nTempPos++;
    }
    ImplPatternMaxPos( rStr, rEditMask, nFormatFlags, bSameMask, nCursorPos, nNewPos );
    return nNewPos;
}

// -----------------------------------------------------------------------

static BOOL ImplPatternProcessKeyInput( Edit* pEdit, const KeyEvent& rKEvt,
                                        const ByteString& rEditMask,
                                        const XubString& rLiteralMask,
                                        BOOL bStrictFormat,
                                        USHORT nFormatFlags,
                                        BOOL bSameMask,
                                        BOOL& rbInKeyInput )
{
    if ( !rEditMask.Len() || !bStrictFormat )
        return FALSE;

    Selection   aOldSel     = pEdit->GetSelection();
    KeyCode     aCode       = rKEvt.GetKeyCode();
    xub_Unicode cChar       = rKEvt.GetCharCode();
    USHORT      nKeyCode    = aCode.GetCode();
    BOOL        bShift      = aCode.IsShift();
    xub_StrLen  nCursorPos  = (xub_StrLen)aOldSel.Max();
    xub_StrLen  nNewPos;
    xub_StrLen  nTempPos;

    if ( nKeyCode && !aCode.IsMod1() && !aCode.IsMod2() )
    {
        if ( nKeyCode == KEY_LEFT )
        {
            Selection aSel( ImplPatternLeftPos( rEditMask, nCursorPos ) );
            if ( bShift )
                aSel.Min() = aOldSel.Min();
            pEdit->SetSelection( aSel );
            return TRUE;
        }
        else if ( nKeyCode == KEY_RIGHT )
        {
            // Hier nehmen wir Selectionsanfang als minimum, da falls durch
            // Focus alles selektiert ist, ist eine kleine Position schon
            // erlaubt.
            Selection aSel( aOldSel );
            aSel.Justify();
            nCursorPos = (xub_StrLen)aSel.Min();
            aSel.Max() = ImplPatternRightPos( pEdit->GetText(), rEditMask, nFormatFlags, bSameMask, nCursorPos );
            if ( bShift )
                aSel.Min() = aOldSel.Min();
            else
                aSel.Min() = aSel.Max();
            pEdit->SetSelection( aSel );
            return TRUE;
        }
        else if ( nKeyCode == KEY_HOME )
        {
            // Home ist Position des ersten nicht literalen Zeichens
            nNewPos = 0;
            while ( (nNewPos < rEditMask.Len()) &&
                    (rEditMask.GetChar(nNewPos) == EDITMASK_LITERAL) )
                nNewPos++;
            // Home sollte nicht nach rechts wandern
            if ( nCursorPos < nNewPos )
                nNewPos = nCursorPos;
            Selection aSel( nNewPos );
            if ( bShift )
                aSel.Min() = aOldSel.Min();
            pEdit->SetSelection( aSel );
            return TRUE;
        }
        else if ( nKeyCode == KEY_END )
        {
            // End ist die Position des letzten nicht literalen Zeichens
            nNewPos = rEditMask.Len();
            while ( nNewPos &&
                    (rEditMask.GetChar(nNewPos-1) == EDITMASK_LITERAL) )
                nNewPos--;
            // Hier nehmen wir Selectionsanfang als minimum, da falls durch
            // Focus alles selektiert ist, ist eine kleine Position schon
            // erlaubt.
            Selection aSel( aOldSel );
            aSel.Justify();
            nCursorPos = (xub_StrLen)aSel.Min();
            ImplPatternMaxPos( pEdit->GetText(), rEditMask, nFormatFlags, bSameMask, nCursorPos, nNewPos );
            aSel.Max() = nNewPos;
            if ( bShift )
                aSel.Min() = aOldSel.Min();
            else
                aSel.Min() = aSel.Max();
            pEdit->SetSelection( aSel );
            return TRUE;
        }
        else if ( (nKeyCode == KEY_BACKSPACE) || (nKeyCode == KEY_DELETE) )
        {
            XubString   aStr( pEdit->GetText() );
            XubString   aOldStr = aStr;
            Selection   aSel = aOldSel;

            aSel.Justify();
            nNewPos = (xub_StrLen)aSel.Min();

            // Wenn Selection, dann diese Loeschen
            if ( aSel.Len() )
            {
                if ( bSameMask )
                    aStr.Erase( (xub_StrLen)aSel.Min(), (xub_StrLen)aSel.Len() );
                else
                {
                    XubString aRep = rLiteralMask.Copy( (xub_StrLen)aSel.Min(), (xub_StrLen)aSel.Len() );
                    aStr.Replace( (xub_StrLen)aSel.Min(), aRep.Len(), aRep );
                }
            }
            else
            {
                if ( nKeyCode == KEY_BACKSPACE )
                {
                    nTempPos = nNewPos;
                    nNewPos = ImplPatternLeftPos( rEditMask, nTempPos );
                }
                else
                    nTempPos = ImplPatternRightPos( aStr, rEditMask, nFormatFlags, bSameMask, nNewPos );

                if ( nNewPos != nTempPos )
                {
                    if ( bSameMask )
                    {
                        if ( rEditMask.GetChar( nNewPos ) != EDITMASK_LITERAL )
                            aStr.Erase( nNewPos, 1 );
                    }
                    else
                    {
                        XubString aTempStr = rLiteralMask.Copy( nNewPos, 1 );
                        aStr.Replace( nNewPos, aTempStr.Len(), aTempStr );
                    }
                }
            }

            if ( aOldStr != aStr )
            {
                if ( bSameMask )
                    aStr = ImplPatternReformat( aStr, rEditMask, rLiteralMask, nFormatFlags );
                rbInKeyInput = TRUE;
                pEdit->SetText( aStr, Selection( nNewPos ) );
                pEdit->SetModifyFlag();
                pEdit->Modify();
                rbInKeyInput = FALSE;
            }
            else
                pEdit->SetSelection( Selection( nNewPos ) );

            return TRUE;
        }
        else if ( nKeyCode == KEY_INSERT )
        {
            // InsertModus kann man beim PatternField nur einstellen,
            // wenn Maske an jeder Eingabeposition die gleiche
            // ist
            if ( !bSameMask )
            {
                Sound::Beep();
                return TRUE;
            }
        }
    }

    if ( rKEvt.GetKeyCode().IsMod2() || (cChar < 32) || (cChar == 127) )
        return FALSE;

    Selection aSel = aOldSel;
    aSel.Justify();
    nNewPos = (xub_StrLen)aSel.Min();

    if ( nNewPos < rEditMask.Len() )
    {
        xub_Unicode cPattChar = ImplPatternChar( cChar, rEditMask.GetChar(nNewPos) );
        if ( cPattChar )
            cChar = cPattChar;
        else
        {
            // Wenn kein gueltiges Zeichen, dann testen wir, ob der
            // Anwender zum naechsten Literal springen wollte. Dies machen
            // wir nur, wenn er hinter einem Zeichen steht, damit
            // eingebene Literale die automatisch uebersprungenen wurden
            // nicht dazu fuehren, das der Anwender dann da steht, wo
            // er nicht stehen wollte.
            if ( nNewPos &&
                 (rEditMask.GetChar(nNewPos-1) != EDITMASK_LITERAL) &&
                 !aSel.Len() )
            {
                // Naechstes Zeichen suchen, was kein Literal ist
                nTempPos = nNewPos;
                while ( nTempPos < rEditMask.Len() )
                {
                    if ( rEditMask.GetChar(nTempPos) == EDITMASK_LITERAL )
                    {
                        // Gilt nur, wenn ein Literalzeichen vorhanden
                        if ( (rEditMask.GetChar(nTempPos+1) != EDITMASK_LITERAL ) &&
                             ImplKommaPointCharEqual( cChar, rLiteralMask.GetChar(nTempPos) ) )
                        {
                            nTempPos++;
                            ImplPatternMaxPos( pEdit->GetText(), rEditMask, nFormatFlags, bSameMask, nNewPos, nTempPos );
                            if ( nTempPos > nNewPos )
                            {
                                pEdit->SetSelection( Selection( nTempPos ) );
                                return TRUE;
                            }
                        }
                        break;
                    }
                    nTempPos++;
                }
            }

            cChar = 0;
        }
    }
    else
        cChar = 0;
    if ( cChar )
    {
        XubString   aStr = pEdit->GetText();
        BOOL        bError = FALSE;
        if ( bSameMask && pEdit->IsInsertMode() )
        {
            // Text um Spacezeichen und Literale am Ende kuerzen, bis zur
            // aktuellen Position
            xub_StrLen n = aStr.Len();
            while ( n && (n > nNewPos) )
            {
                if ( (aStr.GetChar( n-1 ) != ' ') &&
                     ((n > rEditMask.Len()) || (rEditMask.GetChar(n-1) != EDITMASK_LITERAL)) )
                    break;

                n--;
            }
            aStr.Erase( n );

            if ( aSel.Len() )
                aStr.Erase( (xub_StrLen)aSel.Min(), (xub_StrLen)aSel.Len() );

            if ( aStr.Len() < rEditMask.Len() )
            {
                // String evtl. noch bis Cursor-Position erweitern
                if ( aStr.Len() < nNewPos )
                    aStr += rLiteralMask.Copy( aStr.Len(), nNewPos-aStr.Len() );
                if ( nNewPos < aStr.Len() )
                    aStr.Insert( cChar, nNewPos );
                else if ( nNewPos < rEditMask.Len() )
                    aStr += cChar;
                aStr = ImplPatternReformat( aStr, rEditMask, rLiteralMask, nFormatFlags );
            }
            else
                bError = TRUE;
        }
        else
        {
            if ( aSel.Len() )
            {
                // Selection loeschen
                XubString aRep = rLiteralMask.Copy( (xub_StrLen)aSel.Min(), (xub_StrLen)aSel.Len() );
                aStr.Replace( (xub_StrLen)aSel.Min(), aRep.Len(), aRep );
            }

            if ( nNewPos < aStr.Len() )
                aStr.SetChar( nNewPos, cChar );
            else if ( nNewPos < rEditMask.Len() )
                aStr += cChar;
        }

        if ( bError )
            Sound::Beep();
        else
        {
            rbInKeyInput = TRUE;
            Selection aNewSel( ImplPatternRightPos( aStr, rEditMask, nFormatFlags, bSameMask, nNewPos ) );
            pEdit->SetText( aStr, aNewSel );
            pEdit->SetModifyFlag();
            pEdit->Modify();
            rbInKeyInput = FALSE;
        }
    }
    else
        Sound::Beep();

    return TRUE;
}

// -----------------------------------------------------------------------

void PatternFormatter::ImplSetMask( const ByteString& rEditMask,
                                    const XubString& rLiteralMask )
{
    maEditMask      = rEditMask;
    maLiteralMask   = rLiteralMask;
    mbSameMask      = TRUE;

    if ( maEditMask.Len() != maLiteralMask.Len() )
    {
        if ( maEditMask.Len() < maLiteralMask.Len() )
            maLiteralMask.Erase( maEditMask.Len() );
        else
            maLiteralMask.Expand( maEditMask.Len(), ' ' );
    }

    // StrictModus erlaubt nur Input-Mode, wenn als Maske nur
    // gleiche Zeichen zugelassen werden und als Vorgabe nur
    // Spacezeichen vorgegeben werden, die durch die Maske
    // nicht zugelassen sind
    xub_StrLen  i = 0;
    sal_Char    c = 0;
    while ( i < rEditMask.Len() )
    {
        sal_Char cTemp = rEditMask.GetChar( i );
        if ( cTemp != EDITMASK_LITERAL )
        {
            if ( (cTemp == EDITMASK_ALLCHAR) ||
                 (cTemp == EDITMASK_UPPERALLCHAR) ||
                 (cTemp == EDITMASK_NUMSPACE) )
            {
                mbSameMask = FALSE;
                break;
            }
            if ( i < rLiteralMask.Len() )
            {
                if ( rLiteralMask.GetChar( i ) != ' ' )
                {
                    mbSameMask = FALSE;
                    break;
                }
            }
            if ( !c )
                c = cTemp;
            if ( cTemp != c )
            {
                mbSameMask = FALSE;
                break;
            }
        }
        i++;
    }
}

// -----------------------------------------------------------------------

PatternFormatter::PatternFormatter()
{
    mnFormatFlags       = 0;
    mbSameMask          = TRUE;
    mbInPattKeyInput    = FALSE;
}

// -----------------------------------------------------------------------

void PatternFormatter::ImplLoadRes( const ResId& rResId )
{
    ByteString  aEditMask;
    XubString   aLiteralMask;
    ResMgr*     pMgr = rResId.GetResMgr();
    if( pMgr )
    {
        ULONG       nMask = pMgr->ReadLong();
    
        if ( PATTERNFORMATTER_STRICTFORMAT & nMask )
            SetStrictFormat( (BOOL)pMgr->ReadShort() );
    
        if ( PATTERNFORMATTER_EDITMASK & nMask )
            aEditMask = ByteString( pMgr->ReadString(), RTL_TEXTENCODING_ASCII_US );
    
        if ( PATTERNFORMATTER_LITTERALMASK & nMask )
            aLiteralMask = pMgr->ReadString();
    
        if ( (PATTERNFORMATTER_EDITMASK | PATTERNFORMATTER_LITTERALMASK) & nMask )
            ImplSetMask( aEditMask, aLiteralMask );
    }
}

// -----------------------------------------------------------------------

PatternFormatter::~PatternFormatter()
{
}

// -----------------------------------------------------------------------

void PatternFormatter::SetMask( const ByteString& rEditMask,
                                const XubString& rLiteralMask )
{
    ImplSetMask( rEditMask, rLiteralMask );
    ReformatAll();
}

// -----------------------------------------------------------------------

void PatternFormatter::SetString( const XubString& rStr )
{
    maFieldString = rStr;
    if ( GetField() )
    {
        GetField()->SetText( rStr );
        MarkToBeReformatted( FALSE );
    }
}

// -----------------------------------------------------------------------

XubString PatternFormatter::GetString() const
{
    if ( !GetField() )
        return ImplGetSVEmptyStr();
    else
        return ImplPatternReformat( GetField()->GetText(), maEditMask, maLiteralMask, mnFormatFlags );
}

// -----------------------------------------------------------------------

void PatternFormatter::Reformat()
{
    if ( GetField() )
    {
        ImplSetText( ImplPatternReformat( GetField()->GetText(), maEditMask, maLiteralMask, mnFormatFlags ) );
        if ( !mbSameMask && IsStrictFormat() && !GetField()->IsReadOnly() )
            GetField()->SetInsertMode( FALSE );
    }
}

// -----------------------------------------------------------------------

void PatternFormatter::SelectFixedFont()
{
    if ( GetField() )
    {
        Font aFont = OutputDevice::GetDefaultFont( DEFAULTFONT_FIXED, Application::GetSettings().GetLanguage(), 0 );
        Font aControlFont;
        aControlFont.SetName( aFont.GetName() );
        aControlFont.SetFamily( aFont.GetFamily() );
        aControlFont.SetPitch( aFont.GetPitch() );
        GetField()->SetControlFont( aControlFont );
    }
}

// -----------------------------------------------------------------------

PatternField::PatternField( Window* pParent, WinBits nWinStyle ) :
    SpinField( pParent, nWinStyle )
{
    SetField( this );
    Reformat();
}

// -----------------------------------------------------------------------

PatternField::PatternField( Window* pParent, const ResId& rResId ) :
    SpinField( WINDOW_PATTERNFIELD )
{
    rResId.SetRT( RSC_PATTERNFIELD );
    WinBits nStyle = ImplInitRes( rResId );
    ImplInit( pParent, nStyle );
    SetField( this );
    SpinField::ImplLoadRes( rResId );
    PatternFormatter::ImplLoadRes( ResId( (RSHEADER_TYPE *)GetClassRes(), *rResId.GetResMgr() ) );
    Reformat();

    if ( !(nStyle & WB_HIDE ) )
        Show();
}

// -----------------------------------------------------------------------

PatternField::~PatternField()
{
}

// -----------------------------------------------------------------------

long PatternField::PreNotify( NotifyEvent& rNEvt )
{
    if ( (rNEvt.GetType() == EVENT_KEYINPUT) && !rNEvt.GetKeyEvent()->GetKeyCode().IsMod2() )
    {
        if ( ImplPatternProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), GetEditMask(), GetLiteralMask(),
                                         IsStrictFormat(), GetFormatFlags(),
                                         ImplIsSameMask(), ImplGetInPattKeyInput() ) )
            return 1;
    }

    return SpinField::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

long PatternField::Notify( NotifyEvent& rNEvt )
{
    if ( rNEvt.GetType() == EVENT_GETFOCUS )
        MarkToBeReformatted( FALSE );
    else if ( rNEvt.GetType() == EVENT_LOSEFOCUS )
    {
        if ( MustBeReformatted() && (GetText().Len() || !IsEmptyFieldValueEnabled()) )
            Reformat();
    }

    return SpinField::Notify( rNEvt );
}

// -----------------------------------------------------------------------

void PatternField::Modify()
{
    if ( !ImplGetInPattKeyInput() )
    {
        if ( IsStrictFormat() )
            ImplPatternProcessStrictModify( GetField(), GetEditMask(), GetLiteralMask(), GetFormatFlags(), ImplIsSameMask() );
        else
            MarkToBeReformatted( TRUE );
    }

    SpinField::Modify();
}

// -----------------------------------------------------------------------

PatternBox::PatternBox( Window* pParent, WinBits nWinStyle ) :
    ComboBox( pParent, nWinStyle )
{
    SetField( this );
    Reformat();
}

// -----------------------------------------------------------------------

PatternBox::PatternBox( Window* pParent, const ResId& rResId ) :
    ComboBox( WINDOW_PATTERNBOX )
{
    rResId.SetRT( RSC_PATTERNBOX );
    WinBits nStyle = ImplInitRes( rResId );
    ImplInit( pParent, nStyle );

    SetField( this );
    ComboBox::ImplLoadRes( rResId );
    PatternFormatter::ImplLoadRes( ResId( (RSHEADER_TYPE *)GetClassRes(), *rResId.GetResMgr() ) );
    Reformat();

    if ( !(nStyle & WB_HIDE ) )
        Show();
}

// -----------------------------------------------------------------------

PatternBox::~PatternBox()
{
}

// -----------------------------------------------------------------------

long PatternBox::PreNotify( NotifyEvent& rNEvt )
{
    if ( (rNEvt.GetType() == EVENT_KEYINPUT) && !rNEvt.GetKeyEvent()->GetKeyCode().IsMod2() )
    {
        if ( ImplPatternProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), GetEditMask(), GetLiteralMask(),
                                         IsStrictFormat(), GetFormatFlags(),
                                         ImplIsSameMask(), ImplGetInPattKeyInput() ) )
            return 1;
    }

    return ComboBox::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

long PatternBox::Notify( NotifyEvent& rNEvt )
{
    if ( rNEvt.GetType() == EVENT_GETFOCUS )
        MarkToBeReformatted( FALSE );
    else if ( rNEvt.GetType() == EVENT_LOSEFOCUS )
    {
        if ( MustBeReformatted() && (GetText().Len() || !IsEmptyFieldValueEnabled()) )
            Reformat();
    }

    return ComboBox::Notify( rNEvt );
}

// -----------------------------------------------------------------------

void PatternBox::Modify()
{
    if ( !ImplGetInPattKeyInput() )
    {
        if ( IsStrictFormat() )
            ImplPatternProcessStrictModify( GetField(), GetEditMask(), GetLiteralMask(), GetFormatFlags(), ImplIsSameMask() );
        else
            MarkToBeReformatted( TRUE );
    }

    ComboBox::Modify();
}

// -----------------------------------------------------------------------

void PatternBox::ReformatAll()
{
    XubString aStr;
    SetUpdateMode( FALSE );
    USHORT nEntryCount = GetEntryCount();
    for ( USHORT i=0; i < nEntryCount; i++ )
    {
        aStr = ImplPatternReformat( GetEntry( i ), GetEditMask(), GetLiteralMask(), GetFormatFlags() );
        RemoveEntry( i );
        InsertEntry( aStr, i );
    }
    PatternFormatter::Reformat();
    SetUpdateMode( TRUE );
}

// -----------------------------------------------------------------------

void PatternBox::InsertString( const XubString& rStr, USHORT nPos )
{
    ComboBox::InsertEntry( ImplPatternReformat( rStr, GetEditMask(), GetLiteralMask(), GetFormatFlags() ), nPos );
}

// -----------------------------------------------------------------------

void PatternBox::RemoveString( const XubString& rStr )
{
    ComboBox::RemoveEntry( ImplPatternReformat( rStr, GetEditMask(), GetLiteralMask(), GetFormatFlags() ) );
}

// -----------------------------------------------------------------------

XubString PatternBox::GetString( USHORT nPos ) const
{
    return ImplPatternReformat( ComboBox::GetEntry( nPos ), GetEditMask(), GetLiteralMask(), GetFormatFlags() );
}

// -----------------------------------------------------------------------

USHORT PatternBox::GetStringPos( const XubString& rStr ) const
{
    return ComboBox::GetEntryPos( ImplPatternReformat( rStr, GetEditMask(), GetLiteralMask(), GetFormatFlags() ) );
}

// =======================================================================

static ExtDateFieldFormat ImplGetExtFormat( DateFormat eOld )
{
    switch( eOld )
    {
        case DMY:   return XTDATEF_SHORT_DDMMYY;
        case MDY:   return XTDATEF_SHORT_MMDDYY;
        default:    return XTDATEF_SHORT_YYMMDD;
    }
}

// -----------------------------------------------------------------------

static USHORT ImplCutNumberFromString( XubString& rStr )
{
    // Nach Zahl suchen
    while ( rStr.Len() && !(rStr.GetChar( 0 ) >= '0' && rStr.GetChar( 0 ) <= '9') )
        rStr.Erase( 0, 1 );
    if ( !rStr.Len() )
        return 0;
    XubString aNumStr;
    while ( rStr.Len() && (rStr.GetChar( 0 ) >= '0' && rStr.GetChar( 0 ) <= '9') )
    {
        aNumStr.Insert( rStr.GetChar( 0 ) );
        rStr.Erase( 0, 1 );
    }
    return (USHORT)aNumStr.ToInt32();
}

// -----------------------------------------------------------------------

static BOOL ImplCutMonthName( XubString& rStr, const XubString& _rLookupMonthName )
{
    USHORT nPos = rStr.Search( _rLookupMonthName );
    if ( nPos != STRING_NOTFOUND )
    {
        rStr.Erase( 0, nPos + _rLookupMonthName.Len() );
        return TRUE;
    }
    return FALSE;
}

// -----------------------------------------------------------------------

static USHORT ImplCutMonthFromString( XubString& rStr, const CalendarWrapper& rCalendarWrapper )
{
    // search for a month' name
    for ( USHORT i=1; i <= 12; i++ )
    {
        String aMonthName = rCalendarWrapper.getMonths()[i-1].FullName;
        // long month name?
        if ( ImplCutMonthName( rStr, aMonthName ) )
            return i;

        // short month name?
        String aAbbrevMonthName = rCalendarWrapper.getMonths()[i-1].AbbrevName;
        if ( ImplCutMonthName( rStr, aAbbrevMonthName ) )
            return i;
    }

    return ImplCutNumberFromString( rStr );
}

// -----------------------------------------------------------------------

static String ImplGetDateSep( const LocaleDataWrapper& rLocaleDataWrapper, ExtDateFieldFormat eFormat )
{
    String aDateSep = rLocaleDataWrapper.getDateSep();

    if ( ( eFormat == XTDATEF_SHORT_YYMMDD_DIN5008 ) || ( eFormat == XTDATEF_SHORT_YYYYMMDD_DIN5008 ) )
        aDateSep = String( RTL_CONSTASCII_USTRINGPARAM( "-" ) );

    return aDateSep;
}

static BOOL ImplDateProcessKeyInput( Edit*, const KeyEvent& rKEvt, ExtDateFieldFormat eFormat,
                                     const LocaleDataWrapper& rLocaleDataWrapper  )
{
    xub_Unicode cChar = rKEvt.GetCharCode();
    USHORT nGroup = rKEvt.GetKeyCode().GetGroup();
    if ( (nGroup == KEYGROUP_FKEYS) || (nGroup == KEYGROUP_CURSOR) ||
         (nGroup == KEYGROUP_MISC)||
         ((cChar >= '0') && (cChar <= '9')) ||
         (cChar == ImplGetDateSep( rLocaleDataWrapper, eFormat ).GetChar(0) ) )
        return FALSE;
    else
        return TRUE;
}

// -----------------------------------------------------------------------

static BOOL ImplDateGetValue( const XubString& rStr, Date& rDate, ExtDateFieldFormat eDateFormat,
                              const LocaleDataWrapper& rLocaleDataWrapper, const CalendarWrapper& rCalendarWrapper,
                              const AllSettings& rSettings )
{
    USHORT nDay = 0;
    USHORT nMonth = 0;
    USHORT nYear = 0;
    BOOL bYear = TRUE;
    BOOL bError = FALSE;
    String aStr( rStr );

    if ( eDateFormat == XTDATEF_SYSTEM_LONG )
    {
        DateFormat eFormat = rLocaleDataWrapper.getLongDateFormat();
        switch( eFormat )
        {
            case MDY:
                nMonth = ImplCutMonthFromString( aStr, rCalendarWrapper );
                nDay = ImplCutNumberFromString( aStr );
                nYear  = ImplCutNumberFromString( aStr );
                break;
            case DMY:
                nDay = ImplCutNumberFromString( aStr );
                nMonth = ImplCutMonthFromString( aStr, rCalendarWrapper );
                nYear  = ImplCutNumberFromString( aStr );
                break;
            case YMD:
            default:
                nYear = ImplCutNumberFromString( aStr );
                nMonth = ImplCutMonthFromString( aStr, rCalendarWrapper );
                nDay  = ImplCutNumberFromString( aStr );
                break;
        }
    }
    else
    {
        // Check if year is present:
        String aDateSep = ImplGetDateSep( rLocaleDataWrapper, eDateFormat );
        USHORT nSepPos = aStr.Search( aDateSep );
        if ( nSepPos == STRING_NOTFOUND )
            return FALSE;
        nSepPos = aStr.Search( aDateSep, nSepPos+1 );
        if ( ( nSepPos == STRING_NOTFOUND ) || ( nSepPos == (aStr.Len()-1) ) )
        {
            bYear = FALSE;
            nYear = Date().GetYear();
        }

        const sal_Unicode* pBuf = aStr.GetBuffer();
        ImplSkipDelimiters( pBuf );

        switch ( eDateFormat )
        {
            case XTDATEF_SHORT_DDMMYY:
            case XTDATEF_SHORT_DDMMYYYY:
            {
                nDay = ImplGetNum( pBuf, bError );
                ImplSkipDelimiters( pBuf );
                nMonth = ImplGetNum( pBuf, bError );
                ImplSkipDelimiters( pBuf );
                if ( bYear )
                    nYear = ImplGetNum( pBuf, bError );
            }
            break;
            case XTDATEF_SHORT_MMDDYY:
            case XTDATEF_SHORT_MMDDYYYY:
            {
                nMonth = ImplGetNum( pBuf, bError );
                ImplSkipDelimiters( pBuf );
                nDay = ImplGetNum( pBuf, bError );
                ImplSkipDelimiters( pBuf );
                if ( bYear )
                    nYear = ImplGetNum( pBuf, bError );
            }
            break;
            case XTDATEF_SHORT_YYMMDD:
            case XTDATEF_SHORT_YYYYMMDD:
            case XTDATEF_SHORT_YYMMDD_DIN5008:
            case XTDATEF_SHORT_YYYYMMDD_DIN5008:
            {
                if ( bYear )
                    nYear = ImplGetNum( pBuf, bError );
                ImplSkipDelimiters( pBuf );
                nMonth = ImplGetNum( pBuf, bError );
                ImplSkipDelimiters( pBuf );
                nDay = ImplGetNum( pBuf, bError );
            }
            break;

            default:
            {
                DBG_ERROR( "DateFormat???" );
            }
        }
    }

    if ( bError || !nDay || !nMonth )
        return FALSE;

    Date aNewDate( nDay, nMonth, nYear );
    DateFormatter::ExpandCentury( aNewDate, rSettings.GetMiscSettings().GetTwoDigitYearStart() );
    if ( aNewDate.IsValid() )
    {
        rDate = aNewDate;
        return TRUE;
    }
    return FALSE;
}

// -----------------------------------------------------------------------

BOOL DateFormatter::ImplDateReformat( const XubString& rStr, XubString& rOutStr, const AllSettings& rSettings )
{
    Date aDate( 0, 0, 0 );
    if ( !ImplDateGetValue( rStr, aDate, GetExtDateFormat(TRUE), ImplGetLocaleDataWrapper(), GetCalendarWrapper(), GetFieldSettings() ) )
        return TRUE;

    Date aTempDate = aDate;
    if ( aTempDate > GetMax() )
        aTempDate = GetMax();
    else if ( aTempDate < GetMin() )
        aTempDate = GetMin();

    if ( GetErrorHdl().IsSet() && (aDate != aTempDate) )
    {
        maCorrectedDate = aTempDate;
        if( !GetErrorHdl().Call( this ) )
        {
            maCorrectedDate = Date();
            return FALSE;
        }
        else
            maCorrectedDate = Date();
    }

    rOutStr = ImplGetDateAsText( aTempDate, rSettings );

    return TRUE;
}

// -----------------------------------------------------------------------

XubString DateFormatter::ImplGetDateAsText( const Date& rDate,
                                            const AllSettings& rSettings ) const
{
    BOOL bShowCentury = FALSE;
    switch ( GetExtDateFormat() )
    {
        case XTDATEF_SYSTEM_SHORT_YYYY:
        case XTDATEF_SYSTEM_LONG:
        case XTDATEF_SHORT_DDMMYYYY:
        case XTDATEF_SHORT_MMDDYYYY:
        case XTDATEF_SHORT_YYYYMMDD:
        case XTDATEF_SHORT_YYYYMMDD_DIN5008:
        {
            bShowCentury = TRUE;
        }
        break;
        default:
        {
            bShowCentury = FALSE;
        }
    }

    if ( !bShowCentury )
    {
        // Check if I have to use force showing the century
        USHORT nTwoDigitYearStart = rSettings.GetMiscSettings().GetTwoDigitYearStart();
        USHORT nYear = rDate.GetYear();

        // Wenn Jahr nicht im 2stelligen Grenzbereich liegt,
        if ( (nYear < nTwoDigitYearStart) || (nYear >= nTwoDigitYearStart+100) )
            bShowCentury = TRUE;
    }

    sal_Unicode aBuf[128];
    sal_Unicode* pBuf = aBuf;

    String aDateSep = ImplGetDateSep( ImplGetLocaleDataWrapper(), GetExtDateFormat( TRUE ) );
    USHORT nDay = rDate.GetDay();
    USHORT nMonth = rDate.GetMonth();
    USHORT nYear = rDate.GetYear();
    USHORT nYearLen = bShowCentury ? 4 : 2;

    if ( !bShowCentury )
        nYear %= 100;

    switch ( GetExtDateFormat( TRUE ) )
    {
        case XTDATEF_SYSTEM_LONG:
        {
            return ImplGetLocaleDataWrapper().getLongDate( rDate, GetCalendarWrapper(), 1, FALSE, 1, !bShowCentury );
        }
        case XTDATEF_SHORT_DDMMYY:
        case XTDATEF_SHORT_DDMMYYYY:
        {
            pBuf = ImplAddNum( pBuf, nDay, 2 );
            pBuf = ImplAddString( pBuf, aDateSep );
            pBuf = ImplAddNum( pBuf, nMonth, 2 );
            pBuf = ImplAddString( pBuf, aDateSep );
            pBuf = ImplAddNum( pBuf, nYear, nYearLen );
        }
        break;
        case XTDATEF_SHORT_MMDDYY:
        case XTDATEF_SHORT_MMDDYYYY:
        {
            pBuf = ImplAddNum( pBuf, nMonth, 2 );
            pBuf = ImplAddString( pBuf, aDateSep );
            pBuf = ImplAddNum( pBuf, nDay, 2 );
            pBuf = ImplAddString( pBuf, aDateSep );
            pBuf = ImplAddNum( pBuf, nYear, nYearLen );
        }
        break;
        case XTDATEF_SHORT_YYMMDD:
        case XTDATEF_SHORT_YYYYMMDD:
        case XTDATEF_SHORT_YYMMDD_DIN5008:
        case XTDATEF_SHORT_YYYYMMDD_DIN5008:
        {
            pBuf = ImplAddNum( pBuf, nYear, nYearLen );
            pBuf = ImplAddString( pBuf, aDateSep );
            pBuf = ImplAddNum( pBuf, nMonth, 2 );
            pBuf = ImplAddString( pBuf, aDateSep );
            pBuf = ImplAddNum( pBuf, nDay, 2 );
        }
        break;
        default:
        {
            DBG_ERROR( "DateFormat???" );
        }
    }

    return String( aBuf, (xub_StrLen)(ULONG)(pBuf-aBuf) );
}

// -----------------------------------------------------------------------

static void ImplDateIncrementDay( Date& rDate, BOOL bUp )
{
    DateFormatter::ExpandCentury( rDate );

    if ( bUp )
    {
        if ( (rDate.GetDay() != 31) || (rDate.GetMonth() != 12) || (rDate.GetYear() != 9999) )
            rDate++;
    }
    else
    {
        if ( (rDate.GetDay() != 1 ) || (rDate.GetMonth() != 1) || (rDate.GetYear() != 0) )
            rDate--;
    }
}

// -----------------------------------------------------------------------

static void ImplDateIncrementMonth( Date& rDate, BOOL bUp )
{
    DateFormatter::ExpandCentury( rDate );

    USHORT nMonth = rDate.GetMonth();
    USHORT nYear = rDate.GetYear();
    if ( bUp )
    {
        if ( (nMonth == 12) && (nYear < 9999) )
        {
            rDate.SetMonth( 1 );
            rDate.SetYear( nYear + 1 );
        }
        else
        {
            if ( nMonth < 12 )
                rDate.SetMonth( nMonth + 1 );
        }
    }
    else
    {
        if ( (nMonth == 1) && (nYear > 0) )
        {
            rDate.SetMonth( 12 );
            rDate.SetYear( nYear - 1 );
        }
        else
        {
            if ( nMonth > 1 )
                rDate.SetMonth( nMonth - 1 );
        }
    }

    USHORT nDaysInMonth = rDate.GetDaysInMonth();
    if ( rDate.GetDay() > nDaysInMonth )
        rDate.SetDay( nDaysInMonth );
}

// -----------------------------------------------------------------------

static void ImplDateIncrementYear( Date& rDate, BOOL bUp )
{
    DateFormatter::ExpandCentury( rDate );

    USHORT nYear = rDate.GetYear();
    if ( bUp )
    {
        if ( nYear < 9999 )
            rDate.SetYear( nYear + 1 );
    }
    else
    {
        if ( nYear > 0 )
            rDate.SetYear( nYear - 1 );
    }
}

// -----------------------------------------------------------------------
BOOL DateFormatter::ImplAllowMalformedInput() const
{
    return !IsEnforceValidValue();
}

// -----------------------------------------------------------------------

void DateField::ImplDateSpinArea( BOOL bUp )
{
    // Wenn alles selektiert ist, Tage hochzaehlen
    if ( GetField() )
    {
        Date aDate( GetDate() );
        Selection aSelection = GetField()->GetSelection();
        aSelection.Justify();
        XubString aText( GetText() );
        if ( (xub_StrLen)aSelection.Len() == aText.Len() )
            ImplDateIncrementDay( aDate, bUp );
        else
        {
            xub_StrLen nDateArea = 0;

            ExtDateFieldFormat eFormat = GetExtDateFormat( TRUE );
            if ( eFormat == XTDATEF_SYSTEM_LONG )
            {
                eFormat = ImplGetExtFormat( ImplGetLocaleDataWrapper().getLongDateFormat() );
                nDateArea = 1;
            }
            else
            {
                // Area suchen
                xub_StrLen nPos = 0;
                String aDateSep = ImplGetDateSep( ImplGetLocaleDataWrapper(), eFormat );
                for ( xub_StrLen i = 1; i <= 3; i++ )
                {
                    nPos = aText.Search( aDateSep, nPos );
                    if ( nPos >= (USHORT)aSelection.Max() )
                    {
                        nDateArea = i;
                        break;
                    }
                    else
                        nPos++;
                }
            }
                

            switch( eFormat )
            {
                case XTDATEF_SHORT_MMDDYY:
                case XTDATEF_SHORT_MMDDYYYY:
                switch( nDateArea )
                {
                    case 1: ImplDateIncrementMonth( aDate, bUp );
                            break;
                    case 2: ImplDateIncrementDay( aDate, bUp );
                            break;
                    case 3: ImplDateIncrementYear( aDate, bUp );
                            break;
                }
                break;
                case XTDATEF_SHORT_DDMMYY:
                case XTDATEF_SHORT_DDMMYYYY:
                switch( nDateArea )
                {
                    case 1: ImplDateIncrementDay( aDate, bUp );
                            break;
                    case 2: ImplDateIncrementMonth( aDate, bUp );
                            break;
                    case 3: ImplDateIncrementYear( aDate, bUp );
                            break;
                }
                break;
                case XTDATEF_SHORT_YYMMDD:
                case XTDATEF_SHORT_YYYYMMDD:
                case XTDATEF_SHORT_YYMMDD_DIN5008:
                case XTDATEF_SHORT_YYYYMMDD_DIN5008:
                switch( nDateArea )
                {
                    case 1: ImplDateIncrementYear( aDate, bUp );
                            break;
                    case 2: ImplDateIncrementMonth( aDate, bUp );
                            break;
                    case 3: ImplDateIncrementDay( aDate, bUp );
                            break;
                }
                break;
                default:
                    DBG_ERROR( "invalid conversion" );
                    break;
            }
        }

        ImplNewFieldValue( aDate );
    }
}

// -----------------------------------------------------------------------

void DateFormatter::ImplInit()
{
    mbLongFormat        = FALSE;
    mbShowDateCentury   = TRUE;
    mpCalendarWrapper   = NULL;
    mnDateFormat        = 0xFFFF;
    mnExtDateFormat     = XTDATEF_SYSTEM_SHORT;
}

// -----------------------------------------------------------------------

DateFormatter::DateFormatter() :
    maFieldDate( 0 ),
    maLastDate( 0 ),
    maMin( 1, 1, 1900 ),
    maMax( 31, 12, 2200 ),
    mbEnforceValidValue( TRUE )
{
    ImplInit();
}

// -----------------------------------------------------------------------

void DateFormatter::ImplLoadRes( const ResId& rResId )
{
    ResMgr*     pMgr = rResId.GetResMgr();
    if( pMgr )
    {
        ULONG       nMask = pMgr->ReadLong();
    
        if ( DATEFORMATTER_MIN & nMask )
        {
            maMin = Date( ResId( (RSHEADER_TYPE *)pMgr->GetClass(), *pMgr ) );
            pMgr->Increment( pMgr->GetObjSize( (RSHEADER_TYPE*)pMgr->GetClass() ) );
        }
        if ( DATEFORMATTER_MAX & nMask )
        {
            maMax = Date( ResId( (RSHEADER_TYPE *)pMgr->GetClass(), *pMgr ) );
            pMgr->Increment( pMgr->GetObjSize( (RSHEADER_TYPE*)pMgr->GetClass() ) );
        }
        if ( DATEFORMATTER_LONGFORMAT & nMask )
            mbLongFormat = (BOOL)pMgr->ReadShort();
    
        if ( DATEFORMATTER_STRICTFORMAT & nMask )
            SetStrictFormat( (BOOL)pMgr->ReadShort() );
    
        if ( DATEFORMATTER_VALUE & nMask )
        {
            maFieldDate = Date( ResId( (RSHEADER_TYPE *)pMgr->GetClass(), *pMgr ) );
            pMgr->Increment( pMgr->GetObjSize( (RSHEADER_TYPE*)pMgr->GetClass() ) );
            if ( maFieldDate > maMax )
                maFieldDate = maMax;
            if ( maFieldDate < maMin )
                maFieldDate = maMin;
            maLastDate = maFieldDate;
        }
    }
}

// -----------------------------------------------------------------------

DateFormatter::~DateFormatter()
{
    delete mpCalendarWrapper;
    mpCalendarWrapper = NULL;
}

// -----------------------------------------------------------------------

void DateFormatter::SetLocale( const ::com::sun::star::lang::Locale& rLocale )
{
    delete mpCalendarWrapper;
    mpCalendarWrapper = NULL;
    FormatterBase::SetLocale( rLocale );
}


// -----------------------------------------------------------------------

CalendarWrapper& DateFormatter::GetCalendarWrapper() const
{
    if ( !mpCalendarWrapper )
    {
        ((DateFormatter*)this)->mpCalendarWrapper = new CalendarWrapper( vcl::unohelper::GetMultiServiceFactory() );
        mpCalendarWrapper->loadDefaultCalendar( GetLocale() );
    }

    return *mpCalendarWrapper;
}

// -----------------------------------------------------------------------

void DateFormatter::SetExtDateFormat( ExtDateFieldFormat eFormat )
{
    mnExtDateFormat = eFormat;
    ReformatAll();
}

// -----------------------------------------------------------------------

ExtDateFieldFormat DateFormatter::GetExtDateFormat( BOOL bResolveSystemFormat ) const
{
    ExtDateFieldFormat eDateFormat = (ExtDateFieldFormat)mnExtDateFormat;

    if ( bResolveSystemFormat && ( eDateFormat <= XTDATEF_SYSTEM_SHORT_YYYY ) )
    {
        BOOL bShowCentury = (eDateFormat == XTDATEF_SYSTEM_SHORT_YYYY);
        switch ( ImplGetLocaleDataWrapper().getDateFormat() )
        {
            case DMY:   eDateFormat = bShowCentury ? XTDATEF_SHORT_DDMMYYYY : XTDATEF_SHORT_DDMMYY;
                        break;
            case MDY:   eDateFormat = bShowCentury ? XTDATEF_SHORT_MMDDYYYY : XTDATEF_SHORT_MMDDYY;
                        break;
            default:    eDateFormat = bShowCentury ? XTDATEF_SHORT_YYYYMMDD : XTDATEF_SHORT_YYMMDD;

        }
    }

    return eDateFormat;
}

// -----------------------------------------------------------------------

void DateFormatter::ReformatAll()
{
    Reformat();
}

// -----------------------------------------------------------------------

void DateFormatter::SetMin( const Date& rNewMin )
{
    maMin = rNewMin;
    if ( !IsEmptyFieldValue() )
        ReformatAll();
}

// -----------------------------------------------------------------------

void DateFormatter::SetMax( const Date& rNewMax )
{
    maMax = rNewMax;
    if ( !IsEmptyFieldValue() )
        ReformatAll();
}

// -----------------------------------------------------------------------

void DateFormatter::SetLongFormat( BOOL bLong )
{
    mbLongFormat = bLong;

    // #91913# Remove LongFormat and DateShowCentury - redundant
    if ( bLong )
    {
        SetExtDateFormat( XTDATEF_SYSTEM_LONG );
    }
    else
    {
        if( mnExtDateFormat == XTDATEF_SYSTEM_LONG )
            SetExtDateFormat( XTDATEF_SYSTEM_SHORT );
    }

    ReformatAll();
}

// -----------------------------------------------------------------------

void DateFormatter::SetShowDateCentury( BOOL bShowDateCentury )
{
    mbShowDateCentury = bShowDateCentury;

    // #91913# Remove LongFormat and DateShowCentury - redundant
    if ( bShowDateCentury )
    {
        switch ( GetExtDateFormat() )
        {
            case XTDATEF_SYSTEM_SHORT:
            case XTDATEF_SYSTEM_SHORT_YY:
                SetExtDateFormat( XTDATEF_SYSTEM_SHORT_YYYY );  break;
            case XTDATEF_SHORT_DDMMYY:
                SetExtDateFormat( XTDATEF_SHORT_DDMMYYYY );     break;
            case XTDATEF_SHORT_MMDDYY:
                SetExtDateFormat( XTDATEF_SHORT_MMDDYYYY );     break;
            case XTDATEF_SHORT_YYMMDD:
                SetExtDateFormat( XTDATEF_SHORT_YYYYMMDD );     break;
            case XTDATEF_SHORT_YYMMDD_DIN5008:
                SetExtDateFormat( XTDATEF_SHORT_YYYYMMDD_DIN5008 ); break;
            default:
                ;
        }
    }
    else
    {
        switch ( GetExtDateFormat() )
        {
            case XTDATEF_SYSTEM_SHORT:
            case XTDATEF_SYSTEM_SHORT_YYYY:
                SetExtDateFormat( XTDATEF_SYSTEM_SHORT_YY );    break;
            case XTDATEF_SHORT_DDMMYYYY:
                SetExtDateFormat( XTDATEF_SHORT_DDMMYY );       break;
            case XTDATEF_SHORT_MMDDYYYY:
                SetExtDateFormat( XTDATEF_SHORT_MMDDYY );       break;
            case XTDATEF_SHORT_YYYYMMDD:
                SetExtDateFormat( XTDATEF_SHORT_YYMMDD );       break;
            case XTDATEF_SHORT_YYYYMMDD_DIN5008:
                SetExtDateFormat( XTDATEF_SHORT_YYMMDD_DIN5008 );  break;
            default:
                ;
        }
    }

    ReformatAll();
}

// -----------------------------------------------------------------------

void DateFormatter::SetDate( const Date& rNewDate )
{
    SetUserDate( rNewDate );
    maFieldDate = maLastDate;
    maLastDate = GetDate();
}

// -----------------------------------------------------------------------

void DateFormatter::SetUserDate( const Date& rNewDate )
{
    ImplSetUserDate( rNewDate );
}

// -----------------------------------------------------------------------

void DateFormatter::ImplSetUserDate( const Date& rNewDate, Selection* pNewSelection )
{
    Date aNewDate = rNewDate;
    if ( aNewDate > maMax )
        aNewDate = maMax;
    else if ( aNewDate < maMin )
        aNewDate = maMin;
    maLastDate = aNewDate;

    if ( GetField() )
        ImplSetText( ImplGetDateAsText( aNewDate, GetFieldSettings() ), pNewSelection );
}

// -----------------------------------------------------------------------

void DateFormatter::ImplNewFieldValue( const Date& rDate )
{
    if ( GetField() )
    {
        Selection aSelection = GetField()->GetSelection();
        aSelection.Justify();
        XubString aText = GetField()->GetText();
        // Wenn bis ans Ende selektiert war, soll das auch so bleiben...
        if ( (xub_StrLen)aSelection.Max() == aText.Len() )
        {
            if ( !aSelection.Len() )
                aSelection.Min() = SELECTION_MAX;
            aSelection.Max() = SELECTION_MAX;
        }

        Date aOldLastDate  = maLastDate;
        ImplSetUserDate( rDate, &aSelection );
        maLastDate = aOldLastDate;

        // Modify am Edit wird nur bei KeyInput gesetzt...
        if ( GetField()->GetText() != aText )
        {
            GetField()->SetModifyFlag();
            GetField()->Modify();
        }
    }
}

// -----------------------------------------------------------------------

Date DateFormatter::GetDate() const
{
    Date aDate( 0, 0, 0 );

    if ( GetField() )
    {
        if ( ImplDateGetValue( GetField()->GetText(), aDate, GetExtDateFormat(TRUE), ImplGetLocaleDataWrapper(), GetCalendarWrapper(), GetFieldSettings() ) )
        {
            if ( aDate > maMax )
                aDate = maMax;
            else if ( aDate < maMin )
                aDate = maMin;
        }
        else
        {
            // !!! TH-18.2.99: Wenn wir Zeit haben sollte einmal
            // !!! geklaert werden, warum dieses beim Datum gegenueber
            // !!! allen anderen Feldern anders behandelt wird.
            // !!! Siehe dazu Bug: 52304

            if ( !ImplAllowMalformedInput() )
            {
                if ( maLastDate.GetDate() )
                    aDate = maLastDate;
                else if ( !IsEmptyFieldValueEnabled() )
                    aDate = Date();
            }
            else
                aDate = GetInvalidDate();
        }
    }

    return aDate;
}

// -----------------------------------------------------------------------

Date DateFormatter::GetRealDate() const
{
    // !!! TH-18.2.99: Wenn wir Zeit haben sollte dieses auch einmal
    // !!! fuer die Numeric-Klassen eingebaut werden.

    Date aDate( 0, 0, 0 );

    if ( GetField() )
    {
        if ( !ImplDateGetValue( GetField()->GetText(), aDate, GetExtDateFormat(TRUE), ImplGetLocaleDataWrapper(), GetCalendarWrapper(), GetFieldSettings() ) )
            if ( ImplAllowMalformedInput() )
                aDate = GetInvalidDate();
    }

    return aDate;
}

// -----------------------------------------------------------------------

void DateFormatter::SetEmptyDate()
{
    FormatterBase::SetEmptyFieldValue();
}

// -----------------------------------------------------------------------

BOOL DateFormatter::IsEmptyDate() const
{
    BOOL bEmpty = FormatterBase::IsEmptyFieldValue();

    if ( GetField() && MustBeReformatted() && IsEmptyFieldValueEnabled() )
    {
        if ( !GetField()->GetText().Len() )
        {
            bEmpty = TRUE;
        }
        else if ( !maLastDate.GetDate() )
        {
            Date aDate;
            bEmpty = !ImplDateGetValue( GetField()->GetText(), aDate, GetExtDateFormat(TRUE), ImplGetLocaleDataWrapper(), GetCalendarWrapper(), GetFieldSettings() );
        }
    }
    return bEmpty;
}

// -----------------------------------------------------------------------

BOOL DateFormatter::IsDateModified() const
{
    if ( ImplGetEmptyFieldValue() )
        return !IsEmptyDate();
    else if ( GetDate() != maFieldDate )
        return TRUE;
    else
        return FALSE;
}

// -----------------------------------------------------------------------

void DateFormatter::Reformat()
{
    if ( !GetField() )
        return;

    if ( !GetField()->GetText().Len() && ImplGetEmptyFieldValue() )
        return;

    XubString aStr;
    BOOL bOK = ImplDateReformat( GetField()->GetText(), aStr, GetFieldSettings() );
    if( !bOK )
        return;

    if ( aStr.Len() )
    {
        ImplSetText( aStr );
        ImplDateGetValue( aStr, maLastDate, GetExtDateFormat(TRUE), ImplGetLocaleDataWrapper(), GetCalendarWrapper(), GetFieldSettings() );
    }
    else
    {
        if ( maLastDate.GetDate() )
            SetDate( maLastDate );
        else if ( !IsEmptyFieldValueEnabled() )
            SetDate( Date() );
        else
        {
            ImplSetText( ImplGetSVEmptyStr() );
            SetEmptyFieldValueData( TRUE );
        }
    }
}

// -----------------------------------------------------------------------

void DateFormatter::ExpandCentury( Date& rDate )
{
    ExpandCentury( rDate, Application::GetSettings().GetMiscSettings().GetTwoDigitYearStart() );
}

// -----------------------------------------------------------------------

void DateFormatter::ExpandCentury( Date& rDate, USHORT nTwoDigitYearStart )
{
    USHORT nDateYear = rDate.GetYear();
    if ( nDateYear < 100 )
    {
        USHORT nCentury = nTwoDigitYearStart / 100;
        if ( nDateYear < (nTwoDigitYearStart % 100) )
            nCentury++;
        rDate.SetYear( nDateYear + (nCentury*100) );
    }
}

// -----------------------------------------------------------------------

DateField::DateField( Window* pParent, WinBits nWinStyle ) :
    SpinField( pParent, nWinStyle ),
    maFirst( GetMin() ),
    maLast( GetMax() )
{
    SetField( this );
    SetText( ImplGetLocaleDataWrapper().getDate( ImplGetFieldDate() ) );
    Reformat();
    ResetLastDate();
}

// -----------------------------------------------------------------------

DateField::DateField( Window* pParent, const ResId& rResId ) :
    SpinField( WINDOW_DATEFIELD ),
    maFirst( GetMin() ),
    maLast( GetMax() )
{
    rResId.SetRT( RSC_DATEFIELD );
    WinBits nStyle = ImplInitRes( rResId );
    SpinField::ImplInit( pParent, nStyle );
    SetField( this );
    SetText( ImplGetLocaleDataWrapper().getDate( ImplGetFieldDate() ) );
    ImplLoadRes( rResId );

    if ( !(nStyle & WB_HIDE ) )
        Show();

    ResetLastDate();
}

// -----------------------------------------------------------------------

void DateField::ImplLoadRes( const ResId& rResId )
{
    SpinField::ImplLoadRes( rResId );
    
    ResMgr* pMgr = rResId.GetResMgr();
    if( pMgr )
    {
        DateFormatter::ImplLoadRes( ResId( (RSHEADER_TYPE *)GetClassRes(), *pMgr ) );
    
        ULONG  nMask = ReadLongRes();
        if ( DATEFIELD_FIRST & nMask )
        {
            maFirst = Date( ResId( (RSHEADER_TYPE *)GetClassRes(), *pMgr ) );
            IncrementRes( GetObjSizeRes( (RSHEADER_TYPE *)GetClassRes() ) );
        }
        if ( DATEFIELD_LAST & nMask )
        {
            maLast = Date( ResId( (RSHEADER_TYPE *)GetClassRes(), *pMgr ) );
            IncrementRes( GetObjSizeRes( (RSHEADER_TYPE *)GetClassRes() ) );
        }
    }

    Reformat();
}

// -----------------------------------------------------------------------

DateField::~DateField()
{
}

// -----------------------------------------------------------------------

long DateField::PreNotify( NotifyEvent& rNEvt )
{
    if ( (rNEvt.GetType() == EVENT_KEYINPUT) && IsStrictFormat() &&
         ( GetExtDateFormat() != XTDATEF_SYSTEM_LONG ) &&
         !rNEvt.GetKeyEvent()->GetKeyCode().IsMod2() )
    {
        if ( ImplDateProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), GetExtDateFormat( TRUE ), ImplGetLocaleDataWrapper() ) )
            return 1;
    }

    return SpinField::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

long DateField::Notify( NotifyEvent& rNEvt )
{
    if ( rNEvt.GetType() == EVENT_GETFOCUS )
        MarkToBeReformatted( FALSE );
    else if ( rNEvt.GetType() == EVENT_LOSEFOCUS )
    {
        if ( MustBeReformatted() )
        {
            // !!! TH-18.2.99: Wenn wir Zeit haben sollte einmal
            // !!! geklaert werden, warum dieses beim Datum gegenueber
            // !!! allen anderen Feldern anders behandelt wird.
            // !!! Siehe dazu Bug: 52304

            BOOL bTextLen = GetText().Len() != 0;
            if ( bTextLen || !IsEmptyFieldValueEnabled() )
            {
                if ( !ImplAllowMalformedInput() )
                    Reformat();
                else
                {
                    Date aDate( 0, 0, 0 );
                    if ( ImplDateGetValue( GetText(), aDate, GetExtDateFormat(TRUE), ImplGetLocaleDataWrapper(), GetCalendarWrapper(), GetFieldSettings() ) )
                        // even with strict text analysis, our text is a valid date -> do a complete
                        // reformat
                        Reformat();
                }
            }
            else if ( !bTextLen && IsEmptyFieldValueEnabled() )
            {
                ResetLastDate();
                SetEmptyFieldValueData( TRUE );
            }
        }
    }

    return SpinField::Notify( rNEvt );
}

// -----------------------------------------------------------------------

void DateField::DataChanged( const DataChangedEvent& rDCEvt )
{
    SpinField::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) && (rDCEvt.GetFlags() & (SETTINGS_LOCALE|SETTINGS_MISC)) )
    {
        if ( IsDefaultLocale() && ( rDCEvt.GetFlags() & SETTINGS_LOCALE ) )
            ImplGetLocaleDataWrapper().setLocale( GetSettings().GetLocale() );
        ReformatAll();
    }
}

// -----------------------------------------------------------------------

void DateField::Modify()
{
    MarkToBeReformatted( TRUE );
    SpinField::Modify();
}

// -----------------------------------------------------------------------

void DateField::Up()
{
    ImplDateSpinArea( TRUE );
    SpinField::Up();
}

// -----------------------------------------------------------------------

void DateField::Down()
{
    ImplDateSpinArea( FALSE );
    SpinField::Down();
}

// -----------------------------------------------------------------------

void DateField::First()
{
    ImplNewFieldValue( maFirst );
    SpinField::First();
}

// -----------------------------------------------------------------------

void DateField::Last()
{
    ImplNewFieldValue( maLast );
    SpinField::Last();
}

// -----------------------------------------------------------------------

DateBox::DateBox( Window* pParent, WinBits nWinStyle ) :
    ComboBox( pParent, nWinStyle )
{
    SetField( this );
    SetText( ImplGetLocaleDataWrapper().getDate( ImplGetFieldDate() ) );
    Reformat();
}

// -----------------------------------------------------------------------

DateBox::DateBox( Window* pParent, const ResId& rResId ) :
    ComboBox( WINDOW_DATEBOX )
{
    rResId.SetRT( RSC_DATEBOX );
    WinBits nStyle = ImplInitRes( rResId );
    ComboBox::ImplInit( pParent, nStyle );
    SetField( this );
    SetText( ImplGetLocaleDataWrapper().getDate( ImplGetFieldDate() ) );
    ComboBox::ImplLoadRes( rResId );
    ResMgr* pMgr = rResId.GetResMgr();
    if( pMgr )
        DateFormatter::ImplLoadRes( ResId( (RSHEADER_TYPE *)GetClassRes(), *pMgr ) );
    Reformat();

    if ( !( nStyle & WB_HIDE ) )
        Show();
}

// -----------------------------------------------------------------------

DateBox::~DateBox()
{
}

// -----------------------------------------------------------------------

long DateBox::PreNotify( NotifyEvent& rNEvt )
{
    if ( (rNEvt.GetType() == EVENT_KEYINPUT) && IsStrictFormat() &&
         ( GetExtDateFormat() != XTDATEF_SYSTEM_LONG ) &&
         !rNEvt.GetKeyEvent()->GetKeyCode().IsMod2() )
    {
        if ( ImplDateProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), GetExtDateFormat( TRUE ), ImplGetLocaleDataWrapper() ) )
            return 1;
    }

    return ComboBox::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

void DateBox::DataChanged( const DataChangedEvent& rDCEvt )
{
    ComboBox::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) && (rDCEvt.GetFlags() & SETTINGS_LOCALE) )
    {
        if ( IsDefaultLocale() )
            ImplGetLocaleDataWrapper().setLocale( GetSettings().GetLocale() );
        ReformatAll();
    }
}

// -----------------------------------------------------------------------

long DateBox::Notify( NotifyEvent& rNEvt )
{
    if ( rNEvt.GetType() == EVENT_GETFOCUS )
        MarkToBeReformatted( FALSE );
    else if ( rNEvt.GetType() == EVENT_LOSEFOCUS )
    {
        if ( MustBeReformatted() )
        {
            BOOL bTextLen = GetText().Len() != 0;
            if ( bTextLen || !IsEmptyFieldValueEnabled() )
                Reformat();
            else if ( !bTextLen && IsEmptyFieldValueEnabled() )
            {
                ResetLastDate();
                SetEmptyFieldValueData( TRUE );
            }
        }
    }

    return ComboBox::Notify( rNEvt );
}

// -----------------------------------------------------------------------

void DateBox::Modify()
{
    MarkToBeReformatted( TRUE );
    ComboBox::Modify();
}

// -----------------------------------------------------------------------

void DateBox::ReformatAll()
{
    XubString aStr;
    SetUpdateMode( FALSE );
    USHORT nEntryCount = GetEntryCount();
    for ( USHORT i=0; i < nEntryCount; i++ )
    {
        ImplDateReformat( GetEntry( i ), aStr, GetFieldSettings() );
        RemoveEntry( i );
        InsertEntry( aStr, i );
    }
    DateFormatter::Reformat();
    SetUpdateMode( TRUE );
}

// -----------------------------------------------------------------------

void DateBox::InsertDate( const Date& rDate, USHORT nPos )
{
    Date aDate = rDate;
    if ( aDate > GetMax() )
        aDate = GetMax();
    else if ( aDate < GetMin() )
        aDate = GetMin();

    ComboBox::InsertEntry( ImplGetDateAsText( aDate, GetFieldSettings() ), nPos );
}

// -----------------------------------------------------------------------

void DateBox::RemoveDate( const Date& rDate )
{
    ComboBox::RemoveEntry( ImplGetDateAsText( rDate, GetFieldSettings() ) );
}

// -----------------------------------------------------------------------

Date DateBox::GetDate( USHORT nPos ) const
{
    Date aDate( 0, 0, 0 );
    ImplDateGetValue( ComboBox::GetEntry( nPos ), aDate, GetExtDateFormat(TRUE), ImplGetLocaleDataWrapper(), GetCalendarWrapper(), GetSettings() );
    return aDate;
}

// -----------------------------------------------------------------------

USHORT DateBox::GetDatePos( const Date& rDate ) const
{
    XubString aStr;
    if ( IsLongFormat() )
        aStr = ImplGetLocaleDataWrapper().getLongDate( rDate, GetCalendarWrapper(), 1, FALSE, 1, !IsShowDateCentury() );
    else
        aStr = ImplGetLocaleDataWrapper().getDate( rDate );
    return ComboBox::GetEntryPos( aStr );
}

// -----------------------------------------------------------------------

static BOOL ImplTimeProcessKeyInput( Edit*, const KeyEvent& rKEvt,
                                     BOOL bStrictFormat, BOOL bDuration,
                                     TimeFieldFormat eFormat,
                                     const LocaleDataWrapper& rLocaleDataWrapper  )
{
    xub_Unicode cChar = rKEvt.GetCharCode();

    if ( !bStrictFormat )
        return FALSE;
    else
    {
        USHORT nGroup = rKEvt.GetKeyCode().GetGroup();
        if ( (nGroup == KEYGROUP_FKEYS) || (nGroup == KEYGROUP_CURSOR) ||
             (nGroup == KEYGROUP_MISC)   ||
             ((cChar >= '0') && (cChar <= '9')) ||
             (cChar == rLocaleDataWrapper.getTimeSep()) ||
             ( ( rLocaleDataWrapper.getTimeAM().Search( cChar ) != STRING_NOTFOUND ) ) ||
             ( ( rLocaleDataWrapper.getTimePM().Search( cChar ) != STRING_NOTFOUND ) ) ||
             // Accept AM/PM:
             (cChar == 'a') || (cChar == 'A') || (cChar == 'm') || (cChar == 'M') || (cChar == 'p') || (cChar == 'P') ||
             ((eFormat == TIMEF_100TH_SEC) && (cChar == rLocaleDataWrapper.getTime100SecSep())) ||
             ((eFormat == TIMEF_SEC_CS) && (cChar == rLocaleDataWrapper.getTime100SecSep())) ||
             (bDuration && (cChar == '-')) )
            return FALSE;
        else
            return TRUE;
    }
}

// -----------------------------------------------------------------------

static BOOL ImplIsOnlyDigits( const String& _rStr )
{
    const sal_Unicode* _pChr = _rStr.GetBuffer();
    for ( xub_StrLen i = 0; i < _rStr.Len(); ++i, ++_pChr )
    {
        if ( *_pChr < '0' || *_pChr > '9' )
            return FALSE;
    }
    return TRUE;
}

// -----------------------------------------------------------------------

static BOOL ImplIsValidTimePortion( BOOL _bSkipInvalidCharacters, const String& _rStr )
{
    if ( !_bSkipInvalidCharacters )
    {
        if ( ( _rStr.Len() > 2 ) || ( _rStr.Len() < 1 ) || !ImplIsOnlyDigits( _rStr ) )
            return FALSE;
    }
    return TRUE;
}

// -----------------------------------------------------------------------

static BOOL ImplCutTimePortion( String& _rStr, xub_StrLen _nSepPos, BOOL _bSkipInvalidCharacters, short* _pPortion )
{
    String sPortion = _rStr.Copy( 0, _nSepPos );
    _rStr.Erase( 0, _nSepPos + 1 );

    if ( !ImplIsValidTimePortion( _bSkipInvalidCharacters, sPortion ) )
        return FALSE;
    *_pPortion = (short)sPortion.ToInt32();
    return TRUE;
}

// -----------------------------------------------------------------------

static BOOL ImplTimeGetValue( const XubString& rStr, Time& rTime,
                              TimeFieldFormat eFormat, BOOL bDuration,
                              const LocaleDataWrapper& rLocaleDataWrapper, BOOL _bSkipInvalidCharacters = TRUE )
{
    XubString   aStr    = rStr;
    short       nHour   = 0;
    short       nMinute = 0;
    short       nSecond = 0;
    short       n100Sec = 0;
    Time        aTime( 0, 0, 0 );

    if ( !rStr.Len() )
        return FALSE;

    // Nach Separatoren suchen
    if ( rLocaleDataWrapper.getTimeSep().Len() )
    {
        XubString aSepStr( RTL_CONSTASCII_USTRINGPARAM( ",.;:/" ) );
        if ( !bDuration )
            aSepStr.Append( '-' );

        // Die obigen Zeichen durch das Separatorzeichen ersetzen
        for ( xub_StrLen i = 0; i < aSepStr.Len(); i++ )
        {
            if ( aSepStr.GetChar( i ) == rLocaleDataWrapper.getTimeSep() )
                continue;
            for ( xub_StrLen j = 0; j < aStr.Len(); j++ )
            {
                if ( aStr.GetChar( j ) == aSepStr.GetChar( i ) )
                    aStr.SetChar( j, rLocaleDataWrapper.getTimeSep().GetChar(0) );
            }
        }
    }

    BOOL bNegative = FALSE;
    xub_StrLen nSepPos = aStr.Search( rLocaleDataWrapper.getTimeSep() );
    if ( aStr.GetChar( 0 ) == '-' )
        bNegative = TRUE;
    if ( eFormat != TIMEF_SEC_CS )
    {
        if ( nSepPos == STRING_NOTFOUND )
            nSepPos = aStr.Len();
        if ( !ImplCutTimePortion( aStr, nSepPos, _bSkipInvalidCharacters, &nHour ) )
            return FALSE;

        nSepPos = aStr.Search( rLocaleDataWrapper.getTimeSep() );
        if ( aStr.GetChar( 0 ) == '-' )
            bNegative = TRUE;
        if ( nSepPos != STRING_NOTFOUND )
        {
            if ( !ImplCutTimePortion( aStr, nSepPos, _bSkipInvalidCharacters, &nMinute ) )
                return FALSE;

            nSepPos = aStr.Search( rLocaleDataWrapper.getTimeSep() );
            if ( aStr.GetChar( 0 ) == '-' )
                bNegative = TRUE;
            if ( nSepPos != STRING_NOTFOUND )
            {
                if ( !ImplCutTimePortion( aStr, nSepPos, _bSkipInvalidCharacters, &nSecond ) )
                    return FALSE;
                if ( aStr.GetChar( 0 ) == '-' )
                    bNegative = TRUE;
                n100Sec = (short)aStr.ToInt32();
            }
            else
                nSecond = (short)aStr.ToInt32();
        }
        else
            nMinute = (short)aStr.ToInt32();
    }
    else if ( nSepPos == STRING_NOTFOUND )
    {
        nSecond = (short)aStr.ToInt32();
        nMinute += nSecond / 60;
        nSecond %= 60;
        nHour += nMinute / 60;
        nMinute %= 60;
    }
    else
    {
        nSecond = (short)aStr.Copy( 0, nSepPos ).ToInt32();
        aStr.Erase( 0, nSepPos+1 );

        nSepPos = aStr.Search( rLocaleDataWrapper.getTimeSep() );
        if ( aStr.GetChar( 0 ) == '-' )
            bNegative = TRUE;
        if ( nSepPos != STRING_NOTFOUND )
        {
            nMinute = nSecond;
            nSecond = (short)aStr.Copy( 0, nSepPos ).ToInt32();
            aStr.Erase( 0, nSepPos+1 );

            nSepPos = aStr.Search( rLocaleDataWrapper.getTimeSep() );
            if ( aStr.GetChar( 0 ) == '-' )
                bNegative = TRUE;
            if ( nSepPos != STRING_NOTFOUND )
            {
                nHour   = nMinute;
                nMinute = nSecond;
                nSecond = (short)aStr.Copy( 0, nSepPos ).ToInt32();
                aStr.Erase( 0, nSepPos+1 );
            }
            else
            {
                nHour += nMinute / 60;
                nMinute %= 60;
            }
        }
        else
        {
            nMinute += nSecond / 60;
            nSecond %= 60;
            nHour += nMinute / 60;
            nMinute %= 60;
        }
        n100Sec = (short)aStr.ToInt32();

        if ( n100Sec )
        {
            xub_StrLen nLen = 1; // mindestens eine Ziffer, weil sonst n100Sec==0

            while ( aStr.GetChar(nLen) >= '0' && aStr.GetChar(nLen) <= '9' )
                nLen++;

            if ( nLen > 2 )
            {
                while( nLen > 3 )
                {
                    n100Sec = n100Sec / 10;
                    nLen--;
                }
                // Rundung bei negativen Zahlen???
                n100Sec = (n100Sec + 5) / 10;
            }
            else
            {
                while( nLen < 2 )
                {
                    n100Sec = n100Sec * 10;
                    nLen++;
                }
            }
        }
    }

    if ( (nMinute > 59) || (nSecond > 59) || (n100Sec > 100) )
        return FALSE;

    if ( eFormat == TIMEF_NONE )
        nSecond = n100Sec = 0;
    else if ( eFormat == TIMEF_SEC )
        n100Sec = 0;

    if ( !bDuration )
    {
        if ( bNegative || (nHour < 0) || (nMinute < 0) ||
             (nSecond < 0) || (n100Sec < 0) )
            return FALSE;

        aStr.ToUpperAscii();
        XubString aAM( rLocaleDataWrapper.getTimeAM() );
        XubString aPM( rLocaleDataWrapper.getTimePM() );
        aAM.ToUpperAscii();
        aPM.ToUpperAscii();
        XubString aAM2( RTL_CONSTASCII_USTRINGPARAM( "AM" ) );  // aAM is localized
        XubString aPM2( RTL_CONSTASCII_USTRINGPARAM( "PM" ) );  // aPM is localized

        if ( (nHour < 12) && ( ( aStr.Search( aPM ) != STRING_NOTFOUND ) || ( aStr.Search( aPM2 ) != STRING_NOTFOUND ) ) )
            nHour += 12;

        if ( (nHour == 12) && ( ( aStr.Search( aAM ) != STRING_NOTFOUND ) || ( aStr.Search( aAM2 ) != STRING_NOTFOUND ) ) )
            nHour = 0;

        aTime = Time( (USHORT)nHour, (USHORT)nMinute, (USHORT)nSecond,
                      (USHORT)n100Sec );
    }
    else
    {
        if ( bNegative || (nHour < 0) || (nMinute < 0) ||
             (nSecond < 0) || (n100Sec < 0) )
        {
            bNegative   = TRUE;
            nHour       = nHour < 0 ? -nHour : nHour;
            nMinute     = nMinute < 0 ? -nMinute : nMinute;
            nSecond     = nSecond < 0 ? -nSecond : nSecond;
            n100Sec     = n100Sec < 0 ? -n100Sec : n100Sec;
        }

        aTime = Time( (USHORT)nHour, (USHORT)nMinute, (USHORT)nSecond,
                      (USHORT)n100Sec );
        if ( bNegative )
            aTime = -aTime;
    }

    rTime = aTime;

    return TRUE;
}

// -----------------------------------------------------------------------

BOOL TimeFormatter::ImplTimeReformat( const XubString& rStr, XubString& rOutStr )
{
    Time aTime( 0, 0, 0 );
    if ( !ImplTimeGetValue( rStr, aTime, GetFormat(), IsDuration(), ImplGetLocaleDataWrapper() ) )
        return TRUE;

    Time aTempTime = aTime;
    if ( aTempTime > GetMax() )
        aTempTime = GetMax() ;
    else if ( aTempTime < GetMin() )
        aTempTime = GetMin();

    if ( GetErrorHdl().IsSet() && (aTime != aTempTime) )
    {
        maCorrectedTime = aTempTime;
        if ( !GetErrorHdl().Call( this ) )
        {
            maCorrectedTime = Time();
            return FALSE;
        }
        else
            maCorrectedTime = Time();
    }

    BOOL bSecond = FALSE;
    BOOL b100Sec = FALSE;
    if ( meFormat != TIMEF_NONE )
        bSecond = TRUE;
    if ( meFormat == TIMEF_100TH_SEC )
        b100Sec = TRUE;

    if ( meFormat == TIMEF_SEC_CS )
    {
        ULONG n  = aTempTime.GetHour() * 3600L;
        n       += aTempTime.GetMin()  * 60L;
        n       += aTempTime.GetSec();
        rOutStr  = String::CreateFromInt32( n );
        rOutStr += ImplGetLocaleDataWrapper().getTime100SecSep();
        if ( aTempTime.Get100Sec() < 10 )
            rOutStr += '0';
        rOutStr += String::CreateFromInt32( aTempTime.Get100Sec() );
    }
    else if ( mbDuration )
        rOutStr = ImplGetLocaleDataWrapper().getDuration( aTempTime, bSecond, b100Sec );
    else
    {
        rOutStr = ImplGetLocaleDataWrapper().getTime( aTempTime, bSecond, b100Sec );
        if ( GetTimeFormat() == HOUR_12 )
        {
            if ( aTempTime.GetHour() > 12 )
            {
                Time aT( aTempTime );
                aT.SetHour( aT.GetHour() % 12 );
                rOutStr = ImplGetLocaleDataWrapper().getTime( aT, bSecond, b100Sec );
            }
            // Don't use LocaleDataWrapper, we want AM/PM
            if ( aTempTime.GetHour() < 12 )
                rOutStr += String( RTL_CONSTASCII_USTRINGPARAM( "AM" ) ); // ImplGetLocaleDataWrapper().getTimeAM();
            else
                rOutStr += String( RTL_CONSTASCII_USTRINGPARAM( "PM" ) ); // ImplGetLocaleDataWrapper().getTimePM();
        }
    }

    return TRUE;
}

// -----------------------------------------------------------------------
BOOL TimeFormatter::ImplAllowMalformedInput() const
{
    return !IsEnforceValidValue();
}

// -----------------------------------------------------------------------

void TimeField::ImplTimeSpinArea( BOOL bUp )
{
    if ( GetField() )
    {
        xub_StrLen nTimeArea = 0;
        Time aTime( GetTime() );
        XubString aText( GetText() );
        Selection aSelection( GetField()->GetSelection() );

        // Area suchen
        if ( GetFormat() != TIMEF_SEC_CS )
        {
            for ( xub_StrLen i = 1, nPos = 0; i <= 4; i++ )
            {
                xub_StrLen nPos1 = aText.Search( ImplGetLocaleDataWrapper().getTimeSep(), nPos );
                xub_StrLen nPos2 = aText.Search( ImplGetLocaleDataWrapper().getTime100SecSep(), nPos );
                nPos = nPos1 < nPos2 ? nPos1 : nPos2;
                if ( nPos >= (xub_StrLen)aSelection.Max() )
                {
                    nTimeArea = i;
                    break;
                }
                else
                    nPos++;
            }
        }
        else
        {
            xub_StrLen nPos = aText.Search( ImplGetLocaleDataWrapper().getTime100SecSep() );
            if ( nPos == STRING_NOTFOUND || nPos >= (xub_StrLen)aSelection.Max() )
                nTimeArea = 3;
            else
                nTimeArea = 4;
        }

        if ( nTimeArea )
        {
            Time aAddTime( 0, 0, 0 );
            if ( nTimeArea == 1 )
                aAddTime = Time( 1, 0 );
            else if ( nTimeArea == 2 )
                aAddTime = Time( 0, 1 );
            else if ( nTimeArea == 3 )
                aAddTime = Time( 0, 0, 1 );
            else if ( nTimeArea == 4 )
                aAddTime = Time( 0, 0, 0, 1 );

            if ( !bUp )
                aAddTime = -aAddTime;

            aTime += aAddTime;
            if ( !IsDuration() )
            {
                Time aAbsMaxTime( 23, 59, 59, 99 );
                if ( aTime > aAbsMaxTime )
                    aTime = aAbsMaxTime;
                Time aAbsMinTime( 0, 0 );
                if ( aTime < aAbsMinTime )
                    aTime = aAbsMinTime;
            }
            ImplNewFieldValue( aTime );
        }

    }
}

// -----------------------------------------------------------------------

void TimeFormatter::ImplInit()
{
    meFormat        = TIMEF_NONE;
    mbDuration      = FALSE;
    mnTimeFormat    = HOUR_24;  // Should become a ExtTimeFieldFormat in next implementation, merge with mbDuration and meFormat
}

// -----------------------------------------------------------------------

TimeFormatter::TimeFormatter() :
    maLastTime( 0, 0 ),
    maMin( 0, 0 ),
    maMax( 23, 59, 59, 99 ),
    mbEnforceValidValue( TRUE ),
    maFieldTime( 0, 0 )
{
    ImplInit();
}

// -----------------------------------------------------------------------

void TimeFormatter::ImplLoadRes( const ResId& rResId )
{
    ResMgr* pMgr = rResId.GetResMgr();
    if( pMgr )
    {
        ULONG   nMask = pMgr->ReadLong();
    
        if ( TIMEFORMATTER_MIN & nMask )
        {
            SetMin( Time( ResId( (RSHEADER_TYPE *)pMgr->GetClass(), *pMgr ) ) );
            pMgr->Increment( pMgr->GetObjSize( (RSHEADER_TYPE *)pMgr->GetClass() ) );
        }
    
        if ( TIMEFORMATTER_MAX & nMask )
        {
            SetMax( Time( ResId( (RSHEADER_TYPE *)pMgr->GetClass(), *pMgr ) ) );
            pMgr->Increment( pMgr->GetObjSize( (RSHEADER_TYPE *)pMgr->GetClass() ) );
        }
    
        if ( TIMEFORMATTER_TIMEFIELDFORMAT & nMask )
            meFormat = (TimeFieldFormat)pMgr->ReadLong();
    
        if ( TIMEFORMATTER_DURATION & nMask )
            mbDuration = (BOOL)pMgr->ReadShort();
    
        if ( TIMEFORMATTER_STRICTFORMAT & nMask )
            SetStrictFormat( (BOOL)pMgr->ReadShort() );
    
        if ( TIMEFORMATTER_VALUE & nMask )
        {
            maFieldTime = Time( ResId( (RSHEADER_TYPE *)pMgr->GetClass(), *pMgr ) );
            if ( maFieldTime > GetMax() )
                maFieldTime = GetMax();
            if ( maFieldTime < GetMin() )
                maFieldTime = GetMin();
            maLastTime = maFieldTime;
    
            pMgr->Increment( pMgr->GetObjSize( (RSHEADER_TYPE *)pMgr->GetClass() ) );
        }
    }
}

// -----------------------------------------------------------------------

TimeFormatter::~TimeFormatter()
{
}

// -----------------------------------------------------------------------

void TimeFormatter::ReformatAll()
{
    Reformat();
}

// -----------------------------------------------------------------------

void TimeFormatter::SetMin( const Time& rNewMin )
{
    maMin = rNewMin;
    if ( !IsEmptyFieldValue() )
        ReformatAll();
}

// -----------------------------------------------------------------------

void TimeFormatter::SetMax( const Time& rNewMax )
{
    maMax = rNewMax;
    if ( !IsEmptyFieldValue() )
        ReformatAll();
}

// -----------------------------------------------------------------------

void TimeFormatter::SetTimeFormat( TimeFormatter::TimeFormat eNewFormat )
{
    mnTimeFormat = sal::static_int_cast<USHORT>(eNewFormat);
}

// -----------------------------------------------------------------------

TimeFormatter::TimeFormat TimeFormatter::GetTimeFormat() const
{
    return (TimeFormat)mnTimeFormat;
}

// -----------------------------------------------------------------------

void TimeFormatter::SetFormat( TimeFieldFormat eNewFormat )
{
    meFormat = eNewFormat;
    ReformatAll();
}

// -----------------------------------------------------------------------

void TimeFormatter::SetDuration( BOOL bNewDuration )
{
    mbDuration = bNewDuration;
    ReformatAll();
}

// -----------------------------------------------------------------------

void TimeFormatter::SetTime( const Time& rNewTime )
{
    SetUserTime( rNewTime );
    maFieldTime = maLastTime;
    SetEmptyFieldValueData( FALSE );
}

// -----------------------------------------------------------------------

void TimeFormatter::ImplNewFieldValue( const Time& rTime )
{
    if ( GetField() )
    {
        Selection aSelection = GetField()->GetSelection();
        aSelection.Justify();
        XubString aText = GetField()->GetText();
        // Wenn bis ans Ende selektiert war, soll das auch so bleiben...
        if ( (xub_StrLen)aSelection.Max() == aText.Len() )
        {
            if ( !aSelection.Len() )
                aSelection.Min() = SELECTION_MAX;
            aSelection.Max() = SELECTION_MAX;
        }

        Time aOldLastTime = maLastTime;
        ImplSetUserTime( rTime, &aSelection );
        maLastTime = aOldLastTime;

        // Modify am Edit wird nur bei KeyInput gesetzt...
        if ( GetField()->GetText() != aText )
        {
            GetField()->SetModifyFlag();
            GetField()->Modify();
        }
    }
}

// -----------------------------------------------------------------------

void TimeFormatter::ImplSetUserTime( const Time& rNewTime, Selection* pNewSelection )
{
    Time aNewTime = rNewTime;
    if ( aNewTime > GetMax() )
        aNewTime = GetMax();
    else if ( aNewTime < GetMin() )
        aNewTime = GetMin();
    maLastTime = aNewTime;

    if ( GetField() )
    {
        XubString aStr;
        BOOL bSec    = FALSE;
        BOOL b100Sec = FALSE;
        if ( meFormat != TIMEF_NONE )
            bSec = TRUE;
        if ( meFormat == TIMEF_100TH_SEC || meFormat == TIMEF_SEC_CS )
            b100Sec = TRUE;
        if ( meFormat == TIMEF_SEC_CS )
        {
            ULONG n  = aNewTime.GetHour() * 3600L;
            n       += aNewTime.GetMin()  * 60L;
            n       += aNewTime.GetSec();
            aStr     = String::CreateFromInt32( n );
            aStr    += ImplGetLocaleDataWrapper().getTime100SecSep();
            if ( aNewTime.Get100Sec() < 10 )
                aStr += '0';
            aStr += String::CreateFromInt32( aNewTime.Get100Sec() );
        }
        else if ( mbDuration )
        {
            aStr = ImplGetLocaleDataWrapper().getDuration( aNewTime, bSec, b100Sec );
        }
        else
        {
            aStr = ImplGetLocaleDataWrapper().getTime( aNewTime, bSec, b100Sec );
            if ( GetTimeFormat() == HOUR_12 )
            {
                if ( aNewTime.GetHour() > 12 )
                {
                    Time aT( aNewTime );
                    aT.SetHour( aT.GetHour() % 12 );
                    aStr = ImplGetLocaleDataWrapper().getTime( aT, bSec, b100Sec );
                }
                // Don't use LocaleDataWrapper, we want AM/PM
                if ( aNewTime.GetHour() < 12 )
                    aStr += String( RTL_CONSTASCII_USTRINGPARAM( "AM" ) ); // ImplGetLocaleDataWrapper().getTimeAM();
                else
                    aStr += String( RTL_CONSTASCII_USTRINGPARAM( "PM" ) ); // ImplGetLocaleDataWrapper().getTimePM();
            }
        }

        ImplSetText( aStr, pNewSelection );
    }
}

// -----------------------------------------------------------------------

void TimeFormatter::SetUserTime( const Time& rNewTime )
{
    ImplSetUserTime( rNewTime );
}

// -----------------------------------------------------------------------

Time TimeFormatter::GetTime() const
{
    Time aTime( 0, 0, 0 );

    if ( GetField() )
    {
        BOOL bAllowMailformed = ImplAllowMalformedInput();
        if ( ImplTimeGetValue( GetField()->GetText(), aTime, GetFormat(), IsDuration(), ImplGetLocaleDataWrapper(), !bAllowMailformed ) )
        {
            if ( aTime > GetMax() )
                aTime = GetMax();
            else if ( aTime < GetMin() )
                aTime = GetMin();
        }
        else
        {
            if ( bAllowMailformed )
                aTime = GetInvalidTime();
            else
                aTime = maLastTime;
        }
    }

    return aTime;
}

// -----------------------------------------------------------------------

Time TimeFormatter::GetRealTime() const
{
    Time aTime( 0, 0, 0 );

    if ( GetField() )
    {
        BOOL bAllowMailformed = ImplAllowMalformedInput();
        if ( !ImplTimeGetValue( GetField()->GetText(), aTime, GetFormat(), IsDuration(), ImplGetLocaleDataWrapper(), !bAllowMailformed ) )
            if ( bAllowMailformed )
                aTime = GetInvalidTime();
    }

    return aTime;
}

// -----------------------------------------------------------------------

BOOL TimeFormatter::IsTimeModified() const
{
    if ( ImplGetEmptyFieldValue() )
        return !IsEmptyTime();
    else if ( GetTime() != maFieldTime )
        return TRUE;
    else
        return FALSE;
}

// -----------------------------------------------------------------------

void TimeFormatter::Reformat()
{
    if ( !GetField() )
        return;

    if ( !GetField()->GetText().Len() && ImplGetEmptyFieldValue() )
        return;

    XubString aStr;
    BOOL bOK = ImplTimeReformat( GetField()->GetText(), aStr );
    if ( !bOK )
        return;

    if ( aStr.Len() )
    {
        ImplSetText( aStr );
        ImplTimeGetValue( aStr, maLastTime, GetFormat(), IsDuration(), ImplGetLocaleDataWrapper() );
    }
    else
        SetTime( maLastTime );
}

// -----------------------------------------------------------------------

TimeField::TimeField( Window* pParent, WinBits nWinStyle ) :
    SpinField( pParent, nWinStyle ),
    maFirst( GetMin() ),
    maLast( GetMax() )
{
    SetField( this );
    SetText( ImplGetLocaleDataWrapper().getTime( maFieldTime, FALSE, FALSE ) );
    Reformat();
}

// -----------------------------------------------------------------------

TimeField::TimeField( Window* pParent, const ResId& rResId ) :
    SpinField( WINDOW_TIMEFIELD ),
    maFirst( GetMin() ),
    maLast( GetMax() )
{
    rResId.SetRT( RSC_TIMEFIELD );
    WinBits nStyle = ImplInitRes( rResId );
    SpinField::ImplInit( pParent, nStyle );
    SetField( this );
    SetText( ImplGetLocaleDataWrapper().getTime( maFieldTime, FALSE, FALSE ) );
    ImplLoadRes( rResId );

    if ( !(nStyle & WB_HIDE ) )
        Show();
}

// -----------------------------------------------------------------------

void TimeField::ImplLoadRes( const ResId& rResId )
{
    SpinField::ImplLoadRes( rResId );
    ResMgr* pMgr = rResId.GetResMgr();
    if( pMgr )
    {
        TimeFormatter::ImplLoadRes( ResId( (RSHEADER_TYPE *)GetClassRes(), *pMgr ) );
    
        ULONG      nMask = ReadLongRes();
    
        if ( TIMEFIELD_FIRST & nMask )
        {
            maFirst = Time( ResId( (RSHEADER_TYPE *)GetClassRes(), *pMgr ) );
            IncrementRes( GetObjSizeRes( (RSHEADER_TYPE *)GetClassRes() ) );
        }
        if ( TIMEFIELD_LAST & nMask )
        {
            maLast = Time( ResId( (RSHEADER_TYPE *)GetClassRes(), *pMgr ) );
            IncrementRes( GetObjSizeRes( (RSHEADER_TYPE *)GetClassRes() ) );
        }
    }
    
    Reformat();
}

// -----------------------------------------------------------------------

TimeField::~TimeField()
{
}

// -----------------------------------------------------------------------

long TimeField::PreNotify( NotifyEvent& rNEvt )
{
    if ( (rNEvt.GetType() == EVENT_KEYINPUT) && !rNEvt.GetKeyEvent()->GetKeyCode().IsMod2() )
    {
        if ( ImplTimeProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), IsStrictFormat(), IsDuration(), GetFormat(), ImplGetLocaleDataWrapper() ) )
            return 1;
    }

    return SpinField::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

long TimeField::Notify( NotifyEvent& rNEvt )
{
    if ( rNEvt.GetType() == EVENT_GETFOCUS )
        MarkToBeReformatted( FALSE );
    else if ( rNEvt.GetType() == EVENT_LOSEFOCUS )
    {
        if ( MustBeReformatted() && (GetText().Len() || !IsEmptyFieldValueEnabled()) )
        {
            if ( !ImplAllowMalformedInput() )
                Reformat();
            else
            {
                Time aTime( 0, 0, 0 );
                if ( ImplTimeGetValue( GetText(), aTime, GetFormat(), IsDuration(), ImplGetLocaleDataWrapper(), FALSE ) )
                    // even with strict text analysis, our text is a valid time -> do a complete
                    // reformat
                    Reformat();
            }
        }
    }

    return SpinField::Notify( rNEvt );
}

// -----------------------------------------------------------------------

void TimeField::DataChanged( const DataChangedEvent& rDCEvt )
{
    SpinField::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) && (rDCEvt.GetFlags() & SETTINGS_LOCALE) )
    {
        if ( IsDefaultLocale() )
            ImplGetLocaleDataWrapper().setLocale( GetSettings().GetLocale() );
        ReformatAll();
    }
}

// -----------------------------------------------------------------------

void TimeField::Modify()
{
    MarkToBeReformatted( TRUE );
    SpinField::Modify();
}

// -----------------------------------------------------------------------

void TimeField::Up()
{
    ImplTimeSpinArea( TRUE );
    SpinField::Up();
}

// -----------------------------------------------------------------------

void TimeField::Down()
{
    ImplTimeSpinArea( FALSE );
    SpinField::Down();
}

// -----------------------------------------------------------------------

void TimeField::First()
{
    ImplNewFieldValue( maFirst );
    SpinField::First();
}

// -----------------------------------------------------------------------

void TimeField::Last()
{
    ImplNewFieldValue( maLast );
    SpinField::Last();
}

// -----------------------------------------------------------------------

void TimeField::SetExtFormat( ExtTimeFieldFormat eFormat )
{
    switch ( eFormat )
    {
        case EXTTIMEF_24H_SHORT:
        {
            SetTimeFormat( HOUR_24 );
            SetDuration( FALSE );
            SetFormat( TIMEF_NONE );
        }
        break;
        case EXTTIMEF_24H_LONG:
        {
            SetTimeFormat( HOUR_24 );
            SetDuration( FALSE );
            SetFormat( TIMEF_SEC );
        }
        break;
        case EXTTIMEF_12H_SHORT:
        {
            SetTimeFormat( HOUR_12 );
            SetDuration( FALSE );
            SetFormat( TIMEF_NONE );
        }
        break;
        case EXTTIMEF_12H_LONG:
        {
            SetTimeFormat( HOUR_12 );
            SetDuration( FALSE );
            SetFormat( TIMEF_SEC );
        }
        break;
        case EXTTIMEF_DURATION_SHORT:
        {
            SetDuration( TRUE );
            SetFormat( TIMEF_NONE );
        }
        break;
        case EXTTIMEF_DURATION_LONG:
        {
            SetDuration( TRUE );
            SetFormat( TIMEF_SEC );
        }
        break;
        default:    DBG_ERROR( "ExtTimeFieldFormat unknown!" );
    }

    if ( GetField() && GetField()->GetText().Len() )
        SetUserTime( GetTime() );
    ReformatAll();
}

// -----------------------------------------------------------------------

TimeBox::TimeBox( Window* pParent, WinBits nWinStyle ) :
    ComboBox( pParent, nWinStyle )
{
    SetField( this );
    SetText( ImplGetLocaleDataWrapper().getTime( maFieldTime, FALSE, FALSE ) );
    Reformat();
}

// -----------------------------------------------------------------------

TimeBox::TimeBox( Window* pParent, const ResId& rResId ) :
    ComboBox( WINDOW_TIMEBOX )
{
    rResId.SetRT( RSC_TIMEBOX );
    WinBits nStyle = ImplInitRes( rResId );
    ComboBox::ImplInit( pParent, nStyle );
    SetField( this );
    SetText( ImplGetLocaleDataWrapper().getTime( maFieldTime, FALSE, FALSE ) );
    ComboBox::ImplLoadRes( rResId );
    ResMgr* pMgr = rResId.GetResMgr();
    if( pMgr )
        TimeFormatter::ImplLoadRes( ResId( (RSHEADER_TYPE *)GetClassRes(), *pMgr ) );
    Reformat();

    if ( !(nStyle & WB_HIDE) )
        Show();
}

// -----------------------------------------------------------------------

TimeBox::~TimeBox()
{
}

// -----------------------------------------------------------------------

long TimeBox::PreNotify( NotifyEvent& rNEvt )
{
    if ( (rNEvt.GetType() == EVENT_KEYINPUT) && !rNEvt.GetKeyEvent()->GetKeyCode().IsMod2() )
    {
        if ( ImplTimeProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), IsStrictFormat(), IsDuration(), GetFormat(), ImplGetLocaleDataWrapper() ) )
            return 1;
    }

    return ComboBox::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

long TimeBox::Notify( NotifyEvent& rNEvt )
{
    if ( rNEvt.GetType() == EVENT_GETFOCUS )
        MarkToBeReformatted( FALSE );
    else if ( rNEvt.GetType() == EVENT_LOSEFOCUS )
    {
        if ( MustBeReformatted() && (GetText().Len() || !IsEmptyFieldValueEnabled()) )
            Reformat();
    }

    return ComboBox::Notify( rNEvt );
}

// -----------------------------------------------------------------------

void TimeBox::DataChanged( const DataChangedEvent& rDCEvt )
{
    ComboBox::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) && (rDCEvt.GetFlags() & SETTINGS_LOCALE) )
    {
        if ( IsDefaultLocale() )
            ImplGetLocaleDataWrapper().setLocale( GetSettings().GetLocale() );
        ReformatAll();
    }
}

// -----------------------------------------------------------------------

void TimeBox::Modify()
{
    MarkToBeReformatted( TRUE );
    ComboBox::Modify();
}

// -----------------------------------------------------------------------

void TimeBox::ReformatAll()
{
    XubString aStr;
    SetUpdateMode( FALSE );
    USHORT nEntryCount = GetEntryCount();
    for ( USHORT i=0; i < nEntryCount; i++ )
    {
        ImplTimeReformat( GetEntry( i ), aStr );
        RemoveEntry( i );
        InsertEntry( aStr, i );
    }
    TimeFormatter::Reformat();
    SetUpdateMode( TRUE );
}

// -----------------------------------------------------------------------

void TimeBox::InsertTime( const Time& rTime, USHORT nPos )
{
    Time aTime = rTime;
    if ( aTime > GetMax() )
        aTime = GetMax();
    else if ( aTime < GetMin() )
        aTime = GetMin();

    BOOL bSec    = FALSE;
    BOOL b100Sec = FALSE;
    if ( GetFormat() == TIMEF_SEC )
        bSec = TRUE;
    if ( GetFormat() == TIMEF_100TH_SEC || GetFormat() == TIMEF_SEC_CS )
        bSec = b100Sec = TRUE;
    ComboBox::InsertEntry( ImplGetLocaleDataWrapper().getTime( aTime, bSec, b100Sec ), nPos );
}

// -----------------------------------------------------------------------

void TimeBox::RemoveTime( const Time& rTime )
{
    BOOL bSec    = FALSE;
    BOOL b100Sec = FALSE;
    if ( GetFormat() == TIMEF_SEC )
        bSec = TRUE;
    if ( GetFormat() == TIMEF_100TH_SEC || TIMEF_SEC_CS )
        bSec = b100Sec = TRUE;
    ComboBox::RemoveEntry( ImplGetLocaleDataWrapper().getTime( rTime, bSec, b100Sec ) );
}

// -----------------------------------------------------------------------

Time TimeBox::GetTime( USHORT nPos ) const
{
    Time aTime( 0, 0, 0 );
    ImplTimeGetValue( ComboBox::GetEntry( nPos ), aTime, GetFormat(), IsDuration(), ImplGetLocaleDataWrapper() );
    return aTime;
}

// -----------------------------------------------------------------------

USHORT TimeBox::GetTimePos( const Time& rTime ) const
{
    BOOL bSec    = FALSE;
    BOOL b100Sec = FALSE;
    if ( GetFormat() == TIMEF_SEC )
        bSec = TRUE;
    if ( GetFormat() == TIMEF_100TH_SEC || TIMEF_SEC_CS )
        bSec = b100Sec = TRUE;
    return ComboBox::GetEntryPos( ImplGetLocaleDataWrapper().getTime( rTime, bSec, b100Sec ) );
}
