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

// include ---------------------------------------------------------------
#include <vcl/wrkwin.hxx>
#include <vcl/morebtn.hxx>
#include <vcl/msgbox.hxx>
#include <svtools/slstitm.hxx>
#include <svtools/itemiter.hxx>
#include <svtools/style.hxx>
#include <svtools/searchopt.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/module.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/basedlgs.hxx>
#include <svtools/cjkoptions.hxx>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/i18n/TransliterationModules.hpp>
#include <com/sun/star/frame/XDispatch.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/frame/XModuleManager.hpp>
#include <comphelper/processfactory.hxx>
#include <svtools/itempool.hxx>

#include <sfx2/app.hxx>
#include <toolkit/unohlp.hxx>

#define _SVX_SRCHDLG_CXX
#include "srchdlg.hxx"

#include <svx/dialogs.hrc>
#include <svx/svxitems.hrc>
#include "srchdlg.hrc"


#define	ITEMID_SETITEM		0

#include <sfx2/srchitem.hxx>
#include <svx/pageitem.hxx>
#include "srchctrl.hxx"
#include <svx/dialmgr.hxx>
#include "dlgutil.hxx"
#include <svx/brshitem.hxx>

#include <svx/svxdlg.hxx> //CHINA001

#include <sfx2/layout-pre.hxx>

using namespace com::sun::star::i18n;
using namespace com::sun::star;
using namespace comphelper;
// -----------------------------------------------------------------------

#define REMEMBER_SIZE		10

#define MODIFY_SEARCH		0x00000001
#define MODIFY_REPLACE		0x00000002
#define MODIFY_WORD			0x00000004
#define MODIFY_EXACT		0x00000008
#define MODIFY_BACKWARDS	0x00000010
#define MODIFY_SELECTION	0x00000020
#define MODIFY_REGEXP		0x00000040
#define MODIFY_LAYOUT		0x00000080
#define MODIFY_SIMILARITY	0x00000100
#define MODIFY_FORMULAS		0x00000200
#define MODIFY_VALUES		0x00000400
#define MODIFY_CALC_NOTES   0x00000800
#define MODIFY_ROWS			0x00001000
#define MODIFY_COLUMNS		0x00002000
#define MODIFY_ALLTABLES    0x00004000
#define MODIFY_NOTES        0x00008000

SV_IMPL_VARARR(SrchAttrItemList, SearchAttrItem);

//#define NotifyApp( nId )
//    rBindings.ExecuteSynchron( nId, (const SfxPoolItem**)&pSearchItem, 0L )

#define GetCheckBoxValue( rBox )								\
	rBox.IsEnabled() ? rBox.IsChecked() : FALSE

#if ENABLE_LAYOUT
#undef SVX_RES
#define SVX_RES(x) #x
#endif /* ENABLE_LAYOUT */

struct SearchDlg_Impl
{
	FixedText	aSearchFormats;
	FixedText	aReplaceFormats;

    BOOL        bMultiLineEdit   : 1,
                bSaveToModule    : 1,
                bFocusOnSearch   : 1,
                bDeltaCalculated : 1;
	USHORT*		pRanges;
	Timer		aSelectionTimer;

    uno::Reference< frame::XDispatch > xCommand1Dispatch;
    uno::Reference< frame::XDispatch > xCommand2Dispatch;
    util::URL   aCommand1URL;
    util::URL   aCommand2URL;

#if ENABLE_LAYOUT
    SearchDlg_Impl( layout::Context* pParent ) :
#else /* !ENABLE_LAYOUT */
        SearchDlg_Impl( Window* pParent ) :
#endif /* !ENABLE_LAYOUT */
        aSearchFormats  ( pParent, SVX_RES( FT_SEARCH_FORMATS ) ),
        aReplaceFormats ( pParent, SVX_RES( FT_REPLACE_FORMATS ) ),
        bMultiLineEdit  ( FALSE ),
        bSaveToModule   ( TRUE ),
        bFocusOnSearch  ( TRUE ),
        bDeltaCalculated( FALSE ),
        pRanges         ( NULL )
        {
            aCommand1URL.Complete = aCommand1URL.Main = rtl::OUString::createFromAscii("vnd.sun.search:SearchViaComponent1");
            aCommand1URL.Protocol = rtl::OUString::createFromAscii("vnd.sun.search:");
            aCommand1URL.Path = rtl::OUString::createFromAscii("SearchViaComponent1");
            aCommand2URL.Complete = aCommand2URL.Main = rtl::OUString::createFromAscii("vnd.sun.search:SearchViaComponent2");
            aCommand2URL.Protocol = rtl::OUString::createFromAscii("vnd.sun.search:");
            aCommand2URL.Path = rtl::OUString::createFromAscii("SearchViaComponent2");
        }
	~SearchDlg_Impl() { delete[] pRanges; }
};

// -----------------------------------------------------------------------

void ListToStrArr_Impl( USHORT nId, SvStringsDtor& rStrLst, ComboBox& rCBox )
{
	SfxStringListItem* pSrchItem =
		(SfxStringListItem*)SFX_APP()->GetItem( nId );
	List* pLst = pSrchItem ? pSrchItem->GetList() : 0;

	if ( pLst )
		for ( USHORT i = 0; i < pLst->Count(); ++i )
		{
			String* pTmp = new String( *(String*)( pLst->GetObject(i) ) );
			rStrLst.Insert( pTmp, rStrLst.Count() );
			rCBox.InsertEntry( *pTmp );
		}
}

// -----------------------------------------------------------------------

void StrArrToList_Impl( USHORT nId, const SvStringsDtor& rStrLst )
{
	DBG_ASSERT( rStrLst.Count(), "vorher abpruefen!!" );
	List aLst;

	for ( USHORT i = 0; i < rStrLst.Count(); ++i )
		aLst.Insert( rStrLst[ i ], LIST_APPEND );

	SFX_APP()->PutItem( SfxStringListItem( nId, &aLst ) );
}

// class SearchAttrItemList ----------------------------------------------

SearchAttrItemList::SearchAttrItemList( const SearchAttrItemList& rList ) :

	SrchAttrItemList( (BYTE)rList.Count() )

{
	SrchAttrItemList::Insert( &rList, 0 );
    SearchAttrItem* _pData = (SearchAttrItem*)GetData();

    for ( USHORT i = Count(); i; --i, ++_pData )
        if ( !IsInvalidItem( _pData->pItem ) )
            _pData->pItem = _pData->pItem->Clone();
}

// -----------------------------------------------------------------------

SearchAttrItemList::~SearchAttrItemList()
{
	Clear();
}

// -----------------------------------------------------------------------

void SearchAttrItemList::Put( const SfxItemSet& rSet )
{
	if ( !rSet.Count() )
		return;

	SfxItemPool* pPool = rSet.GetPool();
	SfxItemIter aIter( rSet );
	SearchAttrItem aItem;
	const SfxPoolItem* pItem = aIter.GetCurItem();
	USHORT nWhich;

	while ( TRUE )
	{
		// nur testen, ob vorhanden ist ?
		if( IsInvalidItem( pItem ) )
		{
			nWhich = rSet.GetWhichByPos( aIter.GetCurPos() );
			aItem.pItem = (SfxPoolItem*)pItem;
		}
		else
		{
			nWhich = pItem->Which();
			aItem.pItem = pItem->Clone();
		}

		aItem.nSlot = pPool->GetSlotId( nWhich );
		Insert( aItem );

		if ( aIter.IsAtEnd() )
			break;
		pItem = aIter.NextItem();
	}
}

// -----------------------------------------------------------------------

SfxItemSet& SearchAttrItemList::Get( SfxItemSet& rSet )
{
	SfxItemPool* pPool = rSet.GetPool();
    SearchAttrItem* _pData = (SearchAttrItem*)GetData();

    for ( USHORT i = Count(); i; --i, ++_pData )
        if ( IsInvalidItem( _pData->pItem ) )
            rSet.InvalidateItem( pPool->GetWhich( _pData->nSlot ) );
		else
            rSet.Put( *_pData->pItem );
	return rSet;
}

// -----------------------------------------------------------------------

void SearchAttrItemList::Clear()
{
    SearchAttrItem* _pData = (SearchAttrItem*)GetData();

    for ( USHORT i = Count(); i; --i, ++_pData )
        if ( !IsInvalidItem( _pData->pItem ) )
            delete _pData->pItem;
	SrchAttrItemList::Remove( 0, Count() );
}

// -----------------------------------------------------------------------

// l"oscht die Pointer auf die Items
void SearchAttrItemList::Remove( USHORT nPos, USHORT nLen )
{
	if ( nPos + nLen > Count() )
		nLen = Count() - nPos;
    SearchAttrItem* _pData = (SearchAttrItem*)GetData() + nPos;

    for ( USHORT n = nLen; n; --n, ++_pData )
        if ( !IsInvalidItem( _pData->pItem ) )
            delete _pData->pItem;

	SrchAttrItemList::Remove( nPos, nLen );
}

/* //CHINA001
// class SvxJSearchOptionsDialog -----------------------------------------

SvxJSearchOptionsDialog::SvxJSearchOptionsDialog(
			Window *pParent,
			const SfxItemSet& rOptionsSet, USHORT nUniqueId, INT32 nInitialFlags ) :
	SfxSingleTabDialog	( pParent, rOptionsSet, RID_SVXPAGE_JSEARCH_OPTIONS ),
	nInitialTlFlags( nInitialFlags )
{
	pPage = (SvxJSearchOptionsPage *)
					SvxJSearchOptionsPage::Create( this, rOptionsSet );
	SetTabPage( pPage );	//! implicitly calls pPage->Reset(...)!
	pPage->EnableSaveOptions( FALSE );
}


SvxJSearchOptionsDialog::~SvxJSearchOptionsDialog()
{
	// pPage will be implicitly destroyed by the
	// SfxSingleTabDialog destructor
}


void SvxJSearchOptionsDialog::Activate()
{
	pPage->SetTransliterationFlags( nInitialTlFlags );
}


INT32 SvxJSearchOptionsDialog::GetTransliterationFlags() const
{
	return pPage->GetTransliterationFlags();
}


void SvxJSearchOptionsDialog::SetTransliterationFlags( INT32 nSettings )
{
	pPage->SetTransliterationFlags( nSettings );
}
*/ //CHINA001

#if ENABLE_LAYOUT
#undef SfxModelessDialog
#define SfxModelessDialog(bindings, child, parent, id) SfxDialog (parent, "find-and-replace.xml", id, bindings, child)
#define SVX_RES_PLAIN(x) ResId (x, DIALOG_MGR ())
#define THIS_SVX_RES(x) this, #x
#else /* !ENABLE_LAYOUT */
#define SVX_RES_PLAIN SVX_RES
#define THIS_SVX_RES SVX_RES
#endif /* !ENABLE_LAYOUT */

