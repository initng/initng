# This Makefile is for compatibility purposes only

.PHONY: all install
all:
	jam

install:
	jam install

configure: configure.acr
	acr
