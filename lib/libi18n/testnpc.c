
/*
	This file is a testing program should not be include in project.
	Only for testing purpose.
*/
#include "xp_trace.h"
#include "csid.h"
#include "unicpriv.h"
#include "libi18n.h"
#include "npc.h"


/* #define TEST_ROUNDTRIP 1 */
/* #define DONT_REPORT_DETAILS 1 */

void	DebuggerDummy(void);

#ifdef DONT_REPORT_DETAILS
#define XP_TraceHeader(a)	DebuggerDummy()
#else
#define XP_TraceHeader	XP_Trace
#endif
/*-----------------------------------------------------------------------*/
/*	
	Global
*/
unsigned char in[256];
unsigned char out[256];
int16 outcsid;
uint16 inread;
uint16 outlen;
int16 i,j,k;
uint16 csid ;
XP_Bool ret;
unsigned char npcfb;
/*-----------------------------------------------------------------------*/
/*	
	Tables
*/
enum {
	notesting = 0,
	test1byte,
	test2byte,
	test8bit,
	testsjis,
	testbig5
};

static	uint16	testcase[MAXCSIDINNPC] =
{
	notesting,	notesting,	test1byte,	notesting,	testsjis,	notesting,	test1byte,	testbig5,
	test8bit,	notesting,	test1byte,	test1byte,	test8bit,	notesting,	test1byte,	test1byte,
	test1byte,	test1byte,	test1byte,	test1byte,	test1byte,	test1byte,	test1byte,	test1byte,
	test2byte,	test2byte,	test2byte,	test1byte,	test2byte,	test1byte,	test2byte,	test2byte,
	notesting,	notesting,	notesting,	notesting,	notesting,	notesting,	notesting,	notesting,
	notesting,	notesting,	notesting,	notesting,	notesting,	notesting,	notesting,	notesting,
	notesting,	notesting,	notesting,	notesting,	notesting,	notesting,	notesting,	notesting,
	notesting,	notesting,	notesting,	notesting,	notesting,	notesting,	test1byte,	notesting,
};
/*-----------------------------------------------------------------------*/
/*	
	Macros
*/
#define	CheckRet()		XP_ASSERT(ret == TRUE)
#define CheckInread(einread)	XP_ASSERT(inread == einread)
#define CheckOutlen(eoutlen)	XP_ASSERT(outlen == eoutlen)
#define CheckCSID()		XP_ASSERT((uint16)outcsid == csid)

void 	StopInHere(unsigned char *in, unsigned char *out, uint16 inread, int16 outlen, uint16 outcsid )
{
	int a;
	a = 5;	/* Dummy rountine, just stop  in here */
}

void BasicCheck(uint16 einread,uint16 eoutlen)
{	
	CheckRet();	
	CheckInread(einread); 
	CheckOutlen(eoutlen); 
	CheckCSID();
}
void CheckU(uint16 einread,uint16 eoutlen)	
{	
	CheckRet();
	CheckInread(einread); 
	CheckOutlen(eoutlen); 
}
void BasicCheck1(void)
{ 
	BasicCheck(2,1);
	XP_ASSERT(in[2] == out[1]);
}
void BasicCheck2(void)
{ 
	BasicCheck(3,2);
	XP_ASSERT(in[2] == out[1]);	
	XP_ASSERT(in[3] == out[2]);	
}
void Test1(unsigned char b1)
{	
		in[0]= npcfb;	
		in[1] = b1;	
		ret= INTL_ConvertNPC(in, 2,&inread, out, 256,  &outlen, &outcsid);	
		BasicCheck1();
}
void Test2(unsigned char  b1,unsigned char b2)
{	
		in[0]= npcfb;
		in[1] = b1;	
		in[2] = b2;	
		ret= INTL_ConvertNPC(in, 3,&inread, out, 256,  &outlen, &outcsid);
		BasicCheck2();
}
void TestU1(unsigned char b1)	
{
		in[0]= b1;	
		ret= INTL_ConvertNPC(in, 1,&inread, out, 256,  &outlen, &outcsid);	
		CheckU(1,1);
		XP_ASSERT(in[0] == out[0]); 
}
void TestU2(unsigned char b1,unsigned char b2)
{	
		in[0]= b1;	
		in[1]= b2;	
		ret= INTL_ConvertNPC(in, 2,&inread, out, 256,  &outlen, &outcsid);	
		CheckInread(2);
		StopInHere(in,out,inread,outlen,outcsid);	
}
void TestU3(unsigned char b1,unsigned char b2,unsigned char b3)	
{	
		in[0]= b1;	
		in[1]= b2;	
		in[2]= b3;	
		ret= INTL_ConvertNPC(in, 3,&inread, out, 256,  &outlen, &outcsid);	
		CheckInread(3); 
		StopInHere(in,out,inread,outlen,outcsid);	
}

