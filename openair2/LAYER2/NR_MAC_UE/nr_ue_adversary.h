/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
 * \file    nr_ue_adversary.h
 * \brief   v1 adversarial-UE profile for simulation experiments.
 *
 * Reads an optional [adversary] config section into the per-UE
 * nr_ue_adversary_t profile and provides the active-window predicate used by
 * the attack hooks in nr_ue_scheduler.c. With no [adversary] section the
 * profile stays all-zero and the UE behaves exactly like a stock UE.
 *
 * This is intentionally simple (no EA, no coordination) — see
 * doc/adversarial-ue-v1.md.
 */

#ifndef NR_UE_ADVERSARY_H
#define NR_UE_ADVERSARY_H

#include "mac_defs.h" // nr_ue_adversary_t, frame_t

/*! \brief Populate the adversary profile from the [adversary] config section.
 *  Applies defaults (all-off) when keys/section are absent, so stock configs
 *  are unaffected. Logs a warning when adversarial behaviour is enabled. */
void nr_ue_read_adversary_config(nr_ue_adversary_t *adv);

/*! \brief True when adversarial behaviour should act in the given frame, i.e.
 *  the profile is enabled and frame is inside [start_frame, stop_frame]. */
bool nr_ue_adversary_active(const nr_ue_adversary_t *adv, frame_t frame);

#endif /* NR_UE_ADVERSARY_H */
