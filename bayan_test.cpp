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
char*  params[3];
	params[0] = const_cast<char *>("");
	params[1] = const_cast<char *>("--scanDirs");
	params[2] = const_cast<char *>("test");	
	DuplicatesFinder finder = std::get<0>(DuplicateFinderCreator::GetDuplicatesFinder(3,params));
BOOST_REQUIRE_EQUAL(finder.scanDirs[0], "test");

}

BOOST_AUTO_TEST_SUITE_END()


