<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/REC-html40'>

  <xsl:include href="DSML_commonscript.xsl" />
  <xsl:include href="DSML_sitefrags.xsl" />

  <xsl:template match="/dsml/batchResponse">
  <html>
    <xsl:element name="head">
      <xsl:element name="link">
        <xsl:attribute name="rel">STYLESHEET</xsl:attribute>
        <xsl:attribute name="type">text/css</xsl:attribute>
        <xsl:attribute name="media">screen</xsl:attribute>
        <xsl:attribute name="href"><xsl:value-of select="$v_baseURI" />/DSML_psldap.css</xsl:attribute>
      </xsl:element>
      <xsl:element name="link">
        <xsl:attribute name="rel">STYLESHEET</xsl:attribute>
        <xsl:attribute name="type">text/css</xsl:attribute>
        <xsl:attribute name="media">Screen</xsl:attribute>
        <xsl:attribute name="href"><xsl:value-of select="$v_baseURI" />/DSML_desktop.css</xsl:attribute>
      </xsl:element>
      <xsl:element name="link">
        <xsl:attribute name="rel">STYLESHEET</xsl:attribute>
        <xsl:attribute name="type">text/css</xsl:attribute>
        <xsl:attribute name="media">mobile</xsl:attribute>
        <xsl:attribute name="href"><xsl:value-of select="$v_baseURI" />/DSML_mobile.css</xsl:attribute>
      </xsl:element>
      <xsl:element name="link">
        <xsl:attribute name="rel">STYLESHEET</xsl:attribute>
        <xsl:attribute name="type">text/css</xsl:attribute>
        <xsl:attribute name="media">print</xsl:attribute>
        <xsl:attribute name="href"><xsl:value-of select="$v_baseURI" />/DSML_psldap.css</xsl:attribute>
      </xsl:element>
      <xsl:element name="script">
	<xsl:attribute name="type">text/javascript</xsl:attribute>
	<xsl:attribute name="language">JavaScript</xsl:attribute>
	<xsl:attribute name="src"><xsl:value-of select="$v_baseURI" />/psldap_config.js</xsl:attribute>
      </xsl:element>
      <xsl:element name="script">
        <xsl:attribute name="language">JavaScript</xsl:attribute>
        <xsl:attribute name="src"><xsl:value-of select="$v_baseURI" />/DSML_psldap.js</xsl:attribute>
      </xsl:element>
      <xsl:element name="title">Search Results</xsl:element>
    </xsl:element>
    <body>
      <form action="CreateGroup" method="post">
        <table width="100%">
          <tr>
	    <td width="*">
              <input type="button" value="Print" onclick="window.print();" />
	    </td>
            <td width="288px">
              Group Name:
              <input type="text" size="20" name="GroupName"/>
	      &nbsp;
              <input type="submit" value="Submit"/>
            </td>
            <td width="80px" align="right">
              <input type="reset" value="Reset"/>
            </td>
          </tr>
        </table>

	<xsl:apply-templates />
      </form>
    </body>
  </html>
</xsl:template>

<xsl:template match="searchResponse">
  <xsl:element name="hr" />
  <xsl:element name="table">
    <xsl:attribute name="id">directoryTable</xsl:attribute>
    <xsl:attribute name="border">1</xsl:attribute>
    <xsl:attribute name="cellpadding">2</xsl:attribute>
    <xsl:attribute name="width">100%</xsl:attribute>
    <xsl:element name="tr">
      <xsl:element name="th">Edit</xsl:element>
      <xsl:element name="th">Name</xsl:element>
      <xsl:element name="th">Address</xsl:element>
      <xsl:element name="th">E-Mail</xsl:element>
      <xsl:element name="th">Work Phone</xsl:element>
      <xsl:element name="th">Home Phone</xsl:element>
      <xsl:element name="th">Fax</xsl:element>
      <xsl:element name="th">Update</xsl:element>
    </xsl:element>
    <xsl:apply-templates>
      <xsl:sort select="attr[@name='sn']/value" />
      <xsl:sort select="attr[@name='givenName']/value" />
    </xsl:apply-templates>
  </xsl:element>
  <xsl:element name="hr" />
