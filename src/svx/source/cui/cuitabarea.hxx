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
#ifndef _CUI_TAB_AREA_HXX
#define _CUI_TAB_AREA_HXX

// include ---------------------------------------------------------------

#include <svx/tabarea.hxx>

/*************************************************************************
|*
|* Fl"achen-Tab-Dialog
|*
\************************************************************************/

class SvxAreaTabDialog : public SfxTabDialog
{
private:
	SdrModel*           mpDrawModel;
//	const SdrView*		mpView;

	XColorTable*        mpColorTab;
	XColorTable*        mpNewColorTab;
	XGradientList*      mpGradientList;
	XGradientList*      mpNewGradientList;
	XHatchList*         mpHatchingList;
	XHatchList*         mpNewHatchingList;
	XBitmapList*        mpBitmapList;
	XBitmapList*        mpNewBitmapList;

	const SfxItemSet&   mrOutAttrs;

	ChangeType          mnColorTableState;
	ChangeType          mnBitmapListState;
	ChangeType          mnGradientListState;
	ChangeType          mnHatchingListState;

	USHORT              mnPageType;
	USHORT              mnDlgType;
	USHORT              mnPos;
	BOOL                mbAreaTP;
	BOOL                mbDeleteColorTable;

	virtual void        PageCreated( USHORT nId, SfxTabPage &rPage );

protected:
	virtual short       Ok();
#ifdef _SVX_TABAREA_CXX
	DECL_LINK( CancelHdlImpl, void * );
	void                SavePalettes();
#endif

public:
	SvxAreaTabDialog( Window* pParent,
					  const SfxItemSet* pAttr, SdrModel* pModel,
					  const SdrView* pSdrView = NULL );
	~SvxAreaTabDialog();

	void            	 SetNewColorTable( XColorTable* pColTab )
							{ mpNewColorTab = pColTab; }
	XColorTable*    	 GetNewColorTable() const { return mpNewColorTab; }
	const XColorTable*	 GetColorTable() const { return mpColorTab; }

	void            	 SetNewGradientList( XGradientList* pGrdLst)
							{ mpNewGradientList = pGrdLst; }
	XGradientList*  	 GetNewGradientList() const
							{ return mpNewGradientList; }
	const XGradientList* GetGradientList() const { return mpGradientList; }

	void				 SetNewHatchingList( XHatchList* pHtchLst)
							{ mpNewHatchingList = pHtchLst; }
	XHatchList*     	 GetNewHatchingList() const
							{ return mpNewHatchingList; }
	const XHatchList*	 GetHatchingList() const { return mpHatchingList; }

	void				 SetNewBitmapList( XBitmapList* pBmpLst)
							{ mpNewBitmapList = pBmpLst; }
	XBitmapList*		 GetNewBitmapList() const { return mpNewBitmapList; }
	const XBitmapList*	 GetBitmapList() const { return mpBitmapList; }

	void				 DontDeleteColorTable() { mbDeleteColorTable = FALSE; }
};

/*************************************************************************
|*
|* Transparence-Tab-Page
|*
\************************************************************************/

class SvxTransparenceTabPage : public SvxTabPage
{
	using TabPage::ActivatePage;
	using TabPage::DeactivatePage;

	const SfxItemSet&   rOutAttrs;
	RECT_POINT          eRP;

	//CHINA001 UINT16*             pPageType;
	//CHINA001 UINT16*             pDlgType;
	UINT16             nPageType; //add CHINA001 
	UINT16             nDlgType;  //add CHINA001 

	// main selection
    FixedLine           aFlProp;
    RadioButton         aRbtTransOff;
    RadioButton         aRbtTransLinear;
	RadioButton         aRbtTransGradient;

	// linear transparency
	MetricField         aMtrTransparent;

