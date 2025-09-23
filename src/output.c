#include "alink.h"

static unsigned char default_stub[] = {
	0x4D,0x5A,0x6C,0x00,0x01,0x00,0x00,0x00,
	0x04,0x00,0x11,0x00,0xFF,0xFF,0x03,0x00,
	0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
	0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x0E,0x1F,0xBA,0x0E,0x00,0xB4,0x09,0xCD,
	0x21,0xB8,0x00,0x4C,0xCD,0x21,0x54,0x68,
	0x69,0x73,0x20,0x70,0x72,0x6F,0x67,0x72,
	0x61,0x6D,0x20,0x72,0x65,0x71,0x75,0x69,
	0x72,0x65,0x73,0x20,0x57,0x69,0x6E,0x33,
	0x32,0x0D,0x0A,0x24
};

static UINT defaultStubSize = sizeof(default_stub);

void get_fixup_target(PCHAR fname, PRELOC relocation, long* _base_segment, UINT* _target_offset, int is_flat)
{
	long base_segment = -1L;
	long target_segment = -1L;
	UINT target_offset = 0;

	if (relocation->segment < 0) return;

	relocation->output_pos 
		= segment_list[relocation->segment]->base 
		+ relocation->offset;

	switch (relocation->ftype)
	{
	case REL_SEGFRAME:
	case REL_LILEFRAME:
		base_segment = relocation->frame;
		break;
	case REL_GRPFRAME:
		base_segment = group_list[relocation->frame]->segnum;
		break;
	case REL_EXTFRAME:
		switch (extern_records[relocation->frame].flags)
		{
		case EXT_MATCHEDPUBLIC:
			if (!extern_records[relocation->frame].pubdef)
			{
				printf("Reloc:Unmatched extern %s\n", extern_records[relocation->frame].name);
				error_count++;
				break;
			}

			base_segment = extern_records[relocation->frame].pubdef->segment;
			break;
		case EXT_MATCHEDIMPORT:
			base_segment = import_definitions[extern_records[relocation->frame].import].segment;
			break;
		default:
			printf("Reloc:Unmatched external referenced in frame\n");
			error_count++;
			break;
		}
		break;
	default:
		printf("Reloc:Unsupported FRAME type %i\n", relocation->ftype);
		error_count++;
	}
	if (base_segment < 0)
	{
		printf("Undefined base seg\n");
		exit(1);
	}   /* this is a fix for TASM FLAT model, where FLAT group has no segments */

	switch (relocation->ttype)
	{
	case REL_EXTDISP:
		switch (extern_records[relocation->target].flags)
		{
		case EXT_MATCHEDPUBLIC:
			if (!extern_records[relocation->target].pubdef)
			{
				printf("Reloc:Unmatched extern %s\n", extern_records[relocation->frame].name);
				error_count++;
				break;
			}

			target_segment = extern_records[relocation->target].pubdef->segment;
			target_offset = extern_records[relocation->target].pubdef->offset;
			break;
		case EXT_MATCHEDIMPORT:
			target_segment = import_definitions[extern_records[relocation->target].import].segment;
			target_offset = import_definitions[extern_records[relocation->target].import].offset;
			break;
		default:
			printf("Reloc:Unmatched external referenced in frame\n");
			error_count++;
			break;
		}
		target_offset += relocation->disp;
		break;
	case REL_EXTONLY:
		switch (extern_records[relocation->target].flags)
		{
		case EXT_MATCHEDPUBLIC:
			if (!extern_records[relocation->target].pubdef)
			{
				printf("Reloc:Unmatched extern %s\n", extern_records[relocation->target].name);
				error_count++;
				break;
			}

			target_segment = extern_records[relocation->target].pubdef->segment;
			target_offset = extern_records[relocation->target].pubdef->offset;
			break;
		case EXT_MATCHEDIMPORT:
			target_segment = import_definitions[extern_records[relocation->target].import].segment;
			target_offset = import_definitions[extern_records[relocation->target].import].offset;
			break;
		default:
			printf("Reloc:Unmatched external referenced in frame\n");
			error_count++;
			break;
		}
		break;
	case REL_SEGONLY:
		target_segment = relocation->target;
		target_offset = 0;
		break;
	case REL_SEGDISP:
		target_segment = relocation->target;
		target_offset = relocation->disp;
		break;
	case REL_GRPONLY:
		target_segment = group_list[relocation->target]->segnum;
		target_offset = 0;
		break;
	case REL_GRPDISP:
		target_segment = group_list[relocation->target]->segnum;
		target_offset = relocation->disp;
		break;
	default:
		printf("Reloc:Unsupported TARGET type %i\n", relocation->ttype);
		error_count++;
	}
	if (target_segment < 0)
	{
		printf("undefined seg\n");
		target_segment = segcount_combined - 1;
		//NOTICE:
		error_count++;
		//exit(1);
	}
	if ((!error_count) && (!segment_list[target_segment]))
	{
		printf("Reloc: no target segment\n");

		error_count++;
	}
	if ((!error_count) && (!segment_list[base_segment]))
	{
		printf("reloc: no base segment\n");

		error_count++;
	}

	if (!error_count)
	{
		/*
		if(((seglist[targseg]->attr&SEG_ALIGN)!=SEG_ABS) &&
			((seglist[baseseg]->attr&SEG_ALIGN)!=SEG_ABS))
		{
			if(seglist[baseseg]->base>seglist[targseg]->base)
			{
				printf("Reloc:Negative base address\n");
				errcount++;
			}
			targofs+=seglist[targseg]->base-seglist[baseseg]->base;
		}
		*/
		if ((segment_list[base_segment]->attributes & SEG_ALIGN) == SEG_ABS)
		{
			printf("Warning: Reloc frame is absolute segment\n");
			target_segment = base_segment;
		}
		else if ((segment_list[target_segment]->attributes & SEG_ALIGN) == SEG_ABS)
		{
			printf("Warning: Reloc target is in absolute segment\n");
			target_segment = base_segment;
		}
		if (!is_flat || ((segment_list[base_segment]->attributes & SEG_ALIGN) == SEG_ABS))
		{
			if (segment_list[base_segment]->base > (segment_list[target_segment]->base + target_offset))
			{
				printf("Error: target address out of frame\n");
				printf("Base=%08X,target=%08X\n",
					segment_list[base_segment]->base, segment_list[target_segment]->base + target_offset);
				error_count++;
			}
			target_offset += segment_list[target_segment]->base - segment_list[base_segment]->base;
			*_base_segment = base_segment;
			*_target_offset = target_offset;
		}
		else
		{
			*_base_segment = -1;
			*_target_offset = target_offset + segment_list[target_segment]->base;
		}
	}
	else
	{
		//printf("relocation error occurred\n");
		*_base_segment = 0;
		*_target_offset = 0;
	}
}


void output_com_file(PCHAR outname)
{
	long i, j;
	UINT started;
	UINT lastout;
	long targseg;
	UINT targofs;
	FILE* outfile;
	USHORT temps;
	USHORT templ;

	if (impsreq)
	{
		report_error(outname, ERR_ILLEGAL_IMPORTS);
	}

	error_count = 0;
	if (got_start_address)
	{
		get_fixup_target(outname, &start_address, &start_address.segment, &start_address.offset, FALSE);
		if (error_count)
		{
			printf("Invalid start address record\n");
			exit(1);
		}

		if ((start_address.offset + segment_list[start_address.segment]->base) != 0x100)
		{
			printf("Warning, start address not 0100h as required for COM file\n");
		}
	}
	else
	{
		printf("Warning, no entry point specified\n");
	}

	for (i = 0; i < fixcount; i++)
	{
		get_fixup_target(outname, relocations[i], &targseg, &targofs, FALSE);
		switch (relocations[i]->rtype)
		{
		case FIX_BASE:
		case FIX_PTR1616:
		case FIX_PTR1632:
			if ((segment_list[targseg]->attributes & SEG_ALIGN) != SEG_ABS)
			{
				printf("Reloc %li:Segment selector relocations are not supported in COM files\n", i);
				error_count++;
			}
			else
			{
				j = relocations[i]->offset;
				if (relocations[i]->rtype == FIX_PTR1616)
				{
					if (targofs > 0xffff)
					{
						printf("Relocs %li:Offset out of range\n", i);
						error_count++;
					}
					temps = segment_list[relocations[i]->segment]->data[j];
					temps += segment_list[relocations[i]->segment]->data[j + 1] << 8;
					temps += (USHORT)targofs;
					temps += segment_list[targseg]->base & 0xf; /* non-para seg */
					segment_list[relocations[i]->segment]->data[j] = temps & 0xff;
					segment_list[relocations[i]->segment]->data[j + 1] = (temps >> 8) & 0xff;
					j += 2;
				}
				else if (relocations[i]->rtype == FIX_PTR1632)
				{
					templ = segment_list[relocations[i]->segment]->data[j];
					templ += segment_list[relocations[i]->segment]->data[j + 1] << 8;
					templ += segment_list[relocations[i]->segment]->data[j + 2] << 16;
					templ += segment_list[relocations[i]->segment]->data[j + 3] << 24;
					templ += (USHORT)targofs;
					templ += segment_list[targseg]->base & 0xf; /* non-para seg */
					segment_list[relocations[i]->segment]->data[j] = templ & 0xff;
					segment_list[relocations[i]->segment]->data[j + 1] = (templ >> 8) & 0xff;
					segment_list[relocations[i]->segment]->data[j + 2] = (templ >> 16) & 0xff;
					segment_list[relocations[i]->segment]->data[j + 3] = (templ >> 24) & 0xff;
					j += 4;
				}
				temps = segment_list[relocations[i]->segment]->data[j];
				temps += segment_list[relocations[i]->segment]->data[j + 1] << 8;
				temps += (USHORT)segment_list[targseg]->absolute_frame;
				segment_list[relocations[i]->segment]->data[j] = temps & 0xff;
				segment_list[relocations[i]->segment]->data[j + 1] = (temps >> 8) & 0xff;
			}
			break;
		case FIX_OFS32:
		case FIX_OFS32_2:
			templ = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
			templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
			templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] << 16;
			templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] << 24;
			templ += (USHORT)targofs;
			templ += segment_list[targseg]->base & 0xf; /* non-para seg */
			segment_list[relocations[i]->segment]->data[relocations[i]->offset] = templ & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (templ >> 8) & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] = (templ >> 16) & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] = (templ >> 24) & 0xff;
			break;
		case FIX_OFS16:
		case FIX_OFS16_2:
			if (targofs > 0xffff)
			{
				printf("Relocs %li:Offset out of range\n", i);
				error_count++;
			}
			temps = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
			temps += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
			temps += (USHORT)targofs;
			temps += segment_list[targseg]->base & 0xf; /* non-para seg */
			segment_list[relocations[i]->segment]->data[relocations[i]->offset] = temps & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (temps >> 8) & 0xff;
			break;
		case FIX_LBYTE:
			segment_list[relocations[i]->segment]->data[relocations[i]->offset] += targofs & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset] += segment_list[targseg]->base & 0xf; /* non-para seg */
			break;
		case FIX_HBYTE:
			templ = (USHORT)targofs + (USHORT)(segment_list[targseg]->base & 0xf); /* non-para seg */
			segment_list[relocations[i]->segment]->data[relocations[i]->offset] += (templ >> 8) & 0xff;
			break;
		case FIX_SELF_LBYTE:
			if ((segment_list[targseg]->attributes & SEG_ALIGN) == SEG_ABS)
			{
				printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
				error_count++;
			}
			else
			{
				j = segment_list[targseg]->base + targofs;
				j -= segment_list[relocations[i]->segment]->base + relocations[i]->offset + 1;
				if ((j < -128) || (j > 127))
				{
					printf("Error: Reloc %li out of range\n", i);
				}
				else
				{
					segment_list[relocations[i]->segment]->data[relocations[i]->offset] += (UCHAR)j;
				}
			}
			break;
		case FIX_SELF_OFS16:
		case FIX_SELF_OFS16_2:
			if ((segment_list[targseg]->attributes & SEG_ALIGN) == SEG_ABS)
			{
				printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
				error_count++;
			}
			else
			{
				j = segment_list[targseg]->base + targofs;
				j -= segment_list[relocations[i]->segment]->base + relocations[i]->offset + 2;
				if ((j < -32768) || (j > 32767))
				{
					printf("Error: Reloc %li out of range\n", i);
				}
				else
				{
					temps = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
					temps += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
					temps += (USHORT)j;
					segment_list[relocations[i]->segment]->data[relocations[i]->offset] = temps & 0xff;
					segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (temps >> 8) & 0xff;
				}
			}
			break;
		case FIX_SELF_OFS32:
		case FIX_SELF_OFS32_2:
			if ((segment_list[targseg]->attributes & SEG_ALIGN) == SEG_ABS)
			{
				printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
				error_count++;
			}
			else
			{
				j = segment_list[targseg]->base + targofs;
				j -= segment_list[relocations[i]->segment]->base + relocations[i]->offset + 4;
				templ = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
				templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
				templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] << 16;
				templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] << 24;
				templ += (USHORT)j;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset] = templ & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (templ >> 8) & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] = (templ >> 16) & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] = (templ >> 24) & 0xff;
			}
			break;
		default:
			printf("Reloc %li:Relocation type %i not supported\n", i, relocations[i]->rtype);
			error_count++;
		}
	}

	if (error_count != 0)
	{
		exit(1);
	}
	outfile = fopen(outname, "wb");
	if (!outfile)
	{
		printf("Error writing to file %s\n", outname);
		exit(1);
	}

	started = lastout = 0;

	for (i = 0; i < outcount; i++)
	{
		if (out_list[i] && ((out_list[i]->attributes & SEG_ALIGN) != SEG_ABS))
		{
			if (started > out_list[i]->base)
			{
				printf("Segment overlap\n");
				fclose(outfile);
				exit(1);
			}
			if (pad_segments)
			{
				while (started < out_list[i]->base)
				{
					fputc(0, outfile);
					started++;
				}
			}
			else
			{
				started = out_list[i]->base;
			}
			for (j = 0; j < out_list[i]->length; j++)
			{
				if (started >= 0x100)
				{
					if (get_n_bit(out_list[i]->data_mask, j))
					{
						for (; lastout < started; lastout++)
						{
							fputc(0, outfile);
						}
						fputc(out_list[i]->data[j], outfile);
						lastout = started + 1;
					}
					else if (pad_segments)
					{
						fputc(0, outfile);
						lastout = started + 1;
					}
				}
				else
				{
					lastout = started + 1;
					if (get_n_bit(out_list[i]->data_mask, j))
					{
						printf("Warning - data at offset %08lX (%s:%08lX) discarded\n", started, name_list[out_list[i]->name_index], j);
					}
				}
				started++;
			}
		}
	}

	fclose(outfile);
}

