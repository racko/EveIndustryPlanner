#include "libs/industry_resources/blueprint_efficiency.h"
#include <ostream>

std::ostream& operator<<(std::ostream& s, const BlueprintEfficiency& bpe) {
    return s << "ME: " << bpe.getMaterialEfficiency() << ", TE: " << bpe.getTimeEfficiency();
}
