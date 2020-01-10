/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 * 
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * $RCSfile: cexports.cxx,v $
 * $Revision: 1.9 $
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
#include "precompiled_desktop.hxx"

#include "cppuhelper/implementationentry.hxx"
#include "oo3extensionmigration.hxx"

extern "C"
{

::cppu::ImplementationEntry entries [] =
{
    {
        migration::OO3ExtensionMigration_create, migration::OO3ExtensionMigration_getImplementationName,
        migration::OO3ExtensionMigration_getSupportedServiceNames, ::cppu::createSingleComponentFactory,
        0, 0
    },
	{ 0, 0, 0, 0, 0, 0 }
};


void SAL_CALL component_getImplementationEnvironment(
	const sal_Char ** ppEnvTypeName, uno_Environment ** )
{
	*ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

sal_Bool SAL_CALL component_writeInfo(
	void * pServiceManager, void * pRegistryKey )
{
	return ::cppu::component_writeInfoHelper(
        pServiceManager, pRegistryKey, entries );
}

void * SAL_CALL component_getFactory(
	const sal_Char * pImplName, void * pServiceManager, void * pRegistryKey )
{
	return ::cppu::component_getFactoryHelper(
        pImplName, pServiceManager, pRegistryKey, entries );
}

}
