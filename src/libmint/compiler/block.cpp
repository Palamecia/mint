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
#include "case_table.h"
#include "context.h"
#include "mint/compiler/build_context.h"
#include "mint/program/symbol.h"
#include <algorithm>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

using namespace mint;

namespace {

void setup_type_overrides(Context& context, std::vector<OverrideBackup>& dynamic_overrides_backup,
    const ConditionContext& condition_context) {
	dynamic_overrides_backup.reserve(condition_context.pending_type_overrides.size());
	for (const auto& override : condition_context.pending_type_overrides) {
		if (override.index >= context.active_type_overrides.size()) {
			context.active_type_overrides.resize(override.index + 1);
		}
		auto& active_override = context.active_type_overrides.at(override.index);
		dynamic_overrides_backup.push_back({
		    .index = override.index,
		    .type = active_override,
		});
		active_override = &override.type;
	}
}

void cleanup_type_overrides(Context& context, const std::vector<OverrideBackup>& dynamic_overrides_backup) {
	for (const auto& backup : dynamic_overrides_backup) {
		if (backup.index < context.active_type_overrides.size()) {
			context.active_type_overrides.at(backup.index) = backup.type;
		}
	}
}

}

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

bool mint::BreakableBlock::is_breakable() const {
	return true;
}

bool mint::ContinuableBlock::is_continuable() const {
	return true;
}

mint::ConditionalLoopBlock::ConditionalLoopBlock(Context& context) :
    condition_context(std::move(context.current_condition)),
    context(context) {
	scoped_symbols.append_range(condition_context->scoped_symbols);
	if (condition_context->is_narrowing_valid) {
		setup_type_overrides(context, dynamic_overrides_backup, *condition_context);
	}
}

mint::ConditionalLoopBlock::~ConditionalLoopBlock() {
	if (condition_context->is_narrowing_valid) {
		cleanup_type_overrides(context, dynamic_overrides_backup);
	}
}

BuildContext::BlockType mint::ConditionalLoopBlock::type() const {
	return BuildContext::BlockType::conditional_loop_type;
}

std::vector<const Symbol*>* mint::ConditionalLoopBlock::get_condition_scoped_symbols() const {
	return &condition_context->scoped_symbols;
}

bool mint::ConditionalLoopBlock::is_terminated() const {
	return false;
}

void mint::ConditionalLoopBlock::set_terminated(bool terminated) {
	(void)terminated;
}

mint::CustomRangeLoopBlock::CustomRangeLoopBlock(Context& context) :
    condition_context(std::move(context.current_condition)),
    range_loop_context(std::move(context.current_range_loop)),
    context(context) {
	scoped_symbols.append_range(condition_context->scoped_symbols);
	if (condition_context->is_narrowing_valid) {
		setup_type_overrides(context, dynamic_overrides_backup, *condition_context);
	}
}

mint::CustomRangeLoopBlock::~CustomRangeLoopBlock() {
	if (condition_context->is_narrowing_valid) {
		cleanup_type_overrides(context, dynamic_overrides_backup);
	}
}

BuildContext::BlockType mint::CustomRangeLoopBlock::type() const {
	return BuildContext::BlockType::custom_range_loop_type;
}

std::vector<const Symbol*>* mint::CustomRangeLoopBlock::get_condition_scoped_symbols() const {
	return &condition_context->scoped_symbols;
}

std::vector<const Symbol*>* mint::CustomRangeLoopBlock::get_range_loop_scoped_symbols() const {
	return &range_loop_context->scoped_symbols;
}

bool mint::CustomRangeLoopBlock::is_terminated() const {
	return false;
}

void mint::CustomRangeLoopBlock::set_terminated(bool terminated) {
	(void)terminated;
}

mint::RangeLoopBlock::RangeLoopBlock(Context& context) :
    condition_context(std::move(context.current_condition)),
    range_loop_context(std::move(context.current_range_loop)),
    context(context) {
	scoped_symbols.append_range(condition_context->scoped_symbols);
	if (condition_context->is_narrowing_valid) {
		setup_type_overrides(context, dynamic_overrides_backup, *condition_context);
	}
}

mint::RangeLoopBlock::~RangeLoopBlock() {
	if (condition_context->is_narrowing_valid) {
		cleanup_type_overrides(context, dynamic_overrides_backup);
	}
}

BuildContext::BlockType mint::RangeLoopBlock::type() const {
	return BuildContext::BlockType::range_loop_type;
}

std::vector<const Symbol*>* mint::RangeLoopBlock::get_condition_scoped_symbols() const {
	return &condition_context->scoped_symbols;
}

