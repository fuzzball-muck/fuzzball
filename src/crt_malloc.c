/** @file crt_malloc.c
 *
 * crt_malloc.c -- CrT's own silly malloc wrappers for debugging.
 *
 * To enable this, MALLOC_PROFILING must be defined.
 *
 * Can be used to generate malloc statistic listing looking like:

         File Line CurBlks CurBytes MaxBlks MaxBytes MxByTime TotBlks TotBytes
------------- ---- ------- -------- ------- -------- -------- ------- --------
         db.c  377:      0        0       2      197 06232835     366     7174
         db.c 1316:      0        0       1       33 06232836       1       33
         db.c 1376:      0        0       1       17 06232900       1       17
         db.c  873:      0        0    2554    30648 06232919    4954    59448
         db.c  899:      0        0    2554    72068 06232919    4954   131094
    compile.c  242:      0        0      50     1297 06232909     517    11317
    compile.c  503:      0        0       1      106 06232919    4938   130373
    compile.c  542:      0        0       5       41 06232912   12155    67649
    compile.c 1768:      0        0    7443   148860 06232924   13703   274060
    compile.c 1326:      0        0     181     2172 06232924     342     4104
    compile.c 1327:      0        0     181     2438 06232924     342     4357
    compile.c 1339:      0        0       5       60 06232923     577     6924
    compile.c  847:      0        0       1       97 06232909    1493    28733
       edit.c  168:      0        0       1       79 06232908     184     3844
    compile.c  575:      0        0       2      177 06232921     795    33994
    compile.c 1365:      0        0       2       24 06232914      14      168
    compile.c 1378:      0        0       2       24 06232914      15      180
    compile.c  220:      0        0       1       49 06232910     612     9619
    compile.c 1352:      0        0       4       48 06232911     233     2796
     interp.c  155:      0        0       1      432 06232900      11     4752
  interface.c 1208:      0        0       1     1024 06232907       4     4096
     interp.c  578:      0        0       2      864 06232913       9     3888
    compile.c 1532:      0        0       4       38 06232913       6       56
  interface.c 1008:     27     2012      27     2012 06233122      79     5060
  timequeue.c   83:      1       14       3       42 06232900       4       56
  interface.c  961:      1       14       1       14 06232907       1       14
       game.c  435:      1       15       1       15 06232835       1       15
        reg.c  352:      1       16       1       16 06232835       1       16
       game.c  466:      1       16       1       16 06232900       1       16
  timequeue.c   82:      1       17       3       25 06232900       4       41
    boolexp.c   95:      1       20       1       20 06232907       1       20
  interface.c 1007:     35      560      35      560 06233122      87     1392
    compile.c 1125:      4       47       4       47 06232914       4       47
    compile.c 1123:      4       48       4       48 06232914       4       48
  interface.c  940:      1       92       1       92 06232907       1       92
  timequeue.c   72:      3      168       3      168 06232900       3      168
    compile.c 1138:     22      264      22      264 06232914      22      264
    compile.c 1141:     22      340      22      340 06232914      22      340
       edit.c  183:    183     1605     183     1605 06232835     183     1605
       edit.c  176:    183     3660     183     3660 06232835     183     3660
       edit.c  184:    183     5569     183     5569 06232835     183     5569
     interp.c  127:      1    13200       1    13200 06232900       1    13200
   property.c  124:   1493    30167    1493    30167 06232926    1494    30181
         db.c 1256:   5440    38387    5440    38387 06232900    5440    38387
    hashtab.c  102:   5711    42144    5761    42602 06232912    6228    46528
 stringutil.c  386:   1759    50761    2768    82507 06232924    3667   104609
      props.c  154:   1643    56120    1643    56120 06232926    1643    56120
    hashtab.c   96:   5711    68532    5761    69132 06232909    6228    74736
         db.c 1246:  22861    91484   22861    91484 06232900   22861    91484
    compile.c 1721:     16   109752      16   109752 06232924      16   109752
         db.c 1129:  56911   793132   56911   793132 06232900   56911   793132
         db.c  167:      1  9420800       1  9420800 06232836       1  9420800
 Cumulative totals: 102221 10728956                            151500 11586028

 * First two columns are source file and line number of a [cm]alloc() call;
 * 
 * 'CurBlks' is number of ram blocks currently in existence
 *      created by that call;
 *       File Line CurBlks CurBytes MaxBlks MaxBytes MxByTime TotBlks TotBytes
 * 'CurBytes' is number of ram bytes current in existence
 *      created by that call;
 * 'TotBlks' is total number of ram blocks ever created by that call;
 * 'TotBytes' is total number of ram bytes ever allocated by that call.
 * 'MaxBytes' is max value 'CurBytes' has ever had for that call.
 * 'MxByTime' is time 'MaxBytes' value was recorded, as day:hour:minute.
 * 
 * The code adds 8 bytes overhead per ram block allocated.
 *
 * In addition, by defining MALLOC_PROFILING_EXTRA to be TRUE, this file
 * can be configured to check for heap-trashing bugs, free()s of
 * unallocated memory, and so forth.  As configured, (CRT_NEW_TO_CHECK
 * and CRT_OLD_TO_CHECK both set to 128) this option takes ABOUT 10x
 * MORE CPU TIME THAN VANILLA FUZZBALL.  You may reduce these numbers
 * to reduce the CPU overhead, at the cost of decreased likelihood of
 * detecting heap-trashing bugs promptly.  This option adds an additional
 * 13 bytes of overhead per string allocated.
 *
 * Together, the above two options can DOUBLE FUZZBALL'S RAM CONSUMPTION.
 */

