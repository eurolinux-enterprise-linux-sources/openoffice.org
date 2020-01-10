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
#include <sfx2/dispatch.hxx>
#include <sfx2/module.hxx>
#include <sfx2/viewfrm.hxx>
#include <svtools/eitem.hxx>
#include <svtools/colrdlg.hxx>
#include <vcl/msgbox.hxx>
#include <sfx2/viewsh.hxx>
#include <tools/shl.hxx>
#include <svx/xflclit.hxx>
#include <svx/svdmodel.hxx>
#include <globl3d.hxx>
#include <svx/view3d.hxx>
#include <svx/obj3d.hxx>
#include <svx/sphere3d.hxx>
#include <svx/scene3d.hxx>
#include <svx/camera3d.hxx>
#include <svx/fmmodel.hxx>
#include <svx/fmpage.hxx>
#include <svx/polysc3d.hxx>
#include <svx/eeitem.hxx>
#include <svtools/style.hxx>


#include <dlgutil.hxx>
#include <svx/dialmgr.hxx>
#include <svx/viewpt3d.hxx> // ProjectionType

#include <svx/svxids.hrc>
#include <svx/dialogs.hrc>

#include <svx/colritem.hxx>
#include <svx/e3ditem.hxx>

#include <gallery.hxx>
#define GALLERY_THEME "3D"
#include <svtools/whiter.hxx>

#include <svx/float3d.hxx>
#include "float3d.hrc"

SFX_IMPL_DOCKINGWINDOW( Svx3DChildWindow, SID_3D_WIN )

struct Svx3DWinImpl
{
	SfxItemPool*		pPool;
	Image				maImgLightOnH;
	Image				maImgLightOffH;
};

#define SETHCIMAGE(btn,res) \
{ \
	Bitmap aBmp( SVX_RES( res ) ); \
	Image aImage( aBmp, COL_LIGHTMAGENTA ); \
	btn.SetModeImage( aImage, BMP_COLOR_HIGHCONTRAST ); \
}

namespace {
    /** Get the dispatcher from the current view frame, or, if that is not
        available, from the given bindings.
        @param pBindings
            May be NULL.
        @returns NULL when both the current view frame is NULL and the given
            bindings are NULL.
    */
    SfxDispatcher* LocalGetDispatcher (const SfxBindings* pBindings)
    {
        SfxDispatcher* pDispatcher = NULL;
        
        if (SfxViewFrame::Current() != NULL)
            pDispatcher = SfxViewFrame::Current()->GetDispatcher();
        else if (pBindings != NULL)
            pDispatcher = pBindings->GetDispatcher();

        return pDispatcher;
    }
}


/*************************************************************************
|*	Svx3DWin - FloatingWindow
\************************************************************************/
__EXPORT Svx3DWin::Svx3DWin( SfxBindings* pInBindings,
				SfxChildWindow *pCW, Window* pParent ) :
		SfxDockingWindow    ( pInBindings, pCW, pParent,
									SVX_RES( RID_SVXFLOAT_3D ) ),
		aBtnGeo				( this, SVX_RES( BTN_GEO ) ),
		aBtnRepresentation	( this, SVX_RES( BTN_REPRESENTATION ) ),
		aBtnLight			( this, SVX_RES( BTN_LIGHT ) ),
		aBtnTexture			( this, SVX_RES( BTN_TEXTURE ) ),
		aBtnMaterial		( this, SVX_RES( BTN_MATERIAL ) ),
		aBtnUpdate			( this, SVX_RES( BTN_UPDATE ) ),
		aBtnAssign			( this, SVX_RES( BTN_ASSIGN ) ),

		// Geometrie
		aFtPercentDiagonal	( this, SVX_RES( FT_PERCENT_DIAGONAL ) ),
		aMtrPercentDiagonal	( this, SVX_RES( MTR_PERCENT_DIAGONAL ) ),
		aFtBackscale		( this, SVX_RES( FT_BACKSCALE ) ),
		aMtrBackscale		( this, SVX_RES( MTR_BACKSCALE ) ),
		aFtEndAngle			( this, SVX_RES( FT_END_ANGLE ) ),
		aMtrEndAngle		( this, SVX_RES( MTR_END_ANGLE ) ),
		aFtDepth			( this, SVX_RES( FT_DEPTH ) ),
		aMtrDepth			( this, SVX_RES( MTR_DEPTH ) ),
        aFLGeometrie       ( this, SVX_RES( FL_GEOMETRIE ) ),

		aFtHorizontal		( this, SVX_RES( FT_HORIZONTAL ) ),
		aNumHorizontal		( this, SVX_RES( NUM_HORIZONTAL ) ),
		aFtVertical			( this, SVX_RES( FT_VERTICAL ) ),
		aNumVertical		( this, SVX_RES( NUM_VERTICAL ) ),
        aFLSegments        ( this, SVX_RES( FL_SEGMENTS ) ),

		aBtnNormalsObj		( this, SVX_RES( BTN_NORMALS_OBJ ) ),
		aBtnNormalsFlat		( this, SVX_RES( BTN_NORMALS_FLAT ) ),
		aBtnNormalsSphere	( this, SVX_RES( BTN_NORMALS_SPHERE ) ),
		aBtnNormalsInvert	( this, SVX_RES( BTN_NORMALS_INVERT ) ),
		aBtnTwoSidedLighting( this, SVX_RES( BTN_TWO_SIDED_LIGHTING ) ),
        aFLNormals         ( this, SVX_RES( FL_NORMALS ) ),

		aBtnDoubleSided   	( this, SVX_RES( BTN_DOUBLE_SIDED ) ),

		// Darstellung
		aFtShademode		( this, SVX_RES( FT_SHADEMODE ) ),
		aLbShademode		( this, SVX_RES( LB_SHADEMODE ) ),
		aBtnShadow3d 		( this, SVX_RES( BTN_SHADOW_3D ) ),
		aFtSlant      		( this, SVX_RES( FT_SLANT ) ),
		aMtrSlant     		( this, SVX_RES( MTR_SLANT ) ),
        aFLShadow          ( this, SVX_RES( FL_SHADOW ) ),
		aFtDistance			( this, SVX_RES( FT_DISTANCE ) ),
		aMtrDistance		( this, SVX_RES( MTR_DISTANCE ) ),
		aFtFocalLeng		( this, SVX_RES( FT_FOCAL_LENGTH ) ),
		aMtrFocalLength		( this, SVX_RES( MTR_FOCAL_LENGTH ) ),
        aFLCamera          ( this, SVX_RES( FL_CAMERA ) ),
        aFLRepresentation  ( this, SVX_RES( FL_REPRESENTATION ) ),

		// Beleuchtung
		aBtnLight1			( this, SVX_RES( BTN_LIGHT_1 ) ),
		aBtnLight2			( this, SVX_RES( BTN_LIGHT_2 ) ),
		aBtnLight3			( this, SVX_RES( BTN_LIGHT_3 ) ),
		aBtnLight4			( this, SVX_RES( BTN_LIGHT_4 ) ),
		aBtnLight5			( this, SVX_RES( BTN_LIGHT_5 ) ),
		aBtnLight6			( this, SVX_RES( BTN_LIGHT_6 ) ),
		aBtnLight7			( this, SVX_RES( BTN_LIGHT_7 ) ),
		aBtnLight8			( this, SVX_RES( BTN_LIGHT_8 ) ),
		aLbLight1			( this, SVX_RES( LB_LIGHT_1 ) ),
		aLbLight2   		( this, SVX_RES( LB_LIGHT_2 ) ),
		aLbLight3			( this, SVX_RES( LB_LIGHT_3 ) ),
		aLbLight4			( this, SVX_RES( LB_LIGHT_4 ) ),
		aLbLight5			( this, SVX_RES( LB_LIGHT_5 ) ),
		aLbLight6			( this, SVX_RES( LB_LIGHT_6 ) ),
		aLbLight7			( this, SVX_RES( LB_LIGHT_7 ) ),
		aLbLight8			( this, SVX_RES( LB_LIGHT_8 ) ),
		
		aBtnLightColor		( this, SVX_RES( BTN_LIGHT_COLOR ) ),
        aFTLightsource     ( this, SVX_RES( FT_LIGHTSOURCE ) ),

		// #99694# Keyboard shortcuts activate the next control, so the
		// order needed to be changed here
        aFTAmbientlight    ( this, SVX_RES( FT_AMBIENTLIGHT ) ),	// Text label
		aLbAmbientlight		( this, SVX_RES( LB_AMBIENTLIGHT ) ),	// ListBox
		aBtnAmbientColor	( this, SVX_RES( BTN_AMBIENT_COLOR ) ), // color button
        
		aFLLight           ( this, SVX_RES( FL_LIGHT ) ),

		// Texturen
		aFtTexKind			( this, SVX_RES( FT_TEX_KIND ) ),
		aBtnTexLuminance	( this, SVX_RES( BTN_TEX_LUMINANCE ) ),
		aBtnTexColor		( this, SVX_RES( BTN_TEX_COLOR ) ),
		aFtTexMode			( this, SVX_RES( FT_TEX_MODE ) ),
		aBtnTexReplace		( this, SVX_RES( BTN_TEX_REPLACE ) ),
		aBtnTexModulate		( this, SVX_RES( BTN_TEX_MODULATE ) ),
		aBtnTexBlend		( this, SVX_RES( BTN_TEX_BLEND ) ),
		aFtTexProjectionX	( this, SVX_RES( FT_TEX_PROJECTION_X ) ),
		aBtnTexObjectX		( this, SVX_RES( BTN_TEX_OBJECT_X ) ),
		aBtnTexParallelX	( this, SVX_RES( BTN_TEX_PARALLEL_X ) ),
		aBtnTexCircleX		( this, SVX_RES( BTN_TEX_CIRCLE_X ) ),
		aFtTexProjectionY	( this, SVX_RES( FT_TEX_PROJECTION_Y ) ),
		aBtnTexObjectY		( this, SVX_RES( BTN_TEX_OBJECT_Y ) ),
		aBtnTexParallelY	( this, SVX_RES( BTN_TEX_PARALLEL_Y ) ),
		aBtnTexCircleY		( this, SVX_RES( BTN_TEX_CIRCLE_Y ) ),
		aFtTexFilter		( this, SVX_RES( FT_TEX_FILTER ) ),
		aBtnTexFilter		( this, SVX_RES( BTN_TEX_FILTER ) ),
        aFLTexture         ( this, SVX_RES( FL_TEXTURE ) ),

		// Material
		aFtMatFavorites 	( this, SVX_RES( FT_MAT_FAVORITES ) ),
		aLbMatFavorites 	( this, SVX_RES( LB_MAT_FAVORITES ) ),
		aFtMatColor			( this, SVX_RES( FT_MAT_COLOR ) ),
		aLbMatColor			( this, SVX_RES( LB_MAT_COLOR ) ),
		aBtnMatColor		( this, SVX_RES( BTN_MAT_COLOR ) ),
		aFtMatEmission		( this, SVX_RES( FT_MAT_EMISSION ) ),
		aLbMatEmission		( this, SVX_RES( LB_MAT_EMISSION ) ),
		aBtnEmissionColor	( this, SVX_RES( BTN_EMISSION_COLOR ) ),
		aFtMatSpecular		( this, SVX_RES( FT_MAT_SPECULAR ) ),
		aLbMatSpecular		( this, SVX_RES( LB_MAT_SPECULAR ) ),
		aBtnSpecularColor	( this, SVX_RES( BTN_SPECULAR_COLOR ) ),
		aFtMatSpecularIntensity( this, SVX_RES( FT_MAT_SPECULAR_INTENSITY ) ),
		aMtrMatSpecularIntensity( this, SVX_RES( MTR_MAT_SPECULAR_INTENSITY ) ),
        aFLMatSpecular     ( this, SVX_RES( FL_MAT_SPECULAR ) ),
        aFLMaterial        ( this, SVX_RES( FL_MATERIAL ) ),

		// Unterer Bereich
		aBtnConvertTo3D		( this, SVX_RES( BTN_CHANGE_TO_3D ) ),
		aBtnLatheObject		( this, SVX_RES( BTN_LATHE_OBJ ) ),
		aBtnPerspective		( this, SVX_RES( BTN_PERSPECTIVE ) ),
		aCtlPreview 		( this, SVX_RES( CTL_PREVIEW ) ),
		aCtlLightPreview 	( this, SVX_RES( CTL_LIGHT_PREVIEW ) ),

		aImgLightOn			( SVX_RES( RID_SVXIMAGE_LIGHT_ON ) ),
		aImgLightOff		( SVX_RES( RID_SVXIMAGE_LIGHT_OFF ) ),

		bUpdate				( FALSE ),
		eViewType			( VIEWTYPE_GEO ),

		pModel				( NULL ),
		pFmPage				( NULL ),
		pVDev	 			( NULL ),
		p3DView				( NULL ),
		pFavorSetList		( NULL ),
		pMatFavSetList		( NULL ),

		pBindings			( pInBindings ),
		pControllerItem(0L),
		pConvertTo3DItem(0L),
		pConvertTo3DLatheItem(0L),
