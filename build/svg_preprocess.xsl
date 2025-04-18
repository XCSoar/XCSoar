<?xml version="1.0" standalone="no"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:svg="http://www.w3.org/2000/svg"
  xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape">

  <!-- write header to output -->
  <xsl:output
    method="xml"
    encoding="utf-8"
    doctype-public="-//W3C//DTD SVG 1.1//EN"
    doctype-system="http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd"/>


  <!-- default template: matches all elements e.g. <xsl:example>
       and all attributes within elements e.g. x="foo" -->

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <!-- svg preprocessing: match all svg elements (listed) and all
       their attributes (matched by wildcard /@* ).
       This template is called repeatedly - once for each attribute 
       matched by this search. -->

  <xsl:template match="svg:g/@*|svg:rect/@*|svg:path/@*|svg:circle/@*|
  svg:ellipse/@*|svg:line/@*|svg:polyline/@*|svg:polygon/@*|svg:text/@*">
  <!-- copy current attribute's existing value -->
   <xsl:copy-of select="."/>
  <!-- select first 5 characters of the id= and inkscape:label fields 
       of current element -->
   <xsl:variable name="IDPrefix" select="substring(../@id,1,5)"/>
   <xsl:variable name="LabelPrefix" select="substring(../@inkscape:label,1,5)"/>
  <!-- disable anti-aliasing for this element (group, path, circle etc)
       if prefix matches list in command-line argument DisableAA_Select
       eg. DisableAA_Select='NOAA_MASK_' - anti-aliasing will be disabled
       for elements with ids/labels prefixed with NOAA_ or MASK_ -->
   <xsl:if test="(contains($DisableAA_Select,$IDPrefix) and 
                 string-length($IDPrefix)=5)
                 or (contains($DisableAA_Select,$LabelPrefix) and 
                 string-length($LabelPrefix)=5)">
    <xsl:attribute name="style">
  <!-- this style attribute disables anti-aliasing in rsvg; behaviour may
       vary according to svg renderer -->
     <xsl:text>shape-rendering:optimizeSpeed;</xsl:text>
     <xsl:value-of select="../@style"/>
    </xsl:attribute>
   </xsl:if>  
  </xsl:template>
</xsl:stylesheet>
