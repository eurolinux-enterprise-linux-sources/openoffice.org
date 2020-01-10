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

#ifndef _BIGINT_HXX
#define _TOOLS_BIGINT
#include "tools/bigint.hxx"
#endif

#include "tools/debug.hxx"

#include "tools/rc.h"
#include "tools/resary.hxx"
#include "vcl/svids.hrc"
#include "vcl/field.hxx"
#include "vcl/event.hxx"
#include "vcl/svapp.hxx"
#include "vcl/svdata.hxx"
#include "vcl/unohelp.hxx"
#include "i18nutil/unicode.hxx"

#include "rtl/math.hxx"


#include <unotools/localedatawrapper.hxx>

using namespace ::com::sun::star;

static ResStringArray *strAllUnits = NULL;

// -----------------------------------------------------------------------

#define FORMAT_NUMERIC       1
#define FORMAT_METRIC        2
#define FORMAT_CURRENCY      3

// -----------------------------------------------------------------------

static sal_Int64 ImplPower10( USHORT n )
{
    USHORT i;
    sal_Int64   nValue = 1;

    for ( i=0; i < n; i++ )
        nValue *= 10;

    return nValue;
}

// -----------------------------------------------------------------------

static BOOL ImplNumericProcessKeyInput( Edit*, const KeyEvent& rKEvt,
                                        BOOL bStrictFormat, BOOL bThousandSep,
                                        const LocaleDataWrapper& rLocaleDataWrappper )
{
    if ( !bStrictFormat )
        return FALSE;
    else
    {
        xub_Unicode cChar = rKEvt.GetCharCode();
        USHORT      nGroup = rKEvt.GetKeyCode().GetGroup();

        if ( (nGroup == KEYGROUP_FKEYS) || (nGroup == KEYGROUP_CURSOR) ||
             (nGroup == KEYGROUP_MISC) ||
             ((cChar >= '0') && (cChar <= '9')) ||
             (cChar == rLocaleDataWrappper.getNumDecimalSep() ) ||
             (bThousandSep && (cChar == rLocaleDataWrappper.getNumThousandSep())) ||
             (cChar == '-') )
            return FALSE;
        else
            return TRUE;
    }
}

// -----------------------------------------------------------------------

