/*
 * the following header file switches between MACI and CI based on
 * compile options. That lest the rest of the source code operate
 * without change, even if it only suports CI_ calls, not MACI_ calls
 */
#ifndef _GENCI_H_
#define _GENCI_H_ 1

#ifdef XP_UNIX

/*
 * On unix we use full maci
 */
#include "maci.h"

#define MACI_SEL(x)

/*
 * for sec-for.c
 */
#define CI_Initialize	MACI_Initialize
#define CI_Terminate()  { HSESSION hs;\
			  MACI_GetSessionID(&hs);\
			  MACI_Terminate(hs); }

#else

/*
 * On Windows, NT, and Mac we use the original CI_LIB
 */
#include "cryptint.h"

/*
 * MACI specific values not defined for CI lib
 */
#define MACI_SESSION_EXCEEDED     (-53)

typedef int HSESSION;

/*
 * Map MACI_ calls to CI_ calls. NOTE: this assumes the proper CI_Select
 * calls are issued in the CI_ case
 */
#define MACI_ChangePIN(s,pin,old,new)		CI_ChangePin(pin,old,new)
#define MACI_CheckPIN(s,type,pin)   		CI_CheckPIN(type,pin)
#define MACI_Close(s,flag,socket)		CI_Close(flag,socket)
#define MACI_Decrypt(s,size,in,out)		CI_Decrypt(size,in,out)
#define MACI_DeleteCertificate(s,cert)		CI_DeleteCertificate(cert)
#define MACI_DeleteKey(s,index)			CI_DeleteKey(index)
#define MACI_Encrypt(s,size,in,out)		CI_Encrypt(size,in,out)
#define MACI_ExtractX(s,cert,type,pass,ySize,y,x,Ra,pgSize,qSize,p,q,g) \
		CI_ExtractX(cert,type,pass,ySize,y,x,Ra,pgSize,qSize,p,q,g)
#define MACI_FirmwareUpdate(s,flags,Cksum,len,size,data) \
			CI_FirmwareUpdate(flags,Cksum,len,size,data)
#define MACI_GenerateIV(s,iv)			CI_GenerateIV(iv)
#define MACI_GenerateMEK(s,index,res)		CI_GenerateMEK(index,res)
#define MACI_GenerateRa(s,Ra)			CI_GenerateRa(Ra)
#define MACI_GenerateRandom(s,ran)		CI_GenerateRandom(ran)
#define MACI_GenerateTEK(s,flag,index,Ra,Rb,size,Y) \
				CI_GenerateTEK(flag,index,Ra,Rb,size,Y)
#define MACI_GenerateX(s,cert,type,pgSize,qSize,p,q,g,ySize,y) \
			CI_GenerateX(cert,type,pgSize,qSize,p,q,g,ySize,y)
#define MACI_GetCertificate(s,cert,val)		CI_GetCertificate(cert,val)
#define MACI_GetConfiguration(s,config)		CI_GetConfiguration(config)
#define MACI_GetHash(s,size,data,val)		CI_GetHash(size,data,val)
#define MACI_GetPersonalityList(s,cnt,list)	CI_GetPersonalityList(cnt,list)
#define MACI_GetSessionID(s)			CI_OK
#define MACI_GetState(s,state)			CI_GetState(state)
#define MACI_GetStatus(s,status)		CI_GetStatus(status)
#define MACI_GetTime(s,time)			CI_GetTime(time)
#define MACI_Hash(s,size,data)			CI_Hash(size,data)
#define MACI_Initialize(count)			CI_Initialize(count)
#define MACI_InitializeHash(s)			CI_InitializeHash()
#define MACI_InstallX(s,cert,type,pass,ySize,y,x,Ra,pgSize,qSize,p,q,g) \
		CI_InstallX(cert,type,pass,ySize,y,x,Ra,pgSize,qSize,p,q,g)
#define MACI_LoadCertificate(s,cert,label,data,res) \
					CI_LoadCertificate(cert,label,data,res)
#define MACI_LoadDSAParameters(s,pgSize,qSize,p,q,g) \
				CI_LoadDSAParameters(pgSize,qSize,p,q,g)
#define MACI_LoadInitValues(s,seed,Ks)		CI_LoadInitValues(seed,Ks)
#define MACI_LoadIV(s,iv)			CI_LoadIV(iv)
#define MACI_LoadX(s,cert,type,pgSize,qSize,p,q,g,x,ySize,y) \
			CI_LoadX(cert,type,pgSize,qSize,p,q,g,x,ySize,y)
#define MACI_Lock(s,flags)			CI_Lock(flags)
#define MACI_Open(s,flags,index)		CI_Open(flags,index)
#define MACI_RelayX(s,oPass,oSize,oY,oRa,oX,nPass,nSize,nY,nRa,nX) \
			CI_RelayX(oPass,oSize,oY,oRa,oX,nPass,nSize,nY,nRa,nX)
#define MACI_Reset(s)				CI_Reset()
#define MACI_Restore(s,type,data)		CI_Restore(type,data)
#define MACI_Save(s,type,data)			CI_Save(type,data)
#define MACI_Select(s,socket)			CI_Select(socket)
#define MACI_SetConfiguration(s,typ,sz,d)	CI_SetConfiguration(typ,sz,d)
#define MACI_SetKey(s,key)			CI_SetKey(key)
#define MACI_SetMode(s,type,mode)		CI_SetMode(type,mode)
#define MACI_SetPersonality(s,index)		CI_SetPersonality(index)
#define MACI_SetTime(s,time)			CI_SetTime(time)
#define MACI_Sign(s,hash,sig)			CI_Sign(hash,sig)
#define MACI_Terminate(s)			CI_Terminate()
#define MACI_TimeStamp(s,val,sig,time)		CI_TimeStamp(val,sig,time)
#define MACI_Unlock(s)				CI_Unlock()
#define MACI_UnwrapKey(s,targ,wrap,key)		CI_UnwrapKey(targ,wrap,key)
#define MACI_VerifySignature(s,h,siz,y,sig)	CI_VerifySignature(h,siz,y,sig)
#define MACI_VerifyTimeStamp(s,hash,sig,tim)	CI_VerityTimeStap(hash,sig,tim)
#define MACI_WrapKey(s,src,wrap,key)		CI_WrapKey(src,wrap,key)
#define MACI_Zeroize(s)				CI_Zeroize()

#define MACI_SEL(x)	CI_Select(x)
#endif /* ! XP_UNIX */
#endif /* _GENCI_H_ */
