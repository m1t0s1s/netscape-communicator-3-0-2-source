
#ifndef CVDISK_H
#define CVDISK_H

extern NET_StreamClass* fe_MakeSaveAsStream (FO_Present_Types format_out,
                                                 void       *data_obj,
                                                 URL_Struct *URL_s,
                                                 MWContext  *window_id);

#endif /* CVDISK_H */
