/*
 * The builtin CA certs.
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: certinit.c,v 1.26.48.7 1997/05/24 00:23:03 jwz Exp $
 */

#include "cert.h"
#include "base64.h"
#include "mcom_db.h"
#include "certdb.h"

static char ns_b3_code_signer_cert[] =
"MIICTjCCAbegAwIBAgIBADANBgkqhkiG9w0BAQUFADBtMSYwJAYDVQQKEx1OZXRz"
"Y2FwZSBDb21tdW5pY2F0aW9ucyBDb3JwLjFDMEEGA1UEAxM6TmV0c2NhcGUgQ29t"
"bXVuaWNhdG9yIEJldGEgMyBDb2RlIFNpZ25pbmcgVGVzdCBDZXJ0aWZpY2F0ZTAe"
"Fw05NzA0MDIxNzAxMjRaFw05NzA3MDIxNjAxMjRaMG0xJjAkBgNVBAoTHU5ldHNj"
"YXBlIENvbW11bmljYXRpb25zIENvcnAuMUMwQQYDVQQDEzpOZXRzY2FwZSBDb21t"
"dW5pY2F0b3IgQmV0YSAzIENvZGUgU2lnbmluZyBUZXN0IENlcnRpZmljYXRlMIGf"
"MA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDNzjJOpgx3SlIlMJQheNbVNL5R4Jp9"
"fkbrZmgBOkoHu5j0hUA9vC0PP0BDXgk+ENQvV3DxCc1tNw0hj4xzCa9GySniVLoI"
"c0MEfkmubODUllZUuoCEzjuJ8PIvoz5+XDaa/xAwMfvTlEcuvz7JTk68yeTWdElf"
"Li9dGVwKnbr7owIDAQABMA0GCSqGSIb3DQEBBQUAA4GBAJXJgqZgGDOsNZD3vR3V"
"Z3TjHS92KYXq0l9Irq8qqspdNoh8FCoLevpwbEK9bDuRKMSuOn2f2XMVXmP6UdeF"
"X2zawKDrCRK0PFLxLRejmO6OLib1KcminheZRMnt93KZIcmX/XXnaBTvvuPOZs5j"
"SOPkPqjbat6ffFeWiKFxpGo6";

static char ns_b3_system_principal_signer_cert[] =
"MIICVjCCAb+gAwIBAgIBADANBgkqhkiG9w0BAQUFADBxMSYwJAYDVQQKEx1OZXRz"
"Y2FwZSBDb21tdW5pY2F0aW9ucyBDb3JwLjFHMEUGA1UEAxM+TmV0c2NhcGUgQ29t"
"bXVuaWNhdG9yIEJldGEgMyBTeXN0ZW0gUHJpbmNpcGFsIFRlc3QgQ2VydGlmaWNh"
"dGUwHhcNOTcwNDAyMTcwMjA4WhcNOTcwNzAyMTYwMjA4WjBxMSYwJAYDVQQKEx1O"
"ZXRzY2FwZSBDb21tdW5pY2F0aW9ucyBDb3JwLjFHMEUGA1UEAxM+TmV0c2NhcGUg"
"Q29tbXVuaWNhdG9yIEJldGEgMyBTeXN0ZW0gUHJpbmNpcGFsIFRlc3QgQ2VydGlm"
"aWNhdGUwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAKhgVWToJoSIJ8jUyvG9"
"emE+Bb0Joo4Lozfp3DZUZyAba/obOV2nQJCjvN2C7ipnfXKaOpqXlvbu3FuPC71x"
"D8H3UX1PJf8NQl5bP345HcP/hyiK78vZL3SfEVUKCQWXYmzEUwJWXAc9icQZtyfH"
"gEe7tnZEG2Zss3fispoaceiNAgMBAAEwDQYJKoZIhvcNAQEFBQADgYEAgFPaMWy+"
"TcLQuHQNyXABoyGUFqh7IsB+xuwOULXviKHEB9/xJXbYZnSabS8JPXXeQ36/Q1dC"
"eo+GtZE8NNeMzUXkXKIP+Y7GR23jyPLMrJvlZTJBGUBsebeAbWG6yGr4U3V49Q6f"
"GWc2cXOBAQK/Hg6AFqaNqVoMC8b7GrkwHjA=";

static char rsa_server_ca_cert[] =
"MIICKTCCAZYCBQJBAAABMA0GCSqGSIb3DQEBAgUAMF8xCzAJBgNVBAYTAlVTMSAw"
"HgYDVQQKExdSU0EgRGF0YSBTZWN1cml0eSwgSW5jLjEuMCwGA1UECxMlU2VjdXJl"
"IFNlcnZlciBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eTAeFw05NDExMDkyMzU0MTda"
"Fw05OTEyMzEyMzU0MTdaMF8xCzAJBgNVBAYTAlVTMSAwHgYDVQQKExdSU0EgRGF0"
"YSBTZWN1cml0eSwgSW5jLjEuMCwGA1UECxMlU2VjdXJlIFNlcnZlciBDZXJ0aWZp"
"Y2F0aW9uIEF1dGhvcml0eTCBmzANBgkqhkiG9w0BAQEFAAOBiQAwgYUCfgCSznrB"
"roM+WqqJg1esJQF2DK2ujiw3zus1eGRUA+WEQFHJv48I4oqCCNIWhjdV6bEhAq12"
"aIGaBaJLyUslZiJWbIgHj/eBWW2EB2VwE3F2Ppt3TONQiVaYSLkdpykaEy5KEVmc"
"HhXVSVQsczppgrGXOZxtcGdI5d0t1sgeewIDAQABMA0GCSqGSIb3DQEBAgUAA34A"
"iNHReSHO4ovo+MF9NFM/YYPZtgs4F7boviGNjwC4i1N+RGceIr2XJ+CchcxK9oU7"
"suK+ktPlDemvXA4MRpX/oRxePug2WHpzpgr4IhFrwwk4fia7c+8AvQKk8xQNMD9h"
"cHsg/jKjn7P0Z1LctO6EjJY2IN6BCINxIYoPnqk=";

static char rsa_com_ca_cert[] =
"MIICIzCCAZACBQJBAAAWMA0GCSqGSIb3DQEBAgUAMFwxCzAJBgNVBAYTAlVTMSAw"
"HgYDVQQKExdSU0EgRGF0YSBTZWN1cml0eSwgSW5jLjErMCkGA1UECxMiQ29tbWVy"
"Y2lhbCBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eTAeFw05NDExMDQxODU4MzRaFw05"
"OTExMDMxODU4MzRaMFwxCzAJBgNVBAYTAlVTMSAwHgYDVQQKExdSU0EgRGF0YSBT"
"ZWN1cml0eSwgSW5jLjErMCkGA1UECxMiQ29tbWVyY2lhbCBDZXJ0aWZpY2F0aW9u"
"IEF1dGhvcml0eTCBmzANBgkqhkiG9w0BAQEFAAOBiQAwgYUCfgCk+4Fie84QJ93o"
"975sbsZwmdu41QUDaSiCnHJ/lj+O7Kwpkj+KFPhCdr69XQO5kNTQvAayUTNfxMK/"
"touPmbZiImDd298ggrTKoi8tUO2UMt7gVY3UaOLgTNLNBRYulWZcYVI4HlGogqHE"
"7yXpCuaLK44xZtn42f29O2nZ6wIDAQABMA0GCSqGSIb3DQEBAgUAA34AdrW2EP4j"
"9/dZYkuwX5zBaLxJu7NJbyFHXSudVMQAKD+YufKKg5tgf+tQx6sFEC097TgCwaVI"
"0v5loMC86qYjFmZsGySp8+x5NRhPJsjjr1BKx6cxa9B8GJ1Qv6km+iYrRpwUqbtb"
"MJhCKLVLU7tDCZJAuqiqWqTGtotXTcU=";

static char vs1_ca_cert[] =
"MIICMTCCAZoCBQKkAAABMA0GCSqGSIb3DQEBAgUAMF8xCzAJBgNVBAYTAlVTMRcw"
"FQYDVQQKEw5WZXJpU2lnbiwgSW5jLjE3MDUGA1UECxMuQ2xhc3MgMSBQdWJsaWMg"
"UHJpbWFyeSBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eTAeFw05NjAxMjkwMDAwMDBa"
"Fw05OTEyMzEyMzU5NTlaMF8xCzAJBgNVBAYTAlVTMRcwFQYDVQQKEw5WZXJpU2ln"
"biwgSW5jLjE3MDUGA1UECxMuQ2xhc3MgMSBQdWJsaWMgUHJpbWFyeSBDZXJ0aWZp"
"Y2F0aW9uIEF1dGhvcml0eTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEA5Rm/"
"baNWYS2ZSHH2Z965jeu3noaACpEO+jglr0aIguVzqKCbJF0NH8xlbgyw0FaEGIea"
"BpsQoXPftFg5a27B9hXVqKg/qhIGjTGsf7A01480Z4gJzRQR4k5FVmkfeAKA2txH"
"kSm7NsljXMXg1y2He6G3MrB7MLoqLzGq7qNn2tsCAwEAATANBgkqhkiG9w0BAQIF"
"AAOBgQBSc7qaVdzcP4J9sJCYYiqCTHYAbiU91cIJcFcBDA93Hxih+xxgDqB1O0kh"
"Qf6nXC1MQknT/yjYjOqd/skH4neCUyPeVfPORJP6+ky9yjbzW2aynsjyDF5e1KG0"
"IQkzyjtZ/JLCOPyt2ZYk4C36oyn1M2h4TrS8n2k14qiYlHM7xA==";

