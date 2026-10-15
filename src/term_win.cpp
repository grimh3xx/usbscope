#include "term.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdlib>
#include <cwctype>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

namespace term {
namespace {

HANDLE g_out = INVALID_HANDLE_VALUE;
HANDLE g_in = INVALID_HANDLE_VALUE;
DWORD g_oldOutMode = 0, g_oldInMode = 0;
UINT g_oldOutCP = 0, g_oldInCP = 0;
bool g_active = false;

void rawWrite(const std::string& utf8) {
    if (utf8.empty()) return;
    std::wstring w = widen(utf8);
    DWORD written = 0;
    if (!WriteConsoleW(g_out, w.c_str(), DWORD(w.size()), &written, nullptr)) {
        // saída redirecionada (arquivo/pipe): escreve os bytes UTF-8 direto
        WriteFile(g_out, utf8.data(), DWORD(utf8.size()), &written, nullptr);
    }
}

BOOL WINAPI ctrlHandler(DWORD type) {
    if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT || type == CTRL_CLOSE_EVENT) {
        shutdown();
        ExitProcess(0);
    }
    return FALSE;
}

}  // namespace

std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
    std::wstring w(size_t(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), &w[0], n);
    return w;
}

std::string narrow(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), int(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(size_t(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), int(w.size()), &s[0], n, nullptr, nullptr);
    return s;
}

bool isWindowsTerminal() { return std::getenv("WT_SESSION") != nullptr; }

bool init() {
    g_out = GetStdHandle(STD_OUTPUT_HANDLE);
    g_in = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetConsoleMode(g_out, &g_oldOutMode)) return false;
    GetConsoleMode(g_in, &g_oldInMode);
    if (!SetConsoleMode(g_out, g_oldOutMode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        return false;  // Windows antigo sem suporte a VT
    // entrada: eventos de janela pra redesenhar no resize; sem modo de linha
    SetConsoleMode(g_in, ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS | (g_oldInMode & ENABLE_QUICK_EDIT_MODE));
    g_oldOutCP = GetConsoleOutputCP();
    g_oldInCP = GetConsoleCP();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    SetConsoleCtrlHandler(ctrlHandler, TRUE);
    g_active = true;
    rawWrite("\x1b[?1049h\x1b[?25l\x1b[H\x1b[2J");  // tela alternativa + cursor oculto
    return true;
}

void shutdown() {
    if (!g_active) return;
    g_active = false;
    rawWrite("\x1b[0m\x1b[?25h\x1b[?1049l");
    SetConsoleMode(g_out, g_oldOutMode);
    SetConsoleMode(g_in, g_oldInMode);
    if (g_oldOutCP) SetConsoleOutputCP(g_oldOutCP);
    if (g_oldInCP) SetConsoleCP(g_oldInCP);
}

int width() {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(g_out, &info)) return 100;
    return info.srWindow.Right - info.srWindow.Left + 1;
}

int height() {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(g_out, &info)) return 30;
    return info.srWindow.Bottom - info.srWindow.Top + 1;
}

void write(const std::string& utf8) { rawWrite(utf8); }

void present(const std::vector<std::string>& lines) {
    int h = height();
    std::string frame = "\x1b[H";
    int n = 0;
    for (const auto& l : lines) {
        if (n >= h) break;  // nunca rola a tela alternativa
        frame += l;
        frame += "\x1b[0m\x1b[K";
        if (++n < h) frame += "\r\n";
    }
    frame += "\x1b[J";
    rawWrite(frame);
}

KeyEvent readKey(int timeoutMs) {
    DWORD start = GetTickCount();
    for (;;) {
        DWORD wait = INFINITE;
        if (timeoutMs >= 0) {
            DWORD elapsed = GetTickCount() - start;
            if (elapsed >= DWORD(timeoutMs)) return {};
            wait = DWORD(timeoutMs) - elapsed;
        }
        if (WaitForSingleObject(g_in, wait) != WAIT_OBJECT_0) return {};
        DWORD pending = 0;
        if (!GetNumberOfConsoleInputEvents(g_in, &pending) || pending == 0) continue;
        INPUT_RECORD rec;
        DWORD got = 0;
        if (!ReadConsoleInputW(g_in, &rec, 1, &got) || got == 0) continue;

        if (rec.EventType == WINDOW_BUFFER_SIZE_EVENT) return {Key::Resize, 0, 0};
        if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown) continue;

        const KEY_EVENT_RECORD& k = rec.Event.KeyEvent;
        switch (k.wVirtualKeyCode) {
            case VK_UP: return {Key::Up, 0, 0};
            case VK_DOWN: return {Key::Down, 0, 0};
            case VK_LEFT: return {Key::Left, 0, 0};
            case VK_RIGHT: return {Key::Right, 0, 0};
            case VK_PRIOR: return {Key::PageUp, 0, 0};
            case VK_NEXT: return {Key::PageDown, 0, 0};
            case VK_HOME: return {Key::Home, 0, 0};
            case VK_END: return {Key::End, 0, 0};
            case VK_RETURN: return {Key::Enter, 0, 0};
            case VK_ESCAPE: return {Key::Escape, 0, 0};
            case VK_BACK: return {Key::Backspace, 0, 0};
            case VK_TAB: return {Key::Tab, 0, 0};
            default: break;
        }
        wchar_t c = k.uChar.UnicodeChar;
        if (c == 3) {  // Ctrl+C chega como tecla porque desligamos ENABLE_PROCESSED_INPUT
            shutdown();
            ExitProcess(0);
        }
        if (c >= 32) return {Key::Char, char32_t(std::towlower(c)), char32_t(c)};
    }
}

std::string readLine(const std::string& prompt) {
    int h = height();
    std::wstring buf;
    for (;;) {
        // redesenha a linha de entrada
        std::string line = "\x1b[" + std::to_string(h) + ";1H\x1b[2K" + prompt + narrow(buf) + "\x1b[?25h";
        rawWrite(line);
        KeyEvent ev = readKey();
        if (ev.key == Key::Enter) break;
        if (ev.key == Key::Escape) { buf.clear(); break; }
        if (ev.key == Key::Backspace) {
            if (!buf.empty()) buf.pop_back();
            continue;
        }
        if (ev.key == Key::Resize) { h = height(); continue; }
        if (ev.key != Key::Char) continue;
        buf += wchar_t(ev.raw);
    }
    rawWrite("\x1b[?25l");
    std::string out = narrow(buf);
    // arrastar arquivo pro terminal coloca aspas em volta
    while (!out.empty() && (out.back() == ' ' || out.back() == '"')) out.pop_back();
    size_t i = 0;
    while (i < out.size() && (out[i] == ' ' || out[i] == '"')) ++i;
    return out.substr(i);
}

void setTitle(const std::string& utf8) { SetConsoleTitleW(widen(utf8).c_str()); }

void sleepMs(int ms) { Sleep(DWORD(ms)); }

}  // namespace term
