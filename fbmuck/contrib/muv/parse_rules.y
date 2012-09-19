%{
/*
Copyright(C) 1990, Marcus J. Ranum, All Rights Reserved.
This software may be freely used, modified, and redistributed,
as long as this copyright message is left intact, and this
software is not used to develop any commercial product, or used
in any product that is provided on a pay-for-use basis.
*/

/*
MUV itself is Copyright(C) 1990, Russ F. Smith.  Framework borrowed
from Marcus Ranum with author's permission.  The obligations implied
and described by Marcus's statement continue in effect for this
code, as well.

I don't even ask that you send me money.  If you feel so inclined, send
me one of those bottles of Lipton iced tea.  Sugar, no lemon.

Credit also to Robert Earl, current hackmeister of TinyMUCK, for his help
on getting this thing 2.2 compatible.

6/93 - Updated and enhanced for FB MUF by Foxen.  (foxen@netcom.com)

Want your name in this space?  Write me some docs. :)
*/

#define	MAXIDENTLEN	20
#define	STRBUFSIZ	2048

#include	<stdio.h>
#include	<ctype.h>
#include	<strings.h>

/*
scratch buffers in which to compile and assemble input.
this is done in a somewhat roundabout manner, to maintain a static area
for compilation.
*/


static	int	stringoff;
static	char	stringbuf[STRBUFSIZ];
static  char	varsbuf[STRBUFSIZ];
static  int	loopnum = 0;

static char buffer[STRBUFSIZ];

char *savestring();
char *indentlines();

/* compiler state exception flag */
%}


%token	NUM STR IDENT
%token	OUTOFSPACE
%token	IF ELSE FUNC RETURN VNULL
%token  TOP PUSH ME LOC
%token	FOR FOREACH WHILE VAR
%token	DO UNTIL CONTINUE BREAK

%right	ASGN ADDASGN SUBASGN MULASGN DIVASGN MODASGN
%left	REF
%left	OR 
%left	AND
%left   KEYVALDELIM
%left	GT GTE LT LTE EQ NE
%left	ADD SUB
%left	MUL DIV MOD
%right	UNARY
%left	NOT

%%
program:
	| program globalstatement { printf("%s\n",$2); }
	| program funcdef { printf("%s\n",$2); }
	;

asgn:     IDENT ASGN expr {
		sprintf(buffer,"%s%s ! ",$3,&stringbuf[$1]);
		$$ = (int)savestring(buffer);
		}
	;

globalstatement: VAR varlist ';' {
		char *a,*b;
		strcpy(buffer,"");
		for (a=(char *)$2;(b=(char *)index(a,',')) != NULL;) {
			*b = 0;
			strcat(buffer,"lvar ");
			strcat(buffer,a);
			strcat(buffer,"\n");
			a = b+1;
		}
		$$ = (int)savestring(buffer);
		}
	;
	
statement: expr ';' { $$ = $1; }
	| RETURN ';' { 
		sprintf(buffer,"exit ");
		$$ = (int)savestring(buffer);
		}
	| RETURN expr ';' {
		sprintf(buffer,"%sexit ",$2);
		$$ = (int)savestring(buffer);
		}
	| BREAK ';' { 
		sprintf(buffer,"break ");
		$$ = (int)savestring(buffer);
		}
	| CONTINUE ';' { 
		sprintf(buffer,"continue ");
		$$ = (int)savestring(buffer);
		}
	| VAR varlist ';' {
		char *a,*b;

		strcpy(buffer,"");
		for (a=(char *)$2;(b=(char *)index(a,',')) != NULL;) {
			*b = 0;
			strcat(buffer,"var ");
			strcat(buffer,a);
			strcat(buffer,"\n");
			a = b+1;
		}
		$$ = (int)savestring(buffer);
		}
	| ';' { $$ = (int)savestring(""); }
	| ifheader condition statement stop {
	    sprintf(buffer,"%sif\n%s\nthen ",$2,indentlines($3));
	    $$ = (int)savestring(buffer);
	    }
	| ifheader condition statement stop ELSE statement stop {
	   sprintf(buffer,"%sif\n%s\nelse\n%s\nthen ",
		    $2,indentlines($3),indentlines($6));
	   $$ = (int)savestring(buffer);
	   }
	| WHILE condition statement stop {
	   sprintf(buffer,"begin\n%s\nwhile\n%s\nrepeat ",
		   indentlines($2),indentlines($3));
	   $$ = (int)savestring(buffer);
	   loopnum++;
	   }
	| DO statement WHILE condition ';' stop {
	   sprintf(buffer,"begin\n%s\n(conditional follows)\n%s\nnot until ",
		   indentlines($2),indentlines($4));
	   $$ = (int)savestring(buffer);
	   loopnum++;
	   }
	| DO statement UNTIL condition ';' stop {
	   sprintf(buffer,"begin\n%s\n(conditional follows)\n%s\nuntil ",
		   indentlines($2),indentlines($4));
	   $$ = (int)savestring(buffer);
	   loopnum++;
	   }
	| FOR '(' comma_expr ';' comma_expr ';' comma_expr ')' statement stop {
	   sprintf(buffer,"%s\nbegin\n%s\nwhile\n%s\n%s\nrepeat ",$3,
		   indentlines($5),indentlines($9),indentlines($7));
	   $$ = (int)savestring(buffer);
	   loopnum++;
	   }
	| FOREACH '(' comma_expr ';' IDENT KEYVALDELIM IDENT ')' statement stop {
	   sprintf(buffer,"%s\nforeach %s! %s!\n%s\nrepeat ",
	       $3, &stringbuf[$7], &stringbuf[$5], indentlines($9));
	   $$ = (int)savestring(buffer);
	   loopnum++;
	   }
	| '{' statements '}' { $$ = $2; }
	;

