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
#include "ChartDocumentWrapper.hxx"
#include "macros.hxx"
#include "servicenames.hxx"
#include "PropertyHelper.hxx"
#include "TitleHelper.hxx"
#include "LegendHelper.hxx"
#include "ControllerLockGuard.hxx"
#include "ModifyListenerHelper.hxx"
#include "DisposeHelper.hxx"
#include "DataSeriesPointWrapper.hxx"
#include "chartview/ExplicitValueProvider.hxx"
#include "chartview/DrawModelWrapper.hxx"
#include "Chart2ModelContact.hxx"

#include "DiagramHelper.hxx"
#include "DataSourceHelper.hxx"
#include "ChartModelHelper.hxx"
#include "ContainerHelper.hxx"

#include "TitleWrapper.hxx"
#include "ChartDataWrapper.hxx"
#include "DiagramWrapper.hxx"
#include "LegendWrapper.hxx"
#include "AreaWrapper.hxx"
#include "WrappedAddInProperty.hxx"
#include "WrappedIgnoreProperty.hxx"
#include "ChartRenderer.hxx"
#include <com/sun/star/chart2/XTitled.hpp>
#include <com/sun/star/chart2/data/XDataReceiver.hpp>
#include <com/sun/star/chart/ChartDataRowSource.hpp>
#include <com/sun/star/chart/XChartDataArray.hpp>
#include <comphelper/InlineContainer.hxx>
// header for function SvxShapeCollection_NewInstance
#include <svx/unoshcol.hxx>
// header for define DBG_ASSERT
#include <tools/debug.hxx>
#include <com/sun/star/drawing/XDrawPagesSupplier.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/util/DateTime.hpp>

#include <vector>
#include <algorithm>
#include <functional>

using namespace ::com::sun::star;
using namespace ::com::sun::star::chart;

using ::com::sun::star::uno::Any;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::beans::Property;
using ::osl::MutexGuard;
using ::rtl::OUString;

namespace
{
enum eServiceType
{
    SERVICE_NAME_AREA_DIAGRAM = 0,
    SERVICE_NAME_BAR_DIAGRAM,
    SERVICE_NAME_DONUT_DIAGRAM,
    SERVICE_NAME_LINE_DIAGRAM,
    SERVICE_NAME_NET_DIAGRAM,
    SERVICE_NAME_FILLED_NET_DIAGRAM,
    SERVICE_NAME_PIE_DIAGRAM,
    SERVICE_NAME_STOCK_DIAGRAM,
    SERVICE_NAME_XY_DIAGRAM,
    SERVICE_NAME_BUBBLE_DIAGRAM,

    SERVICE_NAME_DASH_TABLE,
    SERVICE_NAME_GARDIENT_TABLE,
    SERVICE_NAME_HATCH_TABLE,
    SERVICE_NAME_BITMAP_TABLE,
    SERVICE_NAME_TRANSP_GRADIENT_TABLE,
    SERVICE_NAME_MARKER_TABLE,

