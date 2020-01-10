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
#include "precompiled_svx.hxx"

#ifdef SVX_DLLIMPLEMENTATION
#undef SVX_DLLIMPLEMENTATION
#endif

#define _SVSTDARR_STRINGSISORTDTOR
#define _SVSTDARR_STRINGSDTOR
#include <svtools/svstdarr.hxx>
#include <vcl/msgbox.hxx>
#include <vcl/field.hxx>
#include <vcl/keycodes.hxx>
#include <sot/exchange.hxx>
#include <svtools/transfer.hxx>
#include <svtools/syslocale.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/viewsh.hxx>
#include <unotools/charclass.hxx>
#include <unotools/collatorwrapper.hxx>
#include <com/sun/star/i18n/CollatorOptions.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <comphelper/processfactory.hxx>

#include <vcl/svapp.hxx>
#include <sfx2/module.hxx>
#include <sfx2/request.hxx>
#include <sfx2/sfxsids.hrc>
#include <svtools/eitem.hxx>
#include <svtools/languageoptions.hxx>
#include <svx/SmartTagMgr.hxx>
#include <com/sun/star/smarttags/XSmartTagRecognizer.hpp>
#include <com/sun/star/smarttags/XSmartTagAction.hpp>

#define _OFA_AUTOCDLG_CXX
#include "autocdlg.hxx"
#include "autocdlg.hrc"
#include "helpid.hrc"
#include "acorrcfg.hxx"
#include <svx/svxacorr.hxx>
#include "cuicharmap.hxx"
#include "unolingu.hxx"
#include <svx/dialmgr.hxx>

static LanguageType eLastDialogLanguage = LANGUAGE_SYSTEM;

using namespace ::com::sun::star::util;
using namespace ::com::sun::star;
using namespace ::rtl;

static ::com::sun::star::uno::Reference<
		::com::sun::star::lang::XMultiServiceFactory >& GetProcessFact()
{
	static ::com::sun::star::uno::Reference<
			::com::sun::star::lang::XMultiServiceFactory > xMSF =
									::comphelper::getProcessServiceFactory();
	return xMSF;
}

/*-----------------14.10.96 15.47-------------------

--------------------------------------------------*/

OfaAutoCorrDlg::OfaAutoCorrDlg(Window* pParent, const SfxItemSet* _pSet ) :
    SfxTabDialog(pParent, SVX_RES( RID_OFA_AUTOCORR_DLG ), _pSet),
	aLanguageFT(    this, SVX_RES(FT_LANG 		  )),
	aLanguageLB(    this, SVX_RES(LB_LANG    		  ))
{
	BOOL bShowSWOptions = FALSE;
    BOOL bOpenSmartTagOptions = FALSE;

    if ( _pSet )
	{
        SFX_ITEMSET_ARG( _pSet, pItem, SfxBoolItem, SID_AUTO_CORRECT_DLG, FALSE );
		if ( pItem && pItem->GetValue() )
			bShowSWOptions = TRUE;

        SFX_ITEMSET_ARG( _pSet, pItem2, SfxBoolItem, SID_OPEN_SMARTTAGOPTIONS, FALSE );
		if ( pItem2 && pItem2->GetValue() )
			bOpenSmartTagOptions = TRUE;
	}

	aLanguageFT.SetZOrder(0, WINDOW_ZORDER_FIRST);
	aLanguageLB.SetZOrder(&aLanguageFT, WINDOW_ZORDER_BEHIND);
	aLanguageLB.SetHelpId(HID_AUTOCORR_LANGUAGE);
	FreeResource();

	AddTabPage(RID_OFAPAGE_AUTOCORR_OPTIONS, OfaAutocorrOptionsPage::Create, 0);
	AddTabPage(RID_OFAPAGE_AUTOFMT_APPLY, OfaSwAutoFmtOptionsPage::Create, 0);
	AddTabPage(RID_OFAPAGE_AUTOCOMPLETE_OPTIONS, OfaAutoCompleteTabPage::Create, 0);
    AddTabPage(RID_OFAPAGE_SMARTTAG_OPTIONS, OfaSmartTagOptionsTabPage::Create, 0);

	if (!bShowSWOptions)
	{
		RemoveTabPage(RID_OFAPAGE_AUTOFMT_APPLY);
		RemoveTabPage(RID_OFAPAGE_AUTOCOMPLETE_OPTIONS);
		RemoveTabPage(RID_OFAPAGE_SMARTTAG_OPTIONS);
	}
	else
    {
        // remove smart tag tab page if no extensions are installed
        SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
        SvxSwAutoFmtFlags *pOpt = &pAutoCorrect->GetSwFlags();
        if ( !pOpt || !pOpt->pSmartTagMgr || 0 == pOpt->pSmartTagMgr->NumberOfRecognizers() )
            RemoveTabPage(RID_OFAPAGE_SMARTTAG_OPTIONS);

		RemoveTabPage(RID_OFAPAGE_AUTOCORR_OPTIONS);
    }

	AddTabPage(RID_OFAPAGE_AUTOCORR_EXCEPT,  OfaAutocorrExceptPage::Create, 0);
	AddTabPage(RID_OFAPAGE_AUTOCORR_QUOTE, 	 OfaQuoteTabPage::Create, 0);

	// initialize languages
	//! LANGUAGE_NONE is displayed as '[All]' and the LanguageType
	//! will be set to LANGUAGE_DONTKNOW
    sal_Int16 nLangList = LANG_LIST_WESTERN;
    
    if( SvtLanguageOptions().IsCTLFontEnabled() ) 
        nLangList |= LANG_LIST_CTL;
    aLanguageLB.SetLanguageList( nLangList, TRUE, TRUE );
	aLanguageLB.SelectLanguage( LANGUAGE_NONE );
	USHORT nPos = aLanguageLB.GetSelectEntryPos();
	DBG_ASSERT( LISTBOX_ENTRY_NOTFOUND != nPos, "listbox entry missing" );
	aLanguageLB.SetEntryData( nPos, (void*)(long) LANGUAGE_DONTKNOW );

	// Initialisierung funktionier fuer static nicht unter Linux - deswegen hier
	if( LANGUAGE_SYSTEM == eLastDialogLanguage )
		eLastDialogLanguage = Application::GetSettings().GetLanguage();

	LanguageType nSelectLang = LANGUAGE_DONTKNOW;
	nPos = aLanguageLB.GetEntryPos( (void*)(long) eLastDialogLanguage );
	if (LISTBOX_ENTRY_NOTFOUND != nPos)
		nSelectLang = eLastDialogLanguage;
	aLanguageLB.SelectLanguage( nSelectLang );

	aLanguageLB.SetSelectHdl(LINK(this, OfaAutoCorrDlg, SelectLanguageHdl));

    Size aMinSize(aLanguageFT.CalcMinimumSize());
    //reserve some extra space for CJK accelerators that are possible inserted
    //later (like '(A)')
    aLanguageFT.SetPosSizePixel( 0, 0, aMinSize.Width() + 20, 0, WINDOW_POSSIZE_WIDTH );

    if ( bOpenSmartTagOptions )
        SetCurPageId( RID_OFAPAGE_SMARTTAG_OPTIONS );
}
/*-----------------16.10.96 14.06-------------------

--------------------------------------------------*/

BOOL lcl_FindEntry( ListBox& rLB, const String& rEntry,
					CollatorWrapper& rCmpClass )
{
	USHORT nCount = rLB.GetEntryCount();
	USHORT nSelPos = rLB.GetSelectEntryPos();
	USHORT i;
	for(i = 0; i < nCount; i++)
	{
		if( 0 == rCmpClass.compareString(rEntry, rLB.GetEntry(i) ))
		{
			rLB.SelectEntryPos(i, TRUE);
			return TRUE;
		}
	}
	if(LISTBOX_ENTRY_NOTFOUND != nSelPos)
		rLB.SelectEntryPos(nSelPos, FALSE);
	return FALSE;
}

/* -----------------23.11.98 10:46-------------------
 *
 * --------------------------------------------------*/
IMPL_LINK(OfaAutoCorrDlg, SelectLanguageHdl, ListBox*, pBox)
{
	USHORT nPos = pBox->GetSelectEntryPos();
	void* pVoid = pBox->GetEntryData(nPos);
	LanguageType eNewLang = (LanguageType)(long)pVoid;
	//alte Einstellungen speichern und neu fuellen
	if(eNewLang != eLastDialogLanguage)
	{
		USHORT	nPageId = GetCurPageId();
		if(RID_OFAPAGE_AUTOCORR_EXCEPT == nPageId)
			((OfaAutocorrExceptPage*)GetTabPage( nPageId ))->SetLanguage(eNewLang);
	}
	return 0;
}
/*-----------------14.10.96 15.57-------------------

--------------------------------------------------*/

OfaAutocorrOptionsPage::OfaAutocorrOptionsPage( Window* pParent,
												const SfxItemSet& rSet ) :
	SfxTabPage(pParent, SVX_RES( RID_OFAPAGE_AUTOCORR_OPTIONS ), rSet),
	aCheckLB			(this, SVX_RES(CLB_SETTINGS	)),

    sDoubleCaps         (SVX_RES(ST_CPTL_STT_WORD     )),
    sStartCap           (SVX_RES(ST_CPTL_STT_SENT     )),
    sBoldUnderline      (SVX_RES(ST_BOLD_UNDER        )),
    sURL                (SVX_RES(ST_DETECT_URL        )),
    sNoDblSpaces        (SVX_RES(STR_NO_DBL_SPACES    )),
    sHalf               (SVX_RES(ST_FRACTION          )),
	sDash    			(SVX_RES(ST_DASH	         	)),
    sFirst              (SVX_RES(ST_ORDINAL           ))
{
	FreeResource();

	aCheckLB.SetHelpId(HID_OFAPAGE_AUTOCORR_CLB);
}

/*-----------------14.10.96 15.58-------------------

--------------------------------------------------*/


OfaAutocorrOptionsPage::~OfaAutocorrOptionsPage()
{
}

/*-----------------14.10.96 15.58-------------------

--------------------------------------------------*/


SfxTabPage*	OfaAutocorrOptionsPage::Create( Window* pParent,
								const SfxItemSet& rSet)
{
	return new OfaAutocorrOptionsPage(pParent, rSet);
}
/*-----------------14.10.96 15.58-------------------

--------------------------------------------------*/


BOOL OfaAutocorrOptionsPage::FillItemSet( SfxItemSet& )
{
	SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
	long nFlags = pAutoCorrect->GetFlags();

	USHORT nPos = 0;
	pAutoCorrect->SetAutoCorrFlag(CptlSttWrd, 			aCheckLB.IsChecked(nPos++));
	pAutoCorrect->SetAutoCorrFlag(CptlSttSntnc, 		aCheckLB.IsChecked(nPos++));
	pAutoCorrect->SetAutoCorrFlag(ChgWeightUnderl,		aCheckLB.IsChecked(nPos++));
	pAutoCorrect->SetAutoCorrFlag(SetINetAttr, 			aCheckLB.IsChecked(nPos++));
	pAutoCorrect->SetAutoCorrFlag(ChgOrdinalNumber,		aCheckLB.IsChecked(nPos++));
	pAutoCorrect->SetAutoCorrFlag(ChgFractionSymbol,	aCheckLB.IsChecked(nPos++));
	pAutoCorrect->SetAutoCorrFlag(ChgToEnEmDash,		aCheckLB.IsChecked(nPos++));
	pAutoCorrect->SetAutoCorrFlag(IngnoreDoubleSpace,	aCheckLB.IsChecked(nPos++));

	BOOL bReturn = nFlags != pAutoCorrect->GetFlags();
	if(bReturn )
    {
        SvxAutoCorrCfg* pCfg = SvxAutoCorrCfg::Get();
        pCfg->SetModified();
        pCfg->Commit();
    }
	return bReturn;
}

/* -----------------23.11.98 16:15-------------------
 *
 * --------------------------------------------------*/
