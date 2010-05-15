<?xml version="1.0" encoding="ISO-8859-1"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/html4/loose.dtd'>

<xsl:include href="DSML_commonscript.xsl" />
<xsl:include href="DSML_sitefrags.xsl" />
<xsl:include href="DSML_edittools.xsl" />

<xsl:output method="html" doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN" doctype-system="http://www.w3.org/TR/html4/loose.dtd" omit-xml-declaration="no" media-type="text/html" />

<xsl:key name="searchEntryLookup" match="searchResultEntry" use="@dn" />
<xsl:key name="objectClassLookup" match="searchResultEntry" use="attr[@name='objectClass']/value" />

<xsl:template name="pageSpecificHeader" >
  <xsl:call-template name="editToolsHeader" />
  <xsl:element name="script">
    <xsl:attribute name="language">JavaScript</xsl:attribute>
    <![CDATA[
    function initialize() {
        window.status = "Loading forms...";

        recordNbrElmt = document.getElementById("recordNumber");

        showRecord(1);
        window.status = getAllFormElements().length + " records loaded";
    }
    ]]>
  </xsl:element>
  
</xsl:template>

<xsl:template match="/dsml" mode="registration" >
  <xsl:call-template name="pageHeaderWithRefClass" >
    <xsl:with-param name='title'>Register</xsl:with-param>
  </xsl:call-template>
    
  <xsl:element name="body">
    <xsl:attribute name="onload">initialize()</xsl:attribute>
      
    <xsl:call-template name="processStatusDiv" >
      <xsl:with-param name="onload">postRegistrationResponse();</xsl:with-param>
    </xsl:call-template>
    <xsl:element name="br" />
    <xsl:apply-templates select="//searchResponse/searchResultEntry[@dn='dc=registered']" mode="registration" >
      <xsl:with-param name='hidden'>off</xsl:with-param>
      <xsl:with-param name='updateURI'>/register</xsl:with-param>
    </xsl:apply-templates>
    <xsl:element name="br" />
    <xsl:element name="div" >
      <xsl:attribute name="align">center</xsl:attribute>
      <xsl:attribute name="style">width: 100%;</xsl:attribute >
      <xsl:element name="input">
	<xsl:attribute name="type">submit</xsl:attribute >
	<xsl:attribute name="value">Create Account</xsl:attribute >
	<xsl:attribute name="onClick">submitVisibleRecord('Register')</xsl:attribute >
      </xsl:element>
      &nbsp; &nbsp;
      <xsl:element name="input">
	<xsl:attribute name="type">submit</xsl:attribute >
	<xsl:attribute name="value">Cancel</xsl:attribute >
	<xsl:attribute name="onClick">window.close()</xsl:attribute >
      </xsl:element>
    </xsl:element>
  </xsl:element>
</xsl:template>

<xsl:template match="/dsml">
  <xsl:variable name="recordCount" select="count(//searchResponse/searchResultEntry)" />
  <xsl:text>
  </xsl:text>
  <xsl:element name="html">
    <xsl:choose>
      <xsl:when test="($recordCount = 1) and (//searchResponse/searchResultEntry[@dn='dc=registered']/attr[@name='objectClass']/value='simpleSecurityObject')" >
	<xsl:apply-templates select="." mode="registration" />
      </xsl:when>
      <xsl:otherwise>
	<xsl:call-template name="pageHeaderWithRefClass" >
	  <xsl:with-param name='title'>Edit LDAP Records</xsl:with-param>
	</xsl:call-template>
	
	<xsl:element name="body">
	  <xsl:attribute name="onload">initialize()</xsl:attribute>
	  
	  <xsl:call-template name="processStatusDiv" />

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
      </xsl:otherwise>
    </xsl:choose>
  </xsl:element>
</xsl:template>

