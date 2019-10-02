/** @file db.c
 *
 * Definition for database functions and source of several important globals.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "boolexp.h"
#include "compile.h"
#include "db.h"
#ifdef DISKBASE
#include "diskprop.h"
#endif
#include "edit.h"
#include "fbstrings.h"
#include "fbmath.h"
#include "fbtime.h"
#include "game.h"
#include "interface.h"
#include "match.h"
#include "log.h"
#include "player.h"
#include "predicates.h"
#include "props.h"
#include "tune.h"

/**
 * @var the array of objects that is our DB
 */
struct object *db = 0;

/**
 * @var the things currently being forced.
 */
objnode *forcelist = 0;

/**
 * @var the 'top' of the database; it is the highest dbref + 1
 */
dbref db_top = 0;

/**
 * @var the head of the garbage dbref list -- recycle-able objects
 *      This may be NOTHING.
 */
dbref recyclable = NOTHING;

/**
 * @private
 * @var this is the DB "load format", or version of the database based on
 *      reading its header.  Currently this can be 0, 10, or 11.
 *      0 is an error.
 */
static int db_load_format = 0;

#ifndef DB_INITIAL_SIZE
#define DB_INITIAL_SIZE 10000
#endif /* DB_INITIAL_SIZE */

/**
 * Grow the DB to a new size.
 *
 * 'newtop' will be the number of elements in the DB.  This won't let you
 * shrink the DB, 'newtop' must be greater than db_top
 *
 * @private
 * @param newtop the new DB size
 */
static void
db_grow(dbref newtop)
{
    if (newtop > db_top) {
        db_top = newtop;

        if (db) {
            if ((db = realloc(db, (size_t)db_top * sizeof(struct object))) == 0) {
                abort();
            }
        } else {
            int startsize = MAX(newtop, DB_INITIAL_SIZE);

            if ((db = malloc((size_t)startsize * sizeof(struct object))) == 0) {
                abort();
            }
        }
    }
}

/**
 * Clear a DB object
 *
 * This will take a ref and zero out the object, making it suitable for
 * use (or reuse, in the case of recycled garbage).
 *
 * This does NOT clear out memory, so you SHOULD NOT use clear out an
 * object with allocated memory -- this should only be used with objects
 * that are brand new (i.e. freshly allocated) or that have been totally
 * cleaned out already (garbage).
 *
 * Flags must be initialized after this is called.  Also, type-specific
 * fields must also be initialized.  The object is not set dirty by this.
 *
 * @param i the dbref of the object to clear out.
 */
void
db_clear_object(dbref i)
{
    struct object *o = DBFETCH(i);

    memset(o, 0, sizeof(struct object));

    NAME(i) = 0;
    ts_newobject(i);
    o->location = NOTHING;
    o->contents = NOTHING;
    o->exits = NOTHING;
    o->next = NOTHING;
    o->properties = 0;

#ifdef DISKBASE
    o->propsfpos = 0;
    o->propstime = 0;
    o->propsmode = PROPS_UNLOADED;
    o->nextold = NOTHING;
    o->prevold = NOTHING;
#endif
}

/**
 * Return an object ready to use.
 *
 * This may be a recycled garbage object or a new object.  Either way,
 * it will be empty and ready for use.
 *
 * @return the database ref of the object
 */
dbref
new_object(void)
{
    dbref newobj;

    if (recyclable != NOTHING) {
        newobj = recyclable;
        recyclable = NEXTOBJ(newobj);

        /*
         * This will deallocate any memory in use by the garbage object
         * before we clear out the object.
         */
        db_free_object(newobj);
    } else {
        newobj = db_top;
        db_grow(db_top + 1);
    }

    /* clear it out */
    db_clear_object(newobj);

    /* Make sure it is set dirty for diskbase purposes. */
    DBDIRTY(newobj);

    return newobj;
}

/**
 * Allocate an object with a certain name, owner, and initial flags
 *
 * This may be a former garbage object, or a brand new one.  The
 * string 'name' is copied.
 *
 * @private
 * @param name the name of the object
 * @param owner the dbref of the owner
 * @param flags the initial object flags
 * @return the dbref of the new object
 */
static dbref
create_object(const char *name, dbref owner, object_flag_type flags)
{
    dbref newobj = new_object();

    NAME(newobj) = alloc_string(name);
    FLAGS(newobj) = flags;
    OWNER(newobj) = OWNER(owner);

    return newobj;
}

/**
 * Allocate an EXIT type object
 *
 * With a given owner, name, and source (location).  Returns the DBREF
 * of the exit.  The 'name' memory is copied.
 *
 * @param player the owner dbref
 * @param name the name of the new exit
 * @param source the location to put the exit
 * @return the dbref of the new exit
 */
dbref
create_action(dbref player, const char *name, dbref source)
{
    dbref newact = create_object(name, player, TYPE_EXIT);

    set_source(newact, source);

    DBFETCH(newact)->sp.exit.ndest = 0;
    DBFETCH(newact)->sp.exit.dest = NULL;

    if (tp_autolink_actions) {
        DBFETCH(newact)->sp.exit.ndest = 1;
        DBFETCH(newact)->sp.exit.dest = malloc(sizeof(dbref));
        (DBFETCH(newact)->sp.exit.dest)[0] = NIL;
    }

    return newact;
}

/**
 * Allocate an PROGRAM type object
 *
 * With a given name and owner/creator.  Returns the DBREF of the program. 
 * The 'name' memory is copied.  Initializes the "special" fields.
 *
 * @param name the name of the new program
 * @param player the owner dbref
 * @return the dbref of the new program
 */
dbref
create_program(dbref player, const char *name)
{
    char buf[BUFFER_LEN];
    int jj;
    dbref newprog = create_object(name, player, TYPE_PROGRAM);

    snprintf(buf, sizeof(buf), "A scroll containing a spell called %s", name);
    SETDESC(newprog, buf);
    LOCATION(newprog) = player;

    ALLOC_PROGRAM_SP(newprog);

    memset(PROGRAM_SP(newprog), 0, sizeof(*(PROGRAM_SP(newprog))));

    /* Add the program to the player's contents */
    PUSH(newprog, CONTENTS(player));

    /* Set dirty for diskbase */
    DBDIRTY(newprog);
    DBDIRTY(player);

    /* Set initial flags from the tune settings */
    set_flags_from_tunestr(newprog, tp_new_program_flags);

    /*
     * Set the right MUCKER level - must be at most the MUCKER level of the
     * creating player.
     */
    jj = MLevel(newprog);
    if (jj == 0 || jj > MLevel(player))
        SetMLevel(newprog, MLevel(player));

    return newprog;
}

/**
 * Allocate an ROOM type object
 *
 * With a given owner/creator, name, and parent (location).  Returns the DBREF
 * of the room.   The 'name' memory is copied.  Initializes the "special"
 * fields for the room.
 *
 * If the player is JUMP_OK, then the created room will be JUMP_OK as well.
 *
 * @param player the owner dbref
 * @param name the name of the new room
 * @param parent the parent room's dbref
 * @return the dbref of the new room
 */
dbref
create_room(dbref player, const char *name, dbref parent)
{
    dbref newroom = create_object(name, player,
                    TYPE_ROOM | (FLAGS(player) & JUMP_OK));

    LOCATION(newroom) = parent;
    PUSH(newroom, CONTENTS(parent));

    EXITS(newroom) = NOTHING;
    DBFETCH(newroom)->sp.room.dropto = NOTHING;

    DBDIRTY(parent);

    return newroom;
}

