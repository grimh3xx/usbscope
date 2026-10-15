// usbscope — inspetor de USB interativo pro terminal. (c) CentralTech Solutions.
#include <algorithm>
#include <string>
#include <vector>

#include "banner.hpp"
#include "device.hpp"
#include "term.hpp"
#include "textutil.hpp"
#include "theme.hpp"
#include "usbenum.hpp"
#include "usbids.hpp"

using namespace theme;

namespace {

char levelGlyph(usb::Level l) { return l == usb::Level::Alert ? '!' : l == usb::Level::Warn ? '*' : '-'; }
uint32_t levelColor(usb::Level l) {
    return l == usb::Level::Alert ? kRed : l == usb::Level::Warn ? kYellow : kDim;
}

class App {
public:
    int run() {
        if (!term::init()) {
            term::write("Este programa precisa do Windows Terminal ou de um console com suporte a VT.\n");
            return 1;
        }
        term::setTitle("CentralTech · usbscope");
        if (term::isWindowsTerminal()) txt::setHyperlinks(true);
        refresh();
        loop();
        term::shutdown();
        printExitSummary();
        return 0;
    }

private:
    enum class Screen { List, Detail };
    Screen screen_ = Screen::List;
    std::vector<dev::Device> devices_;
    std::vector<size_t> filtered_;  // índices visíveis (após filtro)
    std::string filter_;
    std::string status_;
    size_t sel_ = 0;     // posição na lista filtrada
    size_t top_ = 0;     // primeira linha visível (scroll da lista)
    int detailScroll_ = 0;
    int detailTab_ = 0;  // 0 = visão geral, 1 = hex device, 2 = hex config
    bool running_ = true;

    const dev::Device* current() const {
        if (sel_ >= filtered_.size()) return nullptr;
        return &devices_[filtered_[sel_]];
    }

    void refresh() {
        std::string err;
        devices_ = usbenum::enumerate(err);
        // hubs por último, resto por porta
        std::stable_sort(devices_.begin(), devices_.end(), [](const dev::Device& a, const dev::Device& b) {
            if (a.isHub() != b.isHub()) return !a.isHub();
            return a.port < b.port;
        });
        applyFilter();
        status_ = err.empty() ? txt::format("%zu dispositivos", devices_.size())
                              : (devices_.empty() ? err : status_);
        if (sel_ >= filtered_.size()) sel_ = filtered_.empty() ? 0 : filtered_.size() - 1;
    }

    void applyFilter() {
        filtered_.clear();
        std::string f = lower(filter_);
        for (size_t i = 0; i < devices_.size(); ++i) {
            if (f.empty() || matches(devices_[i], f)) filtered_.push_back(i);
        }
        if (sel_ >= filtered_.size()) sel_ = filtered_.empty() ? 0 : filtered_.size() - 1;
    }

    static std::string lower(std::string s) {
        for (char& c : s) c = char(std::tolower((unsigned char)c));
        return s;
    }
    static bool matches(const dev::Device& d, const std::string& f) {
        std::string hay = lower(d.title() + " " + d.bestVendor() + " " + d.vidpid() + " " +
                                usb::classSummary(d.desc, d.hasConfig ? &d.config : nullptr));
        return hay.find(f) != std::string::npos;
    }

    // ---------- loop ----------
    void loop() {
        while (running_) {
            draw();
            term::KeyEvent ev = term::readKey();
            if (ev.key == term::Key::Resize) continue;
            if (screen_ == Screen::List) handleList(ev);
            else handleDetail(ev);
        }
    }

    void handleList(const term::KeyEvent& ev) {
        using K = term::Key;
        switch (ev.key) {
            case K::Up: if (sel_) --sel_; break;
            case K::Down: if (sel_ + 1 < filtered_.size()) ++sel_; break;
            case K::Home: sel_ = 0; break;
            case K::End: if (!filtered_.empty()) sel_ = filtered_.size() - 1; break;
            case K::PageUp: sel_ = sel_ > 8 ? sel_ - 8 : 0; break;
            case K::PageDown: sel_ = std::min(sel_ + 8, filtered_.empty() ? 0 : filtered_.size() - 1); break;
            case K::Enter:
                if (current()) { screen_ = Screen::Detail; detailScroll_ = 0; detailTab_ = 0; }
                break;
            case K::Char:
                if (ev.ch == 'q') running_ = false;
                else if (ev.ch == 'r') { status_ = "atualizando…"; draw(); refresh(); }
                else if (ev.ch == '/') promptFilter();
                else if (ev.ch == 'c' && !filter_.empty()) { filter_.clear(); applyFilter(); }
                break;
            case K::Escape:
                if (!filter_.empty()) { filter_.clear(); applyFilter(); }
                else running_ = false;
                break;
            default: break;
        }
        clampScroll();
    }

