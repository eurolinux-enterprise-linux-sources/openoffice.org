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
#include "precompiled_sd.hxx"


#include <tools/debug.hxx>

#include "sdiocmpt.hxx"

//////////////////////////////////////////////////////////////////////////////

old_SdrDownCompat::old_SdrDownCompat(SvStream& rNewStream, UINT16 nNewMode)
:	rStream(rNewStream), 
	nSubRecSiz(0), 
	nSubRecPos(0), 
	nMode(nNewMode),
	bOpen(FALSE)
{
	OpenSubRecord();
}

old_SdrDownCompat::~old_SdrDownCompat()
{
	if(bOpen)
		CloseSubRecord();
}

void old_SdrDownCompat::Read()
{
	rStream >> nSubRecSiz;   
}

void old_SdrDownCompat::Write()
{
	rStream << nSubRecSiz;   
}

void old_SdrDownCompat::OpenSubRecord()
{
	if(rStream.GetError()) 
		return;
	
	nSubRecPos = rStream.Tell(); 

	if(nMode == STREAM_READ) 
	{
		Read();         
	} 
	else if(nMode == STREAM_WRITE) 
	{
		Write();            
	} 

	bOpen = TRUE;
}

void old_SdrDownCompat::CloseSubRecord()
{
	if(rStream.GetError()) 
		return;

	UINT32 nAktPos(rStream.Tell());
	
	if(nMode == STREAM_READ) 
	{
		UINT32 nReadAnz(nAktPos - nSubRecPos);
		if(nReadAnz != nSubRecSiz) 
		{
			rStream.Seek(nSubRecPos + nSubRecSiz); 
		}
	} 
	else if(nMode == STREAM_WRITE) 
	{
		nSubRecSiz = nAktPos - nSubRecPos; 
		rStream.Seek(nSubRecPos);      
		Write();                    
		rStream.Seek(nAktPos);         
	} 
	
	bOpen = FALSE;
}

/*************************************************************************
|*
|* Konstruktor, schreibt bzw. liest Versionsnummer
|*
\************************************************************************/

SdIOCompat::SdIOCompat(SvStream& rNewStream, USHORT nNewMode, UINT16 nVer)
:	old_SdrDownCompat(rNewStream, nNewMode), nVersion(nVer)
{
	if (nNewMode == STREAM_WRITE)
	{
		DBG_ASSERT(nVer != SDIOCOMPAT_VERSIONDONTKNOW,
				   "kann unbekannte Version nicht schreiben");
		rNewStream << nVersion;
	}
	else if (nNewMode == STREAM_READ)
	{
		DBG_ASSERT(nVer == SDIOCOMPAT_VERSIONDONTKNOW,
				   "Lesen mit Angabe der Version ist Quatsch!");
		rNewStream >> nVersion;
	}
}

SdIOCompat::~SdIOCompat() 
{
}

// eof
