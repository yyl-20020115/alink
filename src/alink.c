#include "alink.h"

char case_sensitive = 1;
char pad_segments = 0;
char patch_near_branches = 0;
char map_file = 0;
PCHAR map_name = 0;
USHORT max_alloc = ~0;
int output_type = OUTPUT_EXE;
PCHAR out_name = 0;

FILE* a_file = 0;
UINT file_position = 0;
long record_length = 0;
UCHAR record_type = 0;
char li_le = 0;
UINT previous_offset = 0;
long previous_segment = 0;
long got_start_address = 0;
RELOC start_address;
UINT image_base = 0;
UINT file_align = 1;
UINT object_align = 1;
UINT stack_size = 4096;
UINT stack_commit_size = 4096;
UINT heap_size = 4096;
UINT heap_commit_size = 4096;
UCHAR os_major, os_minor;
UCHAR sub_system_major, sub_system_minor;
unsigned int sub_system;
int build_dll = FALSE;
PUCHAR stub_name = NULL;

long error_count = 0;
long unresolved_external_warnings_count = 0;

UCHAR buffer[0x10000];
PDATABLOCK lidata;

PPCHAR name_list = NULL;
PPSEG segment_list = NULL;
PPSEG out_list = NULL;
PPGRP group_list = NULL;
PSORTENTRY public_entries = NULL;
PEXTREC extern_records = NULL;
PPCOMREC common_definitions = NULL;
PPRELOC relocations = NULL;
PIMPREC import_definitions = NULL;
PEXPREC export_definitions = NULL;
PLIBFILE library_files = NULL;
PRESOURCE resource = NULL;
PSORTENTRY comdat_entries = NULL;
PPCHAR mod_name;
PPCHAR owner_file_name;
UINT name_count = 0, name_min = 0,
pubcount = 0, pubmin = 0,
segcount = 0, segmin = 0, outcount = 0,
segcount_combined = 0,
grpcount = 0, grpmin = 0,
grpcount_combined = 0,
extcount = 0, extmin = 0,
comcount = 0, commin = 0,
fixcount = 0, fixmin = 0,
impcount = 0, impmin = 0, impsreq = 0,
expcount = 0, expmin = 0,
nummods = 0,
filecount = 0,
libcount = 0,
rescount = 0;
UINT lib_path_count = 0;
PCHAR* lib_path = NULL;
char* entry_point_function_name = NULL;

