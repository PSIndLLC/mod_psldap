<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/REC-html40'>

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
   <head>
    <link rel="STYLESHEET" type="text/css" media="screen" href="/psldap/DSML_psldap.css" />
    <xsl:element name="script">
      <xsl:attribute name="language">JavaScript</xsl:attribute>
      <xsl:attribute name="src">/psldap/DSML_psldap.js</xsl:attribute>
    </xsl:element>
    <xsl:apply-templates select="//searchResponse" mode="refClass">
      <xsl:with-param name="refClass">organization</xsl:with-param>
      <xsl:with-param name="refClassAttr">o</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="//searchResponse" mode="refClass">
      <xsl:with-param name="refClass">organizationalUnit</xsl:with-param>
      <xsl:with-param name="refClassAttr">ou</xsl:with-param>
    </xsl:apply-templates>
    <title>Search Results</title>
   </head>
   <body onload="initializeCards(8)">
     <form action="CreateGroup" method="post">
       <table id='cardTable'>
	 <tr class="resultRow">
	   <xsl:apply-templates select="//searchResponse" />
	 </tr>
       </table>
     </form>
   </body>
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
      <xsl:for-each select="./value">
        <xsl:sort select="." />
        <xsl:if test="(not (position() = 1))">
          <br />
        </xsl:if>
        <xsl:element name="a">
          <xsl:attribute name="target">im_window</xsl:attribute>
          <xsl:value-of select="."/>&nbsp;
        <xsl:if test="($imtype = 'skypeId')">
          <xsl:attribute name="href">skype://<xsl:value-of select="."/></xsl:attribute>
        </xsl:if>
        <xsl:if test="($imtype = 'yahooId')">
          <xsl:attribute name="href">http://edit.yahoo.com/config/send_webmesg?.target=<xsl:value-of select="."/>&amp;.src=pg</xsl:attribute>
          <xsl:element name="img">
            <xsl:attribute name="width">80</xsl:attribute>
            <xsl:attribute name="border">0</xsl:attribute>
            <xsl:attribute name="src">http://opi.yahoo.com/online?u=cptdjpicard&amp;m=g&amp;t=2</xsl:attribute>
          </xsl:element>
        </xsl:if>
        <xsl:if test="($imtype = 'aimId')">
          <xsl:attribute name="href">aim:goim?screenname=<xsl:value-of select="."/>&amp;message=hi.+are+you+there?</xsl:attribute>
          <xsl:element name="img">
            <xsl:attribute name="src">http://big.oscar.aol.com/cptdpicard?on_url=http://www.aim.com/remote/gr/MNB_online.gif&amp;off_url=http://www.aim.com/remote/gr/MNB_offline.gif</xsl:attribute>
            <xsl:attribute name="border">0</xsl:attribute>
            <xsl:attribute name="width">11</xsl:attribute>
            <xsl:attribute name="height">13</xsl:attribute>
          </xsl:element>
        </xsl:if>
        <xsl:if test="($imtype = 'imId')">
          <xsl:attribute name="href">im://<xsl:value-of select="."/></xsl:attribute>
        </xsl:if>
        </xsl:element>
      </xsl:for-each>
    </td>
  </tr>
</xsl:template>

