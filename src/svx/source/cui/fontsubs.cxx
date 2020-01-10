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

#ifdef PRECOMPILED
#include "svxpch.hxx"
#endif
#include <tools/shl.hxx>
#include <svtools/ctrltool.hxx>
#include <vcl/svapp.hxx>
#include <vcl/wrkwin.hxx>
#ifndef _OFA_FONTSUBSTCONFIG_HXX
#include <svtools/fontsubstconfig.hxx>
#endif
#include <svtools/sourceviewconfig.hxx>

#include "fontsubs.hrc"
#include "fontsubs.hxx"
#include <svx/dialmgr.hxx>
#include "helpid.hrc"
#include <svx/dialogs.hrc>

/*********************************************************************/
/*                                                                   */
/* 	TabPage Fontersetzung                                            */
/*                                                                   */
/*********************************************************************/

#define CBCOL_FIRST		0
#define CBCOL_SECOND	1
#define CBCOL_BOTH		2

SvxFontSubstTabPage::SvxFontSubstTabPage( Window* pParent,
								const SfxItemSet& rSet ) :
	SfxTabPage(pParent, SVX_RES(RID_SVX_FONT_SUBSTITUTION), rSet),
	aUseTableCB 		(this,	SVX_RES(CB_USETABLE)),
    aFont1FT            (this,  SVX_RES(FT_FONT1)),
	aFont1CB			(this,	SVX_RES(CB_FONT1)),
	aFont2FT			(this,	SVX_RES(FT_FONT2)),
	aFont2CB			(this,	SVX_RES(CB_FONT2)),
	aNewDelTBX			(this,	SVX_RES(TBX_SUBSTNEWDEL)),
    aCheckLB            (this,  SVX_RES(CLB_SUBSTITUTES)),

    aSourceViewFontsFL (this,  SVX_RES(FL_SOURCEVIEW  )),
    aFontNameFT        (this,  SVX_RES(FT_FONTNAME    )),
    aFontNameLB        (this,  SVX_RES(LB_FONTNAME    )),
    aNonPropFontsOnlyCB(this,  SVX_RES(CB_NONPROP     )),
    aFontHeightFT      (this,  SVX_RES(FT_FONTHEIGHT  )),
    aFontHeightLB      (this,  SVX_RES(LB_FONTHEIGHT  )),

	aImageList			(SVX_RES(IL_ICON)),

    sAutomatic          (SVX_RES( STR_AUTOMATIC  )),
    pConfig(new SvtFontSubstConfig),
    pSourceViewConfig(new svt::SourceViewConfig),

	sHeader1			(SVX_RES( STR_HEADER1		)),
	sHeader2			(SVX_RES( STR_HEADER2		)),
	sHeader3			(SVX_RES( STR_HEADER3		)),
	sHeader4			(SVX_RES( STR_HEADER4		)),

    sFontGroup          ("FontSubstitution"),
    pCheckButtonData(0)
{
	FreeResource();

	aTextColor = aCheckLB.GetTextColor();

	for(USHORT k = 0; k < aNewDelTBX.GetItemCount(); k++)
		aNewDelTBX.SetItemImage(aNewDelTBX.GetItemId(k),
			aImageList.GetImage(aNewDelTBX.GetItemId(k)));

	aNewDelTBX.SetSizePixel( aNewDelTBX.CalcWindowSizePixel() );

	long nDelta = ( aFont1CB.GetSizePixel().Height() -
					aNewDelTBX.GetSizePixel().Height() ) / 2;
	Point aNewPnt = aNewDelTBX.GetPosPixel();
	aNewPnt.Y() += nDelta;
	aNewDelTBX.SetPosPixel( aNewPnt );

	aCheckLB.SetHelpId(HID_OFA_FONT_SUBST_CLB);
	aCheckLB.SetWindowBits(aCheckLB.GetStyle()|WB_HSCROLL|WB_VSCROLL);
	aCheckLB.SetSelectionMode(MULTIPLE_SELECTION);
	aCheckLB.SortByCol(2);

	Link aLink(LINK(this, SvxFontSubstTabPage, SelectHdl));

	aCheckLB.SetSelectHdl(aLink);
	aUseTableCB.SetClickHdl(aLink);
	aFont1CB.SetSelectHdl(aLink);
	aFont1CB.SetModifyHdl(aLink);
	aFont2CB.SetSelectHdl(aLink);
	aFont2CB.SetModifyHdl(aLink);
	aNewDelTBX.SetClickHdl(aLink);

    aNonPropFontsOnlyCB.SetClickHdl(LINK(this, SvxFontSubstTabPage, NonPropFontsHdl));

    static long aStaticTabs[] = { 4, 0, 0, 0, 0 };
    long nW1 = GetTextWidth( sHeader1 );
    long nW2 = GetTextWidth( sHeader2 );
    long nMax = Max( nW1, nW2 ) + 6; // width of the longest header + a little offset
    long nMin = aFontNameFT.LogicToPixel( Size( 30, 0 ), MAP_APPFONT ).Width();
    nMax = Max( nMax, nMin );
    const long nDoubleMax = 2*nMax;
    const long nRest = aCheckLB.GetSizePixel().Width() - nDoubleMax;
    aStaticTabs[2] = nMax;
    aStaticTabs[3] = nDoubleMax;
    aStaticTabs[4] = nDoubleMax + nRest/2;
    aCheckLB.SvxSimpleTable::SetTabs( aStaticTabs, MAP_PIXEL );

    String sHeader(sHeader1);
	String sTab(String::CreateFromAscii("\t"));
	String sTabSpace(String::CreateFromAscii("\t "));
	sHeader += sTab;
	sHeader += sHeader2;
	sHeader += sTabSpace;
	sHeader += sHeader3;
	sHeader += sTabSpace;
	sHeader += sHeader4;
//   sHeader += sTabSpace;
	aCheckLB.InsertHeaderEntry(sHeader);

	HeaderBar* pBar = aCheckLB.GetTheHeaderBar();

	HeaderBarItemBits nBits = pBar->GetItemBits(1) | HIB_FIXEDPOS | HIB_FIXED;
	nBits &= ~HIB_CLICKABLE;
	pBar->SetItemBits(1, nBits);
	pBar->SetItemBits(2, nBits);

    USHORT nHeight;
    for(nHeight = 6; nHeight <= 16; nHeight++)
        aFontHeightLB.InsertEntry(String::CreateFromInt32(nHeight));
    for(nHeight = 18; nHeight <= 28; nHeight+= 2)
        aFontHeightLB.InsertEntry(String::CreateFromInt32(nHeight));
    for(nHeight = 32; nHeight <= 48; nHeight+= 4)
        aFontHeightLB.InsertEntry(String::CreateFromInt32(nHeight));
    for(nHeight = 54; nHeight <= 72; nHeight+= 6)
        aFontHeightLB.InsertEntry(String::CreateFromInt32(nHeight));
    for(nHeight = 80; nHeight <= 96; nHeight+= 8)
        aFontHeightLB.InsertEntry(String::CreateFromInt32(nHeight));
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

SvLBoxEntry* SvxFontSubstTabPage::CreateEntry(String& rFont1, String& rFont2)
{
	SvLBoxEntry* pEntry = new SvLBoxEntry;

	if( !pCheckButtonData )
		pCheckButtonData = new SvLBoxButtonData( &aCheckLB );

	pEntry->AddItem( new SvLBoxContextBmp( pEntry, 0, Image(), Image(), 0));	// Sonst Puff!

	pEntry->AddItem( new SvLBoxButton( pEntry,
                                           SvLBoxButtonKind_enabledCheckbox, 0,
                                           pCheckButtonData ) );
	pEntry->AddItem( new SvLBoxButton( pEntry,
                                           SvLBoxButtonKind_enabledCheckbox, 0,
                                           pCheckButtonData ) );

	pEntry->AddItem( new SvLBoxString( pEntry, 0, rFont1 ) );
	pEntry->AddItem( new SvLBoxString( pEntry, 0, rFont2 ) );

	return pEntry;
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/


 SvxFontSubstTabPage::~SvxFontSubstTabPage()
{
	delete pCheckButtonData;
	delete pConfig;
    delete pSourceViewConfig;
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

SfxTabPage*  SvxFontSubstTabPage::Create( Window* pParent,
								const SfxItemSet& rAttrSet)
{
	return new SvxFontSubstTabPage(pParent, rAttrSet);
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

BOOL  SvxFontSubstTabPage::FillItemSet( SfxItemSet& )
{
	pConfig->ClearSubstitutions();// remove all entries

	pConfig->Enable(aUseTableCB.IsChecked());

	SvLBoxEntry* pEntry = aCheckLB.First();

	while (pEntry)
	{
		SubstitutionStruct aAdd;
        aAdd.sFont = aCheckLB.GetEntryText(pEntry, 0);
        aAdd.sReplaceBy = aCheckLB.GetEntryText(pEntry, 1);
		aAdd.bReplaceAlways = aCheckLB.IsChecked(pEntry, 0);
		aAdd.bReplaceOnScreenOnly = aCheckLB.IsChecked(pEntry, 1);
		pConfig->AddSubstitution(aAdd);
		pEntry = aCheckLB.Next(pEntry);
	}
	if(pConfig->IsModified())
		pConfig->Commit();
	pConfig->Apply();
    if(aFontHeightLB.GetSavedValue() != aFontHeightLB.GetSelectEntryPos())
        pSourceViewConfig->SetFontHeight((sal_Int16)aFontHeightLB.GetSelectEntry().ToInt32());
    if(aNonPropFontsOnlyCB.GetSavedValue() != aNonPropFontsOnlyCB.IsChecked())
        pSourceViewConfig->SetShowProportionalFontsOnly(aNonPropFontsOnlyCB.IsChecked());
    //font name changes cannot be detected by saved values
    String sFontName;
    if(aFontNameLB.GetSelectEntryPos())
        sFontName = aFontNameLB.GetSelectEntry();
    pSourceViewConfig->SetFontName(sFontName);

	return FALSE;
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/


void  SvxFontSubstTabPage::Reset( const SfxItemSet& )
{
	aCheckLB.SetUpdateMode(FALSE);
	aCheckLB.Clear();

	FontList aFntLst( Application::GetDefaultDevice() );
	aFont1CB.Fill( &aFntLst );
	aFont2CB.Fill( &aFntLst );

	sal_Int32 nCount = pConfig->SubstitutionCount();
	if (nCount)
		aUseTableCB.Check(pConfig->IsEnabled());

	for (sal_Int32  i = 0; i < nCount; i++)
	{
		const SubstitutionStruct* pSubs = pConfig->GetSubstitution(i);
		String aTmpStr1(pSubs->sFont);
        String aTmpStr2(pSubs->sReplaceBy);
        SvLBoxEntry* pEntry = CreateEntry(aTmpStr1, aTmpStr2);
        aCheckLB.Insert(pEntry);
		aCheckLB.CheckEntry(pEntry, 0, pSubs->bReplaceAlways);
		aCheckLB.CheckEntry(pEntry, 1, pSubs->bReplaceOnScreenOnly);
	}

	CheckEnable();
	aCheckLB.SetUpdateMode(TRUE);

    //fill font name box first
    aNonPropFontsOnlyCB.Check(pSourceViewConfig->IsShowProportionalFontsOnly());
    NonPropFontsHdl(&aNonPropFontsOnlyCB);
    String sFontName(pSourceViewConfig->GetFontName());
    if(sFontName.Len())
        aFontNameLB.SelectEntry(sFontName);
    else
        aFontNameLB.SelectEntryPos(0);
    short nFontHeight = pSourceViewConfig->GetFontHeight();
    aFontHeightLB.SelectEntry(String::CreateFromInt32(nFontHeight));
    aNonPropFontsOnlyCB.SaveValue();
    aFontHeightLB.SaveValue();
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

IMPL_LINK(SvxFontSubstTabPage, SelectHdl, Window*, pWin)
{
	if (pWin == &aNewDelTBX)
	{
		SvLBoxEntry* pEntry;
		// nCol ist behaemmerterweise die nCol'te Textspalte, werden nicht gezaehlt!
		// Daher als Spalte "0".
		ULONG nPos = aCheckLB.GetEntryPos(aFont1CB.GetText(), 0);

		switch (aNewDelTBX.GetCurItemId())
		{
			case BT_SUBSTAPPLY:
			{
				if (nPos != 0xffffffff)
				{
					// Eintrag aendern
					aCheckLB.SetEntryText(aFont2CB.GetText(), nPos, 1);
					pEntry = aCheckLB.GetEntry(nPos);
				}
				else
				{
					// Neuer Eintrag
					String sFont1 = aFont1CB.GetText();
					String sFont2 = aFont2CB.GetText();

					pEntry = CreateEntry(sFont1, sFont2);
                    aCheckLB.Insert(pEntry);
				}
				aCheckLB.SelectAll(FALSE);
				aCheckLB.Select(pEntry);
			}
			break;

			case BT_SUBSTDELETE:
			{
				if (nPos != 0xffffffff)
				{
					pEntry = aCheckLB.FirstSelected();
					while (pEntry)
					{
						SvLBoxEntry* pDelEntry = pEntry;
						pEntry = aCheckLB.NextSelected(pEntry);
                        aCheckLB.RemoveEntry(pDelEntry);
					}
				}
			}
			break;
		}
	}

	if (pWin == &aCheckLB)
	{
		SvLBoxEntry* pEntry = aCheckLB.FirstSelected();

		if (aCheckLB.NextSelected(pEntry) == 0)
		{
            aFont1CB.SetText(aCheckLB.GetEntryText(pEntry, 0));
            aFont2CB.SetText(aCheckLB.GetEntryText(pEntry, 1));
		}
	}

	if (pWin == &aFont1CB)
	{
		ULONG nPos = aCheckLB.GetEntryPos(aFont1CB.GetText(), 0);

		if (nPos != 0xffffffff)
		{
			SvLBoxEntry* pEntry = aCheckLB.GetEntry(nPos);

			if (pEntry != aCheckLB.FirstSelected())
			{
				aCheckLB.SelectAll(FALSE);
				aCheckLB.Select(pEntry);
			}
		}
	}

	CheckEnable();

	return 0;
}
/* -----------------------------29.08.2002 11:47------------------------------

 ---------------------------------------------------------------------------*/
IMPL_LINK(SvxFontSubstTabPage, NonPropFontsHdl, CheckBox*, pBox)
{
    String sFontName = aFontNameLB.GetSelectEntry();
    BOOL bNonPropOnly = pBox->IsChecked();
    aFontNameLB.Clear();
    FontList aFntLst( Application::GetDefaultDevice() );
    aFontNameLB.InsertEntry(sAutomatic);
    USHORT nFontCount = aFntLst.GetFontNameCount();
    for(USHORT nFont = 0; nFont < nFontCount; nFont++)
    {
        const FontInfo& rInfo = aFntLst.GetFontName( nFont );
        if(!bNonPropOnly || rInfo.GetPitch() == PITCH_FIXED)
            aFontNameLB.InsertEntry(rInfo.GetName());
    }
    aFontNameLB.SelectEntry(sFontName);
    return 0;
}
/*********************************************************************/
/*                                                                   */
/*********************************************************************/

void SvxFontSubstTabPage::CheckEnable()
{
	BOOL bEnableAll = aUseTableCB.IsChecked();

	if (bEnableAll)
	{
		BOOL bApply, bDelete;

		SvLBoxEntry* pEntry = aCheckLB.FirstSelected();

		String sEntry = aFont1CB.GetText();
		sEntry += '\t';
		sEntry += aFont2CB.GetText();

/*		if (!aFont1CB.GetText().Len() || !aFont2CB.GetText().Len() ||
			aFont1CB.GetText() == aFont2CB.GetText() ||
			aCheckLB.GetEntryPos(sEntry) != 0xffffffff ||
			(pEntry != 0 && aCheckLB.NextSelected(pEntry) != 0))
			bApply = FALSE;
		else
			bApply = TRUE;*/

		// Wegen OS/2-Optimierungsfehler (Bug #56267) etwas umstaendlicher:
		if (!aFont1CB.GetText().Len() || !aFont2CB.GetText().Len())
			bApply = FALSE;
		else if(aFont1CB.GetText() == aFont2CB.GetText())
			bApply = FALSE;
		else if(aCheckLB.GetEntryPos(sEntry) != 0xffffffff)
			bApply = FALSE;
		else if(pEntry != 0 && aCheckLB.NextSelected(pEntry) != 0)
			bApply = FALSE;
		else
			bApply = TRUE;

		bDelete = pEntry != 0;

		aNewDelTBX.EnableItem(BT_SUBSTAPPLY, bApply);
		aNewDelTBX.EnableItem(BT_SUBSTDELETE, bDelete);
	}

	if (bEnableAll)
	{
		if (!aCheckLB.IsEnabled())
		{
            aCheckLB.EnableTable();
			aCheckLB.SetTextColor(aTextColor);
			aCheckLB.Invalidate();
			SelectHdl(&aFont1CB);
		}
	}
	else
	{
        if (aCheckLB.IsEnabled())
		{
            aCheckLB.DisableTable();
			aCheckLB.SetTextColor(Color(COL_GRAY));
			aCheckLB.Invalidate();
			aCheckLB.SelectAll(FALSE);
		}
	}
	aNewDelTBX.Enable(bEnableAll);
	aFont1FT.Enable(bEnableAll);
	aFont1CB.Enable(bEnableAll);
	aFont2FT.Enable(bEnableAll);
	aFont2CB.Enable(bEnableAll);
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

void SvxFontSubstCheckListBox::SetTabs()
{
	SvxSimpleTable::SetTabs();
	USHORT nAdjust = SV_LBOXTAB_ADJUST_RIGHT|SV_LBOXTAB_ADJUST_LEFT|SV_LBOXTAB_ADJUST_CENTER|SV_LBOXTAB_ADJUST_NUMERIC|SV_LBOXTAB_FORCE;

	SvLBoxTab* pTab = (SvLBoxTab*)aTabs.GetObject(1);
	pTab->nFlags &= ~nAdjust;
	pTab->nFlags |= SV_LBOXTAB_PUSHABLE|SV_LBOXTAB_ADJUST_CENTER|SV_LBOXTAB_FORCE;

	pTab = (SvLBoxTab*)aTabs.GetObject(2);
	pTab->nFlags &= ~nAdjust;
	pTab->nFlags |= SV_LBOXTAB_PUSHABLE|SV_LBOXTAB_ADJUST_CENTER|SV_LBOXTAB_FORCE;
}
/* -----------------------------22.05.2002 11:06------------------------------

 ---------------------------------------------------------------------------*/
void    SvxFontSubstCheckListBox::KeyInput( const KeyEvent& rKEvt )
{
    if(!rKEvt.GetKeyCode().GetModifier() &&
        KEY_SPACE == rKEvt.GetKeyCode().GetCode())
    {
        ULONG nSelPos = GetModel()->GetAbsPos(GetCurEntry());
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

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

void SvxFontSubstCheckListBox::CheckEntryPos(ULONG nPos, USHORT nCol, BOOL bChecked)
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

void SvxFontSubstCheckListBox::CheckEntry(SvLBoxEntry* pEntry, USHORT nCol, BOOL bChecked)
{
	if ( pEntry )
		SetCheckButtonState(
			pEntry,
			nCol,
			bChecked ? SvButtonState( SV_BUTTON_CHECKED ) :
									   SvButtonState( SV_BUTTON_UNCHECKED ) );
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

BOOL SvxFontSubstCheckListBox::IsChecked(ULONG nPos, USHORT nCol)
{
	return GetCheckButtonState( GetEntry(nPos), nCol ) == SV_BUTTON_CHECKED;
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

BOOL SvxFontSubstCheckListBox::IsChecked(SvLBoxEntry* pEntry, USHORT nCol)
{
	return GetCheckButtonState( pEntry, nCol ) == SV_BUTTON_CHECKED;
}

/*********************************************************************/
/*                                                                   */
/*********************************************************************/

void SvxFontSubstCheckListBox::SetCheckButtonState( SvLBoxEntry* pEntry, USHORT nCol, SvButtonState eState)
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

SvButtonState SvxFontSubstCheckListBox::GetCheckButtonState( SvLBoxEntry* pEntry, USHORT nCol ) const
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


