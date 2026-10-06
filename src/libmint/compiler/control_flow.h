/**
 * Copyright (c) 2026 Gauvain CHÉRY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */

#ifndef LIBMINT_COMPILER_CONTROL_FLOW_H
#define LIBMINT_COMPILER_CONTROL_FLOW_H

#include <type_traits>

namespace mint {

enum class ControlFlow : unsigned {
	next = 0x0001,
	exit = 0x0002,
	break_switch = 0x0004,
	break_loop = 0x0008,
	continue_loop = 0x0010,
	switch_exit = 0x0020,
	raise = 0x0040,
};

using ControlFlowSet = std::underlying_type_t<ControlFlow>;

[[nodiscard]] constexpr ControlFlowSet flow_flag(ControlFlow flow) {
	return static_cast<ControlFlowSet>(flow);
}

[[nodiscard]] constexpr bool has_flow(ControlFlowSet flows, ControlFlow flow) {
	return (flows & flow_flag(flow)) != 0;
}

[[nodiscard]] constexpr ControlFlowSet without_flow(ControlFlowSet flows, ControlFlow flow) {
	return flows & ~flow_flag(flow);
}

[[nodiscard]] constexpr ControlFlowSet sequence_flow(ControlFlowSet current, ControlFlowSet next) {
	if (!has_flow(current, ControlFlow::next)) {
		return current;
	}
	return without_flow(current, ControlFlow::next) | next;
}

}

#endif // LIBMINT_COMPILER_CONTROL_FLOW_H