//		pPool				( NULL ),
		mpImpl				( new Svx3DWinImpl() ),
		mpRemember2DAttributes(NULL),
		bOnly3DChanged		( FALSE )
{
	SETHCIMAGE( aBtnGeo, BMP_GEO_H );
	SETHCIMAGE( aBtnRepresentation, BMP_REPRESENTATION_H );
	SETHCIMAGE( aBtnLight, BMP_3DLIGHT_H );
	SETHCIMAGE( aBtnTexture, BMP_TEXTURE_H );
	SETHCIMAGE( aBtnMaterial, BMP_MATERIAL_H );
	SETHCIMAGE( aBtnUpdate, BMP_UPDATE_H );
	SETHCIMAGE( aBtnAssign, BMP_ASSIGN_H );
	SETHCIMAGE( aBtnNormalsObj, BMP_NORMALS_OBJ_H );
	SETHCIMAGE( aBtnNormalsFlat, BMP_NORMALS_FLAT_H );
	SETHCIMAGE( aBtnNormalsSphere, BMP_NORMALS_SPHERE_H );
	SETHCIMAGE( aBtnTwoSidedLighting, BMP_TWO_SIDED_LIGHTING_H );
	SETHCIMAGE( aBtnNormalsInvert, BMP_NORMALS_INVERT_H );
	SETHCIMAGE( aBtnDoubleSided, BMP_DOUBLE_SIDED_H );
	SETHCIMAGE( aBtnShadow3d, BMP_SHADOW_3D_H );
	SETHCIMAGE( aBtnLight1, BMP_LIGHT_H );
	SETHCIMAGE( aBtnLight2, BMP_LIGHT_H );
	SETHCIMAGE( aBtnLight3, BMP_LIGHT_H );
	SETHCIMAGE( aBtnLight4, BMP_LIGHT_H );
	SETHCIMAGE( aBtnLight5, BMP_LIGHT_H );
	SETHCIMAGE( aBtnLight6, BMP_LIGHT_H );
	SETHCIMAGE( aBtnLight7, BMP_LIGHT_H );
	SETHCIMAGE( aBtnLight8, BMP_LIGHT_H );
	SETHCIMAGE( aBtnLightColor, BMP_LIGHT_COLOR_H );
	SETHCIMAGE( aBtnAmbientColor, BMP_AMBIENT_COLOR_H );
	SETHCIMAGE( aBtnTexLuminance, BMP_TEX_LUMINANCE_H );
	SETHCIMAGE( aBtnTexColor, BMP_TEX_COLOR_H );
	SETHCIMAGE( aBtnTexReplace, BMP_TEX_REPLACE_H );
	SETHCIMAGE( aBtnTexModulate, BMP_TEX_MODULATE_H );
	SETHCIMAGE( aBtnTexBlend, BMP_TEX_BLEND_H );
	SETHCIMAGE( aBtnTexParallelX, BMP_TEX_PARALLEL_H );
	SETHCIMAGE( aBtnTexCircleX, BMP_TEX_CIRCLE_H );
	SETHCIMAGE( aBtnTexObjectX, BMP_TEX_OBJECT_H );
	SETHCIMAGE( aBtnTexParallelY, BMP_TEX_PARALLEL_H );
	SETHCIMAGE( aBtnTexCircleY, BMP_TEX_CIRCLE_H );
	SETHCIMAGE( aBtnTexObjectY, BMP_TEX_OBJECT_H );
	SETHCIMAGE( aBtnTexFilter, BMP_TEX_FILTER_H );
	SETHCIMAGE( aBtnMatColor, BMP_COLORDLG_H );
	SETHCIMAGE( aBtnEmissionColor, BMP_COLORDLG_H );
	SETHCIMAGE( aBtnSpecularColor, BMP_COLORDLG_H );
	SETHCIMAGE( aBtnPerspective, BMP_PERSPECTIVE_H );
	SETHCIMAGE( aBtnConvertTo3D, BMP_CHANGE_TO_3D_H );
	SETHCIMAGE( aBtnLatheObject, BMP_LATHE_OBJ_H );

	mpImpl->pPool = NULL;
	mpImpl->maImgLightOnH = Image( SVX_RES( RID_SVXIMAGE_LIGHT_ON_H ) );
	mpImpl->maImgLightOffH = Image( SVX_RES( RID_SVXIMAGE_LIGHT_OFF_H ) );
	FreeResource();

	// Metrik einstellen
	eFUnit = GetModuleFieldUnit( NULL );
	aMtrDepth.SetUnit( eFUnit );
	aMtrDistance.SetUnit( eFUnit );
	aMtrFocalLength.SetUnit( eFUnit );

	pControllerItem = new Svx3DCtrlItem(SID_3D_STATE, this, pBindings);
	pConvertTo3DItem = new SvxConvertTo3DItem(SID_CONVERT_TO_3D, pBindings);
	pConvertTo3DLatheItem = new SvxConvertTo3DItem(SID_CONVERT_TO_3D_LATHE_FAST, pBindings);

	aBtnAssign.SetClickHdl( LINK( this, Svx3DWin, ClickAssignHdl ) );
	aBtnUpdate.SetClickHdl( LINK( this, Svx3DWin, ClickUpdateHdl ) );

	Link aLink( LINK( this, Svx3DWin, ClickViewTypeHdl ) );
	aBtnGeo.SetClickHdl( aLink );
	aBtnRepresentation.SetClickHdl( aLink );
	aBtnLight.SetClickHdl( aLink );
	aBtnTexture.SetClickHdl( aLink );
	aBtnMaterial.SetClickHdl( aLink );

	aLink = LINK( this, Svx3DWin, ClickHdl );
	aBtnPerspective.SetClickHdl( aLink );
	aBtnConvertTo3D.SetClickHdl( aLink );
	aBtnLatheObject.SetClickHdl( aLink );

	// Geometrie
	aBtnNormalsObj.SetClickHdl( aLink );
	aBtnNormalsFlat.SetClickHdl( aLink );
	aBtnNormalsSphere.SetClickHdl( aLink );
	aBtnTwoSidedLighting.SetClickHdl( aLink );
	aBtnNormalsInvert.SetClickHdl( aLink );
	aBtnDoubleSided.SetClickHdl( aLink );

	// Darstellung
	aBtnShadow3d.SetClickHdl( aLink );

	// Beleuchtung
	aBtnLight1.SetClickHdl( aLink );
	aBtnLight2.SetClickHdl( aLink );
	aBtnLight3.SetClickHdl( aLink );
	aBtnLight4.SetClickHdl( aLink );
	aBtnLight5.SetClickHdl( aLink );
	aBtnLight6.SetClickHdl( aLink );
	aBtnLight7.SetClickHdl( aLink );
	aBtnLight8.SetClickHdl( aLink );

	// Texturen
	aBtnTexLuminance.SetClickHdl( aLink );
	aBtnTexColor.SetClickHdl( aLink );
	aBtnTexReplace.SetClickHdl( aLink );
	aBtnTexModulate.SetClickHdl( aLink );
	//aBtnTexBlend.SetClickHdl( aLink );
	aBtnTexParallelX.SetClickHdl( aLink );
	aBtnTexCircleX.SetClickHdl( aLink );
	aBtnTexObjectX.SetClickHdl( aLink );
	aBtnTexParallelY.SetClickHdl( aLink );
	aBtnTexCircleY.SetClickHdl( aLink );
	aBtnTexObjectY.SetClickHdl( aLink );
	aBtnTexFilter.SetClickHdl( aLink );

	// Material
	aLink = LINK( this, Svx3DWin, ClickColorHdl );
	aBtnLightColor.SetClickHdl( aLink );
	aBtnAmbientColor.SetClickHdl( aLink );
	aBtnMatColor.SetClickHdl( aLink );
	aBtnEmissionColor.SetClickHdl( aLink );
	aBtnSpecularColor.SetClickHdl( aLink );


	aLink = LINK( this, Svx3DWin, SelectHdl );
	aLbMatFavorites.SetSelectHdl( aLink );
	aLbMatColor.SetSelectHdl( aLink );
	aLbMatEmission.SetSelectHdl( aLink );
	aLbMatSpecular.SetSelectHdl( aLink );
	aLbLight1.SetSelectHdl( aLink );
	aLbLight2.SetSelectHdl( aLink );
	aLbLight3.SetSelectHdl( aLink );
	aLbLight4.SetSelectHdl( aLink );
	aLbLight5.SetSelectHdl( aLink );
	aLbLight6.SetSelectHdl( aLink );
	aLbLight7.SetSelectHdl( aLink );
	aLbLight8.SetSelectHdl( aLink );
	aLbAmbientlight.SetSelectHdl( aLink );
	aLbShademode.SetSelectHdl( aLink );

	aLink = LINK( this, Svx3DWin, ModifyHdl );
	aMtrMatSpecularIntensity.SetModifyHdl( aLink );
	aNumHorizontal.SetModifyHdl( aLink );
	aNumVertical.SetModifyHdl( aLink );
	aMtrSlant.SetModifyHdl( aLink );

	// Preview-Callback
	aLink = LINK( this, Svx3DWin, ChangeLightCallbackHdl );
	aCtlLightPreview.SetUserInteractiveChangeCallback(aLink);
	aLink = LINK( this, Svx3DWin, ChangeSelectionCallbackHdl );
	aCtlLightPreview.SetUserSelectionChangeCallback(aLink);

    aSize = GetOutputSizePixel();
	SetMinOutputSizePixel( aSize );

	Construct();

	// Initiierung der Initialisierung der ColorLBs
    SfxDispatcher* pDispatcher = LocalGetDispatcher(pBindings);
    if (pDispatcher != NULL)
    {
        SfxBoolItem aItem( SID_3D_INIT, TRUE );
        pDispatcher->Execute(
            SID_3D_INIT, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD, &aItem, 0L );
    }

	Reset();
}

// -----------------------------------------------------------------------
__EXPORT Svx3DWin::~Svx3DWin()
{
	//delete pMatFavSetList;
	delete p3DView;
	delete pVDev;
	delete pModel;

	delete pControllerItem;
	delete pConvertTo3DItem;
	delete pConvertTo3DLatheItem;

	if(mpRemember2DAttributes)
		delete mpRemember2DAttributes;

	delete mpImpl;
}

// -----------------------------------------------------------------------
void Svx3DWin::Construct()
{
	aBtnGeo.Check();
	Link aLink( LINK( this, Svx3DWin, ClickViewTypeHdl ) );
	aLink.Call( &aBtnGeo );
	aCtlLightPreview.Hide();
}

// -----------------------------------------------------------------------
void Svx3DWin::Reset()
{
	// Diverse Initialisierungen, default ist AllAttributes
	aLbShademode.SelectEntryPos( 0 );
	aMtrMatSpecularIntensity.SetValue( 50 );

	aBtnLight1.Check();
	ClickUpdateHdl( NULL );

	// Nichts selektieren, um Fehler beim erstselektieren zu vermeiden
	aCtlLightPreview.GetSvx3DLightControl().SelectLight(0);
}

bool Svx3DWin::GetUILightState( ImageButton& aBtn ) const
{
	return (aBtn.GetModeImage() == aImgLightOn) || (aBtn.GetModeImage() == mpImpl->maImgLightOnH);
}

void Svx3DWin::SetUILightState( ImageButton& aBtn, bool bState )
{
	aBtn.SetModeImage( bState ? aImgLightOn : aImgLightOff );
	aBtn.SetModeImage( bState ? mpImpl->maImgLightOnH : mpImpl->maImgLightOffH, BMP_COLOR_HIGHCONTRAST );
}

