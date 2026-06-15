/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
 * \file    nr_ue_adversary.c
 * \brief   Config plumbing + active-window helper for the v1 adversarial UE.
 *          See nr_ue_adversary.h and doc/adversarial-ue-v1.md.
 */

#include "nr_ue_adversary.h"

#include "common/config/config_userapi.h"
#include "common/config/config_paramdesc.h"
#include "common/utils/utils.h" // sizeofArray
#include "common/utils/LOG/log.h"

/* config prefix: keys live under the [adversary] section / --adversary.* CLI */
#define ADVERSARY_CONFIG_SECTION "adversary"

#define ADV_ENABLED      "enabled"
#define ADV_BSR_INFLATE  "bsr_inflate"
#define ADV_BSR_MIN      "bsr_min_bytes"
#define ADV_SR_FLOOD     "sr_flood"
#define ADV_RACH_FLOOD   "rach_flood"
#define ADV_RACH_PERIOD  "rach_period_frames"
#define ADV_START_FRAME  "start_frame"
#define ADV_STOP_FRAME   "stop_frame"

/* default claimed buffer: ~150 kB sits near the top of the BSR tables, which
 * is what makes the gNB over-allocate UL grants for a UE with no real data. */
#define ADV_DEFAULT_BSR_MIN_BYTES 150000
#define ADV_DEFAULT_RACH_PERIOD   20 /* frames */

void nr_ue_read_adversary_config(nr_ue_adversary_t *adv)
{
  paramdef_t params[] = {
    BOOLPARAM(ADV_ENABLED, "master enable for adversarial UE behaviour\n", PARAMFLAG_BOOL, &adv->enabled, 0),
    BOOLPARAM(ADV_BSR_INFLATE, "report a falsely large UL buffer (BSR inflation)\n", PARAMFLAG_BOOL, &adv->bsr_inflate, 0),
    INTPARAM(ADV_BSR_MIN, "buffer size in bytes claimed per active LCG when inflating\n", 0, &adv->bsr_min_bytes, ADV_DEFAULT_BSR_MIN_BYTES),
    BOOLPARAM(ADV_SR_FLOOD, "keep a scheduling request permanently pending (SR flood)\n", PARAMFLAG_BOOL, &adv->sr_flood, 0),
    BOOLPARAM(ADV_RACH_FLOOD, "periodically force a new Random Access procedure (RACH flood)\n", PARAMFLAG_BOOL, &adv->rach_flood, 0),
    INTPARAM(ADV_RACH_PERIOD, "RA re-trigger period in frames when rach_flood is set\n", 0, &adv->rach_period_frames, ADV_DEFAULT_RACH_PERIOD),
    INTPARAM(ADV_START_FRAME, "attack inactive while frame < start_frame\n", 0, &adv->start_frame, 0),
    INTPARAM(ADV_STOP_FRAME, "attack inactive while frame > stop_frame; <=0 means no upper bound\n", 0, &adv->stop_frame, 0),
  };

  config_get(config_get_if(), params, sizeofArray(params), ADVERSARY_CONFIG_SECTION);

  if (adv->enabled) {
    if (adv->rach_period_frames <= 0)
      adv->rach_period_frames = ADV_DEFAULT_RACH_PERIOD;
    LOG_W(NR_MAC,
          "[ADVERSARY] ENABLED (simulation): bsr_inflate=%d (min %d B) sr_flood=%d rach_flood=%d (period %d frames) window=[%d,%d]\n",
          adv->bsr_inflate,
          adv->bsr_min_bytes,
          adv->sr_flood,
          adv->rach_flood,
          adv->rach_period_frames,
          adv->start_frame,
          adv->stop_frame);
  }
}

bool nr_ue_adversary_active(const nr_ue_adversary_t *adv, frame_t frame)
{
  if (!adv->enabled)
    return false;
  if ((int)frame < adv->start_frame)
    return false;
  if (adv->stop_frame > 0 && (int)frame > adv->stop_frame)
    return false;
  return true;
}
