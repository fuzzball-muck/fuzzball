/* The Parser of the new MUV.  5/21/2000 */


#define MAX_IDENT_LEN 128

#define ACCESS_NONE    0
#define ACCESS_PRIVATE 1
#define ACCESS_PUBLIC  2
#define ACCESS_WIZCALL 3




/*****************************************************************/


struct parse_state {
	struct line*	curr_line;
	int		lineno;
	int		nowords;
	char*	next_char;
	char*	line_copy;
	struct parse_state* next;
};


void
pushParseState(COMPSTATE* cstat)
{
	struct parse_state* ptr;

	ptr = (struct parse_state*)malloc(sizeof(struct parse_state));
	ptr->lineno = cstat->lineno;
	ptr->nowords = cstat->nowords;
	ptr->curr_line = cstat->curr_line;
	ptr->line_copy = NULL;
	ptr->next_char = ptr->line_copy + (cstat->next_char - cstat->line_copy);
	ptr->next = cstat->parsestate;
	cstat->parsestate = ptr;
}


void
popParseState(COMPSTATE* cstat)
{
	struct parse_state* ptr = cstat->parsestate;

	cstat->parsestate = ptr->next;

	if (ptr->line_copy)
		free(cstat->line_copy);
	free(ptr);
}


void
revertParseState(COMPSTATE* cstat)
{
	struct parse_state* ptr;

	cstat->lineno = ptr->lineno;
	cstat->nowords = ptr->nowords;
	cstat->curr_line = ptr->curr_line;
	cstat->next_char = ptr->next_char;

	if (ptr->line_copy) {
		if (cstat->line_copy)
			free(cstat->line_copy);
		cstat->line_copy = ptr->line_copy;
		ptr->line_copy = NULL;
	}

	popParseState(cstat);
}



char
nextMuvChar(COMPSTATE* cstat)
{
	if (!cstat->curr_line)
		return 0;

	if (!cstat->next_char)
		return 0;

	if (!*cstat->next_char || !*(++(cstat->next_char))) {
		struct parse_state* ptr = cstat->parsestate;

		while (ptr && ptr->curr_line == cstat->curr_line)
		{
			ptr->line_copy = alloc_string(cstat->line_copy);
		}

		advance_line(cstat);
		return nextMuvChar(cstat);
	}
	return *cstat->next_char;
}


char
advanceMuvChar(COMPSTATE* cstat)
{
	char ch;

	if (!cstat->next_char)
		return 0;

	ch = *cstat->next_char;
	nextMuvChar(cstat);
	return ch;
}



/*****************************************************************/


struct expression_data {
	int type;
	struct INTERMEDIATE* expr;
};


void
free_expression(struct expression_data* ptr)
{
	if (ptr && ptr->expr)
		free_intermediate_chain(ptr->expr);
}


/*****************************************************************/


void
append_intermediate(COMPSTATE* cstat, struct INTERMEDIATE* in)
{
	if (!cstat->first_word)
		cstat->first_word = in;

	cstat->curr_word->next = in;
	while (cstat->curr_word && cstat->curr_word->next)
		cstat->curr_word = cstat->curr_word->next;
}


struct INTERMEDIATE*
chain_append(struct INTERMEDIATE* old, struct INTERMEDIATE* in)
{
	if (!old)
		return in;
	append_intermediate_chain(old, in);
	return old;
}


struct INTERMEDIATE*
append_intermediate_copy(COMPSTATE* cstat, struct INTERMEDIATE* old, struct INTERMEDIATE* prim)
{
	struct INTERMEDIATE* out;

	out = new_inst(cstat);
	out->no = cstat->nowords++;
	copyinst(&prim->in, &out->in);
	out->in.line = cstat->lineno;
	return chain_append(old, out);
}


struct INTERMEDIATE*
append_inttype(COMPSTATE* cstat, struct INTERMEDIATE* old, int type, int in)
{
	struct INTERMEDIATE* out;

	out = new_inst(cstat);
	out->no = cstat->nowords++;
	out->in.type = type;
	out->in.line = cstat->lineno;
	out->in.data.number = in;

	return chain_append(old, out);
}


struct INTERMEDIATE*
append_primitive(COMPSTATE* cstat, struct INTERMEDIATE* old, const char* prim)
{
	return append_inttype(old, PROG_PRIMITIVE, in);
}


struct INTERMEDIATE*
append_integer(COMPSTATE* cstat, struct INTERMEDIATE* old, int in)
{
	return append_inttype(old, PROG_INTEGER, in);
}



/*****************************************************************/



inline int
getArrayDepth(int type)
{
	return MUV_ARRAYGETLEVEL(type);
}


inline int
decrementArrayDepth(int type)
{
	int depth = MUV_ARRAY_LEVEL(type);
	return MUV_ARRAYSETLEVEL(type, depth - 1);
}


int
getValueType(int type)
{
	return MUV_TYPEOF(type);
}


int
typeMatches(int type1, int type2)
{
	if (type1 == PROG_UNTYPED)
		return 1;
	if (type2 == PROG_UNTYPED)
		return 1;
	return (type1 == type2);
}


int
getTypeOfVariable(COMPSTATE* cstat, struct INTERMEDIATE* in)
{
	if (in.type == PROG_SVAR) {
		return cstat->scopedvartypes[in->in.data.number];
	} else if (in.type == PROG_LVAR) {
		return cstat->localvartypes[in->in.data.number];
	} else if (in.type == PROG_VAR) {
		return cstat->varabletypes[in->in.data.number];
	}
	return 0;
}



/*****************************************************************/


int
eatOptionalWhiteSpace(COMPSTATE* cstat)
{
	while (cstat->next_char &&
		(isspace(*cstat->next_char) || *cstat->next_char == '/'))
	{
		while (cstat->next_char && isspace(*cstat->next_char)) {
			if (advanceMuvChar(cstat) == '\n') {
				cstat->line++;
			}
		}


		/* Treat C and C++ style comments as whitespace. */
		if (cstat->next_char && *cstat->next_char == '/')
		{
			if (cstat->next_char[1] == '*')
			{
				nextMuvChar(cstat);
				nextMuvChar(cstat);
				while (cstat->next_char &&
					(cstat->next_char[0] != '*' || cstat->next_char[1] != '/'))
				{
					if (advanceMuvChar(cstat) == '\n') {
						cstat->line++;
					}
				}
				if (cstat->next_char)
				{
					nextMuvChar(cstat);
					nextMuvChar(cstat);
				}
			}
			else if (cstat->next_char[1] == '/')
			{
				nextMuvChar(cstat);
				nextMuvChar(cstat);
				while (*cstat->next_char && *cstat->next_char != '\n')
					nextMuvChar(cstat);
			}
			else
			{
				break;
			}
		}
	}
	return 1;
}


int
isWhiteSpace(COMPSTATE* cstat)
{
	if (!cstat->next_char)
		return 0;

    if (!isspace(*cstat->next_char))
		return 0;

	return eatOptionalWhiteSpace(cstat);
}


int
isNextChar(COMPSTATE* cstat, char ch)
{
	eatOptionalWhiteSpace(cstat);
	if (*cstat->next_char != ch) {
		return 0;
	}
	advanceMufChar(cstat);
	return 1;
}


int
isNextToken(COMPSTATE* cstat, char* str)
{
	char *tmp;

	eatOptionalWhiteSpace(cstat);
	tmp = cstat->next_char;
	while (*str && *str == *tmp) str++, tmp++;
	if (!*str) {
		return 0;
	}
	cstat->next_char = tmp;
	return 1;
}


int
getIdentifier(COMPSTATE* cstat, char*outbuf, int outbuflen)
{
	int len = 0;

	if (!cstat->next_char)
		return 0;

	pushParseState(cstat);
	eatOptionalWhiteSpace(cstat);

	if (!isalpha(*cstat->next_char) && *cstat->next_char != '_')
	{
		revertParseState(cstat);
		return 0;
	}

	while (cstat->next_char &&
		(isalnum(*cstat->next_char) || *cstat->next_char == '_'))
	{
		if (len < outbuflen - 1)
		{
			*outbuf++ = advanceMuvChar(cstat);
			len++;
		}
	}

	if (outbuflen > 0)
		*outbuf = '\0';

	popParseState(cstat);
	return len;
}


struct INTERMEDIATE*
getInteger(COMPSTATE* cstat)
{
	int negaflag = 0;
	int val = 0;
	int base = 10;
	struct INTERMEDIATE* out = NULL;

	pushParseState(cstat);

	eatOptionalWhiteSpace(cstat);

	/* WORK: Unary + and - may mean we don't need to handle + and - here. */
	/*       However, we may need to optimize out some '-1 *' dealies. */
	if (*cstat->next_char != '+' && *cstat->next_char != '-' && !isdigit(*cstat->next_char))
	{
		revertParseState(cstat);
		return NULL;
	}

	if (isNextChar(cstat, '+')) {
		negaflag = 0;
	} else if (isNextChar(cstat, '-')) {
		negaflag = 1;
	}

	if (*cstat->next_char == '0') {
		base = 8;
		nextMuvChar(cstat);
		if (*cstat->next_char == 'x' || *cstat->next_char == 'X') {
			base = 16;
			nextMuvChar(cstat);
		}
	}

	if (base == 8) {
		while (*cstat->next_char >= '0' && *cstat->next_char <= '7')
		{
			val *= base;
			val += advanceMuvChar(cstat) - '0';
		}
	} else if (base == 10) {
		if (!isdigit(*cstat->next_char))
		{
			revertParseState(cstat);
			return NULL;
		}
		while (isdigit(*cstat->next_char))
		{
			val *= base;
			val += advanceMuvChar(cstat) - '0';
		}
	} else if (base == 16) {
		if (!isxdigit(*cstat->next_char))
		{
			revertParseState(cstat);
			return NULL;
		}
		while (isxdigit(*cstat->next_char))
		{
			val *= base;
			if (isdigit(*cstat->next_char)) {
				val += advanceMuvChar(cstat) - '0';
			} else if (isupper(*cstat->next_char)) {
				val += 10 + (advanceMuvChar(cstat) - 'A');
			} else {
				val += 10 + (advanceMuvChar(cstat) - 'a');
			}
		}
	}

	if (negaflag)
		val *= -1;

	popParseState(cstat);

	out = new_inst(cstat);
	out->no = cstat->nowords++;
	out->in.type = PROG_INTEGER;
	out->in.line = cstat->lineno;
	out->in.data.number = val;
	return out;
}


