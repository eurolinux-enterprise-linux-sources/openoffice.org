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
#include "precompiled_ucb.hxx"

/**************************************************************************
								TODO
 **************************************************************************

 **************************************************************************

	Props/Commands:

						root	folder 	folder  link	link
										(new) 		   	(new)
	----------------------------------------------------------------
	ContentType     	x		x		x		x       x
	IsDocument      	x		x		x		x       x
	IsFolder        	x		x		x		x       x
	Title           	x		x		x		x       x
	TargetURL									x		x

	getCommandInfo      x		x		x		x       x
	getPropertySetInfo  x		x		x		x       x
	getPropertyValues   x		x		x		x       x
	setPropertyValues   x		x		x		x       x
	insert								x				x
	delete						x				x
	open				x		x
    transfer            x       x

 *************************************************************************/
#include <com/sun/star/beans/Property.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/ucb/CommandInfo.hpp>
#include <com/sun/star/ucb/OpenCommandArgument2.hpp>
#include <com/sun/star/ucb/TransferInfo.hpp>
#include <com/sun/star/uno/Sequence.hxx>
#include "hierarchycontent.hxx"

using namespace com::sun::star;
using namespace hierarchy_ucp;

//=========================================================================
//
// HierarchyContent implementation.
//
//=========================================================================

//=========================================================================
//
// IMPORTENT: If any property data ( name / type / ... ) are changed, then
//            HierarchyContent::getPropertyValues(...) must be adapted too!
//
//=========================================================================

