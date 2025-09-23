#include "alink.h"

static void fix_public_segments(int src, int dest, UINT shift)
{
	UINT i, j;
	PPUBLIC q;

	for (i = 0; i < pubcount; ++i)
	{
		for (j = 0; j < public_entries[i].count; ++j)
		{
			q = (PPUBLIC)public_entries[i].object[j];
			if (q->segment == src)
			{
				q->segment = dest;
				q->offset += shift;
			}
		}
	}
}

static void fix_public_groups(int src, int dest)
{
	UINT i, j;
	PPUBLIC q;

	for (i = 0; i < pubcount; ++i)
	{
		for (j = 0; j < public_entries[i].count; ++j)
		{
			q = (PPUBLIC)public_entries[i].object[j];
			if (q->group == src)
			{
				q->group = dest;
			}
		}
	}
}

void combine_segments(PCHAR fname, long dest, long src)
{
	UINT k, n;
	PUCHAR p, q;
	long a1, a2;

	k = segment_list[dest]->length;
	switch (segment_list[src]->attributes & SEG_ALIGN)
	{
	case SEG_WORD:
		a2 = 2;
		k = (k + 1) & 0xfffffffe;
		break;
	case SEG_PARA:
		a2 = 16;
		k = (k + 0xf) & 0xfffffff0;
		break;
	case SEG_PAGE:
		a2 = 0x100;
		k = (k + 0xff) & 0xffffff00;
		break;
	case SEG_DWORD:
		a2 = 4;
		k = (k + 3) & 0xfffffffc;
		break;
	case SEG_MEMPAGE:
		a2 = 0x1000;
		k = (k + 0xfff) & 0xfffff000;
		break;
	case SEG_8BYTE:
		a2 = 8;
		k = (k + 7) & 0xfffffff8;
		break;
	case SEG_32BYTE:
		a2 = 32;
		k = (k + 31) & 0xffffffe0;
		break;
	case SEG_64BYTE:
		a2 = 64;
		k = (k + 63) & 0xffffffc0;
		break;
	default:
		a2 = 1;
		break;
	}
	switch (segment_list[dest]->attributes & SEG_ALIGN)
	{
	case SEG_WORD:
		a1 = 2;
		break;
	case SEG_DWORD:
		a1 = 4;
		break;
	case SEG_8BYTE:
		a1 = 8;
		break;
	case SEG_PARA:
		a1 = 16;
		break;
	case SEG_32BYTE:
		a1 = 32;
		break;
	case SEG_64BYTE:
		a1 = 64;
		break;
	case SEG_PAGE:
		a1 = 0x100;
		break;
	case SEG_MEMPAGE:
		a1 = 0x1000;
		break;
	default:
		a1 = 1;
		break;
	}
	segment_list[src]->base = k;
	p = check_malloc(k + segment_list[src]->length);
	q = check_malloc((k + segment_list[src]->length + 7) >> 3);
	for (k = 0; k < segment_list[dest]->length; k++)
	{
		if (get_n_bit(segment_list[dest]->data_mask, k))
		{
			set_n_bit(q, k);
			p[k] = segment_list[dest]->data[k];
		}
		else
		{
			clear_n_bit(q, k);
		}
	}
	for (; k < segment_list[src]->base; k++)
	{
		clear_n_bit(q, k);
	}
	for (; k < (segment_list[src]->base + segment_list[src]->length); k++)
	{
		if (get_n_bit(segment_list[src]->data_mask, k - segment_list[src]->base))
		{
			p[k] = segment_list[src]->data[k - segment_list[src]->base];
			set_n_bit(q, k);
		}
		else
		{
			clear_n_bit(q, k);
		}
	}
	segment_list[dest]->length = k;
	if (a2 > a1) segment_list[dest]->attributes = segment_list[src]->attributes;
	segment_list[dest]->win_flags |= segment_list[src]->win_flags;
	free(segment_list[dest]->data);
	free(segment_list[src]->data);
	free(segment_list[dest]->data_mask);
	free(segment_list[src]->data_mask);
	segment_list[dest]->data = p;
	segment_list[dest]->data_mask = q;

	fix_public_segments(src, dest, segment_list[src]->base);

	for (k = 0; k < fixcount; k++)
	{
		if (relocations[k]->segment == src)
		{
			relocations[k]->segment = dest;
			relocations[k]->offset += segment_list[src]->base;
		}
		if (relocations[k]->ttype == REL_SEGDISP)
		{
			if (relocations[k]->target == src)
			{
				relocations[k]->target = dest;
				relocations[k]->disp += segment_list[src]->base;
			}
		}
		else if (relocations[k]->ttype == REL_SEGONLY)
		{
			if (relocations[k]->target == src)
			{
				relocations[k]->target = dest;
				relocations[k]->ttype = REL_SEGDISP;
				relocations[k]->disp = segment_list[src]->base;
			}
		}
		if ((relocations[k]->ftype == REL_SEGFRAME) ||
			(relocations[k]->ftype == REL_LILEFRAME))
		{
			if (relocations[k]->frame == src)
			{
				relocations[k]->frame = dest;
			}
		}
	}

	if (got_start_address)
	{
		if (start_address.ttype == REL_SEGDISP)
		{
			if (start_address.target == src)
			{
				start_address.target = dest;
				start_address.disp += segment_list[src]->base;
			}
		}
		else if (start_address.ttype == REL_SEGONLY)
		{
			if (start_address.target == src)
			{
				start_address.target = dest;
				start_address.disp = segment_list[src]->base;
				start_address.ttype = REL_SEGDISP;
			}
		}
		if ((start_address.ftype == REL_SEGFRAME) ||
			(start_address.ftype == REL_LILEFRAME))
		{
			if (start_address.frame == src)
			{
				start_address.frame = dest;
			}
		}
	}

	for (k = 0; k < grpcount; k++)
	{
		if (group_list[k])
		{
			for (n = 0; n < group_list[k]->numsegs; n++)
			{
				if (group_list[k]->segindex[n] == src)
				{
					group_list[k]->segindex[n] = dest;
				}
			}
		}
	}

	free(segment_list[src]);
	segment_list[src] = 0;
}

