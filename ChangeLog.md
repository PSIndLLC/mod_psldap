# About mod_psldap

mod_psldap was created to provide a very lightweight means of managing directory information by leveraging client processing power to render the edit and reporting pages on any web browser without requiring an ActiveX or other binary executable download. It reduces the incremental network traffic for subsequent edits by 95% or more while still providing a very rich AJAX / Web 2.0 enabled user interface.

## Revision 0.97 (Upcoming release)

* Enable integration to blackberry contact database
* SVG based rendering of organizational hierarchies
* Import from vCard files uploaded to server


## Revision 0.96

This release addresses the introduction of compatibility for Apache 2.4, changing the module initialization binding to use providers for authorization to regain compatibility with Apache's new authorization implementation.

Additional changes include improved management for renaming the base location of the installation to something other than /psldap - however this is not yet fully tested. Requisite changes appear to be mostly manageable through psldap_config.js and DSML_sitefrags.xsl - provided the index file on the installation directory is changed to index.xml from index.html

* Add handling connection to LDAP through port 636 / TLS
* Add LDAP schema definitions to support module management features
* New web UI to create LDAP records based on XML new record template files
* Preparation for consumption and presentation of some HTML5 data types
* Improve XSL references to path configurations, improve multiple LDAP server support
* Improved handling of gif and jpeg image presentation from attributes
* Enhance mobile / handheld device support to tailor presentation to form factor
* Importing / exporting from / to google contact export format
* Fixes for group authorization
* Finish integration of browser support for HttpOnly on secure cookies
* Add some support for SSHA prefixes to password attributes
* Introduce password change support


## Revision 0.95 

* Introduced edit capability in custom person edit for mozilla attributes defined in objectClass mozillaAbPersonAlpha available at mozilla.org
	

## Revision 0.94 

The latest release of mod_psldap provides a new look and feel with updated images and a skinning capability. Two skins are available with the module: green, and blue. Look and feel are now completely customizable.

* Introduced skinning capability with two skins available and shipping with the module: green and blue
* Fixed sorting defect in tree view
* Segregated tree management scripting and styling


## Revision 0.93 

* Fixed DocumentManager JS API to accomodate browser technology changes - mod_psldap now supports XSLT on the client without a round trip to the server to reaquire the XML and reset the default XSL
* Addition of search status update in the tree div and processing status to the record editing div in alt_index.html
* Implemented window resizing to fit content in editable forms popups
* Enabled anonymous access through the module when a user is not provided
* Implemented a Register action to force rebinding to ldap with credentials provided through the PsLDAPRegBindDN and PsLDAPRegBindPassword parameters configured in the Apache configuration for the module
* Modified the DSML_editform.xsl to transform for registration specific view provided the dn attribute on the searchResultEntry node equals 'dc=registered'
* Added operational attributes into the attribute set returned when an LDAP search is executed, allowing visibility to entryUUID, modifyTimestamp, createTimestamp, creatorsName, among other attributes
* Eliminated potential security hole when using cookie based sessions
* Modified LDAP session persistance to pull timestamp from the operational attribute modifyTimestamp to dtermine last access time for Apache session
* Secured session cookies with HttpOnly option to reduce XSS attack risk - pending identification of browser support
* Introduced validation framework in edit tools - invokes any function set in the psvalidate attribute of an input element in the context of the input element on form validation passing the argument list identified in a comma separate string following the function name in the attribute e.g. psvalidate='psldap_validateMinLength,7'
* Introduced 4 standard validation methods: psldap_validateMinLength, psldap_validateMaxLength, psldap_validateEMail, psldap_validatePasswordStrength,
* Fixed the configure script - the generated Makefile now provides reliable builds, installs, and bundling of the module
* Enable drag and drop movement across organizational units and managers in the explorer tree via mouse actions against the image handles on each tree node


## Revision 0.92

* Moved common capabilities back out of DSML_sitefrags.xsl to leave that stylesheet for site specific customizations only - common capabilities are now in DSML_commonscript.xsl
* Refactored the pageHeader template with pageHeaderWithRefClass to reduce duplicate code
* Removed some hardcoded values from the DSML_vendors.xsl (enabled automatic population of the ldapDomains to all select elements with an id of 'dn') and made the template for servicesMgmt site specific
* Corrected minor title setting bug on commonscript XSL
* Extended vcard to include KEY, REV, PRODID, and CLASS, also fixing IM references to be compliant with RFC 4770
* Fixed generated dsml structure to better match spec - pushed searchResponse back under a batchResponse node and change mgmt and org XSL back to reference the correct XPath
* Fixed improper ServerPath inclusion in fully qualified path assembly - repairs issue with XML and XSL file parse for server side operations when recursive path link is not present


## Revision 0.91

