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

#define PSLDAP_VERSION_LABEL "0.87"

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"
#include "ap_compat.h"
#include "ap_config.h"

#if 0
/* Semaphores will not work on Linux, process sharing semaphores are not
   implemented */
 #define CACHE_USE_SEMAPHORE 1
#endif

#ifdef MPM20_MODULE_STUFF
 #define APACHE_V2
 #include "apr_anylock.h"
 #include "apr_compat.h"
 #include "apr_errno.h"
 #include "apr_general.h"
 #include "apr_hooks.h"
 #include "apr_lib.h"
 #include "apr_pools.h"
 #include "apr_shm.h"
 #include "apr_rmm.h"
 #include "apr_sha1.h"
 #include "apr_shm.h"
 #include "apr_strings.h"
 #define XtOffsetOf APR_OFFSETOF
 #define DEBUG_ERRNO ,APR_SUCCESS
 #define ap_log_reason(a,b,c)
 #define ap_sha1_base64 apr_sha1_base64
 typedef apr_shm_t psldap_shm_mgr;
 #define AP_MM apr_rmm_t
 #define AP_MM_LOCK_RD apr_anylock_readlock
 #define AP_MM_LOCK_RW apr_anylock_writelock
 #define ap_mm_calloc(rm,c,s)	apr_rmm_addr_get((rm),apr_rmm_calloc((rm),((c)*(s))))
 #define ap_mm_destroy	apr_shm_destroy

/* TODO - DJP - The next 2 macros must be verified */
 #define ap_mm_error()	NULL
 #define ap_mm_permission(a,b,c,d) 0


 #define ap_mm_free(rm,ptr)	apr_rmm_free((rm), apr_rmm_offset_get((rm),(ptr))) 
 #define ap_mm_malloc(rm,c,s)	apr_rmm_addr_get((rm),apr_rmm_malloc((rm),((c)*(s))))
 #define ap_mm_realloc(rm,ptr,s) apr_rmm_addr_get((rm),apr_rmm_realloc((rm),(ptr),(s)))
 static char* ap_mm_strdup(apr_rmm_t *rm, const char *str)
 {
    char *result = NULL;
    int len = (NULL == str) ? 0 : strlen(str);

    result = ap_mm_calloc(rm, 1, len + 1);
    if (NULL == str) {
        strncpy(result, str, len);
    }
    result[len] = '\0';

    return result;
 }
 #define ap_mm_lock(g,l) 
 #define ap_mm_unlock(g) 
 typedef apr_pool_t pool;
 typedef apr_table_t table;
 typedef apr_array_header_t array_header;
 #define ap_hard_timeout(n, r)
 #define ap_reset_timeout(r)
 #define ap_kill_timeout(r)
 #define ap_soft_timeout(n, r)
 #define add_version_component(p,v)	ap_add_version_component(p, v)
 static apr_uid_t ap_user_id;
 static apr_gid_t ap_group_id;
 extern module MODULE_VAR_EXPORT psldap_module;
#else
 #define add_version_component(p,v)	ap_add_version_component(v)
 typedef const char *(*cmd_func) (cmd_parms*, void*, const char*);
 typedef int apr_status_t;
 typedef AP_MM psldap_shm_mgr;
 #define APR_SUCCESS 0
 #define apr_rmm_destroy(rm) NULL
 #include "http_conf_globals.h"
 #include "ap_mm.h"
 #include "ap_sha1.h"
 #define DEBUG_ERRNO 
 module MODULE_VAR_EXPORT psldap_module;
#endif

#include <lber.h>
#include <ldap.h>
#include <time.h>
#include <unistd.h>

#ifdef CACHE_USE_SEMAPHORE
 #include <semaphore.h>
#endif

#define SHA1_DIGEST_LENGTH 29 /* sha1 digest length including null character */

#define INT_UNSET -1
#define STR_UNSET NULL

#define BINARY_REFS		"BinaryHRef"
#define BINARY_DATA		"BinaryData"
#define BINARY_TYPE		"BinaryType"
#define FORM_ACTION		"FormAction"
#define LOGIN_ACTION	"Login"
#define SEARCH_ACTION	"Search"
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
    int   psldap_auth_enabled;
    int   psldap_searchscope;
    int   psldap_authoritative;
    int   psldap_cryptpasswords;
    int   psldap_authsimple;
    int   psldap_authexternal;
    int   psldap_bindmethod;
    int   psldap_schemeprefix;
    int   psldap_use_ldap_groups;
    int   psldap_secure_auth_cookie;
    char *psldap_cookiedomain;
    char *psldap_credential_uri;
    int   psldap_cache_auth;
    int   psldap_ldap_version;
    char *psldap_auth_filter;
    int   psldap_authz_enabled;
} psldap_config_rec;

typedef struct {
    char *psldap_sm_file;
    int psldap_sm_size;
    int psldap_max_records;
    int psldap_purge_interval;
} psldap_server_rec;

/* -------------------------------------------------------------*/
/* --------------------- Caching code --------------------------*/
/* -------------------------------------------------------------*/

#define MAX_RECORDS			1000
#define MAX_RECORDS_STR		"1000"
#define MAX_RECORD_SZ		1000
#define MAX_INACTIVE_TIME	900	/* 15 minutes */
#define MAX_INACTIVE_TIME_STR "900"	/* 15 minutes */
#define PSLDAP_SHARED_MEM_FILE	"/var/run/apache_psldap.cache"
#define PSLDAP_SHM_SIZE		81920
#define PSLDAP_SHM_SIZE_STR	"81920"
#define PSCACHE_ITEM_FIELDS()	time_t last_access
#define PSLDAP_REDIRECT_URI "PS_Redirect_URI"

typedef struct psldap_array_struct psldap_array;

typedef struct psldap_cache_item_struct {
    PSCACHE_ITEM_FIELDS();
} pscache_item;

#ifdef CACHE_USE_SEMAPHORE
 typedef sem_t psldap_guard;
 #define psldap_guard_init(g,pshr,max)	sem_init(g,pshr,max)
 #define psldap_guard_destroy(g)	sem_destroy(g)
#else
 typedef AP_MM* psldap_guard;
 #define psldap_guard_init(g,pshr,max)	*(g) = dataview.mem_mgr
 #define psldap_guard_destroy(g)		*(g) = NULL
#endif

struct psldap_array_struct
{
    int item_count;			/* The number of items in the node */
    int max_items;			/* The maximum number of items this node
                               may hold */
    psldap_guard ro_guard;	/* Semaphor / object to guard readonly access */
    psldap_guard wr_guard;	/* Semaphor / object to guard write access */
    const pscache_item **items;		/* The allocated array to hold the items */
};

typedef struct
{
    int cache_width;
    psldap_array **multi_cache;
    long max_inactive_time;
    time_t last_purge_t;
} psldap_shared_data;


typedef struct {
    server_rec *s;
    psldap_shm_mgr *shm_mgr;
    AP_MM *mem_mgr;
#ifdef APACHE_V2
    apr_anylock_t lock;
#endif
    int count;
    psldap_shared_data *collection;
    int (***psldap_compares_item)(const void *item1, const void *item2);
    void (**cached_item_free)(void *item);
} psldap_shared_dataview;


static psldap_shared_dataview dataview = {NULL, NULL, NULL,
#ifdef APACHE_V2
					  {apr_anylock_none, NULL},
#endif
					  0, NULL, NULL, NULL};

/** Lock the specified array for read requests, concurrent read operations
    are allowed up to the specified limit on the number of servers. If a
    write operation / lock is pending, the read lock is queued behind the
    pending write lock / operation.
 **/
static void psldap_array_read_lock(psldap_array *array)
{
#ifdef CACHE_USE_SEMAPHORE
    /* If a write lock is pending, don't allow the read until the
       write is performed */
    sem_wait(&(array->wr_guard));
    sem_wait(&(array->ro_guard));
    sem_post(&(array->wr_guard));
#else
    ap_mm_lock(array->ro_guard, AP_MM_LOCK_RD);
#endif
}

/** Release the lock on the read requests against the specified array.
 **/
static void psldap_array_read_unlock(psldap_array* array)
{
#ifdef CACHE_USE_SEMAPHORE
    int roCount = 0;
    sem_post(&(array->ro_guard));
    sem_getvalue(&(array->ro_guard),&roCount);
    if (roCount == HARD_SERVER_LIMIT) {
        /* If a write lock is pending, it is double locked. Remove one
           of the locks to unlock it. If it wasn't locked, unlock it
           after attempting the lock */
        sem_trywait(&(array->wr_guard));
        sem_post(&(array->wr_guard));
    }
#else
    ap_mm_unlock(array->ro_guard);
#endif
}

/** Lock the specified array for write requests, concurrent write operations
    are not allowed, and the lock will block while all current read operations
    complete.
 **/
static void psldap_array_write_lock(psldap_array* array)
{
#ifdef CACHE_USE_SEMAPHORE
    int roCount = 0;
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                 dataview.s,
                 "Acquiring write lock on array");
    sem_wait(&(array->wr_guard));
    /* If the write lock succeeds, wait for the reads to complete
       prior to allowing the write operations */
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                 dataview.s,
                 "Getting ro count from semaphore");
    sem_getvalue(&(array->ro_guard),&roCount);
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                 dataview.s,
                 "RO count from semaphore is %d", roCount);
    if (roCount != HARD_SERVER_LIMIT) {
        sem_wait(&(array->wr_guard));
    }
#else
    ap_mm_lock(array->ro_guard, AP_MM_LOCK_RW);
#endif
}

/** Remove the lock against write operation on the specified array.
 **/
static void psldap_array_write_unlock(psldap_array* array)
{
#ifdef CACHE_USE_SEMAPHORE
    sem_post(&(array->wr_guard));
#else
    ap_mm_unlock(array->ro_guard);
#endif
}

/** Free all shared memory associated / referenced by this array node and
 *  set the reference to the memory to NULL
 *  @param a_array - the reference to the psldap_array reference that is
 *                   to be freed.
 **/
static void psldap_array_free(psldap_array **a_array)
{
    psldap_array *array = *a_array;
    
    if (NULL != array) {
        psldap_array_write_lock(array);
        if (NULL != array->items) {
            ap_mm_free(dataview.mem_mgr, array->items);
        }
        psldap_guard_destroy(&(array->ro_guard));
        psldap_array_write_unlock(array);

        psldap_guard_destroy(&(array->wr_guard));
        ap_mm_free(dataview.mem_mgr, array);
        *a_array = NULL;
    }
}

/** Create and initialize the psldap_array instance with the maximum node
 *  width specified.
 *  @param max_node_size - the maximum number of items that may be contained
 *                         in a node instance.
 *  @return the reference to the newly allocated and initialized psldap_array
 *          object instance
 **/
static psldap_array* psldap_array_create(const int max_record_count)
{
    int i;
    psldap_array *result = ap_mm_calloc(dataview.mem_mgr, 1,
                                        sizeof(psldap_array));
    if (NULL != result) {
        /* Create a guard allowing interprocess access and initialized to a
           value of unlocked */
        psldap_guard_init(&(result->ro_guard), 1, HARD_SERVER_LIMIT);
        psldap_guard_init(&(result->wr_guard), 1, 1);
        result->item_count = 0;
        result->max_items = max_record_count;
        result->items = ap_mm_calloc(dataview.mem_mgr, result->max_items,
                                     sizeof(void*));
        memset(result->items, 0, result->max_items * sizeof(void*));
        if (NULL == result->items) {
            psldap_array_free(&result);
        }
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                     dataview.s,
                     "PSLDAP array created: <%d:%d>", result->ro_guard,
                     result->wr_guard);
    } else {
      	ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, dataview.s,
                     "Error creating array node");
    }
    return result;
}

/** Free all the items maintained within the passed psldap_array reference
 *  @param array - the tree containing the items to free.
 **/
static void free_array_items(psldap_array* array, int collectionNumber)
{
    int i;

    psldap_array_write_lock(array);
    for (i = 0; i < array->item_count; i++)
    {
        dataview.cached_item_free[collectionNumber]((void*)(array->items[i]));
        array->items[i] = NULL;
    }
    psldap_array_write_unlock(array);
}

static const pscache_item* array_find_item(psldap_array *a_array,
                                        int (*compare_item)(const void *item1,
                                                            const void *item2),
                                        const void *key)
{
    pscache_item *const *result = NULL;
    if (NULL != a_array) {
        psldap_array_read_lock(a_array);
        result = (pscache_item**)bsearch(&key, a_array->items,
                                          a_array->item_count,
                                          sizeof(void*), compare_item);
        psldap_array_read_unlock(a_array);
    }
    return (NULL != result) ? *result : NULL;
}

static int compare_access_t(const void *a_item1, const void *a_item2)
{
    int result = 0;
    pscache_item *item1 = *((pscache_item**)a_item1);
    pscache_item *item2 = *((pscache_item**)a_item2);

    if (item1->last_access < item2->last_access) result = -1;
    else if (item1->last_access > item2->last_access) result = 1;

    return result;
}

static short int cache_size_exceeds_pct(psldap_shared_data *data, int pct)
{
    short int result = 0;
    /* Every cache array has the same size */
    psldap_array *cache = (NULL == data->multi_cache) ? NULL :
        data->multi_cache[0];
    if (NULL != cache) {
        result = (((cache->item_count * 100) / cache->max_items) > pct);
    }
    return result;
}

static void purge_stale_cache_items(request_rec *r, psldap_shared_dataview *dv)
{
    int i, j, k;
    for (i = 0; i < dv->count; i++) { 
        psldap_shared_data *data = &(dv->collection[i]);
        /* For now assume the purge interval is the same as the max inactive
           time. Also, if the data cache is more than 95% full, activate the
           purge */
        time_t expires = data->max_inactive_time + data->last_purge_t;
        if ((expires <= time(NULL)) || (cache_size_exceeds_pct(data, 90))) {
            expires = data->max_inactive_time + time(NULL);
            for ( j = data->cache_width - 1; j >= 0; j--) {
                psldap_array *cache = data->multi_cache[j];
                ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                             r->server,
                             "Purging cache dimension: <%d:%d=%d>",
                             i, j, cache->item_count);
                qsort(cache->items, cache->item_count, sizeof(void*),
                      compare_access_t);
                for (k = 0; (k < cache->item_count) &&
                       (cache->items[k]->last_access < expires);
                     k++) {
                    if (j == 0) {
                        dv->cached_item_free[i]((void*)(cache->items[k]));
                    }
                }
                if (k < cache->item_count) {
                    memmove(&cache->items[0], &cache->items[k],
                            (cache->item_count - k) * sizeof(void*));
                }
                cache->item_count -= k;
                qsort(cache->items, cache->item_count, sizeof(void*),
                      dv->psldap_compares_item[i][j]);
                data->last_purge_t = time(NULL);
                ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                             r->server,
                             "Cache dimension <%d:%d=%d> purge completed",
                             i, j, cache->item_count);
            }
        }
    }
    return;
}

static pscache_item* find_cache_item(request_rec *r, int collectionIndex,
                                     int searchBy, const void *key)
{
    pscache_item *result = NULL;
    if( (NULL != dataview.collection) && (dataview.count > collectionIndex) ) {
        psldap_shared_data *data = &(dataview.collection[collectionIndex]);
        if (data->cache_width > searchBy) {
            int (*compare_item)(const void *item1, const void *item2);
            psldap_array *array = data->multi_cache[searchBy];
            compare_item = dataview.psldap_compares_item[collectionIndex][searchBy];
            result = (pscache_item*)array_find_item(array, compare_item, key);
            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                         r->server,
                         "Found cache item in dimension: <%d:%d> = %p",
                         collectionIndex, searchBy, result);
        }
    }
    if (NULL != result) {
        result->last_access = time(NULL);
        purge_stale_cache_items(r, &dataview);
    }

    return result;
}

static int array_insert_item(const void* item, psldap_array* array,
                             int (*compare)(const void* arg1, const void* arg2),
                             int allowDupes)
{
    register int result = -1;
    
    if(NULL != compare) {
        result = 0;

        psldap_array_write_lock(array);

        if(array->item_count == 0) {
            array->item_count++;
        } else if(array->item_count < array->max_items) {
            register int cv;
            register int i = 0, j = array->item_count - 1;
            
            do {
                result = (j + i) / 2;
                cv = compare(&item, &array->items[result]);
                if (0 <= cv) i = result + 1;
                if (0 >= cv) j = result - ((result>i) ? 1 : (i-result));
                if((0 == cv) && (i > j)) j = result; 
            } while(j != result);
            
            if(0 <= cv) result++;
            if((0 != allowDupes) || (0 != cv)) {
                array->item_count++;
                memmove(&array->items[result+1], &array->items[result],
                        (array->item_count - result) * sizeof(void*));
            }
            else result = -1;
        } else {
            result = -1;
        }
        if(result >= 0) array->items[result] = (void*) item;

        psldap_array_write_unlock(array);
    }
    
    return result;
}

