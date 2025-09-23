#include "alink.h"

char t_thred[4];
char f_thred[4];
int t_thredindex[4];
int f_thredindex[4];

void destroy_lidata(PDATABLOCK p)
{
	long i;
	if (p->blocks)
	{
		for (i = 0; i < p->blocks; i++)
		{
			destroy_lidata(((PPDATABLOCK)(p->data))[i]);
		}
	}
	free(p->data);
	free(p);
}

PDATABLOCK build_lidata(long* bufofs)
{
	PDATABLOCK p;
	long i, j;

	p = check_malloc(sizeof(DATABLOCK));
	i = *bufofs;
	p->data_offset = i - lidata->data_offset;
	p->count = buffer[i] + (((UINT)buffer[i + 1]) << 8);
	i += 2;
	if (record_type == LIDATA32)
	{
		p->count += (buffer[i] + (((UINT)buffer[i + 1]) << 8)) << 16;
		i += 2;
	}
	p->blocks = buffer[i] + (((UINT)buffer[i + 1]) << 8);
	i += 2;
	if (p->blocks)
	{
		p->data = check_malloc(p->blocks * sizeof(PDATABLOCK));
		for (j = 0; j < p->blocks; j++)
		{
			((PPDATABLOCK)p->data)[j] = build_lidata(&i);
		}
	}
	else
	{
		p->data = check_malloc(buffer[i] + 1);
		((char*)p->data)[0] = buffer[i];
		i++;
		for (j = 0; j < ((PUCHAR)p->data)[0]; j++, i++)
		{
			((PUCHAR)p->data)[j + 1] = buffer[i];
		}
	}
	*bufofs = i;
	return p;
}

void emit_lidata(PCHAR fname, PDATABLOCK p, long segnum, long* ofs)
{
	long i, j;

	for (i = 0; i < p->count; i++)
	{
		if (p->blocks)
		{
			for (j = 0; j < p->blocks; j++)
			{
				emit_lidata(fname, ((PPDATABLOCK)p->data)[j], segnum, ofs);
			}
		}
		else
		{
			for (j = 0; j < ((PUCHAR)p->data)[0]; j++, (*ofs)++)
			{
				if ((*ofs) >= segment_list[segnum]->length)
				{
					report_error(fname, ERR_INV_DATA);
				}
				if (get_n_bit(segment_list[segnum]->data_mask, *ofs))
				{
					if (segment_list[segnum]->data[*ofs] != ((PUCHAR)p->data)[j + 1])
					{
						report_error(fname, ERR_OVERWRITE);
					}
				}
				segment_list[segnum]->data[*ofs] = ((PUCHAR)p->data)[j + 1];
				set_n_bit(segment_list[segnum]->data_mask, *ofs);
			}
		}
	}
}

void reloc_lidata(PCHAR fname, PDATABLOCK p, long* offset, PRELOC r)
{
	long i, j;

	for (i = 0; i < p->count; i++)
	{
		if (p->blocks)
		{
			for (j = 0; j < p->blocks; j++)
			{
				reloc_lidata(fname, ((PPDATABLOCK)p->data)[j], offset, r);
			}
		}
		else
		{
			j = r->offset - p->data_offset;
			if (j >= 0)
			{
				if ((j < 5) || ((li_le == PREV_LI32) && (j < 7)))
				{
					report_error(fname, ERR_BAD_FIXUP);
				}
				relocations = (PPRELOC)check_realloc(relocations, (fixcount + 1) * sizeof(PRELOC));
				relocations[fixcount] = check_malloc(sizeof(RELOC));
				memcpy(relocations[fixcount], r, sizeof(RELOC));
				relocations[fixcount]->offset = *offset + j;
				fixcount++;
				*offset += ((PUCHAR)p->data)[0];
			}
		}
	}
}

