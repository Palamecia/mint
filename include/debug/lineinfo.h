#ifndef LINE_INFO_H
#define LINE_INFO_H

#include <config.h>
#include <string>
#include <vector>

namespace mint {

class MINT_EXPORT LineInfo {
public:
	LineInfo(const std::string &module, size_t lineNumber = 0);

	std::string moduleName() const;
	size_t lineNumber() const;
	std::string toString() const;

private:
	std::string m_moduleName;
	size_t m_lineNumber;
};

typedef std::vector<LineInfo> LineInfoList;

}

#endif // LINE_INFO_H
