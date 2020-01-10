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

#include <sal/main.h>
#include <rtl/ref.hxx>
#include <rtl/bootstrap.hxx>

#include <cppuhelper/bootstrap.hxx>
#include <cppuhelper/servicefactory.hxx>
#include <cppuhelper/interfacecontainer.hxx> 
#include <cppuhelper/compbase1.hxx>
#include <cppuhelper/compbase2.hxx>

#include <comphelper/processfactory.hxx>
#include <comphelper/broadcasthelper.hxx>
#include <comphelper/anytostring.hxx>
#include <cppuhelper/exc_hlp.hxx>

#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/rendering/XCanvas.hpp>
#include <com/sun/star/rendering/XSpriteCanvas.hpp>
#include <com/sun/star/presentation/XSlideShow.hpp>
#include <com/sun/star/presentation/XSlideShowView.hpp>
#include "com/sun/star/animations/TransitionType.hpp"
#include "com/sun/star/animations/TransitionSubType.hpp"

#include <ucbhelper/contentbroker.hxx>
#include <ucbhelper/configurationkeys.hxx>

#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/tools/canvastools.hxx>
#include <basegfx/range/b2drectangle.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>

#include <cppcanvas/vclfactory.hxx>
#include <cppcanvas/basegfxfactory.hxx>
#include <cppcanvas/polypolygon.hxx>

#include <canvas/canvastools.hxx>

#include <vcl/dialog.hxx>
#include <vcl/timer.hxx>
#include <vcl/window.hxx>
#include <vcl/svapp.hxx>

#include <stdio.h>
#include <unistd.h>


using namespace ::com::sun::star;

