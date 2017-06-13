<!--
Copyright (c) 2010 Mia-Software
All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
which accompanies this distribution, and is available at
http://www.eclipse.org/legal/epl-v10.html

Contributors:
Gregoire Dupe - initial implementation
Stephan Herrmann - adaptation for Object Teams
Ferenc Kovacs - adaptation for Titan, Ericsson AB
-->
<xsl:stylesheet xmlns:xsl='http://www.w3.org/1999/XSL/Transform' version="1.0">
  <xsl:output encoding="UTF-8" method="xml" indent="yes" />
  <xsl:strip-space elements="*" />
  <!-- Not used. -->
  <xsl:param name="repo" value="http://ttcn.ericsson.se/download/eclipse_stats" />

  <xsl:template match="/">
    <xsl:processing-instruction name="artifactRepository">version='1.1.0'</xsl:processing-instruction>
    <xsl:apply-templates />
  </xsl:template>

  <xsl:template match="repository/properties">
    <properties size='{@size+1}'>
      <xsl:copy-of select="property" />
      <property name='p2.statsURI'>
        <xsl:attribute name="value">http://ttcn.ericsson.se/download/eclipse_stats</xsl:attribute>
      </property>
    </properties>
  </xsl:template>

  <xsl:template match="artifact[@classifier='org.eclipse.update.feature' and @id='TITAN_Designer']/properties">
    <xsl:call-template name="artifact_properties" />
  </xsl:template>

  <xsl:template match="artifact[@classifier='org.eclipse.update.feature' and @id='TITAN_Executor']/properties">
    <xsl:call-template name="artifact_properties" />
  </xsl:template>

  <xsl:template match="artifact[@classifier='org.eclipse.update.feature' and @id='TITAN_Log_Viewer']/properties">
    <xsl:call-template name="artifact_properties" />
  </xsl:template>

  <xsl:template name="artifact_properties">
    <properties size='{@size+1}'>
      <xsl:copy-of select="property" />
      <property name='download.stats'>
        <xsl:attribute name="value"><xsl:copy-of select='string(../@id)' />.<xsl:value-of select='substring-before(../@version, "CRL")' />eclipsestats.php</xsl:attribute>
      </property>
    </properties>
  </xsl:template>

  <xsl:template match="*">
    <xsl:copy>
      <xsl:for-each select="@*"><xsl:copy-of select="." /></xsl:for-each>
      <xsl:apply-templates />
    </xsl:copy>
  </xsl:template>

</xsl:stylesheet>
