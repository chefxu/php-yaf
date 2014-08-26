/*
  +----------------------------------------------------------------------+
  | Yet Another Framework                                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Xinchen Hui  <laruence@php.net>                              |
  +----------------------------------------------------------------------+
*/

/* $Id: http.c 329197 2013-01-18 05:55:37Z laruence $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "main/SAPI.h"
#include "ext/standard/url.h" /* for php_url */

#include "php_yaf.h"
#include "yaf_namespace.h"
#include "yaf_request.h"
#include "yaf_exception.h"

#include "requests/yaf_request_http.h"

static zend_class_entry * yaf_request_http_ce;

/** {{{ yaf_request_t * yaf_request_http_instance(yaf_request_t *this_ptr, char *request_uri, char *base_uri TSRMLS_DC)
*/
yaf_request_t * yaf_request_http_instance(yaf_request_t *this_ptr, char *request_uri, char *base_uri TSRMLS_DC) {
	yaf_request_t *instance;
	zval *method, *params, *settled_uri = NULL;

	if (this_ptr) {
		instance = this_ptr;
	} else {
		MAKE_STD_ZVAL(instance);
		object_init_ex(instance, yaf_request_http_ce);
	}

	MAKE_STD_ZVAL(method);
	if (SG(request_info).request_method) {
		ZVAL_STRING(method, (char *)SG(request_info).request_method, 1);
	} else if (strncasecmp(sapi_module.name, "cli", 3)) {
		ZVAL_STRING(method, "Unknow", 1);
	} else {
		ZVAL_STRING(method, "Cli", 1);
	}

	zend_update_property(yaf_request_http_ce, instance, ZEND_STRL(YAF_REQUEST_PROPERTY_NAME_METHOD), method TSRMLS_CC);
	zval_ptr_dtor(&method);

	if (request_uri) {
		MAKE_STD_ZVAL(settled_uri);
		ZVAL_STRING(settled_uri, request_uri, 1);
	} else {
		zval *uri;
		do {
#ifdef PHP_WIN32
			/* check this first so IIS will catch */
			uri = yaf_request_query(YAF_GLOBAL_VARS_SERVER, ZEND_STRL("HTTP_X_REWRITE_URL") TSRMLS_CC);
			if (Z_TYPE_P(uri) != IS_NULL) {
				settled_uri = uri;
				break;
			}
			zval_ptr_dtor(&uri);

			/* IIS7 with URL Rewrite: make sure we get the unencoded url (double slash problem) */
			uri = yaf_request_query(YAF_GLOBAL_VARS_SERVER, ZEND_STRL("IIS_WasUrlRewritten") TSRMLS_CC);
			if (Z_TYPE_P(uri) != IS_NULL) {
				zval *rewrited = yaf_request_query(YAF_GLOBAL_VARS_SERVER, ZEND_STRL("IIS_WasUrlRewritten") TSRMLS_CC);
				zval *unencode = yaf_request_query(YAF_GLOBAL_VARS_SERVER, ZEND_STRL("UNENCODED_URL") TSRMLS_CC);
				if (Z_TYPE_P(rewrited) == IS_LONG
						&& Z_LVAL_P(rewrited) == 1
						&& Z_TYPE_P(unencode) == IS_STRING
						&& Z_STRLEN_P(unencode) > 0) {
					settled_uri = uri;
				}
				break;
			}
			zval_ptr_dtor(&uri);
#endif
			uri = yaf_request_query(YAF_GLOBAL_VARS_SERVER, ZEND_STRL("PATH_INFO") TSRMLS_CC);
			if (Z_TYPE_P(uri) != IS_NULL) {
				settled_uri = uri;
				break;
			}
			zval_ptr_dtor(&uri);

			uri = yaf_request_query(YAF_GLOBAL_VARS_SERVER, ZEND_STRL("REQUEST_URI") TSRMLS_CC);
			if (Z_TYPE_P(uri) != IS_NULL) {
				/* Http proxy reqs setup request uri with scheme and host [and port] + the url path, only use url path */
				if (strstr(Z_STRVAL_P(uri), "http") == Z_STRVAL_P(uri)) {
					php_url *url_info = php_url_parse(Z_STRVAL_P(uri));
					zval_ptr_dtor(&uri);

					if (url_info && url_info->path) {
						MAKE_STD_ZVAL(settled_uri);
						ZVAL_STRING(settled_uri, url_info->path, 1);
					}

					php_url_free(url_info);
				} else {
					char *pos  = NULL;
					if ((pos = strstr(Z_STRVAL_P(uri), "?"))) {
						MAKE_STD_ZVAL(settled_uri);
						ZVAL_STRINGL(settled_uri, Z_STRVAL_P(uri), pos - Z_STRVAL_P(uri), 1);
						zval_ptr_dtor(&uri);
					} else {
						settled_uri = uri;
					}
				}
				break;
			}
			zval_ptr_dtor(&uri);

			uri = yaf_request_query(YAF_GLOBAL_VARS_SERVER, ZEND_STRL("ORIG_PATH_INFO") TSRMLS_CC);
			if (Z_TYPE_P(uri) != IS_NULL) {
				/* intended do nothing */
				/* zval *query = yaf_request_query(YAF_GLOBAL_VARS_SERVER, ZEND_STRL("QUERY_STRING") TSRMLS_CC);
				if (Z_TYPE_P(query) != IS_NULL) {
				}
				*/
				settled_uri = uri;
				break;
			}
			zval_ptr_dtor(&uri);

		} while (0);
	}

	if (settled_uri) {
		char *p = Z_STRVAL_P(settled_uri);

		while (*p == '/' && *(p + 1) == '/') {
			p++;
		}

		if (p != Z_STRVAL_P(settled_uri)) {
			char *garbage = Z_STRVAL_P(settled_uri);
			ZVAL_STRING(settled_uri, p, 1);
			efree(garbage);
		}

		zend_update_property(yaf_request_http_ce, instance, ZEND_STRL(YAF_REQUEST_PROPERTY_NAME_URI), settled_uri TSRMLS_CC);
		yaf_request_set_base_uri(instance, base_uri, Z_STRVAL_P(settled_uri) TSRMLS_CC);
		zval_ptr_dtor(&settled_uri);
	}

	MAKE_STD_ZVAL(params);
	array_init(params);
	zend_update_property(yaf_request_http_ce, instance, ZEND_STRL(YAF_REQUEST_PROPERTY_NAME_PARAMS), params TSRMLS_CC);
	zval_ptr_dtor(&params);

	return instance;
}
/* }}} */

