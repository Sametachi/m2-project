#include "Poly.h"
#include <string>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <Basic/Random.hpp>

using Random = effolkronium::random_static;

double _random()
{
	return std::rand() / (RAND_MAX + 1.0);
}

CBase::CBase()
{
	id = 0;
}

bool CBase::isNumber()
{
	return (id & MID_NUMBER) != 0 ? true : false;
}

bool CBase::isVar()
{
	return (id & MID_VARIABLE) != 0 ? true : false;
}

bool CBase::isSymbol()
{
	return (id & MID_SYMBOL) != 0 ? true : false;
}

CSymbol::CSymbol()
{
	id = MID_SYMBOL;
	iType = ST_UNKNOWN;
}

bool CSymbol::Equal(CSymbol dif)
{
	if (dif.iType / 10 == iType / 10)
		return true;
	return false;
}

bool CSymbol::Less(CSymbol dif)
{
	if (dif.iType / 10 > iType / 10)
		return true;
	return false;
}

int CSymbol::GetType()
{
	return iType;
}

void CSymbol::SetType(int32_t Type)
{
	iType = Type;
}

int32_t CSymbol::issymbol(int32_t ch)
{
	switch (ch) 
	{
	case SY_PLUS:
		return ST_PLUS;
	case SY_MINUS:
		return ST_MINUS;
	case SY_MULTIPLY:
		return ST_MULTIPLY;
	case SY_DIVIDE:
		return SY_DIVIDE;
	case SY_CARET:
		return SY_CARET;
	case SY_OPEN:
		return SY_OPEN;
	case SY_CLOSE:
		return ST_CLOSE;
	}

	return 0;
}

CSymTable::CSymTable(int32_t aTok, const std::string& aStr)
	: dVal(0)
	, token(aTok)
	, strlex(aStr)
{
}

CPoly::CPoly()
	: iToken(0)
	, iNumToken(0)
	, iLookAhead(0)
	, iErrorPos(0)
	, ErrorOccur(true)
	, uiLookPos(0)
	, STSize(0)
	, MathSymbolCount(0)
{
	lSymbol.clear();
	lSymbol.reserve(50);

	init();
}

CPoly::~CPoly()
{
	Clear();
}

void CPoly::SetRandom(int32_t iRandomType)
{
	m_iRandomType = iRandomType;
}

void CPoly::SetStr(const std::string& str)
{
	strData = str;
}

