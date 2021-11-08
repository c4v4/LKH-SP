extern "C" {
#include "LKH.h"
}

#include <fstream>
#include <sstream>
#include <vector>

/* TODO: convert it to C code */
extern "C" void Read_InitialTour_Sol(const char *FileName) {
    assert(NodeSet);

    std::vector<int> tour;
    std::ifstream rfile;
    std::string line;

    rfile.open(FileName);
    std::string delimiter = ":";
    std::string delimiter_num = " ";
    int file_type = 0;
    if (rfile.is_open()) {
        if (!getline(rfile, line).eof()) {
            if (line.find("Route") != std::string::npos) {
                std::stringstream lineStream(line.substr(line.find(delimiter) + 1, line.size()));
                int value;
                tour.push_back(MTSPDepot);
                while (lineStream >> value)
                    tour.push_back(value + 1);
            } else if (line.find("Instance ") == std::string::npos)  // CVRPTW: file_type = 0, but with some intro lines
            {
                file_type = 1;
                getline(rfile, line);  // nvehicles
                getline(rfile, line);  //?
                getline(rfile, line);  //?
            }

            while (!getline(rfile, line).eof()) {
                if (!file_type && line.find("Route") != std::string::npos) {
                    std::stringstream lineStream(line.substr(line.find(delimiter) + 1, line.size()));
                    int value;
                    tour.push_back(MTSPDepot);
                    while (lineStream >> value)
                        tour.push_back(value + 1);
                } else if (file_type) {
                    std::stringstream lineStream(line);
                    int value;
                    for (int i = 0; i < 7; ++i)
                        lineStream >> value;  // ignore first 7 integers
                    while (lineStream >> value)
                        tour.push_back(value + 1);
                }
                // else ignore line
            }
        }
    }

    if (!tour.empty()) {
        tour.push_back(tour[0]);
        SetInitialTour(tour.data());
    }
}