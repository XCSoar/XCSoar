<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet 
	version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
>

<xsl:output method="xml" indent="yes" encoding="UTF-8"/>

<xsl:template match="/">
	<ul>
		<xsl:apply-templates select="//input/mode[@name='default']/type[@name='key']"/>
	</ul>
</xsl:template>

<xsl:template match="entry">
	<li>
		<xsl:value-of select="@data"/>
		<xsl:text> - </xsl:text>
		<xsl:value-of select="@title"/>
	</li>
</xsl:template>

</xsl:stylesheet>

