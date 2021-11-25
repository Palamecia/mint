#ifndef MINT_DATASTREAM_H
#define MINT_DATASTREAM_H

#include "config.h"

#include <functional>
#include <string>

namespace mint {

class MINT_EXPORT DataStream {
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

private:
	void nextLine();
	void startLine();

	size_t m_lineNumber;
	std::function<void(size_t)> m_lineEndCallback;

	std::string m_cachedLine;
	bool m_newLine;
};

}

#endif // MINT_DATASTREAM_H
