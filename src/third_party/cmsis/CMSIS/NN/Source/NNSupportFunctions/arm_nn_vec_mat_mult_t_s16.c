/*
 * Copyright (C) 2020-2021 Arm Limited or its affiliates. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ----------------------------------------------------------------------
 * Project:      CMSIS NN Library
 * Title:        arm_nn_vec_mat_mult_t_s16
 * Description:  s16 vector by matrix (transposed) multiplication
 *
 * $Date:        23. August 2021
 * $Revision:    V.1.1.0
 *
 * Target Processor:  Cortex-M
 *
 * -------------------------------------------------------------------- */

#include "third_party/cmsis/CMSIS/NN/Include/arm_nnsupportfunctions.h"
/**
 * @ingroup groupSupport
 */

/**
 * @addtogroup NNBasicMath
 * @{
 */

/*
 * s16 vector(lhs) by matrix (transposed) multiplication
 *
 * Refer header file for details.
 *
 */
arm_status arm_nn_vec_mat_mult_t_s16(const q15_t *lhs,
                                     const q7_t *rhs,
                                     const q63_t *bias,
                                     q15_t *dst,
                                     const int32_t dst_multiplier,
                                     const int32_t dst_shift,
                                     const int32_t rhs_cols,
                                     const int32_t rhs_rows,
                                     const int32_t activation_min,
                                     const int32_t activation_max)
{
    const int32_t row_loop_cnt = rhs_rows / 2;

#if defined(ARM_MATH_DSP) && !defined(ARM_MATH_MVEI)

    for (int32_t i = 0; i < row_loop_cnt; i++)
    {
        q63_t acc_64_0 = 0;
        q63_t acc_64_1 = 0;
        int32_t acc_0 = 0;
        int32_t acc_1 = 0;

        const int32_t col_loop_cnt = rhs_cols / 4;

        const int16_t *lhs_vec = lhs;
        const int8_t *rhs_0 = rhs;
        const int8_t *rhs_1 = rhs + rhs_cols;
        rhs += 2 * rhs_cols;

        for (int j = col_loop_cnt; j != 0; j--)
        {
            int32_t ker_0, ker_1, vec_part_0, vec_part_1;
            vec_part_0 = arm_nn_read_q15x2_ia(&lhs_vec);
            vec_part_1 = arm_nn_read_q15x2_ia(&lhs_vec);

            rhs_0 = read_and_pad(rhs_0, &ker_0, &ker_1);

            acc_0 = __SMLAD(ker_0, vec_part_0, acc_0);
            acc_0 = __SMLAD(ker_1, vec_part_1, acc_0);

            rhs_1 = read_and_pad(rhs_1, &ker_0, &ker_1);

            acc_1 = __SMLAD(ker_0, vec_part_0, acc_1);
            acc_1 = __SMLAD(ker_1, vec_part_1, acc_1);
        }

        for (int k = col_loop_cnt * 4; k < rhs_cols; k++)
        {
            const int32_t lhs_temp = (*lhs_vec);
            lhs_vec++;
            acc_0 += lhs_temp * (*rhs_0);
            rhs_0++;
            acc_1 += lhs_temp * (*rhs_1);
            rhs_1++;
        }

        acc_64_0 += acc_0;
        acc_64_1 += acc_1;
        if (bias)
        {
            acc_64_0 += *bias++;
        }
        q31_t tmp;
        tmp = arm_nn_requantize_s64(acc_64_0, dst_multiplier, dst_shift);
        tmp = MAX(tmp, activation_min);
        tmp = MIN(tmp, activation_max);
        *dst++ = (q15_t)tmp;

        if (bias)
        {
            acc_64_1 += *bias++;
        }
        tmp = arm_nn_requantize_s64(acc_64_1, dst_multiplier, dst_shift);
        tmp = MAX(tmp, activation_min);
        tmp = MIN(tmp, activation_max);
        *dst++ = (q15_t)tmp;
    }

    if (rhs_rows & 0x1)
    {
        q63_t acc_64_0 = 0;
        int32_t acc_0 = 0;
        const int32_t col_loop_cnt = rhs_cols / 4;

        const int16_t *lhs_vec = lhs;
        const int8_t *rhs_0 = rhs;

        for (int i = col_loop_cnt; i != 0; i--)
        {
            int32_t ker_0, ker_1, vec;
            rhs_0 = read_and_pad(rhs_0, &ker_0, &ker_1);

            vec = arm_nn_read_q15x2_ia(&lhs_vec);
            acc_0 = __SMLAD(ker_0, vec, acc_0);

            vec = arm_nn_read_q15x2_ia(&lhs_vec);
            acc_0 = __SMLAD(ker_1, vec, acc_0);
        }

        for (int j = col_loop_cnt * 4; j < rhs_cols; j++)
        {
            const int32_t lhs_temp = (*lhs_vec);
            lhs_vec++;
            acc_0 += lhs_temp * (*rhs_0);
            rhs_0++;
        }
        acc_64_0 += acc_0;
        if (bias)
        {
            acc_64_0 += *bias++;
        }
        q31_t tmp;
        tmp = arm_nn_requantize_s64(acc_64_0, dst_multiplier, dst_shift);
        tmp = MAX(tmp, activation_min);
        tmp = MIN(tmp, activation_max);
        *dst++ = (q15_t)tmp;
    }

#else

    for (int i_row_loop_cnt = 0; i_row_loop_cnt < row_loop_cnt; i_row_loop_cnt++)
    {
        const q15_t *lhs_ptr = lhs;
        const q7_t *rhs_ptr_0 = &rhs[0];
        const q7_t *rhs_ptr_1 = &rhs[rhs_cols];

        q63_t res00 = 0;
        q63_t res01 = 0;

        if (bias)
        {
            res00 = *bias++;
            res01 = *bias++;
        }
        for (int32_t rhs_cols_idx = 0; rhs_cols_idx < rhs_cols; ++rhs_cols_idx)
        {
            const q63_t rhs_value0 = (int8_t)*rhs_ptr_0;
            const q63_t rhs_value1 = (int8_t)*rhs_ptr_1;
            const q63_t lhs_value = *lhs_ptr;

            res00 += lhs_value * rhs_value0;
            res01 += lhs_value * rhs_value1;

            ++rhs_ptr_0;
            ++rhs_ptr_1;
            ++lhs_ptr;
        }

        // Quantize down
        res00 = arm_nn_requantize_s64(res00, dst_multiplier, dst_shift);
        res01 = arm_nn_requantize_s64(res01, dst_multiplier, dst_shift);

        // Clamp the result
        res00 = MAX(res00, activation_min);
        res00 = MIN(res00, activation_max);
        res01 = MAX(res01, activation_min);
        res01 = MIN(res01, activation_max);

        *dst++ = (q15_t)res00;
        *dst++ = (q15_t)res01;

        rhs += 2 * rhs_cols;
    }

    const int loop_cnt = rhs_rows % 2;

    for (int i_loop_cnt = 0; i_loop_cnt < loop_cnt; i_loop_cnt++)
    {
        const q15_t *lhs_ptr = &lhs[0];
        const q7_t *rhs_ptr = &rhs[0];

        q63_t res00 = 0;
        if (bias)
        {
            res00 = *bias++;
        }

        for (int32_t rhs_cols_idx = 0; rhs_cols_idx < rhs_cols; ++rhs_cols_idx)
        {
            q31_t rhs_value0 = (int8_t)rhs_ptr[0];
            q31_t lhs_value = (int16_t)lhs_ptr[0];

            res00 += lhs_value * rhs_value0;

            ++rhs_ptr;
            ++lhs_ptr;
        }

        // Quantize down
        res00 = arm_nn_requantize_s64(res00, dst_multiplier, dst_shift);

        // Clamp the result
        res00 = MAX(res00, activation_min);
        res00 = MIN(res00, activation_max);

        *dst++ = (q15_t)res00;
        rhs += rhs_cols;
    }
#endif

    return ARM_MATH_SUCCESS;
}

/**
 * @} end of NNBasicMath group
 */
