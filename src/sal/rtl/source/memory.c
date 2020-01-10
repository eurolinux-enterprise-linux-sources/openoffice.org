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


#include <string.h>
#include <rtl/memory.h>

void SAL_CALL rtl_zeroMemory(void *Ptr, sal_Size Bytes)
{
	memset(Ptr, 0, Bytes);
}

void SAL_CALL rtl_fillMemory(void *Ptr, sal_Size Bytes, sal_uInt8 Fill)
{
	memset(Ptr, Fill, Bytes);
}

void SAL_CALL rtl_copyMemory(void *Dst, const void *Src, sal_Size Bytes)
{
	memcpy(Dst, Src, Bytes);
}

void SAL_CALL rtl_moveMemory(void *Dst, const void *Src, sal_Size Bytes)
{
	memmove(Dst, Src, Bytes);
}

sal_Int32 SAL_CALL rtl_compareMemory(const void *MemA, const void *MemB, sal_Size Bytes)
{
	return memcmp(MemA, MemB, Bytes);
}

void* SAL_CALL rtl_findInMemory(const void *MemA, sal_uInt8 ch, sal_Size Bytes)
{
	return memchr(MemA, ch, Bytes);
}


