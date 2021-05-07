/*
 * File: waterPadFiltering.c
 *
 * MATLAB Coder version            : 5.0
 * C/C++ source code generated on  : 02-Apr-2021 23:00:40
 *
 * \par  Copyright Notice
 *       Copyright 2021 charity: water
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *           http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 *
 */

/* Include Files */
#include "waterPadFiltering.h"

/* Function Definitions */

/*
 * Arguments    : const padSample_t *pad_sample
 *                padFilteringData_t *pad_filtering_data
 *                padSample_t *filtered_pad_sample
 * Return Type  : void
 */
void waterPadFiltering(const padSample_t *pad_sample, padFilteringData_t
  *pad_filtering_data, padSample_t *filtered_pad_sample)
{
  uint32_T buffer_5_sum;
  uint32_T buffer_4_sum;
  uint32_T buffer_3_sum;
  uint32_T buffer_2_sum;
  uint32_T buffer_1_sum;
  uint32_T buffer_0_sum;

  /*  Check inputs */
  /*  Variable to store the filtered pad data - set to the current data by */
  /*  default */
  *filtered_pad_sample = *pad_sample;

  /*  variables to stores the buffer sums */
  /*  If the buffer index is at 7 we add the new point, average and shift the */
  /*  buffer and remain at index of 7 */
  if (pad_filtering_data->buffer_idx == 3) {
    /*  Add the newest point to the buffers */
    pad_filtering_data->pad_5_buffer[3] = pad_sample->pad5;
    pad_filtering_data->pad_4_buffer[3] = pad_sample->pad4;
    pad_filtering_data->pad_3_buffer[3] = pad_sample->pad3;
    pad_filtering_data->pad_2_buffer[3] = pad_sample->pad2;
    pad_filtering_data->pad_1_buffer[3] = pad_sample->pad1;
    pad_filtering_data->pad_0_buffer[3] = pad_sample->pad0;

    /*  Sum the data before bitshifting and shift the buffer */
    buffer_5_sum = (uint32_T)pad_filtering_data->pad_5_buffer[0] +
      pad_filtering_data->pad_5_buffer[1];
    buffer_4_sum = (uint32_T)pad_filtering_data->pad_4_buffer[0] +
      pad_filtering_data->pad_4_buffer[1];
    buffer_3_sum = (uint32_T)pad_filtering_data->pad_3_buffer[0] +
      pad_filtering_data->pad_3_buffer[1];
    buffer_2_sum = (uint32_T)pad_filtering_data->pad_2_buffer[0] +
      pad_filtering_data->pad_2_buffer[1];
    buffer_1_sum = (uint32_T)pad_filtering_data->pad_1_buffer[0] +
      pad_filtering_data->pad_1_buffer[1];
    buffer_0_sum = (uint32_T)pad_filtering_data->pad_0_buffer[0] +
      pad_filtering_data->pad_0_buffer[1];
    pad_filtering_data->pad_5_buffer[0] = pad_filtering_data->pad_5_buffer[1];
    pad_filtering_data->pad_4_buffer[0] = pad_filtering_data->pad_4_buffer[1];
    pad_filtering_data->pad_3_buffer[0] = pad_filtering_data->pad_3_buffer[1];
    pad_filtering_data->pad_2_buffer[0] = pad_filtering_data->pad_2_buffer[1];
    pad_filtering_data->pad_1_buffer[0] = pad_filtering_data->pad_1_buffer[1];
    pad_filtering_data->pad_0_buffer[0] = pad_filtering_data->pad_0_buffer[1];
    buffer_5_sum += pad_filtering_data->pad_5_buffer[2];
    buffer_4_sum += pad_filtering_data->pad_4_buffer[2];
    buffer_3_sum += pad_filtering_data->pad_3_buffer[2];
    buffer_2_sum += pad_filtering_data->pad_2_buffer[2];
    buffer_1_sum += pad_filtering_data->pad_1_buffer[2];
    buffer_0_sum += pad_filtering_data->pad_0_buffer[2];
    pad_filtering_data->pad_5_buffer[1] = pad_filtering_data->pad_5_buffer[2];
    pad_filtering_data->pad_4_buffer[1] = pad_filtering_data->pad_4_buffer[2];
    pad_filtering_data->pad_3_buffer[1] = pad_filtering_data->pad_3_buffer[2];
    pad_filtering_data->pad_2_buffer[1] = pad_filtering_data->pad_2_buffer[2];
    pad_filtering_data->pad_1_buffer[1] = pad_filtering_data->pad_1_buffer[2];
    pad_filtering_data->pad_0_buffer[1] = pad_filtering_data->pad_0_buffer[2];
    buffer_5_sum += pad_filtering_data->pad_5_buffer[3];
    buffer_4_sum += pad_filtering_data->pad_4_buffer[3];
    buffer_3_sum += pad_filtering_data->pad_3_buffer[3];
    buffer_2_sum += pad_filtering_data->pad_2_buffer[3];
    buffer_1_sum += pad_filtering_data->pad_1_buffer[3];
    buffer_0_sum += pad_filtering_data->pad_0_buffer[3];
    pad_filtering_data->pad_5_buffer[2] = pad_filtering_data->pad_5_buffer[3];
    pad_filtering_data->pad_4_buffer[2] = pad_filtering_data->pad_4_buffer[3];
    pad_filtering_data->pad_3_buffer[2] = pad_filtering_data->pad_3_buffer[3];
    pad_filtering_data->pad_2_buffer[2] = pad_filtering_data->pad_2_buffer[3];
    pad_filtering_data->pad_1_buffer[2] = pad_filtering_data->pad_1_buffer[3];
    pad_filtering_data->pad_0_buffer[2] = pad_filtering_data->pad_0_buffer[3];

    /*  Use bitshifting to get the average datapoint */
    filtered_pad_sample->pad5 = (uint16_T)(buffer_5_sum >> 2);

    /*  Inexpensive divide operation        */
    filtered_pad_sample->pad4 = (uint16_T)(buffer_4_sum >> 2);

    /*  Inexpensive divide operation        */
    filtered_pad_sample->pad3 = (uint16_T)(buffer_3_sum >> 2);

    /*  Inexpensive divide operation        */
    filtered_pad_sample->pad2 = (uint16_T)(buffer_2_sum >> 2);

    /*  Inexpensive divide operation        */
    filtered_pad_sample->pad1 = (uint16_T)(buffer_1_sum >> 2);

    /*  Inexpensive divide operation        */
    filtered_pad_sample->pad0 = (uint16_T)(buffer_0_sum >> 2);

    /*  Inexpensive divide operation        */
  } else {
    /*  Increment the buffer variable first */
    pad_filtering_data->buffer_idx++;

    /*  Add the newest point to the buffers */
    pad_filtering_data->pad_5_buffer[pad_filtering_data->buffer_idx - 1] =
      pad_sample->pad5;
    pad_filtering_data->pad_4_buffer[pad_filtering_data->buffer_idx - 1] =
      pad_sample->pad4;
    pad_filtering_data->pad_3_buffer[pad_filtering_data->buffer_idx - 1] =
      pad_sample->pad3;
    pad_filtering_data->pad_2_buffer[pad_filtering_data->buffer_idx - 1] =
      pad_sample->pad2;
    pad_filtering_data->pad_1_buffer[pad_filtering_data->buffer_idx - 1] =
      pad_sample->pad1;
    pad_filtering_data->pad_0_buffer[pad_filtering_data->buffer_idx - 1] =
      pad_sample->pad0;
  }
}

/*
 * File trailer for waterPadFiltering.c
 *
 * [EOF]
 */
