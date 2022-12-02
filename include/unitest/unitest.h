/**
 * @file unitest.h
 * @author François Cayre <cayre@uvolante.org>
 * @date Mon Aug  8 08:47:06 2022
 * @brief Unit testing.
 *
 * Unit testing.
 */

#ifndef _UNITEST_H_
#define _UNITEST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unitest/logging.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern unsigned ntests_passed;
extern unsigned ntests_total;
extern int      test_verbose;
extern char    *test_name;
extern char    *prog_name;

void    unit_test( int argc, char *argv[] );

#define test_oracle__( test, oracle, ... ) do {			\
    int  ret__  = 0;						\
    int  out_OK = 1;						\
    char output[ BUFSIZ ] = { 0 };				\
    ntests_total++;						\
    if ( test_verbose ) {					\
      fprintf( stderr, __VA_ARGS__ );				\
      fprintf( stderr, ": " );					\
    }								\
    fflush( stdout );						\
    freopen( "/dev/null", "a", stdout );			\
    setbuf( stdout, output );					\
    ret__ = !!(test);						\
    fflush( stdout );						\
    freopen( "/dev/tty", "a", stdout );				\
    if ( oracle && *oracle ) {					\
      out_OK = !strcmp( oracle, output );			\
    }								\
    if ( out_OK && ret__ ) {					\
      ntests_passed++;						\
      if ( test_verbose ) {					\
	STYLE( stderr, COLOR_GREEN, STYLE_BOLD );		\
	fprintf( stderr, "PASSED" );				\
	STYLE_RESET( stderr );					\
      }								\
    }								\
    else {							\
      if ( test_verbose ) {					\
	STYLE( stderr, COLOR_RED, STYLE_BOLD );			\
	fprintf( stderr, "FAILED" );				\
	STYLE_RESET( stderr );					\
	if ( !out_OK ) {					\
	  fprintf( stderr, " output: '%s'", output );		\
	  if ( !ret__ ) {					\
	    fprintf( stderr, " and" );				\
	  }							\
	}							\
	if ( !ret__ ) {						\
	  fprintf( stderr, " assertion: %s", #test );		\
	}							\
	fprintf( stderr, " at %s:%d", __FILE__, __LINE__ );	\
      }								\
    }								\
    if ( test_verbose ) {					\
      fprintf( stderr, ".\n" );					\
    }								\
  } while( 0 )

#define test_oracle( ... ) test_oracle__( __VA_ARGS__ )
#define test_assert__( test, ... )  test_oracle( test, "", __VA_ARGS__ )
#define test_assert( ... ) test_assert__( __VA_ARGS__ )

#define test_suite( msg ) do {						\
  if ( test_name ) {							\
    if ( ntests_passed != ntests_total ) {				\
      if ( !test_verbose ) {						\
	fprintf( stderr, ": " );					\
      }									\
      STYLE( stderr, COLOR_RED, STYLE_BOLD );				\
      fprintf( stderr, "FAILED " );					\
      STYLE_RESET( stderr );						\
      fprintf( stderr, "%u test%s (out of %u).\n",			\
	       ntests_total - ntests_passed,				\
	       ntests_total - ntests_passed > 1 ? "s" : "",		\
	       ntests_total );						\
      if(0 == test_verbose) { \
          STYLE( stderr, COLOR_BLUE, STYLE_BOLD );				\
          fprintf( stderr, "Relaunch with -v for details.\n"); \
          STYLE_RESET( stderr );						\
      } \
    }									\
    else {								\
      if ( !test_verbose ) {						\
	fprintf( stderr, ": " );					\
	STYLE( stderr, COLOR_GREEN, STYLE_BOLD );			\
	fprintf( stderr, "PASSED" );					\
	STYLE_RESET( stderr );						\
	fprintf( stderr, ".\n" );					\
      }									\
    }									\
  }									\
  test_name = msg;							\
  if ( test_name ) {							\
    STYLE( stderr, COLOR_BLUE, STYLE_BOLD );				\
    fprintf( stderr, "[ %12s ] %s",					\
	     __func__, test_name );					\
    STYLE_RESET( stderr );						\
    if ( test_verbose ) {						\
      fprintf( stderr, "\n" );						\
    }									\
  }									\
  ntests_total  = 0;							\
  ntests_passed = 0;							\
  } while ( 0 )

#ifdef __cplusplus
}
#endif

#endif /* _UNITEST_H_ */
