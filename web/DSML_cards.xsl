<?xml version="1.0"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/REC-html40'>
<xsl:template match="/">
  <html>
   <head>
    <link rel="STYLESHEET" type="text/css" media="screen" href="/psldap/DSML_psldap.css" />
    <title>Search Results</title>
   </head>
   <body>
    <form action="CreateGroup" method="post">
     <table><tr>
       <xsl:apply-templates select="//searchResponse" />
     </tr></table>
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

<xsl:template name="searchResults" match="searchResponse">
    <td valign='top'>
    <xsl:apply-templates select="searchResultEntry[starts-with(@dn,'o=')]">
        <xsl:sort select="attr[@name='o']/value" />
        <xsl:with-param name='heading'>Organization</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="searchResultEntry[starts-with(@dn,'ou=')]">
        <xsl:sort select="attr[@name='ou']/value" />
        <xsl:with-param name='heading'>Org Unit</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="searchResultEntry[starts-with(@dn,'cn=')]">
        <xsl:sort select="attr[@name='sn']/value" />
        <xsl:sort select="attr[@name='givenName']/value" />
        <xsl:sort select="attr[@name='cn']/value" />
        <xsl:with-param name='heading'>People</xsl:with-param>
    </xsl:apply-templates>
    </td>
</xsl:template>

<xsl:template match="attr">
  <xsl:param name='label' select='@name' />
  <xsl:variable name='attrType' select='@name' />
  <tr>
    <td class="label">
      <xsl:value-of select='$label' />
    </td>
    <td class="data">
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
    </td>
  </tr>
</xsl:template>

<xsl:template match="attr[@name='uniqueMember']">
  <xsl:param name='label' select='@name' />
  <tr>
    <td class="label">
      <xsl:value-of select='$label' />
    </td>
    <td class="data">
      <xsl:for-each select='./value'>
        <xsl:sort select='.' />
	<xsl:variable name="myDN" select="." /> 
        <xsl:for-each select="ancestor::searchResponse/searchResultEntry[@dn=$myDN]//attr[@name='mail']/value[contains(text(),'@')]" >
          <xsl:sort select='.' />
          <xsl:if test="(not (position() = 1))">
            <br />
          </xsl:if>
          <xsl:element name="a">
            <xsl:attribute name="href">mailto:<xsl:value-of select="."/></xsl:attribute>
            <xsl:value-of select="."/>
          </xsl:element>
        </xsl:for-each>
      </xsl:for-each>
    </td>
  </tr>
</xsl:template>

<xsl:template match="attr[@name='mail']">
  <xsl:param name='label' select='@name' />
  <tr>
    <td class="label">
      <xsl:value-of select='$label' />
    </td>
    <td class="data">
      <xsl:for-each select="./value[contains(text(),'@')]">
        <xsl:sort select="." />
        <xsl:if test="(not (position() = 1))">
          <br />
        </xsl:if>
        <xsl:element name="a">
          <xsl:attribute name="href">mailto:<xsl:value-of select="."/></xsl:attribute>
          <xsl:value-of select="."/>
        </xsl:element>
      </xsl:for-each>
    </td>
  </tr>
</xsl:template>

<xsl:template match="searchResultEntry">
  <xsl:param name="heading" select="attr[@name='objectClass' and position() = 1]" />
  <xsl:if test="(not ($heading = '')) and (position() = 1)">
    <h2><xsl:value-of select="$heading" /></h2>
  </xsl:if>
  <table width='100%' border='0' cellspacing='0'>
    <tr><td>
      <table width='100%' cellspacing='0'><tr>
        <td class="menubar_left" width="20" />
        <td class="menubar">
          <table width="100%"><tr>
            <td>
              <xsl:element name="a">
                <xsl:if test="./attr[@name='labeledURI']">
                <xsl:attribute name="target">_blank</xsl:attribute>
                <xsl:attribute name="href">
                  <xsl:value-of select="attr[@name='labeledURI']/value"/>
                </xsl:attribute>
                </xsl:if>
                <xsl:element name='font'>
                  <xsl:attribute name='class'>menubar</xsl:attribute>
                  <xsl:choose>
                    <xsl:when test="(attr[@name='sn']/value) and (attr[@name='givenName']/value)" >
                      <xsl:value-of select="attr[@name='sn']/value"/>,
                      <xsl:value-of select="attr[@name='givenName']/value"/>
                    </xsl:when>
                    <xsl:when test="(attr[@name='cn']/value)" >
                      <xsl:value-of select="attr[@name='cn']/value"/>,
                    </xsl:when>
                    <xsl:when test="(attr[@name='o']/value)" >
                      <xsl:value-of select="attr[@name='o']/value"/>
                    </xsl:when>
                    <xsl:when test="(attr[@name='ou']/value)" >
                      <xsl:value-of select="attr[@name='ou']/value"/>
                    </xsl:when>
                  </xsl:choose>
                </xsl:element>
              </xsl:element>
            </td>
            <td width="16">
              <xsl:element name="input">
                <xsl:attribute name="type">checkbox</xsl:attribute>
                <xsl:attribute name="name">member</xsl:attribute>
                <xsl:attribute name="value"><xsl:value-of select="@dn"/></xsl:attribute>
              </xsl:element>
            </td>
          </tr></table>
        </td>
        <td class="menubar_right" width="20" />
      </tr></table>
    </td></tr>
    <tr><td>
      <table width='100%' class='boxed'>
        <xsl:if test="(not (starts-with(@dn, 'o=')))" >
          <xsl:apply-templates select="attr[@name='o']" >
	    <xsl:with-param name='label'>Organization</xsl:with-param>
          </xsl:apply-templates>
        </xsl:if>
        <xsl:if test="(not (starts-with(@dn, 'ou=')))" >
          <xsl:apply-templates select="attr[@name='ou']" >
	    <xsl:with-param name='label'>Org Unit</xsl:with-param>
          </xsl:apply-templates>
        </xsl:if>
        <xsl:apply-templates select="attr[@name='mail']" >
          <xsl:with-param name='label'>e-Mail</xsl:with-param>
        </xsl:apply-templates>
        <xsl:apply-templates select="attr[@name='telephoneNumber']" >
	  <xsl:with-param name='label'>Work Phone</xsl:with-param>
        </xsl:apply-templates>
        <xsl:apply-templates select="attr[@name='postalAddress']" >
	  <xsl:with-param name='label'>Work Addr</xsl:with-param>
        </xsl:apply-templates>
        <xsl:apply-templates select="attr[@name='facsimileTelephoneNumber']" >
	  <xsl:with-param name='label'>Work Fax</xsl:with-param>
        </xsl:apply-templates>
        <xsl:apply-templates select="attr[@name='homePhone']" >
	  <xsl:with-param name='label'>Home Phone</xsl:with-param>
        </xsl:apply-templates>
        <xsl:apply-templates select="attr[@name='homePostalAddress']" >
	  <xsl:with-param name='label'>Home Addr</xsl:with-param>
        </xsl:apply-templates>
        <xsl:apply-templates select="attr[@name='mobile']" >
	  <xsl:with-param name='label'>Cell Phone</xsl:with-param>
        </xsl:apply-templates>
        <xsl:apply-templates select="attr[@name='uniqueMember']" >
	  <xsl:with-param name='label'>Members</xsl:with-param>
        </xsl:apply-templates>
      </table>
    </td></tr></table><br />
  </xsl:template>
</xsl:stylesheet>
