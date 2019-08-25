#include "bayan_lib.h"
int main(int ac, char *argv[]) {
	auto finder = DuplicateFinderCreator::GetDuplicatesFinder(ac, argv);
	if (std::get<1>(finder) == true)
		std::get<0>(finder).printDuplicates();
}
