/* ----------------------------------------------------------------------
 * Project:      CMSIS DSP Library
 * Title:        arm_cfft_radix4_f16.c
 * Description:  Radix-4 Decimation in Frequency CFFT & CIFFT Floating point processing function
 *
 * $Date:        23 April 2021
 * $Revision:    V1.9.0
 *
 * Target Processor: Cortex-M and Cortex-A cores
 * -------------------------------------------------------------------- */
/*
 * Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
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

#include "dsp/transform_functions_f16.h"

#if defined(ARM_FLOAT16_SUPPORTED)

extern void arm_bitreversal_f16(
        float16_t * pSrc,
        uint16_t fftSize,
        uint16_t bitRevFactor,
  const uint16_t * pBitRevTab);

ARM_DSP_ATTRIBUTE void arm_radix4_butterfly_f16(
        float16_t * pSrc,
        uint16_t fftLen,
  const float16_t * pCoef,
        uint16_t twidCoefModifier);

ARM_DSP_ATTRIBUTE void arm_radix4_butterfly_inverse_f16(
        float16_t * pSrc,
        uint16_t fftLen,
  const float16_t * pCoef,
        uint16_t twidCoefModifier,
        float16_t onebyfftLen);


ARM_DSP_ATTRIBUTE void arm_cfft_radix4by2_f16(
    float16_t * pSrc,
    uint32_t fftLen,
    const float16_t * pCoef);


/**
  @addtogroup ComplexFFTDeprecated
  @{
 */

/*
* @brief  Core function for the floating-point CFFT butterfly process.
* @param[in, out] *pSrc            points to the in-place buffer of floating-point data type.
* @param[in]      fftLen           length of the FFT.
* @param[in]      *pCoef           points to the twiddle coefficient buffer.
* @param[in]      twidCoefModifier twiddle coefficient modifier that supports different size FFTs with the same twiddle factor table.
*/

ARM_DSP_ATTRIBUTE void arm_cfft_radix4by2_f16(
    float16_t * pSrc,
    uint32_t fftLen,
    const float16_t * pCoef)
{
    uint32_t i, l;
    uint32_t n2, ia;
    float16_t xt, yt, cosVal, sinVal;
    float16_t p0, p1,p2,p3,a0,a1;

    n2 = fftLen >> 1;
    ia = 0;
    for (i = 0; i < n2; i++)
    {
        cosVal = pCoef[2*ia];
        sinVal = pCoef[2*ia + 1];
        ia++;

        l = i + n2;
       
        /*  Butterfly implementation */
        a0 = (_Float16)pSrc[2 * i] + (_Float16)pSrc[2 * l];
        xt = (_Float16)pSrc[2 * i] - (_Float16)pSrc[2 * l];
  
        yt = (_Float16)pSrc[2 * i + 1] - (_Float16)pSrc[2 * l + 1];
        a1 = (_Float16)pSrc[2 * l + 1] + (_Float16)pSrc[2 * i + 1];
  
        p0 = (_Float16)xt * (_Float16)cosVal;
        p1 = (_Float16)yt * (_Float16)sinVal;
        p2 = (_Float16)yt * (_Float16)cosVal;
        p3 = (_Float16)xt * (_Float16)sinVal;
  
        pSrc[2 * i]     = a0;
        pSrc[2 * i + 1] = a1;
  
        pSrc[2 * l]     = (_Float16)p0 + (_Float16)p1;
        pSrc[2 * l + 1] = (_Float16)p2 - (_Float16)p3;

    }

    // first col
    arm_radix4_butterfly_f16( pSrc, n2, (float16_t*)pCoef, 2U);
    // second col
    arm_radix4_butterfly_f16( pSrc + fftLen, n2, (float16_t*)pCoef, 2U);

}


/**
  @brief         Processing function for the floating-point Radix-4 CFFT/CIFFT.
  @deprecated    Do not use this function. It has been superseded by \ref arm_cfft_f16 and will be removed in the future.
  @param[in]     S    points to an instance of the floating-point Radix-4 CFFT/CIFFT structure
  @param[in,out] pSrc points to the complex data buffer of size <code>2*fftLen</code>. Processing occurs in-place
 */

ARM_DSP_ATTRIBUTE void arm_cfft_radix4_f16(
  const arm_cfft_radix4_instance_f16 * S,
        float16_t * pSrc)
{
   if (S->ifftFlag == 1U)
   {
      /*  Complex IFFT radix-4  */
      arm_radix4_butterfly_inverse_f16(pSrc, S->fftLen, S->pTwiddle, S->twidCoefModifier, S->onebyfftLen);
   }
   else
   {
      /*  Complex FFT radix-4  */
      arm_radix4_butterfly_f16(pSrc, S->fftLen, S->pTwiddle, S->twidCoefModifier);
   }

   if (S->bitReverseFlag == 1U)
   {
      /*  Bit Reversal */
      arm_bitreversal_f16(pSrc, S->fftLen, S->bitRevFactor, S->pBitRevTable);
   }

}

/**
  @} end of ComplexFFTDeprecated group
 */

/* ----------------------------------------------------------------------
 * Internal helper function used by the FFTs
 * ---------------------------------------------------------------------- */

/*
* @brief  Core function for the floating-point CFFT butterfly process.
* @param[in, out] *pSrc            points to the in-place buffer of floating-point data type.
* @param[in]      fftLen           length of the FFT.
* @param[in]      *pCoef           points to the twiddle coefficient buffer.
* @param[in]      twidCoefModifier twiddle coefficient modifier that supports different size FFTs with the same twiddle factor table.
*/

