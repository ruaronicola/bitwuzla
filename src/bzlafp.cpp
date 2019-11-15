/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2019 Aina Niemetz.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#include <unordered_map>

extern "C" {
#include "bzlaabort.h"
#include "bzlaexp.h"
#include "bzlafp.h"
#include "bzlanode.h"
}

#ifdef BZLA_USE_SYMFPU
#include "symfpu/core/unpackedFloat.h"
#endif

#define BZLA_FP_RM_BW 3

/* ========================================================================== */
/* Glue for SymFPU.                                                           */
/* ========================================================================== */

class BzlaFPWordBlaster;
class BzlaFPSymRM;
class BzlaFPSortInfo;
class BzlaFPSymProp;
template <bool T>
class BzlaFPSymBV;

/* Use the same type names as SymFPU. */
typedef uint32_t bwt;

/* Mapping between sorts. */
template <bool T>
struct BzlaSignedToLitSort;
template <>
struct BzlaSignedToLitSort<true>
{
  typedef int32_t BzlaLitSort;
};
template <>
struct BzlaSignedToLitSort<false>
{
  typedef uint32_t BzlaLitSort;
};

/* -------------------------------------------------------------------------- */
/* Bitwuzla wrapper for floating-point sorts.                                */
/* -------------------------------------------------------------------------- */

class BzlaFPSortInfo
{
  friend BzlaFPWordBlaster;

 public:
  BzlaFPSortInfo(const BzlaSortId sort);
  BzlaFPSortInfo(uint32_t ewidth, uint32_t swidth);
  BzlaFPSortInfo(const BzlaFPSortInfo &other);
  ~BzlaFPSortInfo();

  BzlaSortId getSort(void) const;

 private:
  BzlaSortId d_sort;
  static Bzla *s_bzla;
};

/* -------------------------------------------------------------------------- */

Bzla *BzlaFPSortInfo::s_bzla = nullptr;

BzlaFPSortInfo::BzlaFPSortInfo(const BzlaSortId sort)
{
  assert(s_bzla);
  assert(bzla_sort_is_fp(s_bzla, sort));
  d_sort = bzla_sort_copy(s_bzla, sort);
}

BzlaFPSortInfo::BzlaFPSortInfo(uint32_t ewidth, uint32_t swidth)
{
  assert(s_bzla);
  d_sort = bzla_sort_fp(s_bzla, ewidth, swidth);
}

BzlaFPSortInfo::BzlaFPSortInfo(const BzlaFPSortInfo &other)
{
  assert(s_bzla);
  assert(other.d_sort);
  assert(bzla_sort_is_fp(s_bzla, other.d_sort));
  d_sort = bzla_sort_copy(s_bzla, other.d_sort);
}

BzlaFPSortInfo::~BzlaFPSortInfo()
{
  assert(s_bzla);
  bzla_sort_release(s_bzla, d_sort);
}

BzlaSortId
BzlaFPSortInfo::getSort(void) const
{
  assert(d_sort);
  return d_sort;
}

/* -------------------------------------------------------------------------- */
/* Bitwuzla wrapper for propositions.                                        */
/* -------------------------------------------------------------------------- */

class BzlaFPSymProp
{
  friend BzlaFPWordBlaster;
  friend BzlaFPSymBV<true>;
  friend BzlaFPSymBV<false>;
#ifdef BZLA_USE_SYMFPU
  friend ::symfpu::ite<BzlaFPSymProp, BzlaFPSymProp>;
#endif

 public:
  BzlaFPSymProp(BzlaNode *node);
  BzlaFPSymProp(bool v);
  BzlaFPSymProp(const BzlaFPSymProp &other);
  ~BzlaFPSymProp();

  BzlaFPSymProp operator!(void) const;
  BzlaFPSymProp operator&&(const BzlaFPSymProp &op) const;
  BzlaFPSymProp operator||(const BzlaFPSymProp &op) const;
  BzlaFPSymProp operator==(const BzlaFPSymProp &op) const;
  BzlaFPSymProp operator^(const BzlaFPSymProp &op) const;

 protected:
  bool checkNode(const BzlaNode *node) const;

