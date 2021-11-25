#include "memory/functiontool.h"
#include "memory/casttool.h"
#include "system/filesystem.h"

using namespace std;
using namespace mint;

enum class StandardPath {
	root,
	home,
	desktop,
	documents,
	musics,
	movies,
	pictures,
	download,
	applications,
	temporary,
	fonts,
	cache,
	global_cache,
	data,
	local_data,
	global_data,
	config,
	global_config
};

static vector<string> standardPaths(StandardPath type) {
	switch (type) {
	case StandardPath::root:
		return { FileSystem::instance().rootPath() };
	case StandardPath::home:
		return { FileSystem::instance().homePath() };
	case StandardPath::desktop:
		return { FileSystem::instance().homePath() + "/Desktop" };
	case StandardPath::documents:
		return { FileSystem::instance().homePath() + "/Documents" };
	case StandardPath::musics:
		return { FileSystem::instance().homePath() + "/Musics" };
	case StandardPath::movies:
		return { FileSystem::instance().homePath() + "/Movies" };
	case StandardPath::pictures:
		return { FileSystem::instance().homePath() + "/Pictures" };
	case StandardPath::download:
		return { FileSystem::instance().homePath() + "/Downloads" };
	case StandardPath::applications:
#if defined (OS_UNIX)
		return {
			"/usr/bin",
			"/bin",
			"/usr/sbin"
			"/usr/local/bin",
		};
#elif defined (OS_WINDOWS)
		return {
			FileSystem::instance().rootPath() + "/Program Files",
			FileSystem::instance().rootPath() + "/Program Files (x86)"
		};
#elif defined (OS_MAC)
		return {};
#else
		return {};
#endif
	case StandardPath::temporary:
#if defined (OS_UNIX)
		return { "/tmp" };
#elif defined (OS_WINDOWS)
		return {
			FileSystem::instance().homePath() + "/AppData/Local/Temp",
			FileSystem::instance().rootPath() + "/Windows/Temp"
		};
#elif defined (OS_MAC)
		return {};
#else
		return {};
#endif
	case StandardPath::fonts:
		return {};
	case StandardPath::cache:
		return {};
	case StandardPath::global_cache:
		return {};
	case StandardPath::data:
		return {};
	case StandardPath::local_data:
		return {};
	case StandardPath::global_data:
		return {};
	case StandardPath::config:
		return {};
	case StandardPath::global_config:
		return {};
	}
};

MINT_FUNCTION(mint_fs_get_paths, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &type = helper.popParameter();
	WeakReference result = create_array();

	for (const string &path : standardPaths(static_cast<StandardPath>(to_integer(cursor, type)))) {
		array_append(result.data<Array>(), create_string(path));
	}

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_fs_get_path, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &type = helper.popParameter();

	helper.returnValue(create_string(standardPaths(static_cast<StandardPath>(to_integer(cursor, type))).front()));
}

MINT_FUNCTION(mint_fs_get_path, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &path = helper.popParameter();
	Reference &type = helper.popParameter();

	string root = standardPaths(static_cast<StandardPath>(to_integer(cursor, type))).front();
	helper.returnValue(create_string(FileSystem::cleanPath(root + FileSystem::separator + to_string(path))));
}

MINT_FUNCTION(mint_fs_find_paths, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &path = helper.popParameter();
	Reference &type = helper.popParameter();
	WeakReference result = create_array();

	for (const string &root : standardPaths(static_cast<StandardPath>(to_integer(cursor, type)))) {
		string full_path = FileSystem::cleanPath(root + FileSystem::separator + to_string(path));
		if (FileSystem::instance().checkFileAccess(full_path, FileSystem::exists)) {
			array_append(result.data<Array>(), create_string(full_path));
		}
	}

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_fs_find_path, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &path = helper.popParameter();
	Reference &type = helper.popParameter();

	for (const string &root : standardPaths(static_cast<StandardPath>(to_integer(cursor, type)))) {
		string full_path = FileSystem::cleanPath(root + FileSystem::separator + to_string(path));
		if (FileSystem::instance().checkFileAccess(full_path, FileSystem::exists)) {
			helper.returnValue(create_string(full_path));
			break;
		}
	}
}