void output_exe_file(PCHAR out_name)
{
	long i, j;
	UINT started, lastout;
	long target_segment;
	UINT target_offset;
	FILE* outfile;
	PUCHAR header_buffer;
	long relocation_count;
	int got_stack;
	UINT totlength;
	unsigned short temp_segment;
	unsigned long temp_offset;

	if (impsreq)
	{
		report_error(out_name, ERR_ILLEGAL_IMPORTS);
	}

	error_count = 0;
	got_stack = 0;
	header_buffer = check_malloc(0x40 + 4 * fixcount);
	relocation_count = 0;

	for (i = 0; i < 0x40; i++)
	{
		header_buffer[i] = 0;
	}

	header_buffer[0x00] = 'M'; /* sig */
	header_buffer[0x01] = 'Z';
	header_buffer[0x0c] = max_alloc & 0xff;
	header_buffer[0x0d] = max_alloc >> 8;
	header_buffer[0x18] = 0x40;

	if (got_start_address)
	{
		get_fixup_target(out_name, &start_address, &start_address.segment, &start_address.offset, FALSE);
		if (error_count)
		{
			printf("Invalid start address record\n");
			exit(1);
		}

		i = segment_list[start_address.segment]->base;
		start_address.offset += i & 0xf;
		i >>= 4;
		if ((start_address.offset > 0x10000) || (i > 0x10000) || ((segment_list[start_address.segment]->attributes & SEG_ALIGN) == SEG_ABS))
		{
			printf("Invalid start address\n");
			error_count++;
		}
		else
		{
			header_buffer[0x14] = (UCHAR)(start_address.offset & 0xff);
			header_buffer[0x15] = (UCHAR)(start_address.offset >> 8);

			header_buffer[0x16] = (UCHAR)(i & 0xff);
			header_buffer[0x17] = (UCHAR)(i >> 8);
		}
	}
	else
	{
		printf("Warning, no entry point specified\n");
	}

	totlength = 0;

	for (i = 0; i < outcount; i++)
	{
		if ((out_list[i]->attributes & SEG_ALIGN) != SEG_ABS)
		{
			totlength = out_list[i]->base + out_list[i]->length;
			if ((out_list[i]->attributes & SEG_COMBINE) == SEG_STACK)
			{
				if (got_stack)
				{
					printf("Internal error - stack segments not combined\n");
					exit(1);
				}
				got_stack = 1;
				if ((out_list[i]->length > 0x10000) || (out_list[i]->length == 0))
				{
					printf("SP value out of range\n");
					error_count++;
				}
				if ((out_list[i]->base > 0xfffff) || ((out_list[i]->base & 0xf) != 0))
				{
					printf("SS value out of range\n");
					error_count++;
				}
				if (!error_count)
				{
					header_buffer[0x0e] = (UCHAR)((out_list[i]->base >> 4) & 0xff);
					header_buffer[0x0f] = (UCHAR)(out_list[i]->base >> 12);
					header_buffer[0x10] = (UCHAR)(out_list[i]->length & 0xff);
					header_buffer[0x11] = (UCHAR)((out_list[i]->length >> 8) & 0xff);
				}
			}
		}
	}

	if (!got_stack)
	{
		printf("Warning - no stack\n");
	}

	for (i = 0; i < fixcount; i++)
	{
		get_fixup_target(out_name, relocations[i], &target_segment, &target_offset, FALSE);
		switch (relocations[i]->rtype)
		{
		case FIX_BASE:
		case FIX_PTR1616:
		case FIX_PTR1632:
			j = relocations[i]->offset;
			if (relocations[i]->rtype == FIX_PTR1616)
			{
				if (target_offset > 0xffff)
				{
					printf("Relocs %li:Offset out of range\n", i);
					error_count++;
				}
				temp_segment = segment_list[relocations[i]->segment]->data[j];
				temp_segment += segment_list[relocations[i]->segment]->data[j + 1] << 8;
				temp_segment += (UCHAR)target_offset;
				temp_segment += segment_list[target_segment]->base & 0xf; /* non-para seg */
				segment_list[relocations[i]->segment]->data[j] = temp_segment & 0xff;
				segment_list[relocations[i]->segment]->data[j + 1] = (temp_segment >> 8) & 0xff;
				j += 2;
			}
			else if (relocations[i]->rtype == FIX_PTR1632)
			{
				temp_offset = segment_list[relocations[i]->segment]->data[j];
				temp_offset += segment_list[relocations[i]->segment]->data[j + 1] << 8;
				temp_offset += segment_list[relocations[i]->segment]->data[j + 2] << 16;
				temp_offset += segment_list[relocations[i]->segment]->data[j + 3] << 24;
				temp_offset += target_offset;
				temp_offset += segment_list[target_segment]->base & 0xf; /* non-para seg */
				segment_list[relocations[i]->segment]->data[j] = temp_offset & 0xff;
				segment_list[relocations[i]->segment]->data[j + 1] = (temp_offset >> 8) & 0xff;
				segment_list[relocations[i]->segment]->data[j + 2] = (temp_offset >> 16) & 0xff;
				segment_list[relocations[i]->segment]->data[j + 3] = (temp_offset >> 24) & 0xff;
				j += 4;
			}
			if ((segment_list[target_segment]->attributes & SEG_ALIGN) != SEG_ABS)
			{
				if (segment_list[target_segment]->base > 0xfffff)
				{
					printf("Relocs %li:Segment base out of range\n", i);
					error_count++;
				}
				temp_segment = segment_list[relocations[i]->segment]->data[j];
				temp_segment += segment_list[relocations[i]->segment]->data[j + 1] << 8;
				temp_segment += (UCHAR)(segment_list[target_segment]->base >> 4);
				segment_list[relocations[i]->segment]->data[j] = temp_segment & 0xff;
				segment_list[relocations[i]->segment]->data[j + 1] = (temp_segment >> 8) & 0xff;
				temp_offset = segment_list[relocations[i]->segment]->base >> 4;
				header_buffer[0x40 + relocation_count * 4 + 2] = temp_offset & 0xff;
				header_buffer[0x40 + relocation_count * 4 + 3] = (temp_offset >> 8) & 0xff;
				temp_offset = (segment_list[relocations[i]->segment]->base & 0xf) + j;
				header_buffer[0x40 + relocation_count * 4] = (temp_offset) & 0xff;
				header_buffer[0x40 + relocation_count * 4 + 1] = (temp_offset >> 8) & 0xff;
				relocation_count++;
			}
			else
			{
				temp_segment = segment_list[relocations[i]->segment]->data[j];
				temp_segment += segment_list[relocations[i]->segment]->data[j + 1] << 8;
				temp_segment += (UCHAR)segment_list[target_segment]->absolute_frame;
				segment_list[relocations[i]->segment]->data[j] = temp_segment & 0xff;
				segment_list[relocations[i]->segment]->data[j + 1] = (temp_segment >> 8) & 0xff;
			}
			break;
		case FIX_OFS32:
		case FIX_OFS32_2:
			temp_offset = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
			temp_offset += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
			temp_offset += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] << 16;
			temp_offset += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] << 24;
			temp_offset += target_offset;
			temp_offset += segment_list[target_segment]->base & 0xf; /* non-para seg */
			segment_list[relocations[i]->segment]->data[relocations[i]->offset] = temp_offset & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (temp_offset >> 8) & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] = (temp_offset >> 16) & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] = (temp_offset >> 24) & 0xff;
			break;
		case FIX_OFS16:
		case FIX_OFS16_2:
			if (target_offset > 0xffff)
			{
				printf("Relocs %li:Offset out of range\n", i);
				error_count++;
			}
			temp_segment = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
			temp_segment += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
			temp_segment += (UCHAR)target_offset;
			temp_segment += segment_list[target_segment]->base & 0xf; /* non-para seg */
			segment_list[relocations[i]->segment]->data[relocations[i]->offset] = temp_segment & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (temp_segment >> 8) & 0xff;
			break;
		case FIX_LBYTE:
			segment_list[relocations[i]->segment]->data[relocations[i]->offset] += target_offset & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset] += segment_list[target_segment]->base & 0xf; /* non-para seg */
			break;
		case FIX_HBYTE:
			temp_offset = target_offset + (segment_list[target_segment]->base & 0xf); /* non-para seg */
			segment_list[relocations[i]->segment]->data[relocations[i]->offset] += (temp_offset >> 8) & 0xff;
			break;
		case FIX_SELF_LBYTE:
			if ((segment_list[target_segment]->attributes & SEG_ALIGN) == SEG_ABS)
			{
				printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
				error_count++;
			}
			else
			{
				j = segment_list[target_segment]->base + target_offset;
				j -= segment_list[relocations[i]->segment]->base + relocations[i]->offset + 1;
				if ((j < -128) || (j > 127))
				{
					//NOTICE:
					printf("Error: Reloc %li out of range\n", i);
				}
				else
				{
					segment_list[relocations[i]->segment]->data[relocations[i]->offset] += (UCHAR)j;
				}
			}
			break;
		case FIX_SELF_OFS16:
		case FIX_SELF_OFS16_2:
			if ((segment_list[target_segment]->attributes & SEG_ALIGN) == SEG_ABS)
			{
				printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
				error_count++;
			}
			else
			{
				j = segment_list[target_segment]->base + target_offset;
				j -= segment_list[relocations[i]->segment]->base + relocations[i]->offset + 2;
				if ((j < -32768) || (j > 32767))
				{
					//NOTICE:
					printf("Error: Reloc %li out of range\n", i);
				}
				else
				{
					temp_segment = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
					temp_segment += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
					temp_segment += (UCHAR)j;
					segment_list[relocations[i]->segment]->data[relocations[i]->offset] = temp_segment & 0xff;
					segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (temp_segment >> 8) & 0xff;
				}
			}
			break;
		case FIX_SELF_OFS32:
		case FIX_SELF_OFS32_2:
			if ((segment_list[target_segment]->attributes & SEG_ALIGN) == SEG_ABS)
			{
				printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
				error_count++;
			}
			else
			{
				j = segment_list[target_segment]->base + target_offset;
				j -= segment_list[relocations[i]->segment]->base + relocations[i]->offset + 4;
				temp_offset = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
				temp_offset += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
				temp_offset += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] << 16;
				temp_offset += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] << 24;
				temp_offset += j;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset] = temp_offset & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (temp_offset >> 8) & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] = (temp_offset >> 16) & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] = (temp_offset >> 24) & 0xff;
			}
			break;
		default:
			printf("Reloc %li:Relocation type %i not supported\n", i, relocations[i]->rtype);
			error_count++;
		}
	}

	if (relocation_count > 0x10000)
	{
		printf("Too many relocations\n");
		exit(1);
	}

	header_buffer[0x06] = (UCHAR)(relocation_count & 0xff);
	header_buffer[0x07] = (UCHAR)(relocation_count >> 8);
	i = relocation_count * 4 + 0x4f;
	i >>= 4;
	totlength += i << 4;
	header_buffer[0x08] = (UCHAR)(i & 0xff);
	header_buffer[0x09] = (UCHAR)(i >> 8);
	i = totlength % 512;
	header_buffer[0x02] = (UCHAR)(i & 0xff);
	header_buffer[0x03] = (UCHAR)(i >> 8);
	i = (totlength + 0x1ff) >> 9;
	if (i > 0x10000)
	{
		printf("File too large\n");
		exit(1);
	}
	header_buffer[0x04] = (UCHAR)(i & 0xff);
	header_buffer[0x05] = (UCHAR)(i >> 8);

	if (error_count != 0)
	{
		exit(1);
	}

	outfile = fopen(out_name, "wb");
	if (!outfile)
	{
		printf("Error writing to file %s\n", out_name);
		exit(1);
	}

	i = (header_buffer[0x08] + (((UINT)header_buffer[0x09]) << 8)) * 16;
	if (fwrite(header_buffer, 1, i, outfile) != i)
	{
		printf("Error writing to file %s\n", out_name);
		exit(1);
	}

	started = 0;
	lastout = 0;

	for (i = 0; i < outcount; i++)
	{
		if (out_list[i] && ((out_list[i]->attributes & SEG_ALIGN) != SEG_ABS))
		{
			if (started > out_list[i]->base)
			{
				printf("Segment overlap\n");
				fclose(outfile);
				exit(1);
			}
			if (pad_segments)
			{
				while (started < out_list[i]->base)
				{
					fputc(0, outfile);
					started++;
				}
			}
			else
			{
				started = out_list[i]->base;
			}
			for (j = 0; j < out_list[i]->length; j++)
			{
				if (get_n_bit(out_list[i]->data_mask, j))
				{
					for (; lastout < started; lastout++)
					{
						fputc(0, outfile);
					}
					fputc(out_list[i]->data[j], outfile);
					lastout = started + 1;
				}
				else if (pad_segments)
				{
					fputc(0, outfile);
					lastout = started + 1;
				}
				started++;
			}
		}
	}

	if (lastout != started)
	{
		fseek(outfile, 0, SEEK_SET);
		lastout += (header_buffer[8] + (((UINT)header_buffer[9]) << 8)) << 4;
		i = lastout &0x1ff;
		header_buffer[0x02] = (UCHAR)(i & 0xff);
		header_buffer[0x03] = (UCHAR)(i >> 8);
		i = (lastout + 0x1ff) >> 9;
		header_buffer[0x04] = (UCHAR)(i & 0xff);
		header_buffer[0x05] = (UCHAR)(i >> 8);
		i = ((totlength - lastout) + 0xf) >> 4;
		if (i > 0x10000)
		{
			printf("Memory requirements too high\n");
		}
		header_buffer[0x0a] = (UCHAR)(i & 0xff);
		header_buffer[0x0b] = (UCHAR)(i >> 8);
		if (fwrite(header_buffer, 1, 12, outfile) != 12)
		{
			printf("Error writing to file\n");
			exit(1);
		}
	}
	fclose(outfile);
}