static void insert_cache_item(request_rec *r, int collectionIndex,
                              int insertBy, const void *item)
{
    if( (NULL != dataview.collection) && (dataview.count > collectionIndex) ) {
        psldap_shared_data *data = &(dataview.collection[collectionIndex]);
        if (data->cache_width > insertBy) {
            int (*compare_item)(const void *item1, const void *item2);
            psldap_array *array = data->multi_cache[insertBy];
            compare_item = dataview.psldap_compares_item[collectionIndex][insertBy];
            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                         r->server,
                         "Current array size on dimension %d:%d =  %d",
                         collectionIndex, insertBy, array->item_count);
            if (0 > array_insert_item(item, array, compare_item, 0)) {
                ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO,
                             r->server, "Error inserting cache item <%d:%d>!! new size is %d",
                             collectionIndex, insertBy, array->item_count);
            } else {
                ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                             r->server,
                             "Cache item inserted in dimension %d:%d - new size is %d",
                             collectionIndex, insertBy, array->item_count);
            }
        }
    }
}

static int array_remove_item(const void* item, psldap_array* array,
                             int (*compare)(const void* arg1, const void* arg2) )
{
    register int result = -1;
    
    if(NULL != compare) {
        result = 0;

        psldap_array_write_lock(array);
        if(0 != array->item_count) {
            register int cv;
            register int i = 0, j = array->item_count - 1;
            
            do {
                result = (j + i) / 2;
                cv = compare(&item, &array->items[result]);
                if (0 <= cv) i = result + 1;
                if (0 >= cv) j = result - ((result>i) ? 1 : (i-result));
                if((0 == cv) && (i > j)) j = result; 
            } while(j != result);
            
            if(0 != cv) result = -1;
            else memmove(&array->items[result], &array->items[result+1],
                         (array->item_count - result - 1) * sizeof(void*));
        }
        psldap_array_write_unlock(array);

    }
    
    return result;
}

/** Assume 1000 cached records of 1000 bytes * width with an
 *  additional premium of (((max - min node width + 2) / 2) + 6)
 *  * (1000 cached records * 2) / (max - min node width))
 *  @param s - the apache server record
 *  @param width - the maximum number of records to hold
 *  @return the size of the shared memory chunk to hold the records
 **/
static int get_shared_size(server_rec *s, int width)
{
    psldap_server_rec *sec =
        (psldap_server_rec *)ap_get_module_config(s->module_config,
                                                  &psldap_module);
    int result = sec->psldap_max_records;
    result *= (MAX_RECORD_SZ + (width * sizeof(void*)) );
    result += sizeof(psldap_shared_data);

    result = sec->psldap_sm_size;
    return result;
}

/** Get the name of the file to contain the stored memory store
 *  @param s - the apache server record
 *  @return the name of the file to use for the shared memory store
 **/
static char* get_shared_filename(server_rec *s)
{
    psldap_server_rec *sec =
        (psldap_server_rec *)ap_get_module_config(s->module_config,
                                                  &psldap_module);
    return (NULL == sec->psldap_sm_file) ? PSLDAP_SHARED_MEM_FILE :
        sec->psldap_sm_file;
}

/** Get the mode to set against the shared memory store file.
 *  @param s - the apache server record
 *  @return the access mode to set against the shared memory mode file
 **/
static int get_file_mode(server_rec *s)
{
#ifndef WIN32
    return (S_IRUSR|S_IWUSR);
#else
    return (_S_IREAD|_S_IWRITE);
#endif
}

/** Free the passed memory associated with the passed object from the
 *  shared memory store, checks for NULL values on the passed argument.
 *  This method will also free all the items within the tree as it frees
 *  the first dimension of the tree.
 *  @param data - the allocated instance of the shared data object
 **/
static void free_shared_data(psldap_shared_data *data, int collectionNumber)
{
    if (NULL != data) {
        int i;
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                     dataview.s,
                     "Freeing psldap shared data in collection: <%d>",
                     collectionNumber);
        if (data->multi_cache) {
            int items_freed = 0;
            for (i = 0; i < data->cache_width; i++) {
                ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                             dataview.s,
                             "Freeing psldap cache dimenstion: <%d>", i);
                if (!items_freed) {
                    free_array_items(data->multi_cache[i], collectionNumber);
                    items_freed = 1;
                }
                if (NULL != data->multi_cache[i]) {
                    psldap_array_free(&(data->multi_cache[i]));
                }
            }
            ap_mm_free(dataview.mem_mgr, data->multi_cache);
        }
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                     dataview.s,
                     "PSLdap shared data in collection freed: <%d>",
                     collectionNumber);
    }
}

/** Initialize the passed block of memory with the specified width, ensuring
 *  all members are synchronized with the proper width
 *  @param data - the allocated instance of the shared data object
 *  @param width - the number of parallel trees to maintain
 *  @return nothing
 **/
static void init_shared_data(server_rec *s, psldap_shared_data *data,
                             const int width)
{
    if (NULL != data) {
        psldap_server_rec *sec =
            (psldap_server_rec *)ap_get_module_config(s->module_config,
                                                      &psldap_module);
        int i = 0;
        data->max_inactive_time = MAX_INACTIVE_TIME;
        data->last_purge_t = time(NULL);
        data->cache_width = width;

        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                     dataview.s,
                     "Creating multi-cache array: <%d>", width);
        data->multi_cache = ap_mm_calloc(dataview.mem_mgr,
                                         data->cache_width,
                                         sizeof(psldap_array*));
        if (NULL != data->multi_cache) {
            for (i = 0; i < data->cache_width; i++) {
                data->multi_cache[i] =
                    psldap_array_create(sec->psldap_max_records);
                if (NULL == data->multi_cache[i]) {
                    i = 0;
                    break;
                }
            }
        }
        if (0 == i) { free_shared_data(data, dataview.count - 1); }

        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                     dataview.s,
                     "Multi-cache array created: <%d>", width);
    }
}

/** Allocate the shared memory for a specific cache and set local memory
 *  structure as needed to manage the comparison and item free functions.
 *  @param s - apache server record structure
 *  @param p - apache pool object for memory allocation
 *  @param a_free_item - the method to use to free an item in the cache
 *  @param a_compares_item - the array of comparison methods to apply to each
 *                           tree
 *  @param width - number of parallel trees to maintain
 *  @return the memory management object used to create the pool.
 **/
static AP_MM* alloc_shared_data(server_rec *s, pool *p,
                                void (*a_free_item)(void* item),
                                int (**a_compares_item)(const void *item1,
                                                        const void *item2),
                                int width)
{
    psldap_shared_data *result = NULL;
    dataview.s = s;
#ifdef APACHE_V2
    /* TODO - The memory model has changed to such an extent that caching is
       no longer functional in Apache V2. Return immediately and do not
       initialize the cache - remove the return from here when caching is
       again functional. */
    return dataview.mem_mgr;

    if (NULL != s) {
      apr_shm_t *tmp = NULL;
      apr_current_userid(&ap_user_id, &ap_group_id, p);
      if (APR_SUCCESS != apr_shm_attach(&tmp, get_shared_filename(s), p) ) {
	  apr_shm_create(&tmp, get_shared_size(s, width),
			 get_shared_filename(s), p); 
      }
      dataview.shm_mgr = tmp;
      apr_rmm_init(&(dataview.mem_mgr), &(dataview.lock),
		   apr_shm_baseaddr_get(dataview.shm_mgr),
		   get_shared_size(s, width), p);
    }
#else
    dataview.mem_mgr = ap_mm_create(get_shared_size(s, width),
                                    get_shared_filename(s)); 
#endif

    if (NULL == dataview.mem_mgr) {
        /* log error */
        char *strError = ap_mm_error();
        ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, s,
                     "Error creating cache memory manager <%s:%d> : <%s>",
                     get_shared_filename(s), get_shared_size(s, width),
                     (NULL != strError) ? strError : "NO_ERR_STRING");
    } else if (ap_mm_permission(dataview.mem_mgr, get_file_mode(s),
                                ap_user_id, -1) ) {
        ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, s,
                     "Error setting mode on file <%s:%d>",
                     get_shared_filename(s), get_file_mode(s));
    } else {
        dataview.count++;
        if (dataview.collection == NULL) {
            dataview.collection = ap_mm_calloc(dataview.mem_mgr,
                                               dataview.count,
                                               sizeof(psldap_shared_data));
        } else {
            dataview.collection = ap_mm_realloc(dataview.mem_mgr,
                                                dataview.collection,
                                                dataview.count *
                                                sizeof(psldap_shared_data));
        }
        dataview.psldap_compares_item =
            realloc(dataview.psldap_compares_item,
                    dataview.count * sizeof(a_compares_item));
        dataview.psldap_compares_item[dataview.count-1] = a_compares_item;
	
        dataview.cached_item_free =
            realloc(dataview.cached_item_free,
                    dataview.count * sizeof(a_free_item));
        dataview.cached_item_free[dataview.count-1] = a_free_item;
	
        result = &(dataview.collection[dataview.count-1]);
        init_shared_data(s, result, width);
    }
    return (NULL != result) ? dataview.mem_mgr : NULL;
}

/** Cleans up the entire cache and removes the shared file prior to exiting
 *  the apache server process.
 *  @param server - the server instance
 **/
static apr_status_t psldap_server_cleanup(void *server)
{
    int j;
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                 dataview.s,
                 "Cleaning up shared memory on server: <%d>",
                 dataview.count);
    if ( 0 < dataview.count) {
        for (j = 0; j < dataview.count; j++) {
            free_shared_data(&(dataview.collection[j]), j);
        }
        ap_mm_free(dataview.mem_mgr, dataview.collection);
        apr_rmm_destroy(dataview.mem_mgr);
        ap_mm_destroy(dataview.shm_mgr);
        dataview.count = 0;
    }
    return APR_SUCCESS;
}

static apr_status_t psldap_child_cleanup(void *server)
{
    /* Nothing to do here */
    return APR_SUCCESS;
}

void psldap_cache_add_item(request_rec *r, int collectionIndex, void *item)
{
    if (dataview.count > collectionIndex) {
        psldap_shared_data *data = &(dataview.collection[collectionIndex]);
        int i;
        for (i = 0; i < data->cache_width; i++) {
            insert_cache_item(r, collectionIndex, i, item);
        }
    }
}

/** Starts tracking a cache of data within the server. Initializes the
 *  cache internals on the first call, thereafter expanding the cache as
 *  required. The initial file settings for the shared memory store must
 *  allow for the final required memory size for all caches.
 *  @param s
 *  @param p
 *  @param a_free_item - method to invoke to free items in the new cache
 *                       collection
 *  @param a_compares_item - array of methods to use in comparing items in
 *                           each dimension of the cache collection.
 *  @param cache_width - number of dimensions of the cache collection to
 *                       create.
 *  @return an instance to the apache memory manager used to create the
 *          requested cache collection.
 **/
static AP_MM* psldap_cache_start(server_rec *s, pool *sp, pool *p,
                                 void (*a_free_item)(void* item),
                                 int (**a_compares_item)(const void *item1,
                                                         const void *item2),
                                 int cache_width)
{
    ap_register_cleanup(sp, s, psldap_server_cleanup,
                        psldap_child_cleanup);
    return alloc_shared_data(s, p, a_free_item, a_compares_item,
                             cache_width);
}

/* -------------------------------------------------------------*/
/* ----------------- End Caching code --------------------------*/
/* -------------------------------------------------------------*/

typedef enum
{
    CACHE_KEY = 0,
    CACHE_DN,
    CACHE_WIDTH
} cache_search_type;

typedef struct
{
    PSCACHE_ITEM_FIELDS();
    char *lhost;	/* The name of the ldap server hoolding the record */
    char *key;		/* The value of the configured key field */
    char *dn;		/* The distinguished name of the LDAP record*/
    char *passwd;	/* The password passed by the user for the
                       last successful authentication */
    char *groups;	/* The groups for the user in a ',' delmited
                       string */
} psldap_cache_item;

static AP_MM *cache_mem_mgr = NULL;

static void psldap_cache_item_free(void *a_item)
{
    psldap_cache_item *item = a_item;
    if (NULL != item) {
        if (NULL != item->lhost) ap_mm_free(cache_mem_mgr, item->lhost);
        if (NULL != item->key) ap_mm_free(cache_mem_mgr, item->key);
        if (NULL != item->dn) ap_mm_free(cache_mem_mgr, item->dn);
        if (NULL != item->passwd) ap_mm_free(cache_mem_mgr,
                                             item->passwd);
        if (NULL != item->groups) ap_mm_free(cache_mem_mgr,
                                             item->groups);
        ap_mm_free(cache_mem_mgr, item);
    }
}

static LDAP* ps_ldap_init(psldap_config_rec *conf, LDAP_CONST char *host,
			  int port)
{
    int connectVersion = conf->psldap_ldap_version;
    LDAP *ld = ldap_init(host, port);
    ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &connectVersion);
    return ld;
}

static int set_bind_params(request_rec *r, LDAP **ldap,
                           psldap_config_rec *conf,
                           const char **user, char const **password);
static char * get_ldap_grp(request_rec *r, const char *user,
                           const char *pass, psldap_config_rec *conf);
static psldap_cache_item* psldap_cache_item_create(request_rec *r,
                                                   const char *lhost,
                                                   const char *key,
                                                   const char *a_dn,
                                                   const char *passwd,
                                                   const char *a_groups)
{
    server_rec *s = r->server;
    const char *dn = a_dn;
    const char *groups = a_groups;
    psldap_cache_item *result = (NULL == cache_mem_mgr) ? NULL :
        ap_mm_calloc(cache_mem_mgr, 1, sizeof(psldap_cache_item));

    if (NULL != result) {

        /* Insert the values retrieved from LDAP into the cache */
        if ((NULL == dn) || (NULL == groups))
        { /* Set the dn and group if either was not passed */
            LDAP* ldap = NULL;
            psldap_config_rec *conf =
                (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                           &psldap_module);
            if (NULL == (ldap = ps_ldap_init(conf, conf->psldap_hosts,
					     LDAP_PORT)))
            { 
                ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, s,
                             "ldap_init failed <%s>", conf->psldap_hosts);
            }
            if (NULL == dn)
            {
                dn = ap_pstrdup(r->pool, key);
                set_bind_params(r, &ldap, conf, &dn, &passwd);
            }
            if (NULL == groups)
            {
                groups = get_ldap_grp(r, key, passwd, conf);
            }
        }
        result->lhost = ap_mm_strdup(cache_mem_mgr, lhost);
        result->key = ap_mm_strdup(cache_mem_mgr, key);
        result->dn = ap_mm_strdup(cache_mem_mgr, dn);
        result->passwd = ap_mm_strdup(cache_mem_mgr, passwd);
        result->groups = (NULL == groups) ? NULL :
            ap_mm_strdup(cache_mem_mgr, groups);
        if ((NULL == result->key) || (NULL == result->dn) ||
            (NULL == result->passwd) ||
            ((NULL == result->groups) && (NULL != groups)) ) {
            psldap_cache_item_free(result);
            result = NULL;
            ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, s,
                         "Freeing incomplete cache item <%s:%s>",
                         lhost, key);
        } else {
            psldap_cache_add_item(r, 0, result);
        }
    } else if (NULL == cache_mem_mgr) {
      	ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, s,
                     "Error creating cache item <%s:%s>",
                     (NULL == dn) ? "NULL" : dn,
                     (NULL == groups) ? "NULL" : groups);
    }

    return result;
}

static const psldap_cache_item* psldap_cache_item_find(request_rec *r,
                                                       const char *lhost,
                                                       const char *keyvalue,
                                                       const char *dn,
                                                       const char *passwd,
                                                       cache_search_type st)
{
    psldap_cache_item key;
    psldap_cache_item *result = NULL;

    key.lhost = (char*)lhost;
    key.key = (char*)keyvalue;
    key.dn = (char*)dn;
    key.passwd = (char*)passwd;
    
    result = (psldap_cache_item*)find_cache_item(r, 0, st, &key);

    return result;
}

static int get_lderrno(LDAP *ld) 
{
    int result;
    ldap_get_option(ld, LDAP_OPT_ERROR_NUMBER, &result);
    return result;
}

static void set_lderrno(LDAP *ld, int error_code) 
{
    ldap_set_option(ld, LDAP_OPT_ERROR_NUMBER, &error_code);
}

