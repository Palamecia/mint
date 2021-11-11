#ifndef CLASSREGISTER_H
#define CLASSREGISTER_H

#include <ast/symbolmapping.hpp>
#include <memory/class.h>

#include <vector>
#include <string>

namespace mint {

class ClassDescription;

class MINT_EXPORT ClassRegister {
	ClassRegister(const ClassRegister &) = delete;
	ClassRegister &operator =(const ClassRegister &) = delete;
public:
	using Id = size_t;

	ClassRegister();
	virtual ~ClassRegister();

	virtual Id createClass(ClassDescription *desc);

	ClassDescription *findClassDescription(const Symbol &name) const;
	ClassDescription *getClassDescription(Id id) const;
	size_t count() const;

private:
	std::vector<ClassDescription *> m_definedClasses;
};

class MINT_EXPORT ClassDescription : public ClassRegister {
	ClassDescription(const ClassDescription &) = delete;
	ClassDescription &operator =(const ClassDescription &) = delete;
public:
	class MINT_EXPORT Path {
	public:
		ClassDescription *locate(PackageData *package) const;
		std::string toString() const;

		void appendSymbol(const Symbol &symbol);
		void clear();

	private:
		std::vector<Symbol> m_symbols;
	};

	ClassDescription(PackageData *package, Reference::Flags flags, const std::string &name);
	~ClassDescription() override;

	Symbol name() const;
	std::string fullName() const;
	Reference::Flags flags() const;

	void addBase(const Path &base);
	Id createClass(ClassDescription *desc) override;

	bool createMember(Class::Operator op, Reference &&value);
	bool createMember(const Symbol &name, Reference &&value);
	bool updateMember(Class::Operator op, Reference &&value);
	bool updateMember(const Symbol &name, Reference &&value);

	const std::set<Class *> &bases() const;
	Class *generate();

	void cleanupMemory();
	void cleanupMetadata();

private:
	ClassDescription *m_owner;
	PackageData *m_package;
	Reference::Flags m_flags;
	std::vector<Path> m_bases;

	Symbol m_name;
	Class *m_metadata;
	std::set<Class *> m_basesMetadata;

	std::unordered_map<Class::Operator, StrongReference> m_operators;
	SymbolMapping<StrongReference> m_members;
	SymbolMapping<StrongReference> m_globals;
};

}

#endif // CLASSREGISTER_H