void load_fixup(PCHAR fname, PRELOC relocation, PUCHAR buffer, long* p)
{
	long j;
	int thrednum;

	j = *p;

	relocation->ftype = buffer[j] >> 4;
	relocation->ttype = buffer[j] & 0xf;
	relocation->disp = 0;
	j++;
	if (relocation->ftype & FIX_THRED)
	{
		thrednum = relocation->ftype & THRED_MASK;
		if (thrednum > 3)
		{
			report_error(fname, ERR_BAD_FIXUP);
		}
		relocation->ftype = (f_thred[thrednum] >> 2) & 7;
		switch (relocation->ftype)
		{
		case REL_SEGFRAME:
		case REL_GRPFRAME:
		case REL_EXTFRAME:
		{
			relocation->frame = f_thredindex[thrednum];
			if (!relocation->frame)
			{
				report_error(fname, ERR_BAD_FIXUP);
			}
			break;
		}
		case REL_LILEFRAME:
		case REL_TARGETFRAME:
			break;
		default:
		{
			report_error(fname, ERR_BAD_FIXUP);
		}
		}
		switch (relocation->ftype)
		{
		case REL_SEGFRAME:
		{
			relocation->frame += segmin - 1;
			break;
		}
		case REL_GRPFRAME:
		{
			relocation->frame += grpmin - 1;
			break;
		}
		case REL_EXTFRAME:
		{
			relocation->frame += extmin - 1;
			break;
		}
		case REL_LILEFRAME:
		{
			relocation->frame = previous_segment;
			break;
		}
		default:
			break;
		}
	}
	else
	{
		switch (relocation->ftype)
		{
		case REL_SEGFRAME:
		case REL_GRPFRAME:
		case REL_EXTFRAME:
		{
			relocation->frame = get_index(buffer, &j);
			if (!relocation->frame)
			{
				report_error(fname, ERR_BAD_FIXUP);
			}
			break;
		}
		case REL_LILEFRAME:
		case REL_TARGETFRAME:
			break;
		default:
		{
			report_error(fname, ERR_BAD_FIXUP);
		}
		}
		switch (relocation->ftype)
		{
		case REL_SEGFRAME:
		{
			relocation->frame += segmin - 1;
			break;
		}
		case REL_GRPFRAME:
		{
			relocation->frame += grpmin - 1;
			break;
		}
		case REL_EXTFRAME:
		{
			relocation->frame += extmin - 1;
			break;
		}
		case REL_LILEFRAME:
		{
			relocation->frame = previous_segment;
			break;
		}
		default:
			break;
		}
	}
	if (relocation->ttype & FIX_THRED)
	{
		thrednum = relocation->ttype & 3;
		if ((relocation->ttype & 4) == 0) /* P bit not set? */
		{
			relocation->ttype = (t_thred[thrednum] >> 2) & 3; /* DISP present */
		}
		else
		{
			relocation->ttype = ((t_thred[thrednum] >> 2) & 3) | 4; /* no disp */
		}
		relocation->target = t_thredindex[thrednum];
		switch (relocation->ttype)
		{
		case REL_SEGDISP:
		case REL_GRPDISP:
		case REL_EXTDISP:
		case REL_SEGONLY:
		case REL_GRPONLY:
		case REL_EXTONLY:
		{
			if (!relocation->target)
			{
				report_error(fname, ERR_BAD_FIXUP);
			}
			break;
		}
		case REL_EXPFRAME:
			break;
		default:
		{
			report_error(fname, ERR_BAD_FIXUP);
		}
		}
		switch (relocation->ttype)
		{
		case REL_SEGDISP:
			relocation->target += segmin - 1;
			break;
		case REL_GRPDISP:
			relocation->target += grpmin - 1;
			break;
		case REL_EXTDISP:
			relocation->target += extmin - 1;
			break;
		case REL_EXPFRAME:
			break;
		case REL_SEGONLY:
			relocation->target += segmin - 1;
			break;
		case REL_GRPONLY:
			relocation->target += grpmin - 1;
			break;
		case REL_EXTONLY:
			relocation->target += extmin - 1;
			break;
		}
	}
	else
	{
		relocation->target = get_index(buffer, &j);
		switch (relocation->ttype)
		{
		case REL_SEGDISP:
		case REL_GRPDISP:
		case REL_EXTDISP:
		case REL_SEGONLY:
		case REL_GRPONLY:
		case REL_EXTONLY:
		{
			if (!relocation->target)
			{
				report_error(fname, ERR_BAD_FIXUP);
			}
			break;
		}
		case REL_EXPFRAME:
			break;
		default:
			report_error(fname, ERR_BAD_FIXUP);
		}
		switch (relocation->ttype)
		{
		case REL_SEGDISP:
			relocation->target += segmin - 1;
			break;
		case REL_GRPDISP:
			relocation->target += grpmin - 1;
			break;
		case REL_EXTDISP:
			relocation->target += extmin - 1;
			break;
		case REL_EXPFRAME:
			break;
		case REL_SEGONLY:
			relocation->target += segmin - 1;
			break;
		case REL_GRPONLY:
			relocation->target += grpmin - 1;
			break;
		case REL_EXTONLY:
			relocation->target += extmin - 1;
			break;
		}
	}
	switch (relocation->ttype)
	{
	case REL_SEGDISP:
	case REL_GRPDISP:
	case REL_EXTDISP:
	case REL_EXPFRAME:
	{
		relocation->disp = buffer[j] + (((UINT)buffer[j + 1]) << 8);
		j += 2;
		if (record_type == FIXUPP32)
		{
			relocation->disp += (buffer[j] + (((UINT)buffer[j + 1]) << 8)) << 16;
			j += 2;
		}
		break;
	}
	default:
		break;
	}
	if ((relocation->ftype == REL_TARGETFRAME) && ((relocation->ttype & FIX_THRED) == 0))
	{
		switch (relocation->ttype)
		{
		case REL_SEGDISP:
		case REL_GRPDISP:
		case REL_EXTDISP:
		case REL_EXPFRAME:
			relocation->ftype = relocation->ttype;
			relocation->frame = relocation->target;
			break;
		case REL_SEGONLY:
		case REL_GRPONLY:
		case REL_EXTONLY:
			relocation->ftype = relocation->ttype - 4;
			relocation->frame = relocation->target;
			break;
		}
	}

	*p = j;
}

