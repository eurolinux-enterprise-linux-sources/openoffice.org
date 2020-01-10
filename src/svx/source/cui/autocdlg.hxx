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
#ifndef _OFA_AUTOCDLG_HXX
#define _OFA_AUTOCDLG_HXX

#include <svtools/langtab.hxx>
#include <sfx2/tabdlg.hxx>
#include <tools/table.hxx>
#include <svx/checklbx.hxx>
#include <vcl/fixed.hxx>
#include <vcl/field.hxx>
#include <vcl/metric.hxx>
#include <svx/langbox.hxx>

class SvxAutoCorrect;
class CharClass;
class CollatorWrapper;
class SmartTagMgr;

// class OfaAutoCorrDlg --------------------------------------------------

class OfaAutoCorrDlg : public SfxTabDialog
{
	FixedText		aLanguageFT;
	SvxLanguageBox	aLanguageLB;

	DECL_LINK(SelectLanguageHdl, ListBox*);
public:

	OfaAutoCorrDlg(Window* pParent, const SfxItemSet *pSet );

	void	EnableLanguage(BOOL bEnable)
			{	aLanguageFT.Enable(bEnable);
				aLanguageLB.Enable(bEnable);}

};

#ifdef _OFA_AUTOCDLG_CXX
#include <vcl/group.hxx>
#ifndef _SV_BUTTON_HXX
#include <vcl/button.hxx>
#endif
#include <vcl/lstbox.hxx>
#include <svtools/svtabbx.hxx>
#include <svx/simptabl.hxx>

class SvStringsISortDtor;

// class OfaACorrCheckListBox ------------------------------------------

class OfaACorrCheckListBox : public SvxSimpleTable
{
	using SvxSimpleTable::SetTabs;
	using SvTreeListBox::GetCheckButtonState;
	using SvTreeListBox::SetCheckButtonState;

	protected:
		virtual void	SetTabs();
		virtual void	HBarClick();
        virtual void    KeyInput( const KeyEvent& rKEvt );

	public:
		OfaACorrCheckListBox(Window* pParent, const ResId& rResId ) :
			SvxSimpleTable( pParent, rResId ){}

		inline void *GetUserData(ULONG nPos) { return GetEntry(nPos)->GetUserData(); }
		inline void SetUserData(ULONG nPos, void *pData ) { GetEntry(nPos)->SetUserData(pData); }
		inline ULONG GetSelectEntryPos() { return GetModel()->GetAbsPos(FirstSelected()); }

		BOOL			IsChecked(ULONG nPos, USHORT nCol = 0);
		void			CheckEntryPos(ULONG nPos, USHORT nCol, BOOL bChecked);
		SvButtonState	GetCheckButtonState( SvLBoxEntry*, USHORT nCol ) const;
		void			SetCheckButtonState( SvLBoxEntry*, USHORT nCol, SvButtonState );
};

// class OfaAutocorrOptionsPage ------------------------------------------


class OfaAutocorrOptionsPage : public SfxTabPage
{
	using TabPage::ActivatePage;

private:
	SvxCheckListBox	aCheckLB;

	String		sDoubleCaps;
	String		sStartCap;
	String		sBoldUnderline;
	String		sURL;
	String		sNoDblSpaces;
	String		sHalf;
	String		sDash;
	String		sFirst;

public:
						OfaAutocorrOptionsPage( Window* pParent, const SfxItemSet& rSet );
						~OfaAutocorrOptionsPage();

	static SfxTabPage*	Create( Window* pParent,
								const SfxItemSet& rAttrSet);

	virtual	BOOL 		FillItemSet( SfxItemSet& rSet );
	virtual	void 		Reset( const SfxItemSet& rSet );
	virtual void		ActivatePage( const SfxItemSet& );

};

// class OfaSwAutoFmtOptionsPage ----------------------------------------------------

class OfaSwAutoFmtOptionsPage : public SfxTabPage
{
	using TabPage::ActivatePage;

	OfaACorrCheckListBox	aCheckLB;
	PushButton		aEditPB;
	FixedText		aHeader1Expl;
	FixedText		aHeader2Expl;

	String			sHeader1;
	String			sHeader2;

	String			sDeleteEmptyPara;
	String			sCptlSttWord;
	String			sCptlSttSent;
	String			sTypo;
	String			sUserStyle;
	String			sBullet;
	String			sByInputBullet;
	String			sBoldUnder;
	String			sNoDblSpaces;
	String			sFraction;
	String			sDetectURL;
	String          sDash;
	String			sOrdinal;
	String			sRightMargin;
	String			sNum;
	String			sBorder;
	String			sTable;
	String			sReplaceTemplates;
	String			sDelSpaceAtSttEnd;
	String			sDelSpaceBetweenLines;

