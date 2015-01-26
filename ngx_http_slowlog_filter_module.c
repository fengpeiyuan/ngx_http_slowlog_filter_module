#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#define ngx_get_struct_from_member(ptr, type, member) ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

typedef struct {
	ngx_queue_t	*queue_container;
	ngx_pool_t *pool;
} ngx_http_slowlog_main_conf_t;

typedef struct {
    ngx_msec_t slowlog_log_slower_than;
    ngx_flag_t slowlog;
    ngx_uint_t slowlog_max_len;
    ngx_atomic_t slowlog_curr_len;
} ngx_http_slowlog_loc_conf_t;

typedef struct {
	ngx_queue_t queue_ele;
	ngx_uint_t latency;
	ngx_uint_t latency_slower_than;
	ngx_uint_t latency_diff;
	ngx_str_t *uri;
    ngx_str_t url;
    ngx_str_t addr;
} ngx_http_slowlog_queue_ele_t;

static ngx_http_output_header_filter_pt ngx_http_next_header_filter;

static ngx_int_t ngx_http_slowlog_filter_init (ngx_conf_t *cf);
static ngx_int_t ngx_http_slowlog_header_filter(ngx_http_request_t *r);
static void *ngx_http_slowlog_create_main_conf(ngx_conf_t *cf);
static char *ngx_http_slowlog_merge_loc_conf(ngx_conf_t *cf,void *parent,void *child);
static void *ngx_http_slowlog_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_slowlog_get_command(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_slowlog_get_handler(ngx_http_request_t *r);

static ngx_command_t ngx_http_slowlog_filter_commands[] = {
    {
      ngx_string("slowlog"),
      NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_slowlog_loc_conf_t, slowlog),
      NULL
    },
    {
      ngx_string("slowlog_log_slower_than"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_slowlog_loc_conf_t, slowlog_log_slower_than),
      NULL
    },
    {
      ngx_string("slowlog_max_len"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_slowlog_loc_conf_t, slowlog_max_len),
      NULL
    },
 	{
 		ngx_string("slowlog_get"),
 		NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
 		ngx_http_slowlog_get_command,
 		NGX_HTTP_LOC_CONF_OFFSET,
 		0,
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

    main_conf->queue_container = ngx_pcalloc(cf->pool, sizeof(ngx_queue_t));
    ngx_queue_init(main_conf->queue_container);
    main_conf->pool = cf->pool;

    return main_conf;
}

static void *ngx_http_slowlog_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_slowlog_loc_conf_t  *loc_conf;
    loc_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_slowlog_loc_conf_t));
    if (loc_conf == NULL) {
        return NGX_CONF_ERROR;
    }

    loc_conf->slowlog = NGX_CONF_UNSET;
    loc_conf->slowlog_log_slower_than = NGX_CONF_UNSET_MSEC;
    loc_conf->slowlog_max_len = NGX_CONF_UNSET;

    return loc_conf;
}

static char *ngx_http_slowlog_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_slowlog_loc_conf_t *prev = parent;
    ngx_http_slowlog_loc_conf_t *conf = child;

    ngx_conf_merge_uint_value(conf->slowlog_log_slower_than, prev->slowlog_log_slower_than, 5000);
    ngx_conf_merge_value(conf->slowlog, prev->slowlog, 0);
    ngx_conf_merge_uint_value(conf->slowlog_max_len, prev->slowlog_max_len, 10);

    return NGX_CONF_OK;
}

