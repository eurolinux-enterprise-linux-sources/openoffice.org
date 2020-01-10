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
#include "ColumnLineChartTypeTemplate.hxx"
#include "macros.hxx"
#include "CommonConverters.hxx"
#include "DiagramHelper.hxx"
#include "DataSeriesHelper.hxx"
#include "servicenames_charttypes.hxx"
#include "ColumnLineDataInterpreter.hxx"
#include "ContainerHelper.hxx"
#include "PropertyHelper.hxx"
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/chart2/XChartTypeContainer.hpp>
#include <com/sun/star/chart2/XDataSeriesContainer.hpp>

#include <algorithm>

using namespace ::com::sun::star::chart2;
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

static const ::rtl::OUString lcl_aServiceName(
    RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.chart2.ColumnLineChartTypeTemplate" ));

enum
{
    PROP_COL_LINE_NUMBER_OF_LINES
};

void lcl_AddPropertiesToVector(
    ::std::vector< Property > & rOutProperties )
{
    rOutProperties.push_back(
        Property( C2U( "NumberOfLines" ),
                  PROP_COL_LINE_NUMBER_OF_LINES,
                  ::getCppuType( reinterpret_cast< const sal_Int32 * >(0)),
                  beans::PropertyAttribute::BOUND
                  | beans::PropertyAttribute::MAYBEDEFAULT ));
}

void lcl_AddDefaultsToMap(
    ::chart::tPropertyValueMap & rOutMap )
{
    ::chart::PropertyHelper::setPropertyValueDefault< sal_Int32 >( rOutMap, PROP_COL_LINE_NUMBER_OF_LINES, 1 );
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

ColumnLineChartTypeTemplate::ColumnLineChartTypeTemplate(
    Reference<
        uno::XComponentContext > const & xContext,
    const ::rtl::OUString & rServiceName,
    StackMode eStackMode,
    sal_Int32 nNumberOfLines ) :
        ChartTypeTemplate( xContext, rServiceName ),
        ::property::OPropertySet( m_aMutex ),
        m_eStackMode( eStackMode )
{
    setFastPropertyValue_NoBroadcast( PROP_COL_LINE_NUMBER_OF_LINES, uno::makeAny( nNumberOfLines ));
}

ColumnLineChartTypeTemplate::~ColumnLineChartTypeTemplate()
{}

// ____ OPropertySet ____
uno::Any ColumnLineChartTypeTemplate::GetDefaultValue( sal_Int32 nHandle ) const
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

::cppu::IPropertyArrayHelper & SAL_CALL ColumnLineChartTypeTemplate::getInfoHelper()
{
    return lcl_getInfoHelper();
}


// ____ XPropertySet ____
uno::Reference< beans::XPropertySetInfo > SAL_CALL
    ColumnLineChartTypeTemplate::getPropertySetInfo()
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

void ColumnLineChartTypeTemplate::createChartTypes(
    const Sequence< Sequence< Reference< XDataSeries > > > & aSeriesSeq,
    const Sequence< Reference< XCoordinateSystem > > & rCoordSys,
    const Sequence< Reference< XChartType > >& aOldChartTypesSeq )
{
    if( rCoordSys.getLength() == 0 ||
        ! rCoordSys[0].is() )
        return;

    try
    {
        Reference< lang::XMultiServiceFactory > xFact(
            GetComponentContext()->getServiceManager(), uno::UNO_QUERY_THROW );
        Sequence< Reference< XDataSeries > > aFlatSeriesSeq( FlattenSequence( aSeriesSeq ));
        sal_Int32 nNumberOfSeries = aFlatSeriesSeq.getLength();
        sal_Int32 nNumberOfLines = 0;
        sal_Int32 nNumberOfColumns = 0;

        getFastPropertyValue( PROP_COL_LINE_NUMBER_OF_LINES ) >>= nNumberOfLines;
        OSL_ENSURE( nNumberOfLines>=0, "number of lines should be not negative" );
        if( nNumberOfLines < 0 )
            nNumberOfLines = 0;

        if( nNumberOfLines >= nNumberOfSeries )
        {
            if( nNumberOfSeries > 0 )
            {
                nNumberOfLines = nNumberOfSeries - 1;
                nNumberOfColumns = 1;
            }
            else
                nNumberOfLines = 0;
        }
        else
            nNumberOfColumns = nNumberOfSeries - nNumberOfLines;

        // Columns
        // -------
        Reference< XChartType > xCT(
            xFact->createInstance( CHART2_SERVICE_NAME_CHARTTYPE_COLUMN ), uno::UNO_QUERY_THROW );

        ChartTypeTemplate::copyPropertiesFromOldToNewCoordianteSystem( aOldChartTypesSeq, xCT );

        Reference< XChartTypeContainer > xCTCnt( rCoordSys[ 0 ], uno::UNO_QUERY_THROW );
        xCTCnt->setChartTypes( Sequence< Reference< chart2::XChartType > >( &xCT, 1 ));

        if( nNumberOfColumns > 0 )
        {
            Reference< XDataSeriesContainer > xDSCnt( xCT, uno::UNO_QUERY_THROW );
            Sequence< Reference< XDataSeries > > aColumnSeq( nNumberOfColumns );
            ::std::copy( aFlatSeriesSeq.getConstArray(),
                         aFlatSeriesSeq.getConstArray() + nNumberOfColumns,
                         aColumnSeq.getArray());
            xDSCnt->setDataSeries( aColumnSeq );
        }

        // Lines
        // -----
        xCT.set( xFact->createInstance( CHART2_SERVICE_NAME_CHARTTYPE_LINE ), uno::UNO_QUERY_THROW );
        xCTCnt.set( rCoordSys[ 0 ], uno::UNO_QUERY_THROW );
        xCTCnt->addChartType( xCT );

        if( nNumberOfLines > 0 )
        {
            Reference< XDataSeriesContainer > xDSCnt( xCT, uno::UNO_QUERY_THROW );
            Sequence< Reference< XDataSeries > > aLineSeq( nNumberOfLines );
            ::std::copy( aFlatSeriesSeq.getConstArray() + nNumberOfColumns,
                         aFlatSeriesSeq.getConstArray() + aFlatSeriesSeq.getLength(),
                         aLineSeq.getArray());
            xDSCnt->setDataSeries( aLineSeq );
        }
    }
    catch( uno::Exception & ex )
    {
        ASSERT_EXCEPTION( ex );
    }
}

void SAL_CALL ColumnLineChartTypeTemplate::applyStyle(
    const Reference< chart2::XDataSeries >& xSeries,
    ::sal_Int32 nChartTypeIndex,
    ::sal_Int32 nSeriesIndex,
    ::sal_Int32 nSeriesCount )
    throw (uno::RuntimeException)
{
    ChartTypeTemplate::applyStyle( xSeries, nChartTypeIndex, nSeriesIndex, nSeriesCount );

    if( nChartTypeIndex==1 ) // lines
    {
        Reference< beans::XPropertySet > xProp( xSeries, uno::UNO_QUERY );
        if( xProp.is() )
        {
            DataSeriesHelper::switchLinesOnOrOff( xProp, true );
            DataSeriesHelper::switchSymbolsOnOrOff( xProp, false, nSeriesIndex );
            DataSeriesHelper::makeLinesThickOrThin( xProp, true );
        }
    }
}

StackMode ColumnLineChartTypeTemplate::getStackMode( sal_Int32 nChartTypeIndex ) const
{
    if( nChartTypeIndex == 0 )
        return m_eStackMode;
    return StackMode_NONE;
}

// ____ XChartTypeTemplate ____
sal_Bool SAL_CALL ColumnLineChartTypeTemplate::matchesTemplate(
    const uno::Reference< XDiagram >& xDiagram,
    sal_Bool bAdaptProperties )
    throw (uno::RuntimeException)
{
    sal_Bool bResult = sal_False;

    if( ! xDiagram.is())
        return bResult;

    try
    {
        Reference< chart2::XChartType > xColumnChartType;
        Reference< XCoordinateSystem > xColumnChartCooSys;
        Reference< chart2::XChartType > xLineChartType;
        sal_Int32 nNumberOfChartTypes = 0;

        Reference< XCoordinateSystemContainer > xCooSysCnt(
            xDiagram, uno::UNO_QUERY_THROW );
        Sequence< Reference< XCoordinateSystem > > aCooSysSeq(
            xCooSysCnt->getCoordinateSystems());
        for( sal_Int32 i=0; i<aCooSysSeq.getLength(); ++i )
        {
            Reference< XChartTypeContainer > xCTCnt( aCooSysSeq[i], uno::UNO_QUERY_THROW );
            Sequence< Reference< XChartType > > aChartTypeSeq( xCTCnt->getChartTypes());
            for( sal_Int32 j=0; j<aChartTypeSeq.getLength(); ++j )
            {
                if( aChartTypeSeq[j].is())
                {
                    ++nNumberOfChartTypes;
                    if( nNumberOfChartTypes > 2 )
                        break;
                    OUString aCTService = aChartTypeSeq[j]->getChartType();
                    if( aCTService.equals( CHART2_SERVICE_NAME_CHARTTYPE_COLUMN ))
                    {
                        xColumnChartType.set( aChartTypeSeq[j] );
                        xColumnChartCooSys.set( aCooSysSeq[i] );
                    }
                    else if( aCTService.equals( CHART2_SERVICE_NAME_CHARTTYPE_LINE ))
                        xLineChartType.set( aChartTypeSeq[j] );
                }
            }
            if( nNumberOfChartTypes > 2 )
                break;
        }

        if( nNumberOfChartTypes == 2 &&
            xColumnChartType.is() &&
            xLineChartType.is())
        {
            OSL_ASSERT( xColumnChartCooSys.is());

            // check stackmode of bars
            bResult = (xColumnChartCooSys->getDimension() == getDimension());
            if( bResult )
            {
                bool bFound=false;
                bool bAmbiguous=false;
                bResult = ( DiagramHelper::getStackModeFromChartType(
                                xColumnChartType, bFound, bAmbiguous,
                                xColumnChartCooSys )
                            == getStackMode( 0 ) );

                if( bResult && bAdaptProperties )
                {
                    Reference< XDataSeriesContainer > xSeriesContainer( xLineChartType, uno::UNO_QUERY );
                    if( xSeriesContainer.is() )
                    {
                        sal_Int32 nNumberOfLines = xSeriesContainer->getDataSeries().getLength();
                        setFastPropertyValue_NoBroadcast( PROP_COL_LINE_NUMBER_OF_LINES, uno::makeAny( nNumberOfLines ));
                    }
                }
            }
        }
    }
    catch( uno::Exception & ex )
    {
        ASSERT_EXCEPTION( ex );
    }

    return bResult;
}

Reference< chart2::XChartType > ColumnLineChartTypeTemplate::getChartTypeForIndex( sal_Int32 nChartTypeIndex )
{
    Reference< chart2::XChartType > xCT;
    Reference< lang::XMultiServiceFactory > xFact(
            GetComponentContext()->getServiceManager(), uno::UNO_QUERY );
    if(xFact.is())
    {
        if( nChartTypeIndex == 0 )
            xCT.set( xFact->createInstance( CHART2_SERVICE_NAME_CHARTTYPE_COLUMN ), uno::UNO_QUERY );
        else
            xCT.set( xFact->createInstance( CHART2_SERVICE_NAME_CHARTTYPE_LINE ), uno::UNO_QUERY );
    }
    return xCT;
}

Reference< XChartType > SAL_CALL ColumnLineChartTypeTemplate::getChartTypeForNewSeries(
        const uno::Sequence< Reference< chart2::XChartType > >& aFormerlyUsedChartTypes )
    throw (uno::RuntimeException)
{
    Reference< chart2::XChartType > xResult;

    try
    {
        Reference< lang::XMultiServiceFactory > xFact(
            GetComponentContext()->getServiceManager(), uno::UNO_QUERY_THROW );
        xResult.set( xFact->createInstance(
                         CHART2_SERVICE_NAME_CHARTTYPE_LINE ), uno::UNO_QUERY_THROW );
        ChartTypeTemplate::copyPropertiesFromOldToNewCoordianteSystem( aFormerlyUsedChartTypes, xResult );
    }
    catch( uno::Exception & ex )
    {
        ASSERT_EXCEPTION( ex );
    }

    return xResult;
}

Reference< XDataInterpreter > SAL_CALL ColumnLineChartTypeTemplate::getDataInterpreter()
    throw (uno::RuntimeException)
{
    if( ! m_xDataInterpreter.is())
    {
        sal_Int32 nNumberOfLines = 1;
        getFastPropertyValue( PROP_COL_LINE_NUMBER_OF_LINES ) >>= nNumberOfLines;
        m_xDataInterpreter.set( new ColumnLineDataInterpreter( nNumberOfLines, GetComponentContext() ) );
    }
    else
    {
        //todo...
        OSL_ENSURE( false, "number of lines may not be valid anymore in the datainterpreter" );
    
    }

    return m_xDataInterpreter;
}

// ----------------------------------------

uno::Sequence< ::rtl::OUString > ColumnLineChartTypeTemplate::getSupportedServiceNames_Static()
{
    uno::Sequence< ::rtl::OUString > aServices( 2 );
    aServices[ 0 ] = lcl_aServiceName;
    aServices[ 1 ] = C2U( "com.sun.star.chart2.ChartTypeTemplate" );
    return aServices;
}

// implement XServiceInfo methods basing upon getSupportedServiceNames_Static
APPHELPER_XSERVICEINFO_IMPL( ColumnLineChartTypeTemplate, lcl_aServiceName );

IMPLEMENT_FORWARD_XINTERFACE2( ColumnLineChartTypeTemplate, ChartTypeTemplate, OPropertySet )
IMPLEMENT_FORWARD_XTYPEPROVIDER2( ColumnLineChartTypeTemplate, ChartTypeTemplate, OPropertySet )

} //  namespace chart