static long create_output_section(char* name, UINT winFlags)
{
	UINT j;

	out_list = (PPSEG)check_realloc(out_list, sizeof(PSEG) * (outcount + 1));
	out_list[outcount] = (PSEG)check_malloc(sizeof(SEG));
	segment_list = (PPSEG)check_realloc(segment_list, sizeof(PSEG) * (segcount + 1));
	segment_list = (PPSEG)check_realloc(segment_list, (segcount + 1) * sizeof(PSEG));
	segment_list[segcount] = out_list[outcount];
	name_list = (PPCHAR)check_realloc(name_list, (name_count + 1) * sizeof(PCHAR));
	name_list[name_count] = check_strdup(name);
	out_list[outcount]->name_index = name_count;
	out_list[outcount]->class_index = -1;
	out_list[outcount]->overlay_index = -1;
	name_count++;
	j = out_list[outcount - 1]->base + out_list[outcount - 1]->length;
	j += (object_align - 1);
	j &= (0xffffffff - (object_align - 1));
	out_list[outcount]->base = j;
	out_list[outcount]->length = 0;
	out_list[outcount]->data = out_list[outcount]->data_mask = NULL;
	out_list[outcount]->absolute_offset = segcount;
	out_list[outcount]->attributes = SEG_BYTE | SEG_PRIVATE;
	out_list[outcount]->win_flags = winFlags ^ WINF_NEG_FLAGS;
	segcount++;
	outcount++;
	return outcount - 1;
}

static void build_pe_imports(long impsectNum, PUCHAR objectTable)
{
	long i, j, k;
	UINT* reqimps = NULL, reqcount = 0;
	char** dllNames = NULL;
	int* dllNumImps = NULL;
	int* dllImpsDone = NULL;
	int* dllImpNameSize = NULL;
	UINT dllCount = 0, dllNameSize = 0, namePos;
	SEG* impsect;
	UINT thunkPos, thunk2Pos, impNamePos;

	if (impsectNum < 0) return;
	for (i = 0; i < extcount; i++)
	{
		if (extern_records[i].flags != EXT_MATCHEDIMPORT) continue;
		for (j = 0; j < reqcount; j++)
		{
			if (reqimps[j] == extern_records[i].import) break;
		}
		if (j != reqcount) continue;
		reqimps = (UINT*)check_realloc(reqimps, (reqcount + 1) * sizeof(UINT));
		reqimps[reqcount] = extern_records[i].import;
		reqcount++;
		for (j = 0; j < dllCount; j++)
		{
			if (!strcmp(import_definitions[extern_records[i].import].mod_name, dllNames[j])) break;
		}
		if (j == dllCount)
		{
			dllNames = (char**)check_realloc(dllNames, (dllCount + 1) * sizeof(char*));
			dllNumImps = (int*)check_realloc(dllNumImps, (dllCount + 1) * sizeof(int));
			dllImpsDone = (int*)check_realloc(dllImpsDone, (dllCount + 1) * sizeof(int));
			dllImpNameSize = (int*)check_realloc(dllImpNameSize, (dllCount + 1) * sizeof(int));
			dllNames[dllCount] = import_definitions[extern_records[i].import].mod_name;
			dllNumImps[dllCount] = 0;
			dllImpsDone[dllCount] = 0;
			dllImpNameSize[dllCount] = 0;
			dllNameSize += (UINT)strlen(dllNames[dllCount]) + 1;
			if (dllNameSize & 1) dllNameSize++;
			dllCount++;
		}
		dllNumImps[j]++;
		if (import_definitions[extern_records[i].import].flags == 0) /* import by name? */
		{
			dllImpNameSize[j] += (int)strlen(import_definitions[extern_records[i].import].imp_name) + 3;
			/* the +3 ensure room for 2-byte hint and null terminator */
			if (dllImpNameSize[j] & 1) dllImpNameSize[j]++;
		}
	}

	if (!reqcount || !dllCount) return;

	objectTable += PE_OBJECTENTRY_SIZE * impsectNum; /* point to import object entry */
	k = objectTable[-PE_OBJECTENTRY_SIZE + 20]; /* get physical start of prev object */
	k += objectTable[-PE_OBJECTENTRY_SIZE + 21] << 8;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 22] << 16;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 23] << 24;

	k += objectTable[-PE_OBJECTENTRY_SIZE + 16]; /* add physical length of prev object */
	k += objectTable[-PE_OBJECTENTRY_SIZE + 17] << 8;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 18] << 16;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 19] << 24;

	k += file_align - 1;
	k &= (0xffffffff - (file_align - 1)); /* aligned */

	/* k is now physical location of this object */

	objectTable[20] = (k) & 0xff; /* store physical file offset */
	objectTable[21] = (k >> 8) & 0xff;
	objectTable[22] = (k >> 16) & 0xff;
	objectTable[23] = (k >> 24) & 0xff;

	k = objectTable[-PE_OBJECTENTRY_SIZE + 12]; /* get virtual start of prev object */
	k += objectTable[-PE_OBJECTENTRY_SIZE + 13] << 8;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 14] << 16;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 15] << 24;

	k += objectTable[-PE_OBJECTENTRY_SIZE + 8]; /* add virtual length of prev object */
	k += objectTable[-PE_OBJECTENTRY_SIZE + 9] << 8;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 10] << 16;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 11] << 24;

	/* store base address (RVA) of section */
	objectTable[12] = (k) & 0xff;
	objectTable[13] = (k >> 8) & 0xff;
	objectTable[14] = (k >> 16) & 0xff;
	objectTable[15] = (k >> 24) & 0xff;

	impsect = out_list[impsectNum];
	impsect->base = k + image_base; /* get base address of section */

	impsect->length = (dllCount + 1) * PE_IMPORTDIRENTRY_SIZE + dllNameSize;
	if (impsect->length & 3) impsect->length += 4 - (impsect->length & 3); /* align to 4-byte boundary */
	thunkPos = impsect->base + impsect->length;
	for (j = 0, i = 0; j < dllCount; j++)
	{
		i += dllNumImps[j] + 1; /* add number of entries in DLL thunk table */
	}
	/* now i= number of entries in Thunk tables for all DLLs */
	/* get address of name tables, which follow thunk tables */
	impNamePos = thunkPos + i * 2 * 4; /* 2 thunk tables per DLL, 4 bytes per entry */
	impsect->length += i * 2 * 4; /* update length of section too. */

	for (j = 0, i = 0; j < dllCount; j++)
	{
		i += dllImpNameSize[j]; /* add size of import names and hints */
	}

	impsect->length += i;

	impsect->data = (PUCHAR)check_malloc(impsect->length);
	impsect->data_mask = (PUCHAR)check_malloc((impsect->length + 7) / 8);
	for (i = 0; i < (impsect->length + 7) / 8; i++)
	{
		impsect->data_mask[i] = 0xff;
	}

	/* end of directory entries=name table */
	namePos = impsect->base + (dllCount + 1) * PE_IMPORTDIRENTRY_SIZE;
	for (i = 0; i < dllCount; i++)
	{
		/* add directory entry */
		j = i * PE_IMPORTDIRENTRY_SIZE;
		impsect->data[j + 0] = (thunkPos - image_base) & 0xff; /* address of first thunk table */
		impsect->data[j + 1] = ((thunkPos - image_base) >> 8) & 0xff;
		impsect->data[j + 2] = ((thunkPos - image_base) >> 16) & 0xff;
		impsect->data[j + 3] = ((thunkPos - image_base) >> 24) & 0xff;
		impsect->data[j + 4] = 0;/* zero out time stamp */
		impsect->data[j + 5] = 0;
		impsect->data[j + 6] = 0;
		impsect->data[j + 7] = 0;
		impsect->data[j + 8] = 0;/* zero out version number */
		impsect->data[j + 9] = 0;
		impsect->data[j + 10] = 0;
		impsect->data[j + 11] = 0;
		impsect->data[j + 12] = (namePos - image_base) & 0xff; /* address of DLL name */
		impsect->data[j + 13] = ((namePos - image_base) >> 8) & 0xff;
		impsect->data[j + 14] = ((namePos - image_base) >> 16) & 0xff;
		impsect->data[j + 15] = ((namePos - image_base) >> 24) & 0xff;
		thunk2Pos = thunkPos + (dllNumImps[i] + 1) * 4; /* address of second thunk table */
		impsect->data[j + 16] = (thunk2Pos - image_base) & 0xff; /* store it */
		impsect->data[j + 17] = ((thunk2Pos - image_base) >> 8) & 0xff;
		impsect->data[j + 18] = ((thunk2Pos - image_base) >> 16) & 0xff;
		impsect->data[j + 19] = ((thunk2Pos - image_base) >> 24) & 0xff;
		/* add name to table */
		strcpy(impsect->data + namePos - impsect->base, dllNames[i]);
		namePos += (UINT)strlen(dllNames[i]) + 1;
		if (namePos & 1)
		{
			impsect->data[namePos - impsect->base] = 0;
			namePos++;
		}
		/* add imported names to table */
		for (k = 0; k < reqcount; k++)
		{
			if (strcmp(import_definitions[reqimps[k]].mod_name, dllNames[i]) != 0) continue;
			if (import_definitions[reqimps[k]].flags == 0)
			{
				/* store pointers to name entry in thunk tables */
				impsect->data[thunkPos - impsect->base] = (impNamePos - image_base) & 0xff;
				impsect->data[thunkPos - impsect->base + 1] = ((impNamePos - image_base) >> 8) & 0xff;
				impsect->data[thunkPos - impsect->base + 2] = ((impNamePos - image_base) >> 16) & 0xff;
				impsect->data[thunkPos - impsect->base + 3] = ((impNamePos - image_base) >> 24) & 0xff;

				impsect->data[thunk2Pos - impsect->base] = (impNamePos - image_base) & 0xff;
				impsect->data[thunk2Pos - impsect->base + 1] = ((impNamePos - image_base) >> 8) & 0xff;
				impsect->data[thunk2Pos - impsect->base + 2] = ((impNamePos - image_base) >> 16) & 0xff;
				impsect->data[thunk2Pos - impsect->base + 3] = ((impNamePos - image_base) >> 24) & 0xff;

				/* no hint */
				impsect->data[impNamePos - impsect->base] = 0;
				impsect->data[impNamePos - impsect->base + 1] = 0;
				impNamePos += 2;
				/* store name */
				strcpy(impsect->data + impNamePos - impsect->base,
					import_definitions[reqimps[k]].imp_name);
				impNamePos += (UINT)strlen(import_definitions[reqimps[k]].imp_name) + 1;
				if (impNamePos & 1)
				{
					impsect->data[impNamePos - impsect->base] = 0;
					impNamePos++;
				}
			}
			else
			{
				/* store ordinal number in thunk tables */
				j = import_definitions[reqimps[k]].ordinal + PE_ORDINAL_FLAG;
				impsect->data[thunkPos - impsect->base] = (j) & 0xff;
				impsect->data[thunkPos - impsect->base + 1] = (j >> 8) & 0xff;
				impsect->data[thunkPos - impsect->base + 2] = (j >> 16) & 0xff;
				impsect->data[thunkPos - impsect->base + 3] = (j >> 24) & 0xff;

				impsect->data[thunk2Pos - impsect->base] = (j) & 0xff;
				impsect->data[thunk2Pos - impsect->base + 1] = (j >> 8) & 0xff;
				impsect->data[thunk2Pos - impsect->base + 2] = (j >> 16) & 0xff;
				impsect->data[thunk2Pos - impsect->base + 3] = (j >> 24) & 0xff;
			}
			import_definitions[reqimps[k]].segment = impsect->absolute_offset;
			import_definitions[reqimps[k]].offset = thunk2Pos - impsect->base;
			thunkPos += 4;
			thunk2Pos += 4;
		}
		/* zero out end of thunk tables */
		impsect->data[thunkPos - impsect->base] = 0;
		impsect->data[thunkPos - impsect->base + 1] = 0;
		impsect->data[thunkPos - impsect->base + 2] = 0;
		impsect->data[thunkPos - impsect->base + 3] = 0;
		impsect->data[thunk2Pos - impsect->base] = 0;
		impsect->data[thunk2Pos - impsect->base + 1] = 0;
		impsect->data[thunk2Pos - impsect->base + 2] = 0;
		impsect->data[thunk2Pos - impsect->base + 3] = 0;
		thunkPos = thunk2Pos + 4;
	}
	/* zero out the final entry to mark the end of the table */
	j = i * PE_IMPORTDIRENTRY_SIZE;
	for (i = 0; i < PE_IMPORTDIRENTRY_SIZE; i++, j++)
	{
		impsect->data[j] = 0;
	}

	k = impsect->length;
	k += object_align - 1;
	k &= (0xffffffff - (object_align - 1));
	impsect->virtual_size = k;
	objectTable[8] = k & 0xff; /* store virtual size (in memory) of segment */
	objectTable[9] = (k >> 8) & 0xff;
	objectTable[10] = (k >> 16) & 0xff;
	objectTable[11] = (k >> 24) & 0xff;

	k = impsect->length;
	objectTable[16] = (k) & 0xff; /* store initialised data size */
	objectTable[17] = (k >> 8) & 0xff;
	objectTable[18] = (k >> 16) & 0xff;
	objectTable[19] = (k >> 16) & 0xff;

	return;
}

