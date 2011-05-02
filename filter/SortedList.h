#pragma once

#ifndef SORTED_LIST
#define SORTED_LIST

#include "Bacalar/Filter.h"

#define THREAD_PER_BLOCK_MED (128)
#define ADD_MEM_SIZE_PER_BLOCK ((((sem->GetSE(seIndex)->nbSize)/2+2)/4+1)*sizeof(unsigned))

template <typename imDataType>
float Filter<imDataType>::Median(imDataType* dst, int seIndex, imDataType* srcA, fourthParam<imDataType> p4){
	LARGE_INTEGER frq, start, stop;
	QueryPerformanceFrequency(&frq);


	if(UseCuda()){
		//cout << "median on GPU\n";
		unsigned blocks = GetImageSize()/THREAD_PER_BLOCK_MED + 1;
		//cout << blocks <<" kernel blocks used\n";
		unsigned extraMem = (sem->GetSE(seIndex)->nbSize)*sizeof(unsigned)
				+ THREAD_PER_BLOCK_MED*ADD_MEM_SIZE_PER_BLOCK;
		/*cout << extraMem <<" extra mem per block used\n";
		cout << "ADD_MEM_SIZE_PER_BLOCK: " << ADD_MEM_SIZE_PER_BLOCK <<'\n';
		cout << "arr size: " << ((sem->GetSE(seIndex)->nbSize)/2+2) <<'\n';*/
			//bind texture
		uchar1DTextRef.normalized = false;
		uchar1DTextRef.addressMode[0] = cudaAddressModeClamp;
		uchar1DTextRef.filterMode = cudaFilterModePoint;

		cudaChannelFormatDesc channelDesc = cudaCreateChannelDesc<imDataType>();

		cudaBindTexture(0,&uchar1DTextRef,(void*)srcA,&channelDesc,GetTotalPixelSize()*sizeof(imDataType));
		//cout << "binding texture cuda error:" << cudaGetErrorString(cudaGetLastError()) << '\n';
		
			QueryPerformanceCounter(&start);

		GPUmedian<imDataType><<<blocks,THREAD_PER_BLOCK_MED,extraMem>>>(dst,seIndex,srcA);
		cudaThreadSynchronize();
			QueryPerformanceCounter(&stop);

		return (double)((stop.QuadPart-start.QuadPart)*1000)/frq.QuadPart;

	}else{
		imDataType *values, *values1;
		structEl *se = sem->GetSE(seIndex);
		memset(dst,0,GetTotalPixelSize()*sizeof(imDataType));
		values = new imDataType[se->nbSize];
		values1 = new imDataType[se->nbSize];
		Filter<imDataType>::QsortOpt(NULL,se->nbSize);			//initialize qsort
		Filter<imDataType>::MedianFindOpt(NULL,se->nbSize);		//initialize median
		unsigned long pos;

		//3D
			QueryPerformanceCounter(&start);

		BEGIN_FOR3D(pos)	
			for(unsigned m=0;m < se->nbSize; m++){
				values[m] = values1[m] = srcA[pos + se->nb[m]];
			}		

			Filter<imDataType>::MedianFindOpt(values);
			if(se->nbSize%2){							//odd
				dst[pos] = values[se->nbSize/2];
			}else{
				dst[pos] = ((unsigned)values[se->nbSize/2-1]+values[se->nbSize/2])/2;
			}
		END_FOR3D;

			QueryPerformanceCounter(&stop);

		return (double)((stop.QuadPart-start.QuadPart)*1000)/frq.QuadPart;
	}
}


template <typename imDataType>
float Filter<imDataType>::BES(imDataType* dst, int seIndex, imDataType* srcA, fourthParam<imDataType> p4){
	return 1;
}

#endif