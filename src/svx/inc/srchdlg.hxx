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
#ifndef _SVX_SRCHDLG_HXX
#define _SVX_SRCHDLG_HXX

// include ---------------------------------------------------------------


#include <svtools/stdctrl.hxx>
#ifndef _COMBOBOX_HXX //autogen
#include <vcl/combobox.hxx>
#endif
#ifndef _EDIT_HXX //autogen
#include <vcl/edit.hxx>
#endif
#ifndef _LSTBOX_HXX //autogen
#include <vcl/lstbox.hxx>
#endif
#ifndef _BUTTON_HXX //autogen
#include <vcl/button.hxx>
#endif
#ifndef _GROUP_HXX //autogen
#include <vcl/group.hxx>
#endif
#ifndef _FIXED_HXX //autogen
#include <vcl/fixed.hxx>
#endif
#ifndef _DIALOG_HXX //autogen
#include <vcl/dialog.hxx>
#endif
#include <sfx2/childwin.hxx>
#include <sfx2/basedlgs.hxx>
#include <svtools/svarray.hxx>
#define _SVSTDARR_STRINGSDTOR
#include <svtools/svstdarr.hxx>
#include <svtools/svmedit.hxx>
#include <sfx2/srchdefs.hxx>
#include "svx/svxdllapi.h"


// forward ---------------------------------------------------------------

class SvxSearchItem;
class MoreButton;
class SfxStyleSheetBasePool;
class SvxJSearchOptionsPage;
class SvxSearchController;

struct SearchDlg_Impl;

#ifndef NO_SVX_SEARCH

// struct SearchAttrItem -------------------------------------------------

struct SearchAttrItem
{
	USHORT 			nSlot;
	SfxPoolItem* 	pItem;
};

// class SearchAttrItemList ----------------------------------------------

SV_DECL_VARARR_VISIBILITY(SrchAttrItemList, SearchAttrItem, 8, 8, SVX_DLLPUBLIC)

class SVX_DLLPUBLIC SearchAttrItemList : private SrchAttrItemList
{
public:
	SearchAttrItemList() {}
	SearchAttrItemList( const SearchAttrItemList& rList );
	~SearchAttrItemList();

	void            Put( const SfxItemSet& rSet );
	SfxItemSet&     Get( SfxItemSet& rSet );
	void            Clear();
	USHORT          Count() const { return SrchAttrItemList::Count(); }
	SearchAttrItem& operator[](USHORT nPos) const
						{ return SrchAttrItemList::operator[]( nPos ); }
	SearchAttrItem& GetObject( USHORT nPos ) const
						{ return SrchAttrItemList::GetObject( nPos ); }

	// der Pointer auf das Item wird nicht kopiert!!! (also nicht l"oschen)
	void Insert( const SearchAttrItem& rItem )
		{ SrchAttrItemList::Insert( rItem, SrchAttrItemList::Count() ); }
	// l"oscht die Pointer auf die Items
	void Remove( USHORT nPos, USHORT nLen = 1 );
};

#ifndef SV_NODIALOG

// class SvxSearchDialogWrapper ------------------------------------------

#include <sfx2/layout.hxx>
#include <sfx2/layout-pre.hxx>

class SvxSearchDialog;
class SVX_DLLPUBLIC SvxSearchDialogWrapper : public SfxChildWindow
{
    SvxSearchDialog *dialog;
public:
	SvxSearchDialogWrapper( Window*pParent, USHORT nId,
							SfxBindings* pBindings, SfxChildWinInfo* pInfo );

    ~SvxSearchDialogWrapper ();
    SvxSearchDialog *getDialog ();
	SFX_DECL_CHILDWINDOW(SvxSearchDialogWrapper);
};

// class SvxSearchDialog -------------------------------------------------

/*
	{k:\svx\prototyp\dialog\srchdlg.hxx}

	[Beschreibung]
	In diesem Modeless-Dialog werden die Attribute einer Suche eingestellt
	und damit eine Suche gestartet. Es sind mehrere Sucharten
	( Suchen, Alle suchen, Ersetzen, Alle ersetzen ) m"oglich.

	[Items]
	<SvxSearchItem><SID_ATTR_SEARCH>
*/

class SvxSearchDialog : public SfxModelessDialog
{
friend class SvxSearchController;
friend class SvxSearchDialogWrapper;
friend class SvxJSearchOptionsDialog;

public:
	SvxSearchDialog( Window* pParent, SfxBindings& rBind );
	SvxSearchDialog( Window* pParent, SfxChildWindow* pChildWin, SfxBindings& rBind );
	~SvxSearchDialog();

	virtual BOOL    Close();

	// Window
	virtual void	Activate();

	void            GetSearchItems( SfxItemSet& rSet );
	void            GetReplaceItems( SfxItemSet& rSet );

	const SearchAttrItemList*   GetSearchItemList() const
									{ return pSearchList; }
	const SearchAttrItemList*   GetReplaceItemList() const
									{ return pReplaceList; }

	inline BOOL 	HasSearchAttributes() const;
	inline BOOL 	HasReplaceAttributes() const;

	PushButton&     GetReplaceBtn() { return aReplaceBtn; }

