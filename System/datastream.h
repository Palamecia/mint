#ifndef DATA_STREAM_H
#define DATA_STREAM_H

#include <string>

class DataStream {
public:
	DataStream();
	virtual ~DataStream();

	virtual int getChar() = 0;
	virtual bool atEnd() const = 0;

	virtual bool isValid() const = 0;
	virtual size_t lineNumber() const = 0;
	virtual std::string path() const = 0;

	std::string lineError();

protected:
	void addToCache(char c);
	void clearCache();

private:
	std::string m_cachedLine;
	bool m_shouldClearCache;
};

#endif // DATA_STREAM_H