static void build_pe_relocs(long relocSectNum, PUCHAR object_table)
{
	int i, j;
	PRELOC r;
	PSEG relocate_section;
	UINT current_start_position;
	UINT current_block_position;
	UINT k;
	long targseg;
	UINT targofs;
	unsigned long templ;
	unsigned short temps;

	/* do fixups */
	for (i = 0; i < fixcount; i++)
	{
		get_fixup_target(out_name, relocations[i], &targseg, &targofs, TRUE);
		switch (relocations[i]->rtype)
		{
		case FIX_BASE:
		case FIX_PTR1616:
		case FIX_PTR1632:
			if (targseg < 0)
			{
				printf("Reloc %li:Segment selector relocations are not supported in PE files\n", i);
				printf("rtype=%02X, frame=%04X, target=%04X, ftype=%02X, ttype=%02X\n",
					relocations[i]->rtype, relocations[i]->frame, relocations[i]->target, relocations[i]->ftype,
					relocations[i]->ttype);

				error_count++;
			}
			else
			{
				j = relocations[i]->offset;
				if (relocations[i]->rtype == FIX_PTR1616)
				{
					if (targofs > 0xffff)
					{
						printf("Relocs %li:Warning 32 bit offset in 16 bit field\n", i);
					}
					targofs &= 0xffff;
					temps = segment_list[relocations[i]->segment]->data[j];
					temps += segment_list[relocations[i]->segment]->data[j + 1] << 8;
					temps += (unsigned short)targofs;
					segment_list[relocations[i]->segment]->data[j] = temps & 0xff;
					segment_list[relocations[i]->segment]->data[j + 1] = (temps >> 8) & 0xff;
					j += 2;
				}
				else if (relocations[i]->rtype == FIX_PTR1632)
				{
					templ = segment_list[relocations[i]->segment]->data[j];
					templ += segment_list[relocations[i]->segment]->data[j + 1] << 8;
					templ += segment_list[relocations[i]->segment]->data[j + 2] << 16;
					templ += segment_list[relocations[i]->segment]->data[j + 3] << 24;
					templ += targofs;
					segment_list[relocations[i]->segment]->data[j] = templ & 0xff;
					segment_list[relocations[i]->segment]->data[j + 1] = (templ >> 8) & 0xff;
					segment_list[relocations[i]->segment]->data[j + 2] = (templ >> 16) & 0xff;
					segment_list[relocations[i]->segment]->data[j + 3] = (templ >> 24) & 0xff;
					j += 4;
				}
				temps = segment_list[relocations[i]->segment]->data[j];
				temps += segment_list[relocations[i]->segment]->data[j + 1] << 8;
				temps += (unsigned short)segment_list[targseg]->absolute_frame;
				segment_list[relocations[i]->segment]->data[j] = temps & 0xff;
				segment_list[relocations[i]->segment]->data[j + 1] = (temps >> 8) & 0xff;
			}
			break;
		case FIX_OFS32:
		case FIX_OFS32_2:
			templ = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
			templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
			templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] << 16;
			templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] << 24;
			templ += targofs;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset] = templ & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (templ >> 8) & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] = (templ >> 16) & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] = (templ >> 24) & 0xff;
			break;
		case FIX_RVA32:
			templ = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
			templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
			templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] << 16;
			templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] << 24;
			templ += targofs - image_base;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset] = templ & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (templ >> 8) & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] = (templ >> 16) & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] = (templ >> 24) & 0xff;
			break;
		case FIX_OFS16:
		case FIX_OFS16_2:
			if (targofs > 0xffff)
			{
				printf("Relocs %li:Warning 32 bit offset in 16 bit field\n", i);
			}
			targofs &= 0xffff;
			temps = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
			temps += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
			temps += (unsigned short)targofs;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset] = temps & 0xff;
			segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (temps >> 8) & 0xff;
			break;
		case FIX_LBYTE:
		case FIX_HBYTE:
			printf("Error: Byte relocations not supported in PE files\n");
			error_count++;
			break;
		case FIX_SELF_LBYTE:
			if (targseg >= 0)
			{
				printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
				error_count++;
			}
			else
			{
				j = targofs;
				j -= (segment_list[relocations[i]->segment]->base + relocations[i]->offset + 1);
				if ((j < -128) || (j > 127))
				{
					printf("Error: Reloc %li out of range\n", i);
				}
				else
				{
					segment_list[relocations[i]->segment]->data[relocations[i]->offset] += j;
				}
			}
			break;
		case FIX_SELF_OFS16:
		case FIX_SELF_OFS16_2:
			if (targseg >= 0)
			{
				printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
				error_count++;
			}
			else
			{
				j = targofs;
				j -= (segment_list[relocations[i]->segment]->base + relocations[i]->offset + 2);
				if ((j < -32768) || (j > 32767))
				{
					printf("Error: Reloc %li out of range\n", i);
				}
				else
				{
					temps = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
					temps += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
					temps += j;
					segment_list[relocations[i]->segment]->data[relocations[i]->offset] = temps & 0xff;
					segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (temps >> 8) & 0xff;
				}
			}
			break;
		case FIX_SELF_OFS32:
		case FIX_SELF_OFS32_2:
			if (targseg >= 0)
			{
				printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
				error_count++;
			}
			else
			{
				j = targofs;
				j -= (segment_list[relocations[i]->segment]->base + relocations[i]->offset + 4);
				templ = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
				templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
				templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] << 16;
				templ += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] << 24;
				templ += j;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset] = templ & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (templ >> 8) & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] = (templ >> 16) & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] = (templ >> 24) & 0xff;
			}
			break;
		default:
			printf("Reloc %li:Relocation type %i not supported\n", i, relocations[i]->rtype);
			error_count++;
		}
	}

	/* get reloc section */
	relocate_section = out_list[relocSectNum]; /* get section structure */

	/* sort relocations into order of increasing address */
	for (i = 1; i < fixcount; i++)
	{
		r = relocations[i]; /* save current reloc */
		for (j = i - 1; j >= 0; j--) /* search backwards through table */
		{
			/* stop once we've found a target before current */
			if (r->output_pos >= relocations[j]->output_pos) break;
			/* otherwise move reloc entry up */
			relocations[j + 1] = relocations[j];
		}
		j++; /* point to first entry after non-match */
		relocations[j] = r; /* put current reloc in position */
	}

	for (i = 0, current_start_position = 0, current_block_position = 0; i < fixcount; i++)
	{
		switch (relocations[i]->rtype)
		{
		case FIX_SELF_OFS32:
		case FIX_SELF_OFS32_2:
		case FIX_SELF_OFS16:
		case FIX_SELF_OFS16_2:
		case FIX_SELF_LBYTE:
		case FIX_RVA32:
			continue; /* self-relative fixups and RVA fixups don't relocate */
		default:
			break;
		}
		if (relocations[i]->output_pos >= (current_start_position + 0x1000)) /* more than 4K past block start? */
		{
			j = relocate_section->length & 3;
			if (j) /* unaligned block position */
			{
				relocate_section->length += 4 - j; /* update length to align block */
				/* and block memory */
				relocate_section->data = (PUCHAR)check_realloc(relocate_section->data, relocate_section->length);
				/* update size of current reloc block */
				k = relocate_section->data[current_block_position + 4];
				k += relocate_section->data[current_block_position + 5] << 8;
				k += relocate_section->data[current_block_position + 6] << 16;
				k += relocate_section->data[current_block_position + 7] << 24;
				k += 4 - j;
				relocate_section->data[current_block_position + 4] = k & 0xff;
				relocate_section->data[current_block_position + 5] = (k >> 8) & 0xff;
				relocate_section->data[current_block_position + 6] = (k >> 16) & 0xff;
				relocate_section->data[current_block_position + 7] = (k >> 24) & 0xff;
				for (j = 4 - j; j > 0; j--)
				{
					relocate_section->data[relocate_section->length - j] = 0;
				}
			}
			current_block_position = relocate_section->length; /* get address in section of current block */
			relocate_section->length += 8; /* 8 bytes block header */
			/* increase size of block */
			relocate_section->data = (PUCHAR)check_realloc(relocate_section->data, relocate_section->length);
			/* store reloc base address, and block size */
			current_start_position = relocations[i]->output_pos & 0xfffff000; /* start of mem page */

			/* start pos is relative to image base */
			relocate_section->data[current_block_position] = (current_start_position - image_base) & 0xff;
			relocate_section->data[current_block_position + 1] = ((current_start_position - image_base) >> 8) & 0xff;
			relocate_section->data[current_block_position + 2] = ((current_start_position - image_base) >> 16) & 0xff;
			relocate_section->data[current_block_position + 3] = ((current_start_position - image_base) >> 24) & 0xff;

			relocate_section->data[current_block_position + 4] = 8; /* start size is 8 bytes */
			relocate_section->data[current_block_position + 5] = 0;
			relocate_section->data[current_block_position + 6] = 0;
			relocate_section->data[current_block_position + 7] = 0;
		}
		relocate_section->data = (PUCHAR)check_realloc(relocate_section->data, relocate_section->length + 2);

		j = relocations[i]->output_pos - current_start_position; /* low 12 bits of address */
		switch (relocations[i]->rtype)
		{
		case FIX_PTR1616:
		case FIX_OFS16:
		case FIX_OFS16_2:
			j |= PE_REL_LOW16;
			break;
		case FIX_PTR1632:
		case FIX_OFS32:
		case FIX_OFS32_2:
			j |= PE_REL_OFS32;
		}
		/* store relocation */
		relocate_section->data[relocate_section->length] = j & 0xff;
		relocate_section->data[relocate_section->length + 1] = (j >> 8) & 0xff;
		/* update block length */
		relocate_section->length += 2;
		/* update size of current reloc block */
		k = relocate_section->data[current_block_position + 4];
		k += relocate_section->data[current_block_position + 5] << 8;
		k += relocate_section->data[current_block_position + 6] << 16;
		k += relocate_section->data[current_block_position + 7] << 24;
		k += 2;
		relocate_section->data[current_block_position + 4] = k & 0xff;
		relocate_section->data[current_block_position + 5] = (k >> 8) & 0xff;
		relocate_section->data[current_block_position + 6] = (k >> 16) & 0xff;
		relocate_section->data[current_block_position + 7] = (k >> 24) & 0xff;
	}
	/* if no fixups, then build NOP fixups, to make Windows NT happy */
	/* when it trys to relocate image */
	if (relocate_section->length == 0)
	{
		/* 12 bytes for dummy section */
		relocate_section->length = 12;
		relocate_section->data = (PUCHAR)check_malloc(12);
		/* zero it out for now */
		for (i = 0; i < 12; i++) relocate_section->data[i] = 0;
		relocate_section->data[4] = 12; /* size of block */
	}

	relocate_section->data_mask = (PUCHAR)check_malloc((relocate_section->length + 7) / 8);
	for (i = 0; i < (relocate_section->length + 7) / 8; i++)
	{
		relocate_section->data_mask[i] = 0xff;
	}

	object_table += PE_OBJECTENTRY_SIZE * relocSectNum; /* point to reloc object entry */
	k = relocate_section->length;
	k += object_align - 1;
	k &= (0xffffffff - (object_align - 1));
	relocate_section->virtual_size = k;
	object_table[8] = k & 0xff; /* store virtual size (in memory) of segment */
	object_table[9] = (k >> 8) & 0xff;
	object_table[10] = (k >> 16) & 0xff;
	object_table[11] = (k >> 24) & 0xff;

	k = relocate_section->length;
	object_table[16] = (k) & 0xff; /* store initialised data size */
	object_table[17] = (k >> 8) & 0xff;
	object_table[18] = (k >> 16) & 0xff;
	object_table[19] = (k >> 16) & 0xff;

	k = object_table[-PE_OBJECTENTRY_SIZE + 20]; /* get physical start of prev object */
	k += object_table[-PE_OBJECTENTRY_SIZE + 21] << 8;
	k += object_table[-PE_OBJECTENTRY_SIZE + 22] << 16;
	k += object_table[-PE_OBJECTENTRY_SIZE + 23] << 24;

	k += object_table[-PE_OBJECTENTRY_SIZE + 16]; /* add physical length of prev object */
	k += object_table[-PE_OBJECTENTRY_SIZE + 17] << 8;
	k += object_table[-PE_OBJECTENTRY_SIZE + 18] << 16;
	k += object_table[-PE_OBJECTENTRY_SIZE + 19] << 24;

	k += file_align - 1;
	k &= (0xffffffff - (file_align - 1)); /* aligned */

	/* k is now physical location of this object */

	object_table[20] = (k) & 0xff; /* store physical file offset */
	object_table[21] = (k >> 8) & 0xff;
	object_table[22] = (k >> 16) & 0xff;
	object_table[23] = (k >> 24) & 0xff;

	k = object_table[-PE_OBJECTENTRY_SIZE + 12]; /* get virtual start of prev object */
	k += object_table[-PE_OBJECTENTRY_SIZE + 13] << 8;
	k += object_table[-PE_OBJECTENTRY_SIZE + 14] << 16;
	k += object_table[-PE_OBJECTENTRY_SIZE + 15] << 24;

	k += object_table[-PE_OBJECTENTRY_SIZE + 8]; /* add virtual length of prev object */
	k += object_table[-PE_OBJECTENTRY_SIZE + 9] << 8;
	k += object_table[-PE_OBJECTENTRY_SIZE + 10] << 16;
	k += object_table[-PE_OBJECTENTRY_SIZE + 11] << 24;

	/* store base address (RVA) of section */
	object_table[12] = (k) & 0xff;
	object_table[13] = (k >> 8) & 0xff;
	object_table[14] = (k >> 16) & 0xff;
	object_table[15] = (k >> 24) & 0xff;

	relocate_section->base = k + image_base; /* relocate section */

	return;
}

