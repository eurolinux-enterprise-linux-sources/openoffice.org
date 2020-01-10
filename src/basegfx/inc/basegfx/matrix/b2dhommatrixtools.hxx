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

#ifndef _BGFX_MATRIX_B2DHOMMATRIXTOOLS_HXX
#define _BGFX_MATRIX_B2DHOMMATRIXTOOLS_HXX

#include <sal/types.h>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/vector/b2dvector.hxx>

///////////////////////////////////////////////////////////////////////////////

namespace basegfx
{
    class DecomposedB2DHomMatrixContainer
    {
    private:
        B2DHomMatrix           maB2DHomMatrix;
        B2DVector              maScale;
        B2DVector              maTranslate;
        double                 mfRotate;
        double                 mfShearX;

        // bitfield
        unsigned               mbDecomposed : 1;

        void impCheckDecompose()
        {
            if(!mbDecomposed)
            {
                maB2DHomMatrix.decompose(maScale, maTranslate, mfRotate, mfShearX);
                mbDecomposed = true;
            }
        }

    public:
        DecomposedB2DHomMatrixContainer(const B2DHomMatrix& rB2DHomMatrix)
        :   maB2DHomMatrix(rB2DHomMatrix),
            maScale(),
            maTranslate(),
            mfRotate(0.0),
            mfShearX(0.0),
            mbDecomposed(false)
        {
        }

        // data access
        const B2DHomMatrix& getB2DHomMatrix() const { return maB2DHomMatrix; }
        const B2DVector& getScale() const { const_cast< DecomposedB2DHomMatrixContainer* >(this)->impCheckDecompose(); return maScale; }
        const B2DVector& getTranslate() const { const_cast< DecomposedB2DHomMatrixContainer* >(this)->impCheckDecompose(); return maTranslate; }
        double getRotate() const { const_cast< DecomposedB2DHomMatrixContainer* >(this)->impCheckDecompose(); return mfRotate; }
        double getShearX() const { const_cast< DecomposedB2DHomMatrixContainer* >(this)->impCheckDecompose(); return mfShearX; }
    };
} // end of namespace basegfx

///////////////////////////////////////////////////////////////////////////////

#endif /* _BGFX_MATRIX_B2DHOMMATRIXTOOLS_HXX */
