<xsl:stylesheet version="1.0" encoding="UTF-8"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:office="http://openoffice.org/2000/office"
	xmlns:style="http://openoffice.org/2000/style"
	xmlns:table="http://openoffice.org/2000/table"
	xmlns:draw="http://openoffice.org/2000/drawing"
	xmlns:fo="http://www.w3.org/1999/XSL/Format"
	xmlns:xlink="http://www.w3.org/1999/xlink"
	xmlns:dc="http://purl.org/dc/elements/1.1/"
	xmlns:meta="http://openoffice.org/2000/meta"
	xmlns:number="http://openoffice.org/2000/datastyle"
	xmlns:svg="http://www.w3.org/2000/svg"
	xmlns:chart="http://openoffice.org/2000/chart"
	xmlns:help="http://openoffice.org/2000/help"
	xmlns:index="http://sun.com/2000/XMLSearch"	
	xmlns:text="http://openoffice.org/2000/text">

<xsl:param name="Language" select="'en-US'"/>
<xsl:output method="text" encoding="UTF-8"/>

<xsl:template match="helpdocument|body">
  <xsl:choose>
    <xsl:when test="meta/topic[@indexer='exclude']"/>
    <xsl:otherwise>
      <xsl:apply-templates/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="title">
  <xsl:value-of select="."/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="table">
  <xsl:apply-templates/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="tablecell">
  <xsl:apply-templates/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="tablerow">
  <xsl:apply-templates/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="list">
  <xsl:apply-templates/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="listitem">
  <xsl:apply-templates/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="item">
  <xsl:apply-templates/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="emph">
  <xsl:apply-templates/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="paragraph">
  <xsl:value-of select="."/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="section">
  <xsl:apply-templates/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="bookmark">
  <xsl:apply-templates/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="bookmark_value">
  <xsl:apply-templates/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="link">
  <xsl:apply-templates/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="ahelp[@visibility='visible']">
  <xsl:value-of select="."/>
  <xsl:text>&#xA;</xsl:text>
</xsl:template>

<xsl:template match="*"/>

</xsl:stylesheet>


