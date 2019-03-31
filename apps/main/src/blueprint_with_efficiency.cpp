#include "blueprint_with_efficiency.h"
#include <boost/functional/hash.hpp>
#include <tuple>

namespace {
using tuple_t = std::tuple<TypeID,double,double>;
static boost::hash<tuple_t> h;
}

namespace std {
auto hash<BlueprintWithEfficiency>::operator()(argument_type const& bp) const -> result_type {
    return h(tuple_t{bp.getTypeID(), bp.getMaterialEfficiency(), bp.getTimeEfficiency()});
}
}