    void handleDetail(const term::KeyEvent& ev) {
        using K = term::Key;
        int page = std::max(1, term::height() - 10);
        switch (ev.key) {
            case K::Up: if (detailScroll_) --detailScroll_; break;
            case K::Down: ++detailScroll_; break;
            case K::PageUp: detailScroll_ = std::max(0, detailScroll_ - page); break;
            case K::PageDown: detailScroll_ += page; break;
            case K::Home: detailScroll_ = 0; break;
            case K::Left: detailTab_ = (detailTab_ + 2) % 3; detailScroll_ = 0; break;
            case K::Right:
            case K::Tab: detailTab_ = (detailTab_ + 1) % 3; detailScroll_ = 0; break;
            case K::Escape: screen_ = Screen::List; break;
            case K::Char:
                if (ev.ch == 'q') screen_ = Screen::List;
                else if (ev.ch == 'h' || ev.ch == '1') { detailTab_ = 0; detailScroll_ = 0; }
                else if (ev.ch == '2') { detailTab_ = 1; detailScroll_ = 0; }
                else if (ev.ch == '3') { detailTab_ = 2; detailScroll_ = 0; }
                break;
            default: break;
        }
    }

    void promptFilter() {
        std::string p = txt::fg(kCyan) + "  filtro ▸ " + txt::RESET;
        std::string r = term::readLine(p);
        filter_ = r;
        sel_ = 0;
        top_ = 0;
        applyFilter();
    }

    void clampScroll() {
        int rows = listRows();
        if (rows <= 0) return;
        if (sel_ < top_) top_ = sel_;
        if (sel_ >= top_ + size_t(rows)) top_ = sel_ - rows + 1;
    }

    int listRows() const {
        int h = term::height();
        int head = banner::rowsFor(banner::bestTier(term::width(), h - 8)) + 2;  // +cabeçalho da tabela
        return std::max(1, h - head - 2);  // -rodapé
    }

    // ---------- desenho ----------
    void draw() {
        if (screen_ == Screen::List) drawList();
        else drawDetail();
    }

    void footer(std::vector<std::string>& out, const std::string& keys) {
        int w = term::width();
        while (out.size() < size_t(std::max(0, term::height() - 1))) out.push_back("");
        std::string left = txt::fg(kDim) + keys + txt::RESET;
        std::string right = txt::fg(kDim) + status_ + txt::RESET;
        size_t lw = txt::visibleWidth(left), rw = txt::visibleWidth(right);
        std::string line = " " + left;
        if (lw + rw + 3 < size_t(w)) line += std::string(w - lw - rw - 3, ' ') + right + " ";
        out.push_back(txt::gradient(txt::repeat("─", size_t(std::max(1, w))), kFaint, kFaint));
        if (out.size() < size_t(term::height())) out.push_back(line);
    }

    void drawList() {
        int w = term::width(), h = term::height();
        std::vector<std::string> out;
        auto tier = banner::bestTier(w, h - 8);
        for (auto& l : banner::build(w, tier)) out.push_back(l);
        out.push_back("");

        // cabeçalho da tabela
        int rows = listRows();
        clampScroll();
        Cols cols = columns(w);
        out.push_back(tableHeader(cols));

        if (filtered_.empty()) {
            out.push_back("");
            out.push_back(txt::center(txt::fg(kDim) + (devices_.empty() ? "nenhum dispositivo USB detectado"
                                                                        : "nada corresponde ao filtro") + txt::RESET,
                                      size_t(w)));
        }
        for (int i = 0; i < rows; ++i) {
            size_t idx = top_ + size_t(i);
            if (idx >= filtered_.size()) break;
            out.push_back(deviceRow(devices_[filtered_[idx]], cols, idx == sel_, w));
        }

        std::string keys = "↑↓ mover   ↵ abrir   / filtrar   r atualizar   q sair";
        if (!filter_.empty()) keys = "filtro: \"" + filter_ + "\"   c limpar   " + keys;
        footer(out, keys);
        term::present(out);
    }

    struct Cols { int sel, idx, port, speed, vidpid, name, vendor, cls; };
    Cols columns(int w) const {
        Cols c{};
        c.sel = 2; c.idx = 4; c.port = 6; c.speed = 8;
        c.vidpid = 11;
        int rest = w - (c.sel + c.idx + c.port + c.speed + c.vidpid) - 8;
        if (rest < 24) rest = 24;
        c.cls = std::min(16, rest / 4);
        c.vendor = std::min(22, (rest - c.cls) / 3);
        c.name = std::max(10, rest - c.cls - c.vendor);
        return c;
    }

