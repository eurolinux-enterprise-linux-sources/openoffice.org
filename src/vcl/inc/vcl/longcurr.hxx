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

#ifndef _LONGCURR_HXX
#define _LONGCURR_HXX

#include <vcl/dllapi.h>
#include <tools/bigint.hxx>
#ifndef _FIELD_HXX
#include <vcl/field.hxx>
#endif

class LocaleDataWrapper;

// -------------------------
// - LongCurrencyFormatter -
// -------------------------

class VCL_DLLPUBLIC LongCurrencyFormatter : public FormatterBase
{
private:
	SAL_DLLPRIVATE friend BOOL ImplLongCurrencyReformat( const XubString&, BigInt, BigInt, USHORT, const LocaleDataWrapper&, XubString&, LongCurrencyFormatter& );
	SAL_DLLPRIVATE void        ImpInit();

protected:
	BigInt					mnFieldValue;
	BigInt					mnLastValue;
	BigInt					mnMin;
	BigInt					mnMax;
	BigInt					mnCorrectedValue;
    String                  maCurrencySymbol;
	USHORT					mnType;
	USHORT                  mnDecimalDigits;
    BOOL                    mbThousandSep;

							LongCurrencyFormatter();
	SAL_DLLPRIVATE void		ImplLoadRes( const ResId& rResId );

public:
							~LongCurrencyFormatter();

	virtual void			Reformat();
	virtual void			ReformatAll();

    void                    SetUseThousandSep( BOOL b );
    BOOL                    IsUseThousandSep() const { return mbThousandSep; }

    void                    SetCurrencySymbol( const String& rStr );
    String                  GetCurrencySymbol() const;

    void					SetMin( BigInt nNewMin );
	BigInt					GetMin() const { return mnMin; }
	void					SetMax( BigInt nNewMax );
	BigInt					GetMax() const { return mnMax; }

	void					SetDecimalDigits( USHORT nDigits );
	USHORT					GetDecimalDigits() const;
	void					SetValue( BigInt nNewValue );
	void					SetUserValue( BigInt nNewValue );
	BigInt					GetValue() const;
	BOOL					IsValueModified() const;

	void					SetEmptyValue();
	BOOL					IsEmptyValue() const { return !GetField()->GetText().Len(); }

	BigInt					GetCorrectedValue() const { return mnCorrectedValue; }

	BigInt					Normalize( BigInt nValue ) const;
	BigInt					Denormalize( BigInt nValue ) const;
};

// ---------------------
// - LongCurrencyField -
// ---------------------

class VCL_DLLPUBLIC LongCurrencyField : public SpinField, public LongCurrencyFormatter
{
//#if 0 // _SOLAR__PRIVATE
	friend void ImplNewLongCurrencyFieldValue( LongCurrencyField*, BigInt );
//#endif

private:
	BigInt			mnSpinSize;
	BigInt			mnFirst;
	BigInt			mnLast;

protected:
	SAL_DLLPRIVATE void ImplLoadRes( const ResId& rResId );

public:
					LongCurrencyField( Window* pParent, WinBits nWinStyle );
					LongCurrencyField( Window* pParent, const ResId& rResId );
					~LongCurrencyField();

	long			PreNotify( NotifyEvent& rNEvt );
	long			Notify( NotifyEvent& rNEvt );

	void			Modify();
	void			Up();
	void			Down();
	void			First();
	void			Last();

	void			SetFirst( BigInt nNewFirst ) { mnFirst = nNewFirst; }
	BigInt			GetFirst() const { return mnFirst; }
	void			SetLast( BigInt nNewLast ) { mnLast = nNewLast; }
	BigInt			GetLast() const { return mnLast; }
	void			SetSpinSize( BigInt nNewSize ) { mnSpinSize = nNewSize; }
	BigInt			GetSpinSize() const { return mnSpinSize; }
};

// -------------------
// - LongCurrencyBox -
// -------------------

class VCL_DLLPUBLIC LongCurrencyBox : public ComboBox, public LongCurrencyFormatter
{
public:
					LongCurrencyBox( Window* pParent, WinBits nWinStyle );
					LongCurrencyBox( Window* pParent, const ResId& rResId );
					~LongCurrencyBox();

	long			PreNotify( NotifyEvent& rNEvt );
	long			Notify( NotifyEvent& rNEvt );

	void			Modify();
	void			ReformatAll();

	void			InsertValue( BigInt nValue,
								 USHORT nPos = COMBOBOX_APPEND );
	void			RemoveValue( BigInt nValue );
	BigInt			GetValue() const
						{ return LongCurrencyFormatter::GetValue(); }
	BigInt			GetValue( USHORT nPos ) const;
	USHORT			GetValuePos( BigInt nValue ) const;
};

#endif // _LONGCURR_HXX
