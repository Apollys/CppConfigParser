#include <iostream>

#include "config_parser.h"
#include "vector_ostream.hpp"

const std::string kConfigFilename = "test_config.cfg";

int main() {
    std::cout << std::boolalpha;
    // Parse config
    std::cout << "Parsing config file from: " << kConfigFilename << std::endl;
    ConfigParser config_parser(kConfigFilename);
    if (config_parser.ErrorCount()) {
        std::cout << config_parser.ErrorString() << std::endl;
        return -1;
    }
    config_parser.PrintVariableMap();
    std::cout << std::endl;
    
    // Get and print values
    std::cout << "height: " << config_parser.GetFloatValue("height") << std::endl;
    std::cout << "length: " << config_parser.GetIntValue("length") << std::endl;
    std::cout << "x: " << config_parser.GetDoubleValue("x") << std::endl;
    std::cout << "test_bool: " << config_parser.GetBoolValue("test_bool") << std::endl;
    // Get and print vectors
    std::cout << "words: " << config_parser.GetStringVector("words") << std::endl;
    std::cout << "primes: " << config_parser.GetIntVector("primes") << std::endl;
    std::cout << "floats: " << config_parser.GetFloatVector("floats") << std::endl;
    std::cout << "doubles: " << config_parser.GetDoubleVector("doubles") << std::endl;
    std::cout << "bools: " << config_parser.GetBoolVector("bools") << std::endl;
    std::cout << "empty_vector: " << config_parser.GetDoubleVector("empty_vector") << std::endl;
    std::cout << "infinities: " << config_parser.GetFloatVector("infinities") << std::endl;
    std::cout << std::endl;
    
    // Check for errors
    if (config_parser.ErrorCount()) {
        std::cout << config_parser.ErrorString() << std::endl;
        return -1;
    }
    std::cout << "Completed, no errors" << std::endl;
    return 0;
}
    
