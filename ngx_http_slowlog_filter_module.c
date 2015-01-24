#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
    ngx_msec_t slowlog_log_slower_than;
    ngx_flag_t enable;
    ngx_uint_t slowlog_max_len;
} ngx_http_slowlog_conf_t;

static ngx_http_output_header_filter_pt ngx_http_next_header_filter;

static ngx_int_t ngx_http_slowlog_filter_init (ngx_conf_t*);
static ngx_int_t ngx_http_slowlog_header_filter(ngx_http_request_t*);
static char * ngx_http_slowlog_merge_loc_conf(ngx_conf_t*, void*, void*);
static void * ngx_http_slowlog_create_loc_conf(ngx_conf_t*);

static ngx_command_t ngx_http_slowlog_filter_commands[] = {
    {
      ngx_string("slowlog"),
      NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_slowlog_conf_t, enable),
      NULL
    },
    {
      ngx_string("slowlog_log_slower_than"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_slowlog_conf_t, slow_than),
      NULL
    },
    {
      ngx_string("slowlog_max_len"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_slowlog_conf_t, max_len),
      NULL
    },
    ngx_null_command
};

static ngx_http_module_t  ngx_http_slowlog_filter_ctx = {
    NULL,
    ngx_http_slowlog_filter_init,
    NULL,
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

static void *ngx_http_slowlog_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_slowlog_conf_t  *conf;
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_slowlog_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }

    conf->enable = NGX_CONF_UNSET;
    conf->slow_than = NGX_CONF_UNSET_MSEC;
    conf->max_len = NGX_CONF_UNSET;

    return conf;
}

static char *ngx_http_slowlog_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_slowlog_conf_t *prev = parent;
    ngx_http_slowlog_conf_t *conf = child;

    ngx_conf_merge_uint_value(conf->slowlog_log_slower_than, prev->slowlog_log_slower_than, 5000);
    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    ngx_conf_merge_uint_value(conf->slowlog_max_len, prev->slowlog_max_len, 10);

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_slowlog_header_filter(ngx_http_request_t *r)
{
    struct timeval tv;
    ngx_http_log_request_speed_conf_t *lrsl;
    ngx_uint_t now_sec;
    ngx_uint_t req_sec;
    ngx_uint_t now_msec;
    ngx_uint_t req_msec;
    ngx_uint_t msec_diff;

    lrsl = ngx_http_get_module_loc_conf(r, ngx_http_log_request_speed_filter_module);

    if (lrsl->enable == 1)
    {
        ngx_gettimeofday(&tv);

        now_sec = tv.tv_sec;
        now_msec = tv.tv_usec / 1000;

        req_sec = r->start_sec;
        req_msec = r->start_msec;

        if (((now_sec * 1000) + now_msec) - ((req_sec * 1000) + req_msec) >= (lrsl->timeout))
        {
            msec_diff = (now_sec - req_sec) * 1000;
            msec_diff += now_msec - req_msec;
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