	// gradient transparency
	FixedText           aFtTrgrType;
	ListBox             aLbTrgrGradientType;
	FixedText           aFtTrgrCenterX;
	MetricField         aMtrTrgrCenterX;
	FixedText           aFtTrgrCenterY;
	MetricField         aMtrTrgrCenterY;
	FixedText           aFtTrgrAngle;
	MetricField         aMtrTrgrAngle;
	FixedText           aFtTrgrBorder;
	MetricField         aMtrTrgrBorder;
	FixedText           aFtTrgrStartValue;
	MetricField         aMtrTrgrStartValue;
	FixedText           aFtTrgrEndValue;
	MetricField         aMtrTrgrEndValue;

	// preview
	SvxXRectPreview     aCtlBitmapPreview;
	SvxXRectPreview     aCtlXRectPreview;
	BOOL				bBitmap;

	XOutdevItemPool*    pXPool;
	XFillAttrSetItem    aXFillAttr;
	SfxItemSet&         rXFSet;

#ifdef _SVX_TPAREA_CXX
	DECL_LINK(ClickTransOffHdl_Impl, void * );
	DECL_LINK(ClickTransLinearHdl_Impl, void * );
	DECL_LINK(ClickTransGradientHdl_Impl, void * );
	DECL_LINK(ModifyTransparentHdl_Impl, void*);
	DECL_LINK(ChangeTrgrTypeHdl_Impl, void*);
	DECL_LINK(ModifiedTrgrHdl_Impl, void*);
#endif

	void ActivateLinear(BOOL bActivate);
	void ActivateGradient(BOOL bActivate);
	void SetControlState_Impl(XGradientStyle eXGS);

	BOOL InitPreview ( const SfxItemSet& rSet );
	void InvalidatePreview (BOOL bEnable = TRUE );

public:
	SvxTransparenceTabPage(Window* pParent, const SfxItemSet& rInAttrs);
	void Construct();

	static SfxTabPage* Create(Window*, const SfxItemSet&);
	static UINT16* GetRanges();

	virtual BOOL FillItemSet(SfxItemSet&);
	virtual void Reset(const SfxItemSet&);
	virtual void ActivatePage(const SfxItemSet& rSet);
	virtual int  DeactivatePage(SfxItemSet* pSet);
	virtual void PointChanged(Window* pWindow, RECT_POINT eRP);

	//CHINA001 void SetPageType(UINT16 *pInType) { pPageType = pInType; }
	//CHINA001 void SetDlgType(UINT16* pInType) { pDlgType = pInType; }
	void SetPageType(UINT16 nInType) { nPageType = nInType; } //add CHINA001 
	void SetDlgType(UINT16 nInType) { nDlgType = nInType; }//add CHINA001 
	virtual void PageCreated (SfxAllItemSet aSet); //add CHINA001 
};

/*************************************************************************
|*
|* Fl"achen-Tab-Page
|*
\************************************************************************/

class SvxAreaTabPage : public SvxTabPage
{
	using TabPage::ActivatePage;
	using TabPage::DeactivatePage;

private:
    FixedLine           aFlProp;
    ListBox             aTypeLB;

	ColorLB             aLbColor;
	GradientLB          aLbGradient;
	HatchingLB          aLbHatching;
	BitmapLB            aLbBitmap;
	SvxXRectPreview     aCtlBitmapPreview;

	TriStateBox         aTsbStepCount;
    FixedLine           aFlStepCount;
	NumericField        aNumFldStepCount;

	CheckBox			aCbxHatchBckgrd;
	ColorLB             aLbHatchBckgrdColor;

    FixedLine           aFlSize;
	TriStateBox			aTsbOriginal;
	TriStateBox			aTsbScale;
	FixedText			aFtXSize;
	MetricField			aMtrFldXSize;
	FixedText			aFtYSize;
	MetricField			aMtrFldYSize;
    FixedLine           aFlPosition;
	SvxRectCtl			aCtlPosition;
	FixedText			aFtXOffset;
	MetricField			aMtrFldXOffset;
	FixedText			aFtYOffset;
	MetricField			aMtrFldYOffset;
	TriStateBox         aTsbTile;
	TriStateBox         aTsbStretch;
    FixedLine           aFlOffset;
	RadioButton			aRbtRow;
	RadioButton			aRbtColumn;
	MetricField			aMtrFldOffset;