static BOOL ImplNumericGetValue( const XubString& rStr, double& rValue,
                                 USHORT nDecDigits, const LocaleDataWrapper& rLocaleDataWrappper,
                                 BOOL bCurrency = FALSE )
{
    XubString   aStr = rStr;
    XubString   aStr1;
    XubString   aStr2;
    BOOL        bNegative = FALSE;
    xub_StrLen  nDecPos;
    xub_StrLen  i;

    // Reaktion auf leeren String
    if ( !rStr.Len() )
        return FALSE;

    // Fuehrende und nachfolgende Leerzeichen entfernen
    aStr.EraseLeadingAndTrailingChars( ' ' );

    // Position des Dezimalpunktes suchen
    nDecPos = aStr.Search( rLocaleDataWrappper.getNumDecimalSep() );
    if ( nDecPos != STRING_NOTFOUND )
    {
        aStr1 = aStr.Copy( 0, nDecPos );
        aStr2 = aStr.Copy( nDecPos+1 );
    }
    else
        aStr1 = aStr;

    // Negativ ?
    if ( bCurrency )
    {
        if ( (aStr.GetChar( 0 ) == '(') && (aStr.GetChar( aStr.Len()-1 ) == ')') )
            bNegative = TRUE;
        if ( !bNegative )
        {
            for ( i=0; i < aStr.Len(); i++ )
            {
                if ( (aStr.GetChar( i ) >= '0') && (aStr.GetChar( i ) <= '9') )
                    break;
                else if ( aStr.GetChar( i ) == '-' )
                {
                    bNegative = TRUE;
                    break;
                }
            }
        }
        if ( !bNegative && bCurrency && aStr.Len() )
        {
            USHORT nFormat = rLocaleDataWrappper.getCurrNegativeFormat();
            if ( (nFormat == 3) || (nFormat == 6)  ||
                 (nFormat == 7) || (nFormat == 10) )
            {
                for ( i = (xub_StrLen)(aStr.Len()-1); i > 0; i++ )
                {
                    if ( (aStr.GetChar( i ) >= '0') && (aStr.GetChar( i ) <= '9') )
                        break;
                    else if ( aStr.GetChar( i ) == '-' )
                    {
                        bNegative = TRUE;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        if ( aStr1.GetChar( 0 ) == '-' )
            bNegative = TRUE;
    }

    // Alle unerwuenschten Zeichen rauswerfen
    for ( i=0; i < aStr1.Len(); )
    {
        if ( (aStr1.GetChar( i ) >= '0') && (aStr1.GetChar( i ) <= '9') )
            i++;
        else
            aStr1.Erase( i, 1 );
    }
    for ( i=0; i < aStr2.Len(); )
    {
        if ( (aStr2.GetChar( i ) >= '0') && (aStr2.GetChar( i ) <= '9') )
            i++;
        else
            aStr2.Erase( i, 1 );
    }

    if ( !aStr1.Len() && !aStr2.Len() )
        return FALSE;

    if ( !aStr1.Len() )
        aStr1.Insert( '0' );
    if ( bNegative )
        aStr1.Insert( '-', 0 );

    // Nachkommateil zurechtstutzen und dabei runden
    BOOL bRound = FALSE;
    if ( aStr2.Len() > nDecDigits )
    {
        if ( aStr2.GetChar( nDecDigits ) >= '5' )
            bRound = TRUE;
        aStr2.Erase( nDecDigits );
    }
    if ( aStr2.Len() < nDecDigits )
        aStr2.Expand( nDecDigits, '0' );

    aStr  = aStr1;
    aStr += aStr2;

    // Bereichsueberpruefung
    double nValue = aStr.ToDouble();
    if ( bRound )
    {
        if ( !bNegative )
            nValue++;
        else
            nValue--;
    }

    rValue = nValue;

    return TRUE;
}

static void ImplUpdateSeparators( const String& rOldDecSep, const String& rNewDecSep,
                                  const String& rOldThSep, const String& rNewThSep,
                                  Edit* pEdit )
{
    bool bChangeDec = (rOldDecSep != rNewDecSep);
    bool bChangeTh = (rOldThSep != rNewThSep );
    
    if( bChangeDec || bChangeTh )
    {
        BOOL bUpdateMode = pEdit->IsUpdateMode();
        pEdit->SetUpdateMode( FALSE );
        String aText = pEdit->GetText();
        if( bChangeDec )
            aText.SearchAndReplaceAll( rNewDecSep, rOldDecSep );
        if( bChangeTh )
            aText.SearchAndReplaceAll( rNewThSep, rOldThSep );
        pEdit->SetText( aText );
        
        ComboBox* pCombo = dynamic_cast<ComboBox*>(pEdit);
        if( pCombo )
        {
            // update box entries
            USHORT nEntryCount = pCombo->GetEntryCount();
            for ( USHORT i=0; i < nEntryCount; i++ )
            {
                aText = pCombo->GetEntry( i );
                if( bChangeDec )
                    aText.SearchAndReplaceAll( rNewDecSep, rOldDecSep );
                if( bChangeTh )
                    aText.SearchAndReplaceAll( rNewThSep, rOldThSep );
                pCombo->RemoveEntry( i );
                pCombo->InsertEntry( aText, i );
            }
        }
        if( bUpdateMode )
            pEdit->SetUpdateMode( bUpdateMode );
    }
}

// -----------------------------------------------------------------------

FormatterBase::FormatterBase( Edit* pField )
{
    mpField                     = pField;
    mpLocaleDataWrapper         = NULL;
    mbReformat                  = FALSE;
    mbStrictFormat              = FALSE;
    mbEmptyFieldValue           = FALSE;
    mbEmptyFieldValueEnabled    = FALSE;
    mbDefaultLocale             = TRUE;
}

// -----------------------------------------------------------------------

FormatterBase::~FormatterBase()
{
    delete mpLocaleDataWrapper;
}

// -----------------------------------------------------------------------

LocaleDataWrapper& FormatterBase::ImplGetLocaleDataWrapper() const
{
    if ( !mpLocaleDataWrapper )
    {
        ((FormatterBase*)this)->mpLocaleDataWrapper = new LocaleDataWrapper( vcl::unohelper::GetMultiServiceFactory(), GetLocale() );
    }
    return *mpLocaleDataWrapper;
}

const LocaleDataWrapper& FormatterBase::GetLocaleDataWrapper() const
{
    return ImplGetLocaleDataWrapper();
}

// -----------------------------------------------------------------------

void FormatterBase::Reformat()
{
}

// -----------------------------------------------------------------------

void FormatterBase::ReformatAll()
{
    Reformat();
};

// -----------------------------------------------------------------------

void FormatterBase::SetStrictFormat( BOOL bStrict )
{
    if ( bStrict != mbStrictFormat )
    {
        mbStrictFormat = bStrict;
        if ( mbStrictFormat )
            ReformatAll();
    }
}

// -----------------------------------------------------------------------

void FormatterBase::SetLocale( const lang::Locale& rLocale )
{
    ImplGetLocaleDataWrapper().setLocale( rLocale );
    mbDefaultLocale = FALSE;
    ReformatAll();
}

// -----------------------------------------------------------------------

const lang::Locale& FormatterBase::GetLocale() const
{
    if ( !mpLocaleDataWrapper || mbDefaultLocale )
    {
        if ( mpField )
            return mpField->GetSettings().GetLocale();
        else
            return Application::GetSettings().GetLocale();
    }

    return mpLocaleDataWrapper->getLocale();
}

// -----------------------------------------------------------------------

const AllSettings& FormatterBase::GetFieldSettings() const
{
    if ( mpField )
        return mpField->GetSettings();
    else
        return Application::GetSettings();
}

// -----------------------------------------------------------------------

void FormatterBase::SetFieldText( const XubString& rText, BOOL bKeepSelection )
{
    if ( mpField )
    {
        Selection aNewSelection( 0xFFFF, 0xFFFF );
        if ( bKeepSelection )
            aNewSelection = mpField->GetSelection();

        ImplSetText( rText, &aNewSelection );
    }
}

// -----------------------------------------------------------------------

void FormatterBase::ImplSetText( const XubString& rText, Selection* pNewSelection )
{
    if ( mpField )
    {
        if ( pNewSelection )
            mpField->SetText( rText, *pNewSelection );
        else
        {
            Selection aSel = mpField->GetSelection();
            aSel.Min() = aSel.Max();
            mpField->SetText( rText, aSel );
        }

        MarkToBeReformatted( FALSE );
    }
}

// -----------------------------------------------------------------------

void FormatterBase::SetEmptyFieldValue()
{
    if ( mpField )
        mpField->SetText( ImplGetSVEmptyStr() );
    mbEmptyFieldValue = TRUE;
}

// -----------------------------------------------------------------------

BOOL FormatterBase::IsEmptyFieldValue() const
{
    return (!mpField || !mpField->GetText().Len());
}

// -----------------------------------------------------------------------

BOOL NumericFormatter::ImplNumericReformat( const XubString& rStr, double& rValue,
                                            XubString& rOutStr )
{
    if ( !ImplNumericGetValue( rStr, rValue, GetDecimalDigits(), ImplGetLocaleDataWrapper() ) )
        return TRUE;
    else
    {
        double nTempVal = rValue;
        // caution: precision loss in double cast
        if ( nTempVal > mnMax )
            nTempVal = (double)mnMax;
        else if ( nTempVal < mnMin )
            nTempVal = (double)mnMin;

        if ( GetErrorHdl().IsSet() && (rValue != nTempVal) )
        {
            mnCorrectedValue = (sal_Int64)nTempVal;
            if ( !GetErrorHdl().Call( this ) )
            {
                mnCorrectedValue = 0;
                return FALSE;
            }
            else
                mnCorrectedValue = 0;
        }

        rOutStr = CreateFieldText( (sal_Int64)nTempVal );
        return TRUE;
    }
}

// -----------------------------------------------------------------------

void NumericFormatter::ImplInit()
{
    mnFieldValue        = 0;
    mnLastValue         = 0;
    mnMin               = 0;
    mnMax               = 0x7FFFFFFFFFFFFFFFLL;
    mnCorrectedValue    = 0;
    mnDecimalDigits     = 2;
    mnType              = FORMAT_NUMERIC;
    mbThousandSep       = TRUE;
    mbShowTrailingZeros = TRUE;

    // for fields
    mnSpinSize          = 1;
    mnFirst             = mnMin;
    mnLast              = mnMax;

    SetDecimalDigits( 0 );
}

// -----------------------------------------------------------------------

NumericFormatter::NumericFormatter()
{
    ImplInit();
}

// -----------------------------------------------------------------------

void NumericFormatter::ImplLoadRes( const ResId& rResId )
{
    ResMgr*     pMgr = rResId.GetResMgr();

    if( pMgr )
    {
        ULONG nMask = pMgr->ReadLong();
    
        if ( NUMERICFORMATTER_MIN & nMask )
            mnMin = pMgr->ReadLong();
    
        if ( NUMERICFORMATTER_MAX & nMask )
            mnMax = pMgr->ReadLong();
    
        if ( NUMERICFORMATTER_STRICTFORMAT & nMask )
            SetStrictFormat( (BOOL)pMgr->ReadShort() );
    
        if ( NUMERICFORMATTER_DECIMALDIGITS & nMask )
            SetDecimalDigits( pMgr->ReadShort() );
    
        if ( NUMERICFORMATTER_VALUE & nMask )
        {
            mnFieldValue = pMgr->ReadLong();
            if ( mnFieldValue > mnMax )
                mnFieldValue = mnMax;
            else if ( mnFieldValue < mnMin )
                mnFieldValue = mnMin;
            mnLastValue = mnFieldValue;
        }

        if ( NUMERICFORMATTER_NOTHOUSANDSEP & nMask )
            SetUseThousandSep( !(BOOL)pMgr->ReadShort() );
    }
}

// -----------------------------------------------------------------------

NumericFormatter::~NumericFormatter()
{
}

// -----------------------------------------------------------------------

void NumericFormatter::SetMin( sal_Int64 nNewMin )
{
    mnMin = nNewMin;
    if ( !IsEmptyFieldValue() )
        ReformatAll();
}

// -----------------------------------------------------------------------

void NumericFormatter::SetMax( sal_Int64 nNewMax )
{
    mnMax = nNewMax;
    if ( !IsEmptyFieldValue() )
        ReformatAll();
}

// -----------------------------------------------------------------------

void NumericFormatter::SetUseThousandSep( BOOL b )
{
    mbThousandSep = b;
    ReformatAll();
}

// -----------------------------------------------------------------------

void NumericFormatter::SetDecimalDigits( USHORT nDigits )
{
    mnDecimalDigits = nDigits;
    ReformatAll();
}

// -----------------------------------------------------------------------

void NumericFormatter::SetShowTrailingZeros( BOOL bShowTrailingZeros )
{
    if ( mbShowTrailingZeros != bShowTrailingZeros )
    {
        mbShowTrailingZeros = bShowTrailingZeros;
        ReformatAll();
    }
}

// -----------------------------------------------------------------------

USHORT NumericFormatter::GetDecimalDigits() const
{
    return mnDecimalDigits;
}

// -----------------------------------------------------------------------

void NumericFormatter::SetValue( sal_Int64 nNewValue )
{
    SetUserValue( nNewValue );
    mnFieldValue = mnLastValue;
    SetEmptyFieldValueData( FALSE );
}

// -----------------------------------------------------------------------

XubString NumericFormatter::CreateFieldText( sal_Int64 nValue ) const
{
    return ImplGetLocaleDataWrapper().getNum( nValue, GetDecimalDigits(), IsUseThousandSep(), IsShowTrailingZeros() );
}

// -----------------------------------------------------------------------

void NumericFormatter::ImplSetUserValue( sal_Int64 nNewValue, Selection* pNewSelection )
{
    if ( nNewValue > mnMax )
        nNewValue = mnMax;
    else if ( nNewValue < mnMin )
        nNewValue = mnMin;
    mnLastValue = nNewValue;

    if ( GetField() )
        ImplSetText( CreateFieldText( nNewValue ), pNewSelection );
}

// -----------------------------------------------------------------------

void NumericFormatter::SetUserValue( sal_Int64 nNewValue )
{
    ImplSetUserValue( nNewValue );
}

// -----------------------------------------------------------------------

sal_Int64 NumericFormatter::GetValue() const
{
    if ( !GetField() )
        return 0;

    double nTempValue;

    if ( ImplNumericGetValue( GetField()->GetText(), nTempValue,
                              GetDecimalDigits(), ImplGetLocaleDataWrapper() ) )
    {
        // caution: precision loss in double cast
        if ( nTempValue > mnMax )
            nTempValue = (double)mnMax;
        else if ( nTempValue < mnMin )
            nTempValue = (double)mnMin;
        return (sal_Int64)nTempValue;
    }
    else
        return mnLastValue;
}

// -----------------------------------------------------------------------

BOOL NumericFormatter::IsValueModified() const
{
    if ( ImplGetEmptyFieldValue() )
        return !IsEmptyFieldValue();
    else if ( GetValue() != mnFieldValue )
        return TRUE;
    else
        return FALSE;
}

// -----------------------------------------------------------------------

Fraction NumericFormatter::ConvertToFraction( sal_Int64 nValue )
{
    // caution: precision loss in double cast (and in fraction anyhow)
    return Fraction( (double)nValue/(double)ImplPower10( GetDecimalDigits() ) );
}

// -----------------------------------------------------------------------

sal_Int64 NumericFormatter::ConvertToLong( const Fraction& rValue )
{
    Fraction aFract = rValue;
    aFract *= Fraction( (long)ImplPower10( GetDecimalDigits() ), 1 );
    return (sal_Int64)(double)aFract;
}

// -----------------------------------------------------------------------

sal_Int64 NumericFormatter::Normalize( sal_Int64 nValue ) const
{
    return (nValue * ImplPower10( GetDecimalDigits() ) );
}

// -----------------------------------------------------------------------

sal_Int64 NumericFormatter::Denormalize( sal_Int64 nValue ) const
{
    sal_Int64 nFactor = ImplPower10( GetDecimalDigits() );
    if( nValue < 0 )
        return ((nValue-(nFactor/2)) / nFactor );
    else
        return ((nValue+(nFactor/2)) / nFactor );
}

// -----------------------------------------------------------------------

void NumericFormatter::Reformat()
{
    if ( !GetField() )
        return;

    if ( !GetField()->GetText().Len() && ImplGetEmptyFieldValue() )
        return;

    XubString aStr;
    // caution: precision loss in double cast
    double nTemp = (double)mnLastValue;
    BOOL bOK = ImplNumericReformat( GetField()->GetText(), nTemp, aStr );
    mnLastValue = (sal_Int64)nTemp;
    if ( !bOK )
        return;

    if ( aStr.Len() )
        ImplSetText( aStr );
    else
        SetValue( mnLastValue );
}

// -----------------------------------------------------------------------

void NumericFormatter::FieldUp()
{
    sal_Int64 nValue = GetValue();
    nValue += mnSpinSize;
    if ( nValue > mnMax )
        nValue = mnMax;

    ImplNewFieldValue( nValue );
}

// -----------------------------------------------------------------------

void NumericFormatter::FieldDown()
{
    sal_Int64 nValue = GetValue();
    nValue -= mnSpinSize;
    if ( nValue < mnMin )
        nValue = mnMin;

    ImplNewFieldValue( nValue );
}

// -----------------------------------------------------------------------

void NumericFormatter::FieldFirst()
{
    ImplNewFieldValue( mnFirst );
}

// -----------------------------------------------------------------------

void NumericFormatter::FieldLast()
{
    ImplNewFieldValue( mnLast );
}

// -----------------------------------------------------------------------

void NumericFormatter::ImplNewFieldValue( sal_Int64 nNewValue )
{
    if ( GetField() )
    {
        // !!! TH-18.2.99: Wenn wir Zeit haben sollte mal geklaert werden,
        // !!! warum nicht bei ImplSetUserValue() geprueft wird, ob
        // !!! sich der Wert aendert. Denn auch hier muesste dieses
        // !!! gemacht werden, da ansonsten der Modify-Aufruf
        // !!! nicht gemacht werden duerfte. Jedenfalls sollten die
        // !!! Wege von ImplNewFieldValue, ImplSetUserValue und
        // !!! ImplSetText ueberprueft und klarer gestalltet (mit Kommentar)
        // !!! werden, damit wir mal wissen, was dort ablaeuft!!!

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

        sal_Int64 nOldLastValue  = mnLastValue;
        ImplSetUserValue( nNewValue, &aSelection );
        mnLastValue = nOldLastValue;

        // Modify am Edit wird nur bei KeyInput gesetzt...
        if ( GetField()->GetText() != aText )
        {
            GetField()->SetModifyFlag();
            GetField()->Modify();
        }
    }
}

// -----------------------------------------------------------------------

NumericField::NumericField( Window* pParent, WinBits nWinStyle ) :
    SpinField( pParent, nWinStyle )
{
    SetField( this );
    Reformat();
}

// -----------------------------------------------------------------------

NumericField::NumericField( Window* pParent, const ResId& rResId ) :
    SpinField( WINDOW_NUMERICFIELD )
{
    rResId.SetRT( RSC_NUMERICFIELD );
    WinBits nStyle = ImplInitRes( rResId ) ;
    SpinField::ImplInit( pParent, nStyle );
    SetField( this );
    ImplLoadRes( rResId );
    Reformat();

    if ( !(nStyle & WB_HIDE ) )
        Show();
}

// -----------------------------------------------------------------------

void NumericField::ImplLoadRes( const ResId& rResId )
{
    SpinField::ImplLoadRes( rResId );
    NumericFormatter::ImplLoadRes( ResId( (RSHEADER_TYPE *)GetClassRes(), *rResId.GetResMgr() ) );

    ULONG      nMask = ReadLongRes();

    if ( NUMERICFIELD_FIRST & nMask )
        mnFirst = ReadLongRes();

    if ( NUMERICFIELD_LAST & nMask )
        mnLast = ReadLongRes();

    if ( NUMERICFIELD_SPINSIZE & nMask )
        mnSpinSize = ReadLongRes();
}

// -----------------------------------------------------------------------

NumericField::~NumericField()
{
}

// -----------------------------------------------------------------------

long NumericField::PreNotify( NotifyEvent& rNEvt )
{
        if ( (rNEvt.GetType() == EVENT_KEYINPUT) && !rNEvt.GetKeyEvent()->GetKeyCode().IsMod2() )
    {
        if ( ImplNumericProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), IsStrictFormat(), IsUseThousandSep(), ImplGetLocaleDataWrapper() ) )
            return 1;
    }

    return SpinField::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

long NumericField::Notify( NotifyEvent& rNEvt )
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

void NumericField::DataChanged( const DataChangedEvent& rDCEvt )
{
    SpinField::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) && (rDCEvt.GetFlags() & SETTINGS_LOCALE) )
    {
        String sOldDecSep = ImplGetLocaleDataWrapper().getNumDecimalSep();
        String sOldThSep = ImplGetLocaleDataWrapper().getNumThousandSep();
        if ( IsDefaultLocale() )
            ImplGetLocaleDataWrapper().setLocale( GetSettings().GetLocale() );
        String sNewDecSep = ImplGetLocaleDataWrapper().getNumDecimalSep();
        String sNewThSep = ImplGetLocaleDataWrapper().getNumThousandSep();
        ImplUpdateSeparators( sOldDecSep, sNewDecSep, sOldThSep, sNewThSep, this );
        ReformatAll();
    }
}

// -----------------------------------------------------------------------

void NumericField::Modify()
{
    MarkToBeReformatted( TRUE );
    SpinField::Modify();
}

// -----------------------------------------------------------------------

void NumericField::Up()
{
    FieldUp();
    SpinField::Up();
}

// -----------------------------------------------------------------------

void NumericField::Down()
{
    FieldDown();
    SpinField::Down();
}

// -----------------------------------------------------------------------

void NumericField::First()
{
    FieldFirst();
    SpinField::First();
}

// -----------------------------------------------------------------------

void NumericField::Last()
{
    FieldLast();
    SpinField::Last();
}

// -----------------------------------------------------------------------

NumericBox::NumericBox( Window* pParent, WinBits nWinStyle ) :
    ComboBox( pParent, nWinStyle )
{
    SetField( this );
    Reformat();
}

// -----------------------------------------------------------------------

NumericBox::NumericBox( Window* pParent, const ResId& rResId ) :
    ComboBox( WINDOW_NUMERICBOX )
{
    rResId.SetRT( RSC_NUMERICBOX );
    WinBits nStyle = ImplInitRes( rResId );
    ComboBox::ImplInit( pParent, nStyle );
    SetField( this );
    ComboBox::ImplLoadRes( rResId );
    NumericFormatter::ImplLoadRes( ResId( (RSHEADER_TYPE *)GetClassRes(), *rResId.GetResMgr() ) );
    Reformat();

    if ( !(nStyle & WB_HIDE ) )
        Show();
}

// -----------------------------------------------------------------------

NumericBox::~NumericBox()
{
}

// -----------------------------------------------------------------------

long NumericBox::PreNotify( NotifyEvent& rNEvt )
{
    if ( (rNEvt.GetType() == EVENT_KEYINPUT) && !rNEvt.GetKeyEvent()->GetKeyCode().IsMod2() )
    {
        if ( ImplNumericProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), IsStrictFormat(), IsUseThousandSep(), ImplGetLocaleDataWrapper() ) )
            return 1;
    }

    return ComboBox::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

