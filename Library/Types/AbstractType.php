<?php

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

namespace Zephir\Types;

use Zephir\Call;
use Zephir\CompilationContext;
use Zephir\Expression;
use Zephir\CompilerException;

abstract class AbstractType
{
    /**
     * Intercepts calls to built-in methods
     *
     * @param string $methodName
     * @param object $caller
     * @param CompilationContext $compilationContext
     * @param Call $call
     * @param array $expression
     * @throws CompilerException
     * @return
     */
    public function invokeMethod($methodName, $caller, CompilationContext $compilationContext, Call $call, array $expression)
    {
        if (method_exists($this, $methodName)) {
            return $this->{$methodName}($caller, $compilationContext, $call, $expression);
        }

        throw new CompilerException(sprintf('Method "%s" is not a built-in method of type "%s"', $methodName, $this->getTypeName()), $expression);
    }

    /**
     * @return string The name of the type
     */
    abstract public function getTypeName();
}