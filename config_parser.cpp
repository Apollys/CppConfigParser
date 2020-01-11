#include "config_parser.h"

#include <algorithm>
#include <cctype>  // std::isspace
#include <fstream>
#include <iostream>
#include <sstream>

const char ConfigParser::kDeclarationTerminationChar = ';';
const std::string ConfigParser::kCommentPrefix = "#";

const std::string ConfigParser::kStringTypeString = "string";
const std::string ConfigParser::kIntTypeString = "int";
const std::string ConfigParser::kFloatTypeString = "float";
const std::string ConfigParser::kDoubleTypeString = "double";
const std::string ConfigParser::kBoolTypeString = "bool";

const std::vector<std::string> ConfigParser::kValidTypeStrings = std::vector<std::string>{
        kStringTypeString, kIntTypeString, kFloatTypeString, kDoubleTypeString, kBoolTypeString};

const std::unordered_map<std::string, ExpressionType> ConfigParser::kTypeStringMap{
        {ConfigParser::kStringTypeString, ExpressionType::kString},
        {ConfigParser::kIntTypeString, ExpressionType::kInt},
        {ConfigParser::kFloatTypeString, ExpressionType::kFloat},
        {ConfigParser::kDoubleTypeString, ExpressionType::kDouble},
        {ConfigParser::kBoolTypeString, ExpressionType::kBool}};

// TODO: line numbers in error messages are not original line numbers in config file,
// because whitespace/comment stripping removes lines and linebreaks

namespace {

/** String manipulation helper methods **/

// Trims from left, in-place
void Ltrim(std::string& s) {
    // Note: argument to std::isspace should be unsigned char to prevent undefined behavior
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
}

// Trims from right, in-place
void Rtrim(std::string& s) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    // Note: .base() gets forward iterator from reverse
    s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
}

// Trims from both sides, in-place
void Trim(std::string& s) {
    Ltrim(s);
    Rtrim(s);
}

// Splits string by delim, optionally retaining empty strings (caused by consecutive delim chars)
std::vector<std::string> SplitString(const std::string& input_string,
                                     const char delim,
                                     bool retain_empty = false) {
    std::vector<std::string> output_vector;
    size_t start_i = 0;
    size_t found_i = 0;  // dummy initialization to start while loop
    size_t len;
    while (found_i != std::string::npos) {
        found_i = input_string.find(delim, start_i);
        len = (found_i == std::string::npos) ? input_string.length() - start_i : found_i - start_i;
        if (len > 0) {  // non-consecutive delimiters
            output_vector.push_back(input_string.substr(start_i, found_i - start_i));
        } else if (retain_empty) {
            output_vector.push_back(std::string(""));
        }
        start_i = found_i + 1;
    }
    return output_vector;
}

/** Parsing helper methods **/

// Checks whether or not the given position in the string is the start of a comment
bool IsCommentStart(const std::string& input_string, size_t start_index) {
    size_t len = ConfigParser::kCommentPrefix.size();
    return input_string.substr(start_index, len) == ConfigParser::kCommentPrefix;
}

std::string RemoveComments(const std::string& input_string) {
    bool in_comment = false;
    bool in_quotes = false;
    std::string output_string;
    for (size_t i = 0; i < input_string.size(); ++i) {
        const char ch = input_string[i];
        if (in_comment) {  // skip commented out characters
            if (ch == '\n') in_comment = false;
            continue;
        } else if (in_quotes) {  // copy in-quotes characters directly
            if (ch == '"') in_quotes = false;
            output_string += ch;
        } else if (IsCommentStart(input_string, i)) {  // start comment
            in_comment = true;
        } else if (ch == '"') {  // start quotes
            in_quotes = true;
            output_string += ch;  // copy quote character as well (unlike comment)
        } else {                  // any non-special case, copy character to output
            output_string += ch;
        }
    }
    return output_string;
}

// Converts any sequence of consecutive whitespace into a single space (only if outside quotes)
// Should be called after comments are removed, since comment parsing is whitespace dependent
std::string CleanWhitespace(const std::string& input_string) {
    bool in_quotes = false;
    bool in_whitespace = false;
    std::string output_string;
    for (const char ch : input_string) {
        if (in_quotes) {  // copy in-quotes characters directly
            if (ch == '"') in_quotes = false;
            output_string += ch;
        } else if (in_whitespace) {  // when in whitespace, only check for end of whitespace
            if (!ConfigParser::is_space(ch)) {  // end whitespace
                in_whitespace = false;
                output_string += ch;
            }
        } else if (ConfigParser::is_space(ch)) {  // start whitespace
            in_whitespace = true;
            output_string += ' ';
        } else {  // any non-special case, copy character to output
            output_string += ch;
        }
    }
    return output_string;
}