#include "config.h"

#ifdef MALLOC_PROFILING
#include "fbtime.h"
#include "inst.h"
#include "interface.h"

/* #defines you might want to configure. */

/*
 * Select whether to do debug checks also.  These checks eat an additional 8
 * bytes per allocated block, but can be some help in tracking down obscure
 * memory trashing pointer bugs.
 */
/* #define MALLOC_PROFILING_EXTRA          */

/*
 * When debugging is selected, we check the last CRT_NEW_TO_CHECK blocks
 * allocated each time we are called, on the theory that they are the most
 * likely to have been trashed by buggy code:
 */
#ifndef CRT_NEW_TO_CHECK
#define CRT_NEW_TO_CHECK (128)  /* Make it a nonzero power of two.  */
#endif

/*
 * When debugging is selected we also check CRT_OLD_TO_CHECK blocks on the
 * used-ram cycling through all allocated blocks in time:
 */
#ifndef CRT_OLD_TO_CHECK
#define CRT_OLD_TO_CHECK (128)
#endif

/*
 * When MALLOC_PROFILING_EXTRA is true, we add the following value to the
 * end of block, so we can later check to see if it got overwritten by
 * something:
 */
#ifndef CRT_MAGIC
#define CRT_MAGIC ((char)231)
#endif

/*
 * Something we write into the magic byte of a block just before free()ing it,
 * so as to maybe diagnose repeated free()s:
 */
#ifndef CRT_FREE_MAGIC
#define CRT_FREE_MAGIC ((char)~CRT_MAGIC)
#endif

/*
 * block_list, a list of all malloc/free/etc calls in host program.
 *
 * Central data structure: a blocklist with one entry for each textually
 * distinct [mc]alloc() call:
 */
struct CrT_block_rec {
    const char *file;       /* File name where call is sourced  */
    int line;               /* Line where call is sourced       */

    long tot_bytes_alloc;   /* Total number of bytes allocated  */
    long tot_allocs_done;   /* Total number of blocks allocated */
    long live_blocks;       /* Currently live blocks            */
    long live_bytes;        /* Currently live bytes             */
    long max_blocks;        /* Maximum blocks                   */
    long max_bytes;         /* Maximum bytes                    */
    time_t max_bytes_time;  /* Maximum bytes by time            */

    struct CrT_block_rec *next; /* Linked list implementation   */
};
typedef struct CrT_block_rec A_Block;
typedef struct CrT_block_rec *Block;

/**
 * @private
 * @var the master list of allocated blocks
 */
static Block block_list = NULL;

/* Header, a header we add to each block allocated */

