<?xml version="1.0" encoding="ISO-8859-1"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/html4/loose.dtd'>

<xsl:include href="DSML_sitefrags.xsl" />

<xsl:output method="html" doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN" doctype-system="http://www.w3.org/TR/html4/loose.dtd" omit-xml-declaration="no" media-type="text/html" />

<xsl:key name="searchEntryLookup" match="searchResultEntry" use="@dn" />

<xsl:variable name="psldapRoot" select="'/psldap'" />

<xsl:template name="pageSpecificHeader" >
  <xsl:element name="script">
    <xsl:attribute name="type">text/javascript</xsl:attribute>
    <xsl:attribute name="language">javascript</xsl:attribute>
    function setDefaultAddress(ta)
    {
      if (ta.value == "") {
        var objForm = ta;
        while ((undefined == objForm.tagName) ||
               (objForm.tagName.toUpperCase() != "FORM") ) {
          objForm = objForm.parentNode;
        }
        ta.value = objForm.elements["street-1"].value + "\n" + objForm.elements["l-1"].value + ", " + objForm.elements["st-1"].value + " " + objForm.elements["postalCode-1"].value;
      }
    }
  </xsl:element>
  <xsl:element name="style">
    <xsl:attribute name="type">text/css</xsl:attribute>
    div#processDiv {  z-index: 0; background-color: beige; border: 1px solid silver; margin: 0px; display: none; position: absolute; left: 48px; width: 384px; }
  </xsl:element>
</xsl:template>

<xsl:template match="/dsml">
  <xsl:variable name="recordCount" select="count(//searchResponse/searchResultEntry)" />
  <xsl:processing-instruction name="xml"><xsl:text>version="1.0" standalone="yes"</xsl:text></xsl:processing-instruction>
  <xsl:text>
</xsl:text>
  <xsl:element name="html">
    <xsl:call-template name="pageHeaderWithRefClass" >
      <xsl:with-param name='title'>Edit LDAP Records</xsl:with-param>
    </xsl:call-template>
    
    <xsl:element name="body">
      <xsl:attribute name="onload">initialize()</xsl:attribute>
      
      <xsl:element name="div">
	<xsl:attribute name="id">processDiv</xsl:attribute>
	<xsl:element name="a">
	  <xsl:attribute name="href">javascript: void showProcessDocument(false);</xsl:attribute>
	  Hide results<xsl:element name="br" />
	</xsl:element>
	<xsl:element name="iframe">
	  <xsl:attribute name="id">processWindow</xsl:attribute>
	  <xsl:attribute name="name">processWindow</xsl:attribute>
	  <xsl:attribute name="src"><xsl:value-of select="$psldapRoot" />/statusPage.html</xsl:attribute>
	  <xsl:attribute name="cleanSrc"><xsl:value-of select="$psldapRoot" />/statusPage.html</xsl:attribute>
	  <xsl:attribute name="width">100%</xsl:attribute>
	  <xsl:attribute name="style">border: none</xsl:attribute>
	</xsl:element>
      </xsl:element>
      <xsl:element name="br" />
      <xsl:element name="table">
	<xsl:attribute name="width">100%</xsl:attribute>
        <xsl:element name="tr">
	  <xsl:attribute name="class">controlRow</xsl:attribute>
	  <xsl:element name="td">
	    <xsl:attribute name="align">center</xsl:attribute>
	    <xsl:attribute name="width">50px</xsl:attribute>
	    
	    <xsl:call-template name="recordcontrols">
	      <xsl:with-param name='recordCount'><xsl:value-of select="$recordCount" /></xsl:with-param>
	    </xsl:call-template>
	  </xsl:element>
          <xsl:element name="td">
	    <xsl:attribute name="id">editableRecords</xsl:attribute>
	    <xsl:apply-templates select="//searchResponse" />
	  </xsl:element>
        </xsl:element>
      </xsl:element>
    </xsl:element>
  </xsl:element>
</xsl:template>

<xsl:template name="searchResults" match="searchResponse">
  <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='organization')]/ancestor::searchResultEntry">
    <xsl:with-param name='hidden'>on</xsl:with-param>
    <xsl:sort select="attr[@name='o']/value" />
  </xsl:apply-templates>
  
  <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='organizationalUnit')]/ancestor::searchResultEntry">
    <xsl:with-param name='hidden'>on</xsl:with-param>
    <xsl:sort select="attr[@name='ou']/value" />
  </xsl:apply-templates>
  
  <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='organizationalPerson') or (text()='person') or (text()='inetOrgPerson')]/ancestor::searchResultEntry">
    <xsl:with-param name='hidden'>on</xsl:with-param>
    <xsl:sort select="attr[@name='sn']/value" />
    <xsl:sort select="attr[@name='givenName']/value" />
    <xsl:sort select="attr[@name='cn']/value" />
  </xsl:apply-templates>
  
  <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='groupOfUniqueNames')]/ancestor::searchResultEntry">
    <xsl:with-param name='hidden'>on</xsl:with-param>
    <xsl:sort select="attr[@name='sn']/value" />
    <xsl:sort select="attr[@name='givenName']/value" />
    <xsl:sort select="attr[@name='cn']/value" />
  </xsl:apply-templates>
</xsl:template>

<xsl:template name="recordcontrols">
  <xsl:param name="recordCount" select="1" />
  <xsl:element name="div">
    <xsl:attribute name="id">editNavControls</xsl:attribute>
    <xsl:choose>
      <xsl:when test="(//searchResponse/searchResultEntry[@dn='']/attribute::dn='')">
	<xsl:element name="span">
	  <xsl:element name="a">
	    <xsl:attribute name="href">javascript: void submitVisibleRecord('Create')</xsl:attribute>
	    <xsl:attribute name="title">Create a new record</xsl:attribute>
	    <xsl:element name="img">
	      <xsl:attribute name="src"><xsl:value-of select="$psldapRoot" />/images/newRecord.gif</xsl:attribute>
	    </xsl:element>
	  </xsl:element>
	</xsl:element>
      </xsl:when>
      <xsl:otherwise>
	<xsl:element name="span">
	  <xsl:element name="a">
	    <xsl:attribute name="href">javascript: void submitVisibleRecord('Modify')</xsl:attribute>
	    <xsl:attribute name="title">Modify this record</xsl:attribute>
	    <xsl:element name="img">
	      <xsl:attribute name="src"><xsl:value-of select="$psldapRoot" />/images/editRecord.gif</xsl:attribute>
	    </xsl:element>
	  </xsl:element>
	</xsl:element>
	<xsl:element name="span">
	  <xsl:element name="a">
	    <xsl:attribute name="href">javascript: void moveVisibleRecord('modDNRequest')</xsl:attribute>
	    <xsl:attribute name="title">Move this record</xsl:attribute>
	    <xsl:element name="img">
	      <xsl:attribute name="src"><xsl:value-of select="$psldapRoot" />/images/moveRecord.gif</xsl:attribute>
	    </xsl:element>
	  </xsl:element>
	</xsl:element>
	<xsl:element name="span">
	  <xsl:element name="a">
	    <xsl:attribute name="href">javascript: void submitVisibleRecord('Delete')</xsl:attribute>
	    <xsl:attribute name="title">Delete this record</xsl:attribute>
	    <xsl:element name="img">
	      <xsl:attribute name="src"><xsl:value-of select="$psldapRoot" />/images/delRecord.gif</xsl:attribute>
	    </xsl:element>
	  </xsl:element>
	</xsl:element>
      </xsl:otherwise>
    </xsl:choose>
    
    <xsl:element name="span">
      <xsl:element name="a">
	<xsl:attribute name="href">javascript: void resetVisibleRecord()</xsl:attribute>
	<xsl:attribute name="title">Reset changes on current form</xsl:attribute>
	<xsl:element name="img">
	  <xsl:attribute name="src"><xsl:value-of select="$psldapRoot" />/images/resetRec.gif</xsl:attribute>
	</xsl:element>
      </xsl:element>
    </xsl:element>
    
    <xsl:element name="span">
      <xsl:element name="a">
	<xsl:attribute name="href">javascript: void toggleClassInfo()</xsl:attribute>
	<xsl:attribute name="title">Change record classification</xsl:attribute>
	<xsl:element name="img">
	  <xsl:attribute name="src"><xsl:value-of select="$psldapRoot" />/images/showClass.gif</xsl:attribute>
	</xsl:element>
      </xsl:element>
    </xsl:element>
      
    <xsl:if test="$recordCount &gt; 1">
      <xsl:element name="span">
	<xsl:element name="a">
	  <xsl:attribute name="onmouseup">javascript: void showPrevRecord()</xsl:attribute>
	  <xsl:attribute name="title">Go to previous form</xsl:attribute>
	  <img src="/psldap/images/prevRecord.gif" />
	</xsl:element>
      </xsl:element>
      <xsl:element name="span">
	<xsl:element name="input">
	  <xsl:attribute name="type">text</xsl:attribute>
	  <xsl:attribute name="id">recordNumber</xsl:attribute>
	  <xsl:attribute name="size">
	    <xsl:choose>
	      <xsl:when test="($recordCount &lt; 10)" >1</xsl:when>
	      <xsl:when test="($recordCount &lt; 100)" >2</xsl:when>
	      <xsl:otherwise>3</xsl:otherwise>
	    </xsl:choose>
	  </xsl:attribute>
	  <xsl:attribute name="maxlength">
	    <xsl:choose>
	      <xsl:when test="($recordCount &lt; 10)" >1</xsl:when>
	      <xsl:when test="($recordCount &lt; 100)" >2</xsl:when>
	      <xsl:otherwise>6</xsl:otherwise>
	    </xsl:choose>
	  </xsl:attribute>
	  <xsl:attribute name="value">1</xsl:attribute>
	  <xsl:attribute name="onKeyPress">if (event.keyCode == 13) showRecord(this.value)</xsl:attribute>
	  <xsl:attribute name="title">record # of <xsl:number value="$recordCount" /> records</xsl:attribute>
	</xsl:element>
      </xsl:element>
      <xsl:element name="span">
	<xsl:element name="a">
	  <xsl:attribute name="href">javascript: void showNextRecord()</xsl:attribute>
	  <xsl:attribute name="title">Go to next form</xsl:attribute>
	  <img src="/psldap/images/nextRecord.gif" />
	</xsl:element>
      </xsl:element>
    </xsl:if>
  </xsl:element>