#undef INI_LIST
#define INI_LIST() \
	aSearchText 	( this, SVX_RES( FT_SEARCH ) ),							\
	aSearchLB		( this, SVX_RES( ED_SEARCH ) ),							\
	aSearchTmplLB	( this, SVX_RES( LB_SEARCH ) ),							\
	aSearchAttrText ( this, SVX_RES( FT_SEARCH_ATTR ) ),						\
	aReplaceText	( this, SVX_RES( FT_REPLACE ) ),							\
	aReplaceLB		( this, SVX_RES( ED_REPLACE ) ),							\
	aReplaceTmplLB	( this, SVX_RES( LB_REPLACE ) ),							\
	aReplaceAttrText( this, SVX_RES( FT_REPLACE_ATTR ) ),						\
    aSearchBtn      ( this, SVX_RES( BTN_SEARCH ) ),                          \
	aSearchAllBtn	( this, SVX_RES( BTN_SEARCH_ALL ) ),						\
    aSearchCmdLine  ( this, SVX_RES( FL_SEARCH_COMMAND ) ),                   \
    aReplaceBtn     ( this, SVX_RES( BTN_REPLACE ) ),                         \
    aReplaceAllBtn  ( this, SVX_RES( BTN_REPLACE_ALL ) ),                     \
    aSearchComponentFL( this, SVX_RES( FL_SEARCH_COMPONENT ) ), \
    aSearchComponent1PB( this, SVX_RES( BTN_COMPONENT_1 ) ), \
    aSearchComponent2PB( this, SVX_RES( BTN_COMPONENT_2 ) ), \
    aMatchCaseCB    ( this, SVX_RES( CB_MATCH_CASE ) ),                       \
    aWordBtn        ( this, SVX_RES( CB_WHOLE_WORDS ) ),                      \
    aButtonsFL      ( this, SVX_RES( FL_BUTTONS ) ),                          \
    pMoreBtn        ( new MoreButton( this, SVX_RES( BTN_MORE ) ) ),          \
    aHelpBtn        ( this, SVX_RES( BTN_HELP ) ),                            \
    aCloseBtn       ( this, SVX_RES( BTN_CLOSE ) ),                           \
    aOptionsFL      ( this, SVX_RES( FL_OPTIONS ) ),                          \
    aSelectionBtn   ( this, SVX_RES( CB_SELECTIONS ) ),                       \
    aBackwardsBtn   ( this, SVX_RES( CB_BACKWARDS ) ),                        \
    aRegExpBtn      ( this, SVX_RES( CB_REGEXP ) ),                           \
    aSimilarityBox  ( this, SVX_RES( CB_SIMILARITY) ),                        \
    aSimilarityBtn  ( this, SVX_RES( PB_SIMILARITY) ),                        \
    aLayoutBtn      ( this, SVX_RES( CB_LAYOUTS ) ),                          \
    aNotesBtn       ( this, SVX_RES( CB_NOTES ) ),                            \
    aJapMatchFullHalfWidthCB( this, SVX_RES( CB_JAP_MATCH_FULL_HALF_WIDTH ) ),\
    aJapOptionsCB   ( this, SVX_RES( CB_JAP_SOUNDS_LIKE ) ),                  \
    aJapOptionsBtn  ( this, SVX_RES( PB_JAP_OPTIONS ) ),                      \
    aAttributeBtn   ( this, SVX_RES( BTN_ATTRIBUTE ) ),                       \
	aFormatBtn		( this, SVX_RES( BTN_FORMAT ) ),							\
	aNoFormatBtn	( this, SVX_RES( BTN_NOFORMAT ) ),						\
    aCalcFL         ( this, SVX_RES( FL_CALC ) ),                             \
    aCalcSearchInFT ( this, SVX_RES( FT_CALC_SEARCHIN ) ),                    \
    aCalcSearchInLB ( this, SVX_RES( LB_CALC_SEARCHIN ) ),                    \
    aCalcSearchDirFT( this, SVX_RES( FT_CALC_SEARCHDIR ) ),                   \
    aRowsBtn        ( this, SVX_RES( RB_CALC_ROWS ) ),                        \
    aColumnsBtn     ( this, SVX_RES( RB_CALC_COLUMNS ) ),                     \
    aAllSheetsCB    ( this, SVX_RES( CB_ALL_SHEETS ) ),                       \
    rBindings       ( rBind ),                                              \
    bWriter         ( FALSE ),                                              \
    bSearch         ( TRUE ),                                               \
	bFormat 		( FALSE ),												\
	nOptions		( USHRT_MAX ),											\
	bSet			( FALSE ),												\
	bReadOnly		( FALSE ),												\
	bConstruct		( TRUE ),												\
	nModifyFlag		( 0 ),													\
    aCalcStr        ( THIS_SVX_RES( STR_WORDCALC ) ),                       \
    pImpl           ( NULL ),                                               \
	pSearchList 	( NULL ),												\
	pReplaceList	( new SearchAttrItemList ),                             \
	pSearchItem 	( NULL ),												\
	pSearchController		( NULL ),										\
	pOptionsController		( NULL ),										\
	pFamilyController		( NULL ),										\
	pSearchSetController	( NULL ),										\
	pReplaceSetController	( NULL ),										\
	nTransliterationFlags	( 0x00000000 )

// class SvxSearchDialog -------------------------------------------------

SvxSearchDialog::SvxSearchDialog( Window* pParent, SfxBindings& rBind ) :

	SfxModelessDialog( &rBind, NULL, pParent, SVX_RES( RID_SVXDLG_SEARCH ) ),

	INI_LIST()

{
	Construct_Impl();
}

// -----------------------------------------------------------------------

SvxSearchDialog::SvxSearchDialog( Window* pParent, SfxChildWindow* pChildWin, SfxBindings& rBind ) :

	SfxModelessDialog( &rBind, pChildWin, pParent, SVX_RES( RID_SVXDLG_SEARCH ) ),

	INI_LIST()

{
	Construct_Impl();
}

#undef INI_LIST
#if ENABLE_LAYOUT
#undef SVX_RES
#define SVX_RES(x) ResId (x, DIALOG_MGR ())
#endif

// -----------------------------------------------------------------------

SvxSearchDialog::~SvxSearchDialog()
{
	Hide();

	rBindings.EnterRegistrations();
	delete pSearchController;
	delete pOptionsController;
	delete pFamilyController;
	delete pSearchSetController;
	delete pReplaceSetController;
	rBindings.LeaveRegistrations();

	delete pSearchItem;
	delete pImpl;
	delete pSearchList;
	delete pReplaceList;
	delete pMoreBtn;
}

#if ENABLE_LAYOUT
#undef Window
#define Window layout::Window
#endif /* ENABLE_LAYOUT */

void lcl_MoveDown( Window& rWindow, sal_Int32 nOffset )
{
    Point aPos(rWindow.GetPosPixel());
    aPos.Y() += nOffset;
    rWindow.SetPosPixel(aPos);
}

void SvxSearchDialog::Construct_Impl()
{
#if ENABLE_LAYOUT
    SetHelpId (SID_SEARCH_DLG);
#endif /* ENABLE_LAYOUT */

	// temporary to avoid incompatibility
	pImpl = new SearchDlg_Impl( this );
#if !ENABLE_LAYOUT
	pImpl->aSelectionTimer.SetTimeout( 500 );
	pImpl->aSelectionTimer.SetTimeoutHdl(
		LINK( this, SvxSearchDialog, TimeoutHdl_Impl ) );
#endif /* !ENABLE_LAYOUT */
	EnableControls_Impl( 0 );

	// alten Text des aWordBtn's merken
    aCalcStr += sal_Unicode('#');
    aCalcStr += aWordBtn.GetText();

	aLayoutStr = SVX_RESSTR( RID_SVXSTR_SEARCH_STYLES );
	aStylesStr = aLayoutBtn.GetText();

	// gemerkte Such-Strings von der Applikation holen
	ListToStrArr_Impl( SID_SEARCHDLG_SEARCHSTRINGS,
					   aSearchStrings, aSearchLB 	);
	ListToStrArr_Impl( SID_SEARCHDLG_REPLACESTRINGS,
					   aReplaceStrings, aReplaceLB 	);

    pMoreBtn->SetMoreText( String( SVX_RES( STR_MORE_BTN ) ) );
    pMoreBtn->SetLessText( String( SVX_RES( STR_LESS_BTN ) ) );

    FreeResource();
	InitControls_Impl();

	// Attribut-Sets nur einmal im Ctor() besorgen
    const SfxPoolItem* ppArgs[] = { pSearchItem, 0 };
	const SvxSetItem* pSrchSetItem =
        (const SvxSetItem*) rBindings.GetDispatcher()->Execute( FID_SEARCH_SEARCHSET, SFX_CALLMODE_SLOT, ppArgs );

	if ( pSrchSetItem )
		InitAttrList_Impl( &pSrchSetItem->GetItemSet(), 0 );

	const SvxSetItem* pReplSetItem =
        (const SvxSetItem*)rBindings.GetDispatcher()->Execute( FID_SEARCH_REPLACESET, SFX_CALLMODE_SLOT, ppArgs );

	if ( pReplSetItem )
		InitAttrList_Impl( 0, &pReplSetItem->GetItemSet() );

	// Controller erzeugen und gleich aktualisieren
	rBindings.EnterRegistrations();
	pSearchController =
		new SvxSearchController( SID_SEARCH_ITEM, rBindings, *this );
	pOptionsController =
		new SvxSearchController( SID_SEARCH_OPTIONS, rBindings, *this );
	rBindings.LeaveRegistrations();
    rBindings.GetDispatcher()->Execute( FID_SEARCH_ON, SFX_CALLMODE_SLOT, ppArgs );
	pImpl->aSelectionTimer.Start();


    SvtCJKOptions aCJKOptions;
    if(!aCJKOptions.IsJapaneseFindEnabled())
    {
        aJapOptionsCB.Check( FALSE );
        aJapOptionsCB.Hide();
        aJapOptionsBtn.Hide();
    }
    if(!aCJKOptions.IsCJKFontEnabled())
    {
        aJapMatchFullHalfWidthCB.Hide();
    }
    //component extension - show component search buttons if the commands
    // vnd.sun.star::SearchViaComponent1 and 2 are supported
    const uno::Reference< frame::XFrame >xFrame = rBindings.GetActiveFrame();
    const uno::Reference< frame::XDispatchProvider > xDispatchProv(xFrame, uno::UNO_QUERY);
    rtl::OUString sTarget = rtl::OUString::createFromAscii("_self");

    bool bSearchComponent1 = false;
    bool bSearchComponent2 = false;
    if(xDispatchProv.is() &&
            (pImpl->xCommand1Dispatch = xDispatchProv->queryDispatch(pImpl->aCommand1URL, sTarget, 0)).is())
    {
        bSearchComponent1 = true;
    }
    if(xDispatchProv.is() &&
            (pImpl->xCommand2Dispatch = xDispatchProv->queryDispatch(pImpl->aCommand2URL, sTarget, 0)).is())
    {
        bSearchComponent2 = true;
    }

    if( bSearchComponent1 || bSearchComponent2 )
    {
        //get the labels of the FixedLine and the buttons
        // "/org.openoffice.Office.Common/SearchOptions/ComponentSearchGroupLabel
        // "/org.openoffice.Office.Common/SearchOptions/ComponentSearchCommandLabel1
        // "/org.openoffice.Office.Common/SearchOptions/ComponentSearchCommandLabel2
        try
        {
            uno::Reference< lang::XMultiServiceFactory >  xMgr = getProcessServiceFactory();
            uno::Reference< lang::XMultiServiceFactory > xConfigurationProvider(xMgr->createInstance(
                    ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.configuration.ConfigurationProvider"))),
                    uno::UNO_QUERY);
            uno::Sequence< uno::Any > aArgs(1);
            ::rtl::OUString sPath(RTL_CONSTASCII_USTRINGPARAM( "/org.openoffice.Office.Common/SearchOptions/"));
            aArgs[0] <<= sPath;

            uno::Reference< uno::XInterface > xIFace = xConfigurationProvider->createInstanceWithArguments(
                        ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.configuration.ConfigurationUpdateAccess")),
                        aArgs);
            uno::Reference< container::XNameAccess> xDirectAccess(xIFace, uno::UNO_QUERY);
            if(xDirectAccess.is())
            {
                ::rtl::OUString sTemp;
                ::rtl::OUString sProperty(RTL_CONSTASCII_USTRINGPARAM( "ComponentSearchGroupLabel"));
                uno::Any aRet = xDirectAccess->getByName(sProperty);
                aRet >>= sTemp;
                aSearchComponentFL.SetText( sTemp );
                aRet = xDirectAccess->getByName(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM( "ComponentSearchCommandLabel1")));
                aRet >>= sTemp;
                aSearchComponent1PB.SetText( sTemp );
                aRet = xDirectAccess->getByName(::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM( "ComponentSearchCommandLabel2")));
                aRet >>= sTemp;
                aSearchComponent2PB.SetText( sTemp );
            }
        }
        catch(uno::Exception&){}

        if(aSearchComponent1PB.GetText().Len() && bSearchComponent1 )
        {
            aSearchComponentFL.Show();
            aSearchComponent1PB.Show();
        }
        if( aSearchComponent2PB.GetText().Len() )
        {
            if(!aSearchComponent1PB.IsVisible())
            {
                aSearchComponent2PB.SetPosPixel(aSearchComponent1PB.GetPosPixel());
            }
            aSearchComponentFL.Show();
            aSearchComponent2PB.Show();
        }
        if( aSearchComponentFL.IsVisible() && aSearchComponent1PB.IsVisible() )
        {

            //dialog must be resized
            Size aDlgSize(GetSizePixel());
            sal_Int32 nOffset = aSearchCmdLine.GetPosPixel().Y() - aSearchAllBtn.GetPosPixel().Y()
                - aButtonsFL.GetPosPixel().Y() + aSearchComponent2PB.GetPosPixel().Y();

            aDlgSize.Height() += nOffset;
            Window* aWindows[] =
            {
                &aOptionsFL,
                &aSelectionBtn,
                &aBackwardsBtn,
                &aRegExpBtn,
                &aSimilarityBox,
                &aSimilarityBtn,
                &aLayoutBtn,
                &aNotesBtn,
                &aJapMatchFullHalfWidthCB,
                &aJapOptionsCB,
                &aJapOptionsBtn,
                &aAttributeBtn,
                &aFormatBtn,
                &aNoFormatBtn,
                &aCalcFL,
                &aCalcSearchInFT,
                &aCalcSearchInLB,
                &aCalcSearchDirFT,
                &aRowsBtn,
                &aColumnsBtn,
                &aAllSheetsCB,
                &aButtonsFL,
                &aHelpBtn,
                &aCloseBtn,
                pMoreBtn,
                0
            };
            sal_Int32 nWindow = 0;
            do
            {
                lcl_MoveDown( *aWindows[nWindow], nOffset );
            }
            while(aWindows[++nWindow]);

            SetSizePixel(aDlgSize);
        }
    }
}

