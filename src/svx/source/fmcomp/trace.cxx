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
#include "precompiled_svx.hxx"
#include "trace.hxx"
#include <tools/debug.hxx>

#if defined(DBG_UTIL)

//==============================================================================

//------------------------------------------------------------------------------
::vos::OMutex Tracer::s_aMapSafety;
::std::map< ::vos::OThread::TThreadIdentifier, INT32, ::std::less< ::vos::OThread::TThreadIdentifier > >
		Tracer::s_aThreadIndents;

//------------------------------------------------------------------------------
Tracer::Tracer(const char* _pBlockDescription)
	:m_sBlockDescription(_pBlockDescription)
{
	::vos::OGuard aGuard(s_aMapSafety);
	INT32 nIndent = s_aThreadIndents[ ::vos::OThread::getCurrentIdentifier() ]++;

	ByteString sIndent;
	while (nIndent--)
		sIndent += '\t';

    ByteString sThread( ByteString::CreateFromInt32( (INT32)::vos::OThread::getCurrentIdentifier() ) );
	sThread += '\t';

	ByteString sMessage(sThread);
	sMessage += sIndent;
	sMessage += m_sBlockDescription;
    sMessage += " {";
	DBG_TRACE(sMessage.GetBuffer());
}

//------------------------------------------------------------------------------
Tracer::~Tracer()
{
	::vos::OGuard aGuard(s_aMapSafety);
	INT32 nIndent = --s_aThreadIndents[ ::vos::OThread::getCurrentIdentifier() ];

	ByteString sIndent;
	while (nIndent--)
		sIndent += '\t';

    ByteString sThread( ByteString::CreateFromInt32( (INT32)::vos::OThread::getCurrentIdentifier() ) );
	sThread += '\t';

	ByteString sMessage(sThread);
	sMessage += sIndent;
    sMessage += "} // ";
	sMessage += m_sBlockDescription;
	DBG_TRACE(sMessage.GetBuffer());
}

//------------------------------------------------------------------------------
void Tracer::TraceString(const char* _pMessage)
{
	::vos::OGuard aGuard(s_aMapSafety);
	INT32 nIndent = s_aThreadIndents[ ::vos::OThread::getCurrentIdentifier() ];

	ByteString sIndent;
	while (nIndent--)
		sIndent += '\t';

    ByteString sThread( ByteString::CreateFromInt32( (INT32)::vos::OThread::getCurrentIdentifier() ) );
	sThread += '\t';

	ByteString sMessage(sThread);
	sMessage += sIndent;
	sMessage += m_sBlockDescription;
	sMessage += ": ";
	sMessage += _pMessage;
	DBG_TRACE(sMessage.GetBuffer());
}

//------------------------------------------------------------------------------
void Tracer::TraceString1StringParam(const char* _pMessage, const char* _pParam)
{
	::vos::OGuard aGuard(s_aMapSafety);
	INT32 nIndent = s_aThreadIndents[ ::vos::OThread::getCurrentIdentifier() ];

	ByteString sIndent;
	while (nIndent--)
		sIndent += '\t';

    ByteString sThread( ByteString::CreateFromInt32( (INT32)::vos::OThread::getCurrentIdentifier() ) );
	sThread += '\t';

	ByteString sMessage(sThread);
	sMessage += sIndent;
	sMessage += m_sBlockDescription;
	sMessage += ": ";
	sMessage += _pMessage;
	DBG_TRACE1(sMessage.GetBuffer(), _pParam);
}
#endif
