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
#include "ScatterChartTypeTemplate.hxx"
#include "macros.hxx"
#include "XYDataInterpreter.hxx"
#include "CartesianCoordinateSystem.hxx"
#include "DiagramHelper.hxx"
#include "servicenames_charttypes.hxx"
#include "ContainerHelper.hxx"
#include "DataSeriesHelper.hxx"
#include <com/sun/star/chart2/SymbolStyle.hpp>
#include <com/sun/star/chart2/Symbol.hpp>
#include <com/sun/star/drawing/LineStyle.hpp>
#include "PropertyHelper.hxx"
#include <com/sun/star/beans/PropertyAttribute.hpp>

#include <algorithm>

using namespace ::com::sun::star;

using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::rtl::OUString;
using ::com::sun::star::beans::Property;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Any;
using ::osl::MutexGuard;

namespace
{

static const OUString lcl_aServiceName(
    RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.chart2.ScatterChartTypeTemplate" ));

enum
{
    PROP_SCATTERCHARTTYPE_TEMPLATE_CURVE_STYLE,
    PROP_SCATTERCHARTTYPE_TEMPLATE_CURVE_RESOLUTION,
    PROP_SCATTERCHARTTYPE_TEMPLATE_SPLINE_ORDER

};

void lcl_AddPropertiesToVector(
    ::std::vector< Property > & rOutProperties )
{
    rOutProperties.push_back(
        Property( C2U( "CurveStyle" ),
                  PROP_SCATTERCHARTTYPE_TEMPLATE_CURVE_STYLE,
                  ::getCppuType( reinterpret_cast< const chart2::CurveStyle * >(0)),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT ));
    rOutProperties.push_back(
        Property( C2U( "CurveResolution" ),
                  PROP_SCATTERCHARTTYPE_TEMPLATE_CURVE_RESOLUTION,
                  ::getCppuType( reinterpret_cast< const sal_Int32 * >(0)),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT ));
    rOutProperties.push_back(
        Property( C2U( "SplineOrder" ),
                  PROP_SCATTERCHARTTYPE_TEMPLATE_SPLINE_ORDER,
                  ::getCppuType( reinterpret_cast< const sal_Int32 * >(0)),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT ));
}

void lcl_AddDefaultsToMap(
    ::chart::tPropertyValueMap & rOutMap )
{
    ::chart::PropertyHelper::setPropertyValueDefault( rOutMap, PROP_SCATTERCHARTTYPE_TEMPLATE_CURVE_STYLE, chart2::CurveStyle_LINES );
    ::chart::PropertyHelper::setPropertyValueDefault< sal_Int32 >( rOutMap, PROP_SCATTERCHARTTYPE_TEMPLATE_CURVE_RESOLUTION, 20 );

    // todo: check whether order 3 means polygons of order 3 or 2. (see
    // http://www.people.nnov.ru/fractal/Splines/Basis.htm )
    ::chart::PropertyHelper::setPropertyValueDefault< sal_Int32 >( rOutMap, PROP_SCATTERCHARTTYPE_TEMPLATE_SPLINE_ORDER, 3 );
}

const Sequence< Property > & lcl_GetPropertySequence()
{
    static Sequence< Property > aPropSeq;

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

::cppu::IPropertyArrayHelper & lcl_getInfoHelper()
{
    static ::cppu::OPropertyArrayHelper aArrayHelper(
        lcl_GetPropertySequence(),
        /* bSorted = */ sal_True );

    return aArrayHelper;
}

} // anonymous namespace

namespace chart
{

ScatterChartTypeTemplate::ScatterChartTypeTemplate(
    Reference<
        uno::XComponentContext > const & xContext,
    const OUString & rServiceName,
    bool bSymbols,
    bool bHasLines /* = true */,
    sal_Int32 nDim /* = 2 */ ) :
        ChartTypeTemplate( xContext, rServiceName ),
        ::property::OPropertySet( m_aMutex ),
    m_bHasSymbols( bSymbols ),
    m_bHasLines( bHasLines ),
    m_nDim( nDim )
{
    if( nDim == 3 )
        m_bHasSymbols = false;
}

ScatterChartTypeTemplate::~ScatterChartTypeTemplate()
{}

// ____ OPropertySet ____
uno::Any ScatterChartTypeTemplate::GetDefaultValue( sal_Int32 nHandle ) const
    throw(beans::UnknownPropertyException)
{
    static tPropertyValueMap aStaticDefaults;

    // /--
    ::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
    if( 0 == aStaticDefaults.size() )
    {
        // initialize defaults
        lcl_AddDefaultsToMap( aStaticDefaults );
    }

    tPropertyValueMap::const_iterator aFound(
        aStaticDefaults.find( nHandle ));

    if( aFound == aStaticDefaults.end())
        return uno::Any();

    return (*aFound).second;
    // \--
}

::cppu::IPropertyArrayHelper & SAL_CALL ScatterChartTypeTemplate::getInfoHelper()
{
    return lcl_getInfoHelper();
}


// ____ XPropertySet ____
uno::Reference< beans::XPropertySetInfo > SAL_CALL
    ScatterChartTypeTemplate::getPropertySetInfo()
    throw (uno::RuntimeException)
{
    static uno::Reference< beans::XPropertySetInfo > xInfo;

    // /--
    MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
    if( !xInfo.is())
    {
        xInfo = ::cppu::OPropertySetHelper::createPropertySetInfo(
            getInfoHelper());
    }

    return xInfo;
    // \--
}

sal_Int32 ScatterChartTypeTemplate::getDimension() const
{
    return m_nDim;
}

StackMode ScatterChartTypeTemplate::getStackMode( sal_Int32 /* nChartTypeIndex */ ) const
{
    if( m_nDim == 3 )
        return StackMode_Z_STACKED;
    return StackMode_NONE;
}

bool ScatterChartTypeTemplate::supportsCategories() const
{
    return false;
}


void SAL_CALL ScatterChartTypeTemplate::applyStyle(
    const Reference< chart2::XDataSeries >& xSeries,
    ::sal_Int32 nChartTypeIndex,
    ::sal_Int32 nSeriesIndex,
    ::sal_Int32 nSeriesCount )
    throw (uno::RuntimeException)
{
    ChartTypeTemplate::applyStyle( xSeries, nChartTypeIndex, nSeriesIndex, nSeriesCount );

    try
    {
        Reference< beans::XPropertySet > xProp( xSeries, uno::UNO_QUERY_THROW );

        DataSeriesHelper::switchSymbolsOnOrOff( xProp, m_bHasSymbols, nSeriesIndex );
        DataSeriesHelper::switchLinesOnOrOff( xProp, m_bHasLines );
        DataSeriesHelper::makeLinesThickOrThin( xProp, m_nDim==2 );
    }
    catch( uno::Exception & ex )
    {
        ASSERT_EXCEPTION( ex );
    }
}

// ____ XChartTypeTemplate ____
Sequence< OUString > SAL_CALL ScatterChartTypeTemplate::getAvailableCreationParameterNames()
    throw (uno::RuntimeException)
{
    return Sequence< OUString >();
}

sal_Bool SAL_CALL ScatterChartTypeTemplate::matchesTemplate(
    const Reference< chart2::XDiagram >& xDiagram,
    sal_Bool bAdaptProperties )
    throw (uno::RuntimeException)
{
    sal_Bool bResult = ChartTypeTemplate::matchesTemplate( xDiagram, bAdaptProperties );

    // check symbol-style and line-style
    // for a template with symbols (or with lines) it is ok, if there is at least one series
    // with symbols (or with lines)
    if( bResult )
    {
        bool bSymbolFound = false;
        bool bLineFound = false;

        ::std::vector< Reference< chart2::XDataSeries > > aSeriesVec(
            DiagramHelper::getDataSeriesFromDiagram( xDiagram ));

        for( ::std::vector< Reference< chart2::XDataSeries > >::const_iterator aIt =
                 aSeriesVec.begin(); aIt != aSeriesVec.end(); ++aIt )
        {
            try
            {
                chart2::Symbol aSymbProp;
                drawing::LineStyle eLineStyle;
                Reference< beans::XPropertySet > xProp( *aIt, uno::UNO_QUERY_THROW );

                bool bCurrentHasSymbol = (xProp->getPropertyValue( C2U( "Symbol" )) >>= aSymbProp) &&
                    (aSymbProp.Style != chart2::SymbolStyle_NONE);

                if( bCurrentHasSymbol )
                    bSymbolFound = true;

                if( bCurrentHasSymbol && (!m_bHasSymbols) )
                {
                    bResult = false;
                    break;
                }

                bool bCurrentHasLine = (xProp->getPropertyValue( C2U( "LineStyle" )) >>= eLineStyle) &&
                    ( eLineStyle != drawing::LineStyle_NONE );

                if( bCurrentHasLine )
                    bLineFound = true;

                if( bCurrentHasLine && (!m_bHasLines) )
                {
                    bResult = false;
                    break;
                }
            }
            catch( uno::Exception & ex )
            {
                ASSERT_EXCEPTION( ex );
            }
        }

        if(bResult)
        {
            if( !bLineFound && m_bHasLines && bSymbolFound )
                bResult = false;
            else if( !bSymbolFound && m_bHasSymbols && bLineFound )
                bResult = false;
            else if( !bLineFound && !bSymbolFound )
                return m_bHasLines && m_bHasSymbols;
        }
    }

    // adapt curve style, spline order and resolution
    if( bResult && bAdaptProperties )
    {
        try
        {
            uno::Reference< beans::XPropertySet > xChartTypeProp(
                DiagramHelper::getChartTypeByIndex( xDiagram, 0 ),
                uno::UNO_QUERY_THROW );
            setFastPropertyValue_NoBroadcast( PROP_SCATTERCHARTTYPE_TEMPLATE_CURVE_STYLE, xChartTypeProp->getPropertyValue(C2U("CurveStyle" )) );
            setFastPropertyValue_NoBroadcast( PROP_SCATTERCHARTTYPE_TEMPLATE_CURVE_RESOLUTION, xChartTypeProp->getPropertyValue(C2U("CurveResolution" )) );
            setFastPropertyValue_NoBroadcast( PROP_SCATTERCHARTTYPE_TEMPLATE_SPLINE_ORDER, xChartTypeProp->getPropertyValue(C2U("SplineOrder" )) );
        }
        catch( uno::Exception & ex )
        {
            ASSERT_EXCEPTION( ex );
        }
    }

    return bResult;
}

Reference< chart2::XChartType > ScatterChartTypeTemplate::getChartTypeForIndex( sal_Int32 /*nChartTypeIndex*/ )
{
    Reference< chart2::XChartType > xResult;

    try
    {
        Reference< lang::XMultiServiceFactory > xFact(
            GetComponentContext()->getServiceManager(), uno::UNO_QUERY_THROW );
        xResult.set( xFact->createInstance(
                         CHART2_SERVICE_NAME_CHARTTYPE_SCATTER ), uno::UNO_QUERY_THROW );

        Reference< beans::XPropertySet > xCTProp( xResult, uno::UNO_QUERY );
        if( xCTProp.is())
        {
            xCTProp->setPropertyValue(
                C2U( "CurveStyle" ), getFastPropertyValue( PROP_SCATTERCHARTTYPE_TEMPLATE_CURVE_STYLE ));
            xCTProp->setPropertyValue(
                C2U( "CurveResolution" ), getFastPropertyValue( PROP_SCATTERCHARTTYPE_TEMPLATE_CURVE_RESOLUTION ));
            xCTProp->setPropertyValue(
                C2U( "SplineOrder" ), getFastPropertyValue( PROP_SCATTERCHARTTYPE_TEMPLATE_SPLINE_ORDER ));
        }
    }
    catch( uno::Exception & ex )
    {
        ASSERT_EXCEPTION( ex );
    }

    return xResult;
}

Reference< chart2::XChartType > SAL_CALL ScatterChartTypeTemplate::getChartTypeForNewSeries(
        const uno::Sequence< Reference< chart2::XChartType > >& aFormerlyUsedChartTypes )
    throw (uno::RuntimeException)
{
    Reference< chart2::XChartType > xResult;

    try
    {
        Reference< lang::XMultiServiceFactory > xFact(
            GetComponentContext()->getServiceManager(), uno::UNO_QUERY_THROW );
        xResult.set( xFact->createInstance(
                         CHART2_SERVICE_NAME_CHARTTYPE_SCATTER ), uno::UNO_QUERY_THROW );

        ChartTypeTemplate::copyPropertiesFromOldToNewCoordianteSystem( aFormerlyUsedChartTypes, xResult );

        Reference< beans::XPropertySet > xCTProp( xResult, uno::UNO_QUERY );
        if( xCTProp.is())
        {
            xCTProp->setPropertyValue(
                C2U( "CurveStyle" ), getFastPropertyValue( PROP_SCATTERCHARTTYPE_TEMPLATE_CURVE_STYLE ));
            xCTProp->setPropertyValue(
                C2U( "CurveResolution" ), getFastPropertyValue( PROP_SCATTERCHARTTYPE_TEMPLATE_CURVE_RESOLUTION ));
            xCTProp->setPropertyValue(
                C2U( "SplineOrder" ), getFastPropertyValue( PROP_SCATTERCHARTTYPE_TEMPLATE_SPLINE_ORDER ));
        }
    }
    catch( uno::Exception & ex )
    {
        ASSERT_EXCEPTION( ex );
    }

    return xResult;
}

Reference< chart2::XDataInterpreter > SAL_CALL ScatterChartTypeTemplate::getDataInterpreter()
    throw (uno::RuntimeException)
{
    if( ! m_xDataInterpreter.is())
        m_xDataInterpreter.set( new XYDataInterpreter( GetComponentContext()) );

    return m_xDataInterpreter;
}

// ----------------------------------------

Sequence< OUString > ScatterChartTypeTemplate::getSupportedServiceNames_Static()
{
    Sequence< OUString > aServices( 2 );
    aServices[ 0 ] = lcl_aServiceName;
    aServices[ 1 ] = C2U( "com.sun.star.chart2.ChartTypeTemplate" );
    return aServices;
}

// implement XServiceInfo methods basing upon getSupportedServiceNames_Static
APPHELPER_XSERVICEINFO_IMPL( ScatterChartTypeTemplate, lcl_aServiceName );

IMPLEMENT_FORWARD_XINTERFACE2( ScatterChartTypeTemplate, ChartTypeTemplate, OPropertySet )
IMPLEMENT_FORWARD_XTYPEPROVIDER2( ScatterChartTypeTemplate, ChartTypeTemplate, OPropertySet )

} //  namespace chart
