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
#include "precompiled_shell.hxx"
#include "internal/global.hxx"
#include "zipexcptn.hxx"

//------------------------------------------
/**
*/
RuntimeException::RuntimeException(int Error) : 
    m_Error(Error)
{
}

//------------------------------------------
/**
*/
RuntimeException::~RuntimeException() throw()
{
}

//------------------------------------------
/**
*/
int RuntimeException::GetErrorCode() const
{
	return m_Error;
}

//------------------------------------------
/**
*/
ZipException::ZipException(int Error) : 
	RuntimeException(Error)
{
}

//------------------------------------------
/**
*/
const char* ZipException::what() const throw()
{
	return 0;
}

//------------------------------------------
/**
*/
Win32Exception::Win32Exception(int Error) :
	RuntimeException(Error),
	m_MsgBuff(0)
{
}

//------------------------------------------
/**
*/
Win32Exception::~Win32Exception() throw()
{
#ifndef OS2
	if (m_MsgBuff)
		LocalFree(m_MsgBuff);
#endif
}

//------------------------------------------
/**
*/
const char* Win32Exception::what() const throw()
{
#ifdef OS2
	return "Win32Exception!";
#else
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetErrorCode(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &m_MsgBuff,
		0,
		NULL);
		
	return reinterpret_cast<char*>(m_MsgBuff);
#endif
}

//------------------------------------------
/**
*/
ZipContentMissException::ZipContentMissException(int Error) :
	ZipException(Error)
{
}

//------------------------------------------
/**
*/
AccessViolationException::AccessViolationException(int Error) :
	Win32Exception(Error)
{
}

//------------------------------------------
/**
*/
IOException::IOException(int Error) :
	Win32Exception(Error)
{
}
