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
#include "precompiled_framework.hxx"
//_________________________________________________________________________________________________________________
//	includes of my own project
//_________________________________________________________________________________________________________________
#include <macros/registration.hxx>

/*=================================================================================================================
	Add new include and new register info to for new services.

	Example:

		#ifndef __YOUR_SERVICE_1_HXX_
		#include <service1.hxx>
		#endif

		#ifndef __YOUR_SERVICE_2_HXX_
		#include <service2.hxx>
		#endif

		COMPONENTGETIMPLEMENTATIONENVIRONMENT

		COMPONENTWRITEINFO	(	COMPONENTINFO( Service1 )
 								COMPONENTINFO( Service2 )
							)

		COMPONENTGETFACTORY	(	IFFACTORIE( Service1 )
 								else
								IFFACTORIE( Service2 )
 							)
=================================================================================================================*/
#include <jobs/helponstartup.hxx>
#include <tabwin/tabwinfactory.hxx>
#include <dispatch/systemexec.hxx>
#include <jobs/shelljob.hxx>

COMPONENTGETIMPLEMENTATIONENVIRONMENT

COMPONENTWRITEINFO  (   COMPONENTINFO( ::framework::HelpOnStartup	)
						COMPONENTINFO( ::framework::TabWinFactory	)
                        COMPONENTINFO( ::framework::SystemExec	    )
                        COMPONENTINFO( ::framework::ShellJob	    )
                    )

COMPONENTGETFACTORY	(	IFFACTORY( ::framework::HelpOnStartup		) else
						IFFACTORY( ::framework::TabWinFactory		) else
                        IFFACTORY( ::framework::SystemExec	        ) else
                        IFFACTORY( ::framework::ShellJob	        )
					)
