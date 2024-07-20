<?xml version="1.0" encoding="ISO-8859-1"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/REC-html40'>

<xsl:include href="DSML_commonscript.xsl" />
<xsl:include href="DSML_sitefrags.xsl" />

<xsl:output method="html" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1l/DTD/transitional.dtd" omit-xml-declaration="no" media-type="text/html" />

<xsl:template name="pageSpecificHeader">
  <xsl:element name="style">
    <xsl:attribute name="type">text/css</xsl:attribute>
    div#queryDiv {  z-index: 0; background-color: beige; border: 1px solid silver; margin: 0px; display: none; position: absolute; }
  </xsl:element>
  <xsl:element name="script">
    <xsl:attribute name="type">text/javascript</xsl:attribute>
    <xsl:attribute name="src"><xsl:value-of select="$v_baseURI" />/DSML_psldap.js</xsl:attribute>
  </xsl:element>
</xsl:template>

<xsl:template match="/dsml">
  <xsl:element name="html">

    <xsl:call-template name="pageHeaderWithRefClass" >
      <xsl:with-param name="title">DSML Transaction Result</xsl:with-param>
    </xsl:call-template>

    <xsl:element name="body">
      <xsl:element name="div">
	<xsl:attribute name="id">queryDiv</xsl:attribute>
      </xsl:element>
      <xsl:apply-templates select="//batchResponse" />
      <xsl:apply-templates select="//batchRequest" />
      
    </xsl:element>
  </xsl:element>
</xsl:template>

<xsl:template match="batchRequest">
  <xsl:apply-templates select="authRequest" />
  <xsl:apply-templates select="delRequest" />
  <xsl:apply-templates select="addRequest" />
  <xsl:apply-templates select="modifyRequest" />
  <xsl:apply-templates select="searchRequest" />
  <xsl:apply-templates select="modDNRequest" />
  <xsl:apply-templates select="compareRequest" />
</xsl:template>

<xsl:template match="batchResponse">
  <xsl:apply-templates select="authResponse" />
  <xsl:apply-templates select="delResponse" />
  <xsl:apply-templates select="addResponse" />
  <xsl:apply-templates select="modifyResponse" />
  <xsl:apply-templates select="searchResponse" />
  <xsl:apply-templates select="modDNResponse" />
  <xsl:apply-templates select="compareResponse" />
</xsl:template>

<xsl:template match="authRequest" >
  <xsl:element name="p">Authorization requested for principle: <xsl:value-of select="attribute::principle" /></xsl:element>
</xsl:template>

<xsl:template match="delRequest" >
  <xsl:element name="p">Deletion requested for dn="<xsl:value-of select="attribute::dn" />"</xsl:element>
</xsl:template>

<xsl:template match="addRequest" >
  <xsl:element name="p">Addition requested for dn="<xsl:value-of select="attribute::dn" />"</xsl:element>
  <xsl:element name="ul">
    <xsl:for-each select="attr" >
      <xsl:element name="li"><xsl:value-of select="attribute::name" />: <xsl:for-each select="value"><xsl:value-of select="." />;</xsl:for-each></xsl:element>
    </xsl:for-each>
  </xsl:element>
</xsl:template>

<xsl:template match="modifyRequest" >
  <xsl:element name="p">Modification requested for dn="<xsl:value-of select="@dn" />"</xsl:element>
  <xsl:element name="ul">
    <xsl:for-each select="modification" >
      <xsl:element name="li"><xsl:value-of select="@name" /> (<xsl:value-of select="@operation" />):<xsl:for-each select="value"><xsl:value-of select="." />;</xsl:for-each></xsl:element>
    </xsl:for-each>
  </xsl:element>
</xsl:template>

<xsl:template match="searchRequest" >
  <xsl:element name="p">X Requested for X</xsl:element>
</xsl:template>

<xsl:template match="modDNRequest" >
  <xsl:element name="p">Move requested for dn="<xsl:value-of select="attribute::dn" />"</xsl:element>
  <xsl:element name="ul">
    <xsl:element name="li">New RDN: <xsl:value-of select="attribute::newrdn" /></xsl:element>
    <xsl:element name="li">New Superior: <xsl:value-of select="attribute::newSuperior" /></xsl:element>
  </xsl:element>
</xsl:template>

<xsl:template match="compareRequest" >
  <xsl:element name="p">X Requested for X</xsl:element>
</xsl:template>


<xsl:template match="resultCode" >
  <xsl:if test="(not(resultCode/attribute::code='0'))">
    <xsl:element name="p">&nbsp; Error <xsl:element name="resultCode"><xsl:attribute name='code'><xsl:value-of select="./attribute::code" /></xsl:attribute><xsl:attribute name='dn'><xsl:value-of select="../../attribute::id" /></xsl:attribute><xsl:value-of select="./attribute::code" /></xsl:element>: <xsl:value-of select="../errorMessage" /></xsl:element>
  </xsl:if>
</xsl:template>

<xsl:template match="authResponse" >
  <xsl:element name="p">Authorization operation: <xsl:value-of select="errorMessage" /></xsl:element>
  <xsl:apply-templates select="resultCode" />
</xsl:template>

<xsl:template match="delResponse" >
  <xsl:element name="p">Delete operation: <xsl:value-of select="errorMessage" /></xsl:element>
  <xsl:apply-templates select="resultCode" />
</xsl:template>

<xsl:template match="addResponse" >
  <xsl:element name="p">Add operation: <xsl:value-of select="errorMessage" /></xsl:element>
  <xsl:apply-templates select="resultCode" />
</xsl:template>

<xsl:template match="modifyResponse" >
  <xsl:element name="p">Modify operation: <xsl:value-of select="errorMessage" /></xsl:element>
  <xsl:apply-templates select="resultCode" />
</xsl:template>

<xsl:template match="searchResponse" >
  <xsl:element name="p">Search operation: <xsl:value-of select="errorMessage" /></xsl:element>
  <xsl:apply-templates select="resultCode" />
</xsl:template>

<xsl:template match="modDNResponse" >
  <xsl:element name="p">DN Modification operation: <xsl:value-of select="errorMessage" /></xsl:element>
  <xsl:apply-templates select="resultCode" />
</xsl:template>

<xsl:template match="compareResponse" >
  <xsl:element name="p">Compare operation: <xsl:value-of select="errorMessage" /></xsl:element>
  <xsl:apply-templates select="resultCode" />
</xsl:template>

</xsl:stylesheet>
