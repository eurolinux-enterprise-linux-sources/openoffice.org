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

#include <sot/object.hxx>
#define _TOOLS_BIGINT
#include <sot/factory.hxx>
#include <tools/debug.hxx>
#include <tools/bigint.hxx>

#ifndef _SV_RC_H
#include <tools/rc.h>
#endif

#include <vcl/event.hxx>
#include <vcl/svapp.hxx>
#include <vcl/svdata.hxx>
#include <vcl/longcurr.hxx>


#include <unotools/localedatawrapper.hxx>


// =======================================================================

#define FORMAT_LONGCURRENCY 	 4

// =======================================================================

static BigInt ImplPower10( USHORT n )
{
	USHORT i;
	BigInt	 nValue = 1;

	for ( i=0; i < n; i++ )
		nValue *= 10;

	return nValue;
}

// -----------------------------------------------------------------------

static XubString ImplGetCurr( const LocaleDataWrapper& rLocaleDataWrapper, const BigInt &rNumber, USHORT nDigits, const String& rCurrSymbol, BOOL bShowThousandSep )
{
	DBG_ASSERT( nDigits < 10, "LongCurrency duerfen nur maximal 9 Nachkommastellen haben" );

	if ( rNumber.IsZero() || (long)rNumber )
		return rLocaleDataWrapper.getCurr( (long)rNumber, nDigits, rCurrSymbol, bShowThousandSep );

	BigInt aTmp( ImplPower10( nDigits ) );
	BigInt aInteger( rNumber );
	aInteger.Abs();
	aInteger  /= aTmp;
	BigInt aFraction( rNumber );
	aFraction.Abs();
	aFraction %= aTmp;
	if ( !aInteger.IsZero() )
	{
		aFraction += aTmp;
		aTmp	   = 1000000000L;
	}
	if ( rNumber.IsNeg() )
		aFraction *= -1;

	XubString aTemplate = rLocaleDataWrapper.getCurr( (long)aFraction, nDigits, rCurrSymbol, bShowThousandSep );
	while( !aInteger.IsZero() )
	{
		aFraction  = aInteger;
		aFraction %= aTmp;
		aInteger  /= aTmp;
		if( !aInteger.IsZero() )
			aFraction += aTmp;

		XubString aFractionStr = rLocaleDataWrapper.getNum( (long)aFraction, 0 );

		xub_StrLen nSPos = aTemplate.Search( '1' );
		if ( aFractionStr.Len() == 1 )
			aTemplate.SetChar( nSPos, aFractionStr.GetChar( 0 ) );
		else
		{
			aTemplate.Erase( nSPos, 1 );
			aTemplate.Insert( aFractionStr, nSPos );
		}
	}

	return aTemplate;
}

// -----------------------------------------------------------------------

static BOOL ImplNumericProcessKeyInput( Edit*, const KeyEvent& rKEvt,
										BOOL bStrictFormat, BOOL bThousandSep,
										const LocaleDataWrapper& rLocaleDataWrapper )
{
	if ( !bStrictFormat )
		return FALSE;
	else
	{
		sal_Unicode cChar = rKEvt.GetCharCode();
		USHORT		nGroup = rKEvt.GetKeyCode().GetGroup();

		if ( (nGroup == KEYGROUP_FKEYS) || (nGroup == KEYGROUP_CURSOR) ||
			 (nGroup == KEYGROUP_MISC) ||
			 ((cChar >= '0') && (cChar <= '9')) ||
			 (bThousandSep && (cChar == rLocaleDataWrapper.getNumThousandSep())) ||
			 (cChar == rLocaleDataWrapper.getNumDecimalSep() ) ||
			 (cChar == '-') )
			return FALSE;
		else
			return TRUE;
	}
}

// -----------------------------------------------------------------------

