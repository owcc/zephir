
/*
 +--------------------------------------------------------------------------+
 | Zephir Language                                                          |
 +--------------------------------------------------------------------------+
 | Copyright (c) 2013-2014 Zephir Team and contributors                          |
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
#include "scanner.h"
#include "zephir.h"
#include "utils.h"
#include "classes.h"

#include <ext/standard/info.h>
#include <main/php_streams.h>
#include <ext/standard/file.h>
#include <ext/standard/php_filestat.h>

#include <Zend/zend_operators.h>
#include <Zend/zend_exceptions.h>
#include <Zend/zend_interfaces.h>

#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/ExecutionEngine.h>

ZEND_DECLARE_MODULE_GLOBALS(zephir);

zend_op_array *(*zephir_orig_compile_file)(zend_file_handle *file_handle, int type TSRMLS_DC);

/**
 * Initialize globals on each request or each thread started
 */
static void php_zephirt_init_globals(zend_zephir_globals *zephir_globals TSRMLS_DC)
{

	/* Memory options */
	zephir_globals->active_memory = NULL;

	/* Virtual Symbol Tables */
	zephir_globals->active_symbol_table = NULL;

	/* Recursive Lock */
	zephir_globals->recursive_lock = 0;

	/* LLVM Module */
	zephir_globals->module = NULL;
}

static void zephir_compile_program(zval *program TSRMLS_DC)
{
	HashTable           *ht = Z_ARRVAL_P(program);
	HashPosition        pos = {0};
	zval                **z, *type;
	char                *msg;
	zephir_context      *context;

	if (!ZEPHIRT_GLOBAL(module)) {

		ZEPHIRT_GLOBAL(module) = LLVMModuleCreateWithName("zephir");
		ZEPHIRT_GLOBAL(builder) = LLVMCreateBuilder();

		LLVMInitializeNativeTarget();
		LLVMLinkInJIT();

		if (LLVMCreateExecutionEngineForModule(&ZEPHIRT_GLOBAL(engine), ZEPHIRT_GLOBAL(module), &msg) == 1) {
			fprintf(stderr, "%s\n", msg);
			LLVMDisposeMessage(msg);
			return;
		}

		ZEPHIRT_GLOBAL(pass_manager) =  LLVMCreateFunctionPassManagerForModule(ZEPHIRT_GLOBAL(module));
		LLVMAddTargetData(LLVMGetExecutionEngineTargetData(ZEPHIRT_GLOBAL(engine)), ZEPHIRT_GLOBAL(pass_manager));
	}

	context = emalloc(sizeof(zephir_context));
	context->module  = ZEPHIRT_GLOBAL(module);
	context->engine  = ZEPHIRT_GLOBAL(engine);
	context->builder = ZEPHIRT_GLOBAL(builder);
	context->types.zval_type = NULL;

	zend_hash_internal_pointer_reset_ex(ht, &pos);
	for (
	 ; zend_hash_get_current_data_ex(ht, (void**) &z, &pos) == SUCCESS
	 ; zend_hash_move_forward_ex(ht, &pos)
	) {

		_zephir_array_fetch_string(&type, *z, "type", strlen("type") + 1 TSRMLS_CC);
		if (Z_TYPE_P(type) != IS_STRING) {
			continue;
		}

		if (!memcmp(Z_STRVAL_P(type), "class", strlen("class") + 1)) {
			zephir_compile_class(context, *z);
		}
	}

	efree(context);
}

static void zephir_parse_file(char *file_name TSRMLS_DC)
{
	char *contents;
	php_stream *stream;
	int len;
	long maxlen = PHP_STREAM_COPY_ALL;
	zval *zcontext = NULL, *return_value = NULL;
	php_stream_context *context = NULL;

	context = php_stream_context_from_zval(zcontext, 0);

	stream = php_stream_open_wrapper_ex(file_name, "rb", 0 | REPORT_ERRORS, NULL, context);
	if (!stream) {
		return;
	}

	if ((len = php_stream_copy_to_mem(stream, &contents, maxlen, 0)) > 0) {

		zephir_parse_program(&return_value, contents, len, file_name TSRMLS_CC);
		efree(contents);

		zephir_compile_program(return_value TSRMLS_CC);

		zval_ptr_dtor(&return_value);
	}

	php_stream_close(stream);
}

static zend_op_array *zephir_compile_file(zend_file_handle *file_handle, int type TSRMLS_DC) /* {{{ */
{
	zend_op_array *res;
	int failed;

	if (file_handle->filename) {
		if (strstr(file_handle->filename, ".zep") && !strstr(file_handle->filename, "://")) {
			zephir_parse_file(file_handle->filename TSRMLS_CC);
			return NULL;
		}
	}

	zend_try {
		failed = 0;
		res = zephir_orig_compile_file(file_handle, type TSRMLS_CC);
	} zend_catch {
		failed = 1;
		res = NULL;
	} zend_end_try();

	if (failed) {
		zend_bailout();
	}

	return res;
}

PHP_MINIT_FUNCTION(zephir){

	zephir_orig_compile_file = zend_compile_file;
	zend_compile_file = zephir_compile_file;

	return SUCCESS;
}

static PHP_MSHUTDOWN_FUNCTION(zephir){

	if (ZEPHIRT_GLOBAL(module)) {

		LLVMDumpModule(ZEPHIRT_GLOBAL(module));

		LLVMDisposePassManager(ZEPHIRT_GLOBAL(pass_manager));
		LLVMDisposeBuilder(ZEPHIRT_GLOBAL(builder));
		LLVMDisposeModule(ZEPHIRT_GLOBAL(module));

		ZEPHIRT_GLOBAL(module) = NULL;
	}

	return SUCCESS;
}

static PHP_RINIT_FUNCTION(zephir){

	zend_zephir_globals *zephir_globals_ptr	= ZEPHIRT_VGLOBAL;

	php_zephirt_init_globals(zephir_globals_ptr TSRMLS_CC);

	zephir_globals_ptr->fcache = pemalloc(sizeof(HashTable), 1);
	zend_hash_init(zephir_globals_ptr->fcache, 128, NULL, NULL, 1);

	return SUCCESS;
}

static PHP_RSHUTDOWN_FUNCTION(zephir){

	return SUCCESS;
}

static PHP_MINFO_FUNCTION(zephir)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "Version", PHP_ZEPHIR_VERSION);
	php_info_print_table_end();
}

static PHP_GINIT_FUNCTION(zephir)
{

}

static PHP_GSHUTDOWN_FUNCTION(zephir)
{

}

zend_module_entry zephir_module_entry = {
	STANDARD_MODULE_HEADER_EX,
	NULL,
	NULL,
	PHP_ZEPHIR_EXTNAME,
	NULL,
	PHP_MINIT(zephir),
	PHP_MSHUTDOWN(zephir),
	PHP_RINIT(zephir),
	PHP_RSHUTDOWN(zephir),
	PHP_MINFO(zephir),
	PHP_ZEPHIR_VERSION,
	ZEND_MODULE_GLOBALS(zephir),
	PHP_GINIT(zephir),
	PHP_GSHUTDOWN(zephir),
	NULL,
	STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_ZEPHIR
ZEND_GET_MODULE(zephir)
#endif
