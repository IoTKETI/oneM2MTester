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

import java.awt.Color;
import java.awt.Dimension;
import java.awt.EventQueue;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.List;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.JTextPane;
import javax.swing.ScrollPaneConstants;
import javax.swing.filechooser.FileNameExtensionFilter;
import javax.swing.table.DefaultTableModel;
import javax.swing.text.BadLocationException;
import javax.swing.text.Style;
import javax.swing.text.StyleConstants;
import javax.swing.text.StyledDocument;

import org.eclipse.titan.executor.jni.McStateEnum;
import org.eclipse.titan.executor.jni.Timeval;
import org.eclipse.titan.executor.jni.VerdictTypeEnum;
import org.eclipse.titan.executorapi.HostController;
import org.eclipse.titan.executorapi.IJniExecutorObserver;
import org.eclipse.titan.executorapi.JniExecutor;
import org.eclipse.titan.executorapi.exception.JniExecutorIllegalArgumentException;
import org.eclipse.titan.executorapi.exception.JniExecutorJniLoadException;
import org.eclipse.titan.executorapi.exception.JniExecutorStartSessionException;
import org.eclipse.titan.executorapi.exception.JniExecutorWrongStateException;

import java.util.Map;

public class DemoFrame extends JFrame {

    /** Generated serial version ID to avoid warning */
	private static final long serialVersionUID = -7360165029886576581L;
	
	private final JButton mButtonInit = new JButton("Init");
    private final JButton mButtonAddHostController = new JButton("Add Host Controller");
    private final JButton mButtonSetConfigFilename = new JButton("Set CFG file");
    private final JButton mButtonStartSession = new JButton("Start session");
    private final JButton mButtonStartHostControllers = new JButton("Start Host Controllers");
    private final JButton mButtonConfigure = new JButton("Configure");
    private final JButton mButtonCreateMTC = new JButton("Create MTC");
    private final JButton mButtonExecuteControl = new JButton("Execute control");
    private final JButton mButtonExecuteTestcase = new JButton("Execute testcase");
    private final JButton mButtonExecuteCfg = new JButton("Execute CFG");
    private final JButton mButtonExitMTC = new JButton("Exit MTC");
    private final JButton mButtonShutdownSession = new JButton("Shutdown session");
    private final JButton mButtonPause = new JButton("Pause");
    private final JButton mButtonStopExecution = new JButton("Stop execution");
    private final JButton mButtonContinueExecution = new JButton("Continue execution");
    private final JButton mButtonBatch = new JButton("Batch execution");
    
    private final JLabel mLabelCfgFile = new JLabel("Configuration file:");
    private final JLabel mLabelHost = new JLabel("Host:");
    private final JLabel mLabelWorkingDir = new JLabel("Working directory:");
    private final JLabel mLabelExecutable = new JLabel("Executable:");
    private final JTextField mTextFieldCfgFile = new JTextField();
    private final JTextField mTextFieldHost = new JTextField();
    private final JTextField mTextFieldWorkingDir = new JTextField();
    private final JTextField mTextFieldExecutable = new JTextField();
    private final JButton mButtonCfgFile = new JButton("...");
    private final JButton mButtonWorkingDir = new JButton("...");
    private final JButton mButtonExecutable = new JButton("...");

    private final JLabel mLabelHostControllers = new JLabel("Added Host Controllers:");
    /** model for mTableHostControllers */
    private final HostControllerTableModel mModel = new HostControllerTableModel(new String[] {"Host","Working directory","Executable"}, 0);
    private final JTable mTableHostControllers = new JTable(mModel);
    
    /** style for mTextPaneTitanConsole */
    private Style mStyle;
    private final JTextPane mTextPaneTitanConsole = new JTextPane();
    
    private final JLabel mLabelState = new JLabel("MC state:");
    private final JTextField mTextFieldState = new JTextField();
    
	/**
         * Observer for step-by-step test execution.
         */
	private class DemoObserver implements IJniExecutorObserver {

		@Override
		public void statusChanged(McStateEnum aNewState) {
			updateUi();
		}

		@Override
		public void error(int aSeverity, String aMsg) {
			printConsole("Error");
			printConsole("Severity: " + aSeverity);
			printConsole("Message: " + aMsg);
		}

		@Override
		public void notify(Timeval aTime, String aSource, int aSeverity,String aMsg) {
			printConsole("Notification");
			printConsole("Time: " + aTime);
			printConsole("Source: " + aSource);
			printConsole("Severity: " + aSeverity);
			printConsole("Message: " + aMsg);
		}