static BOOL process_command_line(int argc, char** argv)
{
	long i, j;
	int helpRequested = FALSE;
	UINT setbase, setfalign, setoalign;
	UINT setstack, setstackcommit, setheap, setheapcommit;
	int setsubsysmajor, setsubsysminor, setosmajor, setosminor;
	UCHAR setsubsys;
	int gotbase = FALSE, gotfalign = FALSE, gotoalign = FALSE, gotsubsys = FALSE;
	int gotstack = FALSE, gotstackcommit = FALSE, gotheap = FALSE, gotheapcommit = FALSE;
	int gotsubsysver = FALSE, gotosver = FALSE;
	char* p = 0, * q = 0;
	int c;
	char** newargs;
	FILE* argFile;

	for (i = 1; i < argc; i++)
	{
		/* cater for response files */
		if (argv[i][0] == '@')
		{
			argFile = fopen(argv[i] + 1, "rt");
			if (!argFile)
			{
				printf("Unable to open response file \"%s\"\n", argv[i] + 1);
				return FALSE;
				//NOTICE:
				//exit(1);
			}
			newargs = (char**)check_malloc(argc * sizeof(char*));
			for (j = 0; j < argc; j++)
			{
				newargs[j] = argv[j];
			}
			p = NULL;
			j = 0;
			while ((c = fgetc(argFile)) != EOF)
			{
				if (c == ';') /* allow comments, starting with ; */
				{
					while (((c = fgetc(argFile)) != EOF) && (c != '\n')); /* loop until end of line */
					/* continue main loop */
					continue;
				}
				if (isspace(c))
				{
					if (p) /* if we've got an argument, add to list */
					{
						newargs = (char**)check_realloc(newargs, (argc + 1) * sizeof(char*));
						newargs[argc] = p;
						argc++;
						/* clear pointer and length indicator */
						p = NULL;
						j = 0;
					}
					/* and continue */
					continue;
				}
				if (c == '"')
				{
					/* quoted strings */
					while (((c = fgetc(argFile)) != EOF) && (c != '"')) /* loop until end of string */
					{
						if (c == '\\')
						{
							c = fgetc(argFile);
							if (c == EOF)
							{
								printf("Missing character to escape in quoted string, unexpected end of file found\n");
								return FALSE;
								//NOTICE:
								//exit(1);
							}
						}

						p = (char*)check_realloc(p, j + 2);
						p[j] = c;
						j++;
						p[j] = 0;
					}
					if (c == EOF)
					{
						printf("Unexpected end of file encountered in quoted string\n");
						return FALSE;
						//NOTICE:
						//exit(1);
					}

					/* continue main loop */
					continue;
				}
				/* if no special case, then add to string */
				p = (char*)check_realloc(p, j + 2);
				p[j] = c;
				j++;
				p[j] = 0;
			}
			if (p)
			{
				newargs = (char**)check_realloc(newargs, (argc + 1) * sizeof(char*));
				newargs[argc] = p;
				argc++;
			}
			fclose(argFile);
			argv = newargs;
		}
		else if (argv[i][0] == SWITCHCHAR)
		{
			if (strlen(argv[i]) < 2)
			{
				printf("Invalid argument \"%s\"\n", argv[i]);
				return FALSE;
				//NOTICE:
				//exit(1);
			}
			switch (argv[i][1])
			{
			case 'c':
				if (strlen(argv[i]) == 2)
				{
					case_sensitive = 1;
					break;
				}
				else if (strlen(argv[i]) == 3)
				{
					if (argv[i][2] == '+')
					{
						case_sensitive = 1;
						break;
					}
					else if (argv[i][2] == '-')
					{
						case_sensitive = 0;
						break;
					}
				}
				printf("Invalid switch %s\n", argv[i]);
				return FALSE;
				//NOTICE:
				//exit(1);
				break;
			case 'p':
				switch (strlen(argv[i]))
				{
				case 2:
					pad_segments = 1;
					break;
				case 3:
					if (argv[i][2] == '+')
					{
						pad_segments = 1;
						break;
					}
					else if (argv[i][2] == '-')
					{
						pad_segments = 0;
						break;
					}
				default:
					printf("Invalid switch %s\n", argv[i]);
					return FALSE;
					//NOTICE:
					//exit(1);
				}
				break;
			case 'z':
				switch (strlen(argv[i]))
				{
				case 2:
					patch_near_branches = 1;
					break;
				case 3:
					if (argv[i][2] == '+')
					{
						patch_near_branches = 1;
						break;
					}
					else if (argv[i][2] == '-')
					{
						patch_near_branches = 0;
						break;
					}
				default:
					printf("Invalid switch %s\n", argv[i]);
					return FALSE;
					//NOTICE:
					//exit(1);
				}
				break;
			case 'm':
				switch (strlen(argv[i]))
				{
				case 2:
					map_file = 1;
					break;
				case 3:
					if (argv[i][2] == '+')
					{
						map_file = 1;
						break;
					}
					else if (argv[i][2] == '-')
					{
						map_file = 0;
						break;
					}
				default:
					printf("Invalid switch %s\n", argv[i]);
					return FALSE;
					//NOTICE:
					//exit(1);
				}
				break;
			case 'o':
				switch (strlen(argv[i]))
				{
				case 2:
					if (i < (argc - 1))
					{
						i++;
						if (!out_name)
						{
							out_name = check_malloc(strlen(argv[i]) + 1 + 4); /* space for added .EXT if none given */
							strcpy(out_name, argv[i]);
						}
						else
						{
							printf("Can't specify two output names\n");
							return FALSE;
							//NOTICE:
							//exit(1);
						}
					}
					else
					{
						printf("Invalid switch %s\n", argv[i]);
						return FALSE;
						//NOTICE:
						//exit(1);
					}
					break;
				default:
					if (!strcasecmp(argv[i] + 2, "EXE"))
					{
						output_type = OUTPUT_EXE;
						image_base = 0;
						file_align = 1;
						object_align = 1;
						stack_size = 0;
						stack_commit_size = 0;
						heap_size = 0;
						heap_commit_size = 0;
					}
					else if (!strcasecmp(argv[i] + 2, "COM"))
					{
						output_type = OUTPUT_COM;
						image_base = 0;
						file_align = 1;
						object_align = 1;
						stack_size = 0;
						stack_commit_size = 0;
						heap_size = 0;
						heap_commit_size = 0;
					}
					else if (!strcasecmp(argv[i] + 2, "PE"))
					{
						output_type = OUTPUT_PE;
						image_base = WIN32_DEFAULT_BASE;
						file_align = WIN32_DEFAULT_FILEALIGN;
						object_align = WIN32_DEFAULT_OBJECTALIGN;
						stack_size = WIN32_DEFAULT_STACKSIZE;
						stack_commit_size = WIN32_DEFAULT_STACKCOMMITSIZE;
						heap_size = WIN32_DEFAULT_HEAPSIZE;
						heap_commit_size = WIN32_DEFAULT_HEAPCOMMITSIZE;
						sub_system = WIN32_DEFAULT_SUBSYS;
						sub_system_major = WIN32_DEFAULT_SUBSYSMAJOR;
						sub_system_minor = WIN32_DEFAULT_SUBSYSMINOR;
						os_major = WIN32_DEFAULT_OSMAJOR;
						os_minor = WIN32_DEFAULT_OSMINOR;
					}
					else if (!strcasecmp(argv[i] + 1, "objectalign"))
					{
						if (i < (argc - 1))
						{
							i++;
							setoalign = strtoul(argv[i], &p, 0);
							if (p[0]) /* if not at end of arg */
							{
								printf("Bad object alignment\n");
								return FALSE;
								//NOTICE:
								//exit(1);
							}
							if ((setoalign < 0x200) || (setoalign > (0x100 * 1048576))
								|| (get_bit_count(setoalign) > 1))
							{
								printf("Bad object alignment\n");
								return FALSE;
								//NOTICE:
								//exit(1);
							}
							gotoalign = TRUE;
						}
						else
						{
							printf("Invalid switch %s\n", argv[i]);
							return FALSE;
							//NOTICE:
							//exit(1);
						}
					}
					else if (!strcasecmp(argv[i] + 1, "osver"))
					{
						if (i < (argc - 1))
						{
							i++;
							if (sscanf(argv[i], "%d.%d%n", &setosmajor, &setosminor, &j) != 2)
							{
								printf("Invalid version number %s\n", argv[i]);
								return FALSE;
								//NOTICE:
								//exit(1);
							}
							if ((j != strlen(argv[i])) || (setosmajor < 0) || (setosminor < 0)
								|| (setosmajor > 0x10000) || (setosminor > 0x10000))
							{
								printf("Invalid version number %s\n", argv[i]);
								return FALSE;
								//NOTICE:
								//exit(1);
							}
							gotosver = TRUE;
						}
						else
						{
							printf("Invalid switch %s\n", argv[i]);
							return FALSE;
							//NOTICE:
							//exit(1);
						}
						break;
					}
					else
					{
						printf("Invalid switch %s\n", argv[i]);
						return FALSE;
						//NOTICE:
						//exit(1);
					}
					break;
				}
				break;
			case 'L':
				if (strlen(argv[i]) == 2)
				{
					if (i < (argc - 1))
					{
						i++;
						lib_path_count++;
						lib_path = (PCHAR*)check_realloc(lib_path, lib_path_count * sizeof(PCHAR));
						j = (long)strlen(argv[i]);
						if (argv[i][j - 1] != PATH_CHAR)
						{
							/* append a path separator if not present */
							lib_path[lib_path_count - 1] = (char*)check_malloc(j + 2);
							strcpy(lib_path[lib_path_count - 1], argv[i]);
							lib_path[lib_path_count - 1][j] = PATH_CHAR;
							lib_path[lib_path_count - 1][j + 1] = 0;
						}
						else
						{
							lib_path[lib_path_count - 1] = check_strdup(argv[i]);
						}
					}
					else
					{
						printf("Invalid switch %s\n", argv[i]);
						return FALSE;
						//NOTICE:
						//exit(1);
					}
					break;
				}
				printf("Invalid switch %s\n", argv[i]);
				return FALSE;
				//NOTICE:
				//exit(1);
				break;
			case 'h':
			case 'H':
			case '?':
				if (strlen(argv[i]) == 2)
				{
					helpRequested = TRUE;
				}
				else if (!strcasecmp(argv[i] + 1, "heapsize"))
				{
					if (i < (argc - 1))
					{
						i++;
						setheap = strtoul(argv[i], &p, 0);
						if (p[0]) /* if not at end of arg */
						{
							printf("Bad heap size\n");
							return FALSE;
							//NOTICE:
							//exit(1);
						}
						gotheap = TRUE;
					}
					else
					{
						printf("Invalid switch %s\n", argv[i]);
						return FALSE;
						//NOTICE:
						//exit(1);
					}
					break;
				}
				else if (!strcasecmp(argv[i] + 1, "heapcommitsize"))
				{
					if (i < (argc - 1))
					{
						i++;
						setheapcommit = strtoul(argv[i], &p, 0);
						if (p[0]) /* if not at end of arg */
						{
							printf("Bad heap commit size\n");
							return FALSE;
							//NOTICE:
							//exit(1);
						}
						gotheapcommit = TRUE;
					}
					else
					{
						printf("Invalid switch %s\n", argv[i]);
						return FALSE;
						//NOTICE:
						//exit(1);
					}
					break;
				}
				break;
			case 'b':
				if (!strcasecmp(argv[i] + 1, "base"))
				{
					if (i < (argc - 1))
					{
						i++;
						setbase = strtoul(argv[i], &p, 0);
						if (p[0]) /* if not at end of arg */
						{
							printf("Bad image base\n");
							return FALSE;
							//NOTICE:
							//exit(1);
						}
						if (setbase & 0xffff)
						{
							printf("Bad image base\n");
							return FALSE;
							//NOTICE:
							//exit(1);
						}
						gotbase = TRUE;
					}
					else
					{
						printf("Invalid switch %s\n", argv[i]);
						return FALSE;
						//NOTICE:
						//exit(1);
					}
					break;
				}
				else
				{
					printf("Invalid switch %s\n", argv[i]);
					return FALSE;
					//NOTICE:
					//exit(1);
				}
				break;
			case 's':
				if (!strcasecmp(argv[i] + 1, "subsys"))
				{
					if (i < (argc - 1))
					{
						i++;
						if (!strcasecmp(argv[i], "gui")
							|| !strcasecmp(argv[i], "windows")
							|| !strcasecmp(argv[i], "win"))
						{
							setsubsys = PE_SUBSYS_WINDOWS;
							gotsubsys = TRUE;
						}
						else if (!strcasecmp(argv[i], "char")
							|| !strcasecmp(argv[i], "console")
							|| !strcasecmp(argv[i], "con"))
						{
							setsubsys = PE_SUBSYS_CONSOLE;
							gotsubsys = TRUE;
						}
						else if (!strcasecmp(argv[i], "native"))
						{
							setsubsys = PE_SUBSYS_NATIVE;
							gotsubsys = TRUE;
						}
						else if (!strcasecmp(argv[i], "posix"))
						{
							setsubsys = PE_SUBSYS_POSIX;
							gotsubsys = TRUE;
						}
						else
						{
							printf("Invalid subsystem id %s\n", argv[i]);
							return FALSE;
							//NOTICE:
							//exit(1);
						}
					}
					else
					{
						printf("Invalid switch %s\n", argv[i]);
						return FALSE;
						//NOTICE:
						//exit(1);
					}
					break;
				}
				else if (!strcasecmp(argv[i] + 1, "subsysver"))
				{
					if (i < (argc - 1))
					{
						i++;
						if (sscanf(argv[i], "%d.%d%n", &setsubsysmajor, &setsubsysminor, &j) != 2)
						{
							printf("Invalid version number %s\n", argv[i]);
							return FALSE;
							//NOTICE:
							//exit(1);
						}
						if ((j != strlen(argv[i])) || (setsubsysmajor < 0) || (setsubsysminor < 0)
							|| (setsubsysmajor > 0x10000) || (setsubsysminor > 0x10000))
						{
							printf("Invalid version number %s\n", argv[i]);
							return FALSE;
							//NOTICE:
							//exit(1);
						}
						gotsubsysver = TRUE;
					}
					else
					{
						printf("Invalid switch %s\n", argv[i]);
						return FALSE;
						//NOTICE:
						//exit(1);
					}
					break;
				}
				else if (!strcasecmp(argv[i] + 1, "stacksize"))
				{
					if (i < (argc - 1))
					{
						i++;
						setstack = strtoul(argv[i], &p, 0);
						if (p[0]) /* if not at end of arg */
						{
							printf("Bad stack size\n");
							return FALSE;
							//NOTICE:
							//exit(1);
						}
						gotstack = TRUE;
					}
					else
					{
						printf("Invalid switch %s\n", argv[i]);
						return FALSE;
						//NOTICE:
						//exit(1);
					}
					break;
				}
				else if (!strcasecmp(argv[i] + 1, "stackcommitsize"))
				{
					if (i < (argc - 1))
					{
						i++;
						setstackcommit = strtoul(argv[i], &p, 0);
						if (p[0]) /* if not at end of arg */
						{
							printf("Bad stack commit size\n");
							return FALSE;
							//NOTICE:
							//exit(1);
						}
						gotstackcommit = TRUE;
					}
					else
					{
						printf("Invalid switch %s\n", argv[i]);
						return FALSE;
						//NOTICE:
						//exit(1);
					}
					break;
				}
				else if (!strcasecmp(argv[i] + 1, "stub"))
				{
					if (i < (argc - 1))
					{
						i++;
						stub_name = argv[i];
					}
					else
					{
						printf("Invalid switch %s\n", argv[i]);
						return FALSE;
						//NOTICE:
						//exit(1);
					}
					break;
				}
				else
				{
					printf("Invalid switch %s\n", argv[i]);
					return FALSE;
					//NOTICE:
					//exit(1);
				}
				break;
			case 'f':
				if (!strcasecmp(argv[i] + 1, "filealign"))
				{
					if (i < (argc - 1))
					{
						i++;
						setfalign = strtoul(argv[i], &p, 0);
						if (p[0]) /* if not at end of arg */
						{
							printf("Bad file alignment\n");
							return FALSE;
							//NOTICE:
							//exit(1);
						}
						if ((setfalign < 512) || (setfalign > 0x10000)
							|| (get_bit_count(setfalign) > 1))
						{
							printf("Bad file alignment\n");
							return FALSE;
							//NOTICE:
							//exit(1);
						}
						gotfalign = TRUE;
					}
					else
					{
						printf("Invalid switch %s\n", argv[i]);
						return FALSE;
						//NOTICE:
						//exit(1);
					}
				}
				else
				{
					printf("Invalid switch %s\n", argv[i]);
					return FALSE;
					//NOTICE:
					//exit(1);
				}
				break;
			case 'd':
				if (!strcasecmp(argv[i] + 1, "dll"))
				{
					build_dll = TRUE;
				}
				else
				{
					printf("Invalid switch %s\n", argv[i]);
					return FALSE;
					//NOTICE:
					//exit(1);
				}
				break;
			case 'e':
				if (!strcasecmp(argv[i] + 1, "entry"))
				{
					if (i < (argc - 1))
					{
						i++;
						entry_point_function_name = argv[i];
					}
					else
					{
						printf("Invalid switch %s\n", argv[i]);
						return FALSE;
						//NOTICE:
						//exit(1);
					}
				}
				else
				{
					printf("Invalid switch %s\n", argv[i]);
					return FALSE;
					//NOTICE:
					//exit(1);
				}
				break;

			default:
				printf("Invalid switch %s\n", argv[i]);
				return FALSE;
				//NOTICE:
				//exit(1);
			}
		}
		else
		{
			owner_file_name = check_realloc(owner_file_name, (filecount + 1) * sizeof(PCHAR));
			owner_file_name[filecount] = check_malloc(strlen(argv[i]) + 1);
			memcpy(owner_file_name[filecount], argv[i], strlen(argv[i]) + 1);
			for (j = (long)strlen(owner_file_name[filecount]);
				j && (owner_file_name[filecount][j] != '.') &&
				(owner_file_name[filecount][j] != PATH_CHAR);
				j--);
			if ((j < 0) || (owner_file_name[filecount][j] != '.'))
			{
				j = (long)strlen(owner_file_name[filecount]);
				/* add default extension if none specified */
				owner_file_name[filecount] = check_realloc(owner_file_name[filecount], strlen(argv[i]) + 5);
				strcpy(owner_file_name[filecount] + j, DEFAULT_EXTENSION);
			}
			filecount++;
		}
	}
	if (helpRequested || !filecount)
	{
		printf("Usage: ALINK [file [file [...]]] [options]\n");
		printf("\n");
		printf("    Each file may be an object file, a library, or a Win32 resource\n");
		printf("    file. If no extension is specified, .obj is assumed. Modules are\n");
		printf("    only loaded from library files if they are required to match an\n");
		printf("    external reference.\n");
		printf("    Options and files may be listed in any order, all mixed together.\n");
		printf("\n");
		printf("The following options are permitted:\n");
		printf("\n");
		printf("    @name   Load additional options from response file name\n");
		printf("    -c      Enable Case sensitivity\n");
		printf("    -c+     Enable Case sensitivity\n");
		printf("    -c-     Disable Case sensitivity\n");
		printf("    -p      Enable segment padding\n");
		printf("    -p+     Enable segment padding\n");
		printf("    -p-     Disable segment padding\n");
		printf("    -m      Enable map file\n");
		printf("    -m+     Enable map file\n");
		printf("    -m-     Disable map file\n");
		printf("    -x      Enable patching for near call or jumps\n");
		printf("    -x+     Enable patching for near call or jumps\n");
		printf("    -x-     Disable patching for near call or jumps\n");
		printf("----Press Enter to continue---");
		while (((c = getchar()) != '\n') && (c != EOF));
		printf("\n");
		printf("    -h      Display this help list\n");
		printf("    -H      \"\n");
		printf("    -?      \"\n");
		printf("    -L ddd  Add directory ddd to search list\n");
		printf("    -o name Choose output file name\n");
		printf("    -oXXX   Choose output format XXX\n");
		printf("        Available options are:\n");
		printf("            COM - MSDOS COM file\n");
		printf("            EXE - MSDOS EXE file\n");
		printf("            PE  - Win32 PE Executable\n");
		printf("    -entry name   Use public symbol name as the entry point\n");
		printf("----Press Enter to continue---");
		while (((c = getchar()) != '\n') && (c != EOF));
		printf("\nOptions for PE files:\n");
		printf("    -base addr        Set base address of image\n");
		printf("    -filealign addr   Set section alignment in file\n");
		printf("    -objectalign addr Set section alignment in memory\n");
		printf("    -subsys xxx       Set subsystem used\n");
		printf("        Available options are:\n");
		printf("            console   Select character mode\n");
		printf("            con       \"\n");
		printf("            char      \"\n");
		printf("            windows   Select windowing mode\n");
		printf("            win       \"\n");
		printf("            gui       \"\n");
		printf("            native    Select native mode\n");
		printf("            posix     Select POSIX mode\n");
		printf("    -subsysver x.y    Select subsystem version x.y\n");
		printf("    -osver x.y        Select OS version x.y\n");
		printf("    -stub xxx         Use xxx as the MSDOS stub\n");
		printf("    -dll              Build DLL instead of EXE\n");
		printf("    -stacksize xxx    Set stack size to xxx\n");
		printf("    -stackcommitsize xxx Set stack commit size to xxx\n");
		printf("    -heapsize xxx     Set heap size to xxx\n");
		printf("    -heapcommitsize xxx Set heap commit size to xxx\n");
		return TRUE;
		//exit(0);
	}
	if ((output_type != OUTPUT_PE) &&
		(gotoalign || gotfalign || gotbase || gotsubsys || gotstack ||
			gotstackcommit || gotheap || gotheapcommit || build_dll || stub_name ||
			gotsubsysver || gotosver))
	{
		printf("Option not supported for non-PE output formats\n");
		return FALSE;
		//NOTICE:
		//exit(1);
	}
	if (gotstack)
	{
		stack_size = setstack;
	}
	if (gotstackcommit)
	{
		stack_commit_size = setstackcommit;
	}
	if (stack_commit_size > stack_size)
	{
		printf("Stack commit size is greater than stack size, committing whole stack\n");
		stack_commit_size = stack_size;
	}
	if (gotheap)
	{
		heap_size = setheap;
	}
	if (gotheapcommit)
	{
		heap_commit_size = setheapcommit;
	}
	if (heap_commit_size > heap_size)
	{
		printf("Heap commit size is greater than heap size, committing whole heap\n");
		heap_commit_size = heap_size;
	}
	if (gotoalign)
	{
		object_align = setoalign;
	}
	if (gotfalign)
	{
		file_align = setfalign;
	}
	if (gotbase)
	{
		image_base = setbase;
	}
	if (gotsubsys)
	{
		sub_system = setsubsys;
	}
	if (gotsubsysver)
	{
		sub_system_major = setsubsysmajor;
		sub_system_minor = setsubsysminor;
	}
	if (gotosver)
	{
		os_major = setosmajor;
		os_minor = setosminor;
	}
	return TRUE;
}

