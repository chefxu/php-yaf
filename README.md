# Yaf - Yet Another Framework
[![Build Status](https://secure.travis-ci.org/laruence/php-yaf.png)](https://travis-ci.org/laruence/php-yaf)

PHP framework written in c and built as a PHP extension.

## 这是什么？
这个项目的目的是适配yaf框架与现有框架，在尽量不修改现有程序的基础上，享受yaf的高性能。
考虑到现有框架与yaf之间存在较大差异，主要适配点为：
- MVC目录结构差异很大，特别是Action是单文件形式， 无Controller结构， 与yaf加载的目录结构差异很大。(已完成)
- autoload加载机制不一致， 虽然yaf可以通过配置关闭内置的autoload而使用自定义的spl autoload， 但是会损失性能。（已完成）
- request，response方法适配， yaf提供的方法较少， 需要实现诸如request->get, request->getx等方法。（request完成）
- 现有框架的Config性能损失很大，可以学习yaf的方式， 将配置缓存到进程内存中， 检查文件时间戳来更新，能减少不少文件io，提高效率。

## 测试对比
- 现有框架
ab -n 10000 -c 200 http://192.168.1.18:7008/page

```
Server Software:        nginx/1.6.1
Server Hostname:        192.168.1.18
Server Port:            7008

Document Path:          /page
Document Length:        7332 bytes

Concurrency Level:      200
Time taken for tests:   18.144 seconds
Complete requests:      10000
Failed requests:        0
Write errors:           0
Total transferred:      75310000 bytes
HTML transferred:       73320000 bytes
Requests per second:    551.14 [#/sec] (mean)
Time per request:       362.882 [ms] (mean)
Time per request:       1.814 [ms] (mean, across all concurrent requests)
Transfer rate:          4053.38 [Kbytes/sec] received
```

- 适配yaf
ab -n 10000 -c 200 http://192.168.1.18:8000/page

```
Server Software:        nginx/1.6.1
Server Hostname:        192.168.1.18
Server Port:            6000

Document Path:          /page
Document Length:        4645 bytes

Concurrency Level:      200
Time taken for tests:   5.434 seconds
Complete requests:      10000
Failed requests:        0
Write errors:           0
Total transferred:      48724872 bytes
HTML transferred:       46454645 bytes
Requests per second:    1840.14 [#/sec] (mean)
Time per request:       108.688 [ms] (mean)
Time per request:       0.543 [ms] (mean, across all concurrent requests)
Transfer rate:          8755.90 [Kbytes/sec] received
```

## Requirement
- PHP 5.2 +

## Install
### Install Yaf
Yaf is an PECL extension, thus you can simply install it by:

```
$pecl install yaf
```
### Compile Yaf in Linux
```
$/path/to/phpize
$./configure --with-php-config=/path/to/php-config
$make && make install
```
### For windows
Yaf binary dlls could be found at http://code.google.com/p/yafphp/downloads/list

## Document
Yaf manual could be found at: http://www.php.net/manual/en/book.yaf.php

## IRC
efnet.org #php.yaf

## For IDE
you could find a documented prototype script here: https://github.com/elad-yosifon/php-yaf-doc

## Tutorial

### layout
A classic Application directory layout:

```
- .htaccess // Rewrite rules
+ public
  | - index.php // Application entry
  | + css
  | + js
  | + img
+ conf
  | - application.ini // Configure
- application/
  - Bootstrap.php   // Bootstrap
  + controllers
     - Index.php // Default controller
  + views
     |+ index
        - index.phtml // View template for default controller
  - library
  - models  // Models
  - plugins // Plugins
```
### DocumentRoot
you should set DocumentRoot to application/public, thus only the public folder can be accessed by user

### index.php
index.php in the public directory is the only way in of the application, you should rewrite all request to it(you can use .htaccess in Apache+php mod)

```php
<?php
define("APPLICATION_PATH",  dirname(dirname(__FILE__)));

$app  = new Yaf_Application(APPLICATION_PATH . "/conf/application.ini");
$app->bootstrap() //call bootstrap methods defined in Bootstrap.php
    ->run();
```
### Rewrite rules

#### Apache

```conf
#.htaccess
RewriteEngine On
RewriteCond %{REQUEST_FILENAME} !-f
RewriteRule .* index.php
```

#### Nginx

```
server {
  listen ****;
  server_name  domain.com;
  root   document_root;
  index  index.php index.html index.htm;

  if (!-e $request_filename) {
    rewrite ^/(.*)  /index.php/$1 last;
  }
}
```

#### Lighttpd

```
$HTTP["host"] =~ "(www.)?domain.com$" {
  url.rewrite = (
     "^/(.+)/?$"  => "/index.php/$1",
  )
}
```

### application.ini
application.ini is the application config file
```ini
[product]
;CONSTANTS is supported
application.directory = APP_PATH "/application/"
```
alternatively, you can use a PHP array instead:
```php
<?php
$config = array(
   "application" => array(
       "directory" => application_path . "/application/",
    ),
);

$app  = new yaf_application($config);
....

```
### default controller
In Yaf, the default controller is named IndexController:

```php
<?php
class IndexController extends Yaf_Controller_Abstract {
   // default action name
   public function indexAction() {
        $this->getView()->content = "Hello World";
   }
}
?>
```

###view script
The view script for default controller and default action is in the application/views/index/index.phtml, Yaf provides a simple view engineer called "Yaf_View_Simple", which supported the view template written by PHP.

```php
<html>
 <head>
   <title>Hello World</title>
 </head>
 <body>
   <?php echo $content; ?>
 </body>
</html>
```

## Run the Applicatioin
  http://www.yourhostname.com/

## Alternative
you can generate the example above by using Yaf Code Generator:  https://github.com/laruence/php-yaf/tree/master/tools/cg

## More
More info could be found at Yaf Manual: http://www.php.net/manual/en/book.yaf.php
