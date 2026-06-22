(*=p-,t-,s8,u-,y+,k9,l0*)
program pascompl(output, child, pasinput, pasinfor,paseofcd);
%
label 9999;
%
const
    boilerplate = ' PASCAL METAMORPH HELPER (2025) ';
%
    fnSQRT  = 0;  fnSIN  = 1;  fnCOS  = 2;  fnATAN  = 3;  fnASIN = 4;
    fnLN    = 5;  fnEXP  = 6;  fnABS =  7;  fnTRUNC = 8;  fnSIZEOF = 9;
    fnOFFSETOF=10;(*  11         12     *)  fnMALLOC = 13;fnEOF  = 14;
    fnREF   = 15; fnEOLN = 16; fnSETJMP = 17; fnROUND = 18; fnCARD = 19;
    fnMINEL = 20; fnPTR  = 21; fnABSI = 22;
%
    S3 = 0;
    S4 = 1;
    S5 = 2;
    S6 = 3;
    NoStackCheck = 5;
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
    errTooManyArguments = 38;
    errNoCommaOrParenOrTooFewArgs = 41;
    errNumberTooLarge = 43;
    errVarTooComplex = 48;
    errEOFEncountered = 52;
    errFirstDigitInCharLiteralGreaterThan3 = 60;
%
    precNone = -1;   precAssign = 0;
    precCond = 1;    precOr = 2;     precAnd = 3;
    precBitOr = 4;   precBitXor = 5; precBitAnd = 6;
    precEq = 7;      precRel = 8;    precShift = 9;
    precAdd = 10;    precMul = 11;
%
    macro = 100000000B;
    mcACC2ADDR = 6;
    mcPOP = 4;
    mcPUSH = 5;
    mcMULTI = 7;
    mcADDSTK2REG = 8;
    mcADDACC2REG = 9;
    mcDUMMY = 10;
    mcROUND = 11;
    mcMALLOC = 12;
    mcMINEL = 15;
    mcPOP2ADDR = 19;
    mcCOND2INT = 20;
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
    E48 =       4000025B;
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
    KMADDJ =    0450000B;
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
   lookDef = 0;
   lookUse = 1;
   lookWith = 2;
   lookField = 3;
%
   BACKSLASH = '\035';
%
type
    assoc = (leftAs, rightAs);
%
    symbol = (
(*0B*)  IDENT,      INTCONST,   REALCONST,  CHARCONST,
        STRINGSY,   LPAREN,     LBRACK,     EXPROP,
(*10B*) RPAREN,     RBRACK,     COMMA,      SEMICOLON,
        PERIOD,     ARROW,      COLON,      BECOMES,
(*20B*) BEGINSY,    ENDSY,      CONSTSY,    TYPEDEFSY,
        VARSY,      TYPESY,     VOIDSY,     ENUMSY,
(*30B*) PACKEDSY,   ARRAYSY,    STRUCTSY,   FILESY,
        IFSY,       SWITCHSY,   WHILESY,    FORSY,
(*40B*) WITHSY,     GOTOSY,     ELSESY,     OFSY,
        DOSY,       EXTERNSY,   BREAKSY,    CONTSY,
(*50B*) CASESY,     DEFAULTSY,  UNIONSY,    NOSY
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
(*040*) ATI,   STI,   ITA,   ITS,   MTJ,   MADDJ, ELFUN,
(*047*) UTC,   WTC,   VTM,   UTM,   UZA,   U1A,   UJ,    VJM
);
%
setofsys = set of ident .. dosy;
%
operator = (
    SHLEFT,     SHRIGHT,
    SETAND,     SETXOR,     SETOR,
    MUL,        RDIVOP,     ANDOP,     IDIVOP,     IMODOP,
    PLUSOP,     MINUSOP,    OROP,       NEOP,       EQOP,
    LTOP,       GEOP,       GTOP,       LEOP,       INOP,
    IMULOP,     INTPLUS,    INTMINUS,   CONDOP,     ALTERN,
    INCROP,     DECROP,     ASSIGNOP,   GETELT,     GETVAR,
    RMWASSIGN,  op37,       GETENUM,    GETFIELD,   DEREF,
    FILEPTR,    STKLVAL,    ALNUM,      PCALL,      FCALL,
    TOREAL,     NOTOP,      INEGOP,     RNEGOP,     BITNEGOP,
    STANDPROC,  NOOP
);
%
opgen = (
    gen0,  STORE, LOAD,  FORMOP,  SETREG,
    SETREG9,  STOREAT9,  DOIT,  SETREG12,  DFLTWDTH,
    FRACWIDTH, gen11, gen12, FILEACCESS, FILEINIT,
    BRANCH, PCKUNPCK, LITINSN
);
%
% Flags for ops that can potentially be optimized if one operand is a constant
opflg = (
    opfCOMM, opfHELP, opfAND, opfOR, opfDIV, opfMOD, opfSHIFT,
    opfMULMSK, opfASSN
);
%
kind = (
    kindVoid, kindReal, kindScalar, kindPtr,
    kindArray, kindStruct, kindFile,
    kindCases, kindRoutine
);
%
bitset = set of 0..47;
%
eptr = @expr;
irptr = @identrec;
sigptr = @sigrec;
(*=s6 right to left packing *)
pckrep = packed record
   rep    : @types; (* for assignment only, due to a Pascal compiler bug *)
   bits   : 0..48;
   pk     : kind;
   psize  : 0..32767;
   pad    : 0..255;  (* multi-use *)
end;
tptr = record case integer of
         0 : (rep : @types); (* for deref only, due to a Pascal compiler bug *)
         1 : (p: pckrep);
       end;
sigrec = record
    pclass                  : idclass;
    ptyp                    : tptr;
    next                    : sigptr
end;
word = record case integer of
    0: (i: integer);
    1: (r: real);
    2: (b: boolean);
    3: (a: alfa);
    4: (t: packed array[0..7] of '_000' .. '_077');
    5: (typ: tptr);
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
ilmode = (ilCONST, ilLVAL, ilRVAL, ilCOND);
state = (stWORD, stSLICE, stPACKED);
%
insnltyp  = record
    tail, head: oiptr;
    typ: tptr;
    regsused: bitset;
    ilm: ilmode;
    payload: word;
    disp: integer;
    addrmd: integer;
    st: state;
    width, shift: integer
end;
%
types = record
    case kind of
    kindReal:   ();
    kindArray:  (base:      tptr;
                 pck:       boolean;
                 perword,
                 pcksize, aleft, aright:   integer);
    kindScalar: (enums:     irptr;
                 numen,
                 start:     integer);
    kindPtr:    (sbase:      tptr);
    kindFile:   (fbase:      tptr);
    kindStruct: (variants: tptr;
                 fields:    irptr;
                 flag,
                 pckrec:    boolean);
    kindCases:  (first,
                 next:      tptr;
                 alt:       tptr);
    kindRoutine:(rresult:   tptr;
                 rparams:   sigptr;
                 rargc:     integer;
                 rflags:    bitset)
    end;
%
charmap   = packed array ['_000'..'_176'] of char;
textmap   = packed array ['_052'..'_177'] of '_000'..'_077';
%
four = array [1..4] of integer;
entries   = array [1..42] of bitset;
%
expr = record
    vt : word;
    op : operator;
    case operator of (* arbitrary so far *)
    NOOP:       (lit: word);
    MUL:        (expr1, expr2: eptr);
    GETFIELD:   (typ1, typ2: tptr);
    NOTOP:      (id1, id2: irptr);
    STANDPROC:  (num1, num2: integer);
end;
%
kword = record
    next:   @kword;
    w:      word;
    sym:    symbol;
    case integer of
      1 : ( op: operator);
      2 : ( id: irptr);
end;
%
strLabel = record
    next:       @strLabel;
    ident:      word;
    target: integer;
end;
%
numLabel = record
    id:         word;
    line:       integer;
    offset:     integer;
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
             flags: bitset;
             sigtyp: tptr
            );
end;
hashArray = array [0..127] of irptr;
extfilerec = record
    id:     word;
    offset: integer;
    next:   @extfilerec;
    location,
    line: integer