void	OfaAutocorrOptionsPage::ActivatePage( const SfxItemSet& )
{
	((OfaAutoCorrDlg*)GetTabDialog())->EnableLanguage(FALSE);
}

/*-----------------14.10.96 15.58-------------------

--------------------------------------------------*/


void OfaAutocorrOptionsPage::Reset( const SfxItemSet& )
{
	SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
	const long nFlags = pAutoCorrect->GetFlags();

	aCheckLB.SetUpdateMode(FALSE);
	aCheckLB.Clear();

	aCheckLB.InsertEntry(sDoubleCaps);
	aCheckLB.InsertEntry(sStartCap);
	aCheckLB.InsertEntry(sBoldUnderline);
	aCheckLB.InsertEntry(sURL);
	aCheckLB.InsertEntry(sFirst);
	aCheckLB.InsertEntry(sHalf);
	aCheckLB.InsertEntry(sDash);
	aCheckLB.InsertEntry(sNoDblSpaces);

	USHORT nPos = 0;
	aCheckLB.CheckEntryPos( nPos++, 0 != (nFlags & CptlSttWrd) );
	aCheckLB.CheckEntryPos( nPos++, 0 != (nFlags & CptlSttSntnc) );
	aCheckLB.CheckEntryPos( nPos++, 0 != (nFlags & ChgWeightUnderl) );
	aCheckLB.CheckEntryPos( nPos++, 0 != (nFlags & SetINetAttr) );
	aCheckLB.CheckEntryPos( nPos++, 0 != (nFlags & ChgOrdinalNumber) );
	aCheckLB.CheckEntryPos( nPos++, 0 != (nFlags & ChgFractionSymbol) );
	aCheckLB.CheckEntryPos( nPos++, 0 != (nFlags & ChgToEnEmDash) );
	aCheckLB.CheckEntryPos( nPos++, 0 != (nFlags & IngnoreDoubleSpace) );

	aCheckLB.SetUpdateMode(TRUE);
}

/*********************************************************************/
/*                                                                   */
/*  Hilfs-struct fuer dUserDaten der Checklistbox                    */
/*                                                                   */
/*********************************************************************/

struct ImpUserData
{
	String 	*pString;
	Font* 	pFont;

	ImpUserData(String* pText, Font* pFnt)
		{ pString = pText; pFont = pFnt;}
};


/*********************************************************************/
/*                                                                   */
/*	Dialog fuer Prozenteinstellung                                   */
/*                                                                   */
/*********************************************************************/

class OfaAutoFmtPrcntSet : public ModalDialog
{
	OKButton 		aOKPB;
	CancelButton	aCancelPB;
	FixedLine		aPrcntFL;
	MetricField		aPrcntMF;

	public:
		OfaAutoFmtPrcntSet(Window* pParent) :
			ModalDialog(pParent, SVX_RES(RID_OFADLG_PRCNT_SET)),
				aOKPB(this, 		SVX_RES(BT_OK)),
				aCancelPB(this, 	SVX_RES(BT_CANCEL)),
				aPrcntFL(this, 		SVX_RES(FL_PRCNT)),
				aPrcntMF(this, 	SVX_RES(ED_RIGHT_MARGIN))
			{
				FreeResource();
			}
	MetricField& 	GetPrcntFld(){return aPrcntMF;}
};


/*********************************************************************/
/*                                                                   */
/*  veraenderter LBoxString                                          */
/*                                                                   */
/*********************************************************************/

class OfaImpBrwString : public SvLBoxString
{
public:

	OfaImpBrwString( SvLBoxEntry* pEntry, USHORT nFlags,
		const String& rStr ) : SvLBoxString(pEntry,nFlags,rStr){}

	virtual void Paint( const Point& rPos, SvLBox& rDev, USHORT nFlags,
											SvLBoxEntry* pEntry);
};

/*********************************************************************/
/*                                                                   */
/*********************************************************************/


void __EXPORT OfaImpBrwString::Paint( const Point& rPos, SvLBox& rDev, USHORT /*nFlags*/,
	SvLBoxEntry* pEntry )
{
	rDev.DrawText( rPos, GetText() );
	if(pEntry->GetUserData())
	{
		ImpUserData* pUserData = (ImpUserData* )pEntry->GetUserData();
		Point aNewPos(rPos);
		aNewPos.X() += rDev.GetTextWidth(GetText());
		Font aOldFont( rDev.GetFont());
		Font aFont( aOldFont );
		if(pUserData->pFont)
		{
			aFont = *pUserData->pFont;
			aFont.SetColor(aOldFont.GetColor());
			aFont.SetSize(aOldFont.GetSize());
		}
		aFont.SetWeight( WEIGHT_BOLD );

		BOOL bFett = TRUE;
		USHORT nPos = 0;
		do {
			String sTxt( pUserData->pString->GetToken( 0, 1, nPos ));

			if( bFett )
				rDev.SetFont( aFont );

			rDev.DrawText( aNewPos, sTxt );

			if( STRING_NOTFOUND != nPos )
				aNewPos.X() += rDev.GetTextWidth( sTxt );

			if( bFett )
				rDev.SetFont( aOldFont );

			bFett = !bFett;
		} while( STRING_NOTFOUND != nPos );
	}
}

/*********************************************************************/
/*                                                                   */
/* 	TabPage Autoformat anwenden			                             */
/*                                                                   */
/*********************************************************************/

#define CBCOL_FIRST		0
#define CBCOL_SECOND	1
#define CBCOL_BOTH		2

enum OfaAutoFmtOptions
{
	CORR_UPPER,
	BEGIN_UPPER,
	BOLD_UNDERLINE,
	DETECT_URL,
	REPLACE_1ST,
	REPLACE_HALF,
	REPLACE_DASHES,
	DEL_SPACES_AT_STT_END,
	DEL_SPACES_BETWEEN_LINES,
	IGNORE_DBLSPACE,
	APPLY_NUMBERING,
	INSERT_BORDER,
	CREATE_TABLE,
	REPLACE_STYLES,
	DEL_EMPTY_NODE,
	REPLACE_USER_COLL,
	REPLACE_BULLETS,
	REPLACE_QUOTATION,
	MERGE_SINGLE_LINE_PARA
};

OfaSwAutoFmtOptionsPage::OfaSwAutoFmtOptionsPage( Window* pParent,
								const SfxItemSet& rSet ) :
	SfxTabPage(pParent, SVX_RES(RID_OFAPAGE_AUTOFMT_APPLY), rSet),
	aCheckLB			(this,	SVX_RES(CLB_SETTINGS)),
	aEditPB				(this, 	SVX_RES(PB_EDIT)),
	aHeader1Expl		(this, 	SVX_RES(FT_HEADER1_EXPLANATION)),
	aHeader2Expl		(this, 	SVX_RES(FT_HEADER2_EXPLANATION)),
	sHeader1			(SVX_RES( STR_HEADER1		)),
	sHeader2			(SVX_RES( STR_HEADER2		)),
	sDeleteEmptyPara	(SVX_RES( ST_DEL_EMPTY_PARA)),
	sCptlSttWord		(SVX_RES(	ST_CPTL_STT_WORD)),
	sCptlSttSent		(SVX_RES(	ST_CPTL_STT_SENT)),
	sTypo				(SVX_RES(	ST_TYPO         )),
	sUserStyle			(SVX_RES(	ST_USER_STYLE   )),
	sBullet				(SVX_RES(	ST_BULLET       )),
	sBoldUnder			(SVX_RES(	ST_BOLD_UNDER   )),
	sNoDblSpaces		(SVX_RES(	STR_NO_DBL_SPACES)),
	sFraction			(SVX_RES(	ST_FRACTION     )),
	sDetectURL			(SVX_RES(	ST_DETECT_URL   )),
	sDash				(SVX_RES(	ST_DASH         )),
	sOrdinal			(SVX_RES(	ST_ORDINAL      )),
	sRightMargin		(SVX_RES(	ST_RIGHT_MARGIN	)),
	sNum				(SVX_RES(	STR_NUM			)),
	sBorder				(SVX_RES(	STR_BORDER		)),
	sTable				(SVX_RES(	STR_TABLE		)),
	sReplaceTemplates   (SVX_RES(	STR_REPLACE_TEMPLATES)),
	sDelSpaceAtSttEnd	(SVX_RES(	STR_DEL_SPACES_AT_STT_END)),
	sDelSpaceBetweenLines(SVX_RES(STR_DEL_SPACES_BETWEEN_LINES)),

	nPercent		( 50 ),
	pCheckButtonData( NULL )

