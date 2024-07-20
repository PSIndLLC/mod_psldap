<?xml version="1.0" encoding="ISO-8859-1"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/1999/xhtml'>

<xsl:include href="DSML_commonscript.xsl" />
<xsl:include href="DSML_sitefrags.xsl" />

<xsl:output method="html" doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN" doctype-system="http://www.w3.org/TR/xhtml1l/DTD/transitional.dtd" omit-xml-declaration="no" media-type="text/html" />

<xsl:key name="searchEntryLookup" match="searchResultEntry" use="@dn" />

<xsl:template name="pageSpecificHeader" >
  <xsl:element name="script">
    <xsl:attribute name="type">text/javascript</xsl:attribute>
    <xsl:attribute name="language">javascript</xsl:attribute>
    <![CDATA[
        function synchSelectData() {
            var selectElmts = null;
/*
	    window.alert(ldapSchema.ldapSyntaxes.ldapSyntaxes[0].oid);
	    window.alert(ldapSchema.matchingRules.matchingRules[0].oid);
	    window.alert(ldapSchema.matchingRuleUse.matchingRuleUse[0].oid);
	    window.alert(ldapSchema.objectClasses.objectClasses[0].oid);
*/
        }
    ]]>
  </xsl:element>
</xsl:template>

<xsl:template name="escapeQuotes">
  <xsl:param name='myText' />
  <xsl:choose>
    <xsl:when test='contains($myText,"&apos;")' >
      <xsl:call-template name="string-replace-all"><xsl:with-param name='text' select='$myText' /><xsl:with-param name='replace' select='"&apos;"' /><xsl:with-param name='by' select='"&amp;apos;"' /></xsl:call-template>
    </xsl:when>
    <xsl:when test="contains($myText,'&quot;')" >
      <xsl:call-template name="string-replace-all"><xsl:with-param name='text' select='$myText' /><xsl:with-param name='replace' select="'&quot;'" /><xsl:with-param name='by' select="'&amp;quot;'" /></xsl:call-template>
    </xsl:when>
    <xsl:otherwise>  
      <xsl:value-of select='$myText' />
    </xsl:otherwise>  
  </xsl:choose>
</xsl:template>

<xsl:template match="/dsml">
  <html>
    <xsl:call-template name="pageHeader" >
      <xsl:with-param name="title">PsLDAP Schema Definitions</xsl:with-param>
      <xsl:with-param name="ua"><xsl:value-of select='$UserAgent' /></xsl:with-param>
    </xsl:call-template>

    <xsl:element name="body">
      <xsl:if test="($isHandheld='false')">
	<xsl:attribute name="onload">synchSelectData()</xsl:attribute>
      </xsl:if>
      <xsl:element name="script">
	<xsl:attribute name="type">text/javascript</xsl:attribute>
	<xsl:attribute name="language">javascript</xsl:attribute>
	var ldapSchema = { "ldapSyntaxes": [], "matchingRules": [], "matchingRuleUse": [], "attributeTypes": [], "objectClasses": [] };
      </xsl:element>
      <form action="CreateSchemaObject" method="post">
	<table id='schemaTable' class="boxed" width="*">
	    <xsl:apply-templates select="//searchResponse/searchResultEntry" />
	</table>
      </form>
    </xsl:element>
  </html>
</xsl:template>