static void build_pe_exports(long SectNum, PUCHAR objectTable, PUCHAR name)
{
	long i, j;
	UINT k;
	PSEG expSect;
	UINT namelen;
	UINT numNames = 0;
	UINT RVAStart;
	UINT nameRVAStart;
	UINT ordinalStart;
	UINT nameSpaceStart;
	UINT minOrd;
	UINT maxOrd;
	UINT numOrds;
	PPEXPREC nameList;
	PEXPREC curName;

	if (!expcount || (SectNum < 0)) return; /* return if no exports */
	expSect = out_list[SectNum];

	if (name)
	{
		namelen = (UINT)strlen(name);
		/* search backwards for path separator */
		for (i = namelen - 1; (i >= 0) && (name[i] != PATH_CHAR); i--);
		if (i >= 0) /* if found path separator */
		{
			name += (i + 1); /* update name pointer past path */
			namelen -= (i + 1); /* and reduce length */
		}
	}
	else namelen = 0;

	expSect->length = PE_EXPORTHEADER_SIZE + 4 * expcount + namelen + 1;
	/* min section size= header size + num exports * pointer size */
	/* plus space for null-terminated name */

	minOrd = 0xffffffff; /* max ordinal num */
	maxOrd = 0;

	for (i = 0; i < expcount; i++)
	{
		/* check we've got an exported name */
		if (export_definitions[i].exp_name && strlen(export_definitions[i].exp_name))
		{
			/* four bytes for name pointer */
			/* two bytes for ordinal, 1 for null terminator */
			expSect->length += (UINT)strlen(export_definitions[i].exp_name) + 7;
			numNames++;
		}

		if (export_definitions[i].flags & EXP_ORD) /* ordinal? */
		{
			if (export_definitions[i].ordinal < minOrd) minOrd = export_definitions[i].ordinal;
			if (export_definitions[i].ordinal > maxOrd) maxOrd = export_definitions[i].ordinal;
		}
	}

	numOrds = expcount; /* by default, number of RVAs=number of exports */
	if (maxOrd >= minOrd) /* actually got some ordinal references? */
	{
		i = maxOrd - minOrd + 1; /* get number of ordinals */
		if (i > expcount) /* if bigger range than number of exports */
		{
			expSect->length += 4 * (i - expcount); /* up length */
			numOrds = i; /* get new num RVAs */
		}
	}
	else
	{
		minOrd = 1; /* if none defined, min is set to 1 */
	}

	expSect->data = (PUCHAR)check_malloc(expSect->length);

	objectTable += PE_OBJECTENTRY_SIZE * SectNum; /* point to reloc object entry */
	k = expSect->length;
	k += object_align - 1;
	k &= (0xffffffff - (object_align - 1));
	expSect->virtual_size = k;
	objectTable[8] = k & 0xff; /* store virtual size (in memory) of segment */
	objectTable[9] = (k >> 8) & 0xff;
	objectTable[10] = (k >> 16) & 0xff;
	objectTable[11] = (k >> 24) & 0xff;

	k = expSect->length;
	objectTable[16] = (k) & 0xff; /* store initialised data size */
	objectTable[17] = (k >> 8) & 0xff;
	objectTable[18] = (k >> 16) & 0xff;
	objectTable[19] = (k >> 16) & 0xff;

	k = objectTable[-PE_OBJECTENTRY_SIZE + 20]; /* get physical start of prev object */
	k += objectTable[-PE_OBJECTENTRY_SIZE + 21] << 8;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 22] << 16;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 23] << 24;

	k += objectTable[-PE_OBJECTENTRY_SIZE + 16]; /* add physical length of prev object */
	k += objectTable[-PE_OBJECTENTRY_SIZE + 17] << 8;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 18] << 16;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 19] << 24;

	k += file_align - 1;
	k &= (0xffffffff - (file_align - 1)); /* aligned */

	/* k is now physical location of this object */

	objectTable[20] = (k) & 0xff; /* store physical file offset */
	objectTable[21] = (k >> 8) & 0xff;
	objectTable[22] = (k >> 16) & 0xff;
	objectTable[23] = (k >> 24) & 0xff;

	k = objectTable[-PE_OBJECTENTRY_SIZE + 12]; /* get virtual start of prev object */
	k += objectTable[-PE_OBJECTENTRY_SIZE + 13] << 8;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 14] << 16;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 15] << 24;

	k += objectTable[-PE_OBJECTENTRY_SIZE + 8]; /* add virtual length of prev object */
	k += objectTable[-PE_OBJECTENTRY_SIZE + 9] << 8;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 10] << 16;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 11] << 24;

	/* store base address (RVA) of section */
	objectTable[12] = (k) & 0xff;
	objectTable[13] = (k >> 8) & 0xff;
	objectTable[14] = (k >> 16) & 0xff;
	objectTable[15] = (k >> 24) & 0xff;

	expSect->base = k + image_base; /* relocate section */

	/* start with buf=all zero */
	for (i = 0; i < expSect->length; i++) expSect->data[i] = 0;

	/* store creation time of export data */
	k = (UINT)time(NULL);
	expSect->data[4] = k & 0xff;
	expSect->data[5] = (k >> 8) & 0xff;
	expSect->data[6] = (k >> 16) & 0xff;
	expSect->data[7] = (k >> 24) & 0xff;

	expSect->data[16] = (minOrd) & 0xff; /* store ordinal base */
	expSect->data[17] = (minOrd >> 8) & 0xff;
	expSect->data[18] = (minOrd >> 16) & 0xff;
	expSect->data[19] = (minOrd >> 24) & 0xff;

	/* store number of RVAs */
	expSect->data[20] = numOrds & 0xff;
	expSect->data[21] = (numOrds >> 8) & 0xff;
	expSect->data[22] = (numOrds >> 16) & 0xff;
	expSect->data[23] = (numOrds >> 24) & 0xff;

	RVAStart = PE_EXPORTHEADER_SIZE; /* start address of RVA table */
	nameRVAStart = RVAStart + numOrds * 4; /* start of name table entries */
	ordinalStart = nameRVAStart + numNames * 4; /* start of associated ordinal entries */
	nameSpaceStart = ordinalStart + numNames * 2; /* start of actual names */

	/* store number of named exports */
	expSect->data[24] = numNames & 0xff;
	expSect->data[25] = (numNames >> 8) & 0xff;
	expSect->data[26] = (numNames >> 16) & 0xff;
	expSect->data[27] = (numNames >> 24) & 0xff;

	/* store address of address table */
	expSect->data[28] = (RVAStart + expSect->base - image_base) & 0xff;
	expSect->data[29] = ((RVAStart + expSect->base - image_base) >> 8) & 0xff;
	expSect->data[30] = ((RVAStart + expSect->base - image_base) >> 16) & 0xff;
	expSect->data[31] = ((RVAStart + expSect->base - image_base) >> 24) & 0xff;

	/* store address of name table */
	expSect->data[32] = (nameRVAStart + expSect->base - image_base) & 0xff;
	expSect->data[33] = ((nameRVAStart + expSect->base - image_base) >> 8) & 0xff;
	expSect->data[34] = ((nameRVAStart + expSect->base - image_base) >> 16) & 0xff;
	expSect->data[35] = ((nameRVAStart + expSect->base - image_base) >> 24) & 0xff;

	/* store address of ordinal table */
	expSect->data[36] = (ordinalStart + expSect->base - image_base) & 0xff;
	expSect->data[37] = ((ordinalStart + expSect->base - image_base) >> 8) & 0xff;
	expSect->data[38] = ((ordinalStart + expSect->base - image_base) >> 16) & 0xff;
	expSect->data[39] = ((ordinalStart + expSect->base - image_base) >> 24) & 0xff;

	/* process numbered exports */
	for (i = 0; i < expcount; i++)
	{
		if (export_definitions[i].flags & EXP_ORD)
		{
			/* get current RVA */
			k = expSect->data[RVAStart + 4 * (export_definitions[i].ordinal - minOrd)];
			k += expSect->data[RVAStart + 4 * (export_definitions[i].ordinal - minOrd) + 1] << 8;
			k += expSect->data[RVAStart + 4 * (export_definitions[i].ordinal - minOrd) + 2] << 16;
			k += expSect->data[RVAStart + 4 * (export_definitions[i].ordinal - minOrd) + 3] << 24;
			if (k) /* error if already used */
			{
				printf("Duplicate export ordinal %i\n", export_definitions[i].ordinal);
				exit(1);
			}
			/* get RVA of export entry */
			k = export_definitions[i].pubdef->offset +
				segment_list[export_definitions[i].pubdef->segment]->base -
				image_base;
			/* store it */
			expSect->data[RVAStart + 4 * (export_definitions[i].ordinal - minOrd)] = k & 0xff;
			expSect->data[RVAStart + 4 * (export_definitions[i].ordinal - minOrd) + 1] = (k >> 8) & 0xff;
			expSect->data[RVAStart + 4 * (export_definitions[i].ordinal - minOrd) + 2] = (k >> 16) & 0xff;
			expSect->data[RVAStart + 4 * (export_definitions[i].ordinal - minOrd) + 3] = (k >> 24) & 0xff;
		}
	}

	/* process non-numbered exports */
	for (i = 0, j = RVAStart; i < expcount; i++)
	{
		if (!(export_definitions[i].flags & EXP_ORD))
		{
			do
			{
				k = expSect->data[j];
				k += expSect->data[j + 1] << 8;
				k += expSect->data[j + 2] << 16;
				k += expSect->data[j + 3] << 24;
				if (k) j += 4;
			} while (k); /* move through table until we find a free spot */
			/* get RVA of export entry */
			k = export_definitions[i].pubdef->offset;
			k += segment_list[export_definitions[i].pubdef->segment]->base;
			k -= image_base;
			/* store RVA */
			expSect->data[j] = k & 0xff;
			expSect->data[j + 1] = (k >> 8) & 0xff;
			expSect->data[j + 2] = (k >> 16) & 0xff;
			expSect->data[j + 3] = (k >> 24) & 0xff;
			export_definitions[i].ordinal = (j - RVAStart) / 4 + minOrd; /* store ordinal */
			j += 4;
		}
	}

	if (numNames) /* sort name table if present */
	{
		nameList = (PPEXPREC)check_malloc(numNames * sizeof(PEXPREC));
		j = 0; /* no entries yet */
		for (i = 0; i < expcount; i++)
		{
			if (export_definitions[i].exp_name && export_definitions[i].exp_name[0])
			{
				/* make entry in name list */
				nameList[j] = &export_definitions[i];
				j++;
			}
		}
		/* sort them into order */
		for (i = 1; i < numNames; i++)
		{
			curName = nameList[i];
			for (j = i - 1; j >= 0; j--)
			{
				/* break out if we're above previous entry */
				if (strcmp(curName->exp_name, nameList[j]->exp_name) >= 0)
				{
					break;
				}
				/* else move entry up */
				nameList[j + 1] = nameList[j];
			}
			j++; /* move to one after better entry */
			nameList[j] = curName; /* insert current entry into position */
		}
		/* and store */
		for (i = 0; i < numNames; i++)
		{
			/* store ordinal */
			expSect->data[ordinalStart] = (nameList[i]->ordinal - minOrd) & 0xff;
			expSect->data[ordinalStart + 1] = ((nameList[i]->ordinal - minOrd) >> 8) & 0xff;
			ordinalStart += 2;
			/* store name RVA */
			expSect->data[nameRVAStart] = (nameSpaceStart + expSect->base - image_base) & 0xff;
			expSect->data[nameRVAStart + 1] = ((nameSpaceStart + expSect->base - image_base) >> 8) & 0xff;
			expSect->data[nameRVAStart + 2] = ((nameSpaceStart + expSect->base - image_base) >> 16) & 0xff;
			expSect->data[nameRVAStart + 3] = ((nameSpaceStart + expSect->base - image_base) >> 24) & 0xff;
			nameRVAStart += 4;
			/* store name */
			for (j = 0; nameList[i]->exp_name[j]; j++, nameSpaceStart++)
			{
				expSect->data[nameSpaceStart] = nameList[i]->exp_name[j];
			}
			/* store NULL */
			expSect->data[nameSpaceStart] = 0;
			nameSpaceStart++;
		}
	}

	/* store library name */
	for (j = 0; j < namelen; j++)
	{
		expSect->data[nameSpaceStart + j] = name[j];
	}
	if (namelen)
	{
		expSect->data[nameSpaceStart + j] = 0;
		/* store name RVA */
		expSect->data[12] = (nameSpaceStart + expSect->base - image_base) & 0xff;
		expSect->data[13] = ((nameSpaceStart + expSect->base - image_base) >> 8) & 0xff;
		expSect->data[14] = ((nameSpaceStart + expSect->base - image_base) >> 16) & 0xff;
		expSect->data[15] = ((nameSpaceStart + expSect->base - image_base) >> 24) & 0xff;
	}

	expSect->data_mask = (PUCHAR)check_malloc((expSect->length + 7) / 8);
	for (i = 0; i < (expSect->length + 7) / 8; i++)
	{
		expSect->data_mask[i] = 0xff;
	}

	return;
}

