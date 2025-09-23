#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
#define strcasecmp _stricmp
#define strupr _strupr
#define strdup _strdup
#endif

#define TRUE  (1==1)
#define FALSE (1==0)

#define SWITCHCHAR '-'
#define PATH_CHAR '\\'
#define DEFAULT_EXTENSION ".obj"

#define ERR_EXTRA_DATA 1
#define ERR_NO_HEADER 2
#define ERR_NO_RECDATA 3
#define ERR_NO_MEM 4
#define ERR_INV_DATA 5
#define ERR_INV_SEG 6
#define ERR_BAD_FIXUP 7
#define ERR_BAD_SEGDEF 8
#define ERR_ABS_SEG 9
#define ERR_DUP_PUBLIC 10
#define ERR_NO_MODEND 11
#define ERR_EXTRA_HEADER 12
#define ERR_UNKNOWN_RECTYPE 13
#define ERR_SEG_TOO_LARGE 14
#define ERR_MULTIPLE_STARTS 15
#define ERR_BAD_GRPDEF 16
#define ERR_OVERWRITE 17
#define ERR_INVALID_COMENT 18
#define ERR_ILLEGAL_IMPORTS 19

#define PREV_LE 1
#define PREV_LI 2
#define PREV_LI32 3

#define THEADR 0x80
#define LHEADR 0x82
#define COMENT 0x88
#define MODEND 0x8a
#define MODEND32 0x8b
#define EXTDEF 0x8c
#define TYPDEF 0x8e
#define PUBDEF 0x90
#define PUBDEF32 0x91
#define LINNUM 0x94
#define LINNUM32 0x95
#define LNAMES 0x96
#define SEGDEF 0x98
#define SEGDEF32 0x99
#define GRPDEF 0x9a
#define FIXUPP 0x9c
#define FIXUPP32 0x9d
#define LEDATA 0xa0
#define LEDATA32 0xa1
#define LIDATA 0xa2
#define LIDATA32 0xa3
#define COMDEF 0xb0
#define BAKPAT 0xb2
#define BAKPAT32 0xb3
#define LEXTDEF 0xb4
#define LEXTDEF32 0xb5
#define LPUBDEF 0xb6
#define LPUBDEF32 0xb7
#define LCOMDEF 0xb8
#define CEXTDEF 0xbc
#define COMDAT 0xc2
#define COMDAT32 0xc3
#define LINSYM 0xc4
#define LINSYM32 0xc5
#define ALIAS 0xc6
#define NBKPAT 0xc8
#define NBKPAT32 0xc9
#define LLNAMES 0xca
#define LIBHDR 0xf0
#define LIBEND 0xf1

#define COMENT_TRANSLATOR 0x00
#define COMENT_INTEL_COPYRIGHT 0x01
#define COMENT_LIB_SPEC 0x81
#define COMENT_MSDOS_VER 0x9c
#define COMENT_MEMMODEL 0x9d
#define COMENT_DOSSEG 0x9e
#define COMENT_DEFLIB 0x9f
#define COMENT_OMFEXT 0xa0
#define COMENT_NEWOMF 0xa1
#define COMENT_LINKPASS 0xa2
#define COMENT_LIBMOD 0xa3 
#define COMENT_EXESTR 0xa4
#define COMENT_INCERR 0xa6
#define COMENT_NOPAD 0xa7
#define COMENT_WKEXT 0xa8
#define COMENT_LZEXT 0xa9
#define COMENT_PHARLAP 0xaa
#define COMENT_IBM386 0xb0
#define COMENT_RECORDER 0xb1
#define COMENT_COMMENT 0xda
#define COMENT_COMPILER 0xdb
#define COMENT_DATE 0xdc
#define COMENT_TIME 0xdd
#define COMENT_USER 0xdf
#define COMENT_DEPFILE 0xe9
#define COMENT_COMMANDLINE 0xff
#define COMENT_PUBTYPE 0xe1
#define COMENT_COMPARAM 0xea
#define COMENT_TYPDEF 0xe3
#define COMENT_STRUCTMEM 0xe2
#define COMENT_OPENSCOPE 0xe5
#define COMENT_LOCAL 0xe6
#define COMENT_ENDSCOPE 0xe7
#define COMENT_SOURCEFILE 0xe8
/* Directive to disassemblers           */
#define COMENT_DISASM_DIRECTIVE 0xfd
/* Linker directive                     */
#define COMENT_LINKER_DIRECTIVE 0xfe

