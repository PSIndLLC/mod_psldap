<?xml version="1.0" encoding="ISO-8859-1"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/1999/xhtml'>

<xsl:include href="DSML_commonscript.xsl" />
<xsl:include href="DSML_sitefrags.xsl" />

<xsl:output method="html" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1l/DTD/transitional.dtd" omit-xml-declaration="no" media-type="text/html" />

<xsl:key name="searchEntryLookup" match="searchResultEntry" use="@dn" />

<xsl:template name="pageSpecificHeader" >
  <xsl:element name="script">
    <xsl:attribute name="language">JavaScript</xsl:attribute>
    <![CDATA[
    function initializeCardsCB() {
        var cardTable = document.getElementById("cardTable");
        var cardTd = cardTable;
        while ((null != cardTd) && (0 != cardTd.tagName.indexOf("TD"))) {
            cardTd = cardTd.firstChild;
        }
        if (null != cardTd) {
	    if (window.reservedSizeValue < 0) {
	        verticalWrapNColumns(cardTd, -1 * window.reservedSizeValue);
            } else {
	        verticalWrapChildren(cardTd, window.reservedSizeValue);
	    }
        }
        window.status = "Done";
    }

    function initializeCards(reservedSize) {
        window.status = "Organizing cards...";
        // Allow rendering to finish ... wrap after a timeout
        window.reservedSizeValue = reservedSize;
        window.setTimeout(initializeCardsCB, 20);
    }
    ]]>
  </xsl:element>
</xsl:template>

<xsl:template match="/dsml">
  <html>
    <xsl:call-template name="pageHeaderWithRefClass" >
      <xsl:with-param name="title">PsLDAP Contact Cards</xsl:with-param>
      <xsl:with-param name="ua"><xsl:value-of select='$UserAgent' /></xsl:with-param>
    </xsl:call-template>

    <xsl:element name="body">
      <xsl:if test="($isHandheld='false')">
	<xsl:attribute name="onload">initializeCards(8)</xsl:attribute>
      </xsl:if>
      <form action="CreateGroup" method="post">
	<table id='cardTable'>
	  <tr class="resultRow">
	    <xsl:apply-templates select="//searchResponse" />
	  </tr>
	</table>
      </form>
    </xsl:element>
  </html>
</xsl:template>

<xsl:template name="searchResults" match="searchResponse">
    <td class="cardColumn" >
    <xsl:variable name="recordCount" select="count(searchResultEntry)" />
    <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='organization')]/ancestor::searchResultEntry" >
        <xsl:sort select="attr[@name='o']/value" />
        <xsl:with-param name='heading'>Organization</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='organizationalUnit')]/ancestor::searchResultEntry" >
        <xsl:sort select="attr[@name='ou']/value" />
        <xsl:with-param name='heading'>Org Unit</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='groupOfUniqueNames')]/ancestor::searchResultEntry" >
        <xsl:sort select="attr[@name='cn']/value" />
        <xsl:with-param name='heading'>Groups</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='organizationalPerson')]/ancestor::searchResultEntry">
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
    <td class="label" noWrap="true" >
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
    <td class="label" noWrap="true" >
      <xsl:value-of select='$label' />
    </td>
    <td class="data">
      <xsl:for-each select='child::value'>
        <xsl:sort select='self::node()' />
	<xsl:variable name="myDN" select="self::node()/text()" />
        <span><xsl:for-each select="key('searchEntryLookup',$myDN)"><xsl:value-of select="attr[@name='cn']/value" /></xsl:for-each></span>
        <br />
        <xsl:for-each select="key('searchEntryLookup',$myDN)" >
        <xsl:for-each select="attr[@name='mail']/value[contains(text(),'@')]" >
          <xsl:sort select='.' />
          <xsl:if test="(position() = 1)">
            <xsl:element name="a">
              <xsl:attribute name="href">mailto:<xsl:value-of select="."/></xsl:attribute>
              <xsl:value-of select="."/>
            </xsl:element>
            <br />
          </xsl:if>
        </xsl:for-each>
        </xsl:for-each>
      </xsl:for-each>
    </td>
  </tr>
</xsl:template>

<xsl:template match="attr[@name='mail']">
  <xsl:param name='label' select='@name' />
  <tr>
    <td class="label" noWrap="true" >
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

<xsl:template match="attr[@name='imId' or @name='yahooId' or @name='aimId' or @name='skypeId' ]">
  <xsl:param name='label' select='@name' />
  <xsl:param name='imtype' select='@name' />
  <tr>
    <td class="label" noWrap="true" >
      <xsl:value-of select='$label' />
    </td>
    <td class="data">
      <xsl:apply-templates select="./value" mode="chat" >
	<xsl:sort select="./value" />
      </xsl:apply-templates>
    </td>
  </tr>
</xsl:template>

