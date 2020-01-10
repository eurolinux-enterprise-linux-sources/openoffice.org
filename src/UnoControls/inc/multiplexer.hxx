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

#ifndef _UNOCONTROLS_MULTIPLEXER_HXX
#define _UNOCONTROLS_MULTIPLEXER_HXX

//____________________________________________________________________________________________________________
//	includes of other projects
//____________________________________________________________________________________________________________

#include <com/sun/star/awt/XKeyListener.hpp>
#include <com/sun/star/awt/XPaintListener.hpp>
#include <com/sun/star/awt/KeyEvent.hpp>
#include <com/sun/star/awt/KeyModifier.hpp>
#include <com/sun/star/awt/XMouseMotionListener.hpp>
#include <com/sun/star/awt/FocusEvent.hpp>
#include <com/sun/star/awt/XWindowListener.hpp>
#include <com/sun/star/awt/XActivateListener.hpp>
#include <com/sun/star/awt/MouseEvent.hpp>
#include <com/sun/star/awt/XTopWindowListener.hpp>
#include <com/sun/star/awt/PaintEvent.hpp>
#include <com/sun/star/awt/InputEvent.hpp>
#include <com/sun/star/awt/KeyGroup.hpp>
#include <com/sun/star/awt/Key.hpp>
#include <com/sun/star/awt/WindowEvent.hpp>
#include <com/sun/star/awt/XMouseListener.hpp>
#include <com/sun/star/awt/KeyFunction.hpp>
#include <com/sun/star/awt/FocusChangeReason.hpp>
#include <com/sun/star/awt/MouseButton.hpp>
#include <com/sun/star/awt/XFocusListener.hpp>
#include <com/sun/star/awt/XTopWindow.hpp>
#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/awt/PosSize.hpp>
#include <cppuhelper/weak.hxx>
#include <cppuhelper/interfacecontainer.hxx>
//____________________________________________________________________________________________________________
//	includes of my own project
//____________________________________________________________________________________________________________

//____________________________________________________________________________________________________________
//	"namespaces"
//____________________________________________________________________________________________________________

