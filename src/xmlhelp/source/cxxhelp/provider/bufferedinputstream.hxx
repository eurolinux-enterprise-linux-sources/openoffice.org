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

#include <cppuhelper/weak.hxx>
#include <osl/mutex.hxx>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XSeekable.hpp>


namespace chelp {
	
	class BufferedInputStream
		: public cppu::OWeakObject,
		  public com::sun::star::io::XInputStream,
		  public com::sun::star::io::XSeekable
	{
	private:
		
		sal_Int32      m_nBufferLocation;
		sal_Int32      m_nBufferSize;
		sal_Int8      *m_pBuffer;
		osl::Mutex     m_aMutex;
		
	public:
		
		BufferedInputStream(
			const com::sun::star::uno::Reference<com::sun::star::io::XInputStream>& xInputStream);
		
		~BufferedInputStream();
		
		virtual com::sun::star::uno::Any SAL_CALL 
		queryInterface( const com::sun::star::uno::Type& rType )
			throw( com::sun::star::uno::RuntimeException );
		
		virtual void SAL_CALL acquire( void ) throw();
		
		virtual void SAL_CALL release( void ) throw();
		
		
		virtual sal_Int32 SAL_CALL readBytes( com::sun::star::uno::Sequence< sal_Int8 >& aData,
											  sal_Int32 nBytesToRead ) 
			throw( com::sun::star::io::NotConnectedException,
				   com::sun::star::io::BufferSizeExceededException,
				   com::sun::star::io::IOException,
				   com::sun::star::uno::RuntimeException );
		
		virtual sal_Int32 SAL_CALL readSomeBytes( com::sun::star::uno::Sequence< sal_Int8 >& aData,
												  sal_Int32 nMaxBytesToRead )
			throw( com::sun::star::io::NotConnectedException,
				   com::sun::star::io::BufferSizeExceededException,
				   com::sun::star::io::IOException,
				   com::sun::star::uno::RuntimeException );
		
		virtual void SAL_CALL skipBytes( sal_Int32 nBytesToSkip )
			throw( com::sun::star::io::NotConnectedException,
				   com::sun::star::io::BufferSizeExceededException,
				   com::sun::star::io::IOException,
				   com::sun::star::uno::RuntimeException );
		
		virtual sal_Int32 SAL_CALL available( void )
			throw( com::sun::star::io::NotConnectedException,
				   com::sun::star::io::IOException,
				   com::sun::star::uno::RuntimeException );
		
		virtual void SAL_CALL closeInput( void )
			throw( com::sun::star::io::NotConnectedException,
				   com::sun::star::io::IOException,
				   com::sun::star::uno::RuntimeException );
		
		virtual void SAL_CALL seek( sal_Int64 location )
			throw( com::sun::star::lang::IllegalArgumentException,
				   com::sun::star::io::IOException,
				   com::sun::star::uno::RuntimeException );
		
		virtual sal_Int64 SAL_CALL getPosition( void )
			throw( com::sun::star::io::IOException,
				   com::sun::star::uno::RuntimeException );
		
		virtual sal_Int64 SAL_CALL getLength( void )
			throw( com::sun::star::io::IOException,
				   com::sun::star::uno::RuntimeException );
	};
	
	
	extern com::sun::star::uno::Reference<com::sun::star::io::XInputStream>
	turnToSeekable(
		const com::sun::star::uno::Reference<com::sun::star::io::XInputStream>& xInputStream);
	
}