</xsl:template>

<xsl:template name="textarea">
  <xsl:param name="rows" select="4" />
  <xsl:param name="cols" select="32" />
  <xsl:param name="focus" />
  <xsl:param name='attrType' select='@name' />

  <xsl:element name="textarea">
    <xsl:attribute name="name"><xsl:value-of select='$attrType' /><xsl:number value="position()" format="-1" /></xsl:attribute>
    <xsl:attribute name="rows"><xsl:value-of select='$rows' /></xsl:attribute>
    <xsl:attribute name="cols"><xsl:value-of select='$cols' /></xsl:attribute>
    <xsl:if test="(not($focus=''))">
      <xsl:attribute name="onfocus"><xsl:value-of select='$focus' /></xsl:attribute>
    </xsl:if>
    <xsl:value-of select='value' /><xsl:text />
  </xsl:element>
</xsl:template>

<xsl:template match="searchResultEntry">
  <xsl:param name="hidden" select="off" />
  <xsl:apply-templates select="attr[@name='objectClass']/value[(text()='organization')]/ancestor::searchResultEntry" mode="organization">
    <xsl:with-param name='hidden'><xsl:value-of select="$hidden" /></xsl:with-param>
    <xsl:with-param name='heading'>Organization</xsl:with-param>
  </xsl:apply-templates>

  <xsl:apply-templates select="attr[@name='objectClass']/value[(text()='organizationalUnit')]/ancestor::searchResultEntry" mode="organizationalUnit">
    <xsl:with-param name='hidden'><xsl:value-of select="$hidden" /></xsl:with-param>
    <xsl:with-param name='heading'>Unit</xsl:with-param>
  </xsl:apply-templates>

  <xsl:apply-templates select="attr[@name='objectClass']/value[(text()='organizationalPerson') or (text()='person') or (text()='inetOrgPerson')]/ancestor::searchResultEntry" mode="organizationalPerson">
    <xsl:with-param name='hidden'><xsl:value-of select="$hidden" /></xsl:with-param>
    <xsl:with-param name='heading'>People</xsl:with-param>
  </xsl:apply-templates>

  <xsl:apply-templates select="attr[@name='objectClass']/value[(text()='groupOfUniqueNames')]/ancestor::searchResultEntry" mode="groupOfUniqueNames">
    <xsl:with-param name='hidden'><xsl:value-of select="$hidden" /></xsl:with-param>
    <xsl:with-param name='heading'>People</xsl:with-param>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="searchResultEntry" mode="organization">
  <xsl:param name="hidden" select="off" />
  <xsl:param name="heading" select="attr[((@name='objectClass') and (position() = 1))]" />
  <xsl:element name="div">
    <xsl:attribute name="class">resultDiv</xsl:attribute>
    <xsl:attribute name="id"><xsl:value-of select="generate-id(.)" /></xsl:attribute>
    <xsl:if test="($hidden = 'on')">
      <xsl:attribute name="style">display: none;</xsl:attribute>
    </xsl:if>
    <form name="ChangeInfo" method="post" action="/ldapupdate" target="processWindow">
      <xsl:if test="(attribute::dn='')">
	<xsl:apply-templates select="attr[@name='objectClass']" mode="hidden" />
      </xsl:if>
      <table border='0' cellspacing='0'>
	<tr><td>
	    <xsl:apply-templates select="." mode="recordHead">
	      <xsl:with-param name='oClass'>organization</xsl:with-param>
	    </xsl:apply-templates>
	</td></tr>
	<tr><td>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">FormAction</xsl:attribute>
	      <xsl:attribute name="value"></xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">dn</xsl:attribute>
	      <xsl:attribute name="value"><xsl:value-of select="@dn"/></xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">newrdn</xsl:attribute>
	      <xsl:attribute name="value"><xsl:value-of select="substring-before(@dn,',')"/></xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">newSuperior</xsl:attribute>
	      <xsl:attribute name="value"><xsl:value-of select="substring-after(@dn, ',')"/></xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">BinaryType</xsl:attribute>
	      <xsl:attribute name="value">text/xml</xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">xsl1</xsl:attribute>
	      <xsl:attribute name="value">/psldap/DSML_response.xsl</xsl:attribute>
	    </xsl:element>
	    
	    <table class='boxed' width='100%'>
	      <tr><td>
		  <table>
		    <tr>
		      <xsl:call-template name="editableAttr">
			<xsl:with-param name="attrType">o</xsl:with-param>
			<xsl:with-param name="label">Name</xsl:with-param>
			<xsl:with-param name="width">32</xsl:with-param>
			<xsl:with-param name="maxwidth">64</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		    <tr>
		      <xsl:apply-templates select="attr[@name='userPassword']" >
			<xsl:with-param name='label'>Password</xsl:with-param>
		      </xsl:apply-templates>
		    </tr>
		    <tr>
		      <xsl:call-template name="editableAttr">
			<xsl:with-param name="attrType">businessCategory</xsl:with-param>
			<xsl:with-param name="label">Category</xsl:with-param>
			<xsl:with-param name="width">32</xsl:with-param>
			<xsl:with-param name="maxwidth">48</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		    <tr>
		      <xsl:call-template name="editableAttr">
			<xsl:with-param name="attrType">description</xsl:with-param>
			<xsl:with-param name="label">Description</xsl:with-param>
			<xsl:with-param name="width">48</xsl:with-param>
			<xsl:with-param name="maxwidth">128</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		    <tr>
		      <xsl:call-template name="editableAttr">
			<xsl:with-param name="attrType">seeAlso</xsl:with-param>
			<xsl:with-param name="label">See Also</xsl:with-param>
			<xsl:with-param name="width">48</xsl:with-param>
			<xsl:with-param name="maxwidth">128</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		    <tr>
		      <xsl:call-template name="editableDeliveryPreference">
			<xsl:with-param name="attrType">preferredDeliveryMethod</xsl:with-param>
			<xsl:with-param name="label">Delivery Pref</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		  </table>
	      </td></tr>
	      <tr><td>
		  <div class="work_info">
		    <xsl:apply-templates select="." mode="orgAddress">
		    </xsl:apply-templates>
		  </div>
		  
		  <div class="other_info">
		    <xsl:element name="fieldset">
		      <xsl:call-template name="tabLegend">
		      </xsl:call-template>
		      <table>
			<tr>
			  <xsl:call-template name="editableAttr">
			    <xsl:with-param name="attrType">searchGuide</xsl:with-param>
			    <xsl:with-param name="label">Search Guide</xsl:with-param>
			    <xsl:with-param name="width">32</xsl:with-param>
			    <xsl:with-param name="maxwidth">64</xsl:with-param>
			  </xsl:call-template>
			</tr>
			<xsl:call-template name="editableAdditionalAddr" />
		      </table>
		    </xsl:element>
		    <br />
		  </div>
		  
		  <xsl:if test="attr[@name='objectClass']/value='psVendorAcctObject'">
		    <div class="vendor_info">
		      <xsl:element name="fieldset">
			<xsl:call-template name="tabLegend" />
			<table>
			  <tr>
			    <xsl:call-template name="editableAttr">
			      <xsl:with-param name="attrType">vendorClass</xsl:with-param>
			      <xsl:with-param name="label">Vendor Type</xsl:with-param>
			      <xsl:with-param name="width">32</xsl:with-param>
			      <xsl:with-param name="maxwidth">64</xsl:with-param>
			    </xsl:call-template>
			  </tr>
			  <tr>
			    <xsl:call-template name="editableAttr">
			      <xsl:with-param name="attrType">svcDescription</xsl:with-param>
			      <xsl:with-param name="label">Svc. Description</xsl:with-param>
			      <xsl:with-param name="width">48</xsl:with-param>
			      <xsl:with-param name="maxwidth">64</xsl:with-param>
			    </xsl:call-template>
			  </tr>
			  <tr>
			    <xsl:call-template name="editableAttr">
			      <xsl:with-param name="attrType">acctNo</xsl:with-param>
			      <xsl:with-param name="label">Account No</xsl:with-param>
			      <xsl:with-param name="width">32</xsl:with-param>
			      <xsl:with-param name="maxwidth">64</xsl:with-param>
			    </xsl:call-template>
			  </tr>
			  <tr>
			    <xsl:call-template name="editableAttr">
			      <xsl:with-param name="attrType">loginCred</xsl:with-param>
			      <xsl:with-param name="label">Credentials</xsl:with-param>
			      <xsl:with-param name="width">48</xsl:with-param>
			      <xsl:with-param name="maxwidth">64</xsl:with-param>
			    </xsl:call-template>
			  </tr>
			</table>
			<br />
		      </xsl:element>
		    </div>
		  </xsl:if>
	      </td></tr>
	    </table>
	</td></tr>
      </table>
    </form>
  </xsl:element>
