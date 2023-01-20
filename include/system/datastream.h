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

	void setNewLineCallback(const std::function<void(size_t)> &callback);
	size_t lineNumber() const;
	std::string lineError();

protected:
	virtual int readChar() = 0;
	virtual int nextBufferedChar() = 0;

private:
	void continueLine();
	void beginLine();
	void endLine();

	enum State { on_new_line, on_first_char, on_reading };
	std::function<void(size_t)> m_newLineCallback;
	size_t m_lineNumber;
	State m_state;

	std::string m_cachedLine;
};

}

#endif // MINT_DATASTREAM_H
