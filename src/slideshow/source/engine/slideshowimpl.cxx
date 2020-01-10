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
#include "precompiled_slideshow.hxx"

#include <canvas/debug.hxx>
#include <tools/diagnose_ex.h>

#include <cppuhelper/basemutex.hxx>
#include <cppuhelper/compbase1.hxx>
#include <cppuhelper/factory.hxx>
#include <cppuhelper/implementationentry.hxx>
#include <cppuhelper/compbase2.hxx>
#include <cppuhelper/interfacecontainer.h>
#include <cppuhelper/exc_hlp.hxx>

#include <comphelper/anytostring.hxx>
#include <comphelper/make_shared_from_uno.hxx>
#include <comphelper/scopeguard.hxx>
#include <comphelper/optional.hxx>
#include <comphelper/servicedecl.hxx>

#include <cppcanvas/spritecanvas.hxx>
#include <cppcanvas/vclfactory.hxx>
#include <cppcanvas/basegfxfactory.hxx>

#include <tools/debug.hxx>

#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/polygon/b2dpolypolygontools.hxx>
#include <basegfx/tools/canvastools.hxx>

#include <vcl/font.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/util/XModifyListener.hpp>
#include <com/sun/star/util/XUpdatable.hpp>
#include <com/sun/star/awt/XPaintListener.hpp>
#include <com/sun/star/awt/SystemPointer.hpp>
#include <com/sun/star/animations/TransitionType.hpp>
#include <com/sun/star/animations/TransitionSubType.hpp>
#include <com/sun/star/presentation/XSlideShow.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XServiceName.hpp>
#include <com/sun/star/loader/CannotActivateFactoryException.hpp>

#include "unoviewcontainer.hxx"
#include "transitionfactory.hxx"
#include "eventmultiplexer.hxx"
#include "usereventqueue.hxx"
#include "eventqueue.hxx"
#include "cursormanager.hxx"
#include "slideshowcontext.hxx"
#include "activitiesqueue.hxx"
#include "activitiesfactory.hxx"
#include "interruptabledelayevent.hxx"
#include "slide.hxx"
#include "shapemaps.hxx"
#include "slideview.hxx"
#include "tools.hxx"
#include "unoview.hxx"
#include "slidebitmap.hxx"
#include "rehearsetimingsactivity.hxx"
#include "waitsymbol.hxx"
#include "framerate.hxx"

#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>

#include <map>
#include <vector>
#include <iterator>
#include <algorithm>
#include <stdio.h>

using namespace com::sun::star;
using namespace ::slideshow::internal;

