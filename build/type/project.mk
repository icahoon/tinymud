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

__project_mk__ := 1

#------------------------------------------------------------------------
# Prerequisites

ifndef __build_mk__
    $(error project.mk must be included by build.mk)
endif
ifndef project
    $(error project not defined. It should be defined in the top level env file.)
endif


#------------------------------------------------------------------------
# Define common build procedure to be used by the various rules.

define build
@printf "$(nl)$(project_hilite)Building $@ -- `date`$(clear)\n"
@cd $($@) && $(MAKE) $(J) $(make_target)
@printf "$(project_hilite)Finished $@ -- `date`$(clear)\n"
endef

#------------------------------------------------------------------------
# Build rules

all: start

targets :=

.PHONY:: all build start $(targets)

start:
	@printf "$(nl)$(info_hilite)_______ Build _______$(clear)\n"
	@$(MAKE) build

build: $(targets)

$(rules)::
	$(MAKE) make_target=$(@) all

clean::
	@printf "$(nl)$(rm_hilite)Cleaning up $(_lib_)$(clear)\n"
	$(q)rm -rf $(_lib_)/*
	@printf "$(nl)$(rm_hilite)Cleaning up $(_bin_)$(clear)\n"
	$(q)rm -rf $(_bin_)/*


variables += targets

# vim:ts=8 noexpandtab
