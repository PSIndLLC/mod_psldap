var ldapDomains = [];
ldapDomains[0] = {dn:"dc=some,dc=com", label:"Some .com", defaultDomain:0};
ldapDomains[1] = {dn:"dc=another,dc=us", label:"Another Domain", defaultDomain:1};
var ps_siteConfig = {secureBase:"https://www.some.com",
		     unsecureBase:"http://www.some.com",
		     authURI:"/psldap/ldapauth", updateURI:"/psldap/ldapupdate",
		     registerURI:"/psldap/register", homeURI:"/psldap",
		     userKey:"mail", passKey: "userPassword" };
