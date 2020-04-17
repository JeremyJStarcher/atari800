##############################################
## The following part is copied from sdl.m4 ##
##############################################

# Configure paths for SDL2
# Sam Lantinga 9/21/99
# stolen from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_SDL2([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for SDL2, and define SDL2_CFLAGS and SDL2_LIBS
dnl
AC_DEFUN([AM_PATH_SDL2],
[dnl 
dnl Get the cflags and libraries from the sdl2-config script
dnl
AC_ARG_WITH(sdl2-prefix,[  --with-sdl2-prefix=PFX   Prefix where SDL2 is installed (optional)],
            sdl2_prefix="$withval", sdl2_prefix="")
AC_ARG_WITH(sdl2-exec-prefix,[  --with-sdl2-exec-prefix=PFX Exec prefix where SDL2 is installed (optional)],
            sdl2_exec_prefix="$withval", sdl_exec_prefix="")
AC_ARG_ENABLE(sdl2test, [  --disable-sdl2test       Do not try to compile and run a test SDL2 program],
		    , enable_sdl2test=yes)

  if test x$sdl2_exec_prefix != x ; then
    sdl2_config_args="$sdl2_config_args --exec-prefix=$sdl2_exec_prefix"
    if test x${SDL2_CONFIG+set} != xset ; then
      SDL2_CONFIG=$sdl2_exec_prefix/bin/sdl2-config
    fi
  fi
  if test x$sdl2_prefix != x ; then
    sdl2_config_args="$sdl2_config_args --prefix=$sdl2_prefix"
    if test x${SDL2_CONFIG+set} != xset ; then
      SDL2_CONFIG=$sdl2_prefix/bin/sdl2-config
    fi
  fi

  as_save_PATH="$PATH"
  if test "x$prefix" != xNONE; then
    PATH="$prefix/bin:$prefix/usr/bin:$PATH"
  fi
  AC_PATH_PROG(SDL2_CONFIG, sdl2-config, no, [$PATH])
  PATH="$as_save_PATH"
  min_sdl2_version=ifelse([$1], ,0.11.0,$1)
  AC_MSG_CHECKING(for SDL2 - version >= $min_sdl_version)
  no_sdl=""
  if test "$SDL2_CONFIG" = "no" ; then
    no_sdl2=yes
  else
    SDL2_CFLAGS=`$SDL2_CONFIG $sdl_config_args --cflags`
    SDL2_LIBS=`$SDL2_CONFIG $sdl_config_args --libs`

    sdl2_major_version=`$SDL2_CONFIG $sdl2_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    sdl2_minor_version=`$SDL2_CONFIG $sdl2_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    sdl2_micro_version=`$SDL2_CONFIG $sdl2_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_sdl2test" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_CXXFLAGS="$CXXFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $SDL2_CFLAGS"
      CXXFLAGS="$CXXFLAGS $SDL2_CFLAGS"
      LIBS="$LIBS $SDL2_LIBS"
dnl
dnl Now check if the installed SDL2 is sufficiently new. (Also sanity
dnl checks the results of sdl2-config to some extent
dnl
      rm -f conf.sdltest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.sdltest");
  */
  { FILE *fp = fopen("conf.sdltest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_sdl_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_sdl_version");
     exit(1);
   }

   if (($sdl_major_version > major) ||
      (($sdl_major_version == major) && ($sdl_minor_version > minor)) ||
      (($sdl_major_version == major) && ($sdl_minor_version == minor) && ($sdl_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'sdl-config --version' returned %d.%d.%d, but the minimum version\n", $sdl_major_version, $sdl_minor_version, $sdl_micro_version);
      printf("*** of SDL required is %d.%d.%d. If sdl-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If sdl-config was wrong, set the environment variable SDL_CONFIG\n");
      printf("*** to point to the correct copy of sdl-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_sdl2=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       CXXFLAGS="$ac_save_CXXFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_sdl2" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$SDL2_CONFIG" = "no" ; then
       echo "*** The sdl2-config script installed by SDL2 could not be found"
       echo "*** If SDL2 was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the SDL2_CONFIG environment variable to the"
       echo "*** full path to sdl2-config."
     else
       if test -f conf.sdltest ; then
        :
       else
          echo "*** Could not run SDL2 test program, checking why..."
          CFLAGS="$CFLAGS $SDL2_CFLAGS"
          CXXFLAGS="$CXXFLAGS $SDL2_CFLAGS"
          LIBS="$LIBS $SDL2_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include "SDL.h"

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding SDL2 or finding the wrong"
          echo "*** version of SDL2. If it is not finding SDL2, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means SDL2 was incorrectly installed"
          echo "*** or that you have moved SDL2 since it was installed. In the latter case, you"
          echo "*** may want to edit the sdl2-config script: $SDL2_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          CXXFLAGS="$ac_save_CXXFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     SDL2_CFLAGS=""
     SDL2_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(SDL2_CFLAGS)
  AC_SUBST(SDL2_LIBS)
  rm -f conf.sdltest
])