	SvxXRectPreview     aCtlXRectPreview;

	const SfxItemSet&   rOutAttrs;
	RECT_POINT          eRP;

	XColorTable*        pColorTab;
	XGradientList*      pGradientList;
	XHatchList*         pHatchingList;
	XBitmapList*        pBitmapList;

	ChangeType*         pnColorTableState;
	ChangeType*         pnBitmapListState;
	ChangeType*         pnGradientListState;
	ChangeType*         pnHatchingListState;

	//CHINA001 USHORT*             pPageType;
	//CHINA001 USHORT*             pDlgType;
	//CHINA001 USHORT*             pPos;
	UINT16 nPageType; //add CHINA001 
	UINT16 nDlgType;//add CHINA001 
	UINT16 nPos; //add CHINA001 

	BOOL*               pbAreaTP;

	XOutdevItemPool*    pXPool;
	XFillAttrSetItem    aXFillAttr;
	SfxItemSet&         rXFSet;

	SfxMapUnit			ePoolUnit;
	FieldUnit			eFUnit;

#ifdef _SVX_TPAREA_CXX
	DECL_LINK( SelectDialogTypeHdl_Impl, ListBox * );
	DECL_LINK( ClickInvisibleHdl_Impl, void * );
	DECL_LINK( ClickColorHdl_Impl, void * );
	DECL_LINK( ModifyColorHdl_Impl, void * );
	DECL_LINK( ModifyHatchBckgrdColorHdl_Impl, void * );
	DECL_LINK( ClickGradientHdl_Impl, void * );
	DECL_LINK( ModifyGradientHdl_Impl, void * );
	DECL_LINK( ClickHatchingHdl_Impl, void * );
	DECL_LINK( ModifyHatchingHdl_Impl, void * );
	DECL_LINK( ToggleHatchBckgrdColorHdl_Impl, void * );
	DECL_LINK( ClickBitmapHdl_Impl, void * );
	DECL_LINK( ModifyBitmapHdl_Impl, void * );
//	DECL_LINK( ModifyTransparentHdl_Impl, void * );
	DECL_LINK( ModifyStepCountHdl_Impl, void * );
	DECL_LINK( ModifyTileHdl_Impl, void * );
	DECL_LINK( ClickScaleHdl_Impl, void * );
#endif

public:
	SvxAreaTabPage( Window* pParent, const SfxItemSet& rInAttrs  );

	void    Construct();

	static  SfxTabPage* Create( Window*, const SfxItemSet& );
	static  USHORT*	    GetRanges();

	virtual BOOL FillItemSet( SfxItemSet& );
	virtual void Reset( const SfxItemSet & );
	virtual void ActivatePage( const SfxItemSet& rSet );
	virtual int  DeactivatePage( SfxItemSet* pSet );
	virtual void PointChanged( Window* pWindow, RECT_POINT eRP );
	
	void    SetColorTable( XColorTable* pColTab ) { pColorTab = pColTab; }
	void    SetGradientList( XGradientList* pGrdLst)
				{ pGradientList = pGrdLst; }
	void    SetHatchingList( XHatchList* pHtchLst)
				{ pHatchingList = pHtchLst; }
	void    SetBitmapList( XBitmapList* pBmpLst) { pBitmapList = pBmpLst; }

