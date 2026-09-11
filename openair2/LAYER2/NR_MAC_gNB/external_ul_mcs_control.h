/*
 * External sensing-assisted UL MCS control.
 */

#ifndef EXTERNAL_UL_MCS_CONTROL_H
#define EXTERNAL_UL_MCS_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

#include "common/platform_types.h"

int start_external_ul_mcs_control(void);
bool external_ul_mcs_control_is_active(void);
uint8_t get_external_ul_max_mcs(frame_t frame, sub_frame_t slot);

#endif /* EXTERNAL_UL_MCS_CONTROL_H */
