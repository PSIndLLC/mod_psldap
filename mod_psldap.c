/*
 * mod_psldap
 *
 * User Authentication against and maintenance of an LDAP database
 *
 * David Picard dpicard@psind.com
 *
 * http://www.psind.com/projects/mod_psldap/
 *
 */

/* 
 * MODULE-DEFINITION-START
 * Name: psldap_module
 * ConfigStart
 LDAP_LIBS="-lldap -llber"
 LIBS="$LIBS $LDAP_LIBS"
 echo "       + using LDAP libraries for psldap module"
 * ConfigEnd
 * MODULE-DEFINITION-END
 */

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"
#include "ap_sha1.h"
#include <lber.h>
#include <ldap.h>
#include <unistd.h>

#define SHA1_DIGEST_LENGTH 29 /* sha1 digest length including null character */

#define INT_UNSET -1
#define STR_UNSET NULL

#define FORM_ACTION	"FormAction"
#define LOGIN_ACTION	"Login"
#define MODIFY_ACTION	"Modify"
#define CREATE_ACTION	"Create"
#define DISABLE_ACTION	"Disable"
#define DELETE_ACTION	"Delete"

#define LDAP_KEY_PREFIX "psldap-"
#define LDAP_KEY_PREFIX_LEN 7


typedef struct  {
    char *psldap_hosts;
    char *psldap_binddn;
    char *psldap_bindpassword;
    char *psldap_basedn;
    char *psldap_userkey;
    char *psldap_passkey;
    char *psldap_groupkey;
    char *psldap_user_grp_attr;
    char *psldap_grp_mbr_attr;
    char *psldap_grp_nm_attr;
    int   psldap_searchscope;
    int   psldap_authoritative;
    int   psldap_cryptpasswords;
    int   psldap_authsimple;
    int   psldap_authexternal;
    int   psldap_bindmethod;
    int   psldap_schemeprefix;
    int   psldap_use_ldap_groups;
    int   psldap_secure_auth_cookie;
    char *psldap_credential_uri;
} psldap_config_rec;

static void module_init(server_rec *s, pool *p)
{
    ap_add_version_component("mod_psldap/0.71");
}

static void *create_ldap_auth_dir_config (pool *p, char *d)
{
    psldap_config_rec *sec
        = (psldap_config_rec *)ap_pcalloc (p, sizeof(psldap_config_rec));
    
    sec->psldap_hosts = STR_UNSET;
    sec->psldap_binddn = STR_UNSET;
    sec->psldap_bindpassword = STR_UNSET;
    sec->psldap_basedn = STR_UNSET;
    sec->psldap_user_grp_attr = STR_UNSET;
    sec->psldap_grp_mbr_attr = STR_UNSET;
    sec->psldap_grp_nm_attr = STR_UNSET;

    sec->psldap_searchscope = INT_UNSET;
    sec->psldap_authoritative = INT_UNSET;
    sec->psldap_cryptpasswords = INT_UNSET; 
    sec->psldap_authsimple = INT_UNSET;
    sec->psldap_authexternal = INT_UNSET;
    sec->psldap_bindmethod = LDAP_AUTH_NONE;
    sec->psldap_schemeprefix = INT_UNSET;
    sec->psldap_use_ldap_groups = INT_UNSET;
    sec->psldap_secure_auth_cookie = INT_UNSET;
    sec->psldap_credential_uri = STR_UNSET;
    return sec;
}

/** Set a string attribute _a_ in config record _r_ from the attribute _a_ in
 *  config record _n_ if the attribute in _r_ is unset, using apache pool _p_
 */
#define set_cfg_str_if_n_set(_p_, _r_, _n_, _a_)			\
    if ((STR_UNSET == _r_->_a_) && (STR_UNSET != _n_->_a_))		\
        _r_->_a_ = ap_pstrdup(_p_, _n_->_a_)
	
/** Set an integer attribute _a_ in config record _r_ from the attribute _a_ in
 *  config record _n_ if the attribute in _r_ equals value _c_, otherwise set
 *  to default value _d_
 */
#define set_cfg_int_if_n_set(_r_, _n_, _a_, _c_, _d_)			\
    if (_c_ == _r_->_a_)						\
        _r_->_a_ = (_c_ == _n_->_a_) ? _d_ : _n_->_a_

void *merge_ldap_auth_dir_config (pool *p, void *base_conf, void *new_conf)
{
    psldap_config_rec *result
        = (psldap_config_rec *)ap_pcalloc (p, sizeof(psldap_config_rec));
    psldap_config_rec *b = (psldap_config_rec *)base_conf;
    psldap_config_rec *n = (psldap_config_rec *)new_conf;

    *result = *n;
    set_cfg_str_if_n_set(p, result, n, psldap_hosts);
    set_cfg_str_if_n_set(p, result, n, psldap_binddn);
    set_cfg_str_if_n_set(p, result, n, psldap_bindpassword);
    set_cfg_str_if_n_set(p, result, n, psldap_basedn);
    set_cfg_str_if_n_set(p, result, n, psldap_userkey);
    set_cfg_str_if_n_set(p, result, n, psldap_passkey);
    set_cfg_str_if_n_set(p, result, n, psldap_groupkey);
    set_cfg_str_if_n_set(p, result, n, psldap_user_grp_attr);
    set_cfg_str_if_n_set(p, result, n, psldap_grp_mbr_attr);
    set_cfg_str_if_n_set(p, result, n, psldap_grp_nm_attr);

    if (NULL == b) return result;
    
    set_cfg_str_if_n_set(p, result, b, psldap_hosts);
    set_cfg_str_if_n_set(p, result, b, psldap_binddn);
    set_cfg_str_if_n_set(p, result, b, psldap_bindpassword);
    set_cfg_str_if_n_set(p, result, b, psldap_basedn);
    set_cfg_str_if_n_set(p, result, b, psldap_userkey);
    set_cfg_str_if_n_set(p, result, b, psldap_passkey);
    set_cfg_str_if_n_set(p, result, b, psldap_groupkey);
    set_cfg_str_if_n_set(p, result, b, psldap_user_grp_attr);
    set_cfg_str_if_n_set(p, result, b, psldap_grp_mbr_attr);
    set_cfg_str_if_n_set(p, result, b, psldap_grp_nm_attr);

    set_cfg_int_if_n_set(result, b, psldap_searchscope, INT_UNSET,
                         LDAP_SCOPE_BASE);
    /* by default, we use simple binding to ldap, never use auth_none */
    set_cfg_int_if_n_set(result, b, psldap_bindmethod, LDAP_AUTH_NONE,
                         LDAP_AUTH_SIMPLE);
    /* fortress is secure by default */
    set_cfg_int_if_n_set(result, b, psldap_authoritative, INT_UNSET, 1);
    set_cfg_int_if_n_set(result, b, psldap_cryptpasswords, INT_UNSET, 0);
    /* by default, we bind as found user to ldap */
    set_cfg_int_if_n_set(result, b, psldap_authsimple, INT_UNSET, 0);
    /* by default, we bind as found user to ldap */
    set_cfg_int_if_n_set(result, b, psldap_authexternal, INT_UNSET, 0);
    /* no scheme prefix in password strings */
    set_cfg_int_if_n_set(result, b, psldap_schemeprefix, INT_UNSET, 0);
    /* Do not use ldap groups for group identification by default */
    set_cfg_int_if_n_set(result, b, psldap_use_ldap_groups, INT_UNSET, 0);
    /* Cookie is secure by default */
    set_cfg_int_if_n_set(result, b, psldap_secure_auth_cookie, INT_UNSET, 1);
    /* Set the URI for the form to capture credentials */
    set_cfg_str_if_n_set(p, result, b, psldap_credential_uri);

    return result;
}