</xsl:template>

<xsl:template match="searchResultEntry" mode="organizationalUnit">
  <xsl:param name="hidden" select="off" />
  <xsl:param name="heading" select="attr[@name='objectClass' and position() = 1]" />

  <xsl:element name="div">
    <xsl:attribute name="class">resultDiv</xsl:attribute>
    <xsl:attribute name="id"><xsl:value-of select="generate-id(.)" /></xsl:attribute>
    <xsl:if test="($hidden = 'on')">
      <xsl:attribute name="style">display: none;</xsl:attribute>
    </xsl:if>
    <form name="ChangeInfo" method="post" action="/ldapupdate" target="processWindow">
      <xsl:if test="(attribute::dn='')">
	<xsl:apply-templates select="attr[@name='objectClass']" mode="hidden" />
      </xsl:if>
      <table border='0' cellspacing='0'>
	<tr><td>
	    <xsl:apply-templates select="." mode="recordHead">
	      <xsl:with-param name='oClass'>organizationalUnit</xsl:with-param>
	    </xsl:apply-templates>
	</td></tr>
	<tr><td>
	    <table class='boxed' width='100%'>
	      <tr><td>
		  <xsl:element name="input">
		    <xsl:attribute name="type">hidden</xsl:attribute>
		    <xsl:attribute name="name">FormAction</xsl:attribute>
		    <xsl:attribute name="value"></xsl:attribute>
		  </xsl:element>
		  <xsl:element name="input">
		    <xsl:attribute name="type">hidden</xsl:attribute>
		    <xsl:attribute name="name">dn</xsl:attribute>
		    <xsl:attribute name="value"><xsl:value-of select="@dn"/></xsl:attribute>
		  </xsl:element>
		  <xsl:element name="input">
		    <xsl:attribute name="type">hidden</xsl:attribute>
		    <xsl:attribute name="name">newrdn</xsl:attribute>
		    <xsl:attribute name="value"><xsl:value-of select="substring-before(@dn,',')"/></xsl:attribute>
		  </xsl:element>
		  <xsl:element name="input">
		    <xsl:attribute name="type">hidden</xsl:attribute>
		    <xsl:attribute name="name">newSuperior</xsl:attribute>
		    <xsl:attribute name="value"><xsl:value-of select="substring-after(@dn, ',')"/></xsl:attribute>
		  </xsl:element>
		  <xsl:element name="input">
		    <xsl:attribute name="type">hidden</xsl:attribute>
		    <xsl:attribute name="name">BinaryType</xsl:attribute>
		    <xsl:attribute name="value">text/xml</xsl:attribute>
		  </xsl:element>
		  <xsl:element name="input">
		    <xsl:attribute name="type">hidden</xsl:attribute>
		    <xsl:attribute name="name">xsl1</xsl:attribute>
		    <xsl:attribute name="value">/psldap/DSML_response.xsl</xsl:attribute>
		  </xsl:element>
		  <table>
		    <tr>
		      <xsl:call-template name="editableAttr">
			<xsl:with-param name="attrType">ou</xsl:with-param>
			<xsl:with-param name="label">Name</xsl:with-param>
			<xsl:with-param name="width">32</xsl:with-param>
			<xsl:with-param name="maxwidth">64</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		    <tr>
		      <xsl:call-template name="editableAttr">
			<xsl:with-param name="attrType">o</xsl:with-param>
			<xsl:with-param name="label">Organization</xsl:with-param>
			<xsl:with-param name="width">32</xsl:with-param>
			<xsl:with-param name="maxwidth">64</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		    <tr>
		      <xsl:apply-templates select="attr[@name='userPassword']" >
			<xsl:with-param name='label'>Password</xsl:with-param>
		      </xsl:apply-templates>
		    </tr>
		    <tr>
		      <xsl:call-template name="editableAttr">
			<xsl:with-param name="attrType">businessCategory</xsl:with-param>
			<xsl:with-param name="label">Category</xsl:with-param>
			<xsl:with-param name="width">32</xsl:with-param>
			<xsl:with-param name="maxwidth">48</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		    <tr>
		      <xsl:call-template name="editableAttr">
			<xsl:with-param name="attrType">description</xsl:with-param>
			<xsl:with-param name="label">Description</xsl:with-param>
			<xsl:with-param name="width">48</xsl:with-param>
			<xsl:with-param name="maxwidth">128</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		    <tr>
		      <xsl:call-template name="editableAttr">
			<xsl:with-param name="attrType">seeAlso</xsl:with-param>
			<xsl:with-param name="label">See Also</xsl:with-param>
			<xsl:with-param name="width">48</xsl:with-param>
			<xsl:with-param name="maxwidth">128</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		    <tr>
		      <xsl:call-template name="editableDeliveryPreference">
			<xsl:with-param name="attrType">preferredDeliveryMethod</xsl:with-param>
			<xsl:with-param name="label">Delivery Pref</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		  </table>
	      </td></tr>
	      <tr><td>

		  <div class="work_info">
		    <xsl:apply-templates select="." mode="orgAddress">
		    </xsl:apply-templates>
		  </div>
		  
		  <div class="other_info">
		    <xsl:element name="fieldset">
		      <xsl:call-template name="tabLegend" />
		      <table>
			<tr>
			  <xsl:call-template name="editableAttr">
			    <xsl:with-param name="attrType">searchGuide</xsl:with-param>
			    <xsl:with-param name="label">Search Guide</xsl:with-param>
			    <xsl:with-param name="width">32</xsl:with-param>
			    <xsl:with-param name="maxwidth">64</xsl:with-param>
			  </xsl:call-template>
			</tr>
			<xsl:call-template name="editableAdditionalAddr" />
		      </table>
		      <br />
		    </xsl:element>
		  </div>

	      </td></tr>
	    </table>
	</td></tr>
      </table>
    </form>
  </xsl:element>
</xsl:template>

