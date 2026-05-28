#include <cstdint>
#include <gtest/gtest.h>
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/garbage_collector.h"

TEST(garbage_collector, singleton_refs_and_threshold_controls) {

	auto& gc = mint::GarbageCollector::instance();
	const auto old_threshold = gc.get_threshold();

	gc.set_threshold(gc.get_count());
	EXPECT_FALSE(gc.is_threshold_exceeded());

	gc.set_threshold(gc.get_count() > 0 ? gc.get_count() - 1 : 0);
	EXPECT_EQ(gc.get_count() > gc.get_threshold(), gc.is_threshold_exceeded());

	mint::Reference& none = gc.none_ref();
	mint::Reference& null = gc.null_ref();
	EXPECT_EQ(mint::Data::Format::none, none.data().format());
	EXPECT_EQ(mint::Data::Format::null, null.data().format());
	EXPECT_EQ(&none, &gc.none_ref());
	EXPECT_EQ(&null, &gc.null_ref());

	gc.set_threshold(old_threshold);
}

TEST(garbage_collector, defer_scope_delays_collection_requests) {

	auto& gc = mint::GarbageCollector::instance();

	{
		mint::GarbageCollectorDeferScope scope;
		EXPECT_TRUE(gc.is_deferred());

		mint::Reference ref = mint::make_reference<mint::Number>(mint::Reference::default_flags,
		    static_cast<std::intmax_t>(7357));
		EXPECT_EQ(0, gc.collect());
		EXPECT_EQ(mint::Data::Format::number, ref.data().format());
	}

	EXPECT_FALSE(gc.is_deferred());
}
