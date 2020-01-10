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
#ifndef IPARSER_HXX
#define IPARSER_HXX

#include <tools/string.hxx>
#include <tools/link.hxx>
#include <tools/stream.hxx>
#include "tools/toolsdllapi.h"

// forwards
class Dir;
class GenericInformation;
class GenericInformationList;

//
// class InformationParser
//

/******************************************************************************
Purpose: reads generic information files into a simple structure in memory
******************************************************************************/

// information files used by this parser have following format:

/*

key [value]
{
	key [value]
	key [value]
	{
		key [value]
		...
		...
	}
}
key [value]
...
...

*/

#define DEF_STAND_LST  "\\\\dev4\\data1\\b_server\\config\\stand.lst"

// error codes:

#define IP_NO_ERROR			0x0000
#define IP_UNEXPECTED_EOF	0x0001

#define REPLACE_VARIABLES	TRUE

class TOOLS_DLLPUBLIC InformationParser
{
private:
	BOOL bRecover;
	ByteString sOldLine;

	ByteString sCurrentComment;

	BOOL bReplaceVariables; // meaning %UPD and %VERSION
	USHORT nLevel;
	ByteString sUPD;
	ByteString sVersion;

	Link aStatusLink;
	SvStream *pActStream;
    UniString sStreamName;

	USHORT nErrorCode;
	ULONG nErrorLine;
	ByteString sErrorText;
	ULONG nActLine;

	// methods
	TOOLS_DLLPRIVATE ByteString &ReadLine();
	TOOLS_DLLPRIVATE GenericInformation *ReadKey( GenericInformationList *pExistingList );
	inline void Recover();

protected:
  BOOL Save( SvStream &rOutStream,
	     const GenericInformationList *pSaveList, USHORT nLevel, BOOL bStripped );
	GenericInformationList *Execute( SvStream &rSourceStream,
								GenericInformationList *pExistingList );
	virtual void PrintStatus( ByteString &rStatus )
		{ if ( aStatusLink.IsSet()) aStatusLink.Call( &rStatus ); }

public:
	InformationParser( BOOL bReplace = FALSE );
	virtual ~InformationParser();

	// the following methods return NULL if any errors are detected

	// reads a information file and stores the data in a
	// GenericInformationList
	GenericInformationList *Execute( SvMemoryStream &rSourceStream,
								GenericInformationList *pExistingList = NULL );
	GenericInformationList *Execute( SvFileStream &rSourceStream,
								GenericInformationList *pExistingList = NULL );
	GenericInformationList *Execute( UniString &rSourceFile,
								GenericInformationList *pExistingList = NULL );
	// reads all information files in the dir and stores the data in a
	// GenericInformationList => first key is the filename
	GenericInformationList *Execute( Dir &rDir,
								GenericInformationList *pExistingList = NULL );

  // save the InfrormationList to rSourceFile
  // returns FALSE on error
  BOOL Save( SvFileStream &rSourceStream,
	     const GenericInformationList *pSaveList );
  BOOL Save( SvMemoryStream &rSourceStream,
	     const GenericInformationList *pSaveList );
  BOOL Save( const UniString &rSourceFile,
	     const GenericInformationList *pSaveList );

	USHORT GetErrorCode();
	ByteString &GetErrorText();

	void SetStatusHdl( const Link &rHdl ) { aStatusLink = rHdl; }
};

#endif