static const char* set_ldap_search_scope(cmd_parms *parms, void *mconfig,
                                         char *to) {
    psldap_config_rec *lac = (psldap_config_rec*)mconfig;
    if((NULL != to) && (0 == strcasecmp("subtree", to)) )
    {
        lac->psldap_searchscope = LDAP_SCOPE_SUBTREE;
    }
    else if((NULL != to) && (0 == strcasecmp("onelevel", to)) )
    {
        lac->psldap_searchscope = LDAP_SCOPE_ONELEVEL;
    }
    else
    {
        lac->psldap_searchscope = LDAP_SCOPE_BASE;
    }
    return NULL;
}

static const char* set_ldap_slot(cmd_parms *parms, void *mconfig, char *to) {
    psldap_config_rec *lac = (psldap_config_rec*)mconfig;
    if((NULL != to) && (0 == strcasecmp("krbv41", to)) )
    {
        lac->psldap_bindmethod = LDAP_AUTH_KRBV41;
    }
    else if((NULL != to) && (0 == strcasecmp("krbv42", to)) )
    {
        lac->psldap_bindmethod = LDAP_AUTH_KRBV42;
    }
    else
    {
        lac->psldap_bindmethod = LDAP_AUTH_SIMPLE;
    }
    return NULL;
}

command_rec ldap_auth_cmds[] = {
    { "PsLDAPHosts", ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_hosts),
      OR_AUTHCFG, TAKE1, 
      "List of LDAP hosts which should be queried"
    },
    { "PsLDAPBindDN", ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_binddn),
      OR_AUTHCFG, TAKE1, 
      "DN used to bind to the LDAP directory, if binding with provided"
      " credentials is not desired. This value is also used to initially bind"
      " to acquire the DN of the authenticating user."
    },
    { "PsLDAPBindPassword", ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_bindpassword),
      OR_AUTHCFG, TAKE1, 
      "The password corresponding to PsLDAPBindDN"
    },
    { "PsLDAPBaseDN", ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_basedn),
      OR_AUTHCFG, TAKE1, 
      "The DN in the LDAP directory which contains the per-user subnodes"
    },
    { "PsLDAPUserKey", ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_userkey),
      OR_AUTHCFG, TAKE1, 
      "The key in the directory whose value contains the username provided"
      " with the authentication credentials"
    },
    { "PsLDAPPassKey", ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_passkey),
      OR_AUTHCFG, TAKE1, 
      "The key in the directory whose value contains the password provided"
      " with the authentication credentials"
    },
    { "PsLDAPGroupKey", ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_groupkey),
      OR_AUTHCFG, TAKE1, 
      "The key in the directory whose value contains the groups in which the"
      " user maintains membership"
    },
    { "PsLDAPUserGroupAttr", ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_user_grp_attr),
      OR_AUTHCFG, TAKE1, 
      "The LDAP schema attribute of the user which is used to identify the"
      " user as a group member. Default value is 'dn'."
    },
    { "PsLDAPGroupMemberAttr", ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_grp_mbr_attr),
      OR_AUTHCFG, TAKE1, 
      "The LDAP schema attribute of the group object used to identify each"
      " user in the LDAP group. Default value is 'uniqueMember'."
    },
    { "PsLDAPGroupNameAttr", ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_grp_nm_attr),
      OR_AUTHCFG, TAKE1, 
      "The LDAP schema attribute of the group object used to uniquely"
      " identify the group. Default value is 'cn'."
    },
    { "PsLDAPSearchScope", set_ldap_search_scope,
      (void*)XtOffsetOf(psldap_config_rec, psldap_searchscope),
      OR_AUTHCFG, TAKE1, 
      "Set Scope when searching in LDAP. Can be 'base', 'onelevel', or"
      " 'subtree'"
    },
    { "PsLDAPAuthoritative", ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_authoritative),
      OR_AUTHCFG, FLAG, 
      "Set to 'off' to allow control to be passed on, if the user is unknown"
      " to this module"
    },
    { "PsLDAPUseLDAPGroups", ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_use_ldap_groups),
      OR_AUTHCFG, FLAG, 
      "Set to 'on' to lookup the user's group using LDAP groups rather than"
      " using an LDAP user record's attribute to identify the group directly."
      " Default value is 'off'."
    },
    /* Authentication methods */
    { "PsLDAPAuthSimple", ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_authsimple),
      OR_AUTHCFG, FLAG, 
      "Set to 'on' if authentication is to be performed by acquiring an"
      " attribute from the LDAP server with the configured credentials."
    },
    { "PsLDAPAuthExternal", ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_authexternal),
      OR_AUTHCFG, FLAG, 
      "Set to 'on' if authentication is to be performed by binding with the"
      " user provided credentials"
    },
    /* Connection security */
    { "PsLDAPBindMethod", set_ldap_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_bindmethod),
      OR_AUTHCFG, TAKE1, 
      "Set to 'simple', 'krbv41', or 'krbv42' to determine binding to server"
    },
    { "PsLDAPSecureAuthCookie", ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_secure_auth_cookie),
      OR_AUTHCFG, FLAG, 
      "Set to 'off' if cookies are allowed to be sent across an unsecure"
      "    connection"
    },
    /* Password management */
    { "PsLDAPCryptPasswords", ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_cryptpasswords),
      OR_AUTHCFG, FLAG, 
      "Set to 'on' if the LDAP server maintains crypted password strings"
    },
    { "PsLDAPSchemePrefix", ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_schemeprefix),
      OR_AUTHCFG, FLAG, 
      "Set to 'on' if the LDAP server maintains scheme-prefixed password"
      " strings as described in rfc2307"
    },
    { "PsLDAPCredentialForm", ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_credential_uri),
      OR_AUTHCFG, TAKE1, 
      "The URI containing the form to capture the user's credentials."
    },
    { NULL }
};

module psldap_module;

typedef struct
{
    request_rec *rr;
    LDAP *ldap;
    psldap_config_rec *conf;
} psldap_status;