static void match_externs()
{
	long i, j, k, old_nummods;
	int n = 0;
	PSORTENTRY listnode;
	PCHAR name;
	PPUBLIC pubdef;

	do
	{
		for (i = 0; i < expcount; i++)
		{
			if (export_definitions[i].pubdef) continue;
			if (listnode = binary_search(public_entries, pubcount, export_definitions[i].int_name))
			{
				for (k = 0; k < listnode->count; k++)
				{
					/* exports can only match global publics */
					if (((PPUBLIC)listnode->object[k])->mod == 0)
					{
						export_definitions[i].pubdef = (PPUBLIC)listnode->object[k];
						break;
					}
				}
			}
		}
		for (i = 0; i < extcount; i++)
		{
			/* skip if we've already matched a public symbol */
			/* as they override all others */
			if (extern_records[i].flags == EXT_MATCHEDPUBLIC) continue;
			extern_records[i].flags = EXT_NOMATCH;
			if (listnode = binary_search(public_entries, pubcount, extern_records[i].name))
			{
				for (k = 0; k < listnode->count; k++)
				{
					/* local publics can only match externs in same module */
					/* and global publics can only match global externs */
					if (((PPUBLIC)listnode->object[k])->mod == extern_records[i].mod)
					{
						extern_records[i].pubdef = (PPUBLIC)listnode->object[k];
						extern_records[i].flags = EXT_MATCHEDPUBLIC;
						break;
					}
				}
			}
			if (extern_records[i].flags == EXT_NOMATCH)
			{
				for (j = 0; j < impcount; j++)
				{
					if (!strcmp(extern_records[i].name, import_definitions[j].int_name)
						|| ((case_sensitive == 0) &&
							!strcasecmp(extern_records[i].name, import_definitions[j].int_name)))
					{
						extern_records[i].flags = EXT_MATCHEDIMPORT;
						extern_records[i].import = j;
						impsreq++;
					}
				}
			}
			if (extern_records[i].flags == EXT_NOMATCH)
			{
				for (j = 0; j < expcount; j++)
				{
					if (!export_definitions[j].pubdef) continue;
					if (!strcmp(extern_records[i].name, export_definitions[j].exp_name)
						|| ((case_sensitive == 0) &&
							!strcasecmp(extern_records[i].name, export_definitions[j].exp_name)))
					{
						extern_records[i].pubdef = export_definitions[j].pubdef;
						extern_records[i].flags = EXT_MATCHEDPUBLIC;
					}
				}
			}
		}

		old_nummods = nummods;
		for (i = 0; (i < expcount) && (nummods == old_nummods); i++)
		{
			if (!export_definitions[i].pubdef)
			{
				for (k = 0; k < libcount; ++k)
				{
					name = check_strdup(export_definitions[i].int_name);
					if (!(library_files[k].flags & LIBF_CASESENSITIVE))
					{
						strupr(name);
					}

					if (listnode = binary_search(library_files[k].symbols, library_files[k].num_syms, name))
					{
						load_lib_mod(k, listnode->count);
						break;
					}
					free(name);
				}
			}
		}
		for (i = 0; (i < extcount) && (nummods == old_nummods); i++)
		{
			if (extern_records[i].flags == EXT_NOMATCH)
			{
				for (k = 0; k < libcount; ++k)
				{
					name = check_strdup(extern_records[i].name);
					if (!(library_files[k].flags & LIBF_CASESENSITIVE))
					{
						strupr(name);
					}

					if (listnode = binary_search(library_files[k].symbols, library_files[k].num_syms, name))
					{
						load_lib_mod(k, listnode->count);
						break;
					}
					free(name);
				}
			}
		}
		for (i = 0; (i < pubcount) && (nummods == old_nummods); ++i)
		{
			for (k = 0; k < public_entries[i].count; ++k)
			{
				pubdef = (PPUBLIC)public_entries[i].object[k];
				if (!pubdef->alias) continue;
				if (listnode = binary_search(public_entries, pubcount, pubdef->alias))
				{
					for (j = 0; j < listnode->count; j++)
					{
						if ((((PPUBLIC)listnode->object[j])->mod == pubdef->mod)
							&& !((PPUBLIC)listnode->object[j])->alias)
						{
							/* if we've found a match for the alias, then kill the alias */
							free(pubdef->alias);
							(*pubdef) = (*((PPUBLIC)listnode->object[j]));
							break;
						}
					}
				}
				if (!pubdef->alias) continue;
				for (k = 0; k < libcount; ++k)
				{
					name = check_strdup(pubdef->alias);
					if (!(library_files[k].flags & LIBF_CASESENSITIVE))
					{
						strupr(name);
					}

					if (listnode = binary_search(library_files[k].symbols, library_files[k].num_syms, name))
					{
						load_lib_mod(k, listnode->count);
						break;
					}
					free(name);
				}

			}
		}

	} while (old_nummods != nummods);
}

