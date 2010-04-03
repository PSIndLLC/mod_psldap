<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> <!ENTITY copy "&#169;"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:dsml="http://www.dsml.org/DSML" xmlns:html='http://www.w3.org/TR/REC-html40'>

  <xsl:template name="siteHeader" >
    <div class="pageNav">
      <table cellspacing='0' width='100%'>
	<tr align='right'>
	  <td width='150px' align='right'>
            <img src='/psldap/logo.jpg' alt="PSLDAP"/>
	  </td>
	  <td align='right' cellpadding='0px' valign='center' margin='0px'>
            <a class='headerTitle' href='/psldap/index.html'>PSLDAP</a>
	  </td>
	</tr>
      </table>
      
      <table border='0' cellspacing='0' height='30px' width='100%'>
	<tr align='center'>
	  <td width='43%'>&nbsp;</td>
	  <td width='10%'><a href='/psldap/index.html'>X</a></td>
	  <td width='10%'><a href='/psldap/index.html'>X</a></td>
	  <td width='10%'><a href='/psldap/index.html'>X</a></td>
	  <td width='15%'><a href='/psldap/index.html'>X</a></td>
	  <td width='12%'><a href='/psldap/index.html'>psldap</a></td>
	</tr>
      </table>
    </div>
  </xsl:template>
  
  <xsl:template name="siteFooter" >
    <div class="pageNav">
    <hr />
    Copyright &copy; 2010 PSInd, LLC  All Rights Reserved
    </div>
  </xsl:template>
  
</xsl:stylesheet>
