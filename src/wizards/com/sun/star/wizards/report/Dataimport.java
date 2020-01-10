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
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.text.XTextDocument;

import com.sun.star.wizards.ui.*;
import com.sun.star.wizards.common.*;
import com.sun.star.uno.Exception;

public class Dataimport extends UnoDialog2 implements com.sun.star.awt.XActionListener
{ // extends ReportWizard

    // ReportTextDocument CurReportDocument;
    IReportDocument CurReportDocument;

//    PropertyValue[] CurProperties;
    static boolean bStopProcess;
    static String sProgressDBConnection;
    static String sProgressDataImport;
    static String sProgressBaseCurRecord;
    static String sProgressCurRecord;
    static String sProgressTitle;
    static String sStop;

//    public static final String TBLRECORDSECTION = ReportTextDocument.TBLRECORDSECTION;
//    public static final String TBLGROUPSECTION = ReportTextDocument.TBLGROUPSECTION;
//    public static final String RECORDSECTION = ReportTextDocument.RECORDSECTION;
//    public static final String GROUPSECTION = ReportTextDocument.GROUPSECTION;
//    public static final String COPYOFTBLRECORDSECTION = ReportTextDocument.COPYOFTBLRECORDSECTION;
//    public static final String COPYOFTBLGROUPSECTION = ReportTextDocument.COPYOFTBLGROUPSECTION;
//    public static final String COPYOFRECORDSECTION = ReportTextDocument.COPYOFRECORDSECTION;
//    public static final String COPYOFGROUPSECTION = ReportTextDocument.COPYOFGROUPSECTION;
    public Dataimport(XMultiServiceFactory _xMSF)
    {
        super(_xMSF);
        super.addResourceHandler("ReportWizard", "dbw");
        sProgressDBConnection = m_oResource.getResText(UIConsts.RID_DB_COMMON + 34);
        sProgressDataImport = m_oResource.getResText(UIConsts.RID_REPORT + 67);
        sProgressTitle = m_oResource.getResText(UIConsts.RID_REPORT + 62);
        sProgressBaseCurRecord = m_oResource.getResText(UIConsts.RID_REPORT + 63);
        sStop = m_oResource.getResText(UIConsts.RID_DB_COMMON + 21);

    }

    public void disposing(com.sun.star.lang.EventObject eventObject)
    {
    }