<xsl:template match="attr" >
  <xsl:param name='label' select='@name' />
  <xsl:param name='objName' select='@name' />
  <xsl:param name='syntaxRef'>ldapSyntaxes</xsl:param>
  <xsl:param name='showHtml' select='true' />
  <xsl:param name='buildJSON' select='false'/>
  <xsl:variable name='attrType' select='@name' />
  <tr>
    <xsl:if test='$buildJSON'>
      <xsl:element name="script">
	<xsl:attribute name="type">text/javascript</xsl:attribute>
	<xsl:attribute name="language">javascript</xsl:attribute>
        var <xsl:value-of select='$objName' />_obj_str = '{ "<xsl:value-of select='$objName' />" : [';
      </xsl:element>
    </xsl:if>

    <td class="label" noWrap="true" >
      <xsl:value-of select='$label' />
    </td>
    <td class="data">
      <xsl:for-each select='./value'>
        <xsl:sort select='.' />
	<xsl:variable name='oid' select='substring-before(substring-after(.,"( ")," ")' />
	<xsl:variable name='nm_single' select='substring-before(substring-after(.,"NAME &apos;"),"&apos;")' />
	<xsl:variable name='nm'>
	  <xsl:choose>
	    <xsl:when test='not($nm_single="")'><xsl:value-of select='$nm_single' /></xsl:when>
	    <xsl:otherwise><xsl:value-of select='substring-before(substring-after(.,"NAME ( ")," ) ")' /></xsl:otherwise>
	  </xsl:choose>
	</xsl:variable>
	<xsl:variable name='desc' select='substring-before(substring-after(.,"DESC &apos;"),"&apos;")' />
	<xsl:variable name='xorder' select='substring-before(substring-after(.,"X-ORDERED &apos;"),"&apos;")' />
	<xsl:variable name='xbin' select='substring-before(substring-after(.,"X-BINARY-TRANSFER-REQUIRED &apos;"),"&apos;")' />
	<xsl:variable name='xnhr' select='substring-before(substring-after(.,"X-NOT-HUMAN-READABLE &apos;"),"&apos;")' />
	<xsl:variable name='syntax' select='substring-before(substring-after(.,"SYNTAX ")," ")' />
	<xsl:variable name='eq' select='substring-before(substring-after(.,"EQUALITY ")," ")' />
	<xsl:variable name='ss' select='substring-before(substring-after(.,"SUBSTR ")," ")' />
	<xsl:variable name='ordering' select='substring-before(substring-after(.,"ORDERING ")," ")' />
	<xsl:variable name='usage' select='substring-before(substring-after(.,"USAGE ")," ")' />
	<xsl:variable name='applies_multi' select='substring-before(substring-after(.,"APPLIES ( ")," )")' />
	<xsl:variable name='applies'>
	  <xsl:choose>
	    <xsl:when test='not($applies_multi="")'><xsl:value-of select='$applies_multi' /></xsl:when>
	    <xsl:otherwise><xsl:value-of select='substring-before(substring-after(.,"APPLIES ")," ")' /></xsl:otherwise>
	  </xsl:choose>
	</xsl:variable>
	<xsl:variable name='sup_multi' select='substring-before(substring-after(.,"SUP ( ")," )")' />
	<xsl:variable name='sup'>
	  <xsl:choose>
	    <xsl:when test='not($sup_multi="")'><xsl:value-of select='$sup_multi' /></xsl:when>
	    <xsl:otherwise><xsl:value-of select='substring-before(substring-after(.,"SUP ")," ")' /></xsl:otherwise>
	  </xsl:choose>
	</xsl:variable>
	<xsl:variable name='must_multi' select='substring-before(substring-after(.,"MUST ( ")," )")' />
	<xsl:variable name='must'>
	  <xsl:choose>
	    <xsl:when test='not($must_multi="")'><xsl:value-of select='$must_multi' /></xsl:when>
	    <xsl:otherwise><xsl:value-of select='substring-before(substring-after(.,"MUST ")," ")' /></xsl:otherwise>
	  </xsl:choose>
	</xsl:variable>
	<xsl:variable name='may_multi' select='substring-before(substring-after(.,"MAY ( ")," )")' />
	<xsl:variable name='may'>
	  <xsl:choose>
	    <xsl:when test='not($may_multi="")'><xsl:value-of select='$may_multi' /></xsl:when>
	    <xsl:otherwise><xsl:value-of select='substring-before(substring-after(.,"MAY ")," ")' /></xsl:otherwise>
	  </xsl:choose>
	</xsl:variable>
	<xsl:variable name='sv' select='contains(.," SINGLE-VALUE ")' />
	<xsl:variable name='noum' select='contains(.," NO-USER-MODIFICATION ")' />
	<xsl:variable name='aux' select='contains(.," AUXILIARY ")' />
	<xsl:variable name='struct' select='contains(.," STRUCTURAL ")' />
        <xsl:if test="(not (position() = 1))">
          <br />
        </xsl:if>

	<xsl:if test='$showHtml'>
          <xsl:element name="span">
	    <xsl:attribute name="title"><xsl:choose><xsl:when test='string-length($desc)!=0'><xsl:value-of select='$oid' />: <xsl:value-of select='$desc' /></xsl:when><xsl:otherwise><xsl:value-of select='.' /></xsl:otherwise></xsl:choose></xsl:attribute>
	    <xsl:choose>
	      <xsl:when test='not($nm="")'>
		<xsl:value-of select='$nm' />
	      </xsl:when>
	      <xsl:when test='not($desc="")'>
		<xsl:value-of select='$desc' />
	      </xsl:when>
	      <xsl:otherwise>
		<xsl:value-of select='$oid' />
	      </xsl:otherwise>
	    </xsl:choose>
	    <xsl:element name="span">
	      <xsl:attribute name="style">background-color: #D8D8D8; margin-left: 18px;</xsl:attribute>
	      <xsl:if test='$xbin="TRUE"'><xsl:element name="span"><xsl:attribute name="title">Binary Transfer Required</xsl:attribute>BTR </xsl:element></xsl:if><xsl:if test='$xnhr="TRUE"'><xsl:element name="span"><xsl:attribute name="title">Not Human Readable</xsl:attribute>NoHR </xsl:element></xsl:if><xsl:if test='$noum'><xsl:element name="span"><xsl:attribute name="title">No User Modification</xsl:attribute>NoUM </xsl:element></xsl:if><xsl:if test='$sv'><xsl:element name="span"><xsl:attribute name="title">Single Value</xsl:attribute>SV </xsl:element> </xsl:if><xsl:if test='$xorder'><xsl:element name="span"><xsl:attribute name="title">X-Order By <xsl:value-of select='$xorder' /></xsl:attribute>XO </xsl:element></xsl:if>
	    </xsl:element>
	    <xsl:if test='not($must="")'>
	      <xsl:element name="span">
		<xsl:attribute name="title"><xsl:value-of select='$must' /></xsl:attribute>
		<xsl:attribute name="style">background-color: #FF6060; margin-left: 18px;</xsl:attribute>
		MUST
	      </xsl:element>
	    </xsl:if>
	    <xsl:if test='not($may="")'>
	      <xsl:element name="span">
		<xsl:attribute name="title"><xsl:value-of select='$may' /></xsl:attribute>
		<xsl:attribute name="style">background-color: #FFFF60; margin-left: 18px;</xsl:attribute>
		MAY
	      </xsl:element>
	    </xsl:if>
	  </xsl:element>
	</xsl:if>

	<xsl:if test='$buildJSON'>
	  <xsl:element name="script">
	    <xsl:attribute name="type">text/javascript</xsl:attribute>
	    <xsl:attribute name="language">javascript</xsl:attribute>
	    <xsl:value-of select='$objName' />_obj_str = <xsl:value-of select='$objName' />_obj_str + 
	    '<xsl:if test='position() &gt; 1'>, </xsl:if>{ "oid":"<xsl:value-of select='$oid' />","name":"<xsl:call-template name="escapeQuotes"><xsl:with-param name='myText' select='$nm' /></xsl:call-template>","desc":"<xsl:call-template name="escapeQuotes"><xsl:with-param name='myText' select='$desc' /></xsl:call-template>","syntax":"<xsl:value-of select='$syntax' />","single_val":<xsl:value-of select='$sv' />,"super":"<xsl:call-template name="escapeQuotes"><xsl:with-param name='myText' select='$sup' /></xsl:call-template>","must":"<xsl:call-template name="escapeQuotes"><xsl:with-param name='myText' select='$must' /></xsl:call-template>","may":"<xsl:call-template name="escapeQuotes"><xsl:with-param name='myText' select='$may' /></xsl:call-template>","objType":"<xsl:choose><xsl:when test='$aux = "true"' >AUX</xsl:when><xsl:when test='$struct = "true"' >STRUCT</xsl:when><xsl:otherwise></xsl:otherwise></xsl:choose>"}';
	  </xsl:element>
	</xsl:if>

      </xsl:for-each>
    </td>
    <xsl:if test='$buildJSON'>
      <xsl:element name="script">
	<xsl:attribute name="type">text/javascript</xsl:attribute>
	<xsl:attribute name="language">javascript</xsl:attribute>
	<xsl:value-of select='$objName' />_obj_str = <xsl:value-of select='$objName' />_obj_str + ' ]}';
	ldapSchema.<xsl:value-of select='$objName' /> = JSON.parse (<xsl:value-of select='$objName' />_obj_str);
      </xsl:element>
    </xsl:if>
  </tr>
