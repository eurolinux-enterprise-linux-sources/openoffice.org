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
#include "precompiled_basegfx.hxx"

#include <com/sun/star/geometry/RealSize2D.hpp>
#include <com/sun/star/geometry/RealPoint2D.hpp>
#include <com/sun/star/geometry/RealRectangle2D.hpp>
#include <com/sun/star/geometry/RealRectangle3D.hpp>
#include <com/sun/star/geometry/RealBezierSegment2D.hpp>
#include <com/sun/star/geometry/AffineMatrix2D.hpp>
#include <com/sun/star/geometry/AffineMatrix3D.hpp>
#include <com/sun/star/geometry/Matrix2D.hpp>
#include <com/sun/star/geometry/IntegerSize2D.hpp>
#include <com/sun/star/geometry/IntegerPoint2D.hpp>
#include <com/sun/star/geometry/IntegerRectangle2D.hpp>
#include <com/sun/star/rendering/XPolyPolygon2D.hpp>
#include <com/sun/star/rendering/XGraphicDevice.hpp>
#include <com/sun/star/awt/Size.hpp>
#include <com/sun/star/awt/Point.hpp>
#include <com/sun/star/awt/Rectangle.hpp>
#include <basegfx/tools/unopolypolygon.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/matrix/b3dhommatrix.hxx>
#include <basegfx/vector/b2dsize.hxx>
#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/range/b2drectangle.hxx>
#include <basegfx/range/b3drange.hxx>
#include <basegfx/vector/b2isize.hxx>
#include <basegfx/point/b2ipoint.hxx>
#include <basegfx/range/b2irectangle.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <basegfx/tools/canvastools.hxx>
#include <limits>

using namespace ::com::sun::star;

namespace basegfx
{

    namespace unotools
    {
        namespace
        {
            uno::Sequence< geometry::RealBezierSegment2D > bezierSequenceFromB2DPolygon(const ::basegfx::B2DPolygon& rPoly)
            {
                const sal_uInt32 nPointCount(rPoly.count());
                uno::Sequence< geometry::RealBezierSegment2D > outputSequence(nPointCount);
                geometry::RealBezierSegment2D* pOutput = outputSequence.getArray();

				// fill sequences and imply clodes polygon on this implementation layer
				for(sal_uInt32 a(0); a < nPointCount; a++)
				{
                    const basegfx::B2DPoint aStart(rPoly.getB2DPoint(a));
                    const basegfx::B2DPoint aControlA(rPoly.getNextControlPoint(a));
                    const basegfx::B2DPoint aControlB(rPoly.getPrevControlPoint((a + 1) % nPointCount));

					pOutput[a] = geometry::RealBezierSegment2D(
						aStart.getX(), aStart.getY(),
						aControlA.getX(), aControlA.getY(),
						aControlB.getX(), aControlB.getY());
				}

                return outputSequence;
            }

            uno::Sequence< geometry::RealPoint2D > pointSequenceFromB2DPolygon( const ::basegfx::B2DPolygon& rPoly )
            {
                const sal_uInt32 nNumPoints( rPoly.count() );

                uno::Sequence< geometry::RealPoint2D > outputSequence( nNumPoints );
                geometry::RealPoint2D* pOutput = outputSequence.getArray();
                
                // fill sequence from polygon
                sal_uInt32 i;
                for( i=0; i<nNumPoints; ++i )
                {
                    const ::basegfx::B2DPoint 	aPoint( rPoly.getB2DPoint(i) );

                    pOutput[i] = geometry::RealPoint2D( aPoint.getX(),
                                                        aPoint.getY() );
                }

                return outputSequence;
            }
        }

        //---------------------------------------------------------------------------------------

        uno::Sequence< uno::Sequence< geometry::RealBezierSegment2D > > bezierSequenceSequenceFromB2DPolyPolygon( const ::basegfx::B2DPolyPolygon& rPolyPoly )
        {
            const sal_uInt32 nNumPolies( rPolyPoly.count() );
            sal_uInt32 i;

            uno::Sequence< uno::Sequence< geometry::RealBezierSegment2D > > outputSequence( nNumPolies );
            uno::Sequence< geometry::RealBezierSegment2D >* pOutput = outputSequence.getArray();

            for( i=0; i<nNumPolies; ++i )
            {
                pOutput[i] = bezierSequenceFromB2DPolygon( rPolyPoly.getB2DPolygon(i) );
            }

            return outputSequence;
        }

