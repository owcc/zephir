
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
#include "expr.h"
#include "builder.h"
#include "kernel/main.h"

int zephir_statement_return(zephir_context *context, zval *statement TSRMLS_DC)
{

	zval            *expr;
	zephir_compiled_expr *compiled_expr;

	_zephir_array_fetch_string(&expr, statement, SS("expr") TSRMLS_CC);
	if (Z_TYPE_P(expr) != IS_ARRAY) {
		return 0;
	}

	compiled_expr = zephir_expr(context, expr TSRMLS_CC);
	switch (compiled_expr->type) {

		case ZEPHIR_T_INTEGER:
			zephir_build_return_long(context, compiled_expr->value);
			break;

		case ZEPHIR_T_VARIABLE:

			switch (compiled_expr->variable->type) {

				case ZEPHIR_T_TYPE_VAR:
					break;
			}

			break;

		default:
			zend_error(E_ERROR, "Unknown compiled expression %d", compiled_expr->type);
			break;
	}

	efree(compiled_expr);

	return 0;
}
