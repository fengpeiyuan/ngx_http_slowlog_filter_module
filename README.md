Name
====

**ngx_http_slowlog_filter_module**

This project is still experimental and under early development.

Description
===========

**ngx_http_slowlog_filter_module** - one of nginx real-time monitor module to record queries which exceeded a specified execution time. 


*This module is not distributed with the Nginx source.* See [the installation instructions](#installation).


Version
=======

This document describes ngx_http_slowlog_filter_module released on 25 January 2015.

Synopsis
========

This module is a nginx filter module, which accumulate request' time span between now and start_time(start_sec and start_msec in the struct ngx_http_request_t) by http header filter and records in each process memory. The execution time does not include I/O operations like talking with client and sending the reply.
You can config bellow.
```nginx
 http {
	...
	#mark to monitor this location
	slowlog on; 
	#request' execution time(in microseconds) more than 1000ms will be marked
	slowlog_slower_than 1000;
	#the lengh of max slowlog
	slowlog_max_len 10;
	... 

	#check result in this location
	location /sloglogget {
		#fetch results
		slowlog_get;
		#access/deny maybe used
		#deny  192.168.1.1;
		#allow 192.168.1.0/24;
		#allow 10.1.1.0/16;
		#allow 2001:0db8::/32;
		#deny  all;
	 }

```

Limitations
===========


Installation
============

Grab the nginx source code from [nginx.org](http://nginx.org/), for example,
the version 1.7.7, and then build the source with this module:

```bash

 wget 'http://nginx.org/download/nginx-1.7.7.tar.gz'
 tar -xzvf nginx-1.7.7.tar.gz
 cd nginx-1.7.7/

 # Here we assume you would install you nginx under /opt/nginx/.
 ./configure --prefix=/opt/nginx \
     --add-module=/path/to/ngx_http_slowlog_filter_module

 make
 make install
```

Download the latest version of the release tarball of this module

Authors
=======

* Peiyuan Feng *&lt;fengpeiyuan@gmail.com&gt;*.

This wiki page is also maintained by the author himself, and everybody is encouraged to improve this page as well.

Copyright & License
===================

Copyright (c) 2014-2015, Peiyuan Feng <fengpeiyuan@gmail.com>.

This module is licensed under the terms of the BSD license.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