        //---------------------------------------------------------------------------------------

        uno::Sequence< uno::Sequence< geometry::RealPoint2D > > pointSequenceSequenceFromB2DPolyPolygon( const ::basegfx::B2DPolyPolygon& rPolyPoly	 )
        {
            const sal_uInt32 nNumPolies( rPolyPoly.count() );
            sal_uInt32 i;

            uno::Sequence< uno::Sequence< geometry::RealPoint2D > > outputSequence( nNumPolies );
            uno::Sequence< geometry::RealPoint2D >* pOutput = outputSequence.getArray();

            for( i=0; i<nNumPolies; ++i )
            {
                pOutput[i] = pointSequenceFromB2DPolygon( rPolyPoly.getB2DPolygon(i) );
            }

            return outputSequence;
        }

        //---------------------------------------------------------------------------------------

        uno::Reference< rendering::XPolyPolygon2D > xPolyPolygonFromB2DPolygon( const uno::Reference< rendering::XGraphicDevice >& 	xGraphicDevice,
                                                                                const ::basegfx::B2DPolygon&						rPoly	 )
        {
            uno::Reference< rendering::XPolyPolygon2D > xRes;

            if( !xGraphicDevice.is() )
                return xRes;

            if( rPoly.areControlPointsUsed() )
            {
                uno::Sequence< uno::Sequence< geometry::RealBezierSegment2D > > outputSequence( 1 );
                outputSequence[0] = bezierSequenceFromB2DPolygon( rPoly );

                xRes.set( xGraphicDevice->createCompatibleBezierPolyPolygon( outputSequence ),
                          uno::UNO_QUERY );
            }
            else
            {
                uno::Sequence< uno::Sequence< geometry::RealPoint2D > > outputSequence( 1 );
                outputSequence[0] = pointSequenceFromB2DPolygon( rPoly );

                xRes.set( xGraphicDevice->createCompatibleLinePolyPolygon( outputSequence ),
                          uno::UNO_QUERY );
            }

            if( xRes.is() && rPoly.isClosed() )
                xRes->setClosed( 0, sal_True );

            return xRes;
        }

        //---------------------------------------------------------------------------------------

        uno::Reference< rendering::XPolyPolygon2D > xPolyPolygonFromB2DPolyPolygon( const uno::Reference< rendering::XGraphicDevice >& 	xGraphicDevice,
                                                                                    const ::basegfx::B2DPolyPolygon&					rPolyPoly	 )
        {
            uno::Reference< rendering::XPolyPolygon2D > xRes;

            if( !xGraphicDevice.is() )
                return xRes;

            const sal_uInt32 nNumPolies( rPolyPoly.count() );
            sal_uInt32 i;

            if( rPolyPoly.areControlPointsUsed() )
            {
                xRes.set( xGraphicDevice->createCompatibleBezierPolyPolygon( 
                              bezierSequenceSequenceFromB2DPolyPolygon( rPolyPoly ) ),
                          uno::UNO_QUERY );
            }
            else
            {
                xRes.set( xGraphicDevice->createCompatibleLinePolyPolygon( 
                              pointSequenceSequenceFromB2DPolyPolygon( rPolyPoly ) ),
                          uno::UNO_QUERY );
            }

            for( i=0; i<nNumPolies; ++i )
            {
	            xRes->setClosed( i, rPolyPoly.getB2DPolygon(i).isClosed() );
            }

            return xRes;
        }

        //---------------------------------------------------------------------------------------

        ::basegfx::B2DPolygon polygonFromPoint2DSequence( const uno::Sequence< geometry::RealPoint2D >& points )
        {
            const sal_Int32 nCurrSize( points.getLength() );

            ::basegfx::B2DPolygon aPoly;

            for( sal_Int32 nCurrPoint=0; nCurrPoint<nCurrSize; ++nCurrPoint )
                aPoly.append( b2DPointFromRealPoint2D( points[nCurrPoint] ) );

            return aPoly;
        }

        //---------------------------------------------------------------------------------------

        ::basegfx::B2DPolyPolygon polyPolygonFromPoint2DSequenceSequence( const uno::Sequence< uno::Sequence< geometry::RealPoint2D > >& points )
        {
            ::basegfx::B2DPolyPolygon aRes;

            for( sal_Int32 nCurrPoly=0; nCurrPoly<points.getLength(); ++nCurrPoly )
            {
                aRes.append( polygonFromPoint2DSequence( points[nCurrPoly] ) );
            }

            return aRes;
        }

