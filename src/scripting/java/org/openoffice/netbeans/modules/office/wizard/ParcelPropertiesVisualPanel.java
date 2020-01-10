/*
 * ParcelPropertiesVisualPanel.java
 *
 * Created on January 15, 2003
 */

package org.openoffice.netbeans.modules.office.wizard;

import org.openide.util.NbBundle;

/** A single panel for a wizard - the GUI portion.
 *
 * @author tomaso
 */
public class ParcelPropertiesVisualPanel extends javax.swing.JPanel {
    
    /** The wizard panel descriptor associated with this GUI panel.
     * If you need to fire state changes or something similar, you can
     * use this handle to do so.
     */
    private final ParcelPropertiesPanel panel;
    
    /** Create the wizard panel and set up some basic properties. */
    public ParcelPropertiesVisualPanel(ParcelPropertiesPanel panel) {
        this.panel = panel;
        initComponents();

        languagesComboBox.addItem("Java");
        languagesComboBox.addItem("BeanShell");

        // Provide a name in the title bar.
        setName(NbBundle.getMessage(ParcelPropertiesVisualPanel.class, "TITLE_ParcelPropertiesVisualPanel"));
        /*
        // Optional: provide a special description for this pane.
        // You must have turned on WizardDescriptor.WizardPanel_helpDisplayed
        // (see descriptor in standard iterator template for an example of this).
        try {
            putClientProperty("WizardPanel_helpURL", // NOI18N
                new URL("nbresloc:/org/openoffice/netbeans/modules/office/wizard/ParcelPropertiesVisualHelp.html")); // NOI18N
        } catch (MalformedURLException mfue) {
            throw new IllegalStateException(mfue.toString());
        }
         */
    }
    
    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    private void initComponents() {//GEN-BEGIN:initComponents
        java.awt.GridBagConstraints gridBagConstraints;

        jLabel1 = new javax.swing.JLabel();
        recipeName = new javax.swing.JTextField();
        jLabel2 = new javax.swing.JLabel();
        languagesComboBox = new javax.swing.JComboBox();
        jPanel1 = new javax.swing.JPanel();

        setLayout(new java.awt.GridBagLayout());

        setPreferredSize(new java.awt.Dimension(500, 300));
        jLabel1.setText("Parcel Recipe Name");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(12, 12, 11, 2);
        add(jLabel1, gridBagConstraints);

        recipeName.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                recipeNameActionPerformed(evt);
            }
        });

        recipeName.addFocusListener(new java.awt.event.FocusAdapter() {
            public void focusGained(java.awt.event.FocusEvent evt) {
                recipeNameFocusGained(evt);
            }
            public void focusLost(java.awt.event.FocusEvent evt) {
                recipeNameFocusLost(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.gridwidth = java.awt.GridBagConstraints.REMAINDER;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(12, 0, 11, 11);
        add(recipeName, gridBagConstraints);

        jLabel2.setText("Initial Script Language");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(0, 12, 11, 12);
        add(jLabel2, gridBagConstraints);

        languagesComboBox.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                languagesComboBoxActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.gridwidth = java.awt.GridBagConstraints.REMAINDER;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(0, 0, 11, 11);
        add(languagesComboBox, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.VERTICAL;
        gridBagConstraints.weighty = 1.0;
        add(jPanel1, gridBagConstraints);

    }//GEN-END:initComponents

    private void recipeNameFocusGained(java.awt.event.FocusEvent evt) {//GEN-FIRST:event_recipeNameFocusGained
        recipeName.selectAll();
    }//GEN-LAST:event_recipeNameFocusGained

    private void recipeNameFocusLost(java.awt.event.FocusEvent evt) {//GEN-FIRST:event_recipeNameFocusLost
        changeName();
    }//GEN-LAST:event_recipeNameFocusLost

    private void languagesComboBoxActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_languagesComboBoxActionPerformed
        String language = (String)languagesComboBox.getSelectedItem();
        panel.setLanguage(language);
    }//GEN-LAST:event_languagesComboBoxActionPerformed

    private void recipeNameActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_recipeNameActionPerformed
        changeName();
    }//GEN-LAST:event_recipeNameActionPerformed
    
    private void changeName() {
        String name = recipeName.getText().trim();
        if (name.equals(""))
            name = null;
        panel.setName(name);
    }
    
    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JTextField recipeName;
    private javax.swing.JLabel jLabel2;
    private javax.swing.JLabel jLabel1;
    private javax.swing.JPanel jPanel1;
    private javax.swing.JComboBox languagesComboBox;
    // End of variables declaration//GEN-END:variables
    
}
