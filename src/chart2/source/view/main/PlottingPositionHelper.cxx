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
#include "PlottingPositionHelper.hxx"
#include "CommonConverters.hxx"
#include "ViewDefines.hxx"
#include "Linear3DTransformation.hxx"
#include "VPolarTransformation.hxx"

#include "ShapeFactory.hxx"
#include "PropertyMapper.hxx"
#include <com/sun/star/drawing/DoubleSequence.hpp>
#include <com/sun/star/drawing/Position3D.hpp>
#include <com/sun/star/chart2/AxisType.hpp>

#include <rtl/math.hxx>

//.............................................................................
namespace chart
{
//.............................................................................
using namespace ::com::sun::star;
using namespace ::com::sun::star::chart2;

PlottingPositionHelper::PlottingPositionHelper()
        : m_aScales()
        , m_aMatrixScreenToScene()
        , m_xTransformationLogicToScene(NULL)
        , m_bSwapXAndY( false )
        , m_nXResolution( 1000 )
        , m_nYResolution( 1000 )
        , m_nZResolution( 1000 )
        , m_bMaySkipPointsInRegressionCalculation( true )
{
}
PlottingPositionHelper::PlottingPositionHelper( const PlottingPositionHelper& rSource )
        : m_aScales( rSource.m_aScales )
        , m_aMatrixScreenToScene( rSource.m_aMatrixScreenToScene )
        , m_xTransformationLogicToScene( NULL ) //should be recalculated
        , m_bSwapXAndY( rSource.m_bSwapXAndY )
        , m_nXResolution( rSource.m_nXResolution )
        , m_nYResolution( rSource.m_nYResolution )
        , m_nZResolution( rSource.m_nZResolution )
        , m_bMaySkipPointsInRegressionCalculation( rSource.m_bMaySkipPointsInRegressionCalculation )
{
}

PlottingPositionHelper::~PlottingPositionHelper()
{

}

PlottingPositionHelper* PlottingPositionHelper::clone() const
{
    PlottingPositionHelper* pRet = new PlottingPositionHelper(*this);
    return pRet;
}

PlottingPositionHelper* PlottingPositionHelper::createSecondaryPosHelper( const ExplicitScaleData& rSecondaryScale )
{
    PlottingPositionHelper* pRet = this->clone();
    pRet->m_aScales[1]=rSecondaryScale;
    return pRet;
}

void PlottingPositionHelper::setTransformationSceneToScreen( const drawing::HomogenMatrix& rMatrix)
{
    m_aMatrixScreenToScene = HomogenMatrixToB3DHomMatrix(rMatrix);
    m_xTransformationLogicToScene = NULL;
}

void PlottingPositionHelper::setScales( const uno::Sequence< ExplicitScaleData >& rScales, sal_Bool bSwapXAndYAxis )
{
    m_aScales = rScales;
    m_bSwapXAndY = bSwapXAndYAxis;
    m_xTransformationLogicToScene = NULL;
}
const uno::Sequence< ExplicitScaleData >& PlottingPositionHelper::getScales() const
{
    return m_aScales;
}

uno::Reference< XTransformation > PlottingPositionHelper::getTransformationScaledLogicToScene() const
{
    //this is a standard transformation for a cartesian coordinate system

    //transformation from 2) to 4) //@todo 2) and 4) need a ink to a document

    //we need to apply this transformation to each geometric object because of a bug/problem
    //of the old drawing layer (the UNO_NAME_3D_EXTRUDE_DEPTH is an integer value instead of an double )
    if(!m_xTransformationLogicToScene.is())
    {
        ::basegfx::B3DHomMatrix aMatrix;
        double MinX = getLogicMinX();
        double MinY = getLogicMinY();
        double MinZ = getLogicMinZ();
        double MaxX = getLogicMaxX();
        double MaxY = getLogicMaxY();
        double MaxZ = getLogicMaxZ();

        AxisOrientation nXAxisOrientation = m_aScales[0].Orientation;
        AxisOrientation nYAxisOrientation = m_aScales[1].Orientation;
        AxisOrientation nZAxisOrientation = m_aScales[2].Orientation;

        //apply scaling
        doLogicScaling( &MinX, &MinY, &MinZ );
        doLogicScaling( &MaxX, &MaxY, &MaxZ);

        if(m_bSwapXAndY)
        {
            std::swap(MinX,MinY);
            std::swap(MaxX,MaxY);
            std::swap(nXAxisOrientation,nYAxisOrientation);
        }

        double fWidthX = MaxX - MinX;
        double fWidthY = MaxY - MinY;
        double fWidthZ = MaxZ - MinZ;

        double fScaleDirectionX = AxisOrientation_MATHEMATICAL==nXAxisOrientation ? 1.0 : -1.0;
        double fScaleDirectionY = AxisOrientation_MATHEMATICAL==nYAxisOrientation ? 1.0 : -1.0;
        double fScaleDirectionZ = AxisOrientation_MATHEMATICAL==nZAxisOrientation ? -1.0 : 1.0;

        double fScaleX = fScaleDirectionX*FIXED_SIZE_FOR_3D_CHART_VOLUME/fWidthX;
        double fScaleY = fScaleDirectionY*FIXED_SIZE_FOR_3D_CHART_VOLUME/fWidthY;
        double fScaleZ = fScaleDirectionZ*FIXED_SIZE_FOR_3D_CHART_VOLUME/fWidthZ;

        aMatrix.scale(fScaleX, fScaleY, fScaleZ);

        if( AxisOrientation_MATHEMATICAL==nXAxisOrientation )
            aMatrix.translate(-MinX*fScaleX, 0.0, 0.0);
        else
            aMatrix.translate(-MaxX*fScaleX, 0.0, 0.0);
        if( AxisOrientation_MATHEMATICAL==nYAxisOrientation )
            aMatrix.translate(0.0, -MinY*fScaleY, 0.0);
        else
            aMatrix.translate(0.0, -MaxY*fScaleY, 0.0);
        if( AxisOrientation_MATHEMATICAL==nZAxisOrientation )
            aMatrix.translate(0.0, 0.0, -MaxZ*fScaleZ);//z direction in draw is reverse mathematical direction
        else
            aMatrix.translate(0.0, 0.0, -MinZ*fScaleZ);

        aMatrix = m_aMatrixScreenToScene*aMatrix;

        m_xTransformationLogicToScene = new Linear3DTransformation(B3DHomMatrixToHomogenMatrix( aMatrix ),m_bSwapXAndY);
    }
    return m_xTransformationLogicToScene;
}

drawing::Position3D PlottingPositionHelper::transformLogicToScene(
    double fX, double fY, double fZ, bool bClip ) const
{
    if(bClip)
        this->clipLogicValues( &fX,&fY,&fZ );
    this->doLogicScaling( &fX,&fY,&fZ );

    return this->transformScaledLogicToScene( fX, fY, fZ, false );
}

drawing::Position3D PlottingPositionHelper::transformScaledLogicToScene(
    double fX, double fY, double fZ, bool bClip  ) const
{
    if( bClip )
        this->clipScaledLogicValues( &fX,&fY,&fZ );

    drawing::Position3D aPos( fX, fY, fZ);

    uno::Reference< XTransformation > xTransformation =
        this->getTransformationScaledLogicToScene();
    uno::Sequence< double > aSeq =
        xTransformation->transform( Position3DToSequence(aPos) );
    return SequenceToPosition3D(aSeq);
}

//static
awt::Point PlottingPositionHelper::transformSceneToScreenPosition( const drawing::Position3D& rScenePosition3D
                , const uno::Reference< drawing::XShapes >& xSceneTarget
                , ShapeFactory* pShapeFactory
                , sal_Int32 nDimensionCount )
{
    //@todo would like to have a cheaper method to do this transformation
    awt::Point aScreenPoint( static_cast<sal_Int32>(rScenePosition3D.PositionX), static_cast<sal_Int32>(rScenePosition3D.PositionY) );

    //transformation from scene to screen (only neccessary for 3D):
    if(3==nDimensionCount)
    {
        //create 3D anchor shape
        tPropertyNameMap aDummyPropertyNameMap;
        uno::Reference< drawing::XShape > xShape3DAnchor = pShapeFactory->createCube( xSceneTarget
                , rScenePosition3D,drawing::Direction3D(1,1,1)
                , 0, 0, aDummyPropertyNameMap);
        //get 2D position from xShape3DAnchor
        aScreenPoint = xShape3DAnchor->getPosition();
        xSceneTarget->remove(xShape3DAnchor);
    }
    return aScreenPoint;
}

void PlottingPositionHelper::transformScaledLogicToScene( drawing::PolyPolygonShape3D& rPolygon ) const
{
    drawing::Position3D aScenePosition;
    for( sal_Int32 nS = rPolygon.SequenceX.getLength(); nS--;)
    {
        drawing::DoubleSequence& xValues = rPolygon.SequenceX[nS];
        drawing::DoubleSequence& yValues = rPolygon.SequenceY[nS];
        drawing::DoubleSequence& zValues = rPolygon.SequenceZ[nS];
        for( sal_Int32 nP = xValues.getLength(); nP--; )
        {
            double& fX = xValues[nP];
            double& fY = yValues[nP];
            double& fZ = zValues[nP];
            aScenePosition = this->transformScaledLogicToScene( fX,fY,fZ,true );
            fX = aScenePosition.PositionX;
            fY = aScenePosition.PositionY;
            fZ = aScenePosition.PositionZ;
        }
    }
}


void PlottingPositionHelper::clipScaledLogicValues( double* pX, double* pY, double* pZ ) const
{
    //get logic clip values:
    double MinX = getLogicMinX();
    double MinY = getLogicMinY();
    double MinZ = getLogicMinZ();
    double MaxX = getLogicMaxX();
    double MaxY = getLogicMaxY();
    double MaxZ = getLogicMaxZ();

    //apply scaling
    doLogicScaling( &MinX, &MinY, &MinZ );
    doLogicScaling( &MaxX, &MaxY, &MaxZ);

    if(pX)
    {
        if( *pX < MinX )
            *pX = MinX;
        else if( *pX > MaxX )
            *pX = MaxX;
    }
    if(pY)
    {
        if( *pY < MinY )
            *pY = MinY;
        else if( *pY > MaxY )
            *pY = MaxY;
    }
    if(pZ)
    {
        if( *pZ < MinZ )
            *pZ = MinZ;
        else if( *pZ > MaxZ )
            *pZ = MaxZ;
    }
}

basegfx::B2DRectangle PlottingPositionHelper::getScaledLogicClipDoubleRect() const
{
    //get logic clip values:
    double MinX = getLogicMinX();
    double MinY = getLogicMinY();
    double MinZ = getLogicMinZ();
    double MaxX = getLogicMaxX();
    double MaxY = getLogicMaxY();
    double MaxZ = getLogicMaxZ();

    //apply scaling
    doLogicScaling( &MinX, &MinY, &MinZ );
    doLogicScaling( &MaxX, &MaxY, &MaxZ);

    basegfx::B2DRectangle aRet( MinX, MaxY, MaxX, MinY );
    return aRet;
}

drawing::Direction3D PlottingPositionHelper::getScaledLogicWidth() const
{
    drawing::Direction3D aRet;

    double MinX = getLogicMinX();
    double MinY = getLogicMinY();
    double MinZ = getLogicMinZ();
    double MaxX = getLogicMaxX();
    double MaxY = getLogicMaxY();
    double MaxZ = getLogicMaxZ();

    doLogicScaling( &MinX, &MinY, &MinZ );
    doLogicScaling( &MaxX, &MaxY, &MaxZ);

    aRet.DirectionX = MaxX - MinX;
    aRet.DirectionY = MaxY - MinY;
    aRet.DirectionZ = MaxZ - MinZ;
    return aRet;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

PolarPlottingPositionHelper::PolarPlottingPositionHelper( NormalAxis eNormalAxis )
    : m_fRadiusOffset(0.0)
    , m_fAngleDegreeOffset(90.0)
    , m_aUnitCartesianToScene()
    , m_eNormalAxis(eNormalAxis)
{
    m_bMaySkipPointsInRegressionCalculation = false;
}

PolarPlottingPositionHelper::PolarPlottingPositionHelper( const PolarPlottingPositionHelper& rSource )
    : PlottingPositionHelper(rSource)
    , m_fRadiusOffset( rSource.m_fRadiusOffset )
    , m_fAngleDegreeOffset( rSource.m_fAngleDegreeOffset )
    , m_aUnitCartesianToScene( rSource.m_aUnitCartesianToScene )
    , m_eNormalAxis( rSource.m_eNormalAxis )
{
}

PolarPlottingPositionHelper::~PolarPlottingPositionHelper()
{
}

PlottingPositionHelper* PolarPlottingPositionHelper::clone() const
{
    PolarPlottingPositionHelper* pRet = new PolarPlottingPositionHelper(*this);
    return pRet;
}

void PolarPlottingPositionHelper::setTransformationSceneToScreen( const drawing::HomogenMatrix& rMatrix)
{
    PlottingPositionHelper::setTransformationSceneToScreen( rMatrix);
    m_aUnitCartesianToScene =impl_calculateMatrixUnitCartesianToScene( m_aMatrixScreenToScene );
}
void PolarPlottingPositionHelper::setScales( const uno::Sequence< ExplicitScaleData >& rScales, sal_Bool bSwapXAndYAxis )
{
    PlottingPositionHelper::setScales( rScales, bSwapXAndYAxis );
    m_aUnitCartesianToScene =impl_calculateMatrixUnitCartesianToScene( m_aMatrixScreenToScene );
}

::basegfx::B3DHomMatrix PolarPlottingPositionHelper::impl_calculateMatrixUnitCartesianToScene( const ::basegfx::B3DHomMatrix& rMatrixScreenToScene ) const
{
    ::basegfx::B3DHomMatrix aRet;

    if( !m_aScales.getLength() )
        return aRet;

    double fTranslate =1.0;
    double fScale     =FIXED_SIZE_FOR_3D_CHART_VOLUME/2.0;

    double fTranslateLogicZ =fTranslate;
    double fScaleLogicZ     =fScale;
    {
        double fScaleDirectionZ = AxisOrientation_MATHEMATICAL==m_aScales[2].Orientation ? 1.0 : -1.0;
        double MinZ = getLogicMinZ();
        double MaxZ = getLogicMaxZ();
        doLogicScaling( 0, 0, &MinZ );
        doLogicScaling( 0, 0, &MaxZ );
        double fWidthZ = MaxZ - MinZ;

        if( AxisOrientation_MATHEMATICAL==m_aScales[2].Orientation )
            fTranslateLogicZ=MinZ;
        else
            fTranslateLogicZ=MaxZ;
        fScaleLogicZ = fScaleDirectionZ*FIXED_SIZE_FOR_3D_CHART_VOLUME/fWidthZ;
    }

    double fTranslateX = fTranslate;
    double fTranslateY = fTranslate;
    double fTranslateZ = fTranslate;

    double fScaleX = fScale;     
    double fScaleY = fScale;
    double fScaleZ = fScale;

    switch(m_eNormalAxis)
    {
        case NormalAxis_X:
            {
                fTranslateX = fTranslateLogicZ;
                fScaleX = fScaleLogicZ;
            }
            break;
        case NormalAxis_Y:
            {
                fTranslateY = fTranslateLogicZ;
                fScaleY = fScaleLogicZ;
            }
            break;
        default: //NormalAxis_Z:
            {
                fTranslateZ = fTranslateLogicZ;
                fScaleZ = fScaleLogicZ;
            }
            break;
    }

    aRet.translate(fTranslateX, fTranslateY, fTranslateZ);//x first
    aRet.scale(fScaleX, fScaleY, fScaleZ);//x first
    
    aRet = rMatrixScreenToScene * aRet;
    return aRet;
}

::basegfx::B3DHomMatrix PolarPlottingPositionHelper::getUnitCartesianToScene() const
{
    return m_aUnitCartesianToScene;
}

uno::Reference< XTransformation > PolarPlottingPositionHelper::getTransformationScaledLogicToScene() const
{
    if( !m_xTransformationLogicToScene.is() )
        m_xTransformationLogicToScene = new VPolarTransformation(*this);
    return m_xTransformationLogicToScene;
}

double PolarPlottingPositionHelper::getWidthAngleDegree( double& fStartLogicValueOnAngleAxis, double& fEndLogicValueOnAngleAxis ) const
{
    const ExplicitScaleData& rAngleScale = m_bSwapXAndY ? m_aScales[1] : m_aScales[0];
    if( AxisOrientation_MATHEMATICAL != rAngleScale.Orientation )
    {
        double fHelp = fEndLogicValueOnAngleAxis;
        fEndLogicValueOnAngleAxis = fStartLogicValueOnAngleAxis;
        fStartLogicValueOnAngleAxis = fHelp;
    }

    double fStartAngleDegree = this->transformToAngleDegree( fStartLogicValueOnAngleAxis );
    double fEndAngleDegree   = this->transformToAngleDegree( fEndLogicValueOnAngleAxis );
    double fWidthAngleDegree = fEndAngleDegree - fStartAngleDegree;
    
    if( ::rtl::math::approxEqual( fStartAngleDegree, fEndAngleDegree )
        && !::rtl::math::approxEqual( fStartLogicValueOnAngleAxis, fEndLogicValueOnAngleAxis ) )
        fWidthAngleDegree = 360.0;
    
    while(fWidthAngleDegree<0.0)
        fWidthAngleDegree+=360.0;
    while(fWidthAngleDegree>360.0)
        fWidthAngleDegree-=360.0;

    return fWidthAngleDegree;
}

double PolarPlottingPositionHelper::transformToAngleDegree( double fLogicValueOnAngleAxis, bool bDoScaling ) const
{
    double fRet=0.0;

    double fAxisAngleScaleDirection = 1.0;
    {
        const ExplicitScaleData& rScale = m_bSwapXAndY ? m_aScales[1] : m_aScales[0];
        if(AxisOrientation_MATHEMATICAL != rScale.Orientation)
            fAxisAngleScaleDirection *= -1.0;
    }

    double MinAngleValue = 0.0;
    double MaxAngleValue = 0.0;
    {
        double MinX = getLogicMinX();
        double MinY = getLogicMinY();
        double MaxX = getLogicMaxX();
        double MaxY = getLogicMaxY();
        double MinZ = getLogicMinZ();
        double MaxZ = getLogicMaxZ();

        doLogicScaling( &MinX, &MinY, &MinZ );
        doLogicScaling( &MaxX, &MaxY, &MaxZ);

        MinAngleValue = m_bSwapXAndY ? MinY : MinX;
        MaxAngleValue = m_bSwapXAndY ? MaxY : MaxX;
    }

    double fScaledLogicAngleValue = 0.0;
    if(bDoScaling)
    {
        double fX = m_bSwapXAndY ? getLogicMaxX() : fLogicValueOnAngleAxis;
        double fY = m_bSwapXAndY ? fLogicValueOnAngleAxis : getLogicMaxY();
        double fZ = getLogicMaxZ();
        clipLogicValues( &fX, &fY, &fZ );
        doLogicScaling( &fX, &fY, &fZ );
        fScaledLogicAngleValue = m_bSwapXAndY ? fY : fX;
    }
    else
        fScaledLogicAngleValue = fLogicValueOnAngleAxis;

    fRet = m_fAngleDegreeOffset
                  + fAxisAngleScaleDirection*(fScaledLogicAngleValue-MinAngleValue)*360.0
                    /fabs(MaxAngleValue-MinAngleValue);
    while(fRet>360.0)
        fRet-=360.0;
    while(fRet<0)
        fRet+=360.0;
    return fRet;
}

double PolarPlottingPositionHelper::transformToRadius( double fLogicValueOnRadiusAxis, bool bDoScaling ) const
{
    double fNormalRadius = 0.0;
    {
        double fScaledLogicRadiusValue = 0.0;
        double fX = m_bSwapXAndY ? fLogicValueOnRadiusAxis: getLogicMaxX();
        double fY = m_bSwapXAndY ? getLogicMaxY() : fLogicValueOnRadiusAxis;
        if(bDoScaling)
            doLogicScaling( &fX, &fY, 0 );
        
        fScaledLogicRadiusValue = m_bSwapXAndY ? fX : fY;

        bool bMinIsInnerRadius = true;
        const ExplicitScaleData& rScale = m_bSwapXAndY ? m_aScales[0] : m_aScales[1];
        if(AxisOrientation_MATHEMATICAL != rScale.Orientation)
            bMinIsInnerRadius = false;

        double fInnerScaledLogicRadius=0.0;
        double fOuterScaledLogicRadius=0.0;
        {
            double MinX = getLogicMinX();
            double MinY = getLogicMinY();
            doLogicScaling( &MinX, &MinY, 0 );
            double MaxX = getLogicMaxX();
            double MaxY = getLogicMaxY();
            doLogicScaling( &MaxX, &MaxY, 0 );

            double fMin = m_bSwapXAndY ? MinX : MinY;
            double fMax = m_bSwapXAndY ? MaxX : MaxY;
            
            fInnerScaledLogicRadius = bMinIsInnerRadius ? fMin : fMax;
            fOuterScaledLogicRadius = bMinIsInnerRadius ? fMax : fMin;
        }

        if( bMinIsInnerRadius )
            fInnerScaledLogicRadius -= fabs(m_fRadiusOffset);
        else
            fInnerScaledLogicRadius += fabs(m_fRadiusOffset);
        fNormalRadius = (fScaledLogicRadiusValue-fInnerScaledLogicRadius)/(fOuterScaledLogicRadius-fInnerScaledLogicRadius);
    }
    return fNormalRadius;
}

drawing::Position3D PolarPlottingPositionHelper::transformLogicToScene( double fX, double fY, double fZ, bool bClip ) const
{
    if(bClip)
        this->clipLogicValues( &fX,&fY,&fZ );
    double fLogicValueOnAngleAxis  = m_bSwapXAndY ? fY : fX;
    double fLogicValueOnRadiusAxis = m_bSwapXAndY ? fX : fY;
    return this->transformAngleRadiusToScene( fLogicValueOnAngleAxis, fLogicValueOnRadiusAxis, fZ, true );
}

drawing::Position3D PolarPlottingPositionHelper::transformScaledLogicToScene( double fX, double fY, double fZ, bool bClip ) const
{
    if(bClip)
        this->clipScaledLogicValues( &fX,&fY,&fZ );
    double fLogicValueOnAngleAxis  = m_bSwapXAndY ? fY : fX;
    double fLogicValueOnRadiusAxis = m_bSwapXAndY ? fX : fY;
    return this->transformAngleRadiusToScene( fLogicValueOnAngleAxis, fLogicValueOnRadiusAxis, fZ, false );
}
drawing::Position3D PolarPlottingPositionHelper::transformUnitCircleToScene( double fUnitAngleDegree, double fUnitRadius
                                                                            , double fLogicZ, bool /* bDoScaling */ ) const
{
    double fAnglePi = fUnitAngleDegree*F_PI/180.0;

    double fX=fUnitRadius*rtl::math::cos(fAnglePi);
    double fY=fUnitRadius*rtl::math::sin(fAnglePi);
    double fZ=fLogicZ;

    switch(m_eNormalAxis)
    {
        case NormalAxis_X:
            std::swap(fX,fZ);
            break;
        case NormalAxis_Y:
            std::swap(fY,fZ);
            fZ*=-1;
            break;
        default: //NormalAxis_Z
            break;
    }
    
    //!! applying matrix to vector does ignore translation, so it is important to use a B3DPoint here instead of B3DVector
    ::basegfx::B3DPoint aPoint(fX,fY,fZ);
    ::basegfx::B3DPoint aRet = m_aUnitCartesianToScene * aPoint;
    return B3DPointToPosition3D(aRet);
}

drawing::Position3D PolarPlottingPositionHelper::transformAngleRadiusToScene( double fLogicValueOnAngleAxis, double fLogicValueOnRadiusAxis, double fLogicZ, bool bDoScaling ) const
{
    double fUnitAngleDegree = this->transformToAngleDegree(fLogicValueOnAngleAxis,bDoScaling);
    double fUnitRadius      = this->transformToRadius(fLogicValueOnRadiusAxis,bDoScaling); 

    return transformUnitCircleToScene( fUnitAngleDegree, fUnitRadius, fLogicZ, bDoScaling );
}

#ifdef NOTYET
double PolarPlottingPositionHelper::getInnerLogicRadius() const
{
    const ExplicitScaleData& rScale = m_bSwapXAndY ? m_aScales[0] : m_aScales[1];
    if( AxisOrientation_MATHEMATICAL==rScale.Orientation )
        return rScale.Minimum;
    else
        return rScale.Maximum;
}
#endif

double PolarPlottingPositionHelper::getOuterLogicRadius() const
{
    const ExplicitScaleData& rScale = m_bSwapXAndY ? m_aScales[0] : m_aScales[1];
    if( AxisOrientation_MATHEMATICAL==rScale.Orientation )
        return rScale.Maximum;
    else
        return rScale.Minimum;
}

bool PlottingPositionHelper::isPercentY() const
{
    return m_aScales[1].AxisType==AxisType::PERCENT;
}

double PlottingPositionHelper::getBaseValueY() const
{
    return m_aScales[1].Origin;
}

/*
// ____ XTransformation ____
uno::Sequence< double > SAL_CALL PolarPlottingPositionHelper::transform(
                        const uno::Sequence< double >& rSourceValues )
            throw (uno::RuntimeException, lang::IllegalArgumentException)
{
    uno::Sequence< double > aSourceValues(3);
    return aSourceValues;
}

sal_Int32 SAL_CALL PolarPlottingPositionHelper::getSourceDimension() throw (uno::RuntimeException)
{
    return 3;
}

sal_Int32 SAL_CALL PolarPlottingPositionHelper::getTargetDimension() throw (uno::RuntimeException)
{
    return 3;
}
*/

//.............................................................................
} //namespace chart
//.............................................................................