// -----------------------------------------------------------------------
void Svx3DWin::Update( SfxItemSet& rAttrs )
{
	// remember 2d attributes
	if(mpRemember2DAttributes)
		mpRemember2DAttributes->ClearItem();
	else
		mpRemember2DAttributes = new SfxItemSet(*rAttrs.GetPool(),
			SDRATTR_START, SDRATTR_SHADOW_LAST,
			SDRATTR_3D_FIRST, SDRATTR_3D_LAST,
			0, 0);

	SfxWhichIter aIter(*mpRemember2DAttributes);
	sal_uInt16 nWhich(aIter.FirstWhich());

	while(nWhich)
	{
		SfxItemState eState = rAttrs.GetItemState(nWhich, FALSE);
		if(SFX_ITEM_DONTCARE == eState)
			mpRemember2DAttributes->InvalidateItem(nWhich);
		else if(SFX_ITEM_SET == eState)
			mpRemember2DAttributes->Put(rAttrs.Get(nWhich, FALSE));

		nWhich = aIter.NextWhich();
	}

	// construct field values
	const SfxPoolItem* pItem;
	//BOOL bUpdate = FALSE;

	// evtl. PoolUnit ermitteln
	if( !mpImpl->pPool )
	{
		mpImpl->pPool = rAttrs.GetPool();
		DBG_ASSERT( mpImpl->pPool, "Wo ist der Pool?" );
		ePoolUnit = mpImpl->pPool->GetMetric( SID_ATTR_LINE_WIDTH );
	}
	eFUnit = GetModuleFieldUnit( &rAttrs );


// Segmentanzahl aenderbar ? und andere Stati
	SfxItemState eState = rAttrs.GetItemState( SID_ATTR_3D_INTERN, FALSE, &pItem );
	if( SFX_ITEM_SET == eState )
	{
		UINT32 nState = ( ( const SfxUInt32Item* )pItem )->GetValue();
		//BOOL bLathe   = (BOOL) ( nState & 1 );
		BOOL bExtrude = (BOOL) ( nState & 2 );
		BOOL bSphere  = (BOOL) ( nState & 4 );
		BOOL bCube    = (BOOL) ( nState & 8 );

		BOOL bChart = (BOOL) ( nState & 32 ); // Chart

		if( !bChart )
		{
			// Bei Cube-Objekten werden keine Segmente eingestellt
			aFtHorizontal.Enable( !bCube );
			aNumHorizontal.Enable( !bCube );
			aFtVertical.Enable( !bCube );
			aNumVertical.Enable( !bCube );
            aFLSegments.Enable( !bCube );

			aFtPercentDiagonal.Enable( !bCube && !bSphere );
			aMtrPercentDiagonal.Enable( !bCube && !bSphere );
			aFtBackscale.Enable( !bCube && !bSphere );
			aMtrBackscale.Enable( !bCube && !bSphere );
			aFtDepth.Enable( !bCube && !bSphere );
			aMtrDepth.Enable( !bCube && !bSphere );
			if( bCube )
			{
				aNumHorizontal.SetEmptyFieldValue();
				aNumVertical.SetEmptyFieldValue();
			}
			if( bCube || bSphere )
			{
				aMtrPercentDiagonal.SetEmptyFieldValue();
				aMtrBackscale.SetEmptyFieldValue();
				aMtrDepth.SetEmptyFieldValue();
			}

			// Nur bei Lathe-Objekten gibt es einen Endwinkel
			aFtEndAngle.Enable( !bExtrude && !bCube && !bSphere );
			aMtrEndAngle.Enable( !bExtrude && !bCube && !bSphere );
			if( bExtrude || bCube || bSphere )
				aMtrEndAngle.SetEmptyFieldValue();
		}
		else
		{
			// Geometrie
			aFtHorizontal.Enable( FALSE );
			aNumHorizontal.Enable( FALSE );
			aNumHorizontal.SetEmptyFieldValue();
			aFtVertical.Enable( FALSE );
			aNumVertical.Enable( FALSE );
			aNumVertical.SetEmptyFieldValue();
            aFLSegments.Enable( FALSE );
			aFtEndAngle.Enable( FALSE );
			aMtrEndAngle.Enable( FALSE );
			aMtrEndAngle.SetEmptyFieldValue();
			aFtDepth.Enable( FALSE );
			aMtrDepth.Enable( FALSE );
			aMtrDepth.SetEmptyFieldValue();

			// Darstellung
			aBtnShadow3d.Enable( FALSE );
			aFtSlant.Enable( FALSE );
			aMtrSlant.Enable( FALSE );
            aFLShadow.Enable( FALSE );

			aFtDistance.Enable( FALSE );
			aMtrDistance.Enable( FALSE );
			aMtrDistance.SetEmptyFieldValue();
			aFtFocalLeng.Enable( FALSE );
			aMtrFocalLength.Enable( FALSE );
			aMtrFocalLength.SetEmptyFieldValue();
            aFLCamera.Enable( FALSE );

			// Unterer Bereich
			aBtnConvertTo3D.Enable( FALSE );
			aBtnLatheObject.Enable( FALSE );
		}
	}
// Bitmapfuellung ? -> Status
	BOOL bBitmap(FALSE);
	eState = rAttrs.GetItemState(XATTR_FILLSTYLE);
	if(eState != SFX_ITEM_DONTCARE)
	{
		XFillStyle eXFS = (XFillStyle)((const XFillStyleItem&)rAttrs.Get(XATTR_FILLSTYLE)).GetValue();
		bBitmap = (eXFS == XFILL_BITMAP || eXFS == XFILL_GRADIENT || eXFS == XFILL_HATCH);
	}

	aFtTexKind.Enable( bBitmap );
	aBtnTexLuminance.Enable( bBitmap );
	aBtnTexColor.Enable( bBitmap );
	aFtTexMode.Enable( bBitmap );
	aBtnTexReplace.Enable( bBitmap );
	aBtnTexModulate.Enable( bBitmap );
	aBtnTexBlend.Enable( bBitmap );
	aFtTexProjectionX.Enable( bBitmap );
	aBtnTexParallelX.Enable( bBitmap );
	aBtnTexCircleX.Enable( bBitmap );
	aBtnTexObjectX.Enable( bBitmap );
	aFtTexProjectionY.Enable( bBitmap );
	aBtnTexParallelY.Enable( bBitmap );
	aBtnTexCircleY.Enable( bBitmap );
	aBtnTexObjectY.Enable( bBitmap );
	aFtTexFilter.Enable( bBitmap );
	aBtnTexFilter.Enable( bBitmap );
    aFLTexture.Enable( bBitmap );


// Geometrie
	// Anzahl Segmente (horizontal)
	if( aNumHorizontal.IsEnabled() )
	{
		eState = rAttrs.GetItemState(SDRATTR_3DOBJ_HORZ_SEGS);
		if(eState != SFX_ITEM_DONTCARE)
		{
			sal_uInt32 nValue = ((const Svx3DHorizontalSegmentsItem&)rAttrs.Get(SDRATTR_3DOBJ_HORZ_SEGS)).GetValue();
			if(nValue != (sal_uInt32 )aNumHorizontal.GetValue())
			{
				aNumHorizontal.SetValue( nValue );
				// evtl. am Ende...
				// aCtlLightPreview.GetSvx3DLightControl().SetHorizontalSegments( (UINT16)nValue );
				bUpdate = TRUE;
			}
			else if( aNumHorizontal.IsEmptyFieldValue() )
				aNumHorizontal.SetValue( nValue );
		}
		else
		{
			if( !aNumHorizontal.IsEmptyFieldValue() )
			{
				aNumHorizontal.SetEmptyFieldValue();
				bUpdate = TRUE;
			}
		}
	}

	// Anzahl Segmente (vertikal)
	if( aNumVertical.IsEnabled() )
	{
		eState = rAttrs.GetItemState(SDRATTR_3DOBJ_VERT_SEGS);
		if( eState != SFX_ITEM_DONTCARE )
		{
			UINT32 nValue = ((const Svx3DVerticalSegmentsItem&)rAttrs.Get(SDRATTR_3DOBJ_VERT_SEGS)).GetValue();
			if( nValue != (UINT32) aNumVertical.GetValue() )
			{
				aNumVertical.SetValue( nValue );
				// evtl. am Ende...
				//aCtlLightPreview.GetSvx3DLightControl().SetVerticalSegments( (UINT16)nValue );
				bUpdate = TRUE;
			}
			else if( aNumVertical.IsEmptyFieldValue() )
				aNumVertical.SetValue( nValue );
		}
		else
		{
			if( !aNumVertical.IsEmptyFieldValue() )
			{
				aNumVertical.SetEmptyFieldValue();
				bUpdate = TRUE;
			}
		}
	}

	// Tiefe
	if( aMtrDepth.IsEnabled() )
	{
		eState = rAttrs.GetItemState(SDRATTR_3DOBJ_DEPTH);
		if( eState != SFX_ITEM_DONTCARE )
		{
			UINT32 nValue = ((const Svx3DDepthItem&)rAttrs.Get(SDRATTR_3DOBJ_DEPTH)).GetValue();
			UINT32 nValue2 = GetCoreValue( aMtrDepth, ePoolUnit );
			if( nValue != nValue2 )
			{
				if( eFUnit != aMtrDepth.GetUnit() )
					SetFieldUnit( aMtrDepth, eFUnit );

				SetMetricValue( aMtrDepth, nValue, ePoolUnit );
				bUpdate = TRUE;
			}
			else if( aMtrDepth.IsEmptyFieldValue() )
				aMtrDepth.SetValue( aMtrDepth.GetValue() );
		}
		else
		{
			if( !aMtrDepth.IsEmptyFieldValue() )
			{
				aMtrDepth.SetEmptyFieldValue();
				bUpdate = TRUE;
			}
		}
	}

	// Doppelwandig/-seitig
	eState = rAttrs.GetItemState(SDRATTR_3DOBJ_DOUBLE_SIDED);
	if( eState != SFX_ITEM_DONTCARE )
	{
		BOOL bValue = ((const Svx3DDoubleSidedItem&)rAttrs.Get(SDRATTR_3DOBJ_DOUBLE_SIDED)).GetValue();
		if( bValue != aBtnDoubleSided.IsChecked() )
		{
			aBtnDoubleSided.Check( bValue );
			bUpdate = TRUE;
		}
		else if( aBtnDoubleSided.GetState() == STATE_DONTKNOW )
			aBtnDoubleSided.Check( bValue );
	}
	else
	{
		if( aBtnDoubleSided.GetState() != STATE_DONTKNOW )
		{
			aBtnDoubleSided.SetState( STATE_DONTKNOW );
			bUpdate = TRUE;
		}
	}

	// Kantenrundung
	if( aMtrPercentDiagonal.IsEnabled() )
	{
		eState = rAttrs.GetItemState(SDRATTR_3DOBJ_PERCENT_DIAGONAL);
		if( eState != SFX_ITEM_DONTCARE )
		{
			UINT16 nValue = ((const Svx3DPercentDiagonalItem&)rAttrs.Get(SDRATTR_3DOBJ_PERCENT_DIAGONAL)).GetValue();
			if( nValue != aMtrPercentDiagonal.GetValue() )
			{
				aMtrPercentDiagonal.SetValue( nValue );
				bUpdate = TRUE;
			}
			else if( aMtrPercentDiagonal.IsEmptyFieldValue() )
				aMtrPercentDiagonal.SetValue( nValue );
		}
		else
		{
			if( !aMtrPercentDiagonal.IsEmptyFieldValue() )
			{
				aMtrPercentDiagonal.SetEmptyFieldValue();
				bUpdate = TRUE;
			}
		}
	}

	// Tiefenskalierung
	if( aMtrBackscale.IsEnabled() )
	{
		eState = rAttrs.GetItemState(SDRATTR_3DOBJ_BACKSCALE);
		if( eState != SFX_ITEM_DONTCARE )
		{
			UINT16 nValue = ((const Svx3DBackscaleItem&)rAttrs.Get(SDRATTR_3DOBJ_BACKSCALE)).GetValue();
			if( nValue != aMtrBackscale.GetValue() )
			{
				aMtrBackscale.SetValue( nValue );
				bUpdate = TRUE;
			}
			else if( aMtrBackscale.IsEmptyFieldValue() )
				aMtrBackscale.SetValue( nValue );
		}
		else
		{
			if( !aMtrBackscale.IsEmptyFieldValue() )
			{
				aMtrBackscale.SetEmptyFieldValue();
				bUpdate = TRUE;
			}
		}
	}

	// Endwinkel
	if( aMtrEndAngle.IsEnabled() )
	{
		eState = rAttrs.GetItemState(SDRATTR_3DOBJ_END_ANGLE);
		if( eState != SFX_ITEM_DONTCARE )
		{
			INT32 nValue = ((const Svx3DEndAngleItem&)rAttrs.Get(SDRATTR_3DOBJ_END_ANGLE)).GetValue();
			if( nValue != aMtrEndAngle.GetValue() )
			{
				aMtrEndAngle.SetValue( nValue );
				bUpdate = TRUE;
			}
		}
		else
		{
			if( !aMtrEndAngle.IsEmptyFieldValue() )
			{
				aMtrEndAngle.SetEmptyFieldValue();
				bUpdate = TRUE;
			}
		}
	}

	// Normalentyp
	eState = rAttrs.GetItemState(SDRATTR_3DOBJ_NORMALS_KIND);
	if( eState != SFX_ITEM_DONTCARE )
	{
		UINT16 nValue = ((const Svx3DNormalsKindItem&)rAttrs.Get(SDRATTR_3DOBJ_NORMALS_KIND)).GetValue();

		if( ( !aBtnNormalsObj.IsChecked() && nValue == 0 ) ||
			( !aBtnNormalsFlat.IsChecked() && nValue == 1 ) ||
			( !aBtnNormalsSphere.IsChecked() && nValue == 2 ) )
		{
			aBtnNormalsObj.Check( nValue == 0 );
			aBtnNormalsFlat.Check( nValue == 1 );
			aBtnNormalsSphere.Check( nValue == 2 );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aBtnNormalsObj.IsChecked() ||
			aBtnNormalsFlat.IsChecked() ||
			aBtnNormalsSphere.IsChecked() )
		{
			aBtnNormalsObj.Check( FALSE );
			aBtnNormalsFlat.Check( FALSE );
			aBtnNormalsSphere.Check( FALSE );
			bUpdate = TRUE;
		}
	}

	// Normalen invertieren
	eState = rAttrs.GetItemState(SDRATTR_3DOBJ_NORMALS_INVERT);
	if( eState != SFX_ITEM_DONTCARE )
	{
		BOOL bValue = ((const Svx3DNormalsInvertItem&)rAttrs.Get(SDRATTR_3DOBJ_NORMALS_INVERT)).GetValue();
		if( bValue != aBtnNormalsInvert.IsChecked() )
		{
			aBtnNormalsInvert.Check( bValue );
			bUpdate = TRUE;
		}
		else if( aBtnNormalsInvert.GetState() == STATE_DONTKNOW )
			aBtnNormalsInvert.Check( bValue );
	}
	else
	{
		if( aBtnNormalsInvert.GetState() != STATE_DONTKNOW )
		{
			aBtnNormalsInvert.SetState( STATE_DONTKNOW );
			bUpdate = TRUE;
		}
	}

	// 2-seitige Beleuchtung
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_TWO_SIDED_LIGHTING);
	if( eState != SFX_ITEM_DONTCARE )
	{
		BOOL bValue = ((const Svx3DTwoSidedLightingItem&)rAttrs.Get(SDRATTR_3DSCENE_TWO_SIDED_LIGHTING)).GetValue();
		if( bValue != aBtnTwoSidedLighting.IsChecked() )
		{
			aBtnTwoSidedLighting.Check( bValue );
			bUpdate = TRUE;
		}
		else if( aBtnTwoSidedLighting.GetState() == STATE_DONTKNOW )
			aBtnTwoSidedLighting.Check( bValue );
	}
	else
	{
		if( aBtnTwoSidedLighting.GetState() != STATE_DONTKNOW )
		{
			aBtnTwoSidedLighting.SetState( STATE_DONTKNOW );
			bUpdate = TRUE;
		}
	}