static BOOL ImplNumericGetValue( const XubString& rStr, BigInt& rValue,
								 USHORT nDecDigits, const LocaleDataWrapper& rLocaleDataWrapper,
								 BOOL bCurrency = FALSE )
{
	XubString	aStr = rStr;
	XubString	aStr1;
	XubString	aStr2;
	USHORT		nDecPos;
	BOOL		bNegative = FALSE;
	xub_StrLen	i;

	// Reaktion auf leeren String
	if ( !rStr.Len() )
		return FALSE;

	// Fuehrende und nachfolgende Leerzeichen entfernen
	aStr.EraseLeadingAndTrailingChars( ' ' );

	// Position des Dezimalpunktes suchen
	nDecPos = aStr.Search( rLocaleDataWrapper.getNumDecimalSep() );

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
			USHORT nFormat = rLocaleDataWrapper.getCurrNegativeFormat();
			if ( (nFormat == 3) || (nFormat == 6)  ||
				 (nFormat == 7) || (nFormat == 10) )
			{
				for ( i = (USHORT)(aStr.Len()-1); i > 0; i++ )
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
	BigInt nValue( aStr );
	if ( bRound )
	{
		if ( !bNegative )
			nValue+=1;
		else
			nValue-=1;
	}

	rValue = nValue;

	return TRUE;
}

// =======================================================================

static BOOL ImplLongCurrencyProcessKeyInput( Edit* pEdit, const KeyEvent& rKEvt,
											 BOOL, BOOL bUseThousandSep, const LocaleDataWrapper& rLocaleDataWrapper )
{
	// Es gibt hier kein sinnvolles StrictFormat, also alle
	// Zeichen erlauben
	return ImplNumericProcessKeyInput( pEdit, rKEvt, FALSE, bUseThousandSep, rLocaleDataWrapper  );
}

// -----------------------------------------------------------------------

inline BOOL ImplLongCurrencyGetValue( const XubString& rStr, BigInt& rValue,
									  USHORT nDecDigits, const LocaleDataWrapper& rLocaleDataWrapper )
{
	// Zahlenwert holen
	return ImplNumericGetValue( rStr, rValue, nDecDigits, rLocaleDataWrapper, TRUE );
}

// -----------------------------------------------------------------------

BOOL ImplLongCurrencyReformat( const XubString& rStr, BigInt nMin, BigInt nMax,
							   USHORT nDecDigits,
							   const LocaleDataWrapper& rLocaleDataWrapper, String& rOutStr,
							   LongCurrencyFormatter& rFormatter )
{
	BigInt nValue;
	if ( !ImplNumericGetValue( rStr, nValue, nDecDigits, rLocaleDataWrapper, TRUE ) )
		return TRUE;
	else
	{
		BigInt nTempVal = nValue;
		if ( nTempVal > nMax )
			nTempVal = nMax;
		else if ( nTempVal < nMin )
			nTempVal = nMin;

		if ( rFormatter.GetErrorHdl().IsSet() && (nValue != nTempVal) )
		{
            rFormatter.mnCorrectedValue = nTempVal;
			if ( !rFormatter.GetErrorHdl().Call( &rFormatter ) )
			{
                rFormatter.mnCorrectedValue = 0;
				return FALSE;
			}
			else
            {
                rFormatter.mnCorrectedValue = 0;
            }
		}

		rOutStr = ImplGetCurr( rLocaleDataWrapper, nTempVal, nDecDigits, rFormatter.GetCurrencySymbol(), rFormatter.IsUseThousandSep() ); 
		return TRUE;
	}
}


// =======================================================================

void LongCurrencyFormatter::ImpInit()
{
	mnFieldValue		= 0;
	mnLastValue 		= 0;
	mnMin				= 0;
	mnMax				= 0x7FFFFFFF;
	mnMax			   *= 0x7FFFFFFF;
	mnCorrectedValue	= 0;
    mnDecimalDigits     = 0;
	mnType				= FORMAT_LONGCURRENCY;
    mbThousandSep       = TRUE;
	SetDecimalDigits( 0 );
}

// -----------------------------------------------------------------------

LongCurrencyFormatter::LongCurrencyFormatter()
{
	ImpInit();
}

// -----------------------------------------------------------------------

void LongCurrencyFormatter::ImplLoadRes( const ResId& rResId )
{
	ImpInit();

	ResMgr* 	pMgr = rResId.GetResMgr();
    if( pMgr )
    {
        ULONG		nMask = pMgr->ReadLong();
    
        if ( NUMERICFORMATTER_MIN & nMask )
            mnMin = pMgr->ReadLong();
    
        if ( NUMERICFORMATTER_MAX & nMask )
            mnMax = pMgr->ReadLong();
    
        if ( NUMERICFORMATTER_STRICTFORMAT & nMask )
            SetStrictFormat(  (BOOL)pMgr->ReadShort() );
    
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
    }
}

// -----------------------------------------------------------------------

LongCurrencyFormatter::~LongCurrencyFormatter()
{
}

// -----------------------------------------------------------------------

void LongCurrencyFormatter::SetCurrencySymbol( const String& rStr )
{
    maCurrencySymbol= rStr;
    ReformatAll();
}

// -----------------------------------------------------------------------

String LongCurrencyFormatter::GetCurrencySymbol() const
{
    return maCurrencySymbol.Len() ? maCurrencySymbol : GetLocaleDataWrapper().getCurrSymbol();
}

// -----------------------------------------------------------------------

void LongCurrencyFormatter::SetValue( BigInt nNewValue )
{
	SetUserValue( nNewValue );
	mnFieldValue = mnLastValue;
	SetEmptyFieldValueData( FALSE );
}

// -----------------------------------------------------------------------

void LongCurrencyFormatter::SetUserValue( BigInt nNewValue )
{
	if ( nNewValue > mnMax )
		nNewValue = mnMax;
	else if ( nNewValue < mnMin )
		nNewValue = mnMin;
	mnLastValue = nNewValue;

	if ( !GetField() )
		return;

	XubString aStr = ImplGetCurr( GetLocaleDataWrapper(), nNewValue, GetDecimalDigits(), GetCurrencySymbol(), IsUseThousandSep() );
	if ( GetField()->HasFocus() )
	{
		Selection aSelection = GetField()->GetSelection();
		GetField()->SetText( aStr );
		GetField()->SetSelection( aSelection );
	}
	else
		GetField()->SetText( aStr );
	MarkToBeReformatted( FALSE );
}

// -----------------------------------------------------------------------

BigInt LongCurrencyFormatter::GetValue() const
{
	if ( !GetField() )
		return 0;

	BigInt nTempValue;
	if ( ImplLongCurrencyGetValue( GetField()->GetText(), nTempValue, GetDecimalDigits(), GetLocaleDataWrapper() ) )
	{
		if ( nTempValue > mnMax )
			nTempValue = mnMax;
		else if ( nTempValue < mnMin )
			nTempValue = mnMin;
		return nTempValue;
	}
	else
		return mnLastValue;
}

// -----------------------------------------------------------------------

void LongCurrencyFormatter::Reformat()
{
	if ( !GetField() )
		return;

	if ( !GetField()->GetText().Len() && ImplGetEmptyFieldValue() )
		return;

	XubString aStr;
	BOOL bOK = ImplLongCurrencyReformat( GetField()->GetText(), mnMin, mnMax,
										 GetDecimalDigits(), GetLocaleDataWrapper(), aStr, *this );
	if ( !bOK )
		return;

	if ( aStr.Len() )
	{
		GetField()->SetText( aStr );
		MarkToBeReformatted( FALSE );
		ImplLongCurrencyGetValue( aStr, mnLastValue, GetDecimalDigits(), GetLocaleDataWrapper() );
	}
	else
		SetValue( mnLastValue );
}

// -----------------------------------------------------------------------

void LongCurrencyFormatter::ReformatAll()
{
	Reformat();
}

// -----------------------------------------------------------------------

void LongCurrencyFormatter::SetMin( BigInt nNewMin )
{
	mnMin = nNewMin;
	ReformatAll();
}

// -----------------------------------------------------------------------

void LongCurrencyFormatter::SetMax( BigInt nNewMax )
{
	mnMax = nNewMax;
	ReformatAll();
}

// -----------------------------------------------------------------------

void LongCurrencyFormatter::SetUseThousandSep( BOOL b )
{
    mbThousandSep = b;
    ReformatAll();
}


// -----------------------------------------------------------------------

void LongCurrencyFormatter::SetDecimalDigits( USHORT nDigits )
{
//	DBG_ASSERT( nDigits < 10, "LongCurrency duerfen nur maximal 9 Nachkommastellen haben" );

	if ( nDigits > 9 )
		nDigits = 9;
	
	mnDecimalDigits = nDigits;
	ReformatAll();
}

// -----------------------------------------------------------------------

USHORT LongCurrencyFormatter::GetDecimalDigits() const
{
	return mnDecimalDigits;
}

// -----------------------------------------------------------------------

BOOL LongCurrencyFormatter::IsValueModified() const
{
	if ( ImplGetEmptyFieldValue() )
		return !IsEmptyValue();
	else if ( GetValue() != mnFieldValue )
		return TRUE;
	else
		return FALSE;
}

// -----------------------------------------------------------------------

void LongCurrencyFormatter::SetEmptyValue()
{
	GetField()->SetText( ImplGetSVEmptyStr() );
	SetEmptyFieldValueData( TRUE );
}

// -----------------------------------------------------------------------

BigInt LongCurrencyFormatter::Normalize( BigInt nValue ) const
{
	return (nValue * ImplPower10( GetDecimalDigits() ) );
}

// -----------------------------------------------------------------------

BigInt LongCurrencyFormatter::Denormalize( BigInt nValue ) const
{
	BigInt nFactor = ImplPower10( GetDecimalDigits() );
	BigInt nTmp    = nFactor;
	nTmp /= 2;
	nTmp += nValue;
	nTmp /= nFactor;
	return nTmp;
}

// =======================================================================

void ImplNewLongCurrencyFieldValue( LongCurrencyField* pField, BigInt nNewValue )
{
	Selection aSelect = pField->GetSelection();
	aSelect.Justify();
	XubString aText = pField->GetText();
	BOOL bLastSelected = ((xub_StrLen)aSelect.Max() == aText.Len()) ? TRUE : FALSE;

	BigInt nOldLastValue  = pField->mnLastValue;
	pField->SetUserValue( nNewValue );
	pField->mnLastValue  = nOldLastValue;

	if ( bLastSelected )
	{
		if ( !aSelect.Len() )
			aSelect.Min() = SELECTION_MAX;
		aSelect.Max() = SELECTION_MAX;
	}
	pField->SetSelection( aSelect );
	pField->SetModifyFlag();
	pField->Modify();
}

// =======================================================================

LongCurrencyField::LongCurrencyField( Window* pParent, WinBits nWinStyle ) :
	SpinField( pParent, nWinStyle )
{
	SetField( this );
	mnSpinSize	 = 1;
	mnFirst 	 = mnMin;
	mnLast		 = mnMax;

	Reformat();
}

// -----------------------------------------------------------------------

LongCurrencyField::LongCurrencyField( Window* pParent, const ResId& rResId ) :
	SpinField( WINDOW_NUMERICFIELD )
{
	rResId.SetRT( RSC_NUMERICFIELD );
	WinBits nStyle = ImplInitRes( rResId ) ;
	SpinField::ImplInit( pParent, nStyle );

	SetField( this );
	mnSpinSize	 = 1;
	mnFirst 	 = mnMin;
	mnLast		 = mnMax;

	Reformat();

	if ( !(nStyle & WB_HIDE) )
		Show();
}

// -----------------------------------------------------------------------

void LongCurrencyField::ImplLoadRes( const ResId& rResId )
{
	SpinField::ImplLoadRes( rResId );
	LongCurrencyFormatter::ImplLoadRes( ResId( (RSHEADER_TYPE *)GetClassRes(), *rResId.GetResMgr() ) );

	ULONG nMask = ReadLongRes();
	if ( CURRENCYFIELD_FIRST & nMask )
		mnFirst = ReadLongRes();

	if ( CURRENCYFIELD_LAST & nMask )
		mnLast = ReadLongRes();

	if ( CURRENCYFIELD_SPINSIZE & nMask )
		mnSpinSize = ReadLongRes();
}

// -----------------------------------------------------------------------

LongCurrencyField::~LongCurrencyField()
{
}

// -----------------------------------------------------------------------

long LongCurrencyField::PreNotify( NotifyEvent& rNEvt )
{
	if( rNEvt.GetType() == EVENT_KEYINPUT )
	{
		if ( ImplLongCurrencyProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), IsStrictFormat(), IsUseThousandSep(), GetLocaleDataWrapper() ) )
			return 1;
	}
	return SpinField::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

