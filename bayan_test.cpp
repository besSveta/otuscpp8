/*
 * bayan_test.cpp
 *
 *  Created on: 17 авг. 2019 г.
 *      Author: sveta
 */


#define BOOST_TEST_MODULE ip_filter_test_module
#include <boost/test/included/unit_test.hpp>
#include "bayan_lib.h"

BOOST_AUTO_TEST_SUITE(async_test_suite)

BOOST_AUTO_TEST_CASE(async__some_test_case)
{
char *params[1];
params[0]="--scanDir C:\\test";
	DuplicatesFinder finder = DuplicateFinderCreator::GetDuplicatesFinder(1, params);
	  BOOST_REQUIRE_EQUAL(finder.scanDirs.size(), 1);
BOOST_REQUIRE_EQUAL(finder.scanDirs[0], "C:\\test");

}

BOOST_AUTO_TEST_SUITE_END()


