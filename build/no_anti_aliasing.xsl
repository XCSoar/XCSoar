<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:svg="http://www.w3.org/2000/svg">

  <!-- copy all elements and all attributes -->
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <!-- .. but add "shape-rendering:optimizeSpeed" to these elements
  -->
  <xsl:template match="svg:path/@style">
      <xsl:attribute name="style">
        <xsl:text>shape-rendering:optimizeSpeed;</xsl:text>
        <xsl:value-of select="."/>
      </xsl:attribute>
  </xsl:template>

</xsl:stylesheet>
