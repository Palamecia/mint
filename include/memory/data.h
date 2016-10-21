#ifndef DATA_H
#define DATA_H

struct Data {
	enum Format {
		fmt_none,
		fmt_null,
		fmt_number,
		fmt_object,
		fmt_function
	};
	Format format;
	Data() { format = fmt_none; }
	virtual ~Data() {}
};

#endif // DATA_H
