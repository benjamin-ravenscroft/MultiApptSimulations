#ifndef WRITE_CSV_H
#define WRITE_CSV_H

#include <string>
#include <vector>
#include <fstream>

void write_csv(std::string filename, std::vector<std::pair<std::string, double>> dataset) {
    std::ofstream file(filename);

    for (int j=0; j < dataset.size(); j++) {
        file << dataset[j].first;
        if (j != dataset.size() - 1) file << ",";
    }
    file << "\n";

    for (int j=0; j < dataset.size(); j++) {
        file << dataset[j].second;
        if (j != dataset.size() - 1) file << ",";
    }
    file << "\n";

    file.close();
}

#endif