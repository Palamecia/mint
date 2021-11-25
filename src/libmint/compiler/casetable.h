#ifndef CASE_TABLE_H
#define CASE_TABLE_H

#include <memory>
#include <string>
#include <map>

namespace mint {

class Branch;
class BuildContext;

struct CaseTable {
	struct Label {
		Label(Branch *parent);

		std::unique_ptr<Branch> condition;
		size_t offset;
	};

	CaseTable();

	std::map<std::string, Label *> labels;
	size_t *default_label = nullptr;
	Label *current_label = nullptr;
	size_t origin = 0;
};

}

#endif // CASE_TABLE_H
