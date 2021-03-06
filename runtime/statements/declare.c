
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
#include "symtable.h"

#include "kernel/main.h"

int zephir_statement_declare(zephir_context *context, zval *statement TSRMLS_DC)
{
	zval *data_type, *variables, **variable, *name;
	HashTable       *ht;
	HashPosition    pos = {0};
	zephir_variable *symbol;

	_zephir_array_fetch_string(&data_type, statement, SS("data-type") TSRMLS_CC);
	if (Z_TYPE_P(data_type) != IS_STRING) {
		return 0;
	}

	_zephir_array_fetch_string(&variables, statement, SS("variables") TSRMLS_CC);
	if (Z_TYPE_P(variables) != IS_ARRAY) {
		return 0;
	}

	ht = Z_ARRVAL_P(variables);
	zend_hash_internal_pointer_reset_ex(ht, &pos);
	for (
	 ; zend_hash_get_current_data_ex(ht, (void**) &variable, &pos) == SUCCESS
	 ; zend_hash_move_forward_ex(ht, &pos)
	) {

		_zephir_array_fetch_string(&name, *variable, SS("variable") TSRMLS_CC);
		if (Z_TYPE_P(name) != IS_STRING) {
			return 0;
		}

		if (zephir_symtable_has(Z_STRVAL_P(name), Z_STRLEN_P(name), context)) {
			zend_error(E_ERROR, "Variable \"%s\" is already defined", Z_STRVAL_P(name));
		}

		if (!memcmp(Z_STRVAL_P(data_type), SS("int"))) {
			symbol = zephir_symtable_add(ZEPHIR_T_TYPE_INTEGER, Z_STRVAL_P(name), Z_STRLEN_P(name), context);
			symbol->value_ref = LLVMBuildAlloca(context->builder, LLVMInt32Type(), Z_STRVAL_P(name));
			continue;
		}

		zend_print_zval_r(*variable, 0 TSRMLS_CC);

	}


	return 0;
}
