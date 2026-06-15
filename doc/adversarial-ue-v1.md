<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Adversarial UE — v1 (simulation)

A minimal, opt-in modification of the OAI NR UE (`nr-uesoftmodem`) that makes it
misbehave on the uplink control plane, for **authorised security research on a
closed RFsim/lab network you control**. This is the v1 baseline: the three most
basic attacks, hand-toggled, **no evolutionary algorithm and no multi-UE
coordination yet**.

> ⚠️ **Scope / safety.** Only run this against your own gNB on an isolated
> network (RFsim, or a conducted/shielded RF lab). Never transmit adversarial
> behaviour toward a network you do not own.

## What it does

Three independently toggleable attacks, all behind a single config section.
With the section absent or `enabled = 0`, the UE is **byte-for-byte a normal
UE** — nothing in the scheduling path changes.

| Attack | Config key | Effect | Where (code) |
|--------|-----------|--------|--------------|
| **BSR inflation** | `bsr_inflate` | UE reports a falsely large UL buffer → gNB over-allocates UL grants | `nr_update_bsr()` in [nr_ue_scheduler.c](../openair2/LAYER2/NR_MAC_UE/nr_ue_scheduler.c) |
| **SR flood** | `sr_flood` | UE keeps a Scheduling Request permanently pending → gNB keeps issuing grants | `nr_update_sr()` |
| **RACH flood** | `rach_flood` | UE periodically forces a fresh Random Access procedure → loads the gNB RA machinery | `nr_ue_ul_scheduler()` |

The gNB does **not** validate the reported BSR against data actually received
(it even rounds it *up*: `overestim_bsr_index()` in
[gNB_scheduler_ulsch.c](../openair2/LAYER2/NR_MAC_gNB/gNB_scheduler_ulsch.c)), so
BSR inflation lands directly.

## Config

Add an `[adversary]` section to the **UE** config file (e.g. `ue-rfsim.conf`).
Every key is optional and defaults to off/safe:

```libconfig
adversary = {
  enabled            = 1;       # master switch (0 = stock UE)

  # --- BSR inflation ---
  bsr_inflate        = 1;       # claim a large UL buffer
  bsr_min_bytes      = 150000;  # bytes claimed per active LCG (near top of BSR table)

  # --- SR flood ---
  sr_flood           = 0;       # keep a scheduling request permanently pending

  # --- RACH flood ---
  rach_flood         = 0;       # periodically force a new RA procedure
  rach_period_frames = 20;      # RA re-trigger period, in frames

  # --- attack window (SFN); lets you capture clean before/during/after ---
  start_frame        = 0;       # attack inactive while frame < start_frame
  stop_frame         = 0;       # <=0 means no upper bound
};
```

Equivalent command-line form (any key can be overridden on the CLI):

```bash
--adversary.enabled 1 --adversary.bsr_inflate 1 --adversary.bsr_min_bytes 150000
```

When enabled, the UE logs a one-line banner at startup:

```
[ADVERSARY] ENABLED (simulation): bsr_inflate=1 (min 150000 B) sr_flood=0 rach_flood=0 ...
```

### Key reference

| Key | Default | Meaning |
|-----|---------|---------|
| `enabled` | 0 | Master switch. 0 ⇒ stock UE. |
| `bsr_inflate` | 0 | Inflate reported UL buffer occupancy. |
| `bsr_min_bytes` | 150000 | Buffer (bytes) claimed per active LCG when inflating. |
| `sr_flood` | 0 | Keep a scheduling request permanently pending. |
| `rach_flood` | 0 | Periodically force a new RA procedure. |
| `rach_period_frames` | 20 | RA re-trigger period in frames (when `rach_flood`). |
| `start_frame` | 0 | Attack inactive while SFN < `start_frame`. |
| `stop_frame` | 0 | Attack inactive while SFN > `stop_frame`; `<=0` ⇒ no upper bound. |

> **Note on BSR vs. SR.** A connected UE with no application traffic still has
> to *request* a grant before it can send the inflated BSR. The BSR-inflation
> path force-triggers a regular BSR (which drives an SR), so it works on an idle
> connected UE — but the cleanest, most reliable demonstration enables
> `bsr_inflate` together with `sr_flood`, or runs the UE with light background
> traffic (`ping`/`iperf`).

## Build

Same build as stock OAI — the new source (`nr_ue_adversary.c`) is already wired
into `CMakeLists.txt`. From your build tree:

```bash
cd ~/workspaces/openairinterface5g/cmake_targets
./build_oai --nrUE          # or your usual build invocation
# or, if already configured:
cd ran_build/build && make -j nr-uesoftmodem
```

## Run (RFsim)

Using your existing scripts. Start the three components in order:

```bash
./start_nrtric.sh     # near-RT RIC (FlexRIC)
./start_gnb.sh        # gNB + E2 agent
./start_ue.sh         # UE — with the [adversary] section enabled in ue-rfsim.conf
```

Then run your monitoring xApp. The attack is visible per-RNTI in the MAC SM
output:

- **`bsr=`** for the attacker RNTI jumps to a large value (the lie).
- **`ul_tbs`** / **`RRU.PrbTotUl`** (KPM) rise — grants issued for the lie.
- With a second, legitimate UE present, its **`DRB.UEThpUl`** drops as the
  attacker steals contended PRBs — this is the v1 "money-shot".

### Smoke test (single UE, ~2 minutes)

1. Set `enabled = 1; bsr_inflate = 1;` in `ue-rfsim.conf`.
2. Start RIC → gNB → UE, run the xApp.
3. Confirm the attacker's `bsr` is pinned high and `ul_tbs`/PRB allocation
   follows, versus a baseline run with `enabled = 0`.

### Attacker-vs-victim (the real measurement)

Run **two** UEs against the one gNB: one stock (victim, generating steady
traffic) and one adversarial. See
[NR_SA_Tutorial_OAI_multi_UE.md](./NR_SA_Tutorial_OAI_multi_UE.md) for the
multi-UE RFsim setup (distinct IMSIs/configs). Compare the victim's
`DRB.UEThpUl` / `RlcSduDelayDl` with the attacker on vs. off, and use
`start_frame`/`stop_frame` to capture clean before/during/after windows in one
run.

## What v1 deliberately leaves out

- **No evolutionary algorithm** — attacks are hand-toggled, fixed parameters.
- **No multi-UE coordination** — each UE acts independently.
- **No malformed-CE / fuzzing** path.

These are the planned follow-ups (see the EA and multi-UE planning notes). v1
exists to prove the mechanism and the measurement pipeline first.