static void build_pe_resources(long sectNum, PUCHAR objectTable)
{
	long i, j;
	UINT k;
	SEG* ressect;
	RESOURCE curres;
	int numtypes, numnamedtypes;
	int numPairs, numnames, numids;
	UINT nameSize, dataSize;
	UINT tableSize, dataListSize;
	UINT namePos, dataPos, tablePos, dataListPos;
	UINT curTypePos, curNamePos = 0, curLangPos = 0;
	char* curTypeName, * curName;
	int curTypeId, curId;

	if ((sectNum < 0) || !rescount) return;

	objectTable += PE_OBJECTENTRY_SIZE * sectNum; /* point to import object entry */
	k = objectTable[-PE_OBJECTENTRY_SIZE + 20]; /* get physical start of prev object */
	k += objectTable[-PE_OBJECTENTRY_SIZE + 21] << 8;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 22] << 16;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 23] << 24;

	k += objectTable[-PE_OBJECTENTRY_SIZE + 16]; /* add physical length of prev object */
	k += objectTable[-PE_OBJECTENTRY_SIZE + 17] << 8;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 18] << 16;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 19] << 24;

	k += file_align - 1;
	k &= (0xffffffff - (file_align - 1)); /* aligned */

	/* k is now physical location of this object */

	objectTable[20] = (k) & 0xff; /* store physical file offset */
	objectTable[21] = (k >> 8) & 0xff;
	objectTable[22] = (k >> 16) & 0xff;
	objectTable[23] = (k >> 24) & 0xff;

	k = objectTable[-PE_OBJECTENTRY_SIZE + 12]; /* get virtual start of prev object */
	k += objectTable[-PE_OBJECTENTRY_SIZE + 13] << 8;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 14] << 16;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 15] << 24;

	k += objectTable[-PE_OBJECTENTRY_SIZE + 8]; /* add virtual length of prev object */
	k += objectTable[-PE_OBJECTENTRY_SIZE + 9] << 8;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 10] << 16;
	k += objectTable[-PE_OBJECTENTRY_SIZE + 11] << 24;

	/* store base address (RVA) of section */
	objectTable[12] = (k) & 0xff;
	objectTable[13] = (k >> 8) & 0xff;
	objectTable[14] = (k >> 16) & 0xff;
	objectTable[15] = (k >> 24) & 0xff;

	ressect = out_list[sectNum];
	ressect->base = k; /* get base RVA of section */

	/* sort into type-id order */
	for (i = 1; i < rescount; i++)
	{
		curres = resource[i];
		for (j = i - 1; j >= 0; j--)
		{
			if (resource[j].typename)
			{
				if (!curres.typename) break;
				if (wstricmp(curres.typename, resource[j].typename) > 0) break;
				if (wstricmp(curres.typename, resource[j].typename) == 0)
				{
					if (resource[j].name)
					{
						if (!curres.name) break;
						if (wstricmp(curres.name, resource[j].name) > 0) break;
						if (wstricmp(curres.name, resource[j].name) == 0)
						{
							if (resource[j].language_id > curres.language_id)
								break;
							if (resource[j].language_id == curres.language_id)
							{
								printf("Error duplicate resource ID\n");
								exit(1);
							}
						}
					}
					else
					{
						if (!curres.name)
						{
							if (curres.id > resource[j].id) break;
							if (curres.id == resource[j].id)
							{
								if (resource[j].language_id > curres.language_id)
									break;
								if (resource[j].language_id == curres.language_id)
								{
									printf("Error duplicate resource ID\n");
									exit(1);
								}
							}
						}
					}
				}
			}
			else
			{
				if (!curres.typename)
				{
					if (curres.typeid > resource[j].typeid) break;
					if (curres.typeid == resource[j].typeid)
					{
						if (resource[j].name)
						{
							if (!curres.name) break;
							if (wstricmp(curres.name, resource[j].name) > 0) break;
							if (wstricmp(curres.name, resource[j].name) == 0)
							{
								if (resource[j].language_id > curres.language_id)
									break;
								if (resource[j].language_id == curres.language_id)
								{
									printf("Error duplicate resource ID\n");
									exit(1);
								}
							}
						}
						else
						{
							if (!curres.name)
							{
								if (curres.id > resource[j].id) break;
								if (curres.id == resource[j].id)
								{
									if (resource[j].language_id > curres.language_id)
										break;
									if (resource[j].language_id == curres.language_id)
									{
										printf("Error duplicate resource ID\n");
										exit(1);
									}
								}
							}
						}
					}
				}
			}
			resource[j + 1] = resource[j];
		}
		j++;
		resource[j] = curres;
	}

	nameSize = 0;
	dataSize = 0;
	for (i = 0; i < rescount; i++)
	{
		if (resource[i].typename)
		{
			nameSize += 2 + 2 * wstrlen(resource[i].typename);
		}
		if (resource[i].name)
		{
			nameSize += 2 + 2 * wstrlen(resource[i].name);
		}
		dataSize += resource[i].length + 3;
		dataSize &= 0xfffffffc;
	}

	/* count named types */
	numnamedtypes = 0;
	numPairs = rescount;
	for (i = 0; i < rescount; i++)
	{
		if (!resource[i].typename) break;
		if ((i > 0) && !wstricmp(resource[i].typename, resource[i - 1].typename))
		{
			if (resource[i].name)
			{
				if (wstricmp(resource[i].name, resource[i - 1].name) == 0)
					numPairs--;
			}
			else
			{
				if (!resource[i - 1].name && (resource[i].id == resource[i - 1].id))
					numPairs--;
			}
			continue;
		}
		numnamedtypes++;
	}
	numtypes = numnamedtypes;
	for (; i < rescount; i++)
	{
		if ((i > 0) && !resource[i - 1].typename && (resource[i].typeid == resource[i - 1].typeid))
		{
			if (resource[i].name)
			{
				if (wstricmp(resource[i].name, resource[i - 1].name) == 0)
					numPairs--;
			}
			else
			{
				if (!resource[i - 1].name && (resource[i].id == resource[i - 1].id))
					numPairs--;
			}
			continue;
		}
		numtypes++;
	}

	tableSize = (rescount + numtypes + numPairs) * PE_RESENTRY_SIZE +
		(numtypes + numPairs + 1) * PE_RESDIR_SIZE;
	dataListSize = rescount * PE_RESDATAENTRY_SIZE;

	tablePos = 0;
	dataListPos = tableSize;
	namePos = tableSize + dataListSize;
	dataPos = tableSize + nameSize + dataListSize + 3;
	dataPos &= 0xfffffffc;

	ressect->length = dataPos + dataSize;

	ressect->data = (PUCHAR)check_malloc(ressect->length);

	ressect->data_mask = (PUCHAR)check_malloc((ressect->length + 7) / 8);

	/* empty section to start with */
	for (i = 0; i < ressect->length; i++)
		ressect->data[i] = 0;

	/* build master directory */
	/* store time/date of creation */
	k = (UINT)time(NULL);
	ressect->data[4] = k & 0xff;
	ressect->data[5] = (k >> 8) & 0xff;
	ressect->data[6] = (k >> 16) & 0xff;
	ressect->data[7] = (k >> 24) & 0xff;

	ressect->data[12] = numnamedtypes & 0xff;
	ressect->data[13] = (numnamedtypes >> 8) & 0xff;
	ressect->data[14] = (numtypes - numnamedtypes) & 0xff;
	ressect->data[15] = ((numtypes - numnamedtypes) >> 8) & 0xff;

	tablePos = 16 + numtypes * PE_RESENTRY_SIZE;
	curTypePos = 16;
	curTypeName = NULL;
	curTypeId = -1;
	for (i = 0; i < rescount; i++)
	{
		if (!(resource[i].typename && curTypeName &&
			!wstricmp(resource[i].typename, curTypeName))
			&& !(!resource[i].typename && !curTypeName && curTypeId == resource[i].typeid))
		{
			if (resource[i].typename)
			{
				ressect->data[curTypePos] = (namePos) & 0xff;
				ressect->data[curTypePos + 1] = ((namePos) >> 8) & 0xff;
				ressect->data[curTypePos + 2] = ((namePos) >> 16) & 0xff;
				ressect->data[curTypePos + 3] = (((namePos) >> 24) & 0xff) | 0x80;
				curTypeName = resource[i].typename;
				k = wstrlen(curTypeName);
				ressect->data[namePos] = k & 0xff;
				ressect->data[namePos + 1] = (k >> 8) & 0xff;
				namePos += 2;
				memcpy(ressect->data + namePos, curTypeName, k * 2);
				namePos += k * 2;
				curTypeId = -1;
			}
			else
			{
				curTypeName = NULL;
				curTypeId = resource[i].typeid;
				ressect->data[curTypePos] = curTypeId & 0xff;
				ressect->data[curTypePos + 1] = (curTypeId >> 8) & 0xff;
				ressect->data[curTypePos + 2] = (curTypeId >> 16) & 0xff;
				ressect->data[curTypePos + 3] = (curTypeId >> 24) & 0xff;
			}
			ressect->data[curTypePos + 4] = (tablePos) & 0xff;
			ressect->data[curTypePos + 5] = ((tablePos) >> 8) & 0xff;
			ressect->data[curTypePos + 6] = ((tablePos) >> 16) & 0xff;
			ressect->data[curTypePos + 7] = (((tablePos) >> 24) & 0x7f) | 0x80;

			numnames = numids = 0;
			for (j = i; j < rescount; j++)
			{
				if (resource[i].typename)
				{
					if (!resource[j].typename) break;
					if (wstricmp(resource[i].typename, resource[j].typename) != 0) break;
				}
				else
				{
					if (resource[j].typeid != resource[i].typeid) break;
				}
				if (resource[j].name)
				{
					if (((j > i) && (wstricmp(resource[j].name, resource[j - 1].name) != 0))
						|| (j == i))
						numnames++;
				}
				else
				{
					if (((j > i) && (resource[j - 1].name || (resource[j].id != resource[j - 1].id)))
						|| (j == i))
						numids++;
				}
			}
			/* store time/date of creation */
			k = (UINT)time(NULL);
			ressect->data[tablePos + 4] = k & 0xff;
			ressect->data[tablePos + 5] = (k >> 8) & 0xff;
			ressect->data[tablePos + 6] = (k >> 16) & 0xff;
			ressect->data[tablePos + 7] = (k >> 24) & 0xff;

			ressect->data[tablePos + 12] = numnames & 0xff;
			ressect->data[tablePos + 13] = (numnames >> 8) & 0xff;
			ressect->data[tablePos + 14] = numids & 0xff;
			ressect->data[tablePos + 15] = (numids >> 8) & 0xff;

			curNamePos = tablePos + PE_RESDIR_SIZE;
			curName = NULL;
			curId = -1;
			tablePos += PE_RESDIR_SIZE + (numids + numnames) * PE_RESENTRY_SIZE;
			curTypePos += PE_RESENTRY_SIZE;
		}
		if (!(resource[i].name && curName &&
			!wstricmp(resource[i].name, curName))
			&& !(!resource[i].name && !curName && curId == resource[i].id))
		{
			if (resource[i].name)
			{
				ressect->data[curNamePos] = (namePos) & 0xff;
				ressect->data[curNamePos + 1] = ((namePos) >> 8) & 0xff;
				ressect->data[curNamePos + 2] = ((namePos) >> 16) & 0xff;
				ressect->data[curNamePos + 3] = (((namePos) >> 24) & 0xff) | 0x80;
				curName = resource[i].name;
				k = wstrlen(curName);
				ressect->data[namePos] = k & 0xff;
				ressect->data[namePos + 1] = (k >> 8) & 0xff;
				namePos += 2;
				memcpy(ressect->data + namePos, curName, k * 2);
				namePos += k * 2;
				curId = -1;
			}
			else
			{
				curName = NULL;
				curId = resource[i].id;
				ressect->data[curNamePos] = curId & 0xff;
				ressect->data[curNamePos + 1] = (curId >> 8) & 0xff;
				ressect->data[curNamePos + 2] = (curId >> 16) & 0xff;
				ressect->data[curNamePos + 3] = (curId >> 24) & 0xff;
			}
			ressect->data[curNamePos + 4] = (tablePos) & 0xff;
			ressect->data[curNamePos + 5] = ((tablePos) >> 8) & 0xff;
			ressect->data[curNamePos + 6] = ((tablePos) >> 16) & 0xff;
			ressect->data[curNamePos + 7] = (((tablePos) >> 24) & 0x7f) | 0x80;

			numids = 0;
			for (j = i; j < rescount; j++)
			{
				if (resource[i].typename)
				{
					if (!resource[j].typename) break;
					if (wstricmp(resource[i].typename, resource[j].typename) != 0) break;
				}
				else
				{
					if (resource[j].typeid != resource[i].typeid) break;
				}
				if (resource[i].name)
				{
					if (!resource[j].name) break;
					if (wstricmp(resource[j].name, resource[i].name) != 0) break;
				}
				else
				{
					if (resource[j].id != resource[i].id) break;
				}
				numids++;
			}
			numnames = 0; /* no names for languages */
			/* store time/date of creation */
			k = (UINT)time(NULL);
			ressect->data[tablePos + 4] = k & 0xff;
			ressect->data[tablePos + 5] = (k >> 8) & 0xff;
			ressect->data[tablePos + 6] = (k >> 16) & 0xff;
			ressect->data[tablePos + 7] = (k >> 24) & 0xff;

			ressect->data[tablePos + 12] = numnames & 0xff;
			ressect->data[tablePos + 13] = (numnames >> 8) & 0xff;
			ressect->data[tablePos + 14] = numids & 0xff;
			ressect->data[tablePos + 15] = (numids >> 8) & 0xff;

			curLangPos = tablePos + PE_RESDIR_SIZE;
			tablePos += PE_RESDIR_SIZE + numids * PE_RESENTRY_SIZE;
			curNamePos += PE_RESENTRY_SIZE;
		}
		ressect->data[curLangPos] = resource[i].language_id & 0xff;
		ressect->data[curLangPos + 1] = (resource[i].language_id >> 8) & 0xff;
		ressect->data[curLangPos + 2] = (resource[i].language_id >> 16) & 0xff;
		ressect->data[curLangPos + 3] = (resource[i].language_id >> 24) & 0xff;

		ressect->data[curLangPos + 4] = (dataListPos) & 0xff;
		ressect->data[curLangPos + 5] = ((dataListPos) >> 8) & 0xff;
		ressect->data[curLangPos + 6] = ((dataListPos) >> 16) & 0xff;
		ressect->data[curLangPos + 7] = (((dataListPos) >> 24) & 0x7f);
		curLangPos += PE_RESENTRY_SIZE;


		ressect->data[dataListPos] = (dataPos + ressect->base) & 0xff;
		ressect->data[dataListPos + 1] = ((dataPos + ressect->base) >> 8) & 0xff;
		ressect->data[dataListPos + 2] = ((dataPos + ressect->base) >> 16) & 0xff;
		ressect->data[dataListPos + 3] = ((dataPos + ressect->base) >> 24) & 0xff;
		ressect->data[dataListPos + 4] = resource[i].length & 0xff;
		ressect->data[dataListPos + 5] = (resource[i].length >> 8) & 0xff;
		ressect->data[dataListPos + 6] = (resource[i].length >> 16) & 0xff;
		ressect->data[dataListPos + 7] = (resource[i].length >> 24) & 0xff;
		memcpy(ressect->data + dataPos, resource[i].data, resource[i].length);
		dataPos += resource[i].length + 3;
		dataPos &= 0xfffffffc;
		dataListPos += PE_RESDATAENTRY_SIZE;
	}

	/* mark whole section as required output */
	for (i = 0; i < (ressect->length + 7) / 8; i++)
		ressect->data_mask[i] = 0xff;

	/* update object table */
	ressect->base += image_base;

	k = ressect->length;
	k += object_align - 1;
	k &= (0xffffffff - (object_align - 1));
	ressect->virtual_size = k;
	objectTable[8] = k & 0xff; /* store virtual size (in memory) of segment */
	objectTable[9] = (k >> 8) & 0xff;
	objectTable[10] = (k >> 16) & 0xff;
	objectTable[11] = (k >> 24) & 0xff;

	k = ressect->length;
	objectTable[16] = (k) & 0xff; /* store initialised data size */
	objectTable[17] = (k >> 8) & 0xff;
	objectTable[18] = (k >> 16) & 0xff;
	objectTable[19] = (k >> 16) & 0xff;

	return;
}