 private:
  BzlaNode *d_node;
  static Bzla *s_bzla;
};

/* -------------------------------------------------------------------------- */

Bzla *BzlaFPSymProp::s_bzla = nullptr;

BzlaFPSymProp::BzlaFPSymProp(BzlaNode *node)
{
  assert(s_bzla);
  assert(checkNode(node));
  d_node = bzla_node_copy(s_bzla, node);
}

BzlaFPSymProp::BzlaFPSymProp(bool v)
{
  assert(s_bzla);
  d_node = v ? bzla_exp_true(s_bzla) : bzla_exp_false(s_bzla);
}

BzlaFPSymProp::BzlaFPSymProp(const BzlaFPSymProp &other)
{
  assert(s_bzla);
  assert(checkNode(other.d_node));
  d_node = bzla_node_copy(s_bzla, other.d_node);
}

BzlaFPSymProp::~BzlaFPSymProp()
{
  assert(s_bzla);
  bzla_node_release(s_bzla, d_node);
}

BzlaFPSymProp
BzlaFPSymProp::operator!(void) const
{
  assert(s_bzla);
  BzlaNode *n       = bzla_exp_bv_not(s_bzla, d_node);
  BzlaFPSymProp res = BzlaFPSymProp(n);
  bzla_node_release(s_bzla, n);
  return res;
}

BzlaFPSymProp
BzlaFPSymProp::operator&&(const BzlaFPSymProp &op) const
{
  assert(s_bzla);
  assert(checkNode(op.d_node));
  BzlaNode *n       = bzla_exp_bv_and(s_bzla, d_node, op.d_node);
  BzlaFPSymProp res = BzlaFPSymProp(n);
  bzla_node_release(s_bzla, n);
  return res;
}

BzlaFPSymProp
BzlaFPSymProp::operator||(const BzlaFPSymProp &op) const
{
  assert(s_bzla);
  assert(checkNode(op.d_node));
  BzlaNode *n       = bzla_exp_bv_or(s_bzla, d_node, op.d_node);
  BzlaFPSymProp res = BzlaFPSymProp(n);
  bzla_node_release(s_bzla, n);
  return res;
}

BzlaFPSymProp
BzlaFPSymProp::operator==(const BzlaFPSymProp &op) const
{
  assert(s_bzla);
  assert(checkNode(op.d_node));
  BzlaNode *n       = bzla_exp_eq(s_bzla, d_node, op.d_node);
  BzlaFPSymProp res = BzlaFPSymProp(n);
  bzla_node_release(s_bzla, n);
  return res;
}

BzlaFPSymProp
BzlaFPSymProp::operator^(const BzlaFPSymProp &op) const
{
  assert(s_bzla);
  assert(checkNode(op.d_node));
  BzlaNode *n       = bzla_exp_bv_xor(s_bzla, d_node, op.d_node);
  BzlaFPSymProp res = BzlaFPSymProp(n);
  bzla_node_release(s_bzla, n);
  return res;
}

bool
BzlaFPSymProp::checkNode(const BzlaNode *node) const
{
  assert(checkNode(node));
  return bzla_sort_is_bv(node->bzla, bzla_node_get_sort_id(node))
         && bzla_node_bv_get_width(s_bzla, node) == 1;
}

/* -------------------------------------------------------------------------- */
/* Bitwuzla wrapper for bit-vector terms.                                    */
/* -------------------------------------------------------------------------- */

template <bool is_signed>
class BzlaFPSymBV
{
  friend BzlaFPWordBlaster;
  friend BzlaFPSymBV<!is_signed>; /* Allow conversion between the sorts. */
#ifdef BZLA_USE_SYMFPU
  friend ::symfpu::ite<BzlaFPSymProp, BzlaFPSymBV<is_signed> >;
#endif

 public:
  BzlaFPSymBV(BzlaNode *node);
  BzlaFPSymBV(const bwt w, const uint32_t val);
  BzlaFPSymBV(const BzlaFPSymProp &p);
  BzlaFPSymBV(const BzlaFPSymBV<is_signed> &other);
  BzlaFPSymBV(const BzlaBitVector *bv);
  ~BzlaFPSymBV();

