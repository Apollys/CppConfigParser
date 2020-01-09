#include <iostream>

#include "config_parser.h"

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
    std::cout << std::endl;
    // Get and print string vector
    std::vector<std::string> words = config_parser.GetStringVector("words");
    std::cout << "words:" << std::endl;
    for (const std::string& word : words) std::cout << "   \"" << word << "\"" << std::endl;
    std::cout << std::endl;
    // Get and print int vector
    std::vector<int> primes = config_parser.GetIntVector("primes");
    std::cout << "primes:" << std::endl;
    for (int prime : primes) std::cout << "    " << prime << std::endl;
    std::cout << std::endl;
    // Get and print float vector
    std::vector<float> floats = config_parser.GetFloatVector("floats");
    std::cout << "floats:" << std::endl;
    for (float value : floats) std::cout << "    " << value << std::endl;
    std::cout << std::endl;
    // Get and print double vector
    std::vector<double> doubles = config_parser.GetDoubleVector("doubles");
    std::cout << "doubles:" << std::endl;
    for (double value : doubles) std::cout << "    " << value << std::endl;
    std::cout << std::endl;
    // Get and print bool vector
    std::vector<bool> bools = config_parser.GetBoolVector("bools");
    std::cout << "bools:" << std::endl;
    for (bool value : bools) std::cout << "    " << value << std::endl;
    std::cout << std::endl;
    
    if (config_parser.ErrorCount()) {
        std::cout << config_parser.ErrorString() << std::endl;
        return -1;
    } else {
        std::cout << "Completed, no errors" << std::endl;
    }
    
    return 0;
}
    
