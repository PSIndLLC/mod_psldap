<?xml version="1.0" encoding="ISO-8859-1"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/html4/loose.dtd'>

  <xsl:variable name="psldapRoot" select="'/psldap'" />
  
  <xsl:template name="editToolsHeader" >
    <xsl:element name="script">
      <xsl:attribute name="type">text/javascript</xsl:attribute>
      <xsl:attribute name="language">javascript</xsl:attribute>
      <xsl:attribute name="src">/psldap/psajax_docmgr.js</xsl:attribute>
    </xsl:element>
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
      function initialize() {
          window.status = "Loading forms...";
          /* init recordNbrElmt global defined in DSML_psldap.js */
          recordNbrElmt = document.getElementById("recordNumber");
          showRecord(1);
          window.status = getAllFormElements().length + " records loaded";
          sizeWindowToFitDocument(window);
      }
    </xsl:element>
    <xsl:element name="style">
      <xsl:attribute name="type">text/css</xsl:attribute>
      div#processDiv {  z-index: 0; background-color: beige; border: 1px solid silver; margin: 0px; display: none; position: absolute; left: 48px; width: 384px; }
    </xsl:element>
  </xsl:template>
  
  <xsl:template name="processStatusDiv" >
    <xsl:param name="onload" />
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
	<xsl:if test="($onload != '')">
	  <xsl:attribute name="onload"><xsl:value-of select="$onload" /></xsl:attribute>
	</xsl:if>
	<xsl:attribute name="width">100%</xsl:attribute>
	<xsl:attribute name="style">border: none</xsl:attribute>
      </xsl:element>
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
    <xsl:param name="validate" />
    <xsl:param name="width" select="20" />
    <xsl:param name="maxwidth" select="64" />
    <xsl:param name='attrType' select="ancestor-or-self::attr/attribute::name" />
    <xsl:param name='default' select="." />

    <xsl:element name="input">
      <xsl:attribute name="type">password</xsl:attribute>
      <xsl:if test="($validate != '')">
	<xsl:attribute name="psValidate"><xsl:value-of select="$validate" /></xsl:attribute>
      </xsl:if>
      <xsl:attribute name="name"><xsl:value-of select='$attrType' /><xsl:value-of select='$suffix' /></xsl:attribute>
      <xsl:attribute name="size"><xsl:value-of select='$width' /></xsl:attribute>
      <xsl:attribute name="maxlength"><xsl:value-of select='$maxwidth' /></xsl:attribute>
      <xsl:attribute name="value"><xsl:value-of select='$default' /></xsl:attribute>
    </xsl:element>
  </xsl:template>

  <xsl:template name="textinput">
    <xsl:param name="validate" />
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
	<xsl:if test="($validate != '')">
	  <xsl:attribute name="psValidate"><xsl:value-of select="$validate" /></xsl:attribute>
	</xsl:if>
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

  <xsl:template name="editableAttr">
    <xsl:param name="attrType" select="attr/attribute::name" />
    <xsl:param name="label" select="attr/attribute::name" />
    <xsl:param name="validate" />
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
          <xsl:with-param name="validate"><xsl:value-of select="$validate" /></xsl:with-param>
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
            <xsl:with-param name="validate"><xsl:value-of select="$validate" /></xsl:with-param>
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
    <xsl:param name="validate" />
    <xsl:param name="label" select="@name" />
    <xsl:param name="width" select="20" />
    <xsl:param name="maxwidth" select="64" />
    <xsl:param name="attrType" select="@name" />

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
	      <xsl:with-param name='validate'><xsl:value-of select='$validate' /></xsl:with-param>
              <xsl:with-param name='width'><xsl:value-of select='$width' /></xsl:with-param>
              <xsl:with-param name='maxwidth'><xsl:value-of select='$maxwidth' /></xsl:with-param>
              <xsl:with-param name='suffix'>-1</xsl:with-param>
              <xsl:with-param name='attrType'><xsl:value-of select='$attrType' /></xsl:with-param>
              <xsl:with-param name='default'></xsl:with-param>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="textinput">
	      <xsl:with-param name='validate'><xsl:value-of select='$validate' /></xsl:with-param>
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
	      <xsl:with-param name='validate'><xsl:value-of select='$validate' /></xsl:with-param>
              <xsl:with-param name='width'><xsl:value-of select='$width' /></xsl:with-param>
              <xsl:with-param name='maxwidth'><xsl:value-of select='$maxwidth' /></xsl:with-param>
            </xsl:call-template>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="textinput">
	      <xsl:with-param name="validate"><xsl:value-of select="$validate" /></xsl:with-param>
              <xsl:with-param name="suffix"><xsl:number value="position()" format="-1" /></xsl:with-param>
              <xsl:with-param name='width'><xsl:value-of select='$width' /></xsl:with-param>
              <xsl:with-param name='maxwidth'><xsl:value-of select='$maxwidth' /></xsl:with-param>
            </xsl:call-template>
          </xsl:otherwise>
	</xsl:choose>
      </xsl:for-each>
    </td>
  </xsl:template>

  <xsl:template match="searchResultEntry" mode="hiddenAttributes" >
    <xsl:if test="((attribute::dn='') or (attribute::dn='dc=registered'))">
      <xsl:apply-templates select="attr[@name='objectClass']" mode="hidden" />
    </xsl:if>
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
      <xsl:attribute name="value"><xsl:value-of select="$psldapRoot" />/DSML_response.xsl</xsl:attribute>
    </xsl:element>
  </xsl:template>
  
  <xsl:template match="searchResultEntry" mode="orgAddress">
    <xsl:element name="div">
      <xsl:attribute name="class">work_info</xsl:attribute>
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

  <xsl:template match="searchResultEntry" mode="imInfo">
    <xsl:if test="attr[@name='objectClass']/value='imIdObject'">
      <div class="im_info">
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
      </div>
    </xsl:if>
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