namespace {

/******************************************************************************
   
   SlideShowImpl

   This class encapsulates the slideshow presentation viewer.
    
   With an instance of this class, it is possible to statically
   and dynamically show a presentation, as defined by the
   constructor-provided draw model (represented by a sequence
   of ::com::sun::star::drawing::XDrawPage objects).
    
   It is possible to show the presentation on multiple views
   simultaneously (e.g. for a multi-monitor setup). Since this
   class also relies on user interaction, the corresponding
   XSlideShowView interface provides means to register some UI
   event listeners (mostly borrowed from awt::XWindow interface).
   
   Since currently (mid 2004), OOo isn't very well suited to
   multi-threaded rendering, this class relies on <em>very
   frequent</em> external update() calls, which will render the
   next frame of animations. This works as follows: after the
   displaySlide() has been successfully called (which setup and
   starts an actual slide show), the update() method must be
   called until it returns false.
   Effectively, this puts the burden of providing
   concurrency to the clients of this class, which, as noted
   above, is currently unavoidable with the current state of
   affairs (I've actually tried threading here, but failed
   miserably when using the VCL canvas as the render backend -
   deadlocked).
   
 ******************************************************************************/

typedef cppu::WeakComponentImplHelper1<presentation::XSlideShow> SlideShowImplBase;

class SlideShowImpl : private cppu::BaseMutex,
                      public CursorManager,
                      public SlideShowImplBase
{
public:
    explicit SlideShowImpl(
        uno::Reference<uno::XComponentContext> const& xContext );

    /** Notify that the transition phase of the current slide
        has ended.

        The life of a slide has three phases: the transition
        phase, when the previous slide vanishes, and the
        current slide becomes visible, the shape animation
        phase, when shape effects are running, and the phase
        after the last shape animation has ended, but before
        the next slide transition starts.

        This method notifies the end of the first phase.

        @param bPaintSlide
        When true, Slide::show() is passed a true as well, denoting
        explicit paint of slide content. Pass false here, if e.g. a
        slide transition has already rendered the initial slide image.
    */
    void notifySlideTransitionEnded( bool bPaintSlide );

    /** Notify that the shape animation phase of the current slide
        has ended.

        The life of a slide has three phases: the transition
        phase, when the previous slide vanishes, and the
        current slide becomes visible, the shape animation
        phase, when shape effects are running, and the phase
        after the last shape animation has ended, but before
        the next slide transition starts.

        This method notifies the end of the second phase.
    */
    void notifySlideAnimationsEnded();

    /** Notify that the slide has ended.

        The life of a slide has three phases: the transition
        phase, when the previous slide vanishes, and the
        current slide becomes visible, the shape animation
        phase, when shape effects are running, and the phase
        after the last shape animation has ended, but before
        the next slide transition starts.
        
        This method notifies the end of the third phase.
    */
    void notifySlideEnded();

    /** Notification from eventmultiplexer that a hyperlink
        has been clicked.
    */
    bool notifyHyperLinkClicked( rtl::OUString const& hyperLink );
    
    /** Notification from eventmultiplexer that an animation event has occoured.
		This will be forewarded to all registered XSlideShowListener
     */
	bool handleAnimationEvent( const AnimationNodeSharedPtr& rNode );

private:
    // XSlideShow:
    virtual sal_Bool SAL_CALL nextEffect() throw (uno::RuntimeException);
    virtual sal_Bool SAL_CALL startShapeActivity(
        uno::Reference<drawing::XShape> const& xShape )
        throw (uno::RuntimeException);
    virtual sal_Bool SAL_CALL stopShapeActivity(
        uno::Reference<drawing::XShape> const& xShape )
        throw (uno::RuntimeException);
    virtual sal_Bool SAL_CALL pause( sal_Bool bPauseShow )
        throw (uno::RuntimeException);
    virtual uno::Reference<drawing::XDrawPage> SAL_CALL getCurrentSlide()
        throw (uno::RuntimeException);
    virtual void SAL_CALL displaySlide(
        uno::Reference<drawing::XDrawPage> const& xSlide,
        uno::Reference<animations::XAnimationNode> const& xRootNode,
        uno::Sequence<beans::PropertyValue> const& rProperties )
        throw (uno::RuntimeException);
    virtual sal_Bool SAL_CALL setProperty(
        beans::PropertyValue const& rProperty ) throw (uno::RuntimeException);
    virtual sal_Bool SAL_CALL addView(
        uno::Reference<presentation::XSlideShowView> const& xView )
        throw (uno::RuntimeException);
    virtual sal_Bool SAL_CALL removeView(
        uno::Reference<presentation::XSlideShowView> const& xView )
        throw (uno::RuntimeException);
    virtual sal_Bool SAL_CALL update( double & nNextTimeout )
        throw (uno::RuntimeException);
    virtual void SAL_CALL addSlideShowListener(
        uno::Reference<presentation::XSlideShowListener> const& xListener )
        throw (uno::RuntimeException);
    virtual void SAL_CALL removeSlideShowListener(
        uno::Reference<presentation::XSlideShowListener> const& xListener )
        throw (uno::RuntimeException);
    virtual void SAL_CALL addShapeEventListener(
        uno::Reference<presentation::XShapeEventListener> const& xListener,
        uno::Reference<drawing::XShape> const& xShape )
        throw (uno::RuntimeException);
    virtual void SAL_CALL removeShapeEventListener(
        uno::Reference<presentation::XShapeEventListener> const& xListener,
        uno::Reference<drawing::XShape> const& xShape )
        throw (uno::RuntimeException);
    virtual void SAL_CALL setShapeCursor(
        uno::Reference<drawing::XShape> const& xShape, sal_Int16 nPointerShape )
        throw (uno::RuntimeException);


    // CursorManager
    // -----------------------------------------------------------

    virtual bool requestCursor( sal_Int16 nCursorShape );
    virtual void resetCursor();

    
protected:
    // WeakComponentImplHelperBase
    virtual void SAL_CALL disposing();    
    
    bool isDisposed() const 
    {
        return (rBHelper.bDisposed || rBHelper.bInDispose);
    }
    
private:
    struct SeparateListenerImpl; friend struct SeparateListenerImpl;
    struct PrefetchPropertiesFunc; friend struct PrefetchPropertiesFunc;
    
    /// Stop currently running show.
    void stopShow();
    
    /// Creates a new slide.
    SlideSharedPtr makeSlide(
        uno::Reference<drawing::XDrawPage> const& xDrawPage,
        uno::Reference<animations::XAnimationNode> const& xRootNode );
    
    /// Checks whether the given slide/animation node matches mpPrefetchSlide
    static bool matches(
        SlideSharedPtr const& pSlide,
        uno::Reference<drawing::XDrawPage> const& xSlide,
        uno::Reference<animations::XAnimationNode> const& xNode )
    {
        if (pSlide)
            return (pSlide->getXDrawPage() == xSlide &&
                    pSlide->getXAnimationNode() == xNode);
        else
            return (!xSlide.is() && !xNode.is());
    }
    
    /// Resets the current slide transition sound object with a new one:
    SoundPlayerSharedPtr resetSlideTransitionSound(
		uno::Any const& url = uno::Any(), bool bLoopSound = false );
    
	/// stops the current slide transition sound
	void stopSlideTransitionSound();

    /** Prepare a slide transition
                
        This method registers all necessary events and
        activities for a slide transition.
        
        @return the slide change activity, or NULL for no transition effect
    */
    ActivitySharedPtr createSlideTransition(
        const uno::Reference< drawing::XDrawPage >&    xDrawPage,
        const SlideSharedPtr&                          rLeavingSlide,
        const SlideSharedPtr&                          rEnteringSlide,
        const EventSharedPtr&                          rTransitionEndEvent );
    
    /// Display/hide wait symbol on all views
    void setWaitState( bool bOn );

    /// Filter requested cursor shape against hard slideshow cursors (wait, etc.)
    sal_Int16 calcActiveCursor( sal_Int16 nCursorShape ) const;

    /// all registered views
    UnoViewContainer                        maViewContainer;
    
    /// all registered slide show listeners
    cppu::OInterfaceContainerHelper         maListenerContainer;
    
    /// map of vectors, containing all registered listeners for a shape
    ShapeEventListenerMap                   maShapeEventListeners;
    /// map of sal_Int16 values, specifying the mouse cursor for every shape
    ShapeCursorMap                          maShapeCursors;
    
    boost::optional<RGBColor>               maUserPaintColor;

    boost::shared_ptr<canvas::tools::ElapsedTime> mpPresTimer;
    ScreenUpdater                           maScreenUpdater;
    EventQueue                              maEventQueue;
    EventMultiplexer                        maEventMultiplexer;
    ActivitiesQueue                         maActivitiesQueue;
    UserEventQueue                          maUserEventQueue;
    SubsettableShapeManagerSharedPtr        mpDummyPtr;

    boost::shared_ptr<SeparateListenerImpl> mpListener;

    boost::shared_ptr<RehearseTimingsActivity> mpRehearseTimingsActivity;
    boost::shared_ptr<WaitSymbol>           mpWaitSymbol;

    /// the current slide transition sound object:
    SoundPlayerSharedPtr                    mpCurrentSlideTransitionSound;
    
    uno::Reference<uno::XComponentContext>  mxComponentContext;
    uno::Reference<
        presentation::XTransitionFactory>   mxOptionalTransitionFactory;

    /// the previously running slide
    SlideSharedPtr                          mpPreviousSlide;
    /// the currently running slide
    SlideSharedPtr                          mpCurrentSlide;
    /// the already prefetched slide: best candidate for upcoming slide
    SlideSharedPtr                          mpPrefetchSlide;
    /// slide to be prefetched: best candidate for upcoming slide
    uno::Reference<drawing::XDrawPage>      mxPrefetchSlide;
    /// slide animation to be prefetched:
    uno::Reference<animations::XAnimationNode> mxPrefetchAnimationNode;

    sal_Int16                               mnCurrentCursor;
    
    bool                                    mbWaitState;
    bool                                    mbAutomaticAdvancementMode;
    bool                                    mbImageAnimationsAllowed;
    bool                                    mbNoSlideTransitions;
    bool                                    mbMouseVisible;
    bool                                    mbForceManualAdvance;
    bool                                    mbShowPaused;
    bool                                    mbSlideShowIdle;
    bool                                    mbDisableAnimationZOrder;
};


/** Separate event listener for animation, view and hyperlink events.

    This handler is registered for slide animation end, view and
    hyperlink events at the global EventMultiplexer, and forwards
    notifications to the SlideShowImpl
*/
struct SlideShowImpl::SeparateListenerImpl : public EventHandler,
                                             public ViewRepaintHandler,
                                             public HyperlinkHandler,
											 public AnimationEventHandler,
                                             private boost::noncopyable
{
    SlideShowImpl& mrShow;
    ScreenUpdater& mrScreenUpdater;
    EventQueue&    mrEventQueue;

    SeparateListenerImpl( SlideShowImpl& rShow, 
                          ScreenUpdater& rScreenUpdater,
                          EventQueue&    rEventQueue ) :
        mrShow( rShow ), 
        mrScreenUpdater( rScreenUpdater ),
        mrEventQueue( rEventQueue ) 
    {}

    // EventHandler
    virtual bool handleEvent()
    {
        // DON't call notifySlideAnimationsEnded()
        // directly, but queue an event. handleEvent()
        // might be called from e.g.
        // showNext(), and notifySlideAnimationsEnded() must not be called
        // in recursion.
        mrEventQueue.addEvent( 
            makeEvent( boost::bind( &SlideShowImpl::notifySlideAnimationsEnded,
                                    boost::ref(mrShow) )));
        return true;
    }

    // ViewRepaintHandler
    virtual void viewClobbered( const UnoViewSharedPtr& rView )
    {
        // given view needs repaint, request update
        mrScreenUpdater.notifyUpdate(rView, true);
    }

    // HyperlinkHandler
    virtual bool handleHyperlink( ::rtl::OUString const& rLink )
    {
        return mrShow.notifyHyperLinkClicked(rLink);
    }

	// AnimationEventHandler
    virtual bool handleAnimationEvent( const AnimationNodeSharedPtr& rNode )
	{
		return mrShow.handleAnimationEvent(rNode);
	}
};


SlideShowImpl::SlideShowImpl(
    uno::Reference<uno::XComponentContext> const& xContext )
    : SlideShowImplBase(m_aMutex),
      maViewContainer(),
      maListenerContainer( m_aMutex ),
      maShapeEventListeners(),
      maShapeCursors(),
      maUserPaintColor(),
      mpPresTimer( new canvas::tools::ElapsedTime ),
      maScreenUpdater(maViewContainer),
      maEventQueue( mpPresTimer ),
      maEventMultiplexer( maEventQueue,
                          maViewContainer ),
      maActivitiesQueue( mpPresTimer ),
      maUserEventQueue( maEventMultiplexer, 
                        maEventQueue,
                        *this ),
      mpDummyPtr(),
      mpListener(),
      mpRehearseTimingsActivity(),
      mpWaitSymbol(),
      mpCurrentSlideTransitionSound(),
      mxComponentContext( xContext ),
      mxOptionalTransitionFactory(),
      mpCurrentSlide(),
      mpPrefetchSlide(),
      mxPrefetchSlide(),
      mxPrefetchAnimationNode(),
      mnCurrentCursor(awt::SystemPointer::ARROW),
      mbWaitState(false),
      mbAutomaticAdvancementMode(false),
      mbImageAnimationsAllowed( true ),
      mbNoSlideTransitions( false ),
      mbMouseVisible( true ),
      mbForceManualAdvance( false ),
      mbShowPaused( false ),
      mbSlideShowIdle( true ),
      mbDisableAnimationZOrder( false )
{
    // keep care not constructing any UNO references to this inside ctor,
    // shift that code to create()!
   
    uno::Reference<lang::XMultiComponentFactory> xFactory( 
        mxComponentContext->getServiceManager() );

    if( xFactory.is() )
    {
        try
	{
            // #i82460# try to retrieve special transition factory
            mxOptionalTransitionFactory.set( 
                xFactory->createInstanceWithContext( 
                    ::rtl::OUString::createFromAscii( "com.sun.star.presentation.TransitionFactory" ),
                    mxComponentContext ), 
                uno::UNO_QUERY );
        }
        catch (loader::CannotActivateFactoryException const&)
	{
	}
    }

    mpListener.reset( new SeparateListenerImpl(
                          *this, 
                          maScreenUpdater,
                          maEventQueue ));
    maEventMultiplexer.addSlideAnimationsEndHandler( mpListener );
    maEventMultiplexer.addViewRepaintHandler( mpListener );
    maEventMultiplexer.addHyperlinkHandler( mpListener, 0.0 );
	maEventMultiplexer.addAnimationStartHandler( mpListener );
	maEventMultiplexer.addAnimationEndHandler( mpListener );
}

// we are about to be disposed (someone call dispose() on us)
void SlideShowImpl::disposing()
{
    osl::MutexGuard const guard( m_aMutex );

	// stop slide transition sound, if any:
    stopSlideTransitionSound();

    mxComponentContext.clear();

    if( mpCurrentSlideTransitionSound )
    {
        mpCurrentSlideTransitionSound->dispose();
        mpCurrentSlideTransitionSound.reset();
    }

    mpWaitSymbol.reset();

    if( mpRehearseTimingsActivity ) 
    {
        mpRehearseTimingsActivity->dispose();
        mpRehearseTimingsActivity.reset();
    }

    if( mpListener ) 
    {
        maEventMultiplexer.removeSlideAnimationsEndHandler(mpListener);
        maEventMultiplexer.removeViewRepaintHandler(mpListener);
        maEventMultiplexer.removeHyperlinkHandler(mpListener);
		maEventMultiplexer.removeAnimationStartHandler( mpListener );
		maEventMultiplexer.removeAnimationEndHandler( mpListener );

        mpListener.reset();
    }    

    maUserEventQueue.clear();
    maActivitiesQueue.clear();
    maEventMultiplexer.clear();
    maEventQueue.clear();
    mpPresTimer.reset();
    maShapeCursors.clear();
    maShapeEventListeners.clear();

    // send all listeners a disposing() that we are going down:
    maListenerContainer.disposeAndClear(
        lang::EventObject( static_cast<cppu::OWeakObject *>(this) ) );

    maViewContainer.dispose();

    // release slides:
    mxPrefetchAnimationNode.clear();
    mxPrefetchSlide.clear();
    mpPrefetchSlide.reset();
    mpCurrentSlide.reset();
    mpPreviousSlide.reset();
}

/// stops the current slide transition sound
void SlideShowImpl::stopSlideTransitionSound()
{
    if (mpCurrentSlideTransitionSound) 
    {
        mpCurrentSlideTransitionSound->stopPlayback();
        mpCurrentSlideTransitionSound->dispose();
        mpCurrentSlideTransitionSound.reset();
    }
 }

SoundPlayerSharedPtr SlideShowImpl::resetSlideTransitionSound( const uno::Any& rSound, bool bLoopSound )
{
	sal_Bool bStopSound = sal_False;
	rtl::OUString url;

	if( !(rSound >>= bStopSound) )
		bStopSound = sal_False;
	rSound >>= url;

	if( !bStopSound && (url.getLength() == 0) )
		return SoundPlayerSharedPtr();

	stopSlideTransitionSound();

    if (url.getLength() > 0) 
    {
        try 
        {
            mpCurrentSlideTransitionSound = SoundPlayer::create(
                maEventMultiplexer, url, mxComponentContext );
			mpCurrentSlideTransitionSound->setPlaybackLoop( bLoopSound );
        }
        catch (lang::NoSupportException const&) 
        {
            // catch possible exceptions from SoundPlayer, since
            // being not able to playback the sound is not a hard
            // error here (still, the slide transition should be
            // shown).
        }
    }
    return mpCurrentSlideTransitionSound;
}

ActivitySharedPtr SlideShowImpl::createSlideTransition(
    const uno::Reference< drawing::XDrawPage >& xDrawPage,
    const SlideSharedPtr&                       rLeavingSlide,
    const SlideSharedPtr&                       rEnteringSlide,
    const EventSharedPtr&                       rTransitionEndEvent )
{
    ENSURE_OR_THROW( !maViewContainer.empty(),
                      "createSlideTransition(): No views" );
    ENSURE_OR_THROW( rEnteringSlide,
                      "createSlideTransition(): No entering slide" );
    
    // return empty transition, if slide transitions
    // are disabled.
    if (mbNoSlideTransitions)
        return ActivitySharedPtr();
    
    // retrieve slide change parameters from XDrawPage
    uno::Reference< beans::XPropertySet > xPropSet( xDrawPage,
                                                    uno::UNO_QUERY );

    if( !xPropSet.is() )
    {
        OSL_TRACE( "createSlideTransition(): "
                   "Slide has no PropertySet - assuming no transition\n" );
        return ActivitySharedPtr();
    }

    sal_Int16 nTransitionType(0);
    if( !getPropertyValue( nTransitionType,
                           xPropSet,
                           OUSTR("TransitionType" )) )
    {
        OSL_TRACE( "createSlideTransition(): "
                   "Could not extract slide transition type from XDrawPage - assuming no transition\n" );
        return ActivitySharedPtr();
    }
    
    sal_Int16 nTransitionSubType(0);
    if( !getPropertyValue( nTransitionSubType,
                           xPropSet,
                           OUSTR("TransitionSubtype" )) )
    {
        OSL_TRACE( "createSlideTransition(): "
                   "Could not extract slide transition subtype from XDrawPage - assuming no transition\n" );
        return ActivitySharedPtr();
    }
    
    bool bTransitionDirection(false);
    if( !getPropertyValue( bTransitionDirection,
                           xPropSet,
                           OUSTR("TransitionDirection")) )
    {
        OSL_TRACE( "createSlideTransition(): "
                   "Could not extract slide transition direction from XDrawPage - assuming default direction\n" );
    }
    
    sal_Int32 aUnoColor(0);
    if( !getPropertyValue( aUnoColor,
                           xPropSet,
                           OUSTR("TransitionFadeColor")) )
    {
        OSL_TRACE( "createSlideTransition(): "
                   "Could not extract slide transition fade color from XDrawPage - assuming black\n" );
    }

    const RGBColor aTransitionFadeColor( unoColor2RGBColor( aUnoColor ));

	uno::Any aSound;
	sal_Bool bLoopSound = sal_False;

	if( !getPropertyValue( aSound, xPropSet, OUSTR("Sound")) )
		OSL_TRACE( "createSlideTransition(): Could not determine transition sound effect URL from XDrawPage - using no sound\n" );

	if( !getPropertyValue( bLoopSound, xPropSet, OUSTR("LoopSound") ) )
		OSL_TRACE( "createSlideTransition(): Could not get slide property 'LoopSound' - using no sound\n" );
    
    NumberAnimationSharedPtr pTransition(
        TransitionFactory::createSlideTransition(
            rLeavingSlide,
            rEnteringSlide,
            maViewContainer,
            maScreenUpdater,
            maEventMultiplexer,
            mxOptionalTransitionFactory,
            nTransitionType,
            nTransitionSubType,
            bTransitionDirection,
            aTransitionFadeColor,
            resetSlideTransitionSound( aSound, bLoopSound ) ));
    
    if( !pTransition )
        return ActivitySharedPtr(); // no transition effect has been
                                    // generated. Normally, that means
                                    // that simply no transition is
                                    // set on this slide.

    double nTransitionDuration(0.0);
    if( !getPropertyValue( nTransitionDuration,
                           xPropSet,
                           OUSTR("TransitionDuration")) )
    {
        OSL_TRACE( "createSlideTransition(): "
                   "Could not extract slide transition duration from XDrawPage - assuming no transition\n" );
        return ActivitySharedPtr();
    }
    
    sal_Int32 nMinFrames(5);
    if( !getPropertyValue( nMinFrames,
                           xPropSet,
                           OUSTR("MinimalFrameNumber")) )
    {
        OSL_TRACE( "createSlideTransition(): "
                   "No minimal number of frames given - assuming 5\n" );
    }
    
    // prefetch slide transition bitmaps, but postpone it after
    // displaySlide() has finished - sometimes, view size has not yet
    // reached final size
    maEventQueue.addEvent( 
        makeEvent( 
            boost::bind(
                &::slideshow::internal::Animation::prefetch,
                pTransition,
                AnimatableShapeSharedPtr(),
                ShapeAttributeLayerSharedPtr())));

    return ActivitySharedPtr(
        ActivitiesFactory::createSimpleActivity(
            ActivitiesFactory::CommonParameters(
                rTransitionEndEvent,
                maEventQueue,
                maActivitiesQueue,
                nTransitionDuration,
                nMinFrames,
                false,
                boost::optional<double>(1.0),
                0.0,
                0.0,
                ShapeSharedPtr(),
                rEnteringSlide->getSlideSize() ),
            pTransition,
            true ));
}

SlideSharedPtr SlideShowImpl::makeSlide(
    uno::Reference<drawing::XDrawPage> const& xDrawPage,
    uno::Reference<animations::XAnimationNode> const& xRootNode )
{
    if (! xDrawPage.is())
        return SlideSharedPtr();
    
    const SlideSharedPtr pSlide( createSlide(xDrawPage, 
                                             xRootNode,
                                             maEventQueue,
                                             maEventMultiplexer,
                                             maScreenUpdater,
                                             maActivitiesQueue,
                                             maUserEventQueue,
                                             *this,
                                             maViewContainer,
                                             mxComponentContext,
                                             maShapeEventListeners,
                                             maShapeCursors,
                                             maUserPaintColor ? *maUserPaintColor : RGBColor(),
                                             !!maUserPaintColor,
                                             mbImageAnimationsAllowed,
                                             mbDisableAnimationZOrder) );
    
    // prefetch show content (reducing latency for slide 
    // bitmap and effect start later on)
    pSlide->prefetch();

    return pSlide;
}

void SlideShowImpl::setWaitState( bool bOn )
{
    mbWaitState = bOn;
    if( !mpWaitSymbol ) // fallback to cursor
        requestCursor(awt::SystemPointer::WAIT);
    else if( mbWaitState )
        mpWaitSymbol->show();
    else
        mpWaitSymbol->hide();
}

sal_Int16 SlideShowImpl::calcActiveCursor( sal_Int16 nCursorShape ) const
{
    if( mbWaitState && !mpWaitSymbol ) // enforce wait cursor
        nCursorShape = awt::SystemPointer::WAIT;
    else if( !mbMouseVisible ) // enforce INVISIBLE
        nCursorShape = awt::SystemPointer::INVISIBLE;
    else if( maUserPaintColor &&
             nCursorShape == awt::SystemPointer::ARROW )
        nCursorShape = awt::SystemPointer::PEN;

    return nCursorShape;
}
    

void SlideShowImpl::stopShow()
{
    // Force-end running animation
    // ===========================
    if (mpCurrentSlide)
        mpCurrentSlide->hide();
    
    // clear all queues
    maEventQueue.clear();
    maActivitiesQueue.clear();

    // Attention: we MUST clear the user event queue here,
    // this is because the current slide might have registered
    // shape events (click or enter/leave), which might
    // otherwise dangle forever in the queue (because of the
    // shared ptr nature). If someone needs to change this:
    // somehow unregister those shapes at the user event queue
    // on notifySlideEnded().
    maUserEventQueue.clear();

    // re-enable automatic effect advancement
    // (maEventQueue.clear() above might have killed
    // maEventMultiplexer's tick events)
    if (mbAutomaticAdvancementMode) 
    {
        // toggle automatic mode (enabling just again is
        // ignored by EventMultiplexer)
        maEventMultiplexer.setAutomaticMode( false );
        maEventMultiplexer.setAutomaticMode( true );
    }
}

struct SlideShowImpl::PrefetchPropertiesFunc 
{
    SlideShowImpl *const that;
    PrefetchPropertiesFunc( SlideShowImpl * that_ ) : that(that_) {}
    void operator()( beans::PropertyValue const& rProperty ) const {
        if (rProperty.Name.equalsAsciiL(
                RTL_CONSTASCII_STRINGPARAM("Prefetch") )) 
        {
            uno::Sequence<uno::Any> seq;
            if ((rProperty.Value >>= seq) && seq.getLength() == 2) 
            {
                seq[0] >>= that->mxPrefetchSlide;
                seq[1] >>= that->mxPrefetchAnimationNode;
            }
        }
        else 
        {
            OSL_ENSURE( false, rtl::OUStringToOString(
                            rProperty.Name, RTL_TEXTENCODING_UTF8 ).getStr() );
        }
    }
};

void SlideShowImpl::displaySlide(
    uno::Reference<drawing::XDrawPage> const& xSlide,
    uno::Reference<animations::XAnimationNode> const& xRootNode,
    uno::Sequence<beans::PropertyValue> const& rProperties )
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );

    if (isDisposed())
        return;
    
    // precondition: must only be called from the main thread!
    DBG_TESTSOLARMUTEX();

    stopShow();  // MUST call that: results in
    // maUserEventQueue.clear(). What's more,
    // stopShow()'s currSlide->hide() call is
    // now also required, notifySlideEnded()
    // relies on that
    // unconditionally. Otherwise, genuine
    // shape animations (drawing layer and
    // GIF) will not be stopped.
    
    std::for_each( rProperties.getConstArray(),
                   rProperties.getConstArray() + rProperties.getLength(),
                   PrefetchPropertiesFunc(this) );
    
    OSL_ENSURE( !maViewContainer.empty(), "### no views!" );
    if (maViewContainer.empty())
        return;
    
    // this here might take some time
    {
        comphelper::ScopeGuard const scopeGuard(
            boost::bind( &SlideShowImpl::setWaitState, this, false ) );
        setWaitState(true);
        
        mpPreviousSlide = mpCurrentSlide;
        mpCurrentSlide.reset();

        if (matches( mpPrefetchSlide, xSlide, xRootNode )) 
        {
            // prefetched slide matches:
            mpCurrentSlide = mpPrefetchSlide;
        }
        else 
        {
            mpCurrentSlide = makeSlide( xSlide, xRootNode );
        }
        
        OSL_ASSERT( mpCurrentSlide );
        if (mpCurrentSlide) 
        {
            basegfx::B2DSize oldSlideSize;
            if( mpPreviousSlide )
                oldSlideSize = mpPreviousSlide->getSlideSize();

            basegfx::B2DSize const slideSize( mpCurrentSlide->getSlideSize() );

            // push new transformation to all views, if size changed
            if( !mpPreviousSlide || oldSlideSize != slideSize )
            {
                std::for_each( maViewContainer.begin(), 
                               maViewContainer.end(),
                               boost::bind( &View::setViewSize, _1,
                                            boost::cref(slideSize) ));
            
                // explicitly notify view change here,
                // because transformation might have changed:
                // optimization, this->notifyViewChange() would
                // repaint slide which is not necessary.
                maEventMultiplexer.notifyViewsChanged();
            }
            
            // create slide transition, and add proper end event 
            // (which then starts the slide effects
            // via CURRENT_SLIDE.show())
            ActivitySharedPtr const pSlideChangeActivity(
                createSlideTransition( mpCurrentSlide->getXDrawPage(),
                                       mpPreviousSlide,
                                       mpCurrentSlide,
                                       makeEvent( 
                                           boost::bind(
                                               &SlideShowImpl::notifySlideTransitionEnded,
                                               this,
                                               false ))));
            
            if (pSlideChangeActivity) 
            {
                // factory generated a slide transition - activate it!
                maActivitiesQueue.addActivity( pSlideChangeActivity );
            }
            else 
            {
                // no transition effect on this slide - schedule slide
                // effect start event right away.
                maEventQueue.addEvent( 
                    makeEvent( 
                        boost::bind(
                            &SlideShowImpl::notifySlideTransitionEnded,
                            this,
                            true )));
            }
        }
    } // finally

    maEventMultiplexer.notifySlideTransitionStarted();
    maListenerContainer.forEach<presentation::XSlideShowListener>(
        boost::mem_fn( &presentation::XSlideShowListener::slideTransitionStarted ) );
}

