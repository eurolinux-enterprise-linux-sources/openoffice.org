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
#include "ChartController.hxx"
#include "ChartWindow.hxx"
#include "chartview/DrawModelWrapper.hxx"
#include "ObjectIdentifier.hxx"
#include "chartview/ExplicitValueProvider.hxx"
#include "macros.hxx"
#include "dlg_ObjectProperties.hxx"
#include "dlg_View3D.hxx"
#include "dlg_InsertErrorBars.hxx"
#include "ViewElementListProvider.hxx"
#include "DataPointItemConverter.hxx"
#include "AxisItemConverter.hxx"
#include "MultipleChartConverters.hxx"
#include "TitleItemConverter.hxx"
#include "LegendItemConverter.hxx"
#include "RegressionCurveItemConverter.hxx"
#include "RegressionEquationItemConverter.hxx"
#include "ErrorBarItemConverter.hxx"
#include "ChartModelHelper.hxx"
#include "AxisHelper.hxx"
#include "TitleHelper.hxx"
#include "LegendHelper.hxx"
#include "ChartTypeHelper.hxx"
#include "ColorPerPointHelper.hxx"
#include "DiagramHelper.hxx"
#include "servicenames_charttypes.hxx"
#include "ControllerLockGuard.hxx"
#include "UndoGuard.hxx"
#include "ObjectNameProvider.hxx"
#include "ResId.hxx"
#include "Strings.hrc"
#include "ReferenceSizeProvider.hxx"
#include "RegressionCurveHelper.hxx"
#include <com/sun/star/chart2/XChartDocument.hpp>

//for auto_ptr
#include <memory>

// header for define RET_OK
#include <vcl/msgbox.hxx>
// for SolarMutex
#include <vcl/svapp.hxx>
#include <vos/mutex.hxx>
#include <svx/ActionDescriptionProvider.hxx>