static void match_comdefs()
{
	int i, j;// , k;
	int comseg;
	int comfarseg;
	PSORTENTRY listnode;
	PPUBLIC pubdef;

	if (!comcount) return;

	for (i = 0; i < comcount; i++)
	{
		if (!common_definitions[i]) continue;
		for (j = 0; j < i; j++)
		{
			if (!common_definitions[j]) continue;
			if (common_definitions[i]->modnum != common_definitions[j]->modnum) continue;
			if (strcmp(common_definitions[i]->name, common_definitions[j]->name) == 0)
			{
				if (common_definitions[i]->is_far != common_definitions[j]->is_far)
				{
					printf("Mismatched near/far type for COMDEF %s\n", common_definitions[i]->name);
					exit(1);
				}
				if (common_definitions[i]->length > common_definitions[j]->length)
					common_definitions[j]->length = common_definitions[i]->length;
				free(common_definitions[i]->name);
				free(common_definitions[i]);
				common_definitions[i] = 0;
				break;
			}
		}
	}

	for (i = 0; i < comcount; i++)
	{
		if (!common_definitions[i]) continue;
		if (listnode = binary_search(public_entries, pubcount, common_definitions[i]->name))
		{
			for (j = 0; j < listnode->count; j++)
			{
				/* local publics can only match externs in same module */
				/* and global publics can only match global externs */
				if ((((PPUBLIC)listnode->object[j])->mod == common_definitions[i]->modnum)
					&& !((PPUBLIC)listnode->object[j])->alias)
				{
					free(common_definitions[i]->name);
					free(common_definitions[i]);
					common_definitions[i] = 0;
					break;
				}
			}
		}
	}

	segment_list = (PPSEG)check_realloc(segment_list, (segcount + 1) * sizeof(PSEG));
	segment_list[segcount] = (PSEG)check_malloc(sizeof(SEG));
	name_list = (PPCHAR)check_realloc(name_list, (name_count + 1) * sizeof(PCHAR));
	name_list[name_count] = check_strdup("COMDEFS");
	segment_list[segcount]->name_index = name_count;
	segment_list[segcount]->class_index = -1;
	segment_list[segcount]->overlay_index = -1;
	segment_list[segcount]->length = 0;
	segment_list[segcount]->data = NULL;
	segment_list[segcount]->data_mask = NULL;
	segment_list[segcount]->attributes = SEG_PRIVATE | SEG_PARA;
	segment_list[segcount]->win_flags = WINF_READABLE | WINF_WRITEABLE | WINF_NEG_FLAGS;
	comseg = segcount;
	segcount++;
	name_count++;


	for (i = 0; i < grpcount; i++)
	{
		if (!group_list[i]) continue;
		if (group_list[i]->name_index < 0) continue;
		if (!strcmp("DGROUP", name_list[group_list[i]->name_index]))
		{
			if (group_list[i]->numsegs == 0) continue; /* don't add to an emtpy group */
			/* because empty groups are special */
			/* else add to group */
			group_list[i]->segindex[group_list[i]->numsegs] = comseg;
			group_list[i]->numsegs++;
			break;
		}
	}

	segment_list = (PPSEG)check_realloc(segment_list, (segcount + 1) * sizeof(PSEG));
	segment_list[segcount] = (PSEG)check_malloc(sizeof(SEG));
	name_list = (PPCHAR)check_realloc(name_list, (name_count + 1) * sizeof(PCHAR));
	name_list[name_count] = check_strdup("FARCOMDEFS");
	segment_list[segcount]->name_index = name_count;
	segment_list[segcount]->class_index = -1;
	segment_list[segcount]->overlay_index = -1;
	segment_list[segcount]->length = 0;
	segment_list[segcount]->data = NULL;
	segment_list[segcount]->data_mask = NULL;
	segment_list[segcount]->attributes = SEG_PRIVATE | SEG_PARA;
	segment_list[segcount]->win_flags = WINF_READABLE | WINF_WRITEABLE | WINF_NEG_FLAGS;
	name_count++;
	comfarseg = segcount;
	segcount++;

	for (i = 0; i < comcount; i++)
	{
		if (!common_definitions[i]) continue;
		pubdef = (PPUBLIC)check_malloc(sizeof(PUBLIC));
		if (common_definitions[i]->is_far)
		{
			if (common_definitions[i]->length > 0x10000)
			{
				segment_list = (PPSEG)check_realloc(segment_list, (segcount + 1) * sizeof(PSEG));
				segment_list[segcount] = (PSEG)check_malloc(sizeof(SEG));
				name_list = (PPCHAR)check_realloc(name_list, (name_count + 1) * sizeof(PCHAR));
				name_list[name_count] = check_strdup("FARCOMDEFS");
				segment_list[segcount]->name_index = name_count;
				segment_list[segcount]->class_index = -1;
				segment_list[segcount]->overlay_index = -1;
				segment_list[segcount]->length = common_definitions[i]->length;
				segment_list[segcount]->data = NULL;
				segment_list[segcount]->data_mask =
					(PUCHAR)check_malloc((common_definitions[i]->length + 7) >> 3);
				for (j = 0; j < (common_definitions[i]->length + 7) >> 3; j++)
					segment_list[segcount]->data_mask[j] = 0;
				segment_list[segcount]->attributes = SEG_PRIVATE | SEG_PARA;
				segment_list[segcount]->win_flags = WINF_READABLE | WINF_WRITEABLE | WINF_NEG_FLAGS;
				name_count++;
				pubdef->segment = segcount;
				segcount++;
				pubdef->offset = 0;
			}
			else if ((common_definitions[i]->length + segment_list[comfarseg]->length) > 0x10000)
			{
				segment_list[comfarseg]->data_mask =
					(PUCHAR)check_malloc((segment_list[comfarseg]->length + 7) >>3);
				for (j = 0; j < (segment_list[comfarseg]->length + 7) >>3; j++)
					segment_list[comfarseg]->data_mask[j] = 0;

				segment_list = (PPSEG)check_realloc(segment_list, (segcount + 1) * sizeof(PSEG));
				segment_list[segcount] = (PSEG)check_malloc(sizeof(SEG));
				name_list = (PPCHAR)check_realloc(name_list, (name_count + 1) * sizeof(PCHAR));
				name_list[name_count] = check_strdup("FARCOMDEFS");
				segment_list[segcount]->name_index = name_count;
				segment_list[segcount]->class_index = -1;
				segment_list[segcount]->overlay_index = -1;
				segment_list[segcount]->length = common_definitions[i]->length;
				segment_list[segcount]->data = NULL;
				segment_list[segcount]->data_mask = NULL;
				segment_list[segcount]->attributes = SEG_PRIVATE | SEG_PARA;
				segment_list[segcount]->win_flags = WINF_READABLE | WINF_WRITEABLE | WINF_NEG_FLAGS;
				comfarseg = segcount;
				segcount++;
				name_count++;
				pubdef->segment = comfarseg;
				pubdef->offset = 0;
			}
			else
			{
				pubdef->segment = comfarseg;
				pubdef->offset = segment_list[comfarseg]->length;
				segment_list[comfarseg]->length += common_definitions[i]->length;
			}
		}
		else
		{
			pubdef->segment = comseg;
			pubdef->offset = segment_list[comseg]->length;
			segment_list[comseg]->length += common_definitions[i]->length;
		}
		pubdef->mod = common_definitions[i]->modnum;
		pubdef->group = -1;
		pubdef->type = 0;
		pubdef->alias = NULL;
		if (listnode = binary_search(public_entries, pubcount, common_definitions[i]->name))
		{
			for (j = 0; j < listnode->count; ++j)
			{
				if (((PPUBLIC)listnode->object[j])->mod == pubdef->mod)
				{
					if (!((PPUBLIC)listnode->object[j])->alias)
					{
						printf("Duplicate public symbol %s\n", common_definitions[i]->name);
						exit(1);
					}
					free(((PPUBLIC)listnode->object[j])->alias);
					(*((PPUBLIC)listnode->object[j])) = (*pubdef);
					pubdef = NULL;
					break;
				}
			}
		}
		if (pubdef)
		{
			sort_insert(&public_entries, &pubcount, common_definitions[i]->name, pubdef);
		}
	}
	segment_list[comfarseg]->data_mask =
		(PUCHAR)check_malloc((segment_list[comfarseg]->length + 7) >>3);
	for (j = 0; j < (segment_list[comfarseg]->length + 7) >>3; j++)
		segment_list[comfarseg]->data_mask[j] = 0;


	segment_list[comseg]->data_mask =
		(PUCHAR)check_malloc((segment_list[comseg]->length + 7) >> 3);
	for (j = 0; j < (segment_list[comseg]->length + 7) >> 3; j++)
		segment_list[comseg]->data_mask[j] = 0;


	for (i = 0; i < expcount; i++)
	{
		if (export_definitions[i].pubdef) continue;
		if (listnode = binary_search(public_entries, pubcount, export_definitions[i].int_name))
		{
			for (j = 0; j < listnode->count; j++)
			{
				/* global publics only can match exports */
				if (((PPUBLIC)listnode->object[j])->mod == 0)
				{
					export_definitions[i].pubdef = (PPUBLIC)listnode->object[j];
					break;
				}
			}
		}
	}
	for (i = 0; i < extcount; i++)
	{
		if (extern_records[i].flags != EXT_NOMATCH) continue;
		if (listnode = binary_search(public_entries, pubcount, extern_records[i].name))
		{
			for (j = 0; j < listnode->count; j++)
			{
				/* global publics only can match exports */
				if (((PPUBLIC)(listnode->object[j]))->mod == extern_records[i].mod)
				{
					extern_records[i].pubdef = (PPUBLIC)(listnode->object[j]);
					extern_records[i].flags = EXT_MATCHEDPUBLIC;
					break;
				}
			}
		}
	}
}

