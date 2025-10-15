#!/usr/bin/make -f

.PHONY: all build install clean

# Default installation directory for libfastjson (can be overridden by user)
LIBFASTJSON_DIR ?= /usr

# Prefer pkg-config if available
PKGCONFIG := $(shell which pkg-config 2>/dev/null)
ifeq ($(PKGCONFIG),)
  CFLAGS  += -I$(LIBFASTJSON_DIR)/include/libfastjson
  LDFLAGS += -L$(LIBFASTJSON_DIR)/lib -lfastjson
else
  CFLAGS  += $(shell pkg-config --cflags libfastjson)
  LDFLAGS += $(shell pkg-config --libs libfastjson)
endif

all: build

build:
	python setup.py $@

install:
	python setup.py $@ $(if $(DESTDIR),--root=$(DESTDIR))

clean:
	python setup.py $@ --all
	rm -rf *.egg-info
