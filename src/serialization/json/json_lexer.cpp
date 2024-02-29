#include "json_lexer.h"

#include "core/types.h"
#include "containers/darray.h"
#include "containers/string.h"
#include "math/common.h"
#include "json_result.h"
#include "json_debug_output.h"

namespace Json
{

inline bool is_alphabet(char ch)
{
    return ((ch >= 'a') && (ch <= 'z')) ||
           ((ch >= 'A') && (ch <= 'Z'));
}

inline static bool is_digit(char ch)
{
    return (ch >= '0') && (ch <= '9');
}

bool lex(const String content, DynamicArray<Token>& tokens)
{
    clear(tokens);
    resize(tokens, max(2ui64, content.size / 10)); // Just an estimate

    bool encountered_error = false;
    u64 current_index = 0;

    while (true)
    {
        // Won't check for null character to end lexing since strings have a specified length

        // Reached EOF
        if (current_index >= content.size)
            break;
        
        switch (content[current_index])
        {
            // Skip whitespace
            case ' ':
            case '\t':
            case '\r':
            case '\n':
            case '\0':
            {
                current_index++;
            } break;
            
            // Punctuations
            case (char) Token::Type::BRACKET_OPEN:
            case (char) Token::Type::BRACKET_CLOSE:
            case (char) Token::Type::BRACE_OPEN:
            case (char) Token::Type::BRACE_CLOSE:
            case (char) Token::Type::COLON:
            case (char) Token::Type::COMMA:
            {
                Token token;
                token.index = current_index;
                token.type  = (Token::Type) content[current_index];
                token.value = ref(content.data + current_index, 1);

                append(tokens, token);

                current_index++;
            } break;

            // String
            case '\"':
            {
                // Skip the first "
                current_index++;

                // Eat till the next "
                u64 str_size = 0;
                while (true)
                {
                    const u64 index = current_index + str_size;

                    // Reached EOF before closing string
                    if (index >= content.size)
                    {
                        log_error("String was not closed! (line: %)", line_number(content, current_index));
                        encountered_error = true;
                        break;
                    }

                    if (content[index] == '\"')
                        break;

                    // Reached new line before closing string
                    if (content[index] == '\n')
                    {
                        log_error("Reached new line before closing string! (line: %)", line_number(content, index));
                        encountered_error = true;
                        break;
                    }

                    // Skip the next character if current character is a '\'
                    if (content[index] == '\\')
                        str_size++;

                    str_size++;
                }

                Token token;
                token.index = current_index;
                token.type  = Token::Type::STRING;
                token.value = ref(content.data + current_index, str_size);

                append(tokens, token);

                // Skip the ending "
                current_index += str_size + 1;
            } break;

            // Number (integer or float)
            case '-':
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
            {
                bool encountered_dot = false;   // Floats can't be written like .123, they have to be written as 0.123
                u64 number_size = 1;

                while (true)
                {
                    const u64 index = current_index + number_size;

                    if (index >= content.size)
                        break;

                    // - is not allowed between numbers (no math allowed!)
                    if (content[index] == '-')
                    {
                        log_error("'-' sign can only be used at the start of a number! (line: %)", line_number(content, index));
                        encountered_error = true;
                    }
                    
                    // there should be only 1 dot in a number
                    if (content[index] == '.')
                    {
                        if (encountered_dot)
                        {
                            log_error("'.' can only be used once in a number! (line: %)", line_number(content, index));
                            encountered_error = true;
                        }

                        encountered_dot = true;
                    }

                    if (content[index] != '.' && !is_digit(content[index]))
                        break;

                    number_size++;
                }
                
                Token token;
                token.index = current_index;
                token.type  = encountered_dot ? Token::Type::FLOAT : Token::Type::INTEGER;
                token.value = ref(content.data + current_index, number_size);

                append(tokens, token);

                current_index += number_size;

            } break;

            // Probably an identifier
            default:
            {
                // Identifiers can only have alphabets
                if (!is_alphabet(content[current_index]))
                {
                    log_error("Encountered invalid token! (found token: '%', line: %)", content[current_index], line_number(content, current_index));
                    encountered_error = true;
                    current_index++;
                    break;
                }

                u64 identifier_size = 1;
                
                while (true)
                {
                    const u64 index = current_index + identifier_size;

                    if (index >= content.size || !is_alphabet(content[index]))
                        break;

                    identifier_size++;
                }
                
                Token token;
                token.index = current_index;
                token.type  = Token::Type::IDENTIFIER;
                token.value = ref(content.data + current_index, identifier_size);

                append(tokens, token);

                current_index += identifier_size;
            } break;
        }
    }

#ifdef GN_LOG_SERIALIZATION
    if (!encountered_error)
        lexer_debug_output(tokens);
#endif // GN_DEBUG    

    return !encountered_error;
}

} // namespace Json