// -----------------------------------------------------------------------

BOOL SvxSearchDialog::Close()
{
	// remember strings speichern
	if ( aSearchStrings.Count() )
		StrArrToList_Impl( SID_SEARCHDLG_SEARCHSTRINGS, aSearchStrings );

	if ( aReplaceStrings.Count() )
		StrArrToList_Impl( SID_SEARCHDLG_REPLACESTRINGS, aReplaceStrings );

	// save settings to configuration
	SvtSearchOptions aOpt;
	aOpt.SetWholeWordsOnly			( aWordBtn				  .IsChecked() );
	aOpt.SetBackwards				( aBackwardsBtn			  .IsChecked() );
	aOpt.SetUseRegularExpression	( aRegExpBtn			  .IsChecked() );
	//aOpt.SetMatchCase				( aMatchCaseCB			  .IsChecked() );
	aOpt.SetSearchForStyles			( aLayoutBtn			  .IsChecked() );
	aOpt.SetSimilaritySearch		( aSimilarityBox		  .IsChecked() );
    //aOpt.SetMatchFullHalfWidthForms   ( !aJapMatchFullHalfWidthCB.IsChecked() );
	aOpt.SetUseAsianOptions			( aJapOptionsCB		  	  .IsChecked() );
    	aOpt.SetNotes                   ( aNotesBtn               .IsChecked() );

    const SfxPoolItem* ppArgs[] = { pSearchItem, 0 };
    rBindings.GetDispatcher()->Execute( FID_SEARCH_OFF, SFX_CALLMODE_SLOT, ppArgs );
    rBindings.Execute( SID_SEARCH_DLG );

	return TRUE;
}

// -----------------------------------------------------------------------

INT32 SvxSearchDialog::GetTransliterationFlags() const
{
    if (!aMatchCaseCB.IsChecked())
		nTransliterationFlags |=  TransliterationModules_IGNORE_CASE;
	else
		nTransliterationFlags &= ~TransliterationModules_IGNORE_CASE;
    if ( !aJapMatchFullHalfWidthCB.IsChecked())
		nTransliterationFlags |=  TransliterationModules_IGNORE_WIDTH;
	else
		nTransliterationFlags &= ~TransliterationModules_IGNORE_WIDTH;
	return nTransliterationFlags;
}

// -----------------------------------------------------------------------

void SvxSearchDialog::ApplyTransliterationFlags_Impl( INT32 nSettings )
{
	nTransliterationFlags = nSettings;
	BOOL bVal = 0 != (nSettings & TransliterationModules_IGNORE_CASE);
    aMatchCaseCB            .Check(!bVal );
	bVal = 0 != (nSettings & TransliterationModules_IGNORE_WIDTH);
    aJapMatchFullHalfWidthCB.Check( !bVal );
}

// -----------------------------------------------------------------------

void SvxSearchDialog::Activate()
{
	// apply possible transliteration changes of the SvxSearchItem member
	DBG_ASSERT( pSearchItem, "SearchItem missing" );
	if (pSearchItem)
	{
		aMatchCaseCB            .Check( pSearchItem->GetExact() );
        aJapMatchFullHalfWidthCB.Check( !pSearchItem->IsMatchFullHalfWidthForms() );
	}
}

// -----------------------------------------------------------------------

void SvxSearchDialog::InitControls_Impl()
{
	// CaseSensitives AutoComplete
	aSearchLB.EnableAutocomplete( TRUE, TRUE );
	aSearchLB.Show();
	aReplaceLB.EnableAutocomplete( TRUE, TRUE );
	aReplaceLB.Show();

	aFormatBtn.Disable();
	aAttributeBtn.Disable();

	aSearchLB.SetModifyHdl( LINK( this, SvxSearchDialog, ModifyHdl_Impl ) );
	aReplaceLB.SetModifyHdl( LINK( this, SvxSearchDialog, ModifyHdl_Impl ) );

	Link aLink = LINK( this, SvxSearchDialog, FocusHdl_Impl );
	aSearchLB.SetGetFocusHdl( aLink );
	pImpl->aSearchFormats.SetGetFocusHdl( aLink );

	aReplaceLB.SetGetFocusHdl( aLink );
	pImpl->aReplaceFormats.SetGetFocusHdl( aLink );

	aLink = LINK( this, SvxSearchDialog, LoseFocusHdl_Impl );
	aSearchLB.SetLoseFocusHdl( aLink );
	aReplaceLB.SetLoseFocusHdl( aLink );

	aSearchTmplLB.SetLoseFocusHdl( aLink );
	aReplaceTmplLB.SetLoseFocusHdl( aLink );

	aLink = LINK( this, SvxSearchDialog, CommandHdl_Impl );
	aSearchBtn.SetClickHdl( aLink );
	aSearchAllBtn.SetClickHdl( aLink );
	aReplaceBtn.SetClickHdl( aLink );
	aReplaceAllBtn.SetClickHdl( aLink );
	aCloseBtn.SetClickHdl( aLink );
	aSimilarityBtn.SetClickHdl( aLink );
	aJapOptionsBtn.SetClickHdl( aLink );
    aSearchComponent1PB.SetClickHdl( aLink );
    aSearchComponent2PB.SetClickHdl( aLink );

	aLink = LINK( this, SvxSearchDialog, FlagHdl_Impl );
	aWordBtn.SetClickHdl( aLink );
	aSelectionBtn.SetClickHdl( aLink );
	aMatchCaseCB.SetClickHdl( aLink );
	aRegExpBtn.SetClickHdl( aLink );
    aBackwardsBtn.SetClickHdl( aLink );
    aNotesBtn.SetClickHdl( aLink );
	aSimilarityBox.SetClickHdl( aLink );
	aJapOptionsCB.SetClickHdl( aLink );
	aJapMatchFullHalfWidthCB.SetClickHdl( aLink );

	aLayoutBtn.SetClickHdl( LINK( this, SvxSearchDialog, TemplateHdl_Impl ) );
	aFormatBtn.SetClickHdl( LINK( this, SvxSearchDialog, FormatHdl_Impl ) );
	aNoFormatBtn.SetClickHdl(
		LINK( this, SvxSearchDialog, NoFormatHdl_Impl ) );
	aAttributeBtn.SetClickHdl(
		LINK( this, SvxSearchDialog, AttributeHdl_Impl ) );

    // check if buttontext is to wide
    long nTxtW = Max( pMoreBtn->GetCtrlTextWidth( pMoreBtn->GetMoreText() ),
                      pMoreBtn->GetCtrlTextWidth( pMoreBtn->GetLessText() ) );
    nTxtW += ( pMoreBtn->GetTextHeight() * 2 ); // add image size + offset
    long nBtnW = pMoreBtn->GetSizePixel().Width();
    if ( nTxtW > nBtnW )
    {
        // broaden the button
        const long nMinDelta = 10;
        long nDelta = Max( nTxtW - nBtnW, nMinDelta );
        Size aNewSize = pMoreBtn->GetSizePixel();
        aNewSize.Width() += nDelta;
        pMoreBtn->SetSizePixel( aNewSize );
    }
}

// -----------------------------------------------------------------------

void SvxSearchDialog::CalculateDelta_Impl()
{
    DBG_ASSERT( pSearchItem, "no search item" );

    bool bDrawApp = false;
    bool bCalcApp = false;
    bool bWriterApp = false;
    bool bImpressApp = false;
    const uno::Reference< frame::XFrame > xFrame = rBindings.GetActiveFrame();
    uno::Reference< frame::XModuleManager > xModuleManager(
        ::comphelper::getProcessServiceFactory()->createInstance(
            DEFINE_CONST_UNICODE("com.sun.star.frame.ModuleManager") ), uno::UNO_QUERY );
    if ( xModuleManager.is() )
    {
        try
        {
            ::rtl::OUString aModuleIdentifier = xModuleManager->identify( xFrame );
            bCalcApp = aModuleIdentifier.equalsAscii( "com.sun.star.sheet.SpreadsheetDocument" );
            bDrawApp = aModuleIdentifier.equalsAscii( "com.sun.star.drawing.DrawingDocument" );
            bImpressApp = aModuleIdentifier.equalsAscii( "com.sun.star.presentation.PresentationDocument" );
            bWriterApp = aModuleIdentifier.equalsAscii( "com.sun.star.text.TextDocument" );
        }
        catch ( uno::Exception& )
        {
        }
    }

    if ( pImpl->bDeltaCalculated )
        return;
    else
        pImpl->bDeltaCalculated = TRUE;

    ULONG nDelta = 187, nOffset = 0;
    SvtCJKOptions aCJKOptions;

    pMoreBtn->AddWindow( &aOptionsFL );
    if ( !bDrawApp )
        pMoreBtn->AddWindow( &aLayoutBtn );
    if ( bWriterApp )
        pMoreBtn->AddWindow( &aNotesBtn );
	else
	{
		aNotesBtn.Hide();
        nOffset = !bDrawApp ? 13 : 0;
	}
    pMoreBtn->AddWindow( &aBackwardsBtn );
    if ( !bDrawApp )
        pMoreBtn->AddWindow( &aRegExpBtn );
    pMoreBtn->AddWindow( &aSimilarityBox );
    pMoreBtn->AddWindow( &aSimilarityBtn );
    pMoreBtn->AddWindow( &aSelectionBtn );

    if ( aCJKOptions.IsCJKFontEnabled() )
        pMoreBtn->AddWindow( &aJapMatchFullHalfWidthCB );
    else
        nOffset += 13;
    if ( aCJKOptions.IsJapaneseFindEnabled() )
    {
        pMoreBtn->AddWindow( &aJapOptionsCB );
        pMoreBtn->AddWindow( &aJapOptionsBtn );
    }
    else
        nOffset += 17;

    if ( bWriter )
    {
        pMoreBtn->AddWindow( &aAttributeBtn );
        pMoreBtn->AddWindow( &aFormatBtn );
        pMoreBtn->AddWindow( &aNoFormatBtn );
    }

    if ( bDrawApp )
    {
        // Draw App: "Regular expressions" and "Search for Styles" check boxes are hidden
        // so align the other buttons
        const long nAppFontHeight = 13; // checkbox height + space between in APPFONT
        long nH = LogicToPixel( Size( 0, nAppFontHeight ), MAP_APPFONT ).Height();

        Point aNewPos = aSimilarityBox.GetPosPixel();
        aNewPos.Y() -= nH;
        aSimilarityBox.SetPosPixel( aNewPos );
        aNewPos = aSimilarityBtn.GetPosPixel();
        aNewPos.Y() -= nH;
        aSimilarityBtn.SetPosPixel( aNewPos );
        nH *= 3;
        nOffset += ( 3 * nAppFontHeight );
        if ( aCJKOptions.IsCJKFontEnabled() )
        {
            aNewPos = aJapMatchFullHalfWidthCB.GetPosPixel();
            aNewPos.Y() -= nH;
            aJapMatchFullHalfWidthCB.SetPosPixel( aNewPos );
        }
        if ( aCJKOptions.IsJapaneseFindEnabled() )
        {
            aNewPos = aJapOptionsCB.GetPosPixel();
            aNewPos.Y() -= nH;
            aJapOptionsCB.SetPosPixel( aNewPos );
            aNewPos = aJapOptionsBtn.GetPosPixel();
            aNewPos.Y() -= nH;
            aJapOptionsBtn.SetPosPixel( aNewPos );
        }
    }

    if ( bCalcApp || bImpressApp )
    {
        Window* pWins[] =
        {
            &aCalcFL, &aCalcSearchInFT, &aCalcSearchInLB, &aCalcSearchDirFT,
            &aRowsBtn, &aColumnsBtn, &aAllSheetsCB, &aJapMatchFullHalfWidthCB,
			&aJapOptionsCB, &aJapOptionsBtn
        };
        Window** pCurrent = pWins;
        sal_uInt32 i = 0;
        const sal_uInt32 nCalcCtrlCount = 7;
        if ( nOffset > 0 )
        {
            long nH = LogicToPixel( Size( 0, nOffset ), MAP_APPFONT ).Height();
            for ( i = 0; i < sizeof( pWins ) / sizeof( pWins[ 0 ] ); ++i, ++pCurrent )
            {
                if ( ( bCalcApp && i < nCalcCtrlCount )
                    || ( i == nCalcCtrlCount && aCJKOptions.IsCJKFontEnabled() )
                    || ( i > nCalcCtrlCount && aCJKOptions.IsJapaneseFindEnabled() ) )
				{
					Point aNewPos = (*pCurrent)->GetPosPixel();
					aNewPos.Y() -= nH;
					(*pCurrent)->SetPosPixel( aNewPos );
				}
            }
        }

        if ( bCalcApp)
        {
            pCurrent = pWins;
            for ( i = 0; i < nCalcCtrlCount; ++i, ++pCurrent )
                pMoreBtn->AddWindow( *pCurrent );
        }
        else
            nOffset += 64;
    }
    else
        nOffset += 64;

    pMoreBtn->SetDelta( nDelta - nOffset );
    pMoreBtn->Show();
    pMoreBtn->Enable();
}

