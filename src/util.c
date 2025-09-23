#include "alink.h"


int get_bit_count(UINT a)
{
	int count = 0;

	while (a)
	{
		if (a & 1) count++;
		a >>= 1;
	}
	return count;
}

void clear_n_bit(PUCHAR mask, long i)
{
	mask[i >> 3] &= 0xff - (1 << (i & 0x7));
}

void set_n_bit(PUCHAR mask, long i)
{
	mask[i >> 3] |= (1 << (i & 0x7));
}

char get_n_bit(PUCHAR mask, long i)
{
	return (mask[i >> 3] >> (i & 0x7)) & 1;
}

long get_index(PUCHAR buf, long* index)
{
	long i;
	if (buf[*index] & 0x80)
	{
		i = (((UINT)(buf[*index] & 0x7f)) << 8) + buf[(*index) + 1];
		(*index) += 2;
		return i;
	}
	else
	{
		return buf[(*index)++];
	}
}

void report_error(PCHAR fname, long errnum)
{
	UINT tot, i;

	printf("\nError in file %s at %08lX", fname, file_position);
	switch (errnum)
	{
	case ERR_EXTRA_DATA:
		printf(" - extra data in record\n");
		break;
	case ERR_NO_HEADER:
		printf(" - no header record\n");
		break;
	case ERR_NO_RECDATA:
		printf(" - record data not present\n");
		break;
	case ERR_NO_MEM:
		printf(" - insufficient memory\n");
		break;
	case ERR_INV_DATA:
		printf(" - invalid data address\n");
		break;
	case ERR_INV_SEG:
		printf(" - invalid segment number\n");
		break;
	case ERR_BAD_FIXUP:
		printf(" - invalid fixup record\n");
		break;
	case ERR_BAD_SEGDEF:
		printf(" - invalid segment definition record\n");
		break;
	case ERR_ABS_SEG:
		printf(" - data emitted to absolute segment\n");
		break;
	case ERR_DUP_PUBLIC:
		printf(" - duplicate public definition\n");
		break;
	case ERR_NO_MODEND:
		printf(" - unexpected end of file (no MODEND record)\n");
		break;
	case ERR_EXTRA_HEADER:
		printf(" - duplicate module header\n");
		break;
	case ERR_UNKNOWN_RECTYPE:
		printf(" - unknown object module record type %02X\n", record_type);
		break;
	case ERR_SEG_TOO_LARGE:
		printf(" - 4Gb Non-Absolute segments not supported.\n");
		break;
	case ERR_MULTIPLE_STARTS:
		printf(" - start address defined in more than one module.\n");
		break;
	case ERR_BAD_GRPDEF:
		printf(" - illegal group definition\n");
		break;
	case ERR_OVERWRITE:
		printf(" - overlapping data regions\n");
		break;
	case ERR_INVALID_COMENT:
		printf(" - COMENT record format invalid\n");
		break;
	case ERR_ILLEGAL_IMPORTS:
		printf(" - Imports required to link, and not supported by output file type\n");
		break;
	default:
		printf("\n");
	}
	printf("name count = %i\n", name_count);
	printf("seg count = %i\n", segcount);
	printf("extcount=%i\n", extcount);
	printf("grpcount=%i\n", grpcount);
	printf("comcount=%i\n", comcount);
	printf("fixcount=%i\n", fixcount);
	printf("impcount=%i\n", impcount);
	printf("expcount=%i\n", expcount);
	printf("modcount=%i\n", nummods);

	for (i = 0, tot = 0; i < segcount; i++)
	{
		if (segment_list[i] && segment_list[i]->data) {
			tot += segment_list[i]->length;
			//printf("segment %d size=%08X\n", i, seglist[i]->length);
		}
	}
	printf("total segment size=%08X\n", tot);

	//exit(1);
}

unsigned short wtoupper(unsigned short a)
{
	if (a >= 0x100) return a;
	return toupper(a);
}

