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

__common_mk__ := 1

#------------------------------------------------------------------------
# Initialize info variables
export SHELL=/bin/bash

variables :=
options   :=

export _bin_ := $(_root_)/bin
export _lib_ := $(_root_)/lib

ifndef name
  export name := $(notdir ${PWD})
endif

variables += _root_ _build_ name

#------------------------------------------------------------------------
# OS definitions
ostype := $(shell uname -s)
ifeq ($(ostype), Darwin)
  hwplatform := $(shell uname -p)
else
  hwplatform := $(shell uname -i)
endif
arch := $(shell uname -m)
variables += ostype hwplatform arch

#------------------------------------------------------------------------
# color definitions

black       := 30
red         := 31
green       := 32
yellow      := 33
blue        := 34
magenta     := 35
cyan        := 36
white       := 37

project_hilite  := \\033[1;$(yellow)m
dbg_hilite      := \\033[1;$(green)m
obj_hilite      := \\033[1;$(green)m
bin_hilite      := \\033[1;$(blue)m
rm_hilite       := \\033[1;$(red)m
docs_hilite     := \\033[1;$(yellow)m
inst_hilite     := \\033[1;$(green)m
info_hilite     := \\033[1;$(cyan)m
test_hilite     := \\033[1;$(blue)m
warn_hilite     := \\033[1;$(red)m
clear           := \\033[0;$(default)m

options += color
export color_desc := Provide color hints to build output [ "yes" | "no" (default) ], e.g. color=yes

#------------------------------------------------------------------------
# Turn off build details
ifeq ($(quiet),yes)
  export quiet := 1
  q := @
  nl :=
  export MAKEFLAGS := $(MAKEFLAGS) --quiet
else
  export quiet := 0
  q :=
  nl := \n
endif

options += quiet
export quiet_desc := Suppress detailed build command [ "yes" | "no" (default) ], e.g. quiet=yes

#------------------------------------------------------------------------
# Parallel build flag
ifndef j
  j := 4
endif

export J := -j$(j)
export MAKEFLAGS := $(MAKEFLAGS) --no-print-directory

variables += J MAKE MAKEFLAGS

options += j
export j_desc := Number of build jobs that can run simultaneously. [ N (default is 4) ], e.g. j=16

#------------------------------------------------------------------------
# start rules

export all_desc       := Build source files
export clean_desc     := Remove derived objects for source files.
export test_desc      := Run available unit tests
export debug_desc     := Debug information for the local makefile
export info_desc      := This information

rules := clean test
all_rules := all $(rules)

.PHONY:: $(all_rules) info debug

# vim:ts=8 noexpandtab
