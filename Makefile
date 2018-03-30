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

export root := $(shell until [ -f build/build.mk ]; do cd ..; done; pwd)

type := binary
cc   := clang

# To log failed commands (HUH's) to stderr, include -DLOG_FAILED_COMMANDS
# To restrict object-creating commands to users with the BUILDER bit,
#   include -DRESTRICTED_BUILDING
# To log all commands, include -DLOG_COMMANDS
# To force fork_and_dump() to use vfork() instead of fork(), include 
#   -DUSE_VFORK.
# To force grow_database() to be clever about its memory management,
#   include -DDB_DOUBLING.  Use this only if your realloc does not allocate
#   in powers of 2 (if you already have a clever realloc, this option will
#   only cost you extra space).
# By default, db.c initially allocates enough space for 10000 objects, then
#   grows the space if needed.  To change this value, include
#   -DDB_INITIAL_SIZE=xxxx where xxxx is the new value (minimum 1).
# To include code for keeping track of the number of blocks allocated,
#   include -DTEST_MALLOC.
# To include code which attempts to compress string data, include -DCOMPRESS.
# To eliminate the message third parties see when a player whispers, include
#   -DQUIET_WHISPER.
# To include Stephen White's gender flags and pronoun substitution code, 
#   include -DGENDER.
# To give set (!)WIZARD and extended WHO privs only to id #1,
#   include -DGOD_PRIV.  When this option is set, two other options
#   become meanigful
#	-DGOD_MODE		Restricts host names some commands to #1
#	-DGOD_ONLY_PCREATE	Restricts @pcreate to player #1
# To have logs and WHO use hostnames instead of addresses, include
#   -DHOST_NAME.
# To have messages for connect and disconnect, include -DCONNECT_MESSAGES.
# To use a hashed player list for player name lookups, 
#   include -DPLAYER_LIST.
# To disable login-time creation of players, include -DREGISTRATION.
#    see GOD_ONLY_PCREATE above.
# To cause netmud to detach itself from the terminal on startup, include
#   -DDETACH.  The log file appears on LOG_FILE, set in config.h.
# To add the @count & @recycle command, include -DRECYCLE
# To disable core dump on errors, include -DNODUMPCORE
# To add the ROBOT flag (allowing robots to be excluded from some rooms
#   at each player's request), include -DROBOT_MODE
# To prevent users from using confusing names
#   (currently A, An, The, You, Your, Going, Huh?), include -DNOFAKES
# To include code for marking all things with a timestamp/usecnt,
#   include -DTIMESTAMPS
extra_cflags := -DGOD_PRIV -DCOMPRESS -DQUIET_WHISPER -DGENDER -DHOST_NAME \
                -DCONNECT_MESSAGES -DPLAYER_LIST -DDETACH -DROBOT_MODE \
                -DRECYCLE -DNOFAKES -DTIMESTAMPS


include $(root)/build/build.mk