long NumericBox::Notify( NotifyEvent& rNEvt )
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

void NumericBox::DataChanged( const DataChangedEvent& rDCEvt )
{
    ComboBox::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) && (rDCEvt.GetFlags() & SETTINGS_LOCALE) )
    {
        String sOldDecSep = ImplGetLocaleDataWrapper().getNumDecimalSep();
        String sOldThSep = ImplGetLocaleDataWrapper().getNumThousandSep();
        if ( IsDefaultLocale() )
            ImplGetLocaleDataWrapper().setLocale( GetSettings().GetLocale() );
        String sNewDecSep = ImplGetLocaleDataWrapper().getNumDecimalSep();
        String sNewThSep = ImplGetLocaleDataWrapper().getNumThousandSep();
        ImplUpdateSeparators( sOldDecSep, sNewDecSep, sOldThSep, sNewThSep, this );
        ReformatAll();
    }
}

// -----------------------------------------------------------------------

void NumericBox::Modify()
{
    MarkToBeReformatted( TRUE );
    ComboBox::Modify();
}

// -----------------------------------------------------------------------

void NumericBox::ReformatAll()
{
    double nValue;
    XubString aStr;
    SetUpdateMode( FALSE );
    USHORT nEntryCount = GetEntryCount();
    for ( USHORT i=0; i < nEntryCount; i++ )
    {
        ImplNumericReformat( GetEntry( i ), nValue, aStr );
        RemoveEntry( i );
        InsertEntry( aStr, i );
    }
    NumericFormatter::Reformat();
    SetUpdateMode( TRUE );
}

