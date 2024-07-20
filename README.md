# mod_psldap

Version 0.96
by David Picard

mod_psldap is an Apache web server modules that provides interaction with LDAP directory content via the HTTP protocol in addition to implementing authentication and authorization services for the web server.

Apache service configuration follows standard Apache configuration practices while directory access to view and update data is supported through HTTP post operations containing XML following the DSML schema. This package includes JavaScript capabilities to support transaction submission, dynamic form control, and directory record presentation on the browser.

The objective of this architecture is to reduce the incremental network traffic for directory interactions by 95% or more while providing a rich, interactive user experience in navigating and updating LDAP records.

In addition, the powerful capabilities to transform the content have been further leveraged to download other formats supporting import into other contact management solutions.

For history of this module, review the ChangeLog file. The latest version of this module can be acquired at http://www.psind.com/projects/mod_psldap/

## Features

### Authentication
- Direct authentication of a user against an LDAP server.
- Authentication against an LDAP server given the user's knowledge of an attribute setting in the LDAP tree for their record.
- Allow identification of the branch of the LDAP tree under which the user must be located.
- Allow for function with LDAP servers configured to enforce high levels of security.
- Support for kerberos based access to an LDAP server.

### Authorization
1.  Allows authorization based solely on authenticated status
2.  Optional authorization based on group membership
3.  Optional authorization based on attribute value on the user record

### Performance
- Allow for caching of LDAP authentication information to reduce server traffic and improve performance
- Support client side rendering using XSL templates on an XML payload.

### Maintenance Features:
- Allow an authenticated web user to query the LDAP server for information, within the limits of their privileges on the LDAP server
- Suppress client side XSL rendering of an XML response by rendering an HTML response via XSL transform on the server
- Authenticated users may edit any attributes in his/her own record subject to their LDAP access restrictions on the LDAP server
- Authenticated LDAP administrative users may edit any record in the LDAP server within their access restrictions
- Support configuration of apache access restriction to allow only https connections
- Browser based drag and drop movement of records from one node to another in the directory tree - accomodating organizational restructuring or relocation without recreating records
- Creation of new records based on user selection of XML data templates located on the web server

## Compile with Apache

### Prerequisites