static char * get_user_dn(request_rec *r, const char *user, const char *pass,
                          psldap_config_rec *conf)
{
    int err_code;
    LDAP *ldap = NULL;
    LDAPMessage *ld_result = NULL, *ld_entry = NULL;
    const char *ldap_attrs[2] = {LDAP_NO_ATTRS, NULL};
    char *ldap_query = NULL, *ldap_base = NULL, *user_dn = NULL;
    
    /* ldap_open is deprecated in future releases, ldap_init is recommended */
    if(NULL == (ldap = ldap_init(conf->psldap_hosts, LDAP_PORT)))
    { 
        ap_log_error(APLOG_MARK, APLOG_ERR, r->server, "ldap_init failed <%s>",
                     conf->psldap_hosts);
        goto AbortDNAcquisition;
    }

    if(LDAP_SUCCESS != (err_code = ldap_bind_s(ldap, conf->psldap_binddn,
                                               conf->psldap_bindpassword,
                                               conf->psldap_bindmethod))
       )
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE, r->server,
                     "ldap_bind as user <%s> failed: %s", conf->psldap_binddn,
                     ldap_err2string(err_code));
        goto AbortDNAcquisition;
    }
    
    ldap_query = ap_pstrcat(r->pool, conf->psldap_userkey, "=", user, NULL);
    
    /* check for search scope, build the base dn query */
    if(LDAP_SCOPE_BASE == conf->psldap_searchscope)
    {
        ldap_base = ap_pstrcat(r->pool, ldap_query, ",", conf->psldap_basedn,
                               NULL);
    }
    else
    {
        ldap_base = ap_pstrdup(r->pool, conf->psldap_basedn);
    }
	    
    /* Set the attribute list to return to include the user key, which
       is part of the query string. Any user who can successfully
       execute the query may be granted search access to this attribute -
       but not necessarily read access. It is safer to get the dn attribute.
    */
    ldap_attrs[0] = "dn"/*conf->psldap_userkey*/;
    if(LDAP_SUCCESS !=
       (err_code = ldap_search_s(ldap, ldap_base, conf->psldap_searchscope,
                                 ldap_query, (char**)ldap_attrs, 0, &ld_result))
       )
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE, r->server,
                     "ldap_search failed: %s", ldap_err2string(err_code));
        goto AbortDNAcquisition;
    }
    
    if(NULL == (ld_entry = ldap_first_entry(ldap, ld_result)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE, r->server,
                     "attr <%s> for user <%s> not found in <%s>",
                     ldap_attrs[0], ldap_query, ldap_base);
        goto AbortDNAcquisition;
    }

    user_dn = ap_pstrdup(r->pool, ldap_get_dn(ldap, ld_entry) );

    if (NULL != ld_result)
    {
        ldap_msgfree(ld_result);
    }
    if (NULL != ldap)
    {
        ldap_unbind_s(ldap);
    }

 AbortDNAcquisition:
    return user_dn;
}

static int set_bind_params(request_rec *r, psldap_config_rec *conf,
                           const char **user, char const **password)
{
    int result = TRUE;

    if(conf->psldap_authsimple)
    {
        *user = conf->psldap_binddn;
        *password = conf->psldap_bindpassword;
    }
    else if (conf->psldap_authexternal)
    {
        /* Should we really check for empty passwords? What if the password in
           the LDAP server is blank, like for a guest account? Also note that
           this implementation requires the username to be the first segment of
           the dn and for this record to be located directly under the BaseDN
        */
        if ((NULL == *password) || (0 == strcmp(*password,"")))
        {
            ap_log_reason("ldap_bind: no password given (AuthSimple enabled)!",
                          NULL, r);
            result = FALSE;
        }
        else
        {
            *user = get_user_dn(r, *user, *password, conf);
        }
    }

    return result;
}

static char* construct_ldap_query(request_rec *r, psldap_config_rec *conf,
                                  const char *query_by, const char *query_for,
                                  const char *user)
{
    /* If no attribute to query_by is passed, or no value to query_for is
       passed - assume this is a query of an attribute within the user's
       record */
  
    char *result = NULL;
    if ((NULL == query_by) || (NULL == query_for))
    {
        result = ap_pstrcat(r->pool, conf->psldap_userkey, "=", user,
                            NULL);
    }
    else
    {
        result = ap_pstrcat(r->pool, query_by, "=", query_for,
                            NULL);
    }
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                 "User = <%s = %s> : ldap_query = <%s>",
                 conf->psldap_userkey, user, result);

    return result;
}

static char* construct_ldap_base(request_rec *r, psldap_config_rec *conf,
                                 const char *ldap_query)
{
    char *result = NULL;
    /* check for search scope */
    if(LDAP_SCOPE_BASE == conf->psldap_searchscope)
    {
        result = ap_pstrdup(r->pool, ldap_query);
    }
    else
    {
        result = ap_pstrdup(r->pool, conf->psldap_basedn);
    }

    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server, "ldap_base = <%s>",
                 result);

    return result;
}

/** Iterate through all values and concatenate them into a separator
 *  delimited string. This will not function properly if an attribute has
 *  embedded separator characters.
 *
 *  An alternative implementation might determine what the best separator
 *  is from an array of passed separators by performing a double pass
 *  through the values and return the separator it selects for use.
 *
 *  @param r request record from which to preform pool related operations
 *  @param values a NULL terminated array of values to be concatenated into a
 *                string list
 *  @param separator string to use to separate values in the list
 *
 *  @result allocated string containing all the values in the array values
 *          separated by string separator
 */
static char * build_string_list(request_rec *r, char * const *values,
                                const char *separator)
{
    register int i;
    char *result = NULL;
    for(i = 0; NULL != values[i]; i++)
    {
        result = (NULL == result) ? ap_pstrdup(r->pool, values[i]) :
            ap_pstrcat(r->pool, result, separator, values[i], NULL);
    }
    return result;
}

static char * get_ldvalues_from_connection(
                           request_rec *r, psldap_config_rec *conf, LDAP *ldap,
                           char *ldap_base, char *ldap_query, const char *user,
                           const char *attr, const char *separator)
{
    /* Set the attribute list to return to include only the requested value.
       This is done to avoid false errors caused when querying more secure
       LDAP servers that protect information within the records.
    */
    LDAPMessage *ld_result = NULL, *ld_entry = NULL;
    const char *ldap_attrs[2] = {LDAP_NO_ATTRS, NULL};
    char **ld_values = NULL;
    char *result = NULL;
    int  err_code;

    ldap_attrs[0] = attr;
    if(LDAP_SUCCESS !=
       (err_code = ldap_search_s(ldap, ldap_base, conf->psldap_searchscope,
                                 ldap_query, (char**)ldap_attrs, 0, &ld_result))
       )
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE, r->server,
                     "ldap_search failed: %s", ldap_err2string(err_code));
        goto GET_LDVALUES_RETURN;
    }

    if(!(ld_entry = ldap_first_entry(ldap, ld_result)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE, r->server,
                     "user <%s> not found", user);
        goto GET_LDVALUES_RETURN;
    }
    if(!(ld_values = ldap_get_values(ldap, ld_entry, attr)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE, r->server,
                     "ldap_get_values <%s> failed", attr);
        goto GET_LDVALUES_RETURN;
    }

    result = build_string_list(r, ld_values, separator);

 GET_LDVALUES_RETURN:
    if (NULL != ld_values)
    {
        ldap_value_free(ld_values);
    }
    if (NULL != ld_result)
    {
        ldap_msgfree(ld_result);
    }
    return result;
}

