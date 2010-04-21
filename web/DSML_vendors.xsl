<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/REC-html40' xmlns:xs='http://www.w3.org/2001/XMLSchema'>

<xsl:import href="DSML_cards.xsl" />

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
    <xsl:call-template name="pageHeader" >
      <xsl:with-param name="title" >PsLDAP Vendor Cards</xsl:with-param>
    </xsl:call-template>
    <body onload="initializeCards(-3)">
      <xsl:call-template name="siteHeader" />
      <script language="JavaScript" >
	var now = new Date();
	var currentMonthStr = now.getMonth() + 1;
	var previousMonthStr = currentMonthStr - 1;
	var nextMonthStr = currentMonthStr + 1;
	currentMonthStr = now.getFullYear().toString().substring(2,4) + ((currentMonthStr &lt; 10)?"0":"") + currentMonthStr;

	if (previousMonthStr &gt; 0) {
	  previousMonthStr = currentMonthStr.substring(0,2) + ((previousMonthStr &lt; 10)?"0":"") + previousMonthStr;
	} else {
	  previousMonthStr = (now.getFullYear() - 1).toString().substring(2,4) + "12";
	}

	if (nextMonthStr &gt; 13) {
	  nextMonthStr = (now.getFullYear() + 1).toString().substring(2,4) + "01";
	} else {
	  nextMonthStr = currentMonthStr.substring(0,2) + ((nextMonthStr &lt; 10)?"0":"") + nextMonthStr;
	}
      </script>
      <script language="javascript">
	function displayChildDiv(theDiv, bShow) {
	  var j;
	  for (j = 0; j &lt; theDiv.childNodes.length; j++) {
	    var cNode = theDiv.childNodes[j];
	    if (cNode.tagName == "DIV") {
	      cNode.style.display = (bShow) ? "block" : "none";
	    }
	  }
	}
      </script>
      <table>
	<tr>
	  <td width="256px" valign="top">
	    <xsl:call-template name="servicesMgmt" />
	  </td>
	  <td>
	    <table id='cardTable'>
	      <tr class="resultRow">
		<xsl:apply-templates select="//searchResponse" mode="wHeaders" >
		  <xsl:sort select="attr[@name='businessCategory']/value" />
		</xsl:apply-templates>
	      </tr>
	    </table>
	  </td>
	</tr>
      </table>
      <xsl:call-template name="siteFooter" />
    </body>
  </html>
</xsl:template>

<xsl:key name="vendors-by-category" match="searchResultEntry" use="attr[@name='businessCategory']" />
<xsl:key name="vendors-by-vclass" match="searchResultEntry" use="attr[@name='vendorClass']" />

<xsl:template name="vendorList" match="searchResponse" mode="wHeaders">
    <td class="cardColumn" >
    <xsl:for-each select="searchResultEntry[count(. | key('vendors-by-vclass', attr[@name='vendorClass'])[1])=1]">
      <xsl:sort select="attr[@name='businessCategory']" />
      <xsl:sort select="attr[@name='vendorClass']" />

      <h3><xsl:value-of select="attr[@name='businessCategory']" />: <xsl:value-of select="attr[@name='vendorClass']" /></h3>

      <xsl:for-each select="key('vendors-by-vclass', attr[@name='vendorClass'])" >
	<xsl:sort select="attr[@name='o']/value" />
	<xsl:sort select="attr[@name='ou']/value" />
	<xsl:apply-templates select="." />
      </xsl:for-each>
    </xsl:for-each>
    </td>
</xsl:template>

<xsl:template name="vendorList_old" match="searchResponse" mode="busCatSort">
    <td class="cardColumn" >
    <xsl:variable name="recordCount" select="count(searchResultEntry)" />
    <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='psVendorAcctObject')]/ancestor::searchResultEntry" >
      <xsl:sort select="attr[@name='businessCategory']/value" />
      <xsl:sort select="attr[@name='vendorClass']/value" />
      <xsl:sort select="attr[@name='o']/value" />
      <xsl:sort select="attr[@name='ou']/value" />
    </xsl:apply-templates>
    </td>
</xsl:template>

<xsl:template match="searchResultEntry">
  <xsl:param name="heading" select="attr[@name='businessCategory']/value" />
  <xsl:param name="prevResultPos" select="position()-1" />
  <xsl:param name="prevResultBC" select="ancestor::searchResponse/searchResultEntry[position() = $prevResultPos]/attr[@name='businessCategory']/value" />

  <xsl:element name="div">
    <xsl:attribute name="name">vendorSummary</xsl:attribute>
    <xsl:attribute name="style">background-color: cyan; border-width: 1px; border-style: solid; border-color: blue; margin: 3px; padding: 2px; -moz-border-radius: 5px; -webkit-border-radius: 5px; behavior:url(border-radius.htc); width: 256px;</xsl:attribute>
    <xsl:attribute name="onmouseover">displayChildDiv(this, true)</xsl:attribute>
    <xsl:attribute name="onmouseout">displayChildDiv(this, false)</xsl:attribute>
    <xsl:element name="a">
      <xsl:attribute name="style">padding-right: 10px;</xsl:attribute>
      <xsl:attribute name="href">javascript: void getEditableRecord("<xsl:value-of select="@dn"/>");</xsl:attribute>
      <xsl:element name="img">
	<xsl:attribute name="src">/psldap/images/editRecord_sm.gif</xsl:attribute>
	<xsl:attribute name="style">margin-top: 1px; margin-bottom: 0px;</xsl:attribute>
      </xsl:element>
    </xsl:element>
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
    
    <xsl:element name="br"></xsl:element>
    
    <xsl:element name="div">
      <xsl:attribute name="style">padding-left: 10px; display: none;</xsl:attribute>
      <xsl:if test="./attr[@name='acctNo']">
	Account #: <xsl:value-of select="attr[@name='acctNo']/value"/>
	<xsl:element name="br"></xsl:element>
      </xsl:if>
      <xsl:if test="./attr[@name='loginCred']">
	Credentials: <xsl:value-of select="attr[@name='loginCred']/value"/>
	<xsl:element name="br"></xsl:element>
      </xsl:if>
      <xsl:if test="./attr[@name='telephoneNumber']">
	Phone: <xsl:value-of select="attr[@name='telephoneNumber']/value"/>
	<xsl:element name="br"></xsl:element>
      </xsl:if>
      
    </xsl:element>
  </xsl:element>
</xsl:template>

</xsl:stylesheet>
