#include "usbdesc.hpp"

#include <set>

#include "textutil.hpp"
#include "theme.hpp"

namespace usb {
namespace {

uint16_t le16(const uint8_t* p) { return uint16_t(p[0] | (p[1] << 8)); }

// Formatação "  rótulo ........ valor"
struct Fmt {
    bool color;
    std::string label(const std::string& s, size_t w = 18) const {
        std::string padded = s + std::string(s.size() < w ? w - txt::visibleWidth(s) : 1, ' ');
        return color ? txt::fg(theme::kDim) + padded + txt::RESET : padded;
    }
    std::string val(const std::string& s) const { return color ? txt::fg(theme::kWhite) + s + txt::RESET : s; }
    std::string note(const std::string& s) const { return color ? txt::fg(theme::kDim) + s + txt::RESET : s; }
    std::string accent(const std::string& s) const { return color ? txt::fg(theme::kCyan) + s + txt::RESET : s; }
    std::string strong(const std::string& s) const { return color ? std::string(txt::BOLD) + s + txt::RESET : s; }
    std::string section(const std::string& s) const {
        return color ? txt::BOLD + txt::fg(theme::kCyan) + s + txt::RESET : "== " + s + " ==";
    }
};

std::string withString(const Fmt& f, uint8_t idx, const StringLookup& str) {
    if (idx == 0) return f.val("0") + f.note("  (nenhuma)");
    std::string s = str ? str(idx) : std::string();
    return f.val(std::to_string(idx)) + (s.empty() ? f.note("  (não lida)") : "  " + f.accent("\"" + s + "\""));
}

std::string hidSubLabel(const Interface& i) {
    std::string s = i.subCls == 1 ? "boot" : "";
    const char* p = i.proto == 1 ? "teclado" : i.proto == 2 ? "mouse" : nullptr;
    if (p) s += (s.empty() ? "" : " · ") + std::string(p);
    return s;
}

std::string msProtocol(uint8_t p) {
    switch (p) {
        case 0x00: return "CBI c/ interrupção";
        case 0x01: return "CBI";
        case 0x50: return "Bulk-Only (BOT)";
        case 0x62: return "UAS";
        default: return "proto " + txt::hex(p, 2);
    }
}

std::string msSubclass(uint8_t s) {
    switch (s) {
        case 0x01: return "RBC";
        case 0x02: return "MMC-5 (ATAPI)";
        case 0x04: return "UFI (disquete)";
        case 0x06: return "SCSI";
        default: return "sub " + txt::hex(s, 2);
    }
}

}  // namespace

const char* speedName(Speed s) {
    switch (s) {
        case Speed::Low: return "Low Speed (1.5 Mb/s)";
        case Speed::Full: return "Full Speed (12 Mb/s)";
        case Speed::High: return "High Speed (480 Mb/s)";
        case Speed::Super: return "SuperSpeed (5 Gb/s)";
        case Speed::SuperPlus: return "SuperSpeed+ (10+ Gb/s)";
        default: return "desconhecida";
    }
}

const char* speedShort(Speed s) {
    switch (s) {
        case Speed::Low: return "LS 1.5M";
        case Speed::Full: return "FS 12M";
        case Speed::High: return "HS 480M";
        case Speed::Super: return "SS 5G";
        case Speed::SuperPlus: return "SS+ 10G";
        default: return "—";
    }
}

bool parseDeviceDescriptor(const uint8_t* p, size_t n, DeviceDescriptor& d) {
    if (n < 18 || p[0] < 18 || p[1] != 0x01) return false;
    d.bcdUSB = le16(p + 2);
    d.cls = p[4];
    d.subCls = p[5];
    d.proto = p[6];
    d.maxPacket0 = p[7];
    d.vid = le16(p + 8);
    d.pid = le16(p + 10);
    d.bcdDevice = le16(p + 12);
    d.iManufacturer = p[14];
    d.iProduct = p[15];
    d.iSerial = p[16];
    d.numConfigs = p[17];
    return true;
}

bool parseConfiguration(const uint8_t* p, size_t n, Configuration& c) {
    if (n < 9 || p[1] != 0x02) return false;
    c = Configuration{};
    c.totalLength = le16(p + 2);
    c.numInterfaces = p[4];
    c.value = p[5];
    c.iConfiguration = p[6];
    c.attributes = p[7];
    c.maxPower = p[8];
    size_t end = c.totalLength < n ? c.totalLength : n;

    Interface* cur = nullptr;
    for (size_t off = p[0]; off + 2 <= end;) {
        uint8_t len = p[off], type = p[off + 1];
        if (len < 2 || off + len > end) break;  // descritor malformado: para sem estourar o buffer
        const uint8_t* d = p + off;
        switch (type) {
            case 0x04:  // INTERFACE
                if (len >= 9) {
                    Interface i;
                    i.number = d[2];
                    i.alt = d[3];
                    i.cls = d[5];
                    i.subCls = d[6];
                    i.proto = d[7];
                    i.iInterface = d[8];
                    c.interfaces.push_back(i);
                    cur = &c.interfaces.back();
                }
                break;
            case 0x05:  // ENDPOINT
                if (len >= 7 && cur) {
                    Endpoint e;
                    e.address = d[2];
                    e.attributes = d[3];
                    e.maxPacket = le16(d + 4);
                    e.interval = d[6];
                    cur->endpoints.push_back(e);
                }
                break;
            case 0x30:  // SuperSpeed Endpoint Companion
                if (cur && !cur->endpoints.empty() && len >= 6) {
                    cur->endpoints.back().ssCompanion = true;
                    cur->endpoints.back().ssMaxBurst = d[2];
                }
                break;
            case 0x0B:  // Interface Association
                if (len >= 8)
                    c.iads.push_back(txt::format("IAD: interfaces %u–%u, função %s", d[2], d[2] + d[3] - 1,
                                                 className(d[4]).c_str()));
                break;
            case 0x21:  // HID (ou DFU functional, dependendo da classe)
                if (cur && cur->cls == 0x03 && len >= 9) {
                    cur->hidVersion = le16(d + 2);
                    cur->hidCountry = d[4];
                    // d[5] = número de descritores; d[6] = tipo (0x22 = report); d[7..8] = tamanho
                    if (d[6] == 0x22) cur->hidReportLen = le16(d + 7);
                } else if (cur && cur->cls == 0xFE && cur->subCls == 0x01 && len >= 7) {
                    uint8_t attr = d[2];
                    std::string caps;
                    if (attr & 0x01) caps += "download ";
                    if (attr & 0x02) caps += "upload ";
                    if (attr & 0x04) caps += "manifest-tolerant ";
                    if (attr & 0x08) caps += "will-detach ";
                    cur->extras.push_back(txt::format("DFU functional: %stransfer %u bytes, timeout detach %u ms",
                                                      caps.c_str(), le16(d + 5), le16(d + 3)));
                }
                break;
            case 0x24:  // CS_INTERFACE
                if (cur && (cur->cls == 0x02 || cur->cls == 0x0A) && len >= 3) {
                    static const char* names[] = {"Header", "Call Mgmt", "ACM", "Direct Line", "Telephone Ringer",
                                                  "Call/Line Cap", "Union", "Country", "Op Modes", "USB Terminal",
                                                  "Network Channel", "Protocol Unit", "Extension Unit",
                                                  "Multi-Channel", "CAPI", "Ethernet"};
                    uint8_t sub = d[2];
                    std::string name = sub < 16 ? names[sub] : "subtipo " + txt::hex(sub, 2);
                    cur->extras.push_back("CDC " + name);
                } else if (cur) {
                    cur->extras.push_back(txt::format("descritor de classe 0x24, %u bytes", len));
                }
                break;
            case 0x25:  // CS_ENDPOINT
                break;
            default:
                c.unknown.push_back(txt::format("tipo 0x%02X (%s), %u bytes", type, descriptorTypeName(type).c_str(),
                                                len));
        }
        off += len;
    }
    return true;
}

std::string parseStringDescriptor(const uint8_t* p, size_t n) {
    if (n < 2 || p[1] != 0x03) return {};
    size_t len = p[0] < n ? p[0] : n;
    std::string out;
    for (size_t i = 2; i + 1 < len; i += 2) {
        uint32_t cp = le16(p + i);
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 3 < len) {
            uint32_t lo = le16(p + i + 2);
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
                i += 2;
            }
        }
        if (cp == 0) break;
        if (cp < 0x80) out += char(cp);
        else if (cp < 0x800) { out += char(0xC0 | (cp >> 6)); out += char(0x80 | (cp & 0x3F)); }
        else if (cp < 0x10000) {
            out += char(0xE0 | (cp >> 12)); out += char(0x80 | ((cp >> 6) & 0x3F)); out += char(0x80 | (cp & 0x3F));
        } else {
            out += char(0xF0 | (cp >> 18)); out += char(0x80 | ((cp >> 12) & 0x3F));
            out += char(0x80 | ((cp >> 6) & 0x3F)); out += char(0x80 | (cp & 0x3F));
        }
    }
    // tira espaços no fim (alguns firmwares preenchem com espaço)
    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