typedef struct
{
    request_rec *rr;
    LDAP *ldap;
    psldap_config_rec *conf;
    char *mod_dn;
    LDAPMessage *mod_record;
    int mod_err;
    int mod_count;
    LDAPMod **mods;
    char *searchPattern;
    char *xslPrimaryUri;
    char *xslSecondaryUri;
    char *fieldName;
    char *responseType;
    int binaryAsHref;
} psldap_status;

static void psldap_status_init(psldap_status *ps, request_rec *r, LDAP *ldap,
                               psldap_config_rec *conf)
{
    ps->rr = r;
    ps->ldap = ldap;
    ps->conf = conf;
    ps->mod_dn = NULL;
    ps->mod_record = NULL;
    ps->mod_err = LDAP_SUCCESS;
    ps->mod_count = 1;
    ps->mods = ap_palloc (r->pool, ps->mod_count * sizeof(LDAPMod*));
    ps->mods[0] = NULL;
    ps->searchPattern = NULL;
    ps->xslPrimaryUri = NULL;
    ps->xslSecondaryUri = NULL;
    ps->fieldName = NULL;
    ps->responseType = NULL;
    ps->binaryAsHref = 0;    
    set_lderrno(ps->ldap, LDAP_SUCCESS);
}

static void psldap_status_append_mod(psldap_status *ps, request_rec *r,
                                     LDAPMod *newMod)
{
    LDAPMod **mods_coll;

    ap_log_error(APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                 "Appending LDAPMod instance for attr: %s",
                 newMod->mod_type);
    ps->mod_count++;
    mods_coll = ap_palloc(r->pool, ps->mod_count * sizeof(LDAPMod*));
    memcpy(mods_coll, ps->mods, (ps->mod_count - 2) * sizeof(LDAPMod*));
    mods_coll[ps->mod_count-2] = newMod;
    mods_coll[ps->mod_count-1] = NULL;
    ps->mods = mods_coll;
    ap_log_error(APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                 "... LDAPMod instance for attr: %s  appended to collection",
                 newMod->mod_type);
}

static int compare_key(const void *a_item1, const void *a_item2)
{
    int result = 0;
    psldap_cache_item *item1 = *((psldap_cache_item**)a_item1); 
    psldap_cache_item *item2 = *((psldap_cache_item**)a_item2); 

    if (0 == (result = strcmp(item1->lhost, item2->lhost))) {
        result = strcmp(item1->key, item2->key);
    }
    return result;
}

static int compare_dn(const void *a_item1, const void *a_item2)
{
    int result = 0;
    psldap_cache_item *item1 = *((psldap_cache_item**)a_item1);
    psldap_cache_item *item2 = *((psldap_cache_item**)a_item2);

    if (0 == (result = strcmp(item1->lhost, item2->lhost))) {
        result = strcmp(item1->dn, item2->dn);
    }
    return result;
}

int (*psldap_compares_item[CACHE_WIDTH])(const void *item1, const void *item2) = {
    compare_key, compare_dn
};

#ifdef APACHE_V2
static int module_initV2(pool *pconf, pool *plog, pool *ptemp, server_rec *s)
{
    add_version_component(pconf, "mod_psldap/" PSLDAP_VERSION_LABEL);
    cache_mem_mgr = psldap_cache_start(s, pconf, pconf, psldap_cache_item_free,
                                       psldap_compares_item, CACHE_WIDTH);
    return APR_SUCCESS;
}
#else
static void module_init(server_rec *s, pool *p)
{
    add_version_component(p, "mod_psldap/" PSLDAP_VERSION_LABEL);
    cache_mem_mgr = psldap_cache_start(s, s->ctx->cr_pool, p,
				       psldap_cache_item_free,
                                       psldap_compares_item, CACHE_WIDTH);
}
#endif

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

    sec->psldap_auth_enabled = INT_UNSET;
    sec->psldap_authz_enabled = INT_UNSET;
    sec->psldap_searchscope = INT_UNSET;
    sec->psldap_authoritative = INT_UNSET;
    sec->psldap_cryptpasswords = INT_UNSET; 
    sec->psldap_authsimple = INT_UNSET;
    sec->psldap_authexternal = INT_UNSET;
    sec->psldap_bindmethod = LDAP_AUTH_NONE;
    sec->psldap_schemeprefix = INT_UNSET;
    sec->psldap_use_ldap_groups = INT_UNSET;
    sec->psldap_secure_auth_cookie = INT_UNSET;
    sec->psldap_cookiedomain = STR_UNSET;
    sec->psldap_credential_uri = STR_UNSET;
    sec->psldap_cache_auth = INT_UNSET;
    sec->psldap_ldap_version = INT_UNSET;

    return sec;
}

static void *create_ldap_auth_srv_config (pool *p, server_rec *s)
{
    psldap_server_rec *sec
        = (psldap_server_rec *)ap_pcalloc (p, sizeof(psldap_server_rec));
    
    sec->psldap_sm_file = STR_UNSET;
    sec->psldap_sm_size = INT_UNSET;
    sec->psldap_max_records = INT_UNSET;
    sec->psldap_purge_interval = INT_UNSET;

    /* TODO - remove these defaults after into values are set from config */
    sec->psldap_sm_size = PSLDAP_SHM_SIZE;
    sec->psldap_max_records = MAX_RECORDS;
    sec->psldap_purge_interval = MAX_INACTIVE_TIME;

    return sec;
}

/** Set a string attribute _a_ in config record _r_ from the attribute _a_ in
 *  config record _n_ if the attribute in _r_ is unset, using apache pool _p_
 */
#define set_cfg_str_if_n_set(_p_, _r_, _n_, _a_)			\
    if ((STR_UNSET == _r_->_a_) && (STR_UNSET != _n_->_a_))		\
        _r_->_a_ = (char*)ap_pstrdup(_p_, _n_->_a_)
	
/** Set an integer attribute _a_ in config record _r_ from the attribute _a_ in
 *  config record _n_ if the attribute in _r_ equals value _c_, otherwise set
 *  to default value _d_
 */
#define set_cfg_int_if_n_set(_r_, _n_, _a_, _c_, _d_)			\
    if (_c_ == _r_->_a_)						\
        _r_->_a_ = ((_c_ == _n_->_a_) ? _d_ : _n_->_a_)

void *merge_ldap_auth_dir_config (pool *p, void *base_conf, void *new_conf)
{
    psldap_config_rec *result = create_ldap_auth_dir_config(p, NULL);
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
    set_cfg_str_if_n_set(p, result, n, psldap_cookiedomain);
    set_cfg_str_if_n_set(p, result, n, psldap_auth_filter);
    set_cfg_int_if_n_set(result, n, psldap_auth_enabled, INT_UNSET, INT_UNSET);
    set_cfg_int_if_n_set(result, n, psldap_authz_enabled, INT_UNSET, INT_UNSET);
    set_cfg_int_if_n_set(result, n, psldap_searchscope, INT_UNSET, INT_UNSET);
    set_cfg_int_if_n_set(result, n, psldap_bindmethod, LDAP_AUTH_NONE,
                         LDAP_AUTH_NONE);
    set_cfg_int_if_n_set(result, n, psldap_authoritative, INT_UNSET,
                         INT_UNSET);
    set_cfg_int_if_n_set(result, n, psldap_cryptpasswords, INT_UNSET,
                         INT_UNSET);
    set_cfg_int_if_n_set(result, b, psldap_authsimple, INT_UNSET, INT_UNSET);
    set_cfg_int_if_n_set(result, b, psldap_authexternal, INT_UNSET,
                         INT_UNSET);
    set_cfg_int_if_n_set(result, b, psldap_schemeprefix, INT_UNSET, INT_UNSET);
    set_cfg_int_if_n_set(result, b, psldap_use_ldap_groups, INT_UNSET,
                         INT_UNSET);
    set_cfg_int_if_n_set(result, b, psldap_secure_auth_cookie, INT_UNSET,
                         INT_UNSET);
    set_cfg_int_if_n_set(result, b, psldap_cache_auth, INT_UNSET, INT_UNSET);
    set_cfg_int_if_n_set(result, b, psldap_ldap_version, INT_UNSET, INT_UNSET);

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
    set_cfg_str_if_n_set(p, result, b, psldap_cookiedomain);
    set_cfg_str_if_n_set(p, result, b, psldap_auth_filter);
    set_cfg_int_if_n_set(result, b, psldap_auth_enabled, INT_UNSET, 1);
    set_cfg_int_if_n_set(result, b, psldap_authz_enabled, INT_UNSET, 1);
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
    /* Auth does not use cache by default */
    set_cfg_int_if_n_set(result, b, psldap_cache_auth, INT_UNSET, 0);
    /*  */
    set_cfg_int_if_n_set(result, b, psldap_ldap_version, INT_UNSET, 2);
    /* Set the URI for the form to capture credentials */
    set_cfg_str_if_n_set(p, result, b, psldap_credential_uri);

    return result;
}

void *merge_ldap_auth_srv_config (pool *p, void *base_conf, void *new_conf)
{
    psldap_server_rec *result = create_ldap_auth_srv_config(p, NULL);
    psldap_server_rec *b = (psldap_server_rec *)base_conf;
    psldap_server_rec *n = (psldap_server_rec *)new_conf;

    *result = *n;

    set_cfg_str_if_n_set(p, result, n, psldap_sm_file);

    if (NULL == b) return result;
    
    set_cfg_str_if_n_set(p, result, b, psldap_sm_file);

    set_cfg_int_if_n_set(result, b, psldap_sm_size, INT_UNSET,
                         PSLDAP_SHM_SIZE);
    set_cfg_int_if_n_set(result, b, psldap_max_records, INT_UNSET,
                         MAX_RECORDS);
    set_cfg_int_if_n_set(result, b, psldap_purge_interval, INT_UNSET,
                         MAX_INACTIVE_TIME);

    return result;
}

static const char* set_ldap_search_scope(cmd_parms *parms, void *mconfig,
                                         const char *to) {
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

static const char* set_connect_version_int_value(cmd_parms *parms,
						 void *mconfig,
						 const char *to) {
    psldap_config_rec *lac = (psldap_config_rec*)mconfig;
    if(NULL != to)
    {
        lac->psldap_ldap_version = atoi(to);
    }
    else
    {
        lac->psldap_ldap_version = 2;
    }
    return NULL;
}

static const char* set_ldap_slot(cmd_parms *parms, void *mconfig,
                                 const char *to) {
    psldap_config_rec *lac = (psldap_config_rec*)mconfig;
    if((NULL != to) && (0 == strcasecmp("krbv4", to)) )
    {
        lac->psldap_bindmethod = LDAP_AUTH_KRBV4;
    }
    else if((NULL != to) && (0 == strcasecmp("krbv41", to)) )
    {
        lac->psldap_bindmethod = LDAP_AUTH_KRBV41;
    }
    else if((NULL != to) && (0 == strcasecmp("krbv42", to)) )
    {
        lac->psldap_bindmethod = LDAP_AUTH_KRBV42;
    }
    else if((NULL != to) && (0 == strcasecmp("sasl", to)) )
    {
        lac->psldap_bindmethod = LDAP_AUTH_SASL;
    }
    else if((NULL != to) && (0 == strcasecmp("none", to)) )
    {
        lac->psldap_bindmethod = LDAP_AUTH_NONE;
    }
    else
    {
        lac->psldap_bindmethod = LDAP_AUTH_SIMPLE;
    }
    return NULL;
}

command_rec ldap_auth_cmds[] = {
    { "PsLDAPEnableAuth", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_auth_enabled),
      OR_AUTHCFG, FLAG, 
      "Flag to enable / disable authentication. Default value is 'on'"
    },
    { "PsLDAPEnableAuthz", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_authz_enabled),
      OR_AUTHCFG, FLAG, 
      "Flag to enable / disable authorization. Default value is 'on'"
    },
    { "PsLDAPHosts", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_hosts),
      OR_AUTHCFG, TAKE1, 
      "List of LDAP hosts which should be queried"
    },
    { "PsLDAPBindDN", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_binddn),
      OR_AUTHCFG, TAKE1, 
      "DN used to bind to the LDAP directory, if binding with provided"
      " credentials is not desired. This value is also used to initially bind"
      " to acquire the DN of the authenticating user. If this is unset, the"
      " value for PsLDAPBindMethod is forced to 'none' "
    },
    { "PsLDAPBindPassword", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_bindpassword),
      OR_AUTHCFG, TAKE1, 
      "The password corresponding to PsLDAPBindDN"
    },
    { "PsLDAPBaseDN", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_basedn),
      OR_AUTHCFG, TAKE1, 
      "The DN in the LDAP directory which contains the per-user subnodes"
    },
    { "PsLDAPUserKey", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_userkey),
      OR_AUTHCFG, TAKE1, 
      "The key in the directory whose value contains the username provided"
      " with the authentication credentials"
    },
    { "PsLDAPPassKey", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_passkey),
      OR_AUTHCFG, TAKE1, 
      "The key in the directory whose value contains the password provided"
      " with the authentication credentials"
    },
    { "PsLDAPGroupKey", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_groupkey),
      OR_AUTHCFG, TAKE1, 
      "The key in the directory whose value contains the groups in which the"
      " user maintains membership"
    },
    { "PsLDAPAuthFilter", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_auth_filter),
      OR_AUTHCFG, TAKE1, 
      "Additional LDAP filters to be applied when identifying the user for"
      " authentication."
    },
    { "PsLDAPUserGroupAttr", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_user_grp_attr),
      OR_AUTHCFG, TAKE1, 
      "The LDAP schema attribute of the user which is used to identify the"
      " user as a group member. Default value is 'dn'."
    },
    { "PsLDAPGroupMemberAttr", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_grp_mbr_attr),
      OR_AUTHCFG, TAKE1, 
      "The LDAP schema attribute of the group object used to identify each"
      " user in the LDAP group. Default value is 'uniqueMember'."
    },
    { "PsLDAPGroupNameAttr", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_grp_nm_attr),
      OR_AUTHCFG, TAKE1, 
      "The LDAP schema attribute of the group object used to uniquely"
      " identify the group. Default value is 'cn'."
    },
    { "PsLDAPSearchScope", (cmd_func)set_ldap_search_scope,
      (void*)XtOffsetOf(psldap_config_rec, psldap_searchscope),
      OR_AUTHCFG, TAKE1, 
      "Set Scope when searching in LDAP. Can be 'base', 'onelevel', or"
      " 'subtree'"
    },
    { "PsLDAPAuthoritative", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_authoritative),
      OR_AUTHCFG, FLAG, 
      "Set to 'off' to allow control to be passed on, if the user is unknown"
      " to this module"
    },
    { "PsLDAPUseLDAPGroups", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_use_ldap_groups),
      OR_AUTHCFG, FLAG, 
      "Set to 'on' to lookup the user's group using LDAP groups rather than"
      " using an LDAP user record's attribute to identify the group directly."
      " Default value is 'off'."
    },

    /* Authentication methods */
    { "PsLDAPAuthSimple", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_authsimple),
      OR_AUTHCFG, FLAG, 
      "Set to 'on' if authentication is to be performed by acquiring an"
      " attribute from the LDAP server with the configured credentials."
    },
    { "PsLDAPAuthExternal", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_authexternal),
      OR_AUTHCFG, FLAG, 
      "Set to 'on' if authentication is to be performed by binding with the"
      " user provided credentials"
    },
    { "PsLDAPAuthUseCache", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_cache_auth),
      OR_AUTHCFG, FLAG, 
      "Set to 'on' if authentication will check the cache prior to querying "
      " the LDAP server"
    },
    /* Connection security */
    { "PsLDAPBindMethod", (cmd_func)set_ldap_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_bindmethod),
      OR_AUTHCFG, TAKE1, 
      "Set to 'none', 'simple', 'sasl', 'krbv41', or 'krbv42' to determine"
      "    binding to server"
    },
    { "PsLDAPSecureAuthCookie", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_secure_auth_cookie),
      OR_AUTHCFG, FLAG, 
      "Set to 'off' if cookies are allowed to be sent across an unsecure"
      "    connection"
    },
    { "PsLDAPAuthCookieDomain", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_cookiedomain),
      OR_AUTHCFG, TAKE1, 
      "Set to a domain string if cookies are allowed to be used across "
      "    servers in a domain"
    },
    /* Password management */
    { "PsLDAPCryptPasswords", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_cryptpasswords),
      OR_AUTHCFG, FLAG, 
      "Set to 'on' if the LDAP server maintains crypted password strings"
    },
    { "PsLDAPSchemePrefix", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_schemeprefix),
      OR_AUTHCFG, FLAG, 
      "Set to 'on' if the LDAP server maintains scheme-prefixed password"
      " strings as described in rfc2307"
    },
    { "PsLDAPCredentialForm", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_credential_uri),
      OR_AUTHCFG, TAKE1, 
      "The URI containing the form to capture the user's credentials."
    },
    /* Cache management */
    { "PsLDAPCacheFile", (cmd_func)ap_set_string_slot,
      (void*)XtOffsetOf(psldap_server_rec, psldap_sm_file),
      OR_AUTHCFG, TAKE1, 
      "The full path name for the file to use to manage shared memory. Default"
      " value is " PSLDAP_SHARED_MEM_FILE
    },
