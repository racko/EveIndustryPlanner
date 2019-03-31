#include "outdatedOrders.h"
#include <iostream>
#include "profiling.h"
#include <boost/numeric/conversion/cast.hpp>

std::vector<std::size_t> outdatedOrders(const sqlite::dbPtr& db) {
    auto getTypes = sqlite::prepare(db, "select types.typeid from types left outer join robust_price using (typeid) where date <= strftime('%s', 'now', '-5 minutes') or date is null;");
	
	int rc;
	std::vector<std::size_t> types;
	types.reserve(10000);
	std::cout << "determining outdated orders\n";
    TIME(
        for (rc = sqlite::step(getTypes); rc == sqlite::ROW; rc = sqlite::step(getTypes)) {
			types.push_back(boost::numeric_cast<std::size_t>(sqlite::column<std::int64_t>(getTypes, 0)));
        }
    );
	std::cout << types.size() << " orders have to be updated.\n";
	return types;
}

std::vector<std::size_t> outdatedOrders2(const sqlite::dbPtr& db, std::size_t region) {
    auto getTypes = sqlite::prepare(db, "select typeid from types natural left join (select typeid, max(viewtime) as viewtime from orders join (select stationid from regions natural join constellations natural join systems natural join stations where regionid = " + std::to_string(region) + ") on orders.station = stationid join order_views using (orderid) group by typeid) where viewtime - strftime('%s', 'now', '-5 minutes') <= 0;");
	
	int rc;
	std::vector<std::size_t> types;
	types.reserve(10000);
	std::cout << "determining outdated orders\n";
    TIME(
        for (rc = sqlite::step(getTypes); rc == sqlite::ROW; rc = sqlite::step(getTypes)) {
			types.push_back(boost::numeric_cast<std::size_t>(sqlite::column<std::int64_t>(getTypes, 0)));
        }
    );
	std::cout << types.size() << " orders have to be updated.\n";
	return types;
}