static char * get_ldap_val(request_rec *r, const char *user, const char *pass,
                           psldap_config_rec *conf,
                           const char *query_by, const char *query_for,
                           const char *attr, const char *separator) {
    const char *bindas = user, *bindpass = pass;
    LDAP *ldap = NULL;
    char *retval = NULL;

    /* ldap_open is deprecated in future releases, ldap_init is recommended */
    if(NULL == (ldap = ldap_init(conf->psldap_hosts, LDAP_PORT)))
    { 
        ap_log_error(APLOG_MARK, APLOG_ERR, r->server, "ldap_init failed <%s>",
                     conf->psldap_hosts);
        return retval;
    }

    if(NULL == attr) retval = "bind";
    if((NULL != attr) && set_bind_params(r, conf, &bindas, &bindpass))
    {
        if(ldap_bind_s(ldap, bindas, bindpass, conf->psldap_bindmethod))
        {
            ap_log_error(APLOG_MARK, APLOG_WARNING, r->server,
                         "ldap_bind as user <%s> failed", bindas);
        }
        else
        {
            char *ldap_query = NULL, *ldap_base = NULL;

            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO, r->server,
                         "ldap_bind as user <%s> succeeded", bindas);
        
            ldap_query = construct_ldap_query(r, conf, query_by, query_for,
                                              user);
            ldap_base = construct_ldap_base(r, conf, ldap_query);
            
            retval = get_ldvalues_from_connection(r, conf, ldap, ldap_base,
                                                  ldap_query, user, attr,
                                                  separator);
        }
    }

    if (NULL != ldap) { ldap_unbind_s(ldap); }
    
    return retval; 
}

static char * get_groups_containing_grouped_attr(request_rec *r,
                                                 const char *user,
                                                 const char *pass,
                                                 psldap_config_rec *conf)
{
    char *retval = NULL;
    const char *groupkey = (NULL == conf->psldap_user_grp_attr) ? NULL :
        get_ldap_val(r, user, pass, conf, NULL, NULL,
                     conf->psldap_user_grp_attr, ":");
    if (NULL != groupkey)
    {
        while(groupkey[0])
        {
            char *v = ap_getword(r->pool, &groupkey,':');
            char *groups = (NULL == conf->psldap_grp_mbr_attr) ? NULL :
                get_ldap_val(r, user, pass, conf, conf->psldap_grp_mbr_attr,
                             v, conf->psldap_grp_nm_attr, ",");
            if(NULL != groups)
            {
                ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                             "Found LDAP Groups <%s>", groups);
                retval = (NULL == retval) ? groups :
                    ap_pstrcat(r->pool, retval, ",", groups, NULL);
            }
        }
    }
    else
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                     "Could not identify user LDAP User = <%s = %s>",
                     conf->psldap_userkey, user);
    }
    return retval;
}

static char * get_ldap_grp(request_rec *r, const char *user, const char *pass,
                           psldap_config_rec *conf)
{
    if (conf->psldap_use_ldap_groups)
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                     "Attempting to find group membership in LDAP User = <%s = %s>",
                     conf->psldap_userkey, user);
        return get_groups_containing_grouped_attr(r, user, pass, conf);
    }
    else
    {
        return (NULL == conf->psldap_groupkey) ? NULL :
            get_ldap_val(r, user, pass, conf, NULL, NULL,
                         conf->psldap_groupkey, ",");
    }
}

static int password_matches(const psldap_config_rec *sec, request_rec *r,
                            const char* real_pw, const char *sent_pw)
{
    char errstr[MAX_STRING_LEN] = "";
    conn_rec *c = r->connection;
    int result;

    if (sec->psldap_schemeprefix)
    {
        /* we do use scheme prefixes as described in RFC2307 - currently sha,
           crypt are supported
        */
        if (0 == strncasecmp("{crypt}", real_pw, 7) )
        {
            if (0 != strcmp(real_pw+7, crypt(sent_pw,real_pw+7)))
            {
                ap_snprintf(errstr, sizeof(errstr),
                            "user %s: password mismatch",
                            c->user);
            }
        }
        else if (!strncasecmp("{sha}", real_pw, 5))
        {
            char *digest = ap_palloc (r->pool, SHA1_DIGEST_LENGTH);
            ap_sha1_base64(sent_pw, strlen(sent_pw), digest);
            /** /
                sha1_digest(sent_pw, strlen(sent_pw), digest);
            */
            if (0 != strcmp(real_pw+5, digest))
            {
                ap_snprintf(errstr, sizeof(errstr),
                            "user %s: password mismatch",
                            c->user);
            }
        }
    }
    else if (sec->psldap_cryptpasswords)
    {
        /* Passwords are crypted */
        if(strcmp(real_pw,crypt(sent_pw,real_pw)))
        {  
            ap_snprintf(errstr, sizeof(errstr), 
                        "user %s: password mismatch",c->user);
        }
    }
    else
    {
        /* We have clear text passwords ... */
        if(strcmp(real_pw,sent_pw))
        {  
            ap_snprintf(errstr, sizeof(errstr), 
                        "user %s: password mismatch",c->user);
        }
    }

    if ( !(result = ('\0' == errstr[0])) )
    {
        ap_log_reason (errstr, r->uri, r);
        ap_note_basic_auth_failure (r);
    }

    return result;
}

static int authenticate_via_bind (request_rec *r, psldap_config_rec *sec,
                                  const char *user, const char *sent_pw)
{
    /* Get the userkey to avoid any security issues regarding password
       protection on the server. You can pretty much guarantee that a user
       will be able to read their own userkey
    */
    if(NULL != get_ldap_val(r, user, sent_pw, sec, NULL, NULL,
                            sec->psldap_userkey, ",") )
    {
        return OK;
    }
    if (!(sec->psldap_authoritative)) return DECLINED;

    ap_log_error (APLOG_MARK, APLOG_NOTICE, r->server,
                  "LDAP user %s not found or password invalid", user);
    ap_note_basic_auth_failure (r);
    return HTTP_UNAUTHORIZED;
}

static int authenticate_via_query (request_rec *r, psldap_config_rec *sec,
                                   const char *user, const char *sent_pw)
{
    /* This implementation assumes the password is not under access
       restrictions. This is not necessarily a good assumption.
    */
    char *real_pw;

    if(NULL != (real_pw = get_ldap_val(r, user, sent_pw, sec, NULL, NULL,
                                       sec->psldap_passkey, ",")))
    {
        if(password_matches(sec, r, real_pw, sent_pw))
        {
            return OK;
        }
        else if(sec->psldap_authoritative)
        {
            return HTTP_UNAUTHORIZED;
        }
        return DECLINED;
    }    
    if (!(sec->psldap_authoritative))
    {
        return DECLINED;
    }
    ap_log_error (APLOG_MARK, APLOG_NOTICE, r->server,
                  "LDAP user %s not found or password invalid", user);
    ap_note_basic_auth_failure (r);
    return HTTP_UNAUTHORIZED;
}

static int translate_handler(request_rec *r)
{
    return DECLINED;
}

static int util_read(request_rec *r, char **buf)
{
    int rc = !OK;
    if((rc = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR))!= OK)
    {
        return rc;
    }

    if (ap_should_client_block(r)) {
        int rsize, len_read, rpos=0;
        long length = r->remaining;
        char *argsbuf = (char*)*buf = ap_pcalloc(r->pool, length + 2);

        ap_hard_timeout("util_read", r);
	
        while ((length > rpos) &&
               (rpos += ap_get_client_block(r, argsbuf, length - rpos)) > 0)
        {
            argsbuf = (char*)*buf + rpos;
            ap_reset_timeout(r);
        }
        if(rpos <= length) buf[rpos] = '\0';
        else ap_log_error(APLOG_MARK, APLOG_ERR, r->server,
                          "Buffer overflow reading page response %s",
                          r->unparsed_uri);
        ap_kill_timeout(r);
    }
    else rc = !OK;
    return rc;
}

