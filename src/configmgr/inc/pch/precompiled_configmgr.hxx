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

// MARKER(update_precomp.py): Generated on 2006-09-01 17:49:35.077252

#ifdef PRECOMPILED_HEADERS

//---MARKER---
#include "sal/config.h"
#include "sal/types.h"

#include "com/sun/star/beans/IllegalTypeException.hpp"
#include "com/sun/star/beans/NamedValue.hpp"
#include "com/sun/star/beans/Property.hpp"
#include "com/sun/star/beans/PropertyAttribute.hpp"
#include "com/sun/star/beans/PropertyExistException.hpp"
#include "com/sun/star/beans/PropertyValue.hpp"
#include "com/sun/star/beans/PropertyVetoException.hpp"
#include "com/sun/star/beans/UnknownPropertyException.hpp"
#include "com/sun/star/beans/XExactName.hpp"
#include "com/sun/star/beans/XHierarchicalPropertySet.hpp"
#include "com/sun/star/beans/XMultiHierarchicalPropertySet.hpp"
#include "com/sun/star/beans/XMultiPropertySet.hpp"
#include "com/sun/star/beans/XMultiPropertyStates.hpp"
#include "com/sun/star/beans/XPropertiesChangeListener.hpp"
#include "com/sun/star/beans/XProperty.hpp"
#include "com/sun/star/beans/XPropertyChangeListener.hpp"
#include "com/sun/star/beans/XPropertySet.hpp"
#include "com/sun/star/beans/XPropertySetInfo.hpp"
#include "com/sun/star/beans/XPropertyState.hpp"
#include "com/sun/star/beans/XPropertyWithState.hpp"
#include "com/sun/star/beans/XVetoableChangeListener.hpp"
#include "com/sun/star/configuration/CannotLoadConfigurationException.hpp"
#include "com/sun/star/configuration/InstallationIncompleteException.hpp"
#include "com/sun/star/configuration/InvalidBootstrapFileException.hpp"
#include "com/sun/star/configuration/MissingBootstrapFileException.hpp"
#include "com/sun/star/configuration/XTemplateContainer.hpp"
#include "com/sun/star/configuration/XTemplateInstance.hpp"
#include "com/sun/star/configuration/backend/AuthenticationFailedException.hpp"
#include "com/sun/star/configuration/backend/BackendAccessException.hpp"
#include "com/sun/star/configuration/backend/BackendSetupException.hpp"
#include "com/sun/star/configuration/backend/CannotConnectException.hpp"
#include "com/sun/star/configuration/backend/ComponentChangeEvent.hpp"
#include "com/sun/star/configuration/backend/ConnectionLostException.hpp"
#include "com/sun/star/configuration/backend/InsufficientAccessRightsException.hpp"
#include "com/sun/star/configuration/backend/InvalidAuthenticationMechanismException.hpp"
#include "com/sun/star/configuration/backend/MalformedDataException.hpp"
#include "com/sun/star/configuration/backend/MergeRecoveryRequest.hpp"
#include "com/sun/star/configuration/backend/NodeAttribute.hpp"
#include "com/sun/star/configuration/backend/PropertyInfo.hpp"
#include "com/sun/star/configuration/backend/SchemaAttribute.hpp"
#include "com/sun/star/configuration/backend/StratumCreationException.hpp"
#include "com/sun/star/configuration/backend/TemplateIdentifier.hpp"
#include "com/sun/star/configuration/backend/XBackend.hpp"
#include "com/sun/star/configuration/backend/XBackendChangesListener.hpp"
#include "com/sun/star/configuration/backend/XBackendChangesNotifier.hpp"
#include "com/sun/star/configuration/backend/XBackendEntities.hpp"
#include "com/sun/star/configuration/backend/XCompositeLayer.hpp"
#include "com/sun/star/configuration/backend/XLayer.hpp"
#include "com/sun/star/configuration/backend/XLayerContentDescriber.hpp"
#include "com/sun/star/configuration/backend/XLayerHandler.hpp"
#include "com/sun/star/configuration/backend/XLayerImporter.hpp"
#include "com/sun/star/configuration/backend/XMultiLayerStratum.hpp"
#include "com/sun/star/configuration/backend/XSchema.hpp"
#include "com/sun/star/configuration/backend/XSchemaHandler.hpp"
#include "com/sun/star/configuration/backend/XSchemaSupplier.hpp"
#include "com/sun/star/configuration/backend/XSingleLayerStratum.hpp"
#include "com/sun/star/configuration/backend/XUpdatableLayer.hpp"
#include "com/sun/star/configuration/backend/XUpdateHandler.hpp"
#include "com/sun/star/configuration/backend/XVersionedSchemaSupplier.hpp"
#include "com/sun/star/container/ElementExistException.hpp"
#include "com/sun/star/container/NoSuchElementException.hpp"
#include "com/sun/star/container/XChild.hpp"
#include "com/sun/star/container/XContainer.hpp"
#include "com/sun/star/container/XContainerListener.hpp"
#include "com/sun/star/container/XContentEnumerationAccess.hpp"
#include "com/sun/star/container/XEnumeration.hpp"
#include "com/sun/star/container/XHierarchicalName.hpp"
#include "com/sun/star/container/XHierarchicalNameAccess.hpp"
#include "com/sun/star/container/XNameAccess.hpp"
#include "com/sun/star/container/XNameContainer.hpp"
#include "com/sun/star/container/XNameReplace.hpp"
#include "com/sun/star/container/XNamed.hpp"
#include "com/sun/star/io/BufferSizeExceededException.hpp"
#include "com/sun/star/io/IOException.hpp"
#include "com/sun/star/io/UnexpectedEOFException.hpp"
#include "com/sun/star/io/WrongFormatException.hpp"
#include "com/sun/star/io/XActiveDataControl.hpp"
#include "com/sun/star/io/XActiveDataSink.hpp"
#include "com/sun/star/io/XActiveDataSource.hpp"
#include "com/sun/star/io/XDataExporter.hpp"
#include "com/sun/star/io/XDataImporter.hpp"
#include "com/sun/star/io/XDataInputStream.hpp"
#include "com/sun/star/io/XDataOutputStream.hpp"
#include "com/sun/star/io/XDataTransferEventListener.hpp"
#include "com/sun/star/io/XInputStream.hpp"
#include "com/sun/star/io/XOutputStream.hpp"
#include "com/sun/star/lang/DisposedException.hpp"
#include "com/sun/star/lang/IllegalAccessException.hpp"
#include "com/sun/star/lang/IllegalArgumentException.hpp"
#include "com/sun/star/lang/Locale.hpp"
#include "com/sun/star/lang/NoSupportException.hpp"
#include "com/sun/star/lang/NullPointerException.hpp"
#include "com/sun/star/lang/ServiceNotRegisteredException.hpp"
#include "com/sun/star/lang/WrappedTargetException.hpp"
#include "com/sun/star/lang/WrappedTargetRuntimeException.hpp"
#include "com/sun/star/lang/XComponent.hpp"
#include "com/sun/star/lang/XEventListener.hpp"
#include "com/sun/star/lang/XInitialization.hpp"
#include "com/sun/star/lang/XLocalizable.hpp"
#include "com/sun/star/lang/XMultiServiceFactory.hpp"
#include "com/sun/star/lang/XServiceInfo.hpp"
#include "com/sun/star/lang/XSingleComponentFactory.hpp"
#include "com/sun/star/lang/XSingleServiceFactory.hpp"
#include "com/sun/star/lang/XTypeProvider.hpp"
#include "com/sun/star/lang/XUnoTunnel.hpp"
#include "com/sun/star/registry/XRegistryKey.hpp"
#include "com/sun/star/registry/XSimpleRegistry.hpp"
#include "com/sun/star/script/FailReason.hpp"
#include "com/sun/star/script/XTypeConverter.hpp"
#include "com/sun/star/task/XInteractionAbort.hpp"
#include "com/sun/star/task/XInteractionApprove.hpp"
#include "com/sun/star/task/XInteractionDisapprove.hpp"
#include "com/sun/star/task/XInteractionHandler.hpp"
#include "com/sun/star/task/XInteractionRequest.hpp"
#include "com/sun/star/task/XInteractionRetry.hpp"
#include "com/sun/star/task/XJob.hpp"
#include "com/sun/star/uno/Any.h"
#include "com/sun/star/uno/Any.hxx"
#include "com/sun/star/uno/Exception.hpp"
#include "com/sun/star/uno/Reference.h"
#include "com/sun/star/uno/Reference.hxx"
#include "com/sun/star/uno/RuntimeException.hpp"
#include "com/sun/star/uno/Sequence.h"
#include "com/sun/star/uno/Sequence.hxx"
#include "com/sun/star/uno/Type.hxx"
#include "com/sun/star/uno/TypeClass.hpp"
#include "com/sun/star/uno/XComponentContext.hpp"
#include "com/sun/star/uno/XCurrentContext.hpp"
#include "com/sun/star/uno/XInterface.hpp"
#include "com/sun/star/util/XChangesBatch.hpp"
#include "com/sun/star/util/XChangesListener.hpp"
#include "com/sun/star/util/XChangesNotifier.hpp"
#include "com/sun/star/util/XFlushable.hpp"
#include "com/sun/star/util/XRefreshable.hpp"
#include "com/sun/star/util/XStringEscape.hpp"
#include "com/sun/star/util/XTimeStamped.hpp"
#include "com/sun/star/util/logging/LogLevel.hpp"
#include "com/sun/star/util/logging/XLogger.hpp"
#include "com/sun/star/xml/sax/InputSource.hpp"
#include "com/sun/star/xml/sax/SAXException.hpp"
#include "com/sun/star/xml/sax/SAXParseException.hpp"
#include "com/sun/star/xml/sax/XAttributeList.hpp"
#include "com/sun/star/xml/sax/XDocumentHandler.hpp"
#include "com/sun/star/xml/sax/XExtendedDocumentHandler.hpp"
#include "com/sun/star/xml/sax/XParser.hpp"