std::string bcd(uint16_t v) {
    return txt::format("%x.%02x", v >> 8, v & 0xFF);
}

std::string className(uint8_t cls) {
    switch (cls) {
        case 0x00: return "definida por interface";
        case 0x01: return "Áudio";
        case 0x02: return "CDC (comunicação)";
        case 0x03: return "HID";
        case 0x05: return "Físico";
        case 0x06: return "Imagem (PTP/MTP)";
        case 0x07: return "Impressora";
        case 0x08: return "Armazenamento em massa";
        case 0x09: return "Hub";
        case 0x0A: return "CDC-Data";
        case 0x0B: return "Smart Card";
        case 0x0D: return "Content Security";
        case 0x0E: return "Vídeo (UVC)";
        case 0x0F: return "Personal Healthcare";
        case 0x10: return "Áudio/Vídeo";
        case 0x11: return "Billboard";
        case 0x12: return "USB-C Bridge";
        case 0x3C: return "I3C";
        case 0xDC: return "Diagnóstico";
        case 0xE0: return "Controlador sem fio";
        case 0xEF: return "Miscelânea";
        case 0xFE: return "Específica de aplicação";
        case 0xFF: return "Vendor-specific";
        default: return "classe " + txt::hex(cls, 2);
    }
}

bool isKeyboard(const Interface& i) { return i.cls == 0x03 && i.proto == 1; }