struct INTERMEDIATE*
getFloat(COMPSTATE* cstat)
{
	int negaflag = 0;
	int negamantflag = 0;
	int decflag = 0;
	int mantflag = 0;
	double decmag = 1;
	double val = 0;
	double decval = 0;
	int mantval = 0;
	struct INTERMEDIATE* out = NULL;

	pushParseState(cstat);

	eatOptionalWhiteSpace(cstat);

	/* WORK: Unary + and - may mean we don't need to handle + and - here. */
	/*       However, we may need to optimize out some '-1 *' dealies. */
	if (*cstat->next_char != '+' && *cstat->next_char != '-' &&
		*cstat->next_char != '.' && !isdigit(*cstat->next_char))
	{
		revertParseState(cstat);
		return NULL;
	}

	if (isNextChar(cstat, '+')) {
		negaflag = 0;
	} else if (isNextChar(cstat, '-')) {
		negaflag = 1;
	} else if (isNextChar(cstat, '.')) {
		decflag = 1;
	}

	if (!isdigit(*cstat->next_char))
	{
		revertParseState(cstat);
		return NULL;
	}

	while (isdigit(*cstat->next_char) || *cstat->next_char == '.')
	{
		if (isdigit(*cstat->next_char)) {
			if (decflag) {
				decval *= 10.0;
				decval += advanceMuvChar(cstat) - '0';
				decmag *= 10.0;
			} else {
				val *= 10.0;
				val += advanceMuvChar(cstat) - '0';
			}
		} else if (isNextChar(cstat, '.')) {
			decflag = 1;
		}
	}
	if (negaflag)
		val *= -1;
	val = val + (decval / decmag);

	if (isNextChar(cstat, 'e') || isNextChar(cstat, 'E')) {
		pushParseState(cstat);
		if (isNextChar(cstat, '+')) {
			negamantflag = 0;
		} else if (isNextChar(cstat, '-')) {
			negamantflag = 1;
		}

		mantflag = 1;
		if (!isdigit(*cstat->next_char)) {
			if (!decflag) {
				popParseState(cstat);
				revertParseState(cstat);
				return NULL;
			}
			revertParseState(cstat);
		} else {
			while (isdigit(*cstat->next_char)) {
				mantval *= 10;
				mantval += advanceMuvChar(cstat) - '0';
			}
			popParseState(cstat);
		}
	}
	if (negamantflag)
		mantval *= -1;

	if (mantval != 0)
		val *= pow(10.0, mantval);

	popParseState(cstat);

	out = new_inst(cstat);
	out->no = cstat->nowords++;
	out->in.type = PROG_FLOAT;
	out->in.line = cstat->lineno;
	out->in.data.fnumber = val;
	return out;
}


struct INTERMEDIATE*
getString(COMPSTATE* cstat)
{
	struct INTERMEDIATE* out = NULL;
	char *val = (char*)malloc(128);
	char *out = val;
	int maxlen = 128;
	int currlen = 0;

	pushParseState(cstat);
	eatOptionalWhiteSpace(cstat);
	if (!isNextChar(cstat, '"')) {
		revertParseState(cstat);
		return NULL;
	}

	while (*cstat->next_char && *cstat->next_char != '"') {
		if (++currlen >= maxlen) {
			maxlen *= 2;
			val = (char*)realloc(val, maxlen);
		}
		if (isNextChar(cstat, '\\')) {
			switch (*cstat->next_char) {
				case '\0':
					currlen--;
					break;
				case 'n':
				case 'r':
				    *out++ = '\r';
					nextMuvChar(cstat);
					break;
				case '[':
				    *out++ = (char)27;
					nextMuvChar(cstat);
					break;
				default:
				    *out++ = advanceMuvChar(cstat);
					break;
			}
		} else {
			*out++ = advanceMuvChar(cstat);
		}
	}
	if (!isNextChar(cstat, '"')) {
		revertParseState(cstat);
		free(val);
		return NULL;
	}

	popParseState(cstat);

	out = new_inst(cstat);
	out->no = cstat->nowords++;
	out->in.type = PROG_STRING;
	out->in.line = cstat->lineno;
	out->in.data.string = alloc_prog_string(val);
	free(val);
	return out;
}


struct INTERMEDIATE*
getDbref(COMPSTATE* cstat)
{
	struct INTERMEDIATE* out = NULL;

	pushParseState(cstat);
	eatOptionalWhiteSpace(cstat);

	/* if integers stop managing their own + and -, we may need to do that here */
	if (!isNextChar(cstat, '#')) {
		revertParseState(cstat);
		return NULL;
	}

	if (!(out = getInteger(cstat))) {
		revertParseState(cstat);
		return NULL;
	}

	out->in.type = PROG_OBJECT;
	popParseState(cstat);
	return out;
}


struct INTERMEDIATE*
getProcIdent(COMPSTATE* cstat)
{
	struct INTERMEDIATE* out = NULL;
	char ident[MAX_IDENT_LEN];
	struct PROC_LIST *p;

	pushParseState(cstat);

	if (!getIdentifier(cstat, ident, sizeof(ident))) {
		revertParseState(cstat);
		return NULL;
	}

	for (p = cstat->procs; p; p = p->next)
		if (!string_compare(p->name, ident))
			break;
	
	if (!p) {
		revertParseState(cstat);
		return NULL;
	}

	popParseState(cstat);

	out = new_inst(cstat);
	out->no = cstat->nowords++;
	out->in.type = PROG_ADDR;
	out->in.line = cstat->lineno;
	out->in.data.number = p->code->no;

	return out;
}


struct INTERMEDIATE*
getGlobalVar(COMPSTATE* cstat)
{
	struct INTERMEDIATE* out = NULL;
	char ident[MAX_IDENT_LEN];
	int varnum = -1;
	int i;

	pushParseState(cstat);
	if (!getIdentifier(cstat, ident, sizeof(ident))) {
		revertParseState(cstat);
		return NULL;
	}

	for (i = 0; i < MAX_VAR && cstat->variables[i]; i++) {
		if (!string_compare(ident, cstat->variables[i])) {
			varnum = i;
			break;
		}
	}

	if (varnum == -1) {
		revertParseState(cstat);
		return NULL;
	}

	popParseState(cstat);

	out = new_inst(cstat);
	out->no = cstat->nowords++;
	out->in.type = PROG_VAR;
	out->in.line = cstat->lineno;
	out->in.data.number = varnum;
	return out;
}


struct INTERMEDIATE*
getLocalVar(COMPSTATE* cstat)
{
	struct INTERMEDIATE* out = NULL;
	char ident[MAX_IDENT_LEN];
	int varnum = -1;
	int i;

	pushParseState(cstat);
	if (!getIdentifier(cstat, ident, sizeof(ident))) {
		revertParseState(cstat);
		return NULL;
	}

	for (i = 0; i < MAX_VAR && cstat->localvars[i]; i++) {
		if (!string_compare(ident, cstat->localvars[i])) {
			varnum = i;
			break;
		}
	}

	if (varnum == -1) {
		revertParseState(cstat);
		return NULL;
	}

	popParseState(cstat);

	out = new_inst(cstat);
	out->no = cstat->nowords++;
	out->in.type = PROG_LVAR;
	out->in.line = cstat->lineno;
	out->in.data.number = varnum;
	return out;
}


struct INTERMEDIATE*
getScopedVar(COMPSTATE* cstat)
{
	struct INTERMEDIATE* out = NULL;
	char ident[MAX_IDENT_LEN];
	int varnum = -1;
	int i;

	pushParseState(cstat);
	if (!getIdentifier(cstat, ident, sizeof(ident))) {
		revertParseState(cstat);
		return NULL;
	}

	for (i = 0; i < MAX_VAR && cstat->scopedvars[i]; i++) {
		if (!string_compare(ident, cstat->scopedvars[i])) {
			varnum = i;
			break;
		}
	}

	if (varnum == -1) {
		revertParseState(cstat);
		return NULL;
	}

	popParseState(cstat);

	out = new_inst(cstat);
	out->no = cstat->nowords++;
	out->in.type = PROG_SVAR;
	out->in.line = cstat->lineno;
	out->in.data.number = varnum;
	return out;
}



struct INTERMEDIATE*
getVariable(COMPSTATE* cstat)
{
	struct INTERMEDIATE* out;
	if ((out = getScopedVar(cstat)))
		return out;
	if ((out = getLocalVar(cstat)))
		return out;
	if ((out = getGlobalVar(cstat)))
		return out;
	return NULL;
}


int
getVarType(COMPSTATE* cstat)
{
	char ident[MAX_IDENT_LEN];
	int outval = 0;

	pushParseState(cstat);
	if (!getIdentifier(cstat, ident, sizeof(ident))) {
		revertParseState(cstat);
		return 0;
	}

	if (!strcmp(cstat->next_char, "untyped")) {
		outval = PROG_UNTYPED;
	} else if (!strcmp(cstat->next_char, "int")) {
		outval = PROG_INTEGER;
	} else if (!strcmp(cstat->next_char, "lock")) {
		outval = PROG_LOCK;
	} else if (!strcmp(cstat->next_char, "dbref")) {
		outval = PROG_OBJECT;
	} else if (!strcmp(cstat->next_char, "float")) {
		outval = PROG_FLOAT;
	} else if (!strcmp(cstat->next_char, "string")) {
		outval = PROG_STRING;
	} else if (!strcmp(cstat->next_char, "address")) {
		outval = PROG_ADD;
	} else {
		/* WORK: check for typedefed types here. */
		/* WORK: check for class types here */
		/* WORK: check for enum types here */
		revertParseState(cstat);
		return 0;
	}
	popParseState(cstat);
	return outval;
}



struct expression_data
getDatum(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct INTERMEDIATE* out = NULL;

	pushParseState(cstat);

	out = getInteger(cstat);
	if (!out)
		out = getFloat(cstat);

	if (!out)
		out = getDbref(cstat);

	if (!out)
		out = getString(cstat);

	if (!out) {
		struct INTERMEDIATE* var = NULL;
		var = getVariable(cstat);
		if (var) {
			int vartype = var->in.type;
			int varnum = var->in.data.number;
			if (vartype = PROG_SVAR) {
				out = append_inttype(cstat, NULL, PROG_SVAR_AT, varnum);
			} else {
				out = append_inttype(cstat, NULL, vartype, varnum);
				out = append_primitive(cstat, out, "@");
			}
			free(var);
		}
	}
	
	if (!out)
		out = getProcIdent(cstat);

	if (!out) {
		revertParseState(cstat);
		return oed;
	}

	popParseState(cstat);

	oed.type = out->type;
	oed.expr = out;
	return oed;
}


int
getScopedVarDeclAssign(COMPSTATE* cstat)
{
	char ident[MAX_IDENT_LEN];
	struct INTERMEDIATE* out = NULL;
	struct expression_data ed = {0, NULL};
	int varno = 0;
	int vartype = 0;

	pushParseState(cstat);

	if ((vartype = getVarType(cstat)))
	{
		if (!getIdentifier(cstat, ident, sizeof(ident)))
		{
			do_abort_compile(cstat, "Expected variable name.");
			revertParseState(cstat);
			return 0;
		}

		if (isNextChar(cstat, '='))
		{
			ed = getExpression(cstat);
			if (!ed.type)
			{
				do_abort_compile(cstat, "Bad variable assignment expression.");
				revertParseState(cstat);
				return 0;
			}
			if (typeMatches(vartype, ed.type))
			{
				do_abort_compile(cstat, "Assigned value does not match declared variable type.");
				revertParseState(cstat);
				return 0;
			}

			if (!isNextChar(cstat, ';'))
			{
				do_abort_compile(cstat, "Expected a ';' at the end of this statement.");
				revertParseState(cstat);
				return 0;
			}

			varno = add_scopedvar(cstat, ident, vartype);
			cstat->curr_proc->in.data.mufproc->vars++;

			/* initilization here */
			out = new_inst(cstat);
			out->no = cstat->nowords++;
			out->in.type = PROG_SVAR_BANG;
			out->in.line = cstat->lineno;
			out->in.data.number = varno;
			append_intermediate(cstat, ed.expr);
			append_intermediate(cstat, out);
		}
		else if (isNextChar(cstat, ';'))
		{
			varno = add_scopedvar(cstat, ident, vartype);
			cstat->curr_proc->in.data.mufproc->vars++;
		}
		else
		{
			do_abort_compile(cstat, "Expected a ';' at the end of this statement.");
			revertParseState(cstat);
			return 0;
		}

		popParseState(cstat);
		return 1;
	}

	revertParseState(cstat);
	return 0;
}



