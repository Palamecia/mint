#include <gtest/gtest.h>
#include <system/filesystem.h>

using namespace mint;

TEST(filesystem, relativePath) {
	EXPECT_EQ("test/foo", FileSystem::instance().relativePath("root", "root/test/foo"));
	EXPECT_EQ("../test", FileSystem::instance().relativePath("root/foo", "root/test"));
	EXPECT_EQ("", FileSystem::instance().relativePath("root/foo", "root/foo"));
}

TEST(filesystem, cleanPath) {

	EXPECT_EQ("test/foo", FileSystem::cleanPath("test/./foo"));
	EXPECT_EQ("test/foo", FileSystem::cleanPath("test/bar/../foo"));
	EXPECT_EQ("foo", FileSystem::cleanPath("test/../foo"));
	EXPECT_EQ("foo", FileSystem::cleanPath("foo/bar/.."));
	EXPECT_EQ("./test", FileSystem::cleanPath("./test"));
	EXPECT_EQ("./test", FileSystem::cleanPath("./foo/../test"));
	EXPECT_EQ("../test", FileSystem::cleanPath("../test"));
	EXPECT_EQ("../test", FileSystem::cleanPath("../foo/../test"));
	EXPECT_EQ("./../test", FileSystem::cleanPath("./../test"));
	EXPECT_EQ("./../test", FileSystem::cleanPath("./../foo/../test"));
	EXPECT_EQ("../../test", FileSystem::cleanPath("../../test"));
	EXPECT_EQ("../../test", FileSystem::cleanPath("../../foo/../test"));

	/// \todo unit tests

}
