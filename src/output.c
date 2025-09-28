#include "alink.h"

BOOL get_fixup_target(PCHAR fname, PRELOC relocation, long* _target_segment_number, UINT* _target_offset, int is_flat)
{
	long base_segment_number = -1L;
	long target_segment_number = -1L;
	UINT target_offset = 0;
	*_target_segment_number = -1;
	*_target_offset = -1;

	if (relocation->segment < 0) return FALSE;

	relocation->output_pos
		= segment_list[relocation->segment]->base
		+ relocation->offset;

	switch (relocation->ftype)
	{
	case REL_SEGFRAME:
	case REL_LILEFRAME:
		base_segment_number = relocation->frame;
		break;
	case REL_GRPFRAME:
		base_segment_number = group_list[relocation->frame]->segment_number;
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

			base_segment_number = extern_records[relocation->frame].pubdef->segment;
			break;
		case EXT_MATCHEDIMPORT:
			base_segment_number = import_definitions[extern_records[relocation->frame].import].segment;
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
	if (base_segment_number < 0)
	{
		printf("Undefined base seg\n");
		//NOTICE:
		//return FALSE;
		//exit(1);
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

			target_segment_number = extern_records[relocation->target].pubdef->segment;
			target_offset = extern_records[relocation->target].pubdef->offset;
			break;
		case EXT_MATCHEDIMPORT:
			target_segment_number = import_definitions[extern_records[relocation->target].import].segment;
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

			target_segment_number = extern_records[relocation->target].pubdef->segment;
			target_offset = extern_records[relocation->target].pubdef->offset;
			break;
		case EXT_MATCHEDIMPORT:
			target_segment_number = import_definitions[extern_records[relocation->target].import].segment;
			target_offset = import_definitions[extern_records[relocation->target].import].offset;
			break;
		default:
			printf("Reloc:Unmatched external referenced in frame\n");
			error_count++;
			break;
		}
		break;
	case REL_SEGONLY:
		target_segment_number = relocation->target;
		target_offset = 0;
		break;
	case REL_SEGDISP:
		target_segment_number = relocation->target;
		target_offset = relocation->disp;
		break;
	case REL_GRPONLY:
		target_segment_number = group_list[relocation->target]->segment_number;
		target_offset = 0;
		break;
	case REL_GRPDISP:
		target_segment_number = group_list[relocation->target]->segment_number;
		target_offset = relocation->disp;
		break;
	default:
		printf("Reloc:Unsupported TARGET type %i\n", relocation->ttype);
		error_count++;
		break;
	}
	if (target_segment_number == NUMBER_TARGET)
	{
		if (extern_records[relocation->target].pubdef != NULL) {
#if 0
			printf("NUMBER %s(%ld) = %08X\n",
				extern_records[relocation->target].name,
				relocation->target,
				extern_records[relocation->target].pubdef->offset
			);
#endif
			_target_segment_number = target_segment_number;
			return TRUE;
			//NOTICE:
			error_count++;
		}
		//return FALSE;
		//exit(`/1);
	}
	if ((!error_count) && (!segment_list[target_segment_number]))
	{
		printf("Reloc: no target segment\n");

		error_count++;
	}
	if ((!error_count) && (!segment_list[base_segment_number]))
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
		if ((segment_list[base_segment_number]->attributes & SEG_ALIGN) == SEG_ABS)
		{
			printf("Warning: Reloc frame is absolute segment\n");
			target_segment_number = base_segment_number;
		}
		else if ((segment_list[target_segment_number]->attributes & SEG_ALIGN) == SEG_ABS)
		{
			printf("Warning: Reloc target is in absolute segment\n");
			target_segment_number = base_segment_number;
		}
		if (!is_flat || ((segment_list[base_segment_number]->attributes & SEG_ALIGN) == SEG_ABS))
		{
			if (base_segment_number <= target_segment_number && segment_list[base_segment_number]->base > (segment_list[target_segment_number]->base + target_offset))
			{
				printf("Error: target address %08X:%08X out of frame\n",
					segment_list[base_segment_number]->base,
					segment_list[target_segment_number]->base + target_offset);
				error_count++;
			}
			target_offset += segment_list[target_segment_number]->base - segment_list[base_segment_number]->base;
			target_offset &= 0xffff;
			*_target_segment_number = base_segment_number;
			*_target_offset = target_offset;
		}
		else
		{
			*_target_segment_number = -1;
			*_target_offset = target_offset + segment_list[target_segment_number]->base;
		}
	}
	else
	{
		//printf("relocation error occurred\n");
		*_target_segment_number = 0;
		*_target_offset = 0;
		return FALSE;
	}
	return TRUE;
}


