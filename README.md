# zan-stats

[![License](https://img.shields.io/badge/license-apache2-blue.svg)](LICENSE)
[![Build Status](https://api.travis-ci.org/imaben/zan-stats.svg)](https://travis-ci.org/imaben/zan-stats)

```
_____              _____ __        __
/__  /  ____ _____ / ___// /_____ _/ /______
  / /  / __ `/ __ \\__ \/ __/ __ `/ __/ ___/
 / /__/ /_/ / / / /__/ / /_/ /_/ / /_(__  )
/____/\__,_/_/ /_/____/\__/\__,_/\__/____/

```

![](http://imaben.github.io/img/zan-stats.png)

## Build

```
$ git clone https://github.com/imaben/zan-stats.git
$ cd zan-stats
$ ./autogen.sh
$ ./configure
$ make
$ ./zan-stats <url>
```

## Usage

```
zan-stats [options] <url>

options:
 -n <secs> seconds to wait between updates
 -h        show helper
 -v        show version
 ```

**Server**

```php
$http = new swoole_http_server('0.0.0.0', 10900);
$http->on('request', function ($request, $response) use ($http) {
    if ($request->server['request_uri'] == '/stats') {
        $response->write(json_encode($http->stats()));
        return;
    }
});

```

**Client**

```shell
./zan-stats http://127.0.0.1:10900/stats
```
 
 ## Dependencies
 
 - libcurl
 - ncurses


