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
#include "precompiled_toolkit.hxx"
#include <com/sun/star/awt/tree/XMutableTreeDataModel.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <cppuhelper/implbase2.hxx>
#include <cppuhelper/implbase3.hxx>
#include <rtl/ref.hxx>
#include <toolkit/helper/mutexandbroadcasthelper.hxx>
#include <toolkit/helper/servicenames.hxx>

using ::rtl::OUString;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::awt::tree;
using namespace ::com::sun::star::lang;

namespace toolkit
{

	enum broadcast_type { nodes_changed, nodes_inserted, nodes_removed, structure_changed };

class MutableTreeNode;
class MutableTreeDataModel;

typedef rtl::Reference< MutableTreeNode > MutableTreeNodeRef;
typedef std::vector< MutableTreeNodeRef > TreeNodeVector;
typedef rtl::Reference< MutableTreeDataModel > MutableTreeDataModelRef;

static void implThrowIllegalArgumentException() throw( IllegalArgumentException )
{
	throw IllegalArgumentException();
}

class MutableTreeDataModel : public ::cppu::WeakAggImplHelper2<	XMutableTreeDataModel, XServiceInfo >,
							 public MutexAndBroadcastHelper
{
public:
	MutableTreeDataModel();
	virtual ~MutableTreeDataModel();

	void broadcast( broadcast_type eType, const Reference< XTreeNode >& xParentNode, const Reference< XTreeNode >* pNodes, sal_Int32 nNodes );

	// XMutableTreeDataModel
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::awt::tree::XMutableTreeNode > SAL_CALL createNode( const ::com::sun::star::uno::Any& DisplayValue, ::sal_Bool ChildsOnDemand ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setRoot( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::tree::XMutableTreeNode >& RootNode ) throw (::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::uno::RuntimeException);

    // XTreeDataModel
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::awt::tree::XTreeNode > SAL_CALL getRoot(  ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL addTreeDataModelListener( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::tree::XTreeDataModelListener >& Listener ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL removeTreeDataModelListener( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::tree::XTreeDataModelListener >& Listener ) throw (::com::sun::star::uno::RuntimeException);

    // XComponent
    virtual void SAL_CALL dispose(  ) throw (RuntimeException);
    virtual void SAL_CALL addEventListener( const Reference< XEventListener >& xListener ) throw (RuntimeException);
    virtual void SAL_CALL removeEventListener( const Reference< XEventListener >& aListener ) throw (RuntimeException);

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName(  ) throw (RuntimeException);
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) throw (RuntimeException);
    virtual Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) throw (RuntimeException);

private:
	bool mbDisposed;
	Reference< XTreeNode > mxRootNode;
};

class MutableTreeNode: public ::cppu::WeakAggImplHelper2< XMutableTreeNode, XServiceInfo >
{
	friend class MutableTreeDataModel;

public:
	MutableTreeNode( const MutableTreeDataModelRef& xModel, const Any& rValue, sal_Bool bChildsOnDemand );
	virtual ~MutableTreeNode();

	void setParent( MutableTreeNode* pParent );
	void broadcast_changes();
	void broadcast_changes(const Reference< XTreeNode >& xNode, bool bNew);

	// XMutableTreeNode
    virtual ::com::sun::star::uno::Any SAL_CALL getDataValue() throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setDataValue( const ::com::sun::star::uno::Any& _datavalue ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL appendChild( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::tree::XMutableTreeNode >& ChildNode ) throw (::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL insertChildByIndex( ::sal_Int32 Index, const ::com::sun::star::uno::Reference< ::com::sun::star::awt::tree::XMutableTreeNode >& ChildNode ) throw (::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL removeChildByIndex( ::sal_Int32 Index ) throw (::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setHasChildrenOnDemand( ::sal_Bool ChildrenOnDemand ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setDisplayValue( const ::com::sun::star::uno::Any& Value ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setNodeGraphicURL( const ::rtl::OUString& URL ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setExpandedGraphicURL( const ::rtl::OUString& URL ) throw (::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setCollapsedGraphicURL( const ::rtl::OUString& URL ) throw (::com::sun::star::uno::RuntimeException);

    // XTreeNode
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::awt::tree::XTreeNode > SAL_CALL getChildAt( ::sal_Int32 Index ) throw (::com::sun::star::lang::IndexOutOfBoundsException, ::com::sun::star::uno::RuntimeException);
    virtual ::sal_Int32 SAL_CALL getChildCount(  ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::awt::tree::XTreeNode > SAL_CALL getParent(  ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::sal_Int32 SAL_CALL getIndex( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::tree::XTreeNode >& Node ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::sal_Bool SAL_CALL hasChildrenOnDemand(  ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Any SAL_CALL getDisplayValue(  ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::rtl::OUString SAL_CALL getNodeGraphicURL(  ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::rtl::OUString SAL_CALL getExpandedGraphicURL(  ) throw (::com::sun::star::uno::RuntimeException);
    virtual ::rtl::OUString SAL_CALL getCollapsedGraphicURL(  ) throw (::com::sun::star::uno::RuntimeException);

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName(  ) throw (RuntimeException);
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) throw (RuntimeException);
    virtual Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) throw (RuntimeException);

	static MutableTreeNode* getImplementation( const Reference< XTreeNode >& xNode, bool bThrows ) throw (IllegalArgumentException);
	Reference< XTreeNode > getReference( MutableTreeNode* pNode )
	{
		return Reference< XTreeNode >( pNode );
	}

private:
	TreeNodeVector	maChilds;
	Any maDisplayValue;
	Any maDataValue;
	sal_Bool mbHasChildsOnDemand;
	::osl::Mutex maMutex;
	MutableTreeNode* mpParent;
	MutableTreeDataModelRef mxModel;
	OUString maNodeGraphicURL;
	OUString maExpandedGraphicURL;
	OUString maCollapsedGraphicURL;
	bool mbIsInserted;
};

///////////////////////////////////////////////////////////////////////
// class MutableTreeDataModel
///////////////////////////////////////////////////////////////////////

MutableTreeDataModel::MutableTreeDataModel()
: mbDisposed( false )
{
}

//---------------------------------------------------------------------

MutableTreeDataModel::~MutableTreeDataModel()
{
}

//---------------------------------------------------------------------

void MutableTreeDataModel::broadcast( broadcast_type eType, const Reference< XTreeNode >& xParentNode, const Reference< XTreeNode >* pNodes, sal_Int32 nNodes )
{
	::cppu::OInterfaceContainerHelper* pIter = BrdcstHelper.getContainer( XTreeDataModelListener::static_type() );
	if( pIter )
	{
		Reference< XInterface > xSource( static_cast< ::cppu::OWeakObject* >( this ) );
		const Sequence< Reference< XTreeNode > > aNodes( pNodes, nNodes );
		TreeDataModelEvent aEvent( xSource, aNodes, xParentNode );

		::cppu::OInterfaceIteratorHelper aListIter(*pIter);
		while(aListIter.hasMoreElements())
		{
			XTreeDataModelListener* pListener = static_cast<XTreeDataModelListener*>(aListIter.next());
			switch( eType )
			{
			case nodes_changed:		pListener->treeNodesChanged(aEvent); break;
			case nodes_inserted:	pListener->treeNodesInserted(aEvent); break;
			case nodes_removed:		pListener->treeNodesRemoved(aEvent); break;
			case structure_changed:	pListener->treeStructureChanged(aEvent); break;
			}
		}
	}
}

//---------------------------------------------------------------------
// XMutableTreeDataModel
//---------------------------------------------------------------------

Reference< XMutableTreeNode > SAL_CALL MutableTreeDataModel::createNode( const Any& aValue, sal_Bool bChildsOnDemand ) throw (RuntimeException)
{
	return new MutableTreeNode( this, aValue, bChildsOnDemand );
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeDataModel::setRoot( const Reference< XMutableTreeNode >& xNode ) throw (IllegalArgumentException, RuntimeException)
{
	if( !xNode.is() )
		throw IllegalArgumentException();

	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	if( xNode != mxRootNode )
	{
		if( mxRootNode.is() )
		{
			MutableTreeNodeRef xOldImpl( dynamic_cast< MutableTreeNode* >( mxRootNode.get() ) );
			if( xOldImpl.is() )
				xOldImpl->mbIsInserted = false;
		}

		MutableTreeNodeRef xImpl( dynamic_cast< MutableTreeNode* >( xNode.get() ) );
		if( !xImpl.is() || xImpl->mbIsInserted )
			throw IllegalArgumentException();
		
		xImpl->mbIsInserted = true;
		mxRootNode.set(xImpl.get());

		Reference< XTreeNode > xParentNode;
		broadcast( structure_changed, xParentNode, &mxRootNode, 1 );
	}
}

//---------------------------------------------------------------------
// XTreeDataModel
//---------------------------------------------------------------------

Reference< XTreeNode > SAL_CALL MutableTreeDataModel::getRoot(  ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	return mxRootNode;
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeDataModel::addTreeDataModelListener( const Reference< XTreeDataModelListener >& xListener ) throw (RuntimeException)
{
	BrdcstHelper.addListener( XTreeDataModelListener::static_type(), xListener );
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeDataModel::removeTreeDataModelListener( const Reference< XTreeDataModelListener >& xListener ) throw (RuntimeException)
{
	BrdcstHelper.removeListener( XTreeDataModelListener::static_type(), xListener );
}

//---------------------------------------------------------------------
// XComponent
//---------------------------------------------------------------------

void SAL_CALL MutableTreeDataModel::dispose() throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );

	if( !mbDisposed )
	{
		mbDisposed = true;
		::com::sun::star::lang::EventObject aEvent;
		aEvent.Source.set( static_cast< ::cppu::OWeakObject* >( this ) );
		BrdcstHelper.aLC.disposeAndClear( aEvent );
	}
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeDataModel::addEventListener( const Reference< XEventListener >& xListener ) throw (RuntimeException)
{
	BrdcstHelper.addListener( XEventListener::static_type(), xListener );
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeDataModel::removeEventListener( const Reference< XEventListener >& xListener ) throw (RuntimeException)
{
	BrdcstHelper.removeListener( XEventListener::static_type(), xListener );
}

//---------------------------------------------------------------------
// XServiceInfo
//---------------------------------------------------------------------

OUString SAL_CALL MutableTreeDataModel::getImplementationName(  ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	static const OUString aImplName( RTL_CONSTASCII_USTRINGPARAM( "toolkit.MutableTreeDataModel" ) );
	return aImplName;
}

//---------------------------------------------------------------------

sal_Bool SAL_CALL MutableTreeDataModel::supportsService( const OUString& ServiceName ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	return ServiceName.equalsAscii( szServiceName_MutableTreeDataModel );
}

//---------------------------------------------------------------------

Sequence< OUString > SAL_CALL MutableTreeDataModel::getSupportedServiceNames(  ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	static const OUString aServiceName( OUString::createFromAscii( szServiceName_MutableTreeDataModel ) );
	static const Sequence< OUString > aSeq( &aServiceName, 1 );
	return aSeq;
}

///////////////////////////////////////////////////////////////////////
// class MutabelTreeNode
///////////////////////////////////////////////////////////////////////

MutableTreeNode::MutableTreeNode( const MutableTreeDataModelRef& xModel, const Any& rValue, sal_Bool bChildsOnDemand )
: maDisplayValue( rValue )
, mbHasChildsOnDemand( bChildsOnDemand )
, mpParent( 0 )
, mxModel( xModel )
, mbIsInserted( false )
{
}

//---------------------------------------------------------------------

MutableTreeNode::~MutableTreeNode()
{
	TreeNodeVector::iterator aIter( maChilds.begin() );
	while( aIter != maChilds.end() )
		(*aIter++)->setParent(0);
}

//---------------------------------------------------------------------

void MutableTreeNode::setParent( MutableTreeNode* pParent )
{
	mpParent = pParent;
}

//---------------------------------------------------------------------

MutableTreeNode* MutableTreeNode::getImplementation( const Reference< XTreeNode >& xNode, bool bThrows ) throw (IllegalArgumentException)
{
	MutableTreeNode* pImpl = dynamic_cast< MutableTreeNode* >( xNode.get() );
	if( bThrows && !pImpl )
		implThrowIllegalArgumentException();

	return pImpl;
}

//---------------------------------------------------------------------

void MutableTreeNode::broadcast_changes()
{
	if( mxModel.is() )
	{
		Reference< XTreeNode > xParent( getReference( mpParent ) );
		Reference< XTreeNode > xNode( getReference( this ) );
		mxModel->broadcast( nodes_changed, xParent, &xNode, 1 );
	}
}

//---------------------------------------------------------------------

void MutableTreeNode::broadcast_changes(const Reference< XTreeNode >& xNode, bool bNew)
{
	if( mxModel.is() )
	{
		Reference< XTreeNode > xParent( getReference( this ) );
		mxModel->broadcast( bNew ? nodes_inserted : nodes_removed, xParent, &xNode, 1 );
	}
}

//---------------------------------------------------------------------
// XMutableTreeNode
//---------------------------------------------------------------------

Any SAL_CALL MutableTreeNode::getDataValue() throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	return maDataValue;
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeNode::setDataValue( const Any& _datavalue ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	maDataValue = _datavalue;
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeNode::appendChild( const Reference< XMutableTreeNode >& xChildNode ) throw (IllegalArgumentException, RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	Reference< XTreeNode > xNode( xChildNode.get() );
	MutableTreeNodeRef xImpl( dynamic_cast< MutableTreeNode* >( xNode.get() ) );

	if( !xImpl.is() || xImpl->mbIsInserted || (this == xImpl.get()) )
		throw IllegalArgumentException();

	maChilds.push_back( xImpl );
	xImpl->setParent(this);
	xImpl->mbIsInserted = true;

	broadcast_changes( xNode, true );
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeNode::insertChildByIndex( sal_Int32 nChildIndex, const Reference< XMutableTreeNode >& xChildNode ) throw (IllegalArgumentException, IndexOutOfBoundsException, RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );

	if( (nChildIndex < 0) || (nChildIndex > (sal_Int32)maChilds.size()) )
		throw IndexOutOfBoundsException();

	Reference< XTreeNode > xNode( xChildNode.get() );
	MutableTreeNodeRef xImpl( dynamic_cast< MutableTreeNode* >( xNode.get() ) );
	if( !xImpl.is() || xImpl->mbIsInserted || (this == xImpl.get()) )
		throw IllegalArgumentException();

	xImpl->mbIsInserted = true;

	TreeNodeVector::iterator aIter( maChilds.begin() );
	while( (nChildIndex-- > 0) && (aIter != maChilds.end()) )
		aIter++;

	maChilds.insert( aIter, xImpl );
	xImpl->setParent( this );

	broadcast_changes( xNode, true );
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeNode::removeChildByIndex( sal_Int32 nChildIndex ) throw (IndexOutOfBoundsException, RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	
	MutableTreeNodeRef xImpl;

	if( (nChildIndex >= 0) && (nChildIndex < (sal_Int32)maChilds.size()) )
	{
		TreeNodeVector::iterator aIter( maChilds.begin() );
		while( nChildIndex-- && (aIter != maChilds.end()) )
			aIter++;

		if( aIter != maChilds.end() )
		{
			xImpl = (*aIter);
			maChilds.erase( aIter );
		}
	}

	if( !xImpl.is() )
		throw IndexOutOfBoundsException();

	xImpl->setParent(0);
	xImpl->mbIsInserted = false;

	broadcast_changes( getReference( xImpl.get() ), false );
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeNode::setHasChildrenOnDemand( sal_Bool bChildsOnDemand ) throw (RuntimeException)
{
	bool bChanged;

	{
		::osl::Guard< ::osl::Mutex > aGuard( maMutex );
		bChanged = mbHasChildsOnDemand != bChildsOnDemand;
		mbHasChildsOnDemand = bChildsOnDemand;
	}

	if( bChanged )
		broadcast_changes();
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeNode::setDisplayValue( const Any& aValue ) throw (RuntimeException)
{
	{
		::osl::Guard< ::osl::Mutex > aGuard( maMutex );
		maDisplayValue = aValue;
	}

	broadcast_changes();
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeNode::setNodeGraphicURL( const OUString& rURL ) throw (RuntimeException)
{
	bool bChanged;

	{
		::osl::Guard< ::osl::Mutex > aGuard( maMutex );
		bChanged = maNodeGraphicURL != rURL;
		maNodeGraphicURL = rURL;
	}

	if( bChanged )
		broadcast_changes();
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeNode::setExpandedGraphicURL( const OUString& rURL ) throw (RuntimeException)
{
	bool bChanged;

	{
		::osl::Guard< ::osl::Mutex > aGuard( maMutex );
		bChanged = maExpandedGraphicURL != rURL;
		maExpandedGraphicURL = rURL;
	}

	if( bChanged )
		broadcast_changes();
}

//---------------------------------------------------------------------

void SAL_CALL MutableTreeNode::setCollapsedGraphicURL( const OUString& rURL ) throw (RuntimeException)
{
	bool bChanged;

	{
		::osl::Guard< ::osl::Mutex > aGuard( maMutex );
		bChanged = maCollapsedGraphicURL != rURL;
		maCollapsedGraphicURL = rURL;
	}

	if( bChanged )
		broadcast_changes();
}

//---------------------------------------------------------------------
// XTreeNode
//---------------------------------------------------------------------

Reference< XTreeNode > SAL_CALL MutableTreeNode::getChildAt( sal_Int32 nChildIndex ) throw (IndexOutOfBoundsException,RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );

	if( (nChildIndex < 0) || (nChildIndex >= (sal_Int32)maChilds.size()) )
		throw IndexOutOfBoundsException();
	return getReference( maChilds[nChildIndex].get() );
}

//---------------------------------------------------------------------

sal_Int32 SAL_CALL MutableTreeNode::getChildCount(  ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	return (sal_Int32)maChilds.size();
}

//---------------------------------------------------------------------

Reference< XTreeNode > SAL_CALL MutableTreeNode::getParent(  ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	return getReference( mpParent );
}

//---------------------------------------------------------------------

sal_Int32 SAL_CALL MutableTreeNode::getIndex( const Reference< XTreeNode >& xNode ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );

	MutableTreeNodeRef xImpl( MutableTreeNode::getImplementation( xNode, false ) );
	if( xImpl.is() )
	{
		sal_Int32 nChildCount = maChilds.size();
		while( nChildCount-- )
		{
			if( maChilds[nChildCount] == xImpl )
				return nChildCount;
		}
	}

	return -1;
}

//---------------------------------------------------------------------

sal_Bool SAL_CALL MutableTreeNode::hasChildrenOnDemand(  ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	return mbHasChildsOnDemand;
}

//---------------------------------------------------------------------

Any SAL_CALL MutableTreeNode::getDisplayValue(  ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	return maDisplayValue;
}

//---------------------------------------------------------------------

OUString SAL_CALL MutableTreeNode::getNodeGraphicURL(  ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	return maNodeGraphicURL;
}

//---------------------------------------------------------------------

OUString SAL_CALL MutableTreeNode::getExpandedGraphicURL(  ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	return maExpandedGraphicURL;
}

//---------------------------------------------------------------------

OUString SAL_CALL MutableTreeNode::getCollapsedGraphicURL(  ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	return maCollapsedGraphicURL;
}

//---------------------------------------------------------------------
// XServiceInfo
//---------------------------------------------------------------------

OUString SAL_CALL MutableTreeNode::getImplementationName(  ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	static const OUString aImplName( RTL_CONSTASCII_USTRINGPARAM( "toolkit.MutableTreeNode" ) );
	return aImplName;
}

//---------------------------------------------------------------------

sal_Bool SAL_CALL MutableTreeNode::supportsService( const OUString& ServiceName ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	return ServiceName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "com.sun.star.awt.tree.MutableTreeNode" ) );
}

//---------------------------------------------------------------------

Sequence< OUString > SAL_CALL MutableTreeNode::getSupportedServiceNames(  ) throw (RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( maMutex );
	static const OUString aServiceName( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.awt.tree.MutableTreeNode" ) );
	static const Sequence< OUString > aSeq( &aServiceName, 1 );
	return aSeq;
}

}

Reference< XInterface > SAL_CALL MutableTreeDataModel_CreateInstance( const Reference< XMultiServiceFactory >& )
{
	return Reference < XInterface >( ( ::cppu::OWeakObject* ) new ::toolkit::MutableTreeDataModel );
}
