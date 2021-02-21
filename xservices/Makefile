src := $(wildcard src/*.c) $(wildcard src/**/*.c)

ifndef CERT_NAME
$(error CERT_NAME not set!)
endif

ifndef DMG_FILE
$(error DMG_FILE not set!)
endif

ifndef VOL_NAME
$(error VOL_NAME not set!)
endif

host_arch := arm64

CC := $(shell xcrun --sdk iphoneos -f clang) -isysroot $(shell xcrun --sdk iphoneos --show-sdk-path) -miphoneos-version-min=8.0 -framework Foundation -framework IOKit
CFLAGS := -Wall -pipe -Os
LDFLAGS := -Wl,-dead_strip
STRIP := $(shell xcrun --sdk iphoneos -f strip) -Sx
CODESIGN := $(shell xcrun --sdk iphoneos -f codesign) -f -s "$(CERT_NAME)"
LIPO := $(shell xcrun --sdk iphoneos -f lipo)

frida_version := 12.9.4
frida_os_arch := ios-$(host_arch)

all: bin/xservices.dylib bin/test_sleep

clean:
	$(RM) -r bin/ obj/

deploy: bin/xservices.dylib bin/test_sleep
	hdiutil attach $(DMG_FILE)
	cp bin/xservices.dylib bin/test_sleep /Volumes/$(VOL_NAME)/usr/lib/
	    hdiutil detach /Volumes/$(VOL_NAME)/

bin/xservices.dylib: obj/arm64/xservices.dylib obj/arm64e/xservices.dylib
	@mkdir -p $(@D)
	$(LIPO) $^ -create -output $@

bin/test_sleep: obj/arm64/test_sleep obj/arm64e/test_sleep
	@mkdir -p $(@D)
	$(LIPO) $^ -create -output $@

obj/%/xservices.dylib: $(src) | obj/%/frida-gum/.stamp
	@mkdir -p $(@D)
	$(CC) -arch $* -shared $(CFLAGS) -Iinclude -I$(@D)/frida-gum $^ -o $@ -L$(@D)/frida-gum -lfrida-gum -lresolv $(LDFLAGS)
	$(STRIP) $@
	$(CODESIGN) $@

obj/%/test_sleep: test_sleep.c
	@mkdir -p $(@D)
	$(CC) -arch $* $(CFLAGS) -Iinclude test_sleep.c -o $@ $(LDFLAGS)
	$(STRIP) $@
	$(CODESIGN) $@

obj/%/frida-gum/.stamp:
	@mkdir -p $(@D)
	@$(RM) $(@D)/*
	curl -Ls https://github.com/frida/frida/releases/download/$(frida_version)/frida-gum-devkit-$(frida_version)-ios-$*.tar.xz | xz -d | tar -C $(@D) -xf -
	@touch $@

.PHONY: all clean deploy
.PRECIOUS: obj/%/frida-gum/.stamp