{
	FreeResource();

	//typ. Anfuehrungszeichen einsetzen
	SvtSysLocale aSysLcl;
	const LocaleDataWrapper& rLcl = aSysLcl.GetLocaleData();
	sTypo.SearchAndReplace( String::CreateFromAscii("%1"),
										rLcl.getDoubleQuotationMarkStart());
	sTypo.SearchAndReplace( String::CreateFromAscii("%2"),
										rLcl.getDoubleQuotationMarkEnd());

	aCheckLB.SetHelpId(HID_OFAPAGE_AUTOFORMAT_CLB);
	aCheckLB.SetWindowBits(WB_HSCROLL| WB_VSCROLL);

	aCheckLB.SetSelectHdl(LINK(this, OfaSwAutoFmtOptionsPage, SelectHdl));
	aCheckLB.SetDoubleClickHdl(LINK(this, OfaSwAutoFmtOptionsPage, EditHdl));

	static long aStaticTabs[]=
	{
		3, 0, 20, 40
	};

	aCheckLB.SvxSimpleTable::SetTabs(aStaticTabs);
	String sHeader( sHeader1 );
	sHeader += '\t';
	sHeader += sHeader2;
	sHeader += '\t';
	aCheckLB.InsertHeaderEntry( sHeader, HEADERBAR_APPEND,
						HIB_CENTER | HIB_VCENTER | HIB_FIXEDPOS | HIB_FIXED);

	aEditPB.SetClickHdl(LINK(this, OfaSwAutoFmtOptionsPage, EditHdl));
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

SvLBoxEntry* OfaSwAutoFmtOptionsPage::CreateEntry(String& rTxt, USHORT nCol)
{
	SvLBoxEntry* pEntry = new SvLBoxEntry;

	if ( !pCheckButtonData )
	{
		pCheckButtonData = new SvLBoxButtonData( &aCheckLB );
		aCheckLB.SetCheckButtonData( pCheckButtonData );
	}

	pEntry->AddItem( new SvLBoxContextBmp( pEntry, 0, Image(), Image(), 0));	// Sonst Puff!

	String sEmpty;
	if (nCol == CBCOL_SECOND)
		pEntry->AddItem( new SvLBoxString( pEntry, 0, sEmpty) );	// Leerspalte
	else
		pEntry->AddItem( new SvLBoxButton( pEntry, SvLBoxButtonKind_enabledCheckbox, 0, pCheckButtonData ) );

	if (nCol == CBCOL_FIRST)
		pEntry->AddItem( new SvLBoxString( pEntry, 0, sEmpty) );	// Leerspalte
	else
		pEntry->AddItem( new SvLBoxButton( pEntry, SvLBoxButtonKind_enabledCheckbox, 0, pCheckButtonData ) );
	pEntry->AddItem( new OfaImpBrwString( pEntry, 0, rTxt ) );

	return pEntry;
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/


__EXPORT OfaSwAutoFmtOptionsPage::~OfaSwAutoFmtOptionsPage()
{
	delete (ImpUserData*) aCheckLB.GetUserData( REPLACE_BULLETS );
	delete (ImpUserData*) aCheckLB.GetUserData( APPLY_NUMBERING );
	delete (ImpUserData*) aCheckLB.GetUserData( MERGE_SINGLE_LINE_PARA );
	delete pCheckButtonData;
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

SfxTabPage* __EXPORT OfaSwAutoFmtOptionsPage::Create( Window* pParent,
								const SfxItemSet& rAttrSet)
{
	return new OfaSwAutoFmtOptionsPage(pParent, rAttrSet);
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

BOOL OfaSwAutoFmtOptionsPage::FillItemSet( SfxItemSet&  )
{
	BOOL bModified = FALSE;
	SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
	SvxSwAutoFmtFlags *pOpt = &pAutoCorrect->GetSwFlags();
	long nFlags = pAutoCorrect->GetFlags();

	BOOL bCheck = aCheckLB.IsChecked(CORR_UPPER, CBCOL_FIRST);
	bModified |= pOpt->bCptlSttWrd != bCheck;
	pOpt->bCptlSttWrd = bCheck;
	pAutoCorrect->SetAutoCorrFlag(CptlSttWrd,
						aCheckLB.IsChecked(CORR_UPPER, CBCOL_SECOND));

	bCheck = aCheckLB.IsChecked(BEGIN_UPPER, CBCOL_FIRST);
	bModified |= pOpt->bCptlSttSntnc != bCheck;
	pOpt->bCptlSttSntnc = bCheck;
	pAutoCorrect->SetAutoCorrFlag(CptlSttSntnc,
						aCheckLB.IsChecked(BEGIN_UPPER, CBCOL_SECOND));

	bCheck = aCheckLB.IsChecked(BOLD_UNDERLINE, CBCOL_FIRST);
	bModified |= pOpt->bChgWeightUnderl != bCheck;
	pOpt->bChgWeightUnderl = bCheck;
	pAutoCorrect->SetAutoCorrFlag(ChgWeightUnderl,
						aCheckLB.IsChecked(BOLD_UNDERLINE, CBCOL_SECOND));

	pAutoCorrect->SetAutoCorrFlag(IngnoreDoubleSpace,
						aCheckLB.IsChecked(IGNORE_DBLSPACE, CBCOL_SECOND));

	bCheck = aCheckLB.IsChecked(DETECT_URL, CBCOL_FIRST);
	bModified |= pOpt->bSetINetAttr != bCheck;
	pOpt->bSetINetAttr = bCheck;
	pAutoCorrect->SetAutoCorrFlag(SetINetAttr,
						aCheckLB.IsChecked(DETECT_URL, CBCOL_SECOND));

	bCheck = aCheckLB.IsChecked(REPLACE_1ST, CBCOL_FIRST);
	bModified |= pOpt->bChgOrdinalNumber != bCheck;
	pOpt->bChgOrdinalNumber = bCheck;
	pAutoCorrect->SetAutoCorrFlag(ChgOrdinalNumber,
						aCheckLB.IsChecked(REPLACE_1ST, CBCOL_SECOND));

	bCheck = aCheckLB.IsChecked(DEL_EMPTY_NODE, CBCOL_FIRST);
	bModified |= pOpt->bDelEmptyNode != bCheck;
	pOpt->bDelEmptyNode = bCheck;

	bCheck = aCheckLB.IsChecked(REPLACE_QUOTATION, CBCOL_FIRST);
	bModified |= pOpt->bReplaceQuote != bCheck;
	pOpt->bReplaceQuote = bCheck;

	bCheck = aCheckLB.IsChecked(REPLACE_USER_COLL, CBCOL_FIRST);
	bModified |= pOpt->bChgUserColl != bCheck;
	pOpt->bChgUserColl = bCheck;

	bCheck = aCheckLB.IsChecked(REPLACE_BULLETS, CBCOL_FIRST);
	bModified |= pOpt->bChgEnumNum != bCheck;
	pOpt->bChgEnumNum = bCheck;
	bModified |= aBulletFont != pOpt->aBulletFont;
	pOpt->aBulletFont = aBulletFont;
	bModified |= String(pOpt->cBullet) != sBulletChar;
	pOpt->cBullet = sBulletChar.GetChar(0);

	bModified |= aByInputBulletFont != pOpt->aByInputBulletFont;
	bModified |= String(pOpt->cByInputBullet) != sByInputBulletChar;
	pOpt->aByInputBulletFont = aByInputBulletFont;
	pOpt->cByInputBullet = sByInputBulletChar.GetChar(0);

	bCheck = aCheckLB.IsChecked(MERGE_SINGLE_LINE_PARA, CBCOL_FIRST);
	bModified |= pOpt->bRightMargin != bCheck;
	pOpt->bRightMargin = bCheck;
	bModified |= nPercent != pOpt->nRightMargin;
	pOpt->nRightMargin = (BYTE)nPercent;

	bCheck = aCheckLB.IsChecked(APPLY_NUMBERING, CBCOL_SECOND);
	bModified |= pOpt->bSetNumRule != bCheck;
	pOpt->bSetNumRule = bCheck;

	bCheck = aCheckLB.IsChecked(INSERT_BORDER, CBCOL_SECOND);
	bModified |= pOpt->bSetBorder != bCheck;
	pOpt->bSetBorder = bCheck;

	bCheck = aCheckLB.IsChecked(CREATE_TABLE, CBCOL_SECOND);
	bModified |= pOpt->bCreateTable != bCheck;
	pOpt->bCreateTable = bCheck;

	bCheck = aCheckLB.IsChecked(REPLACE_STYLES, CBCOL_SECOND);
	bModified |= pOpt->bReplaceStyles != bCheck;
	pOpt->bReplaceStyles = bCheck;

	bCheck = aCheckLB.IsChecked(REPLACE_HALF, CBCOL_FIRST);
	bModified |= pOpt->bChgFracionSymbol != bCheck;
	pOpt->bChgFracionSymbol = bCheck;
	pAutoCorrect->SetAutoCorrFlag(ChgFractionSymbol,
						aCheckLB.IsChecked(REPLACE_HALF, CBCOL_SECOND));

	bCheck = aCheckLB.IsChecked(REPLACE_DASHES, CBCOL_FIRST);
	bModified |= pOpt->bChgToEnEmDash != bCheck;
	pOpt->bChgToEnEmDash = bCheck;
	pAutoCorrect->SetAutoCorrFlag(ChgToEnEmDash,
						aCheckLB.IsChecked(REPLACE_DASHES, CBCOL_SECOND));

	bCheck = aCheckLB.IsChecked(DEL_SPACES_AT_STT_END, CBCOL_FIRST);
	bModified |= pOpt->bAFmtDelSpacesAtSttEnd != bCheck;
	pOpt->bAFmtDelSpacesAtSttEnd = bCheck;
	bCheck = aCheckLB.IsChecked(DEL_SPACES_AT_STT_END, CBCOL_SECOND);
	bModified |= pOpt->bAFmtByInpDelSpacesAtSttEnd != bCheck;
	pOpt->bAFmtByInpDelSpacesAtSttEnd = bCheck;

	bCheck = aCheckLB.IsChecked(DEL_SPACES_BETWEEN_LINES, CBCOL_FIRST);
	bModified |= pOpt->bAFmtDelSpacesBetweenLines != bCheck;
	pOpt->bAFmtDelSpacesBetweenLines = bCheck;
	bCheck = aCheckLB.IsChecked(DEL_SPACES_BETWEEN_LINES, CBCOL_SECOND);
	bModified |= pOpt->bAFmtByInpDelSpacesBetweenLines != bCheck;
	pOpt->bAFmtByInpDelSpacesBetweenLines = bCheck;

	if(bModified || nFlags != pAutoCorrect->GetFlags())
    {
        SvxAutoCorrCfg* pCfg = SvxAutoCorrCfg::Get();
        pCfg->SetModified();
        pCfg->Commit();
    }

	return TRUE;
}

/* -----------------23.11.98 16:15-------------------
 *
 * --------------------------------------------------*/
void	OfaSwAutoFmtOptionsPage::ActivatePage( const SfxItemSet& )
{
	((OfaAutoCorrDlg*)GetTabDialog())->EnableLanguage(FALSE);
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/


void OfaSwAutoFmtOptionsPage::Reset( const SfxItemSet& )
{
	SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
	SvxSwAutoFmtFlags *pOpt = &pAutoCorrect->GetSwFlags();
	const long nFlags = pAutoCorrect->GetFlags();

	aCheckLB.SetUpdateMode(FALSE);
	aCheckLB.Clear();

	// Die folgenden Eintraege muessen in der selben Reihenfolge, wie im
	// OfaAutoFmtOptions-enum eingefuegt werden!
	aCheckLB.GetModel()->Insert(CreateEntry(sCptlSttWord,		CBCOL_BOTH	));
	aCheckLB.GetModel()->Insert(CreateEntry(sCptlSttSent,		CBCOL_BOTH	));
	aCheckLB.GetModel()->Insert(CreateEntry(sBoldUnder,			CBCOL_BOTH	));
	aCheckLB.GetModel()->Insert(CreateEntry(sDetectURL,			CBCOL_BOTH	));
	aCheckLB.GetModel()->Insert(CreateEntry(sOrdinal,			CBCOL_BOTH	));
	aCheckLB.GetModel()->Insert(CreateEntry(sFraction,			CBCOL_BOTH  ));
	aCheckLB.GetModel()->Insert(CreateEntry(sDash,				CBCOL_BOTH  ));
	aCheckLB.GetModel()->Insert(CreateEntry(sDelSpaceAtSttEnd,	CBCOL_BOTH  ));
	aCheckLB.GetModel()->Insert(CreateEntry(sDelSpaceBetweenLines, CBCOL_BOTH  ));

	aCheckLB.GetModel()->Insert(CreateEntry(sNoDblSpaces,		CBCOL_SECOND));
	aCheckLB.GetModel()->Insert(CreateEntry(sNum,				CBCOL_SECOND));
	aCheckLB.GetModel()->Insert(CreateEntry(sBorder,			CBCOL_SECOND));
	aCheckLB.GetModel()->Insert(CreateEntry(sTable,				CBCOL_SECOND));
	aCheckLB.GetModel()->Insert(CreateEntry(sReplaceTemplates,	CBCOL_SECOND));
	aCheckLB.GetModel()->Insert(CreateEntry(sDeleteEmptyPara,	CBCOL_FIRST	));
	aCheckLB.GetModel()->Insert(CreateEntry(sUserStyle,			CBCOL_FIRST	));
	aCheckLB.GetModel()->Insert(CreateEntry(sBullet,			CBCOL_FIRST	));
	aCheckLB.GetModel()->Insert(CreateEntry(sTypo,				CBCOL_FIRST	));
	aCheckLB.GetModel()->Insert(CreateEntry(sRightMargin,		CBCOL_FIRST	));

	aCheckLB.CheckEntryPos( CORR_UPPER,			CBCOL_FIRST,	pOpt->bCptlSttWrd );
	aCheckLB.CheckEntryPos( CORR_UPPER,			CBCOL_SECOND,	0 != (nFlags & CptlSttWrd) );
	aCheckLB.CheckEntryPos( BEGIN_UPPER,		CBCOL_FIRST,	pOpt->bCptlSttSntnc );
	aCheckLB.CheckEntryPos( BEGIN_UPPER,		CBCOL_SECOND,	0 != (nFlags & CptlSttSntnc) );
	aCheckLB.CheckEntryPos( BOLD_UNDERLINE,		CBCOL_FIRST,	pOpt->bChgWeightUnderl );
	aCheckLB.CheckEntryPos( BOLD_UNDERLINE,		CBCOL_SECOND,	0 != (nFlags & ChgWeightUnderl) );
	aCheckLB.CheckEntryPos( IGNORE_DBLSPACE,	CBCOL_SECOND,	0 != (nFlags & IngnoreDoubleSpace) );
	aCheckLB.CheckEntryPos( DETECT_URL,			CBCOL_FIRST,	pOpt->bSetINetAttr );
	aCheckLB.CheckEntryPos( DETECT_URL,			CBCOL_SECOND,	0 != (nFlags & SetINetAttr) );
	aCheckLB.CheckEntryPos( REPLACE_1ST,		CBCOL_FIRST,	pOpt->bChgOrdinalNumber );
	aCheckLB.CheckEntryPos( REPLACE_1ST,		CBCOL_SECOND,	0 != (nFlags & ChgOrdinalNumber) );
	aCheckLB.CheckEntryPos( REPLACE_HALF,		CBCOL_FIRST,	pOpt->bChgFracionSymbol );
	aCheckLB.CheckEntryPos( REPLACE_HALF,		CBCOL_SECOND,	0 != (nFlags & ChgFractionSymbol) );
	aCheckLB.CheckEntryPos( REPLACE_DASHES,		CBCOL_FIRST,	pOpt->bChgToEnEmDash );
	aCheckLB.CheckEntryPos( REPLACE_DASHES,		CBCOL_SECOND,	0 != (nFlags & ChgToEnEmDash) );
	aCheckLB.CheckEntryPos( DEL_SPACES_AT_STT_END,		CBCOL_FIRST,	pOpt->bAFmtDelSpacesAtSttEnd );
	aCheckLB.CheckEntryPos( DEL_SPACES_AT_STT_END,		CBCOL_SECOND,	pOpt->bAFmtByInpDelSpacesAtSttEnd );
	aCheckLB.CheckEntryPos( DEL_SPACES_BETWEEN_LINES,	CBCOL_FIRST,	pOpt->bAFmtDelSpacesBetweenLines );
	aCheckLB.CheckEntryPos( DEL_SPACES_BETWEEN_LINES,	CBCOL_SECOND,	pOpt->bAFmtByInpDelSpacesBetweenLines );
	aCheckLB.CheckEntryPos( DEL_EMPTY_NODE,		CBCOL_FIRST,	pOpt->bDelEmptyNode );
	aCheckLB.CheckEntryPos( REPLACE_QUOTATION,	CBCOL_FIRST,	pOpt->bReplaceQuote );
	aCheckLB.CheckEntryPos( REPLACE_USER_COLL,	CBCOL_FIRST,	pOpt->bChgUserColl );
	aCheckLB.CheckEntryPos( REPLACE_BULLETS,	CBCOL_FIRST,	pOpt->bChgEnumNum );

	aBulletFont = pOpt->aBulletFont;
	sBulletChar = pOpt->cBullet;
	ImpUserData* pUserData = new ImpUserData(&sBulletChar, &aBulletFont);
	aCheckLB.SetUserData(  REPLACE_BULLETS, pUserData );

	nPercent = pOpt->nRightMargin;
	sMargin = ' ';
	sMargin += String::CreateFromInt32( nPercent );
	sMargin += '%';
	pUserData = new ImpUserData(&sMargin, 0);
	aCheckLB.SetUserData( MERGE_SINGLE_LINE_PARA, pUserData );

	aCheckLB.CheckEntryPos( APPLY_NUMBERING,	CBCOL_SECOND,	pOpt->bSetNumRule );

	aByInputBulletFont = pOpt->aByInputBulletFont;
	sByInputBulletChar = pOpt->cByInputBullet;
	ImpUserData* pUserData2 = new ImpUserData(&sByInputBulletChar, &aByInputBulletFont);
	aCheckLB.SetUserData( APPLY_NUMBERING , pUserData2 );

	aCheckLB.CheckEntryPos( MERGE_SINGLE_LINE_PARA, CBCOL_FIRST, pOpt->bRightMargin );
	aCheckLB.CheckEntryPos( INSERT_BORDER,		CBCOL_SECOND,	pOpt->bSetBorder );
	aCheckLB.CheckEntryPos( CREATE_TABLE,		CBCOL_SECOND,	pOpt->bCreateTable );
	aCheckLB.CheckEntryPos( REPLACE_STYLES,		CBCOL_SECOND,	pOpt->bReplaceStyles );

	aCheckLB.SetUpdateMode(TRUE);
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

IMPL_LINK(OfaSwAutoFmtOptionsPage, SelectHdl, OfaACorrCheckListBox*, pBox)
{
	aEditPB.Enable(0 != pBox->FirstSelected()->GetUserData());
	return 0;
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

IMPL_LINK(OfaSwAutoFmtOptionsPage, EditHdl, PushButton*, EMPTYARG)
{
    ULONG nSelEntryPos = aCheckLB.GetSelectEntryPos();
	if( nSelEntryPos == REPLACE_BULLETS ||
		nSelEntryPos == APPLY_NUMBERING)
	{
		SvxCharacterMap *pMapDlg = new SvxCharacterMap(this);
		ImpUserData* pUserData = (ImpUserData*)aCheckLB.FirstSelected()->GetUserData();
		pMapDlg->SetCharFont(*pUserData->pFont);
		pMapDlg->SetChar( pUserData->pString->GetChar(0) );
		if(RET_OK == pMapDlg->Execute())
		{
			Font aFont(pMapDlg->GetCharFont());
			*pUserData->pFont = aFont;
			sal_UCS4 aChar = pMapDlg->GetChar();
			// using the UCS4 constructor
			rtl::OUString aOUStr( &aChar, 1 );
			*pUserData->pString = aOUStr;
		}
		delete pMapDlg;
	}
	else if( MERGE_SINGLE_LINE_PARA == nSelEntryPos )
	{
		// Dialog fuer Prozenteinstellung
		OfaAutoFmtPrcntSet aDlg(this);
		aDlg.GetPrcntFld().SetValue(nPercent);
		if(RET_OK == aDlg.Execute())
		{
			nPercent = (USHORT)aDlg.GetPrcntFld().GetValue();
			sMargin = ' ';
			sMargin += String::CreateFromInt32( nPercent );
			sMargin += '%';
		}
	}
	aCheckLB.Invalidate();
	return 0;
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

void OfaACorrCheckListBox::SetTabs()
{
	SvxSimpleTable::SetTabs();
	USHORT nAdjust = SV_LBOXTAB_ADJUST_RIGHT|SV_LBOXTAB_ADJUST_LEFT|SV_LBOXTAB_ADJUST_CENTER|SV_LBOXTAB_ADJUST_NUMERIC|SV_LBOXTAB_FORCE;

	if( aTabs.Count() > 1 )
	{
		SvLBoxTab* pTab = (SvLBoxTab*)aTabs.GetObject(1);
		pTab->nFlags &= ~nAdjust;
		pTab->nFlags |= SV_LBOXTAB_PUSHABLE|SV_LBOXTAB_ADJUST_CENTER|SV_LBOXTAB_FORCE;
	}
	if( aTabs.Count() > 2 )
	{
		SvLBoxTab* pTab = (SvLBoxTab*)aTabs.GetObject(2);
		pTab->nFlags &= ~nAdjust;
		pTab->nFlags |= SV_LBOXTAB_PUSHABLE|SV_LBOXTAB_ADJUST_CENTER|SV_LBOXTAB_FORCE;
	}
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

void OfaACorrCheckListBox::CheckEntryPos(ULONG nPos, USHORT nCol, BOOL bChecked)
{
	if ( nPos < GetEntryCount() )
		SetCheckButtonState(
			GetEntry(nPos),
			nCol,
			bChecked ? SvButtonState( SV_BUTTON_CHECKED ) :
									   SvButtonState( SV_BUTTON_UNCHECKED ) );
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

BOOL OfaACorrCheckListBox::IsChecked(ULONG nPos, USHORT nCol)
{
	return GetCheckButtonState( GetEntry(nPos), nCol ) == SV_BUTTON_CHECKED;
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

void OfaACorrCheckListBox::SetCheckButtonState( SvLBoxEntry* pEntry, USHORT nCol, SvButtonState eState)
{
	SvLBoxButton* pItem = (SvLBoxButton*)(pEntry->GetItem(nCol + 1));

	DBG_ASSERT(pItem,"SetCheckButton:Item not found");
	if (((SvLBoxItem*)pItem)->IsA() == SV_ITEM_ID_LBOXBUTTON)
	{
		switch( eState )
		{
			case SV_BUTTON_CHECKED:
				pItem->SetStateChecked();
				break;

			case SV_BUTTON_UNCHECKED:
				pItem->SetStateUnchecked();
				break;

			case SV_BUTTON_TRISTATE:
				pItem->SetStateTristate();
				break;
		}
		InvalidateEntry( pEntry );
	}
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

SvButtonState OfaACorrCheckListBox::GetCheckButtonState( SvLBoxEntry* pEntry, USHORT nCol ) const
{
	SvButtonState eState = SV_BUTTON_UNCHECKED;
	SvLBoxButton* pItem = (SvLBoxButton*)(pEntry->GetItem(nCol + 1));
	DBG_ASSERT(pItem,"GetChButnState:Item not found");

	if (((SvLBoxItem*)pItem)->IsA() == SV_ITEM_ID_LBOXBUTTON)
	{
		USHORT nButtonFlags = pItem->GetButtonFlags();
		eState = pCheckButtonData->ConvertToButtonState( nButtonFlags );
	}

	return eState;
}

void OfaACorrCheckListBox::HBarClick()
{
	// Sortierung durch diese Ueberladung abgeklemmt
}
/* -----------------------------22.05.2002 11:06------------------------------

 ---------------------------------------------------------------------------*/
void    OfaACorrCheckListBox::KeyInput( const KeyEvent& rKEvt )
{
    if(!rKEvt.GetKeyCode().GetModifier() &&
        KEY_SPACE == rKEvt.GetKeyCode().GetCode())
    {
        ULONG nSelPos = GetSelectEntryPos();
        USHORT nCol = GetCurrentTabPos() - 1;
        if ( nCol < 2 )
        {
            CheckEntryPos( nSelPos, nCol, !IsChecked( nSelPos, nCol ) );
            CallImplEventListeners( VCLEVENT_CHECKBOX_TOGGLE, (void*)GetEntry( nSelPos ) );
        }
        else
        {
            USHORT nCheck = IsChecked(nSelPos, 1) ? 1 : 0;
            if(IsChecked(nSelPos, 0))
                nCheck += 2;
            nCheck--;
            nCheck &= 3;
            CheckEntryPos(nSelPos, 1, 0 != (nCheck & 1));
            CheckEntryPos(nSelPos, 0, 0 != (nCheck & 2));
        }
    }
    else
        SvxSimpleTable::KeyInput(rKEvt);
}

/* -----------------20.11.98 13:48-------------------
 *
 * --------------------------------------------------*/

struct StringsArrays
{

	SvStringsDtor aAbbrevStrings;
	SvStringsDtor aDoubleCapsStrings;

	StringsArrays() :
		aAbbrevStrings(5,5), aDoubleCapsStrings(5,5) {}
};
typedef StringsArrays* StringsArraysPtr;
/* -----------------19.11.98 16:07-------------------
 *
 * --------------------------------------------------*/
BOOL lcl_FindInArray(SvStringsDtor& rStrings, const String& rString)
{
	for(USHORT i = 0; i < rStrings.Count(); i++)
		if(rString == *rStrings.GetObject(i))
			return TRUE;
	return FALSE;
}

void lcl_ClearTable(StringsTable& rTable)
{
	StringsArraysPtr pArrays = rTable.Last();
	while(pArrays)
	{
		delete pArrays;
		pArrays = rTable.Prev();
	}
	rTable.Clear();
}

/*-----------------14.10.96 15.57-------------------

--------------------------------------------------*/

OfaAutocorrExceptPage::OfaAutocorrExceptPage( Window* pParent,
												const SfxItemSet& rSet ) :
	SfxTabPage(pParent, SVX_RES( RID_OFAPAGE_AUTOCORR_EXCEPT ), rSet),
	aAbbrevFL    	(this, SVX_RES(FL_ABBREV		 )),
	aAbbrevED       (this, SVX_RES(ED_ABBREV		 )),
	aAbbrevLB    	(this, SVX_RES(LB_ABBREV		 )),
	aNewAbbrevPB    (this, SVX_RES(PB_NEWABBREV	 )),
	aDelAbbrevPB    (this, SVX_RES(PB_DELABBREV	 )),
	aAutoAbbrevCB   (this, SVX_RES(CB_AUTOABBREV	 )),
	aDoubleCapsFL   (this, SVX_RES(FL_DOUBLECAPS	 )),
	aDoubleCapsED   (this, SVX_RES(ED_DOUBLE_CAPS	 )),
	aDoubleCapsLB   (this, SVX_RES(LB_DOUBLE_CAPS	 )),
	aNewDoublePB    (this, SVX_RES(PB_NEWDOUBLECAPS)),
	aDelDoublePB 	(this, SVX_RES(PB_DELDOUBLECAPS)),
	aAutoCapsCB   	(this, SVX_RES(CB_AUTOCAPS	 )),
    eLang(eLastDialogLanguage)
{
	FreeResource();

	::com::sun::star::lang::Locale aLcl( SvxCreateLocale(eLastDialogLanguage ));
	pCompareClass = new CollatorWrapper( GetProcessFact() );
	pCompareClass->loadDefaultCollator( aLcl, ::com::sun::star::i18n::
							CollatorOptions::CollatorOptions_IGNORE_CASE );

	aNewAbbrevPB.SetClickHdl(LINK(this, OfaAutocorrExceptPage, NewDelHdl));
	aDelAbbrevPB.SetClickHdl(LINK(this, OfaAutocorrExceptPage, NewDelHdl));
	aNewDoublePB.SetClickHdl(LINK(this, OfaAutocorrExceptPage, NewDelHdl));
	aDelDoublePB.SetClickHdl(LINK(this, OfaAutocorrExceptPage, NewDelHdl));

	aAbbrevLB.SetSelectHdl(LINK(this, OfaAutocorrExceptPage, SelectHdl));
	aDoubleCapsLB.SetSelectHdl(LINK(this, OfaAutocorrExceptPage, SelectHdl));
	aAbbrevED.SetModifyHdl(LINK(this, OfaAutocorrExceptPage, ModifyHdl));
	aDoubleCapsED.SetModifyHdl(LINK(this, OfaAutocorrExceptPage, ModifyHdl));

	aAbbrevED.SetActionHdl(LINK(this, OfaAutocorrExceptPage, NewDelHdl));
	aDoubleCapsED.SetActionHdl(LINK(this, OfaAutocorrExceptPage, NewDelHdl));

}

/*-----------------14.10.96 15.58-------------------

--------------------------------------------------*/

OfaAutocorrExceptPage::~OfaAutocorrExceptPage()
{
	lcl_ClearTable(aStringsTable);
	delete pCompareClass;
}

/*-----------------14.10.96 15.58-------------------

--------------------------------------------------*/

SfxTabPage*	OfaAutocorrExceptPage::Create( Window* pParent,
								const SfxItemSet& rSet)
{
	return new OfaAutocorrExceptPage(pParent, rSet);
}
/* -----------------20.11.98 13:26-------------------
 *
 * --------------------------------------------------*/
void	OfaAutocorrExceptPage::ActivatePage( const SfxItemSet& )
{
	if(eLang != eLastDialogLanguage)
		SetLanguage(eLastDialogLanguage);
	((OfaAutoCorrDlg*)GetTabDialog())->EnableLanguage(TRUE);
}
/* -----------------20.11.98 13:26-------------------
 *
 * --------------------------------------------------*/
int     OfaAutocorrExceptPage::DeactivatePage( SfxItemSet* )
{
	return LEAVE_PAGE;
}
/*-----------------14.10.96 15.58-------------------

--------------------------------------------------*/

BOOL OfaAutocorrExceptPage::FillItemSet( SfxItemSet&  )
{
    SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
    StringsArraysPtr pArrays = aStringsTable.Last();
	while(pArrays)
	{
		LanguageType eCurLang = (LanguageType)aStringsTable.GetCurKey();
		if(eCurLang != eLang) // die aktuelle Sprache wird weiter hinten behandelt
		{
			SvStringsISortDtor* pWrdList = pAutoCorrect->LoadWrdSttExceptList(eCurLang);

			if(pWrdList)
			{
				USHORT nCount = pWrdList->Count();
				USHORT i;
				for( i = nCount; i; )
				{
					String* pString = pWrdList->GetObject( --i );
					//Eintrag finden u. gfs entfernen
					if( !lcl_FindInArray(pArrays->aDoubleCapsStrings, *pString))
					  pWrdList->DeleteAndDestroy( i );
				}
				nCount = pArrays->aDoubleCapsStrings.Count();
				for( i = 0; i < nCount; ++i )
				{
					String* pEntry = new String( *pArrays->aDoubleCapsStrings.GetObject( i ) );
					if( !pWrdList->Insert( pEntry ))
						delete pEntry;
				}
				pAutoCorrect->SaveWrdSttExceptList(eCurLang);
			}

			SvStringsISortDtor* pCplList = pAutoCorrect->LoadCplSttExceptList(eCurLang);

			if(pCplList)
			{
				USHORT nCount = pCplList->Count();
				USHORT i;
				for( i = nCount; i; )
				{
					String* pString = pCplList->GetObject( --i );
					if( !lcl_FindInArray(pArrays->aAbbrevStrings, *pString))
						pCplList->DeleteAndDestroy( i );
				}
				nCount = pArrays->aAbbrevStrings.Count();
				for( i = 0; i < nCount; ++i )
				{
					String* pEntry = new String( *pArrays->aAbbrevStrings.GetObject(i) );
					if( !pCplList->Insert( pEntry ))
						delete pEntry;
				}
				pAutoCorrect->SaveCplSttExceptList(eCurLang);
			}
		}
		pArrays = aStringsTable.Prev();
	}
	aStringsTable.Clear();

	SvStringsISortDtor* pWrdList = pAutoCorrect->LoadWrdSttExceptList(eLang);

	if(pWrdList)
	{
		USHORT nCount = pWrdList->Count();
		USHORT i;
		for( i = nCount; i; )
		{
			String* pString = pWrdList->GetObject( --i );
			if( USHRT_MAX == aDoubleCapsLB.GetEntryPos(*pString) )
				pWrdList->DeleteAndDestroy( i );
		}
		nCount = aDoubleCapsLB.GetEntryCount();
		for( i = 0; i < nCount; ++i )
		{
			String* pEntry = new String( aDoubleCapsLB.GetEntry( i ) );
			if( !pWrdList->Insert( pEntry ))
				delete pEntry;
		}
		pAutoCorrect->SaveWrdSttExceptList(eLang);
	}

	SvStringsISortDtor* pCplList = pAutoCorrect->LoadCplSttExceptList(eLang);

	if(pCplList)
	{
		USHORT nCount = pCplList->Count();
		USHORT i;
		for( i = nCount; i; )
		{
			String* pString = pCplList->GetObject( --i );
			if( USHRT_MAX == aAbbrevLB.GetEntryPos(*pString) )
				pCplList->DeleteAndDestroy( i );
		}
		nCount = aAbbrevLB.GetEntryCount();
		for( i = 0; i < nCount; ++i )
		{
			String* pEntry = new String( aAbbrevLB.GetEntry( i ) );
			if( !pCplList->Insert( pEntry ))
				delete pEntry;
		}
		pAutoCorrect->SaveCplSttExceptList(eLang);
	}
	if(aAutoAbbrevCB.IsChecked() != aAutoAbbrevCB.GetSavedValue())
		pAutoCorrect->SetAutoCorrFlag( SaveWordCplSttLst, aAutoAbbrevCB.IsChecked());
	if(aAutoCapsCB.IsChecked() != aAutoCapsCB.GetSavedValue())
		pAutoCorrect->SetAutoCorrFlag( SaveWordWrdSttLst, aAutoCapsCB.IsChecked());
	return FALSE;
}

/* -----------------23.11.98 10:33-------------------
 *
 * --------------------------------------------------*/
void OfaAutocorrExceptPage::SetLanguage(LanguageType eSet)
{
	if(eLang != eSet)
	{
		//alte Einstellungen speichern und neu fuellen
		RefillReplaceBoxes(FALSE, eLang, eSet);
		eLastDialogLanguage = eSet;
		delete pCompareClass;
		pCompareClass = new CollatorWrapper( GetProcessFact() );
		pCompareClass->loadDefaultCollator( SvxCreateLocale( eLastDialogLanguage ),
						::com::sun::star::i18n::
							CollatorOptions::CollatorOptions_IGNORE_CASE );
		ModifyHdl(&aAbbrevED);
		ModifyHdl(&aDoubleCapsED);
	}
}
/* -----------------20.11.98 14:06-------------------
 *
 * --------------------------------------------------*/
void OfaAutocorrExceptPage::RefillReplaceBoxes(BOOL bFromReset,
										LanguageType eOldLanguage,
										LanguageType eNewLanguage)
{
	eLang = eNewLanguage;
	if(bFromReset)
		lcl_ClearTable(aStringsTable);
	else
	{
		StringsArraysPtr pArrays = 0;
		if(aStringsTable.IsKeyValid(eOldLanguage))
		{
			pArrays = aStringsTable.Seek(ULONG(eOldLanguage));
			pArrays->aAbbrevStrings.DeleteAndDestroy(
									0, pArrays->aAbbrevStrings.Count());
			pArrays->aDoubleCapsStrings.DeleteAndDestroy(
									0, pArrays->aDoubleCapsStrings.Count());
		}
		else
		{
			pArrays = new StringsArrays;
			aStringsTable.Insert(ULONG(eOldLanguage), pArrays);
		}

		USHORT i;
		for(i = 0; i < aAbbrevLB.GetEntryCount(); i++)
		{
			pArrays->aAbbrevStrings.Insert(
				new String(aAbbrevLB.GetEntry(i)), i);

		}
		for(i = 0; i < aDoubleCapsLB.GetEntryCount(); i++)
		{
			pArrays->aDoubleCapsStrings.Insert(
				new String(aDoubleCapsLB.GetEntry(i)), i);
		}
	}
	aDoubleCapsLB.Clear();
	aAbbrevLB.Clear();
	String sTemp;
	aAbbrevED.SetText(sTemp);
	aDoubleCapsED.SetText(sTemp);

	if(aStringsTable.IsKeyValid(eLang))
	{
		StringsArraysPtr pArrays = aStringsTable.Seek(ULONG(eLang));
		USHORT i;
		for(i = 0; i < pArrays->aAbbrevStrings.Count(); i++ )
		{
			aAbbrevLB.InsertEntry(*pArrays->aAbbrevStrings.GetObject(i));
		}
		for( i = 0; i < pArrays->aDoubleCapsStrings.Count(); i++ )
		{
			aDoubleCapsLB.InsertEntry(*pArrays->aDoubleCapsStrings.GetObject(i));
		}
	}
	else
	{
        SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
        const SvStringsISortDtor* pCplList = pAutoCorrect->GetCplSttExceptList(eLang);
		const SvStringsISortDtor* pWrdList = pAutoCorrect->GetWrdSttExceptList(eLang);
		USHORT i;
		for( i = 0; i < pCplList->Count(); i++ )
		{
			aAbbrevLB.InsertEntry(*pCplList->GetObject(i));
		}
		for( i = 0; i < pWrdList->Count(); i++ )
		{
			aDoubleCapsLB.InsertEntry(*pWrdList->GetObject(i));
		}
	}
}

/*-----------------14.10.96 15.58-------------------

--------------------------------------------------*/

void OfaAutocorrExceptPage::Reset( const SfxItemSet& )
{
    SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
    RefillReplaceBoxes(TRUE, eLang, eLang);
	aAutoAbbrevCB.	Check(	pAutoCorrect->IsAutoCorrFlag( SaveWordCplSttLst ));
	aAutoCapsCB.	Check(	pAutoCorrect->IsAutoCorrFlag( SaveWordWrdSttLst ));
	aAutoAbbrevCB.SaveValue();
	aAutoCapsCB.SaveValue();
}

/*-----------------16.10.96 12.27-------------------

--------------------------------------------------*/


IMPL_LINK(OfaAutocorrExceptPage, NewDelHdl, PushButton*, pBtn)
{
	if((pBtn == &aNewAbbrevPB || pBtn == (PushButton*)&aAbbrevED )
		&& aAbbrevED.GetText().Len())
	{
		aAbbrevLB.InsertEntry(aAbbrevED.GetText());
		ModifyHdl(&aAbbrevED);
	}
	else if(pBtn == &aDelAbbrevPB)
	{
		aAbbrevLB.RemoveEntry(aAbbrevED.GetText());
		ModifyHdl(&aAbbrevED);
	}
	else if((pBtn == &aNewDoublePB || pBtn == (PushButton*)&aDoubleCapsED )
			&& aDoubleCapsED.GetText().Len())
	{
		aDoubleCapsLB.InsertEntry(aDoubleCapsED.GetText());
		ModifyHdl(&aDoubleCapsED);
	}
	else if(pBtn == &aDelDoublePB)
	{
		aDoubleCapsLB.RemoveEntry(aDoubleCapsED.GetText());
		ModifyHdl(&aDoubleCapsED);
	}
	return 0;
}

/*-----------------16.10.96 12.57-------------------

--------------------------------------------------*/

IMPL_LINK(OfaAutocorrExceptPage, SelectHdl, ListBox*, pBox)
{
	if(pBox == &aAbbrevLB)
	{
		aAbbrevED.SetText(pBox->GetSelectEntry());
		aNewAbbrevPB.Enable(FALSE);
		aDelAbbrevPB.Enable();
	}
	else
	{
		aDoubleCapsED.SetText(pBox->GetSelectEntry());
		aNewDoublePB.Enable(FALSE);
		aDelDoublePB.Enable();
	}
	return 0;
}

/*-----------------16.10.96 13.02-------------------

--------------------------------------------------*/

IMPL_LINK(OfaAutocorrExceptPage, ModifyHdl, Edit*, pEdt)
{
//	BOOL bSame = pEdt->GetText() == ->GetSelectEntry();
	const String& sEntry = pEdt->GetText();
	BOOL bEntryLen = 0!= sEntry.Len();
	if(pEdt == &aAbbrevED)
	{
		BOOL bSame = lcl_FindEntry(aAbbrevLB, sEntry, *pCompareClass);
		if(bSame && sEntry != aAbbrevLB.GetSelectEntry())
			pEdt->SetText(aAbbrevLB.GetSelectEntry());
		aNewAbbrevPB.Enable(!bSame && bEntryLen);
		aDelAbbrevPB.Enable(bSame && bEntryLen);
	}
	else
	{
		BOOL bSame = lcl_FindEntry(aDoubleCapsLB, sEntry, *pCompareClass);
		if(bSame && sEntry != aDoubleCapsLB.GetSelectEntry())
			pEdt->SetText(aDoubleCapsLB.GetSelectEntry());
		aNewDoublePB.Enable(!bSame && bEntryLen);
		aDelDoublePB.Enable(bSame && bEntryLen);
	}
	return 0;
}

/*-----------------16.10.96 15.03-------------------

--------------------------------------------------*/

void AutoCorrEdit::KeyInput( const KeyEvent& rKEvt )
{
	const KeyCode aKeyCode = rKEvt.GetKeyCode();
	const USHORT nModifier = aKeyCode.GetModifier();
	if( aKeyCode.GetCode() == KEY_RETURN )
	{
		//wird bei Enter nichts getan, dann doch die Basisklasse rufen
		// um den Dialog zu schliessen
		if(!nModifier && !aActionLink.Call(this))
				 Edit::KeyInput(rKEvt);
	}
	else if(bSpaces || aKeyCode.GetCode() != KEY_SPACE)
		Edit::KeyInput(rKEvt);
}

/*-----------------03.07.97 13:17-------------------

--------------------------------------------------*/

OfaQuoteTabPage::OfaQuoteTabPage( Window* pParent, const SfxItemSet& rSet ) :
	SfxTabPage(pParent, SVX_RES( RID_OFAPAGE_AUTOCORR_QUOTE ), rSet),
	aSingleFL           (this, SVX_RES(FL_SINGLE		 )),
    aSingleTypoCB       (this, SVX_RES(CB_SGL_TYPO     )),
    aSglStartQuoteFT    (this, SVX_RES(FT_SGL_STARTQUOTE )),
    aSglStartQuotePB    (this, SVX_RES(PB_SGL_STARTQUOTE )),
    aSglStartExFT       (this, SVX_RES(FT_SGSTEX       )),
    aSglEndQuoteFT      (this, SVX_RES(FT_SGL_ENDQUOTE   )),
    aSglEndQuotePB      (this, SVX_RES(PB_SGL_ENDQUOTE   )),
    aSglEndExFT         (this, SVX_RES(FT_SGENEX       )),
    aSglStandardPB      (this, SVX_RES(PB_SGL_STD      )),

    aDoubleFL           (this, SVX_RES(FL_DOUBLE       )),
    aTypoCB             (this, SVX_RES(CB_TYPO         )),
    aStartQuoteFT       (this, SVX_RES(FT_STARTQUOTE   )),
    aStartQuotePB       (this, SVX_RES(PB_STARTQUOTE   )),
    aDblStartExFT       (this, SVX_RES(FT_DBSTEX       )),
    aEndQuoteFT         (this, SVX_RES(FT_ENDQUOTE     )),
	aEndQuotePB    		(this, SVX_RES(PB_ENDQUOTE     )),
    aDblEndExFT         (this, SVX_RES(FT_DBECEX       )),
    aDblStandardPB      (this, SVX_RES(PB_DBL_STD      )),

    sStartQuoteDlg  (SVX_RES(STR_CHANGE_START)),
    sEndQuoteDlg    (SVX_RES(STR_CHANGE_END)),

    sStandard(SVX_RES(ST_STANDARD))
{
	FreeResource();

	aStartQuotePB.SetClickHdl(LINK(this, 	OfaQuoteTabPage, QuoteHdl));
	aEndQuotePB.SetClickHdl(LINK(this, 		OfaQuoteTabPage, QuoteHdl));
	aSglStartQuotePB.SetClickHdl(LINK(this, OfaQuoteTabPage, QuoteHdl));
	aSglEndQuotePB.SetClickHdl(LINK(this, 	OfaQuoteTabPage, QuoteHdl));
	aDblStandardPB.SetClickHdl(LINK(this, 	OfaQuoteTabPage, StdQuoteHdl));
	aSglStandardPB.SetClickHdl(LINK(this, 	OfaQuoteTabPage, StdQuoteHdl));

}
/*-----------------03.07.97 13:17-------------------

--------------------------------------------------*/
OfaQuoteTabPage::~OfaQuoteTabPage()
{
}
/*-----------------03.07.97 13:17-------------------

--------------------------------------------------*/
SfxTabPage*	OfaQuoteTabPage::Create( Window* pParent,
								const SfxItemSet& rAttrSet)
{
	return new OfaQuoteTabPage(pParent, rAttrSet);
}
/*-----------------03.07.97 13:18-------------------

--------------------------------------------------*/
BOOL OfaQuoteTabPage::FillItemSet( SfxItemSet&  )
{
	SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();

	long nFlags = pAutoCorrect->GetFlags();
	pAutoCorrect->SetAutoCorrFlag(ChgQuotes, 		aTypoCB.IsChecked());
	pAutoCorrect->SetAutoCorrFlag(ChgSglQuotes, 	aSingleTypoCB.IsChecked());
	BOOL bReturn = nFlags != pAutoCorrect->GetFlags();
	if(cStartQuote != pAutoCorrect->GetStartDoubleQuote())
	{
		bReturn = TRUE;
		sal_Unicode cUCS2 = static_cast<sal_Unicode>(cStartQuote); //TODO
		pAutoCorrect->SetStartDoubleQuote(cUCS2);
	}
	if(cEndQuote != pAutoCorrect->GetEndDoubleQuote())
	{
		bReturn = TRUE;
		sal_Unicode cUCS2 = static_cast<sal_Unicode>(cEndQuote); //TODO
		pAutoCorrect->SetEndDoubleQuote(cUCS2);
	}
	if(cSglStartQuote != pAutoCorrect->GetStartSingleQuote())
	{
		bReturn = TRUE;
		sal_Unicode cUCS2 = static_cast<sal_Unicode>(cSglStartQuote); //TODO
		pAutoCorrect->SetStartSingleQuote(cUCS2);
	}
	if(cSglEndQuote != pAutoCorrect->GetEndSingleQuote())
	{
		bReturn = TRUE;
		sal_Unicode cUCS2 = static_cast<sal_Unicode>(cSglEndQuote); //TODO
		pAutoCorrect->SetEndSingleQuote(cUCS2);
	}

	if(bReturn )
    {
        SvxAutoCorrCfg* pCfg = SvxAutoCorrCfg::Get();
        pCfg->SetModified();
        pCfg->Commit();
    }
	return bReturn;
}
/* -----------------23.11.98 16:15-------------------
 *
 * --------------------------------------------------*/
void OfaQuoteTabPage::ActivatePage( const SfxItemSet& )
{
	((OfaAutoCorrDlg*)GetTabDialog())->EnableLanguage(FALSE);
}
/*-----------------03.07.97 13:18-------------------

--------------------------------------------------*/
void OfaQuoteTabPage::Reset( const SfxItemSet& )
{
	SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
	const long nFlags = pAutoCorrect->GetFlags();

	aTypoCB    			.Check(0 != (nFlags & ChgQuotes));
	aSingleTypoCB    	.Check(0 != (nFlags & ChgSglQuotes));
	aTypoCB    			.SaveValue();
	aSingleTypoCB    	.SaveValue();

	cStartQuote = pAutoCorrect->GetStartDoubleQuote();
	cEndQuote = pAutoCorrect->GetEndDoubleQuote();
	cSglStartQuote = pAutoCorrect->GetStartSingleQuote();
	cSglEndQuote = pAutoCorrect->GetEndSingleQuote();

	aSglStartExFT .SetText(ChangeStringExt_Impl(cSglStartQuote));
	aSglEndExFT   .SetText(ChangeStringExt_Impl(cSglEndQuote));
	aDblStartExFT .SetText(ChangeStringExt_Impl(cStartQuote));
	aDblEndExFT   .SetText(ChangeStringExt_Impl(cEndQuote));
}


/*-----------------15.10.96 16.42-------------------

--------------------------------------------------*/
#define SGL_START		0
#define DBL_START       1
#define SGL_END         2
#define DBL_END         3


IMPL_LINK( OfaQuoteTabPage, QuoteHdl, PushButton*, pBtn )
{
	USHORT nMode = SGL_START;
	if(pBtn == &aSglEndQuotePB)
		nMode = SGL_END;
	else if(pBtn == &aStartQuotePB)
		nMode = DBL_START;
	else if(pBtn == &aEndQuotePB)
		nMode = DBL_END;
	// Zeichenauswahl-Dialog starten
	SvxCharacterMap* pMap = new SvxCharacterMap( this, TRUE );
	pMap->SetCharFont( OutputDevice::GetDefaultFont(DEFAULTFONT_LATIN_TEXT,
						LANGUAGE_ENGLISH_US, DEFAULTFONT_FLAGS_ONLYONE, 0 ));
	pMap->SetText(nMode < SGL_END ? sStartQuoteDlg  :  sEndQuoteDlg );
	sal_UCS4 cDlg;
	//The two lines below are added by BerryJia for Bug95846 Time:2002-8-13 15:50
	SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
	LanguageType eLang = Application::GetSettings().GetLanguage();
	switch( nMode )
	{
		case SGL_START:
			cDlg = cSglStartQuote;
			if(cDlg == 0)
				cDlg = pAutoCorrect->GetQuote('\'',TRUE,eLang);   //add by BerryJia for Bug95846 Time:2002-8-13 15:50
		break;
		case SGL_END:
			cDlg = cSglEndQuote;
			if(cDlg == 0)
				cDlg = pAutoCorrect->GetQuote('\'',FALSE,eLang);  //add by BerryJia for Bug95846 Time:2002-8-13 15:50
		break;
		case DBL_START:
			cDlg = cStartQuote;
			if(cDlg == 0)
				cDlg = pAutoCorrect->GetQuote('\"',TRUE,eLang);   //add by BerryJia for Bug95846 Time:2002-8-13 15:50
		break;
		case DBL_END:
			cDlg = cEndQuote;
			if(cDlg == 0)
				cDlg = pAutoCorrect->GetQuote('\"',FALSE,eLang);  //add by BerryJia for Bug95846 Time:2002-8-13 15:50
		break;
		default:
			DBG_ERROR("svx::OfaQuoteTabPage::QuoteHdl(), how to initialize cDlg?" );
			cDlg = 0;
			break;

	}
	pMap->SetChar(  cDlg );
	pMap->DisableFontSelection();
	if(pMap->Execute() == RET_OK)
	{
		sal_UCS4 cNewChar = pMap->GetChar();
		switch( nMode )
		{
			case SGL_START:
				cSglStartQuote = cNewChar;
				aSglStartExFT.SetText(ChangeStringExt_Impl(cNewChar));
			break;
			case SGL_END:
				cSglEndQuote = cNewChar;
				aSglEndExFT.SetText(ChangeStringExt_Impl(cNewChar));
			break;
			case DBL_START:
				cStartQuote = cNewChar;
				aDblStartExFT.SetText(ChangeStringExt_Impl(cNewChar));
			break;
			case DBL_END:
				cEndQuote = cNewChar;
				aDblEndExFT.SetText(ChangeStringExt_Impl(cNewChar));
			break;
		}
	}
	delete pMap;

	return 0;
}

/*-----------------27.06.97 09.55-------------------

--------------------------------------------------*/
IMPL_LINK( OfaQuoteTabPage, StdQuoteHdl, PushButton*, pBtn )
{
	if(pBtn == &aDblStandardPB)
	{
		cStartQuote = 0;
		aDblStartExFT.SetText(ChangeStringExt_Impl(0));
		cEndQuote = 0;
		aDblEndExFT.SetText(ChangeStringExt_Impl(0));

	}
	else
	{
		cSglStartQuote = 0;
		aSglStartExFT.SetText(ChangeStringExt_Impl(0));
		cSglEndQuote = 0;
		aSglEndExFT.SetText(ChangeStringExt_Impl(0));
	}
	return 0;
}

// --------------------------------------------------

String OfaQuoteTabPage::ChangeStringExt_Impl( sal_UCS4 cChar )
{
	if( !cChar )
		return sStandard;

    // convert codepoint value to unicode-hex string
	sal_UCS4 aStrCodes[32] = { 0, ' ', '(', 'U', '+', '0' };
	aStrCodes[0] = cChar;
	int nFullLen = 5;
    int nHexLen = 4;
    while( (cChar >> (4*nHexLen)) != 0 )
        ++nHexLen;
    for( int i = nHexLen; --i >= 0;)
    {
        sal_UCS4 cHexDigit = ((cChar >> (4*i)) & 0x0f) + '0';
		if( cHexDigit > '9' )
			cHexDigit += 'A' - ('9' + 1);
		aStrCodes[ nFullLen++ ] = cHexDigit;
    }
	aStrCodes[ nFullLen++ ] = ')';
	// using the new UCS4 constructor
	rtl::OUString aOUStr( aStrCodes, nFullLen );
	return aOUStr;
}

OfaAutoCompleteTabPage::OfaAutoCompleteTabPage( Window* pParent,
												const SfxItemSet& rSet )
	: SfxTabPage(pParent, SVX_RES( RID_OFAPAGE_AUTOCOMPLETE_OPTIONS ), rSet),
	aCBActiv		(this, SVX_RES(CB_ACTIV)),
    aCBAppendSpace  (this, SVX_RES(CB_APPEND_SPACE)),
    aCBAsTip        (this, SVX_RES(CB_AS_TIP)),
    aCBCollect      (this, SVX_RES(CB_COLLECT)),
    aCBRemoveList   (this, SVX_RES(CB_REMOVE_LIST)),
    aFTExpandKey    (this, SVX_RES(FT_EXPAND_KEY)),
    aDCBExpandKey   (this, SVX_RES(DCB_EXPAND_KEY)),
    aFTMinWordlen   (this, SVX_RES(FT_MIN_WORDLEN)),
    aNFMinWordlen   (this, SVX_RES(NF_MIN_WORDLEN)),
    aFTMaxEntries   (this, SVX_RES(FT_MAX_ENTRIES)),
    aNFMaxEntries   (this, SVX_RES(NF_MAX_ENTRIES)),
    aLBEntries      (*this, SVX_RES(LB_ENTRIES)),
	aPBEntries      (this, SVX_RES(PB_ENTRIES)),
	pAutoCmpltList( 0 ),
	nAutoCmpltListCnt( 0 )
{
	FreeResource();

	// the defined KEYs
	static const USHORT aKeyCodes[] = {
		KEY_END,
		KEY_RETURN,
		KEY_SPACE,
		KEY_RIGHT,
        KEY_TAB,
		0
	};

	for( const USHORT* pKeys = aKeyCodes; *pKeys; ++pKeys )
	{
		KeyCode aKCode( *pKeys );
		USHORT nPos = aDCBExpandKey.InsertEntry( aKCode.GetName() );
		aDCBExpandKey.SetEntryData( nPos, (void*)(ULONG)*pKeys );
		if( KEY_RETURN == *pKeys )		// default to RETURN
			aDCBExpandKey.SelectEntryPos( nPos );
	}

	aPBEntries.SetClickHdl(LINK(this, OfaAutoCompleteTabPage, DeleteHdl));
	aCBActiv.SetToggleHdl(LINK(this, OfaAutoCompleteTabPage, CheckHdl));
    aCBCollect.SetToggleHdl(LINK(this, OfaAutoCompleteTabPage, CheckHdl));
}

OfaAutoCompleteTabPage::~OfaAutoCompleteTabPage()
{
}

SfxTabPage*	OfaAutoCompleteTabPage::Create( Window* pParent,
											const SfxItemSet& rSet)
{
	return new OfaAutoCompleteTabPage( pParent, rSet );
}

BOOL OfaAutoCompleteTabPage::FillItemSet( SfxItemSet& )
{
	BOOL bModified = FALSE, bCheck;
	SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
	SvxSwAutoFmtFlags *pOpt = &pAutoCorrect->GetSwFlags();
	USHORT nVal;

	bCheck = aCBActiv.IsChecked();
	bModified |= pOpt->bAutoCompleteWords != bCheck;
	pOpt->bAutoCompleteWords = bCheck;
	bCheck = aCBCollect.IsChecked();
	bModified |= pOpt->bAutoCmpltCollectWords != bCheck;
	pOpt->bAutoCmpltCollectWords = bCheck;
    bCheck = !aCBRemoveList.IsChecked(); // inverted value!
    bModified |= pOpt->bAutoCmpltKeepList != bCheck; 
    pOpt->bAutoCmpltKeepList = bCheck;
    bCheck = aCBAppendSpace.IsChecked();
	bModified |= pOpt->bAutoCmpltAppendBlanc != bCheck;
	pOpt->bAutoCmpltAppendBlanc = bCheck;
	bCheck = aCBAsTip.IsChecked();
	bModified |= pOpt->bAutoCmpltShowAsTip != bCheck;
	pOpt->bAutoCmpltShowAsTip = bCheck;

	nVal = (USHORT)aNFMinWordlen.GetValue();
	bModified |= nVal != pOpt->nAutoCmpltWordLen;
	pOpt->nAutoCmpltWordLen = nVal;

	nVal = (USHORT)aNFMaxEntries.GetValue();
	bModified |= nVal != pOpt->nAutoCmpltListLen;
	pOpt->nAutoCmpltListLen = nVal;

	nVal = aDCBExpandKey.GetSelectEntryPos();
	if( nVal < aDCBExpandKey.GetEntryCount() )
	{
		ULONG nKey = (ULONG)aDCBExpandKey.GetEntryData( nVal );
		bModified |= nKey != pOpt->nAutoCmpltExpandKey;
		pOpt->nAutoCmpltExpandKey = (USHORT)nKey;
   }

	if( pAutoCmpltList && nAutoCmpltListCnt != aLBEntries.GetEntryCount() )
	{
		bModified = TRUE;
		pOpt->pAutoCmpltList = pAutoCmpltList;
	}
	if( bModified )
    {
        SvxAutoCorrCfg* pCfg = SvxAutoCorrCfg::Get();
        pCfg->SetModified();
        pCfg->Commit();
    }
	return TRUE;
}

void OfaAutoCompleteTabPage::Reset( const SfxItemSet&  )
{
	SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
	SvxSwAutoFmtFlags *pOpt = &pAutoCorrect->GetSwFlags();

	aCBActiv.Check( 0 != pOpt->bAutoCompleteWords );
	aCBCollect.Check( 0 != pOpt->bAutoCmpltCollectWords );
    aCBRemoveList.Check( !pOpt->bAutoCmpltKeepList ); //inverted value!
    aCBAppendSpace.Check( 0 != pOpt->bAutoCmpltAppendBlanc );
	aCBAsTip.Check( 0 != pOpt->bAutoCmpltShowAsTip );

	aNFMinWordlen.SetValue( pOpt->nAutoCmpltWordLen );
	aNFMaxEntries.SetValue( pOpt->nAutoCmpltListLen );

	// select the specific KeyCode:
	{
		ULONG nKey = pOpt->nAutoCmpltExpandKey;
		for( USHORT n = 0, nCnt = aDCBExpandKey.GetEntryCount(); n < nCnt; ++n )
			if( nKey == (ULONG)aDCBExpandKey.GetEntryData( n ))
			{
				aDCBExpandKey.SelectEntryPos( n );
				break;
			}
	}

	if( pOpt->pAutoCmpltList && pOpt->pAutoCmpltList->Count() )
	{
		pAutoCmpltList = (SvStringsISortDtor*)pOpt->pAutoCmpltList;
		pOpt->pAutoCmpltList = 0;
		nAutoCmpltListCnt = pAutoCmpltList->Count();
		for( USHORT n = 0; n < nAutoCmpltListCnt; ++n )
		{
			const StringPtr pStr = pAutoCmpltList->GetObject( n );
			USHORT nPos = aLBEntries.InsertEntry( *pStr );
			aLBEntries.SetEntryData( nPos, (void*)pStr );
		}
	}
	else
	{
		aLBEntries.Disable();
		aPBEntries.Disable();
	}

	CheckHdl( &aCBActiv );
    CheckHdl( &aCBCollect );
}

void OfaAutoCompleteTabPage::ActivatePage( const SfxItemSet& )
{
	((OfaAutoCorrDlg*)GetTabDialog())->EnableLanguage( FALSE );
}

IMPL_LINK( OfaAutoCompleteTabPage, DeleteHdl, PushButton*, EMPTYARG )
{
	USHORT nSelCnt = pAutoCmpltList ? aLBEntries.GetSelectEntryCount() : 0;
	while( nSelCnt )
	{
		USHORT nPos = aLBEntries.GetSelectEntryPos( --nSelCnt );
		const StringPtr pStr = (StringPtr)aLBEntries.GetEntryData( nPos );
		aLBEntries.RemoveEntry( nPos );
		nPos = pAutoCmpltList->GetPos( pStr );
		if( USHRT_MAX != nPos )
			pAutoCmpltList->Remove( nPos );
	}
	return 0;
}

IMPL_LINK( OfaAutoCompleteTabPage, CheckHdl, CheckBox*, pBox )
{
    BOOL bEnable = pBox->IsChecked();
    if( pBox == &aCBActiv )
	{
        aCBAppendSpace.Enable( bEnable );
        aCBAppendSpace.Enable( bEnable );
		aCBAsTip.Enable( bEnable );
		aDCBExpandKey.Enable( bEnable );
	}
    else if(&aCBCollect == pBox)
        aCBRemoveList.Enable( bEnable );
    return 0;
}

void OfaAutoCompleteTabPage::CopyToClipboard() const
{
	USHORT nSelCnt = aLBEntries.GetSelectEntryCount();
	if( pAutoCmpltList && nSelCnt )
	{
		TransferDataContainer* pCntnr = new TransferDataContainer;
		::com::sun::star::uno::Reference<
			::com::sun::star::datatransfer::XTransferable > xRef( pCntnr );

		ByteString sData;
		const sal_Char* pLineEnd =
#if defined(UNX)
				"\012";
#else
				"\015\012";
#endif

		rtl_TextEncoding nEncode = gsl_getSystemTextEncoding();

		for( USHORT n = 0; n < nSelCnt; ++n )
		{
			sData += ByteString( aLBEntries.GetSelectEntry( n ), nEncode );
			sData += pLineEnd;
		}
		pCntnr->CopyByteString( SOT_FORMAT_STRING, sData );
		pCntnr->CopyToClipboard( (Window*)this );
	}
}

long OfaAutoCompleteTabPage::AutoCompleteMultiListBox::PreNotify(
			NotifyEvent& rNEvt )
{
	long nHandled = MultiListBox::PreNotify( rNEvt );

	if( !nHandled && EVENT_KEYUP == rNEvt.GetType() )
	{
		const KeyCode& rKeyCode = rNEvt.GetKeyEvent()->GetKeyCode();
		switch( rKeyCode.GetModifier() | rKeyCode.GetCode() )
		{
		case KEY_DELETE:
			rPage.DeleteHdl( 0 );
			nHandled = 1;
			break;

		default:
			if( KEYFUNC_COPY == rKeyCode.GetFunction() )
			{
				rPage.CopyToClipboard();
				nHandled = 1;
			}
			break;
		}
	}
	return nHandled;
}

// class OfaSmartTagOptionsTabPage ---------------------------------------------

OfaSmartTagOptionsTabPage::OfaSmartTagOptionsTabPage( Window* pParent,
												      const SfxItemSet& rSet )
    : SfxTabPage(pParent, SVX_RES( RID_OFAPAGE_SMARTTAG_OPTIONS ), rSet),
    m_aMainCB( this, SVX_RES(CB_SMARTTAGS) ),
    m_aSmartTagTypesLB( this, SVX_RES(LB_SMARTTAGS) ),
    m_aPropertiesPB( this, SVX_RES(PB_SMARTTAGS) ),
    m_aTitleFT( this, SVX_RES(FT_SMARTTAGS) )
{
	FreeResource();

    // some options for the list box:
    m_aSmartTagTypesLB.SetWindowBits( m_aSmartTagTypesLB.GetStyle() | WB_HSCROLL | WB_HIDESELECTION );
	m_aSmartTagTypesLB.SetHighlightRange();

    // set the handlers:
	m_aMainCB.SetToggleHdl(LINK(this, OfaSmartTagOptionsTabPage, CheckHdl));
    m_aPropertiesPB.SetClickHdl(LINK(this, OfaSmartTagOptionsTabPage, ClickHdl));
    m_aSmartTagTypesLB.SetSelectHdl(LINK(this, OfaSmartTagOptionsTabPage, SelectHdl));
}

OfaSmartTagOptionsTabPage::~OfaSmartTagOptionsTabPage()
{

}

SfxTabPage*	OfaSmartTagOptionsTabPage::Create( Window* pParent, const SfxItemSet& rSet)
{
	return new OfaSmartTagOptionsTabPage( pParent, rSet );
}

/** This struct is used to associate list box entries with smart tag data
*/
struct ImplSmartTagLBUserData
{
    rtl::OUString maSmartTagType;
    uno::Reference< smarttags::XSmartTagRecognizer > mxRec;
    sal_Int32 mnSmartTagIdx;

    ImplSmartTagLBUserData( const rtl::OUString& rSmartTagType,
                            uno::Reference< smarttags::XSmartTagRecognizer > xRec,
                            sal_Int32 nSmartTagIdx ) :
        maSmartTagType( rSmartTagType ),
        mxRec( xRec ),
        mnSmartTagIdx( nSmartTagIdx ) {}
};

/** Clears m_aSmartTagTypesLB
*/
void OfaSmartTagOptionsTabPage::ClearListBox()
{
    const ULONG nCount = m_aSmartTagTypesLB.GetEntryCount();
    for ( USHORT i = 0; i < nCount; ++i )
	{
        const SvLBoxEntry* pEntry = m_aSmartTagTypesLB.GetEntry(i);
        const ImplSmartTagLBUserData* pUserData = static_cast< ImplSmartTagLBUserData* >(pEntry->GetUserData());
        delete pUserData;
    }

    m_aSmartTagTypesLB.Clear();
}

/** Inserts items into m_aSmartTagTypesLB
*/
void OfaSmartTagOptionsTabPage::FillListBox( const SmartTagMgr& rSmartTagMgr )
{
    // first we have to clear the list box:
    ClearListBox();

    // fill list box:
    const sal_uInt32 nNumberOfRecognizers = rSmartTagMgr.NumberOfRecognizers();
	const lang::Locale aLocale( SvxCreateLocale( eLastDialogLanguage ) );
    std::vector< ActionReference > aActionReferences;

    for ( sal_uInt32 i = 0; i < nNumberOfRecognizers; ++i )
    {
        uno::Reference< smarttags::XSmartTagRecognizer > xRec = rSmartTagMgr.GetRecognizer(i);

        const rtl::OUString aName = xRec->getName( aLocale );
        const rtl::OUString aDesc = xRec->getDescription( aLocale );
        const sal_Int32 nNumberOfSupportedSmartTags = xRec->getSmartTagCount();

        for ( sal_Int32 j = 0; j < nNumberOfSupportedSmartTags; ++j )
        {
            const rtl::OUString aSmartTagType = xRec->getSmartTagName(j);
            rtl::OUString aSmartTagCaption = rSmartTagMgr.GetSmartTagCaption( aSmartTagType, aLocale );

            if ( !aSmartTagCaption.getLength() )
                aSmartTagCaption = aSmartTagType;

            const rtl::OUString aLBEntry = aSmartTagCaption +
                                           OUString::createFromAscii(" (") +
                                           aName +
                                           OUString::createFromAscii(")");

            SvLBoxEntry* pEntry = m_aSmartTagTypesLB.SvTreeListBox::InsertEntry( aLBEntry );
            if ( pEntry )
            {
                const bool bCheck = rSmartTagMgr.IsSmartTagTypeEnabled( aSmartTagType );
                m_aSmartTagTypesLB.SetCheckButtonState( pEntry, bCheck ? SV_BUTTON_CHECKED : SV_BUTTON_UNCHECKED );
                pEntry->SetUserData(static_cast<void*>(new ImplSmartTagLBUserData( aSmartTagType, xRec, j ) ) );
            }
        }
    }
}

/** Handler for the push button
*/
IMPL_LINK( OfaSmartTagOptionsTabPage, ClickHdl, PushButton*, EMPTYARG )
{
	const USHORT nPos = m_aSmartTagTypesLB.GetSelectEntryPos();
    const SvLBoxEntry* pEntry = m_aSmartTagTypesLB.GetEntry(nPos);
    const ImplSmartTagLBUserData* pUserData = static_cast< ImplSmartTagLBUserData* >(pEntry->GetUserData());
    uno::Reference< smarttags::XSmartTagRecognizer > xRec = pUserData->mxRec;
    const sal_Int32 nSmartTagIdx = pUserData->mnSmartTagIdx;

 	const lang::Locale aLocale( SvxCreateLocale( eLastDialogLanguage ) );
    if ( xRec->hasPropertyPage( nSmartTagIdx, aLocale ) )
        xRec->displayPropertyPage( nSmartTagIdx, aLocale );

	return 0;
}

/** Handler for the check box
*/
IMPL_LINK( OfaSmartTagOptionsTabPage, CheckHdl, CheckBox*, EMPTYARG )
{
    const BOOL bEnable = m_aMainCB.IsChecked();
    m_aSmartTagTypesLB.Enable( bEnable );
    m_aSmartTagTypesLB.Invalidate();
    m_aPropertiesPB.Enable( false );

    // if the controls are currently enabled, we still have to check
    // if the properties button should be disabled because the currently
    // seleted smart tag type does not have a properties dialog.
    // We do this by calling SelectHdl:
    if ( bEnable )
        SelectHdl( &m_aSmartTagTypesLB );

    return 0;
}

/** Handler for the list box
*/
IMPL_LINK(OfaSmartTagOptionsTabPage, SelectHdl, SvxCheckListBox*, EMPTYARG)
{
    if ( m_aSmartTagTypesLB.GetEntryCount() < 1 )
        return 0;

	const USHORT nPos = m_aSmartTagTypesLB.GetSelectEntryPos();
    const SvLBoxEntry* pEntry = m_aSmartTagTypesLB.GetEntry(nPos);
    const ImplSmartTagLBUserData* pUserData = static_cast< ImplSmartTagLBUserData* >(pEntry->GetUserData());
    uno::Reference< smarttags::XSmartTagRecognizer > xRec = pUserData->mxRec;
    const sal_Int32 nSmartTagIdx = pUserData->mnSmartTagIdx;

    const lang::Locale aLocale( SvxCreateLocale( eLastDialogLanguage ) );
    if ( xRec->hasPropertyPage( nSmartTagIdx, aLocale ) )
        m_aPropertiesPB.Enable( sal_True );
    else
        m_aPropertiesPB.Enable( sal_False );

	return 0;
}

/** Propagates the current settings to the smart tag manager.
*/
BOOL OfaSmartTagOptionsTabPage::FillItemSet( SfxItemSet& )
{
	SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
	SvxSwAutoFmtFlags *pOpt = &pAutoCorrect->GetSwFlags();
    SmartTagMgr* pSmartTagMgr = pOpt->pSmartTagMgr;

    // robust!
    if ( !pSmartTagMgr )
        return FALSE;

    BOOL bModifiedSmartTagTypes = FALSE;
    std::vector< rtl::OUString > aDisabledSmartTagTypes;

    const ULONG nCount = m_aSmartTagTypesLB.GetEntryCount();

    for ( USHORT i = 0; i < nCount; ++i )
	{
        const SvLBoxEntry* pEntry = m_aSmartTagTypesLB.GetEntry(i);
        const ImplSmartTagLBUserData* pUserData = static_cast< ImplSmartTagLBUserData* >(pEntry->GetUserData());
		const BOOL bChecked = m_aSmartTagTypesLB.IsChecked(i);
        const BOOL bIsCurrentlyEnabled = pSmartTagMgr->IsSmartTagTypeEnabled( pUserData->maSmartTagType );

        bModifiedSmartTagTypes = bModifiedSmartTagTypes || ( !bChecked != !bIsCurrentlyEnabled );

        if ( !bChecked )
            aDisabledSmartTagTypes.push_back( pUserData->maSmartTagType );

        delete pUserData;
    }

    const BOOL bModifiedRecognize = ( !m_aMainCB.IsChecked() != !pSmartTagMgr->IsLabelTextWithSmartTags() );
    if ( bModifiedSmartTagTypes || bModifiedRecognize )
    {
        bool bLabelTextWithSmartTags = m_aMainCB.IsChecked() ? true : false;
        pSmartTagMgr->WriteConfiguration( bModifiedRecognize     ? &bLabelTextWithSmartTags : 0,
                                          bModifiedSmartTagTypes ? &aDisabledSmartTagTypes : 0 );
    }

	return TRUE;
}

/** Sets the controls based on the current settings at SmartTagMgr.
*/
void OfaSmartTagOptionsTabPage::Reset( const SfxItemSet&  )
{
	SvxAutoCorrect* pAutoCorrect = SvxAutoCorrCfg::Get()->GetAutoCorrect();
	SvxSwAutoFmtFlags *pOpt = &pAutoCorrect->GetSwFlags();
    const SmartTagMgr* pSmartTagMgr = pOpt->pSmartTagMgr;

    // robust, should not happen!
    if ( !pSmartTagMgr )
        return;

    FillListBox( *pSmartTagMgr );
	m_aSmartTagTypesLB.SelectEntryPos( 0 );
    m_aMainCB.Check( pSmartTagMgr->IsLabelTextWithSmartTags() );
    CheckHdl( &m_aMainCB );
}

void OfaSmartTagOptionsTabPage::ActivatePage( const SfxItemSet& )
{
	((OfaAutoCorrDlg*)GetTabDialog())->EnableLanguage( FALSE );
}