struct CrT_header_rec {
    Block b;                        /* Pointer to block information record */
    size_t size;                    /* Size of the memory block            */
#ifdef MALLOC_PROFILING_EXTRA
    /*
     * There is a concept of a "root" linked list which is used to keep
     * track of all memory allocations.  This linked list is used for that
     * root list -- and is only used when MALLOC_PROFILING_EXTRA is on.
     */
    struct CrT_header_rec *next;
    struct CrT_header_rec *prev;
    char *end;                      /* Pointer to end of memory block      */
#endif
};

typedef struct CrT_header_rec A_Header;
typedef struct CrT_header_rec *Header;

/* Globals supporting debug functionality. */

#ifdef MALLOC_PROFILING_EXTRA

/**
 * @private
 * @var the root block's header information for the memory allocation linked
 *      list
 */
static A_Block Root_Owner = {

    __FILE__,       /* file */
    __LINE__,       /* line */

    0,              /* tot_bytes_alloc */
    0,              /* tot_allocs_done */
    0,              /* live_blocks     */
    0,              /* live_bytes      */
    0,              /* max_blocks      */
    0,              /* max_bytes       */
    0,              /* max_bytes_time  */

    NULL            /* next            */
};

/**
 * @private
 * @var the 'end' field in a header has to be a memory location, so this
 *      is used for that memory location to fill that requirement.
 */
static char root_end = CRT_MAGIC;

/**
 * @private
 * @var our linked list master header for tracking all memory blocks
 */
static A_Header root = {
    &Root_Owner,    /* b    */
    0,              /* size */
    &root,          /* next */
    &root,          /* prev */
    &root_end,      /* end  */
};

/**
 * @private
 * @var Clockhand that we run circularly around allocated-block list,
 *      looking for problems:
 */
static Header rover = &root;


/**
 * Insert a block into 'root' linklist
 *
 * @private
 * @param m the block to insert
 */
static inline void
insert_block(Header m)
{
    root.next->prev = m;
    m->next =  root.next;
    m->prev = &root;
    root. next = m;
}

/**
 * Remove a block from 'root' linklist
 *
 * @private
 * @param m the block to remove
 */
static inline void
remove_block(Header m)
{
    m->next->prev = m->prev;
    m->prev->next = m->next;
}

/**
 * @private
 * @var An array tracking last CRT_NEW_TO_CHECK blocks touched
 */
static Header just_touched[CRT_NEW_TO_CHECK] = {
    /*
     * I can't think of an elegant way of generating the right number of
     * initializers automatically via cpp :( so:
     */
    &root, &root, &root, &root, &root, &root, &root, &root,
    &root, &root, &root, &root, &root, &root, &root, &root,
    &root, &root, &root, &root, &root, &root, &root, &root,
    &root, &root, &root, &root, &root, &root, &root, &root,

    &root, &root, &root, &root, &root, &root, &root, &root,
    &root, &root, &root, &root, &root, &root, &root, &root,
    &root, &root, &root, &root, &root, &root, &root, &root,
    &root, &root, &root, &root, &root, &root, &root, &root,

    &root, &root, &root, &root, &root, &root, &root, &root,
    &root, &root, &root, &root, &root, &root, &root, &root,
    &root, &root, &root, &root, &root, &root, &root, &root,
    &root, &root, &root, &root, &root, &root, &root, &root,

    &root, &root, &root, &root, &root, &root, &root, &root,
    &root, &root, &root, &root, &root, &root, &root, &root,
    &root, &root, &root, &root, &root, &root, &root, &root,
    &root, &root, &root, &root, &root, &root, &root, &root,
};

/**
 * @private
 * @var keep track of the index in just_touched
 */
static int next_touched = 0;

#endif

/* Random internal macro stuff */

/*
 * Undefine the macros which make the rest of the system call our wrappers
 * instead of calling malloc &Co directly:
 */
#undef malloc
#undef calloc
#undef realloc
#undef free

/* Number of bytes of overhead we add to a block: */
#ifdef MALLOC_PROFILING_EXTRA
#define CRT_OVERHEAD_BYTES (sizeof(A_Header) +1)
#else
#define CRT_OVERHEAD_BYTES (sizeof(A_Header)   )
#endif

/*
 * Some functions to implement a simple block_list merge-sort.
 *
 * Currently, sorts on live_bytes field.
 */

/**
 * Count number of blocks in a blocklist
 *
 * @private
 * @param b the block to generate a count for
 * @return the count of blocks in the blocklist
 */
