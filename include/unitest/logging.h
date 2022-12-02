#define _XOPEN_SOURCE 700

#include <stdio.h>

#define ANSI_ESC                        "\x1b["
#define STYLE_REGULAR                   ANSI_ESC"0;3"
#define STYLE_BOLD                      ANSI_ESC"1;3"
#define STYLE_UNDERLINE                 ANSI_ESC"4;3"
#define STYLE_BACKGROUND                ANSI_ESC"4"
#define STYLE_BLINK                     ANSI_ESC"5;3"
#define STYLE_HIGH_INTENSITY_BACKGROUND ANSI_ESC"0;10"
#define STYLE_HIGH_INTENSITY_TEXT       ANSI_ESC"0;9"
#define STYLE_HIGH_INTENSITY_BOLD_TEXT  ANSI_ESC"1;9"

#define COLOR_BLK      0
#define COLOR_RED      1
#define COLOR_GREEN    2
#define COLOR_YELLOW   3
#define COLOR_BLUE     4
#define COLOR_MAGENTA  5
#define COLOR_CYAN     6
#define COLOR_WHITE    7

#define STYLE( file, color, style ) do {		\
    char fmt[ 128 ];					\
    snprintf( fmt, 127, "%s%dm", style, color );	\
    fprintf( (file), "%s", fmt );			\
  } while ( 0 )

#define STYLE_RESET( file ) fprintf( file, ANSI_ESC"0m" )

#define LOG( fp, ... ) fprintf( (fp), "[TEST]"##__VA_ARGS__ )
