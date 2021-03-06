<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet >
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" >

<xsl:key name="searchEntryLookup" match="searchResultEntry" use="@dn" />
<xsl:output method="text" indent="no" omit-xml-declaration="yes" media-type="text/x-vcard" />

<xsl:template match="/">
    <xsl:apply-templates select="//dsml" />
</xsl:template>

<xsl:template match="/dsml">
  <xsl:text>BEGIN:VCARD</xsl:text>
  <xsl:text>VERSION:2.1</xsl:text>
  <xsl:apply-templates select="//searchResponse" />
  <xsl:text>PRODID:-//MOD_PSLDAP//Version 0.92//EN</xsl:text>
  <xsl:text>REV:</xsl:text><xsl:value-of select="current-dateTime()" />
  <xsl:text>CLASS:PRIVATE</xsl:text>
  <xsl:text>END:VCARD</xsl:text>
</xsl:template>

<xsl:template name="searchResults" match="searchResponse">
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
</xsl:template>

<xsl:template match="attr">
  <xsl:param name='label' select='@name' />
  <xsl:variable name='myvalue' select='.' />
  <xsl:value-of select='$label' /><xsl:if test="(not ($label = ';') )"><xsl:text>:</xsl:text></xsl:if><xsl:value-of select='normalize-space($myvalue)' /><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="attr[@name='sn']">
  <xsl:param name='label' select='@name' />
  <xsl:param name='type' />
  <xsl:variable name='mysn' select="." />
  <xsl:variable name='mygn' select="ancestor-or-self::searchResultEntry/attr[@name='givenName']" />
  <xsl:value-of select='$label' /><xsl:text>:</xsl:text><xsl:value-of select="normalize-space($mysn)" /><xsl:text>;</xsl:text><xsl:if test="(not ($type=''))"><xsl:value-of select='$type' /></xsl:if><xsl:value-of select="normalize-space($mygn)" /><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="searchResultEntry">
  <xsl:param name="heading" select="attr[@name='objectClass' and position() = 1]" />
  <xsl:variable name="jpegLabelType"></xsl:variable>
  <xsl:if test="(starts-with(@dn, 'cn='))" >
    <xsl:apply-templates select="attr[@name='sn']" >
      <xsl:with-param name='label'>N</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='cn']" >
      <xsl:with-param name='label'>FN</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='o']" >
      <xsl:with-param name='label'>ORG</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='ou']" >
      <xsl:with-param name='label'>UNIT</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='description']" >
      <xsl:with-param name='label'>NOTE</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='title']" >
      <xsl:with-param name='label'>TITLE</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='description']" >
      <xsl:with-param name='label'>NOTE</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='mail']" >
      <xsl:with-param name='label'>EMAIL;PREF;INTERNET</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='imId']" >
      <xsl:with-param name='label'>IMPP</xsl:with-param>
      <xsl:with-param name='type'>personal,pref:im</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='yahooId']" >
      <xsl:with-param name='label'>IMPP</xsl:with-param>
      <xsl:with-param name='type'>personal,pref:ymsgr</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='aimId']" >
      <xsl:with-param name='label'>IMPP</xsl:with-param>
      <xsl:with-param name='type'>personal,pref:aim</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='skypeId']" >
      <xsl:with-param name='label'>IMPP</xsl:with-param>
      <xsl:with-param name='type'>personal,pref:skype</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='sipId']" >
      <xsl:with-param name='label'>IMPP</xsl:with-param>
      <xsl:with-param name='type'>personal,pref:sip</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='xmppId']" >
      <xsl:with-param name='label'>IMPP</xsl:with-param>
      <xsl:with-param name='type'>personal,pref:xmpp</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='ircId']" >
      <xsl:with-param name='label'>IMPP</xsl:with-param>
      <xsl:with-param name='type'>personal,pref:irc</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='msnId']" >
      <xsl:with-param name='label'>IMPP</xsl:with-param>
      <xsl:with-param name='type'>personal,pref:msn</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='telephoneNumber']" >
      <xsl:with-param name='label'>TEL;WORK;VOICE</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='postalAddress']" >
      <xsl:with-param name='label'>ADR;WORK;ENCODING=QUOTED-PRINTABLE</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='postalAddress']" >
      <xsl:with-param name='label'>LABEL;WORK;ENCODING=QUOTED-PRINTABLE</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='facsimileTelephoneNumber']" >
      <xsl:with-param name='label'>TEL;WORK;FAX</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='homePhone']" >
      <xsl:with-param name='label'>TEL;HOME;VOICE</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='homePostalAddress']" >
      <xsl:with-param name='label'>ADR;HOME;ENCODING=QUOTED-PRINTABLE</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='mobile']" >
      <xsl:with-param name='label'>TEL;MOBILE;VOICE</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='labeledUri']" >
      <xsl:with-param name='label'>URL;WORK</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='jpegPhoto']" >
      <xsl:with-param name='label' >PHOTO;<xsl:choose><xsl:when test="(not(contains(attr[@name='jpegPhoto']/value,'BinaryData')) or (starts-with(attr[@name='jpegPhoto']/value,'/9j/4AA')))">ENCODING=BASE64;</xsl:when><xsl:otherwise>VALUE=URL;</xsl:otherwise></xsl:choose>TYPE=JPEG</xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='userPKCS12']" >
      <xsl:with-param name='label' >KEY;<xsl:choose><xsl:when test="(not(contains(attr[@name='userPKCS12']/value,'BinaryData')) )">ENCODING=b</xsl:when><xsl:otherwise>VALUE=URL</xsl:otherwise></xsl:choose></xsl:with-param>
    </xsl:apply-templates>
    <xsl:apply-templates select="attr[@name='userCertificate']" >
      <xsl:with-param name='label' >KEY;<xsl:choose><xsl:when test="(not(contains(attr[@name='userCertificate']/value,'BinaryData')) )">ENCODING=b</xsl:when><xsl:otherwise>VALUE=URL</xsl:otherwise></xsl:choose></xsl:with-param>
    </xsl:apply-templates>
  </xsl:if>
</xsl:template>
</xsl:stylesheet>
