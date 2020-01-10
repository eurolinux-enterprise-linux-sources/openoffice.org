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
#include "precompiled_canvas.hxx"

#include <canvas/debug.hxx>
#include <tools/diagnose_ex.h>
#include <canvas/canvastools.hxx>

#include <com/sun/star/rendering/CompositeOperation.hpp>
#include <com/sun/star/rendering/TextDirection.hpp>

#include <vcl/metric.hxx>
#include <vcl/virdev.hxx>

#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/numeric/ftools.hxx>
#include <basegfx/tools/canvastools.hxx>

#include "impltools.hxx"
#include "textlayout.hxx"

#include <boost/scoped_array.hpp>


using namespace ::com::sun::star;

namespace vclcanvas
{
    namespace
    {
        void setupLayoutMode( OutputDevice& rOutDev,
                              sal_Int8		nTextDirection )	
        {
            // TODO(P3): avoid if already correctly set
            ULONG nLayoutMode;
            switch( nTextDirection )
            {
                default:
                    nLayoutMode = 0;
                    break;
                case rendering::TextDirection::WEAK_LEFT_TO_RIGHT:
                    nLayoutMode = TEXT_LAYOUT_BIDI_LTR;
                    break;
                case rendering::TextDirection::STRONG_LEFT_TO_RIGHT:
                    nLayoutMode = TEXT_LAYOUT_BIDI_LTR | TEXT_LAYOUT_BIDI_STRONG;
                    break;
                case rendering::TextDirection::WEAK_RIGHT_TO_LEFT:
                    nLayoutMode = TEXT_LAYOUT_BIDI_RTL;
                    break;
                case rendering::TextDirection::STRONG_RIGHT_TO_LEFT:
                    nLayoutMode = TEXT_LAYOUT_BIDI_RTL | TEXT_LAYOUT_BIDI_STRONG;
                    break;
            }

            // set calculated layout mode. Origin is always the left edge,
            // as required at the API spec
            rOutDev.SetLayoutMode( nLayoutMode | TEXT_LAYOUT_TEXTORIGIN_LEFT );
        }
    }

    TextLayout::TextLayout( const rendering::StringContext&                  aText,
                            sal_Int8                                         nDirection,
                            sal_Int64                                        nRandomSeed,
                            const CanvasFont::Reference&                     rFont,
                            const uno::Reference<rendering::XGraphicDevice>& xDevice,
                            const OutDevProviderSharedPtr&                   rOutDev ) :
        TextLayout_Base( m_aMutex ),
        maText( aText ),
        maLogicalAdvancements(),
        mpFont( rFont ),
        mxDevice( xDevice ),
        mpOutDevProvider( rOutDev ),
        mnTextDirection( nDirection )
    {
        (void)nRandomSeed;
    }

    void SAL_CALL TextLayout::disposing()
    {
        tools::LocalGuard aGuard;

        mpOutDevProvider.reset();
        mxDevice.clear();
        mpFont.reset();
    }

    // XTextLayout
    uno::Sequence< uno::Reference< rendering::XPolyPolygon2D > > SAL_CALL TextLayout::queryTextShapes(  ) throw (uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        OutputDevice& rOutDev = mpOutDevProvider->getOutDev();
        VirtualDevice aVDev( rOutDev );
        aVDev.SetFont( mpFont->getVCLFont() );

        setupLayoutMode( aVDev, mnTextDirection );

        const rendering::ViewState aViewState(
            geometry::AffineMatrix2D(1,0,0, 0,1,0),
            NULL);

        rendering::RenderState aRenderState (
            geometry::AffineMatrix2D(1,0,0,0,1,0),
            NULL,
            uno::Sequence<double>(4),
            rendering::CompositeOperation::SOURCE);

        ::boost::scoped_array< sal_Int32 > aOffsets(new sal_Int32[maLogicalAdvancements.getLength()]);
        setupTextOffsets(aOffsets.get(), maLogicalAdvancements, aViewState, aRenderState);

        uno::Sequence< uno::Reference< rendering::XPolyPolygon2D> > aOutlineSequence;
        ::basegfx::B2DPolyPolygonVector aOutlines;
        if (aVDev.GetTextOutlines(
            aOutlines,
            maText.Text,
            ::canvas::tools::numeric_cast<USHORT>(maText.StartPosition),
            ::canvas::tools::numeric_cast<USHORT>(maText.StartPosition),
            ::canvas::tools::numeric_cast<USHORT>(maText.Length),
            FALSE,
            0,
            aOffsets.get()))
        {
            aOutlineSequence.realloc(aOutlines.size());
            sal_Int32 nIndex (0);
            for (::basegfx::B2DPolyPolygonVector::const_iterator
                     iOutline(aOutlines.begin()),
                     iEnd(aOutlines.end());
                 iOutline!=iEnd;
                 ++iOutline)
            {
                aOutlineSequence[nIndex++] = ::basegfx::unotools::xPolyPolygonFromB2DPolyPolygon(
                    mxDevice,
                    *iOutline);
            }
        }

        return aOutlineSequence;
    }