        //---------------------------------------------------------------------------------------

        ::basegfx::B2DPolygon polygonFromBezier2DSequence( const uno::Sequence< geometry::RealBezierSegment2D >& curves )
        {
            const sal_Int32 nSize(curves.getLength());
			basegfx::B2DPolygon aRetval;

			if(nSize)
			{
				// prepare start with providing a start point. Use the first point from
				// the sequence for this
				const geometry::RealBezierSegment2D& rFirstSegment(curves[0]); // #i79917# first segment, not last
				aRetval.append(basegfx::B2DPoint(rFirstSegment.Px, rFirstSegment.Py));

				for(sal_Int32 a(0); a < nSize; a++)
				{
					const geometry::RealBezierSegment2D& rCurrSegment(curves[a]);
					const geometry::RealBezierSegment2D& rNextSegment(curves[(a + 1) % nSize]);

					// append curved edge with the control points and the next point
					aRetval.appendBezierSegment(
						basegfx::B2DPoint(rCurrSegment.C1x, rCurrSegment.C1y),
						basegfx::B2DPoint(rCurrSegment.C2x, rCurrSegment.C2y), // #i79917# Argh! An x for an y!!
						basegfx::B2DPoint(rNextSegment.Px, rNextSegment.Py));
				}

				// rescue the control point and remove the now double-added point
				aRetval.setPrevControlPoint(0, aRetval.getPrevControlPoint(aRetval.count() - 1));
				aRetval.remove(aRetval.count() - 1);
			}

			return aRetval;
        }

        //---------------------------------------------------------------------------------------

        ::basegfx::B2DPolyPolygon polyPolygonFromBezier2DSequenceSequence( const uno::Sequence< uno::Sequence< geometry::RealBezierSegment2D > >& curves )
        {
            ::basegfx::B2DPolyPolygon aRes;

            for( sal_Int32 nCurrPoly=0; nCurrPoly<curves.getLength(); ++nCurrPoly )
            {
                aRes.append( polygonFromBezier2DSequence( curves[nCurrPoly] ) );
            }

            return aRes;
        }

        //---------------------------------------------------------------------------------------

        ::basegfx::B2DPolyPolygon b2DPolyPolygonFromXPolyPolygon2D( const uno::Reference< rendering::XPolyPolygon2D >& xPoly )
        {
            ::basegfx::unotools::UnoPolyPolygon* pPolyImpl = 
                dynamic_cast< ::basegfx::unotools::UnoPolyPolygon* >( xPoly.get() );

            if( pPolyImpl )
            {
                return pPolyImpl->getPolyPolygon();
            }
            else
            {
                // not a known implementation object - try data source
                // interfaces
                const sal_Int32 nPolys( xPoly->getNumberOfPolygons() );

                uno::Reference< rendering::XBezierPolyPolygon2D > xBezierPoly(
                    xPoly,
                    uno::UNO_QUERY );

                if( xBezierPoly.is() )
                {
                    return ::basegfx::unotools::polyPolygonFromBezier2DSequenceSequence(
                        xBezierPoly->getBezierSegments( 0,
                                                        nPolys,
                                                        0,
                                                        -1 ) );
                }
                else
                {
                    uno::Reference< rendering::XLinePolyPolygon2D > xLinePoly(
                        xPoly,
                        uno::UNO_QUERY );

                    // no implementation class and no data provider
                    // found - contract violation.
                    if( !xLinePoly.is() )
                    {
                        throw lang::IllegalArgumentException(
                            ::rtl::OUString::createFromAscii(
                                    "basegfx::unotools::b2DPolyPolygonFromXPolyPolygon2D(): Invalid input"
                                    "poly-polygon, cannot retrieve vertex data"), 
                            uno::Reference< uno::XInterface >(),
                            0 );
                    }

                    return ::basegfx::unotools::polyPolygonFromPoint2DSequenceSequence(
                        xLinePoly->getPoints( 0,
                                              nPolys,
                                              0,
                                              -1 ));
                }
            }
        }

        //---------------------------------------------------------------------------------------

