#include "csid.h"
#include "resgui.h"
data 'xlat' ( xlat_MAC_CYRILLIC_TO_CP_1251,  "MAC_CYRILLIC -> CP_1251",purgeable){
/*     Translation MacOS_Cyrillic.txt -> cp1251.x   */
/* A2 is unmap !!! */
/* A3 is unmap !!! */
/* AD is unmap !!! */
/* B0 is unmap !!! */
/* B2 is unmap !!! */
/* B3 is unmap !!! */
/* B6 is unmap !!! */
/* C3 is unmap !!! */
/* C4 is unmap !!! */
/* C5 is unmap !!! */
/* C6 is unmap !!! */
/* D6 is unmap !!! */
/* There are total 12 character unmap !! */
/*        x0x1 x2x3 x4x5 x6x7 x8x9 xAxB xCxD xExF   */
/*0x*/  $"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"
/*1x*/  $"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"
/*2x*/  $"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"
/*3x*/  $"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"
/*4x*/  $"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"
/*5x*/  $"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"
/*6x*/  $"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"
/*7x*/  $"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"
/*8x*/  $"C0C1 C2C3 C4C5 C6C7 C8C9 CACB CCCD CECF"
/*9x*/  $"D0D1 D2D3 D4D5 D6D7 D8D9 DADB DCDD DEDF"
/*Ax*/  $"86B0 A2A3 A795 B6B2 AEA9 9980 90AD 8183"
/*Bx*/  $"B0B1 B2B3 B3B5 B6A3 AABA AFBF 8A9A 8C9C"
/*Cx*/  $"BCBD ACC3 C4C5 C6AB BB85 A08E 9E8D 9DBE"
/*Dx*/  $"9697 9394 9192 D684 A1A2 8F9F B9A8 B8FF"
/*Ex*/  $"E0E1 E2E3 E4E5 E6E7 E8E9 EAEB ECED EEEF"
/*Fx*/  $"F0F1 F2F3 F4F5 F6F7 F8F9 FAFB FCFD FEA4"
};

data 'xlat' ( xlat_CP_1251_TO_MAC_CYRILLIC,  "CP_1251 -> MAC_CYRILLIC",purgeable){
/*     Translation cp1251.x -> MacOS_Cyrillic.txt   */
/* 82 is unmap !!! */
/* 87 is unmap !!! */
/* 88 is unmap !!! */
/* 89 is unmap !!! */
/* 8B is unmap !!! */
/* 98 is unmap !!! */
/* 9B is unmap !!! */
/* A5 is unmap !!! */
/* A6 is unmap !!! */
/* AD is unmap !!! */
/* B4 is unmap !!! */
/* B7 is unmap !!! */
/* There are total 12 character unmap !! */
/*        x0x1 x2x3 x4x5 x6x7 x8x9 xAxB xCxD xExF   */
/*0x*/  $"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"
/*1x*/  $"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"
/*2x*/  $"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"
/*3x*/  $"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"
/*4x*/  $"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"
/*5x*/  $"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"
/*6x*/  $"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"
/*7x*/  $"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"
/*8x*/  $"ABAE 82AF D7C9 A087 8889 BC8B BECD CBDA"
/*9x*/  $"ACD4 D5D2 D3A5 D0D1 98AA BD9B BFCE CCDB"
/*Ax*/  $"CAD8 D9B7 FFA5 A6A4 DDA9 B8C7 C2AD A8BA"
/*Bx*/  $"A1B1 A7B4 B4B5 A6B7 DEDC B9C8 C0C1 CFBB"
/*Cx*/  $"8081 8283 8485 8687 8889 8A8B 8C8D 8E8F"
/*Dx*/  $"9091 9293 9495 9697 9899 9A9B 9C9D 9E9F"
/*Ex*/  $"E0E1 E2E3 E4E5 E6E7 E8E9 EAEB ECED EEEF"
/*Fx*/  $"F0F1 F2F3 F4F5 F6F7 F8F9 FAFB FCFD FEDF"
};

