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

#ifndef _SVX_OPTJSEARCH_HXX_
#define _SVX_OPTJSEARCH_HXX_

#include <vcl/fixed.hxx>
#ifndef _SV_BUTTON_HXX
#include <vcl/button.hxx>
#endif
#include <sfx2/tabdlg.hxx>

class Window;
class SfxItemSet;

//////////////////////////////////////////////////////////////////////

class SvxJSearchOptionsPage : public SfxTabPage
{
private:
	FixedLine	aTreatAsEqual;
	CheckBox	aMatchCase;
	CheckBox	aMatchFullHalfWidth;
	CheckBox	aMatchHiraganaKatakana;
	CheckBox	aMatchContractions;
	CheckBox	aMatchMinusDashChoon;
	CheckBox	aMatchRepeatCharMarks;
	CheckBox	aMatchVariantFormKanji;
	CheckBox	aMatchOldKanaForms;
	CheckBox	aMatchDiziDuzu;
	CheckBox	aMatchBavaHafa;
	CheckBox	aMatchTsithichiDhizi;
	CheckBox	aMatchHyuiyuByuvyu;
	CheckBox	aMatchSesheZeje;
	CheckBox	aMatchIaiya;
	CheckBox	aMatchKiku;
    CheckBox    aMatchProlongedSoundMark;
    FixedLine   aIgnore;
	CheckBox	aIgnorePunctuation;
	CheckBox	aIgnoreWhitespace;
    CheckBox    aIgnoreMiddleDot;

	INT32		nTransliterationFlags;
	BOOL		bSaveOptions;

	INT32				GetTransliterationFlags_Impl();

protected:
						SvxJSearchOptionsPage( Window* pParent, const SfxItemSet& rSet );

public:
						~SvxJSearchOptionsPage();

	static SfxTabPage*  Create( Window* pParent, const SfxItemSet& rSet );

	virtual void        Reset( const SfxItemSet& rSet );
	virtual BOOL        FillItemSet( SfxItemSet& rSet );

	BOOL				IsSaveOptions() const			{ return bSaveOptions; }
	void				EnableSaveOptions( BOOL bVal )	{ bSaveOptions = bVal; }

	INT32				GetTransliterationFlags() const	{ return nTransliterationFlags; }
	void				SetTransliterationFlags( INT32 nSettings );
};

//////////////////////////////////////////////////////////////////////

#endif

