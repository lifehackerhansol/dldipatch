# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

dldipatch: dldipatch.c
	gcc -Wall -Wextra -Wno-unused-result -std=gnu11 -O3 -o $@ $<

.PHONY: clean

clean:
	rm -rf dldipatch