/**
 * Allocate an THING type object
 *
 * With a given owner/creator, name, and location.  Returns the DBREF
 * of the thing.   The 'name' memory is copied.  Initializes the "special"
 * fields for the thing.
 *
 * The home is set to the current room if the player controls the room;
 * otherwise the home is set to the player.
 *
 * @param player the owner dbref
 * @param name the name of the new thing
 * @param location the location to place the object
 * @return the dbref of the new thing
 */
dbref
create_thing(dbref player, const char *name, dbref location)
{
    dbref loc;
    dbref newthing = create_object(name, player, TYPE_THING);

    LOCATION(newthing) = location;
    PUSH(newthing, CONTENTS(location));

    EXITS(newthing) = NOTHING;
    SETVALUE(newthing, 0);

    ALLOC_THING_SP(newthing);

    if ((loc = LOCATION(player)) != NOTHING && controls(player, loc)) {
        THING_SET_HOME(newthing, loc);
    } else {
        THING_SET_HOME(newthing, player);
    }

    DBDIRTY(location);

    return newthing;
}

/**
 * Write a ref to the given file handle
 *
 * This will abort the program if the write fails.  A newline character is
 * also written along with the ref.
 *
 * @param f the file to write to
 * @param ref the reference to write
 */
void
putref(FILE * f, dbref ref)
{
    if (fprintf(f, "%d\n", ref) < 0) {
        abort();
    }
}

/**
 * Write a string to the given file handle
 *
 * This will abort the program if the write fails.  A newline character is
 * also written along with the string.
 *
 * @param f the file to write to
 * @param s the string to write
 */
void
putstring(FILE * f, const char *s)
{
    if (s) {
        if (fputs(s, f) == EOF) {
            abort();
        }
    }

    if (putc('\n', f) == EOF) {
        abort();
    }
}

/**
 * Write properties to a given file handle
 *
 * This injects the *Props* and *End* delimiters that mark the start and
 * end of the block of properties in the database.
 *
 * @param f the file to write to
 * @param obj the object to dump props for
 */
void
putproperties(FILE * f, dbref obj)
{
    putstring(f, "*Props*");
    db_dump_props(f, obj);
    putstring(f, "*End*");
}

/**
 * Write an object to a given file handle.
 *
 * Object data is written in the following order, each on its own line:
 *
 * * name
 * * location field as integer string
 * * contents field as integer string
 * * next field as integer string
 * * flags, minus non-internal flags as integer string
 * * created timestamp as integer string
 * * last used timestamp as integer string
 * * use count as integer string
 * * modified timestamp as integer string
 * * The string "*Props*"
 * * The properties - @see putproperties
 * * The string "*End*"
 * * Then the rest varies based on type:
 *   * THING:
 *     * Home field as integer string
 *     * exit field as integer string
 *     * owner field as integer string
 *   * ROOM:
 *     * Drop to field as integer string
 *     * exit field as integer string
 *     * owner field as integer string
 *   * EXIT:
 *     * Number of exit destinations as integer string
 *     * The exit destinations, one per line, as integer strings
 *     * owner field as integer string
 *   * PLAYER:
 *     * Player home as integer string
 *     * exit field as integer string
 *     * player password
 *   * PROGRAM:
 *     * owner field as integer string
 *
 * @private
 * @param f the file handle
 * @param i the object dbref to write to the file handle
 */
static void
db_write_object(FILE * f, dbref i)
{
    struct object *o = DBFETCH(i);
#ifdef DISKBASE
    long tmppos;
#endif /* DISKBASE */

    putstring(f, NAME(i));
    putref(f, o->location);
    putref(f, o->contents);
    putref(f, o->next);

    /*
     * @TODO This writes the flags as a signed integer even though
     *       it isn't signed.  Could we also have endian-ness issues?
     *       Porting a MUCK from one endian-ness to another ...
     *
     *       We could use a string instead kind of like the tune flag
     *       settings which makes this more platform independent, or
     *       write it as an unsigned.
     */
    putref(f, FLAGS(i) & ~DUMP_MASK);   /* write non-internal flags */

    putref(f, (int)o->ts_created);
    putref(f, (int)o->ts_lastused);
    putref(f, o->ts_usecount);
    putref(f, (int)o->ts_modified);

#ifdef DISKBASE
    tmppos = ftell(f) + 1;
    putprops_copy(f, i);
    o->propsfpos = tmppos;
    undirtyprops(i);
#else /* !DISKBASE */
    putproperties(f, i);
#endif /* DISKBASE */

    switch (Typeof(i)) {
        case TYPE_THING:
            putref(f, THING_HOME(i));
            putref(f, o->exits);
            putref(f, OWNER(i));
            break;

        case TYPE_ROOM:
            putref(f, o->sp.room.dropto);
            putref(f, o->exits);
            putref(f, OWNER(i));
            break;

        case TYPE_EXIT:
            putref(f, o->sp.exit.ndest);

            for (int j = 0; j < o->sp.exit.ndest; j++) {
                putref(f, (o->sp.exit.dest)[j]);
            }

            putref(f, OWNER(i));
            break;

        case TYPE_PLAYER:
            putref(f, PLAYER_HOME(i));
            putref(f, o->exits);
            putstring(f, PLAYER_PASSWORD(i));
            break;

        case TYPE_PROGRAM:
            putref(f, OWNER(i));
            break;
    }
}

/**
 * Peek at the next character in the file stream; does not advance stream
 *
 * @private
 * @param f the file handle to peek at
 * @return the next character in the file stream
 */
static int
do_peek(FILE * f)
{
    int peekch;

    ungetc((peekch = getc(f)), f);

    return peekch;
}

/**
 * Get a reference from the given file handle
 *
 * @param f the file handle to get a reference from
 * @return the dbref
 */
dbref
getref(FILE * f)
{
    static char buf[BUFFER_LEN];

    fgets(buf, sizeof(buf), f);
    return atol(buf);
}

/**
 * Get a string from the given file handle
 *
 * The string is read until a \n is encountered or until the buffer runs
 * out; anything over BUFFER_LEN -1 is discarded til the next \n is found.
 *
 * Empty strings are supported and will be returned.  New memory is
 * allocated for the string that is returned.
 *
 * @private
 * @param f the file handle to get a string from
 * @return the string
 */
static char *
getstring(FILE * f)
{
    static char buf[BUFFER_LEN];
    char c;

    if (fgets(buf, sizeof(buf), f) == NULL) {
        return alloc_string("");
    }

    if (strlen(buf) == BUFFER_LEN - 1) {
        /* ignore whatever comes after */
        if (buf[BUFFER_LEN - 2] != '\n')
            while ((c = fgetc(f)) != '\n') ;
    }

    for (char *p = buf; *p; p++) {
        if (*p == '\n') {
            *p = '\0';
            break;
        }
    }

    return alloc_string(buf);
}

/**
 * Read the header from the database file
 *
 * Returns 0 if the format is incorrect or not understood.  10 if it
 * is "Foxen8" or 11 if the format is DB_VERSION_STRING
 *
 * This also loads the tune parameters from the DB file.
 *
 * @see tune_load_parms_from_file
 *
 * The format is:
 *
 * * ***FoxenX TinyMUCK DUMP Format*** where X is a numeric DB version number
 * * the number of objects in the database as an integer string
 * * An ignored dbflags value as an integer string
 * * the tune parameters - see tune_load_parms_from_file for format.
 *
 * @private
 * @param f the file handle
 * @param grow the object count of the DB
 * @return the format of the DB file
 */
