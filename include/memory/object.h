#ifndef MINT_OBJECT_H
#define MINT_OBJECT_H

#include "ast/module.h"
#include "ast/symbolmapping.hpp"
#include "memory/data.h"
#include "memory/reference.h"
#include "memory/memorypool.hpp"

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
		Signature(Module::Handle *handle, bool capture = false);
		Signature(const Signature &other);
		Signature(Signature &&other);
		~Signature();

		Module::Handle *const handle;
		Capture *const capture;
	};

	class MINT_EXPORT mapping_type {
	public:
		using iterator = std::map<int, Signature>::iterator;
		using const_iterator = std::map<int, Signature>::const_iterator;

		mapping_type();
		mapping_type(const mapping_type &other);
		mapping_type(mapping_type &&other) noexcept;
		~mapping_type();

		mapping_type &operator =(mapping_type &&other) noexcept;
		mapping_type &operator =(const mapping_type &other);

		bool operator ==(const mapping_type &other) const;
		bool operator !=(const mapping_type &other) const;

		std::pair<iterator, bool> emplace(int signature, const Signature &handle);
		std::pair<iterator, bool> insert(const std::pair<int, Signature> &signature);

		iterator lower_bound(int signature) const;
		iterator find(int signature) const;

		const_iterator cbegin() const;
		const_iterator begin() const;
		iterator begin();

		const_iterator cend() const;
		const_iterator end() const;
		iterator end();

	private:
		struct shared_data_t {
			std::map<int, Signature> signatures;
			size_t refcount = 1;
			bool sharable = true;
			bool padding[sizeof (void *) - 1];

			shared_data_t() = default;
			shared_data_t(const std::map<int, Signature> &signatures, bool sharable) :
				signatures(signatures),
				sharable(sharable) {

			}

			inline bool isSharable() const{
				return sharable;
			}

			inline bool isShared() const {
				return refcount > 1;
			}

			inline shared_data_t *detach() const {
				return new shared_data_t(signatures, sharable);
			}
		};
		shared_data_t *m_data;
	};

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

#endif // MINT_OBJECT_H
