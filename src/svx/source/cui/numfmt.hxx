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
#ifndef _SVX_NUMFMT_HXX
#define _SVX_NUMFMT_HXX

//------------------------------------------------------------------------

#include <vcl/window.hxx>
#include <tools/color.hxx>
#include <tools/string.hxx>
#include <sfx2/tabdlg.hxx>

#ifndef _FIXED_HXX //autogen
#include <vcl/fixed.hxx>
#endif

#ifndef _LSTBOX_HXX //autogen
#include <vcl/lstbox.hxx>
#endif

#ifndef _EDIT_HXX //autogen
#include <vcl/edit.hxx>
#endif

#ifndef _TOOLBOX_HXX //autogen
#include <vcl/toolbox.hxx>
#endif

#ifndef _FIELD_HXX //autogen
#include <vcl/field.hxx>
#endif
#include <svx/langbox.hxx>
#include "fontlb.hxx"

//CHINA001 #define SVX_NUMVAL_STANDARD		-1234.12345678901234
//CHINA001 #define SVX_NUMVAL_CURRENCY		-1234
//CHINA001 #define SVX_NUMVAL_PERCENT		-0.1295
//CHINA001 #define SVX_NUMVAL_TIME 		36525.5678935185
//CHINA001 #define SVX_NUMVAL_DATE 		36525.5678935185
//CHINA001 #define SVX_NUMVAL_BOOLEAN 		1

//------------------------------------------------------------------------

class SvxNumberFormatShell;
class SvxNumberInfoItem;

//------------------------------------------------------------------------

class SvxNumberPreviewImpl : public Window
{
private:
	String			aPrevStr;
	Color			aPrevCol;

	void			InitSettings( BOOL bForeground, BOOL bBackground );

protected:
	virtual void	Paint( const Rectangle& rRect );
	virtual void	StateChanged( StateChangedType nStateChange );
	virtual void	DataChanged( const DataChangedEvent& rDCEvt );

public:
	SvxNumberPreviewImpl( Window* pParent, const ResId& rResId );
	~SvxNumberPreviewImpl();

    void            NotifyChange( const String& rPrevStr, const Color* pColor = NULL );
};

// -----------------------------------------------------------------------

#include <sfx2/layout.hxx>
#include <layout/layout-pre.hxx>

class SvxNumberFormatTabPage : public SfxTabPage
{
	using SfxTabPage::DeactivatePage;

public:
	~SvxNumberFormatTabPage();

#undef SfxTabPage
#define SfxTabPage ::SfxTabPage
	static SfxTabPage*		Create( Window* pParent,
									const SfxItemSet& rAttrSet );
	static USHORT*			GetRanges();

	virtual	BOOL 			FillItemSet( SfxItemSet& rSet );
	virtual	void 			Reset( const SfxItemSet& rSet );
	virtual int 			DeactivatePage	( SfxItemSet* pSet = NULL );

	void					SetInfoItem( const SvxNumberInfoItem& rItem );
	void					SetNumberFormatList( const SvxNumberInfoItem& rItem )
								{ SetInfoItem( rItem ); }

	void					SetOkHdl( const Link& rOkHandler );
	void					HideLanguage(BOOL nFlag=TRUE);
	virtual long			PreNotify( NotifyEvent& rNEvt );
	virtual void			PageCreated (SfxAllItemSet aSet); //add CHINA001
private:
	SvxNumberFormatTabPage( Window*	pParent,
							const SfxItemSet& rCoreAttrs );
	FixedText				aFtCategory;
	ListBox					aLbCategory;
	FixedText				aFtFormat;
	ListBox					aLbCurrency;
	SvxFontListBox			aLbFormat;
	FixedText				aFtLanguage;
	SvxLanguageBox			aLbLanguage;
    CheckBox                aCbSourceFormat;

	FixedText				aFtDecimals;
	NumericField			aEdDecimals;
	FixedText				aFtLeadZeroes;
	NumericField			aEdLeadZeroes;
	CheckBox				aBtnNegRed;
	CheckBox				aBtnThousand;
    FixedLine               aFlOptions;

	FixedText				aFtEdFormat;
	Edit					aEdFormat;
	ImageButton				aIbAdd;
	ImageButton				aIbInfo;
	ImageButton				aIbRemove;

	FixedText				aFtComment;
	Edit					aEdComment;
	Timer					aResetWinTimer;

	SvxNumberPreviewImpl	aWndPreview;

	SvxNumberInfoItem*		pNumItem;
	SvxNumberFormatShell* 	pNumFmtShell;
	ULONG					nInitFormat;
	Link					fnOkHdl;

	BOOL					bNumItemFlag; //Fuer Handling mit DocShell
	BOOL					bOneAreaFlag;
	short					nFixedCategory;

	long					nCatHeight;

	long					nCurFormatY;
	long					nCurFormatHeight;
	long					nStdFormatY;
	long					nStdFormatHeight;
    LocalizedString sAutomaticEntry;

	Window*					pLastActivWindow;

#ifdef _SVX_NUMFMT_CXX
	void 	Init_Impl();
	void	FillCurrencyBox();
	void 	FillFormatListBox_Impl( SvxDelStrgs& rEntries );
	void 	UpdateOptions_Impl( BOOL bCheckCatChange );
	void	UpdateFormatListBox_Impl( USHORT bCat, BOOL bUpdateEdit );
	void	DeleteEntryList_Impl( SvxDelStrgs& rEntries );
	void	Obstructing();
    void    EnableBySourceFormat_Impl();
	void	SetCategory( USHORT nPos );
    String  GetExpColorString( Color*& rpPreviewColor, const String& aFormatStr, short nTmpCatPos );
    void    MakePreviewText( const String& rFormat );
    void    ChangePreviewText( USHORT nPos );
    void    AddAutomaticLanguage_Impl(LanguageType eAutoLang, BOOL bSelect);
	// Handler
	DECL_LINK( LostFocusHdl_Impl, Edit* pEd );
	DECL_LINK( DoubleClickHdl_Impl, SvxFontListBox* pLb );
	DECL_LINK( SelFormatHdl_Impl, void * );
	DECL_LINK( ClickHdl_Impl, ImageButton* pIB );
	DECL_LINK( EditHdl_Impl, Edit* pEdFormat );
	DECL_LINK( OptHdl_Impl, void * );
	DECL_LINK( TimeHdl_Impl, Timer * );

#endif
};

#include <layout/layout-post.hxx>

#endif