sal_Bool SlideShowImpl::nextEffect() throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );

    if (isDisposed())
        return false;
    
    // precondition: must only be called from the main thread!
    DBG_TESTSOLARMUTEX();

    if (mbShowPaused)
        return true;
    else
        return maEventMultiplexer.notifyNextEffect();
}

sal_Bool SlideShowImpl::startShapeActivity(
    uno::Reference<drawing::XShape> const& /*xShape*/ )
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );

    // precondition: must only be called from the main thread!
    DBG_TESTSOLARMUTEX();

    // TODO(F3): NYI
    OSL_ENSURE( false, "not yet implemented!" );
    return false;
}

sal_Bool SlideShowImpl::stopShapeActivity(
    uno::Reference<drawing::XShape> const& /*xShape*/ )
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );

    // precondition: must only be called from the main thread!
    DBG_TESTSOLARMUTEX();

    // TODO(F3): NYI
    OSL_ENSURE( false, "not yet implemented!" );
    return false;
}

sal_Bool SlideShowImpl::pause( sal_Bool bPauseShow )
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );

    if (isDisposed())
        return false;
    
    // precondition: must only be called from the main thread!
    DBG_TESTSOLARMUTEX();


    if (bPauseShow)
        mpPresTimer->pauseTimer();
    else
        mpPresTimer->continueTimer();
    
    maEventMultiplexer.notifyPauseMode(bPauseShow);
    
    mbShowPaused = bPauseShow;
    return true;
}

