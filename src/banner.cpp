#include "banner.hpp"

#include "textutil.hpp"
#include "theme.hpp"

namespace {

// Logo "grimh3xx" em ANSI Shadow (figlet). "grim" branco, "h3xx" ciano.
constexpr int kBannerW = 63;
constexpr int kShadowRows = 6;

std::vector<std::string> shadowLogo(int leftPad) {
    using namespace theme;
    static const char* art[kShadowRows][2] = {
        {u8" ██████╗ ██████╗ ██╗███╗   ███╗", u8"██╗  ██╗██████╗ ██╗  ██╗██╗  ██╗"},
        {u8"██╔════╝ ██╔══██╗██║████╗ ████║", u8"██║  ██║╚════██╗╚██╗██╔╝╚██╗██╔╝"},
        {u8"██║  ███╗██████╔╝██║██╔████╔██║", u8"███████║ █████╔╝ ╚███╔╝  ╚███╔╝ "},
        {u8"██║   ██║██╔══██╗██║██║╚██╔╝██║", u8"██╔══██║ ╚═══██╗ ██╔██╗  ██╔██╗ "},
        {u8"╚██████╔╝██║  ██║██║██║ ╚═╝ ██║", u8"██║  ██║██████╔╝██╔╝ ██╗██╔╝ ██╗"},
        {u8" ╚═════╝ ╚═╝  ╚═╝╚═╝╚═╝     ╚═╝", u8"╚═╝  ╚═╝╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝"},
    };
    std::vector<std::string> out;
    const std::string pad(size_t(leftPad), ' ');
    for (int i = 0; i < kShadowRows; ++i)
        out.push_back(pad + txt::BOLD + txt::fg(kWhite) + art[i][0] + txt::fg(kCyan) + art[i][1] + txt::RESET);
    return out;
}

int padFor(int termWidth, int contentWidth) {
    return termWidth > contentWidth ? (termWidth - contentWidth) / 2 : 0;
}

std::vector<std::string> headerLines(int leftPad) {
    using namespace theme;
    std::vector<std::string> out;
    const std::string pad(size_t(leftPad), ' ');

    const std::string rule = txt::gradient(txt::repeat("─", size_t(kBannerW)), kBlue, kCyan);
    out.push_back(pad + rule);

    std::string title = txt::BOLD + txt::fg(kWhite) + std::string(kAppTitle) + txt::RESET +
                        txt::fg(kDim) + "  ·  " + kVersion + txt::RESET;
    out.push_back(pad + txt::center(title, size_t(kBannerW)));

    auto lnk = [](const std::string& host, const std::string& url) {
        return txt::fg(kCyan) + "► " + txt::fg(kText) + txt::UNDERLINE +
               txt::link(url, host) + txt::NO_UNDERLINE + txt::RESET;
    };
    std::string links = lnk(kSite, std::string("https://") + kSite) + "        " +
                        lnk(kGithub, std::string("https://") + kGithub);
    out.push_back(pad + txt::center(links, size_t(kBannerW)));
    out.push_back(pad + rule);
    return out;
}

constexpr int kHeaderRows = 4;
}  // namespace

namespace banner {

int logoWidth() { return kBannerW; }

int rowsFor(Tier tier) {
    switch (tier) {
        case Tier::Full: return kShadowRows + kHeaderRows;
        case Tier::Wordmark: return kShadowRows + kHeaderRows;
        case Tier::Text: return 3;
    }
    return 3;
}

Tier bestTier(int termWidth, int maxRows) {
    if (termWidth < kBannerW) return Tier::Text;
    if (maxRows >= rowsFor(Tier::Full)) return Tier::Full;
    if (maxRows >= rowsFor(Tier::Wordmark)) return Tier::Wordmark;
    return Tier::Text;
}

std::vector<std::string> build(int termWidth, Tier tier) {
    using namespace theme;
    std::vector<std::string> out;
    if (tier == Tier::Text) {
        std::string brand = txt::BOLD + txt::fg(kWhite) + "grim" + txt::fg(kCyan) + "h3xx" + txt::RESET;
        std::string sub = txt::fg(kText) + kAppTitle + txt::fg(kDim) + "  ·  " + kVersion + "  ·  " + kSite +
                          txt::RESET;
        out.push_back(txt::center(brand, size_t(termWidth)));
        out.push_back(txt::center(sub, size_t(termWidth)));
        int w = termWidth > 4 ? termWidth - 4 : termWidth;
        out.push_back("  " + txt::gradient(txt::repeat("─", size_t(w)), kBlue, kCyan));
        return out;
    }
    int pad = padFor(termWidth, kBannerW);
    auto logo = shadowLogo(pad);
    (void)tier;
    out.insert(out.end(), logo.begin(), logo.end());
    auto hdr = headerLines(pad);
    out.insert(out.end(), hdr.begin(), hdr.end());
    return out;
}

std::vector<std::string> topBar(int termWidth, const std::string& section) {
    using namespace theme;
    std::string left = txt::fg(kBlue) + "▌" + txt::BOLD + txt::fg(kWhite) + "grim" + txt::fg(kCyan) + "h3xx" +
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