// -----------------------------------------------------------------------

void NumericBox::InsertValue( sal_Int64 nValue, USHORT nPos )
{
    ComboBox::InsertEntry( CreateFieldText( nValue ), nPos );
}

// -----------------------------------------------------------------------

void NumericBox::RemoveValue( sal_Int64 nValue )
{
    ComboBox::RemoveEntry( CreateFieldText( nValue ) );
}

// -----------------------------------------------------------------------

sal_Int64 NumericBox::GetValue( USHORT nPos ) const
{
    double nValue = 0;
    ImplNumericGetValue( ComboBox::GetEntry( nPos ), nValue, GetDecimalDigits(), ImplGetLocaleDataWrapper() );
    return (sal_Int64)nValue;
}

// -----------------------------------------------------------------------

USHORT NumericBox::GetValuePos( sal_Int64 nValue ) const
{
    return ComboBox::GetEntryPos( CreateFieldText( nValue ) );
}

// -----------------------------------------------------------------------

static BOOL ImplMetricProcessKeyInput( Edit* pEdit, const KeyEvent& rKEvt,
                                       BOOL, BOOL bUseThousandSep, const LocaleDataWrapper& rWrapper )
{
    // Es gibt hier kein sinnvolles StrictFormat, also alle
    // Zeichen erlauben
    return ImplNumericProcessKeyInput( pEdit, rKEvt, FALSE, bUseThousandSep, rWrapper );
}

// -----------------------------------------------------------------------

static XubString ImplMetricGetUnitText( const XubString& rStr )
{
    // Einheitentext holen
    XubString aStr;
    for ( short i = rStr.Len()-1; i >= 0; i-- )
    {
        xub_Unicode c = rStr.GetChar( i );
        if ( unicode::isAlpha( c ) ||
             (c == '\'') || (c == '\"') || (c == '%' ) )
            aStr.Insert( c, 0 );
        else
        {
            if ( aStr.Len() )
                break;
        }
    }
    return aStr;
    
/*
    // MT: #90545# Preparation for translated strings...
    String aMetricText;
    for ( USHORT n = rStr.Len(); n; )
    {
        sal_Unicode c = rStr.GetChar( --n );
        sal_Int32 nType = xCharClass->getStringType( rStr, n, 1, rLocale );

        if ( CharClass::isLetterType( nType ) )
        {
            aMetricText.Insert( c, 0 );
        }
        else
        {
            if ( aMetricText.Len() )
                break;
        }
    }
*/    
}

// -----------------------------------------------------------------------

// #104355# support localized mesaurements

static String ImplMetricToString( FieldUnit rUnit )
{
    if( !strAllUnits )
    {
        ResMgr* pResMgr = ImplGetResMgr();
        strAllUnits = new ResStringArray( ResId (SV_FUNIT_STRINGS, *pResMgr) );
    }
    // return unit's default string (ie, the first one )
    for( USHORT i=0; i < strAllUnits->Count(); i++ )
        if( (FieldUnit) strAllUnits->GetValue( i ) == rUnit )
            return strAllUnits->GetString( i );

    return String();
}

static FieldUnit ImplStringToMetric( const String &rMetricString )
{
    if( !strAllUnits )
    {
        ResMgr* pResMgr = ImplGetResMgr();
        strAllUnits = new ResStringArray( ResId (SV_FUNIT_STRINGS, *pResMgr) );
    }
    // return FieldUnit
    String aStr( rMetricString );
    aStr.ToLowerAscii();
    for( USHORT i=0; i < strAllUnits->Count(); i++ )
        if ( strAllUnits->GetString( i ).Equals( aStr ) )
            return (FieldUnit) strAllUnits->GetValue( i );

    return FUNIT_NONE;
}

// -----------------------------------------------------------------------

static FieldUnit ImplMetricGetUnit( const XubString& rStr )
{
    XubString aStr = ImplMetricGetUnitText( rStr );
    return ImplStringToMetric( aStr );
}

#define K *1000L
#define M *1000000L
#define X *5280L

static const sal_Int64 aImplFactor[FUNIT_MILE+1][FUNIT_MILE+1] =
{ /*
mm/100    mm    cm       m     km  twip point  pica  inch    foot     mile */
{    1,  100,  1 K,  100 K, 100 M, 2540, 2540, 2540, 2540,2540*12,2540*12 X },
{    1,    1,   10,    1 K,   1 M, 2540, 2540, 2540, 2540,2540*12,2540*12 X },
{    1,    1,    1,    100, 100 K,  254,  254,  254,  254, 254*12, 254*12 X },
{    1,    1,    1,      1,   1 K,  254,  254,  254,  254, 254*12, 254*12 X },
{    1,    1,    1,      1,     1,    0,  254,  254,  254, 254*12, 254*12 X },
{ 1440,144 K,144 K,14400 K,     0,    1,   20,  240, 1440,1440*12,1440*12 X },
{   72, 7200, 7200,  720 K, 720 M,    1,    1,   12,   72,  72*12,  72*12 X },
{    6,  600,  600,   60 K,  60 M,    1,    1,    1,    6,   6*12,   6*12 X },
{    1,  100,  100,   10 K,  10 M,    1,    1,    1,    1,     12,     12 X },
{    1,  100,  100,   10 K,  10 M,    1,    1,    1,    1,      1,      1 X },
{    1,  100,  100,   10 K,  10 M,    1,    1,    1,    1,      1,        1 }
};

#undef X
#undef M
#undef K
// twip in km 254/14400 M

static FieldUnit eDefaultUnit = FUNIT_NONE;

FieldUnit MetricField::GetDefaultUnit() { return eDefaultUnit; }
void MetricField::SetDefaultUnit( FieldUnit meUnit ) { eDefaultUnit = meUnit; }

static FieldUnit ImplMap2FieldUnit( MapUnit meUnit, long& nDecDigits )
{
    switch( meUnit )
    {
        case MAP_100TH_MM :
            nDecDigits -= 2;
            return FUNIT_MM;
        case MAP_10TH_MM :
            nDecDigits -= 1;
            return FUNIT_MM;
        case MAP_MM :
            return FUNIT_MM;
        case MAP_CM :
            return FUNIT_CM;
        case MAP_1000TH_INCH :
            nDecDigits -= 3;
            return FUNIT_INCH;
        case MAP_100TH_INCH :
            nDecDigits -= 2;
            return FUNIT_INCH;
        case MAP_10TH_INCH :
            nDecDigits -= 1;
            return FUNIT_INCH;
        case MAP_INCH :
            return FUNIT_INCH;
        case MAP_POINT :
            return FUNIT_POINT;
        case MAP_TWIP :
            return FUNIT_TWIP;
        default:
            DBG_ERROR( "default eInUnit" );
            break;
    }
    return FUNIT_NONE;
}

// -----------------------------------------------------------------------

static double nonValueDoubleToValueDouble( double nValue )
{
    return rtl::math::isFinite( nValue ) ? nValue : 0.0;
}

sal_Int64 MetricField::ConvertValue( sal_Int64 nValue, sal_Int64 mnBaseValue, USHORT nDecDigits,
                                     FieldUnit eInUnit, FieldUnit eOutUnit )
{
    // caution: precision loss in double cast
    return static_cast<sal_Int64>(
        // #150733# cast double to sal_Int64 can throw a
        // EXCEPTION_FLT_INVALID_OPERATION on Windows
        nonValueDoubleToValueDouble(
            ConvertDoubleValue( (double)nValue, mnBaseValue, nDecDigits,
                                eInUnit, eOutUnit ) ) );
}

// -----------------------------------------------------------------------

sal_Int64 MetricField::ConvertValue( sal_Int64 nValue, USHORT nDigits,
                                     MapUnit eInUnit, FieldUnit eOutUnit )
{
    return static_cast<sal_Int64>(
        // #150733# cast double to sal_Int64 can throw a
        // EXCEPTION_FLT_INVALID_OPERATION on Windows
        nonValueDoubleToValueDouble(
            ConvertDoubleValue( nValue, nDigits, eInUnit, eOutUnit ) ) );
}

// -----------------------------------------------------------------------

sal_Int64 MetricField::ConvertValue( sal_Int64 nValue, USHORT nDigits,
                                     FieldUnit eInUnit, MapUnit eOutUnit )
{
    return static_cast<sal_Int64>(
        // #150733# cast double to sal_Int64 can throw a
        // EXCEPTION_FLT_INVALID_OPERATION on Windows
        nonValueDoubleToValueDouble(
            ConvertDoubleValue( nValue, nDigits, eInUnit, eOutUnit ) ) );
}

// -----------------------------------------------------------------------

