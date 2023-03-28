/* -*- Mode: C; tab-width: 4 -*-
**
**	Mac_Convert_Stream.h
**	--------------------
**
**	The header file for the constructing functions of apple file 
**		encode/decode stream.
**
**		27sep95		mym		created.
*/

#ifndef	M_CVStrm_h
#define M_CVStrm_h

XP_BEGIN_PROTOS

PUBLIC NET_StreamClass * 
fe_MakeBinHexEncodeStream (int  format_out,
                         void       *data_obj,
                         URL_Struct *URL_s,
                         MWContext  *window_id,
                         char*		dst_filename);
                         
PUBLIC NET_StreamClass * 
fe_MakeBinHexDecodeStream (int  format_out,
                         void       *data_obj,
                         URL_Struct *URL_s,
                         MWContext  *window_id );

PUBLIC NET_StreamClass * 
fe_MakeAppleDoubleDecodeStream (int  format_out,
                         void       *data_obj,
                         URL_Struct *URL_s,
                         MWContext  *window_id,
                         XP_Bool	write_as_binhex,
                         char 		*dst_filename);
                         
PUBLIC NET_StreamClass * 
fe_MakeAppleSingleDecodeStream (int  format_out,
                         void       *data_obj,
                         URL_Struct *URL_s,
                         MWContext  *window_id,
                         XP_Bool	write_as_binhex,
                         char 		*dst_filename);

#ifdef XP_MAC

PUBLIC NET_StreamClass * 
fe_MakeAppleDoubleEncodeStream (int  format_out,
                         void       *data_obj,
                         URL_Struct *URL_s,
                         MWContext  *window_id,
                         char*		src_filename,
                         char*		dst_filename,
                         char*		separator);

XP_Bool	isMacFile(char* filename);

#endif

PUBLIC NET_StreamClass * 
fe_MakeAppleDoubleDecodeStream_1 (int  format_out,
						void       *data_obj,
                        URL_Struct *URL_s,
                        MWContext  *window_id);

PUBLIC NET_StreamClass * 
fe_MakeAppleSingleDecodeStream_1 (int  format_out,
						void       *data_obj,
                        URL_Struct *URL_s,
                        MWContext  *window_id);

XP_END_PROTOS

#endif
