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

__rules_debug_mk__ := 1

variable_values := $(foreach var,$(variables),[$(var)|$(value $(var))])

debug::
	@printf "\n$(info_hilite)Info for $${PWD#*$(project)/}$(clear)\n"
	@all_vars='$(variable_values)'; \
	while [ "$${all_vars}" != "" ]; do \
	    var=$${all_vars%%]*}; \
	    all_vars=$${all_vars#*]}; \
	    var=$${var##*[}; \
	    name=$${var%%|*}; value=$${var##*|}; \
	    printf "  %b%-20s%b\t%s\n" \
		   "$(dbg_hilite)" "$${name}" "$(clear)" "$${value}"; \
	done
