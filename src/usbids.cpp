#include "usbids.hpp"

#include <cstdio>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#endif

namespace usbids {
namespace {

std::unordered_map<uint16_t, std::string>& vendors() {
    static std::unordered_map<uint16_t, std::string> m = {
        {0x03EB, "Atmel / Microchip"}, {0x03F0, "HP"}, {0x0403, "FTDI"}, {0x041E, "Creative"},
        {0x0424, "Microchip (SMSC)"}, {0x045E, "Microsoft"}, {0x046D, "Logitech"}, {0x0483, "STMicroelectronics"},
        {0x04B4, "Cypress / Infineon"}, {0x04CA, "Lite-On"}, {0x04D8, "Microchip"}, {0x04E8, "Samsung"},
        {0x04F2, "Chicony"}, {0x04F3, "Elan"}, {0x0502, "Acer"}, {0x054C, "Sony"}, {0x056A, "Wacom"},
        {0x058F, "Alcor Micro"}, {0x05AC, "Apple"}, {0x05E3, "Genesys Logic"}, {0x0781, "SanDisk"},
        {0x0951, "Kingston"}, {0x0B05, "ASUS"}, {0x0BDA, "Realtek"}, {0x0C45, "Microdia"}, {0x0CF3, "Qualcomm Atheros"},
        {0x0D8C, "C-Media"}, {0x0E8D, "MediaTek"}, {0x1050, "Yubico"}, {0x10C4, "Silicon Labs"}, {0x1209, "pid.codes"},
        {0x1235, "Focusrite"}, {0x13D3, "IMC Networks (AzureWave)"}, {0x1532, "Razer"}, {0x154B, "PNY"},
        {0x16C0, "Van Ooijen (V-USB / Teensy)"}, {0x174C, "ASMedia"}, {0x1A40, "Terminus Technology"},
        {0x1A86, "QinHeng (CH340)"}, {0x1B1C, "Corsair"}, {0x1D50, "OpenMoko (projetos open source)"},
        {0x1D6B, "Linux Foundation"}, {0x1E7D, "ROCCAT"}, {0x1FC9, "NXP"}, {0x2109, "VIA Labs"}, {0x2341, "Arduino"},
        {0x239A, "Adafruit"}, {0x2717, "Xiaomi"}, {0x27B8, "ThingM"}, {0x2833, "Oculus"}, {0x2E8A, "Raspberry Pi"},
        {0x303A, "Espressif"}, {0x3297, "ZSA"}, {0x8086, "Intel"}, {0x8087, "Intel"}, {0x0A12, "Cambridge Silicon Radio"},
        {0x0A5C, "Broadcom"}, {0x0B95, "ASIX"}, {0x1058, "Western Digital"}, {0x0BC2, "Seagate"}, {0x18D1, "Google"},
        {0x22B8, "Motorola"}, {0x12D1, "Huawei"}, {0x2C7C, "Quectel"}, {0x0525, "Netchip (gadget Linux)"},
        {0x1915, "Nordic Semiconductor"}, {0x0D28, "ARM (DAPLink)"}, {0x1366, "SEGGER (J-Link)"}, {0x15BA, "Olimex"},
        {0x0456, "Analog Devices"}, {0x0451, "Texas Instruments"}, {0x1CBE, "Luminary Micro (TI)"},
        {0x2B04, "Particle"}, {0x1B4F, "SparkFun"}, {0x20A0, "Clay Logic"}, {0x2047, "Texas Instruments"},
        {0x3434, "Keychron"}, {0x320F, "Evision"}, {0x258A, "Sinowealth"}, {0x1EA7, "SHARKOON"},
        {0x046A, "Cherry"}, {0x09DA, "A4Tech"}, {0x1038, "SteelSeries"}, {0x0738, "Mad Catz"}, {0x28DE, "Valve"},
        {0x057E, "Nintendo"}, {0x0E6F, "PDP"}, {0x2DC8, "8BitDo"}, {0x0FD9, "Elgato"}, {0x1A2C, "China Resource Semico"},
    };
    return m;
}

std::unordered_map<uint32_t, std::string>& products() {
    static std::unordered_map<uint32_t, std::string> m = {
        {0x046DC52Bu, "Unifying Receiver"}, {0x046DC548u, "Bolt Receiver"}, {0x1A867523u, "CH340 serial"},
        {0x04036001u, "FT232 serial (UART)"}, {0x04036010u, "FT2232 (JTAG/UART)"}, {0x04036014u, "FT232H"},
        {0x10C4EA60u, "CP210x UART bridge"}, {0x2E8A0003u, "RP2040 boot (BOOTSEL)"}, {0x2E8A000Au, "Pico SDK CDC"},
        {0x1D6B0002u, "Root hub 2.0"}, {0x1D6B0003u, "Root hub 3.0"}, {0x0483DF11u, "STM32 DFU bootloader"},
        {0x16C00483u, "Teensyduino serial"}, {0x16C00482u, "Teensyduino teclado/mouse/joystick"},
        {0x23410043u, "Uno R3"}, {0x23418036u, "Leonardo"},
        {0x1D50604Bu, "HackRF Jawbreaker"}, {0x1D506089u, "HackRF One"}, {0x1D506018u, "Black Magic Probe"},
        {0x1D50606Fu, "candleLight CAN"}, {0x1D50605Bu, "RfCat"}, {0x1D506002u, "Ubertooth"},
        {0x303A1001u, "ESP32-S3/C3 USB JTAG/serial"}, {0x0D280204u, "DAPLink CMSIS-DAP"},
    };
    return m;
}

bool g_loaded = false;

std::FILE* openUtf8(const std::string& path) {
#ifdef _WIN32
    int n = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    std::wstring w(size_t(n > 0 ? n : 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &w[0], n);
    return _wfopen(w.c_str(), L"rb");
#else
    return std::fopen(path.c_str(), "rb");
#endif
}

int hexVal(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool parseHex4(const char* s, uint16_t& out) {
    uint16_t v = 0;
    for (int i = 0; i < 4; ++i) {
        int h = hexVal(s[i]);
        if (h < 0) return false;
        v = uint16_t((v << 4) | h);
    }
    out = v;
    return true;
}

}  // namespace

size_t load(const std::string& path) {
    std::FILE* f = openUtf8(path);
    if (!f) return 0;
    char line[512];
    size_t count = 0;
    bool haveVendor = false;
    uint16_t curVid = 0;
    while (std::fgets(line, sizeof line, f)) {
        size_t n = 0;
        while (line[n] && line[n] != '\n' && line[n] != '\r') ++n;
        line[n] = '\0';
        if (n == 0 || line[0] == '#') continue;
        // Seções depois da lista de fabricantes (classes, HID usages...) começam com "C ", "AT ", "HID " etc.
        if (line[0] != '\t' && n > 1 && line[1] == ' ') break;
        uint16_t id;
        if (line[0] != '\t') {
            if (n > 6 && parseHex4(line, id) && line[4] == ' ') {
                curVid = id;
                haveVendor = true;
                vendors()[id] = line + 6;
                ++count;
            }
        } else if (line[1] != '\t' && haveVendor) {
            if (n > 7 && parseHex4(line + 1, id) && line[5] == ' ')
                products()[(uint32_t(curVid) << 16) | id] = line + 7;
        }
    }
    std::fclose(f);
    if (count) g_loaded = true;
    return count;
}

bool loaded() { return g_loaded; }

std::string vendor(uint16_t vid) {
    auto it = vendors().find(vid);
    return it == vendors().end() ? std::string() : it->second;
}

std::string product(uint16_t vid, uint16_t pid) {
    auto it = products().find((uint32_t(vid) << 16) | pid);
    return it == products().end() ? std::string() : it->second;
}

}  // namespace usbids
