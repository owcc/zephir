
/*
 +--------------------------------------------------------------------------+
 | Zephir Language                                                          |
 +--------------------------------------------------------------------------+
 | Copyright (c) 2013-2014 Zephir Team and contributors                     |
 +--------------------------------------------------------------------------+
 | This source file is subject the MIT license, that is bundled with        |
 | this package in the file LICENSE, and is available through the           |
 | world-wide-web at the following url:                                     |
 | http://zephir-lang.com/license.html                                      |
 |                                                                          |
 | If you did not receive a copy of the MIT license and are unable          |
 | to obtain it through the world-wide-web, please send a note to           |
 | license@zephir-lang.com so we can mail you a copy immediately.           |
 +--------------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <php.h>
#include "php_zephir.h"
#include "zephir.h"
#include "utils.h"

#include "kernel/main.h"

#include "statements/echo.h"
#include "statements/if.h"
#include "statements/let.h"
#include "statements/declare.h"
#include "statements/return.h"

LLVMValueRef zephir_compile_block(zephir_context *context, zval *statements TSRMLS_DC)
{
	HashTable       *ht = Z_ARRVAL_P(statements);
	HashPosition    pos = {0};
	zval            **statement, *type;

	zend_hash_internal_pointer_reset_ex(ht, &pos);
	for (
	 ; zend_hash_get_current_data_ex(ht, (void**) &statement, &pos) == SUCCESS
	 ; zend_hash_move_forward_ex(ht, &pos)
	) {

		_zephir_array_fetch_string(&type, *statement, SS("type") TSRMLS_CC);
		if (Z_TYPE_P(type) != IS_STRING) {
			continue;
		}

		if (!memcmp(Z_STRVAL_P(type), SS("if"))) {
			zephir_statement_if(context, *statement);
			continue;
		}

		if (!memcmp(Z_STRVAL_P(type), SS("echo"))) {
			zephir_statement_echo(context, *statement);
			continue;
		}

		if (!memcmp(Z_STRVAL_P(type), SS("let"))) {
			zephir_statement_let(context, *statement);
			continue;
		}

		if (!memcmp(Z_STRVAL_P(type), SS("declare"))) {
			zephir_statement_declare(context, *statement);
			continue;
		}

		if (!memcmp(Z_STRVAL_P(type), SS("return"))) {
			zephir_statement_return(context, *statement);
			continue;
		}

		zend_print_zval_r(*statement, 0 TSRMLS_CC);
		continue;
	}

	return NULL;
}
