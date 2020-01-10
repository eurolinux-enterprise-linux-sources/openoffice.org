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

#ifndef _QUEUE_HXX
#define _QUEUE_HXX

#include <tools/solar.h>
#include <tools/contnr.hxx>

// ---------
// - Queue -
// ---------

#define QUEUE_ENTRY_NOTFOUND   CONTAINER_ENTRY_NOTFOUND

class Queue : private Container
{
public:
			using Container::Clear;
			using Container::Count;
			using Container::GetObject;
			using Container::GetPos;

            Queue( USHORT _nInitSize = 16, USHORT _nReSize = 16 ) :
                Container( _nReSize, _nInitSize, _nReSize ) {}
            Queue( const Queue& rQueue ) : Container( rQueue ) {}

    void    Put( void* p ) { Container::Insert( p, CONTAINER_APPEND ); }
    void*   Get()          { return Container::Remove( (ULONG)0 ); }

    Queue&  operator =( const Queue& rQueue )
                { Container::operator =( rQueue ); return *this; }

    BOOL    operator ==( const Queue& rQueue ) const
                { return Container::operator ==( rQueue ); }
    BOOL    operator !=( const Queue& rQueue ) const
                { return Container::operator !=( rQueue ); }
};

// -----------------
// - DECLARE_QUEUE -
// -----------------

#define DECLARE_QUEUE( ClassName, Type )                            \
class ClassName : private Queue                                     \
{                                                                   \
public:                                                             \
                using Queue::Clear;                                 \
                using Queue::Count;                                 \
                                                                    \
                ClassName( USHORT _nInitSize = 16,                  \
                           USHORT _nReSize = 16 ) :                 \
                    Queue( _nInitSize, _nReSize ) {}                \
                ClassName( const ClassName& rClassName ) :          \
                    Queue( rClassName ) {}                          \
                                                                    \
    void        Put( Type p ) { Queue::Put( (void*)p ); }           \
    Type        Get()         { return (Type)Queue::Get(); }        \
                                                                    \
    Type        GetObject( ULONG nIndex ) const                     \
                    { return (Type)Queue::GetObject( nIndex ); }    \
    ULONG       GetPos( const Type p ) const                        \
                    { return Queue::GetPos( (const void*)p ); }     \
    ULONG       GetPos( const Type p, ULONG nStartIndex,            \
                        BOOL bForward = TRUE ) const                \
                    { return Queue::GetPos( (const void*)p,         \
                                            nStartIndex,            \
                                            bForward ); }           \
                                                                    \
    ClassName&  operator =( const ClassName& rClassName )           \
                    { Queue::operator =( rClassName );              \
                      return *this; }                               \
                                                                    \
    BOOL        operator ==( const Queue& rQueue ) const            \
                    { return Queue::operator ==( rQueue ); }        \
    BOOL        operator !=( const Queue& rQueue ) const            \
                    { return Queue::operator !=( rQueue ); }        \
};

#endif // _QUEUE_HXX
