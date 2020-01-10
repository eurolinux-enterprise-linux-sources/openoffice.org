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

#ifndef _SOLAR_H
#define _SOLAR_H

#include <sal/types.h>
#include <osl/endian.h>
#include <comphelper/fileformat.h>

/*** common solar defines ***********************************/

#ifndef TRUE
#define TRUE		1
#endif
#ifndef FALSE
#define FALSE		0
#endif

#ifdef _SOLAR__PRIVATE
#undef _SOLAR__PRIVATE
#endif
#define _SOLAR__PRIVATE 1
#define __REFERENCED	0

/*** common solar types ********************************************/
/* NOTE: these types are deprecated, or soon will be.  They should */
/* not be used in new code, and should be replaced with their      */
/* corresponding types from sal/types.h in old code when possible. */
/*   Previous typedefs from before sal convergence are in comments */
/*   to the right of the new types.                                */

typedef sal_Bool		BOOL;	/* typedef unsigned char   BOOL; */
typedef sal_uInt8		BYTE;	/* typedef unsigned char   BYTE; */
typedef sal_uInt16		USHORT;	/* typedef unsigned short  USHORT; */
typedef sal_uIntPtr		ULONG;	/* typedef unsigned long   ULONG; */
typedef int 			FASTBOOL;

#if !defined(VCL_NEED_BASETSD) || defined(__MINGW32__)
#ifndef _SOLAR_NOUINT
typedef sal_Int16			INT16;	/* typedef short           INT16; */
typedef sal_uInt16			UINT16;	/* typedef unsigned short  UINT16; */
#if defined(SAL_W32)
typedef sal_sChar			INT8;
#else
typedef sal_Char			INT8;	/* typedef char            INT8; */
#endif
typedef sal_uInt8			UINT8;	/* typedef unsigned char   UINT8; */
#endif /* _SOLAR_NOUINT */
#endif

#ifndef VCL_NEED_BASETSD
#ifndef _SOLAR_NOUINT
/* types with exact defined size (not just the minimum size) */
typedef sal_Int64			INT64;	/* typedef long            INT64;  previously "void" on Windows */
typedef sal_uInt64			UINT64;	/* typedef unsigned long   UINT64; previously "void" on Windows */
typedef sal_Int32			INT32;	/* typedef int             INT32;  previously "long" on Windows */
typedef sal_uInt32			UINT32;	/* typedef unsigned int    UINT32; previously "unsigned long" on Windows */
#endif /* _SOLAR_NOUINT */
#endif

/*** misc. macros to leverage platform and compiler differences ********/

#define DELETEZ( p )	( delete p,p = 0 )

#define __FAR_DATA
#define __READONLY_DATA 		const
#define __EXPORT

#ifdef WNT
#if defined (_MSC_VER) && ( _MSC_VER < 1200 )
#define __LOADONCALLAPI _cdecl
#else
#define __LOADONCALLAPI __cdecl
#endif
#else
#define __LOADONCALLAPI
#endif

#if defined UNX
#define ILLEGAL_POINTER ((void*)1)
#else
#define ILLEGAL_POINTER NULL
#endif

/*** solar binary types **********************************************/

#ifndef _SOLAR_NOSVBT
/* Solar (portable) Binary (exchange) Type; OSI 6 subset
   always little endian;
   not necessarily aligned */

typedef BYTE				SVBT8[1];
typedef BYTE				SVBT16[2];
typedef BYTE				SVBT32[4];
typedef BYTE				SVBT64[8];

#ifdef __cplusplus

inline BYTE 	SVBT8ToByte  ( const SVBT8	p ) { return p[0]; }
#if defined OSL_LITENDIAN && SAL_TYPES_ALIGNMENT2 == 1
inline USHORT	SVBT16ToShort( const SVBT16 p ) { return *(USHORT*)p; }
#else
inline USHORT	SVBT16ToShort( const SVBT16 p ) { return (USHORT)p[0]
												   + ((USHORT)p[1] <<  8); }
#endif
#if defined OSL_LITENDIAN && SAL_TYPES_ALIGNMENT4 == 1
inline sal_uInt32	SVBT32ToUInt32 ( const SVBT32 p ) { return *(sal_uInt32*)p; }
#else
inline sal_uInt32	SVBT32ToUInt32 ( const SVBT32 p ) { return (sal_uInt32)p[0]
												   + ((sal_uInt32)p[1] <<  8)
												   + ((sal_uInt32)p[2] << 16)
												   + ((sal_uInt32)p[3] << 24); }