#define EXT_IMPDEF 0x01
#define EXT_EXPDEF 0x02

#define SEG_ALIGN 0x3e0
#define SEG_COMBINE 0x1c
#define SEG_BIG 0x02
#define SEG_USE32 0x01

#define SEG_ABS 0x00
#define SEG_BYTE 0x20
#define SEG_WORD 0x40
#define SEG_PARA 0x60
#define SEG_PAGE 0x80
#define SEG_DWORD 0xa0
#define SEG_MEMPAGE 0xc0
#define SEG_BADALIGN 0xe0
#define SEG_8BYTE 0x100
#define SEG_32BYTE 0x200
#define SEG_64BYTE 0x300

#define SEG_PRIVATE 0x00
#define SEG_PUBLIC 0x08
#define SEG_PUBLIC2 0x10
#define SEG_STACK 0x14
#define SEG_COMMON 0x18
#define SEG_PUBLIC3 0x1c

#define REL_SEGDISP 0
#define REL_EXTDISP 2
#define REL_GRPDISP 1
#define REL_EXPFRAME 3
#define REL_SEGONLY 4
#define REL_EXTONLY 6
#define REL_GRPONLY 5

#define REL_SEGFRAME 0
#define REL_GRPFRAME 1
#define REL_EXTFRAME 2
#define REL_LILEFRAME 4
#define REL_TARGETFRAME 5

#define FIX_SELFREL 0x10
#define FIX_MASK (0x0f+FIX_SELFREL)

#define FIX_THRED 0x08
#define THRED_MASK 0x07

#define FIX_LBYTE 0
#define FIX_OFS16 1
#define FIX_BASE 2
#define FIX_PTR1616 3
#define FIX_HBYTE 4
#define FIX_OFS16_2 5
#define FIX_OFS32 9
#define FIX_PTR1632 11
#define FIX_OFS32_2 13
/* RVA32 fixups are not supported by OMF, so has an out-of-range number */
#define FIX_RVA32 256

#define FIX_SELF_LBYTE (FIX_LBYTE+FIX_SELFREL)
#define FIX_SELF_OFS16 (FIX_OFS16+FIX_SELFREL)
#define FIX_SELF_OFS16_2 (FIX_OFS16_2+FIX_SELFREL)
#define FIX_SELF_OFS32 (FIX_OFS32+FIX_SELFREL)
#define FIX_SELF_OFS32_2 (FIX_OFS32_2+FIX_SELFREL)

#define LIBF_CASESENSITIVE 1

#define EXT_NOMATCH       0
#define EXT_MATCHEDPUBLIC 1
#define EXT_MATCHEDIMPORT 2

#define PE_SIGNATURE      0x00
#define PE_MACHINEID      0x04
#define PE_NUMOBJECTS     0x06
#define PE_DATESTAMP      0x08
#define PE_SYMBOLPTR      0x0c
#define PE_NUMSYMBOLS     0x10
#define PE_HDRSIZE        0x14
#define PE_FLAGS          0x16
#define PE_MAGIC          0x18
#define PE_LMAJOR         0x1a
#define PE_LMINOR         0x1b
#define PE_CODESIZE       0x1c
#define PE_INITDATASIZE   0x20
#define PE_UNINITDATASIZE 0x24
#define PE_ENTRYPOINT     0x28
#define PE_CODEBASE       0x2c
#define PE_DATABASE       0x30
#define PE_IMAGEBASE      0x34
#define PE_OBJECTALIGN    0x38
#define PE_FILEALIGN      0x3c
#define PE_OSMAJOR        0x40
#define PE_OSMINOR        0x42
#define PE_USERMAJOR      0x44
#define PE_USERMINOR      0x46
#define PE_SUBSYSMAJOR    0x48
#define PE_SUBSYSMINOR    0x4a
#define PE_IMAGESIZE      0x50
#define PE_HEADERSIZE     0x54
#define PE_CHECKSUM       0x58
#define PE_SUBSYSTEM      0x5c
#define PE_DLLFLAGS       0x5e
#define PE_STACKSIZE      0x60
#define PE_STACKCOMMSIZE  0x64
#define PE_HEAPSIZE       0x68
#define PE_HEAPCOMMSIZE   0x6c
#define PE_LOADERFLAGS    0x70
#define PE_NUMRVAS        0x74
#define PE_EXPORTRVA      0x78
#define PE_EXPORTSIZE     0x7c
#define PE_IMPORTRVA      0x80
#define PE_IMPORTSIZE     0x84
#define PE_RESOURCERVA    0x88
#define PE_RESOURCESIZE   0x8c
#define PE_EXCEPTIONRVA   0x90
#define PE_EXCEPTIONSIZE  0x94
#define PE_SECURITYRVA    0x98
#define PE_SECURITYSIZE   0x9c
#define PE_FIXUPRVA       0xa0
#define PE_FIXUPSIZE      0xa4
#define PE_DEBUGRVA       0xa8
#define PE_DEBUGSIZE      0xac
#define PE_DESCRVA        0xb0
#define PE_DESCSIZE       0xb4
#define PE_MSPECRVA       0xb8
#define PE_MSPECSIZE      0xbc
#define PE_TLSRVA         0xc0
#define PE_TLSSIZE        0xc4
#define PE_LOADCONFIGRVA  0xc8
#define PE_LOADCONFIGSIZE 0xcc
#define PE_BOUNDIMPRVA    0xd0
#define PE_BOUNDIMPSIZE   0xd4
#define PE_IATRVA         0xd8
#define PE_IATSIZE        0xdc