void combine_common(PCHAR fname, long i, long j)
{
	UINT k, n;
	PUCHAR p, q;

	if (segment_list[j]->length > segment_list[i]->length)
	{
		k = segment_list[i]->length;
		segment_list[i]->length = segment_list[j]->length;
		segment_list[j]->length = k;
		p = segment_list[i]->data;
		q = segment_list[i]->data_mask;
		segment_list[i]->data = segment_list[j]->data;
		segment_list[i]->data_mask = segment_list[j]->data_mask;
	}
	else
	{
		p = segment_list[j]->data;
		q = segment_list[j]->data_mask;
	}
	for (k = 0; k < segment_list[j]->length; k++)
	{
		if (get_n_bit(q, k))
		{
			if (get_n_bit(segment_list[i]->data_mask, k))
			{
				if (segment_list[i]->data[k] != p[k])
				{
					report_error(fname, ERR_OVERWRITE);
				}
			}
			else
			{
				set_n_bit(segment_list[i]->data_mask, k);
				segment_list[i]->data[k] = p[k];
			}
		}
	}
	free(p);
	free(q);

	fix_public_segments(j, i, 0);

	for (k = 0; k < fixcount; k++)
	{
		if (relocations[k]->segment == j)
		{
			relocations[k]->segment = i;
		}
		if (relocations[k]->ttype == REL_SEGDISP)
		{
			if (relocations[k]->target == j)
			{
				relocations[k]->target = i;
			}
		}
		else if (relocations[k]->ttype == REL_SEGONLY)
		{
			if (relocations[k]->target == j)
			{
				relocations[k]->target = i;
			}
		}
		if ((relocations[k]->ftype == REL_SEGFRAME) ||
			(relocations[k]->ftype == REL_LILEFRAME))
		{
			if (relocations[k]->frame == j)
			{
				relocations[k]->frame = i;
			}
		}
	}

	if (got_start_address)
	{
		if (start_address.ttype == REL_SEGDISP)
		{
			if (start_address.target == j)
			{
				start_address.target = i;
			}
		}
		else if (start_address.ttype == REL_SEGONLY)
		{
			if (start_address.target == j)
			{
				start_address.target = i;
			}
		}
		if ((start_address.ftype == REL_SEGFRAME) ||
			(start_address.ftype == REL_LILEFRAME))
		{
			if (start_address.frame == j)
			{
				start_address.frame = i;
			}
		}
	}

	for (k = 0; k < grpcount; k++)
	{
		if (group_list[k])
		{
			for (n = 0; n < group_list[k]->numsegs; n++)
			{
				if (group_list[k]->segindex[n] == j)
				{
					group_list[k]->segindex[n] = i;
				}
			}
		}
	}

	free(segment_list[j]);
	segment_list[j] = 0;
}

