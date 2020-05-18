/** @file color.h
 *
 * Header for defining ANSI color attributes and codes.
 *
 * This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.
 */

#ifndef COLOR_H
#define COLOR_H

/* ANSI attributes and color codes */

#define ANSI_RESET      "\033[0m"    /**< Reset ANSI */

#define ANSI_BOLD       "\033[1m"    /**< Toggle bold */
#define ANSI_DIM        "\033[2m"    /**< Toggle dim */
#define ANSI_ITALIC     "\033[3m"    /**< Toggle italic */
#define ANSI_UNDERLINE  "\033[4m"    /**< Toggle underline */
#define ANSI_FLASH      "\033[5m"    /**< Toggle flash */
#define ANSI_REVERSE    "\033[7m"    /**< Toggle reverse FG/BG */
#define ANSI_OSTRIKE    "\033[9m"    /**< Toggle overstrike */

#define ANSI_FG_BLACK   "\033[30m"   /**< Foreground black */
#define ANSI_FG_RED     "\033[31m"   /**< Foreground red */
#define ANSI_FG_GREEN   "\033[32m"   /**< Foreground green */
#define ANSI_FG_YELLOW  "\033[33m"   /**< Foreground yellow */
#define ANSI_FG_BLUE    "\033[34m"   /**< Foreground blue */
#define ANSI_FG_MAGENTA "\033[35m"   /**< Foreground magenta */
#define ANSI_FG_CYAN    "\033[36m"   /**< Foreground cyan */
#define ANSI_FG_WHITE   "\033[37m"   /**< Foreground white */

#define ANSI_BG_BLACK   "\033[40m"   /**< Background black */
#define ANSI_BG_RED     "\033[41m"   /**< Background red */
#define ANSI_BG_GREEN   "\033[42m"   /**< Background green */
#define ANSI_BG_YELLOW  "\033[43m"   /**< Background yellow */
#define ANSI_BG_BLUE    "\033[44m"   /**< Background blue */
#define ANSI_BG_MAGENTA "\033[45m"   /**< Background magenta */
#define ANSI_BG_CYAN    "\033[46m"   /**< Background cyan */
#define ANSI_BG_WHITE   "\033[47m"   /**< background white */

#endif /* !COLOR_H */
