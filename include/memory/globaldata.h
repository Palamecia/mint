#ifndef MINT_GLOBALDATA_H
#define MINT_GLOBALDATA_H

#include "ast/classregister.h"
#include "memory/symboltable.h"

namespace mint {

class MINT_EXPORT PackageData : public ClassRegister {
public:
	PackageData *getPackage() const;
	PackageData *getPackage(const Symbol &name);
	PackageData *findPackage(const Symbol &name) const;

	void registerClass(Id id);
	Class *getClass(const Symbol &name);

	std::string name() const;
	std::string fullName() const;
	inline SymbolTable &symbols();

	void cleanupMemory() override;
	void cleanupMetadata() override;

protected:
	PackageData(const std::string &name, PackageData *owner = nullptr);
	~PackageData() override;

private:
	std::string m_name;
	PackageData *m_owner;
	SymbolMapping<PackageData *> m_packages;
	SymbolTable m_symbols;
};

class MINT_EXPORT GlobalData : public PackageData {
public:
	static GlobalData *instance();

	template<class BuiltinClass>
	BuiltinClass *builtin(Class::Metatype type);

	inline Reference *noneRef();
	inline Reference *nullRef();

	void cleanupBuiltin();

protected:
	GlobalData();
	~GlobalData() override;
	friend class AbstractSyntaxTree;

private:
	static GlobalData *g_instance;
	std::array<Class *, 8> m_builtin;
	Reference *m_none = nullptr;
	Reference *m_null = nullptr;
};

SymbolTable &PackageData::symbols() { return m_symbols; }

template<class BuiltinClass>
BuiltinClass *GlobalData::builtin(Class::Metatype type) {
	if (BuiltinClass *instance = static_cast<BuiltinClass *>(m_builtin[type])) {
		return instance;
	}
	return static_cast<BuiltinClass *>(m_builtin[type] = new BuiltinClass);
}

Reference *GlobalData::noneRef() {
	return m_none ? m_none : m_none = new StrongReference(Reference::const_address | Reference::const_value, new None);
}

Reference *GlobalData::nullRef() {
	return m_null ? m_null : m_null = new StrongReference(Reference::const_address | Reference::const_value, new Null);
}

}

#endif // MINT_GLOBALDATA_H