<xsl:template name="searchResults" match="searchResponse">
  <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='organization')]/ancestor::searchResultEntry" mode="organization" >
    <xsl:with-param name='hidden'>on</xsl:with-param>
    <xsl:with-param name='updateURI'>/ldapupdate</xsl:with-param>
    <xsl:with-param name='heading'>Organization</xsl:with-param>
    <xsl:sort select="attr[@name='o']/value" />
  </xsl:apply-templates>
  
  <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='organizationalUnit')]/ancestor::searchResultEntry" mode="orgUnit">
    <xsl:with-param name='hidden'>on</xsl:with-param>
    <xsl:with-param name='updateURI'>/ldapupdate</xsl:with-param>
    <xsl:with-param name='heading'>Unit</xsl:with-param>
    <xsl:sort select="attr[@name='ou']/value" />
  </xsl:apply-templates>
  
  <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='organizationalPerson') or (text()='person') or (text()='inetOrgPerson')]/ancestor::searchResultEntry" mode="person">
    <xsl:with-param name='hidden'>on</xsl:with-param>
    <xsl:with-param name='updateURI'>/ldapupdate</xsl:with-param>
    <xsl:with-param name='heading'>People</xsl:with-param>
    <xsl:sort select="attr[@name='sn']/value" />
    <xsl:sort select="attr[@name='givenName']/value" />
    <xsl:sort select="attr[@name='cn']/value" />
  </xsl:apply-templates>
  
  <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='groupOfUniqueNames')]/ancestor::searchResultEntry" mode="groupOfUniqueNames">
    <xsl:with-param name='hidden'>on</xsl:with-param>
    <xsl:with-param name='updateURI'>/ldapupdate</xsl:with-param>
    <xsl:with-param name='heading'>Groups</xsl:with-param>
    <xsl:sort select="attr[@name='sn']/value" />
    <xsl:sort select="attr[@name='givenName']/value" />
    <xsl:sort select="attr[@name='cn']/value" />
  </xsl:apply-templates>

  <xsl:apply-templates select="searchResultEntry/attr[@name='structuralObjectClass']/value[not((text()='groupOfUniqueNames') or (text()='organizationalPerson') or (text()='person') or (text()='inetOrgPerson') or (text()='organizationalUnit') or (text()='organization') or (text()='OpenLDAPou'))]/ancestor::searchResultEntry" mode="generic">
    <xsl:with-param name='hidden'>on</xsl:with-param>
    <xsl:with-param name='updateURI'>/ldapupdate</xsl:with-param>
    <xsl:with-param name='heading'>Miscellaneous</xsl:with-param>
    <xsl:sort select="attr[@name='structuralObjectClass']/value" />
    <xsl:sort select="attr[@name='cn']/value" />
    <xsl:sort select="@dn" />
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
	  <xsl:element name="img">
	    <xsl:attribute name="src"><xsl:value-of select="$psldapRoot" />/images/prevRecord.gif</xsl:attribute>
	  </xsl:element>
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
	  <xsl:element name="img">
	    <xsl:attribute name="src"><xsl:value-of select="$psldapRoot" />/images/nextRecord.gif</xsl:attribute>
	  </xsl:element>
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

<xsl:template match="searchResultEntry" >
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

  <xsl:apply-templates select="attr[@name='objectClass']/value[(not((text()='groupOfUniqueNames') or (text()='organizationalPerson') or (text()='person') or (text()='inetOrgPerson') or (text()='organizationalUnit') or (text()='organization')))]/ancestor::searchResultEntry" mode="generic">
    <xsl:with-param name='hidden'><xsl:value-of select="$hidden" /></xsl:with-param>
    <xsl:with-param name='heading'>Miscellaneous</xsl:with-param>
    <xsl:sort select="attr[@name='structuralObjectClass']/value" />
    <xsl:sort select="attr[@name='cn']/value" />
    <xsl:sort select="@dn" />
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="attr" mode="genericForm" >
  <xsl:element name="tr">
    <xsl:call-template name="genericAttribute" >
      <xsl:with-param name="attrType"><xsl:value-of select="@name" /></xsl:with-param>
      <xsl:with-param name="label"><xsl:value-of select="@name" /></xsl:with-param>
      <xsl:with-param name="width">32</xsl:with-param>
      <xsl:with-param name="maxwidth">64</xsl:with-param>
    </xsl:call-template>
  </xsl:element>