/** {{{ proto public Yaf_Request_Http::getQuery(mixed $name, mixed $default = NULL)
*/
YAF_REQUEST_METHOD(yaf_request_http, Query, 	YAF_GLOBAL_VARS_GET);
/* }}} */

/** {{{ proto public Yaf_Request_Http::getPost(mixed $name, mixed $default = NULL)
*/
YAF_REQUEST_METHOD(yaf_request_http, Post,  	YAF_GLOBAL_VARS_POST);
/* }}} */

/** {{{ proto public Yaf_Request_Http::getRequet(mixed $name, mixed $default = NULL)
*/
YAF_REQUEST_METHOD(yaf_request_http, Request, YAF_GLOBAL_VARS_REQUEST);
/* }}} */

/** {{{ proto public Yaf_Request_Http::getFiles(mixed $name, mixed $default = NULL)
*/
YAF_REQUEST_METHOD(yaf_request_http, Files, 	YAF_GLOBAL_VARS_FILES);
/* }}} */

/** {{{ proto public Yaf_Request_Http::getEnv(mixed $name, mixed $default = NULL)
*/
YAF_REQUEST_METHOD(yaf_request_http, Env, 	YAF_GLOBAL_VARS_ENV);
/* }}} */

/** {{{ proto public Yaf_Request_Http::getServer(mixed $name, mixed $default = NULL)
*/
YAF_REQUEST_METHOD(yaf_request_http, Server, 	YAF_GLOBAL_VARS_SERVER);
/* }}} */

