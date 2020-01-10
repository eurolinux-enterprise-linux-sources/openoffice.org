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
#ifndef CHART2_CHARTTRANSFERABLE_HXX
#define CHART2_CHARTTRANSFERABLE_HXX

#include <svtools/transfer.hxx>

namespace com { namespace sun { namespace star {
namespace graphic {
    class XGraphic;
}
}}}

class SdrModel;
class SdrObject;

namespace chart
{

class ChartTransferable : public TransferableHelper
{
public:
    explicit ChartTransferable( SdrModel * pDrawModel, SdrObject * pSelectedObj );
    virtual ~ChartTransferable();

protected:

    // implementation of TransferableHelper methods
    virtual void        AddSupportedFormats();
    virtual sal_Bool    GetData( const ::com::sun::star::datatransfer::DataFlavor& rFlavor );

private:
    ::com::sun::star::uno::Reference< ::com::sun::star::graphic::XGraphic > m_xMetaFileGraphic;
};

} //  namespace chart

// CHART2_CHARTTRANSFERABLE_HXX
#endif