	//CHINA001 void    SetPageType( USHORT* pInType ) { pPageType = pInType; }
	void    SetPageType( UINT16 nInType ) { nPageType = nInType; } //add CHINA001 
	//CHINA001 void    SetDlgType( USHORT* pInType ) { pDlgType = pInType; }
	void    SetDlgType( UINT16 nInType ) { nDlgType = nInType; }//add CHINA001 
	//CHINA001 void    SetPos( USHORT* pInPos ) { pPos = pInPos; }
	void    SetPos( UINT16 nInPos ) { nPos = nInPos; }//add CHINA001 
	void    SetAreaTP( BOOL* pIn ) { pbAreaTP = pIn; }
	virtual void PageCreated (SfxAllItemSet aSet); //add CHINA001 
	void    SetColorChgd( ChangeType* pIn ) { pnColorTableState = pIn; }
	void    SetGrdChgd( ChangeType* pIn ) { pnGradientListState = pIn; }
	void    SetHtchChgd( ChangeType* pIn ) { pnHatchingListState = pIn; }
	void    SetBmpChgd( ChangeType* pIn ) { pnBitmapListState = pIn; }
};

/*************************************************************************
|*
|* Schatten-Tab-Page
|*
\************************************************************************/

class SvxShadowTabPage : public SvxTabPage
{
	using TabPage::ActivatePage;
	using TabPage::DeactivatePage;

private:
    FixedLine           aFlProp;
    TriStateBox         aTsbShowShadow;
	FixedText           aFtPosition;
	SvxRectCtl          aCtlPosition;
	FixedText           aFtDistance;
	MetricField         aMtrDistance;
	FixedText           aFtShadowColor;
	ColorLB             aLbShadowColor;
	FixedText           aFtTransparent;
	MetricField         aMtrTransparent;
	SvxXShadowPreview	aCtlXRectPreview;

	const SfxItemSet&   rOutAttrs;
	RECT_POINT          eRP;

	XColorTable*        pColorTab;
	ChangeType*         pnColorTableState;
	UINT16				nPageType;	//add CHINA001 
	UINT16				nDlgType;	//add CHINA001 
	USHORT*             pPos;
	BOOL*               pbAreaTP;

	BOOL				bDisable;

	XOutdevItemPool*    pXPool;
	XFillAttrSetItem    aXFillAttr;
	SfxItemSet&         rXFSet;
	SfxMapUnit			ePoolUnit;

#ifdef _SVX_TPSHADOW_CXX
	DECL_LINK( ClickShadowHdl_Impl, void * );
	DECL_LINK( ModifyShadowHdl_Impl, void * );
#endif

public:
	SvxShadowTabPage( Window* pParent, const SfxItemSet& rInAttrs  );

	void    Construct();
	static  SfxTabPage* Create( Window*, const SfxItemSet& );
	static  USHORT*	    GetRanges();

	virtual BOOL FillItemSet( SfxItemSet& );
	virtual void Reset( const SfxItemSet & );
	virtual void ActivatePage( const SfxItemSet& rSet );
	virtual int  DeactivatePage( SfxItemSet* pSet );
	virtual void PointChanged( Window* pWindow, RECT_POINT eRP );

	void    SetColorTable( XColorTable* pColTab ) { pColorTab = pColTab; }
//CHINA001	void    SetPageType( USHORT* pInType ) { pPageType = pInType; }
//CHINA001	void    SetDlgType( USHORT* pInType ) { pDlgType = pInType; }
	void    SetPageType( UINT16 nInType ) { nPageType = nInType; } //add CHINA001 
	void    SetDlgType( UINT16 nInType ) { nDlgType = nInType; }	//add CHINA001 
	void    SetAreaTP( BOOL* pIn ) { pbAreaTP = pIn; }
	void    SetColorChgd( ChangeType* pIn ) { pnColorTableState = pIn; }
	virtual void PageCreated (SfxAllItemSet aSet); //add CHINA001 
	void	DisablePage( BOOL bIn ) { bDisable = bIn; }
};

/*************************************************************************
|*
|* Farbverlauf-Tab-Page
|*
\************************************************************************/