    std::string tableHeader(const Cols& c) const {
        auto cell = [](const std::string& s, int w) { return txt::fit(s, size_t(std::max(0, w))); };
        std::string h = std::string(c.sel, ' ') + txt::fg(kDim) +
                        cell("#", c.idx) + cell("Porta", c.port) + cell("Vel", c.speed) +
                        cell("VID:PID", c.vidpid) + cell("Dispositivo", c.name) +
                        cell("Fabricante", c.vendor) + cell("Classe", c.cls) + txt::RESET;
        return h;
    }

    std::string deviceRow(const dev::Device& d, const Cols& c, bool selected, int w) {
        (void)w;
        auto cell = [](const std::string& s, int wid) { return txt::fit(s, size_t(std::max(0, wid))); };
        size_t num = &d - devices_.data();
        std::string cls = usb::classSummary(d.desc, d.hasConfig ? &d.config : nullptr);

        auto notes = usb::securityNotes(d.desc, d.hasConfig ? &d.config : nullptr);
        bool alert = false, warn = false;
        for (auto& n : notes) { alert |= n.level == usb::Level::Alert; warn |= n.level == usb::Level::Warn; }

        // Cores dependem de estar selecionado (fundo azul) ou não.
        const std::string reset = selected ? txt::bg(kSelBg) : std::string(txt::RESET);
        uint32_t flagC = alert ? kRed : warn ? kYellow : (selected ? kSelBg : kFaint);
        uint32_t idxC = kDim, portC = kText, speedC = kCyan, vidC = selected ? kCyan : kAccent;
        uint32_t nameC = d.isHub() ? (selected ? 0x9AA4B8 : kDim) : kWhite;
        uint32_t vendC = selected ? 0xD4DAE6 : kText;
        uint32_t clsC = selected ? 0xAAB4CC : kDim;

        std::string marker = selected ? txt::fg(kCyan) + "▌" + txt::RESET + txt::bg(kSelBg) + " "
                                       : std::string("  ");
        std::string flag = (alert || warn) ? txt::fg(flagC) + std::string(alert ? "!" : "*") : reset + " ";

        std::string row = marker + flag + " " +
                          txt::fg(idxC) + cell(std::to_string(num + 1), c.idx) +
                          txt::fg(portC) + cell(std::to_string(d.port), c.port) +
                          txt::fg(speedC) + cell(usb::speedShort(d.speed), c.speed) +
                          txt::fg(vidC) + cell(d.vidpid(), c.vidpid) +
                          (selected ? std::string(txt::BOLD) : "") + txt::fg(nameC) + cell(d.title(), c.name) +
                          (selected ? "\x1b[22m" : "") +
                          txt::fg(vendC) + cell(d.bestVendor(), c.vendor) +
                          txt::fg(clsC) + cell(cls, c.cls);
        return row + txt::RESET;
    }

    // ---------- detalhes ----------
    void drawDetail() {
        const dev::Device* dp = current();
        if (!dp) { screen_ = Screen::List; return; }
        const dev::Device& d = *dp;
        int w = term::width();
        std::vector<std::string> out;
        for (auto& l : banner::topBar(w, d.title())) out.push_back(l);
        out.push_back(tabBar());
        out.push_back("");

        std::vector<std::string> body;
        if (detailTab_ == 0) body = overview(d, w);
        else {
            const std::vector<uint8_t>& raw = detailTab_ == 1 ? d.rawDevice : d.rawConfig;
            body.push_back(txt::BOLD + txt::fg(kCyan) +
                           (detailTab_ == 1 ? "Descritor de dispositivo (cru)" : "Descritor de configuração (cru)") +
                           txt::RESET);
            body.push_back(txt::fg(kDim) + txt::format("%zu bytes", raw.size()) + txt::RESET);
            body.push_back("");
            if (raw.empty()) body.push_back(txt::fg(kDim) + "  (não disponível)" + txt::RESET);
            for (auto& l : txt::hexDump(raw.data(), raw.size(), true)) body.push_back("  " + l);
        }

        int avail = term::height() - int(out.size()) - 2;
        int maxScroll = std::max(0, int(body.size()) - avail);
        if (detailScroll_ > maxScroll) detailScroll_ = maxScroll;
        for (int i = 0; i < avail; ++i) {
            size_t idx = size_t(detailScroll_ + i);
            out.push_back(idx < body.size() ? "  " + body[idx] : "");
        }
        std::string more = maxScroll > 0 ? txt::format("  %d/%d", detailScroll_ + 1, maxScroll + 1) : "";
        status_ = d.vidpid() + (d.serial.empty() ? "" : "  SN " + d.serial) + more;
        footer(out, "←→/tab seções   ↑↓ rolar   esc voltar   q lista");
        term::present(out);
    }

