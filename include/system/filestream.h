#ifndef MINT_FILESTREAM_H
#define MINT_FILESTREAM_H

#include "system/datastream.h"

#include <string>

namespace mint {

class MINT_EXPORT FileStream : public DataStream {
public:
	FileStream(const std::string &name);
	~FileStream();

	bool atEnd() const override;

	bool isValid() const override;
	std::string path() const override;

protected:
	int readChar() override;
	int nextBufferedChar() override;

private:
	FILE *m_file;
	std::string m_path;
	bool m_over;
};

}

#endif // MINT_FILESTREAM_H