    uno::Sequence< geometry::RealRectangle2D > SAL_CALL TextLayout::queryInkMeasures(  ) throw (uno::RuntimeException)
    {
        tools::LocalGuard aGuard;


        OutputDevice& rOutDev = mpOutDevProvider->getOutDev();
        VirtualDevice aVDev( rOutDev );
        aVDev.SetFont( mpFont->getVCLFont() );

        setupLayoutMode( aVDev, mnTextDirection );

        const rendering::ViewState aViewState(
            geometry::AffineMatrix2D(1,0,0, 0,1,0),
            NULL);

        rendering::RenderState aRenderState (
            geometry::AffineMatrix2D(1,0,0,0,1,0),
            NULL,
            uno::Sequence<double>(4),
            rendering::CompositeOperation::SOURCE);

        ::boost::scoped_array< sal_Int32 > aOffsets(new sal_Int32[maLogicalAdvancements.getLength()]);
        setupTextOffsets(aOffsets.get(), maLogicalAdvancements, aViewState, aRenderState);

        MetricVector aMetricVector;
        uno::Sequence<geometry::RealRectangle2D> aBoundingBoxes;
        if (aVDev.GetGlyphBoundRects(
            Point(0,0),
            maText.Text,
            ::canvas::tools::numeric_cast<USHORT>(maText.StartPosition),
            ::canvas::tools::numeric_cast<USHORT>(maText.Length),
            ::canvas::tools::numeric_cast<USHORT>(maText.StartPosition),
            aMetricVector))
        {
            aBoundingBoxes.realloc(aMetricVector.size());
            sal_Int32 nIndex (0);
            for (MetricVector::const_iterator
                     iMetric(aMetricVector.begin()),
                     iEnd(aMetricVector.end());
                 iMetric!=iEnd;
                 ++iMetric)
            {
                aBoundingBoxes[nIndex++] = geometry::RealRectangle2D(
                    iMetric->getX(),
                    iMetric->getY(),
                    iMetric->getX() + iMetric->getWidth(),
                    iMetric->getY() + iMetric->getHeight());
            }
        }
        return aBoundingBoxes;
    }

    uno::Sequence< geometry::RealRectangle2D > SAL_CALL TextLayout::queryMeasures(  ) throw (uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        // TODO(F1)
        return uno::Sequence< geometry::RealRectangle2D >();
    }

    uno::Sequence< double > SAL_CALL TextLayout::queryLogicalAdvancements(  ) throw (uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        return maLogicalAdvancements;
    }

    void SAL_CALL TextLayout::applyLogicalAdvancements( const uno::Sequence< double >& aAdvancements ) throw (lang::IllegalArgumentException, uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        ENSURE_ARG_OR_THROW( aAdvancements.getLength() == maText.Length,
                         "TextLayout::applyLogicalAdvancements(): mismatching number of advancements" );

        maLogicalAdvancements = aAdvancements;
    }