static int
block_count(Block b)
{
    int i;

    for (i = 0; b; b = b->next)
        ++i;

    return i;
}

/**
 * Return nth block in a blocklist.
 *
 * The first block is 0. This does NOT check to see if the 'n' overruns
 * our list (in other words, 'n' is greater than the number of items in
 * the list).  The way this is used at present, an overrun will never
 * happen.
 *
 * @private
 * @param b the block list to retrieve from
 * @param n the block number to retreive
 * @return the block we requested
 */
static Block
blocks_nth(Block b, int n)
{
    for (int i = 0; i < n; b = b->next)
        ++i;

    return b;
}

/**
 * Split blocklist 'b' into two lists, return 2nd list
 *
 * No checking is done to make sure 'n' is a valid position in the list.
 * However, how this call is used currently will not cause errors; however
 * any future users should be aware of this.
 *
 * @see blocks_nth
 *
 * @private
 * @param b the blocklist to split
 * @param n the place to split the list
 * @return the newly split second list
 */
static Block
blocks_split(Block b, int n)
{
    Block tail = blocks_nth(b, n - 1);
    Block head = tail->next;

    tail->next = NULL;
    return head;
}

/**
 * Merge two sorted lists into one sorted list.
 *
 * This merges the lists, sorting by live_bytes in ascending order.
 * The linked lists b0 and b1 are mutated by this call; b0 and b1 will
 * point to valid memory locations but their list positions could be
 * different.
 *
 * @private
 * @param b0 the first blocklist to merge
 * @param b1 the second blocklist to merge
 * @return the merged blocklist
 */
static Block
blocks_merge(Block b0, Block b1)
{
    A_Block head;
    Block tail = &head;

    while (b0 || b1) {
        Block nxt;

        if (!b1) {
            nxt = b0;
            b0 = b0->next;
        } else if (!b0) {
            nxt = b1;
            b1 = b1->next;
        } else if (b0->live_bytes < b1->live_bytes) {
            nxt = b0;
            b0 = b0->next;
        } else {
            nxt = b1;
            b1 = b1->next;
        }

        tail->next = nxt;
        tail = nxt;
    }

    tail->next = NULL;

    return head.next;
}

/**
 * Sort blocklist on 'live_bytes' (ascending)
 *
 * @see blocks_merge
 *
 * Note that 'b1' will likely be mutated by this call unless there is only
 * a single item or if it is already sorted.  You should use the returned
 * value instead rather than continue to us what you pass into this function.
 *
 * @private
 * @param b1 the blocklist to sort
 * @return the sorted blocklist
 */
static Block
blocks_sort(Block b1)
{
    int len = block_count(b1);

    if (len < 2)
        return b1;

    {
        Block b2 = blocks_split(b1, len / 2);

        return blocks_merge(blocks_sort(b1), blocks_sort(b2));
    }
}

/**
 * Alloc block for given file and line
 *
 * This is meant to be used with __FILE__ and __LINE__.  The 'file' string
 * is NOT copied so that memory must not be free'd.  If you use __FILE__,
 * this will work fine since that string is a constant.
 *
 * @private
 * @param file the file we are allocating a block for.
 * @param line the line we are allocating the block for.
 * @return the allocated block
 */
static Block
block_alloc(const char *file, int line)
{
    Block b = calloc(1, sizeof(A_Block));

    b->file = file;
    b->line = line;
    b->next = block_list;

    return b;
}

/**
 * Find block for file/line, creating the block if it doesn't already exist
 *
 * @private
 * @param file the file to find a block for
 * @param line the line to find a block for
 * @return the found block, or a new block for the file/line
 */
static Block
block_find(const char *file, int line)
{
    /*
     * @TODO This is super inefficient.  We're scanning a giant dumpster of
     *       lines and files.  This could be done a lot better.  However,
     *       this code probably isn't worth improving
     */
    for (Block b = block_list; b; b = b->next) {
        if (b->line == line && !strcmp(b->file, file)) {
            return b;
        }
    }

    return block_alloc(file, line);
}

/* Report generation functions: */