/** {{{ proto public Yaf_Request_Http::getCookie(mixed $name, mixed $default = NULL)
*/
YAF_REQUEST_METHOD(yaf_request_http, Cookie, 	YAF_GLOBAL_VARS_COOKIE);
/* }}} */

/** {{{ proto public Yaf_Request_Http::isXmlHttpRequest()
*/
PHP_METHOD(yaf_request_http, isXmlHttpRequest) {
	zval * header = yaf_request_query(YAF_GLOBAL_VARS_SERVER, ZEND_STRL("HTTP_X_REQUESTED_WITH") TSRMLS_CC);
	if (Z_TYPE_P(header) == IS_STRING
			&& strncasecmp("XMLHttpRequest", Z_STRVAL_P(header), Z_STRLEN_P(header)) == 0) {
		zval_ptr_dtor(&header);
		RETURN_TRUE;
	}
	zval_ptr_dtor(&header);
	RETURN_FALSE;
}
/* }}} */

/** {{{ proto public Yaf_Request_Http::get(mixed $name, mixed $default)
 * params -> post -> get -> cookie -> server
 */
PHP_METHOD(yaf_request_http, get) {
	char	*name 	= NULL;
	int 	len	 	= 0;
	zval 	*def 	= NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z", &name, &len, &def) == FAILURE) {
		WRONG_PARAM_COUNT;
	} else {
		zval *value = yaf_request_get_param(getThis(), name, len TSRMLS_CC);
		if (value) {
			RETURN_ZVAL(value, 1, 0);
		} else {
			zval *params	= NULL;
			zval **ppzval	= NULL;

			YAF_GLOBAL_VARS_TYPE methods[4] = {
				YAF_GLOBAL_VARS_GET,
				YAF_GLOBAL_VARS_POST,
				YAF_GLOBAL_VARS_COOKIE,
				YAF_GLOBAL_VARS_SERVER
			};

			{
				int i = 0;
				for (;i<4; i++) {
					params = PG(http_globals)[methods[i]];
					if (params && Z_TYPE_P(params) == IS_ARRAY) {
						if (zend_hash_find(Z_ARRVAL_P(params), name, len + 1, (void **)&ppzval) == SUCCESS ){
							RETURN_ZVAL(*ppzval, 1, 0);
						}
					}
				}

			}
			if (def) {
				RETURN_ZVAL(def, 1, 0);
			}
		}
	}
	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::header(string $name, $mixed $default = NULL)
*/
PHP_METHOD(yaf_request_http, header) {
	char *name;
	uint len;
	zval *def = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|z", &name, &len, &def) == FAILURE) {
		return;
	} else {
		zval **value = NULL;
		zval *params = NULL;
		char *key = NULL;
		uint key_len = 0;
		int i = 0;
		key_len = spprintf(&key, 0, "HTTP_%s", name);
		while(i < key_len && key[i] != '\0') {
			*(key+i) = toupper(*(key+i));
			if (*(key+i) == '-') *(key+i) = '_';
			i++;
		}

		params = PG(http_globals)[YAF_GLOBAL_VARS_SERVER];
		if (params && Z_TYPE_P(params) == IS_ARRAY) {
			if (zend_hash_find(Z_ARRVAL_P(params), key, key_len + 1, (void **)&value) == SUCCESS ){
				RETURN_ZVAL(*value, 1, 0);
			}
		}
		efree(key);

		if (def) {
			RETURN_ZVAL(def, 1, 0);
		}
	}

	RETURN_NULL();
}
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::getx(void)
*/
YAF_REQUEST_VBOX_METHOD(yaf_request_http, getx, YAF_GLOBAL_VARS_GET);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::postx(void)
*/
YAF_REQUEST_VBOX_METHOD(yaf_request_http, postx, YAF_GLOBAL_VARS_POST);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::cookiex(void)
*/
YAF_REQUEST_VBOX_METHOD(yaf_request_http, cookiex, YAF_GLOBAL_VARS_COOKIE);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::serverx(void)
*/
YAF_REQUEST_VBOX_METHOD(yaf_request_http, serverx, YAF_GLOBAL_VARS_SERVER);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::headerx(void)
*/
YAF_REQUEST_VBOX_METHOD(yaf_request_http, headerx, YAF_GLOBAL_VARS_POST);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::envx(void)
*/
YAF_REQUEST_VBOX_METHOD(yaf_request_http, envx, YAF_GLOBAL_VARS_ENV);
/* }}} */