	INT32			GetTransliterationFlags() const;

private:
	FixedText       aSearchText;
	ComboBox        aSearchLB;
	ListBox         aSearchTmplLB;
	FixedInfo       aSearchAttrText;

	FixedText       aReplaceText;
	ComboBox        aReplaceLB;
	ListBox         aReplaceTmplLB;
	FixedInfo       aReplaceAttrText;

    PushButton      aSearchBtn;
	PushButton      aSearchAllBtn;
    FixedLine       aSearchCmdLine;
    PushButton      aReplaceBtn;
	PushButton      aReplaceAllBtn;

    FixedLine       aSearchComponentFL;
    PushButton      aSearchComponent1PB;
    PushButton      aSearchComponent2PB;

    CheckBox        aMatchCaseCB;
    CheckBox        aWordBtn;

    FixedLine       aButtonsFL;
    MoreButton*     pMoreBtn;
    HelpButton      aHelpBtn;
    CancelButton    aCloseBtn;

    FixedLine       aOptionsFL;
    CheckBox        aSelectionBtn;
    CheckBox        aBackwardsBtn;
    CheckBox        aRegExpBtn;
    CheckBox        aSimilarityBox;
    PushButton      aSimilarityBtn;
    CheckBox        aLayoutBtn;
    CheckBox        aNotesBtn;
    CheckBox        aJapMatchFullHalfWidthCB;
    CheckBox        aJapOptionsCB;
    PushButton      aJapOptionsBtn;

    PushButton      aAttributeBtn;
	PushButton      aFormatBtn;
	PushButton      aNoFormatBtn;

    FixedLine       aCalcFL;
    FixedText       aCalcSearchInFT;
    ListBox         aCalcSearchInLB;
    FixedText       aCalcSearchDirFT;
	RadioButton     aRowsBtn;
	RadioButton     aColumnsBtn;
    CheckBox        aAllSheetsCB;

	SfxBindings&    rBindings;
	BOOL            bWriter;
	BOOL            bSearch;
	BOOL            bFormat;
	USHORT          nOptions;
	FASTBOOL		bSet;
	FASTBOOL		bReadOnly;
	FASTBOOL		bConstruct;
	ULONG			nModifyFlag;
	String          aStylesStr;
	String			aLayoutStr;
	LocalizedString aCalcStr;

	SvStringsDtor   aSearchStrings;
	SvStringsDtor   aReplaceStrings;

	SearchDlg_Impl*			pImpl;
	SearchAttrItemList*     pSearchList;
	SearchAttrItemList*     pReplaceList;
	SvxSearchItem*          pSearchItem;

	SvxSearchController*    pSearchController;
	SvxSearchController*    pOptionsController;
	SvxSearchController*    pFamilyController;
	SvxSearchController*    pSearchSetController;
	SvxSearchController*    pReplaceSetController;

	mutable INT32			nTransliterationFlags;

#ifdef _SVX_SRCHDLG_CXX
	DECL_LINK( ModifyHdl_Impl, ComboBox* pEdit );
    DECL_LINK( FlagHdl_Impl, Control* pCtrl );
	DECL_LINK( CommandHdl_Impl, Button* pBtn );
	DECL_LINK( TemplateHdl_Impl, Button* );
	DECL_LINK( FocusHdl_Impl, Control* );
	DECL_LINK( LoseFocusHdl_Impl, Control* );
	DECL_LINK( FormatHdl_Impl, Button* );
	DECL_LINK( NoFormatHdl_Impl, Button* );
	DECL_LINK( AttributeHdl_Impl, Button* );
	DECL_LINK( TimeoutHdl_Impl, Timer* );

	void			Construct_Impl();
	void            InitControls_Impl();
    void            CalculateDelta_Impl();
	void            Init_Impl( int bHasItemSet );
	void            InitAttrList_Impl( const SfxItemSet* pSSet,
									   const SfxItemSet* pRSet );
	void            Remember_Impl( const String &rStr,BOOL bSearch );
	void            PaintAttrText_Impl();
	String&         BuildAttrText_Impl( String& rStr, BOOL bSrchFlag ) const;

	void            TemplatesChanged_Impl( SfxStyleSheetBasePool& rPool );
	void            EnableControls_Impl( const USHORT nFlags );
	void            EnableControl_Impl( Control* pCtrl );
	void            SetItem_Impl( const SvxSearchItem* pItem );

	void			SetModifyFlag_Impl( const Control* pCtrl );
	void			SaveToModule_Impl();

	void			ApplyTransliterationFlags_Impl( INT32 nSettings );
#endif
};

#include <sfx2/layout-post.hxx>

inline BOOL SvxSearchDialog::HasSearchAttributes() const
{
	int bLen = aSearchAttrText.GetText().Len();
	return ( aSearchAttrText.IsEnabled() && bLen );
}

inline BOOL SvxSearchDialog::HasReplaceAttributes() const
{
	int bLen = aReplaceAttrText.GetText().Len();
	return ( aReplaceAttrText.IsEnabled() && bLen );
}


//////////////////////////////////////////////////////////////////////


#endif  // SV_NODIALOG
#endif  // NO_SVX_SEARCH


#endif

