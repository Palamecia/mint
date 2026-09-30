#include "mint/program/file_printer.h"
#include "mint/program/module.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/scheduler.h"
#include <array>
#include <cstdio>
#include <gsl/pointers>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string>
#include <vector>

TEST(file_printer, print_none) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	const auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		const auto none = mint::create_none();
		auto printer = mint::FilePrinter(fd);
		printer.print(none);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("", buffer.data());

	std::fclose(file);
}

TEST(file_printer, print_null) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	const auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		const auto null = mint::create_null();
		auto printer = mint::FilePrinter(fd);
		printer.print(null);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("(null)", buffer.data());

	std::fclose(file);
}

TEST(file_printer, print_libobject) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	const auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		const auto object = mint::create_c_object(scheduler.program(), reinterpret_cast<int*>(0x7357));
		auto printer = mint::FilePrinter(fd);
		printer.print(object);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("(libobject)", buffer.data());

	std::fclose(file);
}

TEST(file_printer, print_package) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	const auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		auto package_data = mint::PackageData(scheduler.program(), "test");
		const auto package = mint::make_reference<mint::Package>(mint::create_flags, package_data);
		auto printer = mint::FilePrinter(fd);
		printer.print(package);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("(package)", buffer.data());

	std::fclose(file);
}

TEST(file_printer, print_function) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	const auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		const auto function = mint::create_function();
		auto printer = mint::FilePrinter(fd);
		printer.print(function);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("(function)", buffer.data());

	std::fclose(file);
}

TEST(file_printer, print_string) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	const auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		const auto string = mint::create_string(scheduler.program(), "foo");
		auto printer = mint::FilePrinter(fd);
		printer.print(string);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("foo", buffer.data());

	std::fclose(file);
}

TEST(file_printer, print_integer) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	const auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		const auto number = mint::create_number(3.);
		auto printer = mint::FilePrinter(fd);
		printer.print(number);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("3", buffer.data());

	std::fclose(file);
}

TEST(file_printer, print_number) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	const auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		const auto number = mint::create_number(3.14);
		auto printer = mint::FilePrinter(fd);
		printer.print(number);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("3.14", buffer.data());

	std::fclose(file);
}

TEST(file_printer, print_scientific_number) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	const auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		const auto number = mint::create_number(31415926535.9);
		auto printer = mint::FilePrinter(fd);
		printer.print(number);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("3.14159e+10", buffer.data());

	std::fclose(file);
}

TEST(file_printer, print_false) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	const auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		const auto boolean = mint::create_boolean(false);
		auto printer = mint::FilePrinter(fd);
		printer.print(boolean);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("false", buffer.data());

	std::fclose(file);
}

TEST(file_printer, print_true) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	const auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		const auto boolean = mint::create_boolean(true);
		auto printer = mint::FilePrinter(fd);
		printer.print(boolean);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));
	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("true", buffer.data());

	std::fclose(file);
}

TEST(file_printer, print_twice) {

	auto scheduler = mint::Scheduler({});
	const auto process = scheduler.enable_testing();
	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	const auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	auto buffer = std::array<char, BUFSIZ>();

	{
		const auto string = mint::create_string(scheduler.program(), "foo\n");
		auto printer = mint::FilePrinter(fd);
		printer.print(string);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("foo\n", buffer.data());

	{
		const auto string = mint::create_string(scheduler.program(), "bar\n");
		auto printer = mint::FilePrinter(fd);
		printer.print(string);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("foo\nbar\n", buffer.data());

	std::fclose(file);
}
