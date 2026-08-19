/*
 * Host-native C++ mirror of work.p2c, the "Pascal to C metamorphosis" (P2C)
 * compiler.  work.p2c is authoritative for semantics, spelling and structure;
 * this file follows it.  Built with `make base`, it compiles work.p2c into
 * the emulator module work.o/work.bin and so roots the bootstrap.
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
#include <functional>

FILE * pasinput = stdin;
int PASINPUT;
const char *outFileName = "output.obj";

const char * boilerplate = " PASCAL METAMORPH HELPER (2025) ";

const int MAXLIT = 500;
const int SYMTAB_LIMIT = 075500;
const int SYMTAB_MAX = 80;
const int OBJBUF_SIZE = 8192;    // initially 1024
const int caseTabMin = 9;        // clause count from which a switch indexes

const int64_t
    fnABS = 0, fnSIZEOF = 1, fnOFFSETOF = 2, fnMALLOC = 3,
    fnCARD = 4, fnMINEL = 5,
    fnREF = 6, fnABSI = 7;

const int64_t
    S3 = 0,
    S4 = 1,
    S5 = 2;

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
    mcMALLOC = 12,
    mcMINEL = 15,
    mcPOP2ADDR = 19,
    mcCOND2INT = 20,
    mcPCKSTORE = 22;

const int64_t
    P_RR = 32,
    C_TR = 33,
    P_LDAR = 46;

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
//  REAL05 =    04000023,
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
    lookField = 3,
    BACKSLASH = 035;            // char '\035' in the internal 6-bit code

enum Symbol {
/*0B*/  IDENT,      INTCONST,   REALCONST,  CHARCONST,
        STRINGSY,   LPAREN,     LBRACK,     EXPROP,
/*10B*/ RPAREN,     RBRACK,     COMMA,      SEMICOLON,
        PERIOD,     ARROW,      COLON,      BECOMES,
/*20B*/ BEGINSY,    ENDSY,      TYPESY,     CONSTSY,
        TYPEDEFSY,  ENUMSY,     PACKEDSY,   STRUCTSY,
/*30B*/ IFSY,       SWITCHSY,   WHILESY,    FORSY,
        GOTOSY,     ELSESY,     DOSY,       EXTERNSY,
/*40B*/ BREAKSY,    CONTSY,     CASESY,     DEFAULTSY,
        UNIONSY,    RETURNSY,   NOSY
};

enum IdClass {
        TYPEID,     ENUMID,     ROUTINEID,  VARID,
        FORMALID,   FIELDID,    REGID
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
    RMWASSIGN,  GETENUM,    GETFIELD,   DEREF,
    STKLVAL,    INDCALL,    PROCADDR,   ALNUM,
    TOREAL,     TOINT,      NOTOP,      INEGOP,     RNEGOP,
    BITNEGOP,   STANDPROC,  NOOP
};

enum OpGen {
    gen0,  STORE, LOAD,  FORMOP,  SETREG,
    SETREG9,  STOREAT9,  DOIT,  SETREG12,  DFLTWDTH,
    FRACWIDTH, SETREG11, PUSHSET11,
    BRANCH, PCKUNPCK
};

// Flags for ops that can potentially be optimized if one operand is a constant
enum OpFlg {
    opfCOMM, opfHELP, opfAND, opfOR, opfDIV, opfMOD, opfSHIFT,
    opfMULMSK, opfASSN
};

