<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML">

<xsl:key name="searchEntryLookup" match="searchResultEntry" use="@dn" />
<xsl:key name="organizationLookup" match="searchResultEntry" use="attr[@name='o']/value" />
<xsl:key name="refAttrLookup" match="searchResultEntry" use="attr[@name='manager']/value" />

<xsl:output method="html" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1l/DTD/transitional.dtd" omit-xml-declaration="no" media-type="text/html" />

<xsl:param name="refAttr" select="'manager'" />
<xsl:param name="xslManager" />

<xsl:include href="DSML_commonscript.xsl" />
<xsl:include href="DSML_sitefrags.xsl" />

<xsl:template name="pageSpecificHeader" >
  <xsl:element name="script">
    <xsl:attribute name="language">JavaScript</xsl:attribute>
    var urlBase = "<xsl:value-of select="$v_baseURI" />";
  </xsl:element>
  <xsl:element name="script">
    <xsl:attribute name="language">JavaScript</xsl:attribute>
    <xsl:attribute name="src"><xsl:value-of select="$v_baseURI" />/treemanager.js</xsl:attribute>
  </xsl:element>
  <xsl:element name="script">
    <xsl:attribute name="language">JavaScript</xsl:attribute>
        var resizeTimeOutId = 0;
        function resizeTopTable() {
            if (0 == arguments.length) {
                if (0 != resizeTimeOutId) {
                    window.clearTimeout(resizeTimeOutId);
                }
                resizeTimeOutId = window.setTimeout("resizeTimeOutId = 0; resizeTopTable(true);", 500);
            } else {
                var winHeight = getWindowHeight(window);
                var cellHeight = winHeight - 8;
                if (document.body.scrollHeight > winHeight) {
                    cellHeight -= 16;
                }
                document.getElementById("treeCell").style.height = cellHeight + "px";
                document.getElementById("editCell").style.height = cellHeight + "px";
            }
        }
  </xsl:element>
</xsl:template>

<xsl:template match="/dsml">
  <html>
    <xsl:call-template name="pageHeader" >
      <xsl:with-param name="title">PsLDAP Search Results</xsl:with-param>
    </xsl:call-template>

    <body onload="resizeTopTable(true);" onresize="resizeTopTable();" style="margin-top: 0px; margin-bottom: 0px;">
      <table width="100%" height="100%">
      <tr>
      <td id="treeCell" style="width: *; ">
        <div style="overflow: auto; height: 100%; ">
          <xsl:apply-templates select="batchResponse/searchResponse" />
        </div>
      </td>
      <td id="editCell" style="width: 568px; ">
        <iframe id="editFrame" frameborder="0" width="100%" height="100%"/>
      </td>
      </tr>
      </table>
    </body>
  </html>
</xsl:template>

<xsl:template match="searchResponse">
  <xsl:element name="table">
    <xsl:attribute name="id">orgTable</xsl:attribute>
    <xsl:attribute name="cellpadding">2</xsl:attribute>
    <xsl:attribute name="width">100%</xsl:attribute>
      <xsl:for-each select="searchResultEntry/attr[@name='objectClass']/value[text()='organizationalPerson']/ancestor::searchResultEntry">
        <xsl:sort select="attr[@name='o']/value" />
        <xsl:sort select="attr[(@name=$refAttr)]/value" />
        <xsl:sort select="attr[@name='givenName']/value" />
        <xsl:sort select="attr[@name='sn']/value" />
        <xsl:variable name="myManager" select="attr[(@name=$refAttr)]/value" />
        <xsl:if test="((@dn=$myManager) or (not (count(ancestor::searchResponse/searchResultEntry[@dn=$myManager]))))">
          <xsl:apply-templates select="." mode="refRecord">
          </xsl:apply-templates>
        </xsl:if>
      </xsl:for-each>
  </xsl:element>
  <xsl:element name="script">
    <xsl:attribute name="language">JavaScript</xsl:attribute>
    <xsl:attribute name="defer">true</xsl:attribute>
    buildOrgTree('orgTable', 'TreeNode', 'recordid', 'refattr'); 
  </xsl:element>
</xsl:template>

<xsl:template match="searchResultEntry" mode="refRecord">
  <xsl:param name="refValue"><xsl:value-of select="@dn" disable-output-escaping="yes" /></xsl:param>
  <xsl:variable name="reportCount" select="count(ancestor::searchResponse/searchResultEntry/attr[(@name=$refAttr)]/value[text()=$refValue])" />
  <xsl:element name="tr">
    <xsl:attribute name="name">TreeNode</xsl:attribute>
    <xsl:attribute name="recordid"><xsl:value-of select="@dn"/></xsl:attribute>
    <xsl:attribute name='refattr'><xsl:value-of select="attr[(@name=$refAttr)]/value"/></xsl:attribute>
    <xsl:attribute name="id"><xsl:value-of select="@dn"/></xsl:attribute>
    <xsl:element name="td" >
      <xsl:element name="img">
	<xsl:attribute name="src"><xsl:value-of select="$v_baseURI" />/images/nconnect.gif</xsl:attribute>
      </xsl:element>
    </xsl:element>
    <xsl:element name="td">
      <xsl:attribute name="width">*</xsl:attribute>
      <xsl:attribute name="objectclass"><xsl:choose>
          <xsl:when test="(attr[@name='objectClass']/value[(text()='organizationalPerson') or (text()='person') or (text()='inetOrgPerson')])" >organizationalPerson</xsl:when>
          <xsl:when test="(attr[@name='objectClass']/value[(text()='organization')])" >organization</xsl:when>
          <xsl:when test="(attr[@name='objectClass']/value[(text()='organizationalUnit')])" >organizationalUnit</xsl:when>
          <xsl:when test="(attr[@name='objectClass']/value[(text()='groupOfUniqueNames')])" >groupOfUniqueNames</xsl:when>
      </xsl:choose></xsl:attribute>
      <xsl:element name="img">
	<xsl:attribute name="class">dragHandle</xsl:attribute>
	<xsl:attribute name="src"><xsl:value-of select="$v_baseURI" />/images/transparent.gif</xsl:attribute>
	<xsl:attribute name="alt"><xsl:value-of select="@dn"/></xsl:attribute>
	<xsl:attribute name="dn"><xsl:value-of select="@dn"/></xsl:attribute>
	<xsl:attribute name="oc"><xsl:value-of select="attr[@name='structuralObjectClass']/value"/></xsl:attribute>
	<xsl:attribute name="dndCtxt">mgmtTree</xsl:attribute>
      </xsl:element>
      <xsl:element name="a">
        <xsl:attribute name="href">javascript: void getEditableRecord("<xsl:value-of select="@dn"/>", "editFrame" <xsl:if test="(not ($xslManager = ''))">, <xsl:value-of select="$xslManager" /></xsl:if>);</xsl:attribute>
	<xsl:value-of select="substring-after(substring-before(@dn,','),'=')" />
      </xsl:element>
      <xsl:if test="(not ($reportCount = '0'))">
        <br />
        <xsl:element name="table">
          <xsl:element name="tbody">
            <xsl:apply-templates select="ancestor::searchResponse/searchResultEntry[(not (@dn=$refValue))]/attr[(@name=$refAttr)]/value[(text()=$refValue)]/ancestor::searchResultEntry" mode="refRecord">
              <xsl:sort select="attr[@name='givenName']/value" />
              <xsl:sort select="attr[@name='sn']/value" />
            </xsl:apply-templates>
          </xsl:element>
        </xsl:element>
      </xsl:if>
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
