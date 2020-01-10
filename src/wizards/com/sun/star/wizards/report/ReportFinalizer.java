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
//import com.sun.star.ucb.CommandAbortedException;
import com.sun.star.ucb.XSimpleFileAccess;
import com.sun.star.uno.Exception;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;
import com.sun.star.wizards.common.Desktop;
import com.sun.star.wizards.common.*;
import com.sun.star.wizards.ui.*;
import com.sun.star.awt.VclWindowPeerAttribute;
import com.sun.star.awt.XTextComponent;
//import com.sun.star.container.XHierarchicalNameAccess;
//import com.sun.star.container.XNameAccess;
//import com.sun.star.lang.EventObject;
//import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.wizards.db.RecordParser;
//import com.sun.star.wizards.document.OfficeDocument;
public class ReportFinalizer
{

    WizardDialog CurUnoDialog;
    XTextComponent xTitleTextBox;
    XTextComponent[] xSaveTextBox = new XTextComponent[2];
    Object chkTemplate;
    String CHANGEREPORTTITLE_FUNCNAME = "changeReportTitle";
    String TOGGLESUBTEMPLATECONTROLS_FUNCNAME = "toggleSubTemplateControls";
//    String slblHowProceed;
//    String slblChooseReportKind;
    String TemplatePath;
    String StoreName;
    boolean bfinalaskbeforeOverwrite;
    String DefaultName;
    String OldDefaultName;
    // ReportTextDocument CurReportDocument;
    IReportDocument CurReportDocument;
    // Desktop.OfficePathRetriever curofficepath;
//    short curtabindex;
//    String sMsgReportDocumentNameDuplicate;
    public static final int SOCREATEDOCUMENT = 1;
    public static final int SOCREATETEMPLATE = 2;
    public static final int SOUSETEMPLATE = 3;
    private XMultiServiceFactory m_xMSF;
    // public Finalizer(ReportTextDocument _CurReportDocument, WizardDialog _CurUnoDialog) {
    public ReportFinalizer(XMultiServiceFactory _xMSF, IReportDocument _CurReportDocument, WizardDialog _CurUnoDialog)
    {
        m_xMSF = _xMSF;

        this.CurUnoDialog = _CurUnoDialog;
        this.CurReportDocument = _CurReportDocument;
        short curtabindex = (short) (ReportWizard.SOSTOREPAGE * 100);
        Desktop odesktop = new Desktop();
        // curofficepath = odesktop.new OfficePathRetriever(m_xMSF);

        String sSaveAsTemplate = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 40);
        String sUseTemplate = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 41);
        String sEditTemplate = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 42);
        String sSaveAsDocument = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 43);
// String            sSaveAs = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 44);
        String sReportTitle = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 33);
        String slblHowProceed = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 78);
        String slblChooseReportKind = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 79);

        CurUnoDialog.insertControlModel("com.sun.star.awt.UnoControlFixedTextModel", "lblTitle",
                new String[]
                {
                    "Height", "Label", "PositionX", "PositionY", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    new Integer(8), sReportTitle, new Integer(95), new Integer(27), new Integer(ReportWizard.SOSTOREPAGE), new Short(curtabindex++), new Integer(68)
                });

        xTitleTextBox = CurUnoDialog.insertTextField("txtTitle", CHANGEREPORTTITLE_FUNCNAME, this,
                new String[]
                {
                    "Height", "HelpURL", "PositionX", "PositionY", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    new Integer(12), "HID:34362", new Integer(95), new Integer(37), new Integer(ReportWizard.SOSTOREPAGE), new Short(curtabindex++), new Integer(209)
                });

        CurUnoDialog.insertControlModel("com.sun.star.awt.UnoControlFixedTextModel", "lblChooseReportKind",
                new String[]
                {
                    "Height", "Label", "PositionX", "PositionY", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    new Integer(8), slblChooseReportKind, new Integer(95), new Integer(57), new Integer(ReportWizard.SOSTOREPAGE), new Short(curtabindex++), new Integer(209)
                });

        CurUnoDialog.insertRadioButton("optCreateDocument", TOGGLESUBTEMPLATECONTROLS_FUNCNAME, this,
                new String[]
                {
                    "Height", "HelpURL", "Label", "PositionX", "PositionY", "State", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    new Integer(10), "HID:34371", sSaveAsDocument, new Integer(95), new Integer(69), new Short((short) 0), new Integer(ReportWizard.SOSTOREPAGE), new Short(curtabindex++), new Integer(138)
                });

        CurUnoDialog.insertRadioButton("optCreateReportTemplate", TOGGLESUBTEMPLATECONTROLS_FUNCNAME, this,
                new String[]
                {
                    "Height", "HelpURL", "Label", "PositionX", "PositionY", "State", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    new Integer(8), "HID:34370", sSaveAsTemplate, new Integer(95), new Integer(81), new Short((short) 1), new Integer(ReportWizard.SOSTOREPAGE), new Short(curtabindex++), new Integer(209)
                });


        CurUnoDialog.insertControlModel("com.sun.star.awt.UnoControlFixedTextModel", "lblHowProceed",
                new String[]
                {
                    "Height", "Label", "PositionX", "PositionY", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    new Integer(8), slblHowProceed, new Integer(105), new Integer(93), new Integer(ReportWizard.SOSTOREPAGE), new Short(curtabindex++), new Integer(209)
                });


        CurUnoDialog.insertRadioButton("optEditTemplate", TOGGLESUBTEMPLATECONTROLS_FUNCNAME, this,
                new String[]
                {
                    "Height", "HelpURL", "Label", "PositionX", "PositionY", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    new Integer(10), "HID:34374", sEditTemplate, new Integer(111), new Integer(105), new Integer(6), new Short(curtabindex++), new Integer(138)
                });

        CurUnoDialog.insertRadioButton("optUseTemplate", TOGGLESUBTEMPLATECONTROLS_FUNCNAME, this,
                new String[]
                {
                    "Height", "HelpURL", "Label", "PositionX", "PositionY", "State", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    new Integer(10), "HID:34375", sUseTemplate, new Integer(111), new Integer(115), new Short((short) 1), new Integer(ReportWizard.SOSTOREPAGE), new Short(curtabindex++), new Integer(138)
                });
    }


    /*
     * This function is called if one of the radio buttons is pressed
     */
    public void toggleSubTemplateControls()
    {
        // String sStorePath = "";
        Short iState = (Short) CurUnoDialog.getControlProperty("optCreateReportTemplate", "State");
        boolean bDoTemplateEnable = iState.shortValue() == 1;
        CurUnoDialog.setControlProperty("optEditTemplate", "Enabled", new Boolean(bDoTemplateEnable));
        CurUnoDialog.setControlProperty("optUseTemplate", "Enabled", new Boolean(bDoTemplateEnable));
        CurUnoDialog.setControlProperty("lblHowProceed", "Enabled", new Boolean(bDoTemplateEnable));

        String sTitle = xTitleTextBox.getText();
        boolean bDoEnable = sTitle.equals("");
        CurUnoDialog.enableFinishButton(!bDoEnable);
    }