</xsl:template>

<xsl:template match="searchResultEntry">
  <xsl:param name="heading" select="attr[@name='entryDN']/value" />
  <xsl:if test="string-length($heading)!=0">
    <tr><th colspan="2" >
      <h2><xsl:value-of select="$heading" /></h2>
    </th></tr>
  </xsl:if>
  
  <xsl:apply-templates select="attr[@name='ldapSyntaxes']" >
    <xsl:sort select="value" />
    <xsl:with-param name='label'>Syntaxes</xsl:with-param>
    <xsl:with-param name='showHtml'>true</xsl:with-param>
    <xsl:with-param name='buildJSON'>true</xsl:with-param>
  </xsl:apply-templates>
  <xsl:apply-templates select="attr[@name='matchingRules']" >
    <xsl:sort select="value" />
    <xsl:with-param name='label'>Matching Rules</xsl:with-param>
    <xsl:with-param name='showHtml'>true</xsl:with-param>
    <xsl:with-param name='buildJSON'>true</xsl:with-param>
  </xsl:apply-templates>
  <xsl:apply-templates select="attr[@name='matchingRuleUse']" >
    <xsl:sort select="value" />
    <xsl:with-param name='label'>Matching Rule Use</xsl:with-param>
    <xsl:with-param name='syntaxRef'></xsl:with-param>
    <xsl:with-param name='showHtml'>true</xsl:with-param>
    <xsl:with-param name='buildJSON'>true</xsl:with-param>
  </xsl:apply-templates>
  <xsl:apply-templates select="attr[@name='attributeTypes']" >
    <xsl:sort select="value" />
    <xsl:with-param name='label'>Attribute Types</xsl:with-param>
    <xsl:with-param name='showHtml'>true</xsl:with-param>
    <xsl:with-param name='buildJSON'>true</xsl:with-param>
  </xsl:apply-templates>
  <xsl:apply-templates select="attr[@name='objectClasses']" >
    <xsl:sort select="value" />
    <xsl:with-param name='label'>Object Classes</xsl:with-param>
    <xsl:with-param name='showHtml'>true</xsl:with-param>
    <xsl:with-param name='buildJSON'>true</xsl:with-param>
  </xsl:apply-templates>
</xsl:template>
</xsl:stylesheet>
