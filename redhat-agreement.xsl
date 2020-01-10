<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:ex="http://openoffice.org/extensions/description/2006" version="1.0">

<xsl:template match="@*|node()">
	<xsl:copy>
	    <xsl:apply-templates select="@*|node()"/>
	</xsl:copy>
</xsl:template>

<xsl:template match="ex:registration"/>

</xsl:stylesheet>