    std::string tabBar() {
        const char* names[] = {"Visão geral", "Hex: device", "Hex: config"};
        std::string s = " ";
        for (int i = 0; i < 3; ++i) {
            bool on = i == detailTab_;
            s += (on ? txt::bg(kSelBg) + txt::BOLD + txt::fg(kCyan) : txt::fg(kDim));
            s += "  " + std::to_string(i + 1) + " " + names[i] + "  ";
            s += std::string(txt::RESET) + " ";
        }
        return s;
    }

    std::vector<std::string> overview(const dev::Device& d, int w) {
        std::vector<std::string> b;
        auto kv = [&](const std::string& k, const std::string& v, uint32_t vc = kWhite) {
            b.push_back("  " + txt::fg(kDim) + txt::fit(k, 16) + txt::RESET + txt::fg(vc) + v + txt::RESET);
        };
        // cabeçalho resumido
        b.push_back(txt::BOLD + txt::fg(kWhite) + d.title() + txt::RESET + txt::fg(kDim) + "   " + d.vidpid() +
                    txt::RESET);
        std::string vend = d.bestVendor();
        if (!d.manufacturer.empty() && !d.vendorName.empty() && d.vendorName != d.manufacturer)
            vend += txt::fg(kDim) + "  (" + d.vendorName + ")" + txt::RESET;
        kv("Fabricante", vend);
        if (!d.productName.empty()) kv("Produto (IDs)", d.productName);
        kv("Velocidade", usb::speedName(d.speed), kCyan);
        kv("Porta", std::to_string(d.port) + (d.locationPath.empty() ? "" : "   " + d.locationPath));
        if (!d.serial.empty()) kv("Nº de série", d.serial);
        if (!d.driver.empty()) kv("Driver", d.driver);
        if (!d.service.empty()) kv("Serviço", d.service);
        if (!d.instanceId.empty()) kv("Instance ID", d.instanceId, kDim);
        if (!d.problem.empty()) kv("Estado", d.problem, kRed);

        // notas de segurança em destaque, logo no topo
        auto notes = usb::securityNotes(d.desc, d.hasConfig ? &d.config : nullptr);
        if (!notes.empty()) {
            b.push_back("");
            b.push_back(txt::BOLD + txt::fg(kCyan) + "Observações de segurança" + txt::RESET);
            for (auto& n : notes) {
                std::string tag(1, levelGlyph(n.level));
                std::string head = txt::fg(levelColor(n.level)) + " " + tag + " " + txt::RESET;
                for (auto& seg : wrap(n.text, std::max(20, w - 8))) {
                    b.push_back("  " + head + txt::fg(kText) + seg + txt::RESET);
                    head = "   ";  // continuação alinhada
                }
            }
        }

        b.push_back("");
        auto str = [&](uint8_t idx) -> std::string {
            auto it = d.strings.find(idx);
            return it == d.strings.end() ? std::string() : it->second;
        };
        for (auto& l : usb::describeDevice(d.desc, str, true)) b.push_back(l);
        if (d.hasConfig) {
            b.push_back("");
            for (auto& l : usb::describeConfig(d.config, d.speed, str, true)) b.push_back(l);
        }
        return b;
    }

    static std::vector<std::string> wrap(const std::string& s, int width) {
        std::vector<std::string> out;
        std::string line, word;
        auto flushWord = [&]() {
            if (word.empty()) return;
            if (!line.empty() && int(line.size() + 1 + word.size()) > width) { out.push_back(line); line.clear(); }
            if (!line.empty()) line += ' ';
            line += word;
            word.clear();
        };
        for (char c : s) {
            if (c == ' ') flushWord();
            else word += c;
        }
        flushWord();
        if (!line.empty()) out.push_back(line);
        return out;
    }

    void printExitSummary() {
        term::write(txt::BOLD + txt::fg(kWhite) + "Central" + txt::fg(kCyan) + "Tech" + txt::RESET +
                    txt::fg(kDim) + " · usbscope — até a próxima.\n" + txt::RESET);
    }
};

}  // namespace

int runApp() {
    // base de IDs opcional: usb.ids ao lado do executável
    usbids::load("usb.ids");
    App app;
    return app.run();
}
