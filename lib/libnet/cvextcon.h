
#ifndef CVEXTCON_H
#define CVEXTCON_H

typedef struct _CV_ExtConverterStruct {
	char * command;
	char * new_format;
    Bool   is_encoding_converter;
} CV_ExtConverterStruct;

extern NET_StreamClass * 
NET_ExtConverterConverter(FO_Present_Types format_out,
                          void       *data_obj,
                          URL_Struct *URL_s,
                          MWContext  *window_id);

#endif /* CVEXTCON_H */