static int read_post(request_rec *r, table **tab)
{
    const char *tmp;
    char *data, *key, *val;
    int rc = OK;

    /* Ensure values are available for later processing */
    if (NULL == (*tab = r->subprocess_env) )
    {
        *tab = r->subprocess_env = ap_make_table(r->pool, 8);
    }
    else if (NULL != ap_table_get(*tab, FORM_ACTION)) return rc;

    if (r->method_number != M_POST)
    {
        return rc;
    }

    tmp = ap_table_get(r->headers_in, "Content-Type");
    if (0 != strcasecmp(tmp, "application/x-www-form-urlencoded"))
    {
        /* TODO - implement multipart form handling */
        if (0 != strcasecmp(tmp, "multipart/form-data")) return DECLINED;
        return DECLINED;
    }

    if ((rc = util_read(r, &data)) != OK) return rc;
    
    while (('\0' != *data) && (val = ap_getword_nc(r->pool, &data, '&')))
    {
        char *vptr = ap_getword_nc(r->pool, &val, '=');
        if (0 == strcmp(FORM_ACTION, vptr)) key = vptr;
        else key = ap_pstrcat(r->pool, LDAP_KEY_PREFIX,
                              ap_getword_nc(r->pool, &vptr, '-'),
                              NULL);
        /* Spaces remain as '+' if submitted in a form */
        vptr = val;
        ap_unescape_url(val);
        while('\0' != *vptr)
        {
            if(*vptr == '+') *vptr = ' ';
            vptr++;
        }
        if (NULL != (tmp = ap_table_get(*tab, key)) )
        {
            ap_table_setn(*tab, key,
                          ap_pstrcat(r->pool, tmp, ";" , val, NULL));
        }
        else
        {
            ap_table_add(*tab, key, ap_pstrdup(r->pool,val));
        }
    }

    return OK;
}

const char* get_qualified_field_name(request_rec *r, const char *fieldname)
{
    const char *result = fieldname;
    if (0 != strcmp(FORM_ACTION, fieldname))
    {
        result = ap_pstrcat(r->pool, LDAP_KEY_PREFIX, fieldname, NULL);
    }
    return result;
}

static const char *cookie_credential_param = "psldapcredentials";
static const char *cookie_field_label = "PsLDAPField";

static void set_psldap_auth_cookie(request_rec *r, psldap_config_rec *sec,
                                   const char *userValue,
                                   const char *passValue)
{
    char *cookie_string;
    char *cookie_value;
    int secure = sec->psldap_secure_auth_cookie;

    cookie_value = ap_pstrcat(r->pool, sec->psldap_userkey, "=", userValue,
                              "&", sec->psldap_passkey, "=", passValue, NULL);
    cookie_string = ap_pstrcat(r->pool, cookie_credential_param, "=",
                               ap_pbase64encode(r->pool, cookie_value),
                               "; path=/",
                               (secure) ? "; secure" : "", NULL);
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                 "Adding auth cookie Set-Cookie: %s", cookie_string);
    ap_table_add(r->err_headers_out, "Set-Cookie", cookie_string);
}

static int get_form_fieldvalue(request_rec *r, const char* fieldname,
                               char **sent_value)
{
    int result = 0;
    char *tmp = NULL;
    table *tab = NULL;

    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                 "getting form value for %s from post", fieldname);
    
    read_post(r, &tab);

    if (NULL != tab)
    {
        tmp = (char*)ap_table_get(tab, get_qualified_field_name(r, fieldname));
    }

    if(NULL != tmp)
    {
        tmp = ap_pstrdup(r->pool, tmp);
        *sent_value = ap_getword_nc(r->pool, &tmp, ';');
        ap_table_add(r->notes, fieldname, *sent_value);
        result = 1;
    }
    return result;
}

static int cookie_fieldvalue_cb(void *data, const char *key,
                                const char *value)
{
    request_rec *r = (request_rec*)data;
    char *tmp = ap_pstrdup(r->pool, value);
    const char *fieldname = ap_table_get(r->notes, cookie_field_label);
    char *cookie_string = ap_getword_nc(r->pool, &tmp, ';');
    char *cookie_value;
    
    if ((0 == strcmp("Cookie", key)) &&
        (0 == strcmp(cookie_credential_param,
                     ap_getword_nc(r->pool, &cookie_string, '=')) ) )
    {
        const char *val;
        cookie_string = ap_pbase64decode(r->pool, cookie_string);
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                     "getting cookie value for %s from %s",
                     key, cookie_string);
        while ((*cookie_string != '\0') &&
               (val = ap_getword_nc(r->pool, &cookie_string, '&')) ) {
            const char *key = ap_getword(r->pool, &val, '=');
            if (0 == strcmp(key, fieldname)) {
                ap_table_unset(r->notes, cookie_field_label);
                ap_table_set(r->notes, fieldname, val);
                return 0;
            }
        }
    }
    return 1;
}

static int get_cookie_fieldvalue(request_rec *r, const char* fieldname,
                                 char **sent_value)
{
    int result = 0;
    const char *tmp = NULL;

    /* Read cookie from Cookie field in the header. Cookie string;
       expires string ddd, DD-MMM-CCYY HH:MM:SS GMT; path string; domain string
    */
    ap_table_set(r->notes, cookie_field_label, fieldname);
    ap_table_set(r->notes, fieldname, "");
    ap_table_do(cookie_fieldvalue_cb, r, r->headers_in, "Cookie", NULL);
    ap_table_do(cookie_fieldvalue_cb, r, r->err_headers_out, "Set-Cookie",
                NULL);
    *sent_value = (char*)ap_table_get(r->notes, fieldname);

    if(0 == strlen(*sent_value) )
    {
        result = get_form_fieldvalue(r, fieldname, sent_value);
    }
    else
    {
        *sent_value = ap_pstrdup(r->pool, *sent_value);
        result = 1;
    }
    return result;
}

/** Provide the requested auth value
 * @param r the apache request record
 * @param field the value to acquire, either "user" (user name) or
 *              "pass" (password). Any value other than "user" gets
 *              the password
 * @param sent_value the memory location in which to write the
 *                   resultant string. This value is allocated in the
 *                   pool of r
 * @return 
 */