#if 0
    /* TODO - How do we set the int values ... ? */
#endif
    { "PsLDAPCacheAllocation", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_server_rec, psldap_sm_size),
      OR_AUTHCFG, TAKE1, 
      "The file size to allocate for shared memory. The default size is "
      PSLDAP_SHM_SIZE_STR " bytes."
    },
    { "PsLDAPCacheMaxSize", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_server_rec, psldap_max_records),
      OR_AUTHCFG, TAKE1, 
      "The maximum number of records to hold in the cache. Default"
      " value is " MAX_RECORDS_STR
    },
    { "PsLDAPCachePurgeInterval", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_server_rec, psldap_purge_interval),
      OR_AUTHCFG, TAKE1, 
      "The maximum time in seconds to hold a record in the cache. Default"
      " value is " MAX_INACTIVE_TIME_STR
    },
    { "PsLDAPConnectVersion", (cmd_func)set_connect_version_int_value,
      (void*)XtOffsetOf(psldap_config_rec, psldap_ldap_version),
      OR_AUTHCFG, TAKE1, 
      "The connection version for the ldap server. Default value is 2"
    },
    { NULL }
};

static char * get_user_name(request_rec *r)
{
#ifdef APACHE_V2
    return r->user;
#else
    return r->connection->user;
#endif
}

static char * get_user_dn(request_rec *r, LDAP **ldap, const char *user,
                          const char *pass, psldap_config_rec *conf)
{
    int err_code;
    LDAPMessage *ld_result = NULL, *ld_entry = NULL;
    const char *ldap_attrs[2] = {LDAP_NO_ATTRS, NULL};
    char *ldap_query = NULL, *ldap_base = NULL, *user_dn = NULL;
    
    /* If the user dn is set as the user name, parse out the value and
       return it ... */
    if (0 == strncmp("dn=", user, 3)) {
        user_dn = ap_pstrdup(r->pool, &user[3]);
        goto AbortDNAcquisition;
    }

    if(LDAP_SUCCESS != (err_code = ldap_bind_s(*ldap,
					       (conf->psldap_binddn == NULL) ?
					       "" : conf->psldap_binddn,
					       (conf->psldap_bindpassword == NULL) ?
                                               "" : conf->psldap_bindpassword,
					       conf->psldap_bindmethod) )
       )
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "ldap_bind as user <%s> to get username for <%s> failed:"
		     " %s", (NULL != conf->psldap_binddn) ?
		     conf->psldap_binddn : "N/A", user,
                     ldap_err2string(err_code));
        /* Don't abort - try to get the username anyway if anonymous access is
	   available ***
	   goto AbortDNAcquisition;
	 */
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
       (err_code = ldap_search_s(*ldap, ldap_base, conf->psldap_searchscope,
                                 ldap_query, (char**)ldap_attrs, 0, &ld_result))
       )
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "ldap_search failed: %s", ldap_err2string(err_code));
        goto CleanAbortDNAcquisition;
    }
    
    if(NULL == (ld_entry = ldap_first_entry(*ldap, ld_result)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "attr <%s> for user <%s> not found in <%s>",
                     ldap_attrs[0], ldap_query, ldap_base);
        goto CleanAbortDNAcquisition;
    }

    user_dn = ap_pstrdup(r->pool, ldap_get_dn(*ldap, ld_entry) );

 CleanAbortDNAcquisition:
    if (NULL != ld_result)
    {
        ldap_msgfree(ld_result);
    }
    ldap_unbind_s(*ldap);
    *ldap = ps_ldap_init(conf, conf->psldap_hosts, LDAP_PORT);

 AbortDNAcquisition:
    return user_dn;
}

static int set_bind_params(request_rec *r, LDAP **ldap,
                           psldap_config_rec *conf,
                           const char **user, char const **password)
{
    int result = TRUE;

    if (conf->psldap_authexternal)
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
            *user = get_user_dn(r, ldap, *user, *password, conf);
        }
    } else if(conf->psldap_authsimple) {
        *user = conf->psldap_binddn;
        *password = conf->psldap_bindpassword;
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
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
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

    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server, "ldap_base = <%s>",
                 result);

    return result;
}

#define PSLDAP_FILE_MAGIC "<FILE>"
#define PSLDAP_FILE_MAGIC_SZ  sizeof(psldap_fmagic)

typedef struct psldap_fmagic_struct {
    char mtype[7];
    size_t msize;
} psldap_fmagic;

static int is_psldap_magic_string(const char *magic)
{
    return ((NULL != magic) &&
            (0 == strncmp(magic, PSLDAP_FILE_MAGIC,
                          strlen(PSLDAP_FILE_MAGIC))));
}

static char* build_psldap_magic_string(request_rec *r, const char *val,
                                       size_t len)
{
    char *result = ap_pcalloc(r->pool, PSLDAP_FILE_MAGIC_SZ + len);
    psldap_fmagic *fmresult = (psldap_fmagic*)result;
    
    
    sprintf(fmresult->mtype, "%s", PSLDAP_FILE_MAGIC);
    fmresult->msize = len;
    memcpy(result + PSLDAP_FILE_MAGIC_SZ, val, len);
    ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                 " ... built magic string of length %d (%d)",
                 len + PSLDAP_FILE_MAGIC_SZ, fmresult->msize);
    return result;
}
  
/** Determines if the passed char array is binary by iterating through the
 *  characters and checking to see if they are printable.
 *  @param str
 *  @param len
 *  @return 0 if the array is printable, non-zero if the array is not
 *          printable.
 **/
static int isCharArrayBinary(request_rec *r, const char *str, size_t len)
{
    int j, result = 0;
    //result = (str[len - 1] != '\0');
    for (j = 0; (!result) && (j < (len -1)); j++) {
        result = (!ap_isprint(str[j]) && !ap_isspace(str[j]) );
    }
    if (result && (j==(len-1))) result = (str[j] != '\0');
    
    if (result) {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     " unprintable %d of %d char <%d:%c> in str",
                     j, len, str[j], str[j]);
    }
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
static char * build_string_list(request_rec *r, struct berval * const *values,
                                const char *separator)
{
    register int i, j;
    char *result = NULL;
    int useMagic = 0;
    
    for(i = 0; (NULL != values[i]) && !useMagic; i++)
    {
        useMagic = isCharArrayBinary(r, values[i]->bv_val, values[i]->bv_len);
    }

    for(i = 0; NULL != values[i]; i++)
    {
        char *val = values[i]->bv_val;
        if (!useMagic) {
            char *valBuffer = ap_pcalloc(r->pool, values[i]->bv_len + 1);
            memcpy(valBuffer, val, values[i]->bv_len);
            valBuffer[values[i]->bv_len] = '\0';
            result = (NULL == result) ? valBuffer :
              ap_pstrcat(r->pool, result, separator, valBuffer, NULL);
        } else {
            /* base64decode the value before passing it to this function */
            /*
            size_t length = ap_base64decode_len(val);
            char *decodedValue = ap_pcalloc(r->pool, length);
            ap_base64decode_binary(decodedValue, val);
            val = build_psldap_magic_string(r, decodedValue, length);
            */
            val = build_psldap_magic_string(r, val, values[i]->bv_len);
            if (NULL != result) {
                result = val;
            } else {
                result = val;
            }
        }          
    }
    return result;
}

static LDAPMessage* get_ldrecords(
    request_rec *r, psldap_config_rec *conf, LDAP *ldap,
    char *ldap_base, const char *user, int scopeOverride)
{
    LDAPMessage *result = NULL;
    char *ldap_query = construct_ldap_query(r, conf, "objectclass", "*", user);
    const char *ldap_attrs[2] = {LDAP_ALL_USER_ATTRIBUTES, NULL};
    int  searchscope, err_code;

    searchscope = (INT_UNSET != scopeOverride) ? scopeOverride : 
        conf->psldap_searchscope;
    if(LDAP_SUCCESS !=
       (err_code = ldap_search_s(ldap, ldap_base, searchscope,
                                 ldap_query, (char**)ldap_attrs, 0, &result))
       )
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "ldap_search - %s | %d | %s- failed: %s",
                     ldap_base, searchscope, ldap_query,
                     ldap_err2string(err_code));
    }
    else if(0 >= ldap_count_entries(ldap, result))
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "user <%s> not found", user);
        if (NULL != result) {
            ldap_msgfree(result);
            result = NULL;
        }
    }

    return result;
}

static char * get_ldvalues_from_record(request_rec *r, psldap_config_rec *conf,
                                       LDAP *ldap, LDAPMessage *record,
                                       const char *attr, const char *separator)
{
    LDAPMessage *ld_entry = NULL;
    struct berval **ld_values = NULL;
    char *result = NULL;

    if((NULL == record) ||
       (NULL == (ld_entry = ldap_first_entry(ldap, record))) )
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "first entry in ldap result not acquired: %d",
                     get_lderrno(ldap) );
    }
    else if(NULL == (ld_values = ldap_get_values_len(ldap, ld_entry, attr)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "get_ldvalues_from_record <%s | %s> failed",
                     attr, ldap_err2string(get_lderrno(ldap)));
        set_lderrno(ldap, LDAP_SUCCESS);
    }
    else
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                     r->server,
                     "Building string list for attr %s", attr);
        result = build_string_list(r, ld_values, separator);
        ldap_value_free_len(ld_values);
    }

    return result;
}

static char * get_ldvalues_from_connection(request_rec *r, psldap_config_rec *conf, LDAP *ldap,
                                           char *ldap_base, char *ldap_query, const char *user,
                                           const char *attr, const char *separator,
                                           int scopeOverride)
{
    /* Set the attribute list to return to include only the requested value.
       This is done to avoid false errors caused when querying more secure
       LDAP servers that protect information within the records.
    */
    LDAPMessage *ld_result = NULL, *ld_entry = NULL;
    const char *ldap_attrs[2] = {LDAP_NO_ATTRS, NULL};
    struct berval **ld_values = NULL;
    char *result = NULL;
    int  searchscope, err_code;

    ldap_attrs[0] = attr;
    searchscope = (INT_UNSET != scopeOverride) ? scopeOverride : 
        conf->psldap_searchscope;
    if(LDAP_SUCCESS !=
       (err_code = ldap_search_s(ldap, ldap_base, searchscope,
                                 ldap_query, (char**)ldap_attrs, 0, &ld_result))
       )
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "ldap_search - %s | %d | %s- failed: %s",
                     ldap_base, searchscope, ldap_query,
                     ldap_err2string(err_code));
    }
    else if(!(ld_entry = ldap_first_entry(ldap, ld_result)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "user <%s> not found", user);
    }
    else if(!(ld_values = ldap_get_values_len(ldap, ld_entry, attr)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "ldap_get_values <%s | %d | %s | %s> failed",
                     ldap_base, searchscope, ldap_query, attr);
    }
    else
    {
        result = build_string_list(r, ld_values, separator);
    }

    if (NULL != ld_values) { ldap_value_free_len(ld_values); }
    if (NULL != ld_result) { ldap_msgfree(ld_result);    }

    return result;
}

static LDAP* ps_bind_ldap(request_rec *r, LDAP **ldap,
                          const char *user, const char *pass,
                          psldap_config_rec *conf
                          )
{
    const char *bindas = user, *bindpass = pass;
    int freeLdap = 0;
    
    if (NULL == *ldap) freeLdap = 1;

    /* ldap_open is deprecated in future releases, ldap_init is recommended */
    if (NULL == (*ldap = ps_ldap_init(conf, conf->psldap_hosts, LDAP_PORT)))
    { 
        ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, r->server,
                     "ldap_init failed <%s>", conf->psldap_hosts);
        return *ldap;
    }
    
    if(set_bind_params(r, ldap, conf, &bindas, &bindpass))
    {
        int err_code;
        if(LDAP_SUCCESS != (err_code = ldap_bind_s(*ldap,
						   (NULL == bindas) ? "" :
						   bindas,
						   (NULL == bindpass) ? "" :
						   bindpass,
						   conf->psldap_bindmethod)))
	{
            ap_log_error(APLOG_MARK, APLOG_WARNING DEBUG_ERRNO, r->server,
                         "ldap_bind as user <%s> failed: %s",
			 (NULL != bindas) ? bindas : "N/A",
			 ldap_err2string(err_code));
            if(freeLdap) ldap_unbind_s(*ldap);
            *ldap = NULL;
        }
        else
        {
            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO, r->server,
                         "ldap_bind as user <%s> succeeded", bindas);
        }
    }
    return *ldap;
}

static char * get_ldap_val_bound(request_rec *r, LDAP *ldap,
                                 psldap_config_rec *conf,  const char *user,
                                 const char *query_by, const char *query_for,
                                 const char *attr, const char *separator,
				 const char *otherParams, const char paramOp) {
    char *retval = NULL;

    if(NULL == attr) retval = "bind";
    else
    {
        char *ldap_query = NULL, *ldap_base = NULL;
        
        ldap_query = construct_ldap_query(r, conf, query_by, query_for,
                                          user);
        if (NULL != otherParams) {
	    char opStr[4] = "(&(";
	    if ('\0' != paramOp) { opStr[1] = paramOp; }
	    ldap_query = ap_pstrcat(r->pool, opStr, ldap_query,")(",
				    otherParams, "))", NULL);
	}
        ldap_base = construct_ldap_base(r, conf, ldap_query);
        
        retval = get_ldvalues_from_connection(r, conf, ldap, ldap_base,
                                              ldap_query, user, attr,
                                              separator, INT_UNSET);
    }

    return retval; 
}

static char * get_ldap_val(request_rec *r, const char *user, const char *pass,
                           psldap_config_rec *conf,
                           const char *query_by, const char *query_for,
                           const char *attr, const char *separator,
			   const char *otherParams, const char paramOp) {
    const char *bindas = user, *bindpass = pass;
    char *retval = NULL;
    LDAP *ldap = NULL;
    
    if(NULL == attr) retval = "bind";
    if((NULL != attr) && (NULL != ps_bind_ldap(r, &ldap, user, pass, conf)) ) {
        retval = get_ldap_val_bound(r, ldap, conf, user, query_by,
                                    query_for, attr, separator, NULL, '\0');
        ldap_unbind_s(ldap);
    }
    
    return retval; 
}

static char * get_groups_containing_grouped_attr(request_rec *r, LDAP *aldap,
                                                 const char *user,
                                                 const char *pass,
                                                 const char *delim,
                                                 psldap_config_rec *conf)
{
    char *retval = NULL;
    const char *groupkey;
    LDAP *ldap = aldap;

    /* ldap_open is deprecated in future releases, ldap_init is recommended */
    if((NULL == ldap) &&
       (NULL == (ldap = ps_ldap_init(conf, conf->psldap_hosts, LDAP_PORT))) )
    { 
        ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, r->server,
                     "ldap_init failed <%s>", conf->psldap_hosts);
        return retval;
    }

    if (NULL != ps_bind_ldap(r, &ldap, user, pass, conf) ) {
        groupkey = (NULL == conf->psldap_user_grp_attr) ? NULL :
            get_ldap_val_bound(r, ldap, conf, user, NULL, NULL,
                               conf->psldap_user_grp_attr, ":", NULL, '\0');
        if (NULL != groupkey)
        {
            while(groupkey[0])
            {
                char *v = ap_getword(r->pool, &groupkey,':');
                char *groups = (NULL == conf->psldap_grp_mbr_attr) ? NULL :
                    get_ldap_val_bound(r, ldap, conf, user,
                                       conf->psldap_grp_mbr_attr,
                                       v, conf->psldap_grp_nm_attr, delim,
				       NULL, '\0');
                if(NULL != groups)
                {
                    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG
                                 DEBUG_ERRNO,
                                 r->server, "Found LDAP Groups <%s>", groups);
                    retval = (NULL == retval) ? groups :
                        ap_pstrcat(r->pool, retval, delim, groups, NULL);
                }
            }
        }
        else
        {
            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                         r->server,
                         "Could not identify user LDAP User = <%s = %s>",
                         conf->psldap_userkey, user);
        }
        if (NULL == aldap) { ldap_unbind_s(ldap); }
    }

    return retval;
}

