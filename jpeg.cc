#include <node.h>
#include <node_buffer.h>
#include <v8.h>
#include <sys/stat.h>

#include <fstream>
#include <cstring>
#include <string>
#include <turbojpeg.h>
#include <jpeglib.h>
#include <node.h>
#include <v8.h>
#include <cmath>

#include <cstdlib>
#include <setjmp.h>

//#include "bicubic.cc"

void scale(void* img, size_t width, size_t height, void* output, size_t newWidth, size_t newHeight);

using namespace v8;

typedef enum ResultCode
{
  OK,
  ERROR
} ResultCode;

struct jpeg_error_manager
{
  struct jpeg_error_mgr pub;

  jmp_buf setjmp_buffer;
};

struct jpeg_error_mgr jerr;

void jpeg_ignore_error(j_common_ptr cinfo)
{

}

void jpeg_error_handler(j_common_ptr cinfo)
{
  cinfo->err->error_exit = jpeg_ignore_error;
}

static void set_scale(struct jpeg_decompress_struct* cinfo, size_t width, size_t height, size_t resize_width, size_t resize_height)
{
  if (resize_width > width && resize_height > height)
  {
    return;
  }

  int coef_width = INT_MAX;
  int coef_height = INT_MAX;

  if (resize_width != 0)
  {
    coef_width = width / resize_width;
  }

  if (resize_height != 0)
  {
    coef_height = height / resize_height;
  }

  int coef = coef_width < coef_height ? coef_width : coef_height;

  if (coef <= 1)
  {
    return;
  }

  if (coef > 16)
  {
    coef = 16;
  }

  cinfo->scale_num = 1;
  cinfo->scale_denom = coef;
  jpeg_calc_output_dimensions(cinfo);
}


#define CINFO_HAS_ERROR (cinfo.err->error_exit != jpeg_error_handler)
#define DECOMPRESS_ERROR_CHECKPOINT if CINFO_HAS_ERROR { jpeg_destroy_decompress(&cinfo); fprintf(stderr, "decompression failed"); return ERROR; }

ResultCode decompress(void* data, size_t size, void** output, size_t& outputWidth, size_t& outputHeight, size_t& outputSize)
{
  struct jpeg_error_mgr mgr;
  jpeg_std_error(&mgr);
  mgr.error_exit = jpeg_error_handler;

  struct jpeg_decompress_struct cinfo;

  jpeg_create_decompress(&cinfo);
  cinfo.err = &mgr;
  jpeg_mem_src(&cinfo, (unsigned char*)data, size);

  DECOMPRESS_ERROR_CHECKPOINT

  if (JPEG_HEADER_OK != jpeg_read_header(&cinfo, TRUE))
  {
    fprintf(stderr, "Failed to read JPEG header");
    jpeg_destroy_decompress(&cinfo);
    return ERROR;
  }

  jpeg_calc_output_dimensions(&cinfo);

  set_scale(&cinfo, cinfo.output_width, cinfo.output_height, outputWidth, outputHeight);

  outputWidth = cinfo.output_width;
  outputHeight = cinfo.output_height;
  cinfo.output_components = 4;
  cinfo.out_color_space = JCS_EXT_RGBA;

  outputSize = cinfo.output_width * cinfo.output_height * cinfo.output_components;

  jpeg_start_decompress(&cinfo);

  DECOMPRESS_ERROR_CHECKPOINT

  *output = malloc(outputSize);

  void* row_pointer[1];

  while (cinfo.output_scanline < cinfo.output_height)
  {

    row_pointer[0] = &((uint8_t*)*output)[cinfo.output_scanline * cinfo.output_width * cinfo.output_components];

    if (1 != jpeg_read_scanlines(&cinfo, (JSAMPARRAY)row_pointer, 1))
    {
      break;
    }

    DECOMPRESS_ERROR_CHECKPOINT

  }

  jpeg_finish_decompress(&cinfo);

  DECOMPRESS_ERROR_CHECKPOINT

  jpeg_destroy_decompress(&cinfo);

  return OK;
}

ResultCode compress(void* data, size_t width, size_t height, int quality, void** output, size_t& outputSize)
{
  tjhandle compressor = tjInitCompress();

  *output = NULL;

  int compressStatus = tjCompress2(compressor, (uint8_t*)data, width, 0, height, TJPF_RGBA,
                                   (uint8_t**)output, &outputSize, TJSAMP_444, quality, TJFLAG_FASTDCT);

  tjDestroy(compressor);

  return (compressStatus == 0) ? OK : ERROR;
}

