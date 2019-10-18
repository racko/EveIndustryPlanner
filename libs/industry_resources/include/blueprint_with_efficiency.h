#pragma once

#include "blueprint_efficiency.h"
#include "type_id.h"
#include <functional>

class BlueprintWithEfficiency {
  public:
    BlueprintWithEfficiency(TypeID bp, const BlueprintEfficiency& be) : blueprint(bp), blueprintEfficiency(be) {}
    BlueprintWithEfficiency(TypeID bp, double me, double te) : blueprint(bp), blueprintEfficiency(me, te) {}
    const BlueprintEfficiency& getEfficiency() const { return blueprintEfficiency; }
    double getMaterialEfficiency() const { return blueprintEfficiency.getMaterialEfficiency(); }
    double getTimeEfficiency() const { return blueprintEfficiency.getTimeEfficiency(); }
    TypeID getTypeID() const { return blueprint; }

  private:
    TypeID blueprint;
    BlueprintEfficiency blueprintEfficiency;
};

inline bool operator==(const BlueprintWithEfficiency& a, const BlueprintWithEfficiency& b) {
    return a.getTypeID() == b.getTypeID() && a.getMaterialEfficiency() == b.getMaterialEfficiency() &&
           a.getTimeEfficiency() == b.getTimeEfficiency();
}

namespace std {
template <>
struct hash<BlueprintWithEfficiency> {
    typedef BlueprintWithEfficiency argument_type;
    typedef std::size_t result_type;

    result_type operator()(argument_type const& bp) const;
};
} // namespace std
