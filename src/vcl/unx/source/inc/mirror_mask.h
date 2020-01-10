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
#define mirror_mask_width 32
#define mirror_mask_height 32
static char mirror_mask_bits[] = {
   0x00, 0x00, 0x04, 0x00, 0xfe, 0x0f, 0x0e, 0x00, 0xfe, 0x0f, 0x1f, 0x00,
   0xfe, 0x8f, 0x3f, 0x00, 0xfe, 0x0f, 0x1f, 0x00, 0xfe, 0x8f, 0x0f, 0x00,
   0xbe, 0x8f, 0x0f, 0x00, 0xbe, 0xcf, 0x07, 0x00, 0xbe, 0xcf, 0x87, 0x00,
   0xbe, 0xef, 0xc3, 0x01, 0xbe, 0xef, 0xe3, 0x03, 0xfe, 0xff, 0xf1, 0x07,
   0xfe, 0xff, 0x79, 0x0f, 0xfe, 0xff, 0x3c, 0x1e, 0xfe, 0xff, 0x1e, 0x3c,
   0xfe, 0x7f, 0x0f, 0x1e, 0x00, 0xfc, 0x07, 0x0f, 0x00, 0xfe, 0x83, 0x07,
   0x00, 0xbe, 0xc7, 0x03, 0x00, 0x1f, 0xef, 0x01, 0x00, 0x1f, 0xfe, 0x00,
   0x80, 0x0f, 0x7c, 0x00, 0xc0, 0x1d, 0x38, 0x00, 0x80, 0x0f, 0x10, 0x00,
   0x00, 0x07, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