namespace {

typedef ::cppu::WeakComponentImplHelper1< presentation::XSlideShowView > ViewBase;
class View : public ::comphelper::OBaseMutex,  
             public ViewBase
{
public:
    explicit View( const uno::Reference< rendering::XSpriteCanvas >& rCanvas ) : 
        ViewBase( m_aMutex ),
        mxCanvas( rCanvas ),
        maPaintListeners( m_aMutex ),
        maTransformationListeners( m_aMutex ),
        maMouseListeners( m_aMutex ),
        maMouseMotionListeners( m_aMutex ),
        maTransform(),
        maSize()
    {
    }

    void resize( const ::Size& rNewSize )
    {
        maSize = rNewSize;
        maTransform.identity();
        const sal_Int32 nSize( std::min( rNewSize.Width(),
                                         rNewSize.Height() ) - 10 );
        maTransform.scale( nSize, nSize );
        maTransform.translate( (rNewSize.Width() - nSize) / 2, 
                               (rNewSize.Height() - nSize) / 2 );

        lang::EventObject aEvent( *this ); 
        maTransformationListeners.notifyEach( &util::XModifyListener::modified,
                                              aEvent );
    }

    void repaint()
    {
        awt::PaintEvent aEvent( *this,
                                awt::Rectangle(),
                                0 ); 
        maPaintListeners.notifyEach( &awt::XPaintListener::windowPaint,
                                     aEvent );
    }

private:
    virtual ~View() {}

    virtual uno::Reference< rendering::XSpriteCanvas > SAL_CALL getCanvas(  ) throw (uno::RuntimeException)
    {
        return mxCanvas;
    }

    virtual void SAL_CALL clear(  ) throw (uno::RuntimeException)
    {
        ::basegfx::B2DPolygon aPoly( ::basegfx::tools::createPolygonFromRect(
                                         ::basegfx::B2DRectangle(0.0,0.0,
                                                                 maSize.Width(),
                                                                 maSize.Height() )));
        ::cppcanvas::SpriteCanvasSharedPtr pCanvas( 
            ::cppcanvas::VCLFactory::getInstance().createSpriteCanvas( mxCanvas ));
        if( !pCanvas )
            return;

        ::cppcanvas::PolyPolygonSharedPtr pPolyPoly( 
            ::cppcanvas::BaseGfxFactory::getInstance().createPolyPolygon( pCanvas,
                                                                          aPoly ) );
        if( !pPolyPoly )
            return;

        if( pPolyPoly )
        {
            pPolyPoly->setRGBAFillColor( 0x808080FFU );
            pPolyPoly->draw();
        }
    }

    virtual geometry::AffineMatrix2D SAL_CALL getTransformation(  ) throw (uno::RuntimeException)
    {
        geometry::AffineMatrix2D aRes;
        return basegfx::unotools::affineMatrixFromHomMatrix( aRes, 
                                                             maTransform );
    }

    virtual void SAL_CALL addTransformationChangedListener( const uno::Reference< util::XModifyListener >& xListener ) throw (uno::RuntimeException)
    {
        maTransformationListeners.addInterface( xListener );
    }

    virtual void SAL_CALL removeTransformationChangedListener( const uno::Reference< util::XModifyListener >& xListener ) throw (uno::RuntimeException)
    {
        maTransformationListeners.removeInterface( xListener );
    }

    virtual void SAL_CALL addPaintListener( const uno::Reference< awt::XPaintListener >& xListener ) throw (uno::RuntimeException)
    {
        maPaintListeners.addInterface( xListener );
    }

    virtual void SAL_CALL removePaintListener( const uno::Reference< awt::XPaintListener >& xListener ) throw (uno::RuntimeException)
    {
        maPaintListeners.removeInterface( xListener );
    }

    virtual void SAL_CALL addMouseListener( const uno::Reference< awt::XMouseListener >& xListener ) throw (uno::RuntimeException)
    {
        maMouseListeners.addInterface( xListener );
    }

    virtual void SAL_CALL removeMouseListener( const uno::Reference< awt::XMouseListener >& xListener ) throw (uno::RuntimeException)
    {
        maMouseListeners.removeInterface( xListener );
    }

    virtual void SAL_CALL addMouseMotionListener( const uno::Reference< awt::XMouseMotionListener >& xListener ) throw (uno::RuntimeException)
    {
        maMouseMotionListeners.addInterface( xListener );
    }

    virtual void SAL_CALL removeMouseMotionListener( const uno::Reference< awt::XMouseMotionListener >& xListener ) throw (uno::RuntimeException)
    {
        maMouseMotionListeners.removeInterface( xListener );
    }

    virtual void SAL_CALL setMouseCursor( ::sal_Int16 /*nPointerShape*/ ) throw (uno::RuntimeException)
    {
    }

    uno::Reference< rendering::XSpriteCanvas > mxCanvas;
    ::cppu::OInterfaceContainerHelper          maPaintListeners;
    ::cppu::OInterfaceContainerHelper          maTransformationListeners;
    ::cppu::OInterfaceContainerHelper          maMouseListeners;
    ::cppu::OInterfaceContainerHelper          maMouseMotionListeners;
    basegfx::B2DHomMatrix                      maTransform;
    Size                                       maSize;
};

typedef ::cppu::WeakComponentImplHelper2< drawing::XDrawPage, 
                                          beans::XPropertySet > SlideBase;
class DummySlide : public ::comphelper::OBaseMutex,  
                   public SlideBase
{
public:
    DummySlide() : SlideBase( m_aMutex ) {}

private:
    // XDrawPage
    virtual void SAL_CALL add( const uno::Reference< drawing::XShape >& /*xShape*/ ) throw (uno::RuntimeException)
    {
    }

    virtual void SAL_CALL remove( const uno::Reference< drawing::XShape >& /*xShape*/ ) throw (uno::RuntimeException)
    {
    }

    virtual ::sal_Int32 SAL_CALL getCount(  ) throw (uno::RuntimeException)
    {
        return 0;
    }

    virtual uno::Any SAL_CALL getByIndex( ::sal_Int32 /*Index*/ ) throw (lang::IndexOutOfBoundsException, lang::WrappedTargetException, uno::RuntimeException)
    {
        return uno::Any();
    }

    virtual uno::Type SAL_CALL getElementType(  ) throw (uno::RuntimeException)
    {
        return uno::Type();
    }

    virtual ::sal_Bool SAL_CALL hasElements(  ) throw (uno::RuntimeException)
    {
        return false;
    }

    // XPropertySet
    virtual uno::Reference< beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) throw (uno::RuntimeException)
    {
        return uno::Reference< beans::XPropertySetInfo >();
    }

    virtual void SAL_CALL setPropertyValue( const ::rtl::OUString& /*aPropertyName*/, 
                                            const uno::Any& /*aValue*/ ) throw (beans::UnknownPropertyException, beans::PropertyVetoException, lang::IllegalArgumentException, lang::WrappedTargetException, uno::RuntimeException)
    {
    }

    virtual uno::Any SAL_CALL getPropertyValue( const ::rtl::OUString& PropertyName ) throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
    {
        typedef ::canvas::tools::ValueMap< sal_Int16 > PropMapT;

        // fixed PropertyValue map
        static PropMapT::MapEntry lcl_propertyMap[] =
            {
                {"Height",               100},
                {"MinimalFrameNumber",   50},
                {"TransitionDuration",   10},
                {"TransitionSubtype",    animations::TransitionSubType::FROMTOPLEFT},
                {"TransitionType",       animations::TransitionType::PUSHWIPE},
                {"Width",                100}
            };

        static PropMapT aMap( lcl_propertyMap,
                              sizeof(lcl_propertyMap)/sizeof(*lcl_propertyMap),
                              true );

        sal_Int16 aRes;            
        if( !aMap.lookup( PropertyName, aRes ))
            return uno::Any();
                
        return uno::makeAny(aRes);
    }

