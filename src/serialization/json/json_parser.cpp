#include "json_parser.h"

#include "core/types.h"
#include "core/logger.h"
#include "json_debug_output.h"
#include "json_document.h"
#include "json_lexer.h"
#include "json_result.h"

namespace Json
{

struct ParserContext
{
    String content;
    u64 current_index;
    bool encountered_error;
};

static inline bool is_token_a_value(Token::Type type)
{
    switch (type)
    {
        case Token::Type::IDENTIFIER:
        case Token::Type::INTEGER:
        case Token::Type::FLOAT:
        case Token::Type::STRING:
        case Token::Type::BRACKET_OPEN:
        case Token::Type::BRACE_OPEN:
            return true;
        
        case Token::Type::COLON:
        case Token::Type::COMMA:
        case Token::Type::BRACE_CLOSE:
        case Token::Type::BRACKET_CLOSE:
            return false;

        default:
            log_error("Incorrect token type! (found: '%')", (s32) type);
    }

    return false;
}

static String copy_and_escape(const String source, ParserContext& context)
{
    DynamicArray<char> result = make<DynamicArray<char>>(source.size);

    for (u64 i = 0; i < source.size; i++)
    {
        char ch = source[i];

        if (ch == '\\')
        {
            i++;
            switch (source[i])
            {
                case 'b': ch = '\b'; break;
                case 'f': ch = '\f'; break;
                case 'n': ch = '\n'; break;
                case 'r': ch = '\r'; break;
                case 't': ch = '\t'; break;

                // No need to change character in these cases
                case '\"':
                case '\\':
                break;

                default:
                {
                    log_error("Unexpected escape character! (character: '\\%', line: %)", ch, line_number(context.content, context.current_index));
                    context.encountered_error = true;
                } break;
            }
        }

        append(result, ch);
    }

    return String { result.data, result.size };
}

static void parse_next(const DynamicArray<Token>& tokens, ParserContext& context, Document& out)
{
    if (context.current_index > tokens.size)
    {
        log_error("Json data is incomplete! (Parser ran out of tokens)");
        context.encountered_error = true;
        return;
    }

    const Token& token = tokens[context.current_index];
    switch (token.type)
    {
        case Token::Type::STRING:
        {
            ResourceIndex index = out.resources.size;

            Resource res = {};
            res.string = copy_and_escape(token.value, context);
            append(out.resources, res);

            DependencyNode node = {};
            node.index = index;
            node.type  = Type::STRING;
            append(out.dependency_tree, node);

        } break;

        case Token::Type::INTEGER:
        {
            ResourceIndex index = out.resources.size;

            // TODO: convert string to integer on your own with error checking
            Resource res = {};
            res.integer64 = _atoi64(token.value.data);
            append(out.resources, res);

            DependencyNode node = {};
            node.index = index;
            node.type  = Type::INTEGER;
            append(out.dependency_tree, node);
        } break;

        case Token::Type::FLOAT:
        {
            ResourceIndex index = out.resources.size;

            // TODO: convert string to float on your own with error checking
            Resource res = {};
            res.float64 = atof(token.value.data);
            append(out.resources, res);

            DependencyNode node = {};
            node.index = index;
            node.type  = Type::FLOAT;
            append(out.dependency_tree, node);
        } break;

        case Token::Type::IDENTIFIER:
        {
            ResourceIndex index = out.resources.size;
            Type node_type = Type::BOOLEAN;

            {   // Determine type of resource
                if (token.value == ref("null", 4))
                {
                    // Point to null value in dependency tree
                    node_type = Type::NONE;
                    index = 0;
                }
                else if (token.value == ref("true", 4))
                {
                    // Point to true value in dependency tree
                    index = 2;
                }
                else if (token.value == ref("false", 5))
                {
                    // Point to true value in dependency tree
                    index = 1;
                }
                else
                {
                    // Can't identify the identifier, lol
                    log_error("Identifiers can only be true, false, or null! (found: '%', line: %)", token.value, line_number(context.content, token.index));
                    context.encountered_error = true;
                }
            }

            DependencyNode node = {};
            node.index = index;
            node.type  = node_type;
            append(out.dependency_tree, node);
        } break;

        case Token::Type::BRACKET_OPEN:
        {
            u64 array_tree_index = out.dependency_tree.size;

            DependencyNode node = {};
            node.array = make<ArrayNode>();
            node.type  = Type::ARRAY;
            append(out.dependency_tree, node);

            // Skip the first [
            context.current_index++;

            while (true)
            {
                if (context.current_index >= tokens.size)
                {
                    log_error("Array was never closed with a ]! (line: %)", line_number(context.content, token.index));
                    context.encountered_error = true;
                    break;
                }

                // Closed array
                if (tokens[context.current_index].type == Token::Type::BRACKET_CLOSE)
                    break;

                append(out.dependency_tree[array_tree_index].array, out.dependency_tree.size);
                parse_next(tokens, context, out);

                if (context.current_index >= tokens.size)
                {
                    log_error("Array was never closed with a ]! (line: %)", line_number(context.content, token.index));
                    context.encountered_error = true;
                    break;
                }

                const Token& current_token = tokens[context.current_index];

                // Closed array
                if (current_token.type == Token::Type::BRACKET_CLOSE)
                    break;

                if (current_token.type != Token::Type::COMMA)
                {
                    log_error("Array items must be separated by commas! (found: '%', line: %)", current_token.value, line_number(context.content, current_token.index));
                    context.encountered_error = true;

                    // Don't skip over values. Helps with error checking.
                    if (is_token_a_value(current_token.type))
                        context.current_index--;
                }

                context.current_index++;
            }

            // ] gets skipped at the end anyways
        } break;

        case Token::Type::BRACE_OPEN:
        {
            u64 object_tree_index = out.dependency_tree.size;

            DependencyNode node = {};
            node.object = make<ObjectNode>();
            node.type  = Type::OBJECT;
            append(out.dependency_tree, node);

            // Skip the first {
            context.current_index++;

            while (true)
            {
                if (context.current_index >= tokens.size)
                {
                    log_error("Object was never closed with a }! (line: %)", line_number(context.content, token.index));
                    context.encountered_error = true;
                    break;
                }

                const Token& key_token = tokens[context.current_index];

                // Closed object
                if (key_token.type == Token::Type::BRACE_CLOSE)
                    break;

                if (key_token.type != Token::Type::STRING)
                {
                    log_error("Expected a key for object! (found: '%', line: %)", key_token.value, line_number(context.content, key_token.index));
                    context.encountered_error = true;
                }

                context.current_index++;
                const Token& colon_token = tokens[context.current_index];

                // Key should be followed by a :
                if (colon_token.type != Token::Type::COLON)
                {
                    log_error("Expected : after key in object! (found: '%', line: %)", colon_token.value, line_number(context.content, colon_token.index));
                    context.encountered_error = true;

                    // Don't skip over values. Helps with error checking.
                    if (is_token_a_value(colon_token.type))
                        context.current_index--;
                }

                String key_string = copy_and_escape(key_token.value, context);
                put(out.dependency_tree[object_tree_index].object, key_string, out.dependency_tree.size);

                context.current_index++;
                parse_next(tokens, context, out);

                if (context.current_index >= tokens.size)
                {
                    log_error("Object was never closed with a }! (line: %)", line_number(context.content, token.index));
                    context.encountered_error = true;
                    break;
                }
                
                // Closed object
                const Token& next_token = tokens[context.current_index];
                if (next_token.type == Token::Type::BRACE_CLOSE)
                    break;
                
                if (next_token.type != Token::Type::COMMA)
                {
                    log_error("Object properties must be separated by commas! (found: '%', line: %)", next_token.value, line_number(context.content, next_token.index));
                    context.encountered_error = true;

                    // Don't skip over values. Helps with error checking.
                    if (is_token_a_value(next_token.type))
                        context.current_index--;
                }

                context.current_index++;
            }

            // } gets skipped at the end anyways
        } break;

        default:
        {
            // The only remaining tokens are single character punctuations
            log_error("Expected a value (identifier, number, string, array, or object), got %", (char) token.type);
            context.encountered_error = true;
        } break;
    }

    context.current_index++;
}

bool parse_tokens(const DynamicArray<Token>& tokens, const String content, Document& out)
{
    gn_assert_with_message(tokens.data, "Tokens array points to null!");
    gn_assert_with_message(out.dependency_tree.size == 0, "Output json document struct is not empty! (number of elements: %)", out.dependency_tree.size);

    {   // Add the null element
        // If user tries to access an object property that wasn't in the file,
        // then the value will point to this element
        append(out.dependency_tree, DependencyNode {});
        append(out.resources, Resource {});
    }

    {   // Add constants for true and false
        DependencyNode node;
        node.type  = Type::BOOLEAN;

        Resource res;

        {
            node.index = out.resources.size;
            res.boolean = false;

            append(out.dependency_tree, node);
            append(out.resources, res);
        }

        {
            node.index = out.resources.size;
            res.boolean = true;

            append(out.dependency_tree, node);
            append(out.resources, res);
        }
    }

    if (tokens.size == 0)
    {
        log_error("Tokens array is empty!");
        return false;
    }

    ParserContext context = {};
    context.content = content;
    
    parse_next(tokens, context, out);

    if (!context.encountered_error && context.current_index < tokens.size)
    {
        log_error("End of file expected! (found: '%', line: %)", tokens[context.current_index].value, line_number(context.content, tokens[context.current_index].index));
        context.encountered_error = true;
    }

    #ifdef GN_LOG_SERIALIZATION
        if (!context.encountered_error)
            document_debug_output(out);
    #endif // GN_DEBUG    

    return !context.encountered_error;
}

bool parse_string(const String content, Document& out)
{
    DynamicArray<Json::Token> tokens = {};
    bool success = lex(content, tokens);

    if (!success)
    {
        print_error("Lexing failed!");
        goto err_lexing;
    }

    success = parse_tokens(tokens, content, out);

    if (!success)
        print_error("Parsing failed!");

err_lexing:
    free(tokens);

    return success;
}

} // namespace Json
