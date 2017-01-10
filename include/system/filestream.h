#ifndef FILE_STREAM_H
#define FILE_STREAM_H

#include "system/datastream.h"

#include <string>

class FileStream : public DataStream {
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

#endif // FILE_STREAM_H
