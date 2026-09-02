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

template <size_t N>
constexpr int64_t toText(const char (&str)[N])
{
    static_assert(N <= 9, "TEXT word cannot hold more than 8 characters");
    int64_t ret = 0;

    for (size_t i = 0; i + 1 < N; ++i) {
        unsigned code = 0;
        char ch = str[i];
        if ('0' <= ch && ch <= '9')
            code = ch - '0' + 020;
        else if ('A' <= ch && ch <= 'Z')
            code = ch - 'A' + 041;
        else if ('a' <= ch && ch <= 'z')
            code = ch - 'a' + 041;
        else if (ch == '*' || ch == '_')
            code = 012;
        else if (ch == '+')
            code = 036;
        else if (ch == '-')
            code = 035;
        else if (ch == '/')
            code = 017;
        ret = ret << 6 | code;
    }
    return ret;
}

FILE * pasinput = stdin;
int PASINPUT;
const char *outFileName = "output.obj";

const char * boilerplate = " PASCAL METAMORPH HELPER (2025) ";

const int SYMTAB_LIMIT = 075500;
// The most formals one routine may have.  Like cases.pairs and declOps this
// is a fixed scratch bound, not a heap chain: the names are only needed
// between reading the list and making the formals just after the scope mark.
// To lift it entirely, give SigRec a pname field: makeFormals would take the
// name from the signature and formalNames would go away, bucket included,
// since addToHashTab derives one from the name.  That costs a word per formal
// of permanent heap, about 106 on a self-compile, which is roughly half of
// what making the formals transient bought in the first place.  The whole
// change is written out above makeFormals.
const int MAXFORMALS = 16;
const int OBJBUF_SIZE = 8192;    // initially 1024

const int64_t
    fnABS = 0, fnSIZEOF = 1, fnOFFSETOF = 2, fnMALLOC = 3,
    fnCARD = 4, fnMINEL = 5,
    fnABSI = 6;

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

// A branch reads the omega left by the last instruction that set it, so the
// load feeding one must survive form1Insn's load-after-store elision.  The
// mode bit rides on the OneInsn, and genOneOp turns it into insnNoElide on
// the buffered word, which is where it has to live: the buffer is reordered
// before it is flushed, so a remembered index would not keep.  insnNoElide is
// bit 27 -- above every macro encoding (the largest is 3*macro + 4096) and
// clear of the masks the flush loop applies.
const int64_t
    macro = 0100000000,
    mdNoElide = 8,
    insnNoElide = 8 * macro,      /* bit 27 */
    mcJUMP = 2,
    mcPOP = 4,
    mcPUSH = 5,
    mcMULTI = 7,
    mcADDSTK2REG = 8,
    mcADDACC2REG = 9,
    /* Adjacent, and in this order: genEntry picks between them with
       'mcACC2ADDR - needPush', argument 1 being on the stack or not. */
    mcSTK2ADDR = 10,
    mcACC2ADDR = 11,
    mcMALLOC = 12,
    mcMINEL = 15,
    mcPOP2ADDR = 19,
    mcCOND2INT = 20,
    mcPCKSTORE = 22;

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
    HEAPLIM =   04000030,      // complement of the heap's top address

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
    KDIV =      0160000,
    KMUL =      0170000,
    KAPX =      0200000,
    KAUX =      0210000,
    KACX =      0220000,
    KANX =      0230000,
    KYTA =      0310000,
//  KASN =      0360000,
    KNTR =      0370000,
    KATI =      0400000,
    KSTI =      0410000,
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
        UNIONSY,    RETURNSY,   STATICSY,   NOSY
};

enum IdClass {
        TYPEID,     ENUMID,     ROUTINEID,  VARID,
        FIELDID,    REGID,      STATICID
};

enum Insn {
/*000*/ ATX,   STX,   OP2,   XTS,   ADD,   SUB,   RSUB,  AMX,
/*010*/ XTA,   AAX,   AEX,   ARX,   AVX,   AOX,   ADIVX, AMULX,
/*020*/ APX,   AUX,   ACX,   ANX,   EADD,  ESUB,  ASX,   XTR,
/*030*/ RTE,   YTA,   OP32,  OP33,  EADDI, ESUBI, ASN,   NTR,
/*040*/ ATI,   STI,   ITA,   ITS,   MTJ,   MADDJ, ELFUN,
/*047*/ UTC,   WTC,   VTM,   UTM,   UZA,   U1A,   UJ,    VJM
};


/* The order carries meaning, in four places:
   -- every operator with a real and an integer flavour is immediately followed
      by its integer twin (MUL/IMULOP, RDIVOP/IDIVOP, PLUSOP/INTPLUS,
      MINUSOP/INTMINUS), so bldArithOp reaches the integer one by adding
      'oper != IMODOP': one step along, and none for '%', which has no real
      flavour to be distinguished from and so is already the integer one;
   -- NEOP..LEOP run contiguously in that order: genComparison indexes them as
      curOP-NEOP, and takes the odd ones for the negated sense;
   -- SHLEFT is 0, and SHLEFT..SETOR, GETELT..ALNUM and TOREAL..BITNEGOP are
      each contiguous, being written as ranges.  GETELT..ALNUM is the set whose
      value reaches the accumulator through a *load*, so omega already means
      "is it zero" and formOperator(BRANCH) needs no AEX to set it.  RMWASSIGN
      is deliberately below GETELT and not in it: it stores after arithmetic,
      leaving the additive omega, which is a sign test;
   -- GETELT sits above every binary operator and DEREF above every lvalue one,
      which formOperator tests with '<'. */
enum Operator {
    SHLEFT,     SHRIGHT,
    SETAND,     SETXOR,     SETOR,
    MUL,        IMULOP,     RDIVOP,     IDIVOP,     IMODOP,
    PLUSOP,     INTPLUS,    MINUSOP,    INTMINUS,   ANDOP,
    OROP,       NEOP,       EQOP,       LTOP,       GEOP,
    GTOP,       LEOP,       CONDOP,     ALTERN,
    INCROP,     DECROP,     ASSIGNOP,   RMWASSIGN,
    GETELT,     GETVAR,     GETENUM,    GETFIELD,   DEREF,
    STKLVAL,    INDCALL,    PROCADDR,   ALNUM,
    TOREAL,     TOINT,      NOTOP,      INEGOP,     RNEGOP,
    BITNEGOP,   STANDPROC,  ADDROF,     NOOP
};

static_assert(IMULOP == MUL + 1 and IDIVOP == RDIVOP + 1 and
              INTPLUS == PLUSOP + 1 and INTMINUS == MINUSOP + 1,
              "bldArithOp steps by one, so every arithmetic operator's integer "
              "twin has to come immediately after it in the enum");