/***/

struct expression_data
getParentheticalExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */

	pushParseState(cstat);
	if (!isNextChar(cstat, '(')) {
		revertParseState(cstat);
		return oed;
	}

	oed = getExpression(cstat);
	if (!oed.type) {
		do_abort_compile(cstat, "Bad expression in ( ).");
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, ')')) {
		do_abort_compile(cstat, "Expected terminating ) in expression.");
		free_expression(oed.expr);
		revertParseState(cstat);
		return oed;
	}

	popParseState(cstat);

	return oed;
}


struct expression_data
getArrayDeclExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data ked = {0, NULL}; /* array Key ED */
	struct expression_data ved = {0, NULL}; /* array Value ED */
	struct INTERMEDIATE* out = NULL;
	int valtype = 0;
	int firstpass = 1;
	int dictflag = 0;
	int valcount = 0;
	char buf[MAX_IDENT_LEN + 128];

	pushParseState(cstat);
	if (!isNextChar(cstat, '{')) {
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, '}')) {
		do {
			valcount++;
			ved = getExpression(cstat);
			if (!ved.type) {
				sprintf(buf, "Bad expression for value %d of array declaration.", valcount);
				do_abort_compile(cstat, buf);
				free_expression(out);
				revertParseState(cstat);
				return oed;
			}
			if (ved.type != PROG_UNTYPED && valtype != PROG_UNTYPED) {
				if (valtype && valtype != ved.type) {
					valtype = PROG_UNTYPED;
				} else {
					valtype = ved.type;
				}
			} else {
				valtype = PROG_UNTYPED;
			}

			if (isNextChar(cstat, '=') && isNextChar(cstat, '>')) {
				if (firstpass) {
					dictflag = 1;
				} else if (!dictflag) {
					do_abort_compile(cstat, "Inconsistent array/dictionary declaration.  First value may be missing a =>.");
					free_expression(out);
					revertParseState(cstat);
					return oed;
				}
				ked = getExpression(cstat);
				if (!ked.type) {
					do_abort_compile(cstat, "Bad expression in dictionary declaration.");
					free_expression(out);
					revertParseState(cstat);
					return oed;
				}
				/* WORK: Need to check key expression type.  */

				out = chain_append(out, ked.expr);
			} else if (dictflag) {
				do_abort_compile(cstat, "Missing => in dictionary declaration.");
				free_expression(out);
				revertParseState(cstat);
				return oed;
			}

			firstpass = 0;
			out = chain_append(out, ved.expr);
		} while (isNextChar(cstat, ','));

		if (!isNextChar(cstat, '}')) {
			do_abort_compile(cstat, "Expected terminating } in Array declaration.");
			free_expression(out);
			revertParseState(cstat);
			return oed;
		}
	}

	if (!valtype)
		valtype = PROG_UNTYPED;

	out = append_integer(cstat, out, valcount);
	if (dictflag)
		out = append_primitive(cstat, out, "ARRAY_MAKE_DICT");
	else
		out = append_primitive(cstat, out, "ARRAY_MAKE");

	popParseState(cstat);

	oed.expr = valtype;
	oed.expr = out;
	return oed;
}



struct expression_data
getPrimaryExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */

	oed = getParentheticalExpr(cstat);

	if (!oed.type)
		oed = getArrayDeclExpr(cstat);

	if (!oed.type)
		oed = getDatum(cstat);

	return oed;
}



struct expression_data
getArrayDeRef(COMPSTATE* cstat)
{
	struct INTERMEDIATE* var;
	struct INTERMEDIATE* out = NULL;
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data aed = {0, NULL}; /* Array subscript ED */
	int vartype, varnum;

	pushParseState(cstat);
	var = getVariable(cstat);
	if (!var) {
		popParseState(cstat);
		return oed;
	}
	vartype = var->in.type;
	varnum = var->in.data.number;

	if (!isNextChar(cstat, '[')) {
		free(var);
		revertParseState(cstat);
		return oed;
	}

	lvaltype = getTypeOfVariable(cstat, var);
	free(var);

	if (lvaltype != PROG_UNTYPED)
	{
		if (getArrayDepth(lvaltype) < 1)
		{
			do_abort_compile(cstat, "Attempting to perform an array operation on a non-array variable.");
			revertParseState(cstat);
			return oed;
		}
		lvaltype = decrementArrayDepth(lvaltype);
	}

	aed = getExpression(cstat);
	if (!aed.type) {
		do_abort_compile(cstat, "Bad array subscript expression.");
		revertParseState(cstat);
		return oed;
	}
	/* WORK: Need to check subscript expression type.  */

	if (vartype == PROG_SVAR) {
		out = append_inttype(cstat, NULL, PROG_SVAR_AT, varnum);
	} else {
		out = append_inttype(cstat, NULL, vartype, varnum);
		out = append_primitive(cstat, out, "@");
	}

	out = chain_append(out, aed.expr);
	out = append_primitive(cstat, out, "ARRAY_GETITEM");

	popParseState(cstat);

	oed.expr = lvaltype;
	oed.expr = out;
	return oed;
}



struct arg_type_stack {
	int* stack;
	int count;
	int maxcount;
};


struct arg_type_stack*
getPrimArgStackResult(struct arg_type_stack* argstack, int primnum)
{
	struct arg_type_stack *inargs, *outargs;
	char buf[3*MAX_IDENT_LEN + 128]
	char buf2[MAX_IDENT_LEN + 128]
	char buf3[MAX_IDENT_LEN + 128]
	int i, j;

	if (!argstack) {
		argstack = (struct arg_type_stack*)malloc(sizeof(arg_type_stack));
		argstack->count = 0;
		argstack->maxcount = 10;
		argstack->stack = (int*)malloc(argstack->maxcount * sizeof(int))
	}
	inargs = primitive_args_in[i];
	outargs = primitive_args_out[i];

	if (!isArgStackOkay(argstack, inargs, base_inst[primnum - BASE_MIN])) {
		return NULL;
	}
	return outargs;
}


int
isArgStackOkay(struct arg_type_stack* argstack, struct arg_type_stack* inargs, const char* ident)
{
	struct arg_type_stack *inargs, *outargs;
	char buf[3*MAX_IDENT_LEN + 128]
	char buf2[MAX_IDENT_LEN + 128]
	char buf3[MAX_IDENT_LEN + 128]
	int i, j;

	if (!argstack) {
		do_abort_compile(cstat, "SERVER ERROR: null argstack in isArgStackOkay()");
		return 0;
	}

	if (inargs->count > argstack->count) {
		sprintf(buf, "Too few arguments for %s!  Expected %d, got %d.",
			ident, inargs->count, argstack->count);
		do_abort_compile(cstat, buf);
		return 0;
	}
	if (inargs->count < argstack->count) {
		sprintf(buf, "Too many arguments for %s!  Expected %d, got %d.",
			ident, inargs->count, argstack->count);
		do_abort_compile(cstat, buf);
		return 0;
	}
	for (i = 0; i < argstack->count; i++) {
		if (argstack->stack[i] != PROG_UNTYPED &&
			inargs->stack[i] != PROG_UNTYPED &&
			argstack->stack[i] != inargs->stack[i])
		{
			sprintf(buf, "Bad type for argument %d of %s!  Expected %s, got %s.",
				j+1, ident,
				typeName(buf2, inargs->stack[i]),
				typeName(buf3, argstack->stack[i]));
			do_abort_compile(cstat, buf);
			return 0;
		}
	}
}


struct expression_data
getProcedureCall(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data aed = {0, NULL}; /* ED for expr After */
	struct INTERMEDIATE* out = NULL;
	struct PROC_LIST *p;
	char ident[MAX_IDENT_LEN];
	int argnum = 0;
	char buf[MAX_IDENT_LEN + 128];
	int primnum = 0;
	int expected_arg_count = 0;

	pushParseState(cstat);
	if (!getIdentifier(cstat, ident, sizeof(ident)))
		popParseState(cstat);
		return oed;
	}

	for (p = cstat->procs; p; p = p->next) {
		if (!string_compare(p->name, ident)) {
			break;
		}
	}
	
	if (!p) {
		/* Check for inserver primitives */
		primnum = get_primitive(ident);
		if (primnum) {
			/* WORK: Need to check inserver primitive args */
		}
	}

	/* WORK: Need to check $define macros.  This may require parsing    */
	/*       underlying prims to determine arg types. */

	/* WORK: support multiple return vals and assignment via comma list */
	/*       ie:  foo, bar = bazqux(fleeble);                           */
	/*       Will need expr_data struct to understand mult return types */

	if (!p && !primnum) {
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, '(')) {
		if (p) {
			oed.type = PROG_ADD;
			oed.expr = append_inttype(cstat, NULL, PROG_ADD, get_address(cstat, p->code, 0));
		}
		popParseState(cstat);
		return oed;
	}


	if (!isNextChar(cstat, ')')) {
		do {
			argnum++;
			ed = getExpression(cstat);
			if (!ed.type) {
				sprintf(buf, "Bad expression for argument %d of call to %s.", argnum, ident);
				do_abort_compile(cstat, buf);
				free_expression(out);
				revertParseState(cstat);
				return oed;
			}
			/* WORK: Need to check arg types.  Needs proc decls to remember arg types */
			out = chain_append(out, ed.expr);
		} while (isNextChar(cstat, ','));

		if (!isNextChar(cstat, ')')) {
			sprintf(buf, "Expected terminating ) in call to %s.", ident);
			do_abort_compile(cstat, buf);
			free_expression(out);
			revertParseState(cstat);
			return oed;
		}
	}

	if (p) {
		out = append_inttype(cstat, out, PROG_EXEC, get_address(cstat, p->code, 0));
		oed.type = p->returntype;
	} else if (primnum) {
		out = append_inttype(cstat, out, PROG_PRIMITIVE, primnum);
	}

	popParseState(cstat);

	oed.expr = out;
	return oed;
}



struct expression_data
getPostIncrDecrExpr(COMPSTATE* cstat)
{
	struct INTERMEDIATE* var;
	struct expression_data oed = {0, NULL}; /* Output ED */
	char* opertype;
	int vartype, varnum;

	pushParseState(cstat);
	var = getVariable(cstat);
	if (!var) {
		popParseState(cstat);
		return oed;
	}
	vartype = var->in.type;
	varnum = var->in.data.number;

	if (isNextToken(cstat, "++")) {
		opertype = "++";
	} else if (!isNextToken(cstat, "--")) {
		opertype = "--"
	} else {
		free(var);
		revertParseState(cstat);
		return oed;
	}

	lvaltype = getTypeOfVariable(cstat, var);
	free(var);

	if (lvaltype != PROG_UNTYPED &&
		lvaltype != PROG_INTEGER &&
		lvaltype != PROG_FLOAT &&
		lvaltype != PROG_OBJECT)
	{
		char buf[128];
		sprintf(buf, "Bad variable type before %s.  Must be of type int, float, or dbref.", opertype);
		do_abort_compile(cstat, buf);
		revertParseState(cstat);
		return oed;
	}

	if (vartype == PROG_SVAR) {
		out = append_inttype(cstat, NULL, PROG_SVAR_AT, varnum);
	} else {
		out = append_inttype(cstat, NULL, vartype, varnum);
		out = append_primitive(cstat, out, "@");
	}

	out = append_primitive(cstat, out, "DUP");
	out = append_inttype(cstat, out, PROG_INTEGER, 1);
	if (*opertype == '+') {
		out = append_primitive(cstat, out, "+");
	} else {
		out = append_primitive(cstat, out, "-");
	}

	if (vartype == PROG_SVAR) {
		out = append_inttype(cstat, out, PROG_SVAR_BANG, varnum);
	} else {
		out = append_inttype(cstat, out, vartype, varnum);
		out = append_primitive(cstat, out, "!");
	}

	popParseState(cstat);

	oed.expr = lvaltype;
	oed.expr = out;
	return oed;
}