std::vector<const Symbol*>* mint::RangeLoopBlock::get_range_loop_scoped_symbols() const {
	return &range_loop_context->scoped_symbols;
}

bool mint::RangeLoopBlock::is_terminated() const {
	return false;
}

void mint::RangeLoopBlock::set_terminated(bool terminated) {
	(void)terminated;
}

mint::SwitchBlock::SwitchBlock(Context& context) :
    condition_context(std::move(context.current_condition)),
    context(context) {
	scoped_symbols.append_range(condition_context->scoped_symbols);
}

mint::SwitchBlock::~SwitchBlock() {
	cleanup_type_overrides(context, dynamic_overrides_backup);
}

BuildContext::BlockType mint::SwitchBlock::type() const {
	return BuildContext::BlockType::switch_type;
}

std::vector<const Symbol*>* mint::SwitchBlock::get_condition_scoped_symbols() const {
	return &condition_context->scoped_symbols;
}

bool mint::SwitchBlock::is_terminated() const {
	if (!case_table.default_case.has_value()) {
		return false;
	}
	return std::ranges::all_of(std::views::values(case_table.cases), &CaseTable::Case::terminated)
	       && case_table.default_case->terminated;
}

void mint::SwitchBlock::set_terminated(bool terminated) {
	if (case_table.default_case) {
		case_table.default_case->terminated = terminated;
	}
	else {
		case_table.cases.back().second.terminated = terminated;
	}
}

void mint::SwitchBlock::setup_type_overrides() {
	::setup_type_overrides(context, dynamic_overrides_backup, *condition_context);
}

void mint::SwitchBlock::reset_type_overrides() {
	cleanup_type_overrides(context, dynamic_overrides_backup);
	condition_context->pending_type_overrides.clear();
}

mint::IfBlock::IfBlock(Context& context) :
    condition_context(std::move(context.current_condition)),
    context(context) {
	scoped_symbols.append_range(condition_context->scoped_symbols);
	if (condition_context->is_narrowing_valid) {
		setup_type_overrides(context, dynamic_overrides_backup, *condition_context);
	}
}

mint::IfBlock::~IfBlock() {
	if (condition_context->is_narrowing_valid) {
		cleanup_type_overrides(context, dynamic_overrides_backup);
	}
}

BuildContext::BlockType mint::IfBlock::type() const {
	return BuildContext::BlockType::if_type;
}

std::vector<const Symbol*>* mint::IfBlock::get_condition_scoped_symbols() const {
	return &condition_context->scoped_symbols;
}

bool mint::IfBlock::is_terminated() const {
	return false;
}

void mint::IfBlock::set_terminated(bool terminated) {
	(void)terminated;
}

mint::ElifBlock::ElifBlock(Context& context) :
    condition_context(std::move(context.current_condition)),
    context(context) {
	scoped_symbols.append_range(condition_context->scoped_symbols);
	if (condition_context->is_narrowing_valid) {
		setup_type_overrides(context, dynamic_overrides_backup, *condition_context);
	}
}

mint::ElifBlock::~ElifBlock() {
	if (condition_context->is_narrowing_valid) {
		cleanup_type_overrides(context, dynamic_overrides_backup);
	}
}

BuildContext::BlockType mint::ElifBlock::type() const {
	return BuildContext::BlockType::elif_type;
}

std::vector<const Symbol*>* mint::ElifBlock::get_condition_scoped_symbols() const {
	return &condition_context->scoped_symbols;
}

bool mint::ElifBlock::is_terminated() const {
	return false;
}

void mint::ElifBlock::set_terminated(bool terminated) {
	(void)terminated;
}

BuildContext::BlockType mint::ElseBlock::type() const {
	return BuildContext::BlockType::else_type;
}

bool mint::ElseBlock::is_terminated() const {
	return false;
}

void mint::ElseBlock::set_terminated(bool terminated) {
	(void)terminated;
}

BuildContext::BlockType mint::TryBlock::type() const {
	return BuildContext::BlockType::try_type;
}

bool mint::TryBlock::is_terminated() const {
	return false;
}

void mint::TryBlock::set_terminated(bool terminated) {
	(void)terminated;
}

BuildContext::BlockType mint::CatchBlock::type() const {
	return BuildContext::BlockType::catch_type;
}

bool mint::CatchBlock::is_terminated() const {
	return false;
}

void mint::CatchBlock::set_terminated(bool terminated) {
	(void)terminated;
}

BuildContext::BlockType mint::PrintBlock::type() const {
	return BuildContext::BlockType::print_type;
}

bool mint::PrintBlock::is_terminated() const {
	return false;
}

void mint::PrintBlock::set_terminated(bool terminated) {
	(void)terminated;
}