/** {{{ proto public Yaf_Request_Abstract::envx(void)
*/
YAF_REQUEST_VBOX_METHOD(yaf_request_http, filex, YAF_GLOBAL_VARS_FILES);
/* }}} */

/** {{{ proto public Yaf_Request_Http::__construct(string $request_uri, string $base_uri)
*/
PHP_METHOD(yaf_request_http, __construct) {
	char *request_uri = NULL;
	char *base_uri	  = NULL;
	int  rlen		  = 0;
	int  blen 		  = 0;

	yaf_request_t *self = getThis();
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|ss", &request_uri, &rlen, &base_uri, &blen) == FAILURE) {
		YAF_UNINITIALIZED_OBJECT(getThis());
		return;
	}

	(void)yaf_request_http_instance(self, request_uri, base_uri TSRMLS_CC);
}
/* }}} */

/** {{{ proto private Yaf_Request_Http::__clone
 */
PHP_METHOD(yaf_request_http, __clone) {
}
/* }}} */

/** {{{ yaf_request_http_methods
 */
zend_function_entry yaf_request_http_methods[] = {
	PHP_ME(yaf_request_http, getQuery, 		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request_http, getRequest, 		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request_http, getPost, 		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request_http, getCookie,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request_http, getFiles,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request_http, get,			NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request_http, isXmlHttpRequest, 	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request_http, __construct,		NULL, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(yaf_request_http, __clone,		NULL, ZEND_ACC_PRIVATE | ZEND_ACC_CLONE)

	/* {{{ add method for wanhuatong */
	PHP_ME(yaf_request_http, postx, 		NULL, 		ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request_http, getx, 		NULL, 		ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request_http, serverx,	NULL, 		ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request_http, cookiex,	NULL, 		ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request_http, envx, 		NULL, 		ZEND_ACC_PUBLIC)
	PHP_ME(yaf_request_http, header, 	NULL, 	ZEND_ACC_PUBLIC)
	PHP_MALIAS(yaf_request_http,   post,     	getPost,	NULL, 	ZEND_ACC_PUBLIC)
	PHP_MALIAS(yaf_request_http,   cookie,      getCookie,	NULL, 	ZEND_ACC_PUBLIC)
	PHP_MALIAS(yaf_request_http,   file,        getFiles,	NULL, 	ZEND_ACC_PUBLIC)
	PHP_MALIAS(yaf_request_http,   server,      getServer,	NULL, 	ZEND_ACC_PUBLIC)
	PHP_MALIAS(yaf_request_http,   env,         getEnv,		NULL, 	ZEND_ACC_PUBLIC)
	/* }}} */
	{NULL, NULL, NULL}
};
/* }}} */

/** {{{ YAF_STARTUP_FUNCTION
 */
YAF_STARTUP_FUNCTION(request_http){
	zend_class_entry ce;
	YAF_INIT_CLASS_ENTRY(ce, "Yaf_Request_Http", "Yaf\\Request\\Http", yaf_request_http_methods);
	yaf_request_http_ce = zend_register_internal_class_ex(&ce, yaf_request_ce, NULL TSRMLS_CC);

	zend_declare_class_constant_string(yaf_request_ce, ZEND_STRL("SCHEME_HTTP"), "http" TSRMLS_CC);
	zend_declare_class_constant_string(yaf_request_ce, ZEND_STRL("SCHEME_HTTPS"), "https" TSRMLS_CC);

	return SUCCESS;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
