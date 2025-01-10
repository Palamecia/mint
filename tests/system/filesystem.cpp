#include <gtest/gtest.h>
#include <mint/system/filesystem.h>

using namespace mint;

TEST(filesystem, relativePath) {
	EXPECT_EQ(FileSystem::native_path("test/foo"), FileSystem::instance().relative_path("root", "root/test/foo"));
	EXPECT_EQ(FileSystem::native_path("../test"), FileSystem::instance().relative_path("root/foo", "root/test"));
	EXPECT_EQ(FileSystem::native_path(""), FileSystem::instance().relative_path("root/foo", "root/foo"));
}

TEST(filesystem, cleanPath) {
	
	EXPECT_EQ(FileSystem::native_path("test/foo"), FileSystem::clean_path("test/./foo"));
	EXPECT_EQ(FileSystem::native_path("test/foo"), FileSystem::clean_path("test/bar/../foo"));
	EXPECT_EQ(FileSystem::native_path("foo"), FileSystem::clean_path("test/../foo"));
	EXPECT_EQ(FileSystem::native_path("foo"), FileSystem::clean_path("foo/bar/.."));
	EXPECT_EQ(FileSystem::native_path("./test"), FileSystem::clean_path("./test"));
	EXPECT_EQ(FileSystem::native_path("./test"), FileSystem::clean_path("./foo/../test"));
	EXPECT_EQ(FileSystem::native_path("../test"), FileSystem::clean_path("../test"));
	EXPECT_EQ(FileSystem::native_path("../test"), FileSystem::clean_path("../foo/../test"));
	EXPECT_EQ(FileSystem::native_path("./../test"), FileSystem::clean_path("./../test"));
	EXPECT_EQ(FileSystem::native_path("./../test"), FileSystem::clean_path("./../foo/../test"));
	EXPECT_EQ(FileSystem::native_path("../../test"), FileSystem::clean_path("../../test"));
	EXPECT_EQ(FileSystem::native_path("../../test"), FileSystem::clean_path("../../foo/../test"));

	/// \todo unit tests

}

TEST(filesystem, copy) {

	char source_path[FileSystem::PATH_LENGTH];
	char target_path[FileSystem::PATH_LENGTH];

	tmpnam(source_path);
	tmpnam(target_path);

	FILE *source = fopen(source_path, "wb");
	ASSERT_NE(nullptr, source);

	const char data[] = "test\r\ntest\n\rtest\ntest\rtest";
	size_t len = sizeof (data);
	fwrite(data, sizeof (char), len, source);
	fclose(source);

	FileSystem::instance().copy(source_path, target_path);
	remove(source_path);

	FILE *target = fopen(target_path, "rb");
	ASSERT_NE(nullptr, target);

	char *buffer = static_cast<char *>(malloc(len));
	fread(buffer, sizeof (char), len, target);
	fclose(target);

	EXPECT_EQ(0, memcmp(data, buffer, len));
	remove(target_path);
	free(buffer);
}
