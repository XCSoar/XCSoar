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
	<xsl:choose>
		<xsl:when test="event/@name='Mode' and event/@misc!='default'">
			<xsl:call-template name="cont" select="."/>
		</xsl:when>
		<xsl:otherwise>
			<xsl:call-template name="one" select="."/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<xsl:template name="one">
	<li>
	<xsl:value-of select="@label"/>
	<xsl:if test="@title and @title != ''">
		<xsl:text> - </xsl:text>
		<xsl:value-of select="@title"/>
	</xsl:if>
	</li>
</xsl:template>

<xsl:template name="cont">
	<xsl:variable name="mode" select="event[@name='Mode']/@misc"/>
	<li>
	<a>
		<xsl:attribute name="href">
			<xsl:text>#mode_</xsl:text>
			<xsl:value-of select="@misc"/>
		</xsl:attribute>
		<xsl:value-of select="@label"/>
	</a>
	<xsl:if test="@title and @title != ''">
		<xsl:text> - </xsl:text>
		<xsl:value-of select="@title"/>
	</xsl:if>
	<ul>
		<xsl:apply-templates select="//input/mode[@name=$mode]/type[@name='key']/*"/>
	</ul>
	</li>
</xsl:template>


</xsl:stylesheet>

