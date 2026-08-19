/*
 * jpegint.h
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * Modified 1997-2019 by Guido Vollbeding.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file provides common declarations for the various JPEG modules.
 * These declarations are considered internal to the JPEG library; most
 * applications using the library shouldn't need to include this file.
 */


/* Declarations for both compression & decompression */

typedef enum {			/* Operating modes for buffer controllers */
	JBUF_PASS_THRU,		/* Plain stripwise operation */
	/* Remaining modes require a full-image buffer to have been created */
	JBUF_SAVE_SOURCE,	/* Run source subobject only, save output */
	JBUF_CRANK_DEST,	/* Run dest subobject only, using saved data */
	JBUF_SAVE_AND_PASS	/* Run both subobjects, save output */
} J_BUF_MODE;

/* Values of global_state field (jdapi.c has some dependencies on ordering!) */
#define CSTATE_START	100	/* after create_compress */
#define CSTATE_SCANNING	101	/* start_compress done, write_scanlines OK */
#define CSTATE_RAW_OK	102	/* start_compress done, write_raw_data OK */
#define CSTATE_WRCOEFS	103	/* jpeg_write_coefficients done */
#define DSTATE_START	200	/* after create_decompress */
#define DSTATE_INHEADER	201	/* reading header markers, no SOS yet */
#define DSTATE_READY	202	/* found SOS, ready for start_decompress */
#define DSTATE_PRELOAD	203	/* reading multiscan file in start_decompress*/
#define DSTATE_PRESCAN	204	/* performing dummy pass for 2-pass quant */
#define DSTATE_SCANNING	205	/* start_decompress done, read_scanlines OK */
#define DSTATE_RAW_OK	206	/* start_decompress done, read_raw_data OK */
#define DSTATE_BUFIMAGE	207	/* expecting jpeg_start_output */
#define DSTATE_BUFPOST	208	/* looking for SOS/EOI in jpeg_finish_output */
#define DSTATE_RDCOEFS	209	/* reading file in jpeg_read_coefficients */
#define DSTATE_STOPPING	210	/* looking for EOI in jpeg_finish_decompress */


/* Declarations for compression modules */

/* Master control module */
struct jpeg_comp_master {
  JMETHOD(void, prepare_for_pass, (jpeg_compress_struct* cinfo));
  JMETHOD(void, pass_startup, (jpeg_compress_struct* cinfo));
  JMETHOD(void, finish_pass, (jpeg_compress_struct* cinfo));

  /* State variables made visible to other modules */
  boolean call_pass_startup;	/* True if pass_startup must be called */
  boolean is_last_pass;		/* True during last pass */
};

/* Main buffer control (downsampled-data buffer) */
struct jpeg_c_main_controller {
  JMETHOD(void, start_pass, (jpeg_compress_struct* cinfo, J_BUF_MODE pass_mode));
  JMETHOD(void, process_data, (jpeg_compress_struct* cinfo,
			       JSAMPARRAY input_buf, JDIMENSION *in_row_ctr,
			       JDIMENSION in_rows_avail));
};

/* Compression preprocessing (downsampling input buffer control) */
struct jpeg_c_prep_controller {
  JMETHOD(void, start_pass, (jpeg_compress_struct* cinfo, J_BUF_MODE pass_mode));
  JMETHOD(void, pre_process_data, (jpeg_compress_struct* cinfo,
				   JSAMPARRAY input_buf,
				   JDIMENSION *in_row_ctr,
				   JDIMENSION in_rows_avail,
				   JSAMPIMAGE output_buf,
				   JDIMENSION *out_row_group_ctr,
				   JDIMENSION out_row_groups_avail));
};

/* Coefficient buffer control */
struct jpeg_c_coef_controller {
  JMETHOD(void, start_pass, (jpeg_compress_struct* cinfo, J_BUF_MODE pass_mode));
  JMETHOD(boolean, compress_data, (jpeg_compress_struct* cinfo,
				   JSAMPIMAGE input_buf));
};

/* Colorspace conversion */
struct jpeg_color_converter {
  JMETHOD(void, start_pass, (jpeg_compress_struct* cinfo));
  JMETHOD(void, color_convert, (jpeg_compress_struct* cinfo,
				JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
				JDIMENSION output_row, int num_rows));
};