uno::Reference<drawing::XDrawPage> SlideShowImpl::getCurrentSlide()
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );

    if (isDisposed())
        return uno::Reference<drawing::XDrawPage>();
    
    // precondition: must only be called from the main thread!
    DBG_TESTSOLARMUTEX();

    if (mpCurrentSlide)
        return mpCurrentSlide->getXDrawPage();
    else
        return uno::Reference<drawing::XDrawPage>();
}

sal_Bool SlideShowImpl::addView(
    uno::Reference<presentation::XSlideShowView> const& xView )
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );

    if (isDisposed())
        return false;
    
    // precondition: must only be called from the main thread!
    DBG_TESTSOLARMUTEX();

    // first of all, check if view has a valid canvas
    ENSURE_OR_RETURN( xView.is(), "addView(): Invalid view" );
    ENSURE_OR_RETURN( xView->getCanvas().is(),
                       "addView(): View does not provide a valid canvas" );
    
    UnoViewSharedPtr const pView( createSlideView(
                                      xView, 
                                      maEventQueue,
                                      maEventMultiplexer ));
    if (!maViewContainer.addView( pView ))
        return false; // view already added
    
    // initialize view content
    // =======================

    if (mpCurrentSlide) 
    {
        // set view transformation
        const basegfx::B2ISize slideSize = mpCurrentSlide->getSlideSize();
        pView->setViewSize( basegfx::B2DSize( slideSize.getX(),
                                              slideSize.getY() ) );
    }

    // clear view area (since its newly added, 
    // we need a clean slate)
    pView->clearAll();

    // broadcast newly added view
    maEventMultiplexer.notifyViewAdded( pView );

    // set current mouse ptr
    pView->setCursorShape( calcActiveCursor(mnCurrentCursor) );

    return true;
}

