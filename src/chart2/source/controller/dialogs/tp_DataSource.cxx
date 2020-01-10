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
#include "precompiled_chart2.hxx"

#include "tp_DataSource.hxx"
#include "tp_DataSource.hrc"
#include "Strings.hrc"
#include "ResId.hxx"
#include "chartview/ChartSfxItemIds.hxx"
#include "macros.hxx"
#include "ChartTypeTemplateProvider.hxx"
#include "RangeSelectionHelper.hxx"
#include "DataSeriesHelper.hxx"
#include "tp_DataSourceControls.hxx"
#include "ControllerLockGuard.hxx"
#include "DataSourceHelper.hxx"
#include <com/sun/star/sheet/XRangeSelection.hpp>
#include <com/sun/star/table/XCellRange.hpp>
#include <com/sun/star/chart2/XChartType.hpp>
#include <com/sun/star/chart2/XChartTypeTemplate.hpp>
#include <com/sun/star/util/XModifiable.hpp>
#include <com/sun/star/chart2/data/XDataSink.hpp>

// for RET_OK
#include <vcl/msgbox.hxx>
#include <rtl/ustrbuf.hxx>

#include <functional>
#include <algorithm>
#include <map>

using namespace ::com::sun::star;
using namespace ::com::sun::star::chart2;

using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::rtl::OUString;
using ::rtl::OUStringBuffer;

// --------------------------------------------------------------------------------

namespace
{

const OUString lcl_aLabelRole( RTL_CONSTASCII_USTRINGPARAM( "label" ));

String lcl_GetRoleLBEntry(
    const OUString & rRole, const OUString & rRange )
{
    String aEntry( rRole );
    aEntry += '\t';
    aEntry += String(
        ::chart::DialogModel::ConvertRoleFromInternalToUI( rRole ));
    aEntry += '\t';
    aEntry += String( rRange );

    return aEntry;
}

void lcl_UpdateCurrentRange(
    SvTabListBox & rOutListBox,
    const OUString & rRole, const OUString & rRange )
{
    SvLBoxEntry * pEntry = rOutListBox.FirstSelected();
    if( pEntry )
        rOutListBox.SetEntryText( lcl_GetRoleLBEntry( rRole, rRange ), pEntry );
}

bool lcl_UpdateCurrentSeriesName(
    SvTreeListBox & rOutListBox )
{
    bool bResult = false;
    ::chart::SeriesEntry * pEntry = dynamic_cast< ::chart::SeriesEntry * >( rOutListBox.FirstSelected());
    if( pEntry &&
        pEntry->m_xDataSeries.is() &&
        pEntry->m_xChartType.is())
    {
        String aLabel( ::chart::DataSeriesHelper::getDataSeriesLabel(
                           pEntry->m_xDataSeries,
                           pEntry->m_xChartType->getRoleOfSequenceForSeriesLabel()));
        if( aLabel.Len())
        {
            rOutListBox.SetEntryText( pEntry, aLabel );
            bResult = true;
        }
    }
    return bResult;
}

OUString lcl_GetSelectedRole( const SvTabListBox & rRoleListBox, bool bUITranslated = false )
{
    OUString aResult;
    SvLBoxEntry * pEntry = rRoleListBox.FirstSelected();
    if( pEntry )
        aResult = OUString( rRoleListBox.GetEntryText( pEntry,
                                                       bUITranslated ? 1 : 0 ));
    return aResult;
}

OUString lcl_GetSelectedRolesRange( const SvTabListBox & rRoleListBox )
{
    OUString aResult;
    SvLBoxEntry * pEntry = rRoleListBox.FirstSelected();
    if( pEntry )
        aResult = OUString( rRoleListBox.GetEntryText( pEntry, 2 ));
    return aResult;
}

OUString lcl_GetSequenceNameForLabel( ::chart::SeriesEntry * pEntry )
{
    OUString aResult( RTL_CONSTASCII_USTRINGPARAM("values-y"));
    if( pEntry &&
        pEntry->m_xChartType.is())
    {
        aResult = pEntry->m_xChartType->getRoleOfSequenceForSeriesLabel();
    }
    return aResult;
}

static long lcl_pRoleListBoxTabs[] =
	{	3,        // Number of Tabs
		0, 0, 75
	};

void lcl_ShowChooserButton(
    ::chart::RangeSelectionButton & rChooserButton,
    Edit & rEditField,
    BOOL bShow )
{
    if( rChooserButton.IsVisible() != bShow )
    {
        rChooserButton.Show( bShow );
        sal_Int32 nWidhtDiff = 12 + 4;
        if( bShow )
            nWidhtDiff = -nWidhtDiff;
        Size aSize = rChooserButton.PixelToLogic( rEditField.GetSizePixel(), MAP_APPFONT );
        aSize.setWidth( aSize.getWidth() + nWidhtDiff );
        rEditField.SetSizePixel( rChooserButton.LogicToPixel( aSize, MAP_APPFONT ));
    }
}

void lcl_enableRangeChoosing( bool bEnable, Dialog * pDialog )
{
    if( pDialog )
    {
        pDialog->Show( bEnable ? FALSE : TRUE );
        pDialog->SetModalInputMode( bEnable ? FALSE : TRUE );
    }
}

void lcl_addLSequenceToDataSource(
    const Reference< chart2::data::XLabeledDataSequence > & xLSequence,
    const Reference< chart2::data::XDataSource > & xSource )
{
    Reference< data::XDataSink > xSink( xSource, uno::UNO_QUERY );
    if( xSink.is())
    {
        Sequence< Reference< chart2::data::XLabeledDataSequence > > aData( xSource->getDataSequences());
        aData.realloc( aData.getLength() + 1 );
        aData[ aData.getLength() - 1 ] = xLSequence;
        xSink->setData( aData );
    }
}

Reference< chart2::data::XLabeledDataSequence > lcl_findLSequenceWithOnlyLabel(
    const Reference< chart2::data::XDataSource > & xDataSource )
{
    Reference< chart2::data::XLabeledDataSequence > xResult;
    Sequence< Reference< chart2::data::XLabeledDataSequence > > aSequences( xDataSource->getDataSequences());

    for( sal_Int32 i=0; i<aSequences.getLength(); ++i )
    {
        // no values are set but a label exists
        if( ! aSequences[i]->getValues().is() &&
            aSequences[i]->getLabel().is())
        {
            xResult.set( aSequences[i] );
            break;
        }
    }

    return xResult;
}

void lcl_shiftControlY( Control & rControl, long nYOffset )
{
    Point aPos( rControl.GetPosPixel());
    aPos.setY( aPos.getY() + nYOffset );
    rControl.SetPosPixel( aPos );
}

void lcl_increaseHeightOfControl( Control & rControl, long nYOffset )
{
    Size aSize( rControl.GetSizePixel());
    aSize.setHeight( aSize.getHeight () + nYOffset );
    rControl.SetSizePixel( aSize );
}

} //  anonymous namespace

