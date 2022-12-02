#define _XOPEN_SOURCE 700

#define _GNU_SOURCE /* RTLD_NEXT */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <err.h>
#include <execinfo.h>
#include <dlfcn.h>
#include <assert.h>

#include <unitest/unitest.h>
#include <pyas/macros.h>

unsigned ntests_passed = 0;
unsigned ntests_total  = 0;
int      test_verbose  = 0;
int      test_debug    = 0;
char    *test_name     = NULL;
char    *prog_name     = NULL;

static void test_finish( void ) {
  test_suite( NULL );

  exit( ntests_total == ntests_passed ? EXIT_SUCCESS : EXIT_FAILURE );
}

/*
  Adapted from:
  https://github.com/ThrowTheSwitch/Unity
  MIT License, Copyright (c) <year> 2007-21
  by Mike Karlesky, Mark VanderVoord and Greg Williams.
 */

static int addr2line( char const * const program_name, char *func, void const * const addr ) {
  char addr2line_cmd[512] = {0};

  /* TODO: Find command for OSX __APPLE__. Maybe atos ?: */
  /* sprintf( addr2line_cmd, "atos -o %.256s %p",  program_name, addr ); */
  
  if ( func && *func ) { /* func+addr */
    sprintf( addr2line_cmd, "addr2line -i -f -p -e %.256s `printf %%x $((0x\\`nm -s %s | grep %s | head -1 | awk '{ print $1 }'\\`+%p))` 2> /dev/null",
	     program_name, program_name, func, addr );

  }
  else { /* absolute addr */
    sprintf( addr2line_cmd, "addr2line -i -f -p -e %.256s %p 2> /dev/null",
	     program_name, addr );
  }
  return system( addr2line_cmd );
}

#define MAX_STACK_FRAMES 256

static void *stack_traces[MAX_STACK_FRAMES];

void posix_print_stack_trace( void ) {
  int   i, trace_size = 0;
  char     **messages = (char **)NULL;

  trace_size = backtrace( stack_traces, MAX_STACK_FRAMES );
  messages   = backtrace_symbols( stack_traces, trace_size );

  for ( i = 3; i < trace_size-2; i++ ) {
    char *c = messages[ i ];
    char *paddr, *pfun;
    void *text_addr;

    while ( '(' != *c ) c++;
    *c = '\0';
    pfun = c+1;
    while ( '+' != *c ) c++;
    *c = '\0';
    paddr = c+1;
    c = paddr;
    while ( ')' != *c ) c++;
    *c = '\0';
    sscanf( paddr, "%p", &text_addr );

    if ( 3 == i ) {
      fprintf( stderr, " >> In " );
      STYLE( stderr, COLOR_BLUE, STYLE_BOLD );
    }
    else {
      fprintf( stderr, " Called by " );
    }

    addr2line( messages[ i ], pfun, text_addr );

    if ( 3 == i ) {
      STYLE_RESET( stderr );
      fprintf( stderr, "\n" );
    }
  }

  fprintf( stderr, "\n" );
  free( messages );
}

#define SIG_CASE_PRINT( sig, msg )					\
  if ( test_verbose ) {							\
    fprintf( stderr, "\n" );						\
  }									\
  STYLE( stderr, COLOR_RED, STYLE_BOLD );				\
  fprintf( stderr, " *** FATAL ERROR :: %s [%s] ***",			\
	   sig, msg );							\
  STYLE_RESET( stderr )

#define SIG_SUB_CASE__( sig, sub, msg ) case sub :			\
  SIG_CASE_PRINT( sig, msg );						\
  break

#define SIG_SUB_CASE( sig, sub, msg ) SIG_SUB_CASE__( #sig, sub, msg )
#define SIG_CASE( signal, msg )       SIG_SUB_CASE__( #signal, signal, msg )