        ::basegfx::B2DHomMatrix& homMatrixFromAffineMatrix( ::basegfx::B2DHomMatrix&		output,
                                                            const geometry::AffineMatrix2D&	input )
        {
            // ensure last row is [0,0,1] (and optimized away)
            output.identity();

            output.set(0,0, input.m00);
            output.set(0,1, input.m01);
            output.set(0,2, input.m02);
            output.set(1,0, input.m10);
            output.set(1,1, input.m11);
            output.set(1,2, input.m12);

            return output;
        }

        ::basegfx::B2DHomMatrix homMatrixFromAffineMatrix( const geometry::AffineMatrix2D& input )
        {
            ::basegfx::B2DHomMatrix output;

            output.set(0,0, input.m00);
            output.set(0,1, input.m01);
            output.set(0,2, input.m02);
            output.set(1,0, input.m10);
            output.set(1,1, input.m11);
            output.set(1,2, input.m12);

            return output;
        }

		::basegfx::B3DHomMatrix homMatrixFromAffineMatrix3D( const ::com::sun::star::geometry::AffineMatrix3D& input )
		{
            ::basegfx::B3DHomMatrix output;

            output.set(0,0, input.m00);
            output.set(0,1, input.m01);
            output.set(0,2, input.m02);
            output.set(0,3, input.m03);

            output.set(1,0, input.m10);
            output.set(1,1, input.m11);
            output.set(1,2, input.m12);
            output.set(1,3, input.m13);

            output.set(2,0, input.m20);
            output.set(2,1, input.m21);
            output.set(2,2, input.m22);
            output.set(2,3, input.m23);

            return output;
		}

        geometry::AffineMatrix2D& affineMatrixFromHomMatrix( geometry::AffineMatrix2D&		output,
                                                             const ::basegfx::B2DHomMatrix&	input)
        {
            output.m00 = input.get(0,0);
            output.m01 = input.get(0,1);
            output.m02 = input.get(0,2);
            output.m10 = input.get(1,0);
            output.m11 = input.get(1,1);
            output.m12 = input.get(1,2);

            return output;
        }

        geometry::AffineMatrix3D& affineMatrixFromHomMatrix3D(
			geometry::AffineMatrix3D& output,
			const ::basegfx::B3DHomMatrix&	input)
        {
            output.m00 = input.get(0,0);
            output.m01 = input.get(0,1);
            output.m02 = input.get(0,2);
            output.m03 = input.get(0,3);
            
			output.m10 = input.get(1,0);
            output.m11 = input.get(1,1);
            output.m12 = input.get(1,2);
            output.m13 = input.get(1,3);

			output.m20 = input.get(2,0);
            output.m21 = input.get(2,1);
            output.m22 = input.get(2,2);
            output.m23 = input.get(2,3);

            return output;
        }

        //---------------------------------------------------------------------------------------

        ::basegfx::B2DHomMatrix& homMatrixFromMatrix( ::basegfx::B2DHomMatrix&	output,
                                                      const geometry::Matrix2D& input )
        {
            // ensure last row is [0,0,1] (and optimized away)
            output.identity();

            output.set(0,0, input.m00);
            output.set(0,1, input.m01);
            output.set(1,0, input.m10);
            output.set(1,1, input.m11);

            return output;
        }

        //---------------------------------------------------------------------------------------

        geometry::RealSize2D size2DFromB2DSize( const ::basegfx::B2DVector& rVec )
        {
            return geometry::RealSize2D( rVec.getX(),
                                         rVec.getY() );
        }

        geometry::RealPoint2D point2DFromB2DPoint( const ::basegfx::B2DPoint& rPoint )
        {
            return geometry::RealPoint2D( rPoint.getX(),
                                          rPoint.getY() );
        }

        geometry::RealRectangle2D rectangle2DFromB2DRectangle( const ::basegfx::B2DRange& rRect )
        {
            return geometry::RealRectangle2D( rRect.getMinX(),
                                              rRect.getMinY(),
                                              rRect.getMaxX(),
                                              rRect.getMaxY() );
        }

        geometry::RealRectangle3D rectangle3DFromB3DRectangle( const ::basegfx::B3DRange& rRect )
        {
            return geometry::RealRectangle3D( rRect.getMinX(),
                                              rRect.getMinY(),
                                              rRect.getMinZ(),
                                              rRect.getMaxX(),
                                              rRect.getMaxY(),
											  rRect.getMaxZ());
        }

        ::basegfx::B2DVector b2DSizeFromRealSize2D( const geometry::RealSize2D& rSize )
        {
            return ::basegfx::B2DVector( rSize.Width,
                                         rSize.Height );
        }