// --------------------------------------------------------------------------------

namespace chart
{

DataSourceTabPage::DataSourceTabPage(
    Window * pParent,
    DialogModel & rDialogModel,
    ChartTypeTemplateProvider* pTemplateProvider,
    Dialog * pParentDialog,
    bool bHideDescription /* = false */ ) :
        ::svt::OWizardPage( pParent, SchResId( TP_DATA_SOURCE )),

    m_aFT_CAPTION     ( this, SchResId( FT_CAPTION_FOR_WIZARD )),
    m_aFT_SERIES      ( this, SchResId( FT_SERIES      )),
    m_apLB_SERIES( new SeriesListBox( this, SchResId( LB_SERIES ))),
    m_aBTN_ADD        ( this, SchResId( BTN_ADD        )),
    m_aBTN_REMOVE     ( this, SchResId( BTN_REMOVE     )),
    m_aBTN_UP         ( this, SchResId( BTN_UP         )),
    m_aBTN_DOWN       ( this, SchResId( BTN_DOWN       )),
    m_aFT_ROLE        ( this, SchResId( FT_ROLE        )),
    m_aLB_ROLE        ( this, SchResId( LB_ROLE        )),
    m_aFT_RANGE       ( this, SchResId( FT_RANGE       )),
    m_aEDT_RANGE      ( this, SchResId( EDT_RANGE      )),
    m_aIMB_RANGE_MAIN ( this, SchResId( IMB_RANGE_MAIN )),
    m_aFT_CATEGORIES  ( this, SchResId( FT_CATEGORIES  )),
    m_aFT_DATALABELS  ( this, SchResId( FT_DATALABELS  )),
    m_aEDT_CATEGORIES ( this, SchResId( EDT_CATEGORIES )),
    m_aIMB_RANGE_CAT  ( this, SchResId( IMB_RANGE_CAT  )),

    m_pTemplateProvider( pTemplateProvider ),
    m_rDialogModel( rDialogModel ),