#endif
#if defined OSL_LITENDIAN && SAL_TYPES_ALIGNMENT8 == 1
inline double	SVBT64ToDouble( const SVBT64 p ) { return *(double*)p; }
#elif defined OSL_LITENDIAN
inline double	SVBT64ToDouble( const SVBT64 p ) { double n;
													((BYTE*)&n)[0] = p[0];
													((BYTE*)&n)[1] = p[1];
													((BYTE*)&n)[2] = p[2];
													((BYTE*)&n)[3] = p[3];
													((BYTE*)&n)[4] = p[4];
													((BYTE*)&n)[5] = p[5];
													((BYTE*)&n)[6] = p[6];
													((BYTE*)&n)[7] = p[7];
													return n; }
#else
inline double	SVBT64ToDouble( const SVBT64 p ) { double n;
													((BYTE*)&n)[0] = p[7];
													((BYTE*)&n)[1] = p[6];
													((BYTE*)&n)[2] = p[5];
													((BYTE*)&n)[3] = p[4];
													((BYTE*)&n)[4] = p[3];
													((BYTE*)&n)[5] = p[2];
													((BYTE*)&n)[6] = p[1];
													((BYTE*)&n)[7] = p[0];
													return n; }
#endif

inline void 	ByteToSVBT8  ( BYTE   n, SVBT8	p ) { p[0] = n; }
#if defined OSL_LITENDIAN && SAL_TYPES_ALIGNMENT2 == 1
inline void 	ShortToSVBT16( USHORT n, SVBT16 p ) { *(USHORT*)p = n; }
#else
inline void 	ShortToSVBT16( USHORT n, SVBT16 p ) { p[0] = (BYTE) n;
													  p[1] = (BYTE)(n >>  8); }
#endif
#if defined OSL_LITENDIAN && SAL_TYPES_ALIGNMENT4 == 1
inline void 	UInt32ToSVBT32 ( sal_uInt32  n, SVBT32 p ) { *(sal_uInt32*)p = n; }
#else
inline void 	UInt32ToSVBT32 ( sal_uInt32  n, SVBT32 p ) { p[0] = (BYTE) n;
													  p[1] = (BYTE)(n >>  8);
													  p[2] = (BYTE)(n >> 16);
													  p[3] = (BYTE)(n >> 24); }
#endif
#if defined OSL_LITENDIAN && SAL_TYPES_ALIGNMENT8 == 1
inline void 	DoubleToSVBT64( double n, SVBT64 p ) { *(double*)p = n; }
#elif defined OSL_LITENDIAN
inline void 	DoubleToSVBT64( double n, SVBT64 p ) { p[0] = ((BYTE*)&n)[0];
													   p[1] = ((BYTE*)&n)[1];
													   p[2] = ((BYTE*)&n)[2];
													   p[3] = ((BYTE*)&n)[3];
													   p[4] = ((BYTE*)&n)[4];
													   p[5] = ((BYTE*)&n)[5];
													   p[6] = ((BYTE*)&n)[6];
													   p[7] = ((BYTE*)&n)[7]; }
#else
inline void 	DoubleToSVBT64( double n, SVBT64 p ) { p[0] = ((BYTE*)&n)[7];
													   p[1] = ((BYTE*)&n)[6];
													   p[2] = ((BYTE*)&n)[5];
													   p[3] = ((BYTE*)&n)[4];
													   p[4] = ((BYTE*)&n)[3];
													   p[5] = ((BYTE*)&n)[2];
													   p[6] = ((BYTE*)&n)[1];
													   p[7] = ((BYTE*)&n)[0]; }
#endif
#endif
#endif


/*** standard floating point definitions *******************************/

#ifndef F_PI
#define F_PI		3.14159265358979323846
#endif
#ifndef F_PI2
#define F_PI2		1.57079632679489661923
#endif
#ifndef F_PI4
#define F_PI4		0.785398163397448309616
#endif
#ifndef F_PI180
#define F_PI180 	0.01745329251994
#endif
#ifndef F_PI1800
#define F_PI1800	0.001745329251994
#endif
#ifndef F_PI18000
#define F_PI18000	0.0001745329251994
#endif
#ifndef F_2PI
#define F_2PI		6.28318530717958647694
#endif