/* Downsampling */
struct jpeg_downsampler {
  JMETHOD(void, start_pass, (jpeg_compress_struct* cinfo));
  JMETHOD(void, downsample, (jpeg_compress_struct* cinfo,
			     JSAMPIMAGE input_buf, JDIMENSION in_row_index,
			     JSAMPIMAGE output_buf,
			     JDIMENSION out_row_group_index));

  boolean need_context_rows;	/* TRUE if need rows above & below */
};

/* Forward DCT (also controls coefficient quantization) */
typedef JMETHOD(void, forward_DCT_ptr,
		(jpeg_compress_struct* cinfo, jpeg_component_info * compptr,
		 JSAMPARRAY sample_data, JBLOCKROW coef_blocks,
		 JDIMENSION start_row, JDIMENSION start_col,
		 JDIMENSION num_blocks));

struct jpeg_forward_dct {
  JMETHOD(void, start_pass, (jpeg_compress_struct* cinfo));
  /* It is useful to allow each component to have a separate FDCT method. */
  forward_DCT_ptr forward_DCT[MAX_COMPONENTS];
};

/* Entropy encoding */
struct jpeg_entropy_encoder {
  JMETHOD(void, start_pass, (jpeg_compress_struct* cinfo, boolean gather_statistics));
  JMETHOD(boolean, encode_mcu, (jpeg_compress_struct* cinfo, JBLOCKROW *MCU_data));
  JMETHOD(void, finish_pass, (jpeg_compress_struct* cinfo));
};

/* Marker writing */
struct jpeg_marker_writer {
  JMETHOD(void, write_file_header, (jpeg_compress_struct* cinfo));
  JMETHOD(void, write_frame_header, (jpeg_compress_struct* cinfo));
  JMETHOD(void, write_scan_header, (jpeg_compress_struct* cinfo));
  JMETHOD(void, write_file_trailer, (jpeg_compress_struct* cinfo));
  JMETHOD(void, write_tables_only, (jpeg_compress_struct* cinfo));
  /* These routines are exported to allow insertion of extra markers */
  /* Probably only COM and APPn markers should be written this way */
  JMETHOD(void, write_marker_header, (jpeg_compress_struct* cinfo, int marker,
				      unsigned int datalen));
  JMETHOD(void, write_marker_byte, (jpeg_compress_struct* cinfo, int val));
};


/* Declarations for decompression modules */

/* Master control module */
struct jpeg_decomp_master {
  JMETHOD(void, prepare_for_output_pass, (jpeg_decompress_struct* cinfo));
  JMETHOD(void, finish_output_pass, (jpeg_decompress_struct* cinfo));

  /* State variables made visible to other modules */
  boolean is_dummy_pass;	/* True during 1st pass for 2-pass quant */
};

/* Input control module */
struct jpeg_input_controller {
  JMETHOD(int, consume_input, (jpeg_decompress_struct* cinfo));
  JMETHOD(void, reset_input_controller, (jpeg_decompress_struct* cinfo));
  JMETHOD(void, start_input_pass, (jpeg_decompress_struct* cinfo));
  JMETHOD(void, finish_input_pass, (jpeg_decompress_struct* cinfo));

  /* State variables made visible to other modules */
  boolean has_multiple_scans;	/* True if file has multiple scans */
  boolean eoi_reached;		/* True when EOI has been consumed */
};

/* Main buffer control (downsampled-data buffer) */
struct jpeg_d_main_controller {
  JMETHOD(void, start_pass, (jpeg_decompress_struct* cinfo, J_BUF_MODE pass_mode));
  JMETHOD(void, process_data, (jpeg_decompress_struct* cinfo,
			       JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
			       JDIMENSION out_rows_avail));
};

/* Coefficient buffer control */
struct jpeg_d_coef_controller {
  JMETHOD(void, start_input_pass, (jpeg_decompress_struct* cinfo));
  JMETHOD(int, consume_data, (jpeg_decompress_struct* cinfo));
  JMETHOD(void, start_output_pass, (jpeg_decompress_struct* cinfo));
  JMETHOD(int, decompress_data, (jpeg_decompress_struct* cinfo,
				 JSAMPIMAGE output_buf));
  /* Pointer to array of coefficient virtual arrays, or NULL if none */
  jvirt_barray_ptr *coef_arrays;
};