</xsl:template>

<xsl:template match="searchResultEntry" mode="generic" >
  <xsl:param name="hidden" select="off" />
  <xsl:param name="updateURI" select="/ldapupdate" />
  <xsl:param name="heading" select="attr[((@name='structuralObjectClass') and (position() = 1))]" />
  <xsl:element name="div">
    <xsl:attribute name="class">resultDiv</xsl:attribute>
    <xsl:attribute name="id"><xsl:value-of select="generate-id(.)" /></xsl:attribute>
    <xsl:if test="($hidden = 'on')">
      <xsl:attribute name="style">display: none;</xsl:attribute>
    </xsl:if>
    <xsl:element name="form"> 
      <xsl:attribute name="name">ChangeInfo</xsl:attribute>
      <xsl:attribute name="method">post</xsl:attribute>
      <xsl:attribute name="action"><xsl:value-of select='$updateURI' /></xsl:attribute>
      <xsl:attribute name="target">processWindow</xsl:attribute>

      <xsl:apply-templates select="." mode="hiddenAttributes" />

      <table border='0' cellspacing='0'>
	<tr><td style="padding-bottom: 0px; " >
	    <xsl:apply-templates select="." mode="recordHead" />
	</td></tr>
	<tr><td style="padding-top: 0px; ">
	    <table class='boxed' width='100%'>
	      <xsl:apply-templates select="attr" mode="genericForm" >
		<xsl:sort select='@name' />
	      </xsl:apply-templates>
	    </table>
	</td></tr>
      </table>
    </xsl:element>
  </xsl:element>
</xsl:template>

<xsl:template match="searchResultEntry" mode="organization" >
  <xsl:param name="hidden" select="off" />
  <xsl:param name="updateURI" select="/ldapupdate" />
  <xsl:param name="heading" select="attr[((@name='structuralObjectClass') and (position() = 1))]" />
  <xsl:element name="div">
    <xsl:attribute name="class">resultDiv</xsl:attribute>
    <xsl:attribute name="id"><xsl:value-of select="generate-id(.)" /></xsl:attribute>
    <xsl:if test="($hidden = 'on')">
      <xsl:attribute name="style">display: none;</xsl:attribute>
    </xsl:if>
    <xsl:element name="form"> 
      <xsl:attribute name="name">ChangeInfo</xsl:attribute>
      <xsl:attribute name="method">post</xsl:attribute>
      <xsl:attribute name="action"><xsl:value-of select='$updateURI' /></xsl:attribute>
      <xsl:attribute name="target">processWindow</xsl:attribute>

      <xsl:apply-templates select="." mode="hiddenAttributes" />

      <table border='0' cellspacing='0'>
	<tr><td style="padding-bottom: 0px; ">
	    <xsl:apply-templates select="." mode="recordHead">
	      <xsl:with-param name='oClass'>organization</xsl:with-param>
	    </xsl:apply-templates>
	</td></tr>
	<tr><td style="padding-top: 0px; ">
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
		  <xsl:element name="fieldset">
		    <xsl:call-template name="tabLegend">
		    </xsl:call-template>
		    <xsl:apply-templates select="." mode="orgAddress" />
		  
		    <div class="other_info">
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
		    </div>
		  
		    <xsl:if test="attr[@name='objectClass']/value='psVendorAcctObject'">
		      <div class="vendor_info">
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
		      </div>
		    </xsl:if>
		  </xsl:element>
	      </td></tr>
	    </table>
	</td></tr>
      </table>
    </xsl:element>
  </xsl:element>
</xsl:template>