static int get_provided_authvalue(request_rec *r, const char* field,
                                  char **sent_value)
{
    psldap_config_rec *sec =
        (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                   &psldap_module);
    /* Get the key for the requested login value, if field is not "user"
       assume the password is requested */
    const char* fieldKey = (0 == strcmp("user", field)) ?
        sec->psldap_userkey : sec->psldap_passkey;
    
    if (NULL != ap_auth_type(r))
    {
        char *authType = ap_pstrdup(r->pool, ap_auth_type(r));
        ap_str_tolower(authType);

        if ((0 == strcmp("basic", authType))   ||
            (0 == strcmp("ssl-cert", authType)) )
        {
            if (fieldKey == sec->psldap_userkey) {
                *sent_value = ap_pstrdup(r->pool, r->connection->user);
                return OK;
            }
            if (fieldKey == sec->psldap_passkey) {
                int res = ap_get_basic_auth_pw(r, (const char**)sent_value);
                return res;
            }
            return HTTP_UNAUTHORIZED;
        }
        else if (0 == strcmp("digest", authType))
        {
            if (fieldKey == sec->psldap_userkey) {
                *sent_value = ap_pstrdup(r->pool, r->connection->user);
                return OK;
            }
            if (fieldKey == sec->psldap_passkey) {
                int res = ap_get_basic_auth_pw(r, (const char**)sent_value);
                return res;
            }
            return HTTP_UNAUTHORIZED;
        }
        else if (0 == strcmp("form", authType))
        {
            return get_form_fieldvalue(r, fieldKey, sent_value) ? OK:
                HTTP_UNAUTHORIZED;
        }
        else if (0 == strcmp("cookie", authType))
        {
            return get_cookie_fieldvalue(r, fieldKey, sent_value) ? OK:
                HTTP_UNAUTHORIZED;
        }
    }
    else
    {
        table *env = r->subprocess_env;
        *sent_value = ap_pstrdup(r->pool, ap_table_get(env, fieldKey) );
        if (NULL != *sent_value) return OK;
    }
    return DECLINED;
}

static int get_provided_password(request_rec *r, char **sent_pw)
{
    return get_provided_authvalue(r, "pass", sent_pw);
}

static int get_provided_username(request_rec *r, char **sent_user)
{
    return get_provided_authvalue(r, "user", sent_user);
}

static int ldap_authenticate_user_old (request_rec *r)
{
    psldap_config_rec *sec =
        (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                   &psldap_module);
    conn_rec *c = r->connection;
    char *sent_pw = NULL;
    char *sent_user = NULL;
    int res = 0;

    if(!sec->psldap_userkey) return DECLINED;
    
    if ((OK != (res = get_provided_password (r, &sent_pw))) ||
        (OK != (res = get_provided_username (r, &sent_user))) )
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO, r->server,
                     "Failed to acquire credentials for authentication",
                     (NULL == sent_user) ? "" : sent_user);
        return res;
    }

    if(sec->psldap_authexternal)
    {
        return authenticate_via_bind (r, sec, sent_user, sent_pw);
    }
    if (sec->psldap_authsimple)
    {
        return authenticate_via_query (r, sec, sent_user, sent_pw);
    }

    return authenticate_via_query (r, sec, sent_user, sent_pw);
}

static int get_provided_credentials(request_rec *r, psldap_config_rec *sec,
                                    char **sent_pw, char **sent_user)
{
    int result;
    char *authType = (char*)ap_auth_type(r);

    if ((OK == (result = get_provided_password(r, sent_pw)) ) &&
        (OK == (result = get_provided_username(r, sent_user)) ) )
    {
        if (NULL != authType)
        {
            authType = ap_pstrdup(r->pool, authType);
            ap_str_tolower(authType);
            if ((0 == strcmp("form", authType) ) ||
                (0 == strcmp("cookie", authType) ) )
            {
                set_psldap_auth_cookie(r, sec, *sent_user, *sent_pw);
            }
        }
    }
    
    /* Recurse up the request chain to get credentials */
    if ((OK != result) && (NULL != r->prev))
    {
        result = get_provided_credentials(r->prev, sec, sent_pw, sent_user);
    }
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                 "Credentials %saquired via %s auth for user %s",
                 (OK == result) ? "" : "not ", authType, *sent_user);
    return result;
}

static const char *cookie_next_uri = "PsLDAPNextUri";
static int cookie_next_uri_cb(void *data, const char *key, const char *value)
{
    request_rec *r = (request_rec*)data;
    char *tmp = ap_pstrdup(r->pool, value);
    char *cookie_string = ap_getword_nc(r->pool, &tmp, ';');
    
    if (0 == strcmp("Referer", key))
    {
        const char *svr_name = ap_get_server_name(r);
        int name_pos = -1;
        if((NULL != (cookie_string = strstr(cookie_string, svr_name)) ) &&
           (NULL != (cookie_string = strchr(cookie_string, '/')) ) )
            /*
              if(NULL != strstr(cookie_string, svr_name))
            */
        {
            ap_table_setn(r->notes, cookie_next_uri, cookie_string);
            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                         "Next uri after auth set from referrer %s",
                         cookie_string);
        }
    }
    else if ((0 == strcmp("Cookie", key)) &&
             (0 == strcmp(cookie_next_uri,
                          ap_getword_nc(r->pool, &cookie_string, '=')) ) )
    {
        const char *val;
        cookie_string = ap_pbase64decode(r->pool, cookie_string);
        ap_table_setn(r->notes, cookie_next_uri, cookie_string);
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                     "Next uri found after auth %s", cookie_string);
        return 0;
    }
    return 1;
}

static const char* get_post_login_uri(request_rec *r)
{
    int result = 0;
    const char *tmp = NULL;

    /* Read cookie from Cookie field in the header. Cookie string;
       expires string ddd, DD-MMM-CCYY HH:MM:SS GMT; path string; domain string
    */
    ap_table_set(r->notes, cookie_next_uri, "");
    /*ap_table_do(cookie_next_uri_cb, r, r->headers_in, "Cookie", NULL);*/
    ap_table_do(cookie_next_uri_cb, r, r->headers_in, NULL);

    return ap_table_get(r->notes, cookie_next_uri);
}

static void set_post_login_uri(request_rec *r, const char *post_login_uri)
{
    char *cookie_string;
    char *cookie_value;

    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                 "Setting post login uri to %s", post_login_uri);
    cookie_value = ap_pstrcat(r->pool, post_login_uri, NULL);
    cookie_string = ap_pstrcat(r->pool, cookie_next_uri, "=",
                               ap_pbase64encode(r->pool, cookie_value),
                               "; path=/", NULL);
    ap_table_add(r->err_headers_out, "Set-Cookie", cookie_string);
}

