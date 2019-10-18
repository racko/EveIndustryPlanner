#pragma once

#include <ostream>
#include <vector>

struct Diff;
struct Order;
struct Id;

std::vector<Order>::reverse_iterator
findOrderWithSmallerId(std::vector<Order>::reverse_iterator begin, std::vector<Order>::reverse_iterator end, Id o);

std::vector<Order>::const_iterator findOrderWithEqualOrGreaterId(std::vector<Order>::const_iterator begin,
                                                                 std::vector<Order>::const_iterator end,
                                                                 Id existing_entry);

void write_order(char*& s, const Order& o);
void write_diff(char*& s, const Diff& d);

void logExtracting(const char* file_name);
void logSkipping(const char* file_name);
void logParsing(const char* file_name);
void logInvalidPage(const char* file_name);

std::vector<std::vector<Order>> readOrders(const char* file_name);