    SERVICE_NAME_NAMESPACE_MAP,
    SERVICE_NAME_EXPORT_GRAPHIC_RESOLVER,
    SERVICE_NAME_IMPORT_GRAPHIC_RESOLVER
};

typedef ::std::map< ::rtl::OUString, enum eServiceType > tServiceNameMap;
typedef ::comphelper::MakeMap< ::rtl::OUString, enum eServiceType > tMakeServiceNameMap;

tServiceNameMap & lcl_getStaticServiceNameMap()
{
    static tServiceNameMap aServiceNameMap(
        tMakeServiceNameMap
        ( C2U( "com.sun.star.chart.AreaDiagram" ),                    SERVICE_NAME_AREA_DIAGRAM )
        ( C2U( "com.sun.star.chart.BarDiagram" ),                     SERVICE_NAME_BAR_DIAGRAM )
        ( C2U( "com.sun.star.chart.DonutDiagram" ),                   SERVICE_NAME_DONUT_DIAGRAM )
        ( C2U( "com.sun.star.chart.LineDiagram" ),                    SERVICE_NAME_LINE_DIAGRAM )
        ( C2U( "com.sun.star.chart.NetDiagram" ),                     SERVICE_NAME_NET_DIAGRAM )
        ( C2U( "com.sun.star.chart.FilledNetDiagram" ),               SERVICE_NAME_FILLED_NET_DIAGRAM )
        ( C2U( "com.sun.star.chart.PieDiagram" ),                     SERVICE_NAME_PIE_DIAGRAM )
        ( C2U( "com.sun.star.chart.StockDiagram" ),                   SERVICE_NAME_STOCK_DIAGRAM )
        ( C2U( "com.sun.star.chart.XYDiagram" ),                      SERVICE_NAME_XY_DIAGRAM )
        ( C2U( "com.sun.star.chart.BubbleDiagram" ),                  SERVICE_NAME_BUBBLE_DIAGRAM )

        ( C2U( "com.sun.star.drawing.DashTable" ),                    SERVICE_NAME_DASH_TABLE )
        ( C2U( "com.sun.star.drawing.GradientTable" ),                SERVICE_NAME_GARDIENT_TABLE )
        ( C2U( "com.sun.star.drawing.HatchTable" ),                   SERVICE_NAME_HATCH_TABLE )
        ( C2U( "com.sun.star.drawing.BitmapTable" ),                  SERVICE_NAME_BITMAP_TABLE )
        ( C2U( "com.sun.star.drawing.TransparencyGradientTable" ),    SERVICE_NAME_TRANSP_GRADIENT_TABLE )
        ( C2U( "com.sun.star.drawing.MarkerTable" ),                  SERVICE_NAME_MARKER_TABLE )

        ( C2U( "com.sun.star.xml.NamespaceMap" ),                     SERVICE_NAME_NAMESPACE_MAP )
        ( C2U( "com.sun.star.document.ExportGraphicObjectResolver" ), SERVICE_NAME_EXPORT_GRAPHIC_RESOLVER )
        ( C2U( "com.sun.star.document.ImportGraphicObjectResolver" ), SERVICE_NAME_IMPORT_GRAPHIC_RESOLVER )
        );

    return aServiceNameMap;
}

enum
{
    PROP_DOCUMENT_HAS_MAIN_TITLE,
    PROP_DOCUMENT_HAS_SUB_TITLE,
    PROP_DOCUMENT_HAS_LEGEND,
    PROP_DOCUMENT_LABELS_IN_FIRST_ROW,
    PROP_DOCUMENT_LABELS_IN_FIRST_COLUMN,
    PROP_DOCUMENT_ADDIN,
    PROP_DOCUMENT_BASEDIAGRAM,
    PROP_DOCUMENT_ADDITIONAL_SHAPES,
    PROP_DOCUMENT_UPDATE_ADDIN,
    PROP_DOCUMENT_NULL_DATE
};

void lcl_AddPropertiesToVector(
    ::std::vector< Property > & rOutProperties )
{
    rOutProperties.push_back(
        Property( C2U( "HasMainTitle" ),
                  PROP_DOCUMENT_HAS_MAIN_TITLE,
                  ::getBooleanCppuType(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT ));
    rOutProperties.push_back(
        Property( C2U( "HasSubTitle" ),
                  PROP_DOCUMENT_HAS_SUB_TITLE,
                  ::getBooleanCppuType(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT ));
    rOutProperties.push_back(
        Property( C2U( "HasLegend" ),
                  PROP_DOCUMENT_HAS_LEGEND,
                  ::getBooleanCppuType(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT ));

    // really needed?
    rOutProperties.push_back(
        Property( C2U( "DataSourceLabelsInFirstRow" ),
                  PROP_DOCUMENT_LABELS_IN_FIRST_ROW,
                  ::getBooleanCppuType(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT ));
    rOutProperties.push_back(
        Property( C2U( "DataSourceLabelsInFirstColumn" ),
                  PROP_DOCUMENT_LABELS_IN_FIRST_COLUMN,
                  ::getBooleanCppuType(),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT ));

    //add-in
    rOutProperties.push_back(
        Property( C2U( "AddIn" ),
                  PROP_DOCUMENT_ADDIN,
                  ::getCppuType( reinterpret_cast< Reference< util::XRefreshable > * >(0)),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID ));
    rOutProperties.push_back(
        Property( C2U( "BaseDiagram" ),
                  PROP_DOCUMENT_BASEDIAGRAM,
                  ::getCppuType( reinterpret_cast< const ::rtl::OUString * >(0)),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID ));
    rOutProperties.push_back(
        Property( C2U( "AdditionalShapes" ),
                  PROP_DOCUMENT_ADDITIONAL_SHAPES,
                  ::getCppuType( reinterpret_cast< Reference< drawing::XShapes > * >(0)),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEVOID
                  | beans::PropertyAttribute::READONLY ));
    rOutProperties.push_back(
        Property( C2U( "RefreshAddInAllowed" ),
                  PROP_DOCUMENT_UPDATE_ADDIN,
                  ::getBooleanCppuType(),
                  beans::PropertyAttribute::BOUND ));

    // table:null-date // i99104
    rOutProperties.push_back(
        Property( C2U( "NullDate" ),
                  PROP_DOCUMENT_NULL_DATE,
                  ::getCppuType( static_cast< const ::com::sun::star::util::DateTime * >(0)),
                  beans::PropertyAttribute::MAYBEVOID ));
}

const uno::Sequence< Property > & lcl_GetPropertySequence()
{
    static uno::Sequence< Property > aPropSeq;

    // /--
    MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
    if( 0 == aPropSeq.getLength() )
    {
        // get properties
        ::std::vector< ::com::sun::star::beans::Property > aProperties;
        lcl_AddPropertiesToVector( aProperties );

        // and sort them for access via bsearch
        ::std::sort( aProperties.begin(), aProperties.end(),
                     ::chart::PropertyNameLess() );

        // transfer result to static Sequence
        aPropSeq = ::chart::ContainerHelper::ContainerToSequence( aProperties );
    }

    return aPropSeq;
}

} //  anonymous namespace

namespace chart
{
namespace wrapper
{
//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------

//PROP_DOCUMENT_LABELS_IN_FIRST_ROW
class WrappedDataSourceLabelsInFirstRowProperty : public WrappedProperty
{
public:
    WrappedDataSourceLabelsInFirstRowProperty( ::boost::shared_ptr< Chart2ModelContact > spChart2ModelContact );
    virtual ~WrappedDataSourceLabelsInFirstRowProperty();

    virtual void setPropertyValue( const ::com::sun::star::uno::Any& rOuterValue, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& xInnerPropertySet ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::uno::Any getPropertyValue( const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& xInnerPropertySet ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::uno::Any getPropertyDefault( const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertyState >& xInnerPropertyState ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

private: //member
    ::boost::shared_ptr< Chart2ModelContact >   m_spChart2ModelContact;
    mutable Any                                 m_aOuterValue;
};

WrappedDataSourceLabelsInFirstRowProperty::WrappedDataSourceLabelsInFirstRowProperty( ::boost::shared_ptr< Chart2ModelContact > spChart2ModelContact )
            : WrappedProperty(C2U("DataSourceLabelsInFirstRow"),OUString())
            , m_spChart2ModelContact( spChart2ModelContact )
            , m_aOuterValue()
{
    m_aOuterValue = WrappedDataSourceLabelsInFirstRowProperty::getPropertyDefault( 0 );
}

WrappedDataSourceLabelsInFirstRowProperty::~WrappedDataSourceLabelsInFirstRowProperty()
{
}

void WrappedDataSourceLabelsInFirstRowProperty::setPropertyValue( const Any& rOuterValue, const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
                throw (beans::UnknownPropertyException, beans::PropertyVetoException, lang::IllegalArgumentException, lang::WrappedTargetException, uno::RuntimeException)
{
    sal_Bool bLabelsInFirstRow = sal_True;
    if( ! (rOuterValue >>= bLabelsInFirstRow) )
        throw lang::IllegalArgumentException( C2U("Property DataSourceLabelsInFirstRow requires value of type boolean"), 0, 0 );

    m_aOuterValue = rOuterValue;
    bool bNewValue = bLabelsInFirstRow;

    ::rtl::OUString aRangeString;
    bool bUseColumns = true;
    bool bFirstCellAsLabel = true;
    bool bHasCategories = true;
    uno::Sequence< sal_Int32 > aSequenceMapping;

    if( DataSourceHelper::detectRangeSegmentation(
            m_spChart2ModelContact->getChartModel(), aRangeString, aSequenceMapping, bUseColumns
            , bFirstCellAsLabel, bHasCategories ) )
    {
        if( bUseColumns && bNewValue != bFirstCellAsLabel )
        {
            DataSourceHelper::setRangeSegmentation(
                m_spChart2ModelContact->getChartModel(), aSequenceMapping, bUseColumns ,bNewValue, bHasCategories );
        }
        else if( !bUseColumns && bNewValue != bHasCategories )
        {
            DataSourceHelper::setRangeSegmentation(
                m_spChart2ModelContact->getChartModel(), aSequenceMapping, bUseColumns , bFirstCellAsLabel, bNewValue );
        }
    }
}

Any WrappedDataSourceLabelsInFirstRowProperty::getPropertyValue( const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
                        throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
{
    ::rtl::OUString aRangeString;
    bool bUseColumns = true;
    bool bFirstCellAsLabel = true;
    bool bHasCategories = true;
    uno::Sequence< sal_Int32 > aSequenceMapping;

    if( DataSourceHelper::detectRangeSegmentation(
            m_spChart2ModelContact->getChartModel(), aRangeString, aSequenceMapping, bUseColumns
            , bFirstCellAsLabel, bHasCategories ) )
    {
        sal_Bool bLabelsInFirstRow = sal_True;
        if( bUseColumns )
            bLabelsInFirstRow = bFirstCellAsLabel;
        else
            bLabelsInFirstRow = bHasCategories;

        m_aOuterValue <<= bLabelsInFirstRow;
    }
    return m_aOuterValue;
}

Any WrappedDataSourceLabelsInFirstRowProperty::getPropertyDefault( const Reference< beans::XPropertyState >& /*xInnerPropertyState*/ ) const
                        throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
{
    Any aRet;
    aRet <<= sal_True;
    return aRet;
}

//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------

//PROP_DOCUMENT_LABELS_IN_FIRST_COLUMN
class WrappedDataSourceLabelsInFirstColumnProperty : public WrappedProperty
{
public:
    WrappedDataSourceLabelsInFirstColumnProperty( ::boost::shared_ptr< Chart2ModelContact > spChart2ModelContact );
    virtual ~WrappedDataSourceLabelsInFirstColumnProperty();

    virtual void setPropertyValue( const ::com::sun::star::uno::Any& rOuterValue, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& xInnerPropertySet ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::uno::Any getPropertyValue( const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& xInnerPropertySet ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::uno::Any getPropertyDefault( const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertyState >& xInnerPropertyState ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

private: //member
    ::boost::shared_ptr< Chart2ModelContact >   m_spChart2ModelContact;
    mutable Any                                 m_aOuterValue;
};

WrappedDataSourceLabelsInFirstColumnProperty::WrappedDataSourceLabelsInFirstColumnProperty( ::boost::shared_ptr< Chart2ModelContact > spChart2ModelContact )
            : WrappedProperty(C2U("DataSourceLabelsInFirstColumn"),OUString())
            , m_spChart2ModelContact( spChart2ModelContact )
            , m_aOuterValue()
{
    m_aOuterValue = WrappedDataSourceLabelsInFirstColumnProperty::getPropertyDefault( 0 );
}

WrappedDataSourceLabelsInFirstColumnProperty::~WrappedDataSourceLabelsInFirstColumnProperty()
{
}

void WrappedDataSourceLabelsInFirstColumnProperty::setPropertyValue( const Any& rOuterValue, const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
                throw (beans::UnknownPropertyException, beans::PropertyVetoException, lang::IllegalArgumentException, lang::WrappedTargetException, uno::RuntimeException)
{
    sal_Bool bLabelsInFirstRow = sal_True;
    if( ! (rOuterValue >>= bLabelsInFirstRow) )
        throw lang::IllegalArgumentException( C2U("Property DataSourceLabelsInFirstRow requires value of type boolean"), 0, 0 );

    m_aOuterValue = rOuterValue;
    bool bNewValue = bLabelsInFirstRow;

    ::rtl::OUString aRangeString;
    bool bUseColumns = true;
    bool bFirstCellAsLabel = true;
    bool bHasCategories = true;
    uno::Sequence< sal_Int32 > aSequenceMapping;

    if( DataSourceHelper::detectRangeSegmentation(
            m_spChart2ModelContact->getChartModel(), aRangeString, aSequenceMapping, bUseColumns
            , bFirstCellAsLabel, bHasCategories ) )
    {
        if( bUseColumns && bNewValue != bHasCategories )
        {
            DataSourceHelper::setRangeSegmentation(
                m_spChart2ModelContact->getChartModel(), aSequenceMapping, bUseColumns, bFirstCellAsLabel, bNewValue );
        }
        else if( !bUseColumns && bNewValue != bFirstCellAsLabel )
        {
            DataSourceHelper::setRangeSegmentation(
                m_spChart2ModelContact->getChartModel(), aSequenceMapping, bUseColumns , bNewValue, bHasCategories );
        }
    }
}

Any WrappedDataSourceLabelsInFirstColumnProperty::getPropertyValue( const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
                        throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
{
    ::rtl::OUString aRangeString;
    bool bUseColumns = true;
    bool bFirstCellAsLabel = true;
    bool bHasCategories = true;
    uno::Sequence< sal_Int32 > aSequenceMapping;

    if( DataSourceHelper::detectRangeSegmentation(
            m_spChart2ModelContact->getChartModel(), aRangeString, aSequenceMapping, bUseColumns
            , bFirstCellAsLabel, bHasCategories ) )
    {
        sal_Bool bLabelsInFirstColumn = sal_True;
        if( bUseColumns )
            bLabelsInFirstColumn = bHasCategories;
        else
            bLabelsInFirstColumn = bFirstCellAsLabel;

        m_aOuterValue <<= bLabelsInFirstColumn;
    }
    return m_aOuterValue;
}

Any WrappedDataSourceLabelsInFirstColumnProperty::getPropertyDefault( const Reference< beans::XPropertyState >& /*xInnerPropertyState*/ ) const
                        throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
{
    Any aRet;
    aRet <<= sal_True;
    return aRet;
}

//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------

//PROP_DOCUMENT_HAS_LEGEND
class WrappedHasLegendProperty : public WrappedProperty
{
public:
    WrappedHasLegendProperty( ::boost::shared_ptr< Chart2ModelContact > spChart2ModelContact );
    virtual ~WrappedHasLegendProperty();

    virtual void setPropertyValue( const ::com::sun::star::uno::Any& rOuterValue, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& xInnerPropertySet ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::uno::Any getPropertyValue( const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& xInnerPropertySet ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::uno::Any getPropertyDefault( const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertyState >& xInnerPropertyState ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

private: //member
    ::boost::shared_ptr< Chart2ModelContact > m_spChart2ModelContact;
};

WrappedHasLegendProperty::WrappedHasLegendProperty( ::boost::shared_ptr< Chart2ModelContact > spChart2ModelContact )
            : WrappedProperty(C2U("HasLegend"),OUString())
            , m_spChart2ModelContact( spChart2ModelContact )
{
}

WrappedHasLegendProperty::~WrappedHasLegendProperty()
{
}

void WrappedHasLegendProperty::setPropertyValue( const Any& rOuterValue, const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
                throw (beans::UnknownPropertyException, beans::PropertyVetoException, lang::IllegalArgumentException, lang::WrappedTargetException, uno::RuntimeException)
{
    sal_Bool bNewValue = sal_True;
    if( ! (rOuterValue >>= bNewValue) )
        throw lang::IllegalArgumentException( C2U("Property HasLegend requires value of type boolean"), 0, 0 );

    try
    {
        Reference< chart2::XLegend > xLegend( LegendHelper::getLegend( m_spChart2ModelContact->getChartModel(), m_spChart2ModelContact->m_xContext,bNewValue ));
        if(xLegend.is())
        {
            Reference< beans::XPropertySet > xLegendProp( xLegend, uno::UNO_QUERY_THROW );
            sal_Bool bOldValue = sal_True;
            Any aAOld = xLegendProp->getPropertyValue( C2U("Show") );
            aAOld >>= bOldValue;
            if( bOldValue != bNewValue )
                xLegendProp->setPropertyValue( C2U("Show"), uno::makeAny( bNewValue ));
        }
    }
    catch( uno::Exception & ex )
    {
        ASSERT_EXCEPTION( ex );
    }
}

Any WrappedHasLegendProperty::getPropertyValue( const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
                        throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
{
    Any aRet;
    try
    {
        Reference< beans::XPropertySet > xLegendProp(
            LegendHelper::getLegend( m_spChart2ModelContact->getChartModel() ), uno::UNO_QUERY );
        if( xLegendProp.is())
            aRet = xLegendProp->getPropertyValue( C2U("Show"));
        else
            aRet <<= sal_False;
    }
    catch( uno::Exception & ex )
    {
        ASSERT_EXCEPTION( ex );
    }
    return aRet;
}

Any WrappedHasLegendProperty::getPropertyDefault( const Reference< beans::XPropertyState >& /*xInnerPropertyState*/ ) const
                        throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
{
    Any aRet;
    aRet <<= sal_False;
    return aRet;
}

//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------

//PROP_DOCUMENT_HAS_MAIN_TITLE
class WrappedHasMainTitleProperty : public WrappedProperty
{
public:
    WrappedHasMainTitleProperty( ::boost::shared_ptr< Chart2ModelContact > spChart2ModelContact );
    virtual ~WrappedHasMainTitleProperty();

    virtual void setPropertyValue( const ::com::sun::star::uno::Any& rOuterValue, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& xInnerPropertySet ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::uno::Any getPropertyValue( const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& xInnerPropertySet ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::uno::Any getPropertyDefault( const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertyState >& xInnerPropertyState ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

private: //member
    ::boost::shared_ptr< Chart2ModelContact > m_spChart2ModelContact;
};

WrappedHasMainTitleProperty::WrappedHasMainTitleProperty( ::boost::shared_ptr< Chart2ModelContact > spChart2ModelContact )
            : WrappedProperty(C2U("HasMainTitle"),OUString())
            , m_spChart2ModelContact( spChart2ModelContact )
{
}

WrappedHasMainTitleProperty::~WrappedHasMainTitleProperty()
{
}

void WrappedHasMainTitleProperty::setPropertyValue( const Any& rOuterValue, const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
                throw (beans::UnknownPropertyException, beans::PropertyVetoException, lang::IllegalArgumentException, lang::WrappedTargetException, uno::RuntimeException)
{
    sal_Bool bNewValue = sal_True;
    if( ! (rOuterValue >>= bNewValue) )
        throw lang::IllegalArgumentException( C2U("Property HasMainTitle requires value of type boolean"), 0, 0 );

    try
    {
        if( bNewValue )
            TitleHelper::createTitle( TitleHelper::MAIN_TITLE, C2U("main-title"), m_spChart2ModelContact->getChartModel(), m_spChart2ModelContact->m_xContext );
        else
            TitleHelper::removeTitle( TitleHelper::MAIN_TITLE, m_spChart2ModelContact->getChartModel() );
    }
    catch( uno::Exception & ex )
    {
        ASSERT_EXCEPTION( ex );
    }
}

Any WrappedHasMainTitleProperty::getPropertyValue( const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
                        throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
{
    Any aRet;
    try
    {
        aRet <<= sal_Bool( TitleHelper::getTitle( TitleHelper::MAIN_TITLE, m_spChart2ModelContact->getChartModel() ).is() );
    }
    catch( uno::Exception & ex )
    {
        ASSERT_EXCEPTION( ex );
    }
    return aRet;
}

Any WrappedHasMainTitleProperty::getPropertyDefault( const Reference< beans::XPropertyState >& /*xInnerPropertyState*/ ) const
                        throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
{
    Any aRet;
    aRet <<= sal_False;
    return aRet;
}

//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------

//PROP_DOCUMENT_HAS_SUB_TITLE
class WrappedHasSubTitleProperty : public WrappedProperty
{
public:
    WrappedHasSubTitleProperty( ::boost::shared_ptr< Chart2ModelContact > spChart2ModelContact );
    virtual ~WrappedHasSubTitleProperty();

    virtual void setPropertyValue( const ::com::sun::star::uno::Any& rOuterValue, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& xInnerPropertySet ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::uno::Any getPropertyValue( const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySet >& xInnerPropertySet ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

    virtual ::com::sun::star::uno::Any getPropertyDefault( const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertyState >& xInnerPropertyState ) const
                        throw (::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

private: //member
    ::boost::shared_ptr< Chart2ModelContact > m_spChart2ModelContact;
};

WrappedHasSubTitleProperty::WrappedHasSubTitleProperty( ::boost::shared_ptr< Chart2ModelContact > spChart2ModelContact )
            : WrappedProperty(C2U("HasSubTitle"),OUString())
            , m_spChart2ModelContact( spChart2ModelContact )
{
}

WrappedHasSubTitleProperty::~WrappedHasSubTitleProperty()
{
}

void WrappedHasSubTitleProperty::setPropertyValue( const Any& rOuterValue, const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
                throw (beans::UnknownPropertyException, beans::PropertyVetoException, lang::IllegalArgumentException, lang::WrappedTargetException, uno::RuntimeException)
{
    sal_Bool bNewValue = sal_True;
    if( ! (rOuterValue >>= bNewValue) )
        throw lang::IllegalArgumentException( C2U("Property HasSubTitle requires value of type boolean"), 0, 0 );


    try
    {
        if( bNewValue )
            TitleHelper::createTitle( TitleHelper::SUB_TITLE, C2U("sub-title"), m_spChart2ModelContact->getChartModel(), m_spChart2ModelContact->m_xContext );
        else
            TitleHelper::removeTitle( TitleHelper::SUB_TITLE, m_spChart2ModelContact->getChartModel() );
    }
    catch( uno::Exception & ex )
    {
        ASSERT_EXCEPTION( ex );
    }
}

Any WrappedHasSubTitleProperty::getPropertyValue( const Reference< beans::XPropertySet >& /*xInnerPropertySet*/ ) const
                        throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
{
    Any aRet;
    try
    {
        aRet <<= sal_Bool( TitleHelper::getTitle( TitleHelper::SUB_TITLE, m_spChart2ModelContact->getChartModel() ).is() );
    }
    catch( uno::Exception & ex )
    {
        ASSERT_EXCEPTION( ex );
    }
    return aRet;
}

Any WrappedHasSubTitleProperty::getPropertyDefault( const Reference< beans::XPropertyState >& /*xInnerPropertyState*/ ) const
                        throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
{
    Any aRet;
    aRet <<= sal_False;
    return aRet;
}

//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------
ChartDocumentWrapper::ChartDocumentWrapper(
    const Reference< uno::XComponentContext > & xContext ) :
        m_spChart2ModelContact( new Chart2ModelContact( xContext ) ),
        m_bUpdateAddIn( sal_True ),
        m_bIsDisposed( false )
{
}

ChartDocumentWrapper::~ChartDocumentWrapper()
{
    stopAllComponentListening();
}

// ____ XInterface (for new interfaces) ____
uno::Any SAL_CALL ChartDocumentWrapper::queryInterface( const uno::Type& aType )
    throw (uno::RuntimeException)
{
    if( m_xDelegator.is())
        // calls queryAggregation if the delegator doesn't know aType
        return m_xDelegator->queryInterface( aType );
    else
        return queryAggregation( aType );
}

// ____ chart::XChartDocument (old API wrapper) ____
Reference< drawing::XShape > SAL_CALL ChartDocumentWrapper::getTitle()
    throw (uno::RuntimeException)
{
    if( !m_xTitle.is()  )
    {
        m_xTitle = new TitleWrapper( TitleHelper::MAIN_TITLE, m_spChart2ModelContact );
    }
    return m_xTitle;
}

Reference< drawing::XShape > SAL_CALL ChartDocumentWrapper::getSubTitle()
    throw (uno::RuntimeException)
{
    if( !m_xSubTitle.is() )
    {
        m_xSubTitle = new TitleWrapper( TitleHelper::SUB_TITLE, m_spChart2ModelContact );
    }
    return m_xSubTitle;
}

Reference< drawing::XShape > SAL_CALL ChartDocumentWrapper::getLegend()
    throw (uno::RuntimeException)
{
    if( ! m_xLegend.is())
    {
        m_xLegend = new LegendWrapper( m_spChart2ModelContact );
        Reference< lang::XComponent > xComp( m_xLegend, uno::UNO_QUERY );
    }

    return m_xLegend;
}

Reference< beans::XPropertySet > SAL_CALL ChartDocumentWrapper::getArea()
    throw (uno::RuntimeException)
{
    if( ! m_xArea.is())
    {
        m_xArea.set( new AreaWrapper( m_spChart2ModelContact ) );
        Reference< lang::XComponent > xComp( m_xArea, uno::UNO_QUERY );
    }

    return m_xArea;
}

Reference< XDiagram > SAL_CALL ChartDocumentWrapper::getDiagram()
    throw (uno::RuntimeException)
{
    if( !m_xDiagram.is()  )
    {
        try
        {
            m_xDiagram = new DiagramWrapper( m_spChart2ModelContact );
        }
        catch( uno::Exception & ex )
        {
            ASSERT_EXCEPTION( ex );
        }
    }

    return m_xDiagram;
}

void SAL_CALL ChartDocumentWrapper::setDiagram( const Reference< XDiagram >& xDiagram )
    throw (uno::RuntimeException)
{
    uno::Reference< util::XRefreshable > xAddIn( xDiagram, uno::UNO_QUERY );
	if( xAddIn.is() )
    {
        setAddIn( xAddIn );
    }
    else if( xDiagram.is() && xDiagram != m_xDiagram )
    {
        // set new wrapped diagram at new chart.  This requires the old
        // diagram given as parameter to implement the new interface.  If
        // this is not possible throw an exception
        Reference< chart2::XDiagramProvider > xNewDiaProvider( xDiagram, uno::UNO_QUERY_THROW );
        Reference< chart2::XDiagram > xNewDia( xNewDiaProvider->getDiagram());

        try
        {
            Reference< chart2::XChartDocument > xChartDoc( m_spChart2ModelContact->getChart2Document() );
            if( xChartDoc.is() )
            {
                // set the new diagram
                xChartDoc->setFirstDiagram( xNewDia );
                m_xDiagram = xDiagram;
            }
        }
        catch( uno::Exception & ex )
        {
            ASSERT_EXCEPTION( ex );
        }
    }
}

Reference< XChartData > SAL_CALL ChartDocumentWrapper::getData()
    throw (uno::RuntimeException)
{
    if( ! m_xChartData.is())
    {
        m_xChartData.set( new ChartDataWrapper( m_spChart2ModelContact ) );
    }
    //@todo: check hasInternalDataProvider also in else?

    return m_xChartData;
}

void SAL_CALL ChartDocumentWrapper::attachData( const Reference< XChartData >& xData )
    throw (uno::RuntimeException)
{
    if( !xData.is())
        return;

    Reference< chart2::XChartDocument > xChartDoc( m_spChart2ModelContact->getChart2Document() );
    if( !xChartDoc.is() )
        return;

    uno::Reference< chart2::data::XDataProvider > xDataProvider( xChartDoc->getDataProvider());
    uno::Reference< XChartDataArray > xDocDataArray( xDataProvider, uno::UNO_QUERY );
    uno::Reference< XChartDataArray > xDataArray( xData, uno::UNO_QUERY );
    OSL_ASSERT( xDataArray.is());
    if( ! xDataArray.is() ||
        xDocDataArray == xDataArray )
        return;

    // remember some diagram properties to reset later
    ChartDataRowSource eSeriesSource = ChartDataRowSource_ROWS;
    sal_Bool bStacked = sal_False;
    sal_Bool bPercent = sal_False;
    sal_Bool bDeep = sal_False;
    Reference< beans::XPropertySet > xDiaProp( getDiagram(), uno::UNO_QUERY );
    if( xDiaProp.is())
    {
        xDiaProp->getPropertyValue( C2U("DataRowSource")) >>= eSeriesSource;
        xDiaProp->getPropertyValue( C2U("Stacked")) >>= bStacked;
        xDiaProp->getPropertyValue( C2U("Percent")) >>= bPercent;
        xDiaProp->getPropertyValue( C2U("Deep")) >>= bDeep;
    }

    // create and attach new data source
    uno::Reference< chart2::data::XDataSource > xSource;
    Sequence< beans::PropertyValue > aArguments( 4 );
    aArguments[0] = beans::PropertyValue(
        C2U("CellRangeRepresentation"), -1, uno::makeAny( C2U("all") ),
        beans::PropertyState_DIRECT_VALUE );
    aArguments[1] = beans::PropertyValue(
        C2U("DataRowSource"), -1, uno::makeAny( eSeriesSource ),
        beans::PropertyState_DIRECT_VALUE );
    aArguments[2] = beans::PropertyValue(
        C2U("FirstCellAsLabel"), -1, uno::makeAny( true ), beans::PropertyState_DIRECT_VALUE );
    aArguments[3] = beans::PropertyValue(
        C2U("HasCategories"), -1, uno::makeAny( true ), beans::PropertyState_DIRECT_VALUE );

    // /-- locked controllers
    ControllerLockGuard aCtrlLockGuard( Reference< frame::XModel >( xChartDoc, uno::UNO_QUERY ));
    if( xDocDataArray.is())
    {
        // we have an internal data provider that supports the XChartDataArray
        // interface
        xDocDataArray->setData( xDataArray->getData());
        xDocDataArray->setRowDescriptions( xDataArray->getRowDescriptions());
        xDocDataArray->setColumnDescriptions( xDataArray->getColumnDescriptions());

        xSource.set( xDataProvider->createDataSource( aArguments ));
    }
    else
    {
        uno::Reference< chart2::data::XDataReceiver > xReceiver( xChartDoc, uno::UNO_QUERY );
        OSL_ASSERT( xChartDoc.is());
        OSL_ASSERT( xReceiver.is());
        OSL_ASSERT( xDataArray.is());
        if( ! (xChartDoc.is() &&
               xReceiver.is()))
            return;

        // create a data provider containing the new data
        Reference< chart2::data::XDataProvider > xTempDataProvider(
            ChartModelHelper::createInternalDataProvider( xDataArray ));

        if( ! xTempDataProvider.is())
            throw uno::RuntimeException( C2U("Couldn't create temporary data provider"),
                                         static_cast< ::cppu::OWeakObject * >( this ));

        // removes existing data provider and attaches the new one
        xReceiver->attachDataProvider( xTempDataProvider );
        xSource.set( xTempDataProvider->createDataSource( aArguments));
    }

    // determine a template
    Reference< lang::XMultiServiceFactory > xFact( xChartDoc->getChartTypeManager(), uno::UNO_QUERY );
    Reference< chart2::XDiagram > xDia( xChartDoc->getFirstDiagram());
    DiagramHelper::tTemplateWithServiceName aTemplateAndService =
        DiagramHelper::getTemplateForDiagram( xDia, xFact );
    OUString aServiceName( aTemplateAndService.second );
    Reference< chart2::XChartTypeTemplate > xTemplate = aTemplateAndService.first;

    // (fall-back)
    if( ! xTemplate.is())
    {
        if( aServiceName.getLength() == 0 )
            aServiceName = C2U("com.sun.star.chart2.template.Column");
        xTemplate.set( xFact->createInstance( aServiceName ), uno::UNO_QUERY );
    }
    OSL_ASSERT( xTemplate.is());

    if( xTemplate.is() && xSource.is())
    {
        // argument detection works with internal knowledge of the
        // ArrayDataProvider
        OSL_ASSERT( xDia.is());
        xTemplate->changeDiagramData(
            xDia, xSource, aArguments );
    }

    // should do nothing if we already have an internal data provider
    xChartDoc->createInternalDataProvider( sal_True /* bCloneExistingData */ );

    //correct stacking mode
    if( bStacked || bPercent || bDeep )
    {
        StackMode eStackMode = StackMode_Y_STACKED;
        if( bDeep )
            eStackMode = StackMode_Z_STACKED;
        else if( bPercent )
            eStackMode = StackMode_Y_STACKED_PERCENT;
        DiagramHelper::setStackMode( xDia, eStackMode );
    }

    m_xChartData = xData;
    // \-- locked controllers
}

// ____ XModel ____
sal_Bool SAL_CALL ChartDocumentWrapper::attachResource(
    const ::rtl::OUString& URL,
    const Sequence< beans::PropertyValue >& Arguments )
    throw (uno::RuntimeException)
{
    Reference< frame::XModel > xModel( m_spChart2ModelContact->getChartModel() );
    if( xModel.is() )
        return xModel->attachResource( URL, Arguments );
    return sal_False;
}

::rtl::OUString SAL_CALL ChartDocumentWrapper::getURL()
    throw (uno::RuntimeException)
{
    Reference< frame::XModel > xModel( m_spChart2ModelContact->getChartModel() );
    if( xModel.is() )
        return xModel->getURL();
    return ::rtl::OUString();
}

Sequence< beans::PropertyValue > SAL_CALL ChartDocumentWrapper::getArgs()
    throw (uno::RuntimeException)
{
    Reference< frame::XModel > xModel( m_spChart2ModelContact->getChartModel() );
    if( xModel.is() )
        return xModel->getArgs();
    return Sequence< beans::PropertyValue >();
}

void SAL_CALL ChartDocumentWrapper::connectController( const Reference< frame::XController >& Controller )
    throw (uno::RuntimeException)
{
    Reference< frame::XModel > xModel( m_spChart2ModelContact->getChartModel() );
    if( xModel.is() )
        xModel->connectController( Controller );
}

void SAL_CALL ChartDocumentWrapper::disconnectController(
    const Reference< frame::XController >& Controller )
    throw (uno::RuntimeException)
{
    Reference< frame::XModel > xModel( m_spChart2ModelContact->getChartModel() );
    if( xModel.is() )
        xModel->disconnectController( Controller );
}

void SAL_CALL ChartDocumentWrapper::lockControllers()
    throw (uno::RuntimeException)
{
    Reference< frame::XModel > xModel( m_spChart2ModelContact->getChartModel() );
    if( xModel.is() )
        xModel->lockControllers();
}

void SAL_CALL ChartDocumentWrapper::unlockControllers()
    throw (uno::RuntimeException)
{
    Reference< frame::XModel > xModel( m_spChart2ModelContact->getChartModel() );
    if( xModel.is() )
        xModel->unlockControllers();
}

sal_Bool SAL_CALL ChartDocumentWrapper::hasControllersLocked()
    throw (uno::RuntimeException)
{
    Reference< frame::XModel > xModel( m_spChart2ModelContact->getChartModel() );
    if( xModel.is() )
        return xModel->hasControllersLocked();
    return sal_False;
}

Reference< frame::XController > SAL_CALL ChartDocumentWrapper::getCurrentController()
    throw (uno::RuntimeException)
{
    Reference< frame::XModel > xModel( m_spChart2ModelContact->getChartModel() );
    if( xModel.is() )
        return xModel->getCurrentController();
    return 0;
}

void SAL_CALL ChartDocumentWrapper::setCurrentController(
    const Reference< frame::XController >& Controller )
    throw (container::NoSuchElementException,
           uno::RuntimeException)
{
    Reference< frame::XModel > xModel( m_spChart2ModelContact->getChartModel() );
    if( xModel.is() )
        xModel->setCurrentController( Controller );
}

Reference< uno::XInterface > SAL_CALL ChartDocumentWrapper::getCurrentSelection()
    throw (uno::RuntimeException)
{
    Reference< frame::XModel > xModel( m_spChart2ModelContact->getChartModel() );
    if( xModel.is() )
        return xModel->getCurrentSelection();
    return 0;
}


// ____ XComponent ____
void SAL_CALL ChartDocumentWrapper::dispose()
    throw (uno::RuntimeException)
{
    if( m_bIsDisposed )
		throw lang::DisposedException(
            C2U("ChartDocumentWrapper is disposed" ),
            static_cast< ::cppu::OWeakObject* >( this ));

    m_bIsDisposed = true;

    try
    {
        Reference< lang::XComponent > xFormerDelegator( m_xDelegator, uno::UNO_QUERY );
        DisposeHelper::DisposeAndClear( m_xTitle );
        DisposeHelper::DisposeAndClear( m_xSubTitle );
        DisposeHelper::DisposeAndClear( m_xLegend );
        DisposeHelper::DisposeAndClear( m_xChartData );
        DisposeHelper::DisposeAndClear( m_xDiagram );
        DisposeHelper::DisposeAndClear( m_xArea );
        m_xChartView.set( 0 );
        m_xShapeFactory.set( 0 );
        m_xDelegator.set( 0 );

        clearWrappedPropertySet();
        m_spChart2ModelContact->clear();
        impl_resetAddIn();

        stopAllComponentListening();

        try
        {
            if( xFormerDelegator.is())
                xFormerDelegator->dispose();
        }
        catch( lang::DisposedException )
        {
            // this is ok, don't panic
        }
    }
    catch( uno::Exception &ex )
    {
        ASSERT_EXCEPTION( ex );
    }
}

void ChartDocumentWrapper::impl_resetAddIn()
{
    Reference< util::XRefreshable > xAddIn( m_xAddIn );
    m_xAddIn.set( 0 );

    if( xAddIn.is() )
    {
        try
        {
            //make sure that the add-in does not hold a refernce to us anymore:
            Reference< lang::XComponent > xComp( xAddIn, uno::UNO_QUERY );
            if( xComp.is())
                xComp->dispose();
            else
            {
                uno::Reference< lang::XInitialization > xInit( xAddIn, uno::UNO_QUERY );
	            if( xInit.is() )
	            {
                    uno::Any aParam;
                    uno::Reference< com::sun::star::chart::XChartDocument > xDoc( 0 );
		            aParam <<= xDoc;
		            uno::Sequence< uno::Any > aSeq( &aParam, 1 );
		            xInit->initialize( aSeq );
                }
            }
        }
        catch( const uno::RuntimeException& ex )
        {
            ASSERT_EXCEPTION( ex );
        }
        catch( const uno::Exception& ex )
        {
            ASSERT_EXCEPTION( ex );
        }
    }
}

void ChartDocumentWrapper::setBaseDiagram( const rtl::OUString& rBaseDiagram )
{
    ControllerLockGuard aCtrlLockGuard( m_spChart2ModelContact->getChartModel() );
    m_aBaseDiagram = rBaseDiagram;

    uno::Reference< XDiagram > xDiagram( ChartDocumentWrapper::createInstance( rBaseDiagram ), uno::UNO_QUERY );
    if( xDiagram.is() )
        this->setDiagram( xDiagram );
}

rtl::OUString ChartDocumentWrapper::getBaseDiagram() const
{
    return m_aBaseDiagram;
}

Reference< util::XRefreshable > ChartDocumentWrapper::getAddIn() const
{
    return m_xAddIn;
}

void ChartDocumentWrapper::setAddIn( const Reference< util::XRefreshable >& xAddIn )
{
    if( m_xAddIn == xAddIn )
        return;

    ControllerLockGuard aCtrlLockGuard( m_spChart2ModelContact->getChartModel() );
    impl_resetAddIn();
    m_xAddIn = xAddIn;
    // initialize AddIn with this as chart document
	uno::Reference< lang::XInitialization > xInit( m_xAddIn, uno::UNO_QUERY );
	if( xInit.is() )
	{
		uno::Any aParam;
        uno::Reference< XChartDocument > xDoc( (XChartDocument*)this, uno::UNO_QUERY );
		aParam <<= xDoc;
		uno::Sequence< uno::Any > aSeq( &aParam, 1 );
		xInit->initialize( aSeq );
	}
}

void ChartDocumentWrapper::setUpdateAddIn( sal_Bool bUpdateAddIn )
{
    m_bUpdateAddIn = bUpdateAddIn;
}
sal_Bool ChartDocumentWrapper::getUpdateAddIn() const
{
    return m_bUpdateAddIn;
}

Reference< drawing::XShapes > ChartDocumentWrapper::getAdditionalShapes() const
{
    // get additional non-chart shapes for XML export
    uno::Reference< drawing::XShapes > xFoundShapes;
    uno::Reference< drawing::XDrawPage > xDrawPage( this->impl_getDrawPage() );

    uno::Reference< drawing::XShapes > xDrawPageShapes( xDrawPage, uno::UNO_QUERY );
    if( !xDrawPageShapes.is() )
        return xFoundShapes;

    uno::Reference<drawing::XShapes> xChartRoot( DrawModelWrapper::getChartRootShape( xDrawPage ) );

    // iterate 'flat' over all top-level objects
    // and determine all that are no chart objects
    ::std::vector< uno::Reference< drawing::XShape > > aShapeVector;
    sal_Int32 nSubCount = xDrawPageShapes->getCount();
    uno::Reference< drawing::XShape > xShape;
    for( sal_Int32 nS = 0; nS < nSubCount; nS++ )
    {
        if( xDrawPageShapes->getByIndex( nS ) >>= xShape )
        {
            if( xShape.is() && xChartRoot!=xShape )
                aShapeVector.push_back( xShape );
        }
    }

    if( !aShapeVector.empty() )
    {
        // create a shape collection
        xFoundShapes = uno::Reference< drawing::XShapes >( SvxShapeCollection_NewInstance(), uno::UNO_QUERY );

        DBG_ASSERT( xFoundShapes.is(), "Couldn't create a shape collection!" );
        if( xFoundShapes.is())
        {
            ::std::vector< uno::Reference< drawing::XShape > >::iterator aIter;
            for( aIter = aShapeVector.begin(); aIter != aShapeVector.end(); ++aIter )
                xFoundShapes->add( *aIter );
        }
    }

    return xFoundShapes;
}

void SAL_CALL ChartDocumentWrapper::addEventListener( const Reference< lang::XEventListener >& xListener )
    throw (uno::RuntimeException)
{
    Reference< frame::XModel > xModel( m_spChart2ModelContact->getChartModel() );
    if( xModel.is() )
        xModel->addEventListener( xListener );
}

void SAL_CALL ChartDocumentWrapper::removeEventListener( const Reference< lang::XEventListener >& aListener )
    throw (uno::RuntimeException)
{
    Reference< frame::XModel > xModel( m_spChart2ModelContact->getChartModel() );
    if( xModel.is() )
        xModel->removeEventListener( aListener );
}

// ____ XDrawPageSupplier ____
uno::Reference< drawing::XDrawPage > SAL_CALL ChartDocumentWrapper::getDrawPage()
    throw (uno::RuntimeException)
{
    return this->impl_getDrawPage();
}

uno::Reference< drawing::XDrawPage > ChartDocumentWrapper::impl_getDrawPage() const
    throw (uno::RuntimeException)
{
    return m_spChart2ModelContact->getDrawPage();
}

// ____ XMultiServiceFactory ____
uno::Reference< uno::XInterface > SAL_CALL ChartDocumentWrapper::createInstance(
    const ::rtl::OUString& aServiceSpecifier )
    throw (uno::Exception,
           uno::RuntimeException)
{
    uno::Reference< uno::XInterface > xResult;

    Reference< chart2::XChartDocument > xChartDoc( m_spChart2ModelContact->getChart2Document() );
    if( !xChartDoc.is() )
        return xResult;

    bool bServiceFound = false;
    tServiceNameMap & rMap = lcl_getStaticServiceNameMap();

    tServiceNameMap::const_iterator aIt( rMap.find( aServiceSpecifier ));
    if( aIt != rMap.end())
    {
        bool bCreateDiagram = false;
        uno::Reference< lang::XMultiServiceFactory > xManagerFact(
            xChartDoc->getChartTypeManager(), uno::UNO_QUERY );
        uno::Reference< chart2::XChartTypeTemplate > xTemplate;

        switch( (*aIt).second )
        {
            case SERVICE_NAME_AREA_DIAGRAM:
                if( xManagerFact.is())
                {
                    xTemplate.set(
                        xManagerFact->createInstance(
                            C2U( "com.sun.star.chart2.template.Area" )), uno::UNO_QUERY );
                    bCreateDiagram = true;
                }
                break;
            case SERVICE_NAME_BAR_DIAGRAM:
                if( xManagerFact.is())
                {
                    // this is for bar and column (the latter is the default if
                    // no "Vertical=false" property was set)
                    xTemplate.set(
                        xManagerFact->createInstance(
                            C2U( "com.sun.star.chart2.template.Column" )), uno::UNO_QUERY );
                    bCreateDiagram = true;
                }
                break;
            case SERVICE_NAME_DONUT_DIAGRAM:
                if( xManagerFact.is())
                {
                    xTemplate.set(
                        xManagerFact->createInstance(
                            C2U( "com.sun.star.chart2.template.Donut" )), uno::UNO_QUERY );
                    bCreateDiagram = true;
                }
                break;
            case SERVICE_NAME_LINE_DIAGRAM:
                if( xManagerFact.is())
                {
                    xTemplate.set(
                        xManagerFact->createInstance(
                            C2U( "com.sun.star.chart2.template.Line" )), uno::UNO_QUERY );
                    bCreateDiagram = true;
                }
                break;
            case SERVICE_NAME_NET_DIAGRAM:
                if( xManagerFact.is())
                {
                    xTemplate.set(
                        xManagerFact->createInstance(
                            C2U( "com.sun.star.chart2.template.Net" )), uno::UNO_QUERY );
                    bCreateDiagram = true;
                }
                break;
            case SERVICE_NAME_FILLED_NET_DIAGRAM:
                if( xManagerFact.is())
                {
                    xTemplate.set(
                        xManagerFact->createInstance(
                            C2U( "com.sun.star.chart2.template.FilledNet" )), uno::UNO_QUERY );
                    bCreateDiagram = true;
                }
            case SERVICE_NAME_PIE_DIAGRAM:
                if( xManagerFact.is())
                {
                    xTemplate.set(
                        xManagerFact->createInstance(
                            C2U( "com.sun.star.chart2.template.Pie" )), uno::UNO_QUERY );
                    bCreateDiagram = true;
                }
                break;
            case SERVICE_NAME_STOCK_DIAGRAM:
                if( xManagerFact.is())
                {
                    xTemplate.set(
                        xManagerFact->createInstance(
                            C2U( "com.sun.star.chart2.template.StockLowHighClose" )), uno::UNO_QUERY );
                    bCreateDiagram = true;
                }
                break;
            case SERVICE_NAME_XY_DIAGRAM:
                if( xManagerFact.is())
                {
                    xTemplate.set(
                        xManagerFact->createInstance(
                            C2U( "com.sun.star.chart2.template.ScatterLineSymbol" )), uno::UNO_QUERY );
                    bCreateDiagram = true;
                }
                break;

            case SERVICE_NAME_BUBBLE_DIAGRAM:
                if( xManagerFact.is())
                {
                    xTemplate.set(
                        xManagerFact->createInstance(
                            C2U( "com.sun.star.chart2.template.Bubble" )), uno::UNO_QUERY );
                    bCreateDiagram = true;
                }
                break;

            case SERVICE_NAME_DASH_TABLE:
            case SERVICE_NAME_GARDIENT_TABLE:
            case SERVICE_NAME_HATCH_TABLE:
            case SERVICE_NAME_BITMAP_TABLE:
            case SERVICE_NAME_TRANSP_GRADIENT_TABLE:
            case SERVICE_NAME_MARKER_TABLE:
                {
                    uno::Reference< lang::XMultiServiceFactory > xTableFactory( xChartDoc, uno::UNO_QUERY );
                    DBG_ASSERT( xTableFactory.get() != this, "new model is expected to implement service factory for gradient table etc" );
                    if( xTableFactory.is() && xTableFactory.get() != this )
                        xResult.set( xTableFactory->createInstance( aIt->first ), uno::UNO_QUERY );
                }
                break;

            case SERVICE_NAME_NAMESPACE_MAP:
//                 xResult = svx::NamespaceMap_createInstance( aWhichIds, &m_pModel->GetPool() );
                break;
            case SERVICE_NAME_EXPORT_GRAPHIC_RESOLVER:
//                 xResult = static_cast< ::cppu::OWeakObject * >( new SvXMLGraphicHelper( GRAPHICHELPER_MODE_WRITE ));
                break;
            case SERVICE_NAME_IMPORT_GRAPHIC_RESOLVER:
//                 xResult = static_cast< ::cppu::OWeakObject * >( new SvXMLGraphicHelper( GRAPHICHELPER_MODE_READ ));
                break;
        }

        if( bCreateDiagram && xTemplate.is() )
        {
            try
            {
                uno::Reference< chart2::XDiagram > xDia( xChartDoc->getFirstDiagram());
                if( xDia.is())
                {
                    // /-- locked controllers
                    ControllerLockGuard aCtrlLockGuard( Reference< frame::XModel >( xChartDoc, uno::UNO_QUERY ));
                    xTemplate->changeDiagram( xDia );
                    // \-- locked controllers
                }
                else
                {
                    // /-- locked controllers
                    ControllerLockGuard aCtrlLockGuard( Reference< frame::XModel >( xChartDoc, uno::UNO_QUERY ));
                    xDia.set( xTemplate->createDiagramByDataSource(
                                  uno::Reference< chart2::data::XDataSource >(),
                                  uno::Sequence< beans::PropertyValue >()));
                    xChartDoc->setFirstDiagram( xDia );
                    // \-- locked controllers
                }

                xResult = static_cast< ::cppu::OWeakObject* >( new DiagramWrapper( m_spChart2ModelContact ));
            }
            catch( uno::Exception & ex )
            {
                ASSERT_EXCEPTION( ex );
            }
        }

        bServiceFound = true;
    }
    else if( aServiceSpecifier.equals( CHART_RENDERER_SERVICE_IMPLEMENTATION_NAME ) )
    {
        Reference< lang::XUnoTunnel > xChartRenderer( new ChartRenderer( m_spChart2ModelContact->getChartModel() ) );
        xResult.set( xChartRenderer );
        bServiceFound = true;
    }
    else if( aServiceSpecifier.equals( C2U("com.sun.star.comp.chart2.DataSeriesWrapper") ) )
    {
        Reference< beans::XPropertySet > xDataSeries( new DataSeriesPointWrapper( m_spChart2ModelContact ) );
        xResult.set( xDataSeries );
        bServiceFound = true;
    }
    else if( aServiceSpecifier.equals( CHART_VIEW_SERVICE_NAME ) )
    {
        if( !m_xChartView.is() )
        {
            Reference< lang::XMultiServiceFactory > xFact(
                m_spChart2ModelContact->m_xContext->getServiceManager(), uno::UNO_QUERY_THROW );
            if( xFact.is() )
            {
                Reference< lang::XInitialization > xViewInit( xFact->createInstance(
                        CHART_VIEW_SERVICE_NAME ), uno::UNO_QUERY );
                if(xViewInit.is())
                {
                    try
                    {
                        m_xChartView = xViewInit;

                        Sequence< Any > aArguments(2);
                        Reference<frame::XModel> xModel(this);
                        aArguments[0]=uno::makeAny(xModel);
                        sal_Bool bRefreshAddIn = sal_True;
                        aArguments[1]=uno::makeAny(bRefreshAddIn);
                        xViewInit->initialize(aArguments);
                    }
                    catch( uno::Exception & ex )
                    {
                        ASSERT_EXCEPTION( ex );
                    }
                }
            }
        }
        xResult.set( m_xChartView );
        bServiceFound = true;
    }
    else
    {
        // try to create a shape
        try
        {
            if( !m_xShapeFactory.is() && m_xChartView.is() )
            {
                Reference< lang::XUnoTunnel> xUnoTunnel(m_xChartView,uno::UNO_QUERY);    
                if(xUnoTunnel.is())
                {
                    ExplicitValueProvider* pProvider = reinterpret_cast<ExplicitValueProvider*>(xUnoTunnel->getSomething(
                        ExplicitValueProvider::getUnoTunnelId() ));
                    if( pProvider )
                        m_xShapeFactory.set( pProvider->getDrawModelWrapper()->getShapeFactory() );
                }
            }
            if( m_xShapeFactory.is() )
            {
                xResult = m_xShapeFactory->createInstance( aServiceSpecifier );
                bServiceFound = true;
            }
        }
        catch( const uno::Exception )
        {
            // couldn't create shape
        }
    }

    // finally, try to create an addin
    if( !bServiceFound )
    {
        try
        {
            Reference< lang::XMultiServiceFactory > xFact(
                m_spChart2ModelContact->m_xContext->getServiceManager(), uno::UNO_QUERY_THROW );
            if( xFact.is() )
            {
                uno::Reference< util::XRefreshable > xAddIn(
                    xFact->createInstance( aServiceSpecifier ), uno::UNO_QUERY );
                if( xAddIn.is() )
                {
                    xResult = xAddIn;
                    bServiceFound = true;
                }
            }
        }
        catch( const uno::Exception& ex )
        {
            (void)ex;
            // couldn't create service
        }
    }

    return xResult;
}

uno::Reference< uno::XInterface > SAL_CALL ChartDocumentWrapper::createInstanceWithArguments(
    const ::rtl::OUString& ServiceSpecifier,
    const uno::Sequence< uno::Any >& Arguments )
    throw (uno::Exception,
           uno::RuntimeException)
{
    OSL_ENSURE( Arguments.getLength(), "createInstanceWithArguments: Warning: Arguments are ignored" );
    (void)(Arguments);

    return createInstance( ServiceSpecifier );
}

uno::Sequence< ::rtl::OUString > SAL_CALL ChartDocumentWrapper::getAvailableServiceNames()
    throw (uno::RuntimeException)
{
    tServiceNameMap & rMap = lcl_getStaticServiceNameMap();
    uno::Sequence< ::rtl::OUString > aResult( rMap.size());

    ::std::transform( rMap.begin(), rMap.end(),
                      aResult.getArray(),
                      ::std::select1st< tServiceNameMap::value_type >() );

    return aResult;

//         // shapes
// //         uno::Sequence< OUString > aDrawServices( SvxUnoDrawMSFactory::getAvailableServiceNames() );
// //         const OUString * pArr = aDrawServices.getConstArray();
// //         aServices.insert( aServices.end(), pArr, pArr + aDrawServices.getLength() );
//     }


    // add-ins
 //    uno::Sequence< OUString > aAddIns( GetAddInCollection().GetAddInNames() );
//     pArr = aAddIns.getConstArray();
//     aServices.insert( aServices.end(), pArr, pArr + aAddIns.getLength() );

//     return ContainerToSequence( aServices );
}

// ____ XAggregation ____
void SAL_CALL ChartDocumentWrapper::setDelegator(
    const uno::Reference< uno::XInterface >& rDelegator )
    throw (uno::RuntimeException)
{
    if( rDelegator.is())
    {
        m_xDelegator = rDelegator;
        m_spChart2ModelContact->setModel( uno::Reference< frame::XModel >(m_xDelegator, uno::UNO_QUERY) );
    }
    else
    {
        // this is a sort of dispose() from the new model,so release ressources here
        try
        {
            this->dispose();
        }
        catch( uno::Exception &ex )
        {
            ASSERT_EXCEPTION( ex );
        }
    }
}

uno::Any SAL_CALL ChartDocumentWrapper::queryAggregation( const uno::Type& rType )
    throw (uno::RuntimeException)
{
    return ChartDocumentWrapper_Base::queryInterface( rType );
}

// ____ ::utl::OEventListenerAdapter ____
void ChartDocumentWrapper::_disposing( const lang::EventObject& rSource )
{
    if( rSource.Source == m_xTitle )
        m_xTitle.set( 0 );
    else if( rSource.Source == m_xSubTitle )
        m_xSubTitle.set( 0 );
    else if( rSource.Source == m_xLegend )
        m_xLegend.set( 0 );
    else if( rSource.Source == m_xChartData )
        m_xChartData.set( 0 );
    else if( rSource.Source == m_xDiagram )
        m_xDiagram.set( 0 );
    else if( rSource.Source == m_xArea )
        m_xArea.set( 0 );
    else if( rSource.Source == m_xAddIn )
        m_xAddIn.set( 0 );
    else if( rSource.Source == m_xChartView )
        m_xChartView.set( 0 );
}

// ================================================================================

// WrappedPropertySet
Reference< beans::XPropertySet > ChartDocumentWrapper::getInnerPropertySet()
{
    return 0;
}
const Sequence< beans::Property >& ChartDocumentWrapper::getPropertySequence()
{
    return lcl_GetPropertySequence();
}

const std::vector< WrappedProperty* > ChartDocumentWrapper::createWrappedProperties()
{
    ::std::vector< ::chart::WrappedProperty* > aWrappedProperties;
    aWrappedProperties.push_back( new WrappedDataSourceLabelsInFirstRowProperty( m_spChart2ModelContact ) );
    aWrappedProperties.push_back( new WrappedDataSourceLabelsInFirstColumnProperty( m_spChart2ModelContact ) );
    aWrappedProperties.push_back( new WrappedHasLegendProperty( m_spChart2ModelContact ) );
    aWrappedProperties.push_back( new WrappedHasMainTitleProperty( m_spChart2ModelContact ) );
    aWrappedProperties.push_back( new WrappedHasSubTitleProperty( m_spChart2ModelContact ) );
    aWrappedProperties.push_back( new WrappedAddInProperty( *this ) );
    aWrappedProperties.push_back( new WrappedBaseDiagramProperty( *this ) );
    aWrappedProperties.push_back( new WrappedAdditionalShapesProperty( *this ) );
    aWrappedProperties.push_back( new WrappedRefreshAddInAllowedProperty( *this ) );
    aWrappedProperties.push_back( new WrappedIgnoreProperty( C2U("NullDate"),Any() ) ); // i99104

    return aWrappedProperties;
}

// ================================================================================

uno::Sequence< ::rtl::OUString > ChartDocumentWrapper::getSupportedServiceNames_Static()
{
    uno::Sequence< ::rtl::OUString > aServices( 4 );
    aServices[ 0 ] = C2U( "com.sun.star.chart.ChartDocument" );
    aServices[ 1 ] = CHART_CHARTAPIWRAPPER_SERVICE_NAME;
    aServices[ 2 ] = C2U( "com.sun.star.xml.UserDefinedAttributeSupplier" );
    aServices[ 3 ] = C2U( "com.sun.star.beans.PropertySet" );
    return aServices;
}

// implement XServiceInfo methods basing upon getSupportedServiceNames_Static
APPHELPER_XSERVICEINFO_IMPL( ChartDocumentWrapper, CHART_CHARTAPIWRAPPER_IMPLEMENTATION_NAME );

} //  namespace wrapper
} //  namespace chart
