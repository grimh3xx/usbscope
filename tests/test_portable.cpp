// Testes das partes portáveis (rodam em Linux/Windows sem hardware USB).
#include <cassert>
#include <cstdio>
#include "../src/banner.hpp"
#include "../src/textutil.hpp"
#include "../src/usbdesc.hpp"

int main(int argc, char**) {
    // texto
    assert(txt::visibleWidth("\x1b[38;2;1;2;3mabc\x1b[0m") == 3);
    assert(txt::visibleWidth("ação") == 4);
    assert(txt::visibleWidth(txt::fit("abcdef", 4)) == 4);
    assert(txt::visibleWidth(txt::fit("ab", 5)) == 5);
    txt::setHyperlinks(true);
    assert(txt::stripAnsi(txt::link("https://x", "y")) == "y");
    assert(txt::visibleWidth(txt::link("https://x", "y")) == 1);

    // descritor de dispositivo (teclado genérico)
    const uint8_t dev[18] = {18, 1, 0x00, 0x02, 0, 0, 0, 8, 0x6D, 0x04, 0x2B, 0xC5, 0x03, 0x12, 1, 2, 0, 1};
    usb::DeviceDescriptor d;
    assert(usb::parseDeviceDescriptor(dev, sizeof dev, d));
    assert(d.vid == 0x046D && d.pid == 0xC52B && d.bcdUSB == 0x0200);

    // configuração: 1 interface HID boot keyboard + 1 endpoint interrupt IN
    const uint8_t cfg[] = {9, 2, 34, 0, 1, 1, 0, 0xA0, 49,
                           9, 4, 0, 0, 1, 3, 1, 1, 0,
                           9, 0x21, 0x11, 0x01, 0, 1, 0x22, 59, 0,
                           7, 5, 0x81, 3, 8, 0, 8};
    usb::Configuration c;
    assert(usb::parseConfiguration(cfg, sizeof cfg, c));
    assert(c.interfaces.size() == 1 && c.interfaces[0].endpoints.size() == 1);
    assert(usb::isKeyboard(c.interfaces[0]) && c.interfaces[0].hidReportLen == 59);
    assert(usb::classSummary(d, &c) == "Teclado");

    // descritor malformado não pode estourar
    const uint8_t bad[] = {9, 2, 200, 0, 1, 1, 0, 0, 0, 0, 4};
    usb::Configuration c2;
    usb::parseConfiguration(bad, sizeof bad, c2);

    const uint8_t str[] = {10, 3, 'U', 0, 'S', 0, 'B', 0, 0xE7, 0x00};
    assert(usb::parseStringDescriptor(str, sizeof str) == "USBç");

    for (auto& l : usb::describeConfig(c, usb::Speed::Full, nullptr, false)) std::puts(l.c_str());

    if (argc > 1) {  // imprime o banner pra conferir no terminal
        for (auto& l : banner::build(120, banner::Tier::Full)) std::puts(l.c_str());
    }
    std::puts("OK");
}
