#include "copyright.h"

/* Wizard-only commands */

#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "db.h"
#include "config.h"
#include "interface.h"
#include "match.h"
#include "externs.h"

void do_teleport(dbref player, const char *arg1, const char *arg2)
{
    dbref victim;
    dbref destination;
    const char *to;

#ifdef RESTRICTED_TELEPORT
    if(!Wizard(player)) {
#ifndef TINKER
        notify(player, "Only a Wizard may teleport at will.");
#else /* TINKER */
        notify(player, "Only a Tinker may teleport at will.");
#endif /* TINKER */
        return;
    }
#endif /* RESTRICTED_TELEPORT */

    /* get victim, destination */
    if(*arg2 == '\0') {
        victim = player;
        to = arg1;
    } else {
        init_match(player, arg1, NOTYPE);
        match_neighbor();
        match_possession();
        match_me();
        match_absolute();
        match_player();

        if((victim = noisy_match_result()) == NOTHING) {
            return;
        }
        to = arg2;
    }

    /* get destination */
    init_match(player, to, TYPE_PLAYER);
    match_here();
    match_absolute();
    if(Wizard(player)) {
        match_neighbor();
        match_me();
        match_player();
    }

    switch(destination = match_result()) {
      case NOTHING:
        notify(player, "Send it where?");
        break;
      case AMBIGUOUS:
        notify(player, "I don't know which destination you mean!");
        break;
      default:
        /* check victim, destination types, teleport if ok */
        if(Typeof(destination) == TYPE_EXIT
           || Typeof(destination) == TYPE_THING
           || Typeof(victim) == TYPE_EXIT
           || Typeof(victim) == TYPE_ROOM
           || (Typeof(victim) == TYPE_PLAYER
               && Typeof(destination) != TYPE_ROOM)) {
            notify(player, "Bad destination.");
#ifndef RESTRICTED_TELEPORT
        } else if(!Wizard(player)
                  && !(Typeof(victim) == TYPE_THING
                       && Typeof(destination) == TYPE_ROOM
                       && (controls(player, victim)
                           || (Typeof(db[victim].location) == TYPE_ROOM
                               && controls(player, db[victim].location)))
                       && (can_link_to(player, TYPE_PLAYER, destination)))) {
            notify(player, "Permission denied.");
#endif /* RESTRICTED_TELEPORT */
        } else if(Typeof(victim) == TYPE_PLAYER) {
            notify(victim, "You feel a wrenching sensation...");
            enter_room(victim, destination);
            notify(player, "Teleported.");
        } else {
            /* check for non-sticky dropto */
            if(Typeof(destination) == TYPE_ROOM
               && db[destination].location != NOTHING
               && !(db[destination].flags & STICKY)) {
                /* destination has immediate dropto */
                destination = db[destination].location;
            }

            /* do the move */
            moveto(victim, destination);
            notify(player, "Teleported.");
        }
    }
}

void do_mass_teleport(dbref player, const char *arg1)
{
    dbref victim;
    dbref destination;
    char buf[BUFFER_LEN];
    int moved = 0;

    if(!God(player)) {
#ifndef TINKER
        notify(player, "Only God can do a mass teleport.");
#else /* TINKER */
        notify(player, "Only the Master Tinker can do a mass teleport.");
#endif /* TINKER */
        return;
    }

    /* get destination */
    init_match(player, arg1, TYPE_ROOM);
    match_here();
    match_absolute();

    if ((destination = match_result()) == NOTHING) {
        notify(player, "Please specify a destination to send everyone to.");
        return;
    }

    switch (Typeof(destination)) {
      case NOTHING:
        notify(player, "Send them where?");
        break;

      case TYPE_ROOM:
        for (victim = 0; victim < db_top; victim++) {
            if (victim != player && Typeof(victim) == TYPE_PLAYER) {
                notify(victim,
                 "You and everyone around you feels a wrenching sensation...");
                moveto(victim, destination);
                look_room(victim, destination);
                moved++;
            }
        }
        sprintf (buf, "Teleported %d players to %s.",
                 moved, unparse_object (player, destination));
        notify(player, "Teleported.");
        break;

      default:
        notify(player, "Mass teleports are legal to rooms only.");
    }
}