bool readFile(const char* path, void** buffer, size_t* size)
{
  FILE* f = fopen(path, "r");

  if (f == NULL)
  {
    return false;
  }

  struct stat fileStat;

  if (0 != stat(path, &fileStat))
  {
    fclose(f);
    return false;
  }

  *size = fileStat.st_size;

  *buffer = malloc(fileStat.st_size);

  fread(*buffer, fileStat.st_blksize, fileStat.st_blocks, f);

  fclose(f);

  return true;
}



ResultCode decompressAndScale(const char* filepath, void** buffer, size_t& size, size_t& targetWidth, size_t& targetHeight)
{

  size_t width = targetWidth;
  size_t height = targetHeight;

  void* decompressed = NULL;

  {
    size_t originalSize;
    bool gotFile = readFile(filepath, buffer, &originalSize);
    ResultCode result;

    if (gotFile)
    {
       result = decompress(*buffer, originalSize, &decompressed, width, height, size);
    }

    if (*buffer != NULL)
    {
      free(*buffer);
    }

    if (!gotFile || OK != result)
    {
      if (decompressed != NULL)
      {
        free(decompressed);
      }

      return ERROR;
    }
  }

  if (width <= targetWidth && height <= targetHeight)
  {
    targetWidth = width;
    targetHeight = height;
    *buffer = decompressed;
    return OK;
  }

  if (width > height)
  {
    targetHeight = round(double(height) / width * targetWidth);
  }
  else
  {
    targetWidth = round(double(width) / height * targetHeight);
  }

  size = targetWidth * targetHeight * 4;

  *buffer = malloc(size);

  scale(decompressed, width, height, *buffer, targetWidth, targetHeight);
  free(decompressed);

  return OK;
}

void decompressFunction(const FunctionCallbackInfo<Value>& args)
{
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  std::string filepath(*v8::String::Utf8Value(args[0]));

  size_t targetWidth = args[1]->NumberValue();
  size_t targetHeight = args[2]->NumberValue();
  Local<Function> cb = Local<Function>::Cast(args[3]);

  void* buffer = NULL;
  size_t size = 0;

  ResultCode decompressStatus = decompressAndScale(filepath.c_str(), &buffer, size, targetWidth, targetHeight);

  if (OK != decompressStatus)
  {
    const unsigned argc = 1;
    Local<Value> argv[argc] =
    {
      v8::Null(isolate)
    };

    Local<Function> cb = Local<Function>::Cast(args[1]);
    cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);
  }

  const unsigned argc = 3;

  Local<Value> argv[argc] =
  {
    node::Buffer::New(isolate, (const char*)buffer, size),
    Number::New(isolate, double(targetWidth)),
    Number::New(isolate, double(targetHeight))
  };

  free(buffer);

  cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);

}

void scaleFunction(const FunctionCallbackInfo<Value>& args)
{
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  std::string filepath(*v8::String::Utf8Value(args[0]));

  size_t targetWidth = args[1]->NumberValue();
  size_t targetHeight = args[2]->NumberValue();
  Local<Function> cb = Local<Function>::Cast(args[3]);

  void* buffer = NULL;
  size_t size = 0;

  ResultCode decompressStatus = decompressAndScale(filepath.c_str(), &buffer, size, targetWidth, targetHeight);

  if (OK != decompressStatus)
  {
    const unsigned argc = 1;
    Local<Value> argv[argc] =
    {
      v8::Null(isolate)
    };

    Local<Function> cb = Local<Function>::Cast(args[1]);
    cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);
    return;
  }

  //ssize_t compress(void* data, size_t width, size_t height, int quality, void** output)
  void* jpegData = NULL;
  size_t jpegSize;
  printf("------------------BEFORE COMPRESS-------------------\n");
  ResultCode compressStatus = compress(buffer, targetWidth, targetHeight, 80, &jpegData, jpegSize);
  printf("------------------AFTER  COMPRESS-------------------\n");

  free(buffer);

  if (OK != compressStatus || jpegData == NULL || jpegSize == 0)
  {
    const unsigned argc = 1;
    Local<Value> argv[argc] =
    {
      v8::Null(isolate)
    };

    Local<Function> cb = Local<Function>::Cast(args[1]);
    cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);
    return;
  }

  const unsigned argc = 3;

  Local<Value> argv[argc] =
  {
    node::Buffer::New(isolate, (const char*)jpegData, jpegSize),
    Number::New(isolate, double(targetWidth)),
    Number::New(isolate, double(targetHeight))
  };


  cb->Call(isolate->GetCurrentContext()->Global(), argc, argv);

}

void init(Handle<Object> exports)
{
  NODE_SET_METHOD(exports, "decompress", decompressFunction);
  NODE_SET_METHOD(exports, "scale", scaleFunction);
}

NODE_MODULE(addon, init)

