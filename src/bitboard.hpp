#ifndef APERY_BITBOARD_HPP
#define APERY_BITBOARD_HPP

#include "common.hpp"
#include "square.hpp"
#include "color.hpp"

class Bitboard;
extern const Bitboard SetMaskBB[SquareNum];

class Bitboard {
public:
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
	Bitboard& operator = (const Bitboard& rhs) {
		_mm_store_si128(&this->m_, rhs.m_);
		return *this;
	}
	Bitboard(const Bitboard& bb) {
		_mm_store_si128(&this->m_, bb.m_);
	}
#endif
	Bitboard() {}
	Bitboard(const u64 v0, const u64 v1) {
		this->p_[0] = v0;
		this->p_[1] = v1;
	}
	u64 p(const int index) const { return p_[index]; }
	void set(const int index, const u64 val) { p_[index] = val; }
	u64 merge() const { return this->p(0) | this->p(1); }
	bool isNot0() const {
#ifdef HAVE_SSE4
		return !(_mm_testz_si128(this->m_, _mm_set1_epi8(static_cast<char>(0xffu))));
#else
		return (this->merge() ? true : false);
#endif
	}
	// これはコードが見難くなるけど仕方ない。
	bool andIsNot0(const Bitboard& bb) const {
#ifdef HAVE_SSE4
		return !(_mm_testz_si128(this->m_, bb.m_));
#else
		return (*this & bb).isNot0();
#endif
	}
	Bitboard operator ~ () const {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		Bitboard tmp;
		_mm_store_si128(&tmp.m_, _mm_andnot_si128(this->m_, _mm_set1_epi8(static_cast<char>(0xffu))));
		return tmp;
#else
		return Bitboard(~this->p(0), ~this->p(1));
#endif
	}
	Bitboard operator &= (const Bitboard& rhs) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_and_si128(this->m_, rhs.m_));
#else
		this->p_[0] &= rhs.p(0);
		this->p_[1] &= rhs.p(1);
#endif
		return *this;
	}
	Bitboard operator |= (const Bitboard& rhs) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_or_si128(this->m_, rhs.m_));
#else
		this->p_[0] |= rhs.p(0);
		this->p_[1] |= rhs.p(1);
#endif
		return *this;
	}
	Bitboard operator ^= (const Bitboard& rhs) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_xor_si128(this->m_, rhs.m_));
#else
		this->p_[0] ^= rhs.p(0);
		this->p_[1] ^= rhs.p(1);
#endif
		return *this;
	}
	Bitboard operator <<= (const int i) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_slli_epi64(this->m_, i));
#else
		this->p_[0] <<= i;
		this->p_[1] <<= i;
#endif
		return *this;
	}
	Bitboard operator >>= (const int i) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_srli_epi64(this->m_, i));
#else
		this->p_[0] >>= i;
		this->p_[1] >>= i;