#if ENABLE_LAYOUT
#undef Window
#define Window ::Window
#endif /* ENABLE_LAYOUT */

// -----------------------------------------------------------------------

void SvxSearchDialog::Init_Impl( int bSearchPattern )
{
	DBG_ASSERT( pSearchItem, "SearchItem == 0" );
	bWriter = ( pSearchItem->GetAppFlag() == SVX_SEARCHAPP_WRITER );

	pImpl->bMultiLineEdit = FALSE;

	if ( !pImpl->bMultiLineEdit )
	{
		pImpl->aSearchFormats.Hide();
		aSearchAttrText.Show();
		pImpl->aReplaceFormats.Hide();
		aReplaceAttrText.Show();
	}
	else
	{
		String aText = aSearchAttrText.GetText();
		aSearchAttrText.Hide();

		if ( aText.Len() )
			pImpl->aSearchFormats.SetText( aText );
		pImpl->aSearchFormats.Show();
		aText = aReplaceAttrText.GetText();
		aReplaceAttrText.Hide();

		if ( aText.Len() )
			pImpl->aReplaceFormats.SetText( aText );
		pImpl->aReplaceFormats.Show();
	}

	if ( ( nModifyFlag & MODIFY_WORD ) == 0 )
 		aWordBtn.Check( pSearchItem->GetWordOnly() );
	if ( ( nModifyFlag & MODIFY_EXACT ) == 0 )
		aMatchCaseCB.Check( pSearchItem->GetExact() );
    if ( ( nModifyFlag & MODIFY_BACKWARDS ) == 0 )
        aBackwardsBtn.Check( pSearchItem->GetBackward() );
    if ( ( nModifyFlag & MODIFY_NOTES ) == 0 )
        aNotesBtn.Check( pSearchItem->GetNotes() );
	if ( ( nModifyFlag & MODIFY_SELECTION ) == 0 )
		aSelectionBtn.Check( pSearchItem->GetSelection() );
	if ( ( nModifyFlag & MODIFY_REGEXP ) == 0 )
		aRegExpBtn.Check( pSearchItem->GetRegExp() );
	if ( ( nModifyFlag & MODIFY_LAYOUT ) == 0 )
		aLayoutBtn.Check( pSearchItem->GetPattern() );
	if (aNotesBtn.IsChecked())
		aLayoutBtn.Disable();
	aSimilarityBox.Check( pSearchItem->IsLevenshtein() );
	if( aJapOptionsCB.IsVisible() )
		aJapOptionsCB.Check( pSearchItem->IsUseAsianOptions() );
	ApplyTransliterationFlags_Impl( pSearchItem->GetTransliterationFlags() );

    CalculateDelta_Impl();

	FASTBOOL bDraw = FALSE;
    if ( pSearchItem->GetAppFlag() == SVX_SEARCHAPP_CALC )
	{
        Link aLink = LINK( this, SvxSearchDialog, FlagHdl_Impl );
        aCalcSearchInLB.SetSelectHdl( aLink );
		aRowsBtn.SetClickHdl( aLink );
		aColumnsBtn.SetClickHdl( aLink );
        aAllSheetsCB.SetClickHdl( aLink );

		switch ( pSearchItem->GetCellType() )
		{
			case SVX_SEARCHIN_FORMULA:
				if ( ( nModifyFlag & MODIFY_FORMULAS ) == 0 )
                    aCalcSearchInLB.SelectEntryPos( SVX_SEARCHIN_FORMULA );
				break;

			case SVX_SEARCHIN_VALUE:
				if ( ( nModifyFlag & MODIFY_VALUES ) == 0 )
                    aCalcSearchInLB.SelectEntryPos( SVX_SEARCHIN_VALUE );
				break;

			case SVX_SEARCHIN_NOTE:
                if ( ( nModifyFlag & MODIFY_CALC_NOTES ) == 0 )
                    aCalcSearchInLB.SelectEntryPos( SVX_SEARCHIN_NOTE );
				break;
		}
		aWordBtn.SetText( aCalcStr.GetToken( 0, '#' ) );

		if ( pSearchItem->GetRowDirection() &&
			 ( nModifyFlag & MODIFY_ROWS ) == 0 )
			aRowsBtn.Check();
		else if ( !pSearchItem->GetRowDirection() &&
				  ( nModifyFlag & MODIFY_COLUMNS ) == 0 )
			aColumnsBtn.Check();

		if ( ( nModifyFlag & MODIFY_ALLTABLES ) == 0 )
            aAllSheetsCB.Check( pSearchItem->IsAllTables() );

		// nur im Writer Suche nach Formatierung
		aFormatBtn.Hide();
		aNoFormatBtn.Hide();
		aAttributeBtn.Hide();
	}
	else
	{
		aWordBtn.SetText( aCalcStr.GetToken( 1, '#' ) );

		if ( pSearchItem->GetAppFlag() == SVX_SEARCHAPP_DRAW )
		{
			aRegExpBtn.Hide();
			aLayoutBtn.Hide();

			// nur im Writer Suche nach Formatierung
			aFormatBtn.Hide();
			aNoFormatBtn.Hide();
			aAttributeBtn.Hide();
			bDraw = TRUE;
		}
		else
		{
			if ( !pSearchList )
			{
				// Attribut-Sets besorgen, wenn noch nicht geschehen
                const SfxPoolItem* ppArgs[] = { pSearchItem, 0 };
				const SvxSetItem* pSrchSetItem =
                (const SvxSetItem*)rBindings.GetDispatcher()->Execute( FID_SEARCH_SEARCHSET, SFX_CALLMODE_SLOT, ppArgs );

				if ( pSrchSetItem )
					InitAttrList_Impl( &pSrchSetItem->GetItemSet(), 0 );

				const SvxSetItem* pReplSetItem =
                (const SvxSetItem*)rBindings.GetDispatcher()->Execute( FID_SEARCH_REPLACESET, SFX_CALLMODE_SLOT, ppArgs );

				if ( pReplSetItem )
					InitAttrList_Impl( 0, &pReplSetItem->GetItemSet() );
			}
/*
			aFormatBtn.Show();
			aNoFormatBtn.Show();
			aAttributeBtn.Show();
*/
		}
//       pMoreBtn->SetState( FALSE );
//       pMoreBtn->Hide();
	}

    if ( 0 && !bDraw ) //!!!!!
	{
		aRegExpBtn.Show();
		aLayoutBtn.Show();
	}

	// "Ahnlichkeitssuche?
	if ( ( nModifyFlag & MODIFY_SIMILARITY ) == 0 )
		aSimilarityBox.Check( pSearchItem->IsLevenshtein() );
	bSet = TRUE;

	pImpl->bSaveToModule = FALSE;
	FlagHdl_Impl( &aSimilarityBox );
	FlagHdl_Impl( &aJapOptionsCB );
	pImpl->bSaveToModule = TRUE;

	FASTBOOL bDisableSearch = FALSE;
	SfxViewShell* pViewShell = SfxViewShell::Current();

	if ( pViewShell )
	{
		BOOL bText = !bSearchPattern;

		if ( pViewShell->HasSelection( bText ) )
			EnableControl_Impl( &aSelectionBtn );
		else
		{
			aSelectionBtn.Check( FALSE );
			aSelectionBtn.Disable();
		}
	}

	// Patternsuche und es wurden keine AttrSets "ubergeben
	if ( bSearchPattern )
	{
		SfxObjectShell* pShell = SfxObjectShell::Current();

		if ( pShell && pShell->GetStyleSheetPool() )
		{
			// Vorlagen beschaffen
			aSearchTmplLB .Clear();
			aReplaceTmplLB.Clear();
			SfxStyleSheetBasePool* pStylePool = pShell->GetStyleSheetPool();
			pStylePool->SetSearchMask( pSearchItem->GetFamily(),
									   SFXSTYLEBIT_ALL );
			SfxStyleSheetBase* pBase = pStylePool->First();

			while ( pBase )
			{
				if ( pBase->IsUsed() )
					aSearchTmplLB.InsertEntry( pBase->GetName() );
				aReplaceTmplLB.InsertEntry( pBase->GetName() );
				pBase = pStylePool->Next();
			}
			aSearchTmplLB.SelectEntry( pSearchItem->GetSearchString() );
			aReplaceTmplLB.SelectEntry( pSearchItem->GetReplaceString() );

		}
		aSearchTmplLB.Show();

		if ( bConstruct )
			// nur nach dem Erzeugen den Fokus grappen
			aSearchTmplLB.GrabFocus();
		aReplaceTmplLB.Show();
		aSearchLB.Hide();
		aReplaceLB.Hide();

		aWordBtn.Disable();
		aRegExpBtn.Disable();
		aMatchCaseCB.Disable();

		bDisableSearch = !aSearchTmplLB.GetEntryCount();
	}
	else
	{
		FASTBOOL bSetSearch = ( ( nModifyFlag & MODIFY_SEARCH ) == 0 );
		FASTBOOL bSetReplace = ( ( nModifyFlag & MODIFY_REPLACE ) == 0 );

		if ( pSearchItem->GetSearchString().Len() && bSetSearch )
			aSearchLB.SetText( pSearchItem->GetSearchString() );
		else if ( aSearchStrings.Count() )
		{
			FASTBOOL bAttributes =
				( ( pSearchList && pSearchList->Count() ) ||
				  ( pReplaceList && pReplaceList->Count() ) );

			if ( bSetSearch && !bAttributes )
				aSearchLB.SetText( *aSearchStrings[ 0 ] );

			String aReplaceTxt = pSearchItem->GetReplaceString();

			if ( aReplaceStrings.Count() )
				aReplaceTxt = *aReplaceStrings[ 0 ];

			if ( bSetReplace && !bAttributes )
				aReplaceLB.SetText( aReplaceTxt );
		}
		aSearchLB.Show();

		if ( bConstruct )
			// nur nach dem Erzeugen den Fokus grappen
			aSearchLB.GrabFocus();
		aReplaceLB.Show();
		aSearchTmplLB.Hide();
		aReplaceTmplLB.Hide();

		EnableControl_Impl( &aRegExpBtn );
		EnableControl_Impl( &aMatchCaseCB );

		if ( aRegExpBtn.IsChecked() )
			aWordBtn.Disable();
		else
			EnableControl_Impl( &aWordBtn );

		String aSrchAttrTxt;

		if ( pImpl->bMultiLineEdit )
			aSrchAttrTxt = pImpl->aSearchFormats.GetText();
		else
			aSrchAttrTxt = aSearchAttrText.GetText();

		bDisableSearch = !aSearchLB.GetText().Len() && !aSrchAttrTxt.Len();
	}
	FocusHdl_Impl( &aSearchLB );

	if ( bDisableSearch )
	{
		aSearchBtn.Disable();
		aSearchAllBtn.Disable();
		aReplaceBtn.Disable();
		aReplaceAllBtn.Disable();
        aSearchComponentFL.Enable(sal_False);
        aSearchComponent1PB.Enable(sal_False);
        aSearchComponent2PB.Enable(sal_False);
    }
	else
	{
		EnableControl_Impl( &aSearchBtn );
		EnableControl_Impl( &aReplaceBtn );
		if (!bWriter || (bWriter && !aNotesBtn.IsChecked()))
		{
			EnableControl_Impl( &aSearchAllBtn );
			EnableControl_Impl( &aReplaceAllBtn );
		}
		if (bWriter && pSearchItem->GetNotes())
		{
			aSearchAllBtn.Disable();
			aReplaceAllBtn.Disable();
		}
	}

	if ( ( !pImpl->bMultiLineEdit && aSearchAttrText.GetText().Len() ) ||
			( pImpl->bMultiLineEdit && pImpl->aSearchFormats.GetText().Len() ) )
		EnableControl_Impl( &aNoFormatBtn );
	else
		aNoFormatBtn.Disable();

	if ( !pSearchList )
	{
		aAttributeBtn.Disable();
		aFormatBtn.Disable();
	}

	if ( aLayoutBtn.IsChecked() )
	{
		pImpl->bSaveToModule = FALSE;
		TemplateHdl_Impl( &aLayoutBtn );
		pImpl->bSaveToModule = TRUE;
	}
}

