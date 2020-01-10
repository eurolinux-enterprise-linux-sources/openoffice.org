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

#ifndef _SD_TPOPTION_HXX
#define _SD_TPOPTION_HXX


#ifndef _LSTBOX_HXX //autogen
#include <vcl/lstbox.hxx>
#endif
#ifndef _GROUP_HXX //autogen
#include <vcl/group.hxx>
#endif
#ifndef _FIXED_HXX //autogen
#include <vcl/fixed.hxx>
#endif
#ifndef _FIELD_HXX //autogen
#include <vcl/field.hxx>
#endif
#ifndef _BUTTON_HXX //autogen
#include <vcl/button.hxx>
#endif
#include <svtools/stdctrl.hxx>
#include <sfx2/tabdlg.hxx>
#include <svx/optgrid.hxx>

/*************************************************************************
|*
|* Optionen-Tab-Page: Snap
|*
\************************************************************************/
class SdTpOptionsSnap : public SvxGridTabPage
{
public:
			SdTpOptionsSnap( Window* pParent, const SfxItemSet& rInAttrs  );
			~SdTpOptionsSnap();

	static	SfxTabPage* Create( Window*, const SfxItemSet& );
	virtual BOOL FillItemSet( SfxItemSet& );
	virtual void Reset( const SfxItemSet & );

//    virtual void ActivatePage( const SfxItemSet& rSet );
//    virtual int  DeactivatePage( SfxItemSet* pSet );
};
/*************************************************************************
|*
|* Optionen-Tab-Page: Contents (Inhalte)
|*
\************************************************************************/
class SdTpOptionsContents : public SfxTabPage
{
private:
    FixedLine    aGrpDisplay;
    CheckBox    aCbxRuler;
	CheckBox	aCbxDragStripes;
	CheckBox	aCbxHandlesBezier;
	CheckBox	aCbxMoveOutline;


public:
			SdTpOptionsContents( Window* pParent, const SfxItemSet& rInAttrs  );
			~SdTpOptionsContents();

	static	SfxTabPage* Create( Window*, const SfxItemSet& );
	virtual BOOL FillItemSet( SfxItemSet& );
	virtual void Reset( const SfxItemSet & );
};

/*************************************************************************
|*
|* Optionen-Tab-Page: View
|*
\************************************************************************/
class SdModule;
class SdTpOptionsMisc : public SfxTabPage
{
 friend class SdOptionsDlg;
 friend class SdModule;

private:
    FixedLine   aGrpText;
    CheckBox    aCbxQuickEdit;
    CheckBox    aCbxPickThrough;

    FixedLine   aGrpProgramStart;
    CheckBox    aCbxStartWithTemplate;

    FixedLine   aGrpSettings;
    CheckBox    aCbxMasterPageCache;
    CheckBox    aCbxCopy;
    CheckBox    aCbxMarkedHitMovesAlways;
    CheckBox    aCbxCrookNoContortion;

    FixedText   aTxtMetric;
    ListBox     aLbMetric;
    FixedText   aTxtTabstop;
	MetricField	aMtrFldTabstop;

    CheckBox    aCbxStartWithActualPage;
    FixedLine   aGrpStartWithActualPage;
    FixedLine   aTxtCompatibility;
    CheckBox    aCbxUsePrinterMetrics;
    CheckBox    aCbxCompatibility;

    //Scale
    FixedLine       aGrpScale;
    FixedText       aFtScale;
    ComboBox        aCbScale;

    FixedText       aFtOriginal;
    FixedText       aFtEquivalent;

    FixedText       aFtPageWidth;
	FixedInfo		aFiInfo1;
    MetricField     aMtrFldOriginalWidth;

    FixedText       aFtPageHeight;
	FixedInfo		aFiInfo2;
    MetricField     aMtrFldOriginalHeight;

    MetricField     aMtrFldInfo1;
	MetricField 	aMtrFldInfo2;

    UINT32          nWidth;
	UINT32			nHeight;
	String			aInfo1;
	String			aInfo2;

	SfxMapUnit			ePoolUnit;

	String			GetScale( INT32 nX, INT32 nY );
	BOOL			SetScale( const String& aScale, INT32& rX, INT32& rY );

	DECL_LINK( ModifyScaleHdl, void * );
	DECL_LINK( ModifyOriginalScaleHdl, void * );
    DECL_LINK( SelectMetricHdl_Impl, ListBox * );

    /** Enable or disable the controls in the compatibility section of the
        'general' tab page depending on whether there is at least one
        document.
    */
    void UpdateCompatibilityControls (void);

protected:
	virtual void ActivatePage( const SfxItemSet& rSet );
	virtual int	DeactivatePage( SfxItemSet* pSet );

public:
			SdTpOptionsMisc( Window* pParent, const SfxItemSet& rInAttrs  );
			~SdTpOptionsMisc();

	static	SfxTabPage* Create( Window*, const SfxItemSet& );
	virtual BOOL FillItemSet( SfxItemSet& );
	virtual void Reset( const SfxItemSet & );

    /** Hide Impress specific controls, make Draw specific controls visible
        and arrange the visible controls.  Do not call this method or the
        <member>SetImpressMode()</member> method more than once.
    */
    void SetDrawMode (void);

    /** Hide Draw specific controls, make Impress specific controls visible
        and arrange the visible controls.  Do not call this method or the
        <member>SetDrawMode()</member> method more than once.
    */
    void SetImpressMode (void);
	virtual void		PageCreated (SfxAllItemSet aSet);

	using TabPage::ActivatePage;
	using TabPage::DeactivatePage;
	using OutputDevice::SetDrawMode;

};


#endif // _SD_TPOPTION_HXX

