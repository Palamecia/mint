#include "casetable.h"
#include "branch.h"
#include "compiler/buildtool.h"

using namespace mint;

CaseTable::CaseTable() :
	current_label(nullptr) {

}

CaseTable::Label::Label(Branch *parent) :
	condition(new SubBranch(parent)) {

}
