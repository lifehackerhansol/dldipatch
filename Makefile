# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023-2024

HOSTCC		?= gcc

dldipatch: dldipatch.c
	$(HOSTCC) -Wall -Wextra -Wno-unused-result -std=gnu11 -O3 -o $@ $<

.PHONY: clean

clean:
	rm -rf dldipatch
