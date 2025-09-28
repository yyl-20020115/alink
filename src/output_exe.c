#include "alink.h"
BOOL output_exe_file(PCHAR out_name)
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
	USHORT temp_segment;
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
		BOOL found = get_fixup_target(out_name, &start_address, &start_address.segment, &start_address.offset, FALSE);
		if (error_count)
		{
			printf("Invalid start address record\n");
			return FALSE;
			//exit(1);
		}
		if (found) {

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
					return FALSE;
					//exit(1);
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
		BOOL found = get_fixup_target(out_name, relocations[i], &target_segment, &target_offset, FALSE);

		if (target_segment != NUMBER_TARGET) {
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
						printf("Error: %li:Offset for %s(%s) out of range\n", i,
							extern_records[relocations[i]->target].name,
							relocations[i]->owner_file_name
						);

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
					printf("Error: %li:Offset for %s(%s) out of range\n", i, 
						extern_records[relocations[i]->target].name,
						relocations[i]->owner_file_name
					);
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
						printf("Error: Short (-128~127) reloc %li for %s(%s:%s:%08X) out of range\n", i,
							extern_records[relocations[i]->target].name,
							relocations[i]->owner_file_name,
							segment_list[relocations[i]->segment]->name,
							relocations[i]->offset_in_file
						);

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
					printf("Error: Absolute reloc target not supported for self-relative fixups\n");
					error_count++;
				}
				else
				{
					j = segment_list[target_segment]->base + target_offset;
					long d = j - segment_list[relocations[i]->segment]->base + relocations[i]->offset + 2;
					if ((d < -32768) || (d > 32767))
					{
						if (!patch_near_branches) {

							//NOTICE:
							printf("Error: Near (-32768~32767) reloc %8li for %s\t(%s:%s:%08X) out of range\n", i,
								extern_records[relocations[i]->target].name,
								relocations[i]->owner_file_name,
								segment_list[relocations[i]->segment]->name,
								relocations[i]->offset_in_file
							);
						}
						else if (relocations[i]->offset >= 3) {
							PUCHAR p = &segment_list[relocations[i]->segment]->data[relocations[i]->offset]-3;
							if (p[0] == 0x90 && p[1] == 0x90) {
								p[0] = 0x90;
								p[1] = 0xff;
								//90F16 0000o               	CALL CS : WORD PTR[Addr]
								//90FF26 0000o               	JMP  CS : WORD PTR[Addr]
								UINT doit = TRUE;
								//USE LAST SEGMENT AS DATA SEGMENT
								UINT length = segment_list[segcount_combined-1]->length;
								PUCHAR data = segment_list[segcount_combined - 1]->data;
								UINT delta = length % 2 == 0 ? 2 : 3;
								length =
									(segment_list[segcount_combined - 1]->virtual_size) =
									(segment_list[segcount_combined - 1]->length += delta);
								if (length > 0xffff) {
									printf("Error allocating memory for patch data!\n");
								}
								data = segment_list[segcount_combined - 1]->data
									 = (PUCHAR)realloc(
										segment_list[segcount_combined-1]->data,length);
								//printf("Direct offset is stored in %08X\n",length-2);
								switch (p[2])
								{
								case 0xe8: //call
									p[2] = 0x16;
									break;
								case 0xe9: //jmp
									p[2] = 0x26;
									break;
								default:
									doit = FALSE;
									break;
								}
								if (doit) {
									p[3] = (UCHAR)(length-2)&0xff;
									p[4] = (UCHAR)((length-2)>>8)&0xff;

									data[length - 2] = (UCHAR)(j&0xff);
									data[length - 1] = (UCHAR)((j>>8)&0xff);
								}
							}

						}
					}
					else
					{
						j = d;
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
					printf("Error: Absolute reloc target not supported for self-relative fixups\n");
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
				break;
			}
		}
	}

	if (relocation_count > 0x10000)
	{
		printf("Too many relocations\n");
		//return FALSE;
		//exit(1);
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
		//return FALSE;
		//exit(1);
	}

	outfile = fopen(out_name, "wb");
	if (!outfile)
	{
		printf("Error writing to file %s\n", out_name);
		//return FALSE;
		//NOTICE:
		//exit(1);
	}

	i = (header_buffer[0x08] + (((UINT)header_buffer[0x09]) << 8)) * 16;
	if (fwrite(header_buffer, 1, i, outfile) != i)
	{
		printf("Error writing to file %s\n", out_name);
		return FALSE;
		//NOTICE:
		//exit(1);
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
				//fclose(outfile);
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
		i = lastout & 0x1ff;
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
			//return FALSE;
			//NOTICE:
			//exit(1);
		}
	}
	fclose(outfile);
	return TRUE;
}
