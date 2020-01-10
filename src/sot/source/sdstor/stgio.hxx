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

#ifndef _STGIO_HXX
#define _STGIO_HXX

#include <stgcache.hxx>
#include <stgelem.hxx>
#include <tools/string.hxx>

class StgFATStrm;
class StgDataStrm;
class StgDirStrm;
class String;

enum FAT_ERROR
{
	FAT_OK,
	FAT_WRONGLENGTH,
	FAT_UNREFCHAIN,
	FAT_OVERWRITE,
	FAT_OUTOFBOUNDS,

	FAT_INMEMORYERROR,
	FAT_ONFILEERROR,
	FAT_BOTHERROR
};

struct StgLinkArg
{
	String aFile;
	ULONG nErr;
};

class StgIo : public StgCache {
	void SetupStreams();			// load all internal streams
	BOOL         bCopied;
public:
	StgIo();
   ~StgIo();
	StgHeader    aHdr;				// storage file header
	StgFATStrm*  pFAT;				// FAT stream
	StgDirStrm*  pTOC;				// TOC stream
	StgDataStrm* pDataFAT;			// small data FAT stream
	StgDataStrm* pDataStrm;			// small data stream
	short	  	 GetDataPageSize();	// get the logical data page size
	BOOL Load();					// load a storage file
	BOOL Init();					// set up an empty file
	BOOL CommitAll();				// commit everything (root commit)
	
	static void SetErrorLink( const Link& );
	static const Link& GetErrorLink();
	ULONG ValidateFATs( );
};

#endif
