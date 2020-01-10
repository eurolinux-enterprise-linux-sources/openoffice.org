/*************************************************************************
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

#ifndef SVTOOLS_INC_TABLE_TABLETYPES_HXX
#define SVTOOLS_INC_TABLE_TABLETYPES_HXX

#include <sal/types.h>

//........................................................................
namespace svt { namespace table
{
//........................................................................
    /// a value denoting the size of a table
    typedef sal_Int32   TableSize;

    /// a value denoting a column position within a table
    typedef sal_Int32   ColPos;
    /// a value denoting a row position within a table
    typedef sal_Int32   RowPos;

    /** a value denoting an arbitrary coordinate value of a position within
        a table

        Values of this type are guaranteed to be large enough to hold column
        positions as well as row positions.
    */
    typedef sal_Int32   AnyPos;

    /// the ID of a column in a table
    typedef sal_Int32   ColumnID;

    typedef sal_Int32   TableMetrics;
/** special column width value which indicates that the column should be
    automatically resized to fit the view
*/
#define COLWIDTH_FIT_TO_VIEW    ((TableMetrics)-1)

/// denotes the column containing the row headers
#define COL_ROW_HEADERS         ((ColPos)-1)
/// denotes the row containing the column headers
#define ROW_COL_HEADERS         ((RowPos)-1)

/// denotes an invalid column index
#define COL_INVALID             ((ColPos)-2)
/// denotes an invalid row index
#define ROW_INVALID             ((RowPos)-2)


//........................................................................
} } // namespace svt::table
//........................................................................

#endif // SVTOOLS_INC_TABLE_TABLETYPES_HXX