/*-----------------------------------------------------------------------*/
/*	
	Routines
*/
void	testutf81(void)
{
	XP_TraceHeader("Start TestUTF81\n");
	for(i=1;i<0x80;i++)
		TestU1(i);
	XP_TraceHeader("End TestUTF81\n");
}
void	testutf82(void)
{
	XP_TraceHeader("Start TestUTF82\n");
	for(i=0xc2;i<0xe0;i++)
		for(j=0x80;j<0xc0;j++)
			TestU2(i,j);
	XP_TraceHeader("End TestUTF82\n");
}
void	testutf83(void)
{
	XP_TraceHeader("Start TestUTF83\n");
	for(j=0xa0;j<0xc0;j++)
		for(k=0x80;k<0xc0;k++)
			TestU3(0xe0,j,k);
	for(i=0xe1;i<0xf0;i++)
		for(j=0x80;j<0xc0;j++)
			for(k=0x80;k<0xc0;k++)
				TestU3(i,j,k);
	XP_TraceHeader("End TestUTF83\n");
}
void	testutf84(void)
{
	XP_TraceHeader("Start TestUTF84\n");
	XP_TraceHeader("To Be Implement\n");
	XP_TraceHeader("End TestUTF84\n");
}
void	testutf85(void)
{
	XP_TraceHeader("Start TestUTF85\n");
	XP_TraceHeader("To Be Implement\n");
	XP_TraceHeader("End TestUTF85\n");
}
void	testutf86(void)
{
	XP_TraceHeader("Start TestUTF86\n");
	XP_TraceHeader("To Be Implement\n");
	XP_TraceHeader("End TestUTF86\n");
}

void	Test1Byte(unsigned char npcfb, uint16 csid)
{
	XP_TraceHeader("Start Test1Byte\n");
	for(i=1;i<256;i++)
		Test1( i);
	XP_TraceHeader("End Test1Byte\n");
}
void	Test2Byte(unsigned char npcfb, uint16 csid)
{
	XP_TraceHeader("Start Test2Byte\n");
	for(i=0x21;i<0x7F;i++)
		for(j=0x21;j<0x7F;j++)
			Test2(i,j);
	XP_TraceHeader("End Test2Byte\n");
}
void	Test8Bit(unsigned char npcfb, uint16 csid)
{
	XP_TraceHeader("Start Test8bit\n");
	for(i=1;i<0x80;i++)
		Test1(i);
	for(i=0xA1;i<0xFF;i++)
		for(j=0xA1;j<0xFF;j++)
			Test2( i,j);
	XP_TraceHeader("End Test8bit\n");
}
void	Testsjis(unsigned char npcfb, uint16 csid)
{
	XP_TraceHeader("Start Testsjis\n");
	for(i=1;i<0x80;i++)
		Test1(i);
	for(i=0xA1;i<0xE0;i++)
		Test1( i);
	for(i=0x81;i<0xA0;i++)
	{
		for(j=0x40;j<0x80;j++)
			Test2( i,j);
		for(j=0x80;j<0xFD;j++)
			Test2( i,j);
	}
	for(i=0xE0;i<0xFD;i++)
	{
		for(j=0x40;j<0x80;j++)
			Test2( i,j);
		for(j=0x80;j<0xFD;j++)
			Test2(i,j);
	}
	XP_TraceHeader("End Testsjis\n");
}
void	Testbig5(unsigned char npcfb, uint16 csid)
{
	XP_TraceHeader("Start Testbit5\n");
	for(i=1;i<0x80;i++)
		Test1( i);
	for(i=0xA1;i<0xFF;i++)
	{
		for(j=0x40;j<0x80;j++)
			Test2( i,j);
		for(j=0xA1;j<0xFF;j++)
			Test2(i,j);
	}
	XP_TraceHeader("End Testbit5\n");
}
void	DebuggerDummy()
{
}