#define PE_OBJECT_NAME     0x00
#define PE_OBJECT_VIRTSIZE 0x08
#define PE_OBJECT_VIRTADDR 0x0c
#define PE_OBJECT_RAWSIZE  0x10
#define PE_OBJECT_RAWPTR   0x14
#define PE_OBJECT_RELPTR   0x18
#define PE_OBJECT_LINEPTR  0x1c
#define PE_OBJECT_NUMREL   0x20
#define PE_OBJECT_NUMLINE  0x22
#define PE_OBJECT_FLAGS    0x24

#define PE_BASE_HEADER_SIZE     0x18
#define PE_OPTIONAL_HEADER_SIZE 0xe0
#define PE_OBJECTENTRY_SIZE     0x28
#define PE_HEADBUF_SIZE         (PE_BASE_HEADER_SIZE+PE_OPTIONAL_HEADER_SIZE)
#define PE_IMPORTDIRENTRY_SIZE  0x14
#define PE_NUM_VAS              0x10
#define PE_EXPORTHEADER_SIZE    0x28
#define PE_RESENTRY_SIZE        0x08
#define PE_RESDIR_SIZE          0x10
#define PE_RESDATAENTRY_SIZE    0x10
#define PE_SYMBOL_SIZE          0x12
#define PE_RELOC_SIZE           0x0a

#define PE_ORDINAL_FLAG    0x80000000
#define PE_INTEL386        0x014c
#define PE_MAGICNUM        0x010b
#define PE_FILE_EXECUTABLE 0x0002
#define PE_FILE_32BIT      0x0100
#define PE_FILE_LIBRARY    0x2000

#define PE_REL_LOW16 0x2000
#define PE_REL_OFS32 0x3000

#define PE_SUBSYS_NATIVE  1
#define PE_SUBSYS_WINDOWS 2
#define PE_SUBSYS_CONSOLE 3
#define PE_SUBSYS_POSIX   7

#define WINF_UNDEFINED   0x00000000
#define WINF_CODE        0x00000020
#define WINF_INITDATA    0x00000040
#define WINF_UNINITDATA  0x00000080
#define WINF_DISCARDABLE 0x02000000
#define WINF_NOPAGE      0x08000000
#define WINF_SHARED      0x10000000
#define WINF_EXECUTE     0x20000000
#define WINF_READABLE    0x40000000
#define WINF_WRITEABLE   0x80000000
#define WINF_ALIGN_NOPAD 0x00000008
#define WINF_ALIGN_BYTE  0x00100000
#define WINF_ALIGN_WORD  0x00200000
#define WINF_ALIGN_DWORD 0x00300000
#define WINF_ALIGN_8     0x00400000
#define WINF_ALIGN_PARA  0x00500000
#define WINF_ALIGN_32    0x00600000
#define WINF_ALIGN_64    0x00700000
#define WINF_ALIGN       (WINF_ALIGN_64)
#define WINF_COMMENT     0x00000200
#define WINF_REMOVE      0x00000800
#define WINF_COMDAT      0x00001000
#define WINF_NEG_FLAGS   (WINF_DISCARDABLE | WINF_NOPAGE)
#define WINF_IMAGE_FLAGS 0xfa0008e0