static int ldap_authenticate_user (request_rec *r)
{
    psldap_config_rec *sec =
        (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                   &psldap_module);
    conn_rec *c = r->connection;
    char *sent_pw = NULL;
    char *sent_user = NULL;
    int res = 0;

    if(!sec->psldap_userkey) return DECLINED;
    
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO, r->server,
                 "Authenticating LDAP user");
    if (OK != (res = get_provided_credentials (r, sec, &sent_pw, &sent_user
                                               )))
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO, r->server,
                     "Failed to acquire credentials for authentication",
                     (NULL == sent_user) ? "" : sent_user);
    }
    else
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO, r->server,
                     "Authenticating user %s with passed credentials",
                     (NULL == sent_user) ? "" : sent_user);
        if(sec->psldap_authexternal)
        {
            res = authenticate_via_bind (r, sec, sent_user, sent_pw);
        }
        else if (sec->psldap_authsimple)
        {
            res = authenticate_via_query (r, sec, sent_user, sent_pw);
        }
        else res = authenticate_via_query (r, sec, sent_user, sent_pw);
    }
	
    if (res == HTTP_UNAUTHORIZED)
    {
        char *authType = (char*)ap_auth_type(r);
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO, r->server,
                     "Credentials don't exist, sending form for %s auth: %s",
                     authType, sec->psldap_credential_uri);
        if (NULL == authType) authType = "";
        authType  = ap_pstrdup(r->pool, authType);
        ap_str_tolower(authType);
        if ((0 == strcmp(authType, "cookie")) &&
            (0 < strlen(sec->psldap_credential_uri)) )
        {
            /* We should return the credential page if authtype is cookie,
               and the credential uri is set */
            const char *post_login_uri = ap_pstrdup(r->pool, r->uri);
            request_rec *sr = ap_sub_req_lookup_uri(sec->psldap_credential_uri,
                                                    r);
            sr->prev = r;
            r->content_type = sr->content_type;
            sr->subprocess_env = ap_copy_table(sr->pool, r->subprocess_env);
            ap_soft_timeout("update ldap data", r);
            ap_send_http_header(r);
            res = ap_run_sub_req(sr);
            if (ap_is_HTTP_SUCCESS(res))
            {
                res = DONE;
            }
            res = DONE;
            ap_destroy_sub_req(sr);
            ap_kill_timeout(r);
        }
    }
    return res;
}

/* Checking ID */
    
static int ldap_check_auth(request_rec *r)
{
    psldap_config_rec *sec =
        (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                   &psldap_module);
    long methodMask = 1 << r->method_number;
    char *errstr = NULL;
    
    const array_header *reqs_arr = ap_requires (r);
    require_line *reqs = reqs_arr ? (require_line *)reqs_arr->elts : NULL;

    register int x;
    char *user = NULL;
    char *sent_pw = NULL;
    const char *orig_groups = NULL;
    int groupRequirementExists = 0;

    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO, r->server,
                 "Checking LDAP user authentication");
    if (!reqs_arr || (OK != get_provided_credentials (r, sec, &sent_pw, &user
                                                      )) )
    {
        return DECLINED;
    }
    

    /* Check the membership in any required groups.*/
    for(x=0; x < reqs_arr->nelts; x++)
    {
        const char *t;
        const char *w;
        if (0 == (reqs[x].method_mask & methodMask)) continue;

        t = reqs[x].requirement;
        w = ap_getword(r->pool, &t, ' ');
	
        if(0 == strcmp(w,"group"))
        {
            char *v;
            const char *groups;

            groupRequirementExists = 1;
            if (NULL != (groups = get_ldap_grp(r, user, sent_pw, sec))) {
                /* orig_groups is never set to NULL here, we rely on this to
                   determine if a group requirement was present
                */
                orig_groups = groups;
                while('\0' != t[0])
                {
                    w = ap_getword(r->pool, &t, ' ');
                    groups = orig_groups;
                    while(groups[0])
                    {
                        v = ap_getword(r->pool, &groups,',');
                        if(0 == strcmp(v,w)) return OK;
                    }
                }
                errstr = ap_pstrcat(r->pool, "user ", user, " <", orig_groups,
                                    "> not in correct group <", t, ">", NULL);
            }
            else
            {
                errstr = ap_pstrcat(r->pool, "groups for user", user,
                                    " not found in LDAP directory (key ",
                                    (NULL == sec->psldap_groupkey) ? "NULL" :
                                    sec->psldap_groupkey, ")", NULL);
            }
        }
    }
    /* If no groups were required, return OK */
    if ((NULL == sec->psldap_groupkey) && !groupRequirementExists) return OK;
    
    if (!sec->psldap_authoritative) return DECLINED;
    if (NULL != errstr) ap_log_reason (errstr, r->filename, r);

    ap_note_basic_auth_failure (r);

    return HTTP_UNAUTHORIZED;
}

static int compare_arg_strings(const void *arg1, const void *arg2)
{
    return strcmp(*((char**)arg1), *((char**)arg2) );
}

static array_header* parse_arg_string(pool *p, const char **arg_string,
                                      const char delimiter)
{
    array_header *result = ap_make_array(p, 1, sizeof(char*));
    while('\0' != (*arg_string)[0])
    {
        char **w = ap_push_array(result);
        *w = ap_getword(p, arg_string, delimiter);
    }
    qsort(result->elts, result->nelts, result->elt_size, compare_arg_strings);
    return result;
}

typedef struct {
    array_header *keepers;
    array_header *deletions;
    array_header *additions;
} psldap_txns;

static psldap_txns* get_transactions(pool *p, array_header *old_v,
                                     array_header *new_v)
{
    psldap_txns *result = ap_palloc(p, sizeof(psldap_txns));
    int i = 0, j = 0;
    result->keepers = ap_make_array(p, 1, sizeof(char*));
    result->deletions = ap_make_array(p, 1, sizeof(char*));
    result->additions = ap_make_array(p, 1, sizeof(char*));

    while ((i < old_v->nelts) || (j < new_v->nelts)) {
        char **value;
        if (j >= new_v->nelts) {
            value = (char**)ap_push_array(result->deletions);
            *value = ap_pstrdup(p, ((char**)(old_v->elts))[i++]);
        } else if (i >= old_v->nelts) {
            value = (char**)ap_push_array(result->additions);
            *value = ap_pstrdup(p, ((char**)(new_v->elts))[j++]);
        } else {
            int compare = strcmp(((char**)(old_v->elts))[i],
                                 ((char**)(new_v->elts))[j]);
            if (compare < 0) {
                value = (char**)ap_push_array(result->deletions);
                *value = ap_pstrdup(p, ((char**)(old_v->elts))[i++]);
            } else if (compare > 0) {
                value = (char**)ap_push_array(result->additions);
                *value = ap_pstrdup(p, ((char**)(new_v->elts))[j++]);
            } else {
                value = (char**)ap_push_array(result->keepers);
                *value = ap_pstrdup(p, ((char**)(old_v->elts))[i++]);
                j++;
            }
        }
    }
    return result;
}

static int get_dn_attributes_from_ldap(void *data, const char *key,
                                       const char *val)
{
    return FALSE;
}

static int login_and_reply(void *data, const char *key, const char *val)
{
    return TRUE;
}

