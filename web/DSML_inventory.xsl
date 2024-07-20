<?xml version="1.0" encoding="UTF-8"?>

<!DOCTYPE stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<xsl:include href="DSML_commonscript.xsl" />
<xsl:include href="DSML_sitefrags.xsl" />

<xsl:output method="html" doctype-system="about:legacy-compat" omit-xml-declaration="no" encoding="UTF-8" indent="yes"/>

<xsl:template name="pageSpecificHeader" >
  <xsl:param name='queryURI'>/psldap/ldapupdate</xsl:param>
  <xsl:element name="script">
      <xsl:attribute name="type">text/javascript</xsl:attribute>
      <xsl:attribute name="language">JavaScript</xsl:attribute>
      <![CDATA[function synchCompareSelect() {
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
	}]]>
  </xsl:element>
  <xsl:element name="script">
    <xsl:attribute name="type">text/javascript</xsl:attribute>
    <xsl:attribute name="language">JavaScript</xsl:attribute>
    <xsl:attribute name="src">psldap_config.js</xsl:attribute>
  </xsl:element>
  <xsl:element name="script">
    <xsl:attribute name="type">text/javascript</xsl:attribute>
    <xsl:attribute name="language">JavaScript</xsl:attribute>
    <xsl:attribute name="src">DSML_psldap.js</xsl:attribute>
  </xsl:element>
  <xsl:element name="script">
    <xsl:attribute name="type">text/javascript</xsl:attribute>
    <xsl:attribute name="language">JavaScript</xsl:attribute>
    <xsl:attribute name="src">psajax_core.js</xsl:attribute>
  </xsl:element>
  <xsl:element name="script">
    <xsl:attribute name="type">text/javascript</xsl:attribute>
    <xsl:attribute name="language">JavaScript</xsl:attribute>
    <xsl:attribute name="src">psajax_docmgr.js</xsl:attribute>
  </xsl:element>
  <xsl:element name="script">
    <xsl:attribute name="type">text/javascript</xsl:attribute>
    <xsl:attribute name="language">JavaScript</xsl:attribute>
    <![CDATA[
        var assetOwners = [];
        assetOwners[0] = {dn:"cn=Alice Picard,o=ExtFamily,dc=picard,dc=us", label:"Alice Picard", defaultOwner:1};
        assetOwners[1] = {dn:"cn=David Picard,o=Family,dc=picard,dc=us", label:"David Picard", defaultOwner:0};
        function setLDAPBase(objBaseDN) {
            var searchBase = document.queryForm.dn;
            searchBase.value = objBaseDN;
        }
        function setFormBaseDN() {
            alert("Loaded window - new dn is ");
        }
        function loadDNInitializationFormUrl(xmlUrl, xslUrl, opt_subdn) {
	    var subdn = "";
	    var addArgs = "&dn=" + subdn + encodeURIComponent(document.queryForm.dn.value);
	    loadTemplateRecord(xmlUrl, xslUrl, null, addArgs, 'setFormBaseDN');
        }
        function resizeResultIframe() {
            var objIframe = document.getElementById("resultSet");
            var winHeight = getWindowHeight(window);
            var ifHeight = 1600;
            if (null != objIframe) {
                objIframe.style.height = ifHeight + "px";
            }
            if (document.body.scrollHeight > winHeight) {
                ifHeight -= document.body.scrollHeight - winHeight + 25;
                objIframe.style.height = ifHeight + "px";
            }
        }
        function setInternalTarget(bChecked) {
            document.queryForm.target = (bChecked) ? "resultSet" : "_blank";
        }
	]]>
  </xsl:element>
</xsl:template>

