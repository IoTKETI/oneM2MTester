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
import org.eclipse.titan.executorapi.exception.JniExecutorWrongStateException;

public class ExecuteCfgDialog extends JDialog {

	/** Generated serial version ID to avoid warning */
	private static final long serialVersionUID = -2926485859174706529L;

    private JTextField mTextFieldIndex = new JTextField();
    private JButton mButtonExecute = new JButton("Execute");
    
    public ExecuteCfgDialog( final DemoFrame aParent ) {
    	super( aParent, "Execute config file", true );
    	
	    // init ui elements
	    //   default values
	    mTextFieldIndex.setText("0");
	    
	    mButtonExecute.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				final JniExecutor je = JniExecutor.getInstance();
				try {
					je.executeCfg(Integer.parseInt(mTextFieldIndex.getText()));
					ExecuteCfgDialog.this.setVisible(false);
					ExecuteCfgDialog.this.dispose();
				} catch (Exception e1) {
					JOptionPane.showMessageDialog(ExecuteCfgDialog.this, e1.toString(), "Error", JOptionPane.ERROR_MESSAGE);
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
	    add( new JLabel("Number of testcases:"), c);
	    c.gridy++;
	    add( new JLabel("Testcase index:"), c);
	    
	    c.weightx = 1.0;
	    c.weighty = 0.0;
	    c.gridx = 1;
	    c.gridy = 0;
	    int numTestcases = 0;
	    try {
			numTestcases = JniExecutor.getInstance().getExecuteCfgLen();
		} catch (JniExecutorWrongStateException e1) {
			JOptionPane.showMessageDialog(ExecuteCfgDialog.this, e1.toString(), "Error", JOptionPane.ERROR_MESSAGE);
		}
		add( new JLabel( "" + numTestcases ), c );
	    c.gridy++;
	    add( mTextFieldIndex, c);
	    
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