#include "comphelper/processfactory.hxx"
#include "comphelper/propertycontainer.hxx"
#include "comphelper/sequence.hxx"
#include "comphelper/stl_types.hxx"

#include "cppuhelper/bootstrap.hxx"
#include "cppuhelper/component_context.hxx"
#include "cppuhelper/exc_hlp.hxx"
#include "cppuhelper/factory.hxx"
#include "cppuhelper/implementationentry.hxx"
#include "cppuhelper/interfacecontainer.hxx"
#include "cppuhelper/propshlp.hxx"
#include "cppuhelper/queryinterface.hxx"
#include "cppuhelper/servicefactory.hxx"
#include "cppuhelper/typeprovider.hxx"
#include "cppuhelper/weak.hxx"

#include "osl/conditn.hxx"
#include "osl/diagnose.h"
#include "osl/file.h"
#include "osl/file.hxx"
#include "osl/interlck.h"
#include "osl/module.hxx"
#include "osl/mutex.hxx"
#include "osl/process.h"
#include "osl/thread.h"
#include "osl/time.h"

#include "rtl/alloc.h"
#include "rtl/bootstrap.h"
#include "rtl/bootstrap.hxx"
#include "rtl/logfile.hxx"
#include "rtl/process.h"
#include "rtl/ref.hxx"
#include "rtl/strbuf.hxx"
#include "rtl/string.h"
#include "rtl/string.hxx"
#include "rtl/ustrbuf.hxx"
#include "rtl/ustring.h"
#include "rtl/ustring.hxx"

#include "sal/main.h"

#include "salhelper/simplereferenceobject.hxx"

#include "sys/timeb.h"

#include "typelib/typedescription.hxx"

#include "uno/any2.h"
#include "uno/current_context.hxx"



#include "vos/conditn.hxx"
#include "vos/pipe.hxx"
#include "vos/ref.hxx"
#include "vos/refernce.hxx"
#include "vos/socket.hxx"
#include "vos/thread.hxx"
#include "vos/timer.hxx"
//---MARKER---

#endif