// virtual
uno::Sequence< beans::Property > HierarchyContent::getProperties(
            const uno::Reference< ucb::XCommandEnvironment > & /*xEnv*/ )
{
	osl::Guard< osl::Mutex > aGuard( m_aMutex );

	if ( m_eKind == LINK )
	{
		//=================================================================
		//
		// Link: Supported properties
		//
		//=================================================================

        if ( isReadOnly() )
        {
            static beans::Property aLinkPropertyInfoTable[] =
            {
                ///////////////////////////////////////////////////////////
                // Required properties
                ///////////////////////////////////////////////////////////
                beans::Property(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "ContentType" ) ),
                    -1,
                    getCppuType( static_cast< const rtl::OUString * >( 0 ) ),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                ),
                beans::Property(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "IsDocument" ) ),
                    -1,
                    getCppuBooleanType(),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                ),
                beans::Property(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "IsFolder" ) ),
                    -1,
                    getCppuBooleanType(),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                ),
                beans::Property(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Title" ) ),
                    -1,
                    getCppuType( static_cast< const rtl::OUString * >( 0 ) ),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                ),
                ///////////////////////////////////////////////////////////
                // Optional standard properties
                ///////////////////////////////////////////////////////////
                beans::Property(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "TargetURL" ) ),
                    -1,
                    getCppuType( static_cast< const rtl::OUString * >( 0 ) ),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                )
                ///////////////////////////////////////////////////////////
                // New properties
                ///////////////////////////////////////////////////////////
            };
            return uno::Sequence<
                            beans::Property >( aLinkPropertyInfoTable, 5 );
        }
        else
        {
            static beans::Property aLinkPropertyInfoTable[] =
            {
                ///////////////////////////////////////////////////////////
                // Required properties
                ///////////////////////////////////////////////////////////
                beans::Property(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "ContentType" ) ),
                    -1,
                    getCppuType( static_cast< const rtl::OUString * >( 0 ) ),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                ),
                beans::Property(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "IsDocument" ) ),
                    -1,
                    getCppuBooleanType(),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                ),
                beans::Property(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "IsFolder" ) ),
                    -1,
                    getCppuBooleanType(),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                ),
                beans::Property(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Title" ) ),
                    -1,
                    getCppuType( static_cast< const rtl::OUString * >( 0 ) ),
                    beans::PropertyAttribute::BOUND
                ),
                ///////////////////////////////////////////////////////////
                // Optional standard properties
                ///////////////////////////////////////////////////////////
                beans::Property(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "TargetURL" ) ),
                    -1,
                    getCppuType( static_cast< const rtl::OUString * >( 0 ) ),
                    beans::PropertyAttribute::BOUND
                )
                ///////////////////////////////////////////////////////////
                // New properties
                ///////////////////////////////////////////////////////////
            };
            return uno::Sequence<
                            beans::Property >( aLinkPropertyInfoTable, 5 );
        }
	}
	else if ( m_eKind == FOLDER )
	{
		//=================================================================
		//
		// Folder: Supported properties
		//
		//=================================================================

        if ( isReadOnly() )
        {
            static beans::Property aFolderPropertyInfoTable[] =
            {
                ///////////////////////////////////////////////////////////
                // Required properties
                ///////////////////////////////////////////////////////////
                beans::Property(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "ContentType" ) ),
                    -1,
                    getCppuType( static_cast< const rtl::OUString * >( 0 ) ),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                ),
                beans::Property(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "IsDocument" ) ),
                    -1,
                    getCppuBooleanType(),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                ),
                beans::Property(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "IsFolder" ) ),
                    -1,
                    getCppuBooleanType(),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                ),
                beans::Property(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Title" ) ),
                    -1,
                    getCppuType( static_cast< const rtl::OUString * >( 0 ) ),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                )
                ///////////////////////////////////////////////////////////
                // Optional standard properties
                ///////////////////////////////////////////////////////////
                ///////////////////////////////////////////////////////////
                // New properties
                ///////////////////////////////////////////////////////////
            };
            return uno::Sequence<
                            beans::Property >( aFolderPropertyInfoTable, 4 );
        }
        else
        {
            static beans::Property aFolderPropertyInfoTable[] =
            {
                ///////////////////////////////////////////////////////////
                // Required properties
                ///////////////////////////////////////////////////////////
                beans::Property(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "ContentType" ) ),
                    -1,
                    getCppuType( static_cast< const rtl::OUString * >( 0 ) ),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                ),
                beans::Property(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "IsDocument" ) ),
                    -1,
                    getCppuBooleanType(),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                ),
                beans::Property(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "IsFolder" ) ),
                    -1,
                    getCppuBooleanType(),
                    beans::PropertyAttribute::BOUND
                        | beans::PropertyAttribute::READONLY
                ),
                beans::Property(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Title" ) ),
                    -1,
                    getCppuType( static_cast< const rtl::OUString * >( 0 ) ),
                    beans::PropertyAttribute::BOUND
                )
                ///////////////////////////////////////////////////////////
                // Optional standard properties
                ///////////////////////////////////////////////////////////
                ///////////////////////////////////////////////////////////
                // New properties
                ///////////////////////////////////////////////////////////
            };
            return uno::Sequence<
                            beans::Property >( aFolderPropertyInfoTable, 4 );
        }
	}
	else
	{
		//=================================================================
		//
		// Root Folder: Supported properties
		//
		//=================================================================

        static beans::Property aRootFolderPropertyInfoTable[] =
		{
			///////////////////////////////////////////////////////////////
			// Required properties
			///////////////////////////////////////////////////////////////
            beans::Property(
                rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "ContentType" ) ),
				-1,
                getCppuType( static_cast< const rtl::OUString * >( 0 ) ),
                beans::PropertyAttribute::BOUND
                    | beans::PropertyAttribute::READONLY
			),
            beans::Property(
                rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "IsDocument" ) ),
				-1,
				getCppuBooleanType(),
                beans::PropertyAttribute::BOUND
                    | beans::PropertyAttribute::READONLY
			),
            beans::Property(
                rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "IsFolder" ) ),
				-1,
				getCppuBooleanType(),
                beans::PropertyAttribute::BOUND
                    | beans::PropertyAttribute::READONLY
			),
            beans::Property(
                rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "Title" ) ),
				-1,
                getCppuType( static_cast< const rtl::OUString * >( 0 ) ),
                beans::PropertyAttribute::BOUND
                    | beans::PropertyAttribute::READONLY
			)
			///////////////////////////////////////////////////////////////
			// Optional standard properties
			///////////////////////////////////////////////////////////////
			///////////////////////////////////////////////////////////////
			// New properties
			///////////////////////////////////////////////////////////////
		};
        return uno::Sequence<
                beans::Property >( aRootFolderPropertyInfoTable, 4 );
	}
}

