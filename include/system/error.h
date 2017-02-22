#ifndef ERROR_H
#define ERROR_H

#include <functional>

void error(const char *format, ...);

int add_error_callback(std::function<void(void)> on_error);
void remove_error_callback(int id);

void set_exit_callback(std::function<void(void)> on_exit);

#endif // ERROR_H