struct expression_data
getPostFixExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */

	oed = getPrimaryExpr(cstat);

	if (!oed.type)
		oed = getArrayDeRef(cstat);

	if (!oed.type)
		oed = getProcedureCall(cstat);

	if (!oed.type)
		oed = getPostIncrDecrExpr(cstat);

	/*
	 * FUTURE: This would be where to put in methods/members type stuff.
	 *   Either need to add a new_class/del_class/put_class/get_class set of
	 *   instrs, or need to create instance references, with put/get instrs.
	 */

	return oed;
}


struct expression_data
getPreIncrDecrExpr(COMPSTATE* cstat)
{
	struct INTERMEDIATE* var;
	struct expression_data oed = {0, NULL}; /* Output ED */
	char* opertype;
	int vartype, varnum;

	pushParseState(cstat);
	if (isNextToken(cstat, "++")) {
		opertype = "++";
	} else if (!isNextToken(cstat, "--")) {
		opertype = "--"
	} else {
		popParseState(cstat);
		return oed;
	}

	var = getVariable(cstat);
	if (!var) {
		char buf[128];
		sprintf(buf, "Bad variable expression after %s.", opertype);
		do_abort_compile(cstat, buf);
		revertParseState(cstat);
		return oed;
	}
	vartype = var->in.type;
	varnum = var->in.data.number;

	lvaltype = getTypeOfVariable(cstat, var);
	free(var);
	if (lvaltype != PROG_UNTYPED &&
		lvaltype != PROG_INTEGER &&
		lvaltype != PROG_FLOAT &&
		lvaltype != PROG_OBJECT)
	{
		char buf[128];
		sprintf(buf, "Bad variable type after %s.  Must be of type int, float, or dbref.", opertype);
		do_abort_compile(cstat, buf);
		revertParseState(cstat);
		return oed;
	}

	if (vartype == PROG_SVAR) {
		out = append_inttype(cstat, NULL, PROG_SVAR_AT, varnum);
	} else {
		out = append_inttype(cstat, NULL, vartype, varnum);
		out = append_primitive(cstat, out, "@");
	}

	out = append_inttype(cstat, out, PROG_INTEGER, 1);
	if (*opertype == '+') {
		out = append_primitive(cstat, out, "+");
	} else {
		out = append_primitive(cstat, out, "-");
	}
	out = append_primitive(cstat, out, "DUP");

	if (vartype == PROG_SVAR) {
		out = append_inttype(cstat, out, PROG_SVAR_BANG, varnum);
	} else {
		out = append_inttype(cstat, out, vartype, varnum);
		out = append_primitive(cstat, out, "!");
	}

	popParseState(cstat);

	oed.expr = lvaltype;
	oed.expr = out;
	return oed;
}


struct expression_data
getUnaryPositiveExpr(COMPSTATE* cstat)
{
	struct expression_data aed = {0, NULL}; /* ED for expr After */

	pushParseState(cstat);
	if (!isNextChar(cstat, '+')) {
		popParseState(cstat);
		return aed;
	}

	/* Don't treat this as a unary positive if it precedes a number */
	eatOptionalWhiteSpace(cstat);
	if (cstat->next_char && isdigit(cstat->next_char)) {
		revertParseState(cstat);
		return oed;
	}

	aed = getUnaryExpr(cstat);
	if (!aed.type) {
		do_abort_compile(cstat, "Bad expression after unary +.");
		revertParseState(cstat);
		return aed;
	}

	popParseState(cstat);

	return aed;
}


struct expression_data
getUnaryNegativeExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data aed = {0, NULL}; /* ED for expr After */

	pushParseState(cstat);
	if (!isNextChar(cstat, '-')) {
		popParseState(cstat);
		return oed;
	}

	/* Don't treat this as a unary negative if it precedes a number */
	eatOptionalWhiteSpace(cstat);
	if (cstat->next_char && isdigit(cstat->next_char)) {
		revertParseState(cstat);
		return oed;
	}

	aed = getUnaryExpr(cstat);
	if (!aed.type) {
		do_abort_compile(cstat, "Bad expression after unary -.");
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, aed.expr);
	out = append_inttype(cstat, out, PROG_INTEGER, -1);
	out = append_primitive(cstat, out, "*");

	popParseState(cstat);

	oed.type = aed.type;
	oed.expr = out;
	return oed;
}


struct expression_data
getBitwiseNotExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data aed = {0, NULL}; /* ED for expr After */

	pushParseState(cstat);
	if (!isNextChar(cstat, '~')) {
		popParseState(cstat);
		return oed;
	}

	aed = getUnaryExpr(cstat);
	if (!aed.type) {
		do_abort_compile(cstat, "Bad expression after ~.");
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, aed.expr);
	out = append_primitive(cstat, out, "BITNOT");

	popParseState(cstat);

	oed.type = PROG_INTEGER;
	oed.expr = out;
	return oed;
}


struct expression_data
getLogicalNotExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data aed = {0, NULL}; /* ED for expr After */

	pushParseState(cstat);
	if (!isNextChar(cstat, '!')) {
		popParseState(cstat);
		return oed;
	}

	aed = getUnaryExpr(cstat);
	if (!aed.type) {
		do_abort_compile(cstat, "Bad expression after !.");
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, aed.expr);
	out = append_primitive(cstat, out, "NOT");

	popParseState(cstat);

	oed.type = PROG_INTEGER;
	oed.expr = out;
	return oed;
}



struct expression_data
getUnaryExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */

	oed = getPostfixExpr(cstat);

	if (!oed.type)
		oed = getPostfixExpr(cstat);

	if (!oed.type)
		oed = getPreIncrDecrExpr(cstat);

	if (!oed.type)
		oed = getUnaryPositiveExpr(cstat);

	if (!oed.type)
		oed = getUnaryNegativeExpr(cstat);

	if (!oed.type)
		oed = getBitwiseNotExpr(cstat);

	if (!oed.type)
		oed = getLogicalNotExpr(cstat);

	return oed;
}


struct expression_data
getMultDivModExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data bed = {0, NULL}; /* ED for expr Before */
	struct expression_data aed = {0, NULL}; /* ED for expr After */
	char* opertype;

	bed = getUnaryExpr(cstat);
	if (!bed.type) {
		return bed;
	}

	pushParseState(cstat);
	if (isNextChar(cstat, '*')) {
		opertype = "*";
	} else if (!isNextChar(cstat, '/')) {
		opertype = "/"
	} else if (!isNextChar(cstat, '%')) {
		opertype = "%"
	} else {
		popParseState(cstat);
		return bed;
	}

	aed = getMultDivModExpr(cstat);
	if (!aed.type) {
		char buf[128];
		sprintf(buf, "Bad expression after %s.", opertype);
		do_abort_compile(cstat, buf);
		free_expression(bed);
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, bed.expr);
	out = chain_append(out, aed.expr);
	out = append_primitive(cstat, out, opertype);

	popParseState(cstat);

	if (bed.type == PROG_UNTYPED || aed.type == PROG_UNTYPED) {
		oed.type = PROG_UNTYPED;
	} else if (bed.type == PROG_OBJECT) {
		oed.type = PROG_OBJECT;
	} else if (bed.type == PROG_FLOAT || aed.type == PROG_FLOAT) {
		oed.type = PROG_FLOAT;
	} else {
		oed.type = PROG_INTEGER;
	}
	oed.expr = out;
	return oed;
}


struct expression_data
getAddSubExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data bed = {0, NULL}; /* ED for expr Before */
	struct expression_data aed = {0, NULL}; /* ED for expr After */
	char* opertype;

	bed = getMultDivModExpr(cstat);
	if (!bed.type) {
		return bed;
	}

	pushParseState(cstat);
	if (isNextChar(cstat, '+')) {
		opertype = "+";
	} else if (!isNextChar(cstat, '-')) {
		opertype = "-"
	} else {
		popParseState(cstat);
		return bed;
	}

	aed = getAddSubExpr(cstat);
	if (!aed.type) {
		char buf[128];
		sprintf(buf, "Bad expression after %s.", opertype);
		do_abort_compile(cstat, buf);
		free_expression(bed);
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, bed.expr);
	out = chain_append(out, aed.expr);
	out = append_primitive(cstat, out, opertype);

	popParseState(cstat);

	if (bed.type == PROG_UNTYPED || aed.type == PROG_UNTYPED) {
		oed.type = PROG_UNTYPED;
	} else if (bed.type == PROG_OBJECT) {
		oed.type = PROG_OBJECT;
	} else if (bed.type == PROG_FLOAT || aed.type == PROG_FLOAT) {
		oed.type = PROG_FLOAT;
	} else {
		oed.type = PROG_INTEGER;
	}
	oed.expr = out;
	return oed;
}


struct expression_data
getBitShiftExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data bed = {0, NULL}; /* ED for expr Before */
	struct expression_data aed = {0, NULL}; /* ED for expr After */
	char* opertype;

	bed = getAddSubExpr(cstat);
	if (!bed.type) {
		return bed;
	}

	pushParseState(cstat);
	if (isNextToken(cstat, "<<")) {
		opertype = "<<";
	} else if (!isNextToken(cstat, ">>")) {
		opertype = ">>"
	} else {
		popParseState(cstat);
		return bed;
	}

	aed = getBitShiftExpr(cstat);
	if (!aed.type) {
		char buf[128];
		sprintf(buf, "Bad expression after %s.", opertype);
		do_abort_compile(cstat, buf);
		free_expression(bed);
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, bed.expr);
	out = chain_append(out, aed.expr);
	if (*opertype == '>') {
		out = append_inttype(cstat, out, PROG_INTEGER, -1);
		out = append_primitive(cstat, out, "*");
	}
	out = append_primitive(cstat, out, "BITSHIFT");

	popParseState(cstat);

	oed.type = PROG_INTEGER;
	oed.expr = out;
	return oed;
}


struct expression_data
getLessGreatExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data bed = {0, NULL}; /* ED for expr Before comparator */
	struct expression_data aed = {0, NULL}; /* ED for expr After comparator */
	char* opertype;

	bed = getBitShiftExpr(cstat);
	if (!bed.type) {
		return bed;
	}

	pushParseState(cstat);
	if (isNextToken(cstat, "<=")) {
		opertype = "<=";
	} else if (!isNextToken(cstat, ">=")) {
		opertype = ">=";
	} else if (!isNextChar(cstat, '>')) {
		opertype = ">";
	} else if (!isNextChar(cstat, '<')) {
		opertype = "<";
	} else {
		popParseState(cstat);
		return bed;
	}

	aed = getLessGreatExpr(cstat);
	if (!aed.type) {
		char buf[128];
		sprintf(buf, "Bad expression after %s.", opertype);
		do_abort_compile(cstat, buf);
		free_expression(bed);
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, bed.expr);
	out = chain_append(out, aed.expr);
	out = append_primitive(cstat, out, opertype);

	popParseState(cstat);

	oed.type = PROG_INTEGER;
	oed.expr = out;
	return oed;
}


