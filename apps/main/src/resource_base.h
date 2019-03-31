#pragma once

#include <infinity.h>
#include <memory>
#include <string>

class Resource_Base {
  public:
    using Ptr = std::shared_ptr<Resource_Base>;
    using ConstPtr = std::shared_ptr<const Resource_Base>;

    //Resource_Base(std::string n, std::string fn, double l = 0, double u = infinity)
    //    : lower_limit(l), upper_limit(u), name(std::move(n)), fullName(std::move(fn)) {}
    Resource_Base(std::string n, std::string fn, double l = 0)
        : lower_limit(l), upper_limit(infinity), name(std::move(n)), fullName(std::move(fn)) {}
    virtual ~Resource_Base() = default;
    const std::string& getName() const { return name; }
    const std::string& getFullName() const { return fullName; }
    // void setRowID(std::size_t r) {
    //    rowID = r;
    //}
    // std::size_t getRowID() const {
    //    return rowID;
    //}
    double getLowerLimit() const { return lower_limit; }
    double getUpperLimit() const { return upper_limit; }
    std::size_t rowID;

  private:
    double lower_limit;
    double upper_limit;
    std::string name;
    std::string fullName;
};

// class DefaultItem : public Resource_Base {
// public:
//    DefaultItem(TypeID t, std::string n) : Resource_Base(std::move(n)), typeID(t) {}
// private:
//    TypeID typeID;
//};
//
// class T1BPO : public Resource_Base {
// public:
//    T1BPO(TypeID t, std::string n) : Resource_Base(std::move(n)), typeID(t) {}
// private:
//    TypeID typeID;
//};
//
// class T1BPC : public Resource_Base {
// public:
//    T1BPC(TypeID t, std::string n) : Resource_Base(std::move(n)), typeID(t) {}
// private:
//    TypeID typeID;
//};
//
// class T2BPO : public Resource_Base {
// public:
//    T2BPO(TypeID t, std::string n) : Resource_Base(std::move(n)), typeID(t) {}
// private:
//    TypeID typeID;
//};
//
// class T2BPC : public Resource_Base {
// public:
//    T2BPC(TypeID t, std::string n) : Resource_Base(std::move(n)), typeID(t) {}
// private:
//    TypeID typeID;
//};
