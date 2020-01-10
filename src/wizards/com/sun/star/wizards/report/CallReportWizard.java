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
package com.sun.star.wizards.report;

import com.sun.star.beans.PropertyValue;
import com.sun.star.beans.PropertyAttribute;
import com.sun.star.uno.Type;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.text.XTextDocument;
import com.sun.star.wizards.common.Properties;
import com.sun.star.lang.XComponent;

/** This class capsulates the class, that implements the minimal component, a
 * factory for creating the service (<CODE>__getServiceFactory</CODE>) and a
 * method, that writes the information into the given registry key
 * (<CODE>__writeRegistryServiceInfo</CODE>).
 * @author Bertram Nolte
 */
public class CallReportWizard
{

    static boolean bWizardstartedalready;

    /** Gives a factory for creating the service.
     * This method is called by the <code>JavaLoader</code>
     * <p>
     * @return Returns a <code>XSingleServiceFactory</code> for creating the
     * component.
     * @see com.sun.star.comp.loader.JavaLoader#
     * @param stringImplementationName The implementation name of the component.
     * @param xMSF The service manager, who gives access to every
     * known service.
     * @param xregistrykey Makes structural information (except regarding tree
     * structures) of a single
     * registry key accessible.
     */
    public static com.sun.star.lang.XSingleServiceFactory __getServiceFactory(String stringImplementationName, com.sun.star.lang.XMultiServiceFactory xMSF, com.sun.star.registry.XRegistryKey xregistrykey)
    {
        com.sun.star.lang.XSingleServiceFactory xsingleservicefactory = null;
        if (stringImplementationName.equals(
                ReportWizardImplementation.class.getName()))
        {
            xsingleservicefactory = com.sun.star.comp.loader.FactoryHelper.getServiceFactory(
                    ReportWizardImplementation.class,
                    ReportWizardImplementation.__serviceName,
                    xMSF,
                    xregistrykey);
        }
        return xsingleservicefactory;
    }

    /** Writes the service information into the given registry key.
     * This method is called by the <code>JavaLoader</code>.
     * @return returns true if the operation succeeded
     * @see com.sun.star.comp.loader.JavaLoader#
     * @param xregistrykey Makes structural information (except regarding tree
     * structures) of a single
     * registry key accessible.
     */
    public static boolean __writeRegistryServiceInfo(com.sun.star.registry.XRegistryKey xregistrykey)
    {
        return com.sun.star.comp.loader.FactoryHelper.writeRegistryServiceInfo(
                ReportWizardImplementation.class.getName(),
                ReportWizardImplementation.__serviceName,
                xregistrykey);
    }

    /** This class implements the component. At least the interfaces XServiceInfo,
     * XTypeProvider, and XInitialization should be provided by the service.
     */
    public static class ReportWizardImplementation extends com.sun.star.lib.uno.helper.PropertySet implements com.sun.star.lang.XInitialization, com.sun.star.lang.XServiceInfo, com.sun.star.lang.XTypeProvider, com.sun.star.task.XJobExecutor
    {

        PropertyValue[] databaseproperties;
        public XComponent DocumentDefinition = null;
        public XComponent Document = null;

        /** The constructor of the inner class has a XMultiServiceFactory parameter.
         * @param xmultiservicefactoryInitialization A special service factory
         * could be introduced while initializing.
         */
        public ReportWizardImplementation(com.sun.star.lang.XMultiServiceFactory xmultiservicefactoryInitialization)
        {
            super();
            xmultiservicefactory = xmultiservicefactoryInitialization;
            registerProperty("Document", (short) (PropertyAttribute.READONLY | PropertyAttribute.MAYBEVOID));
            registerProperty("DocumentDefinition", (short) (PropertyAttribute.READONLY | PropertyAttribute.MAYBEVOID));
        }

