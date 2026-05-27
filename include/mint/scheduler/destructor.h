/**
 * Copyright (c) 2026 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MINT_SCHEDULER_DESTRUCTOR_H
#define MINT_SCHEDULER_DESTRUCTOR_H

#include "mint/ast/abstract_syntax_tree.h"
#include "mint/config.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/process.h"
#include "mint/memory/object.h"
#include <functional>

namespace mint {

class MINT_EXPORT Destructor : public Process {
public:
	Destructor(Object* object, const Reference& member, Class& owner, const Process* process = nullptr);
	Destructor(Object* object, const Reference& member, Class& owner, AbstractSyntaxTree& ast);
	Destructor(Object* object, const Reference& member, Class& owner, const Process& process);
	Destructor(Destructor&&) = delete;
	Destructor(const Destructor&) = delete;
	~Destructor() override;

	Destructor& operator=(Destructor&&) = delete;
	Destructor& operator=(const Destructor&) = delete;

	void setup() override;
	void cleanup() override;

private:
	std::reference_wrapper<Class> _owner;
	Object* _object;
	RootReference _member;
};

MINT_EXPORT bool is_destructor(Process& process);

}

#endif // MINT_SCHEDULER_DESTRUCTOR_H
