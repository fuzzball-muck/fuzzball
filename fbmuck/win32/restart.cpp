// restart.cpp
//

#include <string.h>
#include <time.h>
#include <process.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "win32.h"


#define RESTART_VERSION "Restart Version 2.3"
#define fpeekc(x) ungetc(fgetc(x), x)

#ifndef MAX_PATH
# ifdef _MAX_PATH
#  define MAX_PATH _MAX_PATH
# else
#  define MAX_PATH 1024
# endif
#endif

void create_ini(char *fn);
void print_usage();


typedef struct s_mucklist {
   char *muckname;

   bool logging;
   bool wizonly;
   bool hideconsole;
   char *logfile;
   int  portcount;
   int  *ports;
   int  sslportcount;
   int  *sslports;
   char *macrosfile;
   char *dbinfile;
   char *dboutfile;
   char *dboldfile;
   char *deltasfile;
   char *server;

   struct s_mucklist *next;
} mucklist;

void print_usage() {
   printf("restart [-c] [inifile] - Clean up database files and restart the muck\n");
   printf("          -c - Creates an ini file with the provided name (or restart.ini)\n");
   printf("   [inifile] - Defaults to restart.ini - Specifies the config file\n");
   printf("               for starting up the server\n");
}

void create_ini(char *fn) {
   FILE *f = fopen(fn, "w");
   if (!f) return;

   fprintf(f, "[fbmuck]\n");
   fprintf(f, "logging    = true\n");
   fprintf(f, "logfile    = logs\\restart\n");
   fprintf(f, "ports      = 4201\n");
   fprintf(f, "sslports   = 5201\n");
   fprintf(f, "macrosfile = muf\\macros\n");
   fprintf(f, "dbinfile   = data\\minimal.db\n");
   fprintf(f, "dboutfile  = data\\minimal.out\n");
   fprintf(f, "dboldfile  = data\\minimal.old\n");
   fprintf(f, "deltasfile = data\\deltas-file\n");
   fprintf(f, "server     = fbmuck\n");
   fprintf(f, "wizonly    = false\n");
   fprintf(f, "hideconsole= false\n");

   fclose(f);
}

mucklist * newmucklist(const char *name, mucklist *m) {
   mucklist *nm      = (mucklist *) malloc(sizeof(mucklist));
   nm->muckname      = strdup(name);
   nm->logging       = true;
   nm->wizonly       = false;
   nm->hideconsole   = false;
   nm->logfile       = strdup("logs\\restart");
   nm->ports         = (int *) malloc(sizeof(int) * 1);
   nm->ports[0]      = 4201;
   nm->portcount     = 1;
   nm->sslports      = (int *) malloc(sizeof(int) * 1);
   nm->sslports[0]   = 5201;
   nm->sslportcount  = 1;
   nm->macrosfile    = strdup("muf\\macros");
   nm->dbinfile      = strdup("data\\minimal.db");
   nm->dboutfile     = strdup("data\\minimal.out");
   nm->dboldfile     = strdup("data\\minimal.old");
   nm->deltasfile    = strdup("data\\deltas-file");
   nm->server        = strdup("fbmuck");
   nm->next          = m;
   return nm;
}

char * str_replace(const char *s, char *p) {
   if (p) free(p);
    return strdup(s);
}

bool parse_bool(char *str) {
   const char *p = str;
   while (*p != '=') p++;
   p++;
   while (isspace(*p)) p++;
   if (toupper(*p) == 'T' || *p == '1') return true;
   return false;
}

char *parse_str(char *str) {
   char *p = str;
   while (*p != '=') p++;
   p++;
   while (isspace(*p)) p++;
   char *q = p + strlen(p) - 1;
   if (q <= p) return strdup("");
   while (isspace(*q)) q--;
   *(++q) = '\0';
   return strdup(p);
}

