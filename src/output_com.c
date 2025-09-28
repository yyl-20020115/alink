#include "alink.h"

BOOL output_com_file(PCHAR outname)
{
	long i, j;
	UINT started;
	UINT lastout;
	long target_segment;
	UINT target_offset;
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
			return FALSE;
			//NOTICE:
			//exit(1);
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
		BOOL found = get_fixup_target(outname, relocations[i], &target_segment, &target_offset, FALSE);
		if (target_segment != NUMBER_TARGET) {

			switch (relocations[i]->rtype)
			{
			case FIX_BASE:
			case FIX_PTR1616:
			case FIX_PTR1632:
				if ((segment_list[target_segment]->attributes & SEG_ALIGN) != SEG_ABS)
				{
					printf("Reloc %li:Segment selector relocations are not supported in COM files\n", i);
					error_count++;
				}
				else
				{
					j = relocations[i]->offset;
					if (relocations[i]->rtype == FIX_PTR1616)
					{
						if (target_offset > 0xffff)
						{
							printf("Relocs %li:Offset out of range\n", i);
							error_count++;
						}
						temps = segment_list[relocations[i]->segment]->data[j];
						temps += segment_list[relocations[i]->segment]->data[j + 1] << 8;
						temps += (USHORT)target_offset;
						temps += segment_list[target_segment]->base & 0xf; /* non-para seg */
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
						templ += (USHORT)target_offset;
						templ += segment_list[target_segment]->base & 0xf; /* non-para seg */
						segment_list[relocations[i]->segment]->data[j] = templ & 0xff;
						segment_list[relocations[i]->segment]->data[j + 1] = (templ >> 8) & 0xff;
						segment_list[relocations[i]->segment]->data[j + 2] = (templ >> 16) & 0xff;
						segment_list[relocations[i]->segment]->data[j + 3] = (templ >> 24) & 0xff;
						j += 4;
					}
					temps = segment_list[relocations[i]->segment]->data[j];
					temps += segment_list[relocations[i]->segment]->data[j + 1] << 8;
					temps += (USHORT)segment_list[target_segment]->absolute_frame;
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
				templ += (USHORT)target_offset;
				templ += segment_list[target_segment]->base & 0xf; /* non-para seg */
				segment_list[relocations[i]->segment]->data[relocations[i]->offset] = templ & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (templ >> 8) & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset + 2] = (templ >> 16) & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset + 3] = (templ >> 24) & 0xff;
				break;
			case FIX_OFS16:
			case FIX_OFS16_2:
				if (target_offset > 0xffff)
				{
					printf("Relocs %li:Offset out of range\n", i);
					error_count++;
				}
				temps = segment_list[relocations[i]->segment]->data[relocations[i]->offset];
				temps += segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] << 8;
				temps += (USHORT)target_offset;
				temps += segment_list[target_segment]->base & 0xf; /* non-para seg */
				segment_list[relocations[i]->segment]->data[relocations[i]->offset] = temps & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset + 1] = (temps >> 8) & 0xff;
				break;
			case FIX_LBYTE:
				segment_list[relocations[i]->segment]->data[relocations[i]->offset] += target_offset & 0xff;
				segment_list[relocations[i]->segment]->data[relocations[i]->offset] += segment_list[target_segment]->base & 0xf; /* non-para seg */
				break;
			case FIX_HBYTE:
				templ = (USHORT)target_offset + (USHORT)(segment_list[target_segment]->base & 0xf); /* non-para seg */
				segment_list[relocations[i]->segment]->data[relocations[i]->offset] += (templ >> 8) & 0xff;
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
				if ((segment_list[target_segment]->attributes & SEG_ALIGN) == SEG_ABS)
				{
					printf("Error: Absolute Reloc target not supported for self-relative fixups\n");
					error_count++;
				}
				else
				{
					j = segment_list[target_segment]->base + target_offset;
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
	}

	if (error_count != 0)
	{
		//return FALSE;
		//NOTICE:
		//exit(1);
	}
	outfile = fopen(outname, "wb");
	if (!outfile)
	{
		printf("Error writing to file %s\n", outname);
		return FALSE;
		//NOTICE:
		//exit(1);
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
				return FALSE;
				//NOTICE:
				//exit(1);
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
	return TRUE;
}