<xsl:template match="searchResultEntry" mode="orgUnit" >
  <xsl:param name="hidden" select="off" />
  <xsl:param name="updateURI" select="/ldapupdate" />
  <xsl:param name="heading" select="attr[@name='structuralObjectClass' and position() = 1]" />

  <xsl:element name="div">
    <xsl:attribute name="class">resultDiv</xsl:attribute>
    <xsl:attribute name="id"><xsl:value-of select="generate-id(.)" /></xsl:attribute>
    <xsl:if test="($hidden = 'on')">
      <xsl:attribute name="style">display: none;</xsl:attribute>
    </xsl:if>
    <xsl:element name="form"> 
      <xsl:attribute name="name">ChangeInfo</xsl:attribute>
      <xsl:attribute name="method">post</xsl:attribute>
      <xsl:attribute name="action"><xsl:value-of select='$updateURI' /></xsl:attribute>
      <xsl:attribute name="target">processWindow</xsl:attribute>

      <xsl:apply-templates select="." mode="hiddenAttributes" />

      <table border='0' cellspacing='0'>
	<tr><td style="padding-bottom: 0px; ">
	    <xsl:apply-templates select="." mode="recordHead">
	      <xsl:with-param name='oClass'>organizationalUnit</xsl:with-param>
	    </xsl:apply-templates>
	</td></tr>
	<tr><td style="padding-top: 0px; ">
	    <table class='boxed' width='100%'>
	      <tr><td>
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
		  <xsl:element name="fieldset">
		    <xsl:call-template name="tabLegend" />

		    <xsl:apply-templates select="." mode="orgAddress" />
		    
		    <div class="other_info">
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
		    </div>
		  </xsl:element>
	      </td></tr>
	    </table>
	</td></tr>
      </table>
    </xsl:element>
  </xsl:element>
</xsl:template>

<xsl:template match="searchResultEntry" mode="personalCore" >
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
		    <xsl:with-param name="validate">psldap_validateMinLength,1</xsl:with-param>
		    <xsl:with-param name="width">12</xsl:with-param>
		    <xsl:with-param name="maxwidth">24</xsl:with-param>
		    <xsl:with-param name="multirow">no</xsl:with-param>
		  </xsl:call-template>
		  <xsl:call-template name="editableAttr">
		    <xsl:with-param name="attrType">sn</xsl:with-param>
		    <xsl:with-param name="label">Last</xsl:with-param>
		    <xsl:with-param name="validate">psldap_validateMinLength,1</xsl:with-param>
		    <xsl:with-param name="width">20</xsl:with-param>
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
		  <xsl:if test="((0 = count(attr[@name='userPassword'])) and (not (0 = count(attr[@name='objectClass']/value[text()='person']))))">
		    <xsl:call-template name="genericAttribute">
		      <xsl:with-param name='label'>Password</xsl:with-param>
		      <xsl:with-param name="width">36</xsl:with-param>
		      <xsl:with-param name='attrType'>userPassword</xsl:with-param>
		      <xsl:with-param name="validate">psldap_validatePasswordStrength</xsl:with-param>
		    </xsl:call-template>
		  </xsl:if>  
		  <xsl:apply-templates select="attr[@name='userPassword']" >
		    <xsl:with-param name='label'>Password</xsl:with-param>
		    <xsl:with-param name="width">36</xsl:with-param>
		    <xsl:with-param name="validate">psldap_validatePasswordStrength</xsl:with-param>
		  </xsl:apply-templates>
		</tr>
		<tr>
		  <xsl:call-template name="editableAttr">
		    <xsl:with-param name="attrType">mail</xsl:with-param>
		    <xsl:with-param name="label">e-Mail</xsl:with-param>
		    <xsl:with-param name="validate">psldap_validateEMail</xsl:with-param>
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
</xsl:template>