//.............................................................................
namespace chart
{
//.............................................................................
using namespace ::com::sun::star;
using namespace ::com::sun::star::chart2;
using ::com::sun::star::uno::Reference;
using ::rtl::OUString;

namespace
{

::comphelper::ItemConverter* createItemConverter(
    const ::rtl::OUString & aObjectCID
    , const uno::Reference< frame::XModel > & xChartModel
    , const uno::Reference< uno::XComponentContext > & xContext
    , SdrModel & rDrawModel
    , NumberFormatterWrapper * pNumberFormatterWrapper = NULL
    , ExplicitValueProvider * pExplicitValueProvider = NULL
    , ::std::auto_ptr< ReferenceSizeProvider > pRefSizeProvider =
          ::std::auto_ptr< ReferenceSizeProvider >()
    )
{
    ::comphelper::ItemConverter* pItemConverter=NULL;

    //-------------------------------------------------------------
    //get type of selected object
    ObjectType eObjectType = ObjectIdentifier::getObjectType( aObjectCID );
    if( OBJECTTYPE_UNKNOWN==eObjectType )
    {
        DBG_ERROR("unknown ObjectType");
        return NULL;
    }
    //--
    rtl::OUString aParticleID = ObjectIdentifier::getParticleID( aObjectCID );
    bool bAffectsMultipleObjects = aParticleID.equals(C2U("ALLELEMENTS"));
    //-------------------------------------------------------------
    if( !bAffectsMultipleObjects )
    {
        uno::Reference< beans::XPropertySet > xObjectProperties =
            ObjectIdentifier::getObjectPropertySet( aObjectCID, xChartModel );
        if(!xObjectProperties.is())
            return NULL;
        //create itemconverter for a single object
        switch(eObjectType)
        {
            case OBJECTTYPE_PAGE:
                pItemConverter =  new wrapper::GraphicPropertyItemConverter(
                                        xObjectProperties, rDrawModel.GetItemPool(),
                                        rDrawModel, uno::Reference< lang::XMultiServiceFactory >( xChartModel, uno::UNO_QUERY ),
                                        wrapper::GraphicPropertyItemConverter::LINE_AND_FILL_PROPERTIES );
                    break;
            case OBJECTTYPE_TITLE:
            {
                ::std::auto_ptr< awt::Size > pRefSize;
                if( pRefSizeProvider.get() )
                    pRefSize.reset( new awt::Size( pRefSizeProvider->getPageSize()));

                pItemConverter = new wrapper::TitleItemConverter( xObjectProperties,
                                                                  rDrawModel.GetItemPool(), rDrawModel,
                                                                  uno::Reference< lang::XMultiServiceFactory >( xChartModel, uno::UNO_QUERY ),
                                                                  pRefSize );
            }
            break;
            case OBJECTTYPE_LEGEND:
            {
                ::std::auto_ptr< awt::Size > pRefSize;
                if( pRefSizeProvider.get() )
                    pRefSize.reset( new awt::Size( pRefSizeProvider->getPageSize()));

                pItemConverter = new wrapper::LegendItemConverter( xObjectProperties,
                                                                   rDrawModel.GetItemPool(), rDrawModel,
                                                                   uno::Reference< lang::XMultiServiceFactory >( xChartModel, uno::UNO_QUERY ),
                                                                   pRefSize );
            }
            break;
            case OBJECTTYPE_LEGEND_ENTRY:
                    break;
            case OBJECTTYPE_DIAGRAM:
                    break;
            case OBJECTTYPE_DIAGRAM_WALL:
            case OBJECTTYPE_DIAGRAM_FLOOR:
                pItemConverter =  new wrapper::GraphicPropertyItemConverter(
                                        xObjectProperties, rDrawModel.GetItemPool(),
                                        rDrawModel, uno::Reference< lang::XMultiServiceFactory >( xChartModel, uno::UNO_QUERY ),
                                        wrapper::GraphicPropertyItemConverter::LINE_AND_FILL_PROPERTIES );
                    break;
            case OBJECTTYPE_AXIS:
            {
                ::std::auto_ptr< awt::Size > pRefSize;
                if( pRefSizeProvider.get() )
                    pRefSize.reset( new awt::Size( pRefSizeProvider->getPageSize()));

                uno::Reference< beans::XPropertySet > xDiaProp;
                xDiaProp.set( ChartModelHelper::findDiagram( xChartModel ), uno::UNO_QUERY );

                // the second property set contains the property CoordinateOrigin
                // nOriginIndex is the index of the corresponding index of the
                // origin (x=0, y=1, z=2)

                ExplicitScaleData aExplicitScale;
                ExplicitIncrementData aExplicitIncrement;
                if( pExplicitValueProvider )
                    pExplicitValueProvider->getExplicitValuesForAxis(
                        uno::Reference< XAxis >( xObjectProperties, uno::UNO_QUERY ),
                        aExplicitScale, aExplicitIncrement );

                pItemConverter =  new wrapper::AxisItemConverter(
                    xObjectProperties, rDrawModel.GetItemPool(),
                    rDrawModel,
                    uno::Reference< chart2::XChartDocument >( xChartModel, uno::UNO_QUERY ),
                    &aExplicitScale, &aExplicitIncrement,
                    pRefSize );
            }
            break;
            case OBJECTTYPE_AXIS_UNITLABEL:
                    break;
            case OBJECTTYPE_DATA_LABELS:
            case OBJECTTYPE_DATA_SERIES:
            case OBJECTTYPE_DATA_LABEL:
            case OBJECTTYPE_DATA_POINT:
            {
                ::std::auto_ptr< awt::Size > pRefSize;
                if( pRefSizeProvider.get() )
                    pRefSize.reset( new awt::Size( pRefSizeProvider->getPageSize()));

                wrapper::GraphicPropertyItemConverter::eGraphicObjectType eMapTo =
                    wrapper::GraphicPropertyItemConverter::FILLED_DATA_POINT;

                uno::Reference< XDataSeries > xSeries = ObjectIdentifier::getDataSeriesForCID( aObjectCID, xChartModel );
	            uno::Reference< XChartType > xChartType = ChartModelHelper::getChartTypeOfSeries( xChartModel, xSeries );

                uno::Reference< XDiagram > xDiagram( ChartModelHelper::findDiagram( xChartModel ) );
                sal_Int32 nDimensionCount = DiagramHelper::getDimension( xDiagram );
                if( !ChartTypeHelper::isSupportingAreaProperties( xChartType, nDimensionCount ) )
                    eMapTo = wrapper::GraphicPropertyItemConverter::LINE_DATA_POINT;
                /*
                FILLED_DATA_POINT,
                LINE_DATA_POINT,
                LINE_PROPERTIES,
                FILL_PROPERTIES,
                LINE_AND_FILL_PROPERTIES
                */
                bool bDataSeries = ( eObjectType == OBJECTTYPE_DATA_SERIES || eObjectType == OBJECTTYPE_DATA_LABELS );

                //special color for pie chart:
                bool bUseSpecialFillColor = false;
                sal_Int32 nSpecialFillColor =0;
                sal_Int32 nPointIndex = -1; /*-1 for whole series*/
                if(!bDataSeries)
                {
                    nPointIndex = aParticleID.toInt32();
                    uno::Reference< beans::XPropertySet > xSeriesProp( xSeries, uno::UNO_QUERY );
                    bool bVaryColorsByPoint = false;
                    if( xSeriesProp.is() &&
                        (xSeriesProp->getPropertyValue(C2U("VaryColorsByPoint")) >>= bVaryColorsByPoint) &&
                        bVaryColorsByPoint )
                    {
                        if( !ColorPerPointHelper::hasPointOwnColor( xSeriesProp, nPointIndex, xObjectProperties ) )
                        {
                            bUseSpecialFillColor = true;
                            OSL_ASSERT( xDiagram.is());
                            uno::Reference< XColorScheme > xColorScheme( xDiagram->getDefaultColorScheme() );
                            if( xColorScheme.is())
                                nSpecialFillColor = xColorScheme->getColorByIndex( nPointIndex );
                        }
                    }
                }
                sal_Int32 nNumberFormat=ExplicitValueProvider::getExplicitNumberFormatKeyForDataLabel( xObjectProperties, xSeries, nPointIndex, xDiagram );
                sal_Int32 nPercentNumberFormat=ExplicitValueProvider::getExplicitPercentageNumberFormatKeyForDataLabel(
                        xObjectProperties,uno::Reference< util::XNumberFormatsSupplier >(xChartModel, uno::UNO_QUERY));
                
                pItemConverter =  new wrapper::DataPointItemConverter( xChartModel, xContext,
                                        xObjectProperties, xSeries, rDrawModel.GetItemPool(), rDrawModel,
                                        pNumberFormatterWrapper,
                                        uno::Reference< lang::XMultiServiceFactory >( xChartModel, uno::UNO_QUERY ),
                                        eMapTo, pRefSize, bDataSeries, bUseSpecialFillColor, nSpecialFillColor, false,
                                        nNumberFormat, nPercentNumberFormat );
                    break;
            }
            case OBJECTTYPE_GRID:
            case OBJECTTYPE_SUBGRID:
            case OBJECTTYPE_DATA_AVERAGE_LINE:
                pItemConverter =  new wrapper::GraphicPropertyItemConverter(
                                        xObjectProperties, rDrawModel.GetItemPool(),
                                        rDrawModel, uno::Reference< lang::XMultiServiceFactory >( xChartModel, uno::UNO_QUERY ),
                                        wrapper::GraphicPropertyItemConverter::LINE_PROPERTIES );
                    break;

            case OBJECTTYPE_DATA_ERRORS:
                pItemConverter =  new wrapper::ErrorBarItemConverter(
                    xChartModel, xObjectProperties, rDrawModel.GetItemPool(),
                    rDrawModel, uno::Reference< lang::XMultiServiceFactory >( xChartModel, uno::UNO_QUERY ));
                break;

            case OBJECTTYPE_DATA_CURVE:
                pItemConverter =  new wrapper::RegressionCurveItemConverter(
                    xObjectProperties, uno::Reference< chart2::XRegressionCurveContainer >(
                        ObjectIdentifier::getDataSeriesForCID( aObjectCID, xChartModel ), uno::UNO_QUERY ),
                    rDrawModel.GetItemPool(), rDrawModel,
                    uno::Reference< lang::XMultiServiceFactory >( xChartModel, uno::UNO_QUERY ));
                break;
            case OBJECTTYPE_DATA_CURVE_EQUATION:
            {
                ::std::auto_ptr< awt::Size > pRefSize;
                if( pRefSizeProvider.get() )
                    pRefSize.reset( new awt::Size( pRefSizeProvider->getPageSize()));

                pItemConverter =  new wrapper::RegressionEquationItemConverter(
                                        xObjectProperties, rDrawModel.GetItemPool(), rDrawModel,
                                        uno::Reference< lang::XMultiServiceFactory >( xChartModel, uno::UNO_QUERY ),
                                        pRefSize );
                    break;
            }
            case OBJECTTYPE_DATA_ERRORS_X:
                    break;
            case OBJECTTYPE_DATA_ERRORS_Y:
                    break;
            case OBJECTTYPE_DATA_ERRORS_Z:
                    break;
            case OBJECTTYPE_DATA_STOCK_RANGE:
                    break;
            case OBJECTTYPE_DATA_STOCK_LOSS:
            case OBJECTTYPE_DATA_STOCK_GAIN:
                pItemConverter =  new wrapper::GraphicPropertyItemConverter(
                                        xObjectProperties, rDrawModel.GetItemPool(),
                                        rDrawModel, uno::Reference< lang::XMultiServiceFactory >( xChartModel, uno::UNO_QUERY ),
                                        wrapper::GraphicPropertyItemConverter::LINE_AND_FILL_PROPERTIES );
                    break;
            default: //OBJECTTYPE_UNKNOWN
                    break;
        }
    }
    else
    {
        //create itemconverter for a all objects of given type
        switch(eObjectType)
        {
            case OBJECTTYPE_TITLE:
                pItemConverter =  new wrapper::AllTitleItemConverter( xChartModel, rDrawModel.GetItemPool(),
                                                                     rDrawModel, uno::Reference< lang::XMultiServiceFactory >( xChartModel, uno::UNO_QUERY ));
                break;
            case OBJECTTYPE_AXIS:
            {
                ::std::auto_ptr< awt::Size > pRefSize;
                if( pRefSizeProvider.get() )
                    pRefSize.reset( new awt::Size( pRefSizeProvider->getPageSize()));

                pItemConverter =  new wrapper::AllAxisItemConverter( xChartModel, rDrawModel.GetItemPool(),
                                                                     rDrawModel, uno::Reference< lang::XMultiServiceFactory >( xChartModel, uno::UNO_QUERY ), pRefSize );
            }
            break;
            case OBJECTTYPE_GRID:
            case OBJECTTYPE_SUBGRID:
                pItemConverter =  new wrapper::AllGridItemConverter( xChartModel, rDrawModel.GetItemPool(),
                                                                     rDrawModel, uno::Reference< lang::XMultiServiceFactory >( xChartModel, uno::UNO_QUERY ));
                break;
            default: //for this type it is not supported to change all elements at once
                break;
        }

    }
    return pItemConverter;
}

rtl::OUString lcl_getTitleCIDForCommand( const ::rtl::OString& rDispatchCommand, const uno::Reference< frame::XModel > & xChartModel )
{
    if( rDispatchCommand.equals("AllTitles"))
        return ObjectIdentifier::createClassifiedIdentifier( OBJECTTYPE_TITLE, C2U("ALLELEMENTS") );

    TitleHelper::eTitleType nTitleType( TitleHelper::MAIN_TITLE );
    if( rDispatchCommand.equals("SubTitle") )
        nTitleType = TitleHelper::SUB_TITLE;
    else if( rDispatchCommand.equals("XTitle") )
        nTitleType = TitleHelper::X_AXIS_TITLE;
    else if( rDispatchCommand.equals("YTitle") )
        nTitleType = TitleHelper::Y_AXIS_TITLE;
    else if( rDispatchCommand.equals("ZTitle") )
        nTitleType = TitleHelper::Z_AXIS_TITLE;
    else if( rDispatchCommand.equals("SecondaryXTitle") )
        nTitleType = TitleHelper::SECONDARY_X_AXIS_TITLE;
    else if( rDispatchCommand.equals("SecondaryYTitle") )
        nTitleType = TitleHelper::SECONDARY_Y_AXIS_TITLE;
  
    uno::Reference< XTitle > xTitle( TitleHelper::getTitle( nTitleType, xChartModel ) );
    return ObjectIdentifier::createClassifiedIdentifierForObject( xTitle, xChartModel );
}

rtl::OUString lcl_getAxisCIDForCommand( const ::rtl::OString& rDispatchCommand, const uno::Reference< frame::XModel >& xChartModel )
{
    if( rDispatchCommand.equals("DiagramAxisAll"))
        return ObjectIdentifier::createClassifiedIdentifier( OBJECTTYPE_AXIS, C2U("ALLELEMENTS") );

    sal_Int32   nDimensionIndex=0;
    bool        bMainAxis=true;
    if( rDispatchCommand.equals("DiagramAxisX"))
    {
        nDimensionIndex=0; bMainAxis=true;
    }
    else if( rDispatchCommand.equals("DiagramAxisY"))
    {
        nDimensionIndex=1; bMainAxis=true; 
    }
    else if( rDispatchCommand.equals("DiagramAxisZ"))
    {
        nDimensionIndex=2; bMainAxis=true;
    }
    else if( rDispatchCommand.equals("DiagramAxisA"))
    {
        nDimensionIndex=0; bMainAxis=false;
    }
    else if( rDispatchCommand.equals("DiagramAxisB"))
    {
        nDimensionIndex=1; bMainAxis=false;     
    }

    uno::Reference< XDiagram > xDiagram( ChartModelHelper::findDiagram( xChartModel ) );
    uno::Reference< XAxis > xAxis( AxisHelper::getAxis( nDimensionIndex, bMainAxis, xDiagram ) );
    return ObjectIdentifier::createClassifiedIdentifierForObject( xAxis, xChartModel );
}

rtl::OUString lcl_getGridCIDForCommand( const ::rtl::OString& rDispatchCommand, const uno::Reference< frame::XModel >& xChartModel )
{
    uno::Reference< XDiagram > xDiagram( ChartModelHelper::findDiagram( xChartModel ) );

    if( rDispatchCommand.equals("DiagramGridAll"))
        return ObjectIdentifier::createClassifiedIdentifier( OBJECTTYPE_GRID, C2U("ALLELEMENTS") );

    sal_Int32   nDimensionIndex=0;
    bool        bMainGrid=true;

    //x and y is swapped in the commands

    if( rDispatchCommand.equals("DiagramGridYMain"))
    {
        nDimensionIndex=0; bMainGrid=true;
    }
    else if( rDispatchCommand.equals("DiagramGridXMain"))
    {
        nDimensionIndex=1; bMainGrid=true;
    }
    else if( rDispatchCommand.equals("DiagramGridZMain"))
    {
        nDimensionIndex=2; bMainGrid=true;
    }
    else if( rDispatchCommand.equals("DiagramGridYHelp"))
    {
        nDimensionIndex=0; bMainGrid=false;
    }
    else if( rDispatchCommand.equals("DiagramGridXHelp"))
    {
        nDimensionIndex=1; bMainGrid=false;
    }
    else if( rDispatchCommand.equals("DiagramGridZHelp"))
    {
        nDimensionIndex=2; bMainGrid=false;
    }

    bool bMainAxis = true;
    uno::Reference< XAxis > xAxis( AxisHelper::getAxis( nDimensionIndex, bMainAxis, xDiagram ) );

    sal_Int32   nSubGridIndex= bMainGrid ? (-1) : 0;
    rtl::OUString aCID( ObjectIdentifier::createClassifiedIdentifierForGrid( xAxis, xChartModel, nSubGridIndex ) );
    return aCID;
}
rtl::OUString lcl_getObjectCIDForCommand( const ::rtl::OString& rDispatchCommand, const uno::Reference< XChartDocument > & xChartDocument, const rtl::OUString& rSelectedCID )
{
    ObjectType eObjectType = OBJECTTYPE_UNKNOWN;
    rtl::OUString aParticleID;

    uno::Reference< frame::XModel > xChartModel( xChartDocument, uno::UNO_QUERY );
    const ObjectType eSelectedType = ObjectIdentifier::getObjectType( rSelectedCID );
    uno::Reference< XDataSeries > xSeries = ObjectIdentifier::getDataSeriesForCID( rSelectedCID, xChartModel );
    uno::Reference< chart2::XRegressionCurveContainer > xRegCurveCnt( xSeries, uno::UNO_QUERY );

    //-------------------------------------------------------------------------
    //legend
    if( rDispatchCommand.equals("Legend") || rDispatchCommand.equals("FormatLegend") )
    {
        eObjectType = OBJECTTYPE_LEGEND;
        //@todo set particular aParticleID if we have more than one legend
    }
    //-------------------------------------------------------------------------
    //wall floor area
    else if( rDispatchCommand.equals("DiagramWall") || rDispatchCommand.equals("FormatWall") )
    {
        //OBJECTTYPE_DIAGRAM;
        eObjectType = OBJECTTYPE_DIAGRAM_WALL;
        //@todo set particular aParticleID if we have more than one diagram
    }
    else if( rDispatchCommand.equals("DiagramFloor") || rDispatchCommand.equals("FormatFloor") )
    {
        eObjectType = OBJECTTYPE_DIAGRAM_FLOOR;
        //@todo set particular aParticleID if we have more than one diagram
    }
    else if( rDispatchCommand.equals("DiagramArea") || rDispatchCommand.equals("FormatChartArea") )
    {
        eObjectType = OBJECTTYPE_PAGE;
    }
    //-------------------------------------------------------------------------
    //title
    else if( rDispatchCommand.equals("MainTitle")
        || rDispatchCommand.equals("SubTitle")
        || rDispatchCommand.equals("XTitle")
        || rDispatchCommand.equals("YTitle")
        || rDispatchCommand.equals("ZTitle")
        || rDispatchCommand.equals("SecondaryXTitle")
        || rDispatchCommand.equals("SecondaryYTitle")
        || rDispatchCommand.equals("AllTitles")
        )
    {
        return lcl_getTitleCIDForCommand( rDispatchCommand, xChartModel );
    }
    //-------------------------------------------------------------------------
    //axis
    else if( rDispatchCommand.equals("DiagramAxisX")
        || rDispatchCommand.equals("DiagramAxisY")
        || rDispatchCommand.equals("DiagramAxisZ")
        || rDispatchCommand.equals("DiagramAxisA")
        || rDispatchCommand.equals("DiagramAxisB")
        || rDispatchCommand.equals("DiagramAxisAll")
        )
    {
        return lcl_getAxisCIDForCommand( rDispatchCommand, xChartModel );
    }
    //-------------------------------------------------------------------------
    //grid
    else if( rDispatchCommand.equals("DiagramGridYMain")
        || rDispatchCommand.equals("DiagramGridXMain")
        || rDispatchCommand.equals("DiagramGridZMain")
        || rDispatchCommand.equals("DiagramGridYHelp")
        || rDispatchCommand.equals("DiagramGridXHelp")
        || rDispatchCommand.equals("DiagramGridZHelp")
        || rDispatchCommand.equals("DiagramGridAll")
        )
    {
        return lcl_getGridCIDForCommand( rDispatchCommand, xChartModel );
    }
    //-------------------------------------------------------------------------
    //data series
    else if( rDispatchCommand.equals("FormatDataSeries") )
    {
        if( eSelectedType == OBJECTTYPE_DATA_SERIES )
            return rSelectedCID;
        else
            return ObjectIdentifier::createClassifiedIdentifier(
                OBJECTTYPE_DATA_SERIES, ObjectIdentifier::getSeriesParticleFromCID( rSelectedCID ) );
    }
    //-------------------------------------------------------------------------
    //data point
    else if( rDispatchCommand.equals("FormatDataPoint") )
    {
        return rSelectedCID;
    }
    //-------------------------------------------------------------------------
    //data labels
    else if( rDispatchCommand.equals("FormatDataLabels") )
    {
        if( eSelectedType == OBJECTTYPE_DATA_LABELS )
            return rSelectedCID;
        else
            return ObjectIdentifier::createClassifiedIdentifierWithParent(
                OBJECTTYPE_DATA_LABELS, ::rtl::OUString(), rSelectedCID );
    }
    //-------------------------------------------------------------------------
    //data labels
    else if( rDispatchCommand.equals("FormatDataLabel") )
    {
        if( eSelectedType == OBJECTTYPE_DATA_LABEL )
            return rSelectedCID;
        else
        {
            sal_Int32 nPointIndex = ObjectIdentifier::getParticleID( rSelectedCID ).toInt32();
            if( nPointIndex>=0 )
            {
                OUString aSeriesParticle = ObjectIdentifier::getSeriesParticleFromCID( rSelectedCID );
                OUString aChildParticle( ObjectIdentifier::getStringForType( OBJECTTYPE_DATA_LABELS ) );
                aChildParticle+=(C2U("="));
                OUString aLabelsCID = ObjectIdentifier::createClassifiedIdentifierForParticles( aSeriesParticle, aChildParticle );
                OUString aLabelCID_Stub = ObjectIdentifier::createClassifiedIdentifierWithParent(
                    OBJECTTYPE_DATA_LABEL, ::rtl::OUString(), aLabelsCID );

                return ObjectIdentifier::createPointCID( aLabelCID_Stub, nPointIndex );
            }
        }
    }
    //-------------------------------------------------------------------------
    //mean value line
    else if( rDispatchCommand.equals("FormatMeanValue") )
    {
        if( eSelectedType == OBJECTTYPE_DATA_AVERAGE_LINE )
            return rSelectedCID;
        else
            return ObjectIdentifier::createDataCurveCID(
                ObjectIdentifier::getSeriesParticleFromCID( rSelectedCID ),
                    RegressionCurveHelper::getRegressionCurveIndex( xRegCurveCnt,
                        RegressionCurveHelper::getMeanValueLine( xRegCurveCnt ) ), true );
    }
    //-------------------------------------------------------------------------
    //trend line
    else if( rDispatchCommand.equals("FormatTrendline") )
    {
        if( eSelectedType == OBJECTTYPE_DATA_CURVE )
            return rSelectedCID;
        else
            return ObjectIdentifier::createDataCurveCID(
                ObjectIdentifier::getSeriesParticleFromCID( rSelectedCID ), 
                    RegressionCurveHelper::getRegressionCurveIndex( xRegCurveCnt,
                        RegressionCurveHelper::getFirstCurveNotMeanValueLine( xRegCurveCnt ) ), false );
    }
    //-------------------------------------------------------------------------
    //trend line equation
    else if( rDispatchCommand.equals("FormatTrendlineEquation") )
    {
        if( eSelectedType == OBJECTTYPE_DATA_CURVE_EQUATION )
            return rSelectedCID;
        else
            return ObjectIdentifier::createDataCurveEquationCID(
                ObjectIdentifier::getSeriesParticleFromCID( rSelectedCID ),
                    RegressionCurveHelper::getRegressionCurveIndex( xRegCurveCnt,
                        RegressionCurveHelper::getFirstCurveNotMeanValueLine( xRegCurveCnt ) ) );
    }
    //-------------------------------------------------------------------------
    // y error bars
    else if( rDispatchCommand.equals("FormatYErrorBars") )
    {
        if( eSelectedType == OBJECTTYPE_DATA_ERRORS )
            return rSelectedCID;
        else
            return ObjectIdentifier::createClassifiedIdentifierWithParent(
                OBJECTTYPE_DATA_ERRORS, ::rtl::OUString(), rSelectedCID );
    }
    //-------------------------------------------------------------------------
    // axis
    else if( rDispatchCommand.equals("FormatAxis") )
    {
        if( eSelectedType == OBJECTTYPE_AXIS )
            return rSelectedCID;
        else
        {
            Reference< XAxis > xAxis = ObjectIdentifier::getAxisForCID( rSelectedCID, xChartModel );
            return ObjectIdentifier::createClassifiedIdentifierForObject( xAxis , xChartModel );
        }
    }
    //-------------------------------------------------------------------------
    // major grid
    else if( rDispatchCommand.equals("FormatMajorGrid") )
    {
        if( eSelectedType == OBJECTTYPE_GRID )
            return rSelectedCID;
        else
        {
            Reference< XAxis > xAxis = ObjectIdentifier::getAxisForCID( rSelectedCID, xChartModel );
            return ObjectIdentifier::createClassifiedIdentifierForGrid( xAxis, xChartModel );
        }
        
    }
    //-------------------------------------------------------------------------
    // minor grid
    else if( rDispatchCommand.equals("FormatMinorGrid") )
    {
        if( eSelectedType == OBJECTTYPE_SUBGRID )
            return rSelectedCID;
        else
        {
            Reference< XAxis > xAxis = ObjectIdentifier::getAxisForCID( rSelectedCID, xChartModel );
            return ObjectIdentifier::createClassifiedIdentifierForGrid( xAxis, xChartModel, 0 /*sub grid index*/ );
        }
    }
    //-------------------------------------------------------------------------
    // title
    else if( rDispatchCommand.equals("FormatTitle") )
    {
        if( eSelectedType == OBJECTTYPE_TITLE )
            return rSelectedCID;
    }
    //-------------------------------------------------------------------------
    // stock loss
    else if( rDispatchCommand.equals("FormatStockLoss") )
    {
        if( eSelectedType == OBJECTTYPE_DATA_STOCK_LOSS )
            return rSelectedCID;
        else
            return ObjectIdentifier::createClassifiedIdentifier( OBJECTTYPE_DATA_STOCK_LOSS, rtl::OUString());
    }
    //-------------------------------------------------------------------------
    // stock gain
    else if( rDispatchCommand.equals("FormatStockGain") )
    {
        if( eSelectedType == OBJECTTYPE_DATA_STOCK_GAIN )
            return rSelectedCID;
        else
            return ObjectIdentifier::createClassifiedIdentifier( OBJECTTYPE_DATA_STOCK_GAIN, rtl::OUString() );
    }

    return ObjectIdentifier::createClassifiedIdentifier(
        eObjectType, aParticleID );
}

}
// anonymous namespace

void SAL_CALL ChartController::executeDispatch_FormatObject(const ::rtl::OUString& rDispatchCommand)
{
    uno::Reference< XChartDocument > xChartDocument( m_aModel->getModel(), uno::UNO_QUERY );
    rtl::OString aCommand( rtl::OUStringToOString( rDispatchCommand, RTL_TEXTENCODING_ASCII_US ) );
    rtl::OUString rObjectCID = lcl_getObjectCIDForCommand( aCommand, xChartDocument, m_aSelection.getSelectedCID() );
    executeDlg_ObjectProperties( rObjectCID );
}

void SAL_CALL ChartController::executeDispatch_ObjectProperties()
{
    executeDlg_ObjectProperties( m_aSelection.getSelectedCID() );
}

namespace
{

rtl::OUString lcl_getFormatCIDforSelectedCID( const ::rtl::OUString& rSelectedCID )
{
    ::rtl::OUString aFormatCID(rSelectedCID);

    //get type of selected object
    ObjectType eObjectType = ObjectIdentifier::getObjectType( aFormatCID );

    // some legend entries are handled as if they were data series
	if( OBJECTTYPE_LEGEND_ENTRY==eObjectType )
	{
        rtl::OUString aParentParticle( ObjectIdentifier::getFullParentParticle( rSelectedCID ) );
		aFormatCID  = ObjectIdentifier::createClassifiedIdentifierForParticle( aParentParticle );
	}

    // treat diagram as wall
	if( OBJECTTYPE_DIAGRAM==eObjectType )
		aFormatCID  = ObjectIdentifier::createClassifiedIdentifier( OBJECTTYPE_DIAGRAM_WALL, rtl::OUString() );

    return aFormatCID;
}

}//end anonymous namespace

void SAL_CALL ChartController::executeDlg_ObjectProperties( const ::rtl::OUString& rSelectedObjectCID )
{
    rtl::OUString aObjectCID = lcl_getFormatCIDforSelectedCID( rSelectedObjectCID );

    UndoGuard aUndoGuard( ActionDescriptionProvider::createDescription(
                ActionDescriptionProvider::FORMAT,
                ObjectNameProvider::getName( ObjectIdentifier::getObjectType( aObjectCID ))),
            m_xUndoManager, m_aModel->getModel() );

    bool bSuccess = ChartController::executeDlg_ObjectProperties_withoutUndoGuard( aObjectCID, false );
    if( bSuccess ) 
        aUndoGuard.commitAction();
}

bool ChartController::executeDlg_ObjectProperties_withoutUndoGuard( const ::rtl::OUString& rObjectCID, bool bOkClickOnUnchangedDialogSouldBeRatedAsSuccessAlso )
{
    //return true if the properties were changed successfully
    bool bRet = false;
    if( !rObjectCID.getLength() )
    {
        //DBG_ERROR("empty ObjectID");
        return bRet;
    }
    try
    {
		NumberFormatterWrapper aNumberFormatterWrapper( uno::Reference< util::XNumberFormatsSupplier >(m_aModel->getModel(), uno::UNO_QUERY) );

        //-------------------------------------------------------------
        //get type of object
        ObjectType eObjectType = ObjectIdentifier::getObjectType( rObjectCID );
        if( OBJECTTYPE_UNKNOWN==eObjectType )
        {
            //DBG_ERROR("unknown ObjectType");
            return bRet;
        }
        if( OBJECTTYPE_DIAGRAM_WALL==eObjectType || OBJECTTYPE_DIAGRAM_FLOOR==eObjectType )
        {
            if( !DiagramHelper::isSupportingFloorAndWall( ChartModelHelper::findDiagram( m_aModel->getModel() ) ) )
                return bRet;
        }

        //-------------------------------------------------------------
        //convert properties to ItemSet

        awt::Size aPageSize( ChartModelHelper::getPageSize(m_aModel->getModel()) );

        ::std::auto_ptr< ReferenceSizeProvider > pRefSizeProv(
            impl_createReferenceSizeProvider());
        ::std::auto_ptr< ::comphelper::ItemConverter > apItemConverter(
            createItemConverter( rObjectCID, m_aModel->getModel(), m_xCC,
                                 m_pDrawModelWrapper->getSdrModel(),
                                 &aNumberFormatterWrapper,
                                 ExplicitValueProvider::getExplicitValueProvider(m_xChartView),
                                 pRefSizeProv ));
        if(!apItemConverter.get())
            return bRet;

        SfxItemSet aItemSet = apItemConverter->CreateEmptyItemSet();
        apItemConverter->FillItemSet( aItemSet );

        //-------------------------------------------------------------
        //prepare dialog
        ObjectPropertiesDialogParameter aDialogParameter = ObjectPropertiesDialogParameter( rObjectCID );
		aDialogParameter.init( m_aModel->getModel() );
        ViewElementListProvider aViewElementListProvider( m_pDrawModelWrapper.get() );

        ::vos::OGuard aGuard( Application::GetSolarMutex());
        SchAttribTabDlg aDlg( m_pChartWindow, &aItemSet, &aDialogParameter, &aViewElementListProvider
            , uno::Reference< util::XNumberFormatsSupplier >( m_aModel->getModel(), uno::UNO_QUERY ) );

        if(aDialogParameter.HasSymbolProperties())
        {
            SfxItemSet* pSymbolShapeProperties=NULL;
            uno::Reference< beans::XPropertySet > xObjectProperties =
                ObjectIdentifier::getObjectPropertySet( rObjectCID, m_aModel->getModel() );
            wrapper::DataPointItemConverter aSymbolItemConverter( m_aModel->getModel(), m_xCC
                                        , xObjectProperties, ObjectIdentifier::getDataSeriesForCID( rObjectCID, m_aModel->getModel() )
                                        , m_pDrawModelWrapper->getSdrModel().GetItemPool()
                                        , m_pDrawModelWrapper->getSdrModel()
                                        , &aNumberFormatterWrapper
                                        , uno::Reference< lang::XMultiServiceFactory >( m_aModel->getModel(), uno::UNO_QUERY )
                                        , wrapper::GraphicPropertyItemConverter::FILLED_DATA_POINT );

            pSymbolShapeProperties = new SfxItemSet( aSymbolItemConverter.CreateEmptyItemSet() );
            aSymbolItemConverter.FillItemSet( *pSymbolShapeProperties );

            sal_Int32   nStandardSymbol=0;//@todo get from somewhere
            Graphic*    pAutoSymbolGraphic = new Graphic( aViewElementListProvider.GetSymbolGraphic( nStandardSymbol, pSymbolShapeProperties ) );
            // note: the dialog takes the ownership of pSymbolShapeProperties and pAutoSymbolGraphic
            aDlg.setSymbolInformation( pSymbolShapeProperties, pAutoSymbolGraphic );
        }
        if( aDialogParameter.HasStatisticProperties() )
        {
            aDlg.SetAxisMinorStepWidthForErrorBarDecimals(
                InsertErrorBarsDialog::getAxisMinorStepWidthForErrorBarDecimals( m_aModel->getModel(), m_xChartView, rObjectCID ) );
        }

        //-------------------------------------------------------------
        //open the dialog
        if( aDlg.Execute() == RET_OK || (bOkClickOnUnchangedDialogSouldBeRatedAsSuccessAlso && aDlg.DialogWasClosedWithOK()) )
        {
            const SfxItemSet* pOutItemSet = aDlg.GetOutputItemSet();
            if(pOutItemSet)
            {
                ControllerLockGuard aCLGuard( m_aModel->getModel());
                apItemConverter->ApplyItemSet( *pOutItemSet );//model should be changed now
                bRet = true;
            }
        }
    }
    catch( util::CloseVetoException& )
    {
    }
    catch( uno::RuntimeException& )
    {
    }
    return bRet;
}

void SAL_CALL ChartController::executeDispatch_View3D()
{
    try
    {
        // using assignment for broken gcc 3.3
        UndoLiveUpdateGuard aUndoGuard = UndoLiveUpdateGuard(
            ::rtl::OUString( String( SchResId( STR_ACTION_EDIT_3D_VIEW ))),
            m_xUndoManager, m_aModel->getModel());

        // /--
        //open dialog
		::vos::OGuard aSolarGuard( Application::GetSolarMutex());
        View3DDialog aDlg( m_pChartWindow, m_aModel->getModel(), m_pDrawModelWrapper->GetColorTable() );
        if( aDlg.Execute() == RET_OK )
            aUndoGuard.commitAction();
        // \--
    }
    catch( uno::RuntimeException& e)
    {
        ASSERT_EXCEPTION( e );
    }
}

//.............................................................................
} //namespace chart
//.............................................................................