    virtual void SAL_CALL addPropertyChangeListener( const ::rtl::OUString& /*aPropertyName*/, 
                                                     const uno::Reference< beans::XPropertyChangeListener >& /*xListener*/ ) throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
    {
    }

    virtual void SAL_CALL removePropertyChangeListener( const ::rtl::OUString& /*aPropertyName*/, 
                                                        const uno::Reference< beans::XPropertyChangeListener >& /*aListener*/ ) throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
    {
    }

    virtual void SAL_CALL addVetoableChangeListener( const ::rtl::OUString& /*PropertyName*/, 
                                                     const uno::Reference< beans::XVetoableChangeListener >& /*aListener*/ ) throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
    {
    }

    virtual void SAL_CALL removeVetoableChangeListener( const ::rtl::OUString& /*PropertyName*/, 
                                                        const uno::Reference< beans::XVetoableChangeListener >& /*aListener*/ ) throw (beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException)
    {
    }
};


class DemoApp : public Application
{
public:
	virtual void Main();
	virtual USHORT	Exception( USHORT nError );
};

class ChildWindow : public Window
{
public:
    ChildWindow( Window* pParent );
    virtual ~ChildWindow();
    virtual void Paint( const Rectangle& rRect );
    virtual void Resize();

    void setShow( const uno::Reference< presentation::XSlideShow >& rShow ) { mxShow = rShow; init(); }
    
private:
    void init();

    rtl::Reference< View >                     mpView;
    uno::Reference< presentation::XSlideShow > mxShow;
};

ChildWindow::ChildWindow( Window* pParent ) :
    Window(pParent, WB_CLIPCHILDREN | WB_BORDER| WB_3DLOOK ),
    mpView(),
    mxShow()
{
    EnablePaint( true );
    Show();
}

ChildWindow::~ChildWindow()
{
    if( mxShow.is() && mpView.is() )
        mxShow->removeView( mpView.get() );
}

void ChildWindow::init()
{
	try
	{
        if( !mpView.is() )
        {
            uno::Reference< rendering::XCanvas > xCanvas( GetCanvas(),
                                                          uno::UNO_QUERY_THROW );
            uno::Reference< rendering::XSpriteCanvas > xSpriteCanvas( xCanvas, 
                                                                      uno::UNO_QUERY_THROW );
            mpView = new View( xSpriteCanvas );
            mpView->resize( GetSizePixel() );

            if( mxShow.is() )
                mxShow->addView( mpView.get() );
        }
	}
	catch (const uno::Exception &e)
	{
		OSL_TRACE( "Exception '%s' thrown\n" ,
                   (const sal_Char*)::rtl::OUStringToOString( e.Message, 
                                                              RTL_TEXTENCODING_UTF8 ));
	}
}

void ChildWindow::Paint( const Rectangle& /*rRect*/ )
{
	try
	{
		if( mpView.is() )
            mpView->repaint();
	}
	catch (const uno::Exception &e)
	{
		OSL_TRACE( "Exception '%s' thrown\n" ,
                   (const sal_Char*)::rtl::OUStringToOString( e.Message, 
                                                              RTL_TEXTENCODING_UTF8 ));
	}
}

void ChildWindow::Resize()
{
    if( mpView.is() )
        mpView->resize( GetSizePixel() );
}

class DemoWindow : public Dialog
{
public:
    DemoWindow();
    virtual void Paint( const Rectangle& rRect );
    virtual void Resize();
    
private:
    void init();
	DECL_LINK( updateHdl, Timer* );

    ChildWindow                                maLeftChild; 
    ChildWindow                                maRightTopChild; 
    ChildWindow                                maRightBottomChild; 
    uno::Reference< presentation::XSlideShow > mxShow;
	AutoTimer                                  maUpdateTimer;
    bool                                       mbSlideDisplayed;
};

DemoWindow::DemoWindow() :    
    Dialog((Window*)NULL),
    maLeftChild( this ),
    maRightTopChild( this ),
    maRightBottomChild( this ),
    mxShow(),
    maUpdateTimer(),
    mbSlideDisplayed( false )
{
    SetText( rtl::OUString::createFromAscii( "Slideshow Demo" ) );
    SetSizePixel( Size( 640, 480 ) );
    EnablePaint( true );

    maLeftChild.SetPosSizePixel( Point(), Size(320,480) );
    maRightTopChild.SetPosSizePixel( Point(320,0), Size(320,240) );
    maRightBottomChild.SetPosSizePixel( Point(320,240), Size(320,240) );
    Show();

	maUpdateTimer.SetTimeoutHdl(LINK(this, DemoWindow, updateHdl));
	maUpdateTimer.SetTimeout( (ULONG)30 );
	maUpdateTimer.Start();
}

