#include "dataset.hpp"

#include <algorithm>
#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

std::string findDataDir(const std::string& executableDir) {
    std::error_code ec;
    if (!executableDir.empty()) {
        const std::string besideExe = executableDir + "/../data";
        if (fs::is_directory(fs::u8path(besideExe), ec)) return besideExe;
    }
    return "data";
}

bool loadPresets(const std::string& dataDir, std::vector<Preset>& presets, std::string& error) {
    const fs::path dir = fs::u8path(dataDir + "/presets");
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) {
        error = dataDir + "/presets: pasta não encontrada";
        return false;
    }

    std::vector<Preset> loaded;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".txt") continue;
        Preset preset;
        preset.id = entry.path().stem().u8string();
        if (!loadGraphFile(entry.path().u8string(), preset.graph, error)) return false;
        loaded.push_back(std::move(preset));
    }
    if (loaded.empty()) {
        error = dataDir + "/presets: nenhum preset (.txt) encontrado";
        return false;
    }

    std::sort(loaded.begin(), loaded.end(), [](const Preset& a, const Preset& b) {
        if (a.graph.vertexCount() != b.graph.vertexCount()) return a.graph.vertexCount() < b.graph.vertexCount();
        if (a.graph.edgeCount() != b.graph.edgeCount()) return a.graph.edgeCount() < b.graph.edgeCount();
        return a.id < b.id;
    });
    presets = std::move(loaded);
    return true;
}
