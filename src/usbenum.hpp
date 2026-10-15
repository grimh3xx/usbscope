// Enumera dispositivos USB no Windows e lê os descritores de cada um.
#pragma once
#include <string>
#include <vector>

#include "device.hpp"

namespace usbenum {

// Varre todos os controladores/hubs e devolve os dispositivos conectados (hubs inclusos).
// Em erro grave, 'error' recebe a mensagem e o vetor volta vazio.
std::vector<dev::Device> enumerate(std::string& error);

}  // namespace usbenum
