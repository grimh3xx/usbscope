#include "textutil.hpp"

#include <cstdarg>
#include <cstdio>

namespace txt {

const char* const RESET = "\x1b[0m";
const char* const BOLD = "\x1b[1m";
const char* const DIM = "\x1b[2m";
const char* const UNDERLINE = "\x1b[4m";
const char* const NO_UNDERLINE = "\x1b[24m";

std::string fg(uint8_t r, uint8_t g, uint8_t b) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "\x1b[38;2;%u;%u;%um", r, g, b);
    return buf;
}
std::string bg(uint8_t r, uint8_t g, uint8_t b) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "\x1b[48;2;%u;%u;%um", r, g, b);
    return buf;
}
std::string fg(uint32_t rgb) { return fg(uint8_t(rgb >> 16), uint8_t(rgb >> 8), uint8_t(rgb)); }
std::string bg(uint32_t rgb) { return bg(uint8_t(rgb >> 16), uint8_t(rgb >> 8), uint8_t(rgb)); }

static bool g_hyperlinks = false;
void setHyperlinks(bool enabled) { g_hyperlinks = enabled; }

std::string link(const std::string& url, const std::string& text) {
    if (!g_hyperlinks) return text;
    return "\x1b]8;;" + url + "\x1b\\" + text + "\x1b]8;;\x1b\\";
}

// Divide uma string UTF-8 em code points (cada um como substring).
static std::vector<std::string> codepoints(const std::string& s) {
    std::vector<std::string> out;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        size_t n = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 1;
        if (i + n > s.size()) n = s.size() - i;
        out.push_back(s.substr(i, n));
        i += n;
    }
    return out;
}

std::string gradient(const std::string& utf8, uint32_t from, uint32_t to) {
    auto cps = codepoints(utf8);
    std::string out;
    size_t n = cps.size();
    for (size_t i = 0; i < n; ++i) {
        double t = n > 1 ? double(i) / double(n - 1) : 0.0;
        auto mix = [&](int shift) {
            int a = (from >> shift) & 0xFF, b = (to >> shift) & 0xFF;
            return uint8_t(a + (b - a) * t + 0.5);
        };
        out += fg(mix(16), mix(8), mix(0));
        out += cps[i];
    }
    out += RESET;
    return out;
}

// Retorna o índice logo após a sequência de escape que começa em s[i] (== ESC).
static size_t skipEscape(const std::string& s, size_t i) {
    if (i + 1 >= s.size()) return i + 1;
    char c = s[i + 1];
    if (c == '[') {  // CSI: termina num byte 0x40..0x7E
        size_t j = i + 2;
        while (j < s.size() && !(s[j] >= 0x40 && s[j] <= 0x7E)) ++j;
        return j < s.size() ? j + 1 : j;
    }
    if (c == ']') {  // OSC: termina em BEL ou ESC '\'
        size_t j = i + 2;
        while (j < s.size()) {
            if (s[j] == '\x07') return j + 1;
            if (s[j] == '\x1b' && j + 1 < s.size() && s[j + 1] == '\\') return j + 2;
            ++j;
        }
        return j;
    }
    return i + 2;
}

size_t visibleWidth(const std::string& s) {
    size_t w = 0;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == 0x1b) { i = skipEscape(s, i); continue; }
        if ((c & 0xC0) != 0x80) ++w;
        ++i;
    }
    return w;
}

std::string stripAnsi(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        if (s[i] == '\x1b') { i = skipEscape(s, i); continue; }
        out += s[i++];
    }
    return out;
}

std::string fit(const std::string& s, size_t width) {
    size_t vis = visibleWidth(s);
    if (vis <= width) return s + std::string(width - vis, ' ');
    if (width == 0) return "";
    std::string out;
    size_t w = 0;
    bool hadEsc = false;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == 0x1b) {
            size_t j = skipEscape(s, i);
            out.append(s, i, j - i);
            hadEsc = true;
            i = j;
            continue;
        }
        size_t n = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 1;
        if (w + 1 >= width) break;  // deixa 1 coluna pro "…"
        out.append(s, i, n);
        ++w;
        i += n;
    }
    out += "…";
    if (hadEsc) out += RESET;
    return out;
}

std::string padLeft(const std::string& s, size_t width) {
    size_t vis = visibleWidth(s);
    return vis >= width ? s : std::string(width - vis, ' ') + s;
}

std::string center(const std::string& s, size_t width) {
    size_t vis = visibleWidth(s);
    return vis >= width ? s : std::string((width - vis) / 2, ' ') + s;
}

std::string repeat(const std::string& unit, size_t n) {
    std::string out;
    out.reserve(unit.size() * n);
    for (size_t i = 0; i < n; ++i) out += unit;
    return out;
}

std::string hex(uint64_t v, int digits) {
    static const char* d = "0123456789ABCDEF";
    std::string out(size_t(digits), '0');
    for (int i = digits - 1; i >= 0; --i) {
        out[size_t(i)] = d[v & 0xF];
        v >>= 4;
    }
    return out;
}

std::string format(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    int n = std::vsnprintf(nullptr, 0, fmt, ap);
    va_end(ap);
    std::string out(n > 0 ? size_t(n) : 0, '\0');
    if (n > 0) std::vsnprintf(&out[0], size_t(n) + 1, fmt, ap2);
    va_end(ap2);
    return out;
}

std::vector<std::string> hexDump(const uint8_t* data, size_t len, bool color) {
    std::vector<std::string> lines;
    const std::string off = color ? fg(0x6E7382) : "";
    const std::string hx = color ? fg(0xEBEEF5) : "";
    const std::string zero = color ? fg(0x4A4F5C) : "";
    const std::string asc = color ? fg(0x00BEFF) : "";
    const std::string rst = color ? RESET : "";
    for (size_t row = 0; row < len; row += 16) {
        std::string line = off + hex(row, 4) + rst + "  ";
        std::string ascii;
        for (size_t i = 0; i < 16; ++i) {
            if (row + i < len) {
                uint8_t b = data[row + i];
                line += (b == 0 ? zero : hx) + hex(b, 2) + rst + " ";
                ascii += (b >= 0x20 && b < 0x7F) ? char(b) : '.';
            } else {
                line += "   ";
            }
            if (i == 7) line += " ";
        }
        line += " " + asc + ascii + rst;
        lines.push_back(line);
    }
    return lines;
}

std::string bytesHuman(uint64_t n) {
    if (n < 1024) return format("%llu B", (unsigned long long)n);
    if (n < 1024ull * 1024) return format("%.1f KB", n / 1024.0);
    return format("%.1f MB", n / (1024.0 * 1024.0));
}

}  // namespace txt
