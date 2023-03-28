#include "csid.h"
#include "resgui.h"
data 'xlat' ( xlat_MAC_CYRILLIC_TO_8859_5,  "MAC_CYRILLIC -> 8859_5",purgeable){
/*     Translation MacOS_Cyrillic.txt -> 8859-5.txt   */
/* A0 is unmap !!! */
/* A1 is unmap !!! */
/* A2 is unmap !!! */
/* A3 is unmap !!! */
/* A5 is unmap !!! */
/* A6 is unmap !!! */
/* A8 is unmap !!! */
/* A9 is unmap !!! */
/* AA is unmap !!! */
/* AD is unmap !!! */
/* B0 is unmap !!! */
/* B1 is unmap !!! */
/* B2 is unmap !!! */
/* B3 is unmap !!! */
/* B5 is unmap !!! */
/* B6 is unmap !!! */
/* C2 is unmap !!! */
/* C3 is unmap !!! */
/* C4 is unmap !!! */
/* C5 is unmap !!! */
/* C6 is unmap !!! */
/* C7 is unmap !!! */
/* C8 is unmap !!! */
/* C9 is unmap !!! */
/* D0 is unmap !!! */
/* D1 is unmap !!! */
/* D2 is unmap !!! */
/* D3 is unmap !!! */
/* D4 is unmap !!! */
/* D5 is unmap !!! */
/* D6 is unmap !!! */
/* D7 is unmap !!! */
/* FF is unmap !!! */
/* There are total 33 character unmap !! */
/*        x0x1 x2x3 x4x5 x6x7 x8x9 xAxB xCxD xExF   */
/*0x*/  $"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"
/*1x*/  $"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"
/*2x*/  $"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"
/*3x*/  $"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"
/*4x*/  $"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"
/*5x*/  $"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"
/*6x*/  $"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"
/*7x*/  $"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"
/*8x*/  $"B0B1 B2B3 B4B5 B6B7 B8B9 BABB BCBD BEBF"
/*9x*/  $"C0C1 C2C3 C4C5 C6C7 C8C9 CACB CCCD CECF"
/*Ax*/  $"A0A1 A2A3 FDA5 A6A6 A8A9 AAA2 F2AD A3F3"
/*Bx*/  $"B0B1 B2B3 F6B5 B6A8 A4F4 A7F7 A9F9 AAFA"
/*Cx*/  $"F8A5 C2C3 C4C5 C6C7 C8C9 A0AB FBAC FCF5"
/*Dx*/  $"D0D1 D2D3 D4D5 D6D7 AEFE AFFF F0A1 F1EF"
/*Ex*/  $"D0D1 D2D3 D4D5 D6D7 D8D9 DADB DCDD DEDF"
/*Fx*/  $"E0E1 E2E3 E4E5 E6E7 E8E9 EAEB ECED EEFF"
};

data 'xlat' ( xlat_8859_5_TO_MAC_CYRILLIC,  "8859_5 -> MAC_CYRILLIC",purgeable){
/*     Translation 8859-5.txt -> MacOS_Cyrillic.txt   */
/* 80 is unmap !!! */
/* 81 is unmap !!! */
/* 82 is unmap !!! */
/* 83 is unmap !!! */
/* 84 is unmap !!! */
/* 85 is unmap !!! */
/* 86 is unmap !!! */
/* 87 is unmap !!! */
/* 88 is unmap !!! */
/* 89 is unmap !!! */
/* 8A is unmap !!! */
/* 8B is unmap !!! */
/* 8C is unmap !!! */
/* 8D is unmap !!! */
/* 8E is unmap !!! */
/* 8F is unmap !!! */
/* 90 is unmap !!! */
/* 91 is unmap !!! */
/* 92 is unmap !!! */
/* 93 is unmap !!! */
/* 94 is unmap !!! */
/* 95 is unmap !!! */
/* 96 is unmap !!! */
/* 97 is unmap !!! */
/* 98 is unmap !!! */
/* 99 is unmap !!! */
/* 9A is unmap !!! */
/* 9B is unmap !!! */
/* 9C is unmap !!! */
/* 9D is unmap !!! */
/* 9E is unmap !!! */
/* 9F is unmap !!! */
/* AD is unmap !!! */
/* There are total 33 character unmap !! */
/*        x0x1 x2x3 x4x5 x6x7 x8x9 xAxB xCxD xExF   */
/*0x*/  $"0001 0203 0405 0607 0809 0A0B 0C0D 0E0F"
/*1x*/  $"1011 1213 1415 1617 1819 1A1B 1C1D 1E1F"
/*2x*/  $"2021 2223 2425 2627 2829 2A2B 2C2D 2E2F"
/*3x*/  $"3031 3233 3435 3637 3839 3A3B 3C3D 3E3F"
/*4x*/  $"4041 4243 4445 4647 4849 4A4B 4C4D 4E4F"
/*5x*/  $"5051 5253 5455 5657 5859 5A5B 5C5D 5E5F"
/*6x*/  $"6061 6263 6465 6667 6869 6A6B 6C6D 6E6F"
/*7x*/  $"7071 7273 7475 7677 7879 7A7B 7C7D 7E7F"
/*8x*/  $"8081 8283 8485 8687 8889 8A8B 8C8D 8E8F"
/*9x*/  $"9091 9293 9495 9697 9899 9A9B 9C9D 9E9F"
/*Ax*/  $"CADD ABAE B8C1 A7BA B7BC BECB CDAD D8DA"
/*Bx*/  $"8081 8283 8485 8687 8889 8A8B 8C8D 8E8F"
/*Cx*/  $"9091 9293 9495 9697 9899 9A9B 9C9D 9E9F"
/*Dx*/  $"E0E1 E2E3 E4E5 E6E7 E8E9 EAEB ECED EEEF"
/*Ex*/  $"F0F1 F2F3 F4F5 F6F7 F8F9 FAFB FCFD FEDF"
/*Fx*/  $"DCDE ACAF B9CF B4BB C0BD BFCC CEA4 D9DB"
};

