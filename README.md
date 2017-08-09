# zan-stats

```
_____              _____ __        __
/__  /  ____ _____ / ___// /_____ _/ /______
  / /  / __ `/ __ \\__ \/ __/ __ `/ __/ ___/
 / /__/ /_/ / / / /__/ / /_/ /_/ / /_(__  )
/____/\__,_/_/ /_/____/\__/\__,_/\__/____/

```

## Build

```
$ git clone https://github.com/imaben/zan-stats.git
$ cd zan-stats
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
./zan-stats http://127.0.0.1/stats
```
 
 ## Dependencies
 
 - libcurl
 - ncurses


