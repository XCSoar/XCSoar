<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet 
	version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
>

<xsl:output method="xml" indent="yes" encoding="UTF-8"/>
<!--
<xsl:strip-space elements="*"/>   
-->

<xsl:template match="/">
	<ul>
		<xsl:apply-templates select="//input/mode[@name='default']/type[@name='key']"/>
	</ul>
</xsl:template>

<xsl:template match="entry[@label and @label!='' and @label!=' ']">
	<xsl:apply-templates select="event"/>
</xsl:template>

<xsl:template match="event[@name!='Mode']">
	<li>
	<xsl:value-of select="../@label"/>
	<xsl:if test="../@title and ../@title != ''">
		<xsl:text> - </xsl:text>
		<xsl:value-of select="../@title"/>
	</xsl:if>
	</li>
</xsl:template>

<xsl:template match="event[@name='Mode' and @misc!='default']">
	<xsl:variable name="mode" select="@misc"/>
	<li>
	<a>
		<xsl:attribute name="href">
			<xsl:text>#mode_</xsl:text>
			<xsl:value-of select="@misc"/>
		</xsl:attribute>
		<xsl:value-of select="../@label"/>
	</a>
	<xsl:if test="../@title and ../@title != ''">
		<xsl:text> - </xsl:text>
		<xsl:value-of select="../@title"/>
	</xsl:if>
	<ul>
		<xsl:apply-templates select="//input/mode[@name=$mode]/type[@name='key']/*"/>
	</ul>
	</li>
</xsl:template>


</xsl:stylesheet>

