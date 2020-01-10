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
package com.sun.star.wizards.form;

import com.sun.star.awt.XCheckBox;
import com.sun.star.awt.XFixedText;
import com.sun.star.awt.XListBox;
import com.sun.star.awt.XRadioButton;
import com.sun.star.wizards.common.Helper;
import com.sun.star.wizards.ui.CommandFieldSelection;
import com.sun.star.wizards.ui.UIConsts;
import com.sun.star.wizards.ui.UnoDialog;
import com.sun.star.wizards.ui.WizardDialog;
import com.sun.star.wizards.db.RelationController;

/**
 * @author Administrator
 *
 * To change the template for this generated type comment go to
 * Window&gt;Preferences&gt;Java&gt;Code Generation&gt;Code and Comments
 */
public class FormConfiguration
{

    WizardDialog CurUnoDialog;
    short curtabindex;
    XRadioButton optOnExistingRelation;
    XCheckBox chkcreateSubForm;
    XRadioButton optSelectManually;
    XFixedText lblSubFormDescription;
    XFixedText lblRelations;
    XListBox lstRelations;
    String[] sreferencedTables;
    // Integer ISubFormStep;
    CommandFieldSelection CurSubFormFieldSelection;
    String SSUBFORMMODE = "toggleSubFormMode";
    String STOGGLESTEPS = "toggleSteps";
    String SONEXISTINGRELATIONSELECTION = "onexistingRelationSelection";
    boolean bsupportsRelations;
    RelationController oRelationController = null;

    public FormConfiguration(WizardDialog _CurUnoDialog)
    {
        this.CurUnoDialog = _CurUnoDialog;
        curtabindex = (short) (FormWizard.SOSUBFORM_PAGE * 100);
        Integer ISubFormStep = new Integer(FormWizard.SOSUBFORM_PAGE);
        String sOnExistingRelation = CurUnoDialog.m_oResource.getResText(UIConsts.RID_FORM + 5);
        String sOnManualRelation = CurUnoDialog.m_oResource.getResText(UIConsts.RID_FORM + 7);
        String sSelectManually = CurUnoDialog.m_oResource.getResText(UIConsts.RID_FORM + 4);
        String sSelectRelation = CurUnoDialog.m_oResource.getResText(UIConsts.RID_FORM + 8);
        String sSubFormDescription = CurUnoDialog.m_oResource.getResText(UIConsts.RID_FORM + 3);

        // CheckBox 'Add sub form'
        chkcreateSubForm = CurUnoDialog.insertCheckBox("chkcreateSubForm", SSUBFORMMODE, this,
                new String[]
                {
                    "Height", "HelpURL", "Label", "PositionX", "PositionY", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    UIConsts.INTEGERS[8], "HID:34421", sSelectManually, new Integer(97), new Integer(26), ISubFormStep, new Short(curtabindex++), new Integer(160)
                });
        optOnExistingRelation = CurUnoDialog.insertRadioButton("optOnExistingRelation", STOGGLESTEPS, this,
                new String[]
                {
                    "Enabled", "Height", "HelpURL", "Label", "PositionX", "PositionY", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    Boolean.FALSE, UIConsts.INTEGERS[8], "HID:34422", sOnExistingRelation, new Integer(107), new Integer(43), ISubFormStep, new Short(curtabindex++), new Integer(160)
                });
        optSelectManually = CurUnoDialog.insertRadioButton("optSelectManually", STOGGLESTEPS, this,
                new String[]
                {
                    "Enabled", "Height", "HelpURL", "Label", "PositionX", "PositionY", "State", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    Boolean.FALSE, UIConsts.INTEGERS[8], "HID:34423", sOnManualRelation, new Integer(107), new Integer(99), new Short((short) 1), ISubFormStep, new Short(curtabindex++), new Integer(160)
                });
        lblRelations = CurUnoDialog.insertLabel("lblSelectRelation",
                new String[]
                {
                    "Enabled", "Height", "Label", "MultiLine", "PositionX", "PositionY", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    Boolean.FALSE, new Integer(19), sSelectRelation, Boolean.TRUE, new Integer(119), new Integer(56), ISubFormStep, new Short(curtabindex++), new Integer(80)
                });
        lstRelations = CurUnoDialog.insertListBox("lstrelations", SONEXISTINGRELATIONSELECTION, SONEXISTINGRELATIONSELECTION, this,
                new String[]
                {
                    "Enabled", "Height", "HelpURL", "PositionX", "PositionY", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    Boolean.FALSE, new Integer(37), "HID:34424", new Integer(201), new Integer(55), ISubFormStep, new Short(curtabindex++), new Integer(103)
                });
        lblSubFormDescription = CurUnoDialog.insertLabel("lblSubFormDescription",
                new String[]
                {
                    "Height", "Label", "MultiLine", "PositionX", "PositionY", "Step", "TabIndex", "Width"
                },
                new Object[]
                {
                    new Integer(59), sSubFormDescription, Boolean.TRUE, new Integer(110), new Integer(120), ISubFormStep, new Short(curtabindex++), new Integer(190)
                });
        CurUnoDialog.insertInfoImage(97, 120, ISubFormStep.intValue());
    }