struct expression_data
getEqualNotEqualExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data bed = {0, NULL}; /* ED for expr Before == or != */
	struct expression_data aed = {0, NULL}; /* ED for expr After == or != */
	int notflag = 0;

	bed = getLessGreatExpr(cstat);
	if (!bed.type) {
		return bed;
	}

	pushParseState(cstat);
	if (!isNextToken(cstat, "=="))
	{
		if (!isNextToken(cstat, "!="))
		{
			popParseState(cstat);
			return bed;
		}
		notflag = 1;
	}

	aed = getEqualNotEqualExpr(cstat);
	if (!aed.type) {
		if (notflag) {
			do_abort_compile(cstat, "Bad expression after !=.");
		} else {
			do_abort_compile(cstat, "Bad expression after ==.");
		}
		free_expression(bed);
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, bed.expr);
	out = chain_append(out, aed.expr);
	out = append_primitive(cstat, out, "=");
	if (notflag)
		out = append_primitive(cstat, out, "NOT");

	popParseState(cstat);

	oed.type = PROG_INTEGER;
	oed.expr = out;
	return oed;
}


struct expression_data
getBitwiseAndExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data bed = {0, NULL}; /* ED for expr Before & */
	struct expression_data aed = {0, NULL}; /* ED for expr After & */

	bed = getEqualNotEqualExpr(cstat);
	if (!bed.type) {
		return bed;
	}

	pushParseState(cstat);
	if (!isNextChar(cstat, '&'))
	{
		popParseState(cstat);
		return bed;
	}

	if (isNextChar(cstat, '&'))
	{
		revertParseState(cstat);
		return bed;
	}

	aed = getBitwiseAndExpr(cstat);
	if (!aed.type) {
		do_abort_compile(cstat, "Bad expression after &.");
		free_expression(bed);
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, bed.expr);
	out = chain_append(out, aed.expr);
	out = append_primitive(cstat, out, "BITAND");

	popParseState(cstat);

	oed.type = PROG_INTEGER;
	oed.expr = out;
	return oed;
}



struct expression_data
getBitwiseXorExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data bed = {0, NULL}; /* ED for expr Before ^ */
	struct expression_data aed = {0, NULL}; /* ED for expr After ^ */

	bed = getBitwiseAndExpr(cstat);
	if (!bed.type) {
		return bed;
	}

	pushParseState(cstat);
	if (!isNextChar(cstat, '^'))
	{
		popParseState(cstat);
		return bed;
	}

	aed = getBitwiseXorExpr(cstat);
	if (!aed.type) {
		do_abort_compile(cstat, "Bad expression after ^.");
		free_expression(bed);
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, bed.expr);
	out = chain_append(out, aed.expr);
	out = append_primitive(cstat, out, "BITXOR");

	popParseState(cstat);

	oed.type = PROG_INTEGER;
	oed.expr = out;
	return oed;
}


struct expression_data
getBitwiseOrExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data bed = {0, NULL}; /* ED for expr Before | */
	struct expression_data aed = {0, NULL}; /* ED for expr After | */

	bed = getBitwiseXorExpr(cstat);
	if (!bed.type) {
		return bed;
	}

	pushParseState(cstat);
	if (!isNextChar(cstat, '|'))
	{
		popParseState(cstat);
		return bed;
	}

	if (isNextChar(cstat, '|'))
	{
		revertParseState(cstat);
		return bed;
	}

	aed = getBitwiseOrExpr(cstat);
	if (!aed.type) {
		do_abort_compile(cstat, "Bad expression after |.");
		free_expression(bed);
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, bed.expr);
	out = chain_append(out, aed.expr);
	out = append_primitive(cstat, out, "BITOR");

	popParseState(cstat);

	oed.type = PROG_INTEGER;
	oed.expr = out;
	return oed;
}



struct expression_data
getLogicalAndExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data bed = {0, NULL}; /* ED for expr Before && */
	struct expression_data aed = {0, NULL}; /* ED for expr After && */
	struct INTERMEDIATE* eef;
	struct INTERMEDIATE* beef;
	struct INTERMEDIATE* out;
	struct INTERMEDIATE* end;

	bed = getBitwiseOrExpr(cstat);
	if (!bed.type) {
		return bed;
	}

	pushParseState(cstat);
	if (!isNextToken(cstat, "&&"))
	{
		popParseState(cstat);
		return bed;
	}

	aed = getLogicalAndExpr(cstat);
	if (!aed.type) {
		do_abort_compile(cstat, "Bad expression after &&.");
		free_expression(bed);
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, bed.expr);
	out = append_primitive(cstat, out, "NOT");

	eef = append_inttype(cstat, NULL, PROG_IF, 0);
	out = chain_append(out, eef);
	out = append_inttype(cstat, out, PROG_INTEGER, 0);

	beef = append_inttype(cstat, NULL, PROG_JMP, 0);
	out = chain_append(out, beef);
	out = chain_append(out, aed.expr);

	for (end = aed.expr; end && end->next; end = end->next) ;

	eef->in.data.number = get_address(cstat, aed.expr, 0);
	beef->in.data.number = get_address(cstat, end, 1);
	
	popParseState(cstat);

	oed.type = aed.type;
	oed.expr = out;
	return oed;
}


struct expression_data
getLogicalOrExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct expression_data bed = {0, NULL}; /* ED for expr Before || */
	struct expression_data aed = {0, NULL}; /* ED for expr After || */
	struct INTERMEDIATE* eef;
	struct INTERMEDIATE* beef;
	struct INTERMEDIATE* out;
	struct INTERMEDIATE* end;

	bed = getLogicalAndExpr(cstat);
	if (!bed.type) {
		return bed;
	}

	pushParseState(cstat);
	if (!isNextToken(cstat, "||"))
	{
		popParseState(cstat);
		return bed;
	}

	aed = getLogicalOrExpr(cstat);
	if (!aed.type) {
		do_abort_compile(cstat, "Bad expression after ||.");
		free_expression(bed);
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, bed.expr);
	eef = append_inttype(cstat, NULL, PROG_IF, 0);
	out = chain_append(out, eef);
	out = append_inttype(cstat, out, PROG_INTEGER, 1);

	beef = append_inttype(cstat, NULL, PROG_JMP, 0);
	out = chain_append(out, beef);
	out = chain_append(out, aed.expr);

	for (end = aed.expr; end && end->next; end = end->next) ;

	eef->in.data.number = get_address(cstat, aed.expr, 0);
	beef->in.data.number = get_address(cstat, end, 1);
	
	popParseState(cstat);

	oed.type = aed.type;
	oed.expr = out;
	return oed;
}



struct expression_data
getConditionalExpr(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL};
	struct expression_data ced = {0, NULL};
	struct expression_data ted = {0, NULL};
	struct expression_data fed = {0, NULL};
	struct INTERMEDIATE* eef;
	struct INTERMEDIATE* beef;
	struct INTERMEDIATE* out;
	struct INTERMEDIATE* end;

	pushParseState(cstat);

	ced = getLogicalOrExpr(cstat);
	if (!ced.type) {
		revertParseState(cstat);
		return ced;
	}

	if (!isNextChar(cstat, '?'))
	{
		popParseState(cstat);
		return ced;
	}

	/* ted = getLogicalOrExpr(cstat); */
	ted = getConditionalExpr(cstat);
	if (!ted.type) {
		do_abort_compile(cstat, "Bad expression after ? in conditional expression.");
		free_expression(ced);
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, ':')) {
		do_abort_compile(cstat, "Expected : in conditional expression.");
		free_expression(ced);
		free_expression(ted);
		revertParseState(cstat);
		return oed;
	}

	fed = getConditionalExpr(cstat);
	if (!fed.type) {
		do_abort_compile(cstat, "Bad expression after : in conditional expression.");
		free_expression(ced);
		free_expression(ted);
		revertParseState(cstat);
		return oed;
	}

    if (!typeMatches(ted.type, fed.type)) {
		do_abort_compile(cstat, "Types of values must match before and after the : in a conditional expression.");
		free_expression(ced);
		free_expression(ted);
		free_expression(fed);
		revertParseState(cstat);
		return oed;
	}

	out = chain_append(NULL, ced.expr);
	eef = append_inttype(cstat, NULL, PROG_IF, 0);
	out = chain_append(out, eef);
	out = chain_append(out, ted.expr);
	beef = append_inttype(cstat, NULL, PROG_JMP, 0);
	out = chain_append(out, beef);
	out = chain_append(out, fed.expr);

	for (end = aed.expr; end && end->next; end = end->next) ;

	eef->in.data.number = get_address(cstat, fed.expr, 0);
	beef->in.data.number = get_address(cstat, end, 1);
	
	popParseState(cstat);

	if (ted.type == PROG_UNTYPED || fed.type == PROG_UNTYPED) {
		oed.type = PROG_UNTYPED;
	} else {
		oed.type = fed.type;
	}
	oed.expr = out;
	return oed;
}



