#ifndef hks_hls_template_h
#define hks_hls_template_h

#include <ap_int.h>
#include <hls_stream.h>

// single channel hit samples (time-over-threshold from FADC)
// 8 samples: '1'=hit/'0'=no-hit, 4ns each
// hit[0] @ 0ns, hit[1] @ 4ns, ... hit[7] @ 28ns
typedef struct
{
  ap_uint<8> hit;
} fadc_hit_t;

// fadc hits for whole VXS crate (16 slots, 16 channels each)
// updated every 32ns
// hits[0]   ... hits[15] : fadc slot 3  ch[0] ... ch[15]
//    hits are in order of FADC slot/ch
//    fadc slots are 3 to 10, 13 to 20
//    note: slots 11 and 12 are VXS switch slots (FADCs aren't populated there)
typedef struct
{
  fadc_hit_t hits[256];
} fadc_vxs_hits_t;

#define TRIGGER_NUM   6

// trig_t are the trigger decisions.
// The number of trigger bits defined by TRIGGER_NUM
// Each trigger bit has 8 bits to specify which point in the 32ns window the trigger occured
// trg[trg_num][0] @ 0ns, ... trg[trg_num][7] @ 28ns
typedef struct
{
  ap_uint<8> trg[TRIGGER_NUM];
} trig_t;

// Top-level synthesis target function
//    s_fadc_vxs_hits: stream of FADC hits for 16 FADC slots, each with 16 channels (4ns resolution of time-over-threshold data)
//    s_trig: stream of trigger bit outputs
//    multiplicity_thr: multiplicity trigger threshold, make a trigger when meeting/exceed this hit threshold (example trigger)
void hks_hls_template(
		ap_uint<8> stretch_width[8],
    ap_uint<9> multiplicty_thr,
    hls::stream<fadc_vxs_hits_t> &s_fadc_vxs_hits,
    hls::stream<trig_t> &s_trig
  );

#endif
