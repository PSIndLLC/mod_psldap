<?xml version="1.0"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML">
<xsl:template match="/">
  <html>
    <head>
      <link rel="STYLESHEET" type="text/css" media="screen" href="/psldap/DSML_psldap.css" />
      <title>Search Results</title>
    </head>
    <body>
      <form action="CreateGroup" method="post">
        <table border="1" cellpadding="2" width="100%">
          <tr>
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
              <td class="data">
                <xsl:apply-templates select="attr[@name='cn']" />
              </td>
              <td class="data">
                <xsl:apply-templates select="attr[@name='homePostalAddress']" />
              </td>
              <td class="data">
                <xsl:apply-templates select="attr[@name='mail']" />
              </td>
              <td class="data">
                <xsl:apply-templates select="attr[@name='telephoneNumber']" />
              </td>
              <td class="data">
                <xsl:apply-templates select="attr[@name='homePhone']" />
              </td>
              <td class="data">
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