/**
 * Return ascii time string in static buffer
 *
 * You should not free the memory returned by this.  And of course, this is
 * not threadsafe and theoretically the buffer can mutate on you.
 *
 * @private
 * @param when the time stamp to generate a time string for.
 * @return a time string in a static buffer.
 */
static const char *
CrT_timestr(time_t when)
{
    static char buf[20];
    struct tm *da_time = localtime(&when);

    snprintf(buf, sizeof(buf), "%02d%02d%02d%02d",
             da_time->tm_mday, da_time->tm_hour, da_time->tm_min,
             da_time->tm_sec);
    return buf;
}

/**
 * @private
 * @var a global to keep track of the player to send summary information to.
 *      We use this because of the funky callback system; there's no good way
 *      to pass this around as-written.
 */
static dbref summarize_player;

/**
 * Summarize ram usage statistics.
 *
 * Takes a callback function that takes a character string.  Each line of
 * the summary is fed to the callback which can be used to do the proper
 * display of the said line.
 *
 * @private
 * @param fn the callback, which should take a single string parameter.
 */
static void
summarize(void (*fn) (char *))
{
    int sum_blks = 0;
    int sum_byts = 0;
    int sum_totblks = 0;
    int sum_totbyts = 0;

    char buf[256];

    block_list = blocks_sort(block_list);

#undef  X
#define X(t) (*fn)(t);

    X("         File Line CurBlks CurBytes MaxBlks MaxBytes MxByTime TotBlks TotBytes");
    X("------------- ---- ------- -------- ------- -------- -------- ------- --------")

    for (Block b = block_list; b; b = b->next) {
        sum_blks += b->live_blocks;
        sum_byts += b->live_bytes;

        sum_totblks += b->tot_allocs_done;
        sum_totbyts += b->tot_bytes_alloc;

        snprintf(buf, sizeof(buf),
                 "%13s%5d:%7ld %8ld %7ld %8ld %8s %7ld %8ld",
                 b->file,
                 b->line,
                 b->live_blocks,
                 b->live_bytes,
                 b->max_blocks,
                 b->max_bytes,
                 CrT_timestr(b->max_bytes_time),
                 b->tot_allocs_done,
                 b->tot_bytes_alloc);
        X(buf);
    }

    snprintf(buf, sizeof(buf), " Cumulative totals:%7d %8d                           %7d %8d",
             sum_blks, sum_byts, sum_totblks, sum_totbyts);
    X(buf);
}

/**
 * This is a callback for summarize to notify the 'summarize_player' dbref
 *
 * Uses the summarize_player global
 *
 * @see summarize
 *
 * @private
 * @param t the string to notify the player
 */
static void
summarize_notify(char *t)
{
    notify(summarize_player, t);
}

/**
 * Send memory profile summary to a given player
 *
 * @param player the dbref of the player to notify
 */
void
CrT_summarize(dbref player)
{
    summarize_player = player;
    summarize(summarize_notify);
}

/**
 * @private
 * @var global for keeping track of the file handle between calls of the
 *      callback.  This is not passed into summarize_to_file because of
 *      how the callback works..
 */
static FILE *summarize_fd;

/**
 * Send a string to our summary file.  This is callback for summarize
 *
 * @see summarize
 *
 * @private
 * @param t the string to send to the file.
 */
static void
summarize_to_file(char *t)
{
    fprintf(summarize_fd, "%s\n", t);
}

/**
 * Summarize ram usage and write it to a file
 *
 * @private
 * @param file the file name to write to.
 * @param comment a comment to put at the top of the file, or NULL for none.
 */
void
CrT_summarize_to_file(const char *file, const char *comment)
{
    if ((summarize_fd = fopen(file, "ab")) == 0)
        return;

    if (comment && *comment) {
        fprintf(summarize_fd, "%s\n", comment);
    }

    {
        time_t lt = time(NULL);
        char buf[30];

        strftime(buf, sizeof(buf), "%a %b %d %T %Z %Y", localtime(&lt));
        fprintf(summarize_fd, "%s\n", buf);
    }

    summarize(summarize_to_file);

    fclose(summarize_fd);
}

#ifdef MALLOC_PROFILING_EXTRA

/* Debug support functions: */