static char vs2_ca_cert[] =
"MIICMTCCAZoCBQKjAAABMA0GCSqGSIb3DQEBAgUAMF8xCzAJBgNVBAYTAlVTMRcw"
"FQYDVQQKEw5WZXJpU2lnbiwgSW5jLjE3MDUGA1UECxMuQ2xhc3MgMiBQdWJsaWMg"
"UHJpbWFyeSBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eTAeFw05NjAxMjkwMDAwMDBa"
"Fw05OTEyMzEyMzU5NTlaMF8xCzAJBgNVBAYTAlVTMRcwFQYDVQQKEw5WZXJpU2ln"
"biwgSW5jLjE3MDUGA1UECxMuQ2xhc3MgMiBQdWJsaWMgUHJpbWFyeSBDZXJ0aWZp"
"Y2F0aW9uIEF1dGhvcml0eTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAtlqL"
"ow1qI4OAa885h/QhEzMGTCWi7VUSl8WngLn6g8EgoPovFQ18oWBrfnks+gYPOq72"
"G2+x0v8vKFJfg31LxHq3+GYfgFT8t8KOWUoUV0bRmpO+QZEDuxWAk1zr58wIbD8+"
"s0r8/0tsI9VQgiZEGY4jw3HqGSRHBJ51v8imAB8CAwEAATANBgkqhkiG9w0BAQIF"
"AAOBgQB7r6QcL8CbDjtc/Kjm0ZYPSHJJheWvGiMA4+m7gDRssj+EqDxycLNM3nP6"
"fITSkqUANwnCAzQjA7ftdpbcPk+F/VgX9AS+7FEe3Hrb267oYXjaZThHrB0DcG3p"
"47ugSp9A6rzbc79nTV3GfCBc5+iiCivCCXTXTP7b6WsCY105pw==";

static char vs3_ca_cert[] =
"MIICMTCCAZoCBQKhAAABMA0GCSqGSIb3DQEBAgUAMF8xCzAJBgNVBAYTAlVTMRcw"
"FQYDVQQKEw5WZXJpU2lnbiwgSW5jLjE3MDUGA1UECxMuQ2xhc3MgMyBQdWJsaWMg"
"UHJpbWFyeSBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eTAeFw05NjAxMjkwMDAwMDBa"
"Fw05OTEyMzEyMzU5NTlaMF8xCzAJBgNVBAYTAlVTMRcwFQYDVQQKEw5WZXJpU2ln"
"biwgSW5jLjE3MDUGA1UECxMuQ2xhc3MgMyBQdWJsaWMgUHJpbWFyeSBDZXJ0aWZp"
"Y2F0aW9uIEF1dGhvcml0eTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAyVxZ"
"nvIbigEUtBDfBEDb41evakVAj4QMC9Ez2dkRz+4CWB8l9yqoRAWq7AMfeH+ek7ma"
"AKojfdashaJjRcdyJ8z0TMZ1cdI5709C8HXfCpDGjiBvmA/4rCNfcCk2pMmG57Ga"
"IMtTpYXnPb59mv4kRTPcdhXtD6JxZExlLoFoRacCAwEAATANBgkqhkiG9w0BAQIF"
"AAOBgQB1Zmw+0c2B27X4LzZRtvdCvM1Cr9wO+hVs+GeTVzrrtpLotgHKjLeOQ7RJ"
"Zfk+7r11Ri7J/CVdqMcvi5uPaM+0nJcYwE3vH9mvgrPmZLiEXIqaB1JDYft0nls6"
"NvxMsvwaPxUupVs8G5DsiCnkWRb5zget7Ond2tIxik/W2O8XjQ==";

static char vs4_ca_cert[] =
"MIICMTCCAZoCBQKmAAABMA0GCSqGSIb3DQEBAgUAMF8xCzAJBgNVBAYTAlVTMRcw"
"FQYDVQQKEw5WZXJpU2lnbiwgSW5jLjE3MDUGA1UECxMuQ2xhc3MgNCBQdWJsaWMg"
"UHJpbWFyeSBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0eTAeFw05NjAxMjkwMDAwMDBa"
"Fw05OTEyMzEyMzU5NTlaMF8xCzAJBgNVBAYTAlVTMRcwFQYDVQQKEw5WZXJpU2ln"
"biwgSW5jLjE3MDUGA1UECxMuQ2xhc3MgNCBQdWJsaWMgUHJpbWFyeSBDZXJ0aWZp"
"Y2F0aW9uIEF1dGhvcml0eTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEA0LJ1"
"9njQrlpQ9OlQqZ+M1++RlHDo0iSQdomF1t+s5gEXMoDwnZNHvJplnR+Xrr/phnVj"
"IIm9gFidBAydqMEk6QvlMXi9/C0MN2qeeIDpRnX57aP7E3vIwUzSo+/1PLBij0pd"
"O92VZ48TucE81qcmm+zDO3rZTbxtm+gVAePwR6kCAwEAATANBgkqhkiG9w0BAQIF"
"AAOBgQBT3dPwnCR+QKri/AAa19oM/DJhuBUNlvP6Vxt/M3yv6ZiaYch6s7f/sdyZ"
"g9ysEvxwyR84Qu1E9oAuW2szaayc01znX1oYx7EteQSWQZGZQbE8DbqEOcY7l/Am"
"yY7uvcxClf8exwI/VAx49byqYHwCaejcrOICdmHEPgPq0ook0Q==";

static char att_ca_cert[] =
"MIIB6TCCAVICBQKXAAABMA0GCSqGSIb3DQEBAgUAMDsxCzAJBgNVBAYTAlVTMQ0w"
"CwYDVQQKFARBVCZUMR0wGwYDVQQLExRDZXJ0aWZpY2F0ZSBTZXJ2aWNlczAeFw05"
"NjAxMjkwMDAwMDBaFw05OTEyMzEwMDAwMDBaMDsxCzAJBgNVBAYTAlVTMQ0wCwYD"
"VQQKFARBVCZUMR0wGwYDVQQLExRDZXJ0aWZpY2F0ZSBTZXJ2aWNlczCBnzANBgkq"
"hkiG9w0BAQEFAAOBjQAwgYkCgYEA4B4BZBu4FMf+4eYIognBkdYGm/KT2cN+qWZB"
"nFFn7xNmoQUnAcoQS/l9dS2pOoa+N51yGUdEDGRHRk8qtaC1FYvTTmPDA/jH6hSX"
"uSkutmQZWLsAzL0I1jnCcU+e3uiHYTCF4NX8fNWGKtuTFvAYwzZguuU8xhBGydV8"
"rke2aCECAwEAATANBgkqhkiG9w0BAQIFAAOBgQAY8kGLnBFYFSdCZIRd+3VxQYee"
"T8ekWxvpJOT/lHGwKmrE/FAr2KjQbr4JYD+IxBw3afgkntjGDiU09HqhRJl3W4C3"
"d9IxMPH/S5CBnIAy+qhiZP8dGi0D4Ou3Tbvjo43eTIUNw5iypGttGH/89CAqYH6a"
"lXNGm5lfSbfqcD3tig==";

static char att_directory_ca_cert[] =
"MIIB3zCCAUgCAQAwDQYJKoZIhvcNAQEEBQAwOTELMAkGA1UEBhMCVVMxDTALBgNV"
"BAoUBEFUJlQxGzAZBgNVBAsUEkRpcmVjdG9yeSBTZXJ2aWNlczAeFw05NjAxMTgy"
"MTAzNTJaFw0wMTAxMTYyMTAzNTJaMDkxCzAJBgNVBAYTAlVTMQ0wCwYDVQQKFARB"
"VCZUMRswGQYDVQQLFBJEaXJlY3RvcnkgU2VydmljZXMwgZ0wDQYJKoZIhvcNAQEB"
"BQADgYsAMIGHAoGBAIdkcokLII+HJ6zGIv4AQGlIr8aGzSMz4xHFMRoffp6SE7ai"
"rOOwHyoHbLbU3kv68aKgfc5Lvr4mSAmMhRHeyyLnwu5EUf5n1Vta4BY3VAS4OzIS"
"lIOesU2AbKSpdqy4pJf3qwtspUO6bk/FTgAwFjw/mRTaoiAIi7rtdqyXANVtAgEP"
"MA0GCSqGSIb3DQEBBAUAA4GBADhQHQrTG7uin2yNEKpCGwWN5CWr+1WubbpTZxUH"
"muxVn3KJXySw28pkvWSqwozZPaJFt8aScVHv7eFRVJdWNaHO5ETER2b/kdqInCPC"
"s9RiSryUVZyAjrPdTxrtElq1Lrz4S87G1HCzsyL4Xlw2eqa4OXNGQ1ybmr0efqcE"
"zyU2";

