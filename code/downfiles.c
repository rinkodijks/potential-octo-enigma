/**
 * downfiles.c by Lunarcookies
 * spiderhax example of how download files 
 */

#include "spiderhttpc.h"

int uvl_entry();

#define START_SECTION __attribute__ ((section (".text.start"), naked))

int START_SECTION uvl_start() {
    __asm__ volatile (".word 0xE1A00000");
    uvl_entry();
    __asm__ volatile ("svc 0x03");
}

Result inline downloadPageToSDCard(httpcContext* context, const short* filename, u32 size)
{
    Result ret = 0;
    u32 pos = 0, sz = 0;

    IFile_Open(FILE_LOC, filename, FILE_W);
    *((int *)FILE_LOC + 1) = 0;
    svcSleepThread(0x400000LL);

    while(pos < size)
    {
        sz = size - pos;

        sz = sz > BUF_LEN ? BUF_LEN : sz;

        ret = httpcReceiveData(context, BUF_LOC, sz);

        if(ret == HTTPC_RESULTCODE_DOWNLOADPENDING)
        {
            ret = httpcGetDownloadSizeState(context, &pos, 0);
            if(ret)
                return ret;
            goto filewrite;
        }
        else if(ret)
            return ret;
        else
        {
            pos += sz;
filewrite:  IFile_Write(FILE_LOC, WRITTEN_LOC, BUF_LOC, sz);
            svcSleepThread(0x400000LL);
        }

    }

    return 0;
}

Result downloadPage(Handle httpcHandle, char* url, const short* filename)
{
    Result ret;
    httpcContext context;
    u32 statuscode, size;    

    ret = httpcOpenContext(httpcHandle, &context, url);

    if(!ret)
    {
        ret = httpcBeginRequest(&context);
        if(ret) goto exit;
        ret = httpcGetResponseStatusCode(&context, &statuscode);
        if(ret || statuscode != 200) goto exit;
        ret = httpcGetDownloadSizeState(&context, 0, &size);
        if(ret) goto exit;
        ret = downloadPageToSDCard(&context, filename, size);

exit:   httpcCloseContext(httpcHandle, &context);
    }

    return ret;
}

int uvl_entry()
{
    Handle httpcHandle = 0;
    
    char * url = "http://www.google.com";
    unsigned short * filename = L"dmc:/file.html";

    httpcInit(&httpcHandle);

    downloadPage(httpcHandle, url, (const short *)filename);

    httpcExit(&httpcHandle);

    return 0;
}
