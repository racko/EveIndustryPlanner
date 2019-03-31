#include "outdatedHistories.h"
#include <iostream>
#include "profiling.h"
#include <boost/numeric/conversion/cast.hpp>

std::vector<size_t> outdatedHistories(const sqlite::dbPtr& db) {
    auto getTypes = sqlite::prepare(db, "select types.typeid from types left outer join last_history_update on types.typeid = last_history_update.typeid where date <= strftime('%s', 'now', '-1 days') or date is null;");
	
	int rc;
	std::vector<size_t> types;
	types.reserve(10000);
	std::cout << "determining outdated histories\n";
    TIME(
        for (rc = sqlite::step(getTypes); rc == sqlite::ROW; rc = sqlite::step(getTypes)) {
			types.push_back(boost::numeric_cast<size_t>(sqlite::column<int64_t>(getTypes, 0)));
        }
    );
	std::cout << types.size() << " histories have to be updated.\n";
	return types;
}