#define COFF_SYM_EXTERNAL 2
#define COFF_SYM_STATIC   3
#define COFF_SYM_LABEL    6
#define COFF_SYM_FUNCTION 101
#define COFF_SYM_FILE     103
#define COFF_SYM_SECTION  104

#define COFF_FIX_DIR32    6
#define COFF_FIX_RVA32    7
#define COFF_FIX_REL32    0x14

#define OUTPUT_COM 1
#define OUTPUT_EXE 2
#define OUTPUT_PE  3

#define WIN32_DEFAULT_BASE              0x00400000
#define WIN32_DEFAULT_FILEALIGN         0x00000200
#define WIN32_DEFAULT_OBJECTALIGN       0x00001000
#define WIN32_DEFAULT_STACKSIZE         0x00100000
#define WIN32_DEFAULT_STACKCOMMITSIZE   0x00001000
#define WIN32_DEFAULT_HEAPSIZE          0x00100000
#define WIN32_DEFAULT_HEAPCOMMITSIZE    0x00001000
#define WIN32_DEFAULT_SUBSYS            PE_SUBSYS_WINDOWS
#define WIN32_DEFAULT_SUBSYSMAJOR       4
#define WIN32_DEFAULT_SUBSYSMINOR       0
#define WIN32_DEFAULT_OSMAJOR           1
#define WIN32_DEFAULT_OSMINOR           0

#define EXP_ORD 0x80

typedef unsigned char BOOL;
typedef unsigned char BYTE;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef unsigned long UINT;
typedef unsigned long ULONG;
typedef char* PCHAR, ** PPCHAR;
typedef unsigned char* PUCHAR;

typedef struct __sortentry
{
	char* id;
	void** object;
	UINT count;
} SORTENTRY, * PSORTENTRY;

typedef struct __seg {
	long name_index;
	long class_index;
	long overlay_index;
	long order_index;
	UINT length;
	UINT virtual_size;
	UINT absolute_frame;
	UINT absolute_offset;
	UINT base;
	UINT win_flags;
	USHORT attributes;
	PUCHAR data;
	PUCHAR data_mask;
} SEG, * PSEG, ** PPSEG;

typedef struct __datablock {
	long count;
	long blocks;
	long data_offset;
	void* data;
} DATABLOCK, * PDATABLOCK, ** PPDATABLOCK;

typedef struct __pubdef {
	long segment;
	long group;
	long type;
	UINT offset;
	UINT mod;
	PCHAR alias;
} PUBLIC, * PPUBLIC, ** PPPUBLIC;

typedef struct __extdef {
	PCHAR name;
	long type;
	PPUBLIC pubdef;
	long import;
	long flags;
	UINT mod;
} EXTREC, * PEXTREC, ** PPEXTREC;

typedef struct __imprec {
	PCHAR int_name;
	PCHAR mod_name;
	PCHAR imp_name;
	unsigned short ordinal;
	char flags;
	long segment;
	UINT offset;
} IMPREC, * PIMPREC, ** PPIMPREC;

typedef struct __exprec {
	PCHAR int_name;
	PCHAR exp_name;
	UINT ordinal;
	char flags;
	PPUBLIC pubdef;
	UINT modnum;
} EXPREC, * PEXPREC, ** PPEXPREC;

typedef struct __comdef {
	PCHAR name;
	UINT length;
	int is_far;
	UINT modnum;
} COMREC, * PCOMREC, ** PPCOMREC;

typedef struct __reloc {
	UINT offset;
	long segment;
	unsigned char ftype, ttype;
	unsigned short rtype;
	long target;
	UINT disp;
	long frame;
	UINT output_pos;
} RELOC, * PRELOC, ** PPRELOC;

typedef struct __grp {
	long name_index;
	long numsegs;
	long segindex[256];
	long segnum;
} GRP, * PGRP, ** PPGRP;

typedef struct __libfile {
	PCHAR file_name;
	unsigned short block_size;
	unsigned short num_dic_pages;
	UINT dic_start;
	char flags;
	char lib_type;
	int mods_loaded;
	UINT* mod_list;
	PUCHAR long_names;
	PSORTENTRY symbols;
	UINT num_syms;
} LIBFILE, * PLIBFILE, ** PPLIBFILE;

typedef struct __libentry {
	UINT lib_file;
	UINT mod_page;
} LIBENTRY, * PLIBENTRY, ** PPLIBENTRY;

typedef struct __resource {
	PUCHAR typename;
	PUCHAR name;
	PUCHAR data;
	UINT length;
	unsigned short typeid;
	unsigned short id;
	unsigned short language_id;
} RESOURCE, * PRESOURCE;

