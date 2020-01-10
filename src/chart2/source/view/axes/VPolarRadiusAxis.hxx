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
#ifndef _CHART2_VPOLARRADIUSAXIS_HXX
#define _CHART2_VPOLARRADIUSAXIS_HXX

#include "VPolarAxis.hxx"

#include <memory>

//.............................................................................
namespace chart
{
//.............................................................................

//-----------------------------------------------------------------------------
/**
*/

class VCartesianAxis;

class VPolarRadiusAxis : public VPolarAxis
{
public:
    VPolarRadiusAxis( const AxisProperties& rAxisProperties
           , const ::com::sun::star::uno::Reference< ::com::sun::star::util::XNumberFormatsSupplier >& xNumberFormatsSupplier
           , sal_Int32 nDimensionCount );
    virtual ~VPolarRadiusAxis();

    virtual void SAL_CALL initPlotter(
          const ::com::sun::star::uno::Reference<
                ::com::sun::star::drawing::XShapes >& xLogicTarget
		, const ::com::sun::star::uno::Reference<
                ::com::sun::star::drawing::XShapes >& xFinalTarget
		, const ::com::sun::star::uno::Reference<
                ::com::sun::star::lang::XMultiServiceFactory >& xFactory
        , const rtl::OUString& rCID 
                ) throw (::com::sun::star::uno::RuntimeException );

    virtual void setTransformationSceneToScreen( const ::com::sun::star::drawing::HomogenMatrix& rMatrix );

    virtual void SAL_CALL setScales(
          const ::com::sun::star::uno::Sequence<
            ::com::sun::star::chart2::ExplicitScaleData >& rScales
            , sal_Bool bSwapXAndYAxis )
                throw (::com::sun::star::uno::RuntimeException);

    virtual void SAL_CALL setExplicitScaleAndIncrement(
            const ::com::sun::star::chart2::ExplicitScaleData& rScale
          , const ::com::sun::star::chart2::ExplicitIncrementData& rIncrement )
                throw (::com::sun::star::uno::RuntimeException);

    virtual void SAL_CALL initAxisLabelProperties(
                    const ::com::sun::star::awt::Size& rFontReferenceSize
                  , const ::com::sun::star::awt::Rectangle& rMaximumSpaceForLabels );

    virtual sal_Int32 estimateMaximumAutoMainIncrementCount();

    virtual void SAL_CALL createMaximumLabels();
    virtual void SAL_CALL createLabels();
    virtual void SAL_CALL updatePositions();

    virtual void SAL_CALL createShapes();

protected: //methods
    virtual bool prepareShapeCreation();

private: //member
    std::auto_ptr<VCartesianAxis>  m_apAxisWithLabels;
};

//.............................................................................
} //namespace chart
//.............................................................................
#endif
