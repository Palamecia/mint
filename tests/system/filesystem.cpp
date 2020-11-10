#include <gtest/gtest.h>
#include <system/filesystem.h>

using namespace mint;

TEST(filesystem, relativePath) {
	EXPECT_EQ(FileSystem::nativePath("test/foo"), FileSystem::instance().relativePath("root", "root/test/foo"));
	EXPECT_EQ(FileSystem::nativePath("../test"), FileSystem::instance().relativePath("root/foo", "root/test"));
	EXPECT_EQ(FileSystem::nativePath(""), FileSystem::instance().relativePath("root/foo", "root/foo"));
}

TEST(filesystem, cleanPath) {

	EXPECT_EQ(FileSystem::nativePath("test/foo"), FileSystem::cleanPath("test/./foo"));
	EXPECT_EQ(FileSystem::nativePath("test/foo"), FileSystem::cleanPath("test/bar/../foo"));
	EXPECT_EQ(FileSystem::nativePath("foo"), FileSystem::cleanPath("test/../foo"));
	EXPECT_EQ(FileSystem::nativePath("foo"), FileSystem::cleanPath("foo/bar/.."));
	EXPECT_EQ(FileSystem::nativePath("./test"), FileSystem::cleanPath("./test"));
	EXPECT_EQ(FileSystem::nativePath("./test"), FileSystem::cleanPath("./foo/../test"));
	EXPECT_EQ(FileSystem::nativePath("../test"), FileSystem::cleanPath("../test"));
	EXPECT_EQ(FileSystem::nativePath("../test"), FileSystem::cleanPath("../foo/../test"));
	EXPECT_EQ(FileSystem::nativePath("./../test"), FileSystem::cleanPath("./../test"));
	EXPECT_EQ(FileSystem::nativePath("./../test"), FileSystem::cleanPath("./../foo/../test"));
	EXPECT_EQ(FileSystem::nativePath("../../test"), FileSystem::cleanPath("../../test"));
	EXPECT_EQ(FileSystem::nativePath("../../test"), FileSystem::cleanPath("../../foo/../test"));

	/// \todo unit tests

}

TEST(filesystem, copy) {

	char source_path[FileSystem::path_length];
	char target_path[FileSystem::path_length];

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

	char *buffer = static_cast<char *>(alloca(len));
	fread(buffer, sizeof (char), len, target);
	fclose(target);

	EXPECT_EQ(0, memcmp(data, buffer, len));
	remove(target_path);
}
