(*=p-,t-,s8,u-,y+,k9,l0*)
program pascompl(output, child, pasinput, pasinfor,paseofcd);
%
label 9999;
%
const
    boilerplate = ' PASCAL METAMORPH HELPER (2025) ';
%
    fnSQRT  = 0;  fnSIN  = 1;  fnCOS  = 2;  fnATAN  = 3;  fnASIN = 4;
    fnLN    = 5;  fnEXP  = 6;  fnABS =  7;  fnTRUNC = 8;  fnODD  = 9;
    fnORD   = 10; fnCHR  = 11; fnSUCC = 12; fnPRED  = 13; fnEOF  = 14;
    fnREF   = 15; fnEOLN = 16; (*   17   *) fnROUND = 18; fnCARD = 19;
    fnMINEL = 20; fnPTR  = 21; fnABSI = 22;
%
    S3 = 0;
    S4 = 1;
    S5 = 2;
    S6 = 3;
    NoPtrCheck = 4;
    NoStackCheck = 5;
%
    DebugCode  = 45;
    DebugPrint = 46;
    DebugEntry = 47;
%
    ASN64 = 360100B;
%
    errBooleanNeeded = 0;
    errIdentAlreadyDefined = 2;
    errNoIdent = 3;
    errNotAType = 4;
    errNoConstant = 6;
    errConstOfOtherTypeNeeded = 7;
    errTypeMustNotBeFile = 9;
    errNotDefined = 11;
    errBadSymbol = 12;
    errNeedOtherTypesOfOperands = 21;
    errWrongVarTypeBefore = 22;
    errUsingVarAfterIndexingPackedArray = 28;
    errNoSimpleVarForLoop = 30;
    errTooManyArguments = 38;
    errNoCommaOrParenOrTooFewArgs = 41;
    errNumberTooLarge = 43;
    errVarTooComplex = 48;
    errEOFEncountered = 52;
    errFirstDigitInCharLiteralGreaterThan3 = 60;
%
    macro = 100000000B;
    mcACC2ADDR = 6;
    mcPOP = 4;
    mcPUSH = 5;
    mcMULTI = 7;
    mcADDSTK2REG = 8;
    mcADDACC2REG = 9;
    mcODD = 10;
    mcROUND = 11;
    mcMINEL = 15;
    mcPOP2ADDR = 19;
    mcCARD = 23;
%
    ASCII0 =    4000007B;
    E1 =        4000010B;
    ZERO =      4000011B;
    MULTMASK =  4000012B;
    MANTISSA =  4000014B;
    MINUS1 =    4000017B;
    PLUS1 =     4000021B;
    BITS15 =    4000022B;
    REAL05 =    4000023B;
    ALLONES =   4000024B;
    HEAPPTR =   4000027B;

    KATX =      0000000B;
%   KSTX =      0010000B;
    KXTS =      0030000B;
    KADD =      0040000B;
    KSUB =      0050000B;
    KRSUB =     0060000B;
    KAMX =      0070000B;
    KXTA =      0100000B;
    KAAX =      0110000B;
    KAEX =      0120000B;
    KARX =      0130000B;
    KAVX =      0140000B;
    KAOX =      0150000B;
%   KDIV =      0160000B;
    KMUL =      0170000B;
    KAPX =      0200000B;
    KAUX =      0210000B;
    KACX =      0220000B;
    KANX =      0230000B;
    KYTA =      0310000B;
%   KASN =      0360000B;
    KNTR =      0370000B;
    KATI =      0400000B;
%   KSTI =      0410000B;
    KITA =      0420000B;
    KITS =      0430000B;
    KMTJ =      0440000B;
    KJADDM =    0450000B;
    KE74 =      0740000B;
    KUTC =      2200000B;
    CUTC =      2200000C;
    KWTC =      2300000B;
    CWTC =      2300000C;
    KVTM =      2400000B;
    KUTM =      2500000B;
%   KUZA =      2600000B;
%   KU1A =      2700000B;
    KUJ =       3000000B;
    KVJM =      3100000B;
    KVZM =      3400000B;
%   KV1M =      3500000B;
    KVLM =      3700000B;
%
    I7 =        34000000B;      (* frame pointer *)
    I8 =        40000000B;      (* const pointer *)
    I9 =        44000000B;      (* temp register *)
    I10 =       50000000B;      (* temp register *)
    I11 =       54000000B;      (* temp register *)
    I12 =       60000000B;      (* temp register *)
    I13 =       64000000B;      (* link register *)
    I14 =       70000000B;      (* temp register *)
    SP =        74000000B;      (* stack pointer, reg 15 *)
%
   maxLineLen = 130;
type
    symbol = (
(*0B*)  IDENT,      INTCONST,   REALCONST,  CHARCONST,
        LTSY,       GTSY,       NOTSY,      LPAREN,
(*10B*) LBRACK,     MULOP,      ADDOP,      RELOP,
        RPAREN,     RBRACK,     COMMA,      SEMICOLON,
(*20B*) PERIOD,     ARROW,      COLON,      BECOMES,
        BEGINSY,    ENDSY,
        LABELSY,    CONSTSY,    TYPESY,     VARSY,
(*32B*) FUNCSY,     VOIDSY,     ENUMSY,     PACKEDSY,
        ARRAYSY,    STRUCTSY,   FILESY,
(*41B*) IFSY,       SWITCHSY,     WHILESY,
        FORSY,      WITHSY,     GOTOSY,
(*47B*) ELSESY,     OFSY,       DOSY,
(*52B*) EXTERNSY,  BREAKSY, CONTSY, DEFAULTSY,
                 ASSNOP, NOSY
);
%
idclass = (
        TYPEID,     ENUMID,     ROUTINEID,  VARID,
        FORMALID,   FIELDID
);
%
insn = (
(*000*) ATX,   STX,   OP2,   XTS,   ADD,   SUB,   RSUB,  AMX,
(*010*) XTA,   AAX,   AEX,   ARX,   AVX,   AOX,   ADIVX, AMULX,
(*020*) APX,   AUX,   ACX,   ANX,   EADD,  ESUB,  ASX,   XTR,
(*030*) RTE,   YTA,   OP32,  OP33,  EADDI, ESUBI, ASN,   NTR,
(*040*) ATI,   STI,   ITA,   ITS,   MTJ,   JADDM, ELFUN,
(*047*) UTC,   WTC,   VTM,   UTM,   UZA,   U1A,   UJ,    VJM
);
%
setofsys = set of ident .. dosy;
%
operator = (
    SHLEFT,     SHRIGHT,
    SETAND,     SETXOR,     SETOR,
    MUL,        RDIVOP,     AMPERS,     IDIVOP,     IMODOP,
    PLUSOP,     MINUSOP,    OROP,       NEOP,       EQOP,
    LTOP,       GEOP,       GTOP,       LEOP,       INOP,
    IMULOP,
    SETSUB,     INTPLUS,    INTMINUS,   badop27,    badop30,
    badop31,    MKRANGE,    ASSIGNOP,   GETELT,     GETVAR,
    op36,       op37,       GETENUM,    GETFIELD,   DEREF,
    FILEPTR,    op44,       ALNUM,      PCALL,      FCALL,
    BOUNDS,     TOREAL,     NOTOP,      INEGOP,     RNEGOP,
    STANDPROC,  NOOP
);
%
opgen = (
    gen0,  STORE, LOAD,  FORMOP,  SETREG,
    SETREG9,  STOREAT9,  gen7,  gen8,  DFLTWDTH,
    FRACWIDTH, gen11, gen12, FILEACCESS, FILEINIT,
    BRANCH, PCKUNPCK, LITINSN
);
%
% Flags for ops that can potentially be optimized if one operand is a constant
opflg = (
    opfCOMM, opfHELP, opfAND, opfOR, opfDIV, opfMOD, opfSHIFT,
    opfMULMSK, opfASSN, opfINV
);
%
kind = (
    kindReal, kindScalar, kindRange, kindPtr,
    kindArray, kindStruct, kindFile,
    kindCases
);
%
bitset = set of 0..47;
%
eptr = @expr;
tptr = @types;
irptr = @identrec;
%
word = record case integer of
    0: (i: integer);
    1: (r: real);
    2: (b: boolean);
    3: (a: alfa);
    4: (t: packed array[0..7] of '_000' .. '_077');
    7: (c: char);
    8: (cl: idclass);
    13: (m: bitset)
    end;
%
oiptr = @oneinsn;
%
oneinsn  = record
    next: oiptr;
    mode, code, offset: integer;
end;
%
ilmode = (ilCONST, il1, il2, il3);
state = (st0, st1, st2);
%
insnltyp  = record
    next, next2: oiptr;
    typ: tptr;
    regsused: bitset;
    ilm: ilmode;
    ilf5: word;
    ilf6: integer;
    ilf7: integer;
    st: state;
    width, shift: integer
end;
%
types = record
    size,
    bits:   integer;
    k:      kind;
    case kind of
    kindReal:   ();
    kindRange:  (base:      tptr;
                 checker,
                 left,
                 right:     integer);
    kindArray:  (abase,
                 range:     tptr;
                 pck:       boolean;
                 perword,
                 pcksize:   integer);
    kindScalar: (enums:     irptr;
                 numen,
                 start:     integer);
    kindPtr:    (sbase:      tptr);
    kindFile:   (fbase:      tptr;
                 elsize:    integer);
    kindStruct: (ptr1,
                 ptr2:      irptr;
                 flag,
                 pckrec:    boolean);
    kindCases:  (sel:       word;
                 first,
                 next:      tptr;
                 r6:        tptr)
    end;
%
typechain = record
    next:         @typechain;
    type1, type2: tptr;
end;

charmap   = packed array ['_000'..'_176'] of char;
textmap   = packed array ['_052'..'_177'] of '_000'..'_077';
%
four = array [1..4] of integer;
entries   = array [1..42] of bitset;
%
expr = record
    case operator of (* arbitrary so far *)
    NOOP:       (val:    word;
                 d3:     word;
                 d1, d2: word);
    MUL:        (typ:    tptr;
                 op:     operator;
                 expr1, expr2: eptr);
    BOUNDS:     (d4, d5: word;
                 typ1, typ2: tptr);
    NOTOP:      (d6, d7: word;
                 id1, id2: irptr);
    STANDPROC:  (d8, d9: word;
                 num1, num2: integer);
end;
%
kword = record
    w:      word;
    sym:    symbol;
    op:     operator;
    next:   @kword;
end;
%
strLabel = record
    next:       @strLabel;
    ident:      word;
    offset:     integer;
    exitTarget: integer;
end;
%
numLabel = record
    id:         word;
    line:       integer;
    frame:      integer;
    offset:     integer;
    next:       @numLabel;
    defined:    boolean;
end;
%
identrec = record
    id:     word;
    offset: integer;
    next:   irptr;
    typ: tptr;
    cl: idclass;
    case idclass of
    TYPEID,
    VARID:  ();
    ENUMID,
    FORMALID:
            (list: irptr; value: integer);
    FIELDID:
            (maybeUnused: integer;
             uptype: tptr;
             pckfield:  boolean;
             shift:     integer;
             width:     integer);
    ROUTINEID:
            (low: integer;
             high: word;
             argList, preDefLink: irptr;
             level, pos: integer;
             flags: bitset
            );
end;
extfilerec = record
    id:     word;
    offset: integer;
    next:   @extfilerec;
    location,
    line: integer
end;
numberFormat = (decimal, octal, fullword);
%
var
   numFormat: numberFormat;
   bigSkipSet, statEndSys, blockBegSys, statBegSys,
   skipToSet, lvalOpSet: setofsys;
   bool47z, bool48z, forValue: boolean;
   dataCheck: boolean;
   jumpType: integer;
   jumpTarget: integer;
   exitTarget: integer;
   charClass: operator;
   SY, prevSY: symbol;
   savedObjIdx: integer;
   FcstCnt: integer;
   symTabPos: integer;
   entryPtCnt: integer;
   fileBufSize: integer;
   expr62z, expr63z: eptr;
   curInsnTemplate: integer;
   linePos: integer;
   prevErrPos: integer;
   errsInLine: integer;
   moduleOffset: integer;
   lineStartOffset: integer;
   curFrameRegTemplate: integer;
   curProcNesting: integer;
   totalErrors: integer;
   lineCnt: integer;
   bucket: integer;
   strLen: integer;
   heapCallsCnt: integer;
   heapSize: integer;
   arithMode: integer;
   stmtName: alfa;
   keywordHashPtr: @kword;
   curVarKind: kind;
   curExternFile: @extfilerec;
   commentModeCH: char;
   CH: char;
   debugLine: integer;
   lineNesting: integer;
   FcstCountTo500: integer;
   objBufIdx: integer;
   int92z, int93z, int94z: integer;
   prevOpcode: integer;
   charEncoding: integer;
   errLine: integer;
   atEOL: boolean;
   checkTypes: boolean;
   isDefined, putLeft, readNext: boolean;
   errors: boolean;
   declExternal: boolean;
   rangeMismatch: boolean;
   doPMD: boolean;
   checkBounds: boolean;
   fuzzReals: boolean;
   fixMult: boolean;
   bool110z: boolean;
   allowCompat: boolean;
   checkFortran: boolean;
   outputFile: irptr;
   inputFile: irptr;
   programObj: irptr;
   hashTravPtr: irptr;
   uProcPtr: irptr;
   externFileList: @extfilerec;
   baseType, typ121z: tptr;
   pointerType: tptr;
   booleanType: tptr;
   textType: tptr;
   integerType: tptr;
   realType: tptr;
   charType: tptr;
   alfaType: tptr;
   arg1Type: tptr;
   arg2Type: tptr;
   numLabList: @numLabel;
   chain: @typechain;
   curToken: word;
   curVal: word;
   O77777: bitset;
   intZero: bitset;
   unused138z, extSymMask: bitset;
   halfWord: bitset;
   leftInsn: bitset;
   hashMask: word;
   curIdent: word;
   toAlloc, set145z, set146z, set147z, set148z: bitset;
   optSflags: word;
   litOct: word;
   litForward: word;
   litFortran: word;
   uVarPtr: eptr;
   curExpr: eptr;
   insnList: @insnltyp;
   fileForOutput, fileForInput: @extfilerec;
   maxSmallString: integer;
   smallStringType: array [2..6] of tptr;
   symTabCnt: integer;
   symtabarray: array [1..80] of word;
   symtbidx: array [1..80] of integer;
   iMulOpMap: array [MUL..IMODOP] of operator;
   iAddOpMap: array [PLUSOP..MINUSOP] of operator;
   entryPtTable: entries;
   frameRestore: array [3..6] of four;
   indexreg: array [1..15] of integer;
   opToInsn: array [SHLEFT..op44] of integer;
   opToMode: array [SHLEFT..op44] of integer;
   opFlags: array [SHLEFT..op44] of opflg;
   funcInsn: array [0..23] of integer;
   insnTemp: array [insn] of integer;
   frameRegTemplate: integer;
   constRegTemplate: integer;
   disNormTemplate: integer;
   lineBuf: array [1..130] of char;
   errMap: array [0..9] of integer;
   chrClass: array ['_000'..'_177'] of operator;
   kwordHash: array [0..127] of @kword;
   charSym: array ['_000'..'_177'] of symbol;
   symHash: array [0..127] of irptr;
   typeHash: array [0..127] of irptr;
   helperMap: array [1..99] of integer;
   helperNames: array [1..99] of bitset;
%
   symTab:array [74000B..75500B] of bitset;
   systemProcNames: array [0..22] of integer;
   resWordName: array [0..25] of integer;
   longSymCnt: integer;
   longSym: array [1..90] of integer;
   longSyms: array [1..90] of bitset;
   constVals: array [1..500] of alfa;
   constNums: array [1..500] of integer;
   objBuffer: array [1..1024] of bitset;
   iso2text: array ['_052'..'_177'] of '_000'..'_077';
   fcst: file of bitset; (* last *)
%
    pasinput: text;
%
    child: file of bitset;
%
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
    paseofcd:alfa;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%              PROGRAMME                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure programme(var l2arg1z: integer; l2idr2z: irptr);
label 22420, 22421, 23301;
var
    preDefHead, typelist, scopeBound, l2var4z, curIdRec, workidr: irptr;
    isPredefined, done, inTypeDef: boolean;
    l2var10z: eptr;
    l2int11z: integer;
    l2var12z: word;
    l2typ13z, l2typ14z: tptr;
    labIter, labFence: @numLabel;
    strLabList: @strLabel;
%
    l2int18z, ii, localSize, l2int21z, jj: integer;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%              PrintErrMsg               %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure printErrMsg(errno: integer);
type
    errtxt = packed array [0..100] of '_000'..'_077';
var
    errptr: @errtxt;
    errtext: array [0..100] of '_000'..'_077';
    i: integer;
    c: char;
%
    function pasmitxt(errno: integer): @errtxt;
        fortran;
%
    function pasisoxt(txtchar: '_000'..'_077'): char;
        fortran;
