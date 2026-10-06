#include "mint/compiler/build_context.h"
#include "mint/compiler/compiler.h"
#include "mint/program/module.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/buffer_stream.h"
#include "mint/system/mint_runtime_error.h"
#include <gtest/gtest.h>
#include <optional>
#include <string>

namespace {

/*
 * Compile `source` in a fresh module. Returns the error message if the
 * compilation was rejected, std::nullopt otherwise.
 */
std::optional<std::string> compile(const std::string& source) {
	auto scheduler = mint::Scheduler();
	const auto process = scheduler.enable_testing();

	auto stream = mint::BufferStream(source);
	auto& module = scheduler.program().create_module({"test"}, mint::Module::State::ready);
	auto compiler = mint::Compiler(scheduler.program(), module);

	try {
		compiler.build(stream);
	}
	catch (const mint::MintRuntimeError& error) {
		return std::string(error.what());
	}
	return std::nullopt;
}

}

// Macros (and not functions) so that GTest reports the line of the calling test.

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_COMPILES(source) \
	do { \
		const auto error_message = compile(source); \
		EXPECT_FALSE(error_message.has_value()) << error_message.value_or(""); \
	} \
	while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXPECT_COMPILE_ERROR(source, fragment) \
	do { \
		const auto error_message = compile(source); \
		ASSERT_TRUE(error_message.has_value()) << "expected a compilation error"; \
		EXPECT_NE(error_message->find(fragment), std::string::npos) << *error_message; \
	} \
	while (false)

/* ------------------------------------------------------------------------ */
/* Calls to the parent implementation                                       */
/* ------------------------------------------------------------------------ */

TEST(build_context_calls, parent_call_with_explicit_self_is_accepted) {
	EXPECT_COMPILES(R"mn(class Foo {
    const def foo(self, value) {
        return value
    }
}

class Bar : Foo {
    override const def foo(self, value) {
        return Foo.foo(self, value)
    }
}
)mn");
}

TEST(build_context_calls, parent_call_without_self_reports_too_few_arguments) {
	EXPECT_COMPILE_ERROR(R"mn(class Foo {
    const def foo(self, value) {
        return value
    }
}

class Bar : Foo {
    override const def foo(self, value) {
        return Foo.foo(value)
    }
}
)mn",
	    "too few arguments");
}

TEST(build_context_calls, parent_call_with_extra_argument_reports_too_many_arguments) {
	EXPECT_COMPILE_ERROR(R"mn(class Foo {
    const def foo(self, value) {
        return value
    }
}

class Bar : Foo {
    override const def foo(self, value) {
        return Foo.foo(self, value, value)
    }
}
)mn",
	    "too many arguments");
}

TEST(build_context_calls, parent_call_error_reports_a_position) {
	EXPECT_COMPILE_ERROR(R"mn(class Foo {
    const def foo(self, value) {
        return value
    }
}

class Bar : Foo {
    override const def foo(self, value) {
        return Foo.foo(value)
    }
}
)mn",
	    "buffer:");
}

/* ------------------------------------------------------------------------ */
/* Calls on instances                                                       */
/* ------------------------------------------------------------------------ */

TEST(build_context_calls, instance_call_uses_implicit_self) {
	EXPECT_COMPILES(R"mn(class Foo {
    const def foo(self, value) {
        return value
    }
}

def test() {
    return Foo().foo(1)
}
)mn");
}

TEST(build_context_calls, instance_call_with_extra_argument_reports_too_many_arguments) {
	EXPECT_COMPILE_ERROR(R"mn(
@class Foo {
    const def foo(self, value) {
        return value
    }
}

def test() {
    return Foo().foo(1, 2)
}
)mn",
	    "too many arguments");
}

/* ------------------------------------------------------------------------ */
/* Global functions with a Self receiver                                    */
/* ------------------------------------------------------------------------ */

TEST(build_context_calls, global_function_receives_left_operand_as_self) {
	EXPECT_COMPILES(R"mn(@const def getTypeName(const Self) {
    return typeof Self
}

class Foo {}
class Bar : Foo {}

def test() {
    Foo.getTypeName()
    Bar.getTypeName()
}
)mn");
}

/* ------------------------------------------------------------------------ */
/* Constructors                                                             */
/* ------------------------------------------------------------------------ */

