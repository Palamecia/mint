#include <cstdio>
#include <cstring>
#include <gsl/pointers>
#include <gtest/gtest.h>
#include "mint/config.h"
#include "mint/system/filesystem.h"

#include <filesystem>
#include <array>
#include <memory>
#include <string_view>

#ifdef MINT_OS_WINDOWS
#include <corecrt_io.h>
#else
#include <stdio.h>
#endif

TEST(filesystem, relative_path) {
	EXPECT_EQ(mint::FileSystem::normalized("test/foo"), std::filesystem::relative("root/test/foo", "root"));
	EXPECT_EQ(mint::FileSystem::normalized("../test"), std::filesystem::relative("root/test", "root/foo"));
	EXPECT_EQ(mint::FileSystem::normalized("."), std::filesystem::relative("root/foo", "root/foo"));
}

TEST(filesystem, normalized) {
	EXPECT_EQ(mint::FileSystem::normalized("test/foo"), mint::FileSystem::normalized("test/./foo"));
	EXPECT_EQ(mint::FileSystem::normalized("test/foo"), mint::FileSystem::normalized("test/bar/../foo"));
	EXPECT_EQ(mint::FileSystem::normalized("foo"), mint::FileSystem::normalized("test/../foo"));
	EXPECT_EQ(mint::FileSystem::normalized("foo/"), mint::FileSystem::normalized("foo/bar/.."));
	EXPECT_EQ(mint::FileSystem::normalized("./test"), mint::FileSystem::normalized("./test"));
	EXPECT_EQ(mint::FileSystem::normalized("./test"), mint::FileSystem::normalized("./foo/../test"));
	EXPECT_EQ(mint::FileSystem::normalized("../test"), mint::FileSystem::normalized("../test"));
	EXPECT_EQ(mint::FileSystem::normalized("../test"), mint::FileSystem::normalized("../foo/../test"));
	EXPECT_EQ(mint::FileSystem::normalized("./../test"), mint::FileSystem::normalized("./../test"));
	EXPECT_EQ(mint::FileSystem::normalized("./../test"), mint::FileSystem::normalized("./../foo/../test"));
	EXPECT_EQ(mint::FileSystem::normalized("../../test"), mint::FileSystem::normalized("../../test"));
	EXPECT_EQ(mint::FileSystem::normalized("../../test"), mint::FileSystem::normalized("../../foo/../test"));
}

TEST(filesystem, generic_wstring) {
	EXPECT_EQ(L"êöàç", std::filesystem::path(u8"êöàç").generic_wstring());
}

TEST(filesystem, join) {
	EXPECT_EQ(mint::FileSystem::root_path().generic_string() + "foo/bar/baz",
	    (mint::FileSystem::root_path() / "foo" / "bar" / "baz").generic_string());
}

TEST(filesystem, copy) {

	std::array<char, mint::FileSystem::path_length> source_path {};
	std::array<char, mint::FileSystem::path_length> target_path {};

	tmpnam(source_path.data());
	tmpnam(target_path.data());

	gsl::owner<FILE*> source = std::fopen(source_path.data(), "wb");
	ASSERT_NE(nullptr, source);

	const std::string_view data = "test\r\ntest\n\rtest\ntest\rtest";
	fwrite(data.data(), sizeof(char), data.size(), source);
	fclose(source);

	std::filesystem::copy(source_path.data(), target_path.data());
	remove(source_path.data());

	gsl::owner<FILE*> target = std::fopen(target_path.data(), "rb");
	ASSERT_NE(nullptr, target);

	std::allocator<char> allocator {};
	auto* buffer = allocator.allocate(data.size());
	std::fread(buffer, sizeof(char), data.size(), target);
	std::fclose(target);

	EXPECT_EQ(0, memcmp(data.data(), buffer, data.size()));
	remove(target_path.data());
	allocator.deallocate(buffer, data.size());
}
