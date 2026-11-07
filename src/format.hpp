// Formatação de números para mostrar ao usuário (padrão brasileiro).

#pragma once

#include <cstdint>
#include <string>

// 1234567 -> "1.234.567"
std::string formatThousands(std::uint64_t value);

// Duração legível: "0,412 ms", "3,250 s"
std::string formatDuration(double seconds);

// "1 colaboração" / "3 colaborações"
std::string formatCount(std::uint64_t count, const char* singular, const char* plural);