static int
db_read_header(FILE * f, int *grow)
{
    int result = 0;
    int load_format = 0;
    char *special;

    *grow = 0;

    if (do_peek(f) != '*') {
        return result;
    }

    special = getstring(f);

    /*
     * @TODO So first off, this isn't used *anywhere*.  It is passed into
     *       db_read_object but is ignored.  It is checked if it is 0,
     *       because that is an error, but the number 10 vs 11 has no meaning.
     *
     *       Secondly, this is logically faulty.  If DB_VERSION_STRING
     *       is ever changed to support a new version, then it doesn't
     *       actually change the format integer returned.
     *
     *       An improvement would be to scoop the number that comes
     *       after "Foxen" and use that as the return value here.
     *
     *       We could then move away from having a DB_VERSION_STRING
     *       and just havig a DB_VERSION_NUMBER, since we can construct
     *       the header without it being a hard coded string.
     *
     *       Update the docblock if you correct this oddity.
     */
    if (!strcmp(special, "***Foxen8 TinyMUCK DUMP Format***")) {
        load_format = 10;
    } else if (!strcmp(special, DB_VERSION_STRING)) {
        load_format = 11;
    }

    free(special);

    *grow = getref(f);

    getref(f);      /* ignore dbflags value */

    /**
     * Delay sysparm processing until the end. At this point,
     * setting dbref values will fail since there are no objects.
     * Future database formats should not have this weirdness.
     */

    char buf[BUFFER_LEN];

    for (int i = 0, n = getref(f); i < n; i++) {
        fgets(buf, sizeof(buf), f);
    }

    return load_format;
}

/**
 * Write the DB file header
 *
 * @see db_read_header for format
 *
 * @private
 * @param f the file handle
 */
static void
db_write_header(FILE * f)
{
    putstring(f, DB_VERSION_STRING);

    putref(f, db_top);
    putref(f, DB_PARMSINFO);
    putref(f, tune_count_parms());
    tune_save_parms_to_file(f);
}

/**
 * Write the database out to a given file handle
 *
 * The file format is the header followed by the objects in reverse
 * order (highest number first).  If you wish to see the file format
 * information, see the db_write_header and db_write_object function
 * calls in db.c.
 *
 * The dump ends with ***END OF DUMP***
 *
 * If there is an error writing, this abort()s the program.
 *
 * @param f the file handle to write to
 * @return db_top value
 */
dbref
db_write(FILE * f)
{
    db_write_header(f);

    for (dbref i = db_top; i-- > 0;) {
        if (fprintf(f, "#%d\n", i) < 0)
            abort();

        db_write_object(f, i);
        FLAGS(i) &= ~OBJECT_CHANGED;    /* clear changed flag */
    }

    fseek(f, 0L, SEEK_END);
    putstring(f, "***END OF DUMP***");

    fflush(f);
    return db_top;
}

/**
 * Load properties from the file handle
 *
 * This assumes that the file pointer is in the correct position to
 * load properties for object 'obj' *unless* DISKBASE is true, in which
 * case we will seek to the correct position first.
 *
 * This call assumes the first '*' has been nibbled already, and if the
 * line 'Props*' remains in the file stream we will use the "new" prop
 * loader.  @see db_getprops
 *
 * Otherwise, it uses the "old" prop load process which is the bulk
 * of this function.  The "old" prop load function only supports integer
 * and string loads.
 *
 * @param f the file handle to load from
 * @param obj the object to load props for
 * @param pdir the root prop directory to load into.  This can be NULL for
 *        an initial prop load.  It is used for diskbase prop loading.
 */
void
getproperties(FILE * f, dbref obj, const char *pdir)
{
    char buf[BUFFER_LEN * 3], *p;
    int datalen;

#ifdef DISKBASE
    /* if no props, then don't bother looking. */
    if (!DBFETCH(obj)->propsfpos)
        return;

    /* seek to the proper file position. */
    fseek(f, DBFETCH(obj)->propsfpos, SEEK_SET);
#endif

    /* get rid of first line */
    fgets(buf, sizeof(buf), f);

    /*
     * @TODO Potential bug.  So, there's two cases; the first case, initial
     *       DB load, the function 'db_read_object' in this file, nibbles
     *       the '*' to see if there are props to load.  That's why we
     *       check for "Props*" instead of "*Props*" here.  This is fine for
     *       initial load.
     *
     *       HOWEVER ... diskprop.c also uses this function.  If I'm reading
     *       diskprops right, it is *not* nibbling the '*', and thusly it is
     *       *always* going to use the old loader.  Which means diskprops.c
     *       will always mangle 'ref', 'lock', and other such props.
     *
     *       I don't know enough about how disk props works (yet) to know
     *       if this is a problem or not.  But just pointing out an oddity
     *       here. (tanabi)
     */
    if (strcmp(buf, "Props*\n")) {
        /* initialize first line stuff */
        fgets(buf, sizeof(buf), f);

        while (1) {
            /* fgets reads in \n too! */
            if (!strcmp(buf, "*End*\n"))
                break;

            p = strchr(buf, PROP_DELIMITER);
            *(p++) = '\0';  /* Purrrrrrrrrr... */
            datalen = strlen(p);
            p[datalen - 1] = '\0';

            if ((p - buf) >= BUFFER_LEN)
                buf[BUFFER_LEN - 1] = '\0';

            if (datalen >= BUFFER_LEN)
                p[BUFFER_LEN - 1] = '\0';

            if ((*p == '^') && (number(p + 1))) {
                add_prop_nofetch(obj, buf, NULL, atol(p + 1));
            } else {
                if (*buf) {
                    add_prop_nofetch(obj, buf, p, 0);
                }
            }

            fgets(buf, sizeof(buf), f);
        }
    } else {
        db_getprops(f, obj, pdir);
    }
}

/**
 * Free the memory associated with an object
 *
 * This does not shrink 'db', but it frees all the different pointers
 * used by an object -- name, "special" structure, program code for programs,
 * etc.
 *
 * If this is a program, it is uncompiled first.  @see uncompile_program
 *
 * Not a lot of checking is done, so it would be pretty easy to double-free
 * an object if you are not careful; mutating the object into a TYPE_GARBAGE
 * after it has been free'd will render it fairly safe.
 *
 * @param i the dbref of the object to free memory for.
 */
void
db_free_object(dbref i)
{
    struct object *o;

    /*
     * Do this before freeing the name because uncompile_program()
     * may try to print some error messages.
     */
    if (Typeof(i) == TYPE_PROGRAM) {
        uncompile_program(i);

        if (PROGRAM_FIRST(i)) {
            free_prog_text(PROGRAM_FIRST(i));
            PROGRAM_SET_FIRST(i, NULL);
        }

        FREE_PROGRAM_SP(i);
    }

    o = DBFETCH(i);

    free((void *) NAME(i));

#ifdef DISKBASE
    unloadprops_with_prejudice(i);
#else
    if (o->properties) {
        delete_proplist(o->properties);
    }
#endif

    if (Typeof(i) == TYPE_EXIT) {
        free(o->sp.exit.dest);
    } else if (Typeof(i) == TYPE_PLAYER) {
        free((void *) PLAYER_PASSWORD(i));
        free(PLAYER_DESCRS(i));
        PLAYER_SET_DESCRS(i, NULL);
        PLAYER_SET_DESCRCOUNT(i, 0);

        ignore_flush_cache(i);
    }

    if (Typeof(i) == TYPE_THING) {
        FREE_THING_SP(i);
    }

    if (Typeof(i) == TYPE_PLAYER) {
        FREE_PLAYER_SP(i);
    }
}

