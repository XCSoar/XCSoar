<?xml version="1.0" encoding="UTF-8"?>

<!--

	Temporary convert code for the manual

	- This outputs a simple XHTML output from the manual
	- The simple XHTML is converted via a 3rd party script to FO
	- The FO is converted to PDF et al

	Longer term the converting to XHTML will be skipped as it is
	looses a lot of the information - it is only temporary.

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
		<frontpage>
			<xsl:apply-templates select="document/frontpage"/>
		</frontpage>

		<div>
			<xsl:apply-templates select="document"/>
		</div>
	</body>
	</html>
</xsl:template>

<!--
XXX Not working - see manual2temp.xsl !
	It does mean that the "fudge" factor is no longer required !
<xsl:template match="object">
	<comment>Original document = <xsl:value-of select="@href"/></comment>
	<xsl:apply-templates select="exsl:node-set(document(@href)/document)">
		<xsl:with-param name="fudge" select="count(ancestor::section)"/>
	</xsl:apply-templates>
</xsl:template>
-->

<xsl:template match="document">
	<xsl:apply-templates select="section"/>
</xsl:template>

<xsl:template match="section">
	<xsl:if test="title">
		<xsl:variable name="hl">
			<xsl:text>h</xsl:text>
			<xsl:value-of select="count(ancestor::section) + 1"/>
		</xsl:variable>
		<xsl:element name="{$hl}">
			<!-- The BROKEN heading counter - h1 works, but h2 does not 
				* h1 & h2 fail some times due to the fact that
				  the section levels may be inside a document - 
				  which should be ingored
				* h2 is not picking up the correct first part heading 

			<xsl:choose>
				<xsl:when test="$hl = 'h1'">
					<xsl:call-template name="section-count" select="."/>
					<xsl:text>: </xsl:text>
				</xsl:when>
				<xsl:when test="$hl = 'h2'">
					<xsl:call-template name="section-count" select=".."/>
					<xsl:text>.</xsl:text>
					<xsl:call-template name="section-count" select="."/>
					<xsl:text>: </xsl:text>
				</xsl:when>
			</xsl:choose>
			 -->
			<xsl:value-of select="title"/>
		</xsl:element>
	</xsl:if>

	<!-- Experiment to captue the funny section with no paragraph 
	-->
	<p>
		<xsl:choose>
			<xsl:when test="title">
				<xsl:value-of select="normalize-space(text()[2])"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="normalize-space(text()[1])"/>
			</xsl:otherwise>
		</xsl:choose>
	</p>
	<xsl:apply-templates/>
</xsl:template>

<!-- Remove ALL text from a section - note that the necessary text is copied in
	section template above
-->
<xsl:template match="section/text()"/>

<xsl:template name="section-count">
	<xsl:value-of select="count(preceding-sibling::section) + 1"/>
</xsl:template>

<xsl:template match="screenshots">
	<div class="screenshots">
		<xsl:apply-templates/>
	</div>
</xsl:template>

<xsl:template match="img">
	<img>
		<xsl:attribute name="src">
			<!-- XXX This is not working !!! -->
			<xsl:if test="not(substring-before(ancestor::document/@base, ':') = 'http')">
				<xsl:value-of select="ancestor::document/@base"/>
			</xsl:if>
			<xsl:value-of select="@src"/>
		</xsl:attribute>
	</img>
</xsl:template>

<xsl:template match="screenshot">
	<xsl:if test="@title">
		<!-- XXX Need to fix this hard coded h4 !!! -->
		<h4><xsl:value-of select="@title"/></h4>
	</xsl:if>

	<img>
		<xsl:attribute name="src">
			<xsl:value-of select="ancestor::document/@base"/>
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

<!-- Ignore these - they should not be honored (XXX current done in document2xhtml) -->
<xsl:template match="h1"></xsl:template>
<xsl:template match="h2"></xsl:template>
<xsl:template match="h3"></xsl:template>
<xsl:template match="h4"></xsl:template>
<xsl:template match="title"></xsl:template>

<xsl:template match="faqXXX">
	<!-- XXX Need to fix this hard coded h4 !!! -->

	<!-- XXX This section not really required here - as we have the
		information locally we do not need to load the documents -->

	<test1><xsl:value-of select="ancestor::document/@base"/></test1>
	<xsl:variable name="faqref">
		<xsl:value-of select="ancestor::document/@base"/>
		<xsl:value-of select="@faqref"/>
	</xsl:variable>
	<h4>
		<a>
			<xsl:attribute name="href">
				<xsl:value-of select="@href"/>
			</xsl:attribute>
			<xsl:value-of select="document($faqref)/faq/title"/>
		</a>
		<xsl:text> (</xsl:text>
		<xsl:value-of select="count(document($faqref)/faq/entry)"/>
		<xsl:text> questions)</xsl:text>
	</h4>
</xsl:template>

</xsl:stylesheet>