sal_Bool SlideShowImpl::removeView(
    uno::Reference<presentation::XSlideShowView> const& xView )
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );
    
    // precondition: must only be called from the main thread!
    DBG_TESTSOLARMUTEX();

    ENSURE_OR_RETURN( xView.is(), "removeView(): Invalid view" );
    
    UnoViewSharedPtr const pView( maViewContainer.removeView( xView ) );
    if( !pView )
        return false; // view was not added in the first place
    
    // remove view from EventMultiplexer (mouse events etc.)
    maEventMultiplexer.notifyViewRemoved( pView );
    
    pView->_dispose();

    return true;
}

sal_Bool SlideShowImpl::setProperty( beans::PropertyValue const& rProperty )
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );

    if (isDisposed())
        return false;

    // precondition: must only be called from the main thread!
    DBG_TESTSOLARMUTEX();

    if (rProperty.Name.equalsAsciiL(
            RTL_CONSTASCII_STRINGPARAM("AutomaticAdvancement") ))
    {
        double nTimeout(0.0);
        mbAutomaticAdvancementMode = (rProperty.Value >>= nTimeout);
        if (mbAutomaticAdvancementMode) 
        {
            maEventMultiplexer.setAutomaticTimeout( nTimeout );
        }
        maEventMultiplexer.setAutomaticMode( mbAutomaticAdvancementMode );
        return true;
    }
    
    if (rProperty.Name.equalsAsciiL(
            RTL_CONSTASCII_STRINGPARAM("UserPaintColor") ))
    {
        sal_Int32 nColor(0);
        if (rProperty.Value >>= nColor) 
        {
            OSL_ENSURE( mbMouseVisible,
                        "setProperty(): User paint overrides invisible mouse" );

            // enable user paint
            maUserPaintColor.reset( unoColor2RGBColor( nColor ) );
            maEventMultiplexer.notifyUserPaintColor( *maUserPaintColor );
        }
        else 
        {
            // disable user paint
            maUserPaintColor.reset();
            maEventMultiplexer.notifyUserPaintDisabled();
        }

        if( mnCurrentCursor == awt::SystemPointer::ARROW )
            resetCursor();

        return true;
    }
    
    if (rProperty.Name.equalsAsciiL(
            RTL_CONSTASCII_STRINGPARAM("AdvanceOnClick") )) 
    {
        sal_Bool bAdvanceOnClick = sal_False;
        if (! (rProperty.Value >>= bAdvanceOnClick))
            return false;
        maUserEventQueue.setAdvanceOnClick( bAdvanceOnClick );
        return true;
    }            
    
    if (rProperty.Name.equalsAsciiL(
            RTL_CONSTASCII_STRINGPARAM("DisableAnimationZOrder") )) 
    {
        sal_Bool bDisableAnimationZOrder = sal_False;
        if (! (rProperty.Value >>= bDisableAnimationZOrder))
            return false;
        mbDisableAnimationZOrder = bDisableAnimationZOrder == sal_True;
        return true;
    }            
    
    if (rProperty.Name.equalsAsciiL(
            RTL_CONSTASCII_STRINGPARAM("ImageAnimationsAllowed") ) )
    {
        if (! (rProperty.Value >>= mbImageAnimationsAllowed))
            return false;

        // TODO(F3): Forward to slides!
//         if( bOldValue != mbImageAnimationsAllowed )
//         {
//             if( mbImageAnimationsAllowed )
//                 maEventMultiplexer.notifyIntrinsicAnimationsEnabled();
//             else
//                 maEventMultiplexer.notifyIntrinsicAnimationsDisabled();
//         }

        return true;
    }
    
    if (rProperty.Name.equalsAsciiL(
            RTL_CONSTASCII_STRINGPARAM("MouseVisible") ))
    {
        if (! (rProperty.Value >>= mbMouseVisible))
            return false;

        requestCursor(mnCurrentCursor);

        return true;
    }
    
    if (rProperty.Name.equalsAsciiL(
            RTL_CONSTASCII_STRINGPARAM("ForceManualAdvance") )) 
    {
        return (rProperty.Value >>= mbForceManualAdvance);
    }
    
    if (rProperty.Name.equalsAsciiL(
            RTL_CONSTASCII_STRINGPARAM("RehearseTimings") ))
    {
        bool bRehearseTimings = false;
        if (! (rProperty.Value >>= bRehearseTimings))
            return false;
        
        if (bRehearseTimings) 
        {
            // TODO(Q3): Move to slide
            mpRehearseTimingsActivity = RehearseTimingsActivity::create(
                SlideShowContext(
                    mpDummyPtr,
                    maEventQueue, 
                    maEventMultiplexer, 
                    maScreenUpdater,
                    maActivitiesQueue,
                    maUserEventQueue,
                    *this,
                    maViewContainer,
                    mxComponentContext) );
        }
        else if (mpRehearseTimingsActivity) 
        {
            // removes timer from all views:
            mpRehearseTimingsActivity->dispose();
            mpRehearseTimingsActivity.reset();
        }
        return true;
    }
    
    if (rProperty.Name.equalsAsciiL(
            RTL_CONSTASCII_STRINGPARAM("WaitSymbolBitmap") ))
    {
        uno::Reference<rendering::XBitmap> xBitmap;
        if (! (rProperty.Value >>= xBitmap))
            return false;
        
        mpWaitSymbol = WaitSymbol::create( xBitmap, 
                                           maScreenUpdater,
                                           maEventMultiplexer,
                                           maViewContainer );

        return true;
    }
    
    if (rProperty.Name.equalsAsciiL(
            RTL_CONSTASCII_STRINGPARAM("NoSlideTransitions") )) 
    {
        return (rProperty.Value >>= mbNoSlideTransitions);
    }
    
    return false;
}