/**
 * Free the memory for the whole database
 *
 * This also runs clear_players and clear_primitives
 *
 * @see clear_players
 * @see clear_primitives
 */
void
db_free(void)
{
    if (db) {
        for (dbref i = 0; i < db_top; i++)
            db_free_object(i);

        free(db);
        db = 0;
        db_top = 0;
    }

    clear_players();
    clear_primitives();
    recyclable = NOTHING;
}

/**
 * Read a database object from the given file handle
 *
 * This handles all the intricities of reading a single database object
 * from the file handle.  This includes support for reading both old and new
 * style properties.
 *
 * This takes a "dtype" which is the database version.  It is actually
 * for all intents and purposes ignored.  If it is 0, we return immediately,
 * but otherwise nothing is done with this number.
 *
 * @private
 * @param f the file handle
 * @param objno the object ref we are loading
 */
static void
db_read_object(FILE * f, dbref objno)
{
    int tmp, c, prop_flag = 0;
    int j = 0;
    const char *password;
    struct object *o;

    db_clear_object(objno);

    /*
     * If any file read fails, we abort(), so that is why there is no real
     * error checking here.
     */
    FLAGS(objno) = 0;
    NAME(objno) = getstring(f);

    o = DBFETCH(objno);
    o->location = getref(f);
    o->contents = getref(f);
    o->next = getref(f);

    tmp = getref(f);    /* flags list */
    tmp &= ~DUMP_MASK;
    FLAGS(objno) |= tmp;

    o->ts_created = getref(f);
    o->ts_lastused = getref(f);
    o->ts_usecount = getref(f);
    o->ts_modified = getref(f);

    c = getc(f);
 
    if (c == '*') {
#ifdef DISKBASE
        o->propsfpos = ftell(f);

        /*
         * @TODO I believe that this o->propsmode will always be 0 because
         *       we do db_clear_object and then don't ever set it.  I could
         *       be wrong here because I didn't trace everything; the todo
         *       is to trace it and see if this if statement makes sense.
         */
        if (o->propsmode == PROPS_CHANGED) {
            getproperties(f, objno, NULL);
        } else {
            skipproperties(f, objno);
        }
#else
        getproperties(f, objno, NULL);
#endif

        prop_flag++;
    } else {
        /* do our own getref */
        int sign = 0;
        char buf[BUFFER_LEN];
        int i = 0;

        if (c == '-')
            sign = 1;
        else if (c != '+') {
            buf[i] = c;
            i++;
        }

        while ((c = getc(f)) != '\n') {
            buf[i] = c;
            i++;
        }

        buf[i] = '\0';
        j = atol(buf);

        if (sign)
            j = -j;
    }

    /*
     * This allocates the different "special" structures that are unique
     * to different object types.
     */
    switch (FLAGS(objno) & TYPE_MASK) {
    case TYPE_THING:
        ALLOC_THING_SP(objno);
        THING_SET_HOME(objno, prop_flag ? getref(f) : j);
        o->exits = getref(f);
        OWNER(objno) = getref(f);
        break;

    case TYPE_ROOM:
        o->sp.room.dropto = prop_flag ? getref(f) : j;
        o->exits = getref(f);
        OWNER(objno) = getref(f);
        break;

    case TYPE_EXIT:
        o->sp.exit.ndest = prop_flag ? getref(f) : j;

        /* only allocate space for linked exits */
        if (o->sp.exit.ndest > 0)
            o->sp.exit.dest = malloc(sizeof(dbref) * (size_t)(o->sp.exit.ndest));

        for (j = 0; j < o->sp.exit.ndest; j++) {
            (o->sp.exit.dest)[j] = getref(f);
        }

        OWNER(objno) = getref(f);
        break;

    case TYPE_PLAYER:
        ALLOC_PLAYER_SP(objno);
        PLAYER_SET_HOME(objno, prop_flag ? getref(f) : j);
        o->exits = getref(f);
        password = getstring(f);
        set_password_raw(objno, password);
        PLAYER_SET_CURR_PROG(objno, NOTHING);
        PLAYER_SET_IGNORE_LAST(objno, NOTHING);
        OWNER(objno) = objno;
        add_player(objno);
        break;

    case TYPE_PROGRAM:
        ALLOC_PROGRAM_SP(objno);
        OWNER(objno) = prop_flag ? getref(f) : j;
        FLAGS(objno) &= ~INTERNAL;
        break;

    case TYPE_GARBAGE:
        break;
    }
}

/**
 * Autostart all programs set ABODE
 *
 * This autostarts by scanning the entire DB and compiling the ABODE
 * programs.  Compile will automatically queue up the programs.
 *
 * @static
 */
static void
autostart_progs(void)
{
    struct line *tmp;

    /* Don't do it if we're converting the DB */
    if (db_conversion_flag) {
        return;
    }

    for (dbref i = 0; i < db_top; i++) {
        if (Typeof(i) == TYPE_PROGRAM) {
            if ((FLAGS(i) & ABODE) && TrueWizard(OWNER(i))) {
                /*
                 * Pre-compile AUTOSTART programs.  They queue up when they
                 * finish compiling.
                 */
                tmp = PROGRAM_FIRST(i);
                PROGRAM_SET_FIRST(i, (struct line *) read_program(i));
                do_compile(-1, OWNER(i), i, 0);
                free_prog_text(PROGRAM_FIRST(i));
                PROGRAM_SET_FIRST(i, tmp);
            }
        }
    }
}

/**
 * Read the FuzzBall DB from the given file handle.
 *
 * Returns the dbtop value, or -1 if the header is invalid.  If
 * there is a problem loading the database, chances are it will trigger
 * an abort() as there is no gentle error handling in this process.
 *
 * @param f the file handle to load from
 * @return the dbtop value or #-1 if the header is invalid
 */
dbref
db_read(FILE * f)
{
    dbref grow;
    char *special;

    /* Parse the header */
    db_load_format = db_read_header(f, &grow);

    if (db_load_format == 0) {
        return -1;
    }

    /* This sizes the DB to fit all the objects to load */
    db_grow(grow);

    for (int i = 0; i < db_top; i++) {
        if (getc(f) != NUMBER_TOKEN) return -1;
        db_read_object(f, getref(f));
    }

    special = getstring(f);
    if (strcmp(special, "***END OF DUMP***")) {
        free(special);
        return -1;
    }

    free(special);

    /**
     * Go back and read the sysparms.
     **/

    fseek(f, 0L, SEEK_SET);

    free(getstring(f));
    getref(f);
    getref(f);
    tune_load_parms_from_file(f, NOTHING, getref(f));

    for (dbref j = 0; j < db_top; j++) {
        if (Typeof(j) == TYPE_GARBAGE) {
            NEXTOBJ(j) = recyclable;
            recyclable = j;
        }
    }

    autostart_progs();
    return db_top;
}

/**
 * Find a dbref in an objnode list
 *
 * @param head the head of the objnode list
 * @param data the dbref to look for
 * @return boolean true if dbref is on the list, false if not.
 */
int
objnode_find(objnode *head, dbref data)
{
    objnode *tmp = head;

    while (tmp) {
        if (tmp->data == data)
            return 1;
        tmp = tmp->next;
    }

    return 0;
}

/**
 * Push a dbref onto an objnode
 *
 * The new ref is put at the head of the objnode list, which is why
 * we need a pointer to a pointer here.
 *
 * @param head pointer to pointer to head of objnode list.
 * @param data the dbref to add to the head of the list.
 */
