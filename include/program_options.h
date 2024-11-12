#ifndef READER_WRITER_H
#define READER_WRITER_H

#include <stdexcept>
#include <vector>

namespace program_options {
    void parse(int argc, char* argv[]);
    bool is_int(std::string s);
    bool isFloat(const std::string& str);

    int n_epochs;
    int n_clinicians;
    int max_caseload;
    float arr_lam;
};


#endif