class SvxGradientTabPage : public SfxTabPage
{
	using TabPage::ActivatePage;
	using TabPage::DeactivatePage;

private:
    FixedLine           aFlProp;
	FixedText           aFtType;
	ListBox             aLbGradientType;
	FixedText           aFtCenterX;
	MetricField         aMtrCenterX;
	FixedText           aFtCenterY;
	MetricField         aMtrCenterY;
	FixedText           aFtAngle;
	MetricField         aMtrAngle;
	FixedText           aFtBorder;
	MetricField         aMtrBorder;
	FixedText           aFtColorFrom;
	ColorLB             aLbColorFrom;
	MetricField         aMtrColorFrom;
	FixedText           aFtColorTo;
	ColorLB             aLbColorTo;
	MetricField         aMtrColorTo;
	GradientLB          aLbGradients;
	SvxXRectPreview     aCtlPreview;
	PushButton          aBtnAdd;
	PushButton          aBtnModify;
	PushButton          aBtnDelete;
	ImageButton         aBtnLoad;
	ImageButton         aBtnSave;

	const SfxItemSet&   rOutAttrs;

	XColorTable*        pColorTab;
	XGradientList*      pGradientList;

	ChangeType*         pnGradientListState;
	ChangeType*         pnColorTableState;
	USHORT*             pPageType;
	USHORT*             pDlgType;
	USHORT*             pPos;
	BOOL*               pbAreaTP;

	XOutdevItemPool*    pXPool;
	XFillStyleItem      aXFStyleItem;
	XFillGradientItem   aXGradientItem;
	XFillAttrSetItem    aXFillAttr;
	SfxItemSet&         rXFSet;

#ifdef _SVX_TPGRADNT_CXX
	DECL_LINK( ClickAddHdl_Impl, void * );
	DECL_LINK( ClickModifyHdl_Impl, void * );
	DECL_LINK( ClickDeleteHdl_Impl, void * );
	DECL_LINK( ChangeGradientHdl_Impl, void * );
	DECL_LINK( ModifiedHdl_Impl, void * );
	DECL_LINK( ClickLoadHdl_Impl, void * );
	DECL_LINK( ClickSaveHdl_Impl, void * );

	long CheckChanges_Impl();
	void SetControlState_Impl( XGradientStyle eXGS );
#endif

public:
	SvxGradientTabPage( Window* pParent, const SfxItemSet& rInAttrs  );

	void    Construct();

	static  SfxTabPage* Create( Window*, const SfxItemSet& );
	virtual BOOL FillItemSet( SfxItemSet& );
	virtual void Reset( const SfxItemSet & );

	virtual void ActivatePage( const SfxItemSet& rSet );
	virtual int  DeactivatePage( SfxItemSet* pSet );

	void    SetColorTable( XColorTable* pColTab ) { pColorTab = pColTab; }
	void    SetGradientList( XGradientList* pGrdLst)
				{ pGradientList = pGrdLst; }

	void    SetPageType( USHORT* pInType ) { pPageType = pInType; }
	void    SetDlgType( USHORT* pInType ) { pDlgType = pInType; }
	void    SetPos( USHORT* pInPos ) { pPos = pInPos; }
	void    SetAreaTP( BOOL* pIn ) { pbAreaTP = pIn; }

	void    SetGrdChgd( ChangeType* pIn ) { pnGradientListState = pIn; }
	void    SetColorChgd( ChangeType* pIn ) { pnColorTableState = pIn; }
};

/*************************************************************************
|*
|* Schraffuren-Tab-Page
|*
\************************************************************************/

