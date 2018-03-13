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
CC=gcc
#CC=clang
OPTIM=-g

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
# To use Tinker instead of Wizard, Bobble instead of Toad, and
#    donate instead of sacrifice, include -DTINKER
# To prevent users from using confusing names
#   (currently A, An, The, You, Your, Going, Huh?), include -DNOFAKES
# To include code for marking all things with a timestamp/usecnt,
#   include -DTIMESTAMPS
#
# To Use Islandia values in config.h, include -DISLANDIA
# To Use TinyHELL values in config.h, include -DTINYHELL

#DEFS= -DGOD_PRIV -DCOMPRESS -DQUIET_WHISPER -DGENDER -DHOST_NAME \
#      -DCONNECT_MESSAGES -DPLAYER_LIST -DDETACH -DREGISTRATION \
#      -DGOD_ONLY_PCREATE -DROBOT_MODE -DRECYCLE -DNOFAKES \
#      -DTINYHELL

DEFS= -DGOD_PRIV -DCOMPRESS -DQUIET_WHISPER -DGENDER -DHOST_NAME \
      -DCONNECT_MESSAGES -DPLAYER_LIST -DDETACH -DROBOT_MODE \
      -DRECYCLE -DTINKER -DNOFAKES -DTIMESTAMPS -DISLANDIA

CFLAGS= $(OPTIM) $(DEFS) #-Wall -Werror

# Everything needed to use db.c
DBFILES= db.c compress.c player_list.c stringutil.c
DBOFILES= db.o compress.o player_list.o stringutil.o

# Everything except interface.c --- allows for multiple interfaces
CFILES= create.c game.c help.c look.c match.c move.c player.c predicates.c \
	rob.c set.c speech.c utils.c wiz.c game.c \
	boolexp.c unparse.c conc.c oldinterface.c $(DBFILES)

# .o versions of above
OFILES= create.o game.o help.o look.o match.o move.o player.o predicates.o \
	rob.o set.o speech.o utils.o wiz.o boolexp.o \
	unparse.o $(DBOFILES)

# Files in the standard distribution
DISTFILES= $(CFILES) config.h db.h externs.h interface.h match.h \
	interface.c sanity-check.c extract.c dump.c decompress.c \
	help.txt small.db minimal.db restart-cmu do_gripes \
	restart-day restart-night \
 	README small.db.README \
	CHANGES Makefile copyright.h

OUTFILES= netmud netmud.conc concentrate dump paths sanity-check \
	  extract decompress TAGS conc

BINDIR= /clients/Islandia/bin
LIBDIR= /clients/Islandia/lib

BINS= $(BINDIR)/netmud.conc $(BINDIR)/extract $(BINDIR)/sanity-check \
      $(BINDIR)/dump $(BINDIR)/decompress $(BINDIR)/concentrate \
      $(BINDIR)/netmud

# paths is likely to remain broken
all: extract sanity-check dump decompress netmud netmud.conc concentrate

TAGS: *.c *.h
	etags *.c *.h

netmud.conc: $P interface.o $(OFILES)
	$(CC) $(CFLAGS) -o netmud.conc interface.o $(OFILES)

netmud: $P oldinterface.o $(OFILES)
	$(CC) $(CFLAGS) -o netmud oldinterface.o $(OFILES)

concentrate: $P conc.c config.h
	$(CC) $(CFLAGS) -o concentrate conc.c

dump: $P dump.o unparse.o $(DBOFILES)
	-rm -f dump
	$(CC) $(CFLAGS) -o dump dump.o unparse.o $(DBOFILES) 

sanity-check: $P sanity-check.o utils.o $(DBOFILES) 
	-rm -f sanity-check
	$(CC) $(CFLAGS) -o sanity-check sanity-check.o utils.o $(DBOFILES)

extract: $P extract.o utils.o $(DBOFILES) 
	-rm -f extract
	$(CC) $(CFLAGS) -o extract extract.o utils.o $(DBOFILES)

paths: $P paths.o unparse.o $(DBOFILES)
	-rm -f paths
	$(CC) $(CFLAGS) -o paths paths.o unparse.o $(DBOFILES)

decompress: $P decompress.o compress.o
	-rm -f decompress
	$(CC) $(CFLAGS) -o decompress decompress.o compress.o

clean:
	-rm -f *.o a.out core gmon.out $(OUTFILES)

dist.tar.Z: $(DISTFILES)
	tar cvf - $(DISTFILES) | compress -c > dist.tar.Z.NEW
	mv dist.tar.Z.NEW dist.tar.Z

install: $(BINS)

$(BINDIR)/extract: extract
	cp extract $(BINDIR)/extract
$(BINDIR)/sanity-check: sanity-check
	cp sanity-check $(BINDIR)/sanity-check
$(BINDIR)/concentrate: concentrate
	cp concentrate $(BINDIR)/concentrate
$(BINDIR)/netmud.conc: netmud.conc
	cp netmud.conc $(BINDIR)/netmud.conc
$(BINDIR)/netmud: netmud
	cp netmud $(BINDIR)/netmud
$(BINDIR)/dump: dump
	cp dump $(BINDIR)/dump
$(BINDIR)/decompress: decompress
	cp decompress $(BINDIR)/decompress

# DO NOT REMOVE THIS LINE OR CHANGE ANYTHING AFTER IT #
boolexp.o: boolexp.c copyright.h db.h match.h externs.h config.h interface.h
compress.o: compress.c
create.o: create.c copyright.h db.h config.h interface.h externs.h
db.o: db.c copyright.h db.h config.h
decompress.o: decompress.c
dump.o: dump.c copyright.h db.h
extract.o: extract.c copyright.h db.h
fix.o: fix.c copyright.h db.h config.h
game.o: game.c copyright.h db.h config.h interface.h match.h externs.h
help.o: help.c copyright.h db.h config.h interface.h externs.h
interface.o: interface.c copyright.h db.h interface.h config.h
look.o: look.c copyright.h db.h config.h interface.h match.h externs.h
match.o: match.c copyright.h db.h config.h match.h
move.o: move.c copyright.h db.h config.h interface.h match.h externs.h
oldinterface.o: oldinterface.c copyright.h db.h interface.h config.h
paths.o: paths.c copyright.h db.h config.h
player.o: player.c copyright.h db.h config.h interface.h externs.h
player_list.o: player_list.c copyright.h db.h config.h interface.h externs.h
predicates.o: predicates.c copyright.h db.h interface.h config.h externs.h
rob.o: rob.c copyright.h db.h config.h interface.h match.h externs.h
sanity-check.o: sanity-check.c copyright.h db.h config.h
set.o: set.c copyright.h db.h config.h match.h interface.h externs.h
speech.o: speech.c copyright.h db.h interface.h match.h config.h externs.h
stringutil.o: stringutil.c copyright.h externs.h
unparse.o: unparse.c db.h externs.h config.h interface.h
utils.o: utils.c copyright.h db.h
wiz.o: wiz.c copyright.h db.h interface.h match.h externs.h
config.h: copyright.h
copyright.h:
db.h: copyright.h
externs.h: copyright.h db.h
interface.h: copyright.h db.h
match.h: copyright.h db.h
