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

// header for class SvNumberformat
#ifndef _ZFORMAT_HXX
#ifndef _ZFORLIST_DECLARE_TABLE
#define _ZFORLIST_DECLARE_TABLE
#endif
#include <svtools/zformat.hxx>
#endif
// header for SvNumberFormatter
#include <svtools/zforlist.hxx>

#include "DataBrowser.hxx"
#include "DataBrowserModel.hxx"
#include "Strings.hrc"
#include "ContainerHelper.hxx"
#include "DataSeriesHelper.hxx"
#include "DiagramHelper.hxx"
#include "ChartModelHelper.hxx"
#include "chartview/NumberFormatterWrapper.hxx"
#include "servicenames_charttypes.hxx"
#include "ResId.hxx"
#include "Bitmaps.hrc"
#include "Bitmaps_HC.hrc"
#include "HelpIds.hrc"

#include <vcl/fixed.hxx>
#include <vcl/image.hxx>
#include <vcl/msgbox.hxx>
#include <rtl/math.hxx>

#include <com/sun/star/util/XCloneable.hpp>
#include <com/sun/star/chart2/XChartDocument.hpp>
#include <com/sun/star/chart2/XChartType.hpp>

#include <com/sun/star/container/XIndexReplace.hpp>

#include <algorithm>
#include <functional>

#define SELECT_IMAGE(name,hc) Image( SchResId( hc ? name ## _HC : name ))

/*  BROWSER_COLUMNSELECTION :  single cells may be selected rather than only
                               entire rows
    BROWSER_(H|V)LINES :       show horizontal or vertical grid-lines

    BROWSER_AUTO_(H|V)SCROLL : scroll automated horizontally or vertically when
                               cursor is moved beyond the edge of the dialog
    BROWSER_HIGHLIGHT_NONE :   Do not mark the current row with selection color
                               (usually blue)

 */
#define BROWSER_STANDARD_FLAGS  \
    BROWSER_COLUMNSELECTION | \
    BROWSER_HLINES | BROWSER_VLINES | \
    BROWSER_AUTO_HSCROLL | BROWSER_AUTO_VSCROLL | \
    BROWSER_HIGHLIGHT_NONE

// BROWSER_HIDECURSOR would prevent flickering in edit fields, but navigating
// with shift up/down, and entering non-editable cells would be problematic,
// e.g.  the first cell, or when being in read-only mode


using namespace ::com::sun::star;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::Reference;
using ::rtl::OUString;

using namespace ::svt;

namespace
{
sal_Int32 lcl_getRowInData( long nRow )
{
    return static_cast< sal_Int32 >( nRow );
}

sal_Int32 lcl_getColumnInData( USHORT nCol )
{
    return static_cast< sal_Int32 >( nCol ) - 1;
}

} // anonymous namespace

// --------------------------------------------------------------------------------

namespace chart
{

// ----------------------------------------
namespace impl
{

class SeriesHeaderEdit : public Edit
{
public:
    SeriesHeaderEdit( Window * pParent );
    virtual ~SeriesHeaderEdit();
    virtual void MouseButtonDown( const MouseEvent& rMEvt );

    void setStartColumn( sal_Int32 nStartColumn );
    sal_Int32 getStartColumn() const;
    void SetShowWarningBox( bool bShowWarning = true );

private:
    sal_Int32 m_nStartColumn;
    bool m_bShowWarningBox;
};

SeriesHeaderEdit::SeriesHeaderEdit( Window * pParent ) :
        Edit( pParent ),
        m_nStartColumn( 0 ),
        m_bShowWarningBox( false )
{}
SeriesHeaderEdit::~SeriesHeaderEdit()
{}

void SeriesHeaderEdit::setStartColumn( sal_Int32 nStartColumn )
{
    m_nStartColumn = nStartColumn;
}

sal_Int32 SeriesHeaderEdit::getStartColumn() const
{
    return m_nStartColumn;
}

void SeriesHeaderEdit::SetShowWarningBox( bool bShowWarning )
{
    m_bShowWarningBox = bShowWarning;
}

void SeriesHeaderEdit::MouseButtonDown( const MouseEvent& rMEvt )
{
    Edit::MouseButtonDown( rMEvt );

    if( m_bShowWarningBox )
        WarningBox( this, WinBits( WB_OK ),
                    String( SchResId( STR_INVALID_NUMBER ))).Execute();
}

class SeriesHeader
{
public:
    explicit SeriesHeader( Window * pParent );

    void SetColor( const Color & rCol );
    void SetPos( const Point & rPos );
    void SetWidth( sal_Int32 nWidth );
    void SetChartType( const Reference< chart2::XChartType > & xChartType,
                       bool bSwapXAndYAxis,
                       bool bIsHighContrast );
    void SetSeriesName( const String & rName );
    void SetRange( sal_Int32 nStartCol, sal_Int32 nEndCol );

    void SetPixelPosX( sal_Int32 nPos );
    void SetPixelWidth( sal_Int32 nWidth );

    sal_Int32 GetStartColumn() const;
    sal_Int32 GetEndColumn() const;

    void Show();

    /** call this before destroying the class.  This notifies the listeners to
        changes of the edit field for the series name.
     */
    void applyChanges();

	void SetGetFocusHdl( const Link& rLink );

	void SetEditChangedHdl( const Link & rLink );

    bool HasFocus() const;

private:
    ::boost::shared_ptr< FixedImage >        m_spSymbol;
    ::boost::shared_ptr< SeriesHeaderEdit >  m_spSeriesName;
    ::boost::shared_ptr< FixedText >         m_spColorBar;
    OutputDevice *                           m_pDevice;
    Link                                     m_aChangeLink;

    void notifyChanges();
    DECL_LINK( SeriesNameChanged, void * );
    DECL_LINK( SeriesNameEdited, void * );