void	VerifyUnicodeTestSeg(unsigned char *buf1, unsigned char *buf2, INTL_Unicode *u)
{

	if(strcmp((char*)buf1, (char*)buf2) != 0)
	{
		int i;
		unsigned char *a, *b; 
		INTL_Unicode *p;
		XP_Trace("Round Trip Problem \n Unicode:");
		for(i=0,p=u; *p ; p++,i++)
			XP_Trace("%d- U+%4X",i,*p);
		XP_Trace(" String [from to]");
		for(i=0,a=buf1,b=buf2; (*a && *b) ; a++, b++,i++)
			if(*a != *b)
				XP_Trace("%d- %2X %2X",i,*a,*b);
		for(;*a;a++,i++)
			XP_Trace("%d- %2X NULL",i,*a);
		
		for(;*b;b++,i++)
			XP_Trace("%d- NULL %2X",i,*b);
	}
	else
		XP_TraceHeader("Round Trip OK!!!");
}
void unicodeTestSBSeg(unsigned char f, unsigned char l,
	uint16 csid, char* charsetname )
{
	uint16 len1, len2;
	unsigned char *buf1,*buf2;
	uint16 i;
	uint16 ulen;
	INTL_Unicode *u;
	unsigned char c;

	XP_TraceHeader("Start unicodeTestSeg(%s) routine, %2X %2X\n",charsetname,f,l);

	len1 = l-f+2;
	buf1=XP_ALLOC(len1);
	XP_ASSERT(buf1 != NULL);
	for(i=0,c=f;c<l;i++,c++)
	{
		XP_ASSERT(i < len1 );	
		buf1[i]=c;
	}
	XP_ASSERT(i < len1 );	
	buf1[i++]=l;
	XP_ASSERT(i < len1 );	
	buf1[i]=0;

	ulen = INTL_StrToUnicodeLen(csid,buf1) + 1;

	u=XP_ALLOC((ulen)*sizeof(INTL_Unicode));
	XP_ASSERT(u != NULL);
	
	INTL_StrToUnicode(csid, buf1, u, ulen);

	len2 = INTL_UnicodeToStrLen(csid, u, INTL_UnicodeLen(u)) + 1;

	buf2 = XP_ALLOC(len2);

	XP_ASSERT(buf2 != NULL);

	INTL_UnicodeToStr(csid,u,INTL_UnicodeLen(u), buf2,len2);

	VerifyUnicodeTestSeg(buf1,buf2,u);

	XP_FREE(buf1);
	XP_FREE(buf2);
	XP_FREE(u);
	XP_TraceHeader("End unicodeTestSeg() routine\n");
}
void unicodeTest3BSeg(unsigned char fb, unsigned char sb,
	unsigned char f, unsigned char l,
	uint16 csid, char* charsetname )
{
	uint16 len1,len2;
	unsigned char *buf1, *buf2;
	unsigned char c;
	uint16 i;
	uint16 ulen;
	INTL_Unicode *u;

	XP_TraceHeader("Start unicodeTest3BSeg(%s) routine, %2X%2X%2X %2X%2X%2X\n",
			charsetname,fb,sb,f,fb,sb,l);

	len1 = (l-f+2)*3;
	buf1=XP_ALLOC(len1);
	XP_ASSERT(buf1 != NULL);
	for(i=0,c=f;c<l ;c++)
	{	
		XP_ASSERT(i < len1 );	
		buf1[i++]=fb;
		XP_ASSERT(i < len1 );	
		buf1[i++]=sb;
		XP_ASSERT(i < len1 );	
		buf1[i++]=c;
	}
	XP_ASSERT(i < len1 );	
	buf1[i++]=fb;
	XP_ASSERT(i < len1 );	
	buf1[i++]=sb;
	XP_ASSERT(i < len1 );	
	buf1[i++]=l;
	XP_ASSERT(i < len1 );	
	buf1[i]=0;

	ulen = INTL_StrToUnicodeLen(csid,buf1) + 1;

	u=XP_ALLOC((ulen)*sizeof(INTL_Unicode));
	XP_ASSERT(u != NULL);
	
	INTL_StrToUnicode(csid, buf1, u, ulen);

	len2 = INTL_UnicodeToStrLen(csid, u,INTL_UnicodeLen(u)) + 1;

	buf2 = XP_ALLOC(len2);

	XP_ASSERT(buf2 != NULL);

	INTL_UnicodeToStr(csid,u,INTL_UnicodeLen(u),buf2,len2);

	VerifyUnicodeTestSeg(buf1,buf2,u);

	XP_FREE(buf1);
	XP_FREE(buf2);
	XP_FREE(u);
	XP_TraceHeader("End unicodeTestSeg() routine\n");
}
void unicodeTestTBSeg(unsigned char fb, unsigned char f, unsigned char l,
	uint16 csid, char* charsetname )
{
	uint16 len1,len2;
	unsigned char *buf1, *buf2;
	unsigned char c;
	uint16 i;
	uint16 ulen;
	INTL_Unicode *u;

	XP_TraceHeader("Start unicodeTestTBSeg(%s) routine, %2X%2X %2X%2X\n",
		charsetname,fb,f,fb,l);

	len1 = (l-f+2)*2;
	buf1=XP_ALLOC(len1);
	XP_ASSERT(buf1 != NULL);
	for(i=0,c=f;c<l ;c++)
	{	
		XP_ASSERT(i < len1 );	
		buf1[i++]=fb;
		XP_ASSERT(i < len1 );	
		buf1[i++]=c;
	}
	XP_ASSERT(i < len1 );	
	buf1[i++]=fb;
	XP_ASSERT(i < len1 );	
	buf1[i++]=l;
	XP_ASSERT(i < len1 );	
	buf1[i]=0;

	ulen = INTL_StrToUnicodeLen(csid,buf1) + 1;

	u=XP_ALLOC((ulen)*sizeof(INTL_Unicode));
	XP_ASSERT(u != NULL);
	
	INTL_StrToUnicode(csid, buf1, u, ulen);

	len2 = INTL_UnicodeToStrLen(csid, u,INTL_UnicodeLen(u)) + 1;

	buf2 = XP_ALLOC(len2);

	XP_ASSERT(buf2 != NULL);

	INTL_UnicodeToStr(csid,u,INTL_UnicodeLen(u),buf2,len2);

	VerifyUnicodeTestSeg(buf1,buf2,u);

	XP_FREE(buf1);
	XP_FREE(buf2);
	XP_FREE(u);
	XP_TraceHeader("End unicodeTestSeg() routine\n");
}



