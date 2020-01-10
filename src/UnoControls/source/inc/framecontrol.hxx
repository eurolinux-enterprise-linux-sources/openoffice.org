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

#ifndef _UNOCONTROLS_FRAMECONTROL_CTRL_HXX
#define _UNOCONTROLS_FRAMECONTROL_CTRL_HXX

//______________________________________________________________________________________________________________
//	includes of other projects
//______________________________________________________________________________________________________________

#include <com/sun/star/frame/XFrameActionListener.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/FrameActionEvent.hpp>
#include <com/sun/star/frame/FrameAction.hpp>
#include <com/sun/star/lang/XServiceName.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XConnectionPointContainer.hpp>
#include <cppuhelper/propshlp.hxx>

//______________________________________________________________________________________________________________
//	includes of my own project
//______________________________________________________________________________________________________________
#include "basecontrol.hxx"
#include "OConnectionPointContainerHelper.hxx"

//______________________________________________________________________________________________________________
//	namespaces
//______________________________________________________________________________________________________________

namespace unocontrols{

#define	UNO3_ANY										::com::sun::star::uno::Any
#define	UNO3_ILLEGALARGUMENTEXCEPTION					::com::sun::star::lang::IllegalArgumentException
#define	UNO3_IPROPERTYARRAYHELPER						::cppu::IPropertyArrayHelper
#define	UNO3_OBROADCASTHELPER							::cppu::OBroadcastHelper
#define	UNO3_OCONNECTIONPOINTCONTAINERHELPER			OConnectionPointContainerHelper
#define	UNO3_OMULTITYPEINTERFACECONTAINERHELPER			::cppu::OMultiTypeInterfaceContainerHelper
#define	UNO3_OPROPERTYSETHELPER							::cppu::OPropertySetHelper
#define	UNO3_OUSTRING									::rtl::OUString
#define	UNO3_PROPERTY									::com::sun::star::beans::Property
#define	UNO3_PROPERTYVALUE								::com::sun::star::beans::PropertyValue
#define	UNO3_REFERENCE									::com::sun::star::uno::Reference
#define	UNO3_RUNTIMEEXCEPTION							::com::sun::star::uno::RuntimeException
#define	UNO3_SEQUENCE									::com::sun::star::uno::Sequence
#define	UNO3_TYPE										::com::sun::star::uno::Type
#define	UNO3_WINDOWDESCRIPTOR                           ::com::sun::star::awt::WindowDescriptor
#define	UNO3_XCONNECTIONPOINT							::com::sun::star::lang::XConnectionPoint
#define	UNO3_XCONNECTIONPOINTCONTAINER					::com::sun::star::lang::XConnectionPointContainer
#define	UNO3_XCONTROLMODEL								::com::sun::star::awt::XControlModel
#define	UNO3_XFRAME										::com::sun::star::frame::XFrame
#define	UNO3_XGRAPHICS									::com::sun::star::awt::XGraphics
#define	UNO3_XINTERFACE									::com::sun::star::uno::XInterface
#define	UNO3_XMULTISERVICEFACTORY						::com::sun::star::lang::XMultiServiceFactory
#define	UNO3_XPROPERTYSETINFO							::com::sun::star::beans::XPropertySetInfo
#define	UNO3_XTOOLKIT									::com::sun::star::awt::XToolkit
#define	UNO3_XWINDOWPEER								::com::sun::star::awt::XWindowPeer

//______________________________________________________________________________________________________________
//	defines
//______________________________________________________________________________________________________________

#define	SERVICENAME_FRAMECONTROL						"com.sun.star.frame.FrameControl"
#define	IMPLEMENTATIONNAME_FRAMECONTROL					"stardiv.UnoControls.FrameControl"
#define	PROPERTYNAME_LOADERARGUMENTS					"LoaderArguments"
#define	PROPERTYNAME_COMPONENTURL						"ComponentURL"
#define	PROPERTYNAME_FRAME								"Frame"
#define	ERRORTEXT_VOSENSHURE							"This is an invalid property handle."
#define PROPERTY_COUNT									3                                       				// you must count the propertys
#define PROPERTYHANDLE_COMPONENTURL						0														// Id must be the index into the array
#define PROPERTYHANDLE_FRAME							1
#define PROPERTYHANDLE_LOADERARGUMENTS					2

//______________________________________________________________________________________________________________
//	class
//______________________________________________________________________________________________________________

class FrameControl	: public UNO3_XCONTROLMODEL
					, public UNO3_XCONNECTIONPOINTCONTAINER
					, public BaseControl								// This order is neccessary for right initialization of m_aMutex!
					, public UNO3_OBROADCASTHELPER
					, public UNO3_OPROPERTYSETHELPER
{

//______________________________________________________________________________________________________________
//	public methods
//______________________________________________________________________________________________________________

public:

	//__________________________________________________________________________________________________________
	//	construct/destruct
	//__________________________________________________________________________________________________________

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

	FrameControl( const UNO3_REFERENCE< UNO3_XMULTISERVICEFACTORY >& xFactory );

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

	virtual	~FrameControl();

	//__________________________________________________________________________________________________________
	//	XInterface
	//__________________________________________________________________________________________________________

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
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

	//__________________________________________________________________________________________________________
	//	XTypeProvider
	//__________________________________________________________________________________________________________

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

	virtual UNO3_SEQUENCE< UNO3_TYPE > SAL_CALL getTypes() throw( UNO3_RUNTIMEEXCEPTION );

	//__________________________________________________________________________________________________________
	//	XAggregation
	//__________________________________________________________________________________________________________

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

	UNO3_ANY SAL_CALL queryAggregation( const UNO3_TYPE& aType ) throw( UNO3_RUNTIMEEXCEPTION );

	//__________________________________________________________________________________________________________
	//	XControl
	//__________________________________________________________________________________________________________

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

    virtual void SAL_CALL createPeer(	const	UNO3_REFERENCE< UNO3_XTOOLKIT >&	xToolkit	,
										const	UNO3_REFERENCE< UNO3_XWINDOWPEER >&	xParent		) throw( UNO3_RUNTIMEEXCEPTION );

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

    virtual sal_Bool SAL_CALL setModel( const UNO3_REFERENCE< UNO3_XCONTROLMODEL >& xModel ) throw( UNO3_RUNTIMEEXCEPTION );

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

    virtual UNO3_REFERENCE< UNO3_XCONTROLMODEL > SAL_CALL getModel() throw( UNO3_RUNTIMEEXCEPTION );

	//__________________________________________________________________________________________________________
    //	XComponent
	//__________________________________________________________________________________________________________

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

    virtual void SAL_CALL dispose() throw( UNO3_RUNTIMEEXCEPTION );

	//__________________________________________________________________________________________________________
	//	XView
	//__________________________________________________________________________________________________________

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

    virtual sal_Bool SAL_CALL setGraphics( const UNO3_REFERENCE< UNO3_XGRAPHICS >& xDevice ) throw( UNO3_RUNTIMEEXCEPTION );

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

    virtual UNO3_REFERENCE< UNO3_XGRAPHICS > SAL_CALL getGraphics() throw( UNO3_RUNTIMEEXCEPTION );

	//__________________________________________________________________________________________________________
	//	XConnectionPointContainer
	//__________________________________________________________________________________________________________

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

	virtual UNO3_SEQUENCE< UNO3_TYPE > SAL_CALL getConnectionPointTypes() throw( UNO3_RUNTIMEEXCEPTION );

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

    virtual UNO3_REFERENCE< UNO3_XCONNECTIONPOINT > SAL_CALL queryConnectionPoint( const UNO3_TYPE& aType ) throw( UNO3_RUNTIMEEXCEPTION );

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

    virtual void SAL_CALL advise(	const	UNO3_TYPE&							aType		,
									const	UNO3_REFERENCE< UNO3_XINTERFACE >&	xListener	) throw( UNO3_RUNTIMEEXCEPTION );

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

    virtual void SAL_CALL unadvise(	const	UNO3_TYPE&							aType		,
									const	UNO3_REFERENCE< UNO3_XINTERFACE >&	xListener	) throw( UNO3_RUNTIMEEXCEPTION );

	//__________________________________________________________________________________________________________
	//	impl but public methods to register service!
	//__________________________________________________________________________________________________________

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

    static const UNO3_SEQUENCE< UNO3_OUSTRING > impl_getStaticSupportedServiceNames();

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

    static const UNO3_OUSTRING impl_getStaticImplementationName();

//______________________________________________________________________________________________________________
//	protected methods
//______________________________________________________________________________________________________________

protected:
    using OPropertySetHelper::getFastPropertyValue;
	//__________________________________________________________________________________________________________
	//	OPropertySetHelper
	//__________________________________________________________________________________________________________

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

	virtual sal_Bool SAL_CALL convertFastPropertyValue(			UNO3_ANY&	rConvertedValue	,
																UNO3_ANY&	rOldValue		,
																sal_Int32	nHandle			,
														const	UNO3_ANY&	rValue			) throw( UNO3_ILLEGALARGUMENTEXCEPTION );

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

	virtual void SAL_CALL setFastPropertyValue_NoBroadcast(			sal_Int32	nHandle	,
						  									const	UNO3_ANY&	rValue	) throw ( ::com::sun::star::uno::Exception );

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

	virtual void SAL_CALL getFastPropertyValue(	UNO3_ANY&	rValue	,
						  						sal_Int32	nHandle	) const ;

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

	virtual UNO3_IPROPERTYARRAYHELPER& SAL_CALL getInfoHelper();

	//__________________________________________________________________________________________________________
	//	XPropertySet
	//__________________________________________________________________________________________________________

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

	UNO3_REFERENCE< UNO3_XPROPERTYSETINFO > SAL_CALL getPropertySetInfo() throw( UNO3_RUNTIMEEXCEPTION );

	//__________________________________________________________________________________________________________
	//	BaseControl
	//__________________________________________________________________________________________________________

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

	virtual UNO3_WINDOWDESCRIPTOR* impl_getWindowDescriptor( const UNO3_REFERENCE< UNO3_XWINDOWPEER >& xParentPeer );

//______________________________________________________________________________________________________________
//	private methods
//______________________________________________________________________________________________________________

private:

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

	void impl_createFrame(	const	UNO3_REFERENCE< UNO3_XWINDOWPEER >&		xPeer			,
							const	UNO3_OUSTRING&							sURL			,
							const	UNO3_SEQUENCE< UNO3_PROPERTYVALUE >&	seqArguments	);

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

    void impl_deleteFrame();

	/**_________________________________________________________________________________________________________
		@short
		@descr

		@seealso

		@param

		@return

		@onerror
	*/

	static const UNO3_SEQUENCE< UNO3_PROPERTY > impl_getStaticPropertyDescriptor();


//______________________________________________________________________________________________________________
//	private variables
//______________________________________________________________________________________________________________

private:

	UNO3_REFERENCE< UNO3_XFRAME >				m_xFrame					;
	UNO3_OUSTRING								m_sComponentURL				;
	UNO3_SEQUENCE< UNO3_PROPERTYVALUE >			m_seqLoaderArguments		;
	UNO3_OMULTITYPEINTERFACECONTAINERHELPER		m_aInterfaceContainer		;
	UNO3_OCONNECTIONPOINTCONTAINERHELPER		m_aConnectionPointContainer	;

};	// class FrameControl

}	// namespace unocontrols

#endif	// #ifndef _UNOCONTROLS_FRAMECONTROL_CTRL_HXX