* Fixes to eliminate infinite loop in the vertical wrap to ensure IE displays the card view correctly and does not hang. This is related to a change in XSL based rendering in IE, requiring deferal of the wrap function call through a timeout.
* Introduce session persistence to the LDAP store to offer an alternative to passing credentials in the cookie, replacing content instead with a session id. An additional alternative is also introduced to embed the session id in the URL
* Introduced server side XSL transformation - integrated into vcard display for the contact records and in general response handling for blackberry user agents.
* Fixed issue with poor handling of '&' in dn for URL reference to jpegPhoto which was causing some transformations to fail due to incorrect XML parsing
* Completed the change to DSML response type for jpegPhoto inclusion in the stream to ensure requests from IE return a URL to the photo and not the binary stream while continuing to pass the encoded image to firefox / mozilla based browsers
* Established uniform page head elements across all pages through introduction of XSL includes and imports
* Introduced performance improvements by adding indexes in the XSL processing.
* Introduced first page customizations for handheld user agents - initially only supporting blackberry - to include suppression of JS to wrap columns in the card style. UserAgent parameter added to all xsl templates via the new DSML_sitefrags.xsl inclusion. Telephone dialing, emailing, and SMS functional within handheld devices and tested on the blackberry.
* Addition of xmlObjectTemplate parameter to ldapupdate handler and the Present action type to present XML documents directly from the server. Formerly, this was achieved by getting XML documents directly via HTTP get requests, but this did not accomodate agents - such as handheld or mobile phone browsers - that did not perform the transform via XSL. 
* Addition of the dlFilename parameter to ldap update handler to allow responses to be provided with an attachment disposition whose filename correlates to the value of the parameter.

PENDING
* Development of a registration page to allow users to self-register in the directory. Supplement with a registrations pending page to classify and move users based on their requests to the appropriate groups.


## Revision 0.90

* Change to alt_index page xsl transformations to enable embedded editing of records
* Enabled explicit mime type return setting through BinaryType parameter on the URI - enforced setting 'text/xml' type for dn based queries and create, update, delete operations.
* Fixes in XSL to accomodate latest browser releases of firefix and IE
* Cleanup alternative index page that leverages AJAX and client side application of XSL transforms to reduce server requests to render detail already available in the DSML response to the browser
* Switch alt_index with index to make AJAX processing the default
* Modification to basic index page to make display more compact - page layout modifications to reduce framed content
* Segregated base DNs for the application into a shared configuration - introduced psldap_config.js to hold configuration settings
* Minor CSS style sheet enhancements to accomodate browser changes
* Altered image rendering for Mozilla to take advantage of uuencoded jpeg images in the XML - directly embeds image in the transformed page within the image tag. This was disabled inline in the DSML_editform.xsl but can be re-enabled if you are adventurous ;-)
* Fixed compilation issues with Apache 2.2 - now compiles and runs. Regression of changes with 2.0 and 1.3 not tested, but believed to be compatible - any issues should be reported to the sourceforge project for mod_psldap.
* Repaired cache access error in Apache 2.XX
* Introduced ability to move records to other superior nodes in the edit form. This effectively allows for moving records when someone changes organizations or if an organization restructure occurs
* Addition of PSI vendor schema
* Fixes to column creation / alignment in card display
* XML results modified to be in closer alignment to DSML spec and ennabled XML result return and XSL specification / application on all responses
* Realignment of address fields to compact edit form display; auto populate postal address from street, city, state and postal code; eliminated form copy prior to submittal on edit form
* Introduction of skype calling and yahoo messaging in card view for contacts with skype or yahoo accounts
* Fixed display of ISDN and Telex info


## Revision 0.89

* Fixed lack of recognition of URI search scope in ldap scope execution
* Implemented AJAX framework for Mozilla and IE Browsers and integrated with tree based transforms on alt_index.html page.
* Creation and integration of a vcard stylesheet - writes text in vcard format to new browser window.
* Updated apache module to send XML without specifying stylesheets
* Altered existing stylesheets for tree based rendering to allow for node directed processing through JS calls to transform xml nodes.
* Update of license terms under the GPL within the distribution.


## Revision 0.88

* Fixed load of editable forms for dn's containing an '&'
* Addressed minor defect in authorization when psldap authentication is not used.
* Addition of scope to URI based search to improve edit form link performance
* Addition of links to yahoo, aim, and skype when using the PSInd LDAP objects defined in psldap.schema


## Revision 0.87

* Addition of management based tree for person records


## Revision 0.86

* Fixed cache access error in Apache 2.0 related code to resolve core dump


## Revision 0.85

* Implementation of organizational tree view in the web stylesheets


## Revision 0.84

* Addition of PsLDAPAuthFilter to allow user to add filters to acquisition of the user record during authentication.
* Separation of the disablement of authentication from authorization through the introduction of the PsLDAPEnableAuthz parameter.


## Revision 0.83

* Fixed compilation error in Apache 2
* Addition of switch to connect to LDAP server using V3 protocol through the introduction of the PsLDAPConnectVersion parameter.
* Altered UI for creation of new records to pull the default LDAP server from the new index screen.
* Fixed menubar styling in the UI

	
## Revision 0.82

