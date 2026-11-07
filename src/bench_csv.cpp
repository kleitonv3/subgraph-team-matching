#include "bench_csv.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

const char* benchCsvHeader() { return "solver,series,preset,mode,n,k,m,seed,found,candidates,time_ms,timeout"; }

std::string benchCsvLine(const BenchRow& row) {
    char time[64];
    std::snprintf(time, sizeof time, "%.4f", row.timeMs);
    std::ostringstream line;
    line << row.solver << ',' << row.series << ',' << row.preset << ',' << row.mode << ',' << row.n << ',' << row.k
         << ',' << row.m << ',' << row.seed << ',' << (row.found ? 1 : 0) << ',' << row.candidates << ',' << time << ','
         << (row.timeout ? 1 : 0);
    return line.str();
}

bool readBenchCsv(const std::string& path, std::vector<BenchRow>& rows, std::string& error) {
    std::ifstream file(path);
    if (!file) {
        error = path + ": arquivo não encontrado";
        return false;
    }

    std::vector<BenchRow> result;
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || lineNumber == 1) continue;  // cabeçalho

        std::vector<std::string> fields;
        std::stringstream stream(line);
        std::string field;
        while (std::getline(stream, field, ',')) fields.push_back(field);
        if (fields.size() != 12) {
            error = path + ":" + std::to_string(lineNumber) + ": esperava 12 colunas, encontrou " +
                    std::to_string(fields.size());
            return false;
        }

        BenchRow row;
        row.solver = fields[0];
        row.series = fields[1];
        row.preset = fields[2];
        row.mode = fields[3];
        row.n = std::atoi(fields[4].c_str());
        row.k = std::atoi(fields[5].c_str());
        row.m = std::atoi(fields[6].c_str());
        row.seed = std::strtoull(fields[7].c_str(), nullptr, 10);
        row.found = fields[8] == "1";
        row.candidates = std::strtoull(fields[9].c_str(), nullptr, 10);
        row.timeMs = std::strtod(fields[10].c_str(), nullptr);
        row.timeout = fields[11] == "1";
        result.push_back(row);
    }
    rows = std::move(result);
    return true;
}