<xsl:template match="searchResultEntry" mode="organizationalPerson">
  <xsl:param name="hidden" select="off" />
  <xsl:param name="heading" select="attr[@name='objectClass' and position() = 1]" />
  <xsl:element name="div">
    <xsl:attribute name="class">resultDiv</xsl:attribute>
    <xsl:attribute name="nodeid"><xsl:value-of select="generate-id()" /></xsl:attribute>
    <xsl:if test="($hidden = 'on')">
      <xsl:attribute name="style">display: none;</xsl:attribute>
    </xsl:if>
    <form name="ChangeInfo" method="post" action="/ldapupdate" target="processWindow" enctype="multipart/form-data" >
      <xsl:if test="(attribute::dn='')">
	<xsl:apply-templates select="attr[@name='objectClass']" mode="hidden" />
      </xsl:if>
      <table border='0' cellspacing='0'>
	<tr>
	  <td>
	    <xsl:apply-templates select="." mode="recordHead">
	      <xsl:with-param name='oClass'>organizationalPerson</xsl:with-param>
	    </xsl:apply-templates>
	</td></tr>
	<tr><td>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">FormAction</xsl:attribute>
	      <xsl:attribute name="value"></xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">dn</xsl:attribute>
	      <xsl:attribute name="value"><xsl:value-of select="@dn"/></xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">newrdn</xsl:attribute>
	      <xsl:attribute name="value"><xsl:value-of select="substring-before(@dn,',')"/></xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">newSuperior</xsl:attribute>
	      <xsl:attribute name="value"><xsl:value-of select="substring-after(@dn, ',')"/></xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">BinaryType</xsl:attribute>
	      <xsl:attribute name="value">text/xml</xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">xsl1</xsl:attribute>
	      <xsl:attribute name="value">/psldap/DSML_response.xsl</xsl:attribute>
	    </xsl:element>
	    <table class='boxed' width='100%'>
	      <tr><td>
		  <table>
		    <tr>
		      <td width='120px' align='center'>
			<!--<em>Photo<br /> Here</em> -->
			<xsl:apply-templates select="." mode="jpegPhoto">
			</xsl:apply-templates>
		      </td>
		      <td>
			<table>
			  <tr><td>
			      <xsl:element name="input">
				<xsl:attribute name="type">hidden</xsl:attribute>
				<xsl:attribute name="name">cn</xsl:attribute>
				<xsl:attribute name="value"><xsl:value-of select="attr[@name='cn']/value"/></xsl:attribute>
			      </xsl:element>
			      <table>
				<tr>
				  <xsl:call-template name="editableAttr">
				    <xsl:with-param name="attrType">givenName</xsl:with-param>
				    <xsl:with-param name="label">First</xsl:with-param>
				    <xsl:with-param name="width">10</xsl:with-param>
				    <xsl:with-param name="maxwidth">24</xsl:with-param>
				    <xsl:with-param name="multirow">no</xsl:with-param>
				  </xsl:call-template>
				  <xsl:call-template name="editableAttr">
				    <xsl:with-param name="attrType">sn</xsl:with-param>
				    <xsl:with-param name="label">Last</xsl:with-param>
				    <xsl:with-param name="width">18</xsl:with-param>
				    <xsl:with-param name="maxwidth">32</xsl:with-param>
				    <xsl:with-param name="multirow">no</xsl:with-param>
				  </xsl:call-template>
				</tr>
				<tr>
				  <xsl:call-template name="editableAttr">
				    <xsl:with-param name="attrType">title</xsl:with-param>
				    <xsl:with-param name="label">Title</xsl:with-param>
				    <xsl:with-param name="colspan">3</xsl:with-param>
				    <xsl:with-param name="width">42</xsl:with-param>
				    <xsl:with-param name="maxwidth">64</xsl:with-param>
				    <xsl:with-param name="multirow">no</xsl:with-param>
				  </xsl:call-template>
				</tr>
			      </table>
			  </td></tr>
			  <tr><td>
			  </td></tr>
			  
			  <tr><td>
			      <table>
				<tr>
				  <xsl:choose>
				    <xsl:when test="(attribute::dn='')" >
				      <xsl:comment>
					The template XML should contain a userPassword entry
				      </xsl:comment>
				    </xsl:when>
				    <xsl:otherwise>
				    </xsl:otherwise>
				  </xsl:choose>
				  <xsl:if test="((0 = count(attr[@name='userPassword'])) and (not (0 = count(attr[@name='objectClass']/value[text()='person']))))">
				    <xsl:call-template name="genericAttribute">
				      <xsl:with-param name='label'>Password</xsl:with-param>
				      <xsl:with-param name='attrType'>userPassword</xsl:with-param>
				    </xsl:call-template>
				  </xsl:if>  
				  <xsl:apply-templates select="attr[@name='userPassword']" >
				    <xsl:with-param name='label'>Password</xsl:with-param>
				  </xsl:apply-templates>
				</tr>
				<tr>
				  <xsl:call-template name="editableAttr">
				    <xsl:with-param name="attrType">mail</xsl:with-param>
				    <xsl:with-param name="label">e-Mail</xsl:with-param>
				    <xsl:with-param name="width">32</xsl:with-param>
				    <xsl:with-param name="maxwidth">64</xsl:with-param>
				  </xsl:call-template>
				</tr>
				<tr>
				  <xsl:call-template name="editableAttr">
				    <xsl:with-param name="attrType">o</xsl:with-param>
				    <xsl:with-param name="label">Organization</xsl:with-param>
				    <xsl:with-param name="width">32</xsl:with-param>
				    <xsl:with-param name="maxwidth">64</xsl:with-param>
				  </xsl:call-template>
				</tr>
				<tr>
				  <xsl:call-template name="editableAttr">
				    <xsl:with-param name="attrType">ou</xsl:with-param>
				    <xsl:with-param name="label">Unit</xsl:with-param>
				    <xsl:with-param name="width">32</xsl:with-param>
				    <xsl:with-param name="maxwidth">64</xsl:with-param>
				  </xsl:call-template>
				</tr>
			      </table>
			  </td></tr>
			</table>
		      </td>
		    </tr>
		  </table>
	      </td></tr>

	      <tr><td>
		  <table>
		    <tr>
		      <xsl:call-template name="editableAttr">
			<xsl:with-param name="attrType">manager</xsl:with-param>
			<xsl:with-param name="label">Manager</xsl:with-param>
			<xsl:with-param name="width">56</xsl:with-param>
			<xsl:with-param name="maxwidth">256</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		    <tr>
		      <xsl:call-template name="editableAttr">
			<xsl:with-param name="attrType">description</xsl:with-param>
			<xsl:with-param name="label">Description</xsl:with-param>
			<xsl:with-param name="width">56</xsl:with-param>
			<xsl:with-param name="maxwidth">128</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		    <tr>
		      <xsl:call-template name="editableAttr">
			<xsl:with-param name="attrType">seeAlso</xsl:with-param>
			<xsl:with-param name="label">See Also</xsl:with-param>
			<xsl:with-param name="width">56</xsl:with-param>
			<xsl:with-param name="maxwidth">128</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		    <tr>
		      <xsl:call-template name="editableDeliveryPreference">
			<xsl:with-param name="attrType">preferredDeliveryMethod</xsl:with-param>
			<xsl:with-param name="label">Delivery Pref</xsl:with-param>
		      </xsl:call-template>
		    </tr>
		  </table>
	      </td></tr>

	      <tr><td>

		  <xsl:comment>Home information</xsl:comment>
		  <div class="personal_info">
		    <xsl:element name="fieldset">
		      <xsl:call-template name="tabLegend" />
		      <table>
			<tr>
			  <xsl:call-template name="editableAttrTA">
			    <xsl:with-param name="attrType">homePostalAddress</xsl:with-param>
			    <xsl:with-param name="label">Home Address</xsl:with-param>
			    <xsl:with-param name="rows">2</xsl:with-param>
			    <xsl:with-param name="colspan">3</xsl:with-param>
			    <xsl:with-param name="cols">48</xsl:with-param>
			  </xsl:call-template>
			</tr>
			<tr>
			  <xsl:call-template name="editableAttr">
			    <xsl:with-param name="attrType">homePhone</xsl:with-param>
			    <xsl:with-param name="label">Home Phone</xsl:with-param>
			    <xsl:with-param name="width">16</xsl:with-param>
			    <xsl:with-param name="maxwidth">24</xsl:with-param>
			  </xsl:call-template>
			  <xsl:call-template name="editableAttr">
			    <xsl:with-param name="attrType">mobile</xsl:with-param>
			    <xsl:with-param name="label">Mobile Phone</xsl:with-param>
			    <xsl:with-param name="width">16</xsl:with-param>
			    <xsl:with-param name="maxwidth">24</xsl:with-param>
			  </xsl:call-template>
			</tr>
		      </table>
		    </xsl:element>
		  </div>

		  <xsl:comment>Work information</xsl:comment>
		  <div class="work_info">
		    <xsl:apply-templates select="." mode="orgAddress">
		    </xsl:apply-templates>
		  </div>
		  
		  <div class="other_info">
		    <xsl:element name="fieldset">
		      <xsl:call-template name="tabLegend" />
		      <table>
			<xsl:call-template name="editableAdditionalAddr" />
		      </table>
		      <br />
		    </xsl:element>
		  </div>
		  
		  <xsl:if test="attr[@name='objectClass']/value='imIdObject'">
		    <div class="im_info">
		      <xsl:element name="fieldset">
			<xsl:call-template name="tabLegend" />
			<table>
			  <tr>
			    <xsl:call-template name="editableAttr">
			      <xsl:with-param name="attrType">yahooId</xsl:with-param>
			      <xsl:with-param name="label">Yahoo</xsl:with-param>
			      <xsl:with-param name="width">32</xsl:with-param>
			      <xsl:with-param name="maxwidth">64</xsl:with-param>
			    </xsl:call-template>
			  </tr>
			  <tr>
			    <xsl:call-template name="editableAttr">
			      <xsl:with-param name="attrType">aimId</xsl:with-param>
			      <xsl:with-param name="label">AIM</xsl:with-param>
			      <xsl:with-param name="width">32</xsl:with-param>
			      <xsl:with-param name="maxwidth">64</xsl:with-param>
			    </xsl:call-template>
			  </tr>
			  <tr>
			    <xsl:call-template name="editableAttr">
			      <xsl:with-param name="attrType">skypeId</xsl:with-param>
			      <xsl:with-param name="label">Skype</xsl:with-param>
			      <xsl:with-param name="width">32</xsl:with-param>
			      <xsl:with-param name="maxwidth">64</xsl:with-param>
			    </xsl:call-template>
			  </tr>
			</table>
			<br />
		      </xsl:element>
		    </div>
		  </xsl:if>
	      </td></tr>
	    </table>
	</td></tr>
      </table><br />
    </form>
  </xsl:element>