/* Decompression postprocessing (color quantization buffer control) */
struct jpeg_d_post_controller {
  JMETHOD(void, start_pass, (jpeg_decompress_struct* cinfo, J_BUF_MODE pass_mode));
  JMETHOD(void, post_process_data, (jpeg_decompress_struct* cinfo,
				    JSAMPIMAGE input_buf,
				    JDIMENSION *in_row_group_ctr,
				    JDIMENSION in_row_groups_avail,
				    JSAMPARRAY output_buf,
				    JDIMENSION *out_row_ctr,
				    JDIMENSION out_rows_avail));
};

/* Marker reading & parsing */
struct jpeg_marker_reader {
  JMETHOD(void, reset_marker_reader, (jpeg_decompress_struct* cinfo));
  /* Read markers until SOS or EOI.
   * Returns same codes as are defined for jpeg_consume_input:
   * JPEG_SUSPENDED, JPEG_REACHED_SOS, or JPEG_REACHED_EOI.
   */
  JMETHOD(int, read_markers, (jpeg_decompress_struct* cinfo));
  /* Read a restart marker --- exported for use by entropy decoder only */
  jpeg_marker_parser_method read_restart_marker;

  /* State of marker reader --- nominally internal, but applications
   * supplying COM or APPn handlers might like to know the state.
   */
  boolean saw_SOI;		/* found SOI? */
  boolean saw_SOF;		/* found SOF? */
  int next_restart_num;		/* next restart number expected (0-7) */
  unsigned int discarded_bytes;	/* # of bytes skipped looking for a marker */
};

/* Entropy decoding */
struct jpeg_entropy_decoder {
  JMETHOD(void, start_pass, (jpeg_decompress_struct* cinfo));
  JMETHOD(boolean, decode_mcu, (jpeg_decompress_struct* cinfo, JBLOCKROW *MCU_data));
  JMETHOD(void, finish_pass, (jpeg_decompress_struct* cinfo));
};

/* Inverse DCT (also performs dequantization) */
typedef JMETHOD(void, inverse_DCT_method_ptr,
		(jpeg_decompress_struct* cinfo, jpeg_component_info * compptr,
		 JCOEFPTR coef_block,
		 JSAMPARRAY output_buf, JDIMENSION output_col));

struct jpeg_inverse_dct {
  JMETHOD(void, start_pass, (jpeg_decompress_struct* cinfo));
  /* It is useful to allow each component to have a separate IDCT method. */
  inverse_DCT_method_ptr inverse_DCT[MAX_COMPONENTS];
};

/* Upsampling (note that upsampler must also call color converter) */
struct jpeg_upsampler {
  JMETHOD(void, start_pass, (jpeg_decompress_struct* cinfo));
  JMETHOD(void, upsample, (jpeg_decompress_struct* cinfo,
			   JSAMPIMAGE input_buf,
			   JDIMENSION *in_row_group_ctr,
			   JDIMENSION in_row_groups_avail,
			   JSAMPARRAY output_buf,
			   JDIMENSION *out_row_ctr,
			   JDIMENSION out_rows_avail));

  boolean need_context_rows;	/* TRUE if need rows above & below */
};

/* Colorspace conversion */
struct jpeg_color_deconverter {
  JMETHOD(void, start_pass, (jpeg_decompress_struct* cinfo));
  JMETHOD(void, color_convert, (jpeg_decompress_struct* cinfo,
				JSAMPIMAGE input_buf, JDIMENSION input_row,
				JSAMPARRAY output_buf, int num_rows));
};

/* Color quantization or color precision reduction */
struct jpeg_color_quantizer {
  JMETHOD(void, start_pass, (jpeg_decompress_struct* cinfo, boolean is_pre_scan));
  JMETHOD(void, color_quantize, (jpeg_decompress_struct* cinfo,
				 JSAMPARRAY input_buf, JSAMPARRAY output_buf,
				 int num_rows));
  JMETHOD(void, finish_pass, (jpeg_decompress_struct* cinfo));
  JMETHOD(void, new_color_map, (jpeg_decompress_struct* cinfo));
};


/* Definition of range extension bits for decompression processes.
 * See the comments with prepare_range_limit_table (in jdmaster.c)
 * for more info.
 * The recommended default value for normal applications is 2.
 * Applications with special requirements may use a different value.
 * For example, Ghostscript wants to use 3 for proper handling of
 * wacky images with oversize coefficient values.
 */