<xsl:template match="searchResultEntry" mode="person" >
  <xsl:param name="hidden" select="off" />
  <xsl:param name="updateURI" select="/ldapupdate" />
  <xsl:param name="heading" select="attr[@name='structuralObjectClass' and position() = 1]" />
  <xsl:element name="div">
    <xsl:attribute name="class">resultDiv</xsl:attribute>
    <xsl:attribute name="nodeid"><xsl:value-of select="generate-id()" /></xsl:attribute>
    <xsl:if test="($hidden = 'on')">
      <xsl:attribute name="style">display: none;</xsl:attribute>
    </xsl:if>
    <xsl:element name="form"> 
      <xsl:attribute name="name">ChangeInfo</xsl:attribute>
      <xsl:attribute name="method">post</xsl:attribute>
      <xsl:attribute name="action"><xsl:value-of select='$updateURI' /></xsl:attribute>
      <xsl:attribute name="target">processWindow</xsl:attribute>
      <xsl:attribute name="enctype">multipart/form-data</xsl:attribute>

      <xsl:apply-templates select="." mode="hiddenAttributes" />
      <xsl:element name="input">
	<xsl:attribute name="type">hidden</xsl:attribute>
	<xsl:attribute name="name">uid</xsl:attribute>
	<xsl:attribute name="value"><xsl:value-of select="attr[@name='uid']"/></xsl:attribute>
      </xsl:element>
      <xsl:element name="input">
	<xsl:attribute name="type">hidden</xsl:attribute>
	<xsl:attribute name="name">gid</xsl:attribute>
	<xsl:attribute name="value"><xsl:value-of select="attr[@name='gid']"/></xsl:attribute>
      </xsl:element>

      <table border='0' cellspacing='0'>
	<tr><td style="padding-bottom: 0px; ">
	    <xsl:apply-templates select="." mode="recordHead">
	      <xsl:with-param name='oClass'>organizationalPerson</xsl:with-param>
	    </xsl:apply-templates>
	</td></tr>
	<tr><td style="padding-top: 0px; ">
	    <table class='boxed' width='100%'>
	      <tr><td>
		  <xsl:apply-templates select="." mode="personalCore" />
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
		  <xsl:element name="fieldset">
		    <xsl:call-template name="tabLegend" />

		    <xsl:comment>Home information</xsl:comment>
		    <div class="personal_info">
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
		    </div>
		    
		    <xsl:comment>Work information</xsl:comment>
		    <xsl:apply-templates select="." mode="orgAddress" />
		    
		    <div class="other_info">
		      <table>
			<xsl:call-template name="editableAdditionalAddr" />
		      </table>
		      <br />
		    </div>
		    
		    <xsl:apply-templates select="." mode="imInfo" />

		  </xsl:element>
	      </td></tr>
	    </table>
	</td></tr>
      </table><br />
    </xsl:element>
  </xsl:element>
</xsl:template>