</xsl:template>

<xsl:template match="searchResultDone">
</xsl:template>

<xsl:template match="searchResultEntry">
  <xsl:element name="tr">
    <xsl:element name="td">
      <xsl:element name="a">
        <xsl:attribute name="href">javascript: void getEditableRecord("<xsl:value-of select="@dn"/>");</xsl:attribute>
        <xsl:element name="img">
          <xsl:attribute name="src"><xsl:value-of select="$v_baseURI" />/images/editRecord_sm.gif</xsl:attribute>
          <xsl:attribute name="style">margin-left: 8px; margin-top: 0px; margin-bottom: 0px;</xsl:attribute>
        </xsl:element>
      </xsl:element>
    </xsl:element>
    <xsl:element name="td">
      <xsl:choose>
        <xsl:when test="(attr[@name='cn'])" >
          <xsl:apply-templates select="attr[@name='cn']" />
        </xsl:when>
        <xsl:when test="(attr[@name='sn'])" >
          <xsl:apply-templates select="attr[@name='sn']" />, <xsl:apply-templates select="attr[@name='givenName']" />
        </xsl:when>
        <xsl:when test="(attr[@name='ou'])" >
          <xsl:apply-templates select="attr[@name='ou']" />
        </xsl:when>
        <xsl:when test="(attr[@name='o'])" >
          <xsl:apply-templates select="attr[@name='o']" />
        </xsl:when>
      </xsl:choose>
    </xsl:element>
    <xsl:element name="td">
      <xsl:choose>
        <xsl:when test="(attr[@name='homePostalAddress'])" >
          <xsl:apply-templates select="attr[@name='homePostalAddress']" />
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="attr[@name='postalAddress']" />
        </xsl:otherwise>
      </xsl:choose>
    </xsl:element>
    <xsl:element name="td">
      <xsl:apply-templates select="attr[@name='mail']" />
    </xsl:element>
    <xsl:element name="td">
      <xsl:apply-templates select="attr[@name='telephoneNumber']" />
    </xsl:element>
    <xsl:element name="td">
      <xsl:apply-templates select="attr[@name='homePhone']" />
    </xsl:element>
    <xsl:element name="td">
      <xsl:apply-templates select="attr[@name='facsimileTelephoneNumber']" />
    </xsl:element>
    <xsl:element name="td">
      <xsl:element name="input">
        <xsl:attribute name="type">checkbox</xsl:attribute>
        <xsl:attribute name="name">member</xsl:attribute>
        <xsl:attribute name="value"><xsl:value-of select="@dn"/></xsl:attribute>
      </xsl:element>
    </xsl:element>
    </xsl:element>
</xsl:template>

<xsl:template match="attr[@name='mail']">
  <xsl:for-each select="./value[contains(text(),'@')]">
    <xsl:sort select='.' />
    <xsl:if test="(not (position() = 1))">
      <br />
    </xsl:if>
    <xsl:element name="a">
      <xsl:attribute name="href">mailto:<xsl:value-of select="."/></xsl:attribute>
      <xsl:value-of select="."/>
    </xsl:element>
  </xsl:for-each>
</xsl:template>

<xsl:template match="attr">
  <xsl:for-each select='./value'>
    <xsl:sort select='.' />
    <xsl:if test="(not (position() = 1))">
      <br />
    </xsl:if>
    <xsl:choose>
      <xsl:when test="(ancestor::attr[contains(@name,'Address')])" >
        <xsl:element name='pre'>
          <xsl:value-of select='.' />
        </xsl:element>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select='.' />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:for-each>
</xsl:template>


</xsl:stylesheet>
