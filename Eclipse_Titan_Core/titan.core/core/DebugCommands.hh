/******************************************************************************
 * Copyright (c) 2000-2017 Ericsson Telecom AB
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   
 *   Baranyi, Botond – initial implementation
 *
 ******************************************************************************/

#ifndef DEBUGCOMMANDS_HH
#define DEBUGCOMMANDS_HH

/** list of commands coming from the user interface to the debugger (parameters listed in comments) */

// settings
#define D_SWITCH                   1 // 1, "on" or "off"
#define D_SET_BREAKPOINT           2 // 2-3, module name, and line number or function name, + optional batch file name
#define D_REMOVE_BREAKPOINT        3 // 1-2, 'all', or module name + 'all' or line number or function name
#define D_SET_AUTOMATIC_BREAKPOINT 4 // 2-3, "error" or "fail", + "off", or "on" + optional batch file name
#define D_SET_OUTPUT               5 // 1-2, "console", or "file" or "both", + file name
#define D_SET_GLOBAL_BATCH_FILE    6 // 1-2, "off", or "on" + batch file name
#define D_FUNCTION_CALL_CONFIG     7 // 1-2, ring buffer size or "all", or "file" + file name
#define D_PRINT_SETTINGS           8 // 0
// printing and overwriting data
#define D_LIST_COMPONENTS          9 // 0
#define D_SET_COMPONENT           10 // 1, component name or component reference
#define D_PRINT_CALL_STACK        11 // 0
#define D_SET_STACK_LEVEL         12 // 1, stack level
#define D_LIST_VARIABLES          13 // 0-2, optional "local", "global", "comp" or "all", + optional filter (pattern)
#define D_PRINT_VARIABLE          14 // 1+, list of variable names
#define D_OVERWRITE_VARIABLE      15 // 2, variable name, new value (in module parameter syntax)
#define D_PRINT_FUNCTION_CALLS    16 // 0-1, optional "all" or number of calls
// stepping
#define D_STEP_OVER               17 // 0
#define D_STEP_INTO               18 // 0
#define D_STEP_OUT                19 // 0
#define D_RUN_TO_CURSOR           20 // 2, module name, and line number or function name
// the halted state
#define D_HALT                    21 // 0
#define D_CONTINUE                22 // 0
#define D_EXIT                    23 // 1, "test" or "all"
// initialization
#define D_SETUP                   24 // 11+:
                                     // 1 argument for D_SWITCH,
                                     // 2 arguments for D_SET_OUTPUT,
                                     // 2 arguments (2nd and 3rd) for D_SET_AUTOMATIC_BREAKPOINT, where the first argument is "error",
                                     // 2 arguments (2nd and 3rd) for D_SET_AUTOMATIC_BREAKPOINT, where the first argument is "fail",
                                     // 2 arguments for D_SET_GLOBAL_BATCH_FILE,
                                     // 2 arguments for D_FUNCTION_CALL_CONFIG,
                                     // + arguments for any number of D_SET_BREAKPOINT commands (optional)

#define D_ERROR                    0 // any

/** names of commands in the user interface */

#define D_SWITCH_TEXT "debug"
#define D_SET_BREAKPOINT_TEXT "dsetbp"
#define D_REMOVE_BREAKPOINT_TEXT "drembp"
#define D_SET_AUTOMATIC_BREAKPOINT_TEXT "dautobp"
#define D_SET_OUTPUT_TEXT "doutput"
#define D_SET_GLOBAL_BATCH_FILE_TEXT "dglobbatch"
#define D_FUNCTION_CALL_CONFIG_TEXT "dcallcfg"
#define D_PRINT_SETTINGS_TEXT "dsettings"
#define D_LIST_COMPONENTS_TEXT "dlistcomp"
#define D_SET_COMPONENT_TEXT "dsetcomp"
#define D_PRINT_CALL_STACK_TEXT "dprintstack"
#define D_SET_STACK_LEVEL_TEXT "dstacklevel"
#define D_LIST_VARIABLES_TEXT "dlistvar"
#define D_PRINT_VARIABLE_TEXT "dprintvar"
#define D_OVERWRITE_VARIABLE_TEXT "dsetvar"
#define D_PRINT_FUNCTION_CALLS_TEXT "dprintcalls"
#define D_STEP_OVER_TEXT "dstepover"
#define D_STEP_INTO_TEXT "dstepinto"
#define D_STEP_OUT_TEXT "dstepout"
#define D_RUN_TO_CURSOR_TEXT "drunto"
#define D_HALT_TEXT "dhalt"
#define D_CONTINUE_TEXT "dcont"
#define D_EXIT_TEXT "dexit"

/** debugger return value types */

#define DRET_NOTIFICATION   0
#define DRET_SETTING_CHANGE 1
#define DRET_DATA           2
#define DRET_EXIT_ALL       3

#endif /* DEBUGCOMMANDS_HH */