int wstricmp(const char* s1, const char* s2)
{
	int i = 0;
	unsigned short a, b;

	while (TRUE)
	{
		a = s1[i] + (s1[i + 1] << 8);
		b = s2[i] + (s2[i + 1] << 8);
		if (wtoupper(a) < wtoupper(b)) return -1;
		if (wtoupper(a) > wtoupper(b)) return +1;
		if (!a) return 0;
		i += 2;
	}
}

int wstrlen(const char* s)
{
	int i;
	for (i = 0; s[i] || s[i + 1]; i += 2);
	return i / 2;
}

int sort_compare(const void* x1, const void* x2)
{
	return strcmp(((PSORTENTRY)x1)->id, ((PSORTENTRY)x2)->id);
}

void* check_malloc(size_t x)
{
	void* p;

	p = malloc(x);
	if (!p)
	{
		fprintf(stderr, "Error, Insufficient memory in call to malloc\n");
		exit(1);
	}
	return p;
}

void* check_realloc(void* p, size_t x)
{
	p = realloc(p, x);
	if (!p)
	{
		fprintf(stderr, "Error, Insufficient memory in call to realloc\n");
		exit(1);
	}
	return p;
}

char* check_strdup(const char* s)
{
	char* scopy;

	if (!s)
	{
		fprintf(stderr, "Error, Taking duplicate of NULL pointer in call to strdup\n");
		exit(1);
	}
	scopy = (char*)malloc((strlen(s) + 1) * sizeof(char));

	if (!scopy)
	{
		fprintf(stderr, "Error, Insufficient memory in call to strdup\n");
		exit(1);
	}

	strcpy(scopy, s);

	return scopy;
}


PSORTENTRY binary_search(PSORTENTRY list, UINT count, char* key)
{
	UINT i;
	int j;

	if (!list) return NULL;
	if (!count) return NULL;
	if (!key) return NULL;

	while (count)
	{
		i = count / 2; /* get mid point */
		j = strcmp(key, list[i].id); /* compare */
		if (!j) return list + i; /* return match if found */
		if (j < 0) /* if key is less than current id */
		{
			count = i; /* only search those before current node */
		}
		else /* key is greater than current id */
		{
			list += i + 1; /* start search at next node */
			count -= (i + 1); /* count of those remaining after current node */
		}
	}

	return NULL; /* return NULL if no match (count=0) */
}

void sort_insert(PSORTENTRY* plist, UINT* pcount, char* key, void* object)
{
	PSORTENTRY list, node;
	UINT count, index, i;
	int j;

	if (!plist || !pcount) return;

	count = *pcount;
	list = *plist;

	node = binary_search(list, count, key);
	if (node) /* if ID already exists, add object to it */
	{
		node->object = (void**)check_realloc(node->object, (node->count + 1) * sizeof(void*));
		node->object[node->count] = object;
		++(node->count);
		return;
	}

	/* new node, insert into list */
	/* find index to insert at */
	index = 0;
	while (count)
	{
		i = count / 2;
		j = strcmp(key, list[index + i].id); /* compare */
		if (!j) /* match !! */
		{
			printf("Something odd is happening\n");
			exit(1);
		}

		if (j < 0) /* if key is less than current id */
		{
			count = i; /* only search those before current node */
		}
		else /* key is greater than current id */
		{
			index += i + 1; /* start search at next node */
			count -= (i + 1); /* count of those remaining after current node */
		}

	}
	/* grow list */
	count = *pcount + 1;

	list = (PSORTENTRY)check_realloc(list, sizeof(SORTENTRY) * count);

	j = count - index - 1; /* get number of entries after insertion index */
	if (j) /* move them up 1 entry if some */
	{
		memmove(list + index + 1, list + index, j * sizeof(SORTENTRY));
	}

	/* put new node in position */
	list[index].id = check_strdup(key);
	list[index].object = (void**)check_malloc(sizeof(void*));
	list[index].object[0] = object;
	list[index].count = 1;

	*pcount = count;
	*plist = list;
}
#ifndef _WIN32
char* strupr(char* s)
{
	char* tmp = s;

	for (; *tmp; ++tmp) {
		*tmp = toupper((unsigned char)*tmp);
	}

	return s;
}
#endif