</xsl:template>

<xsl:template match="searchResultEntry" mode="groupOfUniqueNames">
  <xsl:param name="hidden" select="off" />
  <xsl:param name="heading" select="attr[((@name='objectClass') and (position() = 1))]" />
  <xsl:element name="div">
    <xsl:attribute name="class">resultDiv</xsl:attribute>
    <xsl:attribute name="id"><xsl:value-of select="generate-id(.)" /></xsl:attribute>
    <xsl:if test="($hidden = 'on')">
      <xsl:attribute name="style">display: none;</xsl:attribute>
    </xsl:if>
    <form name="ChangeInfo" method="post" action="/ldapupdate" target="processWindow">
      <xsl:if test="(attribute::dn='')">
	<xsl:apply-templates select="attr[@name='objectClass']" mode="hidden" />
      </xsl:if>
      <table >
	<tr><td>
	    <xsl:apply-templates select="." mode="recordHead">
	      <xsl:with-param name='oClass'>Groups</xsl:with-param>
	    </xsl:apply-templates>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">FormAction</xsl:attribute>
	      <xsl:attribute name="value"></xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">dn</xsl:attribute>
	      <xsl:attribute name="value"><xsl:value-of select="@dn"/></xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">newrdn</xsl:attribute>
	      <xsl:attribute name="value"><xsl:value-of select="substring-before(@dn,',')"/></xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">newSuperior</xsl:attribute>
	      <xsl:attribute name="value"><xsl:value-of select="substring-after(@dn, ',')"/></xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">BinaryType</xsl:attribute>
	      <xsl:attribute name="value">text/xml</xsl:attribute>
	    </xsl:element>
	    <xsl:element name="input">
	      <xsl:attribute name="type">hidden</xsl:attribute>
	      <xsl:attribute name="name">xsl1</xsl:attribute>
	      <xsl:attribute name="value">/psldap/DSML_response.xsl</xsl:attribute>
	    </xsl:element>
	</td></tr>
	<tr><td>
	    <table class='boxed' width='100%'>
	      <tr>
		<xsl:call-template name="editableAttr">
		  <xsl:with-param name="attrType">cn</xsl:with-param>
		  <xsl:with-param name="label">Name</xsl:with-param>
		  <xsl:with-param name="width">32</xsl:with-param>
		  <xsl:with-param name="maxwidth">64</xsl:with-param>
		  <xsl:with-param name="multirow">no</xsl:with-param>
		</xsl:call-template>
	      </tr>
	      <tr>
		<xsl:call-template name="editableAttr">
		  <xsl:with-param name="attrType">owner</xsl:with-param>
		  <xsl:with-param name="label">Owner</xsl:with-param>
		  <xsl:with-param name="width">32</xsl:with-param>
		  <xsl:with-param name="maxwidth">64</xsl:with-param>
		</xsl:call-template>
	      </tr>
	      <tr>
		<xsl:call-template name="editableAttrTA">
		  <xsl:with-param name="attrType">description</xsl:with-param>
		  <xsl:with-param name="label">Description</xsl:with-param>
		  <xsl:with-param name="rows">4</xsl:with-param>
		  <xsl:with-param name="cols">32</xsl:with-param>
		</xsl:call-template>
	      </tr>
	      <tr>
		<xsl:call-template name="editableAttr">
		  <xsl:with-param name="attrType">o</xsl:with-param>
		  <xsl:with-param name="label">Organization</xsl:with-param>
		  <xsl:with-param name="width">32</xsl:with-param>
		  <xsl:with-param name="maxwidth">64</xsl:with-param>
		</xsl:call-template>
	      </tr>
	      <tr>
		<xsl:call-template name="editableAttr">
		  <xsl:with-param name="attrType">ou</xsl:with-param>
		  <xsl:with-param name="label">Unit</xsl:with-param>
		  <xsl:with-param name="width">32</xsl:with-param>
		  <xsl:with-param name="maxwidth">64</xsl:with-param>
		</xsl:call-template>
	      </tr>
	      <tr>
		<xsl:call-template name="editableAttr">
		  <xsl:with-param name="attrType">businessCategory</xsl:with-param>
		  <xsl:with-param name="label">Category</xsl:with-param>
		  <xsl:with-param name="width">32</xsl:with-param>
		  <xsl:with-param name="maxwidth">64</xsl:with-param>
		</xsl:call-template>
	      </tr>
	      <tr>
		<xsl:call-template name="editableAttr">
		  <xsl:with-param name="attrType">seeAlso</xsl:with-param>
		  <xsl:with-param name="label">See Also</xsl:with-param>
		  <xsl:with-param name="width">32</xsl:with-param>
		  <xsl:with-param name="maxwidth">64</xsl:with-param>
		</xsl:call-template>
	      </tr>
	      <tr>
		<xsl:call-template name="editableAttr">
		  <xsl:with-param name="attrType">uniqueMember</xsl:with-param>
		  <xsl:with-param name="label">Members</xsl:with-param>
		  <xsl:with-param name="width">32</xsl:with-param>
		  <xsl:with-param name="maxwidth">128</xsl:with-param>
		</xsl:call-template>
	      </tr>
	    </table>
	</td></tr>
      </table><br />
    </form>
  </xsl:element>
</xsl:template>

<xsl:template name="editableAdditionalAddr">
  <tr otherInfo="x121Address">
    <xsl:call-template name="editableAttr">
      <xsl:with-param name="attrType">x121Address</xsl:with-param>
      <xsl:with-param name="label">X121 Address</xsl:with-param>
      <xsl:with-param name="width">48</xsl:with-param>
      <xsl:with-param name="maxwidth">64</xsl:with-param>
    </xsl:call-template>
  </tr>
  <tr otherInfo="registeredAddress">
    <xsl:call-template name="editableAttr">
      <xsl:with-param name="attrType">registeredAddress</xsl:with-param>
      <xsl:with-param name="label">Registered Address</xsl:with-param>
      <xsl:with-param name="width">48</xsl:with-param>
      <xsl:with-param name="maxwidth">64</xsl:with-param>
    </xsl:call-template>
  </tr>
  <tr otherInfo="destinationIndicator">
    <xsl:call-template name="editableAttr">
      <xsl:with-param name="attrType">destinationIndicator</xsl:with-param>
      <xsl:with-param name="label">Destination Indicator</xsl:with-param>
      <xsl:with-param name="width">48</xsl:with-param>
      <xsl:with-param name="maxwidth">64</xsl:with-param>
    </xsl:call-template>
  </tr>
  <tr otherInfo="labeledURI">
    <xsl:call-template name="editableAttr">
      <xsl:with-param name="attrType">labeledURI</xsl:with-param>
      <xsl:with-param name="label">Web Site</xsl:with-param>
      <xsl:with-param name="width">48</xsl:with-param>
      <xsl:with-param name="maxwidth">128</xsl:with-param>
    </xsl:call-template>
  </tr>
</xsl:template>

<xsl:template name="password">
  <xsl:param name="suffix"><xsl:number value="position()" format="-1" /></xsl:param>
  <xsl:param name="width" select="20" />
  <xsl:param name="maxwidth" select="64" />
  <xsl:param name='attrType' select="ancestor-or-self::attr/attribute::name" />
  <xsl:param name='default' select="." />

  <xsl:element name="input">
    <xsl:attribute name="type">password</xsl:attribute>
    <xsl:attribute name="name"><xsl:value-of select='$attrType' /><xsl:value-of select='$suffix' /></xsl:attribute>
    <xsl:attribute name="size"><xsl:value-of select='$width' /></xsl:attribute>
    <xsl:attribute name="maxlength"><xsl:value-of select='$maxwidth' /></xsl:attribute>
    <xsl:attribute name="value"><xsl:value-of select='$default' /></xsl:attribute>
  </xsl:element>
</xsl:template>