void
objnode_push(objnode **head, dbref data)
{
    objnode *tmp;

    if (!(tmp = malloc(sizeof(objnode)))) {
        fprintf(stderr, "objnode_push: out of memory\n");
        abort();
    }

    tmp->data = data;
    tmp->next = *head;
    *head = tmp;
}

/**
 * Pop a dbref off the objnode list.
 *
 * If head is NULL, nothing happens.  Removes the node and frees the
 * memory but does not return the pop'd dbref.
 *
 * @param head pointer to pointer of list to pop from.
 */
void objnode_pop(objnode **head)
{
    if (!*head)
        return;

    objnode *tmp = *head;
    *head = tmp->next;
    free(tmp);
}

/**
 * "Unparses" flags, or rather, gives a string representation of object flags
 *
 * This uses a static buffer, so make sure to copy it if you want to keep
 * it.  This is, of course, not threadsafe.
 *
 * @param thing the object to construct a flag string for
 * @return the constructed flag string in a static buffer
 */
const char *
unparse_flags(dbref thing)
{
    static char buf[BUFFER_LEN];
    char *p;
    const char *type_codes = "R-EPFG";

    p = buf;
    if (Typeof(thing) != TYPE_THING)
        *p++ = type_codes[Typeof(thing)];

    /*
     * @TODO In a perfect world, this would be more configuration based
     *       and not a long line of if statements.  But, it must be
     *       high performance because it is used a lot.
     *
     *       I don't really have an idea here, to be honest, but its
     *       something to think about (tanabi)
     */
    if (FLAGS(thing) & ~TYPE_MASK) {
        /* print flags */
        if (FLAGS(thing) & WIZARD)
            *p++ = 'W';

        if (FLAGS(thing) & LINK_OK)
            *p++ = 'L';

        if (FLAGS(thing) & KILL_OK)
            *p++ = 'K';

        if (FLAGS(thing) & DARK)
            *p++ = 'D';

        if (FLAGS(thing) & STICKY)
            *p++ = 'S';

        if (FLAGS(thing) & QUELL)
            *p++ = 'Q';

        if (FLAGS(thing) & BUILDER)
            *p++ = 'B';

        if (FLAGS(thing) & CHOWN_OK)
            *p++ = 'C';

        if (FLAGS(thing) & JUMP_OK)
            *p++ = 'J';

        if (FLAGS(thing) & GUEST)
            *p++ = 'G';

        if (FLAGS(thing) & HAVEN)
            *p++ = 'H';

        if (FLAGS(thing) & ABODE)
            *p++ = 'A';

        if (FLAGS(thing) & VEHICLE)
            *p++ = 'V';

        if (FLAGS(thing) & XFORCIBLE)
            *p++ = 'X';

        if (FLAGS(thing) & ZOMBIE)
            *p++ = 'Z';

        if (FLAGS(thing) & YIELD)
            *p++ = 'Y';

        if (FLAGS(thing) & OVERT)
            *p++ = 'O';

        if (MLevRaw(thing)) {
            *p++ = 'M';

            switch (MLevRaw(thing)) {
                case 1:
                    *p++ = '1';
                    break;

                case 2:
                    *p++ = '2';
                    break;

                case 3:
                    *p++ = '3';
                    break;
            }
        }
    }

    *p = '\0';
    return buf;
}

/**
 * "Unparse" an object, showing itsnam and list of flags if permissions allow
 *
 * Uses the provided buffer that has the given size.  Practically speaking,
 * names can be as long as BUFFER_LEN, so your buffer should probably
 * be BUFFER_LEN at least in size (This is the most common practice).  If
 * a name was actually its maximum length, then there is not enough room
 * for flags to show up.  Traditionally, Fuzzball has not cared about this
 * problem because, traditionally, names just don't get that long.
 *
 * Flags are only shown if:
 *
 * * player == NOTHING
 * * or player does not have STICKY flag AND:
 *   * 'loc' is linkable - @see can_link_to
 *   * or 'loc' is not a player and 'player' controls 'loc'
 *   * or 'loc' is CHOWN_OK
 *
 * @param player the player doing the call or NOTHING
 * @param loc the target to generate unparse text for
 * @param buffer the buffer to use
 * @param size the size of the buffer
 */
void
unparse_object(dbref player, dbref loc, char *buffer, size_t size)
{
    /* Handle ZOMBIE case */
    if (player != NOTHING)
        player = OWNER(player);

    switch (loc) {
        case NOTHING:
            strcpyn(buffer, size, "*NOTHING*");
            return;

        case AMBIGUOUS:
            strcpyn(buffer, size, "*AMBIGUOUS*");
            return;

        case HOME:
            strcpyn(buffer, size, "*HOME*");
            return;

        case NIL:
            strcpyn(buffer, size, "*NIL*");
            return;

        default:
            if (!ObjExists(loc)) {
                strcpyn(buffer, size, "*INVALID*");
                return;
            }

            if ((player == NOTHING) || (!(FLAGS(player) & STICKY) &&
                (can_link_to(player, NOTYPE, loc) ||
                ((Typeof(loc) != TYPE_PLAYER) &&
                (controls_link(player, loc)
                || (FLAGS(loc) & CHOWN_OK)))))) {
                /* show everything */
                snprintf(buffer, size, "%.*s(#%d%s)", (BUFFER_LEN / 2), NAME(loc), loc,
                         unparse_flags(loc));
            } else {
                /* show only the name */
                strcpyn(buffer, size, NAME(loc));
            }
    }
}

/**
 * Remove something from first's "next" dbref list
 *
 * dbref list refers to the dbref-linked list structure that is used by
 * objects
 *
 * @param first the object to find 'what' in
 * @param what the object to find
 * @return either 'first' or first's NEXTOBJ if first == what
 */
dbref
remove_first(dbref first, dbref what)
{
    dbref prev;

    /* special case if it's the first one */
    if (first == what) {
        return NEXTOBJ(first);
    } else {
        /* have to find it */
        DOLIST(prev, first) {
            if (NEXTOBJ(prev) == what) {
                DBSTORE(prev, next, NEXTOBJ(what));
                return first;
            }
        }

        return first;
    }
}

/**
 * Check to see if 'thing' is on ref-list 'list' or any of 'list's contents
 *
 * This is used by boolexp to see if 'thing' has any relation to 'list'.
 * It scans 'list', then recursively scans every item in 'list'.
 *
 * @param thing the thing to check for
 * @param list the object to scan to see if 'thing' is on its next list
 * @return boolean true if found, false if not
 */
int
member(dbref thing, dbref list)
{
    DOLIST(list, list) {
        if (list == thing)
            return 1;

        if ((CONTENTS(list)) && (member(thing, CONTENTS(list)))) {
            return 1;
        }
    }

    return 0;
}

/**
 * This reverses a dbref linked list
 *
 * Basically it changes all the links around so that what was last will
 * now be first on the list.
 *
 * @param list the list to reverse
 * @return the dbref that is the start of the reversed list
 */
dbref
reverse(dbref list)
{
    dbref newlist;
    dbref rest;

    newlist = NOTHING;
    while (list != NOTHING) {
        rest = NEXTOBJ(list);
        PUSH(list, newlist);
        DBDIRTY(newlist);
        list = rest;
    }

    return newlist;
}

