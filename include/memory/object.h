#ifndef OBJECT_H
#define OBJECT_H

#include <ast/module.h>
#include <ast/symbol.h>
#include <ast/symbolmapping.hpp>
#include <memory/data.h>
#include <memory/reference.h>
#include <memory/memorypool.hpp>
#include <system/poolallocator.hpp>

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <deque>

namespace mint {

class Class;
class PackageData;

struct MINT_EXPORT Number : public Data {
	double value;

protected:
	template<typename Type> friend class LocalPool;
	friend class Reference;

	Number();
	Number(double value);
	Number(const Number &other);

private:
	static LocalPool<Number> g_pool;
};

struct MINT_EXPORT Boolean : public Data {
	bool value;

protected:
	template<typename Type> friend class LocalPool;
	friend class Reference;

	Boolean();
	Boolean(bool value);
	Boolean(const Boolean &other);

private:
	static LocalPool<Boolean> g_pool;
};

struct MINT_EXPORT Object : public Data {
	Class *const metadata;
	WeakReference *data;

	void construct();
	void construct(const Object &other);

	void mark() override;

protected:
	template<typename Type> friend class LocalPool;
	friend class Reference;

	Object(Class *type);
	Object(const Object &other) = delete;
	~Object() override;

private:
	void construct(const Object &other, std::unordered_map<const Data *, Data *> &memory_map);

	static LocalPool<Object> g_pool;
};

struct MINT_EXPORT Package : public Data {
	PackageData *const data;

protected:
	template<typename Type> friend class LocalPool;
	friend class Reference;

	Package(PackageData *package);

private:
	static LocalPool<Package> g_pool;
};

struct MINT_EXPORT Function : public Data {
	using Capture = SymbolMapping<WeakReference>;

	struct MINT_EXPORT Signature {
		Signature(Module::Handle *handle);
		Signature(const Signature &other);
		Signature(Signature &&other);

		Module::Handle *const handle;
		std::unique_ptr<Capture> capture;
	};

	using mapping_type = std::map<int, Signature>;
	mapping_type mapping;

	void mark() override;

protected:
	template<typename Type> friend class LocalPool;
	friend class Reference;

	Function();
	Function(const Function &other);

private:
	static LocalPool<Function> g_pool;
};

}

#endif // OBJECT_H
