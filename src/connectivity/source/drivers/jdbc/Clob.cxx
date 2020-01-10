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
#include "java/sql/Clob.hxx"
#include "java/tools.hxx"
#include "java/io/Reader.hxx"
#include <connectivity/dbexception.hxx>
using namespace connectivity;
//**************************************************************
//************ Class: java.sql.Clob
//**************************************************************

jclass java_sql_Clob::theClass = 0;

java_sql_Clob::java_sql_Clob( JNIEnv * pEnv, jobject myObj )
	: java_lang_Object( pEnv, myObj )
{
	SDBThreadAttach::addRef();
}
java_sql_Clob::~java_sql_Clob()
{
	SDBThreadAttach::releaseRef();
}

jclass java_sql_Clob::getMyClass() const
{
	// die Klasse muss nur einmal geholt werden, daher statisch
	if( !theClass )
        theClass = findMyClass("java/sql/Clob");
	return theClass;
}

sal_Int64 SAL_CALL java_sql_Clob::length(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException)
{
	jlong out(0);
    SDBThreadAttach t; OSL_ENSURE(t.pEnv,"Java Enviroment geloescht worden!");
	
	{
		// temporaere Variable initialisieren
		static const char * cSignature = "()J";
		static const char * cMethodName = "length";
		// Java-Call absetzen
		static jmethodID mID(NULL);
        obtainMethodId(t.pEnv, cMethodName,cSignature, mID);
		out = t.pEnv->CallLongMethod( object, mID );
		ThrowSQLException(t.pEnv,*this);
	} //t.pEnv
	return (sal_Int64)out;
}

::rtl::OUString SAL_CALL java_sql_Clob::getSubString( sal_Int64 pos, sal_Int32 subStringLength ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException)
{
    SDBThreadAttach t; OSL_ENSURE(t.pEnv,"Java Enviroment geloescht worden!");
	::rtl::OUString aStr;
	{
		// temporaere Variable initialisieren
		static const char * cSignature = "(JI)Ljava/lang/String;";
		static const char * cMethodName = "getSubString";
		// Java-Call absetzen
		static jmethodID mID(NULL);
        obtainMethodId(t.pEnv, cMethodName,cSignature, mID);
		jstring out = (jstring)t.pEnv->CallObjectMethod( object, mID,pos,subStringLength);
		ThrowSQLException(t.pEnv,*this);
		aStr = JavaString2String(t.pEnv,out);
	} //t.pEnv
	// ACHTUNG: der Aufrufer wird Eigentuemer des zurueckgelieferten Zeigers !!!
	return 	aStr;
}

::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream > SAL_CALL java_sql_Clob::getCharacterStream(  ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException)
{
    SDBThreadAttach t;
	static jmethodID mID(NULL);
    jobject out = callObjectMethod(t.pEnv,"getCharacterStream","()Ljava/io/Reader;", mID);

	// ACHTUNG: der Aufrufer wird Eigentuemer des zurueckgelieferten Zeigers !!!
	return out==0 ? 0 : new java_io_Reader( t.pEnv, out );
}

sal_Int64 SAL_CALL java_sql_Clob::position( const ::rtl::OUString& searchstr, sal_Int32 start ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException)
{
	jlong out(0);
    SDBThreadAttach t; OSL_ENSURE(t.pEnv,"Java Enviroment geloescht worden!");
	
	{
		jvalue args[1];
		// Parameter konvertieren
		args[0].l = convertwchar_tToJavaString(t.pEnv,searchstr);
		// temporaere Variable initialisieren
		static const char * cSignature = "(Ljava/lang/String;I)J";
		static const char * cMethodName = "position";
		// Java-Call absetzen
		static jmethodID mID(NULL);
        obtainMethodId(t.pEnv, cMethodName,cSignature, mID);
		out = t.pEnv->CallLongMethod( object, mID, args[0].l,start );
		ThrowSQLException(t.pEnv,*this);
		t.pEnv->DeleteLocalRef((jstring)args[0].l);
	} //t.pEnv
	return (sal_Int64)out;
}

sal_Int64 SAL_CALL java_sql_Clob::positionOfClob( const ::com::sun::star::uno::Reference< ::com::sun::star::sdbc::XClob >& /*pattern*/, sal_Int64 /*start*/ ) throw(::com::sun::star::sdbc::SQLException, ::com::sun::star::uno::RuntimeException)
{
    ::dbtools::throwFeatureNotImplementedException( "XClob::positionOfClob", *this );
    // this was put here in CWS warnings01. The previous implementation was defective, as it did ignore
    // the pattern parameter. Since the effort for proper implementation is rather high - we would need
    // to translated patter into a byte[] -, we defer this functionality for the moment (hey, it was
    // unusable, anyway)
    // 2005-11-15 / #i57457# / frank.schoenheit@sun.com
    return 0;
}


