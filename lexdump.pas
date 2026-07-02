(*=p-,t-,s8,u-,y+,k9,l0*)
program lexdump(output, tokens 100450000B);
const
    maxLineLen = 130;
    BACKSLASH = '\035';
    streamEnd = 7777777777777777C;
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
    bitset = set of 0..47;
    word = record case integer of
        0: (i: integer);
        1: (r: real);
        3: (a: alfa);
        4: (t: packed array [0..7] of '_000'..'_077');
        7: (c: char);
        13: (m: bitset)
    end;
    charbuf = (*packed*) array [1..maxLineLen] of char;
var
    tokens: file of integer;
    SY: symbol;
    charClass: operator;
    lineCnt, strLen, charEncoding: integer;
    curToken, w: word;
    strBuf, localBuf: charbuf;
    byteQ: array [1..6] of integer;
    byteQHead, byteQTail: integer;
    symTab: array [0..63] of symbol;
    opTab: array [0..63] of operator;
procedure PASTPR(val: word); external;

procedure clearByteQ;
{
    byteQHead := 1;
    byteQTail := 0;
};

procedure pushByte(b: integer);
{
    byteQTail := byteQTail + 1;
    byteQ[byteQTail] := b;
};

procedure enqueueWord(val: integer);
var idx: integer;
{
    w.i := val;
    for idx := 1 to 6 do
        pushByte(ord(w.a[idx]));
};

function getByte: integer;
{
    if byteQHead > byteQTail then {
        byteQHead := 1;
        byteQTail := 0;
        read(tokens, w.i);
        enqueueWord(w.i);
    };
    getByte := byteQ[byteQHead];
    byteQHead := byteQHead + 1;
};

function getWord: integer;
{
    clearByteQ;
    read(tokens, w.i);
    getWord := w.i;
};

procedure initTabs;
var s: symbol; o: operator;
{
    for s := IDENT to EOFSY do
        symTab[ord(s)] := s;
    for o := SHLEFT to NOOP do
        opTab[ord(o)] := o;
};

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
};

procedure prStrA2;
var
    i, idx, nWords, pos: integer;
{
    write(' value ');
    write('"');
    i := 1;
    nWords := (strLen + 5) div 6;
    for idx := 1 to nWords do {
        curToken.i := getWord;
        unpck(localBuf[1], curToken.a);
        for pos := 1 to 6 do {
            if i <= strLen then {
                writeEscapedChar(localBuf[pos], '"');
                i := i + 1;
            };
        };
    };
    write('"');
};

procedure prStrOct;
var
    idx, nWords: integer;
{
    write(' value ');
    nWords := (strLen + 7) div 8;
    for idx := 1 to nWords do {
        w.i := getWord;
        write(' ', w.m oct);
    };
};

procedure printPseudo;
var
    optCh: char;
    optVal: integer;
{
    optCh := chr(getByte);
    optVal := getByte;
    write(' PSEUDOSY value ', optCh);
    if optCh in ['Y', 'E', 'F', 'P', 'T', 'C', 'M'] then
        write(' ', optVal:0)
    else {
        write(' ', optVal:0);
        if optCh = 'A' then
            charEncoding := optVal;
        if (optCh = 'S') and (optVal = 3) then
            lineCnt := 1;
    };
    writeln;
};

procedure printToken;
{
    write(' ');
    write(SY:1);
    if SY = EOLSY then {
        lineCnt := lineCnt + 1;
        write(' of line ', lineCnt - 1:0);
    };
    if (SY = EXPROP) or (SY = BECOMES) then {
        w.i := getByte;
        charClass := opTab[w.i];
        write(' operator ', charClass:1);
    };
    if SY = IDENT then {
        curToken.i := getWord;
        write(' id ');
        pastpr(curToken);
    };
    if SY = INTCONST then {
        curToken.i := getWord;
        write(' value ', curToken oct);
    };
    if SY = REALCONST then {
        curToken.i := getWord;
        write(' value ', curToken.r);
    };
    if SY = CHARCONST then {
        strBuf[1] := chr(getByte);
        write(' value ');
        write('''');
        writeEscapedChar(strBuf[1], BACKSLASH);
        write('''');
    };
    if SY = STRINGSY then {
        strLen := getByte;
        if charEncoding = 2 then
            prStrA2
        else
            prStrOct;
    };
    writeln;
};

{ (* main *)
    reset(tokens);
    initTabs;
    lineCnt := 1;
    charEncoding := 2;
    clearByteQ;
    repeat
        w.i := getByte;
        SY := symTab[w.i];
        if SY = PSEUDOSY then
            printPseudo
        else
            printToken;
    until SY = EOFSY;
    if not eof(tokens) then
        read(tokens, w.i);
}
.data
end
