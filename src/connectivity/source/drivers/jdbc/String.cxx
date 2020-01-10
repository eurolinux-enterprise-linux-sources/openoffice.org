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
#include "precompiled_connectivity.hxx"
#include "java/lang/String.hxx"
#include "java/tools.hxx"
using namespace connectivity;
//**************************************************************
//************ Class: java.lang.String
//**************************************************************

jclass java_lang_String::theClass = 0;

java_lang_String::~java_lang_String()
{}

jclass java_lang_String::getMyClass() const
{
    return st_getMyClass();
}
jclass java_lang_String::st_getMyClass()
{
	// die Klasse muss nur einmal geholt werden, daher statisch
	if( !theClass )
        theClass = findMyClass("java/lang/String");
	return theClass;
}

//--------------------------------------------------------------------------
java_lang_String::java_lang_String( const ::rtl::OUString& _par0 ): java_lang_Object( NULL, (jobject)NULL )
{
	SDBThreadAttach t;
	if( !t.pEnv )
		return;
	jvalue args[1];
	// Parameter konvertieren
	args[0].l = convertwchar_tToJavaString(t.pEnv,_par0);
	// Java-Call fuer den Konstruktor absetzen
	// temporaere Variable initialisieren
	static const char * cSignature = "(Ljava/lang/String;)V";
	jobject tempObj;
	static jmethodID mID(NULL);
    obtainMethodId(t.pEnv, "<init>",cSignature, mID);
	tempObj = t.pEnv->NewObjectA( getMyClass(), mID, args );
	saveRef( t.pEnv, tempObj );
	t.pEnv->DeleteLocalRef( tempObj );
	t.pEnv->DeleteLocalRef((jstring)args[0].l);
}
//--------------------------------------------------------------------------
java_lang_String::operator ::rtl::OUString()
{
	SDBThreadAttach t;
	if(!t.pEnv)
		return ::rtl::OUString();
	return JavaString2String(t.pEnv,(jstring)object);
}

