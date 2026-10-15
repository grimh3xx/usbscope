// Logo CentralTech em pixel art (meio-blocos ▀▄ + cor 24-bit).
#pragma once
#include <string>
#include <vector>

namespace banner {

enum class Tier { Full, Wordmark, Text };

int logoWidth();

// Logo + cabeçalho (S O L U T I O N S, título, links), centralizados em termWidth.
std::vector<std::string> build(int termWidth, Tier tier);

// Escolhe o maior tier que cabe em maxRows linhas e termWidth colunas.
Tier bestTier(int termWidth, int maxRows);
int rowsFor(Tier tier);

// Barra de uma linha pro topo das telas internas: "▌CentralTech  USB Inspector › Seção".
std::vector<std::string> topBar(int termWidth, const std::string& section);

}  // namespace banner
