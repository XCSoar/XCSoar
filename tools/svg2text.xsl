<?xml version="1.0" encoding="utf-8"?>

<xsl:stylesheet 
  xmlns:svg="http://www.w3.org/2000/svg"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">
  
  <xsl:output method="text" indent="no"/>
  
  <xsl:template match="/">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="svg:text">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="svg:text//text()">
    <xsl:value-of select="normalize-space(.)"/>
    <xsl:text>
</xsl:text>
  </xsl:template>

  <xsl:template match="text()">
  </xsl:template>

</xsl:stylesheet>

