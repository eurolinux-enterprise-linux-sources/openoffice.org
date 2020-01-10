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
#ifndef INCLUDED_SVTOOLS_SECURITYOPTIONS_HXX
#define INCLUDED_SVTOOLS_SECURITYOPTIONS_HXX

//_________________________________________________________________________________________________________________
//	includes
//_________________________________________________________________________________________________________________

#include "svtools/svldllapi.h"
#include <sal/types.h>
#include <osl/mutex.hxx>
#include <com/sun/star/uno/Sequence.h>
#include <rtl/ustring.hxx>
#include <svtools/options.hxx>

//_________________________________________________________________________________________________________________
//	forward declarations
//_________________________________________________________________________________________________________________

/*-************************************************************************************************************//**
	@short			forward declaration to our private date container implementation
	@descr			We use these class as internal member to support small memory requirements.
					You can create the container if it is neccessary. The class which use these mechanism
					is faster and smaller then a complete implementation!
*//*-*************************************************************************************************************/

class SvtSecurityOptions_Impl;

//_________________________________________________________________________________________________________________
//	declarations
//_________________________________________________________________________________________________________________

/*-************************************************************************************************************//**
	@descr          These values present modes to handle StarOffice basic scripts.
					see GetBasicMode/SetBasicMode() for further informations
*//*-*************************************************************************************************************/

enum EBasicSecurityMode
{
	eNEVER_EXECUTE	= 0,
	eFROM_LIST		= 1,
	eALWAYS_EXECUTE = 2
};

/*-************************************************************************************************************//**
	@short			collect informations about security features
	@descr          -

	@implements		-
	@base			-

	@ATTENTION		This class is partially threadsafe.

	@devstatus		ready to use
*//*-*************************************************************************************************************/

class SVL_DLLPUBLIC SvtSecurityOptions: public svt::detail::Options
{
	//-------------------------------------------------------------------------------------------------------------
	//  public types
	//-------------------------------------------------------------------------------------------------------------
	public:

		enum EOption
		{
            E_SECUREURLS,
            E_BASICMODE,					// xmlsec05 depricated
            E_EXECUTEPLUGINS,				// xmlsec05 depricated
            E_WARNING,						// xmlsec05 depricated
            E_CONFIRMATION,					// xmlsec05 depricated
			E_DOCWARN_SAVEORSEND,
			E_DOCWARN_SIGNING,
			E_DOCWARN_PRINT,
			E_DOCWARN_CREATEPDF,
			E_DOCWARN_REMOVEPERSONALINFO,
			E_DOCWARN_RECOMMENDPASSWORD,
			E_MACRO_SECLEVEL,
			E_MACRO_TRUSTEDAUTHORS,
            E_MACRO_DISABLE,
            E_CTRLCLICK_HYPERLINK
		};

		enum MacroAction
		{
			MA_DONTRUN = 0,
			MA_CONFIRM,
			MA_RUN
		};

		typedef ::com::sun::star::uno::Sequence< ::rtl::OUString > Certificate;

        /*
        // MT: Doesn't work for sequence...
        struct Certificate
        {
            ::rtl::OUString SubjectName;
            ::rtl::OUString SerialNumber;
            ::rtl::OUString RawData;
        };
        */

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

         SvtSecurityOptions();
        virtual ~SvtSecurityOptions();

		//---------------------------------------------------------------------------------------------------------
		//	interface
		//---------------------------------------------------------------------------------------------------------

        /*-****************************************************************************************************//**
            @short      returns readonly state
            @descr      It can be called to get information about the readonly state of a provided item.
            @seealso    -

            @param      "eOption", specify, which item is queried
            @return     <TRUE/> if item is readonly; <FALSE/> otherwhise

            @onerror    No error should occurre!
        *//*-*****************************************************************************************************/

        sal_Bool IsReadOnly( EOption eOption ) const ;

        /*-****************************************************************************************************//**
			@short		interface methods to get and set value of config key "org.openoffice.Office.Common/Security/Scripting/SecureURL"
			@descr		These value displays the list of all trustworthy URLs.
						zB.:	file:/					=> All scripts from the local file system including a LAN;
								private:explorer		=> Scripts from the Explorer;
								private:help			=> Scripts in the help system;
								private:newmenu			=> Scripts that are executed by the commands File-New and AutoPilot;
								private:schedule		=> Scripts of  the scheduler;
								private:searchfolder	=> Scripts of the searchfolder;
								private:user			=> Scripts that are entered in the URL field.
			@seealso	-

			@param		"seqURLList", new values to set it in configuration.
			@return		The values which represent current state of internal variable.

			@onerror	No error should occurre!
		*//*-*****************************************************************************************************/

