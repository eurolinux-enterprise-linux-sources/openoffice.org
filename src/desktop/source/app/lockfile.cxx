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
#include "precompiled_desktop.hxx"
#include <stdlib.h>
#include <time.h>
#ifdef WNT
#include <tools/prewin.h>
#include <windows.h>
#include <tools/postwin.h>
#endif
#include <sal/types.h>
#include <osl/file.hxx>
#include <osl/socket.hxx>
#include <osl/security.hxx>
#include <unotools/bootstrap.hxx>
#include <tools/string.hxx>
#include <tools/config.hxx>

#include "lockfile.hxx"


using namespace ::osl;
using namespace ::rtl;
using namespace ::utl;


namespace desktop {

	// initialize static members...
	// lock suffix
	const OUString Lockfile::Suffix()
	    { return OUString::createFromAscii( "/.lock" ); }
	// values for datafile
	const ByteString Lockfile::Group()
        { return ByteString( "Lockdata" ); }
	const ByteString Lockfile::Userkey()
        { return ByteString( "User" ); }
	const ByteString Lockfile::Hostkey()
        { return ByteString( "Host" ); }
	const ByteString Lockfile::Stampkey()
        { return ByteString( "Stamp" ); }
	const ByteString Lockfile::Timekey()
        { return ByteString( "Time" ); }
	const ByteString Lockfile::IPCkey()
        { return ByteString( "IPCServer" ); }

	Lockfile::Lockfile( bool bIPCserver )
    :m_bIPCserver(bIPCserver)
	,m_bRemove(sal_False)
    ,m_bIsLocked(sal_False)
	{
		// build the file-url to use for the lock
		OUString aUserPath;
        utl::Bootstrap::locateUserInstallation( aUserPath );
		m_aLockname = aUserPath + Suffix();

		// generate ID
		const int nIdBytes = 16;
		char tmpId[nIdBytes*2+1];
		time_t t;
		srand( (unsigned)(t = time( NULL )) );
		int tmpByte = 0;
		for (int i = 0; i<nIdBytes; i++) {
			tmpByte = rand( ) % 0xFF;
			sprintf( tmpId+i*2, "%02X", tmpByte ); // #100211# - checked
		}
		tmpId[nIdBytes*2]=0x00;
		m_aId = OUString::createFromAscii( tmpId );

		// generate date string
		char *tmpTime = ctime( &t );
		if (tmpTime != NULL) {
            m_aDate = OUString::createFromAscii( tmpTime );
            sal_Int32 i = m_aDate.indexOf('\n');
            if (i > 0)
                m_aDate = m_aDate.copy(0, i);
        }


		// try to create file
		File aFile(m_aLockname);
		if (aFile.open( OpenFlag_Create ) == File::E_EXIST) {
			m_bIsLocked = sal_True;
		} else {
			// new lock created
			aFile.close( );
			syncToFile( );
			m_bRemove = sal_True;
		}
	}

	sal_Bool Lockfile::check( fpExecWarning execWarning )
	{

		if (m_bIsLocked) {
			// lock existed, ask user what to do
			if (isStale() ||
                (execWarning != 0 && (*execWarning)( this ))) {
				// remove file and create new
				File::remove( m_aLockname );
				File aFile(m_aLockname);
				aFile.open( OpenFlag_Create );
				aFile.close( );
				syncToFile( );
				m_bRemove = sal_True;
				return sal_True;
			} else {
				//leave alone and return false
				m_bRemove = sal_False;
				return sal_False;
			}
		} else {
			// lock was created by us
			return sal_True;
		}
	}

	sal_Bool Lockfile::isStale( void ) const
	{
		// this checks whether the lockfile was created on the same
		// host by the same user. Should this be the case it is safe
		// to assume that it is a stale lockfile which can be overwritten
		String aLockname = m_aLockname;
		Config aConfig(aLockname);
		aConfig.SetGroup(Group());
		ByteString aIPCserver  = aConfig.ReadKey( IPCkey() );
        if (! aIPCserver.EqualsIgnoreCaseAscii( "true" ))
            return false;

		ByteString aHost  = aConfig.ReadKey( Hostkey() );
		ByteString aUser  = aConfig.ReadKey( Userkey() );
		// lockfile from same host?
		ByteString myHost;
#ifdef WNT
        /*
          prevent windows from connecting to the net to get it's own
          hostname by using the netbios name
        */
        sal_Int32 sz = MAX_COMPUTERNAME_LENGTH + 1;
        char* szHost = new char[sz];
        if (GetComputerName(szHost, (LPDWORD)&sz))
            myHost = OString(szHost);
        else
            myHost = OString("UNKNOWN");
        delete[] szHost;
#else
		oslSocketResult sRes;
        myHost  = OUStringToOString(
			SocketAddr::getLocalHostname( &sRes ), RTL_TEXTENCODING_ASCII_US );
#endif
		if (aHost == myHost) {
			// lockfile by same UID
			OUString myUserName;
			Security aSecurity;
			aSecurity.getUserName( myUserName );
			ByteString myUser  = OUStringToOString( myUserName, RTL_TEXTENCODING_ASCII_US );
			if (aUser == myUser)
				return sal_True;
		}
		return sal_False;
	}

	void Lockfile::syncToFile( void ) const
	{
		String aLockname = m_aLockname;
		Config aConfig(aLockname);
		aConfig.SetGroup(Group());

		// get information
        ByteString aHost;
#ifdef WNT
        /*
          prevent windows from connecting to the net to get it's own
          hostname by using the netbios name
        */
        sal_Int32 sz = MAX_COMPUTERNAME_LENGTH + 1;
        char* szHost = new char[sz];
        if (GetComputerName(szHost, (LPDWORD)&sz))
            aHost = OString(szHost);
        else
            aHost = OString("UNKNOWN");
        delete[] szHost;
#else
		oslSocketResult sRes;
		aHost  = OUStringToOString(
			SocketAddr::getLocalHostname( &sRes ), RTL_TEXTENCODING_ASCII_US );
#endif
		OUString aUserName;
		Security aSecurity;
		aSecurity.getUserName( aUserName );
		ByteString aUser  = OUStringToOString( aUserName, RTL_TEXTENCODING_ASCII_US );		
		ByteString aTime  = OUStringToOString( m_aDate, RTL_TEXTENCODING_ASCII_US );
		ByteString aStamp = OUStringToOString( m_aId, RTL_TEXTENCODING_ASCII_US );

		// write information
		aConfig.WriteKey( Userkey(),  aUser );
		aConfig.WriteKey( Hostkey(),  aHost );
		aConfig.WriteKey( Stampkey(), aStamp );
		aConfig.WriteKey( Timekey(),  aTime );
		aConfig.WriteKey(
            IPCkey(),
            m_bIPCserver ? ByteString("true") : ByteString("false") );
		aConfig.Flush( );
	}

	void Lockfile::clean( void )
	{
		if ( m_bRemove )
		{
			File::remove( m_aLockname );
			m_bRemove = sal_False;
		}
	}

	Lockfile::~Lockfile( void )
	{
		// unlock userdata by removing file
		if ( m_bRemove )
			File::remove( m_aLockname );
	}
}









