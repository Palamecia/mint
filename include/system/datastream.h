#ifndef DATA_STREAM_H
#define DATA_STREAM_H

#include <functional>
#include <string>

class DataStream {
public:
	DataStream();
	virtual ~DataStream();

	int getChar();
	virtual bool atEnd() const = 0;

	virtual bool isValid() const = 0;
	virtual std::string path() const = 0;

	void setLineEndCallback(std::function<void(size_t)> callback);
	size_t lineNumber() const;
	std::string lineError();

protected:
	virtual int readChar() = 0;
	virtual int nextBufferedChar() = 0;

	void nextLine();

private:
	size_t m_lineNumber;
	std::function<void(size_t)> m_lineEndCallback;

	std::string m_cachedLine;
	bool m_newLine;
};

#endif // DATA_STREAM_H
