#ifndef INPUT_STREAM_H
#define INPUT_STREAM_H

#include "system/datastream.h"

namespace mint {

class MINT_EXPORT InputStream : public DataStream {
public:
	~InputStream();

	static InputStream &instance();

	bool atEnd() const override;

	bool isValid() const override;
	std::string path() const override;

	void next();

protected:
	InputStream();

	void updateBuffer(const char *prompt);

	int readChar() override;
	int nextBufferedChar() override;

private:
	enum Status {
		ready,
		continuing,
		breaking,
		over
	};

	char *m_buffer;
	char *m_cptr;
	size_t m_level;
	Status m_status;
};

}

#endif // INPUT_STREAM_H
