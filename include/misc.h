#ifndef MISC_H
#define MISC_H

char *str_find_tok(const char *str, const char *tok, char delim);
int str_del_tok(char **dest, const char *str, const char *tok, char delim);
int str_add_tok(char **dest, const char *str, const char *tok, char delim);

#endif
