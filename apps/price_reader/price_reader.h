#pragma once
//#include <vector>
#include <string>
//#include <memory>

struct sqlite3;

struct Price {
  public:
    Price(double adj, double avg, size_t t, std::string n)
        : adjustedPrice(adj), averagePrice(avg), typeID(t), name(std::move(n)) {}
    double getAdjustedPrice() const { return adjustedPrice; }
    double getAveragePrice() const { return averagePrice; }
    size_t getTypeID() const { return typeID; }
    const std::string& getName() const { return name; }

  private:
    double adjustedPrice;
    double averagePrice;
    size_t typeID;
    std::string name;
};

// std::vector<Price> loadPrices();
// void assertTypes(const std::shared_ptr<sqlite3>& db, const std::vector<Price>& prices);