/* Preprocessing to clean up and normalize the input:
 *  - Removes comments (starting from comment prefix, up to newline char)
 *  - Cleans up any whitespace (turning blocks of whitespace into a single space)
 *  - Splits into a list of declarations (by splitting at semicolons)
 *  - Trims whitespace (from left and right ends) of each declaration
 */
std::vector<std::string> PreprocessInput(const std::ifstream& input_filestream) {
    // Read file stream into input string stream and get string
    std::stringstream ss;
    ss << input_filestream.rdbuf();
    std::string input_string = ss.str();
    // Clean up formatting
    std::string cleaned_input = CleanWhitespace(RemoveComments(input_string));
    // Break into lines and trim excess whitespace
    std::vector<std::string> declarations =
            SplitString(cleaned_input, ConfigParser::kDeclarationTerminationChar);
    for (std::string& declaration : declarations) Trim(declaration);
    // Remove empty lines
    declarations.erase(std::remove(declarations.begin(), declarations.end(), ""),
                       declarations.end());
    return declarations;
}

// Advances the current_index until it points to a character for which delimeter_predicate
// returns true, then returns the string up to (but not including) that character.
std::string ReadNextToken(const std::string& input_string,
                          size_t* current_index,
                          bool delimeter_predicate(char c)) {
    std::string token;
    for (/* current_index */; *current_index < input_string.size(); ++(*current_index)) {
        if (delimeter_predicate(input_string[*current_index])) {
            break;
        }
        token += input_string[*current_index];
    }
    return token;
}

// Advances current_index to correspond to the next non-whitespace character in input_string
void SkipWhitespace(const std::string& input_string, size_t* current_index) {
    auto not_space = [](char ch) { return !ConfigParser::is_space(ch); };
    if (ConfigParser::is_space(input_string[*current_index])) {
        ReadNextToken(input_string, current_index, not_space);
    }
}

/** Single value parsing methods **/

std::string ParseString(const std::string& value_string, bool* error_flag) {
    *error_flag = false;
    if ((value_string.size() < 2) || (value_string[0] != '"') || (value_string.back() != '"')) {
        *error_flag = true;
        return "";
    }
    std::string string_contents(value_string.begin() + 1, value_string.end() - 1);
    if (string_contents.find('"') != std::string::npos) {  // string should not contain any quotes
        *error_flag = true;
    }
    return string_contents;
}

int ParseInt(const std::string& value_string, bool* error_flag) {
    *error_flag = false;
    try {
        return std::stoi(value_string);
    } catch (...) {
        *error_flag = true;
        return 0;
    }
}

float ParseFloat(const std::string& value_string, bool* error_flag) {
    *error_flag = false;
    try {
        return std::stof(value_string);
    } catch (...) {
        *error_flag = true;
        return 0;
    }
}

double ParseDouble(const std::string& value_string, bool* error_flag) {
    *error_flag = false;
    try {
        return std::stod(value_string);
    } catch (...) {
        *error_flag = true;
        return 0;
    }
}

bool ParseBool(const std::string& value_string, bool* error_flag) {
    *error_flag = false;
    if (value_string == "true") {
        return true;
    } else if (value_string == "false") {
        return false;
    }
    *error_flag = true;
    return false;
}

/** Vector parsing methods **/

// Forward declaration to use in ParseXxxVector methods
std::vector<std::string> SplitVectorExpression(const std::string& vector_string,
                                               const std::string& type_string,
                                               bool* error_flag);

std::vector<std::string> ParseStringVector(const std::string& expression_string, bool* error_flag) {
    static const std::string type_string = ConfigParser::kStringTypeString;
    // Split into individual values' expressions
    std::vector<std::string> value_strings =
            SplitVectorExpression(expression_string, type_string, error_flag);
    if (*error_flag) return std::vector<std::string>{};
    // Parse each value
    std::vector<std::string> parsed_values;
    for (const std::string& value_string : value_strings) {
        parsed_values.push_back(ParseString(value_string, error_flag));
        if (*error_flag) break;  // break whenever error occurs to preserve error_flag value
    }
    return parsed_values;
}

