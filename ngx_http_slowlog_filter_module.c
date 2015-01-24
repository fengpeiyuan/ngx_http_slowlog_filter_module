#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
	ngx_queue_t	*queue_container;
} ngx_http_slowlog_main_conf_t;

typedef struct {
    ngx_msec_t slowlog_log_slower_than;
    ngx_flag_t enable;
    ngx_uint_t slowlog_max_len;
    ngx_atomic_t slowlog_curr_len;
} ngx_http_slowlog_loc_conf_t;

typedef struct {
	ngx_queue_t queue;
	ngx_uint_t latency_now;
	ngx_uint_t latency_slower_than;
	ngx_uint_t latency_diff;
    ngx_str_t url;
    ngx_str_t addr;
} ngx_http_slowlog_queue_ele_t;

static ngx_http_output_header_filter_pt ngx_http_next_header_filter;

static ngx_int_t ngx_http_slowlog_filter_init (ngx_conf_t *cf);
static ngx_int_t ngx_http_slowlog_header_filter(ngx_http_request_t *r);
static void * ngx_http_slowlog_create_main_conf(ngx_conf_t *cf);
static char * ngx_http_slowlog_merge_loc_conf(ngx_conf_t *cf,,void *parent,void *child);
static void * ngx_http_slowlog_create_loc_conf(ngx_conf_t *cf);

static ngx_command_t ngx_http_slowlog_filter_commands[] = {
    {
      ngx_string("slowlog"),
      NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_slowlog_loc_conf_t, enable),
      NULL
    },
    {
      ngx_string("slowlog_log_slower_than"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_slowlog_loc_conf_t, slow_than),
      NULL
    },
    {
      ngx_string("slowlog_max_len"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_slowlog_loc_conf_t, max_len),
      NULL
    },
    ngx_null_command
};

static ngx_http_module_t  ngx_http_slowlog_filter_ctx = {
    NULL,
    ngx_http_slowlog_filter_init,
    ngx_http_slowlog_create_main_conf,
    NULL,
    NULL,
    NULL,
    ngx_http_slowlog_create_loc_conf,
    ngx_http_slowlog_merge_loc_conf
};

ngx_module_t  ngx_http_slowlog_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_slowlog_filter_ctx,
    ngx_http_slowlog_filter_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

static void *ngx_http_slowlog_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_slowlog_main_conf_t  *main_conf;
    main_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_slowlog_main_conf_t));
    if (main_conf == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_queue_init(main_conf->queue_container);

    return main_conf;
}

static void *ngx_http_slowlog_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_slowlog_loc_conf_t  *loc_conf;
    loc_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_slowlog_loc_conf_t));
    if (loc_conf == NULL) {
        return NGX_CONF_ERROR;
    }

    loc_conf->enable = NGX_CONF_UNSET;
    loc_conf->slow_than = NGX_CONF_UNSET_MSEC;
    loc_conf->max_len = NGX_CONF_UNSET;

    return loc_conf;
}

static char *ngx_http_slowlog_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_slowlog_loc_conf_t *prev = parent;
    ngx_http_slowlog_loc_conf_t *conf = child;

    ngx_conf_merge_uint_value(conf->slowlog_log_slower_than, prev->slowlog_log_slower_than, 5000);
    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    ngx_conf_merge_uint_value(conf->slowlog_max_len, prev->slowlog_max_len, 10);

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_slowlog_header_filter(ngx_http_request_t *r)
{
    struct timeval tv;
    ngx_http_slowlog_loc_conf_t *loc_conf;
    ngx_uint_t now_sec;
    ngx_uint_t req_sec;
    ngx_uint_t now_msec;
    ngx_uint_t req_msec;
    ngx_uint_t latency_msec;

    loc_conf = ngx_http_get_module_loc_conf(r, ngx_http_slowlog_filter_module);

    if (loc_conf->enable == 1)
    {
        ngx_gettimeofday(&tv);
        now_sec = tv.tv_sec;
        now_msec = tv.tv_usec / 1000;
        req_sec = r->start_sec;
        req_msec = r->start_msec;

        latency_msec = ((now_sec * 1000) + now_msec) - ((req_sec * 1000) + req_msec);
        if ( latency_msec >= (loc_conf->slowlog_log_slower_than))
        {
        	ngx_http_slowlog_main_conf_t *main_conf = ngx_http_get_module_main_conf(r,ngx_http_slowlog_filter_module);
        	if(main_conf->queue_container == NULL)
        	{
        		return ngx_http_next_header_filter(r);
        	}



        }
    }
    return ngx_http_next_header_filter(r);
}

static ngx_int_t ngx_http_slowlog_filter_init (ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_slowlog_header_filter;

    return NGX_OK;
}

