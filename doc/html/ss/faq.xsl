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

			<section>
				<title>
					FAQ - 
					<xsl:value-of select="title"/>
				</title>

				<xsl:apply-templates/>

			</section>

		</content>

	</document>
</xsl:template>

<xsl:template match="entry">
	<section>
	<xsl:apply-templates select="question"/>
	<xsl:apply-templates select="answer"/>
	</section>
</xsl:template>

<xsl:template match="question">
	<title>
		<xsl:value-of select="."/>
	</title>
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
