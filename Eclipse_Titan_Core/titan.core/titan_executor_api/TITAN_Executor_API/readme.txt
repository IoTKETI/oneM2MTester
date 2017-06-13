###############################################################################
# Copyright (c) 2000-2017 Ericsson Telecom AB
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the Eclipse Public License v1.0
# which accompanies this distribution, and is available at
# http://www.eclipse.org/legal/epl-v10.html
#
# Contributors:
#   Balasko, Jeno
#   Lovassy, Arpad
#
###############################################################################
Javadoc can be generated with this command:
ant -f javadoc.xml

build.xml and javadoc.xml are generated from Eclipse.
The generated build.xml is modified manually: eclipse dependent targets were removed.

Steps to generate build.xml from Eclipse:
  1. Right click on TITAN_Executor_API -> Export...
  2. Select General/Ant Buildfiles

Steps to generate javadoc.xml from Eclipse:
  1. Right click on TITAN_Executor_API -> Export...
  2. Select Java/Javadoc
  3. Next
  4. Make sure, that "Javadoc command" field contains a javadoc executable from a JDK.
      If a JDK is set in Window/Preferences/Java/Installed JREs as a JRE, this field is automatically filled,
      otherwise a javadoc executable must be selected manually.
  5. Select "Use standard doclet" (default) -> <workspace>/TITAN_Executor_API/doc (default)
  6. Next
  7. Switch on "Document title", type "TITAN Executor API Javadoc"
  8. Next
  9. Switch on "Overview" -> <workspace>/TITAN_Executor_API/doc/javadoc-overview/javadoc-overview.html
 10. Switch on "Save the settings of this Javadoc export as an Ant script" -> <workspace>/TITAN_Executor_API/javadoc.xml (default)
 11. Finish

