#ifndef MINT_BUFFERSTREAM_H
#define MINT_BUFFERSTREAM_H

#include "system/datastream.h"

namespace mint {

class MINT_EXPORT BufferStream : public DataStream {
public:
	BufferStream(const std::string &buffer);
	~BufferStream();

	bool atEnd() const override;

	bool isValid() const override;
	std::string path() const override;

protected:
	int readChar() override;
	int nextBufferedChar() override;

private:
	enum Status {
		ready,
		over
	};

	const char *m_buffer;
	const char *m_cptr;
	Status m_status;
};

}

#endif // MINT_BUFFERSTREAM_H