end;
numberFormat = (decimal, octal, fullword, hex);
%
var
   curTimes  : integer;
   numFormat : numberFormat;
   bigSkipSet, statEndSys, blockBegSys, statBegSys,
   skipToSet, lvalOpSet: setofsys;
   inCallArgs, bool48z, forValue: boolean;
   dataCheck: boolean;
   jumpType: integer;
   jumpTarget: integer;
   charClass: operator;
   SY, prevSY: symbol;
   savedObjIdx: integer;
   FcstCnt: integer;
   symTabPos: integer;
   entryPtCnt: integer;
   fileBufSize: integer;
   withIter, withList: eptr;
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
   CH, prevCH: char;
   prevInsn: word;
   debugLine: integer;
   lineNesting: integer;
   FcstCountTo500: integer;
   objBufIdx: integer;
   lookup2, lookupMode, condLabCnt: integer;
   prevOpcode: integer;
   charEncoding: integer;
   errLine: integer;
   atEOL: boolean;
   checkTypes: boolean;
   isDefined, putLeft, readNext: boolean;
   errors: boolean;
   declEntry: boolean;
   rangeMismatch: boolean;
   doPMD: boolean;
   checkBounds: boolean;
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
   voidType, voidPtr: tptr;
   booleanType: tptr;
   textType: tptr;
   integerType: tptr;
   realType: tptr;
   charType: tptr;
   alfaType: tptr;
   arg1Type: tptr;
   arg2Type: tptr;
   numLabs: array [1..20] of numLabel;
   numLabTop: integer;
   curToken: word;
   curVal: word;
   O77777: bitset;
   intZero: bitset;
   extSymMask: bitset;
   halfWord: bitset;
   leftInsn: bitset;
   hashMask: word;
   curIdent: word;
   toAlloc, usedRegs, liveRegs, freeRegs, auxRegs: bitset;
   optSflags: word;
   litOct: word;
   litForward: word;
   litFortran: word;
   litAssembler: word;
   uVarPtr: eptr;
   curExpr: eptr;
   insnList: @insnltyp;
   fileForOutput, fileForInput: @extfilerec;
   maxSmallString: integer;
   smallStringType: array [2..6] of tptr;
   symTabCnt: integer;
   symtabarray: array [1..80] of word;
   symtbidx: array [1..80] of integer;
   intOpMap: array [MUL..MINUSOP] of operator;
   entryPtTable: entries;
   frameRestore: array [3..6] of four;
   indexreg: array [1..15] of integer;
   opToInsn: array [SHLEFT..STKLVAL] of integer;
   opToMode: array [SHLEFT..STKLVAL] of integer;
   opFlags: array [SHLEFT..STKLVAL] of opflg;
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
   symHash, fieldHash: hashArray;
   helperMap: array [1..99] of integer;
   helperNames: array [1..99] of bitset;
   opPrec: array [operator] of integer;
   opAssoc: array [operator] of assoc;
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
   maxHeap: integer;
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
}; (* regResWord *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%              PROGRAMME                 %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure programme(var l2arg1z: integer; procName: irptr;
                    bodyBlock: boolean);
label 23301;
var
    preDefHead, typelist, scopeBound, l2var4z, curIdRec, workidr: irptr;
    isPredefined, done, retSeen, inTypeDef, hadParens: boolean;
    l2var10z: eptr;
    hasFiles: integer;
    bodyStatSys: setofsys;
    l2var12z: word;
    l2typ13z, l2typ14z, typedRetType, ceTyp: tptr;
    ceVal: word;
    ceRegs: bitset;
    labFence: integer;
    strLabList: @strLabel;
%
    l2int18z, ii, localSize, l2int21z, jj: integer;
%
procedure myrollup(addr : integer);
var cur : @integer;
    top : integer;
{
    setup(cur);
    top := ord(cur);
    if (top > maxHeap) then
        maxHeap := top;
    cur := ptr(addr);
    rollup(cur);
};
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
        else if errno in [16..17, 20] then {
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
var span : tptr; size: integer;
{
    if maxSmallString >= strLen then
        res := smallStringType[strLen]
    else {
        new(res.rep, kindArray);
        size := (strLen + 5) div 6;
        res.p.bits := 0;
        res.p.psize := size;
        if size = 1 then
            res.p.bits := strLen * 8;
        res.p.pk := kindArray;
        with res.rep@ do {
            base := charType;
            pck := true;
            perword := 6;
            pcksize := 8;
            aleft := 1;
            aright := strLen;
        };
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
    } else if (prevOpcode > 1) and (insn.i mod 4096 <> 0) and
    (insn.m mod prevInsn.m = [32]) (* maybe ATX/XTA *) then {
% Load after store; if the load reg/off is the same as the store,
% and the store was not a stack push, there is no need to do the read.
        if (prevInsn.i <> 74000000B (* not 15,ATX, *)) and
            (prevInsn.m * [28, 30..35] = [] (* but still ATX *)) then
            exit (* skip the XTA *)
    };
    prevOpcode := opcode.i;
    prevInsn := insn;
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
procedure formJump(var arg: integer);
var
    pos: integer;
    isLeft: boolean;
{
    if prevOpcode <> insnTemp[UJ] then {
        if putLeft then
            pos := objBufIdx + 4096
        else
            pos := objBufIdx;
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
function mkIntScl(bitWid: integer): tptr;
var
    res: tptr;
    w: word;
{
    if (bitWid < 1) or (40 < bitWid) then {
        error(errNumberTooLarge);
        mkIntScl := integerType;
        exit;
    };
    new(res.rep, kindScalar);
    with res.rep@ do {
        start := -1;
        enums := NIL;
        w.m := [47-bitWid] + intZero;
        numen := w.i;
    };
    res.p.psize := 1;
    res.p.bits := bitWid;
    res.p.pk := kindScalar;
    mkIntScl := res;
}; (* mkIntScl *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function getValueOrAllocSymtab(value: integer): integer;
{
    curVal.i := value MOD 32768;
    if (40000B >= curVal.i) then
        getValueOrAllocSymtab := curVal.i
    else
        getValueOrAllocSymtab :=
            allocSymtab((curVal.m + [24]) * halfWord);
}; (* getValueOrAllocSymtab *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
(*
 * fixup -- address-fixup helper. Behaviour selected by `mode`:
 *
 *   mode  =  0     Resolve a forward-jump chain. `arg` is the head index of
 *                  a linked list of pending UJ/UZA/U1A instructions in
 *                  objBuffer; each link stores the next index in its 15-bit
 *                  operand field. Each instruction has its operand patched
 *                  to the current moduleOffset. Used for if/else, while,
 *                  for, case, break/continue and goto target resolution.
 *                  Entries with index > 4096 patch the LEFT half-word of
 *                  the packed instruction pair instead of the right.
 *
 *   mode  >  2     Same as mode = 0, but the patched target address is
 *                  `mode` itself rather than the current moduleOffset.
 *                  Used to redirect a deferred jump chain to a previously
 *                  recorded label (see formOperator/BRANCH).
 *
 *   mode  =  1     Emit the 6-instruction range-check / dispatch sequence
 *                  used by the case statement, invoking helper P/DA (#68).
 *                  curVal.i holds the low bound; `arg` is the high bound.
 *
 *   mode  < -2     Same six-instruction emission as mode = 1, but the
 *                  helper procedure number is `-mode` instead of P/DA.
 *                  Used by caseStatement to emit the final dispatch via
 *                  mode = -(insnTemp[U1A] + otherOffset).
 *
 *   mode  =  2     Emit a relocation snippet that loads a helper-procedure
 *                  descriptor into I7 and shifts it by `arg` bits via ASN.
 *                  curVal.i is the descriptor address. Used to set up calls
 *                  to descriptor helpers P/DS (mode=2,arg=34) and P/RDC
 *                  (mode=2,arg=49) in routine prologue / epilogue.
 *
 *   mode  = -1     Emit a one-shot helper call with lineCnt staged into
 *                  M14. `arg` is the helper procedure number. Used for the
 *                  P/SC (#95) stack-check call.
 *)
procedure fixup(mode, arg: integer);
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
    } else if (mode < -2) then {
        arg := arg - curVal.i;
        offset := getFCSToffset;
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
}; (* fixup *)
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
        writeln('EOF reached');
        halt;
%       error(errEOFEncountered);
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
function skipSp:boolean;
{
    while (CH = ' ') or (CH = '_011') and not atEOL do
        nextCH;
    if atEOL then {
        endOfLine;
        nextCH;
        skipSp := true;
    } else
        skipSp := false;
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure inSymbol;
label
    1473;
var
    localBuf: array [0..130] of char;
    tokenLen, tokenIdx: integer;
    expSign: boolean;
    chain: irptr;
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
        'Y': readOptFlag(allowCompat); (* just for LINES STRUCTURE *)
        'E': readOptFlag(declEntry);
        'S': {
            readOptVal(curVal.i, 8);
            if curVal.i = 3 then lineCnt := 1
            else if curVal.i in [4..8] then
                optSflags.m := optSflags.m + [curVal.i - 3];
        };
        'F': readOptFlag(checkFortran);
        'L': readOptVal(PASINFOR.listMode, 3);
        'P': readOptFlag(doPMD); (* ignored *)
        'T': readOptFlag(checkBounds); (* ignored *)
        'A': readOptVal(charEncoding, 3);
        'C': readOptFlag(checkTypes);
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
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure lexer;
label
    1, 2, 2175, 2233, 2320;
var done : boolean;
{
done := false;
        case SY of
            IDENT: {
                   done := true;
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
                        exit;
                    };
                    keywordHashPtr := keywordHashPtr@.next;
                };
                isDefined := false;
                SY := IDENT;
                case lookupMode of
                lookDef: {
                    hashTravPtr := symHash[bucket];
                    while hashTravPtr <> NIL do {
                        if hashTravPtr@.offset = curFrameReg then
                        {
                            if hashTravPtr@.id <> curIdent then
                                hashTravPtr := hashTravPtr@.next
                            else {
                                isDefined := true;
                                exit;
                            }
                        } else
                            exit;
                    };
                };
                lookUse: {
2:                  hashTravPtr := symHash[bucket];
                    while hashTravPtr <> NIL do {
                        if hashTravPtr@.id <> curIdent then
                            hashTravPtr := hashTravPtr@.next
                        else {
                            if hashTravPtr@.cl = TYPEID then
                                SY := TYPESY;
                            exit
                        };
                    };
                };
                lookWith: {
                    if withList = NIL then
                        goto 2;
                    withIter := withList;
                    chain := fieldHash[bucket];
                    if chain <> NIL then {
                        while withIter <> NIL do {
                            hashTravPtr := chain;
                            while hashTravPtr <> NIL do {
                                if (hashTravPtr@.id = curIdent)
                                and (hashTravPtr@.uptype =
                                     withIter@.expr2@.vt.typ) then
                                    exit;
                                hashTravPtr := hashTravPtr@.next;
                            };
                            withIter := withIter@.expr1;
                        };
                    };
                    goto 2;
                };
                lookField: {
                    hashTravPtr := fieldHash[bucket];
                    while hashTravPtr <> NIL do {
                        with hashTravPtr@ do {
                            if (id = curIdent) and
                               (typ121z = uptype)
                            then
                                exit;
                            hashTravPtr := next;
                       }
                   }
                };
                end;
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
                    if CH = '.' then {
                        CH := ':';
                        exit
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
                           writeln(' set ', curtoken.m oct);
                        curVal.m := shift(curToken.m, -ord(CH)+ord('0'));
                           writeln(' after shift ', curval.m oct);
                        curVal.i := card(curVal.m) - 1;
                           writeln(' char num ', curval.i:0);
                        curToken.i := 007101412151113C; (* escMap *)
                        curVal.m := shift(curToken.m, 6*curVal.i);
                        curVal.m := curVal.m * [42..47];
                       localBuf[tokenIdx]  :=  curVal.c;
                    }
                } else {
                    goto 2233;
                }
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
                    exit;
                } else 2320: {
                    curVal.a := '      ';
                    SY := STRINGSY;
                    unpck(localBuf[tokenIdx], curVal.a);
                    pck(localBuf[6], curToken.a);
                    curVal :=;
                    if strLen <= 6 then
                        exit
                    else if (charEncoding = 3) and (strLen = 8) then {
                        pack(localbuf, 6, curToken.t);
                        curVal := ;
                        SY := INTCONST;
                        exit
                    } else {
                        curToken.i := FcstCnt;
                        tokenLen := 6;
                        (loop) {
                            toFCST;
                            tokenLen := tokenLen + 6;
                            if tokenIdx < tokenLen then
                                exit;
                            pck(localBuf[tokenLen], curVal.a);
                            goto loop
                        }
                    }
                };
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
        '^=    ': { SY := BECOMES; nextCH; exit };
        '<=    ': { charClass := LEOP; nextCH; exit };
        '<<    ': { charClass := SHLEFT; nextCH;
                    if CH = '=' then { SY := BECOMES; nextCH }; exit };
        '<:    ': { SY := BEGINSY; nextCH; exit };
        '>>    ': { charClass := SHRIGHT; nextCH;
                    if CH = '=' then { SY := BECOMES; nextCH }; exit };
        '>=    ': { charClass := GEOP; nextCH; exit };
        ':>    ': { SY := ENDSY; nextCH; exit };
        '==    ': { SY := EXPROP; charClass := EQOP; nextCH; exit };
        '!=    ': { charClass := NEOP; nextCH; exit };
        '->    ': { SY := ARROW; nextCH; exit };
        '--    ': { charClass := DECROP; nextCH; exit };
        '++    ': { charClass := INCROP; nextCH; exit };
        '||    ': { charClass := OROP; nextCH; exit };
        '&&    ': { charClass := ANDOP; nextCH; exit };
        '/*    ': { parseComment; goto 1473 };
        '//    ': { while not atEOL do nextCH; goto 1473 };
        '..    ': { SY := COLON; nextCH; exit };
        end;
        if (prevCH = '.') and (prevSY = ENDSY) then
           dataCheck := true;
}; (* lexer *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* inSymbol *)
        if dataCheck then {
            writeln('EOF reached');
            halt;
%           error(errEOFEncountered);
%           readToPos80;
        };
1473:   while skipSp do ;
        hashTravPtr := NIL;
        SY := charSym[CH];
        charClass := chrClass[CH];
        lexer;
        prevSY := SY;
        commentModeCH := ' ';
        lookupMode := lookup2;
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
    while (CH <> '_000') and ((sym <> ENDSY) or (SY <> PERIOD)) do {
        sym := SY;
        inSymbol
    };
    if CH = 'D' then
        while (CH <> '_000') and (SY <> ENDSY) do
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
    if (SY > STRINGSY) then {
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
            litType.rep := NIL;
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
        STRINGSY:
            makeStringType(litType);
        end (* case *)
}; (* parseLiteral *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure hash(var l3arg1z: irptr; l3arg2z: irptr);
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
}; (* hash *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function isFileType(typtr: tptr): boolean;
{
isFileType := (typtr.p.pk = kindFile) or
(typtr.p.pk = kindStruct) and typtr.rep@.flag;
}; (* isFileType *)
%
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
    enums1, enums2: irptr;
    span1, span2: integer;
% ifdef kindrout
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function sameRoutineType(type1, type2: tptr): boolean;
var
    p1, p2: sigptr;
{
    if (type1@.rargc <> type2@.rargc) or
       ((type1@.rflags * [20,21,24,26]) <>
        (type2@.rflags * [20,21,24,26])) then {
        sameRoutineType := false;
        exit;
    };
    if (type1@.rresult <> type2@.rresult) and
       ((type1@.rresult = NIL) or (type2@.rresult = NIL) or
        not typeCheck(type1@.rresult, type2@.rresult)) then {
        sameRoutineType := false;
        exit;
    };
    p1 := type1@.rparams;
    p2 := type2@.rparams;
    while (p1 <> NIL) and (p2 <> NIL) do {
        if (p1@.pclass <> p2@.pclass) then {
            sameRoutineType := false;
            exit;
        };
        if (p1@.ptyp <> p2@.ptyp) and
           ((p1@.ptyp = NIL) or (p2@.ptyp = NIL) or
            not typeCheck(p1@.ptyp, p2@.ptyp)) then {
            sameRoutineType := false;
            exit;
        };
        p1 := p1@.next;
        p2 := p2@.next;
    };
    sameRoutineType := (p1 = NIL) and (p2 = NIL);
}; (* sameRoutineType *)
% endif
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* typeCheck *)
    rangeMismatch := false;
    if not checkTypes or (type1 = type2) then
1:      typeCheck := true
    else
       with type1.rep@ do {
            kind1 := type1.p.pk;
            kind2 := type2.p.pk;
            if (kind1 = kind2) then {
                case kind1 of
                kindReal:
                    (* empty *);
                kindScalar: {
                    (* Two enums must be identical,
                     * all other combinations are okay.
                     *)
                if (type1.rep@.enums = NIL) or (type2.rep@.enums = NIL) then
                        goto 1;
                };
                kindPtr: {
                    if (type1 = voidPtr) or (type2 = voidPtr) or
                         typeCheck(type1.rep@.base, type2.rep@.base) then
                        goto 1;
                };
                kindArray: {
                           span1 := type1.rep@.aright - type1.rep@.aleft;
                           span2 := type2.rep@.aright - type2.rep@.aleft;
                           if typeCheck(type1.rep@.base, type2.rep@.base) and
                       (span1 = span2) and
                           (type1.rep@.pck = type2.rep@.pck) and
                       not rangeMismatch then {
                       if type1.rep@.pck then {
                          if (type1.rep@.pcksize = type2.rep@.pcksize) then
                                goto 1
                        } else
                            goto 1
                    }
                };
                kindFile: {
                    if typeCheck(type1.rep@.base, type2.rep@.base) then
                        goto 1;
                };
% ifdef kindrout
                kindRoutine: {
                    if sameRoutineType(type1, type2) then
                        goto 1;
                }
% endif
                end (* case *)
            };
            typeCheck := false;
        }
}; (* typeCheck *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function argCount(l3arg1z: irptr): integer;
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
    argCount := l3var1z;
}; (* argCount *)
% ifdef kindrout
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function makeRoutineType(routine: irptr): tptr;
var
    resultTyp: tptr;
    srcParam: irptr;
    newParam, lastParam: sigptr;
{
    new(resultTyp.rep, kindRoutine);
    with resultTyp.rep@ do {
        rresult := routine@.typ;
        rparams := NIL;
        rargc := 0;
        rflags := routine@.flags;
    };
    resultTyp.p.psize := 1;
    resultTyp.p.bits := 15;
    resultTyp.p.ptrlev := 0;
    resultTyp.p.basepk := kindRoutine;
    resultTyp.p.pk := kindRoutine;
    lastParam := NIL;
    srcParam := routine@.argList;
    if (srcParam <> NIL) then
        while (srcParam <> routine) do {
            new(newParam);
            with newParam@ do {
                pclass := srcParam@.cl;
                ptyp := srcParam@.typ;
                next := NIL;
            };
            if (lastParam = NIL) then
                resultTyp@.rparams := newParam
            else
                lastParam@.next := newParam;
            resultTyp@.rargc := resultTyp@.rargc + 1;
            lastParam := newParam;
            srcParam := srcParam@.list;
        };
    makeRoutineType := resultTyp;
}; (* makeRoutineType *)
% endif
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function leftAlign: bitset;
{
    while curVal.m * [0..5] = [] do {
        curVal := curVal;
        besm(ASN64-6);
        curVal := ;
    };
    leftAlign := curVal.m;
}; (* leftAlign *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure formOperator(l3arg1z: opgen);
var
    l3int1z, l3int2z, l3int3z : integer;
    nextInsn                  : integer;
    helpExpr                  : eptr;
    direction                 : boolean;
    noTarget                  : boolean;
    l3var10z, l3var11z        : word;
    saved                     : @insnltyp;
    rhsMode                 : boolean;
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
function findLabel: boolean;
{
    l4inl7z := l4inl6z;
    while l4inl7z <> NIL do {
        if (l4inl7z@.mode = curInsn.i) then {
            findLabel := true;
            while (l4inl7z@.code = macro) do {
                l4inl7z := ptr(l4inl7z@.offset);
            };
            exit
        } else {
            l4inl7z := l4inl7z@.next;
        }
    };
    findLabel := false;
}; (* findLabel *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure addJumpInsn(opcode: integer);
{
    if not findLabel then {
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
    usedRegs := usedRegs + insnList@.regsused;
    l4oi212z := insnList@.head;
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
            21: goto 3556;
            0:  addJumpInsn(insnTemp[UZA]);
            1:  addJumpInsn(insnTemp[U1A]);
            2: {
                tempInsn.i := curInsn.i mod 4096;
                curInsn.i := curInsn.i div 4096;
                addJumpInsn(insnTemp[UJ]);
                curInsn.i := tempInsn.i;
3556:           if findLabel then
                    addInsnToBuf(2*macro+ord(l4inl7z))
                else
                    error(206);
            };
            3: {
                 tempInsn.i := curInsn.i mod 4096;
                 curInsn.i := curInsn.i div 4096;
                 l4var213z :=  findLabel;
                 l4inl8z := l4inl7z;
                 curInsn.i := tempInsn.i;
                 l4var213z := l4var213z & findLabel;
                 if l4var213z then
                    with l4inl7z@ do {
                        code := macro;
                        offset := ord(l4inl8z);
                    }
                else
                    error(207);
            };
            mcCOND2INT: addInsnToBuf(3*macro + curInsn.i);
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
            mcADDACC2REG:  add2InsnsToBuf(KATI+14, KMADDJ+I14 + curInsn.i);
            mcROUND: {
                addInsnToBuf(KADD+REAL05);                (* round *)
                add2InsnsToBuf(KNTR+7, KADD+ZERO)
            };
            14: add2InsnsToBuf(indexreg[curInsn.i] + KVTM,
                               KITA + curInsn.i);
            mcMINEL: {
                add2InsnsToBuf(KANX, KSUB+E1);
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
            mcMALLOC: {
                (* MALLOC(N): N is in ACC (placed there by prepLoad).
                   Move N to register 14 and invoke the heap-allocator
                   helper P/PF + 4 (helper #33), which returns the newly
                   allocated pointer in ACC.  Same calling convention as
                   the NEW system procedure. *)
                add2InsnsToBuf(KATI+14, getHelperProc(33));
            };
            end; (* case *)
        } else {
            if 28 in tempInsn.m then {
                addInsnToBuf(getValueOrAllocSymtab(curInsn.i)+tempInsn.i);
            } else {
                curval.i := curInsn.i mod 32768;
                if curVal.i < 2048 then
                    addInsnToBuf(tempInsn.i + curInsn.i)
                else if (curVal.i >= 28672) or (curVal.i < 4096) then {
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
                if (not findLabel) then
                    error(211);
                fixup(0, l4inl7z@.code);
            };
            l4var213z := not l4var213z;
            P3363;
            padToLeft;
            exit iter
        };
        if (curInsn.i >= 2*macro) then {
            l4inl7z := ptr(curInsn.i - (2*macro));
            fixup(0, l4inl7z@.code);
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
    liveRegs := liveRegs - usedRegs;
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
        if tail = NIL then
            head := elt
        else
            tail@.next := elt;
        tail := elt
    }
}; (* addToInsnList *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure addInsnAndOffset(insn, l4arg2z: integer);
{
    addToInsnList(insn);
    insnlist@.tail@.offset := l4arg2z
}; (* addInsnAndOffset *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure prependToInsnList(insn: integer);
var
    elt: oiptr;
{
    new(elt);
    with elt@ do {
        next := insnList@.head;
        mode := 0;
        code := insn;
        offset := 0;
    };
    if (insnList@.head = NIL) then {
        insnList@.tail := elt;
    };
    insnList@.head := elt;
}; (* prependToInsnList *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure prepLoad;
label
    4545, 4602;
var
    helper, baseWidth, offsetShift: integer;
    valueType: tptr;
    typKind: kind;
    addrState: state;
    isSimple, needsMask: boolean;
{
    valueType := insnList@.typ;
    with insnList@ do {
        case ilm of
        ilCONST: {
            curVal := payload;
            if (valueType.p.psize = 1) then
                curVal.i := getFCSToffset;
            addToInsnList(constRegTemplate + curInsnTemplate + curVal.i);
        };
        ilLVAL: {
            helper := insnList@.addrmd;
            baseWidth := insnList@.payload.i;
            offsetShift := insnList@.disp;
            if (15 < helper) then {
                (* empty *)
            } else {
                if (helper = 15) then {
                    addToInsnList(macro + mcACC2ADDR);
                } else {
                    helper := indexreg[insnList@.addrmd];
                    if (baseWidth = 0) and (insnList@.st = stWORD) then {
                        addInsnAndOffset(helper + curInsnTemplate,
                                         offsetShift);
                        goto 4602;
                    } else {
                        addToInsnList(helper + insnTemp[UTC]);
                    }
                }
            };
            addrState := insnList@.st;
            if addrState = stWORD then {
                addInsnAndOffset(baseWidth + curInsnTemplate, offsetShift);
            } else {
                typKind := valueType.p.pk;
                if (typKind < kindArray) or
                   (typKind = kindStruct) and (s6 in optSflags.m) then {
                    isSimple := true;
                } else
                    isSimple := false;
                if addrState = stSLICE then {
                    if (offsetShift <> baseWidth) or
                       (helper <> 18) or
                       (baseWidth <> 0) then
                        addInsnAndOffset(baseWidth + insnTemp[XTA],
                                         offsetShift);
                    offsetShift := insnList@.shift;
                    baseWidth := insnList@.width;
                    helper := offsetShift + baseWidth;
                    if isSimple then {
% The commented out optimization is specific to the original BESM-6
% without a barrel shifter. Now there is no need for it.
%                       if (30 < offsetShift) then {
%                           addToInsnList(ASN64-48 + offsetShift);
%                           addToInsnList(insnTemp[YTA]);
%                       } else {
                            if (offsetShift <> 0) then
                                addToInsnList(ASN64 + offsetShift);
%                       };
                        if helper <> 48 then {
                            curVal.m := [(48 - baseWidth)..47];
                            addToInsnList(KAAX+I8 + getFCSToffset);
                        }
                    } else {
                        if (helper <> 48) then
                            addToInsnList(ASN64-48 + helper);
                        curVal.m := [0..(baseWidth-1)];
                        addToInsnList(KAAX+I8 + getFCSToffset);
                    };
                } else {
                    if isSimple then
                        helper := 74 (* P/LDAR *)
                    else
                        helper := 56; (* P/RR *)
                    addToInsnList(getHelperProc(helper));
                    insnList@.tail@.mode := 1;
                }
            };
            goto 4545;
        };
        ilRVAL: {
4545:       if forValue and (valueType = booleanType) and
               (16 in insnList@.regsused) then
                addToInsnList(KAEX+E1);
        };
        ilCOND: {
            if forValue then
                addInsnAndOffset(macro+mcCOND2INT,
                    ord(16 in insnList@.regsused)*10000B + insnList@.payload.i);
        };
        end; (* case *)
4602:
    }; (* with *)
    with insnList@ do {
        ilm := ilRVAL;
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
    l4var1z.i := insnList@.disp;
    l4var1z.i := l4var1z.i mod 32768;
    l4var6z := l4var1z.i
}; (* P4613 *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* setAddrTo *)
    with insnList@ do {
        l4int2z := addrmd;
        opCode := insnTemp[VTM];
        regField := indexreg[reg];
        l4var4z := payload.i;
        regsused := regsused + [reg];
        if (ilm = ilCONST) then {
            curVal := payload;
            if (typ.p.psize = 1) then
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
            l4var4z := insnList@.disp;
            l4var5z := insnList@.tail@.code - insnTemp[UTC];
            if (l4var4z <> 0) then {
                l4var1z.i := macro * l4var5z + l4var4z;
                l4var5z := allocSymtab(l4var1z.m * [12:47]);
            };
            insnList@.tail@.code := regField + l4var5z + opCode;
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
    insnList@.ilm := ilLVAL;
    insnList@.addrmd := reg;
    insnList@.disp := 0;
    insnList@.payload.i := 0;
}; (* setAddrTo *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure prepStore;
var
    l4int1z: integer;
    l4int2z, l4int3z: integer;
    l4bool4z, l4bool5z: boolean;
    addrState: state;
    typKind: kind;
{
    with insnList@ do
        l4int1z := addrmd;
    if (15 < l4int1z) then {
        (* nothing? *)
    } else if (l4int1z = 15) then {
        addToInsnList(macro + mcACC2ADDR)
    } else {
        addToInsnList(indexreg[l4int1z] + insnTemp[UTC]);
    };
    l4bool4z := 0 in insnList@.regsused;
    addrState := insnList@.st;
    if (addrState <> stWORD) or l4bool4z then
        prependToInsnList(macro + mcPUSH);
    if (addrState = stWORD) then {
        if (l4bool4z) then {
            addInsnAndOffset(insnList@.payload.i + insnTemp[UTC],
                             insnList@.disp);
            addToInsnList(macro+mcPOP2ADDR);
        } else {
            addInsnAndOffset(insnList@.payload.i, insnList@.disp);
        }
    } else {
        typKind := insnList@.typ.p.pk;
        l4int1z := insnList@.typ.p.bits;
        l4bool5z := (typKind < kindArray) or
                     (typKind = kindStruct) and (S6 in optSflags.m);
        if (addrState = stSLICE) then {
            l4int2z := insnList@.shift;
            l4int3z := l4int2z + insnList@.width;
            if l4bool5z then {
                if (l4int2z <> 0) then
                    prependToInsnList(ASN64 - l4int2z);
            } else {
                if (l4int3z <> 48) then
                    prependToInsnList(ASN64 + 48 - l4int3z);
            };
            addInsnAndOffset(insnTemp[UTC] + insnList@.payload.i,
                             insnList@.disp);
            curVal.m := [0..47] - [(48-l4int3z)..(47 -l4int2z)];
            addInsnAndOffset(macro+22, getFCSToffset);
        } else {
            if not l4bool5z then {
                l4int2z := (insnList@.width - l4int1z);
                if (l4int2z <> 0) then
                    prependToInsnList(ASN64 - l4int2z);
                prependToInsnList(insnTemp[YTA]);
                prependToInsnList(ASN64 - l4int1z);
            };
            addToInsnList(getHelperProc(77)); (* "P/STAR" *)
            insnList@.tail@.mode := 1;
        }
    }
}; (* prepStore *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure spillAcc(op: operator);
{
    addInsnAndOffset(curFrameRegTemplate, localSize);
    new(curExpr);
    curExpr@.vt.typ := insnList@.typ;
    genOneOp;
    curExpr@.op := op;
    curExpr@.num1 := localSize;
    localSize := localSize + 1;
    if (l2int21z < localSize) then
        l2int21z := localSize;
}; (* spillAcc *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function insnCount: integer;
var
    cnt: integer;
    cur: oiptr;
{
    cnt := 0;
    cur := insnList@.head;
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
    7567, 7760, 10122;
var
    arg1Const, arg2Const: boolean;
    otherIns: @insnltyp;
    arg1Val, arg2Val: word;
    curOP: operator;
    work: integer;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure startLVal;
{
    prepLoad;
    insnList@.ilm := ilLVAL;
    insnList@.st := stWORD;
    insnList@.disp := 0;
    insnList@.payload.i := 0;
    insnList@.addrmd := 18;
}; (* startLVal *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genDeref;
label
    5220;
var
    l5var1z, l5var2z: word;
{
    (* The optimised path manipulates addrmd/disp/payload, which only
       carry meaning for ilLVAL (addressable) operands.  For ilRVAL --
       e.g. a function-call result whose pointer value is already in ACC
       -- those fields are uninitialised and would yield garbage; in
       that case fall through to the general mcACC2ADDR path below. *)
    if (insnList@.ilm = ilLVAL) and (
        (insnList@.st = stWORD) or
        (insnList@.st = stSLICE) and
        (insnList@.shift = 0))
    then {
        l5var1z.i := insnList@.addrmd;
        l5var2z.i := insnList@.disp;
        if (l5var1z.i = 18) or (l5var1z.i = 16) then {
5220:       addInsnAndOffset((insnList@.payload.i + insnTemp[WTC]), l5var2z.i);
        } else {
            if (l5var1z.i = 17) then {
                if (l5var2z.i = 0) then {
                    insnList@.tail@.code := insnList@.tail@.code +
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
        startLVal;
        addToInsnList(macro + mcACC2ADDR);
    };
    insnList@.disp := 0;
    insnList@.payload.i := 0;
    insnList@.addrmd := 16;
    insnList@.st := stWORD;
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
    saved@.tail@.next := insnList@.head;
    insnList@.head := saved@.head;
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
    l5var2z@.tail@.next := insnList@.head;
    l5var2z@.tail := insnList@.tail;
    insnList := l5var2z;
}; (* prepMultiWord *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure negateCond;
{
    if (insnList@.ilm = ilCONST) then {
        insnList@.payload.b := not insnList@.payload.b;
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
    insnList@.tail@.mode := 0;
    saved@.tail@.next := insnList@.head;
    insnList@.head := saved@.head;
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
        l5var5z := condLabCnt;
        condLabCnt := condLabCnt + 1;
        forValue := false;
        l5var6z := ord(l5var1z) + macro;
        l5var7z := ord(l5var2z) + macro;
        if (insnList@.ilm = ilCOND) then {
            l5var3z := insnList@.payload.i;
        } else {
            l5var3z := 0;
            prepLoad;
        };
        if (otherIns@.ilm = ilCOND) then {
            l5var4z := otherIns@.payload.i;
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
        l5ins8z@.tail@.next := insnList@.head;
        insnList@.head := l5ins8z@.head;
        insnList@.ilm := ilCOND;
        insnList@.payload.i := l5var5z;
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
    l5var26z: tptr;
    l5ilm28z: ilmode;
    l5var29z: eptr;
    getEltInsns: array [1..10] of @insnltyp;
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
    l5var22z.m := freeRegs;
    for curDim to dimCnt do
       l5var22z.m := l5var22z.m - getEltInsns[curDim]@.regsused;
    for curDim := dimCnt downto 1 do {
        l5var26z := insnCopy.typ.rep@.base;
        l5var25z := insnCopy.typ.rep@.pck;
        l5var7z := insnCopy.typ.rep@.aleft;
        l5var8z := l5var26z.p.psize;
        if not l5var25z then
            insnCopy.disp := insnCopy.disp - l5var8z * l5var7z;
        insnList := getEltInsns[curDim];
        l5ilm28z := insnList@.ilm;
        if (l5ilm28z = ilCONST) then {
            curVal := insnList@.payload;
            curVal.m := curVal.m +  intZero;
            if (curVal.i < l5var7z) or
                (insnCopy.typ.rep@.aright < curVal.i) then
                error(29); (* errIndexOutOfBounds *)
            if (l5var25z) then {
                l5var4z := curVal.i - l5var7z;
                l5var5z := insnCopy.typ.rep@.perword;
                insnCopy.regsused := insnCopy.regsused + [0];
                insnCopy.disp := l5var4z DIV l5var5z + insnCopy.disp;
                l5var6z := (l5var5z-1-l5var4z MOD l5var5z) *
                           insnCopy.typ.rep@.pcksize;
                case insnCopy.st of
                stWORD: insnCopy.shift := l5var6z;
                stSLICE: insnCopy.shift := insnCopy.shift + l5var6z +
                               insnCopy.typ.p.bits - 48;
                stPACKED: error(errUsingVarAfterIndexingPackedArray);
                end; (* case *)
                insnCopy.width := insnCopy.typ.rep@.pcksize;
                insnCopy.st := stSLICE;
            }  else {
                insnCopy.disp := curVal.i  * l5var26z.p.psize +
                                  insnCopy.disp;
            }
        } else {
            if (l5var8z <> 1) then {
                prepLoad;
                addToInsnList(insnCopy.typ.rep@.perword);
                insnList@.tail@.mode := 1;
                if (l5var7z >= 0) then
                    addToInsnList(KYTA+64)
                else
                    addToInsnList(macro + mcMULTI);
           };
           if (l5ilm28z = ilCOND) or
              (l5ilm28z = ilLVAL) and
              (insnList@.st <> stWORD) then
               prepLoad;
           l5var23z.m := insnCopy.regsused + insnList@.regsused;
           if (not l5var25z) then {
               if (insnCopy.addrmd = 18) then {
                    if (insnList@.ilm = ilRVAL) then {
                        insnCopy.addrmd := 15;
                    } else {
                        insnCopy.addrmd := 16;
                        curInsnTemplate := insnTemp[WTC];
                        prepLoad;
                        curInsnTemplate := insnTemp[XTA];
                    };
                    insnCopy.tail := insnList@.tail;
                    insnCopy.head := insnList@.head;
                } else {
                    if (insnCopy.addrmd >= 15) then {
                        l5var1z :=  minel(l5var22z.m);
                        if (0 >= l5var1z) then {
                            l5var1z := minel(freeRegs - insnCopy.regsused);
                            if (0 >= l5var1z) then
                                l5var1z := 9;
                        };
                        saved := insnList;
                        insnList := copyPtr;
                        l5var23z.m := l5var23z.m + [l5var1z];
                        if (insnCopy.addrmd = 15) then {
                            addToInsnList(insnTemp[ATI] + l5var1z);
                        } else {
                            addToInsnList(indexreg[l5var1z] + insnTemp[VTM]);
                        };
                        insnCopy.addrmd := l5var1z;
                        insnCopy.regsused := insnCopy.regsused + [l5var1z];
                        insnList := saved;
                    } else {
                            l5var1z := insnCopy.addrmd;
                    };
                    if (l5var1z IN insnList@.regsused) then {
                         P4606;
                         insnList@.tail@.next := insnCopy.head;
                         insnCopy.head := insnList@.head;
                         insnList := copyPtr;
                         addInsnAndOffset(macro+mcADDSTK2REG, l5var1z);
                    } else {
                         if (insnList@.ilm = ilRVAL) then {
                             addInsnAndOffset(macro+mcADDACC2REG, l5var1z);
                         } else {
                             curInsnTemplate := insnTemp[WTC];
                             prepLoad;
                             curInsnTemplate := insnTemp[XTA];
                             addToInsnList(indexreg[l5var1z] + insnTemp[UTM]);
                         };
                         insnCopy.tail@.next := insnList@.head;
                         insnCopy.tail := insnList@.tail;
                     }
                };
           } else {
                if (insnCopy.st = stWORD) then {
                    prepLoad;
                    if (l5var7z <> 0) then {
                        curVal.i := 0 - l5var7z;
                        curVal.m := curVal.m - intZero;
                        addToInsnList(KADD+I8 + getFCSToffset);
                        insnList@.tail@.mode := 1;
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
                    insnCopy.st := stPACKED;
                    insnCopy.disp := 0;
                    insnCopy.payload.i := 0;
                    insnCopy.width := insnCopy.typ.rep@.pcksize;
                    curVal.i := insnCopy.width;
                    if (curVal.i = 24) then
                        curVal.i := 7;
                    curVal := curVal;besm(ASN64-24);curVal:=;
                    addToInsnList(allocSymtab(  (* P/00C *)
                        helperNames[76] + curVal.m)+(KVTM+I11));
                    insnCopy.addrmd := 16;
                    insnCopy.shift := 0;
                    saved@.tail@.next := insnCopy.head;
                    insnCopy.head := saved@.head;
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
    l5idr3z, l5idr4z, l5idr5z, curForml: irptr;
    isProc, firstArg, isDirect, isFortrn, isAssembler, allByRef: boolean;
    calleeFl, frameSiz, numArgs: word;
    l5var15z: integer;
    l5var16z, l5var17z, l5var18z, l5var19z: word;
    l5inl20z: @insnltyp;
    l5op21z: operator; paramClass: idclass;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function allocGlobalObject(l6arg1z: irptr): integer;
{
    if (l6arg1z@.pos = 0) then {
        if (l6arg1z@.flags * [20, 21] <> []) then {
            curVal := l6arg1z@.id;
            curVal.m := leftAlign;
            l6arg1z@.pos := allocExtSymbol(extSymMask);
        } else {
            l6arg1z@.pos := symTabPos;
            putToSymTab([]);
        }
    };
    allocGlobalObject := l6arg1z@.pos;
}; (* allocGlobalObject *)
%
{ (* genEntry *)
    l5exp1z := exprToGen@.expr1;
    l5idr5z := exprToGen@.id2;
    isProc := (l5idr5z@.typ = voidType);
    isDirect := (l5idr5z@.list = NIL);
    if (isProc) then
        frameSiz.i := 3 else frameSiz.i := 4;
    calleeFl.m := l5idr5z@.flags;
    isFortrn := 21 in calleeFl.m;
    isAssembler := 26 in calleeFl.m;
    allByRef := 24 in calleeFl.m;
    if (isDirect) then {
        numArgs.i := argCount(l5idr5z);
        curForml := l5idr5z@.argList;
    } else {
        frameSiz.i := frameSiz.i + 2;
    };
    new(insnList);
    insnList@.head := NIL;
    insnList@.tail := NIL;
    insnList@.typ := l5idr5z@.typ;
    insnList@.regsused := (l5idr5z@.flags + [7:15]) * [0:8, 10:15];
    insnList@.ilm := ilRVAL;
    if (isAssembler) then {
        firstArg := false;
    } else if (isFortrn) then {
        firstArg := not isProc;
        if (checkFortran) then {
            addToInsnList(getHelperProc(92)); (* "P/MF" *)
        }
    } else {
        firstArg := true;
        if (not isDirect) and (l5exp1z <> NIL)
            or (isDirect) and (numArgs.i >= 2) then {
            addToInsnList(KUTM+SP + frameSiz.i);
        };
    };
    numArgs.i := 0;
    while l5exp1z <> NIL do {
        l5exp2z := l5exp1z@.expr2;
        l5exp1z := l5exp1z@.expr1;
        l5op21z := l5exp2z@.op;
        numArgs.i := numArgs.i + 1;
        l5inl20z := insnList;
        if (l5op21z = PCALL) or (l5op21z = FCALL) then {
            l5idr4z := l5exp2z@.id2;
            new(insnList);
            insnList@.head := NIL;
            insnList@.tail := NIL;
            insnList@.regsused := [];
            usedRegs := usedRegs + l5idr4z@.flags;
            if (l5idr4z@.list <> NIL) then {
                addToInsnList(l5idr4z@.offset + insnTemp[XTA] +
                              l5idr4z@.value);
                if (isFortrn) then
                    addToInsnList(getHelperProc(19)); (* "P/EA" *)
            } else
(a)         {
                if (l5idr4z@.value = 0) then {
                    if (isFortrn) and (21 in l5idr4z@.flags) then {
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
                        l5var15z := ord(l5idr4z@.typ <> voidType);
                        l5var17z.i := argCount(l5idr4z);
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
                                paramClass := l5idr3z@.cl;
                                if (paramClass = ROUTINEID) and
                                   (l5idr3z@.typ <> voidType) then
                                    paramClass := ENUMID;
                                form2Insn(0, ord(paramClass));
                                l5idr3z := l5idr3z@.list;
                            until (l5idr4z = l5idr3z);
                        };
                        storeObjWord([]);
                        fixup(0, l5var16z.i);
                    }
                };
                addToInsnList(KVTM+I14 + l5idr4z@.value);
                if 21 in l5idr4z@.flags then
                    addToInsnList(KITA+14)
                else
                    addToInsnList(getHelperProc(64)); (* "P/PB" *)
            };
            if (l5op21z = PCALL) then
                paramClass := ROUTINEID
            else
                paramClass := ENUMID;
        } else {
            genFullExpr(l5exp2z);
            if (insnList@.ilm = ilLVAL) then
                paramClass := FORMALID
            else
                paramClass := VARID;
        };
        if not (not isDirect or (paramClass <> FORMALID) or
               (curForml@.cl <> VARID)) then
            paramClass := VARID;
(loop)      if (paramClass = FORMALID) or (allByRef) then {
            setAddrTo(14);
            addToInsnList(KITA+14);
        } else if (paramClass = VARID) then {
            if (insnList@.typ.p.psize <> 1) then {
                paramClass := FORMALID;
                goto loop;
            } else {
                prepLoad;
            }
        };
        if not firstArg then
            prependToInsnList(macro + mcPUSH);
        firstArg := false;
        if (l5inl20z@.tail <> NIL) then {
            l5inl20z@.tail@.next := insnList@.head;
            insnList@.head := l5inl20z@.head;
        };
        insnList@.regsused := insnList@.regsused + l5inl20z@.regsused;
        if not isDirect then {
            curVal.cl := paramClass;
            addToInsnList(KXTS+I8 + getFCSToffset);
        };
        if isDirect and not allByRef then
            curForml := curForml@.list;
    }; (* while -> 7061 *)
    if isFortrn then {
        addToInsnList(KNTR+2);
        insnList@.tail@.mode := 4;
    };
    if isDirect then {
        addToInsnList(allocGlobalObject(l5idr5z) + (KVJM+I13));
        if (20 in l5idr5z@.flags) then {
            l5var17z.i := 1;
        } else {
            l5var17z.i := l5idr5z@.offset div 4000000B;
        }
    } else {
        l5var15z := 0;
        if (numArgs.i = 0) then {
            l5var17z.i := frameSiz.i + 1;
        } else {
            l5var17z.i := -(2 * numArgs.i + frameSiz.i);
            l5var15z := 1;
        };
        addInsnAndOffset(macro+16 + l5var15z,
                         getValueOrAllocSymtab(l5var17z.i));
        addToInsnList(l5idr5z@.offset + insnTemp[UTC] + l5idr5z@.value);
        addToInsnList(macro+18);
        l5var17z.i := 1;
    };
    insnList@.tail@.mode := 2;
    if (not isAssembler) and (curProcNesting <> l5var17z.i) then {
        if not isFortrn then {
            if (l5var17z.i + 1 = curProcNesting) then {
                addToInsnList(KMTJ+I7 + curProcNesting);
            } else {
                l5var15z := frameRestore[curProcNesting][l5var17z.i];
                if (l5var15z = (0)) then {
                    curVal.i := 4317T; (* C/ *)
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
    if (not isAssembler) and
       (not isDirect or ([20, 21] * calleeFl.m <> [])) then {
        addToInsnList(KVTM+40074001B);
    };
    usedRegs := (usedRegs + calleeFl.m) * [1:15];
    if isFortrn then {
        if (not checkFortran) then
            addToInsnList(KNTR+7)
        else
            addToInsnList(getHelperProc(93));    (* "P/FM" *)
        insnList@.tail@.mode := 2;
    };
    if not isProc then {
        insnList@.typ := l5idr5z@.typ;
        insnList@.regsused := insnList@.regsused + [0];
        insnList@.ilm := ilRVAL;
        liveRegs := liveRegs - calleeFl.m;
    }

}; (* genEntry *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure startInsnList(l5arg1z: ilmode);
{
    new(insnList);
    insnList@.tail := NIL;
    insnList@.head := NIL;
    insnList@.typ := exprToGen@.vt.typ;
    insnList@.regsused := [];
    insnList@.ilm := l5arg1z;
    if (l5arg1z = ilCONST) then {
        insnList@.payload.i := exprToGen@.num1;
        insnList@.addrmd := exprToGen@.num2;
    } else {
        insnList@.st := stWORD;
        insnList@.addrmd := 18;
        insnList@.payload.i := curFrameRegTemplate;
        insnList@.disp := exprToGen@.num1;
    }
}; (* startInsnList *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genCopy;
var
    size: integer;
    lhsIns, rhsIns: @insnltyp;
{
    size := insnList@.typ.p.psize;
    if (size = 1) then {
        lhsIns := insnList;
        insnList := otherIns;
        prepLoad;
        rhsIns := insnList;
        insnList := lhsIns;
        prepStore;
        lhsIns := insnList;
        if (rhsIns@.tail = NIL) then {
            rhsIns@.head := lhsIns@.head;
        } else {
            rhsIns@.tail@.next := lhsIns@.head;
        };
        if (lhsIns@.tail <> NIL) then
            rhsIns@.tail := lhsIns@.tail;
        rhsIns@.regsused := rhsIns@.regsused + lhsIns@.regsused + [0];
        rhsIns@.ilm := ilRVAL;
        insnList := rhsIns;
    } else {
        prepMultiWord;
        genOneOp;
        size := size - 1;
        formAndAlign(KVTM+I13 + getValueOrAllocSymtab(-size));
        work := moduleOffset;
        form2Insn(KUTC+I14 + size, KXTA+I13);
        form3Insn(KUTC+I12 + size, KATX+I13,
                  KVLM+I13 + work);
        usedRegs := usedRegs + [12:14];
    }
}; (* genCopy *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genConstDiv;
var r :  real;
{
    if card(arg2val.m) > 1 then {
        arg2Val.m := arg2Val.m + intZero;
        curVal.r := 1.0 / arg2Val.i;
        r := curVal.r * arg2Val.i;
        curVal.m := curVal.m * [7..47] + intZero;
        (*=r- forcing exact comparisons of reals,
         * the stock Pascal compiler needs it.
         *)
        if (r < 1.0) then
            curVal.i := curVal.i + 1;
        curVal.m := curVal.m - [1,3];
        addToInsnList(KMUL+I8 + getFCSToffset);
    };
    addToInsnList(ASN64+47-minel(arg2val.m - intZero))
}; (* genConstDiv *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genComparison;
label
    7504, 7514;
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
                insnList@.payload.b := arg1Val.i IN arg2Val.m;
            } else {
                l5set2z := [arg1Val.i];
                if (l5set2z = []) then {
                    insnList@.payload.b := false;
                } else {
                    insnList := otherIns;
                    prepLoad;
                    curVal.m := l5set2z;
                    addToInsnList(KAAX+I8 + getFCSToffset);
                    insnList@.payload.i := 0;
                    insnList@.ilm := ilCOND;
                }
            };
        } else {
            saved := insnList;
            insnList := otherIns;
            otherIns := saved;
            nextInsn := 66;      (* P/IN *)
            genHelper;
            insnList@.ilm := ilRVAL;
        }
    } else {
        if negate then
            l3int3z := l3int3z - 1;
        l2typ13z := insnList@.typ;
        curVarKind := l2typ13z.p.pk;
        size := l2typ13z.p.psize;
        if (l2typ13z = realType) then {
            work := 1;
        } else if (curVarKind = kindScalar) then
            work := 3
        else {
            work := 4;
        };
        if (size <> 1) then {
            prepMultiWord;
            addInsnAndOffset(KVTM+I11, 1 - size);
            addToInsnList(getHelperProc(89 + l3int3z)); (* P/EQ *)
            insnList@.ilm := ilRVAL;
            negate := not negate;
        } else  if l3int3z = 0 then {
            nextInsn := insnTemp[AEX];
            tryFlip(true);
7504:       insnList@.ilm := ilCOND;
            insnList@.payload.i := 0;
        } else {
            case work of
            1: {
                mode := 3;
7514:           nextInsn := insnTemp[SUB];
                tryFlip(false);
                insnList@.tail@.mode := mode;
                if mode = 3 then {
                    addToInsnList(KNTR+23B);
                    insnList@.tail@.mode := 2;
                };
                goto 7504;
            };
            3: {
                mode := 1;
                goto 7514;
            };
            4: {
                nextInsn := insnTemp[ARX];
                prepLoad;
                addToInsnList(KAEX+ALLONES);
                tryFlip(true);
                goto 7504;
            };
            end; (* case *)
        };
        insnList@.regsused := insnList@.regsused - [16];
        if (negate)
            then negateCond;
    };

}; (* genComparison *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genCondOp;
(*
 * Code generation for the ternary conditional operator
 *      exprToGen = CONDOP{cond, ALTERN{then, else}}
 *
 * Strategy: build a single deferred ilRVAL insnList that, when committed by
 * the surrounding expression via genOneOp, emits the full branching
 * sequence and leaves the result in the accumulator. Both branches simply
 * leave their value in ACC -- no PUSH/POP is needed inside genCondOp,
 * because the framework (tryFlip / genCopy / genEntry) takes care of
 * spilling ACC to the hardware stack only when the surrounding expression
 * actually contends for reg 0.
 *
 * The emitted layout is:
 *
 *      <cond chain>             ; sets ACC, leaving sign per direction
 *      UZA/U1A elseLab          ; jump to elseLab on FALSE
 *      <then chain>             ; ACC := then-value
 *      UJ   endLab
 *  elseLab:
 *      <else chain>             ; ACC := else-value
 *  endLab:
 *                               ; result of CONDOP is in ACC
 *
 * Building this deferred chain reuses the same macro machinery that
 * genBoolAnd / genComparison use to express forward jumps and labels:
 *
 *   macro + 0, label           -> UZA forward jump to label (jump on FALSE)
 *   macro + 1, label           -> U1A forward jump to label (jump on TRUE-side
 *                                 of an inverted/negated condition)
 *   macro + 2, hi*10000B + lo  -> UJ to label `hi` AND define label `lo` here
 *   macro + 21, label          -> define label here (no jump)
 *
 * Labels are simply unique integers drawn from condLabCnt. When genOneOp later
 * processes a macro+0/1/2 entry, addJumpInsn looks up an existing label
 * record by `mode = label`; if found (findLabel), the new jump is linked into
 * that record's chain in the object buffer, so several jumps to the same
 * logical label share one record and are all patched together by fixup.
 *)
var
    altExpr: eptr;
    elseLab, endLab: integer;
    condChain, thenChain: @insnltyp;
{
    altExpr := exprToGen@.expr2;
    (* Allocate two unique forward-label identifiers from the global
       counter used by genBoolAnd. *)
    elseLab := condLabCnt;
    condLabCnt := condLabCnt + 1;
    endLab := condLabCnt;
    condLabCnt := condLabCnt + 1;

    (* Build the condition sub-chain: <cond chain> + branch-to-elseLab-if-false.
       We set forValue := false before generating the condition so that
       genComparison / genBoolAnd leave the chain in ilCOND form (deferred
       conditional), avoiding the wasteful mcCOND2INT sequence that would
       first materialize a 0/1 in ACC and then re-test it.
       Direction = (16 in regsused) follows the convention used by
       formOperator(BRANCH): when set, the embedded condition is "negated"
       and a true-condition path is selected by U1A (macro+1); otherwise
       UZA (macro+0) jumps when ACC>=0 (sign clear), i.e., on FALSE. *)
    forValue := false;
    curExpr := exprToGen@.expr1;
    genFullExpr(curExpr);
    if (insnList@.ilm = ilCOND) and (insnList@.payload.i <> 0) then {
        (* Compound boolean (a && b, a || b, ...): the chain already has an
           embedded forward jump to label `payload`. *)
        if (16 in insnList@.regsused) then
            (* direction=true: embedded jumps fire on FALSE -> reuse them
               as our elseLab jumps; no extra instruction needed. *)
            elseLab := insnList@.payload.i
        else
            (* direction=false: embedded jumps fire on TRUE -> emit
               "UJ elseLab" and define payload at this point so the
               original jumps fall through to the then-branch. *)
            addInsnAndOffset(macro + 2,
                             elseLab * 10000B + insnList@.payload.i);
    } else {
        (* Simple ilCOND (single comparison, payload=0), or ilLVAL/ilRVAL/ilCONST:
           prepLoad with forValue=false is a no-op for ilCOND, and materializes
           an lvalue/constant to ACC otherwise. Then emit a single deferred
           UZA/U1A to elseLab. *)
        prepLoad;
        if (16 in insnList@.regsused) then
            addInsnAndOffset(macro + 1, elseLab)
        else
            addInsnAndOffset(macro + 0, elseLab);
    };
    forValue := true;
    condChain := insnList;

    (* Build the then-branch sub-chain:
           <then chain> + "UJ endLab; elseLab:"
       prepLoad materializes the then-value into ACC. The combined macro+2
       entry then does two things at commit time: it emits the unconditional
       jump to endLab (so the then-path skips over the else-path), and it
       defines elseLab at the current emit position so the earlier UZA/U1A
       forward jump can be patched here. *)
    curExpr := altExpr@.expr1;
    genFullExpr(curExpr);
    prepLoad;
    addInsnAndOffset(macro + 2, endLab * 10000B + elseLab);
    thenChain := insnList;

    (* Build the else-branch sub-chain:
           <else chain> + "endLab:"
       prepLoad materializes the else-value into ACC. macro+21 is a pure
       label-definition marker: it requires findLabel to find an existing
       label record for endLab (the macro+2 above already created it via
       addJumpInsn) and patches the UJ jump to land here. *)
    curExpr := altExpr@.expr2;
    genFullExpr(curExpr);
    prepLoad;
    addInsnAndOffset(macro + 21, endLab);

    (* Concatenate the three sub-chains head-to-tail in evaluation order:
       cond -> then -> else. After this, condChain is the single chain
       that represents the complete deferred ternary computation. *)
    condChain@.tail@.next := thenChain@.head;
    condChain@.tail := thenChain@.tail;
    condChain@.tail@.next := insnList@.head;
    condChain@.tail := insnList@.tail;

    (* Union the regsused sets of all three sub-chains so the surrounding
       expression sees the full set of registers/conditions that the
       ternary touches. Add reg 0 (ACC) since the result lives there. *)
    condChain@.regsused := condChain@.regsused + thenChain@.regsused
                           + insnList@.regsused + [0];

    (* Finalize the result insnList as an ilRVAL (rvalue-in-ACC). Both branches
       leave their value in ACC and the chains rejoin at endLab, so the
       composite chain already meets the ilRVAL contract without any extra
       PUSH/POP. tryFlip / genCopy / genEntry integrate this value into
       the surrounding expression exactly like any other ilRVAL chain, spilling
       ACC to the hardware stack only when actually needed. *)
    insnList := condChain;
    insnList@.typ := exprToGen@.vt.typ;
    insnList@.ilm := ilRVAL;
    insnList@.st := stWORD;
}; (* genCondOp *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure genRMWAssign;
(* Code-gen for RMWASSIGN(lhs, inner-op(rhs, NIL)): a read-modify-write
   assignment (compound op-assign such as `lhs += rhs`, or pre-++/--).
   The naked AST lowering ASSIGNOP(lhs, op(lhs, rhs)) evaluates `lhs`
   twice, which is wrong when computing its address has side effects
   (or even just clobbers ACC).
   Strategy: compute lhs's address exactly once (walking the lvalue
   subtree only once), push that address onto the BESM-6 stack TWICE,
   then synthesise ASSIGNOP(stklval, op(stklval, rhs)) where stklval is
   a sentinel STKLVAL node.  Each visit to STKLVAL (one for the inner
   read, one for the outer write) emits `WTC SP' to pop one copy of
   the address into the working tag, after which prepLoad/prepStore
   append `XTA 0' / `ATX 0' to do the actual transfer.  Pushing twice
   (rather than peeking SP-1) works around the BESM-6 not having a
   non-popping SP-relative read.
   For trivial lvalues that don't touch ACC (plain GETVAR / GETFIELD
   off GETVAR / GETELT with GETVAR index) the synthetic AST may share
   the original lvalue node directly -- re-walking is cheap and side-
   effect free, and skips the push/pop traffic entirely. *)
var
    innerNode, rhsExpr, lhsExpr, rmwLhs: eptr;
    synthOp, synthAsn: eptr;
    innerOp: operator;
    needsMater: boolean;
{
    lhsExpr := exprToGen@.expr1;
    innerNode := exprToGen@.expr2;
    innerOp := innerNode@.op;
    rhsExpr := innerNode@.expr1;

    needsMater := (lhsExpr@.op <> GETVAR) and
                  ((lhsExpr@.op <> GETFIELD) or
                   (lhsExpr@.expr1@.op <> GETVAR)) and
                  ((lhsExpr@.op <> GETELT) or
                   (lhsExpr@.expr2@.op <> GETVAR));

    if needsMater then {
        rhsMode := false;
        genFullExpr(lhsExpr);
        rhsMode := true;
        if (insnList@.st <> stWORD) then {
            error(errVarTooComplex);
            exit;
        };
        setAddrTo(14);
        addToInsnList(KITA + 14);
        addToInsnList(macro + mcPUSH);
        addToInsnList(KITA + 14);
        addToInsnList(macro + mcPUSH);
        genOneOp;
        insnList := NIL;
        new(rmwLhs);
        with rmwLhs@ do {
            vt.typ := lhsExpr@.vt.typ;
            op := STKLVAL;
            expr1 := NIL;
            expr2 := NIL;
        };
    } else {
        rmwLhs := lhsExpr;
    };

    new(synthOp);
    with synthOp@ do {
        vt.typ := innerNode@.vt.typ;
        op := innerOp;
        expr1 := rmwLhs;
        expr2 := rhsExpr;
    };

    new(synthAsn);
    with synthAsn@ do {
        vt.typ := exprToGen@.vt.typ;
        op := ASSIGNOP;
        expr1 := rmwLhs;
        expr2 := synthOp;
    };

    genFullExpr(synthAsn);
}; (* genRMWAssign *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function shift(val:bitset; amt:integer):bitset;
var i    : integer; ret: word;
{
    ret.i := amt;
    ret.m := ret.m + intZero;
    amt := ret.i;
    ret.m := [];
    for i := 0 to 47 do if (i-amt) in val then ret.m := ret.m + [i];
    shift := ret.m;
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* genFullExpr *);
    if exprToGen = NIL then
        exit;
7567:
    curOP := exprToGen@.op;
    if (curOP = CONDOP) then {
        genCondOp;
        exit;
    };
    if (curOP = RMWASSIGN) then {
        genRMWAssign;
        exit;
    };
    if (curOP < GETELT) then {
        genFullExpr(exprToGen@.expr2);
        otherIns := insnList;
        if (curOP = ASSIGNOP) then
            rhsMode := false;
        genFullExpr(exprToGen@.expr1);
        if (curOP = ASSIGNOP) then
            rhsMode := true;
        if (insnList@.ilm = ilCONST) then {
            arg1Const := true;
            arg1Val := insnList@.payload;
        } else
            arg1Const := false;
        if (otherIns@.ilm = ilCONST) then {
            arg2Const := true;
            arg2Val := otherIns@.payload;
        } else
            arg2Const := false;
        if (curOP IN [NEOP, EQOP, LTOP, GEOP, GTOP, LEOP, INOP]) then {
            genComparison;
        } else {
            if arg1Const and arg2Const then {
writeln(' consts ', arg1Val.i oct, arg2val.i oct);
                case curOP of
                MUL:        arg1Val.r := arg1Val.r * arg2Val.r;
                RDIVOP:     arg1Val.r := arg1Val.r / arg2Val.r;
                ANDOP:      arg1Val.b := arg1Val.b and arg2Val.b;
                IDIVOP:     { arg1Val.i := ord(arg1Val.c) DIV ord(arg2Val.c);
                              arg1Val.m := arg1Val.m MOD intZero;
                            };
                IMODOP:     { arg1Val.i := ord(arg1Val.c) MOD ord(arg2Val.c);
                              arg1Val.m := arg1Val.m MOD intZero;
                            };
                PLUSOP:     arg1Val.r := arg1Val.r + arg2Val.r;
                MINUSOP:    arg1Val.r := arg1Val.r - arg2Val.r;
                OROP:       arg1Val.b := arg1Val.b or arg2Val.b:
                IMULOP:     { arg1Val.i := ord(arg1Val.c) * ord(arg2Val.c);
                              arg1Val.m := arg1Val.m MOD intZero;
                            };
                SETAND:     arg1Val.m := arg1Val.m * arg2Val.m;
                SETXOR:     arg1Val.m := arg1Val.m MOD arg2Val.m;
                INTPLUS:    arg1Val.i := arg1Val.i + arg2Val.i;
                INTMINUS:   arg1Val.i := arg1Val.i - arg2Val.i;
                SETOR:      arg1Val.m := arg1Val.m + arg2Val.m;
                SHLEFT:     arg1Val.m := shift(arg1Val.m, -arg2Val.i);
                SHRIGHT:    arg1Val.m := shift(arg1Val.m, arg2Val.i);
                NEOP, EQOP, LTOP, GEOP, GTOP, LEOP, INOP,
                ASSIGNOP:
                    error(200);
                end;
                insnList@.payload := arg1Val;
            } else {
                l3int3z := opToMode[curOP];
                nextInsn := opToInsn[curOP];
                case opFlags[curOP] of
                opfCOMM:
7760:               tryFlip(curOP in [MUL,PLUSOP,SETOR,SETAND,INTPLUS,IMULOP]);
                opfHELP:
                    genHelper;
                opfASSN: {
                    genCopy;
                    with insnList@ do {
                        typ := exprToGen@.vt.typ;
                        regsused := regsused + [0];
                        ilm := ilRVAL;
                        st := stWORD;
                    };
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
                    if arg2Const and (arg2Val.i > 0) then {
                        prepLoad;
                        if (card(arg2Val.m) = 1) then {
                            curVal.m := [minel(arg2Val.m)+1..47];
                            addToInsnList(KAAX+I8 +getFCSToffset);
                            l3int3z := 0;
                        } else {
                            addToInsnList(macro + mcPUSH);
                            genConstDiv;
                            insnList@.tail@.mode := 1;
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
                    if arg2Const and (arg2Val.i > 0) then {
                        prepLoad;
                        genConstDiv;
                        l3int3z := 1;
                    } else
                        genHelper;
                };
                opfMULMSK: {
                    if (arg1Const) then {
                        insnList@.payload.m := arg1Val.m + [0] - [1, 3];
                    } else if (arg2Const) then {
                        otherIns@.payload.m := arg2Val.m + [0] - [1, 3];
                    } else {
                        prepLoad;
                        addToInsnList(KAEX+E48);
                    };
                    tryFlip(true);
                    insnList@.tail@.mode := 1;
                    if (fixMult) then
                        addToInsnList(macro + mcMULTI)
                    else
                        addToInsnList(KYTA+64);
                };
                opfSHIFT: {
                    if (not arg2Const) then genHelper
                    else {
                        prepLoad;
                        arg2Val.m := arg2Val.m + intZero;
                        if (curOP = SHRIGHT) then {
                            addToInsnList(ASN64+arg2Val.i)
                         } else {
                            addToInsnList(ASN64-arg2Val.i)
                         }
                    }
                }
                end; (* case 10122 *)
10122:          insnList@.tail@.mode := l3int3z;
            }
        }
    } else {
        if (curOP <= FILEPTR) then {
            if (curOP = GETVAR) then {
                new(insnList);
                curIdRec := exprToGen@.id1;
                with insnList@ do {
                    tail := NIL;
                    head := NIL;
                    regsused := [];
                    ilm := ilLVAL;
                    payload.i := curIdRec@.offset;
                    disp := curIdRec@.high.i;
                    st := stWORD;
                    addrmd := 18;
                };
                if (curIdRec@.cl = FORMALID) then {
                    genDeref;
                } else if (curIdRec@.cl = ROUTINEID) then {
                    insnList@.disp := 3;
                    insnList@.payload.i :=
                        insnList@.payload.i + frameRegTemplate;
                } else if (insnList@.disp >= 74000B) then {
                    addToInsnList(insnTemp[UTC] + insnList@.disp);
                    insnList@.disp := 0;
                    insnList@.addrmd := 17;
                    insnList@.payload.i := 0;
                }
            } else
            if (curOP = GETFIELD) then {
                genFullExpr(exprToGen@.expr1);
                curIdRec := exprToGen@.id2;
                with insnList@ do {
                    disp := disp + curIdRec@.offset;
                    if (curIdRec@.pckfield) then {
                        case st of
                        stWORD:
                            shift := curIdRec@.shift;
                        stSLICE: {
                            shift := shift + curIdRec@.shift;
                            if not (S6 IN optSflags.m) then
                                shift := shift + curIdRec@.uptype.p.bits - 48;
                        };
                        stPACKED:
                            if (not rhsMode) then
                                error(errUsingVarAfterIndexingPackedArray)
                            else {
                                startLVal;
                                insnList@.shift := curIdRec@.shift;
                            }
                        end; (* 10235*)
                        insnList@.width := curIdRec@.width;
                        insnList@.st := stSLICE;
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
            if (curOP = op37) then {
                startInsnList(ilLVAL);
                genDeref;
            } else
            if (curOP = GETENUM) then
                startInsnList(ilCONST)
        } else if (curOP = STKLVAL) then {
            (* Synthetic lvalue produced by genRMWAssign.  The address
               of the real lvalue has been pushed onto the BESM-6 stack
               TWICE by the RMW prologue, once for the rvalue read and
               once for the lvalue write.  Each STKLVAL evaluation pops
               one copy via `WTC SP' into the working tag M14, and the
               standard prepLoad/prepStore appends `XTA 0' (load) or
               `ATX 0' (store) so that the memory transfer uses that
               popped address.  Pushing twice keeps stack accounting
               trivial (each visit pops exactly one slot) and sidesteps
               BESM-6's lack of true non-popping SP-relative addressing
               for `WTC'. *)
            new(insnList);
            with insnList@ do {
                tail := NIL;
                head := NIL;
                typ := exprToGen@.vt.typ;
                regsused := [];
                ilm := ilLVAL;
                st := stWORD;
                addrmd := 16;
                payload.i := 0;
                disp := 0;
                width := 0;
                shift := 0;
            };
            addToInsnList(KWTC + SP);
        } else if (curOP = ALNUM) then
            genEntry
        else if (curOP IN [TOREAL..BITNEGOP]) then {
            genFullExpr(exprToGen@.expr1);
            if (insnList@.ilm = ilCONST) then {
                arg1Val := insnList@.payload;
                case curOP of
                TOREAL: {
                    arg1Val.m := arg1Val.m + intZero;
                    arg1Val.r := arg1Val.i
                };
                NOTOP:  arg1Val.b := not arg1Val.b;
                RNEGOP: arg1Val.r := -arg1Val.r;
                INEGOP: arg1Val.i := -arg1Val.i;
                BITNEGOP: arg1Val.m := [0..47] - arg1Val.m
                end; (* case 10345 *)
                insnList@.payload := arg1Val;
            } else
            if (curOP = NOTOP) then {
                negateCond;
            } else {
                prepLoad;
                if (curOP = TOREAL) then {
                    addToInsnList(KAOX+ZERO);
                    addToInsnList(insnTemp[AVX]);
                    l3int3z := 3;
                    goto 10122;
                } else if (curOP = BITNEGOP) then {
                    addToInsnList(KAEX+ALLONES);
                    l3int3z := 1;
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
        } else if (curOP = STANDPROC) then {
            genFullExpr(exprToGen@.expr1);
            work := exprToGen@.num2;
            if (work = fnMALLOC) then
                heapCallsCnt := heapCallsCnt + 1;
            if (100 < work) then {
                prepLoad;
                addToInsnList(getHelperProc(work - 100));
            } else {
                if (insnList@.ilm = ilCONST) then {
                    arg1Const := true;
                    arg1Val := insnList@.payload;
                } else
                    arg1Const := false;
                arg2Const := insnList@.typ = realType;
                if arg1Const then {
                    case work of
                    fnABS:   arg1Val.r := abs(arg1Val.r);
                    fnTRUNC: arg1Val.i := trunc(arg1Val.r);
                    fnPTR:   arg1Val.m := arg1Val.m * [7..47];
                    fnROUND: arg1Val.i := round(arg1Val.r);
                    fnCARD:  arg1Val.i := card(arg1Val.m);
                    fnMINEL: arg1Val.i := minel(arg1Val.m);
                    fnABSI:  arg1Val.i := abs(arg1Val.i);
                    fnMALLOC: {
                        arg1Val.m := arg1Val.m + intZero;
                        addToInsnList(KVTM+I14+
                              getValueOrAllocSymtab(arg1Val.i));
                        addToInsnList(getHelperProc(33)); (*"P/NW"*)
                        with insnList@ do {
                            ilm := ilRVAL;
                            regsused := regsused + [0];
                            typ := exprToGen@.vt.typ;
                        };
                        exit
                    };
                    fnEOF,
                    fnREF,
                    fnEOLN:
                        error(201);
                    end;
                    insnList@.payload := arg1Val;
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
                        ilm := ilRVAL;
                        regsused := regsused + [0];
                    }
                } else {
                    prepLoad;
                    if (work = fnTRUNC) then {
                        l3int3z := 2;
                        addToInsnList(getHelperProc(58)); (*"P/TR"*)
                        goto 10122;
                    };
                    if (work IN [fnSQRT:fnEXP, (* was fnSUCC but not fnPRED *)
                                 fnCARD, fnPTR]) then {
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
                curVal := exprToGen@.vt;
                if (curVal.i IN liveRegs) then {
                    new(insnList);
                    with insnList@ do {
                        typ := exprToGen@.expr2@.vt.typ;
                        tail := NIL;
                        head := ;
                        regsused := [];
                        ilm := ilLVAL;
                        addrmd := 18;
                        payload.i := indexreg[curVal.i];
                        disp := 0;
                        st := stWORD;
                    }
                } else {
                    curVal.i := 14;
                    exprToGen@.vt := curVal;
                    exprToGen := exprToGen@.expr2;
                    goto 7567;
                };
                exit
            } else {
                error(220);
            }
        };
    };
    insnList@.typ := exprToGen@.vt.typ;
}; (* genFullExpr *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure formFileInit;
var extFileP: @extfilerec;
    fileBase: tptr;
    fileSym:  irptr;
    fileAddr, elemSize: integer;
{
    if (S5 IN optSflags.m) then {
        formAndAlign(KUJ+I13);
        exit
    };
    form2Insn(KITS+13, KATX+SP);
    while (curExpr <> NIL) do {
        extFileP := ptr(ord(curExpr@.vt.typ.rep));
        fileSym := curExpr@.id2;
        fileAddr := fileSym@.value;
        fileBase := fileSym@.typ.rep@.base;
        elemSize := fileSym@.typ.p.pad;
        if (fileAddr < 74000B) then {
            form1Insn(getValueOrAllocSymtab(fileAddr) +
                      insnTemp[UTC] + I7);
            fileAddr := 0;
        };
        form3Insn(KVTM+I12 + fileAddr, KVTM+I10 + fileBufSize,
                  KVTM+I9 + elemSize);
        form1Insn(KVTM+I11 + fileBase.p.psize);
        if (extFileP = NIL) then {
            form1Insn(insnTemp[XTA]);
        } else {
            curVal.i := extFileP@.location;
            if (curVal.i = 512) then
                curVal.i := extFileP@.offset;
            form1Insn(KXTA+I8 + getFCSToffset);
        };
        formAndAlign(getHelperProc(69)); (*"P/CO"*)
        curVal := fileSym@.id;
        form2Insn(KXTA+I8+getFCSToffset, KATX+I12+26);
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
%    writeln(' ':indent, expr@.op oct, ' ', expr@.op);
    indent := indent + 1;
if not (expr@.op in [NOOP,ALNUM,GETVAR,GETENUM,STANDPROC]) then {
       dump(expr@.expr1, indent);
       if not (expr@.op in
[INEGOP..BITNEGOP,TOREAL,DEREF,FILEPTR,NOTOP,GETFIELD,SHLEFT,SHRIGHT]) then
           dump(expr@.expr2, indent);
    }
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* formOperator *)
%   writeln(' formoperator ', l3arg1z);
    rhsMode := true;
    if (errors and (l3arg1z <> SETREG)) or (curExpr = NIL) then
        exit;
    if not (l3arg1z IN [FORMOP, STOREAT9, DFLTWDTH, FILEINIT, PCKUNPCK]) then {
        dump(curExpr, 1);
        genFullExpr(curExpr);
    };
    case l3arg1z of
    DOIT: genOneOp;
    SETREG: {
        with insnList@ do {
            l3int3z := insnCount;
            new(helpExpr);
            helpExpr@.expr1 := withList;
            withList := helpExpr;
            helpExpr@.op := NOOP;
            case st of
            stWORD: {
                if (l3int3z = 0) then {
                    l3int2z := 14;
                } else {
                    l3var10z.m := auxRegs * freeRegs;
                    if (l3var10z.m <> []) then {
                        l3int2z := minel(l3var10z.m);
                    } else {
                        l3int2z := 14;
                    };
                    if (l3int3z <> 1) then {
                        setAddrTo(l3int2z);
                        addToInsnList(KITA + l3int2z);
                        spillAcc(op37);
                    } else if (l3int2z <> 14) then {
                        setAddrTo(l3int2z);
                        genOneOp;
                    };
                    l3var11z.m := [l3int2z] - [14];
                    usedRegs := usedRegs - l3var11z.m;
                    freeRegs := freeRegs - l3var11z.m;
                    liveRegs := liveRegs + l3var11z.m;
                };
                curVal.i := l3int2z;
                helpExpr@.vt := curVal;
            };
            stSLICE: {
                curVal.i := 14;
                helpExpr@.vt := curVal;
            };
            stPACKED:
                error(errVarTooComplex);
            end; (* case *)
        }; (* with *)
        helpExpr@.expr2 := curExpr;
    }; (* SETREG *)
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
        if (insnList@.st <> stWORD) then
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
    SETREG12: {
        setAddrTo(12);
        genOneOp
    };
    DFLTWDTH: {
        curVal.m := curVal.m + intZero;
        form1Insn(KXTA+I8 + getFCSToffset);
    };
    FRACWIDTH: {
        prepLoad;
        prependToInsnList(macro + mcPUSH);
        genOneOp;
    };
    gen11, gen12: {
        setAddrTo(11);
        if (l3arg1z = gen12) then
            prependToInsnList(macro + mcPUSH);
        genOneOp;
        usedRegs := usedRegs + [12];
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
                if (payload.b) then {
                    jumpTarget := 0;
                } else {
                    if (noTarget) then {
                        formJump(jumpTarget);
                    } else {
                        form1Insn(insnTemp[UJ] + jumpTarget);
                    }
                }
            } else {
                if (curExpr@.vt.typ <> booleanType) and not (curExpr@.op IN
                    [SHLEFT..SETOR,GETELT..ALNUM]) then {
                    addToInsnList(KAEX);
                };
                direction := 16 in insnList@.regsused;
                if (insnList@.ilm = ilCOND) and
                   (insnList@.payload.i <> 0) then {
                    genOneOp;
                    if (direction) then {
                        if (noTarget) then
                            formJump(l3int3z)
                        else
                            form1Insn(insnTemp[UJ] + l3int3z);
                        fixup(0, jumpTarget);
                        jumpTarget := l3int3z;
                    } else {
                        if (not noTarget) then {
                            if (not putLeft) then
                                padToLeft;
                            fixup(l3int3z, jumpTarget);
                        }
                    };
                } else {
                    if (insnList@.ilm = ilLVAL) then {
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
        helpExpr := curExpr;
        curExpr := curExpr@.expr1;
        formOperator(gen11);
        genFullExpr(helpExpr@.expr2);
        if (11 IN insnList@.regsused) then
            error(44); (* errIncorrectUsageOfStandProcOrFunc *)
        setAddrTo(12);
        genOneOp;
        arg1Type := helpExpr@.expr2@.vt.typ;
        with arg1Type.rep@ do
            l3int3z := aright - aleft + 1;
        form2Insn((KVTM+I14) + l3int3z,
                  (KVTM+I10+64) - arg1Type.rep@.pcksize);
        l3int3z := ord(helpExpr@.vt.typ.rep);
        l3int1z := arg1Type.rep@.perword;
        if (l3int3z = 72) then          (* P/KC *)
            l3int1z := 1 - l3int1z;
        form1Insn(getValueOrAllocSymtab(l3int1z) + (KVTM+I9));
        l3int1z := insnTemp[XTA];
        form1Insn(l3int1z);
        formAndAlign(getHelperProc(l3int3z));
   };
   LITINSN: {
        with insnList@ do {
            if (ilm <> ilCONST) then
                error(errNoConstant);
            if (insnList@.typ.p.psize <> 1) then
                error(errConstOfOtherTypeNeeded);
            curVal := insnList@.payload;
        }
    };
    end; (* case *)
}; (* formOperator *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseTypeRef(var newtype: tptr; skipTarget: setofsys);
label
    12247, 12366, 13020;
type
    pair = record
            first, second: integer
        end;
    pair7 = array [1..7] of pair;
    caserec = record
            size, count: integer;
            pairs: pair7;
        end;
    rangeRec = record
            aleft, aright: integer
        end;
    rangeList = array [1..20] of rangeRec;
var
    isPacked: boolean;
    cond: boolean;
    cases: caserec;
    numBits, l3int22z, span, rangeCnt, curDim: integer;
    curEnum, curField: irptr;
    arrayType, nestedType, tempType, curType: tptr;
    ranges: rangeList;
    curRange: rangeRec;
    l3idr31z: irptr;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure definePtrType(toType: tptr);
{
    new(curType.rep = 2);
    with curType.rep@ do {
        base := toType;
    };
    curType.p.psize := 1;
    curType.p.bits := 15;
    curType.p.pk := kindPtr;
    new(curEnum = 5);
    curEnum@ := [curIdent, lineCnt, typelist, curType, TYPEID];
    typelist := curEnum;
}; (* definePtrType *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function getPtrType(baseType: tptr): tptr;
{
    new(curType.rep = 2);
    with curType.rep@ do {
        base := baseType;
    };
    curType.p.psize := 1;
    curType.p.bits := 15;
    curType.p.pk := kindPtr;
    getPtrType := curType;
}; (* getPtrType *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseRecordDecl(var rectype: tptr; isOuterDecl: boolean);
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
    curEnum@ := [curIdent, , fieldHash[bucket], ,
                    FIELDID, NIL, curType, isPacked];
    fieldHash[bucket] := curEnum;
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
    parseTypeRef(selType, skipTarget + [UNIONSY]);
    if (curType.rep@.fields = NIL) then {
        curType.rep@.fields := curField;
    } else {
        l3idr31z@.list := curField;
    };
    cond := isFileType(selType);
    curType.rep@.flag := cond or curType.rep@.flag;
    l3idr31z := curEnum;
    repeat
        curField@.typ := selType;
        if (isPacked) then {
            l5var1z := selType.p.bits;
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
        cases.size := cases.size + selType.p.psize;
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
                write('.WORDS=', selType.p.psize:0);
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
    lookupMode := lookField;
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
                lookupMode := lookField;
                inSymbol;
            };
            cond := (SY <> COMMA);
            if (not cond) then {
                lookupMode := lookField;
                inSymbol;
            }
        until cond;
        checkSymAndRead(COLON);
        packFields;
        if (SY = SEMICOLON) then {
            lookupMode := lookField;
            inSymbol;
        }
    };
    if (SY = UNIONSY) then {
        lookupMode := lookField;
        inSymbol;
        if (SY <> BEGINSY) then
            requiredSymErr(BEGINSY);
        lookupMode := lookField;
        inSymbol;
        cases1 := cases;
        cases2 := cases;
        l4typ1z.rep := NIL;
        l4var7z.i := 0;
        while (SY = STRUCTSY) do {
            lookupMode := lookField;
            inSymbol;
            new(l4var5z.rep = 5);
            with l4var5z.rep@ do {
                first := l4var7z.typ;
                next.rep := NIL;
                alt.rep := NIL;
            };
            l4var5z.p.psize := cases.size;
            l4var5z.p.bits := 48;
            l4var5z.p.pk := kindCases;
            if (l4typ1z.rep = NIL) then {
                if (curType.rep@.variants.rep = NIL) then {
                    curType.rep@.variants := l4var5z;
                } else {
                    rectype.rep@.first := l4var5z;
                }
            } else {
                l4typ1z.rep@.next := l4var5z;
            };
            l4typ1z := l4var5z;
            tempType := l4var5z;
            parseRecordDecl(tempType, false);
            if (cases2.size < cases.size) or
               isPacked and (cases.size = 1) and (cases2.size = 1) and
                (cases.count = 1) and (cases2.count = 1) and
                (cases.pairs[1].first < cases2.pairs[1].first) then {
                cases2 := cases;
            };
            cases := cases1;
            l4var7z.i := l4var7z.i + 1;
            if (SY = SEMICOLON) then {
                lookupMode := lookField;
                inSymbol;
            };
        };
        cases := cases2;
        if (SY <> ENDSY) then
            requiredSymErr(ENDSY);
        lookupMode := lookField;
        inSymbol;
        if (SY = SEMICOLON) then {
            lookupMode := lookField;
            inSymbol;
        };
    };
    rectype.p.psize := cases.size;
    if isPacked and (cases.size = 1) and (cases.count = 1) then {
        rectype.p.bits := 48 - cases.pairs[1].first;
    } else {
        rectype.p.bits := 48;
    };
    if rectype.p.pk = kindStruct then {
        l4var6z := rectype.rep@.fields;
        while l4var6z <> NIL do {
            l4var6z@.uptype := rectype;
            l4var6z := l4var6z@.list;
        };
    };
    checkSymAndRead(ENDSY);
}; (* parseRecordDecl*)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseRange(var aleft, aright: integer);
var
    leftBound, rightBound: word;
    tempType: tptr;
{
    parseLiteral(tempType, curVal, true);
    curVal.m := curVal.m + intZero;
    if (tempType.rep <> NIL) and (tempType.p.pk = kindScalar) then {
        inSymbol;
        if (SY <> COLON) then {
% Handle a single value N as a range 0..N-1
            aright := curVal.i - 1;
            aleft := 0;
            exit;
        };
        aleft := curVal.i;
        inSymbol;
        parseLiteral(tempType, curVal, true);
        curVal.m := curVal.m + intZero;
        inSymbol;
        if (tempType.rep <> NIL) and (tempType.p.pk = kindScalar) then {
            aright := curVal.i;
            exit;
        }
    };
    error(64); (* errIncorrectRangeDefinition *)
    aleft := 0;
    aright := 0;
}; (* parseRange *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function makeArrayType(rg: rangeRec; elem: tptr; pckFlag: boolean): tptr;
var
    makePacked: boolean;
    sizeVal, bitsVal, perwordVal, pcksizeVal: integer;
{
    makePacked := pckFlag;
    span := rg.aright - rg.aleft + 1;
    l3int22z := elem.p.bits;
    if (24 < l3int22z) then
        makePacked := false;
    bitsVal := 48;
    perwordVal := 0;
    pcksizeVal := 0;
    if (makePacked) then {
        l3int22z := 48 DIV l3int22z;
        if (l3int22z = 9) then {
            l3int22z := 8;
        } else if (l3int22z = 5) then {
            l3int22z := 4
        };
        perwordVal := l3int22z;
        pcksizeVal := 48 DIV l3int22z;
        l3int22z := span * pcksizeVal;
        if l3int22z mod 48 = 0 then
            numBits := 0
        else
            numBits := 1;
        sizeVal := l3int22z div 48 + numBits;
        if (sizeVal = 1) then
            bitsVal := l3int22z;
    } else {
        sizeVal := span * elem.p.psize;
        curVal.i := elem.p.psize;
        curVal.m := curVal.m * [7:47] + [0];
        perwordVal := KMUL+ I8 + getFCSToffset;
    };
    new(arrayType.rep, kindArray);
    with arrayType.rep@ do {
        aleft := rg.aleft;
        aright := rg.aright;
        base := elem;
        pck := makePacked;
        perword := perwordVal;
        pcksize := pcksizeVal;
    };
    arrayType.p.psize := sizeVal;
    arrayType.p.bits := bitsVal;
    arrayType.p.pk := kindArray;
    makeArrayType := arrayType;
}; (* makeArrayType *)
% ifdef CTYPES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseDcl(baseType: tptr; var dclName: word; var dclType: tptr);
type
    dclIntList = array [1..20] of integer;
    dclRngType = array [1..20] of rangeRec;
var
    dclCnt, dclIdx: integer;
    dclKind: dclIntList;
    dclRngs: dclRngType;
    rangeInfo: rangeRec;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure addDclPtr;
{
    if (dclCnt = 20) then
        error(errVarTooComplex)
    else {
        dclCnt := dclCnt + 1;
        dclKind[dclCnt] := 1;
    };
}; (* addDclPtr *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure addDclArray(rangeInfo: rangeRec);
{
    if (dclCnt = 20) then
        error(errVarTooComplex)
    else {
        dclCnt := dclCnt + 1;
        dclKind[dclCnt] := 2;
        dclRngs[dclCnt] := rangeInfo;
    };
}; (* addDclArray *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure readDclCore;
{
    if (charClass = MUL) then {
        inSymbol;
        readDclCore;
        addDclPtr;
    } else if (SY = LPAREN) then {
        inSymbol;
        readDclCore;
        checkSymAndRead(RPAREN);
    } else if (SY = IDENT) then {
        dclName := curIdent;
        inSymbol;
    } else {
        error(errNoIdent);
        dclName.i := 0;
    };
    while (SY = LBRACK) do {
        inSymbol;
        parseRange(rangeInfo.aleft, rangeInfo.aright);
        checkSymAndRead(RBRACK);
        addDclArray(rangeInfo);
    };
}; (* readDclCore *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{
    dclCnt := 0;
    dclName.i := 0;
    readDclCore;
    dclType := baseType;
    for dclIdx := dclCnt downto 1 do {
        if (dclKind[dclIdx] = 1) then
            dclType := getPtrType(dclType)
        else
            dclType := makeArrayType(dclRngs[dclIdx], dclType, false);
    };
}; (* parseDcl *)
% endif CTYPES
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* parseTypeRef *)
    isPacked := false;
12247:
    if (SY = ENUMSY) then {
        inSymbol;
        checkSymAndRead(BEGINSY);
        span := 0;
        lookupMode := lookDef;
        curField := NIL;
        new(curType.rep = 4);
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
                curType.rep@.enums := curEnum;
            } else {
                curField@.list := curEnum;
            };
            curField := curEnum;
            inSymbol;
            if (SY = COMMA) then {
                lookupMode := lookDef;
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
            with curType.rep@ do {
                numen := span;
                start := 0;
            };
            curType.p.psize := 1;
            curType.p.bits := nrOfBits(span - 1);
            curType.p.pk := kindScalar;
            curEnum := curType.rep@.enums;
            while curEnum <> NIL do {
                curEnum@.typ := curType;
                curEnum := curEnum@.list;
            };
        };
    } else
    if (charClass = MUL) then {
        inSymbol;
        if not (SY IN [IDENT,TYPESY]) then {
            error(errNoIdent);
            curType := voidPtr;
        } else {
            if (SY = TYPESY) then {
                curType := getPtrType(hashTravPtr@.typ);
            } else if (hashTravPtr = NIL) then {
                if (inTypeDef) then {
                    if (knownInType(curEnum)) then {
                        curType := curEnum@.typ;
                    } else {
                        definePtrType(integerType);
                    };
                } else {
12366:              error(errNotAType);
                    curType := voidPtr;
                };
            } else
                goto 12366;
            inSymbol;
        }
    } else if (SY IN [IDENT,TYPESY]) then {
        if (SY = TYPESY) then {
            curType := hashTravPtr@.typ;
        } else
            goto 12366;
        inSymbol;
        if (curType = integerType) and (SY = COLON) then {
            inSymbol;
            if (SY <> INTCONST) then
                error(errNumberTooLarge)
            else {
                curToken.m := curToken.m + intZero;
                l3int22z := curToken.i;
                inSymbol;
                curType := mkIntScl(l3int22z);
            }
        };
    } else {
        if (SY = PACKEDSY) then {
            isPacked := true;
            inSymbol;
            goto 12247;
        };
        if (SY = STRUCTSY) then {
            new(curType.rep = 5);
            typ121z := curType;
            with curType.rep@ do {
                variants.rep := NIL;
                fields := NIL;
                flag := false;
                pckrec := isPacked;
            };
            curType.p.psize := 0;
            curType.p.bits := 48;
            curType.p.pk := kindStruct;
            cases.size := 0;
            cases.count := 0;
            inSymbol;
            parseRecordDecl(curType, true);
        } else if (SY = FILESY) then {
            inSymbol;
            checkSymAndRead(OFSY);
            parseTypeRef(nestedType, skipTarget);
            if (isPacked) then {
                l3int22z := nestedType.p.bits;
                if (24 < l3int22z) then
                    isPacked := false;
            };
            new(curType.rep, kindFile);
            if (not isPacked) then
                l3int22z := 0;
            with curType.rep@ do {
                base := nestedType;
            };
            curType.p.pad := l3int22z;
            curType.p.psize := 30;
            curType.p.bits := 48;
            curType.p.pk := kindFile
        } else {
            error(errNotAType);
        };
    };
    tempType := curType;
    rangeCnt := 0;
    while (SY = LBRACK) do {
        inSymbol;
        parseRange(curRange.aleft, curRange.aright);
        if (rangeCnt = 20) then {
            error(errVarTooComplex);
        } else {
            rangeCnt := rangeCnt + 1;
            ranges[rangeCnt] := curRange;
        };
        checkSymAndRead(RBRACK);
    };
    curType := tempType;
    for curDim := rangeCnt downto 1 do {
        curType := makeArrayType(ranges[curDim], curType,
                                 isPacked and (curDim = 1));
    };
    if (rangeCnt <> 0) then
        isPacked := false;
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
    if (l3arg1z.rep@.start = 0) then {
        l3arg1z.rep@.start := FcstCnt;
        l3var1z := l3arg1z.rep@.enums;
        while (l3var1z <> NIL) do {
            curVal := l3var1z@.id;
            l3var1z := l3var1z@.list;
            toFCST;
        }
    }
}; (* dumpEnumNames *)
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
        lookupMode := lookDef;
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
        padToLeft;
        l3var3z := 22 IN procName@.flags;
        l3arg1z := procName@.pos;
        frame.i := moduleOffset - 40000B;
        if (l3arg1z <> 0) then
            symTab[l3arg1z] := [24, 29] + frame.m * halfWord;
        procName@.pos := moduleOffset;
        l3arg1z := argCount(procName);
        if l3var3z then {
            if (41 >= entryPtCnt) then {
                curVal := procName@.id;
                entryPtTable[entryPtCnt] := leftAlign;
                entryPtTable[entryPtCnt+1] := [1] + frame.m - [0, 3];
                entryPtCnt := entryPtCnt + 2;
            } else
                error(87); (* errTooManyEntryProcs *)
        };
        if (procName@.typ = voidType) then {
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
        if (hasFiles <> 0) then {
            form1Insn(insnTemp[XTA]);
            formAndAlign(KVJM+I13 + hasFiles);
            curVal.i := hasFiles;
            fixup(2, 49 (* "P/RDC" *));
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
    }
    end; (* case *)
}; (* parseDecls *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure labCheckAndDefine(isDef: boolean);
var
    labIdx: integer;
{
    labIdx := numLabTop;
    while (labIdx > labFence) and (numLabs[labIdx].id <> curToken) do
        labIdx := labIdx - 1;
    if (labIdx = labFence) then {
            if (numLabTop >= 20) then {
                error(50); (* errSymbolTableOverflow *)
                exit;
            };
            numLabTop := numLabTop + 1;
            with numLabs[numLabTop] do {
                id := curToken;
                offset := 0;
                line := lineCnt;
                defined := false;
            };
            labIdx := numLabTop;
    };
    with numLabs[labIdx] do {
        if (isDef) then {
            if defined then {
                errLine := numLabs[labIdx].line;
                error(17); (* errLblAlreadyDefinedInLine *)
                exit;
            };
            line := lineCnt;
            defined := true;
            if offset = 0 then {
                (* empty *)
            } else if (offset >= 74000B) then {
                curVal.i := moduleOffset - 40000B;
                symTab[offset] := [24,29] + curVal.m * O77777;
            } else {
                fixup(0, offset);
            };
            offset := moduleOffset;
        } else {
                if (offset >= 40000B) then {
                    form1Insn(insnTemp[UJ] + offset);
                } else {
                    formJump(offset);
                }
        }
    }
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure statement;
label
    8888;
var
    boundary              : eptr;
    strLabPtr             : @strLabel;
    nest                  : boolean;
    flag                  : boolean;
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
    with arg.rep@ do
        isCharArray := (arg.p.pk = kindArray) and (base = charType);
}; (* isCharArray *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure expression;
    forward;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
(* parsePostfix: consume any chain of postfix operators (@, .field, [idx])
   acting on curExpr.  Returns with SY pointing at the first token that is
   neither a postfix operator nor the trailing `]` of an index list.  Safe
   to call when the next token isn't a postfix at all (loop simply exits). *)
procedure parsePostfix;
label
    13462, 55;
var
    l4exp1z, l4exp2z: eptr;
    l4typ3z: tptr;
    l4var4z: kind;
{
13462:
    l4typ3z := curExpr@.vt.typ;
    l4var4z := l4typ3z.p.pk;
    if (SY = ARROW) then {
        (* '->' is deref + struct field selection; build DEREF here,
           then jump to label 55 to consume the field IDENT. *)
        new(l4exp1z);
        l4exp1z@.expr1  :=  curExpr;
        if ((l4var4z = kindPtr) and
            (l4typ3z.rep@.base.p.pk = kindStruct)) then {
            l4exp1z@.vt.typ  :=  l4typ3z.rep@.base;
            l4exp1z@.op   :=  DEREF;
            curExpr  :=  l4exp1z;
            l4typ3z  :=  l4typ3z.rep@.base;
            goto 55;
        (* leaving it here for the future when accessing fields of the FILE
           structure is possible, that is, when kindFile is a special case
           of kindStruct
        } else if (l4var4z = kindFile) then {
                                            typ  :=  l4typ3z@.base;
                                            op  :=  FILEPTR;
        *)
        } else {
            stmtName  :=  '  ->  ';
            error(errWrongVarTypeBefore);
            l4exp1z@.vt.typ  :=  l4typ3z;
        };
        curExpr  :=  l4exp1z;
        inSymbol;
    } else if (SY = PERIOD) then {
        if (l4var4z = kindStruct) then {
55:         lookupMode := lookField;
            typ121z := l4typ3z;
            inSymbol;
            if (hashTravPtr = NIL) then {
                error(20); (* errDigitGreaterThan7 ??? *)
            } else {
                new(l4exp1z);
                with l4exp1z@ do {
                    vt.typ := hashTravPtr@.typ;
                    op := GETFIELD;
                    expr1 := curExpr;
                    id2 := hashTravPtr;
                };
                curExpr := l4exp1z;
            };
            inSymbol;
        } else {
            stmtName := '  .   ';
            error(errWrongVarTypeBefore);
            exit;
        };
    } else if (SY = LBRACK) then {
        stmtName := '  [   ';
        l4exp1z := curExpr;
        expression;
        l4typ3z := l4exp1z@.vt.typ;
        if (l4typ3z.p.pk <> kindArray) then {
            error(errWrongVarTypeBefore);
        } else {
            new(l4exp2z);
            with l4exp2z@ do {
                vt.typ := l4typ3z.rep@.base;
                expr1 := l4exp1z;
                expr2 := curExpr;
                op := GETELT;
            };
            l4exp1z := l4exp2z;
        };
        curExpr := l4exp1z;
        if (SY <> RBRACK) then
            error(67 (*errNeedBracketAfterIndices*));
        inSymbol;
    } else exit;
    goto 13462;
}; (* parsePostfix *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseLval;
var
    l4exp1z: eptr;
{
    if (hashTravPtr@.cl = FIELDID) then {
        (* Implicit field of the `with` variable: build GETFIELD on
           withIter directly, then continue with any further postfix. *)
        new(l4exp1z);
        with l4exp1z@ do {
            vt.typ := hashTravPtr@.typ;
            op := GETFIELD;
            expr1 := withIter;
            id2 := hashTravPtr;
        };
        curExpr := l4exp1z;
    } else {
        new(curExpr);
        with curExpr@ do {
            vt.typ := hashTravPtr@.typ;
            op := GETVAR;
            id1 := hashTravPtr;
        };
    };
    inSymbol;
    parsePostfix;
}; (* parseLval *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure castToReal(var value: eptr);
var
    cast: eptr;
{
    new(cast);
    with cast@ do {
        vt.typ := realType;
        op := TOREAL;
        expr1 := value;
        value := cast;
    }
}; (* castToReal *)
%
function areTypesCompatible(var other: eptr): boolean;
{
    if (arg1Type = realType) then {
        if typeCheck(integerType, arg2Type) then {
            castToReal(other);
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
        if typ <> voidType then
            liveRegs := liveRegs - flags;
        noArgs := (list = NIL) and not (24 in flags);
    };
    new(callExpr);
    argList := callExpr;
    bool48z := true;
    with callExpr@ do {
        vt.typ := subroutine@.typ;
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
            inCallArgs := true;
            expression;
            actualOp := curExpr@.op;
(a)         if noArgs then {
                formClass := curFormal@.cl;
                if (actualOp = PCALL) then {
                    if (formClass <> ROUTINEID) or
                       (curFormal@.typ <> voidType) then {
13736:                  error(39); (*errIncompatibleArgumentKinds*)
                        exit a
                    }
                } else {
                    if (actualOp = FCALL) then {
                        if (formClass = ROUTINEID) then {
                            if (curFormal@.typ = voidType) then
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
                arg1Type := curExpr@.vt.typ;
                if (arg1Type <> voidType) then {
                    if not typeCheck(arg1Type, curFormal@.typ) then
                        error(40); (*errIncompatibleArgumentTypes*)
                }
            };
            new(curActual);
            with curActual@ do {
                vt.typ.rep := NIL;
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
        error(42); (*errNoArgList*)
    };
    curExpr := callExpr;
}; (* parseCallArgs *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function getPrec(sym: symbol; cls: operator): integer;
{
    if sym = EXPROP then
        getPrec := opPrec[cls]
    else if sym = BECOMES then
        getPrec := precAssign
    else
        getPrec := precNone
}; (* getPrec *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure bldBitOp(oper: operator; leftArg: eptr);
var finalExpr: eptr;
{
    if (arg1Type.p.pk <> kindScalar)
    or (arg2Type.p.pk <> kindScalar) then {
        error(errNeedOtherTypesOfOperands);
        exit;
    };
    new(finalExpr);
    with finalExpr@ do {
        op := oper;
        expr1 := leftArg;
        expr2 := curExpr;
        vt.typ := arg1Type;
    };
    curExpr := finalExpr;
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure bldArithOp(oper: operator; leftExpr: eptr; match: boolean);
var
    finalExpr: eptr;
    k1, k2: kind;
{
    k1 := arg1Type.p.pk;
    k2 := arg2Type.p.pk;
    if (k1 > kindScalar) or (k2 > kindScalar) then {
        error(errNeedOtherTypesOfOperands);
        exit;
    };
    new(finalExpr);
    with finalExpr@ do {
        if (k1 = kindReal) or (k2 = kindReal) then {
            if (oper = IMODOP) then {
                error(62); (* errIntegerNeeded *)
                exit;
            };
            if (k1 <> kindReal) then
                castToReal(curExpr);
            if (k2 <> kindReal) then
                castToReal(leftExpr);
            op := oper;
            vt.typ := realType;
        } else {
            op := intOpMap[oper];
            vt.typ := integerType;
        };
        expr1 := leftExpr;
        expr2 := curExpr;
   };
   curExpr := finalExpr;
}; (* bldArithOp *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure bldRelOp(oper: operator; ex2: eptr);
var ex1: eptr;
{
    if typeCheck(arg1Type, arg2Type) then {
        if
           (arg1Type.p.pk = kindFile) or
           (arg1Type.p.psize <> 1) and
           (oper >= LTOP) and
           not isCharArray(arg1Type) then
            error(errNeedOtherTypesOfOperands);
    } else  {
        if not areTypesCompatible(ex2) and
           ((arg1Type <> integerType) or
            (arg2Type.p.pk <> kindScalar) or
           (oper <> INOP)) then {
            error(errNeedOtherTypesOfOperands);
        }
    };
    new(ex1);
    with ex1@ do {
        vt.typ := booleanType;
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
}; (* bldRelOp *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure bldLogOp(oper: operator; leftExpr: eptr; match: boolean);
var finalExpr: eptr;
{
    if (not match) or
       ((arg1Type <> booleanType) and (arg1Type <> integerType)) then {
        error(errNeedOtherTypesOfOperands);
    } else {
        new(finalExpr);
        with finalExpr@ do {
            vt.typ := booleanType;
            op := oper;
            expr1 := leftExpr;
            expr2 := curExpr;
            curExpr := finalExpr;
        }
    }
}; (* bldLogOp *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure bldCondOp(condExpr, thenExpr: eptr);
var altExpr, condOpExpr: eptr;
    resType: tptr;
{
    if (condExpr@.vt.typ.p.pk > kindPtr) then {
        error(errBooleanNeeded);
        exit;
    };
    arg1Type := thenExpr@.vt.typ;
    arg2Type := curExpr@.vt.typ;
    if not typeCheck(arg1Type, arg2Type) then {
        error(errNeedOtherTypesOfOperands);
        exit;
    };
    resType := arg1Type;
    if (resType.p.psize <> 1) then {
        error(errNeedOtherTypesOfOperands);
        exit;
    };
    new(altExpr);
    with altExpr@ do {
        vt.typ := resType;
        op := ALTERN;
        expr1 := thenExpr;
        expr2 := curExpr;
    };
    new(condOpExpr);
    with condOpExpr@ do {
        vt.typ := resType;
        op := CONDOP;
        expr1 := condExpr;
        expr2 := altExpr;
    };
    curExpr := condOpExpr;
}; (* bldCondOp *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure factor;
label
    14567;
var
    l4var1z: word;
    wasInCall: boolean;
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
    stProcNo, checkMode, resultValue: integer;
{
    curVal.i := routine@.low;
    stProcNo := curVal.i;
    if (SY <> LPAREN) then {
        requiredSymErr(LPAREN);
        goto 8888;
    };
    if (stProcNo IN [fnSIZEOF, fnOFFSETOF]) then {
        lookupMode := lookUse;
        inSymbol;
        if (SY = TYPESY) then {
            l5var2z := hashTravPtr@.typ;
            inSymbol;
        } else {
            if (stProcNo = fnSIZEOF) then {
                readNext := false;
                expression;
                l5var2z := curExpr@.vt.typ;
            } else {
                error(errNotAType);
                l5var2z := integerType;
                if (SY = IDENT) then
                    inSymbol;
            }
        };
        if (stProcNo = fnOFFSETOF) then {
            if (l5var2z.p.pk <> kindStruct) then
                error(errWrongVarTypeBefore);
            if (SY <> COMMA) then
                requiredSymErr(COMMA)
            else {
                typ121z := l5var2z;
                lookupMode := lookField;
                inSymbol;
            };
            if (SY <> IDENT) then {
                error(errNoIdent);
                resultValue := 0;
            } else {
                if (hashTravPtr = NIL) then {
                    error(errNotDefined);
                    resultValue := 0;
                } else {
                    resultValue := hashTravPtr@.offset;
                };
                inSymbol;
            }
        } else {
            resultValue := l5var2z.p.psize;
        };
        new(newExpr);
        with newExpr@ do {
            op := GETENUM;
            lit.c := chr(resultValue);
            vt.typ := integerType;
        };
        curExpr := newExpr;
        checkSymAndRead(RPAREN);
        exit;
    };
    expression;
    if (stProcNo >= fnEOF) and (fnEOLN >= stProcNo) and
       not (curExpr@.op IN [GETELT..FILEPTR]) then {
        error(27); (* errExpressionWhereVariableExpected *)
        exit;
    };
    arg1Type := curExpr@.vt.typ;
    argKind := arg1Type.p.pk;
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
    else if (arg1Type.p.psize = 30) then
        checkMode := chkFILE
    else {
        checkMode := chkOTHER;
    };
    asBitset := [stProcNo];
    if (stProcNo <> fnSIZEOF) and not ((checkMode = chkREAL) and
            (asBitset <= [fnSQRT:fnTRUNC, fnREF, fnROUND])
           or ((checkMode = chkINT) and
            (asBitset <= [fnSQRT:fnABS,fnMALLOC,fnREF,fnCARD,fnMINEL,fnPTR]))
           or ((checkMode IN [chkCHAR, chkSCALAR, chkPTR]) and
            (asBitset <= [fnREF]))
           or ((checkMode = chkFILE) and
            (asBitset <= [fnEOF, fnREF, fnEOLN]))
           or ((checkMode = chkOTHER) and
            (stProcNo = fnREF))) then
        error(errNeedOtherTypesOfOperands);
    if not (asBitset <= [fnABS, fnSIZEOF]) then {
        arg1Type := routine@.typ;
    } else if (checkMode = chkINT) and (asBitset <= [fnABS]) then {
        stProcNo := fnABSI
    };
    new(newExpr);
    with newExpr@ do
        if (stProcNo = fnSIZEOF) then {
            op := GETENUM;
            lit.c := chr(arg1Type.p.psize);
            vt.typ := integerType;
        } else {
            op := STANDPROC;
            expr1 := curExpr;
            num2 := stProcNo;
            vt.typ := arg1Type;
        };
    curExpr := newExpr;
    checkSymAndRead(RPAREN);

}; (* stdCall *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* factor *)
    wasInCall := inCallArgs;
    inCallArgs := false;
    if (SY = TYPESY) then {
        l4typ11z := hashTravPtr@.typ;
        inSymbol;
        if SY <> LPAREN then error(88 + ord(LPAREN));
        expression;
        if curExpr@.vt.typ.p.psize <> l4typ11z.p.psize then
            error(errNeedOtherTypesOfOperands);
        checkSymAndRead(RPAREN);
        curExpr@.vt.typ := l4typ11z;
    } else if (SY IN
        [IDENT,INTCONST,REALCONST,CHARCONST,STRINGSY,LPAREN,LBRACK]) then {
        case SY of
        IDENT: {
            if (hashTravPtr = NIL) then {
                error(errNotDefined);
                curExpr := uVarPtr;
            } else
                case hashTravPtr@.cl of
                ENUMID: {
                    new(curExpr);
                    with curExpr@ do {
                        vt.typ := hashTravPtr@.typ;
                        op := GETENUM;
                        num1 := hashTravPtr@.value;
                        num2 := 0;
                    };
                    inSymbol;
                };
                ROUTINEID:
(rout)          {
                    routine := hashTravPtr;
                    inSymbol;
                    if (routine@.offset = 0) then {
                        if (routine@.typ <> voidType) and
                           (SY = LPAREN) then {
                            stdCall;
                            exit rout;
                        };
                        error(44) (* errIncorrectUsageOfStandProcOrFunc *)
                    } else if (routine@.typ = voidType) then {
                        if (wasInCall) then {
                            newOp := PCALL;
                        } else {
                            error(68); (* errUsingProcedureInExpression *)
                        }
                   } else  {
                        if (SY = LPAREN) then {
                            parseCallArgs(routine);
                            exit rout
                        };
                        if (wasInCall) then {
                            newOp := FCALL;
                        } else {
                            parseCallArgs(routine);
                            exit rout
                        };
                    };
                    new(curExpr);
                    if not (SY IN [RPAREN, COMMA]) then {
                        error(errNoCommaOrParenOrTooFewArgs);
                        goto 8888;
                    };
                    with curExpr@ do {
                        vt.typ := routine@.typ;
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
        INTCONST, REALCONST, CHARCONST, STRINGSY: {
            new(curExpr);
            parseLiteral(curExpr@.vt.typ, curExpr@.lit, false);
            curExpr@.num2 := ord(numFormat);
            curExpr@.op := GETENUM;
            inSymbol;
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
                        l4typ11z := curExpr@.vt.typ;
                        if (l4typ11z.p.pk <> kindScalar) then
                            error(23); (* errTypeIdInsteadOfVar *)
                    } else {
                        if not typeCheck(l4typ11z, curExpr@.vt.typ) then
                            error(24); (*errIncompatibleExprsInSetCtor*)
                    };
                    l4var12z := false;
                    l4exp5z := curExpr;
                    if (SY = COLON) then {
                        expression;
                        if not typeCheck(l4typ11z, curExpr@.vt.typ) then
                            error(24); (*errIncompatibleExprsInSetCtor*)
                        if (l4exp5z@.op <> GETENUM) or
                           (curExpr@.op <> GETENUM) then
                            error(errNoConstant)
                        else {
                            l4var4z.i := l4exp5z@.num1;
                            l4var3z.i := curExpr@.num1;
                            l4var4z.m := l4var4z.m - intZero;
                            l4var3z.m := l4var3z.m - intZero;
                            l4var1z.m := l4var1z.m + [l4var4z.i..l4var3z.i];
                            curExpr := newExpr;
                        };
                        goto 14567;
                   } else {
                        if (l4exp5z@.op = GETENUM) then {
                            l4var4z.i := l4exp5z@.num1;
                            l4var4z.m := l4var4z.m - intZero;
                            l4var1z.m := l4var1z.m + [l4var4z.i];
                            curExpr := newExpr;
                            goto 14567;
                        };
                        error(errNoConstant);
                    };
                    new(curExpr);
                    with curExpr@ do {
                        vt.typ := integerType;
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
                vt.typ := integerType;
                lit := l4var1z;
            }
        };
        end; (* case *)
    } else {
        error(errBadSymbol);
        goto 8888;
    };
    (* Any factor producing an rvalue/lvalue may be followed by postfix
       operators (@ for pointer/file deref, .field for struct member,
       [idx] for array element).  parseLval already drained them above;
       parsePostfix is a no-op when SY is not a postfix token. *)
    parsePostfix;

}; (* factor *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseUnaryExpression;
var
    oper: operator;
    leftExpr, addExpr, oneExpr: eptr;
{
    oper := NOOP;
    if (charClass IN [PLUSOP,MINUSOP,BITNEGOP,NOTOP,
                      MUL,SETAND,INCROP,DECROP]) then {
        if (charClass <> PLUSOP) then
            oper := charClass;
        inSymbol;
    };
    if oper <> NOOP then
        parseUnaryExpression
    else
        factor;
    if oper <> NOOP then {
        arg1Type := curExpr@.vt.typ;
        new(leftExpr);
        with leftExpr@ do {
            vt.typ := arg1Type;
            expr1 := curExpr;
            case oper of
            MINUSOP: {
                if arg1Type = realType then {
                    op := RNEGOP;
                } else if typeCheck(arg1Type, integerType) then {
                    leftExpr@.op := INEGOP;
                    leftExpr@.vt.typ := integerType;
                } else {
                    error(69); (* errUnaryMinusNeedRealOrInteger *)
                    exit
                };
            };
            BITNEGOP: {
                if typeCheck(arg1Type, integerType) then {
                    leftExpr@.op := BITNEGOP;
                    leftExpr@.vt.typ := integerType;
                } else {
                    error(62); (* errIntegerNeeded *)
                    exit
                };
            };
            NOTOP: with leftExpr@ do {
                vt.typ := booleanType;
                if (arg1Type = booleanType) then {
                    op := NOTOP;
                } else if (arg1Type = integerType) then {
                    op := EQOP;
                    new(expr2);
                    expr2@ := [integerType, GETENUM, 0C];
                } else {
                    error(errNeedOtherTypesOfOperands);
                    exit
                };
            };
            MUL: with leftExpr@ do {
                    if (arg1Type.p.pk = kindPtr) then {
                        vt.typ := arg1Type.rep@.base;
                        op := DEREF;
                    } else if (arg1Type.p.pk = kindFile) then {
                        vt.typ := arg1Type.rep@.base;
                        op := FILEPTR;
                    } else {
                        stmtName := 'unary*';
                        error(errWrongVarTypeBefore);
                    }
            };
            SETAND: {
                if not (curExpr@.op IN lvalOpSet) then
                    error(27); (* errExpressionWhereVariableExpected *)
                with leftExpr@ do {
                    vt.typ := voidPtr;
                    op := STANDPROC;
                    num2 := fnREF;
                }
            };
            INCROP, DECROP: with leftExpr@ do {
                (* Pre-increment (++x) / pre-decrement (--x):
                   lower to RMWASSIGN(x, INTPLUS|INTMINUS(1, NIL)).
                   Codegen materialises x's address once (when needed)
                   and reuses the spill slot for both load and store,
                   so side-effectful lvalues fire only once. *)
                if not (curExpr@.op IN lvalOpSet) then {
                    error(27); (* errExpressionWhereVariableExpected *)
                    exit
                };
                if not typeCheck(arg1Type, integerType) then {
                    error(62); (* errIntegerNeeded *)
                    exit
                };
                new(oneExpr);
                oneExpr@ := [integerType, GETENUM, 1C];
                new(addExpr);                  (* INTPLUS / INTMINUS node *)
                with addExpr@ do {
                    vt.typ := integerType;
                    if (oper = INCROP) then
                        op := INTPLUS
                    else
                        op := INTMINUS;
                    expr1 := oneExpr;          (* RHS = 1 *)
                    expr2 := NIL;                (* don't-care slot *)
                };
                vt.typ := integerType;
                op := RMWASSIGN;
                expr2 := addExpr;
            }
            end;
            curExpr := leftExpr;
        }
    };
}; (* parseUnaryExpression *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parsePrc(minPrec: integer);
var
    oper: operator;
    leftExpr, thenExpr: eptr;
    curPrec: integer;
    match: boolean;
{
    (* Parse left operand with unary operators *)
    parseUnaryExpression;

    (* Climb through operators at this precedence level and higher *)
    while true do {
        curPrec := getPrec(SY, charClass);

        (* Stop if operator has lower precedence than minimum *)
        if curPrec < minPrec then
            exit;

        oper := charClass;
        inSymbol;
        leftExpr := curExpr;

        if (oper = CONDOP) then {
            (* Right-associative ternary: cond ? thenExpr : elseExpr *)
            parsePrc(precAssign);
            if (SY <> COLON) then
                requiredSymErr(COLON)
            else
                inSymbol;
            thenExpr := curExpr;
            parsePrc(precCond);
            bldCondOp(leftExpr, thenExpr);
        } else if (curPrec = precAssign) then {
            (* Right-associative assignment: lhs [op]= rhs.  `oper` (captured
               above before inSymbol) is ASSIGNOP for plain `=`; for op-assign
               (+=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=) it carries the
               underlying operation, lexed as SY=BECOMES + charClass=op.
               Plain `=` yields ASSIGNOP(lhs, rhs).  Op-assign yields
               RMWASSIGN(lhs, inner-op(rhs, NIL)) where inner-op carries the
               operator (e.g. INTPLUS) and the RHS in expr1; expr2 is the
               don't-care slot.  Codegen for RMWASSIGN walks lhs once,
               materialising its address into a spill slot when needed, then
               synthesises the equivalent ASSIGNOP for emission. *)
            if not (leftExpr@.op IN lvalOpSet) then
                error(27); (* errExpressionWhereVariableExpected *)
            parsePrc(precAssign);
            arg1Type := leftExpr@.vt.typ;
            arg2Type := curExpr@.vt.typ;
            if (oper <> ASSIGNOP) then {
                (* Reuse bldArithOp/bldBitOp/bldLogOp for operator selection
                   (PLUSOP vs INTPLUS, etc.) and type promotion, then drop
                   the leftExpr slot of the result so it stores op(rhs, NIL)
                   ready for RMWASSIGN.expr2.  RMWASSIGN.expr1 carries the
                   original lvalue subtree, evaluated once at codegen time. *)
                match := typeCheck(arg1Type, arg2Type);
                case opPrec[oper] of
                    precMul,
                    precAdd:    bldArithOp(oper, leftExpr, match);
                    precShift,
                    precBitAnd,
                    precBitXor,
                    precBitOr:  bldBitOp(oper, leftExpr);
                    precAnd,
                    precOr:     bldLogOp(oper, leftExpr, match);
                end;
                curExpr@.expr1 := curExpr@.expr2;
                curExpr@.expr2 := NIL;
                arg2Type := curExpr@.vt.typ;
            };
            if not typeCheck(arg1Type, arg2Type) then {
                if (arg1Type = realType) and
                   typeCheck(integerType, arg2Type) then
                    castToReal(curExpr)
                else
                    error(33); (*errIllegalTypesForAssignment*)
            };
            new(thenExpr);
            with thenExpr@ do {
                vt.typ := arg1Type;
                if (oper <> ASSIGNOP) then
                    op := RMWASSIGN
                else
                    op := ASSIGNOP;
                expr1 := leftExpr;
                expr2 := curExpr;
            };
            curExpr := thenExpr;
        } else {
            (* Recursively parse right operand with higher precedence *)
            (* For left-associative: use curPrec + 1 *)
            parsePrc(curPrec + 1);

            (* Build AST node based on operator type *)
            arg1Type := curExpr@.vt.typ;
            arg2Type := leftExpr@.vt.typ;
            match := typeCheck(arg1Type, arg2Type);

            case curPrec of
                precMul: bldArithOp(oper, leftExpr, match);
                precAdd: bldArithOp(oper, leftExpr, match);
                precRel,
                precEq: bldRelOp(oper, leftExpr);
                precShift,
                precBitAnd,
                precBitXor,
                precBitOr: bldBitOp(oper, leftExpr);
                precAnd,
                precOr: bldLogOp(oper, leftExpr, match);
            end;
        }
    }
}; (* parsePrc *)
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
{
    if (readNext) then
        inSymbol
    else
        readNext := true;
    parsePrc(precAssign);
}; (* expression *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure setStrLab;
{
    new(strLabPtr);
    padToLeft;
    disableNorm;
    with strLabPtr@ do {
        next := strLabList;
        ident := curIdent;
        target := 0;
    };
    strLabList := strLabPtr;
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure setBrCont;
{
    curIdent.i := 4262454153C;
    setStrLab; (* break *)
    curIdent.i := 4357566451566545C;
    setStrLab; (* continue *)
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure brContTarget;
{
    (* assigning target for break/continue if used *)
    with strLabList@ do if (target <> 0) then {
        fixup(0, target);
    };
    strLabList  :=  strLabList@.next; (* removing break/continue *)
};
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
        readNext := false;
        expression;
        formOperator(DOIT);
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
        readNext := false;
        expression;
        loopExpr := curExpr;
    };
    checkSymAndRead(RPAREN);
    setBrCont;
    statement;
    brContTarget; (* removing continue *)
    if (loopExpr <> NIL) then {
        curExpr := loopExpr;
        formOperator(DOIT);
    };
    formJump(toLoop);
    if (leave <> 0) then {
        padToLeft;
        fixup(0, leave);
    };
    brContTarget; (* removing break *)
}; (* forStatement *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure withStatement;
var
    oldWith: eptr;
    l4var2z, l4var3z: bitset;
    l4var4z: integer;
{
    oldWith := withList;
    l4var4z := localSize;
    l4var2z := freeRegs;
    l4var3z := [];
    repeat
            expression;
            if (curExpr@.vt.typ.p.pk = kindStruct) then {
                formOperator(SETREG);
                l4var3z := (l4var3z + [curVal.i]) * auxRegs;
            } else {
                error(71); (* errWithOperatorNotOfARecord *)
            };
    until (SY <> COMMA);
    checkSymAndRead(DOSY);
    statement;
    withList := oldWith;
    localSize := l4var4z;
    freeRegs := l4var2z;
    usedRegs := usedRegs + l4var3z;
}; (* withStatement *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure reportStmtType;
{
    writeln(' STATEMENT ', stmtname:0, ' IN ', startLine:0, ' LINE');
}; (* reportStmtType *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure structBranch;
var
    curLab: @strLabel;
{
    curLab := strLabList;
    while (curLab <> NIL) do {
        if curLab@.ident = curIdent then {
            formJump(curLab@.target);
            exit
        };
        curLab := curLab@.next;
    };
    error(errNotDefined);
    goto 8888;
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
    allClauses, curClause, clause, prev: @casechain;
    isIntCase: boolean;
    otherSeen: boolean;
    otherOffset: integer;
    itemsEnded, goodMode: boolean;
    firstType, itemtype, exprtype: tptr;
    itemvalue: word;
    itemSpan: integer;
    expected: word;
    startLine, decoder, endOfStmt: integer;
    minValue, unused2, maxValue: word;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* caseStatement *)
    startLine := lineCnt;
    parentExpression;
    exprtype := curExpr@.vt.typ;
    otherSeen := false;
    if (exprtype = alfaType) or
       (exprtype.p.pk = kindScalar) then
        formOperator(LOAD)
    else
        error(25); (* errExprNotOfADiscreteType *)
    disableNorm;
    decoder := 0;
    endOfStmt := 0;
    allClauses := NIL;
    formJump(decoder);
    checkSymAndRead(BEGINSY);
    firstType.rep := NIL;
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
            } else {
                if (SY <> CASESY) then
                    requiredSymErr(CASESY);
                expression;
                formOperator(LITINSN);
                itemvalue := curVal;
                itemType := insnList@.typ;
                if (itemtype.rep <> NIL) then {
                    if (firstType.rep = NIL) then {
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
                            prev := curClause;
                            curClause := curClause@.next;
                        }
                    };
                    if (curClause = allClauses) then {
                        clause@.next := allClauses;
                        allClauses := clause;
                    } else {
                        clause@.next := curClause;
                        prev@.next := clause;
                    };
                }
            };
            checkSymAndRead(COLON);
            while not (SY in [CASESY,DEFAULTSY,ENDSY]) do
                statement;
            goodMode := goodMode and (arithMode = 1);
        };
        itemsEnded := (SY = ENDSY);
        if SY = SEMICOLON then
            inSymbol;
    until itemsEnded;
    if (SY <> ENDSY) then {
        requiredSymErr(ENDSY);
        stmtName := 'CASE  ';
        reportStmtType;
    } else
        inSymbol;
    if not typeCheck(firstType, exprtype) then {
        error(88); (* errDifferentTypesOfLabelsAndExpr *);
        exit
    };
    formJump(endOfStmt);
    padToLeft;
    isIntCase := typeCheck(exprtype, integerType);
    if (allClauses <> NIL) then {
        expected := allClauses@.value;
        minValue := expected;
        curClause := allClauses;
        while (curClause <> NIL) do {
            if (expected = curClause@.value) and
               (exprtype.p.pk = kindScalar) then {
                maxValue := expected;
                expected.c := succ(expected.c);
                curClause := curClause@.next;
            } else {
                itemSpan := 34000;
                fixup(0, decoder);
                if (firstType.p.pk = kindScalar) then
                    itemSpan := firstType.rep@.numen;
                itemsEnded := itemSpan < 32000;
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
        fixup(0, decoder);
        curVal := minValue;
        fixup(-(insnTemp[U1A]+otherOffset), maxValue.i);
        curVal.m := minValue.m + intZero;
        curVal.i := curVal.i div 2;
        form3Insn(ASN64+1, KATI+14, KYTA);
        curVal.i := moduleOffset + 1 - curVal.i;
        if (curVal.i < 40000B) then {
            curVal.i := curVal.i - 40000B;
            curVal.i := allocSymtab([24, 29] + (curVal.m * O77777));
        };
        form1Insn(KUJ+I14 + curVal.i);
        padToLeft;
        if (odd(minValue.i)) then {
            form1Insn(KUTC);
            decoder := ord(UJ);
        } else
            decoder := ord(UZA);
        while (allClauses <> NIL) do {
            form1Insn((*=c-*)insnTemp[decoder](*=c+*) + allClauses@.offset);
            allClauses := allClauses@.next;
            decoder := ord(UZA) + ord(UJ) - decoder;
        };
        16211:
        fixup(0, endOfStmt);
        if (not goodMode) then
           disableNorm;

    }
}; (* caseStatement *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure ifWhileStatement;
var eq : eptr;
{
    disableNorm;
    parentExpression;
    if (curExpr@.vt.typ.p.pk > kindPtr) then
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
    l4var7z, savedVal, l4var9z: word;
    F: file of DATAREC;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure putDataRec(count: integer);
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
{ (* putDataRec *)
    rec.a[0] := allocDataRef(length);
    writeln(' datarec ', rec.b oct);
    if (FcstCnt = l4var3z) then {
        curVal := savedVal;
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
}; (* putDataRec *)
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
        l4var7z.m := leftInsn * [12..23];
        l4var3z := FcstCnt;
        length := 0;
        l4var9z.i := 0;
        repeat
            expression;
            formOperator(LITINSN);
            savedVal := curVal;
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
                    putDataRec(1);
                length := 1;
                putDataRec(repCount);
            } else {
                length := length + 1;
                if (SY = COMMA) then {
                    curVal := savedVal;
                    toFCST;
                } else {
                    if (length <> 1) then {
                        curVal := savedVal;
                        toFCST;
                    };
                    putDataRec(1);
                }
            };
        until SY <> COMMA;
        myrollup(ord(boundary));
    until SY <> SEMICOLON;
    if (SY <> ENDSY) then
        error(errBadSymbol);
    reset(F);
    while not eof(F) do {
        write(FCST, F@.b);
        get(F);
    };
    lookup2 := FcstCnt - dsize;
    FcstCnt := dsize;
    lookupMode := setcount;

}; (* parseData *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseConstExpression;
{
    readNext := false;
    ceTyp := voidType;
    ceVal.i := 1;
    expression;
    formOperator(LITINSN);
    ceTyp := insnList@.typ;
    ceVal := curVal;
    myrollup(ord(boundary));
}; (* parseConstExpression *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure standProc;
label
    44;
var
    l4typ1z, l4typ2z, l4typ3z: tptr;
    firstWidth, secondWidth: eptr;
    l4exp6z: eptr;
    l4exp7z, l4exp8z, workExpr: eptr;
    l4bool10z,
    noWidth, needR12: boolean;
    isCharFile: boolean;
    oldOffset: integer;
    defWidth: integer;
    procNo: integer;
    helperNo: integer;
    indCnt: integer;
    opToForm: opgen;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure verifyType(t: tptr);
{
    readNext := false;
    expression;
    if (t <> voidType) and
        not typeCheck(t, curExpr@.vt.typ) then {
        error(errNeedOtherTypesOfOperands);
        curExpr := uVarPtr;
    }
}; (* verifyType *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure startWrite;
{
    expression;
    l4typ3z := curExpr@.vt.typ;
    l4exp7z := curExpr;
    if (workExpr = NIL) then {
        if (l4typ3z.p.pk = kindFile) then {
            workExpr := curExpr;
        } else {
            new(workExpr);
            workExpr@.vt.typ := textType;
            workExpr@.op := GETVAR;
            workExpr@.id1 := outputFile;
        };
        arg2Type := workExpr@.vt.typ;
        (* typeCheck(arg2Type@.base, charType); *)
        isCharFile := arg2Type.rep@.base = charType;
        needR12 := true;
        new(l4exp8z);
        l4exp8z@.vt.typ := arg2Type.rep@.base;
        l4exp8z@.op := FILEPTR;
        l4exp8z@.expr1 := workExpr;
        new(l4exp6z);
        l4exp6z@.vt.typ := l4exp8z@.vt.typ;
        l4exp6z@.op := ASSIGNOP;
        l4exp6z@.expr1 := l4exp8z
    }
}; (* startWrite *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
function parseWidthSpecifier: eptr;
{
    expression;
    if not typeCheck(integerType, curExpr@.vt.typ) then {
        error(14); (* errExprIsNotInteger *)
        parseWidthSpecifier := uVarPtr;
    } else
        parseWidthSpecifier := curExpr;
}; (* parseWidthSpecifier *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure callHelperWithArg;
{
    if ([12] <= usedRegs) or needR12 then {
        curExpr := workExpr;
        formOperator(SETREG12);
    };
    needR12 := false;
    formAndAlign(getHelperProc(helperNo));
    disableNorm;
}; (* callHelperWithArg *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure checkElementForReadWrite;
{
    usedRegs := usedRegs - [12];
    curVarKind := l4typ3z.p.pk;
    helperNo := 36;               (* C/WI *)
    if ((l4typ3z = integerType) or (l4typ3z = booleanType)) then
        defWidth := 10
    else if (l4typ3z = realType) then {
        helperNo := 37;               (* P/WR *)
        defWidth := 14;
    } else if (l4typ3z = charType) then {
        helperNo := 38;               (* P/WC *)
        defWidth := 1;
    } else if (curVarKind = kindScalar)
          and (l4typ3z.rep@.start <> -1) then {
        helperNo := 41;               (* P/WX *)
        dumpEnumNames(l4typ3z);
        defWidth := 8;
    } else if (isCharArray(l4typ3z)) then {
        defWidth := l4typ3z.rep@.aright - l4typ3z.rep@.aleft + 1;
        if not (l4typ3z.rep@.pck) then
            helperNo := 81            (* P/WA *)
        else if (6 >= defWidth) then
            helperNo := 39            (* P/A6 *)
        else
            helperNo := 40;           (* P/A7 *)
    } else if (l4typ3z.p.psize = 1) then {
        helperNo := 42;               (* P/WO *)
        defWidth := 17;
    } else {
        error(34); (* errTypeIsNotAFileElementType *)
    };
}; (* checkElementForReadWrite *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure writeProc;
{
    workExpr := NIL;
    isCharFile := true;
    repeat {
        startWrite;
        if (l4exp7z <> workExpr) then {
            if not isCharFile then {
                helperNo := 29;         (* P/PF *)
                usedRegs := usedRegs - [12];
                if not typeCheck(l4exp8z@.vt.typ, l4exp7z@.vt.typ) then
                    error(34) (* errTypeIsNotAFileElementType *)
                else {
                    l4exp6z@.expr2 := l4exp7z;
                    curExpr := l4exp6z;
                    formOperator(DOIT);
                    callHelperWithArg;
                }
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
                } else if (curToken = litOct) then {
                    helperNo := 42; (* P/WO *)
                    defWidth := 17;
                    if (l4typ3z.p.psize <> 1) then
                        error(34); (* errTypeIsNotAFileElementType *)
                    inSymbol;
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
                        form1Insn(KAOX+ZERO);
                    }
                };
                if (helperNo = 37) then {       (* P/WR *)
                    if (secondWidth = NIL) then {
                        curVal.i := 4;
                        form1Insn(KXTS+I8 + getFCSToffset);
                    } else {
                        curExpr := secondWidth;
                        formOperator(FRACWIDTH);
                        form1Insn(KAOX+ZERO);
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
                        form1Insn(KVTM+I11 + l4typ3z.rep@.start);
                };
                callHelperWithArg;
            }
        }
    } until (SY <> COMMA);
    if (procNo = 11) then {
        helperNo := 46;                 (* P/WL *)
        callHelperWithArg;
    };
    usedRegs := usedRegs + [12];
    if (oldOffset = moduleOffset) then
        error(36); (*errTooFewArguments *)
}; (* writeProc *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure checkArrayArg;
{
    verifyType(voidType);
    workExpr := curExpr;
    l4typ1z := curExpr@.vt.typ;
    if (l4typ1z.rep@.pck) or
       (l4typ1z.p.pk <> kindArray) then
        error(errNeedOtherTypesOfOperands);
    checkSymAndRead(COMMA);
    readNext := false;
    expression;
    l4exp8z := curExpr;
}; (* checkArrayArg *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure doPackUnpack;
var
    t: tptr;
{
    new(l4exp7z);
    l4exp7z@.vt.typ := l4typ1z.rep@.base;
    l4exp7z@.op := GETELT;
    l4exp7z@.expr1 := workExpr;
    l4exp7z@.expr2 := l4exp8z;
    t := l4exp6z@.vt.typ;
    if (t.p.pk <> kindArray) or
       not t.rep@.pck or
        (t.rep@.base.p.pk <> kindScalar) or
        (l4typ1z.rep@.base.p.pk <> kindScalar) then
        error(errNeedOtherTypesOfOperands);
    new(curExpr);
    curExpr@.vt.c := chr(procNo + 50);
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
       (procNo IN [0:5,10,12,15:28]) then
        error(45); (* errNoOpenParenForStandProc *)
    if (procNo IN [0:5,12,15]) then {
        expression;
        if not (curExpr@.op IN lvalOpSet) then {
            error(27); (* errExpressionWhereVariableExpected *)
        };
        arg1Type := curExpr@.vt.typ;
        curVarKind := arg1Type.p.pk;
    };
    if (procNo IN [0..6]) then
        jumpTarget := getHelperProc(29 + procNo); (* P/PF *)
    case procNo of
    0, 1, 2, 3: { (* put, get, rewrite, reset *)
        if (arg1Type.p.psize <> 30) then
            error(47); (* errNoVarOfFileType *)
        if (procNo = 3) and
           (SY = COMMA) then {
            formOperator(SETREG12);
            expression;
            if (not typeCheck(integerType, curExpr@.vt.typ)) then
                error(14); (* errExprIsNotInteger *)
            formOperator(LOAD);
            formAndAlign(getHelperProc(97)); (*"P/RE"*)
        } else {
            formOperator(FILEACCESS);
        }
    };
    5: { (* free *)
        if (curVarKind <> kindPtr) then
            error(13); (* errVarIsNotPointer *)
        heapCallsCnt := heapCallsCnt + 1;
        workExpr := curExpr;
        formOperator(SETREG9);
        l2typ13z := arg1Type.rep@.base;
        ii := l2typ13z.p.psize;
        if (SY = COLON) then {
            expression;
            if not typeCheck(integerType, curExpr@.vt.typ) then
                error(14); (* errExprIsNotInteger *)
            if (curExpr@.op = GETENUM) then {
                curExpr@.lit.m := curExpr@.lit.m + intZero;
                ii := curExpr@.lit.i; goto 44;
            } else {
                formOperator(LOAD);
                form1Insn(KATI+14);
            }
        } else {
44:         form1Insn(KVTM+I14+getValueOrAllocSymtab(ii));
        };
        formAndAlign(jumpTarget);
    };
    6: { (* halt *)
        formAndAlign(jumpTarget);
        exit
    };
    10: { (* write *)
        writeProc;
    };
    11:
       { (* writeln *)
        if (SY = LPAREN) then {
            writeProc;
        } else {
            formAndAlign(getHelperProc(54)); (*"P/WOLN"*)
            exit
        }
    };
    12: { (* ctor(lvalue, expr0, expr1, ...): struct-constructor assignment.
            Lvalue (already parsed above) must be of kindStruct; each comma-
            separated expression is stored at successive word offsets from
            the lvalue address, using register 9 as the base.  Empty argument
            positions are allowed and silently skip an offset. *)
        if (curVarKind <> kindStruct) then
            error(errNeedOtherTypesOfOperands);
        formOperator(SETREG9);
        indCnt := 0;
        inSymbol;          (* consume the comma between lvalue and expr0 *)
(args)  {
            if (SY = COMMA) then {
                indCnt := indCnt + 1;
                inSymbol;
            } else if (SY = RPAREN) then {
                exit args;
            } else {
                readNext := false;
                expression;
                curVal.i := indCnt;
                formOperator(STOREAT9);
            };
            goto args;
        };
    };
    14: { (* return [expr] *)
        if not (SY IN statEndSys) then {
            (* return expr: load expr to ACC, then jump *)
            if (procName@.typ = voidType) then
                error(errNeedOtherTypesOfOperands)
        else {
            if (hasFiles <> 0) then {
                writeln(' functions must not use files');
                error(200);
            };
                retSeen := true;
                readNext := false;
                expression;
                if (typeCheck(procName@.typ, curExpr@.vt.typ)) then
                    (* OK *)
                else if (procName@.typ = realType) and
                        typeCheck(integerType, curExpr@.vt.typ) then
                    castToReal(curExpr)
                else
                    error(33); (* errIllegalTypesForAssignment *)
                formOperator(LOAD);
            }
        } else if (procName@.typ <> voidType) then
            error(errNeedOtherTypesOfOperands);
        form1Insn(getHelperProc(27) + (KUJ-KVJM-I13));
        exit
    };
    16: { (* besm *)
        expression;
        formOperator(LITINSN);
        formAndAlign(curVal.i);
    };
    19, 20: { (* pck, unpck *)
        inSymbol;
        verifyType(charType);
        checkSymAndRead(COMMA);
        formOperator(SETREG12);
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
        verifyType(voidType);
        l4exp6z := curExpr;
        doPackUnpack;
    };
    22: { (* unpack *)
        inSymbol;
        verifyType(voidType);
        l4exp6z := curExpr;
        checkSymAndRead(COMMA);
        checkArrayArg;
        doPackUnpack;
    };
    end;
    if procNo in [0,1,2,3,5,10,11,13,21,22] then
        arithMode := 1;
    checkSymAndRead(RPAREN);

}; (* standProc *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* statement *)
    if (freeRegs <> ceRegs) and (SY = SEMICOLON) then {
        inSymbol;
        exit; (* empty statement *)
    };
    setup(boundary);
    bool110z := false;
    startLine := lineCnt;
    if freeRegs = halfWord then
        parseData
    else if freeRegs = ceRegs then {
        parseConstExpression;
        exit;
    }
    else {
        if SY = INTCONST then {
            liveRegs := [];
            disableNorm;
            flag := true;
            padToLeft;
            labCheckAndDefine(true);
            inSymbol;
            checkSymAndRead(COLON);
        };
        nest := SY IN [BEGINSY,SWITCHSY];
        if nest then
            lineNesting := lineNesting + 1;
(ident)
        if SY = IDENT then {
            if hashTravPtr <> NIL then {
                l3var6z := hashTravPtr@.cl;
                if l3var6z = ROUTINEID then {
                    l3idr12z := hashTravPtr;
                    if l3idr12z@.offset = 0 then {
                        (* System procedure (WRITE, PUT, GET, NEW, ...):
                           special syntax, handled directly. *)
                        inSymbol;
                        standProc;
                        checkSymAndRead(SEMICOLON);
                        exit ident;
                    };
                    if l3idr12z@.typ = voidType then {
                        (* User procedure call (void return): not a valid
                           expression in factor(), so dispatch directly to
                           parseCallArgs. *)
                        inSymbol;
                        parseCallArgs(l3idr12z);
                        formOperator(DOIT);
                        checkSymAndRead(SEMICOLON);
                        exit ident;
                    };
                };
                (* VARID / FORMALID / FIELDID, or ROUTINEID with non-NIL
                   typ (function call): assignment, function call, or other
                   expression used as a statement.  readNext := false keeps
                   the current SY (the leading IDENT) for expression() to
                   consume. *)
                readNext := false;
                expression;
                formOperator(DOIT);
                checkSymAndRead(SEMICOLON);
            } else {
                error(errNotDefined);
8888:           skip(skipToSet + statEndSys);
            };
        } else if (SY IN [EXPROP, LPAREN, INTCONST,
                          REALCONST, CHARCONST, STRINGSY, LBRACK]) then {
            (* Generic expression statement: '++x;', '(x = 1);', etc.
               Includes pre-increment / pre-decrement, parenthesised
               expressions, unary operators and so on.  readNext := false
               keeps the current SY for expression() to consume. *)
            readNext := false;
            expression;
            formOperator(DOIT);
            checkSymAndRead(SEMICOLON);
        } else  if (SY = BEGINSY) then
(rep)   {
            inSymbol;
(skip)      {
                while SY <> ENDSY do statement;
                if (SY <> ENDSY) then {
                    stmtName := ' BEGIN';
                    requiredSymErr(SEMICOLON);
                    reportStmtType;
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
                error(62); (* errIntegerNeeded *)
                goto 8888;
            };
            disableNorm;
            labCheckAndDefine(false);
            inSymbol;
        } else  if (SY = IFSY) then {
            ifWhileStatement;
            if (SY = ELSESY) then {
                elseJump := 0;
                formJump(elseJump);
                fixup(0, ifWhlTarget);
                curOffset.i := arithMode;
                arithMode := 1;
                inSymbol;
                statement;
                fixup(0, elseJump);
                if (curOffset.i <> arithMode) then {
                    arithMode := 2;
                    disableNorm;
                }
            } else {
                fixup(0, ifWhlTarget);
            }
        } else  if (SY = WHILESY) then {
            liveRegs := [];
            setBrCont;
            strLabList@.target := moduleOffset;
            curOffset.i := moduleOffset;
            ifWhileStatement;
            disableNorm;
            form1Insn(insnTemp[UJ] + curOffset.i);
            fixup(0, ifWhlTarget);
            strLabList := strLabList@.next; (* removing continue *)
            brContTarget; (* removing break *)
            arithMode := 1;
        } else if (SY = BREAKSY) or (SY = CONTSY) then {
            structBranch;
            inSymbol;
            checkSymAndRead(SEMICOLON);
        } else  if (SY = DOSY) then {
            liveRegs := [];
            setBrCont;
            curOffset.i := moduleOffset;
            inSymbol;
            statement;
            brContTarget; (* removing continue *)
            if (SY <> WHILESY) then {
                requiredSymErr(WHILESY);
                stmtName := '  DO  ';
                reportStmtType;
                goto 8888;
            };
            disableNorm;
            parentExpression;
            if (curExpr@.vt.typ<>booleanType)and
               (curExpr@.vt.typ<>integerType) then {
                error(errBooleanNeeded)
            } else {
                jumpTarget := curOffset.i;
                whileExpr := curExpr;
                new(curExpr);
                with curExpr@do {
                    vt.typ := booleanType;
                    op := NOTOP;
                    expr1 := whileExpr;
                };
                formOperator(BRANCH);
            };
            brContTarget; (* removing break *)
        } else
        if (SY = FORSY) then {
            liveRegs := [];
            forStatement;
        } else  if (SY = SWITCHSY) then {
            curIdent.i := 4262454153C;
            setStrLab;
            caseStatement;
            brContTarget; (* removing break *)
        } else if (SY = WITHSY) then {
            withStatement;
        };
        if (nest) then
            lineNesting := lineNesting - 1;
        myrollup(ord(boundary));
        if (bool110z) then {
            bool110z := false;
            goto 8888;
        }
    }
}; (* statement *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure parseConstDeclValue(var typ: tptr; var value: word);
var
    savedFreeRegs: bitset;
{
    if (SY = STRINGSY) then {
        parseLiteral(typ, value, true);
        inSymbol;
        exit;
    };
    savedFreeRegs := freeRegs;
    freeRegs := ceRegs;
    statement;
    freeRegs := savedFreeRegs;
    typ := ceTyp;
    value := ceVal;
}; (* parseConstDeclValue *)
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
procedure defineRoutine(bodyBlock: boolean);
var
    l3var1z, l3var2z, l3var3z: word;
    l3int4z: integer;
    l3idr5z: irptr;
    l3var6z, l3var7z: word;
{
    objBufIdx := 1;
    objBuffer[objBufIdx] := [];
    curInsnTemplate := insnTemp[XTA];
    bool48z := 22 IN procName@.flags;
    lineStartOffset := moduleOffset;
    l3var1z := ;
    lookup2 := lookWith;
    withList := NIL;
    arithMode := 1;
    liveRegs := [];
    freeRegs := [curProcNesting+1..6];
    auxRegs := freeRegs - [minel(freeRegs)];
    l3var7z.m := freeRegs;
    usedRegs := [1:15] - freeRegs;
    if (curProcNesting <> 1) then
        parseDecls(2);
    l2int21z := localSize;
    if not bodyBlock and (SY <> BEGINSY) then
        requiredSymErr(BEGINSY);
    if 23 IN procName@.flags then {
        l3idr5z := procName@.argList;
        l3int4z := 3;
        if (procName@.typ <> voidType) then
        l3int4z := 4;
        while (l3idr5z <> procName) do {
            if (l3idr5z@.cl = VARID) then {
                l3var2z.i := l3idr5z@.typ.p.psize;
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
    if not (NoStackCheck IN optSflags.m) then
        fixup(-1, 95); (* P/SC *)
    l3var2z.i := lineNesting;
    if bodyBlock then {
        while (SY <> ENDSY) and (CH <> '_000') do
            statement;
        if (SY <> ENDSY) then
            requiredSymErr(ENDSY)
        else
            inSymbol;
    } else {
        repeat
            statement;
            if (curProcNesting = 1) then
                done := (SY = PERIOD) or (CH = '_000')
            else
                done := (SY IN blockBegSys) or (SY = TYPESY) or (CH = '_000');
            if not done then
               if (curProcNesting = 1) then
                   requiredSymErr(PERIOD)
               else {
                   errAndSkip(errBadSymbol, skipToSet);
               }
        until done;
    };
    procName@.flags := (usedRegs * [0:15]) + (procName@.flags - l3var7z.m);
    lineNesting := l3var2z.i - 1;
    if not bool48z and not doPMD and (l2int21z = 3) and
       (curProcNesting <> 1) and (usedRegs * [1:15] <> [1:15]) then {
        objBuffer[1] := [7:11,21:23,28,31]; (* ,NTR,7; ,UTC, *)
        with procName@ do
            flags := flags + [25];
        if (objBufIdx = 2) then {
            objBuffer[1] := [0,1,3:5]; (* 13,UJ, *)
            putLeft := true;
        } else {
            procName@.pos := l3var1z.i;
            if 13 IN usedRegs then {
                curVal.i := minel([1:15] - usedRegs);
                besm(ASN64-24);
                l3var7z := ;
                objBuffer[2] := objBuffer[2] + [0,1,3,6,9] + l3var7z.m;
            } else {
                curVal.i := (13);
            };
            form1Insn(insnTemp[UJ] + indexreg[curVal.i]);
        }
    } else  {
        if (hasFiles = 0) then
            jj := 27    (* C/E *)
        else
            jj := 28;   (* C/EF *)
        form1Insn(getHelperProc(jj) + (-I13-100000B));
        if (curProcNesting = 1) then {
            parseDecls(2);
            if S3 IN optSflags.m then
                formAndAlign(getHelperProc(78)); (* "P/PMDSET" *)
            form1Insn(insnTemp[UJ] + l3var1z.i);
            curVal.i := procName@.pos - 40000B;
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
    l3var1z, outName, inName: word;
    l3var5z, l3var6z: integer;
    l3var7z: irptr;
    l3var8z, sysProcNum: integer;
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
    curIdRec@ := [l4arg1z, 0, , temptype, ROUTINEID, sysProcNum];
    sysProcNum := sysProcNum + 1;
    addToHashTab(curIdRec);
}; (* registerSysProc *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure defExtern;
var l : integer;
{
    l := 0;
    curVal := curIdent;
    l3var1z.m := leftAlign;
    if (curIdent = inName) then {
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
        l := lineCnt;
    } else if (curIdent = outName) then {
        outputFile := l3var7z;
        l := lineCnt;
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
        line := l;
        offset := l3var1z.i;
    };
    if l <> 0 then {
        if (curIdent = outName) then {
            fileForOutput := curExternFile;
        } else {
            fileForInput := curExternFile;
        }
    };
    externFileList := curExternFile;
    l3var6z := l3var5z;
    l3var5z := l3var5z + 1;
    curExternFile@.location := 512;
};
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* initScalars *)
    new(booleanType.rep, kindScalar);
    with booleanType.rep@ do {
        numen := 2;
        start := 0;
        enums := NIL;
    };
    booleanType.p.psize := 1;
    booleanType.p.bits := 1;
    booleanType.p.pk := kindScalar;
    new(integerType.rep, kindScalar);
    with integerType.rep@ do {
        numen := 100000;
        start := -1;
        enums := NIL;
    };
    integerType.p.psize := 1;
    integerType.p.bits := 48;
    integerType.p.pk := kindScalar;
    new(charType.rep, kindScalar);
    with charType.rep@ do {
        numen := 256;
        start := -1;
        enums := NIL;
    };
    charType.p.psize := 1;
    charType.p.bits := 8;
    charType.p.pk := kindScalar;
    realType.rep := NIL;
    realType.p.psize := 1;
    realType.p.bits := 48;
    realType.p.pk := kindReal;
    voidType.rep := NIL;
    voidType.p.psize := 1;
    voidType.p.bits := 48;
    voidType.p.pk := kindVoid;
    new(voidPtr.rep, kindPtr);
    with voidPtr.rep@ do {
        base := voidType;
    };
    voidPtr.p.psize := 1;
    voidPtr.p.bits := 15;
    voidPtr.p.pk := kindPtr;
    new(textType.rep, kindFile);
    with textType.rep@ do {
        base := charType;
    };
    textType.p.pad := 8;
    textType.p.psize := 30;
    textType.p.bits := 48;
    textType.p.pk := kindFile;
    new(alfaType.rep,kindArray);
    with alfaType.rep@ do {
        base := charType;
        pck := true;
        perword := 6;
        pcksize := 8;
        aleft := 1;
        aright := 6;
    };
    alfaType.p.psize := 1;
    alfaType.p.bits := 48;
    alfaType.p.pk := kindArray;
    smallStringType[6] := alfaType;
    regSysType(515664C  (*"     INT"*), integerType);
    regSysType(43504162C(*"    CHAR"*), charType);
    regSysType(62454154C(*"    REAL"*), realType);
    regSysType(41544641C(*"    ALFA"*), alfaType);
    regSysType(64457064C(*"    TEXT"*), textType);
    tempType := voidPtr;
    regSysEnum(565154C(*"     NIL"*), 74000C);
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
        vt.typ := integerType;
        op := GETVAR;
        id1 := curIdRec;
    };
    new(uProcPtr, 13);
    with uProcPtr@ do {
        typ.rep := NIL;
        list := NIL;
        argList := NIL;
        preDefLink := NIL;
        pos := 0;
        sigtyp.rep := NIL;
    };
    temptype.rep := NIL;
    sysProcNum := 0;
    for l3var5z := 0 to 22 do
        regSysProc(systemProcNames[l3var5z]);
    sysProcNum := 0;
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
    regSysProc(635172455746C(*"  SIZEOF" *));
    regSysProc(5746466345645746C(*"OFFSETOF"*));
    regSysProc(0C(*" was SUCC"*));
    regSysProc(0C(*" was PRED"*));
    temptype := voidPtr;
    regSysProc(554154545743C(*"  MALLOC"*));
    temptype := booleanType;
    regSysProc(455746C(*"     EOF"*));
    temptype := voidPtr;
    regSysProc(0C(*was REF, unused*));
    temptype := booleanType;
    regSysProc(45575456C(*"    EOLN"*));
    temptype := integerType;
    regSysProc(0C(*" was SETJMP"*));
    regSysProc(6257655644C(*"   ROUND"*));
    regSysProc(43416244C(*"    CARD"*));
    regSysProc(5551564554C(*"   MINEL"*));
    temptype := voidPtr;
    regSysProc(606462C(*"     PTR"*));
    l3var11z.i := 30;
    l3var11z.m := l3var11z.m * halfWord + [24,27,28,29];
    new(programObj, 13);
    outName.i := 1257656460656412C(*"*OUTPUT*"*);
    inName.i := 12515660656412C(*" *INPUT*"*);
    symTabPos := 74004B;
    with programObj@ do {
            curVal.i := 6041634357556054C; (* PASCOMPL *)
            id := ;
            pos := 0;
            symTab[74000B] := leftAlign;
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
    programObj@.sigtyp.rep := NIL;
    objBufIdx := 1;
    lookupMode := lookDef;
    outputObjFile;
    outputFile := NIL;
    inputFile := NIL;
    externFileList := NIL;
    new(l3var7z, 12);
    lineStartOffset := moduleOffset;
    with l3var7z@ do {
        id := outName;
        offset := 0;
        typ := textType;
        cl := VARID;
        list := NIL;
    };
    curVal.i := 1257656460656412C(*"*OUTPUT*"*);
    l3var7z@.value := allocExtSymbol(l3var11z.m);
    addToHashTab(l3var7z);
    l3var5z := 1;
    while SY = EXTERNSY do {
        inSymbol;
        while SY = IDENT do {
            defExtern;
            inSymbol;
            if (SY = COMMA) then
                inSymbol;
        };
        checkSymAndRead(SEMICOLON);
    }; (* while SY = EXTERNSY *)
    if (outputFile = NIL) then {
        curIdent := outName;
        defExtern;
    };
    lookupMode := lookUse;
    l3var6z := 40;
    repeat
        programme(l3var6z, programObj, false);
    until (SY = PERIOD) or (CH = '_000');
    if (CH <> 'D') then {
        lookup2 := 0;
        lookupMode := ;
    } else {
        freeRegs := halfWord;
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
        vt.typ.rep := ptr(ord(curExternFile));
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
    lookup2 := lookDef;
    l3var5z := 0;
    lookupMode := lookDef;
    inSymbol;
    l3var2z := NIL;
    if (SY = RPAREN) then {
        inSymbol;
        lookup2 := lookUse;
        lookupMode := lookUse;
        exit;
    };
    if not (SY IN [IDENT,VOIDSY]) then
        errAndSkip(errBadSymbol, (skipToSet + [IDENT,RPAREN]));
    lookup2 := lookUse;
    while (SY IN [IDENT,VOIDSY]) do {
        l3sym7z := SY;
        if (SY = IDENT) then
            parClass := VARID
        else {
            parClass := ROUTINEID;
        };
        l3var3z := NIL;
        if (SY = VOIDSY) then
            expType := voidType
        else
            expType := integerType;
        l3var6z := 0;
        if (SY <> IDENT) then {
            lookupMode := lookDef;
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
                typ := voidType;
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
            lookupMode := lookDef;
            inSymbol;
        };
        until noComma;
        if (l3sym7z <> VOIDSY) then {
            checkSymAndRead(COLON);
            parseTypeRef(expType, (skipToSet + [IDENT,RPAREN]));
            if (isFileType(expType)) then
                error(5) (*errSimpleTypeReq *)
            else if (expType.p.psize <> 1) then
                l3var5z := l3var6z * expType.p.psize + l3var5z;
            if (l3var3z <> NIL) then
                while (l3var3z <> curIdRec) do with l3var3z@ do {
                    typ := expType;
                    l3var3z := list;
                };
        };

        if (SY = SEMICOLON) then {
            lookupMode := lookDef;
            inSymbol;
            if not (SY IN (skipToSet + [IDENT,VOIDSY])) then
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
                l3var5z := l3var2z@.typ.p.psize;
                if (l3var5z <> 1) then {
                    l3var2z@.value := l3var6z;
                    l3var6z := l3var6z + l3var5z;
                }
            };
            l3var2z := l3var2z@.list;
        };
    };

    checkSymAndRead (RPAREN);
    lookup2 := lookUse;
    lookupMode := lookUse;
}; (* parseParameters *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure exitScope(var arg: hashArray);
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
procedure markTypeSym;
{
    if (SY = IDENT) then {
        curVal.m := curIdent.m * hashMask.m;
        mapAI(curVal.a, bucket);
        hashTravPtr := symHash[bucket];
        while (hashTravPtr <> NIL) and (hashTravPtr@.id <> curIdent) do
            hashTravPtr := hashTravPtr@.next;
        if (hashTravPtr <> NIL) and (hashTravPtr@.cl = TYPEID) then
            SY := TYPESY;
    };
}; (* markTypeSym *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
{ (* programme *)
    localSize := l2arg1z;
    ceRegs := halfWord + [23];
    if (localSize = 0) then {
        inSymbol;
        initScalars;
        exit;
    };
    preDefHead := ptr(0);
    inTypeDef := false;
    retSeen := ;
    hasFiles := 0;
    bodyStatSys := statBegSys;
    strLabList := NIL;
    lineNesting := lineNesting + 1;
    labFence := numLabTop;
    repeat
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
            if (charClass <> ASSIGNOP) then
                error(errBadSymbol)
            else
                inSymbol;
            with workidr@ do
                parseConstDeclValue(typ, high);
            with workidr@ do if (typ = voidType) then {
                error(errNoConstant);
                typ := integerType;
                value := 1;
            };
            if (SY = SEMICOLON) then {
                lookupMode := lookDef;
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
    if (SY = TYPEDEFSY) then {
        inTypeDef := true;
        typelist := NIL;
        parseDecls(0);
        while SY = IDENT do {
            if isDefined then
                error(errIdentAlreadyDefined);
            ii := bucket;
            l2var12z := curIdent;
            inSymbol;
            if (charClass <> ASSIGNOP) then
                error(errBadSymbol)
            else
                inSymbol;
            parseTypeRef(l2typ13z, skipToSet + [SEMICOLON]);
            curIdent := l2var12z;
            if (knownInType(curIdRec)) then {
                l2typ14z := curIdRec@.typ;
                if (l2typ14z.rep@.base = booleanType) then {
                    if (l2typ13z.p.pk <> kindPtr) then {
                        parseDecls(1);
                        error(78); (* errPredefinedAsPointer *)
                    };
                    l2typ14z.rep@.base := l2typ13z.rep@.base;
                } else {
                    l2typ14z.rep@.base := l2typ13z;
                    curIdRec@.typ := l2typ13z;
                };
                hash(typelist, curIdRec);
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
            lookupMode := lookDef;
            checkSymAndRead(SEMICOLON);
        };
        while (typelist <> NIL) do {
            l2var12z := typelist@.id;
            curIdRec := typelist;
            parseDecls(1);
            error(79); (* errNotFullyDefined *)
            typelist := typelist@.next;
        }
    }; (* TYPEDEFSY -> 22612 *)
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
                lookupMode := lookDef;
                inSymbol;
            };
            (* 22663 -> 22620 *) until done;
            checkSymAndRead(COLON);
            parseTypeRef(l2typ13z, skipToSet + [IDENT,SEMICOLON]);
            jj := l2typ13z.p.psize;
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
                if l2typ13z.p.pk = kindFile then
                    makeExtFile;
                workidr := curIdRec;
            };
            lookupMode := lookDef;
            checkSymAndRead(SEMICOLON);
            if (SY <> IDENT) and not (SY IN skipToSet) and
               not (bodyBlock and (SY IN statBegSys)) then
                errAndSkip(errBadSymbol, skipToSet + [IDENT]);
            (* A leading type-IDENT after ';' starts a new C-style
               routine decl, not another variable; bail out so the
               routine-declaration loop below can pick it up.
               lookDef leaves hashTravPtr unreliable when the name
               is absent from the current scope, so re-resolve it
               via a scope-agnostic walk over the hash bucket. *)
            hashTravPtr := NIL;
            markTypeSym;
        until (SY <> IDENT) or
              (bodyBlock and (hashTravPtr <> NIL) and
               (hashTravPtr@.cl <> TYPEID));
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
        hasFiles := moduleOffset;
        formOperator(FILEINIT);
    } else
        hasFiles := 0;
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
    markTypeSym;
    while (SY = VOIDSY) or (SY = TYPESY) do {
        done := SY = VOIDSY;
        (* For the new C-style syntax 'RETTYPE NAME(args);' the current
           TYPESY names the return type; stash it before inSymbol clobbers
           hashTravPtr.  NIL marks "not new-style" so the code that sets
           curIdRec@.typ knows which path to take. *)
        if not done then
            markTypeSym;
        if (SY = TYPESY) then
            typedRetType := hashTravPtr@.typ
        else
            typedRetType := voidType;
        if (curFrameRegTemplate = 7) then {
            error(81); (* errProcNestingTooDeep *)
        };
        lookupMode := lookDef;
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
                   ((typ = voidType) = done) then {
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
            new(curIdRec, 13);
            with curIdRec@ do {
                id := curIdent;
                offset := curFrameRegTemplate;
                next := symHash[bucket];
                typ := voidType;
                symHash[bucket] := curIdRec;
                cl := ROUTINEID;
                list := NIL;
                value := 0;
                argList := NIL;
                preDefLink := NIL;
                sigtyp := voidType;
                if (declEntry) then
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
            hadParens := SY = LPAREN;
            if hadParens then
                parseParameters;
            if not done then {
                if (typedRetType <> voidType) then {
                    (* New C-style: return type was stashed at the loop
                       head; no ':TYPE' suffix expected. *)
                    curIdRec@.typ := typedRetType;
                    if (curIdRec@.typ.p.psize <> 1) then
                        error(errTypeMustNotBeFile);
                } else if (SY <> COLON) then
                    errAndSkip(106 (*:*), skipToSet + [SEMICOLON])
                else {
                    inSymbol;
                    parseTypeRef(curIdRec@.typ, skipToSet + [SEMICOLON]);
                    if (curIdRec@.typ.p.psize <> 1) then
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
            hadParens := false;
            if (SY = LPAREN) and (curIdRec@.argList = NIL) then {
                hadParens := true;
                inSymbol;
                checkSymAndRead(RPAREN);
            };
        };
        if (SY = BEGINSY) then {
            if (curIdRec@.argList = NIL) and not hadParens then
                error(42); (* errNoParamList *)
            setup(scopeBound);
            inSymbol;
            programme(l2int18z, curIdRec, true);
% ifdef kindrout
            curIdRec@.sigtyp := makeRoutineType(curIdRec);
% endif
            myrollup(ord(scopeBound));
            exitScope(symHash);
            exitScope(fieldHash);
            goto 23301;
        };
        checkSymAndRead(SEMICOLON);
        with curIdRec@ do if (curIdent = litForward) then {
            if (isPredefined) then
                error(83); (* errRepeatedPredefinition *)
            level := l2int18z;
            preDefLink := preDefHead;
            preDefHead := curIdRec;
% ifdef kindrout
            sigtyp := makeRoutineType(curIdRec);
% endif
        } else  if (SY = EXTERNSY) or
            (curIdent = litFortran) or
            (curIdent = litAssembler) then {
            if (SY = EXTERNSY) then {
                curVal.m := [20];
            } else if (curIdent = litAssembler) then {
                curVal.m := [20,26];
            } else if (checkFortran) then {
                curVal.m := [21,24];
                checkFortran := false;
            } else {
                curVal.m := [21];
            };
            curIdRec@.flags := curIdRec@.flags + curVal.m;
% ifdef kindrout
            curIdRec@.sigtyp := makeRoutineType(curIdRec);
% endif
        } else  {
            error(errBadSymbol);
        };
        inSymbol;
        checkSymAndRead(SEMICOLON);
23301:  workidr := curIdRec@.argList;
        if (workidr <> NIL) then {
            while (workidr <> curIdRec) do {
                scopeBound := NIL;
                hash(scopeBound, workidr);
                workidr := workidr@.list;
            };
        };
        curFrameRegTemplate := curFrameRegTemplate - indexreg[1];
        curProcNesting := curProcNesting - 1;
        markTypeSym;
    };
    markTypeSym;
    if CH = '_000' then exit;
    if bodyBlock then {
        if not (SY IN (bodyStatSys + blockBegSys)) and
           not (SY IN [VOIDSY,TYPESY,ENDSY]) then
            errAndSkip(84 (* errErrorInDeclarations *),
                       skipToSet + bodyStatSys + blockBegSys + [ENDSY]);
    } else if not (SY IN blockBegSys) and not (SY IN [VOIDSY,TYPESY]) then
        errAndSkip(84 (* errErrorInDeclarations *), skipToSet);
    until (bodyBlock and ((SY IN bodyStatSys) or
                          (SY IN [VOIDSY,TYPESY,ENDSY]))) or
          (not bodyBlock and ((SY in statBegSys) or
                              (SY IN [VOIDSY,TYPESY])));
    if (preDefHead <> ptr(0)) then {
        error(85); (* errNotFullyDefinedProcedures *)
        while (preDefHead <> ptr(0)) do {
            printTextWord(preDefHead@.id);
            preDefHead := preDefHead@.preDefLink;
        };
        writeLN;
    };
    lookup2 := lookUse;
    lookupMode := lookUse;
    defineRoutine(bodyBlock);
    if (curProcNesting > 1) and
        not retSeen and (procName@.typ <> voidType) then {
        writeln(' above function must return a value');
        error(200);
    };
    done := true;
    while (numLabTop > labFence) do {
        if not (numLabs[numLabTop].defined) then {
            write(' ', numLabs[numLabTop].id.i:0, ':');
            done := false;
        };
        numLabTop := numLabTop - 1;
    };
    if not done then {
        printTextWord(procName@.id);
        error(18); (* errLabelNotDefined *)
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
    for l3var1z := ATX to MADDJ do
        insnTemp[l3var1z] := ord(l3var1z) * 10000B;
    insnTemp[ELFUN] := 500000B;
    jdx := KUTC;
    for l3var1z := UTC to VJM do {
        insnTemp[l3var1z] := jdx;
        jdx := jdx + 100000B;
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
    opToInsn[IDIVOP] := 17; (* C/DI *)
    opToInsn[IMODOP] := 11; (* C/MD *)
    opToInsn[PLUSOP] := insnTemp[ADD];
    opToInsn[MINUSOP] := insnTemp[SUB];
    opToInsn[IMULOP] := insnTemp[AMULX];
    opToInsn[SETAND] := insnTemp[AAX];
    opToInsn[SETXOR] := insnTemp[AEX];
    opToInsn[SETOR] := insnTemp[AOX];
    opToInsn[INTPLUS] := insnTemp[ADD];
    opToInsn[INTMINUS] := insnTemp[SUB];
    opToInsn[SHLEFT] := 98;
    opToInsn[SHRIGHT] := 99;
    opFlags[ANDOP] := opfAND;
    opFlags[IDIVOP] := opfDIV;
    opFlags[OROP] := opfOR;
    opFlags[IMULOP] := opfMULMSK;
    opFlags[IMODOP] := opfMOD;
    opFlags[ASSIGNOP] := opfASSN;
    opFlags[SHLEFT] := opfSHIFT;
    opFlags[SHRIGHT] := opfSHIFT;
    for jdx := 0 to 6 do {
        funcInsn[jdx] := 500000B + jdx;
    }
}; (* initInsnTemplates *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
procedure regKeywords;
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* regKeywords *)
    SY := EXPROP;
    charClass := INOP;
    regResWord(5156C(*"      IN"*));
    SY := CONSTSY;
    charClass := NOOP;
    for idx := 0 to 25 do {
        regResWord(resWordName[idx]);
        SY := succ(SY);
    }
}; (* regKeywords *)
%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
{ (* initTables *)
    FcstCnt := 0;
    FcstCount := 0;
    initInsnTemplates;
    skipToSet := blockBegSys + statBegSys - [SWITCHSY];
    bigSkipSet := skipToSet + statEndSys;
    unpack(pasinfor.a3@, iso2text, '_052'); (* '*' *)
    iso2text['_'] := iso2text['*'];
    rewrite(CHILD);
    for jdx to 10 do
        put(CHILD);
    regKeywords;
    numLabTop := 0;
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
    sizes[9] := ptr(lookup2);
    sizes[10] := ptr(lookupMode);
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
    lookupMode := lookUse;
    lookup2 := lookUse;
    moduleOffset := 16384;
    lineStartOffset := ;
    condLabCnt := 1;
    inCallArgs := false;
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
    checkBounds := not (44 in curVal.m);
    declEntry := false;
    errors := false;
    allowCompat := false;
    litForward.i := 46576267416244C;
    litFortran.i := 46576264624156C;
    litAssembler.i := 4163634555425445C;
    fileBufSize := 1;
    charEncoding := 2;
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
    programme(curInsnTemplate, hashTravPtr, false);
    if errors then {
9999:   writeln(' IN ', (lineCnt-1):0, ' LINES ',
            totalErrors:0, ' ERRORS');
    } else {
        finalize;
        PASINFOR.errors@ := false;
        writeln(' MAXHEAP = ', maxHeap:5 oct);
    }
}
.data
    frameRegTemplate := 04000000B;
    constRegTemplate := I8;
    disNormTemplate :=  KNTR+7;
    blockBegSys := [CONSTSY, TYPEDEFSY, VARSY, VOIDSY, BEGINSY];
    statBegSys :=  [IDENT, EXPROP, LPAREN, INTCONST, REALCONST,
                    CHARCONST, STRINGSY, LBRACK, BEGINSY, IFSY,
                    SWITCHSY, DOSY, WHILESY, FORSY, WITHSY, GOTOSY,
                    BREAKSY, CONTSY, SEMICOLON];
    O77777 := [33:47];
    intZero := 0;
    maxHeap := 0;
    extSymMask := (43000000C);
    halfWord := [24:47];
    hashMask := 203407C;
    statEndSys := [SEMICOLON, ENDSY, ELSESY, WHILESY];
    lvalOpSet := [GETELT, GETVAR, op37, GETFIELD, DEREF, FILEPTR];
    symHash := NIL:128;
    fieldHash := NIL:128;
    kwordHash := NIL:128;
    resWordName :=
        4357566364C             (*"   CONST"*),
        64716045444546C         (*" TYPEDEF"*),
        664162C                 (*"     VAR"*),
        0C                      (*"was FUNCTION"*),
        66575144C               (*"    VOID"*),
        45566555C               (*"    ENUM"*),
        1212604143534544C       (*"**PACKED"*),
        0C                      (*"was ARRAY"*),
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
        43416345C               (*"    CASE"*),
        44454641655464C         (*" DEFAULT"*),
        6556515756C             (*"   UNION"*);
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
    funcInsn[fnROUND] := macro + mcROUND;
    funcInsn[fnCARD] := KACX;
    funcInsn[fnMINEL] := macro + mcMINEL;
    funcInsn[fnMALLOC] := macro + mcMALLOC;
    funcInsn[fnPTR] := KAAX+MANTISSA;
    funcInsn[fnABSI] := KAMX;
    intOpMap[MUL] := IMULOP,IDIVOP;
    intOpMap[IMODOP] := IMODOP,INTPLUS,INTMINUS;
    frameRestore := 0:16;
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
    helperMap := 0:102;
%
    (* Initialize operator precedence table *)
    opPrec := precNone:48;
    opAssoc := leftAs:48;
%
    (* Conditional ternary operator - precedence 1 (lowest), right-assoc *)
    opPrec[CONDOP] := precCond;
    opAssoc[CONDOP] := rightAs;
%
    (* Logical OR operators - precedence 2 *)
    opPrec[OROP] := precOr;
%
    (* Logical AND operators - precedence 3 *)
    opPrec[ANDOP] := precAnd;
%
    (* Bitwise OR - precedence 4 *)
    opPrec[SETOR] := precBitOr;
%
    (* Bitwise XOR - precedence 5 *)
    opPrec[SETXOR] := precBitXor;
%
    (* Bitwise AND - precedence 6 *)
    opPrec[SETAND] := precBitAnd;
%
    (* Equality operators - precedence 7 *)
    opPrec[NEOP] := precEq;
    opPrec[EQOP] := precEq;
%
    (* Relational operators - precedence 8 *)
    opPrec[LTOP] := precRel;
    opPrec[GEOP] := precRel;
    opPrec[GTOP] := precRel;
    opPrec[LEOP] := precRel;
    opPrec[INOP] := precRel;
%
    (* Shift operators - precedence 9 *)
    opPrec[SHLEFT] := precShift;
    opPrec[SHRIGHT] := precShift;
%
    (* Additive operators - precedence 10 *)
    opPrec[PLUSOP] := precAdd;
    opPrec[MINUSOP] := precAdd;
%
    (* Multiplicative operators - precedence 11 (highest) *)
    opPrec[MUL] := precMul;
    opPrec[RDIVOP] := precMul;
    opPrec[IDIVOP] := precMul;
    opPrec[IMODOP] := precMul;
%
    helperNames :=
        6017210000000000C      (*"P/1     "*),
        6017220000000000C      (*"P/2     "*),
        6017230000000000C      (*"P/3     "*),
        6017240000000000C      (*"P/4     "*),
        6017250000000000C      (*"P/5     "*),
        6017260000000000C      (*"P/6     "*),
        6017434100000000C      (*"P/CA    "*),
        6017455700000000C      (*"P/EO    "*), (* fnEOF - 6 *)
        0000000000000000C      (*"P/SS obs"*),
(*10*)  6017455400000000C      (*"P/EL    "*), (* fnEOLN - 6 *)
        4317554400000000C      (*"C/MD    "*),
        6017555100000000C      (*"P/MI    "*),
        6017604100000000C      (*"P/PA    "*),
        6017655600000000C      (*"P/UN    "*),
        6017436000000000C      (*"P/CP    "*),
        6017414200000000C      (*"P/AB    "*),
        4317445100000000C      (*"C/DI    "*),
        4317624300000000C      (*"C/RC    "*),
        6017454100000000C      (*"P/EA    "*),
(*20*)  6017564100000000C      (*"P/NA    "*),
        6017424100000000C      (*"P/BA    "*),
        6017515100000000C      (*"P/II   u"*),
        6017626200000000C      (*"P/RR    "*),
        6017625100000000C      (*"P/RI    "*),
        6017214400000000C      (*"P/1D    "*),
        6017474400000000C      (*"P/GD    "*),
        4317450000000000C      (*"C/E     "*),
        4317454600000000C      (*"C/EF    "*),
        6017604600000000C      (*"P/PF    "*),
(*30*)  6017474600000000C      (*"P/GF    "*),
        6017644600000000C      (*"P/TF    "*),
        6017624600000000C      (*"P/RF    "*),
        6017566700000000C      (*"P/NW    "*),
        6017446300000000C      (*"P/DS    "*),
        6017506400000000C      (*"P/HT    "*),
        4317675100000000C      (*"C/WI    "*),
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
(*60*)  0000000000000000C      (*" unused "*),
        0000000000000000C      (*"P/PI obs"*),
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
        0C                     (*" was NEW"*),
        44516360576345C        (*"    FREE"*),
        50415464C              (*"    HALT"*),
        0C                     (*"was STOP"*),
        0C                     (*" was SETUP"*),
        0C                     (*" was ROLLUP"*),
(*10*)  6762516445C            (*"   WRITE"*),
        67625164455456C        (*" WRITELN"*),
        43645762C              (*"    CTOR"*),
        0C                     (*"  READLN"*),
        624564656256C          (*"  RETURN"*),
        0C                     (*"was LONGJMP"*),
        42456355C              (*"    BESM"*),
        0C                     (*"   MAPIA"*),
        0C                     (*"   MAPAI"*),
        604353C                (*"     PCK"*),
(*20*)  6556604353C            (*"   UNPCK"*),
        60414353C              (*"    PACK"*),
        655660414353C          (*"  UNPACK"*);
end