long load_mod(PCHAR fname, FILE* objfile)
{
	long modpos;
	long done;
	long i, j, k;
	long segnum, grpnum;
	PRELOC relocation;
	PPUBLIC pubdef;
	PCHAR name, aliasName;
	PSORTENTRY listnode;

	modpos = 0;
	done = 0;
	li_le = 0;
	lidata = 0;

	while (!done)
	{
		if (fread(buffer, 1, 3, objfile) != 3)
		{
			report_error(fname, ERR_NO_MODEND);
		}
		record_type = buffer[0];
		record_length = buffer[1] + (((UINT)buffer[2]) << 8);
		if (fread(buffer, 1, record_length, a_file) != record_length)
		{
			report_error(fname, ERR_NO_RECDATA);
		}
		record_length--; /* remove checksum */
		if ((!modpos) && (record_type != THEADR) && (record_type != LHEADR))
		{
			report_error(fname, ERR_NO_HEADER);
		}
		switch (record_type)
		{
		case THEADR:
		case LHEADR:
		{
			if (modpos)
			{
				report_error(fname, ERR_EXTRA_HEADER);
			}
			mod_name = check_realloc(mod_name, (nummods + 1) * sizeof(PCHAR));
			mod_name[nummods] = check_malloc(buffer[0] + 1);
			for (i = 0; i < buffer[0]; i++)
			{
				mod_name[nummods][i] = buffer[i + 1];
			}
			mod_name[nummods][i] = 0;
			strupr(mod_name[nummods]);
			//printf("Loading module %s\n",modname[nummods]);
			if ((buffer[0] + 1) != record_length)
			{
				report_error(fname, ERR_EXTRA_DATA);
			}
			name_min = name_count;
			segmin = segcount;
			extmin = extcount;
			fixmin = fixcount;
			grpmin = grpcount;
			impmin = impcount;
			expmin = expcount;
			commin = comcount;
			nummods++;
			break;
		}
		case COMENT:
		{
			li_le = 0;
			if (lidata)
			{
				destroy_lidata(lidata);
				lidata = 0;
			}
			if (record_length >= 2)
			{
				//COMENT class
				switch (buffer[1])
				{
				case COMENT_LIB_SPEC:
				case COMENT_DEFLIB:
				{
					file_name = check_realloc(file_name, (filecount + 1) * sizeof(PCHAR));
					file_name[filecount] = (PCHAR)check_malloc(record_length - 1 + 4);
					/* get filename */
					for (i = 0; i < record_length - 2; i++)
					{
						file_name[filecount][i] = buffer[i + 2];
					}
					file_name[filecount][record_length - 2] = 0;
					for (i = (long)strlen(file_name[filecount]) - 1;
						(i >= 0) && (file_name[filecount][i] != PATH_CHAR);
						i--)
					{
						if (file_name[filecount][i] == '.') break;
					}
					if (((i >= 0) && (file_name[filecount][i] != '.')) || (i < 0))
					{
						strcat(file_name[filecount], ".lib");
					}
					/* add default library to file list */
					filecount++;
					break;
				}
				case COMENT_OMFEXT:
				{
					if (record_length < 4)
					{
						report_error(fname, ERR_INVALID_COMENT);
					}
					switch (buffer[2])
					{
					case EXT_IMPDEF:
					{
						j = 4;
						if (record_length < (j + 4))
						{
							report_error(fname, ERR_INVALID_COMENT);
						}
						import_definitions = check_realloc(import_definitions, (impcount + 1) * sizeof(IMPREC));
						import_definitions[impcount].flags = buffer[3];
						import_definitions[impcount].int_name = check_malloc(buffer[j] + 1);
						for (i = 0; i < buffer[j]; i++)
						{
							import_definitions[impcount].int_name[i] = buffer[j + i + 1];
						}
						j += buffer[j] + 1;
						import_definitions[impcount].int_name[i] = 0;
						if (!case_sensitive)
						{
							strupr(import_definitions[impcount].int_name);
						}
						import_definitions[impcount].mod_name = check_malloc(buffer[j] + 1);
						for (i = 0; i < buffer[j]; i++)
						{
							import_definitions[impcount].mod_name[i] = buffer[j + i + 1];
						}
						j += buffer[j] + 1;
						import_definitions[impcount].mod_name[i] = 0;
						if (!case_sensitive)
						{
							strupr(import_definitions[impcount].mod_name);
						}
						if (import_definitions[impcount].flags)
						{
							import_definitions[impcount].ordinal = buffer[j] + (((UINT)buffer[j + 1]) << 8);
							j += 2;
						}
						else
						{
							if (buffer[j])
							{
								import_definitions[impcount].imp_name = check_malloc(buffer[j] + 1);
								for (i = 0; i < buffer[j]; i++)
								{
									import_definitions[impcount].imp_name[i] = buffer[j + i + 1];
								}
								j += buffer[j] + 1;
								import_definitions[impcount].imp_name[i] = 0;
							}
							else
							{
								import_definitions[impcount].imp_name = check_malloc(strlen(import_definitions[impcount].int_name) + 1);
								strcpy(import_definitions[impcount].imp_name, import_definitions[impcount].int_name);
							}
						}
						impcount++;
						break;

					}
					case EXT_EXPDEF:
					{
						export_definitions = check_realloc(export_definitions, (expcount + 1) * sizeof(EXPREC));
						j = 4;
						export_definitions[expcount].flags = buffer[3];
						export_definitions[expcount].pubdef = NULL;
						export_definitions[expcount].exp_name = check_malloc(buffer[j] + 1);
						for (i = 0; i < buffer[j]; i++)
						{
							export_definitions[expcount].exp_name[i] = buffer[j + i + 1];
						}
						export_definitions[expcount].exp_name[buffer[j]] = 0;
						if (!case_sensitive)
						{
							strupr(export_definitions[expcount].exp_name);
						}
						j += buffer[j] + 1;
						if (buffer[j])
						{
							export_definitions[expcount].int_name = check_malloc(buffer[j] + 1);
							for (i = 0; i < buffer[j]; i++)
							{
								export_definitions[expcount].int_name[i] = buffer[j + i + 1];
							}
							export_definitions[expcount].int_name[buffer[j]] = 0;
							if (!case_sensitive)
							{
								strupr(export_definitions[expcount].int_name);
							}
						}
						else
						{
							export_definitions[expcount].int_name = check_malloc(strlen(export_definitions[expcount].exp_name) + 1);
							strcpy(export_definitions[expcount].int_name, export_definitions[expcount].exp_name);
						}
						j += buffer[j] + 1;
						if (export_definitions[expcount].flags & EXP_ORD)
						{
							export_definitions[expcount].ordinal = buffer[j] + (((UINT)buffer[j + 1]) << 8);
						}
						else
						{
							export_definitions[expcount].ordinal = 0;
						}
						expcount++;
						break;
					}
					default:
					{
						report_error(fname, ERR_INVALID_COMENT);
					}
					}
					break;

				}
				case COMENT_DOSSEG:
					break;
				case COMENT_TRANSLATOR:
				case COMENT_INTEL_COPYRIGHT:
				case COMENT_MSDOS_VER:
				case COMENT_MEMMODEL:
				case COMENT_NEWOMF:
				case COMENT_LINKPASS:
				case COMENT_LIBMOD:
				case COMENT_EXESTR:
				case COMENT_INCERR:
				case COMENT_NOPAD:
				case COMENT_WKEXT:
				case COMENT_LZEXT:
				case COMENT_PHARLAP:
				case COMENT_IBM386:
				case COMENT_RECORDER:
				case COMENT_COMMENT:
				case COMENT_COMPILER:
				case COMENT_DATE:
				case COMENT_TIME:
				case COMENT_USER:
				case COMENT_DEPFILE:
				case COMENT_COMMANDLINE:
				case COMENT_PUBTYPE:
				case COMENT_COMPARAM:
				case COMENT_TYPDEF:
				case COMENT_STRUCTMEM:
				case COMENT_OPENSCOPE:
				case COMENT_LOCAL:
				case COMENT_ENDSCOPE:
				case COMENT_SOURCEFILE:
				case COMENT_DISASM_DIRECTIVE:
				case COMENT_LINKER_DIRECTIVE:
					break;
				default:
					printf("COMENT Record (unknown type %02X)\n", buffer[1]);
					break;
				}
			}
			break;

		}
		case LLNAMES:
		case LNAMES:
		{
			j = 0;
			while (j < record_length)
			{
				name_list = (PPCHAR)check_realloc(name_list, (name_count + 1) * sizeof(PCHAR));
				name_list[name_count] = check_malloc(buffer[j] + 1);
				for (i = 0; i < buffer[j]; i++)
				{
					name_list[name_count][i] = buffer[j + i + 1];
				}
				name_list[name_count][buffer[j]] = 0;
				if (!case_sensitive)
				{
					strupr(name_list[name_count]);
				}
				//printf("name %s\n", namelist[namecount]);
				j += buffer[j] + 1;
				name_count++;
			}
			break;

		}
		case SEGDEF:
		case SEGDEF32:
		{
			segment_list = (PPSEG)check_realloc(segment_list, (segcount + 1) * sizeof(PSEG));
			segment_list[segcount] = check_malloc(sizeof(SEG));
			segment_list[segcount]->attributes = buffer[0];
			j = 1;
			if ((segment_list[segcount]->attributes & SEG_ALIGN) == SEG_ABS)
			{
				segment_list[segcount]->absolute_frame = buffer[j] + (((UINT)buffer[j + 1]) << 8);
				segment_list[segcount]->absolute_offset = buffer[j + 2];
				j += 3;
			}
			segment_list[segcount]->length = buffer[j] + (((UINT)buffer[j + 1]) << 8);
			j += 2;
			if (record_type == SEGDEF32)
			{
				segment_list[segcount]->length += (buffer[j] + (((UINT)buffer[j + 1]) << 8)) << 16;
				j += 2;
			}
			if (segment_list[segcount]->attributes & SEG_BIG)
			{
				if (record_type == SEGDEF)
				{
					segment_list[segcount]->length += 0x10000;
				}
				else
				{
					if ((segment_list[segcount]->attributes & SEG_ALIGN) != SEG_ABS)
					{
						report_error(fname, ERR_SEG_TOO_LARGE);
					}
				}
			}
			segment_list[segcount]->name_index = get_index(buffer, &j) - 1;
			segment_list[segcount]->class_index = get_index(buffer, &j) - 1;
			segment_list[segcount]->overlay_index = get_index(buffer, &j) - 1;
			segment_list[segcount]->order_index = -1;
			if (segment_list[segcount]->name_index >= 0)
			{
				segment_list[segcount]->name_index += name_min;
			}
			if (segment_list[segcount]->class_index >= 0)
			{
				segment_list[segcount]->class_index += name_min;
			}
			if (segment_list[segcount]->overlay_index >= 0)
			{
				segment_list[segcount]->overlay_index += name_min;
			}
			if ((segment_list[segcount]->attributes & SEG_ALIGN) != SEG_ABS)
			{
				segment_list[segcount]->data = check_malloc(segment_list[segcount]->length);
				segment_list[segcount]->data_mask = check_malloc(((segment_list[segcount]->length + 7) >>3));
				for (i = 0; i < ((segment_list[segcount]->length + 7) >> 3); i++)
				{
					segment_list[segcount]->data_mask[i] = 0;
				}
			}
			else
			{
				segment_list[segcount]->data = 0;
				segment_list[segcount]->data_mask = 0;
				segment_list[segcount]->attributes &= (0xffff - SEG_COMBINE);
				segment_list[segcount]->attributes |= SEG_PRIVATE;
			}
			switch (segment_list[segcount]->attributes & SEG_COMBINE)
			{
			case SEG_PRIVATE:
			case SEG_PUBLIC:
			case SEG_PUBLIC2:
			case SEG_COMMON:
			case SEG_PUBLIC3:
				break;
			case SEG_STACK:
				/* stack segs are always byte aligned */
				segment_list[segcount]->attributes &= (0xffff - SEG_ALIGN);
				segment_list[segcount]->attributes |= SEG_BYTE;
				break;
			default:
				report_error(fname, ERR_BAD_SEGDEF);
				break;
			}
			if ((segment_list[segcount]->attributes & SEG_ALIGN) == SEG_BADALIGN)
			{
				report_error(fname, ERR_BAD_SEGDEF);
			}
			if ((segment_list[segcount]->class_index >= 0) &&
				(!strcasecmp(name_list[segment_list[segcount]->class_index], "CODE") ||
					!strcasecmp(name_list[segment_list[segcount]->class_index], "TEXT")))
			{
				/* code segment */
				segment_list[segcount]->win_flags = WINF_CODE | WINF_INITDATA | WINF_EXECUTE | WINF_READABLE | WINF_NEG_FLAGS;
			}
			else    /* data segment */
				segment_list[segcount]->win_flags = WINF_INITDATA | WINF_READABLE | WINF_WRITEABLE | WINF_NEG_FLAGS;

			if (!strcasecmp(name_list[segment_list[segcount]->name_index], "$$SYMBOLS") ||
				!strcasecmp(name_list[segment_list[segcount]->name_index], "$$TYPES"))
			{
				segment_list[segcount]->win_flags |= WINF_REMOVE;
			}
			segcount++;
			break;

		}
		case LEDATA:
		case LEDATA32:
		{
			j = 0;
			previous_segment = get_index(buffer, &j) - 1;
			if (previous_segment < 0)
			{
				report_error(fname, ERR_INV_SEG);
			}
			previous_segment += segmin;
			if ((segment_list[previous_segment]->attributes & SEG_ALIGN) == SEG_ABS)
			{
				report_error(fname, ERR_ABS_SEG);
			}
			previous_offset = buffer[j] + (((UINT)buffer[j + 1]) << 8);
			j += 2;
			if (record_type == LEDATA32)
			{
				previous_offset += (buffer[j] + ((UINT)buffer[j + 1] << 8)) << 16;
				j += 2;
			}
			//NOTICE: reclength includes checksum byte
			for (k = 0; j < record_length - 1; j++, k++)
			{
				if ((previous_offset + k) >= segment_list[previous_segment]->length)
				{
					report_error(fname, ERR_INV_DATA);
				}
				if (get_n_bit(segment_list[previous_segment]->data_mask, previous_offset + k))
				{
					if (segment_list[previous_segment]->data[previous_offset + k] != buffer[j])
					{
						printf("%08lX: %08lX: %i, %li,%li,%li\n",
							previous_offset + k, j,
							get_n_bit(segment_list[previous_segment]->data_mask, previous_offset + k),
							segcount, segmin, previous_segment);
						report_error(fname, ERR_OVERWRITE);
					}
				}
				segment_list[previous_segment]->data[previous_offset + k] = buffer[j];
				set_n_bit(segment_list[previous_segment]->data_mask, previous_offset + k);
			}
			li_le = PREV_LE;
			break;

		}
		case LIDATA:
		case LIDATA32:
		{
			if (lidata)
			{
				destroy_lidata(lidata);
			}
			j = 0;
			previous_segment = get_index(buffer, &j) - 1;
			if (previous_segment < 0)
			{
				report_error(fname, ERR_INV_SEG);
			}
			previous_segment += segmin;
			if ((segment_list[previous_segment]->attributes & SEG_ALIGN) == SEG_ABS)
			{
				report_error(fname, ERR_ABS_SEG);
			}
			previous_offset = buffer[j] + (buffer[j + 1] << 8);
			j += 2;
			if (record_type == LIDATA32)
			{
				previous_offset += (buffer[j] + (buffer[j + 1] << 8)) << 16;
				j += 2;
			}
			lidata = check_malloc(sizeof(DATABLOCK));
			lidata->data = check_malloc(sizeof(PDATABLOCK) * (1024 / sizeof(DATABLOCK) + 1));
			lidata->blocks = 0;
			lidata->data_offset = j;
			for (i = 0; j < record_length; i++)
			{
				((PPDATABLOCK)lidata->data)[i] = build_lidata(&j);
			}
			lidata->blocks = i;
			lidata->count = 1;

			k = previous_offset;
			emit_lidata(fname, lidata, previous_segment, &k);
			li_le = (record_type == LIDATA) ? PREV_LI : PREV_LI32;
			break;

		}
		case LPUBDEF:
		case LPUBDEF32:
		case PUBDEF:
		case PUBDEF32:
		{
			j = 0;
			grpnum = get_index(buffer, &j) - 1;
			if (grpnum >= 0)
			{
				grpnum += grpmin;
			}
			segnum = get_index(buffer, &j) - 1;
			if (segnum < 0)
			{
				j += 2;
			}
			else
			{
				segnum += segmin;
			}
			for (; j < record_length;)
			{
				pubdef = (PPUBLIC)check_malloc(sizeof(PUBLIC));
				pubdef->alias = NULL;
				pubdef->group = grpnum;
				pubdef->segment = segnum;
				name = check_malloc(buffer[j] + 1);
				k = buffer[j];
				j++;
				for (i = 0; i < k; i++)
				{
					name[i] = buffer[j];
					j++;
				}
				name[i] = 0;
				if (!case_sensitive)
				{
					strupr(name);
				}
				pubdef->offset = buffer[j] + (((UINT)buffer[j + 1]) << 8);
				j += 2;
				if ((record_type == PUBDEF32) || (record_type == LPUBDEF32))
				{
					pubdef->offset += (buffer[j] + (((UINT)buffer[j + 1]) << 8)) << 16;
					j += 2;
				}
				pubdef->type = get_index(buffer, &j);
				if (record_type == LPUBDEF || record_type == LPUBDEF32)
				{
					pubdef->mod = nummods;
				}
				else
				{
					pubdef->mod = 0;
				}
				if (listnode = binary_search(public_entries, pubcount, name))
				{
					for (i = 0; i < listnode->count; i++)
					{
						if (((PPUBLIC)listnode->object[i])->mod == pubdef->mod)
						{
							if (!((PPUBLIC)listnode->object[i])->alias)
							{
								printf("Duplicate public symbol %s\n", name);
								exit(1);
							}
							free(((PPUBLIC)listnode->object[i])->alias);
							(*((PPUBLIC)listnode->object[i])) = (*pubdef);
							pubdef = NULL;
							break;
						}
					}
				}
				if (pubdef)
				{
					sort_insert(&public_entries, &pubcount, name, pubdef);
				}
				free(name);
			}
			break;

		}
		case LEXTDEF:
		case LEXTDEF32:
		case EXTDEF:
		{
			for (j = 0; j < record_length;)
			{
				extern_records = (PEXTREC)check_realloc(extern_records, (extcount + 1) * sizeof(EXTREC));
				extern_records[extcount].name = check_malloc(buffer[j] + 1);
				k = buffer[j];
				j++;
				for (i = 0; i < k; i++, j++)
				{
					extern_records[extcount].name[i] = buffer[j];
				}
				extern_records[extcount].name[i] = 0;
				if (!case_sensitive)
				{
					strupr(extern_records[extcount].name);
				}
				extern_records[extcount].type = get_index(buffer, &j);
				extern_records[extcount].pubdef = NULL;
				extern_records[extcount].flags = EXT_NOMATCH;
				if ((record_type == LEXTDEF) || (record_type == LEXTDEF32))
				{
					extern_records[extcount].mod = nummods;
				}
				else
				{
					extern_records[extcount].mod = 0;
				}
				extcount++;
			}
			break;

		}
		case GRPDEF:
		{
			group_list = check_realloc(group_list, (grpcount + 1) * sizeof(PGRP));
			group_list[grpcount] = check_malloc(sizeof(GRP));
			j = 0;
			group_list[grpcount]->name_index = get_index(buffer, &j) - 1 + name_min;
			if (group_list[grpcount]->name_index < name_min)
			{
				report_error(fname, ERR_BAD_GRPDEF);
			}
			group_list[grpcount]->numsegs = 0;
			while (j < record_length)
			{
				if (buffer[j] == 0xff)
				{
					j++;
					i = get_index(buffer, &j) - 1 + segmin;
					if (i < segmin)
					{
						report_error(fname, ERR_BAD_GRPDEF);
					}
					group_list[grpcount]->segindex[group_list[grpcount]->numsegs] = i;
					group_list[grpcount]->numsegs++;
				}
				else
				{
					report_error(fname, ERR_BAD_GRPDEF);
				}
			}
			grpcount++;
			break;
		}
		case FIXUPP:
		case FIXUPP32:
		{
			j = 0;
			while (j < record_length)
			{
				if (buffer[j] & 0x80)
				{
					/* FIXUP subrecord */
					if (!li_le)
					{
						report_error(fname, ERR_BAD_FIXUP);
					}
					relocation = check_malloc(sizeof(RELOC));
					relocation->rtype = (buffer[j] >> 2);
					relocation->offset = (((UINT)buffer[j]) << 8) + buffer[j + 1];
					j += 2;
					relocation->offset &= 0x3ff;
					relocation->rtype ^= FIX_SELFREL;
					relocation->rtype &= FIX_MASK;
					switch (relocation->rtype)
					{
					case FIX_LBYTE:
					case FIX_OFS16:
					case FIX_BASE:
					case FIX_PTR1616:
					case FIX_HBYTE:
					case FIX_OFS16_2:
					case FIX_OFS32:
					case FIX_PTR1632:
					case FIX_OFS32_2:
					case FIX_SELF_LBYTE:
					case FIX_SELF_OFS16:
					case FIX_SELF_OFS16_2:
					case FIX_SELF_OFS32:
					case FIX_SELF_OFS32_2:
						break;
					default:
						report_error(fname, ERR_BAD_FIXUP);
					}
					load_fixup(fname, relocation, buffer, &j);

					if (li_le == PREV_LE)
					{
						relocation->offset += previous_offset;
						relocation->segment = previous_segment;
						relocations = (PPRELOC)check_realloc(relocations, (fixcount + 1) * sizeof(PRELOC));
						relocations[fixcount] = relocation;
						fixcount++;
					}
					else
					{
						relocation->segment = previous_segment;
						i = previous_offset;
						reloc_lidata(fname, lidata, &i, relocation);
						free(relocation);
					}
				}
				else
				{
					/* THRED subrecord */
					i = buffer[j]; /* get thred number */
					j++;
					if (i & 0x40) /* Frame? */
					{
						f_thred[i & 3] = (char)i;
						/* get index if required */
						if ((i & 0x1c) < 0xc)
						{
							f_thredindex[i & 3] = get_index(buffer, &j);
						}
						i &= 3;
					}
					else
					{
						t_thred[i & 3] = (char)i;
						/* target always has index */
						t_thredindex[i & 3] = get_index(buffer, &j);
					}
				}
			}
			break;

		}
		case BAKPAT:
		case BAKPAT32:
		{
			j = 0;
			if (j < record_length) i = get_index(buffer, &j);
			i += segmin - 1;
			if (j < record_length)
			{
				k = buffer[j];
				j++;
			}
			while (j < record_length)
			{
				relocations = (PPRELOC)check_realloc(relocations, (fixcount + 1) * sizeof(PRELOC));
				relocations[fixcount] = check_malloc(sizeof(RELOC));
				switch (k)
				{
				case 0: relocations[fixcount]->rtype = FIX_SELF_LBYTE; break;
				case 1: relocations[fixcount]->rtype = FIX_SELF_OFS16; break;
				case 2: relocations[fixcount]->rtype = FIX_SELF_OFS32; break;
				default:
					printf("Bad BAKPAT record\n");
					exit(1);
				}
				relocations[fixcount]->offset = buffer[j] + (((UINT)buffer[j + 1]) << 8);
				j += 2;
				if (record_type == BAKPAT32)
				{
					relocations[fixcount]->offset += (buffer[j] + (((UINT)buffer[j + 1]) << 8)) << 16;
					j += 2;
				}
				relocations[fixcount]->segment = i;
				relocations[fixcount]->target = i;
				relocations[fixcount]->frame = i;
				relocations[fixcount]->ttype = REL_SEGDISP;
				relocations[fixcount]->ftype = REL_SEGFRAME;
				relocations[fixcount]->disp = buffer[j] + (((UINT)buffer[j + 1]) << 8);
				j += 2;
				if (record_type == BAKPAT32)
				{
					relocations[fixcount]->disp += (buffer[j] + (((UINT)buffer[j + 1]) << 8)) << 16;
					j += 2;
				}
				relocations[fixcount]->disp += relocations[fixcount]->offset;
				switch (k)
				{
				case 0: relocations[fixcount]->disp++; break;
				case 1: relocations[fixcount]->disp += 2; break;
				case 2: relocations[fixcount]->disp += 4; break;
				default:
					printf("Bad BAKPAT record\n");
					exit(1);
				}
				fixcount++;
			}
			break;
		}
		case LINNUM:
		case LINNUM32:
			//printf("LINNUM record\n");
			break;
		case MODEND:
		case MODEND32:
		{
			done = 1;
			if (buffer[0] & 0x40)
			{
				if (got_start_address)
				{
					report_error(fname, ERR_MULTIPLE_STARTS);
				}
				got_start_address = 1;
				j = 1;
				load_fixup(fname, &start_address, buffer, &j);
				if (start_address.ftype == REL_LILEFRAME)
				{
					report_error(fname, ERR_BAD_FIXUP);
				}
			}
			break;

		}
		case COMDEF:
		{
			for (j = 0; j < record_length;)
			{
				extern_records = (PEXTREC)check_realloc(extern_records, (extcount + 1) * sizeof(EXTREC));
				extern_records[extcount].name = check_malloc(buffer[j] + 1);
				k = buffer[j];
				j++;
				for (i = 0; i < k; i++, j++)
				{
					extern_records[extcount].name[i] = buffer[j];
				}
				extern_records[extcount].name[i] = 0;
				if (!case_sensitive)
				{
					strupr(extern_records[extcount].name);
				}
				extern_records[extcount].type = get_index(buffer, &j);
				extern_records[extcount].pubdef = NULL;
				extern_records[extcount].flags = EXT_NOMATCH;
				extern_records[extcount].mod = 0;
				if (buffer[j] == 0x61)
				{
					j++;
					i = buffer[j];
					j++;
					if (i == 0x81)
					{
						i = buffer[j] + (((UINT)buffer[j + 1]) << 8);
						j += 2;
					}
					else if (i == 0x84)
					{
						i = buffer[j] + (((UINT)buffer[j + 1]) << 8) + (((UINT)buffer[j + 2]) << 16);
						j += 3;
					}
					else if (i == 0x88)
					{
						i = buffer[j] + (((UINT)buffer[j + 1]) << 8) + (((UINT)buffer[j + 2]) << 16) + (((UINT)buffer[j + 3]) << 24);
						j += 4;
					}
					k = i;
					i = buffer[j];
					j++;
					if (i == 0x81)
					{
						i = buffer[j] + (((UINT)buffer[j + 1]) << 8);
						j += 2;
					}
					else if (i == 0x84)
					{
						i = buffer[j] + (((UINT)buffer[j + 1]) << 8) + (((UINT)buffer[j + 2]) << 16);
						j += 3;
					}
					else if (i == 0x88)
					{
						i = buffer[j] + (((UINT)buffer[j + 1]) << 8) + (((UINT)buffer[j + 2]) << 16) + (((UINT)buffer[j + 3]) << 24);
						j += 4;
					}
					i *= k;
					k = 1;
				}
				else if (buffer[j] == 0x62)
				{
					j++;
					i = buffer[j];
					j++;
					if (i == 0x81)
					{
						i = buffer[j] + (((UINT)buffer[j + 1]) << 8);
						j += 2;
					}
					else if (i == 0x84)
					{
						i = buffer[j] + (((UINT)buffer[j + 1]) << 8) + (((UINT)buffer[j + 2]) << 16);
						j += 3;
					}
					else if (i == 0x88)
					{
						i = buffer[j] + (((UINT)buffer[j + 1]) << 8) + (((UINT)buffer[j + 2]) << 16) + (((UINT)buffer[j + 3]) << 24);
						j += 4;
					}
					k = 0;
				}
				else
				{
					printf("Unknown COMDEF data type %02X\n", buffer[j]);
					exit(1);
				}
				common_definitions = (PPCOMREC)check_realloc(common_definitions, (comcount + 1) * sizeof(PCOMREC));
				common_definitions[comcount] = (PCOMREC)check_malloc(sizeof(COMREC));
				common_definitions[comcount]->length = i;
				common_definitions[comcount]->is_far = k;
				common_definitions[comcount]->modnum = 0;
				common_definitions[comcount]->name = check_strdup(extern_records[extcount].name);
				extcount++;
				comcount++;
			}

			break;
		}
		case COMDAT:
		case COMDAT32:
		{
			printf("COMDAT section\n");
			exit(1);

			break;
		}
		case ALIAS:
		{
			printf("ALIAS record\n");
			j = 0;
			name = check_malloc(buffer[j] + 1);
			k = buffer[j];
			j++;
			for (i = 0; i < k; i++)
			{
				name[i] = buffer[j];
				j++;
			}
			name[i] = 0;
			if (!case_sensitive)
			{
				strupr(name);
			}
			printf("ALIAS name:%s\n", name);
			aliasName = check_malloc(buffer[j] + 1);
			k = buffer[j];
			j++;
			for (i = 0; i < k; i++)
			{
				aliasName[i] = buffer[j];
				j++;
			}
			aliasName[i] = 0;
			if (!case_sensitive)
			{
				strupr(aliasName);
			}
			printf("Substitute name:%s\n", aliasName);
			if (!strlen(name))
			{
				printf("Cannot use alias a blank name\n");
				exit(1);
			}
			if (!strlen(aliasName))
			{
				printf("No Alias name specified for %s\n", name);
				exit(1);
			}
			pubdef = (PPUBLIC)check_malloc(sizeof(PUBLIC));
			pubdef->segment = -1;
			pubdef->group = -1;
			pubdef->type = -1;
			pubdef->offset = 0;
			pubdef->mod = 0;
			pubdef->alias = aliasName;
			if (listnode = binary_search(public_entries, pubcount, name))
			{
				for (i = 0; i < listnode->count; i++)
				{
					if (((PPUBLIC)listnode->object[i])->mod == pubdef->mod)
					{
						if (((PPUBLIC)listnode->object[i])->alias)
						{
							printf("Warning, two aliases for %s, using %s\n", name, ((PPUBLIC)listnode->object[i])->alias);
						}
						free(pubdef->alias);
						free(pubdef);
						pubdef = NULL;
						break;
					}
				}
			}
			if (pubdef)
			{
				sort_insert(&public_entries, &pubcount, name, pubdef);
			}
			free(name);
			break;
		}
		default:
			report_error(fname, ERR_UNKNOWN_RECTYPE);
		}
		file_position += 4 + record_length;
		modpos += 4 + record_length;
	}
	if (lidata)
	{
		destroy_lidata(lidata);
	}
	return 0;
}

