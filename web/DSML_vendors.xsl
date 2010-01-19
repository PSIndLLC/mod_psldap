<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/REC-html40' xmlns:xs='http://www.w3.org/2001/XMLSchema'>

<xsl:import href="DSML_cards.xsl" />
<xsl:include href="DSML_sitefrags.xsl" />

<xsl:key name="searchEntryLookup" match="searchResultEntry" use="@dn" />

<xsl:template name="searchResultsCache" match="searchResponse" mode="refClass">
  <xsl:param name="refClass" />
  <xsl:param name="refClassAttr" />
  
  <script language="javascript">
    var <xsl:value-of select="$refClass" />_ref = new Array();
    <xsl:for-each select="searchResultEntry/attr[@name='objectClass']/value[(text()=$refClass)]/ancestor::searchResultEntry">
      <xsl:sort select="attr[@name=$refClassAttr]/value" />
      <xsl:value-of select="$refClass" />_ref[<xsl:value-of select="position()-1" />] = "<xsl:value-of select="attr[@name=$refClassAttr]/value" />";
    </xsl:for-each>
  </script>
  <script type="text/javascript" src="http://download.skype.com/share/skypebuttons/js/skypeCheck.js">
  </script>
</xsl:template>

<xsl:template match="/dsml">
  <html>
    <xsl:call-template name="pageHeader" />
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

<xsl:template name="servicesMgmt" >
  <script type="text/javascript" language="JavaScript">
    function synchCompareSelect() {
      var strQuery = "";
      var searchByElement = document.queryForm.searchby;
      var searchCompareElement = document.queryForm.searchcompare;
      var searchValueElement = document.queryForm.searchvalue;
      strQuery = searchByElement.options[searchByElement.selectedIndex].value;
      if ("" == strQuery) {
        // Advanced choice selected
        searchCompareElement.selectedIndex = 4;
      } else if (4 == searchCompareElement.selectedIndex) {
        searchCompareElement.selectedIndex = 3;
      }
    }
    function generateQueryString() {
      var searchElement = document.queryForm.search;
      var searchByElement = document.queryForm.searchby;
      var searchCompareElement = document.queryForm.searchcompare;
      var searchValueElement = document.queryForm.searchvalue;
      var strQuery = searchByElement.options[searchByElement.selectedIndex].value;
      strQuery = "(" + strQuery +
      searchCompareElement.options[searchCompareElement.selectedIndex].value.replace(/\?/i, searchValueElement.value) + ")";
      searchElement.value = strQuery;
      window.defaultStatus = "Getting LDAP records for filter = " + strQuery;
      return true;
    }
  </script>

  <p><a href="/family_services/zm">Zoneminder Cams</a></p>
  <p><a href="/psldap/">Picard contacts directory</a></p>
  <p><a href="javascript:void loadRecordUrl('/psldap/DSML_new_v_o.xml?dn=dc=picard,dc=us');">Add a new vendor</a></p>
  Lookup contacts in family address book:<br />
  <form id="queryForm" name="queryForm" onsubmit="return generateQueryString();" method="POST" action="/ldapupdate" target="_new">
    <table width="100%">
      <tr>
	<td align="left">
	  <input type="submit" name="FormAction" value="Search" tabindex="5" />&nbsp;
	  <select size="1" id="dn" name="dn" style="font-weight: bold; margin-left: 6px; " tabindex="1">
	    <option value="dc=psind,dc=com">PSInd, LLC</option>
	    <xsl:element name="option">
	      <xsl:attribute name="selected" />
	      <xsl:attribute name="value">dc=picard,dc=us</xsl:attribute>
	      Picard Family
	    </xsl:element>
	    <option value="dc=ssdca,dc=com">SSDCA</option>
	  </select>
	  &nbsp; for &nbsp;
	  <input type="hidden" name="search" size="32" maxlength="128" value="(mail=*@*)" />
	  <input type="hidden" name="BinaryHRef" size="4" maxlength="4" value="on" />
	  <input type="hidden" name="xsl1" size="32" maxlength="32" value="/psldap/DSML_cards.xsl" />
	  <select onchange="synchCompareSelect();" size="1" name="searchby" style="margin-right: 3px;" tabindex="2">
	    <option value="mail">eMail</option>
	    <option value="givenName">First Name</option>
	    <xsl:element name="option">
	      <xsl:attribute name="selected" />
	      <xsl:attribute name="value">sn</xsl:attribute>
	      Last Name
	    </xsl:element>
	    <option value="o">Organization</option>
	    <option value="ou">Org Unit</option>
	    <option value="">Advanced...</option>
	  </select>&nbsp; &nbsp;
	  <select onchange="synchCompareSelect();" size="1" name="searchcompare" style="margin-right: 3px;" tabindex="3">
	    <xsl:element name="option">
	      <xsl:attribute name="selected" />
	      <xsl:attribute name="value">=?*</xsl:attribute>
	      begins with
	    </xsl:element>
	    <option value="=*?*">contains</option>
	    <option value="=*?">ends with</option>
	    <option value="=?">equals</option>
	    <option value="?">...</option>
	  </select>
	  <input type="text" name="searchvalue" size="29" maxlength="128" value="@" tabindex="4" />
	</td>
      </tr>
    </table>
  </form>
  <br />
  Beacon - <xsl:element name="a"><xsl:attribute name="href">#</xsl:attribute><xsl:attribute name="onClick">window.open('family_services/Beacon'+previousMonthStr+'.pdf','Beacon')</xsl:attribute>Previous</xsl:element>, <xsl:element name="a"><xsl:attribute name="href">#</xsl:attribute><xsl:attribute name="onClick">window.open('family_services/Beacon'+currentMonthStr+'.pdf','Beacon')</xsl:attribute>Current</xsl:element>, &amp; <xsl:element name="a"><xsl:attribute name="href">#</xsl:attribute><xsl:attribute name="onClick">window.open('family_services/Beacon'+nextMonthStr+'.pdf','Beacon')</xsl:attribute>Next</xsl:element>
  <br />
  <script language="JavaScript" >
    var now = new Date();
    var current = now.getMonth() + 1;
    var previous = current - 1;
    current = now.getFullYear().toString().substring(2,4) + ((current &lt; 10)?"0":"") + current;
    if (previous &gt; 0) {
      previous = current.substring(0,2) + ((previous &lt; 10)?"0":"") + previous;
    } else {
      previous = (now.getFullYear() - 1).toString().substring(2,4) + "12";
    }
  </script>
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

  <!--
  <xsl:if test="(position() = 1) or (not ($heading = $prevResultBC))">
    <h2><xsl:value-of select="$heading" /></h2>
    <h3><xsl:value-of select="$prevResultBC" /> <xsl:value-of select="$prevResultPos" /></h3>
  </xsl:if>
-->
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
