#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include <ast/classregister.h>
#include <memory/symboltable.h>

namespace mint {

class MINT_EXPORT PackageData : public ClassRegister {
public:
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
	static GlobalData &instance();

protected:
	GlobalData();
	~GlobalData() override;
};

SymbolTable &PackageData::symbols() { return m_symbols; }

}

#endif // GLOBAL_DATA_H