    public void actionPerformed(com.sun.star.awt.ActionEvent actionEvent)
    {
        // bStopProcess = true;
        CurReportDocument.StopProcess();
    }

//    public static void main(String args[]) 
//        {
//            String ConnectStr = "uno:socket,host=localhost,port=8100;urp,negotiate=0,forcesynchronous=1;StarOffice.NamingService";
//            XMultiServiceFactory xMSF = null;
//            try 
//            {
//                xMSF = com.sun.star.wizards.common.Desktop.connect(ConnectStr);
//                if (xMSF != null)
//                {
//                    System.out.println("Connected to " + ConnectStr);
//                }
//                PropertyValue[] curproperties = new PropertyValue[3];
//                curproperties[0] = Properties.createProperty("DatabaseLocation", "file:///C:/Documents and Settings/bc93774.EHAM02-DEV/My Documents/MyHSQL.odb");
////            curproperties[0] = Properties.createProperty("DatabaseLocation", "file:///C:/Documents and Settings/bc93774.EHAM02-DEV/My Documents/MyDocAssign.odb"); //baseLocation ); "DataSourceName", "db1");
////            curproperties[0] = Properties.createProperty("DataSourceName", "Bibliography");
//                curproperties[1] = Properties.createProperty("CommandType", new Integer(CommandType.TABLE));
//                curproperties[2] = Properties.createProperty("Command", "Table2");
//
//                Dataimport CurDataimport = new Dataimport(xMSF);
//                TextDocument oTextDocument = new TextDocument(xMSF, true, null);
//                CurDataimport.createReport(xMSF, oTextDocument.xTextDocument, curproperties);
//
//            }
//            catch (Exception e) 
//            {
//                e.printStackTrace(System.out);
//            }
//            catch (java.lang.Exception javaexception) 
//            {
//                javaexception.printStackTrace(System.out);
//            }
//        }
    public void showProgressDisplay(XMultiServiceFactory xMSF, boolean bgetConnection)
    {
        try
        {
            Helper.setUnoPropertyValues(xDialogModel,
                    new String[]
                    {
                        "Height", "Step", "Title", "Width"
                    },
                    new Object[]
                    {
                        new Integer(84), new Integer(0), sProgressTitle, new Integer(180)
                    });
            com.sun.star.awt.FontDescriptor oFontDesc = new com.sun.star.awt.FontDescriptor();
            oFontDesc.Weight = com.sun.star.awt.FontWeight.BOLD;
            if (bgetConnection)
            {
                insertControlModel("com.sun.star.awt.UnoControlFixedTextModel", "lblProgressDBConnection",
                        new String[]
                        {
                            "FontDescriptor", "Height", "Label", "PositionX", "PositionY", "Step", "Width"
                        },
                        new Object[]
                        {
                            oFontDesc, new Integer(10), sProgressDBConnection, new Integer(6), new Integer(6), new Integer(0), new Integer(150)
                        });

                insertControlModel("com.sun.star.awt.UnoControlFixedTextModel", "lblProgressDataImport",
                        new String[]
                        {
                            "Height", "Label", "PositionX", "PositionY", "Step", "Width"
                        },
                        new Object[]
                        {
                            new Integer(10), sProgressDataImport, new Integer(6), new Integer(24), new Integer(0), new Integer(120)
                        });
            }
            else
            {
                insertControlModel("com.sun.star.awt.UnoControlFixedTextModel", "lblProgressDataImport",
                        new String[]
                        {
                            "FontDescriptor", "Height", "Label", "PositionX", "PositionY", "Step", "Width"
                        },
                        new Object[]
                        {
                            oFontDesc, new Integer(10), sProgressDataImport, new Integer(6), new Integer(24), new Integer(0), new Integer(120)
                        });
            }
            insertControlModel("com.sun.star.awt.UnoControlFixedTextModel", "lblCurProgress",
                    new String[]
                    {
                        "Height", "Label", "PositionX", "PositionY", "Step", "Width"
                    },
                    new Object[]
                    {
                        new Integer(10), "", new Integer(12), new Integer(42), new Integer(0), new Integer(120)
                    });

            insertButton("cmdCancel", 10000, this,
                    new String[]
                    {
                        "Height", "HelpURL", "PositionX", "PositionY", "Step", "TabIndex", "Width", "Label"
                    },
                    new Object[]
                    {
                        new Integer(14), "HID:34321", new Integer(74), new Integer(58), new Integer(0), new Short((short) 1), new Integer(40), sStop
                    });
            createWindowPeer(CurReportDocument.getWizardParent());
            calculateDialogPosition(CurReportDocument.getFrame().getComponentWindow().getPosSize());
            xWindow.setVisible(true);
            super.xReschedule.reschedule();
            return;
        }
        catch (Exception exception)
        {
            exception.printStackTrace(System.out);
        // return;
        }
        catch (java.lang.Exception jexception)
        {
            jexception.printStackTrace(System.out);
        // return;
        }
    }

    // public void importReportData(final XMultiServiceFactory xMSF, final Dataimport CurDataimport, final ReportTextDocument CurReportDocument.getDoc()) {
    public void importReportData(final XMultiServiceFactory _xMSF, final Dataimport _CurDataimport, IReportDocument _CurReportDocument, PropertyValue[] _properties)
    {
        if (CurReportDocument.reconnectToDatabase(_xMSF, _properties))
        {
            // The following calls to remove the Sections must occur after the connection to the database
            modifyFontWeight("lblProgressDBConnection", com.sun.star.awt.FontWeight.NORMAL);
            modifyFontWeight("lblProgressDataImport", com.sun.star.awt.FontWeight.BOLD);
            CurReportDocument.insertDatabaseDatatoReportDocument(_xMSF);
        }
        xComponent.dispose();
        CurReportDocument.getRecordParser().dispose();
    }

    public void createReport(final XMultiServiceFactory xMSF, XTextDocument _textDocument, PropertyValue[] properties)
    {
        // CurReportDocument = new ReportTextDocument(xMSF, _textDocument,m_oResource);
        CurReportDocument = ReportTextImplementation.create(xMSF, _textDocument, m_oResource);
//            CurProperties = properties;
        showProgressDisplay(xMSF, true);
        importReportData(xMSF, this, CurReportDocument, properties);
    }
}