std::vector<int> ParseIntVector(const std::string& expression_string, bool* error_flag) {
    static const std::string type_string = ConfigParser::kIntTypeString;
    // Split into individual values' expressions
    std::vector<std::string> value_strings =
            SplitVectorExpression(expression_string, type_string, error_flag);
    if (*error_flag) return std::vector<int>{};
    // Parse each value
    std::vector<int> parsed_values;
    for (const std::string& value_string : value_strings) {
        parsed_values.push_back(ParseInt(value_string, error_flag));
        if (*error_flag) break;  // break whenever error occurs to preserve error_flag value
    }
    return parsed_values;
}

std::vector<float> ParseFloatVector(const std::string& expression_string, bool* error_flag) {
    static const std::string type_string = ConfigParser::kFloatTypeString;
    // Split into individual values' expressions
    std::vector<std::string> value_strings =
            SplitVectorExpression(expression_string, type_string, error_flag);
    if (*error_flag) return std::vector<float>{};
    // Parse each value
    std::vector<float> parsed_values;
    for (const std::string& value_string : value_strings) {
        parsed_values.push_back(ParseFloat(value_string, error_flag));
        if (*error_flag) break;  // break whenever error occurs to preserve error_flag value
    }
    return parsed_values;
}

std::vector<double> ParseDoubleVector(const std::string& expression_string, bool* error_flag) {
    static const std::string type_string = ConfigParser::kDoubleTypeString;
    // Split into individual values' expressions
    std::vector<std::string> value_strings =
            SplitVectorExpression(expression_string, type_string, error_flag);
    if (*error_flag) return std::vector<double>{};
    // Parse each value
    std::vector<double> parsed_values;
    for (const std::string& value_string : value_strings) {
        parsed_values.push_back(ParseDouble(value_string, error_flag));
        if (*error_flag) break;  // break whenever error occurs to preserve error_flag value
    }
    return parsed_values;
}

std::vector<bool> ParseBoolVector(const std::string& expression_string, bool* error_flag) {
    static const std::string type_string = ConfigParser::kBoolTypeString;
    // Split into individual values' expressions
    std::vector<std::string> value_strings =
            SplitVectorExpression(expression_string, type_string, error_flag);
    if (*error_flag) return std::vector<bool>{};
    // Parse each value
    std::vector<bool> parsed_values;
    for (const std::string& value_string : value_strings) {
        parsed_values.push_back(ParseBool(value_string, error_flag));
        if (*error_flag) break;  // break whenever error occurs to preserve error_flag value
    }
    return parsed_values;
}

// Breaks a single string containing a vector expression into its component pieces
// Example: "[1, 2, 3]" --> std::vector containing ["1", "2", "3"]
std::vector<std::string> SplitVectorExpression(const std::string& vector_string,
                                               const std::string& type_string,
                                               bool* error_flag) {
    *error_flag = false;
    std::vector<std::string> value_strings;
    // Handle string and non-string types separately
    if (type_string == ConfigParser::kStringTypeString) {
        bool in_quotes = false;
        bool expect_comma = false;
        bool expect_new_value = true;  // expect new value or whitespace
        std::string current_string = "";
        for (size_t i = 1; i + 1 < vector_string.size(); ++i) {  // skip opening and closing []
            const char ch = vector_string[i];
            current_string += ch;
            if (expect_comma && (ch == ',')) {  // expected and found comma
                expect_comma = false;
                expect_new_value = true;
            } else if (expect_comma) {  // expected but didn't find comma
                *error_flag = true;
                break;
            } else if (expect_new_value && (ch == '"')) {  // start new value
                expect_new_value = false;
                in_quotes = true;
                current_string = std::string{} + ch;
            } else if (expect_new_value && ConfigParser::is_space(ch)) {
                ;  // do nothing, just skipping through whitespace
            } else if (expect_new_value && !ConfigParser::is_space(ch)) {
                *error_flag = true;
                break;
            } else if (in_quotes && (ch == '"')) {  // end quotes, end current value
                in_quotes = false;
                expect_comma = true;
                value_strings.push_back(std::move(current_string));
                current_string = "";
            } else if (in_quotes && (ch != '"')) {
                ;     // do nothing, just staying in quotes
            } else {  // the above checks cover all allowed cases, otherwise there is an error
                *error_flag = true;
                break;
            }
        }
    } else {  // non-string type, value separated by comma then optional whitespace
        size_t index = 1;
        auto is_comma = [](char c) { return c == ','; };
        while (index + 1 < vector_string.size()) {  // skip opening and closing []
            std::string value_string = ReadNextToken(vector_string, &index, is_comma);
            if (std::find_if(value_string.begin(), value_string.end(), ConfigParser::is_space) !=
                value_string.end()) {  // true iff there is whitespace in value_string
                *error_flag = true;
                break;
            } else if (value_string.back() == ']') {
                value_string.pop_back();  // remove last closing bracket
            }
            value_strings.push_back(std::move(value_string));
            ++index;  // advance past the comma we just found
            SkipWhitespace(vector_string, &index);
        }
    }
    // DEBUG ONLY
    //    std::cout << "******** SplitVectorExpression *********" << std::endl;
    //    std::cout << "Input: " << vector_string << std::endl;
    //    std::cout << "----------------------------------------" << std::endl;
    //    for (const std::string& line : value_strings) std::cout << line << std::endl;
    //    std::cout << "****************************************" << std::endl;
    return value_strings;
}

