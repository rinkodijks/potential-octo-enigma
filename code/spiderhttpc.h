/**
 * spiderhttpc.h by Lunarcookies
 * Partial implementation of http:C service 
 * for using it with spiderhax
 * mostly stolen from ctrulib
 * https://github.com/smealum/ctrulib
 */

#ifndef _HTTPC_H
#define _HTTPC_H

typedef unsigned char        u8;
typedef unsigned short int  u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef int                 s32;
typedef u32              Handle;
typedef s32              Result;

#define HTTPC_RESULTCODE_DOWNLOADPENDING 0xd840a02b

#define NAKED __attribute__ ((naked))
#define NORETURN __attribute__ ((noreturn))

#define ROP_LOC     ((void*)0x08B88400)
#define BUF_LOC     ((void*)0x18410000)
#define BUF_LEN			 ((u32)0x10000)

#define FILE_R (0x1)
#define FILE_W (0x6)

#define DMC_LOC     ((void*)0x001050B3)
#define FILE_LOC    ((void*)0x08F10000)
#define READ_LOC    ((void*)0x08F10020)
#define WRITTEN_LOC ((void*)0x08F01000)
#define FOPEN_LOC   ((void*)0x0022FE08)
#define FREAD_LOC   ((void*)0x001686DC)
#define FWRITE_LOC  ((void*)0x00168764)

#define GX_SETTEXTURECOPY_LOC    ((void *)0x0011DD48)
#define GSGPU_FLUSHDATACACHE_LOC ((void *)0x00191504)

#define SVCSLEEPTHREAD_LOC       ((void *)0x0023FFE8)
#define SRVGETSERVICEHANDLE_LOC  ((void *)0x00114E00)

int (*IFile_Open)(void *, const short *, int) = FOPEN_LOC;
int (*IFile_Read)(void *, unsigned int *, unsigned int *, unsigned int) = FREAD_LOC;
int (*IFile_Write)(void *, unsigned int *, void *, unsigned int) = FWRITE_LOC;
int (*GX_SetTextureCopy)(void *, void *, unsigned int, int, int, int, int, int) = GX_SETTEXTURECOPY_LOC;
int (*GSPGPU_FlushDataCache)(void *, unsigned int) = GSGPU_FLUSHDATACACHE_LOC;
int (*svcSleepThread)(unsigned long long) = SVCSLEEPTHREAD_LOC;
int (*srvGetServiceHandle)(Handle *, char *, unsigned int, unsigned int) = SRVGETSERVICEHANDLE_LOC;

typedef struct {
	Handle servhandle;
	u32 httphandle;
} httpcContext;

static inline void* getThreadLocalStorage(void) {
    void* ret;
    __asm__ volatile ("mrc p15, 0, %[data], c13, c0, 3" : [data] "=r" (ret));
    return ret;
}

static inline u32* getThreadCommandBuffer(void) {
    return (u32*)((u8*)getThreadLocalStorage() + 0x80);
}

int NAKED svcSendSyncRequest(Handle h) {
    __asm__ volatile ("svc 0x32");
    __asm__ volatile ("bx lr");
}

int NAKED svcCloseHandle(Handle h) {
    __asm__ volatile ("svc 0x23");
    __asm__ volatile ("bx lr");
}

int strlen(char* str) {
    int l=0;
    while(*(str++))l++;
    return l;
}

Result HTTPC_Initialize(Handle handle)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x10044; //request header code
	cmdbuf[1]=0x1000; //unk
	cmdbuf[2]=0x20;//processID header, following word is set to processID by the arm11kernel.
	cmdbuf[4]=0;
	cmdbuf[5]=0;//Some sort of handle.
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result HTTPC_CreateContext(Handle handle, char* url, Handle* contextHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	u32 l=strlen(url)+1;

	cmdbuf[0]=0x20082; //request header code
	cmdbuf[1]=l;
	cmdbuf[2]=0x01; // GET
	cmdbuf[3]=(l<<4)|0xA;
	cmdbuf[4]=(u32)url;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;
	
	if(contextHandle)*contextHandle=cmdbuf[2];

	return cmdbuf[1];
}

