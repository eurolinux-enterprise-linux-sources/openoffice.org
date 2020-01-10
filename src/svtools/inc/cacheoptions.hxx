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

#ifndef INCLUDED_SVTOOLS_CACHEOPTIONS_HXX
#define INCLUDED_SVTOOLS_CACHEOPTIONS_HXX

//_________________________________________________________________________________________________________________
//	includes
//_________________________________________________________________________________________________________________

#include "svtools/svldllapi.h"
#include <sal/types.h>
#include <osl/mutex.hxx>
#include <rtl/ustring.hxx>

//_________________________________________________________________________________________________________________
//	forward declarations
//_________________________________________________________________________________________________________________

/*-************************************************************************************************************//**
	@short			forward declaration to our private date container implementation
	@descr			We use these class as internal member to support small memory requirements.
					You can create the container if it is neccessary. The class which use these mechanism
					is faster and smaller then a complete implementation!
*//*-*************************************************************************************************************/

class SvtCacheOptions_Impl;

//_________________________________________________________________________________________________________________
//	declarations
//_________________________________________________________________________________________________________________

/*-************************************************************************************************************//**
	@short			collect informations about startup features
	@descr          -

	@implements		-
	@base			-

	@devstatus		ready to use
*//*-*************************************************************************************************************/

class SVL_DLLPUBLIC SvtCacheOptions
{
	//-------------------------------------------------------------------------------------------------------------
	//	public methods
	//-------------------------------------------------------------------------------------------------------------

	public:

		//---------------------------------------------------------------------------------------------------------
		//	constructor / destructor
		//---------------------------------------------------------------------------------------------------------

		/*-****************************************************************************************************//**
			@short		standard constructor and destructor
			@descr		This will initialize an instance with default values.
						We implement these class with a refcount mechanism! Every instance of this class increase it
						at create and decrease it at delete time - but all instances use the same data container!
						He is implemented as a static member ...

			@seealso	member m_nRefCount
			@seealso	member m_pDataContainer

			@param		-
			@return		-

			@onerror	-
		*//*-*****************************************************************************************************/

         SvtCacheOptions();
        ~SvtCacheOptions();

		//---------------------------------------------------------------------------------------------------------
		//	interface
		//---------------------------------------------------------------------------------------------------------

		/*-****************************************************************************************************//**
			@short		interface methods to get and set value of config key "org.openoffice.Office.Common/_3D-Engine/..."
			@descr      These options describe internal states to enable/disable features of installed office.

						GetWriterOLE_Objects()
						SetWriterOLE_Objects()				=>	set the number of Writer OLE objects to be cached

						GetDrawingEngineOLE_Objects()
						SetDrawingEngineOLE_Objects()		=>	set the number of DrawingEngine OLE objects to be cached

						GetGraphicManagerTotalCacheSize()
						SetGraphicManagerTotalCacheSize()	=>	set the maximum cache size used by GraphicManager to cache graphic objects

						GetGraphicManagerObjectCacheSize()
						SetGraphicManagerObjectCacheSize()	=>	set the maximum cache size for one GraphicObject to be cached by GraphicManager

			@seealso	configuration package "org.openoffice.Office.Common/_3D-Engine"
		*//*-*****************************************************************************************************/

		sal_Int32	GetWriterOLE_Objects() const;
		sal_Int32	GetDrawingEngineOLE_Objects() const;
		sal_Int32	GetGraphicManagerTotalCacheSize() const; 
		sal_Int32	GetGraphicManagerObjectCacheSize() const;
		sal_Int32	GetGraphicManagerObjectReleaseTime() const;

		void		SetWriterOLE_Objects( sal_Int32 nObjects );
		void		SetDrawingEngineOLE_Objects( sal_Int32 nObjects );
		void		SetGraphicManagerTotalCacheSize( sal_Int32 nTotalCacheSize );
		void		SetGraphicManagerObjectCacheSize( sal_Int32 nObjectCacheSize );
		void        SetGraphicManagerObjectReleaseTime( sal_Int32 nReleaseTimeSeconds );

	//-------------------------------------------------------------------------------------------------------------
	//	private methods
	//-------------------------------------------------------------------------------------------------------------

	private:

		/*-****************************************************************************************************//**
			@short		return a reference to a static mutex
			@descr		These class use his own static mutex to be threadsafe.
						We create a static mutex only for one ime and use at different times.

			@seealso	-

			@param		-
			@return		A reference to a static mutex member.

			@onerror	-
		*//*-*****************************************************************************************************/

		SVL_DLLPRIVATE static ::osl::Mutex& GetOwnStaticMutex();

	//-------------------------------------------------------------------------------------------------------------
	//	private member
	//-------------------------------------------------------------------------------------------------------------

	private:

		/*Attention

			Don't initialize these static member in these header!
			a) Double dfined symbols will be detected ...
			b) and unresolved externals exist at linking time.
			Do it in your source only.
		 */

    	static SvtCacheOptions_Impl*	m_pDataContainer	;	/// impl. data container as dynamic pointer for smaller memory requirements!
		static sal_Int32				m_nRefCount			;	/// internal ref count mechanism

};

#endif // #ifndef INCLUDED_SVTOOLS_CACHEOPTIONS_HXX
