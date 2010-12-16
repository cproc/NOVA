/*
 * Page Table Entry (PTE)
 *
 * Copyright (C) 2009-2010 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#include "dpt.h"
#include "ept.h"
#include "hpt.h"
#include "pte.h"

mword Dpt::ord = ~0UL;
mword Ept::ord = ~0UL;
mword Hpt::ord = ~0UL;

template <typename P, typename E, unsigned L, unsigned B, bool F>
P *Pte<P,E,L,B,F>::walk (E v, unsigned long n, bool d)
{
    unsigned long l = L;

    for (P *p, *e = static_cast<P *>(this);; e = static_cast<P *>(Buddy::phys_to_ptr (e->addr())) + (v >> (--l * B + PAGE_BITS) & ((1UL << B) - 1))) {

        if (l == n)
            return e;

        if (!e->present()) {

            if (d)
                return 0;

            if (!e->set (0, Buddy::ptr_to_phys (p = new P) | (l == L ? P::PTE_P : P::PTE_N)))
                delete p;
        }
    }
}

template <typename P, typename E, unsigned L, unsigned B, bool F>
size_t Pte<P,E,L,B,F>::lookup (E v, Paddr &p)
{
    unsigned long l = L;

    for (P *e = static_cast<P *>(this);; e = static_cast<P *>(Buddy::phys_to_ptr (e->addr())) + (v >> (--l * B + PAGE_BITS) & ((1UL << B) - 1))) {

        if (EXPECT_FALSE (!e->present()))
            return 0;

        if (EXPECT_FALSE (l && !e->super()))
            continue;

        size_t s = 1UL << (l * B + e->order());

        p = static_cast<Paddr>(e->addr() | (v & (s - 1)));

        return s;
    }
}

template <typename P, typename E, unsigned L, unsigned B, bool F>
bool Pte<P,E,L,B,F>::update (E v, mword o, E p, mword a, bool d)
{
    unsigned long l = o / B, n = 1UL << o % B, s = 1UL << (l * B + PAGE_BITS);

    P *e = walk (v, l, d);

    p |= P::order (o % B) | (l ? P::PTE_S : 0) | a;

    for (unsigned long i = 0; i < n; i++, p += s)
        e[i].val = p;

    if (F)
        flush (e, n * sizeof (E));

    return l;
}

template class Pte<Dpt, uint64, 4, 9, true>;
template class Pte<Ept, uint64, 4, 9, false>;
template class Pte<Hpt, mword, 2, 10, false>;