void load_lib(PCHAR libname, FILE* libfile)
{
	unsigned int i, j, k, n;
	PCHAR name;
	unsigned short modpage;
	PLIBFILE p;
	UINT numsyms;
	PSORTENTRY symlist;

	library_files = check_realloc(library_files, (libcount + 1) * sizeof(LIBFILE));
	p = &library_files[libcount];

	p->file_name = check_malloc(strlen(libname) + 1);
	strcpy(p->file_name, libname);

	if (fread(buffer, 1, 3, libfile) != 3)
	{
		printf("Error reading from file\n");
		exit(1);
	}
	p->block_size = buffer[1] + (((UINT)buffer[2]) << 8);
	if (fread(buffer, 1, p->block_size, libfile) != p->block_size)
	{
		printf("Error reading from file\n");
		exit(1);
	}
	p->block_size += 3;
	p->dic_start = buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
	p->num_dic_pages = buffer[4] + (((UINT)buffer[5]) << 8);
	p->flags = buffer[6];
	p->lib_type = 'O';

	fseek(libfile, p->dic_start, SEEK_SET);

	symlist = (PSORTENTRY)check_malloc(p->num_dic_pages * 37 * sizeof(SORTENTRY));

	numsyms = 0;
	for (i = 0; i < p->num_dic_pages; i++)
	{
		if (fread(buffer, 1, 512, libfile) != 512)
		{
			printf("Error reading from file\n");
			exit(1);
		}
		for (j = 0; j < 37; j++)
		{
			k = buffer[j] * 2;
			if (k)
			{
				name = check_malloc(buffer[k] + 1);
				for (n = 0; n < buffer[k]; n++)
				{
					name[n] = buffer[n + k + 1];
				}
				name[buffer[k]] = 0;
				k += buffer[k] + 1;
				modpage = buffer[k] + (((UINT)buffer[k + 1]) << 8);
				if (!(p->flags & LIBF_CASESENSITIVE) || !case_sensitive)
				{
					strupr(name);
				}
				if (name[strlen(name) - 1] == '!')
				{
					free(name);
				}
				else
				{
					symlist[numsyms].id = name;
					symlist[numsyms].count = modpage;
					++numsyms;
				}
			}
		}
	}

	qsort(symlist, numsyms, sizeof(SORTENTRY), sort_compare);
	p->symbols = symlist;
	p->num_syms = numsyms;
	p->mods_loaded = 0;
	p->mod_list = check_malloc(sizeof(unsigned short) * numsyms);
	libcount++;
}