static void sort_segments()
{
	long i, j, k;
	UINT base, align;
	long baseSeg;

	for (i = 0; i < segcount; i++)
	{
		if (segment_list[i])
		{
			if ((segment_list[i]->attributes & SEG_ALIGN) != SEG_ABS)
			{
				segment_list[i]->absolute_frame = 0;
			}
		}
	}

	outcount = 0;
	base = 0;
	out_list = check_malloc(sizeof(PSEG) * segcount);
	for (i = 0; i < grpcount; i++)
	{
		if (group_list[i])
		{
			group_list[i]->segment_number = -1;
			for (j = 0; j < group_list[i]->numsegs; j++)
			{
				k = group_list[i]->segindex[j];
				if (!segment_list[k])
				{
					printf("Error - group %s contains non-existent segment\n", name_list[group_list[i]->name_index]);
					exit(1);
				}
				/* don't add removed sections */
				if (segment_list[k]->win_flags & WINF_REMOVE)
				{
					continue;
				}
				/* add non-absolute segment */
				if ((segment_list[k]->attributes & SEG_ALIGN) != SEG_ABS)
				{
					switch (segment_list[k]->attributes & SEG_ALIGN)
					{
					case SEG_WORD:
						align = 2;
						break;
					case SEG_DWORD:
						align = 4;
						break;
					case SEG_8BYTE:
						align = 0x8;
						break;
					case SEG_PARA:
						align = 0x10;
						break;
					case SEG_32BYTE:
						align = 0x20;
						break;
					case SEG_64BYTE:
						align = 0x40;
						break;
					case SEG_PAGE:
						align = 0x100;
						break;
					case SEG_MEMPAGE:
						align = 0x1000;
						break;
					case SEG_BYTE:
					default:
						align = 1;
						break;
					}
					if (align < object_align)
					{
						align = object_align;
					}
					base = (base + align - 1) & (0xffffffff - (align - 1));
					segment_list[k]->base = base;
					if (segment_list[k]->length > 0)
					{
						base += segment_list[k]->length;
						if (segment_list[k]->absolute_frame != 0)
						{
							printf("Error - Segment %s part of more than one group\n", name_list[segment_list[k]->name_index]);
							exit(1);
						}
						segment_list[k]->absolute_frame = 1;
						segment_list[k]->absolute_offset = i + 1;
						if (group_list[i]->segment_number < 0)
						{
							group_list[i]->segment_number = k;
						}
						if (outcount == 0)
						{
							baseSeg = k;
						}
						else
						{
							out_list[outcount - 1]->virtual_size = segment_list[k]->base -
								out_list[outcount - 1]->base;
						}
						out_list[outcount] = segment_list[k];
						outcount++;
					}
				}
			}
		}
	}
	for (i = 0; i < segcount; i++)
	{
		if (segment_list[i])
		{
			/* don't add removed sections */
			if (segment_list[i]->win_flags & WINF_REMOVE)
			{
				continue;
			}
			/* add non-absolute segment, not already dealt with */
			if (((segment_list[i]->attributes & SEG_ALIGN) != SEG_ABS) &&
				!segment_list[i]->absolute_frame)
			{
				switch (segment_list[i]->attributes & SEG_ALIGN)
				{
				case SEG_WORD:
				case SEG_BYTE:
				case SEG_DWORD:
				case SEG_PARA:
					align = 0x10;
					break;
				case SEG_PAGE:
					align = 0x100;
					break;
				case SEG_MEMPAGE:
					align = 0x1000;
					break;
				default:
					align = 1;
					break;
				}
				if (align < object_align)
				{
					align = object_align;
				}
				base = (base + align - 1) & (0xffffffff - (align - 1));
				segment_list[i]->base = base;
				if (segment_list[i]->length > 0)
				{
					base += segment_list[i]->length;
					segment_list[i]->absolute_frame = 1;
					segment_list[i]->absolute_offset = 0;
					if (outcount == 0)
					{
						baseSeg = i;
					}
					else
					{
						out_list[outcount - 1]->virtual_size = segment_list[i]->base -
							out_list[outcount - 1]->base;
					}
					out_list[outcount] = segment_list[i];
					outcount++;
				}
			}
			else if ((segment_list[i]->attributes & SEG_ALIGN) == SEG_ABS)
			{
				segment_list[i]->base = (segment_list[i]->absolute_frame << 4) + segment_list[i]->absolute_offset;
			}
		}
	}
	/* build size of last segment in output list */
	if (outcount)
	{
		out_list[outcount - 1]->virtual_size =
			(out_list[outcount - 1]->length + object_align - 1) &
			(0xffffffff - (object_align - 1));
	}
	for (i = 0; i < grpcount; i++)
	{
		if (group_list[i] && (group_list[i]->segment_number < 0)) group_list[i]->segment_number = baseSeg;
	}
}

