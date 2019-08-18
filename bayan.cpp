#include "bayan.h"
int main(int ac, char *argv[]) {
	auto finder = DuplicateFinderCreator::GetDuplicatesFinder(ac, argv);
	finder.printDuplicates();
}
