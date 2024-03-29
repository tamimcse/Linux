ccflags-y := -O3 -fvisibility=hidden
ccflags-$(CONFIG_WIREGUARD_DEBUG) := -DDEBUG -g
ccflags-y += -Wframe-larger-than=8192
ccflags-y += -D'pr_fmt(fmt)=KBUILD_MODNAME ": " fmt'
wireguard-y := main.o noise.o device.o peer.o timers.o data.o send.o receive.o socket.o config.o hashtables.o routingtable.o ratelimiter.o cookie.o
wireguard-y += crypto/curve25519.o crypto/chacha20poly1305.o crypto/blake2s.o
ifeq ($(CONFIG_X86_64),y)
	wireguard-y += crypto/chacha20-ssse3-x86_64.o crypto/poly1305-sse2-x86_64.o
avx_supported := $(call as-instr,vpxor %xmm0$(comma)%xmm0$(comma)%xmm0,yes,no)
ifeq ($(avx_supported),yes)
	wireguard-y += crypto/blake2s-avx-x86_64.o crypto/curve25519-avx-x86_64.o
endif
avx2_supported := $(call as-instr,vpgatherdd %ymm0$(comma)(%eax$(comma)%ymm1$(comma)4)$(comma)%ymm2,yes,no)
ifeq ($(avx2_supported),yes)
	wireguard-y += crypto/chacha20-avx2-x86_64.o crypto/poly1305-avx2-x86_64.o
endif
endif

include $(src)/compat/Makefile.include

ifneq ($(KBUILD_EXTMOD),)
CONFIG_WIREGUARD := m
ifeq ($(CONFIG_WIREGUARD_PARALLEL),)
ifneq (,$(filter $(CONFIG_PADATA),y m))
ccflags-y += -DCONFIG_WIREGUARD_PARALLEL=y
endif
endif
ifneq ($(CONFIG_MODULES),)
ifeq ($(CONFIG_NETFILTER_XT_MATCH_HASHLIMIT),)
$(error "WireGuard requires CONFIG_NETFILTER_XT_MATCH_HASHLIMIT to be configured in your kernel. See https://www.wireguard.io/install/#kernel-requirements for more info")
endif
ifeq ($(CONFIG_PADATA),)
ifneq ($(CONFIG_SMP),)
$(warning "PEFORMANCE WARNING: WireGuard has enormous speed benefits when using CONFIG_PADATA on SMP systems. Please enable CONFIG_PADATA in your kernel configuration. See https://www.wireguard.io/install/#kernel-requirements for more info.")
endif
endif
endif
endif

obj-$(CONFIG_WIREGUARD) := wireguard.o