static void load_files()
{
	long i, j, k;
	char* name;

	for (i = 0; i < filecount; i++)
	{
		a_file = fopen(owner_file_name[i], "rb");
		if (!strchr(owner_file_name[i], PATH_CHAR))
		{
			/* if no path specified, search library path list */
			for (j = 0; !a_file && j < lib_path_count; j++)
			{
				name = (char*)check_malloc(strlen(lib_path[j]) + strlen(owner_file_name[i]) + 1);
				strcpy(name, lib_path[j]);
				strcat(name, owner_file_name[i]);
				a_file = fopen(name, "rb");
				if (a_file)
				{
					free(owner_file_name[i]);
					owner_file_name[i] = name;
					name = NULL;
				}
				else
				{
					free(name);
					name = NULL;
				}
			}
		}
		if (!a_file)
		{
			printf("Error opening file %s\n", owner_file_name[i]);
			exit(1);
		}
		for (k = 0; k < i; ++k)
		{
			if (!strcmp(owner_file_name[i], owner_file_name[k])) break;
		}
		if (k != i)
		{
			fclose(a_file);
			continue;
		}

		file_position = 0;
		printf("Loading file %s\n", owner_file_name[i]);
		j = fgetc(a_file);
		fseek(a_file, 0, SEEK_SET);
		switch (j)
		{
		case LIBHDR:
			load_lib(owner_file_name[i], a_file);
			break;
		case THEADR:
		case LHEADR:
			load_mod(owner_file_name[i], a_file);
			break;
		case 0:
			load_resource(owner_file_name[i], a_file);
			break;
		case 0x4c:
		case 0x4d:
		case 0x4e:
			load_coff(owner_file_name[i], a_file);
			break;
		case 0x21:
			load_coff_lib(owner_file_name[i], a_file);
			break;
		default:
			printf("Unknown file type\n");
			fclose(a_file);
			exit(1);
		}
		fclose(a_file);
	}
}

