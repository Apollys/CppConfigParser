# CppConfigParser

A lightwieght C++ config system for quick algorithm development and testing

Supports variables of types `string`, `int`, `uint`, `float`, `double`, and `bool`, and vectors of each of those types.

### Format

The `#` character indicates the start of a **comment** that continues to the end of the line.

Aside from comments, the config file is a `;`-separated set of **declaration**s.

A **declaration** has the form `<type> <variable-name> = <expression>`.
 - `<type>` is one of the supported types listed above, optionally followed by `[]` to declare a vector of values.
 - `<variable-name>` is a sequence of any characters except whitespace or quotes.
 - `<expression>` is either a **single-value expression** or a **vector expression**, depending on the presence of `[]` suffixing the type.
   - A **single-value expression** has the form `"my_string"` for strings, `true` or `false` for bools, or a numeric literal for the numeric types.  For floating point types, infinities are supported as `inf` and `-inf`.
   - A **vector expression** has the form `[<value_1>, <value_2>, ..., <value_n>]`, where each of the `<value_i>` expressions is a single-value expression of the corresponding type.

Note: except for comments, this format is whitespace agnostic: any consecutive sequence of whitespace characters is equivalent to any other. This means that newlines and indents may be inserted in the place of a space anywhere in the declarations to format the config file more clearly.

 ### Example
 
 ```python
 # This is a sample config file
 string message = "Hello Universe";
 int[] perfect_numbers = [6, 28, 496];
 ```
 
 This file could be used in C++ as follows:
 
 ```c++
 ConfigParser config_parser(sample_config_path);
 std::string message = config_parser.GetStringValue("message");
 std::vector<int> perfect_numbers = config_parser.GetIntVector("perfect_numbers");
 ```
 
 For a more thorough example, see `test_config.cfg` and `parse_test.cpp` within this repository.
