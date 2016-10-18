#ifndef INPUT_STREAM_H
#define INPUT_STREAM_H

#include "System/datastream.h"

class InputStream : public DataStream {
public:
	~InputStream();

	static InputStream &instance();

	bool atEnd() const override;

	bool isValid() const override;
	size_t lineNumber() const override;
	std::string path() const override;

	void next();

protected:
	InputStream();

	int getRawChar() override;
	std::string getLine() override;

private:
	enum Status {
		ready,
		breaking,
		over
	};

	char *m_buffer;
	char *m_cptr;
	size_t m_level;
	Status m_status;
	size_t m_lineNumber;
};

#endif // INPUT_STREAM_H