static char * get_ldap_grp(request_rec *r, const char *user,
                           const char *pass, psldap_config_rec *conf)
{
    LDAP *ldap = NULL;
    char *result = NULL;
    int cacheResultMissing = 0;
    
    if (conf->psldap_cache_auth) {
        const psldap_cache_item *record = 
            psldap_cache_item_find(r, conf->psldap_hosts, user, NULL, pass,
                                   CACHE_KEY);
        /* Lookup groups in cache for this user */;
        if(NULL != record) result = ap_pstrdup(r->pool, record->groups);
    }
    if (NULL == result) {
        if (conf->psldap_use_ldap_groups)
        {
            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                         r->server,
                         "Finding group membership of LDAP User = <%s = %s>",
                         conf->psldap_userkey, user);
            result = get_groups_containing_grouped_attr(r, ldap, user, pass,
                                                        ",", conf);
        }
        else
        {
            result = (NULL == conf->psldap_groupkey) ? NULL :
                get_ldap_val(r, user, pass, conf, NULL, NULL,
                             conf->psldap_groupkey, ",", NULL, '\0');
        }
    }
    return result;
}

static int password_matches(const psldap_config_rec *sec, request_rec *r,
                            const char* real_pw, const char *sent_pw)
{
    char errstr[MAX_STRING_LEN] = "";
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
                            get_user_name(r) );
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
                            get_user_name(r) );
            }
        }
    }
    else if (sec->psldap_cryptpasswords)
    {
        /* Passwords are crypted */
        if(strcmp(real_pw,crypt(sent_pw,real_pw)))
        {  
            ap_snprintf(errstr, sizeof(errstr), 
                        "user %s: password mismatch", get_user_name(r) );
        }
    }
    else
    {
        /* We have clear text passwords ... */
        if(strcmp(real_pw,sent_pw))
        {  
            ap_snprintf(errstr, sizeof(errstr), 
                        "user %s: password mismatch", get_user_name(r) );
        }
    }

    if ( !(result = ('\0' == errstr[0])) )
    {
        ap_log_reason (errstr, r->uri, r);
    }

    return result;
}

static int authenticate_via_bind (request_rec *r, psldap_config_rec *sec,
                                  const char *user, const char *sent_pw)
{
    char *auth_filter = sec->psldap_auth_filter;
    char filter_logical_op = ((NULL != auth_filter) &&
			      ('\0' != auth_filter[0])) ? '&' : '\0';

    /* Get the userkey to avoid any security issues regarding password
       protection on the server. You can pretty much guarantee that a user
       will be able to read their own userkey
    */
    if(NULL != get_ldap_val(r, user, sent_pw, sec, NULL, NULL,
                            sec->psldap_userkey, ",",
			    auth_filter, filter_logical_op) )
    {
        return OK;
    }
    return HTTP_UNAUTHORIZED;
}

static int authenticate_via_query (request_rec *r, psldap_config_rec *sec,
                                   const char *user, const char *sent_pw)
{
    /* This implementation assumes the password is not under access
       restrictions. This is not necessarily a good assumption.
    */
    char *real_pw;
    char *auth_filter = sec->psldap_auth_filter;
    char filter_logical_op = ((NULL != auth_filter) &&
			      ('\0' != auth_filter[0])) ? '&' : '\0';

    if(NULL != (real_pw = get_ldap_val(r, user, sent_pw, sec, NULL,
                                       NULL, sec->psldap_passkey, ",",
				       auth_filter, filter_logical_op)))
    {
        if(password_matches(sec, r, real_pw, sent_pw))
        {
            return OK;
        }
    }    
    return HTTP_UNAUTHORIZED;
}

static int translate_handler(request_rec *r)
{
    return DECLINED;
}

static size_t get_psldap_file_magic_fragment_size(request_rec *r,
                                                  const char *magic)
{
    size_t result = 0;
    if (is_psldap_magic_string(magic) ) {
        psldap_fmagic *fm = (psldap_fmagic*)magic;
        result = fm->msize;
        result = PSLDAP_FILE_MAGIC_SZ + result;
    }
    return result;
}    

static size_t get_psldap_file_magic_buffer_size(request_rec *r,
                                                const char *magic)
{
    size_t result = 0;
    if (is_psldap_magic_string(magic) ) {
        size_t fragSz = 0;
        do {
            if (';' == magic[result]) result++;
            fragSz = get_psldap_file_magic_fragment_size(r, magic + result);
            result += fragSz;
        } while((';' != magic[result]) && ('\0' != magic[result]) &&
                (0 != fragSz) );
    }
    return result;
}

/** Finds an instance of the needle string literal within the binary data
 *  in the haystack.
 **/
static const char* psldap_findmatch(char *haystack, const char *needle,
                                    int strawCount)
{
    const char *result = haystack;
    int needleSz = (NULL == needle) ? 0 : strlen(needle);
    int needlePatternSz = 0;
    
    while (needle[++needlePatternSz] == needle[0]);
    if (0 < needleSz) {
        while ((NULL != result) &&
               (0 != memcmp(result, needle, needleSz) ) ) {
            result += 1;
            while ((NULL != result) &&
                   (0 != memcmp(result, needle, needlePatternSz) ) ) {
                result += needlePatternSz;
                result = memchr(result, needle[0],
                                (result - haystack) + strawCount);
            }
        }
    }
    return result;
}

static int isLdapField(const char *fieldName)
{
    return ((0 != strcmp(FORM_ACTION, fieldName)) );
     /* &&
            (0 != strcmp(BINARY_DATA, fieldName)) &&
            (0 != strcmp(BINARY_TYPE, fieldName)) &&
            (0 != strcmp(BINARY_REFS, fieldName)) );
     */
}

/** Parses the key/value pairs out of a multipart/form-data response from the
 *  client as passed through the data pointer, which is a raw read of the
 *  client response. The data string is altered as part of the parse
 *  operation, which populates the parsed key value pairs into the passed
 *  table.
 *  @param r a pointer to the request_rec structure instance for this request
 *  @param data a reference to a string pointer, the reference and the string
 *              pointer must be a non-NULL value
 *  @param tab a reference to the table pointer to be populated with the
 *             key value pairs parsed from the data string
 **/
static int parse_multipart_data(request_rec *r, const char *boundary,
                                char **theData, table **tab)
{
    /* This routine will NOT handle binary files - strstr will choke.
       Recommend the use of memcmp / memchr and construction of an algorithm
       to more efficiently identify the termination of the MIME section.
       Refer to RFCs 1341, 1521, & 1806 for MIME background */
    const char *tmp;
    char *val, *key, *filename;
    const char *data = *theData;
    int rc = OK;
    int boundary_len = strlen(boundary);

    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_WARNING DEBUG_ERRNO,
                 r->server,
                 "Form content-type %s read (%d bytes) <experimental>",
                 r->content_type, r->clength);

    /* return DECLINED; */
    for (data = strstr(data, boundary),
             tmp = data = strstr(data + boundary_len, "Content-");
         NULL != data;
         tmp = ((NULL == data) ? NULL : (data + boundary_len)),
           data = ((NULL == tmp) ? NULL : strstr(tmp, "\r\n\r\n")) )
    {
        size_t val_len = 0;
        filename = NULL;
        /* Key is quoted string in name= on the Content-Disposition: line */
        key = strstr(tmp, "; name=\"") + 8;
        val = strstr(key, "\"");
        val[0] = '\0';

        /* Value is string after double crlf */
        val = strstr(val + 1, "\r\n\r\n") + 4;
        if (NULL == (data = strstr(val, boundary)) ) {
            data = psldap_findmatch(val, boundary,
                                    r->clength - (val - *theData) );
            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                         r->server, "... data size of value for key %s = %d",
                         key, data - val );
        }
        /* The length of the value falls 2 chars before the next boundary,
           account for the CR LF */
        val_len =  data - val - 4;
        val[val_len] = '\0';

        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                     r->server, "... parsing key %s - checking for file name",
                     key);
        filename = strstr(key + strlen(key) + 1, "filename=\"");
        if (NULL != filename) {
            char *quote;
            filename += 10;
            quote = strchr(filename, '\"');
            if((NULL != quote) && (quote != filename)) quote[0] = '\0';
            else filename = NULL;
            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                         r->server, "... key %s file name is %s",
                         key, (NULL != filename) ? filename : "<null>" );
        } else {
            int i = 0;
            /*char *ptr = key + strlen(key) + 1, errBuffer[1024];*/
            char *ptr = (char*)tmp, errBuffer[1024];
            memset(errBuffer, '\0', sizeof(errBuffer));
            while ((i < sizeof(errBuffer)) &&
                   (ap_isprint(ptr[i]) || ap_iscntrl(ptr[i]) ) ) {
                errBuffer[i] = ptr[i];
                i++;
            }
            if (i == sizeof(errBuffer)) i--;
            errBuffer[i] = '\0';
            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                         r->server, "... key %s file name not found in %s",
                         key, errBuffer);
        }

        if ( isLdapField(key) ) {
            key = ap_pstrcat(r->pool, LDAP_KEY_PREFIX,
                             ap_getword_nc(r->pool, &key, '-'),
                             NULL);
        }

        if (NULL != filename) {
            val = build_psldap_magic_string(r, val, val_len + 1);
        }

        if (NULL != (tmp = ap_table_get(*tab, key)) )
        {
            if (NULL != filename) {
                size_t magicSz = get_psldap_file_magic_buffer_size(r, tmp);
                char *mergedVal = ap_pcalloc(r->pool, val_len + magicSz + 2);
                memcpy(mergedVal, tmp, magicSz);
                mergedVal[magicSz] = ';';
                memcpy(mergedVal + magicSz + 1, val, val_len + 1);
                ap_table_setn(*tab, key, mergedVal);
            } else {
                ap_table_setn(*tab, key,
                              ap_pstrcat(r->pool, tmp, ";" , val, NULL));
            }
        } else {
            if (!is_psldap_magic_string(val)) {
                ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                            r->server, "  ...parsed key/value: %s/%s",
                            key, val);
            } else {
                ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                             r->server, "  ...parsed key/value: %s/file:%s",
                             key, filename);
            }
            ap_table_addn(*tab, key, val);
        }
    }
    return rc;
}

/** Parses the key/value pairs out of a query string as passed through the
 *  data pointer. The data string is altered as part of the parse operation,
 *  which populates the parsed key value pairs into the passed table.
 *  @param r a pointer to the request_rec structure instance for this request
 *  @param data a reference to a string pointer, the reference and the string
 *              pointer must be a non-NULL value
 *  @param tab a reference to the table pointer to be populated with the
 *             key value pairs parsed from the data string
 **/
static void parse_client_data(request_rec *r, char **data, table **tab)
{
    const char *tmp;
    char *val, *key;
    while (('\0' != *((char*)*data)) &&
           (val = ap_getword_nc(r->pool, data, '&')))
    {
        char *vptr = ap_getword_nc(r->pool, &val, '=');
        if (!isLdapField(vptr) ) key = vptr;
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
        } else {
            ap_table_add(*tab, key, ap_pstrdup(r->pool,val));
        }
    }
}

static int util_read(request_rec *r, char **buf)
{
    int rc = !OK;
    if((rc = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR))!= OK)
    {
        return rc;
    }

    if (ap_should_client_block(r)) {
        int rsize, len_read = 0, rpos=0;
        long length = (r->remaining > r->clength) ? r->remaining : r->clength;
        char *argsbuf = (char*)*buf = ap_pcalloc(r->pool, length + 2);

        ap_hard_timeout("util_read", r);
        while ((length > rpos) &&
               (len_read = ap_get_client_block(r, argsbuf, length - rpos)) > 0)
        {
            argsbuf[len_read] = '\0';
            rpos += len_read;
            argsbuf = (char*)*buf + rpos;
            ap_reset_timeout(r);
        }
        if(rpos > length) {
            ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, r->server,
                          "Buffer overflow reading page response %s",
                          r->unparsed_uri);
        } else {
            if (r->clength != len_read) r->clength = rpos;
            rc = OK;
        }
        argsbuf[len_read] = '\0';
        ap_kill_timeout(r);
    } else {
        *buf = ap_pstrdup(r->pool, "");
        rc = !OK;
    }
    return rc;
}

static int read_get(request_rec *r, table **tab)
{
    char *data;
    int rc = OK;

    if (r->method_number != M_GET)
    {
        return rc;
    }

    /* Ensure values are available for later processing */
    if (NULL == (*tab = r->subprocess_env) )
    {
        *tab = r->subprocess_env = ap_make_table(r->pool, 8);
    }
    if ( (NULL == ap_table_get(*tab, FORM_ACTION)) &&
         (NULL != r->parsed_uri.query) )
    {
        data = ap_pstrdup(r->pool, r->parsed_uri.query);
        parse_client_data(r, &data, tab);
        if (NULL == ap_table_get(*tab, FORM_ACTION))
        {
            ap_clear_table(*tab);
        }
    }

    return rc;
}

static int read_post(request_rec *r, table **tab)
{
    const char *tmp;
    char *data;
    int rc = OK;

    if (r->method_number != M_POST)
    {
        return rc;
    }

    /* Ensure values are available for later processing */
    if (NULL == (*tab = r->subprocess_env) )
    {
        *tab = r->subprocess_env = ap_make_table(r->pool, 8);
    }
    else if (NULL != ap_table_get(*tab, FORM_ACTION)) return rc;

    tmp = ap_table_get(r->headers_in, "Content-Type");
    if (0 != strstr(tmp, "multipart/form-data") ) 
    {
        char *boundary = ap_pstrdup(r->pool, strstr(tmp, "boundary=") + 9);
        r->content_type = ap_pstrdup(r->pool, "multipart/form-data");

        if ((rc = util_read(r, &data)) != OK) {
            return rc;
        }
        rc = parse_multipart_data(r, boundary, &data, tab);
        return rc;
    } else if (0 != strcasecmp(tmp, "application/x-www-form-urlencoded")) {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_WARNING DEBUG_ERRNO, r->server,
                     "Form content-type %s cannot be read", tmp);
        return DECLINED;
    }

    if ((rc = util_read(r, &data)) != OK) return rc;
    
    parse_client_data(r, &data, tab);

    return OK;
}

const char* get_qualified_field_name(request_rec *r, const char *fieldname)
{
    const char *result = fieldname;
    if (isLdapField(fieldname))
    {
        result = ap_pstrcat(r->pool, LDAP_KEY_PREFIX, fieldname, NULL);
    }
    return result;
}

static const char *cookie_credential_param = "psldapcredentials";
static const char *cookie_field_label = "PsLDAPField";

/** Remove the previously set auth cookie from the err_headers_out struct
 *  on the passed requestor instance.
 *  @param r - the request_rec struct reference from which the 'Set-Cookie'
 *             entry should be removed from the err_headers_out table 
 **/
static void remove_psldap_auth_cookie(request_rec *r)
{
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
                 "Unsetting auth cookie - removing all Set-Cookie entries "
                 "from err_headers_out");
    ap_table_unset(r->err_headers_out, "Set-Cookie");
}

/** Set the auth cookie into the err_headers_out struct on the passed
 *  requestor instance.
 *  @param r - the request_rec struct reference from which the 'Set-Cookie'
 *             entry should be removed from the err_headers_out table 
 *  @param sec - the psldap_config_sec reference containing the configuration
 *               applied to this request
 *  @param userValue - the string value containing the user name entered by
 *                     the user
 *  @param passValue - the string value containing the password entered by
 *                    the user
 **/
static void set_psldap_auth_cookie(request_rec *r, psldap_config_rec *sec,
                                   const char *userValue,
                                   const char *passValue)
{
    char *cookie_string;
    char *cookie_value;
    int secure = sec->psldap_secure_auth_cookie;
    const char *cookiedomain = sec->psldap_cookiedomain;

    cookie_value = ap_pstrcat(r->pool, sec->psldap_userkey, "=", userValue,
                              "&", sec->psldap_passkey, "=", passValue, NULL);
    cookie_string = ap_pstrcat(r->pool, cookie_credential_param, "=",
                               ap_pbase64encode(r->pool, cookie_value),
                               "; path=/",
                               (secure) ? "; secure" : "",
                               (cookiedomain) ? "; domain=" : "",
                               (cookiedomain) ? cookiedomain : "",
                               NULL);
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
                 "Adding auth cookie Set-Cookie: %s", cookie_string);
    ap_table_add(r->err_headers_out, "Set-Cookie", cookie_string);
}