double MetricField::ConvertDoubleValue( double nValue, sal_Int64 mnBaseValue, USHORT nDecDigits,
                                        FieldUnit eInUnit, FieldUnit eOutUnit )
{
    if ( eInUnit != eOutUnit )
    {
        sal_Int64 nMult = 1, nDiv = 1;

        if ( eInUnit == FUNIT_PERCENT )
        {
            if ( (mnBaseValue <= 0) || (nValue <= 0) )
                return nValue;
            nDiv = 100;
            for ( USHORT i=0; i < nDecDigits; i++ )
                nDiv *= 10;

            nMult = mnBaseValue;
        }
        else if ( eOutUnit == FUNIT_PERCENT ||
                  eOutUnit == FUNIT_CUSTOM ||
                  eOutUnit == FUNIT_NONE ||
                  eInUnit  == FUNIT_CUSTOM ||
                  eInUnit  == FUNIT_NONE )
             return nValue;
        else
        {
            if ( eOutUnit == FUNIT_100TH_MM )
                eOutUnit = FUNIT_NONE;
            if ( eInUnit == FUNIT_100TH_MM )
                eInUnit = FUNIT_NONE;

            nDiv  = aImplFactor[eInUnit][eOutUnit];
            nMult = aImplFactor[eOutUnit][eInUnit];

            DBG_ASSERT( nMult > 0, "illegal *" );
            DBG_ASSERT( nDiv  > 0, "illegal /" );
        }

        if ( nMult != 1 && nMult > 0 )
            nValue *= nMult;
        if ( nDiv != 1 && nDiv > 0 )
        {
            nValue += ( nValue < 0 ) ? (-nDiv/2) : (nDiv/2);
            nValue /= nDiv;
        }
    }

    return nValue;
}

// -----------------------------------------------------------------------

double MetricField::ConvertDoubleValue( double nValue, USHORT nDigits,
                                        MapUnit eInUnit, FieldUnit eOutUnit )
{
    if ( eOutUnit == FUNIT_PERCENT ||
         eOutUnit == FUNIT_CUSTOM ||
         eOutUnit == FUNIT_NONE ||
         eInUnit == MAP_PIXEL ||
         eInUnit == MAP_SYSFONT ||
         eInUnit == MAP_APPFONT ||
         eInUnit == MAP_RELATIVE )
    {
        DBG_ERROR( "invalid parameters" );
        return nValue;
    }

    long nDecDigits = nDigits;
    FieldUnit eFieldUnit = ImplMap2FieldUnit( eInUnit, nDecDigits );

    if ( nDecDigits < 0 )
    {
        while ( nDecDigits )
        {
            nValue += 5;
            nValue /= 10;
            nDecDigits++;
        }
    }
    else
    {
        while ( nDecDigits )
        {
            nValue *= 10;
            nDecDigits--;
        }
    }

    if ( eFieldUnit != eOutUnit )
    {
        sal_Int64 nDiv  = aImplFactor[eFieldUnit][eOutUnit];
        sal_Int64 nMult = aImplFactor[eOutUnit][eFieldUnit];

        DBG_ASSERT( nMult > 0, "illegal *" );
        DBG_ASSERT( nDiv  > 0, "illegal /" );

        if ( nMult != 1 && nMult > 0)
            nValue *= nMult;
        if ( nDiv != 1 && nDiv > 0 )
        {
            nValue += (nValue < 0) ? (-nDiv/2) : (nDiv/2);
            nValue /= nDiv;
        }
    }
    return nValue;
}

// -----------------------------------------------------------------------

double MetricField::ConvertDoubleValue( double nValue, USHORT nDigits,
                                        FieldUnit eInUnit, MapUnit eOutUnit )
{
    if ( eInUnit == FUNIT_PERCENT ||
         eInUnit == FUNIT_CUSTOM ||
         eInUnit == FUNIT_NONE ||
         eOutUnit == MAP_PIXEL ||
         eOutUnit == MAP_SYSFONT ||
         eOutUnit == MAP_APPFONT ||
         eOutUnit == MAP_RELATIVE )
    {
        DBG_ERROR( "invalid parameters" );
        return nValue;
    }

    long nDecDigits = nDigits;
    FieldUnit eFieldUnit = ImplMap2FieldUnit( eOutUnit, nDecDigits );

    if ( nDecDigits < 0 )
    {
        while ( nDecDigits )
        {
            nValue *= 10;
            nDecDigits++;
        }
    }
    else
    {
        while ( nDecDigits )
        {
            nValue += 5;
            nValue /= 10;
            nDecDigits--;
        }
    }

    if ( eFieldUnit != eInUnit )
    {
        sal_Int64 nDiv  = aImplFactor[eInUnit][eFieldUnit];
        sal_Int64 nMult = aImplFactor[eFieldUnit][eInUnit];

        DBG_ASSERT( nMult > 0, "illegal *" );
        DBG_ASSERT( nDiv  > 0, "illegal /" );

        if( nMult != 1 && nMult > 0 )
            nValue *= nMult;
        if( nDiv != 1 && nDiv > 0 )
        {
            nValue += (nValue < 0) ? (-nDiv/2) : (nDiv/2);
            nValue /= nDiv;
        }
    }
    return nValue;
}

// -----------------------------------------------------------------------

static BOOL ImplMetricGetValue( const XubString& rStr, double& rValue, sal_Int64 nBaseValue,
                                USHORT nDecDigits, const LocaleDataWrapper& rLocaleDataWrapper, FieldUnit eUnit )
{
    // Zahlenwert holen
    if ( !ImplNumericGetValue( rStr, rValue, nDecDigits, rLocaleDataWrapper ) )
        return FALSE;

    // Einheit rausfinden
    FieldUnit eEntryUnit = ImplMetricGetUnit( rStr );

    // Einheiten umrechnen
    rValue = MetricField::ConvertDoubleValue( rValue, nBaseValue, nDecDigits, eEntryUnit, eUnit );

    return TRUE;
}

// -----------------------------------------------------------------------

BOOL MetricFormatter::ImplMetricReformat( const XubString& rStr, double& rValue, XubString& rOutStr )
{
    if ( !ImplMetricGetValue( rStr, rValue, mnBaseValue, GetDecimalDigits(), ImplGetLocaleDataWrapper(), meUnit ) )
        return TRUE;
    else
    {
        double nTempVal = rValue;
        // caution: precision loss in double cast
        if ( nTempVal > GetMax() )
            nTempVal = (double)GetMax();
        else if ( nTempVal < GetMin())
            nTempVal = (double)GetMin();

        if ( GetErrorHdl().IsSet() && (rValue != nTempVal) )
        {
            mnCorrectedValue = (sal_Int64)nTempVal;
            if ( !GetErrorHdl().Call( this ) )
            {
                mnCorrectedValue = 0;
                return FALSE;
            }
            else
                mnCorrectedValue = 0;
        }

        rOutStr = CreateFieldText( (sal_Int64)nTempVal );
        return TRUE;
    }
}

// -----------------------------------------------------------------------

inline void MetricFormatter::ImplInit()
{
    mnBaseValue = 0;
    meUnit = MetricField::GetDefaultUnit();
    mnType = FORMAT_METRIC;
}

// -----------------------------------------------------------------------

MetricFormatter::MetricFormatter()
{
    ImplInit();
}

// -----------------------------------------------------------------------

void MetricFormatter::ImplLoadRes( const ResId& rResId )
{
    NumericFormatter::ImplLoadRes( rResId );

    ResMgr*     pMgr = rResId.GetResMgr();
    if( pMgr )
    {
        ULONG       nMask = pMgr->ReadLong();
    
        if ( METRICFORMATTER_UNIT & nMask )
            meUnit = (FieldUnit)pMgr->ReadLong();
    
        if ( METRICFORMATTER_CUSTOMUNITTEXT & nMask )
            maCustomUnitText = pMgr->ReadString();
    }
}

// -----------------------------------------------------------------------

MetricFormatter::~MetricFormatter()
{
}

// -----------------------------------------------------------------------

void MetricFormatter::SetUnit( FieldUnit eNewUnit )
{
    if ( eNewUnit == FUNIT_100TH_MM )
    {
        SetDecimalDigits( GetDecimalDigits() + 2 );
        meUnit = FUNIT_MM;
    }
    else
        meUnit = eNewUnit;
    ReformatAll();
}

// -----------------------------------------------------------------------

void MetricFormatter::SetCustomUnitText( const XubString& rStr )
{
    maCustomUnitText = rStr;
    ReformatAll();
}

// -----------------------------------------------------------------------

void MetricFormatter::SetValue( sal_Int64 nNewValue, FieldUnit eInUnit )
{
    SetUserValue( nNewValue, eInUnit );
    mnFieldValue = mnLastValue;
}

// -----------------------------------------------------------------------

XubString MetricFormatter::CreateFieldText( sal_Int64 nValue ) const
{
    XubString aStr = NumericFormatter::CreateFieldText( nValue );

    if( meUnit == FUNIT_CUSTOM )
        aStr += maCustomUnitText;
    else
        aStr += ImplMetricToString( meUnit );

    return aStr;
}

