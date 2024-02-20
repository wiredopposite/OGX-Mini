# Copyright (c) 2022 Nicolai Electronics
# SPDX-License-Identifier: MIT

INSTALL_PREFIX := $PWD
BUILD_DIR := build
GENERATED_DIR := generated

.PHONY: all firmware flash clean install_rules $(BUILD_DIR) format

all: build flash
	@echo "All tasks completed"

build:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(GENERATED_DIR)
	cd $(BUILD_DIR); cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX -DCMAKE_BUILD_TYPE=Release ..
	$(MAKE) -C $(BUILD_DIR) --no-print-directory all

debug:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(GENERATED_DIR)
	cd $(BUILD_DIR); cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX -DCMAKE_BUILD_TYPE=Debug ..
	$(MAKE) -C $(BUILD_DIR) --no-print-directory all

flash:
	picotool load $(BUILD_DIR)/i2c_adapter.bin
	picotool reboot

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(GENERATED_DIR)

install_rules:
	cp tools/99-pico.rules /etc/udev/rules.d/
	@echo "reload rules with:"
	@echo "\tudevadm control --reload-rules"
	@echo "\tudevadm trigger"

format:
	find . -iname '*.h' -o -iname '*.c' -o -iname '*.cpp' | grep -v '$(BUILD_DIR)' | xargs clang-format -i