condition: '(' expr ')' { $$ = $2; } ;

stop: /* nothing */ { $$ = (int)savestring(""); } ;

statements: /* nothing */ { $$ = (int)savestring(""); }
	| statements statement {
			if (*((char *)$1)) {
				if (*((char *)$2))
					sprintf(buffer,"%s\n%s",$1,$2);
				else
					sprintf(buffer,"%s",$1);
			}
			else if (*((char *)$2))
				sprintf(buffer,"%s",$2);
			else
				sprintf(buffer,"");
			$$ = (int)savestring(buffer);
			}
	;

evaluated_element:
	  ME { sprintf(buffer,"me @ ");
		  $$ = (int)savestring(buffer); }
	| LOC { sprintf(buffer,"loc @ ");
		  $$ = (int)savestring(buffer); }
	| IDENT { 
		  sprintf(buffer,"%s @ ",&stringbuf[$1]);
		  $$ = (int)savestring(buffer); }
	;


comma_expr: /* nothing */ { $$ = (int)savestring(""); }
	| expr { $$ = $1; }
	| expr ',' expr {
		sprintf(buffer,"%s\n%s",$1,$3);
		$$ = (int)savestring(buffer);
		}
	| comma_expr ',' expr {
		sprintf(buffer,"%s\n%s",$1,$3);
		$$ = (int)savestring(buffer);
		}
	;