enum Kind {
    kindVoid, kindReal, kindScalar, kindPtr,
    kindArray, kindStruct,
    kindRoutine
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
int64_t heapBase = 100, heapLimit = 074000;
int64_t avail = 100, maxHeap;

void * besm6_alloc_words(size_t words)
{
    if (words > size_t(heapLimit - avail)) {
        fprintf(stderr, "Out of memory: avail = %ld, wants %lu words\n",
                avail, words);
        throw std::bad_alloc();
    }
    int64_t * result = heap + avail;
    avail += words;
    return result;
}

void * besm6_alloc(size_t bytes)
{
    return besm6_alloc_words((bytes + sizeof(int64_t) - 1) /
                             sizeof(int64_t));
}

template<class T> T * besm6_alloc_record(size_t bytes)
{
    assert(bytes % sizeof(int64_t) == 0);
    void * result = besm6_alloc(bytes);
    memset(result, 0, bytes);
    return reinterpret_cast<T*>(result);
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
    if (p < heap + heapBase || p > heap + avail) {
        fprintf(stderr, "Cannot rollup from %p to %p\n", (void*)(heap + avail), p);
        exit(1);
    }
    if (maxHeap < avail)
        maxHeap = avail;
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
// The `pckrep` packed record, s6 right-to-left packing:
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
    // The whole-word view gives exact initialization/copy/comparison, while
    // the LSB rep field matches the address bits used by BESM-6 indirection.
    union {
        int64_t word;      // whole-word view; only the low 48 bits matter
        PckRep p;
    };
    bool operator==(const TPtr & x) const { return word == x.word; }
    bool operator!=(const TPtr & x) const { return word != x.word; }
    // For `typ == NULL` sites: tests the record-pointer part.
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
    // Mimics the BESM-6 comparison exactly: the compiled sequence is
    //   XTA a; AEX <ones>; ARX b; UZA
    // i.e. ~a + b with the end-around carry, "less" iff the result's top bit
    // is clear.  Written the other way round (a + ~b, top bit set) it would
    // call every value less than itself, since a + ~a is the all-ones word.
    // The ordering is not transitive, so a binary-search table can admit
    // repeated literals -- and once the table fills at 500 it stops taking
    // new ones at all, so an early disagreement with the machine snowballs.
    bool operator<(const Alfa & x) const {
        uint64_t tmp = (val ^ 0xFFFFFFFFFFFFL) + x.val;
        tmp = (tmp + (tmp >> 48)) & 0xFFFFFFFFFFFFL;
        return (tmp >> 47) == 0;
    }

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

/* Interned derived-type descriptors (arrays, int:N scalars): a chain of
 * nodes so identical types share one heap record.  Nodes above a rolled-up
 * arena mark are dropped by myrollup. */
struct InternRec : public BESM6Obj {
    TPtr ityp;
    InternRec * inext;
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
// (the `types` variant record, under the compact-pointer layout).
struct Types : public BESM6Obj {
    void * operator new(size_t) = delete;

    union {
        struct {                       // kindArray
            TPtr base;
            int64_t pck;               // boolean
            int64_t perword, pcksize, aleft, aright;
            int64_t szArray;
        };
        struct {                       // kindScalar
            IdentRecPtr enums;
            int64_t numen, start;
            int64_t szScalar;
        };
        struct {                       // kindPtr
            TPtr sbase;
            int64_t szPtr;
        };
        struct {                       // kindStruct
            TPtr variants;
            IdentRecPtr fields;
            int64_t flag, lsbord;      // booleans
            int64_t szStruct;
        };
        struct {                       // unused (work.p2c has no such kind)
            TPtr first, next, alt;
            int64_t szCases;
        };
        struct {                       // kindRoutine
            TPtr rresult;
            SigPtr rparams;
            int64_t rargc;
            int64_t rflags;
            int64_t szRtype;
        };
    };

    std::string p() const { return "type"; } // details live in TPtr
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
    // A TYPESY keyword (int, char, float, void) carries its type;
    // every other keyword carries the character class of its first symbol.
    union {
        Operator op;
        TPtr typ;
    };
    KeyWord() : next(nullptr), w(), sym(NOSY) { op = NOOP; }
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
    void * operator new(size_t) = delete;

    // Packed into one word, mirroring work.p2c's idpck union: `next` is a
    // 15-bit arena word-index (like TPtr's rep), so it shares the word with
    // offset:24 and cl:3.  offset/cl are plain bitfields (access sites keep
    // `pck.offset`/`pck.cl`); next() converts the index back to a pointer.
    union {
        int64_t pckword;
        struct {
            uint64_t nidx   : 15;   // arena word-index (074000 = nil)
            int64_t  offset : 24;
            uint64_t cl     : 3;    // IdClass
        };
    } pck;
    int64_t id;
    TPtr typ;
    // TYPEID, VARID classes end here
    union {
        struct {                // ENUMID, FORMALID
            IdentRecPtr list_;
            int64_t value_;
            int64_t szIdent;
        };
        struct {                // FIELDID
            int64_t maybeUnused_;
            TPtr uptype_;
            bool pckfield_;
            int64_t shift_, width_;
            int64_t szField;
        };
        struct {                // ROUTINEID
            int64_t low_;
            int64_t high_;
            IdentRecPtr argList_, preDefLink_;
            int64_t level_, pos_;
            int64_t flags_;
            int64_t szRoutine;
        };
        struct {                // predefined system routine
            int64_t sysnum_;
            int64_t szSys;
        };
    };
    // Hash-chain link, stored as a compact arena index in pck (see above).
    // nidx==0 is the memset-fresh state (besm6_alloc zero-fills); treat it as
    // nil like the explicit 074000, so a just-allocated record reads NULL.
    IdentRecPtr next() const {
        // heap + index directly (not ptr(), whose bounds check rejects the
        // dangling-but-unused links a native pointer tolerated across rollup).
        return (pck.nidx == 0 || pck.nidx == 074000) ? NULL
             : reinterpret_cast<IdentRecPtr>(heap + pck.nidx);
    }
    IdentRecPtr & list() {
        assert(pck.cl != TYPEID);
        return list_;
    }
    int64_t & value() {
        assert(pck.cl != TYPEID);
        return value_;
    }
    int64_t & low() {
        assert(pck.cl == ROUTINEID);
        return low_;
    }
    Word & high() {
        return *reinterpret_cast<Word*>(&high_);
    }
    int64_t & procno() {
        assert(pck.cl == ROUTINEID);
        return low_;
    }
    TPtr & uptype() {
        assert(pck.cl == FIELDID);
        return uptype_;
    }
    bool & pckfield() {
        assert(pck.cl == FIELDID);
        return pckfield_;
    }
    int64_t & shift() {
        assert(pck.cl == FIELDID);
        return shift_;
    }
    int64_t & width() {
        assert(pck.cl == FIELDID);
        return width_;
    }
    IdentRecPtr & argList() {
        assert(pck.cl == ROUTINEID);
        return argList_;
    }
    IdentRecPtr & preDefLink() {
        assert(pck.cl == ROUTINEID);
        return preDefLink_;
    }
    int64_t & level() {
        assert(pck.cl == ROUTINEID);
        return level_;
    }
    int64_t & pos() {
        assert(pck.cl == ROUTINEID);
        return pos_;
    }
    int64_t & flags() {
        assert(pck.cl == ROUTINEID);
        return flags_;
    }
    
    std::string p(bool verbose = false) const {
        std::string ret;
        char * strp;
        switch (pck.cl) {
        default: ret = toAscii(id);
            return ret.substr(ret.find_last_of(' ')+1, std::string::npos);
        case ROUTINEID:
            ret = toAscii(id);
            if (verbose) {
                if (0 <= asprintf(&strp, "(routine) procno: %ld value: %ld argl: %ld predef: %ld level: %ld pos: %ld flags: %lx",
                                  low_, value_, ord(argList_), ord(preDefLink_), level_, pos_, flags_)) {
                    ret += strp;
                    free(strp);
                } else perror("asprintf");
            }
        }
        return ret;
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

numberFormat numFormat;
SetOfSYs   bigSkipSet, statEndSys, blockBegSys, statBegSys,
           skipToSet, lvalOpSet;

bool   bool48z, forValue;

int64_t jumpType, jumpTarget;

Operator charClass;
Symbol   SY, prevSY;

int64_t savedObjIdx,
        FcstCnt,
        symTabPos,
        entryPtCnt,
        fileBufSize;

// Pointers pinned in index registers by the register declarations of the
// blocks now open, innermost first; genEntry reloads a spilled one after a
// call that clobbers its register.
ExprPtr pinList;

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
TPtr symType;                   // the type denoted by the current TYPESY
Kind curVarKind;
ExtFileRec * curExternFile;
char commentModeCH;
unsigned char CH, prevCH;
Word prevInsn;

int64_t lineNesting,
        FcstTotal,
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
    enableStdInput,
    rangeMismatch,
    fixMult,
    bool110z,
    allowCompat,
    sortFcst,
    checkFortran;

int verbose;

IdentRecPtr outputFile,
    inputFile,
    programObj,
    hashTravPtr,
    uProcPtr;

ExtFileRec * externFileList;

TPtr typ121z;
TPtr voidType, voidPtr;
// Expression-operator tables, filled in the initialize section
// (intOpMap[MUL] = IMULOP,IDIVOP...; opPrec = precNone:48 ...).
Operator intOpMap[64];
int64_t opPrec[64];
TPtr BooleanType;
TPtr IntegerType;
TPtr RealType;
TPtr CharType;
TPtr charPtrType, flatMemType;
IdentRecPtr flatMemVar;

TPtr arg1Type, arg2Type;

NumLabel numLabs[21];    // array [1..20] of numLabel
int64_t numLabTop;
Word curToken, curVal;
const int64_t extSymMask = 043000000L;
const int64_t halfWord = 077777777L;
const int64_t leftAddr = 077777L << 24;
const int64_t litOutput = 01257656460656412L; /* "*OUTPUT*" */
const int64_t litInput = 012515660656412L;    /* " *INPUT*" */

int64_t leftInsn;
int64_t curIdent;
int64_t toAlloc, usedRegs, liveRegs, freeRegs, auxRegs;
Word optSflags;
int64_t litOct, litFortran, litAssembler, litLsb, litMain, litRegister;
ExprPtr uVarPtr, curExpr;
InsnList *  insnList;
InternRec * internHead;
ExtFileRec * fileForOutput, * fileForInput;
int64_t symTabCnt;

int64_t symTabArray[SYMTAB_MAX+1]; // array [1..80] of Word;
int64_t symTabIndex[SYMTAB_MAX+1];
Entries entryPtTable;
four frameRestore[7]; // array [3..6] of four;
int64_t indexreg[16];
int64_t opToInsn[48];
int64_t opToMode[48];
OpFlg opFlags[48]; // array [MUL..op44] of OpFlg;
int64_t funcInsn[8];
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
int64_t helperMap[58];
extern int64_t helperNames[58]; // array [1..57] of int64_t;

int64_t symTab[SYMTAB_LIMIT + 1]; // array [74000B..75500B] of int64_t;
extern int64_t systemProcNames[9];
extern int64_t resWordNameBase[19];
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
        "GETENUM","GETFIELD","DEREF","STKLVAL","INDCALL","PROCADDR","ALNUM",
        "TOREAL","TOINT","NOTOP","INEGOP","RNEGOP","BITNEGOP","STANDPROC",
        "NOOP"
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
                 id1 ? id1->id : -1, id1 ? id1->pck.offset : -1,
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

// The source compiler's defExtern is lexically visible to both mainProgram
// and programme.  The C++ mirror represents mainProgram as initScalars, so
// provide the same shared path for prefix external variable declarations.
int64_t leftAlign(int64_t val);
void addToHashTab(IdentRecPtr arg);
void error(int64_t errNo);
int64_t allocExtSymbol(int64_t newSym);
// Defined below, past the declarator machinery it grew out of; a string
// constant needs it here to type itself.
TPtr makeArrayType(int64_t, int64_t, TPtr, bool);

void defExtern()
{
    int64_t line = 0;
    Word aligned;
    IdentRecPtr idRec;

    aligned.ii = leftAlign(curIdent);
    if (curIdent == litInput || curIdent == litOutput) {
        idRec = besm6_alloc_record<IdentRec>(offsetof(IdentRec, szIdent));
        idRec->id = curIdent;
        idRec->pck.offset = 0;
        /* An FCB is 30 opaque words.  Its size is all the compiler needs
           of the type -- that is how put/get/reset/rewrite and write's
           leading file argument recognize a file -- and no operator
           applies to it, so it is a void of 30 words. */
        idRec->typ.word = 0;
        idRec->typ.setRep(NULL);        // ord(NULL) == 074000, as in work.p2c
        idRec->typ.p.psize = 30;
        idRec->pck.cl = VARID;
        idRec->list() = NULL;
        curVal = aligned;
        idRec->value() = allocExtSymbol(047000000 | 30);
        addToHashTab(idRec);
        if (curIdent == litInput)
            inputFile = idRec;
        else
            outputFile = idRec;
        line = lineCnt;
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
    curExternFile->line = line;
    curExternFile->offset = aligned.ii;
    if (line != 0) {
        if (curIdent == litOutput)
            fileForOutput = curExternFile;
        else
            fileForInput = curExternFile;
    }
    externFileList = curExternFile;
    curExternFile->location = 512;
}

struct programme {
    programme(int64_t & l2arg1z, IdentRecPtr l2idr2z_, bool bodyBlock_ = false);

    IdentRecPtr procName;
    IdentRecPtr preDefHead, typelist, scopeBound, l2var4z, curIdRec, workidr;
    bool isPredefined, l2bool8z, inTypeDef, externDecl;
    bool done, retSeen, hadParens, typedefPending;
    int64_t fileExit;
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

/* The host mirror of the runtime's PASMITXT table, which lives in
   build-pascom.dub -- see that file for the numbering.  Every error number the
   compiler can raise has a case here; anything left over falls through to
   "Dunno", which now means a genuinely unknown number rather than a gap in the
   table.  Errors 200 and up never reach this function: printErrMsg reports
   them as internal errors.  */
const char * pasmitxt(int64_t errNo)
{
    switch (errNo) {
    case errBooleanNeeded: return "Boolean required";
    case 1: return "No comma nor semicolon";
    case errIdentAlreadyDefined: return "Identifier already defined";
    case errNoIdent: return "Missing identifier";
    case errNotAType: return "Not a type";
    case 5: return "Simple type required";
    case errNoConstant: return "Missing constant";
    case errConstOfOtherTypeNeeded: return "Constant of other type required";
    case 8: return "Missing index type";
    case errTypeMustNotBeFile: return "Type must not be a file type";
    case errNotDefined: return "Unknown identifier";
    case errBadSymbol: return "Bad symbol";
    case 13: return "Variable is not a pointer";
    case 14: return "Expression is not of integer type";
    case 16: return "Label not defined in block";
    case 17: return "Label already defined in line";
    case 18: return "Label not defined";
    case errNeedOtherTypesOfOperands: return "Other types of operands required";
    case errWrongVarTypeBefore: return "Illegal type of variable preceding";
    case 23: return "Type ID instead of a variable";
    case 24: return "Expression in set constructor of incompatible type";
    case 25: return "Expression is not a discriminant of scalar type";
    case 27: return "Variable needed before expression";
    case errUsingVarAfterIndexingPackedArray: return "Using a variable after indexing packed array";
    case 29: return "Index out of bounds";
    case 33: return "Illegal types for assignment";
    case 34: return "Type does not fit the file element type";
    case 35: return "2nd write spec is only for real";
    case 36: return "Too few parameters";
    case 37: return "Missing INPUT file in program header";
    case errTooManyArguments: return "Too many parameters";
    case 39: return "Argument kind mismatch in call";
    case 40: return "Argument type mismatch in call";
    case errNoCommaOrParenOrTooFewArgs: return "No comma or parenthesis, or too few args";
    case 42: return "Missing parameter list";
    case errNumberTooLarge: return "Number too large";
    case 44: return "Incorrect usage of a standard procedure or a function";
    case 45: return "Missing open parenthesis for a standard procedure";
    case errVarTooComplex: return "Too complex variable";
    case 49: return "Too many instructions in a block";
    case 50: return "Symbol table overflow";
    case 51: return "Long symbol overflow";
    case errEOFEncountered: return "EOF encountered";
    case 54: return "Error in pseudo-comment";
    case 55: return "More than 16 digits in a number";
    case 56: return "No mantissa after the dot";
    case 57: return "No exponent after E";
    case 58: return "Exponent above 18";
    case 59: return "EOLN true within a line";
    case errFirstDigitInCharLiteralGreaterThan3: return "First digit of a char literal is above 3";
    case 61: return "Empty string";
    case 62: return "Integer needed";
    case 63: return "Bad base type for set";
    case 64: return "Error in range type definition";
    case 68: return "Using a procedure in an expression";
    case 69: return "Minus applies to neither real nor integer";
    case 71: return "Register pointer is not a pointer to a struct";
    case 73: return "Two equal case labels";
    case 74: return "Different types of case labels and expression";
    case 77: return "Missing OUTPUT file in program header";
    case 78: return "Predefined identifier used as a pointer";
    case 79: return "Unknown identifier in type definition";
    case 80: return "External file not defined";
    case 81: return "Procedure nesting is too deep";
    case 82: return "Previous declaration was not a forward declaration";
    case 83: return "Redefinition of a predefined identifier";
    case 84: return "Error in declarations";
    case 85: return "Routines left undefined";
    case 86: return "Required token not found: ";
    case 87: return "Too many procedure entry points";
    /* Token names for "Required token not found" errors.  requiredSymErr(sym)
       reports error(sym + 88), so these labels are spelled `SYM + 88` and track
       the Symbol enum by construction -- they were once hard-coded to the
       pre-compaction values and had silently drifted onto their neighbours.
       The list runs IDENT..RETURNSY, i.e. the whole enum bar NOSY, because
       checkSymAndRead takes any symbol.  Keep the names identical to the
       runtime's, so the two compilers report a missing token the same way. */
    case IDENT + 88:     return "IDENTIFIER";
    case INTCONST + 88:  return "INT CONST";
    case REALCONST + 88: return "REAL CONST";
    case CHARCONST + 88: return "CHAR CONST";
    case STRINGSY + 88:  return "STRING CONST";
    case LPAREN + 88:    return "LEFT PAREN";
    case LBRACK + 88:    return "LEFT BRACK";
    case EXPROP + 88:    return "EXPR OP";
    case RPAREN + 88:    return "RIGHT PAREN";
    case RBRACK + 88:    return "RIGHT BRACK";
    case COMMA + 88:     return "COMMA";
    case SEMICOLON + 88: return "SEMICOLON";
    case PERIOD + 88:    return "PERIOD";
    case ARROW + 88:     return "ARROW";
    case COLON + 88:     return "COLON";
    case BECOMES + 88:   return "ASSIGN";
    case BEGINSY + 88:   return "BEGIN";
    case ENDSY + 88:     return "END";
    case TYPESY + 88:    return "TYPE NAME";
    case CONSTSY + 88:   return "CONST";
    case TYPEDEFSY + 88: return "TYPEDEF";
    case ENUMSY + 88:    return "ENUM";
    case PACKEDSY + 88:  return "PACKED";
    case STRUCTSY + 88:  return "STRUCT";
    case IFSY + 88:      return "IF";
    case SWITCHSY + 88:  return "SWITCH";
    case WHILESY + 88:   return "WHILE";
    case FORSY + 88:     return "FOR";
    case GOTOSY + 88:    return "GOTO";
    case ELSESY + 88:    return "ELSE";
    case DOSY + 88:      return "DO";
    case EXTERNSY + 88:  return "EXTERN";
    case BREAKSY + 88:   return "BREAK";
    case CONTSY + 88:    return "CONTINUE";
    case CASESY + 88:    return "CASE";
    case DEFAULTSY + 88: return "DEFAULT";
    case UNIONSY + 88:   return "UNION";
    case RETURNSY + 88:  return "RETURN";
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
    auto str = toAscii(val);
    const char *s = str.c_str();
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

// File scope (not nested in initTables): the predefined type names are
// registered as TYPESY keywords by initScalars, once their types exist.
void regResWord(int64_t l4arg1z) {
    KeyWord * kw;
    Word l4var2z;
    curVal.ii = l4arg1z;
    curVal.ii = (curVal.ii % 65535) % 128;
    l4var2z.ii = l4arg1z;
    kw = new KeyWord;
    kw->w = l4var2z;
    kw->sym = SY;
    if (SY == TYPESY)
        kw->typ = symType;
    else
        kw->op = charClass;
    kw->next = KeyWordHashTabBase[curVal.ii];
    KeyWordHashTabBase[curVal.ii] = kw;
} /* regResWord */

/* An ordinary pointer type encoded wholly in the tptr word: rep, psize
 * and bits carry the ultimate non-pointer base, pad packs depth*8 plus
 * the base kind.  Base kind 0 is never encoded (pointer-to-void is the
 * voidPtr singleton), so a heap-allocated pointer descriptor, whose pad
 * is 0, is not mistaken for a compact pointer. */
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
 * heap-allocated descriptors read the record. */
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

/* A pointer to a routine, i.e. a function pointer.  getPtrType always
   compact-encodes one (a routine type leaves pad 0), so the test is on the
   encoding itself: calling ptrBase here would read the pointee descriptor,
   which a pointer with no descriptor of its own -- voidPtr -- does not
   have. */
bool isRoutinePtr(TPtr arg)
{
    return isCompactP(arg) and arg.p.pad == 010 + kindRoutine;
} /* isRoutinePtr */

/* The step for pointer arithmetic: the pointee's size in words.  A step of
   0 bars the arithmetic; it covers a void pointer, which is what '&x' and
   NULL are here, and a pointer to a routine, whose value is an entry
   address. */
int64_t eltStep(TPtr t)
{
    if (t.p.pk != kindPtr or t == voidPtr or isRoutinePtr(t))
        return 0;
    return typeSize(ptrBase(t));
} /* eltStep */

void myrollup(void * p)
{
/* Forget interned types allocated above the mark being rolled up: every arena
   rollback comes through here, and a statement's own rollback reclaims the
   types its string constants built. */
    while (internHead != NULL and ord(internHead) >= ord(p))
        internHead = internHead->inext;
    rollup(p);
} /* myrollup */

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
    /* Fallback: heap-allocated pointer descriptor, for bases the compact
       form cannot carry, i.e. pointer depth overflowing the pad field. */
    t.setRep(besm6_alloc_record<Types>(offsetof(Types, szPtr)));
    t.rep()->base = toType;
    t.p.psize = 1;
    t.p.bits = 15;
    t.p.pk = kindPtr;
    return t;
}

ExprPtr bldIncDec(ExprPtr lval, bool isInc, bool isPost)
{
    ExprPtr one, rmw;
    Operator op1, op2;
    TPtr t;
    int64_t step;
    /* A pointer steps by its pointee, and keeps its type; anything else is
       an integer step of one. */
    step = eltStep(lval->vt.typ);
    if (step == 0) {
        t = IntegerType;
        step = 1;
    } else
        t = lval->vt.typ;
    if (isInc) { op1 = INTPLUS; op2 = INTMINUS; }
    else       { op2 = INTPLUS; op1 = INTMINUS; }
    one = mkIntLit(step);
    rmw = mkExpr(RMWASSIGN, t, lval,
                 mkExpr(op1, t, one, NULL));
    if (not isPost) {
        return rmw;
    }
    return mkExpr(op2, t, rmw, one);
} /* bldIncDec */

void addToHashTab(IdentRecPtr arg)
{
    int bucket = (arg->id % 65535) % 128;
    arg->pck.nidx = ord(symHash[bucket]);
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

bool fcstLess(const Word &left, const Word &right)
{
    if (sortFcst)
        return left.a.val < right.a.val;
    return left.a < right.a;
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
            if (curVal.a.val == constVals[mid].a.val) {
              return constNums[mid];
            }
            if (fcstLess(curVal, constVals[mid]))
                high = mid - 1;
            else
                low = mid + 1;
        } while (low <= high);
        ret = FcstCnt;
        if (FcstTotal != MAXLIT) {
            if (fcstLess(curVal, constVals[mid]))
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

TPtr mkIntScl(int64_t bitWid)
{
    TPtr res{};
    InternRec * icand;
    if (bitWid < 1 or 40 < bitWid) {
        error(errNumberTooLarge);
        return IntegerType;
    }
    icand = internHead;
    while (icand != NULL) {
        res = icand->ityp;
        if (res.p.pk == kindScalar and res.p.bits == (uint64_t)bitWid)
            return res;
        icand = icand->inext;
    }
    res.setRep(besm6_alloc_record<Types>(offsetof(Types, szScalar)));
    res.rep()->start = -1;
    res.rep()->enums = NULL;
    res.rep()->numen = 1L << bitWid;
    res.p.psize = 1;
    res.p.bits = bitWid;
    res.p.pk = kindScalar;
    icand = new InternRec;
    icand->ityp = res;
    icand->inext = internHead;
    internHead = icand;
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
        uni2koi8[L'\\'] = 035;   // BACKSLASH is '\035':
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

static int ugetc(FILE * fin)
{
    int c = utf8_getc(fin);
    // Keep the host EOF sentinel distinct in the input lookahead.  nextCH()
    // maps it to the zero-byte sentinel used by work.p2c only when assigning
    // CH; passing EOF through unicode_to_koi8 would turn it into 0377.
    if (c < 0)
        return EOF;
    return unicode_to_koi8(c);
}

void readToPos80()
{
    // work.p2c readToPos80: pad the last line to column 81 without ending it
    // (an endOfLine() here would list a phantom line past the end of the
    // source and count it).  Map host EOF (-1) to the zero-byte sentinel
    // stored by the work compiler.
    while (linePos < 81) {
        linePos = linePos + 1;
        lineBufBase[linePos] = PASINPUT == EOF ? 0 : PASINPUT;
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
    // The end of pasinput has already been reported as CH = 0, so this call
    // is a read past the end.  One is normal (the lexer advances once past
    // the last token), but the parser may also be skipping for a recovery
    // symbol that will never arrive -- skipSp() returns false on the
    // sentinel, so endOfLine(), and with it the second-EOF guard, is never
    // reached again.  Past a line's worth of skipping there is nothing left
    // to recover to; without this the compile never ended, and the runaway
    // linePos corrupted the globals behind lineBufBase, lineCnt among them,
    // which is what printed " IN -1 LINES".
    if (CH == 0) {
        static int64_t overreads;
        if (maxLineLen < ++overreads) {
            error(errEOFEncountered);
            throw 9999;
        }
        atEOL = true;
        // The column still advances -- requiredSymErr() reports only when
        // linePos has moved past the last error.
        if (linePos < maxLineLen)
            linePos = linePos + 1;
        return;
    }
    // Columns past lineBufBase are dropped, not stored: keep reading them
    // until the line ends, so that CH comes back at the line's end and
    // neither linePos nor the stores leave the buffer.  (lineBufBase is
    // 1-based here, so the last column it holds is maxLineLen itself.)
    do {
        atEOL = PASINPUT == '\n' || PASINPUT == EOF;
        CH = PASINPUT == EOF ? 0 : PASINPUT;
        PASINPUT = ugetc(pasinput);
        if (linePos < maxLineLen) {
            linePos = linePos + 1;
            lineBufBase[linePos] = CH;
            return;
        }
    } while (not atEOL);
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
                else if (4 <= curVal.ii && curVal.ii <= 7)
                    optSflags.ii = optSflags.ii | Bits(curVal.ii - 3);
            } break;
            case 'F': case 'f':
                readOptFlag(checkFortran);
                break;
            case 'I': case 'i':
                readOptFlag(enableStdInput);
                break;
            case 'L': case 'l':
                PASINFOR.listMode = readOptVal(3);
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
            if (CH == 0) {
                error(errEOFEncountered);
                throw 9999;
            }
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
    // work.p2c's KOI-7 literal mode distinguishes ASCII '^' and '|':
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
    // nextCH maps host EOF (-1) to the zero-word sentinel returned by the
    // work.p2c FGETC path.  Stop here so the parser unwinds without indexing
    // its character tables with the sentinel.
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
    unsigned char litQuote = 0;
{
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
                        if (SY == TYPESY)
                            symType = keyWordHashPtr->typ;
                        else
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
                        if (hashTravPtr->pck.offset == curFrameRegTemplate)
                        {
                            if (hashTravPtr->id != curIdent)
                                hashTravPtr = hashTravPtr->next();
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
                            hashTravPtr = hashTravPtr->next();
                        else {
                            if (hashTravPtr->pck.cl == TYPEID) {
                                SY = TYPESY;
                                symType = hashTravPtr->typ;
                            }
                            goto exitLexer;
                        }
                    }
                } break;
                case 2:
                    goto L2;
                case 3:
                    hashTravPtr = fieldHash[bucket];
                    while (hashTravPtr != NULL) {
                        if ((hashTravPtr->id == curIdent) and
                            (typ121z == hashTravPtr->uptype()))
                            goto exitLexer;
                        hashTravPtr = hashTravPtr->next();
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
                    /* The delimiter tells a character constant from a string:
                       single quotes give the packed word as an integer wherever
                       it fits one, double quotes give a packed character array
                       of the length written. */
                    unsigned char quote = CH;
                    litQuote = CH;
                    for (tokenIdx = 6; tokenIdx <= 130; ++tokenIdx) {
                        nextCH();
                        if (CH == quote) {
                            nextCH();
                            goto exitLoop;
                        }
                        if (atEOL) {
L2175:                      error(59); /* errEOLNInStringLiteral */
                            goto exitLoop;
                        } else if (CH == BACKSLASH) {
                            // '\NNN' octal (1..3 digits) or a named escape
                            // '\<letter>'.  work.p2c indexes
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
                                // internal 6-bit text
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
                    curVal.ii = 0x202020202020L; // curVal.a = '      '
                    SY = STRINGSY;
                    unpck(localBuf[tokenIdx], curVal.a);
                    pck(localBuf[6], curToken.a);
                    curVal = curToken;
                    if (6 >= strLen) {
                        if (litQuote == '\'')
                            SY = INTCONST;
                        goto exitLexer;
                    } else if (litQuote == '\'' and
                               charEncoding == 3 and strLen == 8) {
                        // an 8-char string in 6-bit-text mode
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
                            if (tokenIdx < tokenLen) // strict <
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
        /* two-char operator lexer.  curToken.a is
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
      exitLexer:
        prevSY = SY;
        commentModeCH = ' ';
        lookupMode = lookup2;
    }
} /* inSymbol */

void skipToEnd()
{
    while (CH != 0)
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

/* Construct a binary op-node, but if both operands are already constants
   (GETENUM), fold at construction and reuse the left operand's node in place
   as the result — no new allocation.  Construction is the *only* place
   constant folding happens, so constant expressions collapse to a literal for
   every downstream site that requires `op == GETENUM`.  A divide/modulo by a
   zero constant is left as an op-node so codegen keeps its behaviour. */
ExprPtr mkExprFold(Operator op, TPtr resTyp, ExprPtr e1, ExprPtr e2)
{
    if (e1->op == GETENUM and e2->op == GETENUM and
        not ((op == IDIVOP or op == IMODOP or op == RDIVOP) and
             e2->lit.ii == 0)) {
        Word &lhs = e1->lit;            /* fold in place into e1's value */
        const Word &rhs = e2->lit;
        switch (op) {
        case MUL:        lhs.r = lhs.r * rhs.r; break;
        case RDIVOP:     lhs.r = lhs.r / rhs.r; break;
        case ANDOP:      lhs.ii = lhs.ii and rhs.ii; break;
        case IDIVOP:
        case IMODOP:
        case IMULOP:
        case INTPLUS:
        case INTMINUS:   lhs = foldRawInt2(op, lhs, rhs); break;
        case PLUSOP:     lhs.r = lhs.r + rhs.r; break;
        case MINUSOP:    lhs.r = lhs.r - rhs.r; break;
        case OROP:       lhs.ii = lhs.ii or rhs.ii; break;
        case SETAND:     lhs.ii = lhs.ii & rhs.ii; break;
        case SETXOR:     lhs.ii = lhs.ii ^ rhs.ii; break;
        case SETOR:      lhs.ii = lhs.ii | rhs.ii; break;
        case SHLEFT:     lhs.ii = shl48(lhs.ii, rhs.ii); break;
        case SHRIGHT:    lhs.ii = (lhs.ii & BitRange(0, 47)) >> rhs.ii; break;
        default:
            return mkExpr(op, resTyp, e1, e2);   /* not a foldable op */
        }
        e1->vt.typ = resTyp;           /* e1 is already GETENUM; reuse it */
        return e1;
    }
    return mkExpr(op, resTyp, e1, e2);
}

/* An index or offset in pointee units, converted to words. */
ExprPtr scaleIdx(ExprPtr n, int64_t step)
{
    if (step == 1)
        return n;
    return mkExprFold(IMULOP, IntegerType, n, mkIntLit(step));
} /* scaleIdx */

/* Unary counterpart of mkExprFold.  Only ever called with a foldable unary
   operator (INEGOP/RNEGOP/BITNEGOP/boolean NOTOP/TOREAL/TOINT), so a GETENUM
   is always folded — mutated in place and reused. */
ExprPtr mkUnaryFold(Operator op, TPtr resTyp, ExprPtr e)
{
    if (e->op == GETENUM) {
        Word &arg = e->lit;
        switch (op) {
        case TOREAL:   arg.r = arg.ii; break;
        case TOINT:    arg = i64ToRawInt(int64_t(arg.r)); break;
        case NOTOP:    arg.b = not arg.b; break;
        case RNEGOP:   arg.r = -arg.r; break;
        case INEGOP:   arg = foldRawInt1(op, arg); break;
        case BITNEGOP: arg.ii = BitRange(0, 47) & ~arg.ii; break;
        default:       break;
        }
        e->vt.typ = resTyp;
        return e;
    }
    return mkExpr(op, resTyp, e, NULL);
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
                (hashTravPtr->pck.cl != ENUMID))
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
            /* A string constant is a packed char array of its own length. */
            litType = makeArrayType(0, strLen - 1, CharType, true);
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
                symHash[l3var2z]->next();
        } else {
            l3arg1z = l3arg2z->next();
        };
    } else {
        l3var3z = l3arg1z;
        while (l3var3z != l3arg2z) {
            l3var4z = l3var3z;
            if (l3var3z != NULL) {
                l3var3z = l3var3z->next();
            } else {
                return;
            }
        };
        l3var4z->pck.nidx = l3arg2z->pck.nidx;
    }
} /* hash */

// name defaults to curIdent (the original, Pascal '^Name' call sites,
// where the lexer is still sitting on Name); C-style forward-referenced
// typedef patching (see TYPEDEFSY) calls this after the declarator's own
// name has already been read and the lexer has moved on, so it passes
// the saved Declarator::name explicitly instead.
bool knownInType(IdentRecPtr & rec, int64_t name = curIdent)
{
    if (programme::super.back()->typelist != NULL) {
        rec = programme::super.back()->typelist;
        while (rec != NULL) {
            if (rec->id == name) {
                return true;
            }
            rec = rec->next();
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

bool typeCheck(TPtr type1, TPtr type2);
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool sameRoutineType(TPtr type1, TPtr type2)
{
    SigPtr p1, p2;
    // Only the flags that shape a call are part of the type: 21 fortran,
    // 24 all-by-reference, 26 assembler.  Bit 20 (extern) is linkage, and
    // including it would keep every declared function pointer type (flags 0)
    // from ever accepting an extern routine.
    if ((type1.rep()->rargc != type2.rep()->rargc) or
        ((Bits(21,24,26) & type1.rep()->rflags) !=
         (Bits(21,24,26) & type2.rep()->rflags))) {
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
            case kindRoutine:
                if (sameRoutineType(type1, type2))
                    goto L1;
                break;
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

// The routine type proper: a result type and a parameter signature.  Setting
// rep clears the descriptor metadata, so pad comes out 0 and getPtrType can
// compact-encode a pointer to this type.
TPtr mkRoutineTyp(TPtr result, SigPtr params, int64_t flags)
{
    TPtr resultTyp{};
    SigPtr p;

    resultTyp.setRep(besm6_alloc_record<Types>(offsetof(Types, szRtype)));
    resultTyp.rep()->rresult = result;
    resultTyp.rep()->rparams = params;
    resultTyp.rep()->rargc = 0;
    resultTyp.rep()->rflags = flags;
    resultTyp.p.psize = 1;
    resultTyp.p.bits = 15;
    resultTyp.p.pk = kindRoutine;
    for (p = params; p != NULL; p = p->next)
        resultTyp.rep()->rargc = resultTyp.rep()->rargc + 1;
    return resultTyp;
}

// The type of an already declared routine, from its parameter records.
TPtr makeRoutineType(IdentRecPtr routine)
{
    IdentRecPtr srcParam;
    SigPtr newParam, lastParam, head;

    head = NULL;
    lastParam = NULL;
    srcParam = routine->argList();
    if (srcParam != NULL) {
        while (srcParam != routine) {
            newParam = new SigRec;
            newParam->pclass = (IdClass)srcParam->pck.cl;
            newParam->ptyp = srcParam->typ;
            newParam->next = NULL;
            if (lastParam == NULL)
                head = newParam;
            else
                lastParam->next = newParam;
            lastParam = newParam;
            srcParam = srcParam->list();
        }
    }
    return mkRoutineTyp(routine->typ, head, routine->flags());
}

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
                    addInsnToBuf(getHelperProc(8));        /* P/MI */
                } break;
                case mcADDSTK2REG:
                    add2InsnsToBuf(KWTC+SP, KUTM+indexreg[curInsn.ii]);
                    break;
                case mcADDACC2REG:
                    add2InsnsToBuf(KATI+14, KMADDJ+I14 + curInsn.ii);
                    break;
                case 14:
                    add2InsnsToBuf(indexreg[curInsn.ii] + KVTM, KITA + curInsn.ii);
                    break;
                case mcMINEL: {
                    add2InsnsToBuf(KANX, KSUB+E1);   /* minel */
                } break;
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
                   helper #33, which returns the newly
                   allocated pointer in ACC.  Same calling convention as
                   the NEW system procedure. */
                    add2InsnsToBuf(KATI+14, getHelperProc(17));
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

// Extract the packed field described by insnList->shift/width out of the
// word that is already in the accumulator, and mark the list a plain word.
// Both of prepLoad's arms need this: an addressable operand loads its word
// first, while a value -- a function result, a ternary -- is in ACC from the
// start.  Leaving the second case out is what made `f(x).packedfield` yield
// the whole word.
void genSliceExtract()
{
    Kind l4var5z;
    bool isSimple;
    int64_t sh, wd, ends;

    l4var5z = (Kind)(insnList->typ.p.pk);
    isSimple = l4var5z < kindArray or
               (l4var5z == kindStruct and insnList->typ.rep()->lsbord);
    sh = insnList->shift;
    wd = insnList->width;
    ends = sh + wd;
    if (isSimple) {
// The commented out optimization is specific to the original BESM-6
// without a barrel shifter; it is not needed here.
//      if (30 < sh) {
//          addToInsnList(ASN64-48 + sh);
//          addToInsnList(KYTA);
//      } else {
            if (sh != 0)
                addToInsnList(ASN64 + sh);
//      }
        if (ends != 48) {
            curVal.ii = MASK48 >> (48 - wd);
            addToInsnList(KAAX+I8 + getFCSToffset());
        }
    } else {
        if (ends != 48)
            addToInsnList(ASN64-48 + ends);
        curVal.ii = shl48(MASK48, 48 - wd);
        addToInsnList(KAAX+I8 + getFCSToffset());
    }
    insnList->st = stWORD;
} /* genSliceExtract */

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
            } else if (l4st6z == stSLICE) {
                if ((l4int3z != l4int2z) or
                    (helper != 15) or
                    (l4int2z != 0))
                    addInsnAndOffset(l4int2z + InsnTemp[XTA],
                                     l4int3z);
                genSliceExtract();
            } else {
                l4var5z = (Kind)(valueType.p.pk);
                if (l4var5z < kindArray or
                    (l4var5z == kindStruct and valueType.rep()->lsbord)) {
                    isSimple = true;
                } else {
                    isSimple = false;
                }
                addToInsnList(getHelperProc(isSimple ? P_LDAR : P_RR));
                insnList->tail->mode = 1;
            }
            goto L4545;
        } break;
        case ilRVAL: {
            // A value already in ACC can still be a slice: GETFIELD on a
            // function result records shift/width without an address to
            // load from.  genSliceExtract clears st, so the jump in from
            // ilLVAL cannot extract twice.
            if (insnList->st == stSLICE)
                genSliceExtract();
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
    insnList->st = stWORD;      // the value is a plain word in ACC now
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
            ((l4var7z == kindStruct) and insnList->typ.rep()->lsbord);
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
            addToInsnList(getHelperProc(48)); /* "P/STAR" */
            insnList->tail->mode = 1;
        }
    }
} /* prepStore */

/* Rotate a 48-bit set left/right by `amt` (negative = left). Matches
   `shift`; the exp-normalization of `amt` is a host no-op. */
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
            // A right-aligned pointer field needs no extraction in this mode
            // either: mcACC2ADDR is ATI, which takes A[15:1], exactly the
            // bits WTC takes above.  Without this, prepLoad's stSLICE arm
            // would emit a redundant shift/mask first.
            if (insnList->st == stSLICE and insnList->shift == 0)
                insnList->st = stWORD;
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
        // power-of-2 divisors (card==1) collapse to a single
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
        // Bit 16 is not a register: it is the condition-polarity flag, and
        // each sub-chain's own is spent by the time it is folded in -- the
        // condition's by the UZA/U1A selected above, an arm's by the prepLoad
        // that materialized its value.  Carrying one into the composite would
        // tell the consumer that this ilRVAL is a negated condition, and
        // prepLoad or formOperator(BRANCH) would invert the ternary's value.
        // genBoolAnd and genComparison mask it off here for the same reason.
        condChain->regsused = (condChain->regsused | thenChain->regsused
                              | insnList->regsused | Bits(0)) & ~ Bits(16);
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
                        helperNames[47] | curVal.ii)+(KVTM+I11));
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

    ExprPtr l5exp1z, l5exp2z, calleeExp;
    IdentRecPtr l5idr5z;
    bool isProc, firstArg, isIndir, isFortrn, isAssembler, allByRef;
    int64_t calleeFl, frameSiz, numArgs;
    int64_t l5var15z;
    Word l5var17z, l5var18z, l5var19z;
    InsnListPtr l5inl20z;
    TPtr routTyp, resTyp;
    IdClass paramClass;
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
    isIndir = exprToGen->op == INDCALL;
    if (isIndir) {
        // Everything about the callee comes from the type it is reached
        // through: the result, the argument count, and the flags that shape
        // the call.  Nothing is known about the registers the routine at the
        // other end uses, so every one of them counts as clobbered.
        calleeExp = exprToGen->expr2;
        routTyp = ptrBase(calleeExp->vt.typ);
        resTyp = routTyp.rep()->rresult;
        numArgs = routTyp.rep()->rargc;
        calleeFl = routTyp.rep()->rflags | BitRange(0,15);
    } else {
        l5idr5z = exprToGen->id2;
        resTyp = l5idr5z->typ;
        numArgs = argCount(l5idr5z);
        calleeFl = l5idr5z->flags();
    }
    isProc = (resTyp == NULL);
    frameSiz = isProc ? 3 : 4;
    isFortrn = has(calleeFl, 21);
    isAssembler = has(calleeFl, 26);
    allByRef = has(calleeFl, 24);
    insnList = new InsnList;
    insnList->head = NULL;
    insnList->tail = NULL;
    insnList->typ = resTyp;
    insnList->regsused = (calleeFl | BitRange(7,15)) & (BitRange(0,8)|BitRange(10,15));
    insnList->ilm = ilRVAL;
    insnList->st = stWORD;      // prepLoad reads st on every ilRVAL list
    if (isAssembler) {          // assembler routine, no frame
        firstArg = false;
    } else if (isFortrn) {
        firstArg = not isProc;
        if (checkFortran) {
            addToInsnList(getHelperProc(53)); /* "P/MF" */
        }
    } else {
        firstArg = true;
        if (numArgs >= 2) {
            addToInsnList(KUTM+SP + frameSiz);
        }
    }
// (loop)
    while (l5exp1z != NULL) { /* 6574 */
        l5exp2z = l5exp1z->expr2;
        l5exp1z = l5exp1z->expr1;
        l5inl20z = insnList;
        (void) genFullExpr(l5exp2z);
        // Every formal is taken by value; one that does not fit a word is
        // passed by address instead, which the callee knows to expect.
        paramClass = VARID;
          loop:
        if ((paramClass == FORMALID) or allByRef) {
            setAddrTo(14);
            addToInsnList(KITA+14);
        } else {
            if (typeSize(insnList->typ) != 1) {
                paramClass = FORMALID;
                goto loop;
            } else {
                prepLoad();
            }
        } /* 7027 */
        if (not firstArg)
            prependToInsnList(macro + mcPUSH);
        firstArg = false;
        if (l5inl20z->tail != NULL) {
            l5inl20z->tail->next = insnList->head;
            insnList->head = l5inl20z->head;
        }
        insnList->regsused = insnList->regsused | l5inl20z->regsused;
    }; /* while -> 7061 */
    if (isFortrn) {
        addToInsnList(KNTR+2);
        insnList->tail->mode = 4;
    }
    if (isIndir) {
        // WTC takes the entry address out of the pointer and into C, which
        // the VJM then jumps to; it touches neither the accumulator, where
        // the last argument is sitting, nor the mode register.  That is only
        // possible while the pointer is directly addressable, i.e. while its
        // whole insnList is a deferred address and nothing has to be
        // computed to reach it.
        l5inl20z = insnList;
        (void) genFullExpr(calleeExp);
        if (insnList->head != NULL or insnList->ilm != ilLVAL
            or insnList->st != stWORD or insnList->addrmd == 15)
            error(errVarTooComplex);
        curInsnTemplate = InsnTemp[WTC];
        prepLoad();
        curInsnTemplate = InsnTemp[XTA];
        if (l5inl20z->tail != NULL) {
            l5inl20z->tail->next = insnList->head;
            insnList->head = l5inl20z->head;
        }
        insnList->regsused = insnList->regsused | l5inl20z->regsused;
        addToInsnList(KVJM+I13);
        // The callee's level is not knowable here, and only a file-scope
        // routine can be pointed at, so it is 1, as for an external.
        l5var17z.ii = 1;
    } else {
        addToInsnList(allocGlobalObject(l5idr5z) + (KVJM+I13));
        if (has(l5idr5z->flags(), 20)) {
            l5var17z.ii = 1;
        } else {
            l5var17z.ii = l5idr5z->pck.offset / 04000000;
        } /* 7102 */
    } /* 7132 */
    insnList->tail->mode = 2;
    if (not isAssembler and curProcNesting != l5var17z.ii) {
        if (not isFortrn) {
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
    // (not isAssembler) and (isIndir or [20,21]*calleeFl)
    if (not isAssembler
        and (isIndir or ((Bits(20, 21) & calleeFl) != Bits()))) {
        addToInsnList(KVTM+040074001);
    }
    /* A `with` base that lives in a frame slot outlives a call that clobbers
       the register holding it: reload the register right here, so that every
       path reaching the clobber also reaches the restore and the body can go
       on addressing the record through the register.  WTC takes the saved
       address from the slot into C, VTM then lands it in the register --
       neither touches the accumulator, which may hold a function result. */
    l5exp2z = pinList;
    while (l5exp2z != NULL) {
        if (l5exp2z->vt.typ.p.psize != 0
            and (Bits(l5exp2z->vt.typ.p.pad) & calleeFl) != Bits()) {
            addInsnAndOffset(curFrameRegTemplate + KWTC,
                             l5exp2z->vt.typ.p.psize - 1);
            addToInsnList(KVTM + indexreg[l5exp2z->vt.typ.p.pad]);
        }
        l5exp2z = l5exp2z->expr1;
    }
    usedRegs = (usedRegs | calleeFl) & BitRange(1,15);
    if (isFortrn) {
        if (not checkFortran)
            addToInsnList(KNTR+7);
        else
            addToInsnList(getHelperProc(54));    /* "P/FM" */
        insnList->tail->mode = 2;
    } /* 7226 */
    // NB: no `else` here -- a non-Fortran function returns
    // its value in ACC, so there is no `KXTA+SP` reload of the result.
    if (not isProc) {
        insnList->typ = resTyp;
        insnList->regsused = insnList->regsused | Bits(0L);
        insnList->ilm = ilRVAL;
        liveRegs = liveRegs & ~ calleeFl;
    }
    /* 7237 */
} /* genEntry */

void startInsnList()
{
    ExprPtr & exprToGen = genFullExpr::super.back()->exprToGen;
    insnList = new InsnList;
    insnList->tail = NULL;
    insnList->head = NULL;
    insnList->typ = exprToGen->vt.typ;
    insnList->regsused = Bits();
    insnList->ilm = ilCONST;
    insnList->payload.ii = exprToGen->num1;
    insnList->addrmd = exprToGen->num2;
    insnList->st = stWORD;
}

void genCopy()
{
    int64_t size;
    InsnList * lhsIns, * rhsIns;
    int64_t &work = genFullExpr::super.back()->work;
    InsnList * &otherIns = genFullExpr::super.back()->otherIns;

    size = typeSize(insnList->typ);
    if (size == 1) {
        // Merge the rhs-load and lhs-store instruction lists into insnList.
        // Build the list rather than emitting directly: emitting leaves
        // insnList consumed, and the caller then dereferences NULL.
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
            nextInsn = 41;      /* P/IN */
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
            addToInsnList(getHelperProc(50 + l3int3z)); /* P/EQ */
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
        } else { /* 7625: a foldable op with two constant operands is already
                    folded to GETENUM at construction (mkExprFold), so only the
                    non-constant codegen path remains here. */
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
                    if (insnList == NULL)
                        return;
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
    } else { /* 10125 */
        if (curOP <= DEREF) {
            if (curOP == GETVAR) {
                insnList = new InsnList;
                curIdRec = exprToGen->id1;
                insnList->tail = NULL;
                insnList->head = NULL;
                insnList->regsused = Bits();
                insnList->ilm = ilLVAL;
                insnList->payload.ii = curIdRec->pck.offset;
                insnList->disp = curIdRec->value();
                insnList->st = stWORD;
                insnList->addrmd = 18;
                if (curIdRec->pck.cl == ROUTINEID) {
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
                insnList->disp = insnList->disp + curIdRec->pck.offset;
                if (curIdRec->pckfield()) {
                    switch (insnList->st) {
                    case stWORD:
                        insnList->shift = curIdRec->shift();
                        break;
                    case stSLICE: {
                        insnList->shift = insnList->shift + curIdRec->shift();
                        if (not curIdRec->uptype().rep()->lsbord)
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
            else if (curOP == DEREF) {
                genFullExpr(exprToGen->expr1);
                genDeref();
            } else if (curOP == GETENUM)
                startInsnList();
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
        } else if (curOP == PROCADDR) {
            /* A function designator: its entry address as a value.  VTM
               lands the address the loader relocates into the instruction
               in the tag register, ITA turns it into an integer word. */
            insnList = new InsnList;
            insnList->tail = NULL;
            insnList->head = NULL;
            insnList->typ = exprToGen->vt.typ;
            insnList->regsused = Bits(0L);
            insnList->ilm = ilRVAL;
            insnList->st = stWORD;
            insnList->addrmd = 18;
            insnList->payload.ii = 0;
            insnList->disp = 0;
            insnList->width = 0;
            insnList->shift = 0;
            addToInsnList(allocGlobalObject(exprToGen->id2) + (KVTM+I14));
            addToInsnList(KITA+14);
        } else if (curOP == ALNUM or curOP == INDCALL)
            genEntry();
        else if (has(BitRange(TOREAL, BITNEGOP), curOP)) {
            genFullExpr(exprToGen->expr1);
            /* A unary op with a constant operand is already folded at
               construction (mkUnaryFold/castToReal), so there is no ilCONST
               case to handle here. */
            if (curOP == NOTOP) {
                negateCond();
            } else {
                prepLoad();
                if (curOP == TOREAL) {
                    addToInsnList(KAOX+ZERO);
                    addToInsnList(InsnTemp[AVX]);
                    l3int3z = 3;
                    goto L10122;
                } else if (curOP == TOINT) {
                    /* Real to integer truncates toward zero, in C/TR
                       (libc); the helper returns with the machine in
                       integer mode. */
                    l3int3z = 2;
                    addToInsnList(getHelperProc(C_TR));
                    goto L10122;
                } else if (curOP == BITNEGOP) {
                    addToInsnList(KAEX+ALLONES);
                    l3int3z = 1;
                    goto L10122;
                } else {
                    addToInsnList(KAVX+ALLONES);
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
                case fnCARD:  arg1Val.ii = card(arg1Val.ii);
                    break;
                case fnMINEL: arg1Val.ii = minel(arg1Val.ii);
                    break;
                case fnABSI:  arg1Val.ii = labs(arg1Val.ii);
                    break;
                case fnMALLOC:
                    addToInsnList(KVTM+I14+getValueOrAllocSymtab(arg1Val.ii));
                    addToInsnList(getHelperProc(17)); /*"P/NW"*/
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
                if (work == fnCARD) {
                    l3int3z = 0;
                } else if (work == fnABS)
                    l3int3z = 3;
                else {
                    l3int3z = 1;
                }
                addToInsnList(funcInsn[work]);
                goto L10122;
            }
        } else { /* 10621 */
            if (curOP == NOOP) {
                curVal.ii = exprToGen->vt.typ.p.pad;
                /* A spilled base is reloaded into its register after every
                   call that clobbers it, so it needs no liveness test. */
                if (exprToGen->vt.typ.p.psize != 0
                    or has(liveRegs, curVal.ii)) {
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
                    exprToGen->vt.typ.p.pad = 14;
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
        formAndAlign(getHelperProc(36)); /*"FCLOSE"*/
    };

    if (has(optSflags.ii, S5)) {
        formAndAlign(KUJ+I13);
        return;
    }
    form2Insn(KITS+13, KATX+SP);
    if (inputFile != NULL) {
        fcloseFile(inputFile);
        form1Insn(KXTA+SP);  // remove FCLOSE's stacked FCB argument
    }
    if (outputFile != NULL)
        fcloseFile(outputFile);
    form1Insn(getHelperProc(42)/*"P/IT"*/ + (KUJ-KVJM-I13));
    padToLeft();
} /* formFileInit */

formOperator::formOperator(OpGen op)
{ /* formOperator */
    int64_t & localSize = programme::super.back()->localSize;
    int64_t & sizeCount = programme::super.back()->sizeCount;

    super.push_back(this);
    rhsMode = true;
    if ((errors and (op != SETREG)) or curExpr == NULL)
        return;
    if (op != FORMOP &&
        op != STOREAT9 &&
        op != DFLTWDTH &&
        op!=PCKUNPCK)
        (void) genFullExpr(curExpr);
    switch (op) {
    case gen0:
        break; /* placeholder OpGen slot, never passed */
    case DOIT:
        genOneOp();
        break;
    case SETREG: {
        if (insnList->head == NULL)
            l3int3z = 0;
        else if (insnList->head == insnList->tail)
            l3int3z = 1;
        else
            l3int3z = 2;
        helpExpr = new Expr;
        helpExpr->expr1 = pinList;
        pinList = helpExpr;
        helpExpr->op = NOOP;
        /* The entry holds the index register carrying the record address
           in vt.typ.p.pad, and -- when that address was too costly to
           recompute and went to a frame slot -- the slot number plus one
           in vt.typ.p.psize.  genEntry reloads the register from that slot
           after every call that clobbers it, so a spilled entry's
           register is valid throughout the body. */
        helpExpr->vt.typ.p.psize = 0;
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
                    addInsnAndOffset(curFrameRegTemplate, localSize);
                    genOneOp();
                    helpExpr->vt.typ.p.psize = localSize + 1;
                    localSize = localSize + 1;
                    if (sizeCount < localSize)
                        sizeCount = localSize;
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
            helpExpr->vt.typ.p.pad = l3int2z;
        } break;
        case stSLICE: {
            curVal.ii = 14;
            helpExpr->vt.typ.p.pad = 14;
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
    case SETREG11: case PUSHSET11: {
        setAddrTo(11);
        if (op == PUSHSET11)
            prependToInsnList(macro + mcPUSH);
        genOneOp();
        usedRegs = usedRegs | Bits(12);
    } break;
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
        (void) formOperator(SETREG11);
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
        if (l3int3z == 44)          /* P/KC */
            l3int1z = 1 - l3int1z;
        form1Insn(getValueOrAllocSymtab(l3int1z) + (KVTM+I9));
        l3int1z = InsnTemp[XTA];
        form1Insn(l3int1z);
        formAndAlign(getHelperProc(l3int3z));
   } break;
    } /* case */
} /* formOperator */

/* Extract the value of a constant expression into curVal.  With folding at
   construction a constant expression is already a GETENUM node, so read it
   directly instead of lowering it through genFullExpr, which saves an insnList
   allocation per constant (case labels, const decls, array bounds, besm). */
void takeConstFromExpr()
{
    if (errors or curExpr == NULL)
        return;
    if (curExpr->op != GETENUM)
        error(errNoConstant);
    else if (typeSize(curExpr->vt.typ) != 1)
        error(errConstOfOtherTypeNeeded);
    else
        curVal = curExpr->lit;
}

void markTypeSym()
{
    if (SY == IDENT) {
        // curVal.ii := curIdent.ii * hashMask.ii; mapAI(curVal.a, bucket);
        bucket = curIdent % 65535 % 128;
        hashTravPtr = symHash[bucket];
        while (hashTravPtr != NULL and hashTravPtr->id != curIdent)
            hashTravPtr = hashTravPtr->next();
        if (hashTravPtr != NULL and hashTravPtr->pck.cl == TYPEID) {
            SY = TYPESY;
            symType = hashTravPtr->typ;
        }
    }
} /* markTypeSym */

// File scope (not nested in parseTypeRef): makeArrayType and the shared
// declarator parser (below) both need to construct/consume range bounds,
// and a struct type nested inside a function is only visible within that
// function's own scope in this codebase's Pascal-mirroring conventions.
struct rangeRec { int64_t aleft, aright; };
typedef rangeRec rangeList[21]; // array [1..20] of rangeRec;

TPtr makeArrayType(int64_t aleft, int64_t aright, TPtr elem, bool makePacked)
{
    int64_t span, l3int22z, numBits;
    int64_t sizeVal, bitsVal, perwordVal, pcksizeVal;
    TPtr arrayType{};

    span = aright - aleft + 1;
    l3int22z = typeBits(elem);
    /* Nothing wider than half a word packs, so the request is refused here
       and the type records what it was given. */
    if (24 < l3int22z)
        makePacked = false;
    InternRec * icand = internHead;
    while (icand != NULL) {
        arrayType = icand->ityp;
        if (arrayType.p.pk == kindArray and
            arrayType.rep()->base == elem and
            arrayType.rep()->aleft == aleft and
            arrayType.rep()->aright == aright and
            arrayType.rep()->pck == makePacked)
            return arrayType;
        icand = icand->inext;
    }
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
    arrayType.setRep(besm6_alloc_record<Types>(offsetof(Types, szArray)));
    arrayType.rep()->aleft = aleft;
    arrayType.rep()->aright = aright;
    arrayType.rep()->base = elem;
    arrayType.rep()->pck = makePacked;
    arrayType.rep()->perword = perwordVal;
    arrayType.rep()->pcksize = pcksizeVal;
    arrayType.p.psize = sizeVal;
    arrayType.p.bits = bitsVal;
    arrayType.p.pk = kindArray;
    icand = new InternRec;
    icand->ityp = arrayType;
    icand->inext = internHead;
    internHead = icand;
    return arrayType;
} /* makeArrayType */

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

    int64_t skipTarget;
    bool isPacked;
    // '__packed __lsb': this struct's fields fill the word from bit 0.  A
    // per-instance member, so a member's own order cannot leak into its
    // container (work.p2c saves/restores a global instead).
    bool lsbOrder;
    // Set when this call resolved its base type via the forward-reference
    // placeholder path below (an undefined name used mid-typedef, e.g.
    // 'typedef expr *eptr;' parsed before 'expr' itself is defined): the
    // returned curType is a heap-allocated (allocPtr, not compact
    // getPtrType) pointer-to-int stand-in whose base field the *later*
    // real definition of the name patches in place. Callers applying a
    // declarator '*' directly to such a curType must reuse it as-is
    // instead of wrapping it in another getPtrType -- see parseOneDeclarator.
    bool isForwardRef;
    bool cond;
    caserec cases;
    int64_t numBits, l3int22z, span, rangeCnt, curDim;
    int64_t nextEnum, enumName, enumBucket;
    bool hasExplicit;
    IdentRecPtr curEnum, curField;
    TPtr arrayType{}, nestedType{}, tempType{}, curType{};
    rangeList ranges;
    rangeRec curRange;
    IdentRecPtr l3idr31z;

    void definePtrType(TPtr toType) {
        IdentRecPtr & typelist = programme::super.back()->typelist;
        /* Heap-allocated pointer descriptor (forward-placeholder, patched in
           place when the real pointee type is later defined). */
        curType.setRep(besm6_alloc_record<Types>(offsetof(Types, szPtr)));
        curType.rep()->base = toType;
        curType.p.psize = 1;
        curType.p.bits = 15;
        curType.p.pk = kindPtr;
        curEnum = besm6_alloc_record<IdentRec>(offsetof(IdentRec, szIdent));
        curEnum->pck.nidx = ord(typelist);
        curEnum->id = curIdent;
        curEnum->pck.offset = lineCnt;
        curEnum->typ = curType;
        curEnum->pck.cl = TYPEID;
        typelist = curEnum;
    } /* definePtrType */
};
std::vector<parseTypeRef*> parseTypeRef::super;

// --- Unified C declarator parsing -----------------------------------
//
// One declarator grammar shared by variable/typedef/struct-field
// declarations (grouped: one type-spec, comma-separated declarator
// list) and routine parameters (individual: each comma-separated item
// carries its own type-spec).
//
// Placed here (right after parseTypeRef's declaration, ahead of its
// out-of-line constructor and of parseRecordDecl) so both can call
// parseGroupedDecls; parseRange's full definition comes later, hence
// the forward declaration.
void parseRange(int64_t & aleft, int64_t & aright);
void parseConstDeclValue(TPtr &typ, Word &value);

struct Declarator {
    int64_t name = 0;
    int64_t bucket = 0;
    // wasDefined mirrors isDefined, meaningful only under lookDef, which is
    // the typedef and parameter namespaces: inSymbol's case 0 sets isDefined
    // when an entry already exists at the current scope. The plain variable
    // and routine declaration loop runs in lookUse, so wasDefined is false
    // throughout it. lookField (struct fields) never sets isDefined at all --
    // its own match check leaves hashTravPtr pointing at the match (or NULL),
    // captured below as foundRec (foundRec != NULL is the field "already
    // defined" signal).
     bool wasDefined = false;
    // The matched symbol-table entry itself (NULL if none), so a caller
    // merging routine/variable dispatch (see the unified TYPESY
    // loop in programme's constructor) can check cl/preDefLink/typ to
    // recognize a forward-declared routine being redefined, without a
    // second hash lookup.
    IdentRecPtr foundRec = NULL;
    TPtr type{};
    // The declarator applied nothing but '*' ops, so it is a name a routine
    // header may use: 'T name', 'T *name'.  An opFun ('T (*fp)(args)') or an
    // opArray ('T a[3]') clears it, and those are variables.
    bool ptrOnly = true;
};

// One declarator operator.  opFun carries the parameter signature its
// '(...)' spelled out; opArray carries its bounds.
enum DclOpKind { opPtr, opArray, opFun };

struct DclOp {
    DclOpKind opKind;
    SigPtr sig;
    rangeRec range;
};

// Set by parseParameters and parseSignature around their parseOneDeclarator
// calls: in a parameter list the declarator may be abstract, i.e. carry no
// name at all.  A global, so parseOneDeclarator's other call sites, none of
// which can accept an abstract declarator, stay as they are.
bool nameOptional = false;

// The parameter list of a function declarator, as a signature: types and
// nothing else, the names (if any) discarded.  Defined below, after
// parseOneDeclarator, which it calls for each parameter.
SigPtr parseSignature();

// '*'* ('(' declarator ')' | IDENT) ('[' range ']' | '(' signature ')')*
// Collects pointer/array/function operators while descending; the caller
// applies them in reverse so precedence matches C: `int *a[10]` is an array
// of pointers (the array op is pushed by the inner IDENT case before the
// enclosing '*' case pushes its own op on the way back out); `int
// (*row)[3]` is a pointer to an array (the parens make the '*' push
// before the outer '[3]' does), and `int (*f)(char)` is a pointer to a
// routine, the '*' likewise pushed before the function op.
// The '(' suffix is taken only after a parenthesized declarator, which is
// exactly where C puts it: in `int f(char)` and `int *f(char)` the '(' binds
// to the name and declares a routine, whose parameter list belongs to
// parseParameters (it gives the parameters records of their own), while in
// `int (*f)(char)` it binds to the group and is part of a type.
void readDeclaratorCore(std::vector<DclOp> & ops, Declarator & d)
{
    bool wasGroup = false;
    if (charClass == MUL) {
        inSymbol();
        readDeclaratorCore(ops, d);
        ops.push_back({opPtr, NULL, {}});
    } else if (SY == LPAREN) {
        inSymbol();
        readDeclaratorCore(ops, d);
        checkSymAndRead(RPAREN);
        wasGroup = true;
    } else if (SY == IDENT) {
        d.name = curIdent;
        d.bucket = bucket;
        d.wasDefined = isDefined /* ||
            (lookupMode == lookDef &&
             (curIdent == litInput || curIdent == litOutput)) */;
        d.foundRec = hashTravPtr;
        inSymbol();
    } else if (nameOptional and has(Bits(RPAREN, COMMA, LBRACK), SY)) {
        // Abstract declarator: a formal parameter's name is optional
        // ('int', 'int *', 'int [0..2]').  work.p2c's curDeclarator is a
        // global and clears the fields the name would have filled; mirrored
        // here, where Declarator is fresh per call.
        d.name = 0;
        d.bucket = 0;
        d.wasDefined = false;
        d.foundRec = NULL;
    } else {
        error(errNoIdent);
        d.name = 0;
    }
    while (SY == LBRACK or (wasGroup and SY == LPAREN)) {
        if (SY == LPAREN) {
            ops.push_back({opFun, parseSignature(), {}});
        } else {
            inSymbol();
            rangeRec r{};
            parseRange(r.aleft, r.aright);
            checkSymAndRead(RBRACK);
            ops.push_back({opArray, NULL, r});
        }
    }
}

// packedFlag mirrors parseTypeRef's own array-suffix handling ('TYPE
// [range]', its curDim==1 case): only the outermost array dimension of
// a multi-dim declarator (e.g. int matrix[2][3]'s [2]) carries it, since
// makeArrayType's flag describes how elements of the final array are
// packed, not
// each nesting level. ops is applied innermost-first (reverse of source
// order), so the outermost dimension is ops.front(), processed last.
Declarator parseOneDeclarator(TPtr baseType, bool packedFlag = false,
                              bool isForwardRef = false)
{
    Declarator d;
    std::vector<DclOp> ops;
    readDeclaratorCore(ops, d);
    d.type = baseType;
    bool firstOp = true;
    for (auto it = ops.rbegin(); it != ops.rend(); ++it) {
        /* A switch loads the field once per operator and turns each kind
           into a label. */
        switch (it->opKind) {
        case opPtr:
            // baseType is already the forward-reference placeholder
            // pointer itself (see parseTypeRef::isForwardRef) -- applying
            // getPtrType on top would compact-encode a second, bogus
            // pointer-to-pointer layer that never gets patched when the
            // real type is later defined. Only the very first op can be
            // this case (it's applied directly to baseType).
            if (not (firstOp and isForwardRef))
                d.type = getPtrType(d.type);
            break;
        case opFun:
            d.type = mkRoutineTyp(d.type, it->sig, 0);
            d.ptrOnly = false;
            break;
        default:
            d.type = makeArrayType(it->range.aleft, it->range.aright, d.type,
                                   packedFlag and (&*it == &ops.front()));
            d.ptrOnly = false;
        }
        firstOp = false;
    }
    return d;
}

// '(' (typeref declarator (',' typeref declarator)*)? ')', with SY at the
// '(': the parameter list of a function declarator.  Only the types reach
// the signature; a name spelled out here is read and dropped, since these
// parameters have no storage and no scope to be visible in.  Every entry is
// VARID -- a parameter taken by address is not expressible in a type.
SigPtr parseSignature()
{
    SigPtr head = NULL, last = NULL, cur;
    inSymbol();
    if (SY != RPAREN) {
        bool noComma;
        do {
            TPtr paramType{};
            // Scoped exactly as parseGroupedDecls's typeParser is.
            bool packedFlag;
            {
                parseTypeRef sigTypeParser(paramType,
                                           skipToSet | Bits(IDENT, RPAREN, COMMA));
                packedFlag = sigTypeParser.isPacked;
            }
            nameOptional = true;
            Declarator d = parseOneDeclarator(paramType, packedFlag);
            nameOptional = false;
            cur = new SigRec;
            cur->pclass = VARID;
            cur->ptyp = d.type;
            cur->next = NULL;
            if (last == NULL)
                head = cur;
            else
                last->next = cur;
            last = cur;
            noComma = (SY != COMMA);
            if (not noComma)
                inSymbol();
        } while (not noComma);
    }
    checkSymAndRead(RPAREN);
    return head;
}

// Parses 'TYPE decl (, decl)* ;' (the grouped form used by variable,
// typedef and struct-field declarations): one type-spec via
// parseTypeRef, then comma-separated declarators sharing it. Caller
// supplies `reg` to register each resulting declarator (as a variable,
// a typedef name, or a field).
void parseGroupedDecls(int64_t skipTarget,
                       std::function<void(Declarator&)> reg)
{
    TPtr baseTy{};
    // Named (not a temporary) so isPacked is still readable after the
    // type-spec is parsed, to apply to each declarator's own array
    // suffix below (parseTypeRef's own '__packed TYPE[range]' form
    // consumes/resets isPacked itself, but here the range is a
    // declarator suffix, not part of the type-spec tokens, so nothing
    // resets it). Scoped to end right here, though: parseTypeRef::super
    // must be popped back to the *enclosing* type-spec (e.g. the struct
    // this field-group belongs to, if any) before reg() runs below --
    // packOneField reads parseTypeRef::super.back()->cases expecting
    // that enclosing record's running bit-packing state, not this
    // field's own fresh one, and every struct's psize silently came out
    // 0 (breaking array-of-struct sizing) while typeParser was still on
    // the stack during registration.
    bool packedFlag;
    bool forwardRef;
    {
        parseTypeRef typeParser(baseTy, skipTarget | Bits(COMMA, SEMICOLON));
        packedFlag = typeParser.isPacked;
        forwardRef = typeParser.isForwardRef;
    }
    bool more;
    do {
        Declarator d = parseOneDeclarator(baseTy, packedFlag, forwardRef);
        if (d.name != 0)
            reg(d);
        more = (SY == COMMA);
        if (more)
            inSymbol();
    } while (more);
    checkSymAndRead(SEMICOLON);
}

struct parseRecordDecl {
    static std::vector<parseRecordDecl*> super;
    parseRecordDecl(TPtr & rectype, bool isOuterDecl_, bool isUnion_);
    ~parseRecordDecl() { super.pop_back(); }

    bool isOuterDecl;
    bool isUnion;
    IdentRecPtr prevField;
    parseTypeRef::caserec cases1, cases2;
};
std::vector<parseRecordDecl*> parseRecordDecl::super;

// True if union member `c` extends the union past the running maximum `mx`.
// Bigger by word size, or (for a single packed word) using more bits --
// preserving the exact overlap rule of the former variant-tail loop.
static bool unionMemberBigger(const parseTypeRef::caserec & mx,
                              const parseTypeRef::caserec & c, bool packed)
{
    return (mx.size < c.size) or
           (packed and (c.size == 1) and (mx.size == 1) and
            (c.count == 1) and (mx.count == 1) and
            (c.pairs[1].first < mx.pairs[1].first));
}

// Assigns fld's offset (and, in a __packed struct, its bit shift/width)
// within the enclosing record's shared bit-packing state (cases), then
// bumps that state past fld. Called once per field of a C-style
// grouped declaration ('TYPE a, b;'), each with its own already-resolved
// fldType (a, b may differ once pointer/array declarator ops are applied).
void packOneField(IdentRecPtr fld, TPtr fldType)
{
    int64_t fieldWidth, pairIdx, minFirst, scanIdx, curFirst;
    parseTypeRef::pair * curSlot;

    parseTypeRef::caserec &cases = parseTypeRef::super.back()->cases;
    bool &isPacked = parseTypeRef::super.back()->isPacked;
    bool &lsbOrder = parseTypeRef::super.back()->lsbOrder;

    fld->typ = fldType;
    if (isPacked) {
        fieldWidth = typeBits(fldType);
        fld->width() = fieldWidth;
        if (fieldWidth != 48) {
            for (pairIdx = 1; pairIdx <= cases.count; ++pairIdx) {
L11523:         curSlot = &cases.pairs[pairIdx];
                if (curSlot->first >= fieldWidth) {
                    fld->shift() = 48 - curSlot->first;
                    fld->pck.offset = curSlot->second;
                    if (not lsbOrder)
                        fld->shift() = 48 - fld->width() - fld->shift();
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
    fld->pckfield() = false;
    fld->pck.offset = cases.size;
    cases.size = cases.size + typeSize(fldType);
L11622:
    if (PASINFOR.listMode == 3) {
        printf("%16c", ' ');
        if (fld->pckfield())
            printf("PACKED");
        printf(" FIELD ");
        printTextWord(fld->id);
        printf(".OFFSET=%05loB", (long)fld->pck.offset);
        if (fld->pckfield()) {
            printf(".<<=SHIFT=%2ld. WIDTH=%2ld BITS", fld->shift(),
                   fld->width());
        } else {
            printf(".WORDS=%ld", typeSize(fldType));
        }
        putchar('\n');
    }
} /* packOneField */

parseRecordDecl::parseRecordDecl(TPtr & rectype, bool isOuterDecl_, bool isUnion_)
    : isOuterDecl(isOuterDecl_), isUnion(isUnion_)
{
    TPtr &curType = parseTypeRef::super.back()->curType;
    IdentRecPtr &curEnum = parseTypeRef::super.back()->curEnum;
    IdentRecPtr &l3idr31z = parseTypeRef::super.back()->l3idr31z;
    bool &isPacked = parseTypeRef::super.back()->isPacked;
    int64_t &skipTarget = parseTypeRef::super.back()->skipTarget;
    parseTypeRef::caserec &cases = parseTypeRef::super.back()->cases;
    int64_t savedLookup2 = lookup2;

    super.push_back(this);

    if (SY != BEGINSY)
        requiredSymErr(BEGINSY);
    // lookup2 (not just lookupMode) must carry lookField through
    // parseTypeRef's own internal inSymbol() calls -- every inSymbol()
    // resets lookupMode := lookup2 on exit, so a field's declarator name
    // a few tokens into its type-spec is classified under whatever
    // lookup2 holds, not this line's lookupMode. Restored to the caller's
    // value below (this ctor recurses into itself for anonymous nested
    // members, and runs while an outer context's own lookup2 is still live).
    lookup2 = lookField;
    lookupMode = lookField;
    inSymbol();

    // A record body is a member list terminated by ENDSY. In a struct the
    // members are laid out sequentially; in a union they overlap at the base
    // (cases1) and the record spans the largest of them (cases2). A member is
    // either a C field group ('TYPE a, b;' -- one FIELDID per declarator,
    // packed onto the shared flat field chain; TYPE may be a named nested
    // record) or an inline anonymous nested struct/union, whose fields are
    // parsed straight into curType (promoted -- true-C anonymous members).
    // Inline 'struct {'/'union {' in a body is always anonymous; a named
    // nested record uses a typedef'd type name.
    cases1 = cases;                     // union base: offset + packing state
    cases2 = cases;                     // running max extent (union only)
    while (has(Bits(IDENT, TYPESY, ENUMSY, STRUCTSY) | Bits(UNIONSY, PACKEDSY), SY)) {
        if (SY == STRUCTSY or SY == UNIONSY) {
            bool nestedIsUnion = (SY == UNIONSY);
            if (isUnion)
                cases = cases1;
            lookupMode = lookField;
            inSymbol();                 // consume 'struct'/'union'
            parseRecordDecl(curType, false, nestedIsUnion);   // consumes { ... }
            if (isUnion and unionMemberBigger(cases2, cases, isPacked))
                cases2 = cases;
            if (SY == SEMICOLON) {
                lookupMode = lookField;
                inSymbol();
            }
        } else {
            parseGroupedDecls(skipTarget | Bits(UNIONSY, STRUCTSY, ENDSY),
                [&](Declarator & d) {
                    // In a union each declarator is a separate member starting
                    // at the union base.
                    if (isUnion)
                        cases = cases1;
                    // lookField never sets isDefined (see Declarator);
                    // foundRec != NULL is the correct "already a field of this
                    // record" signal here.
                    if (d.foundRec != NULL)
                        error(errIdentAlreadyDefined);
                    curEnum = besm6_alloc_record<IdentRec>(
                        offsetof(IdentRec, szField));
                    curEnum->id = d.name;
                    curEnum->pck.nidx = ord(fieldHash[d.bucket]);
                    curEnum->pck.cl = FIELDID;
                    curEnum->uptype() = curType;
                    curEnum->pckfield() = isPacked;
                    fieldHash[d.bucket] = curEnum;
                    if (curType.rep()->fields == NULL)
                        curType.rep()->fields = curEnum;
                    else
                        l3idr31z->list() = curEnum;
                    l3idr31z = curEnum;
                    packOneField(curEnum, d.type);
                    if (isUnion and unionMemberBigger(cases2, cases, isPacked))
                        cases2 = cases;
                });
        }
        lookupMode = lookField;
    }
    if (isUnion)
        cases = cases2;

    // psize/bits belong to the *type* being defined; an inline anonymous
    // member (isOuterDecl_ == false) has no type of its own -- its extent is
    // already reflected in the shared `cases` that its caller reads.
    if (isOuterDecl_) {
        rectype.p.psize = cases.size;
        if (isPacked and (cases.size == 1) and (cases.count == 1))
            rectype.p.bits = 48 - cases.pairs[1].first;
        else
            rectype.p.bits = 48;
        prevField = rectype.rep()->fields;
        while (prevField != NULL) {
            prevField->uptype() = rectype;
            prevField = prevField->list();
        }
    }
    lookup2 = savedLookup2;
    checkSymAndRead(ENDSY);
} /* parseRecordDecl */

// parseRange's definition lives past the Statement struct (near
// parseConstDeclValue): array bounds are const-expressions, evaluated by
// running Statement() in ceRegs mode, which needs the full Statement
// definition in scope.  Only the forward declaration (above) is visible here.

parseTypeRef::parseTypeRef(TPtr & newtype, int64_t skipTarget_)
    : skipTarget(skipTarget_)
{
    bool &inTypeDef = programme::super.back()->inTypeDef;
    super.push_back(this);
    isPacked = false;
    lsbOrder = false;
    isForwardRef = false;
L12247:
    if (SY == IDENT)
        markTypeSym();
    if (SY == ENUMSY) {
        inSymbol();
        checkSymAndRead(BEGINSY);
        span = 0;
        nextEnum = 0;
        hasExplicit = false;
        lookupMode = lookDef;
        curField = NULL;
        curType.setRep(
            besm6_alloc_record<Types>(offsetof(Types, szScalar)));
        while (SY == IDENT) {
            if (isDefined || curIdent == litInput || curIdent == litOutput)
                error(errIdentAlreadyDefined);
            enumName = curIdent;
            enumBucket = bucket;
            inSymbol();
            // Optional '= constExpr': an explicit enumerator value.  Later
            // enumerators auto-increment from it. Evaluated through the shared
            // const-expression path (Statement() in freeRegs==ceRegs mode).
            if (charClass == ASSIGNOP) {
                TPtr enumTyp{};
                Word enumVal;
                inSymbol();
                parseConstDeclValue(enumTyp, enumVal);
                if (enumTyp != NULL and enumTyp.p.pk == kindScalar)
                    nextEnum = enumVal.ii;
                else
                    error(62); /* errIntNeeded */
                hasExplicit = true;
            }
            curEnum = besm6_alloc_record<IdentRec>(
                offsetof(IdentRec, szIdent));
            curEnum->pck.nidx = ord(symHash[enumBucket]);
            curEnum->id = enumName;
            curEnum->pck.offset = curFrameRegTemplate;
            curEnum->typ = curType;
            curEnum->pck.cl = ENUMID;
            curEnum->list() = NULL;
            curEnum->value() = nextEnum;
            symHash[enumBucket] = curEnum;
            nextEnum = nextEnum + 1;
            span = span + 1;
            if (curField == NULL) {
                curType.rep()->enums = curEnum;
            } else {
                curField->list() = curEnum;
            }
            curField = curEnum;
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
            // start = -1 suppresses the name table (writeProc then prints the
            // value as an integer): explicit values can be sparse or negative,
            // so there is no dense name array to index by value.
            curType.rep()->start = hasExplicit ? -1 : 0;
            curType.p.psize = 1;
            // Explicit values may be sparse or negative -> a full 48-bit
            // value field (like int); else the packed minimum.
            curType.p.bits = hasExplicit ? 48
                             : (48 - minel((span - 1) & ((1L << 48) - 1)));
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
                curType = getPtrType(symType);
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
            curType = symType;
        } else if (hashTravPtr == NULL and inTypeDef) {
            // C-style forward-referenced typedef pointer, e.g.
            // 'typedef expr *eptr;' parsed before 'expr' itself is
            // defined (common for mutually-recursive record types).
            // Mirrors the '^expr' handling in the MUL branch above,
            // just entered from a bare (not '*'-prefixed) name because
            // here the '*' is the caller's declarator, not ours.
            if (knownInType(curEnum)) {
                curType = curEnum->typ;
            } else {
                definePtrType(IntegerType);
            }
            isForwardRef = true;
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
            // '__lsb' after '__packed' packs the fields of a struct from the
            // low end of the word (the first one starts at bit 0) instead of
            // from the high end.  Recognized by its spelling, like FORTRAN and
            // ASSEMBLER, so it stays an ordinary identifier elsewhere; the
            // test must precede the markTypeSym at L12247.
            if (SY == IDENT and curIdent == litLsb) {
                lsbOrder = true;
                inSymbol();
            }
            goto L12247;
        }
        if (SY == STRUCTSY or SY == UNIONSY) {
            // A union is a kindStruct whose members overlap (same base offset,
            // size = max member); the only difference from a struct is the
            // layout mode passed to parseRecordDecl.
            bool typeIsUnion = (SY == UNIONSY);
            curType.setRep(
                besm6_alloc_record<Types>(offsetof(Types, szStruct)));
            curType.rep()->variants.setRep(NULL);
            curType.rep()->fields = NULL;
            curType.rep()->flag = false;
            curType.rep()->lsbord = lsbOrder;
            curType.p.psize = 0;
            curType.p.bits = 48;
            curType.p.pk = kindStruct;
            // Captured after curType's word is fully set (psize/bits/pk),
            // not right after setRep(): typ121z is compared word-for-word
            // (TPtr::operator==) against each field's uptype() in
            // inSymbol's lookField case, so a premature snapshot here
            // would silently defeat every "already a field of this
            // record" duplicate check.
            typ121z = curType;
            cases.size = 0;
            cases.count = 0;
            inSymbol();
            parseRecordDecl(curType, true, typeIsUnion);
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
        curType = makeArrayType(ranges[curDim].aleft, ranges[curDim].aright,
                                curType, isPacked and (curDim == 1));
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
    formAndAlign(getHelperProc(35)); /*"FOPEN"*/
} /* fopenFile */

void parseDecls(int64_t l3arg1z)
{
    int64_t l3int1z;
    Word frame;
    bool l3var3z;

    IdentRecPtr &procName = programme::super.back()->procName;
    IdentRecPtr &curIdRec = programme::super.back()->curIdRec;
    int64_t &l2var12z = programme::super.back()->l2var12z;
    int64_t &fileExit = programme::super.back()->fileExit;

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
        printf(" IN LINE %ld", (long)curIdRec->pck.offset);
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
                      getHelperProc(55 /*"P/NN"*/) - 010000000);
        if (1 < l3arg1z) {
            frame.ii = getValueOrAllocSymtab(-(frame.ii+l3arg1z));
        }
        if (has(optSflags.ii, S5) and
            curProcNesting == 1)
            l3int1z = 34;  /* P/LV */
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
        if (curProcNesting == 1) {
            if (inputFile != NULL)
                fopenFile(inputFile, fileForInput);
            if (outputFile != NULL)
                fopenFile(outputFile, fileForOutput);
            curVal.ii = fileExit;
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
                          getHelperProc(14 /*"P/GD"*/));
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
void parseCallArgs(IdentRecPtr subroutine, ExprPtr callee);

/* parsePostfix: consume any chain of postfix operators (@, .field, [idx],
   (args)) acting on curExpr.  Returns with SY pointing at the first token that is
   neither a postfix operator nor the trailing `]` of an index list.  Safe
   to call when the next token isn't a postfix at all (loop simply exits). */
void parsePostfix()
{
    ExprPtr l4exp1z;
    TPtr l4typ3z, l4typ5z;
    Kind l4var4z;
    int64_t l4step6z;
L13462:
    l4typ3z = curExpr->vt.typ;
    l4var4z = (Kind)l4typ3z.p.pk;
    if (SY == ARROW) {
        /* '->' is deref + struct field selection; build DEREF here,
           then jump to label 55 to consume the field IDENT.  The pointee
           goes through l4typ5z because it is needed again below, for the
           DEREF node and for l4typ3z (mirrors work.p2c). */
        l4exp1z = new Expr;
        l4exp1z->expr1 = curExpr;
        l4typ5z = l4var4z == kindPtr ? ptrBase(l4typ3z) : l4typ3z;
        if (l4var4z == kindPtr and
            l4typ5z.p.pk == kindStruct) {
            // Through a register pointer the record is already pinned: stand
            // its `with` entry in for the deref, and the field access costs
            // the one instruction it costs under `with`.
            if (curExpr->op == GETVAR and curExpr->id1->pck.cl == REGID)
                curExpr = reinterpret_cast<ExprPtr>(curExpr->id1->value());
            else {
                l4exp1z->vt.typ = l4typ5z;
                l4exp1z->op = DEREF;
                curExpr = l4exp1z;
            }
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
        l4step6z = eltStep(l4typ3z);
        if (isCharPtr(l4typ3z))
            curExpr = flatMemAt(mkExpr(INTPLUS, charPtrType,
                                       l4exp1z, curExpr));
        else if (l4step6z != 0) {
            /* p[i] is *(p + i), an lvalue like any other DEREF. */
            curExpr = mkExpr(DEREF, ptrBase(l4typ3z),
                             mkExpr(INTPLUS, l4typ3z, l4exp1z,
                                    scaleIdx(curExpr, l4step6z)), NULL);
        } else if (l4typ3z.p.pk != kindArray) {
            error(errWrongVarTypeBefore);
        } else {
            l4exp1z = mkExpr(GETELT, l4typ3z.rep()->base,
                             l4exp1z, curExpr);
            curExpr = l4exp1z;
        }
        if (SY != RBRACK)
            error(67 /*errNeedBracketAfterIndices*/);
        inSymbol();
    } else if (SY == LPAREN and isRoutinePtr(l4typ3z)) {
        /* A call through a pointer to a routine.  Unary '*' leaves such a
           pointer alone, so 'f(x)' and '(*f)(x)' arrive here alike. */
        parseCallArgs(NULL, curExpr);
    } else return;
    goto L13462;
} /* parsePostfix */

void parseLval()
{
    curExpr = mkExpr(GETVAR, hashTravPtr->typ,
                     (ExprPtr)hashTravPtr, NULL);
    inSymbol();
    parsePostfix();
    // A register pointer that no '->' consumed stands for its own value, the
    // address of the record pinned in the register.  Taking that address of
    // the `with` entry also re-derives it when the register was demoted.  The
    // result is not an lvalue, so assigning to such a pointer is refused by
    // the ordinary lvalue check.
    if (curExpr->op == GETVAR and curExpr->id1->pck.cl == REGID) {
        TPtr regTyp = curExpr->vt.typ;
        curExpr = mkRef(reinterpret_cast<ExprPtr>(curExpr->id1->value()));
        curExpr->vt.typ = regTyp;
    }
} /* parseLval */

void castToReal(ExprPtr & value)
{
    /* Fold at construction when the operand is already constant, so a mixed
       int/real constant expression collapses to a single GETENUM node (the
       one case bldArithOp routes through here). */
    value = mkUnaryFold(TOREAL, RealType, value);
} /* castToReal */

/* C's assignment conversions between integer and real, applied wherever a
   value is assigned to a destination of the other type: plain assignment,
   an actual passed by value to a formal, and return.  Widening goes through
   castToReal; narrowing drops the fraction.  Returns false if the mismatch
   is not one of those,
   leaving the caller to report it. */
bool castArith(TPtr dest, ExprPtr & value)
{
    if (dest == RealType and typeCheck(IntegerType, value->vt.typ)) {
        castToReal(value);
        return true;
    }
    if (value->vt.typ != RealType or not typeCheck(IntegerType, dest))
        return false;
    /* mkUnaryFold folds a constant real in place, the way it does for
       castToReal's TOREAL. */
    value = mkUnaryFold(TOINT, IntegerType, value);
    return true;
} /* castArith */

bool areTypesCompatible(ExprPtr & other)
{
    if (arg1Type == RealType) {
        if (typeCheck(IntegerType, arg2Type)) {
            castToReal(curExpr);
            return true;
        }
    } else if (arg2Type == RealType and
               typeCheck(IntegerType, arg1Type)) {
        castToReal(other);
        return true;
    }
    return false;
} /* areTypesCompatible */

/* The arguments of a call, matched against the callee's formals.  A direct
   call names its callee: subroutine is its record and the formals are the
   identrec chain hanging off argList, terminated by the record itself.  A
   call through a pointer (callee != NULL, a value of pointer-to-routine
   type) has only the type to go by: the formals are the sigrec chain it
   carries, and so is the whole calling convention. */
void parseCallArgs(IdentRecPtr subroutine, ExprPtr callee)
{
    bool noArgs, tooMany;
    ExprPtr curActual, callExpr, argList;
    IdentRecPtr curFormal = NULL;
    SigPtr curSig = NULL;
    TPtr routTyp{}, formType{};

    if (callee == NULL) {
        if (subroutine->typ != voidType)
            liveRegs = liveRegs & ~ subroutine->flags();
        noArgs = not has(subroutine->flags(), 24);
    } else {
        routTyp = ptrBase(callee->vt.typ);
        noArgs = not has(routTyp.rep()->rflags, 24);
    }
    callExpr = new Expr;
    argList = callExpr;
    bool48z = true;
    if (callee == NULL) {
        callExpr->vt.typ = subroutine->typ;
        callExpr->op = ALNUM;
        callExpr->id2 = subroutine;
    } else {
        callExpr->vt.typ = routTyp.rep()->rresult;
        callExpr->op = INDCALL;
        callExpr->expr2 = callee;
    }
    callExpr->expr1 = NULL;
    if (SY == LPAREN) {
        if (noArgs) {
            if (callee == NULL)
                curFormal = subroutine->argList();
            else
                curSig = routTyp.rep()->rparams;
            if (curFormal == NULL and curSig == NULL) {
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
            if (noArgs) {
                if (callee == NULL) {
                    tooMany = subroutine == curFormal;
                    formType = curFormal->typ;
                } else {
                    tooMany = curSig == NULL;
                    if (not tooMany)
                        formType = curSig->ptyp;
                }
                if (tooMany) {
                    error(errTooManyArguments);
                    throw 8888;
                }
            }
            expression();
            if (noArgs) {
                arg1Type = curExpr->vt.typ;
                if (arg1Type != voidType) {
                    /* Every formal is taken by value, so an actual converts
                       the way an assignment to it would. */
                    if (not typeCheck(arg1Type, formType) and
                        not castArith(formType, curExpr))
                        error(40); /*errIncompatibleArgumentTypes*/
                }
            }
            curActual = new Expr;
            curActual->vt.typ.setRep(NULL);
            curActual->expr1 = NULL;
            curActual->expr2 = curExpr;
            argList->expr1 = curActual;
            argList = curActual;
            if (noArgs) {
                if (callee == NULL)
                    curFormal = curFormal->list();
                else
                    curSig = curSig->next;
            }
        } while (SY == COMMA);
        if ((SY != RPAREN) or
            (noArgs and (callee == NULL ? curFormal != subroutine
                                        : curSig != NULL)))
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
    curExpr = mkExprFold(oper, arg2Type, leftArg, curExpr);
} /* bldBitOp */

void bldArithOp(Operator oper, ExprPtr leftExpr, [[maybe_unused]] bool match)
{
    Kind k1, k2;
    Operator resOp;
    TPtr resTyp;
    int64_t lstep, rstep;

    k1 = (Kind)arg1Type.p.pk;
    k2 = (Kind)arg2Type.p.pk;
    /* A char pointer is a byte index into flat memory: the byte is already
       its unit, so a count applies as it stands, and either operand may be
       the pointer.  The operands stay in source order, which an op-assign
       depends on -- it drops this node's expr1 and takes expr2 as the
       right-hand side. */
    if ((isCharPtr(arg1Type) and typeCheck(IntegerType, arg2Type)) or
        (isCharPtr(arg2Type) and typeCheck(IntegerType, arg1Type))) {
        curExpr = mkExpr(intOpMap[oper], charPtrType, leftExpr, curExpr);
        return;
    }
    /* Pointer arithmetic steps in pointee units: the integer operand is
       scaled to words here, so codegen sees an ordinary integer add.  Any
       combination not taken apart below -- a scaling operator, 'int - ptr',
       a sum of two pointers -- falls into the rejection that follows. */
    lstep = eltStep(arg1Type);
    rstep = eltStep(arg2Type);
    if (lstep != 0 and rstep != 0) {
        if (oper == MINUSOP and typeCheck(arg1Type, arg2Type)) {
            curExpr = mkExpr(INTMINUS, IntegerType, leftExpr, curExpr);
            if (lstep != 1)
                curExpr = mkExprFold(IDIVOP, IntegerType, curExpr,
                                     mkIntLit(lstep));
            return;
        }
    } else if (lstep != 0) {
        if ((oper == PLUSOP or oper == MINUSOP) and
            typeCheck(IntegerType, arg2Type)) {
            curExpr = mkExpr(intOpMap[oper], arg1Type,
                             leftExpr, scaleIdx(curExpr, lstep));
            return;
        }
    } else if (rstep != 0 and oper == PLUSOP and
               typeCheck(IntegerType, arg1Type)) {
        curExpr = mkExpr(INTPLUS, arg2Type,
                         scaleIdx(leftExpr, rstep), curExpr);
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
            castToReal(leftExpr);
        if (k2 != kindReal)
            castToReal(curExpr);
        resOp = oper;
        resTyp = RealType;
    } else {
        resOp = intOpMap[oper];
        resTyp = IntegerType;
    }
    curExpr = mkExprFold(resOp, resTyp, leftExpr, curExpr);
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
            ((arg2Type != IntegerType) or
             (arg1Type.p.pk != kindScalar) or
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
        curExpr = mkExprFold(oper, BooleanType, leftExpr, curExpr);
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
    Word l4var3z, l4var4z;
    ExprPtr l4exp5z, newExpr, l4var7z, l4var8z;
    IdentRecPtr routine;
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
            l5var2z = symType;
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
                    resultValue = hashTravPtr->pck.offset;
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
              (subset(asint64_t, Bits(fnABS, fnREF))))
          or ((checkMode == chkINT) and
              (subset(asint64_t, (Bits(fnABS,fnMALLOC,fnREF,fnCARD) |
                           Bits(fnMINEL)))))
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
    if (SY == TYPESY) {
        l4typ11z = symType;
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
                switch (hashTravPtr->pck.cl) {
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
                    if (routine->pck.offset == 0) {
                        if (routine->typ != voidType and
                            SY == LPAREN) {
                            stdCall();
                        } else {
                            error(44); /* errIncorrectUsageOfStandProcOrFunc */
                            curExpr = uVarPtr;
                        }
                    } else if (SY == LPAREN) {
                        parseCallArgs(routine, NULL);
                    } else {
                        /* A routine named without an argument list is a
                           function designator, and is worth its entry
                           address; a '&' in front of it is the identity,
                           as in C. */
                        if (routine->pck.offset != frameRegTemplate) {
                            /* A nested routine needs a display an indirect
                               call has no way to set up. */
                            error(81); /* errProcNestingTooDeep */
                            curExpr = uVarPtr;
                        } else {
                            /* The routine's type is built here, on demand,
                               and not kept in its record: a stored one would
                               cost every routine a heap record, and work.p2c
                               has barely any heap to spare when compiling
                               itself. */
                            curExpr = mkExpr(PROCADDR,
                                             getPtrType(makeRoutineType(routine)),
                                             NULL, (ExprPtr)routine);
                        }
                    }
                } break;
                case VARID: case FORMALID: case REGID:
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
        if (not typeCheck(curExpr->vt.typ, IntegerType) and
            eltStep(curExpr->vt.typ) == 0)
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
                curExpr = mkUnaryFold(RNEGOP, RealType, curExpr);
            else if (typeCheck(arg1Type, IntegerType))
                curExpr = mkUnaryFold(INEGOP, IntegerType, curExpr);
            else {
                error(69); /* errUnaryMinusNeedRealOrInteger */
                return;
            }
        } break;
        case BITNEGOP: {
            if (typeCheck(arg1Type, IntegerType))
                curExpr = mkUnaryFold(BITNEGOP, IntegerType, curExpr);
            else {
                error(62); /* errIntegerNeeded */
                return;
            }
        } break;
        case NOTOP: {
            if (arg1Type == BooleanType)
                curExpr = mkUnaryFold(NOTOP, BooleanType, curExpr);
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
            else if (arg1Type.p.pk == kindPtr) {
                /* Dereferencing a pointer to a routine yields the routine,
                   which is worth its address again: leave the pointer as it
                   is, so '(*f)(x)' is the same expression as 'f(x)'. */
                if (not isRoutinePtr(arg1Type))
                    curExpr = mkExpr(DEREF, ptrBase(arg1Type),
                                     curExpr, NULL);
            } else {
                stmtName = "unary*";
                error(errWrongVarTypeBefore);
            }
        } break;
        case SETAND: {
            if (curExpr->op == PROCADDR)
                break;  /* a function designator is already its address */
            if (not has(lvalOpSet, curExpr->op))
                error(27); /* errExpressionWhereVariableExpected */
            if (curExpr->op == GETELT and
                curExpr->expr1->op == GETVAR and
                curExpr->expr1->id1 == flatMemVar) {
                curExpr = curExpr->expr2;
                curExpr->vt.typ = charPtrType;
            } else if (curExpr->op == GETELT and
                       isCharArray(curExpr->expr1->vt.typ) and
                       curExpr->expr1->vt.typ.rep()->pck) {
                /* A packed char array holds six 8-bit bytes to a word, so an
                   element's byte index is the array's word address times six
                   plus the index, counted from the array's lower bound. */
                ExprPtr idxExpr = curExpr->expr2;
                if (curExpr->expr1->vt.typ.rep()->aleft != 0)
                    idxExpr = mkExprFold(INTMINUS, IntegerType, idxExpr,
                                 mkIntLit(curExpr->expr1->vt.typ.rep()->aleft));
                curExpr = mkExpr(INTPLUS, charPtrType,
                    mkExpr(IMULOP, IntegerType,
                           mkCastInt(mkRef(curExpr->expr1)),
                           mkIntLit(6)),
                    idxExpr);
            } else if (arg1Type == CharType)
                /* An unpacked char array's element has a word to itself, and
                   a char value sits in its rightmost byte. */
                curExpr = mkExpr(INTPLUS, charPtrType,
                    mkExpr(IMULOP, IntegerType,
                           mkCastInt(mkRef(curExpr)), mkIntLit(6)),
                    mkIntLit(5));
            else {
                /* The address of an lvalue is a pointer to its type. */
                curExpr = mkRef(curExpr);
                curExpr->vt.typ = getPtrType(arg1Type);
            }
        } break;
        case INCROP: case DECROP: {
            if (not has(lvalOpSet, curExpr->op)) {
                error(27);
                return;
            }
            if (not typeCheck(arg1Type, IntegerType) and
                eltStep(arg1Type) == 0) {
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
                if (castArith(arg1Type, curExpr)) {
                    /* int <-> real, converted as C does */
                } else if (isCharPtr(arg1Type) and
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
            arg1Type = leftExpr->vt.typ;
            arg2Type = curExpr->vt.typ;
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


// 'register TYPE *name = expr;' at the head of a block: name is a pointer
// whose value is pinned in an index register until the block ends, so that
// 'name->field' costs the one instruction a `with` field costs.  It is that
// same machinery: SETREG on a DEREF of the initializer allocates the
// register, spills it when the address is dear to recompute, and pushes the
// entry genEntry reloads after a call that clobbers it.  The declared name
// merely gives the entry something to be reached by, in place of `with`'s
// implicit field lookup.
//
// Returns the registers claimed, for the caller to fold into usedRegs; the
// records themselves come back on the chain headed by `decls`, to be
// unlinked from the symbol table when the block closes.
int64_t registerDecls(IdentRecPtr & decls)
{
    int64_t claimed = Bits();
    while (SY == IDENT and curIdent == litRegister) {
        inSymbol();
        TPtr regType{};
        bool packedFlag, forwardRef;
        {
            parseTypeRef regTypeParser(regType, skipToSet | Bits(IDENT, SEMICOLON));
            packedFlag = regTypeParser.isPacked;
            forwardRef = regTypeParser.isForwardRef;
        }
        Declarator d = parseOneDeclarator(regType, packedFlag, forwardRef);
        // The pointee goes through regBase because it is needed again
        // below, for the DEREF node.
        TPtr regBase{};
        bool regOk = d.type.p.pk == kindPtr;
        if (regOk) {
            regBase = ptrBase(d.type);
            regOk = regBase.p.pk == kindStruct;
        }
        if (not regOk) {
            error(71); /* errRegPtrNotToStruct */
            d.type = voidPtr;
            regBase = voidType;
        }
        checkSymAndRead(BECOMES);
        // SY already sits on the initializer's first token.
        readNext = false;
        expression();
        if (not typeCheck(d.type, curExpr->vt.typ))
            error(33); /* errIllegalTypesForAssignment */
        // Exactly what `with *e do` builds, so the register allocation,
        // spilling and demotion behaviour is the existing one.
        curExpr = mkExpr(DEREF, regBase, curExpr, NULL);
        (void) formOperator(SETREG);
        claimed = (claimed | Bits(curVal.ii)) & auxRegs;
        IdentRecPtr regIdRec =
            besm6_alloc_record<IdentRec>(offsetof(IdentRec, szIdent));
        regIdRec->id = d.name;
        regIdRec->pck.offset = curFrameRegTemplate;
        regIdRec->pck.cl = REGID;
        regIdRec->typ = d.type;
        // the pinList entry this name stands for
        regIdRec->value() = reinterpret_cast<int64_t>(pinList);
        // chain of this block's own names, for the unlinking by the caller
        regIdRec->list() = decls;
        decls = regIdRec;
        addToHashTab(regIdRec);
        checkSymAndRead(SEMICOLON);
    }
    return claimed;
} /* registerDecls */

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
    int64_t nClauses;
    Word expected;
    int64_t decoder, endOfStmt;
    Word minValue, maxValue;

    parentExpression();
    exprtype = curExpr->vt.typ;
    otherSeen = false;
    if (exprtype.p.pk == kindScalar)
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
                if (SY != CASESY) {
                    requiredSymErr(CASESY);
                    // No CASESY was consumed, so the label's own first token
                    // is the current SY.  readNext := false keeps it for
                    // expression(); skipping it instead derails the arm and
                    // leaves the next CASESY/DEFAULTSY in statement context,
                    // where the routine-body loop spins on a symbol
                    // Statement() will not consume.
                    readNext = false;
                }
                expression();
                takeConstFromExpr();
                itemvalue = curVal;
                itemtype = curExpr->vt.typ;
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
        nClauses = 0;
        while (curClause != NULL) {
            if (expected != curClause->value or
                exprtype.p.pk != kindScalar)
                goto L16140;        /* a gap in the labels: compare them */
            maxValue = expected;
            if (isIntCase) {
                expected.ii = expected.ii + 1;
            } else {
                expected.ii = expected.ii + 1; // raw ordinal, no exponent
            }
            ++nClauses;
            curClause = curClause->next;
        } /* while 16142 */
        /* An indexed jump carries a ten-instruction prologue whatever the
           label count is, so it earns that back from caseTabMin clauses up.
           Below it, comparing the labels one by one is shorter and needs no
           range check. */
        if (nClauses < caseTabMin)
            goto L16140;
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
        goto L16211;
L16140:
        itemSpan = 34000;
        fixup(0, decoder);
        if (firstType.p.pk == kindScalar)
            itemSpan = firstType.rep()->numen;
        itemsEnded = itemSpan < 32000;
        if (itemsEnded)
            form1Insn(KATI+14);
        /* Both chains walk the labels by the difference from the one before,
           so each step needs a single instruction to reach the next label:
           an index register counts down through KUTM, and the accumulator
           holds the subject XOR-ed with the label reached so far, AEX being
           its own inverse. */
        minValue.ii = (minValue.ii - minValue.ii); /* WTF? */
        while (allClauses != NULL) {
            if (itemsEnded) {
                curVal.ii = (minValue.ii - allClauses->value.ii);
                /* KVZM reads the index register, so a step of zero -- a
                   first label of zero -- needs no instruction of its own.
                   The accumulator chain below keeps its KAEX even then: it
                   is what leaves omega logical, which an arithmetic subject
                   expression may not have done. */
                if (curVal.ii != 0)
                    form1Insn(getValueOrAllocSymtab(curVal.ii) +
                              (KUTM+I14));
                form1Insn(KVZM+I14 + allClauses->offset);
            } else {
                curVal.ii = (minValue.ii ^ allClauses->value.ii);
                form2Insn(KAEX + I8 + getFCSToffset(),
                          InsnTemp[UZA] + allClauses->offset);
            }
            minValue = allClauses->value;
            allClauses = allClauses->next;
        }
        if (otherSeen)
            form1Insn(InsnTemp[UJ] + otherOffset);
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

// ---- Declaration-site initializers (C-style) -------------------------------
// A global variable/array may carry a load-time initializer at its declaration
// ('int x = 5;', 'int a[N] = { v:count, [i]=w, ... };').  The FCST data-init
// region must be a contiguous trailing block (finalize() describes it only by
// length + record count, right after the constant pool), so each initializer is
// BUFFERED at its declaration and materialized once, at program end, by
// flushInitializers().

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

int64_t allocDataRef(int64_t arg) {
    if (arg >= 2048) {
        curVal.ii = arg;
        return allocSymtab((curVal.ii | 040000000) & halfWord);
    } else {
        return arg;
    }
} /* allocDataRef */

struct InitItem { int64_t value, count; };
struct InitSeg  { int64_t base; std::vector<InitItem> items; };
std::vector<InitSeg> initSegs;

// Start a new destination segment: 'var' bare (offset 0), or -- when
// 'designator' -- 'var[index]...' (SY is at '['; parsePostfix builds the GETELT
// chain and leaves SY at '=').  formOperator(SETREG9) yields the base-register
// template with no module/FCST side effect (it builds only into the object
// buffer, guarded by objBufIdx==1).
void beginInitSeg(IdentRecPtr var, bool designator) {
    curExpr = mkExpr(GETVAR, var->typ, (ExprPtr)var, NULL);
    if (designator)
        parsePostfix();
    putLeft = true;
    objBufIdx = 1;
    (void) formOperator(SETREG9);
    if (objBufIdx != 1)
        error(errVarTooComplex);
    initSegs.push_back(InitSeg{ leftInsn & 0777700000000L, {} });
}

// Parse (but do not emit) one global's '= initializer', buffering it into
// initSegs.  A '[index]=' designator opens a new segment; bare items and
// 'value:count' fills accumulate into the current segment.
void parseInitializer(IdentRecPtr var) {
    ExprPtr boundary;
    inSymbol();                       /* consume '=' -> SY at first init token */
    bool braced = SY == BEGINSY;
    if (braced)
        inSymbol();                   /* consume '{' */
    setup(boundary);
    beginInitSeg(var, false);         /* initial segment: var, offset 0 */
    for (;;) {
        if (braced and SY == LBRACK) {
            /* '[index]=' designator opens a new segment (parsePostfix's
               expression() consumes the '[' -- it needs readNext=true). */
            myrollup(boundary);
            setup(boundary);
            readNext = true;
            beginInitSeg(var, true);
            checkSymAndRead(BECOMES);
        }
        readNext = false;             /* SY already at the value's first token */
        expression();
        takeConstFromExpr();
        int64_t v = curVal.ii;
        int64_t count = 1;
        if (SY == COLON) {
            inSymbol();
            if (SY != INTCONST) {
                error(62);            /* errIntegerNeeded */
                count = 0;
            } else {
                count = curToken.ii;
                inSymbol();
            }
        }
        initSegs.back().items.push_back(InitItem{ v, count });
        /* Only a braced initializer uses ',' to separate items; an unbraced
           scalar ends here so the declarator loop can read the next name. */
        if (braced and SY == COMMA) {
            inSymbol();
            continue;
        }
        break;
    }
    myrollup(boundary);
    if (braced)
        checkSymAndRead(ENDSY);
    /* the trailing ';' is consumed by the declarator loop */
}

// Materialize all buffered declaration-site initializers as the contiguous
// trailing data-init region of FCST.  Called at program end, after the whole
// constant pool has been laid down.
void flushInitializers() {
    if (initSegs.empty()) {
        lookup2 = 0;
        lookupMode = lookDef;
        return;
    }
    int64_t dsize = FcstCnt;
    int64_t setcount = 0;
    int64_t dataLoc = FcstCnt, length = 0, dataSegLen = 0, base = 0;
    Word savedVal;
    std::vector<DATAREC> F;
    auto putDataRec = [&](int64_t rep) {
        DATAREC r;
        r.assn(0, allocDataRef(length));
        if (FcstCnt == dataLoc) {
            curVal = savedVal;
            curVal.ii = addCurValToFCST();
        } else {
            curVal.ii = dataLoc;
        }
        r.assn(1, allocSymtab(0400100000000L | (curVal.ii & halfWord)));
        r.assn(2, allocDataRef(rep));
        if (dataSegLen == 0) {
            curVal.ii = shr48(base, 24);
        } else {
            curVal.ii = allocSymtab(base | (dataSegLen & halfWord));
        }
        r.assn(3, curVal.ii);
        dataSegLen = rep * length + dataSegLen;
        F.push_back(r);
        setcount = setcount + 1;
        length = 0;
        dataLoc = FcstCnt;
    };
    for (size_t s = 0; s < initSegs.size(); ++s) {
        base = initSegs[s].base;
        dataLoc = FcstCnt;
        length = 0;
        dataSegLen = 0;
        std::vector<InitItem> &items = initSegs[s].items;
        for (size_t i = 0; i < items.size(); ++i) {
            savedVal.ii = items[i].value;
            int64_t count = items[i].count;
            bool hasNext = i + 1 < items.size();
            if (count != 1) {
                if (length != 0)
                    putDataRec(1);
                length = 1;
                putDataRec(count);
            } else {
                length = length + 1;
                if (hasNext) {
                    curVal = savedVal;
                    toFCST();
                } else {
                    if (length != 1) {
                        curVal = savedVal;
                        toFCST();
                    }
                    putDataRec(1);
                }
            }
        }
    }
    for (size_t s = 0; s < F.size(); ++s)
        FCST.push_back(F[s].b);
    lookup2 = FcstCnt - dsize;
    FcstCnt = dsize;
    lookupMode = setcount;
} /* flushInitializers */


void parseConstExpression()
{
    TPtr &ceTyp = programme::super.back()->ceTyp;
    Word &ceVal = programme::super.back()->ceVal;
    ExprPtr &boundary = Statement::super.back()->boundary;

    readNext = false;
    ceTyp = voidType;
    ceVal.ii = 1;
    expression();
    takeConstFromExpr();
    ceTyp = curExpr->vt.typ;
    ceVal = curVal;
    myrollup(boundary);
} /* parseConstExpression */

void returnOp() {
    IdentRecPtr &procName = programme::super.back()->procName;
    bool &retSeen = programme::super.back()->retSeen;

    if (not has(statEndSys, SY)) {
        /* return expr: load expr to ACC, then jump */
        if (procName->typ == voidType)
            error(errNeedOtherTypesOfOperands);
        else {
            retSeen = true;
            readNext = false;
            expression();
            if (typeCheck(procName->typ, curExpr->vt.typ)) {
                /* OK */
            } else if (castArith(procName->typ, curExpr)) {
                /* int <-> real, converted as C does */
            } else
                error(33); /* errIllegalTypesForAssignment */
            (void) formOperator(LOAD);
        }
    } else if (procName->typ != voidType)
        error(errNeedOtherTypesOfOperands);
    form1Insn(getHelperProc(15) + (KUJ-KVJM-I13));
} /* returnOp */

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
                workExpr->vt.typ = outputFile->typ;
                workExpr->op = GETVAR;
                workExpr->id1 = outputFile;
            }
            needR12 = true;
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
        helperNo = 20;                   /* C/WI */
        if (l4typ3z == IntegerType or l4typ3z == BooleanType)
            defWidth = 10;
        else if (l4typ3z == RealType) {
            helperNo = 21;               /* P/WR */
            defWidth = 14;
        } else if (l4typ3z == CharType) {
            helperNo = 22;               /* P/WC */
            defWidth = 1;
        } else if (curVarKind == kindScalar
                   and l4typ3z.rep()->start != -1) {
            helperNo = 25;               /* P/WX */
            dumpEnumNames(l4typ3z);
            defWidth = 8;
        } else if (curVarKind == kindScalar
                   and l4typ3z.rep()->enums != NULL) {
            // Explicit-value enum (start == -1): name printing suppressed,
            // so print the value as a decimal integer, exactly like int.
            helperNo = 20;               /* C/WI */
            defWidth = 10;
        } else if (isCharArray(l4typ3z)) {
            defWidth = l4typ3z.rep()->aright - l4typ3z.rep()->aleft + 1;
            if (not l4typ3z.rep()->pck)
                helperNo = 49;            /* P/WA */
            else if (6 >= defWidth)
                helperNo = 23;            /* P/A6 */
            else
                helperNo = 24;           /* P/A7 */
        } else if (typeSize(l4typ3z) == 1) {
            helperNo = 26;               /* P/WO */
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
                    if (helperNo != 21)    /* P/WR */
                        error(35); /* errSecondSpecifierForWriteOnlyForReal */
                } else if (curToken.ii == litOct) {
                    helperNo = 26; /* P/WO */
                    defWidth = 17;
                    if (typeSize(l4typ3z) != 1)
                        error(34); /* errTypeIsNotAFileElementType */
                    inSymbol();
                }
                noWidth = false;
                if (firstWidth == NULL and
                    has(BitRange(22,24), helperNo)) {  /* WC,A6,A7 */
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
                if (helperNo == 21) {       /* P/WR */
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
                    if (helperNo == 29)     /* P/7A */
                        opToForm = SETREG11;
                    else
                        opToForm = LOAD;
                } else {
                    if (helperNo == 24 or       /* P/A7 */
                        helperNo == 49)     /* P/WA */
                        opToForm = PUSHSET11;
                    else
                        opToForm = FRACWIDTH;
                }
                (void) formOperator(opToForm);
                if (has(Bits(23,24,28,29), helperNo) or /* A6,A7,6A,7A */
                    helperNo == 49)
                    form1Insn(KVTM+I10 + defWidth);
                else {
                    if (helperNo == 25) /* P/WX */
                        form1Insn(KVTM+I11 + l4typ3z.rep()->start);
                }
                callHelperWithArg();
            }
        } while (SY == COMMA);
        if (procNo == 8) {
            helperNo = 30;                 /* P/WL */
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
        curExpr->vt.ii = procNo + 41;   /* the P/PK / P/KC helper number */
        curExpr->expr1 = l4exp7z;
        curExpr->expr2 = l4exp6z;
        (void) formOperator(PCKUNPCK);
    } /* doPackUnpack */

    standProc() { /* standProc */
        IdentRecPtr &l3idr12z = Statement::super.back()->l3idr12z;
        TPtr &l2typ13z = programme::super.back()->l2typ13z;
        int64_t &ii = programme::super.back()->ii;

        curVal.ii = l3idr12z->low();
        procNo = curVal.ii;
        l4bool10z = (SY == LPAREN);
        oldOffset = moduleOffset;
        if (not l4bool10z and
            has((BitRange(0,4) | Bits(6,7)), procNo))
            error(45); /* errNoOpenParenForStandProc */
        if (procNo == 4) {
            expression();
            if (not has(lvalOpSet, curExpr->op)) {
                error(27); /* errExpressionWhereVariableExpected */
            }
            arg1Type = curExpr->vt.typ;
            curVarKind = (Kind)(arg1Type.p.pk);
        }
        if (has(Bits(4,5), procNo))
            jumpTarget = getHelperProc(14 + procNo); /* P/DS, P/HT */
        switch (procNo) {
        case 4: { /* free */
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
        case 5: { /* halt */
            formAndAlign(jumpTarget);
            return;
        } break;
        case 7: { /* write */
            writeProc();
        } break;
        case 8: { /* writeln */
            if (SY == LPAREN) {
                writeProc();
            } else {
                formAndAlign(getHelperProc(31)); /*"P/WOLN"*/
                return;
            }
        } break;
        case 6: { /* besm */
            expression();
            takeConstFromExpr();
            formAndAlign(curVal.ii);
        } break;
        case 0: case 1: { /* pck, unpck */
            inSymbol();
            verifyType(CharType);
            checkSymAndRead(COMMA);
            (void) formOperator(SETREG12);
            verifyType(IntegerType);
            if (procNo == 1) {
                (void) formOperator(LOAD);
            }
            formAndAlign(getHelperProc(procNo + 9));
            if (procNo == 0)
                (void) formOperator(STORE);
        } break;
        case 2: { /* pack */
            inSymbol();
            checkArrayArg();
            checkSymAndRead(COMMA);
            verifyType(voidType);
            l4exp6z = curExpr;
            doPackUnpack();
        } break;
        case 3: { /* unpack */
            inSymbol();
            verifyType(voidType);
            l4exp6z = curExpr;
            checkSymAndRead(COMMA);
            checkArrayArg();
            doPackUnpack();
        } break;
        }
        if (has((BitRange(2,4) | Bits(7,8)), procNo))
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
    if (freeRegs == ceRegs) {
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
                    l3var6z = (IdClass)hashTravPtr->pck.cl;
                    if (l3var6z == ROUTINEID) {
                        l3idr12z = hashTravPtr;
                        if (l3idr12z->pck.offset == 0) {
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
                            parseCallArgs(l3idr12z, NULL);
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
                // A block may open with 'register TYPE *p = expr;'
                // declarations; the registers they pin, and the names
                // themselves, last only as far as the closing brace, exactly
                // as a `with`'s do.
                IdentRecPtr blockDecls = NULL, nextDecl, unlinked;
                int64_t & localSize = programme::super.back()->localSize;
                ExprPtr oldWithList = pinList;
                int64_t oldLocalSize = localSize, oldFreeRegs = freeRegs;
                int64_t blockRegs = Bits();
              L_rep:
                inSymbol();
                blockRegs = blockRegs | registerDecls(blockDecls);
              L_skip:
                while (SY != ENDSY and CH != 0)
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
              L_exit_begin:
                if (blockDecls != NULL) {
                    while (blockDecls != NULL) {
                        nextDecl = blockDecls->list();
                        unlinked = NULL;
                        hash(unlinked, blockDecls);
                        blockDecls = nextDecl;
                    }
                    pinList = oldWithList;
                    localSize = oldLocalSize;
                    freeRegs = oldFreeRegs;
                    // The registers this block claimed are clobbered as far
                    // as our callers are concerned: they must reach the
                    // routine's flags mask.
                    usedRegs = usedRegs | blockRegs;
                }
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
            } else if (SY == RETURNSY) {
                inSymbol();
                returnOp();
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
            } else if (has(Bits(TYPEDEFSY, TYPESY, CONSTSY) |
                           Bits(ENUMSY, STRUCTSY, UNIONSY) | Bits(PACKEDSY), SY)) {
                /* A declaration keyword reached statement context -- it leaked
                   here from a malformed routine header (see bad.p2c).  Report
                   and consume it so the enclosing 'while (SY != ENDSY)
                   Statement()' loops make progress instead of spinning.  Only
                   these keywords are caught: other stray tokens (the SEMICOLON
                   of a labelled empty statement, CASESY/DEFAULTSY between switch
                   arms) keep the original silent-return behaviour, so valid
                   code is unaffected. */
                error(errBadSymbol);
                inSymbol();
            }
          exit_ident:;
        } catch (int foo) {
            if (foo != 8888) throw;
            skip(skipToSet | statEndSys);
        }
      L_cleanup:
        if (nest)
            lineNesting = lineNesting - 1;
        myrollup(boundary);
        if (bool110z) {
            bool110z = false;
            skip(skipToSet | statEndSys); /* goto 8888 */
            goto L_cleanup;
        }
    }
    /* 20766 */
} /* Statement */

// Array bounds are const-expressions.  Each bound is evaluated by running
// Statement() in ceRegs mode (parses one expression, const-folds it, and
// leaves the value in ceVal / its type in ceTyp), exactly as
// parseConstDeclValue below drives it.  Forward-declared far above (near
// parseTypeRef); defined here because it needs the Statement definition.
void parseRange(int64_t & aleft, int64_t & aright)
{
    int64_t &ceRegs = programme::super.back()->ceRegs;
    TPtr &ceTyp = programme::super.back()->ceTyp;
    Word &ceVal = programme::super.back()->ceVal;

    freeRegs = ceRegs;
    Statement();
    if (ceTyp != NULL and ceTyp.p.pk == kindScalar) {
        aleft = ceVal.ii;
        if (SY != COLON) {
            // Handle a single value N as a range 0..N-1
            aright = aleft - 1;
            aleft = 0;
            return;
        }
        inSymbol();
        Statement();
        if (ceTyp != NULL and ceTyp.p.pk == kindScalar) {
            aright = ceVal.ii;
            return;
        }
    }
    error(64); /* errIncorrectRangeDefinition */
    aleft = 0;
    aright = 0;
} /* parseRange */

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
    int64_t &sizeCount = programme::super.back()->sizeCount;
    int64_t &jj = programme::super.back()->jj;
    int64_t &localSize = programme::super.back()->localSize;
    bool &done = programme::super.back()->done;
    int64_t &fileExit = programme::super.back()->fileExit;

    objBufIdx = 1;
    objBuffer[objBufIdx] = 0;
    curInsnTemplate = InsnTemp[XTA];
    bool48z = has(procName->flags(), 22);
    if (curProcNesting == 1) {
        fileExit = moduleOffset;
        formFileInit();
    }
    lineStartOffset = moduleOffset;
    l3var1z.ii = moduleOffset;    /* l3var1z := ; (accumulator = moduleOffset) */
    lookup2 = lookUse;
    pinList = NULL;
    arithMode = 1;
    liveRegs = Bits();
    freeRegs = BitRange(curProcNesting+1, 6);
    auxRegs = freeRegs & ~ Bits(minel(freeRegs));
    l3var7z.ii = freeRegs;
    usedRegs = BitRange(1,15) & ~ freeRegs;
    if (curProcNesting != 1)
        parseDecls(2);
    sizeCount = localSize;
    if (not bodyBlock and SY != BEGINSY and CH != 0)
        requiredSymErr(BEGINSY);
    if (has(procName->flags(), 23)) {
        l3idr5z = procName->argList();
        l3int4z = 3;
        if (procName->typ != voidType)
            l3int4z = 4;
        while (l3idr5z != procName) {
            if (l3idr5z->pck.cl == VARID) {
                l3var2z.ii = typeSize(l3idr5z->typ);
                if (l3var2z.ii != 1) {
                    form3Insn(KVTM+I14 + l3int4z,
                              KVTM+I12 + l3var2z.ii,
                              KVTM+I11 + l3idr5z->value());
                    formAndAlign(getHelperProc(45)); /* "P/LNGPAR" */
                }
            }
            l3int4z = l3int4z + 1;
            l3idr5z = l3idr5z->list();
        }
    } /* 21105 */
    l3var2z.ii = lineNesting;
    if (bodyBlock) {
        while (SY != ENDSY and CH != 0)
            Statement();
        if (SY != ENDSY)
            requiredSymErr(ENDSY);
        else
            inSymbol();
    } else if (curProcNesting == 1) {
        // The level 1 block is not written, it is generated: a call of the
        // routine named MAIN, if the program has one.  Anything left over
        // here -- an explicit block above all -- is a bad symbol where a
        // declaration was expected.
        if (CH != 0 or SY != NOSY) {
            error(errBadSymbol);
            skipToEnd();
        }
        bucket = litMain % 65535 % 128;
        l3idr5z = symHash[bucket];
        while (l3idr5z != NULL and l3idr5z->id != litMain)
            l3idr5z = l3idr5z->next();
        if (l3idr5z != NULL and l3idr5z->pck.cl == ROUTINEID
            and l3idr5z->pck.offset != 0) {
            // The call node parseCallArgs would have built for 'main()',
            // with no arguments: op ALNUM, the routine in id2.
            curExpr = mkExpr(ALNUM, l3idr5z->typ, NULL, (ExprPtr) l3idr5z);
            (void) formOperator(DOIT);
        }
    } else if (CH != 0) {
        do {
            Statement();
            done = has(blockBegSys, SY) or (SY == TYPESY) or (CH == 0);
            if (not done)
                errAndSkip(errBadSymbol, skipToSet);
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
    } else {
        jj = curProcNesting == 1 ? 16 /* C/EF */ : 15; /* C/E */
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
            l3var7z.ii = curVal.ii << 24;
            objBuffer[savedObjIdx] |= l3var7z.ii | int64_t(KUTM+SP) << 24;
        }
    } 
    outputObjFile();
} /* defineRoutine */

struct initScalars {
    Word l3var1z, savedIdent;
    int64_t l3var5z, l3var6z;
    IdentRecPtr l3var7z;
    int64_t l3var8z, sysProcNum;
    TPtr temptype;
    Word l3var11z;
    IdentRecPtr &curIdRec;

    void regSysProc(int64_t l4arg1z) {
        curIdRec = besm6_alloc_record<IdentRec>(
            offsetof(IdentRec, szSys));
        // curIdRec@ := [l4arg1z, 0, , temptype, ROUTINEID, sysProcNum];
        curIdRec->id = l4arg1z;
        curIdRec->pck.offset = 0;
        curIdRec->typ = temptype;
        curIdRec->pck.cl = ROUTINEID;
        curIdRec->procno() = sysProcNum;
        addToHashTab(curIdRec);
        sysProcNum = sysProcNum + 1;
    } /* registerSysProc */

    initScalars();
};

initScalars::initScalars() :
    curIdRec(programme::super.back()->curIdRec)
{
    BooleanType.setRep(
        besm6_alloc_record<Types>(offsetof(Types, szScalar)));
    BooleanType.rep()->numen = 2;
    BooleanType.rep()->start = 0;
    BooleanType.rep()->enums = NULL;
    BooleanType.p.psize = 1;
    BooleanType.p.bits = 1;
    BooleanType.p.pk = kindScalar;

    IntegerType.setRep(
        besm6_alloc_record<Types>(offsetof(Types, szScalar)));
    IntegerType.rep()->numen = 100000;
    IntegerType.rep()->start = -1;
    IntegerType.rep()->enums = NULL;
    IntegerType.p.psize = 1;
    IntegerType.p.bits = 48;
    IntegerType.p.pk = kindScalar;

    CharType.setRep(
        besm6_alloc_record<Types>(offsetof(Types, szScalar)));
    CharType.rep()->numen = 256;
    CharType.rep()->start = -1;
    CharType.rep()->enums = NULL;
    CharType.p.psize = 1;
    CharType.p.bits = 8;
    CharType.p.pk = kindScalar;

    /* kindReal and kindVoid carry no payload: no descriptor record. */
    RealType.setRep(NULL);
    RealType.p.psize = 1;
    RealType.p.bits = 48;
    RealType.p.pk = kindReal;

    voidType.setRep(NULL);
    voidType.p.psize = 1;      // sizeof(void) is 1, as in GNU C
    voidType.p.bits = 48;
    voidType.p.pk = kindVoid;

    voidPtr.setRep(besm6_alloc_record<Types>(offsetof(Types, szPtr)));
    voidPtr.rep()->base = voidType;
    voidPtr.p.psize = 1;
    voidPtr.p.bits = 15;
    voidPtr.p.pk = kindPtr;


    charPtrType = getPtrType(CharType);

    flatMemType.setRep(
        besm6_alloc_record<Types>(offsetof(Types, szArray)));
    flatMemType.rep()->base = CharType;
    flatMemType.rep()->pck = true;
    flatMemType.rep()->perword = 6;
    flatMemType.rep()->pcksize = 8;
    flatMemType.rep()->aleft = 0;
    flatMemType.rep()->aright = 32768 * 6 - 1;
    flatMemType.p.psize = 32767;
    flatMemType.p.bits = 48;
    flatMemType.p.pk = kindArray;

    flatMemVar = besm6_alloc_record<IdentRec>(
        offsetof(IdentRec, szIdent));
    flatMemVar->id = 0;
    flatMemVar->pck.offset = 0;
    flatMemVar->typ = flatMemType;
    flatMemVar->pck.cl = VARID;
    flatMemVar->list() = NULL;
    flatMemVar->value() = 0;

    // The predefined type names are reserved words carrying their type,
    // not identifiers: they cannot be shadowed, and they are recognized
    // in every lookup mode.  '_' shares the code of '*', cf. "**PACKED".
    SY = TYPESY;
    symType = IntegerType;  regResWord(0515664L      /*"     INT"*/);
    symType = CharType;     regResWord(043504162L    /*"    CHAR"*/);
    symType = RealType;     regResWord(04654574164L  /*"   FLOAT"*/);
    symType = voidType;     regResWord(066575144L    /*"    VOID"*/);

    curIdRec = besm6_alloc_record<IdentRec>(
        offsetof(IdentRec, szIdent));
    curIdRec->pck.offset = 0;
    curIdRec->typ = IntegerType;
    curIdRec->pck.cl = VARID;
    curIdRec->list() = NULL;
    curIdRec->value() = 7;

    uVarPtr = reinterpret_cast<ExprPtr>(
        besm6_alloc_record<IdentRec>(sizeof(IdentRec)));
    uVarPtr->vt.typ = IntegerType;
    uVarPtr->op = GETVAR;
    uVarPtr->id1 = curIdRec;

    uProcPtr = besm6_alloc_record<IdentRec>(
        offsetof(IdentRec, szRoutine));
    uProcPtr->pck.cl = ROUTINEID;
    uProcPtr->typ.setRep(NULL);
    uProcPtr->list() = NULL;
    uProcPtr->argList() = NULL;
    uProcPtr->preDefLink() = NULL;
    uProcPtr->pos() = 0;

    temptype.setRep(NULL);
    sysProcNum = 0;
    for (l3var5z = 0; l3var5z <= 8; ++l3var5z) {
        if (systemProcNames[l3var5z] != 0)
            regSysProc(systemProcNames[l3var5z]);
        else
            sysProcNum = sysProcNum + 1;
    }
    sysProcNum = 0;
    temptype = RealType;
    regSysProc(0414263L /*"     ABS"*/);
    temptype = IntegerType;
    regSysProc(0635172455746L /*"  SIZEOF"*/);
    regSysProc(05746466345645746L /*"OFFSETOF"*/);
    temptype = voidPtr;
    regSysProc(0554154545743L /*"  MALLOC"*/);
    temptype = IntegerType;
    regSysProc(043416244L /*"    CARD"*/);
    regSysProc(05551564554L /*"   MINEL"*/);

    // The first token of the source is read here, not by the caller: sources
    // usually open with a type keyword (int, void), so the predefined type
    // names must already be registered.  lookupMode is still lookUse.
    inSymbol();

    l3var11z.ii = 30;
    l3var11z.ii = (l3var11z.ii & halfWord) | Bits(24,27,28,29);
    programObj = besm6_alloc_record<IdentRec>(
        offsetof(IdentRec, szRoutine));
    symTabPos = 074004;
    programObj->pck.cl = ROUTINEID; 
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
    objBufIdx = 1;
    lookupMode = lookDef;
    outputObjFile();
    outputFile = NULL;
    inputFile = NULL;
    externFileList = NULL;

    lineStartOffset = moduleOffset;
    l3var5z = 1;
    savedIdent.ii = curIdent;
    curIdent = litOutput;
    defExtern();
    curIdent = litInput;
    defExtern();
    if (!enableStdInput) {
        inputFile = NULL;
        fileForInput = NULL;
    }
    curIdent = savedIdent.ii;
    lookupMode = lookUse;
    l3var6z = 40;
    do {
        programme(l3var6z, programObj, false);
    } while (CH != 0);
    // Emit the data-init region from the declaration-site initializers
    // buffered during parsing.
    flushInitializers();
    readToPos80();
    curVal.ii = l3var6z;
    symTab[074003] = (helperNames[13] | Bits(24,27,28,29)) |
                     (curVal.ii & halfWord);
} /* initScalars */

// C-style 'individual' form: each comma-separated parameter carries its
// own full type-spec ('int a, int *p, char c'), unlike the grouped form
// variables/typedefs/fields use. No ROUTINEID (procedure-valued
// parameter) support -- unexercised by the test corpus; revisit with a
// concrete failing case if one ever turns up.
// matchTo == NULL builds the argument list from scratch, for a routine's
// first declaration.  A non-NULL matchTo walks the records an earlier
// declaration built (terminated by curIdRec, per the argList convention, so
// an empty list arrives as curIdRec itself) while this definition restates
// the list.  Those records are reused, keeping their offsets, the routine's
// multi-word flag and the saved level; the types are checked and the names
// this definition gives are installed.
void parseParameters(IdentRecPtr matchTo)
{
    IdentRecPtr l3var2z;
    int64_t extraWords;
    bool noComma;
    IdentRecPtr &curIdRec = programme::super.back()->curIdRec;
    int64_t &l2int18z = programme::super.back()->l2int18z;

    extraWords = 0;
    // lookup2 (not just lookupMode) must carry lookDef through
    // parseTypeRef's own internal inSymbol() calls -- see the identical
    // note on TYPEDEFSY/parseRecordDecl.
    lookup2 = lookDef;
    lookupMode = lookDef;
    inSymbol();
    l3var2z = NULL;
    if (SY == RPAREN) {
        if (matchTo != NULL and matchTo != curIdRec)
            error(errNoCommaOrParenOrTooFewArgs);
        inSymbol();
        lookup2 = lookUse;
        lookupMode = lookUse;
        return;
    }
    do {
        TPtr paramType{};
        // Scoped exactly as parseGroupedDecls's typeParser is: see its
        // comment for why parseTypeRef::super must be popped back before
        // any registration code runs.
        bool packedFlag;
        {
            parseTypeRef paramTypeParser(paramType, skipToSet | Bits(IDENT, RPAREN, COMMA));
            packedFlag = paramTypeParser.isPacked;
        }
        // Set only here, and only once parseTypeRef is done, so that a
        // struct spelled out in the parameter's own type-spec still holds
        // its fields to the usual "every declarator is named" rule.
        nameOptional = true;
        Declarator d = parseOneDeclarator(paramType, packedFlag);
        nameOptional = false;
        if (matchTo != NULL) {
            if (matchTo == curIdRec)
                error(errTooManyArguments);
            else {
                // Exact identity: typeCheck's assignment compatibility
                // would let a 'char' definition complete an 'int'
                // declaration.
                if (matchTo->typ != d.type)
                    error(40); /* errIncompatibleArgumentTypes */
                // A name given here reaches the symbol table.  Either
                // side may leave the parameter unnamed.
                if (d.name != 0) {
                    if (d.wasDefined)
                        error(errIdentAlreadyDefined);
                    matchTo->id = d.name;
                    addToHashTab(matchTo);
                }
                matchTo = matchTo->list();
            }
        } else {
        IdentRecPtr np = besm6_alloc_record<IdentRec>(
            offsetof(IdentRec, szIdent));
        np->id = d.name;
        np->pck.offset = curFrameRegTemplate;
        np->pck.cl = VARID;
        np->typ = d.type;
        np->list() = curIdRec;
        np->value() = l2int18z;
        // An unnamed parameter takes its argument slot like any other.  It
        // never enters the symbol table, so the body has no way to name it.
        // besm6_alloc_record zero-fills and nidx == 0 reads back as NULL, so
        // the link stays unset.
        if (d.name != 0) {
            if (d.wasDefined)
                error(errIdentAlreadyDefined);
            np->pck.nidx = ord(symHash[d.bucket]);
            symHash[d.bucket] = np;
        }
        l2int18z = l2int18z + 1;
        if (l3var2z == NULL)
            curIdRec->argList() = np;
        else
            l3var2z->list() = np;
        l3var2z = np;
        if (typeSize(d.type) != 1)
            extraWords = extraWords + typeSize(d.type);
        }
        noComma = (SY != COMMA);
        if (not noComma) {
            lookupMode = lookDef;
            inSymbol();
        }
    } while (!noComma);
    /* 22276 */
    if (matchTo != NULL) {
        // The declaration already ran the fix-up below and stashed the
        // resulting offset counter in level, so nothing here may move it.
        if (matchTo != curIdRec)
            error(errNoCommaOrParenOrTooFewArgs);
    } else if (extraWords != 0) {
        curIdRec->flags() = (curIdRec->flags() | Bits(23));
        int64_t base = l2int18z;
        l2int18z = l2int18z + extraWords;
        l3var2z = curIdRec->argList();
        /* 22306 */
        while (l3var2z != curIdRec) {
            if (l3var2z->pck.cl == VARID) {
                int64_t sz = typeSize(l3var2z->typ);
                if (sz != 1) {
                    l3var2z->value() = base;
                    base = base + sz;
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
            workidr = workidr->next();
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
        initScalars();          // reads the first token itself
        return;
    }
    preDefHead = reinterpret_cast<IdentRec*>(ptr(0));
    inTypeDef = false;
    typedefPending = false;
    typelist = NULL;
    retSeen = false;
    bodyStatSys = statBegSys;
    strLabList = NULL;
    lineNesting = lineNesting + 1;
    labFence = numLabTop;
    // Not just TYPESY -- a type-spec can also open with
    // '__packed'/'struct'/'enum' (mirrors parseRecordDecl's own
    // field-group loop condition), so those must be recognized as a
    // declaration start too, or a leading '__packed int x;' would never
    // be seen as one. Declared here (above the do-while, not inside it)
    // so it's visible both in the loop body and in the do-while's own
    // trailing condition below, whose scope excludes the body's locals.
    int64_t declStartSys = Bits(TYPESY, PACKEDSY, STRUCTSY) | Bits(ENUMSY) |
                           Bits(EXTERNSY);
    do {
        if (SY == CONSTSY) {
            parseDecls(0);
            while  (SY == IDENT) {
                if (isDefined)
                    error(errIdentAlreadyDefined);
                /* workidr@ := [curIdent, curFrameRegTemplate, symHash[bucket],
                   , ENUMID, NIL]; */
                workidr = besm6_alloc_record<IdentRec>(
                    offsetof(IdentRec, szIdent));
                workidr->id = curIdent;
                workidr->pck.offset = curFrameRegTemplate;
                workidr->pck.nidx = ord(symHash[bucket]);
                workidr->pck.cl = ENUMID;
                workidr->list() = NULL;
                symHash[bucket] = workidr;
                inSymbol();
                if (charClass != ASSIGNOP)
                    error(errBadSymbol);
                else
                    inSymbol();
                parseConstDeclValue(workidr->typ, workidr->high());
                if (workidr->typ == voidType) {
                    error(errNoConstant);
                    workidr->typ = IntegerType;
                    workidr->value() = 1;
                }
                if (SY == SEMICOLON) {
                    lookupMode = lookDef;
                    inSymbol();
                    // markTypeSym: inSymbol() alone, under lookDef, only
                    // checks the current scope for a match -- a bare type
                    // name from an outer scope (e.g. 'int', predefined at
                    // scope 0) reads back as plain IDENT, not TYPESY.
                    // markTypeSym does its own scope-agnostic hash walk to
                    // fix that up, same as after TYPEDEFSY/routines
                    // below -- needed because a declaration can follow a
                    // const group with nothing but its type-spec to
                    // announce it.
                    markTypeSym();
                    // Besides another const name (IDENT, continuing this
                    // group) or a recovery point, a bare declStartSys
                    // token (the next variable or routine declaration) or
                    // TYPEDEFSY (the next typedef) legitimately
                    // ends the const group -- not an error.
                    if (!has((skipToSet | Bits(IDENT, TYPEDEFSY)) | declStartSys, SY)) {
                        errAndSkip(errBadSymbol, skipToSet | Bits(IDENT));
                    }
                } else {
                    requiredSymErr(SEMICOLON);
                }
            }
        } /* 22511 */
        objBufIdx = 1;
        if (SY == TYPEDEFSY) {
            // C-style: exactly one type-spec + declarator-list per
            // 'typedef' keyword (no Pascal-style continuation without
            // restating it). This keeps a leading TYPESY right after ';'
            // unambiguous evidence of the next nested routine's return
            // type, never another implicit typedef group.
            //
            // Forward-referenced pointer typedefs (mutually-recursive
            // records, e.g. 'typedef expr *eptr;' parsed before 'expr'
            // itself is defined) ARE supported -- parseTypeRef's
            // isForwardRef path leaves a pending placeholder on typelist
            // (see definePtrType); when the real definition for that name
            // comes through here, patch the placeholder's base in place
            // instead of creating an unrelated second IdentRec, exactly
            // the same bookkeeping the '*Name' forward reference needs.
            inTypeDef = true;
            // lookup2 (not just lookupMode) must carry lookDef through
            // parseTypeRef's own internal inSymbol() calls -- every
            // inSymbol() resets lookupMode := lookup2 on exit, so the
            // declarator name a few tokens after 'typedef' is classified
            // under whatever lookup2 holds, not this line's lookupMode.
            lookup2 = lookDef;
            lookupMode = lookDef;
            inSymbol();
            parseGroupedDecls(skipToSet | Bits(IDENT, SEMICOLON),
                [&](Declarator & d) {
                    if (d.wasDefined)
                        error(errIdentAlreadyDefined);
                    IdentRecPtr pending;
                    if (knownInType(pending, d.name)) {
                        TPtr placeholderPtr = pending->typ;
                        placeholderPtr.rep()->base = d.type;
                        pending->typ = d.type;
                        hash(typelist, pending);
                        curIdRec = pending;
                    } else {
                        curIdRec = besm6_alloc_record<IdentRec>(
                            offsetof(IdentRec, szIdent));
                        curIdRec->id = d.name;
                        curIdRec->pck.offset = curFrameRegTemplate;
                        curIdRec->typ = d.type;
                        curIdRec->pck.cl = TYPEID;
                    }
                    // definePtrType parks lineCnt in offset so error 79 can
                    // name the line of an unresolved forward reference.  The
                    // record is a real symbol now, and lookDef stops its walk
                    // at the first entry whose offset is not the frame
                    // template, so anything hashing behind a leftover line
                    // number would be invisible to it.
                    curIdRec->pck.offset = curFrameRegTemplate;
                    curIdRec->pck.nidx = ord(symHash[d.bucket]);
                    symHash[d.bucket] = curIdRec;
                });
            lookup2 = lookUse;
            inTypeDef = false;
        } /* TYPEDEFSY */
        inTypeDef = false;
        curExpr = NULL;
    // The file-init code is emitted once, by defineRoutine's formFileInit call.
    outputObjFile();
    markTypeSym();
    // A leading type-spec starts either a plain variable declarator-list
    // ('TYPE decl, decl;') or a routine ('TYPE name(params) {...}' /
    // 'TYPE name(params);' / 'TYPE name(params) extern;'), exactly as C
    // distinguishes 'int x;' from 'int f(...);'.
    // Disambiguated right after reading the first declarator:
    // a bare name immediately followed by '(' or ':' is a routine. A
    // definition completing an earlier declaration is one too: it restates
    // the whole header, parameter list included ('int add(int a, int b);'
    // ... 'int add(int a, int b) { ... }'). Everything else -- a '*'/'[]'
    // on the declarator, a ',', or a bare ';' -- is a variable.
    //
    while (has(declStartSys, SY)) {
        TPtr baseTy{};
        bool packedFlag, forwardRef;
        externDecl = (SY == EXTERNSY);
        if (externDecl) {
            inSymbol();
            markTypeSym();
            if (not has(Bits(TYPESY, PACKEDSY, STRUCTSY) | Bits(ENUMSY), SY)) {
                error(errBadSymbol);
                skip(skipToSet | Bits(SEMICOLON));
                if (SY == SEMICOLON)
                    inSymbol();
                markTypeSym();
                continue;
            }
        }
        {
            parseTypeRef typeParser(baseTy, skipToSet | Bits(IDENT, MUL, LPAREN, COMMA) | Bits(SEMICOLON));
            packedFlag = typeParser.isPacked;
            forwardRef = typeParser.isForwardRef;
        }
        Declarator d = parseOneDeclarator(baseTy, packedFlag, forwardRef);
        if (d.name == 0) {
            markTypeSym();
            continue;
        }
        // A routine's result is the declarator's type, not the bare type-spec,
        // so 'cell *pick(int)' returns a pointer.  Taken after the declarator
        // for that reason; the redefinition test below compares it against the
        // declaration's return type, which was recorded the same way.
        typedRetType = d.type;
        done = typedRetType == voidType;
        isPredefined = false;
        // A definition restates the whole header, so a redefinition is a
        // name already in the symbol table and followed by its parameter
        // list.  The id check proves foundRec is this name's record: a
        // lookup that found nothing can leave a neighbour there, and a
        // plain 'TYPE name;' would then reach here.
        if (d.ptrOnly and SY == LPAREN and d.foundRec != NULL and
            d.foundRec->id == d.name and
            d.foundRec->pck.cl == ROUTINEID and
            d.foundRec->list() == NULL and
            d.foundRec->preDefLink() != NULL and
            // The whole header is restated, so the return type must agree.
            // A disagreement falls through to the "previous declaration was
            // not a forward declaration" arm below.
            (d.foundRec->typ == typedRetType)) {
            isPredefined = true;
        } else if (d.ptrOnly and SY == LPAREN and d.foundRec != NULL and
                   d.foundRec->id == d.name and
                   // A routine record.  A name may shadow an enum constant
                   // or a variable of an outer scope, and identifiers
                   // collide at 8 characters (standProc the routine and the
                   // STANDPROC operator).
                   d.foundRec->pck.cl == ROUTINEID) {
            error(errIdentAlreadyDefined);
            printErrMsg(82); /* errPrevDeclWasNotForward */
        }
        // A name carrying nothing but '*' ops -- 'name', '*name', '**name'
        // -- is a routine when followed by '(', and a plain variable
        // otherwise. A '[]' or a parenthesized group makes it a variable: an
        // array, or a pointer to a routine. Every routine carries explicit
        // parens, a definition completing an earlier declaration included, so
        // no lookahead past ';' is needed.
        bool isRoutine = d.ptrOnly and SY == LPAREN;
        if (externDecl and isRoutine) {
            error(errBadSymbol);
            skip(skipToSet | Bits(SEMICOLON));
            if (SY == SEMICOLON)
                inSymbol();
            markTypeSym();
            continue;
        }
        if (not isRoutine) {
            /* ---- variable declarator list ---- */
            lookupMode = lookUse;
            bool moreDecls = true;
            while (moreDecls) {
                if (externDecl and curProcNesting == 1) {
                    curIdent = d.name;
                    if (curIdent == litInput or curIdent == litOutput)
                        error(errIdentAlreadyDefined);
                    else
                        defExtern();
                }
                curIdRec = besm6_alloc_record<IdentRec>(
                    offsetof(IdentRec, szIdent));
                curIdRec->id = d.name;
                curIdRec->pck.offset = curFrameRegTemplate;
                curIdRec->pck.nidx = ord(symHash[d.bucket]);
                curIdRec->pck.cl = VARID;
                curIdRec->list() = NULL;
                curIdRec->typ = d.type;
                symHash[d.bucket] = curIdRec;
                jj = typeSize(d.type);
                l2bool8z = true;
                if (curProcNesting == 1) {
                    curExternFile = externFileList;
                    toAlloc = (jj & halfWord) | 047000000;
                    while (l2bool8z and curExternFile != NULL) {
                        if (curExternFile->id == d.name) {
                            l2bool8z = false;
                            if (curExternFile->line == 0) {
                                curVal.ii = curExternFile->offset;
                                curIdRec->value() = allocExtSymbol(toAlloc);
                                curExternFile->line = lineCnt;
                            }
                        } else {
                            curExternFile = curExternFile->next;
                        }
                    }
                }
                if (l2bool8z) {
                    curIdRec->value() = localSize;
                    if (PASINFOR.listMode == 3) {
                        printf("%25s", "VARIABLE ");
                        printTextWord(d.name);
                        printf(" OFFSET (%ld) %05loB. WORDS=%05loB\n",
                               curProcNesting, localSize, jj);
                    }
                    localSize = localSize + jj;
                    curExternFile = NULL;
                }
                if (SY == BECOMES) {
                    if (curProcNesting != 1)
                        error(errVarTooComplex); /* load-time init: globals only */
                    parseInitializer(curIdRec);
                }
                moreDecls = (SY == COMMA);
                if (moreDecls) {
                    inSymbol();
                    d = parseOneDeclarator(baseTy, packedFlag, forwardRef);
                }
            }
            checkSymAndRead(SEMICOLON);
        } else {
            /* ---- routine ---- */
            curIdent = d.name;
            bucket = d.bucket;
            isDefined = d.wasDefined;
            hashTravPtr = d.foundRec;
            if (not isPredefined) {
                if (curFrameRegTemplate == 7) {
                    error(81); /* errProcNestingTooDeep */
                }
                curIdRec = besm6_alloc_record<IdentRec>(
                    offsetof(IdentRec, szRoutine));
                curIdRec->id = curIdent;
                curIdRec->pck.offset = curFrameRegTemplate;
                curIdRec->pck.nidx = ord(symHash[bucket]);
                curIdRec->typ = voidType;
                symHash[bucket] = curIdRec;
                curIdRec->pck.cl = ROUTINEID;
                curIdRec->list() = NULL;
                curIdRec->value() = 0;
                curIdRec->argList() = NULL;
                curIdRec->preDefLink() = NULL;
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
                if (6 < curProcNesting)
                    error(81); /* errProcNestingTooDeep */
                hadParens = (SY == LPAREN);
                if (hadParens)
                    parseParameters(NULL);
                /* The result type comes from the declarator, ahead of the
                   name; there is no ':TYPE' suffix. */
                if (not done) {
                    curIdRec->typ = typedRetType;
                    if (typeSize(curIdRec->typ) != 1)
                        error(errTypeMustNotBeFile);
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
                curIdRec = hashTravPtr;
                // The definition restates the parameter list; it is matched
                // against the declaration's, whose records are reused.
                // parseParameters puts the names in the symbol table.
                workidr = curIdRec->argList();
                if (workidr == NULL)
                    workidr = curIdRec;
                hadParens = (SY == LPAREN);
                if (hadParens)
                    parseParameters(workidr);
                setup(scopeBound);
            } /* 23224 */
            if (SY == BEGINSY) {
                if (not hadParens)
                    error(42); /* errNoParamList */
                setup(scopeBound);
                inSymbol();
                programme(l2int18z, curIdRec, true);
                myrollup(scopeBound);
                exitScope(symHash);
                exitScope(fieldHash);
                goto L23301;
            }
            if (SY == EXTERNSY or
                (SY == IDENT and
                 (curIdent == litFortran or curIdent == litAssembler))) {
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
                inSymbol();
                checkSymAndRead(SEMICOLON);
            } else {
                checkSymAndRead(SEMICOLON);
                if (isPredefined)
                    error(83); /* errRepeatedPredefinition */
                curIdRec->level() = l2int18z;
                curIdRec->preDefLink() = preDefHead;
                preDefHead = curIdRec;
            }
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
        }
        markTypeSym();
    } /* 23320 */
    markTypeSym();
    if (CH == 0 and bodyBlock_) {
        requiredSymErr(ENDSY);
        return;
    }
    if (bodyBlock_) {
        if (not has((bodyStatSys | blockBegSys), SY) and
            not has(declStartSys | Bits(ENDSY), SY))
            errAndSkip(84 /* errErrorInDeclarations */,
                       skipToSet | bodyStatSys | blockBegSys | Bits(ENDSY));
    } else if (CH != 0 and not has(blockBegSys, SY) and
               not has(declStartSys, SY))
        errAndSkip(84 /* errErrorInDeclarations */, skipToSet);
    } while (not ((bodyBlock_ and (has(bodyStatSys, SY) or
                                  has(declStartSys | Bits(ENDSY), SY))) or
                  (not bodyBlock_ and (CH == 0 or has(statBegSys, SY) or
                                      has(declStartSys, SY)))));
    // Checked once per programme() call (guarded by curProcNesting==1, so
    // effectively once for the whole compile), not once per do-while
    // iteration: each variable or typedef declaration stands on its own,
    // so a run of N of them is N do-while iterations, not one --
    // running this check inside the loop re-flagged the same
    // not-yet-declared extern names on every iteration, and the resulting
    // flood of errUndefinedExternFile errors tripped error()'s own
    // "too many errors" abort path (skipToEnd()) mid-file.
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
    }
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
        opToInsn[IDIVOP] = 11; /* P/DI */
        opToInsn[IMODOP] = 7; /* P/MD */
        opToInsn[PLUSOP] = InsnTemp[ADD];
        opToInsn[MINUSOP] = InsnTemp[SUB];
        opToInsn[IMULOP] = InsnTemp[AMULX];
        opToInsn[SETAND] = InsnTemp[AAX];
        opToInsn[SETXOR] = InsnTemp[AEX];
        opToInsn[SETOR] = InsnTemp[AOX];
        opToInsn[INTPLUS] = InsnTemp[ADD];
        opToInsn[INTMINUS] = InsnTemp[SUB];
        opToInsn[SHLEFT] = 56;
        opToInsn[SHRIGHT] = 57;
        opFlags[ANDOP] = opfAND;
        opFlags[IDIVOP] = opfDIV;
        opFlags[OROP] = opfOR;
        opFlags[IMULOP] = opfMULMSK;
        opFlags[IMODOP] = opfMOD;
        opFlags[ASSIGNOP] = opfASSN;
        opFlags[SHLEFT] = opfSHIFT;
        opFlags[SHRIGHT] = opfSHIFT;
    } /* initInsnTemplates */

    void regKeyWords() {
        SY = EXPROP;
        charClass = INOP;
        regResWord(toText("IN"));
        SY = CONSTSY;
        charClass = NOOP;
        // CONSTSY..RETURNSY are 19 consecutive reserved words. TYPESY sits just
        // before CONSTSY, outside this range -- no skip needed; the predefined
        // type names are registered as TYPESY keywords later, by initScalars,
        // and markTypeSym still raises user typedef names to TYPESY.
        for (idx = 0; idx <= 18; ++idx) {
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
        for (idx=1; idx <= 57; ++idx)
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
        koi2text['_'] = koi2text['*']; // iso2text['_'] = iso2text['*']
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
        internHead = NULL;
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
    printf("    -F                  Sort FCST literals by their unsigned 48-bit values\n");
    printf("    -Hooooo             Set first host heap address in octal (9 zones)\n");
    printf("    -i                  Enable automatic fopen/fclose for *INPUT*\n");
    printf("    -k0 -k1 ... -k23    Heap size in 1024-word chunks (default -k4)\n");
    printf("    -l0 -l1 -l2 -l3     Listing mode:\n");
    printf("                        -l0: No listing, only error messages\n");
    printf("                        -l1: Print listing with relative addresses per line\n");
    printf("                        -l2: Also print generated object code\n");
    printf("                        -l3: Also print offsets for variables and fields\n");
    printf("    -m+ -m-             Optimize integer multiplication (positives only)\n");
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
    heapSize = 100;
    forValue = true;
    atEOL = false;
    checkTypes = true;
    fixMult = true;
    declEntry = false;
    enableStdInput = false;
    errors = false;
    allowCompat = false;
    sortFcst = false;
    fileBufSize = 1;
    charEncoding = 2;
    longSymCnt = 0;
    symTabCnt = 0;

    // Get base name of the program.
    progname = strrchr(argv[0], '/');
    progname = progname ? progname+1 : argv[0];

    for (;;) {
        switch (getopt(argc, argv, "vVhiFH:e:c:r:m:y:u:f:a:d:k:b:s:l:")) {
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
        case 'F':
            sortFcst = true;
            continue;
        case 'H': {
            const char *end = optarg;
            while ('0' <= *end && *end <= '7')
                ++end;
            unsigned long base = strtoul(optarg, NULL, 8);
            if (end == optarg || *end != '\0' ||
                base == 0 || base > 074000 - 9 * 1024) {
                fprintf(stderr,
                        "%s: Bad option -H: expected an octal address from 1 through 052000\n",
                        progname);
                exit(-1);
            }
            heapBase = base;
            avail = heapBase;
            heapLimit = heapBase + 9 * 1024;
            continue;
        }
        case 'i':
            enableStdInput = true;
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
        case 'r':
            // No fuzzy real comparison; the option is a no-op.
            continue;
        case 's':
            curVal.ii = strtoul(optarg, 0, 0);
            if (curVal.ii > 9) {
                fprintf(stderr, "%s: Bad option -s\n", progname);
                exit(-1);
            }
            if (curVal.ii == 3) {
                lineCnt = 1;
            } else if (4 <= curVal.ii && curVal.ii <= 7) {
                optSflags.ii = optSflags.ii | Bits(curVal.ii - 3);
            }
            continue;
        case 'u':
            // Source line length is a compile-time constant (maxLineLen),
            // with no runtime override, so this option is a no-op.
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
    // Variable declarations are dispatched via TYPESY (a leading bound
    // type name), same as routines -- see the unified TYPESY loop in
    // programme's constructor.
    blockBegSys = Bits(CONSTSY, TYPEDEFSY, TYPESY) | Bits(BEGINSY);
    statBegSys = Bits(IDENT, EXPROP, LPAREN, INTCONST)
        | Bits(REALCONST, CHARCONST, STRINGSY, LBRACK)
        | Bits(BEGINSY, IFSY, SWITCHSY, DOSY)
        | Bits(WHILESY, FORSY, GOTOSY)
        | Bits(BREAKSY, CONTSY, RETURNSY, SEMICOLON);
    statEndSys = Bits(SEMICOLON, ENDSY, ELSESY, WHILESY);
    lvalOpSet = Bits(GETELT, GETVAR, GETFIELD) | Bits(DEREF);

    funcInsn[fnABS] = KAMX;
    funcInsn[fnCARD] = KACX;
    funcInsn[fnMINEL] = macro + mcMINEL;
    funcInsn[fnMALLOC] = macro + mcMALLOC;
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
    charSymTabBase['"'] = CHARCONST;
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
                                      // not ASCII 0176 -- `chrClass['~']`
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
    charSymTabBase['{'] = BEGINSY;
    charSymTabBase['}'] = ENDSY;
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

    // Operator precedence table: default precNone, then the
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
    printf(" INITHEAP = %05lo\n", avail);
    curInsnTemplate = 0;
    initTables();
    litAssembler = toText("ASSEMBLE");
    litFortran = toText("FORTRAN");
    litLsb = toText("**LSB");           // '__lsb': '_' shares the code of '*'
    litRegister = toText("REGISTER");   // pins a pointer in an index register
    litMain = toText("MAIN");           // the entry point, called by the level 1 block
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
        printf(" MAXHEAP = %05lo\n", maxHeap);
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

int64_t resWordNameBase[19] = {
        04357566364L             /*"   CONST"*/,
        064716045444546L         /*" TYPEDEF"*/,
        045566555L               /*"    ENUM"*/,
        01212604143534544L       /*"**PACKED"*/,
        0636462654364L           /*"  STRUCT"*/,
        05146L                   /*"      IF"*/,
        0636751644350L           /*"  SWITCH"*/,
        06750515445L             /*"   WHILE"*/,
        0465762L                 /*"     FOR"*/,
        047576457L               /*"    GOTO"*/,
        045546345L               /*"    ELSE"*/,
        04457L                   /*"      DO"*/,
        0457064456256L           /*"  EXTERN"*/,
        04262454153L             /*"   BREAK"*/,
        04357566451566545L       /*"CONTINUE"*/,
        043416345L               /*"    CASE"*/,
        044454641655464L         /*" DEFAULT"*/,
        06556515756L             /*"   UNION"*/,
        0624564656256L           /*"  RETURN"*/};

int64_t helperNames[58] = { 0L,
        06017210000000000L      /*"P/1     "*/,
        06017220000000000L      /*"P/2     "*/,
        06017230000000000L      /*"P/3     "*/,
        06017240000000000L      /*"P/4     "*/,
        06017250000000000L      /*"P/5     "*/,
        06017260000000000L      /*"P/6     "*/,
        04317554400000000L      /*"C/MD    "*/,
        06017555100000000L      /*"P/MI    "*/,
        06017604100000000L      /*"P/PA    "*/,
/*10*/  06017655600000000L      /*"P/UN    "*/,
        04317445100000000L      /*"C/DI    "*/,
        06017454100000000L      /*"P/EA    "*/,
        06017214400000000L      /*"P/1D    "*/,
        06017474400000000L      /*"P/GD    "*/,
        04317450000000000L      /*"C/E     "*/,
        04317454600000000L      /*"C/EF    "*/,
        06017566700000000L      /*"P/NW    "*/,
        06017446300000000L      /*"P/DS    "*/,
        06017506400000000L      /*"P/HT    "*/,
/*20*/  04317675100000000L      /*"C/WI    "*/,
        06017676200000000L      /*"P/WR    "*/,
        06017674300000000L      /*"P/WC    "*/,
        06017412600000000L      /*"P/A6    "*/,
        06017412700000000L      /*"P/A7    "*/,
        06017677000000000L      /*"P/WX    "*/,
        06017675700000000L      /*"P/WO    "*/,
        06017436700000000L      /*"P/CW    "*/,
        06017264100000000L      /*"P/6A    "*/,
        06017274100000000L      /*"P/7A    "*/,
/*30*/  06017675400000000L      /*"P/WL    "*/,
        06017675754560000L      /*"P/WOLN  "*/,
        06017626200000000L      /*"P/RR    "*/,
        04317646200000000L      /*"C/TR    "*/,
        06017546600000000L      /*"P/LV    "*/,
        04657604556000000L      /*"FOPEN   "*/,
        04643545763450000L      /*"FCLOSE  "*/,
        06017426000000000L      /*"P/BP    "*/,
        06017422600000000L      /*"P/B6    "*/,
        06017604200000000L      /*"P/PB    "*/,
/*40*/  06017422700000000L      /*"P/B7    "*/,
        06017515600000000L      /*"P/IN    "*/,
        06017516400000000L      /*"P/IT    "*/,
        06017435300000000L      /*"P/CK    "*/,
        06017534300000000L      /*"P/KC    "*/,
        06017545647604162L      /*"P/LNGPAR"*/,
        06017544441620000L      /*"P/LDAR  "*/,
        06017202043000000L      /*"P/00C   "*/,
        06017636441620000L      /*"P/STAR  "*/,
        06017674100000000L      /*"P/WA    "*/,
/*50*/  06017456100000000L      /*"P/EQ    "*/,
        06017624100000000L      /*"P/RA    "*/,  // placeholder: keeps the compare family contiguous
        06017474500000000L      /*"P/GE    "*/,
        06017554600000000L      /*"P/MF    "*/,
        06017465500000000L      /*"P/FM    "*/,
        06017565600000000L      /*"P/NN    "*/,
        04317635054000000L      /*"C/SHL   "*/,
        04317635062000000L      /*"C/SHR   "*/};

// A name's index in this table is its procNo, which several helper numbers
// are derived from: pck/unpck take helper procNo+13, pack/unpack helper
// procNo+69, free/halt helper procNo+30.  This is the P2C set
// (BESM/FREE/PACK...).
int64_t systemProcNames[9] = {
/*0*/   0604353L                /*"     PCK"*/,
        06556604353L            /*"   UNPCK"*/,
        060414353L              /*"    PACK"*/,
        0655660414353L          /*"  UNPACK"*/,
        044516360576345L        /*"    FREE"*/,
        050415464L              /*"    HALT"*/,
        042456355L              /*"    BESM"*/,
        06762516445L            /*"   WRITE"*/,
        067625164455456L        /*" WRITELN"*/};