        public void trigger(String sEvent)
        {
            try
            {
                com.sun.star.frame.XComponentLoader xcomponentloader = (com.sun.star.frame.XComponentLoader) com.sun.star.uno.UnoRuntime.queryInterface(com.sun.star.frame.XComponentLoader.class, xmultiservicefactory.createInstance("com.sun.star.frame.Desktop"));
                if (sEvent.compareTo("start") == 0)
                {
                    if (bWizardstartedalready != true)
                    {
                        ReportWizard CurReportWizard = new ReportWizard(xmultiservicefactory);
                        XComponent[] obj = CurReportWizard.startReportWizard(xmultiservicefactory, databaseproperties);
                        if (obj != null)
                        {
                            DocumentDefinition = obj[0];
                            if (obj.length > 1)
                            {
                                Document = obj[1];
                            }
                            else
                            {
                                Document = null;
                            }
                        }
                    }
                    bWizardstartedalready = false;
                }
                else if (sEvent.compareTo("end") == 0)
                {
                    DocumentDefinition = null;
                    Document = null;
                    databaseproperties = null;
                }
                else if (sEvent.compareTo("fill") == 0)
                {
                    Dataimport CurDataimport = new Dataimport(xmultiservicefactory);
                    XTextDocument xTextDocument = null;
                    if (databaseproperties != null)
                    {
                        for (int i = 0; i < databaseproperties.length; ++i)
                        {
                            if (databaseproperties[i].Name.equals("TextDocument"))
                            {
                                xTextDocument = (XTextDocument) UnoRuntime.queryInterface(XTextDocument.class, databaseproperties[i].Value);
                            }

                        }
                        if (xTextDocument != null)
                        {
                            CurDataimport.createReport(xmultiservicefactory, xTextDocument, databaseproperties);
                        }
                    }
                }
            }
            catch (Exception exception)
            {
                System.err.println(exception);
            }
            System.gc();
        }
        /** The service name, that must be used to get an instance of this service.
         */
        private static final String __serviceName = "com.sun.star.wizards.report.CallReportWizard";
        /** The service manager, that gives access to all registered services.
         */
        private com.sun.star.lang.XMultiServiceFactory xmultiservicefactory;

        /** This method is a member of the interface for initializing an object
         * directly after its creation.
         * @param object This array of arbitrary objects will be passed to the
         * component after its creation.
         * @throws com.sun.star.uno.Exception Every exception will not be handled, but will be
         * passed to the caller.
         */
        public void initialize(Object[] object) throws com.sun.star.uno.Exception
        {
            this.databaseproperties = Properties.convertToPropertyValueArray(object);

        //    xmultiservicefactory = (XMultiservicefactory) UnoRuntime.queryInterface(XMultiServiceFactory.class, object[0]);
        }

        /** This method returns an array of all supported service names.
         * @return Array of supported service names.
         */
        public java.lang.String[] getSupportedServiceNames()
        {
            String[] stringSupportedServiceNames = new String[1];

            stringSupportedServiceNames[ 0] = __serviceName;

            return (stringSupportedServiceNames);
        }

        /** This method returns true, if the given service will be
         * supported by the component.
         * @param stringService Service name.
         * @return True, if the given service name will be supported.
         */
        public boolean supportsService(String stringService)
        {
            boolean booleanSupportsService = false;

            if (stringService.equals(__serviceName))
            {
                booleanSupportsService = true;
            }
            return (booleanSupportsService);
        }

        /** This method returns an array of bytes, that can be used to
         * unambiguously distinguish between two sets of types, e.g.
         * to realise hashing functionality when the object is introspected.
         * Two objects that return the same ID also have to return
         * the same set of types in getTypes(). If an unique
         * implementation Id cannot be provided this method has to
         * return an empty sequence. Important: If the object
         * aggregates other objects the ID has to be unique for the
         * whole combination of objects.
         * @return Array of bytes, in order to distinguish between two sets.
         */
        public byte[] getImplementationId()
        {
            byte[] byteReturn =
            {
            };

            try
            {
                byteReturn = new String("" + this.hashCode()).getBytes();
            }
            catch (Exception exception)
            {
                System.err.println(exception);
            }

            return (byteReturn);
        }

        /** Return the class name of the component.
         * @return Class name of the component.
         */
        public java.lang.String getImplementationName()
        {
            return (ReportWizardImplementation.class.getName());
        }

        /** Provides a sequence of all types (usually interface types)
         * provided by the object.
         * @return Sequence of all types (usually interface types) provided by the
         * service.
         */
        public Type[] getTypes()
        {
            Type[] typeReturn =
            {
            };

            try
            {
                typeReturn = new Type[]
                        {
                            new Type(com.sun.star.task.XJobExecutor.class),
                            new Type(com.sun.star.lang.XTypeProvider.class),
                            new Type(com.sun.star.lang.XServiceInfo.class),
                            new Type(com.sun.star.lang.XInitialization.class)
                        };
            }
            catch (Exception exception)
            {
                System.err.println(exception);
            }

            return (typeReturn);
        }
    }
}

