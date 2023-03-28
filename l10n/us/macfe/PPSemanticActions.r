data 'STR#' (42, "Redo Drag descriptions", purgeable) {
	$"0003 0E52 6564 6F20 4472 6167 2043 6F70"            /* ...Redo Drag Cop */
	$"790E 5265 646F 2044 7261 6720 4D6F 7665"            /* y.Redo Drag Move */
	$"0E52 6564 6F20 4472 6167 2044 726F 70"              /* .Redo Drag Drop */
};

data 'STR#' (43, "Undo Drag descriptions", purgeable) {
	$"0003 0E55 6E64 6F20 4472 6167 2043 6F70"            /* ...Undo Drag Cop */
	$"790E 556E 646F 2044 7261 6720 4D6F 7665"            /* y.Undo Drag Move */
	$"0E55 6E64 6F20 4472 6167 2044 726F 70"              /* .Undo Drag Drop */
};

data 'STR#' (44, "Redo clipboard descriptions", purgeable) {
	$"0004 0852 6564 6F20 4375 7409 5265 646F"            /* ...Redo Cut∆Redo */
	$"2043 6F70 790A 5265 646F 2050 6173 7465"            /*  Copy.Redo Paste */
	$"0A52 6564 6F20 436C 6561 72"                        /* .Redo Clear */
};

data 'STR#' (45, "Undo clipboard descriptions", purgeable) {
	$"0004 0855 6E64 6F20 4375 7409 556E 646F"            /* ...Undo Cut∆Undo */
	$"2043 6F70 790A 556E 646F 2050 6173 7465"            /*  Copy.Undo Paste */
	$"0A55 6E64 6F20 436C 6561 72"                        /* .Undo Clear */
};

data 'STR#' (46, "Redo typing descriptions", purgeable) {
	$"0001 0B52 6564 6F20 5479 7069 6E67"                 /* ...Redo Typing */
};

data 'STR#' (47, "Undo typing descriptions", purgeable) {
	$"0001 0B55 6E64 6F20 5479 7069 6E67"                 /* ...Undo Typing */
};