static char bbn1_ca_cert[] =
"MIICLTCCAZYCBQKpAAABMA0GCSqGSIb3DQEBBAUAMF0xCzAJBgNVBAYTAlVTMSEw"
"HwYDVQQKExhCQk4gQ2VydGlmaWNhdGUgU2VydmljZXMxKzApBgNVBAMTIkJCTiBD"
"ZXJ0aWZpY2F0ZSBTZXJ2aWNlcyBDQSBSb290IDEwHhcNOTYwMjE1MjExMDI4WhcN"
"OTkxMjI1MjM1OTAwWjBdMQswCQYDVQQGEwJVUzEhMB8GA1UEChMYQkJOIENlcnRp"
"ZmljYXRlIFNlcnZpY2VzMSswKQYDVQQDEyJCQk4gQ2VydGlmaWNhdGUgU2Vydmlj"
"ZXMgQ0EgUm9vdCAxMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCumxTY8sfW"
"G+oIQzC8UZXF9srcCW6HCHQnkUuJX+qgINcFWfDZczCGReaMZv4QNCroxL3hRFEU"
"caJwZu2PM9fbwzgCDGFLIfR7Ic7gymjhQnwFJvto86DkJjkVUFeGOdniZSZAWxGI"
"+quBj2pF1g9dduEouvoLYQthcAFcgZHCBQIDAQABMA0GCSqGSIb3DQEBBAUAA4GB"
"AEioN5Rz5i0PHaMe8PhXVAxUvtrcfYppV02qHOukBzRS4Jd+bHGHhP/AWO/hn/Ds"
"6nwT/0FVK5OsXCwaIjelUYf7nhJnJ/XPvDamTL1anx6KWuuF7nBrQ/1hWHc1lWsv"
"cN69hymuI8KmO5S5sBPWTsrb1BV6FRPjlVR1DLU+RNMH";

static char gte_root_ca_cert[] =
"MIIB9TCCAV4CAQAwDQYJKoZIhvcNAQEEBQAwRTELMAkGA1UEBhMCVVMxGDAWBgNV"
"BAoTD0dURSBDb3Jwb3JhdGlvbjEcMBoGA1UEAxMTR1RFIEN5YmVyVHJ1c3QgUm9v"
"dDAaFws5NjAyMjMxOTE1WhcLOTkxMjMxMjM1OVowRTELMAkGA1UEBhMCVVMxGDAW"
"BgNVBAoTD0dURSBDb3Jwb3JhdGlvbjEcMBoGA1UEAxMTR1RFIEN5YmVyVHJ1c3Qg"
"Um9vdDCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAuOZPutuYfHF8r0S30w9G"
"2WTlk8FCjse6SY01LXrni73lBTFZxrEvCgz7n6c/oglmhFYeNykbh+l+DMqan6V/"
"9RWUo9WiRoLYaEzRNxUGaK+9+LCz8Cn1lVoJFmF3CiIl1E9Fqse95Zbf+dSojkLM"
"JMAekSdKtW0GgGM5xKJeOAMCAwEAATANBgkqhkiG9w0BAQQFAAOBgQA0mwkFnUJI"
"f3wkjsKIZXhrTSoOr+3t8GotN+tXUQ6KrS0TAqFAakH2aXK/D6BOVeeCpleRi11D"
"CMn4cQeC4z4UASQMOethIveiC0IjFMoQGDYKBdHISvEOs/0A0MbuVfJMYrQZcx9I"
"GNiwTrB1vLmEix+WioPxoBqi7f+HjKNPog==";

static char keywitness_ca_cert[] =
"MIICHTCCAYYCARQwDQYJKoZIhvcNAQEEBQAwWDELMAkGA1UEBhMCQ0ExHzAdBgNV"
"BAMTFktleXdpdG5lc3MgQ2FuYWRhIEluYy4xKDAmBgorBgEEASoCCwIBExhrZXl3"
"aXRuZXNzQGtleXdpdG5lc3MuY2EwHhcNOTYwNTA3MDAwMDAwWhcNOTkwNTA3MDAw"
"MDAwWjBYMQswCQYDVQQGEwJDQTEfMB0GA1UEAxMWS2V5d2l0bmVzcyBDYW5hZGEg"
"SW5jLjEoMCYGCisGAQQBKgILAgETGGtleXdpdG5lc3NAa2V5d2l0bmVzcy5jYTCB"
"nTANBgkqhkiG9w0BAQEFAAOBiwAwgYcCgYEAzSP6KuHtmPTp0JM+13qAAkzMwQKv"
"XLYff/pXQm8w0SDFtSEHQCyphsLzZISuPYUu7YW9VLAYKO9q+BvnCxYfkyVPx/iO"
"w7nKmIQOVdAv73h3xXIoX2C/GSvRcqK32D/glzRaAb0EnMh4Rc2TjRXydhARq7hb"
"Lp5S3YE+nGTIKZMCAQMwDQYJKoZIhvcNAQEEBQADgYEAMho1ur9DJ9a01Lh25eOb"
"TWzAhsl3NbprFi0TRkqwMlOhW1rpmeIMhogXTg3+gqxOR+/7/zms7jXI+lI3Ckmt"
"Wa3iiqkcxl8f+G9zfs2gMegMvvVN2bKrihK2MHhoEXwN8UlNo/2y6f8d8JH6VIX/"
"M5Dowb+km6RiRr1hElmYQYk=";

static char usps_ca_cert[] =
"MIIB8TCCAVoCAQIwDQYJKoZIhvcNAQEEBQAwQzELMAkGA1UEBhMCVVMxJTAjBgNV"
"BAoTHFVuaXRlZCBTdGF0ZXMgUG9zdGFsIFNlcnZpY2UxDTALBgNVBAMTBFVTUFMw"
"GhcLOTYwMTAxMDAwMFoXCzk3MTIzMTAwMDBaMEMxCzAJBgNVBAYTAlVTMSUwIwYD"
"VQQKExxVbml0ZWQgU3RhdGVzIFBvc3RhbCBTZXJ2aWNlMQ0wCwYDVQQDEwRVU1BT"
"MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCg79GTDCxk4nE8Obqflpxv2mEG"
"JlP3z7xd0edgE88frtK98Hr3QmVlU1hXw0ZgRkyrUHxH26owUa/fD6S4W5w1nrc4"
"6bieOFstNwVmt0IC3+r9ALTVV7hJHsKIP8R451oQdfGRAqjWB0DMYk11cAFwb+pY"
"dBBEYZqOmYkpdAVD7wIDAQABMA0GCSqGSIb3DQEBBAUAA4GBAF4h8pGCvcar8qJC"
"afNK53JE637J1I9nXiVlRqAxvYjbuLIvzOavSZH58essL9uLBcM7vhhJtRaA4PSs"
"X7amt+/qNpgOuglb/UVPwBA4a7fBD/YUw4w0teh/O2QMEotd6pul0CS8ItUpyOQ+"
"ifF+3lMNUDtTPttXYdkpZpBirl5W";

static char canada_post_ca_cert[] =
"MIICoDCCAgmgAwIBAgIEManqojANBgkqhkiG9w0BAQQFADAyMQswCQYDVQQGEwJD"
"QTEjMCEGA1UEChMaQ2FuYWRhIFBvc3QgQ29ycG9yYXRpb24gQ0EwHhcNOTYwNTI3"
"MTg0NzEzWhcNMTYwNTI3MTg0NzEzWjAyMQswCQYDVQQGEwJDQTEjMCEGA1UEChMa"
"Q2FuYWRhIFBvc3QgQ29ycG9yYXRpb24gQ0EwgZ0wDQYJKoZIhvcNAQEBBQADgYsA"
"MIGHAoGBANaOhvxpCFOOAL6o+31+Ocpua+x9vObeuc+RiplHOsBFW3BLdqJti5TU"
"NL6IjaJ5NJyQxmWd7A/SyaaO4DRFYYioS0pbXaXzHhC1j4rEC9OU/HCDjE5rHnjR"
"f5M++fWRSfywcEa3xsK2OlDrZyCLtw44MnetfR6rOkCgIJi0RASHAgEDo4HEMIHB"
"ME4GA1UdGQRHMEUwQzBBMQswCQYDVQQGEwJDQTEjMCEGA1UEChMaQ2FuYWRhIFBv"
"c3QgQ29ycG9yYXRpb24gQ0ExDTALBgNVBAMTBENSTDEwFAYDVR0BBA0wC4AJODMz"
"MjE5MjM0MCsGA1UdAgQkMCIECTgzMzIxOTIzNAMCAgQwEYEPMjAxNjA1MjcxODQ3"
"MTNaMA0GA1UdCgQGMAQDAgeAMB0GCSqGSIb2fQdBAAQQMA4bCHYyLjFhLkIxAwIH"
"gDANBgkqhkiG9w0BAQQFAAOBgQC1NHsoRqGKQ3x6osylZo2A5XftvpVAHj/y1QTz"
"55M2foIxTWrDN599E74usALfIxrXYBJfWSlmtRRXl0qpAkTs+NH0eboEq52Z1Vy6"
"zXFzNrgR/tQIc958XwgwEpUkftfYL22kaA3aKw4Uc70ZZhWe5LeVhQSmavPhoI9f"
"hozpMQ==";