bool CheckParseValue(const std::string& value_string, const std::string& type_string) {
    bool error_flag = true;  // default to error if type is unmatched in switch statement
    const ExpressionType type = ConfigParser::kTypeStringMap.at(type_string);
    switch (type) {
        case ExpressionType::kString: {
            ParseString(value_string, &error_flag);
            break;
        }
        case ExpressionType::kInt: {
            ParseInt(value_string, &error_flag);
            break;
        }
        case ExpressionType::kFloat: {
            ParseFloat(value_string, &error_flag);
            break;
        }
        case ExpressionType::kDouble: {
            ParseDouble(value_string, &error_flag);
            break;
        }
        case ExpressionType::kBool: {
            ParseBool(value_string, &error_flag);
            break;
        }
    }
    return !error_flag;  // return true if can parse successfully, false otherwise
}

bool CheckParseVector(const std::string& vector_string, const std::string& type_string) {
    bool error_flag = true;  // default to error if type is unmatched in switch statement
    const ExpressionType type = ConfigParser::kTypeStringMap.at(type_string);
    switch (type) {
        case ExpressionType::kString: {
            ParseStringVector(vector_string, &error_flag);
            break;
        }
        case ExpressionType::kInt: {
            ParseIntVector(vector_string, &error_flag);
            break;
        }
        case ExpressionType::kFloat: {
            ParseFloatVector(vector_string, &error_flag);
            break;
        }
        case ExpressionType::kDouble: {
            ParseDoubleVector(vector_string, &error_flag);
            break;
        }
        case ExpressionType::kBool: {
            ParseBoolVector(vector_string, &error_flag);
            break;
        }
    }
    return !error_flag;  // return true if can parse successfully, false otherwise
}

bool CheckParseExpression(const std::string& expression_string,
                          const std::string& type_string,
                          bool is_vector) {
    return is_vector ? CheckParseVector(expression_string, type_string)
                     : CheckParseValue(expression_string, type_string);
}

}  // namespace