std::string interfaceLabel(const Interface& i) {
    std::string base = className(i.cls);
    switch (i.cls) {
        case 0x03: {
            std::string s = hidSubLabel(i);
            return s.empty() ? base : base + " · " + s;
        }
        case 0x08: return base + " · " + msSubclass(i.subCls) + " · " + msProtocol(i.proto);
        case 0x02:
            if (i.subCls == 0x02) return "CDC-ACM (porta serial virtual)";
            if (i.subCls == 0x06) return "CDC-ECM (Ethernet)";
            if (i.subCls == 0x0D) return "CDC-NCM (rede)";
            return base;
        case 0x01: return base + (i.subCls == 1 ? " · controle" : i.subCls == 2 ? " · streaming" : i.subCls == 3 ? " · MIDI" : "");
        case 0x0E: return base + (i.subCls == 1 ? " · controle" : i.subCls == 2 ? " · streaming" : "");
        case 0xE0:
            if (i.subCls == 0x01 && i.proto == 0x01) return "Bluetooth";
            if (i.subCls == 0x01 && i.proto == 0x03) return "RNDIS (rede)";
            return base;
        case 0xFE:
            if (i.subCls == 0x01) return "DFU (atualização de firmware)";
            if (i.subCls == 0x02) return "IrDA bridge";
            if (i.subCls == 0x03) return "USB Test & Measurement";
            return base;
        default: return base;
    }
}

