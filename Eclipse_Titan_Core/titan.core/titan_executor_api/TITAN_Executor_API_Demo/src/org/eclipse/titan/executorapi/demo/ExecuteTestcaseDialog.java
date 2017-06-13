/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Balasko, Jeno
 *   Lovassy, Arpad
 *
 ******************************************************************************/
package org.eclipse.titan.executorapi.demo;

import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JTextField;

import org.eclipse.titan.executorapi.JniExecutor;
import org.eclipse.titan.executorapi.exception.JniExecutorIllegalArgumentException;
import org.eclipse.titan.executorapi.exception.JniExecutorWrongStateException;

public class ExecuteTestcaseDialog extends JDialog {

	/** Generated serial version ID to avoid warning */
	private static final long serialVersionUID = 9090176653704431781L;

    private JTextField mTextFieldModule = new JTextField();
    private JTextField mTextFieldTestcase = new JTextField();
    private JButton mButtonExecute = new JButton("Execute");
    
    public ExecuteTestcaseDialog( final DemoFrame aParent ) {
    	super( aParent, "Execute testcase", true );
    	
	    // init ui elements
	    //   default values
	    mTextFieldModule.setText(CommonData.MODULE);
	    mTextFieldTestcase.setText(CommonData.TESTCASE);
	    
	    mButtonExecute.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				final JniExecutor je = JniExecutor.getInstance();
				try {
					je.executeTestcase(mTextFieldModule.getText(), mTextFieldTestcase.getText());
					ExecuteTestcaseDialog.this.setVisible(false);
					ExecuteTestcaseDialog.this.dispose();
				} catch (JniExecutorIllegalArgumentException | JniExecutorWrongStateException e1) {
					JOptionPane.showMessageDialog(ExecuteTestcaseDialog.this, e1.toString(), "Error", JOptionPane.ERROR_MESSAGE);
				}
			}
		});

	    // add ui elements to layout
	    setLayout( new GridBagLayout() );
	    GridBagConstraints c = new GridBagConstraints();
	    c.fill = GridBagConstraints.BOTH;
	    c.insets = new Insets(10, 10, 10, 10);

	    c.gridx = 0;
	    c.gridy = 0;
	    add( new JLabel("Module:"), c);
	    c.gridy++;
	    add( new JLabel("Testcase:"), c);
	    
	    c.weightx = 1.0;
	    c.weighty = 0.0;
	    c.gridx = 1;
	    c.gridy = 0;
	    add( mTextFieldModule, c);
	    c.gridy++;
	    add( mTextFieldTestcase, c);
	    
	    c.gridx = 0;
	    c.gridy++;
	    c.gridwidth = 2;
	    c.fill = GridBagConstraints.NONE;
	    add(mButtonExecute, c);
	    
	    setDefaultCloseOperation( JFrame.DISPOSE_ON_CLOSE );
	    setSize( 600, 200 );
	    // place to the middle of the screen
	    Dimension dim = Toolkit.getDefaultToolkit().getScreenSize();
	    setLocation(dim.width/2-this.getSize().width/2, dim.height/2-this.getSize().height/2);
    }
}
