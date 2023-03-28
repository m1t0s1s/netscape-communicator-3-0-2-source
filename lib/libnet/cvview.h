
#ifndef CVVIEW_H
#define CVVIEW_H

extern NET_StreamClass* NET_ExtViewerConverter ( FO_Present_Types format_out,
                                                 void       *data_obj,
                                                 URL_Struct *URL_s,
                                                 MWContext  *window_id);

typedef struct _CV_ExtViewStruct {
   char    * system_command;
   unsigned int stream_block_size;
} CV_ExtViewStruct;

#endif /* CVVIEW_H */
