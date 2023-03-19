# SPDX-License-Identifier: MIT
#
# Copyright (c) 2023, Antonio Niño Díaz

dldipatch: dldipatch.c
	gcc -Wall -Wextra -Wno-sign-compare -Wno-unused-result -std=gnu11 -O3 -o $@ $<

.PHONY: clean

clean:
	rm -rf dldipatch