#define RANGE_BITS	2
#define RANGE_CENTER	(CENTERJSAMPLE << RANGE_BITS)


/* Miscellaneous useful macros */

#undef MAX
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))


/* We assume that right shift corresponds to signed division by 2 with
 * rounding towards minus infinity.  This is correct for typical "arithmetic
 * shift" instructions that shift in copies of the sign bit.  But some
 * C compilers implement >> with an unsigned shift.  For these machines you
 * must define RIGHT_SHIFT_IS_UNSIGNED.
 * RIGHT_SHIFT provides a proper signed right shift of an INT32 quantity.
 * It is only applied with constant shift counts.  SHIFT_TEMPS must be
 * included in the variables of any routine using RIGHT_SHIFT.
 */

#ifdef RIGHT_SHIFT_IS_UNSIGNED
#define SHIFT_TEMPS	INT32 shift_temp;
#define RIGHT_SHIFT(x,shft)  \
	((shift_temp = (x)) < 0 ? \
	 (shift_temp >> (shft)) | ((~((INT32) 0)) << (32-(shft))) : \
	 (shift_temp >> (shft)))
#else
#define SHIFT_TEMPS
#define RIGHT_SHIFT(x,shft)	((x) >> (shft))
#endif

/* Descale and correctly round an INT32 value that's scaled by N bits.
 * We assume RIGHT_SHIFT rounds towards minus infinity, so adding
 * the fudge factor is correct for either sign of X.
 */

#define DESCALE(x,n)	RIGHT_SHIFT((x) + ((INT32) 1 << ((n)-1)), n)


/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED_SHORT_EXTERNAL_NAMES
#define jinit_compress_master	jICompress
#define jinit_c_master_control	jICMaster
#define jinit_c_main_controller	jICMainC
#define jinit_c_prep_controller	jICPrepC
#define jinit_c_coef_controller	jICCoefC
#define jinit_color_converter	jICColor
#define jinit_downsampler	jIDownsampler
#define jinit_forward_dct	jIFDCT
#define jinit_huff_encoder	jIHEncoder
#define jinit_arith_encoder	jIAEncoder
#define jinit_marker_writer	jIMWriter
#define jinit_master_decompress	jIDMaster
#define jinit_d_main_controller	jIDMainC
#define jinit_d_coef_controller	jIDCoefC
#define jinit_d_post_controller	jIDPostC
#define jinit_input_controller	jIInCtlr
#define jinit_marker_reader	jIMReader
#define jinit_huff_decoder	jIHDecoder
#define jinit_arith_decoder	jIADecoder
#define jinit_inverse_dct	jIIDCT
#define jinit_upsampler		jIUpsampler
#define jinit_color_deconverter	jIDColor
#define jinit_1pass_quantizer	jI1Quant
#define jinit_2pass_quantizer	jI2Quant
#define jinit_merged_upsampler	jIMUpsampler
#define jinit_memory_mgr	jIMemMgr
#define jdiv_round_up		jDivRound
#define jround_up		jRound
#define jzero_far		jZeroFar
#define jcopy_sample_rows	jCopySamples
#define jcopy_block_row		jCopyBlocks
#define jpeg_zigzag_order	jZIGTable
#define jpeg_natural_order	jZAGTable
#define jpeg_natural_order7	jZAG7Table
#define jpeg_natural_order6	jZAG6Table
#define jpeg_natural_order5	jZAG5Table
#define jpeg_natural_order4	jZAG4Table
#define jpeg_natural_order3	jZAG3Table
#define jpeg_natural_order2	jZAG2Table
#define jpeg_aritab		jAriTab
#endif /* NEED_SHORT_EXTERNAL_NAMES */


/* On normal machines we can apply MEMCOPY() and MEMZERO() to sample arrays
 * and coefficient-block arrays.  This won't work on 80x86 because the arrays
 * are FAR and we're assuming a small-pointer memory model.  However, some
 * DOS compilers provide far-pointer versions of memcpy() and memset() even
 * in the small-model libraries.  These will be used if USE_FMEM is defined.
 * Otherwise, the routines in jutils.c do it the hard way.
 */

#ifndef NEED_FAR_POINTERS	/* normal case, same as regular macro */
#define FMEMZERO(target,size)	MEMZERO(target,size)
#else				/* 80x86 case */
#ifdef USE_FMEM
#define FMEMZERO(target,size)	_fmemset((void FAR *)(target), 0, (size_t)(size))
#else
extern void jzero_far JPP((void FAR * target, size_t bytestozero));
#define FMEMZERO(target,size)	jzero_far(target, size)
#endif
#endif


