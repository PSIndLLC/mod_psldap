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

#define PSLDAP_VERSION_LABEL "0.75"

#include "httpd.h"
#include "http_conf_globals.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"
#include "ap_compat.h"
#include "ap_mm.h"

#if 0
/* Semaphores will not work on Linux, process sharing semaphores are not
   implemented */
 #define CACHE_USE_SEMAPHORE 1
#endif

#ifdef MPM20_MODULE_STUFF
 #define APACHE_V2
 #include "apr_compat.h"
 #include "apr_errno.h"
 #include "apr_general.h"
 #include "apr_hooks.h"
 #include "apr_pools.h"
 #include "apr_sha1.h"
 #include "apr_strings.h"
 #define XtOffsetOf APR_OFFSETOF
 #define DEBUG_ERRNO ,APR_SUCCESS
 #define ap_log_reason(a,b,c)
 #define ap_sha1_base64 apr_sha1_base64
 typedef apr_pool_t pool;
 typedef apr_table_t table;
 typedef apr_array_header_t array_header;
 #define ap_hard_timeout(n, r)
 #define ap_reset_timeout(r)
 #define ap_kill_timeout(r)
 #define ap_soft_timeout(n, r)
 #define add_version_component(p,v)	ap_add_version_component(p, v)
 #define sub_req_lookup_uri(uri, r, p) ap_sub_req_lookup_uri(uri, r, p)
 extern module MODULE_VAR_EXPORT psldap_module;
#else
 #define add_version_component(p,v)	ap_add_version_component(v)
 #define sub_req_lookup_uri(uri, r, p) ap_sub_req_lookup_uri(uri, r)
 typedef const char *(*cmd_func) (cmd_parms*, void*, const char*);
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

#define FORM_ACTION		"FormAction"
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

typedef struct psldap_array_struct psldap_array;

typedef struct psldap_cache_item_struct {
    PSCACHE_ITEM_FIELDS();
} pscache_item;

#ifdef CACHE_USE_SEMAPHORE
 typedef sem_t psldap_guard;
 #define psldap_guard_init(g,pshr,max)	sem_init(g,pshr,max)
 #define psldap_guard_destroy(g)		sem_destroy(g)
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
    AP_MM *mem_mgr;
    int count;
    psldap_shared_data *collection;
    int (***psldap_compares_item)(const void *item1, const void *item2);
    void (**cached_item_free)(void *item);
} psldap_shared_dataview;


static psldap_shared_dataview dataview = {NULL, NULL, 0, NULL, NULL, NULL};

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
    dataview.mem_mgr = ap_mm_create(get_shared_size(s, width),
                                    get_shared_filename(s)); 
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
static void psldap_server_cleanup(void *server)
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
        ap_mm_destroy(dataview.mem_mgr);
        dataview.count = 0;
    }
}