// Darstellung
	// Shademode
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_SHADE_MODE);
	if( eState != SFX_ITEM_DONTCARE )
	{
		UINT16 nValue = ((const Svx3DShadeModeItem&)rAttrs.Get(SDRATTR_3DSCENE_SHADE_MODE)).GetValue();
		if( nValue != aLbShademode.GetSelectEntryPos() )
		{
			aLbShademode.SelectEntryPos( nValue );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aLbShademode.GetSelectEntryCount() != 0 )
		{
			aLbShademode.SetNoSelection();
			bUpdate = TRUE;
		}
	}

	// 3D-Shatten
	eState = rAttrs.GetItemState(SDRATTR_3DOBJ_SHADOW_3D);
	if( eState != SFX_ITEM_DONTCARE )
	{
		BOOL bValue = ((const Svx3DShadow3DItem&)rAttrs.Get(SDRATTR_3DOBJ_SHADOW_3D)).GetValue();
		if( bValue != aBtnShadow3d.IsChecked() )
		{
			aBtnShadow3d.Check( bValue );
			aFtSlant.Enable( bValue );
			aMtrSlant.Enable( bValue );
			bUpdate = TRUE;
		}
		else if( aBtnShadow3d.GetState() == STATE_DONTKNOW )
			aBtnShadow3d.Check( bValue );
	}
	else
	{
		if( aBtnShadow3d.GetState() != STATE_DONTKNOW )
		{
			aBtnShadow3d.SetState( STATE_DONTKNOW );
			bUpdate = TRUE;
		}
	}

	// Neigung (Schatten)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_SHADOW_SLANT);
	if( eState != SFX_ITEM_DONTCARE )
	{
		UINT16 nValue = ((const Svx3DShadowSlantItem&)rAttrs.Get(SDRATTR_3DSCENE_SHADOW_SLANT)).GetValue();
		if( nValue != aMtrSlant.GetValue() )
		{
			aMtrSlant.SetValue( nValue );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( !aMtrSlant.IsEmptyFieldValue() )
		{
			aMtrSlant.SetEmptyFieldValue();
			bUpdate = TRUE;
		}
	}

	// Distanz
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_DISTANCE);
	if( eState != SFX_ITEM_DONTCARE )
	{
		UINT32 nValue = ((const Svx3DDistanceItem&)rAttrs.Get(SDRATTR_3DSCENE_DISTANCE)).GetValue();
		UINT32 nValue2 = GetCoreValue( aMtrDistance, ePoolUnit );
		if( nValue != nValue2 )
		{
			if( eFUnit != aMtrDistance.GetUnit() )
				SetFieldUnit( aMtrDistance, eFUnit );

			SetMetricValue( aMtrDistance, nValue, ePoolUnit );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( !aMtrDepth.IsEmptyFieldValue() )
		{
			aMtrDepth.SetEmptyFieldValue();
			bUpdate = TRUE;
		}
	}

	// Brennweite
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_FOCAL_LENGTH);
	if( eState != SFX_ITEM_DONTCARE )
	{
		UINT32 nValue = ((const Svx3DFocalLengthItem&)rAttrs.Get(SDRATTR_3DSCENE_FOCAL_LENGTH)).GetValue();
		UINT32 nValue2 = GetCoreValue( aMtrFocalLength, ePoolUnit );
		if( nValue != nValue2 )
		{
			if( eFUnit != aMtrFocalLength.GetUnit() )
				SetFieldUnit( aMtrFocalLength, eFUnit );

			SetMetricValue( aMtrFocalLength, nValue, ePoolUnit );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( !aMtrFocalLength.IsEmptyFieldValue() )
		{
			aMtrFocalLength.SetEmptyFieldValue();
			bUpdate = TRUE;
		}
	}

// Beleuchtung
	Color aColor;
	basegfx::B3DVector aVector;
	// Licht 1 (Farbe)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTCOLOR_1);
	if( eState != SFX_ITEM_DONTCARE )
	{
		aColor = ((const Svx3DLightcolor1Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTCOLOR_1)).GetValue();
		ColorLB* pLb = &aLbLight1;
		if( aColor != pLb->GetSelectEntryColor() )
		{
			LBSelectColor( pLb, aColor );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aLbLight1.GetSelectEntryCount() != 0 )
		{
			aLbLight1.SetNoSelection();
			bUpdate = TRUE;
		}
	}
	// Licht 1 (an/aus)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTON_1);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bool bOn = ((const Svx3DLightOnOff1Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTON_1)).GetValue() != 0;
		if( ( bOn && !GetUILightState( aBtnLight1 )) ||
			( !bOn && GetUILightState( aBtnLight1 )) )
		{
			SetUILightState( aBtnLight1, bOn );
			bUpdate = TRUE;
		}
		if( aBtnLight1.GetState() == STATE_DONTKNOW )
			aBtnLight1.Check( aBtnLight1.IsChecked() );
	}
	else
	{
		if( aBtnLight1.GetState() != STATE_DONTKNOW )
		{
			aBtnLight1.SetState( STATE_DONTKNOW );
			bUpdate = TRUE;
		}
	}
	// Licht 1 (Richtung)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTDIRECTION_1);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bUpdate = TRUE;
	}

	// Licht 2 (Farbe)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTCOLOR_2);
	if( eState != SFX_ITEM_DONTCARE )
	{
		aColor = ((const Svx3DLightcolor2Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTCOLOR_2)).GetValue();
		ColorLB* pLb = &aLbLight2;
		if( aColor != pLb->GetSelectEntryColor() )
		{
			LBSelectColor( pLb, aColor );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aLbLight2.GetSelectEntryCount() != 0 )
		{
			aLbLight2.SetNoSelection();
			bUpdate = TRUE;
		}
	}
	// Licht 2 (an/aus)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTON_2);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bool bOn = ((const Svx3DLightOnOff2Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTON_2)).GetValue() != 0;
		if( ( bOn && !GetUILightState( aBtnLight2 )) ||
			( !bOn && GetUILightState( aBtnLight2 )) )
		{
			SetUILightState( aBtnLight2, bOn );
			bUpdate = TRUE;
		}
		if( aBtnLight2.GetState() == STATE_DONTKNOW )
			aBtnLight2.Check( aBtnLight2.IsChecked() );
	}
	else
	{
		if( aBtnLight2.GetState() != STATE_DONTKNOW )
		{
			aBtnLight2.SetState( STATE_DONTKNOW );
			bUpdate = TRUE;
		}
	}
	// Licht 2 (Richtung)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTDIRECTION_2);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bUpdate = TRUE;
	}

	// Licht 3 (Farbe)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTCOLOR_3);
	if( eState != SFX_ITEM_DONTCARE )
	{
		aColor = ((const Svx3DLightcolor3Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTCOLOR_3)).GetValue();
		ColorLB* pLb = &aLbLight3;
		if( aColor != pLb->GetSelectEntryColor() )
		{
			LBSelectColor( pLb, aColor );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aLbLight3.GetSelectEntryCount() != 0 )
		{
			aLbLight3.SetNoSelection();
			bUpdate = TRUE;
		}
	}
	// Licht 3 (an/aus)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTON_3);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bool bOn = ((const Svx3DLightOnOff3Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTON_3)).GetValue() != 0;
		if( ( bOn && !GetUILightState( aBtnLight3)) ||
			( !bOn && GetUILightState( aBtnLight3)) )
		{
			SetUILightState( aBtnLight3, bOn );
			bUpdate = TRUE;
		}
		if( aBtnLight3.GetState() == STATE_DONTKNOW )
			aBtnLight3.Check( aBtnLight3.IsChecked() );
	}
	else
	{
		if( aBtnLight3.GetState() != STATE_DONTKNOW )
		{
			aBtnLight3.SetState( STATE_DONTKNOW );
			bUpdate = TRUE;
		}
	}
	// Licht 3 (Richtung)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTDIRECTION_3);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bUpdate = TRUE;
	}

	// Licht 4 (Farbe)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTCOLOR_4);
	if( eState != SFX_ITEM_DONTCARE )
	{
		aColor = ((const Svx3DLightcolor4Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTCOLOR_4)).GetValue();
		ColorLB* pLb = &aLbLight4;
		if( aColor != pLb->GetSelectEntryColor() )
		{
			LBSelectColor( pLb, aColor );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aLbLight4.GetSelectEntryCount() != 0 )
		{
			aLbLight4.SetNoSelection();
			bUpdate = TRUE;
		}
	}
	// Licht 4 (an/aus)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTON_4);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bool bOn = ((const Svx3DLightOnOff4Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTON_4)).GetValue() != 0;
		if( ( bOn && !GetUILightState( aBtnLight4 )) ||
			( !bOn && GetUILightState( aBtnLight4 )) )
		{
			SetUILightState( aBtnLight4, bOn );
			bUpdate = TRUE;
		}
		if( aBtnLight4.GetState() == STATE_DONTKNOW )
			aBtnLight4.Check( aBtnLight4.IsChecked() );
	}
	else
	{
		if( aBtnLight4.GetState() != STATE_DONTKNOW )
		{
			aBtnLight4.SetState( STATE_DONTKNOW );
			bUpdate = TRUE;
		}
	}
	// Licht 4 (Richtung)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTDIRECTION_4);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bUpdate = TRUE;
	}

	// Licht 5 (Farbe)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTCOLOR_5);
	if( eState != SFX_ITEM_DONTCARE )
	{
		aColor = ((const Svx3DLightcolor5Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTCOLOR_5)).GetValue();
		ColorLB* pLb = &aLbLight5;
		if( aColor != pLb->GetSelectEntryColor() )
		{
			LBSelectColor( pLb, aColor );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aLbLight5.GetSelectEntryCount() != 0 )
		{
			aLbLight5.SetNoSelection();
			bUpdate = TRUE;
		}
	}
	// Licht 5 (an/aus)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTON_5);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bool bOn = ((const Svx3DLightOnOff5Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTON_5)).GetValue() != 0;
		if( ( bOn && !GetUILightState( aBtnLight5 )) ||
			( !bOn && GetUILightState( aBtnLight5 )) )
		{
			SetUILightState( aBtnLight5, bOn );
			bUpdate = TRUE;
		}
		if( aBtnLight5.GetState() == STATE_DONTKNOW )
			aBtnLight5.Check( aBtnLight5.IsChecked() );
	}
	else
	{
		if( aBtnLight5.GetState() != STATE_DONTKNOW )
		{
			aBtnLight5.SetState( STATE_DONTKNOW );
			bUpdate = TRUE;
		}
	}
	// Licht 5 (Richtung)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTDIRECTION_5);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bUpdate = TRUE;
	}

	// Licht 6 (Farbe)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTCOLOR_6);
	if( eState != SFX_ITEM_DONTCARE )
	{
		aColor = ((const Svx3DLightcolor6Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTCOLOR_6)).GetValue();
		ColorLB* pLb = &aLbLight6;
		if( aColor != pLb->GetSelectEntryColor() )
		{
			LBSelectColor( pLb, aColor );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aLbLight6.GetSelectEntryCount() != 0 )
		{
			aLbLight6.SetNoSelection();
			bUpdate = TRUE;
		}
	}
	// Licht 6 (an/aus)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTON_6);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bool bOn = ((const Svx3DLightOnOff6Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTON_6)).GetValue() != 0;
		if( ( bOn && !GetUILightState( aBtnLight6 )) ||
			( !bOn && GetUILightState( aBtnLight6 )) )
		{
			SetUILightState( aBtnLight6, bOn );
			bUpdate = TRUE;
		}
		if( aBtnLight6.GetState() == STATE_DONTKNOW )
			aBtnLight6.Check( aBtnLight6.IsChecked() );
	}
	else
	{
		if( aBtnLight6.GetState() != STATE_DONTKNOW )
		{
			aBtnLight6.SetState( STATE_DONTKNOW );
			bUpdate = TRUE;
		}
	}
	// Licht 6 (Richtung)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTDIRECTION_6);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bUpdate = TRUE;
	}

	// Licht 7 (Farbe)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTCOLOR_7);
	if( eState != SFX_ITEM_DONTCARE )
	{
		aColor = ((const Svx3DLightcolor7Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTCOLOR_7)).GetValue();
		ColorLB* pLb = &aLbLight7;
		if( aColor != pLb->GetSelectEntryColor() )
		{
			LBSelectColor( pLb, aColor );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aLbLight7.GetSelectEntryCount() != 0 )
		{
			aLbLight7.SetNoSelection();
			bUpdate = TRUE;
		}
	}
	// Licht 7 (an/aus)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTON_7);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bool bOn = ((const Svx3DLightOnOff7Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTON_7)).GetValue() != 0;
		if( ( bOn && !GetUILightState( aBtnLight7 )) ||
			( !bOn && GetUILightState( aBtnLight7 )) )
		{
			SetUILightState( aBtnLight7	, bOn );
			bUpdate = TRUE;
		}
		if( aBtnLight7.GetState() == STATE_DONTKNOW )
			aBtnLight7.Check( aBtnLight7.IsChecked() );
	}
	else
	{
		if( aBtnLight7.GetState() != STATE_DONTKNOW )
		{
			aBtnLight7.SetState( STATE_DONTKNOW );
			bUpdate = TRUE;
		}
	}
	// Licht 7 (Richtung)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTDIRECTION_7);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bUpdate = TRUE;
	}

	// Licht 8 (Farbe)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTCOLOR_8);
	if( eState != SFX_ITEM_DONTCARE )
	{
		aColor = ((const Svx3DLightcolor8Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTCOLOR_8)).GetValue();
		ColorLB* pLb = &aLbLight8;
		if( aColor != pLb->GetSelectEntryColor() )
		{
			LBSelectColor( pLb, aColor );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aLbLight8.GetSelectEntryCount() != 0 )
		{
			aLbLight8.SetNoSelection();
			bUpdate = TRUE;
		}
	}
	// Licht 8 (an/aus)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTON_8);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bool bOn = ((const Svx3DLightOnOff8Item&)rAttrs.Get(SDRATTR_3DSCENE_LIGHTON_8)).GetValue() != 0;
		if( ( bOn && !GetUILightState( aBtnLight8 )) ||
			( !bOn && GetUILightState( aBtnLight8 )) )
		{
			SetUILightState( aBtnLight8, bOn );
			bUpdate = TRUE;
		}
		if( aBtnLight8.GetState() == STATE_DONTKNOW )
			aBtnLight8.Check( aBtnLight8.IsChecked() );
	}
	else
	{
		if( aBtnLight8.GetState() != STATE_DONTKNOW )
		{
			aBtnLight8.SetState( STATE_DONTKNOW );
			bUpdate = TRUE;
		}
	}
	// Licht 8 (Richtung)
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_LIGHTDIRECTION_8);
	if( eState != SFX_ITEM_DONTCARE )
	{
		bUpdate = TRUE;
	}

	// Umgebungslicht
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_AMBIENTCOLOR);
	if( eState != SFX_ITEM_DONTCARE )
	{
		aColor = ((const Svx3DAmbientcolorItem&)rAttrs.Get(SDRATTR_3DSCENE_AMBIENTCOLOR)).GetValue();
		ColorLB* pLb = &aLbAmbientlight;
		if( aColor != pLb->GetSelectEntryColor() )
		{
			LBSelectColor( pLb, aColor );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aLbAmbientlight.GetSelectEntryCount() != 0 )
		{
			aLbAmbientlight.SetNoSelection();
			bUpdate = TRUE;
		}
	}


