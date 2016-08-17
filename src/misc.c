#include "include.h"
#include "misc.h"

char *str_find_tok(const char *str, const char *tok, char delim)
{
	char	*buf, delimstr[2];
	char	*tmp, *ptr;

    if(NULL == str || NULL == tok){
        return NULL;
    }
	if ((buf = strdup(str)) == NULL) {
		ERROR("strdup %d failed: %m\n", (int)sizeof(str));
		return NULL;
	}
	tmp = buf;
	delimstr[0] = delim;
	delimstr[1] = '\0';
	while ((ptr = strsep(&tmp, delimstr)) != NULL) {
		if (strcmp(ptr, tok) == 0) {
			tmp = buf;
			free(buf);
			return (char *)str + (ptr - tmp);
		}
	}
	free( buf);
	return NULL;
}

int str_del_tok(char **dest, const char *str, const char *tok, char delim)
{
	char	*q, *tmp;

	if(dest == NULL || str == NULL || tok == NULL){
        return ERR_WILD_POINTER;
    }

	DEBUG(100, "delete tok %s from str %s delimited by %c\n", tok, str, delim);

	if ((*dest = strdup(str)) == NULL) {
		ERROR("strdup %d failed: %m\n", (int)sizeof(str));
		return ERR_NOMEM;
	}
	if ((tmp = str_find_tok(str, tok, delim)) == NULL) {
		DEBUG(100, "no tok %s in str %s finded\n", tok, str);
		return SUCCESS;
	}
	tmp = *dest + (tmp - str);
	q = tmp;
	while (*(q++) != delim)
		;
	while (*q != '\0') {
		*(tmp++) = *(q++);
	}
	*tmp = '\0';
	DEBUG(100, "str_del_tok final result: %s\n", *dest);
	return SUCCESS;
}

int str_add_tok(char **dest, const char *str, const char *tok, char delim)
{
	char	*tmp;

	if(dest == NULL || str == NULL || tok == NULL){
        return ERR_WILD_POINTER;
    }

	if ((*dest = malloc(strlen(str) + strlen(tok) + 1)) == NULL) {
		ERROR("malloc #%d bytes mem failed\n", strlen(str)+strlen(tok)+1);
		return ERR_NOMEM;
	}
	tmp = *dest;
	strcpy(tmp, str);
	if (str_find_tok(str, tok, delim) != NULL) {
		DEBUG(100, "tok %s already exist in str %s\n", tok, str);
		return SUCCESS;
	}
	/* tok not in str */
	strcat(tmp, tok);
	*(tmp + strlen(tmp)) = delim;
	*(tmp + strlen(tmp) + 1) = '\0';
	DEBUG(100, "str_add_tok final result: %s\n", *dest);
	return SUCCESS;
}