    geometry::RealRectangle2D SAL_CALL TextLayout::queryTextBounds(  ) throw (uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

    	if( !mpOutDevProvider )
            return geometry::RealRectangle2D();

        OutputDevice& rOutDev = mpOutDevProvider->getOutDev();

        VirtualDevice aVDev( rOutDev );
        aVDev.SetFont( mpFont->getVCLFont() );

        // need metrics for Y offset, the XCanvas always renders
        // relative to baseline
        const ::FontMetric& aMetric( aVDev.GetFontMetric() );

        setupLayoutMode( aVDev, mnTextDirection );

        const sal_Int32 nAboveBaseline( /*-aMetric.GetIntLeading()*/ - aMetric.GetAscent() );
        const sal_Int32 nBelowBaseline( aMetric.GetDescent() );

        if( maLogicalAdvancements.getLength() )
        {
            return geometry::RealRectangle2D( 0, nAboveBaseline,
                                              maLogicalAdvancements[ maLogicalAdvancements.getLength()-1 ],
                                              nBelowBaseline );
        }
        else
        {
            return geometry::RealRectangle2D( 0, nAboveBaseline,
                                              aVDev.GetTextWidth(
                                                  maText.Text,
                                                  ::canvas::tools::numeric_cast<USHORT>(maText.StartPosition),
                                                  ::canvas::tools::numeric_cast<USHORT>(maText.Length) ),
                                              nBelowBaseline );
        }
    }

    double SAL_CALL TextLayout::justify( double nSize ) throw (lang::IllegalArgumentException, uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        (void)nSize;

        // TODO(F1)
        return 0.0;
    }

    double SAL_CALL TextLayout::combinedJustify( const uno::Sequence< uno::Reference< rendering::XTextLayout > >& aNextLayouts, 
                                                 double                                                           nSize ) throw (lang::IllegalArgumentException, uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        (void)aNextLayouts;
        (void)nSize;

        // TODO(F1)
        return 0.0;
    }

    rendering::TextHit SAL_CALL TextLayout::getTextHit( const geometry::RealPoint2D& aHitPoint ) throw (uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        (void)aHitPoint;

        // TODO(F1)
        return rendering::TextHit();
    }

    rendering::Caret SAL_CALL TextLayout::getCaret( sal_Int32 nInsertionIndex, sal_Bool bExcludeLigatures ) throw (lang::IndexOutOfBoundsException, uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        (void)nInsertionIndex;
        (void)bExcludeLigatures;

        // TODO(F1)
        return rendering::Caret();
    }

    sal_Int32 SAL_CALL TextLayout::getNextInsertionIndex( sal_Int32 nStartIndex, sal_Int32 nCaretAdvancement, sal_Bool bExcludeLigatures ) throw (lang::IndexOutOfBoundsException, uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        (void)nStartIndex;
        (void)nCaretAdvancement;
        (void)bExcludeLigatures;

        // TODO(F1)
        return 0;
    }

    uno::Reference< rendering::XPolyPolygon2D > SAL_CALL TextLayout::queryVisualHighlighting( sal_Int32 nStartIndex, sal_Int32 nEndIndex ) throw (lang::IndexOutOfBoundsException, uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        (void)nStartIndex;
        (void)nEndIndex;

        // TODO(F1)
        return uno::Reference< rendering::XPolyPolygon2D >();
    }

    uno::Reference< rendering::XPolyPolygon2D > SAL_CALL TextLayout::queryLogicalHighlighting( sal_Int32 nStartIndex, sal_Int32 nEndIndex ) throw (lang::IndexOutOfBoundsException, uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        (void)nStartIndex;
        (void)nEndIndex;

        // TODO(F1)
        return uno::Reference< rendering::XPolyPolygon2D >();
    }

    double SAL_CALL TextLayout::getBaselineOffset(  ) throw (uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        // TODO(F1)
        return 0.0;
    }

    sal_Int8 SAL_CALL TextLayout::getMainTextDirection(  ) throw (uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        return mnTextDirection;
    }

    uno::Reference< rendering::XCanvasFont > SAL_CALL TextLayout::getFont(  ) throw (uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        return mpFont.getRef();
    }

    rendering::StringContext SAL_CALL TextLayout::getText(  ) throw (uno::RuntimeException)
    {
        tools::LocalGuard aGuard;

        return maText;
    }

