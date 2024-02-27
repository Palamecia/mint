#include <mint/memory/functiontool.h>
#include <mint/memory/reference.h>
#include <mint/system/string.h>
#include <gtest/gtest.h>
#include <mint/ast/abstractsyntaxtree.h>
#include <mint/ast/fileprinter.h>

using namespace mint;

TEST(fileprinter, print) {

	char buffer[BUFSIZ];
	AbstractSyntaxTree ast;

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			WeakReference none = WeakReference::create<None>();
			FilePrinter printer(fd);
			printer.print(none);
		}

		rewind(file);
		EXPECT_EQ(EOF, fscanf(file, "%s", buffer));

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			WeakReference null = WeakReference::create<Null>();
			FilePrinter printer(fd);
			printer.print(null);
		}

		rewind(file);
		ASSERT_NE(EOF, fscanf(file, "%s", buffer));
		EXPECT_STREQ("(null)", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			WeakReference object = create_object(reinterpret_cast<int *>(0x7357));
			FilePrinter printer(fd);
			printer.print(object);
		}

		rewind(file);
		ASSERT_NE(EOF, fscanf(file, "%s", buffer));
		EXPECT_STREQ("(libobject)", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			WeakReference package = WeakReference::create<Package>(nullptr);
			FilePrinter printer(fd);
			printer.print(package);
		}

		rewind(file);
		ASSERT_NE(EOF, fscanf(file, "%s", buffer));
		EXPECT_STREQ("(package)", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			WeakReference function = WeakReference::create<Function>();
			FilePrinter printer(fd);
			printer.print(function);
		}

		rewind(file);
		ASSERT_NE(EOF, fscanf(file, "%s", buffer));
		EXPECT_STREQ("(function)", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			WeakReference string = create_string("foo");
			FilePrinter printer(fd);
			printer.print(string);
		}

		rewind(file);
		ASSERT_NE(EOF, fscanf(file, "%s", buffer));
		EXPECT_STREQ("foo", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			WeakReference number = create_number(3.);
			FilePrinter printer(fd);
			printer.print(number);
		}

		rewind(file);
		ASSERT_NE(EOF, fscanf(file, "%s", buffer));
		EXPECT_STREQ("3", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			WeakReference number = create_number(3.14);
			FilePrinter printer(fd);
			printer.print(number);
		}

		rewind(file);
		ASSERT_NE(EOF, fscanf(file, "%s", buffer));
		EXPECT_STREQ("3.14", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			WeakReference number = create_number(31415926535.9);
			FilePrinter printer(fd);
			printer.print(number);
		}

		rewind(file);
		ASSERT_NE(EOF, fscanf(file, "%s", buffer));
		EXPECT_STREQ("3.14159e+10", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			WeakReference boolean = create_boolean(false);
			FilePrinter printer(fd);
			printer.print(boolean);
		}

		rewind(file);
		ASSERT_NE(EOF, fscanf(file, "%s", buffer));
		EXPECT_STREQ("false", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			WeakReference boolean = create_boolean(true);
			FilePrinter printer(fd);
			printer.print(boolean);
		}

		rewind(file);
		ASSERT_NE(EOF, fscanf(file, "%s", buffer));
		EXPECT_STREQ("true", buffer);

		fclose(file);
	}
}

TEST(fileprinter, print_twice) {

	FILE *file = tmpfile();
	ASSERT_NE(nullptr, file);

	auto fd = fileno(file);
	ASSERT_NE(-1, fd);

	char buffer[BUFSIZ];
	AbstractSyntaxTree ast;

	{
		WeakReference string = create_string("foo\n");
		FilePrinter printer(fd);
		printer.print(string);
	}

	rewind(file);
	ASSERT_NE(EOF, fscanf(file, "%s", buffer));
	EXPECT_STREQ("foo", buffer);

	{
		WeakReference string = create_string("bar\n");
		FilePrinter printer(fd);
		printer.print(string);
	}

	rewind(file);
	ASSERT_NE(EOF, fscanf(file, "%s", buffer));
	EXPECT_STREQ("foo", buffer);
	ASSERT_NE(EOF, fscanf(file, "%s", buffer));
	EXPECT_STREQ("bar", buffer);

	fclose(file);
}