/**
 * Calculate the memory size of an object
 *
 * This is an estimated size of the object as stored on disk.
 * Either that, or it is an inaccurate calculation of memory usage
 * since it doesn't take into account the memory allocated for
 * the 'specific' structures nor the memory allocated for player
 * descriptor lists and ignore cache.
 *
 * Looking at its usage in the code, it appears to be in-memory size,
 * in which case this calculation is inaccurate.
 *
 * Note that the 'load' parameter doesn't have any impact if you are
 * not using DISKBASE.
 *
 * @param i the object to calculate size for
 * @param load boolean if true load props before calculating size.
 * @return the calculated size of the object.
 */
size_t
size_object(dbref i, int load)
{
    size_t byts;
    byts = sizeof(struct object);

    if (NAME(i)) {
        byts += strlen(NAME(i)) + 1;
    }
    byts += size_properties(i, load);

    /*
     * @TODO There's a few missing bytes here.  First off, the
     *       memory allocated for 'specific' structures are not
     *       added up right for players and things.  They are
     *       probably right for rooms and exits.
     *
     *       Further, player descriptor array and ignore cache
     *       are not added up.
     *
     *       Granted, in the end this isn't too many missing bytes,
     *       but may as well improve it because we can.  I did not
     *       dig into if program sizes are calculated correctly
     *
     *       If you fix this, correct the docblock comment both
     *       here and in db.h
     */

    if (Typeof(i) == TYPE_EXIT && DBFETCH(i)->sp.exit.dest) {
        byts += sizeof(dbref) * (size_t)(DBFETCH(i)->sp.exit.ndest);
    } else if (Typeof(i) == TYPE_PLAYER && PLAYER_PASSWORD(i)) {
        byts += strlen(PLAYER_PASSWORD(i)) + 1;
    } else if (Typeof(i) == TYPE_PROGRAM) {
        byts += size_prog(i);
    }

    return byts;
}

/**
 * Validates the player name, as a helper function for ok_object_name.
 *
 * * Names cannot contain nonprintable or space characters.
 * * Names cannot contain the characters (, ), single-quote, or comma.
 * * The name cannot match an existing player.
 * * The name cannot match 'tp_reserved_player_names'
 *
 * This is called from ok_object_name, which also checks acceptance of
 * non-7bit ASCII names.
 *
 * @param name the name to check
 * @return boolean true if name passes the check, false otherwise
 */
static bool
ok_player_name(const char *name)
{
    if (strlen(name) > (unsigned int)tp_player_name_limit)
        return 0;

    for (const char *scan = name; *scan; scan++) {
        if (!(isprint(*scan)
              && !isspace(*scan))
            && *scan != '(' && *scan != ')' && *scan != '\'' && *scan != ',') {
            /* was isgraph(*scan) */
            return 0;
        }
    }

    /* Check the name isn't reserved */
    if (*tp_reserved_player_names
        && equalstr((char *) tp_reserved_player_names, (char *)name))
        return 0;

    /* lookup name to avoid conflicts */
    return (lookup_player(name) == NOTHING);
}

/**
 * Checks to see if 'name' only consists of basic 7-bit ASCII characters.
 *
 * This is specifically ASCII characters of value 127 or less.
 *
 * This does not check for low bytes -- control characters and such.
 *
 * @private
 * @param name the string to iterate over
 * @return boolean true if the check passes, false if the check fails.
 */
static bool
ok_ascii_any(const char *name)
{
    for (const unsigned char *scan = (const unsigned char *) name; *scan; ++scan) {
        if (*scan > 127)
            return 0;
    }
    return 1;
}

/**
 * Validates that the name is appropriate for the given object type.
 *
 * * Names cannot start with !, *, #, or $
 * * Names cannot contain the characters =, &, |, or carriage return/escape.
 * * The name cannot be the strings 'me', 'here', or 'home'
 * * The name cannot match 'tp_reserved_names'
 * * Players must pass ok_player_name.
 * * Things must pass ok_ascii_any if 7bit_thing_names is on.
 * * All other objects must pass ok_ascii_any if 7bit_other_names is on.
 *
 * @see ok_ascii_any
 * @see ok_player_name
 *
 * @param name the name to check
 * @return boolean true if name passes the check, false otherwise
 */
bool
ok_object_name(const char *name, object_flag_type type)
{
    if (!name
        || !*name
        || *name == NOT_TOKEN
        || *name == LOOKUP_TOKEN
        || *name == REGISTERED_TOKEN
        || *name == NUMBER_TOKEN
        || strchr(name, ARG_DELIMITER)
        || strchr(name, AND_TOKEN)
        || strchr(name, OR_TOKEN)
        || strchr(name, '\r')
        || strchr(name, ESCAPE_CHAR)
        || !strcasecmp(name, "me")
        || !strcasecmp(name, "here")
        || !strcasecmp(name, "home")
        || !strcasecmp(name, "nil")
        || (*tp_reserved_names && equalstr((char *) tp_reserved_names, (char *)name)
        ))
        return false;

    switch (type) {
    case TYPE_ROOM:
    case TYPE_EXIT:
    case TYPE_PROGRAM:
        return !tp_7bit_other_names || ok_ascii_any(name);

    case TYPE_THING:
        return !tp_7bit_thing_names || ok_ascii_any(name);

    case TYPE_PLAYER:
        return ok_player_name(name);

    case TYPE_GARBAGE:
        return false;
    }

    return false;
}

/**
 * Determine the parent of an object
 *
 * If object is NOTHING, returns NOTHING.  If it is a THING and a VEHICLE,
 * then it returns the THING's home; if the THING's home is a player, then
 * it returns that player's home.
 *
 * Otherwise, it returns the location of the object.
 *
 * @private
 * @param obj object to get player of
 * @return the dbref of the parent according to the logic above.
 */
static dbref
getparent_logic(dbref obj)
{
    if (obj == NOTHING)
        return NOTHING;

    if (Typeof(obj) == TYPE_THING && (FLAGS(obj) & VEHICLE)) {
        obj = THING_HOME(obj);

        if (obj != NOTHING && Typeof(obj) == TYPE_PLAYER) {
            obj = PLAYER_HOME(obj);
        }
    } else {
        obj = LOCATION(obj);
    }

    return obj;
}

/**
 * Get the parent for a given object
 *
 * This logic is weirdly complicated.  If tp_secure_thing_movement is true,
 * which is to say, moving things is set to act like a player is moving, then
 * this simply returns the location of the object.
 *
 * Otherwise, we iterate.  The complexity is around vehicle things;
 * if a THING is set vehicle, then its parent room is either the vehicle's
 * home, or the vehicle's home is a player, then the player's home.
 *
 * The exact logic here is difficult to compress into a little text blurb,
 * but basically we find the next valid room to be our parent or default
 * to GLOBAL_ENVIRONMENT in a pinch.
 *
 * @param obj the object to get the parent for
 * @return the parent database reference
 */
dbref
getparent(dbref obj)
{
    dbref ptr, oldptr;

    if (tp_secure_thing_movement) {
        obj = LOCATION(obj);
    } else {
        ptr = getparent_logic(obj);

        do {
            obj = getparent_logic(obj);
        } while (obj != (oldptr = ptr = getparent_logic(ptr)) &&
                 obj != (ptr = getparent_logic(ptr)) &&
                 obj != NOTHING && Typeof(obj) == TYPE_THING);

        if (obj != NOTHING && (obj == oldptr || obj == ptr)) {
            obj = GLOBAL_ENVIRONMENT;
        }
    }
    return obj;
}

/**
 * Determine if 'who' controls object 'what'
 *
 * The logic here is relatively simple.  If 'what' is invalid, return
 * false.  If 'who' is a wizard, return true (with GOD_PRIV checking).
 *
 * Next, we check for realms; if a room is set 'W', then the owner of
 * that room controls everything under that room's environment except
 * other players.
 *
 * Then, we check if who owns what.
 *
 * Lastly, we check the own lock.
 *
 * @param who the player we are checking for control
 * @param what the object we are checking to see if player controls
 * @return boolean true if who controls what, false otherwise.
 */
