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

#ifndef BLOCK_H
#define BLOCK_H

#include "context.h"
#include "mint/compiler/build_context.h"
#include "mint/compiler/type_annotation.h"
#include "mint/program/symbol.h"
#include "case_table.h"
#include "catch_context.h"
#include "branch.h"
#include <cstddef>
#include <memory>
#include <vector>

namespace mint {

struct OverrideBackup {
	std::size_t index = invalid_index;
	const TypeAnnotation* type = nullptr;
};

struct Block {
	virtual ~Block() = default;

	[[nodiscard]] virtual BuildContext::BlockType type() const = 0;

	std::size_t retrieve_point_count = 0;
	std::vector<const Symbol*> scoped_symbols;

	[[nodiscard]] virtual std::vector<const Symbol*>* get_condition_scoped_symbols() const;
	[[nodiscard]] virtual std::vector<const Symbol*>* get_range_loop_scoped_symbols() const;

	[[nodiscard]] virtual bool is_terminated() const = 0;
	virtual void set_terminated(bool terminated) = 0;

	[[nodiscard]] virtual bool is_breakable() const;
	[[nodiscard]] virtual bool is_continuable() const;
};

struct BreakableBlock : Block {

	Branch::ForwardNodeIndex* forward = nullptr;

	[[nodiscard]] bool is_breakable() const final;
};

struct ContinuableBlock : BreakableBlock {

	Branch::BackwardNodeIndex* backward = nullptr;

	[[nodiscard]] bool is_continuable() const final;
};

struct ConditionalLoopBlock : ContinuableBlock {
	explicit ConditionalLoopBlock(Context& context);
	~ConditionalLoopBlock() override;

	[[nodiscard]] BuildContext::BlockType type() const final;

	std::unique_ptr<ConditionContext> condition_context;

	[[nodiscard]] std::vector<const Symbol*>* get_condition_scoped_symbols() const final;

	[[nodiscard]] bool is_terminated() const override;
	void set_terminated(bool terminated) override;

	Context& context;
	std::vector<OverrideBackup> dynamic_overrides_backup;
};

struct CustomRangeLoopBlock : ContinuableBlock {
	explicit CustomRangeLoopBlock(Context& context);
	~CustomRangeLoopBlock() override;

	[[nodiscard]] BuildContext::BlockType type() const final;

	std::unique_ptr<ConditionContext> condition_context;
	std::unique_ptr<RangeLoopContext> range_loop_context;

	[[nodiscard]] std::vector<const Symbol*>* get_condition_scoped_symbols() const final;
	[[nodiscard]] std::vector<const Symbol*>* get_range_loop_scoped_symbols() const final;

	[[nodiscard]] bool is_terminated() const override;
	void set_terminated(bool terminated) override;

	Context& context;
	std::vector<OverrideBackup> dynamic_overrides_backup;
};

struct RangeLoopBlock : ContinuableBlock {
	explicit RangeLoopBlock(Context& context);
	~RangeLoopBlock() override;

	[[nodiscard]] BuildContext::BlockType type() const final;

	std::unique_ptr<ConditionContext> condition_context;
	std::unique_ptr<RangeLoopContext> range_loop_context;

	[[nodiscard]] std::vector<const Symbol*>* get_condition_scoped_symbols() const final;
	[[nodiscard]] std::vector<const Symbol*>* get_range_loop_scoped_symbols() const final;

	[[nodiscard]] bool is_terminated() const override;
	void set_terminated(bool terminated) override;

	Context& context;
	std::vector<OverrideBackup> dynamic_overrides_backup;
};

struct SwitchBlock : BreakableBlock {
	explicit SwitchBlock(Context& context);
	~SwitchBlock() override;

	[[nodiscard]] BuildContext::BlockType type() const final;

	std::unique_ptr<ConditionContext> condition_context;
	CaseTable case_table;

	[[nodiscard]] std::vector<const Symbol*>* get_condition_scoped_symbols() const final;

	[[nodiscard]] bool is_terminated() const override;
	void set_terminated(bool terminated) override;

	void setup_type_overrides();
	void reset_type_overrides();

	Context& context;
	std::vector<OverrideBackup> dynamic_overrides_backup;
};

struct IfBlock : Block {
	explicit IfBlock(Context& context);
	~IfBlock() override;

	[[nodiscard]] BuildContext::BlockType type() const final;

	std::unique_ptr<ConditionContext> condition_context;

	[[nodiscard]] std::vector<const Symbol*>* get_condition_scoped_symbols() const final;

	[[nodiscard]] bool is_terminated() const override;
	void set_terminated(bool terminated) override;

	Context& context;
	std::vector<OverrideBackup> dynamic_overrides_backup;
};

struct ElifBlock : Block {
	explicit ElifBlock(Context& context);
	~ElifBlock() override;

	[[nodiscard]] BuildContext::BlockType type() const final;

	std::unique_ptr<ConditionContext> condition_context;

	[[nodiscard]] std::vector<const Symbol*>* get_condition_scoped_symbols() const final;

	[[nodiscard]] bool is_terminated() const override;
	void set_terminated(bool terminated) override;

	Context& context;
	std::vector<OverrideBackup> dynamic_overrides_backup;
};

struct ElseBlock : Block {
	[[nodiscard]] BuildContext::BlockType type() const final;

	[[nodiscard]] bool is_terminated() const override;
	void set_terminated(bool terminated) override;
};

struct TryBlock : Block {
	[[nodiscard]] BuildContext::BlockType type() const final;

	[[nodiscard]] bool is_terminated() const override;
	void set_terminated(bool terminated) override;
};

struct CatchBlock : Block {
	[[nodiscard]] BuildContext::BlockType type() const final;

	[[nodiscard]] bool is_terminated() const override;
	void set_terminated(bool terminated) override;

	CatchContext catch_context;
};

struct PrintBlock : Block {
	[[nodiscard]] BuildContext::BlockType type() const final;

	[[nodiscard]] bool is_terminated() const override;
	void set_terminated(bool terminated) override;
};

}

#endif // BLOCK_H