static char mci_mall_ca_cert[] =
"MIIB8zCCAVwCBQJtAABCMA0GCSqGSIb3DQEBAgUAMEAxCzAJBgNVBAYTAlVTMQww"
"CgYDVQQKEwNNQ0kxFDASBgNVBAsTC2ludGVybmV0TUNJMQ0wCwYDVQQLEwRNQUxM"
"MB4XDTk2MDcxNjAwMDAwMFoXDTk4MDcxNjIzNTk1OVowQDELMAkGA1UEBhMCVVMx"
"DDAKBgNVBAoTA01DSTEUMBIGA1UECxMLaW50ZXJuZXRNQ0kxDTALBgNVBAsTBE1B"
"TEwwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAOMhI19RqC3Aj64Q8G/OSIDy"
"lF6Ig/zkPw03HjTwCAySTaP7y6UG6Z7WNjAGJ8xJiN/Fn1+TbB+pQeyg1NKYdlVv"
"xaOlQkmG9yXGHshDMZH7SebfTjbMbdXg/hiMQ/LrEzmVJ9QrrJjrqQ8tIZtcm1vP"
"HEQZJoFuiO2aY7tWdlFvAgMBAAEwDQYJKoZIhvcNAQECBQADgYEAX+q/3vMnwY9I"
"hiPIX+IggtEOf4U69dyy27k/Tdw22v5YhEzfynrcmxEenGV8ciLk65VblIj+mhAz"
"WwmnpFxPlGJZUGwKvqS9HFxatnBybjEdFf9g5Vxa9isF6YhfqovSNPoIBySSoXSF"
"a0am9n4me9H5GAiTQpNq8SnjcyPWToM=";

static char thawte_basic_ca_cert[] =
"MIIC+TCCAmICAQAwDQYJKoZIhvcNAQEEBQAwgcQxCzAJBgNVBAYTAlpBMRUwEwYD"
"VQQIEwxXZXN0ZXJuIENhcGUxEjAQBgNVBAcTCUNhcGUgVG93bjEdMBsGA1UEChMU"
"VGhhd3RlIENvbnN1bHRpbmcgY2MxKDAmBgNVBAsTH0NlcnRpZmljYXRpb24gU2Vy"
"dmljZXMgRGl2aXNpb24xGTAXBgNVBAMTEFRoYXd0ZSBTZXJ2ZXIgQ0ExJjAkBgkq"
"hkiG9w0BCQEWF3NlcnZlci1jZXJ0c0B0aGF3dGUuY29tMB4XDTk2MDcyNzE4MDc1"
"N1oXDTk4MDcyNzE4MDc1N1owgcQxCzAJBgNVBAYTAlpBMRUwEwYDVQQIEwxXZXN0"
"ZXJuIENhcGUxEjAQBgNVBAcTCUNhcGUgVG93bjEdMBsGA1UEChMUVGhhd3RlIENv"
"bnN1bHRpbmcgY2MxKDAmBgNVBAsTH0NlcnRpZmljYXRpb24gU2VydmljZXMgRGl2"
"aXNpb24xGTAXBgNVBAMTEFRoYXd0ZSBTZXJ2ZXIgQ0ExJjAkBgkqhkiG9w0BCQEW"
"F3NlcnZlci1jZXJ0c0B0aGF3dGUuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCB"
"iQKBgQDTpFBuyP9Wa+bPXbbqDGh1R6KqwtqEJfyo9EdR2oW1IHSUhh4PdcnpCGH1"
"Bm0wbhUZAulSwGLbTZme4moMRDjN/r7jZAlwxf6xaym2L0nIO9QnBCUQly/nkG3A"
"KEKZ10xD3sP1IW1Un13DWOHA5NlbsLjctHvfNjrCtWYiEtaHDQIDAQABMA0GCSqG"
"SIb3DQEBBAUAA4GBAIsvn7ifX3RUIrvYXtpI4DOfARkTogwm6o7OwVdl93yFhDcX"
"7h5t0XZ11MUAMziKdde3rmTvzUYIUCYoY5b032IwGMTvdiclK+STN6NP2m5nvFAM"
"qJT5gC5O+j/jBuZRQ4i0AMYQr5F4lT8oBJnhgafw6PL8aDY2vMHGSPl9+7uf";

static char thawte_premium_ca_cert[] =
"MIIDDTCCAnYCAQAwDQYJKoZIhvcNAQEEBQAwgc4xCzAJBgNVBAYTAlpBMRUwEwYD"
"VQQIEwxXZXN0ZXJuIENhcGUxEjAQBgNVBAcTCUNhcGUgVG93bjEdMBsGA1UEChMU"
"VGhhd3RlIENvbnN1bHRpbmcgY2MxKDAmBgNVBAsTH0NlcnRpZmljYXRpb24gU2Vy"
"dmljZXMgRGl2aXNpb24xITAfBgNVBAMTGFRoYXd0ZSBQcmVtaXVtIFNlcnZlciBD"
"QTEoMCYGCSqGSIb3DQEJARYZcHJlbWl1bS1zZXJ2ZXJAdGhhd3RlLmNvbTAeFw05"
"NjA3MjcxODA3MTRaFw05ODA3MjcxODA3MTRaMIHOMQswCQYDVQQGEwJaQTEVMBMG"
"A1UECBMMV2VzdGVybiBDYXBlMRIwEAYDVQQHEwlDYXBlIFRvd24xHTAbBgNVBAoT"
"FFRoYXd0ZSBDb25zdWx0aW5nIGNjMSgwJgYDVQQLEx9DZXJ0aWZpY2F0aW9uIFNl"
"cnZpY2VzIERpdmlzaW9uMSEwHwYDVQQDExhUaGF3dGUgUHJlbWl1bSBTZXJ2ZXIg"
"Q0ExKDAmBgkqhkiG9w0BCQEWGXByZW1pdW0tc2VydmVyQHRoYXd0ZS5jb20wgZ8w"
"DQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBANI2NmqL18JbntqBQWKPOO5JBFXW0O8c"
"G5UWR+8YSDU6UvQragaPOy/qVuOvho2eF/eetGV1Ak3vywmiIVHYm9Bn0LoNkgYU"
"c9STy5cqAJxcTgy8+hVS/PJEbtoRSm4Iny8t4/mqOoZztkZTWMiJBb2DEbhzP6oH"
"jfRCTedAnRw3AgMBAAEwDQYJKoZIhvcNAQEEBQADgYEAutFIgTRZVYerIZfL9lvR"
"w9Eifvvo5KTZ3h+Bj+VzNnyw4Qc/IyXkPOu6SIiH9LQ3sCmWBdxpe+qr4l77rLj2"
"GYuMtESFfn1XVALzkYgC7JcPuTOjMfIiMByt+uFf8AV8x0IW/Qkuv+hEQcyM9vxK"
"3VZdLbCVIhNoEsysrxCpxcI=";

static char certsign_ca_cert[] =
"MIICLzCCAZgCAQEwDQYJKoZIhvcNAQEEBQAwYDELMAkGA1UEBhMCQlIxLTArBgNV"
"BAoTJENlcnRpU2lnbiBDZXJ0aWZpY2Fkb3JhIERpZ2l0YWwgTHRkYTEiMCAGA1UE"
"CxMZQlIgQ2VydGlmaWNhdGlvbiBTZXJ2aWNlczAeFw05NzAxMDEwMDAwMDBaFw05"
"OTEyMzEyMzU5NTlaMGAxCzAJBgNVBAYTAkJSMS0wKwYDVQQKEyRDZXJ0aVNpZ24g"
"Q2VydGlmaWNhZG9yYSBEaWdpdGFsIEx0ZGExIjAgBgNVBAsTGUJSIENlcnRpZmlj"
"YXRpb24gU2VydmljZXMwgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAMlhcXN2"
"E0NRY6pFj6jgv0yOQL4i8bXx1aPsYCqF8yJVABOdacvOxjsj8GOvVMNkxfxIeyt2"
"8uT9Buhk9G0U9BkzR5ZK5Az+IiSqE7bUc/us/nfoFNo8ViXR5oTsjAIkfMIyhci2"
"ud+iSg+IVT38KaiutSTJJ5vUEITiPSSAosVlAgMBAAEwDQYJKoZIhvcNAQEEBQAD"
"gYEAEhZCBvobbXxhI3GWkLkt52RYccLBFP2tv24T4cbGDf8hVK1zVa4OLFSZ6wnB"
"WKKVquESds7N/09fHxqwKCEJU3F9KE3lrUDfJvLFnpqCvIcMse1bfmrFSBzWhDh3"
"6ck29gn/lSBpdOKL4NYBsbuhwIw3lIJSNVc+0X5zaImWvWg=";

