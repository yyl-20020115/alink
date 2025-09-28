#include "alink.h"

void load_coff(PCHAR fname, FILE* objfile)
{
	UCHAR headbuf[20];
	UCHAR buf[100];
	PUCHAR bigbuf;
	PUCHAR stringList;
	UINT thiscpu;
	UINT numSect;
	UINT headerSize;
	UINT symbolPtr;
	UINT numSymbols;
	UINT stringPtr;
	UINT stringSize;
	UINT stringOfs;
	UINT i, j, k;// , l;
	UINT fileStart;
	UINT minseg;
	UINT numrel;
	UINT relofs;
	UINT relshift;
	UINT sectname;
	long sectorder;
	PCOFFSYM sym;
	UINT combineType;
	PPUBLIC pubdef;
	PCOMDAT comdat;
	PCHAR comdatsym;
	PSORTENTRY listnode;

	nummods++;
	minseg = segcount;
	fileStart = ftell(objfile);

	if (fread(headbuf, 1, 20, objfile) != 20)
	{
		printf("Unable to read from file\n");
		exit(1);
	}
	thiscpu = headbuf[0] + (((UINT)headbuf[1]) << 8);
	if (!thiscpu)
	{
		/* if we've got an import module, start at the beginning */
		fseek(objfile, fileStart, SEEK_SET);
		/* and load it */
		load_coff_import(fname, objfile);
		return;
	}

	if ((thiscpu < 0x14c) || (thiscpu > 0x14e))
	{
		printf("Unsupported CPU type for module\n");
		exit(1);
	}
	numSect = headbuf[PE_NUMOBJECTS - PE_MACHINEID] + (((UINT)headbuf[PE_NUMOBJECTS - PE_MACHINEID + 1]) << 8);

	symbolPtr = headbuf[PE_SYMBOLPTR - PE_MACHINEID] + (headbuf[PE_SYMBOLPTR - PE_MACHINEID + 1] << 8) +
		(headbuf[PE_SYMBOLPTR - PE_MACHINEID + 2] << 16) + (headbuf[PE_SYMBOLPTR - PE_MACHINEID + 3] << 24);

	numSymbols = headbuf[PE_NUMSYMBOLS - PE_MACHINEID] + (headbuf[PE_NUMSYMBOLS - PE_MACHINEID + 1] << 8) +
		(headbuf[PE_NUMSYMBOLS - PE_MACHINEID + 2] << 16) + (headbuf[PE_NUMSYMBOLS - PE_MACHINEID + 3] << 24);

	if (headbuf[PE_HDRSIZE - PE_MACHINEID] | headbuf[PE_HDRSIZE - PE_MACHINEID + 1])
	{
		printf("warning, optional header discarded\n");
		headerSize = headbuf[PE_HDRSIZE - PE_MACHINEID] + (((UINT)headbuf[PE_HDRSIZE - PE_MACHINEID + 1]) << 8);
	}
	else
		headerSize = 0;
	headerSize += PE_BASE_HEADER_SIZE - PE_MACHINEID;

	stringPtr = symbolPtr + numSymbols * PE_SYMBOL_SIZE;
	if (stringPtr)
	{
		fseek(objfile, fileStart + stringPtr, SEEK_SET);
		if (fread(buf, 1, 4, objfile) != 4)
		{
			printf("Invalid COFF object file, unable to read string table size\n");
			exit(1);
		}
		stringSize = buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24);
		if (!stringSize) stringSize = 4;
		if (stringSize < 4)
		{
			printf("Invalid COFF object file, bad string table size %i\n", stringSize);
			exit(1);
		}
		stringPtr += 4;
		stringSize -= 4;
	}
	else
	{
		stringSize = 0;
	}
	if (stringSize)
	{
		stringList = (PUCHAR)check_malloc(stringSize);
		if (fread(stringList, 1, stringSize, objfile) != stringSize)
		{
			printf("Invalid COFF object file, unable to read string table\n");
			exit(1);
		}
		if (stringList[stringSize - 1])
		{
			printf("Invalid COFF object file, last string unterminated\n");
			exit(1);
		}
	}
	else
	{
		stringList = NULL;
	}

	if (symbolPtr && numSymbols)
	{
		fseek(objfile, fileStart + symbolPtr, SEEK_SET);
		sym = (PCOFFSYM)check_malloc(sizeof(COFFSYM) * numSymbols);
		for (i = 0; i < numSymbols; i++)
		{
			if (fread(buf, 1, PE_SYMBOL_SIZE, objfile) != PE_SYMBOL_SIZE)
			{
				printf("Invalid COFF object file, unable to read symbols\n");
				exit(1);
			}
			if (buf[0] | buf[1] | buf[2] | buf[3])
			{
				sym[i].name = (PUCHAR)check_malloc(9);
				strncpy(sym[i].name, buf, 8);
				sym[i].name[8] = 0;
			}
			else
			{
				stringOfs = buf[4] + (buf[5] << 8) + (buf[6] << 16) + (buf[7] << 24);
				if (stringOfs < 4)
				{
					printf("Invalid COFF object file bad symbol location\n");
					exit(1);
				}
				stringOfs -= 4;
				if (stringOfs >= stringSize)
				{
					printf("Invalid COFF object file bad symbol location\n");
					exit(1);
				}
				sym[i].name = check_strdup(stringList + stringOfs);
			}
			if (!case_sensitive)
			{
				strupr(sym[i].name);
			}

			sym[i].value = buf[8] + (buf[9] << 8) + (buf[10] << 16) + (buf[11] << 24);
			sym[i].section = buf[12] + (buf[13] << 8);
			sym[i].type = buf[14] + (buf[15] << 8);
			sym[i].class = buf[16];
			sym[i].ext_num = -1;
			sym[i].num_aux_recs = buf[17];
			sym[i].is_com_dat = FALSE;

			switch (sym[i].class)
			{
			case COFF_SYM_SECTION: /* section symbol */
				if (sym[i].section < -1)
				{
					break;
				}
				/* section symbols declare an extern always, so can use in relocs */
				/* they may also include a PUBDEF */
				extern_records = (PEXTREC)check_realloc(extern_records, (extcount + 1) * sizeof(EXTREC));
				extern_records[extcount].name = sym[i].name;
				extern_records[extcount].pubdef = NULL;
				extern_records[extcount].mod = 0;
				extern_records[extcount].flags = EXT_NOMATCH;
				sym[i].ext_num = extcount;
				extcount++;
				if (sym[i].section != 0) /* if the section is defined here, make public */
				{
					pubdef = (PPUBLIC)check_malloc(sizeof(PUBLIC));
					pubdef->group = -1;
					pubdef->type = 0;
					pubdef->mod = 0;
					pubdef->alias = NULL;
					pubdef->offset = sym[i].value;

					if (sym[i].section == -1)
					{
						pubdef->segment = -1;
					}
					else
					{
						pubdef->segment = minseg + sym[i].section - 1;
					}
					if (listnode = binary_search(public_entries, pubcount, sym[i].name))
					{
						for (j = 0; j < listnode->count; ++j)
						{
							if (((PPUBLIC)listnode->object[j])->mod == pubdef->mod)
							{
								if (!((PPUBLIC)listnode->object[j])->alias)
								{
									printf("Duplicate public symbol %s\n", sym[i].name);
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
						sort_insert(&public_entries, &pubcount, sym[i].name, pubdef);
					}
				}
			case COFF_SYM_STATIC: /* allowed, but ignored for now as we only want to process if required */
			case COFF_SYM_LABEL:
			case COFF_SYM_FILE:
			case COFF_SYM_FUNCTION:
			case COFF_SYM_EXTERNAL:
				break;
			default:
				printf("unsupported symbol class %02X for symbol %s\n", sym[i].class, sym[i].name);
				exit(1);
			}
			if (sym[i].num_aux_recs)
			{
				sym[i].aux_recs = (PUCHAR)check_malloc(sym[i].num_aux_recs * PE_SYMBOL_SIZE);
			}
			else
			{
				sym[i].aux_recs = NULL;
			}

			/* read in the auxillary records for this symbol */
			for (j = 0; j < sym[i].num_aux_recs; j++)
			{
				if (fread(sym[i].aux_recs + j * PE_SYMBOL_SIZE,
					1, PE_SYMBOL_SIZE, objfile) != PE_SYMBOL_SIZE)
				{
					printf("Invalid COFF object file\n");
					exit(1);
				}
				sym[i + j + 1].name = NULL;
				sym[i + j + 1].num_aux_recs = 0;
				sym[i + j + 1].value = 0;
				sym[i + j + 1].section = -1;
				sym[i + j + 1].type = 0;
				sym[i + j + 1].class = 0;
				sym[i + j + 1].ext_num = -1;
			}
			i += j;
		}
	}
	for (i = 0; i < numSect; i++)
	{
		fseek(objfile, fileStart + headerSize + i * PE_OBJECTENTRY_SIZE,
			SEEK_SET);
		if (fread(buf, 1, PE_OBJECTENTRY_SIZE, objfile) != PE_OBJECTENTRY_SIZE)
		{
			printf("Invalid COFF object file, unable to read section header\n");
			exit(1);
		}
		/* virtual size is also the offset of the data into the segment */
	/*
	  if(buf[PE_OBJECT_VIRTSIZE]|buf[PE_OBJECT_VIRTSIZE+1]|buf[PE_OBJECT_VIRTSIZE+2]
	  |buf[PE_OBJECT_VIRTSIZE+3])
	  {
	  printf("Invalid COFF object file, section has non-zero virtual size\n");
	  exit(1);
	  }
	*/
		buf[8] = 0; /* null terminate name */
		/* get shift value for relocs */
		relshift = buf[PE_OBJECT_VIRTADDR] + (buf[PE_OBJECT_VIRTADDR + 1] << 8) +
			(buf[PE_OBJECT_VIRTADDR + 2] << 16) + (buf[PE_OBJECT_VIRTADDR + 3] << 24);

		if (buf[0] == '/')
		{
			sectname = strtoul(buf + 1, (char**)&bigbuf, 10);
			if (*bigbuf)
			{
				printf("Invalid COFF object file, invalid number %s\n", buf + 1);
				exit(1);
			}
			if (sectname < 4)
			{
				printf("Invalid COFF object file\n");
				exit(1);
			}
			sectname -= 4;
			if (sectname >= stringSize)
			{
				printf("Invalid COFF object file\n");
				exit(1);
			}
			name_list = (PPCHAR)check_realloc(name_list, (name_count + 1) * sizeof(PCHAR));
			name_list[name_count] = check_strdup(stringList + sectname);
			sectname = name_count;
			name_count++;
		}
		else
		{
			name_list = (PPCHAR)check_realloc(name_list, (name_count + 1) * sizeof(PCHAR));
			name_list[name_count] = check_strdup(buf);

			sectname = name_count;
			name_count++;
		}
		if (strchr(name_list[sectname], '$'))
		{
			/* if we have a grouped segment, sort by original name */
			sectorder = sectname;
			/* and get real name, without $ sort section */
			name_list = (PPCHAR)check_realloc(name_list, (name_count + 1) * sizeof(PCHAR));
			name_list[name_count] = check_strdup(name_list[sectname]);
			*(strchr(name_list[name_count], '$')) = 0;
			sectname = name_count;
			name_count++;
		}
		else
		{
			sectorder = -1;
		}

		numrel = buf[PE_OBJECT_NUMREL] + (buf[PE_OBJECT_NUMREL + 1] << 8);
		relofs = buf[PE_OBJECT_RELPTR] + (buf[PE_OBJECT_RELPTR + 1] << 8) +
			(buf[PE_OBJECT_RELPTR + 2] << 16) + (buf[PE_OBJECT_RELPTR + 3] << 24);

		segment_list = (PPSEG)check_realloc(segment_list, (segcount + 1) * sizeof(PSEG));
		segment_list[segcount] = (PSEG)check_malloc(sizeof(SEG));

		segment_list[segcount]->name_index = sectname;
		segment_list[segcount]->order_index = sectorder;
		segment_list[segcount]->class_index = -1;
		segment_list[segcount]->overlay_index = -1;
		segment_list[segcount]->length = buf[PE_OBJECT_RAWSIZE] + (buf[PE_OBJECT_RAWSIZE + 1] << 8) +
			(buf[PE_OBJECT_RAWSIZE + 2] << 16) + (buf[PE_OBJECT_RAWSIZE + 3] << 24);

		segment_list[segcount]->attributes = SEG_PUBLIC | SEG_USE32;
		segment_list[segcount]->win_flags = buf[PE_OBJECT_FLAGS] + (buf[PE_OBJECT_FLAGS + 1] << 8) +
			(buf[PE_OBJECT_FLAGS + 2] << 16) + (buf[PE_OBJECT_FLAGS + 3] << 24);
		segment_list[segcount]->base = buf[PE_OBJECT_RAWPTR] + (buf[PE_OBJECT_RAWPTR + 1] << 8) +
			(buf[PE_OBJECT_RAWPTR + 2] << 16) + (buf[PE_OBJECT_RAWPTR + 3] << 24);

		if (segment_list[segcount]->win_flags & WINF_ALIGN_NOPAD)
		{
			segment_list[segcount]->win_flags &= (0xffffffff - WINF_ALIGN);
			segment_list[segcount]->win_flags |= WINF_ALIGN_BYTE;
		}

		switch (segment_list[segcount]->win_flags & WINF_ALIGN)
		{
		case WINF_ALIGN_BYTE:
			segment_list[segcount]->attributes |= SEG_BYTE;
			break;
		case WINF_ALIGN_WORD:
			segment_list[segcount]->attributes |= SEG_WORD;
			break;
		case WINF_ALIGN_DWORD:
			segment_list[segcount]->attributes |= SEG_DWORD;
			break;
		case WINF_ALIGN_8:
			segment_list[segcount]->attributes |= SEG_8BYTE;
			break;
		case WINF_ALIGN_PARA:
			segment_list[segcount]->attributes |= SEG_PARA;
			break;
		case WINF_ALIGN_32:
			segment_list[segcount]->attributes |= SEG_32BYTE;
			break;
		case WINF_ALIGN_64:
			segment_list[segcount]->attributes |= SEG_64BYTE;
			break;
		case 0:
			segment_list[segcount]->attributes |= SEG_PARA; /* default */
			break;
		default:
			printf("Invalid COFF object file, bad section alignment %08X\n", segment_list[segcount]->win_flags);
			exit(1);
		}

		/* invert all negative-logic flags */
		segment_list[segcount]->win_flags ^= WINF_NEG_FLAGS;
		/* remove .debug sections */
		if (!strcasecmp(name_list[sectname], ".debug"))
		{
			segment_list[segcount]->win_flags |= WINF_REMOVE;
			segment_list[segcount]->length = 0;
			numrel = 0;
		}

		if (segment_list[segcount]->win_flags & WINF_COMDAT)
		{
			printf("COMDAT section %s\n", name_list[sectname]);
			comdat = (PCOMDAT)check_malloc(sizeof(COMDATREC));
			combineType = 0;
			comdat->link_with = 0;
			for (j = 0; j < numSymbols; j++)
			{
				if (!sym[j].name) continue;
				if (sym[j].section == (i + 1))
				{
					if (sym[j].num_aux_recs != 1)
					{
						printf("Invalid COMDAT section reference\n");
						exit(1);
					}
					printf("Section %s ", sym[j].name);
					combineType = sym[j].aux_recs[14];
					comdat->link_with = sym[j].aux_recs[12] + (sym[j].aux_recs[13] << 8) + minseg - 1;
					printf("Combine type %i ", sym[j].aux_recs[14]);
					printf("Link alongside section %i", comdat->link_with);

					break;
				}
			}
			if (j == numSymbols)
			{
				printf("Invalid COMDAT section\n");
				exit(1);
			}
			for (j++; j < numSymbols; j++)
			{
				if (!sym[j].name) continue;
				if (sym[j].section == (i + 1))
				{
					if (sym[j].num_aux_recs)
					{
						printf("Invalid COMDAT symbol\n");
						exit(1);
					}

					printf("COMDAT Symbol %s\n", sym[j].name);
					comdatsym = sym[j].name;
					sym[j].is_com_dat = TRUE;
					break;
				}
			}
			/* associative sections don't have a name */
			if (j == numSymbols)
			{
				if (combineType != 5)
				{
					printf("\nInvalid COMDAT section\n");
					exit(1);
				}
				else
				{
					printf("\n");
				}
				comdatsym = ""; /* dummy name */
			}
			comdat->seg_num = segcount;
			comdat->combine_type = combineType;

			printf("COMDATs not yet supported\n");
			exit(1);

			printf("Combine types for duplicate COMDAT symbol %s do not match\n", comdatsym);
			exit(1);
		}

		if (segment_list[segcount]->length)
		{
			segment_list[segcount]->data = (PUCHAR)check_malloc(segment_list[segcount]->length);

			segment_list[segcount]->data_mask = (PUCHAR)check_malloc((segment_list[segcount]->length + 7) / 8);

			if (segment_list[segcount]->base)
			{
				fseek(objfile, fileStart + segment_list[segcount]->base, SEEK_SET);
				if (fread(segment_list[segcount]->data, 1, segment_list[segcount]->length, objfile)
					!= segment_list[segcount]->length)
				{
					printf("Invalid COFF object file\n");
					exit(1);
				}
				for (j = 0; j < (segment_list[segcount]->length + 7) / 8; j++)
					segment_list[segcount]->data_mask[j] = 0xff;
			}
			else
			{
				for (j = 0; j < (segment_list[segcount]->length + 7) / 8; j++)
					segment_list[segcount]->data_mask[j] = 0;
			}

		}
		else
		{
			segment_list[segcount]->data = NULL;
			segment_list[segcount]->data_mask = NULL;
		}

		if (numrel) fseek(objfile, fileStart + relofs, SEEK_SET);
		for (j = 0; j < numrel; j++)
		{
			if (fread(buf, 1, PE_RELOC_SIZE, objfile) != PE_RELOC_SIZE)
			{
				printf("Invalid COFF object file, unable to read reloc table\n");
				exit(1);
			}
			relocations = (PPRELOC)check_realloc(relocations, (fixcount + 1) * sizeof(PRELOC));
			relocations[fixcount] = (PRELOC)check_malloc(sizeof(RELOC));
			/* get address to relocate */
			relocations[fixcount]->offset = buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24);
			relocations[fixcount]->offset -= relshift;
			relocations[fixcount]->offset_in_file = relocations[fixcount]->offset;
			/* get segment */
			relocations[fixcount]->segment = i + minseg;
			relocations[fixcount]->disp = 0;
			/* get relocation target external index */
			relocations[fixcount]->target = buf[4] + (buf[5] << 8) + (buf[6] << 16) + (buf[7] << 24);
			if (relocations[fixcount]->target >= numSymbols)
			{
				printf("Invalid COFF object file, undefined symbol\n");
				exit(1);
			}
			k = relocations[fixcount]->target;
			relocations[fixcount]->ttype = REL_EXTONLY; /* assume external reloc */
			if (sym[k].ext_num < 0)
			{
				switch (sym[k].class)
				{
				case COFF_SYM_EXTERNAL:
					/* global symbols declare an extern when used in relocs */
					extern_records = (PEXTREC)check_realloc(extern_records, (extcount + 1) * sizeof(EXTREC));
					extern_records[extcount].name = sym[k].name;
					extern_records[extcount].pubdef = NULL;
					extern_records[extcount].mod = 0;
					extern_records[extcount].flags = EXT_NOMATCH;
					sym[k].ext_num = extcount;
					extcount++;
					/* they may also include a COMDEF or a PUBDEF */
					/* this is dealt with after all sections loaded, to cater for COMDAT symbols */
					break;
				case COFF_SYM_STATIC: /* static symbol */
				case COFF_SYM_LABEL: /* code label symbol */
					if (sym[k].section < -1)
					{
						printf("cannot relocate against a debug info symbol\n");
						exit(1);
						break;
					}
					if (sym[k].section == 0)
					{
						if (sym[k].value)
						{
							extern_records = (PEXTREC)check_realloc(extern_records, (extcount + 1) * sizeof(EXTREC));
							extern_records[extcount].name = sym[k].name;
							extern_records[extcount].pubdef = NULL;
							extern_records[extcount].mod = nummods;
							extern_records[extcount].flags = EXT_NOMATCH;
							sym[k].ext_num = extcount;
							extcount++;

							common_definitions = (PPCOMREC)check_realloc(common_definitions, (comcount + 1) * sizeof(PCOMREC));
							common_definitions[comcount] = (PCOMREC)check_malloc(sizeof(COMREC));
							common_definitions[comcount]->length = sym[k].value;
							common_definitions[comcount]->is_far = FALSE;
							common_definitions[comcount]->name = sym[k].name;
							common_definitions[comcount]->modnum = nummods;
							comcount++;
						}
						else
						{
							printf("Undefined symbol %s\n", sym[k].name);
							exit(1);
						}
					}
					else
					{
						/* update relocation information to reflect symbol */
						relocations[fixcount]->ttype = REL_SEGDISP;
						relocations[fixcount]->disp = sym[k].value;
						if (sym[k].section == -1)
						{
							/* absolute symbols have section=-1 */
							relocations[fixcount]->target = -1;
						}
						else
						{
							/* else get real number of section */
							relocations[fixcount]->target = sym[k].section + minseg - 1;
						}
					}
					break;
				default:
					printf("undefined symbol class 0x%02X for symbol %s\n", sym[k].class, sym[k].name);
					exit(1);
				}
			}
			if (relocations[fixcount]->ttype == REL_EXTONLY)
			{
				/* set relocation target to external if sym is external */
				relocations[fixcount]->target = sym[k].ext_num;
			}

			/* frame is current segment (only relevant for non-FLAT output) */
			relocations[fixcount]->ftype = REL_SEGFRAME;
			relocations[fixcount]->frame = i + minseg;
			/* set relocation type */
			switch (buf[8] + (buf[9] << 8))
			{
			case COFF_FIX_DIR32:
				relocations[fixcount]->rtype = FIX_OFS32;
				break;
			case COFF_FIX_RVA32:
				relocations[fixcount]->rtype = FIX_RVA32;
				break;
				/*
				  case 0x0a: -
				  break;
				  case 0x0b:
				  break;
				*/
			case COFF_FIX_REL32:
				relocations[fixcount]->rtype = FIX_SELF_OFS32;
				break;
			default:
				printf("unsupported COFF relocation type %04X\n", buf[8] + (buf[9] << 8));
				exit(1);
			}
			fixcount++;
		}

		segcount++;
	}
	/* build PUBDEFs or COMDEFs for external symbols defined here that aren't COMDAT symbols. */
	for (i = 0; i < numSymbols; i++)
	{
		if (sym[i].class != COFF_SYM_EXTERNAL) continue;
		if (sym[i].is_com_dat) continue;
		if (sym[i].section < -1)
		{
			break;
		}
		if (sym[i].section == 0)
		{
			if (sym[i].value)
			{
				common_definitions = (PPCOMREC)check_realloc(common_definitions, (comcount + 1) * sizeof(PCOMREC));
				common_definitions[comcount] = (PCOMREC)check_malloc(sizeof(COMREC));
				common_definitions[comcount]->length = sym[i].value;
				common_definitions[comcount]->is_far = FALSE;
				common_definitions[comcount]->name = sym[i].name;
				common_definitions[comcount]->modnum = 0;
				comcount++;
			}
		}
		else
		{
			pubdef = (PPUBLIC)check_malloc(sizeof(PUBLIC));
			pubdef->group = -1;
			pubdef->type = 0;
			pubdef->mod = 0;
			pubdef->alias = NULL;
			pubdef->offset = sym[i].value;

			if (sym[i].section == -1)
			{
				pubdef->segment = -1;
			}
			else
			{
				pubdef->segment = minseg + sym[i].section - 1;
			}
			if (listnode = binary_search(public_entries, pubcount, sym[i].name))
			{
				for (j = 0; j < listnode->count; ++j)
				{
					if (((PPUBLIC)listnode->object[j])->mod == pubdef->mod)
					{
						if (!((PPUBLIC)listnode->object[j])->alias)
						{
							printf("Duplicate public symbol %s\n", sym[i].name);
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
				sort_insert(&public_entries, &pubcount, sym[i].name, pubdef);
			}
		}
	}

	if (symbolPtr && numSymbols) free(sym);
	if (stringList) free(stringList);
}

void load_coff_import(PCHAR name, FILE* objfile)
{
	UINT fileStart;
	UINT thiscpu;

	fileStart = ftell(objfile);

	if (fread(buffer, 1, 20, objfile) != 20)
	{
		printf("Unable to read from file\n");
		exit(1);
	}

	if (buffer[0] || buffer[1] || (buffer[2] != 0xff) || (buffer[3] |= 0xff))
	{
		printf("Invalid Import entry\n");
		exit(1);
	}
	/* get CPU type */
	thiscpu = buffer[6] + (((UINT)buffer[7]) << 8);
	printf("Import CPU=%04X\n", thiscpu);

	if ((thiscpu < 0x14c) || (thiscpu > 0x14e))
	{
		printf("Unsupported CPU type for module\n");
		exit(1);
	}

}