ARM_DSP_ATTRIBUTE void arm_radix4_butterfly_f16(
float16_t * pSrc,
uint16_t fftLen,
const float16_t * pCoef,
uint16_t twidCoefModifier)
{

   float16_t co1, co2, co3, si1, si2, si3;
   uint32_t ia1, ia2, ia3;
   uint32_t i0, i1, i2, i3;
   uint32_t n1, n2, j, k;

#if defined (ARM_MATH_DSP)

   /* Run the below code for Cortex-M4 and Cortex-M3 */

   float16_t xaIn, yaIn, xbIn, ybIn, xcIn, ycIn, xdIn, ydIn;
   float16_t Xaplusc, Xbplusd, Yaplusc, Ybplusd, Xaminusc, Xbminusd, Yaminusc,
   Ybminusd;
   float16_t Xb12C_out, Yb12C_out, Xc12C_out, Yc12C_out, Xd12C_out, Yd12C_out;
   float16_t Xb12_out, Yb12_out, Xc12_out, Yc12_out, Xd12_out, Yd12_out;
   float16_t *ptr1;
   float16_t p0,p1,p2,p3,p4,p5;
   float16_t a0,a1,a2,a3,a4,a5,a6,a7;

   /*  Initializations for the first stage */
   n2 = fftLen;
   n1 = n2;

   /* n2 = fftLen/4 */
   n2 >>= 2U;
   i0 = 0U;
   ia1 = 0U;

   j = n2;

   /*  Calculation of first stage */
   do
   {
      /*  index calculation for the input as, */
      /*  pSrc[i0 + 0], pSrc[i0 + fftLen/4], pSrc[i0 + fftLen/2], pSrc[i0 + 3fftLen/4] */
      i1 = i0 + n2;
      i2 = i1 + n2;
      i3 = i2 + n2;

      xaIn = pSrc[(2U * i0)];
      yaIn = pSrc[(2U * i0) + 1U];

      xbIn = pSrc[(2U * i1)];
      ybIn = pSrc[(2U * i1) + 1U];

      xcIn = pSrc[(2U * i2)];
      ycIn = pSrc[(2U * i2) + 1U];

      xdIn = pSrc[(2U * i3)];
      ydIn = pSrc[(2U * i3) + 1U];

      /* xa + xc */
      Xaplusc = (_Float16)xaIn + (_Float16)xcIn;
      /* xb + xd */
      Xbplusd = (_Float16)xbIn + (_Float16)xdIn;
      /* ya + yc */
      Yaplusc = (_Float16)yaIn + (_Float16)ycIn;
      /* yb + yd */
      Ybplusd = (_Float16)ybIn + (_Float16)ydIn;

      /*  index calculation for the coefficients */
      ia2 = ia1 + ia1;
      co2 = pCoef[ia2 * 2U];
      si2 = pCoef[(ia2 * 2U) + 1U];

      /* xa - xc */
      Xaminusc = (_Float16)xaIn - (_Float16)xcIn;
      /* xb - xd */
      Xbminusd = (_Float16)xbIn - (_Float16)xdIn;
      /* ya - yc */
      Yaminusc = (_Float16)yaIn - (_Float16)ycIn;
      /* yb - yd */
      Ybminusd = (_Float16)ybIn - (_Float16)ydIn;

      /* xa' = xa + xb + xc + xd */
      pSrc[(2U * i0)] = (_Float16)Xaplusc + (_Float16)Xbplusd;
      /* ya' = ya + yb + yc + yd */
      pSrc[(2U * i0) + 1U] = (_Float16)Yaplusc + (_Float16)Ybplusd;

      /* (xa - xc) + (yb - yd) */
      Xb12C_out = ((_Float16)Xaminusc + (_Float16)Ybminusd);
      /* (ya - yc) + (xb - xd) */
      Yb12C_out = ((_Float16)Yaminusc - (_Float16)Xbminusd);
      /* (xa + xc) - (xb + xd) */
      Xc12C_out = ((_Float16)Xaplusc - (_Float16)Xbplusd);
      /* (ya + yc) - (yb + yd) */
      Yc12C_out = ((_Float16)Yaplusc - (_Float16)Ybplusd);
      /* (xa - xc) - (yb - yd) */
      Xd12C_out = ((_Float16)Xaminusc - (_Float16)Ybminusd);
      /* (ya - yc) + (xb - xd) */
      Yd12C_out = ((_Float16)Xbminusd + (_Float16)Yaminusc);

      co1 = pCoef[ia1 * 2U];
      si1 = pCoef[(ia1 * 2U) + 1U];

      /*  index calculation for the coefficients */
      ia3 = ia2 + ia1;
      co3 = pCoef[ia3 * 2U];
      si3 = pCoef[(ia3 * 2U) + 1U];

      Xb12_out = (_Float16)Xb12C_out * (_Float16)co1;
      Yb12_out = (_Float16)Yb12C_out * (_Float16)co1;
      Xc12_out = (_Float16)Xc12C_out * (_Float16)co2;
      Yc12_out = (_Float16)Yc12C_out * (_Float16)co2;
      Xd12_out = (_Float16)Xd12C_out * (_Float16)co3;
      Yd12_out = (_Float16)Yd12C_out * (_Float16)co3;

      /* xb' = (xa+yb-xc-yd)co1 - (ya-xb-yc+xd)(si1) */
      //Xb12_out -= Yb12C_out * si1;
      p0 = (_Float16)Yb12C_out * (_Float16)si1;
      /* yb' = (ya-xb-yc+xd)co1 + (xa+yb-xc-yd)(si1) */
      //Yb12_out += Xb12C_out * si1;
      p1 = (_Float16)Xb12C_out * (_Float16)si1;
      /* xc' = (xa-xb+xc-xd)co2 - (ya-yb+yc-yd)(si2) */
      //Xc12_out -= Yc12C_out * si2;
      p2 = (_Float16)Yc12C_out * (_Float16)si2;
      /* yc' = (ya-yb+yc-yd)co2 + (xa-xb+xc-xd)(si2) */
      //Yc12_out += Xc12C_out * si2;
      p3 = (_Float16)Xc12C_out * (_Float16)si2;
      /* xd' = (xa-yb-xc+yd)co3 - (ya+xb-yc-xd)(si3) */
      //Xd12_out -= Yd12C_out * si3;
      p4 = (_Float16)Yd12C_out * (_Float16)si3;
      /* yd' = (ya+xb-yc-xd)co3 + (xa-yb-xc+yd)(si3) */
      //Yd12_out += Xd12C_out * si3;
      p5 = (_Float16)Xd12C_out * (_Float16)si3;

      Xb12_out += (_Float16)p0;
      Yb12_out -= (_Float16)p1;
      Xc12_out += (_Float16)p2;
      Yc12_out -= (_Float16)p3;
      Xd12_out += (_Float16)p4;
      Yd12_out -= (_Float16)p5;

      /* xc' = (xa-xb+xc-xd)co2 + (ya-yb+yc-yd)(si2) */
      pSrc[2U * i1] = Xc12_out;

      /* yc' = (ya-yb+yc-yd)co2 - (xa-xb+xc-xd)(si2) */
      pSrc[(2U * i1) + 1U] = Yc12_out;

      /* xb' = (xa+yb-xc-yd)co1 + (ya-xb-yc+xd)(si1) */
      pSrc[2U * i2] = Xb12_out;

      /* yb' = (ya-xb-yc+xd)co1 - (xa+yb-xc-yd)(si1) */
      pSrc[(2U * i2) + 1U] = Yb12_out;

      /* xd' = (xa-yb-xc+yd)co3 + (ya+xb-yc-xd)(si3) */
      pSrc[2U * i3] = Xd12_out;

      /* yd' = (ya+xb-yc-xd)co3 - (xa-yb-xc+yd)(si3) */
      pSrc[(2U * i3) + 1U] = Yd12_out;

      /*  Twiddle coefficients index modifier */
      ia1 += twidCoefModifier;

      /*  Updating input index */
      i0++;

   }
   while (--j);

   twidCoefModifier <<= 2U;

   /*  Calculation of second stage to excluding last stage */
   for (k = fftLen >> 2U; k > 4U; k >>= 2U)
   {
      /*  Initializations for the first stage */
      n1 = n2;
      n2 >>= 2U;
      ia1 = 0U;

      /*  Calculation of first stage */
      j = 0;
      do
      {
         /*  index calculation for the coefficients */
         ia2 = ia1 + ia1;
         ia3 = ia2 + ia1;
         co1 = pCoef[ia1 * 2U];
         si1 = pCoef[(ia1 * 2U) + 1U];
         co2 = pCoef[ia2 * 2U];
         si2 = pCoef[(ia2 * 2U) + 1U];
         co3 = pCoef[ia3 * 2U];
         si3 = pCoef[(ia3 * 2U) + 1U];

         /*  Twiddle coefficients index modifier */
         ia1 += twidCoefModifier;

         i0 = j;
         do
         {
            /*  index calculation for the input as, */
            /*  pSrc[i0 + 0], pSrc[i0 + fftLen/4], pSrc[i0 + fftLen/2], pSrc[i0 + 3fftLen/4] */
            i1 = i0 + n2;
            i2 = i1 + n2;
            i3 = i2 + n2;

            xaIn = pSrc[(2U * i0)];
            yaIn = pSrc[(2U * i0) + 1U];

            xbIn = pSrc[(2U * i1)];
            ybIn = pSrc[(2U * i1) + 1U];

            xcIn = pSrc[(2U * i2)];
            ycIn = pSrc[(2U * i2) + 1U];

            xdIn = pSrc[(2U * i3)];
            ydIn = pSrc[(2U * i3) + 1U];

            /* xa - xc */
            Xaminusc = (_Float16)xaIn - (_Float16)xcIn;
            /* (xb - xd) */
            Xbminusd = (_Float16)xbIn - (_Float16)xdIn;
            /* ya - yc */
            Yaminusc = (_Float16)yaIn - (_Float16)ycIn;
            /* (yb - yd) */
            Ybminusd = (_Float16)ybIn - (_Float16)ydIn;

            /* xa + xc */
            Xaplusc = (_Float16)xaIn + (_Float16)xcIn;
            /* xb + xd */
            Xbplusd = (_Float16)xbIn + (_Float16)xdIn;
            /* ya + yc */
            Yaplusc = (_Float16)yaIn + (_Float16)ycIn;
            /* yb + yd */
            Ybplusd = (_Float16)ybIn + (_Float16)ydIn;

            /* (xa - xc) + (yb - yd) */
            Xb12C_out = ((_Float16)Xaminusc + (_Float16)Ybminusd);
            /* (ya - yc) -  (xb - xd) */
            Yb12C_out = ((_Float16)Yaminusc - (_Float16)Xbminusd);
            /* xa + xc -(xb + xd) */
            Xc12C_out = ((_Float16)Xaplusc - (_Float16)Xbplusd);
            /* (ya + yc) - (yb + yd) */
            Yc12C_out = ((_Float16)Yaplusc - (_Float16)Ybplusd);
            /* (xa - xc) - (yb - yd) */
            Xd12C_out = ((_Float16)Xaminusc - (_Float16)Ybminusd);
            /* (ya - yc) +  (xb - xd) */
            Yd12C_out = ((_Float16)Xbminusd + (_Float16)Yaminusc);

            pSrc[(2U * i0)] = (_Float16)Xaplusc + (_Float16)Xbplusd;
            pSrc[(2U * i0) + 1U] = (_Float16)Yaplusc + (_Float16)Ybplusd;

            Xb12_out = (_Float16)Xb12C_out * (_Float16)co1;
            Yb12_out = (_Float16)Yb12C_out * (_Float16)co1;
            Xc12_out = (_Float16)Xc12C_out * (_Float16)co2;
            Yc12_out = (_Float16)Yc12C_out * (_Float16)co2;
            Xd12_out = (_Float16)Xd12C_out * (_Float16)co3;
            Yd12_out = (_Float16)Yd12C_out * (_Float16)co3;

            /* xb' = (xa+yb-xc-yd)co1 - (ya-xb-yc+xd)(si1) */
            //Xb12_out -= Yb12C_out * si1;
            p0 = (_Float16)Yb12C_out * (_Float16)si1;
            /* yb' = (ya-xb-yc+xd)co1 + (xa+yb-xc-yd)(si1) */
            //Yb12_out += Xb12C_out * si1;
            p1 = (_Float16)Xb12C_out * (_Float16)si1;
            /* xc' = (xa-xb+xc-xd)co2 - (ya-yb+yc-yd)(si2) */
            //Xc12_out -= Yc12C_out * si2;
            p2 = (_Float16)Yc12C_out * (_Float16)si2;
            /* yc' = (ya-yb+yc-yd)co2 + (xa-xb+xc-xd)(si2) */
            //Yc12_out += Xc12C_out * si2;
            p3 = (_Float16)Xc12C_out * (_Float16)si2;
            /* xd' = (xa-yb-xc+yd)co3 - (ya+xb-yc-xd)(si3) */
            //Xd12_out -= Yd12C_out * si3;
            p4 = (_Float16)Yd12C_out * (_Float16)si3;
            /* yd' = (ya+xb-yc-xd)co3 + (xa-yb-xc+yd)(si3) */
            //Yd12_out += Xd12C_out * si3;
            p5 = (_Float16)Xd12C_out * (_Float16)si3;

            Xb12_out += (_Float16)p0;
            Yb12_out -= (_Float16)p1;
            Xc12_out += (_Float16)p2;
            Yc12_out -= (_Float16)p3;
            Xd12_out += (_Float16)p4;
            Yd12_out -= (_Float16)p5;

            /* xc' = (xa-xb+xc-xd)co2 + (ya-yb+yc-yd)(si2) */
            pSrc[2U * i1] = Xc12_out;

            /* yc' = (ya-yb+yc-yd)co2 - (xa-xb+xc-xd)(si2) */
            pSrc[(2U * i1) + 1U] = Yc12_out;

            /* xb' = (xa+yb-xc-yd)co1 + (ya-xb-yc+xd)(si1) */
            pSrc[2U * i2] = Xb12_out;

            /* yb' = (ya-xb-yc+xd)co1 - (xa+yb-xc-yd)(si1) */
            pSrc[(2U * i2) + 1U] = Yb12_out;

            /* xd' = (xa-yb-xc+yd)co3 + (ya+xb-yc-xd)(si3) */
            pSrc[2U * i3] = Xd12_out;

            /* yd' = (ya+xb-yc-xd)co3 - (xa-yb-xc+yd)(si3) */
            pSrc[(2U * i3) + 1U] = Yd12_out;

            i0 += n1;
         } while (i0 < fftLen);
         j++;
      } while (j <= (n2 - 1U));
      twidCoefModifier <<= 2U;
   }

   j = fftLen >> 2;
   ptr1 = &pSrc[0];

   /*  Calculations of last stage */
   do
   {
      xaIn = ptr1[0];
      yaIn = ptr1[1];
      xbIn = ptr1[2];
      ybIn = ptr1[3];
      xcIn = ptr1[4];
      ycIn = ptr1[5];
      xdIn = ptr1[6];
      ydIn = ptr1[7];

      /* xa + xc */
      Xaplusc = (_Float16)xaIn + (_Float16)xcIn;

      /* xa - xc */
      Xaminusc = (_Float16)xaIn - (_Float16)xcIn;

      /* ya + yc */
      Yaplusc = (_Float16)yaIn + (_Float16)ycIn;

      /* ya - yc */
      Yaminusc = (_Float16)yaIn - (_Float16)ycIn;

      /* xb + xd */
      Xbplusd = (_Float16)xbIn + (_Float16)xdIn;

      /* yb + yd */
      Ybplusd = (_Float16)ybIn + (_Float16)ydIn;

      /* (xb-xd) */
      Xbminusd = (_Float16)xbIn - (_Float16)xdIn;

      /* (yb-yd) */
      Ybminusd = (_Float16)ybIn - (_Float16)ydIn;

      /* xa' = xa + xb + xc + xd */
      a0 = ((_Float16)Xaplusc + (_Float16)Xbplusd);
      /* ya' = ya + yb + yc + yd */
      a1 = ((_Float16)Yaplusc + (_Float16)Ybplusd);
      /* xc' = (xa-xb+xc-xd) */
      a2 = ((_Float16)Xaplusc - (_Float16)Xbplusd);
      /* yc' = (ya-yb+yc-yd) */
      a3 = ((_Float16)Yaplusc - (_Float16)Ybplusd);
      /* xb' = (xa+yb-xc-yd) */
      a4 = ((_Float16)Xaminusc + (_Float16)Ybminusd);
      /* yb' = (ya-xb-yc+xd) */
      a5 = ((_Float16)Yaminusc - (_Float16)Xbminusd);
      /* xd' = (xa-yb-xc+yd)) */
      a6 = ((_Float16)Xaminusc - (_Float16)Ybminusd);
      /* yd' = (ya+xb-yc-xd) */
      a7 = ((_Float16)Xbminusd + (_Float16)Yaminusc);

      ptr1[0] = a0;
      ptr1[1] = a1;
      ptr1[2] = a2;
      ptr1[3] = a3;
      ptr1[4] = a4;
      ptr1[5] = a5;
      ptr1[6] = a6;
      ptr1[7] = a7;

      /* increment pointer by 8 */
      ptr1 += 8U;
   } while (--j);

#else

   float16_t t1, t2, r1, r2, s1, s2;

   /* Run the below code for Cortex-M0 */

   /*  Initializations for the fft calculation */
   n2 = fftLen;
   n1 = n2;
   for (k = fftLen; k > 1U; k >>= 2U)
   {
      /*  Initializations for the fft calculation */
      n1 = n2;
      n2 >>= 2U;
      ia1 = 0U;

      /*  FFT Calculation */
      j = 0;
      do
      {
         /*  index calculation for the coefficients */
         ia2 = ia1 + ia1;
         ia3 = ia2 + ia1;
         co1 = pCoef[ia1 * 2U];
         si1 = pCoef[(ia1 * 2U) + 1U];
         co2 = pCoef[ia2 * 2U];
         si2 = pCoef[(ia2 * 2U) + 1U];
         co3 = pCoef[ia3 * 2U];
         si3 = pCoef[(ia3 * 2U) + 1U];

         /*  Twiddle coefficients index modifier */
         ia1 = ia1 + twidCoefModifier;

         i0 = j;
         do
         {
            /*  index calculation for the input as, */
            /*  pSrc[i0 + 0], pSrc[i0 + fftLen/4], pSrc[i0 + fftLen/2], pSrc[i0 + 3fftLen/4] */
            i1 = i0 + n2;
            i2 = i1 + n2;
            i3 = i2 + n2;

            /* xa + xc */
            r1 = (_Float16)pSrc[(2U * i0)] + (_Float16)pSrc[(2U * i2)];

            /* xa - xc */
            r2 = (_Float16)pSrc[(2U * i0)] - (_Float16)pSrc[(2U * i2)];

            /* ya + yc */
            s1 = (_Float16)pSrc[(2U * i0) + 1U] + (_Float16)pSrc[(2U * i2) + 1U];

            /* ya - yc */
            s2 = (_Float16)pSrc[(2U * i0) + 1U] - (_Float16)pSrc[(2U * i2) + 1U];

            /* xb + xd */
            t1 = (_Float16)pSrc[2U * i1] + (_Float16)pSrc[2U * i3];

            /* xa' = xa + xb + xc + xd */
            pSrc[2U * i0] = (_Float16)r1 + (_Float16)t1;

            /* xa + xc -(xb + xd) */
            r1 = (_Float16)r1 - (_Float16)t1;

            /* yb + yd */
            t2 = (_Float16)pSrc[(2U * i1) + 1U] + (_Float16)pSrc[(2U * i3) + 1U];

            /* ya' = ya + yb + yc + yd */
            pSrc[(2U * i0) + 1U] = (_Float16)s1 + (_Float16)t2;

            /* (ya + yc) - (yb + yd) */
            s1 = (_Float16)s1 - (_Float16)t2;

            /* (yb - yd) */
            t1 = (_Float16)pSrc[(2U * i1) + 1U] - (_Float16)pSrc[(2U * i3) + 1U];

            /* (xb - xd) */
            t2 = (_Float16)pSrc[2U * i1] - (_Float16)pSrc[2U * i3];

            /* xc' = (xa-xb+xc-xd)co2 + (ya-yb+yc-yd)(si2) */
            pSrc[2U * i1] = ((_Float16)r1 * (_Float16)co2) + ((_Float16)s1 * (_Float16)si2);

            /* yc' = (ya-yb+yc-yd)co2 - (xa-xb+xc-xd)(si2) */
            pSrc[(2U * i1) + 1U] = ((_Float16)s1 * (_Float16)co2) - ((_Float16)r1 * (_Float16)si2);

            /* (xa - xc) + (yb - yd) */
            r1 = (_Float16)r2 + (_Float16)t1;

            /* (xa - xc) - (yb - yd) */
            r2 = (_Float16)r2 - (_Float16)t1;

            /* (ya - yc) -  (xb - xd) */
            s1 = (_Float16)s2 - (_Float16)t2;

            /* (ya - yc) +  (xb - xd) */
            s2 = (_Float16)s2 + (_Float16)t2;

            /* xb' = (xa+yb-xc-yd)co1 + (ya-xb-yc+xd)(si1) */
            pSrc[2U * i2] = ((_Float16)r1 * (_Float16)co1) + ((_Float16)s1 * (_Float16)si1);

            /* yb' = (ya-xb-yc+xd)co1 - (xa+yb-xc-yd)(si1) */
            pSrc[(2U * i2) + 1U] = ((_Float16)s1 * (_Float16)co1) - ((_Float16)r1 * (_Float16)si1);

            /* xd' = (xa-yb-xc+yd)co3 + (ya+xb-yc-xd)(si3) */
            pSrc[2U * i3] = ((_Float16)r2 * (_Float16)co3) + ((_Float16)s2 * (_Float16)si3);

            /* yd' = (ya+xb-yc-xd)co3 - (xa-yb-xc+yd)(si3) */
            pSrc[(2U * i3) + 1U] = ((_Float16)s2 * (_Float16)co3) - ((_Float16)r2 * (_Float16)si3);

            i0 += n1;
         } while ( i0 < fftLen);
         j++;
      } while (j <= (n2 - 1U));
      twidCoefModifier <<= 2U;
   }

#endif /* #if defined (ARM_MATH_DSP) */

}