	String			sMargin;
	String			sBulletChar;
	String			sByInputBulletChar;

	Font			aBulletFont;
	Font 			aByInputBulletFont;
	USHORT			nPercent;

	SvLBoxButtonData*	pCheckButtonData;

		DECL_LINK(SelectHdl, OfaACorrCheckListBox*);
		DECL_LINK(EditHdl, PushButton*);
		SvLBoxEntry* CreateEntry(String& rTxt, USHORT nCol);


		OfaSwAutoFmtOptionsPage( Window* pParent,
							const SfxItemSet& rSet );
		~OfaSwAutoFmtOptionsPage();

	public:
		static SfxTabPage*  Create( Window* pParent,
								const SfxItemSet& rAttrSet);
		virtual BOOL        FillItemSet( SfxItemSet& rSet );
		virtual void        Reset( const SfxItemSet& rSet );
		virtual void		ActivatePage( const SfxItemSet& );
};

// class AutoCorrEdit ----------------------------------------------------

class AutoCorrEdit : public Edit
{
	Link 	aActionLink;
	BOOL 	bSpaces;

	public:
					AutoCorrEdit(Window* pParent, const ResId& rResId) :
						Edit(pParent, rResId), bSpaces(FALSE){}

	void 			SetActionHdl( const Link& rLink )
								{ aActionLink = rLink;}

	void 			SetSpaces(BOOL bSet)
								{bSpaces = bSet;}

	virtual void	KeyInput( const KeyEvent& rKEvent );
};

// class OfaAutocorrExceptPage ---------------------------------------------

struct StringsArrays;
typedef StringsArrays* StringsArraysPtr;
DECLARE_TABLE(StringsTable, StringsArraysPtr)

class OfaAutocorrExceptPage : public SfxTabPage
{
	using TabPage::ActivatePage;
	using TabPage::DeactivatePage;

private:
		FixedLine		aAbbrevFL;
		AutoCorrEdit	aAbbrevED;
		ListBox			aAbbrevLB;
		PushButton		aNewAbbrevPB;
		PushButton		aDelAbbrevPB;
		CheckBox        aAutoAbbrevCB;

		FixedLine		aDoubleCapsFL;
		AutoCorrEdit	aDoubleCapsED;
		ListBox			aDoubleCapsLB;
		PushButton		aNewDoublePB;
		PushButton		aDelDoublePB;
		CheckBox        aAutoCapsCB;

		StringsTable	aStringsTable;
		CollatorWrapper* pCompareClass;
		LanguageType 	eLang;

	DECL_LINK(NewDelHdl, PushButton*);
	DECL_LINK(SelectHdl, ListBox*);
	DECL_LINK(ModifyHdl, Edit*);

	void 			RefillReplaceBoxes(BOOL bFromReset, //Box mit neuer Sprache fuellen
										LanguageType eOldLanguage,
										LanguageType eNewLanguage);
public:
						OfaAutocorrExceptPage( Window* pParent, const SfxItemSet& rSet );
						~OfaAutocorrExceptPage();

	static SfxTabPage*	Create( Window* pParent,
								const SfxItemSet& rAttrSet);

	virtual	BOOL 		FillItemSet( SfxItemSet& rSet );
	virtual	void 		Reset( const SfxItemSet& rSet );
	virtual void		ActivatePage( const SfxItemSet& );
	virtual int			DeactivatePage( SfxItemSet* pSet = 0 );
	void				SetLanguage(LanguageType eSet);

};

// class OfaQuoteTabPage -------------------------------------------------

class OfaQuoteTabPage : public SfxTabPage
{
	using TabPage::ActivatePage;

private:

	FixedLine	aSingleFL;
	CheckBox    aSingleTypoCB;
	FixedText	aSglStartQuoteFT;
	PushButton  aSglStartQuotePB;
	FixedText	aSglStartExFT;
	FixedText	aSglEndQuoteFT;
	PushButton	aSglEndQuotePB;
	FixedText	aSglEndExFT;
	PushButton	aSglStandardPB;

	FixedLine	aDoubleFL;
	CheckBox    aTypoCB;
	FixedText	aStartQuoteFT;
	PushButton  aStartQuotePB;
	FixedText	aDblStartExFT;
	FixedText	aEndQuoteFT;
	PushButton	aEndQuotePB;
	FixedText	aDblEndExFT;
	PushButton	aDblStandardPB;

	String		sStartQuoteDlg;
	String		sEndQuoteDlg;

	String 		sStandard;


	sal_UCS4	cSglStartQuote;
	sal_UCS4	cSglEndQuote;