static void psldap_child_cleanup(void *server)
{
    /* Nothing to do here */
    return;
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
static AP_MM* psldap_cache_start(server_rec *s, pool *p,
                                 void (*a_free_item)(void* item),
                                 int (**a_compares_item)(const void *item1,
                                                         const void *item2),
                                 int cache_width)
{
    ap_register_cleanup(s->ctx->cr_pool, s, psldap_server_cleanup,
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
    psldap_cache_item *result = ap_mm_calloc(cache_mem_mgr, 1,
                                             sizeof(psldap_cache_item));
    if (NULL != result) {

        /* Insert the values retrieved from LDAP into the cache */
        if ((NULL == dn) || (NULL == groups))
        { /* Set the dn and group if either was not passed */
            LDAP* ldap = NULL;
            psldap_config_rec *conf =
                (psldap_config_rec *)ap_get_module_config (r->per_dir_config,
                                                           &psldap_module);
            if (NULL == (ldap = ldap_init(conf->psldap_hosts, LDAP_PORT)))
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
    } else {
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

typedef struct
{
    request_rec *rr;
    LDAP *ldap;
    psldap_config_rec *conf;
} psldap_status;

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

static void module_init(server_rec *s, pool *p)
{
    add_version_component(p, "mod_psldap/" PSLDAP_VERSION_LABEL);
    cache_mem_mgr = psldap_cache_start(s, p, psldap_cache_item_free,
                                       psldap_compares_item, CACHE_WIDTH);
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

    sec->psldap_auth_enabled = INT_UNSET;
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
    set_cfg_str_if_n_set(p, result, n, psldap_cookiedomain);
    set_cfg_int_if_n_set(result, n, psldap_auth_enabled, INT_UNSET, INT_UNSET);
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

    set_cfg_int_if_n_set(result, b, psldap_auth_enabled, INT_UNSET, 1);
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
    /* Set the URI for the form to capture credentials */
    set_cfg_str_if_n_set(p, result, b, psldap_credential_uri);

    return result;
}

void *merge_ldap_auth_srv_config (pool *p, void *base_conf, void *new_conf)
{
    psldap_server_rec *result
        = (psldap_server_rec *)ap_pcalloc (p, sizeof(psldap_server_rec));
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

static const char* set_ldap_slot(cmd_parms *parms, void *mconfig,
                                 const char *to) {
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
    { "PsLDAPEnableAuth", (cmd_func)ap_set_flag_slot,
      (void*)XtOffsetOf(psldap_config_rec, psldap_auth_enabled),
      OR_AUTHCFG, FLAG, 
      "Flag to enable / disable A&A. Default value is 'on'"
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
      " to acquire the DN of the authenticating user."
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
      "Set to 'simple', 'krbv41', or 'krbv42' to determine binding to server"
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

    if(LDAP_SUCCESS != (err_code = ldap_bind_s(*ldap, conf->psldap_binddn,
                                               conf->psldap_bindpassword,
                                               conf->psldap_bindmethod))
       )
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
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
    *ldap = ldap_init(conf->psldap_hosts, LDAP_PORT);

 AbortDNAcquisition:
    return user_dn;
}

static int set_bind_params(request_rec *r, LDAP **ldap,
                           psldap_config_rec *conf,
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
            *user = get_user_dn(r, ldap, *user, *password, conf);
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
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "ldap_search failed: %s", ldap_err2string(err_code));
    }
    else if(!(ld_entry = ldap_first_entry(ldap, ld_result)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "user <%s> not found", user);
    }
    else if(!(ld_values = ldap_get_values(ldap, ld_entry, attr)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                     "ldap_get_values <%s> failed", attr);
    }
    else
    {
        result = build_string_list(r, ld_values, separator);
    }

    if (NULL != ld_values) { ldap_value_free(ld_values); }
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
    if (NULL == (*ldap = ldap_init(conf->psldap_hosts, LDAP_PORT)))
    { 
        ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, r->server,
                     "ldap_init failed <%s>", conf->psldap_hosts);
        return *ldap;
    }
    
    if(set_bind_params(r, ldap, conf, &bindas, &bindpass))
    {
        if(ldap_bind_s(*ldap, bindas, bindpass, conf->psldap_bindmethod))
        {
            ap_log_error(APLOG_MARK, APLOG_WARNING DEBUG_ERRNO, r->server,
                         "ldap_bind as user <%s> failed", bindas);
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
                                 const char *attr, const char *separator) {
    char *retval = NULL;

    if(NULL == attr) retval = "bind";
    else
    {
        char *ldap_query = NULL, *ldap_base = NULL;
        
        ldap_query = construct_ldap_query(r, conf, query_by, query_for,
                                          user);
        ldap_base = construct_ldap_base(r, conf, ldap_query);
        
        retval = get_ldvalues_from_connection(r, conf, ldap, ldap_base,
                                              ldap_query, user, attr,
                                              separator);
    }

    return retval; 
}

static char * get_ldap_val(request_rec *r, const char *user, const char *pass,
                           psldap_config_rec *conf,
                           const char *query_by, const char *query_for,
                           const char *attr, const char *separator) {
    const char *bindas = user, *bindpass = pass;
    char *retval = NULL;
    LDAP *ldap = NULL;
    
    if(NULL == attr) retval = "bind";
    if((NULL != attr) && (NULL != ps_bind_ldap(r, &ldap, user, pass, conf)) ) {
        retval = get_ldap_val_bound(r, ldap, conf, user, query_by,
                                    query_for, attr, separator);
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
       (NULL == (ldap = ldap_init(conf->psldap_hosts, LDAP_PORT))) )
    { 
        ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, r->server,
                     "ldap_init failed <%s>", conf->psldap_hosts);
        return retval;
    }

    if (NULL != ps_bind_ldap(r, &ldap, user, pass, conf) ) {
        groupkey = (NULL == conf->psldap_user_grp_attr) ? NULL :
            get_ldap_val_bound(r, ldap, conf, user, NULL, NULL,
                               conf->psldap_user_grp_attr, ":");
        if (NULL != groupkey)
        {
            while(groupkey[0])
            {
                char *v = ap_getword(r->pool, &groupkey,':');
                char *groups = (NULL == conf->psldap_grp_mbr_attr) ? NULL :
                    get_ldap_val_bound(r, ldap, conf, user,
                                       conf->psldap_grp_mbr_attr,
                                       v, conf->psldap_grp_nm_attr, delim);
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
                             conf->psldap_groupkey, ",");
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
    /* Get the userkey to avoid any security issues regarding password
       protection on the server. You can pretty much guarantee that a user
       will be able to read their own userkey
    */
    if(NULL != get_ldap_val(r, user, sent_pw, sec, NULL, NULL,
                            sec->psldap_userkey, ",") )
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

    if(NULL != (real_pw = get_ldap_val(r, user, sent_pw, sec, NULL,
                                       NULL, sec->psldap_passkey, ",")))
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
        else ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, r->server,
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

static int get_form_fieldvalue(request_rec *r, const char* fieldname,
                               char **sent_value)
{
    int result = 0;
    char *tmp = NULL;
    table *tab = NULL;

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
                *sent_value = ap_pstrdup(r->pool, get_user_name(r) );
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
                *sent_value = ap_pstrdup(r->pool, get_user_name(r) );
                return OK;
            } else if (fieldKey == sec->psldap_passkey) {
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
    ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
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
            ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
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
                 "Authenticating LDAP user");
    if (OK != (res = get_provided_credentials (r, sec, &sent_pw, &sent_user
                                               )))
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO,
                     r->server,
                     "Failed to acquire credentials for authentication",
                     (NULL == sent_user) ? "" : sent_user);
    }
    else
    {
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
            res = ((0 == strcmp(sent_user, record->key)) &&
                   (0 == strcmp(sent_pw, record->passwd)) ) ?
                OK : HTTP_UNAUTHORIZED;
        } else if(sec->psldap_authexternal) {
            res = authenticate_via_bind (r, sec, sent_user, sent_pw);
        } else if (sec->psldap_authsimple) {
            res = authenticate_via_query (r, sec, sent_user, sent_pw);
        }
        else res = authenticate_via_query (r, sec, sent_user, sent_pw);
    }
	
    if (res == HTTP_UNAUTHORIZED)
    {
        char *authType = (char*)ap_auth_type(r);
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO,
                     r->server,
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
            request_rec *sr;

            sr = sub_req_lookup_uri(sec->psldap_credential_uri, r, NULL);
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
        } else if (sec->psldap_authoritative) {
            ap_log_error (APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
                          "LDAP user %s not found or password invalid",
                          (NULL != sent_user) ? sent_user : "<blank>");
            ap_note_basic_auth_failure (r);
        } else {
            res = DECLINED;
        }
    } else if ((OK == res) && cacheResultMissing) {
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

    if (!sec->psldap_userkey || !sec->psldap_auth_enabled)
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

static array_header* parse_arg_string(request_rec *r, const char **arg_string,
                                      const char delimiter)
{
    array_header *result = ap_make_array(r->pool, 1, sizeof(char*));
    while('\0' != (*arg_string)[0])
    {
        char **w = ap_push_array(result);
        *w = ap_getword(r->pool, arg_string, delimiter);
    }
    qsort(result->elts, result->nelts, result->elt_size, compare_arg_strings);
    return result;
}

typedef struct {
    array_header *keepers;
    array_header *deletions;
    array_header *additions;
} psldap_txns;

static psldap_txns* get_transactions(request_rec *r, array_header *old_v,
                                     array_header *new_v)
{
    psldap_txns *result = ap_palloc(r->pool, sizeof(psldap_txns));
    int i = 0, j = 0;
    result->keepers = ap_make_array(r->pool, 1, sizeof(char*));
    result->deletions = ap_make_array(r->pool, 1, sizeof(char*));
    result->additions = ap_make_array(r->pool, 1, sizeof(char*));

    while ((i < old_v->nelts) || (j < new_v->nelts)) {
        char **value;
        if (j >= new_v->nelts) {
            value = (char**)ap_push_array(result->deletions);
            *value = ap_pstrdup(r->pool, ((char**)(old_v->elts))[i++]);
        } else if (i >= old_v->nelts) {
            value = (char**)ap_push_array(result->additions);
            *value = ap_pstrdup(r->pool, ((char**)(new_v->elts))[j++]);
        } else {
            int compare = strcmp(((char**)(old_v->elts))[i],
                                 ((char**)(new_v->elts))[j]);
            if (compare < 0) {
                value = (char**)ap_push_array(result->deletions);
                *value = ap_pstrdup(r->pool, ((char**)(old_v->elts))[i++]);
            } else if (compare > 0) {
                value = (char**)ap_push_array(result->additions);
                *value = ap_pstrdup(r->pool, ((char**)(new_v->elts))[j++]);
            } else {
                value = (char**)ap_push_array(result->keepers);
                *value = ap_pstrdup(r->pool, ((char**)(old_v->elts))[i++]);
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
        array_header *new_v = parse_arg_string(r, &value, ';');
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
            array_header *old_v = parse_arg_string(r, &oldValue, ';');
            psldap_txns *txns = get_transactions(r, old_v, new_v);
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
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO, r->server,
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
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO, r->server,
                     "User <%s> ldap update declined",
                     (NULL == user) ? "" : user);
        return DECLINED;
    }

    read_post(r, &t_env);
    
    if (OK != (res = get_provided_credentials (r, conf, &password, &user)))
    {
        ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_INFO DEBUG_ERRNO, r->server,
                     "User <%s> ldap update rejected - missing auth info",
                     (NULL == user) ? "null" : user);
    }
    else if(NULL == (ldap = ldap_init(conf->psldap_hosts, LDAP_PORT)))
    { 
        ap_log_error(APLOG_MARK, APLOG_ERR DEBUG_ERRNO, r->server,
                     "ldap_init failed on ldap update <%s>",
                     conf->psldap_hosts);
        res = HTTP_INTERNAL_SERVER_ERROR;
    }
    else if((NULL != (bindas = get_user_dn(r, &ldap, user, password, conf))) &&
            (LDAP_SUCCESS !=
             (err_code = ldap_bind_s(ldap, bindas, password,
                                     conf->psldap_bindmethod) ) )
            )
    {
        ap_log_error(APLOG_MARK, APLOG_NOTICE DEBUG_ERRNO, r->server,
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

            sr = sub_req_lookup_uri(post_login_uri, r, NULL);
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
                ap_log_error(APLOG_MARK, APLOG_NOERRNO|APLOG_DEBUG DEBUG_ERRNO,
                             r->server,
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
    if (strcmp(r->handler, "ldap-update") ||
        strcmp(r->handler, "application/x-ldap-update") ||
        strcmp(r->handler, "application/x-httpd-psldap") ) {
        return DECLINED;
    }
    
    return ldap_update_handler(r);
}

static void register_hooks(pool *p)
{
    /* Hook in the module initialization */
    ap_hook_post_config(module_init, NULL, NULL, APR_HOOK_MIDDLE);
    /* [9]  content handlers */
    ap_hook_handler(ldap_update_handlerV2, NULL, NULL, APR_HOOK_MIDDLE);	
    /* [5]  check/validate user_id        */
    ap_hook_check_user_id(ldap_authenticate_user, NULL, NULL, APR_HOOK_LAST);
    /* [6]  check user_id is valid *here* */
    ap_hook_auth_checker(ldap_check_auth, NULL, NULL, APR_HOOK_LAST);
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
    {"application/x-ldap-update", ldap_update_handler},
    {"application/x-httpd-psldap", ldap_update_handler},
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
#endif