static void get_stub(PUCHAR* pstubData, UINT* pstubSize)
{
	FILE* f;
	unsigned char headbuf[0x1c];
	PUCHAR buf;
	UINT imageSize;
	UINT headerSize;
	UINT relocSize;
	UINT relocStart;
	int i;

	if (stub_name)
	{
		f = fopen(stub_name, "rb");
		if (!f)
		{
			printf("Unable to open stub file %s\n", stub_name);
			exit(1);
		}
		if (fread(headbuf, 1, 0x1c, f) != 0x1c) /* try and read 0x1c bytes */
		{
			printf("Error reading from file %s\n", stub_name);
			exit(1);
		}
		if ((headbuf[0] != 0x4d) || (headbuf[1] != 0x5a))
		{
			printf("Stub not valid EXE file\n");
			exit(1);
		}
		/* get size of image */
		imageSize = headbuf[2] + (headbuf[3] << 8) + ((headbuf[4] + (headbuf[5] << 8)) << 9);
		if (imageSize % 512) imageSize -= 512;
		headerSize = (headbuf[8] + (headbuf[9] << 8)) << 4;
		relocSize = (headbuf[6] + (headbuf[7] << 8)) << 2;
		imageSize -= headerSize;
		printf("imageSize=%i\n", imageSize);
		printf("header=%i\n", headerSize);
		printf("reloc=%i\n", relocSize);

		/* allocate buffer for load image */
		buf = (PUCHAR)check_malloc(imageSize + 0x40 + ((relocSize + 0xf) & 0xfffffff0));
		/* copy header */
		for (i = 0; i < 0x1c; i++) buf[i] = headbuf[i];

		relocStart = headbuf[0x18] + (headbuf[0x19] << 8);
		/* load relocs */
		fseek(f, relocStart, SEEK_SET);
		if (fread(buf + 0x40, 1, relocSize, f) != relocSize)
		{
			printf("Error reading from file %s\n", stub_name);
			exit(1);
		}

		/* paragraph align reloc size */
		relocSize += 0xf;
		relocSize &= 0xfffffff0;

		/* move to start of data */
		fseek(f, headerSize, SEEK_SET);
		/* new header is 4 paragraphs long + relocSize*/
		relocSize >>= 4;
		relocSize += 4;
		buf[8] = relocSize & 0xff;
		buf[9] = (relocSize >> 8) & 0xff;
		headerSize = relocSize << 4;
		/* load data into correct position */
		if (fread(buf + headerSize, 1, imageSize, f) != imageSize)
		{
			printf("Error reading from file %s\n", stub_name);
			exit(1);
		}
		/* relocations start at 0x40 */
		buf[0x18] = 0x40;
		buf[0x19] = 0;
		imageSize += headerSize; /* total file size */
		/* store pointer and size */
		(*pstubData) = buf;
		(*pstubSize) = imageSize;
		i = imageSize % 512; /* size mod 512 */
		imageSize = (imageSize + 511) >> 9; /* number of 512-byte pages */
		buf[2] = i & 0xff;
		buf[3] = (i >> 8) & 0xff;
		buf[4] = imageSize & 0xff;
		buf[5] = (imageSize >> 8) & 0xff;
	}
	else
	{
		(*pstubData) = default_stub;
		(*pstubSize) = defaultStubSize;
	}
}

