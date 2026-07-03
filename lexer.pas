(*=p-,t-,s8,u-,y+,k9,l0*)
program lexer(output, pasinput 100440000B, pasinfor, isoptext,
              tokens 100450000B);
const
    maxLineLen = 130;
    ASN64 = 360100B;
    BACKSLASH = '\035';
    errNumberTooLarge = 43;
    errFirstDigitInCharLiteralGreaterThan3 = 60;
type
    symbol = (
(*0B*)  IDENT,      INTCONST,   REALCONST,  CHARCONST,
        STRINGSY,   LPAREN,     LBRACK,     EXPROP,
(*10B*) RPAREN,     RBRACK,     COMMA,      SEMICOLON,
        PERIOD,     ARROW,      COLON,      BECOMES,
(*20B*) BEGINSY,    ENDSY,      CONSTSY,    TYPEDEFSY,
        VARSY,      TYPESY,     ENUMSY,
(*30B*) PACKEDSY,   STRUCTSY,   IFSY,       SWITCHSY,
        WHILESY,    FORSY,      WITHSY,     GOTOSY,
(*40B*) ELSESY,     DOSY,
        EXTERNSY,   BREAKSY,    CONTSY,     CASESY,
(*50B*) DEFAULTSY,  UNIONSY,    NOSY,
        PSEUDOSY,   EOLSY,      EOFSY
    );
    operator = (
        SHLEFT,     SHRIGHT,
        SETAND,     SETXOR,     SETOR,
        MUL,        RDIVOP,     ANDOP,     IDIVOP,     IMODOP,
        PLUSOP,     MINUSOP,    OROP,       NEOP,       EQOP,
        LTOP,       GEOP,       GTOP,       LEOP,       INOP,
        IMULOP,     INTPLUS,    INTMINUS,   CONDOP,     ALTERN,
        INCROP,     DECROP,     ASSIGNOP,   GETELT,     GETVAR,
        RMWASSIGN,  op37,       GETENUM,    GETFIELD,   DEREF,
        STKLVAL,    ALNUM,      PCALL,      FCALL,
        TOREAL,     NOTOP,      INEGOP,     RNEGOP,     BITNEGOP,
        STANDPROC,  NOOP
    );
    numberFormat = (decimal, octal, fullword, hex);
    bitset = set of 0..47;
    word = record case integer of
        0: (i: integer);
        1: (r: real);
        3: (a: alfa);
        4: (t: packed array [0..7] of '_000'..'_077');
        7: (c: char);
        13: (m: bitset)
    end;
    charmap   = packed array ['_000'..'_176'] of char;
    textmap   = packed array ['_052'..'_177'] of '_000'..'_077';
    entries   = array [1..42] of bitset;
    charbuf   = (*packed*) array [1..maxLineLen] of char;
    kwordp    = @kword;
    kword = record
        next:   kwordp;
        w:      word;
        sym:    symbol;
        op:     operator
    end;
var
    pasinput: text;
    tokens: file of integer;
    pasinfor: record
        (*0*) listMode:     integer;
        (*1*) errors:       @boolean;
        (*2*) entryptr:     @entries;
        (*3*) startOffset:  integer;
      (*4-6*) a0, a1, a4:   @charmap;
        (*7*) a3:           @textmap;
     (*8-17*) sizes:        array [1..10] of @integer;
       (*18*) flags:        bitset;
        end;
    isoptext: textmap;
    SY: symbol;
    charClass: operator;
    CH, prevCH: char;
    atEOL, physicalEOF, eofEmitted, badScan: boolean;
    lineCnt, linePos, bucket, idx, strLen, badCol, formIdx,
        wasteByts, pendCnt: integer;
    lineBuf, strBuf: charbuf;
    curToken, curVal, curIdent, hashMask, forming: word;
    pendWords: array [1..6] of word;
    kwordHash: array [0..127] of kwordp;
    charSym: array ['_000'..'_177'] of symbol;
    chrClass: array ['_000'..'_177'] of operator;
    iso2text: array ['_052'..'_177'] of '_000'..'_077';
    resWordName: array [0..20] of integer;
    keywordHashPtr: kwordp;
    charEncoding: integer;
    intZero: bitset;
    numFormat: numberFormat;
    numstr: array [1..17] of word;
    localBuf: array [0..130] of char;
    curChar: char;
    dummyFlag: boolean;
