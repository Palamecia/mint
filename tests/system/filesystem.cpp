#include <gtest/gtest.h>
#include <mint/system/filesystem.h>

#include <filesystem>
#include <array>

using namespace mint;

TEST(filesystem, relative_path) {
	EXPECT_EQ(FileSystem::normalized("test/foo"), std::filesystem::relative("root/test/foo", "root"));
	EXPECT_EQ(FileSystem::normalized("../test"), std::filesystem::relative("root/test", "root/foo"));
	EXPECT_EQ(FileSystem::normalized("."), std::filesystem::relative("root/foo", "root/foo"));
}

TEST(filesystem, normalized) {
	EXPECT_EQ(FileSystem::normalized("test/foo"), FileSystem::normalized("test/./foo"));
	EXPECT_EQ(FileSystem::normalized("test/foo"), FileSystem::normalized("test/bar/../foo"));
	EXPECT_EQ(FileSystem::normalized("foo"), FileSystem::normalized("test/../foo"));
	EXPECT_EQ(FileSystem::normalized("foo/"), FileSystem::normalized("foo/bar/.."));
	EXPECT_EQ(FileSystem::normalized("./test"), FileSystem::normalized("./test"));
	EXPECT_EQ(FileSystem::normalized("./test"), FileSystem::normalized("./foo/../test"));
	EXPECT_EQ(FileSystem::normalized("../test"), FileSystem::normalized("../test"));
	EXPECT_EQ(FileSystem::normalized("../test"), FileSystem::normalized("../foo/../test"));
	EXPECT_EQ(FileSystem::normalized("./../test"), FileSystem::normalized("./../test"));
	EXPECT_EQ(FileSystem::normalized("./../test"), FileSystem::normalized("./../foo/../test"));
	EXPECT_EQ(FileSystem::normalized("../../test"), FileSystem::normalized("../../test"));
	EXPECT_EQ(FileSystem::normalized("../../test"), FileSystem::normalized("../../foo/../test"));
}

TEST(filesystem, generic_wstring) {
	EXPECT_EQ(L"êöàç", std::filesystem::path("êöàç").generic_wstring());
}

TEST(filesystem, join) {
	EXPECT_EQ(FileSystem::root_path().generic_string() + "foo/bar/baz",
			  (FileSystem::root_path() / "foo" / "bar" / "baz").generic_string());
}

TEST(filesystem, copy) {

	char source_path[FileSystem::PATH_LENGTH];
	char target_path[FileSystem::PATH_LENGTH];

	tmpnam(source_path);
	tmpnam(target_path);

	FILE *source = fopen(source_path, "wb");
	ASSERT_NE(nullptr, source);

	const char data[] = "test\r\ntest\n\rtest\ntest\rtest";
	const size_t len = sizeof(data);
	fwrite(data, sizeof(char), len, source);
	fclose(source);

	std::filesystem::copy(source_path, target_path);
	remove(source_path);

	FILE *target = fopen(target_path, "rb");
	ASSERT_NE(nullptr, target);

	auto *buffer = static_cast<char *>(malloc(len));
	fread(buffer, sizeof(char), len, target);
	fclose(target);

	EXPECT_EQ(0, memcmp(data, buffer, len));
	remove(target_path);
	free(buffer);
}