void output_win32_file(PCHAR outname)
{
	long i, j, k;
	UINT started;
	UINT lastout;
	PUCHAR headbuf;
	PUCHAR stubData;
	FILE* outfile;
	UINT headerSize;
	UINT headerVirtSize;
	UINT stubSize;
	long nameIndex;
	UINT sectionStart;
	UINT headerStart;
	long relocSectNum, importSectNum, exportSectNum, resourceSectNum;
	UINT codeBase = 0;
	UINT dataBase = 0;
	UINT codeSize = 0;
	UINT dataSize = 0;

	printf("Generating PE file %s\n", outname);

	error_count = 0;

	/* allocate section entries for imports, exports and relocs if required */
	if (impsreq)
	{
		importSectNum = create_output_section("imports",
			WINF_INITDATA | WINF_SHARED | WINF_READABLE);
	}
	else
	{
		importSectNum = -1;
	}

	if (expcount)
	{
		exportSectNum = create_output_section("exports",
			WINF_INITDATA | WINF_SHARED | WINF_READABLE);
	}
	else
	{
		exportSectNum = -1;
	}

	/* Windows NT requires a reloc section to relocate image files, even */
	/* if it contains no actual fixups */
	relocSectNum = create_output_section("relocs",
		WINF_INITDATA | WINF_SHARED | WINF_DISCARDABLE | WINF_READABLE);

	if (rescount)
	{
		resourceSectNum = create_output_section("resource",
			WINF_INITDATA | WINF_SHARED | WINF_READABLE);
	}
	else
	{
		resourceSectNum = -1;
	}

	/* build header */
	get_stub(&stubData, &stubSize);

	headerStart = stubSize; /* get start of PE header */
	headerStart += 7;
	headerStart &= 0xfffffff8; /* align PE header to 8 byte boundary */

	headerSize = PE_HEADBUF_SIZE + outcount * PE_OBJECTENTRY_SIZE + stubSize;
	headerVirtSize = headerSize + (object_align - 1);
	headerVirtSize &= (0xffffffff - (object_align - 1));
	headerSize += (file_align - 1);
	headerSize &= (0xffffffff - (file_align - 1));


	headbuf = check_malloc(headerSize);

	for (i = 0; i < headerSize; i++)
	{
		headbuf[i] = 0;
	}

	for (i = 0; i < stubSize; i++) /* copy stub file */
		headbuf[i] = stubData[i];

	headbuf[0x3c] = headerStart & 0xff;         /* store pointer to PE header */
	headbuf[0x3d] = (headerStart >> 8) & 0xff;
	headbuf[0x3e] = (headerStart >> 16) & 0xff;
	headbuf[0x3f] = (headerStart >> 24) & 0xff;

	headbuf[headerStart + PE_SIGNATURE] = 0x50;   /* P */
	headbuf[headerStart + PE_SIGNATURE + 1] = 0x45; /* E */
	headbuf[headerStart + PE_SIGNATURE + 2] = 0x00; /* 0 */
	headbuf[headerStart + PE_SIGNATURE + 3] = 0x00; /* 0 */
	headbuf[headerStart + PE_MACHINEID] = PE_INTEL386 & 0xff;
	headbuf[headerStart + PE_MACHINEID + 1] = (PE_INTEL386 >> 8) & 0xff;
	/* store time/date of creation */
	k = (UINT)time(NULL);
	headbuf[headerStart + PE_DATESTAMP] = k & 0xff;
	headbuf[headerStart + PE_DATESTAMP + 1] = (k >> 8) & 0xff;
	headbuf[headerStart + PE_DATESTAMP + 2] = (k >> 16) & 0xff;
	headbuf[headerStart + PE_DATESTAMP + 3] = (k >> 24) & 0xff;

	headbuf[headerStart + PE_HDRSIZE] = PE_OPTIONAL_HEADER_SIZE & 0xff;
	headbuf[headerStart + PE_HDRSIZE + 1] = (PE_OPTIONAL_HEADER_SIZE >> 8) & 0xff;

	i = PE_FILE_EXECUTABLE | PE_FILE_32BIT;                   /* get flags */
	if (build_dll)
	{
		i |= PE_FILE_LIBRARY;                /* if DLL, flag it */
	}
	headbuf[headerStart + PE_FLAGS] = i & 0xff;                   /* store them */
	headbuf[headerStart + PE_FLAGS + 1] = (i >> 8) & 0xff;

	headbuf[headerStart + PE_MAGIC] = PE_MAGICNUM & 0xff; /* store magic number */
	headbuf[headerStart + PE_MAGIC + 1] = (PE_MAGICNUM >> 8) & 0xff;

	headbuf[headerStart + PE_IMAGEBASE] = image_base & 0xff; /* store image base */
	headbuf[headerStart + PE_IMAGEBASE + 1] = (image_base >> 8) & 0xff;
	headbuf[headerStart + PE_IMAGEBASE + 2] = (image_base >> 16) & 0xff;
	headbuf[headerStart + PE_IMAGEBASE + 3] = (image_base >> 24) & 0xff;

	headbuf[headerStart + PE_FILEALIGN] = file_align & 0xff; /* store image base */
	headbuf[headerStart + PE_FILEALIGN + 1] = (file_align >> 8) & 0xff;
	headbuf[headerStart + PE_FILEALIGN + 2] = (file_align >> 16) & 0xff;
	headbuf[headerStart + PE_FILEALIGN + 3] = (file_align >> 24) & 0xff;

	headbuf[headerStart + PE_OBJECTALIGN] = object_align & 0xff; /* store image base */
	headbuf[headerStart + PE_OBJECTALIGN + 1] = (object_align >> 8) & 0xff;
	headbuf[headerStart + PE_OBJECTALIGN + 2] = (object_align >> 16) & 0xff;
	headbuf[headerStart + PE_OBJECTALIGN + 3] = (object_align >> 24) & 0xff;

	headbuf[headerStart + PE_OSMAJOR] = os_major;
	headbuf[headerStart + PE_OSMINOR] = os_minor;

	headbuf[headerStart + PE_SUBSYSMAJOR] = sub_system_major;
	headbuf[headerStart + PE_SUBSYSMINOR] = sub_system_minor;

	headbuf[headerStart + PE_SUBSYSTEM] = sub_system & 0xff;
	headbuf[headerStart + PE_SUBSYSTEM + 1] = (sub_system >> 8) & 0xff;

	headbuf[headerStart + PE_NUMRVAS] = PE_NUM_VAS & 0xff;
	headbuf[headerStart + PE_NUMRVAS + 1] = (PE_NUM_VAS >> 8) & 0xff;

	headbuf[headerStart + PE_HEADERSIZE] = headerSize & 0xff;
	headbuf[headerStart + PE_HEADERSIZE + 1] = (headerSize >> 8) & 0xff;
	headbuf[headerStart + PE_HEADERSIZE + 2] = (headerSize >> 16) & 0xff;
	headbuf[headerStart + PE_HEADERSIZE + 3] = (headerSize >> 24) & 0xff;

	headbuf[headerStart + PE_HEAPSIZE] = heap_size & 0xff;
	headbuf[headerStart + PE_HEAPSIZE + 1] = (heap_size >> 8) & 0xff;
	headbuf[headerStart + PE_HEAPSIZE + 2] = (heap_size >> 16) & 0xff;
	headbuf[headerStart + PE_HEAPSIZE + 3] = (heap_size >> 24) & 0xff;

	headbuf[headerStart + PE_HEAPCOMMSIZE] = heap_commit_size & 0xff;
	headbuf[headerStart + PE_HEAPCOMMSIZE + 1] = (heap_commit_size >> 8) & 0xff;
	headbuf[headerStart + PE_HEAPCOMMSIZE + 2] = (heap_commit_size >> 16) & 0xff;
	headbuf[headerStart + PE_HEAPCOMMSIZE + 3] = (heap_commit_size >> 24) & 0xff;

	headbuf[headerStart + PE_STACKSIZE] = stack_size & 0xff;
	headbuf[headerStart + PE_STACKSIZE + 1] = (stack_size >> 8) & 0xff;
	headbuf[headerStart + PE_STACKSIZE + 2] = (stack_size >> 16) & 0xff;
	headbuf[headerStart + PE_STACKSIZE + 3] = (stack_size >> 24) & 0xff;

	headbuf[headerStart + PE_STACKCOMMSIZE] = stack_commit_size & 0xff;
	headbuf[headerStart + PE_STACKCOMMSIZE + 1] = (stack_commit_size >> 8) & 0xff;
	headbuf[headerStart + PE_STACKCOMMSIZE + 2] = (stack_commit_size >> 16) & 0xff;
	headbuf[headerStart + PE_STACKCOMMSIZE + 3] = (stack_commit_size >> 24) & 0xff;


	/* shift segment start addresses up into place and build section headers */
	sectionStart = headerSize;
	j = headerStart + PE_HEADBUF_SIZE;

	for (i = 0; i < outcount; i++, j += PE_OBJECTENTRY_SIZE)
	{
		nameIndex = out_list[i]->name_index;
		if (nameIndex >= 0)
		{
			for (k = 0; (k < strlen(name_list[nameIndex])) && (k < 8); k++)
			{
				headbuf[j + k] = name_list[nameIndex][k];
			}
		}
		k = out_list[i]->virtual_size; /* get virtual size */
		headbuf[j + 8] = k & 0xff; /* store virtual size (in memory) of segment */
		headbuf[j + 9] = (k >> 8) & 0xff;
		headbuf[j + 10] = (k >> 16) & 0xff;
		headbuf[j + 11] = (k >> 24) & 0xff;

		if (!pad_segments) /* if not padding segments, reduce space consumption */
		{
			for (k = out_list[i]->length - 1; (k >= 0) && !get_n_bit(out_list[i]->data_mask, k); k--);
			k++; /* k=initialised length */
		}
		headbuf[j + 16] = (k) & 0xff; /* store initialised data size */
		headbuf[j + 17] = (k >> 8) & 0xff;
		headbuf[j + 18] = (k >> 16) & 0xff;
		headbuf[j + 19] = (k >> 24) & 0xff;

		headbuf[j + 20] = (sectionStart) & 0xff; /* store physical file offset */
		headbuf[j + 21] = (sectionStart >> 8) & 0xff;
		headbuf[j + 22] = (sectionStart >> 16) & 0xff;
		headbuf[j + 23] = (sectionStart >> 24) & 0xff;

		k += file_align - 1;
		k &= (0xffffffff - (file_align - 1)); /* aligned initialised length */

		sectionStart += k; /* update section start address for next section */

		out_list[i]->base += headerVirtSize + image_base;
		headbuf[j + 12] = (out_list[i]->base - image_base) & 0xff;
		headbuf[j + 13] = ((out_list[i]->base - image_base) >> 8) & 0xff;
		headbuf[j + 14] = ((out_list[i]->base - image_base) >> 16) & 0xff;
		headbuf[j + 15] = ((out_list[i]->base - image_base) >> 24) & 0xff;

		k = (out_list[i]->win_flags ^ WINF_NEG_FLAGS) & WINF_IMAGE_FLAGS; /* get characteristice for section */
		headbuf[j + 36] = (k) & 0xff; /* store characteristics */
		headbuf[j + 37] = (k >> 8) & 0xff;
		headbuf[j + 38] = (k >> 16) & 0xff;
		headbuf[j + 39] = (k >> 24) & 0xff;
	}

	headbuf[headerStart + PE_NUMOBJECTS] = outcount & 0xff;       /* store number of sections */
	headbuf[headerStart + PE_NUMOBJECTS + 1] = (outcount >> 8) & 0xff;

	/* build import, export and relocation sections */

	build_pe_imports(importSectNum, headbuf + headerStart + PE_HEADBUF_SIZE);
	build_pe_exports(exportSectNum, headbuf + headerStart + PE_HEADBUF_SIZE, outname);
	build_pe_relocs(relocSectNum, headbuf + headerStart + PE_HEADBUF_SIZE);
	build_pe_resources(resourceSectNum, headbuf + headerStart + PE_HEADBUF_SIZE);

	if (error_count)
	{
		exit(1);
	}

	/* get start address */
	if (got_start_address)
	{
		get_fixup_target(outname, &start_address, &start_address.segment, &start_address.offset, TRUE);
		if (error_count)
		{
			printf("Invalid start address record\n");
			exit(1);
		}
		i = start_address.offset;
		if (start_address.segment >= 0)
		{
			i += segment_list[start_address.segment]->base;
		}
		i -= image_base; /* RVA */
		headbuf[headerStart + PE_ENTRYPOINT] = i & 0xff;
		headbuf[headerStart + PE_ENTRYPOINT + 1] = (i >> 8) & 0xff;
		headbuf[headerStart + PE_ENTRYPOINT + 2] = (i >> 16) & 0xff;
		headbuf[headerStart + PE_ENTRYPOINT + 3] = (i >> 24) & 0xff;
		if (build_dll) /* if library */
		{
			/* flag that entry point should always be called */
			headbuf[headerStart + PE_DLLFLAGS] = 0xf;
			headbuf[headerStart + PE_DLLFLAGS + 1] = 0;
		}
	}
	else
	{
		printf("Warning, no entry point specified\n");
	}

	if (importSectNum >= 0) /* if imports, add section entry */
	{
		headbuf[headerStart + PE_IMPORTRVA] = (out_list[importSectNum]->base - image_base) & 0xff;
		headbuf[headerStart + PE_IMPORTRVA + 1] = ((out_list[importSectNum]->base - image_base) >> 8) & 0xff;
		headbuf[headerStart + PE_IMPORTRVA + 2] = ((out_list[importSectNum]->base - image_base) >> 16) & 0xff;
		headbuf[headerStart + PE_IMPORTRVA + 3] = ((out_list[importSectNum]->base - image_base) >> 24) & 0xff;
		headbuf[headerStart + PE_IMPORTSIZE] = (out_list[importSectNum]->length) & 0xff;
		headbuf[headerStart + PE_IMPORTSIZE + 1] = (out_list[importSectNum]->length >> 8) & 0xff;
		headbuf[headerStart + PE_IMPORTSIZE + 2] = (out_list[importSectNum]->length >> 16) & 0xff;
		headbuf[headerStart + PE_IMPORTSIZE + 3] = (out_list[importSectNum]->length >> 24) & 0xff;
	}
	if (relocSectNum >= 0) /* if relocs, add section entry */
	{
		headbuf[headerStart + PE_FIXUPRVA] = (out_list[relocSectNum]->base - image_base) & 0xff;
		headbuf[headerStart + PE_FIXUPRVA + 1] = ((out_list[relocSectNum]->base - image_base) >> 8) & 0xff;
		headbuf[headerStart + PE_FIXUPRVA + 2] = ((out_list[relocSectNum]->base - image_base) >> 16) & 0xff;
		headbuf[headerStart + PE_FIXUPRVA + 3] = ((out_list[relocSectNum]->base - image_base) >> 24) & 0xff;
		headbuf[headerStart + PE_FIXUPSIZE] = (out_list[relocSectNum]->length) & 0xff;
		headbuf[headerStart + PE_FIXUPSIZE + 1] = (out_list[relocSectNum]->length >> 8) & 0xff;
		headbuf[headerStart + PE_FIXUPSIZE + 2] = (out_list[relocSectNum]->length >> 16) & 0xff;
		headbuf[headerStart + PE_FIXUPSIZE + 3] = (out_list[relocSectNum]->length >> 24) & 0xff;
	}

	if (exportSectNum >= 0) /* if relocs, add section entry */
	{
		headbuf[headerStart + PE_EXPORTRVA] = (out_list[exportSectNum]->base - image_base) & 0xff;
		headbuf[headerStart + PE_EXPORTRVA + 1] = ((out_list[exportSectNum]->base - image_base) >> 8) & 0xff;
		headbuf[headerStart + PE_EXPORTRVA + 2] = ((out_list[exportSectNum]->base - image_base) >> 16) & 0xff;
		headbuf[headerStart + PE_EXPORTRVA + 3] = ((out_list[exportSectNum]->base - image_base) >> 24) & 0xff;
		headbuf[headerStart + PE_EXPORTSIZE] = (out_list[exportSectNum]->length) & 0xff;
		headbuf[headerStart + PE_EXPORTSIZE + 1] = (out_list[exportSectNum]->length >> 8) & 0xff;
		headbuf[headerStart + PE_EXPORTSIZE + 2] = (out_list[exportSectNum]->length >> 16) & 0xff;
		headbuf[headerStart + PE_EXPORTSIZE + 3] = (out_list[exportSectNum]->length >> 24) & 0xff;
	}

	if (resourceSectNum >= 0) /* if relocs, add section entry */
	{
		headbuf[headerStart + PE_RESOURCERVA] = (out_list[resourceSectNum]->base - image_base) & 0xff;
		headbuf[headerStart + PE_RESOURCERVA + 1] = ((out_list[resourceSectNum]->base - image_base) >> 8) & 0xff;
		headbuf[headerStart + PE_RESOURCERVA + 2] = ((out_list[resourceSectNum]->base - image_base) >> 16) & 0xff;
		headbuf[headerStart + PE_RESOURCERVA + 3] = ((out_list[resourceSectNum]->base - image_base) >> 24) & 0xff;
		headbuf[headerStart + PE_RESOURCESIZE] = (out_list[resourceSectNum]->length) & 0xff;
		headbuf[headerStart + PE_RESOURCESIZE + 1] = (out_list[resourceSectNum]->length >> 8) & 0xff;
		headbuf[headerStart + PE_RESOURCESIZE + 2] = (out_list[resourceSectNum]->length >> 16) & 0xff;
		headbuf[headerStart + PE_RESOURCESIZE + 3] = (out_list[resourceSectNum]->length >> 24) & 0xff;
	}

	j = headerStart + PE_HEADBUF_SIZE + (outcount - 1) * PE_OBJECTENTRY_SIZE;

	i = headbuf[j + 12];        /* get base of last section - image base */
	i += headbuf[j + 13] << 8;
	i += headbuf[j + 14] << 16;
	i += headbuf[j + 15] << 24;

	i += headbuf[j + 8];        /* add virtual size of section */
	i += headbuf[j + 9] << 8;
	i += headbuf[j + 10] << 16;
	i += headbuf[j + 11] << 24;

	headbuf[headerStart + PE_IMAGESIZE] = i & 0xff;
	headbuf[headerStart + PE_IMAGESIZE + 1] = (i >> 8) & 0xff;
	headbuf[headerStart + PE_IMAGESIZE + 2] = (i >> 16) & 0xff;
	headbuf[headerStart + PE_IMAGESIZE + 3] = (i >> 24) & 0xff;

	headbuf[headerStart + PE_CODEBASE] = codeBase & 0xff;
	headbuf[headerStart + PE_CODEBASE + 1] = (codeBase >> 8) & 0xff;
	headbuf[headerStart + PE_CODEBASE + 2] = (codeBase >> 16) & 0xff;
	headbuf[headerStart + PE_CODEBASE + 3] = (codeBase >> 24) & 0xff;

	headbuf[headerStart + PE_DATABASE] = dataBase & 0xff;
	headbuf[headerStart + PE_DATABASE + 1] = (dataBase >> 8) & 0xff;
	headbuf[headerStart + PE_DATABASE + 2] = (dataBase >> 16) & 0xff;
	headbuf[headerStart + PE_DATABASE + 3] = (dataBase >> 24) & 0xff;

	headbuf[headerStart + PE_CODESIZE] = codeSize & 0xff;
	headbuf[headerStart + PE_CODESIZE + 1] = (codeSize >> 8) & 0xff;
	headbuf[headerStart + PE_CODESIZE + 2] = (codeSize >> 16) & 0xff;
	headbuf[headerStart + PE_CODESIZE + 3] = (codeSize >> 24) & 0xff;

	headbuf[headerStart + PE_INITDATASIZE] = dataSize & 0xff;
	headbuf[headerStart + PE_INITDATASIZE + 1] = (dataSize >> 8) & 0xff;
	headbuf[headerStart + PE_INITDATASIZE + 2] = (dataSize >> 16) & 0xff;
	headbuf[headerStart + PE_INITDATASIZE + 3] = (dataSize >> 24) & 0xff;

	/* zero out section start for all zero-length segments */
	j = headerStart + PE_HEADBUF_SIZE;
	for (i = 0; i < outcount; i++, j += PE_OBJECTENTRY_SIZE)
	{
		/* check if size in file is zero */
		k = headbuf[j + 16] | headbuf[j + 17] | headbuf[j + 18] | headbuf[j + 19];
		if (!k)
		{
			/* if so, zero section start */
			headbuf[j + 20] = headbuf[j + 21] = headbuf[j + 22] = headbuf[j + 23] = 0;
		}
	}

	if (error_count != 0)
	{
		exit(1);
	}

	outfile = fopen(outname, "wb");
	if (!outfile)
	{
		printf("Error writing to file %s\n", outname);
		exit(1);
	}

	for (i = 0; i < headerSize; i++)
	{
		fputc(headbuf[i], outfile);
	}

	started = lastout = image_base + headerVirtSize;

	for (i = 0; i < outcount; i++)
	{
		if (out_list[i] && ((out_list[i]->attributes & SEG_ALIGN) != SEG_ABS))
		{
			/* ensure section is aligned to file-Align */
			while (ftell(outfile) & (file_align - 1))
			{
				fputc(0, outfile);
			}
			if (started > out_list[i]->base)
			{
				printf("Segment overlap\n");
				printf("Next addr=%08X,base=%08X\n", started, out_list[i]->base);
				fclose(outfile);
				exit(1);
			}
			if (pad_segments)
			{
				while (started < out_list[i]->base)
				{
					fputc(0, outfile);
					started++;
				}
			}
			else
			{
				started = out_list[i]->base;
			}
			for (j = 0; j < out_list[i]->length; j++)
			{
				if (get_n_bit(out_list[i]->data_mask, j))
				{
					for (; lastout < started; lastout++)
					{
						fputc(0, outfile);
					}
					fputc(out_list[i]->data[j], outfile);
					lastout = started + 1;
				}
				else if (pad_segments)
				{
					fputc(0, outfile);
					lastout = started + 1;
				}
				started++;
			}
			started = lastout = out_list[i]->base + out_list[i]->virtual_size;
		}
	}

	fclose(outfile);
}

