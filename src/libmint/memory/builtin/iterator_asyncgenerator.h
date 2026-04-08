#ifndef LIBMINT_MEMORY_BUILTIN_ITERATOR_ASYNCGENERATOR_H
#define LIBMINT_MEMORY_BUILTIN_ITERATOR_ASYNCGENERATOR_H

#include "iterator_items.h"
#include "iterator_p.h"
#include "mint/ast/cursor.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace mint::internal {

class AsyncGeneratorData : public ItemsIteratorData {
public:
	AsyncGeneratorData(Coroutine& coroutine, std::size_t stack_size);
	AsyncGeneratorData(AsyncGeneratorData&&) = delete;
	AsyncGeneratorData(const AsyncGeneratorData& other);
	~AsyncGeneratorData() override = default;

	AsyncGeneratorData& operator=(AsyncGeneratorData&&) = delete;
	AsyncGeneratorData& operator=(const AsyncGeneratorData&) = delete;

	[[nodiscard]] std::unique_ptr<IteratorData> copy() override;
	void mark() override;

	[[nodiscard]] mint::Iterator::Context::Type get_type() const override;

	void yield(mint::Cursor& cursor, mint::Iterator::Context::value_type&& value,
	    Iterator::ResumeKind resume_kind) override;
	void next(mint::Cursor& cursor) override;

	void finalize(mint::Cursor& cursor) override;

private:
	std::reference_wrapper<Coroutine> _coroutine;
	std::unique_ptr<mint::SavedState> _state;

	std::vector<mint::WeakReference> _stored_stack;
	std::size_t _stack_size;
};

}

#endif // LIBMINT_MEMORY_BUILTIN_ITERATOR_ASYNCGENERATOR_H
