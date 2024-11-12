#include "program_options.h"

#include <stdexcept>
#include <vector>

namespace {
    static int n_epochs = 10000;
    static int n_clinicians = 80;
    static int max_caseload = 1;
    static float arr_lam = 10;
}

bool program_options::is_int(std::string s) {
    for (char c : s) {
        if (!isdigit(c)) {
            return false;
        }
    }
}

bool program_options::isFloat(const std::string& str)
{
    if (str.empty())
        return false;

    char* ptr;
    strtof(str.c_str(), &ptr);
    return (*ptr) == '\0';
}

void program_options::parse(int argc, char* argv[]) {
    if (argc > 64) {
        throw std::runtime_error("Too many arguments passed!");
    }
  
    for (int i = 1; i < argc; i++) {
        if (argv[i] == "-n") {
            if (i + 1 < argc) {
                if (program_options::is_int(argv[i+1])) {
                    n_epochs = std::stoi(argv[i+1]);
                } else {
                    throw std::runtime_error("Invalid value provided for -n flag");
                }
            } else {
                throw std::runtime_error("No value provided for -n flag");
            }
        }

        if (argv[i] == "-nc") {
            if (i + 1 < argc) {
                if (program_options::is_int(argv[i+1])) {
                    n_clinicians = std::stoi(argv[i+1]);
                } else {
                    throw std::runtime_error("Invalid value provided for -nc flag");
                }
            } else {
                throw std::runtime_error("No value provided for -nc flag");
            }
        }

        if (argv[i] == "-mc") {
            if (i + 1 < argc) {
                if (program_options::is_int(argv[i+1])) {
                    max_caseload = std::stoi(argv[i+1]);
                } else {
                    throw std::runtime_error("Invalid value provided for -mc flag");
                }
            } else {
                throw std::runtime_error("No value provided for -mc flag");
            }
        }

        if (argv[i] == "-lam") {
            if (i + 1 < argc) {
                if (isFloat(argv[i+1])) {
                    arr_lam = std::stof(argv[i+1]);
                } else {
                    throw std::runtime_error("Invalid value provided for -lam flag");
                }
            } else {
                throw std::runtime_error("No value provided for -lam flag");
            }
        }
    }
}