#endif
		return *this;
	}
	Bitboard operator & (const Bitboard& rhs) const { return Bitboard(*this) &= rhs; }
	Bitboard operator | (const Bitboard& rhs) const { return Bitboard(*this) |= rhs; }
	Bitboard operator ^ (const Bitboard& rhs) const { return Bitboard(*this) ^= rhs; }
	Bitboard operator << (const int i) const { return Bitboard(*this) <<= i; }
	Bitboard operator >> (const int i) const { return Bitboard(*this) >>= i; }
	bool operator == (const Bitboard& rhs) const {
#ifdef HAVE_SSE4
		return (_mm_testc_si128(_mm_cmpeq_epi8(this->m_, rhs.m_), _mm_set1_epi8(static_cast<char>(0xffu))) ? true : false);
#else
		return (this->p(0) == rhs.p(0)) && (this->p(1) == rhs.p(1));
#endif
	}
	bool operator != (const Bitboard& rhs) const { return !(*this == rhs); }
	// これはコードが見難くなるけど仕方ない。
	Bitboard andEqualNot(const Bitboard& bb) {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		_mm_store_si128(&this->m_, _mm_andnot_si128(bb.m_, this->m_));
#else
		(*this) &= ~bb;
#endif
		return *this;
	}
	// これはコードが見難くなるけど仕方ない。
	Bitboard notThisAnd(const Bitboard& bb) const {
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
		Bitboard temp;
		_mm_store_si128(&temp.m_, _mm_andnot_si128(this->m_, bb.m_));
		return temp;
#else
		return ~(*this) & bb;
#endif
	}
	bool isSet(const Square sq) const {
		assert(isInSquare(sq));
		return andIsNot0(SetMaskBB[sq]);
	}
	void setBit(const Square sq) { *this |= SetMaskBB[sq]; }
	void clearBit(const Square sq) { andEqualNot(SetMaskBB[sq]); }
	void xorBit(const Square sq) { (*this) ^= SetMaskBB[sq]; }
	void xorBit(const Square sq1, const Square sq2) { (*this) ^= (SetMaskBB[sq1] | SetMaskBB[sq2]); }
	// Bitboard の right 側だけの要素を調べて、最初に 1 であるマスの index を返す。
	// そのマスを 0 にする。
	// Bitboard の right 側が 0 でないことを前提にしている。
	FORCE_INLINE Square firstOneRightFromSQ11() {
		const Square sq = static_cast<Square>(firstOneFromLSB(this->p(0)));
		// LSB 側の最初の 1 の bit を 0 にする
		this->p_[0] &= this->p(0) - 1;
		return sq;
	}
	// Bitboard の left 側だけの要素を調べて、最初に 1 であるマスの index を返す。
	// そのマスを 0 にする。
	// Bitboard の left 側が 0 でないことを前提にしている。
	FORCE_INLINE Square firstOneLeftFromSQ81() {
		const Square sq = static_cast<Square>(firstOneFromLSB(this->p(1)) + 63);
		// LSB 側の最初の 1 の bit を 0 にする
		this->p_[1] &= this->p(1) - 1;
		return sq;
	}
	// Bitboard を SQ11 から SQ99 まで調べて、最初に 1 であるマスの index を返す。
	// そのマスを 0 にする。
	// Bitboard が allZeroBB() でないことを前提にしている。
	// VC++ の _BitScanForward() は入力が 0 のときに 0 を返す仕様なので、
	// 最初に 0 でないか判定するのは少し損。
	FORCE_INLINE Square firstOneFromSQ11() {
		if (this->p(0))
			return firstOneRightFromSQ11();
		return firstOneLeftFromSQ81();
	}
	// 返す位置を 0 にしないバージョン。
	FORCE_INLINE Square constFirstOneRightFromSQ11() const { return static_cast<Square>(firstOneFromLSB(this->p(0))); }
	FORCE_INLINE Square constFirstOneLeftFromSQ81() const { return static_cast<Square>(firstOneFromLSB(this->p(1)) + 63); }
	FORCE_INLINE Square constFirstOneFromSQ11() const {
		if (this->p(0))
			return constFirstOneRightFromSQ11();
		return constFirstOneLeftFromSQ81();
	}
	// Bitboard の 1 の bit を数える。
	// Crossover は、merge() すると 1 である bit が重なる可能性があるなら true
	template <bool Crossover = true>
	int popCount() const { return (Crossover ? count1s(p(0)) + count1s(p(1)) : count1s(merge())); }
	// bit が 1 つだけ立っているかどうかを判定する。
	// Crossover は、merge() すると 1 である bit が重なる可能性があるなら true
	template <bool Crossover = true>
	bool isOneBit() const {
#if defined (HAVE_SSE42)
		return (this->popCount<Crossover>() == 1);
#else
		if (!this->isNot0())
			return false;
		else if (this->p(0))
			return !((this->p(0) & (this->p(0) - 1)) | this->p(1));
		return !(this->p(1) & (this->p(1) - 1));
#endif
	}

	// for debug
	void printBoard() const {
		std::cout << "   A  B  C  D  E  F  G  H  I\n";
		for (Rank r = Rank1; r < RankNum; ++r) {
			std::cout << (9 - r);
			for (File f = File9; File1 <= f; --f)
				std::cout << (this->isSet(makeSquare(f, r)) ? "  X" : "  .");
			std::cout << "\n";
		}

		std::cout << std::endl;
	}

	void printTable(const int part) const {
		for (Rank r = Rank1; r < RankNum; ++r) {
			for (File f = File7; File1 <= f; --f)
				std::cout << (UINT64_C(1) & (this->p(part) >> makeSquare(f, r)));
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}

	// 指定した位置が Bitboard のどちらの u64 変数の要素か
	static int part(const Square sq) { return static_cast<int>(SQ79 < sq); }

private:
#if defined (HAVE_SSE2) || defined (HAVE_SSE4)
	union {
		u64 p_[2];
		__m128i m_;
	};
#else
	u64 p_[2];	// p_[0] : 先手から見て、1一から7九までを縦に並べたbit. 63bit使用. right と呼ぶ。
				// p_[1] : 先手から見て、8一から1九までを縦に並べたbit. 18bit使用. left  と呼ぶ。
#endif
};

inline Bitboard setMaskBB(const Square sq) { return SetMaskBB[sq]; }