typedef struct __coffsym {
	PUCHAR name;
	UINT value;
	short section;
	unsigned short type;
	unsigned char class;
	long ext_num;
	UINT num_aux_recs;
	PUCHAR aux_recs;
	int is_com_dat;
} COFFSYM, * PCOFFSYM;

typedef struct __comdatrec
{
	UINT seg_num;
	UINT combine_type;
	UINT link_with;
} COMDATREC, * PCOMDAT;

int sort_compare(const void* x1, const void* x2);
void combine_groups(PCHAR fname, long i, long j);
void combine_common(PCHAR fname, long i, long j);
void combine_segments(PCHAR fname, long i, long j);
void combine_blocks(PCHAR fname);
BOOL output_win32_file(PCHAR outname);
BOOL output_exe_file(PCHAR outname);
BOOL output_com_file(PCHAR outname);
BOOL get_fixup_target(PCHAR fname, PRELOC r, long* tseg, UINT* tofs, int isFlat);
void load_lib_mod(UINT libnum, UINT modpage);
void load_lib(PCHAR libname, FILE* libfile);
void load_coff_lib(PCHAR libname, FILE* libfile);
void load_coff_lib_mod(PCHAR name, PLIBFILE p, FILE* libfile);
long load_mod(PCHAR name, FILE* objfile);
void load_resource(PCHAR name, FILE* resfile);
void load_coff(PCHAR name, FILE* objfile);
void load_coff_import(PCHAR name, FILE* objfile);
void load_fixup(PCHAR fname, PRELOC r, PUCHAR buf, long* p);
void reloc_lidata(PCHAR fname, PDATABLOCK p, long* ofs, PRELOC r);
void emit_lidata(PCHAR fname, PDATABLOCK p, long segnum, long* ofs);
PDATABLOCK build_lidata(long* bufofs);
void destroy_lidata(PDATABLOCK p);
void report_error(PCHAR fname, long errnum);
long get_index(PUCHAR buf, long* index);
void clear_n_bit(PUCHAR mask, long i);
void set_n_bit(PUCHAR mask, long i);
char get_n_bit(PUCHAR mask, long i);
int wstricmp(const char* s1, const char* s2);
int wstrlen(const char* s);
USHORT wtoupper(USHORT a);
int get_bit_count(UINT a);
void* check_malloc(size_t x);
void* check_realloc(void* p, size_t x);
char* check_strdup(const char* s);
PSORTENTRY binary_search(PSORTENTRY list, UINT count, char* key);
void sort_insert(PSORTENTRY* plist, UINT* pcount, char* key, void* object);

extern char case_sensitive;
extern char pad_segments;
extern char map_file;
extern PCHAR map_name;
extern unsigned short max_alloc;
extern int output_type;
extern PCHAR out_name;

extern FILE* a_file;
extern UINT file_position;
extern long record_length;
extern unsigned char record_type;
extern char li_le;
extern UINT previous_offset;
extern long previous_segment;
extern long got_start_address;
extern RELOC start_address;
extern UINT image_base;
extern UINT file_align;
extern UINT object_align;
extern UINT stack_size;
extern UINT stack_commit_size;
extern UINT heap_size;
extern UINT heap_commit_size;
extern unsigned char os_major, os_minor;
extern unsigned char sub_system_major, sub_system_minor;
extern unsigned int sub_system;

extern long error_count;

extern UCHAR buffer[0x10000];
extern PDATABLOCK lidata;
extern PPCHAR name_list;
extern PPSEG segment_list;
extern PPSEG out_list;
extern PPGRP group_list;
extern PSORTENTRY public_entries;
extern PEXTREC extern_records;
extern PPCOMREC common_definitions;
extern PPRELOC relocations;
extern PIMPREC import_definitions;
extern PEXPREC export_definitions;
extern PLIBFILE library_files;
extern PRESOURCE resource;
extern PPCHAR mod_name;
extern PPCHAR file_name;
extern PSORTENTRY comdat_entries;
extern UINT name_count, name_min,
pubcount, pubmin,
segcount, segmin, outcount,
segcount_combined,
grpcount, grpmin,
grpcount_combined,
extcount, extmin,
comcount, commin,
fixcount, fixmin,
impcount, impmin, impsreq,
expcount, expmin,
nummods,
filecount,
libcount,
rescount;

extern int build_dll;
extern PUCHAR stub_name;
