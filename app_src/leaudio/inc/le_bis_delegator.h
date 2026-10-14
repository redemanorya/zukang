/*
 * le_bis_delegator.h
 *
 *  Created on: Apr 15, 2024
 *      Author: George
 */

#ifndef APP_SRC_APP_MODE_BT_LE_BIS_DELEGATOR_H_
#define APP_SRC_APP_MODE_BT_LE_BIS_DELEGATOR_H_

void le_bis_delegator_init(void);
void le_bis_deleg_configuration_cb_gapm_cmp(uint32_t event, uint16_t status);

#endif /* APP_SRC_APP_MODE_BT_LE_BIS_DELEGATOR_H_ */
