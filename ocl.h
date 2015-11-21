#ifndef __OCL_H__
#define __OCL_H__

#include "config.h"

#include <stdbool.h>
#ifdef __APPLE_CC__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#include "miner.h"

#define KERNELS_COUNT				43

typedef struct {
	cl_context context;
	cl_kernel kernels[KERNELS_COUNT];
	int ep[KERNELS_COUNT];
	cl_command_queue commandQueue;
	cl_program program;
	cl_mem outputBuffer;
	cl_mem CLbuffer0;
	cl_mem hash_buffer;
	unsigned char cldata[80];
	bool hasBitAlign;
	bool hasOpenCL11plus;
	bool hasOpenCL12plus;
	cl_uint vwidth;
	size_t max_work_size;
	size_t wsize;
	size_t compute_shaders;
} _clState;

extern int clDevicesNum(void);
extern _clState *initCl(unsigned int gpu, char *name, size_t nameSize);
extern bool allocateHashBuffer(unsigned int gpu, _clState *clState);
#endif /* __OCL_H__ */
