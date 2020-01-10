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

#ifndef __FRAMEWORK_HELPER_OFRAMES_HXX_
#define __FRAMEWORK_HELPER_OFRAMES_HXX_

//_________________________________________________________________________________________________________________
//	my own includes
//_________________________________________________________________________________________________________________

#include <classes/framecontainer.hxx>
#include <threadhelp/threadhelpbase.hxx>
#include <macros/generic.hxx>
#include <macros/xinterface.hxx>
#include <macros/xtypeprovider.hxx>
#include <macros/debug.hxx>
#include <general.h>

//_________________________________________________________________________________________________________________
//	interface includes
//_________________________________________________________________________________________________________________
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/frame/XFrames.hpp>
#include <com/sun/star/frame/XFrame.hpp>

//_________________________________________________________________________________________________________________
//	other includes
//_________________________________________________________________________________________________________________
#include <cppuhelper/implbase1.hxx>
#include <cppuhelper/weakref.hxx>

//_________________________________________________________________________________________________________________
//	namespace
//_________________________________________________________________________________________________________________

namespace framework{

//_________________________________________________________________________________________________________________
//	exported const
//_________________________________________________________________________________________________________________

//_________________________________________________________________________________________________________________
//	exported definitions
//_________________________________________________________________________________________________________________

/*-************************************************************************************************************//**
	@short          implement XFrames, XIndexAccess and XElementAccess interfaces as helper for services
	@descr			Use this class as helper for these interfaces. We share mutex and framecontainer with ouer owner.
					The framecontainer is a member of it from type "FrameContainer". That means;
					we have the same information as ouer owner. In current implementation we use mutex and lock-mechanism
					to prevent against compete access. In future we plan support of semaphore!

	@devstatus		deprecated
	@implements		XInterface
					XFrames
					XIndexAccess
					XElementAccess
	@base			OWeakObject

	@ATTENTION		Don't use this class as direct member - use it dynamicly. Do not derive from this class.
					We hold a weakreference to ouer owner not to ouer superclass.

	@devstatus		deprecated
*//*-*************************************************************************************************************/

class OFrames   :   private ThreadHelpBase      ,   // Must be the first of baseclasses - Is neccessary for right initialization of objects!
					public ::cppu::WeakImplHelper1< ::com::sun::star::frame::XFrames >
{
	//-------------------------------------------------------------------------------------------------------------
	//	public methods
	//-------------------------------------------------------------------------------------------------------------

	public:

		//---------------------------------------------------------------------------------------------------------
		//	constructor / destructor
		//---------------------------------------------------------------------------------------------------------

		/*-****************************************************************************************************//**
			@short		standard ctor
			@descr		These initialize a new instance of this class with all needed informations for work.
                        We share framecontainer with owner implementation! It's a threadsafe container.

			@seealso	-

			@param		"xFactory"			, reference to factory which has created ouer owner(!). We can use these to create new uno-services.
			@param		"xOwner"			, reference to ouer owner. We hold a wekreference to prevent us against cross-references!
			@param		"pFrameContainer"	, pointer to shared framecontainer of owner. It's valid only, if weakreference is valid!
			@return		-

			@onerror	-
		*//*-*****************************************************************************************************/

	 	OFrames(	const	css::uno::Reference< css::lang::XMultiServiceFactory >&	xFactory		,
					const	css::uno::Reference< css::frame::XFrame >&				xOwner			,
							FrameContainer*											pFrameContainer	);

		//---------------------------------------------------------------------------------------------------------
		//	XFrames
		//---------------------------------------------------------------------------------------------------------

		/*-****************************************************************************************************//**
			@short      append frame to container
			@descr		We share the container with ouer owner. We can do this only, if no lock is set on container.
						Valid references are accepted only!

			@seealso	class FrameContainer

			@param		"xFrame", reference to an existing frame to append.
			@return		-

			@onerror	We do nothing in release or throw an assert in debug version.
		*//*-*****************************************************************************************************/

    	virtual void SAL_CALL append( const css::uno::Reference< css::frame::XFrame >& xFrame ) throw( css::uno::RuntimeException );

		/*-****************************************************************************************************//**
			@short      remove frame from container
			@descr		This is the companion to append(). We only accept valid references and don't work, if
						a lock is set.

			@seealso	class FrameContainer

			@param		"xFrame", reference to an existing frame to remove.
			@return		-

			@onerror	We do nothing in release or throw an assert in debug version.
		*//*-*****************************************************************************************************/

    	virtual void SAL_CALL remove( const css::uno::Reference< css::frame::XFrame >& xFrame ) throw( css::uno::RuntimeException );

		/*-****************************************************************************************************//**
			@short      return list of all applicable frames for given flags
			@descr		Call these to get a list of all frames, which are match with given search flags.

			@seealso	-

			@param		"nSearchFlag", flags to search right frames.
			@return		A list of founded frames.

			@onerror	An empty list is returned.
		*//*-*****************************************************************************************************/

    	virtual css::uno::Sequence< css::uno::Reference< css::frame::XFrame > > SAL_CALL queryFrames( sal_Int32 nSearchFlags ) throw( css::uno::RuntimeException );

		//---------------------------------------------------------------------------------------------------------
		//	XIndexAccess
		//---------------------------------------------------------------------------------------------------------

		/*-****************************************************************************************************//**
			@short      get count of all current frames in container
			@descr		This is the beginning of full index-access. With a count you can step over all items in container.
						Next call shuo�d be getByIndex(). But these mechanism works only, if no lock in container is set!

			@seealso	class FrameContainer
			@seealso	method getByIndex()

			@param		-
			@return		Count of current items in container.

			@onerror	If a lock is set, we return 0 for prevent further access!
		*//*-*****************************************************************************************************/

    	virtual sal_Int32 SAL_CALL getCount() throw( css::uno::RuntimeException );

		/*-****************************************************************************************************//**
			@short		get specified container item by index
			@descr		If you called getCount() successful - this method return the specified element as an Any.
						You must observe the range from 0 to count-1! Otherwise an IndexOutOfBoundsException is thrown.

			@seealso	class FrameContainer
			@seealso	method getCount()

			@param		"nIndex", valid index to get container item.
			@return		A container item (specified by index) wrapped in an Any.

			@onerror	If a lock is set, we return an empty Any!
			@onerror	If index out of range, an IndexOutOfBoundsException is thrown.
		*//*-*****************************************************************************************************/

    	virtual css::uno::Any SAL_CALL getByIndex( sal_Int32 nIndex ) throw(	css::lang::IndexOutOfBoundsException	,
																				css::lang::WrappedTargetException		,
																				css::uno::RuntimeException				);

		//---------------------------------------------------------------------------------------------------------
		//	XElementAccess
		//---------------------------------------------------------------------------------------------------------

		/*-****************************************************************************************************//**
			@short      get uno-type of all container items
			@descr		In current implementation type is fixed to XFrame!
						(container-lock is ignored)

			@seealso	-

			@param		-
			@return		A uno-type descriptor.

			@onerror	-
		*//*-*****************************************************************************************************/

		virtual css::uno::Type SAL_CALL getElementType() throw( css::uno::RuntimeException );

		/*-****************************************************************************************************//**
			@short      get fill state of current container
			@descr		Call these to get information about, if items exist in container or not.
						(container-lock is ignored)

			@seealso	-

			@param		-
			@return		sal_True, if container contains some items.
			@return		sal_False, otherwise.

			@onerror	We return sal_False.
		*//*-*****************************************************************************************************/

    	virtual sal_Bool SAL_CALL hasElements() throw( css::uno::RuntimeException );

	//-------------------------------------------------------------------------------------------------------------
	//	protected methods
	//-------------------------------------------------------------------------------------------------------------

	protected:

		/*-****************************************************************************************************//**
			@short		standard destructor
			@descr		This method destruct an instance of this class and clear some member.
						This method is protected, because its not allowed to use this class as a member!
						You MUST use a dynamical instance (pointer). That's the reason for a protected dtor.

			@seealso	-

			@param		-
			@return		-

			@onerror	-
		*//*-*****************************************************************************************************/

		virtual	~OFrames();

		/*-****************************************************************************************************//**
			@short		reset instance to default values
			@descr		There are two ways to delete an instance of this class.<BR>
						1) delete with destructor<BR>
						2) dispose from parent or factory ore ...<BR>
						This method do the same for both ways! It free used memory and release references ...

			@seealso	method dispose() (if it exist!)
			@seealso	destructor ~TaskEnumeration()

			@param		-

			@return		-

			@onerror	-
		*//*-*****************************************************************************************************/

		virtual void impl_resetObject();

	//-------------------------------------------------------------------------------------------------------------
	//	private methods
	//-------------------------------------------------------------------------------------------------------------

	private:

		/*-****************************************************************************************************//**
			@short		append one sequence to another
			@descr		There is no operation to add to sequences! Use this helper-method to do this.

			@seealso	class Sequence

			@param		"seqDestination", reference to sequence on which operation will append the other sequence.
			@param		"seqSource"		, reference to sequence for append.
			@return		"seqDestination" is parameter AND return value at the same time.

			@onerror	-
		*//*-*****************************************************************************************************/

		void impl_appendSequence(			css::uno::Sequence< css::uno::Reference< css::frame::XFrame > >&	seqDestination	,
			 						const	css::uno::Sequence< css::uno::Reference< css::frame::XFrame > >&	seqSource		);

	//-------------------------------------------------------------------------------------------------------------
	//	debug methods
	//	(should be private everyway!)
	//-------------------------------------------------------------------------------------------------------------

		/*-****************************************************************************************************//**
			@short		debug-method to check incoming parameter of some other mehods of this class
			@descr		The following methods are used to check parameters for other methods
						of this class. The return value is used directly for an ASSERT(...).

			@seealso	ASSERTs in implementation!

			@param		references to checking variables
			@return		sal_False ,on invalid parameter
			@return		sal_True  ,otherwise

			@onerror	-
		*//*-*****************************************************************************************************/

	#ifdef ENABLE_ASSERTIONS

	private:

		static sal_Bool impldbg_checkParameter_OFramesCtor	(	const	css::uno::Reference< css::lang::XMultiServiceFactory >&	xFactory		,
																const	css::uno::Reference< css::frame::XFrame >&				xOwner			,
																		FrameContainer*											pFrameContainer	);
		static sal_Bool impldbg_checkParameter_append		(	const	css::uno::Reference< css::frame::XFrame >&				xFrame			);
		static sal_Bool impldbg_checkParameter_remove		(	const	css::uno::Reference< css::frame::XFrame >&				xFrame			);
		static sal_Bool impldbg_checkParameter_queryFrames	(			sal_Int32												nSearchFlags	);

	#endif	// #ifdef ENABLE_ASSERTIONS

	//-------------------------------------------------------------------------------------------------------------
	//	variables
	//	(should be private everyway!)
	//-------------------------------------------------------------------------------------------------------------

	private:

		css::uno::Reference< css::lang::XMultiServiceFactory >		m_xFactory						;	/// reference to global servicemanager
		css::uno::WeakReference< css::frame::XFrame >				m_xOwner						;	/// reference to owner of this instance (Hold no hard reference!)
		FrameContainer*												m_pFrameContainer				;	/// with owner shared list to hold all direct childs of an XFramesSupplier
		sal_Bool													m_bRecursiveSearchProtection	;	/// flag to protect against recursive searches of frames at parents

};		//	class OFrames

}		//	namespace framework

#endif	//	#ifndef __FRAMEWORK_HELPER_OFRAMES_HXX_