In order to build mod_psldap you will need at a minimum, an ldap library and a build environment (compiler, linker, pre-processor, etc...) installed on the build machine. The mod_psldap module was built and tested against OpenLDAP version 2.0.23, (downloadable from http://www.openldap.org) but may very well build with other versions of this library.

In addition to the ldap library, the Apache web server source must be available or the web server itself must be installed on the build machine. The source can be acquired from the Apache site (http://www.apache.org) or any mirror sites.

This module can be built in three ways: via apxs as a DSO, via autoconf/automake, or as a stitcally linked module in a full Apache build.

### Build with Apache tools / apxs

Apache also provides a mechanism to build the module as a DSO compatible with a precompiled apache server. The following steps are executed within the Makefile generated by ./configure, however, to compile the module independently, you may use the apxs utility as follows:

(... Apache 2.X full functionality build with XSLT ...)
>  apxs -I /usr/include/libxml2 -I /usr/include/libxslt -D USE_LIBXML2_LIBXSL -c mod_psldap.c -lldap -llber -lxml2 -lxslt  
>  apxs -i -a -n psldap mod_psldap.la

(... or Apache 2.0 ...)
>  apxs -c mod_psldap.c -lldap -llber  
>  apxs -i -a -n psldap mod_psldap.la

(with Apache 1.3...)
>  apxs -o mod_psldap.so -c mod_psldap.c -lldap -llber

### Build through autoconf/automake

Change into the newly created "mod_psldap" directory, and configure the module itself. The autoconf and automake configurations are provided with the release and should be leveraged to validate system capabilities supporting the build.

   > cd mod_psldap ; autoreconf ; automake --add-missing ; ./configure

After Apache has been configured to your choice, make and install it as usual:

   > make  
   > make install

### Build Static Apache Module

Unpack Apache as usual, and change into it's "modules" directory:

>  gzip -dc ../path/to/archive/apache_1.3.23.tar.gz | tar -xv  
>  ...  
>  cd apache_1.3.23/src/modules

Then, unpack the mod_psldap archive into the modules directory, e.g.

>  gzip -dc ../path/to/archive/mod_psldap-0.6.tar.gz | tar -xv

To build for Apache 1.3, you must make the mod_psldap.so target and copy it to the
modules directory:

>  cd mod_psldap
>  make mod_psldap.so


## Summary Description of Apache Configuration Parameters

PsLDAPHosts        - List of LDAP hosts which should be queried

PsLDAPBindDN       - DN used to bind to the LDAP directory, if binding with
      provided credentials is not desired. This value is also used to initially
      bind to acquire the DN of the authenticating user.

PsLDAPBindPassword - The password corresponding to PsLDAPBindDN

PsLDAPBaseDN       - The DN in the LDAP directory which contains the per-user
      subnodes

PsLDAPUserKey      - The key in the directory whose value contains the username
      provided with the authentication credentials

PsLDAPPassKey      - The key in the directory whose value contains the password
      provided with the authentication credentials

PsLDAPGroupKey     - The key in the directory whose value contains the groups
      in which the user maintains membership

PsLDAPUserGroupAttr - The LDAP schema attribute of the user which is used to
      identify the user as a group member. Default value is 'dn'.

PsLDAPGroupMemberAttr - The LDAP schema attribute of the group object used to
      identify each user in the LDAP group. Default value is 'uniqueMember'.

PsLDAPGroupNameAttr - The LDAP schema attribute of the group object used to
      uniquely identify the group. Default value is 'cn'.

PsLDAPSearchScope  - Set Scope when searching in LDAP. Can be 'base',
      'onelevel', or 'subtree'

PsLDAPAuthoritative - Set to 'off' to allow control to be passed on, if the
      user is unknown to this module

PsLDAPUseLDAPGroups - Set to 'on' to lookup the user's group using LDAP groups
      rather than using an LDAP user record's attribute to identify the group
      directly. Default value is 'off'.

PsLDAPAuthSimple   - Set to 'on' if authentication is to be performed by
      acquiring an attribute from the LDAP server with the configured
      credentials.

PsLDAPAuthExternal - Set to 'on' if authentication is to be performed by
      binding with the user provided credentials

PsLDAPAuthUseCache - Set to 'on' if authentication will check the cache prior
      to querying the LDAP server

PsLDAPUseSession - Set to 'on' if session information is to persist to LDAP
      server and session id is to be saved to the cookie

PsLDAPSessionTimeout - Set to the number of seconds of inactivity permitted in
      an active session. Sessions extending beyond this period will be 
      terminated on the server. Default value is 1 hour or 3600 seconds.

PsLDAPBindMethod   - Set to 'simple', 'krbv41', or 'krbv42' to determine
      binding to server

PsLDAPConnectVersion - The connection version for the ldap server.
      Default value is 2

PsLDAPSecureAuthCookie - Set to 'off' if cookies are allowed to be sent
      across an unsecure connection

PsLDAPAuthCookieDomain - Set to a domain string if cookies are allowed to be
      used across servers in a domain

PsLDAPCryptPasswords - Set to 'on' if the LDAP server maintains crypted
      password strings

PsLDAPSchemePrefix - Set to 'on' if the LDAP server maintains scheme-prefixed
      password strings as described in rfc2307

PsLDAPCredentialForm - set to the URI that contains the form to capture the
      user's credentials.


## Configuring the module

Next step after installing the mod_psldap-enabled Apache is to configure it for operation. We'll not talk about general Apache configuration issues, please refer to the Apache documentation for that. This page focuses on the directives introduced by mod_psldap and associated standard directives.

First, let's address some basic Apache configuration settings for authentication. There are two types of authentication supported by the Apache web server: Basic and Digest. While Digest authentication is more secure and can be used over a normal HTTP connection without compromising passwords, not all browsers fully support Digest authentication. While Basic authentication is more universally supported, it must be initiated within an SSL session in order to pass the authentication credentials without risk of compromise. This module does not currently support Digest authentication, but does fully support Basic authentication.

To enable Basic authentication, the administrator must specify an authorization name and the authorization type within a location or directory section in the apache configuration or htaccess files. The location or directory within which the AuthName and AuthType are defined will then be protected through Basic authentication.

>  <Location "/someLocation">  
>    AuthName "Protected information"  
>    AuthType Basic  
>    ...  
>  </Location>

In addition to basic authentication, cookie based authentication is also supported. Simply specify the AuthType as "cookie" and authentication will be performed through use of cookies. By default, the cookie containing username and password in plain text form is restricted to a secure connetion and expires with the browser session.
- There is no mechanism for altering the expiration of the cookie on the client. 
- The cookie may be sent across an unsecure connection by specifying PsLDAPSecureAuthCookie to be set to 'off'.
- The username and password are captured through a url containing a form that is specified in the PsLDAPCredentialForm parameter.
- If the credentials are to be used across different sites within the same domain, the administrator may set the domain of the cookie by specifying the domain in the PsLDAPAuthCookieDomain parameter.

*New in version 0.91* Session persistence in the LDAP directory is supported when the server is configured with PsLDAPUseSession set to 'on' - default is 'off'. This keeps user credential data in either the web server or the LDAP server and sends a session identifier to the web client in the cookie instead of the user id and password. The IP,
port, and a random key form the session identifier - if the IP and port of the requesting connection do not match, authentication is denied and the user will be required to reauthenticate their session. Sessions are deleted from the LDAP store after a configurable period of inactivity set via the PsLDAPSessionTimeout parameter, which defaults to 3600 seconds. In order to activate the session capabilities, the LDAP server must be configured to load the psldap.schema provided in this package. This capability further requires the LDAP account credentials set in PsLDAPBindDN and PsLDAPBindPassword where this account has adequate access / permissions to create and maintain all LDAP records of class PSIndHttpSession

Alternatively, the administrator may use the Apache ErrorDocument directive to cause a form to be provided for the user to enter her credentials when she is not successfully authenticated or authorized. The administrator may wish to handle both the 401 & 403 errors in this fashion.

Example:

>    ErrorDocument 401 /cookie_auth_form.html  
>    ErrorDocument 403 /cookie_auth_form.html

To handle the form post, the following lines should be added to your httpd.conf
file.

>    \# The following is for psldap services:  
>    <IfModule mod_psldap.c>  
>      AddHandler ldap-update .ldu  
>      \# Set server wide defaults  
>      <Location />  
>        PsLDAPHosts "ldap.somewhere.com"  
>        PsLDAPUserKey mail  
>        PsLDAPPassKey userPassword  
>        PsLDAPAuthExternal on  
>        PsLDAPSchemePrefix on  
>        PsLDAPGroupKey ou  
>        PsLDAPBaseDN "dc=somewhere,dc=com"  
>        PsLDAPSearchScope subtree  
>        PsLDAPAuthUseCache on  
>        PsLDAPCredentialForm /psldapAuthForm.html  
>        PsLDAPSecureAuthCookie off  
>      </Location>  
>      \# Bind an authentication handler  
>      <Location /ldapauth>  
>        SetHandler ldap-update  
>        AuthType Form  
>        AuthName "LDAP Auth"  
>        require valid-user  
>      </Location>  
>      \# Bind a query handler  
>      <Location /ldapupdate>  
>        SetHandler ldap-update  
>        AuthType Basic  
>        AuthName "LDAP Query"  
>        require valid-user  
>      </Location>  
>      <Location /register>  
>        SetHandler ldap-update  
>      </Location>  
>    </IfModule>  

Note that the auth type and auth name must be specified for the handler. Every action handled by the ldap-update handler is authenticated, therefore the auth type must be specified. We have also commented the line to turn off security on the cookie transmission, as we would strongly advise the administrator only allow for secure cookies containing credentials (although the credentials in the cookie are base 64 encoded). If you are not protecting the pages via SSL, then you must uncomment the secureauthcookie parameter. All the remaining parameters are defined at the root directory on your web server to apply the settings uniformly across your site. If this approach is not taken, the administrator should ensure these lines repeat across locations as well.

The content of the form should follow a very specific format. The name for each input element must coincide with the name of the LDAP field against which authenticatoin will be performed. The name of each Submit input element must be FormAction, and the value must be either "Login" or "Cancel". The form might generally be implemented as follows:

>    <form name="Change Password" action="/ldapupdate" method="post">  
>      <label>Login (E-Mail)</label>  
>      <input type="TEXT" name="mail" size="20" maxlength="64" value="me@myhost.com" required />  
>      <br />  
>      <label>Password</label>  
>      <input type="PASSWORD" name="userPassword" size="20" maxlength="64" value="hackme" required />  
>      <br />  
>      <input type="SUBMIT" name="FormAction" value="Login" />  
>    </form>  

Note that in the form above, the login name field coincides with the setting for PsLDAPUserKey, while the password coincides with the field specified in the PsLDAPPassKey. It is important to ensure the PsLDAPUserKey field in the ldap server is searchable by an anonymous user and readable by the owner. If the query mode of authentication is applied, then the same constraints regarding visibility should also be applied to the password attribute.

In addition, if registration of unknown individuals is required, a Location for 'register' should also be added to the server configuration as illustrated above that does not require any authentication to access the URL as follows:

>  *** Needs example ***

Second, let's address how mod_psldap is enabled to perform authentication operations under Basic authentication. You will want to select the authentication method that is most in line with your network topology.

This module supports two different means of authentication with an LDAP server:
- with a configured set of credentials (PsLDAPAuthSimple)
- with the user provided credentials (PsLDAPAuthExternal)

Keep in mind that connecting with the configured credentials potentially compromises your LDAP server by allowing anybody with read access to the http config or htaccess files to view the configured credentials. You could of course allow an anonymous user access to your LDAP server with the ability to read the passwords of every user in your system, but this is definitely worse than exposing the credentials of a user who is granted read access to the passwords while denying all other users read access to the password attribute. Of course, granting any user read access to the password attribute could be considered a serious compromise to the security of the LDAP server as well. Storing the passwords in the LDAP server in encrypted form helps to lessen the potential for compromise, but does not eliminate it.

It's also important to note that PsLDAP supports connectivity to an LDAP server (PsLDAPBindMethod) using either a simple unencrypted connection (simple), or one encrypted with either kerberos 4.1 (krbv41) or kerberos 4.2 (krbv42).
      
>    ...  
>    \# One of the following must be set to 'on'  
>    PsLDAPAuthSimple off  
>    PsLDAPAuthExternal on  
>    \# Set to 'simple', 'krbv41', or 'krbv42' to determine binding to server  
>    PsLDAPBindMethod simple  
>    ...  

Now we need to decide if PsLDAP is the authoritative authentication mechanism, will we allow another authentication module to check if the user is authenticated if authentication fails through PsLDAP? We control this behavior through the settings indicated by PsLDAPAuthoritative - 'off' indicates yes - 'on' indicates no.

>    ...  
>    PsLDAPAuthoritative on  
>    ...  

### Configuring the client pages

The directory configurations on the client are managed through a javascript based configuration to identify the base DN groups to which the application will apply. In order to use the standard index, psldap_config.js must be modified to contain your domains. The initial configuration looks something like this:

>  ldapDomains[0] = {dn:"dc=some,dc=com", label:"Some .com", defaultDomain:0};  
>  ldapDomains[1] = {dn:"dc=another,dc=us", label:"Another Domain", defaultDomain:1};  

To manage your own base DNs, either change the above records or copy the line beginning with ldapDomains[0] and change the '0' to the next highest number in the sequence in the copied line. Then change the string litteral after the dn: to your base DN, the literal after the label: to the label you want people to see, and ensure that only one of the ldapDomain items has a defaultDomain: setting of 1. The defaultDomain will appear as the default value in the dropdown on the index page and will be used as the base DN from the browser if no other DN has been selected.

Example:
>  ldapDomains[2] = {dn:"dc=mydomain,dc=com", label:"My Domain", defaultDomain:0};


### Additional Filtering on Authentication

The administrator may also pose some additional constraints on users through use of the PsLDAPAuthFilter parameter. For example, if the administrator choses to require the account not be disabled and the user have a last name of 'Picard' in order to allow authentication to proceed, they might add the following value line in their config:
>    ...
>    PsLDAPAuthFilter "(&(!(accountDisabled=1))(sn=Picard))"
>    ...

### Specifying the LDAP Server

By this time, you're probably thinking, "Enough already - how do I tell this thing about my LDAP server?".  There are several configuration settings that impact the connection to the LDAP server.

The first connection setting we'll address is how to determine the location of the LDAP server(s).
>  ...  
>  PsLDAPHosts "myldapserver yourldapserver:9999"  
>  ...  

Note that more than one LDAP server can be specified by separating each server in the list with a space character. Also, an alternative port can be identified for a server by following the server name/IP address with a ':' and the alternative port number.

It's also important to note that the LDAP protocol is significantly different between versions 2 and 3. We can specify which version of the protocol to use via the PsLDAPConnectVersion, which should be set to a value of either 2 or 3. The default value is 2 for backwards compatibility, however this will not work with most new servers.

### Binding To An LDAP Server As A User

Next we'll address how to connect to the LDAP servers listed in the PsLDAPHosts configuration setting. By default, mod_psldap tries to bind anonymously to the LDAP directory.

If you want the module to use specific credentials for binding, you can do that by specifying them in the config section, e.g.:
>  PsLDAPBindDN "mail=webadmin,dc=mycompany,dc=com"  
>  PsLDAPBindPassword wapassword  

Warning! Keep in mind that anybody with read access to those credentials may be able to use them to gain unauthorized access to your LDAP directory. Don't forget to double-check the permissions on the config file.

There's a third method in binding to the directory available: Using the credentials supplied by the browser. If you add lines like the following ones to the config, omitting the settings for PsLDAPBindDN & PsLDAPBindPassword:
>  PsLDAPBaseDN "type=luser,o=psind,c=us"  
>  PsLDAPUserKey mail  
>  PsLDAPUserPass userPassword  
>  PsLDAPAuthExternal on  

... the module will bind as the user under the base DN "type=luser,o=psind,c=us", for an entry whose "mail" attribute value matches the user name passed by the user through the browser authentication with the browser-supplied password. If that succeeds, no more password checks are being done, and the browser supplied credentials are believed to be correct.

If we don't specify the Bind parameters (and therefore didn't add the above lines to the config file), we'll now have to tell the module where and how to find the user's credentials in the LDAP directory. If all your users are at the same level of the directory (e.g. exactly one level below "type=luser,o=psind,c=us"), and they all have the same key in their RDN (e.g. "webuser=<username>"),the story is rather simple:
>  PsLDAPBaseDN "type=luser,o=psind,c=us"  
>  PsLDAPSearchScope base  
>  PsLDAPUserKey webuser  
>  PsLDAPPassKey webpassword  
The last line above tells the module that the user's password is stored in the attribute named "webpassword". The module will search below "type=luser,o=psind,c=us" for an entry with an attribute <webuser>=provided_user_name.

Imagine, all your users are still below the same base DN as above, but some of them have different RDN's. For example, there may be one department storing all their users using the RDN "surname=<name>", maybe another department chose "extension=<number>". If all of those entries have their web credentials stored in the same attributes (e.g. "webuser" and "webpassword" again), you will have to change one line of the config snippet above:
>  PsLDAPSearchScope onelevel

If your users are *not* at exactly one level below the base DN, but scattered through a specific subtree, you can finally use:
>  PsLDAPSearchScope subtree

Again, all those users need to have their credentials in the same attributes, e.g. once again "webuser" and "webpassword". We go now into comparing the password supplied by the browser against the value from the user's node in the LDAP directory.

If we're using PsLDAPAuthExternal the password check is being skipped, because the password has already been checked by the LDAP server. For clear text password strings (generally a very bad idea), you don't have to add anything to the configuration. If your password strings are crypted, you'll have to add:
>  PsLDAPCryptPasswords on  
to your config snippet. Please be aware, that if you have crypted passwords in the directory, and don't set this option to "on", users will be able to authenticate successfully using the crypted(!) password string which may not be what you want... ;-)

There is a third alternative: use scheme prefixed passwords as described in RFC 2307. This seems to be the preferred method to store passwords in Netscape's directory server. You can enable scheme prefixed passwords by setting
>  PsLDAPSchemePrefix on

(Pretty straight forward, isn't it? ;-) mod_psldap will then be able to check passwords prefixed with "{crypt}" (Un*X crypt) and "{sha}" (Base64 encoded SHA1 digests as described in FIPS-180-1). Case of the prefix strings doesn't matter.

### Setting Up Your Groups

Ok, we've finally checked the user's password, we can open the gates... except if only members of specific groups are permitted to enter. In this case, we need to tell mod_psldap the name of the attribute listing the user's memberships, e.g. by adding:
>  PsLDAPGroupKey webgroup

What if you want to use LDAP groups? Maybe you'd like the added protection of having the group membership outside of the user's record and don't want to worry about attribute level permissions in your LDAP configuration. Or perhaps you'd like to set up a group administrator without allowing them administrative access to the record's of the user's in their groups.

Simply turn on LDAP group based authentication by setting the following:
>  PsLDAPUseLDAPGroups on  
>  PsLDAPUserGroupAttr dn  
>  PsLDAPGroupMemberAttr uniqueMember  
>  PsLDAPGroupNameAttr cn  

The above settings will set the group validation to work through the LDAP groups mechanism - specifically using the groupOfUniqueNames schema. It uses the dn of the user to map to the attribute settings for uniqueMember in the groupOfUniqueNames record. The group name against which the user is validated corresponds to the cn attribute set in the groupOfUniqueNames record.

To specify the group to check, regardless of the mechanism used to identify the group, enter the groups in a comma separated list in the .htaccess or apache configuration file, e.g. 
>  <Limit GET POST>  
>    require valid-user  
>  </Limit>  
or
>  <Limit GET POST>  
>    require group beerdrinking,beerguzzling  
>  </Limit>  


### Disabling Authentication and Authorization Functions
To disable authentication and authorization checking by the module, simply set the configuration variable PsLDAPEnableAuth and PsLDAPEnableAuthz respectively to a value of 'off'. Alternatively, do not set the PsLDAPUserKey in the htaccess or anywhere in the path to the disabled directory. A failure to set the user key will also disable the modules A&A functionality.


### Enabling Caching

Enabling the cache is relatively straightforward, simply add and set the PsLDAPAuthUseCache parameter in the configuration to 'on'. Caching is off by default as it is currently experimental. The cache size is hardcoded at 1000 records and purges every fifteen minutes, removing any records from cache that are older than 15 minutes. Future versions of this module will allow for some configurability of this behavior.

*NOTE: Caching is no longer compiled in by default pending a more robust implementation of the cache management capability - it can be enabled by passing '-D USE_PSLDAP_CACHING=X -lmm' as an option to apxs during compilation.*


### Enable Server-Side XSL Transformation

Server side XSL transformation requires the presence of the libxml2 and libxslt libraries on the target environment. To enable server side processing of the results for responses requested in non-XML forms, the module must be compiled passing the '-D USE_LIBXML2_LIBXSL=X -lxml2 -lxslt' options to apxs (prior to the -c option).


### Request Parameters

Request parameters may be passed through input elements in an HTML form or in he URI passed to the server. mod_psldap provides a number of such parameters to specify processing instructions. The following parameters are supported by the module:
  *  "BinaryType" controls the mime type of the HTTP response header
  *  "BinaryHRef" controls how binary object sfrom LDAP are embedded in the
                  DSML response. A value of yes places the base64 encoding in
		  in the value node, no puts an href in place to request the
		  object directly
  *  "BinaryData" the name of the LDAP sttribute containing the binary data in
     		  the result set. If this value is set, only that attribute is
		  returned in the response and the response mime type is set in
		  accordance with the BinaryType specifier
  *  "newrdn" only applies to the modDNRequest FormAction 
  *  "newSuperior" only applies to the modDNRequest FormAction
  *  "xsl1" specifies the XSL stylesheet to use for primary transformation
  *  "xsl2" specifies the XSL stylesheet to use for secondary transformation
  *  "xmlObjectTemplate" specifies the URI from the server root to an XML
     			 file to be returned through mod_psldap. If the client
			 does not support XSLT, then the response is transformed
			 server side.
  *  "dlFilename" specifies the filename to be used when the BinaryType is set 
     		  to a value other than text/xml or text/html
  *  "FormAction" specifies the action requested on the server side (more about
     		  this in 'Setting Up Forms'


### Setting Up Forms

So now you've got everybody you know listed in your LDAP server with all their critical information ... yeah right. One of the problems with a directory is that people move, change their ISPs,  change their phone numbers ... and generally do other things that make them really hard to keep tabs on. Well, now you can let people update their information in your address book themselves - or they won't be able to see the nifty pictures of you and your girlfriend or boyfriend doing the wild thing (if you're more tame like me, they'll miss out on the pictures of your cute kids doing really cute things).

Creating a form is very easy. Just create a normal web form and set the names of the input elements to be the names of your ldap attributes. For the Submit buttons, simply create an input element of type="submit", name="FormAction" and set the value attribute to one of three strings:
  *  "Update" - causes mod_psldap to update the settings listed on the page
              in the LDAP server.
  *  "Present" - causes mod_psldap to read the specified XML from the psldap
              root directory and send it back to the requestor. If the
	      requestor does not transform XSL, mod_psldap performs the
	      transformation server side and streams back HTML.
  *  "Create" - creates the record in the LDAP server.
  *  "Delete" - deletes the record from the LDAP server.
  *  "Disable" - deletes the password of the account, making it inaccessible.
  *  "Login" - performs authentication, sets a auth cookie, and sends the user
              back to the referring page (which should have rejected
              access based on missing credentials).
  *  "modDNRequest" - modifies the DN of the record passed

PsLDAP provides a partial measure of DSML support, it will return a DSML document (this is an XML document) in response to a form post, to which an XSLT may be applied. To specify the transform to be applied, set "xsl1" or "xsl2" on the form when submitting a request. 

I've included a few sample pages in the distribution to provide a starting point for creating simple and complex pages:
  *  change_info.html - a simple form to change fixed fields
  *  index.html - a more advanced form to search for and review records
  *  DSML_editform.xsl - allows for editing of a record via an HTML form -
                         with fields limited to those processed by the XSL -
                         representing a fairly extensive selection.
  *  DSML_cards.xsl - allows the user to view only those fields that are
                      populated in a card type of format.
  *  DSML_table.xsl - allows the user to view specified fields that are
                      visible in a tabular format.
  *  DSML_vcards.xsl - allows the user to download LDAP records as VCF files
  *  DSML_org_tree.xsl - presents results as an organizational hierarchy
  *  DSML_mgmt_tree.xsl - presents results as a management reporting hierarchy

Also note that the samples also ship with a series of XML files intended to provide a mechanism by which new records can be entered. With the XSL implementation of DSML_editform.xsl, an empty XML fragment adhering to the DSML schema is required to create a new record. The good news is that the format of the forms is consistent in all views (edit, create, and delete).

The web directory should be copied recursively to /psldap under your web root if your intent is to run the samples without modification. Revisions after 0.90 improve on configuration of paths in the client side JavaScript by extracting directory names and configuration to a configuration parameter script - psajax_config.js. The ldapDomain entries in this file should be changed to reflect the LDAP server information specific to your configuration.

In order to enable form processing, the following segment must be added to your httpd.conf:
>  <Location /ldapupdate>  
>    SetHandler application/x-ldap-update  
>    AuthType Basic  
>    \# AuthName really can be anything  
>    AuthName "LDAP Update"  
>    <Limit GET POST>  
>      Order Deny,Allow  
>      Deny from all  
>      \# Allow from local network only - no internet access  
>      Allow from 192 10  
>    </Limit>  
>  </Location>  

Note that we set the AuthType to "Basic". It could also be set to "cookie", but it should never be set to "form" as the credentials may get confused with data that is being inserted or modified on any forms.


## Some Examples

Let's say you're maintaining the contact information for everyone you know in your LDAP server. If you want to provide access to your web server to only the people you know, based on their email address and some provided credential, add the following segment to your apache configuration file:
>  \# First, we configure the "default" to be a very restrictive set of   
>  \# permissions.  
>  \#  
>  <Directory />  
>    \# Set the default LDAP directory setting for authentication  
>    PsLDAPHosts "ldap.mydomain.com"  
>    PsLDAPUserKey mail  
>    PsLDAPPassKey userPassword  
>    PsLDAPAuthExternal on  
>    PsLDAPSchemePrefix on  
>    PsLDAPGroupKey ou  
>    PsLDAPBaseDN "dc=mydomain,dc=com"  
>    PsLDAPSearchScope subtree  
>  </Directory>  

*NOTE: The configuration options for connecting to the LDAP server can be set at the Directory or even the server level to avoid repetition across your configurations - I would highly encourage this -*

Why specify the PsLDAPGroupKey to be "ou"? We want to filter by the group in the require line of the configuration. I only want family members to see the pictures I post on my website, so I restrict access to pictures on my web server those people in my LDAP server marked in the ou=Family organizational unit.
>  <FilesMatch "*.jpg">  
>    AuthType Basic  
>    AuthName Family  
>    require group Family  
>    require valid-user  
>  </FilesMatch>  


### Executing Queries From a URL

Sometimes we might need to render a page without going through the form selection and submission. In those cases, we could request the data directly using a URL based representation of the LDAP query. A sample URL may look like:
>  https://www.foo.com/ldapaction?FormAction=Search&search=(objectClass=organization)&dn=dc%3Dfoo%2Cdc%3Dcom&scope=subtree&BinaryHRef=on&xsl1=/psldap/DSML_cards.xsl

Breaking this down a bit we can see that the handler is configured as ldapaction in the configuration file. The FormAction in the argument list must indicate what action should be performed through the psldap handler - in this case 'Search'. We indicate the search string directly in the search argument and further identify the base dn for the ldap search in the dn parameter. We can indicate the scope of the search in the scope parameter to limit the depth of the search. We can further identify the stylesheet to use to render the xml in the xsl1 argument.

## Support

If you need help, have a feature request, compliment, or observation or would like to contribute, report a bug or otherwise contact us, please feel free to do so!
- To initiate or participate in high-level discussions, use the Discussions feature on our GitHub project
- To report bugs, please use the Issue feature on our GitHub project
- To review pending or request new features, please use the Issue feature on our GitHub project

We generally try to respond to messages within 2 days. PSInd offers a paid production support service for those with mission critical support needs.