double CPoly::Eval() const
{
	int32_t stNow;
	double save[POLY_MAXSTACK]{}, t;
	int32_t iSp = 0;

	if (ErrorOccur) 
	{
		return 0;
	}

	auto pos = tokenBase.begin();
	auto posn = numBase.begin();

	while (pos != tokenBase.end()) 
	{
		stNow = *pos;
		++pos;
		switch (stNow) 
		{
		case POLY_NUM:
			save[iSp++] = *posn++;
			break;

		case POLY_ID:
			save[iSp++] = lSymbol[*pos]->dVal;
			pos++;
			break;

		case POLY_PLU:
			iSp--;
			save[iSp - 1] += save[iSp];
			break;

		case POLY_MIN:
			iSp--;
			save[iSp - 1] -= save[iSp];
			break;

		case POLY_MUL:
			iSp--;
			save[iSp - 1] *= save[iSp];
			break;

		case POLY_MOD:
			iSp--;
			if (save[iSp] == 0)
				return 0;

			save[iSp - 1] = fmod(save[iSp - 1], save[iSp]);
			break;

		case POLY_DIV:
			iSp--;
			if (save[iSp] == 0)
				return 0;

			save[iSp - 1] /= save[iSp];
			break;

		case POLY_POW:
			iSp--;
			save[iSp - 1] = pow(save[iSp - 1], save[iSp]);
			break;

		case POLY_ROOT:
			if (save[iSp - 1] < 0)
				return 0;

			save[iSp - 1] = sqrt(save[iSp - 1]);
			break;

		case POLY_COS:
			save[iSp - 1] = cos(save[iSp - 1]);
			break;

		case POLY_SIN:
			save[iSp - 1] = sin(save[iSp - 1]);
			break;

		case POLY_SIGN:
			if (save[iSp - 1] == 0.0)
				save[iSp - 1] = 0.0;
			else if (save[iSp - 1] < 0.0)
				save[iSp - 1] = -1.0;
			else
				save[iSp - 1] = 1.0f;

			break;

		case POLY_TAN:
			if (!(t = cos(save[iSp - 1])))
				return 0;

			save[iSp - 1] = tan(save[iSp - 1]);
			break;

		case POLY_CSC:
			if (!(t = sin(save[iSp - 1])))
				return 0;

			save[iSp - 1] = 1 / t;
			break;

		case POLY_SEC:
			if (!(t = cos(save[iSp - 1])))
				return 0;

			save[iSp - 1] = 1 / t;
			break;

		case POLY_COT:
			if (!(t = sin(save[iSp - 1])))
				return 0;

			save[iSp - 1] = cos(save[iSp - 1]) / t;
			break;

		case POLY_LN:
			if (save[iSp - 1] <= 0)
				return 0;

			save[iSp - 1] = log(save[iSp - 1]);
			break;

		case POLY_LOG10:
			if (save[iSp - 1] <= 0)
				return 0;

			save[iSp - 1] = log10(save[iSp - 1]);
			break;

		case POLY_LOG:
			if (save[iSp - 1] <= 0)
				return 0;

			if (save[iSp - 2] <= 0 || save[iSp - 2] == 1)
				return 0;

			save[iSp - 2] = log(save[iSp - 1]) / log(save[iSp - 2]);
			iSp--;
			break;

		case POLY_ABS:
			save[iSp - 1] = fabs(save[iSp - 1]);
			break;

		case POLY_FLOOR:
			save[iSp - 1] = floor(save[iSp - 1]);
			break;

		case POLY_CEIL:
			save[iSp - 1] = ceil(save[iSp - 1]);
			break;

		case POLY_IRAND:
			save[iSp - 2] = my_irandom(save[iSp - 2], save[iSp - 1]);
			iSp--;
			break;

		case POLY_FRAND:
			save[iSp - 2] = my_frandom(save[iSp - 2], save[iSp - 1]);
			iSp--;
			break;

		case POLY_MINF:
			save[iSp - 2] = (save[iSp - 2] < save[iSp - 1]) ? save[iSp - 2]
				: save[iSp - 1];
			iSp--;
			break;

		case POLY_MAXF:
			save[iSp - 2] = (save[iSp - 2] > save[iSp - 1]) ? save[iSp - 2]
				: save[iSp - 1];
			iSp--;
			break;

		default:
			return 0;
		}
	}
	return save[iSp - 1];
}

int32_t CPoly::Analyze(const char* pszStr)
{
	if (pszStr)
		SetStr(pszStr);

	if (0 == strData.length())
		return true;

	ErrorOccur = false;
	uiLookPos = 0;
	iLookAhead = lexan();

	expr();

	if (tokenBase.empty())
		return false;

	return !ErrorOccur;
}

void CPoly::Clear()
{
	int32_t i;
	tokenBase.clear();
	numBase.clear();

	for (i = 0; i < STSize; ++i) 
	{
		if (lSymbol[i]) 
		{
			delete lSymbol[i];
			lSymbol[i] = nullptr;
		}
	}

	lSymbol.clear();
	SymbolIndex.clear();
	STSize = 0;
	MathSymbolCount = 0;
}

void CPoly::expr()
{
	int32_t t;

	switch (iLookAhead) 
	{
	case '+':
	case '-':
		uiLookPos--;
		iLookAhead = POLY_NUM;
		iNumToken = iToken = 0;
	}

	term();

	while (!ErrorOccur) 
	{
		switch (iLookAhead) 
		{
		case '+':
		case '-':
			t = iLookAhead;
			match(t);
			term();
			emit(t, POLY_NONE);
			continue;

		case POLY_EOS:
		case ')':
		case ',':
			return;

		default:
			error();
			return;
		}
	}
}

void CPoly::error()
{
	iErrorPos = uiLookPos;
	ErrorOccur = true;
}

