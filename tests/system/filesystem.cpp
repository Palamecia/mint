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
