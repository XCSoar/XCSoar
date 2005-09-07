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

<xsl:template match="/">
	<html>
		<head>
			<title>
				XCSoar - <xsl:value-of select="/document/metadata/title"/>
			</title>
			<link href="/style.css" rel="stylesheet" type="text/css"/>
		</head>
		<body>
		<div id="page">

			<div id="header">
				<img
					align="middle"
					src="/logo-small.gif"
					border="0"
				/>
				<h1>
					<a href="/">
						XCSoar - <xsl:value-of select="/document/metadata/title"/>
					</a>
				</h1>
			</div>

			<div id="navigation">
			      <ul>
				      <li><a href="/">Home</a></li>
				      <li><a href="/about/">About XCSoar</a></li>
				      <li><a href="/download/">Download</a></li>
				      <li><a href="/quick/">Quick start</a></li>
				      <li><a href="/infoboxes/">Infoboxes</a></li>
				      <li><a href="/settings/">Configuration settings</a></li>
				      <li><a href="/readme.html">Version 4 Readme</a></li>
				      <li><a href="/faq/">FAQ</a></li>
				      <li><a href="/links.html">Links</a></li>
				      <li><a href="/advanced/">Advanced</a></li>
				      <li><a href="/ref/">Reference Manual</a></li>
				</ul>
			</div>
			    
			<div id="content">
				<xsl:apply-templates select="//document/content/*"/>
			</div>

			<!-- XXX Should be copyright, fix later -->
			<div id="footer">
				<a 
					href="http://sourceforge.net/projects/xcsoar"
				>
					<img
						src="http://sourceforge.net/sflogo.php?group_id=141663&amp;type=3"
						width="125" 
						height="37" 
						border="0"
						alt="SourceForge.net Logo" 
						align="left"
					/>
				</a>

				XCSoar is Free Software released under the GNU/GPL License.
				<br/>
				Copyright 2005
			</div>

		</div>
		</body>
	</html>
</xsl:template>

<xsl:template match="screenshots">
	<div class="screenshots">
		<xsl:apply-templates/>
	</div>
</xsl:template>

<xsl:template match="screenshot">
	<xsl:if test="@title">
		<h2><xsl:value-of select="@title"/></h2>
	</xsl:if>

	<img>
		<xsl:attribute name="src">
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

</xsl:stylesheet>