static int get_first_form_fieldvalue(request_rec *r, const char* fieldname,
                               char **sent_value)
{
    int result = 0;
    char *tmp = NULL;
    table *tab = NULL;

    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
                 "getting form value for %s from get", fieldname);
    read_get(r, &tab);
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
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
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
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
        result = get_first_form_fieldvalue(r, fieldname, sent_value);
    } else {
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
 * @return OK if the auth value was acquired, HTTP_UNAUTHORIZED if the auth
 *         value could not be acquired and psldap was authoritative, or
 *         DECLINED if the auth value could not be acquired and psldap is
 *         not authoritative.
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
                *sent_value = ap_pstrdup(r->pool, get_user_name(r) );
                return OK;
            }
            if (fieldKey == sec->psldap_passkey) {
                int res = ap_get_basic_auth_pw(r, (const char**)sent_value);
                return res;
            }
            return HTTP_UNAUTHORIZED;
        } else if (0 == strcmp("digest", authType)) {
            if (fieldKey == sec->psldap_userkey) {
                *sent_value = ap_pstrdup(r->pool, get_user_name(r) );
                return OK;
            } else if (fieldKey == sec->psldap_passkey) {
                int res = ap_get_basic_auth_pw(r, (const char**)sent_value);
                return res;
            }
            return HTTP_UNAUTHORIZED;
        } else if (0 == strcmp("form", authType)) {
            return get_first_form_fieldvalue(r, fieldKey, sent_value) ? OK:
                HTTP_UNAUTHORIZED;
        } else if (0 == strcmp("cookie", authType)) {
            return get_cookie_fieldvalue(r, fieldKey, sent_value) ? OK:
                HTTP_UNAUTHORIZED;
        }
    } else {
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

static int get_provided_credentials(request_rec *r, psldap_config_rec *sec,
                                    char **sent_pw, char **sent_user)
{
    int result;
    char *authType = (char*)ap_auth_type(r);

    if ((OK == (result = get_provided_password(r, sent_pw)) ) &&
        (!sec->psldap_auth_enabled ||
	 (OK == (result = get_provided_username(r, sent_user)) ) ) )
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
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
                 "Credentials (%s:%s) %saquired for %s via %s auth for user %s",
                 sec->psldap_userkey , sec->psldap_passkey,
                 (OK == result) ? "" : "not ", r->uri, authType, *sent_user);
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
            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
                         "Next uri after auth set from referrer %s",
                         cookie_string);
        }
    } else if ((0 == strcmp("Cookie", key)) &&
               (0 == strcmp(cookie_next_uri,
                            ap_getword_nc(r->pool, &cookie_string, '=')) ) ) {
        const char *val;
        cookie_string = ap_pbase64decode(r->pool, cookie_string);
        ap_table_setn(r->notes, cookie_next_uri, cookie_string);
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
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

    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
                 "Setting post login uri to %s", post_login_uri);
    cookie_value = ap_pstrcat(r->pool, post_login_uri, NULL);
    cookie_string = ap_pstrcat(r->pool, cookie_next_uri, "=",
                               ap_pbase64encode(r->pool, cookie_value),
                               "; path=/", NULL);
    ap_table_add(r->err_headers_out, "Set-Cookie", cookie_string);
}

static int log_table_values(void *data, const char *key,
                                const char *value)
{
    request_rec *r = (request_rec*)data;
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                 r->server,
                 "Header in kv pair: <%s> = <%s>", key, value);
    return 1;
}

/** Sends an HTML page with a META REFRESH to the browser / client to force
 *  it to perform a redirect to the specified URL.
 *  @param r the request_rec instance pointer
 *  @param sec the configuration struct instance pointer
 *  @param redirect_uri the URI to be forwarded to on this server
 *  @return 0 if successful, non-zero otherwise
 **/
static int ldap_redirect_handler(request_rec *r)
{
    psldap_config_rec *conf =
        (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                   &psldap_module);
    const char *redirect_uri = ap_table_get(r->notes, PSLDAP_REDIRECT_URI);

    r->content_type = "text/html";
    ap_table_add(r->err_headers_out, "Location", redirect_uri);

    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                 r->server,
                 "Redirecting to URI %s of type %s", redirect_uri,
                 r->content_type);
    
    return HTTP_MOVED_TEMPORARILY;
}

static int auth_form_redirect_handler(request_rec *r)
{
    psldap_config_rec *conf =
        (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                   &psldap_module);
    /* Remove any previously set cookies for authentication */
    remove_psldap_auth_cookie(r);
    ap_internal_redirect(conf->psldap_credential_uri, r);
    return OK;
}

static int ldap_authenticate_user (request_rec *r)
{
    psldap_config_rec *sec =
        (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                   &psldap_module);
    conn_rec *c = r->connection;
    char *sent_pw = NULL;
    char *sent_user = NULL;
    int res = DECLINED;
    int cacheResultMissing = 1;

    if(!sec->psldap_userkey || !sec->psldap_auth_enabled) {
        return (!sec->psldap_authoritative) ? DECLINED : HTTP_UNAUTHORIZED;
    }
    
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO, r->server,
                 "Authenticating LDAP user for <%s>", r->uri);
    if (OK != (res = get_provided_credentials (r, sec, &sent_pw, &sent_user
                                               )))
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO,
                     r->server,
                     "Failed to acquire credentials for authentication",
                     (NULL == sent_user) ? "" : sent_user);
    } else {
        const psldap_cache_item *record = NULL;
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO,
                     r->server,
                     "Authenticating user %s with passed credentials",
                     (NULL == sent_user) ? "" : sent_user);
        if (sec->psldap_cache_auth) {
            record = psldap_cache_item_find(r, sec->psldap_hosts, sent_user,
                                            NULL, sent_pw, CACHE_KEY);
        }
        if (NULL != record) {
            cacheResultMissing = 0;
ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO,
                     r->server,
                     "Authentication of user %s through cache",
                     (NULL == sent_user) ? "" : sent_user);
            res = ((0 == strcmp(sent_user, record->key)) &&
                   (0 == strcmp(sent_pw, record->passwd)) ) ?
                OK : HTTP_UNAUTHORIZED;
        } else if(sec->psldap_authexternal) {
ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO,
                     r->server,
                     "Authentication of user %s through bind",
                     (NULL == sent_user) ? "" : sent_user);
            res = authenticate_via_bind (r, sec, sent_user, sent_pw);
        } else { /* if (sec->psldap_authsimple)  or anything else */
ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO,
                     r->server,
                     "Authentication of user %s through query",
                     (NULL == sent_user) ? "" : sent_user);
            res = authenticate_via_query (r, sec, sent_user, sent_pw);
        }
    }
ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO,
                     r->server,
                     "Authentication of user %s complete",
                     (NULL == sent_user) ? "" : sent_user);

    if (res == HTTP_UNAUTHORIZED)
    {
        char *authType = (char*)ap_auth_type(r);

        if (NULL == authType) authType = "";
        authType  = ap_pstrdup(r->pool, authType);
        ap_str_tolower(authType);
        if ((0 == strcmp(authType, "cookie")) &&
            (0 < strlen(sec->psldap_credential_uri)) )
        {
            /* We should return the credential page if authtype is cookie,
               and the credential uri is set and unset the cookie */
            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO,
                         r->server,
                         "Credentials don't exist on page request <%s>,"
                         " sending form for %s auth: %s",
                         r->uri, authType, sec->psldap_credential_uri);
            r->handler = ap_pstrdup(r->pool, "ldap-auth-form");
            res = OK;
        } else if (sec->psldap_authoritative) {
            ap_log_error (APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                          "LDAP user %s not found or password invalid",
                          (NULL != sent_user) ? sent_user : "<blank>");
            ap_note_basic_auth_failure (r);
        } else {
            res = DECLINED;
        }
    } else if ((OK == res) && cacheResultMissing && (NULL != cache_mem_mgr)) {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO, r->server,
                     "Cached user credentials and info for user %s: %s",
                     sec->psldap_hosts, sent_user);
        /* Cache the user account information */
        psldap_cache_item_create(r, sec->psldap_hosts, sent_user, NULL,
                                 sent_pw, NULL);
    }
    return res;
}

/* Checking ID */
    
static int ldap_check_authz(request_rec *r)
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

    if (!sec->psldap_userkey || !sec->psldap_authz_enabled)
    {
        return (!sec->psldap_authoritative) ? DECLINED : HTTP_UNAUTHORIZED;
    }
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO, r->server,
                 "Checking LDAP user authorization");
    if (!reqs_arr || 
        (OK != get_provided_credentials (r, sec, &sent_pw, &user)) )
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
                    if (t[0] == '"') {
                        t = &t[1];
                        w = ap_getword(r->pool, &t, '"');
                    } else if (t[0] == '\'') {
                        t = &t[1];
                        w = ap_getword(r->pool, &t, '\'');
                    } else {
                        w = ap_getword(r->pool, &t, ' ');
                    }
                    groups = orig_groups;
                    /* TODO - optimize this to use string functions instead
                       of parsing out the string segments */
                    while(groups[0])
                    {
                        v = ap_getword(r->pool, &groups,',');
                        if(0 == strcmp(v,w)) return OK;
                    }
                }
                errstr = ap_pstrcat(r->pool, "user ", user, " <", orig_groups,
                                    "> not in correct group <", t, ">", NULL);
            } else {
                errstr = ap_pstrcat(r->pool, "groups for user", user,
                                    " not found in LDAP directory (key ",
                                    (NULL == sec->psldap_groupkey) ? "NULL" :
                                    sec->psldap_groupkey, ")", NULL);
            }
        }
    }

    /* If no groups were required, return OK */
    if ((NULL == sec->psldap_groupkey) || !groupRequirementExists) return OK;
    
    if (!sec->psldap_authoritative) return DECLINED;

    if (NULL != errstr) ap_log_reason (errstr, r->filename, r);
    ap_note_basic_auth_failure (r);

    return HTTP_UNAUTHORIZED;
}

static int compare_arg_strings(const void *arg1, const void *arg2)
{
    return strcmp(*((char**)arg1), *((char**)arg2) );
}

static int compare_arg_magic_binary(const void *arg1, const void *arg2)
{
    return memcmp(*((char**)arg1), *((char**)arg2), PSLDAP_FILE_MAGIC_SZ );
}

/** Parses the value string of concatenated values into an array. If the
 *  arg string begins with PSLDAP_FILE_MAGIC, then the complete content is
 *  copied as a single entry into the array_header.
 **/
static array_header* parse_arg_string(request_rec *r, const char **arg_string,
                                      const char delimiter)
{
    array_header *result = ap_make_array(r->pool, 1, sizeof(char*));
    if ((NULL != arg_string) && (NULL != *arg_string)) {
        const char *arg_ptr = *arg_string;
        if (is_psldap_magic_string(arg_ptr) ) {
            size_t magicSz = get_psldap_file_magic_buffer_size(r, arg_ptr);
            size_t fragSz = 0;
            do {
                const char **w = ap_push_array(result);
                if (';' == arg_ptr[0]) {
                    /*arg_ptr[0] = '\0';*/
                    arg_ptr += 1;
                }
                *w = arg_ptr;
                fragSz = get_psldap_file_magic_fragment_size(r, arg_ptr);
                arg_ptr += fragSz;
            } while((arg_ptr < (*arg_string + magicSz)) &&
                    (0 != fragSz) );
            qsort(result->elts, result->nelts, result->elt_size,
                  compare_arg_magic_binary);
        } else {
            while('\0' != (*arg_string)[0])
            {
                char **w = ap_push_array(result);
                *w = ap_getword(r->pool, arg_string, delimiter);
            }
            qsort(result->elts, result->nelts, result->elt_size,
                  compare_arg_strings);
        }
    }
    return result;
}

typedef struct {
    array_header *keepers;
    array_header *deletions;
    array_header *additions;
} psldap_txns;

static void* get_ldapmod_value_from_string(request_rec *r, const char *str,
                                           LDAPMod *modptr)
{
    void *result = (void*)str;
    
    if ( is_psldap_magic_string(str) ) {
        struct berval *value = ap_palloc(r->pool, sizeof(struct berval));
        size_t length = get_psldap_file_magic_fragment_size(r, str);
        modptr->mod_op |= LDAP_MOD_BVALUES;
        /* base64encode the val before sending, ldap interface only accepts
           encoded values */
        /*
        value->bv_len = ap_base64encode_len(length);
        value->bv_val = ap_palloc(r->pool, value->bv_len);
        ap_base64encode_binary(value->bv_val,
                               (char*)str + PSLDAP_FILE_MAGIC_SZ,
                               length);
        */
        value->bv_len = length;
        value->bv_val = (char*)str + PSLDAP_FILE_MAGIC_SZ;
        result = value;
    }
    
    return result;
}

static void psldap_txns_add_item_to_results(request_rec *r, array_header *array,
                                            const char *item, LDAPMod *modptr)
{
    char **value = (char**)ap_push_array(array);
    *value = get_ldapmod_value_from_string(r, item, modptr);
}

static void psldap_add_values_to_mod(request_rec *r, LDAPMod *modptr, ...)
{
    va_list ap;
    int i = 0, useBvalues = (0 != (modptr->mod_op & LDAP_MOD_BVALUES));
    array_header *arg = NULL;

    va_start(ap, modptr);
    for (arg = va_arg(ap, array_header*); NULL != arg;
         arg = va_arg(ap, array_header*)) {
        i += arg->nelts;
    }
    va_end(ap);

    if(useBvalues) {
        modptr->mod_bvalues = ap_palloc(r->pool, (i + 1) *
                                         sizeof(struct berval*));
    } else {
        modptr->mod_values = ap_palloc(r->pool, (i + 1) * sizeof(char*));
    }

    va_start(ap, modptr);
    for (arg = va_arg(ap, array_header*), i = 0; NULL != arg;
         arg = va_arg(ap, array_header*) ) {
        int j;
        for (j = 0; j < arg->nelts; j++) {
            if(useBvalues) {
                modptr->mod_bvalues[i++] = ((struct berval**)(arg->elts))[j];
            } else {
                modptr->mod_values[i++] = ((char**)(arg->elts))[j];
            }
        }
    }
    va_end(ap);

    if(useBvalues) {
        modptr->mod_bvalues[i] = NULL;
    } else {
        modptr->mod_values[i] = NULL;
    }
}

static LDAPMod* get_transactions(request_rec *r, const char* attr,
                                 array_header *old_v, array_header *new_v)
{
    psldap_txns *result = ap_palloc(r->pool, sizeof(psldap_txns));
    LDAPMod *mresult = ap_palloc(r->pool, sizeof(LDAPMod));
    int i = 0, j = 0;
    int old_v_has_values = ((NULL != old_v) && (old_v->nelts > 0));

    result->keepers = ap_make_array(r->pool, 1, sizeof(char*));
    result->deletions = ap_make_array(r->pool, 1, sizeof(char*));
    result->additions = ap_make_array(r->pool, 1, sizeof(char*));

    /* Initialize this to something - add is 0x0000 */
    mresult->mod_op = LDAP_MOD_ADD;

    ap_log_error(APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                 "Finding differences of attr: %s", attr);
    while (((NULL != old_v) && (i < old_v->nelts)) || (j < new_v->nelts)) {
        char **value;
        if (j >= new_v->nelts) {
            psldap_txns_add_item_to_results(r, result->deletions,
                                            ((char**)(old_v->elts))[i++],
                                            mresult);
        } else if ((old_v_has_values) && (i >= old_v->nelts)) {
            psldap_txns_add_item_to_results(r, result->additions,
                                            ((char**)(new_v->elts))[j++],
                                            mresult);
        } else {
            int compare = 1;
            if (old_v_has_values) {
                if (is_psldap_magic_string(((char**)(old_v->elts))[i]) ||
                    is_psldap_magic_string(((char**)(new_v->elts))[j]) ) {
                    compare = memcmp(((char**)(old_v->elts))[i],
                                     ((char**)(new_v->elts))[j],
                                     PSLDAP_FILE_MAGIC_SZ);
                } else {
                    compare = strcmp(((char**)(old_v->elts))[i],
                                     ((char**)(new_v->elts))[j]);
                }
            }
            if (compare < 0) {
                psldap_txns_add_item_to_results(r, result->deletions,
                                                ((char**)(old_v->elts))[i++],
                                                mresult);
            } else if (compare > 0) {
                psldap_txns_add_item_to_results(r, result->additions,
                                                ((char**)(new_v->elts))[j++],
                                                mresult);
            } else {
                psldap_txns_add_item_to_results(r, result->keepers,
                                                ((char**)(old_v->elts))[i++],
                                                mresult);
                j++;
            }
        }
    }

    ap_log_error(APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                 "Creating LDAPMod of attr: %s (%d:%d:%d)", attr,
                 result->keepers->nelts, result->deletions->nelts,
                 result->additions->nelts);
    mresult->mod_type = (char*)attr;
    if ((result->keepers->nelts == 0) && (result->deletions->nelts == 0) &&
        (result->additions->nelts > 0) ) {
        /* Adding new attributes */
        mresult->mod_op |= LDAP_MOD_ADD;
        psldap_add_values_to_mod(r, mresult, result->additions, NULL);
    } else if ((result->additions->nelts == 0) &&
               (result->keepers->nelts == 0) &&
               (result->deletions->nelts > 0) ) {
        /* Deleting the attribute content or replacing */
        mresult->mod_op = LDAP_MOD_DELETE;
        mresult->mod_values = NULL;
    } else if ((result->additions->nelts > 0) ||
               ((result->keepers->nelts > 0) && (result->deletions->nelts > 0)) ) {
        mresult->mod_op |= LDAP_MOD_REPLACE;
        psldap_add_values_to_mod(r, mresult, result->keepers, result->additions,
                                 NULL);
    } else {
        mresult = NULL;
    }
    
    return mresult;
}

