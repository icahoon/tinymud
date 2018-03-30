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

__rules_c_src_mk__ := 1

.SECONDARY:: $(obj) $(dep)

$(_obj_)/%.o: %.c
	@printf "$(nl)$(obj_hilite)Compiling $(subst .o,.c,$(@F))$(clear)\n"
ifneq ($(quiet), yes)
	$(q)$(astyle) $(astyle_flags) $< 2>&1
else
	$(q)$(astyle) $(astyle_flags) $< >/dev/null 2>&1
endif
	$(q)$(cc) $(cflags) -c $< -o $@
ifneq ($(build_deps), no)
	@$(SHELL) -ec '$(mkdep_c) $(depflags) $(cflags) $< 2>/dev/null \
        | sed -e "s,^$(*F).o[ :]*,$@: ," > $(@D)/$(*F).d'
endif

$(_obj_)/%.o: %.cpp
	@printf "$(nl)$(obj_hilite)Compiling $(subst .o,.cpp,$(@F))$(clear)\n"
ifneq ($(quiet), yes)
	$(q)$(astyle) $(astyle_flags) $< 2>&1
else
	$(q)$(astyle) $(astyle_flags) $< >/dev/null 2>&1
endif
	$(q)$(cxx) $(cxxflags) -c $< -o $@
ifneq ($(build_deps), no)
	@$(SHELL) -ec '$(mkdep_cxx) $(depflags) $(cxxflags) $< 2>/dev/null \
        | sed -e "s,^$(*F).o[ :]*,$@: ," > $(@D)/$(*F).d'
endif

$(_obj_)/%.i: %.c
	@printf "$(nl)$(obj_hilite)Pre-compiling $(subst .o,.cpp,$(@F))$(clear)\n"
	$(q)$(cc) -E $(cflags) -c $< -o $@

$(_obj_)/%.i: %.cpp
	@printf "$(nl)$(obj_hilite)Pre-compiling $(subst .o,.cpp,$(@F))$(clear)\n"
	$(q)$(cxx) -E $(cxxflags) -c $< -o $@

obj_clean:
	@printf "$(nl)$(rm_hilite)Cleaning up objects$(clear)\n"
	$(q)rm -f $(_obj_)/*.[oid]

# Convenience rules
%.o: %.c
	@printf "$(nl)$(obj_hilite)Compiling $(subst .o,.c,$(@F))$(clear)\n"
ifneq ($(quiet), yes)
	$(q)$(astyle) $(astyle_flags) $< 2>&1
else
	$(q)$(astyle) $(astyle_flags) $< >/dev/null 2>&1
endif
	$(q)$(cc) $(cflags) -c $< -o $(_obj_)/$@
ifneq ($(build_deps), no)
	@$(SHELL) -ec '$(mkdep_c) $(depflags) $(ccflags) $< 2>/dev/null \
        | sed -e "s,^$(*F).o[ :]*,$(_obj_)/$@: ," > $(_obj_)/$(*F).d'
endif

%.o: %.cpp
	@printf "$(nl)$(obj_hilite)Compiling $(subst .o,.cpp,$(@F))$(clear)\n"
ifneq ($(quiet), yes)
	$(q)$(astyle) $(astyle_flags) $< 2>&1
else
	$(q)$(astyle) $(astyle_flags) $< >/dev/null 2>&1
endif
	$(q)$(cxx) $(cxxflags) -c $< -o $(_obj_)/$@
ifneq ($(build_deps), no)
	@$(SHELL) -ec '$(mkdep_cxx) $(depflags) $(cxxflags) $< 2>/dev/null \
        | sed -e "s,^$(*F).o[ :]*,$(_obj_)/$@: ," > $(_obj_)/$(*F).d'
endif

%.i: %.c
	@printf "$(nl)$(obj_hilite)Pre-compiling $(subst .o,.cpp,$(@F))$(clear)\n"
	$(q)$(cc) -E $(cflags) -c $< -o $(_obj_)/$@

%.i: %.cpp
	@printf "$(nl)$(obj_hilite)Pre-compiling $(subst .o,.cpp,$(@F))$(clear)\n"
	$(q)$(cxx) -E $(cxxflags) -c $< -o $(_obj_)/$@

# Include the dependencies
#
-include $(_obj_)/*.d