// -----------------------------------------------------------------------

void MetricFormatter::SetUserValue( sal_Int64 nNewValue, FieldUnit eInUnit )
{
    // Umrechnen auf eingestellte Einheiten
    nNewValue = MetricField::ConvertValue( nNewValue, mnBaseValue, GetDecimalDigits(), eInUnit, meUnit );
    NumericFormatter::SetUserValue( nNewValue );
}

// -----------------------------------------------------------------------

sal_Int64 MetricFormatter::GetValue( FieldUnit eOutUnit ) const
{
    if ( !GetField() )
        return 0;

    double nTempValue;
    // caution: precision loss in double cast
    if ( !ImplMetricGetValue( GetField()->GetText(), nTempValue, mnBaseValue, GetDecimalDigits(), ImplGetLocaleDataWrapper(), meUnit ) )
        nTempValue = (double)mnLastValue;

    // caution: precision loss in double cast
    if ( nTempValue > mnMax )
        nTempValue = (double)mnMax;
    else if ( nTempValue < mnMin )
        nTempValue = (double)mnMin;

    // Umrechnen auf gewuenschte Einheiten
    return MetricField::ConvertValue( (sal_Int64)nTempValue, mnBaseValue, GetDecimalDigits(), meUnit, eOutUnit );
}

// -----------------------------------------------------------------------

void MetricFormatter::SetValue( sal_Int64 nValue )
{
    // Implementation not inline, because it is a virtual Function
    SetValue( nValue, FUNIT_NONE );
}

// -----------------------------------------------------------------------

sal_Int64 MetricFormatter::GetValue() const
{
    // Implementation not inline, because it is a virtual Function
    return GetValue( FUNIT_NONE );
}

// -----------------------------------------------------------------------

void MetricFormatter::SetMin( sal_Int64 nNewMin, FieldUnit eInUnit )
{
    // Umrechnen auf gewuenschte Einheiten
    NumericFormatter::SetMin( MetricField::ConvertValue( nNewMin, mnBaseValue, GetDecimalDigits(),
                                                         eInUnit, meUnit ) );
}

// -----------------------------------------------------------------------

sal_Int64 MetricFormatter::GetMin( FieldUnit eOutUnit ) const
{
    // Umrechnen auf gewuenschte Einheiten
    return MetricField::ConvertValue( NumericFormatter::GetMin(), mnBaseValue,
                                      GetDecimalDigits(), meUnit, eOutUnit );
}

// -----------------------------------------------------------------------

void MetricFormatter::SetMax( sal_Int64 nNewMax, FieldUnit eInUnit )
{
    // Umrechnen auf gewuenschte Einheiten
    NumericFormatter::SetMax( MetricField::ConvertValue( nNewMax, mnBaseValue, GetDecimalDigits(),
                                                         eInUnit, meUnit ) );
}

// -----------------------------------------------------------------------

sal_Int64 MetricFormatter::GetMax( FieldUnit eOutUnit ) const
{
    // Umrechnen auf gewuenschte Einheiten
    return MetricField::ConvertValue( NumericFormatter::GetMax(), mnBaseValue,
                                      GetDecimalDigits(), meUnit, eOutUnit );
}

// -----------------------------------------------------------------------

void MetricFormatter::SetBaseValue( sal_Int64 nNewBase, FieldUnit eInUnit )
{
    mnBaseValue = MetricField::ConvertValue( nNewBase, mnBaseValue, GetDecimalDigits(),
                                             eInUnit, meUnit );
}

// -----------------------------------------------------------------------

sal_Int64 MetricFormatter::GetBaseValue( FieldUnit eOutUnit ) const
{
    // Umrechnen auf gewuenschte Einheiten
    return MetricField::ConvertValue( mnBaseValue, mnBaseValue, GetDecimalDigits(),
                                      meUnit, eOutUnit );
}

// -----------------------------------------------------------------------

void MetricFormatter::Reformat()
{
    if ( !GetField() )
        return;

    XubString aText = GetField()->GetText();
    if ( meUnit == FUNIT_CUSTOM )
        maCurUnitText = ImplMetricGetUnitText( aText );

    XubString aStr;
    // caution: precision loss in double cast
    double nTemp = (double)mnLastValue;
    BOOL bOK = ImplMetricReformat( aText, nTemp, aStr );
    mnLastValue = (sal_Int64)nTemp;

    if ( !bOK )
        return;

    if ( aStr.Len() )
    {
        ImplSetText( aStr );
        if ( meUnit == FUNIT_CUSTOM )
            CustomConvert();
    }
    else
        SetValue( mnLastValue );
    maCurUnitText.Erase();
}

// -----------------------------------------------------------------------

sal_Int64 MetricFormatter::GetCorrectedValue( FieldUnit eOutUnit ) const
{
    // Umrechnen auf gewuenschte Einheiten
    return MetricField::ConvertValue( mnCorrectedValue, mnBaseValue, GetDecimalDigits(),
                                      meUnit, eOutUnit );
}

// -----------------------------------------------------------------------

MetricField::MetricField( Window* pParent, WinBits nWinStyle ) :
    SpinField( pParent, nWinStyle )
{
    SetField( this );
    Reformat();
}

// -----------------------------------------------------------------------

MetricField::MetricField( Window* pParent, const ResId& rResId ) :
    SpinField( WINDOW_METRICFIELD )
{
    rResId.SetRT( RSC_METRICFIELD );
    WinBits nStyle = ImplInitRes( rResId ) ;
    SpinField::ImplInit( pParent, nStyle );
    SetField( this );
    ImplLoadRes( rResId );

    if ( !(nStyle & WB_HIDE ) )
        Show();
}

// -----------------------------------------------------------------------

void MetricField::ImplLoadRes( const ResId& rResId )
{
    SpinField::ImplLoadRes( rResId );
    MetricFormatter::ImplLoadRes( ResId( (RSHEADER_TYPE *)GetClassRes(), *rResId.GetResMgr() ) );

    ULONG      nMask = ReadLongRes();

    if ( METRICFIELD_FIRST & nMask )
        mnFirst = ReadLongRes();

    if ( METRICFIELD_LAST & nMask )
        mnLast = ReadLongRes();

    if ( METRICFIELD_SPINSIZE & nMask )
        mnSpinSize = ReadLongRes();

    Reformat();
}

// -----------------------------------------------------------------------

MetricField::~MetricField()
{
}

// -----------------------------------------------------------------------

void MetricField::SetFirst( sal_Int64 nNewFirst, FieldUnit eInUnit )
{
    // convert
    nNewFirst = MetricField::ConvertValue( nNewFirst, mnBaseValue, GetDecimalDigits(),
                                           eInUnit, meUnit );
    mnFirst = nNewFirst;
}

// -----------------------------------------------------------------------

sal_Int64 MetricField::GetFirst( FieldUnit eOutUnit ) const
{
    // convert
    return MetricField::ConvertValue( mnFirst, mnBaseValue, GetDecimalDigits(),
                                      meUnit, eOutUnit );
}

// -----------------------------------------------------------------------

void MetricField::SetLast( sal_Int64 nNewLast, FieldUnit eInUnit )
{
    // Umrechnen
    nNewLast = MetricField::ConvertValue( nNewLast, mnBaseValue, GetDecimalDigits(),
                                          eInUnit, meUnit );
    mnLast = nNewLast;
}

// -----------------------------------------------------------------------

sal_Int64 MetricField::GetLast( FieldUnit eOutUnit ) const
{
    // Umrechnen
    return MetricField::ConvertValue( mnLast, mnBaseValue, GetDecimalDigits(),
                                      meUnit, eOutUnit );
}

// -----------------------------------------------------------------------

long MetricField::PreNotify( NotifyEvent& rNEvt )
{
    if ( (rNEvt.GetType() == EVENT_KEYINPUT) && !rNEvt.GetKeyEvent()->GetKeyCode().IsMod2() )
    {
        if ( ImplMetricProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), IsStrictFormat(), IsUseThousandSep(), ImplGetLocaleDataWrapper() ) )
            return 1;
    }

    return SpinField::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

long MetricField::Notify( NotifyEvent& rNEvt )
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

void MetricField::DataChanged( const DataChangedEvent& rDCEvt )
{
    SpinField::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) && (rDCEvt.GetFlags() & SETTINGS_LOCALE) )
    {
        String sOldDecSep = ImplGetLocaleDataWrapper().getNumDecimalSep();
        String sOldThSep = ImplGetLocaleDataWrapper().getNumThousandSep();
        if ( IsDefaultLocale() )
            ImplGetLocaleDataWrapper().setLocale( GetSettings().GetLocale() );
        String sNewDecSep = ImplGetLocaleDataWrapper().getNumDecimalSep();
        String sNewThSep = ImplGetLocaleDataWrapper().getNumThousandSep();
        ImplUpdateSeparators( sOldDecSep, sNewDecSep, sOldThSep, sNewThSep, this );
        ReformatAll();
    }
}

// -----------------------------------------------------------------------

void MetricField::Modify()
{
    MarkToBeReformatted( TRUE );
    SpinField::Modify();
}

