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
#include "precompiled_chart2.hxx"

#include "CommandDispatchContainer.hxx"
#include "UndoCommandDispatch.hxx"
#include "StatusBarCommandDispatch.hxx"
#include "DisposeHelper.hxx"
#include "macros.hxx"

#include <comphelper/InlineContainer.hxx>

#include <com/sun/star/frame/XDispatchProvider.hpp>

using namespace ::com::sun::star;

using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::rtl::OUString;

namespace chart
{

CommandDispatchContainer::CommandDispatchContainer(
    const Reference< uno::XComponentContext > & xContext ) :
        m_xContext( xContext )
{
    m_aContainerDocumentCommands =
        ::comphelper::MakeSet< OUString >
        ( C2U("AddDirect"))    ( C2U("NewDoc"))             ( C2U("Open"))
        ( C2U("Save"))         ( C2U("SaveAs"))             ( C2U("SendMail"))
        ( C2U("EditDoc"))      ( C2U("ExportDirectToPDF"))  ( C2U("PrintDefault"))
        ;
}

void CommandDispatchContainer::setModel(
    const Reference< frame::XModel > & xModel )
{
    // remove all existing dispatcher that base on the old model
    m_aCachedDispatches.clear();
    DisposeHelper::DisposeAllElements( m_aToBeDisposedDispatches );    
    m_aToBeDisposedDispatches.clear();
    m_xModel.set( xModel );
}

// void CommandDispatchContainer::setUndoManager(
//     const Reference< chart2::XUndoManager > & xUndoManager )
// {
//     m_xUndoManager = xUndoManager;
// }

void CommandDispatchContainer::setFallbackDispatch(
    const Reference< frame::XDispatch > xFallbackDispatch,
    const ::std::set< OUString > & rFallbackCommands )
{
    OSL_ENSURE(xFallbackDispatch.is(),"Invalid fall back dispatcher!");
    m_xFallbackDispatcher.set( xFallbackDispatch );
    m_aFallbackCommands = rFallbackCommands;
    m_aToBeDisposedDispatches.push_back( m_xFallbackDispatcher );
}

Reference< frame::XDispatch > CommandDispatchContainer::getDispatchForURL(
    const util::URL & rURL )
{
    Reference< frame::XDispatch > xResult;

    tDispatchMap::const_iterator aIt( m_aCachedDispatches.find( rURL.Complete ));
    if( aIt != m_aCachedDispatches.end())
    {
        xResult.set( (*aIt).second );
    }
    else
    {
        if( rURL.Path.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "Undo" ))
            || rURL.Path.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "Redo" )))
        {
            CommandDispatch * pDispatch = new UndoCommandDispatch( m_xContext, m_xModel );
            xResult.set( pDispatch );
            pDispatch->initialize();
            m_aCachedDispatches[ C2U(".uno:Undo") ].set( xResult );
            m_aCachedDispatches[ C2U(".uno:Redo") ].set( xResult );
            m_aToBeDisposedDispatches.push_back( xResult );
        }
        else if( rURL.Path.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "Context" ))
                 || rURL.Path.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM( "ModifiedStatus" )))
        {
            Reference< view::XSelectionSupplier > xSelSupp;
            if( m_xModel.is())
                xSelSupp.set( m_xModel->getCurrentController(), uno::UNO_QUERY );
            CommandDispatch * pDispatch = new StatusBarCommandDispatch( m_xContext, m_xModel, xSelSupp );
            xResult.set( pDispatch );
            pDispatch->initialize();
            m_aCachedDispatches[ C2U(".uno:Context") ].set( xResult );
            m_aCachedDispatches[ C2U(".uno:ModifiedStatus") ].set( xResult );
            m_aToBeDisposedDispatches.push_back( xResult );
        }
        else if( m_xModel.is() &&
                 (m_aContainerDocumentCommands.find( rURL.Path ) != m_aContainerDocumentCommands.end()) )
        {
            xResult.set( getContainerDispatchForURL( m_xModel->getCurrentController(), rURL ));
            // ToDo: can those dispatches be cached?
            m_aCachedDispatches[ rURL.Complete ].set( xResult );
        }
        else if( m_xFallbackDispatcher.is() &&
                 (m_aFallbackCommands.find( rURL.Path ) != m_aFallbackCommands.end()) )
        {
            xResult.set( m_xFallbackDispatcher );
            m_aCachedDispatches[ rURL.Complete ].set( xResult );
        }
    }

    return xResult;
}

Sequence< Reference< frame::XDispatch > > CommandDispatchContainer::getDispatchesForURLs(
    const Sequence< frame::DispatchDescriptor > & aDescriptors )
{
	sal_Int32 nCount = aDescriptors.getLength();
	uno::Sequence< uno::Reference< frame::XDispatch > > aRet( nCount );

	for( sal_Int32 nPos = 0; nPos < nCount; ++nPos )
	{
        if( aDescriptors[ nPos ].FrameName.equalsAsciiL( RTL_CONSTASCII_STRINGPARAM("_self")))
            aRet[ nPos ] = getDispatchForURL( aDescriptors[ nPos ].FeatureURL );
	}
	return aRet;
}

void CommandDispatchContainer::DisposeAndClear()
{
    m_aCachedDispatches.clear();
    DisposeHelper::DisposeAllElements( m_aToBeDisposedDispatches );    
    m_aToBeDisposedDispatches.clear();
    m_xFallbackDispatcher.clear();
    m_aFallbackCommands.clear();
}

Reference< frame::XDispatch > CommandDispatchContainer::getContainerDispatchForURL(
    const Reference< frame::XController > & xChartController,
    const util::URL & rURL )
{
    Reference< frame::XDispatch > xResult;
    if( xChartController.is())
    {
        Reference< frame::XFrame > xFrame( xChartController->getFrame());
        if( xFrame.is())
        {
            Reference< frame::XDispatchProvider > xDispProv( xFrame->getCreator(), uno::UNO_QUERY );
            if( xDispProv.is())
                xResult.set( xDispProv->queryDispatch( rURL, C2U("_self"), 0 ));
        }
    }
    return xResult;
}

} //  namespace chart