%
{ (* PrintErrMsg *)
    write(' ');
    if errno >= 200 then
        write('system=', errno:0)
    else {
        if (errno > 88) then
            printErrMsg(86)
        else if errno in [16..18, 20] then {
            if errno = 20 then
                errno := ord(sy = ident)*2 + 1
            else
                write(curToken.i:0,' ');
        };
        errptr := pasmitxt(errno);
        unpack(errptr@, errtext, 0);
(loop)  for i:=0 to 100 do {
            c := pasisoxt(errtext[i]);
            if c = '*' then
                exit loop;
            write(c);
        };
        write(' ');
        if errno in [17, 22] then
            if errno = 17 then
                write(errLine:0)
            else
                write(stmtName);
    };
    if errno <> 86 then
        writeln;
}; (* PrintErrMsg *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%              printTextWord             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure printTextWord(val: word);
%
    procedure PASTPR(val: word);
        external;
%
{ (* printTextWord *)
    write(' ');
    PASTPR(val)
}; (* printTextWord *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%              makeStringType                %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure makeStringType(var res: tptr);
var span: tptr;
{
    if maxSmallString >= strLen then
        res := smallStringType[strLen]
    else {
        new(span = 7);
        new(res, kindArray);
        with span@ do {
            size := 1;
            checker := 0;
            bits := 12;
            k := kindRange;
            base := integerType;
            left := 1;
            right := strLen;
        };
        with res@ do {
            size := (strLen + 5) div 6;
            if size = 1 then
                bits := strLen * 8
            else
                bits := 0;
            k := kindArray;
            base := charType;
            range := span;
            pck := true;
            perword := 6;
            pcksize := 8;
        }
    }
}; (* makeStringType *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%              addToHashTab              %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure addToHashTab(arg: irptr);
{
    curVal.m := arg@.id.m * hashMask.m;
    mapai(curval.a, curval.i);
    arg@.next := symHash[curval.i];
    symHash[curval.i] := arg;
}; (* addToHashTab *)
%
procedure error(errno: integer);
    forward;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%              storeObjWord              %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure storeObjWord(insn: bitset);
{
    objBuffer[objBufIdx] := insn;
    moduleOffset := moduleOffset + 1;
    if objBufIdx = 1024 then {
        error(49); (* errTooManyInsnsInBlock *)
        objBufIdx := 1
    } else
        objBufIdx := objBufIdx + 1;
}; (* storeObjWord *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%              form1Insn                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure form1Insn(arg: integer);
var
    insn, opcode: word;
    half1, half2: bitset;
    pos: integer;
{
    insn.i := arg;
    opcode.m := insn.m * [0, 1, 3, 24..32];
    if opcode.i = insnTemp[UJ] then {
        if prevOpcode = opcode.i then
            exit;
        if putLeft and (prevOpcode = 1) then {
            pos := objBufIdx - 1;
            if objBuffer[pos] * [0..8] = [0, 1, 3..5, 8] then {
                prevOpcode := opcode.i;
                half1 := insn.m * [33..47];
                besm(ASN64-24);
                half1 :=;
                half2 := objBuffer[pos] * [9..23];
                besm(ASN64+24);
                half2 :=;
                objBuffer[pos] := [0, 1, 3, 4, 6, 28, 29] +
                    half1 + half2;
                exit;
            }
       }
    };
    prevOpcode := opcode.i;
    if (putLeft) then {
        leftInsn := insn.m * halfWord;
        besm(ASN64-24);
        leftInsn :=;
        putLeft := false
    } else {
        putLeft := true;
        storeObjWord(leftInsn + (insn.m * halfWord))
    }
}; (* form1Insn *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure form2Insn(i1, i2: integer);
{
    form1Insn(i1);
    form1Insn(i2);
}; (* form2Insn *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure form3Insn(i1, i2, i3: integer);
{
    form2Insn(i1, i2);
    form1Insn(i3);
}; (* form3Insn *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure disableNorm;
{
    if arithMode <> 1 then {
        form1Insn(disNormTemplate);
        arithMode := 1;
    }
}; (* disableNorm *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function getObjBufIdxPlus: integer;
{
   if putLeft then
       getObjBufIdxPlus := objBufIdx + 4096
   else
       getObjBufIdxPlus := objBufIdx
}; (* getObjBufIdxPlus *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure formJump(var arg: integer);
var
    pos: integer;
    isLeft: boolean;
{
    if prevOpcode <> insnTemp[UJ] then {
        pos := getObjBufIdxPlus;
        isLeft := putLeft;
        form1Insn(jumpType + arg);
        if putLeft = isLeft then
            pos := pos - 1;
        arg := pos;
    }
}; (* formJump *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure padToLeft;
{
    if not putLeft then
        form1Insn(insnTemp[UTC]);
    prevOpcode := 0;
}; (* padToLeft *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure formAndAlign(arg: integer);
{
    form1Insn(arg);
    padToLeft;
    prevOpcode := 1;
}; (* formAndAlign *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure putToSymTab(arg: bitset);
{
    symTab[symTabPos] := arg;
    if symTabPos = 75500B then {
        error(50); (* errSymbolTableOverflow *)
        symTabPos := 74000B;
    } else
        symTabPos := symTabPos + 1;
}; (* putToSymTab *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function allocExtSymbol(l3arg1z: bitset): integer;
var
    l3var1z: word;
    l3var2z: integer;
{
    allocExtSymbol := symTabPos;
    if (curVal.m * halfWord <> []) then {
        for l3var2z to longSymCnt do
            if (curVal.m = longSyms[l3var2z]) then {
                allocExtSymbol := longSym[l3var2z];
                exit
            };
        longSymCnt := longSymCnt + 1;
        if (longSymCnt >= 90) then {
            error(51); (* errLongSymbolOverflow *)
            longSymCnt := 1;
        };
        longSym[longSymCnt] := symTabPos;
        longSyms[longSymCnt] := curVal.m;
        l3arg1z := l3arg1z + [25];
    } else
        l3arg1z := l3arg1z + curVal.m;
    putToSymTab(l3arg1z);
}; (* allocExtSymbol *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function getHelperProc(l3arg1z: integer): integer;
{
    if (helperMap[l3arg1z] = 0) then {
        curVal.m := helperNames[l3arg1z];
        helperMap[l3arg1z] := allocExtSymbol(extSymMask);
    };
    getHelperProc := helperMap[l3arg1z] + (KVJM+I13);
}; (*getHelperProc *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure toFCST;
{
    write(FCST, curVal.m);
    FcstCnt := FcstCnt + 1;
}; (* toFCST *)
%
function bitsetcmp(a, b: alfa):boolean;
var i : integer; aw, bw: word;
{
(*  TODO: implement proper less-than comparison of full words
    aw.a := a; bw.a := b;
    for i := 0 to 47 do
        if (i in bw.m) and not (i in aw.m) then {
            bitsetcmp := true; exit
        } else if (i in aw.m) and not (i in bw.m) then {
            bitsetcmp := false; exit
        };
    bitsetcmp := false;
*)
    bitsetcmp := a < b;
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function addCurValToFCST: integer;
var
    low, high, mid: integer;
{
    low := 1;
    if (FcstCountTo500 = 0) then {
        addCurValToFCST := FcstCnt;
        FcstCountTo500 := 1;
        constVals[1] := curVal.a;
        constNums[1] := FcstCnt;
        toFCST;
    } else {
        high := FcstCountTo500;
        repeat
            mid := (low + high) div 2;
            if (curVal.a = constVals[mid]) then {
                addCurValToFCST := constNums[mid];
                exit
            };
            if bitsetcmp(curval.a, constVals[mid]) then
                high := mid - 1
            else
                low := mid + 1
        until high < low;
        addCurValToFCST := FcstCnt;
        if FcstCountTo500 <> 500 then {
            if bitsetcmp(curval.a, constVals[mid]) then
                high := mid
            else
                high := mid + 1;
            for mid := FcstCountTo500 downto high do {
                low := mid + 1;
                constVals[low] := constVals[mid];
                constNums[low] := constNums[mid];
            };
            FcstCountTo500 := FcstCountTo500 + 1;
            constVals[high] := curVal.a;
            constNums[high] := FcstCnt;
        };
        toFCST;
    }
}; (* addCurValToFCST *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function allocSymtab(l3arg1z: bitset): integer;
var
    low, high, mid: integer;
    value: word;
{
    low := 1;
    value.m := l3arg1z;
    if symTabCnt = 0 then {
        allocSymtab := symTabPos;
        symTabCnt := 1;
        symTabArray[1].m := l3arg1z;
        symtbidx[1] := symTabPos;
    } else {
        high := symTabCnt;
        repeat
            mid := (low + high) div 2;
            if (value = symTabArray[mid]) then {
                allocSymtab := symtbidx[mid];
                exit
            };
            if  value.a < symTabArray[mid].a then
                 high := mid - 1
            else
                 low := mid + 1;
        until high < low;
        allocSymtab := symTabPos;
        if symTabCnt <> 80 then {
            if value.a < symTabArray[mid].a then
                high := mid
            else
                high := mid + 1;
            for mid := symTabCnt downto high do {
                low := mid + 1;
                symTabArray[low] := symTabArray[mid];
                symtbidx[low] := symtbidx[mid];
            };
            symTabCnt := symTabCnt + 1;
            symTabArray[high] := value;
            symtbidx[high] := symTabPos;
        }
    };
    putToSymTab(value.m);
}; (* allocSymtab *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function getFCSToffset: integer;
var
    offset: word;
{
    getFCSToffset := addCurValToFCST;
    offset :=;
    if (offset.i < 2048) then {
        (* empty *)
    } else if (offset.i >= 4096) then
        error(204)
    else {
        getFCSToffset := allocSymtab(offset.m + [24]) - 70000B;
        exit
    }
}; (* getFCSToffset *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function nrOfBits(value: integer): integer;
{
    curVal.i := value;
    curVal.m := curVal.m * [7..47];
    nrOfBits := 48-minel(curval.m);
}; (* nrOfBits *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure defineRange(var res: tptr; l, r: integer);
var
    temp: tptr;
{
    new(temp=7);
    with temp@ do {
        size := 1;
        bits := 48;
        base := res;
        checker := 0;
        k := kindRange;
        curVal.i := l;
        curVal.m := curVal.m + intZero;
        left := curVal.i;
        curVal.i := r;
        curVal.m := curVal.m + intZero;
        right := curVal.i;
        if (left >= 0) then
            bits := nrOfBits(curVal.i);
        res := temp
    }
}; (* defineRange *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function getValueOrAllocSymtab(value: integer): integer;
{
    curVal.i := value;
    curVal.i := curVal.i MOD 32768;
    if (40000B >= curVal.i) then
        getValueOrAllocSymtab := curVal.i
    else
        getValueOrAllocSymtab :=
            allocSymtab((curVal.m + [24]) * halfWord);
}; (* getValueOrAllocSymtab *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure P0715(mode, arg: integer);
label 1;
var
    addr, insn, leftHalf: bitset;
    isLarge: boolean;
    work, offset: integer;
{
    if mode = 0 then {
        padToLeft;
        curVal.i := moduleOffset;
1:      addr := curval.m * [33..47];
        curVal := curVal;
        besm(ASN64-24);
        leftHalf:=;
        while arg <> 0 do {
            if 4096 < arg then {
                isLarge := true;
                arg := arg - 4096;
            } else isLarge := false;
            insn := objBuffer[arg];
            if isLarge then {
                curVal.m := insn * [9..23];
                besm(ASN64+24);
                curVal :=;
                curVal.m := curVal.m + intZero;
                insn := insn * [0..8, 24..47] + leftHalf;
            } else {
                curVal.m := intZero + insn * [33..47];
                insn := insn * [0..32] + addr;
            };
            objBuffer[arg] := insn;
            arg := curVal.i;
        };
        exit;
    } else if mode = 2 then {
        form1Insn(KVTM+I14 + curVal.i);
        if curVal.i = 74001B then
            form1Insn(KUTM+I14 + FcstCnt);
        form3Insn(KITA+14, insnTemp[ASN] + arg, KAOX+I7+1);
        form1Insn(KATX+I7+1);
        exit;
    } else if (mode = 1) or (mode < -2) then {
        arg := arg - curVal.i;
        offset := getFCSToffset;
        if mode = 1 then
            work := getHelperProc(68) + (-64200000B) (* P/DA *)
        else
            work := -mode;
        curVal.i := arg;
        arg := getFCSToffset;
        form3Insn(KATX+SP+1, KSUB+I8 + offset, work);
        form3Insn(KRSUB+I8 + arg, work, KXTA+SP+1);
        exit;
    } else if mode = -1 then {
        form1Insn(KVTM+I14 + lineCnt);
        formAndAlign(getHelperProc(arg));
        exit;
    };
    curVal.i := mode;
    goto 1;
}; (* P0715 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure endOfLine;
var
    err, errPos, prevPos, listMode,
    startPos, lastErr: integer;
{
    listMode := pasinfor.listMode;
    if (listMode <> 0) or (errsInLine <> 0) then
    {
        write(' ', (lineStartOffset + PASINFOR.startOffset):5 oct,
              lineCnt:5, lineNesting:3, commentModeCH);
        startPos := 13;
        repeat
            linePos := linePos-1
        until (lineBuf[linePos]  <> ' ') or (linePos = 0);
        for err to linePos do {
            output@ := lineBuf[err];
            put(output);
        };
        writeln;
        if errsInLine <> 0 then {
            write('*****':startPos, ' ':errMap[0], '0');
            lastErr := errsInLine - 1;
            for err to lastErr do {
                errPos := errMap[err];
                prevPos := errMap[err-1];
                if errPos <> prevPos then {
                    if prevPos + 1 <> errPos then
                        write(' ':(errPos-prevPos-1));
                    write(chr(err + 48));
                }
            };
            writeln;
            errsInLine := 0;
            prevErrPos := 0;
        }
    };
    lineStartOffset := moduleOffset;
    linePos := 0;
    lineCnt := lineCnt + 1;
    if eof(pasinput) then {
        error(errEOFEncountered);
        goto 9999;
    }
}; (* endOfLine *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure requiredSymErr(sym: symbol);
{
    if linePos <> prevErrPos then
        error(ord(sym) + 88);
}; (* requiredSymErr *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure readToPos80;
{
    while linePos < 81 do {
        linePos := linePos + 1;
        lineBuf[linePos] := PASINPUT@;
        if linePos <> 81 then get(PASINPUT);
    };
    endOfLine
}; (* readToPos80 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure inSymbol;
label
    1473, 1, 2, 2175, 2233, 2320;
var
    localBuf: array [0..130] of char;
    tokenLen, tokenIdx: integer;
    expSign: boolean;
    l3var135z: irptr;
    expMultiple, expValue: real;
    curChar: char;
    numstr: array [1..17] of word;
    l3vars2: array [155..159] of word;
    expLiteral, expMagnitude: integer;
    l3int162z: integer;
    chord: integer;
    l3var164z: integer;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure nextCH;
{
    repeat
        atEOL := eoln(PASINPUT);
        CH := PASINPUT@;
        get(PASINPUT);
        linePos := linePos + 1;
        lineBuf[linePos] := CH;
    until (maxLineLen >= linePos) or atEOL;
}; (* nextCH *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseComment;
var
    badOpt, flag: boolean;
    c: char;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure readOptVal(var res: integer; limit: integer);
{
    nextCH;
    res := 0;
    while ('9' >= CH) and (CH >= '0') do {
        res := 10 * res + ord(CH) - ord('0');
        nextCH;
        badOpt := false;
    };
    if limit < res then badOpt := true;
}; (* readOptVal *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure readOptFlag(var res: boolean);
{
    nextCH;
    if (CH = '-') or (CH = '+') then {
        res := CH = '+';
        badOpt := false;
    };
    nextCH
}; (* readOptFlag *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* parseComment *)
    nextCH;
    if CH = '=' then {
        repeat nextCH;
        badOpt := true;
        case CH of
        'D': {
            readOptVal(curVal.i, 15);
            optSflags.m := optSflags.m * [0..40] + curVal.m * [41..47];
        };
        'Y': readOptFlag(allowCompat);
        'E': readOptFlag(declExternal);
        'S': {
            readOptVal(curVal.i, 9);
            if curVal.i = 3 then lineCnt := 1
            else if curVal.i in [4..9] then
                optSflags.m := optSflags.m + [curVal.i - 3]
        };
        'F': readOptFlag(checkFortran);
        'L': readOptVal(PASINFOR.listMode, 3);
        'P': readOptFlag(doPMD);
        'T': readOptFlag(checkBounds);
        'A': readOptVal(charEncoding, 3);
        'C': readOptFlag(checkTypes);
        'R': readOptFlag(fuzzReals);
        'M': readOptFlag(fixMult);
        'B': readOptVal(fileBufSize, 4);
        'K': readOptVal(heapSize, 23);
        end;
        if badOpt then
            error(54); (* errErrorInPseudoComment *)
        until CH <> ',';
    };
    repeat
        while CH <> '*' do {
            c := commentModeCH;
            commentModeCH := '*';
            if atEOL then
                endOfLine;
            nextCH;
            commentModeCH := c;
        };
        nextCH
    until CH = '/';
    nextCH;
}; (* parseComment *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* inSymbol *)
        if dataCheck then {
            error(errEOFEncountered);
            readToPos80;
            goto 9999;
        };
1473:
        while (CH = ' ') and not atEOL do
            nextCH;
        if '_200' < CH then {
            lineBuf[linePos] := ' ';
            chord := ord(CH);
            for jj := 130 to chord do {
                linePos := linePos + 1;
                lineBuf[linePos] := ' ';
            };
            nextCH;
            goto 1473;
        };
        if atEOL then {
            endOfLine;
            nextCH;
            goto 1473;
        };
        hashTravPtr := NIL;
        SY := charSym[CH];
        charClass := chrClass[CH];
(lexer)
        if SY <> NOSY then {
            case SY of
            IDENT: {
1:              curToken.m := [];
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
                        exit lexer;
                    };
                    keywordHashPtr := keywordHashPtr@.next;
                };
                isDefined := false;
                SY := IDENT;
                case int93z of
                0: {
                    hashTravPtr := symHash[bucket];
                    while hashTravPtr <> NIL do {
                        if hashTravPtr@.offset = curFrameRegTemplate then
                        {
                            if hashTravPtr@.id <> curIdent then
                                hashTravPtr := hashTravPtr@.next
                            else {
                                isDefined := true;
                                exit lexer;
                            }
                        } else
                            exit lexer;
                    };
                };
                1: {
2:                  hashTravPtr := symHash[bucket];
                    while hashTravPtr <> NIL do {
                        if hashTravPtr@.id <> curIdent then
                            hashTravPtr := hashTravPtr@.next
                        else
                            exit lexer;
                    };
                };
                2: {
                    if expr63z = NIL then
                        goto 2;
                    expr62z := expr63z;
                    l3var135z := typeHash[bucket];
                    if l3var135z <> NIL then {
                        while expr62z <> NIL do {
                            l3int162z := expr62z@.typ2@.size;
                            hashTravPtr := l3var135z;
                            while hashTravPtr <> NIL do {
                                if (hashTravPtr@.id = curIdent)
                                and (hashTravPtr@.value = l3int162z) then
                                    exit lexer;
                                hashTravPtr := hashTravPtr@.next;
                            };
                            expr62z := expr62z@.expr1;
                        };
                    };
                    goto 2;
                };
                3: {
                    hashTravPtr := typeHash[bucket];
                    while hashTravPtr <> NIL do {
                        with hashTravPtr@ do {
                            if (id = curIdent) and
                               (typ121z = uptype)
                            then
                                exit lexer;
                            hashTravPtr := next;
                       }
                   }
                };
                end;
            }; (* IDENT *)
            INTCONST: { (*=m-*)
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
                    if numstr[1].i = 0 then {
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
                    if numFormat = OCTAL then {
                        if curToken.m * [0..6] <> [] then {
                            error(errNumberTooLarge);
                            curToken.i := 1;
                        } else
                            curToken.m := curToken.m + intZero;
                    };
                    exit lexer;
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
                    exit lexer;
                };
                expMagnitude := 0;
                if CH = '.' then {
                    nextCH;
                    if CH = '.' then {
                        CH := ':';
                        exit lexer
                    };
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
                };
                exit lexer
            }; (* INTCONST *) (*=m+*)
            CHARCONST: {
(loop)          {
                    for tokenIdx := 6 to 130 do {
                        nextCH;
                        if charSym[CH] = CHARCONST then {
                            nextCH;
                            if charSym[CH] <> CHARCONST then
                                exit loop
                            else
                                goto 2233;
                        };
                        if atEOL then {
2175:                       error(59); (* errEOLNInStringLiteral *)
                            exit loop
                        } else if (CH = '_035')
                               and (charSym[PASINPUT@] = INTCONST)
                        then {
                            expLiteral := 0;
                            for tokenLen to 3 do {
                                nextCH;
                                if '7' < CH then
                                    error(
                                        errFirstDigitInCharLiteralGreaterThan3
                                    );
                                expLiteral := 8*expLiteral + ord(CH) - 48;
                            };
                            if 255 < expLiteral then
                                error(errFirstDigitInCharLiteralGreaterThan3);
                            localBuf[tokenIdx] := chr(expLiteral);
                        } else
2233:                       with PASINFOR do {
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
                   goto 2320
                } else if strLen = 1 then {
                    SY := CHARCONST;
                    tokenLen := 1;
                    curToken.c := chr(0);
                    unpck(localBuf[0], curToken.a);
                    pck(localBuf[tokenLen], curToken.a);
                    exit lexer;
                } else 2320: {
                    curVal.a := '      ';
                    SY := LTSY;
                    unpck(localBuf[tokenIdx], curVal.a);
                    pck(localBuf[6], curToken.a);
                    curVal :=;
                    if strLen <= 6 then
                        exit lexer
                    else if (charEncoding = 3) and (strLen = 8) then {
                        pack(localbuf, 6, curToken.t);
                        curVal := ;
                        SY := INTCONST;
                        exit lexer
                    } else {
                        curToken.i := FcstCnt;
                        tokenLen := 6;
                        (loop) {
                            toFCST;
                            tokenLen := tokenLen + 6;
                            if tokenIdx < tokenLen then
                                exit lexer;
                            pck(localBuf[tokenLen], curVal.a);
                            goto loop
                        }
                    }
                };
            }; (* CHARCONST *)
            LTSY: {
                SY := RELOP;
                nextCH;
                case CH of
                '=': {
                    charClass := LEOP;
                    nextCH;
                };
                '<':   {
                    SY := MULOP;
                    charClass := SHLEFT;
                    nextCH;
                };
                ':': {
                    SY := BEGINSY;
                    nextCH;
                }
                end
            }; (* LTOP *)
            GTSY: {
                SY := RELOP;
                nextCH;
                case CH of
                '>': {
                    SY := MULOP;
                    charClass := SHRIGHT;
                    nextCH;
                };
                '=':  {
                    charClass := GEOP;
                    nextCH
                }
                end
            }; (* GTOP *)
            BECOMES: {
                nextCH;
                if CH = '=' then {
                    nextCH;
                    SY := RELOP;
                }
            };
            COLON: {
                nextCH;
                if CH = '>' then {
                    SY := ENDSY;
                    nextCH;
                }
            };
            NOTSY: {
                if charClass = NEOP then {
                    nextCH;
                    if CH = '=' then {
                        SY := RELOP;
                        nextCH;
                    }
                } else
                    nextCH
            };
            ADDOP: {
                if charClass = OROP then {
                    nextCH;
                    if CH = '|' then nextCH
                    else charClass := SETOR;
                } else if (charClass = MINUSOP) and (PASINPUT@ = '>') then {
                    SY := ARROW; nextCH; CH := '.';
                } else
                    nextCH;
            };
            MULOP: {
               if charClass = AMPERS then {
                   nextCH;
                   if CH = '&' then nextCH
                   else charClass := SETAND;
               } else if charClass = RDIVOP then {
                   nextCH;
                   case CH of
                   '*': {
                       parseComment;
                       goto 1473
                   };
                   '/': {
                       while not atEOL do nextCH;
                       goto 1473;
                   }
                   end
               } else
                   nextCH
            };
            LPAREN, LBRACK, RELOP, RPAREN, RBRACK,
            COMMA, SEMICOLON, ARROW: {
                nextCH;
            };
            PERIOD: {
                nextCH;
                if CH = '.' then {
                    nextCH;
                    SY := COLON;
                    charClass := NOOP
                } else {
                    if prevSY = ENDSY then
                        dataCheck := true;
                }
            };
            end; (* case *)
%            if (CH = '=') and (SY IN [ADDOP,MULOP])  then {
%                 SY := ASSNOP;
%writeln(' ASSNOP');
%                 nextCH;
%            }
        } else {
            nextCH;
        };
        prevSY := SY;
        commentModeCH := ' ';
        int93z := int92z;
}; (* inSymbol *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure error;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure skipToEnd;
var
    sym: symbol;
{
    sym := SY;
    while (sym <> ENDSY) or (SY <> PERIOD) do {
        sym := SY;
        inSymbol
    };
    if CH = 'D' then
        while SY <> ENDSY do
            inSymbol;
    goto 9999;
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* error *)
    errors := true;
    bool110z :=;
    if ((linePos <> prevErrPos) and (9 >= errsInLine))
        or (errno = 52)
    then {
        write(' ');
        totalErrors := totalErrors + 1;
        errMap[errsInLine] := linePos;
        errsInLine := errsInLine + 1;
        prevErrPos := linePos;
        write('******', errno:0);
        printErrMsg(errno);
        if 60 < totalErrors then {
            writeln;
            endOfLine;
            printErrMsg(53);
            skipToEnd
        }
    }
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure skip(toset: setofsys);
{
    while not (SY IN toset) do
        inSymbol;
}; (* skip *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure test1(sym: symbol; toset: setofsys);
{
    if (SY <> sym) then {
        requiredSymErr(sym);
        skip(toset)
    } else
        inSymbol;
}; (* test1 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure errAndSkip(errno: integer; toset: setofsys);
{
    error(errno);
    skip(toset)
}; (* errAndSkip *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseLiteral(var litType: tptr; var litValue: word;
    allowSign: boolean);
label
    99;
var
    l3var1z: operator;
{
    litValue := curToken;
    if (GTSY < SY) then {
        if allowSign and (charClass IN [PLUSOP, MINUSOP]) then {
            l3var1z := charClass;
            inSymbol;
            parseLiteral(litType, litValue, false);
            if (litType <> integerType) then {
                error(62); (* errIntegerNeeded *)
                litType := integerType;
                litValue.i := 1;
            } else {
                if (l3var1z = MINUSOP) then
                    litValue.i := -litValue.i;
            };
        } else
99:     {
            litType := NIL;
            error(errNoConstant);
        }
    } else
        case SY of
        IDENT: {
            if (hashTravPtr = NIL) or
               (hashTravPtr@.cl <> ENUMID) then
                goto 99;
            litType := hashTravPtr@.typ;
            litValue.i := hashTravPtr@.value;
        };
        INTCONST:
            litType := integerType;
        REALCONST:
            litType := realType;
        CHARCONST:
            litType := charType;
        LTSY:
            makeStringType(litType);
        end (* case *)
}; (* parseLiteral *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure P2672(var l3arg1z: irptr; l3arg2z: irptr);
var
    l3var1z: boolean;
    l3var2z: integer;
    l3var3z, l3var4z: irptr;
{
    if l3arg1z = NIL then {
        curVal.m := l3arg2z@.id.m * hashMask.m;
        mapAI(curVal.a, l3var2z);
        l3var1z := true;
        l3arg1z := symHash[l3var2z];
    } else {
        l3var1z := false;
    };
    if (l3arg1z = l3arg2z) then {
        if (l3var1z) then {
            symHash[l3var2z] :=
                symHash[l3var2z]@.next;
        } else {
            l3arg1z := l3arg2z@.next;
        };
    } else {
        l3var3z := l3arg1z;
        while (l3var3z <> l3arg2z) do {
            l3var4z := l3var3z;
            if (l3var3z <> NIL) then {
                l3var3z := l3var3z@.next;
            } else {
                exit
            }
        };
        l3var4z@.next := l3arg2z@.next;
    }
}; (* P2672 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function isFileType(typtr: tptr): boolean;
{
    isFileType := (typtr@.k = kindFile) or
        (typtr@.k = kindStruct) and typtr@.flag;
}; (* isFileType *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function knownInType(var rec: irptr): boolean;
{
    if (typelist <> NIL) then {
        rec := typelist;
        while (rec <> NIL) do {
            if (rec@.id = curIdent) then {
                knownInType := true;
                exit
            };
            rec := rec@.next;
        }
    };
    knownInType := false;
}; (* knownInType *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure checkSymAndRead(sym: symbol);
{
    if (SY <> sym) then {
        requiredSymErr(sym);
        writeln('         got ', SY);
    } else
        inSymbol
}; (* checkSymAndRead *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function typeCheck(type1, type2: tptr): boolean;
label
    1;
var
    baseMatch: boolean;
    kind1, kind2: kind;
    link: @typechain;
    basetyp1, basetyp2: tptr;
    enums1, enums2: irptr;
    span1, span2: integer;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure allocWithTypeCheck;
{
    new(link);
    link@ := [chain, basetyp1, basetyp2];
    chain := link;
    typeCheck := typeCheck(basetyp1, basetyp2);
}; (* allocWithTypeCheck *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function checkRecord(l4arg1z, l4arg2z: tptr): boolean;
var
    l4var1z: boolean;
{
    l4var1z := (l4arg1z = NIL) or (l4arg2z = NIL);
    if (l4var1z) then {
        checkRecord := l4arg1z = l4arg2z;
    } else {
        checkRecord := typeCheck(l4arg1z@.base, l4arg2z@.base) and
                 checkRecord(l4arg1z@.next, l4arg2z@.next);
    };
}; (* checkRecord *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* typeCheck *)
    rangeMismatch := false;
    if (type1@.k = kindRange) then {
        baseType := type1@.base;
    } else {
        baseType := type1;
    };
    if not checkTypes or (type1 = type2) then
1:      typeCheck := true
    else
        with type1@ do {
            kind1 := k;
            kind2 := type2@.k;
            if (kind1 = kind2) then {
                case kind1 of
                kindReal:
                    (* empty *);
                kindScalar: {
(chain)             if (type1@.numen = type2@.numen) then {
                        enums1 := type1@.enums;
                        enums2 := type2@.enums;
                        while (enums1 <> NIL) and (enums2 <> NIL) do {
                            if (enums1@.id <> enums2@.id) then
                                exit chain;
                            enums1 := enums1@.list;
                            enums2 := enums2@.list;
                        };
                        if (enums1 = NIL) and (enums2 = NIL) then
                            goto 1;
                    } else if ((type1 = booleanType) or (type1 = integerType)
%                            or (type1 = charType)
                          ) and ((type2 = booleanType) or (type2 = integerType)
%                            or (type2 = charType)
                           ) then
                       goto 1;
                };
                kindRange: {
                    baseMatch := (type1@.base = type2@.base);
                    baseType := type1@.base;
                    rangeMismatch := (type1@.left <> type2@.left) or
                                (type1@.right <> type2@.right);
                    typeCheck := baseMatch;
                    exit
                };
                kindPtr: {
                    if (type1 = pointerType) or (type2 = pointerType) then
                        goto 1;
                    basetyp1 := type1@.base;
                    basetyp2 := type2@.base;
                    if (chain <> NIL) then {
                        link := chain;
                        while (link <> NIL) do with link@ do {
                            if (type1 = basetyp1) and
                               (type2 = basetyp2) or
                               (type2 = basetyp1) and
                               (type1 = basetyp2) then
                                goto 1;
                            link := next;
                        };
                        allocWithTypeCheck;
                    } else {
                        setup(type1);
                        allocWithTypeCheck;
                        chain := NIL;
                        rollup(type1);
                        exit
                    }
                };
                kindArray: {
                    with type1@.range@ do
                        span1 := right - left;
                    with type2@.range@ do
                        span2 := right - left;
                    if typeCheck(type1@.base, type2@.base) and
                       (span1 = span2) and
                       (type1@.pck = type2@.pck) and
                       not rangeMismatch then {
                        if type1@.pck then {
                            if (type1@.pcksize = type2@.pcksize) then
                                goto 1
                        } else
                            goto 1
                    }
                };
                kindFile: {
                    if typeCheck(type1@.base, type2@.base) then
                        goto 1;
                };
                kindStruct: {
                    if checkRecord(type1@.first, type2@.first) then
                        goto 1;
                }
                end (* case *)
            } else {
                if (kind1 = kindRange) then {
                    rangeMismatch := true;
                    baseType := type2;
                    if (type1@.base = type2) then
                        goto 1;
                } else if (kind2 = kindRange) and
                          (type1 = type2@.base) then
                    goto 1;
            };
            typeCheck := false;
        }
}; (* typeCheck *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function F3307(l3arg1z: irptr): integer;
var
    l3var1z: integer;
    l3var2z: irptr;
{
    l3var2z := l3arg1z@.argList;
    l3var1z := 0;
    if (l3var2z <> NIL) then
        while (l3var2z <> l3arg1z) do {
            l3var1z := l3var1z + 1;
            l3var2z := l3var2z@.list;
        };
    F3307 := l3var1z;
}; (* F3307 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function makeNameWithStars: bitset;
{
    while curVal.m * [0..5] = [] do {
        curVal := curVal;
        besm(ASN64-6);
        curVal := ;
    };
    makeNameWithStars := curVal.m;
}; (* makeNameWithStars *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure formOperator(l3arg1z: opgen);
var
    l3int1z, l3int2z, l3int3z : integer;
    nextInsn                  : integer;
    l3var5z                   : eptr;
    flags                     : opflg;
    direction                 : boolean;
    noTarget                  : boolean;
    l3var10z, l3var11z        : word;
    saved                     : @insnltyp;
    l3bool13z                 : boolean;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genOneOp;
label
    3556;
var
    insnBufIdx: integer;
    l4var2z, l4var3z, l4var4z: integer;
    l4var5z: word;
    l4inl6z, l4inl7z, l4inl8z: oiptr;
    l4var9z: integer;
    insnBuf: array [1..200] of word;
    curInsn: word;
    tempInsn: word;
    l4oi212z: oiptr;
    l4var213z: boolean;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure P3363;
{
    if l4var213z then
        form1Insn(insnTemp[XTA])
    else
        form1Insn(KXTA+E1)
}; (* P3363 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure  addInsnToBuf(insn: integer);
{
    insnBuf[insnBufIdx].i := insn;
    insnBufIdx := insnBufIdx + 1;
}; (* addInsnToBuf *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure add2InsnsToBuf(insn1, insn2: integer);
{
    insnBuf[insnBufIdx].i := insn1;
    insnBuf[insnBufIdx+1].i := insn2;
    insnBufIdx := insnBufIdx + 2;
}; (* add2InsnsToBuf *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function F3413: boolean;
{
    l4inl7z := l4inl6z;
    while l4inl7z <> NIL do {
        if (l4inl7z@.mode = curInsn.i) then {
            F3413 := true;
            while (l4inl7z@.code = macro) do {
                l4inl7z := ptr(l4inl7z@.offset);
            };
            exit
        } else {
            l4inl7z := l4inl7z@.next;
        }
    };
    F3413 := false;
}; (* F3413 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure addJumpInsn(opcode: integer);
{
    if not F3413 then {
        new(l4inl7z);
        l4inl7z@.next := l4inl6z;
        l4inl7z@.mode := curInsn.i;
        l4inl7z@.code := 0;
        l4inl7z@.offset := 0;
        l4inl6z := l4inl7z;
    };
    addInsnToBuf(macro + opcode + ord(l4inl7z))
}; (* addJumpInsn *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* genOneOp *)
    if insnList = NIL
        then exit;
    set145z := set145z + insnList@.regsused;
    l4oi212z := insnList@.next2;
    l4var9z := 370007B;
    insnBufIdx := 1;
    if l4oi212z = NIL then
        exit;
    l4inl6z := NIL;
    while l4oi212z <> NIL do {
        tempInsn.i := l4oi212z@.code;
        l4var4z := tempInsn.i -  macro;
        curInsn.i := l4oi212z@.offset;
        case l4oi212z@.mode of
         0: ;
         1: if arithMode <> 1 then {
                addInsnToBuf(370007B);
                arithMode := 1
            };
         2: arithMode := 1;
         3: if arithMode <> 2 then {
                addInsnToBuf(insnTemp[NTR]);
                arithMode := 2;
            };
         4: arithMode := 2;
        end; (* case *)
        l4oi212z := l4oi212z@.next;
        if l4var4z >= 0 then {
            case l4var4z of
            mcCARD: {
                add2InsnsToBuf(KACX, KAEX+ZERO);
            };
            21: goto 3556;
            0:  addJumpInsn(insnTemp[UZA]);
            1:  addJumpInsn(insnTemp[U1A]);
            2: {
                tempInsn.i := curInsn.i mod 4096;
                curInsn.i := curInsn.i div 4096;
                addJumpInsn(insnTemp[UJ]);
                curInsn.i := tempInsn.i;
3556:           if F3413 then
                    addInsnToBuf(2*macro+ord(l4inl7z))
                else
                    error(206);
            };
            3: {
                 tempInsn.i := curInsn.i mod 4096;
                 curInsn.i := curInsn.i div 4096;
                 l4var213z :=  F3413;
                 l4inl8z := l4inl7z;
                 curInsn.i := tempInsn.i;
                 l4var213z := l4var213z & F3413;
                 if l4var213z then
                    with l4inl7z@ do {
                        code := macro;
                        offset := ord(l4inl8z);
                    }
                else
                    error(207);
            };
            20: addInsnToBuf(3*macro + curInsn.i);
            4: {
                if insnBuf[insnBufIdx-1].m * [21:23, 28:35] = [] then
                    insnBuf[insnBufIdx-1].m := insnBuf[insnBufIdx-1].m + [35]
                else
                    addInsnToBuf(KXTA+SP)
            };
            5:
(blk)       {
                if l4oi212z <> NIL then {
                    tempInsn.i := l4oi212z@.code;
                    if tempInsn.m * [21:23, 28:35] = [32] then {
                        l4oi212z@.code :=
                            tempInsn.i - insnTemp[XTA] + insnTemp[XTS];
                        exit blk
                    }
                };
                addInsnToBuf(KATX+SP);
            };
            mcACC2ADDR:  add2InsnsToBuf(KATI+14, KUTC+I14);
            mcMULTI: {
                addInsnToBuf(getHelperProc(12));        (* P/MI *)
            };
            mcADDSTK2REG:  add2InsnsToBuf(KWTC+SP, KUTM+
                               indexreg[curInsn.i]);
            mcADDACC2REG:  add2InsnsToBuf(KATI+14, KJADDM+I14 + curInsn.i);
            mcODD: {
                add2InsnsToBuf(KAAX+E1, KAEX+ZERO);
            };
            mcROUND: {
                addInsnToBuf(KADD+REAL05);                (* round *)
                add2InsnsToBuf(KNTR+7, KADD+ZERO)
            };
            14: add2InsnsToBuf(indexreg[curInsn.i] + KVTM,
                               KITA + curInsn.i);
            mcMINEL: {
                add2InsnsToBuf(KANX+ZERO, KSUB+PLUS1);   (* minel *)
            };
            16: add2InsnsToBuf(insnTemp[XTA], KATX+SP + curInsn.i);
            17: {
                addInsnToBuf(KXTS);
                add2InsnsToBuf(KATX+SP+1, KUTM+SP + curInsn.i)
            };
            18: add2InsnsToBuf(KVTM+I10, getHelperProc(65)); (* P/B7 *)
            mcPOP2ADDR: {
                addInsnToBuf(KVTM+I14);
                add2InsnsToBuf(KXTA+SP, KATX+I14)
            };
            22: {
                add2InsnsToBuf(KVTM+I14, KXTA+I14);
                curVal.i := 40077777C;
                add2InsnsToBuf(allocSymtab(curVal.m) + (KXTS+SP),
                               KAAX+I8 + curInsn.i);
                add2InsnsToBuf(KAEX+SP, KATX+I14)
            };
            end; (* case *)
        } else {
            if 28 in tempInsn.m then {
                addInsnToBuf(getValueOrAllocSymtab(curInsn.i)+tempInsn.i);
            } else {
                curval.i := curInsn.i mod 32768;
                if curVal.i < 2048 then
                    addInsnToBuf(tempInsn.i + curInsn.i)
                else
(stmt)          if (curVal.i >= 28672) or (curVal.i < 4096) then {
                    addInsnToBuf(
                        allocSymtab((curVal.m + [24])*halfWord)
                        + tempInsn.i - 28672);
                } else {
                    add2InsnsToBuf(getValueOrAllocSymtab(curVal.i)
                                   + insnTemp[UTC], tempInsn.i);
                }
            }
        }
    };
    insnBufIdx := insnBufIdx-1;
    for l4var4z := insnBufIdx downto 1 do {
        curInsn := insnBuf[l4var4z];
        if (curInsn.i = insnTemp[NTR]) or
           (curInsn.i = 370007B)
        then {
            l4var3z := l4var4z - 1;
            l4var213z := false;
(loop)      if l4var3z < 1 then exit loop else {
                tempInsn.m := insnBuf[l4var3z].m * [28:32];
                if (tempInsn.i = CUTC) or (tempInsn.i = CWTC)
                then {
                    l4var3z := l4var3z-1;
                    goto loop;
                }
            };
(* one word shorter
(loop)      while l4var3z >= 1 do {
                tempInsn.m := insnBuf[l4var3z].m * [28:32];
                if (tempInsn.i # CUTC) and (tempInsn.i # CWTC)
                then
                    exit loop;
                l4var3z := l4var3z-1;
            };
*)
            l4var3z := l4var3z + 1;
            if (l4var3z <> l4var4z) then {
                for l4var2z := l4var4z-1 downto l4var3z do {
                    insnBuf[l4var2z+1] := insnBuf[l4var2z]
                };
            };
            insnBuf[l4var3z] := curInsn;
        };
    };
    for l4var4z to insnBufIdx do
(iter)  {
        curInsn := insnBuf[l4var4z];
        tempInsn.m := curInsn.m * [0, 1, 3, 23:32];
        if tempInsn.i = KATX+SP then {
            l4var2z := l4var4z + 1;
            while insnBufIdx + 1 <> l4var2z do {
                curVal.m := insnBuf[l4var2z].m * [0, 1, 3, 23, 28:35];
                tempInsn.m := curVal.m * [0, 1, 3, 23, 28:32];
                if curVal.i = insnTemp[XTA] then {
                    insnBuf[l4var2z].m :=
                        insnBuf[l4var2z].m mod [32, 34, 35];
                    exit iter;
                } else if curVal.i = insnTemp[ITA] then {
                    insnBuf[l4var2z].m := insnBuf[l4var2z].m + [35];
                    exit iter;
                } else if (curVal.i = insnTemp[NTR]) or
                    (tempInsn.i = insnTemp[UTC]) or
                    (tempInsn.i = insnTemp[WTC]) or
                    (tempInsn.i = insnTemp[VTM])
                then
                    l4var2z := l4var2z + 1
                else {
                    l4var2z := insnBufIdx + 1;
                }
            };
        };
        if curInsn.i = insnTemp[UTC] then
            exit iter;
        if curInsn.i < macro then {
            form1Insn(curInsn.i);
            tempInsn.m := curInsn.m * [28:32];
            if (tempInsn.i = 3100000C) or (* VJM *)
               (tempInsn.i = 0500000C)    (* ELFUN *)
            then {
                padToLeft;
                prevOpcode := 1;
            };
            exit iter;
        };
        if (curInsn.i >= 3*macro) then {
            curInsn.i := curInsn.i - (3*macro);
            if curInsn.i >= 4096 then {
                l4var213z := true;
                curInsn.i := curInsn.i - 4096;
            } else {
                l4var213z := false;
            };
            if (curInsn.i = 0) then
                form1Insn(insnTemp[UZA] + moduleOffset + 2);
            P3363;
            form1Insn(insnTemp[UJ] + 2 + moduleOffset);
            padToLeft;
            if (curInsn.i <> 0) then {
                if (not F3413) then
                    error(211);
                P0715(0, l4inl7z@.code);
            };
            l4var213z := not l4var213z;
            P3363;
            padToLeft;
            exit iter
        };
        if (curInsn.i >= 2*macro) then {
            l4inl7z := ptr(curInsn.i - (2*macro));
            P0715(0, l4inl7z@.code);
            l4inl7z@.offset := moduleOffset;
        } else {
            curInsn.i := curInsn.i - macro;
            curVal.m := curInsn.m * [0, 1, 3, 28:32];
            jumpType := curVal.i;
            curVal.m := [0, 1, 3, 33:47] * curInsn.m;
            l4inl7z := ptr(curVal.i);
            formJump(l4inl7z@.code);
            jumpType := insnTemp[UJ];
            exit iter
        }
    }; (* loop *)
    insnList := NIL;
    while (l4inl6z <> NIL) do {
        with l4inl6z@ do
            if (offset = 0) then {
                jumpTarget := code;
                exit;
            } else
                l4inl6z := next;
    };
    set146z := set146z - set145z;
}; (* genOneOp *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure addToInsnList(insn: integer);
var elt: oiptr;
{
    new(elt);
    with elt@ do {
        next := NIL;
        mode := 0;
        code := insn;
        offset := 0;
    };
    with insnList@ do {
        if next = NIL then
            next2 := elt
        else
            next@.next := elt;
        next := elt
    }
}; (* addToInsnList *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure addInsnAndOffset(insn, l4arg2z: integer);
{
    addToInsnList(insn);
    insnlist@.next@.offset := l4arg2z
}; (* addInsnAndOffset *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure addxToInsnList(insn: integer);
var
    elt: oiptr;
{
    new(elt);
    with elt@ do {
        next := insnList@.next2;
        mode := 0;
        code := insn;
        offset := 0;
    };
    if (insnList@.next2 = NIL) then {
        insnList@.next := elt;
    };
    insnList@.next2 := elt;
}; (* addxToInsnList *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure prepLoad;
label
    4545, 4602;
var
    helper, l4int2z, l4int3z: integer;
    l4typ4z: tptr;
    l4var5z: kind;
    l4st6z: state;
    l4bool7z, l4bool8z, l4bool9z: boolean;
{
    l4typ4z := insnList@.typ;
    with insnList@ do {
        case ilm of
        ilCONST: {
            curVal := ilf5;
            if (l4typ4z@.size = 1) then
                curVal.i := getFCSToffset;
            addToInsnList(constRegTemplate + curInsnTemplate + curVal.i);
        };
        il1: {
            helper := insnList@.ilf7;
            l4int2z := insnList@.ilf5.i;
            l4int3z := insnList@.ilf6;
            if (15 < helper) then {
                (* empty *)
            } else {
                if (helper = 15) then { (* P/CP *)
                    addToInsnList(macro + mcACC2ADDR);
                } else {
                    helper := indexreg[insnList@.ilf7];
                    if (l4int2z = 0) and (insnList@.st = st0) then {
                        addInsnAndOffset(helper + curInsnTemplate,
                                         l4int3z);
                        goto 4602;
                    } else {
                        addToInsnList(helper + insnTemp[UTC]);
                    }
                }
            };
            l4st6z := insnList@.st;
            if l4st6z = st0 then {
                addInsnAndOffset(l4int2z + curInsnTemplate, l4int3z);
            } else {
                l4var5z := l4typ4z@.k;
                if (l4var5z < kindArray) or
                   (l4var5z = kindStruct) and (s6 in optSflags.m) then {
                    l4bool7z := true;
                    l4bool8z := typeCheck(l4typ4z, integerType);
                } else {
                    l4bool7z := false;
                    l4bool8z := false;
                };
                if l4st6z = st1 then {
                    if (l4int3z <> l4int2z) or
                       (helper <> 18) or (* P/RC *)
                       (l4int2z <> 0) then
                        addInsnAndOffset(l4int2z + insnTemp[XTA],
                                         l4int3z);
                    l4int3z := insnList@.shift;
                    l4int2z := insnList@.width;
                    l4bool9z := true;
                    helper := l4int3z + l4int2z;
                    if l4bool7z then {
                        if (30 < l4int3z) then {
                            addToInsnList(ASN64-48 + l4int3z);
                            addToInsnList(insnTemp[YTA]);
                            if (helper = 48) then (* P/RDR *)
                                l4bool9z := false;
                        } else {
                            if (l4int3z <> 0) then
                                addToInsnList(ASN64 + l4int3z);
                        };
                        if l4bool9z then {
                            curVal.m := [(48 - l4int2z)..47];
                            addToInsnList(KAAX+I8 + getFCSToffset);
                        }
                    } else {
                        if (helper <> 48) then
                            addToInsnList(ASN64-48 + helper);
                        curVal.m := [0..(l4int2z-1)];
                        addToInsnList(KAAX+I8 + getFCSToffset);
                    };
                    if l4bool8z then
                        addToInsnList(KAEX+ZERO);
                } else {
                    if l4bool7z then
                        helper := ord(l4bool8z)+74 (* P/LDAR[IN] *)
                    else
                        helper := 56; (* P/RR *)
                    addToInsnList(getHelperProc(helper));
                    insnList@.next@.mode := 1;
                }
            };
            goto 4545;
        };
        il2: {
4545:       if forValue and (l4typ4z = booleanType) and
               (16 in insnList@.regsused) then
                addToInsnList(KAEX+E1);
        };
        il3: {
            if forValue then
                addInsnAndOffset(macro+20,
                    ord(16 in insnList@.regsused)*10000B + insnList@.ilf5.i);
        };
        end; (* case *)
4602:
    }; (* with *)
    with insnList@ do {
        ilm := il2;
        regsused := regsused + [0];
    };
}; (* prepLoad *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure P4606;
{
    prepLoad;
    addToInsnList(macro + mcPUSH)
}; (* P4606 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure setAddrTo(reg: integer);
label
    4650, 4654;
var
    l4var1z: word;
    l4int2z, opCode, l4var4z, l4var5z,
    l4var6z, regField: integer;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure P4613;
{
    l4var1z.i := insnList@.ilf6;
    l4var1z.i := l4var1z.i mod 32768;
    l4var6z := l4var1z.i
}; (* P4613 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* setAddrTo *)
    with insnList@ do {
        l4int2z := ilf7;
        opCode := insnTemp[VTM];
        regField := indexreg[reg];
        l4var4z := ilf5.i;
        regsused := regsused + [reg];
        if (ilm = ilCONST) then {
            curVal := ilf5;
            if (typ@.size = 1) then
                curVal.i := addCurValToFCST;
            l4var6z := curVal.i;
            l4var5z := 74001B;
            goto 4654;
        } else if (l4int2z = 18) then {
4650:       P4613;
            if (l4var4z = indexreg[1]) then {
                l4var5z := 74003B;
4654:           l4var1z.i := macro * l4var5z + l4var6z;
                l4var6z := allocSymtab(l4var1z.m * [12:47]);
                addToInsnList(regField + opCode + l4var6z);
            } else if (l4var4z <> 0) then {
                addInsnAndOffset(l4var4z + insnTemp[UTC], l4var6z);
                addToInsnList(regField + opCode);
            } else {
                addInsnAndOffset(regField + opCode, l4var6z);
            }
        } else if (l4int2z = 17) then {
            P4613;
            l4var4z := insnList@.ilf6;
            l4var5z := insnList@.next@.code - insnTemp[UTC];
            if (l4var4z <> 0) then {
                l4var1z.i := macro * l4var5z + l4var4z;
                l4var5z := allocSymtab(l4var1z.m * [12:47]);
            };
            insnList@.next@.code := regField + l4var5z + opCode;
        } else if (l4int2z = 16) then {
            P4613;
            if (l4var4z <> 0) then
                addToInsnList(l4var4z + insnTemp[UTC]);
            addInsnAndOffset(regField + opCode, l4var6z);
        } else if (l4int2z = 15) then {
            addToInsnList(insnTemp[ATI] + reg);
            opCode := insnTemp[UTM];
            goto 4650;
        } else {
            addToInsnList(indexreg[l4int2z] + insnTemp[UTC]);
            goto 4650;
        }
    }; (* with *)
    insnList@.ilm := il1;
    insnList@.ilf7 := reg;
    insnList@.ilf6 := 0;
    insnList@.ilf5.i := 0;
}; (* setAddrTo *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure prepStore;
var
    l4int1z: integer;
    l4int2z, l4int3z: integer;
    l4bool4z, l4bool5z: boolean;
    l4st6z: state;
    l4var7z: kind;
{
    with insnList@ do
        l4int1z := ilf7;
    if (15 < l4int1z) then {
        (* nothing? *)
    } else if (l4int1z = 15) then {
        addToInsnList(macro + mcACC2ADDR)
    } else {
        addToInsnList(indexreg[l4int1z] + insnTemp[UTC]);
    };
    l4bool4z := 0 in insnList@.regsused;
    l4st6z := insnList@.st;
    if (l4st6z <> st0) or l4bool4z then
        addxToInsnList(macro + mcPUSH);
    if (l4st6z = st0) then {
        if (l4bool4z) then {
            addInsnAndOffset(insnList@.ilf5.i + insnTemp[UTC],
                             insnList@.ilf6);
            addToInsnList(macro+mcPOP2ADDR);
        } else {
            addInsnAndOffset(insnList@.ilf5.i, insnList@.ilf6);
        }
    } else {
        l4var7z := insnList@.typ@.k;
        l4int1z := insnList@.typ@.bits;
        l4bool5z := (l4var7z < kindArray) or
                     (l4var7z = kindStruct) and (S6 in optSflags.m);
        if (l4st6z = st1) then {
            l4int2z := insnList@.shift;
            l4int3z := l4int2z + insnList@.width;
            if l4bool5z then {
                if (l4int2z <> 0) then
                    addxToInsnList(ASN64 - l4int2z);
            } else {
                if (l4int3z <> 48) then
                    addxToInsnList(ASN64 + 48 - l4int3z);
            };
            addInsnAndOffset(insnTemp[UTC] + insnList@.ilf5.i,
                             insnList@.ilf6);
            curVal.m := [0..47] - [(48-l4int3z)..(47 -l4int2z)];
            addInsnAndOffset(macro+22, getFCSToffset);
        } else {
            if not l4bool5z then {
                l4int2z := (insnList@.width - l4int1z);
                if (l4int2z <> 0) then
                    addxToInsnList(ASN64 - l4int2z);
                addxToInsnList(insnTemp[YTA]);
                addxToInsnList(ASN64 - l4int1z);
            };
            addToInsnList(getHelperProc(77)); (* "P/STAR" *)
            insnList@.next@.mode := 1;
        }
    }
}; (* prepStore *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure P5117(op: operator);
{
    addInsnAndOffset(curFrameRegTemplate, localSize);
    new(curExpr);
    with curExpr@ do
        typ := insnList@.typ;
    genOneOp;
    curExpr@.op := op;
    curExpr@.num1 := localSize;
    localSize := localSize + 1;
    if (l2int21z < localSize) then
        l2int21z := localSize;
}; (* P5117 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function insnCount: integer;
var
    cnt: integer;
    cur: oiptr;
{
    cnt := 0;
    cur := insnList@.next2;
    while (cur <> NIL) do {
        cur := cur@.next;
        cnt := cnt + 1;
    };
    insnCount := cnt;
}; (* insnCount *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genFullExpr(exprToGen: eptr);
label
    7567, 7760, 10075, 10122;
var
    arg1Const, arg2Const: boolean;
    otherIns: @insnltyp;
    arg1Val, arg2Val: word;
    curOP: operator;
    work: integer;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure P5155;
{
    prepLoad;
    insnList@.ilm := il1;
    insnList@.st := st0;
    insnList@.ilf6 := 0;
    insnList@.ilf5.i := 0;
    insnList@.ilf7 := 18;
}; (* P5155 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genDeref;
label
    5220;
var
    l5var1z, l5var2z: word;
    doPtrCheck: boolean;
{
    doPtrCheck := checkBounds and not (NoPtrCheck in optSflags.m)
               and (curOP = DEREF);
    if not doPtrCheck and (
        (insnList@.st = st0) or
        (insnList@.st = st1) and
        (insnList@.shift = 0))
    then {
        l5var1z.i := insnList@.ilf7;
        l5var2z.i := insnList@.ilf6;
        if (l5var1z.i = 18) or (l5var1z.i = 16) then {
5220:       addInsnAndOffset((insnList@.ilf5.i + insnTemp[WTC]), l5var2z.i);
        } else {
            if (l5var1z.i = 17) then {
                if (l5var2z.i = 0) then {
                    insnList@.next@.code := insnList@.next@.code +
                                                insnTemp[XTA];
                } else
                    goto 5220;
            } else if (l5var1z.i = 15) then {
                addToInsnList(macro + mcACC2ADDR);
                goto 5220;
            } else {
                addInsnAndOffset((indexreg[l5var1z.i] + insnTemp[WTC]),
                                 l5var2z.i);
            }
        }
    } else {
        P5155;
        if (doPtrCheck) then {
            addToInsnList(KVTM+I14 + lineCnt);
            addToInsnList(getHelperProc(7)); (* "P/CA"*)
            insnList@.next@.mode := 1;
        };
        addToInsnList(macro + mcACC2ADDR);
    };
    insnList@.ilf6 := 0;
    insnList@.ilf5.i := 0;
    insnList@.ilf7 := 16;
}; (* genDeref *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genHelper;
{
    P4606;
    saved := insnList;
    insnList := otherIns;
    prepLoad;
    addToInsnList(getHelperProc(nextInsn));
    insnList@.regsused := insnList@.regsused + saved@.regsused + [11:14];
    saved@.next@.next := insnList@.next2;
    insnList@.next2 := saved@.next2;
}; (* genHelper *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure prepMultiWord;
var
    l5var1z: boolean;
    l5var2z: @insnltyp;
{
    l5var1z := 12 in otherIns@.regsused;
    setAddrTo(12);
    if (l5var1z) then {
        addToInsnList(KITA+12);
        addToInsnList(macro + mcPUSH);
    };
    l5var2z := insnList;
    insnList := otherIns;
    setAddrTo(14);
    if (l5var1z) then {
        addToInsnList(macro + mcPOP);
        addToInsnList(KATI+12);
    };
    l5var2z@.regsused := insnList@.regsused + l5var2z@.regsused;
    l5var2z@.next@.next := insnList@.next2;
    l5var2z@.next := insnList@.next;
    insnList := l5var2z;
}; (* prepMultiWord *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
(*procedure genCheckBounds(l5arg1z: tptr);
var
    l5var1z: integer;
    l5var2z, l5var3z, l5var4z: word;
{
    l5var1z := l5arg1z@.checker;
    if (l5var1z = 0) then {
        curVal.i := l5arg1z@.left;
        l5var4z.i := l5arg1z@.right;
        if (l5arg1z@.base <> integerType) then {
            curVal.m := curVal.m * [7:47];
            l5var4z.m := l5var4z.m * [7:47];
        };
        prevOpcode := 0;
        formAndAlign(KUJ+5 + moduleOffset);
        l5arg1z@.checker := moduleOffset;
        l5var1z := moduleOffset;
        P0715(1, l5var4z.i);
        formAndAlign(KUJ+I13);
    };
    prepLoad;
    addToInsnList(KVTM+I14 + lineCnt);
    addToInsnList(KVJM+I13 + l5var1z);
    insnList@.next@.mode := 1;
}; (* genCheckBounds *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure negateCond;
{
    if (insnList@.ilm = ilCONST) then {
        insnList@.ilf5.b := not insnList@.ilf5.b;
    } else {
        insnList@.regsused := insnList@.regsused mod [16];
    }
}; (* negateCond *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure tryFlip(commutes: boolean);
label
    100, 22, 33;
var
    l5var1z: integer;
    l5var2z: @insnltyp;
{
    if not (0 in otherIns@.regsused) then {
        l5var1z := 0;
    } else if not (0 in insnList@.regsused) then {
        l5var1z := ord(commutes) + 1;
    } else {
        l5var1z := 3;
    };
    case l5var1z of
    0:
100: {
        prepLoad;
        saved := insnList;
        insnList := otherIns;
        curInsnTemplate := nextInsn;
        prepLoad;
        curInsnTemplate := insnTemp[XTA];
    };
    1:
        if (nextInsn = insnTemp[SUB]) then {
            nextInsn := insnTemp[RSUB];
            goto 22;
        } else
            goto 33;
   2:
22: {
        saved := insnList;
        insnList := otherIns;
        otherIns := saved;
        goto 100;
    };
    3:
33: {
        prepLoad;
        addToInsnList(indexreg[15] + nextInsn);
        l5var2z := insnList;
        insnList := otherIns;
        P4606;
        saved := insnList;
        insnList := l5var2z;
    };
    end; (* case *)
    insnList@.next@.mode := 0;
    saved@.next@.next := insnList@.next2;
    insnList@.next2 := saved@.next2;
    insnList@.regsused := insnList@.regsused + [0];
}; (* tryFlip *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genBoolAnd;
var
    l5var1z, l5var2z: boolean;
    l5var3z, l5var4z, l5var5z, l5var6z, l5var7z: integer;
    l5ins8z: @insnltyp;
    l5var9z: word;
{
    if (arg1Const) then {
        if (arg1Val.b) then
            insnList := otherIns;
    } else if (arg2Const) then {
        if (not arg2Val.b) then
            insnList := otherIns;
    } else {
        l5var1z := 16 in insnList@.regsused;
        l5var2z := 16 in otherIns@.regsused;
        l5var5z := int94z;
        int94z := int94z + 1;
        forValue := false;
        l5var6z := ord(l5var1z) + macro;
        l5var7z := ord(l5var2z) + macro;
        if (insnList@.ilm = il3) then {
            l5var3z := insnList@.ilf5.i;
        } else {
            l5var3z := 0;
            prepLoad;
        };
        if (otherIns@.ilm = il3) then {
            l5var4z := otherIns@.ilf5.i;
        } else {
            l5var4z := 0;
        };
        l5var9z.m := (insnList@.regsused + otherIns@.regsused);
        if (l5var3z = (0)) then {
            if (l5var4z = (0)) then {
                addInsnAndOffset(l5var6z, l5var5z);
                l5ins8z := insnList;
                insnList := otherIns;
                prepLoad;
                addInsnAndOffset(l5var7z, l5var5z);
            } else {
                if (l5var2z) then {
                    addInsnAndOffset(l5var6z, l5var5z);
                    l5ins8z := insnList;
                    insnList := otherIns;
                    addInsnAndOffset(macro + 2,
                                     10000B * l5var5z + l5var4z);
                } else {
                    addInsnAndOffset(l5var6z, l5var4z);
                    l5var5z := l5var4z;
                    l5ins8z := insnList;
                    insnList := otherIns;
                }
            };
        } else {
            if (l5var4z = (0)) then {
                if (l5var1z) then {
                    addInsnAndOffset(macro + 2,
                                     10000B * l5var5z + l5var3z);
                    l5ins8z := insnList;
                    insnList := otherIns;
                    prepLoad;
                    addInsnAndOffset(l5var7z, l5var5z);
                } else {
                    l5ins8z := insnList;
                    insnList := otherIns;
                    prepLoad;
                    addInsnAndOffset(l5var7z, l5var3z);
                    l5var5z := l5var3z;
                };
            } else {
                if (l5var1z) then {
                    if (l5var2z) then {
                        addInsnAndOffset(macro + 2,
                                         10000B * l5var5z + l5var3z);
                        l5ins8z := insnList;
                        insnList := otherIns;
                        addInsnAndOffset(macro + 2,
                                         10000B * l5var5z + l5var4z);
                    } else {
                        addInsnAndOffset(macro + 2,
                                         10000B * l5var4z + l5var3z);
                        l5ins8z := insnList;
                        insnList := otherIns;
                        l5var5z := l5var4z;
                    }
                } else {
                    l5ins8z := insnList;
                    insnList := otherIns;
                    l5var5z := l5var3z;
                    if (l5var2z) then
                        addInsnAndOffset(macro + 2,
                                         10000B * l5var3z + l5var4z)
                    else
                        addInsnAndOffset(macro + 3,
                                         10000B * l5var3z + l5var4z);
                }
            }
        };
        insnList@.regsused := l5var9z.m - [16];
        l5ins8z@.next@.next := insnList@.next2;
        insnList@.next2 := l5ins8z@.next2;
        insnList@.ilm := il3;
        insnList@.ilf5.i := l5var5z;
        forValue := true;
    }
}; (* genBoolAnd *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genGetElt;
var
    l5var1z, dimCnt, curDim, l5var4z, l5var5z, l5var6z,
        l5var7z, l5var8z: integer;
    insnCopy: insnltyp;
    copyPtr, l5ins21z: @insnltyp;
    l5var22z, l5var23z: word;
    l5var24z: boolean;
    l5var25z: boolean;
    l5var26z, l5var27z: tptr;
    l5ilm28z: ilmode;
    l5var29z: eptr;
    getEltInsns: array [1..10] of @insnltyp;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function myminel(l6arg1z: bitset): integer;
{
    myminel := minel(l6arg1z);
}; (* myminel *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* genGetElt *)
    dimCnt := 0;
    l5var29z := exprToGen;
    while (l5var29z@.op = GETELT) do {
        genFullExpr(l5var29z@.expr2);
        dimCnt := dimCnt + 1;
        getEltInsns[dimCnt] := insnList;
        l5var29z := l5var29z@.expr1;
    };
    genFullExpr(l5var29z);
    l5ins21z := insnList;
    insnCopy := insnList@;
    copyPtr := ref(insnCopy);
    l5var22z.m := set147z;
    for curDim to dimCnt do
       l5var22z.m := l5var22z.m - getEltInsns[curDim]@.regsused;
    for curDim := dimCnt downto 1 do {
        l5var26z := insnCopy.typ@.base;
        l5var27z := insnCopy.typ@.range;
        l5var25z := insnCopy.typ@.pck;
        l5var7z := l5var27z@.left;
        l5var8z := l5var26z@.size;
        if not l5var25z then
            insnCopy.ilf6 := insnCopy.ilf6 - l5var8z * l5var7z;
        insnList := getEltInsns[curDim];
        l5ilm28z := insnList@.ilm;
        if (l5ilm28z = ilCONST) then {
            curVal := insnList@.ilf5;
            curVal.m := curVal.m +  intZero;
            if (curVal.i < l5var7z) or
               (l5var27z@.right < curVal.i) then
                error(29); (* errIndexOutOfBounds *)
            if (l5var25z) then {
                l5var4z := curVal.i - l5var7z;
                l5var5z := insnCopy.typ@.perword;
                insnCopy.regsused := insnCopy.regsused + [0];
                insnCopy.ilf6 := l5var4z DIV l5var5z + insnCopy.ilf6;
                l5var6z := (l5var5z-1-l5var4z MOD l5var5z) *
                           insnCopy.typ@.pcksize;
                case insnCopy.st of
                st0: insnCopy.shift := l5var6z;
                st1: insnCopy.shift := insnCopy.shift + l5var6z +
                                           insnCopy.typ@.bits - 48;
                st2: error(errUsingVarAfterIndexingPackedArray);
                end; (* case *)
                insnCopy.width := insnCopy.typ@.pcksize;
                insnCopy.st := st1;
            }  else {
                insnCopy.ilf6 := curVal.i  * l5var26z@.size +
                                  insnCopy.ilf6;
            }
        } else { (* 6123*)
(*            if (checkBounds) then {
                l5var24z := typeCheck(l5var27z, insnList@.typ);
                if (rangeMismatch) then
                    genCheckBounds(l5var27z);
            };
*)
            if (l5var8z <> 1) then {
                prepLoad;
                if (l5var27z@.base = integerType) then {
                    l5var4z := KYTA+64;
                } else {
                    l5var4z := KYTA+64-40;
                };
                addToInsnList(insnCopy.typ@.perword);
                insnList@.next@.mode := 1;
                if (l5var7z >= 0) then
                    addToInsnList(l5var4z)
                else
                    addToInsnList(macro + mcMULTI);
           };
           if (l5ilm28z = il3) or
              (l5ilm28z = il1) and
              (insnList@.st <> st0) then
               prepLoad;
           l5var23z.m := insnCopy.regsused + insnList@.regsused;
           if (not l5var25z) then {
               if (insnCopy.ilf7 = 18) then {
                    if (insnList@.ilm = il2) then {
                        insnCopy.ilf7 := 15;
                    } else {
                        insnCopy.ilf7 := 16;
                        curInsnTemplate := insnTemp[WTC];
                        prepLoad;
                        curInsnTemplate := insnTemp[XTA];
                    };
                    insnCopy.next := insnList@.next;
                    insnCopy.next2 := insnList@.next2;
                } else {
                    if (insnCopy.ilf7 >= 15) then {
                        l5var1z :=  myminel(l5var22z.m);
                        if (0 >= l5var1z) then {
                            l5var1z := myminel(set147z - insnCopy.regsused);
                            if (0 >= l5var1z) then
                                l5var1z := 9;
                        };
                        saved := insnList;
                        insnList := copyPtr;
                        l5var23z.m := l5var23z.m + [l5var1z];
                        if (insnCopy.ilf7 = 15) then {
                            addToInsnList(insnTemp[ATI] + l5var1z);
                        } else {
                            addToInsnList(indexreg[l5var1z] + insnTemp[VTM]);
                        };
                        insnCopy.ilf7 := l5var1z;
                        insnCopy.regsused := insnCopy.regsused + [l5var1z];
                        insnList := saved;
                    } else {
                            l5var1z := insnCopy.ilf7;
                    };
                    if (l5var1z IN insnList@.regsused) then {
                         P4606;
                         insnList@.next@.next := insnCopy.next2;
                         insnCopy.next2 := insnList@.next2;
                         insnList := copyPtr;
                         addInsnAndOffset(macro+mcADDSTK2REG, l5var1z);
                    } else {
                         if (insnList@.ilm = il2) then {
                             addInsnAndOffset(macro+mcADDACC2REG, l5var1z);
                         } else {
                             curInsnTemplate := insnTemp[WTC];
                             prepLoad;
                             curInsnTemplate := insnTemp[XTA];
                             addToInsnList(indexreg[l5var1z] + insnTemp[UTM]);
                         };
                         insnCopy.next@.next := insnList@.next2;
                         insnCopy.next := insnList@.next;
                     }
                };
           } else {
                if (insnCopy.st = st0) then {
                    prepLoad;
                    if (l5var7z <> 0) then {
                        curVal.i := 0 - l5var7z;
                        if (not typeCheck(insnList@.typ, integerType)) then
                            curVal.m := curVal.m - intZero;
                        addToInsnList(KADD+I8 + getFCSToffset);
                        insnList@.next@.mode := 1;
                    };
                    l5var24z := 0 in insnCopy.regsused;
                    if (l5var24z) then
                        addToInsnList(macro + mcPUSH);
                    saved := insnList;
                    insnList := copyPtr;
                    setAddrTo(14);
                    if (l5var24z) then
                        addToInsnList(macro + mcPOP);
                    l5var23z.m := l5var23z.m + [0, 10, 11, 13, 14];
                    insnCopy.st := st2;
                    insnCopy.ilf6 := 0;
                    insnCopy.ilf5.i := 0;
                    insnCopy.width := insnCopy.typ@.pcksize;
                    curVal.i := insnCopy.width;
                    if (curVal.i = 24) then
                        curVal.i := 7;
                    curVal := curVal;besm(ASN64-24);curVal:=;
                    addToInsnList(allocSymtab(  (* P/00C *)
                        helperNames[76] + curVal.m)+(KVTM+I11));
                    insnCopy.ilf7 := 16;
                    insnCopy.shift := 0;
                    saved@.next@.next := insnCopy.next2;
                    insnCopy.next2 := saved@.next2;
                } else {
                    error(errUsingVarAfterIndexingPackedArray);
                }
            };
            insnCopy.regsused := l5var23z.m;
        };
        insnCopy.typ := l5var26z;
    };
    insnList := l5ins21z;
    insnList@ := insnCopy;
}; (* genGetElt *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genEntry;
var
    l5exp1z, l5exp2z: eptr;
    l5idr3z, l5idr4z, l5idr5z, l5idr6z: irptr;
    l5bool7z, l5bool8z, l5bool9z, l5bool10z, l5bool11z: boolean;
    l5var12z, l5var13z, l5var14z: word;
    l5var15z: integer;
    l5var16z, l5var17z, l5var18z, l5var19z: word;
    l5inl20z: @insnltyp;
    l5op21z: operator; l5idc22z: idclass;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function allocGlobalObject(l6arg1z: irptr): integer;
{
    if (l6arg1z@.pos = 0) then {
        if (l6arg1z@.flags * [20, 21] <> []) then {
            curVal := l6arg1z@.id;
            curVal.m := makeNameWithStars;
            l6arg1z@.pos := allocExtSymbol(extSymMask);
        } else {
            l6arg1z@.pos := symTabPos;
            putToSymTab([]);
        }
    };
    allocGlobalObject := l6arg1z@.pos;
}; (* allocGlobalObject *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure traceEntry(isEntry: boolean);
{
    if not (debugEntry in optSflags.m) then
        exit;
    curVal := l5idr5z@.id;
    addToInsnList(KVTM+I10 + addCurValToFCST);
    if (isEntry) then
        addToInsnList(KVTM+I11 + lineCnt);
    addToInsnList(getHelperProc(ord(isEntry) * 22 + 57)); (* P/C(E|I) *)
}; (* traceEntry *)
%
{ (* genEntry *)
    l5exp1z := exprToGen@.expr1;
    l5idr5z := exprToGen@.id2;
    l5bool7z := (l5idr5z@.typ = NIL);
    l5bool9z := (l5idr5z@.list = NIL);
    if (l5bool7z) then
        l5var13z.i := 3 else l5var13z.i := 4;
    l5var12z.m := l5idr5z@.flags;
    l5bool10z := (21 in l5var12z.m);
    l5bool11z := (24 in l5var12z.m);
    if (l5bool9z) then {
        l5var14z.i := F3307(l5idr5z);
        l5idr6z := l5idr5z@.argList;
    } else {
        l5var13z.i := l5var13z.i + 2;
    };
    new(insnList);
    insnList@.next2 := NIL;
    insnList@.next := NIL;
    insnList@.typ := l5idr5z@.typ;
    insnList@.regsused := (l5idr5z@.flags + [7:15]) * [0:8, 10:15];
    insnList@.ilm := il2;
    if (l5bool10z) then {
        l5bool8z := not l5bool7z;
        if (checkFortran) then {
            addToInsnList(getHelperProc(92)); (* "P/MF" *)
        }
    } else {
        l5bool8z := true;
        if (not l5bool9z) and (l5exp1z <> NIL)
            or (l5bool9z) and (l5var14z.i >= 2) then {
            addToInsnList(KUTM+SP + l5var13z.i);
        };
    };
    l5var14z.i := 0;
(loop)
    while l5exp1z <> NIL do {
        l5exp2z := l5exp1z@.expr2;
        l5exp1z := l5exp1z@.expr1;
        l5op21z := l5exp2z@.op;
        l5var14z.i := l5var14z.i + 1;
        l5inl20z := insnList;
        if (l5op21z = PCALL) or (l5op21z = FCALL) then {
            l5idr4z := l5exp2z@.id2;
            new(insnList);
            insnList@.next2 := NIL;
            insnList@.next := NIL;
            insnList@.regsused := [];
            set145z := set145z + l5idr4z@.flags;
            if (l5idr4z@.list <> NIL) then {
                addToInsnList(l5idr4z@.offset + insnTemp[XTA] +
                              l5idr4z@.value);
                if (l5bool10z) then
                    addToInsnList(getHelperProc(19)); (* "P/EA" *)
            } else
(a)         {
                if (l5idr4z@.value = 0) then {
                    if (l5bool10z) and (21 in l5idr4z@.flags) then {
                        addToInsnList(allocGlobalObject(l5idr4z) +
                                      (KVTM+I14));
                        addToInsnList(KITA+14);
                        exit a;
                    } else {
                        l5var16z.i := 0;
                        formJump(l5var16z.i);
                        padToLeft;
                        l5idr4z@.value := moduleOffset;
                        l5idr3z := l5idr4z@.argList;
                        l5var15z := ord(l5idr4z@.typ <> NIL);
                        l5var17z.i := F3307(l5idr4z);
                        form3Insn(KVTM+I10+ 4+moduleOffset,
                                  KVTM+I9 + l5var15z,
                                  KVTM+I8 + 74001B);
                        formAndAlign(getHelperProc(62)); (* "P/BP" *)
                        l5var15z := l5var17z.i + 2 + l5var15z;
                        form1Insn(KXTA+SP + l5var15z);
                        if ((1) < l5var17z.i) then
                            form1Insn(KUTM+SP + l5var15z)
                        else
                            form1Insn(0);
                        form2Insn(
                            getHelperProc(63(*P/B6*)) + 6437777777300000C,
                            allocGlobalObject(l5idr4z) + KUJ);
                        if (l5idr3z <> NIL) then {
                            repeat
                                l5idc22z := l5idr3z@.cl;
                                if (l5idc22z = ROUTINEID) and
                                   (l5idr3z@.typ <> NIL) then
                                    l5idc22z := ENUMID;
                                form2Insn(0, ord(l5idc22z));
                                l5idr3z := l5idr3z@.list;
                            until (l5idr4z = l5idr3z);
                        };
                        storeObjWord([]);
                        P0715(0, l5var16z.i);
                    }
                };
                addToInsnList(KVTM+I14 + l5idr4z@.value);
                if 21 in l5idr4z@.flags then
                    addToInsnList(KITA+14)
                else
                    addToInsnList(getHelperProc(64)); (* "P/PB" *)
            };
            if (l5op21z = PCALL) then
                l5idc22z := ROUTINEID
            else
                l5idc22z := ENUMID;
        } else {
            genFullExpr(l5exp2z);
            if (insnList@.ilm = il1) then
                l5idc22z := FORMALID
            else
                l5idc22z := VARID;
        };
        if not (not l5bool9z or (l5idc22z <> FORMALID) or
               (l5idr6z@.cl <> VARID)) then
            l5idc22z := VARID;
(loop)      if (l5idc22z = FORMALID) or (l5bool11z) then {
            setAddrTo(14);
            addToInsnList(KITA+14);
        } else if (l5idc22z = VARID) then {
            if (insnList@.typ@.size <> 1) then {
                l5idc22z := FORMALID;
                goto loop;
            } else {
                prepLoad;
            }
        };
        if not l5bool8z then
            addxToInsnList(macro + mcPUSH);
        l5bool8z := false;
        if (l5inl20z@.next <> NIL) then {
            l5inl20z@.next@.next := insnList@.next2;
            insnList@.next2 := l5inl20z@.next2;
        };
        insnList@.regsused := insnList@.regsused + l5inl20z@.regsused;
        if not l5bool9z then {
            curVal.cl := l5idc22z;
            addToInsnList(KXTS+I8 + getFCSToffset);
        };
        if l5bool9z and not l5bool11z then
            l5idr6z := l5idr6z@.list;
    }; (* while -> 7061 *)
    traceEntry(true);
    if l5bool10z then {
        addToInsnList(KNTR+2);
        insnList@.next@.mode := 4;
    };
    if l5bool9z then {
        addToInsnList(allocGlobalObject(l5idr5z) + (KVJM+I13));
        if (20 in l5idr5z@.flags) then {
            l5var17z.i := 1;
        } else {
            l5var17z.i := l5idr5z@.offset div 4000000B;
        }
    } else {
        l5var15z := 0;
        if (l5var14z.i = 0) then {
            l5var17z.i := l5var13z.i + 1;
        } else {
            l5var17z.i := -(2 * l5var14z.i + l5var13z.i);
            l5var15z := 1;
        };
        addInsnAndOffset(macro+16 + l5var15z,
                         getValueOrAllocSymtab(l5var17z.i));
        addToInsnList(l5idr5z@.offset + insnTemp[UTC] + l5idr5z@.value);
        addToInsnList(macro+18);
        l5var17z.i := 1;
    };
    insnList@.next@.mode := 2;
    if (curProcNesting <> l5var17z.i) then {
        if not l5bool10z then {
            if (l5var17z.i + 1 = curProcNesting) then {
                addToInsnList(KMTJ+I7 + curProcNesting);
            } else {
                l5var15z := frameRestore[curProcNesting][l5var17z.i];
                if (l5var15z = (0)) then {
                    curVal.i := 6017T; (* P/ *)
                    l5var19z.i := curProcNesting + 16;
                    besm(ASN64-30);
                    l5var19z := ;
                    l5var18z.i := l5var17z.i + 16;
                    besm(ASN64-24);
                    l5var18z := ;
                    curVal.m := curVal.m + l5var19z.m + l5var18z.m;
                    l5var15z := allocExtSymbol(extSymMask);
                    frameRestore[curProcNesting][l5var17z.i] := l5var15z;
                };
                addToInsnList(KVJM+I13 + l5var15z);
            }
        }
    };
    if not l5bool9z or ([20, 21] * l5var12z.m <> []) then {
        addToInsnList(KVTM+40074001B);
    };
    set145z := (set145z + l5var12z.m) * [1:15];
    traceEntry(false);
    if l5bool10z then {
        if (not checkFortran) then
            addToInsnList(KNTR+7)
        else
            addToInsnList(getHelperProc(93));    (* "P/FM" *)
        insnList@.next@.mode := 2;
    } else {
        if not l5bool7z then
            addToInsnList(KXTA+SP + l5var13z.i - 1);
    };
    if not l5bool7z then {
        insnList@.typ := l5idr5z@.typ;
        insnList@.regsused := insnList@.regsused + [0];
        insnList@.ilm := il2;
        set146z := set146z - l5var12z.m;
    }

}; (* genEntry *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure startInsnList(l5arg1z: ilmode);
{
    new(insnList);
    insnList@.next := NIL;
    insnList@.next2 := NIL;
    insnList@.typ := exprToGen@.typ;
    insnList@.regsused := [];
    insnList@.ilm := l5arg1z;
    if (l5arg1z = ilCONST) then {
        insnList@.ilf5.i := exprToGen@.num1;
        insnList@.ilf7 := exprToGen@.num2;
    } else {
        insnList@.st := st0;
        insnList@.ilf7 := 18;
        insnList@.ilf5.i := curFrameRegTemplate;
        insnList@.ilf6 := exprToGen@.num1;
    }
}; (* startInsnList *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genCopy;
var
    size: integer;
{
    size := insnList@.typ@.size;
    if (size = 1) then {
        saved := insnList;
        insnList := otherIns;
        prepLoad;
        genOneOp;
        insnList := saved;
        prepStore;
        genOneOp;
    } else {
        prepMultiWord;
        genOneOp;
        size := size - 1;
        formAndAlign(KVTM+I13 + getValueOrAllocSymtab(-size));
        work := moduleOffset;
        form2Insn(KUTC+I14 + size, KXTA+I13);
        form3Insn(KUTC+I12 + size, KATX+I13,
                  KVLM+I13 + work);
        set145z := set145z + [12:14];
    }
}; (* genCopy *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genConstDiv;
    function PASDIV(r: real): word;
        external;
{
    curVal := PASDIV(1.0/arg2Val.i);
    addToInsnList(KMUL+I8 + getFCSToffset);
}; (* genConstDiv *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genComparison;
label
    7475, 7504, 7514, 7530;
var
    negate: boolean;
    l5set2z: bitset;
    mode, size: integer;
{
    l3int3z := ord(curOP) - ord(NEOP);
    negate := odd(l3int3z);
    if (l3int3z = 6) then {     (* IN *)
        if (arg1Const) then {
            if (arg2Const) then {
                insnList@.ilf5.b := (arg1Val.i IN arg2Val.m);
            } else {
                l5set2z := [arg1Val.i];
                if (l5set2z = []) then {
                    insnList@.ilf5.b := false;
                } else {
                    insnList := otherIns;
                    prepLoad;
                    curVal.m := l5set2z;
                    addToInsnList(KAAX+I8 + getFCSToffset);
                    insnList@.ilf5.i := 0;
                    insnList@.ilm := il3;
                }
            };
        } else {
            saved := insnList;
            insnList := otherIns;
            otherIns := saved;
            nextInsn := 66;      (* P/IN *)
            genHelper;
            insnList@.ilm := il2;
        }
    } else {
        if negate then
            l3int3z := l3int3z - 1;
        l2typ13z := insnList@.typ;
        curVarKind := l2typ13z@.k;
        size := l2typ13z@.size;
        if (l2typ13z = realType) then {
            if (fuzzReals) then
                work := 0
            else
                work := 1;
        } else if (curVarKind IN [kindScalar, kindRange]) then
            work := 3
        else {
            work := 4;
        };
        if (size <> 1) then {
            prepMultiWord;
            addInsnAndOffset(KVTM+I11, 1 - size);
            addToInsnList(getHelperProc(89 + l3int3z)); (* P/EQ *)
            insnList@.ilm := il2;
            negate := not negate;
        } else  if l3int3z = 0 then {
            if work = 0 then {
                nextInsn := 15;         (* P/CP *)
7475:           genHelper;
                insnList@.ilm := il2;
            } else {
                nextInsn := insnTemp[AEX];
                tryFlip(true);
7504:           insnList@.ilm := il3;
                insnList@.ilf5.i := 0;
            };
        } else {
            case work of
            0: {
                nextInsn := 16;         (* P/AB *)
                goto 7475;
            };
            1: {
                mode := 3;
7514:           nextInsn := insnTemp[SUB];
                tryFlip(false);
                insnList@.next@.mode := mode;
                if mode = 3 then {
                    addToInsnList(KNTR+23B);
                    insnList@.next@.mode := 2;
                };
                goto 7504;
            };
            2: { (* work = 2 unused *)
                nextInsn := insnTemp[AAX];
7530:           prepLoad;
                addToInsnList(KAEX+ALLONES);
                tryFlip(true);
                goto 7504;
            };
            3: {
                mode := 1;
                goto 7514;
            };
            4: {
                nextInsn := insnTemp[ARX];
                goto 7530;
            };
            end; (* case *)
        };
        insnList@.regsused := insnList@.regsused - [16];
        if (negate)
            then negateCond;
    };

}; (* genComparison *)
function shift(val:bitset; amt:integer):bitset;
var i    : integer; ret: bitset;
{
    ret   := [];
    for i := 0 to 47 do if (i-amt) in val then ret := ret + [i];
    shift := ret;
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* genFullExpr *);
    if exprToGen = NIL then
        exit;
7567:
    curOP := exprToGen@.op;
    if (curOP < GETELT) then {
        genFullExpr(exprToGen@.expr2);
        otherIns := insnList;
        if (curOP = ASSIGNOP) then
            l3bool13z := false;
        genFullExpr(exprToGen@.expr1);
        if (curOP = ASSIGNOP) then
            l3bool13z := true;
        if (insnList@.ilm = ilCONST) then {
            arg1Const := true;
            arg1Val := insnList@.ilf5;
        } else
            arg1Const := false;
        if (otherIns@.ilm = ilCONST) then {
            arg2Const := true;
            arg2Val := otherIns@.ilf5;
        } else
            arg2Const := false;
        if (curOP IN [NEOP, EQOP, LTOP, GEOP, GTOP, LEOP, INOP]) then {
            genComparison;
        } else {
            if arg1Const and arg2Const then {
                case curOP of
                MUL:        arg1Val.r := arg1Val.r * arg2Val.r;
                RDIVOP:     arg1Val.r := arg1Val.r / arg2Val.r;
                AMPERS:     arg1Val.b := arg1Val.b and arg2Val.b;
                IDIVOP:     arg1Val.i := arg1Val.i DIV arg2Val.i;
                IMODOP:     arg1Val.i := arg1Val.i MOD arg2Val.i;
                PLUSOP:     arg1Val.r := arg1Val.r + arg2Val.r;
                MINUSOP:    arg1Val.r := arg1Val.r - arg2Val.r;
                OROP:       arg1Val.b := arg1Val.b or arg2Val.b:
                IMULOP:     arg1Val.i := arg1Val.i * arg2Val.i;
                SETAND:     arg1Val.m := arg1Val.m * arg2Val.m;
                SETXOR:     arg1Val.m := arg1Val.m MOD arg2Val.m;
                INTPLUS:    arg1Val.i := arg1Val.i + arg2Val.i;
                INTMINUS:   arg1Val.i := arg1Val.i - arg2Val.i;
                SETOR:      arg1Val.m := arg1Val.m + arg2Val.m;
                SHLEFT:     arg1Val.m := shift(arg1Val.m, -arg2Val.i);
                SHRIGHT:    arg1Val.m := shift(arg1Val.m, arg2Val.i);
                SETSUB:
                    goto 10075;
                NEOP, EQOP, LTOP, GEOP, GTOP, LEOP, INOP,
                MKRANGE, ASSIGNOP:
                    error(200);
                end;
                insnList@.ilf5 := arg1Val;
            } else {
                l3int3z := opToMode[curOP];
                flags := opFlags[curOP];
                nextInsn := opToInsn[curOP];
                case flags of
                opfCOMM:
7760:               tryFlip(curOP in [MUL, PLUSOP, SETAND, INTPLUS]);
                opfHELP:
                    genHelper;
                opfASSN: {
                    genCopy;
                    exit
                };
                opfAND: {
                    genBoolAnd;
                    exit
                };
                opfOR: {
                    negateCond;
                    saved := insnList;
                    insnList := otherIns;
                    negateCond;
                    otherIns := insnList;
                    insnList := saved;
                    genBoolAnd;
                    negateCond;
                    exit
                };
                opfMOD:
                    if (arg2Const) then {
                        prepLoad;
                        if card(arg2Val.m) = 4 then {
                            curVal.m := [0,1,3,minel(arg2Val.m-intZero)+1..47];
                            addToInsnList(KAAX+I8 +getFCSToffset);
                            l3int3z := 0;
                        } else {
                            addToInsnList(macro + mcPUSH);
                            genConstDiv;
                            insnList@.next@.mode := 1;
                            curVal.m := arg2Val.m - [1, 3];
                            addToInsnList(KMUL+I8 + getFCSToffset);
                            addToInsnList(KYTA+64);
                            addToInsnList(KRSUB+SP);
                            l3int3z := 1;
                        }
                    } else {
                        genHelper;
                    };
                opfDIV: {
                    if arg2Const then {
                        prepLoad;
                        genConstDiv;
                        l3int3z := 1;
                    } else
                        genHelper;
                };
                opfMULMSK: {
                    if (arg1Const) then {
                        insnList@.ilf5.m := arg1Val.m MOD [1, 3];
                    } else {
                        if (arg2Const) then {
                            otherIns@.ilf5.m := arg2Val.m MOD [1, 3];
                        } else {
                            prepLoad;
                            addToInsnList(KAEX+MULTMASK);
                        }
                    };
                    tryFlip(true);
                    insnList@.next@.mode := 1;
                    if (fixMult) then
                        addToInsnList(macro + mcMULTI)
                    else
                        addToInsnList(KYTA+64);
                };
                opfINV: {
10075:              saved := insnList;
                    insnList := otherIns;
                    otherIns := saved;
                    prepLoad;
                    addToInsnList(KAEX+ALLONES);
                    goto 7760
                };
                opfSHIFT: {
                    if (not arg2Const) then genHelper
                    else {
                        prepLoad;
                        if (curOP = SHRIGHT) then
                            addToInsnList(ASN64+arg2Val.i)
                         else
                            addToInsnList(ASN64-arg2Val.i)
                    }
                }
                end; (* case 10122 *)
10122:          insnList@.next@.mode := l3int3z;
            }
        }
    } else {
        if (FILEPTR >= curOP) then {
            if (curOP = GETVAR) then {
                new(insnList);
                curIdRec := exprToGen@.id1;
                with insnList@ do {
                    next := NIL;
                    next2 := NIL;
                    regsused := [];
                    ilm := il1;
                    ilf5.i := curIdRec@.offset;
                    ilf6 := curIdRec@.high.i;
                    st := st0;
                    ilf7 := 18;
                };
                if (curIdRec@.cl = FORMALID) then {
                    genDeref;
                } else if (curIdRec@.cl = ROUTINEID) then {
                    insnList@.ilf6 := 3;
                    insnList@.ilf5.i := (insnList@.ilf5.i + frameRegTemplate);
                } else if (insnList@.ilf6 >= 74000B) then {
                    addToInsnList(insnTemp[UTC] + insnList@.ilf6);
                    insnList@.ilf6 := 0;
                    insnList@.ilf7 := 17;
                    insnList@.ilf5.i := 0;
                }
            } else
            if (curOP = GETFIELD) then {
                genFullExpr(exprToGen@.expr1);
                curIdRec := exprToGen@.id2;
                with insnList@ do {
                    ilf6 := ilf6 + curIdRec@.offset;
                    if (curIdRec@.pckfield) then {
                        case st of
                        st0:
                            shift := curIdRec@.shift;
                        st1: {
                            shift := shift + curIdRec@.shift;
                            if not (S6 IN optSflags.m) then
                                shift := shift +
                                           curIdRec@.uptype@.bits - 48;
                        };
                        st2:
                            if (not l3bool13z) then
                                error(errUsingVarAfterIndexingPackedArray)
                            else {
                                P5155;
                                insnList@.shift := curIdRec@.shift;
                            }
                        end; (* 10235*)
                        insnList@.width := curIdRec@.width;
                        insnList@.st := st1;
                        insnList@.regsused := insnList@.regsused + [0];
                    }
                };
            } else
            if (curOP = GETELT) then
                genGetElt
            else
            if (curOP = DEREF) or (curOP = FILEPTR) then {
                genFullExpr(exprToGen@.expr1);
                genDeref;
            } else
            if (curOP = op36) then {
                startInsnList(il1);
            } else
            if (curOP = op37) then {
                startInsnList(il1);
                genDeref;
            } else
            if (curOP = GETENUM) then
                startInsnList(ilCONST)
        } else
        if (curOP = ALNUM) then
            genEntry
        else if (curOP IN [BOUNDS..RNEGOP]) then {
            genFullExpr(exprToGen@.expr1);
            if (insnList@.ilm = ilCONST) then {
                arg1Val := insnList@.ilf5;
                case curOP of
                BOUNDS: {
                    arg2Val.m := [0,1,3] + arg1Val.m;
                    with exprToGen@.typ2@ do {
                        if (arg2Val.i < left) or
                           (right < arg2Val.i) then
                            error(errNeedOtherTypesOfOperands)
                    }
                };
                TOREAL: arg1Val.r := arg1Val.i;
                NOTOP:  arg1Val.b := not arg1Val.b;
                RNEGOP: arg1Val.r := -arg1Val.r;
                INEGOP: arg1Val.i := -arg1Val.i;
                end; (* case 10345 *)
                insnList@.ilf5 := arg1Val;
            } else
            if (curOP = NOTOP) then {
                negateCond;
            } else {
                prepLoad;
                if (curOP = BOUNDS) then {
(*                    if (checkBounds) then
                        genCheckBounds(exprToGen@.typ2);
*)
                } else if (curOP = TOREAL) then {
                    addToInsnList(insnTemp[AVX]);
                    l3int3z := 3;
                    goto 10122;
                } else {
                    addToInsnList(KAVX+MINUS1);
                    if (curOP = RNEGOP) then
                        l3int3z := 3
                    else
                        l3int3z := 1;
                    goto 10122;
                }
            }
        } else
        if (curOP = STANDPROC) then {
            genFullExpr(exprToGen@.expr1);
            work := exprToGen@.num2;
            if (100 < work) then {
                prepLoad;
                addToInsnList(getHelperProc(work - 100));
            } else {
                if (insnList@.ilm = ilCONST) then {
                    arg1Const := true;
                    arg1Val := insnList@.ilf5;
                } else
                    arg1Const := false;
                arg2Const := (insnList@.typ = realType);
                if (arg1Const) then {
                    case work of
                    fnSQRT:  arg1Val.r := sqrt(arg1Val.r);
                    fnSIN:   arg1Val.r := sin(arg1Val.r);
                    fnCOS:   arg1Val.r := cos(arg1Val.r);
                    fnATAN:  arg1Val.r := arctan(arg1Val.r);
                    fnASIN:  arg1Val.r := arcsin(arg1Val.r);
                    fnLN:    arg1Val.r := ln(arg1Val.r);
                    fnEXP:   arg1Val.r := exp(arg1Val.r);
                    fnABS:   arg1Val.r := abs(arg1Val.r);
                    fnTRUNC: arg1Val.i := trunc(arg1Val.r);
                    fnODD:   arg1Val.b := odd(arg1Val.i);
                    fnORD:   arg1Val.i := ord(arg1Val.c);
                    fnCHR:   arg1Val.c := chr(arg1Val.i);
                    fnSUCC:  arg1Val.c := succ(arg1Val.c);
                    fnPRED:  arg1Val.c := pred(arg1Val.c);
                    fnPTR:   arg1Val.c := chr(arg1Val.i);
                    fnROUND: arg1Val.i := round(arg1Val.r);
                    fnCARD:  arg1Val.i := card(arg1Val.m);
                    fnMINEL: arg1Val.i := minel(arg1Val.m);
                    fnABSI:  arg1Val.i := abs(arg1Val.i);
                    fnEOF,
                    fnREF,
                    fnEOLN:
                        error(201);
                    end;
                    insnList@.ilf5 := arg1Val;
                } else
                if (work IN [fnEOF, fnREF, fnEOLN]) then {
                    if (work = fnREF) then {
                        setAddrTo(14);
                        addToInsnList(KITA+14);
                    } else {
                        setAddrTo(12);
                        addToInsnList(getHelperProc(work - 6));
                    };
                    with insnList@ do {
                        ilm := il2;
                        regsused := regsused + [0];
                    }
                } else {
                    prepLoad;
                    if (work = fnTRUNC) then {
                        l3int3z := 2;
                        addToInsnList(getHelperProc(58)); (*"P/TR"*)
                        goto 10122;
                    };
                    if (work IN [fnSQRT:fnEXP,
                                 fnODD:fnSUCC, fnCARD, fnPTR]) then {
                        l3int3z := 0;
                    } else if (work = fnABS) then
                        l3int3z := 3
                    else {
                        l3int3z := 1;
                    };
                    addToInsnList(funcInsn[work]);
                    goto 10122;
                }
            }
        } else {
            if (curOP = NOOP) then {
                curVal := exprToGen@.val;
                if (curVal.i IN set146z) then {
                    new(insnList);
                    with insnList@ do {
                        typ := exprToGen@.expr2@.typ;
                        next := NIL;
                        next2 := ;
                        regsused := [];
                        ilm := il1;
                        ilf7 := 18;
                        ilf5.i := indexreg[curVal.i];
                        ilf6 := 0;
                        st := st0;
                    }
                } else {
                    curVal.i := 14;
                    exprToGen@.val := curVal;
                    exprToGen := exprToGen@.expr2;
                    goto 7567;
                };
                exit
            } else {
                error(220);
            }
        };
    };
    insnList@.typ := exprToGen@.typ;
}; (* genFullExpr *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure formFileInit;
var l4exf1z: @extfilerec;
    l4var2z: tptr;
    l4var3z: irptr;
    l4int4z, l4int5z: integer;
{
    if (S5 IN optSflags.m) then {
        formAndAlign(KUJ+I13);
        exit
    };
    form2Insn(KITS+13, KATX+SP);
    while (curExpr <> NIL) do {
        l4exf1z := ptr(ord(curExpr@.typ));
        l4var3z := curExpr@.id2;
        l4int4z := l4var3z@.value;
        l4var2z := l4var3z@.typ@.base;
        l4int5z := l4var3z@.typ@.elsize;
        if (l4int4z < 74000B) then {
            form1Insn(getValueOrAllocSymtab(l4int4z) +
                      insnTemp[UTC] + I7);
            l4int4z := 0;
        };
        form3Insn(KVTM+I12 + l4int4z, KVTM+I10 + fileBufSize,
                  KVTM+I9 + l4int5z);
        form1Insn(KVTM+I11 + l4var2z@.size);
        if (l4exf1z = NIL) then {
            form1Insn(insnTemp[XTA]);
        } else {
            curVal.i := l4exf1z@.location;
            if (curVal.i = 512) then
                curVal.i := l4exf1z@.offset;
            form1Insn(KXTA+I8 + getFCSToffset);
        };
        formAndAlign(getHelperProc(69)); (*"P/CO"*)
        curVal := l4var3z@.id;
        form2Insn(KXTA+I8+getFCSToffset, KATX+I12+26);
        if (l4int5z <> 0) and (l4var2z <> charType) and
           typeCheck(l4var2z, integerType) then
            form2Insn(KXTA+ZERO, KATX+I12+25);
        curExpr := curExpr@.expr1;
    };
    form1Insn(getHelperProc(70)(*"P/IT"*) + (-I13-100000B));
    padToLeft;
}; (* formFileInit *)
procedure dump(expr : eptr; indent: integer);
{
    if (expr = NIL) or (expr = ptr(0c))then {
        writeln(' ':indent, '<NIL>'); exit;
    };
    writeln(' ':indent, expr@.op oct, ' ', expr@.op);
    indent := indent + 1;
if not (expr@.op in [NOOP,ALNUM,GETVAR,GETENUM,STANDPROC,BOUNDS]) then {
       dump(expr@.expr1, indent);
       if not (expr@.op in
[INEGOP,RNEGOP,TOREAL,DEREF,FILEPTR,NOTOP,GETFIELD,SHLEFT,SHRIGHT]) then
           dump(expr@.expr2, indent);
    }
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* formOperator *)
   writeln(' formoperator ', l3arg1z);
    l3bool13z := true;
    if (errors and (l3arg1z <> SETREG)) or (curExpr = NIL) then
        exit;
    if not (l3arg1z IN [FORMOP, STOREAT9, DFLTWDTH, FILEINIT, PCKUNPCK]) then {
        dump(curExpr, 1);
        genFullExpr(curExpr);
    };
    case l3arg1z of
    gen7: genOneOp;
    SETREG: {
        with insnList@ do {
            l3int3z := insnCount;
            new(l3var5z);
            l3var5z@.expr1 := expr63z;
            expr63z := l3var5z;
            l3var5z@.op := NOOP;
            case st of
            st0: {
                if (l3int3z = 0) then {
                    l3int2z := 14;
                } else {
                    l3var10z.m := set148z * set147z;
                    if (l3var10z.m <> []) then {
                        l3int2z := minel(l3var10z.m);
                    } else {
                        l3int2z := 14;
                    };
                    if (l3int3z <> 1) then {
                        setAddrTo(l3int2z);
                        addToInsnList(KITA + l3int2z);
                        P5117(op37);
                    } else if (l3int2z <> 14) then {
                        setAddrTo(l3int2z);
                        genOneOp;
                    };
                    l3var11z.m := [l3int2z] - [14];
                    set145z := set145z - l3var11z.m;
                    set147z := set147z - l3var11z.m;
                    set146z := set146z + l3var11z.m;
                };
                curVal.i := l3int2z;
                l3var5z@.val := curVal;
            };
            st1: {
                curVal.i := 14;
                l3var5z@.val := curVal;
            };
            st2:
                error(errVarTooComplex);
            end; (* case *)
        }; (* with *)
        l3var5z@.expr2 := curExpr;
    }; (* SETREG *)
    gen0: {
        prepLoad;
        if (insnCount > 1) then
            P5117(op36)
    };
    STORE: {
        prepStore;
        genOneOp
    };
    FORMOP: {
        curInsnTemplate := curVal.i;
        formOperator(LOAD);
        curInsnTemplate := insnTemp[XTA];
    };
    SETREG9: {
        if (insnList@.st <> st0) then
            error(errVarTooComplex);
        setAddrTo(9);
        genOneOp;
    };
    STOREAT9: {
        l3int1z := curVal.i;
        genFullExpr(curExpr);
        prepLoad;
        if (9 IN insnList@.regsused) then
            error(errVarTooComplex);
        genOneOp;
        form1Insn(KATX+I9 + l3int1z);
    };
    gen8: {
        setAddrTo(12);
        genOneOp
    };
    DFLTWDTH: {
        curVal.m := curVal.m + intZero;
        form1Insn(KXTA+I8 + getFCSToffset);
    };
    FRACWIDTH: {
        prepLoad;
        addxToInsnList(macro + mcPUSH);
        genOneOp;
    };
    gen11, gen12: {
        setAddrTo(11);
        if (l3arg1z = gen12) then
            addxToInsnList(macro + mcPUSH);
        genOneOp;
        set145z := set145z + [12];
    };
    FILEACCESS: {
        setAddrTo(12);
        genOneOp;
        formAndAlign(jumpTarget);
    };
    FILEINIT:
        formFileInit;
    LOAD: {
        prepLoad;
        genOneOp
    };
    BRANCH:
        with insnList@ do {
            noTarget := jumpTarget = 0;
            l3int3z := jumpTarget;
            if (ilm = ilCONST) then {
                if (ilf5.b) then {
                    jumpTarget := 0;
                } else {
                    if (noTarget) then {
                        formJump(jumpTarget);
                    } else {
                        form1Insn(insnTemp[UJ] + jumpTarget);
                    }
                }
            } else {
                direction := (16 in insnList@.regsused);
                if (insnList@.ilm = il3) and
                   (insnList@.ilf5.i <> 0) then {
                    genOneOp;
                    if (direction) then {
                        if (noTarget) then
                            formJump(l3int3z)
                        else
                            form1Insn(insnTemp[UJ] + l3int3z);
                        P0715(0, jumpTarget);
                        jumpTarget := l3int3z;
                    } else {
                        if (not noTarget) then {
                            if (not putLeft) then
                                padToLeft;
                            P0715(l3int3z, jumpTarget);
                        }
                    };
                } else {
                    if (insnList@.ilm = il1) then {
                        forValue := false;
                        prepLoad;
                        forValue := true;
                    };
                    genOneOp;
                    if (direction) then
                        nextInsn := insnTemp[U1A]
                    else
                        nextInsn := insnTemp[UZA];
                    if (noTarget) then {
                        jumpType := nextInsn;
                        formJump(l3int3z);
                        jumpType := insnTemp[UJ];
                        jumpTarget := l3int3z;
                    } else {
                        form1Insn(nextInsn + l3int3z);
                    }
                }
            }
        }; (* BRANCH *)
    PCKUNPCK: {
        l3var5z := curExpr;
        curExpr := curExpr@.expr1;
        formOperator(gen11);
        genFullExpr(l3var5z@.expr2);
        if (11 IN insnList@.regsused) then
            error(44); (* errIncorrectUsageOfStandProcOrFunc *)
        setAddrTo(12);
        genOneOp;
        arg1Type := l3var5z@.expr2@.typ;
        with arg1Type@.range@ do
            l3int3z := right - left + 1;
        form2Insn((KVTM+I14) + l3int3z,
                  (KVTM+I10+64) - arg1Type@.pcksize);
        l3int3z := ord(l3var5z@.typ);
        l3int1z := arg1Type@.perword;
        if (l3int3z = 72) then          (* P/KC *)
            l3int1z := 1 - l3int1z;
        form1Insn(getValueOrAllocSymtab(l3int1z) + (KVTM+I9));
        if typeCheck(curExpr@.typ, integerType) then {
            l3int1z := KXTA+ZERO;
        } else {
            l3int1z := insnTemp[XTA];
        };
        form1Insn(l3int1z);
        formAndAlign(getHelperProc(l3int3z));
   };
   LITINSN: {
        with insnList@ do {
            if (ilm <> ilCONST) then
                error(errNoConstant);
            if (insnList@.typ@.size <> 1) then
                error(errConstOfOtherTypeNeeded);
            curVal := insnList@.ilf5;
        }
    };
    end; (* case *)
}; (* formOperator *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseTypeRef(var newtype: tptr; skipTarget: setofsys);
label
    12247, 12366, 12476, 12760, 13020;
type
    pair = record
            first, second: integer
        end;
    pair7 = array [1..7] of pair;
    caserec = record
            size, count: integer;
            pairs: pair7;
        end;
var
    isPacked: boolean;
    cond: boolean;
    cases: caserec;
    leftBound, rightBound: word;
    numBits, l3int22z, span: integer;
    curEnum, curField: irptr;
    l3typ26z, nestedType, tempType, curType: tptr;
    l3idr31z: irptr;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure definePtrType(toType: tptr);
{
    new(curType = 4);
    curType@ := [1, 15, kindPtr, toType];
    new(curEnum = 5);
    curEnum@ := [curIdent, lineCnt, typelist, curType, TYPEID];
    typelist := curEnum;
}; (* definePtrType *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseRecordDecl(rectype: tptr; isOuterDecl: boolean);
var
    l4typ1z, selType, l4var3z, l4var4z, l4var5z: tptr;
    l4var6z: irptr;
    l4var7z, l4var8z: word;
    l4var9z: integer;
    cases1, cases2: caserec;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure addFieldToHash;
{
    curEnum@ := [curIdent, , typeHash[bucket], ,
                    FIELDID, NIL, curType, isPacked];
    typeHash[bucket] := curEnum;
}; (* addFieldToHash *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure packFields;
label
    11523, 11622;
var
    l5var1z, pairIdx, l5var3z, l5var4z, l5var5z: integer;
    l5var6z: @pair;
{
    parseTypeRef(selType, skipTarget + [SWITCHSY]);
    if (curType@.ptr2 = NIL) then {
        curType@.ptr2 := curField;
    } else {
        l3idr31z@.list := curField;
    };
    cond := isFileType(selType);
    if (not isOuterDecl) and cond then
        error(errTypeMustNotBeFile);
    curType@.flag := cond or curType@.flag;
    l3idr31z := curEnum;
    repeat
        curField@.typ := selType;
        if (isPacked) then {
            l5var1z := selType@.bits;
            curField@.width := l5var1z;
            if (l5var1z <> 48) then {
                for pairIdx to cases.count do
11523:          {
                    l5var6z := ref(cases.pairs[pairIdx]);
                    if (l5var6z@.first >= l5var1z) then {
                        curField@.shift := 48 - l5var6z@.first;
                        curField@.offset := l5var6z@.second;
                        if not (S6 IN optSflags.m) then
                            curField@.shift := 48 - curField@.width -
                                                  curField@.shift;
                        l5var6z@.first := l5var6z@.first - l5var1z;
                        if l5var6z@.first = 0 then {
                            cases.pairs[pairIdx] :=
                                cases.pairs[cases.count];
                            cases.count := cases.count - 1;
                        };
                        goto 11622;
                    }
                };
                if (cases.count <> 7) then {
                    cases.count := cases.count + 1;
                    pairIdx := cases.count;
                } else {
                    l5var3z := 48;
                    for l5var4z to 7 do {
                        l5var5z := cases.pairs[l5var4z].first;
                        if (l5var5z < l5var3z) then {
                            l5var3z := l5var5z;
                            pairIdx := l5var4z;
                        }
                    }; (* for *)
                };
                cases.pairs[pairIdx] := [48, cases.size];
                cases.size := cases.size + 1;
                goto 11523;
            }
        };
        curField@.pckfield := false;
        curField@.offset := cases.size;
        cases.size := cases.size + selType@.size;
11622:
        if (PASINFOR.listMode = 3) then {
            write(' ':16);
            if (curField@.pckfield) then
                write('PACKED');
            write(' FIELD ');
            printTextWord(curField@.id);
            write('.OFFSET=', curField@.offset:5 oct, 'B');
            if (curField@.pckfield) then {
                write('.<<=SHIFT=', curField@.shift:2,
                      '. WIDTH=', curField@.width:2, ' BITS');
            } else {
                write('.WORDS=', selType@.size:0);
            };
            writeLN;
        };
        cond := (curField = curEnum);
        curField := curField@.list;
    until cond;

}; (* packFields *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* parseRecordDecl *)
    if (SY <> BEGINSY) then
       requiredSymErr(BEGINSY);
    int93z := 3;
    inSymbol;

    while (SY = IDENT) do {
        l4var6z := NIL;
        repeat
            if (SY <> IDENT) then {
                error(errNoIdent);
            } else {
                if (hashTravPtr <> NIL) then
                    error(errIdentAlreadyDefined);
                new(curEnum = 10);
                addFieldToHash;
                if (l4var6z = NIL) then {
                    curField := curEnum;
                } else {
                    l4var6z@.list := curEnum;
                };
                l4var6z := curEnum;
                int93z := 3;
                inSymbol;
            };
            cond := (SY <> COMMA);
            if (not cond) then {
                int93z := 3;
                inSymbol;
            }
        until cond;
        checkSymAndRead(COLON);
        packFields;
        if (SY = SEMICOLON) then {
            int93z := 3;
            inSymbol;
        }
    };
    if (SY = SWITCHSY) then {
        int93z := 3;
        inSymbol;
        selType := integerType;
(identif)
        if (SY <> IDENT) then {
            error(3);
            skip(skipTarget + [OFSY]);
        } else {
            l4var8z := curIdent;
            l4var9z := bucket;
            curEnum := hashTravPtr;
            inSymbol;
            if (SY = COLON) then {
                if (curEnum <> NIL) then
                    error(errIdentAlreadyDefined);
                new(curEnum = 10);
                curIdent := l4var8z;
                bucket := l4var9z;
                addFieldToHash;
                inSymbol;
                curField := curEnum;
                packFields;
            } else {
                curEnum := symHash[l4var9z];
                while (curEnum <> NIL) do {
                    if (curEnum@.id <> l4var8z) then {
                        curEnum := curEnum@.next;
                    } else {
                        if (curEnum@.cl <> TYPEID) then {
                            error(errNotAType);
                            selType := integerType;
                        } else {
                            selType := curEnum@.typ;
                        };
                        exit identif;
                    };
                };
                error(errNotDefined)
            };
        };
        if (selType@.k = kindRange) then
            selType := selType@.base;
        checkSymAndRead(OFSY);
        cases1 := cases;
        cases2 := cases;
        l4typ1z := NIL;
        repeat
            l4var3z := NIL;
            repeat
                parseLiteral(l4var4z, l4var7z, false);
                if (l4var4z = NIL) then
                    error(errNoConstant)
                else if (not typeCheck(l4var4z, selType)) then
                    error(errConstOfOtherTypeNeeded);
                new(l4var5z = 7);
                l4var5z@ := [cases.size, 48, kindCases,
                                    l4var7z, NIL, NIL, NIL];
                if (l4var3z = NIL) then {
                    tempType := l4var5z;
                } else {
                    l4var3z@.r6 := l4var5z;
                };
                l4var3z := l4var5z;
                inSymbol;
                cond := (SY <> COMMA);
                if (not cond) then
                    inSymbol;
            until cond;
            if (l4typ1z = NIL) then {
                if (curType@.base = NIL) then {
                    curType@.base := tempType;
                } else {
                    rectype@.first := tempType;
                }
            } else {
                l4typ1z@.next := tempType;
            };
            l4typ1z := tempType;
            checkSymAndRead(COLON);
            parseRecordDecl(tempType, false);
            if (cases2.size < cases.size) or
               isPacked and (cases.size = 1) and (cases2.size = 1) and
                (cases.count = 1) and (cases2.count = 1) and
                (cases.pairs[1].first < cases2.pairs[1].first) then {
                cases2 := cases;
            };
            cases := cases1;
            cond := SY <> SEMICOLON;
            if (not cond) then
                inSymbol;
            if (SY = ENDSY) then
                cond := true;
        until cond;
        cases := cases2;
    };
    rectype@.size := cases.size;
    if isPacked and (cases.size = 1) and (cases.count = 1) then {
        rectype@.bits := 48 - cases.pairs[1].first;
    };
    checkSymAndRead(ENDSY);
}; (* parseRecordDecl*)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* parseTypeRef *)
    isPacked := false;
12247:
    if (SY = ENUMSY) then {
        inSymbol;
        checkSymAndRead(BEGINSY);
        span := 0;
        int93z := 0;
        curField := NIL;
        new(curType = 6);
        while (SY = IDENT) do {
            if (isDefined) then
                error(errIdentAlreadyDefined);
            new(curEnum = 7);
            curEnum@ := [curIdent, curFrameRegTemplate,
                            symHash[bucket], curType,
                            ENUMID, NIL, ptr(span)];
            symHash[bucket] := curEnum;
            span := span + 1;
            if (curField = NIL) then {
                curType@.enums := curEnum;
            } else {
                curField@.list := curEnum;
            };
            curField := curEnum;
            inSymbol;
            if (SY = COMMA) then {
                int93z := 0;
                inSymbol;
            } else {
                if (SY <> ENDSY) then
                    requiredSymErr(ENDSY);
            };
        };
        checkSymAndRead(ENDSY);
        if (curField = NIL) then {
            curType := booleanType;
            error(errNoIdent);
        } else {
            curType@ := [1, nrOfBits(span - 1), kindScalar, ,
                          span, 0];
        };
    } else
    if (SY = ARROW) then {
        inSymbol;
        if (SY <> IDENT) then {
            error(errNoIdent);
            curType := pointerType;
        } else {
            if (hashTravPtr = NIL) then {
                if (inTypeDef) then {
                    if (knownInType(curEnum)) then {
                        curType := curEnum@.typ;
                    } else {
                        definePtrType(integerType);
                    };
                } else {
12366:              error(errNotAType);
                    curType := pointerType;
                };
            } else {
                if (hashTravPtr@.cl <> TYPEID) then {
                    goto 12366
                };
                new(curType = 4);
                with curType@ do {
                    size := 1;
                    bits := 15;
                    k := kindPtr;
                    base := hashTravPtr@.typ;
                }
            };
            inSymbol;
        }
    } else
    if (SY = IDENT) then {
        if (hashTravPtr <> NIL) then {
            if (hashTravPtr@.cl = TYPEID) then {
                curType := hashTravPtr@.typ;
            } else {
                goto 12760;
            }
        } else {
            if (inTypeDef) then {
                if (knownInType(curEnum)) then {
                    curType := curEnum@.typ;
                    curType@.base := booleanType;
                } else {
                    definePtrType(booleanType);
                };
            } else {
                error(errNotAType);
                curType := integerType;
            };
        };
        inSymbol;
    } else {
        if (SY = PACKEDSY) then {
            isPacked := true;
            inSymbol;
            goto 12247;
        };
        if (SY = STRUCTSY) then {
            new(curType = 7);
            typ121z := curType;
            with curType@ do {
                size := 0;
                bits := 48;
                k := kindStruct;
                ptr1 := NIL;
                first := NIL;
                flag := false;
                pckrec := isPacked;
            };
            cases.size := 0;
            cases.count := 0;
            inSymbol;
            parseRecordDecl(curType, true);
        } else
        if (SY = ARRAYSY) then {
            inSymbol;
            checkSymAndRead(LBRACK);
            tempType := NIL;
12476:      parseTypeRef(nestedType, skipTarget + [OFSY]);
            if (nestedType@.k <> kindRange) then
                error(8); (* errNotAnIndexType *)
            new(l3typ26z, kindArray);
            with l3typ26z@ do {
                size := ord(tempType);
                bits := 48;
                k := kindArray;
                range := nestedType;
            };
            if (tempType = NIL) then
                curType := l3typ26z
            else
                tempType@.base := l3typ26z;
            tempType := l3typ26z;
            if (SY = COMMA) then {
                inSymbol;
                goto 12476;
            };
            checkSymAndRead(RBRACK);
            checkSymAndRead(OFSY);
            parseTypeRef(nestedType, skipTarget);
            l3typ26z@.base := nestedType;
            if isFileType(nestedType) then
                error(errTypeMustNotBeFile);
            repeat with l3typ26z@, ptr2@ do {
                span := high.i - low + 1;
                tempType := ptr(size);
                l3int22z := base@.bits;
                if (24 < l3int22z) then
                    isPacked := false;
                bits := 48;
                if (isPacked) then {
                    l3int22z := 48 DIV l3int22z;
                    if (l3int22z = 9) then {
                        l3int22z := 8;
                    } else if (l3int22z = 5) then {
                        l3int22z := 4
                    };
                    perword := l3int22z;
                    pcksize := 48 DIV l3int22z;
                    l3int22z := span * pcksize;
                    if l3int22z mod 48 = 0 then
                        numBits := 0
                    else
                        numBits := 1;
                    size := l3int22z div 48 + numBits;
                    if (size = 1) then
                        bits := l3int22z;
                } else {
                    size := span * base@.size;
                    curVal.i := base@.size;
                    curVal.m := curVal.m * [7:47] + [0];
                    if (range@.base <> integerType) then
                        curVal.m := curVal.m + [1, 3];
                    l3typ26z@.perword := KMUL+ I8 + getFCSToffset;
                };
                l3typ26z@.pck := isPacked;
                isPacked := false;
                cond := (curType = l3typ26z);
                l3typ26z := tempType;
            } until cond;
        } else
        if (SY = FILESY) then {
            inSymbol;
            checkSymAndRead(OFSY);
            parseTypeRef(nestedType, skipTarget);
            if (isFileType(nestedType)) then
                error(errTypeMustNotBeFile);
            if (isPacked) then {
                l3int22z := nestedType@.bits;
                if (24 < l3int22z) then
                    isPacked := false;
            };
            new(curType, kindFile);
            if (not isPacked) then
                l3int22z := 0;
            with curType@ do {
                size := 30;
                bits := 48;
                k := kindFile;
                base := nestedType;
                elsize := l3int22z;
            }
        } else {
12760:      ;
            parseLiteral(tempType, leftBound, true);
            if (tempType <> NIL) then {
                inSymbol;
                if (SY <> COLON) then {
                    requiredSymErr(COLON);
                } else {
                    inSymbol;
                };
                parseLiteral(curType, rightBound, true);
                if (curType = tempType) and
                   (curType@.k = kindScalar) then {
                    defineRange(curType, leftBound.i, rightBound.i);
                    inSymbol;
                    goto 13020;
                }
            };
            error(64); (* errIncorrectRangeDefinition *)
            curType := booleanType;
        };
    };
13020:
    if (errors) then
        skip(skipToSet + [RPAREN, RBRACK, SEMICOLON, OFSY]);
    newtype := curType;
}; (* parseTypeRef *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure dumpEnumNames(l3arg1z: tptr);
var
    l3var1z: irptr;
{
    if (l3arg1z@.start = 0) then {
        l3arg1z@.start := FcstCnt;
        l3var1z := l3arg1z@.enums;
        while (l3var1z <> NIL) do {
            curVal := l3var1z@.id;
            l3var1z := l3var1z@.list;
            toFCST;
        }
    }
}; (* dumpEnumNames *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure formPMD;
var
    l3typ1z: tptr;
    l3var2z: word;
    l3var3z: bitset;
    l3var4z: boolean;
    l3var5z: kind;
{
    for l3var4z := false to true do {
        if l3var4z then {
            optSflags.m := (optSflags.m + [S3]);
            curVal.i := 74001B;
            P0715(2, 34); (*"P/DS"*)
            curVal := l2idr2z@.id;
            toFCST;
            curVal.i := lineCnt;
            toFCST;
        };
        for jj := 0 to 127 do {
            curIdRec := symHash[jj];

            while (curIdRec <> NIL) and
                  (l2idr2z < curIdRec) do with curIdRec@ do {
                l3var2z.i := typ@.size;
                if (cl IN [VARID, FORMALID]) and
                  (value < 74000B) then {
                    curVal := id;
                    if (l3var4z) then
                        toFCST;
                    l3typ1z := typ;
                    l3var5z := l3typ1z@.k;
                    l3var3z := [];
                    if (l3var5z = kindPtr) then {
                        l3typ1z := l3typ1z@.base;
                        l3var5z := l3typ1z@.k;
                        l3var3z := [0];
                    };
                    if (l3typ1z = realType) then
                        curVal.i := 0
                    else if typeCheck(l3typ1z, integerType) then
                        curVal.i := 100000B
                    else if typeCheck(l3typ1z, charType) then
                        curVal.i := 200000B
                    else if (l3var5z = kindArray) then
                        curVal.i := 400000B
                    else if (l3var5z = kindScalar) then {
                        dumpEnumNames(l3typ1z);
                        curVal.i := 1000000B * l3typ1z@.start + 300000B;
                    } else if (l3var5z = kindFile) then
                        curVal.i := 600000B
                    else {
                        curVal.i := 500000B;
                    };
                    curVal.i := curVal.i + curIdRec@.value;
                    l3var2z := l3var2z;
                    besm(ASN64-33);
                    l3var2z := ;
                    curVal.m := curVal.m * [15:47] + l3var2z.m + l3var3z;
                    if (l3var4z) then
                        toFCST;
                };
                curIdRec := curIdRec@.next;
            };
        }; (*13167+*)
        curVal.m := [];
        if l3var4z then
            toFCST;
    }
}; (* formPMD *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseDecls(l3arg1z: integer);
var
    l3int1z: integer;
    frame:   word;
    l3var3z: boolean;
{
    case l3arg1z of
    0: {
        int93z := 0;
        inSymbol;
        if (SY <> IDENT) then
            errAndSkip(3, skipToSet + [IDENT]);
    };
    1: {
        prevErrPos := 0;
        write('IDENT ');
        printTextWord(l2var12z);
        write(' IN LINE ', curIdRec@.offset:0);
    };
    2: {
        with l2idr2z@ do
            ; (* useless *)
        padToLeft;
        l3var3z := 22 IN l2idr2z@.flags;
        l3arg1z := l2idr2z@.pos;
        frame.i := moduleOffset - 40000B;
        if (l3arg1z <> 0) then
            symTab[l3arg1z] := [24, 29] + frame.m * halfWord;
        l2idr2z@.pos := moduleOffset;
        l3arg1z := F3307(l2idr2z);
        if l3var3z then {
            if (41 >= entryPtCnt) then {
                curVal := l2idr2z@.id;
                entryPtTable[entryPtCnt] := makeNameWithStars;
                entryPtTable[entryPtCnt+1] := [1] + frame.m - [0, 3];
                entryPtCnt := entryPtCnt + 2;
            } else
                error(87); (* errTooManyEntryProcs *)
        };
        if (l2idr2z@.typ = NIL) then {
            frame.i := 3;
        } else {
            frame.i := 4;
        };
        if l3var3z then
            form2Insn((KVTM+I14) + l3arg1z + (frame.i - 3) * 1000B,
                      getHelperProc(94 (*"P/NN"*)) - 10000000B);
        if 1 < l3arg1z then {
            frame.i := getValueOrAllocSymtab(-(frame.i+l3arg1z));
        };
        if (S5 IN optSflags.m) and
           (curProcNesting = 1) then
            l3int1z := 59  (* P/LV *)
        else
            l3int1z := curProcNesting;
        l3int1z := getHelperProc(l3int1z) - (-4000000B);
        if l3arg1z = 1 then {
            form1Insn((KATX+SP) + frame.i);
        } else if (l3arg1z <> 0) then {
            form2Insn(KATX+SP, (KUTM+SP) + frame.i);
        };
        formAndAlign(l3int1z);
        savedObjIdx := objBufIdx;
        if (curProcNesting <> 1) then
            form1Insn(0);
        if l3var3z then
            form1Insn(KVTM+I8+74001B);
        if (l2int11z <> 0) then {
            form1Insn(insnTemp[XTA]);
            formAndAlign(KVJM+I13 + l2int11z);
            curVal.i := l2int11z;
            P0715(2, 49 (* "P/RDC" *));
        };
        if (curProcNesting = 1) then {
            if (heapCallsCnt <> 0) and
               (heapSize = 0) then
                error(65 (*errCannotHaveK0AndNew*));
            l3var3z := (heapSize = 0) or
                (heapCallsCnt = 0) and (heapSize = 100);
            if (heapSize = 100) then
                heapSize := 4;
            if (not l3var3z) then {
                form2Insn(KVTM+I14+getValueOrAllocSymtab(heapSize*2000B),
                          getHelperProc(26 (*"P/GD"*)));
                padToLeft;
            }
        };
        if (doPMD) then
            formPMD;
    }
    end; (* case *)
}; (* parseDecls *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure statement;
label
    8888;
var
    boundary              : eptr;
    numLabPtr             : @numLabel;
    strLabPtr             : @strLabel;
    l3var4z               : boolean;
    l3bool5z              : boolean;
    l3var6z               : idclass;
    curOffset             : word;
    startLine             : integer;
    ifWhlTarget, elseJump : integer;
    whileExpr             : eptr;
    l3idr12z              : irptr;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function isCharArray(arg: tptr): boolean;
{
    with arg@ do
        isCharArray := (k = kindArray) and (base = charType);
}; (* isCharArray *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure expression;
    forward;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseLval;
label
    13462, 13530;
var
    l4exp1z, l4exp2z: eptr;
    l4typ3z: tptr;
    l4var4z: kind;
{
    if (hashTravPtr@.cl = FIELDID) then {
        curExpr := expr62z;
        goto 13530;
    } else {
        new(curExpr);
        with curExpr@ do {
            typ := hashTravPtr@.typ;
            op := GETVAR;
            id1 := hashTravPtr;
        };
13462:  inSymbol;
        l4typ3z := curExpr@.typ;
        l4var4z := l4typ3z@.k;
        if (SY = ARROW) then {
            new(l4exp1z);
            with l4exp1z@ do {
                expr1 := curExpr;
                if (l4var4z = kindPtr) then {
                    typ := l4typ3z@.base;
                    op := DEREF;
                } else if (l4var4z = kindFile) then {
                    typ := l4typ3z@.base;
                    op := FILEPTR;
                } else {
                    stmtName := '  ^   ';
                    error(errWrongVarTypeBefore);
                    l4exp1z@.typ := l4typ3z;
                }
            };
            curExpr := l4exp1z;
        } else if (SY = PERIOD) then {
            if (l4var4z = kindStruct) then {
                int93z := 3;
                typ121z := l4typ3z;
                inSymbol;
                if (hashTravPtr = NIL) then {
                    error(20); (* errDigitGreaterThan7 ??? *)
                } else 13530: {
                    new(l4exp1z);
                    with l4exp1z@ do {
                        typ := hashTravPtr@.typ;
                        op := GETFIELD;
                        expr1 := curExpr;
                        id2 := hashTravPtr;
                    };
                    curExpr := l4exp1z;
                }
            } else {
                stmtName := '  .   ';
                error(errWrongVarTypeBefore);
            };
        } else if (SY = LBRACK) then {
            stmtName := '  [   ';
            repeat
                l4exp1z := curExpr;
                expression;
                l4typ3z := l4exp1z@.typ;
                if (l4typ3z@.k <> kindArray) then {
                    error(errWrongVarTypeBefore);
                } else {
                    if (not typeCheck(l4typ3z@.range, curExpr@.typ)) then
                        error(66 (*errOtherIndexTypeNeeded *));
                    new(l4exp2z);
                    with l4exp2z@ do {
                        typ := l4typ3z@.base;
                        expr1 := l4exp1z;
                        expr2 := curExpr;
                        op := GETELT;
                    };
                    l4exp1z := l4exp2z;
                };
                curExpr := l4exp1z;
                stmtName := '  ,   ';
            until (SY <> COMMA);
            if (SY <> RBRACK) then
                error(67 (*errNeedBracketAfterIndices*));
        } else exit;
    };
    goto 13462;
}; (* parseLval *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure castToReal(var value: eptr);
var
    cast: eptr;
{
    new(cast);
    with cast@ do {
        typ := realType;
        op := TOREAL;
        expr1 := value;
        value := cast;
    }
}; (* castToReal *)
%
function areTypesCompatible(var l4arg1z: eptr): boolean;
{
    if (arg1Type = realType) then {
        if typeCheck(integerType, arg2Type) then {
            castToReal(l4arg1z);
            areTypesCompatible := true;
            exit
        };
    } else if (arg2Type = realType) and
               typeCheck(integerType, arg1Type) then {
        castToReal(curExpr);
        areTypesCompatible := true;
        exit
    };
    areTypesCompatible := false;
}; (* areTypesCompatible *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseCallArgs(subroutine: irptr);
label
    13736;
var
    noArgs: boolean;
    curActual, callExpr, argList: eptr;
    curFormal: irptr;
    actualOp: operator;
    formClass: idclass;
{
    with subroutine@ do {
        if typ <> NIL then
            set146z := set146z - flags;
        noArgs := (list = NIL) and not (24 in flags);
    };
    new(callExpr);
    argList := callExpr;
    bool48z := true;
    with callExpr@ do {
        typ := subroutine@.typ;
        op := ALNUM;
        id2 := subroutine;
        id1 := NIL;
    };
    if (SY = LPAREN) then {
        if (noArgs) then {
            curFormal := subroutine@.argList;
            if (curFormal = NIL) then {
                inSymbol;
                if (SY <> RPAREN) then {
                    error(errTooManyArguments);
                    goto 8888;
                };
                curExpr := callExpr;
                inSymbol;
                exit;
            }
        };
        repeat
            if (noArgs) and (subroutine = curFormal) then {
                error(errTooManyArguments);
                goto 8888;
            };
            bool47z := true;
            expression;
            actualOp := curExpr@.op;
(a)         if noArgs then {
                formClass := curFormal@.cl;
                if (actualOp = PCALL) then {
                    if (formClass <> ROUTINEID) or
                       (curFormal@.typ <> NIL) then {
13736:                  error(39); (*errIncompatibleArgumentKinds*)
                        exit a
                    }
                } else {
                    if (actualOp = FCALL) then {
                        if (formClass = ROUTINEID) then {
                            if (curFormal@.typ = NIL) then
                                goto 13736
                        } else
                        if (curExpr@.id2@.argList = NIL) and
                           (formClass = VARID) then {
                            curExpr@.op := ALNUM;
                            curExpr@.expr1 := NIL;
                        } else
                            goto 13736;
                    } else
                    if (actualOp IN lvalOpSet) then {
                        if (formClass <> VARID) and
                           (formClass <> FORMALID) then
                            goto 13736;
                    } else {
                        if (formClass <> VARID) then
                            goto 13736;
                    }
                };
                arg1Type := curExpr@.typ;
                if (arg1Type <> NIL) then {
                    if not typeCheck(arg1Type, curFormal@.typ) then
                        error(40); (*errIncompatibleArgumentTypes*)
                }
            };
            new(curActual);
            with curActual@ do {
                typ := NIL;
                expr1 := NIL;
                expr2 := curExpr;
            };
            argList@.expr1 := curActual;
            argList := curActual;
            if (noArgs) then
                curFormal := curFormal@.list;
        until (SY <> COMMA);
        if (SY <> RPAREN) or
           noArgs and (curFormal <> subroutine) then
            error(errNoCommaOrParenOrTooFewArgs)
        else
            inSymbol;
    } else {
%        if (noArgs) and (subroutine@.argList <> NIL) then
            error(42); (*errNoArgList*)
    };
    curExpr := callExpr;
}; (* parseCallArgs *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure factor;
label
    14567;
var
    l4var1z: word;
    l4var2z: boolean;
    l4var3z, l4var4z: word;
    l4exp5z, newExpr, l4var7z, l4var8z: eptr;
    routine: irptr;
    newOp: operator;
    l4typ11z: tptr;
    l4var12z: boolean;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure stdCall;
const chkREAL = 0;  chkINT    = 1;  chkCHAR = 2;    chkSCALAR = 3;
      chkPTR  = 4;  chkFILE   = 5;  chkSET  = 6;    chkOTHER  = 7;
var
    l5op1z: operator;
    l5var2z: tptr;
    argKind: kind;
    asBitset: bitset;
    stProcNo, checkMode: integer;
{
    curVal.i := routine@.low;
    stProcNo := curVal.i;
    if (SY <> LPAREN) then {
        requiredSymErr(LPAREN);
        goto 8888;
    };
    expression;
    if (stProcNo >= fnEOF) and
       (fnEOLN >= stProcNo) and
       not (curExpr@.op IN [GETELT..FILEPTR]) then {
        error(27); (* errExpressionWhereVariableExpected *)
        exit;
    };
    arg1Type := curExpr@.typ;
    if (arg1Type@.k = kindRange) then
        arg1Type := arg1Type@.base;
    argKind := arg1Type@.k;
    if (arg1Type = realType) then
        checkMode := chkREAL
    else if (arg1Type = integerType) then
        checkMode := chkINT
    else if (arg1Type = charType) then
        checkMode := chkCHAR
    else if (argKind = kindScalar) then
        checkMode := chkSCALAR
    else if (argKind = kindPtr) then
        checkMode := chkPTR
    else if (argKind = kindFile) then
        checkMode := chkFILE
    else {
        checkMode := chkOTHER;
    };
    asBitset := [stProcNo];
    if not ((checkMode = chkREAL) and
            (asBitset <= [fnSQRT:fnTRUNC, fnREF, fnROUND])
           or ((checkMode = chkINT) and
            (asBitset <= [fnSQRT:fnABS,fnODD,fnCHR,fnREF,fnCARD,fnMINEL,fnPTR]))
           or ((checkMode IN [chkCHAR, chkSCALAR, chkPTR]) and
            (asBitset <= [fnORD, fnSUCC, fnPRED, fnREF]))
           or ((checkMode = chkFILE) and
            (asBitset <= [fnEOF, fnREF, fnEOLN]))
           or ((checkMode = chkOTHER) and
            (stProcNo = fnREF))) then
        error(errNeedOtherTypesOfOperands);
    if not (asBitset <= [fnABS, fnSUCC, fnPRED]) then {
        arg1Type := routine@.typ;
    } else if (checkMode = chkINT) and (asBitset <= [fnABS]) then {
        stProcNo := fnABSI
    };
    new(newExpr);
    newExpr@.op := STANDPROC;
    newExpr@.expr1 := curExpr;
    newExpr@.num2 := stProcNo;
    curExpr := newExpr;
    curExpr@.typ := arg1Type;
    checkSymAndRead(RPAREN);

}; (* stdCall *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* factor *)
    l4var2z := bool47z;
    bool47z := false;
    if (SY < MULOP) then {
        case SY of
        IDENT: {
            if (hashTravPtr = NIL) then {
                error(errNotDefined);
                curExpr := uVarPtr;
            } else
                case hashTravPtr@.cl of
                TYPEID: {
                    error(23); (* errTypeIdInsteadOfVar *)
                    curExpr := uVarPtr;
                };
                ENUMID: {
                    new(curExpr);
                    with curExpr@ do {
                        typ := hashTravPtr@.typ;
                        op := GETENUM;
                        num1 := hashTravPtr@.value;
                        num2 := 0;
                    };
                    inSymbol;
                };
                ROUTINEID: {
                    routine := hashTravPtr;
                    inSymbol;
                    if (routine@.offset = 0) then {
                        if (routine@.typ <> NIL) and
                           (SY = LPAREN) then {
                            stdCall;
                            exit;
                        };
                        error(44) (* errIncorrectUsageOfStandProcOrFunc *)
                    } else if (routine@.typ = NIL) then {
                        if (l4var2z) then {
                            newOp := PCALL;
                        } else {
                            error(68); (* errUsingProcedureInExpression *)
                        }
                   } else  {
                        if (SY = LPAREN) then {
                            parseCallArgs(routine);
                            exit
                        };
                        if (l4var2z) then {
                            newOp := FCALL;
                        } else {
                            parseCallArgs(routine);
                            exit
                        };
                    };
                    new(curExpr);
                    if not (SY IN [RPAREN, COMMA]) then {
                        error(errNoCommaOrParenOrTooFewArgs);
                        goto 8888;
                    };
                    with curExpr@ do {
                        typ := routine@.typ;
                        op := newOp;
                        expr1 := NIL;
                        id2 := routine;
                    }
                };
                VARID, FORMALID, FIELDID:
                    parseLval;
                end (* case *)
        };
        LPAREN: {
            expression;
            checkSymAndRead(RPAREN);
        };
        INTCONST, REALCONST, CHARCONST, LTSY: {
            new(curExpr);
            parseLiteral(curExpr@.typ, curExpr@.d1, false);
            curExpr@.num2 := ord(numFormat);
            curExpr@.op := GETENUM;
            inSymbol;
        };
        NOTSY: {
            inSymbol;
            factor;
            newExpr := curExpr;
            new(curExpr);
            curexpr@.typ := booleanType;
            if (newExpr@.typ = booleanType) then with curExpr@ do {
                op := NOTOP;
                expr1 := newExpr;
            } else if (newExpr@.typ = integerType) then {
                with curExpr@ do {
                    op := EQOP;
                    expr1 := newExpr;
                    new(expr2);
                };
               curExpr@.expr2@ := [integerType, GETENUM, 0C];
            } else
                error(errNeedOtherTypesOfOperands);
        };
        LBRACK: {
            new(curExpr);
            inSymbol;
            l4var8z := curExpr;
            l4var1z.m := [];
            if (SY <> RBRACK) then {
                l4var12z := true;
                readNext := false;
                repeat
                    newExpr := curExpr;
                    expression;
                    if (l4var12z) then {
                        l4typ11z := curExpr@.typ;
                        if not (l4typ11z@.k IN [kindScalar, kindRange]) then
                            error(23); (* errTypeIdInsteadOfVar *)
                    } else {
                        if not typeCheck(l4typ11z, curExpr@.typ) then
                            error(24); (*errIncompatibleExprsInSetCtor*)
                    };
                    l4var12z := false;
                    l4exp5z := curExpr;
                    if (SY = COLON) then {
                        expression;
                        if not typeCheck(l4typ11z, curExpr@.typ) then
                            error(24); (*errIncompatibleExprsInSetCtor*)
                        if (l4exp5z@.op = GETENUM) and
                           (curExpr@.op = GETENUM) then {
                            l4var4z.i := l4exp5z@.num1;
                            l4var3z.i := curExpr@.num1;
                            l4var4z.m := l4var4z.m - intZero;
                            l4var3z.m := l4var3z.m - intZero;
                            l4var1z.m := l4var1z.m + [l4var4z.i..l4var3z.i];
                            curExpr := newExpr;
                            goto 14567;
                        };
                        new(l4var7z);
                        with l4var7z@ do {
                            typ := integerType;
                            op := MKRANGE;
                            expr1 := l4exp5z;
                            expr2 := curExpr;
                        };
                        l4exp5z := l4var7z;
                   } else {
                        if (l4exp5z@.op = GETENUM) then {
                            l4var4z.i := l4exp5z@.num1;
                            l4var4z.m := l4var4z.m - intZero;
                            l4var1z.m := l4var1z.m + [l4var4z.i];
                            curExpr := newExpr;
                            goto 14567;
                        };
                        new(l4var7z);
                        with l4var7z@ do {
                            typ := integerType;
                            op := STANDPROC;
                            expr1 := l4exp5z;
                            num2 := 109; (* P/SS *)
                            l4exp5z := l4var7z;
                        }
                    };
                    new(curExpr);
                    with curExpr@ do {
                        typ := integerType;
                        op := SETOR;
                        expr1 := newExpr;
                        expr2 := l4exp5z;
                    };
14567:              ;
                until SY <> COMMA;
            };
            checkSymAndRead(RBRACK);
            with l4var8z@ do {
                op := GETENUM;
                typ := integerType;
                d1 := l4var1z;
            }
        };
        end; (* case *)
    } else if (charClass = SETAND) then {
        inSymbol;
        factor;
        if not (curExpr@.op IN lvalOpSet) then
            error(27); (* errExpressionWhereVariableExpected *)
        newExpr := curExpr;
        new(curExpr);
        with curExpr@ do {
            typ := pointerType;
            op  :=  STANDPROC;
            expr1  :=  newExpr;
            num2  :=  fnREF;
        }
    } else {
        error(errBadSymbol);
        goto 8888;
    }

}; (* factor *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure term;
label
    14650;
var
    curOp: operator;
    leftArg, l4var3z: eptr;
    match: boolean;
{
    factor;
    while (SY = MULOP) do {
        curOp := charClass;
        inSymbol;
        leftArg := curExpr;
        factor;
        arg1Type := curExpr@.typ;
        arg2Type := leftArg@.typ;
        match := typeCheck(arg1Type, arg2Type);
        if (not match) and
           (RDIVOP < curOp) then {
14650:                           error(errNeedOtherTypesOfOperands);
                                 writeln(curOp)
        } else {
            case curOp of
            MUL, RDIVOP: {
                if (match) then {
                    if (arg1Type = realType) then {
                        (* empty *)
                    } else {
                        if (baseType = integerType) then {
                            arg1Type := integerType;
                            curOp := imulOpMap[curOp];
                        } else {
                            goto 14650;
                        }
                    }
                } else {
                    if areTypesCompatible(leftArg) then {
                        arg1Type := realType;
                    } else
                        goto 14650;
                }
            };
            AMPERS: {
                    if (arg1Type<>booleanType) and (arg1Type<>integerType) then
                        goto 14650;
                    arg1Type := booleanType;
            };
            IDIVOP: {
                if (baseType <> integerType) then
                    goto 14650;
                arg1Type := integerType;
            };
            SHLEFT, SHRIGHT: { arg1Type := arg2Type; };
            IMODOP: {
                if (baseType = integerType) then {
                    arg1Type := integerType;
                } else {
                    goto 14650;
                }
            };
            end;
            new(l4var3z);
            with l4var3z@ do {
                op := curOp;
                expr1 := leftArg;
                expr2 := curExpr;
                curExpr := l4var3z;
                typ := arg1Type;
            }
        }
    }
}; (* term *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure simpleExpression;
label
    15031;
var
    finalExpr, leftExpr: eptr;
    oper: operator;
    argKind: kind;
    match: boolean;
{
    match := false;
    if (charClass IN [PLUSOP, MINUSOP]) then {
        if (charClass = MINUSOP) then
            match := true;
        inSymbol;
    };
    term;
(minus)
    if (match) then {
        arg1Type := curExpr@.typ;
        new(leftExpr);
        with leftExpr@ do {
            typ := arg1Type;
            expr1 := curExpr;
            if (arg1Type = realType) then {
                op := RNEGOP;
            } else if typeCheck(arg1Type, integerType) then {
                leftExpr@.op := INEGOP;
                leftExpr@.typ := integerType;
            } else {
                error(69); (* errUnaryMinusNeedRealOrInteger *)
                exit minus
            };
            curExpr := leftExpr;
        }
    };
    while (SY = ADDOP) do {
        oper := charClass;
        inSymbol;
        leftExpr := curExpr;
        term;
        arg1Type := curExpr@.typ;
        arg2Type := leftExpr@.typ;
        match := typeCheck(arg1Type, arg2Type);
        argKind := arg2Type@.k;
        if (kindArray <= argKind) then {
15031:      error(errNeedOtherTypesOfOperands);
        } else {
            new(finalExpr);
            with finalExpr@ do {
                if (oper = OROP) then {
                    if match and ((arg1Type = booleanType) or
                                  (arg1Type = integerType)) then {
                       typ := booleanType;
                       op := oper
                    } else goto 15031;
                } else  {
                   if (oper = SETOR) or (oper = SETSUB) then {
                       op := oper;
                       typ := arg2Type;
                   } else  if (match) then {
                        if (arg1Type = realType) then {
                            op := oper;
                            typ := realType;
                        } else if (baseType = integerType) then {
                            op := iAddOpMap[oper];
                            typ := integerType;
                        } else {
                            goto 15031
                        }
                    } else if areTypesCompatible(leftExpr) then {
                        finalExpr@.typ := realType;
                        finalExpr@.op := oper;
                    } else
                        goto 15031
                };
                finalExpr@.expr1 := leftExpr;
                finalExpr@.expr2 := curExpr;
                curExpr := finalExpr;
            }
        };
    }

}; (* simpleExpression *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parentExpression;
{
    if (readNext) then
        inSymbol;
    checkSymAndRead(LPAREN);
    readNext := false;
    expression;
    checkSymAndRead(RPAREN);
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure expression;
var
    oper: operator;
    ex1, ex2: eptr;
{
    if (readNext) then
        inSymbol
    else
        readNext := true;
    simpleExpression;
    if (SY = RELOP) then {
        oper := charClass;
        inSymbol;
        ex2 := curExpr;
        simpleExpression;
        arg1Type := curExpr@.typ;
        arg2Type := ex2@.typ;
        if typeCheck(arg1Type, arg2Type) then {
            if
               (arg1Type@.k = kindFile) or
               (arg1Type@.size <> 1) and
               (oper >= LTOP) and
               not isCharArray(arg1Type) then
                error(errNeedOtherTypesOfOperands);
        } else  {
            if not areTypesCompatible(ex2) and
               ((arg1Type <> integerType) or
               not (arg2Type@.k IN [kindScalar, kindRange]) or
               (oper <> INOP)) then {
                error(errNeedOtherTypesOfOperands);
            }
        };
        new(ex1);
        with ex1@ do {
            typ := booleanType;
            if (oper IN [GTOP, LEOP]) then {
                expr1 := curExpr;
                expr2 := ex2;
                if (oper = GTOP) then
                    op := LTOP
                else
                    op := GEOP;
            } else {
                expr1 := ex2;
                expr2 := curExpr;
                op := oper;
            };
            curExpr := ex1;
        }
    }

}; (* expression *)
procedure assignStatement(doLHS: boolean); forward;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure forStatement;
var
   toLoop, leave : integer;
   loopExpr      : eptr;
{
    inSymbol;
    checkSymAndRead(LPAREN);
    if (SY <> SEMICOLON) then {
        assignStatement(true); (* eventually: expression *)
        formOperator(gen7);
    };
    checkSymAndRead(SEMICOLON);
    padToLeft;
    toLoop := moduleOffset;
    leave := 0;
    if (SY <> SEMICOLON) then {
        readNext := false;
        expression;
        jumpTarget := 0;
        formOperator(BRANCH);
        leave := jumpTarget;
    };
    checkSymAndRead(SEMICOLON);
    loopExpr := NIL;
    if (SY <> RPAREN) then {
        assignStatement(true); (* eventually: expression *)
        loopExpr := curExpr;
    };
    checkSymAndRead(RPAREN);
    statement;
    if (loopExpr <> NIL) then {
        curExpr := loopExpr;
        formOperator(gen7);
    };
    formJump(toLoop);
    if (leave <> 0) then {
        padToLeft;
        P0715(0, leave);
    }
}; (* forStatement *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure withStatement;
var
    l4exp1z: eptr;
    l4var2z, l4var3z: bitset;
    l4var4z: integer;
{
    l4exp1z := expr63z;
    l4var4z := localSize;
    l4var2z := set147z;
    l4var3z := [];
    repeat
        inSymbol;
        if (hashTravPtr <> NIL) and
           (hashTravPtr@.cl >= VARID) then {
            parseLval;
            if (curExpr@.typ@.k = kindStruct) then {
                formOperator(SETREG);
                l4var3z := (l4var3z + [curVal.i]) * set148z;
            } else {
                error(71); (* errWithOperatorNotOfARecord *)
            };
        } else {
            error(72); (* errWithOperatorNotOfAVariable *)
        }
    until (SY <> COMMA);
    checkSymAndRead(DOSY);
    statement;
    expr63z := l4exp1z;
    localSize := l4var4z;
    set147z := l4var2z;
    set145z := set145z + l4var3z;
}; (* withStatement *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure reportStmtType(l4arg1z: integer);
{
    writeln(' STATEMENT ', stmtname:0, ' IN ', startLine:0, ' LINE');
}; (* reportStmtType *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function structBranch(isGoto: boolean): boolean;
var
    curLab: @strLabel;
{
    structBranch := true;
    if (SY = IDENT) or not isGoto then {
        curLab := strLabList;
        ii := 1;
        while (curLab <> NIL) do {
            with curLab@ do {
                if (ident.m = []) then {
                    ii := ii - 1;
                } else {
                    if (ident = curIdent) then {
                        if (ii = 1) then {
                            if (isGoto and (offset <> -1)) then {
                                form1Insn(insnTemp[UJ] + offset);
                                writeln(' goto ', offset oct);
                            } else {
                                formJump(curLab@.exitTarget);
                                writeln(' formjump ', curLab@.exitTarget);
                            };
                        } else {
                            form1Insn(getValueOrAllocSymtab(ii) +
                                      (KVTM+I13));
                            if (isGoto) then {
                                form1Insn(KVTM+I10 + curLab@.offset);
                            } else {
                                jumpType := KVTM+I10;
                                formJump(curLab@.exitTarget);
                                jumpType := insnTemp[UJ];
                            };
                            form1Insn(getHelperProc(60) +
                                      6437777713700000C); (* P/ZAM *)
                        };
                        exit
                    }
                };
                curLab := curLab@.next;
            }
        };
        if not isGoto and (SY <> IDENT) then {
            if (ii <> 1) then {
                form1Insn(getValueOrAllocSymtab(ii) + (KVTM+I13));
                form1Insn(getHelperProc(60)); (* P/ZAM *)
            };
            formJump(exitTarget);
        } else {
            error(errNotDefined);
        }
    } else
        structBranch := false;
}; (* structBranch *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure caseStatement;
label
    16211;
type
    casechain = record
        next:   @casechain;
        value:  word;
        offset: integer;
    end;
var
    allClauses, curClause, clause, unused: @casechain;
    isIntCase: boolean;
    otherSeen: boolean;
    otherOffset: integer;
    itemsEnded, goodMode: boolean;
    firstType, itemtype, exprtype: tptr;
    itemvalue: word;
    itemSpan: integer;
    expected: word;
    startLine, l4var17z, endOfStmt: integer;
    minValue, unused2, maxValue: word;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function max(a, b: integer): integer;
{
    if (b < a) then
        max := a
    else
        max := b;
}; (* max *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* caseStatement *)
    startLine := lineCnt;
    parentExpression;
    exprtype := curExpr@.typ;
    otherSeen := false;
    if (exprtype = alfaType) or
       (exprtype@.k IN [kindScalar, kindRange]) then
        formOperator(LOAD)
    else
        error(25); (* errExprNotOfADiscreteType *)
    disableNorm;
    l4var17z := 0;
    endOfStmt := 0;
    allClauses := NIL;
    formJump(l4var17z);
    checkSymAndRead(BEGINSY);
    firstType := NIL;
    goodMode := true;

    repeat
        if not (SY IN [SEMICOLON, ENDSY]) then {
            padToLeft;
            arithMode := 1;
            if (SY = DEFAULTSY) then {
                if (otherSeen) then
                    error(73); (* errCaseLabelsIdentical *)
                inSymbol;
                otherSeen := true;
                otherOffset := moduleOffset;
            } else  repeat
                parseLiteral(itemtype, itemvalue, true);
                if (itemtype <> NIL) then {
                    if (firstType = NIL) then {
                        firstType := itemtype;
                    } else {
                        if not typeCheck(itemtype, firstType) then
                            error(errConstOfOtherTypeNeeded);
                    };
                    new(clause);
                    clause@.value := itemvalue;
                    clause@.offset := moduleOffset;
                    curClause := allClauses;
(loop)              while (curClause <> NIL) do {
                        if (itemvalue = curClause@.value) then {
                            error(73); (* errCaseLabelsIdentical *)
                            exit loop
                        } else if (itemvalue.i < curClause@.value.i) then {
                            exit loop
                        } else {
                            unused := curClause;
                            curClause := curClause@.next;
                        }
                    };
                    if (curClause = allClauses) then {
                        clause@.next := allClauses;
                        allClauses := clause;
                    } else {
                        clause@.next := curClause;
                        unused@.next := clause;
                    };
                    inSymbol;
                };
                itemsEnded := (SY <> COMMA);
                if not itemsEnded then
                    inSymbol;
            until itemsEnded;
            checkSymAndRead(COLON);
            statement;
            goodMode := goodMode and (arithMode = 1);
            formJump(endOfStmt);
        };
        itemsEnded := (SY = ENDSY);
        if SY = SEMICOLON then
            inSymbol;
    until itemsEnded;
    if (SY <> ENDSY) then {
        requiredSymErr(ENDSY);
        stmtName := 'CASE  ';
        reportStmtType(startLine);
    } else
        inSymbol;
    if not typeCheck(firstType, exprtype) then {
        error(88); (* errDifferentTypesOfLabelsAndExpr *);
        exit
    };
    padToLeft;
    isIntCase := typeCheck(exprtype, integerType);
    if (allClauses <> NIL) then {
        expected := allClauses@.value;
        minValue := expected;
        curClause := allClauses;
        while (curClause <> NIL) do {
            if (expected = curClause@.value) and
               (exprtype@.k = kindScalar) then {
                maxValue := expected;
                if (isIntCase) then {
                    expected.i := expected.i + 1;
                } else {
                    curVal := expected;
                    curVal.c := succ(curVal.c);
                    expected := curVal;
                };
                curClause := curClause@.next;
            } else {
                itemSpan := 34000;
                P0715(0, l4var17z);
                if (firstType@.k = kindRange) then {
                    itemSpan := max(abs(firstType@.left),
                                    abs(firstType@.right));
                } else {
                    if (firstType@.k = kindScalar) then
                        itemSpan := firstType@.numen;
                };
                itemsEnded := (itemSpan < 32000);
                if (itemsEnded) then {
                    form1Insn(KATI+14);
                } else {
                    form1Insn(KATX+SP+1);
                };
                minValue.i := (minValue.i - minValue.i); (* WTF? *)
                while (allClauses <> NIL) do {
                    if (itemsEnded) then {
                        curVal.i := (minValue.i - allClauses@.value.i);
                        curVal.m := (curVal.m + intZero);
                        form1Insn(getValueOrAllocSymtab(curVal.i) +
                                  (KUTM+I14));
                        form1Insn(KVZM+I14 + allClauses@.offset);
                        minValue := allClauses@.value;
                    } else {
                        form1Insn(KXTA+SP+1);
                        curVal := allClauses@.value;
                        form2Insn(KAEX + I8 + getFCSToffset,
                                  insnTemp[UZA] + allClauses@.offset);
                    };
                    allClauses := allClauses@.next;
                };
                if (otherSeen) then
                    form1Insn(insnTemp[UJ] + otherOffset);
                goto 16211;
            }; (* if 16141 *)
        }; (* while 16142 *)
        if (not otherSeen) then {
            otherOffset := moduleOffset;
            formJump(endOfStmt);
        };
        P0715(0, l4var17z);
        curVal := minValue;
        P0715(-(insnTemp[U1A]+otherOffset), maxValue.i);
        curVal := minValue;
        curVal.m := (curVal.m + intZero);
        form1Insn(KATI+14);
        curVal.i := ((moduleOffset + (1)) - curVal.i);
        if (curVal.i < 40000B) then {
            curVal.i := (curVal.i - 40000B);
            curVal.i := allocSymtab([24, 29] +
                        (curVal.m * O77777));
        };
        form1Insn(KUJ+I14 + curVal.i);
        while (allClauses <> NIL) do {
            padToLeft;
            form1Insn(insnTemp[UJ] + allClauses@.offset);
            allClauses := allClauses@.next;
        };
        16211:
        P0715(0, endOfStmt);
        if (not goodMode) then
           disableNorm;

    }
}; (* caseStatement *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure assignStatement;
label
    16332;
var
    lhsExpr, assnExpr: eptr;
    indCnt: integer;
    srcType, targType: tptr;
{
    if (doLHS) then
        parseLval
    else {
        new(curExpr);
        with curExpr@ do {
            typ := hashTravPtr@.typ;
            op := GETVAR;
            id1 := hashTravPtr;
        };
        inSymbol;
    };
    checkSymAndRead(BECOMES);
    readNext := false;
    targType := curExpr@.typ;
    if (targType@.k = kindStruct) and
       (SY = LBRACK) then {
        formOperator(SETREG9);
        indCnt := 0;
        inSymbol;
        l3bool5z := false;
(indices)
        {
            if (SY = COMMA) then {
                indCnt := indCnt + 1;
                inSymbol;
            } else if (SY = RBRACK) then {
                inSymbol;
                exit indices;
            } else  {
                readNext := false;
                expression;
                curVal.i := indCnt;
                formOperator(STOREAT9);
            };
            goto indices;
        };
        curExpr := NIL;
    } else
    if (SY = SEMICOLON) and allowCompat then {
        formOperator(STORE);
        readNext := true;
        curExpr := NIL;
    } else  {
        lhsExpr := curExpr;
        expression;
        srcType := curExpr@.typ;
        if (typeCheck(targType, srcType)) then {
            if (srcType@.k = kindFile) then
                error(75) (*errCannotAssignFiles*)
            else {
                if rangeMismatch and (targType@.k = kindRange) then {
                    new(assnExpr);
                    with assnExpr@ do {
                        typ := srcType;
                        op := BOUNDS;
                        expr1 := curExpr;
                        typ2 := targType;
                    };
                    curExpr := assnExpr;
                };
16332:          new(assnExpr);
                with assnExpr@ do {
                    typ := targType;
                    op := ASSIGNOP;
                    expr1 := lhsExpr;
                    expr2 := curExpr;
                };
                curExpr := assnExpr;
            }
        } else if (targType = realType) and
            typeCheck(integerType, srcType) then {
            castToReal(curExpr);
            goto 16332;
        } else {
            error(33); (*errIllegalTypesForAssignment*)
        }
    }

}; (* assignStatement *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure ifWhileStatement;
{
    disableNorm;
    parentExpression;
    if (curExpr@.typ <> booleanType) and (curExpr@.typ <> integerType) then
        error(errBooleanNeeded)
    else {
        jumpTarget := 0;
        formOperator(BRANCH);
        ifWhlTarget := jumpTarget;
    };
    statement;
}; (* ifWhileStatement *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseData;
label
    16545;
type
    DATAREC = record case boolean of
            false: (a: packed array [0..3] of 0..4095);
            true:  (b: bitset)
        end;
var
    dsize, setcount: integer;
    l4var3z, length: integer;
    repCount: integer;
    boundary: eptr;
    l4var7z, l4var8z, l4var9z: word;
    F: file of DATAREC;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure P16432(count: integer);
var
    rec: DATAREC;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function allocDataRef(value: integer): integer;
{
    if (value >= 2048) then {
        curVal.i := value;
        allocDataRef := allocSymtab((curVal.m + [24]) * halfWord);
    } else {
        allocDataRef := value;
    }
}; (* allocDataRef *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* P16432 *)
    rec.a[0] := allocDataRef(length);
    if (FcstCnt = l4var3z) then {
        curVal := l4var8z;
        curVal.i := addCurValToFCST;
    } else {
        curVal.i := l4var3z;
    };
    rec.a[1] := allocSymtab([12,23] + curVal.m * halfWord);
    rec.a[2] := allocDataRef(count);
    if (l4var9z.i = 0) then {
        curVal := l4var7z;
        besm(ASN64+24);
        curVal := ;
    } else {
        curVal.i := allocSymtab(l4var7z.m + l4var9z.m * halfWord);
    };
    rec.a[3] := curVal.i;
    l4var9z.i := count * length + l4var9z.i;
    F@ := rec;
    put(F);
    setcount := setcount + 1;
    length := 0;
    l4var3z := FcstCnt;
}; (* P16432 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* parseData *)
    dsize := FcstCnt;
    inSymbol;
    setcount := 0;
(loop)
    repeat
        inSymbol;
        setup(boundary);
        if SY <> IDENT then {
            if SY = ENDSY then
                exit loop;
            error(errNoIdent);
            curExpr := uVarPtr;
        } else  {
            if (hashTravPtr = NIL) then {
16545:          error(errNotDefined);
                curExpr := uVarPtr;
                inSymbol;
            } else {
                if (hashTravPtr@.cl = VARID) then {
                    parseLval;
                } else goto 16545;
            }
        };
        putLeft := true;
        objBufIdx := 1;
        formOperator(SETREG9);
        if (objBufIdx <> 1) then
            error(errVarTooComplex);
        l4var7z.m := (leftInsn * [12..23]);
        l4var3z := FcstCnt;
        length := 0;
        l4var9z.i := 0;
        repeat
            expression;
            formOperator(LITINSN);
            l4var8z := curVal;
            if (SY = COLON) then {
                inSymbol;
                repCount := curToken.i;
                if (SY <> INTCONST) then {
                    error(62); (* errIntegerNeeded *)
                    repCount := 0;
                } else
                    inSymbol;
            } else
                repCount := 1;
            if (repCount <> 1) then {
                if (length <> 0) then
                    P16432(1);
                length := 1;
                P16432(repCount);
            } else {
                length := length + 1;
                if (SY = COMMA) then {
                    curVal := l4var8z;
                    toFCST;
                } else {
                    if (length <> 1) then {
                        curVal := l4var8z;
                        toFCST;
                    };
                    P16432(1);
                }
            };
        until SY <> COMMA;
        rollup(boundary);
    until SY <> SEMICOLON;
    if (SY <> ENDSY) then
        error(errBadSymbol);
    reset(F);
    while not eof(F) do {
        write(FCST, F@.b);
        get(F);
    };
    int92z := FcstCnt - dsize;
    FcstCnt := dsize;
    int93z := setcount;

}; (* parseData *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure standProc;
label
    17753, 44;
var
    l4typ1z, l4typ2z, l4typ3z: tptr;
    firstWidth, secondWidth: eptr;
    l4exp6z: eptr;
    l4exp7z, l4exp8z, workExpr: eptr;
    l4bool10z,
    noWidth, l4bool12z: boolean;
    l4var13z: word;
    oldOffset: integer;
    defWidth: integer;
    procNo: integer;
    helperNo: integer;
    opToForm: opgen;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure verifyType(l5arg1z: tptr);
{
    if (hashTravPtr <> NIL) and
       (hashTravPtr@.cl >= VARID) then {
        parseLval;
        if (l5arg1z <> NIL) and
           not typeCheck(l5arg1z, curExpr@.typ) then
            error(errNeedOtherTypesOfOperands);
    } else {
        error(errNotDefined);
        curExpr := uVarPtr;
    }
}; (* verifyType *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure startReadOrWrite(doWrite: boolean);
{
    expression;
    l4typ3z := curExpr@.typ;
    l4exp7z := curExpr;
    if not (doWrite) then {
        if not (curExpr@.op IN lvalOpSet) then
            error(27); (* errExpressionWhereVariableExpected *)
    };
    if (workExpr = NIL) then {
        if (l4typ3z@.k = kindFile) then {
            workExpr := curExpr;
        } else {
            new(workExpr);
            workExpr@.typ := textType;
            workExpr@.op := GETVAR;
            if (doWrite) then {
                workExpr@.id1 := outputFile;
            } else {
                if (inputFile <> NIL) then
                    workExpr@.id1 := inputFile
                else {
                    error(37); (* errInputMissingInProgramHeader *)
                }
            }
        };
        arg2Type := workExpr@.typ;
        l4var13z.b := typeCheck(arg2Type@.base, charType);
        l4bool12z := true;
        new(l4exp8z);
        l4exp8z@.typ := arg2Type@.base;
        l4exp8z@.op := FILEPTR;
        l4exp8z@.expr1 := workExpr;
        new(l4exp6z);
        l4exp6z@.typ := l4exp8z@.typ;
        l4exp6z@.op := ASSIGNOP;
        if (doWrite) then
            l4exp6z@.expr1 := l4exp8z
        else
            l4exp6z@.expr2 := l4exp8z;
    }
}; (* startReadOrWrite *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function parseWidthSpecifier: eptr;
{
    expression;
    if not typeCheck(integerType, curExpr@.typ) then {
        error(14); (* errExprIsNotInteger *)
        parseWidthSpecifier := uVarPtr;
    } else
        parseWidthSpecifier := curExpr;
}; (* parseWidthSpecifier *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure callHelperWithArg;
{
    if ([12] <= set145z) or l4bool12z then {
        curExpr := workExpr;
        formOperator(gen8);
    };
    l4bool12z := false;
    formAndAlign(getHelperProc(helperNo));
    disableNorm;
}; (* callHelperWithArg *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure P17037;
{
    set145z := set145z - [12];
    if (helperNo <> 49) and             (* P/RDC *)
       not typeCheck(l4exp8z@.typ, l4exp7z@.typ) then
        error(34) (* errTypeIsNotAFileElementType *)
    else {
        if (helperNo = 29) then {       (* P/PF *)
            l4exp6z@.expr2 := l4exp7z;
        } else {
            if (helperNo = 49) then
                helperNo := 30;         (* P/GF *)
            l4exp6z@.expr1 := l4exp7z;
        };
        curExpr := l4exp6z;
        formOperator(gen7);
        callHelperWithArg;
    }
}; (* P17037 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure checkElementForReadWrite;
var
    l5typ1z: tptr;
{
    set145z := set145z - [12];
    if (l4typ3z@.k = kindRange) then
        l4typ3z := l4typ3z@.base;
    curVarKind := l4typ3z@.k;
    helperNo := 36;                   (* P/WI *)
    if (l4typ3z = integerType) then
        defWidth := 10
    else if (l4typ3z = realType) then {
        helperNo := 37;               (* P/WR *)
        defWidth := 14;
    } else if (l4typ3z = charType) then {
        helperNo := 38;               (* P/WC *)
        defWidth := 1;
    } else if (curVarKind = kindScalar) then {
        helperNo := 41;               (* P/WX *)
        dumpEnumNames(l4typ3z);
        defWidth := 8;
    } else if (isCharArray(l4typ3z)) then {
        l5typ1z := l4typ3z@.range;
        defWidth := l5typ1z@.right - l5typ1z@.left + 1;
        if not (l4typ3z@.pck) then
            helperNo := 81            (* P/WA *)
        else if (6 >= defWidth) then
            helperNo := 39            (* P/A6 *)
        else
            helperNo := 40;           (* P/A7 *)
    } else if (l4typ3z@.size = 1) then {
        helperNo := 42;               (* P/WO *)
        defWidth := 17;
    } else {
        error(34); (* errTypeIsNotAFileElementType *)
    }
}; (* checkElementForReadWrite *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure writeProc;
{
    workExpr := NIL;
    l4var13z.b := true;
    repeat {
        startReadOrWrite(true);
        if (l4exp7z <> workExpr) then {
            if not (l4var13z.b) then {
                helperNo := 29;         (* P/PF *)
                P17037;
            } else {
                checkElementForReadWrite;
                secondWidth := NIL;
                firstWidth := NIL;
                if (SY = COLON) then
                    firstWidth := parseWidthSpecifier;
                if (SY = COLON) then {
                    secondWidth := parseWidthSpecifier;
                    if (helperNo <> 37) then    (* P/WR *)
                        error(35); (* errSecondSpecifierForWriteOnlyForReal *)
                } else {
                    if (curToken = litOct) then {
                        helperNo := 42; (* P/WO *)
                        defWidth := 17;
                        if (l4typ3z@.size <> 1) then
                            error(34); (* errTypeIsNotAFileElementType *)
                        inSymbol;
                    }
                };
                noWidth := false;
                if (firstWidth = NIL) and
                   (helperNo IN [38,39,40]) then {  (* WC,A6,A7 *)
                    helperNo := helperNo + 5;       (* CW,6A,7A *)
                    noWidth := true;
                } else {
                    if (firstWidth = NIL) then {
                        curVal.i := defWidth;
                        formOperator(DFLTWDTH);
                    } else {
                        curExpr := firstWidth;
                        formOperator(LOAD);
                    }
                };
                if (helperNo = 37) then {       (* P/WR *)
                    if (secondWidth = NIL) then {
                        curVal.i := 4;
                        form1Insn(KXTS+I8 + getFCSToffset);
                    } else {
                        curExpr := secondWidth;
                        formOperator(FRACWIDTH);
                    }
                };
                curExpr := l4exp7z;
                if (noWidth) then {
                    if (helperNo = 45) then     (* P/7A *)
                        opToForm := gen11
                    else
                        opToForm := LOAD;
                } else {
                    if (helperNo = 40) or       (* P/A7 *)
                       (helperNo = 81) then     (* P/WA *)
                        opToForm := gen12
                    else
                        opToForm := FRACWIDTH;
                };
                formOperator(opToForm);
                if (helperNo IN [39,40,44,45]) or (* A6,A7,6A,7A *)
                   (helperNo = 81) then
                    form1Insn(KVTM+I10 + defWidth)
                else {
                    if (helperNo = 41) then (* P/WX *)
                        form1Insn(KVTM+I11 + l4typ3z@.start);
                };
                callHelperWithArg;
            }
        }
    } until (SY <> COMMA);
    if (procNo = 11) then {
        helperNo := 46;                 (* P/WL *)
        callHelperWithArg;
    };
    set145z := set145z + [12];
    if (oldOffset = moduleOffset) then
        error(36); (*errTooFewArguments *)
}; (* writeProc *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure checkArrayArg;
{
    verifyType(NIL);
    workExpr := curExpr;
    l4typ1z := curExpr@.typ;
    if (l4typ1z@.pck) or
       (l4typ1z@.k <> kindArray) then
        error(errNeedOtherTypesOfOperands);
    checkSymAndRead(COMMA);
    readNext := false;
    expression;
    l4exp8z := curExpr;
    if not typeCheck(l4typ1z@.range, l4exp8z@.typ) then
        error(errNeedOtherTypesOfOperands);
}; (* checkArrayArg *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure doPackUnpack;
var
    t: tptr;
{
    new(l4exp7z);
    l4exp7z@.typ := l4typ1z@.base;
    l4exp7z@.op := GETELT;
    l4exp7z@.expr1 := workExpr;
    l4exp7z@.expr2 := l4exp8z;
    t := l4exp6z@.typ;
    if (t@.k <> kindArray) or
       not t@.pck or
       not typeCheck(t@.base, l4typ1z@.base) or
       not typeCheck(l4typ1z@.range, t@.range) then
        error(errNeedOtherTypesOfOperands);
    new(curExpr);
    curExpr@.val.c := chr(procNo + 50);
    curExpr@.expr1 := l4exp7z;
    curExpr@.expr2 := l4exp6z;
    formOperator(PCKUNPCK);
}; (* doPackUnpack *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* standProc *)
    curVal.i := l3idr12z@.low;
    procNo := curVal.i;
    l4bool10z := (SY = LPAREN);
    oldOffset := moduleOffset;
    if not l4bool10z and
       (procNo IN [0:5,8:10,12,16:28]) then
        error(45); (* errNoOpenParenForStandProc *)
    if (procNo IN [0,1,2,3,4,5,8,9]) then {
        inSymbol;
        if (hashTravPtr@.cl < VARID) then
            error(46); (* errNoVarForStandProc *)
        parseLval;
        arg1Type := curExpr@.typ;
        curVarKind := arg1Type@.k;
    };
    if (procNo IN [0..6]) then
        jumpTarget := getHelperProc(29 + procNo); (* P/PF *)
    case procNo of
    0, 1, 2, 3: { (* put, get, rewrite, reset *)
        if (curVarKind <> kindFile) then
            error(47); (* errNoVarOfFileType *)
        if (procNo = 3) and
           (SY = COMMA) then {
            formOperator(gen8);
            expression;
            if (not typeCheck(integerType, curExpr@.typ)) then
                error(14); (* errExprIsNotInteger *)
            formOperator(LOAD);
            formAndAlign(getHelperProc(97)); (*"P/RE"*)
        } else {
            formOperator(FILEACCESS);
        }
    };
    4, 5: { (* new, free *)
        if (curVarKind <> kindPtr) then
            error(13); (* errVarIsNotPointer *)
        heapCallsCnt := heapCallsCnt + 1;
        workExpr := curExpr;
        if (procNo = 5) then
            formOperator(SETREG9);
        l2typ13z := arg1Type@.base;
        ii := l2typ13z@.size;
        if (charClass = EQOP) then {
            expression;
            if not typeCheck(integerType, curExpr@.typ) then
                error(14); (* errExprIsNotInteger *)
            if (curExpr@.op = GETENUM) then {
                ii := curExpr@.num1; goto 44;
            } else {
                formOperator(LOAD);
                form1Insn(KATI+14);
            }
        } else {
            if (arg1Type@.base@.k = kindStruct) then {
                l4typ1z := l2typ13z@.base;
(loop)          while (SY = COMMA) and (l4typ1z <> NIL) do {
                    with l4typ1z@ do
                        ; (* useless *)
                    inSymbol;
                    parseLiteral(l4typ2z, curVal, true);
                    if (l4typ2z = NIL) then
                        exit loop
                    else {
                        inSymbol;
(loop2)                 while (l4typ1z <> NIL) do {
                            l4typ2z := l4typ1z;
                            while (l4typ2z <> NIL) do {
                                if (l4typ2z@.sel = curVal) then {
                                    ii := l4typ1z@.size;
                                    exit loop2;
                                };
                                l4typ2z := l4typ2z@.r6;
                            };
                            l4typ1z := l4typ1z@.next;
                        }
                    }
                }
            };
44:         form1Insn(KVTM+I14+getValueOrAllocSymtab(ii));
        };
        formAndAlign(jumpTarget);
        if (procNo = 4) then {
            curExpr := workExpr;
            formOperator(STORE);
        }
    };
    6: { (* halt *)
        formAndAlign(jumpTarget);
        exit
    };
    7: { (* stop *)
        form1Insn(KE74);
        exit
    };
    8, 9: { (* setup, rollup *)
        if (curVarKind <> kindPtr) then
            error(13); (* errVarIsNotPointer *)
        if (procNo = 8) then {
            form1Insn(KXTA+HEAPPTR);
            formOperator(STORE);
        } else {
            formOperator(LOAD);
            form1Insn(KATX+HEAPPTR);
        }
    };
    10: { (* write *)
        writeProc;
    };
    11:
17753: { (* writeln *)
        if (SY = LPAREN) then {
            writeProc;
        } else {
            formAndAlign(getHelperProc(54)); (*"P/WOLN"*)
            exit
        }
    };
    14: { (* exit *)
        l4bool10z := (SY = LPAREN);
        if (l4bool10z) then
            inSymbol;
        if (SY = IDENT) then {
            if not structBranch(false) then
                error(1); (* errCommaOrSemicolonNeeded *)
            inSymbol;
        } else {
            formJump(exitTarget);
        };
        if not (l4bool10z) then
            exit
    };
    15: { (* debug *)
        if (debugPrint IN optSflags.m) then {
            procNo := 11;
            goto 17753;
        };
        while (SY <> RPAREN) do
            inSymbol;
    };
    16: { (* besm *)
        expression;
        formOperator(LITINSN);
        formAndAlign(curVal.i);
    };
    18: { (* mapai *)
        l4typ1z := alfaType;
        l4typ2z := integerType;
        expression;
        if not typeCheck(curExpr@.typ, l4typ1z) then
            error(errNeedOtherTypesOfOperands);
        checkSymAndRead(COMMA);
        formOperator(LOAD);
        if (procNo = 17) then {
            form3Insn(ASN64-33, KAUX+BITS15, KAEX+ASCII0);
        } else {
            form3Insn(KAPX+BITS15, ASN64+33, KAEX+ZERO);
        };
        verifyType(l4typ2z);
        formOperator(STORE);
    };
    19, 20: { (* pck, unpck *)
        inSymbol;
        verifyType(charType);
        checkSymAndRead(COMMA);
        formOperator(gen8);
        verifyType(alfaType);
        if (procNo = 20) then {
            formOperator(LOAD);
        };
        formAndAlign(getHelperProc(procNo - 6));
        if (procNo = 19) then
            formOperator(STORE);
    };
    21: { (* pack *)
        inSymbol;
        checkArrayArg;
        checkSymAndRead(COMMA);
        verifyType(NIL);
        l4exp6z := curExpr;
        doPackUnpack;
    };
    22: { (* unpack *)
        inSymbol;
        verifyType(NIL);
        l4exp6z := curExpr;
        checkSymAndRead(COMMA);
        checkArrayArg;
        doPackUnpack;
    };
    end;
    if procNo in [0,1,2,3,5,10,11,12,13,21,22] then
        arithMode := 1;
    checkSymAndRead(RPAREN);

}; (* standProc *)
procedure setStrLab(forGoto: boolean);
{
    new(strLabPtr);
    padToLeft;
    disableNorm;
    with strLabPtr@ do {
        next := strLabList;
        ident := curIdent;
        if forGoto then offset := moduleOffset else offset := -1;
        exitTarget := 0;
    };
    strLabList := strLabPtr;
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* statement *)
    if SY = SEMICOLON then {
        inSymbol;
        exit; (* empty statement *)
    };
    setup(boundary);
    bool110z := false;
    startLine := lineCnt;
    if set147z = halfWord then
        parseData
    else {
        if SY = INTCONST then {
            set146z := [];
            numLabPtr := numLabList;
            disableNorm;
            l3bool5z := true;
            padToLeft;
            (loop) if numLabPtr <> labFence then with numLabPtr@ do {
                if id <> curToken then {
                    numLabPtr := next;
                    goto loop;
                };
                l3bool5z := false;
                if (defined) then {
                    curVal.i := line;
                    error(17); (* errLblAlreadyDefinedInLine *);
                } else {
                    numLabPtr@.line := lineCnt;
                    numLabPtr@.defined := true;
                    padToLeft;
                    if numLabPtr@.offset = 0 then {
                        (* empty *)
                    } else if (numLabPtr@.offset >= 74000B) then {
                        curVal.i := (moduleOffset - 40000B);
                        symTab[numLabPtr@.offset] := [24,29] +
                                                     curVal.m * O77777;
                    } else {
                        P0715(0, numLabPtr@.offset);
                    };
                    numLabPtr@.offset := moduleOffset;
                };
            };
            if (l3bool5z) then
                error(16); (* errLblNotDefinedInBlock *);
            inSymbol;
            checkSymAndRead(COLON);
        }; (* 20355*)
        l3var4z := (SY IN [BEGINSY,SWITCHSY]);
        if (l3var4z) then
            lineNesting := lineNesting + 1;
(ident)
        if SY = IDENT then {
            if hashTravPtr <> NIL then {
                l3var6z := hashTravPtr@.cl;
                if l3var6z >= VARID then {
                    assignStatement(true);
                } else {
                    if l3var6z = ROUTINEID then {
                        if hashTravPtr@.typ = NIL then {
                            l3idr12z := hashTravPtr;
                            inSymbol;
                            if l3idr12z@.offset = 0 then {
                                standProc;
                                checkSymAndRead(SEMICOLON);
                                exit ident;
                            };
                            parseCallArgs(l3idr12z);
                        } else {
                            assignStatement(false);
                        };
                    } else {
                        error(32); (* errWrongStartOfOperator *)
                        goto 8888;
                    }
                };
                formOperator(gen7);
                checkSymAndRead(SEMICOLON);
            } else {
                error(errNotDefined);
8888:           skip(skipToSet + statEndSys);
            };
        } else  if (SY = LPAREN) then {
            set146z := [];
            inSymbol;
            if (SY <> IDENT) then {
                error(errNoIdent);
                goto 8888;
            };
            setStrLab(true);
            inSymbol;
            checkSymAndRead(RPAREN);
            statement;
            P0715(0, strLabPtr@.exitTarget);
            strLabList := strLabList@.next;
        } else  if (SY = BEGINSY) then
(rep)   {
            inSymbol;
(skip)      {
                while SY <> ENDSY do statement;
                if (SY <> ENDSY) then {
                    stmtName := ' BEGIN';
                    requiredSymErr(SEMICOLON);
                    reportStmtType(startLine);
                    skip(bigSkipSet);
                    if (SY IN statBegSys) then
                        goto skip;
                    if (SY <> SEMICOLON) then
                        exit rep;
                    goto rep;
                };
            };
            inSymbol;
        } else  if (SY = GOTOSY) then {
            inSymbol;
            if (SY <> INTCONST) then {
                if structBranch(true) then {
                    inSymbol;
                    exit;
                } else
                    goto 8888;
            };
            disableNorm;
            numLabPtr := numLabList;
(loop)      if (numLabPtr <> NIL) then with numLabPtr@ do {
                if (id <> curToken) then {
                    numLabPtr := next;
                } else {
                    if (curFrameRegTemplate = frame) then {
                        if (offset >= 40000B) then {
                            form1Insn(insnTemp[UJ] + offset);
                        } else {
                            formJump(offset);
                        }
                    } else {
                        if offset = 0 then {
                            offset := symTabPos;
                            putToSymTab([]);
                        };
                        form3Insn(frame + (KMTJ + 13), KVTM+I14 + offset,
                                  getHelperProc(18(*"P/RC"*)) + (-64100000B));
                    };
                    exit loop;
                };
                goto loop;
            } else
                error(18); (* errLblNotDefined *)
            inSymbol;
        } else  if (SY = IFSY) then {
            ifWhileStatement;
            if (SY = ELSESY) then {
                elseJump := 0;
                formJump(elseJump);
                P0715(0, ifWhlTarget);
                curOffset.i := arithMode;
                arithMode := 1;
                inSymbol;
                statement;
                P0715(0, elseJump);
                if (curOffset.i <> arithMode) then {
                    arithMode := 2;
                    disableNorm;
                }
            } else {
                P0715(0, ifWhlTarget);
            }
        } else  if (SY = WHILESY) then {
            set146z := [];
            curIdent.i := 4262454153C;
            setStrLab(false); (* break *)
            curIdent.i := 4357566451566545C;
            setStrLab(true); (* continue *)
            curOffset.i := moduleOffset;
            ifWhileStatement;
            disableNorm;
            form1Insn(insnTemp[UJ] + curOffset.i);
            P0715(0, ifWhlTarget);
            strLabList := strLabList@.next; (* removing continue *)
            P0715(0, strLabList@.exitTarget); (* assigning target for break *)
            strLabList := strLabList@.next; (* removing break *)
            arithMode := 1;
        } else if (SY = BREAKSY) then {
            SY := IDENT;
            if not structBranch(false) then goto 8888;
            inSymbol;
        } else if (SY = CONTSY) then {
            SY := IDENT;
            if not structBranch(true) then goto 8888;
            inSymbol;
        } else  if (SY = DOSY) then {
            set146z := [];
            curIdent.i := 4262454153C;
            setStrLab(false); (* break *)
            curIdent.i := 4357566451566545C;
            setStrLab(false); (* continue *)
            curOffset.i := moduleOffset;
            inSymbol;
            statement;
            (* assigning target for continue if used *)
            with strLabList@ do if exitTarget <> 0 then {
                 P0715(0, exitTarget);
writeln(' target for ', strLabList@.exitTarget, ' is ', moduleOffset oct);
                                                        };
            strLabList := strLabList@.next; (* removing continue *)
            if (SY <> WHILESY) then {
                requiredSymErr(WHILESY);
                stmtName := '  DO  ';
                reportStmtType(startLine);
                goto 8888;
            };
            disableNorm;
            parentExpression;
            if (curExpr@.typ<>booleanType)and(curExpr@.typ<>integerType) then {
                error(errBooleanNeeded)
            } else {
                jumpTarget := curOffset.i;
                whileExpr := curExpr;
                new(curExpr);
                with curExpr@do {
                    typ := booleanType;
                    op := NOTOP;
                    expr1 := whileExpr;
                };
                formOperator(BRANCH);
            };
            with strLabList@ do if exitTarget <> 0 then {
            P0715(0, exitTarget); (* assigning target for break *)
writeln(' target for ', strLabList@.exitTarget, ' is ', moduleOffset oct);
};
            strLabList := strLabList@.next; (* removing break *)
        } else
        if (SY = FORSY) then {
            set146z := [];
            forStatement;
        } else  if (SY = SWITCHSY) then {
            caseStatement
        } else if (SY = WITHSY) then {
            withStatement;
        };
        if (l3var4z) then
            lineNesting := lineNesting - 1;
        rollup(boundary);
        if (bool110z) then {
            bool110z := false;
            goto 8888;
        }
    }

}; (* statement *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure outputObjFile;
var
    idx: integer;
{
    padToLeft;
    objBufIdx := objBufIdx - 1;
    for idx to objBufIdx do
        write(CHILD, objBuffer[idx]);
    lineStartOffset := moduleOffset;
    prevOpcode := 0;
}; (* outputObjFile *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure defineRoutine;
var
    l3var1z, l3var2z, l3var3z: word;
    l3int4z: integer;
    l3idr5z: irptr;
    l3var6z, l3var7z: word;
{
    objBufIdx := 1;
    objBuffer[objBufIdx] := [];
    curInsnTemplate := insnTemp[XTA];
    bool48z := 22 IN l2idr2z@.flags;
    lineStartOffset := moduleOffset;
    l3var1z := ;
    int92z := 2;
    expr63z := NIL;
    arithMode := 1;
    set146z := [];
    set147z := [curProcNesting+1..6];
    set148z := set147z - [minel(set147z)];
    l3var7z.m := set147z;
    exitTarget := 0;
    set145z := [1:15] - set147z;
    if (curProcNesting <> 1) then
        parseDecls(2);
    l2int21z := localSize;
    if (SY <> BEGINSY) then
        requiredSymErr(BEGINSY);
    if 23 IN l2idr2z@.flags then {
        l3idr5z := l2idr2z@.argList;
        l3int4z := 3;
        if (l2idr2z@.typ <> NIL) then
        l3int4z := 4;
        while (l3idr5z <> l2idr2z) do {
            if (l3idr5z@.cl = VARID) then {
                l3var2z.i := l3idr5z@.typ@.size;
                if (l3var2z.i <> 1) then {
                    form3Insn(KVTM+I14 + l3int4z,
                              KVTM+I12 + l3var2z.i,
                              KVTM+I11 + l3idr5z@.value);
                    formAndAlign(getHelperProc(73)); (* "P/LNGPAR" *)
                }
            };
            l3int4z := l3int4z + 1;
            l3idr5z := l3idr5z@.list;
        }
    };
    if checkBounds or not (NoStackCheck IN optSflags.m) then
        P0715(-1, 95); (* P/SC *)
    l3var2z.i := lineNesting;
    repeat
        statement;
        if (curProcNesting = 1) then
            done := SY = PERIOD
        else
            done := (SY IN blockBegSys);
        if not done then
           if (curProcNesting = 1) then
               requiredSymErr(PERIOD)
           else {
               errAndSkip(errBadSymbol, skipToSet);
           }
    until done;
    l2idr2z@.flags := (set145z * [0:15]) + (l2idr2z@.flags - l3var7z.m);
    lineNesting := l3var2z.i - 1;
    if (exitTarget <> 0) then
        P0715(0, exitTarget);
    if not bool48z and not doPMD and (l2int21z = 3) and
       (curProcNesting <> 1) and (set145z * [1:15] <> [1:15]) then {
        objBuffer[1] := [7:11,21:23,28,31]; (* ,NTR,7; ,UTC, *)
        with l2idr2z@ do
            flags := flags + [25];
        if (objBufIdx = 2) then {
            objBuffer[1] := [0,1,3:5]; (* 13,UJ, *)
            putLeft := true;
        } else {
            l2idr2z@.pos := l3var1z.i;
            if 13 IN set145z then {
                curVal.i := minel([1:15] - set145z);
                besm(ASN64-24);
                l3var7z := ;
                objBuffer[2] := objBuffer[2] + [0,1,3,6,9] + l3var7z.m;
            } else {
                curVal.i := (13);
            };
            form1Insn(insnTemp[UJ] + indexreg[curVal.i]);
        }
    } else  {
        if (l2int11z = 0) then
            jj := 27    (* P/E *)
        else
            jj := 28;   (* P/EF *)
        form1Insn(getHelperProc(jj) + (-I13-100000B));
        if (curProcNesting = 1) then {
            parseDecls(2);
            if S3 IN optSflags.m then
                formAndAlign(getHelperProc(78)); (* "P/PMDSET" *)
            form1Insn(insnTemp[UJ] + l3var1z.i);
            curVal.i := l2idr2z@.pos - 40000B;
            symTab[74002B] := [24,29] + (curVal.m * halfWord);
        };
        curVal.i := l2int21z;
        if (curProcNesting <> 1) then {
            curVal.i := curVal.i - 2;
            l3var7z := curVal;
            besm(ASN64-24);
            l3var7z := ;
            objBuffer[savedObjIdx] := objBuffer[savedObjIdx] +
                                       l3var7z.m + [0,1,2,3,4,6,8];
        }
    };
    outputObjFile;
}; (* defineRoutine *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure initScalars;
var
    l3var1z, l3var3z, l3var4z: word;
    l3var5z, l3var6z: integer;
    l3var7z: irptr;
    l3var8z, l3var9z: integer;
    temptype: tptr;
    l3var11z: word;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure regSysType(l4arg1z:integer; l4arg2z: tptr);
{
    new(curIdRec = 5);
    curIdRec@ := [l4arg1z, 0, , l4arg2z, TYPEID];
    addToHashTab(curIdRec);
}; (* regSysType *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure regSysEnum(l4arg1z: integer; l4arg2z: integer);
{
    new(curIdRec = 7);
    curIdRec@ := [l4arg1z, 48, , temptype, ENUMID, NIL, l4arg2z];
    addToHashTab(curIdRec);
}; (* regSysEnum *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure regSysProc(l4arg1z: integer);
{
    new(curIdRec = 6);
    curIdRec@ := [l4arg1z, 0, , temptype, ROUTINEID, l3var9z];
    l3var9z := l3var9z + 1;
    addToHashTab(curIdRec);
}; (* registerSysProc *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* initScalars *)
    new(booleanType, kindScalar);
    with booleanType@ do {
        size := 1;
        bits := 1;
        k := kindScalar;
        numen := 2;
        start := 0;
        enums := NIL;
    };
    new(integerType, kindScalar);
    with integerType@ do {
        size := 1;
        bits := 48;
        k := kindScalar;
        numen := 100000;
        start := -1;
        enums := NIL;
    };
    new(charType, kindScalar);
    with charType@ do {
        size := 1;
        bits := (8);
        k := kindScalar;
        numen := 256;
        start := -1;
        enums := NIL;
    };
    new(realType, kindReal);
    with realType@ do {
        size := 1;
        bits := 48;
        k := kindReal;
    };
    new(pointerType, kindPtr);
    with pointerType@ do {
        size := 1;
        bits := 48;
        k := kindPtr;
        base := pointerType;
    };
    new(textType, kindFile);
    with textType@ do {
        size := 30;
        bits := 48;
        k := kindFile;
        base := charType;
        elsize := 8;
    };
    new(alfaType,kindArray);
    with alfaType@ do {
        size := 1;
        bits := 48;
        k := kindArray;
        base := charType;
        range := temptype;
        pck := true;
        perword := 6;
        pcksize := 8;
    };
    smallStringType[6] := alfaType;
    regSysType(515664C  (*"     INT"*), integerType);
    regSysType(43504162C(*"    CHAR"*), charType);
    regSysType(62454154C(*"    REAL"*), realType);
    regSysType(41544641C(*"    ALFA"*), alfaType);
    regSysType(64457064C(*"    TEXT"*), textType);
    tempType := pointerType;
    regSysEnum(565154C(*"     NIL"*), (74000C));
    maxSmallString := 0;
    for strLen := 2 to 5 do
        makeStringType(smallStringType[strLen]);
    maxSmallString := 6;
    new(curIdRec = 7);
    with curIdRec@ do {
        offset := 0;
        typ := integerType;
        cl := VARID;
        list := NIL;
        value := 7;
    };
    new(uVarPtr);
    with uVarPtr@ do {
        typ := integerType;
        op := GETVAR;
        id1 := curIdRec;
    };
    new(uProcPtr, 12);
    with uProcPtr@ do {
        typ := NIL;
        list := NIL;
        argList := NIL;
        preDefLink := NIL;
        pos := 0;
    };
    temptype := NIL;
    l3var9z := 0;
    for l3var5z := 0 to 22 do
        regSysProc(systemProcNames[l3var5z]);
    l3var9z := 0;
    temptype := realType;
    regSysProc(63616264C(*"    SQRT"*));
    regSysProc(635156C(*"     SIN"*));
    regSysProc(435763C(*"     COS"*));
    regSysProc(416243644156C(*"  ARCTAN"*));
    regSysProc(416243635156C(*"  ARCSIN"*));
    regSysProc(5456C(*"      LN"*));
    regSysProc(457060C(*"     EXP"*));
    regSysProc(414263C(*"     ABS"*));
    temptype := integerType;
    regSysProc(6462655643C(*"   TRUNC"*));
    regSysProc(574444C(*"     ODD"*));
    regSysProc(576244C(*"     ORD"*));
    temptype := charType;
    regSysProc(435062C(*"     CHR"*));
    regSysProc(63654343C(*"    SUCC"*));
    regSysProc(60624544C(*"    PRED"*));
    temptype := integerType;
    regSysProc(455746C(*"     EOF"*));
    temptype := pointerType;
    regSysProc(624546C(*"     REF"*));
    temptype := integerType;
    regSysProc(45575456C(*"    EOLN"*));
    regSysProc(0C); (* was SQR, unused *)
    regSysProc(6257655644C(*"   ROUND"*));
    regSysProc(43416244C(*"    CARD"*));
    regSysProc(5551564554C(*"   MINEL"*));
    temptype := pointerType;
    regSysProc(606462C(*"     PTR"*));
    l3var11z.i := 30;
    l3var11z.m := l3var11z.m * halfWord + [24,27,28,29];
    new(programObj, 12);
    curVal.i := 1257656460656412C(*"*OUTPUT*"*);
    l3var3z := curVal;
    curVal.i := 12515660656412C(*" *INPUT*"*);
    l3var4z := curVal;
    test1(EXTERNSY, (skipToSet + [IDENT,SEMICOLON]));
    symTabPos := 74004B;
    with programObj@ do {
            curVal.i := 6041634357556054C; (* PASCOMPL *)
            id := ;
            pos := 0;
            symTab[74000B] := makeNameWithStars;
    };
    entryPtTable[1] := symTab[74000B];
    entryPtTable[3] :=
        [0,1,6,7,10,12,14:18,21:25,28,30,35,36,38,39,41];(*"PROGRAM "*)
    entryPtTable[2] := [1];
    entryPtTable[4] := [1];
    entryPtCnt := 5;
    write(CHILD, [0,4,6,9:12,23,28,29,33:36,46]);(*10 24 74001 00 30 74002*)
    moduleOffset := 40001B;
    programObj@.argList := NIL;
    programObj@.flags := [];
    objBufIdx := 1;
    temptype := integerType;
    defineRange(temptype, 1, 6);
    alfaType@.range := temptype;
    int93z := 0;
    outputObjFile;
    outputFile := NIL;
    inputFile := NIL;
    externFileList := NIL;
    new(l3var7z, 12);
    lineStartOffset := moduleOffset;
    with l3var7z@ do {
        id := l3var3z;
        offset := 0;
        typ := textType;
        cl := VARID;
        list := NIL;
    };
    curVal.i := 1257656460656412C(*"*OUTPUT*"*);
    l3var7z@.value := allocExtSymbol(l3var11z.m);
    addToHashTab(l3var7z);
    l3var5z := 1;
    while SY = IDENT do {
        l3var8z := 0;
        curVal := curIdent;
        l3var1z.m := makeNameWithStars;
        if (curIdent = l3var4z) then {
            new(inputFile, 12);
            with inputFile@ do {
                id := curIdent;
                offset := 0;
                typ := textType;
                cl := VARID;
                list := NIL;
            };
            curVal := l3var1z;
            inputFile@.value := allocExtSymbol(l3var11z.m);
            addToHashTab(inputFile);
            l3var8z := lineCnt;
        } else if (curIdent = l3var3z) then {
            outputFile := l3var7z;
            l3var8z := lineCnt;
        };
        curExternFile := externFileList;
        while (curExternFile <> NIL) do {
            if (curExternFile@.id = curIdent) then {
                curExternFile := NIL;
                error(errIdentAlreadyDefined);
            } else {
                curExternFile := curExternFile@.next;
            };
        };
        new(curExternFile);
        with curExternFile@ do {
            id := curIdent;
            next := externFileList;
            line := l3var8z;
            offset := l3var1z.i;
        };
        if l3var8z <> 0 then {
            if (curIdent = l3var3z) then {
                fileForOutput := curExternFile;
            } else {
                fileForInput := curExternFile;
            }
        };
        externFileList := curExternFile;
        l3var6z := l3var5z;
        l3var5z := l3var5z + 1;
        inSymbol;
        if (charClass = MUL) then {
            l3var6z := l3var6z + 64;
            inSymbol;
        };
        if (SY = INTCONST) then {
            l3var6z := 1000B * curToken.i + l3var6z;
            if (numFormat = DECIMAL) and
               (1 < curToken.i) and
               (curToken.i < 127) then {
                l3var6z := l3var6z + 128;
            } else if (numFormat = OCTAL) and
                      (1000000B < curToken.i) and
                      (curToken.i < 1743671743B) then {
                l3var6z := l3var6z + 256;
            } else {
                error(76); (* errWrongNumberForExternalFile *)
            };
            inSymbol;
        } else {
            l3var6z := 512;
        };
        curExternFile@.location := l3var6z;
        if (SY = COMMA) then
            inSymbol;
    };
    checkSymAndRead(SEMICOLON);
    if (outputFile = NIL) then {
        error(77); (* errNoOutput *)
        outputFile := l3var7z;
    };
    l3var6z := 40;
    repeat
        programme(l3var6z, programObj);
    until (SY = PERIOD);
    if (CH <> 'D') then {
        int92z := 0;
        int93z := ;
    } else {
        set147z := halfWord;
        dataCheck := false;
        statement;
    };
    readToPos80;
    curVal.i := l3var6z;
    symTab[74003B] := (helperNames[25] + [24,27,28,29]) +
                        (curVal.m * halfWord);
}; (* initScalars *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure makeExtFile;
{
    new(l2var10z);
    with l2var10z@ do {
        typ := ptr(ord(curExternFile));
        id2 := workidr;
        expr1 := curExpr;
    };
    curExpr := l2var10z;
}; (* makeExtFile *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseParameters;
var
    l3var1z, l3var2z, l3var3z: irptr;
    parClass: idclass;
    l3var5z, l3var6z: integer;
    l3sym7z: symbol;
    noComma: boolean;
    expType: tptr;
{
    int92z := 0;
    l3var5z := 0;
    int93z := 0;
    inSymbol;
    l3var2z := NIL;
    if not (SY IN [IDENT,VARSY,FUNCSY,VOIDSY]) then
        errAndSkip(errBadSymbol, (skipToSet + [IDENT,RPAREN]));
    int92z := 1;
    while (SY IN [IDENT,VARSY,FUNCSY,VOIDSY]) do {
        l3sym7z := SY;
        if (SY = IDENT) then
            parClass := VARID
        else if (SY = VARSY) then
            parClass := FORMALID
        else {
            parClass := ROUTINEID;
        };
        l3var3z := NIL;
        if (SY = VOIDSY) then
            expType := NIL
        else
            expType := integerType;
        l3var6z := 0;
        if (SY <> IDENT) then {
            int93z := 0;
            inSymbol;
        };
        repeat if (SY = IDENT) then {
            if (isDefined) then
                error(errIdentAlreadyDefined);
            l3var6z := l3var6z + 1;
            new(l3var1z, FORMALID);
            with l3var1z@ do {
                id := curIdent;
                offset := curFrameRegTemplate;
                cl := parClass;
                next := symHash[bucket];
                typ := NIL;
                list := curIdRec;
                value := l2int18z;
            };
            symHash[bucket] := l3var1z;
            l2int18z := l2int18z + 1;
            if (l3var2z = NIL) then
                curIdRec@.argList := l3var1z
            else
                l3var2z@.list := l3var1z;
            l3var2z := l3var1z;
            if (l3var3z = NIL) then
                l3var3z := l3var1z;
            inSymbol;
        } else
            errAndSkip(errNoIdent, skipToSet + [RPAREN,COMMA,COLON]);
        noComma := (SY <> COMMA);
        if not noComma then {
            int93z := 0;
            inSymbol;
        };
        until noComma;
        if (l3sym7z <> VOIDSY) then {
            checkSymAndRead(COLON);
            parseTypeRef(expType, (skipToSet + [IDENT,RPAREN]));
            if (l3sym7z <> VARSY) then {
                if (isFileType(expType)) then
                error(5) (*errSimpleTypeReq *)
                else if (expType@.size <> 1) then
                     l3var5z := l3var6z * expType@.size + l3var5z;
            };
            if (l3var3z <> NIL) then
                while (l3var3z <> curIdRec) do with l3var3z@ do {
                    typ := expType;
                    l3var3z := list;
                };
        };

        if (SY = SEMICOLON) then {
            int93z := 0;
            inSymbol;
            if not (SY IN (skipToSet + [IDENT,VARSY,FUNCSY,VOIDSY])) then
                errAndSkip(errBadSymbol, (skipToSet + [IDENT,RPAREN]));
        };
    };

    if (l3var5z <> 0) then {
        curIdRec@.flags := (curIdRec@.flags + [23]);
        l3var6z := l2int18z;
        l2int18z := l2int18z + l3var5z;
        l3var2z := curIdRec@.argList;

        while (l3var2z <> curIdRec) do {
            if (l3var2z@.cl = VARID) then {
                l3var5z := l3var2z@.typ@.size;
                if (l3var5z <> 1) then {
                    l3var2z@.value := l3var6z;
                    l3var6z := l3var6z + l3var5z;
                }
            };
            l3var2z := l3var2z@.list;
        };
    };

    checkSymAndRead (RPAREN);
}; (* parseParameters *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure exitScope(var arg: array [0..127] of irptr);
{
    for ii := 0 to 127 do {
        workidr := arg[ii];
        while (workidr <> NIL) and
              (workidr >= scopeBound) do
            workidr := workidr@.next;
        arg[ii] := workidr;
    };
}; (* exitScope *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
{ (* programme *)
    localSize := l2arg1z;
    if (localSize = 0) then {
        inSymbol;
        initScalars;
        exit;
    };
    preDefHead := ptr(0);
    inTypeDef := false;
    l2int11z := 0;
    strLabList := NIL;
    lineNesting := lineNesting + 1;
    labFence := numLabList;
    repeat
    if (SY = LABELSY) then {

        repeat
            inSymbol;
            if (SY <> INTCONST) then {
                requiredSymErr(INTCONST);
                goto 22421;
            };
            labIter := numLabList;
            while (labIter <> labFence) do {
                if (labIter@.id <> curToken) then {
                    labIter := labIter@.next;
                } else {
                    errLine := labIter@.line;
                    error(17); (* errLblAlreadyDefinedInLine *)
                    goto 22420;
                }
            };
            new(labIter);
            with labIter@ do {
                id := curToken;
                frame := curFrameRegTemplate;
                offset := 0;
                line := lineCnt;
                defined := false;
                next := numLabList;
            };
            numLabList := labIter;
22420:      inSymbol;
22421:      if not (SY IN [COMMA,SEMICOLON]) then
                errAndSkip(1, skipToSet + [COMMA,SEMICOLON]);
        until SY <> COMMA;
        if SY = SEMICOLON then
            inSymbol;
    };
    if (SY = CONSTSY) then {
        parseDecls(0);
        while  (SY = IDENT) do {
            if (isDefined) then
                error(errIdentAlreadyDefined);
            new(workidr=7);
            workidr@ := [curIdent, curFrameRegTemplate,
                           symHash[bucket], , ENUMID, NIL];
            symHash[bucket] := workidr;
            inSymbol;
            if (charClass <> EQOP) then
                error(errBadSymbol)
            else
                inSymbol;
            with workidr@ do
                parseLiteral(typ, high, true);
            with workidr@ do if (typ = NIL) then {
                error(errNoConstant);
                typ := integerType;
                value := 1;
            } else
                inSymbol;
            if (SY = SEMICOLON) then {
                int93z := 0;
                inSymbol;
                if not (SY IN (skipToSet + [IDENT])) then {
                    errAndSkip(errBadSymbol, skipToSet + [IDENT]);
                }
            } else {
                requiredSymErr(SEMICOLON);
            }
        }
    };
    objBufIdx := 1;
    if (SY = TYPESY) then {
        inTypeDef := true;
        typelist := NIL;
        parseDecls(0);
        while SY = IDENT do {
            if isDefined then
                error(errIdentAlreadyDefined);
            ii := bucket;
            l2var12z := curIdent;
            inSymbol;
            if (charClass <> EQOP) then
                error(errBadSymbol)
            else
                inSymbol;
            parseTypeRef(l2typ13z, skipToSet + [SEMICOLON]);
            curIdent := l2var12z;
            if (knownInType(curIdRec)) then {
                l2typ14z := curIdRec@.typ;
                if (l2typ14z@.base = booleanType) then {
                    if (l2typ13z@.k <> kindPtr) then {
                        parseDecls(1);
                        error(78); (* errPredefinedAsPointer *)
                    };
                    l2typ14z@.base := l2typ13z@.base;
                } else {
                    l2typ14z@.base := l2typ13z;
                    curIdRec@.typ := l2typ13z;
                };
                P2672(typelist, curIdRec);
            } else {
                new(curIdRec=5);
                with curIdRec@ do {
                    id := l2var12z;
                    offset := curFrameRegTemplate;
                    typ := l2typ13z;
                    cl := TYPEID;
                }
            };
            curIdRec@.next := symHash[ii];
            symHash[ii] := curIdRec;
            int93z := 0;
            checkSymAndRead(SEMICOLON);
        };
        while (typelist <> NIL) do {
            l2var12z := typelist@.id;
            curIdRec := typelist;
            parseDecls(1);
            error(79); (* errNotFullyDefined *)
            typelist := typelist@.next;
        }
    }; (* TYPESY -> 22612 *)
    inTypeDef := false;
    curExpr := NIL;
    if (SY = VARSY) then {
        parseDecls(0);

        repeat
            workidr := NIL;

            repeat
            if (SY = IDENT) then {
                new(curIdRec=7);
                if (isDefined) then
                    error(errIdentAlreadyDefined);
                with curIdRec@ do {
                    id := curIdent;
                    offset := curFrameRegTemplate;
                    next := symHash[bucket];
                    cl := VARID;
                    list := NIL;
                };
                symHash[bucket] := curIdRec;
                inSymbol;
                if (workidr = NIL) then
                    workidr := curIdRec
                else
                    l2var4z@.list := curIdRec;
                l2var4z := curIdRec;
            } else
                error(errNoIdent);
            if not (SY IN [COMMA,COLON]) then
                errAndSkip(1, skipToSet + [IDENT,COMMA]);
            done := SY <> COMMA;
            if not done then {
                int93z := 0;
                inSymbol;
            };
            (* 22663 -> 22620 *) until done;
            checkSymAndRead(COLON);
            parseTypeRef(l2typ13z, skipToSet + [IDENT,SEMICOLON]);
            jj := l2typ13z@.size;
            while workidr <> NIL do with workidr@ do {
                curIdRec := list;
                typ := l2typ13z;
                list := NIL;
                done := true;
                if (curProcNesting = 1) then {
                    curExternFile := externFileList;
                    l2var12z := id;
                    curVal.i := jj;
                    toAlloc := curVal.m * halfWord + [24,27,28,29];
                    while done and (curExternFile <> NIL) do {
                        if (curExternFile@.id = l2var12z) then {
                            done := false;
                            if (curExternFile@.line = 0) then {
                                curVal.i := curExternFile@.offset;
                                workidr@.value := allocExtSymbol(toAlloc);
                                curExternFile@.line := lineCnt;
                            }
                        } else {
                            curExternFile := curExternFile@.next;
                        }
                    }
                };
                if (done) then {
                    workidr@.value := localSize;
                    if (PASINFOR.listMode = 3) then {
                        write('VARIABLE ':25);
                        printTextWord(workidr@.id);
                        writeln(' OFFSET (', curProcNesting:0, ') ',
                                localSize:5 oct, 'B. WORDS=',
                                jj:5 oct, 'B');
                    };
                    localSize := localSize + jj;
                    curExternFile := NIL;
                };
                if isFileType(l2typ13z) then
                    makeExtFile;
                workidr := curIdRec;
            };
            int93z := 0;
            checkSymAndRead(SEMICOLON);
            if (SY <> IDENT) and not (SY IN skipToSet) then
                errAndSkip(errBadSymbol, skipToSet + [IDENT]);
        (* 23001 -> 22617 *) until SY <> IDENT;
    }; (* VARSY -> 23003 *)
    if (curProcNesting = 1) then {
        workidr := outputFile;
        curExternFile := fileForOutput;
        makeExtFile;
        if (inputFile <> NIL) then {
            workidr := inputFile;
            curExternFile := fileForInput;
            makeExtFile;
        }
    };
    if (curExpr <> NIL) then {
        l2int11z := moduleOffset;
        formOperator(FILEINIT);
    } else
        l2int11z := 0;
    if (curProcNesting = 1) then {
        curExternFile := externFileList;
        while (curExternFile <> NIL) do {
            if (curExternFile@.line = 0) then {
                error(80); (* errUndefinedExternFile *)
                printTextWord(curExternFile@.id);
                writeLN;
            };
            curExternFile := curExternFile@.next;
        }
    };
    outputObjFile;
    while (SY = VOIDSY) or (SY = FUNCSY) do {
        done := SY = VOIDSY;
        if (curFrameRegTemplate = 7) then {
            error(81); (* errProcNestingTooDeep *)
        };
        int93z := 0;
        inSymbol;
        if (SY <> IDENT) then {
            error(errNoIdent);
            curIdRec := uProcPtr;
            isPredefined := false;
        } else {
            if (isDefined) then with hashTravPtr@ do {
                if (cl = ROUTINEID) and
                   (list = NIL) and
                   (preDefLink <> NIL) and
                   ((typ = NIL) = done) then {
                    isPredefined := true;
                } else {
                    isPredefined := false;
                    error(errIdentAlreadyDefined);
                    printErrMsg(82); (* errPrevDeclWasNotForward *)
                };
            } else
                isPredefined := false;
        };
        if not isPredefined then {
            new(curIdRec, 12);
            with curIdRec@ do {
                id := curIdent;
                offset := curFrameRegTemplate;
                next := symHash[bucket];
                typ := NIL;
                symHash[bucket] := curIdRec;
                cl := ROUTINEID;
                list := NIL;
                value := 0;
                argList := NIL;
                preDefLink := NIL;
                if (declExternal) then
                    flags := [0:15,22]
                else
                    flags := [0:15];
                pos := 0;
                curFrameRegTemplate := curFrameRegTemplate + frameRegTemplate;
                if done then
                    l2int18z := 3
                else
                    l2int18z := 4;
            };
            curProcNesting := curProcNesting + 1;
            inSymbol;
            if (6 < curProcNesting) then
                error(81); (* errProcNestingTooDeep *)
            if not (SY IN [LPAREN,SEMICOLON,COLON]) then
                errAndSkip(errBadSymbol, skipToSet + [LPAREN,SEMICOLON,COLON]);
            if (SY = LPAREN) then
                parseParameters;
            if not done then {
                if (SY <> COLON) then
                    errAndSkip(106 (*:*), skipToSet + [SEMICOLON])
                else {
                    inSymbol;
                    parseTypeRef(curIdRec@.typ, skipToSet + [SEMICOLON]);
                    if (curIdRec@.typ@.size <> 1) then
                        error(errTypeMustNotBeFile);
                }
            };
        } else  {
            with hashTravPtr@ do {
                l2int18z := level;
                curFrameRegTemplate := curFrameRegTemplate + indexreg[1];
                curProcNesting := curProcNesting + 1;
                if (preDefHead = hashTravPtr) then {
                    preDefHead := preDefLink;
                } else {
                    curIdRec := preDefHead;
                    while (hashTravPtr <> curIdRec) do {
                        workidr := curIdRec;
                        curIdRec := curIdRec@.preDefLink;
                    };
                    workidr@.preDefLink := hashTravPtr@.preDefLink;
                }
            };
            hashTravPtr@.preDefLink := NIL;
            curIdRec := hashTravPtr@.argList;
            if (curIdRec <> NIL) then {
                while (curIdRec <> hashTravPtr) do {
                    addToHashTab(curIdRec);
                    curIdRec := curIdRec@.list;
                }
            };
            curIdRec := hashTravPtr;
            setup(scopeBound);
            inSymbol;
        };
        checkSymAndRead(SEMICOLON);
        with curIdRec@ do if (curIdent = litForward) then {
            if (isPredefined) then
                error(83); (* errRepeatedPredefinition *)
            level := l2int18z;
            preDefLink := preDefHead;
            preDefHead := curIdRec;
        } else  if (SY = EXTERNSY) or
            (curIdent = litFortran) then {
            if (SY = EXTERNSY) then {
                curVal.m := [20];
            } else if (checkFortran) then {
                curVal.m := [21,24];
                checkFortran := false;
            } else {
                curVal.m := [21];
            };
            curIdRec@.flags := curIdRec@.flags + curVal.m;
        } else  {
            repeat
                setup(scopeBound);
                programme(l2int18z, curIdRec);
                if not (SY IN [FUNCSY,VOIDSY,BEGINSY]) then
                    errAndSkip(errBadSymbol, skipToSet);
            until SY IN [FUNCSY,VOIDSY,BEGINSY];
            rollup(scopeBound);
            exitScope(symHash);
            exitScope(typeHash);
            goto 23301;
        };
        inSymbol;
        checkSymAndRead(SEMICOLON);
23301:  workidr := curIdRec@.argList;
        if (workidr <> NIL) then {
            while (workidr <> curIdRec) do {
                scopeBound := NIL;
                P2672(scopeBound, workidr);
                workidr := workidr@.list;
            }
        };
        curFrameRegTemplate := curFrameRegTemplate - indexreg[1];
        curProcNesting := curProcNesting - 1;
    };
    if (SY <> BEGINSY) and
       (not allowCompat or not (SY IN blockBegSys)) then
        errAndSkip(84 (* errErrorInDeclarations *), skipToSet);
    until SY in statBegSys;
    if (preDefHead <> ptr(0)) then {
        error(85); (* errNotFullyDefinedProcedures *)
        while (preDefHead <> ptr(0)) do {
            printTextWord(preDefHead@.id);
            preDefHead := preDefHead@.preDefLink;
        };
        writeLN;
    };
    defineRoutine;
    while (numLabList <> labFence) do {
        if not (numLabList@.defined) then {
            write(' ', numLabList@.id.i:0, ':');
            done := false;
        };
        numLabList := numLabList@.next;
    };
    if not done then {
        printTextWord(l2idr2z@.id);
        error(90); (* errLblDefinitionInBlock *)
    };
    l2arg1z := l2int21z;

}; (* programme *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure initTables;
var
    idx, jdx: integer;
    l2unu3z, l2unu4z, l2unu5z: word;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure initInsnTemplates;
var
    l3var1z: insn;
    l3var2z: operator;
{
    for l3var1z := ATX to JADDM do
        insnTemp[l3var1z] := ord(l3var1z) * 10000B;
    insnTemp[ELFUN] := 500000B;
    jdx := KUTC;
    for l3var1z := UTC to VJM do {
        insnTemp[l3var1z] := jdx;
        jdx := (jdx + 100000B);
    };
    for idx to 15 do
        indexreg[idx] := idx * frameRegTemplate;
    jumpType := insnTemp[UJ];
    for l3var2z := MUL to ASSIGNOP do {
        opFlags[l3var2z] := opfCOMM;
        opToInsn[l3var2z] := 0;
        if (l3var2z IN [MUL, RDIVOP, PLUSOP, MINUSOP]) then {
            opToMode[l3var2z] := 3;
        } else if (l3var2z IN [IDIVOP, IMODOP]) then {
            opToMode[l3var2z] := 2;
        } else if (l3var2z IN [IMULOP, INTPLUS, INTMINUS]) then {
            opToMode[l3var2z] := 1;
        } else {
            opToMode[l3var2z] := 0;
        }
    };
    opToInsn[MUL] := insnTemp[AMULX];
    opToInsn[RDIVOP] := insnTemp[ADIVX];
    opToInsn[IDIVOP] := 17; (* P/DI *)
    opToInsn[IMODOP] := 11; (* P/MD *)
    opToInsn[PLUSOP] := insnTemp[ADD];
    opToInsn[MINUSOP] := insnTemp[SUB];
    opToInsn[IMULOP] := insnTemp[AMULX];
    opToInsn[SETAND] := insnTemp[AAX];
    opToInsn[SETXOR] := insnTemp[AEX];
    opToInsn[SETOR] := insnTemp[AOX];
    opToInsn[INTPLUS] := insnTemp[ADD];
    opToInsn[INTMINUS] := insnTemp[SUB];
    opToInsn[MKRANGE] := 61; (* P/PI *)
    opToInsn[SETSUB] := insnTemp[AAX];
    opToInsn[SHLEFT] := 98;
    opToInsn[SHRIGHT] := 99;
    opFlags[AMPERS] := opfAND;
    opFlags[IDIVOP] := opfDIV;
    opFlags[OROP] := opfOR;
    opFlags[IMULOP] := opfMULMSK;
    opFlags[IMODOP] := opfMOD;
    opFlags[MKRANGE] := opfHELP;
    opFlags[ASSIGNOP] := opfASSN;
    opFlags[SETSUB] := opfINV;
    opFlags[SHLEFT] := opfSHIFT;
    opFlags[SHRIGHT] := opfSHIFT;
    for jdx := 0 to 6 do {
        funcInsn[jdx] := (500000B + jdx);
    }
}; (* initInsnTemplates *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure regKeywords;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure regResWord(l4arg1z: integer);
var
    kw: @kword;
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
    if (charClass = NOOP) then {
        SY := succ(SY);
    } else {
        charClass := succ(charClass);
    }
}; (* regResWord *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* regKeywords *)
    SY := MULOP;
    charClass := AMPERS;
    regResWord(415644C(*"     AND"*));
    regResWord(445166C(*"     DIV"*));
    SY := RELOP;
    charClass := INOP;
    regResWord(5156C(*"      IN"*));
    SY := NOTSY;
    charClass := NOOP;
    regResWord(565764C(*"     NOT"*));
    SY := LABELSY;
    charClass := NOOP;
    for idx := 0 to 25 do
        regResWord(resWordName[idx]);
}; (* regKeywords *)
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure initArrays;
var
    l3var1z, l3var2z: word;
{
    FcstCnt := 0;
    FcstCount := 0;
    for idx := 3 to 6 do {
        l3var2z.i := (idx - (2));
        for jdx to l3var2z.i do
            frameRestore[idx][jdx] := 0;
    };
    for idx to 99 do
        helperMap[idx] := 0;
}; (* initArrays *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure initSets;
{
    skipToSet := blockBegSys + statBegSys - [SWITCHSY];
    bigSkipSet := skipToSet + statEndSys;
}; (* initSets *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* initTables *)
    initArrays;
    initInsnTemplates;
    initSets;
    unpack(pasinfor.a3@, iso2text, '_052'); (* '*' *)
    iso2text['_'] := iso2text['*'];
    rewrite(CHILD);
    for jdx to 10 do
        put(CHILD);
    for idx := 0 to 127 do {
        symHash[idx] := NIL;
        typeHash[idx] := ;
        kwordHash[idx] := ;
    };
    regKeywords;
    numLabList := NIL;
    totalErrors := 0;
    heapCallsCnt := 0;
    putLeft := true;
    readNext := true;
    curFrameRegTemplate := frameRegTemplate;
    curProcNesting := 1;
}; (* initTables *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure finalize;
var
    idx, cnt: integer;
    sizes: array [1..10] of @integer;
{
    sizes[1] := ptr(1);
    sizes[2] := ptr(symTabPos - 74000B - 1);
    sizes[5] := ptr(longSymCnt);
    sizes[6] := ptr(moduleOffset - 40000B);
    sizes[8] := ptr(FcstCnt);
    sizes[3] := ptr(0);
    sizes[4] := ;
    sizes[7] := ;
    sizes[9] := ptr(int92z);
    sizes[10] := ptr(int93z);
    curVal.i := moduleOffset - 40000B;
    symTab[74001B] := [24,29] + curVal.m - intZero;
    reset(FCST);
    while not eof(FCST) do {
        write(CHILD, FCST@);
        get(FCST);
    };
    curVal.i := (symTabPos - 70000B) * 100000000B;
    for cnt to longSymCnt do {
        idx := longSym[cnt];
        symTab[idx] := (symTab[idx] + (curVal.m * [9:23]));
        curVal.i := (curVal.i + 100000000B);
    };
    symTabPos := symTabPos - 1;
    for cnt := 74000B to symTabPos do
        write(CHILD, symTab[cnt]);
    for cnt to longSymCnt do
        write(CHILD, longSyms[cnt]);
    if (allowCompat) then {
        write((lineCnt - 1):6, ' LINES STRUCTURE ');
        for idx to 10 do
            write(ord(sizes[idx]):0, ' ');
        writeln;
    };
    entryPtTable[entryPtCnt] := [];
    pasinfor.entryptr@ := entryPtTable;
    pasinfor.sizes := sizes;
}; (* finalize *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure initOptions;
{
    pasinfor.startOffset := pasinfor.startOffset - 16384;
    commentModeCH := ' ';
    lineNesting := 0;
    CH := ' ';
    linePos := 0;
    prevErrPos := 0;
    errsInLine := 0;
    lineCnt := 1;
    checkFortran := false;
    bool110z := false;
    int93z := 1;
    int92z := 1;
    moduleOffset := 16384;
    lineStartOffset := ;
    int94z := 1;
    bool47z := false;
    dataCheck := ;
    heapSize := 100;
    forValue := true;
    atEOL := false;
    curVal.m := pasinfor.flags;
    besm(ASN64 - 39);
    besm(ASN64 + 45);
    optSflags := ;
    doPMD := not (42 in curVal.m);
    checkTypes := true;
    fixMult := true;
    fuzzReals := true;
    checkBounds := not (44 in curVal.m);
    declExternal := false;
    errors := false;
    allowCompat := false;
    litForward.i := 46576267416244C;
    litFortran.i := 46576264624156C;
    fileBufSize := 1;
    charEncoding := 2;
    chain := NIL;
    litOct.i := 574364C;
    longSymCnt := 0;
    pasinfor.errors@ := true;
    symTabCnt := 0;
    paseofcd := '*copy:';
}; (* initOptions *)
%
{ (* main *)
    if PASINFOR.listMode <> 0 then
        writeln(boilerplate);
    initOptions;
    curInsnTemplate := 0;
    initTables;
    programme(curInsnTemplate, hashTravPtr);
    if errors then {
9999:   writeln(' IN ', (lineCnt-1):0, ' LINES ',
            totalErrors:0, ' ERRORS');
    } else {
        finalize;
        PASINFOR.errors@ := false;
    }
}
.data
    frameRegTemplate := 04000000B;
    constRegTemplate := I8;
    disNormTemplate :=  KNTR+7;
    blockBegSys := [LABELSY, CONSTSY, TYPESY, VARSY, FUNCSY, VOIDSY, BEGINSY];
    statBegSys :=  [BEGINSY, IFSY, SWITCHSY, DOSY, WHILESY, FORSY, WITHSY,
                    GOTOSY];
    O77777 := [33:47];
    intZero := 0;
    unused138z := (63000000C);
    extSymMask := (43000000C);
    halfWord := [24:47];
    hashMask := 203407C;
    statEndSys := [SEMICOLON, ENDSY, ELSESY, WHILESY];
    lvalOpSet := [GETELT, GETVAR, op36, op37, GETFIELD, DEREF, FILEPTR];
    resWordName :=
        5441424554C             (*"   LABEL"*),
        4357566364C             (*"   CONST"*),
        64716045C               (*"    TYPE"*),
        664162C                 (*"     VAR"*),
        4665564364515756C       (*"FUNCTION"*),
        66575144C               (*"    VOID"*),
        45566555C               (*"    ENUM"*),
        604143534544C           (*"  PACKED"*),
        4162624171C             (*"   ARRAY"*),
        636462654364C           (*"  STRUCT"*),
        46515445C               (*"    FILE"*),
        5146C                   (*"      IF"*),
        636751644350C           (*"  SWITCH"*),
        6750515445C             (*"   WHILE"*),
        465762C                 (*"     FOR"*),
        67516450C               (*"    WITH"*),
        47576457C               (*"    GOTO"*),
        45546345C               (*"    ELSE"*),
        5746C                   (*"      OF"*),
        4457C                   (*"      DO"*),
        457064456256C           (*"  EXTERN"*),
        4262454153C             (*"   BREAK"*),
        4357566451566545C       (*"CONTINUE"*),
        576450456263C           (*" DEFAULT"*);
%
    charSym := NOSY:128;
    chrClass := NOOP:128;
    charSym['0'] := INTCONST:10;
    chrClass['0'] := ALNUM:10;
    charSym['A'] := IDENT:26;
    chrClass['A'] := ALNUM:26;
    charSym['Ю'] := IDENT:31;
    chrClass['Ю'] := ALNUM:31;
    chrClass['_'] := ALNUM;
    funcInsn[fnABS] := KAMX;
    funcInsn[fnTRUNC] := KADD+ZERO;
    funcInsn[fnODD] := KAAX+E1;
    funcInsn[fnORD] := KAOX+ZERO;
    funcInsn[fnCHR] := KAAX+MANTISSA;
    funcInsn[fnSUCC] := KARX+E1;
    funcInsn[fnPRED] := KSUB+E1;
    funcInsn[fnROUND] := macro + mcROUND;
    funcInsn[fnCARD] := macro + mcCARD;
    funcInsn[fnMINEL] := macro + mcMINEL;
    funcInsn[fnPTR] := KAAX+MANTISSA;
    funcInsn[fnABSI] := KAMX;
    iAddOpMap[PLUSOP] := INTPLUS, INTMINUS;
    imulOpMap := IMULOP, IDIVOP;
    charSym[''''] := CHARCONST;
    charSym['_'] := IDENT;
    charSym['<'] := LTSY;
    charSym['>'] := GTSY;
    chrClass['+'] := PLUSOP;
    chrClass['-'] := MINUSOP;
    chrClass['*'] := MUL;
    chrClass['/'] := RDIVOP;
    chrClass['%'] := IMODOP;
    chrClass['='] := EQOP;
    chrClass['&'] := AMPERS;
    chrClass['|'] := OROP;
    chrClass['^'] := SETXOR;
    chrClass['~'] := SETSUB;
    chrClass['>'] := GTOP;
    chrClass['<'] := LTOP;
    chrClass['!'] := NEOP;
    charSym['+'] := ADDOP;
    charSym['-'] := ADDOP;
    charSym['|'] := ADDOP;
    charSym['*'] := MULOP;
    charSym['/'] := MULOP;
    charSym['%'] := MULOP;
    charSym['&'] := MULOP;
    charSym[','] := COMMA;
    charSym['.'] := PERIOD;
    charSym['@'] := ARROW;
    charSym['^'] := MULOP;
    charSym['('] := LPAREN;
    charSym[')'] := RPAREN;
    charSym[';'] := SEMICOLON;
    charSym['['] := LBRACK;
    charSym[']'] := RBRACK;
    charSym['='] := BECOMES;
    charSym[':'] := COLON;
    charSym['!'] := NOTSY;
    charSym['~'] := ADDOP;
    helperNames :=
        6017210000000000C      (*"P/1     "*),
        6017220000000000C      (*"P/2     "*),
        6017230000000000C      (*"P/3     "*),
        6017240000000000C      (*"P/4     "*),
        6017250000000000C      (*"P/5     "*),
        6017260000000000C      (*"P/6     "*),
        6017434100000000C      (*"P/CA    "*),
        6017455700000000C      (*"P/EO    "*), (* fnEOF - 6 *)
        6017636300000000C      (*"P/SS    "*),
(*10*)  6017455400000000C      (*"P/EL    "*), (* fnEOLN - 6 *)
        6017554400000000C      (*"P/MD    "*),
        6017555100000000C      (*"P/MI    "*),
        6017604100000000C      (*"P/PA    "*),
        6017655600000000C      (*"P/UN    "*),
        6017436000000000C      (*"P/CP    "*),
        6017414200000000C      (*"P/AB    "*),
        6017445100000000C      (*"P/DI    "*),
        6017624300000000C      (*"P/RC    "*),
        6017454100000000C      (*"P/EA    "*),
(*20*)  6017564100000000C      (*"P/NA    "*),
        6017424100000000C      (*"P/BA    "*),
        6017515100000000C      (*"P/II   u"*),
        6017626200000000C      (*"P/RR    "*),
        6017625100000000C      (*"P/RI    "*),
        6017214400000000C      (*"P/1D    "*),
        6017474400000000C      (*"P/GD    "*),
        6017450000000000C      (*"P/E     "*),
        6017454600000000C      (*"P/EF    "*),
        6017604600000000C      (*"P/PF    "*),
(*30*)  6017474600000000C      (*"P/GF    "*),
        6017644600000000C      (*"P/TF    "*),
        6017624600000000C      (*"P/RF    "*),
        6017566700000000C      (*"P/NW    "*),
        6017446300000000C      (*"P/DS    "*),
        6017506400000000C      (*"P/HT    "*),
        6017675100000000C      (*"P/WI    "*),
        6017676200000000C      (*"P/WR    "*),
        6017674300000000C      (*"P/WC    "*),
        6017412600000000C      (*"P/A6    "*),
(*40*)  6017412700000000C      (*"P/A7    "*),
        6017677000000000C      (*"P/WX    "*),
        6017675700000000C      (*"P/WO    "*),
        6017436700000000C      (*"P/CW    "*),
        6017264100000000C      (*"P/6A    "*),
        6017274100000000C      (*"P/7A    "*),
        6017675400000000C      (*"P/WL    "*),
        6017624451000000C      (*"P/RDI   "*),
        6017624462000000C      (*"P/RDR   "*),
        6017624443000000C      (*"P/RDC   "*),
(*50*)  6017624126000000C      (*"P/RA6   "*),
        6017624127000000C      (*"P/RA7   "*),
        6017627000000000C      (*"P/RX   u"*),
        6017625400000000C      (*"P/RL    "*),
        6017675754560000C      (*"P/WOLN  "*),
        6017625154560000C      (*"P/RILN  "*),
        6017626200000000C      (*"P/RR    "*),
        6017434500000000C      (*"P/CE    "*),
        6017646200000000C      (*"P/TR    "*),
        6017546600000000C      (*"P/LV    "*),
(*60*)  6017724155000000C      (*"P/ZAM  u"*),
        6017605100000000C      (*"P/PI    "*),
        6017426000000000C      (*"P/BP    "*),
        6017422600000000C      (*"P/B6    "*),
        6017604200000000C      (*"P/PB    "*),
        6017422700000000C      (*"P/B7    "*),
        6017515600000000C      (*"P/IN    "*),
        6017516300000000C      (*"P/IS    "*),
        6017444100000000C      (*"P/DA    "*),
        6017435700000000C      (*"P/CO    "*),
(*70*)  6017516400000000C      (*"P/IT    "*),
        6017435300000000C      (*"P/CK    "*),
        6017534300000000C      (*"P/KC    "*),
        6017545647604162C      (*"P/LNGPAR"*),
        6017544441620000C      (*"P/LDAR  "*),
        6017544441625156C      (*"P/LDARIN"*),
        6017202043000000C      (*"P/00C   "*),
        6017636441620000C      (*"P/STAR  "*),
        6017605544634564C      (*"P/PMDSET"*),
        6017435100000000C      (*"P/CI    "*),
(*80*)  6041514200000000C      (*"PAIB    "*),
        6017674100000000C      (*"P/WA    "*),
        6361626412000000C      (*"SQRT*   "*),
        6351561200000000C      (*"SIN*    "*),
        4357631200000000C      (*"COS*    "*),
        4162436441561200C      (*"ARCTAN* "*),
        4162436351561200C      (*"ARCSIN* "*),
        5456120000000000C      (*"LN*     "*),
        4570601200000000C      (*"EXP*    "*),
        6017456100000000C      (*"P/EQ    "*),
(*90*)  6017624100000000C      (*"P/RA    "*),
        6017474500000000C      (*"P/GE    "*),
        6017554600000000C      (*"P/MF    "*),
        6017465500000000C      (*"P/FM    "*),
        6017565600000000C      (*"P/NN    "*),
        6017634300000000C      (*"P/SC    "*),
        6017444400000000C      (*"P/DD    "*),
        6017624500000000C      (*"P/RE    "*),
        4317635054000000C      (*"C/SHL   "*),
        4317635062000000C      (*"C/SHR   "*);
    systemProcNames :=
(*0*)   606564C                (*"     PUT"*),
        474564C                (*"     GET"*),
        62456762516445C        (*" REWRITE"*),
        6245634564C            (*"   RESET"*),
        564567C                (*"     NEW"*),
        44516360576345C        (*"    FREE"*),
        50415464C              (*"    HALT"*),
        63645760C              (*"    STOP"*),
        6345646560C            (*"   SETUP"*),
        625754546560C          (*"  ROLLUP"*),
(*10*)  6762516445C            (*"   WRITE"*),
        67625164455456C        (*" WRITELN"*),
        0C                     (*"    READ"*),
        0C                     (*"  READLN"*),
        45705164C              (*"    EXIT"*),
        4445426547C            (*"   DEBUG"*),
        42456355C              (*"    BESM"*),
        0C                     (*"   MAPIA"*),
        5541604151C            (*"   MAPAI"*),
        604353C                (*"     PCK"*),
(*20*)  6556604353C            (*"   UNPCK"*),
        60414353C              (*"    PACK"*),
        655660414353C          (*"  UNPACK"*);
end