/*** standard macros *****************************************/

#define SWAPSHORT(x) ((((x) >> 8) & 0x00FF) | (((x) & 0x00FF) << 8))
#define SWAPLONG(x)  ((((x) >> 24) & 0x000000FF) | (((x) & 0x00FF0000) >> 8) | \
					  (((x) & 0x0000FF00) <<  8) | (((x) & 0x000000FF) << 24))

#ifndef __cplusplus
#ifndef min
#define min(a,b)	(((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#endif



/*** standard inline functions *****************************************/

#ifdef __cplusplus
template<typename T> inline T Min(T a, T b) { return (a<b?a:b); }
template<typename T> inline T Max(T a, T b) { return (a>b?a:b); }
template<typename T> inline T Abs(T a) { return (a>=0?a:-a); }
#endif


/*** C / C++ - macros **************************************************/

#ifdef __cplusplus
#define BEGIN_C 	extern "C" {
#define END_C		}
#define EXTERN_C	extern "C"
#else
#define BEGIN_C
#define END_C
#define EXTERN_C
#endif


/*** macros ************************************************************/

#ifdef NOHACKS
#define HACK( comment ) #error hack: comment
#else
#define HACK( comment )
#endif

#define _MAKE_NUMSTR( n )			# n
#define MAKE_NUMSTR( n )			_MAKE_NUMSTR( n )

#define _LF 	((char)0x0A)
#define _CR 	((char)0x0D)


/*** pragmas ************************************************************/

#if defined _MSC_VER
/* deletion of pointer to incomplete type '...'; no destructor called
 serious error, memory deleted without call of dtor */
#pragma warning( error: 4150 )
// warning C4002: too many actual parameters for macro
// warning C4003: not enough actual parameters for macro
#pragma warning(error : 4002 4003)
#endif


/* dll file extensions *******************************************************/

/* many of these platforms are historic */
#define SYSTEM_WINMSCI		 1
#define SYSTEM_WNTMSCI		 2
#define SYSTEM_WNTMSCA		 3
#define SYSTEM_WNTMSCP		 4
#define SYSTEM_WNTMSCM		 5
#define SYSTEM_OS2BLCI		 6
#define SYSTEM_OS2ICCI		 7
#define SYSTEM_OS2ICCI3 	 8
#define SYSTEM_UNXLNXI		 9
#define SYSTEM_UNXSOLS		10
#define SYSTEM_UNXBSDI		11
#define SYSTEM_UNXBSDA		12
#define SYSTEM_UNXSCOI		13
#define SYSTEM_UNXAIXP		14
#define SYSTEM_UNXHPXR		15
#define SYSTEM_UNXSNIM		16
#define SYSTEM_UNXMVSG		17
#define SYSTEM_UNXIRXM		18
// #define SYSTEM_MACOSP		19
#define SYSTEM_UNXFBSDI 	20
#define SYSTEM_UNXSOLI		21
#define SYSTEM_WINBLCI		22
#define SYSTEM_UNXMACXP 	23
#define SYSTEM_UNXLNXP		24
#define SYSTEM_UNXBSDS		25
#define SYSTEM_UNXLNXR		26
#define SYSTEM_UNXLNX3		28
#define SYSTEM_UNXLNXS		29
#define SYSTEM_UNXLNXX		30
#define SYSTEM_UNXSOGS		31
#define SYSTEM_UNXSOGI		32
#define SYSTEM_UNXMACXI		33
#define SYSTEM_OS2GCCI		34
#define SYSTEM_WNTGCCI  	99

#if defined WNT
#if defined(__MINGW32__)
  #define __DLLEXTENSION    "gi"
#else
  #define __DLLEXTENSION "mi"
#endif
#elif defined OS2
  #define __DLLEXTENSION "go"
#elif defined UNX
#ifdef AIX
  #define __DLLEXTENSION "ap.so"
#elif defined HPUX
  #define __DLLEXTENSION "hr.sl"
#elif defined SOLARIS && defined SPARC && defined IS_LP64
  #define __DLLEXTENSION "su.so"
#elif defined SOLARIS && defined SPARC && !defined __GNUC__
  #define __DLLEXTENSION "ss.so"
#elif defined SOLARIS && defined SPARC && defined __GNUC__
  #define __DLLEXTENSION "sogs.so"
