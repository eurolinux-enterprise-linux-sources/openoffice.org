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
#include <optaccessibility.hxx>
#include <optaccessibility.hrc>
#include <svx/dialmgr.hxx>
#include <svx/dialogs.hrc>
#include <svtools/accessibilityoptions.hxx>
#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>

static void MovePosY( Window& _rWin, long _nDelta )
{
	Point	aPoint = _rWin.GetPosPixel();
	aPoint.Y() += _nDelta;
	
	_rWin.SetPosPixel( aPoint );
}

struct SvxAccessibilityOptionsTabPage_Impl
{
	SvtAccessibilityOptions 	m_aConfig;
	SvxAccessibilityOptionsTabPage_Impl()
			: m_aConfig(){}
};

SvxAccessibilityOptionsTabPage::SvxAccessibilityOptionsTabPage( Window* pParent, const SfxItemSet& rSet )
	:SfxTabPage(pParent, SVX_RES( RID_SVXPAGE_ACCESSIBILITYCONFIG ), rSet)
	,m_aMiscellaneousLabel		(this, SVX_RES(FL_MISCELLANEOUS		))
	,m_aAccessibilityTool		(this, SVX_RES(CB_ACCESSIBILITY_TOOL	))
    ,m_aTextSelectionInReadonly	(this, SVX_RES(CB_TEXTSELECTION		))
	,m_aAnimatedGraphics		(this, SVX_RES(CB_ANIMATED_GRAPHICS	))
	,m_aAnimatedTexts			(this, SVX_RES(CB_ANIMATED_TEXTS		))
	,m_aTipHelpCB				(this, SVX_RES(CB_TIPHELP				))
	,m_aTipHelpNF				(this, SVX_RES(NF_TIPHELP				))
	,m_aTipHelpFT				(this, SVX_RES(FT_TIPHELP				))
	,m_aHCOptionsLabel			(this, SVX_RES(FL_HC_OPTIONS			))
	,m_aAutoDetectHC			(this, SVX_RES(CB_AUTO_DETECT_HC		))
	,m_aAutomaticFontColor		(this, SVX_RES(CB_AUTOMATIC_FONT_COLOR))
    ,m_aPagePreviews			(this, SVX_RES(CB_PAGE_PREVIEWS       ))
	,m_pImpl(new SvxAccessibilityOptionsTabPage_Impl)
{
	FreeResource();
	m_aTipHelpCB.SetClickHdl(LINK(this, SvxAccessibilityOptionsTabPage, TipHelpHdl));

	long nHeightDelta = 0;		// to correct positions _under_ m_aAccessibilityTool

#ifdef UNX
	{
		// UNIX: read the gconf2 setting instead to use the checkbox
		m_aAccessibilityTool.Hide();
		nHeightDelta = -( ROWA_2 - ROWA_1 );
	}
#else
	// calculate the height of the checkbox. Do we need two (default in resource) or only one line
	String aText = m_aAccessibilityTool.GetText();
	long nWidth = m_aAccessibilityTool.GetTextWidth( aText );
	long nCtrlWidth = m_aAccessibilityTool.GetSizePixel().Width() - ( COL2 - COL1 );
	if ( nWidth > nCtrlWidth )
	{
		long nDelta = 2 * RSC_CD_FIXEDLINE_HEIGHT + LINESPACE - RSC_CD_CHECKBOX_HEIGHT;
		nHeightDelta = nDelta;
		Size aSize = m_aAccessibilityTool.LogicToPixel( Size( 0, nDelta ), MAP_APPFONT );
		nDelta = aSize.Height();
		aSize = m_aAccessibilityTool.GetSizePixel();
		aSize.Height() += nDelta;
		m_aAccessibilityTool.SetSizePixel( aSize );
	}
#endif

	if( nHeightDelta )
	{	//adjust positions of controls under m_aAccessibilityTool
		Size aSize = m_aAccessibilityTool.LogicToPixel( Size( 0, nHeightDelta ), MAP_APPFONT );
		nHeightDelta = aSize.Height();

		MovePosY( m_aTextSelectionInReadonly, nHeightDelta );
		MovePosY( m_aAnimatedGraphics, nHeightDelta );
		MovePosY( m_aAnimatedTexts, nHeightDelta );
		MovePosY( m_aTipHelpCB, nHeightDelta );
		MovePosY( m_aTipHelpNF, nHeightDelta );
		MovePosY( m_aTipHelpFT, nHeightDelta );
		MovePosY( m_aHCOptionsLabel, nHeightDelta );
		MovePosY( m_aAutoDetectHC, nHeightDelta );
		MovePosY( m_aAutomaticFontColor, nHeightDelta );
		MovePosY( m_aPagePreviews, nHeightDelta );
	}
}

