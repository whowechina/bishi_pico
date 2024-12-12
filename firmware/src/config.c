/*
 * Controller Config and Runtime Data
 * WHowe <github.com/whowechina>
 * 
 * Config is a global data structure that stores all the configuration
 * Runtime is something to share between files.
 */

#include "config.h"
#include "save.h"

bishi_cfg_t *bishi_cfg;

static bishi_cfg_t default_cfg = {
    .spin = {
        .units_per_turn = 80,
        .reversed = false,
    },
    .light = {
        .level = 128,
    },
    .sn = 0,
};

bishi_runtime_t bishi_runtime;

static void config_loaded()
{
    if (bishi_cfg->spin.units_per_turn < 20) {
        bishi_cfg->spin.units_per_turn = 80;
        config_changed();
    }
    if (bishi_cfg->sn == 0) {
        bishi_cfg->sn = board_id_32();
        config_changed();
    }
}

void config_changed()
{
    save_request(false);
}

void config_factory_reset()
{
    *bishi_cfg = default_cfg;
    save_request(true);
}

void config_init()
{
    bishi_cfg = (bishi_cfg_t *)save_alloc(sizeof(*bishi_cfg), &default_cfg, config_loaded);
}