long LongCurrencyField::Notify( NotifyEvent& rNEvt )
{
	if( rNEvt.GetType() == EVENT_GETFOCUS )
	{
		MarkToBeReformatted( FALSE );
	}
	else if( rNEvt.GetType() == EVENT_LOSEFOCUS )
	{
		if ( MustBeReformatted() )
		{
			Reformat();
			SpinField::Modify();
		}
	}
	return SpinField::Notify( rNEvt );
}

// -----------------------------------------------------------------------

void LongCurrencyField::Modify()
{
	MarkToBeReformatted( TRUE );
	SpinField::Modify();
}

// -----------------------------------------------------------------------

void LongCurrencyField::Up()
{
	BigInt nValue = GetValue();
	nValue += mnSpinSize;
	if ( nValue > mnMax )
		nValue = mnMax;

	ImplNewLongCurrencyFieldValue( this, nValue );
	SpinField::Up();
}

// -----------------------------------------------------------------------

void LongCurrencyField::Down()
{
	BigInt nValue = GetValue();
	nValue -= mnSpinSize;
	if ( nValue < mnMin )
		nValue = mnMin;

	ImplNewLongCurrencyFieldValue( this, nValue );
	SpinField::Down();
}

// -----------------------------------------------------------------------

void LongCurrencyField::First()
{
	ImplNewLongCurrencyFieldValue( this, mnFirst );
	SpinField::First();
}