std::string interfaceTag(const Interface& i) {
    switch (i.cls) {
        case 0x01: return "Audio";
        case 0x02: return i.subCls == 0x02 ? "Serial" : "CDC";
        case 0x03: return i.proto == 1 ? "Teclado" : i.proto == 2 ? "Mouse" : "HID";
        case 0x06: return "Imagem";
        case 0x07: return "Impressora";
        case 0x08: return "Storage";
        case 0x09: return "Hub";
        case 0x0A: return "";  // dados do CDC, já coberto
        case 0x0B: return "SmartCard";
        case 0x0E: return "Video";
        case 0xE0: return i.proto == 0x01 ? "Bluetooth" : "Wireless";
        case 0xFE: return i.subCls == 0x01 ? "DFU" : "App";
        case 0xFF: return "Vendor";
        default: return txt::hex(i.cls, 2);
    }
}

std::string descriptorTypeName(uint8_t t) {
    switch (t) {
        case 0x01: return "DEVICE";
        case 0x02: return "CONFIGURATION";
        case 0x03: return "STRING";
        case 0x04: return "INTERFACE";
        case 0x05: return "ENDPOINT";
        case 0x06: return "DEVICE_QUALIFIER";
        case 0x07: return "OTHER_SPEED_CONFIG";
        case 0x08: return "INTERFACE_POWER";
        case 0x0B: return "INTERFACE_ASSOCIATION";
        case 0x0F: return "BOS";
        case 0x10: return "DEVICE_CAPABILITY";
        case 0x21: return "HID";
        case 0x22: return "HID_REPORT";
        case 0x23: return "HID_PHYSICAL";
        case 0x24: return "CS_INTERFACE";
        case 0x25: return "CS_ENDPOINT";
        case 0x29: return "HUB";
        case 0x2A: return "SS_HUB";
        case 0x30: return "SS_EP_COMPANION";
        default: return "0x" + txt::hex(t, 2);
    }
}

std::string endpointLabel(const Endpoint& e, Speed speed) {
    static const char* types[] = {"Control", "Isochronous", "Bulk", "Interrupt"};
    uint8_t type = e.attributes & 0x03;
    bool in = e.address & 0x80;
    uint16_t size = e.maxPacket & 0x7FF;
    uint16_t extra = (e.maxPacket >> 11) & 0x3;
    std::string s = txt::format("EP 0x%02X %-3s %-11s %4u bytes", e.address, in ? "IN" : "OUT", types[type], size);
    if (extra) s += txt::format(" x%u", extra + 1);
    if (type == 1 || type == 3) {
        bool fast = speed == Speed::High || speed == Speed::Super || speed == Speed::SuperPlus;
        if (fast || type == 1) {
            uint8_t iv = e.interval ? e.interval : 1;
            if (iv > 16) iv = 16;
            double us = (1u << (iv - 1)) * (fast ? 125.0 : 1000.0);
            s += us >= 1000 ? txt::format(", a cada %.0f ms", us / 1000.0) : txt::format(", a cada %.0f µs", us);
        } else {
            s += txt::format(", a cada %u ms", e.interval);
        }
    }
    if (e.ssCompanion && e.ssMaxBurst) s += txt::format(", burst %u", e.ssMaxBurst + 1);
    return s;
}

