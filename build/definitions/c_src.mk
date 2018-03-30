# BSD 2-Clause License
#
# Copyright (c) 2018, Ian Cahoon
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

__defn_c_src_mk__ := 1

c_src   := $(wildcard *.c)
cpp_src := $(wildcard *.cpp)

src     := $(cpp_src) $(c_src)
src     := $(strip $(src))

_obj_ := .obj
obj     := $(patsubst %.c,$(_obj_)/%.o,$(c_src)) \
           $(patsubst %.cpp,$(_obj_)/%.o,$(cpp_src))
obj     := $(strip $(obj))

dep     := $(patsubst %.c,$(_obj_)/%.d,$(c_src)) \
           $(patsubst %.cpp,$(_obj_)/%.d,$(cpp_src))
dep     := $(strip $(dep))

ifeq ($(debug), yes)
  cflags := -g -DDEBUG=1
else
  ifeq ($(debug_symbols), no)
    cflags := -O -DNDEBUG
  else
    cflags := -g -O -DNDEBUG
  endif
endif

warnings := -Wall -Wuninitialized -Werror
#warnings += -pedantic

cflags   += -fPIC $(warnings) -I$(_root_) -I.
cxxflags := $(cflags)

ifdef unit_test
  cflags   += -DUNIT_TEST
  cxxflags += -DUNIT_TEST
endif
ifdef ostype
  cflags   += -D$(ostype)
  cxxflags += -D$(ostype)
endif

cflags += $(extra_cflags)
cxxflags += $(extra_cxxflags)

# Compiler
ifndef cc
  cc  := gcc
endif
ifndef cxx
  cxx := g++
endif

# Dependencies
mkdep_c    := $(cc)
mkdep_cxx  := $(cxx)
depflags   := -MM

# Archiver
ar      := ar
arflags := crsu

# Linker
ifndef ld
	ld    := $(cc)
endif
ldflags := $(extra_ldflags)

objcopy := objcopy
strip   := strip

# Formatter
astyle       := $(shell which astyle)
astyle_flags := --style=google --unpad-paren --indent=force-tab --break-one-line-headers --add-braces --max-code-length=132 --align-pointer=name --align-reference=name --pad-header --pad-comma --suffix=none

variables += cc cxx ar arflags ld ldflags astyle astyle_flags cflags cxxflags src obj dep
