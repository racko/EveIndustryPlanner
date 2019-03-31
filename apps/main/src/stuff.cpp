#include "stuff.h"

#include "assets.h"
#include "industry_limits.h"
#include "names.h"
#include "types.h"

Stuff::Stuff(const Names& names, const Assets& assets, const Types& types, const IndustryLimits& industry) : labTime(std::make_shared<Resource_Base>("LabTime", "LabTime", -industry.timeFrame * double(industry.scienceLabs))), productionLineTime(std::make_shared<Resource_Base>("ProductionLineTime", "ProductionLineTime", -industry.timeFrame * double(industry.productionLines))), names_(names), assets_(assets), types_(types), timeFrame(industry.timeFrame) {}

const Resource_Base::Ptr& Stuff::getResource(std::size_t id) {
    auto& resource = resources[id];
    if (resource == nullptr)
        resource = std::make_shared<Resource_Base>(std::to_string(id), names_.getName(id), -double(assets_.countAsset(id)));
    return resource;
}

const Resource_Base::Ptr& Stuff::getBPO(std::size_t id) {
    auto& resource = bpoTimes[id];
    if (resource == nullptr)
        resource = std::make_shared<Resource_Base>(std::to_string(id) + "_BPO_Time", names_.getName(id) + " BPO Time", -timeFrame);
    return resource;
}

namespace {
std::string makeBpeString(const BlueprintEfficiency& bpe) {
    auto me = bpe.getMaterialEfficiency();
    auto te = bpe.getTimeEfficiency();
    return std::to_string(me) + "_" + std::to_string(te);
}

std::string makeBpeString(const BlueprintWithEfficiency& bpwe) {
    return makeBpeString(bpwe.getEfficiency());
}
}

const Resource_Base::Ptr& Stuff::getBPC(const BlueprintWithEfficiency& bpwe) {
    auto& resource = bpcs[bpwe];
    if (resource == nullptr) {
        auto bp = bpwe.getTypeID();
        auto runs = types_.isT1BP(bp) ? assets_.runs(bp): assets_.runs(bpwe);
        auto bpeString = makeBpeString(bpwe);
        resource = std::make_shared<Resource_Base>(std::to_string(bp) + "_" + bpeString + "_BPC", names_.getName(bp) + " " + bpeString + "_BPC", -double(runs));
    }
    return resource;
}

const std::vector<BlueprintEfficiency> Stuff::bpes{
    BlueprintEfficiency(0.98, 0.96),
    BlueprintEfficiency(1, 0.94),
    BlueprintEfficiency(0.96, 0.96),
    BlueprintEfficiency(0.97, 0.88),
    BlueprintEfficiency(0.95, 0.90),
    BlueprintEfficiency(0.96, 0.86),
    BlueprintEfficiency(0.97, 0.98),
    BlueprintEfficiency(0.99, 0.92),
};

const std::vector<DecryptorProperty> Stuff::t1decryptors{ DecryptorProperty({}, 1, 1, 0, 1) };
const std::vector<DecryptorProperty> Stuff::decryptors{
    DecryptorProperty({}, 0.98, 0.96, 0, 1),
    DecryptorProperty(34203, 1, 0.94, 9, 0.6), // Augmentation
    DecryptorProperty(34208, 0.96, 0.96, 7, 0.9), // Optimized Augmentation
    DecryptorProperty(34206, 0.97, 0.88, 2, 1), // Symmetry
    DecryptorProperty(34205, 0.95, 0.90, 0, 1.1), // Process
    DecryptorProperty(34201, 0.96, 0.86, 1, 1.2), // Accelerant
    DecryptorProperty(34204, 0.97, 0.98, 3, 1.5), // Parity
    DecryptorProperty(34202, 0.99, 0.92, 4, 1.8), // Attainment
    DecryptorProperty(34207, 0.97, 0.98, 2, 1.9), // Optimized Attainment
};
