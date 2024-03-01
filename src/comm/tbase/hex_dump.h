#ifndef HEX_DUMP_H
#define HEX_DUMP_H
int do_hex_dump(FILE *fp, off_t curoff, const char *where, int how_much);
int hex_dump(FILE *fp, const char *where, int how_much);
#endif