<xsl:template name="textinput">
  <xsl:param name="suffix" select="(position())" />
  <xsl:param name="width" >20</xsl:param>
  <xsl:param name="maxwidth" >64</xsl:param>
  <xsl:param name="multirow" >yes</xsl:param>
  <xsl:param name="fillvalue" select="." />
  <xsl:param name='attrType' select="ancestor-or-self::attr/attribute::name" />
  <xsl:param name="refClass"></xsl:param>
  <xsl:param name="refClassAttr"></xsl:param>

  <xsl:element name="span">
    <xsl:element name="input">
      <xsl:attribute name="type">text</xsl:attribute>
      <xsl:attribute name="name"><xsl:value-of select='$attrType' /><xsl:value-of select='$suffix' /></xsl:attribute>
      <xsl:attribute name="size"><xsl:value-of select='$width' /></xsl:attribute>
      <xsl:attribute name="maxlength"><xsl:value-of select='$maxwidth' /></xsl:attribute>
      <xsl:attribute name="value"><xsl:value-of select="$fillvalue" /></xsl:attribute>
      <xsl:if test="(($multirow = 'yes') and ($fillvalue = '') )" >
        <xsl:attribute name="onchange">showNodeManagementImages(this,this.value!='')</xsl:attribute>
      </xsl:if>
      <xsl:if test="(($refClass != '') and ($refClassAttr != '') )" >
	<xsl:attribute name="psLdapRefClass"><xsl:value-of select="$refClass" /></xsl:attribute>
	<xsl:attribute name="psLdapRefClassAttr"><xsl:value-of select="$refClassAttr" /></xsl:attribute>
      </xsl:if>
    </xsl:element>
    <xsl:text>&nbsp;</xsl:text>
    <xsl:element name="img">
      <xsl:attribute name="class">deleteNode</xsl:attribute>
      <xsl:attribute name="name">delete</xsl:attribute>
      <xsl:attribute name="src"><xsl:value-of select="$psldapRoot" />/images/delAttr.gif</xsl:attribute>
      <xsl:attribute name="alt">Del </xsl:attribute>
      <xsl:attribute name="title">Delete attribute</xsl:attribute>
      <xsl:attribute name="onmouseup">deleteCurrentSpan(this)</xsl:attribute>
      <xsl:if test="(not ($multirow = 'yes'))" >
        <xsl:attribute name="style">display: none;</xsl:attribute>
      </xsl:if>
    </xsl:element>
    <xsl:element name="img">
      <xsl:attribute name="class">cloneNode</xsl:attribute>
      <xsl:attribute name="name">clone</xsl:attribute>
      <xsl:attribute name="src"><xsl:value-of select="$psldapRoot" />/images/newAttr.gif</xsl:attribute>
      <xsl:attribute name="alt">New </xsl:attribute>
      <xsl:attribute name="title">Add attribute</xsl:attribute>
      <xsl:attribute name="onmouseup">cloneAndAppendCurrentSpan(this)</xsl:attribute>
      <xsl:if test="(($multirow = 'yes') and (position() = last()) and not ($fillvalue = ''))" >
        <xsl:attribute name="style">display: inline;</xsl:attribute>
      </xsl:if>
    </xsl:element>
    <xsl:if test="not ($suffix = '1')">
      <xsl:element name="br" />
    </xsl:if>
  </xsl:element>
</xsl:template>

<xsl:template name="tabLegend">
  <xsl:element name="Legend">
    <a href="javascript: void showInfo('none', 'block','none','none','none');">Work Info</a> | 
    <a href="javascript: void showInfo('none', 'none','block','none','none');">Other Info </a>
    <xsl:if test="attr[@name='objectClass']/value='person'">
      <a href="javascript: void showInfo('block', 'none','none','none','none');">| Personal Info </a>
    </xsl:if>
    <xsl:if test="attr[@name='objectClass']/value='imIdObject'">
      <a href="javascript: void showInfo('none', 'none','none','block','none');">| IM Info</a> 
    </xsl:if>
    <xsl:if test="attr[@name='objectClass']/value='psVendorAcctObject'">
      <a href="javascript: void showInfo('none', 'none','none','none','block');">| Vendor Info</a> 
    </xsl:if>
  </xsl:element>
</xsl:template>

<xsl:template name="editableAttr">
  <xsl:param name="attrType" select="attr/attribute::name" />
  <xsl:param name="label" select="attr/attribute::name" />
  <xsl:param name="colspan">1</xsl:param>
  <xsl:param name="width">16</xsl:param>
  <xsl:param name="maxwidth">32</xsl:param>
  <xsl:param name="multirow">yes</xsl:param>
  <xsl:param name="refClass"></xsl:param>
  <xsl:param name="refClassAttr"></xsl:param>
  <td class="label" >
    <xsl:element name="label">
      <xsl:value-of select="$label" />
    </xsl:element>
  </td>
  <xsl:element name="td">
    <xsl:attribute name="class">data</xsl:attribute>
    <xsl:if test="(not ($colspan = '1'))">
      <xsl:attribute name="colspan"><xsl:value-of select="$colspan" /></xsl:attribute>
    </xsl:if>
    <xsl:variable name="titleCount" select="count(attr[@name=$attrType])" />
    <xsl:for-each select="attr[@name=$attrType]/value" >
      <xsl:call-template name="textinput">
        <xsl:with-param name="attrType"><xsl:value-of select="$attrType" /></xsl:with-param>
        <xsl:with-param name="suffix"><xsl:number value="position()" format="-1" /></xsl:with-param>
        <xsl:with-param name="width"><xsl:value-of select="$width" /></xsl:with-param>
        <xsl:with-param name="maxwidth"><xsl:value-of select="$maxwidth" /></xsl:with-param>
        <xsl:with-param name="multirow"><xsl:value-of select="$multirow" /></xsl:with-param>
        <xsl:with-param name="refClass"><xsl:value-of select="$refClass" /></xsl:with-param>
        <xsl:with-param name="refClassAttr"><xsl:value-of select="$refClassAttr" /></xsl:with-param>
      </xsl:call-template>
    </xsl:for-each>
    <xsl:if test="($titleCount = '0')">
      <xsl:call-template name="textinput">
        <xsl:with-param name="attrType"><xsl:value-of select="$attrType" /></xsl:with-param>
        <xsl:with-param name="suffix"><xsl:number value="1" format="-1" /></xsl:with-param>
        <xsl:with-param name="width"><xsl:value-of select="$width" /></xsl:with-param>
        <xsl:with-param name="maxwidth"><xsl:value-of select="$maxwidth" /></xsl:with-param>
        <xsl:with-param name="multirow"><xsl:value-of select="$multirow" /></xsl:with-param>
        <xsl:with-param name="fillvalue" />
        <xsl:with-param name="refClass"><xsl:value-of select="$refClass" /></xsl:with-param>
        <xsl:with-param name="refClassAttr"><xsl:value-of select="$refClassAttr" /></xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:element>
</xsl:template>

<xsl:template name="editableAttrTA">
  <xsl:param name="attrType" select="attr[(position()=1)]/attribute::name" />
  <xsl:param name="label" select="attr[(position()=1)]/attribute::name" />
  <xsl:param name="colspan">1</xsl:param>
  <xsl:param name="rows" select="4" />
  <xsl:param name="cols" select="32" />
  <xsl:param name="focus" />
  <td class="label">
    <xsl:element name="label">
      <xsl:value-of select="$label" />
    </xsl:element>
  </td>
  <xsl:element name="td">
    <xsl:attribute name="class">data</xsl:attribute>
    <xsl:attribute name="colspan"><xsl:value-of select="$colspan" /></xsl:attribute>
    <xsl:variable name="titleCount" select="count(attr[@name=$attrType])" />
    <xsl:for-each select="attr[@name=$attrType]" >
      <xsl:call-template name="textarea">
        <xsl:with-param name='rows'><xsl:value-of select='$rows' /></xsl:with-param>
        <xsl:with-param name='cols'><xsl:value-of select='$cols' /></xsl:with-param>
        <xsl:with-param name='focus'><xsl:value-of select='$focus' /></xsl:with-param>
      </xsl:call-template>
      <xsl:element name="br" />
    </xsl:for-each>
    <xsl:if test="($titleCount = '0')">
      <xsl:call-template name="textarea">
        <xsl:with-param name='attrType'><xsl:value-of select='$attrType' /></xsl:with-param>
        <xsl:with-param name='rows'><xsl:value-of select='$rows' /></xsl:with-param>
        <xsl:with-param name='cols'><xsl:value-of select='$cols' /></xsl:with-param>
        <xsl:with-param name='focus'><xsl:value-of select='$focus' /></xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:element>
</xsl:template>

