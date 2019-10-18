#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// class ResourceManager;
//
// class Resource {
//  public:
//    using Ptr = std::shared_ptr<Resource>;
//    using ConstPtr = std::shared_ptr<const Resource>;
//
//    Resource(std::size_t typeId);
//    ~Resource();
//    const std::string& getName() const { return name; }
//    const std::string& getFullName() const { return fullName; }
//    double getLowerLimit() const { return lower_limit; }
//    double getUpperLimit() const { return upper_limit; }
//    int rowID;
//
//  private:
//    double lower_limit;
//    double upper_limit;
//    std::string name;
//    std::string fullName;
//};
//
// class ResourceManager {
//    struct ResourceManagerGuard {
//        ResourceManager* last_;
//
//        ResourceManagerGuard(ResourceManagerGuard&&) = delete;
//        ResourceManagerGuard(const ResourceManagerGuard&) = delete;
//        ResourceManagerGuard& operator=(ResourceManagerGuard&&) = delete;
//        ResourceManagerGuard& operator=(const ResourceManagerGuard&) = delete;
//        ~ResourceManagerGuard() { current_ = last_; }
//    };
//
//    using TypeId = std::size_t;
//    using Index = std::size_t;
//    using Name = std::size_t;
//
//  public:
//    Resource& operator[](const TypeId t) {
//        const auto& [it, inserted] = byTypeId_.try_emplace(t, byTypeId_.size());
//        if (inserted) {
//            resources_.emplace_back(t);
//        }
//        return resources_[it->second];
//    }
//
//    static ResourceManager* get() { return current_; }
//
//    static ResourceManagerGuard set(ResourceManager* rm) {
//        auto last = current_;
//        current_ = rm;
//        return ResourceManagerGuard{last};
//    }
//
//  private:
//    std::vector<Resource> resources_;
//    std::unordered_map<TypeId, Index> byTypeId_;
//    static ResourceManager* current_;
//};

class Resource_Base {
  public:
    using Ptr = std::shared_ptr<Resource_Base>;
    using ConstPtr = std::shared_ptr<const Resource_Base>;

    Resource_Base(std::string n, std::string fn, double l = 0);
    ~Resource_Base();
    const std::string& getName() const { return name; }
    const std::string& getFullName() const { return fullName; }
    double getLowerLimit() const { return lower_limit; }
    double getUpperLimit() const { return upper_limit; }
    int rowID;

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
