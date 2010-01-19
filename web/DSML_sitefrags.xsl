<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> <!ENTITY copy "&#169;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/REC-html40'>

<xsl:template name="pageHeader">
  <xsl:element name="head">
    <xsl:element name="meta">
      <xsl:attribute name="http-equiv">Content-Type</xsl:attribute>
      <xsl:attribute name="content">text/html; charset=iso-8859-1</xsl:attribute>
    </xsl:element>
    <xsl:element name="meta">
      <xsl:attribute name="name">Author</xsl:attribute>
      <xsl:attribute name="content">David J. Picard</xsl:attribute>
    </xsl:element>
    <xsl:element name="title">psldap</xsl:element>
    <xsl:element name="link">
      <xsl:attribute name="rel">STYLESHEET</xsl:attribute>
      <xsl:attribute name="type">text/css</xsl:attribute>
      <xsl:attribute name="href">/psldap/DSML_site.css</xsl:attribute>
    </xsl:element>
    <xsl:element name="link">
      <xsl:attribute name="rel">STYLESHEET</xsl:attribute>
      <xsl:attribute name="type">text/css</xsl:attribute>
      <xsl:attribute name="media">screen</xsl:attribute>
      <xsl:attribute name="href">/psldap/DSML_psldap.css</xsl:attribute>
    </xsl:element>
    <xsl:element name="script">
      <xsl:attribute name="type">text/javascript</xsl:attribute>
      <xsl:attribute name="language">JavaScript</xsl:attribute>
      <xsl:attribute name="src">/psldap/psldap_config.js</xsl:attribute>
    </xsl:element>
    <xsl:element name="script">
      <xsl:attribute name="type">text/javascript</xsl:attribute>
      <xsl:attribute name="language">JavaScript</xsl:attribute>
      <xsl:attribute name="src">/psldap/DSML_psldap.js</xsl:attribute>
    </xsl:element>
  </xsl:element>
</xsl:template>

<xsl:template name="siteHeader" >
  <div class="pageNav">
  <table cellspacing='0' width='100%'>
    <tr align='right'>
      <td width='150px' align='right'>
        <img src='/psldap/logo.jpg' alt="PSLDAP"/> </a>
      </td>
      <td align='right' cellpadding='0px' valign='center' margin='0px'>
        <a class='headerTitle' href='/psldap/index.html'>PSLDAP</a>
      </td>
    </tr>
  </table>

  <table border='0' cellspacing='0' height='30px' width='100%'>
    <tr align='center'>
      <td width='43%'>&nbsp;</td>
      <td width='10%'><a href='/psldap/index.html'>X</a></td>
      <td width='10%'><a href='/psldap/index.html'>X</a></td>
      <td width='10%'><a href='/psldap/index.html'>X</a></td>
      <td width='15%'><a href='/psldap/index.html'>X</a></td>
      <td width='12%'><a href='/psldap/index.html'>psldap</a></td>
    </tr>
  </table>
  </div>
</xsl:template>

<xsl:template name="siteFooter" >
  <div class="pageNav">
    <hr />
    Copyright &copy; 2010 PSInd, LLC  All Rights Reserved
  </div>
</xsl:template>

</xsl:stylesheet>