struct expression_data
getAssignment(COMPSTATE* cstat)
{
	struct INTERMEDIATE* var;
	struct INTERMEDIATE* out;
	struct expression_data arred = {0, NULL};
	struct expression_data ved = {0, NULL};
	struct expression_data oed = {0, NULL};
	char* opertype = NULL;
	int invert = 0;
	int lvaltype = 0;

	pushParseState(cstat);

	var = getVariable(cstat);
	if (!var) {
		revertParseState(cstat);
		return getConditionalExpr(cstat);
	}
	lvaltype = getTypeOfVariable(cstat, var);

	if (isNextChar(cstat, '[')) {
		if (lvaltype != PROG_UNTYPED && getArrayDepth(lvaltype) < 1) {
			do_abort_compile(cstat, "Attempting to subscript a non-array variable in lvalue.");
			revertParseState(cstat);
			return oed;
		}
		lvaltype = decrementArrayDepth(lvaltype)

		arred = getExpression(cstat);
		if (!arred.type) {
			do_abort_compile(cstat, "Bad array subscript expression in lvalue.");
			revertParseState(cstat);
			return oed;
		}

		if (arred.type != PROG_UNTYPED &&
			arred.type != PROG_STRING &&
			arred.type != PROG_INTEGER)
		{
			do_abort_compile(cstat, "Array subscript must be an integer or string.");
			free_expression(arred);
			revertParseState(cstat);
			return oed;
		}

		if (!isNextChar(cstat, ']')) {
			do_abort_compile(cstat, "Expected ']' at end of array subscript expression in lvalue.");
			free_expression(arred);
			revertParseState(cstat);
			return oed;
		}
	}

	if (!isNextToken(cstat, "="))
	{
		eatOptionalWhiteSpace(cstat);
		if (!cstat->next_char) {
			free_expression(arred);
			revertParseState(cstat);
			return getConditionalExpr(cstat);
		}
		opertype = *cstat->next_char;

		if (isNextToken(cstat, "*=")) {
			opertype = "*";
		} else if (isNextToken(cstat, "/=")) {
			opertype = "/";
		} else if (isNextToken(cstat, "%=")) {
			opertype = "%";
		} else if (isNextToken(cstat, "+=")) {
			opertype = "+";
		} else if (isNextToken(cstat, "-=")) {
			opertype = "-";
		} else if (isNextToken(cstat, "&=")) {
			opertype = "BITAND";
		} else if (isNextToken(cstat, "^=")) {
			opertype = "BITXOR";
		} else if (isNextToken(cstat, "|=")) {
			opertype = "BITOR";
		} else if (isNextToken(cstat, "<<=")) {
			opertype = "BITSHIFT";
		} else if (isNextToken(cstat, ">>=")) {
			opertype = "BITSHIFT";
			invert = 1;
		} else {
			free_expression(arred);
			revertParseState(cstat);
			return getConditionalExpr(cstat);
		}

	}

	ved = isAssignmentExpr(cstat);
	if (!ved.type) {
		do_abort_compile(cstat, "Bad value expression in assignment.");
		free_expression(arred);
		revertParseState(cstat);
		return oed;
	}

	if (opertype && *opertype != 'B' && lvaltype != PROG_UNTYPED && ved.type != PROG_UNTYPED) {
		if (lvaltype == PROG_OBJECT && ved.type != PROG_INTEGER)
		{
			do_abort_compile(cstat, "Only integers can be added to or subtracted from dbrefs in arithmetic assignment.");
			free_expression(arred);
			free_expression(ved);
			revertParseState(cstat);
			return oed;
		}
		else if (*opertype == '+' &&
			lvaltype == PROG_STRING &&
			ved.type != PROG_STRING)
		{
			do_abort_compile(cstat, "Only strings can be added to strings in += arithmetic assignment.");
			free_expression(arred);
			free_expression(ved);
			revertParseState(cstat);
			return oed;
		}
		else
		{
			if (lvaltype != PROG_INTEGER && lvaltype != PROG_FLOAT)
			{
				do_abort_compile(cstat, "Expression before = in arithmetic assignment must be a float, integer or dbref.");
				free_expression(arred);
				free_expression(ved);
				revertParseState(cstat);
				return oed;
			}
			if (ved.type != PROG_INTEGER && ved.type != PROG_FLOAT)
			{
				do_abort_compile(cstat, "Value after = in arithmetic assignment must be a float or integer.");
				free_expression(arred);
				free_expression(ved);
				revertParseState(cstat);
				return oed;
			}
		}
	}

	if (opertype && *opertype == 'B' &&
		((lvaltype != PROG_UNTYPED && lvaltype != PROG_INTEGER) ||
		 (ved.type != PROG_UNTYPED && ved.type != PROG_INTEGER)))
	{
		do_abort_compile(cstat, "Only integers can perform bitwise operations on integers in arithmetic assignment.");
		free_expression(arred);
		free_expression(ved);
		revertParseState(cstat);
		return oed;
	}

	if (!opertype && lvaltype != PROG_UNTYPED && ved.type != PROG_UNTYPED) {
		if (lvaltype != ved.type) {
			do_abort_compile(cstat, "Expression returns inapropriate data type for assignment.");
			free_expression(arred);
			free_expression(ved);
			revertParseState(cstat);
			return oed;
		}
	}

	if (opertype) {
		if (var.type == PROG_SVAR) {
			out = append_inttype(cstat, NULL, PROG_SVAR_AT, var->in.data.number);
		} else {
			out = append_inttype(cstat, NULL, var.type, var->in.data.number);
			out = append_primitive(cstat, out, "@");
		}
		if (arred.type) {
			out = chain_append(out, arred.expr);
			out = append_primitive(cstat, out, "ARRAY_GETITEM");
		}
	}

	out = chain_append(out, ved.expr);
	if (opertype) {
		if (invert) {
			out = append_integer(cstat, out, -1);
			out = append_primitive(cstat, out, "*");
		}
		out = append_primitive(cstat, out, opertype);
	}

	if (arred.type) {
		if (var.type == PROG_SVAR) {
			out = append_inttype(cstat, out, PROG_SVAR_AT, var->in.data.number);
		} else {
			out = append_inttype(cstat, out, var.type, var->in.data.number);
			out = append_PRIMITIVE(cstat, out, "@");
		}
		chain_append(out, arred.expr);
		append_primitive(cstat, out, "ARRAY_SETITEM");
	}

	if (var.type == PROG_SVAR) {
		out = append_inttype(cstat, out, PROG_SVAR_BANG, var->in.data.number);
	} else {
		out = append_inttype(cstat, out, var.type, var->in.data.number);
		out = append_PRIMITIVE(cstat, out, "!");
	}

	popParseState(cstat);

	if (!opertype) {
		oed.type = ved.type;
	} else {
		if (*opertype == 'B') {
			oed.type = PROG_INTEGER;
		} else if (lvaltype == PROG_UNTYPED || ved.type == PROG_UNTYPED) {
			oed.type = PROG_UNTYPED;
		} else if (lvaltype == PROG_OBJECT) {
			oed.type = PROG_OBJECT;
		} else if (lvaltype == PROG_FLOAT || ved.type == PROG_FLOAT) {
			oed.type = PROG_FLOAT;
		} else {
			oed.type = PROG_UNTYPED;
		}
	}
	oed.expr = out;
	return ved;
}


struct expression_data
getExpression(COMPSTATE* cstat)
{
	return getAssignment(cstat);
}


struct expression_data
getConditional(COMPSTATE* cstat)
{
	struct expression_data oed = {0, NULL}; /* Output ED */

	pushParseState(cstat);
	if (!isNextChar(cstat, '(')) {
		do_abort_compile(cstat, "Expected ( at start of conditional.");
		revertParseState(cstat);
		return oed;
	}

	oed = getExpression(cstat);
	if (!oed.type) {
		do_abort_compile(cstat, "Bad expression in conditional.");
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, ')')) {
		do_abort_compile(cstat, "Expected terminating ) at end of conditional.");
		free_expression(oed.expr);
		revertParseState(cstat);
		return oed;
	}

	popParseState(cstat);

	return oed;
}


struct expression_data
getIfStatement(COMPSTATE* cstat)
{
	struct expression_data ced = {0, NULL}; /* Conditional ED */
	struct expression_data ted = {0, NULL}; /* True branch ED */
	struct expression_data fed = {0, NULL}; /* False branch ED */
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct INTERMEDIATE* out = NULL;
	struct INTERMEDIATE* end;

	pushParseState(cstat);

	if (!isNextToken(cstat, "if")) {
		revertParseState(cstat);
		return oed;
	}

	ced = getConditional(cstat);
	if (!ced.type) {
		revertParseState(cstat);
		return oed;
	}

	ted = getStatement(cstat);
	if (!ted.type) {
		revertParseState(cstat);
		return oed;
	}

	if (isNextToken(cstat, "else")) {
		fed = getStatement(cstat);
		if (!fed.type) {
			revertParseState(cstat);
			return oed;
		}
	}

	out = chain_append(out, ced.expr);
	if (fed.expr) {
		for (end = fed.expr; end && end->next; end = end->next) ;
		out = append_inttype(cstat, out, PROG_IF, get_address(cstat, fed.expr, 0));
		out = chain_append(out, ted.expr);
		out = append_inttype(cstat, out, PROG_JMP, get_address(cstat, end, 1));
		out = chain_append(out, fed.expr);
	} else {
		for (end = ted.expr; end && end->next; end = end->next) ;
		out = append_inttype(cstat, out, PROG_IF, get_address(cstat, end, 1));
		out = chain_append(out, ted.expr);
	}

	popParseState(cstat);

	oed.type = PROG_VOID;
	oed.expr = out;
	return oed;
}


struct expression_data
getForStatement(COMPSTATE* cstat)
{
	struct expression_data ied = {0, NULL}; /* Initialization ED */
	struct expression_data ced = {0, NULL}; /* Conditional ED */
	struct expression_data ped = {0, NULL}; /* Post-loop ED */
	struct expression_data sed = {0, NULL}; /* Statement ED */
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct INTERMEDIATE* out = NULL;
	struct INTERMEDIATE* end;
	struct INTERMEDIATE* eef = NULL;

	pushParseState(cstat);

	if (!isNextToken(cstat, "for")) {
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, '(')) {
		do_abort_compile(cstat, "Expected ( at start of for statement.");
		revertParseState(cstat);
		return oed;
	}

	ied = getExpression(cstat);
	if (!ied.type) {
		do_abort_compile(cstat, "Bad initializer expression in for statement.");
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, ';')) {
		do_abort_compile(cstat, "Expected ; in for statement.");
		revertParseState(cstat);
		return oed;
	}

	ced = getExpression(cstat);
	if (!ced.type) {
		do_abort_compile(cstat, "Bad conditional expression in for statement.");
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, ';')) {
		do_abort_compile(cstat, "Expected ; in for statement.");
		revertParseState(cstat);
		return oed;
	}

	ped = getExpression(cstat);
	if (!ped.type) {
		do_abort_compile(cstat, "Bad post-loop expression in for statement.");
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, ')')) {
		do_abort_compile(cstat, "Expected terminating ) in for statement.");
		revertParseState(cstat);
		return oed;
	}

	addbegin(cstat, prealloc_inst(cstat));
	sed = getStatement(cstat);
	if (!sed.type) {
		do_abort_compile(cstat, "Bad body statement in for loop.");
		revertParseState(cstat);
		return oed;
	}
	for (end = ped.expr; end && end->next; end = end->next) ;

	/* WORK: need to make CONTINUE; statements jmp to the ped.expr */
	resolve_loop_addrs(cstat, get_address(cstat, end, 2));
	eef = find_begin(cstat);

	out = chain_append(out, ied.expr);
	out = chain_append(out, ced.expr);
	out = append_inttype(cstat, out, PROG_IF, get_address(cstat, end, 1));
	out = chain_append(out, sed.expr);
	out = chain_append(out, ped.expr);
	out = append_inttype(cstat, out, PROG_JMP, get_address(cstat, ced.expr, 0));

	popParseState(cstat);

	oed.type = PROG_VOID;
	oed.expr = out;
	return oed;
}