std::string classSummary(const DeviceDescriptor& d, const Configuration* cfg) {
    std::vector<std::string> tags;
    std::set<std::string> seen;
    auto add = [&](const std::string& t) {
        if (!t.empty() && seen.insert(t).second) tags.push_back(t);
    };
    if (cfg) {
        for (const auto& i : cfg->interfaces)
            if (i.alt == 0) add(interfaceTag(i));
    }
    if (tags.empty() && d.cls != 0x00 && d.cls != 0xEF) {
        Interface fake;
        fake.cls = d.cls;
        fake.subCls = d.subCls;
        fake.proto = d.proto;
        add(interfaceTag(fake));
    }
    std::string out;
    for (size_t i = 0; i < tags.size(); ++i) out += (i ? "+" : "") + tags[i];
    return out.empty() ? "—" : out;
}

std::vector<Note> securityNotes(const DeviceDescriptor& d, const Configuration* cfg) {
    std::vector<Note> notes;
    if (!cfg) return notes;
    bool kbd = false, mouse = false, storage = false, cdc = false, vendor = false, dfu = false, video = false,
         audioIn = false, net = false;
    for (const auto& i : cfg->interfaces) {
        if (isKeyboard(i)) kbd = true;
        if (i.cls == 0x03 && i.proto == 2) mouse = true;
        if (i.cls == 0x08) storage = true;
        if (i.cls == 0x02 && i.subCls == 0x02) cdc = true;
        if (i.cls == 0x02 && (i.subCls == 0x06 || i.subCls == 0x0D)) net = true;
        if (i.cls == 0xE0 && i.proto == 0x03) net = true;
        if (i.cls == 0xFF) vendor = true;
        if (i.cls == 0xFE && i.subCls == 0x01) dfu = true;
        if (i.cls == 0x0E) video = true;
        if (i.cls == 0x01 && i.subCls == 0x02)
            for (const auto& e : i.endpoints)
                if (e.address & 0x80) audioIn = true;
    }
    if (kbd && (storage || cdc || net)) {
        std::string other = storage ? "armazenamento" : cdc ? "porta serial" : "rede";
        notes.push_back({Level::Alert, "Teclado HID + " + other +
                                           " no mesmo dispositivo: combinação incomum. Confirme se é esperado "
                                           "antes de confiar nele."});
    } else if (kbd) {
        notes.push_back({Level::Warn, "Expõe teclado HID: normal em teclados e receptores; suspeito em pendrive, "
                                      "cabo ou carregador."});
    }
    if (dfu) notes.push_back({Level::Warn, "Interface DFU exposta: o firmware pode ser lido/regravado via USB."});
    if (net) notes.push_back({Level::Warn, "Adaptador de rede USB: pode adicionar rotas/DNS novos na máquina; "
                                           "confira se é esperado."});
    if (cdc) notes.push_back({Level::Info, "Porta serial virtual (CDC-ACM): console de debug/UART acessível por COM."});
    if (vendor) notes.push_back({Level::Info, "Interface vendor-specific (0xFF): protocolo proprietário, bom alvo pra "
                                              "engenharia reversa."});
    if (storage) notes.push_back({Level::Info, "Armazenamento em massa: verifique autorun/arquivos antes de abrir."});
    if (video) notes.push_back({Level::Info, "Câmera (UVC) presente."});
    if (audioIn) notes.push_back({Level::Info, "Captura de áudio (microfone) presente."});
    if (mouse && kbd && !storage && !cdc) {
        notes.push_back({Level::Info, "Teclado + mouse no mesmo receptor: típico de receptores wireless; "
                                      "mantenha o firmware atualizado."});
    }
    if (d.iSerial == 0)
        notes.push_back({Level::Info, "Sem número de série: difícil de distinguir de clones com o mesmo VID:PID."});
    return notes;
}

