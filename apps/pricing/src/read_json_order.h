#pragma once

#include "order.h"
#include <rapidjson/document.h>

Order convert(const rapidjson::GenericDocument<rapidjson::ASCII<>>::ConstObject& v);