	sal_UCS4	cStartQuote;
	sal_UCS4	cEndQuote;

	DECL_LINK( QuoteHdl, PushButton* );
	DECL_LINK( StdQuoteHdl, PushButton* );

	String 				ChangeStringExt_Impl( sal_UCS4 );

						OfaQuoteTabPage( Window* pParent, const SfxItemSet& rSet );
public:
						~OfaQuoteTabPage();

	static SfxTabPage*	Create( Window* pParent,
								const SfxItemSet& rAttrSet);

	virtual	BOOL 		FillItemSet( SfxItemSet& rSet );
	virtual	void 		Reset( const SfxItemSet& rSet );
	virtual void		ActivatePage( const SfxItemSet& );
};

// class OfaAutoCompleteTabPage ---------------------------------------------

class OfaAutoCompleteTabPage : public SfxTabPage
{
	using TabPage::ActivatePage;

	class AutoCompleteMultiListBox : public MultiListBox
	{
		OfaAutoCompleteTabPage& rPage;
	public:
		AutoCompleteMultiListBox( OfaAutoCompleteTabPage& rPg,
									const ResId& rResId )
			: MultiListBox( &rPg, rResId ), rPage( rPg ) {}

		virtual long PreNotify( NotifyEvent& rNEvt );
	};

    CheckBox        aCBActiv; //Enable word completion
    CheckBox        aCBAppendSpace;//Append space
    CheckBox        aCBAsTip; //Show as tip

    CheckBox        aCBCollect;//Collect words
    CheckBox        aCBRemoveList;//...save the list for later use...
    //--removed--CheckBox        aCBEndless;//

    FixedText       aFTExpandKey;
	ListBox 		aDCBExpandKey;
    FixedText       aFTMinWordlen;
    NumericField    aNFMinWordlen;
    FixedText       aFTMaxEntries;
    NumericField    aNFMaxEntries;
    AutoCompleteMultiListBox    aLBEntries;
	PushButton					aPBEntries;
	SvStringsISortDtor* 		pAutoCmpltList;
	USHORT 						nAutoCmpltListCnt;

	DECL_LINK( CheckHdl, CheckBox* );

						OfaAutoCompleteTabPage( Window* pParent,
												const SfxItemSet& rSet );
public:
						virtual ~OfaAutoCompleteTabPage();

	static SfxTabPage*	Create( Window* pParent,
								const SfxItemSet& rAttrSet);

	virtual	BOOL 		FillItemSet( SfxItemSet& rSet );
	virtual	void 		Reset( const SfxItemSet& rSet );
	virtual void		ActivatePage( const SfxItemSet& );

	void CopyToClipboard() const;
	DECL_LINK( DeleteHdl, PushButton* );
};

// class OfaSmartTagOptionsTabPage ---------------------------------------------

/** Smart tag options tab page

    This tab page is used to enable/disable smart tag types
*/
class OfaSmartTagOptionsTabPage : public SfxTabPage
{
	using TabPage::ActivatePage;

private:
    
	// controls
    CheckBox				m_aMainCB;
	SvxCheckListBox			m_aSmartTagTypesLB;
	PushButton				m_aPropertiesPB;
	FixedText				m_aTitleFT;

    // construction via Create()
    OfaSmartTagOptionsTabPage( Window* pParent,	const SfxItemSet& rSet );
    
    /** Inserts items into m_aSmartTagTypesLB
    
        Reads out the smart tag types supported by the SmartTagMgr and
        inserts the associated strings into the list box.
    */
    void FillListBox( const SmartTagMgr& rSmartTagMgr );
    
    /** Clears the m_aSmartTagTypesLB
    */
    void ClearListBox();

    /** Handler for the check box

        Enables/disables all controls in the tab page (except from the
        check box.
    */
	DECL_LINK( CheckHdl, CheckBox* );
    
    /** Handler for the push button

        Calls the displayPropertyPage function of the smart tag recognizer
        associated with the currently selected smart tag type.
    */
	DECL_LINK( ClickHdl, PushButton* );

    /** Handler for the list box

        Enables/disables the properties push button if selection in the
        smart tag types list box changes.
    */
    DECL_LINK( SelectHdl, SvxCheckListBox* );
    
public:

	virtual ~OfaSmartTagOptionsTabPage();

	static SfxTabPage*	Create( Window* pParent, const SfxItemSet& rAttrSet);

	virtual	BOOL 		FillItemSet( SfxItemSet& rSet );
	virtual	void 		Reset( const SfxItemSet& rSet );
	virtual void		ActivatePage( const SfxItemSet& );
};

#endif // _OFA_AUTOCDLG_CXX

#endif //

