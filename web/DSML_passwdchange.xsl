<?xml version="1.0" encoding="ISO-8859-1"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/html4/loose.dtd'>
  
  <xsl:include href="DSML_commonscript.xsl" />
  <xsl:include href="DSML_edittools.xsl" />
  <xsl:include href="DSML_sitefrags.xsl" />
  
  <xsl:output method="html" doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN" doctype-system="http://www.w3.org/TR/html4/loose.dtd" omit-xml-declaration="no" media-type="text/html" />
  
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
               sizeWindowToFitDocument(window);
	       }
      ]]>
    </xsl:element>
  </xsl:template>

  <xsl:template name="passwdChange" match="searchResultEntry" >
    <xsl:param name="updateURI" select="$v_updateURI" />
    <xsl:element name="p">Submission of this form will set the password for the user / uid (<xsl:value-of select="attr[@name='givenName']/value"/><xsl:text> </xsl:text><xsl:value-of select="attr[@name='sn']/value" /> / <xsl:value-of select="attr[@name='uid']/value"/>).</xsl:element>
    <xsl:element name="p">If you do not wish to reset this password, exit this page now.</xsl:element>
    
    <xsl:element name="form" > 
      <xsl:attribute name="name">ChangePassword</xsl:attribute>
      <xsl:attribute name="method">post</xsl:attribute>
      <xsl:attribute name="action"><xsl:value-of select='$updateURI' /></xsl:attribute>
      <xsl:attribute name="target">processWindow</xsl:attribute>
      <xsl:attribute name="enctype">multipart/form-data</xsl:attribute>
      
      <xsl:apply-templates select="." mode="hiddenAttributes" />
      
      <xsl:element name="table">
	<xsl:element name="tr">
          <xsl:element name="td">
	    <xsl:attribute name="align">RIGHT</xsl:attribute>
	    <xsl:element name="label">User ID (uid)
            <xsl:element name="input"><xsl:attribute name="type">TEXT</xsl:attribute><xsl:attribute name="name">uid</xsl:attribute><xsl:attribute name="size">20</xsl:attribute><xsl:attribute name="maxlength">64</xsl:attribute><xsl:attribute name="disabled" /><xsl:attribute name="value"><xsl:value-of select="attr[@name='uid']/value"/></xsl:attribute><xsl:attribute name="required" /></xsl:element>
	    </xsl:element>
	  </xsl:element>
	</xsl:element>
	<xsl:element name="tr">
          <xsl:element name="td">
	    <xsl:attribute name="align">RIGHT</xsl:attribute>
	    <xsl:element name="label">New Password  <xsl:element name="input"><xsl:attribute name="type">PASSWORD</xsl:attribute><xsl:attribute name="name">userPassword-1</xsl:attribute><xsl:attribute name="size">20</xsl:attribute><xsl:attribute name="maxlength">64</xsl:attribute><xsl:attribute name="value"></xsl:attribute><xsl:attribute name="required" /></xsl:element></xsl:element>
	  </xsl:element>
	<xsl:element name="tr">
          <xsl:element name="td">
	    <xsl:attribute name="align">RIGHT</xsl:attribute>
	    <xsl:element name="label">Confirm Password  <xsl:element name="input"><xsl:attribute name="type">PASSWORD</xsl:attribute><xsl:attribute name="name">userPassword-2</xsl:attribute><xsl:attribute name="size">20</xsl:attribute><xsl:attribute name="maxlength">64</xsl:attribute><xsl:attribute name="value"></xsl:attribute><xsl:attribute name="required" /></xsl:element></xsl:element>
	  </xsl:element>
	</xsl:element>
	<xsl:element name="tr">
	  <xsl:element name="td">
	    <xsl:attribute name="align">CENTER</xsl:attribute>
	    <xsl:attribute name="style">text-align: center; padding: 16px;</xsl:attribute>
	    <xsl:element name="input">
	      <xsl:attribute name="type">submit</xsl:attribute>
	      <xsl:attribute name="value">Password Change</xsl:attribute>
	      <xsl:attribute name="onClick">submitVisibleRecord('PasswordChange')</xsl:attribute >
	    </xsl:element>
	  </xsl:element>
	  <xsl:element name="td">
	    <xsl:attribute name="align">CENTER</xsl:attribute>
	    <xsl:attribute name="style">text-align: center; padding: 16px;</xsl:attribute>
	    <xsl:element name="input">
	      <xsl:attribute name="type">reset</xsl:attribute>
	      <xsl:attribute name="value">Cancel</xsl:attribute>
	    </xsl:element>
	  </xsl:element>
	</xsl:element>
	</xsl:element>
      </xsl:element>
    </xsl:element>
  </xsl:template>
  
  <xsl:template match="/dsml">
    <xsl:variable name="recordCount" select="count(//searchResponse/searchResultEntry)" />
    <xsl:text>
    </xsl:text>
    <xsl:element name="html">
      <xsl:call-template name="pageHeaderWithRefClass" >
	<xsl:with-param name='title'>Change Password</xsl:with-param>
      </xsl:call-template>
      
      <xsl:element name="body">
	<xsl:attribute name="onload">initialize()</xsl:attribute>
	<xsl:apply-templates select="batchResponse/searchResponse/searchResultEntry" name="passwdChange" />
      </xsl:element>
    </xsl:element>
  </xsl:template>
  
</xsl:stylesheet>
