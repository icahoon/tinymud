#################################################################
#
# Makefile for TinyMUD source code...June 6, 1990
#
#################################################################
#
# Whatever you put in for $(CC) must be able to grok ANSI C.
#

# GCC:
#CC=gcc
#OPTIM= -g -W -Wreturn-type -Wunused -Wcomment -Wwrite-strings

# Systems with 'cc' built from GCC (IBM RT, NeXT):
CC=clang
#CC=gcc
OPTIM=-g -Wall -Werror

# Dec 3100 C compiler
#CC=cc
#OPTIM= -g -Dconst=

# 
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
#

DEFS = -DGOD_PRIV -DCOMPRESS -DQUIET_WHISPER -DGENDER -DHOST_NAME \
       -DCONNECT_MESSAGES -DPLAYER_LIST -DDETACH -DROBOT_MODE \
       -DRECYCLE -DNOFAKES -DTIMESTAMPS

CFLAGS = $(OPTIM) $(DEFS)

# Everything needed to use db.c
DBFILES = db.c compress.c player_list.c stringutil.c
DBOFILES = db.o compress.o player_list.o stringutil.o

CFILES = create.c game.c help.c look.c match.c move.c player.c predicates.c \
	rob.c set.c speech.c utils.c wiz.c game.c \
	boolexp.c unparse.c main.c $(DBFILES)

OFILES = create.o game.o help.o look.o match.o move.o player.o predicates.o \
	rob.o set.o speech.o utils.o wiz.o boolexp.o \
	unparse.o $(DBOFILES)

OUTFILES = tinymud

all: tinymud

tinymud: main.o $(OFILES)
	$(CC) $(CFLAGS) -o tinymud main.o $(OFILES)

clean:
	-rm -f *.o a.out core gmon.out $(OUTFILES)
	-rm -rf *.dSYM

# DO NOT REMOVE THIS LINE OR CHANGE ANYTHING AFTER IT #
boolexp.o: boolexp.c copyright.h db.h match.h externs.h config.h interface.h
compress.o: compress.c
create.o: create.c copyright.h db.h config.h interface.h externs.h
db.o: db.c copyright.h db.h config.h
game.o: game.c copyright.h db.h config.h interface.h match.h externs.h
help.o: help.c copyright.h db.h config.h interface.h externs.h
look.o: look.c copyright.h db.h config.h interface.h match.h externs.h
main.o: main.c copyright.h db.h interface.h config.h
match.o: match.c copyright.h db.h config.h match.h
move.o: move.c copyright.h db.h config.h interface.h match.h externs.h
player.o: player.c copyright.h db.h config.h interface.h externs.h
player_list.o: player_list.c copyright.h db.h config.h interface.h externs.h
predicates.o: predicates.c copyright.h db.h interface.h config.h externs.h
rob.o: rob.c copyright.h db.h config.h interface.h match.h externs.h
set.o: set.c copyright.h db.h config.h match.h interface.h externs.h
speech.o: speech.c copyright.h db.h interface.h match.h config.h externs.h
stringutil.o: stringutil.c copyright.h externs.h
text.o: text.c mem.h text.h
unparse.o: unparse.c db.h externs.h config.h interface.h
utils.o: utils.c copyright.h db.h
wiz.o: wiz.c copyright.h db.h interface.h match.h externs.h
config.h: copyright.h
copyright.h:
db.h: copyright.h
externs.h: copyright.h db.h
interface.h: copyright.h db.h
match.h: copyright.h db.h
