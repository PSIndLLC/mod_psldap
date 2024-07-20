<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> <!ENTITY copy "&#169;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/REC-html40'>
  
  <xsl:param name="UserAgent">Unknown</xsl:param>
  <xsl:param name="isHandheld"><xsl:choose><xsl:when test="contains($UserAgent,'BlackBerry') or contains($UserAgent,'iPhone') or contains($UserAgent,'iPod') or contains($UserAgent,'Android')" >false</xsl:when><xsl:otherwise>false</xsl:otherwise></xsl:choose></xsl:param>

  <xsl:template name="searchResultsCache" match="searchResponse" mode="refClass">
    <xsl:param name="refClass" />
    <xsl:param name="refClassAttr" />
    
    <xsl:element name="script">
      <xsl:attribute name="type">text/javascript</xsl:attribute>
      <xsl:attribute name="language">javascript</xsl:attribute>
        var <xsl:value-of select="$refClass" />_ref = new Array();
        <xsl:for-each select="searchResultEntry/attr[@name='objectClass']/value[(text()=$refClass)]/ancestor::searchResultEntry">
	  <xsl:sort select="attr[@name=$refClassAttr]/value" />
	  <xsl:value-of select="$refClass" />_ref[<xsl:value-of select="position()-1" />] = "<xsl:value-of select="attr[@name=$refClassAttr]/value" />";
	</xsl:for-each>
    </xsl:element>
  </xsl:template>

  <xsl:template name="pageHeader">
    <xsl:param name='title' select=' "psldap" ' />
    <xsl:param name='withRefClass'>no</xsl:param>
    <xsl:element name="head">
      <xsl:element name="meta">
	<xsl:attribute name="http-equiv">Content-Type</xsl:attribute>
	<xsl:attribute name="content">text/html; charset=iso-8859-1</xsl:attribute>
      </xsl:element>
      <xsl:element name="meta">
	<xsl:attribute name="http-equiv">Content-Script-Type</xsl:attribute>
	<xsl:attribute name="content">text/javascript</xsl:attribute>
      </xsl:element>
      <xsl:element name="meta">
	<xsl:attribute name="name">generator</xsl:attribute>
	<xsl:attribute name="content">mod_psldap</xsl:attribute>
      </xsl:element>
      <xsl:element name="meta">
	<xsl:attribute name="name">Author</xsl:attribute>
	<xsl:attribute name="content">David J. Picard</xsl:attribute>
      </xsl:element>
      <xsl:element name="link">
	<xsl:attribute name="rel">STYLESHEET</xsl:attribute>
	<xsl:attribute name="type">text/css</xsl:attribute>
	<xsl:attribute name="media">all</xsl:attribute>
	<xsl:attribute name="href">/psldap/DSML_site.css</xsl:attribute>
      </xsl:element>
      <xsl:element name="link">
	<xsl:attribute name="rel">STYLESHEET</xsl:attribute>
	<xsl:attribute name="type">text/css</xsl:attribute>
	<xsl:attribute name="media">Screen,print</xsl:attribute>
	<xsl:attribute name="href">/psldap/DSML_psldap.css</xsl:attribute>
      </xsl:element>
      <xsl:element name="link">
	<xsl:attribute name="rel">STYLESHEET</xsl:attribute>
	<xsl:attribute name="type">text/css</xsl:attribute>
	<xsl:attribute name="media">Screen,print</xsl:attribute>
	<xsl:choose>
	  <xsl:when test="($isHandheld='true')">
	    <xsl:attribute name="href">/psldap/DSML_mobile.css</xsl:attribute>
	  </xsl:when>
	  <xsl:otherwise>
	    <xsl:attribute name="href">/psldap/DSML_desktop.css</xsl:attribute>
	  </xsl:otherwise>
	</xsl:choose>
      </xsl:element>
      <xsl:element name="link">
	<xsl:attribute name="rel">STYLESHEET</xsl:attribute>
	<xsl:attribute name="type">text/css</xsl:attribute>
	<xsl:attribute name="media">mobile,handheld,only screen and (max-device-width: 480px)</xsl:attribute>
	<xsl:attribute name="href">/psldap/DSML_mobile.css</xsl:attribute>
      </xsl:element>
      <xsl:element name="script">
	<xsl:attribute name="language">JavaScript</xsl:attribute>
	<xsl:choose>
	  <xsl:when test="($isHandheld='true')">
	    var isHandheld = true;
	  </xsl:when>
	  <xsl:otherwise>
	    var isHandheld = false;
	  </xsl:otherwise>
	</xsl:choose>;
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
      <xsl:element name="Title"><xsl:value-of select='$title' /></xsl:element>
      <xsl:element name="script">
	<xsl:attribute name="type">text/javascript</xsl:attribute>
	if (window.addEventListener) {
	    window.addEventListener("load", setOptionsOnAllDNSelects, true);
	} else if (window.attachEvent) {
	    window.attachEvent("onload", setOptionsOnAllDNSelects, true);
	} else {
	    window.onload = setOptionsOnAllDNSelects;
	}
      </xsl:element>
      <xsl:if test="($withRefClass='yes')">
	<xsl:apply-templates select="//searchResponse" mode="refClass">
	  <xsl:with-param name="refClass">organization</xsl:with-param>
	  <xsl:with-param name="refClassAttr">o</xsl:with-param>
	</xsl:apply-templates>
	<xsl:apply-templates select="//searchResponse" mode="refClass">
	  <xsl:with-param name="refClass">organizationalUnit</xsl:with-param>
	  <xsl:with-param name="refClassAttr">ou</xsl:with-param>
	</xsl:apply-templates>
      </xsl:if>
      <xsl:call-template name="pageSpecificHeader" />
    </xsl:element>
  </xsl:template>
  
  <xsl:template name="pageHeaderWithRefClass">
    <xsl:param name='title' />
    <xsl:call-template name="pageHeader">
      <xsl:with-param name='title' select='$title' />
      <xsl:with-param name='withRefClass' select=' "yes" ' />
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="string-replace-all">
    <xsl:param name="text" />
    <xsl:param name="replace" />
    <xsl:param name="by" />
    <xsl:choose><xsl:when test="contains($text, $replace)"><xsl:value-of select="substring-before($text,$replace)" /><xsl:value-of select="$by" /><xsl:call-template name="string-replace-all"><xsl:with-param name="text" select="substring-after($text,$replace)" /><xsl:with-param name="replace" select="$replace" /><xsl:with-param name="by" select="$by" /></xsl:call-template></xsl:when><xsl:otherwise><xsl:value-of select="$text" /></xsl:otherwise></xsl:choose>
  </xsl:template>

  <xsl:template match='option'>
    <xsl:param name='defaultOption' />
    <xsl:variable name='myLabel' select='.' />
    <xsl:element name="option">
      <xsl:attribute name="value"><xsl:value-of select="@value"/></xsl:attribute>
      <xsl:for-each select="@*">
	<xsl:variable name="myAttr" select="name()" />
	<xsl:if test="not ($myAttr = 'value')">
	  <xsl:attribute name="{$myAttr}"><xsl:value-of select="."/></xsl:attribute>
	</xsl:if>
      </xsl:for-each>
      <xsl:if test='normalize-space($myLabel) = normalize-space($defaultOption)'>
	<xsl:attribute name="selected"></xsl:attribute>
      </xsl:if>
      <xsl:value-of select='.'/>
    </xsl:element>
  </xsl:template>
  
  <xsl:template match="attr[@name='telephoneNumber' or @name='homePhone' or @name='mobile' ]/value" mode="wtai">
    <xsl:element name="a">
      <xsl:attribute name="class">phone</xsl:attribute>
      <xsl:attribute name="href">wtai://wp/mc;<xsl:if test="(not (starts-with(.,'('))) and (not (starts-with(.,'+')))">+</xsl:if><xsl:if test="(starts-with(.,'('))">+1</xsl:if><xsl:value-of select="translate(translate(translate(translate(.,' ',''),'(',''),')',''),'-','')"/></xsl:attribute>
      <xsl:value-of select="."/>
    </xsl:element>
  </xsl:template>
  
  <xsl:template match="attr[@name='telephoneNumber' or @name='homePhone' or @name='mobile' ]/value" mode="tel">
    <xsl:element name="a">
      <xsl:attribute name="class">mobile</xsl:attribute>
      <xsl:attribute name="href">tel:<xsl:if test="(not (starts-with(.,'('))) and (not (starts-with(.,'+')))">+</xsl:if><xsl:if test="(starts-with(.,'('))">+1</xsl:if><xsl:value-of select="translate(translate(translate(translate(.,' ',''),'(',''),')',''),'-','')"/></xsl:attribute>
      <xsl:value-of select="."/>
    </xsl:element>
    <xsl:text>&nbsp;</xsl:text>
    <xsl:element name="a">
      <xsl:attribute name="class">mobile</xsl:attribute>
      <xsl:attribute name="href">sms:<xsl:if test="(not (starts-with(.,'('))) and (not (starts-with(.,'+')))">+</xsl:if><xsl:if test="(starts-with(.,'('))">+1</xsl:if><xsl:value-of select="translate(translate(translate(translate(.,' ',''),'(',''),')',''),'-','')"/></xsl:attribute>
      <xsl:text>SMS</xsl:text>
    </xsl:element>
  </xsl:template>
  
  <xsl:template match="attr[@name='telephoneNumber' or @name='homePhone' or @name='mobile' ]/value" mode="skype">
    <xsl:element name="a">
      <xsl:attribute name="class">desktop</xsl:attribute>
      <xsl:attribute name="href">skype:<xsl:if test="(not (starts-with(.,'('))) and (not (starts-with(.,'+')))">+</xsl:if><xsl:if test="(starts-with(.,'('))">+1</xsl:if><xsl:value-of select="translate(translate(translate(translate(.,' ',''),'(',''),')',''),'-','')"/>?call</xsl:attribute>
      <xsl:attribute name="onclick">return skypeCheck();</xsl:attribute>
      <xsl:value-of select="."/>
    </xsl:element>
  </xsl:template>
  
  <xsl:template match="attr[@name='telephoneNumber' or @name='homePhone' or @name='mobile' ]/value" mode="dialer">
    <xsl:if test="(not (position() = 1))">
      <br />
    </xsl:if>
    <xsl:apply-templates select="." mode="skype" />
    <xsl:apply-templates select="." mode="tel" />
    <xsl:apply-templates select="." mode="wtai" />
  </xsl:template>
  
  <xsl:template match="attr[@name='imId']/value"  mode="chat-l">
      <xsl:value-of select="."/>&nbsp; ( Chat Unavailable )
  </xsl:template>
  
  <xsl:template match="attr[@name='yahooId']/value"  mode="chat-l">
      <xsl:value-of select="."/>&nbsp; ( Chat Discontinued )
  </xsl:template>
  
  <xsl:template match="attr[@name='aimId']/value" mode="chat-l">
      <xsl:value-of select="."/>&nbsp; ( Chat Discontinued )
  </xsl:template>
  
  <xsl:template match="attr[@name='skypeId']/value" mode="chat-l-old">
    <xsl:element name="script">
      <xsl:attribute name="type">text/javascript</xsl:attribute>
      <xsl:attribute name="src">http://www.skypeassets.com/i/scom/js/skype-uri.js</xsl:attribute>
    </xsl:element>
    <xsl:element name="div">
      <xsl:attribute name="id">SkypeButton_Call_<xsl:value-of select="."/>_1</xsl:attribute>
      <xsl:element name="script">
	<xsl:attribute name="type">text/javascript</xsl:attribute>
	Skype.ui({ "name": "dropdown", "element": "SkypeButton_Call_<xsl:value-of select="."/>_1", "participants": ["<xsl:value-of select="."/>"], "imageSize": 12 });
      </xsl:element>
    </xsl:element>
  </xsl:template>
  
  <xsl:template match="attr[@name='skypeId']/value" mode="chat-l">
    <xsl:element name="div">
      <xsl:attribute name="id">SkypeButton_Call_<xsl:value-of select="."/>_1</xsl:attribute>
      <xsl:element name="a">
	<xsl:attribute name="href">skype:<xsl:value-of select="."/>?chat</xsl:attribute>
	Start chat
      </xsl:element>
    </xsl:element>
  </xsl:template>
  
  <xsl:template match="attr[@name='imId' or @name='yahooId' or @name='aimId' or @name='skypeId' ]/value" mode="chat">
    <xsl:if test="(not (position() = 1))">
      <br />
    </xsl:if>
    <xsl:apply-templates select="." mode="chat-l" />
  </xsl:template>

  <xsl:template name="queryJS" >
    <xsl:element name="script">
      <xsl:attribute name="type">text/javascript</xsl:attribute>
      <xsl:attribute name="language">JavaScript</xsl:attribute>
      <![CDATA[
      function synchCompareSelect(el) {
        var strQuery = "";
        var searchByElement = document.queryForm.searchby;
        var searchCompareElement = document.queryForm.searchcompare;
        var searchValueElement = document.queryForm.searchvalue;
        strQuery = searchByElement.options[searchByElement.selectedIndex].value;
	if (el.name == "searchby") {
	  var sVal = el.options[el.selectedIndex].getAttribute("default");
	  if (null == sVal) { sVal = ""; }
	  searchValueElement.setAttribute("value", sVal);
	}
        if ("" == strQuery) {
          // Advanced choice selected
          searchCompareElement.selectedIndex = 4;
	  searchValueElement.style.display = 'inline';
        } else if (strQuery.indexOf("=") > -1) {
          // "Select all" selected
          searchCompareElement.selectedIndex = 4;
          searchValueElement.value = "";
	  searchValueElement.style.display = 'none';
        } else if (4 == searchCompareElement.selectedIndex) {
          searchCompareElement.selectedIndex = 3;
	  searchValueElement.style.display = 'inline';
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
      ]]>
    </xsl:element>
  </xsl:template>
  
  <xsl:template name="simpleQuery" >
    <xsl:param name='queryURI'>/psldap/ldapupdate</xsl:param>
    <xsl:call-template name="queryJS" />
    <xsl:element name="form" >
      <xsl:attribute name="id">queryForm</xsl:attribute>
      <xsl:attribute name="name">queryForm</xsl:attribute>
      <xsl:attribute name="onsubmit">return generateQueryString();</xsl:attribute>
      <xsl:attribute name="method">POST</xsl:attribute>
      <xsl:attribute name="action"><xsl:value-of select="$queryURI" /></xsl:attribute>
      <xsl:attribute name="target">_new_</xsl:attribute>
      <table width="100%">
	<tr>
	  <td align="left">
	    <input type="submit" name="FormAction" value="Search" tabindex="5" />&nbsp;
	    <select size="1" id="dn" name="dn" style="font-weight: bold; margin-left: 6px; " tabindex="1" />
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
    </xsl:element>
  </xsl:template>

</xsl:stylesheet>
