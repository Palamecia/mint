#include <cstdint>
#include <gtest/gtest.h>
#include "mint/memory/data.h"
#include "mint/memory/garbage_collector.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"

TEST(reference, default_reference_points_to_none_with_requested_flags) {

	mint::Reference ref(mint::Reference::const_value | mint::Reference::temporary);

	EXPECT_EQ(mint::Data::Format::none, ref.data().format());
	EXPECT_EQ(mint::Reference::const_value | mint::Reference::temporary, ref.flags());
	EXPECT_GE(mint::GarbageCollector::get_refcount(ref.data()), 1);
}

TEST(reference, create_copy_and_move_data_manage_distinct_storage) {

	mint::Reference source = mint::make_reference<mint::Number>(mint::Reference::default_flags,
	    static_cast<std::intmax_t>(7357));
	mint::Reference alias(mint::create_from, source);
	mint::Reference copy(mint::copy_from, source);

	EXPECT_EQ(&source.data(), &alias.data());
	EXPECT_NE(&source.data(), &copy.data());
	EXPECT_EQ(7357, copy.data<mint::Number>().value);

	source.copy_data(mint::make_reference<mint::Boolean>(mint::Reference::default_flags, true));
	EXPECT_EQ(mint::Data::Format::boolean, source.data().format());
	EXPECT_TRUE(source.data<mint::Boolean>().value);
	EXPECT_EQ(mint::Data::Format::number, alias.data().format());

	source.move_data(copy);
	EXPECT_EQ(&copy.data(), &source.data());
}

TEST(reference, root_reference_exposes_referenced_data) {

	mint::RootReference root = mint::make_root_reference<mint::Number>(mint::Reference::default_flags,
	    static_cast<std::intmax_t>(42));

	EXPECT_EQ(mint::Data::Format::number, root.data().format());
	EXPECT_EQ(42, root.data<mint::Number>().value);
}
