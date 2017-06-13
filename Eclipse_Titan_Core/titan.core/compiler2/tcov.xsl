<?xml version="1.0"?>
<!--
 Copyright (c) 2000-2017 Ericsson Telecom AB
 All rights reserved. This program and the accompanying materials
 are made available under the terms of the Eclipse Public License v1.0
 which accompanies this distribution, and is available at
 http://www.eclipse.org/legal/epl-v10.html

 Contributors:
  Balasko, Jeno
  Kovacs, Ferenc
-->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                              xmlns="http://www.w3.org/1999/xhtml">
<xsl:output method="html" />
<xsl:template match="titan_coverage">
  <html>
  <head>
  <title>RAW Coverage Data</title>
  <style type="text/css">
  body, td {
    font-family: Verdana, Cursor;
    font-size: 12px;
    font-weight: normal;
  }
  table {
    border-spacing: 4px 4px;
  }
  table td {
    padding: 4px 2px 4px 2px;
    text-align: center;
  }
  table th {
    background-color: #c0c0c0;
  }
  </style>
  </head>
  <body>
  <h1>RAW Coverage Data</h1>
  <xsl:apply-templates select="version" />
  <xsl:apply-templates select="component" />
  <xsl:apply-templates select="files" />
  </body>
  </html>
</xsl:template>

<xsl:template match="version">
  <h2>Version Information</h2>
  Version Major: <xsl:value-of select="@major" /><br />
  Version Minor: <xsl:value-of select="@minor" />
</xsl:template>

<xsl:template match="component">
  <h2>Component Information</h2>
  Component Name: <xsl:value-of select="@name" /><br />
  Component Id: <xsl:value-of select="@name" />
</xsl:template>

<xsl:template match="files">
  <h2>Coverage Functions Information</h2>
  <table border="1" width="60%">
  <th>Path</th><th>Line #</th><th>Count</th>
  <xsl:for-each select="file">
    <xsl:variable name="file" select="position()" />
    <xsl:for-each select="functions">
      <xsl:for-each select="function">
        <tr>
          <td><xsl:value-of select="../../@path" /></td>
          <td><xsl:value-of select="@name" /></td>
          <td><xsl:value-of select="@count" /></td>
        </tr>
      </xsl:for-each>
    </xsl:for-each>
  </xsl:for-each>
  </table>
  <h2>Coverage Lines Information</h2>
  <table border="1" width="60%">
  <th>Path</th><th>Line #</th><th>Count</th>
  <xsl:for-each select="file">
    <xsl:variable name="file" select="position()" />
    <xsl:for-each select="lines">
      <xsl:for-each select="line">
        <tr>
          <td><xsl:value-of select="../../@path" /></td>
          <td><xsl:value-of select="@no" /></td>
          <td><xsl:value-of select="@count" /></td>
        </tr>
      </xsl:for-each>
    </xsl:for-each>
  </xsl:for-each>
  </table>
</xsl:template>

</xsl:stylesheet>
