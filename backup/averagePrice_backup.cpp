void saveHistories(const std::unordered_map<std::size_t, double>& avgPrices, const sqlite::dbPtr& db, const std::string& access_token) {
    using namespace soplex;
    std::cout << "Volumes:\n";
    auto insertHistory = sqlite::prepare(db, "insert or ignore into history values(?, ?, ?, ?, ?, ?, ?, ?);");
    auto insertRobustPrice = sqlite::prepare(db, "insert or replace into robust_price values(?, ?, ?);");
    auto i = 0u;
    auto count = avgPrices.size();
    std::stringstream datestr;
    auto now = std::chrono::system_clock::now();
    auto baseTime = now - std::chrono::seconds(86400 * 31);
    static const DSVector dummycol(0);
    //Eigen::MatrixXd A(avgPrices.size(), 30);
	Context::Ptr context = new Context(Context::CLIENT_USE, "", Context::VERIFY_NONE);
	HTTPSClientSession session("crest-tq.eveonline.com", HTTPSClientSession::HTTPS_PORT, context.get());
	session.setKeepAlive(true);

	NameValueCollection cookies;
    for (auto& p : avgPrices) {
		HTTPRequest request(HTTPRequest::HTTP_GET, "/market/10000002/types/" + std::to_string(p.first) + "/history/", HTTPMessage::HTTP_1_1);
		request.add("Authorization", "Bearer " + access_token);

        Json::Value root;
		handleRequest(session, request, "", cookies, [&] (const auto& response, auto& s) { s >> root; });
        //auto aRow = A.row(i);

        //std::ifstream pricesFile(std::string("/tmp/histories/") + std::to_string(p.first) + ".json", std::ifstream::binary);
        //auto pTime = time<float,std::milli>([&] { pricesFile >> root; });
        const auto& items = root["items"];
        if (items.isNull()) {
            continue;
        }
        SoPlex lp; // min |y - ax+b|
        lp.setIntParam(SoPlex::OBJSENSE, SoPlex::OBJSENSE_MINIMIZE);
        lp.setIntParam(SoPlex::VERBOSITY, SoPlex::VERBOSITY_WARNING);
        lp.addColReal(LPCol(0, dummycol, infinity, -infinity)); // a
        lp.addColReal(LPCol(0, dummycol, infinity, -infinity)); // b
        std::vector<double> averages;
        //std::vector<double> volumes;
        averages.reserve(500);
        //volumes.reserve(500);
        for (const auto& item : items) {
            datestr << item["date"].asString();
            std::tm time = {};
            datestr >> std::get_time(&time, "%Y-%m-%d");
            if (!datestr)
                throw std::runtime_error("parse failed");
            std::uint64_t volume = item["volume"].asUInt64();
            datestr.seekg(0);
            datestr.seekp(0);
            auto then_time_t = std::mktime(&time);
            auto then = std::chrono::system_clock::from_time_t(then_time_t);
            std::uint64_t orderCount = item["orderCount"].asUInt64();
            auto avg = item["avgPrice"].asDouble();
            auto low = item["lowPrice"].asDouble();
            auto high = item["highPrice"].asDouble();
            if (then >= baseTime) {
                auto x = std::chrono::duration_cast<std::chrono::seconds>(then - baseTime).count() / 86400.0;
                //volumes.push_back(volume);
                averages.push_back(avg);
                lp.addColReal(LPCol(1.0, dummycol, infinity, 0));
                DSVector row(3);
                row.add(0, -x);
                row.add(1, -1);
                row.add(lp.numColsReal()-1, 1);
                lp.addRowReal(LPRow(row, LPRow::GREATER_EQUAL, -avg));
                row.clear();
                row.add(0, x);
                row.add(1, 1);
                row.add(lp.numColsReal()-1, 1);
                lp.addRowReal(LPRow(row, LPRow::GREATER_EQUAL, avg));
            }
            sqlite::bind(insertHistory, 1, p.first);
            sqlite::bind(insertHistory, 2, then_time_t);
            sqlite::bind(insertHistory, 3, 10000002);
            sqlite::bind(insertHistory, 4, orderCount);
            sqlite::bind(insertHistory, 5, low);
            sqlite::bind(insertHistory, 6, high);
            sqlite::bind(insertHistory, 7, avg);
            sqlite::bind(insertHistory, 8, volume);
            sqlite::step(insertHistory);
            sqlite::reset(insertHistory);
        }
        //if (!averages.empty()) {
        //    lp.writeFileReal(("/tmp/lps/" + std::to_string(p.first) + ".lp").c_str());
        //    auto status = lp.solve();
        //    auto lastPrice = averages.back();
        //    DVector prim(lp.numColsReal());
        //    lp.getPrimalReal(prim);
        //    auto a = prim[0];
        //    auto b = prim[1];
        //    auto x = std::chrono::duration_cast<std::chrono::seconds>(now + std::chrono::seconds(86400) - baseTime).count() / 86400.0;
        //    auto y = a * x + b;
        //    //std::cout << '\n' << p.first << ": a = " << a << ", b = " << b << ", x = " << x << ", y = " << y << "\n";
		//
        //    auto n = averages.size();
        //    std::sort(averages.begin(), averages.end());
        //    //std::sort(volumes.begin(), volumes.end());
        //    auto medianPrice = n % 2 == 0 ? 0.5 * (averages[n/2-1] + averages[n/2]) : averages[(n-1)/2];
        //    std::cout << p.first << ": " << y << ", " << lp.objValueReal() << " / " << double(averages.size()) << " = " << (lp.objValueReal() / double(averages.size())) << '\n';
        //    //auto medianVolume = n % 2 == 0 ? 0.5 * (volumes[n/2-1] + volumes[n/2]) : volumes[(n-1)/2];
        //    sqlite::bind(insertRobustPrice, 1, p.first);
        //    //sqlite::bind(insertRobustPrice, 2, medianPrice);
        //    //sqlite::bind(insertRobustPrice, 2, y);
        //    sqlite::bind(insertRobustPrice, 2, lastPrice + 2 *lp.objValueReal() / double(averages.size()));
        //    sqlite::bind(insertRobustPrice, 3, std::max(0.0, lastPrice - 4 * lp.objValueReal() / double(averages.size())));
        //    auto rc = sqlite::step(insertRobustPrice);
        //    if (rc != SQLITE_DONE) {
        //        std::cout << __PRETTY_FUNCTION__ << ":  " << sqlite3_errstr(rc) << " (" << rc << ")\n";
        //        throw std::runtime_error("...");
        //    }
        //    sqlite::reset(insertRobustPrice);
        //}
        static constexpr const char* ANSI_CLEAR_LINE = "\033[2K";
        ++i;
        std::cout << ANSI_CLEAR_LINE << " " << i << " / " << count << " | " << (double(i) / double(count) * 100.0) << "%\r" << std::flush;
    }
}

