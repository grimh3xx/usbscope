// Parser de descritores USB (padrão USB 2.0/3.x) — portável, sem WinAPI.
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace usb {

enum class Speed { Unknown, Low, Full, High, Super, SuperPlus };
const char* speedName(Speed s);   // "High Speed (480 Mb/s)"
const char* speedShort(Speed s);  // "HS"

struct DeviceDescriptor {
    uint16_t bcdUSB = 0;
    uint8_t cls = 0, subCls = 0, proto = 0, maxPacket0 = 0;
    uint16_t vid = 0, pid = 0, bcdDevice = 0;
    uint8_t iManufacturer = 0, iProduct = 0, iSerial = 0, numConfigs = 0;
};

struct Endpoint {
    uint8_t address = 0, attributes = 0, interval = 0;
    uint16_t maxPacket = 0;
    bool ssCompanion = false;
    uint8_t ssMaxBurst = 0;
};

struct Interface {
    uint8_t number = 0, alt = 0, cls = 0, subCls = 0, proto = 0, iInterface = 0;
    std::vector<Endpoint> endpoints;
    uint16_t hidVersion = 0, hidReportLen = 0;
    uint8_t hidCountry = 0;
    std::vector<std::string> extras;  // descritores específicos de classe, já descritos
};

struct Configuration {
    uint8_t value = 0, attributes = 0, maxPower = 0, numInterfaces = 0, iConfiguration = 0;
    uint16_t totalLength = 0;
    std::vector<Interface> interfaces;
    std::vector<std::string> iads;  // Interface Association Descriptors
    std::vector<std::string> unknown;
};

bool parseDeviceDescriptor(const uint8_t* p, size_t n, DeviceDescriptor& out);
bool parseConfiguration(const uint8_t* p, size_t n, Configuration& out);
std::string parseStringDescriptor(const uint8_t* p, size_t n);  // UTF-16LE -> UTF-8

std::string bcd(uint16_t v);                 // 0x0210 -> "2.10"
std::string className(uint8_t cls);
std::string interfaceLabel(const Interface& i);  // "HID · boot · teclado"
std::string interfaceTag(const Interface& i);    // "HID-Kbd"
std::string descriptorTypeName(uint8_t type);
std::string endpointLabel(const Endpoint& e, Speed speed);
bool isKeyboard(const Interface& i);

// Resumo curto das classes ("HID-Kbd, MSC") pra tabela de dispositivos.
std::string classSummary(const DeviceDescriptor& d, const Configuration* cfg);

// ---- Notas de segurança (heurísticas) ----
enum class Level { Info, Warn, Alert };
struct Note {
    Level level;
    std::string text;
};
std::vector<Note> securityNotes(const DeviceDescriptor& d, const Configuration* cfg);

// ---- Descrição legível (linhas com ou sem cor) ----
using StringLookup = std::function<std::string(uint8_t index)>;
std::vector<std::string> describeDevice(const DeviceDescriptor& d, const StringLookup& str, bool color);
std::vector<std::string> describeConfig(const Configuration& c, Speed speed, const StringLookup& str, bool color);

}  // namespace usb