static char gte_secure_server_ca_cert[] =
"MIICJTCCAY4CAU8wDQYJKoZIhvcNAQEEBQAwWzELMAkGA1UEBhMCVVMxGDAWBgNV"
"BAoTD0dURSBDb3Jwb3JhdGlvbjEyMDAGA1UEAxMpR1RFIFNlY3VyZSBTZXJ2ZXIg"
"Q2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNOTcwMjEzMTYwMjEzWhcNOTkxMjMw"
"MjM1OTAwWjBbMQswCQYDVQQGEwJVUzEYMBYGA1UEChMPR1RFIENvcnBvcmF0aW9u"
"MTIwMAYDVQQDEylHVEUgU2VjdXJlIFNlcnZlciBDZXJ0aWZpY2F0aW9uIEF1dGhv"
"cml0eTCBnzANBgkqhkiG9w0BAQEFAAOBjQAwgYkCgYEAseQZFuzriZ+mN7yI6GKO"
"PfAUXT/80hM6J03XXhT/1C/BAa9ziIYclDKoxSHv45+DFl3f5lZBQyfbmAxc8gVT"
"Oq8X9hNDIVuBFUYip7BwoxOvjXC1FK3jWEvLeVTtvCeW9/Uu9MjTIE1AvcwPUuT+"
"ecnhXqUcVuD+3LaEj9vVAqUCAwEAATANBgkqhkiG9w0BAQQFAAOBgQBaUWg15Wnp"
"LLmvndnWDCXedGTLJJdQxOleCX/3ZorwsuZpfEBAfqduxsHowQOgAMzSeuFLSNfU"
"U7hIbxWTjazlrjO7a+lcKYWf6KpFfF7uaYdKZRiEt32ftoag0Pt2qRFr3mvAFwrz"
"TDSYONKgPg9FYF9DhkPsGfXDzgC3cx+qUg==";

static char entrust_gtis_ca_cert[] =
"MIIClzCCAgCgAwIBAgIEM2DfuTANBgkqhkiG9w0BAQQFADAsMQswCQYDVQQGEwJD"
"QTELMAkGA1UEChMCZ2MxEDAOBgNVBAMTB0dUSVMuQ0EwJhcROTcwNDI1MTI0NTQ0"
"KzA1MDAXETE3MDQyNTEyNDU0NCswNTAwMCwxCzAJBgNVBAYTAkNBMQswCQYDVQQK"
"EwJnYzEQMA4GA1UEAxMHR1RJUy5DQTCBnTANBgkqhkiG9w0BAQEFAAOBiwAwgYcC"
"gYEAnpY3wwHvZOMklOj2GQsFQuGfUxs3UTabjlpflY29/OE/XhphGiOvgXWKlOhd"
"Es+QPtv9Hwh41ZDK8FjoVxDxB75W+JA9mKiwFxPP+D6y/2NHmgobS0Rzb9AZxWiQ"
"/GYW8GfJF2YBW8+7VZSZrGihHOUjUropAdkVv/hyK7427BkCAQOjgb8wgbwwSAYD"
"VR0ZBEEwPzA9MDsxCzAJBgNVBAYTAkNBMQswCQYDVQQKEwJnYzEQMA4GA1UEAxMH"
"R1RJUy5DQTENMAsGA1UEAxMEQ1JMMTAUBgNVHQEEDTALgAk4NjE5ODY3NDUwLwYD"
"VR0CBCgwJgQJODYxOTg2NzQ1AwICBDAVgRMyMDE3MDQyNTEyNDU0NCswNTAwMA0G"
"A1UdCgQGMAQDAgeAMBoGCSqGSIb2fQdBAAQNMAsbBXYyLjFkAwIHgDANBgkqhkiG"
"9w0BAQQFAAOBgQB5SVjeXva/2XyNWu3ynves91YagqcwP80jrMGlLuXnIcY1G8jh"
"hKoYbag5HaI6u5mFWiPDu0XRlZ8EdOzgmool5JHEXn0gMmLiFokubkM4bbHidObv"
"xU2KViOBGSZK5xkeQoiSFKkRKb0pQ6lX+3Qq64RcErZ4UTnDYdIHeV6jSg==";

static char entrust_gtis_web_ca_cert[] =
"MIIC1zCCAkCgAwIBAgIEMwTIGTANBgkqhkiG9w0BAQQFADAvMQswCQYDVQQGEwJD"
"QTELMAkGA1UEChMCZ2MxEzARBgNVBAsTCkdUSVMuV2ViQ0EwHhcNOTcwMjE0MjAx"
"NjI0WhcNMDIwMjE0MjAxNjI0WjAvMQswCQYDVQQGEwJDQTELMAkGA1UEChMCZ2Mx"
"EzARBgNVBAsTCkdUSVMuV2ViQ0EwgZ0wDQYJKoZIhvcNAQEBBQADgYsAMIGHAoGB"
"AL8/DPpoIBgpc5mcJftJYUDSzUI2j5tY1GQBip5QFXQIFFSYO8AOoXpzE/QOHnzS"
"bStoaARlRu0BNp9mhpmVIsxcsvQYWGmqy5oyZ0VXYcoVD0sajpUP01Iel2BpkG1e"
"4cbLINkASKtEnGTaYRi/EMGHCCASgidLka9mBv7J7dsNAgEDo4IBADCB/TAfBgNV"
"HSMEGDAWgBQ2QWSJWcYO7DEQlKpkiyobwyRSrTAdBgNVHQ4EFgQUNkFkiVnGDuwx"
"EJSqZIsqG8MkUq0wCwYDVR0PBAQDAgEGMBoGA1UdEAQTMBGBDzIwMDIwMjE0MjAx"
"NjI0WjAMBgNVHRMEBTADAQH/MFEGA1UdHwRKMEgwRqBEoEKkQDA+MQswCQYDVQQG"
"EwJDQTELMAkGA1UEChMCZ2MxEzARBgNVBAsTCkdUSVMuV2ViQ0ExDTALBgNVBAMT"
"BENSTDEwHgYJKoZIhvZ9B0EABBEwDxsJV0VCQ0EgMS4wAwIGwDARBglghkgBhvhC"
"AQEEBAMCAQYwDQYJKoZIhvcNAQEEBQADgYEAVCd1Jtu7ewlCbc+0Bzvlr/v6Pysx"
"Yii3KrD4aK057zSxj4eE4Enu27xKS0VDEZyHvVCRTdlUp14lYMn+ky45skr9H8UH"
"ZZzfrhfEiZW5TBTjgEpC+xTYugj52JZYf6pNgq+yWvpjSSA3ekgUCqKAqDK06oy5"
"6RtyijZjq0z2ec4=";

static char uptime_class1_ca_cert[] =
"MIIDojCCAooCAQAwDQYJKoZIhvcNAQEEBQAwgZYxCzAJBgNVBAYTAlVLMQ8wDQYD"
"VQQIEwZMb25kb24xGTAXBgNVBAoTEFVwdGltZSBHcm91cCBQbGMxHDAaBgNVBAsT"
"E1VwdGltZSBDb21tZXJjZSBMdGQxFzAVBgNVBAMTDlVUQyBDbGFzcyAxIENBMSQw"
"IgYJKoZIhvcNAQkBFhVjZXJ0c0B1cHRpbWVncm91cC5jb20wHhcNOTcwNDIyMTQ1"
"MDEwWhcNMDIwNDIxMTQ1MDEwWjCBljELMAkGA1UEBhMCVUsxDzANBgNVBAgTBkxv"
"bmRvbjEZMBcGA1UEChMQVXB0aW1lIEdyb3VwIFBsYzEcMBoGA1UECxMTVXB0aW1l"
"IENvbW1lcmNlIEx0ZDEXMBUGA1UEAxMOVVRDIENsYXNzIDEgQ0ExJDAiBgkqhkiG"
"9w0BCQEWFWNlcnRzQHVwdGltZWdyb3VwLmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD"
"ggEPADCCAQoCggEBAPQoQsO2kFXOCSTPns0SyKv/r4YghHaifW/ApwiNEW1xA0aG"
"qBbKVxX1TceS7Jg+lpmDtSxc5F6Nlsc58ZXCBakB1smBAvfwYgt2B5vUDeQVFZtM"
"bfJT70xpIT+iPLdZscCSVvMqcrGbgyeXqH0LhWcKnT2G4jcHy7eoOkDolng3H1jp"
"gkabA1e+0zMF7nOMgb1m90yab1M3IfW0UzqR8LmZONb9ZaLu0vAsbRdJ7XXkz5XF"
"shDHw4qXmQuk+A2HWSe/S0Q+3pb9Cwt3ys5pDxIR+FYjHuBc6tQE8wGyNM2k4yLA"
"zj7JnTL8HOJHd0fqGiyjARH9qaRWVeDXAphQmiMCAwEAATANBgkqhkiG9w0BAQQF"
"AAOCAQEALclv/qbaCcTqIuRw3VvQPubyszQqnLwXxq2LUL5EC+dkyq1leTQhMB16"
"eIorl8nQFSr2c8cjLjnbgIVTdssrlgzWBZrX/rtJA/qgavK7ncoKrcjGl9wSolKd"
"0xOVPkpxAzQJv1T/FQkgHXpV18q8jFq5mx9vs9o/vzogDD7Vw1B2uHkT/IcTJN4h"
"OTKLQJnJXJgqwxeX6BFYFX2s4m2To8i9B/vorAj3Ak0wwwLOP0xsrEvbx2wcrE+i"
"FHdRSG2rNPAc7MoYeQkwZ/U7fZ4niaqgghcxgn7ItiCTrvj1UBXgb6lYkSnM0hbH"
"pw1V0iqRo6JIHWeVbrc4Ek4XXOEASg==";