		@Override
		public void verdict( String aTestcase, VerdictTypeEnum aVerdictType ) {
			addVerdict( aTestcase, aVerdictType );
		}

		@Override
		public void verdictStats( Map<VerdictTypeEnum, Integer> aVerdictStats ) {
			final int none = aVerdictStats.get(VerdictTypeEnum.NONE);
			final int pass = aVerdictStats.get(VerdictTypeEnum.PASS);
			final int inconc = aVerdictStats.get(VerdictTypeEnum.INCONC);
			final int fail = aVerdictStats.get(VerdictTypeEnum.FAIL);
			final int error = aVerdictStats.get(VerdictTypeEnum.ERROR);

			final StringBuilder sb = new StringBuilder();
			sb.append("<html>"); // to make it multiline
			if (mVerdicts != null) {
				for ( int i = 0; i < mVerdicts.size(); i++ ) {
					Verdict v = mVerdicts.get(i);
					sb.append(v.mTestcase).append(": ").append(v.mVerdictType.getName()).append("<br>");
				}
			}
			if ( none > 0 ) {
				sb.append("None: ").append(none).append("<br>");
			}
			if ( pass > 0 ) {
				sb.append("Pass: ").append(pass).append("<br>");
			}
			if ( inconc > 0 ) {
				sb.append("Inconc: ").append(inconc).append("<br>");
			}
			if ( fail > 0 ) {
				sb.append("Fail: ").append(fail).append("<br>");
			}
			if ( error > 0 ) {
				sb.append("Error: ").append(error).append("<br>");
			}
			
			mVerdicts = null;
			// Showing âJOptionPane.showMessageDialogâ without stopping flow of execution,
			// this line would block the thread:
			//JOptionPane.showMessageDialog(DemoFrame.this, sb.toString(), "Verdict statistics", JOptionPane.INFORMATION_MESSAGE);
			EventQueue.invokeLater(new Runnable(){
				@Override
				public void run() {
					JOptionPane op = new JOptionPane(sb.toString(),JOptionPane.INFORMATION_MESSAGE);
					JDialog dialog = op.createDialog("Verdict statistics");
					dialog.setAlwaysOnTop(true);
					dialog.setModal(true);
					dialog.setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);      
					dialog.setVisible(true);
				}
			});
		}
	}
	
        /**
         * Observer for step-by-step test execution.
         * <p>
         * NOTE: This observer is not used for batch test execution.
         * @see #batchExecution()
         */
	private final DemoObserver mObserver = new DemoObserver();
	
        /**
         * Table model for the added HCs to make it non-editable
         */
	private class HostControllerTableModel extends DefaultTableModel {
	    /** Generated serial version ID to avoid warning */
		private static final long serialVersionUID = -2214569504530345210L;

		public HostControllerTableModel(final String[] columnNames, final int i) {
			super(columnNames, i);
		}

		@Override
    	public boolean isCellEditable(int row, int column) {
    		//all cells false
    		return false;
    	}
    }
	
	public DemoFrame() {
	    super( "TITAN Executor API Demo" );
	    initUi();
	    updateUi();
	    
	    setDefaultCloseOperation( JFrame.EXIT_ON_CLOSE );
	    setSize( 1200, 750 );
	    // place to the middle of the screen
	    Dimension dim = Toolkit.getDefaultToolkit().getScreenSize();
	    setLocation(dim.width/2-this.getSize().width/2, dim.height/2-this.getSize().height/2);
	}
	
        /**
         * Initialization of the UI elements:
         * <ul>
         *   <li> Setting non-editable elements
         *   <li> Setting default texts where applicable
         *   <li> Adding tooltips to UI elements
         *   <li> Defining behavior of the buttons
         *   <li> Building layout
         * </ul>
         */
	private void initUi() {
	    // init ui elements
	    mTextPaneTitanConsole.setEditable(false);
	    mTextFieldState.setEditable(false);
        mStyle = mTextPaneTitanConsole.addStyle("standard", null); // standard style
	    
	    //   default values
	    mTextFieldCfgFile.setText( CommonData.getDefaultCfgFile() );
	    mTextFieldHost.setText( CommonData.LOCALHOST );
	    mTextFieldWorkingDir.setText( CommonData.getDefaultWorkingDir() );
	    mTextFieldExecutable.setText( CommonData.EXECUTABLE );
	    
	    initTooltips();
	    initButtonActions();
	    initLayout();
	}

	/**
	 * Adds tooltips to UI elements where needed.
	 */
	private void initTooltips() {
		final String TOOLTIPTEXT_INIT = "Connects to MC. Start here to execute tests step-by-step using atomic commands.";
		final String TOOLTIPTEXT_ADDHC = "Adds a new host controller using the following fields: Host, Working directory, Executable. In general case there is 1 HC.";
		final String TOOLTIPTEXT_SET_CFG_FILE = "Sends CFG file name to MC.";
		final String TOOLTIPTEXT_START_SESSION = "Starts MC session to listen for connecting HCs";
		final String TOOLTIPTEXT_START_HCS = "Starts the added Host Controllers to connect with MC.";
		final String TOOLTIPTEXT_CONFIGURE = "MC sends the config string (which was previously processed by Set CFG file) to all HCs.";
		final String TOOLTIPTEXT_CREATE_MTC = "Creates Main Test Component";
		final String TOOLTIPTEXT_EXECUTE_CONTROL = "Executes control by module name";
		final String TOOLTIPTEXT_EXECUTE_TESTCASE = "Executes testcase by module and testcase name";
		final String TOOLTIPTEXT_EXECUTE_CFG = "Executes testcases by index, which are defined in the CFG file";
		final String TOOLTIPTEXT_EXIT_MTC = "Exits from Main Test Component, use it after testcase execution.";
		final String TOOLTIPTEXT_SHUTDOWN_SESSION = "Closes connection to MC.";
		final String TOOLTIPTEXT_BATCHEXECUTION = "One step execution. It executes cfg file execute list using 1 HC defined by Host, Working directory, Executable.";
		
		final String TOOLTIPTEXT_CFG_FILE = "TTCN-3 configutation file (.cfg) with full path";
		final String TOOLTIPTEXT_HOST = "Host Controller host name. It is localhost in the following cases: NULL, 0.0.0.0, empty string";
		final String TOOLTIPTEXT_WORKINGDIR = "Host Controller working directory without the ending /";
		final String TOOLTIPTEXT_EXECUTABLE = "Host Controller executable file name without path";
		final String TOOLTIPTEXT_TITANCONSOLE = "TITAN Console";
		
	    mButtonInit.setToolTipText( TOOLTIPTEXT_INIT );
	    mButtonAddHostController.setToolTipText( TOOLTIPTEXT_ADDHC );
	    mButtonSetConfigFilename.setToolTipText( TOOLTIPTEXT_SET_CFG_FILE );
	    mButtonStartSession.setToolTipText( TOOLTIPTEXT_START_SESSION );
	    mButtonStartHostControllers.setToolTipText( TOOLTIPTEXT_START_HCS );
	    mButtonConfigure.setToolTipText( TOOLTIPTEXT_CONFIGURE );
	    mButtonCreateMTC.setToolTipText( TOOLTIPTEXT_CREATE_MTC );
	    mButtonExecuteControl.setToolTipText( TOOLTIPTEXT_EXECUTE_CONTROL );
	    mButtonExecuteTestcase.setToolTipText( TOOLTIPTEXT_EXECUTE_TESTCASE );
	    mButtonExecuteCfg.setToolTipText( TOOLTIPTEXT_EXECUTE_CFG );
	    mButtonExitMTC.setToolTipText( TOOLTIPTEXT_EXIT_MTC );
	    mButtonShutdownSession.setToolTipText( TOOLTIPTEXT_SHUTDOWN_SESSION );
	    mButtonBatch.setToolTipText( TOOLTIPTEXT_BATCHEXECUTION );

	    mLabelCfgFile.setToolTipText( TOOLTIPTEXT_CFG_FILE );
	    mLabelHost.setToolTipText( TOOLTIPTEXT_HOST );
	    mLabelWorkingDir.setToolTipText( TOOLTIPTEXT_WORKINGDIR );
	    mLabelExecutable.setToolTipText( TOOLTIPTEXT_EXECUTABLE );
	    
	    mTextFieldCfgFile.setToolTipText( TOOLTIPTEXT_CFG_FILE );
	    mTextFieldHost.setToolTipText( TOOLTIPTEXT_HOST );
	    mTextFieldWorkingDir.setToolTipText( TOOLTIPTEXT_WORKINGDIR );
	    mTextFieldExecutable.setToolTipText( TOOLTIPTEXT_EXECUTABLE );
	    
	    mButtonCfgFile.setToolTipText( TOOLTIPTEXT_CFG_FILE );
	    mButtonWorkingDir.setToolTipText( TOOLTIPTEXT_WORKINGDIR );
	    mButtonExecutable.setToolTipText( TOOLTIPTEXT_EXECUTABLE );
	    
	    mTextPaneTitanConsole.setToolTipText( TOOLTIPTEXT_TITANCONSOLE );
	}

	/**
	 * Sets button behaviors
	 */
	private void initButtonActions() {
	    final JniExecutor je = JniExecutor.getInstance();
	    mButtonInit.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				try {
					je.init();
					je.setObserver(mObserver);
					updateUi(); // it must be called after synchronous calls
				} catch (JniExecutorWrongStateException	| JniExecutorJniLoadException e1) {
					printConsole(e1.toString());
				}
			}
		});
	    mButtonAddHostController.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				final JniExecutor je = JniExecutor.getInstance();
				try {
					HostController hc = new HostController(mTextFieldHost.getText(), mTextFieldWorkingDir.getText(), mTextFieldExecutable.getText());
					je.addHostController(hc);
					updateTableHostControllers();
				} catch (JniExecutorIllegalArgumentException | JniExecutorWrongStateException e1) {
					JOptionPane.showMessageDialog(DemoFrame.this, e1.toString(), "Error", JOptionPane.ERROR_MESSAGE);
				}
			}
		});
	    mButtonSetConfigFilename.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				try {
					je.setConfigFileName(mTextFieldCfgFile.getText());
					updateUi(); // it must be called after synchronous calls
				} catch (JniExecutorWrongStateException	| JniExecutorIllegalArgumentException e1) {
					printConsoleException(e1);
				}
			}
		});
	    mButtonStartSession.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				try {
					je.startSession();
					updateUi(); // it must be called after synchronous calls
				} catch (JniExecutorWrongStateException | JniExecutorStartSessionException e1) {
					printConsoleException(e1);
				}
			}
		});
	    mButtonStartHostControllers.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				try {
					je.startHostControllers();
				} catch (JniExecutorWrongStateException e1) {
					printConsoleException(e1);
				}
			}
		});
	    mButtonConfigure.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				try {
					je.configure();
				} catch (JniExecutorWrongStateException e1) {
					printConsoleException(e1);
				}
			}
		});
	    mButtonCreateMTC.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				try {
					je.createMTC();
				} catch (JniExecutorWrongStateException e1) {
					printConsoleException(e1);
				}
			}
		});
	    mButtonExecuteControl.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				new ExecuteControlDialog(DemoFrame.this).setVisible(true);
			}
		});
	    mButtonExecuteTestcase.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				new ExecuteTestcaseDialog(DemoFrame.this).setVisible(true);
			}
		});
	    mButtonExecuteCfg.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				new ExecuteCfgDialog(DemoFrame.this).setVisible(true);
			}
		});
	    mButtonExitMTC.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				try {
					je.exitMTC();
				} catch (JniExecutorWrongStateException e1) {
					printConsoleException(e1);
				}
			}
		});
	    mButtonShutdownSession.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				je.shutdownSession();
				// sometimes it's synchronous
				updateUi(); // it must be called after synchronous calls
			}
		});
	    mButtonPause.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				try {
					final boolean paused = je.isPaused();
					je.pauseExecution(!paused);
					updateUi(); // it must be called after synchronous calls
				} catch (JniExecutorWrongStateException e1) {
					printConsoleException(e1);
				}
			}
		});
	    mButtonStopExecution.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				try {
					je.stopExecution();
				} catch (JniExecutorWrongStateException e1) {
					printConsoleException(e1);
				}
			}
		});
	    mButtonContinueExecution.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				try {
					je.continueExecution();
				} catch (JniExecutorWrongStateException e1) {
					printConsoleException(e1);
				}
			}
		});
	    mButtonBatch.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				batchExecution();
			}
	    });
	    
	    mButtonCfgFile.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				final JFileChooser fc = new JFileChooser();
				fc.setFileFilter(new FileNameExtensionFilter("TTCN-3 CFG FILES", "cfg", "cfg"));
				File f = new File( mTextFieldWorkingDir.getText() );
				if ( f.exists() && f.isDirectory() ) {
					fc.setCurrentDirectory( f );
				} else {
					fc.setCurrentDirectory( new File( CommonData.getDefaultWorkingDir() ) );
				}
				fc.setDialogTitle("Choose Configuration File");
				fc.setFileSelectionMode(JFileChooser.FILES_ONLY);
				// disable the "All files" option.
				fc.setAcceptAllFileFilterUsed(false);
				if (fc.showOpenDialog(DemoFrame.this) == JFileChooser.APPROVE_OPTION) { 
					mTextFieldCfgFile.setText( fc.getSelectedFile().toString() );
				} else {
					// no selection
				}
			}
	    });
	    mButtonWorkingDir.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				final JFileChooser fc = new JFileChooser();
				fc.setCurrentDirectory(new File( CommonData.getDefaultWorkingDir() ));
				fc.setDialogTitle("Choose working directory");
				fc.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
				// disable the "All files" option.
				fc.setAcceptAllFileFilterUsed(false);
				if (fc.showOpenDialog(DemoFrame.this) == JFileChooser.APPROVE_OPTION) { 
					mTextFieldWorkingDir.setText( fc.getSelectedFile().toString() );
				} else {
					// no selection
				}
			}
	    });
	    mButtonExecutable.addActionListener(new ActionListener() {
			@Override
			public void actionPerformed(ActionEvent e) {
				final JFileChooser fc = new JFileChooser();
				File f = new File( mTextFieldWorkingDir.getText() );
				if ( f.exists() && f.isDirectory() ) {
					fc.setCurrentDirectory( f );
				} else {
					fc.setCurrentDirectory( new File( CommonData.getDefaultWorkingDir() ) );
				}
				fc.setDialogTitle("Choose working directory");
				fc.setFileSelectionMode(JFileChooser.FILES_ONLY);
				// disable the "All files" option.
				fc.setAcceptAllFileFilterUsed(false);
				if (fc.showOpenDialog(DemoFrame.this) == JFileChooser.APPROVE_OPTION) { 
					mTextFieldExecutable.setText( fc.getSelectedFile().getName() );
				} else {
					// no selection
				}
			}
	    });
	}

	/**
	 * Creates layout and adds UI elements to layout
	 */
	private void initLayout() {
	    setLayout( new GridBagLayout() );
	    GridBagConstraints c = new GridBagConstraints();
	    c.fill = GridBagConstraints.BOTH;
	    c.insets = new Insets(10, 10, 10, 10);

	    c.gridx = 0;
	    c.gridy = 0;
	    add( mButtonInit, c );
	    c.gridy++;
	    add( mButtonAddHostController, c );
	    c.gridy++;
	    add( mButtonSetConfigFilename, c );
	    c.gridy++;
	    add( mButtonStartSession, c );
	    c.gridy++;
	    add( mButtonStartHostControllers, c );
	    c.gridy++;
	    add( mButtonConfigure, c );
	    c.gridy++;
	    add( mButtonCreateMTC, c );
	    c.gridy++;
	    add( mButtonExecuteControl, c );
	    c.gridy++;
	    add( mButtonExecuteTestcase, c );
	    c.gridy++;
	    add( mButtonExecuteCfg, c );
	    c.gridy++;
	    add( mButtonPause, c );
	    c.gridy++;
	    add( mButtonStopExecution, c );
	    c.gridy++;
	    add( mButtonContinueExecution, c );
	    c.gridy++;
	    add( mButtonExitMTC, c );
	    c.gridy++;
	    add( mButtonShutdownSession, c );
	    c.gridy++;
	    add( mButtonBatch, c );
	    
	    c.weightx = 0.0;
	    c.weighty = 0.0;
	    c.gridx = 1;
	    c.gridy = 0;
	    add( mLabelCfgFile, c );
	    c.weightx = 1.0;
	    c.weighty = 0.0;
	    c.gridx = 2;
	    add( mTextFieldCfgFile, c );
	    c.weightx = 0.0;
	    c.weighty = 0.0;
	    c.gridx = 3;
	    add( mButtonCfgFile, c );
	    
	    c.weightx = 0.0;
	    c.weighty = 0.0;
	    c.gridx = 1;
	    c.gridy++;
	    add( mLabelHost, c );
	    c.weightx = 1.0;
	    c.weighty = 0.0;
	    c.gridx = 2;
	    add( mTextFieldHost, c );

	    c.weightx = 0.0;
	    c.weighty = 0.0;
	    c.gridx = 1;
	    c.gridy++;
	    add( mLabelWorkingDir, c );
	    c.weightx = 1.0;
	    c.weighty = 0.0;
	    c.gridx = 2;
	    add( mTextFieldWorkingDir, c );
	    c.weightx = 0.0;
	    c.weighty = 0.0;
	    c.gridx = 3;
	    add( mButtonWorkingDir, c );

	    c.weightx = 0.0;
	    c.weighty = 0.0;
	    c.gridx = 1;
	    c.gridy++;
	    add( mLabelExecutable, c );
	    c.weightx = 1.0;
	    c.weighty = 0.0;
	    c.gridx = 2;
	    add( mTextFieldExecutable, c );
	    c.weightx = 0.0;
	    c.weighty = 0.0;
	    c.gridx = 3;
	    add( mButtonExecutable, c );
	    
	    c.weightx = 0.0;
	    c.weighty = 0.0;
	    c.gridx = 1;
	    c.gridy++;
	    add( mLabelHostControllers, c );

	    c.weightx = 0.0;
	    c.weighty = 0.0;
	    c.gridy++;
	    c.gridwidth = 3;
	    c.gridheight = 2;
	    JScrollPane scrollHostControllers = new JScrollPane( mTableHostControllers );
        scrollHostControllers.setVerticalScrollBarPolicy(ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS);
        add( scrollHostControllers, c );
	    c.gridwidth = 1;
	    c.gridheight = 1;
	    
	    c.weightx = 1.0;
	    c.weighty = 0.0;
	    c.gridx = 1;
	    c.gridy = 7;
	    c.gridwidth = 3;
	    c.gridheight = 8;
	    JScrollPane scrollTitanConsole = new JScrollPane( mTextPaneTitanConsole );
        scrollTitanConsole.setVerticalScrollBarPolicy(ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS);
        add( scrollTitanConsole, c );
	    c.gridwidth = 1;
	    c.gridheight = 1;
	    
	    c.weightx = 0.0;
	    c.weighty = 0.0;
	    c.gridy += 8;
	    add( mLabelState, c);
	    
	    c.weightx = 1.0;
	    c.weighty = 0.0;
	    c.gridx = 2;
	    c.gridwidth = 2;
	    add( mTextFieldState, c);
	    c.gridwidth = 1;
	}

	/**
         * Update UI elements:
	 * <ul>
	 *   <li> Sets enabled/disabled state for UI elements depending on the state
	 *   <li> Refreshes state text
	 *   <li> Refreshes added host controllers list
	 * </ul>
	 */
	private void updateUi() {
		switchOffButtons();
		final JniExecutor je = JniExecutor.getInstance();
		final boolean connected = je.isConnected();
		if (!connected) {
			mButtonInit.setEnabled(true);
			mTextFieldCfgFile.setEditable(true);
			mButtonCfgFile.setEnabled(true);
			mButtonBatch.setEnabled(true);
			
			// host controller related ui elements
			// switched on because of batched execution
			mTextFieldHost.setEditable(true);
			mTextFieldWorkingDir.setEditable(true);
			mTextFieldExecutable.setEditable(true);
			mButtonWorkingDir.setEnabled(true);
			mButtonExecutable.setEnabled(true);
		} else {
			final McStateEnum state = je.getState();
			switch (state) {
			case MC_INACTIVE:
				mButtonAddHostController.setEnabled(true);
				mButtonSetConfigFilename.setEnabled(true);
				mButtonStartSession.setEnabled(true);
				mTextFieldCfgFile.setEditable(true);
				mButtonCfgFile.setEnabled(true);
				
				// host controller related ui elements
				// switched on because Add Host Controller button is switched on
				mTextFieldHost.setEditable(true);
				mTextFieldWorkingDir.setEditable(true);
				mTextFieldExecutable.setEditable(true);
				mButtonWorkingDir.setEnabled(true);
				mButtonExecutable.setEnabled(true);
				break;
			case MC_LISTENING:
				mButtonAddHostController.setEnabled(true);
				mButtonStartHostControllers.setEnabled(true);
				mButtonConfigure.setEnabled(true);
				
				// host controller related ui elements
				// switched on because Add Host Controller button is switched on
				mTextFieldHost.setEditable(true);
				mTextFieldWorkingDir.setEditable(true);
				mTextFieldExecutable.setEditable(true);
				mButtonWorkingDir.setEnabled(true);
				mButtonExecutable.setEnabled(true);
				break;
			case MC_LISTENING_CONFIGURED:
				mButtonAddHostController.setEnabled(true);
				mButtonStartHostControllers.setEnabled(true);
				mButtonConfigure.setEnabled(true);
				
				// host controller related ui elements
				// switched on because Add Host Controller button is switched on
				mTextFieldHost.setEditable(true);
				mTextFieldWorkingDir.setEditable(true);
				mTextFieldExecutable.setEditable(true);
				mButtonWorkingDir.setEnabled(true);
				mButtonExecutable.setEnabled(true);
				break;
			case MC_HC_CONNECTED:
				mButtonConfigure.setEnabled(true);
				break;
			case MC_CONFIGURING:
				break;
			case MC_ACTIVE:
				mButtonCreateMTC.setEnabled(true);
				break;
			case MC_SHUTDOWN:
				break;
			case MC_CREATING_MTC:
				break;
			case MC_READY:
				mButtonExecuteControl.setEnabled(true);
				mButtonExecuteTestcase.setEnabled(true);
				mButtonExecuteCfg.setEnabled(true);
				mButtonExitMTC.setEnabled(true);
				break;
			case MC_TERMINATING_MTC:
				break;
			case MC_EXECUTING_CONTROL:
				mButtonStopExecution.setEnabled(true);
				break;
			case MC_EXECUTING_TESTCASE:
				mButtonStopExecution.setEnabled(true);
				break;
			case MC_TERMINATING_TESTCASE:
				break;
			case MC_PAUSED:
				mButtonContinueExecution.setEnabled(true);
				mButtonStopExecution.setEnabled(true);
				break;
			default:
				break;
			}
			
			mButtonPause.setEnabled(true);
			try {
				pausedButton(je.isPaused());
			} catch (JniExecutorWrongStateException e) {
				printConsoleException(e);
			}
			mButtonShutdownSession.setEnabled(true);
		}
		updateState();
		updateTableHostControllers();
	}
	
	/**
	 * Refreshes state text
	 */
	private void updateState() {
		final JniExecutor je = JniExecutor.getInstance();
		final boolean connected = je.isConnected();
		if (!connected) {
			mTextFieldState.setText("Disconnected");
		} else {
			final McStateEnum state = je.getState();
			mTextFieldState.setText( state.toString() );
		}
	}
	
        /**
         * Set all buttons to disabled, except buttons which can be pressed in any state.
         */
	private void switchOffButtons() {
		mButtonInit.setEnabled(false);
		mButtonAddHostController.setEnabled(false);
		mButtonSetConfigFilename.setEnabled(false);
		mButtonStartSession.setEnabled(false);
		mButtonStartHostControllers.setEnabled(false);
		mButtonConfigure.setEnabled(false);
		mButtonCreateMTC.setEnabled(false);
		mButtonExecuteControl.setEnabled(false);
		mButtonExecuteTestcase.setEnabled(false);
		mButtonExecuteCfg.setEnabled(false);
		mButtonExitMTC.setEnabled(false);
		mButtonShutdownSession.setEnabled(false);
		mButtonPause.setEnabled(false);
		mButtonStopExecution.setEnabled(false);
		mButtonContinueExecution.setEnabled(false);
		mButtonBatch.setEnabled(false);
		
		mTextFieldCfgFile.setEditable(false);
		mButtonCfgFile.setEnabled(false);
		
		mTextFieldHost.setEditable(false);
		mTextFieldWorkingDir.setEditable(false);
		mTextFieldExecutable.setEditable(false);
		mButtonWorkingDir.setEnabled(false);
		mButtonExecutable.setEnabled(false);
	}

        /**
         * Sets the text of pause button according to the paused state
         * @param aOn true: on, false: off
         */
	private void pausedButton(boolean aOn) {
		mButtonPause.setText("Pause" + (aOn ? " (ON)" : " (OFF)"));
	}

        /**
         * Update host controller table.
         * This is called always on UI update,
         * and changed when new HC is added or table is deleted after shutdown and before init.
         */
	public void updateTableHostControllers() {
		final JniExecutor je = JniExecutor.getInstance();
		List<HostController> hcs = je.getHostControllers();
		if (hcs != null) {
			mModel.setRowCount(hcs.size());
			for (int i = 0; i < hcs.size(); i++) {
				HostController hc = hcs.get(i);
				mModel.setValueAt(hc.getHost(), i, 0);
				mModel.setValueAt(hc.getWorkingDirectory(), i, 1);
				mModel.setValueAt(hc.getExecutable(), i, 2);
			}
		} else {
			mModel.setRowCount(0);
		}
	}
	
	/**
         * Observer used for batch execution
         */
	private class DemoBatchObserver extends DemoObserver {
            
		/**
		 * The length of the execute list of the cfg file,
		 * or -1 if it's not known yet. It is read in MC_READY state
		 */
		private int mExecuteCfgLen = -1;
		
		/**
		 * The index of the current item, which is currently executed
		 * Used for CFG_FILE and TEST_CONTROL
		 */
		private int mExecuteIndex = -1;
                
		@Override
		public void statusChanged(McStateEnum aNewState) {
			final JniExecutor je = JniExecutor.getInstance();
			try {
				switch (aNewState) {
				case MC_INACTIVE:
					// back to normal
					updateUi();
					break;
				case MC_LISTENING_CONFIGURED:
					je.startHostControllers();
					break;
				case MC_HC_CONNECTED:
					je.configure();
					break;
				case MC_ACTIVE:
					if ( executionReady() ) {
						je.shutdownSession();
					}
					else {
						je.createMTC();
					}
					break;
				case MC_READY:
					if ( executionReady() ) {
						je.exitMTC();
					}
					else {
						executeNext( je );
					}
					break;
				default:
					break;
				}
				updateState();
			} catch (Exception e) {
				printConsoleException(e);
				updateUi();
			}
		}
		
                /**
                 * @return true if execution of all items from the configuration file execution list is finished
                 */
		private boolean executionReady() {
			return mExecuteCfgLen != -1 && mExecuteIndex + 1 >= mExecuteCfgLen;
		}

                /**
                 * Executes next test from the configuration file execution list
                 * @param aJe executor instance
                 * @throws JniExecutorWrongStateException
                 * @throws JniExecutorIllegalArgumentException
                 */
		private void executeNext(final JniExecutor aJe) throws JniExecutorWrongStateException, JniExecutorIllegalArgumentException {
			if ( mExecuteCfgLen == -1 ) {
				mExecuteCfgLen = aJe.getExecuteCfgLen();
			}
			aJe.executeCfg( ++mExecuteIndex );
		}
	}
	
        /**
         * Execute the test of the CFG execution list in one step
         * instead of calling the atomic MainController commands one-by-one.
         */
	private void batchExecution() {
		switchOffButtons();
		IJniExecutorObserver o = new DemoBatchObserver(); 
		final JniExecutor je = JniExecutor.getInstance();
		try {
			je.init();
			final HostController hc = new HostController( mTextFieldHost.getText(), mTextFieldWorkingDir.getText(), mTextFieldExecutable.getText());
			je.addHostController(hc);
			je.setConfigFileName( mTextFieldCfgFile.getText() );
			je.setObserver(o);
			je.startSession();
			//je.configure(); // to go through MC_LISTENING_CONFIGURED state
			je.startHostControllers(); // to go through MC_HC_CONNECTED state
		} catch (JniExecutorWrongStateException | JniExecutorJniLoadException | JniExecutorIllegalArgumentException | JniExecutorStartSessionException e) {
			printConsoleException(e);
			updateUi();
		}
	}

	/**
	 * Writes new message to the console
	 * @param aMsg the new message line
	 * @param aColor font color
	 */
	private void printConsole( final String aMsg, final Color aColor ) {
        StyleConstants.setForeground(mStyle, aColor);
		StyledDocument doc = mTextPaneTitanConsole.getStyledDocument();
		try {
			doc.insertString( doc.getLength(), aMsg + "\n", mStyle );
		} catch (BadLocationException e){
		}
		// scroll to bottom when text is appended
		mTextPaneTitanConsole.setCaretPosition(mTextPaneTitanConsole.getText().length() - 1);
	}
	
	/**
	 * Writes new info message to the console with black
	 * @param aMsg the new message line
	 */
	private void printConsole( final String aMsg ) {
		printConsole(aMsg, Color.black);
	}
	
	/**
	 * Writes new error message to the console with red
	 * @param aMsg the new message line
	 */
	private void printConsoleError( final String aMsg ) {
		printConsole(aMsg, Color.red);
	}
	
	/**
	 * Writes new exception message and stack trace to the console with red
	 * @param aMsg the new message line
	 */
	private void printConsoleException(final Exception e) {
		StringWriter sw = new StringWriter();
		PrintWriter pw = new PrintWriter(sw);
		e.printStackTrace(pw);
		printConsoleError(sw.toString());
	}
	
        /**
         * Verdict of the execution of 1 testcase
         */
	private class Verdict {
		/** testcase name */
		public String mTestcase;
		/** verdict type (none/pass/inconc/fail/error) */
		public VerdictTypeEnum mVerdictType;
	}
	
        /** Verdict statistics */
	private List<Verdict> mVerdicts = null;
	
        /**
         * Add a new verdict to the verdict statistics
         * @param aTestcase testcase name
         * @param aVerdictType verdict type name (none/pass/inconc/fail/error)
         */
	private void addVerdict(final String aTestcase, final VerdictTypeEnum aVerdictType) {
		if ( mVerdicts == null ) {
			mVerdicts = new ArrayList<>();
		}
		Verdict v = new Verdict();
		v.mTestcase = aTestcase;
		v.mVerdictType = aVerdictType;
		mVerdicts.add(v);
	}
}