/**
 * Crashes the server and generates a mesage that isn't displayed anywhere
 *
 * @private
 * @param err the error message which isn't displayed.
 * @param m the header for the faulty block.
 * @param file the file where the fault happened.
 * @param line the line where the fault happened.
 */
static void
crash(char *err, Header m, const char *file, int line)
{
    char buf[256];

    snprintf(buf, sizeof(buf), "Err found at %s:%d in block from %s:%d", file,
             line, m->b->file, m->b->line);

    panic(buf);
}

/**
 * Check the valididty of a given block
 *
 * @private
 * @param m the block to check
 * @param file the file that originated this check
 * @param line the line that originated this check
 */
static void
check_block(Header m, const char *file, int line)
{
    if (*m->end != CRT_MAGIC)
        crash("Bottom overwritten", m, file, line);

    if (m->next->prev != m)
        crash("next->prev != m", m, file, line);

    if (m->prev->next != m)
        crash("prev->next != m", m, file, line);
}

/**
 * This checks 'old' blocks looking for integrity issues
 *
 * It uses the 'rover' global to keep track of where we left off.
 * We check CRT_OLD_TO_CHECK entries at a time.
 *
 * @private
 * @param file the file that instigated the check
 * @param line the line that instigated the check
 */
static void
check_old_blocks(const char *file, int line)
{
    int i = CRT_OLD_TO_CHECK;

    while (i-- > 0) {
        check_block(rover, file, line);
        rover = rover->next;
    }
}

/**
 * This checks 'new' blocks looking for integrity issues.
 *
 * These are recently touched blocks.  We will check CRT_NEW_TO_CHECK
 * entries at a time.
 *
 * @private
 * @param file the file that instigated the check
 * @param line the line that instigated the check
 */
static void
check_new_blocks(const char *file, int line)
{
    int i = CRT_NEW_TO_CHECK;

    while (i-- > 0)
        check_block(just_touched[i], file, line);
}

/**
 * Abort the program if we can find any trashed malloc blocks.
 *
 * @private
 * @param file the file that instigated the check
 * @param line the line that instigated the check
 */
static void
CrT_check(const char *file, int line)
{
    check_old_blocks(file, line);
    check_new_blocks(file, line);
}

#endif /* MALLOC_PROFILING_EXTRA */

/* The actual wrappers for malloc/calloc/realloc/free: */

/**
 * This is the wrapper for malloc.
 *
 * It works just like malloc except it takes a file and line argument pair
 * in order to track where the call came from.  __FILE__ and __LINE__ should
 * be used.  The memory passed into 'file' must not be deleted; a copy is
 * not made.
 *
 * @param size the size of the memory block to allocate
 * @param file the file the originated this request
 * @param line the line that originaed this request
 * @return allocated and tracked memory block
 */
void *
CrT_malloc(size_t size, const char *file, int line)
{
    Block b = block_find(file, line);

    Header m = malloc(size + CRT_OVERHEAD_BYTES);

    if (!m) {
        fprintf(stderr, "CrT_malloc(): Out of Memory!\n");
        abort();
    }

    m->b = b;
    m->size = size;

#ifdef MALLOC_PROFILING_EXTRA
    /* Look around for trashed ram blocks: */
    CrT_check(file, line);

    /* Remember where end of block is: */
    m->end = &((char *) m)[size + (CRT_OVERHEAD_BYTES - 1)];

    /* Write a sacred value at end of block: */
    *m->end = CRT_MAGIC;

    /* Thread m into linklist of allocated blocks: */
    insert_block(m);

    /* Remember we've touched 'm' recently: */
    just_touched[++next_touched & (CRT_NEW_TO_CHECK - 1)] = m;

#endif

    b->tot_bytes_alloc += size;
    b->tot_allocs_done++;
    b->live_blocks++;
    b->live_bytes += size;

    if (b->live_bytes > b->max_bytes) {
        b->max_bytes = b->live_bytes;
        b->max_bytes_time = time(NULL);
    }

    if (b->live_blocks > b->max_blocks)
        b->max_blocks = b->live_blocks;

    return (void *) (m + 1);
}

