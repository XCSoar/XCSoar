<?xml version="1.0" encoding="UTF-8"?>

<!--

	Temporary convert code for the manual

	- This outputs a simple XHTML output from the manual
	- The simple XHTML is converted via a 3rd party script to FO
	- The FO is converted to PDF et al

	Longer term the converting to XHTML will be skipped as it is
	looses a lot of the information - it is only temporary.


Parameters used

	relative_path		Location of the imported document (for images etc)
	fudge			Heading depth fudge factor (XXX better name)

-->

<!DOCTYPE midoc [
	<!ENTITY nbsp "&#160;">
	<!ENTITY copy "&#169;">
	<!ENTITY reg  "&#174;">
	<!ENTITY raquo  "&#187;">
]>

<xsl:stylesheet 
	version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:exsl="http://exslt.org/common"
	extension-element-prefixes="exsl"
>

<!--
<xsl:import href="exsl.xsl" />
-->

<xsl:output
	method="xml"
/>


<xsl:template match="/">
	<html xmlns="http://www.w3.org/1999/xhtml">
	<head>
		<title>
			<xsl:value-of select="//document/metadata/title"/>
		</title>
	</head>
	<body>
		<div>
			<xsl:apply-templates select="//document">
				<xsl:with-param name="fudge" select="0"/>
			</xsl:apply-templates>
		</div>
	</body>
	</html>
</xsl:template>

<xsl:template match="object">
	<!-- XXX Deal with comments ??? -->
	<comment>Original document = <xsl:value-of select="@href"/></comment>

	<!-- Extract PATH for use in src etc -->

	<xsl:apply-templates select="exsl:node-set(document(@href)/document)">
		<xsl:with-param name="fudge" select="count(parent::section)"/>
	</xsl:apply-templates>
</xsl:template>

<xsl:template match="document">
	<xsl:param name="fudge"/>
	<xsl:apply-templates select="section">
		<xsl:with-param name="fudge" select="$fudge"/>
	</xsl:apply-templates>
</xsl:template>

<xsl:template match="section">
	<xsl:param name="fudge"/>
	<xsl:if test="title">
		<xsl:variable name="hl">
			<xsl:text>h</xsl:text>
			<xsl:value-of select="count(parent::section) + $fudge"/>
		</xsl:variable>
		<xsl:element name="{$hl}">
			<xsl:value-of select="title"/>
		</xsl:element>
	</xsl:if>
	<xsl:apply-templates>
		<xsl:with-param name="fudge" select="$fudge"/>
	</xsl:apply-templates>
</xsl:template>

<xsl:template match="screenshots">
	<div class="screenshots">
		<xsl:apply-templates/>
	</div>
</xsl:template>

<xsl:template match="img">
	<img>
		<xsl:attribute name="src">
			<xsl:value-of select="parent::document/@base"/>
			<xsl:value-of select="@src"/>
		</xsl:attribute>
	</img>
</xsl:template>

<xsl:template match="screenshot">
	<xsl:if test="@title">
		<h2><xsl:value-of select="@title"/></h2>
	</xsl:if>

	<img>
		<xsl:attribute name="src">
			<xsl:value-of select="parent::document/@base"/>
			<xsl:value-of select="@src"/>
		</xsl:attribute>
	</img>

	<p><xsl:apply-templates/></p>
</xsl:template>


<xsl:template match="@*|node()">
        <xsl:copy>
                <xsl:apply-templates select="@*|node()"/>
        </xsl:copy>
</xsl:template>

<!-- Ignore these - they should not be honored -->
<xsl:template match="h1"></xsl:template>
<xsl:template match="h2"></xsl:template>
<xsl:template match="h3"></xsl:template>
<xsl:template match="h4"></xsl:template>
<xsl:template match="title"></xsl:template>

<xsl:template match="faq">
	<!-- XXX Need to fix this hard coded h2 !!! -->
	<h2>
		<a>
			<xsl:attribute name="href">
				<xsl:value-of select="@href"/>
			</xsl:attribute>
			<xsl:value-of select="document(@faqref)/faq/title"/>
		</a>
		<xsl:text> (</xsl:text>
		<xsl:value-of select="count(document(@faqref)/faq/entry)"/>
		<xsl:text> questions)</xsl:text>
	</h2>
</xsl:template>

</xsl:stylesheet>