int *parse_int_list(char *str, int &count) {
   char *p = str;
   while (*p != '=') p++;
   str = ++p;
   int acount = 0;
   bool skipping = true;
   while (*p) {
      if (skipping && !isspace(*p)) {
         acount++;
         skipping = false;
         p++;
      } else if (!skipping && isspace(*p)) {
         skipping = true;
      }
      p++;
   }

   p = str;
   int *list = (int *) malloc(sizeof(int) * acount);
   skipping = true;
   char *iptr = 0; count = 0;
   while (*p && count < acount) {
      if (skipping && !isspace(*p)) {
         iptr = p;
         skipping = false;
      } if (!skipping && isspace(*p)) {
         *p = '\0';
         list[count++] = atoi(iptr);
         skipping = true;
      }
      p++;
   }
   if (!*p) list[count++] = atoi(iptr);
   count--;

   return list;
}

mucklist * parse_ini(const char *fn) {
   FILE *f = fopen(fn, "r");
   if (!f) {
      printf("ERROR: Could not open %s for input.\n", fn);
      return 0;
   }

   mucklist *m = 0;
   char buf[MAX_PATH];
   char *p = buf;
   int line = 1;
   bool running = true;
   while (running) {
      char c;
      switch (c = fpeekc(f)) {
         case EOF:
             running = false;
             break;
         
          case '[':
             fgets(buf, MAX_PATH, f);
             p = buf + strlen(buf) - 1;
             while (!isalnum(*p)) p--;
             *(p+1) = '\0';
             m = newmucklist(buf + 1, m);
             break;

          case 'm':
          case 'M':
             fgets(buf, MAX_PATH, f);
             if (!_strnicmp(buf, "macrosfile", 10)) {
                if (!m) {
                   printf("\nERROR: macros file without a muck section - line %d\n", line);
                   return 0;
                }
                m->macrosfile = str_replace(parse_str(buf), m->macrosfile);
             }
             break;
             

          case 'l':
          case 'L':
             fgets(buf, MAX_PATH, f);
             if (!_strnicmp(buf, "logging", 7)) {
                if (!m) {
                   printf("\nERROR: logging without a muck section - line %d\n", line);
                   return 0;
                }
                m->logging = parse_bool(buf);
             } else if (!_strnicmp(buf, "logfile", 7)) {
                if (!m) {
                   printf("\nERROR: logfile without a muck section - line %d\n", line);
                   return 0;
                }
                m->logfile = str_replace(parse_str(buf),m->logfile);
             }
             break;

          case 'p':
          case 'P':
             fgets(buf, MAX_PATH, f);
             if (!_strnicmp(buf, "ports", 5)) {
                if (!m) {
                   printf("\nERROR: port numbers without a muck section - line %d\n", line);
                   return 0;
                }
                m->ports = parse_int_list(buf, m->portcount);
             }
             break;

          case 'h':
          case 'H':
             fgets(buf, MAX_PATH, f);
             if (!_strnicmp(buf, "hideconsole", 11)) {
                if (!m) {
                   printf("\nERROR: hide console option without a muck section - line %d\n", line);
                   return 0;
                }
                m->hideconsole = parse_bool(buf);
             }
             break;

          
          case 's':
          case 'S':
             fgets(buf, MAX_PATH, f);
             if (!_strnicmp(buf, "sslports", 8)) {
                if (!m) {
                   printf("\nERROR: SSL port numbers without a muck section - line %d\n", line);
                   return 0;
                }
                m->sslports = parse_int_list(buf, m->sslportcount);
             } else if (!_strnicmp(buf, "server", 6)) {
                if (!m) {
                   printf("\nERROR: server name without a muck section - line %d\n", line);
                   return 0;
                }
                m->server = str_replace(parse_str(buf), m->server);
             }
             break;

          case 'd':
          case 'D':
             fgets(buf, MAX_PATH, f);
             if (!_strnicmp(buf, "dbinfile", 8)) {
                if (!m) {
                   printf("\nERROR: database input file without a muck section - line %d\n", line);
                   return 0;
                }
                m->dbinfile = str_replace(parse_str(buf), m->dbinfile);
             } else if (!_strnicmp(buf, "dboutfile", 9)) {
                if (!m) {
                   printf("\nERROR: database output file without a muck section - line %d\n", line);
                   return 0;
                }
                m->dboutfile = str_replace(parse_str(buf), m->dboutfile);
             } else if (!_strnicmp(buf, "dboldfile", 9)) {
                if (!m) {
                   printf("\nERROR: database old file without a muck section - line %d\n", line);
                   return 0;
                }
                m->dboldfile = str_replace(parse_str(buf), m->dboldfile);
             } else if (!_strnicmp(buf, "deltasfile", 10)) {
                if (!m) {
                   printf("\nERROR: database deltas file without a muck section - line %d\n", line);
                   return 0;
                }
                m->deltasfile = str_replace(parse_str(buf), m->deltasfile);
             }
             break;

          case 'w':
          case 'W':
             fgets(buf, MAX_PATH, f);
             if (!_strnicmp(buf, "wizonly", 7)) {
                if (!m) {
                   printf("\nERROR: wizonly option without a muck section - line %d\n", line);
                   return 0;
                }
                m->wizonly = parse_bool(buf);
             }
             break;

          case '\n':
          case '\r':
          case '\t':
             fgets(buf, MAX_PATH, f);
             break;             
             
          default:
             printf("\nERROR: Unrecognized input on line %d\n", line);
             return 0;
            
      }
      line++;
   }

   return m;
}