    // public void disableSubFormCheckBox()
    // {
    //     Helper.setUnoPropertyValue(UnoDialog.getModel(chkcreateSubForm), "Enabled", Boolean.FALSE);
    // }

    public RelationController getRelationController()
    {
        return oRelationController;
    }

    public boolean areexistingRelationsdefined()
    {
        return ((chkcreateSubForm.getState() == 1) && (optOnExistingRelation.getState()));
    }

    public void toggleSubFormMode()
    {
        boolean bdoEnable = (this.chkcreateSubForm.getState() == 1);
        Helper.setUnoPropertyValue(UnoDialog.getModel(optOnExistingRelation), "Enabled", new Boolean(bdoEnable && bsupportsRelations));
        Helper.setUnoPropertyValue(UnoDialog.getModel(optSelectManually), "Enabled", new Boolean(bdoEnable));
        toggleSteps();
    }

    public void initialize(CommandFieldSelection _CurSubFormFieldSelection, RelationController _oRelationController)
    {
        oRelationController = _oRelationController;
        sreferencedTables = oRelationController.getExportedKeys();
        bsupportsRelations = (sreferencedTables.length > 0);
        Helper.setUnoPropertyValue(UnoDialog.getModel(lstRelations), "StringItemList", sreferencedTables);
        this.CurSubFormFieldSelection = _CurSubFormFieldSelection;
        toggleRelationsListbox();
        Helper.setUnoPropertyValue(UnoDialog.getModel(optOnExistingRelation), "Enabled", new Boolean(bsupportsRelations && (chkcreateSubForm.getState() == 1)));
    }

    public void toggleSteps()
    {
        boolean bDoEnableFollowingSteps;
        if (chkcreateSubForm.getState() == 1)
        {
            if (optOnExistingRelation.getState())
            {
                onexistingRelationSelection();
            }
            else if (optSelectManually.getState())
            {
                CurUnoDialog.enablefromStep(FormWizard.SOFIELDLINKER_PAGE, (CurSubFormFieldSelection.getSelectedFieldNames().length > 0));
                CurUnoDialog.setStepEnabled(FormWizard.SOSUBFORMFIELDS_PAGE, true);
            }
        }
        else
        {
            CurUnoDialog.setStepEnabled(FormWizard.SOSUBFORMFIELDS_PAGE, false);
            CurUnoDialog.setStepEnabled(FormWizard.SOFIELDLINKER_PAGE, false);
            CurUnoDialog.enablefromStep(FormWizard.SOCONTROL_PAGE, true);
        }
        toggleRelationsListbox();
    }

    public String getreferencedTableName()
    {
        if (areexistingRelationsdefined())
        {
            short[] iselected = (short[]) Helper.getUnoArrayPropertyValue(UnoDialog.getModel(lstRelations), "SelectedItems");
            if (iselected != null)
            {
                if (iselected.length > 0)
                {
                    return sreferencedTables[iselected[0]];
                }
            }
        }
        return "";
    }

    public void onexistingRelationSelection()
    {
        String scurreferencedTableName = getreferencedTableName();
        if (scurreferencedTableName.length() > 0)
        {
            if (CurSubFormFieldSelection.getSelectedCommandName().equals(scurreferencedTableName))
            {
                CurUnoDialog.enablefromStep(FormWizard.SOSUBFORMFIELDS_PAGE, true);
                CurUnoDialog.setStepEnabled(FormWizard.SOFIELDLINKER_PAGE, false);
                return;
            }
            else
            {
                CurUnoDialog.setStepEnabled(FormWizard.SOSUBFORMFIELDS_PAGE, true);
                CurUnoDialog.enablefromStep(FormWizard.SOFIELDLINKER_PAGE, false);
                return;
            }
        }
        CurUnoDialog.enablefromStep(FormWizard.SOSUBFORMFIELDS_PAGE, false);
    }

    private void toggleRelationsListbox()
    {
        boolean bdoenable = bsupportsRelations && this.optOnExistingRelation.getState() && (chkcreateSubForm.getState() == 1);
        Helper.setUnoPropertyValue(UnoDialog.getModel(lblRelations), "Enabled", new Boolean(bdoenable));
        Helper.setUnoPropertyValue(UnoDialog.getModel(lstRelations), "Enabled", new Boolean(bdoenable));
    }

    public boolean hasSubForm()
    {
        return (this.chkcreateSubForm.getState() == 1);
    }
}