TEST(build_context_calls, constructor_calls_are_accepted) {
	EXPECT_COMPILES(R"mn(class Foo {
    const def new(self, value) {
        return self
    }
}

class Bar : Foo {}

class Baz : Foo {
    const def new(self, value) {
        Foo.new(self, value)
        return self
    }
}

def test() {
    Foo(1)
    Bar(2)
    Baz(3)
}
)mn");
}

TEST(build_context_calls, constructor_without_argument_reports_too_few_arguments) {
	EXPECT_COMPILE_ERROR(R"mn(
@class Foo {
    const def new(self, value) {
        return self
    }
}

def test() {
    return Foo()
}
)mn",
	    "too few arguments");
}

TEST(build_context_calls, parent_constructor_without_self_reports_too_few_arguments) {
	EXPECT_COMPILE_ERROR(R"mn(class Foo {
    const def new(self, value) {
        return self
    }
}

class Baz : Foo {
    const def new(self, value) {
        Foo.new(value)
        return self
    }
}
)mn",
	    "too few arguments");
}

/* ------------------------------------------------------------------------ */
/* Aliases                                                                  */
/* ------------------------------------------------------------------------ */

TEST(build_context_aliases, alias_of_class_behaves_like_the_class) {
	EXPECT_COMPILES(R"mn(
@class Foo {
    const def new(self, value) {
        return self
    }
    const def foo(self, value) {
        return value
    }
}

const Alias = Foo

def test() {
    Alias(1)
    Alias.foo(Foo(0), 5)
    Alias(2).foo(6)
}
)mn");
}

TEST(build_context_aliases, alias_call_without_self_reports_too_few_arguments) {
	EXPECT_COMPILE_ERROR(R"mn(class Foo {
    const def foo(self, value) {
        return value
    }
}

const Alias = Foo

def test() {
    return Alias.foo(1)
}
)mn",
	    "too few arguments");
}

TEST(build_context_aliases, alias_cannot_be_reassigned) {
	const auto error_message = compile(R"mn(class Foo {}
class Other {}

const Alias = Foo
Alias = Other
)mn");
	EXPECT_TRUE(error_message.has_value());
}

TEST(build_context_aliases, alias_used_before_its_target_is_declared) {
	EXPECT_COMPILES(R"mn(def test() {
    return Alias.foo(Foo(), 3)
}

const Alias = Foo

class Foo {
    const def foo(self, value) {
        return value
    }
}
)mn");
}

TEST(build_context_aliases, forward_reference_to_class_in_unbound_call) {
	EXPECT_COMPILES(R"mn(def test(value) {
    return Foo.foo(Foo(), value)
}

class Foo {
    const def foo(self, value) {
        return value
    }
}
)mn");
}

TEST(build_context_aliases, variable_holding_a_class_is_not_checked_statically) {
	EXPECT_COMPILES(R"mn(class Foo {
    const def foo(self, value) {
        return value
    }
}

def test() {
    var c = Foo
    return c.foo(Foo(), 4)
}
)mn");
}

// Run under ASAN: converting a variable into an alias must not leave
// dangling references in the symbol table.
TEST(build_context_aliases, variables_declared_after_an_alias_stay_valid) {
	EXPECT_COMPILES(R"mn(class Foo {}

const Alias = Foo
var x = 10
var y = 20

def test() {
    var a = 1
    const Local = Foo
    var b = 2
    return a + b
}
)mn");
}

/* ------------------------------------------------------------------------ */
/* Type narrowing                                                           */
/* ------------------------------------------------------------------------ */

/*
 * Rule: an unknown type is only compatible with another unknown type.
 *
 * `x` is an untyped parameter. Inside a narrowed branch its type is `Date`,
 * which a parameter typed `Base` accepts (Date derives from Base). Anywhere
 * else it is unknown, and the same parameter must reject it. So:
 *   - a call accepted inside the branch proves that the narrowing is applied;
 *   - a call rejected outside the branch proves that the narrowing did not leak.
 */

