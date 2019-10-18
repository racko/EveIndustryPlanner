#include "libs/industry_resources/resource_manager/stuff.h"

#include "industry_limits.h"
#include "libs/assets/assets.h"
#include "libs/industry_resources/types/types.h"
#include "names.h"

Stuff::Stuff(const Names& names, const Assets& assets, const Types& types, const IndustryLimits& industry)
    : NormalResources{names, assets},
      BlueprintOriginals{names, industry.timeFrame},
      BlueprintCopies{names, assets, types},
      labTime(
          std::make_shared<Resource_Base>("LabTime", "LabTime", -industry.timeFrame * double(industry.scienceLabs))),
      productionLineTime(std::make_shared<Resource_Base>(
          "ProductionLineTime", "ProductionLineTime", -industry.timeFrame * double(industry.productionLines))) {}

NormalResources::NormalResources(const Names& names, const Assets& assets) : names_{names}, assets_{assets} {}
BlueprintOriginals::BlueprintOriginals(const Names& names, double timeFrame) : names_{names}, timeFrame_{timeFrame} {}
BlueprintCopies::BlueprintCopies(const Names& names, const Assets& assets, const Types& types)
    : names_{names}, assets_{assets}, types_{types} {}

const Resource_Base::Ptr& NormalResources::getResource(const std::size_t id) {
    auto& resource = resources_[id];
    if (resource == nullptr)
        resource =
            std::make_shared<Resource_Base>(std::to_string(id), names_.getName(id), -double(assets_.countAsset(id)));
    return resource;
}

const Resource_Base::Ptr& BlueprintOriginals::getBPO(const std::size_t id) {
    auto& resource = bpoTimes_[id];
    if (resource == nullptr)
        resource = std::make_shared<Resource_Base>(
            std::to_string(id) + "_BPO_Time", names_.getName(id) + " BPO Time", -timeFrame_);
    return resource;
}

namespace {
std::string makeBpeString(const BlueprintEfficiency& bpe) {
    const auto me = bpe.getMaterialEfficiency();
    const auto te = bpe.getTimeEfficiency();
    return std::to_string(me) + "_" + std::to_string(te);
}

std::string makeBpeString(const BlueprintWithEfficiency& bpwe) { return makeBpeString(bpwe.getEfficiency()); }
} // namespace

const Resource_Base::Ptr& BlueprintCopies::getBPC(const BlueprintWithEfficiency& bpwe) {
    auto& resource = bpcs_[bpwe];
    if (resource == nullptr) {
        const auto bp = bpwe.getTypeID();
        const auto runs = types_.isT1BP(bp) ? assets_.runs(bp) : assets_.runs(bpwe);
        const auto bpeString = makeBpeString(bpwe);
        resource = std::make_shared<Resource_Base>(std::to_string(bp) + "_" + bpeString + "_BPC",
                                                   names_.getName(bp) + " " + bpeString + "_BPC",
                                                   -double(runs));
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

const std::vector<DecryptorProperty> Stuff::t1decryptors{DecryptorProperty({}, 1, 1, 0, 1)};
const std::vector<DecryptorProperty> Stuff::decryptors{
    DecryptorProperty({}, 0.98, 0.96, 0, 1),
    DecryptorProperty(34203, 1, 0.94, 9, 0.6),    // Augmentation
    DecryptorProperty(34208, 0.96, 0.96, 7, 0.9), // Optimized Augmentation
    DecryptorProperty(34206, 0.97, 0.88, 2, 1),   // Symmetry
    DecryptorProperty(34205, 0.95, 0.90, 0, 1.1), // Process
    DecryptorProperty(34201, 0.96, 0.86, 1, 1.2), // Accelerant
    DecryptorProperty(34204, 0.97, 0.98, 3, 1.5), // Parity
    DecryptorProperty(34202, 0.99, 0.92, 4, 1.8), // Attainment
    DecryptorProperty(34207, 0.97, 0.98, 2, 1.9), // Optimized Attainment
};
