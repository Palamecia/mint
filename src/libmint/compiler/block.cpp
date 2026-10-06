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
#include "control_flow.h"
#include "mint/compiler/build_context.h"
#include "mint/program/symbol.h"
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

ControlFlowSet mint::Block::flow() const {
	return _flow;
}

void mint::Block::append_flow(ControlFlowSet flow) {
	_flow = sequence_flow(_flow, flow);
}

void mint::Block::terminate(ControlFlow flow) {
	if (has_flow(_flow, ControlFlow::next)) {
		_flow = without_flow(_flow, ControlFlow::next) | flow_flag(flow);
	}
}

bool mint::Block::is_terminated() const {
	const auto current_flow = flow();
	const auto terminating_flows = flow_flag(ControlFlow::exit) | flow_flag(ControlFlow::raise);
	return current_flow != 0 && (current_flow & ~terminating_flows) == 0;
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

ControlFlowSet mint::ConditionalLoopBlock::flow() const {
	auto result = without_flow(Block::flow(), ControlFlow::break_loop);
	result = without_flow(result, ControlFlow::continue_loop);
	return result | flow_flag(ControlFlow::next);
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

ControlFlowSet mint::CustomRangeLoopBlock::flow() const {
	auto result = without_flow(Block::flow(), ControlFlow::break_loop);
	result = without_flow(result, ControlFlow::continue_loop);
	return result | flow_flag(ControlFlow::next);
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

ControlFlowSet mint::RangeLoopBlock::flow() const {
	auto result = without_flow(Block::flow(), ControlFlow::break_loop);
	result = without_flow(result, ControlFlow::continue_loop);
	return result | flow_flag(ControlFlow::next);
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

ControlFlowSet mint::SwitchBlock::flow() const {
	auto following = flow_flag(ControlFlow::switch_exit);
	ControlFlowSet outcomes = 0;

	for (const auto& body : std::views::reverse(case_table.bodies)) {
		auto body_flow = body.flow;
		if (has_flow(body_flow, ControlFlow::break_switch)) {
			body_flow = without_flow(body_flow, ControlFlow::break_switch);
			body_flow = body_flow | flow_flag(ControlFlow::switch_exit);
		}
		if (has_flow(body_flow, ControlFlow::next)) {
			body_flow = without_flow(body_flow, ControlFlow::next);
			body_flow = body_flow | following;
		}
		following = body_flow;
		outcomes = outcomes | body_flow;
	}

	if (has_flow(outcomes, ControlFlow::switch_exit)) {
		outcomes = without_flow(outcomes, ControlFlow::switch_exit);
		outcomes = outcomes | flow_flag(ControlFlow::next);
	}
	if (!case_table.default_case) {
		outcomes = outcomes | flow_flag(ControlFlow::next);
	}
	return outcomes;
}

void mint::SwitchBlock::append_flow(ControlFlowSet flow) {
	if (case_table.current_body) {
		case_table.current_body->flow = sequence_flow(case_table.current_body->flow, flow);
	}
	else {
		Block::append_flow(flow);
	}
}

void mint::SwitchBlock::terminate(ControlFlow flow) {
	if (case_table.current_body) {
		if (has_flow(case_table.current_body->flow, ControlFlow::next)) {
			case_table.current_body->flow = without_flow(case_table.current_body->flow, ControlFlow::next)
			                                | flow_flag(flow);
		}
	}
	else {
		Block::terminate(flow);
	}
}

void mint::SwitchBlock::setup_type_overrides() {
	::setup_type_overrides(context, dynamic_overrides_backup, *condition_context);
}

void mint::SwitchBlock::reset_type_overrides() {
	cleanup_type_overrides(context, dynamic_overrides_backup);
	condition_context->pending_type_overrides.clear();
}

mint::IfBaseBlock::IfBaseBlock(Context& context) :
    condition_context(std::move(context.current_condition)),
    context(context) {
	scoped_symbols.append_range(condition_context->scoped_symbols);
	if (condition_context->is_narrowing_valid) {
		setup_type_overrides(context, dynamic_overrides_backup, *condition_context);
	}
}

mint::IfBaseBlock::~IfBaseBlock() {
	if (condition_context->is_narrowing_valid) {
		cleanup_type_overrides(context, dynamic_overrides_backup);
	}
}

std::vector<const Symbol*>* mint::IfBaseBlock::get_condition_scoped_symbols() const {
	return &condition_context->scoped_symbols;
}

ControlFlowSet mint::IfBaseBlock::branch_flow() const {
	return Block::flow();
}

BuildContext::BlockType mint::IfBlock::type() const {
	return BuildContext::BlockType::if_type;
}

mint::IfBlock::IfBlock(Context& context) :
    IfBaseBlock(context) {}

ControlFlowSet mint::IfBlock::flow() const {
	return branch_flow() | flow_flag(ControlFlow::next);
}

mint::ElifBlock::ElifBlock(Context& context, std::unique_ptr<IfBaseBlock>&& parent_if_block) :
    IfBaseBlock(context),
    parent_if_block(std::move(parent_if_block)) {}

BuildContext::BlockType mint::ElifBlock::type() const {
	return BuildContext::BlockType::elif_type;
}

ControlFlowSet mint::ElifBlock::branch_flow() const {
	return parent_if_block->branch_flow() | Block::flow();
}

ControlFlowSet mint::ElifBlock::flow() const {
	return branch_flow() | flow_flag(ControlFlow::next);
}

mint::ElseBlock::ElseBlock(std::unique_ptr<IfBaseBlock>&& parent_if_block) :
    parent_if_block(std::move(parent_if_block)) {}

BuildContext::BlockType mint::ElseBlock::type() const {
	return BuildContext::BlockType::else_type;
}

ControlFlowSet mint::ElseBlock::flow() const {
	return parent_if_block->branch_flow() | Block::flow();
}

BuildContext::BlockType mint::TryBlock::type() const {
	return BuildContext::BlockType::try_type;
}

ControlFlowSet mint::TryBlock::flow() const {
	const auto current_flow = Block::flow();
	if (has_flow(current_flow, ControlFlow::raise)) {
		return without_flow(current_flow, ControlFlow::raise) | flow_flag(ControlFlow::next);
	}
	return current_flow;
}

BuildContext::BlockType mint::CatchBlock::type() const {
	return BuildContext::BlockType::catch_type;
}

mint::CatchBlock::CatchBlock(std::unique_ptr<TryBlock>&& parent_try_block) :
    parent_try_block(std::move(parent_try_block)) {}

ControlFlowSet mint::CatchBlock::flow() const {
	const auto try_flow = without_flow(parent_try_block->Block::flow(), ControlFlow::raise);
	return try_flow | Block::flow();
}

BuildContext::BlockType mint::PrintBlock::type() const {
	return BuildContext::BlockType::print_type;
}
