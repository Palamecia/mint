#ifndef FILE_STREAM_H
#define FILE_STREAM_H

#include "System/datastream.h"

#include <string>

class FileStream : public DataStream {
public:
	FileStream(const std::string &name);
	~FileStream();

	bool atEnd() const override;

	bool isValid() const override;
	size_t lineNumber() const override;
	std::string path() const override;

protected:
	int getRawChar() override;
	std::string getLine() override;

private:
	FILE *m_file;
	std::string m_path;
	size_t m_lineNumber;
	bool m_over;
};

#endif // FILE_STREAM_H