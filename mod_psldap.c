/*
  * mod_psldap
  *
  * User Authentication against and maintenance of an LDAP database
  *
  * David Picard dpicard@psind.com
  *
  * http://www.psind.com:8080/projects/mod_psldap/
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
#include <lber.h>
#include <ldap.h>
#include <unistd.h>

#define SHA1_DIGEST_LENGTH 29 /* sha1 digest length including null character */

#define INT_UNSET -1
#define STR_UNSET NULL

/*
  conf->psldap_use_ldap_groups - boolean value to indicate if ldap group
  queries are to nbe executed to determine the group (default: off)
  conf->psldap_user_grp_attr - attribute of the user to identify the user
  as a group member (default: dn)
  conf->psldap_grp_mbr_attr - the attribute of the group object used to
  identify each user (default: uniqueMember)
  conf->psldap_grp_nm_attr - the attribute of the group object used to
  identify the group (default: cn)
*/

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

} psldap_config_rec;

void *create_ldap_auth_dir_config (pool *p, char *d)
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
    { NULL }
};

module psldap_module;

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

static char * get_ldap_val(request_rec *r, const char *user, const char *pass,
                           psldap_config_rec *conf,
                           const char *query_by, const char *query_for,
                           const char *attr, const char *separator) {
    const char *bindas = NULL, *bindpass = NULL;
    LDAP *ldap = NULL;
    LDAPMessage *ld_result = NULL, *ld_entry = NULL;
    char *ldap_query = NULL;
    char *ldap_base = NULL;
    const char *ldap_attrs[2] = {LDAP_NO_ATTRS, NULL};
    char **ld_values = NULL;
    char *retval = NULL;
    int  ldap_scope;
    int  err_code;
    char *user_dn;

    user_dn = get_user_dn(r, user, pass, conf);

    /* ldap_open is deprecated in future releases, ldap_init is recommended */
    if(NULL == (ldap = ldap_init(conf->psldap_hosts, LDAP_PORT)))
    { 
        ap_log_error(APLOG_MARK, APLOG_ERR, r->server, "ldap_init failed <%s>",
                     conf->psldap_hosts);
        goto GET_LDAP_VAL_RETURN;
    }

    /* If no attribute to query_by is passed, or no value to query_for is
       passed - assume this is a query of an attribute within the user's
       record */
    if ((NULL == query_by) || (NULL == query_for))
    {
        ldap_query = ap_pstrcat(r->pool, conf->psldap_userkey, "=", user,
                                NULL);
    }
    else
    {
        ldap_query = ap_pstrcat(r->pool, query_by, "=", query_for,
                                NULL);
    }
    ap_log_error(APLOG_MARK, APLOG_DEBUG, r->server,
                 "User = <%s = %s> : ldap_query = <%s>",
                 conf->psldap_userkey, user, ldap_query);

    if(conf->psldap_authsimple)
    {
        bindas = conf->psldap_binddn;
        bindpass = conf->psldap_bindpassword;
    }
    else if (conf->psldap_authexternal)
    {
        /* Should we really check for empty passwords? What if the password in
           the LDAP server is blank, like for a guest account? Also note that
           this implementation requires the username to be the first segment of
           the dn and for this record to be located directly under the BaseDN
        */
        if ((NULL == pass) || (0 == strcmp(pass,"")))
        {
            ap_log_reason("ldap_bind: no password given (AuthSimple enabled)!",
                          NULL, r);
            goto GET_LDAP_VAL_RETURN;
        }
        bindas = user_dn;
        bindpass = pass;
    }

    if(NULL == attr)
    {
        retval = "bind";
        goto GET_LDAP_VAL_RETURN;
    }

    if(ldap_bind_s(ldap, bindas, bindpass, conf->psldap_bindmethod))
    {
        ap_log_error(APLOG_MARK, APLOG_WARNING, r->server,
                     "ldap_bind as user <%s> failed", bindas);
        goto GET_LDAP_VAL_RETURN;
    }

    ap_log_error(APLOG_MARK, APLOG_INFO, r->server,
                 "ldap_bind as user <%s:%s> succeeded", bindas, bindpass);

    /* check for search scope */
    if(LDAP_SCOPE_BASE == conf->psldap_searchscope)
    {
        ldap_base = ap_pstrdup(r->pool, ldap_query);
    }
    else
    {
        ldap_base = ap_pstrdup(r->pool, conf->psldap_basedn);
    }

    ap_log_error(APLOG_MARK, APLOG_DEBUG, r->server, "ldap_base = <%s>",
                 ldap_base);

    /* Set the attribute list to return to include only the requested value.
       This is done to avoid false errors caused when querying more secure
       LDAP servers that protect information within the records.
    */
    ldap_attrs[0] = attr;
    if(LDAP_SUCCESS !=
       (err_code = ldap_search_s(ldap, ldap_base, conf->psldap_searchscope,
                                 ldap_query, (char**)ldap_attrs, 0, &ld_result))
       )
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE, r->server,
                     "ldap_search failed: %s", ldap_err2string(err_code));
        goto GET_LDAP_VAL_RETURN;
    }

    if(!(ld_entry = ldap_first_entry(ldap, ld_result)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE, r->server,
                     "user <%s> not found", user);
        goto GET_LDAP_VAL_RETURN;
    }
    if(!(ld_values = ldap_get_values(ldap, ld_entry, attr)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE, r->server,
                     "ldap_get_values <%s> failed", attr);
        goto GET_LDAP_VAL_RETURN;
    }
    
    retval = build_string_list(r, ld_values, separator);

 GET_LDAP_VAL_RETURN:
    if (NULL != ld_values)
    {
        ldap_value_free(ld_values);
    }
    if (NULL != ld_result)
    {
        ldap_msgfree(ld_result);
    }
    if (NULL != ldap)
    {
        ldap_unbind_s(ldap);
    }
    
    return retval; 
}