#elif defined SOLARIS && defined INTEL && !defined __GNUC__
  #define __DLLEXTENSION "si.so"
#elif defined SOLARIS && defined INTEL && defined __GNUC__
  #define __DLLEXTENSION "sogi.so"
#elif defined SCO
  #define __DLLEXTENSION "ci.so"
#elif defined NETBSD && defined X86
  #define __DLLEXTENSION "bi.so"
#elif defined NETBSD && defined ARM32
  #define __DLLEXTENSION "ba.so"
#elif defined NETBSD && defined SPARC
  #define __DLLEXTENSION "bs.so"
#elif defined NETBSD && defined POWERPC
  #define __DLLEXTENSION "bp.so"
#elif defined LINUX && defined X86
  #define __DLLEXTENSION "li.so"
#elif defined LINUX && defined POWERPC
  #define __DLLEXTENSION "lp.so"
#elif defined LINUX && defined S390
  #define __DLLEXTENSION "l3.so"
#elif defined LINUX && defined ARM32
  #define __DLLEXTENSION "lr.so"
#elif defined LINUX && defined SPARC
  #define __DLLEXTENSION "ls.so"
#elif defined LINUX && defined __x86_64__
  #define __DLLEXTENSION "lx.so"
#elif defined LINUX && defined MIPS
  #define __DLLEXTENSION "lm.so"
#elif defined LINUX && defined IA64
  #define __DLLEXTENSION "la.so"
#elif defined LINUX && defined M68K
  #define __DLLEXTENSION "lm.so"
#elif defined LINUX
  #error unknown plattform
#elif defined FREEBSD && defined X86
  #define __DLLEXTENSION "fi.so"
#elif defined FREEBSD && defined X86_64
  #define __DLLEXTENSION "fx.so"
#elif defined IRIX
  #define __DLLEXTENSION "im.so"
#elif defined MACOSX && defined POWERPC
  #define __DLLEXTENSION "mxp.dylib"
#elif defined MACOSX && defined X86
  #define __DLLEXTENSION "mxi.dylib"
#else
  #define __DLLEXTENSION ".so"
#endif
#endif

// -----------------------------------------------------------------------

#define NOOLDSTRING
#ifndef NOREPLACESTRING
#define UniString		String
#define XubString		String
#else
#define XubString		UniString
#endif
#define xub_Unicode 	sal_Unicode
#define xub_uUnicode	sal_Unicode
#ifdef STRING32
#define xub_StrLen		sal_uInt32
#else
#define xub_StrLen		USHORT
#endif

// -- moved here from libcall.hxx ----------------------------------------

#define LIBRARY_STR(s)		# s
#define LIBRARY_STRING(s)	LIBRARY_STR(s)

#define GETFUNCTION( s ) GetFunction( s )
#define LIBRARY_CONCAT3( s1, s2, s3 ) \
	s1 s2 s3
#define LIBRARY_CONCAT4( s1, s2, s3, s4 ) \
	s1 s2 s3 s4

#if defined WIN || defined WNT || defined OS2
#define SVLIBRARY( Base ) \
	LIBRARY_CONCAT3( Base, __DLLEXTENSION, ".DLL" )
#define SVLIBRARYLANG( Base, Lang ) \
	LIBRARY_CONCAT3( Base, Lang, ".DLL" )
#elif defined UNX
#define SVLIBRARY( Base ) \
	LIBRARY_CONCAT3( "lib", Base, __DLLEXTENSION )
#define SVLIBRARYLANG( Base, Lang ) \
	LIBRARY_CONCAT3( "lib", Base, Lang )
#else
#define SVLIBRARY( Base ) \
	LIBRARY_CONCAT2( Base, __DLLEXTENSION )
#define SVLIBRARYLANG( Base, Lang ) \
	LIBRARY_CONCAT2( Base, Lang )
#endif

#if defined MACOSX
#define SV_LIBFILENAME(str) \
	LIBRARYFILENAME_CONCAT2( str, __DLLEXTENSION )
#elif defined UNX
#define SV_LIBFILENAME(str) \
	LIBRARYFILENAME_CONCAT2( str, __DLLEXTENSION )
#else
#define SV_LIBFILENAME(str) \
	LIBRARYFILENAME_CONCAT3( str, __DLLEXTENSION, ".dll" )
#endif

#endif	/* _SOLAR_H */