void do_force(dbref player, const char *what, char *command)
{
    dbref victim;

    if(!Wizard(player)) {
        writelog ("FORCE: failed, priv, player %s(%d), who '%s', what '%s'\n",
                  db[player].name, player, what, command);
#ifndef TINKER
        notify(player, "Only Wizards may use this command.");
#else /* TINKER */
        notify(player, "Only Tinkers may use this command.");
#endif /* TINKER */
        return;
    }

    /* get victim */
    if((victim = lookup_player(what)) == NOTHING) {
        writelog ("FORCE: failed, victim, player %s(%d), who '%s', what '%s'\n",
                  db[player].name, player, what, command);
        notify(player, "That player does not exist.");
        return;
    }

#ifdef GOD_PRIV
    if(God(victim)) {
#ifndef TINKER
        writelog ("FORCE: failed, wizard, player %s(%d), who '%s', what '%s'\n",
#else /* TINKER */
        writelog ("FORCE: failed, tinker, player %s(%d), who '%s', what '%s'\n",
#endif /* TINKER */
                  db[player].name, player, what, command);
#ifndef TINKER
        notify(player, "You can't force God.");
#else /* TINKER */
        notify(player, "You can't force the Master Tinker.");
#endif /* TINKER */
        return;
    }
#endif /* GOD_PRIV */

    /* force victim to do command */
    writelog ("FORCE: success, player %s(%d), who '%s', what '%s'\n",
              db[player].name, player, what, command);

    process_command(victim, command);
}

void do_stats(dbref player, const char *name)
{
    dbref rooms;
    dbref exits;
    dbref things;
    dbref players;
    dbref total;
    dbref i;
    dbref owner;
    char buf[BUFFER_LEN];

    if(!Wizard(player)) {
        sprintf(buf, "The universe contains %d objects.", db_top);
        notify(player, buf);
    } else {
        owner = lookup_player(name);
        total = rooms = exits = things = players = 0;
        for(i = 0; i < db_top; i++) {
            if(owner == NOTHING || owner == db[i].owner) {
                total++;
                switch(Typeof(i)) {
                  case TYPE_ROOM:
                    rooms++;
                    break;
                  case TYPE_EXIT:
                    exits++;
                    break;
                  case TYPE_THING:
                    things++;
                    break;
                  case TYPE_PLAYER:
                    players++;
                    break;
                  default:
                    abort();
                    break;
                }
            }
        }
        sprintf(buf,
                "%d objects = %d rooms, %d exits, %d things, %d players.",
                total, rooms, exits, things, players);
        notify(player, buf);
#ifdef TEST_MALLOC
        sprintf(buf, "Malloc count = %d.", malloc_count);
        notify(player, buf);
#endif /* TEST_MALLOC */
    }
}

void do_bobble(dbref player, const char *name, const char *rname)
{
    dbref victim, recip, i;
    char buf[BUFFER_LEN];
    int newflags = 0;

    if(!Wizard(player)) {
#ifndef TINKER
        writelog("TOAD: failed, priv %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player, "Only a Wizard can turn a person into a toad.");
#else /* TINKER */
        writelog("BOBBLE: failed, priv %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player, "Only a Tinker can bobble a person.");
#endif /* TINKER */
        return;
    }

    if (!name || !*name) {
#ifndef TINKER
        writelog("TOAD: failed, syntax %s(%d) who '%s'\n",
                 db[player].name, player, name);
#else /* TINKER */
        writelog("BOBBLE: failed, syntax %s(%d) who '%s'\n",
                 db[player].name, player, name);
#endif /* TINKER */
        notify(player, "You must specify a victim.");
        return;
    }

    init_match(player, name, TYPE_PLAYER);
    match_neighbor();
    match_absolute();
    match_player();
    if((victim = noisy_match_result()) == NOTHING) {
#ifndef TINKER
        writelog("TOAD: failed, victim %s(%d) who '%s'\n",
                 db[player].name, player, name);
#else /* TINKER */
        writelog("BOBBLE: failed, victim %s(%d) who '%s'\n",
                 db[player].name, player, name);
#endif /* TINKER */
        notify(player, "Please specify another victim.");
        return;
    }

#ifdef RECYCLE
    /* Default the recipient to RECYCLER */
    if (!rname || !*rname || string_compare (rname, RECYCLER) == 0) {
        recip = lookup_player(RECYCLER);
        newflags = UNWANTED;                /* Can be gotten by other players */
    } else {
        recip = lookup_player(rname);
    }
#else
    recip = lookup_player(rname);
#endif

    if(recip == NOTHING || recip == victim) {
#ifndef TINKER
        writelog("TOAD: failed, recip %s(%d) who '%s'\n",
                 db[player].name, player, name);
#else /* TINKER */
        writelog("BOBBLE: failed, recip %s(%d) who '%s'\n",
                 db[player].name, player, name);
#endif /* TINKER */
        notify(player, "Please specify another player to own the victim's effects.");
        return;
    }

    if(Typeof(victim) != TYPE_PLAYER) {
#ifndef TINKER
        writelog("TOAD: failed, type %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player, "You can only turn players into toads!");
#else /* TINKER */
        writelog("BOBBLE: failed, type %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player, "You can only bobble players!");
#endif /* TINKER */
    } else if(Wizard(victim)) {
#ifndef TINKER
        writelog("TOAD: failed, wizard %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player, "You can't turn a Wizard into a toad.");
#else /* TINKER */
        writelog("BOBBLE: failed, tinker %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player, "You can't bobble a Tinker.");
#endif /* TINKER */
    } else if(db[victim].contents != NOTHING) {
#ifndef TINKER
        writelog("TOAD: failed, contents %s(%d) who '%s'\n",
                 db[player].name, player, name);
#else /* TINKER */
        writelog("BOBBLE: failed, contents %s(%d) who '%s'\n",
                 db[player].name, player, name);
#endif /* TINKER */

        notify(player, "What about what they are carrying?");
    } else {
#ifndef TINKER
        writelog("TOAD: success %s(%d) who '%s'\n",
                 db[player].name, player, name);
#else /* TINKER */
        writelog("BOBBLE: success %s(%d) who '%s'\n",
                 db[player].name, player, name);
#endif /* TINKER */

        /* we are ok */
        /* do it */
        if(db[victim].password) {
            free((void *) db[victim].password);
            db[victim].password = 0;
        }
        db[victim].flags = TYPE_THING;

        /* Give the sphere and all old belongings to recipient */
        db[victim].owner = recip;
        for (i=0; i<db_top; i++) {
            if (db[i].owner == victim) {
                db[i].owner = recip;
                db[i].flags |= newflags;
            }
        }

        db[victim].pennies = 1;        /* dont let him keep his immense wealth */

        /* notify people */
#ifndef TINKER
        sprintf(buf, "You have been turned into a toad by %s.",
                db[player].name);
#else /* TINKER */
        sprintf(buf, "You have been encased in a stasis sphere by %s.",
                db[player].name);
#endif /* TINKER */
        notify(victim, buf);
#ifndef TINKER
        sprintf(buf, "You turned %s into a toad!", db[victim].name);
#else /* TINKER */
        sprintf(buf, "You bobbled %s!", db[victim].name);
#endif /* TINKER */
        notify(player, buf);

        /* reset name */
#ifdef PLAYER_LIST
        delete_player(victim);
#endif /* PLAYER_LIST         */
#ifndef TINKER
        sprintf(buf, "a slimy toad named %s", db[victim].name);
#else /* TINKER */
        sprintf(buf, "a silvery sphere containing %s", db[victim].name);
#endif /* TINKER */
        free((void *) db[victim].name);
        db[victim].name = alloc_string(buf);
    }
}

void do_unbobble(dbref player, const char *name, const char *newname)
{
    dbref victim;
    char buf[BUFFER_LEN];

    if(!Wizard(player)) {
#ifndef TINKER
        writelog("UNTOAD: failed, priv %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player, "Only a Wizard can turn a toad into a person.");
#else /* TINKER */
        writelog("UNBOBBLE: failed, priv %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player, "Only a Tinker can unbobble a person.");
#endif /* TINKER */
        return;
    }

    if (!name || !*name) {
#ifndef TINKER
        writelog("UNTOAD: failed, syntax %s(%d) who '%s'\n",
                 db[player].name, player, name);
#else /* TINKER */
        writelog("UNBOBBLE: failed, syntax %s(%d) who '%s'\n",
                 db[player].name, player, name);
#endif /* TINKER */
        notify(player, "You must specify a thing.");
        return;
    }

    if (!newname || !*newname) {
#ifndef TINKER
        writelog("UNTOAD: failed, syntax %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player,"You must specify a new name: @untoad <thing> = <name>");
#else /* TINKER */
        writelog("UNBOBBLE: failed, syntax %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player,
               "You must specify a new name: @unbobble <thing> = <name>");
#endif /* TINKER */
        return;
    }

    init_match(player, name, TYPE_THING);
    match_neighbor();
    match_absolute();
    match_player();
    if((victim = noisy_match_result()) == NOTHING) {
#ifndef TINKER
        writelog("UNTOAD: failed, victim %s(%d) who '%s'\n",
                 db[player].name, player, name);
#else /* TINKER */
        writelog("UNBOBBLE: failed, victim %s(%d) who '%s'\n",
                 db[player].name, player, name);
#endif /* TINKER */
        notify(player, "Please specify another thing.");
        return;
    }

    if(Typeof(victim) != TYPE_THING) {
#ifndef TINKER
        writelog("UNTOAD: failed, type %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player, "You can only turn players into toads!");
#else /* TINKER */
        writelog("UNBOBBLE: failed, type %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player, "You can only bobble players!");
#endif /* TINKER */
    } else if (!ok_player_name(newname)) {
#ifndef TINKER
        writelog("UNTOAD: failed, name %s(%d) who '%s'\n",
                 db[player].name, player, name);
#else /* TINKER */
        writelog("UNBOBBLE: failed, name %s(%d) who '%s'\n",
                 db[player].name, player, name);
#endif /* TINKER */
        notify(player, "You can't give a player that name!");
    } else {
#ifndef TINKER
        writelog("UNTOAD: success %s(%d) who '%s'\n",
                 db[player].name, player, name);
#else /* TINKER */
        writelog("UNBOBBLE: success %s(%d) who '%s'\n",
                 db[player].name, player, name);
#endif /* TINKER */

        /* we are ok */
        /* do it */
        db[victim].flags = TYPE_PLAYER;
        db[victim].owner = victim;
        db[victim].pennies = KILL_BONUS;

        /* reset name */
        free((void *) db[victim].name);
        db[victim].name = alloc_string(newname);

#ifndef TINKER
        sprintf(buf, "You turned the toad back into %s!", db[victim].name);
#else /* TINKER */
        sprintf(buf, "You unbobbled %s!", db[victim].name);
#endif /* TINKER */
        notify(player, buf);

        sprintf(buf, "Use @newpassword to give %s a password",
                db[victim].name);
        notify(player, buf);

#ifdef PLAYER_LIST
        add_player(victim);
#endif /* PLAYER_LIST         */
    }
}

void do_newpassword(dbref player, const char *name, const char *password)
{
    dbref victim;
    char buf[BUFFER_LEN];

    if(!Wizard(player)) {
        writelog("PASSWORD: failed, priv %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player, "Your delusions of grandeur have been duly noted.");
        return;
    } else if((victim = lookup_player(name)) == NOTHING) {
        writelog("PASSWORD: failed, victim %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player, "No such player.");
    } else if(*password != '\0' && !ok_password(password)) {
        /* Wiz can set null passwords, but not bad passwords */
        writelog("PASSWORD: failed, password %s(%d) who '%s'\n",
                 db[player].name, player, name);
        notify(player, "Bad password");
#ifdef GOD_PRIV
    } else if (God(victim) && !God(player)) {
#ifndef TINKER
        writelog("PASSWORD: failed, wizard %s(%d) who '%s'\n",
#else /* TINKER */
        writelog("PASSWORD: failed, tinker %s(%d) who '%s'\n",
#endif /* TINKER */
                 db[player].name, player, name);
        notify(player, "You cannot change that player's password.");
#endif /* GOD_PRIV         */
    } else {
        writelog("PASSWORD: success %s(%d) who '%s'\n",
                 db[player].name, player, name);
        /* it's ok, do it */
        if(db[victim].password) free((void *) db[victim].password);
        db[victim].password = alloc_string(password);
        notify(player, "Password changed.");
        sprintf(buf, "Your password has been changed by %s.", db[player].name);
        notify(victim, buf);
    }
}

void do_boot(dbref player, const char *name)
{
    dbref victim;
    char buf[BUFFER_LEN];

    if(!Wizard(player)) {
        writelog ("BOOT: failed, priv player %s(%d), who '%s'\n",
                  db[player].name, player, name);

#ifndef TINKER
        notify(player, "Only a Wizard can boot another player off!");
#else /* TINKER */
        notify(player, "Only a Tinker can boot another player off!");
#endif /* TINKER */
        return;
    }

    init_match(player, name, TYPE_PLAYER);
    match_neighbor();
    match_absolute();
    match_player();
    if((victim = noisy_match_result()) == NOTHING) return;

#ifdef GOD_PRIV
    if(God(victim)) {
#ifndef TINKER
        writelog ("BOOT: failed, wizard, player %s(%d), who '%s'\n",
#else /* TINKER */
        writelog ("BOOT: failed, tinker, player %s(%d), who '%s'\n",
#endif /* TINKER */
                  db[player].name, player, name);

        notify(player, "You can't boot that player!");
        return;
    }
#endif /* GOD_PRIV */

    if(Typeof(victim) != TYPE_PLAYER) {
        writelog ("BOOT: failed, victim, player %s(%d), who '%s'\n",
                  db[player].name, player, name);

        notify(player, "You can only boot off other players!");
    } else {
        writelog ("BOOT: success, player %s(%d), who '%s'\n",
                  db[player].name, player, name);

        /* we are ok */
        /* do it */
        /* notify people */
        sprintf(buf, "You have been booted off the game by %s.",
                db[player].name);
        notify(victim, buf);
        sprintf(buf, "You booted %s off!", db[victim].name);
        notify(player, buf);
        boot_off(victim);
        /* reset name */
    }
}

# define MINUTES (60)
# define HOURS   (3600)
# define DAYS    (24 * HOURS)
# define WEEKS   (7 * DAYS)
# define MONTHS  (30 * DAYS)
# define YEARS   (365 * DAYS)

long atosec (const char *str)
{
    long num;
    const char *s;

    if (!str || !*str) return (0L);

    num = atol (str);

    for (s=str; *s && (isspace (*s) || isdigit (*s)); s++) ;

    switch (*s) {
      case '\0':
      case 's':
      case 'S':                return (num); break;

      case 'm':
      case 'M':                if (s[1] == 'i' || s[1] == 'I') {
                                 return (num * MINUTES); break;
                               } else {
                                 return (num * MONTHS); break;
                               }
      case 'h':
      case 'H':                return (num * HOURS); break;

      case 'd':
      case 'D':                return (num * DAYS); break;

      case 'w':
      case 'W':                return (num * WEEKS); break;

      case 'y':
      case 'Y':                return (num * YEARS); break;

      default:                 return (num);
    }
}

void do_date(dbref player, const char *start, const char *thstr)
{
    dbref rooms=0, arooms=0;
    dbref exits=0, aexits=0;
    dbref things=0, athings=0;
    dbref players=0, aplayers=0;
    dbref total=0, atotal=0;
    dbref i;
    long now, start_tm, thresh;
    char buf[BUFFER_LEN];

    now = time (0);

    if(!Wizard(player)) {
        sprintf(buf, "%24.24s", ctime (&now));
        notify(player, buf);
        return;
    }

    start_tm = (start && *start) ?  now - atosec (start) : now - MONTHS;
    thresh = (thstr && *thstr) ? atoi (thstr) : 0;

    for (i=0; i<db_top; i++) {
        if (db[i].lastused >= start_tm && db[i].usecnt > thresh) {
            switch (Typeof(i)) {
              case TYPE_ROOM:        rooms++; arooms++; break;
              case TYPE_THING:        things++; athings++; break;
              case TYPE_EXIT:        exits++; aexits++; break;
              case TYPE_PLAYER:        players++; aplayers++; break;
            }
            total++; atotal++;
        } else {
            switch (Typeof(i)) {
              case TYPE_ROOM:        rooms++; break;
              case TYPE_THING:        things++; break;
              case TYPE_EXIT:        exits++; break;
              case TYPE_PLAYER:        players++; break;
            }
            total++;
        }
    }

    if (thresh == 0) {
        sprintf (buf, "Objects used since %20.20s:", ctime (&start_tm)+4);
    } else {
        sprintf (buf, "Objects used more than %ld time%s since %20.20s:",
                 thresh, (thresh == 1) ? "" : "s", ctime (&start_tm)+4);
    }
    notify (player, buf);

    sprintf (buf, "Totals:  %8d of %8d", atotal, total);
    notify (player, buf);

    sprintf (buf, "Rooms:   %8d of %8d", arooms, rooms);
    notify (player, buf);

    sprintf (buf, "Exits:   %8d of %8d", aexits, exits);
    notify (player, buf);

    sprintf (buf, "Things:  %8d of %8d", athings, things);
    notify (player, buf);

    sprintf (buf, "Players: %8d of %8d", aplayers, players);
    notify (player, buf);

    writelog ("DATE %s(%d) %s\n", db[player].name, player, start);
}

void do_top(dbref player, const char *numstr, const char *typestr)
{
    dbref *top = NULL;
    long num=10, type=NOTHING;
    dbref i, j, k;
    char buf[BUFFER_LEN];

    if(!Wizard(player)) {
        notify(player, "@Top is a restricted command.");
        return;
    }

    if (numstr && *numstr) num = atol (numstr);

    if (num < 1 || num > db_top) {
            sprintf (buf, "You must specify an integer from %d to %d for @top",
                 1, db_top);
        notify (player, buf);
        return;
    }

    if (typestr && *typestr) {
        switch (*typestr) {
          case 'p':
          case 'P':        type = TYPE_PLAYER; break;

          case 'e':
          case 'E':        type = TYPE_EXIT; break;

          case 'r':
          case 'R':        type = TYPE_ROOM; break;

          case 't':
          case 'T':        type = TYPE_THING; break;
        }
    }

    top = (dbref *) malloc (num * sizeof (dbref));
    for (j=0; j<num; j++) top[j] = NOTHING;

    /* For each item in the database */
    for (i=0; i<db_top; i++) {
        if (db[i].usecnt == 0) continue;

        if (type != NOTHING && Typeof(i) != type) continue;

        /* Loop through the top table, sorted by usecnt */
        for (j = num; j > 0; j--) {
            if (top[j-1] == NOTHING) continue;
            if (db[i].usecnt <= db[top[j-1]].usecnt) break;
        }

        /* Roll down the less used items, and insert i */
        if (j < num) {
            for (k = num-1; k > j; k--) top[k] = top[k-1];
            top[j] = i;
        }
    }

    /* Now print the items */
    switch (type) {
      case NOTHING:        sprintf (buf, "The top %ld items:", num);
                              break;
      case TYPE_PLAYER:        sprintf (buf, "The top %ld most active players:", num);
                              break;
      case TYPE_ROOM:        sprintf (buf, "The top %ld most active rooms:", num);
                              break;
      case TYPE_EXIT:        sprintf (buf, "The top %ld most used exits:", num);
                              break;
      case TYPE_THING:        sprintf (buf, "The top %ld most used things:", num);
                              break;
    }
    notify (player, buf);

    for (j=0; j<num && top[j] != NOTHING; j++) {
        sprintf (buf, "%10ld  %s", db[top[j]].usecnt,
                 unparse_object (player, top[j]));
        notify (player, buf);
    }

    writelog ("TOP  %s(%d) %s %s\n", db[player].name, player,
                  numstr ? numstr : "", typestr ? typestr : "");
}