/*
* @brief  Core function for the floating-point CIFFT butterfly process.
* @param[in, out] *pSrc            points to the in-place buffer of floating-point data type.
* @param[in]      fftLen           length of the FFT.
* @param[in]      *pCoef           points to twiddle coefficient buffer.
* @param[in]      twidCoefModifier twiddle coefficient modifier that supports different size FFTs with the same twiddle factor table.
* @param[in]      onebyfftLen      value of 1/fftLen.
*/

ARM_DSP_ATTRIBUTE void arm_radix4_butterfly_inverse_f16(
float16_t * pSrc,
uint16_t fftLen,
const float16_t * pCoef,
uint16_t twidCoefModifier,
float16_t onebyfftLen)
{
   float16_t co1, co2, co3, si1, si2, si3;
   uint32_t ia1, ia2, ia3;
   uint32_t i0, i1, i2, i3;
   uint32_t n1, n2, j, k;

#if defined (ARM_MATH_DSP)

   float16_t xaIn, yaIn, xbIn, ybIn, xcIn, ycIn, xdIn, ydIn;
   float16_t Xaplusc, Xbplusd, Yaplusc, Ybplusd, Xaminusc, Xbminusd, Yaminusc,
   Ybminusd;
   float16_t Xb12C_out, Yb12C_out, Xc12C_out, Yc12C_out, Xd12C_out, Yd12C_out;
   float16_t Xb12_out, Yb12_out, Xc12_out, Yc12_out, Xd12_out, Yd12_out;
   float16_t *ptr1;
   float16_t p0,p1,p2,p3,p4,p5,p6,p7;
   float16_t a0,a1,a2,a3,a4,a5,a6,a7;


   /*  Initializations for the first stage */
   n2 = fftLen;
   n1 = n2;

   /* n2 = fftLen/4 */
   n2 >>= 2U;
   i0 = 0U;
   ia1 = 0U;

   j = n2;

   /*  Calculation of first stage */
   do
   {
      /*  index calculation for the input as, */
      /*  pSrc[i0 + 0], pSrc[i0 + fftLen/4], pSrc[i0 + fftLen/2], pSrc[i0 + 3fftLen/4] */
      i1 = i0 + n2;
      i2 = i1 + n2;
      i3 = i2 + n2;

      /*  Butterfly implementation */
      xaIn = pSrc[(2U * i0)];
      yaIn = pSrc[(2U * i0) + 1U];

      xcIn = pSrc[(2U * i2)];
      ycIn = pSrc[(2U * i2) + 1U];

      xbIn = pSrc[(2U * i1)];
      ybIn = pSrc[(2U * i1) + 1U];

      xdIn = pSrc[(2U * i3)];
      ydIn = pSrc[(2U * i3) + 1U];

      /* xa + xc */
      Xaplusc = (_Float16)xaIn + (_Float16)xcIn;
      /* xb + xd */
      Xbplusd = (_Float16)xbIn + (_Float16)xdIn;
      /* ya + yc */
      Yaplusc = (_Float16)yaIn + (_Float16)ycIn;
      /* yb + yd */
      Ybplusd = (_Float16)ybIn + (_Float16)ydIn;

      /*  index calculation for the coefficients */
      ia2 = ia1 + ia1;
      co2 = pCoef[ia2 * 2U];
      si2 = pCoef[(ia2 * 2U) + 1U];

      /* xa - xc */
      Xaminusc = (_Float16)xaIn - (_Float16)xcIn;
      /* xb - xd */
      Xbminusd = (_Float16)xbIn - (_Float16)xdIn;
      /* ya - yc */
      Yaminusc = (_Float16)yaIn - (_Float16)ycIn;
      /* yb - yd */
      Ybminusd = (_Float16)ybIn - (_Float16)ydIn;

      /* xa' = xa + xb + xc + xd */
      pSrc[(2U * i0)] = (_Float16)Xaplusc + (_Float16)Xbplusd;

      /* ya' = ya + yb + yc + yd */
      pSrc[(2U * i0) + 1U] = (_Float16)Yaplusc + (_Float16)Ybplusd;

      /* (xa - xc) - (yb - yd) */
      Xb12C_out = ((_Float16)Xaminusc - (_Float16)Ybminusd);
      /* (ya - yc) + (xb - xd) */
      Yb12C_out = ((_Float16)Yaminusc + (_Float16)Xbminusd);
      /* (xa + xc) - (xb + xd) */
      Xc12C_out = ((_Float16)Xaplusc - (_Float16)Xbplusd);
      /* (ya + yc) - (yb + yd) */
      Yc12C_out = ((_Float16)Yaplusc - (_Float16)Ybplusd);
      /* (xa - xc) + (yb - yd) */
      Xd12C_out = ((_Float16)Xaminusc + (_Float16)Ybminusd);
      /* (ya - yc) - (xb - xd) */
      Yd12C_out = ((_Float16)Yaminusc - (_Float16)Xbminusd);

      co1 = pCoef[ia1 * 2U];
      si1 = pCoef[(ia1 * 2U) + 1U];

      /*  index calculation for the coefficients */
      ia3 = ia2 + ia1;
      co3 = pCoef[ia3 * 2U];
      si3 = pCoef[(ia3 * 2U) + 1U];

      Xb12_out = (_Float16)Xb12C_out * (_Float16)co1;
      Yb12_out = (_Float16)Yb12C_out * (_Float16)co1;
      Xc12_out = (_Float16)Xc12C_out * (_Float16)co2;
      Yc12_out = (_Float16)Yc12C_out * (_Float16)co2;
      Xd12_out = (_Float16)Xd12C_out * (_Float16)co3;
      Yd12_out = (_Float16)Yd12C_out * (_Float16)co3;

      /* xb' = (xa+yb-xc-yd)co1 - (ya-xb-yc+xd)(si1) */
      //Xb12_out -= Yb12C_out * si1;
      p0 = (_Float16)Yb12C_out * (_Float16)si1;
      /* yb' = (ya-xb-yc+xd)co1 + (xa+yb-xc-yd)(si1) */
      //Yb12_out += Xb12C_out * si1;
      p1 = (_Float16)Xb12C_out * (_Float16)si1;
      /* xc' = (xa-xb+xc-xd)co2 - (ya-yb+yc-yd)(si2) */
      //Xc12_out -= Yc12C_out * si2;
      p2 = (_Float16)Yc12C_out * (_Float16)si2;
      /* yc' = (ya-yb+yc-yd)co2 + (xa-xb+xc-xd)(si2) */
      //Yc12_out += Xc12C_out * si2;
      p3 = (_Float16)Xc12C_out * (_Float16)si2;
      /* xd' = (xa-yb-xc+yd)co3 - (ya+xb-yc-xd)(si3) */
      //Xd12_out -= Yd12C_out * si3;
      p4 = (_Float16)Yd12C_out * (_Float16)si3;
      /* yd' = (ya+xb-yc-xd)co3 + (xa-yb-xc+yd)(si3) */
      //Yd12_out += Xd12C_out * si3;
      p5 =(_Float16) Xd12C_out * (_Float16)si3;

      Xb12_out -= (_Float16)p0;
      Yb12_out += (_Float16)p1;
      Xc12_out -= (_Float16)p2;
      Yc12_out += (_Float16)p3;
      Xd12_out -= (_Float16)p4;
      Yd12_out += (_Float16)p5;

      /* xc' = (xa-xb+xc-xd)co2 - (ya-yb+yc-yd)(si2) */
      pSrc[2U * i1] = Xc12_out;

      /* yc' = (ya-yb+yc-yd)co2 + (xa-xb+xc-xd)(si2) */
      pSrc[(2U * i1) + 1U] = Yc12_out;

      /* xb' = (xa+yb-xc-yd)co1 - (ya-xb-yc+xd)(si1) */
      pSrc[2U * i2] = Xb12_out;

      /* yb' = (ya-xb-yc+xd)co1 + (xa+yb-xc-yd)(si1) */
      pSrc[(2U * i2) + 1U] = Yb12_out;

      /* xd' = (xa-yb-xc+yd)co3 - (ya+xb-yc-xd)(si3) */
      pSrc[2U * i3] = Xd12_out;

      /* yd' = (ya+xb-yc-xd)co3 + (xa-yb-xc+yd)(si3) */
      pSrc[(2U * i3) + 1U] = Yd12_out;

      /*  Twiddle coefficients index modifier */
      ia1 = ia1 + twidCoefModifier;

      /*  Updating input index */
      i0 = i0 + 1U;

   } while (--j);

   twidCoefModifier <<= 2U;

   /*  Calculation of second stage to excluding last stage */
   for (k = fftLen >> 2U; k > 4U; k >>= 2U)
   {
      /*  Initializations for the first stage */
      n1 = n2;
      n2 >>= 2U;
      ia1 = 0U;

      /*  Calculation of first stage */
      j = 0;
      do
      {
         /*  index calculation for the coefficients */
         ia2 = ia1 + ia1;
         ia3 = ia2 + ia1;
         co1 = pCoef[ia1 * 2U];
         si1 = pCoef[(ia1 * 2U) + 1U];
         co2 = pCoef[ia2 * 2U];
         si2 = pCoef[(ia2 * 2U) + 1U];
         co3 = pCoef[ia3 * 2U];
         si3 = pCoef[(ia3 * 2U) + 1U];

         /*  Twiddle coefficients index modifier */
         ia1 = ia1 + twidCoefModifier;

         i0 = j;
         do
         {
            /*  index calculation for the input as, */
            /*  pSrc[i0 + 0], pSrc[i0 + fftLen/4], pSrc[i0 + fftLen/2], pSrc[i0 + 3fftLen/4] */
            i1 = i0 + n2;
            i2 = i1 + n2;
            i3 = i2 + n2;

            xaIn = pSrc[(2U * i0)];
            yaIn = pSrc[(2U * i0) + 1U];

            xbIn = pSrc[(2U * i1)];
            ybIn = pSrc[(2U * i1) + 1U];

            xcIn = pSrc[(2U * i2)];
            ycIn = pSrc[(2U * i2) + 1U];

            xdIn = pSrc[(2U * i3)];
            ydIn = pSrc[(2U * i3) + 1U];

            /* xa - xc */
            Xaminusc = (_Float16)xaIn - (_Float16)xcIn;
            /* (xb - xd) */
            Xbminusd = (_Float16)xbIn - (_Float16)xdIn;
            /* ya - yc */
            Yaminusc = (_Float16)yaIn - (_Float16)ycIn;
            /* (yb - yd) */
            Ybminusd = (_Float16)ybIn - (_Float16)ydIn;

            /* xa + xc */
            Xaplusc = (_Float16)xaIn + (_Float16)xcIn;
            /* xb + xd */
            Xbplusd = (_Float16)xbIn + (_Float16)xdIn;
            /* ya + yc */
            Yaplusc = (_Float16)yaIn + (_Float16)ycIn;
            /* yb + yd */
            Ybplusd = (_Float16)ybIn + (_Float16)ydIn;

            /* (xa - xc) - (yb - yd) */
            Xb12C_out = ((_Float16)Xaminusc - (_Float16)Ybminusd);
            /* (ya - yc) +  (xb - xd) */
            Yb12C_out = ((_Float16)Yaminusc + (_Float16)Xbminusd);
            /* xa + xc -(xb + xd) */
            Xc12C_out = ((_Float16)Xaplusc - (_Float16)Xbplusd);
            /* (ya + yc) - (yb + yd) */
            Yc12C_out = ((_Float16)Yaplusc - (_Float16)Ybplusd);
            /* (xa - xc) + (yb - yd) */
            Xd12C_out = ((_Float16)Xaminusc + (_Float16)Ybminusd);
            /* (ya - yc) -  (xb - xd) */
            Yd12C_out = ((_Float16)Yaminusc - (_Float16)Xbminusd);

            pSrc[(2U * i0)] = (_Float16)Xaplusc + (_Float16)Xbplusd;
            pSrc[(2U * i0) + 1U] = (_Float16)Yaplusc + (_Float16)Ybplusd;

            Xb12_out = (_Float16)Xb12C_out * (_Float16)co1;
            Yb12_out = (_Float16)Yb12C_out * (_Float16)co1;
            Xc12_out = (_Float16)Xc12C_out * (_Float16)co2;
            Yc12_out = (_Float16)Yc12C_out * (_Float16)co2;
            Xd12_out = (_Float16)Xd12C_out * (_Float16)co3;
            Yd12_out = (_Float16)Yd12C_out * (_Float16)co3;

            /* xb' = (xa+yb-xc-yd)co1 - (ya-xb-yc+xd)(si1) */
            //Xb12_out -= Yb12C_out * si1;
            p0 = (_Float16)Yb12C_out * (_Float16)si1;
            /* yb' = (ya-xb-yc+xd)co1 + (xa+yb-xc-yd)(si1) */
            //Yb12_out += Xb12C_out * si1;
            p1 = (_Float16)Xb12C_out * (_Float16)si1;
            /* xc' = (xa-xb+xc-xd)co2 - (ya-yb+yc-yd)(si2) */
            //Xc12_out -= Yc12C_out * si2;
            p2 = (_Float16)Yc12C_out * (_Float16)si2;
            /* yc' = (ya-yb+yc-yd)co2 + (xa-xb+xc-xd)(si2) */
            //Yc12_out += Xc12C_out * si2;
            p3 = (_Float16)Xc12C_out * (_Float16)si2;
            /* xd' = (xa-yb-xc+yd)co3 - (ya+xb-yc-xd)(si3) */
            //Xd12_out -= Yd12C_out * si3;
            p4 = (_Float16)Yd12C_out * (_Float16)si3;
            /* yd' = (ya+xb-yc-xd)co3 + (xa-yb-xc+yd)(si3) */
            //Yd12_out += Xd12C_out * si3;
            p5 = (_Float16)Xd12C_out * (_Float16)si3;

            Xb12_out -= (_Float16)p0;
            Yb12_out += (_Float16)p1;
            Xc12_out -= (_Float16)p2;
            Yc12_out += (_Float16)p3;
            Xd12_out -= (_Float16)p4;
            Yd12_out += (_Float16)p5;

            /* xc' = (xa-xb+xc-xd)co2 - (ya-yb+yc-yd)(si2) */
            pSrc[2U * i1] = Xc12_out;

            /* yc' = (ya-yb+yc-yd)co2 + (xa-xb+xc-xd)(si2) */
            pSrc[(2U * i1) + 1U] = Yc12_out;

            /* xb' = (xa+yb-xc-yd)co1 - (ya-xb-yc+xd)(si1) */
            pSrc[2U * i2] = Xb12_out;

            /* yb' = (ya-xb-yc+xd)co1 + (xa+yb-xc-yd)(si1) */
            pSrc[(2U * i2) + 1U] = Yb12_out;

            /* xd' = (xa-yb-xc+yd)co3 - (ya+xb-yc-xd)(si3) */
            pSrc[2U * i3] = Xd12_out;

            /* yd' = (ya+xb-yc-xd)co3 + (xa-yb-xc+yd)(si3) */
            pSrc[(2U * i3) + 1U] = Yd12_out;

            i0 += n1;
         } while (i0 < fftLen);
         j++;
      } while (j <= (n2 - 1U));
      twidCoefModifier <<= 2U;
   }
   /*  Initializations of last stage */

   j = fftLen >> 2;
   ptr1 = &pSrc[0];

   /*  Calculations of last stage */
   do
   {
      xaIn = ptr1[0];
      yaIn = ptr1[1];
      xbIn = ptr1[2];
      ybIn = ptr1[3];
      xcIn = ptr1[4];
      ycIn = ptr1[5];
      xdIn = ptr1[6];
      ydIn = ptr1[7];

      /*  Butterfly implementation */
      /* xa + xc */
      Xaplusc = (_Float16)xaIn + (_Float16)xcIn;

      /* xa - xc */
      Xaminusc = (_Float16)xaIn - (_Float16)xcIn;

      /* ya + yc */
      Yaplusc = (_Float16)yaIn + (_Float16)ycIn;

      /* ya - yc */
      Yaminusc = (_Float16)yaIn - (_Float16)ycIn;

      /* xb + xd */
      Xbplusd = (_Float16)xbIn + (_Float16)xdIn;

      /* yb + yd */
      Ybplusd = (_Float16)ybIn + (_Float16)ydIn;

      /* (xb-xd) */
      Xbminusd = (_Float16)xbIn - (_Float16)xdIn;

      /* (yb-yd) */
      Ybminusd = (_Float16)ybIn - (_Float16)ydIn;

      /* xa' = (xa+xb+xc+xd) * onebyfftLen */
      a0 = ((_Float16)Xaplusc + (_Float16)Xbplusd);
      /* ya' = (ya+yb+yc+yd) * onebyfftLen */
      a1 = ((_Float16)Yaplusc + (_Float16)Ybplusd);
      /* xc' = (xa-xb+xc-xd) * onebyfftLen */
      a2 = ((_Float16)Xaplusc - (_Float16)Xbplusd);
      /* yc' = (ya-yb+yc-yd) * onebyfftLen  */
      a3 = ((_Float16)Yaplusc - (_Float16)Ybplusd);
      /* xb' = (xa-yb-xc+yd) * onebyfftLen */
      a4 = ((_Float16)Xaminusc - (_Float16)Ybminusd);
      /* yb' = (ya+xb-yc-xd) * onebyfftLen */
      a5 = ((_Float16)Yaminusc + (_Float16)Xbminusd);
      /* xd' = (xa-yb-xc+yd) * onebyfftLen */
      a6 = ((_Float16)Xaminusc + (_Float16)Ybminusd);
      /* yd' = (ya-xb-yc+xd) * onebyfftLen */
      a7 = ((_Float16)Yaminusc - (_Float16)Xbminusd);

      p0 = (_Float16)a0 * (_Float16)onebyfftLen;
      p1 = (_Float16)a1 * (_Float16)onebyfftLen;
      p2 = (_Float16)a2 * (_Float16)onebyfftLen;
      p3 = (_Float16)a3 * (_Float16)onebyfftLen;
      p4 = (_Float16)a4 * (_Float16)onebyfftLen;
      p5 = (_Float16)a5 * (_Float16)onebyfftLen;
      p6 = (_Float16)a6 * (_Float16)onebyfftLen;
      p7 = (_Float16)a7 * (_Float16)onebyfftLen;

      /* xa' = (xa+xb+xc+xd) * onebyfftLen */
      ptr1[0] = p0;
      /* ya' = (ya+yb+yc+yd) * onebyfftLen */
      ptr1[1] = p1;
      /* xc' = (xa-xb+xc-xd) * onebyfftLen */
      ptr1[2] = p2;
      /* yc' = (ya-yb+yc-yd) * onebyfftLen  */
      ptr1[3] = p3;
      /* xb' = (xa-yb-xc+yd) * onebyfftLen */
      ptr1[4] = p4;
      /* yb' = (ya+xb-yc-xd) * onebyfftLen */
      ptr1[5] = p5;
      /* xd' = (xa-yb-xc+yd) * onebyfftLen */
      ptr1[6] = p6;
      /* yd' = (ya-xb-yc+xd) * onebyfftLen */
      ptr1[7] = p7;

      /* increment source pointer by 8 for next calculations */
      ptr1 = ptr1 + 8U;

   } while (--j);

#else

   float16_t t1, t2, r1, r2, s1, s2;

   /* Run the below code for Cortex-M0 */

   /*  Initializations for the first stage */
   n2 = fftLen;
   n1 = n2;

   /*  Calculation of first stage */
   for (k = fftLen; k > 4U; k >>= 2U)
   {
      /*  Initializations for the first stage */
      n1 = n2;
      n2 >>= 2U;
      ia1 = 0U;

      /*  Calculation of first stage */
      j = 0;
      do
      {
         /*  index calculation for the coefficients */
         ia2 = ia1 + ia1;
         ia3 = ia2 + ia1;
         co1 = pCoef[ia1 * 2U];
         si1 = pCoef[(ia1 * 2U) + 1U];
         co2 = pCoef[ia2 * 2U];
         si2 = pCoef[(ia2 * 2U) + 1U];
         co3 = pCoef[ia3 * 2U];
         si3 = pCoef[(ia3 * 2U) + 1U];

         /*  Twiddle coefficients index modifier */
         ia1 = ia1 + twidCoefModifier;

         i0 = j;
         do
         {
            /*  index calculation for the input as, */
            /*  pSrc[i0 + 0], pSrc[i0 + fftLen/4], pSrc[i0 + fftLen/2], pSrc[i0 + 3fftLen/4] */
            i1 = i0 + n2;
            i2 = i1 + n2;
            i3 = i2 + n2;

            /* xa + xc */
            r1 = (_Float16)pSrc[(2U * i0)] + (_Float16)pSrc[(2U * i2)];

            /* xa - xc */
            r2 = (_Float16)pSrc[(2U * i0)] - (_Float16)pSrc[(2U * i2)];

            /* ya + yc */
            s1 = (_Float16)pSrc[(2U * i0) + 1U] + (_Float16)pSrc[(2U * i2) + 1U];

            /* ya - yc */
            s2 = (_Float16)pSrc[(2U * i0) + 1U] - (_Float16)pSrc[(2U * i2) + 1U];

            /* xb + xd */
            t1 = (_Float16)pSrc[2U * i1] + (_Float16)pSrc[2U * i3];

            /* xa' = xa + xb + xc + xd */
            pSrc[2U * i0] = (_Float16)r1 + (_Float16)t1;

            /* xa + xc -(xb + xd) */
            r1 = (_Float16)r1 - (_Float16)t1;

            /* yb + yd */
            t2 = (_Float16)pSrc[(2U * i1) + 1U] + (_Float16)pSrc[(2U * i3) + 1U];

            /* ya' = ya + yb + yc + yd */
            pSrc[(2U * i0) + 1U] = (_Float16)s1 + (_Float16)t2;

            /* (ya + yc) - (yb + yd) */
            s1 = (_Float16)s1 - (_Float16)t2;

            /* (yb - yd) */
            t1 = (_Float16)pSrc[(2U * i1) + 1U] - (_Float16)pSrc[(2U * i3) + 1U];

            /* (xb - xd) */
            t2 = (_Float16)pSrc[2U * i1] - (_Float16)pSrc[2U * i3];

            /* xc' = (xa-xb+xc-xd)co2 - (ya-yb+yc-yd)(si2) */
            pSrc[2U * i1] = ((_Float16)r1 * (_Float16)co2) - ((_Float16)s1 * (_Float16)si2);

            /* yc' = (ya-yb+yc-yd)co2 + (xa-xb+xc-xd)(si2) */
            pSrc[(2U * i1) + 1U] = ((_Float16)s1 * (_Float16)co2) + ((_Float16)r1 * (_Float16)si2);

            /* (xa - xc) - (yb - yd) */
            r1 = (_Float16)r2 - (_Float16)t1;

            /* (xa - xc) + (yb - yd) */
            r2 = (_Float16)r2 + (_Float16)t1;

            /* (ya - yc) +  (xb - xd) */
            s1 = (_Float16)s2 + (_Float16)t2;

            /* (ya - yc) -  (xb - xd) */
            s2 = (_Float16)s2 - (_Float16)t2;

            /* xb' = (xa+yb-xc-yd)co1 - (ya-xb-yc+xd)(si1) */
            pSrc[2U * i2] = ((_Float16)r1 * (_Float16)co1) - ((_Float16)s1 * (_Float16)si1);

            /* yb' = (ya-xb-yc+xd)co1 + (xa+yb-xc-yd)(si1) */
            pSrc[(2U * i2) + 1U] = ((_Float16)s1 * (_Float16)co1) + ((_Float16)r1 * (_Float16)si1);

            /* xd' = (xa-yb-xc+yd)co3 - (ya+xb-yc-xd)(si3) */
            pSrc[2U * i3] = ((_Float16)r2 * (_Float16)co3) - ((_Float16)s2 * (_Float16)si3);

            /* yd' = (ya+xb-yc-xd)co3 + (xa-yb-xc+yd)(si3) */
            pSrc[(2U * i3) + 1U] = ((_Float16)s2 * (_Float16)co3) + ((_Float16)r2 * (_Float16)si3);

            i0 += n1;
         } while ( i0 < fftLen);
         j++;
      } while (j <= (n2 - 1U));
      twidCoefModifier <<= 2U;
   }
   /*  Initializations of last stage */
   n1 = n2;
   n2 >>= 2U;

   /*  Calculations of last stage */
   for (i0 = 0U; i0 <= (fftLen - n1); i0 += n1)
   {
      /*  index calculation for the input as, */
      /*  pSrc[i0 + 0], pSrc[i0 + fftLen/4], pSrc[i0 + fftLen/2], pSrc[i0 + 3fftLen/4] */
      i1 = i0 + n2;
      i2 = i1 + n2;
      i3 = i2 + n2;

      /*  Butterfly implementation */
      /* xa + xc */
      r1 = (_Float16)pSrc[2U * i0] + (_Float16)pSrc[2U * i2];

      /* xa - xc */
      r2 = (_Float16)pSrc[2U * i0] - (_Float16)pSrc[2U * i2];

      /* ya + yc */
      s1 = (_Float16)pSrc[(2U * i0) + 1U] + (_Float16)pSrc[(2U * i2) + 1U];

      /* ya - yc */
      s2 = (_Float16)pSrc[(2U * i0) + 1U] - (_Float16)pSrc[(2U * i2) + 1U];

      /* xc + xd */
      t1 = (_Float16)pSrc[2U * i1] + (_Float16)pSrc[2U * i3];

      /* xa' = xa + xb + xc + xd */
      pSrc[2U * i0] = ((_Float16)r1 + (_Float16)t1) * (_Float16)onebyfftLen;

      /* (xa + xb) - (xc + xd) */
      r1 = (_Float16)r1 - (_Float16)t1;

      /* yb + yd */
      t2 = (_Float16)pSrc[(2U * i1) + 1U] + (_Float16)pSrc[(2U * i3) + 1U];

      /* ya' = ya + yb + yc + yd */
      pSrc[(2U * i0) + 1U] = ((_Float16)s1 + (_Float16)t2) * (_Float16)onebyfftLen;

      /* (ya + yc) - (yb + yd) */
      s1 = (_Float16)s1 - (_Float16)t2;

      /* (yb-yd) */
      t1 = (_Float16)pSrc[(2U * i1) + 1U] - (_Float16)pSrc[(2U * i3) + 1U];

      /* (xb-xd) */
      t2 = (_Float16)pSrc[2U * i1] - (_Float16)pSrc[2U * i3];

      /* xc' = (xa-xb+xc-xd)co2 - (ya-yb+yc-yd)(si2) */
      pSrc[2U * i1] = (_Float16)r1 * (_Float16)onebyfftLen;

      /* yc' = (ya-yb+yc-yd)co2 + (xa-xb+xc-xd)(si2) */
      pSrc[(2U * i1) + 1U] = (_Float16)s1 * (_Float16)onebyfftLen;

      /* (xa - xc) - (yb-yd) */
      r1 = (_Float16)r2 - (_Float16)t1;

      /* (xa - xc) + (yb-yd) */
      r2 = (_Float16)r2 + (_Float16)t1;

      /* (ya - yc) + (xb-xd) */
      s1 = (_Float16)s2 + (_Float16)t2;

      /* (ya - yc) - (xb-xd) */
      s2 = (_Float16)s2 - (_Float16)t2;

      /* xb' = (xa+yb-xc-yd)co1 - (ya-xb-yc+xd)(si1) */
      pSrc[2U * i2] = (_Float16)r1 * (_Float16)onebyfftLen;

      /* yb' = (ya-xb-yc+xd)co1 + (xa+yb-xc-yd)(si1) */
      pSrc[(2U * i2) + 1U] = (_Float16)s1 * (_Float16)onebyfftLen;

      /* xd' = (xa-yb-xc+yd)co3 - (ya+xb-yc-xd)(si3) */
      pSrc[2U * i3] = (_Float16)r2 * (_Float16)onebyfftLen;

      /* yd' = (ya+xb-yc-xd)co3 + (xa-yb-xc+yd)(si3) */
      pSrc[(2U * i3) + 1U] = (_Float16)s2 * (_Float16)onebyfftLen;
   }

#endif /* #if defined (ARM_MATH_DSP) */
}

#endif /* #if defined(ARM_FLOAT16_SUPPORTED) */