procedure PASTPR(val: word); external;
procedure PASISOCD; external;
procedure PASCONTR; external;

procedure flushPendWords;
var i: integer;
{
    for i := 1 to pendCnt do
        write(tokens, pendWords[i].i);
    pendCnt := 0;
};

procedure emitByte(val : integer);
{
    formIdx := formIdx + 1;
    forming.a[formIdx] := chr(val);
    if formIdx = 6 then {
        write(tokens, forming.i);
        formIdx := 0;
        flushPendWords;
    }
};

procedure flushForm;
{
    if formIdx <> 0 then
        wasteByts := wasteByts + (6 - formIdx);
    while formIdx <> 0 do
        emitByte(0);
};

procedure flushFPend;
{
    flushForm;
};

procedure emitPending;
{
    if (SY = STRINGSY) and (pendCnt > 0) then {
        if formIdx <> 0 then
            flushFPend
        else
            flushPendWords;
    }
};

procedure emitWord(val : integer);
{
    if (formIdx <> 0) or (pendCnt > 0) then {
        if pendCnt = 6 then {
            if formIdx <> 0 then
                flushFPend
            else
                flushPendWords;
        };
        pendCnt := pendCnt + 1;
        pendWords[pendCnt].i := val;
    end else
        write(tokens, val);
};

procedure nextCH;
{
% writeln(' reading byte ', PASINPUT@:3 oct);
    if eof(pasinput) or (PASINPUT@ = '_000') then {
        physicalEOF := true;
        atEOL := true;
        CH := ' ';
        exit;
    };
    atEOL := eoln(pasinput);
    CH := pasinput@;
    get(pasinput);
    if linePos < maxLineLen then {
        linePos := linePos + 1;
        lineBuf[linePos] := CH;
    };
}; (* nextCH *)

procedure endOfLine;
{
    lineCnt := lineCnt + 1;
    linePos := 0;
    if eof(pasinput) then
        physicalEOF := true;
}; (* endOfLine *)

function skipSp: boolean;
{
    while ((CH = ' ') or (CH = '_011')) and not atEOL do
        nextCH;
    skipSp := false;
}; (* skipSp *)

procedure markBad;
{
    badCol := linePos;
    badScan := true;
}; (* markBad *)

procedure reportBadScan;
{
    while not atEOL do nextCH;
    writeln(' ', lineBuf:linePos);
    writeln('?':badCol+1);
}; (* reportBadScan *)

procedure error(errno: integer);
{
    write(' line ', lineCnt:0, ' pos ', linePos:0, ' error ', errno:0, ': ');
    case errno of
    12:write('ILLEGAL SYMBOL');
    20:write('DIGIT ABOVE 7');
    43:write('NUMBER TOO LARGE');
    52:write('END OF FILE ENCOUNTERED');
    54:write('ERROR IN PSEUDO COMMENT');
    55:write('NUMBER ABOVE 16 DIGITS');
    56:write('AFTER DOT NO MANTISSA');
    57:write('NO EXPONENT AFTER E');
    58:write('EXPONENT ABOVE 18');
    59:write('IN LINE EOLN TRUE');
    60:write('1ST DIGIT IN BYTE ABOVE 3');
    61:write('EMPTY LINE');
    end;
    writeln;
    markBad;
    reportBadScan;
    halt;
}; (* error *)

procedure regResWord(l4arg1z: integer);
var
    kw: kwordp;
    l4var2z: word;
{
    curVal.i := l4arg1z;
    curVal.m := curVal.m * hashMask.m;
    mapai(curVal.a, curVal.i);
    l4var2z.i := l4arg1z;
    new(kw);
    with kw@ do {
        w := l4var2z;
        sym := SY;
        op := charClass;
        next := kwordHash[curVal.i];
    };
    kwordHash[curVal.i] := kw;
}; (* regResWord *)

procedure regKeywords;
{
    SY := EXPROP;
    charClass := INOP;
    regResWord(5156C(*"      IN"*));
    SY := CONSTSY;
    charClass := NOOP;
    for idx := 0 to 20 do {
        if SY <> TYPESY then
            regResWord(resWordName[idx]);
        SY := succ(SY);
    };
}; (* regKeywords *)
procedure printToken; forward;
procedure scanComment;
var badOpt, boolOpt, dummyFlag : boolean; optVal: integer; o:char;
procedure readOptVal(var res   : integer; limit: integer);
{
    nextCH;
    res := 0;
    while ('9' >= CH) and (CH >= '0') do {
        res := 10 * res + ord(CH) - ord('0');
        nextCH;
    };
    if res <= limit then badOpt := false;
}; (* readOptVal *)
procedure readOptFlag(var res: boolean);
{
    nextCH;
    if (CH = '-') or (CH = '+') then {
        res := CH = '+';
        badOpt := false;
    };
    nextCH;
}; (* readOptFlag *)
{
    nextCH;
    if CH = '=' then {
        repeat nextCH;
        badOpt := true;
        boolOpt := false;
        o := CH;
        case CH of
        'Y', 'E', 'F', 'P', 'T', 'C', 'M': {
            readOptFlag(dummyFlag);
            boolOpt := true;
        };
        'D': {
            readOptVal(curVal.i, 15);
        };
        'S': {
            readOptVal(optVal, 8);
            if optVal = 3 then lineCnt := 1
        };
        'L': readOptVal(optVal, 3);
        'A': { readOptVal(charEncoding, 3); optVal := charEncoding; };
        'B': readOptVal(optVal, 4);
        'K': readOptVal(optVal, 23);
        end;
        if badOpt then
            error(54) (* errErrorInPseudoComment *)
        else {
            write(' PSEUDOSY value ', o);
            emitByte(ord(PSEUDOSY));
            emitByte(ord(o));
            if (boolOpt) then {
                write(' ', ord(dummyflag):0);
                emitByte(ord(dummyflag));
            } else {
                write(' ', optVal:0);
                emitByte(optVal);
            };
            writeln;
        };
        until CH <> ',';
    };
        repeat
            while (CH <> '*') and not physicalEOF do {
                if atEOL then {
                    endOfLine;
                    SY := EOLSY;
                    printToken;
                };
                nextCH;
            };
            if physicalEOF then {
                error(52);
                exit;
            };
        nextCH
    until CH = '/';
    nextCH;
}; (* scanComment *)

procedure inSymbol;
label
    1473;
var
    tokenLen, tokenIdx: integer;
    expSign: boolean;
    expMultiple, expValue: real;
    expLiteral, expMagnitude: integer;
%
function shift(s: bitset; amt: integer): bitset;
var res : bitset; cur: integer;
{
res := [];
repeat
cur := minel(s);
s := s - [cur];
res := res + [cur+amt]
until s = [];
shift := res;
}; (* shift *)
%
procedure lexer;
label
    2175, 2233;
var done : boolean;
{
done := false;
        case SY of
            IDENT: {
                done := true;
                curToken.m := [];
                tokenLen := 1;
                repeat
                    curVal.c := iso2text[CH];
                    nextCH;
                    if 8 >= tokenLen then {
                        tokenLen := tokenLen + 1;
                        curToken := curToken;
                        besm(ASN64-6);
                        curToken:=;
                        curToken.m := curToken.m + curVal.m;
                    };
                until chrClass[CH] <> ALNUM;
                curVal.m := curToken.m * hashMask.m;
                mapAI(curVal.a, bucket);
                curIdent := curToken;
                keywordHashPtr := kwordHash[bucket];
                while keywordHashPtr <> NIL do {
                    if keywordHashPtr@.w = curToken then {
                        SY := keywordHashPtr@.sym;
                        charClass := keywordHashPtr@.op;
                        exit;
                    };
                    keywordHashPtr := keywordHashPtr@.next;
                };
                SY := IDENT;
            }; (* IDENT *)
            INTCONST: { (*=m-*)
                      done := true;
                SY := INTCONST;
                tokenLen := 0;
                repeat
                    tokenLen := tokenLen + 1;
                    if (tokenLen <= 17) then
                        numstr[tokenLen].i := ord(CH)-ord('0')
                    else {
                        error(55); (* errMoreThan16DigitsInNumber *)
                        tokenLen := 1;
                    };
                    nextCH;
                until charSym[CH] <> INTCONST;
(octdec)        {
                    if (numstr[1].i = 0) and (CH <> '.') then {
                        if (tokenLen = 1) and (CH = 'X') then {
                            (* Hex literal: 0Xhhh[U] *)
                            numFormat := hex;
                            nextCH;
                            curToken.i := 0C;
                            while (charSym[CH] = INTCONST)
                               or (('A' <= CH) and (CH <= 'F')) do {
                                curToken := curToken;
                                besm(ASN64-4);
                                curToken := ;
                                if charSym[CH] = INTCONST
                                then curVal.c := CH
                                else curVal.i := ord(CH)-55;
                                curToken.m := curToken.m + curVal.m * [44:47];
                                nextCH;
                            };
                            if CH = 'U' then
                                nextCH;
                            exit;
                        };
                        numFormat := OCTAL;
                        if CH = 'U' then {
                            numFormat := FULLWORD;
                            nextCH;
                        }
                    } else {
                        numFormat := DECIMAL;
                        exit octdec;
                    };
                    curToken.c := chr(0);
                    for tokenIdx to tokenLen do {
                        if 7 < numstr[tokenIdx].i then
                            error(20); (* errDigitGreaterThan7 *)
                        curToken := curToken;
                        besm(ASN64-3);
                        curToken:=;
                        curToken.m := numstr[tokenIdx].m * [45..47] +
                        curToken.m;
                    };
                    exit;
                }; (* octdec *)
                curToken.i := 0;
                for tokenIdx to tokenLen do {
                    if 109951162777 >= curToken.i then
                        curToken.i := 10 * curToken.i +
                            numstr[tokenIdx].i
                    else {
                        error(errNumberTooLarge);
                        curToken.i := 1;
                    };
                };
                if CH = 'U' then {
                    curToken.m := curToken.m - [0,1,3];
                    numFormat := FULLWORD;
                    nextCH;
                    exit;
                };
                expMagnitude := 0;
                if CH = '.' then {
                    nextCH;
                    if CH = '.' then
                        CH := ':'
                    else {
                        curToken.r := curToken.i;
                        SY := REALCONST;
                        if charSym[CH] <> INTCONST then
                            error(56) (* errNeedMantissaAfterDecimal *)
                        else
                            repeat
                                curToken.r := 10.0*curToken.r + ord(CH)-48;
                                expMagnitude := expMagnitude-1;
                                nextCH;
                            until charSym[CH] <> INTCONST;
                    };
                };
                if CH = 'E' then {
                    if expMagnitude = 0 then {
                        curToken.r := curToken.i;
                        SY := REALCONST;
                    };
                    expSign := false;
                    nextCH;
                    if CH = '+' then
                        nextCH
                    else if CH = '-' then {
                        expSign := true;
                        nextCH
                    };
                    expLiteral := 0;
                    if charSym[CH] <> INTCONST then
                        error(57) (* errNeedExponentAfterE *)
                    else
                        repeat
                            expLiteral := 10 * expLiteral + ord(CH) - 48;
                            nextCH
                        until charSym[CH] <> INTCONST;
                    if expSign then
                        expMagnitude := expMagnitude - expLiteral
                    else
                        expMagnitude := expMagnitude + expLiteral;
                };
                if expMagnitude <> 0 then {
                    expValue := 1.0;
                    expSign := expMagnitude < 0;
                    expMagnitude := abs(expMagnitude);
                    expMultiple := 10.0;
                    if 18 < expMagnitude then {
                        expMagnitude := 1;
                        error(58); (* errExponentGreaterThan18 *)
                    };
                    repeat
                        if odd(expMagnitude) then
                            expValue := expValue * expMultiple;
                        expMagnitude := expMagnitude div 2;
                        if expMagnitude <> 0 then
                            expMultiple := expMultiple*expMultiple;
                    until expMagnitude = 0;
                    if expSign then
                        curToken.r := curToken.r / expValue
                    else
                        curToken.r := curToken.r * expValue;
                } else
                    curToken.m := curToken.m - intZero;
                exit
            }; (* INTCONST *) (*=m+*)
            CHARCONST: {
                       done := true;
(loop)          {
                    for tokenIdx := 6 to 130 do {
                        nextCH;
                        if charSym[CH] = CHARCONST then {
                            nextCH;
                            exit loop;
                        };
            if atEOL then {
2175:           error(59); (* errEOLNInStringLiteral *)
                exit loop
            } else if CH = BACKSLASH then {
                curToken.m := [0..7,17,18,22,30,34,36,38]; (* escSet *)
                nextCH;
                if ord(CH) - ord('0') IN curToken.m then {
                    if (CH <= '7') then {
                        expLiteral  :=  0;
                        tokenLen  :=  0;
(octal)                 repeat
                            expLiteral := 8*expLiteral + ord(CH) - ord('0');
                            tokenLen := tokenLen + 1;
                            if ((tokenLen < 3) and
                                ('0' <= PASINPUT@) and (PASINPUT@ <= '7')) then
                                nextCH
                            else
                                exit octal;
                        until false;
                        if (255 < expLiteral) then
                            error(errFirstDigitInCharLiteralGreaterThan3);
                        localBuf[tokenIdx] := chr(expLiteral);
                    } else {
                        curVal.m := shift(curToken.m, -ord(CH)+ord('0'));
                        curVal.i := card(curVal.m) - 1;
                        curToken.i := 007101412151113C; (* escMap *)
                        curVal.m := shift(curToken.m, 6*curVal.i);
                        curVal.m := curVal.m * [42..47];
                       localBuf[tokenIdx]  :=  curVal.c;
                    }
                } else {
                    goto 2233;
                }
            } else
2233:           with PASINFOR do {
                    if charEncoding = 3 then {
                        if (ch < '*') or ('_176' < CH) then
                            curChar := chr(0)
                        else {
                            curChar := iso2text[CH];
                        }
                    } else {
                        curChar := CH;
                    };
                    localBuf[tokenIdx] := curChar;
                };
            };
            goto 2175
        };
        strLen := tokenIdx - 6;
        if strLen = 0 then {
            error(61); (* errEmptyString *)
            strLen := 1;
            localBuf[6] := ' ';
        };
        if strLen = 1 then
            SY := CHARCONST
        else
            SY := STRINGSY;
        for tokenLen := 1 to strLen do
            strBuf[tokenLen] := localBuf[5 + tokenLen];
     }; (* CHARCONST *)
     end; (* case *)
        if done then exit;
        prevCH := CH;
        nextCH;
        curToken.a := '      ';
        curToken.a[1] := prevCH;
        curToken.a[2] := CH;
        case curToken.a of
        '+=    ',
        '-=    ',
        '*=    ',
        '/=    ',
        '%=    ',
        '&=    ',
        '|=    ',
        '^=    ': { SY := BECOMES; nextCH };
        '<=    ': { charClass := LEOP; nextCH };
        '<<    ': { charClass := SHLEFT; nextCH;
                    if CH = '=' then { SY := BECOMES; nextCH } };
        '<:    ': { SY := BEGINSY; nextCH };
        '>>    ': { charClass := SHRIGHT; nextCH;
                    if CH = '=' then { SY := BECOMES; nextCH } };
        '>=    ': { charClass := GEOP; nextCH };
        ':>    ': { SY := ENDSY; nextCH };
        '==    ': { SY := EXPROP; charClass := EQOP; nextCH };
        '!=    ': { charClass := NEOP; nextCH };
        '->    ': { SY := ARROW; nextCH };
        '--    ': { charClass := DECROP; nextCH };
        '++    ': { charClass := INCROP; nextCH };
        '||    ': { charClass := OROP; nextCH };
        '&&    ': { charClass := ANDOP; nextCH };
        '/*    ': { scanComment; goto 1473 };
        '//    ': { while not atEOL do nextCH; goto 1473 };
        '..    ': { SY := COLON; nextCH };
        end;
}; (* lexer *)
%
{ (* inSymbol *)
1473:
    while skipSp do ;
    if physicalEOF then {
        if eofEmitted then
            SY := NOSY
        else {
            eofEmitted := true;
            SY := EOFSY;
        };
        exit;
    };
    if atEOL then {
        endOfLine;
        if not physicalEOF then
            nextCH;
        SY := EOLSY;
        exit;
    };
    SY := charSym[CH];
    charClass := chrClass[CH];
    if SY = NOSY then {
        error(12);
        exit;
    };
    lexer;
}; (* inSymbol *)

procedure writeEscapedChar(ch, quote: char);
var
    value: integer;
{
    if ch = quote then
        write(BACKSLASH, ch)
    else if ch = BACKSLASH then
        write(BACKSLASH, BACKSLASH)
    else if ord(ch) < 32 then {
        value := ord(ch);
        write(BACKSLASH);
        write(value div 64:0);
        write((value div 8) mod 8:0);
        write(value mod 8:0);
    } else
        write(ch);
}; (* writeEscapedChar *)

procedure writeBuf(var buf: charbuf; len: integer; quote: char);
var
   i : integer; w:word;
{
    write(quote);
    for i := 1 to len do
        writeEscapedChar(buf[i], quote);
    write(quote);
    i := 1;
    repeat
      pck(buf[i], w.a);
      emitWord(w.i);
      i := i + 6;
    until i > len;
}; (* writeBuf *)

procedure printToken;
var
    i: integer;
    w: word;
{
    write(' ');
    write(SY:1);
    emitByte(ord(SY));
    if (SY = EOLSY) then {
        write(' of line ', lineCnt-1:0);
    };
    if (SY = EXPROP) or (SY = BECOMES) then {
        write(' operator ', charClass:1);
        emitByte(ord(charClass));
    };
    if SY = IDENT then {
        write(' id ');
        pastpr(curToken);
        emitWord(curToken.i);
    };
    if SY = INTCONST then {
        write(' value ', curToken oct);
        emitWord(curToken.i);
    };
    if SY = REALCONST then {
        write(' value ', curToken.r);
        emitWord(curToken.i);
    };
    if (SY = CHARCONST) then {
        write(' value ');
        write('''');
        writeEscapedChar(strBuf[1], backslash);
        emitByte(ord(strBuf[1]));
        write('''');
    };
    if (SY = STRINGSY) then {
        write(' value ');
        emitByte(strLen);
        if charEncoding = 2 then {
            writeBuf(strBuf, strLen, '"');
        } else {
            (* not A2: pack the characters into 48-bit words (eight
               6-bit codes each) and print them as raw octal words *)
            i := 1;
            while i <= strLen do {
                w.m := [];
                for idx := 0 to 7 do {
                    w := w;
                    besm(ASN64-6);
                    w := ;
                    if i + idx <= strLen then {
                        curVal.c := strBuf[i + idx];
                        w.m := w.m + curVal.m;
                    };
                };
                write(' ', w.m oct);
                emitWord(w.i);
                i := i + 8;
            };
        };
    };
    emitPending;
    writeln;
}; (* printToken *)

procedure initTables;
{
    unpack(isoptext, iso2text, '_052');
    iso2text['_'] := iso2text['*'];
    regKeywords;
}; (* initTables *)

{ (* main *)
    rewrite(tokens);
    initTables;
    lineCnt := 1;
    linePos := 0;
    formIdx := 0;
    pendCnt := 0;
    wasteByts := 0;
    atEOL := false;
    physicalEOF := eof(pasinput);
    eofEmitted := false;
    badScan := false;
    charEncoding := 2;
    intZero := [0,1,3];
    if not physicalEOF then
        nextCH;
    repeat
        inSymbol;
        if badScan then {
            reportBadScan;
            halt;
        };
        if SY <> NOSY then
            printToken;
    until SY = EOFSY;
    pasisocd;
    flushFPend;
    writeln(' WASTED BYTES ', wasteByts:0);
    emitWord(7777777777777777C);
%    reset(tokens);
}
.data
    hashMask := 203407C;
    kwordHash := NIL:128;
    charSym := NOSY:128;
    chrClass := NOOP:128;
    charSym['0'] := INTCONST:10;
    chrClass['0'] := ALNUM:10;
    charSym['A'] := IDENT:26;
    chrClass['A'] := ALNUM:26;
    chrClass['_'] := ALNUM;
    charSym[''''] := CHARCONST;
    charSym['_'] := IDENT;
    charSym['<'] := EXPROP;
    charSym['>'] := EXPROP;
    chrClass['+'] := PLUSOP;
    chrClass['-'] := MINUSOP;
    chrClass['*'] := MUL;
    chrClass['/'] := RDIVOP;
    chrClass['%'] := IMODOP;
    chrClass['='] := ASSIGNOP;
    chrClass['&'] := SETAND;
    chrClass['|'] := SETOR;
    chrClass['^'] := SETXOR;
    chrClass['~'] := BITNEGOP;
    chrClass['>'] := GTOP;
    chrClass['<'] := LTOP;
    chrClass['!'] := NOTOP;
    chrClass['?'] := CONDOP;
    charSym['+'] := EXPROP;
    charSym['-'] := EXPROP;
    charSym['|'] := EXPROP;
    charSym['*'] := EXPROP;
    charSym['/'] := EXPROP;
    charSym['%'] := EXPROP;
    charSym['&'] := EXPROP;
    charSym[','] := COMMA;
    charSym['.'] := PERIOD;
    charSym['^'] := EXPROP;
    charSym['('] := LPAREN;
    charSym[')'] := RPAREN;
    charSym[';'] := SEMICOLON;
    charSym['['] := LBRACK;
    charSym[']'] := RBRACK;
    charSym['='] := BECOMES;
    charSym[':'] := COLON;
    charSym['!'] := EXPROP;
    charSym['~'] := EXPROP;
    charSym['?'] := EXPROP;
    resWordName :=
        4357566364C             (*"   CONST"*),
        64716045444546C         (*" TYPEDEF"*),
        664162C                 (*"     VAR"*),
        0C                      (*"was FUNCTION"*),
        45566555C               (*"    ENUM"*),
        1212604143534544C       (*"**PACKED"*),
        636462654364C           (*"  STRUCT"*),
        5146C                   (*"      IF"*),
        636751644350C           (*"  SWITCH"*),
        6750515445C             (*"   WHILE"*),
        465762C                 (*"     FOR"*),
        67516450C               (*"    WITH"*),
        47576457C               (*"    GOTO"*),
        45546345C               (*"    ELSE"*),
        4457C                   (*"      DO"*),
        457064456256C           (*"  EXTERN"*),
        4262454153C             (*"   BREAK"*),
        4357566451566545C       (*"CONTINUE"*),
        43416345C               (*"    CASE"*),
        44454641655464C         (*" DEFAULT"*),
        6556515756C             (*"   UNION"*);
end