std::vector<std::string> describeDevice(const DeviceDescriptor& d, const StringLookup& str, bool color) {
    Fmt f{color};
    std::vector<std::string> out;
    out.push_back(f.section("Descritor de dispositivo"));
    out.push_back("  " + f.label("bcdUSB") + f.val(bcd(d.bcdUSB)));
    out.push_back("  " + f.label("Classe") + f.val(txt::format("%02X %02X %02X", d.cls, d.subCls, d.proto)) +
                  f.note("  " + className(d.cls)));
    out.push_back("  " + f.label("bMaxPacketSize0") +
                  f.val(d.bcdUSB >= 0x0300 && d.maxPacket0 < 32 ? txt::format("%u", 1u << d.maxPacket0)
                                                                 : std::to_string(d.maxPacket0)) +
                  f.note(" bytes"));
    out.push_back("  " + f.label("idVendor") + f.val("0x" + txt::hex(d.vid, 4)));
    out.push_back("  " + f.label("idProduct") + f.val("0x" + txt::hex(d.pid, 4)));
    out.push_back("  " + f.label("bcdDevice") + f.val(bcd(d.bcdDevice)));
    out.push_back("  " + f.label("iManufacturer") + withString(f, d.iManufacturer, str));
    out.push_back("  " + f.label("iProduct") + withString(f, d.iProduct, str));
    out.push_back("  " + f.label("iSerialNumber") + withString(f, d.iSerial, str));
    out.push_back("  " + f.label("bNumConfigurations") + f.val(std::to_string(d.numConfigs)));
    return out;
}

std::vector<std::string> describeConfig(const Configuration& c, Speed speed, const StringLookup& str, bool color) {
    Fmt f{color};
    std::vector<std::string> out;
    out.push_back(f.section(txt::format("Configuração %u", c.value)));
    std::string attr;
    attr += (c.attributes & 0x40) ? "auto-alimentado" : "alimentado pelo barramento";
    if (c.attributes & 0x20) attr += ", remote wakeup";
    bool ss = speed == Speed::Super || speed == Speed::SuperPlus;
    unsigned ma = c.maxPower * (ss ? 8u : 2u);
    out.push_back("  " + f.label("wTotalLength") + f.val(std::to_string(c.totalLength)) + f.note(" bytes"));
    out.push_back("  " + f.label("bNumInterfaces") + f.val(std::to_string(c.numInterfaces)));
    out.push_back("  " + f.label("Atributos") + f.val(attr));
    out.push_back("  " + f.label("MaxPower") + f.val(std::to_string(ma) + " mA"));
    if (c.iConfiguration) out.push_back("  " + f.label("iConfiguration") + withString(f, c.iConfiguration, str));
    for (const auto& s : c.iads) out.push_back("  " + f.note(s));

    for (const auto& i : c.interfaces) {
        out.push_back("");
        std::string head = txt::format("Interface %u", i.number);
        if (i.alt) head += txt::format(" (alt %u)", i.alt);
        std::string lbl = interfaceLabel(i);
        out.push_back("  " + f.strong(head) + "  " + (isKeyboard(i) && color ? txt::fg(theme::kYellow) + lbl + txt::RESET
                                                                                : f.accent(lbl)));
        out.push_back("    " + f.label("Classe", 16) + f.val(txt::format("%02X %02X %02X", i.cls, i.subCls, i.proto)));
        if (i.iInterface) out.push_back("    " + f.label("iInterface", 16) + withString(f, i.iInterface, str));
        if (i.hidVersion)
            out.push_back("    " + f.label("HID", 16) +
                          f.val(txt::format("versão %s, report descriptor %u bytes, país %u", bcd(i.hidVersion).c_str(),
                                            i.hidReportLen, i.hidCountry)));
        for (const auto& x : i.extras) out.push_back("    " + f.note(x));
        if (i.endpoints.empty()) out.push_back("    " + f.note("sem endpoints (só EP0)"));
        for (const auto& e : i.endpoints) out.push_back("    " + f.val(endpointLabel(e, speed)));
    }
    if (!c.unknown.empty()) {
        out.push_back("");
        out.push_back("  " + f.note("Outros descritores:"));
        for (const auto& s : c.unknown) out.push_back("    " + f.note(s));
    }
    return out;
}

}  // namespace usb
