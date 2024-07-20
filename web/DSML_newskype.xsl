    <xsl:element name="script">
      <xsl:attribute name="type">text/javascript</xsl:attribute>
      <xsl:attribute name="src">http://www.skypeassets.com/i/scom/js/skype-uri.js</attribute>
    </xsl:element>
    <xsl:element name="div">
      <xsl:attribute name="id">SkypeButton_Call_<xsl:value-of select="."/>_1</xsl:attribute>
      <xsl:element name="script">
	<xsl:attribute name="type">text/javascript</xsl:attribute>
	Skype.ui({
	"name": "dropdown",
	"element": "SkypeButton_Call_<xsl:value-of select="."/>_1",
	"participants": ["<xsl:value-of select="."/>"],
	"imageSize": 12
	});
      </xsl:element>
    </xsl:element>
