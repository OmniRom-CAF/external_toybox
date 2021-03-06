/* uudecode.c - uudecode / base64 decode
 *
 * Copyright 2013 Erich Plondke <toybox@erich.wreck.org>
 *
 * See http://pubs.opengroup.org/onlinepubs/9699919799/utilities/uudecode.html

USE_UUDECODE(NEWTOY(uudecode, ">1o:", TOYFLAG_USR|TOYFLAG_BIN|TOYFLAG_UMASK))

config UUDECODE
  bool "uudecode"
  default y
  help
    usage: uudecode [-o OUTFILE] [INFILE]

    Decode file from stdin (or INFILE).

    -o	write to OUTFILE instead of filename in header
*/

#define FOR_uudecode
#include "toys.h"

GLOBALS(
  char *o;
)

void uudecode_main(void)
{
  int ifd = 0, ofd, idx = 0, m = m;
  char *line = 0, mode[16],
       *class[] = {"begin%*[ ]%15s%*[ ]%n", "begin-base64%*[ ]%15s%*[ ]%n"};

  if (toys.optc) ifd = xopenro(*toys.optargs);

  while (!idx) {
    free(line);
    if (!(line = get_line(ifd))) error_exit("bad EOF");
    for (m=0; m < 2; m++) {
      sscanf(line, class[m], mode, &idx);
      if (idx) break;
    }
  }

  ofd = xcreate(TT.o ? TT.o : line+idx, O_WRONLY|O_CREAT|O_TRUNC,
    string_to_mode(mode, 0777^toys.old_umask));

  for(;;) {
    char *in, *out;
    int olen;

    free(line);
    if (m == 2 || !(line = get_line(ifd))) break;
    if (!strcmp(line, m ? "====" : "end")) {
      m = 2;
      continue;
    }

    olen = 0;
    in = out = line;
    if (!m) olen = (*(in++) - 32) & 0x3f;

    for (;;) {
      int i = 0, x = 0, len = 4;
      char c = 0;

      if (!m) {
        if (olen < 1) break;
        if (olen < 3) len = olen + 1;
      }

      while (i < len) {
        if (!(c = *(in++))) goto line_done;

        if (m) {
          if (c == '=') {
            len--;
            continue;
          } else if (len != 4) break;

          if (c >= 'A' && c <= 'Z') c -= 'A';
          else if (c >= 'a' && c <= 'z') c += 26 - 'a';
          else if (c >= '0' && c <= '9') c += 52 - '0';
          else if (c == '+') c = 62;
          else if (c == '/') c = 63;
          else continue;
        } else c = (c - 32) & 0x3f;

        x |= c << (6*(3-i));

        if (i && i < len) {
          *(out++) = (x>>(8*(3-i))) & 0xff;
          olen--;
        }
        i++;
      }

      if (i && i!=len) error_exit("bad %s", line);
    }
line_done:
    xwrite(ofd, line, out-line);
  }

  if (CFG_TOYBOX_FREE) {
    if (ifd) close(ifd);
    close(ofd);
  }
}