//=========================================================================
// virtual
uno::Sequence< ucb::CommandInfo > HierarchyContent::getCommands(
            const uno::Reference< ucb::XCommandEnvironment > & /*xEnv*/ )
{
	osl::Guard< osl::Mutex > aGuard( m_aMutex );

	if ( m_eKind == LINK )
	{
		//=================================================================
		//
		// Link: Supported commands
		//
		//=================================================================

        if ( isReadOnly() )
        {
            static const ucb::CommandInfo aLinkCommandInfoTable[] =
            {
                ///////////////////////////////////////////////////////////
                // Required commands
                ///////////////////////////////////////////////////////////
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getCommandInfo" ) ),
                    -1,
                    getCppuVoidType()
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getPropertySetInfo" ) ),
                    -1,
                    getCppuVoidType()
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getPropertyValues" ) ),
                    -1,
                    getCppuType(
                        static_cast< uno::Sequence< beans::Property > * >( 0 ) )
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "setPropertyValues" ) ),
                    -1,
                    getCppuType(
                        static_cast<
                            uno::Sequence< beans::PropertyValue > * >( 0 ) )
                )
                ///////////////////////////////////////////////////////////
                // Optional standard commands
                ///////////////////////////////////////////////////////////

                ///////////////////////////////////////////////////////////
                // New commands
                ///////////////////////////////////////////////////////////
            };
            return uno::Sequence<
                    ucb::CommandInfo >( aLinkCommandInfoTable, 4 );
        }
        else
        {
            static const ucb::CommandInfo aLinkCommandInfoTable[] =
            {
                ///////////////////////////////////////////////////////////
                // Required commands
                ///////////////////////////////////////////////////////////
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getCommandInfo" ) ),
                    -1,
                    getCppuVoidType()
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getPropertySetInfo" ) ),
                    -1,
                    getCppuVoidType()
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getPropertyValues" ) ),
                    -1,
                    getCppuType(
                        static_cast< uno::Sequence< beans::Property > * >( 0 ) )
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "setPropertyValues" ) ),
                    -1,
                    getCppuType(
                        static_cast<
                            uno::Sequence< beans::PropertyValue > * >( 0 ) )
                ),
                ///////////////////////////////////////////////////////////
                // Optional standard commands
                ///////////////////////////////////////////////////////////
                ucb::CommandInfo(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "delete" ) ),
                    -1,
                    getCppuBooleanType()
                ),
                ucb::CommandInfo(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "insert" ) ),
                    -1,
                    getCppuVoidType()
                )
                ///////////////////////////////////////////////////////////
                // New commands
                ///////////////////////////////////////////////////////////
            };
            return uno::Sequence<
                    ucb::CommandInfo >( aLinkCommandInfoTable, 6 );
        }
	}
	else if ( m_eKind == FOLDER )
	{
        //=================================================================
        //
        // Folder: Supported commands
        //
        //=================================================================

        if ( isReadOnly() )
        {
            static const ucb::CommandInfo aFolderCommandInfoTable[] =
            {
                ///////////////////////////////////////////////////////////
                // Required commands
                ///////////////////////////////////////////////////////////
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getCommandInfo" ) ),
                    -1,
                    getCppuVoidType()
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getPropertySetInfo" ) ),
                    -1,
                    getCppuVoidType()
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getPropertyValues" ) ),
                    -1,
                    getCppuType(
                        static_cast< uno::Sequence< beans::Property > * >( 0 ) )
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "setPropertyValues" ) ),
                    -1,
                    getCppuType(
                        static_cast<
                            uno::Sequence< beans::PropertyValue > * >( 0 ) )
                ),
                ///////////////////////////////////////////////////////////
                // Optional standard commands
                ///////////////////////////////////////////////////////////
                ucb::CommandInfo(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "open" ) ),
                    -1,
                    getCppuType(
                        static_cast< ucb::OpenCommandArgument2 * >( 0 ) )
                )
                ///////////////////////////////////////////////////////////
                // New commands
                ///////////////////////////////////////////////////////////
            };
            return uno::Sequence<
                    ucb::CommandInfo >( aFolderCommandInfoTable, 5 );
        }
        else
        {
            static const ucb::CommandInfo aFolderCommandInfoTable[] =
            {
                ///////////////////////////////////////////////////////////
                // Required commands
                ///////////////////////////////////////////////////////////
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getCommandInfo" ) ),
                    -1,
                    getCppuVoidType()
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getPropertySetInfo" ) ),
                    -1,
                    getCppuVoidType()
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getPropertyValues" ) ),
                    -1,
                    getCppuType(
                        static_cast< uno::Sequence< beans::Property > * >( 0 ) )
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "setPropertyValues" ) ),
                    -1,
                    getCppuType(
                        static_cast<
                            uno::Sequence< beans::PropertyValue > * >( 0 ) )
                ),
                ///////////////////////////////////////////////////////////
                // Optional standard commands
                ///////////////////////////////////////////////////////////
                ucb::CommandInfo(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "delete" ) ),
                    -1,
                    getCppuBooleanType()
                ),
                ucb::CommandInfo(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "insert" ) ),
                    -1,
                    getCppuVoidType()
                ),
                ucb::CommandInfo(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "open" ) ),
                    -1,
                    getCppuType(
                        static_cast< ucb::OpenCommandArgument2 * >( 0 ) )
                ),
                ucb::CommandInfo(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "transfer" ) ),
                    -1,
                    getCppuType( static_cast< ucb::TransferInfo * >( 0 ) )
                )
                ///////////////////////////////////////////////////////////
                // New commands
                ///////////////////////////////////////////////////////////
            };
            return uno::Sequence<
                    ucb::CommandInfo >( aFolderCommandInfoTable, 8 );
        }
	}
	else
	{
		//=================================================================
		//
		// Root Folder: Supported commands
		//
		//=================================================================

        if ( isReadOnly() )
        {
            static const ucb::CommandInfo aRootFolderCommandInfoTable[] =
            {
                ///////////////////////////////////////////////////////////
                // Required commands
                ///////////////////////////////////////////////////////////
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getCommandInfo" ) ),
                    -1,
                    getCppuVoidType()
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getPropertySetInfo" ) ),
                    -1,
                    getCppuVoidType()
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getPropertyValues" ) ),
                    -1,
                    getCppuType(
                        static_cast< uno::Sequence< beans::Property > * >( 0 ) )
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "setPropertyValues" ) ),
                    -1,
                    getCppuType(
                        static_cast<
                            uno::Sequence< beans::PropertyValue > * >( 0 ) )
                ),
                ///////////////////////////////////////////////////////////
                // Optional standard commands
                ///////////////////////////////////////////////////////////
                ucb::CommandInfo(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "open" ) ),
                    -1,
                    getCppuType(
                        static_cast< ucb::OpenCommandArgument2 * >( 0 ) )
                )
                ///////////////////////////////////////////////////////////
                // New commands
                ///////////////////////////////////////////////////////////
            };
            return uno::Sequence<
                    ucb::CommandInfo >( aRootFolderCommandInfoTable, 5 );
        }
        else
        {
            static const ucb::CommandInfo aRootFolderCommandInfoTable[] =
            {
                ///////////////////////////////////////////////////////////
                // Required commands
                ///////////////////////////////////////////////////////////
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getCommandInfo" ) ),
                    -1,
                    getCppuVoidType()
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getPropertySetInfo" ) ),
                    -1,
                    getCppuVoidType()
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "getPropertyValues" ) ),
                    -1,
                    getCppuType(
                        static_cast< uno::Sequence< beans::Property > * >( 0 ) )
                ),
                ucb::CommandInfo(
                    rtl::OUString(
                        RTL_CONSTASCII_USTRINGPARAM( "setPropertyValues" ) ),
                    -1,
                    getCppuType(
                        static_cast<
                            uno::Sequence< beans::PropertyValue > * >( 0 ) )
                ),
                ///////////////////////////////////////////////////////////
                // Optional standard commands
                ///////////////////////////////////////////////////////////
                ucb::CommandInfo(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "open" ) ),
                    -1,
                    getCppuType(
                        static_cast< ucb::OpenCommandArgument2 * >( 0 ) )
                ),
                ucb::CommandInfo(
                    rtl::OUString( RTL_CONSTASCII_USTRINGPARAM( "transfer" ) ),
                    -1,
                    getCppuType( static_cast< ucb::TransferInfo * >( 0 ) )
                )
                ///////////////////////////////////////////////////////////
                // New commands
                ///////////////////////////////////////////////////////////
            };
            return uno::Sequence<
                    ucb::CommandInfo >( aRootFolderCommandInfoTable, 6 );
        }
	}
}