static char uptime_class2_ca_cert[] =
"MIIDojCCAooCAQAwDQYJKoZIhvcNAQEEBQAwgZYxCzAJBgNVBAYTAlVLMQ8wDQYD"
"VQQIEwZMb25kb24xGTAXBgNVBAoTEFVwdGltZSBHcm91cCBQbGMxHDAaBgNVBAsT"
"E1VwdGltZSBDb21tZXJjZSBMdGQxFzAVBgNVBAMTDlVUQyBDbGFzcyAyIENBMSQw"
"IgYJKoZIhvcNAQkBFhVjZXJ0c0B1cHRpbWVncm91cC5jb20wHhcNOTcwNDIyMTUw"
"NzQ4WhcNMDIwNDIxMTUwNzQ4WjCBljELMAkGA1UEBhMCVUsxDzANBgNVBAgTBkxv"
"bmRvbjEZMBcGA1UEChMQVXB0aW1lIEdyb3VwIFBsYzEcMBoGA1UECxMTVXB0aW1l"
"IENvbW1lcmNlIEx0ZDEXMBUGA1UEAxMOVVRDIENsYXNzIDIgQ0ExJDAiBgkqhkiG"
"9w0BCQEWFWNlcnRzQHVwdGltZWdyb3VwLmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD"
"ggEPADCCAQoCggEBAMATJMT7b+8pbfgG2KVEEdnRQnC2+db7JOI1dWm4To9olUuc"
"3TlquvJ4ZsmK+3dVbh9xow4LTQCnImwqnAWzr3geHvLFP01IfVWgpWPFZ5hrMx6X"
"xPRJ5yDpYvusMn7MS0pQjKmdKCW9o+fnoLj168ffBtRozXg5MbWrFeQ8pkTRDItz"
"rLoNlCZpHPqqctaabJ5CmNbGjMfScrezxbTe/VsZFOGnW5F0p5BvV7AeclUkbZsu"
"A/zt4N8FEPEEgG6qM7ybEwj85Yn0Zf+Lnj/dB+6/h33katp9YRLFbxpDNFPCAIPt"
"FwqkFKm9D69d5rKz6WuhFvcWytnIW6SPFq2BROUCAwEAATANBgkqhkiG9w0BAQQF"
"AAOCAQEAJqXpA+iigG46cQyRmtbp56U+Cr2Ee20fU52LTZYqQx0vO8rPdXCvPZQG"
"lXRVQCN43SIlf8fu9qRGgk57Whsz0ndq6tgGG+SCzYX4Ic6G1A5ABrTKcdujYlcs"
"DZfr578Th55m4T4r5Sl3MVs2nLkCf15GYULJqkiDpICsnvCymLJe1nqmRubO7Fk8"
"Qxjsu+eTmD2sxW80w7n0w0/Gn8LHB/LFnnRmqrnJgwATR+IlW/ojk+GzwGHAL8br"
"IWxjemkBdSYMgFBB/pRYbJI7KY8Cjd/F4DpCYUYrAd3/zywHC0Kpir81ZZYzGz3J"
"7Dm6ulALVW08BkueGo6Dai4V1wiG2A==";

static char uptime_class3_ca_cert[] =
"MIIDojCCAooCAQAwDQYJKoZIhvcNAQEEBQAwgZYxCzAJBgNVBAYTAlVLMQ8wDQYD"
"VQQIEwZMb25kb24xGTAXBgNVBAoTEFVwdGltZSBHcm91cCBQbGMxHDAaBgNVBAsT"
"E1VwdGltZSBDb21tZXJjZSBMdGQxFzAVBgNVBAMTDlVUQyBDbGFzcyAzIENBMSQw"
"IgYJKoZIhvcNAQkBFhVjZXJ0c0B1cHRpbWVncm91cC5jb20wHhcNOTcwNDIyMTUx"
"ODI5WhcNMDIwNDIxMTUxODI5WjCBljELMAkGA1UEBhMCVUsxDzANBgNVBAgTBkxv"
"bmRvbjEZMBcGA1UEChMQVXB0aW1lIEdyb3VwIFBsYzEcMBoGA1UECxMTVXB0aW1l"
"IENvbW1lcmNlIEx0ZDEXMBUGA1UEAxMOVVRDIENsYXNzIDMgQ0ExJDAiBgkqhkiG"
"9w0BCQEWFWNlcnRzQHVwdGltZWdyb3VwLmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD"
"ggEPADCCAQoCggEBAKsfYAK/UDNcXPl7ZczrjYEt72odEFfczb4IkL16VrWd4DRi"
"BPcEZerbrNf6qjv/zl8Z3iYMGCskLsqEGIFdIHwHwyFTFvqn7o/eyiBtxIf63Icl"
"JiWf3/1A2x5GZ5U09IRxacpKl0uKuq3k1HyyCKeL9tzfFHRoaV+NoGXhcP8CQa2x"
"xCmdUoi+XWweB0RB370O/i1gjqXu+lh3K6U7nxMiSnrwI9LVEWwuqF14MiZLExl/"
"2vjr8N1UDUjRlFMdE7cX7y8XMtmrmy2OgnoDP1C0Kbrx4koBr53JBebdpn8Iaxkx"
"WCnS8mBbDUVhkcBWs0UeW+qMSxFb1NcbMxO+0WECAwEAATANBgkqhkiG9w0BAQQF"
"AAOCAQEAfm14amDVZ9FJOan0Lljfzuy2HS9ZheUC8AbKsy5oB1wrn0P2v+R84b71"
"FFAibwJEG62u4k5Y0vDGAuhl3vWVjLkEK4V4b4utZ7Y4uGmdEdWAt4usl7u4pcdh"
"A+1JEt0kCbfPhRQkOlNSpgLPMgvNiSOHF7T6j255CpUfp6YHuf5SeZJYBdnk2a1H"
"nQ7RUT6H5fHvESy0Qa0Pg0pOrvyh8ym0TLSFuCwUhvJT0uFYh/2pYX10SqAQ1ayQ"
"FavlHeYT9rHjS3Hukl3BtKeHREuqgknWMxs+Mo/kEGoAkQo94gACH/IDp67sjQQb"
"01EjJ6AehwjKiYwU5C1oI5PJRPLb1Q==";

static char uptime_class4_ca_cert[] =
"MIIDojCCAooCAQAwDQYJKoZIhvcNAQEEBQAwgZYxCzAJBgNVBAYTAlVLMQ8wDQYD"
"VQQIEwZMb25kb24xGTAXBgNVBAoTEFVwdGltZSBHcm91cCBQbGMxHDAaBgNVBAsT"
"E1VwdGltZSBDb21tZXJjZSBMdGQxFzAVBgNVBAMTDlVUQyBDbGFzcyA0IENBMSQw"
"IgYJKoZIhvcNAQkBFhVjZXJ0c0B1cHRpbWVncm91cC5jb20wHhcNOTcwNDIyMTUy"
"NjEzWhcNMDIwNDIxMTUyNjEzWjCBljELMAkGA1UEBhMCVUsxDzANBgNVBAgTBkxv"
"bmRvbjEZMBcGA1UEChMQVXB0aW1lIEdyb3VwIFBsYzEcMBoGA1UECxMTVXB0aW1l"
"IENvbW1lcmNlIEx0ZDEXMBUGA1UEAxMOVVRDIENsYXNzIDQgQ0ExJDAiBgkqhkiG"
"9w0BCQEWFWNlcnRzQHVwdGltZWdyb3VwLmNvbTCCASIwDQYJKoZIhvcNAQEBBQAD"
"ggEPADCCAQoCggEBAMhtrrgyds49zudjK8OFALva1ADqhhpmPiZHIVzkcsf5TF7s"
"zGb6PBC/noZxBdHs406LYZ655UC24Iub6LVn3XiSEqhJNmKgnBH2gM94mcZR3KIp"
"LZBYdyocBJFQ0cWlsZrAy60UNWVoncVg+Uvp4qDwjz4DPFRadRAb9Mmpswk9Wr3c"
"oqgj46E/mQp+ne8oGCBSKrFF5TqTNy7Dj2wvpfTLeIkecOZ5HvIldBqZJ0Zf+/ps"
"JaID3B8ACiZyGlwS2edHVmfNHe15FMTllx+EyNq80vwjHIMZhENRhkNDXPuXOeE5"
"4YNFWaK9isFZi31Rw2wdw46YwkKOHzj4QalMwbECAwEAATANBgkqhkiG9w0BAQQF"
"AAOCAQEAKUDQukOPK5jK6oEpJYB7TX1E9yhB26hgHeK61oag+iBEcFK1Fk5wQWbZ"
"F97uJK7ofwpE1DRSXpvccvLiZIKngN7Ca7jttAoQYvRkc6xj8c+pbInvinITME74"
"2mazPeuvdgl5DwNv/9Htq8P73WEQXW/LX/H1BGX8RnFBKRaeKPIMnyRLb49f5/Lz"
"Fhrwb4ZQnl20ktUEkNeep8E9LoWomUajKOLX+Lcm0bAqVIESNVam7/cPeoOpLa8R"
"8ZmCOSPSy+qLUf+7yMVVmrCtmGMSXdqBpCTvZLBU41x+K7sYme8CXl5weDQZTn2c"
"fK8/9TT8cxbw+gczPRMCC1Xu95tU2A==";

