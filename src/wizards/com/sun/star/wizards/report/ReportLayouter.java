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

import com.sun.star.awt.XListBox;
// import com.sun.star.awt.XTextComponent;
import com.sun.star.lang.EventObject;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.wizards.common.*;
import com.sun.star.wizards.ui.*;

public class ReportLayouter
{

    UnoDialog CurUnoDialog;
//    static String sOrientationHeader;
//    static String sOrientVertical;
//    static String sOrientHorizontal;
    final int SOTXTTITLE = 28;
    final int SOCONTENTLST = 29;
    final static public int SOOPTLANDSCAPE = 30;
    final static public int SOOPTPORTRAIT = 31;
    final int SOLAYOUTLST = 32;
//    static String sReportTitle;
//    static String slblDataStructure;
//    static String slblPageLayout;
//    static String sOrganizeFields;
    XListBox xContentListBox;
    XListBox xLayoutListBox;
    int iOldContentPos;
    int iOldLayoutPos;
    // ReportTextDocument CurReportDocument;
    IReportDocument CurReportDocument;
    public String[][] LayoutFiles;
    public String[][] ContentFiles;
    // private Desktop.OfficePathRetriever curofficepath;
    Object aOrientationImage;
    // boolean m_bLandscape = true;
    private XMultiServiceFactory m_xMSF;

    public ReportLayouter(XMultiServiceFactory _xMSF, IReportDocument _CurReportDocument, UnoDialog _CurUnoDialog)
    {
        m_xMSF = _xMSF;
        try
        {
            short curtabindex = (short) (100 * ReportWizard.SOTEMPLATEPAGE);
            this.CurUnoDialog = _CurUnoDialog;
            this.CurReportDocument = _CurReportDocument;
            //TODO the constructor for the OfficePathRetriever is redundant and should be instantiated elsewhere
            // Desktop odesktop = new Desktop();
            // curofficepath = odesktop.new OfficePathRetriever(m_xMSF);
            String slblDataStructure = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 15);
            String slblPageLayout = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 16);
// String                sOrganizeFields = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 19);

            String sOrientationHeader = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 22);
            String sOrientVertical = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 23);
            String sOrientHorizontal = CurUnoDialog.m_oResource.getResText(UIConsts.RID_REPORT + 24);

            // XInterface xUcbInterface = (XInterface) m_xMSF.createInstance("com.sun.star.ucb.SimpleFileAccess");
            // XSimpleFileAccess xSimpleFileAccess = (XSimpleFileAccess) com.sun.star.uno.UnoRuntime.queryInterface(XSimpleFileAccess.class, xUcbInterface);
            // boolean bcntexists = xSimpleFileAccess.exists(CurReportDocument.getReportPath() + "/cnt-default.ott");
            // boolean bstlexists = xSimpleFileAccess.exists(CurReportDocument.getReportPath() + "/stl-default.ott");
            //      if ((bcntexists == false) || (bstlexists == false))
            //          throw  new NoValidPathException(CurReportDocument.xMSF);


            CurUnoDialog.insertControlModel("com.sun.star.awt.UnoControlFixedTextModel", "lblContent",
                    new String[]
                    {
                        "Height", "Label", "PositionX", "PositionY", "Step", "TabIndex", "Width"
                    },
                    new Object[]
                    {
                        new Integer(8), slblDataStructure, new Integer(95), new Integer(27), new Integer(ReportWizard.SOTEMPLATEPAGE), new Short(curtabindex++), new Integer(99)
                    });

            short iSelPos = 0;
//                    ContentFiles = FileAccess.getFolderTitles(m_xMSF, "cnt", CurReportDocument.getReportPath());
            ContentFiles = CurReportDocument.getDataLayout();
            iSelPos = (short) JavaTools.FieldInList(ContentFiles[1], CurReportDocument.getContentPath());
            if (iSelPos < 0)
            {
                iSelPos = 0;
            }
            iOldContentPos = (int) iSelPos;
            xContentListBox = CurUnoDialog.insertListBox("lstContent", SOCONTENTLST, new ActionListenerImpl(), new ItemListenerImpl(),
                    new String[]
                    {
                        "Height", "HelpURL", "PositionX", "PositionY", "SelectedItems", "Step", "StringItemList", "TabIndex", "Width"
                    },
                    new Object[]
                    {
                        new Integer(108), "HID:34363", new Integer(95), new Integer(37), new short[]
                        {
                            iSelPos
                        }, new Integer(ReportWizard.SOTEMPLATEPAGE), ContentFiles[0], new Short(curtabindex++), new Integer(99)
                    });

            CurUnoDialog.insertControlModel("com.sun.star.awt.UnoControlFixedTextModel", "lblLayout",
                    new String[]
                    {
                        "Height", "Label", "PositionX", "PositionY", "Step", "TabIndex", "Width"
                    },
                    new Object[]
                    {
                        new Integer(8), slblPageLayout, new Integer(205), new Integer(27), new Integer(ReportWizard.SOTEMPLATEPAGE), new Short(curtabindex++), new Integer(99)
                    });

            short iSelLayoutPos = 0;