void combine_groups(PCHAR fname, long i, long j)
{
	long n, m;
	char match;

	for (n = 0; n < group_list[j]->numsegs; n++)
	{
		match = 0;
		for (m = 0; m < group_list[i]->numsegs; m++)
		{
			if (group_list[j]->segindex[n] == group_list[i]->segindex[m])
			{
				match = 1;
			}
		}
		if (!match)
		{
			group_list[i]->numsegs++;
			group_list[i]->segindex[group_list[i]->numsegs] = group_list[j]->segindex[n];
		}
	}
	free(group_list[j]);
	group_list[j] = 0;

	fix_public_groups(j, i);

	for (n = 0; n < fixcount; n++)
	{
		if (relocations[n]->ftype == REL_GRPFRAME)
		{
			if (relocations[n]->frame == j)
			{
				relocations[n]->frame = i;
			}
		}
		if ((relocations[n]->ttype == REL_GRPONLY) || (relocations[n]->ttype == REL_GRPDISP))
		{
			if (relocations[n]->target == j)
			{
				relocations[n]->target = i;
			}
		}
	}

	if (got_start_address)
	{
		if ((start_address.ttype == REL_GRPDISP) || (start_address.ttype == REL_GRPONLY))
		{
			if (start_address.target == j)
			{
				start_address.target = i;
			}
		}
		if (start_address.ftype == REL_GRPFRAME)
		{
			if (start_address.frame == j)
			{
				start_address.frame = i;
			}
		}
	}
}

void combine_blocks(PCHAR fname)
{
	long i, j, k;
	char* name;
	long attr;
	UINT count;
	UINT* slist;
	UINT curseg;

	for (i = 0; i < segcount; i++)
	{
		if (segment_list[i] && ((segment_list[i]->attributes & SEG_ALIGN) != SEG_ABS))
		{
			segcount_combined++;
			if (segment_list[i]->win_flags & WINF_COMDAT) continue; /* don't combine COMDAT segments */
			name = name_list[segment_list[i]->name_index];
			attr = segment_list[i]->attributes & (SEG_COMBINE | SEG_USE32);
			switch (attr & SEG_COMBINE)
			{
			case SEG_STACK:
			{
				for (j = i + 1; j < segcount; j++)
				{
					if (!segment_list[j]) continue;
					if (segment_list[j]->win_flags & WINF_COMDAT) continue;
					if ((segment_list[j]->attributes & SEG_ALIGN) == SEG_ABS) continue;
					if ((segment_list[j]->attributes & SEG_COMBINE) != SEG_STACK) continue;
					combine_segments(fname, i, j);
				}
				break;
			}
			case SEG_PUBLIC:
			case SEG_PUBLIC2:
			case SEG_PUBLIC3:
			{
				slist = (UINT*)check_malloc(sizeof(UINT));
				slist[0] = i;
				/* get list of segments to combine */
				for (j = i + 1, count = 1; j < segcount; j++)
				{
					if (!segment_list[j]) continue;
					if (segment_list[j]->win_flags & WINF_COMDAT) continue;
					if ((segment_list[j]->attributes & SEG_ALIGN) == SEG_ABS) continue;
					if (attr != (segment_list[j]->attributes & (SEG_COMBINE | SEG_USE32))) continue;
					if (strcmp(name, name_list[segment_list[j]->name_index]) != 0) continue;
					slist = (UINT*)check_realloc(slist, (count + 1) * sizeof(UINT));
					slist[count] = j;
					count++;
				}
				/* sort them by sortorder */
				for (j = 1; j < count; j++)
				{
					curseg = slist[j];
					for (k = j - 1; k >= 0; k--)
					{
						if (segment_list[slist[k]]->order_index < 0) break;
						if (segment_list[curseg]->order_index >= 0)
						{
							if (strcmp(name_list[segment_list[curseg]->order_index],
								name_list[segment_list[slist[k]]->order_index]) >= 0) break;
						}
						slist[k + 1] = slist[k];
					}
					k++;
					slist[k] = curseg;
				}
				/* then combine in that order */
				for (j = 1; j < count; j++)
				{
					combine_segments(fname, i, slist[j]);
				}
				free(slist);
				break;
			}
			case SEG_COMMON:
			{
				for (j = i + 1; j < segcount; j++)
				{
					if ((segment_list[j] && ((segment_list[j]->attributes & SEG_ALIGN) != SEG_ABS)) &&
						((segment_list[i]->attributes & (SEG_ALIGN | SEG_COMBINE | SEG_USE32)) == (segment_list[j]->attributes & (SEG_ALIGN | SEG_COMBINE | SEG_USE32)))
						&&
						(strcmp(name, name_list[segment_list[j]->name_index]) == 0)
						&& !(segment_list[j]->win_flags & WINF_COMDAT)
						)
					{
						combine_common(fname, i, j);
					}
				}
				break;
			}
			default:
				break;
			}
		}
	}

	for (i = 0; i < grpcount; i++)
	{
		if (group_list[i])
		{
			for (j = i + 1; j < grpcount; j++)
			{
				if (!group_list[j]) continue;
				if (strcmp(name_list[group_list[i]->name_index], name_list[group_list[j]->name_index]) == 0)
				{
					combine_groups(fname, i, j);
				}
			}
		}
	}
}