// -----------------------------------------------------------------------

void SvxSearchDialog::InitAttrList_Impl( const SfxItemSet* pSSet,
										 const SfxItemSet* pRSet )
{
	if ( !pSSet && !pRSet )
		return;

	if ( !pImpl->pRanges && pSSet )
	{
		sal_sSize nCnt = 0;
		const USHORT* pPtr = pSSet->GetRanges();
		const USHORT* pTmp = pPtr;

		while( *pPtr )
		{
			nCnt += ( *(pPtr+1) - *pPtr ) + 1;
			pPtr += 2;
		}
		nCnt = pPtr - pTmp + 1;
		pImpl->pRanges = new USHORT[nCnt];
		memcpy( pImpl->pRanges, pTmp, sizeof(USHORT) * nCnt );
	}

	// sorge daf"ur, das die Texte der Attribute richtig stehen
	String aDesc;

	if ( pSSet )
	{
		delete pSearchList;
		pSearchList = new SearchAttrItemList;

		if ( pSSet->Count() )
		{
			pSearchList->Put( *pSSet );

			if ( !pImpl->bMultiLineEdit )
				aSearchAttrText.SetText( BuildAttrText_Impl( aDesc, TRUE ) );
			else
				pImpl->aSearchFormats.SetText( BuildAttrText_Impl( aDesc, TRUE ) );

			if ( aDesc.Len() )
				bFormat |= TRUE;
		}
	}

	if ( pRSet )
	{
		delete pReplaceList;
		pReplaceList = new SearchAttrItemList;

		if ( pRSet->Count() )
		{
			pReplaceList->Put( *pRSet );

			if ( !pImpl->bMultiLineEdit )
				aReplaceAttrText.SetText( BuildAttrText_Impl( aDesc, FALSE ) );
			else
				pImpl->aReplaceFormats.SetText( BuildAttrText_Impl( aDesc, FALSE ) );

			if ( aDesc.Len() )
				bFormat |= TRUE;
		}
	}
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxSearchDialog, FlagHdl_Impl, Control *, pCtrl )
{
    if ( pCtrl && !bSet )
        SetModifyFlag_Impl( pCtrl );
	else
		bSet = FALSE;

    if ( pCtrl == &aSimilarityBox )
	{
		BOOL bIsChecked = aSimilarityBox.IsChecked();

		if ( bIsChecked )
		{
			aSimilarityBtn.Enable();
			aRegExpBtn.Check( FALSE );
			aRegExpBtn.Disable();
			EnableControl_Impl( &aWordBtn );

			if ( aLayoutBtn.IsChecked() )
			{
				EnableControl_Impl( &aMatchCaseCB );
				aLayoutBtn.Check( FALSE );
			}
			aRegExpBtn.Disable();
			aLayoutBtn.Disable();
			aFormatBtn.Disable();
			aNoFormatBtn.Disable();
			aAttributeBtn.Disable();
		}
		else
		{
			EnableControl_Impl( &aRegExpBtn );
			if (!aNotesBtn.IsChecked())
				EnableControl_Impl( &aLayoutBtn );
			EnableControl_Impl( &aFormatBtn );
			EnableControl_Impl( &aAttributeBtn );
			aSimilarityBtn.Disable();
		}
		pSearchItem->SetLevenshtein( bIsChecked );
	}
	else
	if ( pCtrl == &aNotesBtn)
	{
		if (aNotesBtn.IsChecked())
		{
			aLayoutBtn.Disable();
			aSearchAllBtn.Disable();
			aReplaceAllBtn.Disable();
		}
		else
		{
			EnableControl_Impl( &aLayoutBtn );
			ModifyHdl_Impl( &aSearchLB );
		}
	}
	else
	{
		if ( aLayoutBtn.IsChecked() && !bFormat )
		{
			aWordBtn.Check( FALSE );
			aWordBtn.Disable();
			aRegExpBtn.Check( FALSE );
			aRegExpBtn.Disable();
			aMatchCaseCB.Check( FALSE );
			aMatchCaseCB.Disable();
			aNotesBtn.Disable();

			if ( aSearchTmplLB.GetEntryCount() )
			{
				EnableControl_Impl( &aSearchBtn );
				EnableControl_Impl( &aSearchAllBtn );
				EnableControl_Impl( &aReplaceBtn );
				EnableControl_Impl( &aReplaceAllBtn );
			}
		}
		else
		{
			EnableControl_Impl( &aRegExpBtn );
			EnableControl_Impl( &aMatchCaseCB );
			EnableControl_Impl( &aNotesBtn );

			if ( aRegExpBtn.IsChecked() )
			{
				aWordBtn.Check( FALSE );
				aWordBtn.Disable();
				aSimilarityBox.Disable();
				aSimilarityBtn.Disable();
			}
			else
			{
				EnableControl_Impl( &aWordBtn );
				EnableControl_Impl( &aSimilarityBox );
			}

			// Such-String vorhanden? dann Buttons enablen
			bSet = TRUE;
			ModifyHdl_Impl( &aSearchLB );
		}
	}

    if ( &aAllSheetsCB == pCtrl )
	{
        if ( aAllSheetsCB.IsChecked() )
			aSearchAllBtn.Disable();
		else
		{
			bSet = TRUE;
			ModifyHdl_Impl( &aSearchLB );
		}
	}

    if ( &aJapOptionsCB == pCtrl )
	{
		BOOL bEnableJapOpt = aJapOptionsCB.IsChecked();
		aMatchCaseCB			.Enable(!bEnableJapOpt );
		aJapMatchFullHalfWidthCB.Enable(!bEnableJapOpt );
		aJapOptionsBtn			.Enable( bEnableJapOpt );
	}

	if ( pImpl->bSaveToModule )
		SaveToModule_Impl();
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxSearchDialog, CommandHdl_Impl, Button *, pBtn )
{
	FASTBOOL bInclusive = ( aLayoutBtn.GetText() == aLayoutStr );

	if ( ( pBtn == &aSearchBtn )	||
		 ( pBtn == &aSearchAllBtn ) ||
		 ( pBtn == &aReplaceBtn )	||
		 ( pBtn == &aReplaceAllBtn ) )
	{
		if ( aLayoutBtn.IsChecked() && !bInclusive )
		{
			pSearchItem->SetSearchString ( aSearchTmplLB.GetSelectEntry() );
			pSearchItem->SetReplaceString( aReplaceTmplLB.GetSelectEntry() );
		}
		else
		{
			pSearchItem->SetSearchString ( aSearchLB.GetText() );
			pSearchItem->SetReplaceString( aReplaceLB.GetText() );

			if ( pBtn == &aReplaceBtn )
				Remember_Impl( aReplaceLB.GetText(), FALSE );
			else
			{
				Remember_Impl( aSearchLB.GetText(), TRUE );

				if ( pBtn == &aReplaceAllBtn )
					Remember_Impl( aReplaceLB.GetText(), FALSE );
			}
		}

		pSearchItem->SetRegExp( FALSE );
		pSearchItem->SetLevenshtein( FALSE );
		if (GetCheckBoxValue( aRegExpBtn ))
			pSearchItem->SetRegExp( TRUE );
		else if (GetCheckBoxValue( aSimilarityBox ))
			pSearchItem->SetLevenshtein( TRUE );

		pSearchItem->SetWordOnly( GetCheckBoxValue( aWordBtn ) );
        pSearchItem->SetBackward( GetCheckBoxValue( aBackwardsBtn ) );
        pSearchItem->SetNotes( GetCheckBoxValue( aNotesBtn ) );
		pSearchItem->SetPattern( GetCheckBoxValue( aLayoutBtn ) );
		pSearchItem->SetSelection( GetCheckBoxValue( aSelectionBtn ) );

		pSearchItem->SetUseAsianOptions( GetCheckBoxValue( aJapOptionsCB ) );
		INT32 nFlags = GetTransliterationFlags();
		if( !pSearchItem->IsUseAsianOptions())
            nFlags &= (TransliterationModules_IGNORE_CASE |
					   TransliterationModules_IGNORE_WIDTH );
		pSearchItem->SetTransliterationFlags( nFlags );

		if ( !bWriter )
		{
            if ( aCalcSearchInLB.GetSelectEntryPos() != LISTBOX_ENTRY_NOTFOUND )
                pSearchItem->SetCellType( aCalcSearchInLB.GetSelectEntryPos() );

			pSearchItem->SetRowDirection( aRowsBtn.IsChecked() );
            pSearchItem->SetAllTables( aAllSheetsCB.IsChecked() );
		}

		if ( pBtn == &aSearchBtn )
			pSearchItem->SetCommand( SVX_SEARCHCMD_FIND );
		else if ( pBtn == &aSearchAllBtn )
			pSearchItem->SetCommand( SVX_SEARCHCMD_FIND_ALL );
		else if ( pBtn == &aReplaceBtn )
			pSearchItem->SetCommand( SVX_SEARCHCMD_REPLACE );
		else if ( pBtn == &aReplaceAllBtn )
			pSearchItem->SetCommand( SVX_SEARCHCMD_REPLACE_ALL );

		// wenn nach Vorlagen gesucht wird, dann Format-Listen l"oschen
		if ( !bFormat && pSearchItem->GetPattern() )
		{
			if ( pSearchList )
				pSearchList->Clear();

			if ( pReplaceList )
				pReplaceList->Clear();
		}
		nModifyFlag = 0;
        const SfxPoolItem* ppArgs[] = { pSearchItem, 0 };
        rBindings.ExecuteSynchron( FID_SEARCH_NOW, ppArgs, 0L );
	}
	else if ( pBtn == &aCloseBtn )
	{
		if ( !aLayoutBtn.IsChecked() || bInclusive )
		{
			String aStr( aSearchLB.GetText() );

			if ( aStr.Len() )
				Remember_Impl( aStr, TRUE );
			aStr = aReplaceLB.GetText();

			if ( aStr.Len() )
				Remember_Impl( aStr, FALSE );
		}
		SaveToModule_Impl();
		Close();
	}
	else if ( pBtn == &aSimilarityBtn )
	{
//CHINA001 		SvxSearchSimilarityDialog* pDlg =
//CHINA001 			new SvxSearchSimilarityDialog( this,
//CHINA001 										   pSearchItem->IsLEVRelaxed(),
//CHINA001 										   pSearchItem->GetLEVOther(),
//CHINA001 										   pSearchItem->GetLEVShorter(),
//CHINA001 										   pSearchItem->GetLEVLonger() );
		SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
		if(pFact)
		{
			AbstractSvxSearchSimilarityDialog* pDlg = pFact->CreateSvxSearchSimilarityDialog( LAYOUT_THIS_WINDOW (this),
																		pSearchItem->IsLEVRelaxed(),
																		pSearchItem->GetLEVOther(),
																		pSearchItem->GetLEVShorter(),
																		pSearchItem->GetLEVLonger() );
			DBG_ASSERT(pDlg, "Dialogdiet fail!");//CHINA001
			if ( pDlg && pDlg->Execute() == RET_OK )
			{
				pSearchItem->SetLEVRelaxed( pDlg->IsRelaxed() );
				pSearchItem->SetLEVOther( pDlg->GetOther() );
				pSearchItem->SetLEVShorter( pDlg->GetShorter() );
				pSearchItem->SetLEVLonger( pDlg->GetLonger() );
				SaveToModule_Impl();
			}
			delete pDlg;
		}
	}
	else if ( pBtn == &aJapOptionsBtn )
	{
		SfxItemSet aSet( SFX_APP()->GetPool() );
		pSearchItem->SetTransliterationFlags( GetTransliterationFlags() );
		//CHINA001 SvxJSearchOptionsDialog aDlg( this, aSet, RID_SVXPAGE_JSEARCH_OPTIONS,
		//CHINA001 							pSearchItem->GetTransliterationFlags() );
        SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
		if(pFact)
		{
			AbstractSvxJSearchOptionsDialog* aDlg = pFact->CreateSvxJSearchOptionsDialog( LAYOUT_THIS_WINDOW (this), aSet, RID_SVXPAGE_JSEARCH_OPTIONS, pSearchItem->GetTransliterationFlags(), RID_SVXPAGE_JSEARCH_OPTIONS );
			DBG_ASSERT(aDlg, "Dialogdiet fail!");//CHINA001
			int nRet = aDlg->Execute(); //CHINA001 int nRet = aDlg.Execute();
			if (RET_OK == nRet) //! true only if FillItemSet of SvxJSearchOptionsPage returns true
			{
				INT32 nFlags = aDlg->GetTransliterationFlags(); //CHINA001 INT32 nFlags = aDlg.GetTransliterationFlags();
				pSearchItem->SetTransliterationFlags( nFlags );
				ApplyTransliterationFlags_Impl( nFlags );
			}
			delete aDlg; //add for CHINA001
		}
	}
    else if(pBtn == &aSearchComponent1PB || pBtn == &aSearchComponent2PB )
    {
        uno::Sequence < beans::PropertyValue > aArgs(2);
        beans::PropertyValue* pArgs = aArgs.getArray();
        pArgs[0].Name = ::rtl::OUString::createFromAscii("SearchString");
        pArgs[0].Value <<= ::rtl::OUString(aSearchLB.GetText());
        pArgs[1].Name = ::rtl::OUString::createFromAscii("ParentWindow");
        pArgs[1].Value <<= VCLUnoHelper::GetInterface( LAYOUT_THIS_WINDOW (this) );
        if(pBtn == &aSearchComponent1PB)
        {
            if ( pImpl->xCommand1Dispatch.is() )
                pImpl->xCommand1Dispatch->dispatch(pImpl->aCommand1URL, aArgs);
        }
        else
        {
            if ( pImpl->xCommand2Dispatch.is() )
                pImpl->xCommand2Dispatch->dispatch(pImpl->aCommand2URL, aArgs);
        }
    }

	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxSearchDialog, ModifyHdl_Impl, ComboBox *, pEd )
{
	if ( !bSet )
		SetModifyFlag_Impl( pEd );
	else
		bSet = FALSE;

	if ( pEd == &aSearchLB || pEd == &aReplaceLB )
	{
		xub_StrLen nLBTxtLen = aSearchLB.GetText().Len(), nTxtLen;

		if ( !pImpl->bMultiLineEdit )
		   nTxtLen = aSearchAttrText.GetText().Len();
		else
			nTxtLen = pImpl->aSearchFormats.GetText().Len();

		if ( nLBTxtLen || nTxtLen )
		{
			EnableControl_Impl( &aSearchBtn );
			EnableControl_Impl( &aReplaceBtn );
			if (!bWriter || (bWriter && !aNotesBtn.IsChecked()))
			{
				EnableControl_Impl( &aSearchAllBtn );
				EnableControl_Impl( &aReplaceAllBtn );
			}
		}
		else
		{
            aSearchComponentFL.Enable(sal_False);
            aSearchComponent1PB.Enable(sal_False);
            aSearchComponent2PB.Enable(sal_False);
            aSearchBtn.Disable();
			aSearchAllBtn.Disable();
			aReplaceBtn.Disable();
			aReplaceAllBtn.Disable();
		}
	}
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxSearchDialog, TemplateHdl_Impl, Button *, EMPTYARG )
{
	if ( pImpl->bSaveToModule )
		SaveToModule_Impl();

	if ( bFormat )
		return 0;
	String sDesc;

	if ( aLayoutBtn.IsChecked() )
	{
		if ( !pFamilyController )
		{
			USHORT nId = 0;

			// Vorlagen-Controller enablen
			switch ( pSearchItem->GetFamily() )
			{
				case SFX_STYLE_FAMILY_CHAR:
					nId = SID_STYLE_FAMILY1; break;

				case SFX_STYLE_FAMILY_PARA:
					nId = SID_STYLE_FAMILY2; break;

				case SFX_STYLE_FAMILY_FRAME:
					nId = SID_STYLE_FAMILY3; break;

				case SFX_STYLE_FAMILY_PAGE:
					nId = SID_STYLE_FAMILY4; break;

				case SFX_STYLE_FAMILY_ALL:
					break;

				default:
					DBG_ERROR( "StyleSheetFamily wurde geaendert?" );
			}

			rBindings.EnterRegistrations();
			pFamilyController =
				new SvxSearchController( nId, rBindings, *this );
			rBindings.LeaveRegistrations();
			aSearchTmplLB.Clear();
			aReplaceTmplLB.Clear();

			aSearchTmplLB.Show();
			aReplaceTmplLB.Show();
			aSearchLB.Hide();
			aReplaceLB.Hide();

			if ( !pImpl->bMultiLineEdit )
			{
				aSearchAttrText.SetText( sDesc );
				aReplaceAttrText.SetText( sDesc );
			}
			else
			{
				pImpl->aSearchFormats.SetText( sDesc );
				pImpl->aReplaceFormats.SetText( sDesc );
			}
		}
		aFormatBtn.Disable();
		aNoFormatBtn.Disable();
		aAttributeBtn.Disable();
		aSimilarityBox.Disable();
		aSimilarityBtn.Disable();
	}
	else
	{
		// Vorlagen-Controller disablen
		rBindings.EnterRegistrations();
		DELETEZ( pFamilyController );
		rBindings.LeaveRegistrations();

		aSearchLB.Show();
		aReplaceLB.Show();
		aSearchTmplLB.Hide();
		aReplaceTmplLB.Hide();

		if ( !pImpl->bMultiLineEdit )
		{
			aSearchAttrText.SetText( BuildAttrText_Impl( sDesc, TRUE ) );
			aReplaceAttrText.SetText( BuildAttrText_Impl( sDesc, FALSE ) );
		}
		else
		{
			pImpl->aSearchFormats.SetText( BuildAttrText_Impl( sDesc, TRUE ) );
			pImpl->aReplaceFormats.SetText( BuildAttrText_Impl( sDesc, FALSE ) );
		}

		EnableControl_Impl( &aFormatBtn );
		EnableControl_Impl( &aAttributeBtn );
		EnableControl_Impl( &aSimilarityBox );

		FocusHdl_Impl( bSearch ? &aSearchLB : &aReplaceLB );
	}
	bSet = TRUE;
	pImpl->bSaveToModule = FALSE;
	FlagHdl_Impl( &aLayoutBtn );
	pImpl->bSaveToModule = TRUE;
	return 0;
}

