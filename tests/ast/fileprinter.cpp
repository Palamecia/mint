#include "mint/ast/fileprinter.h"
#include "mint/ast/module.h"
#include "mint/memory/functiontool.h"
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

TEST(fileprinter, print_none) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	auto fd = fileno(file);
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

TEST(fileprinter, print_null) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	auto fd = fileno(file);
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

TEST(fileprinter, print_libobject) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		const auto object = mint::create_c_object(scheduler.ast(), reinterpret_cast<int*>(0x7357));
		auto printer = mint::FilePrinter(fd);
		printer.print(object);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("(libobject)", buffer.data());

	std::fclose(file);
}

TEST(fileprinter, print_package) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		auto package_data = mint::PackageData(scheduler.ast(), "test");
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

TEST(fileprinter, print_function) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	auto fd = fileno(file);
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

TEST(fileprinter, print_string) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	{
		const auto string = mint::create_string(scheduler.ast(), "foo");
		auto printer = mint::FilePrinter(fd);
		printer.print(string);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("foo", buffer.data());

	std::fclose(file);
}

TEST(fileprinter, print_integer) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	auto fd = fileno(file);
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

TEST(fileprinter, print_number) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	auto fd = fileno(file);
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

TEST(fileprinter, print_scientific_number) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	auto fd = fileno(file);
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

TEST(fileprinter, print_false) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	auto fd = fileno(file);
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

TEST(fileprinter, print_true) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	auto buffer = std::array<char, BUFSIZ>();

	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	auto fd = fileno(file);
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

TEST(fileprinter, print_twice) {

	mint::Scheduler scheduler({});
	auto process = scheduler.enable_testing();
	gsl::owner<FILE*> file = tmpfile();
	ASSERT_NE(nullptr, file);

	auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	auto buffer = std::array<char, BUFSIZ>();

	{
		const auto string = mint::create_string(scheduler.ast(), "foo\n");
		auto printer = mint::FilePrinter(fd);
		printer.print(string);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("foo\n", buffer.data());

	{
		const auto string = mint::create_string(scheduler.ast(), "bar\n");
		auto printer = mint::FilePrinter(fd);
		printer.print(string);
	}

	ASSERT_EQ(0, std::fseek(file, 0, SEEK_SET));

	std::fread(buffer.data(), sizeof(char), buffer.size(), file);
	ASSERT_EQ(0, std::ferror(file));
	EXPECT_STREQ("foo\nbar\n", buffer.data());

	std::fclose(file);
}