/** Strips the leading and trailing white space in a given character string.
 *  @param str the character string to be altered
 **/
static char* strip_lt_whitespace(char *str)
{
    int i, len = (NULL != str) ? strlen(str) : 0;
    for (i = 0; ap_isspace(str[i]) && (i < len); i++) ;
    for (len; ap_isspace(str[len - 1]) && (i < len); len--) ;
    memmove(str, &str[i], (len - i + 1));
    str[len] = '\0';
    return str;
}

typedef struct {
    int error_code;
    const char *dsml_string;
} OpenLdapDsmlMapEntry;  

static const char* get_dsml_err_code_string(int err_code)
{
    static const OpenLdapDsmlMapEntry dsml_err_map[] = {
        {LDAP_SUCCESS, "success"},
        {LDAP_OPERATIONS_ERROR, "operationsError"},
        {LDAP_PROTOCOL_ERROR, "protocolError"},
        {LDAP_TIMELIMIT_EXCEEDED, "timeLimitExceeded"},
        {LDAP_SIZELIMIT_EXCEEDED, "sizeLimitExceeded"},
        {LDAP_COMPARE_FALSE, "compareElse"},
        {LDAP_COMPARE_TRUE, "compareTrue"},
        {LDAP_AUTH_METHOD_NOT_SUPPORTED, "authMethodNotSupported"},
        {LDAP_STRONG_AUTH_REQUIRED, "strongAuthRequired"},
        {LDAP_REFERRAL, "referral"},
        {LDAP_ADMINLIMIT_EXCEEDED, "adminLimitExceeded"}, /* 10 */
        {LDAP_UNAVAILABLE_CRITICAL_EXTENSION, "unavailableCriticalExtension"},
        {LDAP_CONFIDENTIALITY_REQUIRED, "confidentialityRequired"},
        {LDAP_SASL_BIND_IN_PROGRESS, "saslBindInProgress"},
        {LDAP_NO_SUCH_ATTRIBUTE, "noSuchAttribute"},
        {LDAP_UNDEFINED_TYPE, "undefinedAttributeType"},
        {LDAP_INAPPROPRIATE_MATCHING, "inappropriateMatching"},
        {LDAP_CONSTRAINT_VIOLATION, "constraintViolation"},
        {LDAP_TYPE_OR_VALUE_EXISTS, "attributeOrValueExists"},
        {LDAP_INVALID_SYNTAX, "invalidAttributeSyntax"},
        {LDAP_NO_SUCH_OBJECT, "noSuchObject"}, /*20 */
        {LDAP_ALIAS_PROBLEM, "aliasProblem"},
        {LDAP_INVALID_DN_SYNTAX, "invalidDNSyntax"},
        {LDAP_ALIAS_DEREF_PROBLEM, "aliasDereferencingProblem"},
        {LDAP_INAPPROPRIATE_AUTH, "inappropriateAuthentication"},
        {LDAP_INVALID_CREDENTIALS, "invalidCredentials"},
        {LDAP_INSUFFICIENT_ACCESS, "insufficientAccessRights"},
        {LDAP_BUSY, "busy"},
        {LDAP_UNAVAILABLE, "unavailable"},
        {LDAP_UNWILLING_TO_PERFORM, "unwillingToPerform"},
        {LDAP_LOOP_DETECT, "loopDetect"}, /* 30 */
        {LDAP_NAMING_VIOLATION, "namingViolation"},
        {LDAP_OBJECT_CLASS_VIOLATION, "objectClassViolation"},
        {LDAP_NOT_ALLOWED_ON_NONLEAF, "notAllowedOnNonLeaf"},
        {LDAP_NOT_ALLOWED_ON_RDN, "notAllowedOnRDN"},
        {LDAP_ALREADY_EXISTS, "entryAlreadyExists"},
        {LDAP_NO_OBJECT_CLASS_MODS, "objectClassModsProhibited"},
        {LDAP_AFFECTS_MULTIPLE_DSAS, "affectMultipleDSAs"},
        {LDAP_OTHER, "other"}
        };
    const char *result = dsml_err_map[38].dsml_string;
    int i;
    
    for (i = 0; i < 39; i++) {
        if(err_code == dsml_err_map[i].error_code) {
            result = dsml_err_map[i].dsml_string;
        }
    }

    return result;
}

static void write_dsml_response_fragment(request_rec *r, const char *rt,
                                         int err_code)
{
    ap_rprintf(r, "\t\t<%s>\n", rt);
    ap_rputs("\t\t\t<resultCode>\n", r);
    ap_rprintf(r, "\t\t\t\t%s\n", get_dsml_err_code_string(err_code) );
    ap_rputs("\t\t\t</resultCode>\n", r);
    ap_rputs("\t\t\t<errorMessage>\n", r);
    ap_rprintf(r, "\t\t\t\t%s\n", ldap_err2string(err_code) );
    ap_rputs("\t\t\t</errorMessage>\n", r);
    ap_rprintf(r, "\t\t</%s>\n", rt);
}

/** Iterate through the results returned by the LDAP server and write them out as
 *  a complete DSML searchResult node.
 *
 *  @param r request record from which to preform pool related operations
 *  @param ldResult the LDAPMessage instance received from the server containing
 *         the data in response to a search query.
 *  @param mimeType is a character string indicating the expected mime type of
 *         the response. Any value other than NULL causes only the content of the
 *         first entry to be printed to the client (future implementation)
 *  @result allocated string containing all the values in the array values
 *          separated by string separator
 */
static void write_dsml_search_response(request_rec *r, LDAP *ld,
                                       LDAPMessage *ldResult,
                                       const char *mimeType,
                                       int binaryAsHref)
{
    register LDAPMessage *ldEntry = NULL;
    char *attr, *dn, *tmp;
    struct berval **values;
    BerElement *ber;
    int i, bytesWritten = 0;

    if (NULL == mimeType) {
        ap_rputs("\t\t<searchResponse>\n", r);
        write_dsml_response_fragment(r, "searchResultDone", LDAP_SUCCESS);
    }

    for(ldEntry = ldap_first_entry(ld, ldResult); NULL != ldEntry;
        ldEntry = ldap_next_entry(ld, ldEntry))
    {
        if (LDAP_RES_SEARCH_ENTRY == ldap_msgtype(ldEntry)) {
            dn = ldap_get_dn(ld, ldEntry);
            if (NULL == mimeType) {
                ap_rvputs(r, "\t\t\t<searchResultEntry dn=\"",
                          ap_escape_html(r->pool, dn), "\">\n", NULL);
            }
            for (tmp = ldap_first_attribute(ld, ldEntry, &ber); NULL != tmp;
                 tmp = ldap_next_attribute(ld, ldEntry, ber)) {
                /* NOTE - use ldap_get_values_len for binary data - assumed ascii */
                attr = ap_pstrdup(r->pool, tmp);
                ldap_memfree(tmp);
                values = ldap_get_values_len(ld, ldEntry, attr);
                if ((NULL != values) &&
                    (0 <= (i = ldap_count_values_len(values) - 1)) ) {
                    if (NULL == mimeType) {
                        ap_rvputs(r, "\t\t\t\t<attr name=\"", attr, "\">\n",
                                  NULL);
                    }
                    while (i >= 0) {
                        char *sendValue = values[i]->bv_val;
                        int useMagic = isCharArrayBinary(r, values[i]->bv_val,
                                                         values[i]->bv_len);
                        if (!useMagic) {
                            char *encVal = ap_palloc(r->pool,
                                                     values[i]->bv_len + 1);
                            strncpy(encVal, values[i]->bv_val, 
                                    values[i]->bv_len);
                            encVal[values[i]->bv_len] = '\0';
                            sendValue = ap_escape_html(r->pool,
                                               strip_lt_whitespace(encVal));
                        } else if (NULL != mimeType) {
                            if (0 == bytesWritten) {
                                ap_kill_timeout(r);
                                bytesWritten = ap_send_mmap(values[i]->bv_val, r,
                                                          0, values[i]->bv_len);
                            }
                        } else if (binaryAsHref) {
                            /* TODO - compose URL to acquire binary file. How
                               do we determine the appropriate type ??? */
                            sendValue = ap_pstrcat(r->pool, "<![CDATA[",
                               r->uri,
                               "?FormAction=Search",
                               "&search=", ap_escape_uri(r->pool, "(objectClass=*)"),
                               "&dn=", ap_escape_uri(r->pool, dn),
                               "&", BINARY_DATA, "=", attr,
                               "&", BINARY_TYPE, "=", ap_escape_uri(r->pool, "image/jpeg"),
                               "]]>",
                               NULL);
                        } else {
                            int j = ap_base64encode_len(values[i]->bv_len);
                            char *encVal = ap_palloc(r->pool, j);
                            ap_base64encode_binary(encVal, sendValue,
                                                   values[i]->bv_len);
                            sendValue = encVal;
                        }
                        i--;
                        if (NULL == mimeType) {
                            ap_rvputs(r, "\t\t\t\t\t<value>", sendValue,
                                      "</value>\n", NULL);
                        }
                    }
                    if (NULL == mimeType) {
                        ap_rputs("\t\t\t\t</attr>\n", r);
                    }
                    ldap_value_free_len(values);
                }
            }
            ldap_memfree(dn);
            if (NULL != ber) ber_free(ber, 0);
            if (NULL == mimeType) {
                ap_rputs("\t\t\t</searchResultEntry>\n", r);
            }
        } else {
            ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                         "Cannot read msg type building dsml results: %x",
                         ldap_msgtype(ldEntry) );
        }
    }
    if (NULL == mimeType) {
        ap_rputs("\t\t</searchResponse>\n", r);
    }
}

/** This writes the DSML search response directly to the client. This response is not
 *  buffered by design to eliminate any overhead that may be incurred with overly large
 *  datasets.
 *  @param r request_rec pointer
 *  @param conf psldap configuration instance
 *  @param ldap LDAP pointer
 *  @param ps psldap_status pointer
 *  @param ldap_base string representation of the base of the query
 *  @param ldap_query string representation of the query filter
 *  @param attr the attribute to acquire from the server. See the man page on
 *         ldap_search_s
 *  @param mimeType is a character string indicating the expected mime type of
 *         the response. Any value other than NULL causes only the content of the
 *         first entry to be printed to the client (future implementation)
 **/
static void write_dsml_sr_to_connection(request_rec *r, psldap_config_rec *conf,
                                        LDAP *ldap, psldap_status *ps,
                                        char *ldap_base, char *ldap_query,
                                        const char *attr, const char* mimeType,
                                        int binaryAsHref)
{
    LDAPMessage *ld_result = NULL, *ld_entry = NULL;
    const char *ldap_attrs[2] = {LDAP_ALL_USER_ATTRIBUTES, NULL};
    char **ld_values = NULL;
    int  err_code;

    ap_log_error(APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                 "Executing ldap_search with base / scope / pattern: "
                 "%s / %d / %s ...",
                 ldap_base, conf->psldap_searchscope, ldap_query);
    ldap_attrs[0] = attr;
    if(LDAP_SUCCESS !=
       (err_code = ldap_search_s(ldap, ldap_base, conf->psldap_searchscope,
                                 ldap_query, (char**)ldap_attrs, 0, &ld_result))
       )
    {
        if (NULL == mimeType) {
            ap_rputs("\t\t<searchResponse>\n", r);
            write_dsml_response_fragment(r, "searchResultDone", err_code);
            ap_rputs("\t\t</searchResponse>\n", r);
            ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                         "ldap_search failed: %s", ldap_err2string(err_code));
        }
    } else {
        ap_log_error(APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                     "...ldap_search successfully executed with base / scope / pattern: "
                     "%s / %d / %s\n"
                     "Generating and sending DSML response to ldap search request...",
                     ldap_base, conf->psldap_searchscope, ldap_query);
        write_dsml_search_response(r, ldap, ld_result, mimeType, binaryAsHref);
        ap_log_error (APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                      "...DSML search response sent");
    }
    
    if (NULL != ld_result) { ldap_msgfree(ld_result); }
}

static int get_dn_attributes_from_ldap(void *data, const char *key,
                                       const char *val)
{
    psldap_status *ps = (psldap_status*)data;
    request_rec *r = ps->rr;
    LDAP *ldap = ps->ldap;
    psldap_config_rec *conf = ps->conf;
    char *result = NULL;
    char *user = NULL;
    char *ldap_base = NULL;
    char *ldap_query = ps->searchPattern;
    char *ldap_field = ps->fieldName;
    char *response_type = ps->responseType;
    char *xslUri1 = ps->xslPrimaryUri;
    char *xslUri2 = ps->xslSecondaryUri;
    int binaryHRef = ps->binaryAsHref;

    if (NULL == key) {
        if (NULL != val) strncpy((char*)val, "queryResponse", 15);
        if ((NULL != ldap_query) &&
            ( ((NULL != xslUri1) && (NULL != xslUri2) && (NULL != ps->mod_dn))
              ||
              (NULL != ps->responseType)
              )
            )
        {
            /* Use ldap_modify_s here to directly modify the entries. Possibly
               add the LDAPMod to an array passed in the data. */
            ldap_base = (NULL != ps->mod_dn) ? ps->mod_dn :
                construct_ldap_base(r, conf, conf->psldap_basedn);
            get_provided_username(r, &user);
            if (NULL == ps->responseType) {
                ap_rvputs(r,
                          "<?xml-stylesheet type=\"text/xsl\" title=\"Primary View\" href=\"",
                          xslUri1,
                          "\"?>\n",
                          "<?xml-stylesheet type=\"text/xsl\" alternate=\"yes\" title=\"Secondary View\" href=\"",
                          xslUri2,
                          "\"?>\n",
                          "<dsml>\n",
                          " <batchResponse>\n",
                          NULL);
            }
            write_dsml_sr_to_connection(r, conf, ldap, ps, ldap_base, ldap_query,
                                        ps->fieldName, ps->responseType, 
                                        ps->binaryAsHref);
            if (NULL == ps->responseType) {
                ap_rvputs(r,
                          " </batchResponse>\n",
                          "</dsml>",
                          NULL);
            }
        }
    }
    else if (0 == strncmp(LDAP_KEY_PREFIX, key, LDAP_KEY_PREFIX_LEN))
    {
        if(0 == strcmp("search", key + LDAP_KEY_PREFIX_LEN))
        {
            ldap_query = ps->searchPattern = ap_pstrdup(r->pool, val);
            ap_log_error (APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                          "LDAP query pattern set to %s",
                          ldap_query);
        } else if (0 == strcmp("xsl1", key + LDAP_KEY_PREFIX_LEN)) {
            xslUri1 = ps->xslPrimaryUri = ap_pstrdup(r->pool, val);
            ap_log_error (APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                          "LDAP query primary xsl uri set to %s",
                          xslUri1);
        } else if (0 == strcmp("xsl2", key + LDAP_KEY_PREFIX_LEN)) {
            xslUri2 = ps->xslSecondaryUri = ap_pstrdup(r->pool, val);
            ap_log_error (APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                          "LDAP query secondary xsl uri set to %s",
                          xslUri2);
        } else if (0 == strcmp("dn", key + LDAP_KEY_PREFIX_LEN)) {
            ps->mod_dn = ap_pstrdup(r->pool, val);
            ap_log_error (APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                          "LDAP query record dn set to %s",
                          ps->mod_dn);
        } else if (0 == strcmp(BINARY_DATA, key + LDAP_KEY_PREFIX_LEN)) {
            ldap_field = ps->fieldName = ap_pstrdup(r->pool, val);
            ap_log_error (APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                          "LDAP query field set to %s", ldap_field);
        } else if (0 == strcmp(BINARY_TYPE, key + LDAP_KEY_PREFIX_LEN)) {
            response_type = ps->responseType = ap_pstrdup(r->pool, val);
            ap_log_error (APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                          "LDAP response type set to %s",response_type);
        } else if (0 == strcmp(BINARY_REFS, key + LDAP_KEY_PREFIX_LEN)) {
            binaryHRef = ps->binaryAsHref = strcasecmp("off", val);
            ap_log_error (APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                          "LDAP BinaryHRef set to %d", binaryHRef);
        } else {
            ap_log_error (APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                          "Form field %s not processed to perform ldap query",
                          key);
        }
    }
    return TRUE;
}

