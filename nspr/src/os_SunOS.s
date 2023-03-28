	.text

	.global __MD_FlushRegisterWindows
	.global _MD_FlushRegisterWindows

__MD_FlushRegisterWindows:
_MD_FlushRegisterWindows:

	ta	3
	ret
	restore
