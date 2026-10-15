// Utilitários de texto portáveis (sem WinAPI): cores ANSI, largura visível, hex dump.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace txt {

// ---- Cores / estilos (sequências VT) ----
std::string fg(uint32_t rgb);
std::string bg(uint32_t rgb);
std::string fg(uint8_t r, uint8_t g, uint8_t b);
std::string bg(uint8_t r, uint8_t g, uint8_t b);
extern const char* const RESET;
extern const char* const BOLD;
extern const char* const DIM;
extern const char* const UNDERLINE;
extern const char* const NO_UNDERLINE;

// Hyperlink clicável (OSC 8) — Windows Terminal suporta; conhost só mostra o texto.
std::string link(const std::string& url, const std::string& text);
void setHyperlinks(bool enabled);  // desligado por padrão; ligue no Windows Terminal

// Texto com degradê horizontal entre duas cores.
std::string gradient(const std::string& utf8, uint32_t from, uint32_t to);

// ---- Medidas ----
size_t visibleWidth(const std::string& s);              // ignora ESC e conta code points
std::string stripAnsi(const std::string& s);
std::string fit(const std::string& s, size_t width);     // corta com "…" e completa com espaços
std::string padLeft(const std::string& s, size_t width);
std::string center(const std::string& s, size_t width);  // só o padding à esquerda
std::string repeat(const std::string& unit, size_t n);

// ---- Formatação ----
std::string hex(uint64_t v, int digits);                  // "0A3F"
std::string format(const char* fmt, ...);
std::vector<std::string> hexDump(const uint8_t* data, size_t len, bool color);
std::string bytesHuman(uint64_t n);

}  // namespace txt
