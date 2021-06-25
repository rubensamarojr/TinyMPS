#
# **************************************************************
# *                Simple C++ Makefile Template                *
# *                                                            *
# * Author: Arash Partow (2003)                                *
# * URL: http://www.partow.net/programming/makefile/index.html *
# *                                                            *
# * Copyright notice:                                          *
# * Free use of this C++ Makefile template is permitted under  *
# * the guidelines and in accordance with the the MIT License  *
# * http://www.opensource.org/licenses/MIT                     *
# *                                                            *
# **************************************************************
#

# Create executable
# make
# Create static library
# make static

CXX      := -c++
AR		 := ar
CXXFLAGS := -pedantic-errors -Wall -Werror -fopenmp
ARFLAGS  := ru
CXXDYNFLAGS := -fPIC -g #-pedantic -Wall -Wextra -ggdb3
LDFLAGS  := -Wl,-R -Llib -lstdc++ -lm -lvoro++
LDDYNFLAGS  := -Wl,-R -Llib -lstdc++ -lm -lvoro++ -shared

BUILD    := ./bin
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)
# Executable
TARGET   := voroGmsh
# Static library
STATLIB  := libvoroGmsh.a
DYNLIB	 := libvoroGmsh.dll
INCLUDE  := -Iinclude/
# List your source files to generate executable
SRC      := $(wildcard src/*.cpp)
#   $(wildcard src/module1/*.cpp) \
#   $(wildcard src/module2/*.cpp) \
#   $(wildcard src/*.cpp)         \

# List your source files to generate static library (remove main.cpp)
SRC_STAT := $(filter-out src/main.cpp, $(SRC))

OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)
DEPENDENCIES \
         := $(OBJECTS:.o=.d)

OBJECTS_LIB := $(SRC_STAT:%.cpp=$(OBJ_DIR)/%.o)
DEPENDENCIES \
         := $(OBJECTS_LIB:.o=.d)

all: build $(APP_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -MMD -o $@

$(APP_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(APP_DIR)/$(TARGET) $^ $(LDFLAGS)

static: build $(STATLIB)

$(STATLIB): $(OBJECTS_LIB)
	$(AR) $(ARFLAGS) $@ $^

shared: build $(DYNLIB)

$(DYNLIB): $(OBJECTS_LIB)
	$(CXX) $(LDDYNFLAGS) $(CXXDYNFLAGS) -o $(DYNLIB) $(OBJECTS_LIB)

-include $(DEPENDENCIES)

.PHONY: all build clean debug release info

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O2
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*.exe
	-@rm -rvf $(STATLIB)

info:
	@echo "[*] Application dir: ${APP_DIR}     "
	@echo "[*] Object dir:      ${OBJ_DIR}     "
	@echo "[*] Sources:         ${SRC}         "
	@echo "[*] Objects:         ${OBJECTS}     "
	@echo "[*] Objects Static:  ${OBJECTS_LIB}"
	@echo "[*] Dependencies:    ${DEPENDENCIES}"
