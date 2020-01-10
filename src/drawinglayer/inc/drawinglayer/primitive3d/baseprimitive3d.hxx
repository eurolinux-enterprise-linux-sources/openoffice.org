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

#ifndef INCLUDED_DRAWINGLAYER_PRIMITIVE3D_BASEPRIMITIVE3D_HXX
#define INCLUDED_DRAWINGLAYER_PRIMITIVE3D_BASEPRIMITIVE3D_HXX

#include <cppuhelper/compbase1.hxx>
#include <boost/utility.hpp>
#include <com/sun/star/graphic/XPrimitive3D.hpp>
#include <comphelper/broadcasthelper.hxx>
#include <basegfx/range/b3drange.hxx>

//////////////////////////////////////////////////////////////////////////////
// defines for DeclPrimitrive3DIDBlock and ImplPrimitrive3DIDBlock
// Added to be able to simply change identification stuff later, e.g. add
// a identification string and/or ID to the interface and to the implementation
// ATM used to delclare implement getPrimitiveID()

#define DeclPrimitrive3DIDBlock() \
	virtual sal_uInt32 getPrimitiveID() const;

#define ImplPrimitrive3DIDBlock(TheClass, TheID) \
	sal_uInt32 TheClass::getPrimitiveID() const { return TheID; }

//////////////////////////////////////////////////////////////////////////////
// predefines

namespace drawinglayer { namespace geometry {
	class ViewInformation3D;
}}

//////////////////////////////////////////////////////////////////////////////
// basePrimitive3D class

namespace drawinglayer
{
	namespace primitive3d
	{
		// typedefs for basePrimitive3DImplBase, Primitive3DSequence and Primitive3DReference
		typedef cppu::WeakComponentImplHelper1< ::com::sun::star::graphic::XPrimitive3D > BasePrimitive3DImplBase;
		typedef ::com::sun::star::uno::Reference< ::com::sun::star::graphic::XPrimitive3D > Primitive3DReference;
		typedef ::com::sun::star::uno::Sequence< Primitive3DReference > Primitive3DSequence;

		class BasePrimitive3D
		:	private boost::noncopyable,
			protected comphelper::OBaseMutex,
			public BasePrimitive3DImplBase
		{
		private:
			// a sequence used for buffering the last createLocalDecomposition() result. Use
			// the protected access methods to change. Only implementations of getDecomposition()
			// should make use.
			Primitive3DSequence								maLocalDecomposition;

		protected:
			// access methods to maLocalDecomposition. The usage of this methods may allow
			// later thread-safe stuff to be added if needed. Only to be used by getDecomposition()
			// implementations for buffering the last decomposition.
			const Primitive3DSequence& getLocalDecomposition() const { return maLocalDecomposition; }
			void setLocalDecomposition(const Primitive3DSequence& rNew) { maLocalDecomposition = rNew; }

			// method which is to be used to implement the local decomposition of a 2D primitive. The default
			// implementation will just return an empty decomposition
			virtual Primitive3DSequence createLocalDecomposition(const geometry::ViewInformation3D& rViewInformation) const;

		public:
			// constructor
			BasePrimitive3D();

			// the ==operator is mainly needed to allow testing newly-created high level primitives against their last
			// incarnation which buffers/holds the decompositionsThe default implementation
			// uses getPrimitiveID()-calls to test if it's the same ID at last. Overloaded implementation are then
			// based on this implementation
			virtual bool operator==( const BasePrimitive3D& rPrimitive ) const;
			bool operator!=( const BasePrimitive3D& rPrimitive ) const { return !operator==(rPrimitive); }

			// This method is for places where using the C++ implementation directly is possible. The subprocessing
			// and range merging is more efficient when working directly on basegfx::B3DRange. The default implementation
			// will use getDecomposition results to create the range
			virtual basegfx::B3DRange getB3DRange(const geometry::ViewInformation3D& rViewInformation) const;

			// provide unique ID for fast identifying of known primitive implementations in renderers. These use
			// the the defines from primitivetypes3d.hxx to define unique IDs.
			virtual sal_uInt32 getPrimitiveID() const = 0;

			// The getDecomposition default implementation will on demand use createLocalDecomposition() if maLocalDecomposition is empty.
			// It will set maLocalDecomposition to this obtained decomposition to buffer it.
			// If the decomposition is also ViewInformation-dependent, this method needs to be overloaded and the 
			// ViewInformation for the last decomposition needs to be remembered, too, and be used in the next call to decide if
			// the buffered decomposition may be reused or not.
			virtual Primitive3DSequence get3DDecomposition(const geometry::ViewInformation3D& rViewInformation) const;

			//
			// Methods from XPrimitive3D
			//

			// The getDecomposition implementation for UNO API will use getDecomposition from this implementation. It
			// will get the ViewInformation from the ViewParameters for that purpose
			virtual Primitive3DSequence SAL_CALL getDecomposition( const ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue >& rViewParameters ) throw ( ::com::sun::star::uno::RuntimeException );

			// the getRange default implemenation will use getDecomposition to create the range information from merging
			// getRange results from the single local decomposition primitives.
			virtual ::com::sun::star::geometry::RealRectangle3D SAL_CALL getRange( const ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue >& rViewParameters ) throw ( ::com::sun::star::uno::RuntimeException );
		};
	} // end of namespace primitive3d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////
// tooling

namespace drawinglayer
{
	namespace primitive3d
	{
		// get B3DRange from a given Primitive3DReference
		basegfx::B3DRange getB3DRangeFromPrimitive3DReference(const Primitive3DReference& rCandidate, const geometry::ViewInformation3D& aViewInformation);

		// get range3D from a given Primitive3DSequence
		basegfx::B3DRange getB3DRangeFromPrimitive3DSequence(const Primitive3DSequence& rCandidate, const geometry::ViewInformation3D& aViewInformation);

		// compare two Primitive2DReferences for equality, including trying to get implementations (BasePrimitive2D)
		// and using compare operator
		bool arePrimitive3DReferencesEqual(const Primitive3DReference& rA, const Primitive3DReference& rB);

		// compare two Primitive3DReferences for equality, uses arePrimitive3DReferencesEqual internally
		bool arePrimitive3DSequencesEqual(const Primitive3DSequence& rA, const Primitive3DSequence& rB);

		// concatenate sequence
		void appendPrimitive3DSequenceToPrimitive3DSequence(Primitive3DSequence& rDest, const Primitive3DSequence& rSource);

		// concatenate single Primitive3D
		void appendPrimitive3DReferenceToPrimitive3DSequence(Primitive3DSequence& rDest, const Primitive3DReference& rSource);

	} // end of namespace primitive3d
} // end of namespace drawinglayer

//////////////////////////////////////////////////////////////////////////////

#endif //INCLUDED_DRAWINGLAYER_PRIMITIVE3D_BASEPRIMITIVE3D_HXX

//////////////////////////////////////////////////////////////////////////////
// eof