    /// @param bHC </TRUE> for hight-contrast image
    static Image GetChartTypeImage(
        const Reference< chart2::XChartType > & xChartType,
        bool bSwapXAndYAxis,
        bool bHC );

    sal_Int32 m_nStartCol, m_nEndCol;
    sal_Int32 m_nWidth;
    Point     m_aPos;
    bool      m_bSeriesNameChangePending;
};

SeriesHeader::SeriesHeader( Window * pParent ) :
        m_spSymbol( new FixedImage( pParent, WB_NOBORDER )),
        m_spSeriesName( new SeriesHeaderEdit( pParent )),
        m_spColorBar( new FixedText( pParent, WB_NOBORDER )),
        m_pDevice( pParent ),
        m_nStartCol( 0 ),
        m_nEndCol( 0 ),
        m_nWidth( 42 ),
        m_aPos( 0, 22 ),
        m_bSeriesNameChangePending( false )
{
    m_spSeriesName->EnableUpdateData( 4 * EDIT_UPDATEDATA_TIMEOUT ); // define is in vcl/edit.hxx
    m_spSeriesName->SetUpdateDataHdl( LINK( this, SeriesHeader, SeriesNameChanged ));
    m_spSeriesName->SetModifyHdl( LINK( this, SeriesHeader, SeriesNameEdited ));
    m_spSeriesName->SetSmartHelpId( SmartId( HID_SCH_DATA_SERIES_LABEL ));
    Show();
}

void SeriesHeader::notifyChanges()
{
    if( m_aChangeLink.IsSet())
		m_aChangeLink.Call( m_spSeriesName.get());

    m_bSeriesNameChangePending = false;
}

void SeriesHeader::applyChanges()
{
    if( m_bSeriesNameChangePending )
    {
        notifyChanges();
    }
}

void SeriesHeader::SetColor( const Color & rCol )
{
    m_spColorBar->SetControlBackground( rCol );
}

void SeriesHeader::SetPos( const Point & rPos )
{
    m_aPos = rPos;

    // chart type symbol
    sal_Int32 nHeight = 10;
    Point aPos( rPos );
    aPos.setY( aPos.getY() + 2 );
    Size aSize( nHeight, nHeight );
    m_spSymbol->SetPosPixel( m_pDevice->LogicToPixel( aPos, MAP_APPFONT ));
    m_spSymbol->SetSizePixel( m_pDevice->LogicToPixel( aSize, MAP_APPFONT ));
    aPos.setY( aPos.getY() - 2 );

    // series name edit field
    aPos.setX( aPos.getX() + nHeight + 2 );
    aSize.setWidth( m_nWidth - nHeight - 2 );
    nHeight = 12;
    aSize.setHeight( nHeight );
    m_spSeriesName->SetPosPixel( m_pDevice->LogicToPixel( aPos, MAP_APPFONT ));
    m_spSeriesName->SetSizePixel( m_pDevice->LogicToPixel( aSize, MAP_APPFONT ));

    // color bar
    aPos.setX( rPos.getX() + 1 );
    aPos.setY( aPos.getY() + nHeight + 2 );
    nHeight = 3;
    aSize.setWidth( m_nWidth - 1 );
    aSize.setHeight( nHeight );
    m_spColorBar->SetPosPixel( m_pDevice->LogicToPixel( aPos, MAP_APPFONT ));
    m_spColorBar->SetSizePixel( m_pDevice->LogicToPixel( aSize, MAP_APPFONT ));
}

void SeriesHeader::SetWidth( sal_Int32 nWidth )
{
    m_nWidth = nWidth;
    SetPos( m_aPos );
}


void SeriesHeader::SetPixelPosX( sal_Int32 nPos )
{
    Point aPos( m_pDevice->LogicToPixel( m_aPos, MAP_APPFONT ));
    aPos.setX( nPos );
    SetPos( m_pDevice->PixelToLogic( aPos, MAP_APPFONT ));
}

void SeriesHeader::SetPixelWidth( sal_Int32 nWidth )
{
    SetWidth( m_pDevice->PixelToLogic( Size( nWidth, 0 ), MAP_APPFONT ).getWidth());
}

void SeriesHeader::SetChartType(
    const Reference< chart2::XChartType > & xChartType,
    bool bSwapXAndYAxis,
    bool bIsHighContrast )
{
    m_spSymbol->SetImage( GetChartTypeImage( xChartType, bSwapXAndYAxis, bIsHighContrast ));
}

void SeriesHeader::SetSeriesName( const String & rName )
{
    m_spSeriesName->SetText( rName );
}

void SeriesHeader::SetRange( sal_Int32 nStartCol, sal_Int32 nEndCol )
{
    m_nStartCol = nStartCol;
    m_nEndCol = (nEndCol > nStartCol) ? nEndCol : nStartCol;
    m_spSeriesName->setStartColumn( nStartCol );
}

sal_Int32 SeriesHeader::GetStartColumn() const
{
    return m_nStartCol;
}

sal_Int32 SeriesHeader::GetEndColumn() const
{
    return m_nEndCol;
}

void SeriesHeader::Show()
{
    m_spSymbol->Show();
    m_spSeriesName->Show();
    m_spColorBar->Show();
}

void SeriesHeader::SetEditChangedHdl( const Link & rLink )
{
    m_aChangeLink = rLink;
}

IMPL_LINK( SeriesHeader, SeriesNameChanged, void * , EMPTYARG )
{
    notifyChanges();
    return 0;
}

IMPL_LINK( SeriesHeader, SeriesNameEdited, void * , EMPTYARG )
{
    m_bSeriesNameChangePending = true;
    return 0;
}

void SeriesHeader::SetGetFocusHdl( const Link& rLink )
{
    m_spSeriesName->SetGetFocusHdl( rLink );
}

bool SeriesHeader::HasFocus() const
{
    return m_spSeriesName->HasFocus();
}

// static
Image SeriesHeader::GetChartTypeImage(
    const Reference< chart2::XChartType > & xChartType,
    bool bSwapXAndYAxis,
    bool bHC )
{
    Image aResult;
    if( !xChartType.is())
        return aResult;
    OUString aChartTypeName( xChartType->getChartType());

    if( aChartTypeName.equals( CHART2_SERVICE_NAME_CHARTTYPE_AREA ))
    {
        aResult = SELECT_IMAGE( IMG_TYPE_AREA, bHC );
    }
    else if( aChartTypeName.equals( CHART2_SERVICE_NAME_CHARTTYPE_COLUMN ))
    {
        if( bSwapXAndYAxis )
            aResult = SELECT_IMAGE( IMG_TYPE_BAR, bHC );
        else
            aResult = SELECT_IMAGE( IMG_TYPE_COLUMN, bHC );
    }
    else if( aChartTypeName.equals( CHART2_SERVICE_NAME_CHARTTYPE_LINE ))
    {
        aResult = SELECT_IMAGE( IMG_TYPE_LINE, bHC );
    }
    else if( aChartTypeName.equals( CHART2_SERVICE_NAME_CHARTTYPE_SCATTER ))
    {
        aResult = SELECT_IMAGE( IMG_TYPE_XY, bHC );
    }
    else if( aChartTypeName.equals( CHART2_SERVICE_NAME_CHARTTYPE_PIE ))
    {
        aResult = SELECT_IMAGE( IMG_TYPE_PIE, bHC );
    }
    else if( aChartTypeName.equals( CHART2_SERVICE_NAME_CHARTTYPE_NET )
          || aChartTypeName.equals( CHART2_SERVICE_NAME_CHARTTYPE_FILLED_NET ) )
    {
        aResult = SELECT_IMAGE( IMG_TYPE_NET, bHC );
    }
    else if( aChartTypeName.equals( CHART2_SERVICE_NAME_CHARTTYPE_CANDLESTICK ))
    {
        // @todo: correct image for candle-stick type
        aResult = SELECT_IMAGE( IMG_TYPE_STOCK, bHC );
    }
    else if( aChartTypeName.equals( CHART2_SERVICE_NAME_CHARTTYPE_BUBBLE ))
    {
        aResult = SELECT_IMAGE( IMG_TYPE_BUBBLE, bHC );
    }

    return aResult;
}

struct applyChangesFunctor : public ::std::unary_function< ::boost::shared_ptr< SeriesHeader >, void >
{
    void operator() ( ::boost::shared_ptr< SeriesHeader > spHeader )
    {
        spHeader->applyChanges();
    }
};

} // namespace impl
// ----------------------------------------

namespace
{

/** returns false, if no header as the focus.

    If a header has the focus, true is returned and the index of the header
    with focus is set at pIndex if pOutIndex is not 0.
*/
bool lcl_SeriesHeaderHasFocus(
    const ::std::vector< ::boost::shared_ptr< ::chart::impl::SeriesHeader > > & rSeriesHeader,
    sal_Int32 * pOutIndex = 0 )
{
    sal_Int32 nIndex = 0;
    for( ::std::vector< ::boost::shared_ptr< ::chart::impl::SeriesHeader > >::const_iterator aIt( rSeriesHeader.begin());
         aIt != rSeriesHeader.end(); ++aIt, ++nIndex )
    {
        if( (*aIt)->HasFocus())
        {
            if( pOutIndex )
                *pOutIndex = nIndex;
            return true;
        }
    }
    return false;
}

sal_Int32 lcl_getColumnInDataOrHeader(
    USHORT nCol, const ::std::vector< ::boost::shared_ptr< ::chart::impl::SeriesHeader > > & rSeriesHeader )
{
    sal_Int32 nColIdx = 0;
    bool bHeaderHasFocus( lcl_SeriesHeaderHasFocus( rSeriesHeader, &nColIdx ));

    if( bHeaderHasFocus )
        nColIdx = lcl_getColumnInData( static_cast< USHORT >( rSeriesHeader[nColIdx]->GetStartColumn()));
    else
        nColIdx = lcl_getColumnInData( nCol );

    return nColIdx;
}

} // anonymous namespace


DataBrowser::DataBrowser( Window* pParent, const ResId& rId, bool bLiveUpdate ) :
	::svt::EditBrowseBox( pParent, rId, EBBF_SMART_TAB_TRAVEL | EBBF_HANDLE_COLUMN_TEXT, BROWSER_STANDARD_FLAGS ),
	m_nSeekRow( 0 ),
    m_bIsReadOnly( false ),
    m_bIsDirty( false ),
    m_bLiveUpdate( bLiveUpdate ),
    m_bDataValid( true ),
    m_aNumberEditField( & EditBrowseBox::GetDataWindow(), WB_NOBORDER ),
    m_aTextEditField( & EditBrowseBox::GetDataWindow(), WB_NOBORDER ),
    m_rNumberEditController( new ::svt::FormattedFieldCellController( & m_aNumberEditField )),
    m_rTextEditController( new ::svt::EditCellController( & m_aTextEditField ))
{
    double fNan;
    ::rtl::math::setNan( & fNan );
    m_aNumberEditField.SetDefaultValue( fNan );
    m_aNumberEditField.TreatAsNumber( TRUE );
    RenewTable();
    SetClean();
}

DataBrowser::~DataBrowser()
{
}

bool DataBrowser::MayInsertRow() const
{
    return ! IsReadOnly()
        && ( !lcl_SeriesHeaderHasFocus( m_aSeriesHeaders ));
}

bool DataBrowser::MayInsertColumn() const
{
    return ! IsReadOnly();
}

bool DataBrowser::MayDeleteRow() const
{
    return ! IsReadOnly()
        && ( !lcl_SeriesHeaderHasFocus( m_aSeriesHeaders ))
        && ( GetCurRow() >= 0 )
        && ( GetRowCount() > 1 );
}

bool DataBrowser::MayDeleteColumn() const
{
    // if a series header has the focus
    if( lcl_SeriesHeaderHasFocus( m_aSeriesHeaders ))
        return true;

    return ! IsReadOnly()
        && ( GetCurColumnId() > 1 )
        && ( ColCount() > 2 );
}

bool DataBrowser::MaySwapRows() const
{
    return ! IsReadOnly()
        && ( !lcl_SeriesHeaderHasFocus( m_aSeriesHeaders ))
        && ( GetCurRow() >= 0 )
        && ( GetCurRow() < GetRowCount() - 1 );
}

bool DataBrowser::MaySwapColumns() const
{
    // if a series header (except the last one) has the focus
    {
        sal_Int32 nColIndex(0);
        if( lcl_SeriesHeaderHasFocus( m_aSeriesHeaders, &nColIndex ))
            return (static_cast< sal_uInt32 >( nColIndex ) < (m_aSeriesHeaders.size() - 1));
    }

    return ! IsReadOnly()
        && ( GetCurColumnId() > 1 )
        && ( GetCurColumnId() < ColCount() - 1 );
}

// bool DataBrowser::MaySortRow() const
// {
//     // not implemented
//     return false;
// //     return ! IsReadOnly() && ( GetCurRow() >= 0 );
// }

// bool DataBrowser::MaySortColumn() const
// {
//     // not implemented
//     return false;
// //     return ! IsReadOnly() && ( GetCurColumnId() > 1 );
// }

void DataBrowser::clearHeaders()
{
    ::std::for_each( m_aSeriesHeaders.begin(), m_aSeriesHeaders.end(), impl::applyChangesFunctor());
    m_aSeriesHeaders.clear();
}

void DataBrowser::RenewTable()
{
    if( ! m_apDataBrowserModel.get())
        return;

    long   nOldRow     = GetCurRow();
    USHORT nOldColId   = GetCurColumnId();

	BOOL bLastUpdateMode = GetUpdateMode();
	SetUpdateMode( FALSE );

    if( IsModified() )
        SaveModified();

    DeactivateCell();

    RemoveColumns();
    RowRemoved( 1, GetRowCount() );

    // for row numbers
	InsertHandleColumn( static_cast< sal_uInt16 >(
                            GetDataWindow().LogicToPixel( Size( 42, 0 )).getWidth() ));

    const sal_Int32 nDefaultColumnWidth = 94;

    sal_Int32 nColumnWidth( GetDataWindow().LogicToPixel( Size( nDefaultColumnWidth, 0 )).getWidth());
    sal_Int32 nColumnCount = m_apDataBrowserModel->getColumnCount();
    // nRowCount is a member of a base class
    sal_Int32 nRowCountLocal = m_apDataBrowserModel->getMaxRowCount();
    for( sal_Int32 nColIdx=1; nColIdx<=nColumnCount; ++nColIdx )
    {
        InsertDataColumn( static_cast< sal_uInt16 >( nColIdx ), GetColString( nColIdx ), nColumnWidth );
    }

    RowInserted( 1, nRowCountLocal );
    GoToRow( ::std::min( nOldRow, GetRowCount() - 1 ));
    GoToColumnId( ::std::min( nOldColId, static_cast< USHORT >( ColCount() - 1 )));

    Window * pWin = this->GetParent();
    if( !pWin )
        pWin = this;

    // fill series headers
    clearHeaders();
    const DataBrowserModel::tDataHeaderVector& aHeaders( m_apDataBrowserModel->getDataHeaders());
    Link aFocusLink( LINK( this, DataBrowser, SeriesHeaderGotFocus ));
    Link aSeriesHeaderChangedLink( LINK( this, DataBrowser, SeriesHeaderChanged ));
    bool bIsHighContrast = pWin ? (pWin->GetDisplayBackground().GetColor().IsDark()) : false;

    for( DataBrowserModel::tDataHeaderVector::const_iterator aIt( aHeaders.begin());
         aIt != aHeaders.end(); ++aIt )
    {
        ::boost::shared_ptr< impl::SeriesHeader > spHeader( new impl::SeriesHeader( pWin ));
        Reference< beans::XPropertySet > xSeriesProp( aIt->m_xDataSeries, uno::UNO_QUERY );
        sal_Int32 nColor = 0;
        // @todo: Set "DraftColor", i.e. interpolated colors for gradients, bitmaps, etc.
        if( xSeriesProp.is() &&
            ( xSeriesProp->getPropertyValue( OUString( RTL_CONSTASCII_USTRINGPARAM("Color"))) >>= nColor ))
            spHeader->SetColor( Color( nColor ));
        spHeader->SetChartType( aIt->m_xChartType, aIt->m_bSwapXAndYAxis, bIsHighContrast );
        spHeader->SetSeriesName(
            String( DataSeriesHelper::getDataSeriesLabel(
                        aIt->m_xDataSeries,
                        (aIt->m_xChartType.is() ?
                         aIt->m_xChartType->getRoleOfSequenceForSeriesLabel() :
                         OUString( RTL_CONSTASCII_USTRINGPARAM("values-y"))))));
        // index is 1-based, as 0 is for the column that contains the row-numbers
        spHeader->SetRange( aIt->m_nStartColumn + 1, aIt->m_nEndColumn + 1 );
        spHeader->SetGetFocusHdl( aFocusLink );
        spHeader->SetEditChangedHdl( aSeriesHeaderChangedLink );
        m_aSeriesHeaders.push_back( spHeader );
    }

    ImplAdjustHeaderControls();
    SetDirty();
	SetUpdateMode( bLastUpdateMode );
    ActivateCell();
    Invalidate();
}

String DataBrowser::GetColString( sal_Int32 nColumnId ) const
{
    OSL_ASSERT( m_apDataBrowserModel.get());
    if( nColumnId > 0 )
        return String( m_apDataBrowserModel->getRoleOfColumn( static_cast< sal_Int32 >( nColumnId ) - 1 ));
    return String();
}

String DataBrowser::GetRowString( sal_Int32 nRow ) const
{
	return String::CreateFromInt32( nRow + 1 );
}

String DataBrowser::GetCellText( long nRow, USHORT nColumnId ) const
{
	String aResult;

	if( nColumnId == 0 )
	{
		aResult = GetRowString( static_cast< sal_Int32 >( nRow ));
	}
	else if( nRow >= 0 &&
             m_apDataBrowserModel.get())
	{
        sal_Int32 nColIndex = static_cast< sal_Int32 >( nColumnId ) - 1;

        if( m_apDataBrowserModel->getCellType( nColIndex, nRow ) == DataBrowserModel::NUMBER )
        {
            double fData( m_apDataBrowserModel->getCellNumber( nColIndex, nRow ));
            sal_Int32 nLabelColor;
            bool bColorChanged = false;

            if( ! ::rtl::math::isNan( fData ) &&
                m_spNumberFormatterWrapper.get() )
                aResult = String( m_spNumberFormatterWrapper->getFormattedString(
                                      GetNumberFormatKey( nRow, nColumnId ),
                                      fData, nLabelColor, bColorChanged ));
        }
        else
        {
            OSL_ASSERT( m_apDataBrowserModel->getCellType( nColIndex, nRow ) == DataBrowserModel::TEXT );
            aResult = m_apDataBrowserModel->getCellText( nColIndex, nRow );
        }
    }

	return aResult;
}

double DataBrowser::GetCellNumber( long nRow, USHORT nColumnId ) const
{
    double fResult;
    ::rtl::math::setNan( & fResult );

    if(( nColumnId >= 1 ) && ( nRow >= 0 ) &&
        m_apDataBrowserModel.get())
	{
        fResult = m_apDataBrowserModel->getCellNumber(
            static_cast< sal_Int32 >( nColumnId ) - 1, nRow );
    }

	return fResult;
}

void DataBrowser::Resize()
{
    BOOL bLastUpdateMode = GetUpdateMode();
	SetUpdateMode( FALSE );

    ::svt::EditBrowseBox::Resize();
    ImplAdjustHeaderControls();
    SetUpdateMode( bLastUpdateMode );
}

bool DataBrowser::SetReadOnly( bool bNewState )
{
    bool bResult = m_bIsReadOnly;

    if( m_bIsReadOnly != bNewState )
    {
        m_bIsReadOnly = bNewState;
        Invalidate();
        DeactivateCell();
    }

    return bResult;
}

bool DataBrowser::IsReadOnly() const
{
    return m_bIsReadOnly;
}


void DataBrowser::SetClean()
{
    m_bIsDirty = false;
}

void DataBrowser::SetDirty()
{
    if( !m_bLiveUpdate )
        m_bIsDirty = true;
}

void DataBrowser::CursorMoved()
{
	EditBrowseBox::CursorMoved();

	if( GetUpdateMode() && m_aCursorMovedHdlLink.IsSet())
		m_aCursorMovedHdlLink.Call( this );
}

void DataBrowser::SetCellModifiedHdl( const Link& rLink )
{
    m_aCellModifiedLink = rLink;
}

void DataBrowser::MouseButtonDown( const BrowserMouseEvent& rEvt )
{
    if( !m_bDataValid )
        ShowWarningBox();
    else
        EditBrowseBox::MouseButtonDown( rEvt );
}

void DataBrowser::ShowWarningBox()
{
    WarningBox( this, WinBits( WB_OK ),
                String( SchResId( STR_INVALID_NUMBER ))).Execute();
}

bool DataBrowser::ShowQueryBox()
{
    QueryBox* pQueryBox = new QueryBox( this, WB_YES_NO, String( SchResId( STR_DATA_EDITOR_INCORRECT_INPUT )));

    return ( pQueryBox->Execute() == RET_YES );
}

bool DataBrowser::IsDataValid()
{
    bool bValid = true;
    const sal_Int32 nRow = lcl_getRowInData( GetCurRow());
    const sal_Int32 nCol = lcl_getColumnInData( GetCurColumnId());

    if( m_apDataBrowserModel->getCellType( nCol, nRow ) == DataBrowserModel::NUMBER )
    {
        sal_uInt32 nDummy = 0;
        double fDummy = 0.0;
        String aText( m_aNumberEditField.GetText());

        if( aText.Len() > 0 &&
            m_spNumberFormatterWrapper.get() &&
            m_spNumberFormatterWrapper->getSvNumberFormatter() &&
            ! m_spNumberFormatterWrapper->getSvNumberFormatter()->IsNumberFormat(
              aText, nDummy, fDummy ))
        {
            bValid = false;
        }
    }

    return bValid;
}

bool DataBrowser::IsEnableItem()
{
    return m_bDataValid;
}

void DataBrowser::CellModified()
{
    m_bDataValid = IsDataValid();
    SetDirty();
    if( m_aCellModifiedLink.IsSet())
        m_aCursorMovedHdlLink.Call( this );
}

void DataBrowser::SetDataFromModel(
    const Reference< chart2::XChartDocument > & xChartDoc,
    const Reference< uno::XComponentContext > & xContext )
{
    if( m_bLiveUpdate )
    {
        m_xChartDoc.set( xChartDoc );
    }
    else
    {
        Reference< util::XCloneable > xCloneable( xChartDoc, uno::UNO_QUERY );
        if( xCloneable.is())
            m_xChartDoc.set( xCloneable->createClone(), uno::UNO_QUERY );
    }

    m_apDataBrowserModel.reset( new DataBrowserModel( m_xChartDoc, xContext ));
    m_spNumberFormatterWrapper.reset(
        new NumberFormatterWrapper(
            Reference< util::XNumberFormatsSupplier >( m_xChartDoc, uno::UNO_QUERY )));

    RenewTable();

    const sal_Int32 nColCnt  = m_apDataBrowserModel->getColumnCount();
    const sal_Int32 nRowCnt =  m_apDataBrowserModel->getMaxRowCount();
    if( nRowCnt && nColCnt )
    {
        GoToRow( 0 );
        GoToColumnId( 1 );
    }
    SetClean();
}

void DataBrowser::InsertColumn()
{
    sal_Int32 nColIdx = lcl_getColumnInDataOrHeader( GetCurColumnId(), m_aSeriesHeaders );

	if( nColIdx >= 0 &&
        m_apDataBrowserModel.get())
	{
        // save changes made to edit-field
        if( IsModified() )
            SaveModified();

        m_apDataBrowserModel->insertDataSeries( nColIdx );
		RenewTable();
	}
}

void DataBrowser::RemoveColumn()
{
    sal_Int32 nColIdx = lcl_getColumnInDataOrHeader( GetCurColumnId(), m_aSeriesHeaders );

	if( nColIdx >= 0 &&
        m_apDataBrowserModel.get())
	{
        // save changes made to edit-field
        if( IsModified() )
            SaveModified();

        m_bDataValid = true;
        m_apDataBrowserModel->removeDataSeries( nColIdx );
        RenewTable();
	}
}

void DataBrowser::InsertRow()
{
 	sal_Int32 nRowIdx = lcl_getRowInData( GetCurRow());

 	if( nRowIdx >= 0 &&
        m_apDataBrowserModel.get())
    {
        // save changes made to edit-field
        if( IsModified() )
            SaveModified();

        m_apDataBrowserModel->insertDataPointForAllSeries( nRowIdx );
		RenewTable();
	}
}

void DataBrowser::RemoveRow()
{
 	sal_Int32 nRowIdx = lcl_getRowInData( GetCurRow());

 	if( nRowIdx >= 0 &&
        m_apDataBrowserModel.get())
    {
        // save changes made to edit-field
        if( IsModified() )
            SaveModified();

        m_bDataValid = true;
        m_apDataBrowserModel->removeDataPointForAllSeries( nRowIdx );
		RenewTable();
	}
}

void DataBrowser::SwapColumn()
{
    sal_Int32 nColIdx = lcl_getColumnInDataOrHeader( GetCurColumnId(), m_aSeriesHeaders );

	if( nColIdx >= 0 &&
        m_apDataBrowserModel.get())
	{
        // save changes made to edit-field
        if( IsModified() )
            SaveModified();

        m_apDataBrowserModel->swapDataSeries( nColIdx );

        // keep cursor in swapped column
        if( GetCurColumnId() < ColCount() - 1 )
		{
            Dispatch( BROWSER_CURSORRIGHT );
		}
        RenewTable();
	}
}

void DataBrowser::SwapRow()
{
 	sal_Int32 nRowIdx = lcl_getRowInData( GetCurRow());

 	if( nRowIdx >= 0 &&
        m_apDataBrowserModel.get())
    {
        // save changes made to edit-field
        if( IsModified() )
            SaveModified();

        m_apDataBrowserModel->swapDataPointForAllSeries( nRowIdx );

        // keep cursor in swapped row
        if( GetCurRow() < GetRowCount() - 1 )
		{
            Dispatch( BROWSER_CURSORDOWN );
		}
        RenewTable();
	}
}

void DataBrowser::SetCursorMovedHdl( const Link& rLink )
{
    m_aCursorMovedHdlLink = rLink;
}

// implementations for ::svt::EditBrowseBox (pure virtual methods)
void DataBrowser::PaintCell(
    OutputDevice& rDev, const Rectangle& rRect, sal_uInt16 nColumnId ) const
{
    Point aPos( rRect.TopLeft());
    aPos.X() += 1;

    String aText = GetCellText( m_nSeekRow, nColumnId );
    Size TxtSize( GetDataWindow().GetTextWidth( aText ), GetDataWindow().GetTextHeight());

    // clipping
    if( aPos.X() < rRect.Right() || aPos.X() + TxtSize.Width() > rRect.Right() ||
        aPos.Y() < rRect.Top() || aPos.Y() + TxtSize.Height() > rRect.Bottom())
        rDev.SetClipRegion( rRect );

    // allow for a disabled control ...
    sal_Bool bEnabled = IsEnabled();
    Color aOriginalColor = rDev.GetTextColor();
    if( ! bEnabled )
        rDev.SetTextColor( GetSettings().GetStyleSettings().GetDisableColor() );

    // TEST
//     if( nColumnId == 1 )
//         // categories
//         rDev.SetFillColor( Color( 0xff, 0xff, 0xff ));
//     else if( nColumnId == 2 )
//         // x-values
//         rDev.SetFillColor( Color( 0xf0, 0xf0, 0xff ));
//     else
//         // y-values
//         rDev.SetFillColor( Color( 0xff, 0xff, 0xf0 ));

//     rDev.DrawRect( rRect );

    // draw the text
    rDev.DrawText( aPos, aText );

    // reset the color (if necessary)
    if( ! bEnabled )
        rDev.SetTextColor( aOriginalColor );

    if( rDev.IsClipRegion())
        rDev.SetClipRegion();
}

sal_Bool DataBrowser::SeekRow( long nRow )
{
    if( ! EditBrowseBox::SeekRow( nRow ))
        return sal_False;

    if( nRow < 0 )
        m_nSeekRow = - 1;
    else
        m_nSeekRow = nRow;

    return sal_True;
}

sal_Bool DataBrowser::IsTabAllowed( sal_Bool bForward ) const
{
    long nRow = GetCurRow();
    long nCol = GetCurColumnId();

    // column 0 is header-column
    long nBadCol = bForward
        ? GetColumnCount() - 1
        : 1;
    long nBadRow = bForward
        ? GetRowCount() - 1
        : 0;

    if( !m_bDataValid )
    {
        const_cast< DataBrowser* >( this )->ShowWarningBox();
        return sal_False;
    }

    return ( nRow != nBadRow ||
             nCol != nBadCol );
}

::svt::CellController* DataBrowser::GetController( long nRow, sal_uInt16 nCol )
{
    if( m_bIsReadOnly )
        return 0;

    if( CellContainsNumbers( nRow, nCol ))
    {
        m_aNumberEditField.UseInputStringForFormatting();
        m_aNumberEditField.SetFormatKey( GetNumberFormatKey( nRow, nCol ));
        return m_rNumberEditController;
    }

    return m_rTextEditController;
}

void DataBrowser::InitController(
    ::svt::CellControllerRef& rController, long nRow, sal_uInt16 nCol )
{
    if( rController == m_rTextEditController )
    {
        String aText( GetCellText( nRow, nCol ) );
        m_aTextEditField.SetText( aText );
        m_aTextEditField.SetSelection( Selection( 0, aText.Len() ));
    }
    else if( rController == m_rNumberEditController )
    {
        // treat invalid and empty text as Nan
        m_aNumberEditField.EnableNotANumber( true );
        if( ::rtl::math::isNan( GetCellNumber( nRow, nCol )))
            m_aNumberEditField.SetTextValue( String());
        else
            m_aNumberEditField.SetValue( GetCellNumber( nRow, nCol ) );
        String aText( m_aNumberEditField.GetText());
        m_aNumberEditField.SetSelection( Selection( 0, aText.Len()));
    }
    else
    {
        DBG_ERROR( "Invalid Controller" );
    }
}

bool DataBrowser::CellContainsNumbers( sal_Int32 nRow, sal_uInt16 nCol ) const
{
    if( ! m_apDataBrowserModel.get())
        return false;
    return (m_apDataBrowserModel->getCellType( lcl_getColumnInData( nCol ), lcl_getRowInData( nRow )) ==
            DataBrowserModel::NUMBER);
}

sal_uInt32 DataBrowser::GetNumberFormatKey( sal_Int32 nRow, sal_uInt16 nCol ) const
{
    if( ! m_apDataBrowserModel.get())
        return 0;
    return m_apDataBrowserModel->getNumberFormatKey( lcl_getColumnInData( nCol ), lcl_getRowInData( nRow ));
}

sal_Bool DataBrowser::SaveModified()
{
    if( ! IsModified() )
        return sal_True;

    sal_Bool bChangeValid = sal_True;

    const sal_Int32 nRow = lcl_getRowInData( GetCurRow());
    const sal_Int32 nCol = lcl_getColumnInData( GetCurColumnId());

    DBG_ASSERT( nRow >= 0 || nCol >= 0, "This cell should not be modified!" );

    switch( m_apDataBrowserModel->getCellType( nCol, nRow ))
    {
        case DataBrowserModel::NUMBER:
        {
            sal_uInt32 nDummy = 0;
            double fDummy = 0.0;
            String aText( m_aNumberEditField.GetText());
            // an empty string is valid, if no numberformatter exists, all
            // values are treated as valid
            if( aText.Len() > 0 &&
                m_spNumberFormatterWrapper.get() &&
                m_spNumberFormatterWrapper->getSvNumberFormatter() &&
                ! m_spNumberFormatterWrapper->getSvNumberFormatter()->IsNumberFormat(
                    aText, nDummy, fDummy ))
            {
                bChangeValid = sal_False;
            }
            else
            {
                double fData = m_aNumberEditField.GetValue();
                bChangeValid = m_apDataBrowserModel->setCellNumber( nCol, nRow, fData );
            }
        }
        break;
        case DataBrowserModel::TEXT:
        {
            OUString aText( m_aTextEditField.GetText());
            bChangeValid = m_apDataBrowserModel->setCellText( nCol, nRow, aText );
        }
        break;
    }

    // the first valid change changes this to true
    if( bChangeValid )
    {
        RowModified( GetCurRow(), GetCurColumnId());
        ::svt::CellController* pCtrl = GetController( GetCurRow(), GetCurColumnId());
        if( pCtrl )
            pCtrl->ClearModified();
        SetDirty();
    }

    return bChangeValid;
}

bool DataBrowser::EndEditing()
{
    if( IsModified())
        SaveModified();

    // apply changes made to series headers
    ::std::for_each( m_aSeriesHeaders.begin(), m_aSeriesHeaders.end(), impl::applyChangesFunctor());

    if( m_bDataValid )
        return true;
    else
        return ShowQueryBox();
}

sal_Int16 DataBrowser::GetFirstVisibleColumNumber() const
{
    return GetFirstVisibleColNumber();
}

void DataBrowser::ColumnResized( USHORT nColId )
{
    BOOL bLastUpdateMode = GetUpdateMode();
	SetUpdateMode( FALSE );

    EditBrowseBox::ColumnResized( nColId );
    ImplAdjustHeaderControls();
    SetUpdateMode( bLastUpdateMode );
}

// 	virtual void    MouseMove( const MouseEvent& rEvt );

void DataBrowser::EndScroll()
{
    BOOL bLastUpdateMode = GetUpdateMode();
	SetUpdateMode( FALSE );

    EditBrowseBox::EndScroll();
    RenewSeriesHeaders();

    SetUpdateMode( bLastUpdateMode );
}

void DataBrowser::RenewSeriesHeaders()
{
    Window * pWin = this->GetParent();
    if( !pWin )
        pWin = this;

    clearHeaders();
    DataBrowserModel::tDataHeaderVector aHeaders( m_apDataBrowserModel->getDataHeaders());
    Link aFocusLink( LINK( this, DataBrowser, SeriesHeaderGotFocus ));
    Link aSeriesHeaderChangedLink( LINK( this, DataBrowser, SeriesHeaderChanged ));
    bool bIsHighContrast = pWin ? (pWin->GetDisplayBackground().GetColor().IsDark()) : false;

    for( DataBrowserModel::tDataHeaderVector::const_iterator aIt( aHeaders.begin());
         aIt != aHeaders.end(); ++aIt )
    {
        ::boost::shared_ptr< impl::SeriesHeader > spHeader( new impl::SeriesHeader( pWin ));
        Reference< beans::XPropertySet > xSeriesProp( aIt->m_xDataSeries, uno::UNO_QUERY );
        sal_Int32 nColor = 0;
        if( xSeriesProp.is() &&
            ( xSeriesProp->getPropertyValue( OUString( RTL_CONSTASCII_USTRINGPARAM("Color"))) >>= nColor ))
            spHeader->SetColor( Color( nColor ));
        spHeader->SetChartType( aIt->m_xChartType, aIt->m_bSwapXAndYAxis, bIsHighContrast );
        spHeader->SetSeriesName(
            String( DataSeriesHelper::getDataSeriesLabel(
                        aIt->m_xDataSeries,
                        (aIt->m_xChartType.is() ?
                         aIt->m_xChartType->getRoleOfSequenceForSeriesLabel() :
                         OUString( RTL_CONSTASCII_USTRINGPARAM("values-y"))))));
        spHeader->SetRange( aIt->m_nStartColumn + 1, aIt->m_nEndColumn + 1 );
        spHeader->SetGetFocusHdl( aFocusLink );
        spHeader->SetEditChangedHdl( aSeriesHeaderChangedLink );
        m_aSeriesHeaders.push_back( spHeader );
    }

    ImplAdjustHeaderControls();
}

void DataBrowser::ImplAdjustHeaderControls()
{
    sal_uInt16 nColCount = this->GetColumnCount();
    sal_uInt32 nCurrentPos = this->GetPosPixel().getX();
    sal_uInt32 nMaxPos = nCurrentPos + this->GetOutputSizePixel().getWidth();
    sal_uInt32 nStartPos = nCurrentPos;

    // width of header column
    nCurrentPos +=  this->GetColumnWidth( 0 );
    tSeriesHeaderContainer::iterator aIt( m_aSeriesHeaders.begin());
    sal_uInt16 i = this->GetFirstVisibleColumNumber();
    while( (aIt != m_aSeriesHeaders.end()) && ((*aIt)->GetStartColumn() < i) )
        ++aIt;
    for( ; i < nColCount && aIt != m_aSeriesHeaders.end(); ++i )
    {
        if( (*aIt)->GetStartColumn() == i )
            nStartPos = nCurrentPos;

        nCurrentPos += (this->GetColumnWidth( i ));

        if( (*aIt)->GetEndColumn() == i )
        {
            if( nStartPos < nMaxPos )
            {
                (*aIt)->SetPixelPosX( nStartPos + 2 );
                (*aIt)->SetPixelWidth( nCurrentPos - nStartPos - 3 );
            }
            else
                // do not hide, to get focus events. Move outside the dialog for "hiding"
                (*aIt)->SetPixelPosX( nMaxPos + 42 );
            ++aIt;
        }
    }
}

IMPL_LINK( DataBrowser, SeriesHeaderGotFocus, impl::SeriesHeaderEdit*, pEdit )
{
    if( pEdit )
    {
        pEdit->SetShowWarningBox( !m_bDataValid );

        if( !m_bDataValid )
            GoToCell( 0, 0 );
        else
        {
            //DeactivateCell();
            MakeFieldVisible( GetCurRow(), static_cast< sal_uInt16 >( pEdit->getStartColumn()), true /* bComplete */ );
            ActivateCell();
            m_aCursorMovedHdlLink.Call( this );
        }
    }
    return 0;
}

IMPL_LINK( DataBrowser, SeriesHeaderChanged, impl::SeriesHeaderEdit*, pEdit )
{
    if( pEdit )
    {
        Reference< chart2::XDataSeries > xSeries(
            m_apDataBrowserModel->getDataSeriesByColumn( pEdit->getStartColumn() - 1 ));
        Reference< chart2::data::XDataSource > xSource( xSeries, uno::UNO_QUERY );
        if( xSource.is())
        {
            Reference< chart2::XChartType > xChartType(
                m_apDataBrowserModel->getHeaderForSeries( xSeries ).m_xChartType );
            if( xChartType.is())
            {
                Reference< chart2::data::XLabeledDataSequence > xLabeledSeq(
                    DataSeriesHelper::getDataSequenceByRole( xSource, xChartType->getRoleOfSequenceForSeriesLabel()));
                if( xLabeledSeq.is())
                {
                    Reference< container::XIndexReplace > xIndexReplace( xLabeledSeq->getLabel(), uno::UNO_QUERY );
                    if( xIndexReplace.is())
                        xIndexReplace->replaceByIndex(
                            0, uno::makeAny( OUString( pEdit->GetText())));
                }
            }
        }
    }
    return 0;
}

sal_Int32 DataBrowser::GetTotalWidth() const
{
    ULONG nWidth = 0;
	for ( USHORT nCol = 0; nCol < ColCount(); ++nCol )
		nWidth += GetColumnWidth( nCol );
	return static_cast< sal_Int32 >( nWidth );
}

} // namespace chart