static char * get_groups_containing_grouped_attr(request_rec *r,
                                                 char *user, char *pass,
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
                ap_log_error(APLOG_MARK, APLOG_DEBUG, r->server,
                             "Found LDAP Groups <%s>", groups);
                retval = (NULL == retval) ? groups :
                    ap_pstrcat(r->pool, retval, ",", groups, NULL);
            }
        }
    }
    else
    {
        ap_log_error(APLOG_MARK, APLOG_DEBUG, r->server,
                     "Could not identify user LDAP User = <%s = %s>",
                     conf->psldap_userkey, user);
    }
    return retval;
}

static char * get_ldap_grp(request_rec *r, char *user, char *pass,
                           psldap_config_rec *conf)
{
    if (conf->psldap_use_ldap_groups)
    {
        ap_log_error(APLOG_MARK, APLOG_DEBUG, r->server,
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
            sha1_digest(sent_pw, strlen(sent_pw), digest);
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
    return AUTH_REQUIRED;
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
            return AUTH_REQUIRED;
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
    return AUTH_REQUIRED;
}

static int get_provided_password(request_rec *r, const char **sent_pw)
{
    if (NULL != ap_auth_type(r))
    {
        char *authType = ap_pstrdup(r->pool, ap_auth_type(r));
        ap_str_tolower(authType);
        if (0 == strcmp("basic", authType))
        {
            return ap_get_basic_auth_pw (r, sent_pw);
        }
        else if (0 == strcmp("digest", authType))
        {
            return AUTH_REQUIRED;
        }
        else if (0 == strcmp("form", authType))
        {
            return AUTH_REQUIRED;
        }
        else if (0 == strcmp("ssl-cert", authType))
        {
            return AUTH_REQUIRED;
        }
    }
    else
    {
        table *env = r->subprocess_env;
        psldap_config_rec *sec =
            (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                       &psldap_module);
        *sent_pw = ap_pstrdup(r->pool, ap_table_get(env, sec->psldap_passkey));
        if (NULL != *sent_pw) return OK;
    }
    return DECLINED;
}

static int ldap_authenticate_user (request_rec *r)
{
    psldap_config_rec *sec =
        (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                   &psldap_module);
    conn_rec *c = r->connection;
    char *sent_pw;
    int res = 0;

    if (OK != (res = get_provided_password (r, (const char **)&sent_pw)))
    {
        return res;
    }

    if(!sec->psldap_userkey) return DECLINED;
    
    if(sec->psldap_authexternal)
    {
        return authenticate_via_bind (r, sec, c->user, sent_pw);
    }
    if (sec->psldap_authsimple)
    {
        return authenticate_via_query (r, sec, c->user, sent_pw);
    }

    return authenticate_via_query (r, sec, c->user, sent_pw);
}
    
/* Checking ID */
    
static int ldap_check_auth(request_rec *r)
{
    psldap_config_rec *sec =
        (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                   &psldap_module);
    char *user = r->connection->user;
    long methodMask = 1 << r->method_number;
    char *errstr = NULL;
    
    const array_header *reqs_arr = ap_requires (r);
    require_line *reqs = reqs_arr ? (require_line *)reqs_arr->elts : NULL;

    register int x;
    const char *t;
    char *w, *sent_pw;
    const char *orig_groups = NULL;
    int groupRequirementExists = 0;

    if (!reqs_arr || ap_get_basic_auth_pw (r, (const char **)&sent_pw) )
    {
        return DECLINED;
    }

    /* Check the membership in any required groups.*/
    for(x=0; x < reqs_arr->nelts; x++)
    {
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

    return AUTH_REQUIRED;
}

static int update_dn_attributes_in_ldap(void *data, const char *key,
                                        const char *val)
{
    request_rec *r = (request_rec*)data;
    /* Use ldap_modify_s here to directly modify the entries. Possibly add
       the LDAPMod to an array passed in the data. */
    ap_rprintf(r, " Attribute %s -> %s</br>\n", key, val);
    return TRUE;
}

static int util_read(request_rec *r, const char **buf)
{
    int rc;
    if((rc = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR))!= OK)
    {
        return rc;
    }

    if (ap_should_client_block(r)) {
        char argsbuffer[HUGE_STRING_LEN];
        int rsize, len_read, rpos=0;
        long length = r->remaining;
        *buf = ap_pcalloc(r->pool, length + 1);

        ap_hard_timeout("util_read", r);
	
        while ((len_read =
                ap_get_client_block(r, argsbuffer, sizeof(argsbuffer))) > 0)
        {
            ap_reset_timeout(r);
            rsize = ((rpos + len_read) > length) ? (length - rpos) : len_read;
            memcpy((char*)*buf + rpos, argsbuffer, rsize);
            rpos += rsize;
        }
        ap_kill_timeout(r);
    }
    return rc;
}

static int read_post(request_rec *r, table **tab)
{
    const char *data, *type;
    char *key;
    char *val;
    int rc = OK;

    if (r->method_number != M_POST)
    {
        return rc;
    }
    type = ap_table_get(r->headers_in, "Content-Type");
    if (0 != strcasecmp(type, "application/x-www-form-urlencoded"))
    {
        return DECLINED;
    }
    if ((rc = util_read(r, &data)) != OK)
    {
        return rc;
    }

    if (NULL != *tab)
    {
        ap_clear_table(*tab);
    }
    else
    {
        *tab = ap_make_table(r->pool, 8);
    }

    while (*data && (val = ap_getword(r->pool, &data, '&')))
    {
        key = ap_getword(r->pool, &val, '=');
        ap_unescape_url(key);
        ap_unescape_url(val);
        ap_table_add(*tab, key, val);
    }

    return OK;
}

static int ldap_update_handler(request_rec *r)
{
    char *password = NULL;
    const char *user = r->connection->user;
    char *bindas = NULL;
    table *t_env = NULL;
    int res;
    LDAP *ldap = NULL;
    psldap_config_rec *conf =
        (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                   &psldap_module);

    if (OK != (res = get_provided_password(r, (const char **)&password)))
    {
        return res;
    }

    read_post(r, &t_env);
    
    if ((NULL == user) && (NULL != t_env))
    {
        user = ap_table_get(t_env, conf->psldap_userkey);
    }

    ap_log_error(APLOG_MARK, APLOG_INFO, r->server,
                 "User <%s> request ldap update", user);

    if(NULL == (ldap = ldap_init(conf->psldap_hosts, LDAP_PORT)))
    { 
        ap_log_error(APLOG_MARK, APLOG_INFO, r->server,
                     "ldap_init failed <%s>", conf->psldap_hosts);
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    else
    {
        int err_code;
        bindas = get_user_dn(r, user, password, conf);

        if(LDAP_SUCCESS != (err_code = ldap_bind_s(ldap, bindas, password,
                                                   conf->psldap_bindmethod)))
        {
            ap_log_error(APLOG_MARK, APLOG_NOTICE, r->server,
                         "ldap_bind as user <%s:%s> failed: %s", bindas,
                         password, ldap_err2string(err_code));
            return HTTP_INTERNAL_SERVER_ERROR;
        }
        else
        {
            /* Process the requested update here. Send response - ideally
               this will be an XSL transform */
            r->content_type = "text/html";
            ap_send_http_header(r);
            ap_rvputs(r,
                      "<body>\n"
                      "<h1>Updates were not performed as ", user,
                      ", functionality is disabled</h1>\n",
                      NULL);
            if (NULL != t_env)
            {
                ap_table_do(update_dn_attributes_in_ldap, (void*)r, t_env,
                            NULL);
            }
            ap_rputs("</body>", r);
	    
            ap_log_error(APLOG_MARK, APLOG_INFO, r->server,
                         "ldap_bind as user <%s:%s> succeeded: %s", bindas,
                         password, ldap_err2string(err_code));
            ldap_unbind_s(ldap);
        }
    }

    return OK;
}

handler_rec ldap_handlers [] =
{
    {"ldap-update", ldap_update_handler},
    {"application/x-ldap-update", ldap_update_handler},
    {NULL}
};

module MODULE_VAR_EXPORT psldap_module =
{
    STANDARD_MODULE_STUFF,
    NULL,			/* initializer                        */
    create_ldap_auth_dir_config,/* per-directory config creator       */
    merge_ldap_auth_dir_config,	/* dir config merger                  */
    NULL,			/* server config creator              */
    NULL,			/* server config merger               */
    ldap_auth_cmds,		/* config directive table             */
    ldap_handlers,		/* [9]  content handlers              */
    NULL,			/* [2]  URI-to-filename translation   */
    ldap_authenticate_user,	/* [5]  check/validate user_id        */
    ldap_check_auth,		/* [6]  check user_id is valid *here* */
    NULL,			/* [4]  check access by host address  */
    NULL,			/* [7]  MIME type checker/setter      */
    NULL,			/* [8]  fixups                        */
    NULL,			/* [10] logger                        */
    NULL,			/* [3]  header parser                 */
    NULL,			/* process initialization             */
    NULL,			/* process exit/cleanup               */
    NULL			/* post read_request handling         */
};