// Control: the narrowed type is accepted inside its branch.
TEST(build_context_narrowing, narrowed_branch_is_accepted_by_matching_parameter) {
	EXPECT_COMPILES(R"mn(
@class Base {}

@class Date : Base {
    const def seconds(const self) -> number {
        return 1
    }
}

@def takesBase(value: Base) -> number {
    return 0
}

def convert(x) -> number {
    if typeof x === 'Date' {
        return takesBase(x)
    }
    return 0
}
)mn");
}

// Regression: the narrowing of a `case` must not leak into `default`.
TEST(build_context_narrowing, switch_default_does_not_inherit_case_narrowing) {
	EXPECT_COMPILE_ERROR(R"mn(
@class Base {}

@class Date : Base {
    const def seconds(const self) -> number {
        return 1
    }
}

@def takesBase(value: Base) -> number {
    return 0
}

def convert(x) -> number {
    switch typeof x {
    case 'Date':
        return x.seconds()
    default:
        return takesBase(x)
    }
}
)mn",
	    "cannot pass");
}

TEST(build_context_narrowing, switch_default_after_several_cases_is_not_narrowed) {
	EXPECT_COMPILE_ERROR(R"mn(
@class Base {}

@class A : Base {
    const def a(const self) -> number {
        return 1
    }
}

@class B : Base {
    const def b(const self) -> number {
        return 2
    }
}

@def takesBase(value: Base) -> number {
    return 0
}

def convert(x) -> number {
    switch typeof x {
    case 'A':
        return x.a()
    case 'B':
        return x.b()
    default:
        return takesBase(x)
    }
}
)mn",
	    "cannot pass");
}

TEST(build_context_narrowing, switch_default_declared_first_is_not_narrowed) {
	EXPECT_COMPILE_ERROR(R"mn(
@class Base {}

@class Date : Base {
    const def seconds(const self) -> number {
        return 1
    }
}

@def takesBase(value: Base) -> number {
    return 0
}

def convert(x) -> number {
    switch typeof x {
    default:
        return takesBase(x)
    case 'Date':
        return x.seconds()
    }
}
)mn",
	    "cannot pass");
}

TEST(build_context_narrowing, if_narrowing_does_not_outlive_its_block) {
	EXPECT_COMPILE_ERROR(R"mn(
@class Base {}

@class Date : Base {
    const def seconds(const self) -> number {
        return 1
    }
}

@def takesBase(value: Base) -> number {
    return 0
}

def convert(x) -> number {
    if typeof x === 'Date' {
        return x.seconds()
    }
    return takesBase(x)
}
)mn",
	    "cannot pass");
}

/* ------------------------------------------------------------------------ */
/* Unknown types                                                            */
/* ------------------------------------------------------------------------ */

TEST(build_context_unknown_types, unknown_argument_is_rejected_by_typed_parameter) {
	EXPECT_COMPILE_ERROR(R"mn(
@def takesNumber(value: number) -> number {
    return value
}

def convert(x) -> number {
    return takesNumber(x)
}
)mn",
	    "cannot pass");
}

TEST(build_context_unknown_types, unknown_argument_is_rejected_by_class_typed_parameter) {
	EXPECT_COMPILE_ERROR(R"mn(
@class Foo {}

@def takesFoo(value: Foo) -> number {
    return 0
}

def convert(x) -> number {
    return takesFoo(x)
}
)mn",
	    "cannot pass");
}

TEST(build_context_unknown_types, unknown_argument_is_accepted_by_untyped_parameter) {
	EXPECT_COMPILES(R"mn(
@def takesAny(value) {
    return value
}

def convert(x) {
    return takesAny(x)
}
)mn");
}

TEST(build_context_narrowing, narrowing_still_rejects_wrong_argument_inside_its_branch) {
	EXPECT_COMPILE_ERROR(R"mn(class Date {}

def takesNumber(value: number) -> number {
    return value
}

def convert(x) -> number {
    if typeof x === 'Date' {
        return takesNumber(x)
    }
    return 0
}
)mn",
	    "cannot pass");
}

/* ------------------------------------------------------------------------ */
/* Scope of `@` (Reference::global) and receiver insertion                  */
/* ------------------------------------------------------------------------ */

// A root-scope global function takes its left operand as first parameter,
// whatever the type of this operand.
TEST(build_context_scopes, root_global_function_accepts_any_left_operand) {
	EXPECT_COMPILES(R"mn(@const def describe(const Self) {
    return typeof Self
}

class Foo {}

def test() {
    var n = 1
    n.describe()
    Foo().describe()
    Foo.describe()
}
)mn");
}