<xsl:template match="searchResultEntry" mode="registration" >
  <xsl:param name="updateURI" select="/ldapupdate" />
  <xsl:param name="heading" select="attr[@name='structuralObjectClass' and position() = 1]" />
  <div style="background-color: #F0F0F0; padding: 5px; padding-left: 15px; padding-right: 15px; margin: 10px; border-color: #B0B0B0; border-width: 1px; border-style: solid;" > 
    <ul>
      <li>You must provide a first and last name, email, and password</li>
      <li>Your e-mail address and password are your login credentials</li>
    </ul>
  </div>
  <xsl:element name="div">
    <xsl:attribute name="class">resultDiv</xsl:attribute>
    <xsl:attribute name="nodeid"><xsl:value-of select="generate-id()" /></xsl:attribute>
    <xsl:element name="form"> 
      <xsl:attribute name="name">ChangeInfo</xsl:attribute>
      <xsl:attribute name="method">post</xsl:attribute>
      <xsl:attribute name="onsubmit">return psldap_validateFormInput()</xsl:attribute>
      <xsl:attribute name="action"><xsl:value-of select='$updateURI' /></xsl:attribute>
      <xsl:attribute name="target">processWindow</xsl:attribute>
      <xsl:attribute name="enctype">multipart/form-data</xsl:attribute>

      <br />

      <xsl:apply-templates select="." mode="hiddenAttributes" />

      <table border='0' cellspacing='0'>
	<tr><td>
	    <xsl:apply-templates select="." mode="personalCore" />
	</td></tr>


	<tr><td>
	    <xsl:element name="fieldset">
	      <xsl:comment>Work information</xsl:comment>
	      <xsl:apply-templates select="." mode="orgAddress" />
	      
	      <div class="other_info">
		<table>
		  <tr otherInfo="labeledURI">
		    <xsl:call-template name="editableAttr">
		      <xsl:with-param name="attrType">labeledURI</xsl:with-param>
		      <xsl:with-param name="label">Web Site</xsl:with-param>
		      <xsl:with-param name="width">48</xsl:with-param>
		      <xsl:with-param name="maxwidth">128</xsl:with-param>
		    </xsl:call-template>
		  </tr>
		</table>
		<br />
	      </div>
	      
	    </xsl:element>
	    <xsl:element name="script">
	      <xsl:attribute name="type">text/javascript</xsl:attribute>
	      <xsl:attribute name="language">javascript</xsl:attribute>
	      function showRegistrationDivsCB() {
	        showInfo('none', 'block', 'block', 'block', 'none');
	      }
	      window.setTimeout(showRegistrationDivsCB, 20)
	    </xsl:element>
	</td></tr>
	<tr><td>
	    <table>
	      <tr>
		<xsl:call-template name="editableAttr">
		  <xsl:with-param name="attrType">description</xsl:with-param>
		  <xsl:with-param name="label">Description</xsl:with-param>
		  <xsl:with-param name="width">48</xsl:with-param>
		  <xsl:with-param name="maxwidth">128</xsl:with-param>
		</xsl:call-template>
	      </tr>
	      <tr>
		<xsl:call-template name="editableDeliveryPreference">
		  <xsl:with-param name="attrType">preferredDeliveryMethod</xsl:with-param>
		  <xsl:with-param name="label">Contact Preference</xsl:with-param>
		</xsl:call-template>
	      </tr>
	    </table>
	    <br />
	    <xsl:comment>
	      <xsl:apply-templates select="." mode="imInfo" />
	      <br />  
	    </xsl:comment>
	</td></tr>
      </table><br />
    </xsl:element>
  </xsl:element>
</xsl:template>

<xsl:template match="searchResultEntry" mode="groupOfUniqueNames" >
  <xsl:param name="hidden" select="off" />
  <xsl:param name="updateURI" select="/ldapupdate" />
  <xsl:param name="heading" select="attr[((@name='structuralObjectClass') and (position() = 1))]" />
  <xsl:element name="div">
    <xsl:attribute name="class">resultDiv</xsl:attribute>
    <xsl:attribute name="id"><xsl:value-of select="generate-id(.)" /></xsl:attribute>
    <xsl:if test="($hidden = 'on')">
      <xsl:attribute name="style">display: none;</xsl:attribute>
    </xsl:if>
    <xsl:element name="form"> 
      <xsl:attribute name="name">ChangeInfo</xsl:attribute>
      <xsl:attribute name="method">post</xsl:attribute>
      <xsl:attribute name="action"><xsl:value-of select='$updateURI' /></xsl:attribute>
      <xsl:attribute name="target">processWindow</xsl:attribute>

      <xsl:apply-templates select="." mode="hiddenAttributes" />

      <table >
	<tr><td  style="padding-bottom: 0px; ">
	    <xsl:apply-templates select="." mode="recordHead">
	      <xsl:with-param name='oClass'>Groups</xsl:with-param>
	    </xsl:apply-templates>
	</td></tr>
	<tr><td style="padding-top: 0px; ">
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
    </xsl:element>
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

<xsl:template match="searchResultEntry" mode="recordHead">
  <xsl:param name="oClass" select="attr[@name='structuralObjectClass']/value[(position()=1)]" />
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
                    <xsl:text> - </xsl:text>
                    <xsl:value-of select="attr[@name='cn']/value"/>
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

</xsl:stylesheet>