static char ns_export_policy_ca_cert[] =
"MIIDBzCCAnACBQK0AAABMA0GCSqGSIb3DQEBAgUAMIHJMR8wHQYDVQQKExZWZXJp"
"U2lnbiBUcnVzdCBOZXR3b3JrMSwwKgYDVQQLEyNOZXRzY2FwZSBDb21tdW5pY2F0"
"aW9ucyBDb3Jwb3JhdGlvbjEtMCsGA1UECxMkTmV0c2NhcGUgRXhwb3J0IENvbnRy"
"b2wgQ0EgLSBDbGFzcyAzMUkwRwYDVQQLE0B3d3cudmVyaXNpZ24uY29tL0NQUyBJ"
"bmNvcnAuYnkgUmVmLiBMSUFCSUxJVFkgTFRELihjKTk3IFZlcmlTaWduMB4XDTk3"
"MDQxNzAwMDAwMFoXDTk5MDQxNzIzNTk1OVowgckxHzAdBgNVBAoTFlZlcmlTaWdu"
"IFRydXN0IE5ldHdvcmsxLDAqBgNVBAsTI05ldHNjYXBlIENvbW11bmljYXRpb25z"
"IENvcnBvcmF0aW9uMS0wKwYDVQQLEyROZXRzY2FwZSBFeHBvcnQgQ29udHJvbCBD"
"QSAtIENsYXNzIDMxSTBHBgNVBAsTQHd3dy52ZXJpc2lnbi5jb20vQ1BTIEluY29y"
"cC5ieSBSZWYuIExJQUJJTElUWSBMVEQuKGMpOTcgVmVyaVNpZ24wgZ8wDQYJKoZI"
"hvcNAQEBBQADgY0AMIGJAoGBALwnuUxWcOzn6U1Nioaf8Kg/EtuVGiBO7csUAOZF"
"scE07pg44rOKpUmo9YaGcsZyuxjdXaq8IWydgt3s/iGsPaH1mec6nf2U1Le0x7tc"
"OOuqCNp7Hf3+UcsgPM+JHJ4vbSIWW1PaZe4ZN/Zz5Gp6RhIpu9cvV9NtBS4i/rxC"
"sDR/AgMBAAEwDQYJKoZIhvcNAQECBQADgYEAXfMdcvO0JGcMgBfGILFkV+WnWM6A"
"oq0ABSxy+Mo6O+AJjyUQI+/7AGPh9cVcJgPlCL0ZSvq7N0XO+F8b04KTS0k+TjlK"
"5jOaJxEvGCHQoyX0xOP9eYWH1HpH5BfeEG7DKigPj12DTOcP71haxwu5hMx7WR6P"
"U2SOtUcNapnMsOM=";

static char ns_beta4_os_ca_cert[] =
"MIICHTCCAYYCAQEwDQYJKoZIhvcNAQEEBQAwVzEsMCoGA1UEChMjTmV0c2NhcGUg"
"Q29tbXVuaWNhdGlvbnMgQ29ycG9yYXRpb24xJzAlBgNVBAsTHk5ldHNjYXBlIFBS"
"NCBPYmplY3QgU2lnbmluZyBDQTAeFw05NzA0MjEyMzIwNTZaFw05NzA3MjAyMzIw"
"NTZaMFcxLDAqBgNVBAoTI05ldHNjYXBlIENvbW11bmljYXRpb25zIENvcnBvcmF0"
"aW9uMScwJQYDVQQLEx5OZXRzY2FwZSBQUjQgT2JqZWN0IFNpZ25pbmcgQ0EwgZ8w"
"DQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAKdTKgh8+z0N6dL8FMx4nnYfaRtZghGT"
"WbCxTIP9TUwjU83JurifKHsQKlGm/r+cl3MouZijifgklgpdSJcOk5rpk/wLWXpJ"
"dm7q8lTNQjWcYP+afhYxuwmLUm3mHIegXPWVWfZx13J2hRcqkNr0gYD6MlJjFZrz"
"RHNtmScWGcV5AgMBAAEwDQYJKoZIhvcNAQEEBQADgYEAKS58efa3ieX0yJC3/bf2"
"oOALJzx+BTgsZnJHJohbLT6OQuZqwOAuCPts3IFW44Tq8m6MK0zgN5d5+zemyNJU"
"SE/Ktd1EpTsECZZ1WiwUs5KpqDuCzhWWuJIOszy/I1h9TUaPHJ2IsG6KKP3sEQE5"
"CdpgdK0UQJ0R0MS7Mye9rx0=";

#define DEFAULT_TRUST_FLAGS (CERTDB_VALID_CA | \
			     CERTDB_TRUSTED_CA | \
			     CERTDB_NS_TRUSTED_CA)

typedef enum {
    certUpdateNone,
    certUpdateAdd,
    certUpdateDelete,
    certUpdateAddTrust,
    certUpdateRemoveTrust
} certUpdateOp;

typedef struct {
    char *cert;
    char *nickname;
    CERTCertTrust trust;
    int updateVersion;
    certUpdateOp op;
    CERTCertTrust trustDelta;
} certInitEntry;