  bwt getWidth(void) const;
  BzlaNode *getNode(void) const { return d_node; }

  /*** Constant creation and test ***/
  static BzlaFPSymBV<is_signed> one(const bwt &w);
  static BzlaFPSymBV<is_signed> zero(const bwt &w);
  static BzlaFPSymBV<is_signed> allOnes(const bwt &w);

  BzlaFPSymProp isAllOnes() const;
  BzlaFPSymProp isAllZeros() const;

  static BzlaFPSymBV<is_signed> maxValue(const bwt &w);
  static BzlaFPSymBV<is_signed> minValue(const bwt &w);

  /*** Operators ***/
  BzlaFPSymBV<is_signed> operator<<(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> operator>>(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> operator|(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> operator&(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> operator+(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> operator-(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> operator*(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> operator/(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> operator%(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> operator-(void) const;
  BzlaFPSymBV<is_signed> operator~(void) const;
  BzlaFPSymBV<is_signed> increment() const;
  BzlaFPSymBV<is_signed> decrement() const;
  BzlaFPSymBV<is_signed> signExtendRightShift(
      const BzlaFPSymBV<is_signed> &op) const;

  /*** Modular operations ***/
  // This back-end doesn't do any overflow checking so these are the same as
  // other operations
  BzlaFPSymBV<is_signed> modularLeftShift(
      const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> modularRightShift(
      const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> modularIncrement() const;
  BzlaFPSymBV<is_signed> modularDecrement() const;
  BzlaFPSymBV<is_signed> modularAdd(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> modularNegate() const;

  /*** Comparisons ***/
  BzlaFPSymProp operator==(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymProp operator<=(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymProp operator>=(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymProp operator<(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymProp operator>(const BzlaFPSymBV<is_signed> &op) const;

  /*** Type conversion ***/
  // Bitwuzla nodes make no distinction between signed and unsigned, thus these
  // are trivial
  BzlaFPSymBV<true> toSigned(void) const;
  BzlaFPSymBV<false> toUnsigned(void) const;

  /*** Bit hacks ***/
  BzlaFPSymBV<is_signed> extend(bwt extension) const;
  BzlaFPSymBV<is_signed> contract(bwt reduction) const;
  BzlaFPSymBV<is_signed> resize(bwt newSize) const;
  BzlaFPSymBV<is_signed> matchWidth(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> append(const BzlaFPSymBV<is_signed> &op) const;
  BzlaFPSymBV<is_signed> extract(bwt upper, bwt lower) const;

 protected:
  typedef typename BzlaSignedToLitSort<is_signed>::BzlaLitSort literalType;

  // BzlaNode* boolNodeToBV(BzlaNode* node) const;
  // BzlaNode* BVToBoolNode(BzlaNode* node) const;

  bool checkNode(const BzlaNode *node) const;
  // BzlaNode *fromProposition (BzlaNode *node) const;
  // BzlaNode *toProposition (BzlaNode *node) const;

 private:
  BzlaNode *d_node;
  static Bzla *s_bzla;
};

/* -------------------------------------------------------------------------- */

template <>
Bzla *BzlaFPSymBV<true>::s_bzla = nullptr;
template <>
Bzla *BzlaFPSymBV<false>::s_bzla = nullptr;

template <bool is_signed>
BzlaFPSymBV<is_signed>::BzlaFPSymBV(BzlaNode *node)
{
  assert(s_bzla);
  assert(checkNode(node));
  d_node = bzla_node_copy(s_bzla, node);
}

template <bool is_signed>
BzlaFPSymBV<is_signed>::BzlaFPSymBV(const bwt w, const uint32_t val)
{
  assert(s_bzla);
  BzlaSortId s = bzla_sort_bv(s_bzla, w);
  d_node       = is_signed ? bzla_exp_bv_int(s_bzla, val, s)
                     : bzla_exp_bv_unsigned(s_bzla, val, s);
  bzla_sort_release(s_bzla, s);
}

template <bool is_signed>
BzlaFPSymBV<is_signed>::BzlaFPSymBV(const BzlaFPSymProp &p)
{
  assert(s_bzla);
  assert(p.d_node);
  assert(bzla_sort_bv_get_width(s_bzla, bzla_node_get_sort_id(p.d_node)));
  d_node = bzla_node_copy(s_bzla, p.d_node);
}

template <bool is_signed>
BzlaFPSymBV<is_signed>::BzlaFPSymBV(const BzlaFPSymBV<is_signed> &other)
{
  assert(s_bzla);
  assert(checkNode(other.d_node));
  d_node = bzla_node_copy(s_bzla, other.d_node);
}

template <bool is_signed>
BzlaFPSymBV<is_signed>::BzlaFPSymBV(const BzlaBitVector *bv)
{
  assert(s_bzla);
  d_node = bzla_exp_bv_const(s_bzla, bv);
}

template <bool is_signed>
BzlaFPSymBV<is_signed>::~BzlaFPSymBV()
{
  assert(s_bzla);
  bzla_node_release(s_bzla, d_node);
}

template <bool is_signed>
bwt
BzlaFPSymBV<is_signed>::getWidth(void) const
{
  assert(s_bzla);
  return bzla_node_bv_get_width(s_bzla, d_node);
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::one(const bwt &w)
{
  assert(s_bzla);
  BzlaSortId s               = bzla_sort_bv(s_bzla, w);
  BzlaNode *n                = bzla_exp_bv_one(s_bzla, s);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  bzla_sort_release(s_bzla, s);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::zero(const bwt &w)
{
  assert(s_bzla);
  BzlaSortId s               = bzla_sort_bv(s_bzla, w);
  BzlaNode *n                = bzla_exp_bv_zero(s_bzla, s);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  bzla_sort_release(s_bzla, s);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::allOnes(const bwt &w)
{
  assert(s_bzla);
  BzlaSortId s               = bzla_sort_bv(s_bzla, w);
  BzlaNode *n                = bzla_exp_bv_ones(s_bzla, s);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  bzla_sort_release(s_bzla, s);
  return res;
}

template <bool is_signed>
BzlaFPSymProp
BzlaFPSymBV<is_signed>::isAllOnes() const
{
  return *this == BzlaFPSymBV<is_signed>::allOnes(this->getWidth());
}

template <bool is_signed>
BzlaFPSymProp
BzlaFPSymBV<is_signed>::isAllZeros() const
{
  return *this == BzlaFPSymBV<is_signed>::zero(this->getWidth());
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::maxValue(const bwt &w)
{
  assert(s_bzla);
  BzlaSortId s               = bzla_sort_bv(s_bzla, w);
  BzlaNode *n                = bzla_exp_bv_max_signed(s_bzla, s);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  bzla_sort_release(s_bzla, s);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::minValue(const bwt &w)
{
  assert(s_bzla);
  BzlaSortId s               = bzla_sort_bv(s_bzla, w);
  BzlaNode *n                = bzla_exp_bv_min_signed(s_bzla, s);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  bzla_sort_release(s_bzla, s);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::operator<<(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n                = bzla_exp_bv_sll(s_bzla, d_node, op.d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::operator>>(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n = is_signed ? bzla_exp_bv_sra(s_bzla, d_node, op.d_node)
                          : bzla_exp_bv_srl(s_bzla, d_node, op.d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::operator|(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n                = bzla_exp_bv_or(s_bzla, d_node, op.d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::operator&(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n                = bzla_exp_bv_and(s_bzla, d_node, op.d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::operator+(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n                = bzla_exp_bv_add(s_bzla, d_node, op.d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::operator-(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n                = bzla_exp_bv_sub(s_bzla, d_node, op.d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::operator*(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n                = bzla_exp_bv_mul(s_bzla, d_node, op.d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::operator/(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n = is_signed ? bzla_exp_bv_sdiv(s_bzla, d_node, op.d_node)
                          : bzla_exp_bv_udiv(s_bzla, d_node, op.d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::operator%(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n = is_signed ? bzla_exp_bv_srem(s_bzla, d_node, op.d_node)
                          : bzla_exp_bv_urem(s_bzla, d_node, op.d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::operator-(void) const
{
  assert(s_bzla);
  BzlaNode *n                = bzla_exp_bv_neg(s_bzla, d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::operator~(void) const
{
  assert(s_bzla);
  BzlaNode *n                = bzla_exp_bv_not(s_bzla, d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::increment() const
{
  assert(s_bzla);
  BzlaNode *n                = bzla_exp_bv_inc(s_bzla, d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::decrement() const
{
  assert(s_bzla);
  BzlaNode *n                = bzla_exp_bv_dec(s_bzla, d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::signExtendRightShift(
    const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n                = bzla_exp_bv_sra(s_bzla, d_node, op.d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::modularLeftShift(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  return *this << op;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::modularRightShift(
    const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  return *this >> op;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::modularIncrement() const
{
  assert(s_bzla);
  return this->increment();
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::modularDecrement() const
{
  assert(s_bzla);
  return this->decrement();
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::modularAdd(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  return *this + op;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::modularNegate() const
{
  assert(s_bzla);
  return -(*this);
}

template <bool is_signed>
BzlaFPSymProp
BzlaFPSymBV<is_signed>::operator==(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n       = bzla_exp_eq(s_bzla, d_node, op.d_node);
  BzlaFPSymProp res = BzlaFPSymProp(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymProp
BzlaFPSymBV<is_signed>::operator<=(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n = is_signed ? bzla_exp_bv_slte(s_bzla, d_node, op.d_node)
                          : bzla_exp_bv_ulte(s_bzla, d_node, op.d_node);
  BzlaFPSymProp res = BzlaFPSymProp(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymProp
BzlaFPSymBV<is_signed>::operator>=(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n = is_signed ? bzla_exp_bv_sgte(s_bzla, d_node, op.d_node)
                          : bzla_exp_bv_ugte(s_bzla, d_node, op.d_node);
  BzlaFPSymProp res = BzlaFPSymProp(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymProp
BzlaFPSymBV<is_signed>::operator<(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n = is_signed ? bzla_exp_bv_slt(s_bzla, d_node, op.d_node)
                          : bzla_exp_bv_ult(s_bzla, d_node, op.d_node);
  BzlaFPSymProp res = BzlaFPSymProp(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymProp
BzlaFPSymBV<is_signed>::operator>(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n = is_signed ? bzla_exp_bv_sgt(s_bzla, d_node, op.d_node)
                          : bzla_exp_bv_ugt(s_bzla, d_node, op.d_node);
  BzlaFPSymProp res = BzlaFPSymProp(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<true>
BzlaFPSymBV<is_signed>::toSigned(void) const
{
  return BzlaFPSymBV<true>(*this);
}

template <bool is_signed>
BzlaFPSymBV<false>
BzlaFPSymBV<is_signed>::toUnsigned(void) const
{
  return BzlaFPSymBV<false>(*this);
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::extend(bwt extension) const
{
  assert(s_bzla);
  BzlaNode *n = is_signed ? bzla_exp_bv_sext(s_bzla, d_node, extension)
                          : bzla_exp_bv_uext(s_bzla, d_node, extension);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::contract(bwt reduction) const
{
  assert(s_bzla);
  assert(this->getWidth() > reduction);
  BzlaNode *n =
      bzla_exp_bv_slice(s_bzla, d_node, this->getWidth - 1 - reduction, 0);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::resize(bwt newSize) const
{
  bwt bw = this->getWidth();
  if (newSize > bw) return this->extend(newSize - bw);
  if (newSize < bw) return this->contract(bw - newSize);
  return *this;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::matchWidth(const BzlaFPSymBV<is_signed> &op) const
{
  assert(this->getWidth() <= op.getWidth());
  return this->extend(op.getWidth() - this->getWidth());
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::append(const BzlaFPSymBV<is_signed> &op) const
{
  assert(s_bzla);
  BzlaNode *n                = bzla_exp_bv_concat(s_bzla, d_node, op.d_node);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
BzlaFPSymBV<is_signed>
BzlaFPSymBV<is_signed>::extract(bwt upper, bwt lower) const
{
  assert(s_bzla);
  assert(upper >= lower);
  BzlaNode *n                = bzla_exp_bv_slice(s_bzla, d_node, upper, lower);
  BzlaFPSymBV<is_signed> res = BzlaFPSymBV<is_signed>(n);
  bzla_node_release(s_bzla, n);
  return res;
}

template <bool is_signed>
bool
BzlaFPSymBV<is_signed>::checkNode(const BzlaNode *node) const
{
  return bzla_sort_is_bv(node->bzla, bzla_node_get_sort_id(node));
}

// BzlaNode* BzlaFPSymBV::boolNodeToBV(BzlaNode* node) const;
// BzlaNode* BzlaFPSymBV::BVToBoolNode(BzlaNode* node) const;

/* -------------------------------------------------------------------------- */
/* Bitwuzla wrapper for rounding modes.                                      */
/* -------------------------------------------------------------------------- */

class BzlaFPSymRM
{
  friend BzlaFPWordBlaster;
#ifdef BZLA_USE_SYMFPU
  friend symfpu::ite<BzlaFPSymProp, BzlaFPSymRM>;
#endif

 public:
  BzlaFPSymRM(BzlaNode *node);
  BzlaFPSymRM(const uint32_t val);
  BzlaFPSymRM(const BzlaFPSymRM &other);

  BzlaFPSymProp valid(void) const;
  BzlaFPSymProp operator==(const BzlaFPSymRM &other) const;

 protected:
  bool checkNode(const BzlaNode *node) const;

 private:
  BzlaNode *d_node;
  static Bzla *s_bzla;
};

/* -------------------------------------------------------------------------- */

Bzla *BzlaFPSymRM::s_bzla = nullptr;

BzlaFPSymRM::BzlaFPSymRM(BzlaNode *node)
{
  assert(s_bzla);
  assert(checkNode(node));
  d_node = bzla_node_copy(s_bzla, node);
}

BzlaFPSymRM::BzlaFPSymRM(const uint32_t val)
{
  assert(val < BZLA_RM_MAX);
  d_node = bzla_exp_fp_rm(s_bzla, static_cast<BzlaRoundingMode>(val));
  assert(checkNode(d_node));
}

BzlaFPSymRM::BzlaFPSymRM(const BzlaFPSymRM &other)
{
  assert(s_bzla);
  assert(other.d_node);
  assert(checkNode(other.d_node));
  d_node = bzla_node_copy(s_bzla, other.d_node);
}

BzlaFPSymProp
BzlaFPSymRM::valid(void) const
{
  assert(d_node);
  BzlaNode *max;
  max =
      bzla_exp_bv_unsigned(s_bzla, BZLA_RM_MAX, bzla_node_get_sort_id(d_node));
  bzla_node_release(s_bzla, max);
  return bzla_exp_bv_ult(s_bzla, d_node, max);
}

BzlaFPSymProp
BzlaFPSymRM::operator==(const BzlaFPSymRM &other) const
{
  assert(d_node);
  assert(other.d_node);
  return bzla_exp_eq(s_bzla, d_node, other.d_node);
}

bool
BzlaFPSymRM::checkNode(const BzlaNode *node) const
{
  assert(s_bzla);
  assert(node);

  BzlaSortId sort = bzla_node_get_sort_id(node);
  if (!bzla_sort_is_bv(s_bzla, sort))
  {
    return false;
  }
#ifdef BZLA_USE_SYMFPU
  assert((((uint32_t) 1u) << BZLA_FP_RM_BW) >= SYMFPU_NUMBER_OF_ROUNDING_MODES);
  return bzla_sort_bv_get_width(s_bzla, sort) == BZLA_FP_RM_BW;
#else
  return false;
#endif
}

/* -------------------------------------------------------------------------- */
/* Template parameter for SymFPU templates.                                   */
/* -------------------------------------------------------------------------- */

class BzlaFPSymTraits
{
 public:
  /* The six key types that SymFPU uses. */
  typedef BzlaFPSymRM rm;
  typedef BzlaFPSortInfo fpt;
  typedef BzlaFPSymProp prop;
  typedef BzlaFPSymBV<true> sbv;
  typedef BzlaFPSymBV<false> ubv;

  /* Give concrete instances (wrapped nodes) for each rounding mode. */
  static BzlaFPSymRM RNE(void);
  static BzlaFPSymRM RNA(void);
  static BzlaFPSymRM RTP(void);
  static BzlaFPSymRM RTN(void);
  static BzlaFPSymRM RTZ(void);

  /* Properties used by Symfpu. */
  static void precondition(const bool b);
  static void postcondition(const bool b);
  static void invariant(const bool b);
  static void precondition(const prop &p);
  static void postcondition(const prop &p);
  static void invariant(const prop &p);
};

/* -------------------------------------------------------------------------- */

BzlaFPSymRM
BzlaFPSymTraits::RNE(void)
{
  return BZLA_RM_RNE;
}

BzlaFPSymRM
BzlaFPSymTraits::RNA(void)
{
  return BZLA_RM_RNA;
}

BzlaFPSymRM
BzlaFPSymTraits::RTP(void)
{
  return BZLA_RM_RTP;
}

BzlaFPSymRM
BzlaFPSymTraits::RTN(void)
{
  return BZLA_RM_RTN;
}

BzlaFPSymRM
BzlaFPSymTraits::RTZ(void)
{
  return BZLA_RM_RTZ;
}

void
BzlaFPSymTraits::precondition(const bool b)
{
  assert(b);
  (void) b;
}

void
BzlaFPSymTraits::postcondition(const bool b)
{
  assert(b);
  (void) b;
}

void
BzlaFPSymTraits::invariant(const bool b)
{
  assert(b);
  (void) b;
}

void
BzlaFPSymTraits::precondition(const prop &p)
{
  (void) p;
}

void
BzlaFPSymTraits::postcondition(const prop &p)
{
  (void) p;
}

void
BzlaFPSymTraits::invariant(const prop &p)
{
  (void) p;
}

/* ========================================================================== */
/* Word blaster.                                                              */
/* ========================================================================== */

struct BzlaSortHashFunction
{
  size_t operator()(BzlaSortId sort) const { return sort; }
};

struct BzlaNodeHashFunction
{
  size_t operator()(BzlaNode *exp) const { return bzla_node_hash_by_id(exp); }
};

class BzlaFPWordBlaster
{
 public:
  BzlaFPWordBlaster(Bzla *bzla) : d_bzla(bzla)
  {
    BzlaFPSortInfo::s_bzla     = bzla;
    BzlaFPSymRM::s_bzla        = bzla;
    BzlaFPSymProp::s_bzla      = bzla;
    BzlaFPSymBV<true>::s_bzla  = bzla;
    BzlaFPSymBV<false>::s_bzla = bzla;
  }

  void word_blast();

 private:
  using BzlaFPSortInfoMap =
      std::unordered_map<BzlaSortId, BzlaFPSortInfo, BzlaSortHashFunction>;
  using BzlaFPSymRMMap =
      std::unordered_map<BzlaNode *, BzlaFPSymRM, BzlaNodeHashFunction>;
  using BzlaFPSymPropMap =
      std::unordered_map<BzlaNode *, BzlaFPSymProp, BzlaNodeHashFunction>;
  using BzlaFPSymUBVMap =
      std::unordered_map<BzlaNode *, BzlaFPSymBV<false>, BzlaNodeHashFunction>;
  using BzlaFPSymSBVMap =
      std::unordered_map<BzlaNode *, BzlaFPSymBV<true>, BzlaNodeHashFunction>;

  BzlaFPSortInfoMap s_sort_map;
  BzlaFPSymRMMap d_rm_map;
  BzlaFPSymPropMap d_prop_map;
  BzlaFPSymUBVMap d_ubv_map;
  BzlaFPSymSBVMap d_sbv_map;
  Bzla *d_bzla;
};

void *
bzla_fp_word_blaster_new(Bzla *bzla)
{
  return new BzlaFPWordBlaster(bzla);
}

void
bzla_fp_word_blaster_delete(void *wblaster)
{
  delete static_cast<BzlaFPWordBlaster *>(wblaster);
}