class SvxHatchTabPage : public SvxTabPage
{
	using TabPage::ActivatePage;
	using TabPage::DeactivatePage;

private:
	FixedText           aFtDistance;
	MetricField         aMtrDistance;
	FixedText           aFtAngle;
	MetricField         aMtrAngle;
	SvxRectCtl          aCtlAngle;
    FixedLine           aFlProp;
	FixedText           aFtLineType;
	ListBox             aLbLineType;
	FixedText           aFtLineColor;
	ColorLB             aLbLineColor;
	HatchingLB          aLbHatchings;
	SvxXRectPreview     aCtlPreview;
	PushButton          aBtnAdd;
	PushButton          aBtnModify;
	PushButton          aBtnDelete;
	ImageButton         aBtnLoad;
	ImageButton         aBtnSave;

	const SfxItemSet&   rOutAttrs;
	RECT_POINT          eRP;

	XColorTable*        pColorTab;
	XHatchList*         pHatchingList;

	ChangeType*         pnHatchingListState;
	ChangeType*         pnColorTableState;
	USHORT*             pPageType;
	USHORT*             pDlgType;
	USHORT*             pPos;
	BOOL*               pbAreaTP;

	XOutdevItemPool*    pXPool;
	XFillStyleItem      aXFStyleItem;
	XFillHatchItem      aXHatchItem;
	XFillAttrSetItem    aXFillAttr;
	SfxItemSet&         rXFSet;

	SfxMapUnit			ePoolUnit;

#ifdef _SVX_TPHATCH_CXX
	DECL_LINK( ChangeHatchHdl_Impl, void * );
	DECL_LINK( ModifiedHdl_Impl, void * );
	DECL_LINK( ClickAddHdl_Impl, void * );
	DECL_LINK( ClickModifyHdl_Impl, void * );
	DECL_LINK( ClickDeleteHdl_Impl, void * );
	DECL_LINK( ClickLoadHdl_Impl, void * );
	DECL_LINK( ClickSaveHdl_Impl, void * );

	long CheckChanges_Impl();
#endif

public:
	SvxHatchTabPage( Window* pParent, const SfxItemSet& rInAttrs  );

	void    Construct();

	static  SfxTabPage* Create( Window*, const SfxItemSet& );
	virtual BOOL FillItemSet( SfxItemSet& );
	virtual void Reset( const SfxItemSet & );

	virtual void ActivatePage( const SfxItemSet& rSet );
	virtual int  DeactivatePage( SfxItemSet* pSet );

	virtual void PointChanged( Window* pWindow, RECT_POINT eRP );

	void    SetColorTable( XColorTable* pColTab ) { pColorTab = pColTab; }
	void    SetHatchingList( XHatchList* pHtchLst)
				{ pHatchingList = pHtchLst; }

	void    SetPageType( USHORT* pInType ) { pPageType = pInType; }
	void    SetDlgType( USHORT* pInType ) { pDlgType = pInType; }
	void    SetPos( USHORT* pInPos ) { pPos = pInPos; }
	void    SetAreaTP( BOOL* pIn ) { pbAreaTP = pIn; }

	void    SetHtchChgd( ChangeType* pIn ) { pnHatchingListState = pIn; }
	void    SetColorChgd( ChangeType* pIn ) { pnColorTableState = pIn; }

	virtual void		DataChanged( const DataChangedEvent& rDCEvt );
};

/*************************************************************************
|*
|* Bitmap-Tab-Page
|*
\************************************************************************/

class SvxBitmapTabPage : public SvxTabPage
{
	using TabPage::ActivatePage;
	using TabPage::DeactivatePage;

private:
	SvxPixelCtl         aCtlPixel;
    FixedText           aFtPixelEdit;
	FixedText           aFtColor;
	ColorLB             aLbColor;
	FixedText           aFtBackgroundColor;
	ColorLB             aLbBackgroundColor;
	FixedText           aLbBitmapsHidden;
	BitmapLB            aLbBitmaps;
    FixedLine           aFlProp;
	SvxXRectPreview     aCtlPreview;
	PushButton          aBtnAdd;
	PushButton          aBtnModify;
	PushButton          aBtnImport;
	PushButton          aBtnDelete;
	ImageButton         aBtnLoad;
	ImageButton         aBtnSave;