// Texturen
	// Art
	if( bBitmap )
	{
		eState = rAttrs.GetItemState(SDRATTR_3DOBJ_TEXTURE_KIND);
		if( eState != SFX_ITEM_DONTCARE )
		{
			UINT16 nValue = ((const Svx3DTextureKindItem&)rAttrs.Get(SDRATTR_3DOBJ_TEXTURE_KIND)).GetValue();

			if( ( !aBtnTexLuminance.IsChecked() && nValue == 1 ) ||
				( !aBtnTexColor.IsChecked() && nValue == 3 ) )
			{
				aBtnTexLuminance.Check( nValue == 1 );
				aBtnTexColor.Check( nValue == 3 );
				bUpdate = TRUE;
			}
		}
		else
		{
			if( aBtnTexLuminance.IsChecked() ||
				aBtnTexColor.IsChecked() )
			{
				aBtnTexLuminance.Check( FALSE );
				aBtnTexColor.Check( FALSE );
				bUpdate = TRUE;
			}
		}

		// Modus
		eState = rAttrs.GetItemState(SDRATTR_3DOBJ_TEXTURE_MODE);
		if( eState != SFX_ITEM_DONTCARE )
		{
			UINT16 nValue = ((const Svx3DTextureModeItem&)rAttrs.Get(SDRATTR_3DOBJ_TEXTURE_MODE)).GetValue();

			if( ( !aBtnTexReplace.IsChecked() && nValue == 1 ) ||
				( !aBtnTexModulate.IsChecked() && nValue == 2 ) )
			{
				aBtnTexReplace.Check( nValue == 1 );
				aBtnTexModulate.Check( nValue == 2 );
				//aBtnTexBlend.Check( nValue == 2 );
				bUpdate = TRUE;
			}
		}
		else
		{
			if( aBtnTexReplace.IsChecked() ||
				aBtnTexModulate.IsChecked() )
			{
				aBtnTexReplace.Check( FALSE );
				aBtnTexModulate.Check( FALSE );
				//aBtnTexBlend.Check( FALSE );
				bUpdate = TRUE;
			}
		}

		// Projektion X
		eState = rAttrs.GetItemState(SDRATTR_3DOBJ_TEXTURE_PROJ_X);
		if( eState != SFX_ITEM_DONTCARE )
		{
			UINT16 nValue = ((const Svx3DTextureProjectionXItem&)rAttrs.Get(SDRATTR_3DOBJ_TEXTURE_PROJ_X)).GetValue();

			if( ( !aBtnTexObjectX.IsChecked() && nValue == 0 ) ||
				( !aBtnTexParallelX.IsChecked() && nValue == 1 ) ||
				( !aBtnTexCircleX.IsChecked() && nValue == 2 ) )
			{
				aBtnTexObjectX.Check( nValue == 0 );
				aBtnTexParallelX.Check( nValue == 1 );
				aBtnTexCircleX.Check( nValue == 2 );
				bUpdate = TRUE;
			}
		}
		else
		{
			if( aBtnTexObjectX.IsChecked() ||
				aBtnTexParallelX.IsChecked() ||
				aBtnTexCircleX.IsChecked() )
			{
				aBtnTexObjectX.Check( FALSE );
				aBtnTexParallelX.Check( FALSE );
				aBtnTexCircleX.Check( FALSE );
				bUpdate = TRUE;
			}
		}

		// Projektion Y
		eState = rAttrs.GetItemState(SDRATTR_3DOBJ_TEXTURE_PROJ_Y);
		if( eState != SFX_ITEM_DONTCARE )
		{
			UINT16 nValue = ((const Svx3DTextureProjectionYItem&)rAttrs.Get(SDRATTR_3DOBJ_TEXTURE_PROJ_Y)).GetValue();

			if( ( !aBtnTexObjectY.IsChecked() && nValue == 0 ) ||
				( !aBtnTexParallelY.IsChecked() && nValue == 1 ) ||
				( !aBtnTexCircleY.IsChecked() && nValue == 2 ) )
			{
				aBtnTexObjectY.Check( nValue == 0 );
				aBtnTexParallelY.Check( nValue == 1 );
				aBtnTexCircleY.Check( nValue == 2 );
				bUpdate = TRUE;
			}
		}
		else
		{
			if( aBtnTexObjectY.IsChecked() ||
				aBtnTexParallelY.IsChecked() ||
				aBtnTexCircleY.IsChecked() )
			{
				aBtnTexObjectY.Check( FALSE );
				aBtnTexParallelY.Check( FALSE );
				aBtnTexCircleY.Check( FALSE );
				bUpdate = TRUE;
			}
		}

		// Filter
		eState = rAttrs.GetItemState(SDRATTR_3DOBJ_TEXTURE_FILTER);
		if( eState != SFX_ITEM_DONTCARE )
		{
			BOOL bValue = ((const Svx3DTextureFilterItem&)rAttrs.Get(SDRATTR_3DOBJ_TEXTURE_FILTER)).GetValue();
			if( bValue != aBtnTexFilter.IsChecked() )
			{
				aBtnTexFilter.Check( bValue );
				bUpdate = TRUE;
			}
			if( aBtnTexFilter.GetState() == STATE_DONTKNOW )
				aBtnTexFilter.Check( bValue );
		}
		else
		{
			if( aBtnTexFilter.GetState() != STATE_DONTKNOW )
			{
				aBtnTexFilter.SetState( STATE_DONTKNOW );
				bUpdate = TRUE;
			}
		}
	}


	// Material Favoriten
	aLbMatFavorites.SelectEntryPos( 0 );

	// Objektfarbe
	eState = rAttrs.GetItemState(XATTR_FILLCOLOR);
	if( eState != SFX_ITEM_DONTCARE )
	{
		aColor = ((const XFillColorItem&)rAttrs.Get(XATTR_FILLCOLOR)).GetColorValue();
		ColorLB* pLb = &aLbMatColor;
		if( aColor != pLb->GetSelectEntryColor() )
		{
			LBSelectColor( pLb, aColor );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aLbMatColor.GetSelectEntryCount() != 0 )
		{
			aLbMatColor.SetNoSelection();
			bUpdate = TRUE;
		}
	}

	// Slebstleuchtfarbe
	eState = rAttrs.GetItemState(SDRATTR_3DOBJ_MAT_EMISSION);
	if( eState != SFX_ITEM_DONTCARE )
	{
		aColor = ((const Svx3DMaterialEmissionItem&)rAttrs.Get(SDRATTR_3DOBJ_MAT_EMISSION)).GetValue();
		ColorLB* pLb = &aLbMatEmission;
		if( aColor != pLb->GetSelectEntryColor() )
		{
			LBSelectColor( pLb, aColor );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aLbMatEmission.GetSelectEntryCount() != 0 )
		{
			aLbMatEmission.SetNoSelection();
			bUpdate = TRUE;
		}
	}

	// Glanzpunkt
	eState = rAttrs.GetItemState(SDRATTR_3DOBJ_MAT_SPECULAR);
	if( eState != SFX_ITEM_DONTCARE )
	{
		aColor = ((const Svx3DMaterialSpecularItem&)rAttrs.Get(SDRATTR_3DOBJ_MAT_SPECULAR)).GetValue();
		ColorLB* pLb = &aLbMatSpecular;
		if( aColor != pLb->GetSelectEntryColor() )
		{
			LBSelectColor( pLb, aColor );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( aLbMatSpecular.GetSelectEntryCount() != 0 )
		{
			aLbMatSpecular.SetNoSelection();
			bUpdate = TRUE;
		}
	}

	// Glanzpunkt Intensitaet
	eState = rAttrs.GetItemState(SDRATTR_3DOBJ_MAT_SPECULAR_INTENSITY);
	if( eState != SFX_ITEM_DONTCARE )
	{
		UINT16 nValue = ((const Svx3DMaterialSpecularIntensityItem&)rAttrs.Get(SDRATTR_3DOBJ_MAT_SPECULAR_INTENSITY)).GetValue();
		if( nValue != aMtrMatSpecularIntensity.GetValue() )
		{
			aMtrMatSpecularIntensity.SetValue( nValue );
			bUpdate = TRUE;
		}
	}
	else
	{
		if( !aMtrMatSpecularIntensity.IsEmptyFieldValue() )
		{
			aMtrMatSpecularIntensity.SetEmptyFieldValue();
			bUpdate = TRUE;
		}
	}


// Sonstige
	// Perspektive
	eState = rAttrs.GetItemState(SDRATTR_3DSCENE_PERSPECTIVE);
	if( eState != SFX_ITEM_DONTCARE )
	{
		ProjectionType ePT = (ProjectionType)((const Svx3DPerspectiveItem&)rAttrs.Get(SDRATTR_3DSCENE_PERSPECTIVE)).GetValue();
		if( ( !aBtnPerspective.IsChecked() && ePT == PR_PERSPECTIVE ) ||
			( aBtnPerspective.IsChecked() && ePT == PR_PARALLEL ) )
		{
			aBtnPerspective.Check( ePT == PR_PERSPECTIVE );
			bUpdate = TRUE;
		}
		if( aBtnPerspective.GetState() == STATE_DONTKNOW )
			aBtnPerspective.Check( ePT == PR_PERSPECTIVE );
	}
	else
	{
		if( aBtnPerspective.GetState() != STATE_DONTKNOW )
		{
			aBtnPerspective.SetState( STATE_DONTKNOW );
			bUpdate = TRUE;
		}
	}

	if( !bUpdate && !bOnly3DChanged )
	{
		// Eventuell sind aber die 2D-Attribute unterschiedlich. Vergleiche
		// diese und entscheide


		bUpdate = TRUE;
	}

	if( bUpdate || bOnly3DChanged )
	{
		// Preview updaten
		SfxItemSet aSet(rAttrs);

		// set LineStyle hard to XLINE_NONE when it's not set so that
		// the default (XLINE_SOLID) is not used for 3d preview
		if(SFX_ITEM_SET != aSet.GetItemState(XATTR_LINESTYLE, FALSE))
			aSet.Put(XLineStyleItem(XLINE_NONE));

		// set FillColor hard to WHITE when it's SFX_ITEM_DONTCARE so that
		// the default (Blue7) is not used for 3d preview
		if(SFX_ITEM_DONTCARE == aSet.GetItemState(XATTR_FILLCOLOR, FALSE))
			aSet.Put(XFillColorItem(String(), Color(COL_WHITE)));

		aCtlPreview.Set3DAttributes(aSet);
		aCtlLightPreview.GetSvx3DLightControl().Set3DAttributes(aSet);

        // try to select light corresponding to active button
        sal_uInt32 nNumber(0xffffffff);

        if(aBtnLight1.IsChecked())
            nNumber = 0;
        else if(aBtnLight2.IsChecked())
            nNumber = 1;
        else if(aBtnLight3.IsChecked())
            nNumber = 2;
        else if(aBtnLight4.IsChecked())
            nNumber = 3;
        else if(aBtnLight5.IsChecked())
            nNumber = 4;
        else if(aBtnLight6.IsChecked())
            nNumber = 5;
        else if(aBtnLight7.IsChecked())
            nNumber = 6;
        else if(aBtnLight8.IsChecked())
            nNumber = 7;

        if(nNumber != 0xffffffff)
        {
    		aCtlLightPreview.GetSvx3DLightControl().SelectLight(nNumber);
        }
	}

	// handle state of converts possible
	aBtnConvertTo3D.Enable(pConvertTo3DItem->GetState());
	aBtnLatheObject.Enable(pConvertTo3DLatheItem->GetState());
}

// -----------------------------------------------------------------------
void Svx3DWin::GetAttr( SfxItemSet& rAttrs )
{
	// get remembered 2d attributes from the dialog
	if(mpRemember2DAttributes)
	{
		SfxWhichIter aIter(*mpRemember2DAttributes);
		sal_uInt16 nWhich(aIter.FirstWhich());

		while(nWhich)
		{
			SfxItemState eState = mpRemember2DAttributes->GetItemState(nWhich, FALSE);
			if(SFX_ITEM_DONTCARE == eState)
				rAttrs.InvalidateItem(nWhich);
			else if(SFX_ITEM_SET == eState)
				rAttrs.Put(mpRemember2DAttributes->Get(nWhich, FALSE));

			nWhich = aIter.NextWhich();
		}
	}

// Sonstige, muss vorne stehen da auf allen Seiten
	// Perspektive
	if( aBtnPerspective.GetState() != STATE_DONTKNOW )
	{
		UINT16 nValue;
		if( aBtnPerspective.IsChecked() )
			nValue = PR_PERSPECTIVE;
		else
			nValue = PR_PARALLEL;
		rAttrs.Put(Svx3DPerspectiveItem(nValue));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_PERSPECTIVE);

// Geometrie
	// evtl. PoolUnit ermitteln (Falls dies in Update() nicht passiert ist)
	if( !mpImpl->pPool )
	{
		DBG_ERROR( "Kein Pool in GetAttr()! Evtl. inkompatibel zu drviewsi.cxx ?" );
		mpImpl->pPool = rAttrs.GetPool();
		DBG_ASSERT( mpImpl->pPool, "Wo ist der Pool?" );
		ePoolUnit = mpImpl->pPool->GetMetric( SID_ATTR_LINE_WIDTH );

		eFUnit = GetModuleFieldUnit( &rAttrs );
	}

	// Anzahl Segmente (horizontal)
	if( !aNumHorizontal.IsEmptyFieldValue() )
	{
		sal_uInt32 nValue = static_cast<sal_uInt32>(aNumHorizontal.GetValue());
		rAttrs.Put(Svx3DHorizontalSegmentsItem(nValue));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_HORZ_SEGS);

	// Anzahl Segmente (vertikal)
	if( !aNumVertical.IsEmptyFieldValue() )
	{
		UINT32 nValue = static_cast<UINT32>(aNumVertical.GetValue());
		rAttrs.Put(Svx3DVerticalSegmentsItem(nValue));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_VERT_SEGS);

	// Tiefe
	if( !aMtrDepth.IsEmptyFieldValue() )
	{
		UINT32 nValue = GetCoreValue( aMtrDepth, ePoolUnit );
		rAttrs.Put(Svx3DDepthItem(nValue));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_DEPTH);

	// Doppelseitig
	TriState eState = aBtnDoubleSided.GetState();
	if( eState != STATE_DONTKNOW )
	{
		BOOL bValue = STATE_CHECK == eState;
		rAttrs.Put(Svx3DDoubleSidedItem(bValue));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_DOUBLE_SIDED);

	// Kantenrundung
	if( !aMtrPercentDiagonal.IsEmptyFieldValue() )
	{
		UINT16 nValue = (UINT16) aMtrPercentDiagonal.GetValue();
		rAttrs.Put(Svx3DPercentDiagonalItem(nValue));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_PERCENT_DIAGONAL);

	// Tiefenskalierung
	if( !aMtrBackscale.IsEmptyFieldValue() )
	{
		UINT16 nValue = (UINT16)aMtrBackscale.GetValue();
		rAttrs.Put(Svx3DBackscaleItem(nValue));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_BACKSCALE);

	// Endwinkel
	if( !aMtrEndAngle.IsEmptyFieldValue() )
	{
		UINT16 nValue = (UINT16)aMtrEndAngle.GetValue();
		rAttrs.Put(Svx3DEndAngleItem(nValue));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_END_ANGLE);

	// Normalentyp
	UINT16 nValue = 99;
	if( aBtnNormalsObj.IsChecked() )
		nValue = 0;
	else if( aBtnNormalsFlat.IsChecked() )
		nValue = 1;
	else if( aBtnNormalsSphere.IsChecked() )
		nValue = 2;

	if( nValue <= 2 )
		rAttrs.Put(Svx3DNormalsKindItem(nValue));
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_NORMALS_KIND);

	// Normalen invertieren
	eState = aBtnNormalsInvert.GetState();
	if( eState != STATE_DONTKNOW )
	{
		BOOL bValue = STATE_CHECK == eState;
		rAttrs.Put(Svx3DNormalsInvertItem(bValue));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_NORMALS_INVERT);

	// 2-seitige Beleuchtung
	eState = aBtnTwoSidedLighting.GetState();
	if( eState != STATE_DONTKNOW )
	{
		BOOL bValue = STATE_CHECK == eState;
		rAttrs.Put(Svx3DTwoSidedLightingItem(bValue));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_TWO_SIDED_LIGHTING);

// Darstellung
	// Shademode
	if( aLbShademode.GetSelectEntryCount() )
	{
		nValue = aLbShademode.GetSelectEntryPos();
		rAttrs.Put(Svx3DShadeModeItem(nValue));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_SHADE_MODE);

	// 3D-Shatten
	eState = aBtnShadow3d.GetState();
	if( eState != STATE_DONTKNOW )
	{
		BOOL bValue = STATE_CHECK == eState;
		rAttrs.Put(Svx3DShadow3DItem(bValue));
		rAttrs.Put(SdrShadowItem(bValue));
	}
	else
	{
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_SHADOW_3D);
		rAttrs.InvalidateItem(SDRATTR_SHADOW);
	}

	// Neigung (Schatten)
	if( !aMtrSlant.IsEmptyFieldValue() )
	{
		UINT16 nValue2 = (UINT16) aMtrSlant.GetValue();
		rAttrs.Put(Svx3DShadowSlantItem(nValue2));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_SHADOW_SLANT);

	// Distanz
	if( !aMtrDistance.IsEmptyFieldValue() )
	{
		UINT32 nValue2 = GetCoreValue( aMtrDistance, ePoolUnit );
		rAttrs.Put(Svx3DDistanceItem(nValue2));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_DISTANCE);

	// Brennweite
	if( !aMtrFocalLength.IsEmptyFieldValue() )
	{
		UINT32 nValue2 = GetCoreValue( aMtrFocalLength, ePoolUnit );
		rAttrs.Put(Svx3DFocalLengthItem(nValue2));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_FOCAL_LENGTH);