namespace unocontrols{

#define	UNO3_OWEAKOBJECT							::cppu::OWeakObject
#define	UNO3_XWINDOW								::com::sun::star::awt::XWindow
#define	UNO3_REFERENCE								::com::sun::star::uno::Reference
#define	UNO3_WEAKREFERENCE							::com::sun::star::uno::WeakReference
#define	UNO3_MUTEX									::osl::Mutex
#define	UNO3_XWINDOWLISTENER						::com::sun::star::awt::XWindowListener
#define	UNO3_XKEYLISTENER							::com::sun::star::awt::XKeyListener
#define	UNO3_XMOUSELISTENER							::com::sun::star::awt::XMouseListener
#define	UNO3_XMOUSEMOTIONLISTENER					::com::sun::star::awt::XMouseMotionListener
#define	UNO3_XPAINTLISTENER							::com::sun::star::awt::XPaintListener
#define	UNO3_XTOPWINDOWLISTENER						::com::sun::star::awt::XTopWindowListener
#define	UNO3_XFOCUSLISTENER							::com::sun::star::awt::XFocusListener
#define	UNO3_ANY									::com::sun::star::uno::Any
#define	UNO3_TYPE									::com::sun::star::uno::Type
#define	UNO3_RUNTIMEEXCEPTION						::com::sun::star::uno::RuntimeException
#define	UNO3_XINTERFACE								::com::sun::star::uno::XInterface
#define	UNO3_EVENTOBJECT							::com::sun::star::lang::EventObject
#define	UNO3_FOCUSEVENT								::com::sun::star::awt::FocusEvent
#define	UNO3_WINDOWEVENT							::com::sun::star::awt::WindowEvent
#define	UNO3_KEYEVENT								::com::sun::star::awt::KeyEvent
#define	UNO3_MOUSEEVENT								::com::sun::star::awt::MouseEvent
#define	UNO3_PAINTEVENT								::com::sun::star::awt::PaintEvent
#define	UNO3_OMULTITYPEINTERFACECONTAINERHELPER		::cppu::OMultiTypeInterfaceContainerHelper

//____________________________________________________________________________________________________________
//	class
//____________________________________________________________________________________________________________

class OMRCListenerMultiplexerHelper	: public UNO3_XFOCUSLISTENER
									, public UNO3_XWINDOWLISTENER
									, public UNO3_XKEYLISTENER
									, public UNO3_XMOUSELISTENER
									, public UNO3_XMOUSEMOTIONLISTENER
									, public UNO3_XPAINTLISTENER
									, public UNO3_XTOPWINDOWLISTENER
									, public UNO3_OWEAKOBJECT
{

//____________________________________________________________________________________________________________
//	public methods
//____________________________________________________________________________________________________________

public:

	//________________________________________________________________________________________________________
	//	construct/destruct
	//________________________________________________________________________________________________________

	/**_______________________________________________________________________________________________________
		@short		constructor
		@descr		Create a Multiplexer of XWindowEvents.

		@seealso	-

		@param		rControl	The control. All listeners think that this is the original broadcaster.
		@param		rPeer		The peer from which the original events are dispatched. Null is allowed.

		@return		-

		@onerror	-
	*/

	OMRCListenerMultiplexerHelper(	const	UNO3_REFERENCE< UNO3_XWINDOW >&	xControl	,
									const	UNO3_REFERENCE< UNO3_XWINDOW >&	xPeer		);

	/**_______________________________________________________________________________________________________
		@short		copy-constructor
		@descr

		@seealso	-

		@param		rCopyInstance	C++-Reference to instance to make copy from.

		@return		-

		@onerror	-
	*/

	OMRCListenerMultiplexerHelper( const OMRCListenerMultiplexerHelper& aCopyInstance );

	/**_______________________________________________________________________________________________________
		@short		destructor
		@descr		-

		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

	~OMRCListenerMultiplexerHelper();

	//________________________________________________________________________________________________________
	//	XInterface
	//________________________________________________________________________________________________________

	/**_______________________________________________________________________________________________________
		@short		give answer, if interface is supported
		@descr		The interfaces are searched by type.

		@seealso	XInterface

		@param      "rType" is the type of searched interface.

		@return		Any		information about found interface

		@onerror	A RuntimeException is thrown.
	*/

	virtual UNO3_ANY SAL_CALL queryInterface( const UNO3_TYPE& aType ) throw( UNO3_RUNTIMEEXCEPTION );

	/**_______________________________________________________________________________________________________
		@short		increment refcount
		@descr		-

		@seealso	XInterface
		@seealso	release()

		@param		-

		@return		-

		@onerror	A RuntimeException is thrown.
	*/

    virtual void SAL_CALL acquire() throw();

	/**_______________________________________________________________________________________________________
		@short		decrement refcount
		@descr		-

		@seealso	XInterface
		@seealso	acquire()

		@param		-

		@return		-

		@onerror	A RuntimeException is thrown.
	*/

    virtual void SAL_CALL release() throw();

	//________________________________________________________________________________________________________
	//	operator
	//________________________________________________________________________________________________________

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

	operator UNO3_REFERENCE< UNO3_XINTERFACE >() const;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

	OMRCListenerMultiplexerHelper& operator= ( const OMRCListenerMultiplexerHelper& aCopyInstance );

	//________________________________________________________________________________________________________
	//	container methods
	//________________________________________________________________________________________________________

	/**_______________________________________________________________________________________________________
		@short		Remove all listeners from the previous set peer and add the needed listeners to rPeer.
		@descr		-

		@seealso	-

		@param		rPeer		The peer from which the original events are dispatched. Null is allowed.

		@return		-

		@onerror	-
	*/

	void setPeer( const UNO3_REFERENCE< UNO3_XWINDOW >& xPeer );

	/**_______________________________________________________________________________________________________
		@short		Remove all listeners and send a disposing message.
		@descr		-

		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

	void disposeAndClear();

	/**_______________________________________________________________________________________________________
		@short		Add the specified listener to the source.
		@descr		-

		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    void advise(	const	UNO3_TYPE&							aType		,
					const	UNO3_REFERENCE< UNO3_XINTERFACE >&	xListener	);

	/**_______________________________________________________________________________________________________
		@short		Remove the specified listener from the source.
		@descr		-

		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    void unadvise(	const	UNO3_TYPE&							aType		,
					const	UNO3_REFERENCE< UNO3_XINTERFACE >&	xListener	);

	//________________________________________________________________________________________________________
	//	XEventListener
	//________________________________________________________________________________________________________

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

	virtual void SAL_CALL disposing(const UNO3_EVENTOBJECT& aSource) throw( UNO3_RUNTIMEEXCEPTION ) ;

	//________________________________________________________________________________________________________
	//	XFocusListener
	//________________________________________________________________________________________________________

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL focusGained(const UNO3_FOCUSEVENT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL focusLost(const UNO3_FOCUSEVENT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	//________________________________________________________________________________________________________
	//	XWindowListener
	//________________________________________________________________________________________________________

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL windowResized(const UNO3_WINDOWEVENT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL windowMoved(const UNO3_WINDOWEVENT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL windowShown(const UNO3_EVENTOBJECT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL windowHidden(const UNO3_EVENTOBJECT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	//________________________________________________________________________________________________________
	//	XKeyListener
	//________________________________________________________________________________________________________

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL keyPressed( const UNO3_KEYEVENT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL keyReleased( const UNO3_KEYEVENT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	//________________________________________________________________________________________________________
	//	XMouseListener
	//________________________________________________________________________________________________________

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL mousePressed(const UNO3_MOUSEEVENT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL mouseReleased(const UNO3_MOUSEEVENT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL mouseEntered(const UNO3_MOUSEEVENT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL mouseExited(const UNO3_MOUSEEVENT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	//________________________________________________________________________________________________________
	//	XMouseMotionListener
	//________________________________________________________________________________________________________

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL mouseDragged(const UNO3_MOUSEEVENT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL mouseMoved(const UNO3_MOUSEEVENT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	//________________________________________________________________________________________________________
	//	XPaintListener
	//________________________________________________________________________________________________________

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL windowPaint(const UNO3_PAINTEVENT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	//________________________________________________________________________________________________________
	//	XTopWindowListener
	//________________________________________________________________________________________________________

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

	virtual void SAL_CALL windowOpened( const UNO3_EVENTOBJECT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

	virtual void SAL_CALL windowClosing( const UNO3_EVENTOBJECT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL windowClosed( const UNO3_EVENTOBJECT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

	virtual void SAL_CALL windowMinimized( const UNO3_EVENTOBJECT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

	virtual void SAL_CALL windowNormalized( const UNO3_EVENTOBJECT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL windowActivated( const UNO3_EVENTOBJECT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

	/**_______________________________________________________________________________________________________
		@short		-
		@descr		-

		@seealso	-
		@seealso	-

		@param		-

		@return		-

		@onerror	-
	*/

    virtual void SAL_CALL windowDeactivated( const UNO3_EVENTOBJECT& aEvent ) throw( UNO3_RUNTIMEEXCEPTION ) ;

//____________________________________________________________________________________________________________
//	protected methods
//____________________________________________________________________________________________________________

protected:

	/**_______________________________________________________________________________________________________
		@short		Remove the listener from the peer.
		@descr		-

		@seealso	-

		@param		xPeer	The peer from which the listener is removed.
		@param		rType	The listener type, which specify the type of the listener.

		@return		-

		@onerror	-
	*/

	void impl_adviseToPeer(	const	UNO3_REFERENCE< UNO3_XWINDOW >&	xPeer	,
							const	UNO3_TYPE&						aType	);

	/**_______________________________________________________________________________________________________
		@short		Add the listener to the peer.
		@descr		-

		@seealso	-

		@param		xPeer	The peer to which the listener is added.
		@param		rType	The listener type, which specify the type of the listener.

		@return		-

		@onerror	-
	*/

	void impl_unadviseFromPeer(	const	UNO3_REFERENCE< UNO3_XWINDOW >&	xPeer	,
								const	UNO3_TYPE&						aType	);

//____________________________________________________________________________________________________________
//	private variables
//____________________________________________________________________________________________________________

private:

	UNO3_MUTEX									m_aMutex			;
	UNO3_REFERENCE< UNO3_XWINDOW >				m_xPeer				;	/// The source of the events. Normally this is the peer object.
	UNO3_WEAKREFERENCE< UNO3_XWINDOW >          m_xControl			;
	UNO3_OMULTITYPEINTERFACECONTAINERHELPER		m_aListenerHolder	;

};	// class OMRCListenerMultiplexerHelper

}	// namespace unocontrols

#endif	// ifndef _UNOCONTROLS_MULTIPLEXER_HXX
