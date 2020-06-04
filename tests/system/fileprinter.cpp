#include <gtest/gtest.h>
#include <system/fileprinter.h>

using namespace mint;

TEST(fileprinter, print) {

	char buffer[BUFSIZ];
	void *addr = reinterpret_cast<void *>(0x7357);

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			FilePrinter printer(fd);
			printer.print(FilePrinter::none, addr);
		}

		rewind(file);
		fscanf(file, "%s", buffer);
		EXPECT_STREQ("", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			FilePrinter printer(fd);
			printer.print(FilePrinter::null, addr);
		}

		rewind(file);
		fscanf(file, "%s", buffer);
		EXPECT_STREQ("(null)", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			FilePrinter printer(fd);
			printer.print(FilePrinter::object, addr);
		}

		rewind(file);
		fscanf(file, "%s", buffer);
		EXPECT_STREQ("0x0000000000007357", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			FilePrinter printer(fd);
			printer.print(FilePrinter::package, addr);
		}

		rewind(file);
		fscanf(file, "%s", buffer);
		EXPECT_STREQ("(package)", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			FilePrinter printer(fd);
			printer.print(FilePrinter::function, addr);
		}

		rewind(file);
		fscanf(file, "%s", buffer);
		EXPECT_STREQ("(function)", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			FilePrinter printer(fd);
			printer.print("foo");
		}

		rewind(file);
		fscanf(file, "%s", buffer);
		EXPECT_STREQ("foo", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			FilePrinter printer(fd);
			printer.print(3.);
		}

		rewind(file);
		fscanf(file, "%s", buffer);
		EXPECT_STREQ("3", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			FilePrinter printer(fd);
			printer.print(3.14);
		}

		rewind(file);
		fscanf(file, "%s", buffer);
		EXPECT_STREQ("3.14", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			FilePrinter printer(fd);
			printer.print(31415926535.9);
		}

		rewind(file);
		fscanf(file, "%s", buffer);
		EXPECT_STREQ("3.14159e+10", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			FilePrinter printer(fd);
			printer.print(false);
		}

		rewind(file);
		fscanf(file, "%s", buffer);
		EXPECT_STREQ("false", buffer);

		fclose(file);
	}

	{
		FILE *file = tmpfile();
		ASSERT_NE(nullptr, file);

		auto fd = fileno(file);
		ASSERT_NE(-1, fd);

		{
			FilePrinter printer(fd);
			printer.print(true);
		}

		rewind(file);
		fscanf(file, "%s", buffer);
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

	{
		FilePrinter printer(fd);
		printer.print("foo\n");
	}

	rewind(file);
	fscanf(file, "%s", buffer);
	EXPECT_STREQ("foo", buffer);

	{
		FilePrinter printer(fd);
		printer.print("bar\n");
	}

	rewind(file);
	fscanf(file, "%s", buffer);
	EXPECT_STREQ("foo", buffer);
	fscanf(file, "%s", buffer);
	EXPECT_STREQ("bar", buffer);

	fclose(file);
}
