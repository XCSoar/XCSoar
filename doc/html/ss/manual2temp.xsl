<?xml version="1.0" encoding="UTF-8"?>

<!--

	Temporary Exapsnion...


	Found some bugs in my code that stopped the expand working in one step.
	I should be able to convert Manual.xml -> Manual.fo in one step.
	For now I am using 3 steps

	1. Expand the object to one big tree - prevents problems with things like "count(parent::section)"
	2. Convert to XHTML - this is temporary until I combine with stage 3 (better)
	3. Convert to FO - 3rd party code (good starting point)

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

<xsl:output
	method="xml"
/>


<xsl:template match="object">
	<!-- XXX Deal with comments ??? -->
	<comment>Original document = <xsl:value-of select="@href"/></comment>

	<!-- Extract PATH for use in src etc - OR see example in img/@src -->

	<!-- NOTE - if object is inside another nodeset this will NOT work -->

	<xsl:apply-templates select="exsl:node-set(document(@href)/document)"/>
</xsl:template>

<!-- WE DON'T LIKE THESE - get rid of em -->
<xsl:template match="h1"></xsl:template>
<xsl:template match="h2"></xsl:template>
<xsl:template match="h3"></xsl:template>
<xsl:template match="h4"></xsl:template>

<xsl:template match="@*|node()">
        <xsl:copy>
                <xsl:apply-templates select="@*|node()"/>
        </xsl:copy>
</xsl:template>

</xsl:stylesheet>