<xsl:template name="editableDeliveryPreference">
  <xsl:param name="attrType" select="attr[@name='preferredDeliveryMethod']/attribute::name" />
  <xsl:param name="label" select="attr[@name='preferredDeliveryMethod']/attribute::name" />
  <xsl:param name="size" select="1" />
  <td class="label">
    <xsl:element name="label">
      <xsl:value-of select="$label" />
    </xsl:element>
  </td>
  <td class="data">
    <xsl:variable name="titleCount" select="count(attr[@name=$attrType])" />
    <xsl:variable name="selection" select="attr[@name=$attrType]/value[(position() = 1)]" />
    <xsl:element name="select">
      <xsl:attribute name="name"><xsl:value-of select="$attrType" /></xsl:attribute>
      <xsl:attribute name="size">1</xsl:attribute>
      <xsl:element name="option">
        <xsl:attribute name="value"></xsl:attribute>
        <xsl:if test="($selection = '')">
          <xsl:attribute name="selected" />
        </xsl:if>
        -- DNC --
      </xsl:element>
      <xsl:element name="option">
        <xsl:attribute name="value">mail</xsl:attribute>
        <xsl:if test="($selection = 'mail')">
          <xsl:attribute name="selected" />
        </xsl:if>
        e-Mail
      </xsl:element>
      <xsl:element name="option">
	<xsl:attribute name="value">facsimileTelephoneNumber</xsl:attribute>
        <xsl:if test="($selection = 'facsimileTelephoneNumber')">
          <xsl:attribute name="selected" />
        </xsl:if>
        Fax
      </xsl:element>
      <xsl:element name="option">
	<xsl:attribute name="value">homePhone</xsl:attribute>
        <xsl:if test="($selection = 'homePhone')">
          <xsl:attribute name="selected" />
        </xsl:if>
        Home Phone
      </xsl:element>
      <xsl:element name="option">
	<xsl:attribute name="value">postalAddress</xsl:attribute>
        <xsl:if test="($selection = 'postalAddress')">
          <xsl:attribute name="selected" />
        </xsl:if>
        Mail
      </xsl:element>
      <xsl:element name="option">
	<xsl:attribute name="value">mobile</xsl:attribute>
        <xsl:if test="($selection = 'mobile')">
          <xsl:attribute name="selected" />
        </xsl:if>
        Mobile Phone
      </xsl:element>
      <xsl:element name="option">
	<xsl:attribute name="value">telephoneNumber</xsl:attribute>
        <xsl:if test="($selection = 'telephoneNumber')">
          <xsl:attribute name="selected" />
        </xsl:if>
        Office Phone
      </xsl:element>
    </xsl:element>
  </td>
</xsl:template>

<xsl:template match="attr[@name='objectClass']" mode="hidden">
  <xsl:param name='attrType' select="ancestor-or-self::attr/attribute::name" />
  <xsl:element name="div">
    <xsl:attribute name="class">ocEdit</xsl:attribute>
    <xsl:attribute name="name">ocEdit</xsl:attribute>
    <xsl:element name="table">
      <xsl:element name="tr">
	<xsl:apply-templates select=".">
	  <xsl:with-param name="attrType">objectClass</xsl:with-param>
	  <xsl:with-param name="label">Object Class:</xsl:with-param>
	  <xsl:with-param name="colspan">3</xsl:with-param>
	  <xsl:with-param name="width">48</xsl:with-param>
	  <xsl:with-param name="maxwidth">64</xsl:with-param>
	  <xsl:with-param name="multirow">yes</xsl:with-param>
	</xsl:apply-templates>
      </xsl:element>
    </xsl:element>
  </xsl:element>
</xsl:template>

<xsl:template match="attr" name="genericAttribute">
  <xsl:param name="label" select="@name" />
  <xsl:param name="width" select="20" />
  <xsl:param name="maxwidth" select="64" />
  <xsl:param name='attrType' select='@name' />

  <td class="label">
    <xsl:element name="label">
      <xsl:value-of select='$label' />
    </xsl:element>
  </td>
  <td class="data">
    <xsl:if test="(not (@name=$attrType))">
      <xsl:choose>
        <xsl:when test="($label = 'Password')" >
          <xsl:call-template name="password">
            <xsl:with-param name='width'><xsl:value-of select='$width' /></xsl:with-param>
            <xsl:with-param name='maxwidth'><xsl:value-of select='$maxwidth' /></xsl:with-param>
            <xsl:with-param name='suffix'>-1</xsl:with-param>
            <xsl:with-param name='attrType'><xsl:value-of select='$attrType' /></xsl:with-param>
            <xsl:with-param name='default'></xsl:with-param>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="textinput">
            <xsl:with-param name="suffix"><xsl:number value="1" format="-1" /></xsl:with-param>
            <xsl:with-param name='width'><xsl:value-of select='$width' /></xsl:with-param>
            <xsl:with-param name='maxwidth'><xsl:value-of select='$maxwidth' /></xsl:with-param>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:if>
    <xsl:for-each select='./value'>
      <xsl:sort select='.' />
      <xsl:choose>
        <xsl:when test="($label = 'Password')" >
          <xsl:call-template name="password">
            <xsl:with-param name='width'><xsl:value-of select='$width' /></xsl:with-param>
            <xsl:with-param name='maxwidth'><xsl:value-of select='$maxwidth' /></xsl:with-param>
          </xsl:call-template>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="textinput">
            <xsl:with-param name="suffix"><xsl:number value="position()" format="-1" /></xsl:with-param>
            <xsl:with-param name='width'><xsl:value-of select='$width' /></xsl:with-param>
            <xsl:with-param name='maxwidth'><xsl:value-of select='$maxwidth' /></xsl:with-param>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
  </td>
</xsl:template>

<xsl:template match="searchResultEntry" mode="recordHead">
  <xsl:param name="oClass" select="attr[@name='objectClass']/value[(position()=1)]" />
  <table class="menubar" width='100%' cellspacing='0'><tr>
      <td class="menubar_left">&nbsp;</td>
      <td class="menubar_center">
	<table width='100%'><tr>
            <td>
	      <xsl:element name="a">
		<xsl:if test="attr/attribute::name='labeledURI'">
		  <xsl:attribute name="href">
		    <xsl:value-of select="attr[@name='labeledURI']/value"/>
		  </xsl:attribute>
		</xsl:if>
		<xsl:choose>
		  <xsl:when test="($oClass = 'organization')" >
                    <xsl:value-of select="attr[@name='o']/value"/>
		  </xsl:when>
		  <xsl:when test="($oClass = 'organizationalUnit')" >
                    <xsl:value-of select="attr[@name='ou']/value"/>
		  </xsl:when>
		  <xsl:when test="($oClass = 'organizationalPerson')" >
                    <xsl:value-of select="attr[@name='givenName']/value"/>
                    <xsl:text> </xsl:text>
                    <xsl:value-of select="attr[@name='sn']/value"/>
		  </xsl:when>
		  <xsl:otherwise>
                    <xsl:value-of select="$oClass" />
                    <xsl:value-of select="attr[@name='cn']/value"/>,
		  </xsl:otherwise>
		</xsl:choose>
	      </xsl:element>
            </td>
            <td width="16">
	      <xsl:element name="input">
		<xsl:attribute name="type">checkbox</xsl:attribute>
		<xsl:attribute name="name">uniqueMember</xsl:attribute>
		<xsl:attribute name="value"><xsl:value-of select="@dn"/></xsl:attribute>
	      </xsl:element>
            </td>
	  </tr>
	</table>
      </td>
      <td class="menubar_right">&nbsp;</td>
  </tr></table>
</xsl:template>

