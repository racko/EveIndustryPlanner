#pragma once

#include "apps/pricing/order.h"
#include <rapidjson/document.h>

Order convert(const rapidjson::GenericDocument<rapidjson::ASCII<>>::ConstObject& v);