	SvxBitmapCtl        aBitmapCtl;

	const SfxItemSet&   rOutAttrs;

	XColorTable*        pColorTab;
	XBitmapList*        pBitmapList;

	ChangeType*         pnBitmapListState;
	ChangeType*         pnColorTableState;
	USHORT*             pPageType;
	USHORT*             pDlgType;
	USHORT*             pPos;
	BOOL*               pbAreaTP;

	BOOL                bBmpChanged;

	XOutdevItemPool*    pXPool;
	XFillStyleItem      aXFStyleItem;
	XFillBitmapItem     aXBitmapItem;
	XFillAttrSetItem    aXFillAttr;
	SfxItemSet&         rXFSet;

#ifdef _SVX_TPBITMAP_CXX
	DECL_LINK( ClickAddHdl_Impl, void * );
	DECL_LINK( ClickImportHdl_Impl, void * );
	DECL_LINK( ClickModifyHdl_Impl, void * );
	DECL_LINK( ClickDeleteHdl_Impl, void * );
	DECL_LINK( ChangeBitmapHdl_Impl, void * );
	DECL_LINK( ChangePixelColorHdl_Impl, void * );
	DECL_LINK( ChangeBackgrndColorHdl_Impl, void * );
	DECL_LINK( ClickLoadHdl_Impl, void * );
	DECL_LINK( ClickSaveHdl_Impl, void * );

	long CheckChanges_Impl();
#endif

public:
	SvxBitmapTabPage( Window* pParent, const SfxItemSet& rInAttrs  );

	void    Construct();

	static  SfxTabPage* Create( Window*, const SfxItemSet& );
	virtual BOOL FillItemSet( SfxItemSet& );
	virtual void Reset( const SfxItemSet & );

	virtual void ActivatePage( const SfxItemSet& rSet );
	virtual int  DeactivatePage( SfxItemSet* pSet );

	virtual void PointChanged( Window* pWindow, RECT_POINT eRP );

	void    SetColorTable( XColorTable* pColTab ) { pColorTab = pColTab; }
	void    SetBitmapList( XBitmapList* pBmpLst) { pBitmapList = pBmpLst; }

	void    SetPageType( USHORT* pInType ) { pPageType = pInType; }
	void    SetDlgType( USHORT* pInType ) { pDlgType = pInType; }
	void    SetPos( USHORT* pInPos ) { pPos = pInPos; }
	void    SetAreaTP( BOOL* pIn ) { pbAreaTP = pIn; }

	void    SetBmpChgd( ChangeType* pIn ) { pnBitmapListState = pIn; }
	void    SetColorChgd( ChangeType* pIn ) { pnColorTableState = pIn; }

    /** Return a label that is associated with the given control.  This
        label is used to the determine the name for the control.
        @param pLabeled
            The control for which to return a label.
        @return
            Return a label control that provides a name for the specified
            control.
    */
    virtual Window*	GetParentLabeledBy( const Window* pLabeled ) const;
};

/*************************************************************************
|*
|* Farben-Tab-Page
|*
\************************************************************************/

class SvxColorTabPage : public SfxTabPage
{
	using TabPage::ActivatePage;
	using TabPage::DeactivatePage;

private:
    FixedLine           aFlProp;
	FixedText           aFtName;
	Edit                aEdtName;
    FixedText           aFtColor;
	ColorLB             aLbColor;

    FixedText           aTableNameFT;
    ValueSet            aValSetColorTable;

    SvxXRectPreview     aCtlPreviewOld;
	SvxXRectPreview     aCtlPreviewNew;

    ListBox             aLbColorModel;
    FixedText           aFtColorModel1;
	MetricField         aMtrFldColorModel1;
    FixedText           aFtColorModel2;
	MetricField         aMtrFldColorModel2;
    FixedText           aFtColorModel3;
	MetricField         aMtrFldColorModel3;