//                    LayoutFiles = FileAccess.getFolderTitles(m_xMSF, "stl", CurReportDocument.getReportPath());
            LayoutFiles = CurReportDocument.getHeaderLayout();
            iSelLayoutPos = (short) JavaTools.FieldInList(LayoutFiles[1], CurReportDocument.getLayoutPath());
            if (iSelLayoutPos < 0)
            {
                iSelLayoutPos = (short) JavaTools.FieldInList(LayoutFiles[0], CurReportDocument.getLayoutPath());
                if (iSelLayoutPos < 0)
                {
                    iSelLayoutPos = 0;
                }
            }
            xLayoutListBox = CurUnoDialog.insertListBox("lstLayout", SOLAYOUTLST, new ActionListenerImpl(), new ItemListenerImpl(),
                    new String[]
                    {
                        "Height", "HelpURL", "PositionX", "PositionY", "SelectedItems", "Step", "StringItemList", "TabIndex", "Width"
                    },
                    new Object[]
                    {
                        new Integer(108), "HID:34364", new Integer(205), new Integer(37), new short[]
                        {
                            iSelLayoutPos
                        }, new Integer(ReportWizard.SOTEMPLATEPAGE), LayoutFiles[0], new Short(curtabindex++), new Integer(99)
                    });
            iOldLayoutPos = (int) iSelPos;
            CurUnoDialog.insertControlModel("com.sun.star.awt.UnoControlFixedTextModel", "lblOrientation",
                    new String[]
                    {
                        "Height", "Label", "PositionX", "PositionY", "Step", "TabIndex", "Width"
                    },
                    new Object[]
                    {
                        new Integer(8), sOrientationHeader, new Integer(95), new Integer(148), new Integer(ReportWizard.SOTEMPLATEPAGE), new Short(curtabindex++), new Integer(74)
                    });

            short m_nLandscapeState = CurReportDocument.getDefaultPageOrientation() == SOOPTLANDSCAPE ? (short) 1 : 0;
            CurUnoDialog.insertRadioButton("optLandscape", SOOPTLANDSCAPE, new ItemListenerImpl(),
                    new String[]
                    {
                        "Height", "HelpURL", "Label", "PositionX", "PositionY", "State", "Step", "TabIndex", "Width"
                    },
                    new Object[]
                    {
                        new Integer(10), "HID:34365", sOrientHorizontal, new Integer(101), new Integer(158), new Short(m_nLandscapeState), new Integer(ReportWizard.SOTEMPLATEPAGE), new Short(curtabindex++), new Integer(60)
                    });

            short m_nPortraitState = CurReportDocument.getDefaultPageOrientation() == SOOPTPORTRAIT ? (short) 1 : (short) 0;
            CurUnoDialog.insertRadioButton("optPortrait", SOOPTPORTRAIT, new ItemListenerImpl(),
                    new String[]
                    {
                        "Height", "HelpURL", "Label", "PositionX", "PositionY", "State", "Step", "TabIndex", "Width"
                    },
                    new Object[]
                    {
                        new Integer(10), "HID:34366", sOrientVertical, new Integer(101), new Integer(171), new Short(m_nPortraitState), new Integer(ReportWizard.SOTEMPLATEPAGE), new Short(curtabindex++), new Integer(60)
                    });

            aOrientationImage = CurUnoDialog.insertControlModel("com.sun.star.awt.UnoControlImageControlModel", "imgOrientation",
                    new String[]
                    {
                        "Border", "Height", "PositionX", "PositionY", "ScaleImage", "Step", "Width"
                    },
                    new Object[]
                    {
                        new Short("0"), new Integer(23), new Integer(164), new Integer(158), new Boolean(false), new Integer(ReportWizard.SOTEMPLATEPAGE), new Integer(30)
                    });

            String sNote = ReportWizard.getBlindTextNote(CurReportDocument, CurUnoDialog.m_oResource);
            CurUnoDialog.insertControlModel("com.sun.star.awt.UnoControlFixedTextModel", "lblBlindTextNote_2",
                    new String[]
                    {
                        "Height", "Label", "MultiLine", "PositionX", "PositionY", "Step", "Width"
                    },
                    new Object[]
                    {
                        new Integer(34), sNote, new Boolean(true), new Integer(205), new Integer(148), new Integer(ReportWizard.SOTEMPLATEPAGE), new Integer(99)
                    });
            if (m_nLandscapeState == 1)
            {
                CurUnoDialog.getPeerConfiguration().setImageUrl(aOrientationImage, 1002, 1003);
            }
            else
            {
                CurUnoDialog.getPeerConfiguration().setImageUrl(aOrientationImage, 1000, 1001);
            }
        }
        catch (Exception exception)
        {
            exception.printStackTrace(System.out);
        }
    }

    public void initialize(String _defaultTemplatePath)
    {
//             CurReportDocument.getDoc().xTextDocument.lockControllers();
        CurReportDocument.layout_setupRecordSection(_defaultTemplatePath);
        if (CurUnoDialog.getControlProperty("txtTitle", "Text").equals(""))
        {
            String[] sCommandNames = CurReportDocument.getRecordParser().getIncludedCommandNames();
            CurUnoDialog.setControlProperty("txtTitle", "Text", sCommandNames[0]);
        }
        // CurReportDocument.getDoc().oViewHandler.selectFirstPage(CurReportDocument.getDoc().oTextTableHandler);
        CurReportDocument.layout_selectFirstPage();
        CurUnoDialog.setFocus("lblContent");
//            CurReportDocument.getDoc().unlockallControllers();
    }

    class ItemListenerImpl implements com.sun.star.awt.XItemListener
    {

        public void itemStateChanged(com.sun.star.awt.ItemEvent EventObject)
        {
            try
            {
                int iPos;
                Helper.setUnoPropertyValue(CurUnoDialog.xDialogModel, "Enabled", new Boolean(false));
                // LLA: should we lock controllers here?
                //                    CurReportDocument.getDoc().xTextDocument.lockControllers();
                int iKey = CurUnoDialog.getControlKey(EventObject.Source, CurUnoDialog.ControlList);
                switch (iKey)
                {
                    case SOCONTENTLST:
                        iPos = xContentListBox.getSelectedItemPos();
                        if (iPos != iOldContentPos)
                        {
                            iOldContentPos = iPos;
                            CurReportDocument.liveupdate_changeContentTemplate(ContentFiles[1][iPos]);
                        }
                        break;

                    case SOLAYOUTLST:
                        iPos = xLayoutListBox.getSelectedItemPos();
                        if (iPos != iOldLayoutPos)
                        {
                            iOldLayoutPos = iPos;
                            CurReportDocument.liveupdate_changeLayoutTemplate(LayoutFiles[1][iPos]/*, Desktop.getBitmapPath(m_xMSF)*/);
                        }
                        break;

                    case SOOPTLANDSCAPE:
                        // CurReportDocument.getDoc().changePageOrientation(true);
                        try
                        {
                            CurReportDocument.setPageOrientation(SOOPTLANDSCAPE);
                        }
                        catch (com.sun.star.lang.IllegalArgumentException e)
                        {
                        }
                        Helper.setUnoPropertyValue(aOrientationImage, "ImageURL", CurUnoDialog.getWizardImageUrl(1002, 1003));

                        break;

                    case SOOPTPORTRAIT:
                        // CurReportDocument.getDoc().changePageOrientation( false);
                        try
                        {
                            CurReportDocument.setPageOrientation(SOOPTPORTRAIT);
                        }
                        catch (com.sun.star.lang.IllegalArgumentException e)
                        {
                        }

                        CurUnoDialog.setControlProperty("imgOrientation", "ImageURL", Desktop.getBitmapPath(m_xMSF) + "/portrait.gif");
                        Helper.setUnoPropertyValue(aOrientationImage, "ImageURL", CurUnoDialog.getWizardImageUrl(1000, 1001));
                        break;

                    default:
                        break;
                }
            }
            catch (RuntimeException e)
            {
                e.printStackTrace();
            }
            // CurReportDocument.getDoc().unlockallControllers();
            Helper.setUnoPropertyValue(CurUnoDialog.xDialogModel, "Enabled", new Boolean(true));
        }

        public void disposing(EventObject eventObject)
        {
        }
    }

    class ActionListenerImpl implements com.sun.star.awt.XActionListener
    {

        public void disposing(EventObject eventObject)
        {
        }

        public void actionPerformed(com.sun.star.awt.ActionEvent actionEvent)
        {
            try
            {
                Helper.setUnoPropertyValue(CurUnoDialog.xDialogModel, "Enabled", new Boolean(false));
//                     CurReportDocument.getDoc().xTextDocument.lockControllers();
                boolean blandscape = (((Short) CurUnoDialog.getControlProperty("optLandscape", "State")).shortValue() == 1);
                CurReportDocument.setPageOrientation((blandscape == true) ? SOOPTLANDSCAPE : SOOPTPORTRAIT);
            }
            catch (Exception exception)
            {
                exception.printStackTrace(System.out);
            }
//                CurReportDocument.getDoc().unlockallControllers();
            Helper.setUnoPropertyValue(CurUnoDialog.xDialogModel, "Enabled", new Boolean(true));
        }
    }
}