int main (int argc, char **argv) {
   const char *iniFileName = "restart.ini";

   for (int i = 1; i < argc; i++) {
      switch(argv[i][0]) {
         case '-':
            if (!strcmp(argv[i], "--help")) {
               print_usage();
               return 0;
            } else if (!strcmp(argv[i], "-c")) {
               create_ini((argc == 3 ? argv[2] : "restart.ini"));
               return 0;
            }
         default:
            iniFileName = argv[i];
      }
   }

   mucklist *ml = parse_ini(iniFileName);
   if (!ml) return -1;

   mucklist *p = ml;
   while (p) {
      FILE    *logfile;
      char    dmy[12];
      char    hms[10];
      time_t  now    = time(NULL);
      struct  tm *lt = localtime(&now);
      strftime(dmy, 12, "%m-%d-%Y", lt);
      strftime(hms, 10, "%H:%M:%S", lt);

      printf("[%s]\n", p->muckname);

      // Log file
      if (p->logging) {
         printf("   Opening log file...");
         logfile = fopen(p->logfile, "a");
         if (!logfile) {
            printf("FAILED\n");
            p->logging = false;
         } else {
            printf("done\n");
            fprintf(logfile, "---------------[ Starting Server: %s @ %s]---------------\n", dmy, hms);
         }
      }

      // DB Old -> DB Older
      FILE *f = fopen(p->dboldfile, "r");
      if (f) {
         fclose(f);
         printf("   Moving db old to db older...");
         if (p->logging) fprintf(logfile, "Moving db old to db older...");
         char *oldbuf = (char *) malloc(strlen(p->dboldfile) + strlen(dmy) + 2);
         sprintf(oldbuf, "%s.%s", p->dboldfile, dmy);
         unlink(oldbuf);
         if (rename(p->dboldfile, oldbuf)) {
            printf("FAILED\n");
            if (p->logging) fprintf(logfile, "FAILED\n");
            free(oldbuf);
            if (p->logging) fclose(logfile);
            p = p->next;
            continue;
         } else {
            free(oldbuf);
            printf("done\n");
            if (p->logging) fprintf(logfile, "done\n");
         }
      }


      // Check for PANIC
      char *panicbuf = (char *) malloc(strlen(p->dboutfile) + 7);
      sprintf(panicbuf, "%s.PANIC", p->dboutfile);
      f = fopen(panicbuf, "r");
      if (f) {
         fclose(f);
         printf("   Moving PANIC file(s)...");
         if (p->logging) fprintf(logfile, "Moving PANIC file(s)...");
         char *macros = (char *) malloc(strlen(p->macrosfile) + 7);
         sprintf(macros, "%s.PANIC", p->macrosfile);
         unlink(p->macrosfile);
         if (rename(macros, p->macrosfile)) {
            printf("FAILED\n");
            if (p->logging) fprintf(logfile, "FAILED\n");
            free(macros);
            if (p->logging) fclose(logfile);
            p = p->next;
            continue;
         }
         free(macros);
         unlink(p->dbinfile);
         if (rename(panicbuf, p->dbinfile)) {
            printf("FAILED\n");
            if (p->logging) fprintf(logfile, "FAILED\n");
            free(panicbuf);
            if (p->logging) fclose(logfile);
            p = p->next;
            continue;
         } else {
            printf("done\n");
            if (p->logging) fprintf(logfile, "done\n");
            free(panicbuf);
         }
      }

      // If dbout, dbin -> dbold
      f = fopen(p->dboutfile, "r");
      if (f) {
         fclose(f);
         f = fopen(p->dbinfile, "r");
         if (f) {
            fclose(f);
            printf("   Moving db in to db old...");
            if (p->logging) fprintf(logfile, "Moving db in to db old...");
            unlink(p->dboldfile);
            if (rename(p->dbinfile, p->dboldfile)) {
               printf("FAILED\n");
               if (p->logging) fprintf(logfile, "FAILED\n");
               if (p->logging) fclose(logfile);
               p = p->next;
               continue;
            } else {
               printf("done\n");
               if (p->logging) fprintf(logfile, "done\n");
            }
         }
         unlink(p->dbinfile);
         printf("   Moving db out to db in...");
         if (p->logging) fprintf(logfile, "Moving db out to db in...");
         if (rename(p->dboutfile, p->dbinfile)) {
            printf("FAILED\n");
            if (p->logging) fprintf(logfile, "FAILED\n");
            if (p->logging) fclose(logfile);
            p = p->next;
            continue;
         } else {
            printf("done\n");
            if (p->logging) fprintf(logfile, "done\n");
         }
      }

      // Validate DB in file
      printf("   Verifying input file...");
      if (p->logging) fprintf(logfile, "Verifying input file...");
      f = fopen(p->dbinfile, "r");
      if (!f) {
         printf("FAILED\n");
         if (p->logging) fprintf(logfile, "FAILED\n");
         if (p->logging) fclose(logfile);
         p = p->next;
         continue;
      }

      fseek(f, -18L, SEEK_END);
      char checkbuf[18];
      fread(checkbuf, sizeof(char), 17, f);
      checkbuf[17] = '\0';
      fclose(f);
      if (strcmp(checkbuf, "***END OF DUMP***")) {
         if (!strcmp(checkbuf, "**END OF DUMP***\n")) {
            printf("delayed\n");
            if (p->logging) fprintf(logfile, "delayed\n");
            printf("   Converting input file from dos to unix format...");
            if (p->logging) fprintf(logfile, "Converting input file from dos to unix format...");
            FILE *out = fopen(p->dboutfile, "wb");
            FILE *in  = fopen(p->dbinfile, "rb");
            if (!in || !out) {
               printf("FAILED\n");
               if (p->logging) fprintf(logfile, "FAILED\n");
               fclose(in);
               fclose(out);
               if (p->logging) fclose(logfile);
               p = p->next;
               continue;
            }
            char c;
            while ((c = fgetc(in)) != EOF) {
               if (c != '\r') fputc(c, out);
            }
            fclose(in);
            fclose(out);
            unlink(p->dbinfile);
            if (rename(p->dboutfile, p->dbinfile)) {
               printf("FAILED\n");
               if (p->logging) fprintf(logfile, "FAILED\n");
               if (p->logging) fclose(logfile);
               p = p->next;
               continue;
            }

            char *outmacro = (char *) malloc(strlen(p->macrosfile) + 5);
            sprintf(outmacro, "%s.out", p->macrosfile);
            out = fopen(outmacro, "wb");
            in  = fopen(p->macrosfile, "rb");
            if (out && in) {
               while ((c = fgetc(in)) != EOF) {
                  if (c != '\r') fputc(c, out);
               }
               fclose(in);
               fclose(out);
               unlink(p->macrosfile);
               rename(outmacro, p->macrosfile);
            }
            free(outmacro);
            printf("done\n");
            if (p->logging) fprintf(logfile, "done\n");

            if (p->logging) fprintf(logfile, "Verifying input file...");
            printf("   Verifying input file...");
            f = fopen(p->dbinfile, "r");
            if (!f) {
               if (p->logging) fprintf(logfile, "FAILED\n");
               printf("FAILED\n");
               if (p->logging) fclose(logfile);
               p = p->next;
               continue;
            }
            fseek(f, -18L, SEEK_END);
            fread(checkbuf, sizeof(char), 17, f);
            checkbuf[17] = '\0';
            fclose(f);
            if (strcmp(checkbuf, "***END OF DUMP***")) {
               if (p->logging) fprintf(logfile, "FAILED\n");
               printf("FAILED\n");
               if (p->logging) fclose(logfile);
               p = p->next;
               continue;
            }
         } else {
            printf("FAILED\n");
            if (p->logging) fprintf(logfile, "FAILED\n");
            if (p->logging) fclose(logfile);
            p = p->next;
            continue;
         }
      }
      printf("done\n");
      if (p->logging) fprintf(logfile, "done\n");

      // Check for a deltas file
      f = fopen(p->deltasfile, "rb");
      if (f) {
         printf("   Merging deltas file...");
         if (p->logging) fprintf(logfile, "Merging deltas file...");
         fseek(f, -18L, SEEK_END);
         char checkbuf[18];
         fread(checkbuf, sizeof(char), 17, f);
         if (strcmp(checkbuf,"***END OF DUMP***")) {
            printf("FAILED\n");
            if (p->logging) fprintf(logfile, "FAILED\n");
            fclose(f);
         } else {
            FILE *dbin = fopen(p->dbinfile, "a+b");
            if (!dbin) {
               printf("FAILED\n");
               if (p->logging) fprintf(logfile, "FAILED\n");
               fclose(f);
               if (p->logging) fclose(logfile);
               p = p->next;
               continue;
            }
            fseek(f, 0L, SEEK_SET);
            char c;
            while((c = fgetc(f)) != EOF) {
               if (c != '\r') fputc(c, dbin);
            }
            fclose(f);
            fclose(dbin);
            printf("done\n");
            if (p->logging) fprintf(logfile, "done\n");
         }
         
      }

      // Startup the server
      if (p->logging) fprintf(logfile, "Starting server...\n");
      printf("   Starting server...\n");
      int argcount = 0;
      argcount  = 1 + 2 + p->portcount + (p->sslportcount * 2);
      argcount += (p->wizonly ? 1 : 0) + (p->hideconsole) + 1;
      char **args = (char **) malloc(sizeof(char *) * argcount);
      int marg = 0;
      args[marg++] = strdup(p->server);

      if (p->wizonly) args[marg++] = strdup("-wizonly");
      if (p->hideconsole) args[marg++] = strdup("-freeconsole");
      for (int i = 0; i < p->sslportcount; i++) {
         args[marg++] = strdup("-sport");
         char buf[15];
         _snprintf(buf, 14, "%d", p->sslports[i]);
         args[marg++] = strdup(buf);
      }
      args[marg++] = strdup(p->dbinfile);
      args[marg++] = strdup(p->dboutfile);
      for (int i = 0; i < p->portcount; i++) {
         char buf[15];
         _snprintf(buf, 14, "%d", p->ports[i]);
         args[marg++] = strdup(buf);
      }
      args[marg++] = '\0';
      _spawnv(_P_NOWAIT, p->server, args);
      for (int i = 0; i < argcount; i++) free(args[i]);
      free(args);
      printf("done\n");
      p = p->next;
   }
}

