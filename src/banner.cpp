#include "banner.hpp"

#include "textutil.hpp"
#include "theme.hpp"

namespace {
#include "banner_data.inc"

int paletteIndex(char c) {
    if (c == ' ') return -1;
    for (int i = 0; kPaletteChars[i]; ++i)
        if (kPaletteChars[i] == c) return i;
    return -1;
}

// Cor do bloco (x, y) ou -1 se vazio / fora da área.
long long pixel(int x, int y) {
    if (y < 0 || y >= kLogoH || x < 0) return -1;
    const char* row = kLogo[y];
    for (int i = 0; i < x; ++i)
        if (!row[i]) return -1;
    int idx = paletteIndex(row[x]);
    return idx < 0 ? -1 : (long long)kPalette[idx];
}

// Converte as linhas [y0, y1) em linhas de terminal (2 linhas de pixel por linha de texto).
std::vector<std::string> renderRows(int y0, int y1, int leftPad) {
    std::vector<std::string> out;
    for (int y = y0; y < y1; y += 2) {
        std::string line(size_t(leftPad), ' ');
        long long curFg = -2, curBg = -2;  // -2 = estado desconhecido/padrão
        std::string pendingSpaces;
        for (int x = 0; x < kLogoW; ++x) {
            long long t = pixel(x, y);
            long long b = (y + 1 < y1) ? pixel(x, y + 1) : -1;
            if (t < 0 && b < 0) {
                pendingSpaces += ' ';
                continue;
            }
            // espaços pendentes precisam do fundo padrão
            if (!pendingSpaces.empty()) {
                if (curBg >= 0) { line += "\x1b[49m"; curBg = -2; }
                line += pendingSpaces;
                pendingSpaces.clear();
            }
            const char* glyph;
            long long wantFg, wantBg = -1;
            if (t >= 0 && b >= 0) {
                if (t == b) { glyph = "█"; wantFg = t; }
                else { glyph = "▀"; wantFg = t; wantBg = b; }
            } else if (t >= 0) {
                glyph = "▀"; wantFg = t;
            } else {
                glyph = "▄"; wantFg = b;
            }
            if (wantFg != curFg) { line += txt::fg(uint32_t(wantFg)); curFg = wantFg; }
            if (wantBg >= 0 && wantBg != curBg) { line += txt::bg(uint32_t(wantBg)); curBg = wantBg; }
            if (wantBg < 0 && curBg >= 0) { line += "\x1b[49m"; curBg = -2; }
            line += glyph;
        }
        line += txt::RESET;
        out.push_back(line);
    }
    return out;
}

int padFor(int termWidth, int contentWidth) {
    return termWidth > contentWidth ? (termWidth - contentWidth) / 2 : 0;
}

std::vector<std::string> headerLines(int leftPad) {
    using namespace theme;
    std::vector<std::string> out;
    const std::string pad(size_t(leftPad), ' ');

    // "S O L U T I O N S" espaçado embaixo do wordmark, como no samcli
    std::string sol(size_t(kLogoW), ' ');
    const char* word = "SOLUTIONS";
    for (int i = 0; word[i]; ++i) {
        size_t col = size_t(kSolutionsCol + i * 6);
        if (col < sol.size()) sol[col] = word[i];
    }
    while (!sol.empty() && sol.back() == ' ') sol.pop_back();
    out.push_back(pad + txt::fg(kAccent) + sol + txt::RESET);

    const std::string rule = txt::gradient(txt::repeat("─", size_t(kLogoW)), kBlue, kCyan);
    out.push_back(pad + rule);

    std::string title = txt::BOLD + txt::fg(kWhite) + std::string(kAppTitle) + txt::RESET +
                        txt::fg(kDim) + "  ·  " + kVersion + txt::RESET;
    out.push_back(pad + txt::center(title, size_t(kLogoW)));

    auto lnk = [](const std::string& host, const std::string& url) {
        return txt::fg(kCyan) + "► " + txt::fg(kText) + txt::UNDERLINE +
               txt::link(url, host) + txt::NO_UNDERLINE + txt::RESET;
    };
    std::string links = lnk(kSite, std::string("https://") + kSite) + "        " +
                        lnk(kGithub, std::string("https://") + kGithub);
    out.push_back(pad + txt::center(links, size_t(kLogoW)));
    out.push_back(pad + rule);
    return out;
}

constexpr int kHeaderRows = 5;
}  // namespace

namespace banner {

int logoWidth() { return kLogoW; }

int rowsFor(Tier tier) {
    switch (tier) {
        case Tier::Full: return (kLogoH + 1) / 2 + kHeaderRows;
        case Tier::Wordmark: return (kLogoH - kWordStart + 1) / 2 + kHeaderRows;
        case Tier::Text: return 3;
    }
    return 3;
}

Tier bestTier(int termWidth, int maxRows) {
    if (termWidth < kLogoW) return Tier::Text;
    if (maxRows >= rowsFor(Tier::Full)) return Tier::Full;
    if (maxRows >= rowsFor(Tier::Wordmark)) return Tier::Wordmark;
    return Tier::Text;
}

std::vector<std::string> build(int termWidth, Tier tier) {
    using namespace theme;
    std::vector<std::string> out;
    if (tier == Tier::Text) {
        std::string brand = txt::BOLD + txt::fg(kWhite) + "Central" + txt::fg(kCyan) + "Tech" + txt::RESET +
                            txt::fg(kAccent) + "  SOLUTIONS" + txt::RESET;
        std::string sub = txt::fg(kText) + kAppTitle + txt::fg(kDim) + "  ·  " + kVersion + "  ·  " + kSite +
                          txt::RESET;
        out.push_back(txt::center(brand, size_t(termWidth)));
        out.push_back(txt::center(sub, size_t(termWidth)));
        int w = termWidth > 4 ? termWidth - 4 : termWidth;
        out.push_back("  " + txt::gradient(txt::repeat("─", size_t(w)), kBlue, kCyan));
        return out;
    }
    int pad = padFor(termWidth, kLogoW);
    auto logo = renderRows(tier == Tier::Full ? 0 : kWordStart, kLogoH, pad);
    out.insert(out.end(), logo.begin(), logo.end());
    auto hdr = headerLines(pad);
    out.insert(out.end(), hdr.begin(), hdr.end());
    return out;
}

std::vector<std::string> topBar(int termWidth, const std::string& section) {
    using namespace theme;
    std::string left = txt::fg(kBlue) + "▌" + txt::BOLD + txt::fg(kWhite) + "Central" + txt::fg(kCyan) + "Tech" +
                       txt::RESET + txt::fg(kDim) + "  " + kAppTitle + txt::RESET;
    if (!section.empty()) left += txt::fg(kDim) + "  ›  " + txt::RESET + txt::BOLD + section + txt::RESET;
    std::string right = txt::fg(kDim) + kVersion + txt::RESET;
    size_t lw = txt::visibleWidth(left), rw = txt::visibleWidth(right);
    size_t total = size_t(termWidth > 1 ? termWidth - 1 : 1);
    std::string line = left + (lw + rw + 1 < total ? std::string(total - lw - rw, ' ') + right : "");
    std::vector<std::string> out{line};
    out.push_back(txt::gradient(txt::repeat("─", total), kBlue, kCyan));
    return out;
}

}  // namespace banner