ConfigParser::ConfigParser(const std::string& config_path)
    : _config_path(config_path), _line_number{0} {
    // Open config file
    std::ifstream input_filestream(config_path);
    if (!input_filestream.is_open()) {
        _error_messages.emplace_back("Error opening file: " + config_path);
        return;
    }
    const std::vector<std::string> declarations = PreprocessInput(input_filestream);
    for (const std::string& line : declarations) {
        ++_line_number;
        size_t current_index = 0;
        std::string type_string, name_string, equals_string, expression_string;
        bool is_vector;
        // Read type
        type_string = ReadNextToken(line, &current_index, ConfigParser::is_space);
        is_vector =
                (type_string.size() >= 2) && (type_string.substr(type_string.size() - 2) == "[]");
        if (is_vector) {
            type_string.erase(type_string.size() - 2);  // remove [] from type_string
        }
        //        std::cout << "type_string: " << type_string << std::endl;
        if (!TypeStringIsValid(type_string)) {
            AddErrorMessage("invalid type: " + type_string);
            return;
        }
        SkipWhitespace(line, &current_index);
        // Read name
        name_string = ReadNextToken(line, &current_index, ConfigParser::is_space);
        //        std::cout << "name_string: " << name_string << std::endl;
        if (_var_map.count(name_string)) {
            AddErrorMessage("redefinition of entity: " + name_string);
            return;
        }
        SkipWhitespace(line, &current_index);
        // Verify equals sign is next
        equals_string = ReadNextToken(line, &current_index, ConfigParser::is_space);
        //        std::cout << "equals_string: " << equals_string << std::endl;
        if (equals_string != "=") {
            AddErrorMessage("expected \"=\", encountered \"" + equals_string + "\"");
            return;
        }
        SkipWhitespace(line, &current_index);
        // If type is vector, look for enclosing []
        if (is_vector) {
            if ((line[current_index] != '[') || (line.back() != ']')) {
                AddErrorMessage("vector must be enclosed in []");
                return;
            }
            expression_string = line.substr(current_index);
            current_index = line.size();
        }
        // If type is string, look for enclosing ""
        else if (type_string == kStringTypeString) {
            if ((line[current_index] != '"') || (line.back() != '"')) {
                AddErrorMessage("string value must be enclosed in \"\"");
                return;
            }
            expression_string = line.substr(current_index);
            current_index = line.size();
        } else {  // single non-string value
            expression_string = ReadNextToken(line, &current_index, ConfigParser::is_space);
        }
        //        std::cout << "value_string: " << value_string << std::endl << std::endl;
        // Check if the value string is valid for the given type
        if (!CheckParseExpression(expression_string, type_string, is_vector)) {
            AddErrorMessage(std::string("could not parse `") + expression_string + "` as type " +
                            type_string + (is_vector ? "[]" : ""));
            return;
        }
        // We should now be at the end of the declaration
        if (current_index < line.size()) {
            AddErrorMessage(std::string("expected ") + kDeclarationTerminationChar + " at \"" +
                            line.substr(current_index) + "\"");
            return;
        }
        _var_map[name_string] = Variable{type_string, is_vector, expression_string};
    }
}

std::string ConfigParser::GetStringValue(const std::string& variable_name) {
    static const std::string expected_type_string = kStringTypeString;
    static const bool expected_is_vector = false;
    if (!CheckVariableExists(variable_name, expected_type_string, expected_is_vector)) {
        return {};
    }
    const std::string expression_string = _var_map[variable_name].expression_string;
    bool error_flag_unusued;  // parse check already done
    return ParseString(expression_string, &error_flag_unusued);
}

int ConfigParser::GetIntValue(const std::string& variable_name) {
    static const std::string expected_type_string = kIntTypeString;
    static const bool expected_is_vector = false;
    if (!CheckVariableExists(variable_name, expected_type_string, expected_is_vector)) {
        return {};
    }
    const std::string expression_string = _var_map[variable_name].expression_string;
    bool error_flag_unusued;  // parse check already done
    return ParseInt(expression_string, &error_flag_unusued);
}

float ConfigParser::GetFloatValue(const std::string& variable_name) {
    static const std::string expected_type_string = kFloatTypeString;
    static const bool expected_is_vector = false;
    if (!CheckVariableExists(variable_name, expected_type_string, expected_is_vector)) {
        return {};
    }
    const std::string expression_string = _var_map[variable_name].expression_string;
    bool error_flag_unusued;  // parse check already done
    return ParseFloat(expression_string, &error_flag_unusued);
}

double ConfigParser::GetDoubleValue(const std::string& variable_name) {
    static const std::string expected_type_string = kDoubleTypeString;
    static const bool expected_is_vector = false;
    if (!CheckVariableExists(variable_name, expected_type_string, expected_is_vector)) {
        return {};
    }
    const std::string expression_string = _var_map[variable_name].expression_string;
    bool error_flag_unusued;  // parse check already done
    return ParseDouble(expression_string, &error_flag_unusued);
}

bool ConfigParser::GetBoolValue(const std::string& variable_name) {
    static const std::string expected_type_string = kBoolTypeString;
    static const bool expected_is_vector = false;
    if (!CheckVariableExists(variable_name, expected_type_string, expected_is_vector)) {
        return {};
    }
    const std::string expression_string = _var_map[variable_name].expression_string;
    bool error_flag_unusued;  // parse check already done
    return ParseBool(expression_string, &error_flag_unusued);
}

