=========================================================================================================================
``slowlog module`` - one of nginx real-time monitor module to record queries which exceeded a specified execution time.
=========================================================================================================================

= Name =

'''ngx_http_slowlog_filter_module''' - a nginx filter module, which accumulate request' time span between now and start_time(start_sec and start_msec in the ngx_http_request_t struct) by filte http header and records in memory.

''This module is not distributed with the Nginx source.'' See [[#Installation|the installation instructions]].

= Version =

This document describes ngx_http_slowlog_filter_module released on 25 January 2015.

= Synopsis =

<geshi lang="nginx">
    # set the Server output header
    more_set_headers 'Server: my-server';

    # set and clear output headers
    location /bar {
        more_set_headers 'X-MyHeader: blah' 'X-MyHeader2: foo';
        more_set_headers -t 'text/plain text/css' 'Content-Type: text/foo';
        more_set_headers -s '400 404 500 503' -s 413 'Foo: Bar';
        more_clear_headers 'Content-Type';
        
        # your proxy_pass/memcached_pass/or any other config goes here...
    }

    # set output headers
    location /type {
        more_set_headers 'Content-Type: text/plain';
        # ...
    }

    # set input headers
    location /foo {
        set $my_host 'my dog';
        more_set_input_headers 'Host: $my_host';
        more_set_input_headers -t 'text/plain' 'X-Foo: bah';
       
        # now $host and $http_host have their new values...
        # ...
    }

    # replace input header X-Foo *only* if it already exists
    more_set_input_headers -r 'X-Foo: howdy';
</geshi>

= Description =



= Directives =


= Limitations =


= Installation =

Grab the nginx source code from [http://nginx.org/ nginx.org], for example,
the version 1.7.7 (see [[#Compatibility|nginx compatibility]]), and then build the source with this module:

<geshi lang="bash">
    wget 'http://nginx.org/download/nginx-1.7.7.tar.gz'
    tar -xzvf nginx-1.7.7.tar.gz
    cd nginx-1.7.7/
    
    # Here we assume you would install you nginx under /opt/nginx/.
    ./configure --prefix=/opt/nginx \
        --add-module=/path/to/ngx_http_slowlog_filter_module
     
    make
    make install
</geshi>

Download the latest version of the release tarball of this module from 

= Compatibility =

The following versions of Nginx should work with this module:

* '''1.7.x'''                       (last tested: 1.7.7)
* '''1.6.x'''                       (last tested: 1.6.2)
* '''1.5.x'''                       (last tested: 1.5.8)
* '''1.4.x'''                       (last tested: 1.4.4)
* '''1.3.x'''                       (last tested: 1.3.7)
* '''1.2.x'''                       (last tested: 1.2.9)
* '''1.1.x'''                       (last tested: 1.1.5)
* '''1.0.x'''                       (last tested: 1.0.11)
* '''0.9.x'''                       (last tested: 0.9.4)
* '''0.8.x'''                       (last tested: 0.8.54)
* '''0.7.x >= 0.7.44'''             (last tested: 0.7.68)

Earlier versions of Nginx like 0.6.x and 0.5.x will ''not'' work.




= TODO =


= Getting involved =

You'll be very welcomed to submit patches to the [[#Author|author]] or just ask for a commit bit to the [[#Source Repository|source repository]] on GitHub.

= Authors =

* Peiyuan Feng ''<fengpeiyuan@gmail.com>''.


= Copyright & License =

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

= See Also =



