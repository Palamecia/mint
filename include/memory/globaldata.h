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

	void cleanupMemory();
	void cleanupMetadata();

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

protected:
	GlobalData();
	~GlobalData() override;
	friend class AbstractSyntaxTree;

private:
	static GlobalData *g_instance;
	Class *m_builtin[Class::libobject + 1];
};

SymbolTable &PackageData::symbols() { return m_symbols; }

template<class BuiltinClass>
BuiltinClass *GlobalData::builtin(Class::Metatype type) {
	BuiltinClass *instance = static_cast<BuiltinClass *>(m_builtin[type]);
	if (UNLIKELY(instance == nullptr)) {
		instance = static_cast<BuiltinClass *>(m_builtin[type] = new BuiltinClass);
	}
	return instance;
}

}

#endif // MINT_GLOBALDATA_H