// -----------------------------------------------------------------------

void SvxSearchDialog::Remember_Impl( const String &rStr,BOOL _bSearch )
{
	if ( !rStr.Len() )
		return;

    SvStringsDtor* pArr = _bSearch ? &aSearchStrings : &aReplaceStrings;
    ComboBox* pListBox = _bSearch ? &aSearchLB : &aReplaceLB;

	// identische Strings ignorieren
	for ( USHORT i = 0; i < pArr->Count(); ++i )
	{
		if ( COMPARE_EQUAL == (*pArr)[i]->CompareTo( rStr ) )
			return;
	}

	// bei maximaler Belegung "altesten Eintrag l"oschen (ListBox und Array)
	String* pInsStr;

	if ( pArr->Count() >= REMEMBER_SIZE )
	{
		pInsStr = (*pArr)[REMEMBER_SIZE - 1];
		pListBox->RemoveEntry( USHORT(REMEMBER_SIZE - 1) );
		pArr->Remove( REMEMBER_SIZE - 1 );
		*pInsStr = rStr;
	}
	else
		pInsStr = new String( rStr );

	pArr->Insert( pInsStr, 0 );
	pListBox->InsertEntry( *pInsStr, 0 );
}

// -----------------------------------------------------------------------

void SvxSearchDialog::TemplatesChanged_Impl( SfxStyleSheetBasePool& rPool )
{
//	SetUpdateMode( FALSE );
	String aOldSrch( aSearchTmplLB .GetSelectEntry() );
	String aOldRepl( aReplaceTmplLB.GetSelectEntry() );
	aSearchTmplLB .Clear();
	aReplaceTmplLB.Clear();
	rPool.SetSearchMask( pSearchItem->GetFamily(), SFXSTYLEBIT_ALL );
	aSearchTmplLB.SetUpdateMode( FALSE );
	aReplaceTmplLB.SetUpdateMode( FALSE );
	SfxStyleSheetBase* pBase = rPool.First();

	while ( pBase )
	{
		if ( pBase->IsUsed() )
			aSearchTmplLB.InsertEntry( pBase->GetName() );
		aReplaceTmplLB.InsertEntry( pBase->GetName() );
		pBase = rPool.Next();
	}
	aSearchTmplLB.SetUpdateMode( TRUE );
	aReplaceTmplLB.SetUpdateMode( TRUE );
	aSearchTmplLB.SelectEntryPos(0);

	if ( aOldSrch.Len() )
		aSearchTmplLB .SelectEntry( aOldSrch );
	aReplaceTmplLB.SelectEntryPos(0);

	if ( aOldRepl.Len() )
		aReplaceTmplLB.SelectEntry( aOldRepl );

	if ( aSearchTmplLB.GetEntryCount() )
	{
		EnableControl_Impl( &aSearchBtn );
		EnableControl_Impl( &aSearchAllBtn );
		EnableControl_Impl( &aReplaceBtn );
		EnableControl_Impl( &aReplaceAllBtn );
	}
//	FlagHdl_Impl(0);
//	SetUpdateMode( TRUE );
}

// -----------------------------------------------------------------------

