#include <stdio.h>
#include "xp.h"
#include "pa_parse.h"

typedef struct _FileHolder {
    char * filename;
    FILE * fp;
} FileHolder;

PRIVATE FileHolder file_list[1000] = {0,0};


intn
LO_Format(void * data_object, PA_Tag *tag, intn status)
{
	int cnt;
        int doc_id;
	FILE *out;
        pa_DocData *doc_data;

        doc_data = (pa_DocData *)data_object;
        doc_id = doc_data->doc_id;

	if(!file_list[doc_id].fp)
	  {
	    char  buffer[500];
            XP_SPRINTF(buffer,"footest/foo%d.html",doc_id);
	    file_list[doc_id].filename = strdup(buffer);
	    file_list[doc_id].fp = fopen(buffer,"w");
	  }

	out = file_list[doc_id].fp;

	if ((tag == NULL)&&(status == PA_PARSED))
	{
		fprintf(out, "<NULL>");
		return(1);
	}
	else if (tag != NULL)
	{
		if (tag->type == P_TEXT)
		{
			fprintf(out, "%s", tag->values[0]);
		}
		else
		{
		 	/* char *token_text; */

			fprintf(out, "{");
			if (tag->is_end == TRUE)
			{
				fprintf(out, "/");
			}
			/* token_text = pa_PrintTagToken(tag->type);
			 * fprintf(out, "%s", token_text);
			 * free(token_text);
			 */
			fprintf(out, "%d", tag->type);

			cnt = 0;
			while(tag->params[cnt] != NULL)
			{
				fprintf(out, " %s", tag->params[cnt]);
				if (tag->values[cnt] != NULL)
				{
					fprintf(out, "=\"%s\"",
						tag->values[cnt]);
				}
				cnt++;
			}

			fprintf(out, "}");
		}
	}
	if ((status == PA_COMPLETE)||(status == PA_ABORT))
	{
	    /* done */
	    char buffer[500];
	    fclose(file_list[doc_id].fp);
	    XP_SPRINTF(buffer, "xv %s&", file_list[doc_id].filename);
	    /* system(buffer);
	     */
	    free(file_list[doc_id].filename);
	}
}