// -----------------------------------------------------------------------

void LongCurrencyField::Last()
{
	ImplNewLongCurrencyFieldValue( this, mnLast );
	SpinField::Last();
}

// =======================================================================

LongCurrencyBox::LongCurrencyBox( Window* pParent, WinBits nWinStyle ) :
	ComboBox( pParent, nWinStyle )
{
	SetField( this );
	Reformat();
}

// -----------------------------------------------------------------------

LongCurrencyBox::LongCurrencyBox( Window* pParent, const ResId& rResId ) :
	ComboBox( WINDOW_NUMERICFIELD )
{
	SetField( this );
	WinBits nStyle = ImplInitRes( rResId ) ;
	ComboBox::ImplLoadRes( rResId );
    ImplInit( pParent, nStyle );
	LongCurrencyFormatter::ImplLoadRes( rResId );
	Reformat();

	if ( !(nStyle & WB_HIDE) )
		Show();
}

// -----------------------------------------------------------------------

LongCurrencyBox::~LongCurrencyBox()
{
}

// -----------------------------------------------------------------------

long LongCurrencyBox::PreNotify( NotifyEvent& rNEvt )
{
	if( rNEvt.GetType() == EVENT_KEYINPUT )
	{
		if ( ImplLongCurrencyProcessKeyInput( GetField(), *rNEvt.GetKeyEvent(), IsStrictFormat(), IsUseThousandSep(), GetLocaleDataWrapper() ) )
			return 1;
	}
	return ComboBox::PreNotify( rNEvt );
}