// -----------------------------------------------------------------------

void MetricField::Up()
{
    FieldUp();
    SpinField::Up();
}

// -----------------------------------------------------------------------

void MetricField::Down()
{
    FieldDown();
    SpinField::Down();
}

// -----------------------------------------------------------------------

void MetricField::First()
{
    FieldFirst();
    SpinField::First();
}

// -----------------------------------------------------------------------

void MetricField::Last()
{
    FieldLast();
    SpinField::Last();
}

// -----------------------------------------------------------------------

void MetricField::CustomConvert()
{
    maCustomConvertLink.Call( this );
}

// -----------------------------------------------------------------------

MetricBox::MetricBox( Window* pParent, WinBits nWinStyle ) :
    ComboBox( pParent, nWinStyle )
{
    SetField( this );
    Reformat();
}

// -----------------------------------------------------------------------

MetricBox::MetricBox( Window* pParent, const ResId& rResId ) :
    ComboBox( WINDOW_METRICBOX )
{
    rResId.SetRT( RSC_METRICBOX );
    WinBits nStyle = ImplInitRes( rResId );
    ComboBox::ImplInit( pParent, nStyle );
    SetField( this );
    Reformat();
    ComboBox::ImplLoadRes( rResId );
    MetricFormatter::ImplLoadRes( ResId( (RSHEADER_TYPE *)GetClassRes(), *rResId.GetResMgr() ) );

    if ( !(nStyle & WB_HIDE ) )
        Show();
}

// -----------------------------------------------------------------------

MetricBox::~MetricBox()
{
}

// -----------------------------------------------------------------------

long MetricBox::PreNotify( NotifyEvent& rNEvt )
{
    if ( (rNEvt.GetType() == EVENT_KEYINPUT) && !rNEvt.GetKeyEvent()->GetKeyCode().IsMod2()  )
    {
        if ( ImplMetricProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), IsStrictFormat(), IsUseThousandSep(), ImplGetLocaleDataWrapper() ) )
            return 1;
    }

    return ComboBox::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

long MetricBox::Notify( NotifyEvent& rNEvt )
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

void MetricBox::DataChanged( const DataChangedEvent& rDCEvt )
{
    ComboBox::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) && (rDCEvt.GetFlags() & SETTINGS_LOCALE) )
    {
        String sOldDecSep = ImplGetLocaleDataWrapper().getNumDecimalSep();
        String sOldThSep = ImplGetLocaleDataWrapper().getNumThousandSep();
        if ( IsDefaultLocale() )
            ImplGetLocaleDataWrapper().setLocale( GetSettings().GetLocale() );
        String sNewDecSep = ImplGetLocaleDataWrapper().getNumDecimalSep();
        String sNewThSep = ImplGetLocaleDataWrapper().getNumThousandSep();
        ImplUpdateSeparators( sOldDecSep, sNewDecSep, sOldThSep, sNewThSep, this );
        ReformatAll();
    }
}

// -----------------------------------------------------------------------

void MetricBox::Modify()
{
    MarkToBeReformatted( TRUE );
    ComboBox::Modify();
}

// -----------------------------------------------------------------------

void MetricBox::ReformatAll()
{
    double nValue;
    XubString aStr;
    SetUpdateMode( FALSE );
    USHORT nEntryCount = GetEntryCount();
    for ( USHORT i=0; i < nEntryCount; i++ )
    {
        ImplMetricReformat( GetEntry( i ), nValue, aStr );
        RemoveEntry( i );
        InsertEntry( aStr, i );
    }
    MetricFormatter::Reformat();
    SetUpdateMode( TRUE );
}

// -----------------------------------------------------------------------

void MetricBox::CustomConvert()
{
    maCustomConvertLink.Call( this );
}

// -----------------------------------------------------------------------

void MetricBox::InsertValue( sal_Int64 nValue, FieldUnit eInUnit, USHORT nPos )
{
    // Umrechnen auf eingestellte Einheiten
    nValue = MetricField::ConvertValue( nValue, mnBaseValue, GetDecimalDigits(),
                                        eInUnit, meUnit );
    ComboBox::InsertEntry( CreateFieldText( nValue ), nPos );
}

// -----------------------------------------------------------------------

void MetricBox::RemoveValue( sal_Int64 nValue, FieldUnit eInUnit )
{
    // Umrechnen auf eingestellte Einheiten
    nValue = MetricField::ConvertValue( nValue, mnBaseValue, GetDecimalDigits(),
                                        eInUnit, meUnit );
    ComboBox::RemoveEntry( CreateFieldText( nValue ) );
}

// -----------------------------------------------------------------------

sal_Int64 MetricBox::GetValue( USHORT nPos, FieldUnit eOutUnit ) const
{
    double nValue = 0;
    ImplMetricGetValue( ComboBox::GetEntry( nPos ), nValue, mnBaseValue,
                        GetDecimalDigits(), ImplGetLocaleDataWrapper(), meUnit );

    // Umrechnen auf eingestellte Einheiten
    sal_Int64 nRetValue = MetricField::ConvertValue( (sal_Int64)nValue, mnBaseValue, GetDecimalDigits(),
                                                     meUnit, eOutUnit );

    return nRetValue;
}

// -----------------------------------------------------------------------

USHORT MetricBox::GetValuePos( sal_Int64 nValue, FieldUnit eInUnit ) const
{
    // Umrechnen auf eingestellte Einheiten
    nValue = MetricField::ConvertValue( nValue, mnBaseValue, GetDecimalDigits(),
                                        eInUnit, meUnit );
    return ComboBox::GetEntryPos( CreateFieldText( nValue ) );
}

// -----------------------------------------------------------------------

sal_Int64 MetricBox::GetValue( FieldUnit eOutUnit ) const
{
    // Implementation not inline, because it is a virtual Function
    return MetricFormatter::GetValue( eOutUnit );
}

// -----------------------------------------------------------------------

sal_Int64 MetricBox::GetValue() const
{
    // Implementation not inline, because it is a virtual Function
    return GetValue( FUNIT_NONE );
}

// -----------------------------------------------------------------------

static BOOL ImplCurrencyProcessKeyInput( Edit* pEdit, const KeyEvent& rKEvt,
                                         BOOL, BOOL bUseThousandSep, const LocaleDataWrapper& rWrapper )
{
    // Es gibt hier kein sinnvolles StrictFormat, also alle
    // Zeichen erlauben
    return ImplNumericProcessKeyInput( pEdit, rKEvt, FALSE, bUseThousandSep, rWrapper );
}

// -----------------------------------------------------------------------

inline BOOL ImplCurrencyGetValue( const XubString& rStr, double& rValue,
                                  USHORT nDecDigits, const LocaleDataWrapper& rWrapper )
{
    // Zahlenwert holen
    return ImplNumericGetValue( rStr, rValue, nDecDigits, rWrapper, TRUE );
}

// -----------------------------------------------------------------------

BOOL CurrencyFormatter::ImplCurrencyReformat( const XubString& rStr,
                                              XubString& rOutStr )
{
    double nValue;
    if ( !ImplNumericGetValue( rStr, nValue, GetDecimalDigits(), ImplGetLocaleDataWrapper(), TRUE ) )
        return TRUE;
    else
    {
        double nTempVal = nValue;
        // caution: precision loss in double cast
        if ( nTempVal > GetMax() )
            nTempVal = (double)GetMax();
        else if ( nTempVal < GetMin())
            nTempVal = (double)GetMin();

        if ( GetErrorHdl().IsSet() && (nValue != nTempVal) )
        {
            mnCorrectedValue = (sal_Int64)nTempVal;
            if ( !GetErrorHdl().Call( this ) )
            {
                mnCorrectedValue = 0;
                return FALSE;
            }
            else
                mnCorrectedValue = 0;
        }

        rOutStr = CreateFieldText( (long)nTempVal );
        return TRUE;
    }
}

// -----------------------------------------------------------------------

inline void CurrencyFormatter::ImplInit()
{
    mnType = FORMAT_CURRENCY;
}

// -----------------------------------------------------------------------

CurrencyFormatter::CurrencyFormatter()
{
    ImplInit();
}

// -----------------------------------------------------------------------

CurrencyFormatter::~CurrencyFormatter()
{
}

// -----------------------------------------------------------------------

void CurrencyFormatter::SetCurrencySymbol( const String& rStr )
{
    maCurrencySymbol= rStr;
    ReformatAll();
}

// -----------------------------------------------------------------------

String CurrencyFormatter::GetCurrencySymbol() const
{
    return maCurrencySymbol.Len() ? maCurrencySymbol : ImplGetLocaleDataWrapper().getCurrSymbol();
}

// -----------------------------------------------------------------------

void CurrencyFormatter::SetValue( sal_Int64 nNewValue )
{
    SetUserValue( nNewValue );
    mnFieldValue = mnLastValue;
    SetEmptyFieldValueData( FALSE );
}

// -----------------------------------------------------------------------