		::com::sun::star::uno::Sequence< ::rtl::OUString >	GetSecureURLs(																		) const	;
		void												SetSecureURLs( const ::com::sun::star::uno::Sequence< ::rtl::OUString >& seqURLList	)		;

		/*-****************************************************************************************************//**
			@short		interface methods to get and set value of config key "org.openoffice.Office.Common/Security/Scripting/StarOfficeBasic"
			@descr		These value determines how StarOffice Basic scripts should be handled.
						It exist 3 different modes:
							0 = never execute
							1 = from list
							2 = always execute

			@ATTENTION	These methods don't check for valid or invalid values!
						Our configuration server can do it ... but these implementation don't get any notifications
						about wrong commits ...!
						=> If you set an invalid value - nothing will be changed. The information will lost.

			@seealso	enum EBasicSecurityMode

			@param		"eMode" to set new mode ... Value must defined as an enum of type EBasicSecurityMode!
			@return		An enum value, which present current mode.

			@onerror	No error should occurre!
		*//*-*****************************************************************************************************/

		sal_Int32			GetMacroSecurityLevel		(					) const	;
		void				SetMacroSecurityLevel		( sal_Int32 _nLevel	)		;

		sal_Bool			IsMacroDisabled				(					) const	;

		/*-****************************************************************************************************//**
			@short		special method to check an URL and his referer corresponding to ouer internal security cessation
			@descr		Give us an URL and his referer and we will say you if these url can be scripted or not!

			@seealso	-

			@param		"sURL" reference to URL for checking
			@param		"sReferer" reference to referer which whish to run script by given URL
			@return		sal_True if URL is secure or security is obsolete(!) or sal_False otherwise.

			@onerror	No error should occurre!
		*//*-*****************************************************************************************************/

		sal_Bool IsSecureURL(	const	::rtl::OUString&	sURL		,
								const	::rtl::OUString&	sReferer	) const ;

		::com::sun::star::uno::Sequence< Certificate >	GetTrustedAuthors		(																	) const	;
		void											SetTrustedAuthors		( const ::com::sun::star::uno::Sequence< Certificate >& rAuthors	)		;
		sal_Bool										IsTrustedAuthorsEnabled	(																	)		;

		// for bool options only!
		bool		IsOptionSet		( EOption eOption					) const	;
		bool		SetOption		( EOption eOption, bool bValue		)		;
		bool		IsOptionEnabled	( EOption eOption					) const	;

		// xmlsec05 depricated methods
        sal_Bool    IsExecutePlugins() const;
        void        SetExecutePlugins( sal_Bool bSet );
		EBasicSecurityMode	GetBasicMode(							) const	;
		void				SetBasicMode( EBasicSecurityMode eMode	)		;
        sal_Bool IsWarningEnabled() const;
        void SetWarningEnabled( sal_Bool bSet );
        sal_Bool IsConfirmationEnabled() const;
        void SetConfirmationEnabled( sal_Bool bSet );

	//-------------------------------------------------------------------------------------------------------------
	//	private methods
	//-------------------------------------------------------------------------------------------------------------

	private:

		/*-****************************************************************************************************//**
			@short		return a reference to a static mutex
			@descr		These class is partially threadsafe (for de-/initialization only).
						All access methods are'nt safe!
						We create a static mutex only for one ime and use at different times.

			@seealso	-

			@param		-
			@return		A reference to a static mutex member.

			@onerror	-
		*//*-*****************************************************************************************************/

		SVL_DLLPRIVATE static ::osl::Mutex& GetInitMutex();

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

    	static SvtSecurityOptions_Impl*	m_pDataContainer	;	/// impl. data container as dynamic pointer for smaller memory requirements!
		static sal_Int32				m_nRefCount			;	/// internal ref count mechanism

};		// class SvtSecurityOptions

#endif	// #ifndef INCLUDED_SVTOOLS_SECURITYOPTIONS_HXX