//  private boolean fileexists(XMultiServiceFactory _xMSF, String _spath){
//  try {
//      XInterface xUcbInterface = (XInterface) _xMSF.createInstance("com.sun.star.ucb.SimpleFileAccess");
//      XSimpleFileAccess xSimpleFileAccess = (XSimpleFileAccess) com.sun.star.uno.UnoRuntime.queryInterface(XSimpleFileAccess.class, xUcbInterface);
//      return xSimpleFileAccess.exists(_spath);
//  } catch (Exception exception) {
//      exception.printStackTrace(System.out);
//      return false;
//  }}
    public void initialize(RecordParser _CurDBMetaData)
    {
        String FirstCommandName = (_CurDBMetaData.getIncludedCommandNames())[0];
        DefaultName = Desktop.getUniqueName(_CurDBMetaData.getReportDocuments(), FirstCommandName);
        if (DefaultName.equals(OldDefaultName) == false)
        {
            OldDefaultName = DefaultName;
        }
        xTitleTextBox.setText(DefaultName);
    }

    public String getStoreName()
    {
        if (CurUnoDialog != null)
        {
            String LocStoreName = xTitleTextBox.getText();
            if (!LocStoreName.equals(""))
            {
                StoreName = LocStoreName;
            }
        }
        return (StoreName);
    }

    public String getStorePath()
    {
        try
        {
            StoreName = getStoreName();
            String StorePath;
            XInterface xInterface = (XInterface) m_xMSF.createInstance("com.sun.star.ucb.SimpleFileAccess");
            XSimpleFileAccess xSimpleFileAccess = (XSimpleFileAccess) UnoRuntime.queryInterface(XSimpleFileAccess.class, xInterface);
            StorePath = FileAccess.getOfficePath(m_xMSF, "Temp", xSimpleFileAccess) + "/" + StoreName;
            return StorePath;
        }
        catch (Exception e)
        {
            e.printStackTrace(System.out);
            return "";
        }
    }

    public void changeReportTitle()
    {
        String TitleName = xTitleTextBox.getText();
        CurReportDocument.liveupdate_updateReportTitle(TitleName);
    }

    public int getReportOpenMode()
    {
        int ReportMode = SOCREATEDOCUMENT;
        boolean bcreateTemplate = ((Short) CurUnoDialog.getControlProperty("optCreateReportTemplate", "State")).shortValue() == (short) 1;
        if (bcreateTemplate)
        {
            ReportMode = SOCREATETEMPLATE;
        }
        boolean buseTemplate = ((Short) CurUnoDialog.getControlProperty("optUseTemplate", "State")).shortValue() == (short) 1;
        if (buseTemplate)
        {
            ReportMode = SOUSETEMPLATE;
        }
        boolean buseDocument = ((Short) CurUnoDialog.getControlProperty("optCreateDocument", "State")).shortValue() == (short) 1;
        if (buseDocument)
        {
            ReportMode = SOCREATEDOCUMENT;
        }
        return ReportMode;
    }

    public boolean finish()
    {
        StoreName = getStoreName();
        if (CurReportDocument.getRecordParser().getReportDocuments().hasByHierarchicalName(StoreName))
        {
            String sMsgReportDocumentNameDuplicate = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 76);
            String sShowMsgReportNameisDuplicate = JavaTools.replaceSubString(sMsgReportDocumentNameDuplicate, StoreName, "%REPORTNAME");
            /* int iMsg = */ CurUnoDialog.showMessageBox("ErrorBox", VclWindowPeerAttribute.OK, sShowMsgReportNameisDuplicate);
            return false;
        }
        else
        {
            CurReportDocument.store(StoreName, getReportOpenMode());
            ReportWizard.bCloseDocument = false;
            return true;
        }
    }
}
