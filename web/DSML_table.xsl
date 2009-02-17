<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML">
<xsl:template match="/">
  <html>
    <head>
      <link rel="STYLESHEET" type="text/css" media="screen" href="/psldap/DSML_psldap.css" />
      <link rel="STYLESHEET" type="text/css" media="print" href="/psldap/DSML_psldap.css" />
      <xsl:element name="script">
        <xsl:attribute name="language">JavaScript</xsl:attribute>
        <xsl:attribute name="src">/psldap/DSML_psldap.js</xsl:attribute>
      </xsl:element>
      <title>Search Results</title>
    </head>
    <body>
      <form action="CreateGroup" method="post">
        <input type="button" value="Print" onclick="window.print();" />
        <br />
        <table id="directoryTable" border="1" cellpadding="2" width="100%">
          <tr>
            <th>Edit</th>
            <th>Name</th>
            <th>Address</th>
            <th>E-Mail</th>
            <th>Work Phone</th>
            <th>Home Phone</th>
            <th>Fax</th>
            <th>Update</th>
          </tr>
          <xsl:for-each select="dsml/batchResponse/searchResponse/searchResultEntry">
            <xsl:sort select="attr[@name='sn']/value" />
            <xsl:sort select="attr[@name='givenName']/value" />
            <tr>
              <td>
                <xsl:element name="a">
                  <xsl:attribute name="href">javascript: void getEditableRecord("<xsl:value-of select="@dn"/>");</xsl:attribute>
                  <xsl:element name="img">
                    <xsl:attribute name="src">/psldap/images/editRecord_sm.gif</xsl:attribute>
                    <xsl:attribute name="style">margin-left: 8px; margin-top: 0px; margin-bottom: 0px;</xsl:attribute>
                  </xsl:element>
                </xsl:element>
              </td>
              <td>
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
              </td>
              <td>
                <xsl:choose>
                  <xsl:when test="(attr[@name='homePostalAddress'])" >
                    <xsl:apply-templates select="attr[@name='homePostalAddress']" />
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:apply-templates select="attr[@name='postalAddress']" />
                  </xsl:otherwise>
                </xsl:choose>
              </td>
              <td>
                <xsl:apply-templates select="attr[@name='mail']" />
              </td>
              <td>
                <xsl:apply-templates select="attr[@name='telephoneNumber']" />
              </td>
              <td>
                <xsl:apply-templates select="attr[@name='homePhone']" />
              </td>
              <td>
                <xsl:apply-templates select="attr[@name='facsimileTelephoneNumber']" />
              </td>
              <td>
                <xsl:element name="input">
                  <xsl:attribute name="type">checkbox</xsl:attribute>
                  <xsl:attribute name="name">member</xsl:attribute>
                  <xsl:attribute name="value"><xsl:value-of select="@dn"/></xsl:attribute>
                </xsl:element>
              </td>
            </tr>
          </xsl:for-each>
        </table>
        <hr />
        <table>
          <tr>
            <td>
              Group Name:
              <input type="text" size="20" name="GroupName"/>
            </td>
            <td>
              <input type="submit" value="Submit"/>
            </td>
            <td>
              <input type="reset" value="Reset"/>
            </td>
          </tr>
        </table>
      </form>
    </body>
  </html>
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