static certInitEntry initialcerts[] = {
    {
	certsign_ca_cert,
	"CertiSign BR",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	1, certUpdateAdd,
        { 0, 0, 0 },
    },
    {
	gte_secure_server_ca_cert,
	"GTE CyberTrust Secure Server CA",
        { DEFAULT_TRUST_FLAGS, 0, 0 },
	1, certUpdateAdd,
        { 0, 0, 0 },
    },
    {
	entrust_gtis_ca_cert,
	"GTIS/PWGSC, Canada Gov. Secure CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS },
	2, certUpdateAdd,
        { 0, 0, 0 },
    },
    {
	entrust_gtis_ca_cert,
	"GTIS/PWGSC, Canada Gov. Secure CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS },
	3, certUpdateAddTrust,
        { 0, 0, DEFAULT_TRUST_FLAGS },
    },
    {
	entrust_gtis_web_ca_cert,
	"GTIS/PWGSC, Canada Gov. Web CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	1, certUpdateAdd,
        { 0, 0, 0 },
    },
    { /* add it if its not there */
	uptime_class1_ca_cert,
	"Uptime Group Plc. Class 1 CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	1, certUpdateAdd,
        { 0, 0, 0 },
    },
    { /* add SSL trust for people with v1 databases */
	uptime_class1_ca_cert,
	"Uptime Group Plc. Class 1 CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	2, certUpdateAddTrust,
        { DEFAULT_TRUST_FLAGS, 0, 0 },
    },
    {
	uptime_class2_ca_cert,
	"Uptime Group Plc. Class 2 CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	1, certUpdateAdd,
        { 0, 0, 0 },
    },
    {
	uptime_class3_ca_cert,
	"Uptime Group Plc. Class 3 CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	1, certUpdateAdd,
        { 0, 0, 0 },
    },
    { /* add it for people with v0 databases */
	uptime_class4_ca_cert,
	"Uptime Group Plc. Class 4 CA",
        { 0, DEFAULT_TRUST_FLAGS, 0 },
	1, certUpdateAdd,
        { 0, 0, 0 },
    },
    { /* delete ssl flags for people with v1 databases */
	uptime_class4_ca_cert,
	"Uptime Group Plc. Class 4 CA",
        { 0, DEFAULT_TRUST_FLAGS, 0 },
	2, certUpdateRemoveTrust,
        { DEFAULT_TRUST_FLAGS, 0, 0 },
    },
    {
	ns_export_policy_ca_cert,
	"Netscape Export Control Policy CA",
        { 0, 0, DEFAULT_TRUST_FLAGS | CERTDB_INVISIBLE_CA },
	1, certUpdateAdd,
        { 0, 0, 0 },
    },
    {
	ns_beta4_os_ca_cert,
	"Netscape Communicator Beta 4 Object Signing CA",
        { 0, 0, DEFAULT_TRUST_FLAGS | CERTDB_INVISIBLE_CA },
	1, certUpdateAdd,
        { 0, 0, 0 },
    },
    {
	rsa_server_ca_cert,
	"Verisign/RSA Secure Server CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	0, certUpdateNone,
        { 0, 0, 0 },
    },
    {
	mci_mall_ca_cert,
	"MCI Mall CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	0, certUpdateNone,
        { 0, 0, 0 },
    },
    {
	rsa_com_ca_cert,
	"Verisign/RSA Commercial CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	0, certUpdateNone,
        { 0, 0, 0 },
    },
    {
	vs1_ca_cert,
	"VeriSign Class 1 Primary CA",
        { 0, DEFAULT_TRUST_FLAGS, 0 },
	3, certUpdateAdd,
        { 0, 0, 0 },
    },
    {
	vs2_ca_cert,
	"VeriSign Class 2 Primary CA",
        { DEFAULT_TRUST_FLAGS | CERTDB_GOVT_APPROVED_CA,
	      DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS },
	3, certUpdateAddTrust,
        { CERTDB_GOVT_APPROVED_CA, 0, DEFAULT_TRUST_FLAGS },
    },
    {
	vs3_ca_cert,
	"VeriSign Class 3 Primary CA",
        { DEFAULT_TRUST_FLAGS | CERTDB_GOVT_APPROVED_CA,
	      DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS },
	3, certUpdateAddTrust,
        { CERTDB_GOVT_APPROVED_CA, 0, DEFAULT_TRUST_FLAGS },
    },
    {
	vs4_ca_cert,
	"VeriSign Class 4 Primary CA",
        { DEFAULT_TRUST_FLAGS | CERTDB_GOVT_APPROVED_CA,
	      DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS },
	3, certUpdateAddTrust,
        { CERTDB_GOVT_APPROVED_CA, 0, DEFAULT_TRUST_FLAGS },
    },
    {
	att_ca_cert,
	"AT&T Certificate Services",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	0, certUpdateNone,
        { 0, 0, 0 },
    },
    {
	att_directory_ca_cert,
	"AT&T Directory Services",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	0, certUpdateNone,
        { 0, 0, 0 },
    },
    {
	bbn1_ca_cert,
	"BBN Certificate Services CA Root 1",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	0, certUpdateNone,
        { 0, 0, 0 },
    },
    {
	gte_root_ca_cert,
	"GTE CyberTrust Root CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	0, certUpdateNone,
        { 0, 0, 0 },
    },
    {
	usps_ca_cert,
	"United States Postal Service CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	0, certUpdateNone,
        { 0, 0, 0 },
    },
    {
	keywitness_ca_cert,
	"KEYWITNESS, Canada CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	0, certUpdateNone,
        { 0, 0, 0 },
    },
    {
	canada_post_ca_cert,
	"Canada Post Corporation CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	0, certUpdateNone,
        { 0, 0, 0 },
    },
    {
	thawte_basic_ca_cert,
	"Thawte Server CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, 0 },
	0, certUpdateNone,
        { 0, 0, 0 },
    },
    {
	thawte_premium_ca_cert,
	"Thawte Premium Server CA",
        { DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS, DEFAULT_TRUST_FLAGS },
	3, certUpdateAddTrust,
        { 0, 0, DEFAULT_TRUST_FLAGS },
    },
    {
	ns_b3_code_signer_cert,
	"Netscape Communicator Beta 3 Code Signer",
        { 0, 0, 0 },
	1, certUpdateDelete,
        { 0, 0, 0 },
    },
    {
	ns_b3_system_principal_signer_cert,
	"Netscape Communicator Beta 3 System Principal Signer",
        { 0, 0, 0 },
	1, certUpdateDelete,
        { 0, 0, 0 },
    },
    {
	0, 0
    }
};

	
static SECStatus
ConvertAndCheckCertificate(CERTCertDBHandle *handle, char *asciicert,
			   char *nickname, CERTCertTrust *trust)
{
    SECItem sdder;
    SECStatus rv;
    CERTCertificate *cert;
    PRBool conflict;
    SECItem derSubject;
    
    /* First convert ascii to binary */
    rv = ATOB_ConvertAsciiToItem (&sdder, asciicert);
    if (rv != SECSuccess) {
	return(rv);
    }

    /*
    ** Inside the ascii is a Signed Certificate.
    */

    cert = NULL;
    
    /* make sure that no conflicts exist */
    conflict = SEC_CertDBKeyConflict(&sdder, handle);
    if ( conflict ) {
	goto done;
    }

    rv = CERT_NameFromDERCert(&sdder, &derSubject);
    if ( rv != SECSuccess ) {
	goto loser;
    }
    
    conflict = SEC_CertNicknameConflict(nickname, &derSubject, handle);
    if ( conflict ) {
	goto done;
    }
    
    cert = CERT_NewTempCertificate(handle, &sdder, NULL, PR_FALSE, PR_TRUE);
    if ( cert == NULL ) {
	goto loser;
    }
	
    rv = CERT_AddTempCertToPerm(cert, nickname, trust);

    CERT_DestroyCertificate(cert);
    
    if (rv == SECSuccess) {
	/*
	** XXX should verify signatures too, if we have the certificate for
	** XXX its issuer...
	*/
    }

done:
    PORT_Free(sdder.data);
    return(rv);

loser:
    return(SECFailure);
}

SECStatus
CERT_InitCertDB(CERTCertDBHandle *handle)
{
    SECStatus rv;
    certInitEntry *entry;
    
    entry = initialcerts;
    
    while ( entry->cert != NULL) {
	if ( entry->op != certUpdateDelete ) {
	    rv = ConvertAndCheckCertificate(handle, entry->cert,
					    entry->nickname, &entry->trust);
	    /* keep going */
	}
	
	entry++;
    }
done:
    CERT_SetDBContentVersion(CERT_DB_CONTENT_VERSION, handle);
    return(rv);
}

static CERTCertificate *
CertFromEntry(CERTCertDBHandle *handle, char *asciicert)
{
    SECItem sdder;
    SECStatus rv;
    CERTCertificate *cert;
    
    /* First convert ascii to binary */
    rv = ATOB_ConvertAsciiToItem (&sdder, asciicert);
    if (rv != SECSuccess) {
	return(NULL);
    }

    /*
    ** Inside the ascii is a Signed Certificate.
    */

    cert = CERT_NewTempCertificate(handle, &sdder, NULL, PR_FALSE, PR_TRUE);

    return(cert);
}

SECStatus
CERT_AddNewCerts(CERTCertDBHandle *handle)
{
    int oldversion;
    int newversion;
    certInitEntry *entry;
    CERTCertTrust tmptrust;
    SECStatus rv;
    CERTCertificate *cert;
    
    newversion = CERT_DB_CONTENT_VERSION;
    
    oldversion = CERT_GetDBContentVersion(handle);
    
    if ( newversion > oldversion ) {
	entry = initialcerts;
    
	while ( entry->cert != NULL ) {
	    if ( entry->updateVersion > oldversion ) {
		switch ( entry->op ) {
		  default:
		    break;
		  case certUpdateAdd:
		    rv = ConvertAndCheckCertificate(handle, entry->cert,
						    entry->nickname,
						    &entry->trust);
		    break;
		  case certUpdateDelete:
		    cert = CertFromEntry(handle, entry->cert);
		    if ( cert != NULL ) {
			if ( cert->isperm ) {
			    rv = SEC_DeletePermCertificate(cert);
			}
			CERT_DestroyCertificate(cert);
		    }
		    break;
		  case certUpdateAddTrust:
		    cert = CertFromEntry(handle, entry->cert);
		    if ( cert != NULL ) {
			if ( cert->isperm ) {
			    tmptrust = *cert->trust;
			    tmptrust.sslFlags |= entry->trustDelta.sslFlags;
			    tmptrust.emailFlags |=
				entry->trustDelta.emailFlags;
			    tmptrust.objectSigningFlags |=
				entry->trustDelta.objectSigningFlags;
			    rv = CERT_ChangeCertTrust(handle, cert, &tmptrust);
			}
			CERT_DestroyCertificate(cert);
		    }
		    break;
		  case certUpdateRemoveTrust:
		    cert = CertFromEntry(handle, entry->cert);
		    if ( cert != NULL ) {
			if ( cert->isperm ) {
			    tmptrust = *cert->trust;
			    tmptrust.sslFlags &=
				(~entry->trustDelta.sslFlags);
			    tmptrust.emailFlags &=
				(~entry->trustDelta.emailFlags);
			    tmptrust.objectSigningFlags &=
				(~entry->trustDelta.objectSigningFlags);
			    rv = CERT_ChangeCertTrust(handle, cert, &tmptrust);
			}
			CERT_DestroyCertificate(cert);
		    }
		    break;
		}
	    }
	    
	    entry++;
	}
	
	CERT_SetDBContentVersion(newversion, handle);
    }
    
    return(SECSuccess);
}
