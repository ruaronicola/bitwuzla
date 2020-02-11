/*  Boolector: Satisfiability Modulo Theories (SMT) solver.
 *
 *  Copyright (C) 2015-2020 Aina Niemetz.
 *
 *  This file is part of Boolector.
 *  See COPYING for more information on using this software.
 */

#ifndef BZLARNG_H_INCLUDED
#define BZLARNG_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#ifdef BZLA_USE_GMP
#include "utils/bzlamem.h"
#endif

struct BzlaRNG
{
  uint32_t z, w;
#ifdef BZLA_USE_GMP
  BzlaMemMgr* mm;
  bool is_init;
  /* This is a bit ugly, but a workaround to not include gmp.h in this header
   * (including the GMP header causes compilation problems with gtest). */
  void* gmp_state;
#endif
};

typedef struct BzlaRNG BzlaRNG;

/**
 * Initialize RNG with given seed. If compiled with GMP, this additionally
 * initializes the state of the GMP RNG, thus calling this function is in
 * that case mandatory.
 */
void bzla_rng_init(BzlaRNG* rng, uint32_t seed);

/**
 * Initialize given RNG clone when cloning.
 * This does nothing when not compiled with GMP but must be called when cloning
 * objects that maintain an RNG struct (and thus memcpy the state of this object
 * to the cloned object when cloning) when compiled with GMP.
 */
void bzla_rng_clone(BzlaRNG* rng, BzlaRNG* clone);

/**
 * Delete allocated data members of the given RNG.
 * This does nothing when not compiled with GMP but must be called to free
 * allocated GMP data members when compiled with GMP.
 */
void bzla_rng_delete(BzlaRNG* rng);

/* Pick a random unsigned 32 bit integer. */
uint32_t bzla_rng_rand(BzlaRNG* rng);
/* Pick a random unsigned 32 bit integer between given bounds (inclusive). */
uint32_t bzla_rng_pick_rand(BzlaRNG* rng, uint32_t from, uint32_t to);
/* Pick a random double between given bounds (inclusive). */
double bzla_rng_pick_rand_dbl(BzlaRNG* rng, double from, double to);

/* Pick true with given probability (1000 = 100%). */
bool bzla_rng_pick_with_prob(BzlaRNG* rng, uint32_t prob);
/* Pick true with probability 50%. */
bool bzla_rng_flip_coin(BzlaRNG* rng);

#endif