XubString CurrencyFormatter::CreateFieldText( sal_Int64 nValue ) const
{
    return ImplGetLocaleDataWrapper().getCurr( nValue, GetDecimalDigits(), GetCurrencySymbol(), IsUseThousandSep() );
}

// -----------------------------------------------------------------------

sal_Int64 CurrencyFormatter::GetValue() const
{
    if ( !GetField() )
        return 0;

    double nTempValue;
    if ( ImplCurrencyGetValue( GetField()->GetText(), nTempValue, GetDecimalDigits(), ImplGetLocaleDataWrapper() ) )
    {
        // caution: precision loss in double cast
        if ( nTempValue > mnMax )
            nTempValue = (double)mnMax;
        else if ( nTempValue < mnMin )
            nTempValue = (double)mnMin;
        return (sal_Int64)nTempValue;
    }
    else
        return mnLastValue;
}

// -----------------------------------------------------------------------

void CurrencyFormatter::Reformat()
{
    if ( !GetField() )
        return;

    XubString aStr;
    BOOL bOK = ImplCurrencyReformat( GetField()->GetText(), aStr );
    if ( !bOK )
        return;

    if ( aStr.Len() )
    {
        ImplSetText( aStr  );
        // caution: precision loss in double cast
        double nTemp = (double)mnLastValue;
        ImplCurrencyGetValue( aStr, nTemp, GetDecimalDigits(), ImplGetLocaleDataWrapper() );
        mnLastValue = (sal_Int64)nTemp;
    }
    else
        SetValue( mnLastValue );
}

// -----------------------------------------------------------------------

CurrencyField::CurrencyField( Window* pParent, WinBits nWinStyle ) :
    SpinField( pParent, nWinStyle )
{
    SetField( this );
    Reformat();
}

// -----------------------------------------------------------------------

CurrencyField::CurrencyField( Window* pParent, const ResId& rResId ) :
    SpinField( WINDOW_CURRENCYFIELD )
{
    rResId.SetRT( RSC_CURRENCYFIELD );
    WinBits nStyle = ImplInitRes( rResId );
    SpinField::ImplInit( pParent, nStyle);
    SetField( this );
    ImplLoadRes( rResId );

    if ( !(nStyle & WB_HIDE ) )
        Show();
}

// -----------------------------------------------------------------------

void CurrencyField::ImplLoadRes( const ResId& rResId )
{
    SpinField::ImplLoadRes( rResId );
    CurrencyFormatter::ImplLoadRes( ResId( (RSHEADER_TYPE *)GetClassRes(), *rResId.GetResMgr() ) );

    ULONG      nMask = ReadLongRes();

    if ( CURRENCYFIELD_FIRST & nMask )
        mnFirst = ReadLongRes();

    if ( CURRENCYFIELD_LAST & nMask )
        mnLast = ReadLongRes();

    if ( CURRENCYFIELD_SPINSIZE & nMask )
        mnSpinSize = ReadLongRes();

    Reformat();
}

// -----------------------------------------------------------------------

CurrencyField::~CurrencyField()
{
}

// -----------------------------------------------------------------------

long CurrencyField::PreNotify( NotifyEvent& rNEvt )
{
    if ( (rNEvt.GetType() == EVENT_KEYINPUT) && !rNEvt.GetKeyEvent()->GetKeyCode().IsMod2() )
    {
        if ( ImplCurrencyProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), IsStrictFormat(), IsUseThousandSep(), ImplGetLocaleDataWrapper() ) )
            return 1;
    }

    return SpinField::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

long CurrencyField::Notify( NotifyEvent& rNEvt )
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

void CurrencyField::DataChanged( const DataChangedEvent& rDCEvt )
{
    SpinField::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) && (rDCEvt.GetFlags() & SETTINGS_LOCALE) )
    {
        String sOldDecSep = ImplGetLocaleDataWrapper().getNumDecimalSep();
        String sOldThSep = ImplGetLocaleDataWrapper().getNumThousandSep();
        if ( IsDefaultLocale() )
            ImplGetLocaleDataWrapper().setLocale( GetSettings().GetLocale() );
        String sNewDecSep = ImplGetLocaleDataWrapper().getNumDecimalSep();
        String sNewThSep = ImplGetLocaleDataWrapper().getNumThousandSep();
        ImplUpdateSeparators( sOldDecSep, sNewDecSep, sOldThSep, sNewThSep, this );
        ReformatAll();
    }
}

// -----------------------------------------------------------------------

void CurrencyField::Modify()
{
    MarkToBeReformatted( TRUE );
    SpinField::Modify();
}

// -----------------------------------------------------------------------

void CurrencyField::Up()
{
    FieldUp();
    SpinField::Up();
}

// -----------------------------------------------------------------------

void CurrencyField::Down()
{
    FieldDown();
    SpinField::Down();
}

// -----------------------------------------------------------------------

void CurrencyField::First()
{
    FieldFirst();
    SpinField::First();
}

// -----------------------------------------------------------------------

void CurrencyField::Last()
{
    FieldLast();
    SpinField::Last();
}

// -----------------------------------------------------------------------

CurrencyBox::CurrencyBox( Window* pParent, WinBits nWinStyle ) :
    ComboBox( pParent, nWinStyle )
{
    SetField( this );
    Reformat();
}

// -----------------------------------------------------------------------

CurrencyBox::CurrencyBox( Window* pParent, const ResId& rResId ) :
    ComboBox( WINDOW_CURRENCYBOX )
{
    rResId.SetRT( RSC_CURRENCYBOX );
    WinBits nStyle = ImplInitRes( rResId );
    ComboBox::ImplInit( pParent, nStyle );
    CurrencyFormatter::ImplLoadRes( ResId( (RSHEADER_TYPE *)GetClassRes(), *rResId.GetResMgr() ) );
    SetField( this );
    ComboBox::ImplLoadRes( rResId );
    Reformat();

    if ( !(nStyle & WB_HIDE ) )
        Show();
}

// -----------------------------------------------------------------------

CurrencyBox::~CurrencyBox()
{
}

// -----------------------------------------------------------------------

long CurrencyBox::PreNotify( NotifyEvent& rNEvt )
{
    if ( (rNEvt.GetType() == EVENT_KEYINPUT) && !rNEvt.GetKeyEvent()->GetKeyCode().IsMod2() )
    {
        if ( ImplCurrencyProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), IsStrictFormat(), IsUseThousandSep(), ImplGetLocaleDataWrapper() ) )
            return 1;
    }

    return ComboBox::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

long CurrencyBox::Notify( NotifyEvent& rNEvt )
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

void CurrencyBox::DataChanged( const DataChangedEvent& rDCEvt )
{
    ComboBox::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) && (rDCEvt.GetFlags() & SETTINGS_LOCALE) )
    {
        String sOldDecSep = ImplGetLocaleDataWrapper().getNumDecimalSep();
        String sOldThSep = ImplGetLocaleDataWrapper().getNumThousandSep();
        if ( IsDefaultLocale() )
            ImplGetLocaleDataWrapper().setLocale( GetSettings().GetLocale() );
        String sNewDecSep = ImplGetLocaleDataWrapper().getNumDecimalSep();
        String sNewThSep = ImplGetLocaleDataWrapper().getNumThousandSep();
        ImplUpdateSeparators( sOldDecSep, sNewDecSep, sOldThSep, sNewThSep, this );
        ReformatAll();
    }
}

// -----------------------------------------------------------------------

void CurrencyBox::Modify()
{
    MarkToBeReformatted( TRUE );
    ComboBox::Modify();
}

// -----------------------------------------------------------------------

void CurrencyBox::ReformatAll()
{
    XubString aStr;
    SetUpdateMode( FALSE );
    USHORT nEntryCount = GetEntryCount();
    for ( USHORT i=0; i < nEntryCount; i++ )
    {
        ImplCurrencyReformat( GetEntry( i ), aStr );
        RemoveEntry( i );
        InsertEntry( aStr, i );
    }
    CurrencyFormatter::Reformat();
    SetUpdateMode( TRUE );
}

// -----------------------------------------------------------------------

void CurrencyBox::InsertValue( sal_Int64 nValue, USHORT nPos )
{
    ComboBox::InsertEntry( CreateFieldText( nValue ), nPos );
}

// -----------------------------------------------------------------------

void CurrencyBox::RemoveValue( sal_Int64 nValue )
{
    ComboBox::RemoveEntry( CreateFieldText( nValue ) );
}

// -----------------------------------------------------------------------

sal_Int64 CurrencyBox::GetValue( USHORT nPos ) const
{
    double nValue = 0;
    ImplCurrencyGetValue( ComboBox::GetEntry( nPos ), nValue, GetDecimalDigits(), ImplGetLocaleDataWrapper() );
    return (sal_Int64)nValue;
}

// -----------------------------------------------------------------------

USHORT CurrencyBox::GetValuePos( sal_Int64 nValue ) const
{
    return ComboBox::GetEntryPos( CreateFieldText( nValue ) );
}

// -----------------------------------------------------------------------

sal_Int64 CurrencyBox::GetValue() const
{
    // Implementation not inline, because it is a virtual Function
    return CurrencyFormatter::GetValue();
}