<xsl:template match="searchResultEntry" mode="orgAddress">
  <xsl:element name="fieldset">
    <xsl:call-template name="tabLegend" />
    <table>
      <tr><td>
	  <table>
	    <tr workInfo="physicalDeliveryOfficeName">
	      <xsl:call-template name="editableAttr">
		<xsl:with-param name="attrType">physicalDeliveryOfficeName</xsl:with-param>
		<xsl:with-param name="label">Bldg</xsl:with-param>
		<xsl:with-param name="colspan">5</xsl:with-param>
		<xsl:with-param name="width">56</xsl:with-param>
		<xsl:with-param name="maxwidth">92</xsl:with-param>
	      </xsl:call-template>
	      <td width='*'></td>
	    </tr>
	    <tr workInfo="street">
	      <xsl:call-template name="editableAttr">
		<xsl:with-param name="attrType">street</xsl:with-param>
		<xsl:with-param name="label">Street</xsl:with-param>
		<xsl:with-param name="colspan">3</xsl:with-param>
		<xsl:with-param name="width">34</xsl:with-param>
		<xsl:with-param name="maxwidth">64</xsl:with-param>
		<xsl:with-param name="multirow">no</xsl:with-param>
	      </xsl:call-template>
	      <xsl:call-template name="editableAttr">
		<xsl:with-param name="attrType">postOfficeBox</xsl:with-param>
		<xsl:with-param name="label">P.O. Box</xsl:with-param>
		<xsl:with-param name="width">10</xsl:with-param>
		<xsl:with-param name="maxwidth">16</xsl:with-param>
		<xsl:with-param name="multirow">no</xsl:with-param>
	      </xsl:call-template>
	      <td width='*'></td>
	    </tr>
	    <tr workInfo="l">
	      <xsl:call-template name="editableAttr">
		<xsl:with-param name="attrType">l</xsl:with-param>
		<xsl:with-param name="label">City</xsl:with-param>
		<xsl:with-param name="width">20</xsl:with-param>
		<xsl:with-param name="maxwidth">48</xsl:with-param>
		<xsl:with-param name="multirow">no</xsl:with-param>
	      </xsl:call-template>
	      <xsl:call-template name="editableAttr">
		<xsl:with-param name="attrType">st</xsl:with-param>
		<xsl:with-param name="label">State</xsl:with-param>
		<xsl:with-param name="width">4</xsl:with-param>
		<xsl:with-param name="maxwidth">16</xsl:with-param>
		<xsl:with-param name="multirow">no</xsl:with-param>
	      </xsl:call-template>
	      <xsl:call-template name="editableAttr">
		<xsl:with-param name="attrType">postalCode</xsl:with-param>
		<xsl:with-param name="label">Postal</xsl:with-param>
		<xsl:with-param name="width">10</xsl:with-param>
		<xsl:with-param name="maxwidth">16</xsl:with-param>
		<xsl:with-param name="multirow">no</xsl:with-param>
	      </xsl:call-template>
	      <td width='*'></td>
	    </tr>

	    <tr workInfo="postalAddress">
	      <xsl:call-template name="editableAttrTA">
		<xsl:with-param name="attrType">postalAddress</xsl:with-param>
		<xsl:with-param name="label">Address</xsl:with-param>
		<xsl:with-param name="rows">3</xsl:with-param>
		<xsl:with-param name="colspan">5</xsl:with-param>
		<xsl:with-param name="cols">56</xsl:with-param>
		<xsl:with-param name="focus">setDefaultAddress(this);</xsl:with-param>
	      </xsl:call-template>
	      <td colspan='1'></td>
	    </tr>
	    <tr workInfo="telephoneNumber">
	      <xsl:call-template name="editableAttr">
		<xsl:with-param name="attrType">telephoneNumber</xsl:with-param>
		<xsl:with-param name="label">Phone</xsl:with-param>
		<xsl:with-param name="width">21</xsl:with-param>
		<xsl:with-param name="maxwidth">28</xsl:with-param>
		<xsl:with-param name="colspan">2</xsl:with-param>
	      </xsl:call-template>
	      <xsl:call-template name="editableAttr">
		<xsl:with-param name="attrType">facsimileTelephoneNumber</xsl:with-param>
		<xsl:with-param name="label">Fax</xsl:with-param>
		<xsl:with-param name="width">16</xsl:with-param>
		<xsl:with-param name="maxwidth">24</xsl:with-param>
		<xsl:with-param name="colspan">2</xsl:with-param>
	      </xsl:call-template>
	    </tr>
	  </table>
	</td>
      </tr>
      
      <tr workInfo="telexNumber"><td colspan='2'>
	  <table width='100%'>
	    <tr>
	      <xsl:apply-templates select="attr[@name='telexNumber']" >
		<xsl:with-param name='label'>Telex Number</xsl:with-param>
		<xsl:with-param name='width'>12</xsl:with-param>
		<xsl:with-param name='maxwidth'>12</xsl:with-param>
	      </xsl:apply-templates>
	    </tr>
	  </table>
      </td></tr>
      
      <tr workInfo="telexTerminalIdentifier"><td colspan='2'>
	  <table width='100%'>
	    <tr>
	      <xsl:apply-templates select="attr[@name='teletexTerminalIdentifier']" >
		<xsl:with-param name='label'>TTX Terminal Id</xsl:with-param>
		<xsl:with-param name='width'>16</xsl:with-param>
		<xsl:with-param name='maxwidth'>16</xsl:with-param>
	      </xsl:apply-templates>
	    </tr>
	  </table>
      </td></tr>
      
      <tr workInfo="internationaliSDNNumber"><td  colspan='2'>
	  <table width='100%'>
	    <tr>
	      <xsl:apply-templates select="attr[@name='internationaliSDNNumber']" >
		<xsl:with-param name='label'>ISDN Number</xsl:with-param>
		<xsl:with-param name='width'>14</xsl:with-param>
		<xsl:with-param name='maxwidth'>14</xsl:with-param>
	      </xsl:apply-templates>
	    </tr>
	  </table>
      </td></tr>
    </table>
  </xsl:element>
</xsl:template>

<xsl:template match="searchResultEntry" mode="jpegPhoto">
  <xsl:variable name="jpegUrl"><xsl:value-of select="attr[@name='jpegPhoto']/value" disable-output-escaping="yes" /></xsl:variable>
  <xsl:variable name="jpegCount" select="count(attr[@name='jpegPhoto'])+1" />
  <xsl:choose>
    <xsl:when test="(attr[@name='jpegPhoto']/value)">
      <xsl:element name="img">
        <xsl:attribute name="onclick">showNextSiblingAndHide(this,true)</xsl:attribute>
        <xsl:attribute name="name">encJpegPhoto</xsl:attribute>
        <xsl:attribute name="width">96</xsl:attribute>
        <xsl:attribute name="src"><xsl:if test="(not(contains(attr[@name='jpegPhoto']/value,'BinaryData')) or (starts-with(attr[@name='jpegPhoto']/value,'/9j/4AA')))"><xsl:text>data:image/jpeg;base64,</xsl:text></xsl:if><xsl:value-of select="$jpegUrl" /></xsl:attribute>
      </xsl:element>
    </xsl:when>
    <xsl:otherwise>
      <xsl:element name="br" />
    </xsl:otherwise>
  </xsl:choose>
  <xsl:element name="span">
    <xsl:attribute name="style">vertical-align:middle;<xsl:if test="(attr[@name='jpegPhoto']/value)">display:none;</xsl:if></xsl:attribute>
    <em>Photo<br />Here<br /></em>
    <xsl:element name="input">
      <xsl:attribute name="style">margin-top:10px</xsl:attribute>
      <xsl:attribute name="type">file</xsl:attribute>
      <xsl:attribute name="name"><xsl:if test="(attr[@name='jpegPhoto']/value)">hidden_</xsl:if>jpegPhoto<xsl:number value="1" format="-1" /></xsl:attribute>
      <xsl:attribute name="size">4</xsl:attribute>
      <xsl:attribute name="accept">image/jpeg,image/jpg</xsl:attribute>
    </xsl:element>
    <xsl:if test="(contains(attr[@name='jpegPhoto']/value,'BinaryData'))">
      <xsl:element name="br" />
      <xsl:element name="button">
        <xsl:attribute name="type">button</xsl:attribute>
        <xsl:attribute name="style">margin-top: 10px</xsl:attribute>
        <xsl:attribute name="onclick">showPreviousSiblingAndHide(this, true, "SPAN")</xsl:attribute>
        Cancel
      </xsl:element>
    </xsl:if>
  </xsl:element>
</xsl:template>

<xsl:template match="searchResultEntry" mode="userCertificate">
  <xsl:choose>
    <xsl:when test="(attr[@name='userCertificate']/value)">
      <xsl:element name="img">
        <xsl:attribute name="onclick">showNextSiblingAndHide(this,true)</xsl:attribute>
        <xsl:attribute name="name">encUserCertificate</xsl:attribute>
        <xsl:attribute name="width">24</xsl:attribute>
        <xsl:attribute name="src"><xsl:value-of select="$psldapRoot" />/images/certPresent.gif</xsl:attribute>
      </xsl:element>
    </xsl:when>
    <xsl:otherwise>
      <xsl:element name="br" />
    </xsl:otherwise>
  </xsl:choose>
  <xsl:element name="span">
    <xsl:attribute name="style">vertical-align:middle;<xsl:if test="(attr[@name='userCertificate']/value)">display:none;</xsl:if></xsl:attribute>
    <xsl:element name="input">
      <xsl:attribute name="style">margin-top:10px</xsl:attribute>
      <xsl:attribute name="type">file</xsl:attribute>
      <xsl:attribute name="name"><xsl:if test="(attr[@name='userCertificate']/value)">hidden_</xsl:if>userCertificate<xsl:number value="1" format="-1" /></xsl:attribute>
      <xsl:attribute name="size">4</xsl:attribute>
      <xsl:attribute name="accept">file/text,file/text</xsl:attribute>
    </xsl:element>
    <xsl:if test="(contains(attr[@name='userCertificate']/value,'BinaryData'))">
      <xsl:element name="br" />
      <xsl:element name="button">
        <xsl:attribute name="type">button</xsl:attribute>
        <xsl:attribute name="style">margin-top: 10px</xsl:attribute>
        <xsl:attribute name="onclick">showPreviousSiblingAndHide(this, true, "SPAN")</xsl:attribute>
        Cancel
      </xsl:element>
    </xsl:if>
  </xsl:element>
</xsl:template>

</xsl:stylesheet>
