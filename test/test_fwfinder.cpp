#include <gtest/gtest.h>

#include <fwfinder.hpp>

TEST(FwFinder, BasicAssertions) {
    auto finder = Fw::Finder();
    Fw::list_all();
}