// 実際に使用する部分の全て bit が立っている Bitboard
inline Bitboard allOneBB() { return Bitboard(UINT64_C(0x7fffffffffffffff), UINT64_C(0x000000000003ffff)); }
inline Bitboard allZeroBB() { return Bitboard(0, 0); }

extern const int RookBlockBits[SquareNum];
extern const int BishopBlockBits[SquareNum];
extern const int RookShiftBits[SquareNum];
extern const int BishopShiftBits[SquareNum];
#if defined HAVE_BMI2
#else
extern const u64 RookMagic[SquareNum];
extern const u64 BishopMagic[SquareNum];
#endif

// 指定した位置の属する file の bit を shift し、
// index を求める為に使用する。
extern const int Slide[SquareNum];

extern const Bitboard File1Mask;
extern const Bitboard File2Mask;
extern const Bitboard File3Mask;
extern const Bitboard File4Mask;
extern const Bitboard File5Mask;
extern const Bitboard File6Mask;
extern const Bitboard File7Mask;
extern const Bitboard File8Mask;
extern const Bitboard File9Mask;

extern const Bitboard Rank1Mask;
extern const Bitboard Rank2Mask;
extern const Bitboard Rank3Mask;
extern const Bitboard Rank4Mask;
extern const Bitboard Rank5Mask;
extern const Bitboard Rank6Mask;
extern const Bitboard Rank7Mask;
extern const Bitboard Rank8Mask;
extern const Bitboard Rank9Mask;

extern const Bitboard FileMask[FileNum];
extern const Bitboard RankMask[RankNum];
extern const Bitboard InFrontMask[ColorNum][RankNum];

inline Bitboard fileMask(const File f) { return FileMask[f]; }
template <File F> inline Bitboard fileMask() {
	static_assert(File1 <= F && F <= File9, "");
	return (F == File1 ? File1Mask
			: F == File2 ? File2Mask
			: F == File3 ? File3Mask
			: F == File4 ? File4Mask
			: F == File5 ? File5Mask
			: F == File6 ? File6Mask
			: F == File7 ? File7Mask
			: F == File8 ? File8Mask
			: /*F == File9 ?*/ File9Mask);
}

inline Bitboard rankMask(const Rank r) { return RankMask[r]; }
template <Rank R> inline Bitboard rankMask() {
	static_assert(Rank1 <= R && R <= Rank9, "");
	return (R == Rank1 ? Rank1Mask
			: R == Rank2 ? Rank2Mask
			: R == Rank3 ? Rank3Mask
			: R == Rank4 ? Rank4Mask
			: R == Rank5 ? Rank5Mask
			: R == Rank6 ? Rank6Mask
			: R == Rank7 ? Rank7Mask
			: R == Rank8 ? Rank8Mask
			: /*R == Rank9 ?*/ Rank9Mask);
}

// 直接テーブル引きすべきだと思う。
inline Bitboard squareFileMask(const Square sq) {
	const File f = makeFile(sq);
	return fileMask(f);
}

// 直接テーブル引きすべきだと思う。
inline Bitboard squareRankMask(const Square sq) {
	const Rank r = makeRank(sq);
	return rankMask(r);
}

extern const Bitboard InFrontOfRank1Black;
extern const Bitboard InFrontOfRank2Black;
extern const Bitboard InFrontOfRank3Black;
extern const Bitboard InFrontOfRank4Black;
extern const Bitboard InFrontOfRank5Black;
extern const Bitboard InFrontOfRank6Black;
extern const Bitboard InFrontOfRank7Black;
extern const Bitboard InFrontOfRank8Black;
extern const Bitboard InFrontOfRank9Black;

extern const Bitboard InFrontOfRank9White;
extern const Bitboard InFrontOfRank8White;
extern const Bitboard InFrontOfRank7White;
extern const Bitboard InFrontOfRank6White;
extern const Bitboard InFrontOfRank5White;
extern const Bitboard InFrontOfRank4White;
extern const Bitboard InFrontOfRank3White;
extern const Bitboard InFrontOfRank2White;
extern const Bitboard InFrontOfRank1White;