    FixedText           aFtColorModel4;
	MetricField         aMtrFldColorModel4;

    PushButton          aBtnAdd;
	PushButton          aBtnModify;
	PushButton          aBtnWorkOn;
	PushButton          aBtnDelete;
	ImageButton         aBtnLoad;
	ImageButton         aBtnSave;

	const SfxItemSet&   rOutAttrs;

	XColorTable*        pColorTab;

	ChangeType*         pnColorTableState;
	USHORT*             pPageType;
	USHORT*             pDlgType;
	USHORT*             pPos;
	BOOL*               pbAreaTP;
	BOOL                bDeleteColorTable;

	XOutdevItemPool*    pXPool;
	XFillStyleItem      aXFStyleItem;
	XFillColorItem      aXFillColorItem;
	XFillAttrSetItem    aXFillAttr;
	SfxItemSet&         rXFSet;

	ColorModel          eCM;

	Color				aAktuellColor;

#ifdef _SVX_TPCOLOR_CXX
	void    ConvertColorValues (Color& rColor, ColorModel eModell);
	void    RgbToCmyk_Impl( Color& rColor, USHORT& rK );
	void    CmykToRgb_Impl( Color& rColor, const USHORT nKey );
	USHORT  ColorToPercent_Impl( USHORT nColor );
	USHORT  PercentToColor_Impl( USHORT nPercent );

	void    FillValueSet_Impl( ValueSet& rVs );
	//-----------------------------------------------------------------------------------------------------
	DECL_LINK( ClickAddHdl_Impl, void * );		// Button 'Hinzufuegen'
	DECL_LINK( ClickModifyHdl_Impl, void * );	// Button 'Aendern'
	DECL_LINK( ClickDeleteHdl_Impl, void * );	// Button 'loeschen'
	DECL_LINK( ClickWorkOnHdl_Impl, void * );	// Button 'Bearbeiten'

	DECL_LINK( SelectColorLBHdl_Impl, void * );	// Farbe aus Listbox ausw�hlen
	DECL_LINK( SelectValSetHdl_Impl, void * );	// Farbe aus Farbpalette (links) ausw�hlen
	DECL_LINK( SelectColorModelHdl_Impl, void * );	// Auswahl Listbox 'Farbmodell'
	long ChangeColorHdl_Impl( void* p );
	DECL_LINK( ModifiedHdl_Impl, void * );		// Inhalt der Farbwerte-Felder wurde ver�ndert
	DECL_LINK( ClickLoadHdl_Impl, void * );		// Button 'Farbtabelle laden'
	DECL_LINK( ClickSaveHdl_Impl, void * );		// Button 'Farbtabelle sichern'

	long CheckChanges_Impl();
#endif

public:
	SvxColorTabPage( Window* pParent, const SfxItemSet& rInAttrs  );

	void    Construct();

	static  SfxTabPage* Create( Window*, const SfxItemSet& );
	virtual BOOL FillItemSet( SfxItemSet& );
	virtual void Reset( const SfxItemSet & );

	virtual void ActivatePage( const SfxItemSet& rSet );
	virtual int  DeactivatePage( SfxItemSet* pSet );

	void    SetColorTable( XColorTable* pColTab ) { pColorTab = pColTab; }

	void    SetPageType( USHORT* pInType ) { pPageType = pInType; }
	void    SetDlgType( USHORT* pInType ) { pDlgType = pInType; }
	void    SetPos( USHORT* pInPos ) { pPos = pInPos; }
	void    SetAreaTP( BOOL* pIn ) { pbAreaTP = pIn; }

	void    SetColorChgd( ChangeType* pIn ) { pnColorTableState = pIn; }

	void	SetDeleteColorTable( BOOL bIn ) { bDeleteColorTable = bIn; }

	virtual void FillUserData();
};


#endif // _CUI_TAB_AREA_HXX