void unicodeTest(void)
{
	XP_TraceHeader("Start unicodeTest() routine\n");
	XP_TraceHeader("To Be Implement\n");
	/* CS_LATIN1 */
#ifndef XP_MAC
	unicodeTestSBSeg(0x01,0x7e,CS_LATIN1, "CS_LATIN1");
	unicodeTestSBSeg(0xa0,0xfe,CS_LATIN1, "CS_LATIN1");
#endif
	/* CS_SJIS */
	unicodeTestSBSeg(0x20,0x7e,CS_SJIS, "CS_SJIS");
	unicodeTestSBSeg(0xA1,0xDF,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x81,0x40,0x7E,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x81,0x80,0xac,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x81,0xb8,0xbf,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x81,0xc8,0xce,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x81,0xda,0xe8,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x81,0xf0,0xf7,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x82,0x4f,0x58,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x82,0x6e,0x79,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x82,0x81,0x9a,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x82,0x9F,0xf1,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x83,0x40,0x7E,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x83,0x80,0x96,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x83,0x9F,0xb6,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x83,0xbF,0xD6,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x84,0x40,0x60,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x84,0x70,0x7E,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x84,0x80,0x91,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x84,0x9f,0xbE,CS_SJIS, "CS_SJIS");
	unicodeTestTBSeg(0x88,0x9f,0xFC,CS_SJIS, "CS_SJIS");
	{
		unsigned char fb;
		for(fb=0x89;fb<0x98;fb++)
		{
			unicodeTestTBSeg(fb,0x40,0x7E,CS_SJIS, "CS_SJIS");
			unicodeTestTBSeg(fb,0x80,0xFC,CS_SJIS, "CS_SJIS");
		}
		unicodeTestTBSeg(0x98,0x40,0x72,CS_SJIS, "CS_SJIS");
		unicodeTestTBSeg(0x98,0x9F,0xFC,CS_SJIS, "CS_SJIS");
		for(fb=0x99;fb<0xa0;fb++)
		{
			unicodeTestTBSeg(fb,0x40,0x7E,CS_SJIS, "CS_SJIS");
			unicodeTestTBSeg(fb,0x80,0xFC,CS_SJIS, "CS_SJIS");
		}
		for(fb=0xe0;fb<0xEA;fb++)
		{
			unicodeTestTBSeg(fb,0x40,0x7E,CS_SJIS, "CS_SJIS");
			unicodeTestTBSeg(fb,0x80,0xFC,CS_SJIS, "CS_SJIS");
		}
		unicodeTestTBSeg(0xEA,0x40,0x7E,CS_SJIS, "CS_SJIS");
		unicodeTestTBSeg(0xEA,0x80,0xA4,CS_SJIS, "CS_SJIS");
	}
		/* To Be Continue */
	/* CS_EUCJP */
#ifdef XP_UNIX
	unicodeTestSBSeg(0x20,0x7e,CS_EUCJP, "CS_EUCJP");
	DebuggerDummy();
	unicodeTestTBSeg(0x8e,0xA1,0xDF,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA1,0xA1,0xFE,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA2,0xA1,0xAE,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA2,0xBA,0xC1,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA2,0xCA,0xD0,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA2,0xDc,0xEA,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA2,0xf2,0xf9,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA3,0xB0,0xB9,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA3,0xc1,0xda,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA3,0xe1,0xfa,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA4,0xA1,0xf3,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA5,0xA1,0xf6,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA6,0xA1,0xb8,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA6,0xc1,0xd8,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA7,0xA1,0xC1,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA7,0xD1,0xF1,CS_EUCJP, "CS_EUCJP");
	unicodeTestTBSeg(0xA8,0xA1,0xc0,CS_EUCJP, "CS_EUCJP");
	{
		unsigned char fb;
		for(fb=0xB0;fb<0xcf;fb++)
			unicodeTestTBSeg(fb,0xA1,0xFE,CS_EUCJP, "CS_EUCJP");
		unicodeTestTBSeg(0xcf,0xA1,0xd3,CS_EUCJP, "CS_EUCJP");
		for(fb=0xd0;fb<0xF4;fb++)
			unicodeTestTBSeg(fb,0xA1,0xFE,CS_EUCJP, "CS_EUCJP");
		unicodeTestTBSeg(0xF4,0xA1,0xA6,CS_EUCJP, "CS_EUCJP");
	}
	unicodeTest3BSeg(0x8F,0xA2,0xAF,0xB9,CS_EUCJP, "CS_EUCJP");
	unicodeTest3BSeg(0x8F,0xA2,0xc2,0xc4,CS_EUCJP, "CS_EUCJP");
	unicodeTest3BSeg(0x8F,0xA2,0xeb,0xf1,CS_EUCJP, "CS_EUCJP");
	unicodeTest3BSeg(0x8F,0xA6,0xe1,0xe5,CS_EUCJP, "CS_EUCJP");
	unicodeTest3BSeg(0x8F,0xA6,0xf1,0xfc,CS_EUCJP, "CS_EUCJP");
	unicodeTest3BSeg(0x8F,0xA6,0xf1,0xfc,CS_EUCJP, "CS_EUCJP");
	unicodeTest3BSeg(0x8F,0xA7,0xc2,0xce,CS_EUCJP, "CS_EUCJP");
	unicodeTest3BSeg(0x8F,0xA7,0xf2,0xfe,CS_EUCJP, "CS_EUCJP");
/*	GAP
	unicodeTest3BSeg(0x8F,0xA9,0xA1,0xB0,CS_EUCJP, "CS_EUCJP");
*/
	unicodeTest3BSeg(0x8F,0xA9,0xC1,0xD0,CS_EUCJP, "CS_EUCJP");
	unicodeTest3BSeg(0x8F,0xAa,0xA1,0xB8,CS_EUCJP, "CS_EUCJP");
	unicodeTest3BSeg(0x8F,0xAa,0xBA,0xF7,CS_EUCJP, "CS_EUCJP");
	unicodeTest3BSeg(0x8F,0xAb,0xA1,0xBB,CS_EUCJP, "CS_EUCJP");
	unicodeTest3BSeg(0x8F,0xAb,0xBD,0xC3,CS_EUCJP, "CS_EUCJP");
	unicodeTest3BSeg(0x8F,0xAb,0xC5,0xF7,CS_EUCJP, "CS_EUCJP");
	{
		unsigned char fb;
		for(fb=0xB0;fb<0xED;fb++)
			unicodeTest3BSeg(0x8F,fb,0xA1,0xFE,CS_EUCJP, "CS_EUCJP");
		unicodeTest3BSeg(0x8F,0xED,0xA1,0xe3,CS_EUCJP, "CS_EUCJP");
	}
#endif
	/* CS_MAC_ROMAN */
#ifdef XP_MAC
	unicodeTestSBSeg(0x01,0x7e,CS_MAC_ROMAN, "CS_MAC_ROMAN");
	unicodeTestSBSeg(0xa0,0xfe,CS_MAC_ROMAN, "CS_MAC_ROMAN");
#endif
	/* CS_BIG5 */
	unicodeTestSBSeg(0x20,0x7e,CS_BIG5, "CS_BIG5");
	DebuggerDummy();
	unicodeTestTBSeg(0xA1,0x40,0x7E,CS_BIG5, "CS_BIG5");
	unicodeTestTBSeg(0xA1,0xA1,0xFE,CS_BIG5, "CS_BIG5");
	unicodeTestTBSeg(0xA2,0x40,0x7E,CS_BIG5, "CS_BIG5");
	unicodeTestTBSeg(0xA2,0xA1,0xFE,CS_BIG5, "CS_BIG5");
	unicodeTestTBSeg(0xA3,0x40,0x7E,CS_BIG5, "CS_BIG5");
	unicodeTestTBSeg(0xA3,0xA1,0xe0,CS_BIG5, "CS_BIG5");
	{
		unsigned char fb;
		for(fb=0xa4;fb<0xc5;fb++)
		{
			unicodeTestTBSeg(fb,0x40,0x7E,CS_BIG5, "CS_BIG5");
			unicodeTestTBSeg(fb,0xA1,0xfe,CS_BIG5, "CS_BIG5");
		}
		unicodeTestTBSeg(0xc6,0x40,0x7E,CS_BIG5, "CS_BIG5");
		for(fb=0xc9;fb<0xf8;fb++)
		{
			unicodeTestTBSeg(fb,0x40,0x7E,CS_BIG5, "CS_BIG5");
			unicodeTestTBSeg(fb,0xA1,0xfe,CS_BIG5, "CS_BIG5");
		}
		unicodeTestTBSeg(0xf9,0x40,0x7E,CS_BIG5, "CS_BIG5");
		unicodeTestTBSeg(0xf9,0xA1,0xf5,CS_BIG5, "CS_BIG5");
	}
	/* CS_GB_8BIT */
	/* CS_CNS_8BIT */
#ifdef XP_UNIX
#endif
	/* CS_LATIN2 */
#ifndef XP_MAC
#endif
	/* CS_MAC_CE */
#ifdef XP_MAC
#endif
	/* CS_KSC_8BIT */
	/* CS_8859_3 */
	/* CS_8859_4 */
	/* CS_8859_5 */
	/* CS_8859_6 */
	/* CS_8859_7 */
	/* CS_8859_8 */
	XP_TraceHeader("End unicodeTest() routine\n");
}
void iteratorTest(void)
{
	XP_TraceHeader("Start iteratorTest() routine\n");
	XP_TraceHeader("To Be Implement\n");
{
	uint16 i1,i2;
	for(i1=0;i1<256;i1++)
	{
		INTL_CompoundStr *cstr;
		INTL_Unicode uline[257];
		INTL_CompoundStrIterator  ite;
		INTL_Encoding_ID encoding;
		unsigned char *outtext;


		XP_TraceHeader("INTL_CompoundStr Test U+%4X - U+%4X",(i1<<8),(((i1+1)<<8)-1));
		for(i2=0;i2<256;i2++)
			uline[i2] = (i1	<< 8 ) + i2;
		uline[257] = 0;
		if(uline[0] == 0)
			uline[0] = 1;
		cstr = INTL_CompoundStrFromUnicode(uline,256);
		for( ite = INTL_CompoundStrFirstStr(cstr, &encoding, &outtext);
	     	     ite;
		     ite= INTL_CompoundStrNextStr(ite, &encoding, &outtext))
		{
			unsigned char tmp[3*256+8];
			unsigned char *s,*d;
			uint16 i; 	
			for(i=0,d=tmp, s=outtext; *s; s++,i++,d+=3)
				sprintf((char*)d,"%2X ",*s);
			*d = '\0';	
			XP_Trace("EncodingID %d -%s",encoding,tmp );
		}
		INTL_CompoundStrDestroy(cstr);
	}
}
	XP_TraceHeader("End iteratorTest() routine\n");
}

void npctest(void)
{
	/* test all the csid stuff */
	int i;
#ifdef TEST_ROUNDTRIP
	unicodeTest();
#endif
	iteratorTest();
	XP_TraceHeader("Enter npctest() routine\n");
	for( i = 0x00 ; i <= 0x40 ; i++)
	{	
		csid = NPCToCSID(i);
		XP_Trace("Test CSID = %d\n",csid);
		npcfb = i | 0x80;
		switch(testcase[i])
		{
			case test1byte:
				Test1Byte(npcfb,csid);
				break;
			case test2byte:
				Test2Byte(npcfb,csid);
				break;
			case test8bit:
				Test8Bit(npcfb,csid);
				break;
			case testsjis:
				Testsjis(npcfb,csid);
				break;
			case testbig5:
				Testbig5(npcfb,csid);
				break;
			case notesting:
				break;
		}
	}
	/* test UTF8 */
	testutf81();
	testutf82();
	testutf83();
	testutf84();
	testutf85();
	testutf86();
	XP_TraceHeader("Leave npctest() routine\n");
}
