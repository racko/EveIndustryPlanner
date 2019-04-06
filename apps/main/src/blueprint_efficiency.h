#pragma once
#include <iosfwd>

class BlueprintEfficiency {
  public:
    BlueprintEfficiency(double me, double te) : materialEfficiency(me), timeEfficiency(te) {}
    double getMaterialEfficiency() const { return materialEfficiency; }
    double getTimeEfficiency() const { return timeEfficiency; }

  private:
    double materialEfficiency;
    double timeEfficiency;
};

std::ostream& operator<<(std::ostream& s, const BlueprintEfficiency& bpe);
