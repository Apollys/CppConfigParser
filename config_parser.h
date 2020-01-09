/* A simple config parser, following the syntax of python with the addition of typed variables.
 *
 * The general syntax rules are as follows:
 *   - Whitespace at the beginning or end of any line is ignored
 *   - The character # indicates the start of a comment, continuing to the end of the line
 *   - Each line has whitespace/comments only, or a variable declaration
 *   - A single-valued variable declaration has the format:
 *     <typename> <variable_name> = <value>
 *     where valid typenames are string, int, float, douable, bool.
 *     Values of type string are enclosed in ""s, and bool values are true/false.
 *   - A std::vector variable declaration has the format:
 *     <typename>[] <variable_name> = [<value_0>, <optional_newline><value_1>, ...]
 *
 * Sample config:
 *   # my_config.cfg
 *   string message = "Hello Universe"
 *   int[] primes = [2, 3, 5, 7]
 *
 * Values are then retrieved via the Get{Typename}Value(variable_name) methods, e.g.:
 *   ConfigParser config_parser("my_config.cfg");
 *   std::string message = config_parser.GetStringValue("message");
 *   std::vector<int> primes = config_parser.GetIntVector("primes");
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// TODO: replace type_string and is_vector with enum everywhere possible
// Make enum specify vector/non-vector types, or make a struct containing {Type, IsVector}

// Attributes of a variable except for its name
struct Variable {
    std::string type_string;
    bool is_vector;
    std::string expression_string;
};

enum class ExpressionType {
    kString,
    kInt,
    kFloat,
    kDouble,
    kBool,
};

class ConfigParser {
  public:
    static const std::string kCommentPrefix;
    // Type names
    static const std::string kStringTypeString;
    static const std::string kIntTypeString;
    static const std::string kFloatTypeString;
    static const std::string kDoubleTypeString;
    static const std::string kBoolTypeString;
    static const std::vector<std::string> kValidTypeStrings;
    static const std::unordered_map<std::string, ExpressionType> kTypeStringMap;

    // Static utility functions
    static bool TypeStringIsValid(const std::string& type_string);
    static bool is_space(char c);

    ConfigParser(const std::string& config_path);

    size_t ErrorCount() const;
    std::string ErrorString() const;

    // Note: getters are not `const` because they may add error messages

    // Single value getters
    std::string GetStringValue(const std::string& variable_name);
    int GetIntValue(const std::string& variable_name);
    float GetFloatValue(const std::string& variable_name);
    double GetDoubleValue(const std::string& variable_name);
    bool GetBoolValue(const std::string& variable_name);
    // Vector getters
    std::vector<std::string> GetStringVector(const std::string& variable_name);
    std::vector<int> GetIntVector(const std::string& variable_name);
    std::vector<float> GetFloatVector(const std::string& variable_name);
    std::vector<double> GetDoubleVector(const std::string& variable_name);
    std::vector<bool> GetBoolVector(const std::string& variable_name);

    // Shows direct result of parsing, useful for debugging
    void PrintVariableMap() const;

  private:
    // Helper member functions

    bool CheckVariableExists(const std::string& variable_name,
                             const std::string& expected_type_string,
                             bool expected_is_vector);

    void AddErrorMessage(const std::string& error_message);

    // Member variables
    std::string _config_path;
    std::unordered_map<std::string, Variable> _var_map;
    std::vector<std::string> _error_messages;
    int _line_number;  // note: currently innaccurate because of preprocessing
};
