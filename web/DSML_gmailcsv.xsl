<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet >
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" >

<xsl:key name="searchEntryLookup" match="searchResultEntry" use="@dn" />
<xsl:output method="text" indent="no" omit-xml-declaration="yes" media-type="text/x-vcard" />

<xsl:template match="/">
    <xsl:apply-templates select="//dsml" />
</xsl:template>

<xsl:template match="/dsml">
  <xsl:text>Name,Given Name,Additional Name,Family Name,Yomi Name,Given Name Yomi,Additional Name Yomi,Family Name Yomi,Name Prefix,Name Suffix,Initials,Nickname,Short Name,Maiden Name,Birthday,Gender,Location,Billing Information,Directory Server,Mileage,Occupation,Hobby,Sensitivity,Priority,Subject,Notes,Group Membership,E-mail 1 - Type,E-mail 1 - Value,E-mail 2 - Type,E-mail 2 - Value,E-mail 3 - Type,E-mail 3 - Value,E-mail 4 - Type,E-mail 4 - Value,IM 1 - Type,IM 1 - Service,IM 1 - Value,Phone 1 - Type,Phone 1 - Value,Phone 2 - Type,Phone 2 - Value,Phone 3 - Type,Phone 3 - Value,Phone 4 - Type,Phone 4 - Value,Address 1 - Type,Address 1 - Formatted,Address 1 - Street,Address 1 - City,Address 1 - PO Box,Address 1 - Region,Address 1 - Postal Code,Address 1 - Country,Address 1 - Extended Address,Address 2 - Type,Address 2 - Formatted,Address 2 - Street,Address 2 - City,Address 2 - PO Box,Address 2 - Region,Address 2 - Postal Code,Address 2 - Country,Address 2 - Extended Address,Organization 1 - Type,Organization 1 - Name,Organization 1 - Yomi Name,Organization 1 - Title,Organization 1 - Department,Organization 1 - Symbol,Organization 1 - Location,Organization 1 - Job Description,Website 1 - Type,Website 1 - Value,Website 2 - Type,Website 2 - Value,Custom Field 1 - Type,Custom Field 1 - Value,Jot 1 - Type,Jot 1 - Value</xsl:text>
  <xsl:apply-templates select="//searchResponse" />
</xsl:template>

<xsl:template name="searchResults" match="searchResponse">
    <xsl:variable name="recordCount" select="count(searchResultEntry)" />
    <xsl:apply-templates select="searchResultEntry/attr[@name='objectClass']/value[(text()='organizationalPerson')]/ancestor::searchResultEntry">
        <xsl:sort select="attr[@name='sn']/value" />
        <xsl:sort select="attr[@name='givenName']/value" />
        <xsl:sort select="attr[@name='cn']/value" />
    </xsl:apply-templates>
</xsl:template>

<xsl:template match="attr">
  <xsl:variable name='myvalue' select='.' />
  <xsl:text><xsl:value-of select='normalize-space($myvalue)' /><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="attr[@name='sn']">
  <xsl:param name='label' select='@name' />
  <xsl:param name='type' />
  <xsl:variable name='mysn' select="." />
  <xsl:variable name='mygn' select="ancestor-or-self::searchResultEntry/attr[@name='givenName']" />
  <xsl:value-of select="normalize-space($mysn)" /><xsl:text>;</xsl:text><xsl:if test="(not ($type=''))"><xsl:value-of select='$type' /></xsl:if><xsl:value-of select="normalize-space($mygn)" /><xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="searchResultEntry">
  <xsl:if test="(starts-with(@dn, 'cn='))" >
    <xsl:apply-templates select="attr[@name='cn']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='gn']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='displayName']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='sn']" /><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='initials']" /><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='birthyear']" /><xsl:apply-templates select="attr[@name='birthmonth']" /><xsl:apply-templates select="attr[@name='birthday']" /><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='description']" /><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='mail']" /><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='mail']" /><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='mail']" /><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='mail']" /><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,Work,</xsl:text><xsl:apply-templates select="attr[@name='telephoneNumber']" /><xsl:text>,Home,</xsl:text><xsl:apply-templates select="attr[@name='homePhone']" /><xsl:text>,Mobile,</xsl:text><xsl:apply-templates select="attr[@name='mobile']" /><xsl:text>,Pager,</xsl:text><xsl:apply-templates select="attr[@name='pager']" /><xsl:text>,Work,</xsl:text><xsl:apply-templates select="attr[@name='postalAddress']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='street']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='l']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='postOfficeBox']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='st']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='postalCode']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='c']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='mozillaWorkStreet2']" /><xsl:text>,Home,</xsl:text><xsl:apply-templates select="attr[@name='homePostalAddress']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='mozillaHomeAddress']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='mozillaHomeLocalityName']" /><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='mozillaHomeState']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='mozillaHomePostalCode']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='mozillaHomeCountryName']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='mozillaHomeStreet2']" /><xsl:text>,Work,</xsl:text><xsl:apply-templates select="attr[@name='o']" /><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='title']" /><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='ou']" /><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:apply-templates select="attr[@name='physicalDeliveryOfficeName']" /><xsl:text>,</xsl:text><xsl:text>,Home,</xsl:text><xsl:apply-templates select="attr[@name='labeledURI']" /><xsl:text>,</xsl:text><xsl:text>,</xsl:text><xsl:text>,Manager's Name,</xsl:text><xsl:apply-templates select="attr[@name='manager']" /><xsl:text>,</xsl:text><xsl:text>,</xsl:text>
  </xsl:if>
</xsl:template>
</xsl:stylesheet>
