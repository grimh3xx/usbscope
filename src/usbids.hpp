// Nomes de fabricante/produto: tabela embutida + arquivo usb.ids (linux-usb.org), se existir.
#pragma once
#include <cstdint>
#include <string>

namespace usbids {

// Carrega usb.ids do caminho informado. Retorna quantos fabricantes foram lidos (0 = falhou).
size_t load(const std::string& utf8Path);
bool loaded();

std::string vendor(uint16_t vid);                 // "" se desconhecido
std::string product(uint16_t vid, uint16_t pid);  // "" se desconhecido

}  // namespace usbids
