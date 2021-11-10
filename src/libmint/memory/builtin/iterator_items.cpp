#include "iterator_items.h"
#include "memory/functiontool.h"
#include "memory/builtin/string.h"
#include "system/utf8iterator.h"

using namespace _mint_iterator;
using namespace mint;
using namespace std;

LocalPool<item> items_data::g_pool;

items_iterator::items_iterator(item *impl) : m_impl(impl) {

}

Iterator::ctx_type::value_type &items_iterator::get() {
	return m_impl->value;
}

bool items_iterator::compare(data_iterator *other) const {
	return m_impl != static_cast<items_iterator *>(other)->m_impl;
}

data_iterator *items_iterator::copy() {
	return new items_iterator(m_impl);
}

void items_iterator::next() {
	m_impl = m_impl->next;
}

items_data::items_data(Reference &ref) {
	switch (ref.data()->format) {
	case Data::fmt_none:
		break;
	case Data::fmt_object:
		switch (ref.data<Object>()->metadata->metatype()) {
		case Class::string:
			for (utf8iterator i = ref.data<String>()->str.begin(); i != ref.data<String>()->str.end(); ++i) {
				emplace_back(create_string(*i));
			}
			break;
		case Class::array:
			for (auto &item : ref.data<Array>()->values) {
				emplace_back(array_get_item(item));
			}
			break;
		case Class::hash:
			for (auto &item : ref.data<Hash>()->values) {
				WeakReference element(Reference::const_address | Reference::const_value, Reference::alloc<Iterator>());
				iterator_insert(element.data<Iterator>(), hash_get_key(item));
				iterator_insert(element.data<Iterator>(), hash_get_value(item));
				emplace_back(move(element));
			}
			break;
		case Class::iterator:
			for (Reference &item : ref.data<Iterator>()->ctx) {
				emplace_back(WeakReference::share(item));
			}
			break;
		default:
			emplace_back(move(ref));
			break;
		}
		break;
	default:
		emplace_back(move(ref));
		break;
	}
}

items_data::items_data(const items_data &other) {
	for (item *it = other.m_head; it != nullptr; it = it->next) {
		emplace_back(WeakReference::share(it->value));
	}
}

items_data::~items_data() {
	item *it = m_head;
	while (it != nullptr) {
		item *tmp = it;
		it = tmp->next;
		g_pool.free(tmp);
	}
}

void items_data::mark() {
	for (item *it = m_head; it != nullptr; it = it->next) {
		it->value.data()->mark();
	}
}

_mint_iterator::data *items_data::copy() const {
	return new items_data(*this);
}

Iterator::ctx_type::type items_data::getType() {
	return Iterator::ctx_type::items;
}

data_iterator *items_data::begin() {
	return new items_iterator(m_head);
}

data_iterator *items_data::end() {
	return new items_iterator(nullptr);
}

Iterator::ctx_type::value_type &items_data::front() {
	return m_head->value;
}

Iterator::ctx_type::value_type &items_data::back() {
	return m_tail->value;
}

void items_data::emplace_front(Iterator::ctx_type::value_type &&value) {
	if (m_head) {
		item *tmp = g_pool.alloc(move(value));
		tmp->next = m_head;
		m_head->prev = tmp;
		m_head = tmp;
	}
	else {
		m_head = m_tail = g_pool.alloc(move(value));
	}
}

void items_data::emplace_back(Iterator::ctx_type::value_type &&value) {
	if (m_tail) {
		item *tmp = g_pool.alloc(move(value));
		tmp->prev = m_tail;
		m_tail->next = tmp;
		m_tail = tmp;
	}
	else {
		m_head = m_tail = g_pool.alloc(move(value));
	}
}

void items_data::pop_front() {

	item *tmp = m_head;

	if (m_head != m_tail) {
		m_head = tmp->next;
	}
	else {
		m_head = m_tail = nullptr;
	}

	g_pool.free(tmp);
}

void items_data::pop_back() {

	item *tmp = m_tail;

	if (m_head != m_tail) {
		m_tail = tmp->prev;
	}
	else {
		m_head = m_tail = nullptr;
	}

	g_pool.free(tmp);
}

void items_data::finalize() {

}

void items_data::clear() {

	item *it = m_head;

	while (it != nullptr) {
		item *tmp = it;
		it = tmp->next;
		g_pool.free(tmp);
	}

	m_head = m_tail = nullptr;
}

size_t items_data::size() const {

	size_t count = 0;

	for (item *it = m_head; it != nullptr; it = it->next) {
		++count;
	}

	return count;
}

bool items_data::empty() const {
	return m_head == nullptr;
}