inline Bitboard inFrontMask(const Color c, const Rank r) { return InFrontMask[c][r]; }
template <Color C, Rank R> inline Bitboard inFrontMask() {
	static_assert(C == Black || C == White, "");
	static_assert(Rank1 <= R && R <= Rank9, "");
	return (C == Black ? (R == Rank1 ? InFrontOfRank1Black
						  : R == Rank2 ? InFrontOfRank2Black
						  : R == Rank3 ? InFrontOfRank3Black
						  : R == Rank4 ? InFrontOfRank4Black
						  : R == Rank5 ? InFrontOfRank5Black
						  : R == Rank6 ? InFrontOfRank6Black
						  : R == Rank7 ? InFrontOfRank7Black
						  : R == Rank8 ? InFrontOfRank8Black
						  : /*R == Rank9 ?*/ InFrontOfRank9Black)
			: (R == Rank1 ? InFrontOfRank1White
			   : R == Rank2 ? InFrontOfRank2White
			   : R == Rank3 ? InFrontOfRank3White
			   : R == Rank4 ? InFrontOfRank4White
			   : R == Rank5 ? InFrontOfRank5White
			   : R == Rank6 ? InFrontOfRank6White
			   : R == Rank7 ? InFrontOfRank7White
			   : R == Rank8 ? InFrontOfRank8White
			   : /*R == Rank9 ?*/ InFrontOfRank9White));
}

// メモリ節約の為、1次元配列にして無駄が無いようにしている。
#if defined HAVE_BMI2
extern Bitboard RookAttack[495616];
#else
extern Bitboard RookAttack[512000];
#endif
extern int RookAttackIndex[SquareNum];
// メモリ節約の為、1次元配列にして無駄が無いようにしている。
extern Bitboard BishopAttack[20224];
extern int BishopAttackIndex[SquareNum];
extern Bitboard RookBlockMask[SquareNum];
extern Bitboard BishopBlockMask[SquareNum];
// メモリ節約をせず、無駄なメモリを持っている。
extern Bitboard LanceAttack[ColorNum][SquareNum][128];

extern Bitboard KingAttack[SquareNum];
extern Bitboard GoldAttack[ColorNum][SquareNum];
extern Bitboard SilverAttack[ColorNum][SquareNum];
extern Bitboard KnightAttack[ColorNum][SquareNum];
extern Bitboard PawnAttack[ColorNum][SquareNum];

extern Bitboard BetweenBB[SquareNum][SquareNum];

extern Bitboard RookAttackToEdge[SquareNum];
extern Bitboard BishopAttackToEdge[SquareNum];
extern Bitboard LanceAttackToEdge[ColorNum][SquareNum];

extern Bitboard GoldCheckTable[ColorNum][SquareNum];
extern Bitboard SilverCheckTable[ColorNum][SquareNum];
extern Bitboard KnightCheckTable[ColorNum][SquareNum];
extern Bitboard LanceCheckTable[ColorNum][SquareNum];

#if defined HAVE_BMI2
// PEXT bitboard.
inline u64 occupiedToIndex(const Bitboard& block, const Bitboard& mask) {
	return _pext_u64(block.merge(), mask.merge());
}

inline Bitboard rookAttack(const Square sq, const Bitboard& occupied) {
	const Bitboard block(occupied & RookBlockMask[sq]);
	return RookAttack[RookAttackIndex[sq] + occupiedToIndex(block, RookBlockMask[sq])];
}
inline Bitboard bishopAttack(const Square sq, const Bitboard& occupied) {
	const Bitboard block(occupied & BishopBlockMask[sq]);
	return BishopAttack[BishopAttackIndex[sq] + occupiedToIndex(block, BishopBlockMask[sq])];
}
#else
// magic bitboard.
// magic number を使って block の模様から利きのテーブルへのインデックスを算出
inline u64 occupiedToIndex(const Bitboard& block, const u64 magic, const int shiftBits) {
	return (block.merge() * magic) >> shiftBits;
}

inline Bitboard rookAttack(const Square sq, const Bitboard& occupied) {
	const Bitboard block(occupied & RookBlockMask[sq]);
	return RookAttack[RookAttackIndex[sq] + occupiedToIndex(block, RookMagic[sq], RookShiftBits[sq])];
}
inline Bitboard bishopAttack(const Square sq, const Bitboard& occupied) {
	const Bitboard block(occupied & BishopBlockMask[sq]);
	return BishopAttack[BishopAttackIndex[sq] + occupiedToIndex(block, BishopMagic[sq], BishopShiftBits[sq])];
}
#endif
// todo: 香車の筋がどこにあるか先に分かっていれば、Bitboard の片方の変数だけを調べれば良くなる。
inline Bitboard lanceAttack(const Color c, const Square sq, const Bitboard& occupied) {
	const int part = Bitboard::part(sq);
	const int index = (occupied.p(part) >> Slide[sq]) & 127;
	return LanceAttack[c][sq][index];
}
// 飛車の縦だけの利き。香車の利きを使い、index を共通化することで高速化している。
inline Bitboard rookAttackFile(const Square sq, const Bitboard& occupied) {
	const int part = Bitboard::part(sq);
	const int index = (occupied.p(part) >> Slide[sq]) & 127;
	return LanceAttack[Black][sq][index] | LanceAttack[White][sq][index];
}
inline Bitboard goldAttack(const Color c, const Square sq) { return GoldAttack[c][sq]; }
inline Bitboard silverAttack(const Color c, const Square sq) { return SilverAttack[c][sq]; }
inline Bitboard knightAttack(const Color c, const Square sq) { return KnightAttack[c][sq]; }
inline Bitboard pawnAttack(const Color c, const Square sq) { return PawnAttack[c][sq]; }