/* Compression module initialization routines */
extern void jinit_compress_master (jpeg_compress_struct* cinfo);
extern void jinit_c_master_control (jpeg_compress_struct* cinfo,
					 boolean transcode_only);
extern void jinit_c_main_controller (jpeg_compress_struct* cinfo,
					  boolean need_full_buffer);
extern void jinit_c_prep_controller (jpeg_compress_struct* cinfo,
					  boolean need_full_buffer);
extern void jinit_c_coef_controller (jpeg_compress_struct* cinfo,
					  boolean need_full_buffer);
extern void jinit_color_converter (jpeg_compress_struct* cinfo);
extern void jinit_downsampler (jpeg_compress_struct* cinfo);
extern void jinit_forward_dct (jpeg_compress_struct* cinfo);
extern void jinit_huff_encoder (jpeg_compress_struct* cinfo);
extern void jinit_arith_encoder (jpeg_compress_struct* cinfo);
extern void jinit_marker_writer (jpeg_compress_struct* cinfo);
/* Decompression module initialization routines */
extern void jinit_master_decompress (jpeg_decompress_struct* cinfo);
extern void jinit_d_main_controller (jpeg_decompress_struct* cinfo,
					  boolean need_full_buffer);
extern void jinit_d_coef_controller (jpeg_decompress_struct* cinfo,
					  boolean need_full_buffer);
extern void jinit_d_post_controller (jpeg_decompress_struct* cinfo,
					  boolean need_full_buffer);
extern void jinit_input_controller (jpeg_decompress_struct* cinfo);
extern void jinit_marker_reader (jpeg_decompress_struct* cinfo);
extern void jinit_huff_decoder (jpeg_decompress_struct* cinfo);
extern void jinit_arith_decoder (jpeg_decompress_struct* cinfo);
extern void jinit_inverse_dct (jpeg_decompress_struct* cinfo);
extern void jinit_upsampler (jpeg_decompress_struct* cinfo);
extern void jinit_color_deconverter (jpeg_decompress_struct* cinfo);
extern void jinit_1pass_quantizer (jpeg_decompress_struct* cinfo);
extern void jinit_2pass_quantizer (jpeg_decompress_struct* cinfo);
extern void jinit_merged_upsampler (jpeg_decompress_struct* cinfo);
/* Memory manager initialization */
extern void jinit_memory_mgr (jpeg_common_struct* cinfo);

/* Utility routines in jutils.c */
extern long jdiv_round_up (long a, long b);
extern long jround_up (long a, long b);
extern void jcopy_sample_rows (JSAMPARRAY input_array, int source_row,
				    JSAMPARRAY output_array, int dest_row,
				    int num_rows, JDIMENSION num_cols);
extern void jcopy_block_row (JBLOCKROW input_row, JBLOCKROW output_row,
				  JDIMENSION num_blocks);
/* Constant tables in jutils.c */
#if 0				/* This table is not actually needed in v6a */
extern const int jpeg_zigzag_order[]; /* natural coef order to zigzag order */
#endif
extern const int jpeg_natural_order[]; /* zigzag coef order to natural order */
extern const int jpeg_natural_order7[]; /* zz to natural order for 7x7 block */
extern const int jpeg_natural_order6[]; /* zz to natural order for 6x6 block */
extern const int jpeg_natural_order5[]; /* zz to natural order for 5x5 block */
extern const int jpeg_natural_order4[]; /* zz to natural order for 4x4 block */
extern const int jpeg_natural_order3[]; /* zz to natural order for 3x3 block */
extern const int jpeg_natural_order2[]; /* zz to natural order for 2x2 block */

/* Arithmetic coding probability estimation tables in jaricom.c */
extern const INT32 jpeg_aritab[];

/* Suppress undefined-structure complaints if necessary. */

#ifdef INCOMPLETE_TYPES_BROKEN
#ifndef AM_MEMORY_MANAGER	/* only jmemmgr.c defines these */
struct jvirt_sarray_control { long dummy; };
struct jvirt_barray_control { long dummy; };
#endif
#endif /* INCOMPLETE_TYPES_BROKEN */
