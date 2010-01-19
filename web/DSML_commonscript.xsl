<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/REC-html40'>
  
  <xsl:template name="searchResults" match="searchResponse" mode="refClass">
    <xsl:param name="refClass" />
    <xsl:param name="refClassAttr" />
    
    <script language="javascript">
      <xsl:value-of select="$refClass" />_ref = new Array();
      <xsl:for-each select="searchResultEntry/attr[@name='objectClass']/value[(text()=$refClass)]/ancestor::searchResultEntry">
	<xsl:sort select="attr[@name=$refClassAttr]/value" />
	<xsl:value-of select="$refClass" />_ref[<xsl:value-of select="position()-1" />] = "<xsl:value-of select="attr[@name=$refClassAttr]/value" />";
      </xsl:for-each>
      alert("RefClasses loaded: " + <xsl:value-of select="count(searchResultEntry/attr[@name='objectClass']/value[(text()=$refClass)]/ancestor::searchResultEntry)" /> );
    </script>
  </xsl:template>

</xsl:stylesheet>