// -----------------------------------------------------------------------

long LongCurrencyBox::Notify( NotifyEvent& rNEvt )
{
	if( rNEvt.GetType() == EVENT_GETFOCUS )
	{
		MarkToBeReformatted( FALSE );
	}
	else if( rNEvt.GetType() == EVENT_LOSEFOCUS )
	{
		if ( MustBeReformatted() )
		{
			Reformat();
			ComboBox::Modify();
		}
	}
	return ComboBox::Notify( rNEvt );
}

// -----------------------------------------------------------------------

void LongCurrencyBox::Modify()
{
	MarkToBeReformatted( TRUE );
	ComboBox::Modify();
}

// -----------------------------------------------------------------------

void LongCurrencyBox::ReformatAll()
{
	XubString aStr;
	SetUpdateMode( FALSE );
	USHORT nEntryCount = GetEntryCount();
	for ( USHORT i=0; i < nEntryCount; i++ )
	{
		ImplLongCurrencyReformat( GetEntry( i ), mnMin, mnMax,
								  GetDecimalDigits(), GetLocaleDataWrapper(),
								  aStr, *this );
		RemoveEntry( i );
		InsertEntry( aStr, i );
	}
	LongCurrencyFormatter::Reformat();
	SetUpdateMode( TRUE );
}

// -----------------------------------------------------------------------

void LongCurrencyBox::InsertValue( BigInt nValue, USHORT nPos )
{
	XubString aStr = ImplGetCurr( GetLocaleDataWrapper(), nValue, GetDecimalDigits(), GetCurrencySymbol(), IsUseThousandSep() );
	ComboBox::InsertEntry( aStr, nPos );
}

// -----------------------------------------------------------------------

void LongCurrencyBox::RemoveValue( BigInt nValue )
{
	XubString aStr = ImplGetCurr( GetLocaleDataWrapper(), nValue, GetDecimalDigits(), GetCurrencySymbol(), IsUseThousandSep() );
	ComboBox::RemoveEntry( aStr );
}

// -----------------------------------------------------------------------

BigInt LongCurrencyBox::GetValue( USHORT nPos ) const
{
	BigInt nValue = 0;
	ImplLongCurrencyGetValue( ComboBox::GetEntry( nPos ), nValue,
							  GetDecimalDigits(), GetLocaleDataWrapper() );
	return nValue;
}

// -----------------------------------------------------------------------

USHORT LongCurrencyBox::GetValuePos( BigInt nValue ) const
{
	XubString aStr = ImplGetCurr( GetLocaleDataWrapper(), nValue, GetDecimalDigits(), GetCurrencySymbol(), IsUseThousandSep() );
	return ComboBox::GetEntryPos( aStr );
}

// =======================================================================
