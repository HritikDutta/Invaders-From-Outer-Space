#pragma once

#include "containers/bytes.h"
#include "serialization/json.h"

namespace Binary
{

Bytes json_document_to_binary(const Json::Document& document);

} // namespace Binary