void SlideShowImpl::addSlideShowListener(
    uno::Reference<presentation::XSlideShowListener> const& xListener )
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );

    if (isDisposed())
        return;

    // container syncs with passed mutex ref
    maListenerContainer.addInterface(xListener);
}

void SlideShowImpl::removeSlideShowListener(
    uno::Reference<presentation::XSlideShowListener> const& xListener )
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );

    // container syncs with passed mutex ref
    maListenerContainer.removeInterface(xListener);
}

void SlideShowImpl::addShapeEventListener(
    uno::Reference<presentation::XShapeEventListener> const& xListener,
    uno::Reference<drawing::XShape> const& xShape )
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );

    if (isDisposed())
        return;
    
    // precondition: must only be called from the main thread!
    DBG_TESTSOLARMUTEX();

    ShapeEventListenerMap::iterator aIter;
    if( (aIter=maShapeEventListeners.find( xShape )) ==
        maShapeEventListeners.end() )
    {
        // no entry for this shape -> create one
        aIter = maShapeEventListeners.insert( 
            ShapeEventListenerMap::value_type(
                xShape,
                boost::shared_ptr<cppu::OInterfaceContainerHelper>( 
                    new cppu::OInterfaceContainerHelper(m_aMutex)))).first;    
    }

    // add new listener to broadcaster
    if( aIter->second.get() )
        aIter->second->addInterface( xListener );

    maEventMultiplexer.notifyShapeListenerAdded(xListener,
                                                xShape);
}

void SlideShowImpl::removeShapeEventListener(
    uno::Reference<presentation::XShapeEventListener> const& xListener,
    uno::Reference<drawing::XShape> const& xShape )
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );
    
    // precondition: must only be called from the main thread!
    DBG_TESTSOLARMUTEX();

    ShapeEventListenerMap::iterator aIter;
    if( (aIter = maShapeEventListeners.find( xShape )) !=
        maShapeEventListeners.end() )
    {
        // entry for this shape found -> remove listener from
        // helper object
        ENSURE_OR_THROW(
            aIter->second.get(),
            "SlideShowImpl::removeShapeEventListener(): "
            "listener map contains NULL broadcast helper" );
        
        aIter->second->removeInterface( xListener );
    }

    maEventMultiplexer.notifyShapeListenerRemoved(xListener,
                                                  xShape);
}

void SlideShowImpl::setShapeCursor(
    uno::Reference<drawing::XShape> const& xShape, sal_Int16 nPointerShape )
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );

    if (isDisposed())
        return;
    
    // precondition: must only be called from the main thread!
    DBG_TESTSOLARMUTEX();

    ShapeCursorMap::iterator aIter;
    if( (aIter=maShapeCursors.find( xShape )) == maShapeCursors.end() ) 
    {
        // no entry for this shape -> create one
        if( nPointerShape != awt::SystemPointer::ARROW ) 
        {
            // add new entry, unless shape shall display
            // normal pointer arrow -> no need to handle that
            // case
            maShapeCursors.insert(
                ShapeCursorMap::value_type(xShape, 
                                           nPointerShape) );
        }
    }
    else if( nPointerShape == awt::SystemPointer::ARROW ) 
    {
        // shape shall display normal cursor -> can disable
        // the cursor and clear the entry
        maShapeCursors.erase( xShape );
    }
    else 
    {
        // existing entry found, update with new cursor ID
        aIter->second = nPointerShape;
    }

    maEventMultiplexer.notifyShapeCursorChange(xShape, 
                                               nPointerShape);
}