/**
 * This is the wrapper for calloc
 *
 * It works just like calloc, except it takes a file and line for tracking
 * purposes.  File should usually be __FILE__ and line __LINE__.  The
 * memory for 'file' is not copied and therefore must not be deallocated.
 *
 * @param num the number of elements to allocate
 * @param siz the size of the elements to allocate
 * @param file the originating file
 * @param line the originating line
 * @return allocated and tracked memory block
 */
void *
CrT_calloc(size_t num, size_t siz, const char *file, int line)
{
    size_t size = siz * num;
    Block b = block_find(file, line);
    Header m = calloc(size + CRT_OVERHEAD_BYTES, 1);

    if (!m) {
        fprintf(stderr, "CrT_calloc(): Out of Memory!\n");
        abort();
    }

    m->b = b;
    m->size = size;

#ifdef MALLOC_PROFILING_EXTRA

    /* Look around for trashed ram blocks: */
    CrT_check(file, line);

    /* Remember where end of block is: */
    m->end = &((char *) m)[size + (CRT_OVERHEAD_BYTES - 1)];

    /* Write a sacred value at end of block: */
    *m->end = CRT_MAGIC;

    /* Thread m into linklist of allocated blocks: */
    insert_block(m);

    /* Remember we've touched 'm' recently: */
    just_touched[++next_touched & (CRT_NEW_TO_CHECK - 1)] = m;

#endif

    b->tot_bytes_alloc += size;
    b->tot_allocs_done++;
    b->live_blocks++;
    b->live_bytes += size;

    if (b->live_bytes > b->max_bytes) {
        b->max_bytes = b->live_bytes;
        b->max_bytes_time = time(NULL);
    }

    if (b->live_blocks > b->max_blocks)
        b->max_blocks = b->live_blocks;

    return (void *) (m + 1);
}

/**
 * This is the wrapper for realloc
 *
 * It works just like realloc, except it takes a file and line for tracking
 * purposes.  File should usually be __FILE__ and line __LINE__.  The
 * memory for 'file' is not copied and therefore must not be deallocated.
 *
 * @param p the memory block to resize
 * @param size the new size
 * @param file the originating file
 * @param line the originating line
 * @return allocated and tracked memory block
 */
void *
CrT_realloc(void *p, size_t size, const char *file, int line)
{
    Header m = ((Header) p) - 1;
    Block b = m->b;

#ifdef MALLOC_PROFILING_EXTRA
    /* Look around for trashed ram blocks: */
    check_block(m, __FILE__, __LINE__);
    CrT_check(file, line);

    /* Don't clobber 'rover': */
    if (rover == m) {
        rover = rover->next;
    }

    /* Remove m from linklist of allocated blocks: */
    remove_block(m);

    /* Remove m from just_touched[], if present: */
    {
        int i = CRT_NEW_TO_CHECK;

        while (i-- > 0) {
            if (just_touched[i] == m) {
                just_touched[i] = &root;
            }
        }
    }
#endif

    m = realloc(m, size + CRT_OVERHEAD_BYTES);

    if (!m) {
        fprintf(stderr, "CrT_realloc(): Out of Memory!\n");
        abort();
    }

    b->live_bytes -= m->size;
    b->live_bytes += size;

    m->size = size;

#ifdef MALLOC_PROFILING_EXTRA

    /* Remember where end of block is: */
    m->end = &((char *) m)[size + (CRT_OVERHEAD_BYTES - 1)];

    /* Write a sacred value at end of block: */
    *m->end = CRT_MAGIC;

    /* Thread m into linklist of allocated blocks: */
    insert_block(m);

    /* Remember we've touched 'm' recently: */
    just_touched[++next_touched & (CRT_NEW_TO_CHECK - 1)] = m;

#endif

    if (b->live_bytes > b->max_bytes) {
        b->max_bytes = b->live_bytes;
        b->max_bytes_time = time(NULL);
    }

    return (void *) (m + 1);
}


/**
 * This is the wrapper for free
 *
 * It works just like free, except it takes a file and line for tracking
 * purposes.  File should usually be __FILE__ and line __LINE__.  The
 * memory for 'file' is not copied and therefore must not be deallocated.
 *
 * @param p the memory block to free
 * @param file the originating file
 * @param line the originating line
 */
