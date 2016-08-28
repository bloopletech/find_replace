#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <string.h>

//from http://creativeandcritical.net/str-replace-c/
char *replace_str2(const char *str, const char *old, const char *new)
{
	char *ret, *r;
	const char *p, *q;
	size_t oldlen = strlen(old);
	size_t count, retlen, newlen = strlen(new);
	int samesize = (oldlen == newlen);

	if (!samesize) {
		for (count = 0, p = str; (q = strstr(p, old)) != NULL; p = q + oldlen)
			count++;
		/* This is undefined if p - str > PTRDIFF_MAX */
		retlen = p - str + strlen(p) + count * (newlen - oldlen);
	} else
		retlen = strlen(str);

	if ((ret = malloc(retlen + 1)) == NULL)
		return NULL;

	r = ret, p = str;
	while (1) {
		/* If the old and new strings are different lengths - in other
		 * words we have already iterated through with strstr above,
		 * and thus we know how many times we need to call it - then we
		 * can avoid the final (potentially lengthy) call to strstr,
		 * which we already know is going to return NULL, by
		 * decrementing and checking count.
		 */
		if (!samesize && !count--)
			break;
		/* Otherwise i.e. when the old and new strings are the same
		 * length, and we don't know how many times to call strstr,
		 * we must check for a NULL return here (we check it in any
		 * event, to avoid further conditions, and because there's
		 * no harm done with the check even when the old and new
		 * strings are different lengths).
		 */
		if ((q = strstr(p, old)) == NULL)
			break;
		/* This is undefined if q - p > PTRDIFF_MAX */
		ptrdiff_t l = q - p;
		memcpy(r, p, l);
		r += l;
		memcpy(r, new, newlen);
		r += newlen;
		p = q + oldlen;
	}
	strcpy(r, p);

	return ret;
}

void find_replace(char *path, char *from, char *to) {
  int fd = open(path, O_RDWR);
  if(fd == -1) {
    error(1, errno, "Error when trying to open file %s", path);
  }

  struct stat file_stat;
  if(fstat(fd, &file_stat) == -1) {
    error(1, errno, "Error when trying to get file metadata %s", path);
  }

  char *buffer = malloc(file_stat.st_size + 1);
  buffer[file_stat.st_size] = '\0';

  if(read(fd, buffer, file_stat.st_size) == -1) {
    free(buffer);
    error(1, errno, "Error when trying to read from %s", path);
  }

  if(lseek(fd, 0, SEEK_SET) == -1) {
    free(buffer);
    error(1, errno, "Error when trying to seek in %s", path);
  }

  char *replaced = replace_str2(buffer, from, to);
  free(buffer);

  if(!replaced) {
    error(1, 0, "Error trying to replace string in %s", path);
  }

  if(write(fd, replaced, strlen(replaced)) == -1) {
    free(replaced);
    error(1, errno, "Error when trying to write to %s", path);
  }

  free(replaced);

  if(close(fd) == -1) {
    error(1, errno, "Error when trying to close file %s", path);
  }
}

int main(int argc, char *argv[]) {
  if(argc < 4) {
    error(1, 0, "Usage: find_replace FROM TO file1 [file2 file3...]");
  }

  char *from = argv[1];
  char *to = argv[2];

  for(int i = 3; i < argc; ++i) {
    find_replace(argv[i], from, to);
  }
}