enum OpGen {
    gen0,  STORE, LOAD,  FORMOP,  SETREG,
    DOIT,  SETREG12,  BRANCH
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
/* The top of the heap, moved down to reserve a region whose lifetime is a
   statement's, not an arena mark's.  settop() publishes it to the allocator,
   so besm6_alloc_words' own limit test keeps allocation out of those words. */

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

/* The runtime keeps the complement of the LAST USABLE word, while heapLimit
   here is one past it, so the two conversions carry the difference and both
   compilers can speak of the top as the last word malloc may return. */
int64_t heaptop()
{
    return heapLimit - 1;
}

void settop(int64_t top)
{
    heapLimit = top + 1;
}

/* talloc(n) - n words off the top of the heap, the mirror of libc/talloc.madlen.
   The block is [top-n+1, top] and the limit drops below it, so the arena's own
   test keeps allocation out of it.  Meeting the arena means the heap is full,
   which is P/NW's diagnostic and its wording. */
int64_t talloc(int64_t words)
{
    int64_t newtop = heaptop() - words;
    if (newtop <= avail) {
        printf(" NO GLOBAL MEMORY\n");
        exit(1);
    }
    settop(newtop);
    return newtop + 1;
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

// NULL is zero, and arena index 0 is never handed out (allocation starts at
// heapBase), so ptr(0) is exactly NULL.
void * ptr(int64_t x)
{
    if (x == 0) return NULL;
    if (x < 0 || x >= avail) {
        fprintf(stderr, "Cannot convert %ld to a pointer, avail = %ld\n", x, avail);
        exit(1);
    }
    return heap + x;
}

int64_t ord(void * p)
{
    int64_t ret = reinterpret_cast<int64_t>(p);
    if (p == NULL) return 0;
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
    uint64_t rep   : 15;   // arena word-index of the Types record (0 = nil)
    uint64_t bits  : 6;
    uint64_t pk    : 3;    // Kind
    uint64_t psize : 15;
    uint64_t pad   : 8;    // packed-array element width, or pointer metadata
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

int64_t pck(unsigned char * p)
{
    int64_t w = 0;
    for (int i = 0; i < 6; ++i)
        w = (w << 8) | p[i];
    return w;
}

void unpck(int64_t w, unsigned char * p)
{
    for (int i = 5; i >= 0; --i) {
        p[i] = w & 0xFF;
        w >>= 8;
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
            int64_t perword, asize;
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
            int64_t rflags;
            int64_t szRtype;
        };
    };

    std::string p() const { return "type"; } // details live in TPtr
};

inline Types * TPtr::rep() const
{
    return p.rep == 0 ? nullptr : reinterpret_cast<Types*>(heap + p.rep);
}

inline void TPtr::setRep(TypesPtr t)
{
    p.rep = ord(t);
}

inline bool TPtr::operator==(const void * q) const { return rep() == q; }
inline bool TPtr::operator!=(const void * q) const { return rep() != q; }

// A routine's signature: the permanent record of its parameters.  The
// formals' own identrecs live only as long as the body that names them, so
// everything a later call site or a definition needs is here -- the class,
// the type, and the frame slot the formal occupies.  pclass is three bits
// and poffset a frame offset, so the two share a word and the record stays
// three, mirroring work.p2c's sigrec.
struct SigRec : public BESM6Obj {
    union {
        int64_t s1word;
        struct {
            uint64_t pclass  : 3;
            int64_t  poffset : 24;
        };
    } s1;
    TPtr ptyp;
    SigPtr next;
    SigRec() : s1{0}, ptyp(), next(nullptr) { s1.pclass = TYPEID; }
};

typedef char charmap[128];
typedef char textmap[128];

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

/* One label of a switch: the value it matches and the code offset it enters
   at.  The labels of one switch hang off curSwitch.allClauses in value order. */
struct CaseChain : public BESM6Obj {
    CaseChain * next;
    Word value;
    int64_t offset;
};
typedef CaseChain * CaseChainPtr;

/* The switch being parsed: its labels in value order, the type they all have
   to share, whether a default has been seen and the offset it enters at, and
   whether every arm so far left the arithmetic mode alone.  One assignment
   saves the lot, which is what lets a switch nested in an arm of another keep
   its own. */
struct SwState {
    CaseChainPtr allClauses;
    TPtr firstType;
    bool otherSeen;
    int64_t otherOffset;
    bool goodMode;
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
            uint64_t nidx   : 15;   // arena word-index (0 = nil)
            int64_t  offset : 24;
            uint64_t cl     : 3;    // IdClass
        };
    } pck;
    int64_t id;
    TPtr typ;
    // TYPEID, VARID classes end here
    union {
        struct {                // TYPEID: nothing past typ
            int64_t szBase;
        };
        struct {                // ENUMID
            IdentRecPtr list_;
            int64_t value_;
            int64_t szIdent;
        };
        struct {                // FIELDID
            int64_t maybeUnused_;
            TPtr uptype_;
            // A packed field's placement, one word instead of three,
            // mirroring work.p2c's fldpck union.  shift and width both run
            // 0..48, so six bits each; they are read only when pckfield is
            // set, and a fresh record is zero, which is what a 48-bit packed
            // member (whose shift is never assigned) relies on.
            union {
                int64_t fpkword;
                struct {
                    uint64_t pckfield : 1;
                    uint64_t shift    : 6;
                    uint64_t width    : 6;
                };
            } fpk;
            int64_t szField;
        };
        struct {                // ROUTINEID
            int64_t low_;
            int64_t high_;
            // Four small fields in two words, mirroring work.p2c's rtnpck1
            // and rtnpck2.  Both pointers are 15-bit arena word-indices at
            // the bottom of their word (as pck.nidx is), so a dereference
            // costs nothing extra; pos is a symbol-table position and level
            // the frame size the definition inherits from its declaration,
            // 8 or 9.
            union {
                int64_t r1word;
                struct {
                    uint64_t sig : 15;
                    int64_t  pos : 24;
                };
            } r1;
            union {
                int64_t r2word;
                struct {
                    uint64_t preDefLink : 15;
                    int64_t  level      : 9;
                };
            } r2;
            int64_t flags_;
            int64_t szRoutine;
        };
        struct {                // predefined system routine
            int64_t sysnum_;
            int64_t szSys;
        };
    };
    // Hash-chain link, stored as a compact arena index in pck (see above).
    // nidx==0 is both nil and the memset-fresh state (besm6_alloc zero-fills),
    // so a just-allocated record reads NULL.
    IdentRecPtr next() const {
        // heap + index directly (not ptr(), whose bounds check rejects the
        // dangling-but-unused links a native pointer tolerated across rollup).
        return pck.nidx == 0 ? NULL
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
    // Arena-index accessors, exactly as next() is: 0 reads back as NULL,
    // and heap + index skips ptr()'s bounds check because a link may outlive
    // its record across a rollup.
    SigPtr sig() const {
        return r1.sig == 0 ? NULL
             : reinterpret_cast<SigPtr>(heap + r1.sig);
    }
    void setSig(SigPtr p) { r1.sig = ord(p); }
    IdentRecPtr preDefLink() const {
        return r2.preDefLink == 0 ? NULL
             : reinterpret_cast<IdentRecPtr>(heap + r2.preDefLink);
    }
    void setPreDef(IdentRecPtr p) { r2.preDefLink = ord(p); }
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
                if (0 <= asprintf(&strp, "(routine) procno: %ld value: %ld sig: %ld predef: %ld level: %ld pos: %ld flags: %lx",
                                  low_, value_, (long)r1.sig, (long)r2.preDefLink,
                                  (long)r2.level, (long)r1.pos, flags_)) {
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
Symbol   SY;

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
        // Module-lifetime globals and routine statics share this high-water
        // mark; routine locals continue to use programme::localSize.
        moduleDataSize,
        totalErrors,
        lineCnt,
        eofOverreads,
        bucket,
        strLen,
        strWords,
        heapCallsCnt,
        heapSize,
        arithMode;

std::string stmtName;
TPtr symType;                   // the type denoted by the current TYPESY
Kind curVarKind;
ExtFileRec * curExternFile;
char commentModeCH;
unsigned char CH;

// A double-quoted string is packed into strBuf rather than laid straight into
// the constant pool: which pool the words belong in is the consumer's to
// decide.  parseLiteral puts them in FCST, an expression needing an address to
// load from; parseInitializer puts them into the initializer stream instead,
// where they cost no pool word and coalesce with the items around them.  The
// longest a string can reach is the 125 characters doCharConst stores, which
// is 21 words.
int64_t strBuf[21];

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
    bool110z,
    sortFcst,
    checkFortran;

int verbose;

IdentRecPtr outputFile,
    inputFile,
    hashTravPtr,
    uProcPtr;

ExtFileRec * externFileList;

TPtr typ121z;
TPtr voidType, voidPtr;
// Expression-operator table, filled in the initialize section
// (opPrec = precNone:48 ...).
int64_t opPrec[64];
TPtr BooleanType;
TPtr IntegerType;
// This machine has one integer but not one multiply: 'unsigned' is a type of
// its own only so that a product of unsigned operands takes the multiply the
// m- mode takes.  Everything else treats the two alike -- typeCheck holds any
// two non-enum scalars compatible -- which is what isIntTyp is for.
TPtr UnsignedType;
TPtr RealType;
TPtr CharType;
TPtr charPtrType, flatMemType;
IdentRecPtr flatMemVar;

TPtr arg1Type, arg2Type;

NumLabel numLabs[20];
int64_t numLabTop;
Word curToken, curVal;
const int64_t extSymMask = 043000000L;
const int64_t halfWord = 077777777L;
const int64_t leftAddr = 077777L << 24;

int64_t leftInsn;
int64_t curIdent;
int64_t usedRegs, liveRegs, freeRegs, auxRegs;

/* M2-M6.  Every C/n entry helper saves all five in the callee's own frame and
   C/E puts them back, so a call hands them over exactly as it found them.  A
   pinned base always lives in this range -- freeRegs is a subrange of it --
   so no call may take one out of liveRegs, whatever the callee's flags say
   about the registers it touches.  An ASSEMBLER external has no entry helper
   to do the saving and is held to the same rule by hand. */
const int64_t calleeSaved = BitRange(2,6);
ExprPtr uVarPtr, curExpr;
InsnList *  insnList;
InternRec * internHead;
ExtFileRec * fileForOutput, * fileForInput;
int64_t symTabCnt;
Entries entryPtTable;
int64_t indexreg[16];
int64_t opToMode[48];
int64_t funcInsn[7];
int64_t InsnTemp[48];

int64_t frameRegTemplate = 04000000;

char lineBufBase[132]; // array [1..130] of char;
int64_t errMapBase[10];
Operator chrClassTabBase[256]; // array ['_000'..'_177'] of Operator;
KeyWord * KeyWordHashTabBase[128]; // array [0..127] of @KeyWord;
Symbol charSymTabBase[256]; // array ['_000'..'_177'] of Symbol;
IdentRecPtr symHash[128]; // array [0..127] of IdentRecPtr;
IdentRecPtr fieldHash[128]; //array [0..127] of IdentRecPtr;
int64_t helperMap[31];
extern int64_t helperNames[31]; // array [1..30] of int64_t;

// Zero-based backing storage; symTabPos and stored references remain BESM
// symbol-table addresses starting at 074000.
int64_t symTab[SYMTAB_LIMIT - 074000 + 1];
int64_t longSymCnt;
int64_t longSymTabBase[90];
int64_t longSyms[90];
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
        "SHLEFT","SHRIGHT","SETAND","SETXOR","SETOR","MUL","IMULOP","RDIVOP",
        "IDIVOP","IMODOP","PLUSOP","INTPLUS","MINUSOP","INTMINUS","ANDOP",
        "OROP","NEOP","EQOP","LTOP","GEOP","GTOP","LEOP","CONDOP",
        "ALTERN","INCROP","DECROP","ASSIGNOP","RMWASSIGN","GETELT","GETVAR",
        "GETENUM","GETFIELD","DEREF","STKLVAL","INDCALL","PROCADDR","ALNUM",
        "TOREAL","TOINT","NOTOP","INEGOP","RNEGOP","BITNEGOP","STANDPROC",
        "ADDROF","NOOP"
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
TPtr makeArrayType(int64_t, TPtr, bool);

void defExtern()
{
    int64_t line = 0;
    Word aligned;
    IdentRecPtr idRec;

    aligned.ii = leftAlign(curIdent);
    if (curIdent == toText("*INPUT*") || curIdent == toText("*OUTPUT*")) {
        idRec = besm6_alloc_record<IdentRec>(offsetof(IdentRec, szIdent));
        idRec->id = curIdent;
        idRec->pck.offset = 0;
        /* An FCB is 30 opaque words.  Its size is all the compiler needs
           of the type -- that is how put/get/reset/rewrite and write's
           leading file argument recognize a file -- and no operator
           applies to it, so it is a void of 30 words. */
        idRec->typ.word = 0;
        idRec->typ.setRep(NULL);        // ord(NULL) == 0, as in work.p2c
        idRec->typ.p.psize = 30;
        idRec->pck.cl = VARID;
        idRec->list() = NULL;
        curVal = aligned;
        idRec->value() = allocExtSymbol(047000000 | 30);
        addToHashTab(idRec);
        if (curIdent == toText("*INPUT*"))
            inputFile = idRec;
        else
            outputFile = idRec;
        line = lineCnt;
    }
    curExternFile = externFileList;
    while (curExternFile) {
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
    if (line) {
        if (curIdent == toText("*OUTPUT*"))
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
    SetOfSYs bodyStatSys;
    StrLabel * strLabList;
    /* The switch being parsed.  A label reaches it from wherever it is
       written. */
    SwState curSwitch;
    /* Non-zero while a switch body is being parsed, so a label knows it has
       one to bind to.  caseStatement counts it up and down, which nests by
       itself. */
    int64_t switchDepth;
    CaseChainPtr curClause, clause, prev;
    TPtr itemtype;
    Word itemvalue;

    int64_t l2int18z, ii, localSize, sizeCount, jj;
    int64_t toAlloc;
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
       The list runs IDENT..STATICSY, i.e. the whole enum bar NOSY, because
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
    case STATICSY + 88:  return "STATIC";
    }
    return "Dunno";
}

void printTextWord(int64_t val);

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
        else if (errNo == 17) {
            printTextWord(curToken.ii);
            putchar(' ');
        }
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
    // work.p2c is 'write(' '); PASTPR(val)', and PASTPR prints the whole text
    // word -- eight characters, the name right-aligned in them.  Trimming the
    // blanks, or dropping the space in front, would shift every name the
    // listing prints and the two compilers' listings would differ by
    // whitespace alone.
    putchar(' ');
    fputs(toAscii(val).c_str(), stdout);
}

std::string Word::pt() const
{
    return toAscii(ii);
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
    while (internHead and ord(internHead) >= ord(p))
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

/* The address of an lvalue, typed by the caller: a pointer to the lvalue's
   type where that is what is wanted, an int where the address is about to
   take part in byte arithmetic. */
ExprPtr mkRef(ExprPtr lval, TPtr typ)
{
    return mkExpr(ADDROF, typ, lval, NULL);
} /* mkRef */

ExprPtr cpDsLval(ExprPtr e)
{
    if (e and e->op == DEREF and
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

// Not enabled -- to switch it on, uncomment this and its call in
// formOperator.  It saves one A-X per discarded ++/-- in a compiled
// program, and costs more object words than that in this compiler, whose
// own source spells every discarded step as a prefix ++.
//
// The value of a post-increment is the lvalue's old value, which bldIncDec
// forms by undoing the step afterwards: the inverse operator applied to the
// RMWASSIGN, stepping by the very literal node the RMWASSIGN steps by.
// Where the value is discarded -- an expression statement, a for's step --
// that undo is dead code, so hand back the RMWASSIGN alone.  The shared
// literal node is what identifies the pair.
//
// ExprPtr dropPostFixup(ExprPtr e)
// {
//     ExprPtr rmw, step;
//     if (e->op != INTPLUS and e->op != INTMINUS)
//         return e;
//     rmw = e->expr1;
//     if (rmw->op != RMWASSIGN)
//         return e;
//     step = rmw->expr2;
//     // The inner operator is the outer one's inverse, so it is the other
//     // member of the same pair.
//     if (step->op == e->op or (step->op != INTPLUS and step->op != INTMINUS))
//         return e;
//     if (step->expr1 != e->expr2)
//         return e;
//     return rmw;
// } /* dropPostFixup */

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
    static Word prevInsn;
    int64_t pos;
    bool noElide;
    noElide = (arg & insnNoElide) != 0;
    arg = arg & ~insnNoElide;
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
    } else if (not noElide && prevOpcode > 1 && Insn.ii % 4096 &&
               (Insn.ii ^ prevInsn.ii) == Bits(32)) /* maybe ATX/XTA */ {
// Load after store; if the load reg/off is the same as the store,
// and the store was not a stack push, there is no need to so the read.
// Not when the load is marked: it is feeding a branch, and dropping it would
// leave the branch reading whatever omega the body computed with -- after an
// additive op that is a sign test, not "is it zero".
// prevOpcode > 1, not != -1: at 1 the last thing emitted was a call, and
// prevInsn still holds what preceded it, so it says nothing about memory now.
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
    const int64_t disNormTemplate = KNTR+7;
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
    symTab[symTabPos - 074000] = arg;
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
        for (i = 0; i < longSymCnt; ++i) {
            if (curVal.ii == longSyms[i]) {
                return longSymTabBase[i];
            }
        }
        if (longSymCnt >= 90) {
            error(51); /* errLongSymbolOverflow */
            longSymCnt = 0;
        };
        longSymTabBase[longSymCnt] = symTabPos;
        longSyms[longSymCnt] = curVal.ii;
        longSymCnt++;
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

// Lay the buffered string into the constant pool; answer where it starts.
int64_t strToFCST()
{
    int64_t start = FcstCnt;
    for (int64_t i = 0; i < strWords; ++i) {
        curVal.ii = strBuf[i];
        toFCST();
    }
    return start;
}

bool fcstLess(const Word &left, const Word &right)
{
    if (sortFcst)
        return left.a.val < right.a.val;
    return left.a < right.a;
}

int64_t addCurValToFCST()
{
    const int64_t MAXLIT = 500;
    static Word constVals[MAXLIT];
    static int64_t constNums[MAXLIT];
    int64_t ret;
    int64_t low, high, mid;
    low = 0;
    static std::set<int64_t> lits;
    if (FcstTotal == 0) {
        ret = FcstCnt;
        FcstTotal = 1;
        constVals[0] = curVal;
        constNums[0] = FcstCnt;
        toFCST();
        lits.insert(curVal.ii);
    } else {
        high = FcstTotal - 1;
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
            for (mid = FcstTotal - 1; mid >= high; --mid) {
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
    const int64_t SYMTAB_MAX = 80;
    static int64_t symTabArray[SYMTAB_MAX+1];
    static int64_t symTabIndex[SYMTAB_MAX+1];
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
    while (t) {
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
    while (icand) {
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
        while (arg) {
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
    if ((listMode) or (errsInLine)) {
        printf(" %05lo%5ld%3ld%c", (lineStartOffset + PASINFOR.startOffset),
               lineCnt, lineNesting, commentModeCH);
        startPos = 12;
        do
            linePos = linePos-1;
        while ((lineBufBase[linePos]  == ' ') and (linePos));
        for (err = 1; err <= linePos; ++err) {
            kputc(lineBufBase[err]);
        };
        putchar('\n');
        if (errsInLine)  {
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
    /* Six characters more than doCharConst's 125 and its six-word lead-in:
       the padding word is unpacked past the last character, and a string
       filling its last word exactly is packed one word beyond that for the
       terminator. */
    unsigned char localBuf[137];
    int64_t tokenLen, tokenIdx;
    bool expSign;
    IdentRecPtr l3var135z;
    Real expMultiple, expValue;
    char curChar;
    int64_t numstr[17];
    int64_t expLiteral;
    int64_t expMagnitude;
    int64_t charBits, charMask;
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
        if (maxLineLen < ++eofOverreads) {
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
            case 'E': case 'e':
                readOptFlag(declEntry);
                break;
            case 'F': case 'f':
                readOptFlag(checkFortran);
                break;
            case 'I': case 'i':
                readOptFlag(enableStdInput);
                break;
            case 'L': case 'l':
                PASINFOR.listMode = readOptVal(3);
                break;
            case 'C': case 'c':
                readOptFlag(checkTypes);
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
    KeyWord * keyWordHashPtr;
    unsigned char prevCH;
    unsigned char litQuote = 0;
    int literalEncoding = charEncoding;
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
                if (tokenLen == 2 and curToken.ii == koi2text['T'] and
                    (CH == '\'' or CH == '"')) {
                    literalEncoding = 3;
                    goto L2290;
                }
                bucket = curToken.ii % 65535 % 128;
                curIdent = curToken.ii;
                keyWordHashPtr = KeyWordHashTabBase[bucket];
                while (keyWordHashPtr) {
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
                case lookDef: {
                    hashTravPtr = symHash[bucket];
                    while (hashTravPtr) {
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
                case lookUse: {
L2:                 hashTravPtr = symHash[bucket];
                    while (hashTravPtr) {
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
                case 2:          // unassigned; work.p2c has no arm for it
                    goto L2;
                case lookField:
                    hashTravPtr = fieldHash[bucket];
                    while (hashTravPtr) {
                        if ((hashTravPtr->id == curIdent) and
                            (typ121z == hashTravPtr->uptype()))
                            goto exitLexer;
                        hashTravPtr = hashTravPtr->next();
                    }
                    break;
                }
                goto exitLexer;
            } break; /* IDENT */
            case INTCONST: {
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
                            goto Lsuffix;
                        }
                        numFormat = octal;
                        curToken.ii = 0;
                        for (tokenIdx = 1; tokenIdx <= tokenLen; ++tokenIdx) {
                            if (7 < numstr[tokenIdx])
                                error(20); /* errDigitGreaterThan7 */
                            curToken.ii = shl48(curToken.ii, 3);
                            curToken.ii = (numstr[tokenIdx] & 7) | curToken.ii;
                        }
                        goto Lsuffix;
                    } else {
                        numFormat = decimal;
                        goto exitOctdec;
                    }
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
                if (CH == 'U' or CH == 'u') {
                    curToken.ii = curToken.ii & ~ Bits(0, 1, 3);
                    goto Lsuffix;
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
                if (CH == 'E' or CH == 'e') {
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
                if (expMagnitude) {
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
                        if (expMagnitude)
                            expMultiple = expMultiple*expMultiple;
                    } while (expMagnitude);
                    if (expSign)
                        curToken.r = curToken.r / expValue;
                    else
                        curToken.r = curToken.r * expValue;
                }
                goto exitLexer;
              Lsuffix:
                /* A 'U' suffix says the digits are a bit pattern filling the
                   word, which is what fullword records: the literal is
                   unsigned, so a product of it takes the unsigned multiply.
                   All three bases end here; only the decimal one has to mask
                   before it does. */
                if (CH == 'U' or CH == 'u') {
                    numFormat = fullword;
                    nextCH();
                }
                goto exitLexer;
            } break; /* INTCONST */
            case CHARCONST: {
                literalEncoding = charEncoding;
L2290:
                {
                    /* The delimiter tells a character constant from a string:
                       single quotes give the packed word as an integer wherever
                       it fits one, double quotes give a packed character array
                       of the length written. An adjacent t or T prefix selects
                       TEXT encoding for that literal. */
                    litQuote = CH;
                    for (tokenIdx = 6; tokenIdx <= 130; ++tokenIdx) {
                        nextCH();
                        if (CH == litQuote) {
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
                            /* A backslash immediately before the physical
                               newline splices the next source line into this
                               literal.  It occupies no slot; the next
                               iteration reads the first character of the new
                               line. */
                            if (CH == '\n') {
                                endOfLine();
                                --tokenIdx;
                                continue;
                            }
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
L2233:                      switch (literalEncoding) {
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
                    strLen = 1;
                }
                /* A character is eight bits in ISO and six in TEXT, so a word
                   holds six of them or eight. */
                charBits = literalEncoding == 3 ? 6 : 8;
                charMask = literalEncoding == 3 ? 077 : 0377;
                if (litQuote == '\'' and strLen * charBits <= 48) {
                    /* A single-quoted literal is one word, right-aligned from
                       one character up: the characters land in the low end, so
                       'AB' is 'A' * 256 + 'B' and t'AB' is 'A' * 64 + 'B'.
                       One character is a char, more are that word as an int. */
                    curToken.ii = 0;
                    for (tokenLen = 6; tokenLen < tokenIdx; ++tokenLen)
                        curToken.ii = (curToken.ii << charBits)
                                    | (localBuf[tokenLen] & charMask);
                    curVal = curToken;
                    SY = strLen == 1 ? CHARCONST : INTCONST;
                    goto exitLexer;
                }
                {
                    /* Double quotes give a packed character array of the
                       length written, left-aligned and padded with NULs: six
                       characters to the word.  The words go to strBuf and no
                       further -- where they end up is for the consumer to say.
                       As many of them as the type has, so a length that is an
                       exact multiple of six takes no word of padding along.
                       NUL is what the padding has to be, and one of them
                       belongs to the string: the type counts strLen + 1
                       characters, so every literal carries a terminator and
                       strlen and puts stop on it.  The word count follows
                       that length, which is why a string filling its last
                       word exactly takes another one -- there is nowhere else
                       for the terminator to go.  It costs nothing anywhere
                       else: the terminator lands in padding that was there
                       already. */
                    SY = STRINGSY;
                    unpck(0, &localBuf[tokenIdx]);
                    strWords = (strLen + 6) / 6;
                    for (tokenLen = 0; tokenLen < strWords; ++tokenLen)
                        strBuf[tokenLen] = pck(&localBuf[6 + 6*tokenLen]);
                    /* A one-word string's token is its value, as an
                       integer's is. */
                    curToken.ii = strBuf[0];
                    curVal = curToken;
                    /* Single quotes give one word, so they hold what a word
                       holds: six ISO characters, or eight in TEXT.  A longer
                       string is written in double quotes -- and only a longer
                       one gets this far, one that fits having been taken by
                       the branch above. */
                    if (litQuote == '\'')
                        error(errNumberTooLarge);
                    goto exitLexer;
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
        commentModeCH = ' ';
        lookupMode = lookup2;
    }
} /* inSymbol */

void skipToEnd()
{
    while (CH)
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
    const int64_t INT41_SIGN = 1L << 40;
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
        // C/DI truncates toward zero and C/MD leaves what it leaves over,
        // which is what C++ does here; work.p2c folds with the target's own
        // '/' and '%' and needs neither spelled out.
        r = a / b;
        break;
    case IMODOP:
        r = a % b;
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

// int and unsigned are one integer in all but the multiply, so a test for
// "an int" takes either.
bool isIntTyp(TPtr t)
{
    return t == IntegerType or t == UnsignedType;
} /* isIntTyp */

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
            if (not isIntTyp(litType)) {
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
            /* A 'U' suffix says the digits are a bit pattern filling the
               word, which is what fullword records, so the literal is
               unsigned and a product of it takes the unsigned multiply. */
            if (numFormat == fullword)
                litType = UnsignedType;
            else
                litType = IntegerType;
            break;
        case REALCONST:
            litType = RealType;
            break;
        case CHARCONST:
            litType = CharType;
            break;
        case STRINGSY:
            /* A string constant is a packed char array of its own length
               and a NUL: the terminator is part of the type, so sizeof
               counts it and write emits it, taking no column.  One word is
               the value itself; more than one goes to the pool now, an
               expression needing an address to load from, and that address
               is the value. */
            litType = makeArrayType(strLen + 1, CharType, true);
            if (strWords != 1)
                litValue.ii = strToFCST();
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
            if (l3var3z) {
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
    if (programme::super.back()->typelist) {
        rec = programme::super.back()->typelist;
        while (rec) {
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
    if ((Bits(21,24,26) & type1.rep()->rflags) !=
        (Bits(21,24,26) & type2.rep()->rflags)) {
        return false;
    }
    if ((type1.rep()->rresult != type2.rep()->rresult) and
        (type1.rep()->rresult == NULL or type2.rep()->rresult == NULL or
         not typeCheck(type1.rep()->rresult, type2.rep()->rresult))) {
        return false;
    }
    p1 = type1.rep()->rparams;
    p2 = type2.rep()->rparams;
    while (p1 and p2) {
        if (p1->s1.pclass != p2->s1.pclass)
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
                if (typeCheck(type1.rep()->base, type2.rep()->base) and
                    (type1.rep()->asize == type2.rep()->asize) and
                    (type1.p.pad == type2.p.pad))
                    goto L1;
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

// The argument slots a signature claims.  A formal wider than a word occupies
// that many consecutive slots, so the count is in words, not in formals: it is
// what the caller must reserve and what the callee unwinds.
int64_t argWords(SigPtr it)
{
    int64_t l3var1z;
    l3var1z = 0;
    while (it) {
        l3var1z = l3var1z + typeSize(it->ptyp);
        it = it->next;
    }
    return l3var1z;
} /* argWords */

// The routine type proper: a result type and a parameter signature.  Setting
// rep clears the descriptor metadata, so pad comes out 0 and getPtrType can
// compact-encode a pointer to this type.
TPtr mkRoutineTyp(TPtr result, SigPtr params, int64_t flags)
{
    TPtr resultTyp{};

    resultTyp.setRep(besm6_alloc_record<Types>(offsetof(Types, szRtype)));
    resultTyp.rep()->rresult = result;
    resultTyp.rep()->rparams = params;
    resultTyp.rep()->rflags = flags;
    resultTyp.p.psize = 1;
    resultTyp.p.bits = 15;
    resultTyp.p.pk = kindRoutine;
    return resultTyp;
}

// The type of an already declared routine.  Its signature is kept from the
// declaration, so there is nothing to rebuild.
TPtr makeRoutineType(IdentRecPtr routine)
{
    return mkRoutineTyp(routine->typ, routine->sig(), routine->flags());
}

struct formOperator {
    static std::vector<formOperator*> super;
    formOperator(OpGen l3arg1z);
    ~formOperator() { super.pop_back(); }

    /* scratch3 is one word lent out to four unrelated jobs, none of whose
       lives overlap: genComparison holds the comparison's distance from NEOP
       in it, genFullExpr the arithmetic mode of the instruction it is
       emitting, formOperator's own body first an instruction count and later
       a jump target.  Each of those says at the point of use what it is
       holding, since the name cannot. */
    int64_t l3int2z, scratch3;
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
        while (l4inl7z) {
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
        bool noElide = false;
        int64_t bufMark = 0;
        if (insnList == NULL)
            return;
        usedRegs = usedRegs | insnList->regsused;
        l4oi212z = insnList->head;
        l4var9z = KNTR+7;
        insnBufIdx = 1;
        if (l4oi212z == NULL)
            return;
        l4inl6z = NULL;

        while (l4oi212z) {
            tempInsn.ii = l4oi212z->code;
            l4var4z = tempInsn.ii -  macro;
            curInsn.ii = l4oi212z->offset;
            // Read before the node is stepped past, applied after its
            // instruction is buffered.
            noElide = (l4oi212z->mode & mdNoElide) != 0;
            bufMark = insnBufIdx;
            switch (l4oi212z->mode & 7) {
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
                    if (l4oi212z) {
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
                case mcSTK2ADDR:
                    /* The same, for a call whose argument 1 is on the stack:
                       STI takes the address and pops the argument back. */
                    add2InsnsToBuf(KSTI+14, KUTC+I14);
                    break;
                case mcMULTI: {
                    addInsnToBuf(getHelperProc(7));        /* P/MI */
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
                   helper #13, which returns the newly
                   allocated pointer in ACC.  Same calling convention as
                   the NEW system procedure. */
                    add2InsnsToBuf(KATI+14, getHelperProc(13));
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
            if (noElide and (insnBufIdx != bufMark))
                insnBuf[insnBufIdx-1].ii = insnBuf[insnBufIdx-1].ii | insnNoElide;
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
            noElide = (curInsn.ii & insnNoElide) != 0;
            curInsn.ii = curInsn.ii & ~insnNoElide;
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
                form1Insn(noElide ? curInsn.ii | insnNoElide : curInsn.ii);
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
                if (curInsn.ii) {
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
        while (l4inl6z) {
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
            if (sh)
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
    const int64_t constRegTemplate = I8;
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
                    break;
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
                    (l4int2z))
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
                addToInsnList(getHelperProc(isSimple
                    ? 20 /* "P/LDAR" */ : 14 /* "P/RR" */));
                insnList->tail->mode = 1;
            }
        } // FALLTHRU
        case ilRVAL: {
            // A value already in ACC can still be a slice: GETFIELD on a
            // function result records shift/width without an address to
            // load from.  genSliceExtract clears st, so the fallthrough
            // from ilLVAL cannot extract twice.
            if (insnList->st == stSLICE)
                genSliceExtract();
            if (forValue and (valueType == BooleanType) and
                has(insnList->regsused, 16))
                addToInsnList(KAEX+E1);
        } break;
        case ilCOND: {
            if (forValue)
                addInsnAndOffset(macro+mcCOND2INT,
                                 has(insnList->regsused, 16)*010000 + insnList->payload.ii);
        } break;
    } /* case */
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
            } else if (l4var4z) {
                addInsnAndOffset(l4var4z + InsnTemp[UTC], l4var6z);
                addToInsnList(regField + opCode);
            } else {
                addInsnAndOffset(regField + opCode, l4var6z);
            }
        } else if (l4int2z == 17) {
            getOffset();
            l4var4z = insnList->disp;
            l4var5z = insnList->tail->code - InsnTemp[UTC];
            if (l4var4z) {
                l4var1z.ii = macro * l4var5z + l4var4z;
                l4var5z = allocSymtab(l4var1z.ii & 0777777777777L);
            }
            insnList->tail->code = regField + l4var5z + opCode;
        } else if (l4int2z == 16) {
            getOffset();
            if (l4var4z)
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
                if (l4int2z)
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
                if (l4int2z)
                    prependToInsnList(ASN64 - l4int2z);
                prependToInsnList(InsnTemp[YTA]);
                prependToInsnList(ASN64 - l4int1z);
            }
            addToInsnList(getHelperProc(22)); /* "P/STAR" */
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

ExprPtr stripIdx(ExprPtr idx, int64_t &offset);

void startLVal()
{
    prepLoad();
    insnList->ilm = ilLVAL;
    insnList->st = stWORD;
    insnList->disp = 0;
    insnList->payload.ii = 0;
    insnList->addrmd = 18;
} /* startLVal */

void genDeref()
{
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
} /* genDeref */

void negateCond()
{
    if (insnList->ilm == ilCONST) {
        insnList->payload.ii = not insnList->payload.ii;
    } else {
        insnList->regsused = insnList->regsused ^ Bits(16);
    }
} /* negateCond */

/* An lvalue whose address is reached without touching the accumulator and
   without side effects: a plain variable, a dereference of one, a field of
   such an lvalue, or an element at a variable index.  genRMWAssign may walk
   one of these twice instead of materializing its address on the stack.
   The element case does not look at the base for the same reason the field
   case does not add code: reaching it only adjusts the displacement.
   A pinned base (NOOP) is the cheapest of all -- genFullExpr answers it with
   a bare descriptor, and at most a WTC/VTM reload for a base sharing the
   fallback register, neither of which touches the accumulator -- but only on
   that path: the arm that finds the register no longer live rebuilds the
   address from expr2, which does cost instructions and the accumulator. */
bool isCheapLval(ExprPtr e)
{
    int64_t idxOffset;
    switch (e->op) {
    case GETVAR:   return true;
    case DEREF:    return e->expr1->op == GETVAR;
    case GETFIELD: return isCheapLval(e->expr1);
    case GETELT:   return stripIdx(e->expr2, idxOffset)->op == GETVAR;
    case NOOP:     return e->vt.typ.p.psize != 0
                       or has(liveRegs, e->vt.typ.p.pad);
    default:       return false;
    }
} /* isCheapLval */

/* True when the value about to be branched on reached the accumulator through
   a load, so omega already means "is it zero".  Anything else leaves whatever
   omega the arithmetic set, which is a sign test, and needs an AEX to fix it.
   NOTOP only flips the polarity bit -- a do-while wraps its condition in one
   -- so look under it. */
bool omegaIsZeroTest(ExprPtr e)
{
    while (e->op == NOTOP)
        e = e->expr1;
    return e->vt.typ == BooleanType or
           has((BitRange(SHLEFT, SETOR) | BitRange(GETELT, ALNUM)), e->op);
}

/* Load one && / || operand and leave omega meaning "is it zero", for the
   conditional jump genBoolAnd is about to emit on it.  Same two obligations
   formOperator(BRANCH) has: a load must survive the peephole, and a value
   that did not come from one needs the AEX. */
void prepBoolArg(ExprPtr e)
{
    bool wasLval = insnList->ilm == ilLVAL;
    prepLoad();
    if (wasLval)
        insnList->tail->mode = insnList->tail->mode | mdNoElide;
    else if (not omegaIsZeroTest(e))
        addToInsnList(KAEX);
} /* prepBoolArg */

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
    /* What genRecip answered about the constant divisor: read by genConstDiv,
       and by opfMOD to tell a power of two from anything else. */
    int64_t recip;

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
        /* A one-constant pair still takes the ordinary chain path. OROP has
           already negated both lists before calling here, and a constant
           right operand must not discard the left operand's side effects.
           The chain also normalizes a surviving nonzero operand to Boolean 1.
           Two constants fold in mkExprFold. */
        bool lhsNeg, rhsNeg, oldFVal;
        int64_t lhsLab, rhsLab, joinLab;
        InsnList * lhsList;
        Word allRegs;

        lhsNeg = has(insnList->regsused, 16);
        rhsNeg = has(otherIns->regsused, 16);
        joinLab = condLabCnt;
        condLabCnt = condLabCnt + 1;
        oldFVal = forValue;
        forValue = false;
        if (insnList->ilm == ilCOND) {
            lhsLab = insnList->payload.ii;
        } else {
            lhsLab = 0;
            prepBoolArg(exprToGen->expr1);
        }
        if (otherIns->ilm == ilCOND)
            rhsLab = otherIns->payload.ii;
        else
            rhsLab = 0;
        allRegs.ii = insnList->regsused | otherIns->regsused;
        lhsList = insnList;
        if (rhsLab == 0) {
            insnList = otherIns;
            prepBoolArg(exprToGen->expr2);
            otherIns = insnList;
            insnList = lhsList;
        }
        if (lhsLab == 0) {
            if (rhsLab == 0) {
                addInsnAndOffset(macro + lhsNeg, joinLab);
                lhsList = insnList;
                insnList = otherIns;
                addInsnAndOffset(macro + rhsNeg, joinLab);
            } else {
                if (rhsNeg) {
                    addInsnAndOffset(macro + lhsNeg, joinLab);
                    lhsList = insnList;
                    insnList = otherIns;
                    addInsnAndOffset(macro + mcJUMP,
                                     010000 * joinLab + rhsLab);
                } else {
                    addInsnAndOffset(macro + lhsNeg, rhsLab);
                    joinLab = rhsLab;
                    lhsList = insnList;
                    insnList = otherIns;
                }
            }
        } else {
            if (rhsLab == 0) {
                if (lhsNeg) {
                    addInsnAndOffset(macro + mcJUMP,
                                     010000 * joinLab + lhsLab);
                    lhsList = insnList;
                    insnList = otherIns;
                    addInsnAndOffset(macro + rhsNeg, joinLab);
                } else {
                    lhsList = insnList;
                    insnList = otherIns;
                    addInsnAndOffset(macro + rhsNeg, lhsLab);
                    joinLab = lhsLab;
                }
            } else {
                if (lhsNeg) {
                    if (rhsNeg) {
                        addInsnAndOffset(macro + mcJUMP,
                                         010000 * joinLab + lhsLab);
                        lhsList = insnList;
                        insnList = otherIns;
                        addInsnAndOffset(macro + mcJUMP,
                                         010000 * joinLab + rhsLab);
                    } else {
                        addInsnAndOffset(macro + mcJUMP,
                                         010000 * rhsLab + lhsLab);
                        lhsList = insnList;
                        insnList = otherIns;
                        joinLab = rhsLab;
                    }
                } else {
                    lhsList = insnList;
                    insnList = otherIns;
                    joinLab = lhsLab;
                    if (rhsNeg)
                        addInsnAndOffset(macro + mcJUMP,
                                         010000 * lhsLab + rhsLab);
                    else
                        addInsnAndOffset(macro + 3,
                                         010000 * lhsLab + rhsLab);
                }
            }
        }
        insnList->regsused = allRegs.ii & ~ Bits(16);
        lhsList->tail->next = insnList->head;
        insnList->head = lhsList->head;
        insnList->ilm = ilCOND;
        insnList->payload.ii = joinLab;
        forValue = oldFVal;
    } /* genBoolAnd */


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
        if (insnList->ilm == ilCOND and insnList->payload.ii) {
            if (has(insnList->regsused, 16))
                /* Set: an OR, which genBoolAnd built by De Morgan and
                   negateCond flipped, so the embedded jumps fire on TRUE.
                   Send the fall-through to elseLab and define payload here,
                   where the then-branch starts. */
                addInsnAndOffset(macro + 2,
                                 elseLab * 010000 + insnList->payload.ii);
            else
                /* Clear: an AND, whose short-circuit jumps already fire on
                   FALSE, which is where elseLab wants them. */
                elseLab = insnList->payload.ii;
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
        bool &rhsMode = formOperator::super.back()->rhsMode;

        lhsExpr = exprToGen->expr1;
        innerNode = exprToGen->expr2;
        innerOp = innerNode->op;
        rhsExpr = innerNode->expr1;
        if (not isCheapLval(lhsExpr)) {
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

/* Peel constant terms from a dynamic array index.  Pointer subscripts do not
   reach this helper: they are lowered to pointer arithmetic and DEREF before
   code generation.  A constant-only index stays intact so genGetElt can keep
   doing compile-time bounds checking against the declared bounds. */
ExprPtr stripIdx(ExprPtr idx, int64_t &offset)
{
    offset = 0;
    while (idx->op == INTPLUS or idx->op == INTMINUS) {
        ExprPtr c = idx->expr2;
        if (c->op == GETENUM and c->vt.typ == IntegerType) {
            int64_t value = rawIntToI64(c->lit);
            offset += idx->op == INTPLUS ? value : -value;
            idx = idx->expr1;
        } else if (idx->op == INTPLUS and
                   idx->expr1->op == GETENUM and
                   idx->expr1->vt.typ == IntegerType) {
            offset += rawIntToI64(idx->expr1->lit);
            idx = idx->expr2;
        } else {
            return idx;
        }
    }
    return idx;
} /* stripIdx */

void genGetElt()
{
    int64_t l5var1z, dimCnt, curDim, l5var4z, l5var5z, l5var6z,
        l5var7z, l5var8z;
    InsnList insnCopy;
    InsnListPtr copyPtr, l5ins21z;
    Word l5var22z, l5var23z;
    bool l5var24z, l5var25z, isFlatMem;
    TPtr l5var26z;
    ilmode l5ilm28z;
    ExprPtr l5var29z, idxExpr, baseExpr;
    InsnListPtr getEltInsns[10];
    int64_t idxOffsets[10];
    ExprPtr & exprToGen = genFullExpr::super.back()->exprToGen;
    InsnList * &saved = formOperator::super.back()->saved;

    dimCnt = 0;
    baseExpr = exprToGen;
    while (baseExpr->op == GETELT)
        baseExpr = baseExpr->expr1;
    isFlatMem = baseExpr->op == GETVAR and baseExpr->id1 == flatMemVar;
    l5var29z = exprToGen;
    while (l5var29z->op == GETELT) {
        if (isFlatMem) {
            idxExpr = l5var29z->expr2;
            idxOffsets[dimCnt] = 0;
        } else {
            idxExpr = stripIdx(l5var29z->expr2, idxOffsets[dimCnt]);
        }
        (void) genFullExpr(idxExpr);
        getEltInsns[dimCnt] = insnList;
        dimCnt = dimCnt + 1;
        l5var29z = l5var29z->expr1;
    }
    (void) genFullExpr(l5var29z);
    l5ins21z = insnList;
    insnCopy = *insnList;
    copyPtr = &insnCopy;
    l5var22z.ii = freeRegs;
    for (curDim = 0; curDim < dimCnt; ++curDim)
        l5var22z.ii = l5var22z.ii & ~ getEltInsns[curDim]->regsused;
    for (curDim = dimCnt - 1; curDim >= 0; curDim--) {
        l5var26z = insnCopy.typ.rep()->base;
        l5var25z = insnCopy.typ.p.pad != 0;
        l5var7z = 0;
        l5var8z = typeSize(l5var26z);
        insnList = getEltInsns[curDim];
        l5ilm28z = insnList->ilm;
        /* A literal index was deliberately not stripped: the declared size
           still governs the compile-time check.  Only a dynamic index uses
           an effective base after its constant terms have been peeled. */
        if (l5ilm28z != ilCONST)
            l5var7z -= idxOffsets[curDim];
        if (not l5var25z)
            insnCopy.disp = insnCopy.disp - l5var8z * l5var7z;
        if (l5ilm28z == ilCONST) {
            curVal = insnList->payload;
            if (curVal.ii < l5var7z or
                insnCopy.typ.rep()->asize <= curVal.ii)
                error(29); /* errIndexOutOfBounds */
            if (l5var25z) {
                l5var4z = curVal.ii - l5var7z;
                l5var5z = insnCopy.typ.rep()->perword;
                insnCopy.regsused = insnCopy.regsused | Bits(0L);
                insnCopy.disp = l5var4z / l5var5z + insnCopy.disp;
                l5var6z = (l5var5z-1-l5var4z % l5var5z) *
                    insnCopy.typ.p.pad;
                switch (insnCopy.st) {
                case stWORD: insnCopy.shift = l5var6z;
                    break;
                case stSLICE: insnCopy.shift = insnCopy.shift + l5var6z +
                        typeBits(insnCopy.typ) - 48;
                    break;
                case stPACKED: error(errUsingVarAfterIndexingPackedArray);
                    break;
                } /* case */
                insnCopy.width = insnCopy.typ.p.pad;
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
                    if (l5var7z) {
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
                    insnCopy.width = insnCopy.typ.p.pad;
                    curVal.ii = insnCopy.width;
                    if (curVal.ii == 24)
                        curVal.ii = 7;
                    curVal.ii = shl48(curVal.ii, 24);
                    addToInsnList(allocSymtab(  /* P/00C */
                        helperNames[21] | curVal.ii)+(KVTM+I11));
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
    // needPush says the accumulator holds a value that has to be pushed
    // before another is loaded over it -- which is the same thing as "this is
    // not the first argument", and after the list the same thing again as
    // "argument 1 is on the stack" for the indirect call's tail macro.
    bool isProc, needPush, isIndir, isFortrn, isAssembler, allByRef;
    int64_t calleeFl, frameSiz, tailMacro, slot;
    bool manyArgs;
    InsnListPtr l5inl20z;
    TPtr routTyp, resTyp;
    SigPtr curSig;
};

int64_t allocGlobalObject(IdentRecPtr l6arg1z)
{
    if (l6arg1z->r1.pos == 0) {
        if ((l6arg1z->flags() & Bits(20, 21))) {
            curVal.ii = leftAlign(l6arg1z->id);
            l6arg1z->r1.pos = allocExtSymbol(extSymMask);
        } else {
            l6arg1z->r1.pos = symTabPos;
            putToSymTab(0);
        }
    }
    return l6arg1z->r1.pos;
}

genEntry::genEntry()
{
    // A struct argument at least this wide is streamed onto the stack by
    // C/LNGPAR instead of by one load per word.  The call is three
    // instructions, or four where the struct is the first argument and has to
    // load word 0 itself, whatever the width; below this the unrolled run is
    // shorter.  The width is where size turns, not speed: C/LNGPAR runs about
    // three instructions per word against the unrolled one.
    const int64_t loopArgSz = 4;
    ExprPtr & exprToGen = genFullExpr::super.back()->exprToGen;
    l5exp1z = exprToGen->expr1;
    isIndir = exprToGen->op == INDCALL;
    if (isIndir) {
        // Everything about the callee comes from the type it is reached
        // through: the result, the parameter list, and the flags that shape
        // the call.  Nothing is known about the registers the routine at the
        // other end uses, so every one of them counts as clobbered.
        calleeExp = exprToGen->expr2;
        routTyp = ptrBase(calleeExp->vt.typ);
        resTyp = routTyp.rep()->rresult;
        curSig = routTyp.rep()->rparams;
        manyArgs = argWords(curSig) >= 2;
        calleeFl = routTyp.rep()->rflags | BitRange(0,15);
    } else {
        l5idr5z = exprToGen->id2;
        resTyp = l5idr5z->typ;
        curSig = l5idr5z->sig();
        manyArgs = argWords(curSig) >= 2;
        calleeFl = l5idr5z->flags();
    }
    isProc = (resTyp == NULL);
    frameSiz = isProc ? 8 : 9;
    isFortrn = has(calleeFl, 21);
    isAssembler = has(calleeFl, 26);
    allByRef = has(calleeFl, 24);
    insnList = new InsnList;
    insnList->head = NULL;
    insnList->tail = NULL;
    insnList->typ = resTyp;
    /* The display registers the callee names are not the caller's business:
       calleeSaved is put back before the call returns.  Leaving them in
       regsused would fold them into usedRegs when the list is emitted, and
       genOneOp would then take a base pinned there out of liveRegs. */
    insnList->regsused = ((calleeFl & ~ calleeSaved) | BitRange(7,15))
                         & (BitRange(0,8)|BitRange(10,15));
    insnList->ilm = ilRVAL;
    insnList->st = stWORD;      // prepLoad reads st on every ilRVAL list
    if (isFortrn) {
        needPush = isProc;
        if (checkFortran) {
            addToInsnList(getHelperProc(25)); /* "P/MF" */
        }
    } else {
        // The first argument travels in the accumulator and the ones after it
        // are pushed.  An assembler routine reads those off the stack top and
        // has no frame of its own, so nothing is skipped for it.
        needPush = false;
        if (not isAssembler and manyArgs) {
            addToInsnList(KUTM+SP + frameSiz);
        }
    }
// (loop)
    while (l5exp1z) { /* 6574 */
        l5exp2z = l5exp1z->expr2;
        l5exp1z = l5exp1z->expr1;
        l5inl20z = insnList;
        (void) genFullExpr(l5exp2z);
        // The formal says how many argument slots the actual fills, since it
        // is the formal that laid the callee's frame out.  One wider than a
        // word claims that many, and the struct's own words go into them:
        // passed by value in the literal sense.  Each push fuses with the
        // load that follows it into an XTS, and the last word is left in the
        // accumulator, where an argument of one word would have been.
        // A formal one word wide takes an address instead of a value when the
        // value does not fit one -- which is how a file reaches a pointer
        // formal -- and FORTRAN takes an address whatever the widths.
        if (allByRef or curSig == NULL or typeSize(curSig->ptyp) == 1) {
            if (not allByRef and typeSize(insnList->typ) == 1) {
                prepLoad();
            } else {
                setAddrTo(14);
                addToInsnList(KITA+14);
            }
        } else if (typeSize(curSig->ptyp) < loopArgSz) {
            setAddrTo(14);
            for (slot = 0; slot < typeSize(curSig->ptyp); ++slot) {
                if (slot)
                    addToInsnList(macro + mcPUSH);
                addInsnAndOffset(I14 + KXTA, slot);
            }
        } else {
            // C/LNGPAR streams the words instead.  Its opening XTS pushes
            // whatever is in the accumulator, which is the previous argument
            // -- so that push is the one this loop would have prepended, and
            // word 0 is read from where the address register already points.
            // As the first argument there is nothing to push ahead of the
            // struct, so word 0 is loaded here and the register starts one
            // past it.
            slot = typeSize(curSig->ptyp) - 1;
            if (not needPush) {
                ++insnList->disp;
                setAddrTo(14);
                addInsnAndOffset(I14 + KXTA, -1);
                --slot;
            } else {
                setAddrTo(14);
                needPush = false;
            }
            addToInsnList(KVTM+I12 + getValueOrAllocSymtab(-slot));
            addToInsnList(getHelperProc(19)); /* "C/LNGPAR" */
            insnList->regsused = insnList->regsused | Bits(12, 13);
        } /* 7027 */
        if (curSig)
            curSig = curSig->next;
        if (needPush)
            prependToInsnList(macro + mcPUSH);
        needPush = true;
        if (l5inl20z->tail) {
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
        // A directly addressable pointer -- one whose whole insnList is a
        // deferred address -- is reached by WTC, which puts the entry address
        // in C for the VJM and touches neither the accumulator, where
        // argument 1 is sitting, nor the mode register.
        // Any other pointer has to be loaded, and the load wants the
        // accumulator, so it is marshalled like one more argument: the push
        // takes argument 1 out of the accumulator and fuses with the load
        // that follows into a single XTS.  STI then does both halves of the
        // call at once, taking the address into M14 and popping argument 1
        // back.  With no arguments there is nothing to pop and ATI serves.
        l5inl20z = insnList;
        (void) genFullExpr(calleeExp);
        if (insnList->head or insnList->ilm != ilLVAL
            or insnList->st != stWORD or insnList->addrmd == 15) {
            prepLoad();
            if (needPush)
                prependToInsnList(macro + mcPUSH);
            tailMacro = mcACC2ADDR - needPush;
        } else {
            curInsnTemplate = InsnTemp[WTC];
            prepLoad();
            curInsnTemplate = InsnTemp[XTA];
            tailMacro = 0;
        }
        if (l5inl20z->tail) {
            l5inl20z->tail->next = insnList->head;
            insnList->head = l5inl20z->head;
        }
        insnList->regsused = insnList->regsused | l5inl20z->regsused;
        if (tailMacro)
            addToInsnList(macro + tailMacro);
        addToInsnList(KVJM+I13);
    } else {
        addToInsnList(allocGlobalObject(l5idr5z) + (KVJM+I13));
    } /* 7132 */
    insnList->tail->mode = 2;
    /* Nothing follows the call: the display registers a callee disturbs are
       the ones its own entry helper saved in its frame, and C/E puts them
       back.  A frameless routine has no helper to do that, which is why one
       that uses a display register does not get to stay frameless. */
    // (not isAssembler) and (isIndir or [20,21]*calleeFl)
    if (not isAssembler
        and (isIndir or ((Bits(20, 21) & calleeFl) != Bits()))) {
        addToInsnList(KVTM+040074001);
    }
    /* Registers 2..6 are callee-saved, so a pinned base in one of them already
       survives the call.  Reload only a spilled base in a caller-clobbered
       fallback register (normally M14).  WTC takes the saved address from the
       slot into C, VTM then lands it in the register; neither touches the
       accumulator, which may hold a function result. */
    l5exp2z = pinList;
    while (l5exp2z) {
        if (l5exp2z->vt.typ.p.psize
            and (6 < l5exp2z->vt.typ.p.pad)
            and (Bits(l5exp2z->vt.typ.p.pad) & calleeFl)) {
            addInsnAndOffset(curFrameRegTemplate + KWTC,
                             l5exp2z->vt.typ.p.psize - 1);
            addToInsnList(KVTM + indexreg[l5exp2z->vt.typ.p.pad]);
        }
        l5exp2z = l5exp2z->expr1;
    }
    usedRegs = (usedRegs | (calleeFl & ~ calleeSaved)) & BitRange(1,15);
    if (isFortrn) {
        if (not checkFortran)
            addToInsnList(KNTR+7);
        else
            addToInsnList(getHelperProc(26));    /* "P/FM" */
        insnList->tail->mode = 2;
    } /* 7226 */
    // NB: no `else` here -- a non-Fortran function returns
    // its value in ACC, so there is no `KXTA+SP` reload of the result.
    if (not isProc) {
        insnList->typ = resTyp;
        insnList->regsused = insnList->regsused | Bits(0L);
        insnList->ilm = ilRVAL;
        liveRegs = liveRegs & ~ (calleeFl & ~ calleeSaved);
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
        if (lhsIns->tail)
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
    int64_t mode, size;

    int64_t &scratch3 = formOperator::super.back()->scratch3;
    Operator &curOP = genFullExpr::super.back()->curOP;
    int64_t &nextInsn = formOperator::super.back()->nextInsn;
    int64_t &work = genFullExpr::super.back()->work;
    TPtr &l2typ13z = programme::super.back()->l2typ13z;

    scratch3 = curOP - NEOP;
    negate = scratch3 & 1;
    if (negate)
        scratch3 = scratch3 - 1;
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
        work = scratch3 == 4 ? 26 : 23 + (scratch3 >> 1);
        addToInsnList(getHelperProc(work)); /* P/EQ */
        insnList->ilm = ilRVAL;
        negate = not negate;
    } else if (scratch3 == 0) {
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
        negateCond();
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
    /* Positional, since g++ takes no designated array initialisers: the
       entries run in Operator order from SHLEFT, and everything past
       ASSIGNOP is the quiet default. */
    static int64_t opToInsn[47] = {
        /*SHLEFT*/ 28,   /*SHRIGHT*/  29,
        /*SETAND*/ KAAX, /*SETXOR*/   KAEX, /*SETOR*/ KAOX,
        /*MUL*/    KMUL, /*IMULOP*/   KMUL,
        /*RDIVOP*/ KDIV, /*IDIVOP*/   8,           /* P/DI */
        /*IMODOP*/ 6,                              /* P/MD */
        /*PLUSOP*/ KADD, /*INTPLUS*/  KADD,
        /*MINUSOP*/ KSUB, /*INTMINUS*/ KSUB };
    static OpFlg opFlags[47] = {
        /*SHLEFT*/  opfSHIFT, /*SHRIGHT*/  opfSHIFT,
        /*SETAND*/  opfCOMM,  /*SETXOR*/   opfCOMM, /*SETOR*/ opfCOMM,
        /*MUL*/     opfCOMM,  /*IMULOP*/   opfMULMSK,
        /*RDIVOP*/  opfCOMM,  /*IDIVOP*/   opfDIV,
        /*IMODOP*/  opfMOD,
        /*PLUSOP*/  opfCOMM,  /*INTPLUS*/  opfCOMM,
        /*MINUSOP*/ opfCOMM,  /*INTMINUS*/ opfCOMM,
        /*ANDOP*/   opfAND,   /*OROP*/     opfOR,
        /*NEOP*/    opfCOMM,  /*EQOP*/     opfCOMM,
        /*LTOP*/    opfCOMM,  /*GEOP*/     opfCOMM, /*GTOP*/ opfCOMM,
        /*LEOP*/    opfCOMM,
        /*CONDOP*/  opfCOMM,  /*ALTERN*/   opfCOMM,
        /*INCROP*/  opfCOMM,  /*DECROP*/   opfCOMM,
        /*ASSIGNOP*/ opfASSN };
    int64_t &scratch3 = formOperator::super.back()->scratch3;
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
             Bits(GTOP) | Bits(LEOP)), curOP)) {
            genComparison();
        } else { /* 7625: a foldable op with two constant operands is already
                    folded to GETENUM at construction (mkExprFold), so only the
                    non-constant codegen path remains here. */
                scratch3 = opToMode[curOP];
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
                /* Division by a constant, inline.  The reciprocal is a real
                   in [0.5,1) and KMUL keeps the high half of the product,
                   which is a shift by 40; ASN takes the rest of it.  No sign
                   arises anywhere, because only an unsigned operand and a
                   divisor above zero are admitted here -- either operand
                   unsigned asks for this, the way either asks IMULOP for the
                   unsigned multiply, and everything else takes C/DI or C/MD.

                   The signed sequences cost five instructions against these
                   two, which is too much to spend inline; a signed divide
                   goes to C/DI instead.  They are written out here because
                   they are what truncation costs if it is ever wanted, and
                   they are also where a negative divisor would belong --
                   there the magnitude would divide and one closing negation
                   would carry the divisor's sign, which is meaningless while
                   the dividend is unsigned.  KMUL's high half is an
                   arithmetic shift and carries the sign, ASN does not, so the
                   residual shift becomes a second multiply by an exact
                   negative power of two, and what it leaves is the floor.
                   Truncation is one step up from that wherever the dividend
                   is negative, and which step depends on whether the
                   reciprocal is exact:

                     |c| = 2^k   the reciprocal is exact, so the floor is a
                                 floor and the dividend is biased by |c|-1
                                 where it is negative.  The bias also keeps it
                                 off the range minimum, whose magnitude has
                                 nowhere to go.

                         ATX SP+1 ; MUL 2^-40 ; AAX |c|-1 ; ADD SP+1 ; MUL 2^-k

                     otherwise   the reciprocal is rounded up, and that
                                 overshoot drops a negative quotient exactly
                                 one below its truncation, so the sign is
                                 added back at the end.  The floor is negative
                                 exactly where the dividend was, so it is its
                                 own sign mask and the dividend never has to
                                 be kept.

                         MUL magic ; MUL 2^-k ; ATX SP+1 ; MUL 2^-40 ; RSUB SP+1

                   The sign mask either shape wants is one more multiply:
                   2^-40 keeps only the sign bit, giving 0 or -1.  Nothing in
                   either branches, and the word above the stack top is the
                   only scratch. */
                case opfMOD:
                case opfDIV:
                    /* recip says which of three things the divisor is: 0 for
                       one not to be divided by inline, 1 for a power of two,
                       where the shift alone divides and no reciprocal is
                       wanted, and otherwise the mantissa of 1.0/c, which is
                       ceil(2^(40+k)/c) with k the index of c's top bit.  That
                       mantissa is never below 2^39, so it cannot be taken for
                       either of the other two.

                       The division rounds either way and the product says
                       which: below 1.0 the mantissa came back short and is
                       stepped up to the ceiling.  That is the one place the
                       two compilers meet by different routes, this dividing
                       in a host double and work.p2c on the machine.

                       Rounding up is what makes the reciprocal usable and
                       also what limits it: the mantissa overshoots
                       2^(40+k)/c by some e, and the quotient it yields is one
                       too high once a*e reaches 2^(40+k).  A dividend stops
                       below 2^40, so the sequence holds exactly while
                       e <= 2^k: the largest product it can then face is
                       (2^40 - 1) * 2^k, which is 2^k short of reaching.
                       Nothing reconstructs e -- m*c is 2^(40+k) + e and 40+k
                       is at least 41, so the low half of that product is e
                       alone.  The target keeps that half for free, an
                       unsigned multiply being what it does; a host word has
                       no room for the whole product, so the width is spelled
                       out here instead. */
                    recip = 0;
                    if (arg2Const and rawIntToI64(arg2Val) > 0 and
                        (exprToGen->expr1->vt.typ == UnsignedType or
                         exprToGen->expr2->vt.typ == UnsignedType)) {
                        int64_t c = rawIntToI64(arg2Val);
                        if (card(c) == 1)
                            recip = 1;
                        else {
                            curVal.r = 1.0 / (double)c;
                            recip = (double)curVal.r * c < 1.0;
                            curVal.ii = (curVal.ii & BitRange(7, 47)) + recip;
                            recip = 0;
                            if ((int64_t)(((__int128)curVal.ii * c)
                                          & ((1L << 40) - 1))
                                <= (1L << (47 - minel(c))))
                                recip = curVal.ii;
                        }
                    }
                    if (recip == 0) {
                        genHelper();
                        break;
                    }
                    prepLoad();
                    scratch3 = 1;
                    if (curOP == IMODOP) {
                        if (recip == 1) {
                            /* what is left over a power of two is a mask
                               away, and wants no quotient at all */
                            curVal.ii = arg2Val.ii - 1;
                            addToInsnList(KAAX+I8 + getFCSToffset());
                            scratch3 = 0;
                            break;
                        }
                        /* the dividend is wanted again once the quotient is
                           in, so it goes on the stack first */
                        addToInsnList(macro + mcPUSH);
                    }
                    if (recip != 1) {
                        curVal.ii = recip | Bits(0);
                        addToInsnList(KMUL+I8 + getFCSToffset());
                    }
                    addToInsnList(ASN64 + 47 - minel(arg2Val.ii));
                    if (curOP == IMODOP) {
                        /* what is left is the dividend less the quotient
                           times the divisor */
                        insnList->tail->mode = 1;
                        curVal.ii = arg2Val.ii | Bits(0);
                        addToInsnList(KMUL+I8 + getFCSToffset());
                        addToInsnList(KYTA+64);
                        addToInsnList(KRSUB+SP);
                    }
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
                    /* The unsigned product takes what the m- mode takes:
                       the low half of it, with no P/MI fixup. */
                    addToInsnList(exprToGen->vt.typ == UnsignedType
                                  ? KYTA+64 : macro + mcMULTI);
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
                insnList->tail->mode = scratch3;
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
                /* A static keeps the routine template in pck.offset for
                   lexical lookup; its generated address alone uses M1. */
                insnList->payload.ii = curIdRec->pck.cl == STATICID ?
                                       frameRegTemplate : curIdRec->pck.offset;
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
                if (curIdRec->fpk.pckfield) {
                    switch (insnList->st) {
                    case stWORD:
                        insnList->shift = curIdRec->fpk.shift;
                        break;
                    case stSLICE: {
                        insnList->shift = insnList->shift + curIdRec->fpk.shift;
                        if (not curIdRec->uptype().rep()->lsbord)
                            insnList->shift = insnList->shift + typeBits(curIdRec->uptype()) - 48;
                    } break;
                    case stPACKED:
                        if (not rhsMode)
                            error(errUsingVarAfterIndexingPackedArray);
                        else {
                            startLVal();
                            insnList->shift = curIdRec->fpk.shift;
                        }
                        break;
                    } /* 10235*/
                    insnList->width = curIdRec->fpk.width;
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
        } else if (curOP == ADDROF) {
            /* An lvalue's address as a value: setAddrTo lands the address in
               the tag register and ITA turns it into an integer word, as just
               above for a routine's entry. */
            genFullExpr(exprToGen->expr1);
            if (insnList->ilm == ilCONST and
                exprToGen->expr1->vt.typ.p.pk != kindArray)
                error(201);
            setAddrTo(14);
            addToInsnList(KITA+14);
            insnList->ilm = ilRVAL;
            insnList->regsused = insnList->regsused | Bits(0);
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
                    scratch3 = 3;
                    goto L10122;
                } else if (curOP == TOINT) {
                    /* Real to integer truncates toward zero, in C/TR
                       (libc); the helper returns with the machine in
                       integer mode. */
                    scratch3 = 2;
                    addToInsnList(getHelperProc(15)); /* "C/TR" */
                    goto L10122;
                } else if (curOP == BITNEGOP) {
                    addToInsnList(KAEX+ALLONES);
                    scratch3 = 1;
                    goto L10122;
                } else {
                    addToInsnList(KAVX+ALLONES);
                    if (curOP == RNEGOP)
                        scratch3 = 3;
                    else
                        scratch3 = 1;
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
                    addToInsnList(getHelperProc(13)); /*"P/NW"*/
                    insnList->ilm = ilRVAL;
                    insnList->regsused = insnList->regsused | Bits(0);
                    insnList->typ = exprToGen->vt.typ;
                    return;
                default:
                    break;
                } /* 10546 */
                insnList->payload = arg1Val;
            } else {
                prepLoad();
                if (work == fnCARD) {
                    scratch3 = 0;
                } else if (work == fnABS)
                    scratch3 = 3;
                else {
                    scratch3 = 1;
                }
                addToInsnList(funcInsn[work]);
                goto L10122;
            }
        } else { /* 10621 */
            if (curOP == NOOP) {
                curVal.ii = exprToGen->vt.typ.p.pad;
                /* A spilled base has a frame slot to come back from, so it
                   needs no liveness test. */
                if (exprToGen->vt.typ.p.psize
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
                    /* Registers 2..6 hold one pinned base each -- SETREG takes
                       the register out of freeRegs -- so M[pad] still holds
                       this one and the descriptor above is the whole story.
                       Above 6 there is only the shared fallback, which every
                       base that ran out of registers is given: the frame slot
                       is what holds this one, and it has to come back before
                       each use, not just after a call.  WTC then VTM leave the
                       accumulator alone. */
                    if (6 < curVal.ii) {
                        addInsnAndOffset(curFrameRegTemplate + KWTC,
                                         exprToGen->vt.typ.p.psize - 1);
                        addToInsnList(KVTM + indexreg[curVal.ii]);
                    }
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
        formAndAlign(getHelperProc(17)); /*"FCLOSE"*/
    };

    form2Insn(KITS+13, KATX+SP);
    if (inputFile) {
        fcloseFile(inputFile);
        form1Insn(KXTA+SP);  // remove FCLOSE's stacked FCB argument
    }
    if (outputFile)
        fcloseFile(outputFile);
    form1Insn(getHelperProc(18)/*"P/IT"*/ + (KUJ-KVJM-I13));
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
//  if (op == DOIT)                             /* see dropPostFixup */
//      curExpr = dropPostFixup(curExpr);
    if (op != FORMOP)
        (void) genFullExpr(curExpr);
    switch (op) {
    case gen0:
        break; /* placeholder OpGen slot, never passed */
    case DOIT:
        genOneOp();
        break;
    case SETREG: {
        if (insnList->head == NULL)
            scratch3 = 0;
        else if (insnList->head == insnList->tail)
            scratch3 = 1;
        else
            scratch3 = 2;
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
            if (scratch3 == 0)  {
                l3int2z = 14;
            } else {
                l3var10z.ii = auxRegs & freeRegs;
                if (l3var10z.ii) {
                    l3int2z = minel(l3var10z.ii);
                } else {
                    l3int2z = 14;
                }
                if (scratch3 != 1) {
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
    case SETREG12: {
        /* A bit slice has no address of its own to put in a register. */
        if (insnList->st != stWORD)
            error(errVarTooComplex);
        (void) setAddrTo(12);
        genOneOp();
    } break;
    case LOAD: {
        prepLoad();
        genOneOp();
    } break;
    case BRANCH:
        noTarget = jumpTarget == 0;
        scratch3 = jumpTarget;
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
            // GETELT..ALNUM is the set whose value reaches the accumulator
            // through a load, so omega already means "is it zero"; anything
            // else needs an AEX to set it, since the omega standing after
            // arithmetic is a sign test.  Look through NOTOP first: a
            // do-while wraps its condition in one to branch back on true, and
            // NOTOP only flips the polarity bit -- what the accumulator holds
            // is decided by what is under it.
            if (not omegaIsZeroTest(curExpr))
                addToInsnList(KAEX);
            direction = has(insnList->regsused, 16);
            if ((insnList->ilm == ilCOND) and
                (insnList->payload.ii)) {
                genOneOp();
                if (direction) {
                    if (noTarget)
                        formJump(scratch3);
                    else
                        form1Insn(InsnTemp[UJ] + scratch3);
                    fixup(0, jumpTarget);
                    jumpTarget = scratch3;
                } else {
                    if (not noTarget) {
                        if (not putLeft)
                            padToLeft();
                        fixup(scratch3, jumpTarget);
                    }
                }
            } else {
                if (insnList->ilm == ilLVAL) {
                    forValue = false;
                    prepLoad();
                    forValue = true;
                    // This load is what sets the omega the jump below reads,
                    // so the peephole must not drop it as a repeat of the
                    // store before it.  The tail is the right instruction in
                    // either mode: the load itself for a whole word, and
                    // genSliceExtract's closing AAX for a slice, which is
                    // logical and sets omega correctly anyway.
                    insnList->tail->mode = insnList->tail->mode | mdNoElide;
                }
                genOneOp();
                if (direction)
                    nextInsn = InsnTemp[U1A];
                else
                    nextInsn = InsnTemp[UZA];
                if (noTarget) {
                    jumpType = nextInsn;
                    formJump(scratch3);
                    jumpType = InsnTemp[UJ];
                    jumpTarget = scratch3;
                } else {
                    form1Insn(nextInsn + scratch3);
                }
            }
        }
        break; /* CONDJUMP */
    } /* case */
} /* formOperator */

/* Extract the value of a constant expression into curVal.  With folding at
   construction a constant expression is already a GETENUM node, so read it
   directly instead of lowering it through genFullExpr, which saves an insnList
   allocation per constant (case labels, const decls, array sizes, besm). */
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
        while (hashTravPtr and hashTravPtr->id != curIdent)
            hashTravPtr = hashTravPtr->next();
        if (hashTravPtr and hashTravPtr->pck.cl == TYPEID) {
            SY = TYPESY;
            symType = hashTravPtr->typ;
        }
    }
} /* markTypeSym */

// File scope (not nested in parseTypeRef): makeArrayType and the shared
// declarator parser (below) both need to construct/consume array sizes,
// and a struct type nested inside a function is only visible within that
// function's own scope in this codebase's Pascal-mirroring conventions.
struct rangeRec { int64_t asize; };
typedef rangeRec rangeList[20];

TPtr makeArrayType(int64_t asize, TPtr elem, bool makePacked)
{
    int64_t l3int22z, numBits;
    int64_t sizeVal, bitsVal, perwordVal, pcksizeVal;
    TPtr arrayType{};

    l3int22z = typeBits(elem);
    /* Nothing wider than half a word packs, so the request is refused here
       and the type records what it was given. */
    if (24 < l3int22z)
        makePacked = false;
    InternRec * icand = internHead;
    while (icand) {
        arrayType = icand->ityp;
        if (arrayType.p.pk == kindArray and
            arrayType.rep()->base == elem and
            arrayType.rep()->asize == asize and
            ((arrayType.p.pad != 0) == makePacked))
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
        l3int22z = asize * pcksizeVal;
        if (l3int22z % 48 == 0)
            numBits = 0;
        else
            numBits = 1;
        sizeVal = l3int22z / 48 + numBits;
        if (sizeVal == 1)
            bitsVal = l3int22z;
    } else {
        sizeVal = asize * typeSize(elem);
        curVal.ii = typeSize(elem);
        curVal.ii = (curVal.ii & BitRange(7,47)) | Bits(0);
        perwordVal = KMUL+ I8 + getFCSToffset();
    }
    arrayType.setRep(besm6_alloc_record<Types>(offsetof(Types, szArray)));
    arrayType.rep()->asize = asize;
    arrayType.rep()->base = elem;
    arrayType.rep()->perword = perwordVal;
    arrayType.p.psize = sizeVal;
    arrayType.p.bits = bitsVal;
    arrayType.p.pad = pcksizeVal;
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
    typedef pair pair7[7];
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
        curEnum = besm6_alloc_record<IdentRec>(offsetof(IdentRec, szBase));
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
// parseGroupedDecls; parseArrSz's full definition comes later, hence
// the forward declaration.
void parseArrSz(int64_t & asize);
void parseConstDeclValue(TPtr declared, TPtr &typ, Word &value);
void constExpr();

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

// Set by a parameter list around its declarator, and there alone, where
// 'int q[]' is legal because the extent is discarded when the parameter
// becomes a pointer.  nameOptional, set at the same two places, will not do:
// readTypeName sets that as well, and a type name with no extent --
// 'sizeof(int[])' -- is as wrong here as it is in C.
bool noExtentOk = false;

// Words placed by the initializer just parsed, which is what sizes an array
// whose declarator left its extent out.
int64_t initWords = 0;

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
             (curIdent == toText("*INPUT*") || curIdent == toText("*OUTPUT*"))) */;
        d.foundRec = hashTravPtr;
        inSymbol();
    } else if (nameOptional and has(Bits(RPAREN, COMMA, LBRACK), SY)) {
        // Abstract declarator: a formal parameter's name is optional
        // ('int', 'int *', 'int [3]').  work.p2c's curDeclarator is a
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
            rangeRec r{};
            parseArrSz(r.asize);
            checkSymAndRead(RBRACK);
            ops.push_back({opArray, NULL, r});
        }
    }
}

// packedFlag mirrors parseTypeRef's own array-suffix handling ('TYPE
// [size]', its curDim==1 case): only the outermost array dimension of
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
            d.type = makeArrayType(it->range.asize, d.type,
                                   packedFlag and (&*it == &ops.front()));
            d.ptrOnly = false;
        }
        firstOp = false;
    }
    return d;
}

// A type name: a type-spec and an abstract declarator, the pair a parameter
// list takes, so `char *`, `int **`, `int (*)(char)` and `int[4]` all name
// a type.  A cast reads one, and so do sizeof and offsetof.
// Entered with SY on the TYPESY that opens the name; leaves SY on the token
// that follows it, which the caller checks -- ')' for a cast or sizeof, ',' or
// ')' for offsetof.
// nameOptional is restored rather than cleared: parseArrSz evaluates a
// constant bound where it stands, so a type name can be read while an
// enclosing declarator is still open.
TPtr readTypeName()
{
    TPtr nameBase = symType;
    inSymbol();
    bool wasOptional = nameOptional;
    nameOptional = true;
    Declarator d = parseOneDeclarator(nameBase);
    nameOptional = wasOptional;
    return d.type;
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
            noExtentOk = true;
            Declarator d = parseOneDeclarator(paramType, packedFlag);
            nameOptional = false;
            noExtentOk = false;
            if (d.type.p.pk == kindArray)
                d.type = getPtrType(d.type.rep()->base);
            cur = new SigRec;
            cur->s1.pclass = VARID;
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
                       std::function<void(Declarator&)> reg,
                       bool fieldWidths = false)
{
    TPtr baseTy{};
    // Named (not a temporary) so isPacked is still readable after the
    // type-spec is parsed, to apply to each declarator's own array
    // suffix below (parseTypeRef's own '__packed TYPE[size]' form
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
        // C spelling: a record bit width follows this declarator, so each
        // member in a comma group may choose its own.  The field keeps the
        // narrow integer type used by the existing prefix spelling.
        if (fieldWidths and SY == COLON) {
            parseTypeRef::super.back()->isPacked = true;
            bool fieldIsInt = isIntTyp(d.type);
            if (not fieldIsInt)
                error(62); /* errIntegerNeeded */
            inSymbol();
            if (SY != INTCONST) {
                error(errNumberTooLarge);
            } else {
                int64_t fieldWidth = curToken.ii;
                inSymbol();
                if (fieldIsInt)
                    d.type = mkIntScl(fieldWidth);
            }
        }
        if (d.name)
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
            (c.pairs[0].first < mx.pairs[0].first));
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
        fld->fpk.width = fieldWidth;
        if (fieldWidth != 48) {
            for (pairIdx = 0; pairIdx < cases.count; ++pairIdx) {
L11523:         curSlot = &cases.pairs[pairIdx];
                if (curSlot->first >= fieldWidth) {
                    fld->fpk.shift = 48 - curSlot->first;
                    fld->pck.offset = curSlot->second;
                    if (not lsbOrder)
                        fld->fpk.shift = 48 - fld->fpk.width - fld->fpk.shift;
                    curSlot->first = curSlot->first - fieldWidth;
                    if (curSlot->first == 0) {
                        cases.count = cases.count - 1;
                        cases.pairs[pairIdx] = cases.pairs[cases.count];
                    }
                    goto L11622;
                }
            }
            if (cases.count != 7) {
                pairIdx = cases.count;
                cases.count = cases.count + 1;
            } else {
                minFirst = 48;
                for (scanIdx = 0; scanIdx < 7; ++scanIdx) {
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
    fld->fpk.pckfield = false;
    fld->pck.offset = cases.size;
    cases.size = cases.size + typeSize(fldType);
L11622:
    if (PASINFOR.listMode == 3) {
        printf("%16c", ' ');
        if (fld->fpk.pckfield)
            printf("PACKED");
        printf(" FIELD ");
        printTextWord(fld->id);
        printf(".OFFSET=%05loB", (long)fld->pck.offset);
        if (fld->fpk.pckfield) {
            printf(".<<=SHIFT=%2ld. WIDTH=%2ld BITS", (long)fld->fpk.shift,
                   (long)fld->fpk.width);
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
    lookup2 = lookupMode = lookField;
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
                    if (d.foundRec)
                        error(errIdentAlreadyDefined);
                    curEnum = besm6_alloc_record<IdentRec>(
                        offsetof(IdentRec, szField));
                    curEnum->id = d.name;
                    curEnum->pck.nidx = ord(fieldHash[d.bucket]);
                    curEnum->pck.cl = FIELDID;
                    curEnum->uptype() = curType;
                    curEnum->fpk.pckfield = isPacked;
                    fieldHash[d.bucket] = curEnum;
                    if (curType.rep()->fields == NULL)
                        curType.rep()->fields = curEnum;
                    else
                        l3idr31z->list() = curEnum;
                    l3idr31z = curEnum;
                    packOneField(curEnum, d.type);
                    if (isUnion and unionMemberBigger(cases2, cases, isPacked))
                        cases2 = cases;
                }, true);
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
            rectype.p.bits = 48 - cases.pairs[0].first;
        else
            rectype.p.bits = 48;
        prevField = rectype.rep()->fields;
        while (prevField) {
            prevField->uptype() = rectype;
            prevField = prevField->list();
        }
    }
    lookup2 = savedLookup2;
    checkSymAndRead(ENDSY);
} /* parseRecordDecl */

// parseArrSz's definition lives past the Statement struct, next to the
// declaration-time constant-value parser.  Only the forward declaration
// (above) is visible here.

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
            if (isDefined || curIdent == toText("*INPUT*") || curIdent == toText("*OUTPUT*"))
                error(errIdentAlreadyDefined);
            enumName = curIdent;
            enumBucket = bucket;
            inSymbol();
            // Optional '= constExpr': an explicit enumerator value.  Later
            // enumerators auto-increment from it, through the shared
            // const-expression path.
            if (charClass == ASSIGNOP) {
                TPtr &ceTyp = programme::super.back()->ceTyp;
                Word &ceVal = programme::super.back()->ceVal;
                inSymbol();
                constExpr();
                if (ceTyp.rep() and ceTyp.p.pk == kindScalar)
                    nextEnum = ceVal.ii;
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
            /* work.p2c increments an int and the machine keeps it one; here
               nextEnum is a 41-bit word in an int64_t, so the carry out of
               the sign bit has to be dropped rather than land in what would
               be the exponent field. */
            nextEnum = (nextEnum + 1) & INT41_MASK;
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
            // start = -1 suppresses the name table (the value then prints as
            // an integer): explicit values can be sparse or negative,
            // so there is no dense name array to index by value.
            curType.rep()->start = hasExplicit ? -1 : 0;
            curType.p.psize = 1;
            // Explicit values may be sparse or negative -> a full 48-bit
            // value field (like int); else the packed minimum.
            curType.p.bits = hasExplicit ? 48
                             : (48 - minel((span - 1) & ((1L << 48) - 1)));
            curType.p.pk = kindScalar;
            curEnum = curType.rep()->enums;
            while (curEnum) {
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
        // A run of type specifiers is one type: this machine has one integer,
        // so 'unsigned', 'long long', 'short int' and the rest all name it.
        // A later specifier that is not int wins, making 'unsigned char' a
        // char.  Only a built-in type keyword is taken -- a typedef name is
        // raised to TYPESY by markTypeSym, which does not run here.
        while (isIntTyp(curType) and SY == TYPESY) {
            curType = symType;
            inSymbol();
        }
        if (isIntTyp(curType) and SY == COLON) {
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
            if (SY == IDENT and curIdent == toText("**LSB")) { // '__lsb': '_' shares the code of '*'
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
        parseArrSz(curRange.asize);
        if (rangeCnt == 20) {
            error(errVarTooComplex);
        } else {
            ranges[rangeCnt] = curRange;
            rangeCnt = rangeCnt + 1;
        }
        checkSymAndRead(RBRACK);
    }
    curType = tempType;
    for (curDim = rangeCnt - 1; curDim >= 0; --curDim) {
        curType = makeArrayType(ranges[curDim].asize, curType,
                                isPacked and (curDim == 0));
    }
    if (rangeCnt)
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
        while (l3var1z) {
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
    formAndAlign(getHelperProc(16)); /*"FOPEN"*/
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
        l3arg1z = procName->r1.pos;
        frame.ii = moduleOffset - 040000;
        if (l3arg1z)
            symTab[l3arg1z - 074000] = 041000000 + (frame.ii & halfWord);
        procName->r1.pos = moduleOffset;
        l3arg1z = argWords(procName->sig());
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
            frame.ii = 8;
        } else {
            frame.ii = 9;
        }
        if (l3var3z)
            form2Insn((KVTM+I14) + l3arg1z + (frame.ii - 8) * 01000,
                      getHelperProc(27 /*"P/NN"*/) - 010000000);
        if (1 < l3arg1z) {
            frame.ii = getValueOrAllocSymtab(-(frame.ii+l3arg1z));
        }
        l3int1z = getHelperProc(curProcNesting) - (-04000000);
        if (l3arg1z == 1) {
            form1Insn((KATX+SP) + frame.ii);
        } else if (l3arg1z) {
            form2Insn(KATX+SP, (KUTM+SP) + frame.ii);
        }
        formAndAlign(l3int1z);
        savedObjIdx = objBufIdx;
        if (curProcNesting != 1)
            form1Insn(0);
        if (l3var3z)
            form1Insn(KVTM+I8+074001);
        if (curProcNesting == 1) {
            if (inputFile)
                fopenFile(inputFile, fileForInput);
            if (outputFile)
                fopenFile(outputFile, fileForOutput);
            curVal.ii = fileExit;
            fixup(2, 49);
        }
        if (curProcNesting == 1) {
            if (heapCallsCnt and
                heapSize == 0)
                error(65 /*errCannotHaveK0AndNew*/);
            l3var3z = (heapSize == 0) or
                ((heapCallsCnt == 0) and (heapSize == 100));
            if (heapSize == 100)
                heapSize = 4;
            if (not l3var3z) {
                form2Insn(KVTM+I14+getValueOrAllocSymtab(heapSize*02000),
                          getHelperProc(10 /*"P/GD"*/));
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

    labIdx = numLabTop - 1;
    while (labIdx >= labFence and numLabs[labIdx].id != curToken)
        labIdx = labIdx - 1;
    if (labIdx < labFence) {
        if (numLabTop >= 20) {
            error(50); /* errSymbolTableOverflow */
            return;
        }
        numLabs[numLabTop].id = curToken;
        numLabs[numLabTop].offset = 0;
        numLabs[numLabTop].line = lineCnt;
        numLabs[numLabTop].defined = false;
        labIdx = numLabTop;
        numLabTop = numLabTop + 1;
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
            // symTab[offset-074000] := [24,29] + curVal.ii * O77777
            curVal.ii = moduleOffset - 040000;
            symTab[numLabs[labIdx].offset - 074000] =
                041000000 + (curVal.ii & 077777);
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

/* An array used as a value is the address of its first element, as in C.
   Anything else comes back as it stands, so a call site may apply this
   unconditionally.  The shapes are the ones the '&' arm builds for '&a[0]':
   a char array yields a byte index into flat memory, six bytes to a word,
   the unpacked case pointing at the rightmost byte of the element's own
   word.  A string literal is an array constant whose payload is either its
   word or its FCST offset; setAddrTo makes either form addressable.  Any other
   array that is not an lvalue -- the result of a cast -- has no address to
   take and is left for the caller's type check to refuse. */
ExprPtr decayArray(ExprPtr e)
{
    if (e->vt.typ.p.pk != kindArray or
       (not has(lvalOpSet, e->op) and e->op != GETENUM))
        return e;
    if (not isCharArray(e->vt.typ))
        return mkRef(e, getPtrType(e->vt.typ.rep()->base));
    ExprPtr addr = mkExpr(IMULOP, charPtrType, mkRef(e, IntegerType),
                          mkIntLit(6));
    if (e->vt.typ.p.pad)
        return addr;
    return mkExpr(INTPLUS, charPtrType, addr, mkIntLit(5));
} /* decayArray */

void expression();
void parseCallArgs(IdentRecPtr subroutine, ExprPtr callee);

/* Recovery for an error that leaves nothing to build a value from: skip to a
   token a statement can end on, and stand an undefined-variable node in for
   the value.  The caller reports the error and returns, so the expression
   finishes with a well-typed placeholder and the parse carries on from the
   terminator. */
void badExpr()
{
    skip(skipToSet | statEndSys);
    curExpr = uVarPtr;
} /* badExpr */

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
        else if (l4step6z) {
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
        curExpr = mkRef(reinterpret_cast<ExprPtr>(curExpr->id1->value()),
                        curExpr->vt.typ);
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
    SigPtr curSig = NULL;
    TPtr routTyp{}, formType{};

    if (callee == NULL) {
        if (subroutine->typ != voidType)
            liveRegs = liveRegs & ~ (subroutine->flags() & ~ calleeSaved);
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
                curSig = subroutine->sig();
            else
                curSig = routTyp.rep()->rparams;
            if (curSig == NULL) {
                inSymbol();
                if (SY != RPAREN) {
                    error(errTooManyArguments);
                    badExpr();
                    return;
                }
                curExpr = callExpr;
                inSymbol();
                return;
            }
        }
        do {
            if (noArgs) {
                tooMany = curSig == NULL;
                if (not tooMany)
                    formType = curSig->ptyp;
                if (tooMany) {
                    error(errTooManyArguments);
                    badExpr();
                    return;
                }
            }
            expression();
            if (noArgs) {
                /* A pointer formal takes an array actual by decay, as in C. */
                if (formType.p.pk == kindPtr)
                    curExpr = decayArray(curExpr);
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
            if (noArgs)
                curSig = curSig->next;
        } while (SY == COMMA);
        if ((SY != RPAREN) or (noArgs and curSig))
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

    /* An array in an arithmetic context is the address of its first element,
       as in C, whatever stands on the other side.  A decayed char array is a
       char pointer, so it takes the byte-unit branch just below, and
       decayArray leaves anything that is not an array as it is. */
    leftExpr = decayArray(leftExpr);
    arg1Type = leftExpr->vt.typ;
    curExpr = decayArray(curExpr);
    arg2Type = curExpr->vt.typ;
    k1 = (Kind)arg1Type.p.pk;
    k2 = (Kind)arg2Type.p.pk;
    /* A char pointer is a byte index into flat memory: the byte is already
       its unit, so a count applies as it stands, and either operand may be
       the pointer.  The operands stay in source order, which an op-assign
       depends on -- it drops this node's expr1 and takes expr2 as the
       right-hand side. */
    if ((isCharPtr(arg1Type) and typeCheck(IntegerType, arg2Type)) or
        (isCharPtr(arg2Type) and typeCheck(IntegerType, arg1Type))) {
        curExpr = mkExpr(Operator(oper + (oper != IMODOP)), charPtrType,
                         leftExpr, curExpr);
        return;
    }
    /* Pointer arithmetic steps in pointee units: the integer operand is
       scaled to words here, so codegen sees an ordinary integer add.  Any
       combination not taken apart below -- a scaling operator, 'int - ptr',
       a sum of two pointers -- falls into the rejection that follows. */
    lstep = eltStep(arg1Type);
    rstep = eltStep(arg2Type);
    if (lstep and rstep) {
        if (oper == MINUSOP and typeCheck(arg1Type, arg2Type)) {
            curExpr = mkExpr(INTMINUS, IntegerType, leftExpr, curExpr);
            if (lstep != 1)
                curExpr = mkExprFold(IDIVOP, IntegerType, curExpr,
                                     mkIntLit(lstep));
            return;
        }
    } else if (lstep) {
        if ((oper == PLUSOP or oper == MINUSOP) and
            typeCheck(IntegerType, arg2Type)) {
            /* Only PLUSOP and MINUSOP reach here, so the step is plain. */
            curExpr = mkExpr(Operator(oper + 1), arg1Type,
                             leftExpr, scaleIdx(curExpr, lstep));
            return;
        }
    } else if (rstep and oper == PLUSOP and
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
        resOp = Operator(oper + (oper != IMODOP));
        /* One integer, but not one multiply: a product with an unsigned
           operand is the unsigned one, and says so in its own type, so a
           chain of them stays unsigned. */
        resTyp = IntegerType;
        if (resOp == IMULOP and
            (arg1Type == UnsignedType or arg2Type == UnsignedType))
            resTyp = UnsignedType;
    }
    curExpr = mkExprFold(resOp, resTyp, leftExpr, curExpr);
} /* bldArithOp */

void bldRelOp(Operator oper, ExprPtr ex2)
{
    Operator resOp;

    /* A compared array is its first element's address, as in C, whatever
       stands on the other side: two arrays compare those addresses, not
       their contents.  decayArray leaves anything else as it is. */
    ex2 = decayArray(ex2);
    arg1Type = ex2->vt.typ;
    curExpr = decayArray(curExpr);
    arg2Type = curExpr->vt.typ;
    if (typeCheck(arg1Type, arg2Type)) {
        /* A multi-word value compares only for equality; ordering it would
           need the P/EQ family to report which way.  The ordering operators
           are the ones from LTOP up, which genComparison relies on too. */
        if ((typeSize(arg1Type) != 1) and
            (oper >= LTOP) and
            not isCharArray(arg1Type))
            error(errNeedOtherTypesOfOperands);
    } else if (not areTypesCompatible(ex2)) {
        error(errNeedOtherTypesOfOperands);
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

void bldLogOp(Operator oper, ExprPtr leftExpr, [[maybe_unused]] bool match)
{
    // Each operand is a condition in its own right -- genBoolAnd branches on
    // each and short-circuits between them -- so each has only to be
    // branchable, by the same kind test ifWhileStatement and bldCondOp use.
    // They need not be compatible with one another, which is why match, the
    // typeCheck of one against the other, is not consulted: it would keep a
    // packed field, whose int:N is a type of its own, out of an && with an
    // int, which is exactly the mismatch the kind test exists to end.
    if ((arg1Type.p.pk > (uint64_t)kindPtr) or
        (arg2Type.p.pk > (uint64_t)kindPtr))
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

void parseUnaryExpression();

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
    TPtr l5var2z{};
    int64_t asint64_t;
    int64_t stProcNo, checkMode, resultValue;

    curVal.ii = routine->low();
    stProcNo = curVal.ii;
    if (SY != LPAREN) {
        requiredSymErr(LPAREN);
        badExpr();
        return;
    }
    if (stProcNo == fnSIZEOF or stProcNo == fnOFFSETOF) {
        lookupMode = lookUse;
        inSymbol();
        if (SY == TYPESY) {
            /* A whole type name, declarator and all: sizeof(char *),
               sizeof(int[4]), offsetof(rec, f). */
            l5var2z = readTypeName();
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
            /* Only a record has fields.  Error 22 names the construct it is
               reporting on, since stmtName otherwise still holds whatever
               operator set it last. */
            bool hasFields = l5var2z.p.pk == kindStruct;
            if (not hasFields) {
                stmtName = "OFFSET";
                error(errWrongVarTypeBefore);
            }
            if (SY != COMMA)
                requiredSymErr(COMMA);
            else {
                typ121z = l5var2z;
                lookupMode = lookField;
                inSymbol();
            }
            resultValue = 0;
            if (SY != IDENT) {
                error(errNoIdent);
            } else {
                /* The name is read either way, to keep the parse in step, but
                   looked up only where a field could be found. */
                if (hasFields) {
                    if (hashTravPtr == NULL)
                        error(errNotDefined);
                    else
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
    arg1Type = curExpr->vt.typ;
    /* The set of functions this argument type admits.  Every type outside
       these two admits none: the address-of operator is not one of them. */
    if (arg1Type == RealType)
        checkMode = Bits(fnABS);
    else if (isIntTyp(arg1Type))
        checkMode = Bits(fnABS, fnMALLOC, fnCARD) | Bits(fnMINEL);
    else
        checkMode = 0;
    asint64_t = Bits(stProcNo);
    if (stProcNo != fnSIZEOF and not subset(asint64_t, checkMode))
        error(errNeedOtherTypesOfOperands);
    if (not (subset(asint64_t, Bits(fnABS, fnSIZEOF)))) {
        arg1Type = routine->typ;
    } else if (isIntTyp(arg1Type) and subset(asint64_t, Bits(fnABS))) {
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
    if (SY == IDENT or SY == INTCONST or SY == REALCONST or
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
                case VARID: case REGID: case STATICID:
                    parseLval();
                    break;
                default:
                    break;
                } /* case */
        } break;
        case LPAREN: {
            inSymbol();
            if (SY == TYPESY) {
                /* '(' type-name ')' cast-expression.  int and float differ in
                   representation, so a cast between the two converts, the way
                   an assignment does; any other pair of types of the same size
                   is a reinterpretation, and costs nothing. */
                l4typ11z = readTypeName();
                checkSymAndRead(RPAREN);
                parseUnaryExpression();
                castArith(l4typ11z, curExpr);
                if (typeSize(curExpr->vt.typ) != typeSize(l4typ11z))
                    error(errNeedOtherTypesOfOperands);
                curExpr->vt.typ = l4typ11z;
            } else {
                readNext = false;
                expression();
                checkSymAndRead(RPAREN);
            }
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
        badExpr();
        return;
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
            else if (isIntTyp(arg1Type)) {
                curExpr = mkExpr(EQOP, BooleanType, curExpr, mkIntLit(0));
            } else {
                error(errNeedOtherTypesOfOperands);
                return;
            }
        } break;
        case MUL: {
            /* An array is its first element's address here too, so '*a' is
               'a[0]', as in C. */
            if (arg1Type.p.pk == kindArray) {
                curExpr = decayArray(curExpr);
                arg1Type = curExpr->vt.typ;
            }
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
                       curExpr->expr1->vt.typ.p.pad) {
                /* A packed char array holds six 8-bit bytes to a word, so an
                   element's byte index is the array's word address times six
                   plus the zero-based index. */
                ExprPtr idxExpr = curExpr->expr2;
                curExpr = mkExpr(INTPLUS, charPtrType,
                    mkExpr(IMULOP, IntegerType,
                           mkRef(curExpr->expr1, IntegerType),
                           mkIntLit(6)),
                    idxExpr);
            } else if (arg1Type == CharType)
                /* An unpacked char array's element has a word to itself, and
                   a char value sits in its rightmost byte. */
                curExpr = mkExpr(INTPLUS, charPtrType,
                    mkExpr(IMULOP, IntegerType,
                           mkRef(curExpr, IntegerType), mkIntLit(6)),
                    mkIntLit(5));
            else {
                /* The address of an lvalue is a pointer to its type. */
                curExpr = mkRef(curExpr, getPtrType(arg1Type));
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

    auto bldByPrc = [&](int64_t prec) {
        switch (prec) {
        case precMul:
        case precAdd:
            bldArithOp(oper, leftExpr, match);
            break;
        case precRel:
        case precEq:
            bldRelOp(oper, leftExpr);
            break;
        case precShift:
        case precBitAnd:
        case precBitXor:
        case precBitOr:
            bldBitOp(oper, leftExpr);
            break;
        case precAnd:
        case precOr:
            bldLogOp(oper, leftExpr, match);
            break;
        }
    };

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
            /* An array is never an lvalue: its name in a value context is
               an address, so there is nothing to assign to. */
            if (not has(lvalOpSet, leftExpr->op) or
                leftExpr->vt.typ.p.pk == kindArray)
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
                bldByPrc(opPrec[oper]);
                curExpr->expr1 = curExpr->expr2;
                curExpr->expr2 = NULL;
                arg2Type = curExpr->vt.typ;
            }
            leftExpr = cpDsLval(leftExpr);
            /* An array assigned to a pointer decays, as in C.  An array is
               not assignable at all, again as in C: it is not a modifiable
               lvalue, and its name in a value context is an address. */
            if (arg1Type.p.pk == kindPtr) {
                curExpr = decayArray(curExpr);
                arg2Type = curExpr->vt.typ;
            }
            if (arg1Type.p.pk == kindArray)
                arg2Type = arg1Type;   /* refused above; one report, not two */
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

            bldByPrc(curPrec);
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
    curIdent = toText("BREAK");
    setStrLab();
    curIdent = toText("CONTINUE");
    setStrLab();
} /* setBrCont */

void brContTarget()
{
    StrLabel * &strLabList = programme::super.back()->strLabList;

    /* assigning target for break/continue if used */
    if (strLabList->target)
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
    if (loopExpr) {
        curExpr = loopExpr;
        (void) formOperator(DOIT);
    }
    formJump(toLoop);
    if (leave) {
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
    while (SY == IDENT and curIdent == toText("REGISTER")) { // pins a pointer in an index register
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
    while (curLab) {
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
    const int64_t caseTabMin = 9; // clause count from which a switch indexes
    SwState &curSwitch = programme::super.back()->curSwitch;
    CaseChainPtr &curClause = programme::super.back()->curClause;
    SwState savSwitch;
    int64_t savTop;
    bool isIntCase;
    bool itemsEnded;
    TPtr exprtype;
    int64_t itemSpan;
    int64_t nClauses;
    Word expected;
    int64_t decoder, endOfStmt;
    Word minValue, maxValue;

    savSwitch = curSwitch;
    savTop = heaptop();
    ++programme::super.back()->switchDepth;
    parentExpression();
    exprtype = curExpr->vt.typ;
    curSwitch.otherSeen = false;
    if (exprtype.p.pk == kindScalar)
        (void) formOperator(LOAD);
    else
        error(25); /* errExprNotOfADiscreteType */
    disableNorm();
    decoder = 0;
    endOfStmt = 0;
    curSwitch.allClauses = NULL;
    formJump(decoder);
    checkSymAndRead(BEGINSY);
    curSwitch.firstType.setRep(NULL);
    curSwitch.goodMode = true;
    /* The body is statements, and a label anywhere in one of them binds to
       this switch -- Statement() collects it.  CH != 0 stops an unterminated
       body at end of file instead of spinning on it. */
    while (SY != ENDSY and CH != 0)
        Statement();
    curSwitch.goodMode = curSwitch.goodMode and (arithMode == 1);
    if (SY != ENDSY) {
        requiredSymErr(ENDSY);
        stmtName = "CASE  ";
        reportStmtType();
    } else
        inSymbol();
    if (not typeCheck(curSwitch.firstType, exprtype)) {
        error(74); /* errDifferentTypesOfLabelsAndExpr */
        goto L16220;
    }
    formJump(endOfStmt);
    padToLeft();
    isIntCase = typeCheck(exprtype, IntegerType);
    if (curSwitch.allClauses) {
        expected = curSwitch.allClauses->value;
        minValue = expected;
        curClause = curSwitch.allClauses;
        nClauses = 0;
        while (curClause) {
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
        if (not curSwitch.otherSeen) {
            curSwitch.otherOffset = moduleOffset;
            formJump(endOfStmt);
        }
        fixup(0, decoder);
        curVal = minValue;
        fixup(-(InsnTemp[U1A]+curSwitch.otherOffset), maxValue.ii);
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
        while (curSwitch.allClauses) {
            form1Insn(InsnTemp[decoder] + curSwitch.allClauses->offset);
            curSwitch.allClauses = curSwitch.allClauses->next;
            decoder = (int64_t)UZA + (int64_t)UJ - decoder;
        }
        goto L16211;
L16140:
        itemSpan = 34000;
        fixup(0, decoder);
        if (curSwitch.firstType.p.pk == kindScalar)
            itemSpan = curSwitch.firstType.rep()->numen;
        itemsEnded = itemSpan < 32000;
        if (itemsEnded)
            form1Insn(KATI+14);
        /* Both chains walk the labels by the difference from the one before,
           so each step needs a single instruction to reach the next label:
           an index register counts down through KUTM, and the accumulator
           holds the subject XOR-ed with the label reached so far, AEX being
           its own inverse. */
        minValue.ii = (minValue.ii - minValue.ii); /* WTF? */
        while (curSwitch.allClauses) {
            if (itemsEnded) {
                curVal.ii = (minValue.ii - curSwitch.allClauses->value.ii);
                /* KVZM reads the index register, so a step of zero -- a
                   first label of zero -- needs no instruction of its own.
                   The accumulator chain below keeps its KAEX even then: it
                   is what leaves omega logical, which an arithmetic subject
                   expression may not have done. */
                if (curVal.ii)
                    form1Insn(getValueOrAllocSymtab(curVal.ii) +
                              (KUTM+I14));
                form1Insn(KVZM+I14 + curSwitch.allClauses->offset);
            } else {
                curVal.ii = (minValue.ii ^ curSwitch.allClauses->value.ii);
                form2Insn(KAEX + I8 + getFCSToffset(),
                          InsnTemp[UZA] + curSwitch.allClauses->offset);
            }
            minValue = curSwitch.allClauses->value;
            curSwitch.allClauses = curSwitch.allClauses->next;
        }
        if (curSwitch.otherSeen)
            form1Insn(InsnTemp[UJ] + curSwitch.otherOffset);
L16211:
        fixup(0, endOfStmt);
        if (not curSwitch.goodMode)
            disableNorm();
    }
    /* Put back the switch this one is nested in. */
L16220:
    --programme::super.back()->switchDepth;
    settop(savTop);
    curSwitch = savSwitch;
} /* caseStatement */

void ifWhileStatement()
{
    int64_t &ifWhlTarget = Statement::super.back()->ifWhlTarget;

    disableNorm();
    parentExpression();
    // Anything a branch can test: a scalar, a pointer, or a packed field of
    // either, which is a type of its own and so not IntegerType.  bldCondOp
    // admits the same set for the ternary.
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
        return allocSymtab((arg | 040000000) & halfWord);
    } else {
        return arg;
    }
} /* allocDataRef */

/* An ordinary item carries a value and a repeat count.  A string wider than
   a word is in the constant pool already, six characters to the word: words
   names how long it is and value where it sits. */
struct InitItem { int64_t value, count; };
struct InitSeg  { int64_t base; std::vector<InitItem> items; };
std::vector<InitSeg> initSegs;

// Start a new destination segment: 'var' bare (offset 0), or -- when
// 'designator' -- 'var[index]...' (SY is at '['; parsePostfix builds the GETELT
// chain and leaves SY at '=').  formOperator(SETREG12) forms the destination's
// address as a single instruction with no module/FCST side effect: putLeft
// parks it in leftInsn instead of the object buffer, and only its address field
// is kept, so which register it would have loaded does not matter here.
void beginInitSeg(IdentRecPtr var, bool designator) {
    curExpr = mkExpr(GETVAR, var->typ, (ExprPtr)var, NULL);
    if (designator)
        parsePostfix();
    putLeft = true;
    objBufIdx = 1;
    (void) formOperator(SETREG12);
    if (objBufIdx != 1)
        error(errVarTooComplex);
    initSegs.push_back(InitSeg{ leftInsn & 0777700000000L, {} });
}

// Parse (but do not emit) one global's '= initializer', buffering it into
// initSegs.  A '[index]=' designator opens a new segment; bare items and
// 'value:count' fills accumulate into the current segment.
void parseInitializer(IdentRecPtr var) {
    ExprPtr boundary;
    TPtr dest;
    inSymbol();                       /* consume '=' -> SY at first init token */
    bool braced = SY == BEGINSY;
    if (braced)
        inSymbol();                   /* consume '{' */
    setup(boundary);
    initWords = 0;
    beginInitSeg(var, false);         /* initial segment: var, offset 0 */
    for (;;) {
        if (braced and SY == LBRACK) {
            /* '[index]=' designator opens a new segment (parsePostfix's
               expression() consumes the '[' -- it needs readNext=true).
               It moves the destination, so the items no longer count off
               the array from its start and there is nothing to take a
               missing size from. */
            if (var->typ.p.pk == kindArray and var->typ.rep()->asize == 0)
                error(64); /* errIncorrectRangeDefinition */
            myrollup(boundary);
            setup(boundary);
            readNext = true;
            beginInitSeg(var, true);
            checkSymAndRead(BECOMES);
        }
        int64_t count = 1;
        /* Nothing relocatable can be an initializer, a module carrying
           absolute words only, so a pointer cannot be initialized at all:
           an address is what the loader puts in place.  takeConstFromExpr
           refuses every address expression -- '&x', an array's name, a
           routine's -- by seeing no constant; a string is a constant, so
           the destination is what has to be tested here.  Nor does a
           statement give a pointer a string: only a char array takes one,
           as its contents. */
        if (SY == STRINGSY) {
            /* Where the words would land.  A designator only walks 'var'
               along its own element type, so unwrapping the array levels
               answers for both the bare and the designated segment.  The
               innermost array is what a string goes into, whatever is
               wrapped around it, so its size on the way down is the room --
               zero where the extent was left out and the initializer is
               what says how long the array is. */
            count = 0;
            dest = var->typ;
            while (dest.p.pk == kindArray) {
                count = dest.rep()->asize ? typeSize(dest) : 0;
                dest = dest.rep()->base;
            }
            if (dest.p.pk == kindPtr)
                error(errNoConstant);
        }
        if (SY == STRINGSY and strWords != 1) {
            /* A string wider than a word is buffered, not in the pool, so
               its words go into the stream as ordinary items: they land in
               the data region beside the items around them and coalesce
               into one record rather than forcing one of their own.  A
               one-word string goes the other way, being a value like any
               other -- and the only width for which a ':count' fill means
               anything.  The clamp waits until the width above has been
               read, since it may change it: count still holds the room. */
            if (count != 0 and count < strWords)
                strWords = count;     /* no room for the terminator: drop it */
            count = 0;
            while (count != strWords) {
                initSegs.back().items.push_back(InitItem{ strBuf[count], 1 });
                ++count;
            }
            inSymbol();
        } else {
            readNext = false;         /* SY already at the value's first token */
            expression();
            takeConstFromExpr();
            int64_t v = curVal.ii;
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
        }
        initWords = initWords + count;
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
        // Not modes: the two lookup globals carry the results back, so this
        // is a data region of no words and no sets.
        lookup2 = 0;
        lookupMode = 0;
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
                if (length)
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
    /* The two lookup globals carry the results back: they are dead between
       declarations, and this runs at program end.  Neither is a mode here. */
    lookup2 = FcstCnt - dsize;
    FcstCnt = dsize;
    lookupMode = setcount;
} /* flushInitializers */


/* One constant expression, with SY already on its first token: parse it, take
   the folded value into ceVal and its type into ceTyp.  The nodes it builds
   live no longer than the expression, so the arena goes back to where it
   stood. */
void constExpr()
{
    TPtr &ceTyp = programme::super.back()->ceTyp;
    Word &ceVal = programme::super.back()->ceVal;
    ExprPtr boundary;

    setup(boundary);
    ceTyp = voidType;
    ceVal.ii = 1;
    readNext = false;
    expression();
    takeConstFromExpr();
    ceTyp = curExpr->vt.typ;
    ceVal = curVal;
    myrollup(boundary);
} /* constExpr */

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
            if (procName->typ.p.pk == kindPtr)
                curExpr = decayArray(curExpr);
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
    form1Insn(getHelperProc(11) + (KUJ-KVJM-I13));
} /* returnOp */

struct standProc {

    TPtr l4typ3z;
    ExprPtr l4exp7z, workExpr;
    int64_t procNo;
    int64_t besmOpcode;

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
        }
    } /* startWrite */

    void printfProc() {
        int64_t argCount;
        bool isFormat;

        workExpr = NULL;
        argCount = 0;
        isFormat = true;
        do {
            /* startWrite takes the same optional leading file argument write
               takes; the destination reaches C/PRINTF in M12, not on the
               stack. */
            startWrite();
            if (l4exp7z != workExpr) {
                curExpr = l4exp7z;
                if (curExpr->vt.typ.p.pk == kindArray)
                    curExpr = decayArray(curExpr);
                if (isFormat and not isCharPtr(curExpr->vt.typ)) {
                    error(40); /* errIncompatibleArgumentTypes */
                    curExpr = uVarPtr;
                } else if (typeSize(curExpr->vt.typ) != 1) {
                    error(40); /* errIncompatibleArgumentTypes */
                    curExpr = uVarPtr;
                }
                (void) formOperator(LOAD);
                /* An enum printed by name has no conversion of its own, so
                   the actual is the name itself: the entry the ordinal
                   selects in the table C/WX would have indexed, for %t to
                   write. */
                l4typ3z = curExpr->vt.typ;
                if (l4typ3z.p.pk == kindScalar and l4typ3z.rep()->start != -1) {
                    dumpEnumNames(l4typ3z);
                    form1Insn(KATI+14);
                    form1Insn(KUTC+I14);
                    form1Insn(KXTA+I8 + l4typ3z.rep()->start);
                }
                form1Insn(KXTS);  /* C/PRINTF owns and removes every argument */
                isFormat = false;
                argCount = argCount + 1;
            }
        } while (SY == COMMA);
        if (argCount == 0)
            error(36); /* errTooFewArguments */
        form1Insn(KVTM+I10 + getValueOrAllocSymtab(-argCount));
        curExpr = workExpr;
        (void) formOperator(SETREG12);
        formAndAlign(getHelperProc(30)); /* "C/PRINTF" */
        usedRegs = usedRegs | Bits(12);
    } /* printfProc */

    standProc() { /* standProc */
        IdentRecPtr &l3idr12z = Statement::super.back()->l3idr12z;

        curVal.ii = l3idr12z->low();
        procNo = curVal.ii;
        if (SY != LPAREN) {
            error(45); /* errNoOpenParenForStandProc */
            return;
        }
        switch (procNo) {
        case 0: { /* besm */
            do {
                expression();
                takeConstFromExpr();
                if (SY == COLON) {
                    besmOpcode = curVal.ii;
                    inSymbol();
                    /* An lvalue supplies its address through STORE's final
                       ATX; any other expression is formed normally through
                       LOAD. */
                    readNext = false;
                    expression();
                    prevOpcode = 0;
                    if (has(lvalOpSet, curExpr->op))
                        (void) formOperator(STORE);
                    else
                        (void) formOperator(LOAD);
                    /* Replace only the opcode of the instruction just
                       formed; retain its register and address fields. */
                    if (putLeft)
                        objBuffer[objBufIdx-1] =
                            (objBuffer[objBufIdx-1] & ~02770000LL) |
                            besmOpcode;
                    else
                        leftInsn =
                            (leftInsn & ~(02770000LL << 24)) |
                            (besmOpcode << 24);
                } else {
                    /* Literal commands are not candidates for the ordinary
                       instruction peepholes.  Reset their history while
                       leaving putLeft alone, so successive operands fill
                       successive halves. */
                    prevOpcode = 0;
                    form1Insn(curVal.ii);
                }
            } while (SY == COMMA);
            padToLeft();
            prevOpcode = 1;
        } break;
        case 1: { /* printf */
            inSymbol();
            if (SY == RPAREN)
                error(36); /* errTooFewArguments */
            else {
                readNext = false;
                printfProc();
            }
        } break;
        }
        if (procNo == 1)
            arithMode = 1;
        checkSymAndRead(RPAREN);
    }
}; /* standProc */

Statement::Statement()
{
    StrLabel * &strLabList = programme::super.back()->strLabList;

    super.push_back(this);
    if (SY == SEMICOLON) {
        inSymbol();
        return; /* empty statement */
    }
    setup(boundary);
    bool110z = false;
    startLine = lineCnt;
    {
        try {
            /* A switch label binds to the switch being parsed, wherever it
               is written: the label list belongs to programme, not to
               caseStatement, so a label in a nested block, in an arm of an
               if, or in a loop body -- which is how Duff's device interleaves
               them with a do-while -- reaches it from here.  Several may
               stand in front of one statement, and the statement they label
               is then the one this same call goes on to parse. */
            while (programme::super.back()->switchDepth
                   and (SY == CASESY or SY == DEFAULTSY)) {
                SwState &curSwitch = programme::super.back()->curSwitch;
                CaseChainPtr &curClause = programme::super.back()->curClause;
                CaseChainPtr &clause = programme::super.back()->clause;
                CaseChainPtr &prev = programme::super.back()->prev;
                TPtr &itemtype = programme::super.back()->itemtype;
                Word &itemvalue = programme::super.back()->itemvalue;
                const int64_t caseWords = sizeof(CaseChain) / sizeof(int64_t);

                curSwitch.goodMode = curSwitch.goodMode and (arithMode == 1);
                padToLeft();
                arithMode = 1;
                if (SY == DEFAULTSY) {
                    if (curSwitch.otherSeen)
                        error(73); /* errCaseLabelsIdentical */
                    inSymbol();
                    curSwitch.otherSeen = true;
                    curSwitch.otherOffset = moduleOffset;
                } else {
                    /* expression() reads a token first, so it consumes the
                       CASESY itself, the way the arm loop that used to stand
                       here did. */
                    expression();
                    takeConstFromExpr();
                    itemvalue = curVal;
                    itemtype = curExpr->vt.typ;
                    if (itemtype.rep()) {
                        if (curSwitch.firstType.rep() == NULL) {
                            curSwitch.firstType = itemtype;
                        } else {
                            if (not typeCheck(itemtype, curSwitch.firstType))
                                error(errConstOfOtherTypeNeeded);
                        }
                        /* Clause records outlive the statement the label sits
                           in, so they cannot come from the arena: every
                           statement rolls that back on the way out.  talloc
                           takes them from the top of the heap, where nothing
                           rolls back, and caseStatement gives the whole
                           switch's worth back at once. */
                        {
                            clause = reinterpret_cast<CaseChainPtr>(
                                heap + talloc(caseWords));
                            clause->value = itemvalue;
                            clause->offset = moduleOffset;
                            curClause = curSwitch.allClauses;
                            while (curClause) {
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
                            if (curClause == curSwitch.allClauses) {
                                clause->next = curSwitch.allClauses;
                                curSwitch.allClauses = clause;
                            } else {
                                clause->next = curClause;
                                prev->next = clause;
                            }
                        }
                    }
                }
                checkSymAndRead(COLON);
            }
            if (SY == IDENT) {
                /* CH is already the first character after the identifier.
                   Advance over blanks directly, leaving the identifier token
                   and its symbol lookup untouched unless the next character
                   is a label colon. */
                while (skipSp()) ;
                if (CH == ':') {
                    liveRegs = Bits();
                    disableNorm();
                    flag = true;
                    padToLeft();
                    labCheckAndDefine(true);
                    nextCH();
                    inSymbol();
                }
            }
            nest = has(Bits(BEGINSY,SWITCHSY), SY);
            if (nest)
                lineNesting = lineNesting + 1;
/*(ident)*/
            if (SY == IDENT) {
                if (hashTravPtr) {
                    l3var6z = (IdClass)hashTravPtr->pck.cl;
                    if (l3var6z == ROUTINEID) {
                        l3idr12z = hashTravPtr;
                        if (l3idr12z->pck.offset == 0) {
                            /* System procedure (BESM, PRINTF): special
                               syntax, handled directly. */
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
                    /* VARID / FIELDID, or ROUTINEID with non-NIL
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
                while (SY != ENDSY and CH)
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
                if (blockDecls) {
                    while (blockDecls) {
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
                if (SY != IDENT) {
                    requiredSymErr(IDENT);
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
                // The same set ifWhileStatement and bldCondOp take: NOTOP
                // only flips the polarity of the list BRANCH is about to
                // read, so whatever one can branch on the other can too.
                if (curExpr->vt.typ.p.pk > (uint64_t)kindPtr) {
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
                curIdent = toText("BREAK");
                setStrLab();
                caseStatement();
                brContTarget(); /* removing break */
            } else if (SY == CASESY or SY == DEFAULTSY) {
                /* A switch label with no switch around it -- one inside a
                   switch is taken by the loop above, before this dispatch.
                   The whole label has to go, colon included: dropping the
                   keyword alone would leave the constant and the colon for
                   the enclosing loop to spin on instead.  The statement the
                   label was attached to still parses. */
                errAndSkip(errBadSymbol, Bits(COLON, SEMICOLON, ENDSY, NOSY));
                if (SY == COLON)
                    inSymbol();
            } else if (has(Bits(TYPEDEFSY, TYPESY, CONSTSY) |
                           Bits(ENUMSY, STRUCTSY, UNIONSY) |
                           Bits(PACKEDSY, STATICSY), SY)) {
                /* A declaration keyword reached statement context -- it leaked
                   here from a malformed routine header (see bad.p2c).  Report
                   and consume it so the enclosing 'while (SY != ENDSY)
                   Statement()' loops make progress instead of spinning.  Other
                   stray tokens -- the SEMICOLON of a labelled empty statement
                   above all -- keep the original silent-return behaviour, so
                   valid code is unaffected. */
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

// An array size is a const-expression.  It is evaluated by constExpr,
// which parses one expression, const-folds it and leaves the value in ceVal /
// its type in ceTyp.
// Forward-declared far above (near parseTypeRef).
// SY is on the '[' that opens the size; parseArrSz reads past it itself.  A
// size is a constant expression, so it names ordinary identifiers -- a const,
// an enumerator, a type for sizeof -- and never a record field: the lookup
// goes to lookUse for the whole expression and back to the caller's afterwards,
// which is what lets a size inside a record body reach them at all.  lookup2
// must move with lookupMode, since every inSymbol() resets lookupMode from it.
void parseArrSz(int64_t & asize)
{
    TPtr &ceTyp = programme::super.back()->ceTyp;
    Word &ceVal = programme::super.back()->ceVal;

    int64_t savedLookup = lookup2;
    lookup2 = lookupMode = lookUse;
    inSymbol();
    if (noExtentOk and SY == RBRACK) {
        /* 'int q[]': zero marks the missing extent.  A parameter discards
           the array type for a pointer, and a variable takes its size from
           the initializer that has to follow. */
        asize = 0;
    } else {
        constExpr();
        if (ceTyp.rep() and ceTyp.p.pk == kindScalar and SY == RBRACK) {
            asize = ceVal.ii;
        } else {
            error(64); /* errIncorrectRangeDefinition */
            asize = 1;
        }
    }
    lookup2 = lookupMode = savedLookup;
} /* parseArrSz */

void parseConstDeclValue(TPtr declared, TPtr &typ, Word &value)
{
    if (SY == STRINGSY) {
        parseLiteral(typ, value, true);
        if (not isCharArray(declared)) {
            error(33); /* errIllegalTypesForAssignment */
        } else {
            if (declared.rep()->asize == 0) {
                int64_t extent;
                if (declared.p.pad)
                    extent = strWords * declared.rep()->perword;
                else
                    extent = strWords / typeSize(declared.rep()->base);
                declared = makeArrayType(extent, declared.rep()->base,
                                         declared.p.pad != 0);
            }
            int64_t capacity = typeSize(declared);
            if (capacity > strWords)
                error(33); /* no constant-pool padding */
            else if (capacity == 1)
                value.ii = strBuf[0];
        }
        typ = declared;
        inSymbol();
        return;
    }
    if (declared.p.pk == kindArray and declared.rep()->asize == 0) {
        error(64); /* errIncorrectRangeDefinition */
        declared = makeArrayType(1, declared.rep()->base,
                                 declared.p.pad != 0);
    }
    ExprPtr boundary;
    setup(boundary);
    readNext = false;
    expression();
    if (not typeCheck(declared, curExpr->vt.typ) and
        not castArith(declared, curExpr))
        error(33); /* errIllegalTypesForAssignment */
    takeConstFromExpr();
    typ = declared;
    value = curVal;
    myrollup(boundary);
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
    IdentRecPtr l3idr5z = NULL;   /* only read when hasMain says it was found */
    Word l3var7z;
    bool hasMain = false;
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
        // The level 1 block is a call of the routine named MAIN.  A module
        // without one has nothing to run, so neither the block nor the file
        // open/close that frames it is emitted, and the module keeps only the
        // entry points its E+ routines declare.
        bucket = toText("MAIN") % 65535 % 128;
        l3idr5z = symHash[bucket];
        while (l3idr5z and l3idr5z->id != toText("MAIN"))
            l3idr5z = l3idr5z->next();
        hasMain = l3idr5z != NULL and l3idr5z->pck.cl == ROUTINEID
                  and l3idr5z->pck.offset != 0;
        if (hasMain) {
            fileExit = moduleOffset;
            formFileInit();
        } else {
            // The module header's start word is emitted whatever happens, so
            // its symbol must resolve to something: absolute 20, the abort
            // entry, since a module with no MAIN is not meant to be started.
            // 040000000 alone is the absolute tag; 041000000 would relocate it.
            // The module names itself NOPROGRA for the same reason: PASCOMPL
            // is the name of a program, and two libraries carrying it would
            // collide.
            symTab[2] = 040000000 | 020;
            entryPtTable[1] = symTab[0] = toText("NOPROGRA");
        }
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
    if (not bodyBlock and SY != BEGINSY and CH)
        requiredSymErr(BEGINSY);
    l3var2z.ii = lineNesting;
    if (bodyBlock) {
        while (SY != ENDSY and CH)
            Statement();
        if (SY != ENDSY)
            requiredSymErr(ENDSY);
        else
            inSymbol();
    } else if (curProcNesting == 1) {
        // The level 1 block is not written, it is generated, and MAIN was
        // looked up above.  Anything left over here -- an explicit block above
        // all -- is a bad symbol where a declaration was expected.
        if (CH or SY != NOSY) {
            error(errBadSymbol);
            skipToEnd();
        }
        if (hasMain) {
            // The call node parseCallArgs would have built for 'main()',
            // with no arguments: op ALNUM, the routine in id2.
            curExpr = mkExpr(ALNUM, l3idr5z->typ, NULL, (ExprPtr) l3idr5z);
            (void) formOperator(DOIT);
        }
    } else if (CH) {
        do {
            Statement();
            done = has(blockBegSys, SY) or (SY == TYPESY) or (CH == 0);
            if (not done)
                errAndSkip(errBadSymbol, skipToSet);
        } while (not done);
    }
    procName->flags() = (usedRegs & BitRange(0,15)) | (procName->flags() & ~ l3var7z.ii);
    lineNesting = l3var2z.ii - 1;
    /* An empty body needs no frame: the return goes straight back through
       M13 and no register is disturbed.  A body with anything in it borrows
       a scratch display register to hold the link, and a routine without a
       frame has nowhere to save what it borrows -- its callers no longer
       rebuild the display, so it gets a frame like everything else. */
    if (not bool48z and (objBufIdx == 2) and (sizeCount == 8) and
        (curProcNesting != 1) and ((usedRegs & BitRange(1,15)) != BitRange(1,15))) {
        objBuffer[1] = int64_t(KUJ+I13) << 24;          /* 13,UJ, */
        procName->flags() = procName->flags() | Bits(25);
        putLeft = true;
    } else if (curProcNesting != 1 or hasMain) {
        jj = curProcNesting == 1 ? 12 /* C/EF */ : 11; /* C/E */
        form1Insn(getHelperProc(jj) + (KUJ-KVJM-I13));
        if (curProcNesting == 1) {
            parseDecls(2);
            form1Insn(InsnTemp[UJ] + l3var1z.ii);
            curVal.ii = procName->r1.pos - 040000;
            symTab[2] = 041000000 | (curVal.ii & halfWord);
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
    static constexpr int64_t systemProcNames[2] = {
    /*0*/   toText("BESM"),
            toText("PRINTF")};
    IdentRecPtr programObj;
    BooleanType.setRep(
        besm6_alloc_record<Types>(offsetof(Types, szScalar)));
    BooleanType.rep()->numen = 2;
    BooleanType.rep()->start = -1;
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

    UnsignedType.setRep(
        besm6_alloc_record<Types>(offsetof(Types, szScalar)));
    UnsignedType.rep()->numen = 100000;
    UnsignedType.rep()->start = -1;
    UnsignedType.rep()->enums = NULL;
    UnsignedType.p.psize = 1;
    UnsignedType.p.bits = 48;
    UnsignedType.p.pk = kindScalar;

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
    flatMemType.rep()->perword = 6;
    flatMemType.rep()->asize = 32768 * 6;
    flatMemType.p.psize = 32767;
    flatMemType.p.bits = 48;
    flatMemType.p.pad = 8;
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
    // This machine has one integer, so every C spelling of one names it.
    symType = IntegerType;  regResWord(toText("INT"));
                            regResWord(toText("SHORT"));
                            regResWord(toText("LONG"));
    // short and long say nothing about signedness; unsigned does.
    symType = UnsignedType; regResWord(toText("UNSIGNED"));
    symType = CharType;     regResWord(toText("CHAR"));
    symType = RealType;     regResWord(toText("FLOAT"));
    symType = voidType;     regResWord(toText("VOID"));

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
    uProcPtr->setSig(NULL);
    uProcPtr->setPreDef(NULL);
    uProcPtr->r1.pos = 0;

    temptype.setRep(NULL);
    sysProcNum = 0;
    for (l3var5z = 0; l3var5z <= 1; ++l3var5z) {
        regSysProc(systemProcNames[l3var5z]);
    }
    sysProcNum = 0;
    temptype = RealType;
    regSysProc(toText("ABS"));
    temptype = IntegerType;
    regSysProc(toText("SIZEOF"));
    regSysProc(toText("OFFSETOF"));
    temptype = voidPtr;
    regSysProc(toText("MALLOC"));
    temptype = IntegerType;
    regSysProc(toText("CARD"));
    regSysProc(toText("MINEL"));

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
    curVal.ii = toText("PASCOMPL");
    programObj->id = curVal.ii;
    programObj->r1.pos = 0;
    symTab[0] = leftAlign(curVal.ii);

    entryPtTable[1] = symTab[0];
    entryPtTable[3] = toText("PROGRAM ");
    entryPtTable[2] = Bits(1);
    entryPtTable[4] = Bits(1);
    entryPtCnt = 5;
    CHILD.push_back((Bits(0,4,6) | BitRange(9,12) | Bits(23,28,29) |
                     BitRange(33,36) | Bits(46))); /*10 24 74001 00 30 74002*/
    moduleOffset = 040001;
    programObj->setSig(NULL);
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
    curIdent = toText("*OUTPUT*");
    defExtern();
    curIdent = toText("*INPUT*");
    defExtern();
    if (!enableStdInput) {
        inputFile = NULL;
        fileForInput = NULL;
    }
    curIdent = savedIdent.ii;
    lookupMode = lookUse;
    l3var6z = 40;
    moduleDataSize = l3var6z;
    do {
        programme(l3var6z, programObj, false);
    } while (CH);
    // Emit the data-init region from the declaration-site initializers
    // buffered during parsing.
    flushInitializers();
    readToPos80();
    curVal.ii = l3var6z;
    symTab[3] = (helperNames[9] | Bits(24,27,28,29)) |
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
// A parameter list is read once, into the routine's signature -- the record
// that outlives the body.  The formals' own identrecs are not made here:
// makeFormals does that after the caller has taken the scope mark, so the
// rollback that ends the body takes them with it.  A declaration without a
// body therefore builds no identrecs at all.
struct FormalName {
    int64_t name, bucket;
};
FormalName formalNames[MAXFORMALS];
int64_t formalCnt;

void parseParameters(SigPtr matchTo)
{
    SigPtr newSig, lastSig;
    bool noComma, matching;
    IdentRecPtr &curIdRec = programme::super.back()->curIdRec;
    int64_t &l2int18z = programme::super.back()->l2int18z;

    formalCnt = 0;
    lastSig = NULL;
    matching = matchTo != NULL;
    // lookup2 (not just lookupMode) must carry lookDef through
    // parseTypeRef's own internal inSymbol() calls -- see the identical
    // note on TYPEDEFSY/parseRecordDecl.
    lookup2 = lookupMode = lookDef;
    inSymbol();
    if (SY == RPAREN) {
        if (matching)
            error(errNoCommaOrParenOrTooFewArgs);
        inSymbol();
        lookup2 = lookupMode = lookUse;
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
        noExtentOk = true;
        Declarator d = parseOneDeclarator(paramType, packedFlag);
        nameOptional = false;
        noExtentOk = false;
        /* An array parameter is a pointer to its element, as in C.  The
           actual is the array's address either way, so the call site does
           not move; what goes is the run of argument slots a formal wider
           than a word claims.  A struct parameter keeps them: C passes
           structs by value, and here that is literal. */
        if (d.type.p.pk == kindArray)
            d.type = getPtrType(d.type.rep()->base);
        if (matching) {
            if (matchTo == NULL)
                error(errTooManyArguments);
            else {
                // Exact identity: typeCheck's assignment compatibility
                // would let a 'char' definition complete an 'int'
                // declaration.
                if (matchTo->ptyp != d.type)
                    error(40); /* errIncompatibleArgumentTypes */
                matchTo = matchTo->next;
            }
        } else {
            newSig = new SigRec;
            newSig->s1.pclass = VARID;
            newSig->ptyp = d.type;
            newSig->s1.poffset = l2int18z;
            newSig->next = NULL;
            if (lastSig == NULL)
                curIdRec->setSig(newSig);
            else
                lastSig->next = newSig;
            lastSig = newSig;
            // A formal claims one argument slot per word, so a struct's own
            // words are the ones the caller pushes.
            l2int18z = l2int18z + typeSize(d.type);
        }
        // The name is remembered, not registered: with no identrec yet there
        // is nothing to hash, so a repeated formal is caught against the
        // names already collected rather than by lookDef.
        if (formalCnt == MAXFORMALS)
            error(errTooManyArguments);
        else {
            if (d.name) {
                if (d.wasDefined)
                    error(errIdentAlreadyDefined);
                for (int64_t ii = 0; ii < formalCnt; ++ii)
                    if (formalNames[ii].name == d.name)
                        error(errIdentAlreadyDefined);
            }
            formalNames[formalCnt].name = d.name;
            formalNames[formalCnt].bucket = d.bucket;
            formalCnt = formalCnt + 1;
        }
        noComma = (SY != COMMA);
        if (not noComma) {
            lookupMode = lookDef;
            inSymbol();
        }
    } while (!noComma);
    /* 22276 */
    if (matching) {
        // The declaration settled the frame layout and recorded it in the
        // signature, so nothing here may move it.
        if (matchTo)
            error(errNoCommaOrParenOrTooFewArgs);
    }
    /* 22322 */
    checkSymAndRead (RPAREN);
    lookup2 = lookupMode = lookUse;
} /* parseParameters */

// Lifting MAXFORMALS.  The bound exists only because a formal's name has to
// survive from parseParameters, which reads it, to makeFormals, which runs
// after the scope mark -- and nothing allocated in between can be anything
// but permanent.  Carrying the name in the signature removes the need for a
// scratch array at all.  In full:
//
//   struct SigRec : public BESM6Obj {
//       union { ... } s1;
//       TPtr ptyp;
//       int64_t pname;            // <-- added, SigRec goes 3 words to 4
//       SigPtr next;
//   };
//
// In parseParameters, store the name where the offset already goes, and
// check for a repeated formal against the chain instead of the array:
//
//       newSig->s1.poffset = l2int18z;
//       newSig->pname = d.name;
//   ...
//       if (d.name != 0) {
//           if (d.wasDefined)
//               error(errIdentAlreadyDefined);
//           for (SigPtr dup = curIdRec->sig(); dup != NULL; dup = dup->next)
//               if (dup != newSig and dup->pname == d.name)
//                   error(errIdentAlreadyDefined);
//       }
//
// and the formalNames/formalCnt bookkeeping goes away with the MAXFORMALS
// test that guards it.  makeFormals then walks the signature alone -- no
// index, no count -- and lets addToHashTab work the bucket out:
//
//   void makeFormals() {
//       IdentRecPtr np;
//       SigPtr curSig;
//       IdentRecPtr &curIdRec = programme::super.back()->curIdRec;
//
//       for (curSig = curIdRec->sig(); curSig != NULL; curSig = curSig->next) {
//           np = besm6_alloc_record<IdentRec>(offsetof(IdentRec, szIdent));
//           np->id = curSig->pname;
//           np->pck.offset = curFrameRegTemplate;
//           np->pck.cl = (IdClass)curSig->s1.pclass;
//           np->typ = curSig->ptyp;
//           np->list() = NULL;
//           np->value() = curSig->s1.poffset;
//           if (np->id != 0)
//               addToHashTab(np);
//       }
//   } /* makeFormals */
//
// work.p2c mirrors it, with pname a plain int beside ptyp.  Cost: one word
// per formal that never goes away, against a bound nothing has hit.

// The formals themselves, made once the caller has taken the scope mark so
// that myrollup reclaims them with the body.  Names come from the list just
// read, everything else from the signature, which is the record of record.
void makeFormals()
{
    IdentRecPtr np;
    SigPtr curSig;
    int64_t ii;
    IdentRecPtr &curIdRec = programme::super.back()->curIdRec;

    curSig = curIdRec->sig();
    for (ii = 0; ii < formalCnt and curSig != NULL; ++ii) {
        np = besm6_alloc_record<IdentRec>(offsetof(IdentRec, szIdent));
        np->id = formalNames[ii].name;
        np->pck.offset = curFrameRegTemplate;
        np->pck.cl = (IdClass)curSig->s1.pclass;
        np->typ = curSig->ptyp;
        // Nothing chains the formals: the signature is the list.
        np->list() = NULL;
        np->value() = curSig->s1.poffset;
        // An unnamed parameter takes its argument slot like any other.  It
        // never enters the symbol table, so the body has no way to name it.
        // besm6_alloc_record zero-fills and nidx == 0 reads back as NULL, so
        // the link stays unset.
        if (np->id) {
            np->pck.nidx = ord(symHash[formalNames[ii].bucket]);
            symHash[formalNames[ii].bucket] = np;
        }
        curSig = curSig->next;
    }
} /* makeFormals */

void exitScope(IdentRecPtr arg[128])
{
    IdentRecPtr &workidr = programme::super.back()->workidr;
    IdentRecPtr &scopeBound = programme::super.back()->scopeBound;

    for (int ii = 0; ii <= 127; ++ii) {
        workidr = arg[ii];
        while (workidr and
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
    if (localSize == 0) {
        initScalars();          // reads the first token itself
        return;
    }
    preDefHead = uProcPtr;
    inTypeDef = false;
    typedefPending = false;
    typelist = NULL;
    retSeen = false;
    bodyStatSys = statBegSys;
    strLabList = NULL;
    switchDepth = 0;
    lineNesting = lineNesting + 1;
    labFence = numLabTop;
    // Not just TYPESY -- a type-spec can also open with
    // '__packed'/'struct'/'enum' (mirrors parseRecordDecl's own
    // field-group loop condition), so those must be recognized as a
    // declaration start too, or a leading '__packed int x;' would never
    // be seen as one. Declared here (above the do-while, not inside it)
    // so it's visible both in the loop body and in the do-while's own
    // trailing condition below, whose scope excludes the body's locals.
    int64_t declStartSys = Bits(TYPESY, PACKEDSY, STRUCTSY) |
                           Bits(ENUMSY, EXTERNSY, STATICSY);
    do {
        if (SY == CONSTSY) {
            /* C-style: one explicit type and a comma-separated declarator
               list.  Read the type under lookUse so a typedef is raised to
               TYPESY, then switch to lookDef before parseTypeRef reads the
               first name. */
            lookup2 = lookupMode = lookUse;
            inSymbol();
            markTypeSym();
            if (not has(Bits(TYPESY, PACKEDSY, STRUCTSY, ENUMSY), SY)) {
                error(errNotAType);
                skip(skipToSet | Bits(SEMICOLON));
                if (SY == SEMICOLON)
                    inSymbol();
            } else {
                TPtr constBase;
                bool packedFlag, forwardRef;
                lookup2 = lookupMode = lookDef;
                {
                    parseTypeRef typeParser(constBase,
                        skipToSet | Bits(IDENT, MUL, LPAREN, COMMA) |
                        Bits(SEMICOLON));
                    packedFlag = typeParser.isPacked;
                    forwardRef = typeParser.isForwardRef;
                }
                bool moreConsts = true;
                while (moreConsts) {
                    noExtentOk = true;
                    Declarator d = parseOneDeclarator(constBase, packedFlag,
                                                      forwardRef);
                    noExtentOk = false;
                    lookup2 = lookupMode = lookUse;
                    if (d.ptrOnly and SY == LPAREN) {
                        error(errBadSymbol);       /* no const routines */
                        skip(skipToSet | Bits(SEMICOLON));
                        moreConsts = false;
                        continue;
                    }
                    if (d.wasDefined)
                        error(errIdentAlreadyDefined);
                    workidr = besm6_alloc_record<IdentRec>(
                        offsetof(IdentRec, szIdent));
                    workidr->id = d.name;
                    workidr->pck.offset = curFrameRegTemplate;
                    workidr->pck.nidx = ord(symHash[d.bucket]);
                    workidr->pck.cl = ENUMID;
                    workidr->list() = NULL;
                    workidr->typ = d.type;
                    workidr->value() = 1;
                    symHash[d.bucket] = workidr;
                    if (SY != BECOMES or charClass != ASSIGNOP) {
                        error(errBadSymbol);
                        if (not has(Bits(COMMA, SEMICOLON), SY))
                            skip(skipToSet | Bits(COMMA, SEMICOLON));
                    } else {
                        inSymbol();
                        parseConstDeclValue(d.type, workidr->typ,
                                            workidr->high());
                    }
                    if (workidr->typ == voidType) {
                        error(errNoConstant);
                        workidr->typ = IntegerType;
                        workidr->value() = 1;
                    }
                    moreConsts = SY == COMMA;
                    if (moreConsts) {
                        lookup2 = lookupMode = lookDef;
                        inSymbol();
                    }
                }
                checkSymAndRead(SEMICOLON);
            }
            lookup2 = lookupMode = lookUse;
            markTypeSym();
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
            lookup2 = lookupMode = lookDef;
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
                            offsetof(IdentRec, szBase));
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
        bool staticDecl = SY == STATICSY;
        if (staticDecl) {
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
        // The declarator's name is being defined, so it is read in the mode
        // that looks only in this scope -- the one that sets isDefined, and
        // d.wasDefined with it.  It has to be in force before parseTypeRef,
        // whose own trailing inSymbol reads that name.  The typedef path does
        // the same for the same reason.
        lookup2 = lookupMode = lookDef;
        {
            parseTypeRef typeParser(baseTy, skipToSet | Bits(IDENT, MUL, LPAREN, COMMA) | Bits(SEMICOLON));
            packedFlag = typeParser.isPacked;
            forwardRef = typeParser.isForwardRef;
        }
        noExtentOk = true;
        Declarator d = parseOneDeclarator(baseTy, packedFlag, forwardRef);
        lookup2 = lookUse;
        noExtentOk = false;
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
        if (d.ptrOnly and SY == LPAREN and d.foundRec and
            d.foundRec->id == d.name and
            d.foundRec->pck.cl == ROUTINEID and
            d.foundRec->list() == NULL and
            d.foundRec->preDefLink() and
            // The whole header is restated, so the return type must agree.
            // A disagreement falls through to the "previous declaration was
            // not a forward declaration" arm below.
            (d.foundRec->typ == typedRetType)) {
            isPredefined = true;
        } else if (d.ptrOnly and SY == LPAREN and d.foundRec and
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
        if (staticDecl and isRoutine)
            error(errBadSymbol);
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
                    if (curIdent == toText("*INPUT*") or curIdent == toText("*OUTPUT*"))
                        error(errIdentAlreadyDefined);
                    else
                        defExtern();
                }
                /* Declaring a name this scope already has is an error, not a
                   silent shadow: the record below would go in front of the
                   older one, which keeps storage of its own and becomes
                   unreachable.  Shadowing an outer scope stays legal --
                   lookDef stops its walk at the first record from one. */
                if (d.wasDefined)
                    error(errIdentAlreadyDefined);
                curIdRec = besm6_alloc_record<IdentRec>(
                    offsetof(IdentRec, szIdent));
                curIdRec->id = d.name;
                curIdRec->pck.offset = curFrameRegTemplate;
                curIdRec->pck.nidx = ord(symHash[d.bucket]);
                curIdRec->pck.cl = staticDecl ? STATICID : VARID;
                curIdRec->list() = NULL;
                curIdRec->typ = d.type;
                symHash[d.bucket] = curIdRec;
                jj = typeSize(d.type);
                /* 'int a[] = { ... }' is sized by its initializer, which has
                   not been read yet, so it allocates nothing here and comes
                   back for its storage below.  Without an initializer there
                   is nothing to take a size from. */
                if (d.type.p.pk == kindArray and d.type.rep()->asize == 0 and
                    SY != BECOMES)
                    error(64); /* errIncorrectRangeDefinition */
                l2bool8z = true;
                if (curProcNesting == 1) {
                    curExternFile = externFileList;
                    toAlloc = (jj & halfWord) | 047000000;
                    while (l2bool8z and curExternFile) {
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
                    if (curProcNesting == 1 or staticDecl)
                        curIdRec->value() = moduleDataSize;
                    else
                        curIdRec->value() = localSize;
                    if (PASINFOR.listMode == 3) {
                        /* 25, not 26: work.p2c writes 'VARIABLE ':26, and
                           the runtime counts the literal's terminator in the
                           padding without printing it, so a width of w comes
                           out w-1 columns wide. */
                        printf("%25s", "VARIABLE ");
                        printTextWord(d.name);
                        printf(" OFFSET (%ld) %05loB. WORDS=%05loB\n",
                               curProcNesting, curIdRec->value(), jj);
                    }
                    if (curProcNesting == 1 or staticDecl) {
                        moduleDataSize = moduleDataSize + jj;
                        if (curProcNesting == 1)
                            localSize = moduleDataSize;
                    } else
                        localSize = localSize + jj;
                    curExternFile = NULL;
                }
                if (SY == BECOMES) {
                    if (curProcNesting != 1 and not staticDecl)
                        error(errVarTooComplex); /* load-time init: globals only */
                    parseInitializer(curIdRec);
                    if (d.type.p.pk == kindArray and
                        d.type.rep()->asize == 0) {
                        /* The initializer places whole words, so a packed
                           array holds perword elements in each of them and
                           an unpacked one an element every typeSize words. */
                        if (d.type.p.pad)
                            jj = initWords * d.type.rep()->perword;
                        else
                            jj = initWords / typeSize(d.type.rep()->base);
                        curIdRec->typ = makeArrayType(jj, d.type.rep()->base,
                                                      d.type.p.pad != 0);
                        if (curProcNesting == 1 or staticDecl) {
                            moduleDataSize = moduleDataSize +
                                             typeSize(curIdRec->typ);
                            if (curProcNesting == 1)
                                localSize = moduleDataSize;
                        } else
                            localSize = localSize + typeSize(curIdRec->typ);
                    }
                }
                moreDecls = (SY == COMMA);
                if (moreDecls) {
                    lookup2 = lookupMode = lookDef;   // the next name is a definition too
                    inSymbol();
                    noExtentOk = true;
                    d = parseOneDeclarator(baseTy, packedFlag, forwardRef);
                    lookup2 = lookupMode = lookUse;
                    noExtentOk = false;
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
                curIdRec->setSig(NULL);
                curIdRec->setPreDef(NULL);
                if (declEntry)
                    curIdRec->flags() = BitRange(0,15) | Bits(22);
                else
                    curIdRec->flags() = BitRange(0,15);
                curIdRec->r1.pos = 0;
                curFrameRegTemplate = curFrameRegTemplate + frameRegTemplate;
                if (done)
                    l2int18z = 8;
                else
                    l2int18z = 9;
                curProcNesting = curProcNesting + 1;
                /* The display takes one index register per level, and
                   freeRegs is [curProcNesting+1:6]: level 6 would leave the
                   expression evaluator none. */
                if (5 < curProcNesting)
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
                l2int18z = hashTravPtr->r2.level;
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
                    workidr->setPreDef(hashTravPtr->preDefLink());
                }
                hashTravPtr->setPreDef(NULL);
                curIdRec = hashTravPtr;
                // The definition restates the parameter list; it is matched
                // against the signature the declaration left behind, which
                // also settled the frame layout.
                hadParens = (SY == LPAREN);
                if (hadParens)
                    parseParameters(curIdRec->sig());
            } /* 23224 */
            if (SY == BEGINSY) {
                if (not hadParens)
                    error(42); /* errNoParamList */
                setup(scopeBound);
                makeFormals();
                inSymbol();
                programme(l2int18z, curIdRec, true);
                // The formals are above the mark now, so the hash buckets
                // have to be walked before the rollback releases them.
                exitScope(symHash);
                exitScope(fieldHash);
                myrollup(scopeBound);
                goto L23301;
            }
            if (SY == EXTERNSY or
                (SY == IDENT and
                 (curIdent == toText("FORTRAN") or curIdent == toText("ASSEMBLE")))) {
                if (SY == EXTERNSY) {
                    curVal.ii = Bits(20);
                } else if (curIdent == toText("ASSEMBLE")) {
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
                curIdRec->r2.level = l2int18z;
                curIdRec->setPreDef(preDefHead);
                preDefHead = curIdRec;
            }
L23301:
            /* Nothing to unhash: a formal is entered in the symbol table
               only by makeFormals, above the scope mark, and exitScope has
               already dropped it.  A declaration without a body never made
               one at all. */
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
    } else if (CH and not has(blockBegSys, SY) and
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
        while (curExternFile) {
            if (curExternFile->line == 0) {
                error(80); /* errUndefinedExternFile */
                printTextWord(curExternFile->id);
                putchar('\n');
            }
            curExternFile = curExternFile->next;
        }
    }
    if (preDefHead != uProcPtr)  {
        error(85); /* errNotFullyDefinedProcedures */
        while (preDefHead != uProcPtr) {
            printTextWord(preDefHead->id);
            preDefHead = preDefHead->preDefLink();
        }
        putchar('\n');
    }
    lookup2 = lookupMode = lookUse;
    if (curProcNesting == 1)
        localSize = moduleDataSize;
    defineRoutine(bodyBlock_);
    if (curProcNesting > 1 and
        not retSeen and (procName->typ != voidType)) {
        printf(" above function must return a value\n");
        error(200);
    }
    done = true;
    while (numLabTop > labFence) {
        numLabTop = numLabTop - 1;
        if (not numLabs[numLabTop].defined) {
            putchar(' ');
            printTextWord(numLabs[numLabTop].id.ii);
            putchar(':');
            done = false;
        }
    }
    if (not done) {
        putchar(' ');
        printTextWord(procName->id);
        putchar(' ');
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
    } /* initInsnTemplates */

    void regKeyWords() {
        static constexpr int64_t resWordNameBase[20] = {
                toText("CONST"),
                toText("TYPEDEF"),
                toText("ENUM"),
                toText("**PACKED"),
                toText("STRUCT"),
                toText("IF"),
                toText("SWITCH"),
                toText("WHILE"),
                toText("FOR"),
                toText("GOTO"),
                toText("ELSE"),
                toText("DO"),
                toText("EXTERN"),
                toText("BREAK"),
                toText("CONTINUE"),
                toText("CASE"),
                toText("DEFAULT"),
                toText("UNION"),
                toText("RETURN"),
                toText("STATIC")};
        SY = CONSTSY;
        charClass = NOOP;
        // CONSTSY..STATICSY are the consecutive reserved-word table. TYPESY
        // sits before CONSTSY, outside this range; predefined type names are
        // registered as TYPESY keywords later, by initScalars.
        for (idx = 0; idx <= 19; ++idx) {
            regResWord(resWordNameBase[idx]);
            succ(SY);
        }
    } /* regKeyWords */

    void initArrays() {
        FcstCnt = 0;
        FcstTotal = 0;
        for (idx=1; idx <= 30; ++idx)
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
    symTab[1] = 041000000 | curVal.ii;
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
    for (cnt = 0; cnt < longSymCnt; ++cnt) {
        idx = longSymTabBase[cnt] - 074000;
        symTab[idx] |= curVal.ii & leftAddr;
        curVal.ii = (curVal.ii + 0100000000L);
    }
    symTabPos = symTabPos - 1;
    for (cnt = 0; cnt <= symTabPos - 074000; ++cnt)
        CHILD.push_back(symTab[cnt]);
    for (cnt = 0; cnt < longSymCnt; ++cnt)
        CHILD.push_back(longSyms[cnt]);
    if (PASINFOR.listMode) {
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
    printf("    -u- -u+             Set length of source lines: 120 or 72 columns\n");
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
    eofOverreads = 0;
    checkFortran = false;
    bool110z = false;
    lookup2 = lookupMode = lookUse;
    moduleOffset = 16384;
    lineStartOffset = 16384;
    condLabCnt = 1;
    heapSize = 100;
    forValue = true;
    atEOL = false;
    checkTypes = true;
    declEntry = false;
    enableStdInput = false;
    errors = false;
    sortFcst = false;
    fileBufSize = 1;
    charEncoding = 2;
    longSymCnt = 0;
    symTabCnt = 0;

    // Get base name of the program.
    progname = strrchr(argv[0], '/');
    progname = progname ? progname+1 : argv[0];

    for (;;) {
        switch (getopt(argc, argv, "vVhiFH:e:c:r:u:f:a:k:b:l:")) {
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
        case 'r':
            // No fuzzy real comparison; the option is a no-op.
            continue;
        case 'u':
            // Source line length is a compile-time constant (maxLineLen),
            // with no runtime override, so this option is a no-op.
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
    if (strcmp(argv[0], "-")) {
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
    blockBegSys = Bits(CONSTSY, TYPEDEFSY, TYPESY) |
                  Bits(STATICSY, BEGINSY);
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
    if (PASINFOR.listMode)
        printf("%s\n", boilerplate);
    printf(" INITHEAP = %05lo\n", avail);
    curInsnTemplate = 0;
    initTables();
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

int64_t helperNames[31] = { 0L,
        toText("P/1     "),
        toText("C/2     "),
        toText("C/3     "),
        toText("C/4     "),
        toText("C/5     "),
        toText("C/MD    "),
        toText("P/MI    "),
        toText("C/DI    "),
        toText("P/1D    "),
/*10*/  toText("P/GD    "),
        toText("C/E     "),
        toText("C/EF    "),
        toText("P/NW    "),
        toText("P/RR    "),
        toText("C/TR    "),
        toText("FOPEN   "),
        toText("FCLOSE  "),
        toText("P/IT    "),
        toText("C/LNGPAR"),
/*20*/  toText("P/LDAR  "),
        toText("P/00C   "),
        toText("P/STAR  "),
        toText("P/EQ    "),
        toText("P/GE    "),
        toText("P/MF    "),
        toText("P/FM    "),
        toText("P/NN    "),
        toText("C/SHL   "),
        toText("C/SHR   "),
/*30*/  toText("C/PRINTF")};