    bool TextLayout::draw( OutputDevice&                 rOutDev,
                           const Point&                  rOutpos,
                           const rendering::ViewState&   viewState,
                           const rendering::RenderState& renderState ) const
    {
        tools::LocalGuard aGuard;

        setupLayoutMode( rOutDev, mnTextDirection );

        if( maLogicalAdvancements.getLength() )
        {
            // TODO(P2): cache that
            ::boost::scoped_array< sal_Int32 > aOffsets(new sal_Int32[maLogicalAdvancements.getLength()]);
            setupTextOffsets( aOffsets.get(), maLogicalAdvancements, viewState, renderState );
            
            // TODO(F3): ensure correct length and termination for DX
            // array (last entry _must_ contain the overall width)
            
            rOutDev.DrawTextArray( rOutpos,
                                   maText.Text,
                                   aOffsets.get(),
                                   ::canvas::tools::numeric_cast<USHORT>(maText.StartPosition),
                                   ::canvas::tools::numeric_cast<USHORT>(maText.Length) );
        }
        else
        {
            rOutDev.DrawText( rOutpos,
                              maText.Text,
                              ::canvas::tools::numeric_cast<USHORT>(maText.StartPosition),
                              ::canvas::tools::numeric_cast<USHORT>(maText.Length) );
        }

        return true;
    }

    namespace
    {
        class OffsetTransformer
        {
        public:
            OffsetTransformer( const ::basegfx::B2DHomMatrix& rMat ) :
                maMatrix( rMat )
            {
            }

            sal_Int32 operator()( const double& rOffset )
            {
                // This is an optimization of the normal rMat*[x,0]
                // transformation of the advancement vector (in x
                // direction), followed by a length calculation of the
                // resulting vector: advancement' =
                // ||rMat*[x,0]||. Since advancements are vectors, we
                // can ignore translational components, thus if [x,0],
                // it follows that rMat*[x,0]=[x',0] holds. Thus, we
                // just have to calc the transformation of the x
                // component.

                // TODO(F2): Handle non-horizontal advancements!
                return ::basegfx::fround( hypot(maMatrix.get(0,0)*rOffset,
												maMatrix.get(1,0)*rOffset) );
            }

        private:
            ::basegfx::B2DHomMatrix maMatrix;
        };
    }

    void TextLayout::setupTextOffsets( sal_Int32*						outputOffsets,
                                       const uno::Sequence< double >& 	inputOffsets,
                                       const rendering::ViewState& 		viewState,
                                       const rendering::RenderState& 	renderState		) const
    {
        ENSURE_OR_THROW( outputOffsets!=NULL,
                          "TextLayout::setupTextOffsets offsets NULL" );

        ::basegfx::B2DHomMatrix aMatrix;

        ::canvas::tools::mergeViewAndRenderTransform(aMatrix,
                                                     viewState,
                                                     renderState);

        // fill integer offsets
        ::std::transform( const_cast< uno::Sequence< double >& >(inputOffsets).getConstArray(),
                          const_cast< uno::Sequence< double >& >(inputOffsets).getConstArray()+inputOffsets.getLength(),
                          outputOffsets,
                          OffsetTransformer( aMatrix ) );
    }


#define IMPLEMENTATION_NAME "VCLCanvas::TextLayout"
#define SERVICE_NAME "com.sun.star.rendering.TextLayout"

    ::rtl::OUString SAL_CALL TextLayout::getImplementationName() throw( uno::RuntimeException )
    {
        return ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( IMPLEMENTATION_NAME ) );
    }

    sal_Bool SAL_CALL TextLayout::supportsService( const ::rtl::OUString& ServiceName ) throw( uno::RuntimeException )
    {
        return ServiceName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM ( SERVICE_NAME ) );
    }

    uno::Sequence< ::rtl::OUString > SAL_CALL TextLayout::getSupportedServiceNames()  throw( uno::RuntimeException )
    {
        uno::Sequence< ::rtl::OUString > aRet(1);
        aRet[0] = ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM ( SERVICE_NAME ) );

        return aRet;
    }
}