SvxAccessibilityOptionsTabPage::~SvxAccessibilityOptionsTabPage()
{
	delete m_pImpl;
}

SfxTabPage*	SvxAccessibilityOptionsTabPage::Create( Window* pParent, const SfxItemSet& rAttrSet )
{
	return new SvxAccessibilityOptionsTabPage(pParent, rAttrSet);
}

BOOL SvxAccessibilityOptionsTabPage::FillItemSet( SfxItemSet& )
{
	//aConfig.Set... from controls

    m_pImpl->m_aConfig.SetIsForPagePreviews( m_aPagePreviews.IsChecked() );
	m_pImpl->m_aConfig.SetIsHelpTipsDisappear( m_aTipHelpCB.IsChecked() );
	m_pImpl->m_aConfig.SetHelpTipSeconds( (short)m_aTipHelpNF.GetValue() );
	m_pImpl->m_aConfig.SetIsAllowAnimatedGraphics( m_aAnimatedGraphics.IsChecked() );
	m_pImpl->m_aConfig.SetIsAllowAnimatedText( m_aAnimatedTexts.IsChecked() );
	m_pImpl->m_aConfig.SetIsAutomaticFontColor(	m_aAutomaticFontColor.IsChecked() );
    m_pImpl->m_aConfig.SetSelectionInReadonly( m_aTextSelectionInReadonly.IsChecked());
    m_pImpl->m_aConfig.SetAutoDetectSystemHC( m_aAutoDetectHC.IsChecked());

	if(m_pImpl->m_aConfig.IsModified())
		m_pImpl->m_aConfig.Commit();

    AllSettings aAllSettings = Application::GetSettings();
    MiscSettings aMiscSettings = aAllSettings.GetMiscSettings();
#ifndef UNX
    aMiscSettings.SetEnableATToolSupport( m_aAccessibilityTool.IsChecked() );
#endif
    aAllSettings.SetMiscSettings(aMiscSettings);
    Application::SetSettings(aAllSettings);
    Application::MergeSystemSettings( aAllSettings );

	return FALSE;
}

void SvxAccessibilityOptionsTabPage::Reset( const SfxItemSet& )
{
	//set controls from aConfig.Get...

    m_aPagePreviews.Check(			  m_pImpl->m_aConfig.GetIsForPagePreviews() );
	EnableTipHelp(					  m_pImpl->m_aConfig.GetIsHelpTipsDisappear() );
	m_aTipHelpNF.SetValue(			  m_pImpl->m_aConfig.GetHelpTipSeconds() );
	m_aAnimatedGraphics.Check(		  m_pImpl->m_aConfig.GetIsAllowAnimatedGraphics() );
	m_aAnimatedTexts.Check(			  m_pImpl->m_aConfig.GetIsAllowAnimatedText() );
	m_aAutomaticFontColor.Check(	  m_pImpl->m_aConfig.GetIsAutomaticFontColor() );
//	m_aSystemFont.Check(			  m_pImpl->m_aConfig.GetIsSystemFont() );
    m_aTextSelectionInReadonly.Check( m_pImpl->m_aConfig.IsSelectionInReadonly() );
    m_aAutoDetectHC.Check(			  m_pImpl->m_aConfig.GetAutoDetectSystemHC() );


    AllSettings aAllSettings = Application::GetSettings();
    MiscSettings aMiscSettings = aAllSettings.GetMiscSettings();
    m_aAccessibilityTool.Check( aMiscSettings.GetEnableATToolSupport() );
}

IMPL_LINK(SvxAccessibilityOptionsTabPage, TipHelpHdl, CheckBox*, pBox)
{
	sal_Bool bChecked = pBox->IsChecked();
	m_aTipHelpNF.Enable(bChecked);
	return 0;
}

void SvxAccessibilityOptionsTabPage::EnableTipHelp(sal_Bool bCheck)
{
	m_aTipHelpCB.Check(bCheck);
	m_aTipHelpNF.Enable(bCheck);
}
