#ifndef DATA_STREAM_H
#define DATA_STREAM_H

#include <string>

class DataStream {
public:
	DataStream();
	virtual ~DataStream();

	int getChar();
	virtual bool atEnd() const = 0;

	virtual bool isValid() const = 0;
	virtual size_t lineNumber() const = 0;
	virtual std::string path() const = 0;

	std::string lineError();

protected:
	virtual int getRawChar() = 0;
	virtual std::string getLine() = 0;

private:
	std::string m_cachedLine;
	bool m_shouldClearCache;
};

#endif // DATA_STREAM_H
