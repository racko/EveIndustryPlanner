#pragma once

#include "blueprint_efficiency.h"
#include "type_id.h"

#include <boost/optional.hpp>

class DecryptorProperty {
    public:
        DecryptorProperty(boost::optional<TypeID> d, BlueprintEfficiency be, unsigned r, double p) : decryptor(std::move(d)), blueprintEfficiency(std::move(be)), runModifier(r), probabilityModifier(p) {}
        DecryptorProperty(boost::optional<TypeID> d, double me, double te, unsigned r, double p) : decryptor(std::move(d)), blueprintEfficiency(me, te), runModifier(r), probabilityModifier(p) {}
        boost::optional<TypeID> getDecryptor() const { return decryptor; }
        double getMaterialEfficiency() const { return blueprintEfficiency.getMaterialEfficiency(); }
        double getTimeEfficiency() const { return blueprintEfficiency.getTimeEfficiency(); }
        unsigned getRunModifier() const { return runModifier; }
        double getProbabilityModifier() const { return probabilityModifier; }
    private:
        boost::optional<TypeID> decryptor;
        BlueprintEfficiency blueprintEfficiency;
        unsigned runModifier;
        double probabilityModifier;
};
