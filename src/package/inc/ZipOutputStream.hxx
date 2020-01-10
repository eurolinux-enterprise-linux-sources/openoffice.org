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
#ifndef _ZIP_OUTPUT_STREAM_HXX
#define _ZIP_OUTPUT_STREAM_HXX

#include <ByteChucker.hxx>
#ifndef _DEFLATER_HXX
#include <Deflater.hxx>
#endif
#include <CRC32.hxx>
#include <rtl/cipher.h>
#ifndef RTL_DIGEST_H_
#include <rtl/digest.h>
#endif

#include <vector>

struct ZipEntry;
class EncryptionData;
namespace vos 
{
	template < class T > class ORef;
}
class ZipOutputStream
{
protected:
	com::sun::star::uno::Reference < com::sun::star::io::XOutputStream > xStream;
	::std::vector < ZipEntry * >			aZipList;
	com::sun::star::uno::Sequence < sal_Int8 > aBuffer, aEncryptionBuffer;
	::rtl::OUString		sComment;
	Deflater			aDeflater;
	rtlCipher 			aCipher;
	rtlDigest 			aDigest;
	CRC32				aCRC;
	ByteChucker			aChucker;
	ZipEntry 			*pCurrentEntry;
	sal_Int16			nMethod, nLevel, mnDigested;
	sal_Bool			bFinished, bEncryptCurrentEntry;
	EncryptionData 		*pCurrentEncryptData;

public:
	ZipOutputStream( com::sun::star::uno::Reference < com::sun::star::io::XOutputStream > &xOStream );
	~ZipOutputStream();
	
	// rawWrite to support a direct write to the output stream
    void SAL_CALL rawWrite( ::com::sun::star::uno::Sequence< sal_Int8 >& rBuffer, sal_Int32 nNewOffset, sal_Int32 nNewLength )
		throw(::com::sun::star::io::IOException, ::com::sun::star::uno::RuntimeException);
    void SAL_CALL rawCloseEntry(  )
		throw(::com::sun::star::io::IOException, ::com::sun::star::uno::RuntimeException);

	// XZipOutputStream interfaces
    void SAL_CALL setMethod( sal_Int32 nNewMethod )
		throw(::com::sun::star::uno::RuntimeException);
    void SAL_CALL setLevel( sal_Int32 nNewLevel )
		throw(::com::sun::star::uno::RuntimeException);
    void SAL_CALL putNextEntry( ZipEntry& rEntry, 
			vos::ORef < EncryptionData > &rData,
			sal_Bool bEncrypt = sal_False )
		throw(::com::sun::star::io::IOException, ::com::sun::star::uno::RuntimeException);
    void SAL_CALL closeEntry(  )
		throw(::com::sun::star::io::IOException, ::com::sun::star::uno::RuntimeException);
    void SAL_CALL write( const ::com::sun::star::uno::Sequence< sal_Int8 >& rBuffer, sal_Int32 nNewOffset, sal_Int32 nNewLength )
		throw(::com::sun::star::io::IOException, ::com::sun::star::uno::RuntimeException);
    void SAL_CALL finish(  )
		throw(::com::sun::star::io::IOException, ::com::sun::star::uno::RuntimeException);
    static sal_uInt32 getCurrentDosTime ( );
protected:
	void doDeflate();
	void writeEND(sal_uInt32 nOffset, sal_uInt32 nLength)
		throw(::com::sun::star::io::IOException, ::com::sun::star::uno::RuntimeException);
	void writeCEN( const ZipEntry &rEntry )
		throw(::com::sun::star::io::IOException, ::com::sun::star::uno::RuntimeException);
	void writeEXT( const ZipEntry &rEntry )
		throw(::com::sun::star::io::IOException, ::com::sun::star::uno::RuntimeException);
	sal_Int32 writeLOC( const ZipEntry &rEntry )
		throw(::com::sun::star::io::IOException, ::com::sun::star::uno::RuntimeException);
};

#endif
