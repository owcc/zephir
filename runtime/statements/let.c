
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

int zephir_statement_let_variable(zephir_context *context, zval *assignment, zval *statement TSRMLS_DC) {

	zval *expr, *variable;
	zephir_compiled_expr *compiled_expr;
	zephir_variable *symbol_variable;

	_zephir_array_fetch_string(&expr, assignment, SS("expr") TSRMLS_CC);
	if (Z_TYPE_P(expr) != IS_ARRAY) {
		return 0;
	}

	_zephir_array_fetch_string(&variable, assignment, SS("variable") TSRMLS_CC);
	if (Z_TYPE_P(variable) != IS_STRING) {
		return 0;
	}

	compiled_expr = zephir_expr(context, expr TSRMLS_CC);
	symbol_variable = zephir_symtable_get_variable_for_write(context->symtable, Z_STRVAL_P(variable), Z_STRLEN_P(variable));

	switch (symbol_variable->type) {

		case ZEPHIR_T_TYPE_INTEGER:

			switch (compiled_expr->type) {

				case ZEPHIR_T_INTEGER:
					LLVMBuildStore(context->builder, compiled_expr->value, symbol_variable->value_ref);
					break;

			}

			break;

	}

	//zend_print_zval_r(assignment, 0 TSRMLS_CC);

	efree(compiled_expr);
	return 0;
}

int zephir_statement_let(zephir_context *context, zval *statement TSRMLS_DC)
{
	HashTable       *ht;
	HashPosition    pos = {0};
	zval *assigments, **assignment, *assign_type;

	_zephir_array_fetch_string(&assigments, statement, SS("assignments") TSRMLS_CC);
	if (Z_TYPE_P(assigments) != IS_ARRAY) {
		return 0;
	}

	ht = Z_ARRVAL_P(assigments);
	zend_hash_internal_pointer_reset_ex(ht, &pos);
	for (
	 ; zend_hash_get_current_data_ex(ht, (void**) &assignment, &pos) == SUCCESS
	 ; zend_hash_move_forward_ex(ht, &pos)
	) {

		_zephir_array_fetch_string(&assign_type, *assignment, SS("assign-type") TSRMLS_CC);
		if (Z_TYPE_P(assign_type) != IS_STRING) {
			return 0;
		}

		if (!memcmp(Z_STRVAL_P(assign_type), SS("variable"))) {
			zephir_statement_let_variable(context, *assignment, statement);
			continue;
		}

	}

	//zend_print_zval_r(statement, 0 TSRMLS_CC);
	return 0;
}
