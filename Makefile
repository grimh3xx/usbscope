# usbscope — build nativo (Windows) e build de UI pra desenvolvimento (Linux/macOS).
#
#   make            -> compila o usbscope.exe (precisa de MinGW-w64 ou rodar no MSYS2)
#   make win        -> idem
#   make test       -> roda os testes das partes portáveis (qualquer SO)
#   make test-ui    -> compila a prévia da UI com dados simulados (qualquer SO)
#   make clean

CXX        ?= g++
CXXFLAGS   ?= -std=c++17 -O2 -Wall -Wextra
SRC_COMMON := src/textutil.cpp src/banner.cpp src/usbdesc.cpp src/usbids.cpp src/device.cpp src/app.cpp
WIN_SRC    := src/main.cpp src/term_win.cpp src/usbenum_win.cpp $(SRC_COMMON)
WIN_LIBS   := -lsetupapi -lcfgmgr32 -static -static-libgcc -static-libstdc++

# Cross-compile a partir do Linux: make CROSS=x86_64-w64-mingw32-
CROSS      ?=
WINCXX     := $(CROSS)$(CXX)

BIN        := usbscope.exe

.PHONY: all win test test-ui clean
all: win

win:
	$(WINCXX) $(CXXFLAGS) $(WIN_SRC) -o $(BIN) $(WIN_LIBS)
	@echo "ok -> $(BIN)"

test:
	$(CXX) -std=c++17 -Wall -Wextra -O1 tests/test_portable.cpp \
	  src/textutil.cpp src/banner.cpp src/usbdesc.cpp src/usbids.cpp -o /tmp/usbscope_test
	/tmp/usbscope_test

test-ui:
	$(CXX) -std=c++17 -w tests/render_ui.cpp tests/term_stub.cpp \
	  src/usbenum_mock.cpp $(SRC_COMMON) -o /tmp/usbscope_ui
	@echo "Rode: USBSCOPE_KEYS=\"DDE\" /tmp/usbscope_ui"

clean:
	rm -f $(BIN) /tmp/usbscope_test /tmp/usbscope_ui