bool SlideShowImpl::requestCursor( sal_Int16 nCursorShape )
{
    mnCurrentCursor = nCursorShape;

    const sal_Int16 nActualCursor = calcActiveCursor(mnCurrentCursor);

    // change all views to the requested cursor ID
    std::for_each( maViewContainer.begin(), 
                   maViewContainer.end(),
                   boost::bind( &View::setCursorShape, 
                                _1, 
                                nActualCursor ));

    return nActualCursor==nCursorShape;
}

void SlideShowImpl::resetCursor()
{
    mnCurrentCursor = awt::SystemPointer::ARROW;

    // change all views to the default cursor ID
    std::for_each( maViewContainer.begin(), 
                   maViewContainer.end(),
                   boost::bind( &View::setCursorShape, 
                                _1, 
                                calcActiveCursor(mnCurrentCursor) ));
}

sal_Bool SlideShowImpl::update( double & nNextTimeout )
    throw (uno::RuntimeException)
{
    osl::MutexGuard const guard( m_aMutex );

    if (isDisposed())
        return false;
    
    // precondition: update() must only be called from the
    // main thread!
    DBG_TESTSOLARMUTEX();

    if( mbShowPaused ) 
    {
        // commit frame (might be repaints pending)
        maScreenUpdater.commitUpdates();

        return false;
    }
    else 
    {
        // TODO(F2): re-evaluate whether that timer lagging makes
        // sense.
        
        // hold timer, while processing the queues (ensures
        // same time for all activities and events)
        {
            comphelper::ScopeGuard scopeGuard(
                boost::bind( &canvas::tools::ElapsedTime::releaseTimer,
                             boost::cref(mpPresTimer) ) );

            // no need to hold timer for only one active animation -
            // it's only meant to keep multiple ones in sync
            if( maActivitiesQueue.size() > 1 ) 
                mpPresTimer->holdTimer();
            else
                scopeGuard.dismiss(); // we're not holding the timer
                    
            // process queues
            maEventQueue.process();
            maActivitiesQueue.process();

            // commit frame to screen
            maScreenUpdater.commitUpdates();

            // TODO(Q3): remove need to call dequeued() from
            // activities. feels like a wart.
            //
            // Rationale for ActivitiesQueue::processDequeued(): when
            // an activity ends, it usually pushed the end state to
            // the animated shape in question, and ends the animation
            // (which, in turn, will usually disable shape sprite
            // mode). Disabling shape sprite mode causes shape
            // repaint, which, depending on slide content, takes
            // considerably more time than sprite updates. Thus, the
            // last animation step tends to look delayed. To
            // camouflage this, reaching end position and disabling
            // sprite mode is split into two (normal Activity::end(),
            // and Activity::dequeued()). Now, the reason to call
            // commitUpdates() twice here is caused by the unrelated
            // fact that during wait cursor display/hide, the screen
            // is updated, and shows hidden sprites, but, in case of
            // leaving the second commitUpdates() call out and punting
            // that to the next round, no updated static slide
            // content. In short, the last shape animation of a slide
            // tends to blink at its end.
            
            // process dequeued activities _after_ commit to screen
            maActivitiesQueue.processDequeued();

            // commit frame to screen
            maScreenUpdater.commitUpdates();
        }
        // Time held until here
                
        const bool bActivitiesLeft = (! maActivitiesQueue.isEmpty());
        const bool bTimerEventsLeft = (! maEventQueue.isEmpty());
        const bool bRet = (bActivitiesLeft || bTimerEventsLeft);
                
        if (bRet) 
        {
            // calc nNextTimeout value:
            if (bActivitiesLeft) 
            {
                // Activity queue is not empty.  Tell caller that we would
                // like to render another frame.
                nNextTimeout = 1.0 / FrameRate::PreferredFramesPerSecond;
            }
            else 
            {
                // timer events left:
                // difference from current time (nota bene:
                // time no longer held here!) to the next event in
                // the event queue.

                // #i61190# Retrieve next timeout only _after_
                // processing activity queue

                // ensure positive value:
                nNextTimeout = std::max( 0.0, maEventQueue.nextTimeout() );
            }

            mbSlideShowIdle = false;
        }

#if defined(VERBOSE) && defined(DBG_UTIL)
        // when slideshow is idle, issue an XUpdatable::update() call
        // exactly once after a previous animation sequence finished -
        // this might trigger screen dumps on some canvas
        // implementations
        if( !mbSlideShowIdle &&
            (!bRet ||
             nNextTimeout > 1.0) )
        {
            UnoViewVector::const_iterator       aCurr(maViewContainer.begin());
            const UnoViewVector::const_iterator aEnd(maViewContainer.end());
            while( aCurr != aEnd )
            {
                try
                {
                    uno::Reference< presentation::XSlideShowView > xView( (*aCurr)->getUnoView(),
                                                                          uno::UNO_QUERY_THROW );
                    uno::Reference< util::XUpdatable >             xUpdatable( xView->getCanvas(),
                                                                               uno::UNO_QUERY_THROW );
                    xUpdatable->update();
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

                ++aCurr;
            }

            mbSlideShowIdle = true;
        }
#endif
        
        return bRet;
    }
}

void SlideShowImpl::notifySlideTransitionEnded( bool bPaintSlide )
{
    osl::MutexGuard const guard( m_aMutex );

    OSL_ENSURE( !isDisposed(), "### already disposed!" );
    OSL_ENSURE( mpCurrentSlide,
                "notifySlideTransitionEnded(): Invalid current slide" );
    if (mpCurrentSlide) 
    {
        // first init show, to give the animations
        // the chance to register SlideStartEvents
        const bool bBackgroundLayerRendered( !bPaintSlide );
        mpCurrentSlide->show( bBackgroundLayerRendered );
        maEventMultiplexer.notifySlideStartEvent();
    }
}

void queryAutomaticSlideTransition( uno::Reference<drawing::XDrawPage> const& xDrawPage,
                                    double&                                   nAutomaticNextSlideTimeout,
                                    bool&                                     bHasAutomaticNextSlide )
{
    // retrieve slide change parameters from XDrawPage
    // ===============================================

    uno::Reference< beans::XPropertySet > xPropSet( xDrawPage,
                                                    uno::UNO_QUERY );
            
    sal_Int32 nChange(0);
    if( !xPropSet.is() ||
        !getPropertyValue( nChange,
                           xPropSet,
                           ::rtl::OUString( 
                               RTL_CONSTASCII_USTRINGPARAM("Change"))) )
    {
        OSL_TRACE(
            "queryAutomaticSlideTransition(): "
            "Could not extract slide change mode from XDrawPage - assuming <none>\n" );
    }
            
    bHasAutomaticNextSlide = nChange == 1;

    if( !xPropSet.is() ||
        !getPropertyValue( nAutomaticNextSlideTimeout,
                           xPropSet,
                           ::rtl::OUString( 
                               RTL_CONSTASCII_USTRINGPARAM("Duration"))) )
    {
        OSL_TRACE(
            "queryAutomaticSlideTransition(): "
            "Could not extract slide transition timeout from "
            "XDrawPage - assuming 1 sec\n" );
    }
}

void SlideShowImpl::notifySlideAnimationsEnded()
{
    osl::MutexGuard const guard( m_aMutex );

    OSL_ENSURE( !isDisposed(), "### already disposed!" );
    
    // This struct will receive the (interruptable) event,
    // that triggers the notifySlideEnded() method.
    InterruptableEventPair aNotificationEvents;

    if( maEventMultiplexer.getAutomaticMode() ) 
    {
        OSL_ENSURE( ! mpRehearseTimingsActivity,
                    "unexpected: RehearseTimings mode!" );
                
        // schedule a slide end event, with automatic mode's
        // delay
        aNotificationEvents = makeInterruptableDelay(
            boost::bind<void>( boost::mem_fn(&SlideShowImpl::notifySlideEnded), this ),
            maEventMultiplexer.getAutomaticTimeout() );
    }
    else 
    {
        OSL_ENSURE( mpCurrentSlide,
                    "notifySlideAnimationsEnded(): Invalid current slide!" );
        
        bool   bHasAutomaticNextSlide=false;
        double nAutomaticNextSlideTimeout=0.0;
        queryAutomaticSlideTransition(mpCurrentSlide->getXDrawPage(),
                                      nAutomaticNextSlideTimeout,
                                      bHasAutomaticNextSlide);

        // check whether slide transition should happen
        // 'automatically'. If yes, simply schedule the
        // specified timeout.
        // NOTE: mbForceManualAdvance and mpRehearseTimingsActivity
        // override any individual slide setting, to always
        // step slides manually.
        if( !mbForceManualAdvance &&
            !mpRehearseTimingsActivity &&
            bHasAutomaticNextSlide )
        {
            aNotificationEvents = makeInterruptableDelay(
                boost::bind<void>( boost::mem_fn(&SlideShowImpl::notifySlideEnded), this ),
                nAutomaticNextSlideTimeout);
            
            // TODO(F2): Provide a mechanism to let the user override   
            // this automatic timeout via next()
        }
        else 
        {
            if (mpRehearseTimingsActivity)
                mpRehearseTimingsActivity->start();
                    
            // generate click event. Thus, the user must
            // trigger the actual end of a slide. No need to
            // generate interruptable event here, there's no
            // timeout involved.
            aNotificationEvents.mpImmediateEvent = 
                makeEvent( boost::bind<void>(
                    boost::mem_fn(&SlideShowImpl::notifySlideEnded), this ) );
        }
    }
    
    // register events on the queues. To make automatic slide
    // changes interruptable, register the interruption event
    // as a nextEffectEvent target. Note that the timeout
    // event is optional (e.g. manual slide changes don't
    // generate a timeout)            
    maUserEventQueue.registerNextEffectEvent(
        aNotificationEvents.mpImmediateEvent );
    
    if( aNotificationEvents.mpTimeoutEvent )
        maEventQueue.addEvent( aNotificationEvents.mpTimeoutEvent );
    
    // current slide's main sequence is over. Now should be
    // the time to prefetch the next slide (if any), and
    // prepare the initial slide bitmap (speeds up slide
    // change setup time a lot). Show the wait cursor, this
    // indeed might take some seconds.
    {
        comphelper::ScopeGuard const scopeGuard(
            boost::bind( &SlideShowImpl::setWaitState, this, false ) );
        setWaitState(true);
        
        if (! matches( mpPrefetchSlide,
                       mxPrefetchSlide, mxPrefetchAnimationNode )) 
        {
            mpPrefetchSlide = makeSlide( mxPrefetchSlide,
                                         mxPrefetchAnimationNode );
        }
        if (mpPrefetchSlide) 
        {
            // ignore return value, this is just to populate
            // Slide's internal bitmap buffer, such that the time
            // needed to generate the slide bitmap is not spent
            // when the slide change is requested.
            mpPrefetchSlide->getCurrentSlideBitmap( *maViewContainer.begin() );
        }
    } // finally

    maListenerContainer.forEach<presentation::XSlideShowListener>(
        boost::mem_fn( &presentation::XSlideShowListener::slideAnimationsEnded ) );
}

void SlideShowImpl::notifySlideEnded()
{
    osl::MutexGuard const guard( m_aMutex );

    OSL_ENSURE( !isDisposed(), "### already disposed!" );
    
    if (mpRehearseTimingsActivity) 
    {
        const double time = mpRehearseTimingsActivity->stop();
        if (mpRehearseTimingsActivity->hasBeenClicked()) 
        {
            // save time at current drawpage:
            uno::Reference<beans::XPropertySet> xPropSet(
                mpCurrentSlide->getXDrawPage(), uno::UNO_QUERY );
            OSL_ASSERT( xPropSet.is() );
            if (xPropSet.is()) 
            {
                xPropSet->setPropertyValue(
                    OUSTR("Change"),
                    uno::Any( static_cast<sal_Int32>(1) ) );
                xPropSet->setPropertyValue(
                    OUSTR("Duration"),
                    uno::Any( static_cast<sal_Int32>(time) ) );
            }
        }
    }
    
    maEventMultiplexer.notifySlideEndEvent();
    
    stopShow();  // MUST call that: results in
                 // maUserEventQueue.clear(). What's more,
                 // stopShow()'s currSlide->hide() call is
                 // now also required, notifySlideEnded()
                 // relies on that
                 // unconditionally. Otherwise, genuine
                 // shape animations (drawing layer and
                 // GIF) will not be stopped.
    
    maListenerContainer.forEach<presentation::XSlideShowListener>(
        boost::mem_fn( &presentation::XSlideShowListener::slideEnded ) );
}

bool SlideShowImpl::notifyHyperLinkClicked( rtl::OUString const& hyperLink )
{
    osl::MutexGuard const guard( m_aMutex );

    maListenerContainer.forEach<presentation::XSlideShowListener>(
        boost::bind( &presentation::XSlideShowListener::hyperLinkClicked,
                     _1, 
                     boost::cref(hyperLink) ));
    return true;
}

/** Notification from eventmultiplexer that an animation event has occoured.
	This will be forewarded to all registered XSlideShoeListener
 */
bool SlideShowImpl::handleAnimationEvent( const AnimationNodeSharedPtr& rNode )
{
    osl::MutexGuard const guard( m_aMutex );

	uno::Reference<animations::XAnimationNode> xNode( rNode->getXAnimationNode() );

	switch( rNode->getState() )
	{
	case AnimationNode::ACTIVE:
	    maListenerContainer.forEach<presentation::XSlideShowListener>(
		    boost::bind( &animations::XAnimationListener::beginEvent,
			             _1, 
				         boost::cref(xNode) ));
		break;
		
	case AnimationNode::FROZEN:
	case AnimationNode::ENDED:
	    maListenerContainer.forEach<presentation::XSlideShowListener>(
		    boost::bind( &animations::XAnimationListener::endEvent,
			             _1, 
				         boost::cref(xNode) ));
		break;
	default:
		break;
	}

	return true;
}

} // anon namespace

namespace sdecl = comphelper::service_decl;
#if defined (__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ <= 3)
 sdecl::class_<SlideShowImpl> serviceImpl;
 const sdecl::ServiceDecl slideShowDecl(
     serviceImpl,
#else
 const sdecl::ServiceDecl slideShowDecl(
     sdecl::class_<SlideShowImpl>(),
#endif
    "com.sun.star.comp.presentation.SlideShow",
    "com.sun.star.presentation.SlideShow" );

// The C shared lib entry points
COMPHELPER_SERVICEDECL_EXPORTS1(slideShowDecl)