static void generate_map()
{
	long i, j;
	PPUBLIC q;

	a_file = fopen(map_name, "wt");
	if (!a_file)
	{
		printf("Error opening map file %s\n", map_name);
		exit(1);
	}
	printf("Generating map file %s\n", map_name);

	for (i = 0; i < segcount; i++)
	{
		if (segment_list[i])
		{
			fprintf(a_file, "SEGMENT %s ",
				(segment_list[i]->name_index >= 0) ? name_list[segment_list[i]->name_index] : "");
			switch (segment_list[i]->attributes & SEG_COMBINE)
			{
			case SEG_PRIVATE:
				fprintf(a_file, "PRIVATE ");
				break;
			case SEG_PUBLIC:
				fprintf(a_file, "PUBLIC ");
				break;
			case SEG_PUBLIC2:
				fprintf(a_file, "PUBLIC(2) ");
				break;
			case SEG_STACK:
				fprintf(a_file, "STACK ");
				break;
			case SEG_COMMON:
				fprintf(a_file, "COMMON ");
				break;
			case SEG_PUBLIC3:
				fprintf(a_file, "PUBLIC(3) ");
				break;
			default:
				fprintf(a_file, "unknown ");
				break;
			}
			if (segment_list[i]->attributes & SEG_USE32)
			{
				fprintf(a_file, "USE32 ");
			}
			else
			{
				fprintf(a_file, "USE16 ");
			}
			switch (segment_list[i]->attributes & SEG_ALIGN)
			{
			case SEG_ABS:
				fprintf(a_file, "AT 0%04lXh ", segment_list[i]->absolute_frame);
				break;
			case SEG_BYTE:
				fprintf(a_file, "BYTE ");
				break;
			case SEG_WORD:
				fprintf(a_file, "WORD ");
				break;
			case SEG_PARA:
				fprintf(a_file, "PARA ");
				break;
			case SEG_PAGE:
				fprintf(a_file, "PAGE ");
				break;
			case SEG_DWORD:
				fprintf(a_file, "DWORD ");
				break;
			case SEG_MEMPAGE:
				fprintf(a_file, "MEMPAGE ");
				break;
			default:
				fprintf(a_file, "unknown ");
			}
			if (segment_list[i]->class_index >= 0)
				fprintf(a_file, "'%s'\n", name_list[segment_list[i]->class_index]);
			else
				fprintf(a_file, "\n");
			fprintf(a_file, "  at %08lX, length %08lX\n", segment_list[i]->base, segment_list[i]->length);
		}
	}
	for (i = 0; i < grpcount; i++)
	{
		if (!group_list[i]) continue;
		fprintf(a_file, "\nGroup %s:\n", name_list[group_list[i]->name_index]);
		for (j = 0; j < group_list[i]->numsegs; j++)
		{
			fprintf(a_file, "    %s\n", name_list[segment_list[group_list[i]->segindex[j]]->name_index]);
		}
	}

	if (pubcount)
	{
		fprintf(a_file, "\npublics:\n");
	}
	for (i = 0; i < pubcount; ++i)
	{
		for (j = 0; j < public_entries[i].count; ++j)
		{
			q = (PPUBLIC)public_entries[i].object[j];
			if (q->mod) continue;

			if (q->segment >= 0) {
				fprintf(a_file, "%s @ %s:%08lX\n",
					public_entries[i].id,
					name_list[segment_list[q->segment]->name_index],
					q->offset);
			}
			else {
				fprintf(a_file, "%s = %08lX\n",
					public_entries[i].id,
					q->offset);
			}
		}
	}
	if (extcount) {
		fprintf(a_file, "\n %li externs:\n", extcount);
		for (i = 0; i < extcount; i++)
		{
			fprintf(a_file, "%s(%d) type=%ld, import = %08X\n"
				, extern_records[i].name
				, i
				, extern_records[i].type
				, extern_records[i].import
			);
		}

	}
	if (expcount)
	{
		fprintf(a_file, "\n %li exports:\n", expcount);
		for (i = 0; i < expcount; i++)
		{
			fprintf(a_file, "%s(%i)=%s\n", export_definitions[i].exp_name, export_definitions[i].ordinal, export_definitions[i].int_name);
		}
	}
	if (impcount)
	{
		fprintf(a_file, "\n %li imports:\n", impcount);
		for (i = 0; i < impcount; i++)
		{
			fprintf(a_file, "%s=%s:%s(%i)\n", import_definitions[i].int_name, import_definitions[i].mod_name, import_definitions[i].flags == 0 ? import_definitions[i].imp_name : "",
				import_definitions[i].flags == 0 ? 0 : import_definitions[i].ordinal);
		}
	}
	fclose(a_file);
}