<xsl:template match="attr[@name='telephoneNumber' or @name='homePhone' or @name='mobilePhone' ]" mode="foo" >
  <xsl:param name='label' select='@name' />
  <xsl:variable name='attrType' select='@name' />
  <tr>
    <td class="label" noWrap="true" >
      <xsl:value-of select='$label' />
    </td>
    <td class="data">
      <xsl:for-each select='./value'>
	<xsl:sort select='.' />
	<xsl:element name="span">
	  <xsl:attribute name="isdynflag">1</xsl:attribute>
	  <xsl:attribute name="info">Call <xsl:value-of select="."/>;0;<xsl:value-of select="."/>;0;</xsl:attribute>
	  <xsl:attribute name="onmouseup">SkypeSetCallButtonPressed(this, 0,0,0)</xsl:attribute>
	  <xsl:attribute name="onmousedown">SkypeSetCallButtonPressed(this, 1,0,0)</xsl:attribute>
	  <xsl:attribute name="onmouseover">SkypeSetCallButton(this, 1,0,0);skype_active=SkypeCheckCallButton(this);</xsl:attribute>
	  <xsl:attribute name="onmouseout">SkypeSetCallButton(this, 0,0,0);HideSkypeMenu();</xsl:attribute>
	  <xsl:attribute name="context"><xsl:value-of select="."/></xsl:attribute>
	  <xsl:attribute name="fax">0</xsl:attribute>
	  <xsl:attribute name="rtl">false</xsl:attribute>
	  <xsl:attribute name="class">skype_tb_injection</xsl:attribute>
	  <xsl:attribute name="id">__skype_highlight_id"</xsl:attribute>
	  <span title="Skype actions" onmouseout="SkypeSetCallButtonPart(this, 0);" onmouseover="SkypeSetCallButtonPart(this, 1);" class="skype_tb_injection_left" id="__skype_highlight_id_left">
	    <span style="background-image: url(chrome://skype_ff_toolbar_win/content/cb_normal_l.gif);" class="skype_tb_injection_left_img" id="__skype_highlight_id_left_adge">
	      <img src="chrome://skype_ff_toolbar_win/content/cb_transparent_l.gif" style="height: 11px; width: 7px;" class="skype_tb_img_adge" height="11" />
	    </span>
	    <span style="background-image: url(chrome://skype_ff_toolbar_win/content/cb_normal_m.gif);" class="skype_tb_injection_left_img" id="__skype_highlight_id_left_img">
	      <img style="padding: 0px 1px 1px 0px; width: 16px; top: 0px; left: 0px;" src="chrome://skype_ff_toolbar_win/content/famfamfam/ca.gif" title="" class="skype_tb_img_flag" name="skype_tb_img_f0" />
	      <img src="chrome://skype_ff_toolbar_win/content/space.gif" style="margin: 0px; padding: 0px; height: 1px; width: 1px;" class="skype_tb_img_space" width="1" height="1" />
	      <img src="chrome://skype_ff_toolbar_win/content/space.gif" style="margin: 0px; padding: 0px; height: 1px; width: 1px;" class="skype_tb_img_space" width="1" height="1" />
	      <img src="chrome://skype_ff_toolbar_win/content/arrow.gif" title="" class="skype_tb_img_arrow" name="skype_tb_img_a0" />
	      <img src="chrome://skype_ff_toolbar_win/content/space.gif" style="margin: 0px; padding: 0px; height: 1px; width: 1px;" class="skype_tb_img_space" width="1" height="1" />
	      <img src="chrome://skype_ff_toolbar_win/content/space.gif" style="margin: 0px; padding: 0px; height: 1px; width: 1px;" class="skype_tb_img_space" width="1" height="1" />
	    </span>
	  </span>
	  <img src="chrome://skype_ff_toolbar_win/content/space.gif" style="margin: 0px; padding: 0px; height: 1px; width: 1px;" class="skype_tb_img_space" width="1" height="1" />
	  <xsl:element name="span">
	    <xsl:attribute name="title">Call this phone number in United States with Skype: <xsl:value-of select="."/></xsl:attribute>
	    <xsl:attribute name="onmouseout">SkypeSetCallButtonPart(this, 0)</xsl:attribute>
	    <xsl:attribute name="onmouseover">SkypeSetCallButtonPart(this, 1)</xsl:attribute>
	    <xsl:attribute name="class">skype_tb_injection_right</xsl:attribute>
	    <xsl:attribute name="id">__skype_highlight_id_right</xsl:attribute>
	    <span style="background-image: url(chrome://skype_ff_toolbar_win/content/cb_normal_m.gif);" class="skype_tb_innerText" id="__skype_highlight_id_innerText">
	      <img src="chrome://skype_ff_toolbar_win/content/space.gif" style="margin: 0px; padding: 0px; height: 1px; width: 1px;" class="skype_tb_img_space" width="1" height="1" />
	      <img src="chrome://skype_ff_toolbar_win/content/space.gif" style="margin: 0px; padding: 0px; height: 1px; width: 1px;" class="skype_tb_img_space" width="1" height="1" />
	      <img src="chrome://skype_ff_toolbar_win/content/space.gif" style="margin: 0px; padding: 0px; height: 1px; width: 1px;" class="skype_tb_img_space" width="1" height="1" />
	      <img src="chrome://skype_ff_toolbar_win/content/space.gif" style="margin: 0px; padding: 0px; height: 1px; width: 1px;" class="skype_tb_img_space" width="1" height="1" />
	      <xsl:value-of select="."/>
	    </span>
	    <span style="background-image: url(chrome://skype_ff_toolbar_win/content/cb_normal_r.gif);" class="skype_tb_injection_left_img" id="__skype_highlight_id_right_adge">
	      <img src="chrome://skype_ff_toolbar_win/content/cb_transparent_r.gif" style="height: 11px; width: 19px;" class="skype_tb_img_adge" height="11" />
	    </span>
	  </xsl:element>
	</xsl:element>
      </xsl:for-each>
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
      <xsl:for-each select='./value'>
	<xsl:sort select='.' />
	<xsl:if test="(not (position() = 1))">
	  <br />
	</xsl:if>
	<xsl:element name="a">
	  <xsl:attribute name="href">skype:<xsl:if test="(not (starts-with(.,'('))) and (not (starts-with(.,'+')))">+</xsl:if><xsl:if test="(starts-with(.,'('))">+1</xsl:if><xsl:value-of select="translate(translate(translate(translate(.,' ',''),'(',''),')',''),'-','')"/>?call</xsl:attribute>
	  <xsl:attribute name="onclick">return skypeCheck();</xsl:attribute>
	  <xsl:value-of select="."/>
	</xsl:element>
      </xsl:for-each>
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
  <table name='cardInstance' width='100%' border='0' cellspacing='0'>
    <tr class="menubar">
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
	  <td width="64" noWrap="true" >
	    <xsl:element name="a">
	      <xsl:attribute name="href">javascript: void getEditableRecord("<xsl:value-of select="@dn"/>");</xsl:attribute>
	      <xsl:element name="img">
		<xsl:attribute name="src">/psldap/images/editRecord_sm.gif</xsl:attribute>
		<xsl:attribute name="style">margin-top: 3px; margin-bottom: 0px;</xsl:attribute>
	      </xsl:element>
	    </xsl:element>
	    <xsl:comment />
	    <xsl:element name="a">
	      <xsl:attribute name="href">javascript: void getVCard("<xsl:value-of select="@dn"/>");</xsl:attribute>
	      <xsl:element name="img">
		<xsl:attribute name="src">/psldap/images/vcard.gif</xsl:attribute>
		<xsl:attribute name="style">margin-top: 3px; margin-bottom: 0px;</xsl:attribute>
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