* Implemented handling of multipart/form-data in post responses.
* Implemented updates to LDAP backing store with binary data, allowing for the setting of the jpegPhoto field in the inetOrgPerson schema.
* Fixed defect in delete handler for ldap records.
	
### XSL/HTML Updates -

* Updates to sample XSL to add links for editing visible records in table and card view.
* Fixed issue with password field in the new user XSL.
* Also added field to insert jpegPhoto when editing inetOrgPerson records.
* Allowed printing of name in table view XSL when CN is protected by accessing first and last name
* Implemented new look and feel for edit form buttons
* Set print css for the table view to style for printing
* Modified index page for XSL sample interface to create new from an input select. Tweaked the layout of the index as well to make a little more user friendly.

	
## Revision 0.81

* Changed auth form internal redirect to send 302 response - fixes pages with relative references to other resources and authenticated directory requests.


## Revision 0.80

* Resolved defect with cookie processing on authentication when the server is misconfigured
* Fixed minor syntax error in JS example files.
* Updated user documentation.


## Revision 0.79

* Repaired compilation error in Apache 2.XX
* Disabled cache implementation on Apache2.XX


## Revision 0.78

* Adjust DSML_psldap.js to address IE failure to implement importNode - fixes updates to records through DSML_editform.xsl


## Revision 0.77

* Enable processing of parameters sent through both GET and POST to module.
* Created mechanism to handle LDAP search, add, modify, and delete operations.
* Created DSML generation mechanism to expose new LDAP interface.
* Created XSL templates to apply to DSML to facilitate interactions through the new interfaces.


## Revision 0.76

* Fixed directory and server initialization routines - feedback accounted for.


## Revision 0.75

* Fixed directory and server initialization routines - untested.


## Revision 0.74

* Recognized failure to provide credentials as an auth failure, allowing denial after three attempts to authenticate without credentials.
* Changed authorization handler to check for existence of user key definition and to decline authorization handling if the key is not defined. Authentication had already been checking this condition. This fixes a crash in the module.
* Addition of configuration parameter, PsLDAPEnableAuth to control whether or not A&A is enabled. Set to 'on' by default.
* Changed require group parsing to recognize group names with spaces when they are quoted with either single or double quotes. The type of quote used to delineate the group may not be used in the group name.

	
## Revision 0.73

* Implemented caching array in shared memory leveraging the apache ap_mm APIs.
* Addition of caching, controlled by the PsLDAPAuthUseCache parameter, set to off by default
* Reuse of existing LDAP connections implemented in acquiring authorization data to improve overall performance in authorization phase.
* Addition of PsLDAPAuthCookieDomain. The default is to let the cookie domain default to the server domain
* Initialization code has been added for Apache 2.0 (Courtesy Gunter Knauf)
* Reorganized code to improve readability of mixed Apache 2.0 and Apache 1.3 compatible implementation


## Revision 0.71

* Addition of cookie based authentication against LDAP server using forms to collect the authentication data.
* Made form data accessible to all subrequests by adding it to the subprocess_env table immediately after acquisition.
* Addition of ability to recurse up request_rec chain to acquire authentication data
* Created mechanism to identify pending changes to current record when ldap records are updated through forms (experimental - not exposed).

	
## Revision 0.70

* Initial work started on the processing of LDAP requests within the module

	
## Revision 0.63

* Addition of LDAP group authentication through the addition of the following parameters: PsLDAPUseLDAPGroups, PsLDAPUserGroupAttr, PsLDAPGroupMemberAttr, PsLDAPGroupNameAttr. The default is to not use LDAP based group authorization.


## Revision 0.62
	
* Changed auth_ldapsearchscope member of ldap_auth_config_rec. Implemented a new mechanism to initialize the value on reading from the config file.
* Addition of AuthLDAPBindAsFoundUser configuration parameter to bind as the user found in the filter. This differs from the AuthLDAPBindAsUser functionality as it does not assume the search base concatenated to the user key represents the DN. It should be noted that the password need only match the first eight letters when this mechanism is employed.
* Addition of AuthLDAPBindMethod to allow binding to LDAP servers implementing kerberos authentication.
* Changed AuthLDAPSearchScope parameter to be initialized in the ldap_auth_config_rec structure directly.
* Changed get_ldap_val to acquire only the requested attribute from the ldap record. This must be done to allow the module to function when authenticating against a highly secure ldap server that denies access by field.
* Replaced calls to ldap_open with ldap_init to conform with deprecation of ldap_open.
* Addition of extended information regarding the ldap errors on searches.
* Fixed attribute value return to concatenate all values. This fixed false authorization failures that occurred when more than one group attribute instance appears in a record.
* Repaired hole in returning AUTH_REQUIRED for unencrypted password comparisons when DECLINED should have been returned. This was occurring regardless of the setting of AuthLDAPAuthoritative. Repaired a similar hole in the group based authorization.
* Changed group validation to allow the user to be authenticated if no group is identified and no group attribute is identified.


## Startup Version
This module was originally based on Alexander Mayrhofer's mod_auth_ldap, version 0.5.