// Beleuchtung
	Image aImg;
	basegfx::B3DVector aVector;
	Color aColor;
    const SfxItemSet aLightItemSet(aCtlLightPreview.GetSvx3DLightControl().Get3DAttributes());

    // Licht 1 Farbe
	if( aLbLight1.GetSelectEntryCount() )
	{
		aColor = aLbLight1.GetSelectEntryColor();
		rAttrs.Put(Svx3DLightcolor1Item(aColor));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTCOLOR_1);
	// Licht 1 (an/aus)
	eState = aBtnLight1.GetState();
	if( eState != STATE_DONTKNOW )
	{
		BOOL bValue = GetUILightState( aBtnLight1 );
		rAttrs.Put(Svx3DLightOnOff1Item(bValue));

		// Licht 1 (Richtung)
		if( bValue )
		{
            rAttrs.Put(aLightItemSet.Get(SDRATTR_3DSCENE_LIGHTDIRECTION_1));
		}
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTON_1);


	// Licht 2 Farbe
	if( aLbLight2.GetSelectEntryCount() )
	{
		aColor = aLbLight2.GetSelectEntryColor();
		rAttrs.Put(Svx3DLightcolor2Item(aColor));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTCOLOR_2);
	// Licht 2 (an/aus)
	eState = aBtnLight2.GetState();
	if( eState != STATE_DONTKNOW )
	{
		BOOL bValue = GetUILightState( aBtnLight2 );
		rAttrs.Put(Svx3DLightOnOff2Item(bValue));

		// Licht 2 (Richtung)
		if( bValue )
		{
            rAttrs.Put(aLightItemSet.Get(SDRATTR_3DSCENE_LIGHTDIRECTION_2));
		}
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTON_2);

	// Licht 3 Farbe
	if( aLbLight3.GetSelectEntryCount() )
	{
		aColor = aLbLight3.GetSelectEntryColor();
		rAttrs.Put(Svx3DLightcolor3Item(aColor));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTCOLOR_3);
	// Licht 3 (an/aus)
	eState = aBtnLight3.GetState();
	if( eState != STATE_DONTKNOW )
	{
		BOOL bValue = GetUILightState( aBtnLight3 );
		rAttrs.Put(Svx3DLightOnOff3Item(bValue));

		// Licht 3 (Richtung)
		if( bValue )
		{
            rAttrs.Put(aLightItemSet.Get(SDRATTR_3DSCENE_LIGHTDIRECTION_3));
		}
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTON_3);

	// Licht 4 Farbe
	if( aLbLight4.GetSelectEntryCount() )
	{
		aColor = aLbLight4.GetSelectEntryColor();
		rAttrs.Put(Svx3DLightcolor4Item(aColor));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTCOLOR_4);
	// Licht 4 (an/aus)
	eState = aBtnLight4.GetState();
	if( eState != STATE_DONTKNOW )
	{
		BOOL bValue = GetUILightState( aBtnLight4 );
		rAttrs.Put(Svx3DLightOnOff4Item(bValue));

		// Licht 4 (Richtung)
		if( bValue )
		{
            rAttrs.Put(aLightItemSet.Get(SDRATTR_3DSCENE_LIGHTDIRECTION_4));
		}
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTON_4);

	// Licht 5 Farbe
	if( aLbLight5.GetSelectEntryCount() )
	{
		aColor = aLbLight5.GetSelectEntryColor();
		rAttrs.Put(Svx3DLightcolor5Item(aColor));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTCOLOR_5);
	// Licht 5 (an/aus)
	eState = aBtnLight5.GetState();
	if( eState != STATE_DONTKNOW )
	{
		BOOL bValue = GetUILightState( aBtnLight5 );
		rAttrs.Put(Svx3DLightOnOff5Item(bValue));

		// Licht 5 (Richtung)
		if( bValue )
		{
            rAttrs.Put(aLightItemSet.Get(SDRATTR_3DSCENE_LIGHTDIRECTION_5));
		}
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTON_5);

	// Licht 6 Farbe
	if( aLbLight6.GetSelectEntryCount() )
	{
		aColor = aLbLight6.GetSelectEntryColor();
		rAttrs.Put(Svx3DLightcolor6Item(aColor));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTCOLOR_6);
	// Licht 6 (an/aus)
	eState = aBtnLight6.GetState();
	if( eState != STATE_DONTKNOW )
	{
		BOOL bValue = GetUILightState( aBtnLight6 );
		rAttrs.Put(Svx3DLightOnOff6Item(bValue));

		// Licht 6 (Richtung)
		if( bValue )
		{
            rAttrs.Put(aLightItemSet.Get(SDRATTR_3DSCENE_LIGHTDIRECTION_6));
		}
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTON_6);

	// Licht 7 Farbe
	if( aLbLight7.GetSelectEntryCount() )
	{
		aColor = aLbLight7.GetSelectEntryColor();
		rAttrs.Put(Svx3DLightcolor7Item(aColor));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTCOLOR_7);
	// Licht 7 (an/aus)
	eState = aBtnLight7.GetState();
	if( eState != STATE_DONTKNOW )
	{
		BOOL bValue = GetUILightState( aBtnLight7 );
		rAttrs.Put(Svx3DLightOnOff7Item(bValue));

		// Licht 7 (Richtung)
		if( bValue )
		{
            rAttrs.Put(aLightItemSet.Get(SDRATTR_3DSCENE_LIGHTDIRECTION_7));
		}
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTON_7);

	// Licht 8 Farbe
	if( aLbLight8.GetSelectEntryCount() )
	{
		aColor = aLbLight8.GetSelectEntryColor();
		rAttrs.Put(Svx3DLightcolor8Item(aColor));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTCOLOR_8);
	// Licht 8 (an/aus)
	eState = aBtnLight8.GetState();
	if( eState != STATE_DONTKNOW )
	{
		BOOL bValue = GetUILightState( aBtnLight8 );
		rAttrs.Put(Svx3DLightOnOff8Item(bValue));

		// Licht 8 (Richtung)
		if( bValue )
		{
            rAttrs.Put(aLightItemSet.Get(SDRATTR_3DSCENE_LIGHTDIRECTION_8));
		}
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_LIGHTON_8);

	// Umgebungslicht
	if( aLbAmbientlight.GetSelectEntryCount() )
	{
		aColor = aLbAmbientlight.GetSelectEntryColor();
		rAttrs.Put(Svx3DAmbientcolorItem(aColor));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DSCENE_AMBIENTCOLOR);

// Texturen
	// Art
	nValue = 3;
	if( aBtnTexLuminance.IsChecked() )
		nValue = 1;
	else if( aBtnTexColor.IsChecked() )
		nValue = 3;

	if( nValue == 1 || nValue == 3 )
		rAttrs.Put(Svx3DTextureKindItem(nValue));
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_TEXTURE_KIND);


	// Modus
	nValue = 99;
	if( aBtnTexReplace.IsChecked() )
		nValue = 1;
	else if( aBtnTexModulate.IsChecked() )
		nValue = 2;
	//else if( aBtnTexBlend.IsChecked() )
	//	nValue = 2;

	if( nValue == 1 || nValue == 2 )
		rAttrs.Put(Svx3DTextureModeItem(nValue));
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_TEXTURE_MODE);

	// Projektion X
	nValue = 99;
	if( aBtnTexObjectX.IsChecked() )
		nValue = 0;
	else if( aBtnTexParallelX.IsChecked() )
		nValue = 1;
	else if( aBtnTexCircleX.IsChecked() )
		nValue = 2;

	if( nValue <= 2 )
		rAttrs.Put(Svx3DTextureProjectionXItem(nValue));
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_TEXTURE_PROJ_X);

	// Projektion Y
	nValue = 99;
	if( aBtnTexObjectY.IsChecked() )
		nValue = 0;
	else if( aBtnTexParallelY.IsChecked() )
		nValue = 1;
	else if( aBtnTexCircleY.IsChecked() )
		nValue = 2;

	if( nValue <= 2 )
		rAttrs.Put(Svx3DTextureProjectionYItem(nValue));
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_TEXTURE_PROJ_Y);


	// Filter
	eState = aBtnTexFilter.GetState();
	if( eState != STATE_DONTKNOW )
	{
		BOOL bValue = STATE_CHECK == eState;
		rAttrs.Put(Svx3DTextureFilterItem(bValue));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_TEXTURE_FILTER);


// Material
	// Objektfarbe
	if( aLbMatColor.GetSelectEntryCount() )
	{
		aColor = aLbMatColor.GetSelectEntryColor();
		rAttrs.Put( XFillColorItem( String(), aColor) );
	}
	else
	{
		rAttrs.InvalidateItem( XATTR_FILLCOLOR );
	}

	// Slebstleuchtfarbe
	if( aLbMatEmission.GetSelectEntryCount() )
	{
		aColor = aLbMatEmission.GetSelectEntryColor();
		rAttrs.Put(Svx3DMaterialEmissionItem(aColor));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_MAT_EMISSION);

	// Glanzpunkt
	if( aLbMatSpecular.GetSelectEntryCount() )
	{
		aColor = aLbMatSpecular.GetSelectEntryColor();
		rAttrs.Put(Svx3DMaterialSpecularItem(aColor));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_MAT_SPECULAR);

	// Glanzpunkt Intensitaet
	if( !aMtrMatSpecularIntensity.IsEmptyFieldValue() )
	{
		UINT16 nValue2 = (UINT16) aMtrMatSpecularIntensity.GetValue();
		rAttrs.Put(Svx3DMaterialSpecularIntensityItem(nValue2));
	}
	else
		rAttrs.InvalidateItem(SDRATTR_3DOBJ_MAT_SPECULAR_INTENSITY);
}

// -----------------------------------------------------------------------
void __EXPORT Svx3DWin::Resize()
{
	if ( !IsFloatingMode() ||
		 !GetFloatingWindow()->IsRollUp() )
	{
		Size aWinSize( GetOutputSizePixel() ); // vorher rSize im Resizing()

		if( aWinSize.Height() >= GetMinOutputSizePixel().Height() &&
			aWinSize.Width() >= GetMinOutputSizePixel().Width() )
		{
			Size aDiffSize;
			aDiffSize.Width() = aWinSize.Width() - aSize.Width();
			aDiffSize.Height() = aWinSize.Height() - aSize.Height();

			Point aXPt;
			Point aYPt;
			aXPt.X() = aDiffSize.Width();
			aYPt.Y() = aDiffSize.Height();

			Size aObjSize;

			// Hide
			aBtnUpdate.Hide();
			aBtnAssign.Hide();

			aBtnConvertTo3D.Hide();
			aBtnLatheObject.Hide();
			aBtnPerspective.Hide();

			aCtlPreview.Hide();
			aCtlLightPreview.Hide();

            aFLGeometrie.Hide();
            aFLRepresentation.Hide();
            aFLLight.Hide();
            aFLTexture.Hide();
            aFLMaterial.Hide();

			// Verschieben / Resizen
			aBtnUpdate.SetPosPixel( aBtnUpdate.GetPosPixel() + aXPt );
			aBtnAssign.SetPosPixel( aBtnAssign.GetPosPixel() + aXPt );

				// Preview-Controls
			aObjSize = aCtlPreview.GetOutputSizePixel();
			aObjSize.Width() += aDiffSize.Width();
			aObjSize.Height() += aDiffSize.Height();
			aCtlPreview.SetOutputSizePixel( aObjSize );
			aCtlLightPreview.SetOutputSizePixel( aObjSize );

            // Groups
            aObjSize = aFLGeometrie.GetOutputSizePixel();
			aObjSize.Width() += aDiffSize.Width();
            aFLGeometrie.SetOutputSizePixel( aObjSize );
            aFLSegments.SetOutputSizePixel( aObjSize );
            aFLShadow.SetOutputSizePixel( aObjSize );
            aFLCamera.SetOutputSizePixel( aObjSize );
            aFLRepresentation.SetOutputSizePixel( aObjSize );
            aFLLight.SetOutputSizePixel( aObjSize );
            aFLTexture.SetOutputSizePixel( aObjSize );
            aFLMaterial.SetOutputSizePixel( aObjSize );

				// Y-Position der unteren Buttons
			aBtnConvertTo3D.SetPosPixel( aBtnConvertTo3D.GetPosPixel() + aYPt );
			aBtnLatheObject.SetPosPixel( aBtnLatheObject.GetPosPixel() + aYPt );
			aBtnPerspective.SetPosPixel( aBtnPerspective.GetPosPixel() + aYPt );

			// Show
			aBtnUpdate.Show();
			aBtnAssign.Show();

			aBtnConvertTo3D.Show();
			aBtnLatheObject.Show();
			aBtnPerspective.Show();

			if( aBtnGeo.IsChecked() )
				ClickViewTypeHdl( &aBtnGeo );
			if( aBtnRepresentation.IsChecked() )
				ClickViewTypeHdl( &aBtnRepresentation );
			if( aBtnLight.IsChecked() )
				ClickViewTypeHdl( &aBtnLight );
			if( aBtnTexture.IsChecked() )
				ClickViewTypeHdl( &aBtnTexture );
			if( aBtnMaterial.IsChecked() )
				ClickViewTypeHdl( &aBtnMaterial );

			aSize = aWinSize;
		}
	}

	SfxDockingWindow::Resize();
}

// -----------------------------------------------------------------------
IMPL_LINK( Svx3DWin, ClickUpdateHdl, void *, EMPTYARG )
{
	bUpdate = !aBtnUpdate.IsChecked();
	aBtnUpdate.Check( bUpdate );

	if( bUpdate )
	{
        SfxDispatcher* pDispatcher = LocalGetDispatcher(pBindings);
        if (pDispatcher != NULL)
        {
            SfxBoolItem aItem( SID_3D_STATE, TRUE );
            pDispatcher->Execute(
                SID_3D_STATE, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD, &aItem, 0L );
        }
	}
	else
	{
		// Controls koennen u.U. disabled sein
	}

	return( 0L );
}

// -----------------------------------------------------------------------
IMPL_LINK( Svx3DWin, ClickAssignHdl, void *, EMPTYARG )
{
    SfxDispatcher* pDispatcher = LocalGetDispatcher(pBindings);
    if (pDispatcher != NULL)
    {
        SfxBoolItem aItem( SID_3D_ASSIGN, TRUE );
        pDispatcher->Execute(
            SID_3D_ASSIGN, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD, &aItem, 0L );
    }

	return( 0L );
}

// -----------------------------------------------------------------------

