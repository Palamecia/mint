#include <gtest/gtest.h>
#include "mint/memory/object_printer.h"
#include "mint/ast/symbol.h"
#include "mint/memory/builtin/string.h"
#include "mint/memory/class.h"
#include "mint/memory/class_tools.h"
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/processor.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/mint_runtime_error.h"

namespace {

#define WAIT_FOR_RESULT(__cursor) \
	while ((__cursor).call_in_progress()) { \
		ASSERT_EQ(mint::ProcessStatus::paused, mint::run_step(__cursor)); \
	}

mint::Reference create_writer_object(mint::AbstractSyntaxTree& ast) {
	const auto write_function = mint::create_function(ast, R"(
			def (self, value) {
				self.value = value
			}
		)");
	auto& writer_class = mint::create_class(ast, "__object_printer_writer__",
	    {
	        {mint::builtin_symbols::write_method, write_function},
	        {"value", mint::create_none()},
	    });
	return mint::create_object(writer_class);
}

mint::Reference get_printed_value(const mint::Reference& writer) {
	if (auto* member = writer.data<mint::Object>().metadata.find_member("value")) {
		return mint::Class::MemberInfo::get(*member, writer.data<mint::Object>());
	}
	return mint::create_none();
}

}

TEST(object_printer, print_does_throw_on_invalid_target) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	const auto target = mint::create_string(scheduler.ast());
	mint::ObjectPrinter printer(process->cursor(), mint::Reference::default_flags, target.data<mint::Object>());

	EXPECT_THROW(printer.print(mint::create_string(scheduler.ast(), "hello")), mint::MintRuntimeError);
}

TEST(object_printer, print_does_not_throw) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	const auto target = create_writer_object(scheduler.ast());
	mint::ObjectPrinter printer(process->cursor(), mint::Reference::default_flags, target.data<mint::Object>());

	EXPECT_NO_THROW(printer.print(mint::create_string(scheduler.ast(), "hello")));
}

TEST(object_printer, print_string) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();

	const auto target = create_writer_object(scheduler.ast());
	mint::ObjectPrinter printer(process->cursor(), mint::Reference::default_flags, target.data<mint::Object>());

	EXPECT_NO_THROW(printer.print(mint::create_string(scheduler.ast(), "hello")));
	WAIT_FOR_RESULT(process->cursor());

	const auto ref = get_printed_value(target);
	ASSERT_EQ(mint::Data::Format::object, ref.data().format());
	ASSERT_EQ(mint::Class::Metatype::string, ref.data<mint::Object>().metadata.metatype());
	EXPECT_EQ("hello", ref.data<mint::String>().str);
}
