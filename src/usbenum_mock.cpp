// Backend falso pra desenvolver/testar a UI fora do Windows (make test-ui).
// No Windows isto não entra no build; lá vale o usbenum_win.cpp.
#include "usbenum.hpp"

namespace usbenum {
namespace {

dev::Device make(uint16_t vid, uint16_t pid, usb::Speed sp, int port, const std::string& mfg,
                 const std::string& prod, const std::string& serial, const std::vector<uint8_t>& devDesc,
                 const std::vector<uint8_t>& cfg) {
    dev::Device d;
    d.vid = vid;
    d.pid = pid;
    d.speed = sp;
    d.port = port;
    d.hubPath = "\\\\.\\ROOTHUB";
    d.manufacturer = mfg;
    d.product = prod;
    d.serial = serial;
    d.instanceId = "USB\\VID_" + std::string("") + "/sim";
    d.rawDevice = devDesc;
    d.rawConfig = cfg;
    usb::parseDeviceDescriptor(devDesc.data(), devDesc.size(), d.desc);
    d.hasConfig = usb::parseConfiguration(cfg.data(), cfg.size(), d.config);
    dev::resolveNames(d);
    return d;
}

}  // namespace

std::vector<dev::Device> enumerate(std::string& error) {
    (void)error;
    std::vector<dev::Device> v;

    // Teclado HID boot
    v.push_back(make(0x046D, 0xC52B, usb::Speed::Full, 3, "Logitech", "USB Receiver", "",
                     {18, 1, 0x00, 0x02, 0, 0, 0, 8, 0x6D, 0x04, 0x2B, 0xC5, 0x12, 0x24, 1, 2, 0, 1},
                     {9, 2, 59, 0, 2, 1, 0, 0xA0, 49,
                      9, 4, 0, 0, 1, 3, 1, 1, 0, 9, 0x21, 0x11, 0x01, 0, 1, 0x22, 59, 0, 7, 5, 0x81, 3, 8, 0, 10,
                      9, 4, 1, 0, 1, 3, 1, 2, 0, 9, 0x21, 0x11, 0x01, 0, 1, 0x22, 148, 0, 7, 5, 0x82, 3, 8, 0, 10}));

    // Pendrive (mass storage)
    v.push_back(make(0x0781, 0x5581, usb::Speed::Super, 1, "SanDisk", "Ultra", "4C530001",
                     {18, 1, 0x00, 0x03, 0, 0, 0, 9, 0x81, 0x07, 0x81, 0x55, 0x10, 0x01, 1, 2, 3, 1},
                     {9, 2, 44, 0, 1, 1, 0, 0x80, 0x70,
                      9, 4, 0, 0, 2, 8, 6, 0x50, 0,
                      7, 5, 0x81, 2, 0x00, 0x04, 0, 6, 0x30, 15, 0, 0, 0,
                      7, 5, 0x02, 2, 0x00, 0x04, 0, 6, 0x30, 15, 0, 0, 0}));

    // Dispositivo suspeito: teclado + storage (padrão BadUSB)
    v.push_back(make(0x1A86, 0x5537, usb::Speed::Full, 5, "", "", "",
                     {18, 1, 0x10, 0x01, 0, 0, 0, 8, 0x86, 0x1A, 0x37, 0x55, 0x00, 0x01, 0, 0, 0, 1},
                     {9, 2, 55, 0, 2, 1, 0, 0x80, 0xFA,
                      9, 4, 0, 0, 1, 3, 1, 1, 0, 9, 0x21, 0x10, 0x01, 0, 1, 0x22, 65, 0, 7, 5, 0x81, 3, 8, 0, 2,
                      9, 4, 1, 0, 2, 8, 6, 0x50, 0, 7, 5, 0x82, 2, 0x40, 0, 0, 7, 5, 0x03, 2, 0x40, 0, 0}));

    // Pico em modo BOOTSEL
    v.push_back(make(0x2E8A, 0x0003, usb::Speed::Full, 2, "Raspberry Pi", "RP2 Boot", "E660",
                     {18, 1, 0x00, 0x02, 0, 0, 0, 64, 0x8A, 0x2E, 0x03, 0x00, 0x01, 0x01, 1, 2, 3, 1},
                     {9, 2, 32, 0, 1, 1, 0, 0x80, 0xFA,
                      9, 4, 0, 0, 2, 0xFF, 0, 0, 0, 7, 5, 0x81, 2, 0x40, 0, 0, 7, 5, 0x02, 2, 0x40, 0, 0}));

    // Hub
    v.push_back(make(0x05E3, 0x0610, usb::Speed::High, 4, "Genesys Logic", "USB2.0 Hub", "",
                     {18, 1, 0x00, 0x02, 9, 0, 1, 64, 0xE3, 0x05, 0x10, 0x06, 0x72, 0x92, 0, 0, 0, 1},
                     {9, 2, 25, 0, 1, 1, 0, 0xE0, 0x32, 9, 4, 0, 0, 1, 9, 0, 0, 0, 7, 5, 0x81, 3, 1, 0, 12}));

    for (auto& d : v) d.strings.clear();
    return v;
}

}  // namespace usbenum
