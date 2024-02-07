#include <eigenapi.h>
#include <lib_alpha2/alpha2_usb.h>
#include <picross/pic_config.h>
#include <picross/pic_log.h>
#include <picross/pic_resources.h>
#include <picross/pic_time.h>
#include <picross/pic_usb.h>

#include "ef_harp.h"
#include "eigenlite_impl.h"

// INFO ONLY from alpha2_active.h
// #define TAU_KBD_KEYS 84 // normal keys(84)
// #define TAU_KEYS_OFFSET 5
// #define TAU_MODE_KEYS   8
// #define TAU_KBD_DESENSE (TAU_KBD_KEYS+0) // 84
// #define TAU_KBD_BREATH1 (TAU_KBD_KEYS+1) // 85
// #define TAU_KBD_BREATH2 (TAU_KBD_KEYS+2) // 86
// #define TAU_KBD_STRIP1  (TAU_KBD_KEYS+3) // 87
// #define TAU_KBD_ACCEL   (TAU_KBD_KEYS+4) // 89


// main keys = 0  to 71
// perc keys = 72 to 83
// mode keys = 89 to 96
static constexpr unsigned TAU_PERC_KEYS_START = 72;
static constexpr unsigned TAU_PERC_KEYS_END = (TAU_PERC_KEYS_START + 12) - 1;
static constexpr unsigned TAU_MODE_KEYS_START = 89; // TAU_KBD_KEYS + TAU_KEYS_OFFSET
static constexpr unsigned TAU_MODE_KEYS_END = (TAU_MODE_KEYS_START + TAU_MODE_KEYS) - 1;


namespace EigenApi {

// static const unsigned TAU_COLUMNCOUNT = 7;
// static const unsigned TAU_COLUMNS[TAU_COLUMNCOUNT] = {16, 16, 20, 20, 12, 4, 4};
// static const unsigned TAU_COURSEKEYS = 92;

void EF_Tau::fireTauKeyEvent(unsigned long long t, unsigned k, bool a, float p, float r, float y) {
    unsigned course = k < TAU_PERC_KEYS_START ? 0 : (k <= TAU_PERC_KEYS_END ? 1 : 2);
    int key = 0;
    switch (course) {
        case 0: {
            key = k;
            break;
        }
        case 1: {
            key = k - TAU_PERC_KEYS_START;
            break;
        }
        case 2: {
            key = k - TAU_MODE_KEYS_START;
            break;
        }
    }
    // pic::logmsg() << "fireTauKeyEvent  - course " << course << " key " << key;
    parent_.fireKeyEvent(t, course, key, a, p, r, y);
}

void EF_Tau::kbd_key(unsigned long long t, unsigned key, unsigned p, int r, int y) {
    unsigned w = alpha2::active_t::key2word(key);
    unsigned short m = alpha2::active_t::key2mask(key);
    bool a = false;

    if (!(parent_.skpMap()[w] & m)) {
        parent_.curMap()[w] |= m;
        a = true;
    }

    if (key == TAU_KBD_DESENSE) return;
    if (key == TAU_KBD_ACCEL) return;  

    // pic::logmsg() << "kbd_key " << key << " p " << p << " r " << r << " y " << y;

    if (key < TAU_KBD_KEYS || key >= (TAU_KBD_KEYS + TAU_KEYS_OFFSET)) {
        // pic::logmsg() << "kbd_key fire " << key << " p " << p << " r " << r << " y " << y;
        float fp = pToFloat(p);
        float fr = rToFloat(r);
        float fy = yToFloat(y);

        if (key >= (TAU_KBD_KEYS + TAU_KEYS_OFFSET)) {
            // mode key (p=1, not 4095)
            fp = float(p);
        }

        fireTauKeyEvent(t, key, a, fp, fr, fy);
        return;
    }

    // TAU_KEYS_OFFSET = 5
    // TAU_MODE_KEYS = 8

    switch (key) {
        case TAU_KBD_BREATH1: {
            // pic::logmsg() << "TAU_KBD_BREATH1 " << p;
            float fp = breathToFloat(p);
            parent_.fireBreathEvent(t, fp);
            break;
        }
        case TAU_KBD_STRIP1: {
            float fp = stripToFloat(p);
            parent_.fireStripEvent(t, 1, fp, a);
            break;
        }
        // case TAU_KBD_DESENSE : break;
        // case TAU_KBD_BREATH2 : break;
        // case TAU_KBD_ACCEL   : break;
        default:;
    }
}

void EF_Tau::kbd_keydown(unsigned long long t, const unsigned short *newmap) {
    // char buf[121];
    // for(int i=0;i < 120;i++) {
    //     buf[i] = alpha2::active_t::keydown(i,bitmap) ? '1' :  '0';
    // }
    // buf[120]=0;
    // pic::logmsg() << buf;
    for (unsigned w = 0; w < 9; w++) {
        parent_.skpMap()[w] &= newmap[w];

        if (parent_.curMap()[w] == newmap[w]) continue;

        unsigned keybase = alpha2::active_t::word2key(w);

        for (unsigned k = 0; k < 16; k++) {
            unsigned short mask = alpha2::active_t::key2mask(k);

            // if was on, but no longer on
            if ((parent_.curMap()[w] & mask) && !(newmap[w] & mask)) {
                unsigned key = keybase + k;
                if (key == TAU_KBD_DESENSE) continue;
                if (key == TAU_KBD_ACCEL) continue;  

                if (key == TAU_KBD_STRIP1) {
                    parent_.fireStripEvent(t, 1, 0.0f, 0);
                } else {
                    fireTauKeyEvent(t, keybase + k, 0, 0.f, 0.f, 0.f);
                }
            }
        }

        parent_.curMap()[w] = newmap[w];
    }
}

}  // namespace EigenApi
