#include <cstdlib>
#include <cstdio>
#include <cmath>

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#define CHANNELS 4
#define GRID_SIZE 1

void scale(void* img, size_t width, size_t height, void* output, size_t newWidth, size_t newHeight)
{

#define RANGE(x,m) max(0,min((m)-1, (x)))
#define P(x,y,c) data[RANGE(x,height)*width*CHANNELS + RANGE(y,width)*CHANNELS + c]

  uint8_t* data = (uint8_t*)img;
  uint8_t* res = (uint8_t*)output;

  double tx = double(width) / double(newWidth);
  double ty = double(height) / double(newHeight);

  for (size_t i = 0; i < newHeight; i++)
  {
    for (size_t j = 0; j < newWidth; j++)
    {
      size_t x = round(tx * j);
      size_t y = round(ty * i);

      for (size_t channel = 0; channel < CHANNELS - 1; channel++)
      {

        uint64_t sum = 0;

        for (size_t yOffset = 1; yOffset <= GRID_SIZE; yOffset++)
        {
          for (size_t xOffset = 1; xOffset <= GRID_SIZE; xOffset++)
          {
            sum += P(y + yOffset, x + xOffset, channel);
            sum += P(y - yOffset, x + xOffset, channel);
            sum += P(y + yOffset, x - xOffset, channel);
            sum += P(y - yOffset, x - xOffset, channel);
          }
        }

        int64_t color = (P(y, x, channel) + sum / (GRID_SIZE * GRID_SIZE * 4)) / 2;

        size_t index = i * CHANNELS * newWidth + j * CHANNELS + channel;

        res[index] = uint8_t(max(0, min(255, color)));

      }

      res[i * CHANNELS * newWidth + j * CHANNELS + 3] = 0xFF;

    }
  }
}

