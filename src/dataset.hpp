// Leitura do conjunto de dados (pasta data/): empresa de demonstração e presets de equipe.
// Usado pela interface e pela CLI.

#pragma once

#include <string>
#include <vector>

#include "graph.hpp"

struct Preset {
    std::string id;  // nome do arquivo sem .txt (ex.: "agile_squad")
    Graph graph;
};

// Procura a pasta de dados: primeiro ao lado do executável (<exeDir>/../data), depois a partir da
// pasta atual (data). Retorna o caminho encontrado, ou "data" se nenhum existir.
std::string findDataDir(const std::string& executableDir);

// Lê todos os presets de <dataDir>/presets, em ordem de dificuldade (k, depois número de arestas).
bool loadPresets(const std::string& dataDir, std::vector<Preset>& presets, std::string& error);
