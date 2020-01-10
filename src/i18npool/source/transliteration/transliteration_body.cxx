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
#include "precompiled_i18npool.hxx"

#include <i18nutil/casefolding.hxx>
#define TRANSLITERATION_ALL
#include "transliteration_body.hxx"

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::rtl;

namespace com { namespace sun { namespace star { namespace i18n {

Transliteration_body::Transliteration_body()
{
	nMappingType = 0;
	transliterationName = "Transliteration_body";
	implementationName = "com.sun.star.i18n.Transliteration.Transliteration_body";
}

sal_Int16 SAL_CALL Transliteration_body::getType() throw(RuntimeException)
{
	return TransliterationType::ONE_TO_ONE;
}

sal_Bool SAL_CALL Transliteration_body::equals(
	const OUString& /*str1*/, sal_Int32 /*pos1*/, sal_Int32 /*nCount1*/, sal_Int32& /*nMatch1*/,
	const OUString& /*str2*/, sal_Int32 /*pos2*/, sal_Int32 /*nCount2*/, sal_Int32& /*nMatch2*/)
	throw(RuntimeException)
{
	throw RuntimeException();
}

Sequence< OUString > SAL_CALL 
Transliteration_body::transliterateRange( const OUString& str1, const OUString& str2 ) 
	throw( RuntimeException)
{
	Sequence< OUString > ostr(2);
	ostr[0] = str1;
	ostr[1] = str2;
	return ostr;
}

OUString SAL_CALL 
Transliteration_body::transliterate( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount, 
	Sequence< sal_Int32 >& offset) throw(RuntimeException)
{
#if 0
/* Performance optimization:
 * The two realloc() consume 48% (32% grow, 16% shrink) runtime of this method!
 * getValue() needs about 15%, so there is equal balance if we trade the second
 * (shrinking) realloc() for a getValue(). But if the caller initializes the
 * sequence to nCount elements there isn't any change in size necessary in most
 * cases (one-to-one mapping) and we gain 33%.
 *
 * Of that constellation the getValue() method takes 20% upon each call, so 40%
 * for both. By remembering the first calls' results we could gain some extra
 * percentage again, but unfortunately getValue() may return a reference to a
 * static buffer, so we can't store the pointer directly but would have to
 * copy-construct an array, which doesn't give us any advantage.
 *
 * Much more is accomplished by working directly on the sequence buffer
 * returned by getArray() instead of using operator[] for each and every
 * access.
 *
 * And while we're at it: now that we know the size in advance we don't need to
 * copy the buffer anymore, just create the real string buffer and let the
 * return value take ownership.
 *
 * All together these changes result in the new implementation needing only 62%
 * of the time of the old implementation (in other words: that one was 1.61
 * times slower ...)
 */

    // Allocate the max possible buffer. Try to use stack instead of heap which
    // would have to be reallocated most times anyway.
    const sal_Int32 nLocalBuf = 512 * NMAPPINGMAX;
    sal_Unicode aLocalBuf[nLocalBuf], *out = aLocalBuf, *aHeapBuf = NULL;

    const sal_Unicode *in = inStr.getStr() + startPos;

    if (nCount > 512)
        out = aHeapBuf =  (sal_Unicode*) malloc((nCount * NMAPPINGMAX) * sizeof(sal_Unicode));

        if (useOffset)
            offset.realloc(nCount * NMAPPINGMAX);
	sal_Int32 j = 0;
	for (sal_Int32 i = 0; i < nCount; i++) {
	    Mapping &map = casefolding::getValue(in, i, nCount, aLocale, nMappingType);
	    for (sal_Int32 k = 0; k < map.nmap; k++) {
                if (useOffset)
                    offset[j] = i + startPos;
		out[j++] = map.map[k];
	    }
	}
        if (useOffset)
            offset.realloc(j);

	OUString r(out, j);

	if (aHeapBuf)
	    free(aHeapBuf);

	return r;
#else
    const sal_Unicode *in = inStr.getStr() + startPos;

    // Two different blocks to eliminate the if(useOffset) condition inside the
    // inner k loop. Yes, on massive use even such small things do count.
    if ( useOffset )
    {
        sal_Int32 nOffCount = 0, i;
        for (i = 0; i < nCount; i++)
        {
            const Mapping &map = casefolding::getValue(in, i, nCount, aLocale, nMappingType);
            nOffCount += map.nmap;
        }
        rtl_uString* pStr = x_rtl_uString_new_WithLength( nOffCount, 1 );  // our x_rtl_ustring.h
        sal_Unicode* out = pStr->buffer;

        if ( nOffCount != offset.getLength() )
            offset.realloc( nOffCount );

        sal_Int32 j = 0;
        sal_Int32 * pArr = offset.getArray();
        for (i = 0; i < nCount; i++)
        {
            const Mapping &map = casefolding::getValue(in, i, nCount, aLocale, nMappingType);
            for (sal_Int32 k = 0; k < map.nmap; k++)
            {
                pArr[j] = i + startPos;
                out[j++] = map.map[k];
            }
        }
        out[j] = 0;

        return OUString( pStr, SAL_NO_ACQUIRE );
    }
    else
    {
        // In the simple case of no offset sequence used we can eliminate the
        // first getValue() loop. We could also assume that most calls result
        // in identical string lengths, thus using a preallocated
        // OUStringBuffer could be an easy way to assemble the return string
        // without too much hassle. However, for single characters the
        // OUStringBuffer::append() method is quite expensive compared to a
        // simple array operation, so it pays here to copy the final result
        // instead.

        // Allocate the max possible buffer. Try to use stack instead of heap,
        // which would have to be reallocated most times anyways.
        const sal_Int32 nLocalBuf = 2048;
        sal_Unicode aLocalBuf[ nLocalBuf * NMAPPINGMAX ], *out = aLocalBuf, *pHeapBuf = NULL;
        if ( nCount > nLocalBuf )
            out = pHeapBuf = new sal_Unicode[ nCount * NMAPPINGMAX ];

        sal_Int32 j = 0;
        for ( sal_Int32 i = 0; i < nCount; i++)
        {
            const Mapping &map = casefolding::getValue(in, i, nCount, aLocale, nMappingType);
            for (sal_Int32 k = 0; k < map.nmap; k++)
            {
                out[j++] = map.map[k];
            }
        }

        OUString aRet( out, j );
        if ( pHeapBuf )
            delete [] pHeapBuf;
        return aRet;
    }
#endif
}

OUString SAL_CALL 
Transliteration_body::transliterateChar2String( sal_Unicode inChar ) throw(RuntimeException)
{
        const Mapping &map = casefolding::getValue(&inChar, 0, 1, aLocale, nMappingType);
        rtl_uString* pStr = x_rtl_uString_new_WithLength( map.nmap, 1 );  // our x_rtl_ustring.h
        sal_Unicode* out = pStr->buffer;
        sal_Int32 i;

        for (i = 0; i < map.nmap; i++)
            out[i] = map.map[i];
        out[i] = 0;

        return OUString( pStr, SAL_NO_ACQUIRE );
}

sal_Unicode SAL_CALL 
Transliteration_body::transliterateChar2Char( sal_Unicode inChar ) throw(MultipleCharsOutputException, RuntimeException)
{
        const Mapping &map = casefolding::getValue(&inChar, 0, 1, aLocale, nMappingType);
        if (map.nmap > 1)
            throw MultipleCharsOutputException();
        return map.map[0];
}

OUString SAL_CALL
Transliteration_body::folding( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount, 
	Sequence< sal_Int32 >& offset) throw(RuntimeException)
{
	return this->transliterate(inStr, startPos, nCount, offset);
}

Transliteration_casemapping::Transliteration_casemapping()
{
	nMappingType = 0;
	transliterationName = "casemapping(generic)";
	implementationName = "com.sun.star.i18n.Transliteration.Transliteration_casemapping";
}

void SAL_CALL
Transliteration_casemapping::setMappingType( const sal_uInt8 rMappingType, const Locale& rLocale )
{
	nMappingType = rMappingType;
	aLocale = rLocale;
}

Transliteration_u2l::Transliteration_u2l()
{
	nMappingType = MappingTypeUpperToLower;
	transliterationName = "upper_to_lower(generic)";
	implementationName = "com.sun.star.i18n.Transliteration.Transliteration_u2l";
}

Transliteration_l2u::Transliteration_l2u()
{
	nMappingType = MappingTypeLowerToUpper;
	transliterationName = "lower_to_upper(generic)";
	implementationName = "com.sun.star.i18n.Transliteration.Transliteration_l2u";
}

} } } }
