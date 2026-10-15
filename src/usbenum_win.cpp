#include "usbenum.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// INITGUID faz os GUIDs de interface (usbiodef.h) e as DEVPKEY (devpkey.h) virarem
// definições de verdade neste .cpp, em vez de só declarações — senão o link falha.
#define INITGUID
#include <windows.h>
// depois de windows.h:
#include <initguid.h>
#include <cfgmgr32.h>
#include <devpkey.h>
#include <setupapi.h>
#include <usbioctl.h>
#include <usbiodef.h>

#include <algorithm>
#include <cstdlib>

#include "term.hpp"
#include "textutil.hpp"

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

namespace usbenum {
namespace {

// -------- helpers de handle / string --------
struct Handle {
    HANDLE h = INVALID_HANDLE_VALUE;
    Handle() = default;
    explicit Handle(HANDLE x) : h(x) {}
    ~Handle() { if (h != INVALID_HANDLE_VALUE) CloseHandle(h); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    bool ok() const { return h != INVALID_HANDLE_VALUE; }
    operator HANDLE() const { return h; }
};

std::string u8(const std::wstring& w) { return term::narrow(w); }

// Propriedade de registro de um devnode (DEVPKEY via CM) como string.
std::string deviceProp(DEVINST inst, const DEVPROPKEY& key) {
    DEVPROPTYPE type = 0;
    ULONG size = 0;
    CM_Get_DevNode_PropertyW(inst, &key, &type, nullptr, &size, 0);
    if (!size) return {};
    std::wstring buf(size / sizeof(wchar_t) + 1, L'\0');
    if (CM_Get_DevNode_PropertyW(inst, &key, &type, reinterpret_cast<PBYTE>(&buf[0]), &size, 0) != CR_SUCCESS)
        return {};
    buf.resize(wcslen(buf.c_str()));
    return u8(buf);
}

// Busca o devnode cujo caminho de interface bate com o driver key / instance, pra pegar nome e serviço.
void fillRegistryInfo(dev::Device& d, DEVINST inst) {
    d.product = d.product.empty() ? deviceProp(inst, DEVPKEY_NAME) : d.product;
    d.service = deviceProp(inst, DEVPKEY_Device_Service);
    std::string mfg = deviceProp(inst, DEVPKEY_Device_Manufacturer);
    if (!mfg.empty() && mfg != "(Standard USB Host Controller)") d.manufacturer = mfg;
    std::string loc = deviceProp(inst, DEVPKEY_Device_LocationInfo);
    if (!loc.empty()) d.locationPath = loc;
    // problema (código de erro do device manager)
    ULONG status = 0, problem = 0;
    if (CM_Get_DevNode_Status(&status, &problem, inst, 0) == CR_SUCCESS && (status & DN_HAS_PROBLEM))
        d.problem = "Device Manager: problema código " + std::to_string(problem);
}

// -------- IOCTLs de hub --------
std::wstring driverKeyName(HANDLE hub, ULONG port) {
    USB_NODE_CONNECTION_DRIVERKEY_NAME req{};
    req.ConnectionIndex = port;
    ULONG ret = 0;
    DeviceIoControl(hub, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, &req, sizeof req, &req, sizeof req, &ret, nullptr);
    ULONG bytes = req.ActualLength;
    if (bytes < sizeof req) return {};
    std::vector<char> buf(bytes);
    auto* r = reinterpret_cast<USB_NODE_CONNECTION_DRIVERKEY_NAME*>(buf.data());
    r->ConnectionIndex = port;
    if (!DeviceIoControl(hub, IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME, r, sizeof *r, r, bytes, &ret, nullptr))
        return {};
    return std::wstring(r->DriverKeyName);
}

// Lê um descritor (config/string) via IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION.
std::vector<uint8_t> getDescriptor(HANDLE hub, ULONG port, uint8_t type, uint8_t index, uint16_t lang,
                                   uint16_t wantLen) {
    size_t total = sizeof(USB_DESCRIPTOR_REQUEST) + wantLen;
    std::vector<uint8_t> buf(total, 0);
    auto* req = reinterpret_cast<USB_DESCRIPTOR_REQUEST*>(buf.data());
    req->ConnectionIndex = port;
    req->SetupPacket.bmRequest = 0x80;
    req->SetupPacket.bRequest = 0x06;  // GET_DESCRIPTOR
    req->SetupPacket.wValue = uint16_t((type << 8) | index);
    req->SetupPacket.wIndex = lang;
    req->SetupPacket.wLength = wantLen;
    ULONG ret = 0;
    if (!DeviceIoControl(hub, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, buf.data(), ULONG(total), buf.data(),
                         ULONG(total), &ret, nullptr))
        return {};
    if (ret <= sizeof(USB_DESCRIPTOR_REQUEST)) return {};
    size_t dataLen = ret - sizeof(USB_DESCRIPTOR_REQUEST);
    uint8_t* data = buf.data() + sizeof(USB_DESCRIPTOR_REQUEST);
    return std::vector<uint8_t>(data, data + dataLen);
}

usb::Speed mapSpeed(USB_DEVICE_SPEED s) {
    switch (s) {
        case UsbLowSpeed: return usb::Speed::Low;
        case UsbFullSpeed: return usb::Speed::Full;
        case UsbHighSpeed: return usb::Speed::High;
        case UsbSuperSpeed: return usb::Speed::Super;
        default: return usb::Speed::Unknown;
    }
}

void readStrings(dev::Device& d, HANDLE hub, ULONG port) {
    // lang IDs
    auto lang0 = getDescriptor(hub, port, 0x03, 0, 0, 255);
    uint16_t lang = (lang0.size() >= 4) ? uint16_t(lang0[2] | (lang0[3] << 8)) : 0x0409;
    auto want = {d.desc.iManufacturer, d.desc.iProduct, d.desc.iSerial, d.config.iConfiguration};
    std::vector<uint8_t> idx;
    for (const auto& i : d.config.interfaces)
        if (i.iInterface) idx.push_back(i.iInterface);
    for (uint8_t i : want)
        if (i) idx.push_back(i);
    std::sort(idx.begin(), idx.end());
    idx.erase(std::unique(idx.begin(), idx.end()), idx.end());
    for (uint8_t i : idx) {
        auto raw = getDescriptor(hub, port, 0x03, i, lang, 255);
        std::string s = usb::parseStringDescriptor(raw.data(), raw.size());
        if (!s.empty()) d.strings[i] = s;
    }
    if (d.desc.iManufacturer && d.strings.count(d.desc.iManufacturer))
        d.manufacturer = d.strings[d.desc.iManufacturer];
    if (d.desc.iProduct && d.strings.count(d.desc.iProduct)) d.product = d.strings[d.desc.iProduct];
    if (d.desc.iSerial && d.strings.count(d.desc.iSerial)) d.serial = d.strings[d.desc.iSerial];
}

// Lê um dispositivo numa porta do hub e adiciona a 'out' (se houver algo conectado).
void queryOneDevice(HANDLE hub, ULONG port, const std::string& hubPath, std::vector<dev::Device>& out) {
    USB_NODE_CONNECTION_INFORMATION_EX info{};
    info.ConnectionIndex = port;
    ULONG ret = 0;
    if (!DeviceIoControl(hub, IOCTL_USB_GET_NODE_CONNECTION_INFORMATION_EX, &info, sizeof info, &info, sizeof info,
                         &ret, nullptr))
        return;
    if (info.ConnectionStatus != DeviceConnected) return;  // porta vazia ou com falha

    dev::Device d;
    d.port = int(port);
    d.hubPath = hubPath;
    d.speed = mapSpeed(USB_DEVICE_SPEED(info.Speed));
    const uint8_t* dd = reinterpret_cast<const uint8_t*>(&info.DeviceDescriptor);
    d.rawDevice.assign(dd, dd + sizeof(USB_DEVICE_DESCRIPTOR));
    usb::parseDeviceDescriptor(d.rawDevice.data(), d.rawDevice.size(), d.desc);
    d.vid = d.desc.vid;
    d.pid = d.desc.pid;

    // config completa: primeiro 9 bytes pra saber wTotalLength, depois o bloco inteiro
    auto head = getDescriptor(hub, port, 0x02, 0, 0, 9);
    if (head.size() >= 4) {
        uint16_t total = uint16_t(head[2] | (head[3] << 8));
        auto full = getDescriptor(hub, port, 0x02, 0, 0, total);
        if (full.size() >= 9) {
            d.rawConfig = full;
            d.hasConfig = usb::parseConfiguration(full.data(), full.size(), d.config);
        }
    }
    readStrings(d, hub, port);

    // devnode -> nome amigável, serviço, location
    std::wstring key = driverKeyName(hub, port);
    if (!key.empty()) {
        // o instance id vem do SetupAPI; aqui guardamos a driver key e resolvemos via CM_Get_Device_ID
        d.instanceId = u8(key);  // provisório: será trocado se acharmos o id real
    }
    dev::resolveNames(d);
    out.push_back(std::move(d));
}

std::string externalHubPath(HANDLE parentHub, ULONG port) {
    USB_NODE_CONNECTION_NAME name{};
    name.ConnectionIndex = port;
    ULONG ret = 0;
    DeviceIoControl(parentHub, IOCTL_USB_GET_NODE_CONNECTION_NAME, &name, sizeof name, &name, sizeof name, &ret,
                    nullptr);
    if (name.ActualLength < sizeof name) return {};
    std::vector<char> buf(name.ActualLength);
    auto* n = reinterpret_cast<USB_NODE_CONNECTION_NAME*>(buf.data());
    n->ConnectionIndex = port;
    if (!DeviceIoControl(parentHub, IOCTL_USB_GET_NODE_CONNECTION_NAME, n, sizeof *n, n, name.ActualLength, &ret,
                         nullptr))
        return {};
    return "\\\\.\\" + u8(std::wstring(n->NodeName));
}

void walkHub(const std::string& hubPath, std::vector<dev::Device>& out, int depth) {
    if (depth > 7) return;  // evita loop em topologia estranha
    Handle hub(CreateFileA(hubPath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
    if (!hub.ok()) return;

    USB_NODE_INFORMATION node{};
    ULONG ret = 0;
    if (!DeviceIoControl(hub, IOCTL_USB_GET_NODE_INFORMATION, &node, sizeof node, &node, sizeof node, &ret, nullptr))
        return;
    ULONG ports = node.u.HubInformation.HubDescriptor.bNumberOfPorts;

    for (ULONG p = 1; p <= ports; ++p) {
        size_t before = out.size();
        queryOneDevice(hub, p, hubPath, out);
        // se o dispositivo recém-adicionado é um hub externo, desce nele também
        if (out.size() > before && out.back().isHub()) {
            std::string child = externalHubPath(hub, p);
            if (!child.empty()) walkHub(child, out, depth + 1);
        }
    }
}

// Liga cada dispositivo ao seu devnode via SetupAPI, casando pela driver key
// (que guardamos de cada porta do hub) — assim o match é 1:1 e não depende de VID/PID.
void matchDevNodes(std::vector<dev::Device>& devs) {
    HDEVINFO set = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_USB_DEVICE, nullptr, nullptr,
                                        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (set == INVALID_HANDLE_VALUE) return;
    SP_DEVINFO_DATA info{};
    info.cbSize = sizeof info;
    for (DWORD i = 0; SetupDiEnumDeviceInfo(set, i, &info); ++i) {
        std::string driverKey = deviceProp(info.DevInst, DEVPKEY_Device_Driver);  // "{guid}\\000N"
        wchar_t idbuf[512];
        std::string id = (CM_Get_Device_IDW(info.DevInst, idbuf, 512, 0) == CR_SUCCESS) ? u8(idbuf) : std::string();

        dev::Device* best = nullptr;
        for (auto& d : devs) {
            if (!driverKey.empty() && d.instanceId == driverKey) { best = &d; break; }  // match exato
        }
        if (!best) continue;
        best->instanceId = id;  // troca a driver key provisória pelo instance id de verdade
        best->driver = driverKey;
        fillRegistryInfo(*best, info.DevInst);
    }
    SetupDiDestroyDeviceInfoList(set);
}

}  // namespace

std::vector<dev::Device> enumerate(std::string& error) {
    std::vector<dev::Device> out;
    HDEVINFO set = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_USB_HOST_CONTROLLER, nullptr, nullptr,
                                        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (set == INVALID_HANDLE_VALUE) {
        error = "não consegui abrir a lista de controladores USB";
        return out;
    }
    SP_DEVICE_INTERFACE_DATA ifd{};
    ifd.cbSize = sizeof ifd;
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(set, nullptr, &GUID_DEVINTERFACE_USB_HOST_CONTROLLER, i, &ifd); ++i) {
        DWORD need = 0;
        SetupDiGetDeviceInterfaceDetailW(set, &ifd, nullptr, 0, &need, nullptr);
        if (!need) continue;
        std::vector<uint8_t> buf(need);
        auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buf.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(set, &ifd, detail, need, &need, nullptr)) continue;
        std::string ctrlPath = u8(detail->DevicePath);

        // hub raiz do controlador
        Handle ctrl(CreateFileA(ctrlPath.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
        if (!ctrl.ok()) continue;
        USB_ROOT_HUB_NAME rootName{};
        ULONG ret = 0;
        DeviceIoControl(ctrl, IOCTL_USB_GET_ROOT_HUB_NAME, nullptr, 0, &rootName, sizeof rootName, &ret, nullptr);
        if (rootName.ActualLength < sizeof rootName) continue;
        std::vector<char> nbuf(rootName.ActualLength);
        auto* rn = reinterpret_cast<USB_ROOT_HUB_NAME*>(nbuf.data());
        if (!DeviceIoControl(ctrl, IOCTL_USB_GET_ROOT_HUB_NAME, nullptr, 0, rn, rootName.ActualLength, &ret, nullptr))
            continue;
        std::string rootPath = "\\\\.\\" + u8(std::wstring(rn->RootHubName));
        walkHub(rootPath, out, 0);
    }
    SetupDiDestroyDeviceInfoList(set);

    matchDevNodes(out);
    for (auto& d : out) {
        if (d.instanceId.find("USB\\") == std::string::npos) d.instanceId.clear();  // era só a driver key
        // nome final: descritor de produto > nome do devnode > classe
        if (d.product.empty() && !d.productName.empty()) d.product = d.productName;
    }
    if (out.empty() && error.empty()) error = "nenhum dispositivo USB encontrado";
    return out;
}

}  // namespace usbenum