int32_t CPoly::lexan()
{
	int32_t t;
	double tt;

	while (uiLookPos < strData.size()) 
	{
		if (strData[uiLookPos] == ' ' || strData[uiLookPos] == '\t');
		else if (isdigit(strData[uiLookPos])) 
		{
			t = 0;
			for (; uiLookPos < strData.size(); uiLookPos++) 
			{
				if (isdigit(strData[uiLookPos]))
					t = t * 10 + strData[uiLookPos] - '0';
				else
					break;
			}

			iToken = t;
			tt = 0.1;
			iNumToken = 0;

			if (uiLookPos < strData.size() && strData[uiLookPos] == '.') 
			{
				uiLookPos++;
				for (; uiLookPos < strData.size(); uiLookPos++, tt *= 0.1) 
				{
					if (isdigit(strData[uiLookPos]))
						iNumToken += tt * (strData[uiLookPos] - '0');
					else
						break;
				}
			}

			iNumToken += iToken;
			return POLY_NUM;
		}
		else if (isalpha(strData[uiLookPos])) 
		{
			std::string localSymbol("");
			while (uiLookPos < strData.size() && isalpha(strData[uiLookPos])) 
			{
				localSymbol += strData[uiLookPos];
				uiLookPos++;
			}

			iToken = find(localSymbol);
			if (iToken == -1)
				iToken = emplace(localSymbol, POLY_ID);

			return lSymbol[(/*FindIndex*/ (iToken))]->token;
		}
		else 
		{
			iToken = 0;
			return strData[uiLookPos++];
		}

		uiLookPos++;
	}

	return POLY_EOS;
}

void CPoly::term()
{
	int32_t t;
	factor();

	while (!ErrorOccur) 
	{
		switch (iLookAhead)
		{
		case '*':
		case '/':
		case '%':
			t = iLookAhead;
			match(t);
			factor();
			emit(t, POLY_NONE);
			continue;

		default:
			return;
		}
	}
}

void CPoly::factor()
{
	int32_t t;
	expo();

	while (!ErrorOccur) 
	{
		switch (iLookAhead) 
		{
		case '^':
			t = iLookAhead;
			match(t);
			expo();
			emit(t, POLY_NONE);
			continue;

		default:
			return;
		}
	}
}

void CPoly::expo()
{
	int32_t t;
	switch (iLookAhead) 
	{
	case '(':
		match('(');
		expr();
		match(')');
		break;

	case POLY_NUM:
		emit(POLY_NUM, iToken);
		match(POLY_NUM);
		break;

	case POLY_ID:
		emit(POLY_ID, iToken);
		match(POLY_ID);
		break;

	case POLY_ROOT:
	case POLY_SIN:
	case POLY_COT:
	case POLY_TAN:
	case POLY_CSC:
	case POLY_SEC:
	case POLY_LN:
	case POLY_LOG10:
	case POLY_COS:
	case POLY_ABS:
	case POLY_FLOOR:
	case POLY_CEIL:
		t = iLookAhead;
		match(iLookAhead); match('('); expr(); match(')'); emit(t, iToken);
		break;
	case POLY_SIGN:
		t = iLookAhead;
		match(iLookAhead);
		match('(');
		expr();
		match(')');
		emit(t, iToken);
		break;

	case POLY_LOG:
	case POLY_MINF:
	case POLY_MAXF:
	case POLY_IRAND:
	case POLY_FRAND:
	case POLY_MOD:
		t = iLookAhead;
		match(iLookAhead);
		match('(');
		expr();
		match(',');
		expr();
		match(')');
		emit(t, iToken);
		break;

	case POLY_EOS:
		break;

	default:
		error();
	}
}

void CPoly::match(int32_t t)
{
	if (iLookAhead == t)
		iLookAhead = lexan();
	else
		error();
}

void CPoly::emit(int32_t t, int32_t tval)
{
	switch (t) 
	{
	case '+':
		tokenBase.emplace_back(POLY_PLU);
		break;

	case '-':
		tokenBase.emplace_back(POLY_MIN);
		break;

	case '*':
		tokenBase.emplace_back(POLY_MUL);
		break;

	case '/':
		tokenBase.emplace_back(POLY_DIV);
		break;

	case '%':
		tokenBase.emplace_back(POLY_MOD);
		break;

	case '^':
		tokenBase.emplace_back(POLY_POW);
		break;

	case POLY_ROOT:
	case POLY_SIN:
	case POLY_TAN:
	case POLY_COT:
	case POLY_COS:
	case POLY_CSC:
	case POLY_SEC:
	case POLY_LOG:
	case POLY_LN:
	case POLY_LOG10:
	case POLY_ABS:
	case POLY_MINF:
	case POLY_MAXF:
	case POLY_IRAND:
	case POLY_FRAND:
	case POLY_MOD:
	case POLY_FLOOR:
	case POLY_CEIL:
		tokenBase.emplace_back(t);
		break;
	case POLY_SIGN:
		tokenBase.emplace_back(t);
		break;

	case POLY_NUM:
		tokenBase.emplace_back(t);
		numBase.emplace_back(iNumToken);
		break;

	case POLY_ID:
		tokenBase.emplace_back(t);
		tokenBase.emplace_back(tval);
		break;

	default:
		error();
		Clear();
		return;
	}
}

