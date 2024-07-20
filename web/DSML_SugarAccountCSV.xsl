<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet >
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" >

<xsl:key name="searchEntryLookup" match="searchResultEntry" use="@dn" />
<xsl:output method="text" indent="no" omit-xml-declaration="yes" media-type="text/csv" />

<xsl:template match="/">
    <xsl:apply-templates select="//dsml" />
</xsl:template>

<xsl:template match="/dsml">
  <xsl:text>"Name","ID","Website","Email Address","Non Primary E-mails","Office Phone","Alternate Phone","Fax","Billing Street","Billing City","Billing State","Billing Postal Code","Billing Country","Shipping Street","Shipping City","Shipping State","Shipping Postal Code","Shipping Country","Description","Type","Industry","Annual Revenue","Employees","SIC Code","Ticker Symbol","Parent Account ID","Ownership","Campaign ID","Rating","Assigned User Name","Assigned To","Date Created","Date Modified","Modified By","Created By","Deleted"</xsl:text>
  <xsl:text></xsl:text>
  <xsl:apply-templates select="batchResponse/searchResponse" />
</xsl:template>

<xsl:template match="searchResponse">
    <xsl:variable name="recordCount" select="count(searchResultEntry)" />
    <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='organization')]/ancestor::searchResultEntry" >
        <xsl:sort select="attr[@name='o']/value" />
    </xsl:apply-templates>
</xsl:template>

<xsl:template match="attr">
  <xsl:param name='openq' >"</xsl:param>
  <xsl:param name='closeq' >",</xsl:param>
  <xsl:variable name='myvalue' select='.' />
  <xsl:value-of select='$openq' /><xsl:value-of select='normalize-space($myvalue)' /><xsl:value-of select='$closeq' />
</xsl:template>

<xsl:template match="searchResultEntry" >
  <xsl:variable name='creatorDN' select="attr[@name='creatorsName']" />
  <xsl:variable name='modifierDN' select="attr[@name='modifiersName']" />
  <xsl:if test="(starts-with(@dn, 'o='))" >
  <xsl:apply-templates select="attr[@name='o']" />
  <xsl:apply-templates select="attr[@name='entryUUID']" />
  <xsl:apply-templates select="attr[@name='labeledUri']" />
  <xsl:apply-templates select="attr[@name='mail']" />
  <xsl:text>"Non Primary E-mails",</xsl:text>
  <xsl:apply-templates select="attr[@name='telephoneNumber']" />
  <xsl:text>"Alternate Phone",</xsl:text>
  <xsl:apply-templates select="attr[@name='facsimileTelephoneNumber']" />
  <xsl:text>"Billing Street",</xsl:text>
  <xsl:text>"Billing City",</xsl:text>
  <xsl:text>"Billing State",</xsl:text>
  <xsl:text>"Billing Postal Code",</xsl:text>
  <xsl:text>"Billing Country",</xsl:text>
  <xsl:apply-templates select="attr[@name='street']" />
  <xsl:apply-templates select="attr[@name='l']" />
  <xsl:apply-templates select="attr[@name='st']" />
  <xsl:apply-templates select="attr[@name='postalCode']" />
  <xsl:apply-templates select="attr[@name='c']" />
  <xsl:apply-templates select="attr[@name='description']" />
  <xsl:text>"Type",</xsl:text>
  <xsl:apply-templates select="attr[@name='businessCategory']" />
  <xsl:text>"Annual Revenue",</xsl:text>
  <xsl:text>"Employees",</xsl:text>
  <xsl:text>"SIC Code",</xsl:text>
  <xsl:text>"Ticker Symbol",</xsl:text>
  <xsl:text>"Parent Account ID",</xsl:text>
  <xsl:text>"Ownership",</xsl:text>
  <xsl:text>"Campaign ID",</xsl:text>
  <xsl:text>"Rating",</xsl:text>
  <xsl:text>"Assigned User Name",</xsl:text>
  <xsl:text>"Assigned To",</xsl:text>
  <xsl:apply-templates select="attr[@name='createTimestamp']" />
  <xsl:apply-templates select="attr[@name='modifyTimestamp']" />
  <xsl:apply-templates select="attr[@name='modifiersName']" />
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