IMPL_LINK( Svx3DWin, ClickViewTypeHdl, void *, pBtn )
{

	if( pBtn )
	{
		// Da das permanente Updaten der Preview zu teuer waere
		BOOL bUpdatePreview = aBtnLight.IsChecked();

		aBtnGeo.Check( &aBtnGeo	== pBtn );
		aBtnRepresentation.Check( &aBtnRepresentation == pBtn );
		aBtnLight.Check( &aBtnLight	== pBtn );
		aBtnTexture.Check( &aBtnTexture	== pBtn );
		aBtnMaterial.Check( &aBtnMaterial == pBtn );

		if( aBtnGeo.IsChecked() )
			eViewType = VIEWTYPE_GEO;
		if( aBtnRepresentation.IsChecked() )
			eViewType = VIEWTYPE_REPRESENTATION;
		if( aBtnLight.IsChecked() )
			eViewType = VIEWTYPE_LIGHT;
		if( aBtnTexture.IsChecked() )
			eViewType = VIEWTYPE_TEXTURE;
		if( aBtnMaterial.IsChecked() )
			eViewType = VIEWTYPE_MATERIAL;

		// Geometrie
		if( eViewType == VIEWTYPE_GEO )
		{
			aFtHorizontal.Show();
			aNumHorizontal.Show();
			aFtVertical.Show();
			aNumVertical.Show();
            aFLSegments.Show();
			aFtPercentDiagonal.Show();
			aMtrPercentDiagonal.Show();
			aFtBackscale.Show();
			aMtrBackscale.Show();
			aFtEndAngle.Show();
			aMtrEndAngle.Show();
			aFtDepth.Show();
			aMtrDepth.Show();
            aFLGeometrie.Show();

			aBtnNormalsObj.Show();
			aBtnNormalsFlat.Show();
			aBtnNormalsSphere.Show();
			aBtnTwoSidedLighting.Show();
			aBtnNormalsInvert.Show();
            aFLNormals.Show();
			aBtnDoubleSided.Show();
		}
		else
		{
			aFtHorizontal.Hide();
			aNumHorizontal.Hide();
			aFtVertical.Hide();
			aNumVertical.Hide();
            aFLSegments.Hide();
			aFtPercentDiagonal.Hide();
			aMtrPercentDiagonal.Hide();
			aFtBackscale.Hide();
			aMtrBackscale.Hide();
			aFtEndAngle.Hide();
			aMtrEndAngle.Hide();
			aFtDepth.Hide();
			aMtrDepth.Hide();
            aFLGeometrie.Hide();

			aBtnNormalsObj.Hide();
			aBtnNormalsFlat.Hide();
			aBtnNormalsSphere.Hide();
			aBtnTwoSidedLighting.Hide();
			aBtnNormalsInvert.Hide();
            aFLNormals.Hide();
			aBtnDoubleSided.Hide();
		}

		// Darstellung
		if( eViewType == VIEWTYPE_REPRESENTATION )
		{
			aFtShademode.Show();
			aLbShademode.Show();
			aBtnShadow3d.Show();
			aFtSlant.Show();
			aMtrSlant.Show();
            aFLShadow.Show();
			aFtDistance.Show();
			aMtrDistance.Show();
			aFtFocalLeng.Show();
			aMtrFocalLength.Show();
            aFLCamera.Show();
            aFLRepresentation.Show();
		}
		else
		{
			aFtShademode.Hide();
			aLbShademode.Hide();
			aBtnShadow3d.Hide();
			aFtSlant.Hide();
			aMtrSlant.Hide();
            aFLShadow.Hide();
			aFtDistance.Hide();
			aMtrDistance.Hide();
			aFtFocalLeng.Hide();
			aMtrFocalLength.Hide();
            aFLCamera.Hide();
            aFLRepresentation.Hide();
		}

		// Beleuchtung
		if( eViewType == VIEWTYPE_LIGHT )
		{
			aBtnLight1.Show();
			aBtnLight2.Show();
			aBtnLight3.Show();
			aBtnLight4.Show();
			aBtnLight5.Show();
			aBtnLight6.Show();
			aBtnLight7.Show();
			aBtnLight8.Show();
			//aLbLight1.Show();
			aBtnLightColor.Show();
            aFTLightsource.Show();
			aLbAmbientlight.Show();
			aBtnAmbientColor.Show();
            aFTAmbientlight.Show();
            aFLLight.Show();
			//aFtLightX.Show();
			//aFtLightY.Show();
			//aFtLightZ.Show();
			//aGrpLightInfo.Show();

			ColorLB* pLb = GetLbByButton();
			if( pLb )
				pLb->Show();

			aCtlLightPreview.Show();
			aCtlPreview.Hide();
		}
		else
		{
			aBtnLight1.Hide();
			aBtnLight2.Hide();
			aBtnLight3.Hide();
			aBtnLight4.Hide();
			aBtnLight5.Hide();
			aBtnLight6.Hide();
			aBtnLight7.Hide();
			aBtnLight8.Hide();
			aLbLight1.Hide();
			aLbLight2.Hide();
			aLbLight3.Hide();
			aLbLight4.Hide();
			aLbLight5.Hide();
			aLbLight6.Hide();
			aLbLight7.Hide();
			aLbLight8.Hide();
			aBtnLightColor.Hide();
            aFTLightsource.Hide();
			aLbAmbientlight.Hide();
			aBtnAmbientColor.Hide();
            aFTAmbientlight.Hide();
            aFLLight.Hide();

			if( !aCtlPreview.IsVisible() )
			{
				aCtlPreview.Show();
				aCtlLightPreview.Hide();
			}
		}

		// Texturen
		if( eViewType == VIEWTYPE_TEXTURE )
		{
			aFtTexKind.Show();
			aBtnTexLuminance.Show();
			aBtnTexColor.Show();
			aFtTexMode.Show();
			aBtnTexReplace.Show();
			aBtnTexModulate.Show();
			//aBtnTexBlend.Show();
			aFtTexProjectionX.Show();
			aBtnTexParallelX.Show();
			aBtnTexCircleX.Show();
			aBtnTexObjectX.Show();
			aFtTexProjectionY.Show();
			aBtnTexParallelY.Show();
			aBtnTexCircleY.Show();
			aBtnTexObjectY.Show();
			aFtTexFilter.Show();
			aBtnTexFilter.Show();
            aFLTexture.Show();
		}
		else
		{
			aFtTexKind.Hide();
			aBtnTexLuminance.Hide();
			aBtnTexColor.Hide();
			aFtTexMode.Hide();
			aBtnTexReplace.Hide();
			aBtnTexModulate.Hide();
			aBtnTexBlend.Hide();
			aFtTexProjectionX.Hide();
			aBtnTexParallelX.Hide();
			aBtnTexCircleX.Hide();
			aBtnTexObjectX.Hide();
			aFtTexProjectionY.Hide();
			aBtnTexParallelY.Hide();
			aBtnTexCircleY.Hide();
			aBtnTexObjectY.Hide();
			aFtTexFilter.Hide();
			aBtnTexFilter.Hide();
            aFLTexture.Hide();
		}

		// Material
		if( eViewType == VIEWTYPE_MATERIAL )
		{
			aFtMatFavorites.Show();
			aLbMatFavorites.Show();
			aFtMatColor.Show();
			aLbMatColor.Show();
			aBtnMatColor.Show();
			aFtMatEmission.Show();
			aLbMatEmission.Show();
			aBtnEmissionColor.Show();
			aFtMatSpecular.Show();
			aLbMatSpecular.Show();
			aBtnSpecularColor.Show();
			aFtMatSpecularIntensity.Show();
			aMtrMatSpecularIntensity.Show();
            aFLMatSpecular.Show();
            aFLMaterial.Show();
		}
		else
		{
			aFtMatFavorites.Hide();
			aLbMatFavorites.Hide();
			aFtMatColor.Hide();
			aLbMatColor.Hide();
			aBtnMatColor.Hide();
			aFtMatEmission.Hide();
			aLbMatEmission.Hide();
			aBtnEmissionColor.Hide();
			aFtMatSpecular.Hide();
			aLbMatSpecular.Hide();
			aBtnSpecularColor.Hide();
			aFtMatSpecularIntensity.Hide();
			aMtrMatSpecularIntensity.Hide();
            aFLMatSpecular.Hide();
            aFLMaterial.Hide();
		}
		if( bUpdatePreview && !aBtnLight.IsChecked() )
			UpdatePreview();

	}
	else
	{
		aBtnGeo.Check( eViewType == VIEWTYPE_GEO );
		aBtnRepresentation.Check( eViewType == VIEWTYPE_REPRESENTATION );
		aBtnLight.Check( eViewType == VIEWTYPE_LIGHT );
		aBtnTexture.Check( eViewType == VIEWTYPE_TEXTURE );
		aBtnMaterial.Check( eViewType == VIEWTYPE_MATERIAL );
	}
	return( 0L );
}

// -----------------------------------------------------------------------
IMPL_LINK( Svx3DWin, ClickHdl, PushButton *, pBtn )
{
	BOOL bUpdatePreview = FALSE;

	if( pBtn )
	{
		USHORT nSId = 0;

		if( pBtn == &aBtnConvertTo3D )
		{
			nSId = SID_CONVERT_TO_3D;
		}
		else if( pBtn == &aBtnLatheObject )
		{
			nSId = SID_CONVERT_TO_3D_LATHE_FAST;
		}
		// Geometrie
		else if( pBtn == &aBtnNormalsObj ||
				 pBtn == &aBtnNormalsFlat ||
				 pBtn == &aBtnNormalsSphere )
		{
			aBtnNormalsObj.Check( pBtn == &aBtnNormalsObj );
			aBtnNormalsFlat.Check( pBtn == &aBtnNormalsFlat );
			aBtnNormalsSphere.Check( pBtn == &aBtnNormalsSphere );
			bUpdatePreview = TRUE;
		}
		else if( pBtn == &aBtnLight1 ||
				 pBtn == &aBtnLight2 ||
				 pBtn == &aBtnLight3 ||
				 pBtn == &aBtnLight4 ||
				 pBtn == &aBtnLight5 ||
				 pBtn == &aBtnLight6 ||
				 pBtn == &aBtnLight7 ||
				 pBtn == &aBtnLight8 )
		{
			// Beleuchtung
			ColorLB* pLb = GetLbByButton( pBtn );
			pLb->Show();

			if( pBtn->IsChecked() )
			{
				SetUILightState( *(ImageButton*)pBtn, !GetUILightState( *(ImageButton*)pBtn ) );
			}
			else
			{
				pBtn->Check();

				if( pBtn != &aBtnLight1 && aBtnLight1.IsChecked() )
				{
					aBtnLight1.Check( FALSE );
					aLbLight1.Hide();
				}
				if( pBtn != &aBtnLight2 && aBtnLight2.IsChecked() )
				{
					aBtnLight2.Check( FALSE );
					aLbLight2.Hide();
				}
				if( pBtn != &aBtnLight3 && aBtnLight3.IsChecked() )
				{
					aBtnLight3.Check( FALSE );
					aLbLight3.Hide();
				}
				if( pBtn != &aBtnLight4 && aBtnLight4.IsChecked() )
				{
					aBtnLight4.Check( FALSE );
					aLbLight4.Hide();
				}
				if( pBtn != &aBtnLight5 && aBtnLight5.IsChecked() )
				{
					aBtnLight5.Check( FALSE );
					aLbLight5.Hide();
				}
				if( pBtn != &aBtnLight6 && aBtnLight6.IsChecked() )
				{
					aBtnLight6.Check( FALSE );
					aLbLight6.Hide();
				}
				if( pBtn != &aBtnLight7 && aBtnLight7.IsChecked() )
				{
					aBtnLight7.Check( FALSE );
					aLbLight7.Hide();
				}
				if( pBtn != &aBtnLight8 && aBtnLight8.IsChecked() )
				{
					aBtnLight8.Check( FALSE );
					aLbLight8.Hide();
				}
			}
			BOOL bEnable = GetUILightState( *(ImageButton*)pBtn );
			aBtnLightColor.Enable( bEnable );
			pLb->Enable( bEnable );

			ClickLightHdl( pBtn );
			bUpdatePreview = TRUE;
		}
		// Texturen
		else if( pBtn == &aBtnTexLuminance ||
				 pBtn == &aBtnTexColor )
		{
			aBtnTexLuminance.Check( pBtn == &aBtnTexLuminance );
			aBtnTexColor.Check( pBtn == &aBtnTexColor );
			bUpdatePreview = TRUE;
		}
		else if( pBtn == &aBtnTexReplace ||
				 pBtn == &aBtnTexModulate )// ||
				 //pBtn == &aBtnTexBlend )
		{
			aBtnTexReplace.Check( pBtn == &aBtnTexReplace );
			aBtnTexModulate.Check( pBtn == &aBtnTexModulate );
			//aBtnTexBlend.Check( pBtn == &aBtnTexBlend );
			bUpdatePreview = TRUE;
		}
		else if( pBtn == &aBtnTexParallelX ||
				 pBtn == &aBtnTexCircleX ||
				 pBtn == &aBtnTexObjectX )
		{
			aBtnTexParallelX.Check( pBtn == &aBtnTexParallelX );
			aBtnTexCircleX.Check( pBtn == &aBtnTexCircleX );
			aBtnTexObjectX.Check( pBtn == &aBtnTexObjectX );
			bUpdatePreview = TRUE;
		}
		else if( pBtn == &aBtnTexParallelY ||
				 pBtn == &aBtnTexCircleY ||
				 pBtn == &aBtnTexObjectY )
		{
			aBtnTexParallelY.Check( pBtn == &aBtnTexParallelY );
			aBtnTexCircleY.Check( pBtn == &aBtnTexCircleY );
			aBtnTexObjectY.Check( pBtn == &aBtnTexObjectY );
			bUpdatePreview = TRUE;
		}
		else if( pBtn == &aBtnShadow3d  )
		{
			pBtn->Check( !pBtn->IsChecked() );
			aFtSlant.Enable( pBtn->IsChecked() );
			aMtrSlant.Enable( pBtn->IsChecked() );
			bUpdatePreview = TRUE;
		}
		// Sonstige (keine Gruppen)
		else if( pBtn != NULL )
		{
			pBtn->Check( !pBtn->IsChecked() );
			bUpdatePreview = TRUE;
		}

		if( nSId > 0 )
		{
            SfxDispatcher* pDispatcher = LocalGetDispatcher(pBindings);
            if (pDispatcher != NULL)
            {
                SfxBoolItem aItem( nSId, TRUE );
                pDispatcher->Execute(
                    nSId, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD, &aItem, 0L );
            }
		}
		else if( bUpdatePreview == TRUE )
			UpdatePreview();
	}
	return( 0L );
}

//------------------------------------------------------------------------

IMPL_LINK( Svx3DWin, ClickColorHdl, PushButton *, pBtn )
{
	SvColorDialog aColorDlg( this );
	ColorLB* pLb;

	if( pBtn == &aBtnLightColor )
		pLb = GetLbByButton();
	else if( pBtn == &aBtnAmbientColor )
		pLb = &aLbAmbientlight;
	else if( pBtn == &aBtnMatColor )
		pLb = &aLbMatColor;
	else if( pBtn == &aBtnEmissionColor )
		pLb = &aLbMatEmission;
	else // if( pBtn == &aBtnSpecularColor )
		pLb = &aLbMatSpecular;

	Color aColor = pLb->GetSelectEntryColor();

	aColorDlg.SetColor( aColor );
	if( aColorDlg.Execute() == RET_OK )
	{
		aColor = aColorDlg.GetColor();
		if( LBSelectColor( pLb, aColor ) )
			SelectHdl( pLb );
	}
	return( 0L );
}

// -----------------------------------------------------------------------
IMPL_LINK( Svx3DWin, SelectHdl, void *, p )
{
	if( p )
	{
		Color aColor;
		BOOL bUpdatePreview = FALSE;

		// Material
		if( p == &aLbMatFavorites )
		{
			Color aColObj( COL_WHITE );
			Color aColEmis( COL_BLACK );
			Color aColSpec( COL_WHITE );
			USHORT nSpecIntens = 20;

			USHORT nPos = aLbMatFavorites.GetSelectEntryPos();
			switch( nPos )
			{
				case 1: // Metall
				{
					aColObj = Color(230,230,255);
					aColEmis = Color(10,10,30);
					aColSpec = Color(200,200,200);
					nSpecIntens = 20;
				}
				break;

				case 2: // Gold
				{
					aColObj = Color(230,255,0);
					aColEmis = Color(51,0,0);
					aColSpec = Color(255,255,240);
					nSpecIntens = 20;
				}
				break;

				case 3: // Chrom
				{
					aColObj = Color(36,117,153);
					aColEmis = Color(18,30,51);
					aColSpec = Color(230,230,255);
					nSpecIntens = 2;
				}
				break;

				case 4: // Plastik
				{
					aColObj = Color(255,48,57);
					aColEmis = Color(35,0,0);
					aColSpec = Color(179,202,204);
					nSpecIntens = 60;
				}
				break;

				case 5: // Holz
				{
					aColObj = Color(153,71,1);
					aColEmis = Color(21,22,0);
					aColSpec = Color(255,255,153);
					nSpecIntens = 75;
				}
				break;
			}
			LBSelectColor( &aLbMatColor, aColObj );
			LBSelectColor( &aLbMatEmission, aColEmis );
			LBSelectColor( &aLbMatSpecular, aColSpec );
			aMtrMatSpecularIntensity.SetValue( nSpecIntens );

			bUpdatePreview = TRUE;
		}
		else if( p == &aLbMatColor ||
				 p == &aLbMatEmission ||
				 p == &aLbMatSpecular )
		{
			aLbMatFavorites.SelectEntryPos( 0 );
			bUpdatePreview = TRUE;
		}
		// Beleuchtung
		else if( p == &aLbAmbientlight )
		{
			bUpdatePreview = TRUE;
		}
		else if( p == &aLbLight1 ||
				 p == &aLbLight2 ||
				 p == &aLbLight3 ||
				 p == &aLbLight4 ||
				 p == &aLbLight5 ||
				 p == &aLbLight6 ||
				 p == &aLbLight7 ||
				 p == &aLbLight8 )
		{
			bUpdatePreview = TRUE;
		}
		else if( p == &aLbShademode )
			bUpdatePreview = TRUE;

		if( bUpdatePreview == TRUE )
			UpdatePreview();
	}
	return( 0L );
}