struct expression_data
getForeachStatement(COMPSTATE* cstat)
{
	struct expression_data aed = {0, NULL}; /* Conditional ED */
	struct expression_data sed = {0, NULL}; /* Statement ED */
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct INTERMEDIATE* keyvar = NULL;
	struct INTERMEDIATE* valvar = NULL;
	struct INTERMEDIATE* out = NULL;
	struct INTERMEDIATE* end;

	pushParseState(cstat);

	if (!isNextToken(cstat, "foreach")) {
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, '(')) {
		do_abort_compile(cstat, "Expected ( at start of foreach statement.");
		revertParseState(cstat);
		return oed;
	}

	aed = getExpression(cstat);
	if (!aed.type) {
		do_abort_compile(cstat, "Bad array expression in foreach statement.");
		revertParseState(cstat);
		return oed;
	}
	if (aed.type != PROG_UNTYPED && getArrayDepth(aed.type) < 1) {
		do_abort_compile(cstat, "Expected an expression returning an array in foreach statement.");
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, ';')) {
		do_abort_compile(cstat, "Expected ';' after array expression in foreach statement.");
		revertParseState(cstat);
		return oed;
	}

	keyvar = getVariable(cstat);
	if (!keyvar) {
		do_abort_compile(cstat, "Undeclared key variable in foreach statement.");
		revertParseState(cstat);
		return oed;
	}

	if (!isNextToken(cstat, "=>")) {
		do_abort_compile(cstat, "Expected => between key and value variables in foreach statement.");
		revertParseState(cstat);
		return oed;
	}

	valvar = getVariable(cstat);
	if (!valvar) {
		do_abort_compile(cstat, "Undeclared value variable in foreach statement.");
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, ')')) {
		do_abort_compile(cstat, "Expected terminating ) in foreach statement.");
		revertParseState(cstat);
		return oed;
	}

	addfor(cstat, prealloc_inst(cstat));
	sed = getStatement(cstat);
	if (!sed.type) {
		do_abort_compile(cstat, "Bad body statement in foreach loop.");
		revertParseState(cstat);
		return oed;
	}
	for (end = sed.expr; end && end->next; end = end->next) ;
	resolve_loop_addrs(cstat, get_address(cstat, end, 2));
	eef = find_begin(cstat);

	out = chain_append(out, aed.expr);
	out = append_inttype(cstat, out, PROG_PRIMITIVE, IN_FOREACH);
	eef = append_inttype(cstat, NULL, PROG_PRIMITIVE, IN_FORITER);
	out = chain_append(out, eef);
	out = append_inttype(cstat, out, PROG_IF, get_address(cstat, end, 2));
	if (valvar->type == PROG_SVAR) {
		out = append_inttype(cstat, out, PROG_SVAR_BANG, valvar->in.data.number);
	} else {
		out = append_inttype(cstat, out, valvar->in.type, valvar->in.data.number);
		out = append_primitive(cstat, out, "!");
	}
	if (keyvar->type == PROG_SVAR) {
		out = append_inttype(cstat, out, PROG_SVAR_BANG, keyvar->in.data.number);
	} else {
		out = append_inttype(cstat, out, keyvar->in.type, keyvar->in.data.number);
		out = append_primitive(cstat, out, "!");
	}
	out = chain_append(out, sed.expr);
	out = append_inttype(cstat, out, PROG_JMP, get_address(cstat, eef, 0));
	out = append_inttype(cstat, out, PROG_PRIMITIVE, IN_FORPOP);

	popParseState(cstat);

	oed.type = PROG_VOID;
	oed.expr = out;
	return oed;
}


struct expression_data
getWhileStatement(COMPSTATE* cstat)
{
	struct expression_data ced = {0, NULL}; /* Conditional ED */
	struct expression_data sed = {0, NULL}; /* Statement ED */
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct INTERMEDIATE* out = NULL;
	struct INTERMEDIATE* end;
	struct INTERMEDIATE* eef = NULL;

	pushParseState(cstat);

	if (!isNextToken(cstat, "while")) {
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, '(')) {
		do_abort_compile(cstat, "Expected ( at start of while statement conditional.");
		revertParseState(cstat);
		return oed;
	}

	ced = getExpression(cstat);
	if (!ced.type) {
		do_abort_compile(cstat, "Bad conditional expression in while statement.");
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, ')')) {
		do_abort_compile(cstat, "Expected terminating ) at end of conditional in while statement.");
		revertParseState(cstat);
		return oed;
	}

	addbegin(cstat, prealloc_inst(cstat));
	sed = getStatement(cstat);
	if (!sed.type) {
		do_abort_compile(cstat, "Bad body statement in while loop.");
		revertParseState(cstat);
		return oed;
	}
	for (end = sed.expr; end && end->next; end = end->next) ;
	resolve_loop_addrs(cstat, get_address(cstat, end, 2));
	eef = find_begin(cstat);

	out = chain_append(out, ced.expr);
	out = append_inttype(cstat, out, PROG_IF, get_address(cstat, end, 1));
	out = chain_append(out, sed.expr);
	out = append_inttype(cstat, out, PROG_JMP, get_address(cstat, ced.expr, 0));

	popParseState(cstat);

	oed.type = PROG_VOID;
	oed.expr = out;
	return oed;
}


struct expression_data
getDoLoopStatement(COMPSTATE* cstat)
{
	struct expression_data ced = {0, NULL}; /* Conditional ED */
	struct expression_data sed = {0, NULL}; /* Statement ED */
	struct expression_data oed = {0, NULL}; /* Output ED */
	struct INTERMEDIATE* out = NULL;
	struct INTERMEDIATE* end = NULL;
	struct INTERMEDIATE* eef = NULL;
	int inverseflag = 0;

	pushParseState(cstat);

	if (!isNextToken(cstat, "do")) {
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, '{')) {
		do_abort_compile(cstat, "Expected { after do.");
		revertParseState(cstat);
		return oed;
	}

	addbegin(cstat, prealloc_inst(cstat));
	sed = getStatement(cstat);
	if (!sed.type) {
		do_abort_compile(cstat, "Bad body statement in while loop.");
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, '}')) {
		do_abort_compile(cstat, "Expected } at end of do-loop body.");
		revertParseState(cstat);
		return oed;
	}

	if (!isNextToken(cstat, "while")) {
		if (!isNextToken(cstat, "until")) {
			do_abort_compile(cstat, "Expected while or until at end of do-loop.");
			revertParseState(cstat);
			return oed;
		} else {
			inverseflag = 0;
		}
	} else {
		inverseflag = 1;
	}

	if (!isNextChar(cstat, '(')) {
		do_abort_compile(cstat, "Expected ( at start of do-loop conditional.");
		revertParseState(cstat);
		return oed;
	}

	ced = getExpression(cstat);
	if (!ced.type) {
		do_abort_compile(cstat, "Bad conditional expression in do-loop.");
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, ')')) {
		do_abort_compile(cstat, "Expected terminating ) at end of conditional in do-loop.");
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, ';')) {
		do_abort_compile(cstat, "Expected ; at end of conditional in do-loop.");
		revertParseState(cstat);
		return oed;
	}

	for (end = ced.expr; end && end->next; end = end->next) ;
	resolve_loop_addrs(cstat, get_address(cstat, end, 2));
	eef = find_begin(cstat);

	out = chain_append(out, sed.expr);
	out = chain_append(out, ced.expr);
	if (inverseflag) {
		out = append_primitive(cstat, out, "NOT");
	}
	out = append_inttype(cstat, out, PROG_IF, get_address(cstat, sed.expr, 0));

	popParseState(cstat);

	oed.type = PROG_VOID;
	oed.expr = out;
	return oed;
}


int
isStatementEnd(COMPSTATE* cstat)
{
	return isNextChar(cstat, ';');
}


struct expression_data
getReturnStatement(COMPSTATE* cstat)
{
	struct INTERMEDIATE* out = NULL;
	struct expression_data oed = {0, NULL};

	pushParseState(cstat);
	if (!isNextToken(cstat, "return")) {
		revertParseState(cstat);
		return oed;
	}

	if (!isNextChar(cstat, ';')) {
		ed = getExpression(cstat);
		if (!ed.type)
		{
			do_abort_compile(cstat, "Bad expression or missing ; in return statement.");
			revertParseState(cstat);
			return oed;
		}
		if (cstat->returntype != PROG_UNTYPED && ed.type != cstat->returntype)
		{
			do_abort_compile(cstat, "Return value does not match declared function return type.");
			revertParseState(cstat);
			return oed;
		}
		if (!isNextChar(cstat, ';')) {
			do_abort_compile(cstat, "Expected ; at end of return statement.");
			revertParseState(cstat);
			return oed;
		}
		out = ed.expr;
		oed.type = ed.type;
	}
	else 
	{
		if (cstat->returntype != PROG_VOID)
		{
			do_abort_compile(cstat, "Non-void procedure returning void.");
			revertParseState(cstat);
			return oed;
		}
		oed.type = PROG_VOID;
	}

	out = append_inttype(cstat, out, PROG_PRIMITIVE, IN_RET);

	popParseState(cstat);
	oed.expr = out;
	return oed;
}


int
isBreakStatement(COMPSTATE* cstat)
{
	struct INTERMEDIATE* beef;
	struct INTERMEDIATE* out;

	pushParseState(cstat);
	if (!isNextToken(cstat, "break")) {
		revertParseState(cstat);
		return 0;
	}

	if (!isStatementEnd(cstat)) {
		do_abort_compile(cstat, "Expected a ';' at the end of this statement.");
		revertParseState(cstat);
		return 0;
	}

	beef = locate_begin(cstat);
	if (!beef) {
		do_abort_compile(cstat, "Break statement must be inside loop.");
		revertParseState(cstat);
		return 0;
	}

	out = new_inst(cstat);
	out->no = cstat->nowords++;
	out->in.type = PROG_JMP;
	out->in.line = cstat->lineno;
	out->in.data.number = 0;

	addwhile(cstat, out);
	append_intermediate(cstat, out);

	popParseState(cstat);
	return 1;
}


int
isContinueStatement(COMPSTATE* cstat)
{
	struct INTERMEDIATE* beef;
	struct INTERMEDIATE* out;

	pushParseState(cstat);
	if (!isNextToken(cstat, "continue")) {
		revertParseState(cstat);
		return 0;
	}

	if (!isStatementEnd(cstat)) {
		do_abort_compile(cstat, "Expected a ';' at the end of this statement.");
		revertParseState(cstat);
		return 0;
	}

	beef = locate_begin(cstat);
	if (!beef) {
		do_abort_compile(cstat, "Continue statement must be inside loop.");
		revertParseState(cstat);
		return 0;
	}

	out = new_inst(cstat);
	out->no = cstat->nowords++;
	out->in.type = PROG_JMP;
	out->in.line = cstat->lineno;
	out->in.data.number = beef->no;

	append_intermediate(cstat, out);

	popParseState(cstat);
	return 1;
}


int
isStatement(COMPSTATE* cstat)
{
	pushParseState(cstat);

	if (isStatementBlock(cstat) || 
		getScopedVarDeclAssign(cstat) && isStatementEnd(cstat) || 
		isIfStatement(cstat) ||
		isReturnStatement(cstat) ||
		isForStatement(cstat) ||
		isForeachStatement(cstat) ||
		isWhileStatement(cstat) ||
		isDoLoopStatement(cstat) ||
		isBreakStatement(cstat) ||
		isContinueStatement(cstat) ||
		isExpression(cstat) && isStatementEnd(cstat)
		)
	{
		popParseState(cstat);
		return 1;
	}

	revertParseState(cstat);
	return 0;
}



int
isStatementBlock(COMPSTATE* cstat)
{
	pushParseState(cstat);

	if (!isNextChar(cstat, '{')) {
		revertParseState(cstat);
		return 0;
	}

	while (!isNextChar(cstat, '}')) {
		if (!isStatement(cstat)) {
			do_abort_compile(cstat, "Bad statement syntax.");
			revertParseState(cstat);
			return 0;
		}
	}

	popParseState(cstat);
	return 1;
}


