<?xml version="1.0" encoding="UTF-8"?>

<!DOCTYPE midoc [
	<!ENTITY nbsp "&#160;">
	<!ENTITY copy "&#169;">
	<!ENTITY reg  "&#174;">
	<!ENTITY raquo  "&#187;">
	<!ENTITY tree SYSTEM "../input/tree.xml">
]>

<xsl:stylesheet 
	version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
>

<!--
<xsl:output method="html" indent="yes" encoding="UTF-8"/>
<xsl:strip-space elements="*"/>   
-->

<xsl:template match="/input">
	<document>
		<metadata>
			<title>Input and Events File</title>
		</metadata>

		<content>

<div class="input">

<p>
This document describes the input and events in the
selected input file. Start at <a href="#mode_default">default</a> to view the
purpose and mapping of each button on the organiser, and what happens during
automatic glide computer events. Click on Event Names to take you to a
description of the purpose and options for an event. Click on Mode names (eg:
Menu1) to jump to the next mode.
</p>

<h2>Tree view</h2>
&tree;

<!--
		<h2>Index of Modes and Event Types</h2>
			<ul>
				<xsl:for-each select="mode">
					<li>
							<a>
								<xsl:attribute name="href">
									<xsl:text>#mode_</xsl:text>
									<xsl:value-of select="@name"/>
								</xsl:attribute>
								<xsl:value-of select="@name"/>
							</a>
						<xsl:text> - </xsl:text>
							<xsl:for-each select="type">
								<a>
									<xsl:attribute name="href">
										<xsl:text>#mode_</xsl:text>
										<xsl:value-of select="parent::mode/@name"/>
										<xsl:text>_type_</xsl:text>
										<xsl:value-of select="@name"/>
									</xsl:attribute>
									<xsl:value-of select="@name"/>
								</a>
								<xsl:text> </xsl:text>
							</xsl:for-each>
					</li>
				</xsl:for-each>
			</ul>
-->

			<xsl:apply-templates/>

</div>

		</content>

	</document>
</xsl:template>

<xsl:template match="input">
	<xsl:apply-templates/>
</xsl:template>

<xsl:template match="mode">
	<a>
		<xsl:attribute name="name">
			<xsl:text>mode_</xsl:text>
			<xsl:value-of select="@name"/>
		</xsl:attribute>
	</a>
	<h1>MODE = <xsl:value-of select="@name"/></h1>
	<xsl:apply-templates>
		<xsl:with-param name="mode" select="@name"/>
	</xsl:apply-templates>
</xsl:template>

<xsl:template match="type">
	<xsl:param name="mode"/>
	<a>
		<xsl:attribute name="name">
			<xsl:text>mode_</xsl:text>
			<xsl:value-of select="$mode"/>
			<xsl:text>_type_</xsl:text>
			<xsl:value-of select="@name"/>
		</xsl:attribute>
	</a>
	<h2>
		Input type = <xsl:value-of select="@name"/>
		<xsl:if test="@name = 'gce'">
			- Glide Computer Event Trigger
		</xsl:if>
		<xsl:if test="@name = 'key'">
			- Keyboard (or hardware) button input
		</xsl:if>
	</h2>
	<table>
		<tr>
			<th>Button/Trigger</th>
			<th>Title</th>
			<th>Label</th>
			<th>Events</th>
		</tr>
		<xsl:apply-templates/>
	</table>
</xsl:template>

<xsl:template match="entry">
	<tr>
		<td>
			<xsl:choose>
				<xsl:when test="parent::type/@name='gce'">
				<a>
					<xsl:attribute name="href">
						<xsl:text>/ref/gce/</xsl:text>
						<xsl:value-of select="@data"/>
						<xsl:text>/</xsl:text>
					</xsl:attribute>
					<xsl:value-of select="@data"/>
				</a>
				</xsl:when>
				<xsl:otherwise>
					<xsl:value-of select="@data"/>
				</xsl:otherwise>
			</xsl:choose>
		</td>
		<td>
			<xsl:value-of select="@title"/>
		</td>
		<td>
			<xsl:if test="@label != ''">
				<xsl:value-of select="@location"/>
				<xsl:text>: "</xsl:text>
				<xsl:value-of select="@label"/>
				<xsl:text>"</xsl:text>
			</xsl:if>
		</td>
		<td>
			<xsl:apply-templates/>
		</td>
	</tr>
</xsl:template>

<xsl:template match="event">
	<li>
		<a>
			<xsl:attribute name="href">
				<xsl:text>/ref/events/</xsl:text>
				<xsl:value-of select="@name"/>
				<xsl:text>/</xsl:text>
			</xsl:attribute>
			<xsl:value-of select="@name"/>
		</a>
		<xsl:text> </xsl:text>
		<xsl:choose>
			<xsl:when test="@name = 'Mode'">
				<a>
					<xsl:attribute name="href">
						<xsl:text>#mode_</xsl:text>
						<xsl:value-of select="@misc"/>
						<!--
						<xsl:text>_type_key</xsl:text>
						-->
					</xsl:attribute>
					<xsl:value-of select="@misc"/>
				</a>
			</xsl:when>
			<xsl:otherwise>
				<xsl:value-of select="@misc"/>
			</xsl:otherwise>
		</xsl:choose>
	</li>
</xsl:template>


<!--
<xsl:template match="@*|node()">
        <xsl:copy>
                <xsl:apply-templates select="@*|node()"/>
        </xsl:copy>
</xsl:template>
-->

</xsl:stylesheet>
