<?xml version="1.0" encoding="UTF-8"?>

<!DOCTYPE midoc [
	<!ENTITY nbsp "&#160;">
	<!ENTITY copy "&#169;">
	<!ENTITY reg  "&#174;">
	<!ENTITY raquo  "&#187;">
]>

<xsl:stylesheet 
	version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
>

<xsl:template match="/faq">
	<document>
		<metadata>
			<title>
				FAQ - 
				<xsl:value-of select="title"/>
			</title>
		</metadata>

		<content>

			<h1>
				FAQ - 
				<xsl:value-of select="title"/>
			</h1>

			<xsl:apply-templates/>

		</content>

	</document>
</xsl:template>

<xsl:template match="entry">
	<xsl:apply-templates select="question"/>
	<xsl:apply-templates select="answer"/>
</xsl:template>

<xsl:template match="question">
	<h2>
		<xsl:value-of select="."/>
	</h2>
</xsl:template>

<!-- apply everything inside answers -->
<xsl:template match="answer">
	<xsl:if test=".!=''">
		<p>
			<xsl:value-of select="."/>
		</p>
	</xsl:if>
	<xsl:apply-templates/>
</xsl:template>

<xsl:template match="@*|node()">
        <xsl:copy>
                <xsl:apply-templates select="@*|node()"/>
        </xsl:copy>
</xsl:template>

</xsl:stylesheet>
