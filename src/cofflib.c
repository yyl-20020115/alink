#include "alink.h"

void load_coff_lib(PCHAR libname, FILE* libfile)
{
	UINT i, j;
	UINT numsyms;
	UINT modpage;
	UINT memberSize;
	UINT startPoint;
	PUCHAR endptr;
	PLIBFILE p;
	PCHAR name;
	PUCHAR modbuf;
	PSORTENTRY symlist;
	int x;

	library_files = check_realloc(library_files, (libcount + 1) * sizeof(LIBFILE));
	p = &library_files[libcount];
	p->file_name = check_malloc(strlen(libname) + 1);
	strcpy(p->file_name, libname);
	startPoint = ftell(libfile);

	if (fread(buffer, 1, 8, libfile) != 8)
	{
		printf("Error reading from file\n");
		exit(1);
	}
	buffer[8] = 0;
	/* complain if file header is wrong */
	if (strcmp(buffer, "!<arch>\n"))
	{
		printf("Invalid library file format - bad file header\n");
		printf("\"%s\"\n", buffer);

		exit(1);
	}
	/* read archive member header */
	if (fread(buffer, 1, 60, libfile) != 60)
	{
		printf("Error reading from file\n");
		exit(1);
	}
	if ((buffer[58] != 0x60) || (buffer[59] != '\n'))
	{
		printf("Invalid library file format - bad member signature\n");
		exit(1);
	}
	buffer[16] = 0;
	/* check name of first linker member */
	if (strcmp(buffer, "/               ")) /* 15 spaces */
	{
		printf("Invalid library file format - bad member name\n");
		exit(1);
	}
	buffer[58] = 0;

	/* strip trailing spaces from size */
	endptr = buffer + 57;
	while ((endptr > (buffer + 48)) && isspace(*endptr))
	{
		*endptr = 0;
		endptr--;
	}

	/* get size */
	errno = 0;
	memberSize = strtoul(buffer + 48, (PPCHAR)&endptr, 10);
	if (errno || (*endptr))
	{
		printf("Invalid library file format - bad member size\n");
		exit(1);
	}
	if ((memberSize < 4) && memberSize)
	{
		printf("Invalid library file format - bad member size\n");
		exit(1);
	}
	if (!memberSize)
	{
		numsyms = 0;
	}
	else
	{
		if (fread(buffer, 1, 4, libfile) != 4)
		{
			printf("Error reading from file\n");
			exit(1);
		}
		numsyms = buffer[3] + (buffer[2] << 8) + (buffer[1] << 16) + (buffer[0] << 24);
	}
	printf("%u symbols\n", numsyms);
	modbuf = (PUCHAR)check_malloc(numsyms * 4);

	if (numsyms)
	{
		if (fread(modbuf, 1, 4 * numsyms, libfile) != 4 * numsyms)
		{
			printf("Error reading from file\n");
			exit(1);
		}
		symlist = (PSORTENTRY)check_malloc(sizeof(SORTENTRY) * numsyms);
	}

	for (i = 0; i < numsyms; i++)
	{
		modpage = modbuf[3 + i * 4] + (modbuf[2 + i * 4] << 8) + (modbuf[1 + i * 4] << 16) + (modbuf[i * 4] << 24);

		name = NULL;
		for (j = 0; TRUE; j++)
		{
			if ((x = getc(libfile)) == EOF)
			{
				printf("Error reading from file\n");
				exit(1);
			}
			if (!x) break;
			name = (char*)check_realloc(name, j + 2);
			name[j] = x;
			name[j + 1] = 0;
		}
		if (!name)
		{
			printf("NULL name for symbol %i\n", i);
			exit(1);
		}
		if (!case_sensitive)
		{
			strupr(name);
		}

		symlist[i].id = name;
		symlist[i].count = modpage;
	}

	if (numsyms)
	{
		qsort(symlist, numsyms, sizeof(SORTENTRY), sort_compare);
		p->symbols = symlist;
		p->num_syms = numsyms;

		free(modbuf);
	}
	else
	{
		p->symbols = NULL;
		p->num_syms = 0;
	}

	/* move to an even byte boundary in the file */
	if (ftell(libfile) & 1)
	{
		fseek(libfile, 1, SEEK_CUR);
	}

	if (ftell(libfile) != (startPoint + 68 + memberSize))
	{
		printf("Invalid first linker member\n");
		printf("Pos=%08X, should be %08X\n", ftell(libfile), startPoint + 68 + memberSize);

		exit(1);
	}

	printf("Loaded first linker member\n");

	startPoint = ftell(libfile);

	/* read archive member header */
	if (fread(buffer, 1, 60, libfile) != 60)
	{
		printf("Error reading from file\n");
		exit(1);
	}
	if ((buffer[58] != 0x60) || (buffer[59] != '\n'))
	{
		printf("Invalid library file format - bad member signature\n");
		exit(1);
	}
	buffer[16] = 0;
	/* check name of second linker member */
	if (!strcmp(buffer, "/               ")) /* 15 spaces */
	{
		/* OK, so we've found it, now skip over */
		buffer[58] = 0;

		/* strip trailing spaces from size */
		endptr = buffer + 57;
		while ((endptr > (buffer + 48)) && isspace(*endptr))
		{
			*endptr = 0;
			endptr--;
		}

		/* get size */
		errno = 0;
		memberSize = strtoul(buffer + 48, (PPCHAR)&endptr, 10);
		if (errno || (*endptr))
		{
			printf("Invalid library file format - bad member size\n");
			exit(1);
		}
		if ((memberSize < 8) && memberSize)
		{
			printf("Invalid library file format - bad member size\n");
			exit(1);
		}

		/* move over second linker member */
		fseek(libfile, startPoint + 60 + memberSize, SEEK_SET);

		/* move to an even byte boundary in the file */
		if (ftell(libfile) & 1)
		{
			fseek(libfile, 1, SEEK_CUR);
		}
	}
	else
	{
		fseek(libfile, startPoint, SEEK_SET);
	}


	startPoint = ftell(libfile);
	p->long_names = NULL;

	/* read archive member header */
	if (fread(buffer, 1, 60, libfile) != 60)
	{
		printf("Error reading from file\n");
		exit(1);
	}
	if ((buffer[58] != 0x60) || (buffer[59] != '\n'))
	{
		printf("Invalid library file format - bad 3rd member signature\n");
		exit(1);
	}
	buffer[16] = 0;
	/* check name of long names linker member */
	if (!strcmp(buffer, "//              ")) /* 14 spaces */
	{
		buffer[58] = 0;

		/* strip trailing spaces from size */
		endptr = buffer + 57;
		while ((endptr > (buffer + 48)) && isspace(*endptr))
		{
			*endptr = 0;
			endptr--;
		}

		/* get size */
		errno = 0;
		memberSize = strtoul(buffer + 48, (PPCHAR)&endptr, 10);
		if (errno || (*endptr))
		{
			printf("Invalid library file format - bad member size\n");
			exit(1);
		}
		if (memberSize)
		{
			p->long_names = (PUCHAR)check_malloc(memberSize);
			if (fread(p->long_names, 1, memberSize, libfile) != memberSize)
			{
				printf("Error reading from file\n");
				exit(1);
			}
		}
	}
	else
	{
		/* if no long names member, move back to member header */
		fseek(libfile, startPoint, SEEK_SET);
	}


	p->mods_loaded = 0;
	p->mod_list = check_malloc(sizeof(USHORT) * numsyms);
	p->lib_type = 'C';
	p->block_size = 1;
	p->flags = LIBF_CASESENSITIVE;
	libcount++;
}

void load_coff_lib_mod(PCHAR fname, PLIBFILE p, FILE* libfile)
{
	char* name;
	UINT ofs;

	if (fread(buffer, 1, 60, libfile) != 60)
	{
		printf("Error reading from file\n");
		exit(1);
	}
	if ((buffer[58] != 0x60) || (buffer[59] != '\n'))
	{
		printf("Invalid library member header\n");
		exit(1);
	}
	buffer[16] = 0;
	if (buffer[0] == '/')
	{
		ofs = 15;
		while (isspace(buffer[ofs]))
		{
			buffer[ofs] = 0;
			ofs--;
		}

		ofs = strtoul(buffer + 1, &name, 10);
		if (!buffer[1] || *name)
		{
			printf("Invalid string number \n");
			exit(1);
		}
		name = p->long_names + ofs;
	}
	else
	{
		name = buffer;
	}

	printf("Loading module %s\n", name);
	load_coff(name, libfile);
}