int main(int argc, char* argv[])
{
	long i, j;
	int isend;
	char* libList;

	printf("ALINK v1.6 (C) Copyright 1998-9 Anthony A.J. Williams.\n");
	printf("All Rights Reserved\n\n");

	libList = getenv("LIB");
	if (libList)
	{
		for (i = 0, j = 0;; i++)
		{
			isend = (!libList[i]);
			if (libList[i] == ';' || !libList[i])
			{
				if (i - j)
				{
					lib_path = (PCHAR*)check_realloc(lib_path, (lib_path_count + 1) * sizeof(PCHAR));
					libList[i] = 0;
					if (libList[i - 1] == PATH_CHAR)
					{
						lib_path[lib_path_count] = check_strdup(libList + j);
					}
					else
					{
						lib_path[lib_path_count] = (PCHAR)check_malloc(i - j + 2);
						strcpy(lib_path[lib_path_count], libList + j);
						lib_path[lib_path_count][i - j] = PATH_CHAR;
						lib_path[lib_path_count][i - j + 1] = 0;
					}
					lib_path_count++;
				}
				j = i + 1;
			}
			if (isend) break;
		}
	}

	if (!process_command_line(argc, argv)) {
		return 1;
	}

	if (!filecount)
	{
		printf("No files specified\n");
		return 1;
		//exit(1);
	}

	if (!out_name)
	{
		//first obj's name as default name
		out_name = check_malloc(strlen(owner_file_name[0]) + 1 + 4);
		strcpy(out_name, owner_file_name[0]);
		i = (long)strlen(out_name);
		while ((i >= 0) && (out_name[i] != '.') && (out_name[i] != PATH_CHAR) && (out_name[i] != ':'))
		{
			i--;
		}
		if (out_name[i] == '.')
		{
			out_name[i] = 0;
		}
	}
	i = (long)strlen(out_name);
	while ((i >= 0) && (out_name[i] != '.') && (out_name[i] != PATH_CHAR) && (out_name[i] != ':'))
	{
		i--;
	}
	if (out_name[i] != '.')
	{
		switch (output_type)
		{
		case OUTPUT_EXE:
		case OUTPUT_PE:
			if (!build_dll)
			{
				strcat(out_name, ".exe");
			}
			else
			{
				strcat(out_name, ".dll");
			}
			break;
		case OUTPUT_COM:
			strcat(out_name, ".com");
			break;
		default:
			break;
		}
	}

	if (map_file)
	{
		if (!map_name)
		{
			map_name = check_malloc(strlen(out_name) + 1 + 4);
			strcpy(map_name, out_name);
			i = (long)strlen(map_name);
			while ((i >= 0) && (map_name[i] != '.') && (map_name[i] != PATH_CHAR) && (map_name[i] != ':'))
			{
				i--;
			}
			if (map_name[i] != '.')
			{
				i = (long)strlen(map_name);
			}
			strcpy(map_name + i, ".map");
		}
	}
	else
	{
		if (map_name)
		{
			free(map_name);
			map_name = 0;
		}
	}

	load_files();

	if (!nummods)
	{
		printf("No required modules specified\n");
		return 1;
		//NOTICE:
		//exit(1);
	}

	if (rescount && (output_type != OUTPUT_PE))
	{
		printf("Cannot link resources into a non-PE application\n");
		return 1;
		//NOTICE:
		//exit(1);
	}

	if (entry_point_function_name)
	{
		if (!case_sensitive)
		{
			strupr(entry_point_function_name);
		}

		if (got_start_address)
		{
			printf("Warning, overriding entry point from Command Line\n");
		}
		/* define an external reference for entry point */
		extern_records = check_realloc(extern_records, (extcount + 1) * sizeof(EXTREC));
		extern_records[extcount].name = entry_point_function_name;
		extern_records[extcount].type = -1;
		extern_records[extcount].pubdef = NULL;
		extern_records[extcount].flags = EXT_NOMATCH;
		extern_records[extcount].mod = 0;

		/* point start address to this external */
		start_address.ftype = REL_EXTDISP;
		start_address.frame = extcount;
		start_address.ttype = REL_EXTONLY;
		start_address.target = extcount;

		extcount++;
		got_start_address = TRUE;
	}

	match_externs();
	printf("matched Externs\n");

	match_comdefs();
	printf("matched ComDefs\n");

	for (i = 0; i < expcount; i++)
	{
		if (!export_definitions[i].pubdef)
		{
			printf("Unresolved export %s=%s\n", export_definitions[i].exp_name, export_definitions[i].int_name);
			error_count++;
		}
		else if (export_definitions[i].pubdef->alias)
		{
			printf("Unresolved export %s=%s, with alias %s\n", export_definitions[i].exp_name, export_definitions[i].int_name, export_definitions[i].pubdef->alias);
			error_count++;
		}

	}

	for (i = 0; i < extcount; i++)
	{
		if (extern_records[i].flags == EXT_NOMATCH)
		{
			printf("Unresolved external %s\n", extern_records[i].name);
			error_count++;
			unresolved_external_warnings_count++;
		}
		else if (extern_records[i].flags == EXT_MATCHEDPUBLIC)
		{
			if (extern_records[i].pubdef->alias)
			{
				printf("Unresolved external %s with alias %s\n", extern_records[i].name, extern_records[i].pubdef->alias);
				error_count++;
				unresolved_external_warnings_count++;
			}
		}
	}

	if (error_count)
	{
		if (unresolved_external_warnings_count) {
			printf("Warning: %d unresolved externals\n", unresolved_external_warnings_count);
		}
		return 1;
		//NOTICE:
		//exit(1);
	}

	combine_blocks(out_name);
	sort_segments();

	if (map_file)
	{
		generate_map();
	}
	BOOL ret = FALSE;
	switch (output_type)
	{
	case OUTPUT_COM:
		ret = output_com_file(out_name);
		break;
	case OUTPUT_EXE:
		ret = output_exe_file(out_name);
		break;
	case OUTPUT_PE:
		ret = output_win32_file(out_name);
		break;
	default:
		printf("Invalid output type\n");
		ret = FALSE;
		break;
	}
	return !ret;
}