    m_pCurrentRangeChoosingField( 0 ),
    m_bIsDirty( false ),
    m_pParentDialog( pParentDialog ),
    m_pTabPageNotifiable( dynamic_cast< TabPageNotifiable * >( pParentDialog ))
{
    FreeResource();

    if( bHideDescription )
    {
        // note: the offset should be a negative value for shifting upwards, the
        // 4 is for the offset difference between a wizard page and a tab-page
        long nYOffset = - ( m_aFT_SERIES.GetPosPixel().getY() - m_aFT_CAPTION.GetPosPixel().getY() + 4 );
        long nUpShift = - 2;
        long nYResize = - (nYOffset - nUpShift);
        m_aFT_CAPTION.Hide();

        // shift list boxes and enlarge them by the space saved by hiding the caption
        lcl_shiftControlY( m_aFT_SERIES, nYOffset );
        lcl_shiftControlY( *(m_apLB_SERIES.get()), nYOffset );
        lcl_increaseHeightOfControl( *(m_apLB_SERIES.get()), nYResize );

        lcl_shiftControlY( m_aFT_ROLE, nYOffset );
        lcl_shiftControlY( m_aLB_ROLE, nYOffset );
        lcl_increaseHeightOfControl( m_aLB_ROLE, nYResize );

        lcl_shiftControlY( m_aBTN_ADD, nUpShift );
        lcl_shiftControlY( m_aBTN_REMOVE, nUpShift );
        lcl_shiftControlY( m_aBTN_UP, nUpShift );
        lcl_shiftControlY( m_aBTN_DOWN, nUpShift );
        lcl_shiftControlY( m_aFT_RANGE, nUpShift );
        lcl_shiftControlY( m_aEDT_RANGE, nUpShift );
        lcl_shiftControlY( m_aIMB_RANGE_MAIN, nUpShift );
        lcl_shiftControlY( m_aFT_CATEGORIES, nUpShift );
        lcl_shiftControlY( m_aFT_DATALABELS, nUpShift );
        lcl_shiftControlY( m_aEDT_CATEGORIES, nUpShift );
        lcl_shiftControlY( m_aIMB_RANGE_CAT, nUpShift );
    }
    else
    {
        // make font of caption bold
        Font aFont( m_aFT_CAPTION.GetControlFont() );
        aFont.SetWeight( WEIGHT_BOLD );
        m_aFT_CAPTION.SetControlFont( aFont );

        // no mnemonic
        m_aFT_CAPTION.SetStyle( m_aFT_CAPTION.GetStyle() | WB_NOLABEL );
    }

    m_aFixedTextRange = OUString( m_aFT_RANGE.GetText() );
    this->SetText( String( SchResId( STR_OBJECT_DATASERIES_PLURAL ) ) );

    // set handlers
    m_apLB_SERIES->SetSelectHdl( LINK( this, DataSourceTabPage, SeriesSelectionChangedHdl ));

	m_aLB_ROLE.SetWindowBits( WB_HSCROLL | WB_CLIPCHILDREN );
    m_aLB_ROLE.SetSelectionMode( SINGLE_SELECTION );
    m_aLB_ROLE.SetSelectHdl( LINK( this, DataSourceTabPage, RoleSelectionChangedHdl ));

    m_aEDT_RANGE.SetKeyInputHdl( LINK( this, DataSourceTabPage, MainRangeButtonClickedHdl ));
    m_aEDT_CATEGORIES.SetKeyInputHdl( LINK( this, DataSourceTabPage, CategoriesRangeButtonClickedHdl ));

    m_aIMB_RANGE_MAIN.SetClickHdl( LINK( this, DataSourceTabPage, MainRangeButtonClickedHdl ));
    m_aIMB_RANGE_CAT.SetClickHdl( LINK( this, DataSourceTabPage, CategoriesRangeButtonClickedHdl ));

    m_aBTN_ADD.SetClickHdl( LINK( this, DataSourceTabPage, AddButtonClickedHdl ));
    m_aBTN_REMOVE.SetClickHdl( LINK( this, DataSourceTabPage, RemoveButtonClickedHdl ));

    m_aBTN_UP.SetClickHdl( LINK( this, DataSourceTabPage, UpButtonClickedHdl ));
    m_aBTN_DOWN.SetClickHdl( LINK( this, DataSourceTabPage, DownButtonClickedHdl ));

    m_aEDT_RANGE.SetModifyHdl( LINK( this, DataSourceTabPage, RangeModifiedHdl ));
    m_aEDT_CATEGORIES.SetModifyHdl( LINK( this, DataSourceTabPage, RangeModifiedHdl ));
    m_aEDT_RANGE.SetUpdateDataHdl( LINK( this, DataSourceTabPage, RangeUpdateDataHdl ));
    m_aEDT_CATEGORIES.SetUpdateDataHdl( LINK( this, DataSourceTabPage, RangeUpdateDataHdl ));
    
    // #i75179# enable setting the background to a different color
    m_aEDT_RANGE.SetStyle( m_aEDT_RANGE.GetStyle() | WB_FORCECTRLBACKGROUND );
    m_aEDT_CATEGORIES.SetStyle( m_aEDT_CATEGORIES.GetStyle() | WB_FORCECTRLBACKGROUND );

    // set symbol font for arrows
    // note: StarSymbol is substituted to OpenSymbol for OOo
    Font aSymbolFont( m_aBTN_UP.GetFont());
    aSymbolFont.SetName( String( RTL_CONSTASCII_USTRINGPARAM( "StarSymbol" )));
    m_aBTN_UP.SetControlFont( aSymbolFont );
    m_aBTN_DOWN.SetControlFont( aSymbolFont );

    // set button text
    sal_Unicode cBlackUpPointingTriangle( 0x25b2 );
    sal_Unicode cBlackDownPointingTriangle( 0x25bc );
    m_aBTN_UP.SetText( String( cBlackUpPointingTriangle ));
    m_aBTN_DOWN.SetText( String( cBlackDownPointingTriangle ));

    // init controls
    m_aLB_ROLE.SetTabs( lcl_pRoleListBoxTabs, MAP_APPFONT );
    m_aLB_ROLE.Show();

    updateControlsFromDialogModel();

    // select first series
    if( m_apLB_SERIES->First())
        m_apLB_SERIES->Select( m_apLB_SERIES->First());
    m_apLB_SERIES->GrabFocus();
}

DataSourceTabPage::~DataSourceTabPage()
{}

void DataSourceTabPage::ActivatePage()
{
    OWizardPage::ActivatePage();
    updateControlsFromDialogModel();
}

void DataSourceTabPage::initializePage()
{
}

void DataSourceTabPage::DeactivatePage()
{
    commitPage();
    svt::OWizardPage::DeactivatePage();
}

void DataSourceTabPage::commitPage()
{
    commitPage(eFinish);
}

sal_Bool DataSourceTabPage::commitPage( CommitPageReason /*eReason*/ )
{
    //ranges may have been edited in the meanwhile (dirty is true in that case here)
    if( isValid() )
    {
        updateModelFromControl( 0 /*update all*/ );
        return sal_True;//return false if this page should not be left
    }
    else
        return sal_False;
}

bool DataSourceTabPage::isRangeFieldContentValid( Edit & rEdit )
{
    OUString aRange( rEdit.GetText());
    bool bIsValid = ( aRange.getLength() == 0 ) ||
        m_rDialogModel.getRangeSelectionHelper()->verifyCellRange( aRange );

    if( bIsValid )
    {
        rEdit.SetControlForeground();
        rEdit.SetControlBackground();
    }
    else
    {
        rEdit.SetControlBackground( RANGE_SELECTION_INVALID_RANGE_BACKGROUND_COLOR );
        rEdit.SetControlForeground( RANGE_SELECTION_INVALID_RANGE_FOREGROUND_COLOR );
    }

    return bIsValid;
}

bool DataSourceTabPage::isValid()
{
    bool bRoleRangeValid = true;
    bool bCategoriesRangeValid = true;
    bool bHasSelectedEntry = (m_apLB_SERIES->FirstSelected() != 0);

    if( bHasSelectedEntry )
        bRoleRangeValid = isRangeFieldContentValid( m_aEDT_RANGE );
    if( m_aEDT_CATEGORIES.IsEnabled() )
        bCategoriesRangeValid = isRangeFieldContentValid( m_aEDT_CATEGORIES );
    bool bValid = ( bRoleRangeValid && bCategoriesRangeValid );

    if( m_pTabPageNotifiable )
    {
        if( bValid )
            m_pTabPageNotifiable->setValidPage( this );
        else
            m_pTabPageNotifiable->setInvalidPage( this );
    }

    return bValid;
}

void DataSourceTabPage::setDirty()
{
    m_bIsDirty = true;
}

void DataSourceTabPage::updateControlsFromDialogModel()
{
    // series
    fillSeriesListBox();
    SeriesSelectionChangedHdl( 0 );

    // categories
    m_aEDT_CATEGORIES.SetText( String( m_rDialogModel.getCategoriesRange() ));

    updateControlState();
}

void DataSourceTabPage::fillSeriesListBox()
{
    m_apLB_SERIES->SetUpdateMode( FALSE );

    Reference< XDataSeries > xSelected;
    SeriesEntry * pEntry = dynamic_cast< SeriesEntry * >( m_apLB_SERIES->FirstSelected());
    if( pEntry )
        xSelected.set( pEntry->m_xDataSeries );

    bool bHasSelectedEntry = (pEntry != 0);
    SvLBoxEntry * pSelectedEntry = 0;
    m_apLB_SERIES->Clear();

    ::std::vector< DialogModel::tSeriesWithChartTypeByName > aSeries(
        m_rDialogModel.getAllDataSeriesWithLabel() );

    sal_Int32 nUnnamedSeriesIndex = 1;
    for( ::std::vector< DialogModel::tSeriesWithChartTypeByName >::const_iterator aIt = aSeries.begin();
         aIt != aSeries.end(); ++aIt )
    {
        String aLabel( (*aIt).first );
        if( !aLabel.Len())
        {
            if( nUnnamedSeriesIndex > 1 )
            {
                OUString aResString( String( ::chart::SchResId( STR_DATA_UNNAMED_SERIES_WITH_INDEX )));

                // replace index of unnamed series
                const OUString aReplacementStr( RTL_CONSTASCII_USTRINGPARAM( "%NUMBER" ));
                sal_Int32 nIndex = aResString.indexOf( aReplacementStr );
                if( nIndex != -1 )
                    aLabel = String( aResString.replaceAt(
                                         nIndex, aReplacementStr.getLength(),
                                         String::CreateFromInt32( nUnnamedSeriesIndex )));
            }
            if( aLabel.Len() == 0 )
                aLabel = String( ::chart::SchResId( STR_DATA_UNNAMED_SERIES ));

            ++nUnnamedSeriesIndex;
        }
        pEntry = dynamic_cast< SeriesEntry * >(
            m_apLB_SERIES->InsertEntry( aLabel ));
        if( pEntry )
        {
            pEntry->m_xDataSeries.set( (*aIt).second.first );
            pEntry->m_xChartType.set(  (*aIt).second.second );
            if( bHasSelectedEntry && ((*aIt).second.first == xSelected))
                pSelectedEntry = pEntry;
        }
    }

    if( bHasSelectedEntry && pSelectedEntry )
        m_apLB_SERIES->Select( pSelectedEntry );

    m_apLB_SERIES->SetUpdateMode( TRUE );
}

void DataSourceTabPage::fillRoleListBox()
{
    SeriesEntry * pSeriesEntry = dynamic_cast< SeriesEntry * >( m_apLB_SERIES->FirstSelected());
    bool bHasSelectedEntry = (pSeriesEntry != 0);

    SvLBoxEntry * pRoleEntry =  m_aLB_ROLE.FirstSelected();
    ULONG nRoleIndex = SAL_MAX_UINT32;
    if( pRoleEntry )
        nRoleIndex = m_aLB_ROLE.GetModel()->GetAbsPos( pRoleEntry );

    if( bHasSelectedEntry )
    {
        DialogModel::tRolesWithRanges aRoles(
            m_rDialogModel.getRolesWithRanges(
                pSeriesEntry->m_xDataSeries,
                lcl_GetSequenceNameForLabel( pSeriesEntry ),
                pSeriesEntry->m_xChartType ));

        // fill role list
        m_aLB_ROLE.SetUpdateMode( FALSE );
        m_aLB_ROLE.Clear();
        m_aLB_ROLE.RemoveSelection();

        for( DialogModel::tRolesWithRanges::const_iterator aIt( aRoles.begin());
             aIt != aRoles.end(); ++ aIt )
        {
            m_aLB_ROLE.InsertEntry( lcl_GetRoleLBEntry( aIt->first, aIt->second ));
        }

        // series may contain no roles, check listbox size before selecting entries
        if( m_aLB_ROLE.GetEntryCount() > 0 )
        {
            if( nRoleIndex >= m_aLB_ROLE.GetEntryCount())
                nRoleIndex = 0;
            m_aLB_ROLE.Select( m_aLB_ROLE.GetEntry( nRoleIndex ));
        }

        m_aLB_ROLE.SetUpdateMode( TRUE );
    }
}

void DataSourceTabPage::updateControlState()
{
    SvLBoxEntry * pSeriesEntry = m_apLB_SERIES->FirstSelected();
    bool bHasSelectedSeries = (pSeriesEntry != 0);
    bool bHasValidRole = false;
    bool bHasRangeChooser = m_rDialogModel.getRangeSelectionHelper()->hasRangeSelection();

    if( bHasSelectedSeries )
    {
        SvLBoxEntry * pRoleEntry =  m_aLB_ROLE.FirstSelected();
        bHasValidRole = (pRoleEntry != 0);
    }

    m_aBTN_ADD.Enable( true );
    m_aBTN_REMOVE.Enable( bHasSelectedSeries );

    m_aBTN_UP.Enable( bHasSelectedSeries && (pSeriesEntry != m_apLB_SERIES->First()));
    m_aBTN_DOWN.Enable( bHasSelectedSeries && (pSeriesEntry != m_apLB_SERIES->Last()));

    bool bHasCategories = m_rDialogModel.isCategoryDiagram();

    m_aFT_DATALABELS.Show(!bHasCategories);
    m_aFT_CATEGORIES.Show( bHasCategories);
    BOOL bShowIB = bHasRangeChooser;
    lcl_ShowChooserButton( m_aIMB_RANGE_CAT, m_aEDT_CATEGORIES, bShowIB );

    m_aFT_SERIES.Enable();
    m_apLB_SERIES->Enable();

    m_aFT_ROLE.Enable( bHasSelectedSeries );
    m_aLB_ROLE.Enable( bHasSelectedSeries );

    m_aFT_RANGE.Enable( bHasValidRole );
    m_aEDT_RANGE.Enable( bHasValidRole );
    lcl_ShowChooserButton( m_aIMB_RANGE_MAIN, m_aEDT_RANGE, bShowIB );
    isValid();
}

IMPL_LINK( DataSourceTabPage, SeriesSelectionChangedHdl, void *, EMPTYARG )
{
    m_rDialogModel.startControllerLockTimer();
    if( m_apLB_SERIES->FirstSelected())
    {
        fillRoleListBox();
        RoleSelectionChangedHdl( 0 );
    }
    updateControlState();

    return 0;
}

IMPL_LINK( DataSourceTabPage, RoleSelectionChangedHdl, void *, EMPTYARG )
{
    m_rDialogModel.startControllerLockTimer();
    SvLBoxEntry * pEntry = m_aLB_ROLE.FirstSelected();
    if( pEntry )
    {
        OUString aRange( m_aEDT_RANGE.GetText());
        OUString aSelectedRoleUI = lcl_GetSelectedRole( m_aLB_ROLE, true );
        OUString aSelectedRange = lcl_GetSelectedRolesRange( m_aLB_ROLE );

        // replace role in fixed text label
        const OUString aReplacementStr( RTL_CONSTASCII_USTRINGPARAM( "%VALUETYPE" ));
        sal_Int32 nIndex = m_aFixedTextRange.indexOf( aReplacementStr );
        if( nIndex != -1 )
        {
            m_aFT_RANGE.SetText(
                String( m_aFixedTextRange.replaceAt(
                            nIndex, aReplacementStr.getLength(), aSelectedRoleUI )));
        }

        m_aEDT_RANGE.SetText( String( aSelectedRange ));
        isValid();
    }

    return 0;
}

IMPL_LINK( DataSourceTabPage, MainRangeButtonClickedHdl, void *, EMPTYARG )
{
    OSL_ASSERT( m_pCurrentRangeChoosingField == 0 );
    m_pCurrentRangeChoosingField = & m_aEDT_RANGE;
    if( m_aEDT_RANGE.GetText().Len() > 0 &&
        ! updateModelFromControl( m_pCurrentRangeChoosingField ))
        return 0;

    SeriesEntry * pEntry = dynamic_cast< SeriesEntry * >( m_apLB_SERIES->FirstSelected());
    bool bHasSelectedEntry = (pEntry != 0);

    OUString aSelectedRolesRange = lcl_GetSelectedRolesRange( m_aLB_ROLE );

    if( bHasSelectedEntry && (m_aLB_ROLE.FirstSelected() != 0))
    {
        String aStr( SchResId( STR_DATA_SELECT_RANGE_FOR_SERIES ));
        OUString aUIStr( aStr );

        // replace role
        OUString aReplacement( RTL_CONSTASCII_USTRINGPARAM( "%VALUETYPE" ));
        sal_Int32 nIndex = aUIStr.indexOf( aReplacement );
        if( nIndex != -1 )
        {
            aUIStr = aUIStr.replaceAt( nIndex, aReplacement.getLength(),
                                       lcl_GetSelectedRole( m_aLB_ROLE, true ));
        }
        // replace series name
        aReplacement = C2U( "%SERIESNAME" );
        nIndex = aUIStr.indexOf( aReplacement );
        if( nIndex != -1 )
        {
            aUIStr = aUIStr.replaceAt( nIndex, aReplacement.getLength(),
                                       OUString( m_apLB_SERIES->GetEntryText( pEntry )));
        }

        lcl_enableRangeChoosing( true, m_pParentDialog );
        m_rDialogModel.getRangeSelectionHelper()->chooseRange( aSelectedRolesRange, aUIStr, *this );
    }
    else
        m_pCurrentRangeChoosingField = 0;

    return 0;
}

IMPL_LINK( DataSourceTabPage, CategoriesRangeButtonClickedHdl, void *, EMPTYARG )
{
    OSL_ASSERT( m_pCurrentRangeChoosingField == 0 );
    m_pCurrentRangeChoosingField = & m_aEDT_CATEGORIES;
    if( m_aEDT_CATEGORIES.GetText().Len() > 0 &&
        ! updateModelFromControl( m_pCurrentRangeChoosingField ))
        return 0;

    String aStr( SchResId( m_aFT_CATEGORIES.IsVisible() ? STR_DATA_SELECT_RANGE_FOR_CATEGORIES : STR_DATA_SELECT_RANGE_FOR_DATALABELS ));
    lcl_enableRangeChoosing( true, m_pParentDialog );
    m_rDialogModel.getRangeSelectionHelper()->chooseRange(
        m_rDialogModel.getCategoriesRange(), OUString( aStr ), *this );
    return 0;
}

IMPL_LINK( DataSourceTabPage, AddButtonClickedHdl, void *, EMPTYARG )
{
    m_rDialogModel.startControllerLockTimer();
    SeriesEntry * pEntry = dynamic_cast< SeriesEntry * >( m_apLB_SERIES->FirstSelected());
    Reference< XDataSeries > xSeriesToInsertAfter;
    Reference< XChartType > xChartTypeForNewSeries;
    if( m_pTemplateProvider )
            m_rDialogModel.setTemplate( m_pTemplateProvider->getCurrentTemplate());

    if( pEntry )
    {
        xSeriesToInsertAfter.set( pEntry->m_xDataSeries );
        xChartTypeForNewSeries.set( pEntry->m_xChartType );
    }
    else
    {
        ::std::vector< Reference< XDataSeriesContainer > > aCntVec(
            m_rDialogModel.getAllDataSeriesContainers());
        if( ! aCntVec.empty())
            xChartTypeForNewSeries.set( aCntVec.front(), uno::UNO_QUERY );
    }
    OSL_ENSURE( xChartTypeForNewSeries.is(), "Cannot insert new series" );

    m_rDialogModel.insertSeriesAfter( xSeriesToInsertAfter, xChartTypeForNewSeries );
    setDirty();

    fillSeriesListBox();
    // note the box was cleared and refilled, so pEntry is invalid now
    SvLBoxEntry * pSelEntry = m_apLB_SERIES->FirstSelected();
    if( pSelEntry )
    {
        SvLBoxEntry * pNextEntry = m_apLB_SERIES->Next( pSelEntry );
        if( pNextEntry )
            m_apLB_SERIES->Select( pNextEntry );
    }
    SeriesSelectionChangedHdl( 0 );

    return 0;
}

IMPL_LINK( DataSourceTabPage, RemoveButtonClickedHdl, void *, EMPTYARG )
{
    m_rDialogModel.startControllerLockTimer();
    SeriesEntry * pEntry = dynamic_cast< SeriesEntry * >( m_apLB_SERIES->FirstSelected());
    if( pEntry )
    {
        Reference< XDataSeries > xNewSelSeries;
        SeriesEntry * pNewSelEntry = dynamic_cast< SeriesEntry * >(
            m_apLB_SERIES->Next( pEntry ));
        if( pNewSelEntry )
            xNewSelSeries.set( pNewSelEntry->m_xDataSeries );
        else
        {
            pNewSelEntry = dynamic_cast< SeriesEntry * >( m_apLB_SERIES->Prev( pEntry ));
            if( pNewSelEntry )
                xNewSelSeries.set( pNewSelEntry->m_xDataSeries );
        }

        m_rDialogModel.deleteSeries( pEntry->m_xDataSeries, pEntry->m_xChartType );
        setDirty();

        m_apLB_SERIES->RemoveSelection();
        fillSeriesListBox();

        // select previous or next series
        //@improve: see methods GetModel()->GetAbsPos()/GetEntry() for absoulte list positions
        if( xNewSelSeries.is())
        {
            pEntry = dynamic_cast< SeriesEntry * >( m_apLB_SERIES->First());
            while( pEntry )
            {
                if( pEntry->m_xDataSeries == xNewSelSeries )
                {
                    m_apLB_SERIES->Select( pEntry );
                    break;
                }
                pEntry = dynamic_cast< SeriesEntry * >( m_apLB_SERIES->Next( pEntry ));
            }
        }
        SeriesSelectionChangedHdl( 0 );
    }

    return 0;
}

IMPL_LINK( DataSourceTabPage, UpButtonClickedHdl, void *, EMPTYARG )
{
    m_rDialogModel.startControllerLockTimer();
    SeriesEntry * pEntry = dynamic_cast< SeriesEntry * >( m_apLB_SERIES->FirstSelected());
    bool bHasSelectedEntry = (pEntry != 0);

    if( bHasSelectedEntry )
    {
        m_rDialogModel.moveSeries( pEntry->m_xDataSeries, DialogModel::MOVE_UP );
        setDirty();
        fillSeriesListBox();
        SeriesSelectionChangedHdl(0);
    }

    return 0;
}

IMPL_LINK( DataSourceTabPage, DownButtonClickedHdl, void *, EMPTYARG )
{
    m_rDialogModel.startControllerLockTimer();
    SeriesEntry * pEntry = dynamic_cast< SeriesEntry * >( m_apLB_SERIES->FirstSelected());
    bool bHasSelectedEntry = (pEntry != 0);

    if( bHasSelectedEntry )
    {
        m_rDialogModel.moveSeries( pEntry->m_xDataSeries, DialogModel::MOVE_DOWN );
        setDirty();
        fillSeriesListBox();
        SeriesSelectionChangedHdl(0);
    }

    return 0;
}

IMPL_LINK( DataSourceTabPage, RangeModifiedHdl, Edit*, pEdit )
{
    if( isRangeFieldContentValid( *pEdit ))
        setDirty();

    // enable/disable OK button
    isValid();

    return 0;
}

IMPL_LINK( DataSourceTabPage, RangeUpdateDataHdl, Edit*, pEdit )
{
    // note: isValid sets the color of the edit field
    if( isRangeFieldContentValid( *pEdit ))
    {
        setDirty();
        updateModelFromControl( pEdit );
        if( pEdit== &m_aEDT_RANGE )
        {
            if( ! lcl_UpdateCurrentSeriesName( *m_apLB_SERIES ))
                fillSeriesListBox();
        }
    }
    // enable/disable OK button
    isValid();

    return 0;
}

void DataSourceTabPage::listeningFinished(
    const ::rtl::OUString & rNewRange )
{
    // rNewRange becomes invalid after removing the listener
    OUString aRange( rNewRange );

    m_rDialogModel.startControllerLockTimer();

    // stop listening
    m_rDialogModel.getRangeSelectionHelper()->stopRangeListening();

    // change edit field
    ToTop();
    GrabFocus();
    if( m_pCurrentRangeChoosingField )
    {
        m_pCurrentRangeChoosingField->SetText( String( aRange ));
        m_pCurrentRangeChoosingField->GrabFocus();
    }

    if( m_pCurrentRangeChoosingField == & m_aEDT_RANGE )
    {
        m_aEDT_RANGE.SetText( String( aRange ));
        setDirty();
    }
    else if( m_pCurrentRangeChoosingField == & m_aEDT_CATEGORIES )
    {
        m_aEDT_CATEGORIES.SetText( String( aRange ));
        setDirty();
    }

    updateModelFromControl( m_pCurrentRangeChoosingField );
    if( ! lcl_UpdateCurrentSeriesName( *m_apLB_SERIES ))
        fillSeriesListBox();

    m_pCurrentRangeChoosingField = 0;

    updateControlState();
    lcl_enableRangeChoosing( false, m_pParentDialog );
}

void DataSourceTabPage::disposingRangeSelection()
{
    m_rDialogModel.getRangeSelectionHelper()->stopRangeListening( false );
}

bool DataSourceTabPage::updateModelFromControl( Edit * pField )
{
    if( !m_bIsDirty )
        return true;

    ControllerLockGuard aLockedControllers( m_rDialogModel.getChartModel() );

    // @todo: validity check of field content
    bool bResult = true;
    bool bAll = (pField == 0);
    Reference< data::XDataProvider > xDataProvider( m_rDialogModel.getDataProvider());

    if( bAll || (pField == & m_aEDT_CATEGORIES) )
    {
        Reference< data::XLabeledDataSequence > xLabeledSeq( m_rDialogModel.getCategories() );
        if( xDataProvider.is())
        {
            OUString aRange( m_aEDT_CATEGORIES.GetText());
            if( aRange.getLength())
            {
                // create or change categories
                if( !xLabeledSeq.is())
                {
                    xLabeledSeq.set( DataSourceHelper::createLabeledDataSequence( Reference< uno::XComponentContext >(0)));
                    m_rDialogModel.setCategories( xLabeledSeq );
                }
                try
                {
                    xLabeledSeq->setValues( xDataProvider->createDataSequenceByRangeRepresentation( aRange ));
                }
                catch( const uno::Exception & ex )
                {
                    // should work as validation should have happened before
                    ASSERT_EXCEPTION( ex );
                }
            }
            else if( xLabeledSeq.is())
            {
                // clear existing categories
                xLabeledSeq.set(0);
                m_rDialogModel.setCategories( xLabeledSeq );
            }
        }
    }

    SeriesEntry * pSeriesEntry = dynamic_cast< SeriesEntry * >( m_apLB_SERIES->FirstSelected());
    bool bHasSelectedEntry = (pSeriesEntry != 0);

    if( bHasSelectedEntry )
    {
        if( bAll || (pField == & m_aEDT_RANGE) )
        {
            try
            {
                OUString aSelectedRole = lcl_GetSelectedRole( m_aLB_ROLE );
                OUString aRange( m_aEDT_RANGE.GetText());
                OUString aSequenceRole( aSelectedRole );
                bool bIsLabel = aSequenceRole.equals( lcl_aLabelRole );
                OUString aSequenceNameForLabel( lcl_GetSequenceNameForLabel( pSeriesEntry ));

                if( bIsLabel )
                    aSequenceRole = aSequenceNameForLabel;

                Reference< data::XDataSource > xSource( pSeriesEntry->m_xDataSeries, uno::UNO_QUERY_THROW );
                Reference< data::XLabeledDataSequence > xLabeledSeq(
                    DataSeriesHelper::getDataSequenceByRole( xSource, aSequenceRole ));

                if( xDataProvider.is())
                {
                    if( bIsLabel )
                    {
                        if( ! xLabeledSeq.is())
                        {
                            // check if there is already an "orphan" label sequence
                            xLabeledSeq.set( lcl_findLSequenceWithOnlyLabel( xSource ));
                            if( ! xLabeledSeq.is())
                            {
                                // no corresponding labeled data sequence for label found
                                xLabeledSeq.set( DataSourceHelper::createLabeledDataSequence( Reference< uno::XComponentContext >(0)));
                                lcl_addLSequenceToDataSource( xLabeledSeq, xSource );
                            }
                        }
                        if( xLabeledSeq.is())
                        {
                            if( aRange.getLength())
                            {
                                Reference< data::XDataSequence > xNewSeq;
                                try
                                {
                                    xNewSeq.set( xDataProvider->createDataSequenceByRangeRepresentation( aRange ));
                                }
                                catch( const uno::Exception & ex )
                                {
                                    // should work as validation should have happened before
                                    ASSERT_EXCEPTION( ex );
                                }
                                if( xNewSeq.is())
                                {
                                    // update range name by the full string provided
                                    // by the data provider. E.g. "a1" might become
                                    // "$Sheet1.$A$1"
                                    aRange = xNewSeq->getSourceRangeRepresentation();
                                    Reference< beans::XPropertySet > xProp( xNewSeq, uno::UNO_QUERY_THROW );
                                    xProp->setPropertyValue( C2U("Role"), uno::makeAny( lcl_aLabelRole ));
                                    xLabeledSeq->setLabel( xNewSeq );
                                }
                            }
                            else
                            {
                                xLabeledSeq->setLabel( Reference< data::XDataSequence >());
                            }
                        }
                    }
                    else
                    {
                        if( aRange.getLength())
                        {
                            Reference< data::XDataSequence > xNewSeq;
                            try
                            {
                                xNewSeq.set( xDataProvider->createDataSequenceByRangeRepresentation( aRange ));
                            }
                            catch( const uno::Exception & ex )
                            {
                                // should work as validation should have happened before
                                ASSERT_EXCEPTION( ex );
                            }
                            if( xNewSeq.is())
                            {
                                // update range name by the full string provided
                                // by the data provider. E.g. "a1:e1" might become
                                // "$Sheet1.$A$1:$E$1"
                                aRange = xNewSeq->getSourceRangeRepresentation();

                                Reference< beans::XPropertySet > xProp( xNewSeq, uno::UNO_QUERY_THROW );
                                xProp->setPropertyValue( C2U("Role"), uno::makeAny( aSelectedRole ));
                                if( !xLabeledSeq.is())
                                {
                                    if( aSelectedRole.equals( aSequenceNameForLabel ))
                                        xLabeledSeq.set( lcl_findLSequenceWithOnlyLabel( xSource ));
                                    if( ! xLabeledSeq.is())
                                    {
                                        xLabeledSeq.set( DataSourceHelper::createLabeledDataSequence( Reference< uno::XComponentContext >(0)));
                                        lcl_addLSequenceToDataSource( xLabeledSeq, xSource );
                                    }
                                }
                                xLabeledSeq->setValues( xNewSeq );
                            }
                        }
                        else if( xLabeledSeq.is())
                        {
                            // values cannot be deleted. This would also delete the Role (for labels)
//                             xLabeledSeq->setValues( Reference< data::XDataSequence >());
                        }
                    }
                }

                lcl_UpdateCurrentRange( m_aLB_ROLE, aSelectedRole, aRange );
            }
            catch( uno::Exception & ex )
            {
                bResult = false;
                ASSERT_EXCEPTION( ex );
            }
        }
    }

    // update View
    // @todo remove this when automatic view updates from calc, writer and own data sequences are available
    if( bResult )
    {
        try
        {
            Reference< util::XModifiable > xModifiable( m_rDialogModel.getChartModel(), uno::UNO_QUERY );
            if( xModifiable.is() )
                xModifiable->setModified( sal_True );
        }
        catch( uno::Exception & ex )
        {
            ASSERT_EXCEPTION( ex );
        }
    }

    return bResult;
}

} //  namespace chart
