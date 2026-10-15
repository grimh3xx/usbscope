// Modelo de um dispositivo USB enumerado — portável, sem WinAPI.
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "usbdesc.hpp"

namespace dev {

struct Device {
    // identidade
    uint16_t vid = 0, pid = 0;
    usb::Speed speed = usb::Speed::Unknown;
    int port = 0;                 // porta no hub
    std::string hubPath;          // qual hub/controlador
    std::string instanceId;       // "USB\\VID_046D&PID_C52B\\..."
    std::string locationPath;     // "Port_#0003.Hub_#0004" quando disponível

    // strings (do Windows e/ou dos descritores)
    std::string manufacturer, product, serial;
    std::string vendorName, productName;  // da base usb.ids / embutida
    std::string driver, service;

    // descritores crus
    std::vector<uint8_t> rawDevice;
    std::vector<uint8_t> rawConfig;
    std::map<uint8_t, std::string> strings;  // índice -> texto já decodificado

    // já parseado
    usb::DeviceDescriptor desc;
    bool hasConfig = false;
    usb::Configuration config;

    std::string problem;  // texto se o dispositivo estiver com erro/desconectando

    std::string title() const;       // "Logitech USB Receiver"
    std::string vidpid() const;      // "046D:C52B"
    std::string bestVendor() const;  // nome de fabricante mais confiável
    bool isHub() const { return desc.cls == 0x09; }
};

// Resolve nomes de fabricante/produto a partir da base de IDs (preenche vendorName/productName).
void resolveNames(Device& d);

}  // namespace dev