int
controls(dbref who, dbref what)
{
    /* No one controls invalid objects */
    if (!OkObj(what))
        return 0;

    who = OWNER(who);

    /* Wizard controls everything */
    if (Wizard(who)) {
#ifdef GOD_PRIV
        if (tp_strict_god_priv && God(OWNER(what)) && !God(who))
            /* Only God controls God's objects */
            return 0;
        else
#endif
            return 1;
    }

    if (tp_realms_control) {
        /*
         * Realm Owner controls everything under his environment.
         * To set up a Realm, a Wizard sets the W flag on a room.  The
         * owner of that room controls every Room object contained within
         * that room, all the way to the leaves of the tree.
         * -winged
         */
        for (dbref index = what; index != NOTHING; index = LOCATION(index)) {
            if ((OWNER(index) == who) && (Typeof(index) == TYPE_ROOM)
                && Wizard(index)) {
                /* Realm Owner doesn't control other Player objects */
                if (Typeof(what) == TYPE_PLAYER) {
                    return 0;
                } else {
                    return 1;
                }
            }
        }
    }

    /* owners control their own stuff */
    if (who == OWNER(what))
        return 1;

    return (test_lock_false_default(NOTHING, who, what, MESGPROP_OWNLOCK));
}

/**
 * Determine if who controls the ability to manipulate the link of what
 *
 * For unlinking, this determines if 'who' is able to unlink 'what'.
 * For linking, this logic is only applied if 'what' is already
 * determined to be an exit -- if you aren't careful with this, you can
 * allow someone to arbitrarily re-home other players that live in
 * a room that someone owns, for instance.
 *
 * @param who the person doing the link/unlink
 * @param what the object being linked/unlinked
 * @return boolean true if who may link/unlink what
 */
int
controls_link(dbref who, dbref what)
{
    switch (Typeof(what)) {
    case TYPE_EXIT: 
        for (int i = 0, n = DBFETCH(what)->sp.exit.ndest; i < n; i++) {
            if (controls(who, DBFETCH(what)->sp.exit.dest[i]))
                return 1;
        }

        return (who == OWNER(LOCATION(what)));

    case TYPE_ROOM:
        return (controls(who, DBFETCH(what)->sp.room.dropto));

    case TYPE_PLAYER:
        return (controls(who, PLAYER_HOME(what)));

    case TYPE_THING:
        return (controls(who, THING_HOME(what)));

    case TYPE_PROGRAM:
    default:
        return 0;
    }
}

/**
 * Set the source of an action -- basically move 'action' to 'source'
 *
 * This is the proper way to 'move' an action from one location to another,
 * updating the action's location and the source's exit list.
 *
 * This is only for exits; no error checking is done, so be careful with it.
 *
 * If you are moving an exit, you should unset_source(action) first to remove
 * it from its current exit linked list.  If you don't do that, you'll
 * probably corrupt the DB.
 *
 * @see unset_source
 *
 * @param action the action to move
 * @param source the destination to move it to
 */
void
set_source(dbref action, dbref source)
{
    PUSH(action, EXITS(source));
    LOCATION(action) = source;
    DBDIRTY(source);
}

/**
 * Remove the action from its current location's exit list
 *
 * This doesn't alter action's location, so after calling this, the
 * action is in weird limbo.  You should do something with it,
 * such as recycle it or use set_source to give it a new location.
 *
 * @see set_source
 *
 * @param action the action to remove from its location's exist list
 */
void
unset_source(dbref action)
{
    dbref source = LOCATION(action);
    EXITS(source) = remove_first(EXITS(source), action);
    DBDIRTY(source);
    DBDIRTY(action);
}

/**
 * Parse 'dest_name' to see if 'player' can link 'exit' to there.
 *
 * A utility for open and link which checks whether a given destination
 * string is valid.  It returns a parsed dbref on success, and NOTHING
 * on failure.
 *
 * Failures are notified to the player with descriptive messages.
 *
 * @private
 * @param descr the linking player's descriptor
 * @param player the linking player
 * @param exit the exit we are linking
 * @param dest_name the destination to parse
 * @return the destination DBREF or NOTHING on failure.
 */
static dbref
parse_linkable_dest(int descr, dbref player, dbref exit, const char *dest_name)
{
    dbref dobj;                 /* destination room/player/thing/link */
    struct match_data md;

    init_match(descr, player, dest_name, NOTYPE, &md);
    match_everything(&md);
    match_home(&md);
    match_nil(&md);

    if ((dobj = noisy_match_result(&md)) == NOTHING) {
        return NOTHING;
    }

    if (dobj != NIL && !tp_teleport_to_player && Typeof(dobj) == TYPE_PLAYER) {
        char unparse_buf[BUFFER_LEN];
        unparse_object(player, dobj, unparse_buf, sizeof(unparse_buf));
        notifyf(player, "You can't link to players.  Destination %s ignored.",
                unparse_buf);
        return NOTHING;
    }

    if (!can_link(player, exit)) {
        notify(player, "You can't link that.");
        return NOTHING;
    }

    if (!can_link_to(player, Typeof(exit), dobj)) {
        char unparse_buf[BUFFER_LEN];
        unparse_object(player, dobj, unparse_buf, sizeof(unparse_buf));
        notifyf(player, "You can't link to %s.", unparse_buf);
        return NOTHING;
    } else {
        return dobj;
    }
}

/**
 * The underpinnings of link_exit and link_exit_dry
 *
 * This takes player info, an exit, a string containing destinations,
 * a dbref list presumed to be MAX_LINKS in size, and a dry run boolean.
 *
 * It checks to see if exit can be linked to dest_name, which may be
 * a string with multiple destinations delimited by ;
 *
 * If dryrun is 1, exit links will not be set, but any error messaging
 * will be displayed to the user.  If dryrun is 0, links will be made;
 * any failures along the way will display messages but will not stop
 * the overall process.
 *
 * 'dest_list' will contain the linked destinations, and the function
 * will return the total number of links made.  dest_list is populated
 * even if dryrun is 1.
 *
 * @see parse_linkable_dest
 *
 * A lot of different checks are done to make sure the exit doesn't
 * make a loop, permissions are done correctly, and that multiple
 * links are only allowed for EXITS and THINGS.
 *
 * @private
 * @param descr the descriptor of the player
 * @param player the player doing the linking
 * @param exit the exit being linked
 * @param dest_name the destination to target
 * @param dest_list the array of size MAX_LINKS to store linked destinations
 * @param dryrun boolean if true we won't link anything, false we will link
 * @return number of links made - this will be 0 on failure
 */