expr:
	NUM	{ sprintf(buffer,"%d ",$1); $$ = (int)savestring(buffer); }
	| '#' '-' NUM {
		sprintf(buffer,"#-%d ",$2); $$ = (int)savestring(buffer);
			}
	| '#' NUM { sprintf(buffer,"#%d ",$2); $$ = (int)savestring(buffer); }
	| STR	{ sprintf(buffer,"\"%s\" ",&stringbuf[$1]);
		  $$ = (int)savestring(buffer); }
	| TOP   { $$ = (int)savestring(""); }
	| evaluated_element { $$ = $1; }
	| asgn { $$ = $1; }
	| PUSH beginargs '(' arglist ')'  { $$ = $4; }
	| IDENT beginargs '(' arglist ')' {
				sprintf(buffer,"%s%s ",$4,&stringbuf[$1]);
				$$ = (int)savestring(buffer);
				}
	| '(' expr ')' { $$ = $2; }
	| IDENT ADDASGN expr {
			sprintf(buffer,"%s @ %s+ %s ! ",
				&stringbuf[$1], $3, &stringbuf[$1]);
			$$ = (int)savestring(buffer);
			}
	| IDENT SUBASGN expr {
			sprintf(buffer,"%s @ %s- %s ! ",
				&stringbuf[$1], $3, &stringbuf[$1]);
			$$ = (int)savestring(buffer);
			}
	| IDENT MULASGN expr {
			sprintf(buffer,"%s @ %s* %s ! ",
				&stringbuf[$1], $3, &stringbuf[$1]);
			$$ = (int)savestring(buffer);
			}
	| IDENT DIVASGN expr {
			sprintf(buffer,"%s @ %s/ %s ! ",
				&stringbuf[$1], $3, &stringbuf[$1]);
			$$ = (int)savestring(buffer);
			}
	| IDENT MODASGN expr {
			sprintf(buffer,"%s @ %s\% %s ! ",
				&stringbuf[$1], $3, &stringbuf[$1]);
			$$ = (int)savestring(buffer);
			}
	| expr ADD expr {
			sprintf(buffer,"%s%s+ ",$1,$3);
			$$ = (int)savestring(buffer);
			}
	| expr SUB expr {
			sprintf(buffer,"%s%s- ",$1,$3);
			$$ = (int)savestring(buffer);
			}
	| expr MUL expr {
			sprintf(buffer,"%s%s* ",$1,$3);
			$$ = (int)savestring(buffer);
			}
	| expr DIV expr {
			sprintf(buffer,"%s%s/ ",$1,$3);
			$$ = (int)savestring(buffer);
			}
	| expr MOD expr {
			sprintf(buffer,"%s%s%% ",$1,$3);
			$$ = (int)savestring(buffer);
			}
	| SUB expr %prec UNARY {
			sprintf(buffer,"%s0 swap - ",$2);
			$$ = (int)savestring(buffer);
			}
	| expr EQ expr {
			sprintf(buffer,"%s%s= ",$1,$3);
			$$ = (int)savestring(buffer);
			}
	| expr NE expr {
			sprintf(buffer,"%s%s= not ",$1,$3);
			$$ = (int)savestring(buffer);
			}
	| expr LT expr {
			sprintf(buffer,"%s%s< ",$1,$3);
			$$ = (int)savestring(buffer);
			}
	| expr LTE expr {
			sprintf(buffer,"%s%s<= ",$1,$3);
			$$ = (int)savestring(buffer);
			}
	| expr GT expr {
			sprintf(buffer,"%s%s> ",$1,$3);
			$$ = (int)savestring(buffer);
			}
	| expr GTE expr {
			sprintf(buffer,"%s%s>= ",$1,$3);
			$$ = (int)savestring(buffer);
			}
	| expr AND expr {
			sprintf(buffer,"%s%sand ",$1,$3);
			$$ = (int)savestring(buffer);
			}
	| expr OR expr {
			sprintf(buffer,"%s%sor ",$1,$3);
			$$ = (int)savestring(buffer);
			}
	| NOT expr %prec UNARY {
			sprintf(buffer,"%snot ",$2);
			$$ = (int)savestring(buffer);
			}
	;



ifheader: IF ;

beginargs: /* nothing */  { $$ = (int)savestring(""); } ;

funcdef: funcheader IDENT '(' varlist ')' statement {
		char *a,*b;
		char tmp[STRBUFSIZ];

		strcpy(buffer,"");
		for (a=(char *)$4;(b=(char *)index(a,',')) != NULL;) {
			*b = 0;
			strcpy(tmp,a);
			strcat(tmp," ");
			strcat(tmp,buffer);
			a = b+1;
		}
		sprintf(buffer,": %s[ %s -- ret ]\n%s\n;\n\n",
		        &stringbuf[$2],
			tmp,
			indentlines($6));
		$$ = (int)savestring(buffer);
	} ;

arglist:
	/* nothing */ { $$ = (int)savestring(""); }
	| expr	{ $$ = $1; }
	| arglist ',' expr {
			sprintf(buffer,"%s%s",$1,$3);
			$$ = (int)savestring(buffer);
			}
	;

varlist:
	/* nothing */ { $$ = (int)savestring(""); }
	| IDENT	{ 
		sprintf(buffer,"%s,",&stringbuf[$1]);
		$$ = (int)savestring(buffer);
		}
	| varlist ',' IDENT {
			sprintf(buffer,"%s%s,",$1,&stringbuf[$3]);
			$$ = (int)savestring(buffer);
			}
	;

funcheader: FUNC ;

%%
FILE	*yyin=NULL;
int	yylineno = 1;


void
setyyinput(f)
FILE	*f;
{
	yyin = f;
}


putinstringbuf(b)
char	*b;
{
	register char	*op = &stringbuf[stringoff];
	int		ooff = stringoff;

	while(*b && stringoff < sizeof(stringbuf)) {
		if(isprint(*b)) {
			*op++ = *b;
			stringoff++;
			if(stringoff >= sizeof(stringbuf))
				return(-1);
		}
		b++;
	}
	 stringoff++;
	*op = '\0';
	return(ooff);
}