// -----------------------------------------------------------------------
IMPL_LINK( Svx3DWin, ModifyHdl, void*, pField )
{
	if( pField )
	{
		BOOL bUpdatePreview = FALSE;

		// Material
		if( pField == &aMtrMatSpecularIntensity )
		{
			bUpdatePreview = TRUE;
		}
		else if( pField == &aNumHorizontal )
		{
			bUpdatePreview = TRUE;
		}
		else if( pField == &aNumVertical )
		{
			bUpdatePreview = TRUE;
		}
		else if( pField == &aMtrSlant )
		{
			bUpdatePreview = TRUE;
		}

		if( bUpdatePreview == TRUE )
			UpdatePreview();
	}
	return( 0L );
}

// -----------------------------------------------------------------------

IMPL_LINK( Svx3DWin, ClickLightHdl, PushButton*, pBtn )
{

	if( pBtn )
	{
		USHORT nLightSource = GetLightSource( pBtn );
		ColorLB* pLb = GetLbByButton( pBtn );
		Color aColor( pLb->GetSelectEntryColor() );
        SfxItemSet aLightItemSet(aCtlLightPreview.GetSvx3DLightControl().Get3DAttributes());
        const bool bOnOff(GetUILightState( *(ImageButton*)pBtn ));

        switch(nLightSource)
        {
            case 0: aLightItemSet.Put(Svx3DLightcolor1Item(aColor)); aLightItemSet.Put(Svx3DLightOnOff1Item(bOnOff)); break;
            case 1: aLightItemSet.Put(Svx3DLightcolor2Item(aColor)); aLightItemSet.Put(Svx3DLightOnOff2Item(bOnOff)); break;
            case 2: aLightItemSet.Put(Svx3DLightcolor3Item(aColor)); aLightItemSet.Put(Svx3DLightOnOff3Item(bOnOff)); break;
            case 3: aLightItemSet.Put(Svx3DLightcolor4Item(aColor)); aLightItemSet.Put(Svx3DLightOnOff4Item(bOnOff)); break;
            case 4: aLightItemSet.Put(Svx3DLightcolor5Item(aColor)); aLightItemSet.Put(Svx3DLightOnOff5Item(bOnOff)); break;
            case 5: aLightItemSet.Put(Svx3DLightcolor6Item(aColor)); aLightItemSet.Put(Svx3DLightOnOff6Item(bOnOff)); break;
            case 6: aLightItemSet.Put(Svx3DLightcolor7Item(aColor)); aLightItemSet.Put(Svx3DLightOnOff7Item(bOnOff)); break;
            default:
            case 7: aLightItemSet.Put(Svx3DLightcolor8Item(aColor)); aLightItemSet.Put(Svx3DLightOnOff8Item(bOnOff)); break;
        }

        aCtlLightPreview.GetSvx3DLightControl().Set3DAttributes(aLightItemSet);
		aCtlLightPreview.GetSvx3DLightControl().SelectLight(nLightSource);
		aCtlLightPreview.CheckSelection();
	}
	return( 0L );
}


// -----------------------------------------------------------------------
IMPL_LINK( Svx3DWin, DoubleClickHdl, void*, EMPTYARG )
{
	//USHORT nItemId = aCtlFavorites.GetSelectItemId();

	//SfxItemSet* pSet = (SfxItemSet*) pFavorSetList->GetObject( nItemId - 1 );
	//Update( *pSet );

	// und zuweisen
	ClickAssignHdl( NULL );

	return( 0L );
}

// -----------------------------------------------------------------------

IMPL_LINK( Svx3DWin, ChangeLightCallbackHdl, void*, EMPTYARG )
{
	return( 0L );
}


// -----------------------------------------------------------------------

IMPL_LINK( Svx3DWin, ChangeSelectionCallbackHdl, void*, EMPTYARG )
{
	const sal_uInt32 nLight(aCtlLightPreview.GetSvx3DLightControl().GetSelectedLight());
	PushButton* pBtn = 0;

	switch( nLight )
	{
		case 0: pBtn = &aBtnLight1; break;
		case 1: pBtn = &aBtnLight2; break;
		case 2: pBtn = &aBtnLight3; break;
		case 3: pBtn = &aBtnLight4; break;
		case 4: pBtn = &aBtnLight5; break;
		case 5: pBtn = &aBtnLight6; break;
		case 6: pBtn = &aBtnLight7; break;
		case 7: pBtn = &aBtnLight8; break;
		default: break;
	}

	if( pBtn )
		ClickHdl( pBtn );
	else
	{
		// Zustand: Keine Lampe selektiert
		if( aBtnLight1.IsChecked() )
		{
			aBtnLight1.Check( FALSE );
			aLbLight1.Enable( FALSE );
		}
		else if( aBtnLight2.IsChecked() )
		{
			aBtnLight2.Check( FALSE );
			aLbLight2.Enable( FALSE );
		}
		else if( aBtnLight3.IsChecked() )
		{
			aBtnLight3.Check( FALSE );
			aLbLight3.Enable( FALSE );
		}
		else if( aBtnLight4.IsChecked() )
		{
			aBtnLight4.Check( FALSE );
			aLbLight4.Enable( FALSE );
		}
		else if( aBtnLight5.IsChecked() )
		{
			aBtnLight5.Check( FALSE );
			aLbLight5.Enable( FALSE );
		}
		else if( aBtnLight6.IsChecked() )
		{
			aBtnLight6.Check( FALSE );
			aLbLight6.Enable( FALSE );
		}
		else if( aBtnLight7.IsChecked() )
		{
			aBtnLight7.Check( FALSE );
			aLbLight7.Enable( FALSE );
		}
		else if( aBtnLight8.IsChecked() )
		{
			aBtnLight8.Check( FALSE );
			aLbLight8.Enable( FALSE );
		}
		aBtnLightColor.Enable( FALSE );
	}

	return( 0L );
}

// -----------------------------------------------------------------------
// Methode um sicherzustellen, dass die LB auch mit einer Farbe gesetzt ist
// Liefert TRUE zurueck, falls Farbe hinzugefuegt wurde
// -----------------------------------------------------------------------
BOOL Svx3DWin::LBSelectColor( ColorLB* pLb, const Color& rColor )
{
	BOOL bRet = FALSE;

	pLb->SetNoSelection();
	pLb->SelectEntry( rColor );
	if( pLb->GetSelectEntryCount() == 0 )
	{
		String aStr(SVX_RES(RID_SVXFLOAT3D_FIX_R));

		aStr += String::CreateFromInt32((INT32)rColor.GetRed());
		aStr += sal_Unicode(' ');
		aStr += String(SVX_RES(RID_SVXFLOAT3D_FIX_G));
		aStr += String::CreateFromInt32((INT32)rColor.GetGreen());
		aStr += sal_Unicode(' ');
		aStr += String(SVX_RES(RID_SVXFLOAT3D_FIX_B));
		aStr += String::CreateFromInt32((INT32)rColor.GetBlue());

		USHORT nPos = pLb->InsertEntry( rColor, aStr );
		pLb->SelectEntryPos( nPos );
		bRet = TRUE;
	}
	return( bRet );
}

// -----------------------------------------------------------------------
void Svx3DWin::UpdatePreview()
{
	if( pModel == NULL )
		pModel = new FmFormModel();

	if(bOnly3DChanged)
	{
		// slot executen
        SfxDispatcher* pDispatcher = LocalGetDispatcher(pBindings);
        if (pDispatcher != NULL)
        {
            SfxBoolItem aItem( SID_3D_STATE, TRUE );
            pDispatcher->Execute(
                SID_3D_STATE, SFX_CALLMODE_SYNCHRON | SFX_CALLMODE_RECORD, &aItem, 0L );
        }
		// Flag zuruecksetzen
		bOnly3DChanged = FALSE;
	}

	// ItemSet besorgen
	SfxItemSet aSet( pModel->GetItemPool(), SDRATTR_START, SDRATTR_END);

	// Attribute holen und im Preview setzen
	GetAttr( aSet );
	aCtlPreview.Set3DAttributes( aSet );
	aCtlLightPreview.GetSvx3DLightControl().Set3DAttributes( aSet );
}

//////////////////////////////////////////////////////////////////////////////
// document is to be reloaded, destroy remembered ItemSet (#83951#)
void Svx3DWin::DocumentReload()
{
	if(mpRemember2DAttributes)
		delete mpRemember2DAttributes;
	mpRemember2DAttributes = 0L;
}

// -----------------------------------------------------------------------
void Svx3DWin::InitColorLB( const SdrModel* pDoc )
{
	aLbLight1.Fill( pDoc->GetColorTable() );
	aLbLight2.CopyEntries( aLbLight1 );
	aLbLight3.CopyEntries( aLbLight1 );
	aLbLight4.CopyEntries( aLbLight1 );
	aLbLight5.CopyEntries( aLbLight1 );
	aLbLight6.CopyEntries( aLbLight1 );
	aLbLight7.CopyEntries( aLbLight1 );
	aLbLight8.CopyEntries( aLbLight1 );
	aLbAmbientlight.CopyEntries( aLbLight1 );
	aLbMatColor.CopyEntries( aLbLight1 );
	aLbMatEmission.CopyEntries( aLbLight1 );
	aLbMatSpecular.CopyEntries( aLbLight1 );

	// Erstmal...
	Color aColWhite( COL_WHITE );
	Color aColBlack( COL_BLACK );
	aLbLight1.SelectEntry( aColWhite );
	aLbLight2.SelectEntry( aColWhite );
	aLbLight3.SelectEntry( aColWhite );
	aLbLight4.SelectEntry( aColWhite );
	aLbLight5.SelectEntry( aColWhite );
	aLbLight6.SelectEntry( aColWhite );
	aLbLight7.SelectEntry( aColWhite );
	aLbLight8.SelectEntry( aColWhite );
	aLbAmbientlight.SelectEntry( aColBlack );
	aLbMatColor.SelectEntry( aColWhite );
	aLbMatEmission.SelectEntry( aColBlack );
	aLbMatSpecular.SelectEntry( aColWhite );
}

// -----------------------------------------------------------------------
USHORT Svx3DWin::GetLightSource( const PushButton* pBtn )
{
	USHORT nLight = 8;

	if( pBtn == NULL )
	{
		if( aBtnLight1.IsChecked() )
			nLight = 0;
		else if( aBtnLight2.IsChecked() )
			nLight = 1;
		else if( aBtnLight3.IsChecked() )
			nLight = 2;
		else if( aBtnLight4.IsChecked() )
			nLight = 3;
		else if( aBtnLight5.IsChecked() )
			nLight = 4;
		else if( aBtnLight6.IsChecked() )
			nLight = 5;
		else if( aBtnLight7.IsChecked() )
			nLight = 6;
		else if( aBtnLight8.IsChecked() )
			nLight = 7;
	}
	else
	{
		if( pBtn == &aBtnLight1 )
			nLight = 0;
		else if( pBtn == &aBtnLight2 )
			nLight = 1;
		else if( pBtn == &aBtnLight3 )
			nLight = 2;
		else if( pBtn == &aBtnLight4 )
			nLight = 3;
		else if( pBtn == &aBtnLight5 )
			nLight = 4;
		else if( pBtn == &aBtnLight6 )
			nLight = 5;
		else if( pBtn == &aBtnLight7 )
			nLight = 6;
		else if( pBtn == &aBtnLight8 )
			nLight = 7;
	}
	return( nLight );
};

// -----------------------------------------------------------------------
ColorLB* Svx3DWin::GetLbByButton( const PushButton* pBtn )
{
	ColorLB* pLb = NULL;

	if( pBtn == NULL )
	{
		if( aBtnLight1.IsChecked() )
			pLb = &aLbLight1;
		else if( aBtnLight2.IsChecked() )
			pLb = &aLbLight2;
		else if( aBtnLight3.IsChecked() )
			pLb = &aLbLight3;
		else if( aBtnLight4.IsChecked() )
			pLb = &aLbLight4;
		else if( aBtnLight5.IsChecked() )
			pLb = &aLbLight5;
		else if( aBtnLight6.IsChecked() )
			pLb = &aLbLight6;
		else if( aBtnLight7.IsChecked() )
			pLb = &aLbLight7;
		else if( aBtnLight8.IsChecked() )
			pLb = &aLbLight8;
	}
	else
	{
		if( pBtn == &aBtnLight1 )
			pLb = &aLbLight1;
		else if( pBtn == &aBtnLight2 )
			pLb = &aLbLight2;
		else if( pBtn == &aBtnLight3 )
			pLb = &aLbLight3;
		else if( pBtn == &aBtnLight4 )
			pLb = &aLbLight4;
		else if( pBtn == &aBtnLight5 )
			pLb = &aLbLight5;
		else if( pBtn == &aBtnLight6 )
			pLb = &aLbLight6;
		else if( pBtn == &aBtnLight7 )
			pLb = &aLbLight7;
		else if( pBtn == &aBtnLight8 )
			pLb = &aLbLight8;
	}
	return( pLb );
};

/*************************************************************************
|*
|* Ableitung vom SfxChildWindow als "Behaelter" fuer Effekte
|*
\************************************************************************/
__EXPORT Svx3DChildWindow::Svx3DChildWindow( Window* _pParent,
														 USHORT nId,
														 SfxBindings* pBindings,
														 SfxChildWinInfo* pInfo ) :
	SfxChildWindow( _pParent, nId )
{
	Svx3DWin* pWin = new Svx3DWin( pBindings, this, _pParent );
	pWindow = pWin;

	eChildAlignment = SFX_ALIGN_NOALIGNMENT;

	pWin->Initialize( pInfo );
}

/*************************************************************************
|*
|* ControllerItem fuer 3DStatus
|*
\************************************************************************/
Svx3DCtrlItem::Svx3DCtrlItem( USHORT _nId,
								Svx3DWin* pWin,
								SfxBindings* _pBindings) :
	SfxControllerItem( _nId, *_pBindings ),
	p3DWin( pWin )
{
}

// -----------------------------------------------------------------------
void __EXPORT Svx3DCtrlItem::StateChanged( USHORT /*nSId*/,
						SfxItemState /*eState*/, const SfxPoolItem* /*pItem*/ )
{
}

/*************************************************************************
|*
|* ControllerItem fuer Status Slot SID_CONVERT_TO_3D
|*
\************************************************************************/

SvxConvertTo3DItem::SvxConvertTo3DItem(UINT16 _nId, SfxBindings* _pBindings)
:	SfxControllerItem(_nId, *_pBindings),
	bState(FALSE)
{
}

void SvxConvertTo3DItem::StateChanged(UINT16 /*_nId*/, SfxItemState eState, const SfxPoolItem* /*pState*/)
{
	BOOL bNewState = (eState != SFX_ITEM_DISABLED);
	if(bNewState != bState)
	{
		bState = bNewState;
        SfxDispatcher* pDispatcher = LocalGetDispatcher(&GetBindings());
        if (pDispatcher != NULL)
        {
            SfxBoolItem aItem( SID_3D_STATE, TRUE );
            pDispatcher->Execute(
                SID_3D_STATE, SFX_CALLMODE_ASYNCHRON|SFX_CALLMODE_RECORD, &aItem, 0L);
        }
	}
}