static int
_link_exit(int descr, dbref player, dbref exit, char *dest_name,
           dbref * dest_list, int dryrun)
{
    char *p, *q;
    int prdest;
    dbref dest;
    int ndest, error;
    char qbuf[BUFFER_LEN];
    char unparse_buf[BUFFER_LEN];

    prdest = 0;
    ndest = 0;
    error = 0;

    /*
     * We iterate over dest_name because there may be multiple destinations.
     * Each iteration of this loop will be a single potential destination.
     */
    while (*dest_name) {
        skip_whitespace((const char **)&dest_name);
        p = dest_name;

        while (*dest_name && (*dest_name != EXIT_DELIMITER))
            dest_name++;

        q = (char *) strncpy(qbuf, p, BUFFER_LEN);  /* copy word */
        q[(dest_name - p)] = '\0';  /* terminate it */

        if (*dest_name) {
            dest_name++;
            skip_whitespace((const char **)&dest_name);
        }

        /*
         * This one takes the string and tries to parse it into a linkable
         * destination, with proper permissions.  It will emit errors to
         * the player if applicable.
         */
        if ((dest = parse_linkable_dest(descr, player, exit, q)) == NOTHING)
            continue;

        if (dest == NIL) {
            if (!dryrun) {
                notify(player, "Linked to NIL.");
            }

            dest_list[ndest++] = dest;
            continue;
        }

        unparse_object(player, dest, unparse_buf, sizeof(unparse_buf));

        switch (Typeof(dest)) {
            case TYPE_PLAYER:
            case TYPE_ROOM:
            case TYPE_PROGRAM:
                if (prdest) {
                    notifyf(player,
                            "Only one player, room, or program destination allowed. Destination %s ignored.",
                            unparse_buf);

                    if (dryrun)
                        error = 1;

                    continue;
                }

                dest_list[ndest++] = dest;
                prdest = 1;
                break;

            case TYPE_THING:
                dest_list[ndest++] = dest;
                break;

            case TYPE_EXIT:
                if (exit_loop_check(exit, dest)) {
                    notifyf(player, "Destination %s would create a loop, ignored.",
                            unparse_buf);

                    if (dryrun)
                        error = 1;

                    continue;
                }

                dest_list[ndest++] = dest;
                break;

            default:
                notify(player, "Internal error: weird object type.");
                log_status("PANIC: weird object: Typeof(%d) = %d", dest, Typeof(dest));

                if (dryrun)
                    error = 1;

                break;
        }

        if (!dryrun) {
            if (dest == HOME) {
                notify(player, "Linked to HOME.");
            } else {
                notifyf(player, "Linked to %s.", unparse_buf);
            }
        }

        if (ndest >= MAX_LINKS) {
            notify(player, "Too many destinations, rest ignored.");

            if (dryrun)
                error = 1;

            break;
        }
    }

    if (dryrun && error)
        return 0;

    return ndest;
}

/**
 * This routine connects an exit to (potentially) a bunch of destinations.
 *
 * 'exit' is the the exit whose destinations are to be linked.
 * 'dest_name' is a character string containing the list of destinations
 * 'dest_list' is an array of dbref's where the valid destinations are
 * stored.  It is assumed to be MAX_LINKS in size.
 *
 * The destinations are delimited by ; if doing multiple.
 *
 * A lot of different checks are done to make sure the exit doesn't
 * make a loop, permissions are done correctly, and that multiple
 * links are only allowed for EXITS and THINGS.
 *
 * @param descr the descriptor of the player
 * @param player the player doing the linkage
 * @param exit the ref of the exit we are linking
 * @param dest_name the destination or destinations
 * @param dest_list the array to store ref dbrefs, MAX_LINKS in size
 * @return the count of destinations linked.
 */
int
link_exit(int descr, dbref player, dbref exit, char *dest_name,
          dbref * dest_list)
{
    return _link_exit(descr, player, exit, dest_name, dest_list, 0);
}

/**
 * A "dry run" version of link_exit to check if we can do the links
 *
 * The underpinnings of this are identical to link_exits.  However,
 * no linking is done.  Instead, if there would have been errors,
 * this will display error messages and return 0.  If it would have
 * been successful, this will return the number of links that
 * would have been made and dest_list will be populated.
 *
 * For more documentation and deatils, @see link_exit
 *
 * @param descr the descriptor of the player
 * @param player the player doing the linkage
 * @param exit the ref of the exit we are linking
 * @param dest_name the destination or destinations
 * @param dest_list the array to store ref dbrefs, MAX_LINKS in size
 * @return the count of destinations linked.
 */
int
link_exit_dry(int descr, dbref player, dbref exit, char *dest_name,
              dbref * dest_list)
{
    return _link_exit(descr, player, exit, dest_name, dest_list, 1);
}

/**
 * Register an object ref on the given location in the given propdir
 *
 * 'name' will be concatenated onto propdir with a / delimiter.
 * propdir is usually either _reg or a propqueue.  The object dbref is
 * stored as a ref-style property on the object.
 *
 * Displays appropriate messages; if the reference previously existed,
 * its old value is displayed.
 *
 * If object is NOTHING, then the property is cleared.  Permissions
 * are not checked.
 *
 * @param player the player doing the registration
 * @param location the location to do the registration on
 * @param propdir the prop dir to register into
 * @param name the registration name
 * @param object the object to register
 */
void
register_object(dbref player, dbref location, const char *propdir, char *name,
                dbref object)
{
    PData mydat;
    PropPtr p;
    dbref prevobj = -50;
    char buf[BUFFER_LEN];
    char unparse_buf[BUFFER_LEN], unparse_buf2[BUFFER_LEN];
    char *strval;

    if (!is_valid_propname(name)) {
        notifyf_nolisten(player, "Registry name '%s' is not valid", name);
        return;
    }

    snprintf(buf, sizeof(buf), "%s/%s", propdir, name);

    if ((p = get_property(location, buf))) {
#ifdef DISKBASE
        propfetch(location, p);
#endif
        switch (PropType(p)) {
            case PROP_STRTYP:
                strval = PropDataStr(p);

                if (*strval == NUMBER_TOKEN)
                    strval++;

                if (number(strval)) {
                    prevobj = atoi(strval);
                } else {
                    prevobj = AMBIGUOUS;
                }

                break;

            case PROP_INTTYP:
                prevobj = PropDataVal(p);
                break;

            case PROP_REFTYP:
                prevobj = PropDataRef(p);
                break;

            default:
                break;
        }

        if (prevobj != -50) {
            unparse_object(player, prevobj, unparse_buf, sizeof(unparse_buf));
            notifyf_nolisten(player, "Used to be registered as %s: %s", buf, unparse_buf);
        }
    } else if (object == NOTHING) {
        notifyf_nolisten(player, "Nothing to remove.");
        return;
    }

    unparse_object(player, object, unparse_buf, sizeof(unparse_buf));
    unparse_object(player, location, unparse_buf2, sizeof(unparse_buf2));

    if (object == NOTHING) {
        remove_property(location, buf, 0);
        notifyf_nolisten(player, "Registry entry on %s removed.", unparse_buf2);
    } else {
        mydat.flags = PROP_REFTYP;
        mydat.data.ref = object;
        set_property(location, buf, &mydat, 0);
        notifyf_nolisten(player, "Now registered as %s: %s on %s", buf, unparse_buf, unparse_buf2);
    }
}

/**
 * Calculate the environmental distance between 'from' and 'to'.
 *
 * This is used in matching to try and find what potential match is
 * more relevant.  The parent of 'to' is found, and then we find the
 * number of parents between 'from' and 'to'.
 *
 * If from is the parent of to, this will return 0.  Otherwise, it
 * returns the number of hops between 'from' and 'to's parent; if
 * 'from' isn't a descendant of 'to', then the returned number will
 * be the number of hops from 'from' to #0.
 *
 * @see getparent
 *
 * @param from the starting reference
 * @param to the destination reference
 * @return the number of hops between 'from' and 'to's parent.
 */
int
env_distance(dbref from, dbref to)
{
    int distance = 0;
    dbref dest = getparent(to);

    if (from == dest)
        return 0;

    do {
        distance++;
    } while ((from = getparent(from)) != dest && from != NOTHING);

    return distance;
}
