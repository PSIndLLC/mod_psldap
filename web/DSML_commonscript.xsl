<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/REC-html40'>
  
  <xsl:param name="UserAgent">Unknown</xsl:param>
  <xsl:param name="isHandheld"><xsl:choose><xsl:when test="($UserAgent='BlackBerry')" >true</xsl:when><xsl:otherwise>false</xsl:otherwise></xsl:choose></xsl:param>

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
    <xsl:element name="a">
      <xsl:attribute name="target">im_window</xsl:attribute>
      <xsl:attribute name="href">im://<xsl:value-of select="."/></xsl:attribute>
      <xsl:value-of select="."/>&nbsp;
    </xsl:element>
  </xsl:template>
  
  <xsl:template match="attr[@name='yahooId']/value"  mode="chat-l">
    <xsl:element name="a">
      <xsl:attribute name="target">im_window</xsl:attribute>
      <xsl:attribute name="href">http://edit.yahoo.com/config/send_webmesg?.target=<xsl:value-of select="."/>&amp;.src=pg</xsl:attribute>
      <xsl:value-of select="."/>&nbsp;
      
      <xsl:element name="img">
	<xsl:attribute name="width">80</xsl:attribute>
	<xsl:attribute name="border">0</xsl:attribute>
	<xsl:attribute name="src">http://opi.yahoo.com/online?u=<xsl:value-of select="."/>&amp;m=g&amp;t=2</xsl:attribute>
      </xsl:element>
    </xsl:element>
  </xsl:template>
  
  <xsl:template match="attr[@name='aimId']/value" mode="chat-l">
    <xsl:element name="a">
      <xsl:attribute name="target">im_window</xsl:attribute>
      <xsl:attribute name="href">aim:goim?screenname=<xsl:value-of select="."/>&amp;message=hi.+are+you+there?</xsl:attribute>
      <xsl:value-of select="."/>&nbsp;
      
      <xsl:element name="img">
	<xsl:attribute name="src">http://big.oscar.aol.com/<xsl:value-of select="."/>?on_url=http://www.aim.com/remote/gr/MNB_online.gif&amp;off_url=http://www.aim.com/remote/gr/MNB_offline.gif</xsl:attribute>
	<xsl:attribute name="border">0</xsl:attribute>
	<xsl:attribute name="width">11</xsl:attribute>
	<xsl:attribute name="height">13</xsl:attribute>
    </xsl:element>
    </xsl:element>
  </xsl:template>
  
  <xsl:template match="attr[@name='skypeId']/value" mode="chat-l">
    <xsl:element name="a">
      <xsl:attribute name="target">im_window</xsl:attribute>
      <xsl:attribute name="href">skype://<xsl:value-of select="."/>?call</xsl:attribute>
      <xsl:value-of select="."/>&nbsp;
      <xsl:element name="img">
	<xsl:attribute name="src">http://mystatus.skype.com/bigclassic/<xsl:value-of select="." /></xsl:attribute>
	<xsl:attribute name="style">border: none;</xsl:attribute>
	<xsl:attribute name="height">12</xsl:attribute>
	<xsl:attribute name="alt">My status</xsl:attribute>
      </xsl:element>
    </xsl:element>
  </xsl:template>
  
  <xsl:template match="attr[@name='imId' or @name='yahooId' or @name='aimId' or @name='skypeId' ]/value" mode="chat">
    <xsl:if test="(not (position() = 1))">
      <br />
    </xsl:if>
    <xsl:apply-templates select="." mode="chat-l" />
  </xsl:template>
  
</xsl:stylesheet>