void SvxSearchDialog::EnableControls_Impl( const USHORT nFlags )
{
	if ( nFlags == nOptions )
		return;
	else
		nOptions = nFlags;

	if ( !nOptions )
	{
		if ( IsVisible() )
		{
			Hide();
			return;
		}
	}
	else if ( !IsVisible() )
		Show();
	FASTBOOL bNoSearch = TRUE;

    sal_Bool bEnableSearch = ( SEARCH_OPTIONS_SEARCH & nOptions ) != 0;
    aSearchBtn.Enable(bEnableSearch);

    if( bEnableSearch )
        bNoSearch = FALSE;


    if ( ( SEARCH_OPTIONS_SEARCH_ALL & nOptions ) != 0 )
	{
		aSearchAllBtn.Enable();
		bNoSearch = FALSE;
	}
	else
		aSearchAllBtn.Disable();
	if ( ( SEARCH_OPTIONS_REPLACE & nOptions ) != 0 )
	{
		aReplaceBtn.Enable();
		aReplaceText.Enable();
		aReplaceLB.Enable();
		aReplaceTmplLB.Enable();
		bNoSearch = FALSE;
	}
	else
	{
		aReplaceBtn.Disable();
		aReplaceText.Disable();
		aReplaceLB.Disable();
		aReplaceTmplLB.Disable();
	}
	if ( ( SEARCH_OPTIONS_REPLACE_ALL & nOptions ) != 0 )
	{
		aReplaceAllBtn.Enable();
		bNoSearch = FALSE;
	}
	else
		aReplaceAllBtn.Disable();
    aSearchComponentFL.Enable(!bNoSearch);
    aSearchComponent1PB.Enable(!bNoSearch);
    aSearchComponent2PB.Enable(!bNoSearch);
    aSearchBtn.Enable( !bNoSearch );
	aSearchText.Enable( !bNoSearch );
	aSearchLB.Enable( !bNoSearch );

	if ( ( SEARCH_OPTIONS_WHOLE_WORDS & nOptions ) != 0 )
		aWordBtn.Enable();
	else
		aWordBtn.Disable();
	if ( ( SEARCH_OPTIONS_BACKWARDS & nOptions ) != 0 )
		aBackwardsBtn.Enable();
	else
		aBackwardsBtn.Disable();
    //!if ( ( SEARCH_OPTIONS_NOTES & nOptions ) != 0 )
        aNotesBtn.Enable();
    //!else
    //!    aNotesBtn.Disable();
	if ( ( SEARCH_OPTIONS_REG_EXP & nOptions ) != 0 )
		aRegExpBtn.Enable();
	else
		aRegExpBtn.Disable();
	if ( ( SEARCH_OPTIONS_EXACT & nOptions ) != 0 )
		aMatchCaseCB.Enable();
	else
		aMatchCaseCB.Disable();
	if ( ( SEARCH_OPTIONS_SELECTION & nOptions ) != 0 )
		aSelectionBtn.Enable();
	else
		aSelectionBtn.Disable();
	if ( ( SEARCH_OPTIONS_FAMILIES & nOptions ) != 0 )
		aLayoutBtn.Enable();
	else
		aLayoutBtn.Disable();
	if ( ( SEARCH_OPTIONS_FORMAT & nOptions ) != 0 )
	{
		aAttributeBtn.Enable();
		aFormatBtn.Enable();
		aNoFormatBtn.Enable();
	}
	else
	{
		aAttributeBtn.Disable();
		aFormatBtn.Disable();
		aNoFormatBtn.Disable();
	}
/*
	if ( ( SEARCH_OPTIONS_MORE & nOptions ) != 0 &&
		 pSearchItem && pSearchItem->GetAppFlag() == SVX_SEARCHAPP_CALC )
		pMoreBtn->Enable();
	else
	{
		pMoreBtn->SetState( FALSE );
		pMoreBtn->Disable();
	}
*/
	if ( ( SEARCH_OPTIONS_SIMILARITY & nOptions ) != 0 )
	{
		aSimilarityBox.Enable();
		aSimilarityBtn.Enable();
	}
	else
	{
		aSimilarityBox.Disable();
		aSimilarityBtn.Disable();
	}

	if ( pSearchItem )
		Init_Impl( pSearchItem->GetPattern() &&
				   ( !pSearchList || !pSearchList->Count() ) );
}

// -----------------------------------------------------------------------

void SvxSearchDialog::EnableControl_Impl( Control* pCtrl )
{
	if ( &aSearchBtn == pCtrl && ( SEARCH_OPTIONS_SEARCH & nOptions ) != 0 )
	{
        aSearchComponentFL.Enable();
        aSearchComponent1PB.Enable();
        aSearchComponent2PB.Enable();
        aSearchBtn.Enable();
		return;
	}
	if ( &aSearchAllBtn == pCtrl &&
		 ( SEARCH_OPTIONS_SEARCH_ALL & nOptions ) != 0 )
	{
        aSearchAllBtn.Enable( ( bWriter || !aAllSheetsCB.IsChecked() ) );
		return;
	}
	if ( &aReplaceBtn == pCtrl && ( SEARCH_OPTIONS_REPLACE & nOptions ) != 0 )
	{
		aReplaceBtn.Enable();
		return;
	}
	if ( &aReplaceAllBtn == pCtrl &&
		 ( SEARCH_OPTIONS_REPLACE_ALL & nOptions ) != 0 )
	{
		aReplaceAllBtn.Enable();
		return;
	}
	if ( &aWordBtn == pCtrl && ( SEARCH_OPTIONS_WHOLE_WORDS & nOptions ) != 0 )
	{
		aWordBtn.Enable();
		return;
	}
    if ( &aBackwardsBtn == pCtrl && ( SEARCH_OPTIONS_BACKWARDS & nOptions ) != 0 )
    {
        aBackwardsBtn.Enable();
        return;
    }
    if ( &aNotesBtn == pCtrl /*! && ( SEARCH_OPTIONS_NOTES & nOptions ) != 0 */ )
    {
        aNotesBtn.Enable();
        return;
    }
	if ( &aRegExpBtn == pCtrl && ( SEARCH_OPTIONS_REG_EXP & nOptions ) != 0
		&& !aSimilarityBox.IsChecked())
	{
		aRegExpBtn.Enable();
		return;
	}
	if ( &aMatchCaseCB == pCtrl && ( SEARCH_OPTIONS_EXACT & nOptions ) != 0 )
	{
		if (!aJapOptionsCB.IsChecked())
			aMatchCaseCB.Enable();
		return;
	}
	if ( &aSelectionBtn == pCtrl && ( SEARCH_OPTIONS_SELECTION & nOptions ) != 0 )
	{
		aSelectionBtn.Enable();
		return;
	}
	if ( &aLayoutBtn == pCtrl && ( SEARCH_OPTIONS_FAMILIES & nOptions ) != 0 )
	{
		aLayoutBtn.Enable();
		return;
	}
	if ( 	&aAttributeBtn == pCtrl
		 && ( SEARCH_OPTIONS_FORMAT & nOptions ) != 0
		 && pSearchList )
	{
		aAttributeBtn.Enable( pImpl->bFocusOnSearch );
	}
	if ( &aFormatBtn == pCtrl && ( SEARCH_OPTIONS_FORMAT & nOptions ) != 0 )
	{
		aFormatBtn.Enable();
		return;
	}
	if ( &aNoFormatBtn == pCtrl && ( SEARCH_OPTIONS_FORMAT & nOptions ) != 0 )
	{
		aNoFormatBtn.Enable();
		return;
	}
	if ( &aSimilarityBox == pCtrl &&
		 ( SEARCH_OPTIONS_SIMILARITY & nOptions ) != 0 )
	{
		aSimilarityBox.Enable();

		if ( aSimilarityBox.IsChecked() )
			aSimilarityBtn.Enable();
	}
}

// -----------------------------------------------------------------------

void SvxSearchDialog::SetItem_Impl( const SvxSearchItem* pItem )
{
	if ( pItem )
	{
		delete pSearchItem;
		pSearchItem = (SvxSearchItem*)pItem->Clone();
		Init_Impl( pSearchItem->GetPattern() &&
				   ( !pSearchList || !pSearchList->Count() ) );
	}
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxSearchDialog, FocusHdl_Impl, Control *, pCtrl )
{
	xub_StrLen nTxtLen;

	if ( !pImpl->bMultiLineEdit )
		nTxtLen = aSearchAttrText.GetText().Len();
	else
		nTxtLen = pImpl->aSearchFormats.GetText().Len();

	if ( pCtrl == &aSearchLB || pCtrl == &pImpl->aSearchFormats )
	{
		if ( pCtrl->HasChildPathFocus() )
			pImpl->bFocusOnSearch = TRUE;
		pCtrl = &aSearchLB;
		bSearch = TRUE;

		if( nTxtLen )
			EnableControl_Impl( &aNoFormatBtn );
		else
			aNoFormatBtn.Disable();
		EnableControl_Impl( &aAttributeBtn );
	}
	else
	{
		pImpl->bFocusOnSearch = FALSE;
		pCtrl = &aReplaceLB;
		bSearch = FALSE;

		if ( ( !pImpl->bMultiLineEdit && aReplaceAttrText.GetText().Len() ) ||
				( pImpl->bMultiLineEdit && pImpl->aReplaceFormats.GetText().Len() ) )
			EnableControl_Impl( &aNoFormatBtn );
		else
			aNoFormatBtn.Disable();
		aAttributeBtn.Disable();
	}
	bSet = TRUE;

	aSearchLB.SetSelection( Selection( SELECTION_MIN, SELECTION_MAX ) );

	ModifyHdl_Impl( (ComboBox*)pCtrl );

	aLayoutBtn.SetText( bFormat && nTxtLen ? aLayoutStr : aStylesStr );
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxSearchDialog, LoseFocusHdl_Impl, Control *, EMPTYARG )
{
	SaveToModule_Impl();
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxSearchDialog, FormatHdl_Impl, Button *, EMPTYARG )
{
	SfxObjectShell* pSh = SfxObjectShell::Current();

	DBG_ASSERT( pSh, "no DocShell" );

	if ( !pSh || !pImpl->pRanges )
		return 0;

	sal_sSize nCnt = 0;
	const USHORT* pPtr = pImpl->pRanges;
	const USHORT* pTmp = pPtr;

	while( *pTmp )
		pTmp++;
	nCnt = pTmp - pPtr + 7;
	USHORT* pWhRanges = new USHORT[nCnt];
	USHORT nPos = 0;

	while( *pPtr )
	{
		pWhRanges[nPos++] = *pPtr++;
	}

	pWhRanges[nPos++] = SID_ATTR_PARA_MODEL;
	pWhRanges[nPos++] = SID_ATTR_PARA_MODEL;

	USHORT nBrushWhich = pSh->GetPool().GetWhich(SID_ATTR_BRUSH);
	pWhRanges[nPos++] = nBrushWhich;
	pWhRanges[nPos++] = nBrushWhich;
	pWhRanges[nPos++] = SID_PARA_BACKGRND_DESTINATION;
	pWhRanges[nPos++] = SID_PARA_BACKGRND_DESTINATION;
	pWhRanges[nPos] = 0;
	SfxItemPool& rPool = pSh->GetPool();
	SfxItemSet aSet( rPool, pWhRanges );
	String aTxt;

	aSet.InvalidateAllItems();
	aSet.Put(SvxBrushItem(nBrushWhich));
	aSet.Put(SfxUInt16Item(SID_PARA_BACKGRND_DESTINATION, PARA_DEST_CHAR));

	if ( bSearch )
	{
		aTxt = SVX_RESSTR( RID_SVXSTR_SEARCH );
		pSearchList->Get( aSet );
	}
	else
	{
		aTxt = SVX_RESSTR( RID_SVXSTR_REPLACE );
		pReplaceList->Get( aSet );
	}
	aSet.DisableItem(SID_ATTR_PARA_MODEL);
	aSet.DisableItem(rPool.GetWhich(SID_ATTR_PARA_PAGEBREAK));
	aSet.DisableItem(rPool.GetWhich(SID_ATTR_PARA_KEEP));

	//CHINA001 SvxSearchFormatDialog* pDlg = new SvxSearchFormatDialog( this, aSet );
	SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
	if(pFact)
	{
		SfxAbstractTabDialog* pDlg = pFact->CreateTabItemDialog( LAYOUT_THIS_WINDOW (this), aSet, RID_SVXDLG_SEARCHFORMAT );
		DBG_ASSERT(pDlg, "Dialogdiet fail!");//CHINA001
		aTxt.Insert( pDlg->GetText(), 0 );
		pDlg->SetText( aTxt );

		if ( pDlg->Execute() == RET_OK )
		{
			DBG_ASSERT( pDlg->GetOutputItemSet(), "invalid Output-Set" );
			SfxItemSet aOutSet( *pDlg->GetOutputItemSet() );

			SearchAttrItemList* pList = bSearch ? pSearchList : pReplaceList;

			SearchAttrItem* pAItem;
			const SfxPoolItem* pItem;
			for( USHORT n = 0; n < pList->Count(); ++n )
				if( !IsInvalidItem( (pAItem = &pList->GetObject(n))->pItem ) &&
					SFX_ITEM_SET == aOutSet.GetItemState(
						pAItem->pItem->Which(), FALSE, &pItem ) )
				{
					delete pAItem->pItem;
					pAItem->pItem = pItem->Clone();
					aOutSet.ClearItem( pAItem->pItem->Which() );
				}

			if( aOutSet.Count() )
				pList->Put( aOutSet );

			PaintAttrText_Impl(); // AttributText in GroupBox setzen
		}
		delete pDlg;
	}
	delete[] pWhRanges;
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxSearchDialog, NoFormatHdl_Impl, Button *, EMPTYARG )
{
	aLayoutBtn.SetText( aStylesStr );
	bFormat = FALSE;
	aLayoutBtn.Check( FALSE );

	if ( bSearch )
	{
		if ( !pImpl->bMultiLineEdit )
			aSearchAttrText.SetText( String() );
		else
			pImpl->aSearchFormats.SetText( String() );
		pSearchList->Clear();
	}
	else
	{
		if ( !pImpl->bMultiLineEdit )
			aReplaceAttrText.SetText( String() );
		else
			pImpl->aReplaceFormats.SetText( String() );
		pReplaceList->Clear();
	}
	pImpl->bSaveToModule = FALSE;
	TemplateHdl_Impl( &aLayoutBtn );
	pImpl->bSaveToModule = TRUE;
	aNoFormatBtn.Disable();
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxSearchDialog, AttributeHdl_Impl, Button *, EMPTYARG )
{
	if ( !pSearchList || !pImpl->pRanges )
		return 0;

	//CHINA001 SvxSearchAttributeDialog* pDlg = new SvxSearchAttributeDialog( this, *pSearchList, pImpl->pRanges );
	SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
	if(pFact)
	{
		VclAbstractDialog* pDlg = pFact->CreateSvxSearchAttributeDialog( LAYOUT_THIS_WINDOW (this), *pSearchList, pImpl->pRanges, RID_SVXDLG_SEARCHATTR );
		DBG_ASSERT(pDlg, "Dialogdiet fail!");//CHINA001
		pDlg->Execute();
		delete pDlg;
	}
	PaintAttrText_Impl();
	return 0;
}