int
getLocalVarDecl(COMPSTATE* cstat)
{
	char ident[MAX_IDENT_LEN];
	struct INTERMEDIATE* out = NULL;
	struct expression_data ed = {0, NULL};
	int varno = 0;
	int vartype = 0;

	pushParseState(cstat);

	if ((vartype = getVarType(cstat))) {
	{
		if (!getIdentifier(cstat, ident, sizeof(ident)))
		{
			do_abort_compile(cstat, "Expected variable name.");
			revertParseState(cstat);
			return 0;
		}

		if (isNextChar(cstat, ';'))
		{
			varno = add_localvar(cstat, ident, vartype);
			popParseState(cstat);
			return 1;
		}
		else if (isNextChar(cstat, '='))
		{
			ed = getExpression(cstat);
			if (!ed.type)
			{
				do_abort_compile(cstat, "Bad variable initialization expression.");
				revertParseState(cstat);
				return 0;
			}
			if (vartype != PROG_UNTYPED && ed.type != vartype) {
				do_abort_compile(cstat, "Assigned value does not match declared variable type.");
				revertParseState(cstat);
				return 0;
			}

			if (!isNextChar(cstat, ';'))
			{
				do_abort_compile(cstat, "Expected ';' at end of variable declaration.");
				revertParseState(cstat);
				return 0;
			}

			varno = add_localvar(cstat, ident, vartype);

			/* remember initilization here */
			out = new_inst(cstat);
			out->no = cstat->nowords++;
			out->in.type = PROG_SVAR_BANG;
			out->in.line = cstat->lineno;
			out->in.data.number = varno;

			/* WORK: initcode needs to get initialized before this*/
			append_intermediate_chain(ed.expr, out);
			append_intermediate_chain(cstat->initcode, ed.expr);

			popParseState(cstat);
			return 1;
		}
		else
		{
			do_abort_compile(cstat, "Expected ';' at end of variable declaration.");
			revertParseState(cstat);
			return 0;
		}
	}
	revertParseState(cstat);
	return 0;
}


int
getProcedureAccess(COMPSTATE* cstat)
{
	char ident[MAX_IDENT_LEN];
	int result = 0;

	pushParseState(cstat);

	if (!getIdentifier(cstat, ident, sizeof(ident))) {
		result = ACCESS_NONE;
	} else if (!strcmp(ident, "private")) {
		result = ACCESS_PRIVATE;
	} else if (!strcmp(ident, "public")) {
		result = ACCESS_PUBLIC;
	} else if (!strcmp(ident, "wizcall")) {
		result = ACCESS_WIZCALL;
	} else {
		result = ACCESS_NONE;
	}

	if (result) {
		popParseState(cstat);
	} else {
		revertParseState(cstat);
	}
	return result;
}


int
getProcReturnType(COMPSTATE* cstat)
{
	char ident[MAX_IDENT_LEN];
	int outval = 0;

	pushParseState(cstat);
	if (!(outval = getVarType(cstat))) {
		if (!getIdentifier(cstat, ident, sizeof(ident))) {
			revertParseState(cstat);
			return 0;
		}
		if (!strcmp(cstat->next_char, "void")) {
			outval = PROG_VOID;
		} else if (!strcmp(cstat->next_char, "untyped")) {
			outval = PROG_UNTYPED;
		} else {
			revertParseState(cstat);
			return 0;
		}
	}
	popParseState(cstat);
	return outval;
}



int
getProcDeclArg(COMPSTATE* cstat)
{
	char ident[MAX_IDENT_LEN];
	int varno = 0;
	int vartype = 0;

	pushParseState(cstat);

	if ((vartype = getVarType(cstat))) {
	{
		if (!getIdentifier(cstat, ident, sizeof(ident)))
		{
			do_abort_compile(cstat, "Expected argument name.");
			revertParseState(cstat);
			return 0;
		}
		varno = add_scopedvar(cstat, ident, vartype);
		cstat->curr_proc->in.data.mufproc->vars++;
		cstat->curr_proc->in.data.mufproc->args++;

		popParseState(cstat);
		return 1;
	}

	do_abort_compile(cstat, "Expected argument type declaration.");
	revertParseState(cstat);
	return 0;
}



int
getProcDeclArgList(COMPSTATE* cstat)
{
	int result = 1;

	pushParseState(cstat);

	if (!getProcDeclArg(cstat))
		result = 0;

	if (result && isNextChar(cstat, ','))
	{
		getProcDeclArgList(cstat);
	}

	if (result) {
		popParseState(cstat);
	} else {
		revertParseState(cstat);
	}
	return result;
}


int
isProcedureDecl(COMPSTATE* cstat)
{
	struct INTERMEDIATE* out;
	char ident[MAX_IDENT_LEN];
	int i;
	int result = 0;
	int access = 0;
	int rettype = 0;

	pushParseState(cstat);

	access = getProcedureAccess(cstat);
	rettype = getProcReturnType(cstat);
	if (!rettype) {
		revertParseState(cstat);
		return 0;
	}

	if (!getIdentifier(cstat, ident, sizeof(ident))) {
		do_abort_compile(cstat, "Expected procedure name.");
		revertParseState(cstat);
		return 0;
	}

	if (!isNextChar(cstat, '(')) {
		revertParseState(cstat);
		return 0;
	}

	/* okay, we know this is supposed to be a procedure declaration */
	out = new_inst(cstat);
	out->no = cstat->nowords++;
	out->in.type = PROG_FUNCTION;
	out->in.line = cstat->lineno;
	out->in.data.mufproc = (struct muf_proc_data*)malloc(sizeof(struct muf_proc_data));
	out->in.data.mufproc->procname = string_dup(ident);
	out->in.data.mufproc->vars = 0;
	out->in.data.mufproc->args = 0;

	append_intermediate(cstat, out);

	cstat->curr_proc = out;
	add_proc(cstat, ident, out, rettype);

	if (isNextChar(cstat, ')'))
	{
		/* no args, but okay. */
	}
	else if (!getProcDeclArgList(cstat))
	{
		do_abort_compile(cstat, "Expected procedure arguments declarations.");
		revertParseState(cstat);
		return 0;
	}
	else
	{
		if (!isNextChar(cstat, ')'))
		{
			do_abort_compile(cstat, "Expected ')'.");
			revertParseState(cstat);
			return 0;
		}
	}

	if (!getStatementBlock(cstat))
	{
		do_abort_compile(cstat, "Expected procedure body.");
		revertParseState(cstat);
		return 0;
	}

	cstat->curr_proc = NULL;
	for (i = 0; i < MAX_VAR && cstat->scopedvars[i]; i++) {
		free((void *) cstat->scopedvars[i]);
		cstat->scopedvars[i] = 0;
	}

	popParseState(cstat);
	return 1;
}


int
isProgram(COMPSTATE* cstat)
{
	while (eatOptionalWhiteSpace(cstat) && cstat->next_char)
	{
		isProcedureDecl(cstat) ||
			isLocalVarDecl(cstat);
		if (cstat.compile_err)
			break;
	}

	return 1;
}



/*
function_definition
   : declarator function_body
   | declaration_specifiers declarator function_body
   ;

function_body
   : compound_statement
   ;

postfix_expr
   ...
   | postfix_expr '.' identifier
   ;

constant_expr
   : conditional_expr
   ;

declaration
   : declaration_specifiers ';'
   | declaration_specifiers init_declarator_list ';'
   ;

declaration_specifiers
   : storage_class_specifier
   | storage_class_specifier declaration_specifiers
   | type_specifier
   | type_specifier declaration_specifiers
   ;

init_declarator_list
   : init_declarator
   | init_declarator_list ',' init_declarator
   ;

init_declarator
   : declarator
   | declarator '=' initializer
   ;

storage_class_specifier
   : TYPEDEF
   | STATIC
   ;

type_specifier
   : type_specifier2
   | type_specifier2 AT CONSTANT
   ;

type_specifier2
   : STRING
   | INT
   | VOID
   | CONST
   | FLOAT
   | DBREF
   | struct_or_union_specifier
   | enum_specifier
   | TYPE_NAME
   ;

struct_or_union_specifier
   : struct_or_union opt_stag '{' struct_declaration_list '}'
   | struct_or_union stag
   ;

struct_or_union
   : STRUCT
   | UNION
   ;

opt_stag
   : stag
   |  // synthesize a name add to structtable
   ;

stag
   :  identifier    // add name to structure table
   ;

struct_declaration_list
   : struct_declaration
   | struct_declaration_list struct_declaration
   ;

struct_declaration
   : type_specifier_list struct_declarator_list ';'
           // add this type to all the symbols
   ;

struct_declarator_list
   : struct_declarator
   | struct_declarator_list ',' struct_declarator
   ;

struct_declarator
   : declarator
   | ':' constant_expr
   | declarator ':' constant_expr
   ;

enum_specifier
   : ENUM            '{' enumerator_list '}'
   | ENUM identifier '{' enumerator_list '}'
   | ENUM identifier
   ;

enumerator_list
   : enumerator
   | enumerator_list ',' enumerator
   ;

enumerator
   : identifier opt_assign_expr
   ;

opt_assign_expr
   :  '='   constant_expr
   |
   ;

declarator2
   : identifier
   | '(' declarator ')'
   | declarator2 '[' ']'
   | declarator2 '[' constant_expr ']'
   | declarator2 '('  ')'
   | declarator2 '(' parameter_type_list ')'
   | declarator2 '(' parameter_identifier_list ')'
   ;

type_specifier_list
   : type_specifier
   | type_specifier_list type_specifier
   ;

parameter_identifier_list
   : identifier_list
   | identifier_list ',' ELIPSIS
   ;

identifier_list
   : identifier
   | identifier_list ',' identifier
   ;

parameter_type_list
        : parameter_list
        | parameter_list ',' VAR_ARGS
        ;

parameter_list
   : parameter_declaration
   | parameter_list ',' parameter_declaration
   ;

parameter_declaration
   : type_specifier_list declarator
   | type_name
   ;

type_name
   : type_specifier_list
   | type_specifier_list abstract_declarator
   ;

abstract_declarator
   : pointer
   | abstract_declarator2
   | pointer abstract_declarator2
   ;

abstract_declarator2
   : '(' abstract_declarator ')'
   | '[' ']'
   | '[' constant_expr ']'
   | abstract_declarator2 '[' ']'
   | abstract_declarator2 '[' constant_expr ']'
   | '(' ')'
   | '(' parameter_type_list ')'
   | abstract_declarator2 '(' ')'
   | abstract_declarator2 '(' parameter_type_list ')'
   ;

initializer
   : assignment_expr
   | '{'  initializer_list '}'
   | '{'  initializer_list ',' '}'
   ;

initializer_list
   : initializer
   | initializer_list ',' initializer
   ;

statement
   : labeled_statement
   | compound_statement
   | expression_statement
   | selection_statement
   | iteration_statement
   | jump_statement
   ;

labeled_statement
   : identifier ':' statement
   | CASE constant_expr ':' statement
   | DEFAULT ':' statement
   ;

start_block : '{'
            ;

end_block   : '}'
            ;

compound_statement
   : start_block end_block
   | start_block statement_list end_block
   | start_block
          declaration_list
     end_block
   | start_block
          declaration_list
          statement_list
     end_block
   | error ';'
   ;

declaration_list
   : declaration
   | declaration_list declaration
   ;

statement_list
   : statement
   | statement_list statement
   ;

expression_statement
   : ';'
   | expr ';'
   ;

else_statement
   :  ELSE  statement
   |
   ;


selection_statement
   : IF '(' expr ')'  statement else_statement
   | SWITCH '(' expr ')' statement
   ;

iteration_statement
   : while '(' expr ')'  statement
   | do statement   WHILE '(' expr ')' ';'
   | for '(' expr_opt   ';' expr_opt ';' expr_opt ')'  statement
   ;

expr_opt
        :
        | expr
        ;

jump_statement
   : CONTINUE ';'
   | BREAK ';'
   | RETURN ';'
   | RETURN expr ';'
   ;

identifier
   : IDENTIFIER
   ;

*/