/*
search the string buffer for an existing copy of string. in case it is
already there, return the offset (cut down on duplication). if not,
return -1
*/
static	int
searchbuf(s)
char	*s;
{
	register	char	*p = stringbuf;
	register	char	*kp;
	int		strt;
	int		srch = 0;

	while(srch < stringoff) {
		kp = s;
		strt = srch;

		/* match as match can */
		while(*p == *kp && *kp != '\0' && srch < stringoff - 1) {
			p++;
			kp++;
			srch++;
		}

		/* is it a match ? */
		if(*p == '\0' && *kp == '\0' && srch < stringoff - 1) {
			return(strt);
		}

		/* it is not a match, skip to next NULL, and continue */
		while(*p != '\0') {
			p++;
			srch++;
		}

		/* and skip the NULL */
		srch++;
		if(srch < stringoff - 1)
			p++;
	}
	return(-1);
}



static	int
lookup(s,bval)
char	*s;
int	*bval;
{
	int	start	= 0;
	int	ret;

	static	struct	kwordz{
		char	*kw;
		int	rval;
		int	bltin;		/* # of builtin if builtin */
	} keyz[] = {
	/* MUST BE IN LEXICAL SORT ORDER !!!!!! */
	"break",		BREAK,		-1,
	"continue",		CONTINUE,	-1,
	"do",			DO,			-1,
	"else",			ELSE,		-1,
	"for",			FOR,		-1,
	"foreach",		FOREACH,	-1,
	"func",			FUNC,		-1,
	"if",			IF,			-1,
	"loc",			LOC,		-1,
	"me",			ME,			-1,
	"push",			PUSH,		-1,
	"return",		RETURN,		-1,
	"top",			TOP,		-1,
	"until",		UNTIL,		-1,
	"var",			VAR,		-1,
	"while",		WHILE,		-1,
	0,0,0
	};

	int	end	= (sizeof(keyz)/sizeof(struct kwordz)) - 2;
	int	p	= end/2;

	*bval = -1;
	while(start <= end) {
		ret = strcmp(s,keyz[p].kw);
		if(ret == 0) {
			*bval = keyz[p].bltin;
			return(keyz[p].rval);
		}
		if(ret > 0)
			start = p + 1;
		else
			end = p - 1;

		p = start + ((end - start)/2);
	}
	return(-1);
}