<xsl:template name='optionSelect'>
  <xsl:param name='selectData' />
  <xsl:param name='defaultLabel' select='*[name()=$selectData]/@default'/>
  <xsl:apply-templates select='*[name()=$selectData]/option' >
    <xsl:with-param name='defaultOption'><xsl:value-of select='$defaultLabel'/></xsl:with-param>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="/psldap">
  <html>
    <xsl:call-template name="pageHeader" >
      <xsl:with-param name="title">PsLDAP Management Page</xsl:with-param>
      <xsl:with-param name="ua"><xsl:value-of select='$UserAgent' /></xsl:with-param>
    </xsl:call-template>


    <xsl:element name="body">
      <xsl:if test="($isHandheld='false')">
	<xsl:attribute name="onload">resizeResultIframe()</xsl:attribute>
	<xsl:attribute name="onresize">resizeResultIframe()</xsl:attribute>
      </xsl:if>
      
      <!-- Add body of page here -->
      <xsl:call-template name="siteHeader" />  
      <table width="100%">
	<tr width="100%">
	  <td align="left" width="*">
	    <h2>Manage Inventory 
            <select size="1" name="dn" style="font-size: 14px; font-weight: bold; margin-left: 6px; " onchange="setLDAPBase(this.value)">
	      <script type="text/javascript" language="JavaScript" >
		<![CDATA[
	        for (var i=0; i<ldapDomains.length; i++) {
		document.write('<option ' + ((ldapDomains[i].defaultDomain==1)?"selected ":"") + 'value="' + ldapDomains[i].dn + '">' + ldapDomains[i].label + '</option>');
		}
		]]>
	      </script>
            </select>
            <select size="1" name="assetOwner" style="font-size: 14px; font-weight: bold; margin-left: 6px; " onchange="setAssetOwner(this.value)">
              <script type="text/javascript" language="JavaScript" >
                <![CDATA[
                for (var i=0; i<assetOwners.length; i++) {
                document.write('<option ' + ((assetOwners[i].defaultOwner==1)?"selected ":"") + 'value="' + assetOwners[i].dn + '">' + assetOwners[i].label + '</option>');
                }
                ]]>
              </script>
            </select>
	    <xsl:element name="span">
	      <xsl:attribute name="style">font-size: 10px; margin-left: 12px; padding-bottom: 6px;</xsl:attribute>
	      <xsl:element name="a">
		<xsl:attribute name="href"><xsl:value-of select="$v_updateURI" /><![CDATA[?FormAction=Search&search=(objectClass=*)&dn=cn%3Dsubschema&scope=base&BinaryHRef=on&xsl1=]]><xsl:value-of select="$v_baseURI" />/DSML_schema.xsl</xsl:attribute>
		<xsl:attribute name="target">_blank</xsl:attribute>
		LDAP Schemas
	      </xsl:element>
	    </xsl:element>
	  </h2>
	</td>
	<td align="right" width="5px">
	  <fieldset>
            <legend>Create a New ...</legend>
            <select size="1" name="newRecordSelect" subdn="" style="font-size: 12px; font-weight: bold; " onchange="loadDNInitializationFormUrl(psldapRootUri + '/' + this.value, psldapRootUri + '/DSML_editform.xsl'); this.value='';">
	      <xsl:call-template name="optionSelect">
		<xsl:with-param name="selectData">createas</xsl:with-param>
		<xsl:with-param name="defaultLabel">Select One ...</xsl:with-param>
	      </xsl:call-template>
            </select>
	  </fieldset>
	</td>
      </tr>
    </table>
    
    <fieldset style="margin-bottom: 6px; ">
      <legend>Search for a Record</legend>
      <xsl:element name="div">
	<xsl:attribute name="style">margin-top: 6px;</xsl:attribute>
	<xsl:element name="form">
	  <xsl:attribute name="id">queryForm</xsl:attribute>
	  <xsl:attribute name="name">queryForm</xsl:attribute>
	  <xsl:attribute name="onsubmit">generateQueryString();</xsl:attribute>
	  <xsl:attribute name="method">POST</xsl:attribute>
	  <xsl:attribute name="action"><xsl:value-of select="$v_updateURI" /></xsl:attribute>
	  <xsl:attribute name="target">resultSet</xsl:attribute>
	  <xsl:element name="script">
	    <xsl:attribute name="type">text/javascript</xsl:attribute>
	    <xsl:attribute name="language">JavaScript</xsl:attribute>
	    <![CDATA[
	     var defaultDomain = 0;
	     for (var i=0; i<ldapDomains.length; i++) {
	       if (ldapDomains[i].defaultDomain==1) { defaultDomain = i; }
	     }
	     document.write('<input type="hidden" name="dn" value="' + ldapDomains[defaultDomain].dn + '" >');
	    ]]>
	</xsl:element>
	  
	<table style=" width: 100%; ">
          <tr><td align="left">
	    <input type="hidden" name="search" size="32" maxlength="128" value="(mail=*@*)" />
	    <input type="hidden" name="BinaryHRef" size="4" maxlength="4" value="on" />
	    <input type="submit" name="FormAction" value="Search" />
	    <label>
	      <span style="margin-right: 6px; margin-left: 6px;">for</span>
	    </label>
	    <select onchange="synchCompareSelect(this);" size="1" name="searchby" style="margin-right: 3px;">
	      <xsl:call-template name="optionSelect">
		<xsl:with-param name="selectData">searchby</xsl:with-param>
	      </xsl:call-template>
	    </select>
	    <select onchange="synchCompareSelect(this);" size="1" name="searchcompare" style="margin-right: 3px;">
	      <xsl:call-template name="optionSelect">
		<xsl:with-param name="selectData">searchcompare</xsl:with-param>
	      </xsl:call-template>
	    </select>
	    <input type="text" name="searchvalue" size="32" maxlength="128" value="@" />
	    <label><span  style="margin-right: 6px; margin-left: 6px;">styled as</span>
	    <select size="1" name="xsl1" style="margin-right: 3px;">
	      <xsl:call-template name="optionSelect">
		<xsl:with-param name="selectData">styleas</xsl:with-param>
	      </xsl:call-template>
	    </select>
	    </label>
	    <label><span  style="margin-right: 6px; margin-left: 6px;">/</span>
	    <select size="1" name="xsl2" style="margin-right: 3px;">
	      <xsl:call-template name="optionSelect">
		<xsl:with-param name="selectData">styleas</xsl:with-param>
		<xsl:with-param name="defaultLabel">&lt;None&gt;</xsl:with-param>
	      </xsl:call-template>
	    </select>
	    </label>
	    </td><td>
	    <input type="reset" />
	  </td></tr>
	</table>
      </xsl:element>
    </xsl:element>
    <hr />
    <xsl:element name='label'>
      <xsl:element name='input'>
	<xsl:attribute name='type'>checkbox</xsl:attribute>
	<xsl:attribute name='name'>resultsInFrame</xsl:attribute>
	<xsl:attribute name='style'>margin-right: 5px;</xsl:attribute>
	<xsl:attribute name='checked'></xsl:attribute>
	<xsl:attribute name='onchange'>setInternalTarget(this.checked)</xsl:attribute>
      </xsl:element>
      Show results on this page (use new window if unchecked)
    </xsl:element>
    
  </fieldset>


      <iframe id="resultSet" name="resultSet" src="" frameborder="0" style="height: 0px; width: 100%; margin-top: 0px; margin-bottom: 0px; border: none; " ></iframe>

      <xsl:call-template name="siteFooter" />  
    </xsl:element>
  </html>
</xsl:template>

</xsl:stylesheet>
