// Console do Windows: sequências VT, UTF-8, teclas e desenho de tela sem flicker.
#pragma once
#include <string>
#include <vector>

namespace term {

enum class Key { None, Up, Down, Left, Right, PageUp, PageDown, Home, End, Enter, Escape, Backspace, Tab, Resize, Char };

struct KeyEvent {
    Key key = Key::None;
    char32_t ch = 0;   // quando key == Key::Char: caractere em minúscula (pra atalhos)
    char32_t raw = 0;  // caractere original (pra texto digitado)
};

bool init();      // liga VT + UTF-8, entra na tela alternativa, esconde o cursor
void shutdown();  // restaura tudo (também roda no Ctrl+C)

int width();
int height();

void write(const std::string& utf8);
// Desenha um quadro inteiro: volta o cursor pro topo, escreve as linhas e limpa o resto.
void present(const std::vector<std::string>& lines);

// timeoutMs < 0 = espera pra sempre; retorna Key::None se o tempo acabar.
KeyEvent readKey(int timeoutMs = -1);

// Mostra o prompt na última linha e lê uma linha de texto (UTF-8). Esc/vazio = "".
std::string readLine(const std::string& prompt);

void setTitle(const std::string& utf8);
void sleepMs(int ms);
bool isWindowsTerminal();

std::wstring widen(const std::string& utf8);
std::string narrow(const std::wstring& w);

}  // namespace term