int CPoly::find(const std::string& s)
{
	int32_t l, m, r;

	l = 0;
	r = STSize - 1;

	while (l <= r) 
	{
		m = (l + r) >> 1;

		if (lSymbol[SymbolIndex[m]]->strlex == s)
			return SymbolIndex[m];
		else if (lSymbol[SymbolIndex[m]]->strlex < s)
			l = m + 1;
		else
			r = m - 1;
	}

	return -1;
}

int CPoly::emplace(const std::string& s, int32_t tok)
{
	int32_t i;
	bool bAdded = false;

	lSymbol.emplace_back(new CSymTable(tok, s));

	for (i = 0; i < STSize; i++) 
	{
		if (s < lSymbol[SymbolIndex[i]]->strlex) 
		{
			SymbolIndex.emplace(SymbolIndex.begin() + i, STSize);
			bAdded = true;
			break;
		}
	}

	if (!bAdded)
		SymbolIndex.emplace_back(STSize);

	STSize++;
	return STSize - 1;
}

int32_t CPoly::SetVar(const std::string& strName, double dVar)
{
	if (ErrorOccur)
		return false;

	int index = find(strName);
	if (index == -1)
		return false;

	CSymTable* stVar = lSymbol[index];
	stVar->dVal = dVar;
	return true;
}

int32_t CPoly::GetVarCount()
{
	return lSymbol.size() - MathSymbolCount;
}

const char* CPoly::GetVarName(uint32_t dwIndex)
{
//	assert(dwIndex + MathSymbolCount < lSymbol.size());
	return lSymbol[dwIndex + MathSymbolCount]->strlex.c_str();
}

double CPoly::GetVar(const std::string& strName)
{
	if (ErrorOccur)
		return false;

	int index = find(strName);
	if (index == -1)
		return false;

	CSymTable* stVar = lSymbol[index];
	if (!stVar)
		return -1;

	return stVar->dVal;
}

void CPoly::init()
{
	emplace("min", POLY_MINF);
	emplace("max", POLY_MAXF);
	emplace("number", POLY_IRAND);
	emplace("irandom", POLY_IRAND);
	emplace("irand", POLY_IRAND);
	emplace("GetRandom", POLY_FRAND);
	emplace("frandom", POLY_FRAND);
	emplace("frand", POLY_FRAND);
	emplace("rt", POLY_ROOT);
	emplace("sqrt", POLY_ROOT);
	emplace("cos", POLY_COS);
	emplace("sin", POLY_SIN);
	emplace("tan", POLY_TAN);
	emplace("cot", POLY_COT);
	emplace("csc", POLY_CSC);
	emplace("cosec", POLY_COSEC);
	emplace("sec", POLY_SEC);
	emplace("pi", POLY_PI);
	SetVar("pi", 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117068);
	emplace("e", POLY_EXP);
	SetVar("e", 2.718281828459045235360287471352662497757247093699959574966967627724076630353547594571382178525166427);
	emplace("log", POLY_LOG);
	emplace("ln", POLY_LN);
	emplace("log10", POLY_LOG10);
	emplace("abs", POLY_ABS);
	emplace("mod", POLY_MOD);
	emplace("floor", POLY_FLOOR);
	emplace("sign", POLY_SIGN);
	emplace("sign", POLY_CEIL);
	MathSymbolCount = STSize;
}

int32_t CPoly::my_irandom(double start, double end) const
{
	// Make range as inclusive-exclusive
	int32_t is = int32_t(start + 0.5);
	int32_t ie = int32_t(end - start + 0.5) + 1;

	return Random::get<int32_t>(start, end);
}

double CPoly::my_frandom(double start, double end) const
{
	return Random::get<double>(start, end);
}
