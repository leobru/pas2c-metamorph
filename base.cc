/*
 * Host-native C++ port of the base.pas "Pascal to C metamorphosis" (P2C)
 * compiler, replacing the emulator-hosted base compiler. base.pas is the
 * authoritative source for semantics; work.p2c is the C-style mirror (a
 * phrasing donor only — the two mirrors diverge, e.g. work.p2c drops fnPTR
 * and adds fnEOF/fnEOLN/fnSETJMP).
 *
 * Invoked with two arguments, infile and outfile. Infile is the P2C source
 * in ASCII/UTF-8 (read as KOI-8 via unicode_to_koi8); outfile is a
 * big-endian bytestream of the object module. The module is emitted in the
 * "unpacked" form (section lengths occupy one word each); the monitor system
 * packs it before storing into the library.
 */
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <stdint.h>
#include <cmath>
#include <cstring>
#include <sstream>
#include <wctype.h>
#include <unistd.h>
#include <cassert>
#include <set>

FILE * pasinput = stdin;
unsigned char PASINPUT;
const char *outFileName = "output.obj";

const char * boilerplate = " PASCAL METAMORPH HELPER (2025) ";

const int MAXLIT = 500;
const int SYMTAB_LIMIT = 075500;
const int SYMTAB_MAX = 80;
const int OBJBUF_SIZE = 8192;    // initially 1024

const int64_t
    /* slots 0-6 were SQRT, SIN, COS, ARCTAN, ARCSIN, LN, EXP */
    fnABS =  7,  fnTRUNC = 8,  fnSIZEOF = 9,
    fnOFFSETOF = 10, /*  11        12  */  fnMALLOC = 13, /*  14  */
    fnREF   = 15, /*  16        17  */  fnROUND = 18, fnCARD = 19,
    fnMINEL = 20, fnPTR  = 21, fnABSI = 22;

const int64_t
    S3 = 0,
    S4 = 1,
    S5 = 2,
    S6 = 3,
    NoStackCheck = 5;

const int64_t
    errBooleanNeeded = 0,
    errIdentAlreadyDefined = 2,
    errNoIdent = 3,
    errNotAType = 4,
    errNoConstant = 6,
    errConstOfOtherTypeNeeded = 7,
    errTypeMustNotBeFile = 9,
    errNotDefined = 11,
    errBadSymbol = 12,
    errNeedOtherTypesOfOperands = 21,
    errWrongVarTypeBefore = 22,
    errUsingVarAfterIndexingPackedArray = 28,
    errTooManyArguments = 38,
    errNoCommaOrParenOrTooFewArgs = 41,
    errNumberTooLarge = 43,
    errVarTooComplex = 48,
    errEOFEncountered = 52,
    errFirstDigitInCharLiteralGreaterThan3 = 60;

const int64_t
    precNone = -1,   precAssign = 0,
    precCond = 1,    precOr = 2,     precAnd = 3,
    precBitOr = 4,   precBitXor = 5, precBitAnd = 6,
    precEq = 7,      precRel = 8,    precShift = 9,
    precAdd = 10,    precMul = 11;

const int64_t
    macro = 0100000000,
    mcJUMP = 2,
    mcACC2ADDR = 6,
    mcPOP = 4,
    mcPUSH = 5,
    mcMULTI = 7,
    mcADDSTK2REG = 8,
    mcADDACC2REG = 9,
    mcDUMMY = 10,
    mcROUND = 11,
    mcMALLOC = 12,
    mcMINEL = 15,
    mcPOP2ADDR = 19,
    mcCOND2INT = 20,
    mcPCKSTORE = 22;

const int64_t
    P_CP = 15,
    P_RC = 18,
    P_RR = 56,
    P_TR = 58,
    P_LDAR = 74;

const int64_t
    ASN64 = 0360100,
    ASCII0 =    04000007,
    E1 =        04000010,
    ZERO =      04000011,
    MULTMASK =  04000012,
    MANTISSA =  04000014,
    MINUS1 =    04000017,
    PLUS1 =     04000021,
    BITS15 =    04000022,
    REAL05 =    04000023,
    ALLONES =   04000024,
    MSB =       04000025,
    HEAPPTR =   04000027,

    KATX =      0000000,
    KXTS =      0030000,
    KADD =      0040000,
    KSUB =      0050000,
    KRSUB =     0060000,
    KAMX =      0070000,
    KXTA =      0100000,
    KAAX =      0110000,
    KAEX =      0120000,
    KARX =      0130000,
    KAVX =      0140000,
    KAOX =      0150000,
//  KDIV =      0160000,
    KMUL =      0170000,
    KAPX =      0200000,
    KAUX =      0210000,
    KACX =      0220000,
    KANX =      0230000,
    KYTA =      0310000,
//  KASN =      0360000,
    KNTR =      0370000,
    KATI =      0400000,
//  KSTI =      0410000,
    KITA =      0420000,
    KITS =      0430000,
    KMTJ =      0440000,
    KMADDJ =    0450000,
    KE74 =      0740000,
    KUTC =      02200000,
    KWTC =      02300000,
    KVTM =      02400000,
    KUTM =      02500000,
//  KUZA =      02600000,
//  KU1A =      02700000,
    KUJ =       03000000,
    KVJM =      03100000,
    KVZM =      03400000,
//  KV1M =      03500000,
    KVLM =      03700000,

    I7 =        034000000,      /* frame pointer */
    I8 =        040000000,      /* const pointer */
    I9 =        044000000,      /* temp register */
    I10 =       050000000,      /* temp register */
    I11 =       054000000,      /* temp register */
    I12 =       060000000,      /* temp register */
    I13 =       064000000,      /* link register */
    I14 =       070000000,      /* temp register */
    SP =        074000000;      /* stack pointer, reg 15 */

const int64_t
    maxLineLen = 130,
    lookDef = 0,
    lookUse = 1,
    lookWith = 2,
    lookField = 3,
    BACKSLASH = 035;            // char '\035' in the internal 6-bit code

enum Assoc { leftAs, rightAs };

enum Symbol {
/*0B*/  IDENT,      INTCONST,   REALCONST,  CHARCONST,
        STRINGSY,   LPAREN,     LBRACK,     EXPROP,
/*10B*/ RPAREN,     RBRACK,     COMMA,      SEMICOLON,
        PERIOD,     ARROW,      COLON,      BECOMES,
/*20B*/ BEGINSY,    ENDSY,      CONSTSY,    TYPEDEFSY,
        VARSY,      TYPESY,     ENUMSY,
/*30B*/ PACKEDSY,   STRUCTSY,   IFSY,       SWITCHSY,
        WHILESY,    FORSY,      WITHSY,     GOTOSY,
/*40B*/ ELSESY,     DOSY,
        EXTERNSY,   BREAKSY,    CONTSY,     CASESY,
/*50B*/ DEFAULTSY,  UNIONSY,    NOSY
};

enum IdClass {
        TYPEID,     ENUMID,     ROUTINEID,  VARID,
        FORMALID,   FIELDID
};

enum Insn {
/*000*/ ATX,   STX,   OP2,   XTS,   ADD,   SUB,   RSUB,  AMX,
/*010*/ XTA,   AAX,   AEX,   ARX,   AVX,   AOX,   ADIVX, AMULX,
/*020*/ APX,   AUX,   ACX,   ANX,   EADD,  ESUB,  ASX,   XTR,
/*030*/ RTE,   YTA,   OP32,  OP33,  EADDI, ESUBI, ASN,   NTR,
/*040*/ ATI,   STI,   ITA,   ITS,   MTJ,   MADDJ, ELFUN,
/*047*/ UTC,   WTC,   VTM,   UTM,   UZA,   U1A,   UJ,    VJM
};


enum Operator {
    SHLEFT,     SHRIGHT,
    SETAND,     SETXOR,     SETOR,
    MUL,        RDIVOP,     ANDOP,      IDIVOP,     IMODOP,
    PLUSOP,     MINUSOP,    OROP,       NEOP,       EQOP,
    LTOP,       GEOP,       GTOP,       LEOP,       INOP,
    IMULOP,     INTPLUS,    INTMINUS,   CONDOP,     ALTERN,
    INCROP,     DECROP,     ASSIGNOP,   GETELT,     GETVAR,
    RMWASSIGN,  op37,       GETENUM,    GETFIELD,   DEREF,
    FILEPTR,    STKLVAL,    ALNUM,      PCALL,      FCALL,
    TOREAL,     NOTOP,      INEGOP,     RNEGOP,     BITNEGOP,
    STANDPROC,  NOOP
};

enum OpGen {
    gen0,  STORE, LOAD,  FORMOP,  SETREG,
    SETREG9,  STOREAT9,  DOIT,  SETREG12,  DFLTWDTH,
    FRACWIDTH, gen11, gen12, FILEACCESS, FILEINIT,
    BRANCH, PCKUNPCK, LITINSN
};

// Flags for ops that can potentially be optimized if one operand is a constant
enum OpFlg {
    opfCOMM, opfHELP, opfAND, opfOR, opfDIV, opfMOD, opfSHIFT,
    opfMULMSK, opfASSN
};

enum Kind {
    kindVoid, kindReal, kindScalar, kindPtr,
    kindArray, kindStruct,
    kindCases, kindRoutine
};

// BESM-6 words are 48-bit sets/bitmaps.  work.p2c models these as plain int
// with C bitwise ops (|, &, ^, & ~); base.cc does the same -- there is no
// Bitset type and no Word.m.  Bit 0 is the MSB (position 47).  Bits()/BitRange()
// build masks; has()/subset() test them (P2C's `bit in set` and `a <= b` have
// no direct C++ spelling); shl48() keeps shifts inside 48 bits.
static const int64_t MASK48 = (1L<<48)-1;
static const int64_t INT41_MASK = 0x1FFFFFFFFFFL;
static const int64_t INT41_SIGN = 1L << 40;

inline int64_t Bits() { return 0; }
inline int64_t Bits(int64_t bit) { return (1L << (47-bit)) & MASK48; }
inline int64_t Bits(int64_t b1, int64_t b2) { return Bits(b1) | Bits(b2); }
inline int64_t Bits(int64_t b1, int64_t b2, int64_t b3) { return Bits(b1)|Bits(b2)|Bits(b3); }
inline int64_t Bits(int64_t b1, int64_t b2, int64_t b3, int64_t b4) { return Bits(b1,b2)|Bits(b3,b4); }
inline int64_t BitRange(int64_t b1, int64_t b2) {
    int64_t r = 0;
    for (; b1 <= b2; ++b1) r |= Bits(b1);
    return r;
}
inline bool has(int64_t s, int64_t b) { return b < 48 && ((s >> (47-b)) & 1); }
inline bool subset(int64_t a, int64_t b) { return (a & ~b) == 0; }
inline int64_t shl48(int64_t a, int x) { return (a << x) & MASK48; }
inline int64_t shr48(int64_t a, int x) { return (a >> x) & MASK48; }

typedef int64_t SetOfSYs; // set of ident .. selectsy;

struct Integer {
    int64_t val = 0;
    int64_t operator=(int64_t i);
    operator int64_t() const {
        int64_t v = val & 0x1FFFFFFFFFFL;
        if (v & (1L << 40))
            v -= (1L << 41);
        return v;
    }
};

int64_t Integer::operator=(int64_t i)
{
    val = i & 0x1FFFFFFFFFFL;
    return i;
}

struct Real {
    int64_t mantissa:41;
    unsigned exponent:7;
    void operator=(int64_t i) {
        mantissa = i  & ((1L<<48)-1); exponent = 104;
        if (mantissa == 0)
            exponent = 0;
        else
            while ((mantissa >> 39) == 0 || (mantissa >> 39) == -1) { exponent--; mantissa <<= 1; }
    }
    void operator=(Integer i) { (*this) = int64_t(i); }
    operator double() const {
        return ldexp(mantissa, exponent-104);
    }
    std::string print() const;
    void operator=(double d) {
        int exp;
        double mant = frexp(d, &exp);
        mantissa = ldexp(mant, 40);
        exponent = exp + 64;
    }
};

std::string Real::print() const
{
    std::ostringstream ostr;
    ostr << double(*this);
    return ostr.str();
}

int64_t heap[32768];
int64_t avail = 100;

void * besm6_alloc(size_t s)
{
    s = (s + 7) & ~7;
    s /= sizeof(int64_t);
    if (avail + s > 074000) {
        fprintf(stderr, "Out of memory: avail = %ld, wants %lu words\n", avail, s);
        throw std::bad_alloc();
    }
    avail += s;
    return heap + avail - s;
}

// Dynamic allocation in the compiler expects that the pointer can be represented as
// a 15-bit word offset into the memory pool. Deallocation is never used explicitly;
// instead, the heap high watermark is saved at the start of a scope and rolled down
// at its end.
struct BESM6Obj {
    void * operator new(size_t s) {
        return besm6_alloc(s);
    }

    // No-op: arena objects are never freed. Must be defined (not just
    // declared) because g++ 15 emits a call from ctor exception cleanup.
    void operator delete(void *) { }
};

template<class T> void setup(T * &p)
{
    p = reinterpret_cast<T*>(heap + avail);
}

template<typename T> void succ(T & v)
{
    v = (T)(int(v)+1);
}

void rollup(void * p)
{
    if (p < heap || p > heap + avail) {
        fprintf(stderr, "Cannot rollup from %p to %p\n", (void*)(heap + avail), p);
        exit(1);
    }
    avail = reinterpret_cast<int64_t*>(p) - heap;
    if (heap + avail != p) {
        fprintf(stderr, "Cannot rollup to unaligned pointer %p\n", p);
        exit(1);
    }
}

// We need to be able to produce NULL, which must not be equal to ptr(0).
// In the BESM-6, NIL was equal to 074000.
void * ptr(int64_t x)
{
    if (x == 074000) return NULL;
    if (x < 0 || x >= avail) {
        fprintf(stderr, "Cannot convert %ld to a pointer, avail = %ld\n", x, avail);
        exit(1);
    }
    return heap + x;
}

int64_t ord(void * p)
{
    int64_t ret = reinterpret_cast<int64_t>(p);
    if (p == NULL) return 074000;
    if (ret < avail || ret <= 100) return ret;
    // The exact heap top is a valid mark (cf. rollup): ord(heap+avail) = avail.
    if (p < heap || p > heap + avail) {
        fprintf(stderr, "Invalid pointer to integer conversion, %p is outside of valid heap range %p-%p\n",
                p, (void*)heap, (void*)(heap + avail));
        exit(1);
    }
    if (heap + (reinterpret_cast<int64_t*>(p) - heap) != p) {
        fprintf(stderr, "Unaligned pointer to integer conversion: %p\n", p);
        exit(1);
    }
    return reinterpret_cast<int64_t*>(p) - heap;
}

typedef struct Expr * ExprPtr;
typedef struct Types * TypesPtr;
typedef struct IdentRec * IdentRecPtr;
typedef struct SigRec * SigPtr;

// Compact type descriptor: one 48-bit word holding both the (arena-index)
// pointer to the Types record and the pointee's metadata, mirroring
// base.pas's `pckrep` packed record with s6 right-to-left packing:
//   rep:15  bits:6  pk:3  psize:15  pad:8   (47 bits used)
// An ordinary one-word pointer type (*T) is encoded entirely in the word,
// with no Types record allocated. g++ allocates bitfields from the LSB,
// which reproduces the right-to-left field order.
struct PckRep {
    uint64_t rep   : 15;   // arena word-index of the Types record (074000 = nil)
    uint64_t bits  : 6;
    uint64_t pk    : 3;    // Kind
    uint64_t psize : 15;
    uint64_t pad   : 8;    // multi-use
};

struct TPtr {
    // Aggregate (no user ctor: it must live in anonymous union arms).
    // TPtr() as an expression still value-initializes to all-zero.
    union {
        int64_t word;      // whole-word view; only the low 48 bits matter
        PckRep p;
    };
    bool operator==(const TPtr & x) const { return word == x.word; }
    bool operator!=(const TPtr & x) const { return word != x.word; }
    // Bridge for legacy `typ == NULL` sites: tests the record-pointer part.
    bool operator==(const void * q) const;
    bool operator!=(const void * q) const;
    Types * rep() const;   // deref the arena pointer part (defined after Types)
    void setRep(TypesPtr t);
};

struct Alfa {
    uint64_t val:48;
    unsigned char operator[](int64_t i) const { return (val >> (48-8*i)) & 0xFF; }
    void put(int64_t i, unsigned char c) {
        c ^= (*this)[i];
        val = (val ^ (uint64_t(c) << (48-8*i))) & 0xFFFFFFFFFFFFL;
    }
    // Mimics BESM-6 exactly, but is not transitive: the list of literals can have repetitions.
    bool operator<(const Alfa & x) const {
        uint64_t tmp = val + (x.val ^ 0xFFFFFFFFFFFFL);
        tmp = (tmp + (tmp >> 48)) & 0xFFFFFFFFFFFFL;
        return tmp >> 47;
    }
    // Better use
    // bool operator<(const Alfa & x) const { return val < x; }

    std::string print() const;
};

std::string Alfa::print() const
{
    std::string ret;
    for (int i = 1; i <= 6; ++i)
        ret += (*this)[i];
    return ret;
}

void unpck(unsigned char & to, Alfa & from)
{
    unsigned char * p = &to;
    for (int i = 0; i < 6; ++i) {
        p[i] = from[i+1];
    }
}

void pck(unsigned char & from, Alfa & to)
{
    unsigned char * p = &from;
    for (int i = 0; i < 6; ++i) {
        to.put(i+1, p[i]);
    }
}

struct Word {
    union {
        int64_t ii;
        Real r;
        bool b;
        Alfa a;
        TPtr typ;
    };
    // Zero at construction: bits 48-63 have no BESM-6 counterpart, and
    // bitfield writes through .ii/.a/.r never touch them. Without this,
    // stack-allocated Words carry ASLR-dependent garbage into 64-bit .ii
    // reads, making symtab/FCST dedup (and thus output) nondeterministic.
    Word() : ii(0) {}
    bool operator==(const Word & x) const;
    bool operator!=(const Word & x) const;
    std::string pt() const; 

};

bool Word::operator==(const Word &x) const { return ii == x.ii; }
bool Word::operator!=(const Word &x) const { return !(*this == x); }

typedef struct OneInsn * OneInsnPtr;

struct OneInsn : public BESM6Obj {
    OneInsnPtr next;
    int64_t mode, code, offset;
};

enum ilmode { ilCONST, ilLVAL, ilRVAL, ilCOND };
enum state {stWORD, stSLICE, stPACKED};

struct InsnList : public BESM6Obj {
    OneInsnPtr tail, head;
    TPtr typ;
    int64_t regsused;
    ilmode ilm;
    Word payload;
    int64_t disp;
    int64_t addrmd;
    state st;
    int64_t width, shift;
};

typedef InsnList * InsnListPtr;

// The type descriptor record proper: kind-specific payload ONLY — size,
// bits, and kind live in the compact TPtr word that references this record
// (base.pas `types` variant record after the compact-pointer redesign).
struct Types : public BESM6Obj {
    union {
        struct {                       // kindArray
            TPtr base;
            int64_t pck;               // boolean
            int64_t perword, pcksize, aleft, aright;
        };
        struct {                       // kindScalar
            IdentRecPtr enums;
            int64_t numen, start;
        };
        struct {                       // kindPtr
            TPtr sbase;
        };
        struct {                       // kindStruct
            TPtr variants;
            IdentRecPtr fields;
            int64_t flag, pckrec;      // booleans
        };
        struct {                       // kindCases
            TPtr first, next, alt;
        };
        struct {                       // kindRoutine
            TPtr rresult;
            SigPtr rparams;
            int64_t rargc;
            int64_t rflags;
        };
    };
    // Zero the largest arm: the arena recycles memory that held raw host
    // pointers (see IdentRec::zeroUnions), and BESM-6 heap garbage has no
    // host counterpart.
    Types() {
        base = TPtr(); pck = 0;
        perword = 0; pcksize = 0; aleft = 0; aright = 0;
    }

    std::string p() const { return "type"; } // details live in TPtr now
};

inline Types * TPtr::rep() const
{
    return p.rep == 074000 ? nullptr : reinterpret_cast<Types*>(heap + p.rep);
}

inline void TPtr::setRep(TypesPtr t)
{
    p.rep = ord(t);
}

inline bool TPtr::operator==(const void * q) const { return rep() == q; }
inline bool TPtr::operator!=(const void * q) const { return rep() != q; }

struct SigRec : public BESM6Obj {
    IdClass pclass;
    TPtr ptyp;
    SigPtr next;
    SigRec() : pclass(TYPEID), ptyp(), next(nullptr) {}
};

typedef char charmap[128];
typedef char textmap[128];

typedef int64_t four[5]; // [1..4]
typedef int64_t Entries[43]; // [1..42]

struct Expr : public BESM6Obj {
    Word vt;                    // the expression's type (a TPtr in a Word)
    Operator op;
    union {
        Word lit{};             // NOOP arm: literal value
        ExprPtr expr1;
        TPtr typ1;
        IdentRecPtr id1;
        int64_t num1;
    };
    union {
        Word lit2{};
        ExprPtr expr2;
        TPtr typ2;
        IdentRecPtr id2;
        int64_t num2;
    };
    std::string p();
};

void p(ExprPtr e) {
    fprintf(stderr, "%s\n", e->p().c_str());
}

struct KeyWord : public BESM6Obj {
    KeyWord * next;
    Word w;
    Symbol sym;
    union {
        Operator op;
        IdentRecPtr id;
    };
    KeyWord() : next(nullptr), w(), sym(NOSY) { id = nullptr; }
};

struct StrLabel : public BESM6Obj {
    StrLabel * next;
    Word ident;
    int64_t target;
};

struct NumLabel : public BESM6Obj {
    Word id;
    int64_t line, offset;
    bool defined;
};

std::string toAscii(int64_t val)
{
    std::string ret;
    for (int i = 0; i < 8; ++i) {
        int c = (val >> (42-(i*6))) & 077;
        if (c == 0) ret += ' ';
        else if (020 <= c && c <= 031) ret += char(c-020+'0');
        else if (041 <= c && c <= 072) ret += char (c-041+'A');
        else if (c == 012) ret += '*';
        else if (c == 017) ret += '/';
        else ret += '?';
    }
    return ret;
}

struct IdentRec : public BESM6Obj {
    int64_t id;
    int64_t offset;
    IdentRecPtr next;
    TPtr typ;
    IdClass cl;
    // TYPEID, VARID classes end here
    union {
        IdentRecPtr list_;      // ENUMID, FORMALID
        int64_t low_;           // ROUTINEID
        int64_t maybeUnused_;   // FIELDID
        int64_t procno_;        // legacy alias of low_ (standard procs)
    };
    union {
        int64_t value_;         // ENUMID, FORMALID
        Word high_;             // ROUTINEID (same storage as value)
        TPtr uptype_;           // FIELDID
    };
    union {
        // FIELDID
        struct {
            bool pckfield_;
            int64_t shift_, width_;
        };

        // ROUTINEID
        struct {
            IdentRecPtr argList_, preDefLink_;
            int64_t level_, pos_;
            int64_t flags_;
            TPtr sigtyp_;
        };
    };
    IdentRecPtr & list() {
        assert(cl != TYPEID);
        return list_;
    }
    int64_t & value() {
        assert (cl != TYPEID);
        return value_;
    }
    int64_t & low() {
        assert(cl == ROUTINEID);
        return low_;
    }
    Word & high() {
        assert(cl == ROUTINEID);
        return high_;
    }
    int64_t & procno() {
        assert(cl == ROUTINEID);
        return procno_;
    }
    TPtr & sigtyp() {
        assert(cl == ROUTINEID);
        return sigtyp_;
    }
    TPtr & uptype() {
        assert (cl == FIELDID);
        return uptype_;
    }
    bool & pckfield() {
        assert(cl == FIELDID);
        return pckfield_;
    }
    int64_t & shift() {
        assert(cl == FIELDID);
        return shift_;
    }
    int64_t & width() {
        assert(cl == FIELDID);
        return width_;
    }
    IdentRecPtr & argList() {
        assert(cl == ROUTINEID);
        return argList_;
    }
    IdentRecPtr & preDefLink() {
        assert(cl == ROUTINEID);
        return preDefLink_;
    }
    int64_t & level() {
        assert(cl == ROUTINEID);
        return level_;
    }
    int64_t & pos() {
        assert(cl == ROUTINEID);
        return pos_;
    }
    int64_t & flags() {
        assert(cl == ROUTINEID);
        return flags_;
    }
    
    std::string p(bool verbose = false) const {
        std::string ret;
        char * strp;
        switch (cl) {
        default: ret = toAscii(id);
            return ret.substr(ret.find_last_of(' ')+1, std::string::npos);
        case ROUTINEID:
            ret = toAscii(id);
            if (verbose) {
                if (0 <= asprintf(&strp, "(routine) procno: %ld value: %ld argl: %ld predef: %ld level: %ld pos: %ld flags: %lx",
                                  procno_, value_, ord(argList_), ord(preDefLink_), level_, pos_, flags_)) {
                    ret += strp;
                    free(strp);
                } else perror("asprintf");
            }
        }
        return ret;
    }
    IdentRec(int64_t id_, int64_t o_, IdentRecPtr n_, TPtr t_, IdClass cl_) :
        id(id_), offset(o_), next(n_), typ(t_), cl(cl_) { zeroUnions(); }
    IdentRec(int64_t id_, int64_t o_, IdentRecPtr n_, TPtr t_, IdClass cl_, IdentRecPtr l_, int64_t v_) :
        id(id_), offset(o_), next(n_), typ(t_), cl(cl_) { zeroUnions(); list_ = l_; value_ = v_; }
    IdentRec() : cl(IdClass(6)) { typ = TPtr(); zeroUnions(); }

    // The arena recycles memory that previously held raw host pointers, so
    // fields never assigned (e.g. flags_ of a formal routine parameter, read
    // by genEntry via has(flags(), 24)) would otherwise contain PIE-base-
    // dependent garbage, making generated code differ from run to run.
    // BESM-6 heap garbage has no host counterpart; zero deterministically.
    void zeroUnions() {
        list_ = nullptr;
        value_ = 0;
        argList_ = nullptr;
        preDefLink_ = nullptr;
        level_ = 0;
        pos_ = 0;
        flags_ = int64_t();
        sigtyp_ = TPtr();
    }
};
typedef IdentRecPtr HashArray[128];

struct ExtFileRec : public BESM6Obj {
    int64_t id;
    int64_t offset;
    ExtFileRec * next;
    int64_t location, line;
};

enum numberFormat { decimal, octal, fullword, hex };

// Globals

int64_t curTimes;
numberFormat numFormat;
SetOfSYs   bigSkipSet, statEndSys, blockBegSys, statBegSys,
           skipToSet, lvalOpSet;

bool   inCallArgs, bool48z, forValue;
bool   dataCheck;

int64_t jumpType, jumpTarget;

Operator charClass;
Symbol   SY, prevSY;

int64_t savedObjIdx,
        FcstCnt,
        symTabPos,
        entryPtCnt,
        fileBufSize;

ExprPtr withIter, withList;

int64_t curInsnTemplate,
        linePos,
        prevErrPos,
        errsInLine,
        moduleOffset,
        lineStartOffset,
        curFrameRegTemplate,
        curProcNesting,
        totalErrors,
        lineCnt,
        bucket,
        strLen,
        heapCallsCnt,
        heapSize,
        arithMode;

std::string stmtName;
KeyWord * keyWordHashPtr;
Kind curVarKind;
ExtFileRec * curExternFile;
char commentModeCH;
unsigned char CH, prevCH;
Word prevInsn;

int64_t debugLine,
        lineNesting,
        FcstTotal,       // FcstCountTo500 in base.pas
        objBufIdx,
        lookup2, lookupMode, condLabCnt,
        prevOpcode,
        charEncoding,
        errLine;

bool atEOL,
    checkTypes,
    isDefined, putLeft, readNext,
    errors,
    declEntry,
    rangeMismatch,
    doPMD,
    checkBounds,
    fixMult,
    bool110z,
    allowCompat,
    checkFortran;

int verbose;

IdentRecPtr outputFile,
    inputFile,
    programObj,
    hashTravPtr,
    uProcPtr;

ExtFileRec * externFileList;

TPtr baseType, typ121z;
TPtr voidType, voidPtr;
// Expression-operator tables, filled in the initialize section
// (base.pas: intOpMap[MUL] := IMULOP,IDIVOP... ; opPrec := precNone:48 ...).
Operator intOpMap[64];
int64_t opPrec[64];
TPtr BooleanType;
TPtr textType;
TPtr IntegerType;
TPtr RealType;
TPtr CharType;
TPtr charPtrType, flatMemType;
IdentRecPtr flatMemVar;
TPtr AlfaType;

TPtr arg1Type, arg2Type;

NumLabel numLabs[21];    // array [1..20] of numLabel
int64_t numLabTop;
Word curToken, curVal;
const int64_t extSymMask = 043000000L;
const int64_t halfWord = 077777777L;
const int64_t leftAddr = 077777L << 24;

int64_t leftInsn;
int64_t curIdent;
int64_t toAlloc, usedRegs, liveRegs, freeRegs, auxRegs;
Word optSflags;
int64_t litOct, litForward, litFortran, litAssembler;
ExprPtr uVarPtr, curExpr;
InsnList *  insnList;
ExtFileRec * fileForOutput, * fileForInput;
int64_t maxSmallString;

TPtr smallStringType[7]; // [2..6]
int64_t symTabCnt;

int64_t symTabArray[SYMTAB_MAX+1]; // array [1..80] of Word;
int64_t symTabIndex[SYMTAB_MAX+1];
Entries entryPtTable;
four frameRestore[7]; // array [3..6] of four;
int64_t indexreg[16];
int64_t opToInsn[48];
int64_t opToMode[48];
OpFlg opFlags[48]; // array [MUL..op44] of OpFlg;
int64_t funcInsn[24];
int64_t InsnTemp[48];

int64_t frameRegTemplate = 04000000,
        constRegTemplate = I8,
        disNormTemplate = KNTR+7;

char lineBufBase[132]; // array [1..130] of char;
int64_t errMapBase[10];
Operator chrClassTabBase[256]; // array ['_000'..'_177'] of Operator;
KeyWord * KeyWordHashTabBase[128]; // array [0..127] of @KeyWord;
Symbol charSymTabBase[256]; // array ['_000'..'_177'] of Symbol;
IdentRecPtr symHash[128]; // array [0..127] of IdentRecPtr;
IdentRecPtr fieldHash[128]; //array [0..127] of IdentRecPtr;
int64_t helperMap[100];
extern int64_t helperNames[100]; // array [1..99] of int64_t;

int64_t symTab[SYMTAB_LIMIT + 1]; // array [74000B..75500B] of int64_t;
extern int64_t systemProcNames[30];
extern int64_t resWordNameBase[21];
int64_t longSymCnt;
int64_t longSymTabBase[91];
int64_t longSyms[91]; // array [1..90] of int64_t;
Word constVals[MAXLIT+1]; // array [1..500] of Alfa;
int64_t constNums[MAXLIT+1];
int64_t objBuffer[OBJBUF_SIZE+1]; // array [1..1024] of int64_t;
char koi2text[256];
std::vector<int64_t> FCST; // file of int64_t; /* last */

std::vector<int64_t> CHILD; // file of int64_t;

struct PasInfor {
    int64_t listMode;
    int64_t startOffset;
} PASINFOR;

static const char *koi2utf[64] = {
    "ю","а","б","ц","д","е","ф","г","х","и","й","к","л","м","н","о",
    "п","я","р","с","т","у","ж","в","ь","ы","з","ш","э","щ","ч","ъ",
    "Ю","А","Б","Ц","Д","Е","Ф","Г","Х","И","Й","К","Л","М","Н","О",
    "П","Я","Р","С","Т","У","Ж","В","Ь","Ы","З","Ш","Э","Щ","Ч","Ъ",
};

std::string Expr::p()
{
    static const char * opName[] = {
        "SHLEFT","SHRIGHT","SETAND","SETXOR","SETOR","MUL","RDIVOP","ANDOP",
        "IDIVOP","IMODOP","PLUSOP","MINUSOP","OROP","NEOP","EQOP","LTOP",
        "GEOP","GTOP","LEOP","INOP","IMULOP","INTPLUS","INTMINUS","CONDOP",
        "ALTERN","INCROP","DECROP","ASSIGNOP","GETELT","GETVAR","RMWASSIGN",
        "op37","GETENUM","GETFIELD","DEREF","FILEPTR","STKLVAL","ALNUM","PCALL","FCALL",
        "TOREAL","NOTOP","INEGOP","RNEGOP","BITNEGOP","STANDPROC","NOOP"
    };
    char buf[256];
    const char * nm = (op >= 0 && op <= NOOP) ? opName[op] : "??";
    if (op < GETELT) {
        std::string a = expr1 ? expr1->p() : std::string("<nil>");
        std::string b = expr2 ? expr2->p() : std::string("<nil>");
        snprintf(buf, sizeof buf, "%s(%s, %s)", nm, a.c_str(), b.c_str());
        return buf;
    }
    if (op == GETVAR) {
        snprintf(buf, sizeof buf, "GETVAR[id=%ld off=%ld val=%ld]",
                 id1 ? id1->id : -1, id1 ? id1->offset : -1,
                 id1 ? id1->value_ : -1);
        return buf;
    }
    if (op == GETENUM) {
        snprintf(buf, sizeof buf, "GETENUM[%ld]", num1);
        return buf;
    }
    if (op == NOOP) {
        snprintf(buf, sizeof buf, "NOOP[lit=%ld]", lit.ii);
        return buf;
    }
    // unary / other: show op and recurse expr1 if plausibly an expr
    snprintf(buf, sizeof buf, "%s(...)", nm);
    return buf;
}

struct programme {
    programme(int64_t & l2arg1z, IdentRecPtr l2idr2z_, bool bodyBlock_ = false);

    IdentRecPtr procName;
    IdentRecPtr preDefHead, typelist, scopeBound, l2var4z, curIdRec, workidr;
    bool isPredefined, l2bool8z, inTypeDef;
    bool done, retSeen, hadParens, typedefPending;
    ExprPtr l2var10z;
    int64_t hasFiles;
    int64_t l2var12z;
    TPtr l2typ13z, l2typ14z, typedRetType, ceTyp;
    Word ceVal;
    int64_t ceRegs;
    SetOfSYs bodyStatSys;
    StrLabel * strLabList;

    int64_t l2int18z, ii, localSize, sizeCount, jj;
    int64_t labFence;
    static std::vector<programme *> super;
    programme();
    ~programme() {
        super.pop_back();
    }
};

std::vector<programme *> programme::super;

const char *progname;

const char * pasmitxt(int64_t errNo)
{
    switch (errNo) {
    case errBooleanNeeded: return "Boolean required";
    case errIdentAlreadyDefined: return "Identifier already defined";
    case errNoIdent: return "Missing identifier";
    case errNotAType: return "Not a type";
    case errNoConstant: return "Missing constant";
    case errConstOfOtherTypeNeeded: return "Constant of other type required";
    case errTypeMustNotBeFile: return "Type must not be a file type";
    case errNotDefined: return "Unknown identifier";
    case errBadSymbol: return "Bad symbol";
    case errNeedOtherTypesOfOperands: return "Other types of operands required";
    case errNumberTooLarge: return "Number too large";
    case errNoCommaOrParenOrTooFewArgs: return "No comma or parenthesis, or too few args";
//    errWrongVarTypeBefore = 22,
    case errUsingVarAfterIndexingPackedArray: return "Using a variable after indexing packed array";
//    errTooManyArguments = 38,
//    errVarTooComplex = 48,
//    errFirstDigitInCharLiteralGreaterThan3 = 60;
    case 1: return "No comma nor semicolon";
    case 5: return "Simple type required";
    case 16: return "Label not defined in block";
    case 23: return "Type ID instead of a variable";
    case 29: return "Index out of bounds";
    case 33: return "Illegal types for assignment";
    case 37: return "Missing INPUT file in program header";
    case 44: return "Incorrect usage of a standard procedure or a function";
    case 49: return "Too many instructions in a block";
    case 50: return "Symbol table overflow";
    case 51: return "Long symbol overflow";
    case 52: return "EOF encountered";
    case 54: return "Error in pseudo-comment";
    case 55: return "More than 16 digits in a number";
    case 61: return "Empty string";
    case 62: return "Integer needed";
    case 63: return "Bad base type for set";
    case 68: return "Using a procedure in an expression";
    case 77: return "Missing OUTPUT file in program header";
    case 79: return "Unknown identifier in type definition";
    case 81: return "Procedure nesting is too deep";
    case 82: return "Previous declaration was not FORWARD";
    case 84: return "Error in declarations";
    case 85: return "Routines left undefined";
    case 86: return "Required token not found: ";
    case 88: return "Different types of case labels and expression";
    case 89: return "integer";
    case 95: return "LPAREN";
    case 96: return "LBRACK";
    case 100: return "RPAREN";
    case 101: return "RBRACK";
    case 102: return "COMMA";
    case 103: return "SEMICOLON";
    case 104: return "PERIOD";
    case 105: return "ARROW";
    case 106: return "COLON";
    case 107: return "ASSIGN";
    case 136: return "PROGRAM";
    }
    return "Dunno";
}

void printErrMsg(int64_t errNo)
{
    putchar(' ');
    if (errNo >= 200)
        printf("Internal error %ld", errNo);
    else {
        if (errNo > 88)
            printErrMsg(86);
        else if (errNo == 20)
            errNo = (SY == IDENT)*2 + 1;
        else if (16 <= errNo && errNo <= 18)
            printf("%ld ", int64_t(curToken.ii));
        printf("%s ", pasmitxt(errNo));
        if (errNo == 17)
            printf("%ld", errLine);
        else if (errNo == 22)
            printf("%6s", stmtName.c_str());
    }
    if (errNo != 86 && errNo != 78 && errNo != 79)
        putchar('\n');
}

void printTextWord(int64_t val)
{
    const char *s = toAscii(val).c_str();
    while (*s == ' ')
        s++;
    fputs(s, stdout);
}

std::string Word::pt() const
{
    return toAscii(ii);
}


int64_t toText(const char * str) {
    int64_t ret;
    ret = 0;
    for (; *str; ++str)
        ret = ret << 6 | koi2text[*str & 0xFF];
    return ret;
}

int64_t leftAlign(int64_t val)
{
    // work.p2c: shift the packed name left until its low 6-bit slot is
    // non-empty (left-justify the identifier in the word).
    while ((val & BitRange(0, 5)) == 0)
        val = shl48(val, 6);
    return val;
}

TPtr makeStringType()
{
    TPtr res;
    int64_t size;

    if (maxSmallString >= strLen)
        return smallStringType[strLen];
    else {
        res.setRep(new Types);
        size = (strLen + 5) / 6;
        res.p.bits = 0;
        res.p.psize = size;
        if (size == 1)
            res.p.bits = strLen * 8;
        res.p.pk = kindArray;
        Types & r = *res.rep();
        r.base = CharType;
        r.pck = true;
        r.perword = 6;
        r.pcksize = 8;
        r.aleft = 1;
        r.aright = strLen;
        return res;
    }
}

/* An ordinary pointer type encoded wholly in the tptr word: rep, psize
 * and bits carry the ultimate non-pointer base, pad packs depth*8 plus
 * the base kind.  Base kind 0 is never encoded (pointer-to-void is the
 * voidPtr singleton), so textType (pk=kindPtr, pad=8) is not mistaken
 * for a compact pointer. */
bool isCompactP(TPtr t)
{
    return t.p.pk == kindPtr and (t.p.pad & 7) != 0;
} /* isCompactP */

int64_t typeBits(TPtr typtr)
{
    if (isCompactP(typtr))
        return 15;
    return typtr.p.bits;
} /* typeBits */

int64_t typeSize(TPtr typtr)
{
    if (isCompactP(typtr))
        return 1;
    return typtr.p.psize;
} /* typeSize */

/* Pointee of a pointer type: compact words reconstruct it in place,
 * legacy allocated descriptors read the heap record. */
TPtr ptrBase(TPtr t)
{
    TPtr b;
    if (not isCompactP(t))
        return t.rep()->base;
    b = t;
    if (t.p.pad >= 020) {
        b.p.pad = t.p.pad - 010;
    } else {
        b.p.pk = t.p.pad & 7;
        b.p.pad = 0;
    }
    return b;
} /* ptrBase */

bool isCharPtr(TPtr arg)
{
    return arg.p.pk == kindPtr and typeSize(arg) == 1 and
           ptrBase(arg) == CharType;
} /* isCharPtr */

ExprPtr mkExpr(Operator oper, TPtr resTyp, ExprPtr e1, ExprPtr e2)
{
    ExprPtr n;
    n = new Expr;
    n->vt.typ = resTyp;
    n->op = oper;
    n->expr1 = e1;
    n->expr2 = e2;
    return n;
} /* mkExpr */

ExprPtr mkIntLit(int64_t val)
{
    ExprPtr n;
    n = new Expr;
    // n@ := [integerType, GETENUM, val]
    n->vt.typ = IntegerType;
    n->op = GETENUM;
    n->num1 = val;
    return n;
} /* mkIntLit */

ExprPtr flatMemAt(ExprPtr idx)
{
    idx->vt.typ = IntegerType;
    return mkExpr(GETELT, CharType,
                  mkExpr(GETVAR, flatMemType, (ExprPtr)flatMemVar, NULL), idx);
} /* flatMemAt */

ExprPtr mkCastInt(ExprPtr e)
{
    ExprPtr n;
    n = new Expr;
    *n = *e;
    n->vt.typ = IntegerType;
    return n;
} /* mkCastInt */

ExprPtr mkRef(ExprPtr lval)
{
    ExprPtr ret;
    ret = mkExpr(STANDPROC, voidPtr, lval, NULL);
    ret->num2 = fnREF;
    return ret;
} /* mkRef */

ExprPtr cpDsLval(ExprPtr e)
{
    if (e != NULL and e->op == DEREF and
        isCharPtr(e->expr1->vt.typ))
        return flatMemAt(e->expr1);
    else
        return e;
} /* cpDsLval */

ExprPtr cpDsExpr(ExprPtr e)
{
    if (e == NULL)
        return NULL;
    else if (e->op == DEREF and isCharPtr(e->expr1->vt.typ))
        return flatMemAt(e->expr1);
    else
        return e;
} /* cpDsExpr */

TPtr allocPtr(TPtr toType)
{
    /* Heap-allocated pointer descriptor: needed for typedef forward
     * placeholders (patched in place) and for bases the compact form
     * cannot carry (nonzero pad, e.g. textType; depth overflow). */
    TPtr t{};
    t.setRep(new Types);
    t.rep()->base = toType;
    t.p.psize = 1;
    t.p.bits = 15;
    t.p.pk = kindPtr;
    return t;
}

TPtr getPtrType(TPtr toType)
{
    TPtr t{};
    if (toType == voidType)
        return voidPtr;
    if (isCompactP(toType)) {
        t = toType;
        if (t.p.pad < 0370) {
            t.p.pad = t.p.pad + 010;
            return t;
        }
    } else if (toType.p.pad == 0) {
        t = toType;
        t.p.pad = 010 + toType.p.pk;
        t.p.pk = kindPtr;
        return t;
    }
    return allocPtr(toType);
}

ExprPtr bldIncDec(ExprPtr lval, bool isInc, bool isPost)
{
    ExprPtr one, rmw;
    Operator op1, op2;
    if (isInc) { op1 = INTPLUS; op2 = INTMINUS; }
    else       { op2 = INTPLUS; op1 = INTMINUS; }
    one = mkIntLit(1);
    rmw = mkExpr(RMWASSIGN, IntegerType, lval,
                 mkExpr(op1, IntegerType, one, NULL));
    if (not isPost) {
        return rmw;
    }
    return mkExpr(op2, IntegerType, rmw, one);
} /* bldIncDec */

void addToHashTab(IdentRecPtr arg)
{
    int bucket = (arg->id % 65535) % 128;
    arg->next = symHash[bucket];
    symHash[bucket] = arg;
}

void error(int64_t errNo);

void storeObjWord(int64_t insn)
{
    objBuffer[objBufIdx] = insn;
    moduleOffset = moduleOffset + 1;
    if (objBufIdx == OBJBUF_SIZE) {
        error(49); /* errTooManyInsnsInBlock */
        objBufIdx = 1;
    } else
        objBufIdx = objBufIdx + 1;
}

void form1Insn(int64_t arg)
{
    Word Insn, opcode;
    int64_t pos;
    Insn.ii = arg;
    opcode.ii = Insn.ii & ~077777;
    if (opcode.ii == InsnTemp[UJ]) {
        if (prevOpcode == opcode.ii)
            // No need for a jump after jump.
            return;
        if (putLeft and (prevOpcode == 1)) {
            pos = objBufIdx - 1;
            if (((objBuffer[pos] >> 24) & ~077777) == I13+KVJM) {
                // Chaining the call and the jump.
                int64_t addr1, addr2;
                prevOpcode = opcode.ii;
                addr1 = Insn.ii & 077777;
                addr2 = (objBuffer[pos] >> 24) & 077777;
                objBuffer[pos] = (I13+KVTM+addr1) << 24 | (KUJ+addr2);
                return;
            }
        }
    } else if (prevOpcode != -1 && Insn.ii % 4096 != 0 &&
               (Insn.ii ^ prevInsn.ii) == Bits(32)) /* maybe ATX/XTA */ {
// Load after store; if the load reg/off is the same as the store,
// and the store was not a stack push, there is no need to so the read.
        if ((prevInsn.ii != 074000000) /* not 15,ATX, */ &&
            (prevInsn.ii & (Bits(28)|BitRange(30,35))) == Bits() /* but still ATX */) 
            return; /* skip the XTA */
    }
    prevOpcode = opcode.ii;
    prevInsn = Insn;
    if (putLeft) {
        leftInsn = (Insn.ii & halfWord) << 24;
        putLeft = false;
    } else {
        putLeft = true;
        storeObjWord(leftInsn | (Insn.ii & halfWord));
    }
}

void form2Insn(int64_t i1, int64_t i2)
{
    form1Insn(i1);
    form1Insn(i2);
}

void form3Insn(int64_t i1, int64_t i2, int64_t i3)
{
    form2Insn(i1, i2);
    form1Insn(i3);
}

void disableNorm()
{
    if (arithMode != 1) {
        form1Insn(disNormTemplate);
        arithMode = 1;
    }
}

int64_t getObjBufIdxPlus()
{
    if (putLeft)
        return objBufIdx + 4096;
    else
        return objBufIdx;
}

void formJump(int64_t & arg)
{
    int64_t pos;
    bool isLeft;
    if (prevOpcode != InsnTemp[UJ]) {
        pos = getObjBufIdxPlus();
        isLeft = putLeft;
        form1Insn(jumpType + arg);
        if (putLeft == isLeft)
            pos = pos - 1;
        arg = pos;
    }
}

void padToLeft()
{
    if (not putLeft)
        form1Insn(InsnTemp[UTC]);
    prevOpcode = -1;
}

void formAndAlign(int64_t arg)
{
    form1Insn(arg);
    padToLeft();
    prevOpcode = 1;
}

void putToSymTab(int64_t arg)
{
    symTab[symTabPos] = arg;
    if (symTabPos == SYMTAB_LIMIT) {
        error(50); /* errSymbolTableOverflow */
        symTabPos = 074000;
    } else
        symTabPos = symTabPos + 1;
}

//
// Allocate external symbol: name in curVal.
//
int64_t allocExtSymbol(int64_t newSym)
{
    int64_t ret = symTabPos;

    curVal.ii &= 0xFFFFFFFFFFFFL; // 48-bit word; see allocSymtab
    if (curVal.ii & halfWord) {
        int64_t i;
        for (i = 1; i <= longSymCnt; ++i) {
            if (curVal.ii == longSyms[i]) {
                return longSymTabBase[i];
            }
        }
        longSymCnt++;
        if (longSymCnt >= 90) {
            error(51); /* errLongSymbolOverflow */
            longSymCnt = 1;
        };
        longSymTabBase[longSymCnt] = symTabPos;
        longSyms[longSymCnt] = curVal.ii;
        newSym |= 020000000;
    } else {
        newSym |= curVal.ii;
    }
    putToSymTab(newSym);
    return ret;
}

int64_t getHelperProc(int64_t l3arg1z)
{
    if (helperMap[l3arg1z] == 0)  {
        curVal.ii = helperNames[l3arg1z];
        helperMap[l3arg1z] = allocExtSymbol(extSymMask);
    };
    return helperMap[l3arg1z] + (KVJM+I13);
}

void toFCST()
{
    curVal.ii &= 0xFFFFFFFFFFFFL; // 48-bit word; see allocSymtab
    FCST.push_back(curVal.ii);
    FcstCnt = FcstCnt + 1;
}

int64_t addCurValToFCST()
{
    int64_t ret;
    int64_t low, high, mid;
    low = 1;
    static std::set<int64_t> lits;
    if (FcstTotal == 0) {
        ret = FcstCnt;
        FcstTotal = 1;
        constVals[1] = curVal;
        constNums[1] = FcstCnt;
        toFCST();
        lits.insert(curVal.ii);
    } else {
        high = FcstTotal;
        do {
            mid = (low + high) / 2;
            if (curVal.ii == constVals[mid].ii) {
              return constNums[mid];
            }
            if (curVal.a < constVals[mid].a)
                high = mid - 1;
            else
                low = mid + 1;
        } while (low <= high);
        ret = FcstCnt;
        if (FcstTotal != MAXLIT) {
            if (curVal.a < constVals[mid].a)
                high = mid;
            else
                high = mid + 1;
            for (mid = FcstTotal; mid >= high; --mid) {
                low = mid + 1;
                constVals[low] = constVals[mid];
                constNums[low] = constNums[mid];
            }
            FcstTotal = FcstTotal + 1;
            constVals[high] = curVal;
            constNums[high] = FcstCnt;
            lits.insert(curVal.ii);
            if (int64_t(lits.size()) != FcstTotal)
                fprintf(stderr, "Literal divergence: %d\n", int(FcstTotal - lits.size()));
        };
        toFCST();
    }
    return ret;
}

int64_t allocSymtab(int64_t newSym)
{
    int64_t ret = symTabPos;

    // BESM-6 words are 48 bits; bits 48-63 of a Word can hold stack garbage
    // (bitfield writes through .ii never touch them). Mask so value-based
    // dedup below cannot depend on them (they are never emitted either).
    newSym &= 0xFFFFFFFFFFFFL;

    if (symTabCnt == 0) {
        symTabCnt = 1;
        symTabArray[1] = newSym;
        symTabIndex[1] = symTabPos;
    } else {
        int64_t low = 1;
        int64_t high = symTabCnt;
        int64_t mid;

        do {
            mid = (low + high) / 2;
            if (newSym == symTabArray[mid]) {
                return symTabIndex[mid];
            }
            if (newSym < symTabArray[mid])
                high = mid - 1;
            else
                low = mid + 1;
        } while (high >= low);

        if (symTabCnt != SYMTAB_MAX) {
            if (newSym < symTabArray[mid])
                high = mid;
            else
                high = mid + 1;
            for (mid = symTabCnt; mid >= high; --mid) {
                low = mid + 1;
                symTabArray[low] = symTabArray[mid];
                symTabIndex[low] = symTabIndex[mid];
            }
            symTabCnt = symTabCnt + 1;
            symTabArray[high] = newSym;
            symTabIndex[high] = symTabPos;
        }
    }
    putToSymTab(newSym);
    return ret;
}

int64_t getFCSToffset()
{
    int64_t ret;
    int64_t offset;
    ret = addCurValToFCST();
    offset = ret;
    if (offset < 2048) {
        /* empty */
    } else if (offset >= 4096)
        error(204);
    else {
        ret = allocSymtab(offset + 040000000) - 070000;
    }
    return ret;
}

int64_t minel(int64_t b)
{
    if (!b) return -1;
    int64_t ret = 0;
    uint64_t t = b;
    while (((t >> 47) & 1) == 0) {
        ret++;
        t <<= 1;
    }
    return ret;
}

int64_t card(int64_t b)
{
    int64_t val = b, ret = 0;
    while (val) {
        ++ret;
        val &= val-1;
    }
    return ret;
}

std::string bset(int64_t t)
{
    std::ostringstream ostr;
    ostr <<'[';
    int64_t start = minel(t);
    int64_t prev = start;
    t = t & ~ Bits(start);
    while (t != Bits()) {
        int64_t m = minel(t);
        if (m != prev + 1) {
            if (ostr.str().size() != 1) ostr << ',';
            ostr << start;
            if (start != prev)
                ostr << (prev-start == 1 ? "," : "..") << prev;
            start = m;
        }
        prev = m;
        t = t & ~ Bits(m);
    }
    if (ostr.str().size() != 1) ostr << ',';
    if (start >= 0) {
        ostr << start;
        if (start != prev)
            ostr << (prev-start == 1 ? "," : "..") << prev;
    }
    ostr << ']';
    return ostr.str();
}

int64_t nrOfBits(Integer value)
{
    curVal.ii = value;
    curVal.ii = curVal.ii & BitRange(7, 47);
    return 48-minel(curVal.ii);
}

int64_t nrOfBits(int64_t value)
{
    int64_t b;
    b = value & ((1L<<48)-1);
    return 48-minel(b);
}

TPtr mkIntScl(int64_t bitWid)
{
    TPtr res{};
    if (bitWid < 1 or 40 < bitWid) {
        error(errNumberTooLarge);
        return IntegerType;
    }
    res.setRep(new Types);
    res.rep()->start = -1;
    res.rep()->enums = NULL;
    res.rep()->numen = 1L << bitWid;
    res.p.psize = 1;
    res.p.bits = bitWid;
    res.p.pk = kindScalar;
    return res;
} /* mkIntScl */

int64_t getValueOrAllocSymtab(int64_t value)
{
    curVal.ii = value;
    curVal.ii = curVal.ii & 077777;
    if (040000 >= curVal.ii)
        return curVal.ii;
    else
        return
            allocSymtab((curVal.ii | 040000000) & halfWord);
}

void fixup(int64_t mode, int64_t arg)
{
    int64_t work, offset;
    if (mode == 0) {
        int64_t addr, insn, leftHalf;
        bool isLeft;
        padToLeft();
        curVal.ii = moduleOffset;
L1:     addr = curVal.ii & 077777;
        leftHalf = (curVal.ii & halfWord) << 24;
        while (arg != 0) {
            if (4096 < arg)  {
                isLeft = true;
                arg = arg - 4096;
            } else isLeft = false;
            insn = objBuffer[arg];
            if (isLeft) {
                curVal.ii = insn & leftAddr;
                curVal.ii = curVal.ii >> 24;
                insn = (insn & ~leftAddr) | leftHalf;
            } else {
                curVal.ii = insn & 077777;
                insn = (insn & ~077777L) | addr;
            };
            objBuffer[arg] = insn;
            arg = curVal.ii;
        };
        return;
    } else if (mode == 2) {
        form1Insn(KVTM+I14 + curVal.ii);
        if (curVal.ii == 074001)
            form1Insn(KUTM+I14 + FcstCnt);
        form3Insn(KITA+14, InsnTemp[ASN] + arg, KAOX+I7+1);
        form1Insn(KATX+I7+1);
        return;
    } else if (mode < -2) {
        arg = arg - curVal.ii;
        offset = getFCSToffset();
        work = -mode;
        curVal.ii = arg & 0x1FFFFFFFFFFL;
        arg = getFCSToffset();
        form3Insn(KATX+SP+1, KSUB+I8 + offset, work);
        form3Insn(KRSUB+I8 + arg, work, KXTA+SP+1);
        return;
    } else if (mode == -1) {
        form1Insn(KVTM+I14 + lineCnt);
        formAndAlign(getHelperProc(arg));
        return;
    };
    curVal.ii = mode;
    goto L1;
} /* fixup */

void prInsn(int insn)
{
    if ((insn >> 19) & 1)
        printf("%02o %02o %05o", insn >> 20, (insn >> 15) & 037, insn & 077777);
    else
        printf("%02o %03o %04o", insn >> 20, (insn >> 12) & 0177, insn & 07777);
}

void OBPROG(int64_t & start, int64_t & fin)
{
    for (int64_t * p = &start; p <= &fin; ++p) {
        if (p != &start && (p - &start) % 4 == 0) putchar('\n');
        prInsn(*p >> 24); putchar(' '); prInsn(*p & 0xFFFFFF); printf("     ");
    }
    putchar('\n');
}

//
// Encode the symbol from KOI-8 to UTF-8, and output to stdout.
//
static void kputc(uint8_t c)
{
    if (c >= 0300) {
        fputs(koi2utf[c - 0300], stdout);
        return;
    }
    if (c < 040) {
        static const char *extra2utf[32] = {
            0,  0,  0,  0,  0,  0,  "×",0,  0,  0,  0,  0,  0,  0,  "≤","≥",
            0,  0,  0,  0,  0,  0,  0,  "≡","#",0,  "÷",0,  0,  0,  "∨","~",
        };
        const char *u = extra2utf[c];
        if (u) {
            fputs(u, stdout);
            return;
        }
    }
    putchar(c);
}

void endOfLine()
{
    int64_t err, errPos, prevPos, listMode,
    startPos, lastErr;

    listMode = PASINFOR.listMode;
    if ((listMode != 0) or (errsInLine != 0)) {
        printf(" %05lo%5ld%3ld%c", (lineStartOffset + PASINFOR.startOffset),
               lineCnt, lineNesting, commentModeCH);
        startPos = 12;
        if (has(optSflags.ii, S4)
            and (maxLineLen == 72)
            and (linePos >= 80)) {
            for (err = 73; err <= 80; ++err)
                putchar(lineBufBase[err]);
            putchar(' ');
            linePos = 73;
            startPos += 9;
        }; /* 1106 */
        do
            linePos = linePos-1;
        while ((lineBufBase[linePos]  == ' ') and (linePos != 0));
        for (err = 1; err <= linePos; ++err) {
            kputc(lineBufBase[err]);
        };
        putchar('\n');
        if (errsInLine != 0)  {
            printf("%*s %*c0", int(startPos), "^^^^^", int(errMapBase[0]), ' ');
            lastErr = errsInLine - 1;
            for (err = 1; err <= lastErr; ++err) {
                errPos = errMapBase[err];
                prevPos = errMapBase[err-1];
                if (errPos != prevPos) {
                    if (prevPos + 1 != errPos)
                        printf("%*c", int(errPos-prevPos-1), ' ');
                    putchar(char(err + 48));
                }
            }
            putchar('\n');
            errsInLine = 0;
            prevErrPos = 0;
        }
    } /* 1160 */
    if ((listMode == 2) and (moduleOffset != lineStartOffset)) {
        OBPROG(objBuffer[objBufIdx - moduleOffset + lineStartOffset],
               objBuffer[objBufIdx-1]);
    } /* 1174 */
    lineStartOffset = moduleOffset;
    linePos = 0;
    lineCnt = lineCnt + 1;
    // One EOF is OK when the file doesn't have any extra characters after "END."
    static int eofs;
    if (feof(pasinput) && eofs++) {
        error(errEOFEncountered);
        throw 9999;
    }
} /* endOfLine */

void requiredSymErr(Symbol sym)
{
    if (linePos != prevErrPos)
        error(sym + 88);
} /* requiredSymErr */

static unsigned char
unicode_to_koi8(int val)
{
    static std::map<int, unsigned char> uni2koi8;
    if (uni2koi8.empty()) {
        static wchar_t cyr[] = L"юабцдефгхийклмнопярстужвьызшэщчъ"
                               L"ЮАБЦДЕФГХИЙКЛМНОПЯРСТУЖВЬЫЗШЭЩЧЪ";
        for (int i = 0; cyr[i]; ++i)
            uni2koi8[cyr[i]] = (unsigned char)(i + 0300);
        uni2koi8[L'×'] = 6;
        uni2koi8[L'#'] = uni2koi8[L'≠'] = 030;
        uni2koi8[L'≤'] = 016;
        uni2koi8[L'≥'] = 017;
        uni2koi8[L'≡'] = 027;
        uni2koi8[L'\\'] = 035;   // BACKSLASH (base.pas BACKSLASH = '\035'):
                                 // the '\NNN' / '\<letter>' escape introducer.
        uni2koi8[L'÷'] = 032;
        uni2koi8[L'∨'] = 036;
        uni2koi8[L'~'] = 037;
    }
    if (uni2koi8.count(val))
        return uni2koi8[val];
    else if (val < 0177)
        return (unsigned char)val;
    else return ' ';
}

static int utf8_getc(FILE *fin)
{
    int c1, c2, c3;
    c1 = getc (fin);
    if (c1 < 0 || ! (c1 & 0x80))
        return c1;
    c2 = getc (fin);
    if (! (c1 & 0x20))
        return (c1 & 0x1f) << 6 | (c2 & 0x3f);
    c3 = getc (fin);
    return (c1 & 0x0f) << 12 | (c2 & 0x3f) << 6 | (c3 & 0x3f);

}

static unsigned char ugetc(FILE * fin)
{
    int c = utf8_getc(fin);
    // At EOF base.pas sees CH = '_000' (NUL); the programme/initScalars loops
    // use `CH == 0` as the end-of-input sentinel. unicode_to_koi8(-1) would
    // otherwise yield 0377, so the sentinel would never fire.
    if (c < 0)
        return 0;
    return unicode_to_koi8(c);
}

void readToPos80()
{
    // base.pas readToPos80: fill lineBuf to column 81, no endOfLine (adding one
    // here trips the second-EOF guard on the final flush). ugetc yields NUL
    // past EOF, matching base.pas's PASINPUT@ = '_000'.
    while (linePos < 81) {
        linePos = linePos + 1;
        lineBufBase[linePos] = PASINPUT;
        if (linePos != 81) PASINPUT = ugetc(pasinput);
    }
}

struct inSymbol {
    unsigned char localBuf[131];
    int64_t tokenLen, tokenIdx;
    bool expSign;
    IdentRecPtr l3var135z;
    Real expMultiple, expValue;
    char curChar;
    int64_t numstr[17];
    int64_t expLiteral;
    int64_t expMagnitude;
    int64_t l3int162z;
    int64_t chord;
    int64_t l3var164z;
    inSymbol();
};

void nextCH()
{
    do {
        atEOL = PASINPUT == '\n' || feof(pasinput);
        CH = PASINPUT;
        PASINPUT = ugetc(pasinput);
        linePos = linePos + 1;
        lineBufBase[linePos] = CH;
    } while (not ((maxLineLen >= linePos) or atEOL));
} /* nextCH */

struct parseComment {
    // non-recursive, no need for a super stack
    static parseComment * super;
    bool badOpt, flag;
    char c;
    parseComment();
};
parseComment * parseComment::super;

int64_t readOptVal(int64_t limit)
{
    nextCH();
    int64_t res = 0;
    while (('9' >= CH) and (CH >= '0')) {
        res = 10 * res + CH - '0';
        nextCH();
        parseComment::super->badOpt = false;
    }
    if (limit < res) parseComment::super->badOpt = true;
    return res;
}

void readOptFlag(bool & res)
{
    nextCH();
    if ((CH == '-') or (CH == '+')) {
        res = CH == '+';
        parseComment::super->badOpt = false;
    }
    nextCH();
}

parseComment::parseComment()
{
    super = this;
    nextCH();
    if (CH == '=') {
        do {
            nextCH();
            badOpt = true;
            switch (CH) {
            case 'D': case 'd': {
                curVal.ii = readOptVal(15);
                optSflags.ii = (optSflags.ii & BitRange(0, 40)) | (curVal.ii & BitRange(41, 47));
            } break;
            case 'Y': case 'y':
                readOptFlag(allowCompat);
                break;
            case 'E': case 'e':
                readOptFlag(declEntry);
                break;
            case 'S': case 's': {
                curVal.ii = readOptVal(8);
                if (curVal.ii == 3)
                    lineCnt = 1;
                else if (4 <= curVal.ii && curVal.ii <= 8)
                    optSflags.ii = optSflags.ii | Bits(curVal.ii - 3);
            } break;
            case 'F': case 'f':
                readOptFlag(checkFortran);
                break;
            case 'L': case 'l':
                PASINFOR.listMode = readOptVal(3);
                break;
            case 'P': case 'p':
                readOptFlag(doPMD);
                break;
            case 'T': case 't':
                readOptFlag(checkBounds);
                break;
            case 'A': case 'a':
                charEncoding = readOptVal(3);
                break;
            case 'C': case 'c':
                readOptFlag(checkTypes);
                break;
            case 'M': case 'm':
                readOptFlag(fixMult);
                break;
            case 'B': case 'b':
                fileBufSize = readOptVal(4);
                break;
            case 'K': case 'k':
                heapSize = readOptVal(23);
                break;
            }
            if (badOpt)
                error(54); /* errErrorInPseudoComment */
        } while (CH == ',');
    }; /* 1446 */
    do {
        while (CH != '*') {
            c = commentModeCH;
            commentModeCH = '*';
            if (atEOL)
                endOfLine();
            nextCH();
            commentModeCH = c;
        };
        nextCH();
    } while (CH != '/');
    nextCH();
} /* parseComment */

unsigned char koi8_to_koi7(unsigned char ch)
{
    if (ch >= 0300)
        return (ch & 0177) | 040;
    if (ch >= 0200)
        return ' ';
    // work.p2c/base.pas KOI-7 literal mode distinguishes ASCII '^' and '|':
    // '^' becomes 0134, while '|' becomes the OR/caret glyph 0136.
    if (ch == '^')
        return 0134;
    if (ch == '|')
        return 0136;
    if (ch >= 0140)
        ch ^= 040;
    return ch;
}

bool skipSp()
{
    while ((CH == ' ') or ((CH == 011) and not atEOL))
        nextCH();
    // At true EOF ugetc yields the NUL sentinel (base.pas: CH = '_000').
    // base.cc's atEOL is feof-sticky (unlike base.pas's per-line eoln), so
    // without this guard skipSp would keep calling endOfLine past EOF and trip
    // the second-EOF error. Stop here so the parser sees CH == 0 and unwinds.
    if (CH == 0)
        return false;
    if (atEOL) {
        endOfLine();
        nextCH();
        return true;
    } else
        return false;
}

inSymbol::inSymbol()
{
{
        if (dataCheck) {
            error(errEOFEncountered);
            readToPos80();
            throw 9999;
        }
L1473:
        while (skipSp()) ;
        hashTravPtr = NULL;
        SY = charSymTabBase[CH];
        charClass = chrClassTabBase[CH];
//      lexer:
        switch (SY) {
            case IDENT: {
                curToken.ii = 0;
                tokenLen = 1;
                do {
                    curVal.ii = koi2text[CH];
                    nextCH();
                    if (8 >= tokenLen) {
                        tokenLen = tokenLen + 1;
                        curToken.ii = shl48(curToken.ii, 6);
                        curToken.ii = curToken.ii | curVal.ii;
                    }
                } while (chrClassTabBase[CH] == ALNUM);
                bucket = curToken.ii % 65535 % 128;
                curIdent = curToken.ii;
                keyWordHashPtr = KeyWordHashTabBase[bucket];
                while (keyWordHashPtr != NULL) {
                    if (keyWordHashPtr->w.ii == curToken.ii) {
                        SY = keyWordHashPtr->sym;
                        charClass = keyWordHashPtr->op;
                        goto exitLexer;
                    }
                    keyWordHashPtr = keyWordHashPtr->next;
                }
                isDefined = false;
                SY = IDENT;
                switch (lookupMode) {
                case 0: {
                    hashTravPtr = symHash[bucket];
                    while (hashTravPtr != NULL) {
                        if (hashTravPtr->offset == curFrameRegTemplate)
                        {
                            if (hashTravPtr->id != curIdent)
                                hashTravPtr = hashTravPtr->next;
                            else {
                                isDefined = true;
                                goto exitLexer;
                            }
                        } else
                            goto exitLexer;
                    }
                } break;
                case 1: {
L2:                 hashTravPtr = symHash[bucket];
                    while (hashTravPtr != NULL) {
                        if (hashTravPtr->id != curIdent)
                            hashTravPtr = hashTravPtr->next;
                        else {
                            if (hashTravPtr->cl == TYPEID)
                                SY = TYPESY;
                            goto exitLexer;
                        }
                    }
                } break;
                case 2: {
                    if (withList == NULL)
                        goto L2;
                    withIter = withList;
                    l3var135z = fieldHash[bucket];
                    if (l3var135z != NULL) {
                        while (withIter != NULL) {
                            hashTravPtr = l3var135z;
                            while (hashTravPtr != NULL) {
                                if ((hashTravPtr->id == curIdent)
                                    and (hashTravPtr->uptype() == withIter->expr2->vt.typ))
                                    goto exitLexer;
                                hashTravPtr = hashTravPtr->next;
                            }
                            withIter = withIter->expr1;
                        }
                    }
                    goto L2;
                } break;
                case 3:
                    hashTravPtr = fieldHash[bucket];
                    while (hashTravPtr != NULL) {
                        if ((hashTravPtr->id == curIdent) and
                            (typ121z == hashTravPtr->uptype()))
                            goto exitLexer;
                        hashTravPtr = hashTravPtr->next;
                    }
                    break;
                }
                goto exitLexer;
            } break; /* IDENT */
            case INTCONST: { /*=m-*/
                SY = INTCONST;
                tokenLen = 0;
                do {
                    tokenLen = tokenLen + 1;
                    if (tokenLen <= 17)
                        numstr[tokenLen] = CH - '0';
                    else {
                        error(55); /* errMoreThan16DigitsInNumber */
                        tokenLen = 1;
                    }
                    nextCH();
                } while (charSymTabBase[CH] == INTCONST);
                { /* octdec */
                    if ((numstr[1] == 0) and (CH != '.')) {
                        if ((tokenLen == 1) and (CH == 'X' || CH == 'x')) {
                            // Hex literal: 0Xhhh[U]
                            numFormat = hex;
                            nextCH();
                            curToken.ii = 0;
                            while ((charSymTabBase[CH] == INTCONST)
                                   or (('A' <= CH) and (CH <= 'F'))
                                   or (('a' <= CH) and (CH <= 'f'))) {
                                curToken.ii = shl48(curToken.ii, 4);
                                if (charSymTabBase[CH] == INTCONST)
                                    curVal.ii = CH - '0';
                                else if ('A' <= CH and CH <= 'F')
                                    curVal.ii = CH - 55;
                                else
                                    curVal.ii = CH - 87;
                                curToken.ii = curToken.ii | (curVal.ii & BitRange(44, 47));
                                nextCH();
                            }
                            if (CH == 'U')
                                nextCH();
                            goto exitLexer;
                        }
                        numFormat = octal;
                        if (CH == 'U') {
                            numFormat = fullword;
                            nextCH();
                        }
                    } else {
                        numFormat = decimal;
                        goto exitOctdec;
                    }
                    curToken.ii = 0;
                    for (tokenIdx = 1; tokenIdx <= tokenLen; ++tokenIdx) {
                        if (7 < numstr[tokenIdx])
                            error(20); /* errDigitGreaterThan7 */
                        curToken.ii = shl48(curToken.ii, 3);
                        curToken.ii = (numstr[tokenIdx] & 7) | curToken.ii;
                    }
                    goto exitLexer;
                } exitOctdec:
                curToken.ii = 0;
                for (tokenIdx = 1; tokenIdx <= tokenLen; ++tokenIdx) {
                    if (109951162777L >= curToken.ii)
                        curToken.ii = 10 * curToken.ii +
                            numstr[tokenIdx];
                    else {
                        error(errNumberTooLarge);
                        curToken.ii = 1;
                    }
                }
                if (CH == 'U') {
                    curToken.ii = curToken.ii & ~ Bits(0, 1, 3);
                    numFormat = fullword;
                    nextCH();
                    goto exitLexer;
                }
                expMagnitude = 0;
                if (CH == '.') {
                    nextCH();
                    if (CH == '.') {
                        CH = ':';
                        goto exitLexer;
                    }
                    curToken.r = curToken.ii;
                    SY = REALCONST;
                    if (charSymTabBase[CH] != INTCONST)
                        error(56); /* errNeedMantissaAfterDecimal */
                    else
                        do {
                            curToken.r = 10.0*curToken.r + CH - 48;
                            expMagnitude = expMagnitude-1;
                            nextCH();
                        } while (charSymTabBase[CH] == INTCONST);
                } /*2062*/
                if (CH == 'E') {
                    if (expMagnitude == 0) {
                        curToken.r = curToken.ii;
                        SY = REALCONST;
                    }
                    expSign = false;
                    nextCH();
                    if (CH == '+')
                        nextCH();
                    else if (CH == '-') {
                        expSign = true;
                        nextCH();
                    }
                    expLiteral = 0;
                    if (charSymTabBase[CH] != INTCONST)
                        error(57); /* errNeedExponentAfterE */
                    else
                        do {
                            expLiteral = 10 * expLiteral + CH - 48;
                            nextCH();
                        } while (charSymTabBase[CH] == INTCONST);
                    if (expSign)
                        expMagnitude = expMagnitude - expLiteral;
                    else
                        expMagnitude = expMagnitude + expLiteral;
                }; /* 2122 */
                if (expMagnitude != 0) {
                    expValue = 1.0;
                    expSign = expMagnitude < 0;
                    expMagnitude = std::abs(expMagnitude);
                    expMultiple = 10.0;
                    if (18 < expMagnitude) {
                        expMagnitude = 1;
                        error(58); /* errExponentGreaterThan18 */
                    }
                    do {
                        if (expMagnitude & 1)
                            expValue = expValue * expMultiple;
                        expMagnitude = expMagnitude / 2;
                        if (expMagnitude != 0)
                            expMultiple = expMultiple*expMultiple;
                    } while (expMagnitude != 0);
                    if (expSign)
                        curToken.r = curToken.r / expValue;
                    else
                        curToken.r = curToken.r * expValue;
                }
                goto exitLexer;
            } break; /* INTCONST */ /*=m+*/
            case CHARCONST: {
                {
                    for (tokenIdx = 6; tokenIdx <= 130; ++tokenIdx) {
                        nextCH();
                        if (charSymTabBase[CH] == CHARCONST) {
                            nextCH();
                            goto exitLoop;
                        }
                        if (atEOL) {
L2175:                      error(59); /* errEOLNInStringLiteral */
                            goto exitLoop;
                        } else if (CH == BACKSLASH) {
                            // base.pas 1563: '\NNN' octal (1..3 digits) or a
                            // named escape '\<letter>'.  base.pas indexes
                            // escSet/escMap by the BESM-6 input code; base.cc
                            // reads KOI-8, so map each (case-folded) letter
                            // directly to the same control code escMap yields.
                            nextCH();
                            if ('0' <= CH and CH <= '7') {
                                expLiteral = 0;
                                for (tokenLen = 0; ; ) {
                                    expLiteral = 8*expLiteral + CH - '0';
                                    tokenLen = tokenLen + 1;
                                    if (tokenLen < 3 and
                                        '0' <= PASINPUT and PASINPUT <= '7')
                                        nextCH();
                                    else
                                        break;
                                }
                                if (255 < expLiteral)
                                    error(
                                        errFirstDigitInCharLiteralGreaterThan3);
                                localBuf[tokenIdx] = (unsigned char)expLiteral;
                            } else {
                                unsigned char e = (CH >= 'A' and CH <= 'Z')
                                                  ? CH + 040 : CH;
                                int64_t val;
                                switch (e) {
                                case 'a': val = 7;  break; /* BEL */
                                case 'b': val = 8;  break; /* BS  */
                                case 'f': val = 12; break; /* FF  */
                                case 'n': val = 10; break; /* LF  */
                                case 'r': val = 13; break; /* CR  */
                                case 't': val = 9;  break; /* HT  */
                                case 'v': val = 11; break; /* VT  */
                                default:  goto L2233;   // not a known escape
                                }
                                localBuf[tokenIdx] = (unsigned char)val;
                            }
                        } else {
                            // Modify output encoding:
                            // a0 - UTF-8, a1 - KOI-8, a2 - KOI7 (default).
L2233:                      switch (charEncoding) {
                            case 0:
                                // KOI-8 to UTF-8.
                                if (CH < 0300) {
                                    localBuf[tokenIdx] = (CH < 0200) ? CH : ' ';
                                } else {
                                    const char *utf = koi2utf[CH - 0300];
                                    localBuf[tokenIdx++] = *utf++;
                                    localBuf[tokenIdx] = *utf;
                                }
                                break;
                            case 1:
                                // KOI-8.
                                localBuf[tokenIdx] = CH;
                                break;
                            case 3:
                                // base.pas 1598: internal 6-bit text
                                // (iso2text == koi2text), printable range
                                // '*'(052)..'_176' only, else NUL.
                                if (CH < '*' or 0176 < CH)
                                    localBuf[tokenIdx] = 0;
                                else
                                    localBuf[tokenIdx] = koi2text[CH];
                                break;
                            case 2:
                            default:
                                localBuf[tokenIdx] = koi8_to_koi7(CH);
                                break;
                            }
                        }
                    }
                    goto L2175;
                }
exitLoop:
                strLen = tokenIdx - 6;
                if (strLen == 0) {
                   error(61); /* errEmptyString */
                }
                if (strLen <= 1) {
                    SY = CHARCONST;
                    tokenLen = 1;
                    curToken.ii = '\0';
                    unpck(localBuf[0], curToken.a);
                    pck(localBuf[tokenLen], curToken.a);
                    goto exitLexer;
                } else {
                    curVal.ii = 0x202020202020L; // base.pas: curVal.a := '      '
                    SY = STRINGSY;
                    unpck(localBuf[tokenIdx], curVal.a);
                    pck(localBuf[6], curToken.a);
                    curVal = curToken;
                    if (6 >= strLen)
                        goto exitLexer;
                    else if (charEncoding == 3 and strLen == 8) {
                        // base.pas 1632: an 8-char string in 6-bit-text mode
                        // packs into one 48-bit word (pack(localbuf,6,.t)) and
                        // becomes an INTCONST.  Pack localBuf[6..13] MSB-first.
                        curToken.ii = 0;
                        for (tokenLen = 0; tokenLen < 8; ++tokenLen)
                            curToken.ii = (curToken.ii << 6)
                                        | (localBuf[6 + tokenLen] & 077);
                        curVal = curToken;
                        SY = INTCONST;
                        goto exitLexer;
                    } else {
                        curToken.ii = FcstCnt;
                        tokenLen = 6;
loop:                   {
                            toFCST();
                            tokenLen = tokenLen + 6;
                            if (tokenIdx < tokenLen) // base.pas 1643: strict <
                                goto exitLexer;      // exact multiples of 6 get
                                                     // a trailing 6-space word
                            pck(localBuf[tokenLen], curVal.a);
                            goto loop;
                       }
                   }
                };
                } break; /* CHARCONST */
            default: break;
            } /* switch */
        /* two-char operator lexer (base.pas 1652-1687). curToken.a is
           conceptually '      ' with [1]=prevCH, [2]=CH; only the pair
           is significant, so we match (prevCH, CH) directly. */
        prevCH = CH;
        nextCH();
        switch (prevCH) {
        case '+': case '-': case '*': case '/':
        case '%': case '&': case '|': case '^':
            if (CH == '=') { SY = BECOMES; nextCH(); goto exitLexer; }
            break;
        }
        switch (prevCH) {
        case '<':
            if (CH == '=') { charClass = LEOP; nextCH(); goto exitLexer; }
            if (CH == '<') { charClass = SHLEFT; nextCH();
                             if (CH == '=') { SY = BECOMES; nextCH(); }
                             goto exitLexer; }
            if (CH == ':') { SY = BEGINSY; nextCH(); goto exitLexer; }
            break;
        case '>':
            if (CH == '>') { charClass = SHRIGHT; nextCH();
                             if (CH == '=') { SY = BECOMES; nextCH(); }
                             goto exitLexer; }
            if (CH == '=') { charClass = GEOP; nextCH(); goto exitLexer; }
            break;
        case ':':
            if (CH == '>') { SY = ENDSY; nextCH(); goto exitLexer; }
            break;
        case '=':
            if (CH == '=') { SY = EXPROP; charClass = EQOP; nextCH();
                             goto exitLexer; }
            break;
        case '!':
            if (CH == '=') { charClass = NEOP; nextCH(); goto exitLexer; }
            break;
        case '-':
            if (CH == '>') { SY = ARROW; nextCH(); goto exitLexer; }
            if (CH == '-') { charClass = DECROP; nextCH(); goto exitLexer; }
            break;
        case '+':
            if (CH == '+') { charClass = INCROP; nextCH(); goto exitLexer; }
            break;
        case '|':
            if (CH == '|') { charClass = OROP; nextCH(); goto exitLexer; }
            break;
        case '&':
            if (CH == '&') { charClass = ANDOP; nextCH(); goto exitLexer; }
            break;
        case '/':
            if (CH == '*') { parseComment(); goto L1473; }
            if (CH == '/') { while (not atEOL) nextCH(); goto L1473; }
            break;
        case '.':
            if (CH == '.') { SY = COLON; nextCH(); goto exitLexer; }
            break;
        }
        if ((prevCH == '.') and (prevSY == ENDSY))
            dataCheck = true;
      exitLexer:
        prevSY = SY;
        commentModeCH = ' ';
        lookupMode = lookup2;
    }
} /* inSymbol */

void skipToEnd()
{
    Symbol sym;
    sym = SY;
    while ((sym != ENDSY) or (SY != PERIOD)) {
        sym = SY;
        inSymbol();
    }
    if (CH == 'D' || CH == 'd')
        while (SY != ENDSY)
            inSymbol();
    throw 9999;
}

void error(int64_t errNo)
{
    errors = true;
    bool110z = true;
    if (((linePos != prevErrPos) and (9 >= errsInLine))
        or (errNo == 52)) {
        totalErrors = totalErrors + 1;
        errMapBase[errsInLine] = linePos;
        errsInLine = errsInLine + 1;
        prevErrPos = linePos;
        printf("Error %ld:", errNo);
        printErrMsg(errNo);
        if (60 < totalErrors) {
            putchar('\n');
            endOfLine();
            printErrMsg(53);
            skipToEnd();
        }
    }
}

bool rawIntOk(const Word &w)
{
    return (w.ii >> 41) == 0;
}

int64_t rawIntToI64(const Word &w)
{
    if (not rawIntOk(w)) {
        error(200);
        return w.ii;
    }
    int64_t out = w.ii & INT41_MASK;
    if (out & INT41_SIGN)
        out -= (1L << 41);
    return out;
}

Word i64ToRawInt(int64_t value)
{
    Word out;
    out.ii = value & INT41_MASK;
    return out;
}

Word foldRawInt2(Operator op, const Word &lhs, const Word &rhs)
{
    int64_t a = rawIntToI64(lhs), b = rawIntToI64(rhs), r;

    switch (op) {
    case IDIVOP:
        r = a / b;
        // We're not yet ANSI C, % is modulo
        if (a % b < 0) --r;
        break;
    case IMODOP:
        r = a % b;
        if (r < 0) r += b > 0 ? b : - b;
        break;
    case IMULOP:
        r = a * b;
        break;
    case INTPLUS:
        r = a + b;
        break;
    case INTMINUS:
        r = a - b;
        break;
    default:
        error(200);
        return lhs;
    }
    return i64ToRawInt(r);
}

Word foldRawInt1(Operator op, const Word &arg)
{
    int64_t a = rawIntToI64(arg), r;

    switch (op) {
    case INEGOP:
        r = -a;
        break;
    default:
        error(200);
        return arg;
    }
    return i64ToRawInt(r);
}

void skip(int64_t toset)
{
    while (not has(toset, SY))
        inSymbol();
}

void errAndSkip(int64_t errNo, int64_t toset)
{
    error(errNo);
    skip(toset);
}

void parseLiteral(TPtr & litType, Word & litValue,
    bool allowSign)
{
    Operator l3var1z;
    litValue = curToken;
    if (STRINGSY < SY) {
        if (allowSign and (charClass == PLUSOP || charClass == MINUSOP))  {
            l3var1z = charClass;
            inSymbol();
            parseLiteral(litType, litValue, false);
            if (litType != IntegerType) {
                error(62); /* errIntegerNeeded */
                litType = IntegerType;
                litValue.ii = 1;
            } else if (l3var1z == MINUSOP) {
                litValue.ii = -litValue.ii;
            }
        } else {
L99:        litType.setRep(NULL);
            error(errNoConstant);
        }
    } else
        switch (SY) {
        case IDENT: {
            if ((hashTravPtr == NULL) or
                (hashTravPtr->cl != ENUMID))
                goto L99;
            litType = hashTravPtr->typ;
            litValue.ii = hashTravPtr->value();
        } break;
        case INTCONST:
            litType = IntegerType;
            break;
        case REALCONST:
            litType = RealType;
            break;
        case CHARCONST:
            litType = CharType;
            break;
        case STRINGSY:
            litType = makeStringType();
            break;
        default: break;
        } /* case */
} /* parseLiteral */

void hash(IdentRecPtr & l3arg1z, IdentRecPtr l3arg2z)
{
    bool l3var1z;
    int64_t l3var2z = 0;
    IdentRecPtr l3var3z, l3var4z;
    if (l3arg1z == NULL) {
        l3var2z = (l3arg2z->id % 65535) % 128;
        l3var1z = true;
        l3arg1z = symHash[l3var2z];
    } else {
        l3var1z = false;
    }
    if (l3arg1z == l3arg2z) {
        if (l3var1z) {
            symHash[l3var2z] =
                symHash[l3var2z]->next;
        } else {
            l3arg1z = l3arg2z->next;
        };
    } else {
        l3var3z = l3arg1z;
        while (l3var3z != l3arg2z) {
            l3var4z = l3var3z;
            if (l3var3z != NULL) {
                l3var3z = l3var3z->next;
            } else {
                return;
            }
        };
        l3var4z->next = l3arg2z->next;
    }
} /* hash */

// NOTE: base.pas has no isFileType; the sole caller lives in the still-upstream
// programme routine-declaration section and will be revisited with that
// transplant. Reconciled to the compact model here only so base.cc compiles.
bool isFileType(TPtr typtr)
{
    return (typtr.p.pk == kindStruct) and typtr.rep()->flag;
}

bool knownInType(IdentRecPtr & rec)
{
    if (programme::super.back()->typelist != NULL) {
        rec = programme::super.back()->typelist;
        while (rec != NULL) {
            if (rec->id == curIdent) {
                return true;
            }
            rec = rec->next;
        }
    }
    return false;
}

void checkSymAndRead(Symbol sym)
{
    if (SY != sym)
        requiredSymErr(sym);
    else
        inSymbol();
}

#ifdef kindrout
bool typeCheck(TPtr type1, TPtr type2);
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool sameRoutineType(TPtr type1, TPtr type2)
{
    SigPtr p1, p2;
    if ((type1.rep()->rargc != type2.rep()->rargc) or
        ((type1.rep()->rflags * Bits(20,21,24,26)) !=
         (type2.rep()->rflags * Bits(20,21,24,26)))) {
        return false;
    }
    if ((type1.rep()->rresult != type2.rep()->rresult) and
        (type1.rep()->rresult == NULL or type2.rep()->rresult == NULL or
         not typeCheck(type1.rep()->rresult, type2.rep()->rresult))) {
        return false;
    }
    p1 = type1.rep()->rparams;
    p2 = type2.rep()->rparams;
    while (p1 != NULL and p2 != NULL) {
        if (p1->pclass != p2->pclass)
            return false;
        if ((p1->ptyp != p2->ptyp) and
            (p1->ptyp == NULL or p2->ptyp == NULL or
             not typeCheck(p1->ptyp, p2->ptyp))) {
            return false;
        }
        p1 = p1->next;
        p2 = p2->next;
    }
    return (p1 == NULL) and (p2 == NULL);
} /* sameRoutineType */
#endif
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool typeCheck(TPtr type1, TPtr type2)
{ /* typeCheck */
    Kind kind1, kind2;
    int64_t span1, span2;
    rangeMismatch = false;
    if (not checkTypes or (type1 == type2)) {
L1:     return true;
    } else {
        kind1 = (Kind)type1.p.pk;
        kind2 = (Kind)type2.p.pk;
        if (kind1 == kind2) {
            switch (kind1) {
            case kindReal:
                /* empty */ break;
            case kindScalar:
                /* Two enums must be identical,
                 * all other combinations are okay.
                 */
                if (type1.rep()->enums == NULL or type2.rep()->enums == NULL)
                    goto L1;
                break;
            case kindPtr:
                if (type1 == voidPtr or type2 == voidPtr or
                    typeCheck(ptrBase(type1), ptrBase(type2)))
                    goto L1;
                break;
            case kindArray:
                span1 = type1.rep()->aright - type1.rep()->aleft;
                span2 = type2.rep()->aright - type2.rep()->aleft;
                if (typeCheck(type1.rep()->base, type2.rep()->base) and
                    (span1 == span2) and
                    (type1.rep()->pck == type2.rep()->pck) and
                    not rangeMismatch) {
                    if (type1.rep()->pck) {
                        if (type1.rep()->pcksize == type2.rep()->pcksize)
                            goto L1;
                    } else
                        goto L1;
                }
                break;
#ifdef kindrout
            case kindRoutine:
                if (sameRoutineType(type1, type2))
                    goto L1;
                break;
#endif
            default:
                break;
            } /* switch */
        }
        return false;
    }
} /* typeCheck */

int64_t argCount(IdentRecPtr l3arg1z)
{
    int64_t l3var1z;
    IdentRecPtr l3var2z;
    l3var2z = l3arg1z->argList();
    l3var1z = 0;
    if (l3var2z != NULL)
        while (l3var2z != l3arg1z) {
            l3var1z = l3var1z + 1;
            l3var2z = l3var2z->list();
        }
    return l3var1z;
} /* argCount */

struct formOperator {
    static std::vector<formOperator*> super;
    formOperator(OpGen l3arg1z);
    ~formOperator() { super.pop_back(); }

    int64_t l3int1z, l3int2z, l3int3z;
    int64_t nextInsn;
    ExprPtr helpExpr;
    OpFlg flags;
    bool direction;
    bool noTarget;
    Word l3var10z, l3var11z;
    InsnList * saved;
    bool rhsMode;
};
std::vector<formOperator*> formOperator::super;

struct genOneOp {
    int64_t insnBufIdx;
    int64_t l4var2z, l4var3z, l4var4z;
    Word l4var5z;
    OneInsnPtr l4inl6z, l4inl7z, l4inl8z;
    int64_t l4var9z;
    Word insnBuf[201]; // array [1..200] of Word;
    Word curInsn;
    Word tempInsn;
    OneInsnPtr l4oi212z;
    bool l4var213z;

    void P3363() {
        if (l4var213z)
            form1Insn(InsnTemp[XTA]);
        else
            form1Insn(KXTA+E1);
    }; /* P3363 */

    void addInsnToBuf(int64_t insn) {
        insnBuf[insnBufIdx].ii = insn;
        insnBufIdx = insnBufIdx + 1;
    }; /* addInsnToBuf */

    void add2InsnsToBuf(int64_t insn1, int64_t insn2) {
        insnBuf[insnBufIdx].ii = insn1;
        insnBuf[insnBufIdx+1].ii = insn2;
        insnBufIdx = insnBufIdx + 2;
    }; /* add2InsnsToBuf */

    bool F3413() {
        bool ret;
        l4inl7z = l4inl6z;
        while (l4inl7z != NULL) {
            if (l4inl7z->mode == curInsn.ii) {
                ret = true;
                while (l4inl7z->code == macro) {
                    l4inl7z = reinterpret_cast<OneInsn*>(ptr(l4inl7z->offset));
                }
                return ret;
            } else {
                l4inl7z = l4inl7z->next;
            }
        }
        return false;
    }; /* F3413 */

    void addJumpInsn(int64_t opcode) {
        if (not F3413()) {
            l4inl7z = new OneInsn;
            l4inl7z->next = l4inl6z;
            l4inl7z->mode = curInsn.ii;
            l4inl7z->code = 0;
            l4inl7z->offset = 0;
            l4inl6z = l4inl7z;
        };
        addInsnToBuf(macro + opcode + ord(l4inl7z));
    }; /* addJumpInsn */

    genOneOp() {
        if (insnList == NULL)
            return;
        usedRegs = usedRegs | insnList->regsused;
        l4oi212z = insnList->head;
        l4var9z = KNTR+7;
        insnBufIdx = 1;
        if (l4oi212z == NULL)
            return;
        l4inl6z = NULL;

        while (l4oi212z != NULL) {
            tempInsn.ii = l4oi212z->code;
            l4var4z = tempInsn.ii -  macro;
            curInsn.ii = l4oi212z->offset;
            switch (l4oi212z->mode) {
            case 0: break;
            case 1: if (arithMode != 1) {
                    addInsnToBuf(KNTR+7);
                    arithMode = 1;
                } break;
            case 2:
                arithMode = 1;
                break;
            case 3: if (arithMode != 2) {
                    addInsnToBuf(InsnTemp[NTR]);
                    arithMode = 2;
                } break;
            case 4:
                arithMode = 2;
                break;
            }; /* case */
            l4oi212z = l4oi212z->next;
            if (l4var4z >= 0) {
                switch (l4var4z) {
                case 21:
                    goto L3556;
                case 0:
                    addJumpInsn(InsnTemp[UZA]);
                    break;
                case 1:
                    addJumpInsn(InsnTemp[U1A]);
                    break;
                case 2: {
                      tempInsn.ii = curInsn.ii % 4096;
                      curInsn.ii = curInsn.ii / 4096;
                      addJumpInsn(InsnTemp[UJ]);
                      curInsn.ii = tempInsn.ii;
L3556:
                      if (F3413())
                          addInsnToBuf(2*macro+ord(l4inl7z));
                      else
                          error(206);
                } break;
                case 3: {
                      tempInsn.ii = curInsn.ii % 4096;
                      curInsn.ii = curInsn.ii / 4096;
                      l4var213z =  F3413();
                      l4inl8z = l4inl7z;
                      curInsn.ii = tempInsn.ii;
                      l4var213z = l4var213z && F3413();
                      if (l4var213z) {
                          l4inl7z->code = macro;
                          l4inl7z->offset = ord(l4inl8z);
                      }
                      else
                          error(207);
                } break;
                case 20:
                    addInsnToBuf(3*macro + curInsn.ii);
                    break;
                case 4: {
                    if ((insnBuf[insnBufIdx-1].ii & (BitRange(21,23)|BitRange(28,35))) == Bits())
                        insnBuf[insnBufIdx-1].ii = insnBuf[insnBufIdx-1].ii | Bits(35);
                    else
                        addInsnToBuf(KXTA+SP);
                } break;
                case 5:
                    /*blk*/ {
                    if (l4oi212z != NULL) {
                        tempInsn.ii = l4oi212z->code;
                        if ((tempInsn.ii & (BitRange(21,23)|BitRange(28,35))) == Bits(32)) {
                            l4oi212z->code =
                                tempInsn.ii - InsnTemp[XTA] + InsnTemp[XTS];
                            break; // exit blk
                        }
                    };
                    addInsnToBuf(KATX+SP);
                } break;
                case mcACC2ADDR:
                    add2InsnsToBuf(KATI+14, KUTC+I14);
                    break;
                case mcMULTI: {
                    addInsnToBuf(getHelperProc(12));        /* P/MI */
                } break;
                case mcADDSTK2REG:
                    add2InsnsToBuf(KWTC+SP, KUTM+indexreg[curInsn.ii]);
                    break;
                case mcADDACC2REG:
                    add2InsnsToBuf(KATI+14, KMADDJ+I14 + curInsn.ii);
                    break;
                case mcROUND: {
                    addInsnToBuf(KADD+REAL05);                /* round */
                    add2InsnsToBuf(KNTR+7, KADD+ZERO);
                } break;
                case 14:
                    add2InsnsToBuf(indexreg[curInsn.ii] + KVTM, KITA + curInsn.ii);
                    break;
                case mcMINEL: {
                    add2InsnsToBuf(KANX, KSUB+E1);   /* minel */
                } break;
                case 16:
                    add2InsnsToBuf(InsnTemp[XTA], KATX+SP + curInsn.ii);
                    break;
                case 17: {
                    addInsnToBuf(KXTS);
                    add2InsnsToBuf(KATX+SP+1, KUTM+SP + curInsn.ii);
                } break;
                case 18:
                    add2InsnsToBuf(KVTM+I10, getHelperProc(65)); /* P/B7 */
                    break;
                case mcPOP2ADDR: {
                    addInsnToBuf(KVTM+I14);
                    add2InsnsToBuf(KXTA+SP, KATX+I14);
                } break;
                case 22: {
                    add2InsnsToBuf(KVTM+I14, KXTA+I14);
                      curVal.ii = 040077777;
                      add2InsnsToBuf(allocSymtab(curVal.ii) + (KXTS+SP),
                                     KAAX+I8 + curInsn.ii);
                      add2InsnsToBuf(KAEX+SP, KATX+I14);
                } break;
                case mcMALLOC:
                /* MALLOC(N): N is in ACC (placed there by prepLoad).
                   Move N to register 14 and invoke the heap-allocator
                   helper P/PF + 4 (helper #33), which returns the newly
                   allocated pointer in ACC.  Same calling convention as
                   the NEW system procedure. */
                    add2InsnsToBuf(KATI+14, getHelperProc(33));
                    break;
                }; /* case */
            } else { /* 4003 */
                if (has(tempInsn.ii, 28)) {
                    addInsnToBuf(getValueOrAllocSymtab(curInsn.ii)+tempInsn.ii);
                } else {
                    curVal.ii = curInsn.ii & 077777;
                    if (curVal.ii < 2048)
                        addInsnToBuf(tempInsn.ii + curInsn.ii);
                    else
                        if ((curVal.ii >= 28672) or (curVal.ii < 4096)) {
                            addInsnToBuf(
                                allocSymtab((curVal.ii | 040000000) & halfWord)
                                + tempInsn.ii - 28672);
                        } else {
                            add2InsnsToBuf(getValueOrAllocSymtab(curVal.ii)
                                           + InsnTemp[UTC], tempInsn.ii);
                        }
                }
            }
        }; /* 4037 */
        insnBufIdx = insnBufIdx-1;

        for (l4var4z = insnBufIdx; l4var4z >= 1; --l4var4z) {
            curInsn = insnBuf[l4var4z];
            if ((curInsn.ii == InsnTemp[NTR]) or
                (curInsn.ii == KNTR+7)) {
                l4var3z = l4var4z - 1;
                l4var213z = false;
                while (l4var3z >= 1) {
                    tempInsn.ii = insnBuf[l4var3z].ii & BitRange(28,32);
                    if ((tempInsn.ii != KUTC) and (tempInsn.ii != KWTC))
                        break;
                    l4var3z = l4var3z-1;
                };

                l4var3z = l4var3z + 1;
                if (l4var3z != l4var4z) {
                    for (l4var2z = l4var4z-1;  l4var2z >= l4var3z; --l4var2z) {
                        insnBuf[l4var2z+1] = insnBuf[l4var2z];
                    }
                }
                insnBuf[l4var3z] = curInsn;
            } /* 4103 */
        }
        for (l4var4z = 1; l4var4z <= insnBufIdx; ++l4var4z)
            /*iter*/  {
            curInsn = insnBuf[l4var4z];
            tempInsn.ii = curInsn.ii & (Bits(0, 1, 3) | BitRange(23,32));
            if (tempInsn.ii == KATX+SP) {
                l4var2z = l4var4z + 1;
                while (insnBufIdx + 1 != l4var2z) {
                    curVal.ii = insnBuf[l4var2z].ii & (Bits(0, 1, 3, 23) | BitRange(28,35));
                    tempInsn.ii = curVal.ii & (Bits(0, 1, 3, 23) | BitRange(28,32));
                    if (curVal.ii == InsnTemp[XTA]) {
                        insnBuf[l4var2z].ii =
                            insnBuf[l4var2z].ii ^ Bits(32, 34, 35);
                        goto exit_iter;
                    } else if (curVal.ii == InsnTemp[ITA]) {
                        insnBuf[l4var2z].ii = insnBuf[l4var2z].ii | Bits(35);
                        goto exit_iter;
                    } else if ((curVal.ii == InsnTemp[NTR]) or
                               (tempInsn.ii == InsnTemp[UTC]) or
                               (tempInsn.ii == InsnTemp[WTC]) or
                               (tempInsn.ii == InsnTemp[VTM]))
                        l4var2z = l4var2z + 1;
                    else
                        l4var2z = insnBufIdx + 1;
                }
            } /* 4150 */
            if (curInsn.ii == InsnTemp[UTC])
                continue; // exit iter
            if (curInsn.ii < macro) {
                form1Insn(curInsn.ii);
                tempInsn.ii = curInsn.ii & BitRange(28,32);
                if ((tempInsn.ii == 03100000) or /* VJM */
                    (tempInsn.ii == 00500000))    /* ELFUN */
                    {
                        padToLeft();
                        prevOpcode = 1;
                    };
                continue; // exit iter
            }
            if (curInsn.ii >= 3*macro) {
                curInsn.ii = curInsn.ii - (3*macro);
                if (curInsn.ii >= 4096) {
                    l4var213z = true;
                    curInsn.ii = curInsn.ii - 4096;
                } else {
                  l4var213z = false;
                }
                curVal.ii = l4var213z;
                l4var2z = addCurValToFCST();
                curVal.ii = l4var213z ^ 1;
                tempInsn.ii = addCurValToFCST() - l4var2z;
                if (curInsn.ii == 0) {
                    padToLeft();
                    form1Insn(InsnTemp[UZA] + moduleOffset + 1);
                } else if (putLeft) {
                    form1Insn(InsnTemp[UTC]);
                }
                form1Insn(InsnTemp[UTC] + getValueOrAllocSymtab(tempInsn.ii));
                if (curInsn.ii != 0) {
                    if (not F3413())
                        error(211);
                    fixup(0, l4inl7z->code);
                }
                form1Insn(KXTA+I8 + l4var2z);
                continue;
            }; /* 4230 */
            if (curInsn.ii >= 2*macro) {
                l4inl7z = reinterpret_cast<OneInsn*>(ptr(curInsn.ii - (2*macro)));
                fixup(0, l4inl7z->code);
                l4inl7z->offset = moduleOffset;
            } else {
                curInsn.ii = curInsn.ii - macro;
                curVal.ii = curInsn.ii & (Bits(0, 1, 3) | BitRange(28,32));
                jumpType = curVal.ii;
                curVal.ii = (Bits(0, 1, 3) | BitRange(33,47)) & curInsn.ii;
                l4inl7z = reinterpret_cast<OneInsn*>(ptr(curVal.ii));
                formJump(l4inl7z->code);
                jumpType = InsnTemp[UJ];
                continue;
            }
          exit_iter:;
        } /* loop */

        insnList = NULL;
        while (l4inl6z != NULL) {
            if (l4inl6z->offset == 0) {
                jumpTarget = l4inl6z->code;
                return;
            } else
                l4inl6z = l4inl6z->next;
        }
        liveRegs = liveRegs & ~ usedRegs;
    }
}; /* genOneOp */

void addToInsnList(int64_t insn)
{
    OneInsnPtr elt = new OneInsn;
    elt->next = NULL;
    elt->mode = 0;
    elt->code = insn;
    elt->offset = 0;
    if (insnList->tail == NULL)
        insnList->head = elt;
    else
        insnList->tail->next = elt;
    insnList->tail = elt;
}

void addInsnAndOffset(int64_t insn, int64_t l4arg2z)
{
    addToInsnList(insn);
    insnList->tail->offset = l4arg2z;
}

void prependToInsnList(int64_t insn)
{
    OneInsnPtr elt = new OneInsn;
    elt->next = insnList->head;
    elt->mode = 0;
    elt->code = insn;
    elt->offset = 0;
    if (insnList->head == NULL)  {
        insnList->tail = elt;
    }
    insnList->head = elt;
}

void prepLoad()
{
    int64_t helper, l4int2z, l4int3z;
    TPtr valueType;
    Kind l4var5z;
    state l4st6z;
    bool isSimple;

    valueType = insnList->typ;
    switch (insnList->ilm) {
        case ilCONST: {
            curVal.ii = insnList->payload.ii;
            if (typeSize(valueType) == 1)
                curVal.ii = getFCSToffset();
            addToInsnList(constRegTemplate + curInsnTemplate + curVal.ii);
        } break;
        case ilLVAL: {
            helper = insnList->addrmd;
            l4int2z = insnList->payload.ii;
            l4int3z = insnList->disp;
            if (15 < helper) {
                /* empty */
            } else if (helper == 15) {
                addToInsnList(macro + mcACC2ADDR);
            } else {
                helper = indexreg[insnList->addrmd];
                if ((l4int2z == 0) and (insnList->st == stWORD)) {
                    addInsnAndOffset(helper + curInsnTemplate,
                                     l4int3z);
                    goto L4602;
                } else {
                    addToInsnList(helper + InsnTemp[UTC]);
                }
            }
            l4st6z = insnList->st;
            if (l4st6z == stWORD) {
                addInsnAndOffset(l4int2z + curInsnTemplate, l4int3z);
            } else {
                l4var5z = (Kind)(valueType.p.pk);
                if (l4var5z < kindArray or
                    (l4var5z == kindStruct and has(optSflags.ii, S6))) {
                    isSimple = true;
                } else {
                    isSimple = false;
                }
                if (l4st6z == stSLICE) {
                    if ((l4int3z != l4int2z) or
                        (helper != 15) or
                        (l4int2z != 0))
                        addInsnAndOffset(l4int2z + InsnTemp[XTA],
                                         l4int3z);
                    l4int3z = insnList->shift;
                    l4int2z = insnList->width;
                    helper = l4int3z + l4int2z;
                    if (isSimple) {
// The commented out optimization is specific to the original BESM-6
// without a barrel shifter. Now there is no need for it.
//                      if (30 < l4int3z) {
//                          addToInsnList(ASN64-48 + l4int3z);
//                          addToInsnList(KYTA);
//                      } else {
                            if (l4int3z != 0)
                                addToInsnList(ASN64 + l4int3z);
//                      }
                        if (helper != 48) {
                            curVal.ii = MASK48 >> (48 - l4int2z);
                            addToInsnList(KAAX+I8 + getFCSToffset());
                        }
                    } else {
                        if (helper != 48)
                            addToInsnList(ASN64-48 + helper);
                        curVal.ii = shl48(MASK48, 48 - l4int2z);
                        addToInsnList(KAAX+I8 + getFCSToffset());
                    }
                } else {
                    addToInsnList(getHelperProc(isSimple ? P_LDAR : P_RR));
                    insnList->tail->mode = 1;
                }
            }
            goto L4545;
        } break;
        case ilRVAL: {
L4545:      if (forValue and (valueType == BooleanType) and
                has(insnList->regsused, 16))
                addToInsnList(KAEX+E1);
        } break;
        case ilCOND: {
            if (forValue)
                addInsnAndOffset(macro+mcCOND2INT,
                                 has(insnList->regsused, 16)*010000 + insnList->payload.ii);
        } break;
    } /* case */
L4602:
    insnList->ilm = ilRVAL;
    insnList->regsused = insnList->regsused | Bits(0L);
} /* prepLoad */

void push()
{
    prepLoad();
    addToInsnList(macro + mcPUSH);
}

struct setAddrTo {
    Word l4var1z;
    int64_t l4int2z, opCode, l4var4z, l4var5z,
        l4var6z, regField;

    void getOffset() {
        l4var1z.ii = insnList->disp;
        l4var1z.ii = l4var1z.ii & 077777;
        l4var6z = l4var1z.ii;
    }; /* getOffset */

    setAddrTo(int64_t reg) {
        l4int2z = insnList->addrmd;
        opCode = InsnTemp[VTM];
        regField = indexreg[reg];
        l4var4z = insnList->payload.ii;
        insnList->regsused = insnList->regsused | Bits(reg);
        if (insnList->ilm == ilCONST) {
            curVal = insnList->payload;
            if (typeSize(insnList->typ) == 1)
                curVal.ii = addCurValToFCST();
            l4var6z = curVal.ii;
            l4var5z = 074001;
            goto L4654;
        } else if (l4int2z == 18) {
L4650:      getOffset();
            if (l4var4z == indexreg[1]) {
                l4var5z = 074003;
L4654:
                l4var1z.ii = macro * l4var5z + l4var6z;
                l4var6z = allocSymtab(l4var1z.ii & 0777777777777L);
                addToInsnList(regField + opCode + l4var6z);
            } else if (l4var4z != 0) {
                addInsnAndOffset(l4var4z + InsnTemp[UTC], l4var6z);
                addToInsnList(regField + opCode);
            } else {
                addInsnAndOffset(regField + opCode, l4var6z);
            }
        } else if (l4int2z == 17) {
            getOffset();
            l4var4z = insnList->disp;
            l4var5z = insnList->tail->code - InsnTemp[UTC];
            if (l4var4z != 0) {
                l4var1z.ii = macro * l4var5z + l4var4z;
                l4var5z = allocSymtab(l4var1z.ii & 0777777777777L);
            }
            insnList->tail->code = regField + l4var5z + opCode;
        } else if (l4int2z == 16) {
            getOffset();
            if (l4var4z != 0)
                addToInsnList(l4var4z + InsnTemp[UTC]);
            addInsnAndOffset(regField + opCode, l4var6z);
        } else if (l4int2z == 15) {
            addToInsnList(InsnTemp[ATI] + reg);
            opCode = InsnTemp[UTM];
            goto L4650;
        } else {
            addToInsnList(indexreg[l4int2z] + InsnTemp[UTC]);
            goto L4650;
        }
        insnList->ilm = ilLVAL;
        insnList->addrmd = reg;
        insnList->disp = 0;
        insnList->payload.ii = 0;
    } /* setAddrTo */
};

void prepStore()
{
    int64_t l4int1z, l4int2z, l4int3z;
    bool l4bool4z, l4bool5z;
    state l4st6z;
    Kind l4var7z;

    l4int1z = insnList->addrmd;
    if (15 < l4int1z) {
        /* nothing? */
    } else if (l4int1z == 15)  {
        addToInsnList(macro + mcACC2ADDR);
    } else {
        addToInsnList(indexreg[l4int1z] + InsnTemp[UTC]);
    }
    l4bool4z = has(insnList->regsused, 0);
    l4st6z = insnList->st;
    if ((l4st6z != stWORD) or l4bool4z)
        prependToInsnList(macro + mcPUSH);
    if (l4st6z == stWORD) {
        if (l4bool4z)  {
            addInsnAndOffset(insnList->payload.ii + InsnTemp[UTC],
                             insnList->disp);
            addToInsnList(macro+mcPOP2ADDR);
        } else {
            addInsnAndOffset(insnList->payload.ii, insnList->disp);
        }
    } else {
        l4var7z = (Kind)(insnList->typ.p.pk);
        l4int1z = typeBits(insnList->typ);
        l4bool5z = (l4var7z < kindArray) or
            ((l4var7z == kindStruct) and has(optSflags.ii, S6));
        if (l4st6z == stSLICE) {
            l4int2z = insnList->shift;
            l4int3z = l4int2z + insnList->width;
            if (l4bool5z)  {
                if (l4int2z != 0)
                    prependToInsnList(ASN64 - l4int2z);
            } else {
                if (l4int3z != 48)
                    prependToInsnList(ASN64 + 48 - l4int3z);
            }
            addInsnAndOffset(InsnTemp[UTC] + insnList->payload.ii,
                             insnList->disp);
            // binary negation of [(48-l4int3z)..(47 -l4int2z)]
            curVal.ii = shl48(MASK48, l4int3z) | (MASK48 >> (48 - l4int2z));
            addInsnAndOffset(macro+mcPCKSTORE, getFCSToffset());
        } else {
            if (not l4bool5z) {
                l4int2z = (insnList->width - l4int1z);
                if (l4int2z != 0)
                    prependToInsnList(ASN64 - l4int2z);
                prependToInsnList(InsnTemp[YTA]);
                prependToInsnList(ASN64 - l4int1z);
            }
            addToInsnList(getHelperProc(77)); /* "P/STAR" */
            insnList->tail->mode = 1;
        }
    }
} /* prepStore */

void spillAcc(Operator op)
{
    int64_t & localSize = programme::super.back()->localSize;
    int64_t & sizeCount = programme::super.back()->sizeCount;

    addInsnAndOffset(curFrameRegTemplate, localSize);
    curExpr = new Expr;
    curExpr->vt.typ = insnList->typ;
    genOneOp();
    curExpr->op = op;
    curExpr->num1 = localSize;
    localSize = localSize + 1;
    if (sizeCount < localSize)
        sizeCount = localSize;
}

int64_t insnCount()
{
    int64_t cnt;
    OneInsnPtr cur;
    cnt = 0;
    cur = insnList->head;
    while (cur != NULL) {
        cur = cur->next;
        cnt = cnt + 1;
    }
    return cnt;
}

/* Rotate a 48-bit set left/right by `amt` (negative = left). Matches
   base.pas `shift`; the exp-normalization of `amt` there is a host no-op. */
int64_t shift(int64_t val, int64_t amt)
{
    int64_t i;
    int64_t ret = Bits();
    for (i = 0; i <= 47; ++i)
        if (has(val, i - amt))
            ret = ret | Bits(i);
    return ret;
}

struct genFullExpr {
    static std::vector<genFullExpr*> super;
    genFullExpr(ExprPtr exprToGen_);
    ~genFullExpr() { super.pop_back(); }

    ExprPtr & exprToGen;
    bool arg1Const, arg2Const;
    InsnList * otherIns;
    Word arg1Val, arg2Val;
    Operator curOP;
    int64_t work;

    void startLVal() {
        prepLoad();
        insnList->ilm = ilLVAL;
        insnList->st = stWORD;
        insnList->disp = 0;
        insnList->payload.ii = 0;
        insnList->addrmd = 18;
    }; /* startLVal */

    void genDeref() {
        Word l5var1z, l5var2z;

        /* The optimised path manipulates addrmd/disp/payload, which only carry
           meaning for ilLVAL operands; for ilRVAL fall through to mcACC2ADDR. */
        if (insnList->ilm == ilLVAL and (
                (insnList->st == stWORD) or
                (insnList->st == stSLICE and
                 insnList->shift == 0))) {
            l5var1z.ii = insnList->addrmd;
            l5var2z.ii = insnList->disp;
            if (l5var1z.ii == 18 or l5var1z.ii == 16) {
L5220:          addInsnAndOffset((insnList->payload.ii + InsnTemp[WTC]), l5var2z.ii);
            } else {
                if (l5var1z.ii == 17) {
                    if (l5var2z.ii == 0) {
                        insnList->tail->code = insnList->tail->code +
                            InsnTemp[XTA];
                    } else
                        goto L5220;
                } else if (l5var1z.ii == 15) {
                    addToInsnList(macro + mcACC2ADDR);
                    goto L5220;
                } else {
                    addInsnAndOffset((indexreg[l5var1z.ii] + InsnTemp[WTC]),
                                     l5var2z.ii);
                }
            }
        } else {
            startLVal();
            addToInsnList(macro + mcACC2ADDR);
        }
        insnList->disp = 0;
        insnList->payload.ii = 0;
        insnList->addrmd = 16;
        insnList->st = stWORD;
    }; /* genDeref */

    void genHelper() {
        InsnList * &saved = formOperator::super.back()->saved;
        push();
        saved = insnList;
        insnList = otherIns;
        prepLoad();
        addToInsnList(getHelperProc(formOperator::super.back()->nextInsn));
        insnList->regsused = insnList->regsused | saved->regsused | BitRange(11,14);
        saved->tail->next = insnList->head;
        insnList->head = saved->head;
    }; /* genHelper */

    void prepMultiWord() {
        bool l5var1z;
        InsnList * l5var2z;

        l5var1z = has(otherIns->regsused, 12);
        setAddrTo(12);
        if (l5var1z) {
            addToInsnList(KITA+12);
            addToInsnList(macro + mcPUSH);
        }
        l5var2z = insnList;
        insnList = otherIns;
        setAddrTo(14);
        if (l5var1z) {
            addToInsnList(macro + mcPOP);
            addToInsnList(KATI+12);
        }
        l5var2z->regsused = insnList->regsused | l5var2z->regsused;
        l5var2z->tail->next = insnList->head;
        l5var2z->tail = insnList->tail;
        insnList = l5var2z;
    }; /* prepMultiWord */

    void negateCond () {
        if (insnList->ilm == ilCONST) {
            insnList->payload.ii = not insnList->payload.ii;
        } else {
            insnList->regsused = insnList->regsused ^ Bits(16);
        }
    }; /* negateCond */

    void tryFlip(bool commutes) {
        int64_t l5var1z;
        InsnList * l5var2z;
        InsnList * &saved = formOperator::super.back()->saved;
        int64_t &nextInsn = formOperator::super.back()->nextInsn;

        if (not has(otherIns->regsused, 0)) {
            l5var1z = 0;
        } else if (not has(insnList->regsused, 0)) {
            l5var1z = commutes + 1;
        } else {
            l5var1z = 3;
        }
        switch (l5var1z) {
        case 0: {
L100:     prepLoad();
          saved = insnList;
          insnList = otherIns;
          curInsnTemplate = nextInsn;
          prepLoad();
          curInsnTemplate = InsnTemp[XTA];
        } break;
        case 1:
            if (nextInsn == InsnTemp[SUB]) {
                nextInsn = InsnTemp[RSUB];
                goto L22;
            } else
                goto L33;
            break;
        case 2: {
L22:        saved = insnList;
            insnList = otherIns;
            otherIns = saved;
            goto L100;
        } break;
        case 3: {
L33:        prepLoad();
            addToInsnList(indexreg[15] + nextInsn);
            l5var2z = insnList;
            insnList = otherIns;
            push();
              saved = insnList;
              insnList = l5var2z;
        } break;
        }; /* case */
        insnList->tail->mode = 0;
        saved->tail->next = insnList->head;
        insnList->head = saved->head;
        insnList->regsused = insnList->regsused | Bits(0L);
    }; /* tryFlip */

    void genBoolAnd() {
        bool l5var1z, l5var2z;
        int64_t l5var3z, l5var4z, l5var5z, l5var6z, l5var7z;
        InsnList * l5ins8z;
        Word l5var9z;

        if (arg1Const) {
            if (arg1Val.ii)
              insnList = otherIns;
        } else if (arg2Const) {
            if (not arg2Val.ii)
                insnList = otherIns;
        } else {
            l5var1z = has(insnList->regsused, 16);
            l5var2z = has(otherIns->regsused, 16);
            l5var5z = condLabCnt;
            condLabCnt = condLabCnt + 1;
            forValue = false;
            l5var6z = l5var1z + macro;
            l5var7z = l5var2z + macro;
            if (insnList->ilm == ilCOND) {
                l5var3z = insnList->payload.ii;
            } else {
                l5var3z = 0;
                prepLoad();
            }
            if (otherIns->ilm == ilCOND) {
                l5var4z = otherIns->payload.ii;
            } else {
                l5var4z = 0;
            }
            l5var9z.ii = (insnList->regsused | otherIns->regsused);
            if (l5var3z == 0) {
                if (l5var4z == 0) {
                    addInsnAndOffset(l5var6z, l5var5z);
                    l5ins8z = insnList;
                    insnList = otherIns;
                    prepLoad();
                    addInsnAndOffset(l5var7z, l5var5z);
                } else {
                    if (l5var2z) {
                        addInsnAndOffset(l5var6z, l5var5z);
                        l5ins8z = insnList;
                        insnList = otherIns;
                        addInsnAndOffset(macro + mcJUMP,
                                         010000 * l5var5z + l5var4z);
                    } else {
                        addInsnAndOffset(l5var6z, l5var4z);
                        l5var5z = l5var4z;
                        l5ins8z = insnList;
                        insnList = otherIns;
                    }
                }
            } else {
                if (l5var4z == 0) {
                    if (l5var1z) {
                        addInsnAndOffset(macro + mcJUMP,
                                         010000 * l5var5z + l5var3z);
                        l5ins8z = insnList;
                        insnList = otherIns;
                        prepLoad();
                        addInsnAndOffset(l5var7z, l5var5z);
                    } else {
                        l5ins8z = insnList;
                        insnList = otherIns;
                        prepLoad();
                        addInsnAndOffset(l5var7z, l5var3z);
                        l5var5z = l5var3z;
                    }
                } else {
                    if (l5var1z) {
                        if (l5var2z) {
                            addInsnAndOffset(macro + mcJUMP,
                                             010000 * l5var5z + l5var3z);
                            l5ins8z = insnList;
                            insnList = otherIns;
                            addInsnAndOffset(macro + mcJUMP,
                                             010000 * l5var5z + l5var4z);
                        } else {
                            addInsnAndOffset(macro + mcJUMP,
                                             010000 * l5var4z + l5var3z);
                            l5ins8z = insnList;
                            insnList = otherIns;
                            l5var5z = l5var4z;
                        }
                    } else {
                        l5ins8z = insnList;
                        insnList = otherIns;
                        l5var5z = l5var3z;
                        if (l5var2z)
                            addInsnAndOffset(macro + mcJUMP,
                                             010000 * l5var3z + l5var4z);
                        else
                            addInsnAndOffset(macro + 3,
                                             010000 * l5var3z + l5var4z);
                    }
                }
            }
            insnList->regsused = l5var9z.ii & ~ Bits(16);
            l5ins8z->tail->next = insnList->head;
            insnList->head = l5ins8z->head;
            insnList->ilm = ilCOND;
            insnList->payload.ii = l5var5z;
            forValue = true;
        }
    } /* genBoolAnd */

    void genConstDiv() {
        // base.pas 3561: power-of-2 divisors (card==1) collapse to a single
        // arithmetic shift; other divisors emit a reciprocal multiply first,
        // then the residual shift.
        Real r;
        if (card(arg2Val.ii) > 1) {
            curVal.r = 1.0 / (double)(int64_t)arg2Val.ii;
            r = (double)curVal.r * (int64_t)arg2Val.ii;
            curVal.ii = curVal.ii & BitRange(7, 47);
            if ((double)r < 1.0)
                curVal.ii = curVal.ii + 1;
            curVal.ii = curVal.ii | Bits(0);
            addToInsnList(KMUL+I8 + getFCSToffset());
        }
        addToInsnList(ASN64 + 47 - minel(arg2Val.ii));
    }; /* genConstDiv */

    /* Ternary conditional CONDOP{cond, ALTERN{then, else}}: build one deferred
       ilRVAL chain (cond; UZA/U1A elseLab; then; UJ endLab; elseLab: else;
       endLab:) using the macro forward-jump/label machinery. */
    void genCondOp() {
        ExprPtr altExpr;
        int64_t elseLab, endLab;
        InsnList * condChain, * thenChain;

        altExpr = exprToGen->expr2;
        elseLab = condLabCnt;
        condLabCnt = condLabCnt + 1;
        endLab = condLabCnt;
        condLabCnt = condLabCnt + 1;
        forValue = false;
        curExpr = exprToGen->expr1;
        (void) genFullExpr(curExpr);
        if (insnList->ilm == ilCOND and insnList->payload.ii != 0) {
            if (has(insnList->regsused, 16))
                elseLab = insnList->payload.ii;
            else
                addInsnAndOffset(macro + 2,
                                 elseLab * 010000 + insnList->payload.ii);
        } else {
            prepLoad();
            if (has(insnList->regsused, 16))
                addInsnAndOffset(macro + 1, elseLab);
            else
                addInsnAndOffset(macro + 0, elseLab);
        }
        forValue = true;
        condChain = insnList;
        curExpr = altExpr->expr1;
        (void) genFullExpr(curExpr);
        prepLoad();
        addInsnAndOffset(macro + 2, endLab * 010000 + elseLab);
        thenChain = insnList;
        curExpr = altExpr->expr2;
        (void) genFullExpr(curExpr);
        prepLoad();
        addInsnAndOffset(macro + 21, endLab);
        condChain->tail->next = thenChain->head;
        condChain->tail = thenChain->tail;
        condChain->tail->next = insnList->head;
        condChain->tail = insnList->tail;
        condChain->regsused = condChain->regsused | thenChain->regsused
                              | insnList->regsused | Bits(0);
        insnList = condChain;
        insnList->typ = exprToGen->vt.typ;
        insnList->ilm = ilRVAL;
        insnList->st = stWORD;
    } /* genCondOp */

    /* RMWASSIGN(lhs, inner-op(rhs, NIL)): read-modify-write assignment; walk
       the lvalue once, push its address twice, then generate
       ASSIGNOP(stklval, op(stklval, rhs)) via the STKLVAL sentinel. */
    void genRMWAssign() {
        ExprPtr innerNode, rhsExpr, lhsExpr, rmwLhs;
        ExprPtr synthOp, synthAsn;
        Operator innerOp;
        bool needsMater;
        bool &rhsMode = formOperator::super.back()->rhsMode;

        lhsExpr = exprToGen->expr1;
        innerNode = exprToGen->expr2;
        innerOp = innerNode->op;
        rhsExpr = innerNode->expr1;
        needsMater = (lhsExpr->op != GETVAR) and
                     ((lhsExpr->op != GETFIELD) or
                      (lhsExpr->expr1->op != GETVAR)) and
                     ((lhsExpr->op != GETELT) or
                      (lhsExpr->expr2->op != GETVAR));
        if (needsMater) {
            rhsMode = false;
            (void) genFullExpr(lhsExpr);
            rhsMode = true;
            if (insnList->st != stWORD) {
                error(errVarTooComplex);
                return;
            }
            setAddrTo(14);
            addToInsnList(KITA + 14);
            addToInsnList(macro + mcPUSH);
            addToInsnList(KITA + 14);
            addToInsnList(macro + mcPUSH);
            genOneOp();
            insnList = NULL;
            rmwLhs = mkExpr(STKLVAL, lhsExpr->vt.typ, NULL, NULL);
        } else {
            rmwLhs = lhsExpr;
        }
        synthOp = mkExpr(innerOp, innerNode->vt.typ, rmwLhs, rhsExpr);
        synthAsn = mkExpr(ASSIGNOP, exprToGen->vt.typ, rmwLhs, synthOp);
        (void) genFullExpr(synthAsn);
    } /* genRMWAssign */

};
std::vector<genFullExpr*> genFullExpr::super;

void genGetElt()
{
    int64_t l5var1z, dimCnt, curDim, l5var4z, l5var5z, l5var6z,
        l5var7z, l5var8z;
    InsnList insnCopy;
    InsnListPtr copyPtr, l5ins21z;
    Word l5var22z, l5var23z;
    bool l5var24z, l5var25z;
    TPtr l5var26z;
    ilmode l5ilm28z;
    ExprPtr l5var29z;
    InsnListPtr getEltInsns[11]; // array [1..10] of InsnListPtr;
    ExprPtr & exprToGen = genFullExpr::super.back()->exprToGen;
    InsnList * &saved = formOperator::super.back()->saved;

    dimCnt = 0;
    l5var29z = exprToGen;
    while (l5var29z->op == GETELT) {
        genFullExpr(l5var29z->expr2);
        dimCnt = dimCnt + 1;
        getEltInsns[dimCnt] = insnList;
        l5var29z = l5var29z->expr1;
    }
    (void) genFullExpr(l5var29z);
    l5ins21z = insnList;
    insnCopy = *insnList;
    copyPtr = &insnCopy;
    l5var22z.ii = freeRegs;
    for (curDim = 1; curDim <= dimCnt; ++curDim)
        l5var22z.ii = l5var22z.ii & ~ getEltInsns[curDim]->regsused;
    for (curDim = dimCnt; curDim >= 1; curDim--) {
        l5var26z = insnCopy.typ.rep()->base;
        l5var25z = insnCopy.typ.rep()->pck;
        l5var7z = insnCopy.typ.rep()->aleft;
        l5var8z = typeSize(l5var26z);
        if (not l5var25z)
            insnCopy.disp = insnCopy.disp - l5var8z * l5var7z;
        insnList = getEltInsns[curDim];
        l5ilm28z = insnList->ilm;
        if (l5ilm28z == ilCONST) {
            curVal = insnList->payload;
            if (curVal.ii < l5var7z or
                insnCopy.typ.rep()->aright < curVal.ii)
                error(29); /* errIndexOutOfBounds */
            if (l5var25z) {
                l5var4z = curVal.ii - l5var7z;
                l5var5z = insnCopy.typ.rep()->perword;
                insnCopy.regsused = insnCopy.regsused | Bits(0L);
                insnCopy.disp = l5var4z / l5var5z + insnCopy.disp;
                l5var6z = (l5var5z-1-l5var4z % l5var5z) *
                    insnCopy.typ.rep()->pcksize;
                switch (insnCopy.st) {
                case stWORD: insnCopy.shift = l5var6z;
                    break;
                case stSLICE: insnCopy.shift = insnCopy.shift + l5var6z +
                        typeBits(insnCopy.typ) - 48;
                    break;
                case stPACKED: error(errUsingVarAfterIndexingPackedArray);
                    break;
                } /* case */
                insnCopy.width = insnCopy.typ.rep()->pcksize;
                insnCopy.st = stSLICE;
            } /* 6116 */ else {
                insnCopy.disp = curVal.ii  * typeSize(l5var26z) +
                    insnCopy.disp;
            }
        } else { /* 6123*/
            if (l5var8z != 1) {
                prepLoad();
                addToInsnList(insnCopy.typ.rep()->perword);
                insnList->tail->mode = 1;
                if (l5var7z >= 0)
                    addToInsnList(KYTA+64);
                else
                    addToInsnList(macro + mcMULTI);
           }
            if (l5ilm28z == ilCOND or
                (l5ilm28z == ilLVAL and
                 insnList->st != stWORD))
                prepLoad();
           l5var23z.ii = insnCopy.regsused | insnList->regsused;
           if (not l5var25z) {
               if (insnCopy.addrmd == 18) {
                    if (insnList->ilm == ilRVAL) {
                        insnCopy.addrmd = 15;
                    } else { /* 6200 */
                        insnCopy.addrmd = 16;
                        curInsnTemplate = InsnTemp[WTC];
                        prepLoad();
                        curInsnTemplate = InsnTemp[XTA];
                    }; /* 6205 */
                    insnCopy.tail = insnList->tail;
                    insnCopy.head = insnList->head;
                } else { /* 6211 */
                    if (insnCopy.addrmd >= 15) {
                        l5var1z = minel(l5var22z.ii);
                        if (0 >= l5var1z) {
                            l5var1z = minel(freeRegs & ~ insnCopy.regsused);
                            if (0 >= l5var1z)
                                l5var1z = 9;
                        }
                        saved = insnList;
                        insnList = copyPtr;
                        l5var23z.ii = l5var23z.ii | Bits(l5var1z);
                        if (insnCopy.addrmd == 15) {
                            addToInsnList(InsnTemp[ATI] + l5var1z);
                        } else {
                            addToInsnList(indexreg[l5var1z] + InsnTemp[VTM]);
                        }
                        insnCopy.addrmd = l5var1z;
                        insnCopy.regsused = insnCopy.regsused | Bits(l5var1z);
                        insnList = saved;
                    } else {
                            l5var1z = insnCopy.addrmd;
                    } /* 6251 */
                    if (has(insnList->regsused, l5var1z)) {
                        push();
                        insnList->tail->next = insnCopy.head;
                        insnCopy.head = insnList->head;
                        insnList = copyPtr;
                        addInsnAndOffset(macro+mcADDSTK2REG, l5var1z);
                    } else {
                         if (insnList->ilm == ilRVAL) {
                             addInsnAndOffset(macro+mcADDACC2REG, l5var1z);
                         } else {
                             curInsnTemplate = InsnTemp[WTC];
                             prepLoad();
                             curInsnTemplate = InsnTemp[XTA];
                             addToInsnList(indexreg[l5var1z] + InsnTemp[UTM]);
                         }
                         insnCopy.tail->next = insnList->head;
                         insnCopy.tail = insnList->tail;
                     }
                } /* 6305 */
           } else { /* 6306 */
                if (insnCopy.st == stWORD) {
                    prepLoad();
                    if (l5var7z != 0) {
                        curVal.ii = (0 - l5var7z) & INT41_MASK;
                        addToInsnList(KADD+I8 + getFCSToffset());
                        insnList->tail->mode = 1;
                    }
                    l5var24z = has(insnCopy.regsused, 0);
                    if (l5var24z)
                        addToInsnList(macro + mcPUSH);
                    saved = insnList;
                    insnList = copyPtr;
                    setAddrTo(14);
                    if (l5var24z)
                        addToInsnList(macro + mcPOP);
                    l5var23z.ii = l5var23z.ii | Bits(0, 10, 11, 13) | Bits(14);
                    insnCopy.st = stPACKED;
                    insnCopy.disp = 0;
                    insnCopy.payload.ii = 0;
                    insnCopy.width = insnCopy.typ.rep()->pcksize;
                    curVal.ii = insnCopy.width;
                    if (curVal.ii == 24)
                        curVal.ii = 7;
                    curVal.ii = shl48(curVal.ii, 24);
                    addToInsnList(allocSymtab(  /* P/00C */
                        helperNames[76] | curVal.ii)+(KVTM+I11));
                    insnCopy.addrmd = 16;
                    insnCopy.shift = 0;
                    saved->tail->next = insnCopy.head;
                    insnCopy.head = saved->head;
                } else {
                    error(errUsingVarAfterIndexingPackedArray);
                }
            } /* 6403 */
            insnCopy.regsused = l5var23z.ii;
        }
        insnCopy.typ = l5var26z;
    } /* 6406 */
    insnList = l5ins21z;
    *insnList = insnCopy;
} /* genGetElt */

struct genEntry {
    genEntry();

    ExprPtr l5exp1z, l5exp2z;
    IdentRecPtr l5idr3z, l5idr4z, l5idr5z, l5idr6z;
    bool l5bool7z, l5bool8z, l5bool9z, l5bool10z, l5bool11z;
    bool isAssembler; // base.pas: 26 in calleeFl.ii (flags bit 26)
    Word l5var12z, l5var13z, l5var14z;
    int64_t l5var15z, l5var16z;
    Word l5var17z, l5var18z, l5var19z;
    InsnListPtr l5inl20z;
    Operator l5op21z;
    IdClass l5idc22z;
};

int64_t allocGlobalObject(IdentRecPtr l6arg1z)
{
    if (l6arg1z->pos() == 0) {
        if ((l6arg1z->flags() & Bits(20, 21)) != Bits()) {
            curVal.ii = leftAlign(l6arg1z->id);
            l6arg1z->pos() = allocExtSymbol(extSymMask);
        } else {
            l6arg1z->pos() = symTabPos;
            putToSymTab(0);
        }
    }
    return l6arg1z->pos();
}

genEntry::genEntry()
{
    ExprPtr & exprToGen = genFullExpr::super.back()->exprToGen;
    l5exp1z = exprToGen->expr1;
    l5idr5z = exprToGen->id2;
    l5bool7z = (l5idr5z->typ == NULL);
    l5bool9z = (l5idr5z->list() == NULL);
    if (l5bool7z)
        l5var13z.ii = 3;
    else
        l5var13z.ii = 4;
    l5var12z.ii = l5idr5z->flags();
    l5bool10z = (has(l5var12z.ii, 21));   // isFortrn
    isAssembler = (has(l5var12z.ii, 26)); // base.pas 3297
    l5bool11z = (has(l5var12z.ii, 24));   // allByRef
    if (l5bool9z) {
        l5var14z.ii = argCount(l5idr5z);
        l5idr6z = l5idr5z->argList();
    } else {
        l5var13z.ii = l5var13z.ii + 2;
    }
    insnList = new InsnList;
    insnList->head = NULL;
    insnList->tail = NULL;
    insnList->typ = l5idr5z->typ;
    insnList->regsused = (l5idr5z->flags() | BitRange(7,15)) & (BitRange(0,8)|BitRange(10,15));
    insnList->ilm = ilRVAL;
    if (isAssembler) {          // base.pas 3311: assembler routine, no frame
        l5bool8z = false;
    } else if (l5bool10z) {     // isFortrn
        l5bool8z = not l5bool7z;
        if (checkFortran) {
            addToInsnList(getHelperProc(92)); /* "P/MF" */
        }
    } else {
        l5bool8z = true;
        if (((not l5bool9z) and (l5exp1z != NULL))
            or ((l5bool9z) and (l5var14z.ii >= 2))) {
            addToInsnList(KUTM+SP + l5var13z.ii);
        }
    }
    l5var14z.ii = 0;
// (loop)
    while (l5exp1z != NULL) { /* 6574 */
        l5exp2z = l5exp1z->expr2;
        l5exp1z = l5exp1z->expr1;
        l5op21z = l5exp2z->op;
        l5var14z.ii = l5var14z.ii + 1;
        l5inl20z = insnList;
        if ((l5op21z == PCALL) or (l5op21z == FCALL)) {
            l5idr4z = l5exp2z->id2;
            insnList = new InsnList;
            insnList->head = NULL;
            insnList->tail = NULL;
            insnList->regsused = Bits();
            usedRegs = usedRegs | l5idr4z->flags();
            if (l5idr4z->list() != NULL) {
                addToInsnList(l5idr4z->offset + InsnTemp[XTA] +
                              l5idr4z->value());
                if (l5bool10z)
                    addToInsnList(getHelperProc(19)); /* "P/EA" */
            } else
                /*(a) */         { /* 6636 */
                if (l5idr4z->value() == 0) {
                    if ((l5bool10z) and (has(l5idr4z->flags(), 21))) {
                        addToInsnList(allocGlobalObject(l5idr4z) +
                                      (KVTM+I14));
                        addToInsnList(KITA+14);
                        goto exit_a;
                    } else { /* 6651 */
                        l5var16z = 0;
                        formJump(l5var16z);
                        padToLeft();
                        l5idr4z->value() = moduleOffset;
                        l5idr3z = l5idr4z->argList();
                        l5var15z = l5idr4z->typ != NULL;
                        l5var17z.ii = argCount(l5idr4z);
                        form3Insn(KVTM+I10+ 4+moduleOffset,
                                  KVTM+I9 + l5var15z,
                                  KVTM+I8 + 074001);
                        formAndAlign(getHelperProc(62)); /* "P/BP" */
                        l5var15z = l5var17z.ii + 2 + l5var15z;
                        form1Insn(KXTA+SP + l5var15z);
                        if ((1) < l5var17z.ii)
                            form1Insn(KUTM+SP + l5var15z);
                        else
                            form1Insn(0);
                        form2Insn(
                            getHelperProc(63/*P/B6*/) - 0500000,
                            allocGlobalObject(l5idr4z) + KUJ);
                        // If a routine is passed as an actual parameter,
                        // its (rough) prototype is stored for checking
                        // against calls to formal parameters at runtime.
                        if (l5idr3z != NULL) {
                            do {
                                l5idc22z = l5idr3z->cl;
                                if ((l5idc22z == ROUTINEID) and
                                    (l5idr3z->typ != NULL))
                                    l5idc22z = ENUMID;
                                form2Insn(0, l5idc22z);
                                l5idr3z = l5idr3z->list();
                            } while (l5idr4z != l5idr3z);
                        } /* 6745 */
                        storeObjWord(0);
                        fixup(0, l5var16z);
                    }
                } /* 6752 */
                addToInsnList(KVTM+I14 + l5idr4z->value());
                if (has(l5idr4z->flags(), 21))
                    addToInsnList(KITA+14);
                else
                    addToInsnList(getHelperProc(64)); /* "P/PB" */
              exit_a:;
            }; /* 6765 */
            if (l5op21z == PCALL)
                l5idc22z = ROUTINEID;
            else
                l5idc22z = ENUMID;
        } else { /* 6772 */
            (void) genFullExpr(l5exp2z);
            if (insnList->ilm == ilLVAL)
              l5idc22z = FORMALID;
            else
                l5idc22z = VARID;
        } /* 7001 */
        if (not (not l5bool9z or (l5idc22z != FORMALID) or
                 (l5idr6z->cl != VARID)))
            l5idc22z = VARID;
          loop:
        if ((l5idc22z == FORMALID) or (l5bool11z)) {
            setAddrTo(14);
            addToInsnList(KITA+14);
        } else if (l5idc22z == VARID) {
            if (typeSize(insnList->typ) != 1) {
                l5idc22z = FORMALID;
                goto loop;
            } else {
                prepLoad();
            }
        } /* 7027 */
        if (not l5bool8z)
            prependToInsnList(macro + mcPUSH);
        l5bool8z = false;
        if (l5inl20z->tail != NULL) {
            l5inl20z->tail->next = insnList->head;
            insnList->head = l5inl20z->head;
        }
        insnList->regsused = insnList->regsused | l5inl20z->regsused;
        if (not l5bool9z) {
            curVal.ii = l5idc22z;
            addToInsnList(KXTS+I8 + getFCSToffset());
        }
        if (l5bool9z and not l5bool11z)
            l5idr6z = l5idr6z->list();
    }; /* while -> 7061 */
    if (l5bool10z) {
        addToInsnList(KNTR+2);
        insnList->tail->mode = 4;
    }
    if (l5bool9z) {
        addToInsnList(allocGlobalObject(l5idr5z) + (KVJM+I13));
        if (has(l5idr5z->flags(), 20)) {
            l5var17z.ii = 1;
        } else {
            l5var17z.ii = l5idr5z->offset / 04000000;
        } /* 7102 */
    } else { /* 7103 */
        l5var15z = 0;
        if (l5var14z.ii == 0) {
            l5var17z.ii = l5var13z.ii + 1;
        } else {
            l5var17z.ii = -(2 * l5var14z.ii + l5var13z.ii);
            l5var15z = 1;
        } /* 7115 */
        addInsnAndOffset(macro+16 + l5var15z,
                         getValueOrAllocSymtab(l5var17z.ii));
        addToInsnList(l5idr5z->offset + InsnTemp[UTC] + l5idr5z->value());
        addToInsnList(macro+18);
        l5var17z.ii = 1;
    } /* 7132 */
    insnList->tail->mode = 2;
    if (not isAssembler and curProcNesting != l5var17z.ii) { // base.pas 3459
        if (not l5bool10z) {
            if (l5var17z.ii + 1 == curProcNesting) {
                addToInsnList(KMTJ+I7 + curProcNesting);
            } else {
                l5var15z = frameRestore[curProcNesting][l5var17z.ii];
                if (l5var15z == (0)) {
                    curVal.ii = 04317L << 36; /* C/ */
                    l5var19z.ii = (curProcNesting + 16) << 30;
                    l5var18z.ii = (l5var17z.ii + 16) << 24;
                    curVal.ii = curVal.ii | l5var19z.ii | l5var18z.ii;
                    l5var15z = allocExtSymbol(extSymMask);
                    frameRestore[curProcNesting][l5var17z.ii] = l5var15z;
                }
                addToInsnList(KVJM+I13 + l5var15z);
            }
        }
    } /* 7176 */
    // base.pas 3481: (not isAssembler) and (not isDirect or [20,21]*calleeFl)
    if (not isAssembler
        and (not l5bool9z or ((Bits(20, 21) & l5var12z.ii) != Bits()))) {
        addToInsnList(KVTM+040074001);
    }
    usedRegs = (usedRegs | l5var12z.ii) & BitRange(1,15);
    if (l5bool10z) {
        if (not checkFortran)
            addToInsnList(KNTR+7);
        else
            addToInsnList(getHelperProc(93));    /* "P/FM" */
        insnList->tail->mode = 2;
    } /* 7226 */
    // NB: base.pas 3486 has no `else` here -- a non-Fortran function returns
    // its value in ACC, so there is no `KXTA+SP` reload of the result.
    if (not l5bool7z) {
        insnList->typ = l5idr5z->typ;
        insnList->regsused = insnList->regsused | Bits(0L);
        insnList->ilm = ilRVAL;
        liveRegs = liveRegs & ~ l5var12z.ii;
    }
    /* 7237 */
} /* genEntry */

void startInsnList(ilmode l5arg1z)
{
    ExprPtr & exprToGen = genFullExpr::super.back()->exprToGen;
    insnList = new InsnList;
    insnList->tail = NULL;
    insnList->head = NULL;
    insnList->typ = exprToGen->vt.typ;
    insnList->regsused = Bits();
    insnList->ilm = l5arg1z;
    if (l5arg1z == ilCONST) {
        insnList->payload.ii = exprToGen->num1;
        insnList->addrmd = exprToGen->num2;
    } else {
        insnList->st = stWORD;
        insnList->addrmd = 18;
        insnList->payload.ii = curFrameRegTemplate;
        insnList->disp = exprToGen->num1;
    }
}

void genCopy()
{
    int64_t size;
    InsnList * lhsIns, * rhsIns;
    int64_t &work = genFullExpr::super.back()->work;
    InsnList * &otherIns = genFullExpr::super.back()->otherIns;

    size = typeSize(insnList->typ);
    if (size == 1) {
        // Merge the rhs-load and lhs-store instruction lists into insnList
        // (base.pas builds the list; the upstream genOneOp version emitted
        // directly and left insnList consumed -> NULL deref at the caller).
        lhsIns = insnList;
        insnList = otherIns;
        prepLoad();
        rhsIns = insnList;
        insnList = lhsIns;
        prepStore();
        lhsIns = insnList;
        if (rhsIns->tail == NULL)
            rhsIns->head = lhsIns->head;
        else
            rhsIns->tail->next = lhsIns->head;
        if (lhsIns->tail != NULL)
            rhsIns->tail = lhsIns->tail;
        rhsIns->regsused = rhsIns->regsused | lhsIns->regsused | Bits(0);
        rhsIns->ilm = ilRVAL;
        insnList = rhsIns;
    } else {
        genFullExpr::super.back()->prepMultiWord();
        genOneOp();
        size = size - 1;
        formAndAlign(KVTM+I13 + getValueOrAllocSymtab(-size));
        work = moduleOffset;
        form2Insn(KUTC+I14 + size, KXTA+I13);
        form3Insn(KUTC+I12 + size, KATX+I13,
                  KVLM+I13 + work);
        usedRegs = usedRegs | BitRange(12,14);
        /* work.p2c does not rebuild insnList here before the caller's
           opfASSN metadata write; keep a placeholder in the host port. */
        insnList = new InsnList;
        insnList->head = NULL;
        insnList->tail = NULL;
        insnList->regsused = Bits();
    }
}

void genComparison()
{
    bool negate;
    int64_t l5set2z;
    int64_t mode, size;

    int64_t &l3int3z = formOperator::super.back()->l3int3z;
    Operator &curOP = genFullExpr::super.back()->curOP;
    bool &arg1Const = genFullExpr::super.back()->arg1Const;
    bool &arg2Const = genFullExpr::super.back()->arg2Const;
    Word &arg1Val = genFullExpr::super.back()->arg1Val;
    Word &arg2Val = genFullExpr::super.back()->arg2Val;
    InsnList * &otherIns = genFullExpr::super.back()->otherIns;
    InsnList * &saved = formOperator::super.back()->saved;
    int64_t &nextInsn = formOperator::super.back()->nextInsn;
    int64_t &work = genFullExpr::super.back()->work;
    TPtr &l2typ13z = programme::super.back()->l2typ13z;

    l3int3z = curOP - NEOP;
    negate = l3int3z & 1;
    if (l3int3z == 6) {     /* IN */
        if (arg1Const) {
            if (arg2Const) {
                insnList->payload.ii = has(arg2Val.ii, arg1Val.ii);
            } else {
                l5set2z = Bits(arg1Val.ii);
                if (l5set2z == Bits()) {
                    insnList->payload.ii = false;
                } else {
                    insnList = otherIns;
                    prepLoad();
                    curVal.ii = l5set2z;
                    addToInsnList(KAAX+I8 + getFCSToffset());
                    insnList->payload.ii = 0;
                    insnList->ilm = ilCOND;
                }
            } /* 7412 */
        } else { /* 7413 */
            saved = insnList;
            insnList = otherIns;
            otherIns = saved;
            nextInsn = 66;      /* P/IN */
            genFullExpr::super.back()->genHelper();
            insnList->ilm = ilRVAL;
        }
    } else { /* 7423 */
        if (negate)
            l3int3z = l3int3z - 1;
        l2typ13z = insnList->typ;
        curVarKind = (Kind)(l2typ13z.p.pk);
        size = typeSize(l2typ13z);
        if (l2typ13z == RealType) {
            work = 1;
        } else if (curVarKind == kindScalar)
            work = 3;
        else {
            work = 4;
        }
        if (size != 1) {
            genFullExpr::super.back()->prepMultiWord();
            addInsnAndOffset(KVTM+I11, 1 - size);
            addToInsnList(getHelperProc(89 + l3int3z)); /* P/EQ */
            insnList->ilm = ilRVAL;
            negate = not negate;
        } else if (l3int3z == 0) {
            nextInsn = InsnTemp[AEX];
            genFullExpr::super.back()->tryFlip(true);
L7504:
            insnList->ilm = ilCOND;
            insnList->payload.ii = 0;
        } else { /* 7510 */
            switch (work) {
            case 1: { /*7513*/
                mode = 3;
L7514:
                nextInsn = InsnTemp[SUB];
                genFullExpr::super.back()->tryFlip(false);
                insnList->tail->mode = mode;
                if (mode == 3) {
                    addToInsnList(KNTR+023);
                    insnList->tail->mode = 2;
                }
                goto L7504;
            } break;
            case 3: { /*7536*/
                mode = 1;
                goto L7514;
            } break;
            case 4: { /*7540*/
                nextInsn = InsnTemp[ARX];
                prepLoad();
                addToInsnList(KAEX+ALLONES);
                genFullExpr::super.back()->tryFlip(true);
                goto L7504;
            } break;
            }; /* case */
        }; /* 7554 */
        insnList->regsused = insnList->regsused & ~ Bits(16);
        if (negate)
            genFullExpr::super.back()->negateCond();
    } /* 7562 */
} /* genComparison */

struct Level {
    int & cnt;
    Level(int & c) : cnt(c) { ++c; }
    ~Level() { if (cnt) --cnt; }
    operator bool() const { return cnt == 1; }
};

genFullExpr::genFullExpr(ExprPtr exprToGen_)
    : exprToGen(exprToGen_)
{
    int64_t &l3int3z = formOperator::super.back()->l3int3z;
    bool &rhsMode = formOperator::super.back()->rhsMode;
    int64_t &nextInsn = formOperator::super.back()->nextInsn;
    OpFlg &flags = formOperator::super.back()->flags;
    InsnList * &saved = formOperator::super.back()->saved;
    IdentRecPtr &curIdRec = programme::super.back()->curIdRec;

    static int level;
    Level l(level);
    
    super.push_back(this);

    if (exprToGen == NULL)
        return;
    exprToGen = cpDsExpr(exprToGen);
L7567:
    if (verbose) {
        if (l) {
            fprintf(stderr, "%ld: %s\n", lineCnt, exprToGen->p().c_str());
        }
    }
    curOP = exprToGen->op;
    if (curOP == CONDOP) {
        genCondOp();
        return;
    }
    if (curOP == RMWASSIGN) {
        genRMWAssign();
        return;
    }
    if (curOP < GETELT) {
        genFullExpr(exprToGen->expr2);
        otherIns = insnList;
        if (curOP == ASSIGNOP)
            rhsMode = false;
        genFullExpr(exprToGen->expr1);
        if (curOP == ASSIGNOP)
            rhsMode = true;
        if (insnList->ilm == ilCONST) {
            arg1Const = true;
            arg1Val = insnList->payload;
        } else
            arg1Const = false;
        if (otherIns->ilm == ilCONST) {
            arg2Const = true;
            arg2Val = otherIns->payload;
        } else
            arg2Const = false;
        if (has((Bits(NEOP) | Bits(EQOP) | Bits(LTOP) | Bits(GEOP) |
             Bits(GTOP) | Bits(LEOP) | Bits(INOP)), curOP)) {
            genComparison();
        } else { /* 7625 */
            if (arg1Const and arg2Const) {
                switch (curOP) {
                case MUL:        arg1Val.r = arg1Val.r * arg2Val.r;
                    break;
                case RDIVOP:     arg1Val.r = arg1Val.r / arg2Val.r;
                    break;
                case ANDOP:      arg1Val.ii = arg1Val.ii and arg2Val.ii;
                    break;
                case IDIVOP:
                case IMODOP:
                case IMULOP:
                case INTPLUS:
                case INTMINUS:
                                 arg1Val = foldRawInt2(curOP, arg1Val, arg2Val);
                    break;
                case PLUSOP:     arg1Val.r = arg1Val.r + arg2Val.r;
                    break;
                case MINUSOP:    arg1Val.r = arg1Val.r - arg2Val.r;
                    break;
                case OROP:       arg1Val.ii = arg1Val.ii or arg2Val.ii;
                    break;
                case SETAND:     arg1Val.ii = arg1Val.ii & arg2Val.ii;
                    break;
                case SETXOR:     arg1Val.ii = arg1Val.ii ^ arg2Val.ii;
                    break;
                case SETOR:      arg1Val.ii = arg1Val.ii | arg2Val.ii;
                    break;
                case SHLEFT:     arg1Val.ii = shl48(arg1Val.ii, arg2Val.ii);
                    break;
                case SHRIGHT:    arg1Val.ii = (arg1Val.ii & BitRange(0, 47)) >> arg2Val.ii;
                    break;
                case NEOP: case EQOP: case LTOP: case GEOP: case GTOP: case LEOP:
                case INOP: case ASSIGNOP:
                    error(200);
                    break;
                default:
                    break;
                } /* case 7750 */
                insnList->payload = arg1Val;
            } else { /*7752*/
                l3int3z = opToMode[curOP];
                flags = opFlags[curOP];
                nextInsn = opToInsn[curOP];
                switch (flags) {
                case opfCOMM:
                    tryFlip(has((Bits(MUL,PLUSOP,SETOR,SETAND) |
                             Bits(INTPLUS,IMULOP)), curOP));
                    break;
                case opfHELP:
                    genHelper();
                    break;
                case opfASSN:
                    genCopy();
                    insnList->typ = exprToGen->vt.typ;
                    insnList->regsused = insnList->regsused | Bits(0);
                    insnList->ilm = ilRVAL;
                    insnList->st = stWORD;
                    return;
                case opfAND:
                    genBoolAnd();
                    return;
                case opfOR:
                    negateCond();
                    saved = insnList;
                    insnList = otherIns;
                    negateCond();
                    otherIns = insnList;
                    insnList = saved;
                    genBoolAnd();
                    negateCond();
                    return;
                case opfMOD:
                    if (arg2Const and arg2Val.ii > 0) {
                        prepLoad();
                        if (card(arg2Val.ii) == 1) {
                            curVal.ii = BitRange(minel(arg2Val.ii)+1, 47);
                            addToInsnList(KAAX+I8 + getFCSToffset());
                            l3int3z = 0;
                        } else {
                            addToInsnList(macro + mcPUSH);
                            genConstDiv();
                            insnList->tail->mode = 1;
                            curVal.ii = arg2Val.ii | Bits(0);
                            addToInsnList(KMUL+I8 + getFCSToffset());
                            addToInsnList(KYTA+64);
                            addToInsnList(KRSUB+SP);
                            l3int3z = 1;
                        }
                    } else {
                        genHelper();
                    }
                    break;
                case opfDIV:
                    if (arg2Const and arg2Val.ii > 0) {
                        prepLoad();
                        genConstDiv();
                        l3int3z = 1;
                    } else
                        genHelper();
                    break;
                case opfMULMSK:
                    if (arg1Const) {
                        insnList->payload.ii = (arg1Val.ii | Bits(0)) & ~ Bits(1, 3);
                    } else if (arg2Const) {
                        otherIns->payload.ii = (arg2Val.ii | Bits(0)) & ~ Bits(1, 3);
                    } else {
                        prepLoad();
                        addToInsnList(KAEX+MSB);
                    }
                    tryFlip(true);
                    insnList->tail->mode = 1;
                    if (fixMult)
                        addToInsnList(macro + mcMULTI);
                    else
                        addToInsnList(KYTA+64);
                    break;
                case opfSHIFT:
                    if (not arg2Const)
                        genHelper();
                    else {
                        prepLoad();
                        if (curOP == SHRIGHT)
                            addToInsnList(ASN64+arg2Val.ii);
                        else
                            addToInsnList(ASN64-arg2Val.ii);
                    }
                    break;
                default:
                    break;
                } /* case 10122 */
L10122:
                insnList->tail->mode = l3int3z;
            }
        }
    } else { /* 10125 */
        if (curOP <= FILEPTR) {
            if (curOP == GETVAR) {
                insnList = new InsnList;
                curIdRec = exprToGen->id1;
                insnList->tail = NULL;
                insnList->head = NULL;
                insnList->regsused = Bits();
                insnList->ilm = ilLVAL;
                insnList->payload.ii = curIdRec->offset;
                insnList->disp = curIdRec->value();
                insnList->st = stWORD;
                insnList->addrmd = 18;
                if (curIdRec->cl == FORMALID) {
                    genDeref();
                } else if (curIdRec->cl == ROUTINEID) {
                    insnList->disp = 3;
                    insnList->payload.ii = (insnList->payload.ii + frameRegTemplate);
                } else if (insnList->disp >= 074000) {
                    addToInsnList(InsnTemp[UTC] + insnList->disp);
                    insnList->disp = 0;
                    insnList->addrmd = 17;
                    insnList->payload.ii = 0;
                }
            } else /* 10171 */
            if (curOP == GETFIELD) {
                genFullExpr(exprToGen->expr1);
                curIdRec = exprToGen->id2;
                insnList->disp = insnList->disp + curIdRec->offset;
                if (curIdRec->pckfield()) {
                    switch (insnList->st) {
                    case stWORD:
                        insnList->shift = curIdRec->shift();
                        break;
                    case stSLICE: {
                        insnList->shift = insnList->shift + curIdRec->shift();
                        if (not has(optSflags.ii, S6))
                            insnList->shift = insnList->shift + typeBits(curIdRec->uptype()) - 48;
                    } break;
                    case stPACKED:
                        if (not rhsMode)
                            error(errUsingVarAfterIndexingPackedArray);
                        else {
                            startLVal();
                            insnList->shift = curIdRec->shift();
                        }
                        break;
                    } /* 10235*/
                    insnList->width = curIdRec->width();
                    insnList->st = stSLICE;
                    insnList->regsused = insnList->regsused | Bits(0L);
                }
            } else /* 10244 */
            if (curOP == GETELT)
                genGetElt();
            else if (curOP == DEREF || curOP == FILEPTR) {
                genFullExpr(exprToGen->expr1);
                genDeref();
            } else if (curOP == op37) {
                startInsnList(ilLVAL);
                genDeref();
            } else if (curOP == GETENUM)
                startInsnList(ilCONST);
        } else if (curOP == STKLVAL) {
            /* Synthetic lvalue produced by genRMWAssign: the real lvalue
               address has been pushed onto the BESM-6 stack twice; each
               STKLVAL visit pops one copy via `WTC SP' into M14. */
            insnList = new InsnList;
            insnList->tail = NULL;
            insnList->head = NULL;
            insnList->typ = exprToGen->vt.typ;
            insnList->regsused = Bits();
            insnList->ilm = ilLVAL;
            insnList->st = stWORD;
            insnList->addrmd = 16;
            insnList->payload.ii = 0;
            insnList->disp = 0;
            insnList->width = 0;
            insnList->shift = 0;
            addToInsnList(KWTC + SP);
        } else if (curOP == ALNUM)
            genEntry();
        else if (has(BitRange(TOREAL, BITNEGOP), curOP)) {
            genFullExpr(exprToGen->expr1);
            if (insnList->ilm == ilCONST) {
                arg1Val = insnList->payload;
                switch (curOP) {
                case TOREAL:
                    arg1Val.r = arg1Val.ii;
                    break;
                case NOTOP: arg1Val.b = not arg1Val.b;
                    break;
                case RNEGOP: arg1Val.r = -arg1Val.r;
                    break;
                case INEGOP: arg1Val = foldRawInt1(curOP, arg1Val);
                    break;
                case BITNEGOP: arg1Val.ii = BitRange(0,47) & ~ arg1Val.ii;
                    break;
                default:
                    break;
                } /* case 10345 */
                insnList->payload = arg1Val;
            } else if (curOP == NOTOP) {
                negateCond();
            } else {
                prepLoad();
                if (curOP == TOREAL) {
                    addToInsnList(KAOX+ZERO);
                    addToInsnList(InsnTemp[AVX]);
                    l3int3z = 3;
                    goto L10122;
                } else if (curOP == BITNEGOP) {
                    addToInsnList(KAEX+ALLONES);
                    l3int3z = 1;
                    goto L10122;
                } else {
                    addToInsnList(KAVX+MINUS1);
                    if (curOP == RNEGOP)
                        l3int3z = 3;
                    else
                        l3int3z = 1;
                    goto L10122;
                }
            }
        } else /* 10376 */
        if (curOP == STANDPROC) {
            genFullExpr(exprToGen->expr1);
            work = exprToGen->num2;
            if (work == fnMALLOC)
                heapCallsCnt = heapCallsCnt + 1;
            if (100 < work) {
                prepLoad();
                addToInsnList(getHelperProc(work - 100));
            } else {
                if (insnList->ilm == ilCONST) {
                    arg1Const = true;
                    arg1Val = insnList->payload;
                } else
                    arg1Const = false;
                arg2Const = (insnList->typ == RealType);
                if (arg1Const) {
                    switch (work) {
                    case fnABS:   arg1Val.r = fabs(arg1Val.r);
                        break;
                    case fnTRUNC: arg1Val.ii = int64_t(trunc(arg1Val.r));
                        break;
                    case fnPTR:   arg1Val.ii = arg1Val.ii & BitRange(7,47);
                        break;
                    case fnROUND: arg1Val.ii = int64_t(round(arg1Val.r));
                        break;
                    case fnCARD:  arg1Val.ii = card(arg1Val.ii);
                        break;
                    case fnMINEL: arg1Val.ii = minel(arg1Val.ii);
                        break;
                    case fnABSI:  arg1Val.ii = labs(arg1Val.ii);
                        break;
                    case fnMALLOC:
                        addToInsnList(KVTM+I14+getValueOrAllocSymtab(arg1Val.ii));
                        addToInsnList(getHelperProc(33)); /*"P/NW"*/
                        insnList->ilm = ilRVAL;
                        insnList->regsused = insnList->regsused | Bits(0);
                        insnList->typ = exprToGen->vt.typ;
                        return;
                    case fnREF:
                        error(201);
                        break;
                    default:
                        break;
                    } /* 10546 */
                    insnList->payload = arg1Val;
                } else if (work == fnREF) {
                    setAddrTo(14);
                    addToInsnList(KITA+14);
                    insnList->ilm = ilRVAL;
                    insnList->regsused = insnList->regsused | Bits(0);
                } else {
                    prepLoad();
                    if (work == fnTRUNC) {
                        l3int3z = 2;
                        addToInsnList(getHelperProc(P_TR));
                        goto L10122;
                    }
                    if (work == fnCARD or work == fnPTR) {
                        l3int3z = 0;
                    } else if (work == fnABS)
                        l3int3z = 3;
                    else {
                        l3int3z = 1;
                    }
                    addToInsnList(funcInsn[work]);
                    goto L10122;
                }
            }
        } else { /* 10621 */
            if (curOP == NOOP) {
                curVal = exprToGen->vt;
                if (has(liveRegs, curVal.ii)) {
                    insnList = new InsnList;
                    insnList->typ = exprToGen->expr2->vt.typ;
                    insnList->tail = NULL;
                    insnList->head = NULL;
                    insnList->regsused = Bits();
                    insnList->ilm = ilLVAL;
                    insnList->addrmd = 18;
                    insnList->payload.ii = indexreg[curVal.ii];
                    insnList->disp = 0;
                    insnList->st = stWORD;
                } else {
                    curVal.ii = 14;
                    exprToGen->vt = curVal;
                    exprToGen = exprToGen->expr2;
                    goto L7567;
                };
                return;
            } else {
                error(220);
            }
        }
    } /* 10654 */
    insnList->typ = exprToGen->vt.typ;
    /* 10656 */
} /* genFullExpr */

void formFileInit()
{
    /* fcloseFile: emit the close sequence (P/61) for one file. */
    auto fcloseFile = [](IdentRecPtr fileSym) {
        int64_t fileAddr = fileSym->value();
        if (fileAddr < 074000) {
            form1Insn(getValueOrAllocSymtab(fileAddr) + InsnTemp[UTC] + I7);
            fileAddr = 0;
        }
        form1Insn(KVTM+I14 + fileAddr);
        form1Insn(KITS+14);
        formAndAlign(getHelperProc(61)); /*"FCLOSE"*/
    };

    if (has(optSflags.ii, S5)) {
        formAndAlign(KUJ+I13);
        return;
    }
    form2Insn(KITS+13, KATX+SP);
    if (inputFile != NULL)
        fcloseFile(inputFile);
    if (outputFile != NULL)
        fcloseFile(outputFile);
    form1Insn(getHelperProc(70)/*"P/IT"*/ + (KUJ-KVJM-I13));
    padToLeft();
} /* formFileInit */

formOperator::formOperator(OpGen op)
{ /* formOperator */
    super.push_back(this);
    rhsMode = true;
    if ((errors and (op != SETREG)) or curExpr == NULL)
        return;
    if (op != FORMOP &&
        op != STOREAT9 &&
        op != DFLTWDTH &&
        op != FILEINIT &&
        op!=PCKUNPCK)
        (void) genFullExpr(curExpr);
    switch (op) {
    case gen0:
        break; /* placeholder OpGen slot, never passed */
    case DOIT:
        genOneOp();
        break;
    case SETREG: {
        l3int3z = insnCount();
        helpExpr = new Expr;
        helpExpr->expr1 = withList;
        withList = helpExpr;
        helpExpr->op = NOOP;
        switch (insnList->st) {
        case stWORD: {
            if (l3int3z == 0)  {
                l3int2z = 14;
            } else {
                l3var10z.ii = auxRegs & freeRegs;
                if (l3var10z.ii != Bits()) {
                    l3int2z = minel(l3var10z.ii);
                } else {
                    l3int2z = 14;
                }
                if (l3int3z != 1) {
                    (void) setAddrTo(l3int2z);
                    addToInsnList(KITA + l3int2z);
                    spillAcc(op37);
                } else if (l3int2z != 14) {
                    (void) setAddrTo(l3int2z);
                    genOneOp();
                }
                l3var11z.ii = Bits(l3int2z) & ~ Bits(14);
                usedRegs = usedRegs & ~ l3var11z.ii;
                freeRegs = freeRegs & ~ l3var11z.ii;
                liveRegs = liveRegs | l3var11z.ii;
            }
            curVal.ii = l3int2z;
            helpExpr->vt = curVal;
        } break;
        case stSLICE: {
            curVal.ii = 14;
            helpExpr->vt = curVal;
        } break;
        case stPACKED:
            error(errVarTooComplex);
            break;
        } /* case */
        helpExpr->expr2 = curExpr;
    } break; /* SETREG */
    case STORE: {
        prepStore();
        genOneOp();
    } break;
    case FORMOP: {
        curInsnTemplate = curVal.ii;
        (void) formOperator(LOAD);
        curInsnTemplate = InsnTemp[XTA];
    } break;
    case SETREG9: {
        if (insnList->st != stWORD)
            error(errVarTooComplex);
        setAddrTo(9);
        genOneOp();
    } break;
    case STOREAT9: {
        l3int1z = curVal.ii;
        (void) genFullExpr(curExpr);
        prepLoad();
        if (has(insnList->regsused, 9))
            error(errVarTooComplex);
        genOneOp();
        form1Insn(KATX+I9 + l3int1z);
    } break;
    case SETREG12: {
        (void) setAddrTo(12);
        genOneOp();
    } break;
    case DFLTWDTH: {
        curVal.ii |= 0xDLL << 44;
        form1Insn(KXTA+I8 + getFCSToffset());
    } break;
    case FRACWIDTH: {
        prepLoad();
        prependToInsnList(macro + mcPUSH);
        genOneOp();
    } break;
    case gen11: case gen12: {
        setAddrTo(11);
        if (op == gen12)
            prependToInsnList(macro + mcPUSH);
        genOneOp();
        usedRegs = usedRegs | Bits(12);
    } break;
    case FILEACCESS: {
        setAddrTo(12);
        genOneOp();
        formAndAlign(jumpTarget);
    } break;
    case FILEINIT:
        formFileInit();
        break;
    case LOAD: {
        prepLoad();
        genOneOp();
    } break;
    case BRANCH:
        noTarget = jumpTarget == 0;
        l3int3z = jumpTarget;
        if (insnList->ilm == ilCONST) {
            if (insnList->payload.ii) {
                jumpTarget = 0;
            } else {
                if (noTarget) {
                    formJump(jumpTarget);
                } else {
                    form1Insn(InsnTemp[UJ] + jumpTarget);
                }
            }
        } else {
            if (curExpr->vt.typ != BooleanType and
                not has((BitRange(SHLEFT, SETOR) |
                     BitRange(GETELT, ALNUM)), curExpr->op))
                addToInsnList(KAEX);
            direction = has(insnList->regsused, 16);
            if ((insnList->ilm == ilCOND) and
                (insnList->payload.ii != 0)) {
                genOneOp();
                if (direction) {
                    if (noTarget)
                        formJump(l3int3z);
                    else
                        form1Insn(InsnTemp[UJ] + l3int3z);
                    fixup(0, jumpTarget);
                    jumpTarget = l3int3z;
                } else {
                    if (not noTarget) {
                        if (not putLeft)
                            padToLeft();
                        fixup(l3int3z, jumpTarget);
                    }
                }
            } else {
                if (insnList->ilm == ilLVAL) {
                    forValue = false;
                    prepLoad();
                    forValue = true;
                }
                genOneOp();
                if (direction)
                    nextInsn = InsnTemp[U1A];
                else
                    nextInsn = InsnTemp[UZA];
                if (noTarget) {
                    jumpType = nextInsn;
                    formJump(l3int3z);
                    jumpType = InsnTemp[UJ];
                    jumpTarget = l3int3z;
                } else {
                    form1Insn(nextInsn + l3int3z);
                }
            }
        }
        break; /* CONDJUMP */
    case PCKUNPCK: {
        helpExpr = curExpr;
        curExpr = curExpr->expr1;
        (void) formOperator(gen11);
        genFullExpr(helpExpr->expr2);
        if (has(insnList->regsused, 11))
            error(44); /* errIncorrectUsageOfStandProcOrFunc */
        setAddrTo(12);
        genOneOp();
        arg1Type = helpExpr->expr2->vt.typ;
        l3int3z = arg1Type.rep()->aright - arg1Type.rep()->aleft + 1;
        form2Insn((KVTM+I14) + l3int3z,
                  (KVTM+I10+64) - arg1Type.rep()->pcksize);
        l3int3z = helpExpr->vt.typ.p.rep;
        l3int1z = arg1Type.rep()->perword;
        if (l3int3z == 72)          /* P/KC */
            l3int1z = 1 - l3int1z;
        form1Insn(getValueOrAllocSymtab(l3int1z) + (KVTM+I9));
        l3int1z = InsnTemp[XTA];
        form1Insn(l3int1z);
        formAndAlign(getHelperProc(l3int3z));
   } break;
   case LITINSN:
        if (insnList->ilm != ilCONST)
            error(errNoConstant);
        if (typeSize(insnList->typ) != 1)
            error(errConstOfOtherTypeNeeded);
        curVal = insnList->payload;
        break;
    } /* case */
} /* formOperator */

void markTypeSym()
{
    if (SY == IDENT) {
        // curVal.ii := curIdent.ii * hashMask.ii; mapAI(curVal.a, bucket);
        bucket = curIdent % 65535 % 128;
        hashTravPtr = symHash[bucket];
        while (hashTravPtr != NULL and hashTravPtr->id != curIdent)
            hashTravPtr = hashTravPtr->next;
        if (hashTravPtr != NULL and hashTravPtr->cl == TYPEID)
            SY = TYPESY;
    }
} /* markTypeSym */

struct parseTypeRef {
    static std::vector<parseTypeRef*> super;
    parseTypeRef(TPtr & newtype, int64_t skipTarget_);
    ~parseTypeRef() { super.pop_back(); }
    typedef std::pair<int64_t, int64_t> pair;
    typedef pair pair7[8]; // array [1..7] of pair;
    typedef struct {
            int64_t size, count;
            pair7 pairs;
    } caserec;
    struct rangeRec { int64_t aleft, aright; };
    typedef rangeRec rangeList[21]; // array [1..20] of rangeRec;

    int64_t skipTarget;
    bool isPacked;
    bool cond;
    caserec cases;
    int64_t numBits, l3int22z, span, rangeCnt, curDim;
    IdentRecPtr curEnum, curField;
    TPtr arrayType{}, nestedType{}, tempType{}, curType{};
    rangeList ranges;
    rangeRec curRange;
    IdentRecPtr l3idr31z;

    void definePtrType(TPtr toType) {
        IdentRecPtr & typelist = programme::super.back()->typelist;
        curType = allocPtr(toType);
        curEnum = new IdentRec(curIdent, lineCnt, typelist, curType, TYPEID);
        typelist = curEnum;
    } /* definePtrType */

    TPtr makeArrayType(rangeRec rg, TPtr elem, bool pckFlag);
};
std::vector<parseTypeRef*> parseTypeRef::super;

struct parseRecordDecl {
    static std::vector<parseRecordDecl*> super;
    parseRecordDecl(TPtr & rectype, bool isOuterDecl_);
    ~parseRecordDecl() { super.pop_back(); }

    bool isOuterDecl;
    TPtr prevVariant{}, selType{}, newVariant{};
    IdentRecPtr prevField;
    Word variantIdx;
    parseTypeRef::caserec cases1, cases2;

    void addFieldToHash() {
        IdentRecPtr &curEnum = parseTypeRef::super.back()->curEnum;
        TPtr &curType = parseTypeRef::super.back()->curType;
        bool &isPacked = parseTypeRef::super.back()->isPacked;
        // curEnum@ := [curIdent, , fieldHash[bucket], ,
        //              FIELDID, NIL, curType, isPacked]
        curEnum->id = curIdent;
        curEnum->next = fieldHash[bucket];
        curEnum->cl = FIELDID;
        curEnum->uptype() = curType;
        curEnum->pckfield() = isPacked;
        fieldHash[bucket] = curEnum;
    } /* addFieldToHash */
};
std::vector<parseRecordDecl*> parseRecordDecl::super;

void packFields()
{
    int64_t fieldWidth, pairIdx, minFirst, scanIdx, curFirst;
    parseTypeRef::pair * curSlot;

    bool &cond = parseTypeRef::super.back()->cond;
    TPtr &curType = parseTypeRef::super.back()->curType;
    IdentRecPtr &curField = parseTypeRef::super.back()->curField;
    IdentRecPtr &l3idr31z = parseTypeRef::super.back()->l3idr31z;
    TPtr &selType = parseRecordDecl::super.back()->selType;
    int64_t &skipTarget = parseTypeRef::super.back()->skipTarget;
    parseTypeRef::caserec &cases = parseTypeRef::super.back()->cases;
    IdentRecPtr &curEnum = parseTypeRef::super.back()->curEnum;
    bool &isPacked = parseTypeRef::super.back()->isPacked;

    parseTypeRef(selType, skipTarget | Bits(UNIONSY));
    if (curType.rep()->fields == NULL) {
        curType.rep()->fields = curField;
    } else {
        l3idr31z->list() = curField;
    }
    l3idr31z = curEnum;
    do {
        curField->typ = selType;
        if (isPacked) {
            fieldWidth = typeBits(selType);
            curField->width() = fieldWidth;
            if (fieldWidth != 48) {
                for (pairIdx = 1; pairIdx <= cases.count; ++pairIdx) {
L11523:             curSlot = &cases.pairs[pairIdx];
                    if (curSlot->first >= fieldWidth) {
                        curField->shift() = 48 - curSlot->first;
                        curField->offset = curSlot->second;
                        if (not has(optSflags.ii, S6))
                            curField->shift() = 48 - curField->width() - curField->shift();
                        curSlot->first = curSlot->first - fieldWidth;
                        if (curSlot->first == 0) {
                            cases.pairs[pairIdx] = cases.pairs[cases.count];
                            cases.count = cases.count - 1;
                        }
                        goto L11622;
                    }
                }
                if (cases.count != 7) {
                    cases.count = cases.count + 1;
                    pairIdx = cases.count;
                } else {
                    minFirst = 48;
                    for (scanIdx = 1; scanIdx <= 7; ++scanIdx) {
                        curFirst = cases.pairs[scanIdx].first;
                        if (curFirst < minFirst) {
                            minFirst = curFirst;
                            pairIdx = scanIdx;
                        }
                    } /* for */
                }
                cases.pairs[pairIdx] = std::make_pair(48, cases.size);
                cases.size = cases.size + 1;
                goto L11523;
            }
        }
        curField->pckfield() = false;
        curField->offset = cases.size;
        cases.size = cases.size + typeSize(selType);
L11622:
        if (PASINFOR.listMode == 3) {
            printf("%16c", ' ');
            if (curField->pckfield())
                printf("PACKED");
            printf(" FIELD ");
            printTextWord(curField->id);
            printf(".OFFSET=%05loB", curField->offset);
            if (curField->pckfield()) {
                printf(".<<=SHIFT=%2ld. WIDTH=%2ld BITS", curField->shift(),
                       curField->width());
            } else {
                printf(".WORDS=%ld", typeSize(selType));
            }
            putchar('\n');
        }
        cond = (curField == curEnum);
        curField = curField->list();
    } while (!cond);
} /* packFields */

parseRecordDecl::parseRecordDecl(TPtr & rectype, bool isOuterDecl_)
    : isOuterDecl(isOuterDecl_)
{
    bool &cond = parseTypeRef::super.back()->cond;
    TPtr &curType = parseTypeRef::super.back()->curType;
    TPtr &tempType = parseTypeRef::super.back()->tempType;
    IdentRecPtr &curField = parseTypeRef::super.back()->curField;
    IdentRecPtr &curEnum = parseTypeRef::super.back()->curEnum;
    bool &isPacked = parseTypeRef::super.back()->isPacked;
    parseTypeRef::caserec &cases = parseTypeRef::super.back()->cases;

    super.push_back(this);

    if (SY != BEGINSY)
        requiredSymErr(BEGINSY);
    lookupMode = lookField;
    inSymbol();

    while (SY == IDENT) {
        prevField = NULL;
        do {
            if (SY != IDENT) {
                error(errNoIdent);
            } else {
                if (hashTravPtr != NULL)
                    error(errIdentAlreadyDefined);
                curEnum = new IdentRec;
                addFieldToHash();
                if (prevField == NULL) {
                    curField = curEnum;
                } else {
                    prevField->list() = curEnum;
                }
                prevField = curEnum;
                lookupMode = lookField;
                inSymbol();
            }
            cond = (SY != COMMA);
            if (not cond) {
                lookupMode = lookField;
                inSymbol();
            }
        } while (!cond);
        checkSymAndRead(COLON);
        packFields();
        if (SY == SEMICOLON) {
            lookupMode = lookField;
            inSymbol();
        }
    }
    if (SY == UNIONSY) {
        lookupMode = lookField;
        inSymbol();
        if (SY != BEGINSY)
            requiredSymErr(BEGINSY);
        lookupMode = lookField;
        inSymbol();
        cases1 = cases;
        cases2 = cases;
        prevVariant.setRep(NULL);
        variantIdx.ii = 0;
        while (SY == STRUCTSY) {
            lookupMode = lookField;
            inSymbol();
            newVariant.setRep(new Types);
            newVariant.rep()->first = variantIdx.typ;
            newVariant.rep()->next.setRep(NULL);
            newVariant.rep()->alt.setRep(NULL);
            newVariant.p.psize = cases.size;
            newVariant.p.bits = 48;
            newVariant.p.pk = kindCases;
            if (prevVariant == NULL) {
                if (curType.rep()->variants == NULL) {
                    curType.rep()->variants = newVariant;
                } else {
                    rectype.rep()->first = newVariant;
                }
            } else {
                prevVariant.rep()->next = newVariant;
            }
            prevVariant = newVariant;
            tempType = newVariant;
            parseRecordDecl(tempType, false);
            if ((cases2.size < cases.size) or
                (isPacked and (cases.size == 1) and (cases2.size == 1) and
                 (cases.count == 1) and (cases2.count == 1) and
                 (cases.pairs[1].first < cases2.pairs[1].first))) {
                cases2 = cases;
            }
            cases = cases1;
            variantIdx.ii = variantIdx.ii + 1;
            if (SY == SEMICOLON) {
                lookupMode = lookField;
                inSymbol();
            }
        }
        cases = cases2;
        if (SY != ENDSY)
            requiredSymErr(ENDSY);
        lookupMode = lookField;
        inSymbol();
        if (SY == SEMICOLON) {
            lookupMode = lookField;
            inSymbol();
        }
    }
    rectype.p.psize = cases.size;
    if (isPacked and (cases.size == 1) and (cases.count == 1)) {
        rectype.p.bits = 48 - cases.pairs[1].first;
    } else {
        rectype.p.bits = 48;
    }
    if (rectype.p.pk == kindStruct) {
        prevField = rectype.rep()->fields;
        while (prevField != NULL) {
            prevField->uptype() = rectype;
            prevField = prevField->list();
        }
    }
    checkSymAndRead(ENDSY);
} /* parseRecordDecl */

void parseRange(int64_t & aleft, int64_t & aright)
{
    TPtr tempType{};
    parseLiteral(tempType, curVal, true);
    if (tempType != NULL and tempType.p.pk == kindScalar) {
        inSymbol();
        if (SY != COLON) {
            // Handle a single value N as a range 0..N-1
            aright = curVal.ii - 1;
            aleft = 0;
            return;
        }
        aleft = curVal.ii;
        inSymbol();
        parseLiteral(tempType, curVal, true);
        inSymbol();
        if (tempType != NULL and tempType.p.pk == kindScalar) {
            aright = curVal.ii;
            return;
        }
    }
    error(64); /* errIncorrectRangeDefinition */
    aleft = 0;
    aright = 0;
} /* parseRange */

TPtr parseTypeRef::makeArrayType(rangeRec rg, TPtr elem, bool pckFlag)
{
    bool makePacked;
    int64_t sizeVal, bitsVal, perwordVal, pcksizeVal;

    makePacked = pckFlag;
    span = rg.aright - rg.aleft + 1;
    l3int22z = typeBits(elem);
    if (24 < l3int22z)
        makePacked = false;
    bitsVal = 48;
    perwordVal = 0;
    pcksizeVal = 0;
    if (makePacked) {
        l3int22z = 48 / l3int22z;
        if (l3int22z == 9) {
            l3int22z = 8;
        } else if (l3int22z == 5) {
            l3int22z = 4;
        }
        perwordVal = l3int22z;
        pcksizeVal = 48 / l3int22z;
        l3int22z = span * pcksizeVal;
        if (l3int22z % 48 == 0)
            numBits = 0;
        else
            numBits = 1;
        sizeVal = l3int22z / 48 + numBits;
        if (sizeVal == 1)
            bitsVal = l3int22z;
    } else {
        sizeVal = span * typeSize(elem);
        curVal.ii = typeSize(elem);
        curVal.ii = (curVal.ii & BitRange(7,47)) | Bits(0);
        perwordVal = KMUL+ I8 + getFCSToffset();
    }
    arrayType.setRep(new Types);
    arrayType.rep()->aleft = rg.aleft;
    arrayType.rep()->aright = rg.aright;
    arrayType.rep()->base = elem;
    arrayType.rep()->pck = makePacked;
    arrayType.rep()->perword = perwordVal;
    arrayType.rep()->pcksize = pcksizeVal;
    arrayType.p.psize = sizeVal;
    arrayType.p.bits = bitsVal;
    arrayType.p.pk = kindArray;
    return arrayType;
} /* makeArrayType */

parseTypeRef::parseTypeRef(TPtr & newtype, int64_t skipTarget_)
    : skipTarget(skipTarget_)
{
    bool &inTypeDef = programme::super.back()->inTypeDef;
    super.push_back(this);
    isPacked = false;
L12247:
    if (SY == IDENT)
        markTypeSym();
    if (SY == ENUMSY) {
        inSymbol();
        checkSymAndRead(BEGINSY);
        span = 0;
        lookupMode = lookDef;
        curField = NULL;
        curType.setRep(new Types);
        while (SY == IDENT) {
            if (isDefined)
                error(errIdentAlreadyDefined);
            curEnum = new IdentRec(curIdent, curFrameRegTemplate,
                                   symHash[bucket], curType,
                                   ENUMID, NULL, span);
            symHash[bucket] = curEnum;
            span = span + 1;
            if (curField == NULL) {
                curType.rep()->enums = curEnum;
            } else {
                curField->list() = curEnum;
            }
            curField = curEnum;
            inSymbol();
            if (SY == COMMA) {
                lookupMode = lookDef;
                inSymbol();
            } else {
                if (SY != ENDSY)
                    requiredSymErr(ENDSY);
            }
        }
        checkSymAndRead(ENDSY);
        if (curField == NULL) {
            curType = BooleanType;
            error(errNoIdent);
        } else {
            curType.rep()->numen = span;
            curType.rep()->start = 0;
            curType.p.psize = 1;
            curType.p.bits = nrOfBits(span - 1);
            curType.p.pk = kindScalar;
            curEnum = curType.rep()->enums;
            while (curEnum != NULL) {
                curEnum->typ = curType;
                curEnum = curEnum->list();
            }
        }
    } else
    if (charClass == MUL) {
        inSymbol();
        if (not (SY == IDENT or SY == TYPESY)) {
            error(errNoIdent);
            curType = voidPtr;
        } else {
            if (SY == TYPESY) {
                curType = getPtrType(hashTravPtr->typ);
            } else if (hashTravPtr == NULL) {
                if (inTypeDef) {
                    if (knownInType(curEnum)) {
                        curType = curEnum->typ;
                    } else {
                        definePtrType(IntegerType);
                    }
                } else {
L12366:             error(errNotAType);
                    curType = voidPtr;
                }
            } else
                goto L12366;
            inSymbol();
        }
    } else if (SY == IDENT or SY == TYPESY) {
        if (SY == TYPESY) {
            curType = hashTravPtr->typ;
        } else
            goto L12366;
        inSymbol();
        if (curType == IntegerType and SY == COLON) {
            inSymbol();
            if (SY != INTCONST)
                error(errNumberTooLarge);
            else {
                l3int22z = curToken.ii;
                inSymbol();
                curType = mkIntScl(l3int22z);
            }
        }
    } else {
        if (SY == PACKEDSY) {
            isPacked = true;
            inSymbol();
            goto L12247;
        }
        if (SY == STRUCTSY) {
            curType.setRep(new Types);
            typ121z = curType;
            curType.rep()->variants.setRep(NULL);
            curType.rep()->fields = NULL;
            curType.rep()->flag = false;
            curType.rep()->pckrec = isPacked;
            curType.p.psize = 0;
            curType.p.bits = 48;
            curType.p.pk = kindStruct;
            cases.size = 0;
            cases.count = 0;
            inSymbol();
            parseRecordDecl(curType, true);
        } else {
            error(errNotAType);
        }
    }
    tempType = curType;
    rangeCnt = 0;
    while (SY == LBRACK) {
        inSymbol();
        parseRange(curRange.aleft, curRange.aright);
        if (rangeCnt == 20) {
            error(errVarTooComplex);
        } else {
            rangeCnt = rangeCnt + 1;
            ranges[rangeCnt] = curRange;
        }
        checkSymAndRead(RBRACK);
    }
    curType = tempType;
    for (curDim = rangeCnt; curDim >= 1; --curDim) {
        curType = makeArrayType(ranges[curDim], curType,
                                isPacked and (curDim == 1));
    }
    if (rangeCnt != 0)
        isPacked = false;
/* L13020: */
    if (errors)
        skip(skipToSet | Bits(RPAREN, RBRACK, SEMICOLON));
    newtype = curType;
} /* parseTypeRef */

void dumpEnumNames(TPtr l3arg1z)
{
    IdentRecPtr l3var1z;
    if (l3arg1z.rep()->start == 0) {
        l3arg1z.rep()->start = FcstCnt;
        l3var1z = l3arg1z.rep()->enums;
        while (l3var1z != NULL) {
            curVal.ii = l3var1z->id;
            l3var1z = l3var1z->list();
            toFCST();
        }
    }
} /* dumpEnumNames */

void fopenFile(IdentRecPtr fileSym, ExtFileRec * extFileP)
{
    int64_t fileAddr;
    // fileBase := fileSym@.typ.rep@.base and elemSize := fileSym@.typ.p.pad
    // are computed but never used in base.pas; omitted here.
    fileAddr = fileSym->value();
    if (fileAddr < 074000) {
        form1Insn(getValueOrAllocSymtab(fileAddr) +
                  InsnTemp[UTC] + I7);
        fileAddr = 0;
    }
    form1Insn(KVTM+I14 + fileAddr);
    form1Insn(KITS+14);
    // The only files opened this way are *INPUT* and *OUTPUT*
    // with known characteristics (1 word, 8 bits).
    curVal.ii = fileBufSize * 010000000000L + 0100010;
    form1Insn(KXTS+I8 + getFCSToffset());
    if (extFileP == NULL) {
        form1Insn(KXTS);
    } else {
        curVal.ii = extFileP->location;
        if (curVal.ii == 512)
            // offset holds a packed file name (e.g. "*OUTPUT*"), not a number.
            curVal.ii = extFileP->offset;
        form1Insn(KXTS+I8 + getFCSToffset());
    }
    formAndAlign(getHelperProc(60)); /*"FOPEN"*/
} /* fopenFile */

void parseDecls(int64_t l3arg1z)
{
    int64_t l3int1z;
    Word frame;
    bool l3var3z;

    IdentRecPtr &procName = programme::super.back()->procName;
    IdentRecPtr &curIdRec = programme::super.back()->curIdRec;
    int64_t &l2var12z = programme::super.back()->l2var12z;
    int64_t &hasFiles = programme::super.back()->hasFiles;

    switch (l3arg1z) {
    case 0: {
        lookupMode = lookDef;
        inSymbol();
        if (SY != IDENT)
            errAndSkip(3, skipToSet | Bits(IDENT));
    } break;
    case 1: {
        prevErrPos = 0;
        printf("IDENT ");
        printTextWord(l2var12z);
        printf(" IN LINE %ld", curIdRec->offset);
    } break;
    case 2: {
        padToLeft();
        l3var3z = has(procName->flags(), 22);
        l3arg1z = procName->pos();
        frame.ii = moduleOffset - 040000;
        if (l3arg1z != 0)
            symTab[l3arg1z] = 041000000 + (frame.ii & halfWord);
        procName->pos() = moduleOffset;
        l3arg1z = argCount(procName);
        if (l3var3z) {
            if (41 >= entryPtCnt) {
                entryPtTable[entryPtCnt] = leftAlign(procName->id);
                // [1] + frame.ii - [0, 3]
                entryPtTable[entryPtCnt+1] = (1L << 46) | frame.ii;
                entryPtCnt = entryPtCnt + 2;
            } else
                error(87); /* errTooManyEntryProcs */
        }
        if (procName->typ == voidType) {
            frame.ii = 3;
        } else {
            frame.ii = 4;
        }
        if (l3var3z)
            form2Insn((KVTM+I14) + l3arg1z + (frame.ii - 3) * 01000,
                      getHelperProc(94 /*"P/NN"*/) - 010000000);
        if (1 < l3arg1z) {
            frame.ii = getValueOrAllocSymtab(-(frame.ii+l3arg1z));
        }
        if (has(optSflags.ii, S5) and
            curProcNesting == 1)
            l3int1z = 59;  /* P/LV */
        else
            l3int1z = curProcNesting;
        l3int1z = getHelperProc(l3int1z) - (-04000000);
        if (l3arg1z == 1) {
            form1Insn((KATX+SP) + frame.ii);
        } else if (l3arg1z != 0) {
            form2Insn(KATX+SP, (KUTM+SP) + frame.ii);
        }
        formAndAlign(l3int1z);
        savedObjIdx = objBufIdx;
        if (curProcNesting != 1)
            form1Insn(0);
        if (l3var3z)
            form1Insn(KVTM+I8+074001);
        if (hasFiles != 0) {
            if (inputFile != NULL)
                fopenFile(inputFile, fileForInput);
            if (outputFile != NULL)
                fopenFile(outputFile, fileForOutput);
            curVal.ii = hasFiles;
            fixup(2, 49);
        }
        if (curProcNesting == 1) {
            if (heapCallsCnt != 0 and
                heapSize == 0)
                error(65 /*errCannotHaveK0AndNew*/);
            l3var3z = (heapSize == 0) or
                ((heapCallsCnt == 0) and (heapSize == 100));
            if (heapSize == 100)
                heapSize = 4;
            if (not l3var3z) {
                form2Insn(KVTM+I14+getValueOrAllocSymtab(heapSize*02000),
                          getHelperProc(26 /*"P/GD"*/));
                padToLeft();
            }
        }
    } break;
    } /* case */
} /* parseDecls */

void labCheckAndDefine(bool isDef)
{
    int64_t labIdx;
    int64_t &labFence = programme::super.back()->labFence;

    labIdx = numLabTop;
    while (labIdx > labFence and numLabs[labIdx].id != curToken)
        labIdx = labIdx - 1;
    if (labIdx == labFence) {
        if (numLabTop >= 20) {
            error(50); /* errSymbolTableOverflow */
            return;
        }
        numLabTop = numLabTop + 1;
        numLabs[numLabTop].id = curToken;
        numLabs[numLabTop].offset = 0;
        numLabs[numLabTop].line = lineCnt;
        numLabs[numLabTop].defined = false;
        labIdx = numLabTop;
    }
    if (isDef) {
        if (numLabs[labIdx].defined) {
            errLine = numLabs[labIdx].line;
            error(17); /* errLblAlreadyDefinedInLine */
            return;
        }
        numLabs[labIdx].line = lineCnt;
        numLabs[labIdx].defined = true;
        if (numLabs[labIdx].offset == 0) {
            /* empty */
        } else if (numLabs[labIdx].offset >= 074000) {
            // symTab[offset] := [24,29] + curVal.ii * O77777
            curVal.ii = moduleOffset - 040000;
            symTab[numLabs[labIdx].offset] = 041000000 + (curVal.ii & 077777);
        } else {
            fixup(0, numLabs[labIdx].offset);
        }
        numLabs[labIdx].offset = moduleOffset;
    } else {
        if (numLabs[labIdx].offset >= 040000) {
            form1Insn(InsnTemp[UJ] + numLabs[labIdx].offset);
        } else {
            formJump(numLabs[labIdx].offset);
        }
    }
} /* labCheckAndDefine */

struct Statement {
    static std::vector<Statement*> super;
    Statement();
    ~Statement() { super.pop_back(); }

    ExprPtr boundary;
    StrLabel * strLabPtr;
    bool nest;
    bool flag;
    IdClass l3var6z;
    Word curOffset;
    int64_t startLine;
    int64_t ifWhlTarget, elseJump;
    ExprPtr whileExpr;
    IdentRecPtr l3idr12z;
};

std::vector<Statement*> Statement::super;

bool isCharArray(TPtr arg)
{
    return arg.p.pk == kindArray and arg.rep()->base == CharType;
} /* isCharArray */

void expression();

/* parsePostfix: consume any chain of postfix operators (@, .field, [idx])
   acting on curExpr.  Returns with SY pointing at the first token that is
   neither a postfix operator nor the trailing `]` of an index list.  Safe
   to call when the next token isn't a postfix at all (loop simply exits). */
void parsePostfix()
{
    ExprPtr l4exp1z;
    TPtr l4typ3z, l4typ5z;
    Kind l4var4z;
L13462:
    l4typ3z = curExpr->vt.typ;
    l4var4z = (Kind)l4typ3z.p.pk;
    if (SY == ARROW) {
        /* '->' is deref + struct field selection; build DEREF here,
           then jump to label 55 to consume the field IDENT.  The pointee
           goes through l4typ5z: field selection on a function result is
           not supported by the language (mirrors work.p2c). */
        l4exp1z = new Expr;
        l4exp1z->expr1 = curExpr;
        l4typ5z = l4var4z == kindPtr ? ptrBase(l4typ3z) : l4typ3z;
        if (l4var4z == kindPtr and
            l4typ5z.p.pk == kindStruct) {
            l4exp1z->vt.typ = l4typ5z;
            l4exp1z->op = DEREF;
            curExpr = l4exp1z;
            l4typ3z = l4typ5z;
            goto L55;
        } else {
            stmtName = "  ->  ";
            error(errWrongVarTypeBefore);
            l4exp1z->vt.typ = l4typ3z;
        }
        curExpr = l4exp1z;
        inSymbol();
    } else if (SY == PERIOD) {
        if (l4var4z == kindStruct) {
L55:        lookupMode = lookField;
            typ121z = l4typ3z;
            inSymbol();
            if (hashTravPtr == NULL) {
                error(20); /* errDigitGreaterThan7 ??? */
            } else {
                curExpr = mkExpr(GETFIELD, hashTravPtr->typ,
                                 curExpr, (ExprPtr)hashTravPtr);
            }
            inSymbol();
        } else {
            stmtName = "  .   ";
            error(errWrongVarTypeBefore);
            return;
        }
    } else if (SY == LBRACK) {
        stmtName = "  [   ";
        l4exp1z = curExpr;
        expression();
        l4typ3z = l4exp1z->vt.typ;
        if (isCharPtr(l4typ3z))
            curExpr = flatMemAt(mkExpr(INTPLUS, charPtrType,
                                       l4exp1z, curExpr));
        else if (l4typ3z.p.pk != kindArray) {
            error(errWrongVarTypeBefore);
        } else {
            l4exp1z = mkExpr(GETELT, l4typ3z.rep()->base,
                             l4exp1z, curExpr);
            curExpr = l4exp1z;
        }
        if (SY != RBRACK)
            error(67 /*errNeedBracketAfterIndices*/);
        inSymbol();
    } else return;
    goto L13462;
} /* parsePostfix */

void parseLval()
{
    if (hashTravPtr->cl == FIELDID) {
        /* Implicit field of the `with` variable: build GETFIELD on
           withIter directly, then continue with any further postfix. */
        curExpr = mkExpr(GETFIELD, hashTravPtr->typ,
                         withIter, (ExprPtr)hashTravPtr);
    } else {
        curExpr = mkExpr(GETVAR, hashTravPtr->typ,
                         (ExprPtr)hashTravPtr, NULL);
    }
    inSymbol();
    parsePostfix();
} /* parseLval */

void castToReal(ExprPtr & value)
{
    value = mkExpr(TOREAL, RealType, value, NULL);
} /* castToReal */

bool areTypesCompatible(ExprPtr & other)
{
    if (arg1Type == RealType) {
        if (typeCheck(IntegerType, arg2Type)) {
            castToReal(other);
            return true;
        }
    } else if (arg2Type == RealType and
               typeCheck(IntegerType, arg1Type)) {
        castToReal(curExpr);
        return true;
    }
    return false;
} /* areTypesCompatible */

void parseCallArgs(IdentRecPtr subroutine)
{
    bool noArgs;
    ExprPtr curActual, callExpr, argList;
    IdentRecPtr curFormal;
    Operator actualOp;
    IdClass formClass;

    if (subroutine->typ != voidType)
        liveRegs = liveRegs & ~ subroutine->flags();
    noArgs = (subroutine->list() == NULL) and not has(subroutine->flags(), 24);
    callExpr = new Expr;
    argList = callExpr;
    bool48z = true;
    callExpr->vt.typ = subroutine->typ;
    callExpr->op = ALNUM;
    callExpr->id2 = subroutine;
    callExpr->id1 = NULL;
    if (SY == LPAREN) {
        if (noArgs) {
            curFormal = subroutine->argList();
            if (curFormal == NULL) {
                inSymbol();
                if (SY != RPAREN) {
                    error(errTooManyArguments);
                    throw 8888;
                }
                curExpr = callExpr;
                inSymbol();
                return;
            }
        }
        do {
            if (noArgs and subroutine == curFormal) {
                error(errTooManyArguments);
                throw 8888;
            }
            inCallArgs = true;
            expression();
            actualOp = curExpr->op;
            if (noArgs) { /*(a)*/
                formClass = curFormal->cl;
                if (actualOp == PCALL) {
                    if (formClass != ROUTINEID or
                        curFormal->typ != voidType) {
L13736:                 error(39); /*errIncompatibleArgumentKinds*/
                        goto exit_a;
                    }
                } else {
                    if (actualOp == FCALL) {
                        if (formClass == ROUTINEID) {
                            if (curFormal->typ == voidType)
                                goto L13736;
                        } else
                        if (curExpr->id2->argList() == NULL and
                            formClass == VARID) {
                            curExpr->op = ALNUM;
                            curExpr->expr1 = NULL;
                        } else
                            goto L13736;
                    } else
                    if (has(lvalOpSet, actualOp)) {
                        if (formClass != VARID and
                            formClass != FORMALID)
                            goto L13736;
                    } else {
                        if (formClass != VARID)
                            goto L13736;
                    }
                }
                arg1Type = curExpr->vt.typ;
                if (arg1Type != voidType) {
                    if (not typeCheck(arg1Type, curFormal->typ))
                        error(40); /*errIncompatibleArgumentTypes*/
                }
            }
exit_a:
            curActual = new Expr;
            curActual->vt.typ.setRep(NULL);
            curActual->expr1 = NULL;
            curActual->expr2 = curExpr;
            argList->expr1 = curActual;
            argList = curActual;
            if (noArgs)
                curFormal = curFormal->list();
        } while (SY == COMMA);
        if ((SY != RPAREN) or
            (noArgs and (curFormal != subroutine)))
            error(errNoCommaOrParenOrTooFewArgs);
        else
            inSymbol();
    } else {
        error(42); /*errNoArgList*/
    }
    curExpr = callExpr;
} /* parseCallArgs */

int64_t getPrec(Symbol sym, Operator cls)
{
    if (sym == EXPROP)
        return opPrec[cls];
    else if (sym == BECOMES)
        return precAssign;
    else
        return precNone;
} /* getPrec */

void bldBitOp(Operator oper, ExprPtr leftArg)
{
    if (arg1Type.p.pk != kindScalar
        or arg2Type.p.pk != kindScalar) {
        error(errNeedOtherTypesOfOperands);
        return;
    }
    curExpr = mkExpr(oper, arg1Type, leftArg, curExpr);
} /* bldBitOp */

void bldArithOp(Operator oper, ExprPtr leftExpr, [[maybe_unused]] bool match)
{
    Kind k1, k2;
    Operator resOp;
    TPtr resTyp;

    k1 = (Kind)arg1Type.p.pk;
    k2 = (Kind)arg2Type.p.pk;
    if (isCharPtr(arg1Type) and typeCheck(IntegerType, arg2Type)) {
        resOp = intOpMap[oper];
        resTyp = charPtrType;
        curExpr = mkExpr(resOp, resTyp, leftExpr, curExpr);
        return;
    }
    if (isCharPtr(arg2Type) and typeCheck(IntegerType, arg1Type)) {
        resOp = intOpMap[oper];
        resTyp = charPtrType;
        curExpr = mkExpr(resOp, resTyp, curExpr, leftExpr);
        return;
    }
    if (k1 > kindScalar or k2 > kindScalar) {
        error(errNeedOtherTypesOfOperands);
        return;
    }
    if (k1 == kindReal or k2 == kindReal) {
        if (oper == IMODOP) {
            error(62); /* errIntegerNeeded */
            return;
        }
        if (k1 != kindReal)
            castToReal(curExpr);
        if (k2 != kindReal)
            castToReal(leftExpr);
        resOp = oper;
        resTyp = RealType;
    } else {
        resOp = intOpMap[oper];
        resTyp = IntegerType;
    }
    curExpr = mkExpr(resOp, resTyp, leftExpr, curExpr);
} /* bldArithOp */

void bldRelOp(Operator oper, ExprPtr ex2)
{
    Operator resOp;

    if (typeCheck(arg1Type, arg2Type)) {
        if ((typeSize(arg1Type) != 1) and
            (oper >= LTOP) and
            not isCharArray(arg1Type))
            error(errNeedOtherTypesOfOperands);
    } else {
        if (not areTypesCompatible(ex2) and
            ((arg1Type != IntegerType) or
             (arg2Type.p.pk != kindScalar) or
             (oper != INOP))) {
            error(errNeedOtherTypesOfOperands);
        }
    }
    if (oper == GTOP or oper == LEOP) {
        if (oper == GTOP)
            resOp = LTOP;
        else
            resOp = GEOP;
        curExpr = mkExpr(resOp, BooleanType, curExpr, ex2);
    } else
        curExpr = mkExpr(oper, BooleanType, ex2, curExpr);
} /* bldRelOp */

void bldLogOp(Operator oper, ExprPtr leftExpr, bool match)
{
    if ((not match) or
        ((arg1Type != BooleanType) and (arg1Type != IntegerType)))
        error(errNeedOtherTypesOfOperands);
    else
        curExpr = mkExpr(oper, BooleanType, leftExpr, curExpr);
} /* bldLogOp */

void bldCondOp(ExprPtr condExpr, ExprPtr thenExpr)
{
    TPtr resType;
    ExprPtr altExpr;

    if (condExpr->vt.typ.p.pk > kindPtr) {
        error(errBooleanNeeded);
        return;
    }
    arg1Type = thenExpr->vt.typ;
    arg2Type = curExpr->vt.typ;
    if (not typeCheck(arg1Type, arg2Type)) {
        error(errNeedOtherTypesOfOperands);
        return;
    }
    resType = arg1Type;
    if (typeSize(resType) != 1) {
        error(errNeedOtherTypesOfOperands);
        return;
    }
    altExpr = mkExpr(ALTERN, resType, thenExpr, curExpr);
    curExpr = mkExpr(CONDOP, resType, condExpr, altExpr);
} /* bldCondOp */

struct Factor {
    static std::vector<Factor*> super;
    Factor();
    ~Factor() { super.pop_back(); }

    Word l4var1z;
    bool wasInCall;
    Word l4var3z, l4var4z;
    ExprPtr l4exp5z, newExpr, l4var7z, l4var8z;
    IdentRecPtr routine;
    Operator newOp;
    TPtr l4typ11z{};
    bool l4var12z;

    void stdCall();
};
std::vector<Factor*> Factor::super;

void Factor::stdCall()
{
    const int64_t chkREAL = 0, chkINT    = 1, chkCHAR = 2, chkSCALAR = 3,
                  chkPTR  = 4, chkFILE   = 5, /* chkSET = 6, */ chkOTHER = 7;
    TPtr l5var2z{};
    Kind argKind;
    int64_t asint64_t;
    int64_t stProcNo, checkMode, resultValue;

    curVal.ii = routine->low();
    stProcNo = curVal.ii;
    if (SY != LPAREN) {
        requiredSymErr(LPAREN);
        throw 8888;
    }
    if (stProcNo == fnSIZEOF or stProcNo == fnOFFSETOF) {
        lookupMode = lookUse;
        inSymbol();
        if (SY == TYPESY) {
            l5var2z = hashTravPtr->typ;
            inSymbol();
        } else {
            if (stProcNo == fnSIZEOF) {
                readNext = false;
                expression();
                l5var2z = curExpr->vt.typ;
            } else {
                error(errNotAType);
                l5var2z = IntegerType;
                if (SY == IDENT)
                    inSymbol();
            }
        }
        if (stProcNo == fnOFFSETOF) {
            if (l5var2z.p.pk != kindStruct)
                error(errWrongVarTypeBefore);
            if (SY != COMMA)
                requiredSymErr(COMMA);
            else {
                typ121z = l5var2z;
                lookupMode = lookField;
                inSymbol();
            }
            if (SY != IDENT) {
                error(errNoIdent);
                resultValue = 0;
            } else {
                if (hashTravPtr == NULL) {
                    error(errNotDefined);
                    resultValue = 0;
                } else {
                    resultValue = hashTravPtr->offset;
                }
                inSymbol();
            }
        } else {
            resultValue = typeSize(l5var2z);
        }
        curExpr = mkIntLit(resultValue);
        checkSymAndRead(RPAREN);
        return;
    }
    expression();
    if (stProcNo == fnREF and
        not (GETELT <= curExpr->op and curExpr->op <= DEREF)) {
        error(27); /* errExpressionWhereVariableExpected */
        return;
    }
    arg1Type = curExpr->vt.typ;
    argKind = (Kind)arg1Type.p.pk;
    if (arg1Type == RealType)
        checkMode = chkREAL;
    else if (arg1Type == IntegerType)
        checkMode = chkINT;
    else if (arg1Type == CharType)
        checkMode = chkCHAR;
    else if (argKind == kindScalar)
        checkMode = chkSCALAR;
    else if (argKind == kindPtr)
        checkMode = chkPTR;
    else if (typeSize(arg1Type) == 30)
        checkMode = chkFILE;
    else {
        checkMode = chkOTHER;
    }
    asint64_t = Bits(stProcNo);
    if (stProcNo != fnSIZEOF and
        not (((checkMode == chkREAL) and
              (subset(asint64_t, (BitRange(fnABS,fnTRUNC) | Bits(fnREF, fnROUND)))))
          or ((checkMode == chkINT) and
              (subset(asint64_t, (Bits(fnABS,fnMALLOC,fnREF,fnCARD) |
                           Bits(fnMINEL,fnPTR)))))
          or ((checkMode == chkCHAR or checkMode == chkSCALAR or
               checkMode == chkPTR) and
              (subset(asint64_t, Bits(fnREF))))
          or ((checkMode == chkFILE) and
              (subset(asint64_t, Bits(fnREF))))
          or ((checkMode == chkOTHER) and
              (stProcNo == fnREF))))
        error(errNeedOtherTypesOfOperands);
    if (not (subset(asint64_t, Bits(fnABS, fnSIZEOF)))) {
        arg1Type = routine->typ;
    } else if (checkMode == chkINT and subset(asint64_t, Bits(fnABS))) {
        stProcNo = fnABSI;
    }
    if (stProcNo == fnSIZEOF)
        curExpr = mkIntLit(typeSize(arg1Type));
    else
        curExpr = mkExpr(STANDPROC, arg1Type, curExpr, (ExprPtr)stProcNo);
    checkSymAndRead(RPAREN);
} /* stdCall */

Factor::Factor()
{ /* factor */
    super.push_back(this);
    wasInCall = inCallArgs;
    inCallArgs = false;
    if (SY == TYPESY) {
        l4typ11z = hashTravPtr->typ;
        inSymbol();
        if (SY != LPAREN) error(88 + (int64_t)LPAREN);
        expression();
        if (typeSize(curExpr->vt.typ) != typeSize(l4typ11z))
            error(errNeedOtherTypesOfOperands);
        checkSymAndRead(RPAREN);
        curExpr->vt.typ = l4typ11z;
    } else if (SY == IDENT or SY == INTCONST or SY == REALCONST or
               SY == CHARCONST or SY == STRINGSY or SY == LPAREN or
               SY == LBRACK) {
        switch (SY) {
        case IDENT: {
            if (hashTravPtr == NULL) {
                error(errNotDefined);
                curExpr = uVarPtr;
                inSymbol();
            } else
                switch (hashTravPtr->cl) {
                case ENUMID: {
                    curExpr = new Expr;
                    curExpr->vt.typ = hashTravPtr->typ;
                    curExpr->op = GETENUM;
                    curExpr->num1 = hashTravPtr->value();
                    curExpr->num2 = 0;
                    inSymbol();
                } break;
                case ROUTINEID: { /*(rout)*/
                    routine = hashTravPtr;
                    inSymbol();
                    if (routine->offset == 0) {
                        if (routine->typ != voidType and
                            SY == LPAREN) {
                            stdCall();
                            break; /* exit rout */
                        }
                        error(44); /* errIncorrectUsageOfStandProcOrFunc */
                    } else if (routine->typ == voidType) {
                        if (wasInCall) {
                            newOp = PCALL;
                        } else {
                            error(68); /* errUsingProcedureInExpression */
                        }
                    } else {
                        if (SY == LPAREN) {
                            parseCallArgs(routine);
                            break; /* exit rout */
                        }
                        if (wasInCall) {
                            newOp = FCALL;
                        } else {
                            parseCallArgs(routine);
                            break; /* exit rout */
                        }
                    }
                    if (not (SY == RPAREN or SY == COMMA)) {
                        error(errNoCommaOrParenOrTooFewArgs);
                        throw 8888;
                    }
                    curExpr = mkExpr(newOp, routine->typ, NULL,
                                     (ExprPtr)routine);
                } break;
                case VARID: case FORMALID: case FIELDID:
                    parseLval();
                    break;
                default:
                    break;
                } /* case */
        } break;
        case LPAREN: {
            expression();
            checkSymAndRead(RPAREN);
        } break;
        case INTCONST: case REALCONST: case CHARCONST: case STRINGSY: {
            curExpr = new Expr;
            parseLiteral(curExpr->vt.typ, curExpr->lit, false);
            curExpr->num2 = (int64_t)numFormat;
            curExpr->op = GETENUM;
            inSymbol();
        } break;
        case LBRACK: {
            curExpr = new Expr;
            inSymbol();
            l4var8z = curExpr;
            l4var1z.ii = Bits();
            if (SY != RBRACK) {
                l4var12z = true;
                readNext = false;
                do {
                    newExpr = curExpr;
                    expression();
                    if (l4var12z) {
                        l4typ11z = curExpr->vt.typ;
                        if (l4typ11z.p.pk != kindScalar)
                            error(23); /* errTypeIdInsteadOfVar */
                    } else {
                        if (not typeCheck(l4typ11z, curExpr->vt.typ))
                            error(24); /*errIncompatibleExprsInSetCtor*/
                    }
                    l4var12z = false;
                    l4exp5z = curExpr;
                    if (SY == COLON) {
                        expression();
                        if (not typeCheck(l4typ11z, curExpr->vt.typ))
                            error(24); /*errIncompatibleExprsInSetCtor*/
                        if (l4exp5z->op != GETENUM or
                            curExpr->op != GETENUM)
                            error(errNoConstant);
                        else {
                            l4var4z.ii = l4exp5z->num1;
                            l4var3z.ii = curExpr->num1;
                            l4var1z.ii = l4var1z.ii |
                                BitRange(l4var4z.ii, l4var3z.ii);
                            curExpr = newExpr;
                        }
                        goto L14567;
                    } else {
                        if (l4exp5z->op == GETENUM) {
                            l4var4z.ii = l4exp5z->num1;
                            l4var1z.ii = l4var1z.ii | Bits(l4var4z.ii);
                            curExpr = newExpr;
                            goto L14567;
                        }
                        error(errNoConstant);
                    }
                    curExpr = mkExpr(SETOR, IntegerType, newExpr, l4exp5z);
L14567:             ;
                } while (SY == COMMA);
            }
            checkSymAndRead(RBRACK);
            l4var8z->op = GETENUM;
            l4var8z->vt.typ = IntegerType;
            l4var8z->lit = l4var1z;
        } break;
        default:
            break;
        } /* case */
    } else {
        error(errBadSymbol);
        throw 8888;
    }
    /* Any factor producing an rvalue/lvalue may be followed by postfix
       operators (@ for pointer/file deref, .field for struct member,
       [idx] for array element).  parseLval already drained them above;
       parsePostfix is a no-op when SY is not a postfix token. */
    parsePostfix();
    if (charClass == INCROP or charClass == DECROP) {
        if (not has(lvalOpSet, curExpr->op))
            error(27);
        if (not typeCheck(curExpr->vt.typ, IntegerType))
            error(62);
        l4var1z.b = (charClass == INCROP);
        inSymbol();
        curExpr = bldIncDec(curExpr, l4var1z.b, true);
    }
} /* factor */

void parseUnaryExpression()
{
    Operator oper;

    oper = NOOP;
    if (has((Bits(PLUSOP, MINUSOP, BITNEGOP, NOTOP) |
         Bits(MUL, SETAND, INCROP, DECROP)), charClass)) {
        if (charClass != PLUSOP)
            oper = charClass;
        inSymbol();
    }
    if (oper != NOOP)
        parseUnaryExpression();
    else
        Factor();
    if (oper != NOOP) {
        arg1Type = curExpr->vt.typ;
        switch (oper) {
        case MINUSOP: {
            if (arg1Type == RealType)
                curExpr = mkExpr(RNEGOP, RealType, curExpr, NULL);
            else if (typeCheck(arg1Type, IntegerType))
                curExpr = mkExpr(INEGOP, IntegerType, curExpr, NULL);
            else {
                error(69); /* errUnaryMinusNeedRealOrInteger */
                return;
            }
        } break;
        case BITNEGOP: {
            if (typeCheck(arg1Type, IntegerType))
                curExpr = mkExpr(BITNEGOP, IntegerType, curExpr, NULL);
            else {
                error(62); /* errIntegerNeeded */
                return;
            }
        } break;
        case NOTOP: {
            if (arg1Type == BooleanType)
                curExpr = mkExpr(NOTOP, BooleanType, curExpr, NULL);
            else if (arg1Type == IntegerType) {
                curExpr = mkExpr(EQOP, BooleanType, curExpr, mkIntLit(0));
            } else {
                error(errNeedOtherTypesOfOperands);
                return;
            }
        } break;
        case MUL: {
            if (isCharPtr(arg1Type))
                curExpr = flatMemAt(curExpr);
            else if (arg1Type.p.pk == kindPtr)
                curExpr = mkExpr(DEREF, ptrBase(arg1Type),
                                 curExpr, NULL);
            else {
                stmtName = "unary*";
                error(errWrongVarTypeBefore);
            }
        } break;
        case SETAND: {
            if (not has(lvalOpSet, curExpr->op))
                error(27); /* errExpressionWhereVariableExpected */
            if (curExpr->op == GETELT and
                curExpr->expr1->op == GETVAR and
                curExpr->expr1->id1 == flatMemVar) {
                curExpr = curExpr->expr2;
                curExpr->vt.typ = charPtrType;
            } else if (arg1Type == CharType)
                curExpr = mkExpr(INTPLUS, charPtrType,
                    mkExpr(IMULOP, IntegerType,
                           mkCastInt(mkRef(curExpr)), mkIntLit(6)),
                    mkIntLit(5));
            else if (curExpr->op == GETELT and
                     isCharArray(curExpr->expr1->vt.typ))
                curExpr = mkExpr(INTPLUS, charPtrType,
                    mkExpr(IMULOP, IntegerType,
                           mkCastInt(mkRef(curExpr->expr1)),
                           mkIntLit(6)),
                    curExpr->expr2);
            else {
                curExpr = mkExpr(STANDPROC, voidPtr, curExpr, NULL);
                curExpr->num2 = fnREF;
            }
        } break;
        case INCROP: case DECROP: {
            if (not has(lvalOpSet, curExpr->op)) {
                error(27);
                return;
            }
            if (not typeCheck(arg1Type, IntegerType)) {
                error(62);
                return;
            }
            curExpr = bldIncDec(curExpr, oper == INCROP, false);
        } break;
        default:
            break;
        }
    }
} /* parseUnaryExpression */

void parsePrc(int64_t minPrec)
{
    Operator oper;
    ExprPtr leftExpr, thenExpr;
    int64_t curPrec;
    bool match;

    /* Parse left operand with unary operators */
    parseUnaryExpression();

    /* Climb through operators at this precedence level and higher */
    while (true) {
        curPrec = getPrec(SY, charClass);

        /* Stop if operator has lower precedence than minimum */
        if (curPrec < minPrec)
            return;

        oper = charClass;
        inSymbol();
        leftExpr = curExpr;

        if (oper == CONDOP) {
            /* Right-associative ternary: cond ? thenExpr : elseExpr */
            parsePrc(precAssign);
            if (SY != COLON)
                requiredSymErr(COLON);
            else
                inSymbol();
            thenExpr = curExpr;
            parsePrc(precCond);
            bldCondOp(leftExpr, thenExpr);
        } else if (curPrec == precAssign) {
            /* Right-associative assignment: lhs [op]= rhs.  `oper` (captured
               above before inSymbol) is ASSIGNOP for plain `=`; for op-assign
               (+=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=) it carries the
               underlying operation, lexed as SY=BECOMES + charClass=op.
               Plain `=` yields ASSIGNOP(lhs, rhs).  Op-assign yields
               RMWASSIGN(lhs, inner-op(rhs, NIL)) where inner-op carries the
               operator (e.g. INTPLUS) and the RHS in expr1; expr2 is the
               don't-care slot.  Codegen for RMWASSIGN walks lhs once,
               materialising its address into a spill slot when needed, then
               synthesises the equivalent ASSIGNOP for emission. */
            if (not has(lvalOpSet, leftExpr->op))
                error(27); /* errExpressionWhereVariableExpected */
            parsePrc(precAssign);
            arg1Type = leftExpr->vt.typ;
            arg2Type = curExpr->vt.typ;
            if (oper != ASSIGNOP) {
                /* Reuse bldArithOp/bldBitOp/bldLogOp for operator selection
                   (PLUSOP vs INTPLUS, etc.) and type promotion, then drop
                   the leftExpr slot of the result so it stores op(rhs, NIL)
                   ready for RMWASSIGN.expr2.  RMWASSIGN.expr1 carries the
                   original lvalue subtree, evaluated once at codegen time. */
                match = typeCheck(arg1Type, arg2Type);
                switch (opPrec[oper]) {
                case precMul:
                case precAdd:    bldArithOp(oper, leftExpr, match); break;
                case precShift:
                case precBitAnd:
                case precBitXor:
                case precBitOr:  bldBitOp(oper, leftExpr); break;
                case precAnd:
                case precOr:     bldLogOp(oper, leftExpr, match); break;
                }
                curExpr->expr1 = curExpr->expr2;
                curExpr->expr2 = NULL;
                arg2Type = curExpr->vt.typ;
            }
            leftExpr = cpDsLval(leftExpr);
            if (not typeCheck(arg1Type, arg2Type)) {
                if (arg1Type == RealType and
                    typeCheck(IntegerType, arg2Type))
                    castToReal(curExpr);
                else if (isCharPtr(arg1Type) and
                         isCharPtr(arg2Type))
                    curExpr = curExpr;
                else
                    error(33); /*errIllegalTypesForAssignment*/
            }
            if (oper != ASSIGNOP)
                curExpr = mkExpr(RMWASSIGN, arg1Type,
                                 leftExpr, curExpr);
            else
                curExpr = mkExpr(ASSIGNOP, arg1Type,
                                 leftExpr, curExpr);
        } else {
            /* Recursively parse right operand with higher precedence */
            /* For left-associative: use curPrec + 1 */
            parsePrc(curPrec + 1);

            /* Build AST node based on operator type */
            arg1Type = curExpr->vt.typ;
            arg2Type = leftExpr->vt.typ;
            match = typeCheck(arg1Type, arg2Type);

            switch (curPrec) {
            case precMul:
            case precAdd: bldArithOp(oper, leftExpr, match); break;
            case precRel:
            case precEq: bldRelOp(oper, leftExpr); break;
            case precShift:
            case precBitAnd:
            case precBitXor:
            case precBitOr: bldBitOp(oper, leftExpr); break;
            case precAnd:
            case precOr: bldLogOp(oper, leftExpr, match); break;
            }
        }
    }
} /* parsePrc */

void parentExpression()
{
    if (readNext)
        inSymbol();
    checkSymAndRead(LPAREN);
    readNext = false;
    expression();
    checkSymAndRead(RPAREN);
} /* parentExpression */

void expression()
{
    if (readNext)
        inSymbol();
    else
        readNext = true;
    parsePrc(precAssign);
} /* expression */

void setStrLab()
{
    StrLabel * &strLabPtr = Statement::super.back()->strLabPtr;
    StrLabel * &strLabList = programme::super.back()->strLabList;

    strLabPtr = new StrLabel;
    padToLeft();
    disableNorm();
    strLabPtr->next = strLabList;
    strLabPtr->ident.ii = curIdent;
    strLabPtr->target = 0;
    strLabList = strLabPtr;
} /* setStrLab */

void setBrCont()
{
    curIdent = 04262454153LL;         /* BREAK */
    setStrLab();
    curIdent = 04357566451566545LL;   /* CONTINUE */
    setStrLab();
} /* setBrCont */

void brContTarget()
{
    StrLabel * &strLabList = programme::super.back()->strLabList;

    /* assigning target for break/continue if used */
    if (strLabList->target != 0)
        fixup(0, strLabList->target);
    strLabList = strLabList->next; /* removing break/continue */
} /* brContTarget */

void forStatement()
{
    int64_t toLoop, leave;
    ExprPtr loopExpr;

    inSymbol();
    checkSymAndRead(LPAREN);
    if (SY != SEMICOLON) {
        readNext = false;
        expression();
        (void) formOperator(DOIT);
    }
    checkSymAndRead(SEMICOLON);
    padToLeft();
    toLoop = moduleOffset;
    leave = 0;
    if (SY != SEMICOLON) {
        readNext = false;
        expression();
        jumpTarget = 0;
        (void) formOperator(BRANCH);
        leave = jumpTarget;
    }
    checkSymAndRead(SEMICOLON);
    loopExpr = NULL;
    if (SY != RPAREN) {
        readNext = false;
        expression();
        loopExpr = curExpr;
    }
    checkSymAndRead(RPAREN);
    setBrCont();
    Statement();
    brContTarget(); /* removing continue */
    if (loopExpr != NULL) {
        curExpr = loopExpr;
        (void) formOperator(DOIT);
    }
    formJump(toLoop);
    if (leave != 0) {
        padToLeft();
        fixup(0, leave);
    }
    brContTarget(); /* removing break */
} /* forStatement */

void withStatement()
{
    ExprPtr oldWith;
    int64_t l4var2z, l4var3z;
    int64_t l4var4z;
    int64_t & localSize = programme::super.back()->localSize;

    oldWith = withList;
    l4var4z = localSize;
    l4var2z = freeRegs;
    l4var3z = Bits();
    do {
        expression();
        if (curExpr->vt.typ.p.pk == kindStruct) {
            (void) formOperator(SETREG);
            l4var3z = (l4var3z | Bits(curVal.ii)) & auxRegs;
        } else {
            error(71); /* errWithOperatorNotOfARecord */
        }
    } while (SY == COMMA);
    checkSymAndRead(DOSY);
    Statement();
    withList = oldWith;
    localSize = l4var4z;
    freeRegs = l4var2z;
    usedRegs = usedRegs | l4var3z;
} /* withStatement */

void reportStmtType()
{
    int64_t &startLine = Statement::super.back()->startLine;

    printf(" STATEMENT %s IN %ld LINE\n", stmtName.c_str(), startLine);
} /* reportStmtType */

void structBranch()
{
    StrLabel * curLab;
    StrLabel * &strLabList = programme::super.back()->strLabList;

    curLab = strLabList;
    while (curLab != NULL) {
        if (curLab->ident.ii == curIdent) {
            formJump(curLab->target);
            return;
        }
        curLab = curLab->next;
    }
    error(errNotDefined);
    throw 8888;
} /* structBranch */

void caseStatement()
{
    typedef struct CaseChain : public BESM6Obj {
        CaseChain * next;
        Word value;
        int64_t offset;
    } * CaseChainPtr;

    CaseChainPtr allClauses, curClause, clause, prev = NULL;
    bool isIntCase;
    bool otherSeen;
    int64_t otherOffset = -1;
    bool itemsEnded, goodMode;
    TPtr firstType, itemtype, exprtype;
    Word itemvalue;
    int64_t itemSpan;
    Word expected;
    int64_t decoder, endOfStmt;
    Word minValue, maxValue;

    parentExpression();
    exprtype = curExpr->vt.typ;
    otherSeen = false;
    if (exprtype == AlfaType or exprtype.p.pk == kindScalar)
        (void) formOperator(LOAD);
    else
        error(25); /* errExprNotOfADiscreteType */
    disableNorm();
    decoder = 0;
    endOfStmt = 0;
    allClauses = NULL;
    formJump(decoder);
    checkSymAndRead(BEGINSY);
    firstType.setRep(NULL);
    goodMode = true;
    do {
        if (not (SY == SEMICOLON || SY == ENDSY)) {
            padToLeft();
            arithMode = 1;
            if (SY == DEFAULTSY) {
                if (otherSeen)
                    error(73); /* errCaseLabelsIdentical */
                inSymbol();
                otherSeen = true;
                otherOffset = moduleOffset;
            } else {
                if (SY != CASESY)
                    requiredSymErr(CASESY);
                expression();
                (void) formOperator(LITINSN);
                itemvalue = curVal;
                itemtype = insnList->typ;
                if (itemtype.rep() != NULL) {
                    if (firstType.rep() == NULL) {
                        firstType = itemtype;
                    } else {
                        if (not typeCheck(itemtype, firstType))
                            error(errConstOfOtherTypeNeeded);
                    }
                    clause = new CaseChain;
                    clause->value = itemvalue;
                    clause->offset = moduleOffset;
                    curClause = allClauses;
                    while (curClause != NULL) {
                        if (itemvalue == curClause->value) {
                            error(73); /* errCaseLabelsIdentical */
                            break;
                        } else if (itemvalue.ii < curClause->value.ii) {
                            break;
                        } else {
                            prev = curClause;
                            curClause = curClause->next;
                        }
                    }
                    if (curClause == allClauses) {
                        clause->next = allClauses;
                        allClauses = clause;
                    } else {
                        clause->next = curClause;
                        prev->next = clause;
                    }
                }
            }
            checkSymAndRead(COLON);
            while (not (SY == CASESY || SY == DEFAULTSY || SY == ENDSY))
                Statement();
            goodMode = goodMode and (arithMode == 1);
        }
        itemsEnded = (SY == ENDSY);
        if (SY == SEMICOLON)
            inSymbol();
    } while (not itemsEnded);
    if (SY != ENDSY) {
        requiredSymErr(ENDSY);
        stmtName = "CASE  ";
        reportStmtType();
    } else
        inSymbol();
    if (not typeCheck(firstType, exprtype)) {
        error(74); /* errDifferentTypesOfLabelsAndExpr */
        return;
    }
    formJump(endOfStmt);
    padToLeft();
    isIntCase = typeCheck(exprtype, IntegerType);
    if (allClauses != NULL) {
        expected = allClauses->value;
        minValue = expected;
        curClause = allClauses;
        while (curClause != NULL) {
            if (expected == curClause->value and
                exprtype.p.pk == kindScalar) {
                maxValue = expected;
                if (isIntCase) {
                    expected.ii = expected.ii + 1;
                } else {
                    expected.ii = expected.ii + 1; // raw ordinal, no exponent
                }
                curClause = curClause->next;
            } else {
                itemSpan = 34000;
                fixup(0, decoder);
                if (firstType.p.pk == kindScalar)
                    itemSpan = firstType.rep()->numen;
                itemsEnded = itemSpan < 32000;
                if (itemsEnded) {
                    form1Insn(KATI+14);
                } else {
                    form1Insn(KATX+SP+1);
                }
                minValue.ii = (minValue.ii - minValue.ii); /* WTF? */
                while (allClauses != NULL) {
                    if (itemsEnded) {
                        curVal.ii = (minValue.ii - allClauses->value.ii);
                        form1Insn(getValueOrAllocSymtab(curVal.ii) +
                                  (KUTM+I14));
                        form1Insn(KVZM+I14 + allClauses->offset);
                        minValue = allClauses->value;
                    } else {
                        form1Insn(KXTA+SP+1);
                        curVal = allClauses->value;
                        form2Insn(KAEX + I8 + getFCSToffset(),
                                  InsnTemp[UZA] + allClauses->offset);
                    }
                    allClauses = allClauses->next;
                }
                if (otherSeen)
                    form1Insn(InsnTemp[UJ] + otherOffset);
                goto L16211;
            } /* if 16141 */
        } /* while 16142 */
        if (not otherSeen) {
            otherOffset = moduleOffset;
            formJump(endOfStmt);
        }
        fixup(0, decoder);
        curVal = minValue;
        fixup(-(InsnTemp[U1A]+otherOffset), maxValue.ii);
        curVal.ii = minValue.ii;
        curVal.ii = curVal.ii / 2;
        form3Insn(ASN64+1, KATI+14, KYTA);
        curVal.ii = moduleOffset + 1 - curVal.ii;
        if (curVal.ii < 040000) {
            curVal.ii = curVal.ii - 040000;
            curVal.ii = allocSymtab(041000000 | (curVal.ii & 077777));
        }
        form1Insn(KUJ+I14 + curVal.ii);
        padToLeft();
        if (minValue.ii & 1) {
            form1Insn(KUTC);
            decoder = (int64_t)UJ;
        } else
            decoder = (int64_t)UZA;
        while (allClauses != NULL) {
            form1Insn(InsnTemp[decoder] + allClauses->offset);
            allClauses = allClauses->next;
            decoder = (int64_t)UZA + (int64_t)UJ - decoder;
        }
L16211:
        fixup(0, endOfStmt);
        if (not goodMode)
            disableNorm();
    }
} /* caseStatement */

void ifWhileStatement()
{
    int64_t &ifWhlTarget = Statement::super.back()->ifWhlTarget;

    disableNorm();
    parentExpression();
    if (curExpr->vt.typ.p.pk > (uint64_t)kindPtr) {
        error(errBooleanNeeded);
    } else {
        jumpTarget = 0;
        (void) formOperator(BRANCH);
        ifWhlTarget = jumpTarget;
    }
    Statement();
} /* ifWhileStatement */

struct ParseData {
    struct DATAREC {
        int64_t b = 0;
        unsigned operator[](int i) {
            return (b >> (12*(3-i))) & 4095;
        }
        void assn(int i, int64_t val) {
            val &= 4095;
            val ^= (*this)[i];
            b = (b ^ (val << (12*(3-i)))) & 0xFFFFFFFFFFFFL;
        }
    };

    int64_t dsize, setcount;
    Word l4var3z, l4var4z, l4var5z;
    ExprPtr boundary;
    Word l4var7z, l4var8z, l4var9z;
    std::vector<DATAREC> F;

    int64_t allocDataRef(int64_t l6arg1z) {
        if (l6arg1z >= 2048) {
            curVal.ii = l6arg1z;
            return allocSymtab((curVal.ii | 040000000) & halfWord);
        } else {
            return l6arg1z;
        }
    } /* allocDataRef */

    void putDataRec(int64_t l5arg1z) {
        DATAREC l5var1z;

        l5var1z.assn(0, allocDataRef(l4var4z.ii));
        if (FcstCnt == l4var3z.ii) {
            curVal = l4var8z;
            curVal.ii = addCurValToFCST();
        } else {
            curVal = l4var3z;
        }
        l5var1z.assn(1, allocSymtab(0400100000000L | (curVal.ii & halfWord)));
        l5var1z.assn(2, allocDataRef(l5arg1z));
        if (l4var9z.ii == 0) {
            curVal.ii = shr48(l4var7z.ii, 24);
        } else {
            curVal.ii = allocSymtab(l4var7z.ii | (l4var9z.ii & halfWord));
        }
        l5var1z.assn(3, curVal.ii);
        l4var9z.ii = l5arg1z * l4var4z.ii + l4var9z.ii;
        F.push_back(l5var1z);
        setcount = setcount + 1;
        l4var4z.ii = 0;
        l4var3z.ii = FcstCnt;
    } /* putDataRec */

    ParseData() {
        dsize = FcstCnt;
        inSymbol();
        setcount = 0;
/*(loop)*/
        do { /* 16530 */
            inSymbol();
            setup(boundary);
            if (SY != IDENT) {
                if (SY == ENDSY)
                    break;
                error(errNoIdent);
                curExpr = uVarPtr;
            } else /* 16543 */ {
                if (hashTravPtr == NULL) {
L16545:             error(errNotDefined);
                    curExpr = uVarPtr;
                    inSymbol();
                } else {
                    if (hashTravPtr->cl == VARID) {
                        parseLval();
                    } else goto L16545;
                }
            } /* 16557 */
            putLeft = true;
            objBufIdx = 1;
            (void) formOperator(SETREG9);
            if (objBufIdx != 1)
                error(errVarTooComplex);
            l4var7z.ii = leftInsn & 0777700000000L;
            l4var3z.ii = FcstCnt;
            l4var4z.ii = 0;
            l4var9z.ii = 0;
            do { /* 16574 */
                expression();
                (void) formOperator(LITINSN);
                l4var8z = curVal;
                if (SY == COLON) {
                    inSymbol();
                    l4var5z = curToken;
                    if (SY != INTCONST) {
                        error(62); /* errIntegerNeeded */
                        l4var5z.ii = 0;
                    } else
                        inSymbol();
                } else
                    l4var5z.ii = 1;
                if (l4var5z.ii != 1) {
                    if (l4var4z.ii != 0)
                        putDataRec(1);
                    l4var4z.ii = 1;
                    putDataRec(l4var5z.ii);
                } else {
                    l4var4z.ii = l4var4z.ii + 1;
                    if (SY == COMMA) {
                        curVal = l4var8z;
                        toFCST();
                    } else {
                        if (l4var4z.ii != 1) {
                            curVal = l4var8z;
                            toFCST();
                        }
                        putDataRec(1);
                    }
                } /* 16641 */
            } while (SY == COMMA);
            rollup(boundary);
        } while (SY == SEMICOLON); /* 16645 */
        if (SY != ENDSY)
            error(errBadSymbol);
        for (size_t s = 0; s < F.size(); ++s) FCST.push_back(F[s].b);
        lookup2 = FcstCnt - dsize;
        FcstCnt = dsize;
        lookupMode = setcount;
    }
}; /* parseData */

void parseConstExpression()
{
    TPtr &ceTyp = programme::super.back()->ceTyp;
    Word &ceVal = programme::super.back()->ceVal;
    ExprPtr &boundary = Statement::super.back()->boundary;

    readNext = false;
    ceTyp = voidType;
    ceVal.ii = 1;
    expression();
    (void) formOperator(LITINSN);
    ceTyp = insnList->typ;
    ceVal = curVal;
    rollup(boundary);
} /* parseConstExpression */

struct standProc {

    TPtr l4typ1z, l4typ2z, l4typ3z;
    ExprPtr firstWidth, secondWidth;
    ExprPtr l4exp6z;
    ExprPtr l4exp7z, l4exp8z, workExpr;
    bool l4bool10z, noWidth, needR12;
    int64_t oldOffset;
    int64_t defWidth;
    int64_t procNo;
    int64_t helperNo;
    int64_t indCnt;
    OpGen opToForm;

    void verifyType(TPtr t) {
        readNext = false;
        expression();
        if (t != voidType and
            not typeCheck(t, curExpr->vt.typ)) {
            error(errNeedOtherTypesOfOperands);
            curExpr = uVarPtr;
        }
    } /* verifyType */

    void startWrite() {
        expression();
        l4typ3z = curExpr->vt.typ;
        l4exp7z = curExpr;
        if (workExpr == NULL) {
            if (typeSize(l4typ3z) == 30) {
                workExpr = curExpr;
            } else {
                workExpr = new Expr;
                workExpr->vt.typ = textType;
                workExpr->op = GETVAR;
                workExpr->id1 = outputFile;
            }
            arg2Type = workExpr->vt.typ;
            needR12 = true;
            l4exp8z = mkExpr(DEREF, ptrBase(arg2Type), workExpr, NULL);
            l4exp6z = mkExpr(ASSIGNOP, l4exp8z->vt.typ, l4exp8z, NULL);
        }
    } /* startWrite */

    ExprPtr parseWidthSpecifier() {
        expression();
        if (not typeCheck(IntegerType, curExpr->vt.typ)) {
            error(14); /* errExprIsNotInteger */
            return uVarPtr;
        } else
            return curExpr;
    } /* parseWidthSpecifier */

    void callHelperWithArg() {
        if (has(usedRegs, 12) or needR12) {
            curExpr = workExpr;
            (void) formOperator(SETREG12);
        }
        needR12 = false;
        formAndAlign(getHelperProc(helperNo));
        disableNorm();
    } /* callHelperWithArg */

    void checkElementForReadWrite() {
        usedRegs = usedRegs & ~ Bits(12);
        curVarKind = (Kind)(l4typ3z.p.pk);
        helperNo = 36;                   /* C/WI */
        if (l4typ3z == IntegerType or l4typ3z == BooleanType)
            defWidth = 10;
        else if (l4typ3z == RealType) {
            helperNo = 37;               /* P/WR */
            defWidth = 14;
        } else if (l4typ3z == CharType) {
            helperNo = 38;               /* P/WC */
            defWidth = 1;
        } else if (curVarKind == kindScalar
                   and l4typ3z.rep()->start != -1) {
            helperNo = 41;               /* P/WX */
            dumpEnumNames(l4typ3z);
            defWidth = 8;
        } else if (isCharArray(l4typ3z)) {
            defWidth = l4typ3z.rep()->aright - l4typ3z.rep()->aleft + 1;
            if (not l4typ3z.rep()->pck)
                helperNo = 81;            /* P/WA */
            else if (6 >= defWidth)
                helperNo = 39;            /* P/A6 */
            else
                helperNo = 40;           /* P/A7 */
        } else if (typeSize(l4typ3z) == 1) {
            helperNo = 42;               /* P/WO */
            defWidth = (typeBits(l4typ3z) + 5) / 3;
        } else {
            error(34); /* errTypeIsNotAFileElementType */
        }
    } /* checkElementForReadWrite */

    void writeProc() {
        workExpr = NULL;
        do {
            startWrite();
            if (l4exp7z != workExpr) {
                checkElementForReadWrite();
                secondWidth = NULL;
                firstWidth = NULL;
                if (SY == COLON)
                    firstWidth = parseWidthSpecifier();
                if (SY == COLON) {
                    secondWidth = parseWidthSpecifier();
                    if (helperNo != 37)    /* P/WR */
                        error(35); /* errSecondSpecifierForWriteOnlyForReal */
                } else if (curToken.ii == litOct) {
                    helperNo = 42; /* P/WO */
                    defWidth = 17;
                    if (typeSize(l4typ3z) != 1)
                        error(34); /* errTypeIsNotAFileElementType */
                    inSymbol();
                }
                noWidth = false;
                if (firstWidth == NULL and
                    has(BitRange(38,40), helperNo)) {  /* WC,A6,A7 */
                    helperNo = helperNo + 5;       /* CW,6A,7A */
                    noWidth = true;
                } else {
                    if (firstWidth == NULL) {
                        curVal.ii = defWidth;
                        (void) formOperator(DFLTWDTH);
                    } else {
                        curExpr = firstWidth;
                        (void) formOperator(LOAD);
                        form1Insn(KAOX+ZERO);
                    }
                }
                if (helperNo == 37) {       /* P/WR */
                    if (secondWidth == NULL) {
                        curVal.ii = 4 | 0xDLL << 44;
                        form1Insn(KXTS+I8 + getFCSToffset());
                    } else {
                        curExpr = secondWidth;
                        (void) formOperator(FRACWIDTH);
                        form1Insn(KAOX+ZERO);
                    }
                }
                curExpr = l4exp7z;
                if (noWidth) {
                    if (helperNo == 45)     /* P/7A */
                        opToForm = gen11;
                    else
                        opToForm = LOAD;
                } else {
                    if (helperNo == 40 or       /* P/A7 */
                        helperNo == 81)     /* P/WA */
                        opToForm = gen12;
                    else
                        opToForm = FRACWIDTH;
                }
                (void) formOperator(opToForm);
                if (has(Bits(39,40,44,45), helperNo) or /* A6,A7,6A,7A */
                    helperNo == 81)
                    form1Insn(KVTM+I10 + defWidth);
                else {
                    if (helperNo == 41) /* P/WX */
                        form1Insn(KVTM+I11 + l4typ3z.rep()->start);
                }
                callHelperWithArg();
            }
        } while (SY == COMMA);
        if (procNo == 11) {
            helperNo = 46;                 /* P/WL */
            callHelperWithArg();
        }
        usedRegs = usedRegs | Bits(12);
        if (oldOffset == moduleOffset)
            error(36); /*errTooFewArguments */
    } /* writeProc */

    void checkArrayArg() {
        verifyType(voidType);
        workExpr = curExpr;
        l4typ1z = curExpr->vt.typ;
        if (l4typ1z.rep()->pck or
            l4typ1z.p.pk != kindArray)
            error(errNeedOtherTypesOfOperands);
        checkSymAndRead(COMMA);
        readNext = false;
        expression();
        l4exp8z = curExpr;
    } /* checkArrayArg */

    void doPackUnpack() {
        TPtr t;

        l4exp7z = mkExpr(GETELT, l4typ1z.rep()->base, workExpr, l4exp8z);
        t = l4exp6z->vt.typ;
        if (t.p.pk != kindArray or
            not t.rep()->pck or
            t.rep()->base.p.pk != kindScalar or
            l4typ1z.rep()->base.p.pk != kindScalar)
            error(errNeedOtherTypesOfOperands);
        curExpr = new Expr;
        curExpr->vt.ii = procNo + 50;   /* curExpr@.vt.c := chr(procNo + 50) */
        curExpr->expr1 = l4exp7z;
        curExpr->expr2 = l4exp6z;
        (void) formOperator(PCKUNPCK);
    } /* doPackUnpack */

    standProc() { /* standProc */
        IdentRecPtr &l3idr12z = Statement::super.back()->l3idr12z;
        TPtr &l2typ13z = programme::super.back()->l2typ13z;
        IdentRecPtr &procName = programme::super.back()->procName;
        int64_t &hasFiles = programme::super.back()->hasFiles;
        bool &retSeen = programme::super.back()->retSeen;
        int64_t &ii = programme::super.back()->ii;

        curVal.ii = l3idr12z->low();
        procNo = curVal.ii;
        l4bool10z = (SY == LPAREN);
        oldOffset = moduleOffset;
        if (not l4bool10z and
            has((BitRange(0,5) | Bits(10) | Bits(12) | BitRange(15,28)), procNo))
            error(45); /* errNoOpenParenForStandProc */
        if (has((BitRange(0,5) | Bits(12,15)), procNo)) {
            expression();
            if (not has(lvalOpSet, curExpr->op)) {
                error(27); /* errExpressionWhereVariableExpected */
            }
            arg1Type = curExpr->vt.typ;
            curVarKind = (Kind)(arg1Type.p.pk);
        }
        if (has(BitRange(0,6), procNo))
            jumpTarget = getHelperProc(29 + procNo); /* P/PF */
        switch (procNo) {
        case 0: case 1: case 2: case 3: { /* put, get, rewrite, reset */
            if (typeSize(arg1Type) != 30)
                error(47); /* errNoVarOfFileType */
            if (procNo == 3 and SY == COMMA) {
                (void) formOperator(SETREG12);
                expression();
                if (not typeCheck(IntegerType, curExpr->vt.typ))
                    error(14); /* errExprIsNotInteger */
                (void) formOperator(LOAD);
                formAndAlign(getHelperProc(97)); /*"P/RE"*/
            } else {
                (void) formOperator(FILEACCESS);
            }
        } break;
        case 5: { /* free */
            if (curVarKind != kindPtr)
                error(13); /* errVarIsNotPointer */
            heapCallsCnt = heapCallsCnt + 1;
            workExpr = curExpr;
            (void) formOperator(SETREG9);
            l2typ13z = ptrBase(arg1Type);
            ii = typeSize(l2typ13z);
            if (SY == COLON) {
                expression();
                if (not typeCheck(IntegerType, curExpr->vt.typ))
                error(14); /* errExprIsNotInteger */
                if (curExpr->op == GETENUM) {
                    ii = curExpr->lit.ii;
                    goto L5_44;
                } else {
                    (void) formOperator(LOAD);
                    form1Insn(KATI+14);
                }
            } else {
L5_44:          form1Insn(KVTM+I14+getValueOrAllocSymtab(ii));
            }
            formAndAlign(jumpTarget);
        } break;
        case 6: { /* halt */
            formAndAlign(jumpTarget);
            return;
        } break;
        case 10: { /* write */
            writeProc();
        } break;
        case 11: { /* writeln */
            if (SY == LPAREN) {
                writeProc();
            } else {
                formAndAlign(getHelperProc(54)); /*"P/WOLN"*/
                return;
            }
        } break;
        case 12: { /* ctor(lvalue, expr0, expr1, ...): struct-constructor
                      assignment.  Lvalue (already parsed above) must be
                      kindStruct; each comma-separated expression is stored at
                      successive word offsets from the lvalue address, using
                      register 9 as the base.  Empty argument positions skip an
                      offset. */
            if (curVarKind != kindStruct)
                error(errNeedOtherTypesOfOperands);
            (void) formOperator(SETREG9);
            indCnt = 0;
            inSymbol();          /* consume the comma between lvalue and expr0 */
            while (true) {
                if (SY == COMMA) {
                    indCnt = indCnt + 1;
                    inSymbol();
                } else if (SY == RPAREN) {
                    break;
                } else {
                    readNext = false;
                    expression();
                    curVal.ii = indCnt;
                    (void) formOperator(STOREAT9);
                }
            }
        } break;
        case 14: { /* return [expr] */
            if (not has(statEndSys, SY)) {
                /* return expr: load expr to ACC, then jump */
                if (procName->typ == voidType)
                    error(errNeedOtherTypesOfOperands);
                else {
                    if (hasFiles != 0) {
                        printf(" functions must not use files\n");
                        error(200);
                    }
                    retSeen = true;
                    readNext = false;
                    expression();
                    if (typeCheck(procName->typ, curExpr->vt.typ)) {
                        /* OK */
                    } else if (procName->typ == RealType and
                               typeCheck(IntegerType, curExpr->vt.typ)) {
                        castToReal(curExpr);
                    } else
                        error(33); /* errIllegalTypesForAssignment */
                    (void) formOperator(LOAD);
                }
            } else if (procName->typ != voidType)
                error(errNeedOtherTypesOfOperands);
            form1Insn(getHelperProc(27) + (KUJ-KVJM-I13));
            return;
        } break;
        case 16: { /* besm */
            expression();
            (void) formOperator(LITINSN);
            formAndAlign(curVal.ii);
        } break;
        case 19: case 20: { /* pck, unpck */
            inSymbol();
            verifyType(CharType);
            checkSymAndRead(COMMA);
            (void) formOperator(SETREG12);
            verifyType(AlfaType);
            if (procNo == 20) {
                (void) formOperator(LOAD);
            }
            formAndAlign(getHelperProc(procNo - 6));
            if (procNo == 19)
                (void) formOperator(STORE);
        } break;
        case 21: { /* pack */
            inSymbol();
            checkArrayArg();
            checkSymAndRead(COMMA);
            verifyType(voidType);
            l4exp6z = curExpr;
            doPackUnpack();
        } break;
        case 22: { /* unpack */
            inSymbol();
            verifyType(voidType);
            l4exp6z = curExpr;
            checkSymAndRead(COMMA);
            checkArrayArg();
            doPackUnpack();
        } break;
        }
        if (has((Bits(0,1,2,3) | Bits(5,10,11,13) | Bits(21,22)), procNo))
            arithMode = 1;
        checkSymAndRead(RPAREN);
    }
}; /* standProc */

Statement::Statement()
{
    int64_t &ceRegs = programme::super.back()->ceRegs;
    StrLabel * &strLabList = programme::super.back()->strLabList;

    super.push_back(this);
    if (freeRegs != ceRegs and SY == SEMICOLON) {
        inSymbol();
        return; /* empty statement */
    }
    setup(boundary);
    bool110z = false;
    startLine = lineCnt;
    if (freeRegs == halfWord)
        ParseData();
    else if (freeRegs == ceRegs) {
        parseConstExpression();
        return;
    } else {
        try {
            if (SY == INTCONST) {
                liveRegs = Bits();
                disableNorm();
                flag = true;
                padToLeft();
                labCheckAndDefine(true);
                inSymbol();
                checkSymAndRead(COLON);
            }
            nest = has(Bits(BEGINSY,SWITCHSY), SY);
            if (nest)
                lineNesting = lineNesting + 1;
/*(ident)*/
            if (SY == IDENT) {
                if (hashTravPtr != NULL) {
                    l3var6z = hashTravPtr->cl;
                    if (l3var6z == ROUTINEID) {
                        l3idr12z = hashTravPtr;
                        if (l3idr12z->offset == 0) {
                            /* System procedure (WRITE, PUT, GET, NEW, ...):
                               special syntax, handled directly. */
                            inSymbol();
                            standProc();
                            checkSymAndRead(SEMICOLON);
                            goto exit_ident;
                        }
                        if (l3idr12z->typ == voidType) {
                            /* User procedure call (void return): not a valid
                               expression in factor(), so dispatch directly to
                               parseCallArgs. */
                            inSymbol();
                            parseCallArgs(l3idr12z);
                            (void) formOperator(DOIT);
                            checkSymAndRead(SEMICOLON);
                            goto exit_ident;
                        }
                    }
                    /* VARID / FORMALID / FIELDID, or ROUTINEID with non-NIL
                       typ (function call): assignment, function call, or other
                       expression used as a statement.  readNext := false keeps
                       the current SY (the leading IDENT) for expression(). */
                    readNext = false;
                    expression();
                    (void) formOperator(DOIT);
                    checkSymAndRead(SEMICOLON);
                } else {
                    error(errNotDefined);
                    inSymbol();
                    throw 8888;
                }
            } else if (has((Bits(EXPROP,LPAREN,INTCONST,REALCONST) |
                        Bits(CHARCONST,STRINGSY,LBRACK)), SY)) {
                /* Generic expression statement: '++x;', '(x = 1);', etc. */
                readNext = false;
                expression();
                (void) formOperator(DOIT);
                checkSymAndRead(SEMICOLON);
            } else if (SY == BEGINSY) {
              L_rep:
                inSymbol();
              L_skip:
                while (SY != ENDSY)
                    Statement();
                if (SY != ENDSY) {
                    stmtName = " BEGIN";
                    requiredSymErr(SEMICOLON);
                    reportStmtType();
                    skip(bigSkipSet);
                    if (has(statBegSys, SY))
                        goto L_skip;
                    if (SY != SEMICOLON)
                        goto L_exit_begin;
                    goto L_rep;
                }
                inSymbol();
              L_exit_begin:;
            } else if (SY == GOTOSY) {
                inSymbol();
                if (SY != INTCONST) {
                    error(62); /* errIntegerNeeded */
                    throw 8888;
                }
                disableNorm();
                labCheckAndDefine(false);
                inSymbol();
            } else if (SY == IFSY) {
                ifWhileStatement();
                if (SY == ELSESY) {
                    elseJump = 0;
                    formJump(elseJump);
                    fixup(0, ifWhlTarget);
                    curOffset.ii = arithMode;
                    arithMode = 1;
                    inSymbol();
                    Statement();
                    fixup(0, elseJump);
                    if (curOffset.ii != arithMode) {
                        arithMode = 2;
                        disableNorm();
                    }
                } else {
                    fixup(0, ifWhlTarget);
                }
            } else if (SY == WHILESY) {
                liveRegs = Bits();
                setBrCont();
                strLabList->target = moduleOffset;
                curOffset.ii = moduleOffset;
                ifWhileStatement();
                disableNorm();
                form1Insn(InsnTemp[UJ] + curOffset.ii);
                fixup(0, ifWhlTarget);
                strLabList = strLabList->next; /* removing continue */
                brContTarget(); /* removing break */
                arithMode = 1;
            } else if (SY == BREAKSY or SY == CONTSY) {
                structBranch();
                inSymbol();
                checkSymAndRead(SEMICOLON);
            } else if (SY == DOSY) {
                liveRegs = Bits();
                setBrCont();
                curOffset.ii = moduleOffset;
                inSymbol();
                Statement();
                brContTarget(); /* removing continue */
                if (SY != WHILESY) {
                    requiredSymErr(WHILESY);
                    stmtName = "  DO  ";
                    reportStmtType();
                    throw 8888;
                }
                disableNorm();
                parentExpression();
                if (curExpr->vt.typ != BooleanType and
                    curExpr->vt.typ != IntegerType) {
                    error(errBooleanNeeded);
                } else {
                    jumpTarget = curOffset.ii;
                    whileExpr = curExpr;
                    curExpr = mkExpr(NOTOP, BooleanType, whileExpr, NULL);
                    (void) formOperator(BRANCH);
                }
                brContTarget(); /* removing break */
            } else if (SY == FORSY) {
                liveRegs = Bits();
                forStatement();
            } else if (SY == SWITCHSY) {
                curIdent = 04262454153LL;      /* BREAK */
                setStrLab();
                caseStatement();
                brContTarget(); /* removing break */
            } else if (SY == WITHSY) {
                withStatement();
            }
          exit_ident:;
        } catch (int foo) {
            if (foo != 8888) throw;
            skip(skipToSet | statEndSys);
        }
      L_cleanup:
        if (nest)
            lineNesting = lineNesting - 1;
        rollup(boundary);
        if (bool110z) {
            bool110z = false;
            skip(skipToSet | statEndSys); /* goto 8888 */
            goto L_cleanup;
        }
    }
    /* 20766 */
} /* Statement */

void parseConstDeclValue(TPtr &typ, Word &value)
{
    int64_t savedFreeRegs;
    int64_t &ceRegs = programme::super.back()->ceRegs;
    TPtr &ceTyp = programme::super.back()->ceTyp;
    Word &ceVal = programme::super.back()->ceVal;

    if (SY == STRINGSY) {
        parseLiteral(typ, value, true);
        inSymbol();
        return;
    }
    savedFreeRegs = freeRegs;
    freeRegs = ceRegs;
    Statement();
    freeRegs = savedFreeRegs;
    typ = ceTyp;
    value = ceVal;
} /* parseConstDeclValue */

void outputObjFile()
{
    int64_t idx;

    padToLeft();
    objBufIdx = objBufIdx - 1;
    for (idx = 1; idx <= objBufIdx; ++idx)
        CHILD.push_back(objBuffer[idx]);
    lineStartOffset = moduleOffset;
    prevOpcode = -1;
}

void defineRoutine(bool bodyBlock = false)
{
    Word l3var1z, l3var2z;
    int64_t l3int4z;
    IdentRecPtr l3idr5z;
    Word l3var7z;
    IdentRecPtr &procName = programme::super.back()->procName;
    int64_t &hasFiles = programme::super.back()->hasFiles;
    int64_t &sizeCount = programme::super.back()->sizeCount;
    int64_t &jj = programme::super.back()->jj;
    int64_t &localSize = programme::super.back()->localSize;
    bool &done = programme::super.back()->done;

    objBufIdx = 1;
    objBuffer[objBufIdx] = 0;
    curInsnTemplate = InsnTemp[XTA];
    bool48z = has(procName->flags(), 22);
    if (curProcNesting == 1 and hasFiles != 0) {
        hasFiles = moduleOffset;
        (void) formOperator(FILEINIT);
    }
    lineStartOffset = moduleOffset;
    l3var1z.ii = moduleOffset;    /* l3var1z := ; (accumulator = moduleOffset) */
    lookup2 = lookWith;
    withList = NULL;
    arithMode = 1;
    liveRegs = Bits();
    freeRegs = BitRange(curProcNesting+1, 6);
    auxRegs = freeRegs & ~ Bits(minel(freeRegs));
    l3var7z.ii = freeRegs;
    usedRegs = BitRange(1,15) & ~ freeRegs;
    if (curProcNesting != 1)
        parseDecls(2);
    sizeCount = localSize;
    if (not bodyBlock and SY != BEGINSY)
        requiredSymErr(BEGINSY);
    if (has(procName->flags(), 23)) {
        l3idr5z = procName->argList();
        l3int4z = 3;
        if (procName->typ != voidType)
            l3int4z = 4;
        while (l3idr5z != procName) {
            if (l3idr5z->cl == VARID) {
                l3var2z.ii = typeSize(l3idr5z->typ);
                if (l3var2z.ii != 1) {
                    form3Insn(KVTM+I14 + l3int4z,
                              KVTM+I12 + l3var2z.ii,
                              KVTM+I11 + l3idr5z->value());
                    formAndAlign(getHelperProc(73)); /* "P/LNGPAR" */
                }
            }
            l3int4z = l3int4z + 1;
            l3idr5z = l3idr5z->list();
        }
    } /* 21105 */
    if (not has(optSflags.ii, NoStackCheck))
        fixup(-1, 95); /* P/SC */
    l3var2z.ii = lineNesting;
    if (bodyBlock) {
        while (SY != ENDSY and CH != 0)
            Statement();
        if (SY != ENDSY)
            requiredSymErr(ENDSY);
        else
            inSymbol();
    } else {
        do {
            Statement();
            if (curProcNesting == 1)
                done = (SY == PERIOD) or (CH == 0);
            else
                done = has(blockBegSys, SY) or (SY == TYPESY) or (CH == 0);
            if (not done) {
                if (curProcNesting == 1)
                    requiredSymErr(PERIOD);
                else {
                    errAndSkip(errBadSymbol, skipToSet);
                }
            }
        } while (not done);
    }
    procName->flags() = (usedRegs & BitRange(0,15)) | (procName->flags() & ~ l3var7z.ii);
    lineNesting = l3var2z.ii - 1;
    if (not bool48z and (sizeCount == 3) and
        (curProcNesting != 1) and ((usedRegs & BitRange(1,15)) != BitRange(1,15))) {
        objBuffer[1] = int64_t(KNTR+7) << 24 | KUTC;   /* ,NTR,7; ,UTC, */
        procName->flags() = procName->flags() | Bits(25);
        if (objBufIdx == 2) {
            objBuffer[1] = int64_t(KUJ+I13) << 24;      /* 13,UJ, */
            putLeft = true;
        } else {
            procName->pos() = l3var1z.ii;
            if (has(usedRegs, 13)) {
                curVal.ii = minel(BitRange(1,15) & ~ usedRegs);
                l3var7z.ii = curVal.ii << 24;           /* besm(ASN64-24) */
                objBuffer[2] |= int64_t(I13+KMTJ) << 24 | l3var7z.ii;
            } else {
                curVal.ii = 13;
            }
            form1Insn(InsnTemp[UJ] + indexreg[curVal.ii]);
        }
    } else /* 21220 */ {
        if (hasFiles == 0)
            jj = 27;    /* C/E */
        else
            jj = 28;   /* C/EF */
        form1Insn(getHelperProc(jj) + (KUJ-KVJM-I13));
        if (curProcNesting == 1) {
            parseDecls(2);
            form1Insn(InsnTemp[UJ] + l3var1z.ii);
            curVal.ii = procName->pos() - 040000;
            symTab[074002] = 041000000 | (curVal.ii & halfWord);
        }
        curVal.ii = sizeCount;
        if (curProcNesting != 1) {
            curVal.ii = curVal.ii - 2;
            l3var7z.ii = curVal.ii << 24;               /* besm(ASN64-24) */
            objBuffer[savedObjIdx] |= l3var7z.ii | int64_t(KUTM+SP) << 24;
        }
    } /* 21261 */
    outputObjFile();
} /* defineRoutine */

struct initScalars {
    Word l3var1z, outName, inName, savedIdent;
    int64_t l3var5z, l3var6z;
    IdentRecPtr l3var7z;
    int64_t l3var8z, sysProcNum;
    TPtr temptype;
    Word l3var11z;
    IdentRecPtr &curIdRec;

    void regSysType(int64_t l4arg1z, TPtr l4arg2z) {
        curIdRec = new IdentRec;
        // curIdRec@ := [l4arg1z, 0, , l4arg2z, TYPEID];
        curIdRec->id = l4arg1z;
        curIdRec->offset = 0;
        curIdRec->typ = l4arg2z;
        curIdRec->cl = TYPEID;
        addToHashTab(curIdRec);
    } /* regSysType */

    void regSysEnum(int64_t l4arg1z, int64_t l4arg2z) {
        curIdRec = new IdentRec;
        // curIdRec@ := [l4arg1z, 48, , temptype, ENUMID, NIL, l4arg2z];
        curIdRec->id = l4arg1z;
        curIdRec->offset = 48;
        curIdRec->typ = temptype;
        curIdRec->cl = ENUMID;
        curIdRec->list() = NULL;
        curIdRec->value() = l4arg2z;
        addToHashTab(curIdRec);
    } /* regSysEnum */

    void regSysProc(int64_t l4arg1z) {
        curIdRec = new IdentRec;
        // curIdRec@ := [l4arg1z, 0, , temptype, ROUTINEID, sysProcNum];
        curIdRec->id = l4arg1z;
        curIdRec->offset = 0;
        curIdRec->typ = temptype;
        curIdRec->cl = ROUTINEID;
        curIdRec->procno() = sysProcNum;
        sysProcNum = sysProcNum + 1;
        addToHashTab(curIdRec);
    } /* registerSysProc */

    void defExtern();
    initScalars();
};

void initScalars::defExtern()
{
    int64_t l = 0;
    l3var1z.ii = leftAlign(curIdent);
    if (curIdent == inName.ii) {
        inputFile = new IdentRec;
        inputFile->id = curIdent;
        inputFile->offset = 0;
        inputFile->typ = textType;
        inputFile->cl = VARID;
        inputFile->list() = NULL;
        curVal = l3var1z;
        inputFile->value() = allocExtSymbol(l3var11z.ii);
        addToHashTab(inputFile);
        l = lineCnt;
    } else if (curIdent == outName.ii) {
        outputFile = l3var7z;
        l = lineCnt;
    }
    curExternFile = externFileList;
    while (curExternFile != NULL) {
        if (curExternFile->id == curIdent) {
            curExternFile = NULL;
            error(errIdentAlreadyDefined);
        } else {
            curExternFile = curExternFile->next;
        }
    }
    curExternFile = new ExtFileRec;
    curExternFile->id = curIdent;
    curExternFile->next = externFileList;
    curExternFile->line = l;
    curExternFile->offset = l3var1z.ii;
    if (l != 0) {
        if (curIdent == outName.ii) {
            fileForOutput = curExternFile;
        } else {
            fileForInput = curExternFile;
        }
    }
    externFileList = curExternFile;
    l3var6z = l3var5z;
    l3var5z = l3var5z + 1;
    curExternFile->location = 512;
} /* defExtern */

initScalars::initScalars() :
    curIdRec(programme::super.back()->curIdRec)
{
    BooleanType.setRep(new Types);
    BooleanType.rep()->numen = 2;
    BooleanType.rep()->start = 0;
    BooleanType.rep()->enums = NULL;
    BooleanType.p.psize = 1;
    BooleanType.p.bits = 1;
    BooleanType.p.pk = kindScalar;

    IntegerType.setRep(new Types);
    IntegerType.rep()->numen = 100000;
    IntegerType.rep()->start = -1;
    IntegerType.rep()->enums = NULL;
    IntegerType.p.psize = 1;
    IntegerType.p.bits = 48;
    IntegerType.p.pk = kindScalar;

    CharType.setRep(new Types);
    CharType.rep()->numen = 256;
    CharType.rep()->start = -1;
    CharType.rep()->enums = NULL;
    CharType.p.psize = 1;
    CharType.p.bits = 8;
    CharType.p.pk = kindScalar;

    RealType.setRep(NULL);
    RealType.p.psize = 1;
    RealType.p.bits = 48;
    RealType.p.pk = kindReal;

    voidType.setRep(NULL);
    voidType.p.psize = 1;
    voidType.p.bits = 48;
    voidType.p.pk = kindVoid;

    voidPtr.setRep(new Types);
    voidPtr.rep()->base = voidType;
    voidPtr.p.psize = 1;
    voidPtr.p.bits = 15;
    voidPtr.p.pk = kindPtr;

    textType.setRep(new Types);
    textType.rep()->base = CharType;
    textType.p.pad = 8;
    textType.p.psize = 30;
    textType.p.bits = 48;
    textType.p.pk = kindPtr;

    AlfaType.setRep(new Types);
    AlfaType.rep()->base = CharType;
    AlfaType.rep()->pck = true;
    AlfaType.rep()->perword = 6;
    AlfaType.rep()->pcksize = 8;
    AlfaType.rep()->aleft = 1;
    AlfaType.rep()->aright = 6;
    AlfaType.p.psize = 1;
    AlfaType.p.bits = 48;
    AlfaType.p.pk = kindArray;

    charPtrType = getPtrType(CharType);

    flatMemType.setRep(new Types);
    flatMemType.rep()->base = CharType;
    flatMemType.rep()->pck = true;
    flatMemType.rep()->perword = 6;
    flatMemType.rep()->pcksize = 8;
    flatMemType.rep()->aleft = 0;
    flatMemType.rep()->aright = 32768 * 6 - 1;
    flatMemType.p.psize = 32767;
    flatMemType.p.bits = 48;
    flatMemType.p.pk = kindArray;

    flatMemVar = new IdentRec;
    flatMemVar->id = 0;
    flatMemVar->offset = 0;
    flatMemVar->typ = flatMemType;
    flatMemVar->cl = VARID;
    flatMemVar->list() = NULL;
    flatMemVar->value() = 0;

    smallStringType[6] = AlfaType;
    regSysType(0515664L /*"     INT"*/, IntegerType);
    regSysType(043504162L /*"    CHAR"*/, CharType);
    regSysType(062454154L /*"    REAL"*/, RealType);
    regSysType(041544641L /*"    ALFA"*/, AlfaType);
    regSysType(066575144L /*"    VOID"*/, voidType);
    temptype = voidPtr;
    regSysEnum(0565154L /*"     NIL"*/, 074000L);
    maxSmallString = 0;
    for (strLen = 2; strLen <= 5; ++strLen)
        smallStringType[strLen] = makeStringType();
    maxSmallString = 6;

    curIdRec = new IdentRec;
    curIdRec->offset = 0;
    curIdRec->typ = IntegerType;
    curIdRec->cl = VARID;
    curIdRec->list() = NULL;
    curIdRec->value() = 7;

    uVarPtr = new Expr;
    uVarPtr->vt.typ = IntegerType;
    uVarPtr->op = GETVAR;
    uVarPtr->id1 = curIdRec;

    uProcPtr = new IdentRec;
    uProcPtr->cl = ROUTINEID; // cl left as heap garbage in base.pas
    uProcPtr->typ.setRep(NULL);
    uProcPtr->list() = NULL;
    uProcPtr->argList() = NULL;
    uProcPtr->preDefLink() = NULL;
    uProcPtr->pos() = 0;
    uProcPtr->sigtyp().setRep(NULL);

    temptype.setRep(NULL);
    sysProcNum = 0;
    for (l3var5z = 0; l3var5z <= 22; ++l3var5z)
        regSysProc(systemProcNames[l3var5z]);
    sysProcNum = 0;
    temptype = RealType;
    regSysProc(0L /*"was SQRT"*/);
    regSysProc(0L /*"was SIN"*/);
    regSysProc(0L /*"was COS"*/);
    regSysProc(0L /*"was ATAN"*/);
    regSysProc(0L /*"was ASIN"*/);
    regSysProc(0L /*"was LN"*/);
    regSysProc(0L /*"was EXP"*/);
    regSysProc(0414263L /*"     ABS"*/);
    temptype = IntegerType;
    regSysProc(06462655643L /*"   TRUNC"*/);
    regSysProc(0635172455746L /*"  SIZEOF"*/);
    regSysProc(05746466345645746L /*"OFFSETOF"*/);
    regSysProc(0L /*" was SUCC"*/);
    regSysProc(0L /*" was PRED"*/);
    temptype = voidPtr;
    regSysProc(0554154545743L /*"  MALLOC"*/);
    temptype = BooleanType;
    regSysProc(0455746L /*"     EOF"*/);
    temptype = voidPtr;
    regSysProc(0L /*"was REF, unused"*/);
    temptype = BooleanType;
    regSysProc(045575456L /*"    EOLN"*/);
    temptype = IntegerType;
    regSysProc(0L /*" was SETJMP"*/);
    regSysProc(06257655644L /*"   ROUND"*/);
    regSysProc(043416244L /*"    CARD"*/);
    regSysProc(05551564554L /*"   MINEL"*/);
    temptype = voidPtr;
    regSysProc(0606462L /*"     PTR"*/);

    l3var11z.ii = 30;
    l3var11z.ii = (l3var11z.ii & halfWord) | Bits(24,27,28,29);
    programObj = new IdentRec;
    outName.ii = 01257656460656412L /*"*OUTPUT*"*/;
    inName.ii = 012515660656412L /*" *INPUT*"*/;
    symTabPos = 074004;
    programObj->cl = ROUTINEID; // cl left as heap garbage in base.pas
    curVal.ii = 06041634357556054L; /* PASCOMPL */
    programObj->id = curVal.ii;
    programObj->pos() = 0;
    symTab[074000] = leftAlign(curVal.ii);

    entryPtTable[1] = symTab[074000];
    entryPtTable[3] = (Bits(0,1,6,7) | Bits(10,12) | BitRange(14,18) |
                       BitRange(21,25) | Bits(28,30) | Bits(35,36) |
                       Bits(38,39) | Bits(41)); /*"PROGRAM "*/
    entryPtTable[2] = Bits(1);
    entryPtTable[4] = Bits(1);
    entryPtCnt = 5;
    CHILD.push_back((Bits(0,4,6) | BitRange(9,12) | Bits(23,28,29) |
                     BitRange(33,36) | Bits(46))); /*10 24 74001 00 30 74002*/
    moduleOffset = 040001;
    programObj->argList() = NULL;
    programObj->flags() = int64_t();
    programObj->sigtyp().setRep(NULL);
    objBufIdx = 1;
    lookupMode = lookDef;
    outputObjFile();
    outputFile = NULL;
    inputFile = NULL;
    externFileList = NULL;

    l3var7z = new IdentRec;
    lineStartOffset = moduleOffset;
    l3var7z->id = outName.ii;
    l3var7z->offset = 0;
    l3var7z->typ = textType;
    l3var7z->cl = VARID;
    l3var7z->list() = NULL;
    curVal.ii = 01257656460656412L /*"*OUTPUT*"*/;
    l3var7z->value() = allocExtSymbol(l3var11z.ii);
    addToHashTab(l3var7z);
    l3var5z = 1;
    while (SY == EXTERNSY) {
        inSymbol();
        while (SY == IDENT) {
            defExtern();
            inSymbol();
            if (SY == COMMA)
                inSymbol();
        }
        checkSymAndRead(SEMICOLON);
    } /* while SY = EXTERNSY */
    if (outputFile == NULL) {
        savedIdent.ii = curIdent;
        curIdent = outName.ii;
        defExtern();
        curIdent = savedIdent.ii;
    }
    lookupMode = lookUse;
    l3var6z = 40;
    do {
        programme(l3var6z, programObj, false);
    } while (!(SY == PERIOD || CH == 0));
    if (CH != 'D' && CH != 'd') {
        lookup2 = 0;
        lookupMode = lookDef;
    } else {
        freeRegs = halfWord;
        dataCheck = false;
        Statement();
    }
    readToPos80();
    curVal.ii = l3var6z;
    symTab[074003] = (helperNames[25] | Bits(24,27,28,29)) |
                     (curVal.ii & halfWord);
} /* initScalars */

void makeExtFile()
{
    ExprPtr &l2var10z = programme::super.back()->l2var10z;
    IdentRecPtr &workidr = programme::super.back()->workidr;
    l2var10z = new Expr;
    // base.pas smuggles the ExtFileRec pointer through the type word
    // ((*=c-*) mkExpr(NOOP, curExternFile, ...)); store its arena ordinal.
    l2var10z->vt.ii = ord(curExternFile);
    l2var10z->id2 = workidr;
    l2var10z->expr1 = curExpr;
    curExpr = l2var10z;
}

void parseParameters()
{
    IdentRecPtr l3var1z, l3var2z, l3var3z;
    IdClass parClass;
    int64_t l3var5z, l3var6z;
    Symbol l3sym7z;
    bool noComma;
    TPtr expType;
    IdentRecPtr &curIdRec = programme::super.back()->curIdRec;
    int64_t &l2int18z = programme::super.back()->l2int18z;

    lookup2 = lookDef;
    l3var5z = 0;
    lookupMode = lookDef;
    inSymbol();
    l3var2z = NULL;
    if (SY == RPAREN) {
        inSymbol();
        lookup2 = lookUse;
        lookupMode = lookUse;
        return;
    }
    if (SY == IDENT)
        markTypeSym();
    if (not has(Bits(IDENT, TYPESY), SY))
        errAndSkip(errBadSymbol, (skipToSet | Bits(IDENT, RPAREN)));
    lookup2 = lookUse;
    while (has(Bits(IDENT, TYPESY), SY)) {
        if (SY == IDENT)
            markTypeSym();
        l3sym7z = SY;
        if (SY == IDENT)
            parClass = VARID;
        else {
            parClass = ROUTINEID;
        }
        l3var3z = NULL;
        if (SY == TYPESY)
            expType = hashTravPtr->typ;
        else
            expType = IntegerType;
        l3var6z = 0;
        if (SY != IDENT) {
            lookupMode = lookDef;
            inSymbol();
        }
        do {
            if (SY == IDENT) {
                if (isDefined)
                    error(errIdentAlreadyDefined);
                l3var6z = l3var6z + 1;
                l3var1z = new IdentRec;
                l3var1z->id = curIdent;
                l3var1z->offset = curFrameRegTemplate;
                l3var1z->cl = parClass;
                l3var1z->next = symHash[bucket];
                l3var1z->typ = voidType;
                l3var1z->list() = curIdRec;
                l3var1z->value() = l2int18z;
                symHash[bucket] = l3var1z;
                l2int18z = l2int18z + 1;
                if (l3var2z == NULL)
                    curIdRec->argList() = l3var1z;
                else
                    l3var2z->list() = l3var1z;
                l3var2z = l3var1z;
                if (l3var3z == NULL)
                    l3var3z = l3var1z;
                inSymbol();
            } else
                errAndSkip(errNoIdent, skipToSet | Bits(RPAREN, COMMA, COLON));
            noComma = (SY != COMMA);
            if (not noComma) {
                lookupMode = lookDef;
                inSymbol();
            }
        } while (!noComma);
        if (l3sym7z != TYPESY) {
            checkSymAndRead(COLON);
            parseTypeRef(expType, (skipToSet | Bits(IDENT, RPAREN)));
            if (typeSize(expType) != 1)
                l3var5z = l3var6z * typeSize(expType) + l3var5z;
            if (l3var3z != NULL)
                while (l3var3z != curIdRec) /* with l3var3z@ */ {
                    l3var3z->typ = expType;
                    l3var3z = l3var3z->list();
                }
        } else if (l3var3z != NULL)
            while (l3var3z != curIdRec) /* with l3var3z@ */ {
                l3var3z->typ = expType;
                l3var3z = l3var3z->list();
            }

        if (SY == SEMICOLON) {
            lookupMode = lookDef;
            inSymbol();
            if (not has((skipToSet | Bits(IDENT, VARSY, TYPESY)), SY))
                errAndSkip(errBadSymbol, (skipToSet | Bits(IDENT, RPAREN)));
        }
    }
    /* 22276 */
    if (l3var5z != 0) {
        curIdRec->flags() = (curIdRec->flags() | Bits(23));
        l3var6z = l2int18z;
        l2int18z = l2int18z + l3var5z;
        l3var2z = curIdRec->argList();
        /* 22306 */
        while (l3var2z != curIdRec) {
            if (l3var2z->cl == VARID) {
                l3var5z = typeSize(l3var2z->typ);
                if (l3var5z != 1) {
                    l3var2z->value() = l3var6z;
                    l3var6z = l3var6z + l3var5z;
                }
            }
            l3var2z = l3var2z->list();
        }
    }
    /* 22322 */
    checkSymAndRead (RPAREN);
    lookup2 = lookUse;
    lookupMode = lookUse;
} /* parseParameters */

void exitScope(IdentRecPtr arg[128])
{
    IdentRecPtr &workidr = programme::super.back()->workidr;
    IdentRecPtr &scopeBound = programme::super.back()->scopeBound;

    for (int ii = 0; ii <= 127; ++ii) {
        workidr = arg[ii];
        while (workidr != NULL and
              workidr >= scopeBound)
            workidr = workidr->next;
        arg[ii] = workidr;
    }
} /* exitScope */

programme::programme(int64_t & l2arg1z, IdentRecPtr const l2idr2z_, bool bodyBlock_)
    : procName(l2idr2z_)
{
    super.push_back(this);
    localSize = l2arg1z;
    ceRegs = halfWord | Bits(23);   /* halfWord + [23] */
    if (localSize == 0) {
        inSymbol();
        initScalars();
        return;
    }
    preDefHead = reinterpret_cast<IdentRec*>(ptr(0));
    inTypeDef = false;
    typedefPending = false;
    typelist = NULL;
    retSeen = false;
    hasFiles = 0;
    bodyStatSys = statBegSys;
    strLabList = NULL;
    lineNesting = lineNesting + 1;
    labFence = numLabTop;
    do {
        if (SY == CONSTSY) {
            parseDecls(0);
            while  (SY == IDENT) {
                if (isDefined)
                    error(errIdentAlreadyDefined);
                /* workidr@ := [curIdent, curFrameRegTemplate, symHash[bucket],
                   , ENUMID, NIL]; */
                workidr = new IdentRec;
                workidr->id = curIdent;
                workidr->offset = curFrameRegTemplate;
                workidr->next = symHash[bucket];
                workidr->cl = ENUMID;
                workidr->list() = NULL;
                symHash[bucket] = workidr;
                inSymbol();
                if (charClass != ASSIGNOP)
                    error(errBadSymbol);
                else
                    inSymbol();
                parseConstDeclValue(workidr->typ, workidr->high_); // actually value() but need a Word here
                if (workidr->typ == voidType) {
                    error(errNoConstant);
                    workidr->typ = IntegerType;
                    workidr->value() = 1;
                }
                if (SY == SEMICOLON) {
                    lookupMode = lookDef;
                    inSymbol();
                    if (!has((skipToSet | Bits(IDENT)), SY)) {
                        errAndSkip(errBadSymbol, skipToSet | Bits(IDENT));
                    }
                } else {
                    requiredSymErr(SEMICOLON);
                }
            }
        } /* 22511 */
        objBufIdx = 1;
        if (SY == TYPEDEFSY) {  // base.pas 7936: the `typedef` keyword, NOT a
                                // type-name (TYPESY); otherwise TYPEDEFSY is
                                // never consumed and the decl loop spins.
            inTypeDef = true;
            typelist = NULL;
            parseDecls(0);
            while (SY == IDENT) {
                if (isDefined)
                    error(errIdentAlreadyDefined);
                ii = bucket;
                l2var12z = curIdent;
                inSymbol();
                if (charClass != ASSIGNOP)  // base.pas 7948: the `=` in a
                    error(errBadSymbol);    // typedef is a single `=` (ASSIGNOP);
                else                        // `==` (EQOP) is the equality op.
                    inSymbol();
                parseTypeRef(l2typ13z, skipToSet | Bits(SEMICOLON));
                curIdent = l2var12z;
                if (knownInType(curIdRec)) {
                    l2typ14z = curIdRec->typ;
                    if (l2typ14z.rep()->base == BooleanType) {
                        if (l2typ13z.p.pk != kindPtr) {
                            prevErrPos = 0;
                            error(78); /* errPredefinedAsPointer */
                            printf(": ");
                            printTextWord(l2var12z);
                            printf(" in line %ld\n", curIdRec->offset);
                        }
                        l2typ14z.rep()->base = l2typ13z.rep()->base;
                    } else {
                        l2typ14z.rep()->base = l2typ13z;
                        curIdRec->typ = l2typ13z;
                    }
                    hash(typelist, curIdRec);
                } else {
                    curIdRec = new IdentRec;
                    curIdRec->id = l2var12z;
                    curIdRec->offset = curFrameRegTemplate;
                    curIdRec->typ = l2typ13z;
                    curIdRec->cl = TYPEID;
                } /* 22574 */
                curIdRec->next = symHash[ii];
                symHash[ii] = curIdRec;
                lookupMode = lookDef;
                checkSymAndRead(SEMICOLON);
                hashTravPtr = NULL;
                markTypeSym();
                if (SY == TYPESY)
                    break;
            } /* 22602 */
            while (typelist != NULL) {
                l2var12z = typelist->id;
                curIdRec = typelist;
                prevErrPos = 0;
                error(79); /* errNotFullyDefined */
                printf(": ");
                printTextWord(l2var12z);
                printf(" in line %ld\n", curIdRec->offset);
                typelist = typelist->next;
            }
        } /* TYPESY -> 22612 */
        inTypeDef = false;
        curExpr = NULL;
    if (SY == VARSY) {
        parseDecls(0);
        /*22617*/
        do {
            workidr = NULL;
            /*22620*/
            do {
                if (SY == IDENT) {
                    curIdRec = new IdentRec;
                    if (isDefined)
                        error(errIdentAlreadyDefined);
                    curIdRec->id = curIdent;
                    curIdRec->offset = curFrameRegTemplate;
                    curIdRec->next = symHash[bucket];
                    curIdRec->cl = VARID;
                    curIdRec->list() = NULL;
                    symHash[bucket] = curIdRec;
                    inSymbol();
                    if (workidr == NULL)
                        workidr = curIdRec;
                    else
                        l2var4z->list() = curIdRec;
                    l2var4z = curIdRec;
                } else
                    error(errNoIdent);
                if (SY == LBRACK) {
                    inSymbol();
                    if (SY != INTCONST ||
                        curToken.ii < 0 ||
                        curToken.ii > 77777)
                        error(errNumberTooLarge);
                    curIdRec->value() = curToken.ii;
                    curIdRec->offset = 0;
                    inSymbol();
                    checkSymAndRead(RBRACK);
                } else curIdRec->value() = -1;
                if (SY != COMMA && SY != COLON)
                    errAndSkip(1, skipToSet | Bits(IDENT, COMMA));
                l2bool8z = SY != COMMA;
                if (not l2bool8z) {
                    lookupMode = lookDef;
                    inSymbol();
                };
            } while (!l2bool8z);
            checkSymAndRead(COLON);
            parseTypeRef(l2typ13z, skipToSet | Bits(IDENT, SEMICOLON));
            jj = typeSize(l2typ13z);
            while (workidr != NULL) /* do with workidr@ do */ {
                curIdRec = workidr->list();
                workidr->typ = l2typ13z;
                workidr->list() = NULL;
                l2bool8z = true;
                if (curProcNesting == 1) {
                    curExternFile = externFileList;
                    l2var12z = workidr->id;
                    curVal.ii = jj;
                    toAlloc = (curVal.ii & halfWord) | 047000000;
                    while (l2bool8z and curExternFile != NULL) {
                        if (curExternFile->id == l2var12z) {
                            l2bool8z = false;
                            if (curExternFile->line == 0) {
                                curVal.ii = curExternFile->offset;
                                workidr->value() = allocExtSymbol(toAlloc);
                                curExternFile->line = lineCnt;
                            }
                        } else {
                            curExternFile = curExternFile->next;
                        }
                    }
                } /* 22731 */
                if (l2bool8z && workidr->value() == -1) {
                    workidr->value() = localSize;
                    if (PASINFOR.listMode == 3) {
                        printf("%25s", "VARIABLE ");
                        printTextWord(workidr->id);
                        printf(" OFFSET (%ld) %05loB. WORDS=%05loB\n", curProcNesting,
                                localSize, jj);
                    }
                    localSize = localSize + jj;
                    curExternFile = NULL;
                } /*22764*/
                if (isFileType(l2typ13z))
                    makeExtFile();
                workidr = curIdRec;
            } /* 22771 */
            lookupMode = lookDef;
            checkSymAndRead(SEMICOLON);
            if (SY != IDENT and not has(skipToSet, SY)
                and not (bodyBlock_ and has(statBegSys, SY)))
                errAndSkip(errBadSymbol, skipToSet | Bits(IDENT));
            /* base.pas 8079: a leading type-IDENT after ';' starts a new
               C-style routine decl, not another variable; markTypeSym re-
               resolves it (scope-agnostic bucket walk, hence the reset) so
               the loop bails and the routine-decl loop below picks it up. */
            hashTravPtr = NULL;
            markTypeSym();
        } while (SY == IDENT and
                 not (bodyBlock_ and hashTravPtr != NULL and
                      hashTravPtr->cl != TYPEID));
    } /* VARSY -> 23003 */
    if (curProcNesting == 1) {
        if (outputFile != NULL) {
            workidr = outputFile;
            curExternFile = fileForOutput;
            makeExtFile();
        }
        if (inputFile != NULL) {
            workidr = inputFile;
            curExternFile = fileForInput;
            makeExtFile();
        }
    }
    // base.pas just sets hasFiles := 0 here; the file-init code is emitted once,
    // by defineRoutine's formOperator(FILEINIT). The upstream extra call here
    // duplicated the file-close block.
    hasFiles = 0;
    if (curProcNesting == 1) {
        curExternFile = externFileList;
        while (curExternFile != NULL) {
            if (curExternFile->line == 0) {
                error(80); /* errUndefinedExternFile */
                printTextWord(curExternFile->id);
                putchar('\n');
            }
            curExternFile = curExternFile->next;
        }
    } /*23035*/
    outputObjFile();
    markTypeSym();
    while (SY == TYPESY) {
        done = hashTravPtr->typ == voidType;
        /* For the new C-style syntax 'RETTYPE NAME(args);' the current TYPESY
           names the return type; stash it before inSymbol clobbers
           hashTravPtr.  voidType (i.e. done) marks a procedure. */
        typedRetType = hashTravPtr->typ;
        if (curFrameRegTemplate == 7) {
            error(81); /* errProcNestingTooDeep */
        }
        lookupMode = lookDef;
        inSymbol();
        if (SY != IDENT) {
            error(errNoIdent);
            curIdRec = uProcPtr;
            isPredefined = false;
        } else {
            if (isDefined) {
                if (hashTravPtr->cl == ROUTINEID and
                    hashTravPtr->list() == NULL and
                    hashTravPtr->preDefLink() != NULL and
                    ((hashTravPtr->typ == voidType) == done)) {
                    isPredefined = true;
                } else {
                    isPredefined = false;
                    error(errIdentAlreadyDefined);
                    printErrMsg(82); /* errPrevDeclWasNotForward */
                }
            } else
                isPredefined = false;
        } /* 23103 */
        if (not isPredefined) {
            curIdRec = new IdentRec;
            curIdRec->id = curIdent;
            curIdRec->offset = curFrameRegTemplate;
            curIdRec->next = symHash[bucket];
            curIdRec->typ = voidType;
            symHash[bucket] = curIdRec;
            curIdRec->cl = ROUTINEID;
            curIdRec->list() = NULL;
            curIdRec->value() = 0;
            curIdRec->argList() = NULL;
            curIdRec->preDefLink() = NULL;
            curIdRec->sigtyp() = voidType;
            if (declEntry)
                curIdRec->flags() = BitRange(0,15) | Bits(22);
            else
                curIdRec->flags() = BitRange(0,15);
            curIdRec->pos() = 0;
            curFrameRegTemplate = curFrameRegTemplate + frameRegTemplate;
            if (done)
                l2int18z = 3;
            else
                l2int18z = 4;
            curProcNesting = curProcNesting + 1;
            inSymbol();
            if (6 < curProcNesting)
                error(81); /* errProcNestingTooDeep */
            if (not has(Bits(LPAREN, SEMICOLON, COLON), SY))
                errAndSkip(errBadSymbol, skipToSet | Bits(LPAREN, SEMICOLON, COLON));
            hadParens = SY == LPAREN;
            if (hadParens)
                parseParameters();
            if (not done) {
                if (typedRetType != voidType) {
                    /* New C-style: return type stashed at the loop head;
                       no ':TYPE' suffix expected. */
                    curIdRec->typ = typedRetType;
                    if (typeSize(curIdRec->typ) != 1)
                        error(errTypeMustNotBeFile);
                } else if (SY != COLON)
                    errAndSkip(106 /*:*/, skipToSet | Bits(SEMICOLON));
                else {
                    inSymbol();
                    parseTypeRef(curIdRec->typ, skipToSet | Bits(SEMICOLON));
                    if (typeSize(curIdRec->typ) != 1)
                        error(errTypeMustNotBeFile);
                }
            }
        } else /*23167*/ {
            l2int18z = hashTravPtr->level();
            curFrameRegTemplate = curFrameRegTemplate + indexreg[1];
            curProcNesting = curProcNesting + 1;
            if (preDefHead == hashTravPtr) {
                preDefHead = hashTravPtr->preDefLink();
            } else {
                curIdRec = preDefHead;
                while (hashTravPtr != curIdRec) {
                    workidr = curIdRec;
                    curIdRec = curIdRec->preDefLink();
                }
                workidr->preDefLink() = hashTravPtr->preDefLink();
            }
            hashTravPtr->preDefLink() = NULL;
            curIdRec = hashTravPtr->argList();
            if (curIdRec != NULL) {
                while (curIdRec != hashTravPtr) {
                    addToHashTab(curIdRec);
                    curIdRec = curIdRec->list();
                }
            }
            curIdRec = hashTravPtr;
            setup(scopeBound);
            inSymbol();
            hadParens = false;
            if (SY == LPAREN and curIdRec->argList() == NULL) {
                hadParens = true;
                inSymbol();
                checkSymAndRead(RPAREN);
            }
        } /* 23224 */
        if (SY == BEGINSY) {
            if (curIdRec->argList() == NULL and not hadParens)
                error(42); /* errNoParamList */
            setup(scopeBound);
            inSymbol();
            programme(l2int18z, curIdRec, true);
            rollup(scopeBound);
            exitScope(symHash);
            exitScope(fieldHash);
            goto L23301;
        }
        checkSymAndRead(SEMICOLON);
        if (curIdent == litForward) {
            if (isPredefined)
                error(83); /* errRepeatedPredefinition */
            curIdRec->level() = l2int18z;
            curIdRec->preDefLink() = preDefHead;
            preDefHead = curIdRec;
        } else if (SY == EXTERNSY or
                   curIdent == litFortran or
                   curIdent == litAssembler) {
            if (SY == EXTERNSY) {
                curVal.ii = Bits(20);
            } else if (curIdent == litAssembler) {
                curVal.ii = Bits(20,26);
            } else if (checkFortran) {
                curVal.ii = Bits(21,24);
                checkFortran = false;
            } else {
                curVal.ii = Bits(21);
            }
            curIdRec->flags() = curIdRec->flags() | curVal.ii;
        } else /* 23257 */ {
            error(errBadSymbol);
        } /* 23277 */
        inSymbol();
        checkSymAndRead(SEMICOLON);
L23301:
        workidr = curIdRec->argList();
        if (workidr != NULL) {
            while (workidr != curIdRec) {
                scopeBound = NULL;
                hash(scopeBound, workidr);
                workidr = workidr->list();
            }
        } /* 23314 */
        curFrameRegTemplate = curFrameRegTemplate - indexreg[1];
        curProcNesting = curProcNesting - 1;
        markTypeSym();
    } /* 23320 */
    if (curProcNesting == 1 and curExpr != NULL) {
        hasFiles = 1;
    } else if (curProcNesting == 1) {
        hasFiles = 0;
    }
    markTypeSym();
    if (CH == 0) return;
    if (bodyBlock_) {
        if (not has((bodyStatSys | blockBegSys), SY) and
            not has(Bits(TYPESY, ENDSY), SY))
            errAndSkip(84 /* errErrorInDeclarations */,
                       skipToSet | bodyStatSys | blockBegSys | Bits(ENDSY));
    } else if (not has(blockBegSys, SY) and (SY != TYPESY))
        errAndSkip(84 /* errErrorInDeclarations */, skipToSet);
    } while (not ((bodyBlock_ and (has(bodyStatSys, SY) or
                                  has(Bits(TYPESY, ENDSY), SY))) or
                  (not bodyBlock_ and (has(statBegSys, SY) or
                                      (SY == TYPESY)))));
    if (preDefHead != ptr(0))  {
        error(85); /* errNotFullyDefinedProcedures */
        while (preDefHead != ptr(0)) {
            printTextWord(preDefHead->id);
            preDefHead = preDefHead->preDefLink();
        }
        putchar('\n');
    }
    lookup2 = lookUse;
    lookupMode = lookUse;
    defineRoutine(bodyBlock_);
    if (curProcNesting > 1 and
        not retSeen and (procName->typ != voidType)) {
        printf(" above function must return a value\n");
        error(200);
    }
    done = true;
    while (numLabTop > labFence) {
        if (not numLabs[numLabTop].defined) {
            printf(" %ld:", int64_t(numLabs[numLabTop].id.ii));
            done = false;
        }
        numLabTop = numLabTop - 1;
    }
    if (not done) {
        printTextWord(procName->id);
        error(18); /* errLabelNotDefined */
    }
    l2arg1z = sizeCount;
    /* 23364 */
} /* programme */

struct initTables {
    int64_t idx, jdx;

    void initInsnTemplates() {
        Insn l3var1z;
        Operator l3var2z;

        for (l3var1z = ATX; l3var1z <= MADDJ; succ(l3var1z))
            InsnTemp[l3var1z] = l3var1z * 010000;
        InsnTemp[ELFUN] = 0500000;
        jdx = KUTC;
        for (l3var1z = UTC; l3var1z <= VJM; succ(l3var1z)) {
            InsnTemp[l3var1z] = jdx;
            jdx = (jdx + 0100000);
        }
        for (idx=1; idx <= 15; ++idx)
            indexreg[idx] = idx * frameRegTemplate;
        jumpType = InsnTemp[UJ];
        for (l3var2z = MUL; l3var2z<=ASSIGNOP; succ(l3var2z)) {
            opFlags[l3var2z] = opfCOMM;
            opToInsn[l3var2z] = 0;
            if (has(Bits(MUL, RDIVOP, PLUSOP, MINUSOP), l3var2z)) {
                opToMode[l3var2z] = 3;
            } else if (has(Bits(IDIVOP, IMODOP), l3var2z)) {
                opToMode[l3var2z] = 2;
            } else if (has(Bits(IMULOP, INTPLUS, INTMINUS), l3var2z)) {
                opToMode[l3var2z] = 1;
            } else {
                opToMode[l3var2z] = 0;
            }
        }
        opToInsn[MUL] = InsnTemp[AMULX];
        opToInsn[RDIVOP] = InsnTemp[ADIVX];
        opToInsn[IDIVOP] = 17; /* P/DI */
        opToInsn[IMODOP] = 11; /* P/MD */
        opToInsn[PLUSOP] = InsnTemp[ADD];
        opToInsn[MINUSOP] = InsnTemp[SUB];
        opToInsn[IMULOP] = InsnTemp[AMULX];
        opToInsn[SETAND] = InsnTemp[AAX];
        opToInsn[SETXOR] = InsnTemp[AEX];
        opToInsn[SETOR] = InsnTemp[AOX];
        opToInsn[INTPLUS] = InsnTemp[ADD];
        opToInsn[INTMINUS] = InsnTemp[SUB];
        opToInsn[SHLEFT] = 98;
        opToInsn[SHRIGHT] = 99;
        opFlags[ANDOP] = opfAND;
        opFlags[IDIVOP] = opfDIV;
        opFlags[OROP] = opfOR;
        opFlags[IMULOP] = opfMULMSK;
        opFlags[IMODOP] = opfMOD;
        opFlags[ASSIGNOP] = opfASSN;
        opFlags[SHLEFT] = opfSHIFT;
        opFlags[SHRIGHT] = opfSHIFT;
    } /* initInsnTemplates */

    void regResWord(int64_t l4arg1z) {
        KeyWord * kw;
        Word l4var2z;
        curVal.ii = l4arg1z;
        curVal.ii = (curVal.ii % 65535) % 128;
        l4var2z.ii = l4arg1z;
        kw = new KeyWord;
        kw->w = l4var2z;
        kw->sym = SY;
        kw->op = charClass;
        kw->next = KeyWordHashTabBase[curVal.ii];
        KeyWordHashTabBase[curVal.ii] = kw;
    } /* regResWord */

    void regKeyWords() {
        SY = EXPROP;
        charClass = INOP;
        regResWord(toText("IN"));
        SY = CONSTSY;
        charClass = NOOP;
        for (idx = 0; idx <= 20; ++idx) {
            if (SY != TYPESY)
                regResWord(resWordNameBase[idx]);
            succ(SY);
        }
    } /* regKeyWords */

    void initArrays() {
        // int64_t l3var1z;
        int64_t l3var2z;
        FcstCnt = 0;
        FcstTotal = 0;
        for (idx = 3; idx <= 6; ++idx) {
            l3var2z = idx - 2;
            for (jdx=1; jdx <= l3var2z; ++jdx)
                frameRestore[idx][jdx] = 0;
        }
        for (idx=1; idx <= 99; ++idx)
            helperMap[idx] = 0;
    } /* initArrays */

    void initSets() {
        skipToSet = (blockBegSys | statBegSys) & ~ Bits(CASESY);
        bigSkipSet = skipToSet | statEndSys;
    } /* initSets */

    initTables () {
        initArrays();
        initInsnTemplates();
        initSets();
        memcpy(&koi2text['*'],
               "\012\036\000\035\000\017" // 052-057 (* + , - . /)
               "\020\021\022\023\024\025\026\027" // 060-067 (0 - 7)
               "\030\031\000\000\000\000\000\000" // 070-077 (8 9 : ; < = > ?)
               "\000\041\042\043\044\045\046\047" // 100-107 (@ - G)
               "\050\051\052\053\054\055\056\057" // 110-117 (H - O)
               "\060\061\062\063\064\065\066\067" // 120-127 (P - W)
               "\070\071\072\000\000\000\000\000" // 130-137 (X Y Z [ \ ] ^ _)
               "\000\041\042\043\044\045\046\047" // 140-147 (` - g)
               "\050\051\052\053\054\055\056\057" // 150-157 (h - o)
               "\060\061\062\063\064\065\066\067" // 160-167 (p - w)
               "\070\071\072\000\000\000\000\000" // 170-177 (x y z { | } ~ )
               , 86);
        koi2text['_'] = koi2text['*']; // base.pas: iso2text['_'] := iso2text['*']
        memcpy(&koi2text[0300],
               "\077\041\002\003\004\045\005\006" // 300-307 (ю - г)
               "\070\007\013\053\014\055\050\057" // 310-317 (х - о)
               "\034\015\060\043\064\071\016\042" // 320-327 (п - в)
               "\032\037\040\073\074\075\076\000" // 330-337 (ь - ъ)
               "\077\041\002\003\004\045\005\006" // 340-347 (Ю - Г)
               "\070\007\013\053\014\055\050\057" // 350-357 (Х - О)
               "\034\015\060\043\064\071\016\042" // 360-367 (П - В)
               "\032\037\040\073\074\075\076\000" // 370-377 (Ь - Ъ)
               , 64);
        CHILD.clear();
        for (jdx = 1; jdx <= 10; ++jdx)
            CHILD.push_back(0);
        for (idx = 0; idx <= 127; ++idx) {
            symHash[idx] = NULL;
            fieldHash[idx] = NULL;
            KeyWordHashTabBase[idx] = NULL;
        }
        regKeyWords();
        numLabTop = 0;
        totalErrors = 0;
        heapCallsCnt = 0;
        putLeft = true;
        readNext = true;
        curFrameRegTemplate = frameRegTemplate;
        curProcNesting = 1;
    } /* initTables */
};

void finalize()
{
    int64_t idx, cnt;
    int64_t sizes[11];

    sizes[1] = 1;
    sizes[2] = symTabPos - 074000 - 1;
    sizes[5] = longSymCnt;
    sizes[6] = moduleOffset - 040000;
    sizes[8] = FcstCnt;
    sizes[3] = 0;
    sizes[4] = 0;
    sizes[7] = 0;
    sizes[9] = lookup2;
    sizes[10] = lookupMode;
    curVal.ii = moduleOffset - 040000;
    symTab[074001] = 041000000 | curVal.ii;
    // Forming the compact form of the module header.
    CHILD[7] = sizes[1] | (sizes[2] << 12);
    CHILD[8] = sizes[5] << 30 | sizes[9] << 15 | sizes[10];
    CHILD[9] = sizes[8] << 30 | sizes[7] << 15 | sizes[6];
    /*
    reset(FCST);
    while not eof(FCST) do {
        write(CHILD, FCST@);
        get(FCST);
    };
    */
    CHILD.insert(CHILD.end(), FCST.begin(), FCST.end());
    curVal.ii = (symTabPos - 070000L) * 0100000000L;
    for (cnt = 1; cnt <= longSymCnt; ++cnt) {
        idx = longSymTabBase[cnt];
        symTab[idx] |= curVal.ii & leftAddr;
        curVal.ii = (curVal.ii + 0100000000L);
    }
    symTabPos = symTabPos - 1;
    for (cnt = 074000; cnt <= symTabPos; ++cnt)
        CHILD.push_back(symTab[cnt]);
    for (cnt = 1; cnt <= longSymCnt; ++cnt)
        CHILD.push_back(longSyms[cnt]);
    if (allowCompat) {
        printf("%6ld LINES STRUCTURE ", lineCnt - 1);
        for (idx=1; idx <=10; ++idx)
            printf("%ld ", sizes[idx]);
        putchar('\n');
    }
    entryPtTable[entryPtCnt] = 0;

} /* finalize */

void usage ()
{
    printf("%s\n", boilerplate);
    printf("Usage:\n");
    printf("    %s [option...] infile [outfile]\n", progname);
    printf("Options:\n");
    printf("    -a0 -a1 -a2         Output encoding for strings:\n");
    printf("                        -a0: UTF-8\n");
    printf("                        -a1: KOI-8\n");
    printf("                        -a2: KOI-7 (aka ISO, default)\n");
    printf("    -b0 -b1 ... -b4     Size of file buffer, in 256-word chunks\n");
    printf("    -c- -c+             Disable/enable checking of data types\n");
    printf("    -d0 -d1 ... -d15    Bitmask of debug flags:\n");
    printf("                        -d1: Trace function calls\n");
    printf("                        -d2: Enable debug() as writeln()\n");
    printf("                        -d4: Enable code enclosed in {=Z-}/{=Z+}\n");
    printf("                        -d8: Invoke Pascal Debugger\n");
    printf("    -e- -e+             Make procedures external (-e+) or local (-e-)\n");
    printf("    -f- -f+             Compile procedures as Pascal (-f-) or Fortran (-f+)\n");
    printf("    -k0 -k1 ... -k23    Heap size in 1024-word chunks (default -k4)\n");
    printf("    -l0 -l1 -l2 -l3     Listing mode:\n");
    printf("                        -l0: No listing, only error messages\n");
    printf("                        -l1: Print listing with relative addresses per line\n");
    printf("                        -l2: Also print generated object code\n");
    printf("                        -l3: Also print offsets for variables and fields\n");
    printf("    -m+ -m-             Optimize integer multiplication (positives only)\n");
    printf("    -p+ -p-             Enable/disable debug information and crash dump\n");
    printf("    -r+ -r-             Compare reals with predefined tolerance\n");
    printf("    -s0                 Use stars for commons (like *foobar*)\n");
    printf("    -s1                 Append one star for external names (like foobar*)\n");
    printf("    -s2                 No stars for external names (like foobar)\n");
    printf("    -s3                 Re-start line numbering from this line\n");
    printf("    -s4                 Print columns 73-80 as line tags\n");
    printf("    -s5                 Disable external files\n");
    printf("    -s6                 Pack record fields from right to left\n");
    printf("    -s7                 Disable pointer checking\n");
    printf("    -s8                 Disable checking for stack overflow\n");
    printf("    -s9                 Unknown\n");
    printf("    -t+ -t-             Enable/disable range checks\n");
    printf("    -u- -u+             Set length of source lines: 120 or 72 columns\n");
    printf("    -y- -y+             Disable/enable non-standard syntax\n");
    printf("    -v                  Output version information and exit\n");
    printf("    -h                  Display this help and exit\n");
    exit(0);
}

void initOptions(int argc, char **argv)
{
    PASINFOR.startOffset -= 040000;
    commentModeCH = ' ';
    lineNesting = 0;
    CH = ' ';
    linePos = 0;
    prevErrPos = 0;
    errsInLine = 0;
    lineCnt = 1;
    checkFortran = false;
    bool110z = false;
    lookupMode = 1;
    lookup2 = 1;
    moduleOffset = 16384;
    lineStartOffset = 16384;
    condLabCnt = 1;
    inCallArgs = false;
    dataCheck = false;
    heapSize = 100;
    forValue = true;
    atEOL = false;
    doPMD = true; // not (42 in curVal.ii);
    checkTypes = true;
    fixMult = true;
    checkBounds = true; // not (44 in curVal.ii);
    declEntry = false;
    errors = false;
    allowCompat = false;
    fileBufSize = 1;
    charEncoding = 2;
    longSymCnt = 0;
    symTabCnt = 0;

    // Get base name of the program.
    progname = strrchr(argv[0], '/');
    progname = progname ? progname+1 : argv[0];

    for (;;) {
        switch (getopt(argc, argv, "vVhe:p:t:c:r:m:y:u:f:a:d:k:b:s:l:")) {
        case EOF:
            break;
        case 'a':
            charEncoding = strtoul(optarg, 0, 0);
            if (charEncoding > 2) {
                fprintf(stderr, "%s: Bad option -a\n", progname);
                exit(-1);
            }
            continue;
        case 'b':
            fileBufSize = strtoul(optarg, 0, 0);
            if (fileBufSize > 4) {
                fprintf(stderr, "%s: Bad option -b\n", progname);
                exit(-1);
            }
            continue;
        case 'c':
            checkTypes = (optarg[0] == '+');
            continue;
        case 'd':
            curVal.ii = strtoul(optarg, 0, 0);
            if (curVal.ii > 15) {
                fprintf(stderr, "%s: Bad option -d\n", progname);
                exit(-1);
            }
            optSflags.ii = (optSflags.ii & BitRange(0, 40)) | (curVal.ii & BitRange(41, 47));
            continue;
        case 'e':
            declEntry = (optarg[0] == '+');
            continue;
        case 'f':
            checkFortran = (optarg[0] == '+');
            continue;
        case 'k':
            heapSize = strtoul(optarg, 0, 0);
            if (heapSize > 23) {
                fprintf(stderr, "%s: Bad option -k\n", progname);
                exit(-1);
            }
            continue;
        case 'l':
            PASINFOR.listMode = strtoul(optarg, 0, 0);
            if (PASINFOR.listMode > 3) {
                fprintf(stderr, "%s: Bad option -l\n", progname);
                exit(-1);
            }
            continue;
        case 'm':
            fixMult = (optarg[0] == '+');
            continue;
        case 'p':
            doPMD = (optarg[0] == '+');
            continue;
        case 'r':
            // Fuzzy real comparison was removed from base.pas; option is a no-op.
            continue;
        case 's':
            curVal.ii = strtoul(optarg, 0, 0);
            if (curVal.ii > 9) {
                fprintf(stderr, "%s: Bad option -s\n", progname);
                exit(-1);
            }
            if (curVal.ii == 3) {
                lineCnt = 1;
            } else if (4 <= curVal.ii && curVal.ii <= 9) {
                optSflags.ii = optSflags.ii | Bits(curVal.ii - 3);
            }
            continue;
        case 't':
            checkBounds = (optarg[0] == '+');
            continue;
        case 'u':
            // Source line length is a compile-time constant (maxLineLen);
            // base.pas has no runtime override, so this option is a no-op.
            continue;
        case 'y':
            allowCompat = (optarg[0] == '+');
            continue;
        case 'v':
            printf("%s\n", boilerplate);
            exit(0);
        case 'V':
            ++verbose;
            continue;
        default:
            usage();
        }
        break;
    }
    argc -= optind;
    argv += optind;
    if (argc < 1 || argc > 2)
        usage();

    // Open input file on stdin.
    if (strcmp(argv[0], "-") != 0) {
        if (freopen(argv[0], "r", stdin) == NULL) {
            fprintf(stderr, "%s: Cannot open input file\n", progname);
            perror(argv[0]);
            exit(-1);
        }
    }

    // Open output file on stdout.
    if (argc > 1) {
        outFileName = argv[1];
        unlink(outFileName);
    }
} /* initOptions */

int main(int argc, char **argv)
{
    // Data Initializations moved here
    blockBegSys = Bits(CONSTSY, TYPEDEFSY, VARSY, TYPESY) | Bits(BEGINSY);
    statBegSys = Bits(IDENT, EXPROP, LPAREN, INTCONST)
        | Bits(REALCONST, CHARCONST, STRINGSY, LBRACK)
        | Bits(BEGINSY, IFSY, SWITCHSY, DOSY)
        | Bits(WHILESY, FORSY, WITHSY, GOTOSY)
        | Bits(BREAKSY, CONTSY, SEMICOLON);
    statEndSys = Bits(SEMICOLON, ENDSY, ELSESY, WHILESY);
    lvalOpSet = Bits(GETELT, GETVAR, op37, GETFIELD) | Bits(DEREF, FILEPTR);

    funcInsn[fnABS] = KAMX;
    funcInsn[fnTRUNC] = KADD+ZERO;
    funcInsn[fnROUND] = macro + mcROUND;
    funcInsn[fnCARD] = KACX;
    funcInsn[fnMINEL] = macro + mcMINEL;
    funcInsn[fnMALLOC] = macro + mcMALLOC;
    funcInsn[fnPTR] = KAAX+MANTISSA;
    funcInsn[fnABSI] = KAMX;

    for (int i = 0; i < 128; ++i) {
        charSymTabBase[i] = NOSY;
        chrClassTabBase[i] = NOOP;
    }
    for (int i = 0; i < 10; ++i) {
        charSymTabBase[i+'0'] = INTCONST;
        chrClassTabBase[i+'0'] = ALNUM;
    }
    for (int i = 0; i < 26; ++i) {
        charSymTabBase[i+'A'] = IDENT;
        chrClassTabBase[i+'A'] = ALNUM;
        charSymTabBase[i+'a'] = IDENT;
        chrClassTabBase[i+'a'] = ALNUM;
    }

    for (int i = 0300; i < 0337; ++i) {
        charSymTabBase[i] = IDENT;
        chrClassTabBase[i] = ALNUM;
        charSymTabBase[i+040] = IDENT;
        chrClassTabBase[i+040] = ALNUM;
    }
    chrClassTabBase['_'] = ALNUM;
    charSymTabBase['\''] = CHARCONST;
    charSymTabBase['_'] = IDENT;
    charSymTabBase['<'] = EXPROP;
    charSymTabBase['>'] = EXPROP;
    chrClassTabBase['+'] = PLUSOP;
    chrClassTabBase['-'] = MINUSOP;
    chrClassTabBase['*'] = MUL;
    chrClassTabBase['/'] = RDIVOP;
    chrClassTabBase['%'] = IMODOP;
    chrClassTabBase['='] = ASSIGNOP;
    chrClassTabBase['&'] = SETAND;
    chrClassTabBase['|'] = SETOR;
    chrClassTabBase['^'] = SETXOR;
    chrClassTabBase[037] = BITNEGOP;  // '~': BESM-6 code 037 (unicode_to_koi8),
                                      // not ASCII 0176 -- base.pas `chrClass['~']`
    chrClassTabBase['>'] = GTOP;
    chrClassTabBase['<'] = LTOP;
    chrClassTabBase['!'] = NOTOP;
    chrClassTabBase['?'] = CONDOP;
    charSymTabBase['+'] = EXPROP;
    charSymTabBase['-'] = EXPROP;
    charSymTabBase['|'] = EXPROP;
    charSymTabBase['*'] = EXPROP;
    charSymTabBase['/'] = EXPROP;
    charSymTabBase['%'] = EXPROP;
    charSymTabBase['&'] = EXPROP;
    charSymTabBase[','] = COMMA;
    charSymTabBase['.'] = PERIOD;
    charSymTabBase['^'] = EXPROP;
    charSymTabBase['('] = LPAREN;
    charSymTabBase[')'] = RPAREN;
    charSymTabBase[';'] = SEMICOLON;
    charSymTabBase['['] = LBRACK;
    charSymTabBase[']'] = RBRACK;
    charSymTabBase['='] = BECOMES;
    charSymTabBase[':'] = COLON;
    charSymTabBase['!'] = EXPROP;
    charSymTabBase[037] = EXPROP;     // '~' -> BESM-6 037 (see chrClassTabBase)
    charSymTabBase['?'] = EXPROP;

    intOpMap[MUL] = IMULOP;
    intOpMap[RDIVOP] = IDIVOP;
    intOpMap[IMODOP] = IMODOP;
    intOpMap[PLUSOP] = INTPLUS;
    intOpMap[MINUSOP] = INTMINUS;

    // Operator precedence table (base.pas 8649): default precNone, then the
    // per-operator levels used by parsePrc/getPrec.  Without this every EXPROP
    // operator reads back precAssign(0), collapsing `a + b` into an op-assign
    // (RMWASSIGN) node.
    for (int i = 0; i < 64; ++i)
        opPrec[i] = precNone;
    opPrec[CONDOP] = precCond;
    opPrec[OROP] = precOr;
    opPrec[ANDOP] = precAnd;
    opPrec[SETOR] = precBitOr;
    opPrec[SETXOR] = precBitXor;
    opPrec[SETAND] = precBitAnd;
    opPrec[NEOP] = precEq;
    opPrec[EQOP] = precEq;
    opPrec[LTOP] = precRel;
    opPrec[GEOP] = precRel;
    opPrec[GTOP] = precRel;
    opPrec[LEOP] = precRel;
    opPrec[INOP] = precRel;
    opPrec[SHLEFT] = precShift;
    opPrec[SHRIGHT] = precShift;
    opPrec[PLUSOP] = precAdd;
    opPrec[MINUSOP] = precAdd;
    opPrec[MUL] = precMul;
    opPrec[RDIVOP] = precMul;
    opPrec[IDIVOP] = precMul;
    opPrec[IMODOP] = precMul;

    // Main program starts here

    // L0 by default: no listing, only errors
    PASINFOR.listMode = 0;
    initOptions(argc, argv);
    if (PASINFOR.listMode != 0)
        printf("%s\n", boilerplate);
    curInsnTemplate = 0;
    initTables();
    litAssembler = toText("ASSEMBLE");
    litForward = toText("FORWARD");
    litFortran = toText("FORTRAN");
    litOct = toText("OCT");
    PASINPUT = ugetc(pasinput);
    try {
        programme(curInsnTemplate, hashTravPtr);
    } catch (int foo) {
        if (foo == 9999) goto L9999;
    }
    if (errors) {
L9999:  printf(" IN %ld LINES %ld ERRORS\n", lineCnt-1, totalErrors);
        exit(1);
    } else {
        finalize();
        // Dump CHILD here
        FILE *f = fopen(outFileName, "w");
        if (f == NULL) {
            fprintf(stderr, "%s: Cannot open output file\n", progname);
            perror(outFileName);
            exit(-1);
        }
        fwrite("BESM6\0", 6, 1, f);
        for (size_t i = 7; i < CHILD.size(); ++i) {
            for (int j = 40; j >= 0; j -= 8)
                fputc((CHILD[i] >> j) & 0xFF, f);
        }
        fclose(f);
        exit(0);
    }
}

int64_t resWordNameBase[21] = {
        04357566364L             /*"   CONST"*/,
        064716045444546L         /*" TYPEDEF"*/,
        0664162L                 /*"     VAR"*/,
        0L                       /*"was FUNCTION"*/,
        045566555L               /*"    ENUM"*/,
        01212604143534544L       /*"**PACKED"*/,
        0636462654364L           /*"  STRUCT"*/,
        05146L                   /*"      IF"*/,
        0636751644350L           /*"  SWITCH"*/,
        06750515445L             /*"   WHILE"*/,
        0465762L                 /*"     FOR"*/,
        067516450L               /*"    WITH"*/,
        047576457L               /*"    GOTO"*/,
        045546345L               /*"    ELSE"*/,
        04457L                   /*"      DO"*/,
        0457064456256L           /*"  EXTERN"*/,
        04262454153L             /*"   BREAK"*/,
        04357566451566545L       /*"CONTINUE"*/,
        043416345L               /*"    CASE"*/,
        044454641655464L         /*" DEFAULT"*/,
        06556515756L             /*"   UNION"*/};

int64_t helperNames[100] = { 0L,
        06017210000000000L      /*"P/1     "*/,
        06017220000000000L      /*"P/2     "*/,
        06017230000000000L      /*"P/3     "*/,
        06017240000000000L      /*"P/4     "*/,
        06017250000000000L      /*"P/5     "*/,
        06017260000000000L      /*"P/6     "*/,
        06017434100000000L      /*"P/CA    "*/,
        0L                      /*"P/EO obs"*/,
        0L                      /*"P/SS obs"*/,
/*10*/  0L                      /*"P/EL obs"*/,
        04317554400000000L      /*"C/MD    "*/,
        06017555100000000L      /*"P/MI    "*/,
        06017604100000000L      /*"P/PA    "*/,
        06017655600000000L      /*"P/UN    "*/,
        06017436000000000L      /*"P/CP    "*/,
        06017414200000000L      /*"P/AB    "*/,
        04317445100000000L      /*"C/DI    "*/,
        04317624300000000L      /*"C/RC    "*/,
        06017454100000000L      /*"P/EA    "*/,
/*20*/  06017564100000000L      /*"P/NA    "*/,
        06017424100000000L      /*"P/BA    "*/,
        06017515100000000L      /*"P/II   u"*/,
        06017626200000000L      /*"P/RR    "*/,
        06017625100000000L      /*"P/RI    "*/,
        06017214400000000L      /*"P/1D    "*/,
        06017474400000000L      /*"P/GD    "*/,
        04317450000000000L      /*"C/E     "*/,
        04317454600000000L      /*"C/EF    "*/,
        06017604600000000L      /*"P/PF    "*/,
/*30*/  06017474600000000L      /*"P/GF    "*/,
        06017644600000000L      /*"P/TF    "*/,
        06017624600000000L      /*"P/RF    "*/,
        06017566700000000L      /*"P/NW    "*/,
        06017446300000000L      /*"P/DS    "*/,
        06017506400000000L      /*"P/HT    "*/,
        04317675100000000L      /*"C/WI    "*/,
        06017676200000000L      /*"P/WR    "*/,
        06017674300000000L      /*"P/WC    "*/,
        06017412600000000L      /*"P/A6    "*/,
/*40*/  06017412700000000L      /*"P/A7    "*/,
        06017677000000000L      /*"P/WX    "*/,
        06017675700000000L      /*"P/WO    "*/,
        06017436700000000L      /*"P/CW    "*/,
        06017264100000000L      /*"P/6A    "*/,
        06017274100000000L      /*"P/7A    "*/,
        06017675400000000L      /*"P/WL    "*/,
        06017624451000000L      /*"P/RDI   "*/,
        06017624462000000L      /*"P/RDR   "*/,
        06017624443000000L      /*"P/RDC   "*/,
/*50*/  06017624126000000L      /*"P/RA6   "*/,
        06017624127000000L      /*"P/RA7   "*/,
        06017627000000000L      /*"P/RX   u"*/,
        06017625400000000L      /*"P/RL    "*/,
        06017675754560000L      /*"P/WOLN  "*/,
        06017625154560000L      /*"P/RILN  "*/,
        06017626200000000L      /*"P/RR    "*/,
        06017434500000000L      /*"P/CE    "*/,
        06017646200000000L      /*"P/TR    "*/,
        06017546600000000L      /*"P/LV    "*/,
/*60*/  04657604556000000L      /*"FOPEN   "*/,
        04643545763450000L      /*"FCLOSE  "*/,
        06017426000000000L      /*"P/BP    "*/,
        06017422600000000L      /*"P/B6    "*/,
        06017604200000000L      /*"P/PB    "*/,
        06017422700000000L      /*"P/B7    "*/,
        06017515600000000L      /*"P/IN    "*/,
        06017516300000000L      /*"P/IS    "*/,
        06017444100000000L      /*"P/DA    "*/,
        06017435700000000L      /*"P/CO    "*/,
/*70*/  06017516400000000L      /*"P/IT    "*/,
        06017435300000000L      /*"P/CK    "*/,
        06017534300000000L      /*"P/KC    "*/,
        06017545647604162L      /*"P/LNGPAR"*/,
        06017544441620000L      /*"P/LDAR  "*/,
        06017544441625156L      /*"P/LDARIN"*/,
        06017202043000000L      /*"P/00C   "*/,
        06017636441620000L      /*"P/STAR  "*/,
        06017605544634564L      /*"P/PMDSET"*/,
        06017435100000000L      /*"P/CI    "*/,
/*80*/  06041514200000000L      /*"PAIB    "*/,
        06017674100000000L      /*"P/WA    "*/,
        0L                      /*"was SQRT"*/,
        0L                      /*"was SIN "*/,
        0L                      /*"was COS "*/,
        0L                      /*"was ATAN"*/,
        0L                      /*"was ASIN"*/,
        0L                      /*"was LN  "*/,
        0L                      /*"was EXP "*/,
        06017456100000000L      /*"P/EQ    "*/,
/*90*/  06017624100000000L      /*"P/RA    "*/,
        06017474500000000L      /*"P/GE    "*/,
        06017554600000000L      /*"P/MF    "*/,
        06017465500000000L      /*"P/FM    "*/,
        06017565600000000L      /*"P/NN    "*/,
        06017634300000000L      /*"P/SC    "*/,
        06017444400000000L      /*"P/DD    "*/,
        06017624500000000L      /*"P/RE    "*/,
        04317635054000000L      /*"C/SHL   "*/,
        04317635062000000L      /*"C/SHR   "*/};

// Copied verbatim from base.pas 8796 (systemProcNames: array [0..22]).  The
// registration loop (regSysProc) only reads indices 0..22; the trailing slots
// zero-fill.  This is the P2C set (CTOR/RETURN/BESM/FREE...), NOT the upstream
// pascompl set (READ/EXIT/DEBUG/NEW/DISPOSE...) -- index 14 in particular is
// RETURN, not EXIT, so `return` is recognised as standproc #14.
int64_t systemProcNames[30] = {
/*0*/   0606564L                /*"     PUT"*/,
        0474564L                /*"     GET"*/,
        062456762516445L        /*" REWRITE"*/,
        06245634564L            /*"   RESET"*/,
        0L                      /*" was NEW"*/,
        044516360576345L        /*"    FREE"*/,
        050415464L              /*"    HALT"*/,
        0L                      /*"was STOP"*/,
        0L                      /*" was SETUP"*/,
        0L                      /*" was ROLLUP"*/,
/*10*/  06762516445L            /*"   WRITE"*/,
        067625164455456L        /*" WRITELN"*/,
        043645762L              /*"    CTOR"*/,
        0L                      /*"  READLN"*/,
        0624564656256L          /*"  RETURN"*/,
        0L                      /*"was LONGJMP"*/,
        042456355L              /*"    BESM"*/,
        0L                      /*"   MAPIA"*/,
        0L                      /*"   MAPAI"*/,
        0604353L                /*"     PCK"*/,
/*20*/  06556604353L            /*"   UNPCK"*/,
        060414353L              /*"    PACK"*/,
        0655660414353L          /*"  UNPACK"*/};