yylex()
{
	char	in[BUFSIZ];
	char	*p = in;
	int	c;

	/* handle whitespace */
	while(isspace(c = fgetc(yyin)))
		if(c == '\n')
			yylineno++;
	
	/* handle EOF */
	if(c == EOF) {
		exit(0);
	}

	/* save current char - it is valuable */
	*p++ = c;

	/* handle NUM */
	if(isdigit(c)) {
		int	num;

		num = c - '0';
		while(isdigit(c = fgetc(yyin)))
			num = (num * 10) + (c - '0');

		(void)ungetc(c,yyin);
		if(c == '\n')
			yylineno--;

		yylval = num;
		return(NUM);
	}

	/* handle keywords or idents/builtins */
	if(isalpha(c) || c == '_' || c == '.') {
		int	cnt = 0;
		int	rv;
		int	bltin;

		while((c = fgetc(yyin)) != EOF && (isalnum(c) || c == '_' || c == '?')) {
			if(++cnt + 1 >= MAXIDENTLEN)
				yyerror("identifier too long");
			*p++ = c;
		}

		(void)ungetc(c,yyin);

		*p = '\0';

		if((rv = lookup(in,&bltin)) != -1) {
			return(rv);
		}

		/* if the string is already in the buffer, return that */
		if(stringoff > 0 && (yylval = searchbuf(in)) != -1) {
			return(IDENT);
		}

		if(stringoff + cnt > sizeof(stringbuf)) {
			yyerror("out of string space!");
			return(OUTOFSPACE);
		}
		bcopy(in,&stringbuf[stringoff],cnt + 2);
		yylval = stringoff;
		stringoff += cnt + 2;
		stringbuf[stringoff - 1] = '\0';
		return(IDENT);
	}

	/* handle quoted strings */
	if(c == '"') {
		int	cnt = 0;
		int	quot = c;

		/* strip start quote by resetting ptr */
		p = in;

		/* match quoted strings */
		while((c = fgetc(yyin)) != EOF && c != quot) {
			if(!isascii(c))
				continue;

			if(++cnt + 1 >= sizeof(in))
				yyerror("string too long");

			/* we have to guard the line count */
			if(c == '\n')
				yylineno++;

			if (c == '\\') {
				*p++ = c;
				cnt++;
				c = fgetc(yyin);
			}

			*p++ = c;
		}

		if(c == EOF)
			yyerror("EOF in quoted string");

		*p = '\0';
	
		/* if the string is already in the buffer, return that */
		if(stringoff > 0 && (yylval = searchbuf(in)) != -1) {
			return(STR);
		}

		if(stringoff + cnt > sizeof(stringbuf)) {
			yyerror("out of string space!");
			return(OUTOFSPACE);
		}
		bcopy(in,&stringbuf[stringoff],cnt + 2);
		yylval = stringoff;
		stringoff += cnt + 2;
		stringbuf[stringoff - 1] = '\0';
		return(STR);
	}

	switch(c) {
		case	'=':
			{
				int	t;
				t = fgetc(yyin);
				if(t == '=') {
					return(EQ);
				} else if (t == '>') {
					return(KEYVALDELIM);
				} else {
					(void)ungetc(t,yyin);
				}
			}
			return(ASGN);

		case	'>':
			{
				int	t;
				t = fgetc(yyin);
				if(t != '=') {
					(void)ungetc(t,yyin);
					return(GT);
				}
			}
			return(GTE);

		case	'<':
			{
				int	t;
				t = fgetc(yyin);
				if(t != '=') {
					(void)ungetc(t,yyin);
					return(LT);
				}
			}
			return(LTE);

		case	'!':
			{
				int	t;
				t = fgetc(yyin);
				if(t != '=') {
					(void)ungetc(t,yyin);
					return(NOT);
				}
			}
			return(NE);

		case	'&':
			{
				int	t;
				t = fgetc(yyin);
				if(t != '&') {
					(void)ungetc(t,yyin);
					return(REF);
				}
			}
			return(AND);

		case	'|':
			{
				int	t;
				t = fgetc(yyin);
				if(t != '|') {
					(void)ungetc(t,yyin);
					return(t);
				}
			}
			return(OR);

		case	'%':
			{
				int	t;
				t = fgetc(yyin);
				if(t != '=') {
					(void)ungetc(t,yyin);
					return(MOD);
				}
			}
			return(MODASGN);

		case	'/':
			{
				int	t;
				t = fgetc(yyin);
				if(t != '=') {
					(void)ungetc(t,yyin);
					return(DIV);
				}
			}
			return(DIVASGN);

		case	'*':
			{
				int	t;
				t = fgetc(yyin);
				if(t != '=') {
					(void)ungetc(t,yyin);
					return(MUL);
				}
			}
			return(MULASGN);

		case	'-':
			{
				int	t;
				t = fgetc(yyin);
				if(t != '=') {
					(void)ungetc(t,yyin);
					return(SUB);
				}
			}
			return(SUBASGN);

		case	'+':
			{
				int	t;
				t = fgetc(yyin);
				if(t != '=') {
					(void)ungetc(t,yyin);
					return(ADD);
				}
			}
			return(ADDASGN);
	}

	/* punt */
	if(c == '\n')
		yylineno++;
	return(c);
}

main()
{
	yyin = stdin;
	while (1)
		yyparse();
}

char *
savestring(arg)
char *arg;
{
	
	char *tmp;

	tmp = (char *)malloc(STRBUFSIZ);
	strcpy(tmp,arg);
	return(tmp);
}


char *
indentlines(arg)
char *arg;
{
    char *buf;
    char *ptr, *ptr2;
    int i;

    if (!arg || !*arg) return arg;
    buf = (char *)malloc(STRBUFSIZ);
    ptr = arg;
    ptr2 = buf;
    for (i = 1; i <= 4; i++)
	*ptr2++ = ' ';
    while(*ptr2 = *ptr) {
	if (*ptr == '\n') {
	    for (i = 1; i <= 4; i++)
		*(++ptr2) = ' ';
	}
	ptr++; ptr2++;
    }
    return buf;
}



yyerror(arg)
char *arg;
{
	fprintf(stderr,"%s in line %d\n",arg,yylineno);
}
