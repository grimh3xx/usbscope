#include "device.hpp"

#include "textutil.hpp"
#include "usbids.hpp"

namespace dev {

std::string Device::vidpid() const { return txt::hex(vid, 4) + ":" + txt::hex(pid, 4); }

std::string Device::bestVendor() const {
    if (!manufacturer.empty()) return manufacturer;
    if (!vendorName.empty()) return vendorName;
    return "VID " + txt::hex(vid, 4);
}

std::string Device::title() const {
    if (!product.empty()) return product;
    if (!productName.empty()) return productName;
    if (isHub()) return "Hub USB";
    std::string c = usb::className(desc.cls);
    return c.find("interface") != std::string::npos ? "Dispositivo " + vidpid() : c;
}

void resolveNames(Device& d) {
    d.vendorName = usbids::vendor(d.vid);
    d.productName = usbids::product(d.vid, d.pid);
}

}  // namespace dev
