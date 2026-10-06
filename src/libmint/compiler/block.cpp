/**
 * Copyright (c) 2026 Gauvain CHÉRY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "block.h"
#include "context.h"
#include "mint/compiler/build_context.h"
#include "mint/program/symbol.h"
#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

using namespace mint;

std::vector<const Symbol*>* mint::Block::get_condition_scoped_symbols() const {
	return nullptr;
}

std::vector<const Symbol*>* mint::Block::get_range_loop_scoped_symbols() const {
	return nullptr;
}

bool mint::Block::is_breakable() const {
	return false;
}

bool mint::Block::is_continuable() const {
	return false;
}

mint::ConditionalLoopBlock::ConditionalLoopBlock(Context& context) :
    condition_scoped_symbols(std::move(context.condition_scoped_symbols)) {
	std::ranges::move(*condition_scoped_symbols, std::back_inserter(block_scoped_symbols));
}

BuildContext::BlockType mint::ConditionalLoopBlock::type() const {
	return BuildContext::BlockType::conditional_loop_type;
}

std::vector<const Symbol*>* mint::ConditionalLoopBlock::get_condition_scoped_symbols() const {
	return condition_scoped_symbols.get();
}

bool mint::ConditionalLoopBlock::is_breakable() const {
	return true;
}

bool mint::ConditionalLoopBlock::is_continuable() const {
	return true;
}

mint::CustomRangeLoopBlock::CustomRangeLoopBlock(Context& context) :
    condition_scoped_symbols(std::move(context.condition_scoped_symbols)),
    range_loop_scoped_symbols(std::move(context.range_loop_scoped_symbols)) {
	std::ranges::move(*condition_scoped_symbols, std::back_inserter(block_scoped_symbols));
}

BuildContext::BlockType mint::CustomRangeLoopBlock::type() const {
	return BuildContext::BlockType::custom_range_loop_type;
}

std::vector<const Symbol*>* mint::CustomRangeLoopBlock::get_condition_scoped_symbols() const {
	return condition_scoped_symbols.get();
}

std::vector<const Symbol*>* mint::CustomRangeLoopBlock::get_range_loop_scoped_symbols() const {
	return range_loop_scoped_symbols.get();
}

bool mint::CustomRangeLoopBlock::is_breakable() const {
	return true;
}

bool mint::CustomRangeLoopBlock::is_continuable() const {
	return true;
}

mint::RangeLoopBlock::RangeLoopBlock(Context& context) :
    condition_scoped_symbols(std::move(context.condition_scoped_symbols)),
    range_loop_scoped_symbols(std::move(context.range_loop_scoped_symbols)) {
	std::ranges::move(*condition_scoped_symbols, std::back_inserter(block_scoped_symbols));
}

BuildContext::BlockType mint::RangeLoopBlock::type() const {
	return BuildContext::BlockType::range_loop_type;
}

std::vector<const Symbol*>* mint::RangeLoopBlock::get_condition_scoped_symbols() const {
	return condition_scoped_symbols.get();
}

std::vector<const Symbol*>* mint::RangeLoopBlock::get_range_loop_scoped_symbols() const {
	return range_loop_scoped_symbols.get();
}

bool mint::RangeLoopBlock::is_breakable() const {
	return true;
}

bool mint::RangeLoopBlock::is_continuable() const {
	return true;
}

mint::SwitchBlock::SwitchBlock(Context& context) :
    condition_scoped_symbols(std::move(context.condition_scoped_symbols)) {
	std::ranges::move(*condition_scoped_symbols, std::back_inserter(block_scoped_symbols));
}

BuildContext::BlockType mint::SwitchBlock::type() const {
	return BuildContext::BlockType::switch_type;
}

std::vector<const Symbol*>* mint::SwitchBlock::get_condition_scoped_symbols() const {
	return condition_scoped_symbols.get();
}

bool mint::SwitchBlock::is_breakable() const {
	return true;
}

mint::IfBlock::IfBlock(Context& context) :
    condition_scoped_symbols(std::move(context.condition_scoped_symbols)) {
	std::ranges::move(*condition_scoped_symbols, std::back_inserter(block_scoped_symbols));
}

BuildContext::BlockType mint::IfBlock::type() const {
	return BuildContext::BlockType::if_type;
}

std::vector<const Symbol*>* mint::IfBlock::get_condition_scoped_symbols() const {
	return condition_scoped_symbols.get();
}

mint::ElifBlock::ElifBlock(Context& context) :
    condition_scoped_symbols(std::move(context.condition_scoped_symbols)) {
	std::ranges::move(*condition_scoped_symbols, std::back_inserter(block_scoped_symbols));
}

BuildContext::BlockType mint::ElifBlock::type() const {
	return BuildContext::BlockType::elif_type;
}

std::vector<const Symbol*>* mint::ElifBlock::get_condition_scoped_symbols() const {
	return condition_scoped_symbols.get();
}

BuildContext::BlockType mint::ElseBlock::type() const {
	return BuildContext::BlockType::else_type;
}

BuildContext::BlockType mint::TryBlock::type() const {
	return BuildContext::BlockType::try_type;
}

BuildContext::BlockType mint::CatchBlock::type() const {
	return BuildContext::BlockType::catch_type;
}

BuildContext::BlockType mint::PrintBlock::type() const {
	return BuildContext::BlockType::print_type;
}
