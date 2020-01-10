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

#ifndef INCLUDED_CANVAS_CANVASCUSTOMSPRITEBASE_HXX
#define INCLUDED_CANVAS_CANVASCUSTOMSPRITEBASE_HXX

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/rendering/XCustomSprite.hpp>
#include <com/sun/star/rendering/XPolyPolygon2D.hpp>
#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/vector/b2dvector.hxx>
#include <basegfx/range/b2drange.hxx>
#include <canvas/base/integerbitmapbase.hxx>
#include <canvas/base/sprite.hxx>

#include <boost/utility.hpp>


namespace canvas
{
    /** Helper template to handle XCustomSprite method forwarding to
        CanvasCustomSpriteHelper

    	Use this helper to handle the XCustomSprite part of your
    	implementation.

        @tpl Base
        Base class to use, most probably one of the
        WeakComponentImplHelperN templates with the appropriate
        interfaces. At least XCustomSprite and Sprite should be among
        them (why else would you use this template, then?). Base class
        must have an Base( const Mutex& ) constructor (like the
        WeakComponentImplHelperN templates have).

        @tpl SpriteHelper
        Sprite helper implementation for the backend in question

        @tpl CanvasHelper
        Canvas helper implementation for the backend in question

        @tpl Mutex
        Lock strategy to use. Defaults to using the
        OBaseMutex-provided lock.  Everytime one of the methods is
        entered, an object of type Mutex is created with m_aMutex as
        the sole parameter, and destroyed again when the method scope
        is left.

        @tpl UnambiguousBase
        Optional unambiguous base class for XInterface of Base. It's
        sometimes necessary to specify this parameter, e.g. if Base
        derives from multiple UNO interface (were each provides its
        own version of XInterface, making the conversion ambiguous)

        @see CanvasCustomSpriteHelper for further contractual
        requirements towards the SpriteHelper type, and some examples.
     */
    template< class Base, 
              class SpriteHelper, 
              class CanvasHelper, 
              class Mutex=::osl::MutexGuard, 
              class UnambiguousBase=::com::sun::star::uno::XInterface > class CanvasCustomSpriteBase : 
        public IntegerBitmapBase< Base, CanvasHelper, Mutex, UnambiguousBase >
    {
    public:
        typedef IntegerBitmapBase< Base, CanvasHelper, Mutex, UnambiguousBase >	BaseType;
        typedef SpriteHelper													SpriteHelperType;

        CanvasCustomSpriteBase() :
            maSpriteHelper()
        {
        }

        /** Object is being disposed.

        	Called from the cppu helper base, to notify disposal of
        	this object. Already releases all internal references.

            @derive when overriding this method in derived classes,
            <em>always</em> call the base class' method!
         */
        virtual void SAL_CALL disposing()
        {
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            maSpriteHelper.disposing();

            // pass on to base class
            BaseType::disposing();
        }

        // XCanvas: selectively override base's methods here, for opacity tracking
        virtual void SAL_CALL clear() throw (::com::sun::star::uno::RuntimeException)
        {
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            maSpriteHelper.clearingContent( this );

            // and forward to base class, which handles the actual rendering
            return BaseType::clear();
        }

        virtual ::com::sun::star::uno::Reference< ::com::sun::star::rendering::XCachedPrimitive > SAL_CALL 	
        	drawBitmap( const ::com::sun::star::uno::Reference< ::com::sun::star::rendering::XBitmap >& xBitmap, 
                        const ::com::sun::star::rendering::ViewState& 									viewState, 
                        const ::com::sun::star::rendering::RenderState& 								renderState ) throw (::com::sun::star::lang::IllegalArgumentException, 
                                                                                                                             ::com::sun::star::uno::RuntimeException)
        {
            tools::verifyArgs(xBitmap, viewState, renderState, 
                              BOOST_CURRENT_FUNCTION,
                              static_cast< typename BaseType::UnambiguousBaseType* >(this));
        
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            maSpriteHelper.checkDrawBitmap( this, xBitmap, viewState, renderState );

            // and forward to base class, which handles the actual rendering
            return BaseType::drawBitmap( xBitmap,
                                         viewState,
                                         renderState );
        }

        // TODO(F3): If somebody uses the XIntegerBitmap methods to
        // clear pixel (setting alpha != 1.0 there), or a compositing
        // mode results in similar alpha, maSpriteHelper might
        // errorneously report fully opaque sprites. Effectively, all
        // render methods must be overridden here; or better,
        // functionality provided at the baseclass.

        // XSprite
        virtual void SAL_CALL setAlpha( double alpha ) throw (::com::sun::star::lang::IllegalArgumentException, 
                                                              ::com::sun::star::uno::RuntimeException)
        {
            tools::verifyRange( alpha, 0.0, 1.0 );
            
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            maSpriteHelper.setAlpha( this, alpha );
        }

        virtual void SAL_CALL move( const ::com::sun::star::geometry::RealPoint2D& 	aNewPos, 
                                    const ::com::sun::star::rendering::ViewState& 	viewState, 
                                    const ::com::sun::star::rendering::RenderState& renderState ) throw (::com::sun::star::lang::IllegalArgumentException, 
                                                                                                         ::com::sun::star::uno::RuntimeException)
        {
            tools::verifyArgs(aNewPos, viewState, renderState, 
                              BOOST_CURRENT_FUNCTION,
                              static_cast< typename BaseType::UnambiguousBaseType* >(this));

            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            maSpriteHelper.move( this, aNewPos, viewState, renderState );
        }

        virtual void SAL_CALL transform( const ::com::sun::star::geometry::AffineMatrix2D& aTransformation ) throw (::com::sun::star::lang::IllegalArgumentException, 
                                                                                                                    ::com::sun::star::uno::RuntimeException)
        {
            tools::verifyArgs(aTransformation, 
                              BOOST_CURRENT_FUNCTION,
                              static_cast< typename BaseType::UnambiguousBaseType* >(this));

            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            maSpriteHelper.transform( this, aTransformation );
        }

        virtual void SAL_CALL clip( const ::com::sun::star::uno::Reference< ::com::sun::star::rendering::XPolyPolygon2D >& aClip ) throw (::com::sun::star::uno::RuntimeException)
        {
            // NULL xClip explicitely allowed here (to clear clipping)

            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            maSpriteHelper.clip( this, aClip );
        }

        virtual void SAL_CALL setPriority( double nPriority ) throw (::com::sun::star::uno::RuntimeException)
        {
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            maSpriteHelper.setPriority( this, nPriority );
        }

        virtual void SAL_CALL show() throw (::com::sun::star::uno::RuntimeException)
        {
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            maSpriteHelper.show( this );
        }

        virtual void SAL_CALL hide() throw (::com::sun::star::uno::RuntimeException)
        {
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            maSpriteHelper.hide( this );
        }

        // XCustomSprite
        virtual ::com::sun::star::uno::Reference< ::com::sun::star::rendering::XCanvas > SAL_CALL 
        	getContentCanvas() throw (::com::sun::star::uno::RuntimeException)
        {
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            return this;
        }

        // Sprite
        virtual bool isAreaUpdateOpaque( const ::basegfx::B2DRange& rUpdateArea ) const
        {
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            return maSpriteHelper.isAreaUpdateOpaque( rUpdateArea );
        }

        virtual bool isContentChanged() const
        {
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            return BaseType::mbSurfaceDirty;
        }

        virtual ::basegfx::B2DPoint getPosPixel() const
        {
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            return maSpriteHelper.getPosPixel();
        }

        virtual ::basegfx::B2DVector getSizePixel() const
        {
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            return maSpriteHelper.getSizePixel();
        }

        virtual ::basegfx::B2DRange getUpdateArea() const
        {
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            return maSpriteHelper.getUpdateArea();
        }

        virtual double getPriority() const
        {
            typename BaseType::MutexType aGuard( BaseType::m_aMutex );

            return maSpriteHelper.getPriority();
        }

    protected:
        SpriteHelperType maSpriteHelper;
    };
}

#endif /* INCLUDED_CANVAS_CANVASCUSTOMSPRITEBASE_HXX */
