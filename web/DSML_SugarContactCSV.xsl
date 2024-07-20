<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet >
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" >

<xsl:key name="searchEntryLookup" match="searchResultEntry" use="@dn" />
<xsl:output method="text" indent="no" omit-xml-declaration="yes" media-type="text/csv" />

<xsl:template match="/">
    <xsl:apply-templates select="//dsml" />
</xsl:template>

<xsl:template match="/dsml">
  <xsl:text>"First Name","Last Name","ID","Salutation","Title","Department","Account Name","Email Address","Non Primary E-mails","Mobile Phone","Office Phone","Home Phone","Other Phone","Fax","Primary Address Street","Primary Address City","Primary Address State","Primary Address Postal Code","Primary Address Country","Alternate Address Street","Alternate Address City","Alternate Address State","Alternate Address Postal Code","Alternate Address Country","Description","Birthdate","Lead Source","Campaign ID","Do Not Call","Reports To ID","Assistant","Assistant Phone","Assigned User Name","Assigned User ID","Date Created","Date Modified","Modified By ID","Created By ID","Deleted"</xsl:text>
    <xsl:text></xsl:text>
  <xsl:apply-templates select="//searchResponse" />
</xsl:template>

<xsl:template name="searchResults" match="searchResponse">
    <xsl:variable name="recordCount" select="count(searchResultEntry)" />
    <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='organizationalPerson')]/ancestor::searchResultEntry">
        <xsl:sort select="attr[@name='sn']/value" />
        <xsl:sort select="attr[@name='givenName']/value" />
        <xsl:sort select="attr[@name='cn']/value" />
        <xsl:with-param name='heading'>People</xsl:with-param>
    </xsl:apply-templates>
</xsl:template>

<xsl:template match="value">
  <xsl:value-of select='normalize-space()' /><xsl:text> </xsl:text>
</xsl:template>

<xsl:template match="attr">
  <xsl:param name='openq' >"</xsl:param>
  <xsl:param name='closeq' >",</xsl:param>
  <xsl:param name='posInd'>position() &lt; 2</xsl:param>
  <xsl:variable name='myvalue' select='.' />
  <xsl:value-of select='$openq' /><xsl:value-of select='normalize-space($myvalue)' /><xsl:value-of select='$closeq' />
</xsl:template>

<xsl:template match="searchResultEntry">
  <xsl:variable name='creatorDN' select="attr[@name='creatorsName']" />
  <xsl:variable name='modifierDN' select="attr[@name='modifiersName']" />
  <xsl:if test="(starts-with(@dn, 'cn='))" >
    <xsl:apply-templates select="attr[@name='givenName']" />
    <xsl:apply-templates select="attr[@name='sn']" />
    <xsl:apply-templates select="attr[@name='entryUUID']" />
    <xsl:text>"Salutation",</xsl:text>
    <xsl:apply-templates select="attr[@name='title']" />
    <xsl:apply-templates select="attr[@name='ou']" />
    <xsl:apply-templates select="attr[@name='o']" />
    <xsl:text>"</xsl:text><xsl:apply-templates select="attr[@name='mail']/value[position()=1]" /><xsl:text>",</xsl:text>
    <xsl:text>"</xsl:text><xsl:for-each select="attr[@name='mail']/value[position()&gt;1]" ><xsl:apply-templates select="." /></xsl:for-each><xsl:text>",</xsl:text>
    <xsl:apply-templates select="attr[@name='mobile']" />
    <xsl:apply-templates select="attr[@name='telephoneNumber']" />
    <xsl:apply-templates select="attr[@name='homePhone']" />
    <xsl:text>"Other Phone",</xsl:text>
    <xsl:apply-templates select="attr[@name='facsimileTelephoneNumber']" />
    <xsl:apply-templates select="attr[@name='street']" />
    <xsl:apply-templates select="attr[@name='l']" />
    <xsl:apply-templates select="attr[@name='st']" />
    <xsl:apply-templates select="attr[@name='postalCode']" />
    <xsl:apply-templates select="attr[@name='c']" />
    <xsl:text>"Alternate Address Street",</xsl:text>
    <xsl:text>"Alternate Address City",</xsl:text>
    <xsl:text>"Alternate Address State",</xsl:text>
    <xsl:text>"Alternate Address Postal Code",</xsl:text>
    <xsl:text>"Alternate Address Country",</xsl:text>
    <xsl:apply-templates select="attr[@name='description']" />
    <xsl:text>"Birthdate",</xsl:text>
    <xsl:text>"Lead Source",</xsl:text>
    <xsl:text>"Campaign ID",</xsl:text>
    <xsl:text>"Do Not Call",</xsl:text>
    <xsl:text>"Reports To ID",</xsl:text>
    <xsl:text>"Assistant",</xsl:text>
    <xsl:text>"Assistant Phone",</xsl:text>
    <xsl:text>"Assigned User Name",</xsl:text>
    <xsl:text>"Assigned User ID",</xsl:text>
    <xsl:apply-templates select="attr[@name='createTimestamp']" />
    <xsl:apply-templates select="attr[@name='modifyTimestamp']" />
  <xsl:choose>
    <xsl:when test="ancestor::searchResponse/searchResultEntry[@dn=$modifierDN]/attr[@name='entryUUID']" >
      <xsl:apply-templates select="ancestor::searchResponse/searchResultEntry[@dn=$modifierDN]/attr[@name='entryUUID']" />
    </xsl:when>
    <xsl:otherwise><xsl:text>"",</xsl:text></xsl:otherwise>
  </xsl:choose>
  <xsl:choose>
    <xsl:when test="ancestor::searchResponse/searchResultEntry[@dn=$creatorDN]/attr[@name='entryUUID']" >
      <xsl:apply-templates select="ancestor::searchResponse/searchResultEntry[@dn=$creatorDN]/attr[@name='entryUUID']" />
    </xsl:when>
    <xsl:otherwise><xsl:text>"",</xsl:text></xsl:otherwise>
  </xsl:choose>
    <xsl:text>"Deleted"</xsl:text>
    <xsl:text></xsl:text>
  </xsl:if>
</xsl:template>
</xsl:stylesheet>