static void rprintf_LDAPMod_instance(request_rec *r, LDAPMod *mod)
{
    int i = mod->mod_op ^ LDAP_MOD_BVALUES;
    const char *op_name;

    switch(i)
    {
    case LDAP_MOD_REPLACE:
        op_name = "LDAP_MOD_REPLACE";
        break;
    case LDAP_MOD_ADD:
        op_name = "LDAP_MOD_ADD";
        break;
    case LDAP_MOD_DELETE:
        op_name = "LDAP_MOD_DELETE";
        break;
    default:
        op_name = "Unknown Operation";
    }

    ap_log_error (APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                  ".. writing %s response to client on mod_op %d",
                  (NULL != mod->mod_type) ? mod->mod_type : "",
                  mod->mod_op);
    ap_rprintf(r, "%s (%d): Attr = %s: ", op_name, mod->mod_op,
               (NULL != mod->mod_type) ? mod->mod_type : "");
    if (0 != (mod->mod_op & LDAP_MOD_BVALUES)) {
        if (NULL != mod->mod_bvalues) {
            ap_log_error (APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                          ".. writing binary values of %s response to client",
                          (NULL != mod->mod_type) ? mod->mod_type : "");
            for (i = 0; NULL != mod->mod_bvalues[i]; i++) {
                ap_rprintf(r, " binary (%d) bytes: %d%s",
                           i, mod->mod_bvalues[i]->bv_len,
                           (NULL != mod->mod_bvalues[i+1]) ? "; " : "");
            }
        }
    } else if (NULL != mod->mod_values) {
        ap_log_error (APLOG_MARK, APLOG_DEBUG DEBUG_ERRNO, r->server,
                      ".. writing char values of %s response to client",
                      (NULL != mod->mod_type) ? mod->mod_type : "");
        for (i = 0; NULL != mod->mod_values[i]; i++) {
            ap_rprintf(r, "%s%s",
                       mod->mod_values[i],
                       (NULL != mod->mod_values[i+1]) ? "; " : "");
        }
    }
}

static const char* duplicate_value_data(request_rec *r, const char *value)
{
    char *result = NULL;
    if (is_psldap_magic_string(value)) {
        size_t newSz = get_psldap_file_magic_buffer_size(r, value);
        result = ap_pcalloc(r->pool, newSz);
        memcpy(result, value, newSz);
    } else {
        result = ap_pstrdup(r->pool, value);
    }
    return result;
}

static int add_record_in_ldap(void *data, const char *key, const char *val)
{
    psldap_status *ps = (psldap_status*)data;
    request_rec *r = ps->rr;
    LDAP *ldap = ps->ldap;
    psldap_config_rec *conf = ps->conf;

    if (NULL == key) {
        if (NULL != val) strncpy((char*)val, "addResponse", 15);
        if ((NULL != ps->mod_dn) && (LDAP_SUCCESS == ps->mod_err)) {
            ps->mod_err = ldap_add_s(ps->ldap, ps->mod_dn, ps->mods);
        }
    }
    else if (0 == strncmp(LDAP_KEY_PREFIX, key, LDAP_KEY_PREFIX_LEN))
    {
        if (0 == strcmp("dn", key + LDAP_KEY_PREFIX_LEN))
        {
            /* The dn cannot be modified */
            ps->mod_dn = ap_pstrdup(r->pool, val);
        } else {
            const char *value = duplicate_value_data(r, val);
            array_header *new_v = parse_arg_string(r, &value, ';');
            LDAPMod **mods_coll;
            LDAPMod *tmp_mod = get_transactions(r, key + LDAP_KEY_PREFIX_LEN,
                                                NULL, new_v);
            if (NULL != tmp_mod) {
                psldap_status_append_mod(ps, r, tmp_mod);
                rprintf_LDAPMod_instance(r, tmp_mod);
                ap_rprintf(r, " <br />");
            }
        }
    }
    return TRUE;
}

static int delete_record_in_ldap(void *data, const char *key, const char *val)
{
    psldap_status *ps = (psldap_status*)data;
    request_rec *r = ps->rr;
    LDAP *ldap = ps->ldap;
    psldap_config_rec *conf = ps->conf;

    if (NULL == key) {
        if (NULL != val) strncpy((char*)val, "delResponse", 15);
        if ((NULL != ps->mod_dn) && (LDAP_SUCCESS == ps->mod_err)) {
            ps->mod_err = ldap_delete_s(ps->ldap, ps->mod_dn);
        }
    }
    else if (0 == strncmp(LDAP_KEY_PREFIX, key, LDAP_KEY_PREFIX_LEN))
    {
        if (0 == strcmp("dn", key + LDAP_KEY_PREFIX_LEN))
        {
            /* The dn cannot be modified */
            ps->mod_dn = ap_pstrdup(r->pool, val);
        }
    }
    return TRUE;
}

static int login_and_reply(void *data, const char *key, const char *val)
{
    return TRUE;
}

static int update_record_in_ldap(void *data, const char *key,
                                 const char *val)
{
    psldap_status *ps = (psldap_status*)data;
    request_rec *r = ps->rr;
    LDAP *ldap = ps->ldap;
    psldap_config_rec *conf = ps->conf;
    const char *oldValue = NULL;
    char *user;

    if (NULL == key) {
        if (NULL != val) strncpy((char*)val, "modifyResponse", 15);
        if ((NULL != ps->mod_dn) && (LDAP_SUCCESS == ps->mod_err)) {
            int i;
            for (i = 0; NULL != ps->mods[i]; i++) {
                rprintf_LDAPMod_instance(ps->rr, ps->mods[i]);
                ap_rprintf(ps->rr, " <br />\n");
            }
            ps->mod_err = ldap_modify_s(ps->ldap, ps->mod_dn, ps->mods);
        }
    }
    else
    {
        if (0 == strncmp(LDAP_KEY_PREFIX, key, LDAP_KEY_PREFIX_LEN))
        {
            char *ldap_query = NULL, *ldap_base = NULL;
            
            get_provided_username(r, &user);
            if (0 == strcmp("dn", key + LDAP_KEY_PREFIX_LEN))
            {
                /* The dn cannot be modified */
                ps->mod_dn = ap_pstrdup(r->pool, val);
                ps->mod_record = get_ldrecords(r, conf, ldap, ps->mod_dn,
                                               user, LDAP_SCOPE_BASE);
                ps->mod_err = get_lderrno(ldap) ;
                oldValue = ps->mod_dn;
            } else {
                oldValue = get_ldvalues_from_record(r, conf, ldap,
                                                    ps->mod_record,
                                                    key + LDAP_KEY_PREFIX_LEN,
                                                    ";");
            }

            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                         r->server,
                         "Finding transactions for key %s", key);
            {
                const char *value = duplicate_value_data(r, val);
                array_header *new_v = parse_arg_string(r, &value, ';');
                array_header *old_v = parse_arg_string(r, &oldValue, ';');
                LDAPMod **mods_coll;
                LDAPMod *tmp_mod = get_transactions(r, key + LDAP_KEY_PREFIX_LEN,
                                                    old_v, new_v);
                if (NULL != tmp_mod) {
                    psldap_status_append_mod(ps, r, tmp_mod);
                }
            }
        }
    }
    return TRUE;
}

static int (*get_action_handler(request_rec *r, const char **action))(void*, const char *,const char *)
{
    get_first_form_fieldvalue(r, FORM_ACTION, (char**)action);
    if (NULL == *action)
    {
        // Assume this is a read request
        return get_dn_attributes_from_ldap;
    }
    else ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                      r->server,
                     "Getting handler for action %s", *action);

    if (0 == strcmp(LOGIN_ACTION, *action))
    {
        return login_and_reply;
    }
    if (0 == strcmp(SEARCH_ACTION, *action))
    {
        return get_dn_attributes_from_ldap;
    }
    if (0 == strcmp(MODIFY_ACTION, *action))
    {
        return update_record_in_ldap;
    }
    if (0 == strcmp(CREATE_ACTION, *action))
    {
        return add_record_in_ldap;
    }
    if (0 == strcmp(DISABLE_ACTION, *action))
    {
        return update_record_in_ldap;
    }
    if (0 == strcmp(DELETE_ACTION, *action))
    {
        return delete_record_in_ldap;
    }
    return NULL;
}

static void write_dsml_err_response(request_rec *r, psldap_status *ps,
                                    const char *opName)
{
    ap_rvputs(r,
              "<?xml-stylesheet type=\"text/xsl\" title=\"Primary View\" href=\"",
              (NULL != ps->xslPrimaryUri) ? ps->xslPrimaryUri : "",
              "\"?>\n",
              "<?xml-stylesheet type=\"text/xsl\" alternate=\"yes\" title=\"Secondary View\" href=\"",
              (NULL != ps->xslSecondaryUri) ? ps->xslSecondaryUri : "",
              "\"?>\n",
              "<dsml>\n",
              " <batchResponse id='",
              (NULL != ps->mod_dn) ? ps->mod_dn : "NULL_DN",
              "'>\n",
              NULL);
    write_dsml_response_fragment(r, opName, ps->mod_err);
    ap_rvputs(r,
              " </batchResponse>\n",
              "</dsml>",
              NULL);
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
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO, r->server,
                     "User <%s> ldap update declined",
                     (NULL == user) ? "" : user);
        return DECLINED;
    }

    read_get(r, &t_env);
    read_post(r, &t_env);
    
    if (OK != (res = get_provided_credentials (r, conf, &password, &user)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO, r->server,
                     "User <%s> ldap update rejected - missing auth info",
                     (NULL == user) ? "null" : user);
    } else if(NULL == (ldap = ps_ldap_init(conf, conf->psldap_hosts,
					   LDAP_PORT))) { 
        ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, r->server,
                     "ldap_init failed on ldap update <%s>",
                     conf->psldap_hosts);
        res = HTTP_INTERNAL_SERVER_ERROR;
    } else if((NULL != (bindas =
                        get_user_dn(r, &ldap, user, password, conf))) &&
              (LDAP_SUCCESS !=
               (err_code = ldap_bind_s(ldap, bindas, password,
                                       conf->psldap_bindmethod) ) )
        ) {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "ldap_bind as user <%s> failed on ldap update: %s",
                     (NULL != bindas) ? bindas : "N/A",
		     ldap_err2string(err_code));
        res = HTTP_INTERNAL_SERVER_ERROR;
    } else {
        const char *action = NULL;
        int (*actionHandler)(void *data, const char *key,
                             const char *val);
        actionHandler = get_action_handler(r, &action);
        if (login_and_reply ==actionHandler)
        {
            const char *post_login_uri = get_post_login_uri(r);

            set_psldap_auth_cookie(r, conf, user, password);
            ap_table_set(r->notes, PSLDAP_REDIRECT_URI, post_login_uri);
            res = ldap_redirect_handler(r);
        } else {
            int sendXml = (get_dn_attributes_from_ldap == actionHandler);

            if (NULL != t_env)
            {
                int err_code = LDAP_SUCCESS;
                const char opName[16] = "errorResponse";
                /* Add ldap connection pointer to the request and process
                   the table entries corresponding to the ldap attributes
                */
                psldap_status ps;
                psldap_status_init(&ps, r, ldap, conf);

                /*  Collect all supporting values from the request into the
                    psldap_status struct */
                ap_table_do(actionHandler, (void*)&ps, t_env, NULL);
                ps.mod_err = get_lderrno(ps.ldap);

                /* Setup the HTTP response ...*/
                if (ps.responseType != NULL) {
                    r->content_type = ps.responseType;
                    sendXml = 0;
                } else {
                    r->content_type = (sendXml) ? "text/xml" : "text/html";
                }
                ap_soft_timeout("update ldap data", r);
                ap_send_http_header(r);
                if (NULL == ps.responseType) {
                    ap_rputs((sendXml) ? "<?xml version=\"1.0\"?>\n" :
                             "<body>\n", r);
                }

                /* Handle the request, fill the opName */
                actionHandler(&ps, NULL, opName);

                if (NULL != ps.mod_record) {
                    ldap_msgfree(ps.mod_record);
                    ps.mod_record = NULL;
                }

                if (NULL == ps.responseType){
                    if (!sendXml) {
                        ap_rputs("<xml id='errResponse'>\n", r);
                    }
                    if (get_dn_attributes_from_ldap != actionHandler) {
                        write_dsml_err_response(r, &ps, opName);
                    }
                    if (!sendXml) {
                        ap_rputs("</xml>\n</body>", r);
                    }
                }
            } else {
                /* Setup the HTTP response ...*/
                r->content_type = "text/html";
                ap_soft_timeout("update ldap data", r);
                ap_send_http_header(r);
                ap_rputs("<body>\n</body>", r);
            }

            ap_kill_timeout(r);

            res = OK;
        }
	
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
                     "ldap_bind as user <%s> status on ldap update: %s",
                     bindas, ldap_err2string(err_code));
        ldap_unbind_s(ldap);
    }
    return res;
}

#ifdef APACHE_V2
static int ldap_update_handlerV2(request_rec *r)
{
    if ((0 != strcmp(r->handler, "ldap-update")) &&
        (0 != strcmp(r->handler, "ldap-auth-form")) &&
        (0 != strcmp(r->handler, "ldap-send-redirect")) ) {
           return DECLINED;
    }
    
    return ldap_update_handler(r);
}

static void register_hooks(pool *p)
{
    /* Hook in the module initialization */
    ap_hook_post_config(module_initV2, NULL, NULL, APR_HOOK_MIDDLE);
    /* [9]  content handlers */
    ap_hook_handler(ldap_update_handlerV2, NULL, NULL, APR_HOOK_MIDDLE);	
    /* [5]  check/validate user_id        */
    ap_hook_check_user_id(ldap_authenticate_user, NULL, NULL, APR_HOOK_LAST);
    /* [6]  check user_id is valid *here* */
    ap_hook_auth_checker(ldap_check_authz, NULL, NULL, APR_HOOK_LAST);
}

module MODULE_VAR_EXPORT psldap_module =
{
    STANDARD20_MODULE_STUFF,
    create_ldap_auth_dir_config,	/* per-directory config creator       */
    merge_ldap_auth_dir_config,		/* dir config merger                  */
    create_ldap_auth_srv_config,	/* server config creator              */
    merge_ldap_auth_srv_config,		/* server config merger               */
    ldap_auth_cmds,					/* config directive table             */
    register_hooks
};

#else
handler_rec ldap_handlers [] =
{
    {"ldap-update", ldap_update_handler},
    {"ldap-auth-form", auth_form_redirect_handler},
    {"ldap-send-redirect", ldap_redirect_handler},
    {NULL}
};

module MODULE_VAR_EXPORT psldap_module =
{
    STANDARD_MODULE_STUFF,
    module_init,		/* initializer                        */
    create_ldap_auth_dir_config,	/* per-directory config creator       */
    merge_ldap_auth_dir_config,		/* dir config merger                  */
    create_ldap_auth_srv_config,	/* server config creator              */
    merge_ldap_auth_srv_config,		/* server config merger               */
    ldap_auth_cmds,		/* config directive table             */
    ldap_handlers,		/* [9]  content handlers              */
    translate_handler,		/* [2]  URI-to-filename translation   */
    ldap_authenticate_user,	/* [5]  check/validate user_id        */
    ldap_check_authz,		/* [6]  check user_id is valid *here* */
    NULL,			/* [4]  check access by host address  */
    NULL,			/* [7]  MIME type checker/setter      */
    NULL,			/* [8]  fixups                        */
    NULL,			/* [10] logger                        */
    NULL,			/* [3]  header parser                 */
    NULL,			/* process initialization             */
    NULL,			/* process exit/cleanup               */
    NULL			/* post read_request handling         */
};
#endif