// A global function declared in a class has the class as owner scope:
// no receiver is inserted.
TEST(build_context_scopes, class_scoped_global_function_receives_no_receiver) {
	EXPECT_COMPILES(R"mn(class Foo {
    @def helper(value) {
        return value
    }
}

def test() {
    return Foo.helper(1)
}
)mn");
}

TEST(build_context_scopes, class_scoped_global_function_without_argument_reports_too_few_arguments) {
	EXPECT_COMPILE_ERROR(R"mn(class Foo {
    @def helper(value) {
        return value
    }
}

def test() {
    return Foo.helper()
}
)mn",
	    "too few arguments");
}

TEST(build_context_scopes, class_scoped_global_function_with_extra_argument_reports_too_many_arguments) {
	EXPECT_COMPILE_ERROR(R"mn(class Foo {
    @def helper(value) {
        return value
    }
}

def test() {
    return Foo.helper(1, 2)
}
)mn",
	    "too many arguments");
}

// Same rule through an alias: the receiver decision is taken on the class.
TEST(build_context_scopes, class_scoped_global_function_through_alias_receives_no_receiver) {
	EXPECT_COMPILES(R"mn(class Foo {
    @def helper(value) {
        return value
    }
}

const Alias = Foo

def test() {
    return Alias.helper(1)
}
)mn");
}

/* ------------------------------------------------------------------------ */
/* Packages                                                                 */
/* ------------------------------------------------------------------------ */

// A global function declared in a package has the package as owner scope:
// no receiver is inserted.
TEST(build_context_packages, package_function_receives_no_receiver) {
	EXPECT_COMPILES(R"mn(package P {
    @def f(value) {
        return value
    }
}

def test() {
    return P.f(1)
}
)mn");
}

TEST(build_context_packages, package_function_without_argument_reports_too_few_arguments) {
	EXPECT_COMPILE_ERROR(R"mn(package P {
    @def f(value) {
        return value
    }
}

def test() {
    return P.f()
}
)mn",
	    "too few arguments");
}

TEST(build_context_packages, package_function_with_extra_argument_reports_too_many_arguments) {
	EXPECT_COMPILE_ERROR(R"mn(package P {
    @def f(value) {
        return value
    }
}

def test() {
    return P.f(1, 2)
}
)mn",
	    "too many arguments");
}

// A function with an explicit `self` parameter, called through its package,
// still receives nothing implicitly (System.spawnMethod).
TEST(build_context_packages, package_function_with_explicit_self_parameter) {
	EXPECT_COMPILES(R"mn(package P {
    @def method(self, value) {
        return value
    }
}

class Foo {}

def test() {
    return P.method(Foo(), 1)
}
)mn");
}

// A root-scope global function takes the package as its first parameter.
TEST(build_context_packages, package_can_be_passed_as_self_to_a_root_global_function) {
	EXPECT_COMPILES(R"mn(@const def describe(const Self) {
    return typeof Self
}

package P {
    @def f(value) {
        return value
    }
}

def test() {
    return P.describe()
}
)mn");
}

// Several `package P` blocks complete each other.
TEST(build_context_packages, package_blocks_are_merged) {
	EXPECT_COMPILES(R"mn(package P {
    @def f(value) {
        return value
    }
}

package P {
    @def g(value) {
        return value
    }
}

def test() {
    P.f(1)
    P.g(2)
}
)mn");
}

TEST(build_context_packages, alias_of_package_behaves_like_the_package) {
	EXPECT_COMPILES(R"mn(@const def describe(const Self) {
    return typeof Self
}

package P {
    @def f(value) {
        return value
    }
}

const Alias = P

def test() {
    Alias.f(1)
    Alias.describe()
}
)mn");
}

TEST(build_context_packages, alias_of_package_call_without_argument_reports_too_few_arguments) {
	EXPECT_COMPILE_ERROR(R"mn(package P {
    @def f(value) {
        return value
    }
}

const Alias = P

def test() {
    return Alias.f()
}
)mn",
	    "too few arguments");
}