void DemoWindow::init()
{
	try
	{
        if( !mxShow.is() )
        {
            uno::Reference< lang::XMultiServiceFactory > xFactory( 
                ::comphelper::getProcessServiceFactory(),
                uno::UNO_QUERY_THROW );

            uno::Reference< uno::XInterface > xInt( xFactory->createInstance( 
                                                        ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("com.sun.star.presentation.SlideShow")) ));

            mxShow.set( xInt, 
                        uno::UNO_QUERY_THROW );

            maLeftChild.setShow( mxShow );
            maRightTopChild.setShow( mxShow );
            maRightBottomChild.setShow( mxShow );
        }

        if( mxShow.is() && !mbSlideDisplayed )
        {
            uno::Reference< drawing::XDrawPage > xSlide( new DummySlide );
            mxShow->displaySlide( xSlide,
                                  uno::Reference< animations::XAnimationNode >(), 
                                  uno::Sequence< beans::PropertyValue >() );
            mxShow->setProperty( beans::PropertyValue( 
                                     rtl::OUString::createFromAscii("RehearseTimings"),
                                     0,
                                     uno::makeAny( sal_True ),
                                     beans::PropertyState_DIRECT_VALUE ));
            mbSlideDisplayed = true;
        }
	}
	catch (const uno::Exception &e)
	{
		OSL_TRACE( "Exception '%s' thrown\n" ,
                   (const sal_Char*)::rtl::OUStringToOString( e.Message, 
                                                              RTL_TEXTENCODING_UTF8 ));
	}
}

IMPL_LINK( DemoWindow, updateHdl, Timer*, EMPTYARG )
{
    init();

    double nTimeout;
    if( mxShow.is() )
        mxShow->update(nTimeout);

    return 0;
}

void DemoWindow::Paint( const Rectangle& /*rRect*/ )
{
    init();
}

void DemoWindow::Resize()
{
    // TODO
}

USHORT DemoApp::Exception( USHORT nError )
{
	switch( nError & EXC_MAJORTYPE )
	{
		case EXC_RSCNOTLOADED:
			Abort( String::CreateFromAscii( "Error: could not load language resources.\nPlease check your installation.\n" ) );
			break;
	}
	return 0;
}

void DemoApp::Main()
{
	bool bHelp = false;

	for( USHORT i = 0; i < GetCommandLineParamCount(); i++ )
	{
		::rtl::OUString aParam = GetCommandLineParam( i );

		if( aParam.equalsAscii( "--help" ) ||
			aParam.equalsAscii( "-h" ) )
				bHelp = true;
	}

	if( bHelp )
	{
        printf( "demoshow - life Slideshow testbed\n" );
		return;
	}

    // bootstrap UNO
    uno::Reference< lang::XMultiServiceFactory > xFactory;
    try
    {
        uno::Reference< uno::XComponentContext > xCtx = ::cppu::defaultBootstrap_InitialComponentContext();
        xFactory = uno::Reference< lang::XMultiServiceFactory >(  xCtx->getServiceManager(), 
                                                                  uno::UNO_QUERY );
        if( xFactory.is() )
            ::comphelper::setProcessServiceFactory( xFactory );
    }
    catch( uno::RuntimeException& )
    {
        throw;
    }
    catch( uno::Exception& )
    {
        OSL_ENSURE( false,
                    rtl::OUStringToOString(
                        comphelper::anyToString( cppu::getCaughtException() ),
                        RTL_TEXTENCODING_UTF8 ).getStr() );
    }

    if( !xFactory.is() )
    {
        OSL_TRACE( "Could not bootstrap UNO, installation must be in disorder. Exiting.\n" );
        exit( 1 );
    }

    // Create UCB.
    uno::Sequence< uno::Any > aArgs( 2 );
	aArgs[ 0 ] <<= rtl::OUString::createFromAscii( UCB_CONFIGURATION_KEY1_LOCAL );
	aArgs[ 1 ] <<= rtl::OUString::createFromAscii( UCB_CONFIGURATION_KEY2_OFFICE );
    ::ucbhelper::ContentBroker::initialize( xFactory, aArgs );

	DemoWindow pWindow;
	pWindow.Execute();

    // clean up UCB
	::ucbhelper::ContentBroker::deinitialize();
}
}

DemoApp aApp;
