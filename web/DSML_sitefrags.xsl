<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> <!ENTITY copy "&#169;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/REC-html40'>

  <xsl:variable name="v_baseURI">/psldap</xsl:variable>
  <xsl:variable name="v_updateURI"><xsl:value-of select="$v_baseURI" />/ldapupdate</xsl:variable>
  <xsl:variable name="v_authURI"><xsl:value-of select="$v_baseURI" />/ldapauth</xsl:variable>
  <xsl:variable name="v_registerURI"><xsl:value-of select="$v_baseURI" />/register</xsl:variable>

  <xsl:template name="servicesMgmt" >
    <xsl:element name="p">
      <xsl:element name="a">
	<xsl:attribute name="href"><xsl:value-of select="$v_baseURI" />/index.html</xsl:attribute>
	Contact directory
      </xsl:element>
    </xsl:element>
    <xsl:element name="p">
      <xsl:element name="a">
	<xsl:attribute name="href">javascript:void loadTemplateRecord('<xsl:value-of select="$v_baseURI" />/DSML_new_v_o.xml', '<xsl:value-of select="$v_baseURI" />/DSML_editform.xsl', null, '&amp;dn=' + encodeURIComponent(document.queryForm.dn.value));</xsl:attribute>
	Add a new vendor
      </xsl:element>
    </xsl:element>
    Lookup contacts in address book:<br />
    <xsl:call-template name="simpleQuery" >
      <xsl:with-param name='queryURI' select='$v_updateURI' />
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="siteHeader" >
    <xsl:element name='header'>
      <xsl:element name='div'>
	<xsl:attribute name='class'>pageNav</xsl:attribute>
      </xsl:element>
    </xsl:element>
  </xsl:template>
  
  <xsl:template name="siteFooter" >
    <xsl:element name='footer'>
      <xsl:element name='div'>
	<xsl:element name='hr' />
	Copyright &copy; 2003-2024 PSInd, LLC, All Rights Reserved
      </xsl:element>
    </xsl:element>
  </xsl:template>
  
</xsl:stylesheet>