void load_lib_mod(UINT libnum, UINT modpage)
{
	PLIBFILE p;
	FILE* libfile;
	UINT i;

	p = &library_files[libnum];

	/* don't open a module we've loaded already */
	for (i = 0; i < p->mods_loaded; i++)
	{
		if (p->mod_list[i] == modpage) return;
	}

	libfile = fopen(p->file_name, "rb");
	if (!libfile)
	{
		printf("Error opening file %s\n", p->file_name);
		exit(1);
	}
	fseek(libfile, modpage * p->block_size, SEEK_SET);
	switch (p->lib_type)
	{
	case 'O':
		load_mod(p->file_name, libfile);
		break;
	case 'C':
		load_coff_lib_mod(p->file_name, p, libfile);
		break;
	default:
		printf("Unknown library file format\n");
		exit(1);
	}

	p->mod_list[p->mods_loaded] = modpage;
	p->mods_loaded++;
	fclose(libfile);
}

void load_resource(PCHAR name, FILE* f)
{
	unsigned char buf[32];
	static unsigned char buf2[32] = { 0,0,0,0,0x20,0,0,0,0xff,0xff,0,0,0xff,0xff,0,0,
				   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	UINT i, j;
	UINT hdrsize, datsize;
	PUCHAR data;
	PUCHAR hdr;

	if (fread(buf, 1, 32, f) != 32)
	{
		printf("Invalid resource file\n");
		exit(1);
	}
	if (memcmp(buf, buf2, 32))
	{
		printf("Invalid resource file\n");
		exit(1);
	}
	printf("Loading Win32 Resource File\n");
	while (!feof(f))
	{
		i = ftell(f);
		if (i & 3)
		{
			fseek(f, 4 - (i & 3), SEEK_CUR);
		}
		i = (UINT)fread(buf, 1, 8, f);
		if (i == 0 && feof(f)) return;
		if (i != 8)
		{
			printf("Invalid resource file, no header\n");
			exit(1);
		}
		datsize = buf[0] + (buf[1] << 8) + (buf[2] << 16) + (buf[3] << 24);
		hdrsize = buf[4] + (buf[5] << 8) + (buf[6] << 16) + (buf[7] << 24);
		if (hdrsize < 16)
		{
			printf("Invalid resource file, bad header\n");
			exit(1);
		}
		hdr = (PUCHAR)check_malloc(hdrsize);
		if (fread(hdr, 1, hdrsize - 8, f) != (hdrsize - 8))
		{
			printf("Invalid resource file, missing header\n");
			exit(1);
		}
		/* if this is a NULL resource, then skip */
		if (!datsize && (hdrsize == 32) && !memcmp(buf2 + 8, hdr, 24))
		{
			free(hdr);
			continue;
		}
		if (datsize)
		{
			data = (PUCHAR)check_malloc(datsize);
			if (fread(data, 1, datsize, f) != datsize)
			{
				printf("Invalid resource file, no data\n");
				exit(1);
			}
		}
		else data = NULL;
		resource = (PRESOURCE)check_realloc(resource, (rescount + 1) * sizeof(RESOURCE));
		resource[rescount].data = data;
		resource[rescount].length = datsize;
		i = 0;
		hdrsize -= 8;
		if ((hdr[i] == 0xff) && (hdr[i + 1] == 0xff))
		{
			resource[rescount].typename = NULL;
			resource[rescount].typeid = hdr[i + 2] + (((UINT)hdr[i + 3]) << 8);
			i += 4;
		}
		else
		{
			for (j = i; (j < (hdrsize - 1)) && (hdr[j] | hdr[j + 1]); j += 2);
			if (hdr[j] | hdr[j + 1])
			{
				printf("Invalid resource file, bad name\n");
				exit(1);
			}
			resource[rescount].typename = (PUCHAR)check_malloc(j - i + 2);
			memcpy(resource[rescount].typename, hdr + i, j - i + 2);
			i = j + 5;
			i &= 0xfffffffc;
		}
		if (i > hdrsize)
		{
			printf("Invalid resource file, overflow\n");
			exit(1);
		}
		if ((hdr[i] == 0xff) && (hdr[i + 1] == 0xff))
		{
			resource[rescount].name = NULL;
			resource[rescount].id = hdr[i + 2] + (((UINT)hdr[i + 3]) << 8);
			i += 4;
		}
		else
		{
			for (j = i; (j < (hdrsize - 1)) && (hdr[j] | hdr[j + 1]); j += 2);
			if (hdr[j] | hdr[j + 1])
			{
				printf("Invalid resource file,bad name (2)\n");
				exit(1);
			}
			resource[rescount].name = (PUCHAR)check_malloc(j - i + 2);
			memcpy(resource[rescount].name, hdr + i, j - i + 2);
			i = j + 5;
			i &= 0xfffffffc;
		}
		i += 6; /* point to Language ID */
		if (i > hdrsize)
		{
			printf("Invalid resource file, overflow(2)\n");
			exit(1);
		}
		resource[rescount].language_id = hdr[i] + (((UINT)hdr[i + 1]) << 8);
		rescount++;
		free(hdr);
	}
}
