# Subgraph Team Matching — build
#
# Compila a raylib vendorizada (third_party/) e a interface. Só precisa de um compilador C/C++ e do make.
#   make          compila tudo
#   make run      abre a interface
#   make clean    apaga os artefatos de build
#
# No Windows, use o make do w64devkit (veja o README).

# ---------------------------------------------------------------------------
# Detecção do sistema operacional
# ---------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    PLATFORM := windows
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        PLATFORM := macos
    else
        PLATFORM := linux
    endif
endif

# ---------------------------------------------------------------------------
# Pastas e ferramentas
# ---------------------------------------------------------------------------
CC  ?= cc
CXX ?= g++
AR  ?= ar

BUILD_DIR  := build
BIN_DIR    := bin
RAYLIB_SRC := third_party/raylib/src
RAYGUI_DIR := third_party/raygui

# ---------------------------------------------------------------------------
# raylib (código de terceiros: compilado sem avisos, uma única vez)
# ---------------------------------------------------------------------------
# raudio fica de fora: o projeto não usa áudio.
RAYLIB_MODULES := rcore rshapes rtextures rtext rmodels utils rglfw
RAYLIB_OBJS    := $(addprefix $(BUILD_DIR)/raylib/,$(addsuffix .o,$(RAYLIB_MODULES)))
RAYLIB_LIB     := $(BUILD_DIR)/libraylib.a

RAYLIB_CFLAGS := -std=c99 -O2 -w -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 \
                 -I$(RAYLIB_SRC) -I$(RAYLIB_SRC)/external/glfw/include

# ---------------------------------------------------------------------------
# Código do projeto
# ---------------------------------------------------------------------------
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -MMD -MP -Isrc -I$(RAYLIB_SRC) -I$(RAYGUI_DIR)
CFLAGS   := -std=c99 -O2 -MMD -MP -I$(RAYLIB_SRC) -I$(RAYGUI_DIR)

GUI_CXX_SRCS := src/gui/main.cpp
GUI_C_SRCS   := src/gui/raygui_impl.c
GUI_OBJS     := $(addprefix $(BUILD_DIR)/,$(GUI_CXX_SRCS:.cpp=.o) $(GUI_C_SRCS:.c=.o))

# ---------------------------------------------------------------------------
# Ajustes por plataforma
# ---------------------------------------------------------------------------
ifeq ($(PLATFORM),windows)
    EXE      := .exe
    # -static evita depender das DLLs do compilador ao rodar o .exe em outra máquina
    LDFLAGS  := -static
    LDLIBS   := -lopengl32 -lgdi32 -lwinmm
endif
ifeq ($(PLATFORM),linux)
    EXE      :=
    # O GLFW precisa saber qual sistema de janelas usar no Linux (X11 funciona também sob Wayland)
    RAYLIB_CFLAGS += -D_GNU_SOURCE -D_GLFW_X11
    LDLIBS   := -lGL -lm -lpthread -ldl -lrt -lX11
endif
ifeq ($(PLATFORM),macos)
    EXE      :=
    LDLIBS   := -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
endif

GUI_BIN := $(BIN_DIR)/teams_gui$(EXE)

# ---------------------------------------------------------------------------
# Alvos
# ---------------------------------------------------------------------------
.PHONY: all run clean

all: $(GUI_BIN)

run: $(GUI_BIN)
	./$(GUI_BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

$(GUI_BIN): $(GUI_OBJS) $(RAYLIB_LIB)
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -o $@ $(GUI_OBJS) $(RAYLIB_LIB) $(LDLIBS)

$(RAYLIB_LIB): $(RAYLIB_OBJS)
	$(AR) rcs $@ $^

# No macOS, o GLFW (rglfw.c) precisa ser compilado como Objective-C
ifeq ($(PLATFORM),macos)
$(BUILD_DIR)/raylib/rglfw.o: $(RAYLIB_SRC)/rglfw.c
	@mkdir -p $(@D)
	$(CC) -x objective-c $(RAYLIB_CFLAGS) -c $< -o $@
endif

$(BUILD_DIR)/raylib/%.o: $(RAYLIB_SRC)/%.c
	@mkdir -p $(@D)
	$(CC) $(RAYLIB_CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

# A implementação da raygui é código de terceiros: sem avisos
$(BUILD_DIR)/src/gui/raygui_impl.o: CFLAGS += -w

-include $(GUI_OBJS:.o=.d)
