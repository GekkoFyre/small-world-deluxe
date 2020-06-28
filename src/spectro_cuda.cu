/**
 **     __                 _ _   __    __           _     _ 
 **    / _\_ __ ___   __ _| | | / / /\ \ \___  _ __| | __| |
 **    \ \| '_ ` _ \ / _` | | | \ \/  \/ / _ \| '__| |/ _` |
 **    _\ \ | | | | | (_| | | |  \  /\  / (_) | |  | | (_| |
 **    \__/_| |_| |_|\__,_|_|_|   \/  \/ \___/|_|  |_|\__,_|
 **                                                         
 **                  ___     _                              
 **                 /   \___| |_   ___  _____               
 **                / /\ / _ \ | | | \ \/ / _ \              
 **               / /_//  __/ | |_| |>  <  __/              
 **              /___,' \___|_|\__,_/_/\_\___|              
 **
 **
 **   If you have downloaded the source code for "Small World Deluxe" and are reading this,
 **   then thank you from the bottom of our hearts for making use of our hard work, sweat
 **   and tears in whatever you are implementing this into!
 **
 **   Copyright (C) 2020. GekkoFyre.
 **
 **   Small World Deluxe is free software: you can redistribute it and/or modify
 **   it under the terms of the GNU General Public License as published by
 **   the Free Software Foundation, either version 3 of the License, or
 **   (at your option) any later version.
 **
 **   Small world is distributed in the hope that it will be useful,
 **   but WITHOUT ANY WARRANTY; without even the implied warranty of
 **   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **   GNU General Public License for more details.
 **
 **   You should have received a copy of the GNU General Public License
 **   along with Small World Deluxe.  If not, see <http://www.gnu.org/licenses/>.
 **
 **
 **   The latest source code updates can be obtained from [ 1 ] below at your
 **   discretion. A web-browser or the 'git' application may be required.
 **
 **   [ 1 ] - https://code.gekkofyre.io/phobos-dthorga/small-world-deluxe
 **
 ****************************************************************************************************/

#include "spectro_cuda.h"
#include <cufft.h>
#include <cuda_runtime_api.h>

/**
 * @brief SpectroFFTW::processCUDAFFT
 * @author Ville Räisänen <https://github.com/vsr83/QSpectrogram/blob/master/fftcuda.cu>
 * @param inputData
 * @param outputData
 * @param numSamples
 */
void processCUDAFFT(int16_t *inputData, float *outputData, unsigned int numSamples)
{
    cufftHandle plan;
    cufftComplex *inputDataG, *outputDataG;
    int i;

    float *inputDataC, *outputDataC;
    outputDataC = (float *)malloc(sizeof(float) * numSamples * 2);
    inputDataC  = (float *)malloc(sizeof(float) * numSamples * 2);

    for (i = 0; i < numSamples; ++i) {
      inputDataC[i * 2]     = inputData[i];
      inputDataC[i * 2 + 1] = 0.0f;
    }

    cudaMalloc((void **)&inputDataG,  sizeof(cufftComplex) * numSamples);
    cudaMalloc((void **)&outputDataG, sizeof(cufftComplex) * numSamples);

    cudaMemcpy(inputDataG, inputDataC, sizeof(cufftComplex) * numSamples, cudaMemcpyHostToDevice);
    cufftPlan1d(&plan, numSamples, CUFFT_C2C, 1);
    cufftExecC2C(plan, inputDataG, outputDataG, CUFFT_FORWARD);
    cufftDestroy(plan);
    cudaMemcpy(outputDataC, outputDataG, sizeof(cufftComplex) * numSamples, cudaMemcpyDeviceToHost);
    cudaFree(inputDataG);
    cudaFree(outputDataG);

    for (i = 0; i < numSamples; ++i) {
        outputData[i] = outputDataC[i * 2];
    }

    free(outputDataC);
    free(inputDataC);

    return;
}