void
CrT_free(void *p, const char *file, int line)
{
    Header m = ((Header) p) - 1;
    Block b = m->b;

#ifdef MALLOC_PROFILING_EXTRA
    /* Look around for trashed ram blocks: */
    if (*m->end == CRT_FREE_MAGIC)
        crash("Duplicate free()", m, file, line);

    check_block(m, __FILE__, __LINE__);
    CrT_check(file, line);

    /* Don't clobber 'rover': */
    if (rover == m) {
        rover = rover->next;
    }

    /* Remove m from linklist of allocated blocks: */
    remove_block(m);

    /* Remove m from just_touched[], if present: */
    {
        int i = CRT_NEW_TO_CHECK;

        while (i-- > 0) {
            if (just_touched[i] == m) {
                just_touched[i] = &root;
            }
        }
    }

    /*
     * Mark m so as to give some chance of diagnosing repeat free()s of same
     * block:
     */
    *m->end = CRT_FREE_MAGIC;
#endif

    b->live_bytes -= m->size;
    b->live_blocks--;

    free(m);
}

/* }}} */

/* Some convenience functions: */

/**
 * This is the wrapper for alloc_string
 *
 * It works just like alloc_string, except it takes a file and line for tracking
 * purposes.  File should usually be __FILE__ and line __LINE__.  The
 * memory for 'file' is not copied and therefore must not be deallocated.
 *
 * @param string the string to copy
 * @param file the originating file
 * @param line the originating line
 * @return a copy of 'string'
 */
char *
CrT_alloc_string(const char *string, const char *file, int line)
{
    char *s;

    /*
     * @TODO This is copy/paste code from fbstring.c and is thus very
     *       brittle.  fbstring.c recommends changing alloc_string to use
     *       strdup which will invalidate this code here.
     *
     *       The reason this code is copied is because for file/line, we
     *       want the file/line of the code doing the alloc_string ...
     *       the file/line of the malloc() that alloc_string does is not
     *       useful information.
     *
     *       Thus, the answer (I guess) is to comment in alloc_string in
     *       fbstrings.c that you need to change this method too.
     */

    /* NULL, "" -> NULL */
    if (!string || !*string)
        return 0;

    if ((s = CrT_malloc(strlen(string) + 1, file, line)) == 0) {
        abort();
    }

    strcpy(s, string);  /* Guaranteed enough space. */
    return s;
}

/**
 * This is the wrapper for alloc_prog_string
 *
 * It works just like alloc_prog_string, except it takes a file and line for
 * tracking purposes.  File should usually be __FILE__ and line __LINE__.  The
 * memory for 'file' is not copied and therefore must not be deallocated.
 *
 * @param s the string to copy
 * @param file the originating file
 * @param line the originating line
 * @return a copy of 's' wrapped in a shared_string struct
 */
struct shared_string *
CrT_alloc_prog_string(const char *s, const char *file, int line)
{
    /*
     * @TODO This is copy/paste code from fbstring.c and is thus very
     *       brittle.
     *
     *       The reason this code is copied is because for file/line, we
     *       want the file/line of the code doing the alloc_prog_string ...
     *       the file/line of the malloc() that alloc_prog_string does is not
     *       useful information.
     *
     *       Thus, the answer (I guess) is to comment in alloc_prog_string in
     *       fbstrings.c that you need to change this method too.
     */

    struct shared_string *ss;
    size_t length;

    if (s == NULL || *s == '\0')
        return (NULL);

    length = strlen(s);

    if ((ss = CrT_malloc(sizeof(struct shared_string) + length, file, line)) == NULL)
        abort();

    ss->links = 1;
    ss->length = length;
    memmove(ss->data, s, ss->length + 1);
    return (ss);
}

/**
 * This is the wrapper for strdup
 *
 * It works just like strdup, except it takes a file and line for
 * tracking purposes.  File should usually be __FILE__ and line __LINE__.  The
 * memory for 'file' is not copied and therefore must not be deallocated.
 *
 * @param s the string to copy
 * @param file the originating file
 * @param line the originating line
 * @return a copy of 's'
 */
char *
CrT_strdup(const char *s, const char *file, int line)
{
    char *p;

    p = CrT_malloc(1 + strlen(s), file, line);

    if (p)
        strcpy(p, s);   /* Guaranteed enough space. */

    return (p);
}

#endif /* MALLOC_PROFILING */