// -----------------------------------------------------------------------

IMPL_LINK( SvxSearchDialog, TimeoutHdl_Impl, Timer *, pTimer )
{
    SfxViewShell* pViewShell = SfxViewShell::Current();

    if ( pViewShell )
    {
        if ( pViewShell->HasSelection( aSearchLB.IsVisible() ) )
            EnableControl_Impl( &aSelectionBtn );
        else
        {
            aSelectionBtn.Check( FALSE );
            aSelectionBtn.Disable();
        }
    }

    pTimer->Start();
    return 0;
}

// -----------------------------------------------------------------------

void SvxSearchDialog::GetSearchItems( SfxItemSet& rSet )
{
	xub_StrLen nLen;

	if ( !pImpl->bMultiLineEdit )
		nLen = aSearchAttrText.GetText().Len();
	else
		nLen = pImpl->aSearchFormats.GetText().Len();

	if ( nLen && pSearchList )
		pSearchList->Get( rSet );
}

// -----------------------------------------------------------------------

void SvxSearchDialog::GetReplaceItems( SfxItemSet& rSet )
{
	xub_StrLen nLen;

	if ( !pImpl->bMultiLineEdit )
		nLen = aReplaceAttrText.GetText().Len();
	else
		nLen = pImpl->aReplaceFormats.GetText().Len();

	if ( nLen && pReplaceList )
		pReplaceList->Get( rSet );
}

// -----------------------------------------------------------------------

String& SvxSearchDialog::BuildAttrText_Impl( String& rStr,
											 BOOL bSrchFlag ) const
{
	if ( rStr.Len() )
		rStr.Erase();

	SfxObjectShell* pSh = SfxObjectShell::Current();
	DBG_ASSERT( pSh, "no DocShell" );

	if ( !pSh )
		return rStr;

	SfxItemPool& rPool = pSh->GetPool();
	SearchAttrItemList* pList = bSrchFlag ? pSearchList : pReplaceList;

	if ( !pList )
		return rStr;

	// Metrik abfragen
	SfxMapUnit eMapUnit = SFX_MAPUNIT_CM;
	FieldUnit eFieldUnit = GetModuleFieldUnit();

	switch ( eFieldUnit )
	{
		case FUNIT_MM:			eMapUnit = SFX_MAPUNIT_MM; break;
		case FUNIT_CM:
		case FUNIT_M:
		case FUNIT_KM:			eMapUnit = SFX_MAPUNIT_CM; break;
		case FUNIT_TWIP:		eMapUnit = SFX_MAPUNIT_TWIP; break;
		case FUNIT_POINT:
		case FUNIT_PICA:		eMapUnit = SFX_MAPUNIT_POINT; break;
		case FUNIT_INCH:
		case FUNIT_FOOT:
		case FUNIT_MILE:		eMapUnit = SFX_MAPUNIT_INCH; break;
		case FUNIT_100TH_MM:	eMapUnit = SFX_MAPUNIT_100TH_MM; break;
        default: ;//prevent warning
	}

	for ( USHORT i = 0; i < pList->Count(); ++i )
	{
		const SearchAttrItem& rItem = pList->GetObject(i);

		if ( rStr.Len() )
			rStr.AppendAscii( RTL_CONSTASCII_STRINGPARAM( ", " ) );

		if ( !IsInvalidItem( rItem.pItem ) )
		{
			String aStr;
			rPool.GetPresentation( *rItem.pItem,
									SFX_ITEM_PRESENTATION_COMPLETE,
									eMapUnit, aStr );
			rStr += aStr;
		}
		else
		{
			//Sonderbehandlung fuer Zeichenhintergrund
			USHORT nId = rItem.nSlot == SID_ATTR_BRUSH_CHAR ?
								RID_SVXITEMS_BRUSH_CHAR :
									rItem.nSlot - SID_SVX_START + RID_ATTR_BEGIN;
			rStr += SVX_RESSTR( nId );
		}
	}
	return rStr;
}

// -----------------------------------------------------------------------

void SvxSearchDialog::PaintAttrText_Impl()
{
	String aDesc;
	BuildAttrText_Impl( aDesc, bSearch );

	if ( !bFormat && aDesc.Len() )
		bFormat = TRUE;

	if ( bSearch )
	{
		if ( !pImpl->bMultiLineEdit )
			aSearchAttrText.SetText( aDesc );
		else
			pImpl->aSearchFormats.SetText( aDesc );
		FocusHdl_Impl( &aSearchLB );
	}
	else
	{
		if ( !pImpl->bMultiLineEdit )
			aReplaceAttrText.SetText( aDesc );
		else
			pImpl->aReplaceFormats.SetText( aDesc );
		FocusHdl_Impl( &aReplaceLB );
	}
}

// -----------------------------------------------------------------------

void SvxSearchDialog::SetModifyFlag_Impl( const Control* pCtrl )
{
	if ( &aSearchLB == (ComboBox*)pCtrl )
		nModifyFlag |= MODIFY_SEARCH;
	else if ( &aReplaceLB == (ComboBox*)pCtrl )
		nModifyFlag |= MODIFY_REPLACE;
	else if ( &aWordBtn == (CheckBox*)pCtrl )
		nModifyFlag |= MODIFY_WORD;
	else if ( &aMatchCaseCB == (CheckBox*)pCtrl )
		nModifyFlag |= MODIFY_EXACT;
	else if ( &aBackwardsBtn == (CheckBox*)pCtrl )
		nModifyFlag |= MODIFY_BACKWARDS;
    else if ( &aNotesBtn == (CheckBox*)pCtrl )
        nModifyFlag |= MODIFY_NOTES;
	else if ( &aSelectionBtn == (CheckBox*)pCtrl )
		nModifyFlag |= MODIFY_SELECTION;
	else if ( &aRegExpBtn == (CheckBox*)pCtrl )
		nModifyFlag |= MODIFY_REGEXP;
	else if ( &aLayoutBtn == (CheckBox*)pCtrl )
		nModifyFlag |= MODIFY_LAYOUT;
	else if ( &aSimilarityBox == (CheckBox*)pCtrl )
		nModifyFlag |= MODIFY_SIMILARITY;
    else if ( &aCalcSearchInLB == (ListBox*)pCtrl )
    {
		nModifyFlag |= MODIFY_FORMULAS;
		nModifyFlag |= MODIFY_VALUES;
        nModifyFlag |= MODIFY_CALC_NOTES;
    }
	else if ( &aRowsBtn == (RadioButton*)pCtrl )
		nModifyFlag |= MODIFY_ROWS;
	else if ( &aColumnsBtn == (RadioButton*)pCtrl )
		nModifyFlag |= MODIFY_COLUMNS;
    else if ( &aAllSheetsCB == (CheckBox*)pCtrl )
		nModifyFlag |= MODIFY_ALLTABLES;
}

// -----------------------------------------------------------------------

void SvxSearchDialog::SaveToModule_Impl()
{
	if ( !pSearchItem )
		return;

	if ( aLayoutBtn.IsChecked() )
	{
		pSearchItem->SetSearchString ( aSearchTmplLB.GetSelectEntry() );
		pSearchItem->SetReplaceString( aReplaceTmplLB.GetSelectEntry() );
	}
	else
	{
		pSearchItem->SetSearchString ( aSearchLB.GetText() );
		pSearchItem->SetReplaceString( aReplaceLB.GetText() );
		Remember_Impl( aSearchLB.GetText(), TRUE );
	}

	pSearchItem->SetRegExp( FALSE );
	pSearchItem->SetLevenshtein( FALSE );
	if (GetCheckBoxValue( aRegExpBtn ))
		pSearchItem->SetRegExp( TRUE );
	else if (GetCheckBoxValue( aSimilarityBox ))
		pSearchItem->SetLevenshtein( TRUE );

	pSearchItem->SetWordOnly( GetCheckBoxValue( aWordBtn ) );
    pSearchItem->SetBackward( GetCheckBoxValue( aBackwardsBtn ) );
    pSearchItem->SetNotes( GetCheckBoxValue( aNotesBtn ) );
	pSearchItem->SetPattern( GetCheckBoxValue( aLayoutBtn ) );
	pSearchItem->SetSelection( GetCheckBoxValue( aSelectionBtn ) );

	pSearchItem->SetUseAsianOptions( GetCheckBoxValue( aJapOptionsCB ) );
	INT32 nFlags = GetTransliterationFlags();
	if( !pSearchItem->IsUseAsianOptions())
		nFlags &= (TransliterationModules_IGNORE_CASE |
				   TransliterationModules_IGNORE_WIDTH );
	pSearchItem->SetTransliterationFlags( nFlags );

	if ( !bWriter )
	{
        if ( aCalcSearchInLB.GetSelectEntryPos() != LISTBOX_ENTRY_NOTFOUND )
            pSearchItem->SetCellType( aCalcSearchInLB.GetSelectEntryPos() );

		pSearchItem->SetRowDirection( aRowsBtn.IsChecked() );
        pSearchItem->SetAllTables( aAllSheetsCB.IsChecked() );
	}

	pSearchItem->SetCommand( SVX_SEARCHCMD_FIND );
	nModifyFlag = 0;
    const SfxPoolItem* ppArgs[] = { pSearchItem, 0 };
    rBindings.GetDispatcher()->Execute( SID_SEARCH_ITEM, SFX_CALLMODE_SLOT, ppArgs );
}

// class SvxSearchDialogWrapper ------------------------------------------

SFX_IMPL_CHILDWINDOW(SvxSearchDialogWrapper, SID_SEARCH_DLG);

// -----------------------------------------------------------------------

SvxSearchDialogWrapper::SvxSearchDialogWrapper( Window* _pParent, USHORT nId,
												SfxBindings* pBindings,
												SfxChildWinInfo* pInfo )
    : SfxChildWindow( _pParent, nId )
    , dialog (new SvxSearchDialog (_pParent, this, *pBindings))
{
    pWindow = LAYOUT_THIS_WINDOW (dialog);
	dialog->Initialize( pInfo );

	pBindings->Update( SID_SEARCH_ITEM );
	pBindings->Update( SID_SEARCH_OPTIONS );
	pBindings->Update( SID_SEARCH_SEARCHSET );
	pBindings->Update( SID_SEARCH_REPLACESET );
	eChildAlignment = SFX_ALIGN_NOALIGNMENT;
	dialog->bConstruct = FALSE;
}

SvxSearchDialogWrapper::~SvxSearchDialogWrapper ()
{
#if ENABLE_LAYOUT
    delete dialog;
    pWindow = 0;
#endif /* ENABLE_LAYOUT */
}

SvxSearchDialog *SvxSearchDialogWrapper::getDialog ()
{
    return dialog;
}

// -----------------------------------------------------------------------

SfxChildWinInfo SvxSearchDialogWrapper::GetInfo() const
{
	SfxChildWinInfo aInfo = SfxChildWindow::GetInfo();
	aInfo.bVisible = sal_False;
	return aInfo;
}