static int update_dn_attributes_in_ldap(void *data, const char *key,
                                        const char *val)
{
    psldap_status *ps = (psldap_status*)data;
    request_rec *r = ps->rr;
    LDAP *ldap = ps->ldap;
    psldap_config_rec *conf = ps->conf;
    const char *oldValue = NULL;
    char *user;

    get_provided_username(r, &user);
    if (0 == strncmp(LDAP_KEY_PREFIX, key, LDAP_KEY_PREFIX_LEN))
    {
        char *ldap_query = NULL, *ldap_base = NULL;
        
        ldap_query = construct_ldap_query(r, conf, NULL, NULL, user);
        ldap_base = construct_ldap_base(r, conf, ldap_query);
        oldValue = get_ldvalues_from_connection(r, conf, ldap, ldap_base,
                                                ldap_query, user,
                                                key + LDAP_KEY_PREFIX_LEN,
                                                ";");
    }

    if (0 == strncmp(LDAP_KEY_PREFIX, key, LDAP_KEY_PREFIX_LEN))
    {
        /* Use ldap_modify_s here to directly modify the entries. Possibly add
           the LDAPMod to an array passed in the data. */
        const char *value = ap_pstrdup(r->pool, val);
        array_header *new_v = parse_arg_string(r->pool, &value, ';');
        ap_rprintf(r, " Attribute %s -> %s", key + LDAP_KEY_PREFIX_LEN,
                   ap_array_pstrcat(r->pool, new_v, ';') );
        
        if(NULL == oldValue)
        {
            ap_rprintf(r, "&lt; no old value &gt; &lt;additions:%s&gt;</br>\n",
                       ap_array_pstrcat(r->pool, new_v, ';')
                       );
        }
        else
        {
            array_header *old_v = parse_arg_string(r->pool, &oldValue, ';');
            psldap_txns *txns = get_transactions(r->pool, old_v, new_v);
            ap_rprintf(r, " &lt; oldValue = <em>%s</em> &gt;",
                       ap_array_pstrcat(r->pool, old_v, ';') );
            ap_rprintf(r, " &lt; keepers:%s additions:%s deletions:%s &gt; </br>\n",
                       ap_array_pstrcat(r->pool, txns->keepers, ';'),
                       ap_array_pstrcat(r->pool, txns->additions, ';'),
                       ap_array_pstrcat(r->pool, txns->deletions, ';')
                       );
        }
    }
    return TRUE;
}

static int (*get_action_handler(request_rec *r, const char **action))(void*, const char *,const char *)
{
    get_form_fieldvalue(r, FORM_ACTION, (char**)action);
    if (NULL == *action)
    {
        // Assume this is a read request
        return get_dn_attributes_from_ldap;
    }
    else
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                     "Getting handler for action %s", *action);

    if (0 == strcmp(LOGIN_ACTION, *action))
    {
        return login_and_reply;
    }
    if (0 == strcmp(MODIFY_ACTION, *action))
    {
        return update_dn_attributes_in_ldap;
    }
    if (0 == strcmp(CREATE_ACTION, *action))
    {
        return update_dn_attributes_in_ldap;
    }
    if (0 == strcmp(DISABLE_ACTION, *action))
    {
        return update_dn_attributes_in_ldap;
    }
    if (0 == strcmp(DELETE_ACTION, *action))
    {
        return update_dn_attributes_in_ldap;
    }
    return NULL;
}

static int ldap_update_handler(request_rec *r)
{
    char *password = NULL;
    char *user = NULL;
    char *bindas = NULL;
    table *t_env = NULL;
    int err_code, res = OK;
    LDAP *ldap = NULL;
    psldap_config_rec *conf =
        (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                   &psldap_module);
    if (r->method_number == M_OPTIONS) {
        r->allowed |= (1 << M_GET) | (1 << M_POST);
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO, r->server,
                     "User <%s> ldap update declined",
                     (NULL == user) ? "" : user);
        return DECLINED;
    }

    read_post(r, &t_env);
    
    if (OK != (res = get_provided_credentials (r, conf, &password, &user)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO, r->server,
                     "User <%s> ldap update rejected - missing auth info",
                     (NULL == user) ? "null" : user);
    }
    else if(NULL == (ldap = ldap_init(conf->psldap_hosts, LDAP_PORT)))
    { 
        ap_log_error(APLOG_MARK, APLOG_ERR, r->server,
                     "ldap_init failed on ldap update <%s>",
                     conf->psldap_hosts);
        res = HTTP_INTERNAL_SERVER_ERROR;
    }
    else if(LDAP_SUCCESS != (err_code = ldap_bind_s(ldap,
                                                    bindas = get_user_dn(r, user, password, conf),
                                                    password, conf->psldap_bindmethod) ) )
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE, r->server,
                     "ldap_bind as user <%s> failed on ldap update: %s",
                     bindas, ldap_err2string(err_code));
        res = HTTP_INTERNAL_SERVER_ERROR;
    }
    else
    {
        const char *action = NULL;
        int (*actionHandler)(void *data, const char *key,
                             const char *val);
        actionHandler = get_action_handler(r, &action);
        if (login_and_reply ==actionHandler)
        {
            request_rec *sr;
            const char *post_login_uri = get_post_login_uri(r);
            sr = ap_sub_req_lookup_uri(post_login_uri, r);
            sr->prev = r;
            r->content_type = sr->content_type;
            set_psldap_auth_cookie(r, conf, user, password);
            sr->subprocess_env = ap_copy_table(sr->pool, r->subprocess_env);
            ap_soft_timeout("update ldap data", r);
            ap_send_http_header(r);
            res = ap_run_sub_req(sr);
            if (ap_is_HTTP_SUCCESS(res))
            {
                res = DONE;
            }
            else
            {
                ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                             "LDAP update sub request had problems (%d): %s",
                             res, post_login_uri);
            }
            res = DONE;
            ap_destroy_sub_req(sr);
            ap_kill_timeout(r);
            return res;
        }
        else
        {
            r->content_type = "text/html";
            ap_soft_timeout("update ldap data", r);
            ap_send_http_header(r);
	    
            /* Process the requested update here. Send response - ideally
               this will send back the page with a status in a status div
               and the submit buttons hidden 
            */
            ap_rvputs(r,
                      "<body>\n"
                      "<h2>", action, " was not performed as ", user,
                      ", functionality is disabled</h2>\n",
                      NULL);
            if (NULL != t_env)
            {
                /* Add ldap connection pointer to the request and process
                   the table entries corresponding to the ldap attributes
                */
                psldap_status ps;
                ps.rr = r;
                ps.ldap = ldap;
                ps.conf = conf;
                ap_table_do(actionHandler, (void*)&ps, t_env, NULL);
            }
            ap_rputs("</body>", r);
            ap_kill_timeout(r);
            res = OK;
        }
	
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG, r->server,
                     "ldap_bind as user <%s> status on ldap update: %s",
                     bindas, ldap_err2string(err_code));
        ldap_unbind_s(ldap);
    }
    return res;
}

handler_rec ldap_handlers [] =
{
    {"ldap-update", ldap_update_handler},
    {"application/x-ldap-update", ldap_update_handler},
    {"application/x-httpd-psldap", ldap_update_handler},
    {NULL}
};

module MODULE_VAR_EXPORT psldap_module =
{
    STANDARD_MODULE_STUFF,
    module_init,	/* initializer                        */
    create_ldap_auth_dir_config,/* per-directory config creator       */
    merge_ldap_auth_dir_config,	/* dir config merger                  */
    NULL,			/* server config creator              */
    NULL,			/* server config merger               */
    ldap_auth_cmds,	/* config directive table             */
    ldap_handlers,	/* [9]  content handlers              */
    translate_handler,			/* [2]  URI-to-filename translation   */
    ldap_authenticate_user,		/* [5]  check/validate user_id        */
    ldap_check_auth,/* [6]  check user_id is valid *here* */
    NULL,			/* [4]  check access by host address  */
    NULL,			/* [7]  MIME type checker/setter      */
    NULL,			/* [8]  fixups                        */
    NULL,			/* [10] logger                        */
    NULL,			/* [3]  header parser                 */
    NULL,			/* process initialization             */
    NULL,			/* process exit/cleanup               */
    NULL			/* post read_request handling         */
};
