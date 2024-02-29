#pragma once

#include "json_document.h"
#include "json_lexer.h"

namespace Json
{

bool parse_tokens(const DynamicArray<Token>& tokens, const String content, Document& out);
bool parse_string(const String content, Document& out);

} // namespace Json