<xsl:template match="attr[@name='telephoneNumber' or @name='homePhone' or @name='mobile' ]" >
  <xsl:param name='label' select='@name' />
  <xsl:variable name='attrType' select='@name' />
  <tr>
    <td class="label" noWrap="true" >
      <xsl:value-of select='$label' />
    </td>
    <td class="data">
      <xsl:apply-templates select="./value" mode="dialer">
	<xsl:sort select='.' />
      </xsl:apply-templates>
    </td>
  </tr>
</xsl:template>

<xsl:template match="searchResultEntry" mode="searchResultEntryTitle">
  <xsl:choose>
    <xsl:when test="(attr[@name='sn']/value) and (attr[@name='givenName']/value)" >
      <xsl:value-of select="attr[@name='sn']/value"/>,
      <xsl:value-of select="attr[@name='givenName']/value"/>
    </xsl:when>
    <xsl:when test="(attr[@name='cn']/value)" >
      <xsl:value-of select="attr[@name='cn']/value"/>
    </xsl:when>
    <xsl:when test="(attr[@name='ou']/value)" >
      <xsl:for-each select="attr[@name='ou']/value">
        <xsl:if test="(not (position() = 1))"> - </xsl:if>
        <xsl:value-of select="."/>
      </xsl:for-each>
    </xsl:when>
    <xsl:when test="(attr[@name='o']/value)" >
      <xsl:value-of select="attr[@name='o']/value"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="searchResultEntry">
  <xsl:param name="heading" select="attr[@name='objectClass' and position() = 1]" />
  <xsl:if test="(not ($heading = '')) and (position() = 1)">
    <h2><xsl:value-of select="$heading" /></h2>
  </xsl:if>
  <table name='cardInstance' width='100%' >
    <tr class="menubar" cellspacing='0'>
      <td class="menubar_left" ><img src="/psldap/images/left_header.gif" /></td>
      <td class="menubar_center" >
	<table width="100%"><tr>
	  <td noWrap="true" >
	    <xsl:choose>
	      <xsl:when test="./attr[@name='labeledURI']">
		<xsl:element name="a">
		  <xsl:attribute name="target">_blank</xsl:attribute>
		  <xsl:attribute name="href">
		    <xsl:value-of select="attr[@name='labeledURI']/value"/>
		  </xsl:attribute>
		  <xsl:apply-templates select="." mode="searchResultEntryTitle">
		  </xsl:apply-templates>
		</xsl:element>
	      </xsl:when>
	      <xsl:otherwise>
		<xsl:apply-templates select="." mode="searchResultEntryTitle">
		</xsl:apply-templates>
	      </xsl:otherwise>
	    </xsl:choose>
	  </td>
	  <td width="72" noWrap="true" >
	    <xsl:element name="a">
	      <xsl:attribute name="href">javascript: void getEditableRecord("<xsl:value-of select="@dn"/>");</xsl:attribute>
	      <xsl:element name="img">
		<xsl:attribute name="src">/psldap/images/editRecord_sm.gif</xsl:attribute>
		<xsl:attribute name="style">margin-top: 1px; margin-bottom: 0px;</xsl:attribute>
	      </xsl:element>
	    </xsl:element>
	    <xsl:comment />
	    <xsl:element name="a">
	      <xsl:attribute name="href">javascript: void getVCard("<xsl:value-of select="@dn"/>");</xsl:attribute>
	      <xsl:element name="img">
		<xsl:attribute name="src">/psldap/images/vcard.gif</xsl:attribute>
		<xsl:attribute name="style">margin-top: 1px; margin-bottom: 0px;</xsl:attribute>
	      </xsl:element>
	    </xsl:element>
	    <xsl:comment />
	    <xsl:element name="input">
	      <xsl:attribute name="type">checkbox</xsl:attribute>
	      <xsl:attribute name="name">member</xsl:attribute>
	      <xsl:attribute name="value"><xsl:value-of select="@dn"/></xsl:attribute>
	    </xsl:element>
	  </td>
	</tr>
	</table>
      </td>
      <td class="menubar_right" ><img src="/psldap/images/right_header.gif" /></td>
    </tr>
    <tr>
      <td colspan="3">
	<table class="boxed" width="100%" >
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
	  <xsl:apply-templates select="attr[@name='imId']" >
	    <xsl:with-param name='label'>IM</xsl:with-param>
	  </xsl:apply-templates>
	  <xsl:apply-templates select="attr[@name='yahooId']" >
	    <xsl:with-param name='label'>Yahoo</xsl:with-param>
	  </xsl:apply-templates>
	  <xsl:apply-templates select="attr[@name='aimId']" >
	    <xsl:with-param name='label'>AIM</xsl:with-param>
	  </xsl:apply-templates>
	  <xsl:apply-templates select="attr[@name='skypeId']" >
	    <xsl:with-param name='label'>Skype</xsl:with-param>
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
      </td>
    </tr>
  </table>
</xsl:template>
</xsl:stylesheet>