        ::basegfx::B2DPoint b2DPointFromRealPoint2D( const geometry::RealPoint2D& rPoint )
        {
            return ::basegfx::B2DPoint( rPoint.X,
                                        rPoint.Y );
        }

        ::basegfx::B2DRange b2DRectangleFromRealRectangle2D( const geometry::RealRectangle2D& rRect )
        {
            return ::basegfx::B2DRange( rRect.X1,
                                        rRect.Y1,
                                        rRect.X2,
                                        rRect.Y2 );
        }

        ::basegfx::B3DRange b3DRectangleFromRealRectangle3D( const geometry::RealRectangle3D& rRect )
        {
            return ::basegfx::B3DRange( rRect.X1,
                                        rRect.Y1,
                                        rRect.Z1,
                                        rRect.X2,
                                        rRect.Y2,
										rRect.Z2);
        }

        geometry::IntegerSize2D integerSize2DFromB2ISize( const ::basegfx::B2IVector& rSize )
        {
            return geometry::IntegerSize2D( rSize.getX(),
                                            rSize.getY() );
        }

        geometry::IntegerPoint2D integerPoint2DFromB2IPoint( const ::basegfx::B2IPoint& rPoint )
        {
            return geometry::IntegerPoint2D( rPoint.getX(),
                                             rPoint.getY() );
        }

        geometry::IntegerRectangle2D integerRectangle2DFromB2IRectangle( const ::basegfx::B2IRange& rRectangle )
        {
            return geometry::IntegerRectangle2D( rRectangle.getMinX(), rRectangle.getMinY(),
                                                 rRectangle.getMaxX(), rRectangle.getMaxY() );
        }

        ::basegfx::B2IVector b2ISizeFromIntegerSize2D( const geometry::IntegerSize2D& rSize )
        {
            return ::basegfx::B2IVector( rSize.Width,
                                         rSize.Height );
        }

        ::basegfx::B2IPoint b2IPointFromIntegerPoint2D( const geometry::IntegerPoint2D& rPoint )
        {
            return ::basegfx::B2IPoint( rPoint.X,
                                        rPoint.Y );
        }

        ::basegfx::B2IRange b2IRectangleFromIntegerRectangle2D( const geometry::IntegerRectangle2D& rRectangle )
        {
            return ::basegfx::B2IRange( rRectangle.X1, rRectangle.Y1,
                                        rRectangle.X2, rRectangle.Y2 );
        }

        awt::Size awtSizeFromB2ISize( const ::basegfx::B2IVector& rVec )
        {
            return awt::Size( rVec.getX(),
                              rVec.getY() );
        }

        awt::Point awtPointFromB2IPoint( const ::basegfx::B2IPoint& rPoint )
        {
            return awt::Point( rPoint.getX(),
                               rPoint.getY() );
        }

        awt::Rectangle awtRectangleFromB2IRectangle( const ::basegfx::B2IRange& rRect )
        {
            OSL_ENSURE( rRect.getWidth() < ::std::numeric_limits< sal_Int32 >::max() &&
                        rRect.getWidth() > ::std::numeric_limits< sal_Int32 >::min(),
                        "awtRectangleFromB2IRectangle(): width overflow" );
            OSL_ENSURE( rRect.getHeight() < ::std::numeric_limits< sal_Int32 >::max() &&
                        rRect.getHeight() > ::std::numeric_limits< sal_Int32 >::min(),
                        "awtRectangleFromB2IRectangle(): height overflow" );

            return awt::Rectangle( rRect.getMinX(),
                                   rRect.getMinY(),
                                   static_cast< sal_Int32 >(rRect.getWidth()),
                                   static_cast< sal_Int32 >(rRect.getHeight()) );
        }

        ::basegfx::B2IVector b2ISizeFromAwtSize( const awt::Size& rSize )
        {
            return ::basegfx::B2IVector( rSize.Width,
                                         rSize.Height );
        }

        ::basegfx::B2IPoint b2IPointFromAwtPoint( const awt::Point& rPoint )
        {
            return ::basegfx::B2IPoint( rPoint.X,
                                        rPoint.Y );
        }

        ::basegfx::B2IRange b2IRectangleFromAwtRectangle( const awt::Rectangle& rRect )
        {
            return ::basegfx::B2IRange( rRect.X,
                                        rRect.Y,
                                        rRect.X + rRect.Width,
                                        rRect.Y + rRect.Height );
        }

