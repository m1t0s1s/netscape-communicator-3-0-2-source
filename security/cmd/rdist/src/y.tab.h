
typedef union
#ifdef __cplusplus
	YYSTYPE
#endif
 {
	opt_t 			optval;
	char 		       *string;
	struct subcmd 	       *subcmd;
	struct namelist        *namel;
} YYSTYPE;
extern YYSTYPE yylval;
# define ARROW 1
# define COLON 2
# define DCOLON 3
# define NAME 4
# define STRING 5
# define INSTALL 6
# define NOTIFY 7
# define EXCEPT 8
# define PATTERN 9
# define SPECIAL 10
# define CMDSPECIAL 11
# define OPTION 12
