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
#ifndef _CONNECTIVITY_JAVA_MATH_BIGDECIMAL_HXX_
#define	_CONNECTIVITY_JAVA_MATH_BIGDECIMAL_HXX_

#include "java/lang/Object.hxx"
//**************************************************************
//************ Class: java.lang.Boolean
//**************************************************************
namespace connectivity
{
	class java_math_BigDecimal : public java_lang_Object
	{
    	// statische Daten fuer die Klasse
		static jclass theClass;
	public:
		virtual jclass getMyClass() const;
		virtual ~java_math_BigDecimal();
		// ein Konstruktor, der fuer das Returnen des Objektes benoetigt wird:
		java_math_BigDecimal( JNIEnv * pEnv, jobject myObj ) : java_lang_Object( pEnv, myObj ){}

        java_math_BigDecimal( const ::rtl::OUString& _par0 );
        java_math_BigDecimal( const double& _par0 );
	};
}

#endif // _CONNECTIVITY_JAVA_MATH_BIGDECIMAL_HXX_