        ::basegfx::B2IRange b2ISurroundingRangeFromB2DRange( const ::basegfx::B2DRange& rRange )
        {
            return ::basegfx::B2IRange( static_cast<sal_Int32>( floor(rRange.getMinX()) ),
                                        static_cast<sal_Int32>( floor(rRange.getMinY()) ),
                                        static_cast<sal_Int32>( ceil(rRange.getMaxX()) ),
                                        static_cast<sal_Int32>( ceil(rRange.getMaxY()) ) );
        }

        ::basegfx::B2DRange b2DSurroundingIntegerRangeFromB2DRange( const ::basegfx::B2DRange& rRange )
        {
            return ::basegfx::B2DRange( floor(rRange.getMinX()),
                                        floor(rRange.getMinY()),
                                        ceil(rRange.getMaxX()),
                                        ceil(rRange.getMaxY()) );
        }

        // Geometry comparisons
        // ===================================================================

		bool RealSize2DAreEqual( const ::com::sun::star::geometry::RealSize2D& rA, const ::com::sun::star::geometry::RealSize2D& rB )
		{
			return (rA.Width == rB.Width && rA.Height == rB.Height);
		}

		bool RealPoint2DAreEqual( const ::com::sun::star::geometry::RealPoint2D& rA, const ::com::sun::star::geometry::RealPoint2D& rB )
		{
			return (rA.X == rB.X && rA.Y == rB.Y);
		}

		bool RealRectangle2DAreEqual( const ::com::sun::star::geometry::RealRectangle2D& rA, const ::com::sun::star::geometry::RealRectangle2D& rB )
		{
			return (rA.X1 == rB.X1 && rA.Y1 == rB.Y1 && rA.X2 == rB.X2 && rA.Y2 == rB.Y2);
		}

		bool RealRectangle3DAreEqual( const ::com::sun::star::geometry::RealRectangle3D& rA, const ::com::sun::star::geometry::RealRectangle3D& rB )
		{
			return (rA.X1 == rB.X1 && rA.Y1 == rB.Y1 && rA.Z1 == rB.Z1 && rA.X2 == rB.X2 && rA.Y2 == rB.Y2 && rA.Z2 == rB.Z2);
		}

		bool AffineMatrix2DAreEqual( const ::com::sun::star::geometry::AffineMatrix2D& rA, const ::com::sun::star::geometry::AffineMatrix2D& rB )
		{
			return (rA.m00 == rB.m00 && rA.m01 == rB.m01 && rA.m02 == rB.m02 && rA.m10 == rB.m10 && rA.m11 == rB.m11 && rA.m12 == rB.m12);
		}

		bool IntegerSize2DAreEqual( const ::com::sun::star::geometry::IntegerSize2D& rA, const ::com::sun::star::geometry::IntegerSize2D& rB )
		{
			return (rA.Width == rB.Width && rA.Height == rB.Height);
		}

		bool IntegerPoint2DAreEqual( const ::com::sun::star::geometry::IntegerPoint2D& rA, const ::com::sun::star::geometry::IntegerPoint2D& rB )
		{
			return (rA.X == rB.X && rA.Y == rB.Y);
		}

		bool IntegerRectangle2DAreEqual( const ::com::sun::star::geometry::IntegerRectangle2D& rA, const ::com::sun::star::geometry::IntegerRectangle2D& rB )
		{
			return (rA.X1 == rB.X1 && rA.Y1 == rB.Y1 && rA.X2 == rB.X2 && rA.Y2 == rB.Y2);
		}

		bool awtSizeAreEqual( const ::com::sun::star::awt::Size& rA, const ::com::sun::star::awt::Size& rB )
		{
			return (rA.Width == rB.Width && rA.Height == rB.Height);
		}

		bool awtPointAreEqual( const ::com::sun::star::awt::Point& rA, const ::com::sun::star::awt::Point& rB )
		{
			return (rA.X == rB.X && rA.Y == rB.Y);
		}

		bool awtRectangleAreEqual( const ::com::sun::star::awt::Rectangle& rA, const ::com::sun::star::awt::Rectangle& rB )
		{
			return (rA.X == rB.X && rA.Y == rB.Y && rA.Width == rB.Width && rA.Height == rB.Height);
		}
    } // namespace bgfxtools

} // namespace canvas