// The package is used before its declaration: the validation is deferred.
TEST(build_context_packages, package_function_used_before_the_package_is_declared) {
	EXPECT_COMPILES(R"mn(def test() {
    return P.f(1)
}

package P {
    @def f(value) {
        return value
    }
}
)mn");
}

/* ------------------------------------------------------------------------ */
/* Try/catch control flow                                                   */
/* ------------------------------------------------------------------------ */

TEST(build_context_try_catch, catch_return_does_not_hide_try_fallthrough) {
	EXPECT_COMPILE_ERROR(R"mn(def select() -> number {
    try {
        1
    } catch error {
        return 1
    }
}
)mn",
	    "return type mismatch");
}

TEST(build_context_try_catch, both_try_and_catch_return) {
	EXPECT_COMPILES(R"mn(def select() -> number {
    try {
        return 1
    } catch error {
        return 2
    }
}
)mn");
}

TEST(build_context_try_catch, try_without_catch_preserves_if_fallthrough) {
	EXPECT_COMPILE_ERROR(R"mn(def select(flag) -> number {
    try {
        if flag {
            1
        } else {
            return 1
        }
    }
}
)mn",
	    "return type mismatch");
}

TEST(build_context_try_catch, try_without_catch_preserves_elif_fallthrough) {
	EXPECT_COMPILE_ERROR(R"mn(def select(flag) -> number {
    try {
        if flag {
            return 1
        } elif flag {
            2
        } else {
            return 3
        }
    }
}
)mn",
	    "return type mismatch");
}

TEST(build_context_try_catch, try_without_catch_preserves_raise_fallthrough) {
	EXPECT_COMPILE_ERROR(R"mn(def select(value) -> number {
    try {
        raise value
    }
}
)mn",
	    "return type mismatch");
}

TEST(build_context_try_catch, uncaught_raise_terminates_function) {
	EXPECT_COMPILES(R"mn(def select(value) -> number {
    raise value
}
)mn");
}

TEST(build_context_try_catch, caught_raise_uses_catch_flow) {
	EXPECT_COMPILES(R"mn(def select(value) -> number {
    try {
        raise value
    } catch error {
        return 1
    }
}
)mn");
}

/* ------------------------------------------------------------------------ */
/* Aliases passed as arguments (deferred validation chain)                  */
/* ------------------------------------------------------------------------ */

// Target case: alias of a module-local class passed to a constructor.
TEST(build_context_alias_arguments, alias_of_local_class_passed_to_constructor) {
	EXPECT_COMPILES(R"mn(class Local {}

class Foo {
    const def new(self, value) {
        return self
    }
}

const Alias = Local

def test() {
    return Foo(Alias)
}
)mn");
}

// Control: no alias.
TEST(build_context_alias_arguments, local_class_passed_to_constructor) {
	EXPECT_COMPILES(R"mn(class Local {}

class Foo {
    const def new(self, value) {
        return self
    }
}

def test() {
    return Foo(Local)
}
)mn");
}

// Control: the target is global (exported).
TEST(build_context_alias_arguments, alias_of_global_class_passed_to_constructor) {
	EXPECT_COMPILES(R"mn(@class Global {}

class Foo {
    const def new(self, value) {
        return self
    }
}

const Alias = Global

def test() {
    return Foo(Alias)
}
)mn");
}

// Control: no constructor involved.
TEST(build_context_alias_arguments, alias_of_local_class_passed_to_function) {
	EXPECT_COMPILES(R"mn(class Local {}

def take(value) {
    return value
}

const Alias = Local

def test() {
    return take(Alias)
}
)mn");
}

// Control: a method instead of a constructor.
TEST(build_context_alias_arguments, alias_of_local_class_passed_to_method) {
	EXPECT_COMPILES(R"mn(class Local {}

class Foo {
    const def take(self, value) {
        return value
    }
}

const Alias = Local

def test() {
    return Foo().take(Alias)
}
)mn");
}

// The use comes before the alias and the class: everything is deferred.
TEST(build_context_alias_arguments, alias_argument_used_before_alias_and_target_are_declared) {
	EXPECT_COMPILES(R"mn(def test() {
    return Foo(Alias)
}

class Foo {
    const def new(self, value) {
        return self
    }
}

const Alias = Local

class Local {}
)mn");
}