Result HTTPC_InitializeConnectionSession(Handle handle, Handle contextHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x80042; //request header code
	cmdbuf[1]=contextHandle;
	cmdbuf[2]=0x20; //unk, constant afaict
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result HTTPC_CloseContext(Handle handle, Handle contextHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x30040; //request header code
	cmdbuf[1]=contextHandle;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result HTTPC_BeginRequest(Handle handle, Handle contextHandle)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x90040; //request header code
	cmdbuf[1]=contextHandle;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result HTTPC_ReceiveData(Handle handle, Handle contextHandle, u8* buffer, u32 size)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0xB0082; //request header code
	cmdbuf[1]=contextHandle;
	cmdbuf[2]=size;
	cmdbuf[3]=(size<<4)|12;
	cmdbuf[4]=(u32)buffer;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	return cmdbuf[1];
}

Result HTTPC_GetDownloadSizeState(Handle handle, Handle contextHandle, u32* downloadedsize, u32* contentsize)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x60040; //request header code
	cmdbuf[1]=contextHandle;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	if(downloadedsize)*downloadedsize = cmdbuf[2];
	if(contentsize)*contentsize = cmdbuf[3];

	return cmdbuf[1];
}

Result HTTPC_GetResponseStatusCode(Handle handle, Handle contextHandle, u32* out)
{
	u32* cmdbuf=getThreadCommandBuffer();

	cmdbuf[0]=0x220040; //request header code
	cmdbuf[1]=contextHandle;
	
	Result ret=0;
	if((ret=svcSendSyncRequest(handle)))return ret;

	*out = cmdbuf[2];

	return cmdbuf[1];
}

Result inline httpcInit(Handle* __httpc_servhandle)
{
	Result ret=0;
	if(__httpc_servhandle==0)return 0;
	if(*__httpc_servhandle)return 0;
	if((ret=srvGetServiceHandle(__httpc_servhandle, "http:C", 6, 0)))return ret;

	ret = HTTPC_Initialize(*__httpc_servhandle);
	if(ret!=0)return ret;

	return 0;
}

void inline httpcExit(Handle* __httpc_servhandle)
{
	if(__httpc_servhandle==0)return;
	if(*__httpc_servhandle==0)return;

	svcCloseHandle(*__httpc_servhandle);
}

Result inline httpcOpenContext(Handle __httpc_servhandle, httpcContext *context, char* url)
{
	Result ret=0;

	ret = HTTPC_CreateContext(__httpc_servhandle, url, &context->httphandle);
	if(ret!=0)return ret;

	ret = srvGetServiceHandle(&context->servhandle, "http:C", 6, 0);
	if(ret!=0) {
		HTTPC_CloseContext(__httpc_servhandle, context->httphandle);
		return ret;
        }

	ret = HTTPC_InitializeConnectionSession(context->servhandle, context->httphandle);
	if(ret!=0) {
		svcCloseHandle(context->servhandle);
		HTTPC_CloseContext(__httpc_servhandle, context->httphandle);
		return ret;
        }

	return 0;
}

Result inline httpcCloseContext(Handle __httpc_servhandle, httpcContext *context)
{
	Result ret=0;

	svcCloseHandle(context->servhandle);
	ret = HTTPC_CloseContext(__httpc_servhandle, context->httphandle);

	return ret;
}

Result inline httpcBeginRequest(httpcContext *context)
{
	return HTTPC_BeginRequest(context->servhandle, context->httphandle);
}

Result inline httpcReceiveData(httpcContext *context, u8* buffer, u32 size)
{
	return HTTPC_ReceiveData(context->servhandle, context->httphandle, buffer, size);
}

Result inline httpcGetDownloadSizeState(httpcContext *context, u32* downloadedsize, u32* contentsize)
{
	return HTTPC_GetDownloadSizeState(context->servhandle, context->httphandle, downloadedsize, contentsize);
}

Result inline httpcGetResponseStatusCode(httpcContext *context, u32* out)
{
		return HTTPC_GetResponseStatusCode(context->servhandle, context->httphandle, out);
}

#endif /* _HTTPC_H */