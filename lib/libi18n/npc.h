#ifndef _NPC_H_
#define _NPC_H_


#define CSIDToNPC(csid)	((unsigned char) (((csid) & 0x3f) | 0x80))


#define MAXCSIDINNPC	64

#define NPCToCSID(fb)	(npctocsid[(fb) & (MAXCSIDINNPC - 1)])


extern int16 npctocsid[];


#endif /* _NPC_H_ */
