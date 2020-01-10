/************************************************************************
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
#include "precompiled_sc.hxx"



#include <svx/eeitem.hxx>


#ifndef _FM_FMOBJFAC_HXX
#include <svx/fmobjfac.hxx>
#endif
#include <svx/objfac3d.hxx>
#include <svx/tbxcolor.hxx>

#include <sot/clsids.hxx>
#include <sfx2/docfilt.hxx>
#include <sfx2/fcontnr.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/app.hxx>
#include <avmedia/mediaplayer.hxx>
#include <avmedia/mediatoolbox.hxx>
#include <comphelper/types.hxx>
#include <svx/extrusioncontrols.hxx>
#include <svx/fontworkgallery.hxx>
#include <svx/tbxcustomshapes.hxx>

#include <svtools/parhtml.hxx>
#include <sot/formats.hxx>
#define SOT_FORMATSTR_ID_STARCALC_30 SOT_FORMATSTR_ID_STARCALC

#include "scitems.hxx"		// fuer tbxctrls etc.
#include "scmod.hxx"
#include "scresid.hxx"
#include "sc.hrc"
#include "cfgids.hxx"

//!	die Registrierung wird wegen CLOOKs in ein eigenes File wandern muessen...

// Interface-Registrierung
#include "docsh.hxx"
#include "tabvwsh.hxx"
#include "prevwsh.hxx"
#include "drawsh.hxx"
#include "drformsh.hxx"
#include "drtxtob.hxx"
#include "editsh.hxx"
#include "pivotsh.hxx"
#include "auditsh.hxx"
#include "cellsh.hxx"
#include "oleobjsh.hxx"
#include "chartsh.hxx"
#include "graphsh.hxx"
#include "mediash.hxx"
#include "pgbrksh.hxx"

#include "docpool.hxx"
#include "appoptio.hxx"

// Controls

#include <svx/tbxalign.hxx>
#include <svx/tbxctl.hxx>
#include <svx/fillctrl.hxx>
#include <svx/linectrl.hxx>
#include <svx/tbcontrl.hxx>
#include <svx/selctrl.hxx>
#include <svx/insctrl.hxx>
#include <svx/zoomctrl.hxx>
#include <svx/flditem.hxx>
#include <svx/modctrl.hxx>
#include <svx/pszctrl.hxx>
#include <svx/fntctl.hxx>
#include <svx/fntszctl.hxx>
#include <svx/grafctrl.hxx>
#include <svx/galbrws.hxx>
#include <svx/clipboardctl.hxx>
#include <svx/lboxctrl.hxx>
#include <svx/verttexttbxctrl.hxx>
#include <svx/formatpaintbrushctrl.hxx>
#include "tbinsert.hxx"
#include "tbzoomsliderctrl.hxx"
#include <svx/zoomsliderctrl.hxx>

#include <svx/xmlsecctrl.hxx>
// Child-Windows
#include "reffact.hxx"
#include "navipi.hxx"
#include "inputwin.hxx"
#include "spelldialog.hxx"
#include <svx/fontwork.hxx>
#include <svx/srchdlg.hxx>
#include <svx/hyprlink.hxx>
#include <svx/hyperdlg.hxx>
#include <svx/imapdlg.hxx>

#include "editutil.hxx"
#include <svx/svdfield.hxx>		//	SdrRegisterFieldClasses
#include <rtl/logfile.hxx>

#include "dwfunctr.hxx"
#include "acredlin.hxx"

//------------------------------------------------------------------

//UNUSED2008-05  //	filter detection can't use ScFilterOptions (in sc-dll),
//UNUSED2008-05  //	so access to wk3 flag must be implemented here again
//UNUSED2008-05  
//UNUSED2008-05  class ScLibOptions : public utl::ConfigItem
//UNUSED2008-05  {
//UNUSED2008-05      BOOL        bWK3Flag;
//UNUSED2008-05  
//UNUSED2008-05  public:
//UNUSED2008-05                  ScLibOptions();
//UNUSED2008-05      BOOL        GetWK3Flag() const          { return bWK3Flag; }
//UNUSED2008-05  };
//UNUSED2008-05  
//UNUSED2008-05  #define CFGPATH_LIBFILTER		"Office.Calc/Filter/Import/Lotus123"
//UNUSED2008-05  #define ENTRYSTR_WK3			"WK3"
//UNUSED2008-05  
//UNUSED2008-05  ScLibOptions::ScLibOptions() :
//UNUSED2008-05      ConfigItem( rtl::OUString::createFromAscii( CFGPATH_LIBFILTER ) ),
//UNUSED2008-05      bWK3Flag( FALSE )
//UNUSED2008-05  {
//UNUSED2008-05      com::sun::star::uno::Sequence<rtl::OUString> aNames(1);
//UNUSED2008-05      aNames[0] = rtl::OUString::createFromAscii( ENTRYSTR_WK3 );
//UNUSED2008-05      com::sun::star::uno::Sequence<com::sun::star::uno::Any> aValues = GetProperties(aNames);
//UNUSED2008-05      if ( aValues.getLength() == 1 && aValues[0].hasValue() )
//UNUSED2008-05          bWK3Flag = comphelper::getBOOL( aValues[0] );
//UNUSED2008-05  }

//------------------------------------------------------------------

ScResId::ScResId( USHORT nId ) :
	ResId( nId, *SC_MOD()->GetResMgr() )
{
}

//------------------------------------------------------------------

void ScDLL::Init()
{
	RTL_LOGFILE_CONTEXT_AUTHOR ( aLog, "sc", "nn93723", "ScDLL::Init" );

	ScModule **ppShlPtr = (ScModule**) GetAppData(SHL_CALC);
	if ( *ppShlPtr )
		return;

	ScDocumentPool::InitVersionMaps();	// wird im ScModule ctor gebraucht

	ScModule* pMod = new ScModule( &ScDocShell::Factory() );
	(*ppShlPtr) = pMod;

//REMOVE		ScDocShell::RegisterFactory( SDT_SC_DOCFACTPRIO );

	ScDocShell::Factory().SetDocumentServiceName( rtl::OUString::createFromAscii( "com.sun.star.sheet.SpreadsheetDocument" ) );

	ScGlobal::Init();		// erst wenn der ResManager initialisiert ist
							//	erst nach ScGlobal::Init duerfen die App-Optionen
							//	initialisiert werden

	// register your view-factories here

	ScTabViewShell		::RegisterFactory(1);
	ScPreviewShell		::RegisterFactory(2);

	// register your shell-interfaces here

	ScModule			::RegisterInterface(pMod);
	ScDocShell			::RegisterInterface(pMod);
	ScTabViewShell		::RegisterInterface(pMod);
	ScPreviewShell		::RegisterInterface(pMod);
	ScDrawShell			::RegisterInterface(pMod);
	ScDrawFormShell		::RegisterInterface(pMod);
	ScDrawTextObjectBar	::RegisterInterface(pMod);
	ScEditShell			::RegisterInterface(pMod);
	ScPivotShell		::RegisterInterface(pMod);
	ScAuditingShell		::RegisterInterface(pMod);
	ScFormatShell		::RegisterInterface(pMod);
	ScCellShell			::RegisterInterface(pMod);
	ScOleObjectShell	::RegisterInterface(pMod);
	ScChartShell		::RegisterInterface(pMod);
	ScGraphicShell		::RegisterInterface(pMod);
	ScMediaShell		::RegisterInterface(pMod);
	ScPageBreakShell	::RegisterInterface(pMod);

	//	eigene Controller
	ScTbxInsertCtrl		::RegisterControl(SID_TBXCTL_INSERT, pMod);
	ScTbxInsertCtrl		::RegisterControl(SID_TBXCTL_INSCELLS, pMod);
	ScTbxInsertCtrl		::RegisterControl(SID_TBXCTL_INSOBJ, pMod);
    ScZoomSliderControl ::RegisterControl(SID_PREVIEW_SCALINGFACTOR, pMod);

	//	Svx-Toolbox-Controller
    SvxTbxCtlDraw                   ::RegisterControl(SID_INSERT_DRAW,          pMod);
	SvxTbxCtlCustomShapes			::RegisterControl(SID_DRAWTBX_CS_BASIC,		pMod);
	SvxTbxCtlCustomShapes			::RegisterControl(SID_DRAWTBX_CS_SYMBOL,	pMod);
	SvxTbxCtlCustomShapes			::RegisterControl(SID_DRAWTBX_CS_ARROW,		pMod);
	SvxTbxCtlCustomShapes			::RegisterControl(SID_DRAWTBX_CS_FLOWCHART, pMod);
	SvxTbxCtlCustomShapes			::RegisterControl(SID_DRAWTBX_CS_CALLOUT,	pMod);
	SvxTbxCtlCustomShapes			::RegisterControl(SID_DRAWTBX_CS_STAR,		pMod);
	SvxTbxCtlAlign					::RegisterControl(SID_OBJECT_ALIGN,			pMod);
	SvxFillToolBoxControl			::RegisterControl(0, pMod);
	SvxLineStyleToolBoxControl		::RegisterControl(0, pMod);
	SvxLineWidthToolBoxControl		::RegisterControl(0, pMod);
	SvxLineColorToolBoxControl		::RegisterControl(0, pMod);
	SvxLineEndToolBoxControl		::RegisterControl(SID_ATTR_LINEEND_STYLE,	pMod);
	SvxStyleToolBoxControl			::RegisterControl(SID_STYLE_APPLY,			pMod);
	SvxFontNameToolBoxControl		::RegisterControl(SID_ATTR_CHAR_FONT,		pMod);
//	SvxFontHeightToolBoxControl		::RegisterControl(SID_ATTR_CHAR_FONTHEIGHT,	pMod);
	SvxFontColorToolBoxControl		::RegisterControl(SID_ATTR_CHAR_COLOR,		pMod);
	SvxColorToolBoxControl			::RegisterControl(SID_BACKGROUND_COLOR,		pMod);
	SvxFrameToolBoxControl			::RegisterControl(SID_ATTR_BORDER,			pMod);
	SvxFrameLineStyleToolBoxControl	::RegisterControl(SID_FRAME_LINESTYLE,		pMod);
	SvxFrameLineColorToolBoxControl	::RegisterControl(SID_FRAME_LINECOLOR,		pMod);
	SvxClipBoardControl				::RegisterControl(SID_PASTE,				pMod );
	SvxUndoRedoControl				::RegisterControl(SID_UNDO,					pMod );
	SvxUndoRedoControl				::RegisterControl(SID_REDO,					pMod );
    svx::FormatPaintBrushToolBoxControl::RegisterControl(SID_FORMATPAINTBRUSH,  pMod );

	SvxGrafModeToolBoxControl		::RegisterControl(SID_ATTR_GRAF_MODE,		pMod);
	SvxGrafRedToolBoxControl		::RegisterControl(SID_ATTR_GRAF_RED,		pMod);
	SvxGrafGreenToolBoxControl		::RegisterControl(SID_ATTR_GRAF_GREEN,		pMod);
	SvxGrafBlueToolBoxControl		::RegisterControl(SID_ATTR_GRAF_BLUE,		pMod);
	SvxGrafLuminanceToolBoxControl	::RegisterControl(SID_ATTR_GRAF_LUMINANCE,	pMod);
	SvxGrafContrastToolBoxControl	::RegisterControl(SID_ATTR_GRAF_CONTRAST,	pMod);
	SvxGrafGammaToolBoxControl		::RegisterControl(SID_ATTR_GRAF_GAMMA,		pMod);
	SvxGrafTransparenceToolBoxControl::RegisterControl(SID_ATTR_GRAF_TRANSPARENCE, pMod);
	SvxGrafFilterToolBoxControl		::RegisterControl(SID_GRFFILTER,			pMod);

    SvxVertTextTbxCtrl::RegisterControl(SID_DRAW_CAPTION_VERTICAL,          pMod);
    SvxVertTextTbxCtrl::RegisterControl(SID_DRAW_TEXT_VERTICAL,             pMod);
    SvxVertTextTbxCtrl::RegisterControl(SID_TEXTDIRECTION_LEFT_TO_RIGHT,    pMod);
    SvxVertTextTbxCtrl::RegisterControl(SID_TEXTDIRECTION_TOP_TO_BOTTOM,    pMod);
    SvxCTLTextTbxCtrl::RegisterControl(SID_ATTR_PARA_LEFT_TO_RIGHT, pMod);
    SvxCTLTextTbxCtrl::RegisterControl(SID_ATTR_PARA_RIGHT_TO_LEFT, pMod);

	//Media Controller
	::avmedia::MediaToolBoxControl::RegisterControl( SID_AVMEDIA_TOOLBOX, pMod );

	// Svx-StatusBar-Controller
	SvxInsertStatusBarControl		::RegisterControl(SID_ATTR_INSERT,		pMod);
	SvxSelectionModeControl			::RegisterControl(SID_STATUS_SELMODE,	pMod);
	SvxZoomStatusBarControl			::RegisterControl(SID_ATTR_ZOOM,		pMod);
    SvxZoomSliderControl            ::RegisterControl(SID_ATTR_ZOOMSLIDER,  pMod);
	SvxModifyControl 				::RegisterControl(SID_DOC_MODIFIED,		pMod);
	XmlSecStatusBarControl          ::RegisterControl( SID_SIGNATURE,       pMod );

	SvxPosSizeStatusBarControl		::RegisterControl(SID_ATTR_SIZE,		pMod);

	// Svx-Menue-Controller
	SvxFontMenuControl				::RegisterControl(SID_ATTR_CHAR_FONT,		pMod);
	SvxFontSizeMenuControl			::RegisterControl(SID_ATTR_CHAR_FONTHEIGHT,	pMod);

	// CustomShape extrusion controller
	svx::ExtrusionDepthControl::RegisterControl( SID_EXTRUSION_DEPTH_FLOATER, pMod );
	svx::ExtrusionDirectionControl::RegisterControl( SID_EXTRUSION_DIRECTION_FLOATER, pMod );
	svx::ExtrusionLightingControl::RegisterControl( SID_EXTRUSION_LIGHTING_FLOATER, pMod );
	svx::ExtrusionSurfaceControl::RegisterControl( SID_EXTRUSION_SURFACE_FLOATER, pMod );
	svx::ExtrusionColorControl::RegisterControl( SID_EXTRUSION_3D_COLOR, pMod );

	svx::FontWorkShapeTypeControl::RegisterControl( SID_FONTWORK_SHAPE_TYPE, pMod );
	svx::FontWorkAlignmentControl::RegisterControl( SID_FONTWORK_ALIGNMENT_FLOATER, pMod );
	svx::FontWorkCharacterSpacingControl::RegisterControl( SID_FONTWORK_CHARACTER_SPACING_FLOATER, pMod );

	//	Child-Windows

	// Hack: Eingabezeile mit 42 registrieren, damit sie im PlugIn immer sichtbar ist
	ScInputWindowWrapper		::RegisterChildWindow(42, pMod, SFX_CHILDWIN_TASK|SFX_CHILDWIN_FORCEDOCK);
    ScNavigatorDialogWrapper    ::RegisterChildWindowContext(
            sal::static_int_cast<sal_uInt16>(ScTabViewShell::GetInterfaceId()), pMod);
	ScSolverDlgWrapper			::RegisterChildWindow(FALSE, pMod);
    ScOptSolverDlgWrapper       ::RegisterChildWindow(FALSE, pMod);
	ScNameDlgWrapper			::RegisterChildWindow(FALSE, pMod);
	ScPivotLayoutWrapper		::RegisterChildWindow(FALSE, pMod);
	ScTabOpDlgWrapper			::RegisterChildWindow(FALSE, pMod);
	ScFilterDlgWrapper			::RegisterChildWindow(FALSE, pMod);
	ScSpecialFilterDlgWrapper	::RegisterChildWindow(FALSE, pMod);
	ScDbNameDlgWrapper			::RegisterChildWindow(FALSE, pMod);
	ScPrintAreasDlgWrapper		::RegisterChildWindow(FALSE, pMod);
	ScCondFormatDlgWrapper		::RegisterChildWindow(FALSE, pMod);
	ScColRowNameRangesDlgWrapper::RegisterChildWindow(FALSE, pMod);
	ScFormulaDlgWrapper			::RegisterChildWindow(FALSE, pMod);

	// First docking Window for Calc
	ScFunctionChildWindow		::RegisterChildWindow(FALSE, pMod);

	// Redlining- Window
	ScAcceptChgDlgWrapper		::RegisterChildWindow(FALSE, pMod);
    ScSimpleRefDlgWrapper       ::RegisterChildWindow(FALSE, pMod, SFX_CHILDWIN_ALWAYSAVAILABLE|SFX_CHILDWIN_NEVERHIDE );
	ScHighlightChgDlgWrapper	::RegisterChildWindow(FALSE, pMod);

    SvxSearchDialogWrapper      ::RegisterChildWindow(FALSE, pMod);
    SvxHlinkDlgWrapper          ::RegisterChildWindow(FALSE, pMod);
	SvxFontWorkChildWindow		::RegisterChildWindow(FALSE, pMod);
	SvxHyperlinkDlgWrapper		::RegisterChildWindow(FALSE, pMod, SFX_CHILDWIN_FORCEDOCK);
	SvxIMapDlgChildWindow		::RegisterChildWindow(FALSE, pMod);
	GalleryChildWindow			::RegisterChildWindow(FALSE, pMod);
    ScSpellDialogChildWindow    ::RegisterChildWindow(FALSE, pMod);
	::avmedia::MediaPlayer		::RegisterChildWindow(FALSE, pMod);

	//	Edit-Engine-Felder, soweit nicht schon in OfficeApplication::Init

	SvClassManager& rClassManager = SvxFieldItem::GetClassManager();
//	rClassManager.SV_CLASS_REGISTER( SvxURLField );
//	rClassManager.SV_CLASS_REGISTER( SvxDateField );
//	rClassManager.SV_CLASS_REGISTER( SvxPageField );
	rClassManager.SV_CLASS_REGISTER( SvxPagesField );
//	rClassManager.SV_CLASS_REGISTER( SvxTimeField );
	rClassManager.SV_CLASS_REGISTER( SvxFileField );
//	rClassManager.SV_CLASS_REGISTER( SvxExtFileField );
	rClassManager.SV_CLASS_REGISTER( SvxTableField );

	SdrRegisterFieldClasses();		// SvDraw-Felder registrieren

	// 3D-Objekt-Factory eintragen
    E3dObjFactory();

    // ::com::sun::star::form::component::Form-Objekt-Factory eintragen
    FmFormObjFactory();

    pMod->PutItem( SfxUInt16Item( SID_ATTR_METRIC, sal::static_int_cast<UINT16>(pMod->GetAppOptions().GetAppMetric()) ) );

	//	StarOne Services are now handled in the registry
}

void ScDLL::Exit()
{
	// the SxxModule must be destroyed
	ScModule **ppShlPtr = (ScModule**) GetAppData(SHL_CALC);
	delete (*ppShlPtr);
	(*ppShlPtr) = NULL;

	//	ScGlobal::Clear ist schon im Module-dtor
}

//------------------------------------------------------------------
//	Statusbar
//------------------------------------------------------------------

#define TEXT_WIDTH(s)	rStatusBar.GetTextWidth((s))

//UNUSED2008-05  void ScDLL::FillStatusBar(StatusBar &rStatusBar)
//UNUSED2008-05  {
//UNUSED2008-05      // Dokumentposition (Tabelle x / y)
//UNUSED2008-05      rStatusBar.InsertItem( SID_STATUS_DOCPOS,
//UNUSED2008-05                              TEXT_WIDTH( String().Fill( 10, 'X' ) ),
//UNUSED2008-05                              SIB_LEFT|SIB_AUTOSIZE );
//UNUSED2008-05  
//UNUSED2008-05      // Seitenvorlage
//UNUSED2008-05      rStatusBar.InsertItem( SID_STATUS_PAGESTYLE,
//UNUSED2008-05                              TEXT_WIDTH( String().Fill( 15, 'X' ) ),
//UNUSED2008-05                              SIB_LEFT|SIB_AUTOSIZE );
//UNUSED2008-05  
//UNUSED2008-05      // Ma"sstab
//UNUSED2008-05      rStatusBar.InsertItem(  SID_ATTR_ZOOM,
//UNUSED2008-05                              SvxZoomStatusBarControl::GetDefItemWidth(rStatusBar),
//UNUSED2008-05                              SIB_CENTER );
//UNUSED2008-05  
//UNUSED2008-05      // Einfuege-/Ueberschreibmodus
//UNUSED2008-05      rStatusBar.InsertItem( SID_ATTR_INSERT,
//UNUSED2008-05                              SvxInsertStatusBarControl::GetDefItemWidth(rStatusBar),
//UNUSED2008-05                              SIB_CENTER );
//UNUSED2008-05  
//UNUSED2008-05      // Selektionsmodus
//UNUSED2008-05      rStatusBar.InsertItem( SID_STATUS_SELMODE,
//UNUSED2008-05                              SvxSelectionModeControl::GetDefItemWidth(rStatusBar),
//UNUSED2008-05                              SIB_CENTER );
//UNUSED2008-05  
//UNUSED2008-05      // Dokument geaendert
//UNUSED2008-05      rStatusBar.InsertItem( SID_DOC_MODIFIED,
//UNUSED2008-05                              SvxModifyControl::GetDefItemWidth(rStatusBar));
//UNUSED2008-05  
//UNUSED2008-05      // signatures
//UNUSED2008-05      rStatusBar.InsertItem( SID_SIGNATURE, XmlSecStatusBarControl::GetDefItemWidth( rStatusBar ), SIB_USERDRAW );
//UNUSED2008-05      rStatusBar.SetHelpId(SID_SIGNATURE, SID_SIGNATURE);
//UNUSED2008-05  
//UNUSED2008-05      // Mail
//UNUSED2008-05      rStatusBar.InsertItem( SID_MAIL_NOTIFY,
//UNUSED2008-05                              TEXT_WIDTH( String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("Mail")) ),
//UNUSED2008-05                              SIB_CENTER );
//UNUSED2008-05  
//UNUSED2008-05      // den aktuellen Kontext anzeigen Uhrzeit / FramePos / TabellenInfo / Errors
//UNUSED2008-05      rStatusBar.InsertItem( SID_ATTR_SIZE,
//UNUSED2008-05                              SvxPosSizeStatusBarControl::GetDefItemWidth(rStatusBar),
//UNUSED2008-05                              SIB_AUTOSIZE|SIB_LEFT|SIB_USERDRAW);
//UNUSED2008-05  }

#undef TEXT_WIDTH

// DetectFilter functionality has moved - please update your bookmarks
// see sc/source/ui/unoobj/scdetect.cxx, have a nice day.
