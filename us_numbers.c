#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#define BUF_SIZE 1024 * 1024 * 10
#define BYTES_COUNT 5
#define MAX_DIGIT_COUNT (BYTES_COUNT * 2)
#define LINE_LENGTH (MAX_DIGIT_COUNT + 1)

inline int
usage(char *fn) {
	printf("Wrong usage:\n");
	printf("%s [input filename]\n", fn);

	return EINVAL;
}

bool
is_line_valid(char *l) {
	int i;

	for (i = 0; i < MAX_DIGIT_COUNT; ++i)
		if (l[i] < '0' || l[i] > '9')
			return false;

	if (l[MAX_DIGIT_COUNT] != '\n')
		return false;

	return true;
}

inline u_int8_t
get_byte(char *line, int i) {
	return ((line[i * 2] - '0') * 10 + (line[i * 2 + 1] - '0'));
}

// set high bit
inline u_int8_t
make_last(u_int8_t in) {
	return (in | 0x80);
}

// clear high bit
inline u_int8_t
clear_last(u_int8_t in) {
	return (in & 0x7f);
}

// do we have high bit
inline bool
is_last(u_int8_t in) {
	return (in & 0x80 ? true : false);
}

bool
process_line(char *line, u_int8_t *buf, u_int8_t **tail) {
	int i, j;
	u_int8_t cur, src;
	u_int8_t *pos = buf;
	u_int8_t stack[BYTES_COUNT];

	if (buf == *tail) {
		for (i = 0; i < BYTES_COUNT; ++i)
			pos[i] = make_last(get_byte(line, i));
		*tail += BYTES_COUNT;

		return true;
	}

	for (i = 0; i < BYTES_COUNT; ++i) {
		stack[i] = *pos;
		cur = get_byte(line, i);
		src = clear_last(*pos);

		if (cur > src) {
			if (is_last(*pos)) {
				if ((*tail + BYTES_COUNT - i) > (buf + BUF_SIZE))
					return false;

				*pos = clear_last(*pos);
				j = i;
				while (pos < *tail && !(j == i && is_last(*pos)) && j >= i) {
					if ((j + 1) == BYTES_COUNT) {
						while (is_last(stack[j]))
							--j;
					} else
						++j;

					++pos;
					stack[j] = *pos;
					src = clear_last(*pos);
				}

				memmove(pos + BYTES_COUNT - i, pos, *tail - pos);
				*tail += BYTES_COUNT - i;
				for (j = i; j < BYTES_COUNT; ++j) {
					pos[j - i] = make_last(get_byte(line, j));
				}

				return true;
			} else {
				// skip and redo "for"
				j = i;
				while ((j != i || cur > src) && !(j == i && is_last(*pos))) {
					if ((j + 1) == BYTES_COUNT) {
						while (is_last(stack[j]))
							--j;
					} else
						++j;

					++pos;
					stack[j] = *pos;
					src = clear_last(*pos);
				}

				--i;
				continue;
			}
		}

		if (cur < src) {
			if ((*tail + BYTES_COUNT - i) > (buf + BUF_SIZE))
				return false;

			memmove(pos + BYTES_COUNT - i, pos, *tail - pos);
			*tail += BYTES_COUNT - i;
			for (j = i; j < BYTES_COUNT; ++j) {
				pos[j - i] = get_byte(line, j);
				if (i != j)
					pos[j - i] = make_last(pos[j - i]);
			}

			return true;
		}

		// cur == src
		++pos;
	}

	return true;
}

void
print_data(u_int8_t *buf, u_int8_t *tail) {
	u_int8_t *pos = buf;
	u_int8_t line[BYTES_COUNT];
	int i = 0, j;

	while (pos < tail) {
		line[i] = *pos;

		if ((i + 1) == BYTES_COUNT) {
			for (j = 0; j < BYTES_COUNT; ++j)
				printf("%02d", clear_last(line[j]));
			printf("\n");

			while (is_last(line[i]))
				--i;
		} else
			++i;
		++pos;
	}

	fprintf(stderr, "Size: %ld bytes (%.2f Kbytes)\n", tail - buf, (double)(tail - buf) / 1024);
}

int
main(int argc, char **argv) {
	char line[LINE_LENGTH];
	int  tmp, line_no = 0;

	u_int8_t *buf, *tail;

	if (argc != 2)
		return usage(argv[0]);

	buf = malloc(BUF_SIZE);
	if (!buf) {
		perror("Can't allocate memory");
		return errno;
	}

	tail = buf;

	FILE *fh = fopen(argv[1], "r");
	if (!fh) {
		perror("Can't open input file");
		return errno;
	}

	while (42) {
		++line_no;
		tmp = fread(line, 1, LINE_LENGTH, fh);

		if (!tmp && feof(fh))
			break;

		if (tmp < LINE_LENGTH || !is_line_valid(line)) {
			printf("File corrupted at line: %d\n", line_no);
			break;
		}

		if (!process_line(line, buf, &tail)) {
			printf("There is no space in the buffer. Fail :(\n");
			break;
		}
	}

	print_data(buf, tail);

	free(buf);
	return fclose(fh);
}