void posix_signal_handler( int sig, siginfo_t *siginfo, void *context ) {

  fputs( "\n", stderr );

  switch ( sig ) {
    SIG_CASE( SIGSEGV, "Segmentation fault" );
    SIG_CASE( SIGINT , "Got Ctrl-C" );
    SIG_CASE( SIGTERM, "A termination request was sent to the program" );
    SIG_CASE( SIGABRT, "Usually caused by an abort() or assert()" );
  case SIGFPE:
    switch( siginfo->si_code ) {
      SIG_SUB_CASE( SIGFPE, FPE_INTDIV, "Integer divide by zero" );
      SIG_SUB_CASE( SIGFPE, FPE_INTOVF, "Integer overflow" );
      SIG_SUB_CASE( SIGFPE, FPE_FLTDIV, "Floating point divide by zero" );
      SIG_SUB_CASE( SIGFPE, FPE_FLTOVF, "Floating point overflow" );
      SIG_SUB_CASE( SIGFPE, FPE_FLTUND, "Floating point underflow" );
      SIG_SUB_CASE( SIGFPE, FPE_FLTRES, "Floating point inexact result" );
      SIG_SUB_CASE( SIGFPE, FPE_FLTINV, "Floating point invalid operation" );
      SIG_SUB_CASE( SIGFPE, FPE_FLTSUB, "Subscript out of range" );
    default:
      SIG_CASE_PRINT( "SIGFPE", "Arithmetic exception" );
      break;
    }
    break;
  case SIGILL:
    switch( siginfo->si_code ) {
      SIG_SUB_CASE( SIGILL, ILL_ILLOPC, "Illegal opcode" );
      SIG_SUB_CASE( SIGILL, ILL_ILLOPN, "Illegal operand" );
      SIG_SUB_CASE( SIGILL, ILL_ILLADR, "Illegal addressing mode" );
      SIG_SUB_CASE( SIGILL, ILL_ILLTRP, "Illegal trap" );
      SIG_SUB_CASE( SIGILL, ILL_PRVOPC, "Privileged opcode" );
      SIG_SUB_CASE( SIGILL, ILL_PRVREG, "Privileged register" );
      SIG_SUB_CASE( SIGILL, ILL_COPROC, "Coprocessor error" );
      SIG_SUB_CASE( SIGILL, ILL_BADSTK, "Internal stack error" );
    default:
      SIG_CASE_PRINT( "SIGILL", "Illegal instruction" );
      break;
    }
    break;

  default :
    break;
  }

  fputs( "\n\n", stderr );

  /* During a test, stdout is redirected to
     /dev/null - so restore it back to /dev/tty.
  */
  freopen( "/dev/tty", "a", stdout );

#ifdef __APPLE__
  STYLE( stderr, COLOR_BLUE, STYLE_BOLD );
  fprintf(stderr, "unitest.c can't handle signals details on MacOS. Please run on Linux for details.\n");
  STYLE_RESET( stderr );
#else
  if ( test_debug ) {

    char debugger[ 1+STRLEN ];
    /*
      https://askubuntu.com/questions/41629/after-upgrade-gdb-wont-attach-to-process
     */
    snprintf( debugger, STRLEN, "sudo gdb -p %d", getpid() );

    system( debugger );
  }
  else {
    posix_print_stack_trace();
  }
#endif
  _Exit( EXIT_FAILURE );

  UNUSED( context );
}

#define HANDLE_SIGNAL( signal ) do {					\
    if ( sigaction( (signal), &sig_action, NULL ) != 0 ) {		\
      err( EXIT_FAILURE, "sigaction" );					\
    }									\
  } while ( 0 )


static void set_signal_handler( void ) {

  struct sigaction sig_action;

  sig_action.sa_sigaction = posix_signal_handler;
  sig_action.sa_flags     = SA_SIGINFO | SA_RESETHAND;

  sigemptyset( &sig_action.sa_mask );

  HANDLE_SIGNAL( SIGSEGV );
  HANDLE_SIGNAL( SIGFPE  );
  HANDLE_SIGNAL( SIGINT  );
  HANDLE_SIGNAL( SIGILL  );
  HANDLE_SIGNAL( SIGTERM );
  HANDLE_SIGNAL( SIGABRT );
}

static void print_test_usage( char *progname ) {
  fprintf( stderr, "\nSynopsis:\n\n" );
  fprintf( stderr, "  %s [OPTIONS]\n\n", progname );
  fprintf( stderr, "OPTIONS\n\n" );
  fprintf( stderr, "  -v | --verbose\tMake test verbose\n" );
  fprintf( stderr, "  -g | --debug  \tLaunch debugger on crash\n" );
  fprintf( stderr, "  -h | --help   \tPrint this help\n" );
  fprintf( stderr, "\n" );
  fprintf( stderr, "EXAMPLE\n\n" );
  fprintf( stderr, "Launch test in verbose mode and spin off debugger on crash:\n\n" );
  fprintf( stderr, "  %s -v -g\n", progname );
  fprintf( stderr, "  %s -vg\n", progname );
  fprintf( stderr, "\n" );
}

void    unit_test( int argc, char *argv[] ) {
  int  i = 0;

  prog_name   = argv[ 0 ];

  set_signal_handler();

  for ( i = 1 ; i < argc ; i++ ) {
    if ( !strcmp( "-h", argv[ i ] ) || !strcmp( "--help", argv[ i ] ) ) {
      print_test_usage( prog_name );
      exit( EXIT_SUCCESS );
    }
    if ( '-' == argv[ i ][ 0 ] && '-' != argv[ i ][ 1 ] ) {
      char *ptr = argv[ i ];
      while ( *++ptr ) {
	if ( 'v' == *ptr ) {
	  test_verbose = 1;
	}
	if ( 'g' == *ptr ) {
	  test_debug   = 1;
	}
      }
    }
    if ( !strcmp( "--verbose", argv[ i ] ) ) {
      test_verbose = 1;
    }
    if ( !strcmp( "--debug", argv[ i ] ) ) {
      test_debug   = 1;
    }
  }

  atexit( test_finish );
}