static ngx_int_t node_comparer(const ngx_queue_t* a,const ngx_queue_t* b)
{
	ngx_http_slowlog_queue_ele_t *node_a = ngx_queue_data(a,ngx_http_slowlog_queue_ele_t,queue_ele);
	ngx_http_slowlog_queue_ele_t *node_b = ngx_queue_data(b,ngx_http_slowlog_queue_ele_t,queue_ele);
	ngx_atomic_t a_count = node_a->latency;
	ngx_atomic_t b_count = node_b->latency;
	return a_count < b_count;
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
    if (loc_conf->slowlog == 1)
    {
        ngx_gettimeofday(&tv);
        now_sec = tv.tv_sec;
        now_msec = tv.tv_usec / 1000;
        req_sec = r->start_sec;
        req_msec = r->start_msec;
        latency_msec = ((now_sec * 1000) + now_msec) - ((req_sec * 1000) + req_msec);
        if ( latency_msec >= loc_conf->slowlog_log_slower_than)
        {
        	ngx_http_slowlog_main_conf_t *main_conf = ngx_http_get_module_main_conf(r,ngx_http_slowlog_filter_module);
        	if(main_conf->queue_container == NULL)
        	{
        		ngx_log_error(NGX_LOG_ERR,r->connection->log,0,"# ngx_http_slowlog_header_filter # queue_container is NULL");
        		return ngx_http_next_header_filter(r);
        	}
        	if(loc_conf->slowlog_curr_len >= loc_conf->slowlog_max_len+1)
        	{
        		ngx_queue_sort(main_conf->queue_container, node_comparer);
        		ngx_queue_t *qlast = ngx_queue_last(main_conf->queue_container);
        		ngx_http_slowlog_queue_ele_t *qele_last = ngx_get_struct_from_member(qlast,ngx_http_slowlog_queue_ele_t,queue_ele);
        		ngx_queue_remove(qlast);
        		ngx_pfree(main_conf->pool,qele_last);

        	}

        	ngx_http_slowlog_queue_ele_t *qele = ngx_pcalloc(main_conf->pool, sizeof(ngx_http_slowlog_queue_ele_t));
        	qele->latency = latency_msec;
        	qele->latency_slower_than = loc_conf->slowlog_log_slower_than;
        	qele->latency_diff = latency_msec - loc_conf->slowlog_log_slower_than;
        	qele->uri = &r->uri;
        					//TODO add others

        	ngx_queue_insert_tail(main_conf->queue_container,&qele->queue_ele);
        	ngx_atomic_fetch_add(&loc_conf->slowlog_curr_len,1);

        }


    }
    return ngx_http_next_header_filter(r);
}

static char *ngx_http_slowlog_get_command(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_http_core_loc_conf_t  *clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
	clcf->handler = ngx_http_slowlog_get_handler;

	return NGX_CONF_OK;
}

static ngx_int_t ngx_http_slowlog_get_handler(ngx_http_request_t *r)
{
	ngx_int_t rc;
	if (!(r->method & (NGX_HTTP_GET | NGX_HTTP_HEAD)))
	{
	    return NGX_HTTP_NOT_ALLOWED;
	}
	rc = ngx_http_discard_request_body(r);
	if (rc != NGX_OK)
	{
	    return rc;
	}

	ngx_str_set(&r->headers_out.content_type,"text/plain");
	if(r->method == NGX_HTTP_HEAD)
	{
		r->headers_out.status = NGX_HTTP_OK;
		rc = ngx_http_send_header(r);
		if(rc == NGX_ERROR || rc > NGX_OK || r->header_only)
		{
			return rc;
		}
	}

	ngx_http_slowlog_main_conf_t *main_conf = ngx_http_get_module_main_conf(r,ngx_http_slowlog_filter_module);

	size_t size = sizeof("slowlogs list:\r\n");
	ngx_buf_t *b = ngx_create_temp_buf(r->pool,size);
	ngx_chain_t *out = ngx_alloc_chain_link(r->pool);
	if(b == NULL || out == NULL)
	{
		return NGX_HTTP_INTERNAL_SERVER_ERROR;
	}
	out->buf = b;
	out->next = NULL;
	b->last = ngx_sprintf(b->last,"slowlogs list:\r\n");
	b->last_buf = 1;

	ngx_chain_t *out_mid = ngx_alloc_chain_link(r->pool);
	out_mid->next = out;

	ngx_int_t c = 0;
	ngx_queue_t *q;
	for(q = ngx_queue_head(main_conf->queue_container);q!=ngx_queue_sentinel(main_conf->queue_container);q=ngx_queue_next(q))
	{
		ngx_http_slowlog_queue_ele_t *node = ngx_queue_data(q,ngx_http_slowlog_queue_ele_t,queue_ele);

		size_t size_tmp = sizeof(node->latency)+sizeof(*(node->uri))+sizeof("\r\n");
		ngx_buf_t *b_tmp = ngx_create_temp_buf(r->pool,size_tmp);
		ngx_chain_t *out_tmp = ngx_alloc_chain_link(r->pool);
		if(b_tmp == NULL || out_tmp == NULL)
		{
			return NGX_HTTP_INTERNAL_SERVER_ERROR;
		}
		out_tmp->buf = b_tmp;
		out_tmp->next = NULL;

		b_tmp->last = ngx_sprintf(b_tmp->last,"%d %s\r\n",node->latency,node->uri->data);
		b_tmp->last_buf = 1;

		out_mid->next->next = out_tmp;
		out_mid->next = out_tmp;

		c++;
		if(c>100) break;
	}

	r->headers_out.status = NGX_HTTP_OK;

	rc = ngx_http_send_header(r);
	if (rc == NGX_ERROR || rc > NGX_OK || r->header_only)
	{
	    return rc;
	}

	return ngx_http_output_filter(r, out);
}


static ngx_int_t ngx_http_slowlog_filter_init (ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_slowlog_header_filter;

    return NGX_OK;
}