// Bitboard で直接利きを返す関数。
// 1段目には歩は存在しないので、1bit シフトで別の筋に行くことはない。
// ただし、from に歩以外の駒の Bitboard を入れると、別の筋のビットが立ってしまうことがあるので、
// 別の筋のビットが立たないか、立っても問題ないかを確認して使用すること。
template <Color US> inline Bitboard pawnAttack(const Bitboard& from) { return (US == Black ? (from >> 1) : (from << 1)); }
inline Bitboard kingAttack(const Square sq) { return KingAttack[sq]; }
inline Bitboard dragonAttack(const Square sq, const Bitboard& occupied) { return rookAttack(sq, occupied) | kingAttack(sq); }
inline Bitboard horseAttack(const Square sq, const Bitboard& occupied) { return bishopAttack(sq, occupied) | kingAttack(sq); }
inline Bitboard queenAttack(const Square sq, const Bitboard& occupied) { return rookAttack(sq, occupied) | bishopAttack(sq, occupied); }

// sq1, sq2 の間(sq1, sq2 は含まない)のビットが立った Bitboard
inline Bitboard betweenBB(const Square sq1, const Square sq2) { return BetweenBB[sq1][sq2]; }
inline Bitboard rookAttackToEdge(const Square sq) { return RookAttackToEdge[sq]; }
inline Bitboard bishopAttackToEdge(const Square sq) { return BishopAttackToEdge[sq]; }
inline Bitboard lanceAttackToEdge(const Color c, const Square sq) { return LanceAttackToEdge[c][sq]; }
inline Bitboard dragonAttackToEdge(const Square sq) { return rookAttackToEdge(sq) | kingAttack(sq); }
inline Bitboard horseAttackToEdge(const Square sq) { return bishopAttackToEdge(sq) | kingAttack(sq); }
inline Bitboard goldCheckTable(const Color c, const Square sq) { return GoldCheckTable[c][sq]; }
inline Bitboard silverCheckTable(const Color c, const Square sq) { return SilverCheckTable[c][sq]; }
inline Bitboard knightCheckTable(const Color c, const Square sq) { return KnightCheckTable[c][sq]; }
inline Bitboard lanceCheckTable(const Color c, const Square sq) { return LanceCheckTable[c][sq]; }
// todo: テーブル引きを検討
inline Bitboard rookStepAttacks(const Square sq) { return goldAttack(Black, sq) & goldAttack(White, sq); }
// todo: テーブル引きを検討
inline Bitboard bishopStepAttacks(const Square sq) { return silverAttack(Black, sq) & silverAttack(White, sq); }
// 前方3方向の位置のBitboard
inline Bitboard goldAndSilverAttacks(const Color c, const Square sq) { return goldAttack(c, sq) & silverAttack(c, sq); }

// Bitboard の全ての bit に対して同様の処理を行う際に使用するマクロ
// xxx に処理を書く。
// xxx には template 引数を 2 つ以上持つクラスや関数は () でくくらないと使えない。
// これはマクロの制約。
// 同じ処理のコードが 2 箇所で生成されるため、コードサイズが膨れ上がる。
// その為、あまり多用すべきでないかも知れない。
#define FOREACH_BB(bb, sq, xxx)						\
	do {											\
		while (bb.p(0)) {							\
			sq = bb.firstOneRightFromSQ11();		\
			xxx;									\
		}											\
		while (bb.p(1)) {							\
			sq = bb.firstOneLeftFromSQ81();			\
			xxx;									\
		}											\
	} while (false)

template <typename T> FORCE_INLINE void foreachBB(Bitboard& bb, Square& sq, T t) {
	while (bb.p(0)) {
		sq = bb.firstOneRightFromSQ11();
		t(0);
	}
	while (bb.p(1)) {
		sq = bb.firstOneLeftFromSQ81();
		t(1);
	}
}

#endif // #ifndef APERY_BITBOARD_HPP