std::vector<std::string> ConfigParser::GetStringVector(const std::string& variable_name) {
    static const std::string expected_type_string = kStringTypeString;
    static const bool expected_is_vector = true;
    if (!CheckVariableExists(variable_name, expected_type_string, expected_is_vector)) {
        return {};
    }
    const std::string expression_string = _var_map[variable_name].expression_string;
    bool error_flag_unusued;  // parse check already done
    return ParseStringVector(expression_string, &error_flag_unusued);
}

std::vector<int> ConfigParser::GetIntVector(const std::string& variable_name) {
    static const std::string expected_type_string = kIntTypeString;
    static const bool expected_is_vector = true;
    if (!CheckVariableExists(variable_name, expected_type_string, expected_is_vector)) {
        return {};
    }
    const std::string expression_string = _var_map[variable_name].expression_string;
    bool error_flag_unusued;  // parse check already done
    return ParseIntVector(expression_string, &error_flag_unusued);
}

std::vector<float> ConfigParser::GetFloatVector(const std::string& variable_name) {
    static const std::string expected_type_string = kFloatTypeString;
    static const bool expected_is_vector = true;
    if (!CheckVariableExists(variable_name, expected_type_string, expected_is_vector)) {
        return {};
    }
    const std::string expression_string = _var_map[variable_name].expression_string;
    bool error_flag_unusued;  // parse check already done
    return ParseFloatVector(expression_string, &error_flag_unusued);
}

std::vector<double> ConfigParser::GetDoubleVector(const std::string& variable_name) {
    static const std::string expected_type_string = kDoubleTypeString;
    static const bool expected_is_vector = true;
    if (!CheckVariableExists(variable_name, expected_type_string, expected_is_vector)) {
        return {};
    }
    const std::string expression_string = _var_map[variable_name].expression_string;
    bool error_flag_unusued;  // parse check already done
    return ParseDoubleVector(expression_string, &error_flag_unusued);
}

std::vector<bool> ConfigParser::GetBoolVector(const std::string& variable_name) {
    static const std::string expected_type_string = kBoolTypeString;
    static const bool expected_is_vector = true;
    if (!CheckVariableExists(variable_name, expected_type_string, expected_is_vector)) {
        return {};
    }
    const std::string expression_string = _var_map[variable_name].expression_string;
    bool error_flag_unusued;  // parse check already done
    return ParseBoolVector(expression_string, &error_flag_unusued);
}

size_t ConfigParser::ErrorCount() const {
    return _error_messages.size();
}

std::string ConfigParser::ErrorString() const {
    return _error_messages.size() ? _error_messages[0] : std::string("");
}

void ConfigParser::PrintVariableMap() const {
    std::cout << "Variable Map:" << std::endl;
    for (const auto& item : _var_map) {
        std::string variable_name = item.first;
        std::string type_string = item.second.type_string + (item.second.is_vector ? "[]" : "");
        std::string expression_string = item.second.expression_string;
        std::cout << "\t" << variable_name << " --> <" << type_string << "> : " << expression_string
                  << std::endl;
    }
}

/** End of public API **/

/** Helper Methods **/

bool ConfigParser::TypeStringIsValid(const std::string& type_string) {
    return std::find(kValidTypeStrings.begin(), kValidTypeStrings.end(), type_string) !=
           kValidTypeStrings.end();
}

bool ConfigParser::is_space(char c) {
    return (std::isspace(static_cast<unsigned char>(c)));
}

// Checks if a variable of the given type exists, and adds an error message if not found
bool ConfigParser::CheckVariableExists(const std::string& variable_name,
                                       const std::string& expected_type_string,
                                       bool expected_is_vector) {
    const std::string error_message = std::string("Error: didn't find variable ") + variable_name +
                                      " of type " + expected_type_string +
                                      (expected_is_vector ? "[]" : "");
    bool variable_exists = (_var_map.count(variable_name) != 0) &&
                           (_var_map.at(variable_name).type_string == expected_type_string) &&
                           (_var_map.at(variable_name).is_vector == expected_is_vector);
    if (!variable_exists) {
        _error_messages.emplace_back(std::move(error_message));
    }
    return variable_exists;
}

void ConfigParser::AddErrorMessage(const std::string& error_message) {
    _error_messages.push_back("Parsing error in file " + _config_path + ", line " +
                              std::to_string(_line_number) + ": " + error_message);
}
